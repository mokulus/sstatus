#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <time.h>

char *load_average();
char *datetime();
char *battery_level();
void msleep(time_t ms);

#endif
