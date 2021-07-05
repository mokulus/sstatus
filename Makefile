NAME=sstatus
CC=gcc
COMMON_FLAGS=-Wall -Wextra -Wpedantic -Wno-missing-field-initializers
LDFLAGS=-lbsd -pthread
RFLAGS=-O2
DFLAGS=-Og -g
FLAGS=$(COMMON_FLAGS)

SRC=sstatus.c

release: FLAGS += $(RFLAGS)
release: $(NAME)

debug: FLAGS += $(DFLAGS)
debug: $(NAME)

$(NAME): $(SRC)
	$(CC) $(FLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(NAME)

install: release
	cp -f $(NAME) /usr/local/bin/$(NAME)

uninstall:
	rm -f /usr/local/bin/$(NAME)

.PHONY: release debug clean install uninstall
