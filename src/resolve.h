#ifndef RESOLVE_H
#define RESOLVE_H 1

#include <request.h>
#include <primp_config.h>

typedef enum resolution_strategy {
  RESOLUTION_STRATEGY_400   = 1,
  RESOLUTION_STRATEGY_404   = 2,
  RESOLUTION_STRATEGY_500   = 3,
  RESOLUTION_STRATEGY_502   = 4,
  RESOLUTION_STRATEGY_504   = 5,
  RESOLUTION_STRATEGY_CLOSE = 6
} resolution_strategy_t;

resolution_strategy_t resolve_request(request_t *, config_t *);
void execute_fallback_strategy(int, resolution_strategy_t);

resolution_strategy_t resolve_with_static_path(request_t *, char *);
resolution_strategy_t resolve_with_server(request_t *, server_t *);

#endif
