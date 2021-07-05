NAME=sstatus
COMMON_FLAGS=-Wall -Wextra -Wpedantic -Wno-missing-field-initializers
LDFLAGS=-lbsd -pthread
RFLAGS=-O2 $(COMMON_FLAGS)
DFLAGS=-Og -g $(COMMON_FLAGS)

SRC = $(wildcard *.c)
HDR = $(wildcard *.h)

ROBJ = $(SRC:%.c=release/obj/%.o)
DOBJ = $(SRC:%.c=debug/obj/%.o)

release: release/$(NAME)

release/$(NAME): $(ROBJ)
	$(CC) $(RLAGS) $(LDFLAGS) $^ -o $@

release/obj/%.o: %.c | release/obj
	$(CC) $(RFLAGS) $^ -c -o $@

release/obj:
	mkdir -p $@


debug: debug/$(NAME)

debug/$(NAME): $(DOBJ)
	$(CC) $(DLAGS) $(LDFLAGS) $^ -o $@

debug/obj/%.o: %.c | debug/obj
	$(CC) $(DFLAGS) $^ -c -o $@

debug/obj:
	mkdir -p $@


clean:
	rm -rf $(NAME) release debug

install: release
	cp -f release/$(NAME) /usr/local/bin/$(NAME)

uninstall:
	rm -f /usr/local/bin/$(NAME)

.PHONY: release debug clean install uninstall
