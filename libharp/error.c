#include <config.h>

#include <errno.h>
#include <string.h>

#include <harp.h>

// This file is not really useful for now. If some function needs to
// return an error, it could set harp_errno to errno for standard errors
// or to a custom HARP_ERROR_X being a negative number.

// FIXME: thread-local storage?
int harp_errno;

char *harp_strerror(int errnum) {
  return strerror(errnum);
}
