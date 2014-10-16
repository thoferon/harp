#include <config.h>

#include <syslog.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <smem.h>
#include <options.h>

#include <log.h>

int global_facility = 0;

void initialize_logger(options_t *options) {
  global_facility = options->syslog_facility;
}

#ifdef SYSLOG_LOGGING

void logmsg(int priority, const char *format, ...) {
  va_list args;
  va_start(args, format);
  vsyslog(global_facility | priority, format, args);
  va_end(args);
}

void logerror(const char *name) {
  size_t name_len = strlen(name);
  char *format = (char*)smalloc(name_len + strlen(": %s") + 1);
  strncpy(format, name, name_len);
  strncat(format, ": %s", 4);
  format[name_len + 4] = '\0';

  perror(name);
  syslog(global_facility | LOG_ERR, format, strerror(errno));

  free(format);
}

#else

void logmsg(int priority, const char *format, ...) {
  va_list args;
  va_start(args, format);

  char *level = "";
  switch(priority) {
  case LOG_EMERG:   level = "EMERG: ";   break;
  case LOG_ALERT:   level = "ALERT: ";   break;
  case LOG_CRIT:    level = "CRIT: ";    break;
  case LOG_ERR:     level = "ERR: ";     break;
  case LOG_WARNING: level = "WARNING: "; break;
  case LOG_NOTICE:  level = "NOTICE: ";  break;
  case LOG_INFO:    level = "INFO: ";    break;
  case LOG_DEBUG:   level = "DEBUG: ";   break;
  }

  fprintf(stderr, "%s", level);
  vfprintf(stderr, format, args);

  va_end(args);
}

void logerror(const char *name) {
  fprintf(stderr, "ERR: %s: %s\n", name, strerror(errno));
}

#endif
