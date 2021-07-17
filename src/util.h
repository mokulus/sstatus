#ifndef UTIL_H
#define UTIL_H

#include <time.h>

char *load_average();
char *datetime();
char *battery_level();
void mpc_status_routine(mod *m);
void timespec_relative(struct timespec *ts, long ms);
void pthread_block_signals(void);

#endif
