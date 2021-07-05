#include "buf.h"
#include <stdlib.h>
#include <string.h>

void buf_init(buf *b)
{
	b->cap = 32;
	b->len = 0;
	b->buf = malloc(b->cap);
	b->buf[0] = '\0';
}

void buf_append(buf *b, char *str)
{
	if (!str)
		return;
	const size_t len = strlen(str);
	while (b->len + len + 1 > b->cap) {
		b->cap = 3 * b->cap / 2;
		b->buf = realloc(b->buf, b->cap);
	}
	memcpy(b->buf + b->len, str, len);
	b->len += len;
	b->buf[b->len] = '\0';
}
