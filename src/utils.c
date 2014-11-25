#include <fcntl.h>

#include <log.h>

#include <utils.h>

int set_nonblocking(int sock) {
  int flags = fcntl(sock, F_GETFL, NULL);
  if(flags == -1) {
    logerror("set_nonblocking:fcntl with F_GETFL");
    return -1;
  }

  int rc = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
  if(rc == -1) {
    logerror("set_nonblocking:fcntl with F_SETFL");
    return -1;
  }

  return 0;
}
