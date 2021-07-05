#include <bsd/string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "buf.h"
#include "util.h"

typedef struct mod {
	union {
		char *(*basic)();
		void *(*adv)(void *vp);
	} fp;
	size_t interval;
	char *store;
	pthread_mutex_t store_mutex;
	int *update;
	pthread_cond_t *update_cond;
	pthread_mutex_t *update_mutex;
	pthread_t thread;
} mod;

void *mod_routine(void *vm)
{
	mod *m = (mod *)vm;
	for (;;) {
		char *str = m->fp.basic();
		pthread_mutex_lock(&m->store_mutex);
		free(m->store);
		m->store = str;
		pthread_mutex_unlock(&m->store_mutex);

		pthread_mutex_lock(m->update_mutex);
		*m->update = 1;
		pthread_cond_signal(m->update_cond);
		pthread_mutex_unlock(m->update_mutex);

		msleep(m->interval);
	}
}

void *mpc_status_routine(void *vm)
{
	mod *m = vm;
	for (;;) {
		int p[2];
		if (pipe(p) == -1)
			return NULL;
		pid_t id = fork();
		if (id == -1)
			return NULL;
		if (id == 0) {
			close(p[0]);
			dup2(p[1], 1);
			execvp("sh", (char *const[]) {
				"sh", "-c",
				"mpc status | "
				"sed 1q | "
				"grep -v 'volume: n/a' | "
				/* "tr -dc '[:print:]' | " */
				"awk '{ if ($0 ~ /.{30,}/) { print "
				"substr($0, 1, "
				"29) \"…\" } else { print $0 } }'",
				(char *)NULL
			});
			exit(1);
		}
		close(p[1]);
		char *str = NULL;
		size_t n = 0;
		FILE *f = fdopen(p[0], "r");
		size_t read = getline(&str, &n, f);
		str[read - 1] = '\0';
		fclose(f);

		pthread_mutex_lock(&m->store_mutex);
		free(m->store);
		m->store = str;
		pthread_mutex_unlock(&m->store_mutex);

		pthread_mutex_lock(m->update_mutex);
		*m->update = 1;
		pthread_cond_signal(m->update_cond);
		pthread_mutex_unlock(m->update_mutex);

		id = fork();
		if (id == -1)
			return NULL;
		if (id == 0) {
			int fd = open("/dev/null", O_WRONLY);
			dup2(fd, 1);
			execvp("mpc",
			(char *const[]) {
				"mpc", "idle", (char *)NULL
			});
			exit(1);
		}
		waitpid(id, NULL, 0);
	}
	return NULL;
}

static mod mods[] = {
	{{.adv = mpc_status_routine}, 0},
	{{load_average}, 60 * 1000},
	{{battery_level}, 10 * 1000},
	{{datetime}, 60 * 1000},
};

int main()
{
	const size_t mod_count = sizeof(mods) / sizeof(mod);
	pthread_cond_t update_cond;
	pthread_cond_init(&update_cond, NULL);
	pthread_mutex_t update_mutex;
	pthread_mutex_init(&update_mutex, NULL);
	int update = 0;
	for (size_t i = 0; i < mod_count; ++i) {
		mod *m = &mods[i];
		m->store = NULL;
		pthread_mutex_init(&m->store_mutex, NULL);
		m->update = &update;
		m->update_cond = &update_cond;
		m->update_mutex = &update_mutex;
		if (m->interval)
			pthread_create(&m->thread, NULL, mod_routine, m);
		else
			pthread_create(&m->thread, NULL, m->fp.adv, m);
	}
	char *oldbuf = strdup("");
	buf b;
	buf_init(&b);
	for (;;) {
		pthread_mutex_lock(&update_mutex);
		while (!update)
			pthread_cond_wait(&update_cond, &update_mutex);
		update = 0;
		pthread_mutex_unlock(&update_mutex);

		for (size_t i = 0; i < mod_count; ++i) {
			mod *m = &mods[i];
			pthread_mutex_lock(&m->store_mutex);
			if (m->store && strcmp(m->store, "")) {
				buf_append(&b, " | ");
				buf_append(&b, m->store);
			}
			pthread_mutex_unlock(&m->store_mutex);
		}
		if (strcmp(b.buf, oldbuf)) {
			puts(b.buf);
			fflush(stdout);
		}
		free(oldbuf);
		oldbuf = b.buf;
		b.buf = malloc(b.cap);
		b.len = 0;
	}
}
