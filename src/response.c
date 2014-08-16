#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define PACKAGE_STRING "primp"
#endif

#include <log.h>

#include <response.h>

// FIXME: Calculate static string lengths at compile time

int send_http_status(int, http_status_t);
int send_server_header(int);
int send_connection_header(int);
int send_content_length_header(int, int);
int send_content_type_header(int, mime_type_t);
int send_extra_crlf(int);

int send_response_headers(int socket, http_status_t status,
                          int content_length, mime_type_t mime_type) {
#define RETURNIFFAILURE(exp) do { rc = (exp); if(rc != 0) { return rc; } } while(0)
  int rc;
  RETURNIFFAILURE(send_http_status(socket, status));
  RETURNIFFAILURE(send_server_header(socket));
  RETURNIFFAILURE(send_connection_header(socket));
  RETURNIFFAILURE(send_content_length_header(socket, content_length));
  RETURNIFFAILURE(send_content_type_header(socket, mime_type));
  RETURNIFFAILURE(send_extra_crlf(socket));
  return 0;
}

int send_http_status(int socket, http_status_t status) {
  char *str;
  int status_code;
  char *status_message;
  switch(status) {
  case HTTP_STATUS_200:
    status_code    = 200;
    status_message = "OK";
    break;
  case HTTP_STATUS_400:
    status_code    = 400;
    status_message = "Bad Request";
    break;
  case HTTP_STATUS_404:
    status_code    = 404;
    status_message = "Not Found";
    break;
  case HTTP_STATUS_500:
    status_code    = 500;
    status_message = "Internal Server Error";
    break;
  default: return -1;
  }

  // Should we send HTTP/1.0 or HTTP/1.1?
  int count = asprintf(&str, "HTTP/1.1 %i %s\r\n", status_code, status_message);
  if(count == -1) {
    logerror("send_http_status:asprintf");
    return -1;
  }

  int rc;
  while((rc = send(socket, str, count, MSG_DONTWAIT)) == -1 && errno == EAGAIN);
  free(str);

  if(rc == -1) {
    logerror("send_http_status:send");
    return -1;
  }

  return 0;
}

int send_server_header(int socket) {
  char *server_header = "Server: " PACKAGE_STRING "\r\n";
  size_t server_header_len = strlen(server_header);
  int rc;
  while((rc = send(socket, server_header, server_header_len, MSG_DONTWAIT)) == -1
        && errno == EAGAIN);
  if(rc == -1) {
    logerror("send_server_header:send");
    return -1;
  }
  return 0;
}

int send_connection_header(int socket) {
  char *connection_header = "Connection: close\r\n";
  size_t connection_header_len = strlen(connection_header);
  int rc;
  while((rc = send(socket, connection_header, connection_header_len, MSG_DONTWAIT)) == -1
        && errno == EAGAIN);
  if(rc == -1) {
    logerror("send_connection_header:send");
    return -1;
  }
  return 0;
}

int send_content_length_header(int socket, int content_length) {
  char *content_length_header;
  int count = asprintf(&content_length_header, "Content-Length: %u\r\n", content_length);
  if(count == -1) {
    logerror("send_content_length:asprintf");
    return -1;
  }

  int rc;
  while((rc = send(socket, content_length_header, count, MSG_DONTWAIT)) == -1
        && errno == EAGAIN);
  if(rc == -1) {
    logerror("send_content_length_header:send");
    return -1;
  }
  free(content_length_header);

  return 0;
}

int send_content_type_header(int socket, mime_type_t mime_type) {
  char *formatted_mime_type = NULL;
  switch(mime_type) {
  case MIME_TYPE_UNKNOWN: break;
  case MIME_TYPE_HTML:
    formatted_mime_type = "text/html";
    break;
  case MIME_TYPE_CSS:
    formatted_mime_type = "text/css";
    break;
  case MIME_TYPE_JAVASCRIPT:
    formatted_mime_type = "text/javascript";
    break;
  }
  if(formatted_mime_type == NULL) {
    return 0; // This is NOT an error, it just won't send the header
  }

  char *content_type_header;
  int count = asprintf(&content_type_header, "Content-Type: %s\r\n",
                       formatted_mime_type);
  if(count == 1) {
    logerror("send_content_type_header:asprintf");
    return -1;
  }

  int rc;
  while((rc = send(socket, content_type_header, count, MSG_DONTWAIT)) == -1
        && errno == EAGAIN);
  if(rc == -1) {
    logerror("send_content_length:send");
    return -1;
  }

  return 0;
}

int send_extra_crlf(int socket) {
  int rc;
  while((rc = send(socket, "\r\n", 2, MSG_DONTWAIT)) == -1 && errno == EAGAIN);
  if(rc == -1) {
    logerror("send_extra_crlf:send");
    return -1;
  }
  return 0;
}

mime_type_t get_mime_type(const char *path) {
  size_t len = strlen(path);
  if(len >= 5 && strcmp(path + len - 5, ".html") == 0) { return MIME_TYPE_HTML; }
  if(len >= 4 && strcmp(path + len - 4, ".css")  == 0) { return MIME_TYPE_CSS; }
  if(len >= 3 && strcmp(path + len - 3, ".js")   == 0) { return MIME_TYPE_JAVASCRIPT; }
  return MIME_TYPE_UNKNOWN;
}
