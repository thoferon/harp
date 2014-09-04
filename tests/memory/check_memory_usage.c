// PTHREAD_CREATE_JOINABLE might be defined here
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>

#include <start.h>
#include <memory_tracking.h>

void *start_server(void *ignore) {
  char *argv[3];
  argv[0] = "check_memory_usage";
  argv[1] = "-c";
  argv[2] = TESTSMEMORYDIR "/memory_usage.conf";

  int rc = start(3, argv);
  if(rc != 0) {
    fprintf(stderr, "WARNING: The server returned exit code %i\n", rc);
  }

  pthread_exit(NULL);
}

int main(int argc, char **argv) {
  // Start the server
  int rc;

#define EXIT_WITH_PTHREAD_ERROR(name) do {      \
    if(rc != 0) {                               \
      errno = rc;                               \
      perror(name);                             \
      return EXIT_FAILURE;                      \
    }                                           \
  } while(0)

  pthread_attr_t attr;
  rc = pthread_attr_init(&attr);
  EXIT_WITH_PTHREAD_ERROR("main:pthread_attr_init");
  rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  EXIT_WITH_PTHREAD_ERROR("main:pthread_attr_setdetachstate");

  pthread_t server_thread;
  rc = pthread_create(&server_thread, NULL, (void*)&start_server, NULL);
  EXIT_WITH_PTHREAD_ERROR("main:pthread_create");

  pthread_attr_destroy(&attr);

  sleep(1);

  // Put some load on the server
  struct addrinfo hints, *addrinfos, *current;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE | AI_NUMERICSERV;

  int i, port, sock;
  for(i = 0; i < 3; i++) {
    for(port = 10001; port <= 10003; port++) {
      fprintf(stderr, "Request on port %i\n", port);

      char port_string[6];
      snprintf(port_string, 6, "%i", port);

      while((rc = getaddrinfo("127.0.0.1", port_string, &hints, &addrinfos))
            == EAI_AGAIN);
      if(rc != 0) {
        fprintf(stderr, "main:getaddrinfo: %s\n", gai_strerror(rc));
        return EXIT_FAILURE;
      }

      if(addrinfos != NULL) {
        rc = -1;
        for(current = addrinfos; current != NULL;
            current = current->ai_next) {

          sock = socket(current->ai_family, current->ai_socktype,
                        current->ai_protocol);
          if(sock == -1) {
            perror("main:socket");
            return EXIT_FAILURE;
          }

          rc = connect(sock, current->ai_addr, current->ai_addrlen);
          if(rc == 0) { break; }
          if(errno == EINTR) {
            struct pollfd pollfds[1];
            pollfds[0].fd     = sock;
            pollfds[0].events = POLLOUT;

            rc = poll(pollfds, 1, 3 * 60 * 1000);

            if(rc == -1) {
              perror("main:poll");
              return EXIT_FAILURE;
            } else if(rc == 0) {
              fprintf(stderr, "main:poll: Timed out\n");
              return EXIT_FAILURE;
            }
          }
        }

        freeaddrinfo(addrinfos);
        if(rc == -1) {
          perror("main:connect");
          return EXIT_FAILURE;
        }

      } else {
        fprintf(stderr, "main:getaddrinfo: Empty result\n");
        return EXIT_FAILURE;
      }

      char *request =
        "GET /memory_usage.conf HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
      int sent = 0;
      size_t request_len = strlen(request);
      while(sent < request_len) {
        rc = send(sock, request + sent, request_len - sent, MSG_NOSIGNAL);
        if(rc == -1) {
          perror("main:send");
          return EXIT_FAILURE;
        } else {
          sent += rc;
        }
      }

      char first_character;
      rc = recv(sock, &first_character, 1, MSG_WAITALL);
      if(rc == -1) {
        perror("main:recv");
        return EXIT_FAILURE;
      }

      rc = close(sock);
      if(rc == -1) {
        perror("main:close");
        return EXIT_FAILURE;
      }
    }

    fprintf(stderr, "Sending SIGUSR1\n");
    rc = kill(getpid(), SIGUSR1);
    if(rc != 0) {
      perror("main:kill with SIGUSR1");
      return EXIT_FAILURE;
    }
  }

  // Terminate the server
  fprintf(stderr, "Sending SIGTERM\n");
  rc = kill(getpid(), SIGTERM);
  if(rc != 0) {
    perror("main:kill with SIGTERM");
    return EXIT_FAILURE;
  }

  rc = pthread_join(server_thread, NULL);
  if(rc != 0) {
    errno = rc;
    perror("main:pthread_join");
    return EXIT_FAILURE;
  }

  // Check for memory leak
  if(allocations == NULL) {
    printf("No memory leak detected. :-)\n");
  } else {
    printf("The program is leaking memory:\n\n");
    size_t memory_leaked = 0;
    allocation_t *current;
    for(current = allocations; current != NULL; current = current->next) {
      memory_leaked += current->size;
      printf("%8i bytes leaked at %p by function at %p\n",
             (int)current->size, current->pointer, current->function_pointer);
    }
    printf("\nTotal memory leaked (in bytes): %i\n", (int)memory_leaked);
    abort(); // handy to set a breakpoint
  }

  return EXIT_SUCCESS;
}
