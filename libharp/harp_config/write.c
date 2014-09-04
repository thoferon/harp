#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include <harp.h>

#define INDENT_SIZE 2

#define RETURNERROR() do {                      \
    harp_errno = errno;                        \
    return -1;                                  \
  } while(0)

#define INDENT() do {                           \
    rc = fprintf(f, "%s", indent);              \
    if(rc == -1) {                              \
      RETURNERROR();                            \
    }                                           \
  } while(0)

inline char *create_new_indent(char *indent) {
  size_t indent_len = strlen(indent);
  char *new_indent = (char*)malloc(indent_len + INDENT_SIZE + 1);
  memset(new_indent, ' ', indent_len + INDENT_SIZE);
  new_indent[indent_len + INDENT_SIZE] = '\0';
  return new_indent;
}

int harp_write_configs_f(harp_list_t *, FILE *, char *);
int harp_write_config(harp_config_t *, FILE *, char *, bool);
int harp_write_filter(harp_filter_t *, FILE *, char *);
int harp_write_resolver(harp_resolver_t *, FILE *, char *);
int harp_write_tag(char *, FILE *, char *);
int harp_write_choice_group(harp_list_t *, FILE *, char *);
int harp_write_choice(harp_choice_t *, FILE *, char *);
int harp_write_string(char *, FILE *);
int harp_write_number(int, FILE *);

int harp_write_configs(harp_list_t *configs, char *path) {
  int rc;

  FILE *f = fopen(path, "w");
  if(f == NULL) {
    RETURNERROR();
  }

  rc = harp_write_configs_f(configs, f, "");
  if(rc == -1) {
    return -1;
  }

  rc = fclose(f);
  if(rc != 0) {
    RETURNERROR();
  }

  return 0;
}

int harp_write_configs_f(harp_list_t *configs, FILE *f, char *indent) {
  int rc;

  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, configs) {
    harp_config_t *config = (harp_config_t*)current->element;

    rc = harp_write_config(config, f, indent, true);
    if(rc == -1) {
      return -1;
    }

    rc = fprintf(f, "\n");
    if(rc == -1) {
      RETURNERROR();
    }
  }

  return 0;
}

int harp_write_config(harp_config_t *config, FILE *f,
                       char *indent, bool indent_flag) {
  int rc;

  if(indent_flag) {
    INDENT();
  }

  rc = fprintf(f, "{\n");
  if(rc == -1) {
    RETURNERROR();
  }

  char *new_indent = create_new_indent(indent);

  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, config->filters) {
    harp_filter_t *filter = (harp_filter_t*)current->element;
    rc = harp_write_filter(filter, f, new_indent);
    if(rc == -1) {
      free(new_indent);
      return -1;
    }
  }
  HARP_LIST_FOR_EACH(current, config->tags) {
    char *tag = (char*)current->element;
    rc = harp_write_tag(tag, f, new_indent);
    if(rc == -1) {
      free(new_indent);
      return -1;
    }
  }
  HARP_LIST_FOR_EACH(current, config->resolvers) {
    harp_resolver_t *resolver = (harp_resolver_t*)current->element;
    rc = harp_write_resolver(resolver, f, new_indent);
    if(rc == -1) {
      free(new_indent);
      return -1;
    }
  }
  HARP_LIST_FOR_EACH(current, config->choice_groups) {
    harp_list_t *choice_group = (harp_list_t*)current->element;
    rc = harp_write_choice_group(choice_group, f, new_indent);
    if(rc == -1) {
      free(new_indent);
      return -1;
    }
  }

  free(new_indent);

  INDENT();
  rc = fprintf(f, "}\n");
  if(rc != 2) {
    RETURNERROR();
  }

  return 0;
}

int harp_write_filter(harp_filter_t *filter, FILE *f, char *indent) {
  int rc;
  harp_list_t *current;

  INDENT();

  switch(filter->type) {
  case HARP_FILTER_TYPE_HOSTNAMES:
    fprintf(f, "hostnames");
    if(rc == -1) {
      RETURNERROR();
    }

    HARP_LIST_FOR_EACH(current, filter->hostnames) {
      char *hostname = (char*)current->element;
      rc = fprintf(f, " ");
      if(rc == -1) {
        RETURNERROR();
      }
      rc = harp_write_string(hostname, f);
      if(rc == -1) {
        return -1;
      }
    }

    break;

  case HARP_FILTER_TYPE_PORTS:
    fprintf(f, "ports");
    if(rc == -1) {
      RETURNERROR();
    }

    HARP_LIST_FOR_EACH(current, filter->ports) {
      int port = *((int*)current->element);
      rc = fprintf(f, " ");
      if(rc == -1) {
        RETURNERROR();
      }
      rc = harp_write_number(port, f);
      if(rc == -1) {
        return -1;
      }
    }

    break;

  default:
    harp_errno = HARP_ERROR_INVALID_FILTER;
    return -1;
  }

  rc = fprintf(f, ";\n");
  if(rc == -1) {
    RETURNERROR();
  }

  return 0;
}

int harp_write_resolver(harp_resolver_t *resolver, FILE *f, char *indent) {
  int rc;

  INDENT();

  switch(resolver->type) {
  case HARP_RESOLVER_TYPE_STATIC_PATH:
    rc = fprintf(f, "static-path ");
    if(rc == -1) {
      RETURNERROR();
    }

    rc = harp_write_string(resolver->static_path, f);
    if(rc == -1) {
      return -1;
    }

    break;

  case HARP_RESOLVER_TYPE_SERVER:
    rc = fprintf(f, "server ");
    if(rc == -1) {
      RETURNERROR();
    }

    rc = harp_write_string(resolver->server->hostname, f);
    if(rc == -1) {
      return -1;
    }

    rc = fprintf(f, " ");
    if(rc == -1) {
      RETURNERROR();
    }

    rc = harp_write_number(resolver->server->port, f);
    if(rc == -1) {
      return -1;
    }

    break;

  default:
    harp_errno = HARP_ERROR_INVALID_FILTER;
    return -1;
  }

  rc = fprintf(f, ";\n");
  if(rc == -1) {
    RETURNERROR();
  }

  return 0;
}

int harp_write_tag(char *tag, FILE *f, char *indent) {
  int rc;

  INDENT();

  rc = fprintf(f, "tag ");
  if(rc == -1) {
    RETURNERROR();
  }

  rc = harp_write_string(tag, f);
  if(rc == -1) {
    return -1;
  }

  rc = fprintf(f, ";\n");
  if(rc == -1) {
    RETURNERROR();
  }

  return 0;
}

int harp_write_choice_group(harp_list_t *choices, FILE *f, char *indent) {
  int rc;

  char *new_indent = create_new_indent(indent);

  rc = fprintf(f, "\n");
  if(rc == -1) {
    free(new_indent);
    RETURNERROR();
  }

  INDENT();

  rc = fprintf(f, "[\n");
  if(rc == -1) {
    free(new_indent);
    RETURNERROR();
  }

  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, choices) {
    harp_choice_t *choice = (harp_choice_t*)current->element;
    rc = harp_write_choice(choice, f, new_indent);
    if(rc == -1) {
      free(new_indent);
      return -1;
    }

    if(current->next != HARP_EMPTY_LIST) {
      rc = fprintf(f, "\n");
      if(rc == -1) {
        free(new_indent);
        RETURNERROR();
      }
    }
  }

  INDENT();

  rc = fprintf(f, "]\n");
  if(rc == -1) {
    free(new_indent);
    RETURNERROR();
  }

  return 0;
}

int harp_write_choice(harp_choice_t *choice, FILE *f, char *indent) {
  int rc;

  INDENT();
  rc = harp_write_number((int)choice->prob, f);
  if(rc == -1) {
    return -1;
  }

  rc = fprintf(f, " ");
  if(rc == -1) {
    RETURNERROR();
  }

  rc = harp_write_config(choice->config, f, indent, false);
  if(rc == -1) {
    return -1;
  }

  return 0;
}

int harp_write_string(char *str, FILE *f) {
  int rc;
  int start   = 0;
  int current = 0;

  rc = fprintf(f, "\"");
  if(rc == -1) {
    RETURNERROR();
  }

  while(1) {
    if(str[current] == '\0') {
      rc = fwrite(str + start, 1, current - start, f);
      if(rc != current - start) {
        RETURNERROR();
      }
      break;
    }

    if(str[current] == '"') {
      rc = fwrite(str + start, 1, current - start, f);
      if(rc != current - start) {
        RETURNERROR();
      }
      rc = fprintf(f, "\\\"");
      if(rc == -1) {
        RETURNERROR();
      }
      start = current + 1;
    }

    if(str[current] == '\\') {
      rc = fwrite(str + start, 1, current - start, f);
      if(rc != current - start) {
        RETURNERROR();
      }
      rc = fprintf(f, "\\\\");
      if(rc == -1) {
        RETURNERROR();
      }
      start = current + 1;
    }

    current++;
  }

  rc = fprintf(f, "\"");
  if(rc == -1) {
    RETURNERROR();
  }

  return 0;
}

int harp_write_number(int num, FILE *f) {
  int rc;

  rc = fprintf(f, "%i", num);
  if(rc == -1) {
    RETURNERROR();
  }

  return 0;
}
