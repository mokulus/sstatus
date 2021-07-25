#include "mpc.h"
#include "mod.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void mpc_status_routine(mod *m)
{
	int rc;
	while (!mod_should_exit(m)) {
		int p[2];
		if (pipe(p) == -1) {
			perror("pipe");
			mod_set_exit(m);
			continue;
		}
		pid_t id = fork();
		if (id == -1) {
			perror("fork");
			mod_set_exit(m);
			close(p[0]);
			close(p[1]);
			continue;
		}
		if (id == 0) {
			close(p[0]);
			dup2(p[1], 1);
			execlp("sh",
			       "sh",
			       "-c",
			       "mpc status | "
			       "sed 1q | "
			       "grep -v 'volume: ' | "
			       /* "tr -dc '[:print:]' | " */
			       "awk '{ if ($0 ~ /.{30,}/) { print "
			       "substr($0, 1, "
			       "29) \"…\" } else { print $0 } }'",
			       (char *)NULL);
			exit(1);
		}
		close(p[1]);
		char *str = NULL;
		size_t n = 0;
		FILE *f = fdopen(p[0], "r");
		ssize_t read = getline(&str, &n, f);
		if (read == -1) {
			fclose(f);
			wait(NULL);
			if (errno) {
				perror("getline");
				mod_set_exit(m);
			} else {
				/* read is -1 and no errno */
				/* so the pipe was empty/broken */
				/* when playlist is empty */
				/* grep -v discards everything */
				mod_store(m, NULL);
			}
			continue;
		}
		str[read - 1] = '\0';
		fclose(f);
		waitpid(id, &rc, 0);
		if (WIFEXITED(rc) && WEXITSTATUS(rc)) {
			fputs("mpc status failed\n", stderr);
			mod_set_exit(m);
			continue;
		}

		mod_store(m, str);

		id = fork();
		if (id == -1)
			return;
		if (id == 0) {
			int fd = open("/dev/null", O_WRONLY);
			dup2(fd, 1);
			execlp("mpc", "mpc", "idle", (char *)NULL);
			exit(1);
		}
		/* check in case we got signal just before wait
		   if we did we'd only learn after mpc idle,
		   which would delay exit
		   we have to kill and reap the child
		*/
		if (mod_should_exit(m)) {
			kill(id, SIGTERM);
		}
		waitpid(id, &rc, 0);
		if (WIFEXITED(rc) && WEXITSTATUS(rc)) {
			fputs("mpc idle failed\n", stderr);
			mod_set_exit(m);
			continue;
		}
	}
}
