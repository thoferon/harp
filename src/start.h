#ifndef START_H
#define START_H 1

#include <stdbool.h>

extern volatile bool global_ready;

int start(int argc, char **argv);

#endif
