#include <stdio.h>
#include <stdlib.h>

#include <config_parser.h>
#include <config_lexer.h> // FIXME: Make config_lexer.h include config_parser.h

#include <memory.h>
#include <log.h>
#include <config.h>
#include <list.h>
#include <primp_config.h>

list_t *read_configs(char *path) {
  list_t *configs;

  void *scanner;
  yylex_init(&scanner);

  FILE *f = fopen(path, "r");
  if(f == NULL) {
    char *fname      = "read_configs";
    size_t fname_len = strlen(fname);
    size_t path_len  = strlen(path);
    char *prefix     = (char*)smalloc(fname_len + path_len + 3); // brackets and '\0'
    char *end        = prefix;
    memcpy(prefix, fname, fname_len);
    end += fname_len;
    *(end++) = '(';
    memcpy(end, path, path_len);
    end += path_len;
    *end = ')';
    logerror(prefix);
    return NULL;
  }
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

filter_t *make_hostnames_filter(list_t *hostnames) {
  filter_t *filter  = (filter_t*)smalloc(sizeof(struct filter));
  filter->type      = FILTER_TYPE_HOSTNAMES;
  filter->hostnames = hostnames;
  return filter;
}

filter_t *make_ports_filter(list_t *ports) {
  filter_t *filter = (filter_t*)smalloc(sizeof(struct filter));
  filter->type     = FILTER_TYPE_PORTS;
  filter->ports    = ports;
  return filter;
}

resolver_t *make_static_path_resolver(char *static_path) {
  resolver_t *resolver  = (resolver_t*)smalloc(sizeof(struct resolver));
  resolver->type        = RESOLVER_TYPE_STATIC_PATH;
  resolver->static_path = static_path;
  return resolver;
}

resolver_t *make_server_resolver(char *hostname, int port) {
  resolver_t *resolver = (resolver_t*)smalloc(sizeof(struct resolver));
  server_t   *server   = (server_t*)  smalloc(sizeof(struct server));

  server->hostname = hostname;
  server->port     = port;

  resolver->type   = RESOLVER_TYPE_SERVER;
  resolver->server = server;

  return resolver;
}

config_t *make_empty_config() {
  return (config_t*)scalloc(1, sizeof(struct config));
}

void cons_filter(filter_t *filter, config_t *config) {
  config->filters = cons(filter, config->filters);
}

void cons_resolver(resolver_t *resolver, config_t *config) {
  config->resolvers = cons(resolver, config->resolvers);
}

void cons_choice_group(list_t *choice_group, config_t *config) {
  config->choice_groups = cons(choice_group, config->choice_groups);
}

choice_t *make_choice(prob_t prob, config_t *config) {
  choice_t *choice = (choice_t*)smalloc(sizeof(choice));

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

filter_t *duplicate_filter(filter_t *filter) {
  list_t *new_hostnames;
  list_t *new_ports;

  switch(filter->type) {
  case FILTER_TYPE_HOSTNAMES:
    new_hostnames = duplicate(filter->hostnames, (duplicate_function_t*)&strdup);
    return make_hostnames_filter(new_hostnames);

  case FILTER_TYPE_PORTS:
    new_ports = duplicate(filter->ports, (duplicate_function_t*)&duplicate_port);
    return make_ports_filter(new_ports);

  default: return NULL;
  }
}

resolver_t *duplicate_resolver(resolver_t *resolver) {
  switch(resolver->type) {
  case RESOLVER_TYPE_STATIC_PATH:
    return make_static_path_resolver(strdup(resolver->static_path));

  case RESOLVER_TYPE_SERVER:
    return make_server_resolver(strdup(resolver->server->hostname),
                                resolver->server->port);

  default: return NULL;
  }
}

/*
 * Functions to free memory
 */

void free_filter(filter_t *filter) {
  switch(filter->type) {
  case FILTER_TYPE_HOSTNAMES:
    free_list(filter->hostnames, &free);
    break;
  case FILTER_TYPE_PORTS:
    free_list(filter->ports, &free);
    break;
  }

  free(filter);
}

inline void free_server(server_t *server) {
  free(server->hostname);
  free(server);
}

void free_resolver(resolver_t *resolver) {
  switch(resolver->type) {
  case RESOLVER_TYPE_STATIC_PATH:
    free(resolver->static_path);
    break;
  case RESOLVER_TYPE_SERVER:
    free_server(resolver->server);
    break;
  }

  free(resolver);
}

void free_choice(choice_t *choice) {
  free_config(choice->config);
  free(choice);
}

void free_choice_group(list_t *choice_group) {
  free_list(choice_group, (free_function_t*)&free_choice);
}

void free_config(config_t *config) {
  free_list(config->filters,       (free_function_t*)&free_filter);
  free_list(config->resolvers,     (free_function_t*)&free_resolver);
  free_list(config->choice_groups, (free_function_t*)&free_choice_group);
  free(config);
}

/*
 * Getter functions
 */

list_t *get_ports_from_choice_group(list_t *);
list_t *get_ports_from_config(config_t *);
list_t *append_port(list_t *, int *);

list_t *get_ports(list_t *configs) {
  list_t *ports = EMPTY_LIST;
  list_t *current;

  LISTFOREACH(current, configs) {
    list_t *new_ports = get_ports_from_config((config_t*)current->element);
    list_t *current2;
    LISTFOREACH(current2, new_ports) {
      ports = append_port(ports, current2->element);
    }
    free_list(new_ports, NULL);
  }

  return ports;
}

// might have duplicates
list_t *get_ports_from_choice_group(list_t *choices) {
  list_t *ports = EMPTY_LIST;

  list_t *current;
  LISTFOREACH(current, choices) {
    choice_t *choice = (choice_t*)current->element;
    ports = concat(ports, get_ports_from_config(choice->config));
  }

  return ports;
}

// might have duplicates
list_t *get_ports_from_config(config_t *config) {
  list_t *ports = EMPTY_LIST;
  list_t *current;

  LISTFOREACH(current, config->filters) {
    filter_t *filter = (filter_t*)current->element;
    if(filter->type == FILTER_TYPE_PORTS) {
      ports = concat(ports, duplicate(filter->ports, NULL));
    }
  }

  LISTFOREACH(current, config->choice_groups) {
    ports = concat(ports, get_ports_from_choice_group((list_t*)current->element));
  }

  return ports;
}

// The list can be freed but NOT the ports.
list_t *append_port(list_t *ports, int *port) {
  list_t *new_ports = ports;
  bool already_in = false;
  list_t *current;

  LISTFOREACH(current, ports) {
    if(*((int*)current->element) == *port) {
      already_in = true;
    }
  }

  if(!already_in) { new_ports = append(new_ports, port); }

  return new_ports;
}
