#ifndef RESPONSE_H
#define RESPONSE_H 1

typedef enum http_status {
  HTTP_STATUS_200,
  HTTP_STATUS_400,
  HTTP_STATUS_404,
  HTTP_STATUS_500,
  HTTP_STATUS_502,
  HTTP_STATUS_504
} http_status_t;

typedef enum mime_type {
  MIME_TYPE_UNKNOWN,
  MIME_TYPE_HTML,
  MIME_TYPE_CSS,
  MIME_TYPE_JAVASCRIPT
} mime_type_t;

int send_response_headers(int, http_status_t, int, mime_type_t);

mime_type_t get_mime_type(const char *);

#endif
