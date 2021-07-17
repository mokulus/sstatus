#include "buf.h"
#include <stdlib.h>
#include <string.h>

buf *buf_new(void)
{
	buf *b = malloc(sizeof(*b));
	if (!b)
		return b;
	b->cap = 32;
	b->len = 0;
	b->buf = malloc(b->cap);
	if (!b->buf) {
		free(b);
		return NULL;
	}
	b->buf[0] = '\0';
	return b;
}

int buf_append(buf *b, const char *str)
{
	if (!str)
		return 1;
	const size_t len = strlen(str);
	while (b->len + len + 1 > b->cap) {
		b->cap = 3 * b->cap / 2;
		void *p = realloc(b->buf, b->cap);
		if (!p) {
			return 0;
		}
		b->buf = p;
	}
	memcpy(b->buf + b->len, str, len);
	b->len += len;
	b->buf[b->len] = '\0';
	return 1;
}

void buf_free(buf *b)
{
	if (!b)
		return;
	free(b->buf);
	free(b);
}
