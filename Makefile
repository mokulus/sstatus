sstatus: sstatus.c
	gcc -O2 -g -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -lbsd -lpthread $^ -o $@

clean:
	rm -f sstatus

install: sstatus
	cp -f sstatus /usr/local/bin/sstatus

uninstall:
	rm -f /usr/local/bin/sstatus

.PHONY: clean
