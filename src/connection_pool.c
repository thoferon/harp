#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#include <memory.h>
#include <config.h>
#include <list.h>
#include <connection_pool.h>

// FIXME: This should be overwritable by a top-level directive in the config
#define MAXCONN 128

/*
 * Functions to create structures
 */

lconnection_t *make_lconnection(int port, int socket) {
  lconnection_t *lconnection = (lconnection_t*)smalloc(sizeof(struct lconnection));
  lconnection->port          = port;
  lconnection->socket        = socket;
  return lconnection;
}

aconnection_t *make_aconnection(unsigned int addr_hash, int port, int socket) {
  aconnection_t *aconnection = (aconnection_t*)smalloc(sizeof(struct aconnection));
  aconnection->addr_hash     = addr_hash;
  aconnection->port          = port;
  aconnection->socket        = socket;
  return aconnection;
}

/*
 * Functions to free memory
 */

inline void free_lconnection(lconnection_t *lconnection) {
  free(lconnection);
}

inline void free_aconnection(aconnection_t *aconnection) {
  free(aconnection);
}

/*
 * Networking functions
 */

list_t *create_connection_pool(list_t *ports) {
  list_t *lconnections = EMPTY_LIST;
  list_t *current;

  LISTFOREACH(current, ports) {
    int port = *((int*)current->element);
    lconnection_t *new_connection = create_lconnection(port);

    if(new_connection != NULL) {
      // Order does not matter
      lconnections = cons(new_connection, lconnections);
    } else {
      free_list(lconnections, (free_function_t*)&destroy_lconnection);
      return NULL;
    }
  }

  return lconnections;
}

lconnection_t *create_lconnection(int port) {
  int rc;
  int sock;

  struct addrinfo hints, *addrinfos, *current;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE | AI_NUMERICSERV;

  char *port_string = (char*)smalloc(6);
  snprintf(port_string, 6, "%i", port);

  // FIXME: The hostname should be overwritable in the config (as a top-level directive)
  while((rc = getaddrinfo(NULL, port_string, &hints, &addrinfos)) == EAI_AGAIN);
  if(rc != 0) {
    fprintf(stderr, "create_connection:getaddrinfo: %s\n", gai_strerror(rc));
    return NULL;
  }

#define RETURNERROR() do {                              \
    rc = close(sock);                                   \
    if(rc == -1) { perror("create_connection:close"); } \
    return NULL;                                        \
  } while(0)

  if(addrinfos != NULL) {
    rc = -1;
    for(current = addrinfos; current != NULL; current = current->ai_next) {
      sock = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
      if(sock == -1) {
        perror("create_connnection:socket");
        return NULL;
      }

      rc = bind(sock, current->ai_addr, current->ai_addrlen);
      if(rc == 0) { break; }
    }

    freeaddrinfo(addrinfos);
    if(rc == -1) {
      perror("create_connection:bind");
      RETURNERROR();
    }

  } else {
    fprintf(stderr, "create_connection:getaddrinfo: Empty result\n");
    return NULL;
  }

  if(listen(sock, MAXCONN) == -1) {
    perror("create_connection:listen");
    RETURNERROR();
  }

  int flags = fcntl(sock, F_GETFL, NULL);
  if(flags == -1) {
    perror("create_connection:fcntl with F_GETFL");
    RETURNERROR();
  }

  rc = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
  if(rc == -1) {
    perror("create_connection:fcntl with F_SETFL");
    RETURNERROR();
  }

  return make_lconnection(port, sock);
}

void destroy_connection_pool(list_t *connection_pool) {
  list_t *current;
  LISTFOREACH(current, connection_pool) {
    lconnection_t *lconnection = current->element;
    destroy_lconnection(lconnection);
  }
}

void destroy_lconnection(lconnection_t *lconnection) {
  if(close(lconnection->socket) == -1) { perror("destroy_lconnection"); }
  free_lconnection(lconnection);
}

void destroy_aconnection(aconnection_t *aconnection) {
  if(close(aconnection->socket) == -1) { perror("destroy_aconnection"); }
  free_aconnection(aconnection);
}

// FIXME: Should use uint32_t
unsigned int compute_hash(struct sockaddr_in *sin) {
  struct in_addr addr        = sin->sin_addr;
  unsigned long long int tmp = addr.s_addr;
  tmp                       *= 2654435761;
  tmp                      >>= 32;
  return tmp;
}

void create_pollfds(list_t *lconnections, struct pollfd **pollfds_ptr,
                    nfds_t *n_ptr) {
  nfds_t n = length(lconnections);
  struct pollfd *pollfds = (struct pollfd*)scalloc(n, sizeof(struct pollfd));

  int i;
  list_t *current;
  for(i = 0, current = lconnections; current != NULL;
      i++, current = current->next) {
    lconnection_t *lconnection = current->element;
    pollfds[i].fd     = lconnection->socket;
    pollfds[i].events = POLLIN;
  }

  *pollfds_ptr = pollfds;
  *n_ptr       = n;
}

aconnection_t *get_next_connection(list_t *lconnections, struct pollfd *pollfds,
                                   nfds_t n) {
  list_t *current;
  while(1) {
    // FIXME: Any way to make it faster?
    int rc = poll(pollfds, n, -1);
    if(rc == -1) {
      perror("get_next_connection:poll");
    }

    LISTFOREACH(current, lconnections) {
      lconnection_t *lconnection = current->element;
      struct sockaddr_storage addr;
      socklen_t addrlen = sizeof(struct sockaddr_storage);

      int sock = accept(lconnection->socket, (struct sockaddr*)&addr, &addrlen);
      if(sock != -1) {
        unsigned int addr_hash = compute_hash((struct sockaddr_in*)&addr);
        return make_aconnection(addr_hash, lconnection->port, sock);
      } else if(sock != EINTR || sock != EWOULDBLOCK) {
        // FIXME: log error
      }
    }
  }
}
