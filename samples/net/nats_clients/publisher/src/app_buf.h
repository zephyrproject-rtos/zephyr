#ifndef _APP_BUF_H_
#define _APP_BUF_H_

#include <zephyr.h>		/* for __deprecated */
#include <stdint.h>
#include <stddef.h>

struct __deprecated app_buf_t;

struct app_buf_t {
	uint8_t *buf;
	size_t size;
	size_t length;
};

#define APP_BUF_INIT(_buf, _size, _length) {.buf = _buf, .size = _size,\
					    .length = _length}

#endif
