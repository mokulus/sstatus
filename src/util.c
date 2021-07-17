#include "util.h"
#include <signal.h>
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

void mpc_status_routine(mod *m)
{
	while (!mod_should_exit(m)) {
		int p[2];
		if (pipe(p) == -1)
			return;
		pid_t id = fork();
		if (id == -1)
			return;
		if (id == 0) {
			close(p[0]);
			dup2(p[1], 1);
			execlp("sh", "sh", "-c",
			       "mpc status | "
			       "sed 1q | "
			       "grep -v 'volume: n/a' | "
			       /* "tr -dc '[:print:]' | " */
			       "awk '{ if ($0 ~ /.{30,}/) { print "
			       "substr($0, 1, "
			       "29) \"…\" } else { print $0 } }'",
			       (char *)NULL);
			exit(1);
		}
		close(p[1]);
		char *str = NULL;
		size_t n = 0;
		FILE *f = fdopen(p[0], "r");
		ssize_t read = getline(&str, &n, f);
		str[read - 1] = '\0';
		fclose(f);

		mod_new_store(m, str);

		id = fork();
		if (id == -1)
			return;
		if (id == 0) {
			int fd = open("/dev/null", O_WRONLY);
			dup2(fd, 1);
			execlp("mpc", "mpc", "idle", (char *)NULL);
			exit(1);
		}
		/* check in case we got signal just before wait
		   if we did we'd only learn after mpc idle,
		   which would delay exit
		*/
		if (mod_should_exit(m))
			continue;
		int rc = 0;
		waitpid(id, &rc, 0);
		if (WIFEXITED(rc) && WEXITSTATUS(rc)) {
			fprintf(stderr, "mpc idle failed\n");
			mod_set_exit(m);
			continue;
		}
	}
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

void pthread_block_signals(void)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}
