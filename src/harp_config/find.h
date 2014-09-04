#ifndef HARP_CONFIG_FIND_H
#define HARP_CONFIG_FIND_H 1

#include <harp.h>
#include <request.h>

harp_config_t *find_config(harp_list_t *, request_info_t *, unsigned int);
harp_config_t *choose_config(harp_config_t *, unsigned int);

#endif
