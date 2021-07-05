#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <bsd/string.h>

char *load_average() {
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

char *datetime() {
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

char *battery_level() {
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

void msleep(size_t ms) {
	time_t sec = ms / 1000;
	long rem = ms % 1000;
	struct timespec ts = {sec, 1e6 * rem};
	nanosleep(&ts, NULL);
}

void *mod_routine(void *vm) {
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

void *mpc_status_routine(void *vm) {
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
			execvp("sh", (char * const []){"sh", "-c",
					"mpc status | "
					"sed 1q | "
					"grep -v 'volume: n/a' | "
					/* "tr -dc '[:print:]' | " */
					"awk '{ if ($0 ~ /.{30,}/) { print substr($0, 1, 29) \"…\" } else { print $0 } }'"
					, (char *) NULL});
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
			execvp("mpc", (char * const []){"mpc", "idle", (char *) NULL});
			exit(1);
		}
		waitpid(id, NULL, 0);
	}
	return NULL;
}

typedef struct buf {
	char *buf;
	size_t len;
	size_t cap;
} buf;

void buf_init(buf *b) {
	b->cap = 32;
	b->len = 0;
	b->buf = malloc(b->cap);
	b->buf[0] = '\0';
}

void buf_append(buf *b, char *str) {
	if (!str)
		return;
	const size_t len = strlen(str);
	while (b->len + len + 1 > b->cap) {
		b->cap = 3 * b->cap / 2;
		b->buf = realloc(b->buf, b->cap);
	}
	memcpy(b->buf + b->len, str, len);
	b->len += len;
	b->buf[b->len] = '\0';
}

static mod mods[] = {
	{ { .adv = mpc_status_routine}, 0 },
	{ {load_average}, 60 * 1000 },
	{ {battery_level}, 10 * 1000 },
	{ {datetime}, 60 * 1000 },
};

int main() {
	/* printf("%s\n", load_average()); */
	/* printf("%s\n", battery_level()); */
	/* printf("%s\n", datetime()); */
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
