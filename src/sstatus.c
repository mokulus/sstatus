#include <bsd/string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "buf.h"
#include "mod.h"
#include "util.h"

static volatile sig_atomic_t g_quit = 0;

static void set_quit_handler(int signal)
{
	(void)signal;
	g_quit = 1;
}

void mpc_status_routine(mod *m)
{
	while (!mod_safe_should_exit(m)) {
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

		mod_safe_new_store(m, str);
		mod_safe_update_signal(m);

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
		if (mod_safe_should_exit(m))
			continue;
		int rc = 0;
		waitpid(id, &rc, 0);
		if (WIFEXITED(rc) && WEXITSTATUS(rc)) {
			fprintf(stderr, "mpc idle failed\n");
			mod_safe_set_exit(m);
			continue;
		}
	}
}

static mod mods[] = {
    {.fp = {.adv = mpc_status_routine}, .interval = 0},
    {.fp = {.basic = load_average}, .interval = 60 * 1000},
    {.fp = {.basic = battery_level}, .interval = 60 * 1000},
    {.fp = {.basic = datetime}, .interval = 60 * 1000},
};

static unsigned register_signals(void)
{
	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = set_quit_handler;
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("couldn't register SIGINT signal handler");
		return 0;
	}
	if (sigaction(SIGTERM, &sa, NULL) == -1) {
		perror("couldn't register SIGTERM signal handler");
		return 0;
	}
	return 1;
}

int main()
{
	if (!register_signals())
		return 1;
	const size_t mod_count = sizeof(mods) / sizeof(mod);
	pthread_cond_t update_cond;
	pthread_cond_init(&update_cond, NULL);
	pthread_mutex_t update_mutex;
	pthread_mutex_init(&update_mutex, NULL);
	int update = 0;
	for (size_t i = 0; i < mod_count; ++i) {
		mod_init(&mods[i], &update, &update_cond, &update_mutex);
	}
	char *oldbuf = NULL;
	buf *b = NULL;
	if (!(oldbuf = strdup("")))
		goto fail;
	if (!(b = buf_new()))
		goto fail;
	struct timespec ts;
	while (!g_quit) {
		pthread_mutex_lock(&update_mutex);
		timespec_relative(&ts, 20);
		int rc = 0;
		while (!update && !g_quit && rc == 0)
			rc = pthread_cond_timedwait(&update_cond, &update_mutex,
						    &ts);
		int should_update = update;
		update = 0;
		pthread_mutex_unlock(&update_mutex);
		/* if not g_quit or just timeout (rc != 0) */
		if (!should_update)
			continue;

		for (size_t i = 0; i < mod_count; ++i) {
			mod *m = &mods[i];
			pthread_mutex_lock(&m->store_mutex);
			if (m->store && strcmp(m->store, "")) {
				buf_append(b, " | ");
				buf_append(b, m->store);
			}
			pthread_mutex_unlock(&m->store_mutex);
		}
		if (strcmp(b->buf, oldbuf)) {
			fflush(stdout);
			puts(b->buf);
			fflush(stdout);
			free(oldbuf);
			oldbuf = strdup(b->buf);
		}
		b->len = 0;
	}
fail:
	free(oldbuf);
	buf_free(b);
	for (size_t i = 0; i < mod_count; ++i) {
		mod *m = &mods[i];
		pthread_mutex_lock(&m->exit_mutex);
		m->exit = 1;
		pthread_cond_signal(&m->exit_cond);
		pthread_mutex_unlock(&m->exit_mutex);
		if (!m->interval) {
			pthread_kill(m->thread, SIGUSR1);
		}
		pthread_join(m->thread, NULL);
		mod_deinit(m);
	}
}
