#ifndef LOG_H
#define LOG_H 1

#include <syslog.h> // for levels

void logmsg(int, const char *, ...);
void logerror(const char *);

#endif
