/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell_fprintf.h>
#include <shell/shell.h>

extern int z_prf(int (*func)(), void *dest, char *format, va_list vargs);

static int out_func(int c, void *ctx)
{
	const struct shell_fprintf *sh_fprintf;
	const struct shell *shell;

	sh_fprintf = (const struct shell_fprintf *)ctx;
	shell = (const struct shell *)sh_fprintf->user_ctx;

	if ((shell->shell_flag == SHELL_FLAG_OLF_CRLF) && (c == '\n')) {
		(void)out_func('\r', ctx);
	}

	sh_fprintf->buffer[sh_fprintf->ctrl_blk->buffer_cnt] = (uint8_t)c;
	sh_fprintf->ctrl_blk->buffer_cnt++;

	if (sh_fprintf->ctrl_blk->buffer_cnt == sh_fprintf->buffer_size) {
		shell_fprintf_buffer_flush(sh_fprintf);
	}

	return 0;
}

void shell_fprintf_fmt(const struct shell_fprintf *sh_fprintf,
		       const char *fmt, va_list args)
{
	(void)z_prf(out_func, (void *)sh_fprintf, (char *)fmt, args);

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
