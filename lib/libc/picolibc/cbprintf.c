/*
 * Copyright © 2021, Keith Packard <keithp@keithp.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "picolibc-hooks.h"

/*
 * When using Clang with a GCC-compiled picolibc (e.g. from the Zephyr SDK),
 * the va_list ABI may not be compatible across the Clang→GCC boundary on
 * Xtensa windowed targets. Routing through picolibc's vfprintf causes crashes
 * because the GCC-compiled __d_vfprintf misinterprets the Clang-constructed
 * va_list.
 *
 * To avoid this, use Zephyr's own cbvprintf_complete/cbvprintf_nano
 * (compiled by the same compiler as the caller) as the formatting engine.
 * Picolibc is still used for headers (math.h, stdio.h) and libm.
 */
#if defined(__clang__)

/* Use Zephyr's native cbprintf implementation.
 * cbvprintf is defined as a weak alias in cbprintf_complete.c / cbprintf_nano.c
 * and will be linked from there.  We simply don't provide our own.
 */

#else /* GCC - original picolibc path */

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

#endif /* __clang__ */
