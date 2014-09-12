#include <config.h>

#include <errno.h>
#include <string.h>

#include <harp.h>

// FIXME: thread-local storage?
int harp_errno = 0;
char harp_errstr[1024];

char *harp_strerror(int errnum) {
  switch(errnum) {
  case HARP_ERROR_INVALID_FILTER:
    return "Invalid filter";
  case HARP_ERROR_INVALID_RESOLVER:
    return "Invalid resolver";
  case HARP_ERROR_PARSE_ERROR:
    return harp_errstr;
  case HARP_ERROR_NO_CONFIG:
    return "No configuration found";
  default: return strerror(errnum);
  }
}
