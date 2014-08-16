#include <stdlib.h>
#include <string.h>

#include <primp_config.h>
#include <list.h>
#include <request.h>

#include <primp_config/find.h>

// FIXME: Use autotools to have uint32_t for the hash and UINT_MAX
#define MAX_HASH 4294967295

// The config is coming from choose_config and therefore does not have choice groups
bool matches_request_info(config_t *config, request_info_t *request_info) {
  list_t *current, *current2;

  LISTFOREACH(current, config->filters) {
    filter_t *filter    = current->element;
    bool matches_filter = false;

    switch(filter->type) {
    case FILTER_TYPE_HOSTNAMES:
      LISTFOREACH(current2, filter->hostnames) {
        char *hostname = current2->element;
        if(strcmp(request_info->hostname, hostname) == 0) {
          matches_filter = true;
          break;
        }
      }
      break;

    case FILTER_TYPE_PORTS:
      LISTFOREACH(current2, filter->ports) {
        int port = *((int*)current2->element);
        if(port == request_info->port) {
          matches_filter = true;
          break;
        }
      }
      break;
    }

    if(!matches_filter) { return false; }
  }
  return true;
}

config_t *find_config(list_t *configs, request_info_t *request_info,
                      unsigned int hash) {
  list_t *current;
  LISTFOREACH(current, configs) {
    config_t *config        = current->element;
    config_t *chosen_config = choose_config(config, hash);
    if(matches_request_info(chosen_config, request_info)) {
      return chosen_config;
    } else {
      free_config(chosen_config);
    }
  }
  return NULL;
}

int total_probs_in_choice_group(list_t *choice_group) {
  int total = 0;
  list_t *current;
  LISTFOREACH(current, choice_group) {
    choice_t *choice = current->element;
    total += choice->prob;
  }
  return total;
}

config_t *choose_config_from_choice_group(list_t *choice_group, unsigned int hash) {
  unsigned int total = total_probs_in_choice_group(choice_group);
  double unit        = MAX_HASH / total;

  config_t *config      = NULL;
  config_t *last_config = NULL;
  unsigned int width    = 0;

  int acc = 0;
  list_t *current;
  LISTFOREACH(current, choice_group) {
    choice_t *choice = current->element;
    int offset       = unit * acc;
    width            = unit * choice->prob;
    last_config      = choice->config;
    if(offset <= hash && hash < offset + width) {
      break;
    }
    acc += choice->prob;
  }
  config = last_config; // So it works with MAX_HASH as well

  unsigned long long int new_hash = hash;
  new_hash -= acc * unit;
  new_hash *= MAX_HASH;
  new_hash /= width;

  return choose_config(config, new_hash);
}

// Create a brand new config. It should be possible to free the original config.
config_t *choose_config(config_t *config, unsigned int hash) {
  config_t *result_config = make_empty_config();

  result_config->filters = duplicate(config->filters,
                                     (duplicate_function_t*)&duplicate_filter);
  result_config->resolvers = duplicate(config->resolvers,
                                       (duplicate_function_t*)&duplicate_resolver);

  list_t *current;
  LISTFOREACH(current, config->choice_groups) {
    list_t *choice_group = (list_t*)current->element;
    config_t *subconfig  = choose_config_from_choice_group(choice_group, hash);

    result_config->filters   = concat(result_config->filters,   subconfig->filters);
    result_config->resolvers = concat(result_config->resolvers, subconfig->resolvers);

    // There are no choice groups and filters & resolvers are used in `config`
    free(subconfig);
  }

  return result_config;
}
