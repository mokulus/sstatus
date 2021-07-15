#include "mod.h"
#include "util.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void *mod_basic_routine(void *vm);
static void *mod_advanced_routine(void *vm);

void mod_init(mod *m, int *update, pthread_cond_t *update_cond,
	      pthread_mutex_t *update_mutex)
{
	m->store = NULL;
	pthread_mutex_init(&m->store_mutex, NULL);
	m->exit = 0;
	pthread_mutex_init(&m->exit_mutex, NULL);
	pthread_cond_init(&m->exit_cond, NULL);
	m->update = update;
	m->update_cond = update_cond;
	m->update_mutex = update_mutex;
	if (m->interval)
		pthread_create(&m->thread, NULL, mod_basic_routine, m);
	else
		pthread_create(&m->thread, NULL, mod_advanced_routine, m);
}

void mod_deinit(mod *m)
{
	pthread_mutex_destroy(&m->store_mutex);
	pthread_mutex_destroy(&m->exit_mutex);
	pthread_cond_destroy(&m->exit_cond);
}

static void *mod_basic_routine(void *vm)
{
	mod *m = (mod *)vm;
	unsigned exit = 0;
	struct timespec ts;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	while (!exit) {
		char *str = m->fp.basic();
		pthread_mutex_lock(&m->store_mutex);
		free(m->store);
		m->store = str;
		pthread_mutex_unlock(&m->store_mutex);

		pthread_mutex_lock(m->update_mutex);
		*m->update = 1;
		pthread_cond_signal(m->update_cond);
		pthread_mutex_unlock(m->update_mutex);

		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += m->interval / 1000;
		ts.tv_nsec += (m->interval % 1000) * 1000 * 1000;
		pthread_mutex_lock(&m->exit_mutex);
		int rc = 0;
		while (!m->exit && rc == 0)
			rc = pthread_cond_timedwait(&m->exit_cond,
						    &m->exit_mutex, &ts);
		exit = m->exit;
		pthread_mutex_unlock(&m->exit_mutex);
	}
	pthread_mutex_lock(&m->store_mutex);
	free(m->store);
	m->store = NULL;
	pthread_mutex_unlock(&m->store_mutex);
	return NULL;
}

static void *mod_advanced_routine(void *vm)
{
	mod *m = (mod *)vm;
	m->fp.adv(m);
	return NULL;
}
