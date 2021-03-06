#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mod.h"
#include "mpc.h"
#include "util.h"

static volatile sig_atomic_t g_quit = 0;

static void set_quit_handler(int signal)
{
	(void)signal;
	g_quit = 1;
}

static mod mods[] = {
    {.fp = {.adv = mpc_status_routine}, .interval = 0},
    {.fp = {.basic = load_average}, .interval = 1000},
    {.fp = {.basic = battery_level}, .interval = 1000},
    {.fp = {.basic = temperature}, .interval = 1000},
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
	sem_t update_sem;
	sem_init(&update_sem, 0, 0);
	for (size_t i = 0; i < mod_count; ++i) {
		mod_init(&mods[i], &update_sem);
	}
	while (!g_quit) {
		if (sem_wait(&update_sem))
			continue;

		for (size_t i = 0; i < mod_count; ++i) {
			mod *m = &mods[i];
			pthread_mutex_lock(&m->store_mutex);
			if (m->store && strcmp(m->store, "")) {
				fputs(" | ", stdout);
				fputs(m->store, stdout);
			}
			pthread_mutex_unlock(&m->store_mutex);
		}
		putchar('\n');
		fflush(stdout);
	}
	for (size_t i = 0; i < mod_count; ++i) {
		mod_destroy(&mods[i]);
	}
}
