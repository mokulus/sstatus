#ifndef MOD_H
#define MOD_H

#include <pthread.h>
#include <time.h>

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
	unsigned exit;
	pthread_mutex_t exit_mutex;
	pthread_cond_t exit_cond;
	pthread_t thread;
};

void mod_init(mod *m, int *update, pthread_cond_t *update_cond,
	      pthread_mutex_t *update_mutex);
void mod_deinit(mod *m);
void mod_safe_new_store(mod *m, char *str);
void mod_safe_update_signal(mod *m);
unsigned mod_safe_should_exit(mod *m);
void mod_safe_set_exit(mod *m);

#endif
