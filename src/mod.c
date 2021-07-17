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
	free(m->store);
	pthread_mutex_destroy(&m->store_mutex);
	pthread_mutex_destroy(&m->exit_mutex);
	pthread_cond_destroy(&m->exit_cond);
}

void mod_safe_new_store(mod *m, char *str)
{
	pthread_mutex_lock(&m->store_mutex);
	free(m->store);
	m->store = str;
	pthread_mutex_unlock(&m->store_mutex);

	pthread_mutex_lock(m->update_mutex);
	*m->update = 1;
	pthread_cond_signal(m->update_cond);
	pthread_mutex_unlock(m->update_mutex);
}

unsigned mod_safe_should_exit(mod *m)
{
	unsigned exit = 0;
	pthread_mutex_lock(&m->exit_mutex);
	exit = m->exit;
	pthread_mutex_unlock(&m->exit_mutex);
	return exit;
}

void mod_safe_set_exit(mod *m)
{
	pthread_mutex_lock(&m->exit_mutex);
	m->exit = 1;
	pthread_cond_signal(&m->exit_cond);
	pthread_mutex_unlock(&m->exit_mutex);
}

static void *mod_basic_routine(void *vm)
{
	mod *m = (mod *)vm;
	pthread_block_signals();
	struct timespec ts;
	while (!mod_safe_should_exit(m)) {
		mod_safe_new_store(m, m->fp.basic());

		timespec_relative(&ts, m->interval);
		pthread_mutex_lock(&m->exit_mutex);
		int rc = 0;
		while (!m->exit && rc == 0)
			rc = pthread_cond_timedwait(&m->exit_cond,
						    &m->exit_mutex, &ts);
		pthread_mutex_unlock(&m->exit_mutex);
	}
	return NULL;
}

static void empty_handler(int signal) { (void)signal; }

static void *mod_advanced_routine(void *vm)
{
	pthread_block_signals();

	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = empty_handler;
	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		perror("couldn't register SIGUSR1 signal handler");
		return NULL;
	}

	mod *m = (mod *)vm;
	m->fp.adv(m);
	return NULL;
}
