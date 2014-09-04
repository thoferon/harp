#include <stdio.h>
#include <stdlib.h>

#include <config_parser.h>
#include <config_lexer.h>

#include <memory.h>
#include <config.h>

#include <harp.h>

harp_list_t *harp_read_configs(char *path) {
  harp_list_t *configs;

  FILE *f = fopen(path, "r");
  if(f == NULL) {
    harp_errno = errno;
    return NULL;
  }

  void *scanner;
  yylex_init(&scanner);
  yyset_in(f, scanner);

  int rc = yyparse(scanner, &configs);

  yylex_destroy(scanner);
  fclose(f);

  // FIXME: in case of failure, what about memory?
  return rc == 0 ? configs : NULL;
}

/*
 * Functions to create the structures
 */

harp_filter_t *harp_make_hostnames_filter(harp_list_t *hostnames) {
  harp_filter_t *filter = (harp_filter_t*)smalloc(sizeof(struct harp_filter));
  filter->type      = HARP_FILTER_TYPE_HOSTNAMES;
  filter->hostnames = hostnames;
  return filter;
}

harp_filter_t *harp_make_ports_filter(harp_list_t *ports) {
  harp_filter_t *filter = (harp_filter_t*)smalloc(sizeof(struct harp_filter));
  filter->type  = HARP_FILTER_TYPE_PORTS;
  filter->ports = ports;
  return filter;
}

harp_resolver_t *harp_make_static_path_resolver(char *static_path) {
  harp_resolver_t *resolver =
    (harp_resolver_t*)smalloc(sizeof(struct harp_resolver));
  resolver->type        = HARP_RESOLVER_TYPE_STATIC_PATH;
  resolver->static_path = static_path;
  return resolver;
}

harp_resolver_t *harp_make_server_resolver(char *hostname, int port) {
  harp_resolver_t *resolver =
    (harp_resolver_t*)smalloc(sizeof(struct harp_resolver));
  harp_server_t *server = (harp_server_t*)smalloc(sizeof(struct harp_server));

  server->hostname = hostname;
  server->port     = port;

  resolver->type   = HARP_RESOLVER_TYPE_SERVER;
  resolver->server = server;

  return resolver;
}

harp_config_t *harp_make_empty_config() {
  return (harp_config_t*)scalloc(1, sizeof(struct harp_config));
}

void harp_cons_filter(harp_filter_t *filter, harp_config_t *config) {
  config->filters = harp_cons(filter, config->filters);
}

void harp_cons_tag(char *tag, harp_config_t *config) {
  config->tags = harp_cons(tag, config->tags);
}

void harp_cons_resolver(harp_resolver_t *resolver, harp_config_t *config) {
  config->resolvers = harp_cons(resolver, config->resolvers);
}

void harp_cons_choice_group(harp_list_t *choice_group, harp_config_t *config) {
  config->choice_groups = harp_cons(choice_group, config->choice_groups);
}

harp_choice_t *harp_make_choice(harp_prob_t prob, harp_config_t *config) {
  harp_choice_t *choice = (harp_choice_t*)smalloc(sizeof(struct harp_choice));

  choice->prob   = prob;
  choice->config = config;

  return choice;
}

/*
 * Duplicate functions
 */

int *duplicate_port(int *port) {
  int *new_port = (int*)smalloc(sizeof(int));
  *new_port = *port;
  return new_port;
}

harp_filter_t *harp_duplicate_filter(harp_filter_t *filter) {
  harp_list_t *new_hostnames;
  harp_list_t *new_ports;

  switch(filter->type) {
  case HARP_FILTER_TYPE_HOSTNAMES:
    new_hostnames = harp_duplicate(filter->hostnames,
                                   (harp_duplicate_function_t*)&strdup);
    return harp_make_hostnames_filter(new_hostnames);

  case HARP_FILTER_TYPE_PORTS:
    new_ports = harp_duplicate(filter->ports,
                               (harp_duplicate_function_t*)&duplicate_port);
    return harp_make_ports_filter(new_ports);

  default: return NULL;
  }
}

harp_resolver_t *harp_duplicate_resolver(harp_resolver_t *resolver) {
  switch(resolver->type) {
  case HARP_RESOLVER_TYPE_STATIC_PATH:
    return harp_make_static_path_resolver(strdup(resolver->static_path));

  case HARP_RESOLVER_TYPE_SERVER:
    return harp_make_server_resolver(strdup(resolver->server->hostname),
                                     resolver->server->port);

  default: return NULL;
  }
}

/*
 * Functions to free memory
 */

void harp_free_filter(harp_filter_t *filter) {
  switch(filter->type) {
  case HARP_FILTER_TYPE_HOSTNAMES:
    harp_free_list(filter->hostnames, &free);
    break;
  case HARP_FILTER_TYPE_PORTS:
    harp_free_list(filter->ports, &free);
    break;
  }

  free(filter);
}

inline void harp_free_server(harp_server_t *server) {
  free(server->hostname);
  free(server);
}

void harp_free_resolver(harp_resolver_t *resolver) {
  switch(resolver->type) {
  case HARP_RESOLVER_TYPE_STATIC_PATH:
    free(resolver->static_path);
    break;
  case HARP_RESOLVER_TYPE_SERVER:
    harp_free_server(resolver->server);
    break;
  }

  free(resolver);
}

void harp_free_choice(harp_choice_t *choice) {
  harp_free_config(choice->config);
  free(choice);
}

void harp_free_choice_group(harp_list_t *choice_group) {
  harp_free_list(choice_group,
                 (harp_free_function_t*)&harp_free_choice);
}

void harp_free_config(harp_config_t *config) {
  harp_free_list(config->filters,
                 (harp_free_function_t*)&harp_free_filter);
  harp_free_list(config->tags,
                 (harp_free_function_t*)&free);
  harp_free_list(config->resolvers,
                 (harp_free_function_t*)&harp_free_resolver);
  harp_free_list(config->choice_groups,
                 (harp_free_function_t*)&harp_free_choice_group);
  free(config);
}

/*
 * Getter functions
 */

harp_list_t *get_ports_from_choice_group(harp_list_t *);
harp_list_t *get_ports_from_config(harp_config_t *);
harp_list_t *append_port(harp_list_t *, int *);

harp_list_t *harp_get_ports(harp_list_t *configs) {
  harp_list_t *ports = HARP_EMPTY_LIST;
  harp_list_t *current;

  HARP_LIST_FOR_EACH(current, configs) {
    harp_list_t *new_ports = get_ports_from_config((harp_config_t*)current->element);
    harp_list_t *current2;
    HARP_LIST_FOR_EACH(current2, new_ports) {
      ports = append_port(ports, current2->element);
    }
    harp_free_list(new_ports, NULL);
  }

  return ports;
}

// might have duplicates
harp_list_t *get_ports_from_choice_group(harp_list_t *choices) {
  harp_list_t *ports = HARP_EMPTY_LIST;

  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, choices) {
    harp_choice_t *choice = (harp_choice_t*)current->element;
    ports = harp_concat(ports, get_ports_from_config(choice->config));
  }

  return ports;
}

// might have duplicates
harp_list_t *get_ports_from_config(harp_config_t *config) {
  harp_list_t *ports = HARP_EMPTY_LIST;
  harp_list_t *current;

  HARP_LIST_FOR_EACH(current, config->filters) {
    harp_filter_t *filter = (harp_filter_t*)current->element;
    if(filter->type == HARP_FILTER_TYPE_PORTS) {
      ports = harp_concat(ports, harp_duplicate(filter->ports, NULL));
    }
  }

  HARP_LIST_FOR_EACH(current, config->choice_groups) {
    harp_list_t *new_ports =
      get_ports_from_choice_group((harp_list_t*)current->element);
    ports = harp_concat(ports, new_ports);
  }

  return ports;
}

// The list can be freed but NOT the ports.
harp_list_t *append_port(harp_list_t *ports, int *port) {
  harp_list_t *new_ports = ports;
  bool already_in        = false;
  harp_list_t *current;

  HARP_LIST_FOR_EACH(current, ports) {
    if(*((int*)current->element) == *port) {
      already_in = true;
    }
  }

  if(!already_in) { new_ports = harp_append(new_ports, port); }

  return new_ports;
}
