#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <memory.h>
#include <request.h>

/*
 * Create/Destroy
 */

// FIXME: Check RFC for spaces between Host: and the hostname
// FIXME: If the hostname is an IPv6 address, it won't work
int read_info(int socket, char **path_ptr, char **hostname_ptr,
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
          perror("read_hostname:realloc");
          RETURNFAILURE();
        }
        buffer         = new_buffer;
        allocated_size = new_allocated_size;
      }

      int count;
      while((count = recv(socket, (char*)buffer + i, allocated_size - buffer_size, MSG_DONTWAIT)) == -1 && errno == EAGAIN);
      if(count == 0 || count == -1) {
        if(count == -1) {
          perror("read_hostname:recv");
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

  if(read_info(aconnection->socket, &path, &hostname, &buffer, &buffer_size) == 0) {
    request_info_t *request_info = make_request_info(path, hostname, aconnection->port);
    request_t *request = make_request(request_info, aconnection, buffer, buffer_size);

    return request;
  } else {
    return NULL;
  }
}

void destroy_request(request_t *request) {
  destroy_aconnection(request->aconnection);
  free_request(request);
}

/*
 * Functions to create structures
 */

request_info_t *make_request_info(char *path, char *hostname, int port) {
  request_info_t *request_info = (request_info_t*)smalloc(sizeof(struct request_info));
  request_info->path     = path;
  request_info->hostname = hostname;
  request_info->port     = port;
  return request_info;
}

request_t *make_request(request_info_t *request_info, aconnection_t *aconnection,
                        char *buffer, size_t buffer_size) {
  request_t *request = (request_t*)smalloc(sizeof(struct request));
  request->info        = request_info;
  request->aconnection = aconnection;
  request->buffer      = buffer;
  request->buffer_size = buffer_size;
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
