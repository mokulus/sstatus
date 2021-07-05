NAME=sstatus
COMMON_FLAGS=-Wall -Wextra -Wpedantic -Wno-missing-field-initializers
LDFLAGS=-lbsd -pthread
RFLAGS=-O2 $(COMMON_FLAGS)
DFLAGS=-Og -g $(COMMON_FLAGS)

SRC = $(wildcard src/*.c)
HDR = $(wildcard src/*.h)

ROBJ = $(SRC:src/%.c=release/obj/%.o)
DOBJ = $(SRC:src/%.c=debug/obj/%.o)

release: release/$(NAME)

release/$(NAME): $(ROBJ)
	$(CC) $(RLAGS) $(LDFLAGS) $^ -o $@

release/obj/%.o: src/%.c | release/obj
	$(CC) $(RFLAGS) $^ -c -o $@

release/obj:
	mkdir -p $@


debug: debug/$(NAME)

debug/$(NAME): $(DOBJ)
	$(CC) $(DLAGS) $(LDFLAGS) $^ -o $@

debug/obj/%.o: src/%.c | debug/obj
	$(CC) $(DFLAGS) $^ -c -o $@

debug/obj:
	mkdir -p $@


format: $(SRC) $(HDR)
	astyle -n --style=linux --indent=tab $^

clean:
	rm -rf $(NAME) release debug

install: release
	cp -f release/$(NAME) /usr/local/bin/$(NAME)

uninstall:
	rm -f /usr/local/bin/$(NAME)

.PHONY: release debug format clean install uninstall
