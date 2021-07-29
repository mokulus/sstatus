#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *load_average()
{
	double lavg[3];
	if (getloadavg(lavg, 3) == -1)
		return NULL;
	const char *format = "%1.2f %1.2f %1.2f";
	char *str = NULL;
	size_t size = 0;
	int n = snprintf(str, size, format, lavg[0], lavg[1], lavg[2]);
	size = (size_t)n + 1;
	if (!(str = malloc(size)))
		return NULL;
	n = snprintf(str, size, format, lavg[0], lavg[1], lavg[2]);
	if (n < 0) {
		free(str);
		return NULL;
	}
	return str;
}

char *datetime()
{
	const char *format = "%a %-d %b %H:%M";

	size_t n = 64;
	char *str = malloc(n);
	if (!str)
		return NULL;
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
	FILE *f = fopen("/sys/class/power_supply/BAT0/capacity", "r");
	if (!f)
		return NULL;
	char *str = NULL;
	size_t n = 0;
	if (getline(&str, &n, f) == -1)
		goto fail;
	/* the newline is included, replace it with % */
	*strchr(str, '\n') = '%';
fail:
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
