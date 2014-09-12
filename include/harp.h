#ifndef HARP_H
#define HARP_H 1

#include <stdbool.h>

/*
 * Errors
 */

#define HARP_ERROR_INVALID_FILTER   -1
#define HARP_ERROR_INVALID_RESOLVER -2
#define HARP_ERROR_PARSE_ERROR      -3
#define HARP_ERROR_NO_CONFIG        -4

extern int harp_errno;
extern char harp_errstr[1024]; /* Should not be accessed directly */
char *harp_strerror(int);

/*
 * Lists
 */

#define HARP_EMPTY_LIST NULL

typedef void harp_free_function_t(void *);
typedef void *harp_duplicate_function_t(void *);
typedef bool harp_predicate_function_t(void *);

typedef struct harp_list {
  void *element;
  struct harp_list *next;
} harp_list_t;

harp_list_t *harp_singleton(void *);
harp_list_t *harp_cons(void *, harp_list_t *);
harp_list_t *harp_concat(harp_list_t *, harp_list_t *);
harp_list_t *harp_last(harp_list_t *);
harp_list_t *harp_append(harp_list_t *, void *);
harp_list_t *harp_duplicate(harp_list_t *, harp_duplicate_function_t *);
int harp_length(harp_list_t *);

void harp_free_list(harp_list_t *, harp_free_function_t *);

harp_list_t *harp_find_element(harp_list_t *, harp_predicate_function_t *);

#define HARP_LIST_FOR_EACH(varname, list) \
  for(varname = (list); varname != HARP_EMPTY_LIST; varname = varname->next)

/*
 * Configuration
 */

typedef enum harp_filter_type {
  HARP_FILTER_TYPE_HOSTNAMES = 0,
  HARP_FILTER_TYPE_PORTS = 1
} harp_filter_type_t;

typedef struct harp_filter {
  harp_filter_type_t type;
  union {
    harp_list_t *hostnames;
    harp_list_t *ports;
  };
} harp_filter_t;

typedef struct harp_server {
  char *hostname;
  int port;
} harp_server_t;

typedef enum harp_resolver_type {
  HARP_RESOLVER_TYPE_STATIC_PATH = 0,
  HARP_RESOLVER_TYPE_SERVER = 1
} harp_resolver_type_t;

typedef struct harp_resolver {
  harp_resolver_type_t type;
  union {
    char *static_path;
    harp_server_t *server;
  };
} harp_resolver_t;

typedef unsigned short harp_prob_t;

typedef struct harp_choice harp_choice_t;
typedef struct harp_config harp_config_t;

struct harp_choice {
  harp_prob_t prob;
  harp_config_t *config;
};

struct harp_config {
  harp_list_t *filters;
  harp_list_t *tags;
  harp_list_t *resolvers;
  harp_list_t *choice_groups;
  harp_list_t *subconfigs;
};

harp_list_t *harp_read_configs(char *, char *);
int harp_write_configs(harp_list_t *, char *);

harp_filter_t *harp_make_hostnames_filter(harp_list_t *);
harp_filter_t *harp_make_ports_filter(harp_list_t *);
harp_resolver_t *harp_make_static_path_resolver(char *);
harp_resolver_t *harp_make_server_resolver(char *, int);

harp_config_t *harp_make_empty_config();
void harp_cons_filter(harp_filter_t *, harp_config_t *);
void harp_cons_tag(char *, harp_config_t *);
void harp_cons_resolver(harp_resolver_t *, harp_config_t *);
void harp_cons_choice_group(harp_list_t *, harp_config_t *);
void harp_cons_subconfig(harp_config_t *, harp_config_t *);

harp_choice_t *harp_make_choice(harp_prob_t, harp_config_t *);

harp_filter_t *harp_duplicate_filter(harp_filter_t *);
harp_resolver_t *harp_duplicate_resolver(harp_resolver_t *);
harp_choice_t *harp_duplicate_choice(harp_choice_t *);
harp_list_t *harp_duplicate_choice_group(harp_list_t *);
harp_config_t *harp_duplicate_config(harp_config_t *);

harp_config_t *harp_merge_configs(harp_config_t *, harp_config_t *);

void harp_free_filter(harp_filter_t *);
void harp_free_server(harp_server_t *);
void harp_free_resolver(harp_resolver_t *);
void harp_free_choice(harp_choice_t *);
void harp_free_choice_group(harp_list_t *);
void harp_free_config(harp_config_t *);

harp_list_t *harp_get_ports(harp_list_t *);

#endif
