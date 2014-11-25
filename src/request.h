#ifndef REQUEST_H
#define REQUEST_H 1

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <poll.h>

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
  time_t start;
  time_t last_transfer;
  size_t total_transferred;
} request_t;

request_t *create_request(aconnection_t *);
void destroy_request(request_t *);

request_info_t *make_request_info(char *, char *, int);
request_t *make_request(request_info_t *, aconnection_t *, char *,
                        size_t, time_t);

void free_request_info(request_info_t *);
void free_request(request_t *);

int update_request(request_t *, size_t);
bool is_still_valid(request_t *);

int poll_with_request(request_t *, struct pollfd *, nfds_t);
int send_with_request(request_t *, int, const void *, size_t);
ssize_t recv_with_request(request_t *, int, void *, size_t);

#endif
