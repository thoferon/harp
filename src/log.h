#ifndef LOG_H
#define LOG_H 1

#include <syslog.h> // for levels

#include <options.h>

void initialize_logger(options_t *);

void logmsg(int, const char *, ...);
void logerror(const char *);

#endif
