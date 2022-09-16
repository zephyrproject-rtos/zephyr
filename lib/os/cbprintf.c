/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stddef.h>
#include <zephyr/sys/cbprintf.h>

int cbprintf(cbprintf_cb out, void *ctx, const char *format, ...)
{
	va_list ap;
	int rc;

	va_start(ap, format);
	rc = cbvprintf(out, ctx, format, ap);
	va_end(ap);

	return rc;
}

#if defined(CONFIG_CBPRINTF_LIBC_SUBSTS)

#include <stdio.h>

/* Context for sn* variants is the next space in the buffer, and the buffer
 * end.
 */
struct str_ctx {
	char *dp;
	char *const dpe;
};

static int str_out(int c,
		   void *ctx)
{
	struct str_ctx *scp = ctx;

	/* s*printf must return the number of characters that would be
	 * output, even if they don't all fit, so conditionally store
	 * and unconditionally succeed.
	 */
	if (scp->dp < scp->dpe) {
		*(scp->dp++) = c;
	}

	return c;
}

int fprintfcb(FILE *stream, const char *format, ...)
{
	va_list ap;
	int rc;

	va_start(ap, format);
	rc = vfprintfcb(stream, format, ap);
	va_end(ap);

	return rc;
}

int vfprintfcb(FILE *stream, const char *format, va_list ap)
{
	return cbvprintf(fputc, stream, format, ap);
}

int printfcb(const char *format, ...)
{
	va_list ap;
	int rc;

	va_start(ap, format);
	rc = vprintfcb(format, ap);
	va_end(ap);

	return rc;
}

int vprintfcb(const char *format, va_list ap)
{
	return cbvprintf(fputc, stdout, format, ap);
}

int snprintfcb(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int rc;

	va_start(ap, format);
	rc = vsnprintfcb(str, size, format, ap);
	va_end(ap);

	return rc;
}

int vsnprintfcb(char *str, size_t size, const char *format, va_list ap)
{
	struct str_ctx ctx = {
		.dp = str,
		.dpe = str + size,
	};
	int rv = cbvprintf(str_out, &ctx, format, ap);

	if (ctx.dp < ctx.dpe) {
		ctx.dp[0] = 0;
	} else {
		ctx.dp[-1] = 0;
	}

	return rv;
}

#endif /* CONFIG_CBPRINTF_LIBC_SUBSTS */
