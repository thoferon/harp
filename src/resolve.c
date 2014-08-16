#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include <memory.h>
#include <log.h>
#include <request.h>
#include <primp_config.h>
#include <list.h>
#include <response.h>

#include <resolve.h>

#define READ_BUFFER_SIZE 1024

/**
 * Poll and write
 * \return See return values of proxy.
 */
inline int ssend(int sock, char *buf, ssize_t count) {
 ssize_t written = 0;
  do {
    struct pollfd pollfds[1];
    pollfds[0].fd     = sock;
    pollfds[0].events = POLLOUT;

    int rc = poll(pollfds, 1, 3 * 60 * 1000);
    switch(rc) {
    case -1: return -1;
    case 0:  return -2;
    }

    ssize_t count2 = send(sock, buf + written, count - written, MSG_DONTWAIT);
    if(count2 == -1) {
      if(errno == EAGAIN) {
        continue;
      } else {
        return -1;
      }
    }
    written += count2;
  } while(written < count);

  // proxy() will return this
  return 1;
}

resolution_strategy_t resolve_request(request_t *request, config_t *config) {
  if(config == NULL) {
    return RESOLUTION_STRATEGY_400;
  }

  resolution_strategy_t strategy = RESOLUTION_STRATEGY_500;

  list_t *current;
  LISTFOREACH(current, config->resolvers) {
    resolver_t *resolver = (resolver_t*)current->element;
    switch(resolver->type) {
    case RESOLVER_TYPE_STATIC_PATH:
      strategy = resolve_with_static_path(request, resolver->static_path);
      break;
    case RESOLVER_TYPE_SERVER:
      strategy = resolve_with_server(request, resolver->server);
      break;
    }
    if(strategy == 0) { break; }
  }

  return strategy;
}

void execute_fallback_strategy(int socket, resolution_strategy_t strategy) {
  http_status_t status;
  char *body; // FIXME: Put the bodies in files and load them at compile time

  switch(strategy) {
  case RESOLUTION_STRATEGY_400:
    status = HTTP_STATUS_400;
    body = "<!DOCTYPE html><html><body>400 Bad Request</body></html>";
    break;
  case RESOLUTION_STRATEGY_404:
    status = HTTP_STATUS_404;
    body = "<!DOCTYPE html><html><body>404 Not Found</body></html>";
    break;
  case RESOLUTION_STRATEGY_500:
    status = HTTP_STATUS_500;
    body = "<!DOCTYPE html><html><body>500 Internal Error</body></html>";
    break;
  case RESOLUTION_STRATEGY_502:
    status = HTTP_STATUS_502;
    body = "<!DOCTYPE html><html><body>502 Bad Gateway</body></html>";
    break;
  case RESOLUTION_STRATEGY_504:
    status = HTTP_STATUS_504;
    body = "<!DOCTYPE html><html><body>504 Gateway Timeout</body></html>";
    break;
  default:
    return; // Send nothing and impolitely close the connection later on
  }

  size_t body_len = strlen(body);
  send_response_headers(socket, status, body_len, MIME_TYPE_HTML);

  int rc = ssend(socket, body, body_len);
  // If it's anything else than -1, it doesn't matter,
  // it's gonna get closed after anyway.
  if(rc == -1) {
    logerror("execute_fallback_strategy:ssend");
  }
}

/*
 * Resolve with static_path serving static files
 */

// This is supposed to stay really basic.
// The recommended way should be to proxy every requests.
resolution_strategy_t resolve_with_static_path(request_t *request, char *static_path) {
  int rc;

  size_t static_path_len  = strlen(static_path);
  size_t request_path_len = strlen(request->info->path);

  // + 12 for '\0' and /index.html
  char *local_path = (char*)smalloc(static_path_len + request_path_len + 12);
  memcpy(local_path, static_path, static_path_len);
  memcpy(local_path + static_path_len, request->info->path, request_path_len);
  local_path[static_path_len + request_path_len] = '\0';

  struct stat dirstat;
  if((rc = stat(local_path, &dirstat)) == -1) {
    if(errno != ENOENT) {
      logerror("resolve_with_static_path:stat");
    }
    free(local_path);
    return errno == ENOENT ? RESOLUTION_STRATEGY_404 : RESOLUTION_STRATEGY_500;
  }

  FILE *f;
  // Ugly but it's only two alternatives
  if(S_ISDIR(dirstat.st_mode) == 0) {
    f = fopen(local_path, "r");
  } else {
    memcpy(local_path + static_path_len + request_path_len, "/index.htm", 10);
    local_path[static_path_len + request_path_len + 10] = '\0';
    f = fopen(local_path, "r");

    if(f == NULL) {
      local_path[static_path_len + request_path_len + 10] = 'l';
      local_path[static_path_len + request_path_len + 11] = '\0';
      f = fopen(local_path, "r");
    }
  }
  if(f == NULL) {
    return RESOLUTION_STRATEGY_404;
  }
  free(local_path);

#define CLOSEFILE() do {                                \
    rc = fclose(f);                                     \
    if(rc == -1) {                                      \
      logerror("resolve_with_static_path:fclose");      \
      return RESOLUTION_STRATEGY_500;                   \
    }                                                   \
  } while(0)                                            \

  int socket = request->aconnection->socket;

  int fd = fileno(f);
  struct stat fs;
  if(fstat(fd, &fs) == -1) {
    logerror("resolve_with_static_path:fstat");
    CLOSEFILE();
    return RESOLUTION_STRATEGY_500;
  }

  mime_type_t mime_type = get_mime_type(request->info->path);

  rc = send_response_headers(socket, HTTP_STATUS_200, fs.st_size, mime_type);
  if(rc == -1) {
    return RESOLUTION_STRATEGY_500;
  }

  int count;
  char buffer[READ_BUFFER_SIZE];
  while((count = fread(&buffer, 1, READ_BUFFER_SIZE, f)) > 0) {
    while((rc = send(socket, buffer, count, MSG_DONTWAIT)) == -1 && errno == EAGAIN);
    if(rc == -1) {
      logerror("resolve_with_static_path:send");
      CLOSEFILE();
      return RESOLUTION_STRATEGY_500;
    }
  }
  // FIXME: Check errors with ferror()

  CLOSEFILE();
  return 0;
}

/*
 * Resolve with server by proxying the request
 */

/**
 * Proxy from one socket to another if there is something to read from the first.
 * \return -2 when it timed out
 * \return -1 when an error occured
 * \return 0 when the read socket is closed
 * \return 1 when the transmission was successful
 */
int proxy(int from, int to) {
  char buf[READ_BUFFER_SIZE];
  ssize_t count = recv(from, &buf, READ_BUFFER_SIZE, MSG_DONTWAIT);

  if(count == -1) {
    if(errno == EINTR || errno == EAGAIN) {
      return 1;
    } else {
      logerror("proxy:recv");
      return -1;
    }
  } else if(count == 0) {
    return 0;
  } else {
    int rc = ssend(to, buf, count);
    if(rc == -1) {
      logerror("proxy:ssend");
    }
    return rc;
  }
}

resolution_strategy_t resolve_with_server(request_t *request, server_t *server) {
  int sock;
  int rc;

  struct addrinfo hints, *addrinfos, *current;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE | AI_NUMERICSERV;

  char *port_string = (char*)smalloc(6);
  snprintf(port_string, 6, "%i", server->port);

  while((rc = getaddrinfo(server->hostname, port_string, &hints, &addrinfos))
        == EAI_AGAIN);
  if(rc != 0) {
    logmsg(LOG_ERR, "resolve_with_server:getaddrinfo: %s\n", gai_strerror(rc));
    return RESOLUTION_STRATEGY_502;
  }

#define RETURNERROR(code) do {                                  \
    rc = close(sock);                                           \
    if(rc == -1) { logerror("resolve_with_server:close"); }     \
    return (code);                                              \
  } while(0)

  if(addrinfos != NULL) {
    rc = -1;
    for(current = addrinfos; current != NULL; current = current->ai_next) {
      sock = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
      if(sock == -1) {
        logerror("resolve_with_server:socket");
        return RESOLUTION_STRATEGY_500;
      }

      rc = connect(sock, current->ai_addr, current->ai_addrlen);
      if(rc == 0) { break; }
      if(errno == EINTR) {
        struct pollfd pollfds[1];
        pollfds[0].fd     = sock;
        pollfds[0].events = POLLOUT;

        rc = poll(pollfds, 1, 3 * 60 * 1000);

        if(rc == -1) {
          logerror("resolve_with_server:poll");
          RETURNERROR(RESOLUTION_STRATEGY_500);
        } else if(rc == 0) {
          RETURNERROR(RESOLUTION_STRATEGY_504);
        }
      }
    }

    freeaddrinfo(addrinfos);
    if(rc == -1) {
      logerror("resolve_with_server:connect");
      if(errno == ETIMEDOUT) {
        RETURNERROR(RESOLUTION_STRATEGY_504);
      } else {
        RETURNERROR(RESOLUTION_STRATEGY_502);
      }
    }

  } else {
    logmsg(LOG_ERR, "resolve_with_server:getaddrinfo: Empty result\n");
    return RESOLUTION_STRATEGY_502;
  }

  int flags = fcntl(sock, F_GETFL, NULL);
  if(flags == -1) {
    logerror("resolve_with_server:fcntl with F_GETFL");
    RETURNERROR(RESOLUTION_STRATEGY_500);
  }

  rc = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
  if(rc == -1) {
    logerror("resolve_with_server:fcntl with F_SETFL");
    RETURNERROR(RESOLUTION_STRATEGY_500);
  }

  rc = ssend(sock, request->buffer, request->buffer_size);
  if(rc == -1) {
    logerror("resolve_with_request:ssend");
    RETURNERROR(RESOLUTION_STRATEGY_502);
  }
  if(rc == -2) {
    RETURNERROR(RESOLUTION_STRATEGY_504);
  }

  int rc1 = -1, rc2 = -1;
  int has_sent_to_client = 0;

#define RETURNERROR2(code) RETURNERROR(has_sent_to_client ? RESOLUTION_STRATEGY_CLOSE : (code))

  do {
    struct pollfd pollfds[2];
    pollfds[0].fd     = sock;
    pollfds[0].events = rc1 == 0 ? 0 : POLLIN;
    pollfds[1].fd     = request->aconnection->socket;
    pollfds[1].events = rc2 == 0 ? 0 : POLLIN;

    rc = poll(pollfds, 2, 3 * 60 * 1000);
    if(rc == -1) {
      logerror("resolve_with_server:poll");
      RETURNERROR2(RESOLUTION_STRATEGY_500);
    }
    if(rc == 0) {
      RETURNERROR2(RESOLUTION_STRATEGY_504);
    }

    if(rc1 != 0 && pollfds[0].revents == POLLIN) {
      rc1 = proxy(sock, request->aconnection->socket);
      if(rc1 == 1) { has_sent_to_client = 1; }
      switch(rc1) {
      case -1: RETURNERROR2(RESOLUTION_STRATEGY_502);
      case -2: RETURNERROR2(RESOLUTION_STRATEGY_504);
      }
    }

    if(rc2 != 0 && pollfds[1].revents == POLLIN) {
      rc2 = proxy(request->aconnection->socket, sock);
      switch(rc2) {
      case -1: RETURNERROR2(RESOLUTION_STRATEGY_CLOSE);
      case -2: RETURNERROR2(RESOLUTION_STRATEGY_504);
      }
    }

  } while(rc1 != 0 || rc2 != 0);

  return 0;
}
