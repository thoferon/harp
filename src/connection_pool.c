#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

#include <memory.h>
#include <log.h>
#include <harp.h>
#include <connection_pool.h>

#define MAXCONN 128

/*
 * Functions to create structures
 */

lconnection_t *make_lconnection(int port, int socket) {
  lconnection_t *lconnection =
    (lconnection_t*)smalloc(sizeof(struct lconnection));
  lconnection->port   = port;
  lconnection->socket = socket;
  return lconnection;
}

aconnection_t *make_aconnection(unsigned int addr_hash, int port, int socket) {
  aconnection_t *aconnection =
    (aconnection_t*)smalloc(sizeof(struct aconnection));
  aconnection->addr_hash = addr_hash;
  aconnection->port      = port;
  aconnection->socket    = socket;
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

harp_list_t *create_connection_pool(harp_list_t *ports,
                                     harp_list_t *old_connection_pool) {
  harp_list_t *lconnections = HARP_EMPTY_LIST;
  harp_list_t *current;

  HARP_LIST_FOR_EACH(current, ports) {
    int port = *((int*)current->element);

    lconnection_t *new_connection = NULL;
    harp_list_t *current2;
    HARP_LIST_FOR_EACH(current2, old_connection_pool) {
      lconnection_t *old_connection = (lconnection_t*)current2->element;
      if(old_connection->port == port) {
        new_connection = old_connection;
        break;
      }
    }

    if(new_connection == NULL) {
      new_connection = create_lconnection(port);
    }

    if(new_connection != NULL) {
      // Order does not matter
      lconnections = harp_cons(new_connection, lconnections);
    } else {
      harp_free_list(lconnections,
                      (harp_free_function_t*)&destroy_lconnection);
      return NULL;
    }
  }

  return lconnections;
}

lconnection_t *create_lconnection(int port) {
  int rc, sock;

  struct addrinfo hints, *addrinfos, *current;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE | AI_NUMERICSERV;

  char port_string[6];
  snprintf(port_string, 6, "%i", port);

  // FIXME: The hostname should be overwritable in the options
  while((rc = getaddrinfo(NULL, port_string, &hints, &addrinfos)) == EAI_AGAIN);
  if(rc != 0) {
    logmsg(LOG_ERR, "create_connection:getaddrinfo: %s\n", gai_strerror(rc));
    return NULL;
  }

#define RETURNERROR() do {                                      \
    rc = close(sock);                                           \
    if(rc == -1) { logerror("create_connection:close"); }       \
    return NULL;                                                \
  } while(0)

  if(addrinfos != NULL) {
    rc = -1;
    for(current = addrinfos; current != NULL; current = current->ai_next) {
      sock = socket(current->ai_family, current->ai_socktype,
                    current->ai_protocol);
      if(sock == -1) {
        logerror("create_connnection:socket");
        freeaddrinfo(addrinfos);
        return NULL;
      }

      rc = bind(sock, current->ai_addr, current->ai_addrlen);
      if(rc == 0) { break; }
    }

    freeaddrinfo(addrinfos);
    if(rc == -1) {
      logerror("create_connection:bind");
      RETURNERROR();
    }

  } else {
    logmsg(LOG_ERR, "create_connection:getaddrinfo: Empty result\n");
    return NULL;
  }

  if(listen(sock, MAXCONN) == -1) {
    logerror("create_connection:listen");
    RETURNERROR();
  }

  int flags = fcntl(sock, F_GETFL, NULL);
  if(flags == -1) {
    logerror("create_connection:fcntl with F_GETFL");
    RETURNERROR();
  }

  rc = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
  if(rc == -1) {
    logerror("create_connection:fcntl with F_SETFL");
    RETURNERROR();
  }

  logmsg(LOG_INFO, "Bound to port %i\n", port);

  return make_lconnection(port, sock);
}

/**
 * Destroy the list of lconnection.
 * @param ports The list of ports for which NOT to destroy the lconnection.
 */
void destroy_connection_pool(harp_list_t *connection_pool, harp_list_t *ports) {
  harp_list_t *current;
  HARP_LIST_FOR_EACH(current, connection_pool) {
    lconnection_t *lconnection = current->element;

    bool to_keep = false;
    harp_list_t *current2;
    HARP_LIST_FOR_EACH(current2, ports) {
      int port = *((int*)current2->element);
      if(lconnection->port == port) {
        to_keep = true;
        break;
      }
    }
    if(!to_keep) {
      destroy_lconnection(lconnection);
    }
  }

  harp_free_list(connection_pool, NULL);
}

void destroy_lconnection(lconnection_t *lconnection) {
  if(close(lconnection->socket) == -1) {
    logerror("destroy_lconnection");
  } else {
    logmsg(LOG_INFO, "Listening socket on port %i closed\n", lconnection->port);
  }
  free_lconnection(lconnection);
}

void destroy_aconnection(aconnection_t *aconnection) {
  if(close(aconnection->socket) == -1) {
    logerror("destroy_aconnection");
  }
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

void create_pollfds(harp_list_t *lconnections, struct pollfd **pollfds_ptr,
                    nfds_t *n_ptr) {
  nfds_t n = harp_length(lconnections);
  struct pollfd *pollfds = (struct pollfd*)scalloc(n, sizeof(struct pollfd));

  int i;
  harp_list_t *current;
  for(i = 0, current = lconnections; current != NULL;
      i++, current = current->next) {
    lconnection_t *lconnection = current->element;
    pollfds[i].fd     = lconnection->socket;
    pollfds[i].events = POLLIN;
  }

  *pollfds_ptr = pollfds;
  *n_ptr       = n;
}

/**
 * Wait for the next connection on any of the socket of the connection pool.
 * @param pollfds Structure to poll the sockets. It should be the same size and in the same order than the list of listening connections.
 * @return An accepted connection or NULL if it timed out.
 */
aconnection_t *get_next_connection(harp_list_t *lconnections,
                                   struct pollfd *pollfds, nfds_t n) {
  harp_list_t *current;
  int i;

  while(1) {
    // FIXME: Any way to make it faster?
    int rc = poll(pollfds, n, 5 * 1000);
    if(rc == -1) {
      logerror("get_next_connection:poll");
    } else if(rc == 0) {
      return NULL;
    }

    for(i = 0, current = lconnections; current != NULL;
        i++, current = current->next) {

      if((pollfds[i].revents & POLLIN) == POLLIN) {
        lconnection_t *lconnection = current->element;
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof(struct sockaddr_storage);

        int sock = accept(lconnection->socket, (struct sockaddr*)&addr, &addrlen);
        if(sock != -1) {
          unsigned int addr_hash = compute_hash((struct sockaddr_in*)&addr);
          return make_aconnection(addr_hash, lconnection->port, sock);
        } else if(sock != EINTR || sock != EWOULDBLOCK) {
          logerror("get_next_connection:accept");
        }
      }
    }
  }
}
