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
	const char *format = "%a %d %b %H:%M";

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
	fclose(f);
	return str;
}

void msleep(size_t ms)
{
	time_t sec = ms / 1000;
	long rem = ms % 1000;
	struct timespec ts = {sec, 1e6 * rem};
	nanosleep(&ts, NULL);
}
