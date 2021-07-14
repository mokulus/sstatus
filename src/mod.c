#include "mod.h"
#include "util.h"
#include <stdlib.h>

static void *mod_basic_routine(void *vm);
static void *mod_advanced_routine(void *vm);

void mod_init(mod *m, int *update, pthread_cond_t *update_cond, pthread_mutex_t *update_mutex) {
	m->store = NULL;
	pthread_mutex_init(&m->store_mutex, NULL);
	m->update = update;
	m->update_cond = update_cond;
	m->update_mutex = update_mutex;
	if (m->interval)
		pthread_create(&m->thread, NULL, mod_basic_routine, m);
	else
		pthread_create(&m->thread, NULL, mod_advanced_routine, m);
}

static void *mod_basic_routine(void *vm)
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
	return NULL;
}

static void *mod_advanced_routine(void *vm) {
	mod *m = (mod *)vm;
	m->fp.adv(m);
	return NULL;
}
