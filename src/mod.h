#ifndef MOD_H
#define MOD_H

#include <time.h>
#include <pthread.h>

typedef struct mod mod;
struct mod {
	union {
		char *(*basic)();
		void (*adv)(mod *m);
	} fp;
	time_t interval;
	char *store;
	pthread_mutex_t store_mutex;
	int *update;
	pthread_cond_t *update_cond;
	pthread_mutex_t *update_mutex;
	pthread_t thread;
};

void mod_init(mod *m, int *update, pthread_cond_t *update_cond, pthread_mutex_t *update_mutex);

#endif
