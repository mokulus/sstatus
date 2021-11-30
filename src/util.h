#ifndef UTIL_H
#define UTIL_H

struct timespec;

char *load_average();
char *datetime();
char *battery_level();
char *temperature();
void timespec_relative(struct timespec *ts, long ms);

#endif
