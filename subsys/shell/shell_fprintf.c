/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell_fprintf.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/cbprintf.h>

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
		z_shell_fprintf_buffer_flush(sh_fprintf);
	}

	return 0;
}

void z_shell_fprintf_fmt(const struct shell_fprintf *sh_fprintf,
			 const char *fmt, va_list args)
{
	(void)cbvprintf(out_func, (void *)sh_fprintf, fmt, args);

	if (sh_fprintf->ctrl_blk->autoflush) {
		z_shell_fprintf_buffer_flush(sh_fprintf);
	}
}


void z_shell_fprintf_buffer_flush(const struct shell_fprintf *sh_fprintf)
{
	sh_fprintf->fwrite(sh_fprintf->user_ctx, sh_fprintf->buffer,
			   sh_fprintf->ctrl_blk->buffer_cnt);
	sh_fprintf->ctrl_blk->buffer_cnt = 0;
}
