/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "picolibc-hooks.h"

struct cb_bits {
	FILE f;
	cbprintf_cb out;
	void	*ctx;
};

static int cbputc(char c, FILE *_s)
{
	struct cb_bits *s = (struct cb_bits *) _s;

	(*s->out) (c, s->ctx);
	return 0;
}

int cbvprintf(cbprintf_cb out, void *ctx, const char *fp, va_list ap)
{
	struct cb_bits	s = {
		.f = FDEV_SETUP_STREAM(cbputc, NULL, NULL, _FDEV_SETUP_WRITE),
		.out = out,
		.ctx = ctx,
	};
	return vfprintf(&s.f, fp, ap);
}
