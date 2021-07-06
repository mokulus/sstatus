#ifndef BUF_H
#define BUF_H

#include <stddef.h>

typedef struct buf {
	char *buf;
	size_t len;
	size_t cap;
} buf;

int buf_init(buf *b);
int buf_append(buf *b, char *str);

#endif
