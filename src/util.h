#ifndef UTIL_H
#define UTIL_H

#include "mod.h"
struct timespec;

char *load_average();
char *datetime();
char *battery_level();
void mpc_status_routine(mod *m);
void timespec_relative(struct timespec *ts, long ms);

#endif
