#ifndef REQUEST_H
#define REQUEST_H 1

#include <stdlib.h>

#include <connection_pool.h>

typedef struct request_info {
  char *path;
  char *hostname;
  int port;
} request_info_t;

typedef struct request {
  request_info_t *info;
  aconnection_t *aconnection;
  char *buffer;
  size_t buffer_size;
} request_t;

request_t *create_request(aconnection_t *aconnection);
void destroy_request(request_t *);

request_info_t *make_request_info(char *, char *, int);
request_t *make_request(request_info_t *, aconnection_t *, char *, size_t);

void free_request_info(request_info_t *);
void free_request(request_t *);

#endif
