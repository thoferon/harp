#ifndef PRIMP_CONFIG_FIND_H
#define PRIMP_CONFIG_FIND_H 1

#include <primp_config.h>
#include <list.h>
#include <request.h>

config_t *find_config(list_t *, request_info_t *, unsigned int);
config_t *choose_config(config_t *, unsigned int);

#endif
