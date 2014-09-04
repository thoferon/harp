#include <stdlib.h>
#include <string.h>

#include <harp.h>
#include <request.h>

#include <harp_config/find.h>

// FIXME: Use autotools to have uint32_t for the hash and UINT_MAX
#define MAX_HASH 4294967295

// The config is coming from choose_config and therefore
// does not have choice groups
bool matches_request_info(harp_config_t *config,
                          request_info_t *request_info) {
  harp_list_t *current, *current2;

  HARP_LIST_FOR_EACH(current, config->filters) {
    harp_filter_t *filter    = current->element;
    bool matches_filter = false;

    switch(filter->type) {
    case HARP_FILTER_TYPE_HOSTNAMES:
      HARP_LIST_FOR_EACH(current2, filter->hostnames) {
        char *hostname = current2->element;
        if(strcmp(request_info->hostname, hostname) == 0) {
          matches_filter = true;
          break;
        }
      }
      break;

    case HARP_FILTER_TYPE_PORTS:
      HARP_LIST_FOR_EACH(current2, filter->ports) {
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

harp_config_t *find_config(harp_list_t *configs, request_info_t *request_info,
                            unsigned int hash) {
  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, configs) {
    harp_config_t *config        = current->element;
    harp_config_t *chosen_config = choose_config(config, hash);
    if(matches_request_info(chosen_config, request_info)) {
      return chosen_config;
    } else {
      harp_free_config(chosen_config);
    }
  }
  return NULL;
}

int total_probs_in_choice_group(harp_list_t *choice_group) {
  int total = 0;
  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, choice_group) {
    harp_choice_t *choice = current->element;
    total += choice->prob;
  }
  return total;
}

harp_config_t *choose_config_from_choice_group(harp_list_t *choice_group,
                                               unsigned int hash) {
  unsigned int total = total_probs_in_choice_group(choice_group);
  double unit        = MAX_HASH / total;

  harp_config_t *config      = NULL;
  harp_config_t *last_config = NULL;
  unsigned int width         = 0;

  int acc = 0;
  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, choice_group) {
    harp_choice_t *choice = current->element;
    int offset            = unit * acc;
    width                 = unit * choice->prob;
    last_config           = choice->config;
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
harp_config_t *choose_config(harp_config_t *config, unsigned int hash) {
  harp_config_t *result_config = harp_make_empty_config();

  result_config->filters =
    harp_duplicate(config->filters,
                   (harp_duplicate_function_t*)&harp_duplicate_filter);
  result_config->tags = harp_duplicate(config->tags,
                                       (harp_duplicate_function_t*)&strdup);
  result_config->resolvers =
    harp_duplicate(config->resolvers,
                   (harp_duplicate_function_t*)&harp_duplicate_resolver);

  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, config->choice_groups) {
    harp_list_t *choice_group = (harp_list_t*)current->element;
    harp_config_t *subconfig  = choose_config_from_choice_group(choice_group,
                                                                hash);

    result_config->filters   = harp_concat(result_config->filters,
                                           subconfig->filters);
    result_config->tags      = harp_concat(result_config->tags,
                                           subconfig->tags);
    result_config->resolvers = harp_concat(result_config->resolvers,
                                           subconfig->resolvers);

    // There are no choice groups and the other elements are used in `config`
    free(subconfig);
  }

  return result_config;
}
