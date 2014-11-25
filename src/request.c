#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <poll.h>

#include <smem.h>
#include <log.h>

#include <request.h>

#define REQUEST_MIN_TIME      60   /* seconds */
#define REQUEST_MAX_IDLE_TIME 120  /* seconds */
#define REQUEST_MIN_SPEED     2048 /* bytes per second */

#define POLL_TIMEOUT 100

/*
 * Create/Destroy
 */

// FIXME: Check RFC for spaces between Host: and the hostname
// FIXME: If the hostname is an IPv6 address, it won't work
int read_info(int sock, char **path_ptr, char **hostname_ptr,
              char **buffer_ptr, size_t *buffer_size_ptr) {
#define SEARCH_PATH_START     1
#define SEARCH_PATH_END       2
#define SEARCH_HOSTNAME_START 3
#define SEARCH_HOSTNAME_END   4

  size_t buffer_size    = 0;
  size_t allocated_size = 0;
  char *path            = NULL;
  char *hostname        = NULL;
  char *buffer          = NULL;
  int i                 = 0;
  int mode              = SEARCH_PATH_START;
  int start             = 0;

  char *hostname_search      = "\r\nHost: ";
  size_t hostname_search_len = strlen(hostname_search);

#define RETURNFAILURE() {    \
    free(buffer);            \
    *hostname_ptr    = NULL; \
    *buffer_ptr      = NULL; \
    *buffer_size_ptr = 0;    \
    return -1;               \
  }

  while(true) {
    size_t chars_left = buffer_size - i;
    // We need to have enough characters for Host: or CRLF
    if((mode == SEARCH_PATH_START && chars_left < 1)
       || (mode == SEARCH_PATH_END && chars_left < 1)
       || (mode == SEARCH_HOSTNAME_START && chars_left < hostname_search_len)
       || (mode == SEARCH_HOSTNAME_END && chars_left < 2)) {

      if(allocated_size == buffer_size) {
        size_t new_allocated_size = allocated_size + 512;
        char *new_buffer;
        if((new_buffer = (char*)realloc(buffer, new_allocated_size)) == NULL) {
          logerror("read_info:realloc");
          RETURNFAILURE();
        }
        buffer         = new_buffer;
        allocated_size = new_allocated_size;
      }

      struct pollfd pollfds[1];
      pollfds[0].fd     = sock;
      pollfds[0].events = POLLIN;

      /* We can't use recv_with_request here as
       * the request doesn't exist yet.
       */
      int count;
      while((count = recv(sock, (char*)buffer + i,
                          allocated_size - buffer_size, MSG_DONTWAIT)) == -1
            && errno == EAGAIN) {
        int rc = poll(pollfds, 1, REQUEST_MIN_TIME);
        if(rc == -1) {
          logerror("read_info:poll");
          RETURNFAILURE();
        } else if(rc == 0) {
          RETURNFAILURE();
        }
      }
      if(count <= 0) {
        if(count == -1) {
          logerror("read_info:recv");
        }
        RETURNFAILURE();
      }
      buffer_size += count;

      chars_left = buffer_size - i;
    }

    if(mode == SEARCH_PATH_START) {
      if(buffer[i] == ' ') {
        mode = SEARCH_PATH_END;
        start = i + 1;
      }
    } else if(mode == SEARCH_PATH_END) {
      if(buffer[i] == '?' || buffer[i] == ' ') {
        size_t path_size = i - start;
        path = (char*)smalloc(path_size + 1);
        memcpy(path, buffer + start, path_size);
        path[path_size] = '\0';
        mode = SEARCH_HOSTNAME_START;
      }
    } else if(mode == SEARCH_HOSTNAME_START) {
      // chars_left >= search_len and we should compare only the beginning
      if(strncmp(buffer + i, hostname_search, hostname_search_len) == 0) {
        mode = SEARCH_HOSTNAME_END;
        i += hostname_search_len;
        start = i;
        continue; // avoid i++ at the end
      }
    } else {
      if(buffer[i] == ':' || (buffer[i] == '\r' && buffer[i+1] == '\n')) {
        size_t hostname_size = i - start;
        hostname = (char*)smalloc(hostname_size + 1);
        memcpy(hostname, buffer + start, hostname_size);
        hostname[hostname_size] = '\0';
        break; // we're done
      }
    }

    i++;
  }

  *path_ptr        = path;
  *hostname_ptr    = hostname;
  *buffer_ptr      = buffer;
  *buffer_size_ptr = buffer_size;

  return 0;
}

request_t *create_request(aconnection_t *aconnection) {
  char *path, *hostname, *buffer;
  size_t buffer_size;
  request_t *request = NULL;

  time_t now = time(NULL);
  if(now == -1) {
    logerror("create_request:time");
    return NULL;
  }

  if(read_info(aconnection->socket, &path, &hostname,
               &buffer, &buffer_size) == 0) {
    request_info_t *request_info =
      make_request_info(path, hostname, aconnection->port);
    request = make_request(request_info, aconnection, buffer, buffer_size, now);
  }

  return request;
}

void destroy_request(request_t *request) {
  destroy_aconnection(request->aconnection);
  free_request(request);
}

/*
 * Functions to create structures
 */

request_info_t *make_request_info(char *path, char *hostname, int port) {
  request_info_t *request_info =
    (request_info_t*)smalloc(sizeof(struct request_info));
  request_info->path     = path;
  request_info->hostname = hostname;
  request_info->port     = port;
  return request_info;
}

request_t *make_request(request_info_t *request_info,
                        aconnection_t *aconnection,
                        char *buffer, size_t buffer_size, time_t start) {
  request_t *request = (request_t*)smalloc(sizeof(struct request));

  request->info              = request_info;
  request->aconnection       = aconnection;
  request->buffer            = buffer;
  request->buffer_size       = buffer_size;
  request->start             = start;
  request->last_transfer     = start;
  request->total_transferred = 0;

  return request;
}

/*
 * Functions to free memory
 */

void free_request_info(request_info_t *request_info) {
  free(request_info->path);
  free(request_info->hostname);
  free(request_info);
}

// It does not free the aconnection, see destroy_request.
void free_request(request_t *request) {
  free_request_info(request->info);
  free(request->buffer);
  free(request);
}

/*
 * Request validity
 */

int update_request(request_t *request, size_t transferred) {
  time_t now = time(NULL);
  if(now == -1) {
    logerror("update_request:time");
    return -1;
  }

  request->last_transfer      = now;
  request->total_transferred += transferred;

  return 0;
}

bool is_still_valid(request_t *request) {
  time_t now = time(NULL);
  if(now == -1) {
    logerror("is_still_valid:time");
    return false;
  }

  time_t spent      = now - request->start;
  time_t idle_time  = now - request->last_transfer;
  int average_speed = request->total_transferred / (spent == 0 ? 1 : spent);

  return spent <= REQUEST_MIN_TIME
    || (average_speed >= REQUEST_MIN_SPEED
        && idle_time <= REQUEST_MAX_IDLE_TIME);
}

/*
 * Transfer data
 */

int poll_with_request(request_t *request, struct pollfd *pollfds, nfds_t nfds) {
  while(true) {
    if(!is_still_valid(request)) {
      return -1;
    }

    int rc = poll(pollfds, nfds, POLL_TIMEOUT);
    if(rc == -1) {
      if(errno == EINTR) {
        continue;
      } else {
        logerror("poll_with_request:poll");
        return -1;
      }
    } else {
      return rc;
    }
  }
}

int send_with_request(request_t *request, int sock,
                      const void *msg, size_t len) {
  int count, rc;
  size_t so_far;

  struct pollfd pollfds[1];
  pollfds[0].fd     = sock;
  pollfds[0].events = POLLOUT;

  for(so_far = 0; so_far < len; ) {
    rc = poll_with_request(request, pollfds, 1);
    if(rc == -1) {
      return -1;
    }

    if((pollfds[0].revents & POLLOUT) == POLLOUT) {
      count = send(sock, msg + so_far , len - so_far,
                   MSG_DONTWAIT | MSG_NOSIGNAL);

      if(count == -1) {
        if(errno != EAGAIN && errno != EINTR) {
          logerror("send_with_request:send");
          return -1;
        }
      } else {
        update_request(request, count);
        so_far += count;
      }
    }
  }

  return 0;
}

ssize_t recv_with_request(request_t *request, int sock, void *buf, size_t len) {
  int count, rc;

  struct pollfd pollfds[1];
  pollfds[0].fd     = sock;
  pollfds[0].events = POLLIN;

  while(true) {
    rc = poll_with_request(request, pollfds, 1);
    if(rc == -1) {
      return -1;
    }

    if((pollfds[0].revents & POLLIN) == POLLIN) {
      count = recv(sock, buf, len, MSG_DONTWAIT | MSG_NOSIGNAL);

      if(count == -1) {
        if(errno != EAGAIN && errno != EINTR) {
          logerror("recv_with_request:recv");
          return -1;
        }
      } else {
        return count;
      }
    }
  }
}
