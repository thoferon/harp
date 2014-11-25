#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>

#ifdef HAVE_CONFIG
#include <config.h>
#endif

#include <smem.h>
#include <log.h>
#include <request.h>
#include <harp.h>
#include <response.h>
#include <utils.h>

#include <resolve.h>

#define READ_BUFFER_SIZE 1024

resolution_strategy_t resolve_request(request_t *request,
                                      harp_config_t *config) {
  if(config == NULL) {
    return RESOLUTION_STRATEGY_400;
  }

  resolution_strategy_t strategy = RESOLUTION_STRATEGY_500;

  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, config->resolvers) {
    harp_resolver_t *resolver = (harp_resolver_t*)current->element;
    switch(resolver->type) {
    case HARP_RESOLVER_TYPE_STATIC_PATH:
      strategy = resolve_with_static_path(request, resolver->static_path);
      break;
    case HARP_RESOLVER_TYPE_SERVER:
      strategy = resolve_with_server(request, resolver->server, config);
      break;
    }
    if(strategy == 0) { break; }
  }

  return strategy;
}

void execute_fallback_strategy(request_t *request,
                               resolution_strategy_t strategy) {
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
  int rc = send_response_headers(request, status, body_len, MIME_TYPE_HTML);
  if(rc == -1) { return; }

  int sock = request->aconnection->socket;
  send_with_request(request, sock, body, body_len);
}

/*
 * Resolve with static_path serving static files
 */

// This is supposed to stay really basic.
// The recommended way should be to proxy every requests.
resolution_strategy_t resolve_with_static_path(request_t *request,
                                               char *static_path) {
  int rc;

  size_t static_path_len  = strlen(static_path);
  size_t request_path_len = strlen(request->info->path);

  char *local_path = (char*)smalloc(static_path_len + request_path_len
                                    + strlen("/index.html") + 1);
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
    memcpy(local_path + static_path_len + request_path_len,
           "/index.html", strlen("/index.html"));
    local_path[static_path_len + request_path_len
               + strlen("/index.html")] = '\0';
    f = fopen(local_path, "r");

    if(f == NULL) {
      local_path[static_path_len + request_path_len
                 + strlen("/index.htm")] = '\0';
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
      return RESOLUTION_STRATEGY_CLOSE;                 \
    }                                                   \
  } while(0)                                            \

  int fd = fileno(f);
  struct stat fs;
  if(fstat(fd, &fs) == -1) {
    logerror("resolve_with_static_path:fstat");
    CLOSEFILE();
    return RESOLUTION_STRATEGY_500;
  }

  mime_type_t mime_type = get_mime_type(request->info->path);

  rc = send_response_headers(request, HTTP_STATUS_200, fs.st_size, mime_type);
  if(rc == -1) {
    return RESOLUTION_STRATEGY_CLOSE;
  }

  int count;
  char buffer[READ_BUFFER_SIZE];
  int sock = request->aconnection->socket;

  while((count = fread(&buffer, 1, READ_BUFFER_SIZE, f)) > 0) {
    int rc = send_with_request(request, sock, buffer, count);
    if(rc == -1) {
      CLOSEFILE();
      return RESOLUTION_STRATEGY_CLOSE;
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
 * \return -1 when an error occured
 * \return 0 when the read socket is closed
 * \return 1 when the transmission was successful
 */
int proxy(request_t *request, int from, int to) {
  char buf[READ_BUFFER_SIZE];
  ssize_t count = recv_with_request(request, from, &buf, READ_BUFFER_SIZE);

  if(count > 0) {
    int rc = send_with_request(request, to, buf, count);
    return rc == -1 ? -1 : 1;
  } else {
    return count;
  }
}

int send_x_tags_header(request_t *, int, harp_list_t *);

resolution_strategy_t resolve_with_server(request_t *request,
                                          harp_server_t *server,
                                          harp_config_t *config) {
  int rc, server_sock;

  struct addrinfo hints, *addrinfos, *current;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_NUMERICSERV;

  char port_string[6];
  snprintf(port_string, 6, "%i", server->port);

  while((rc = getaddrinfo(server->hostname, port_string, &hints, &addrinfos))
        == EAI_AGAIN);
  if(rc != 0) {
    logmsg(LOG_ERR, "resolve_with_server:getaddrinfo: %s\n", gai_strerror(rc));
    return RESOLUTION_STRATEGY_502;
  }

#define RETURNERROR(code) do {                                  \
    rc = close(server_sock);                                    \
    if(rc == -1) { logerror("resolve_with_server:close"); }     \
    return (code);                                              \
  } while(0)

  if(addrinfos != NULL) {
    rc = -1;
    for(current = addrinfos; current != NULL; current = current->ai_next) {
      server_sock = socket(current->ai_family, current->ai_socktype,
                           current->ai_protocol);
      if(server_sock == -1) {
        logerror("resolve_with_server:socket");
        freeaddrinfo(addrinfos);
        return RESOLUTION_STRATEGY_500;
      }

      rc = connect(server_sock, current->ai_addr, current->ai_addrlen);
      if(rc == 0) { break; }
      if(errno == EINTR) {
        struct pollfd pollfds[1];
        pollfds[0].fd     = server_sock;
        pollfds[0].events = POLLOUT;

        while((rc = poll(pollfds, 1, 100)) == -1 && errno == EINTR);

        if(rc == -1) {
          logerror("resolve_with_server:poll");
          freeaddrinfo(addrinfos);
          RETURNERROR(RESOLUTION_STRATEGY_500);
        } else if(rc > 0) {
          break;
        }
      }

      close(server_sock);
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

  rc = set_nonblocking(server_sock);
  if(rc == -1) {
    RETURNERROR(RESOLUTION_STRATEGY_500);
  }

  // Transfer

  char *buffer_remaining       = request->buffer;
  size_t buffer_remaining_size = request->buffer_size;

  int i;
  for(i = 0; i < request->buffer_size; i++) {
    if(request->buffer[i] == '\r' && request->buffer[i+1] == '\n') {
      rc = send_with_request(request, server_sock, request->buffer, i + 2);
      if(rc == -1) {
        RETURNERROR(RESOLUTION_STRATEGY_502);
      }
      buffer_remaining      += i + 2;
      buffer_remaining_size -= i + 2;

      rc = send_x_tags_header(request, server_sock, config->tags);
      if(rc == -1) {
        RETURNERROR(RESOLUTION_STRATEGY_502);
      }

      break;
    }
  }

  rc = send_with_request(request, server_sock,
                         buffer_remaining, buffer_remaining_size);
  if(rc == -1) {
    RETURNERROR(RESOLUTION_STRATEGY_502);
  }

  int rc1 = -1, rc2 = -1;
  int has_sent_to_client = 0;
  int client_sock = request->aconnection->socket;

#define RETURNERROR2(code) RETURNERROR(has_sent_to_client ? RESOLUTION_STRATEGY_CLOSE : (code))

  do {
    struct pollfd pollfds[2];
    pollfds[0].fd     = server_sock;
    pollfds[0].events = (rc1 == 0 ? 0 : POLLIN);
    pollfds[1].fd     = client_sock;
    pollfds[1].events = (rc2 == 0 ? 0 : POLLIN);

    rc = poll_with_request(request, pollfds, 2);
    if(rc == -1) {
      RETURNERROR2(RESOLUTION_STRATEGY_502);
    }

    if(rc1 != 0 && (pollfds[0].revents & POLLIN) == POLLIN) {
      rc1 = proxy(request, server_sock, client_sock);
      if(rc1 == 1) {
        has_sent_to_client = 1;
      } else if(rc1 == -1) {
        RETURNERROR2(RESOLUTION_STRATEGY_502);
      }
    }

    if(rc2 != 0 && (pollfds[1].revents & POLLIN) == POLLIN) {
      rc2 = proxy(request, client_sock, server_sock);
      if(rc2 == -1) {
        RETURNERROR2(RESOLUTION_STRATEGY_CLOSE);
      }
    }

  } while(rc1 != 0 || rc2 != 0);

  rc = close(server_sock);
  if(rc != 0) {
    logerror("resolve_with_server:close");
  }

  return 0;
}

int send_x_tags_header(request_t *request, int sock, harp_list_t *tags) {
  int rc;
  harp_list_t *current;

  size_t list_len = 0;
  HARP_LIST_FOR_EACH(current, tags) {
    char *tag = (char*)current->element;
    list_len += strlen(tag);
    if(current->next != HARP_EMPTY_LIST) {
      list_len += strlen(",");
    }
  }

  char *header_prefix = "X-Tags: ";
  size_t size = strlen(header_prefix) + list_len + strlen("\r\n");
  char *x_tags_header = (char*)smalloc(size);
  size_t so_far = 0;

  strncpy(x_tags_header, header_prefix, size - so_far);
  so_far += strlen(header_prefix);

  HARP_LIST_FOR_EACH(current, tags) {
    char *tag = (char*)current->element;
    strncat(x_tags_header + so_far, tag, size - so_far);
    so_far += strlen(tag);
    if(current->next != HARP_EMPTY_LIST) {
      x_tags_header[so_far++] = ',';
    }
  }

  strncat(x_tags_header + so_far, "\r\n", size - so_far);
  so_far += 2;

  rc = send_with_request(request, sock, x_tags_header, size);
  free(x_tags_header);

  return rc;
}
