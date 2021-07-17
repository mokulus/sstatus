#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *load_average()
{
	char *str = NULL;
	FILE *f = fopen("/proc/loadavg", "r");
	if (!f)
		goto fail;
	size_t n = 0;
	if (getline(&str, &n, f) == -1)
		goto fail;
	char *cut = str;
	for (int i = 0; i < 3; ++i)
		cut = strchr(cut, ' ') + 1;
	cut--; /* went 1 char after space, move back */
	*cut = '\0';
fail:
	fclose(f);
	return str;
}

char *datetime()
{
	const char *format = "%a %-d %b %H:%M";

	size_t n = 64;
	char *str = malloc(n);
	time_t now = time(NULL);
	struct tm tmp;
	localtime_r(&now, &tmp);
	while (strftime(str, n, format, &tmp) == 0) {
		n *= 2;
		char *tmp = realloc(str, n);
		if (!tmp) {
			free(str);
			return NULL;
		}
		str = tmp;
	};
	return str;
}

char *battery_level()
{
	char *str = NULL;
	FILE *f = fopen("/sys/class/power_supply/BAT0/capacity", "r");
	if (!f)
		goto fail;
	size_t n = 0;
	if (getline(&str, &n, f) == -1)
		goto fail;
	/* the newline is included, replace it with % */
	*strchr(str, '\n') = '%';
fail:
	if (f)
		fclose(f);
	return str;
}

void timespec_relative(struct timespec *ts, long ms)
{
	if (clock_gettime(CLOCK_REALTIME, ts))
		perror("clock_gettime");
	long ms2ns = 1000 * 1000;
	long ms2s = ms2ns * 1000;
	ts->tv_nsec += ms * ms2ns;
	ts->tv_sec += ts->tv_nsec / ms2s;
	ts->tv_nsec = ts->tv_nsec % ms2s;
}
