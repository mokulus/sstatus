#ifndef MOD_H
#define MOD_H

#include <pthread.h>
#include <semaphore.h>
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
	sem_t *update_sem;
	sem_t exit_sem;
	pthread_t thread;
};

void mod_init(mod *m, sem_t *update_sem);
void mod_deinit(mod *m);
void mod_safe_new_store(mod *m, char *str);
unsigned mod_safe_should_exit(mod *m);
void mod_safe_set_exit(mod *m);

#endif
