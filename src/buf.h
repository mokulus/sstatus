#ifndef BUF_H
#define BUF_H

#include <stddef.h>

typedef struct buf {
	char *buf;
	size_t len;
	size_t cap;
} buf;

buf *buf_new(void);
int buf_append(buf *b, const char *str);
void buf_free(buf *b);

#endif
