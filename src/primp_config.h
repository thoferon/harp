#ifndef PRIMP_CONFIG_H
#define PRIMP_CONFIG_H 1

#include <list.h>

typedef enum filter_type {
  FILTER_TYPE_HOSTNAMES = 0,
  FILTER_TYPE_PORTS = 1
} filter_type_t;

typedef struct filter {
  filter_type_t type;
  union {
    list_t *hostnames;
    list_t *ports;
  };
} filter_t;

typedef struct server {
  char *hostname;
  int port;
} server_t;

typedef enum resolver_type {
  RESOLVER_TYPE_STATIC_PATH = 0,
  RESOLVER_TYPE_SERVER = 1
} resolver_type_t;

typedef struct resolver {
  resolver_type_t type;
  union {
    char *static_path;
    server_t *server;
  };
} resolver_t;

typedef unsigned short prob_t;

typedef struct choice choice_t;
typedef struct config config_t;

struct choice {
  prob_t prob;
  config_t *config;
};

struct config {
  list_t *filters;
  list_t *resolvers;
  list_t *choice_groups;
};

list_t *read_configs(char *);

filter_t *make_hostnames_filter(list_t *);
filter_t *make_ports_filter(list_t *);
resolver_t *make_static_path_resolver(char *);
resolver_t *make_server_resolver(char *, int);

config_t *make_empty_config();
void cons_filter(filter_t *, config_t *);
void cons_resolver(resolver_t *, config_t *);
void cons_choice_group(list_t *, config_t *);

choice_t *make_choice(prob_t, config_t *);

filter_t *duplicate_filter(filter_t *);
resolver_t *duplicate_resolver(resolver_t *);

void free_filter(filter_t *);
void free_server(server_t *);
void free_resolver(resolver_t *);
void free_choice(choice_t *);
void free_choice_group(list_t *);
void free_config(config_t *);

list_t *get_ports(list_t *);

#endif
