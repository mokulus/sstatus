#include "mod.h"
#include "util.h"
#include <errno.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void *mod_basic_routine(void *vm);
static void *mod_advanced_routine(void *vm);
static void mod_block_signals(void);

void mod_init(mod *m, sem_t *update_sem)
{
	m->store = NULL;
	m->update_sem = update_sem;
	pthread_mutex_init(&m->store_mutex, NULL);
	sem_init(&m->exit_sem, 0, 0);
	if (m->interval)
		pthread_create(&m->thread, NULL, mod_basic_routine, m);
	else
		pthread_create(&m->thread, NULL, mod_advanced_routine, m);
}

void mod_destroy(mod *m)
{
	mod_set_exit(m);
	if (!m->interval) {
		pthread_kill(m->thread, SIGUSR1);
	}
	pthread_join(m->thread, NULL);
	free(m->store);
	pthread_mutex_destroy(&m->store_mutex);
	sem_destroy(&m->exit_sem);
}

void mod_store(mod *m, char *str)
{
	pthread_mutex_lock(&m->store_mutex);
	free(m->store);
	m->store = str;
	pthread_mutex_unlock(&m->store_mutex);

	sem_post(m->update_sem);
}

unsigned mod_should_exit(mod *m)
{
	int old_errno = errno;
	unsigned ret = sem_trywait(&m->exit_sem) == 0;
	errno = old_errno;
	return ret;
}

void mod_set_exit(mod *m)
{
	sem_post(&m->exit_sem);
}

static void *mod_basic_routine(void *vm)
{
	mod *m = (mod *)vm;
	mod_block_signals();
	struct timespec ts;
	while (!mod_should_exit(m)) {
		mod_store(m, m->fp.basic());
		timespec_relative(&ts, m->interval);
		/* have to lock it for mod_should_exit */
		if (!sem_timedwait(&m->exit_sem, &ts))
			sem_post(&m->exit_sem);
	}
	return NULL;
}

static void empty_handler(int signal)
{
	(void)signal;
}

static void *mod_advanced_routine(void *vm)
{
	mod_block_signals();

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

static void mod_block_signals(void)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}
