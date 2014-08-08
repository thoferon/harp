#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include <options.h>
#include <list.h>
#include <primp_config.h>
#include <primp_config/find.h>
#include <connection_pool.h>
#include <request.h>
#include <resolve.h>

int main(int argc, char **argv) {
  options_t *options = read_options(argc, argv);
  list_t *configs = read_configs(options->config_path);

  list_t *ports = get_ports(configs);
  list_t *connection_pool = create_connection_pool(ports);

  if(connection_pool == NULL) {
    printf("Connections couldn't be created\n");
  } else {
    printf("Listening...\n");
    nfds_t n;
    struct pollfd *pollfds;
    create_pollfds(connection_pool, &pollfds, &n);
    while(true) {
      aconnection_t *aconnection = get_next_connection(connection_pool, pollfds, n);
      request_t *request = create_request(aconnection);
      if(request != NULL) {
        printf("Received connection on hostname = %s with path = %s\n", request->info->hostname, request->info->path);
        config_t *config = find_config(configs, request->info, aconnection->addr_hash);
        printf("Found a config at 0x%x\n", config);
        resolution_strategy_t strategy = resolve_request(request, config);
        printf("fallback strategy = %i\n", strategy);
        execute_fallback_strategy(request->aconnection->socket, strategy);
        free_config(config);
        destroy_request(request); // Destroys the aconnection as well
      } else {
        printf("Can't parse request, ignoring it.\n");
        destroy_aconnection(aconnection);
      }
    }
  }

  destroy_connection_pool(connection_pool);

  free_list(ports, NULL);
  free_list(configs, (free_function_t*)&free_config);
  free_options(options);

  return EXIT_SUCCESS;
}
