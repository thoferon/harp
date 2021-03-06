#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H 1

#include <stdbool.h>
#include <poll.h>

#include <harp.h>

// Listening connection
typedef struct lconnection {
  int port;
  int socket;
} lconnection_t;

// Accepted connection
typedef struct aconnection {
  unsigned int addr_hash;
  int port;
  int socket;
} aconnection_t;

lconnection_t *make_lconnection(int, int);
aconnection_t *make_aconnection(unsigned int, int, int);

void free_lconnection(lconnection_t *);
void free_aconnection(aconnection_t *);

harp_list_t *create_connection_pool(harp_list_t *, harp_list_t *);
lconnection_t *create_lconnection(int);

void destroy_connection_pool(harp_list_t *, harp_list_t *);
void destroy_lconnection(lconnection_t *);
void destroy_aconnection(aconnection_t *);

void create_pollfds(harp_list_t *, struct pollfd **, nfds_t *);
aconnection_t *get_next_connection(harp_list_t *, struct pollfd *, nfds_t);

#endif
