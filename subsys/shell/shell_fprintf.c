/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell_fprintf.h>

#ifdef CONFIG_NEWLIB_LIBC
typedef int (*out_func_t)(int c, void *ctx);
extern void _vprintk(out_func_t out, void *ctx, const char *fmt, va_list ap);
#else
extern int _prf(int (*func)(), void *dest, char *format, va_list vargs);
#endif

static int out_func(int c, void *ctx)
{
	const struct shell_fprintf *sh_fprintf;

	sh_fprintf = (const struct shell_fprintf *)ctx;

	sh_fprintf->buffer[sh_fprintf->ctrl_blk->buffer_cnt] = (u8_t)c;
	sh_fprintf->ctrl_blk->buffer_cnt++;

	if (sh_fprintf->ctrl_blk->buffer_cnt == sh_fprintf->buffer_size) {
		shell_fprintf_buffer_flush(sh_fprintf);
	}

	return 0;
}

void shell_fprintf_fmt(const struct shell_fprintf *sh_fprintf,
		       const char *fmt, va_list args)
{
#ifndef CONFIG_NEWLIB_LIBC
	(void)_prf(out_func, (void *)sh_fprintf, (char *)fmt, args);
#else
	_vprintk(out_func, (void *)sh_fprintf, fmt, args);
#endif

	if (sh_fprintf->ctrl_blk->autoflush) {
		shell_fprintf_buffer_flush(sh_fprintf);
	}
}


void shell_fprintf_buffer_flush(const struct shell_fprintf *sh_fprintf)
{
	sh_fprintf->fwrite(sh_fprintf->user_ctx, sh_fprintf->buffer,
			   sh_fprintf->ctrl_blk->buffer_cnt);
	sh_fprintf->ctrl_blk->buffer_cnt = 0;
}
