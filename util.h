#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

char *load_average();
char *datetime();
char *battery_level();
void msleep(size_t ms);

#endif
