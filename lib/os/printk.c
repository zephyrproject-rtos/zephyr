/*
 * Copyright (c) 2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Low-level debug output
 *
 * Low-level debugging output. Platform installs a character output routine at
 * init time. If no routine is installed, a nop routine is called.
 */

#include <kernel.h>
#include <sys/printk.h>
#include <stdarg.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <syscall_handler.h>
#include <logging/log.h>
#include <sys/cbprintf.h>
#include <sys/types.h>

#ifdef CONFIG_PRINTK_SYNC
static struct k_spinlock lock;
#endif

#ifdef CONFIG_PRINTK
/**
 * @brief Default character output routine that does nothing
 * @param c Character to swallow
 *
 * Note this is defined as a weak symbol, allowing architecture code
 * to override it where possible to enable very early logging.
 *
 * @return 0
 */
/* LCOV_EXCL_START */
__attribute__((weak)) int arch_printk_char_out(int c)
{
	ARG_UNUSED(c);

	/* do nothing */
	return 0;
}
/* LCOV_EXCL_STOP */

int (*_char_out)(int) = arch_printk_char_out;

/**
 * @brief Install the character output routine for printk
 *
 * To be called by the platform's console driver at init time. Installs a
 * routine that outputs one ASCII character at a time.
 * @param fn putc routine to install
 *
 * @return N/A
 */
void __printk_hook_install(int (*fn)(int))
{
	_char_out = fn;
}

/**
 * @brief Get the current character output routine for printk
 *
 * To be called by any console driver that would like to save
 * current hook - if any - for later re-installation.
 *
 * @return a function pointer or NULL if no hook is set
 */
void *__printk_get_hook(void)
{
	return _char_out;
}
#endif /* CONFIG_PRINTK */


#ifdef CONFIG_PRINTK
#ifdef CONFIG_USERSPACE
struct buf_out_context {
	int count;
	unsigned int buf_count;
	char buf[CONFIG_PRINTK_BUFFER_SIZE];
};

static void buf_flush(struct buf_out_context *ctx)
{
	k_str_out(ctx->buf, ctx->buf_count);
	ctx->buf_count = 0U;
}

static int buf_char_out(int c, void *ctx_p)
{
	struct buf_out_context *ctx = ctx_p;

	ctx->count++;
	ctx->buf[ctx->buf_count++] = c;
	if (ctx->buf_count == CONFIG_PRINTK_BUFFER_SIZE) {
		buf_flush(ctx);
	}

	return c;
}
#endif /* CONFIG_USERSPACE */

struct out_context {
	int count;
};

static int char_out(int c, void *ctx_p)
{
	struct out_context *ctx = ctx_p;

	ctx->count++;
	return _char_out(c);
}

#ifdef CONFIG_USERSPACE
void vprintk(const char *fmt, va_list ap)
{
	if (_is_user_context()) {
		struct buf_out_context ctx = { 0 };

		cbvprintf(buf_char_out, &ctx, fmt, ap);

		if (ctx.buf_count) {
			buf_flush(&ctx);
		}
	} else {
		struct out_context ctx = { 0 };
#ifdef CONFIG_PRINTK_SYNC
		k_spinlock_key_t key = k_spin_lock(&lock);
#endif

		cbvprintf(char_out, &ctx, fmt, ap);

#ifdef CONFIG_PRINTK_SYNC
		k_spin_unlock(&lock, key);
#endif
	}
}
#else
void vprintk(const char *fmt, va_list ap)
{
	struct out_context ctx = { 0 };
#ifdef CONFIG_PRINTK_SYNC
	k_spinlock_key_t key = k_spin_lock(&lock);
#endif

	cbvprintf(char_out, &ctx, fmt, ap);

#ifdef CONFIG_PRINTK_SYNC
	k_spin_unlock(&lock, key);
#endif
}
#endif /* CONFIG_USERSPACE */

void z_impl_k_str_out(char *c, size_t n)
{
	size_t i;
#ifdef CONFIG_PRINTK_SYNC
	k_spinlock_key_t key = k_spin_lock(&lock);
#endif

	for (i = 0; i < n; i++) {
		_char_out(c[i]);
	}

#ifdef CONFIG_PRINTK_SYNC
	k_spin_unlock(&lock, key);
#endif
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_str_out(char *c, size_t n)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(c, n));
	z_impl_k_str_out((char *)c, n);
}
#include <syscalls/k_str_out_mrsh.c>
#endif /* CONFIG_USERSPACE */

/**
 * @brief Output a string
 *
 * Output a string on output installed by platform at init time. Some
 * printf-like formatting is available.
 *
 * Available formatting:
 * - %x/%X:  outputs a number in hexadecimal format
 * - %s:     outputs a null-terminated string
 * - %p:     pointer, same as %x with a 0x prefix
 * - %u:     outputs a number in unsigned decimal format
 * - %d/%i:  outputs a number in signed decimal format
 *
 * Field width (with or without leading zeroes) is supported.
 * Length attributes h, hh, l, ll and z are supported. However, integral
 * values with %lld and %lli are only printed if they fit in a long
 * otherwise 'ERR' is printed. Full 64-bit values may be printed with %llx.
 *
 * @param fmt formatted string to output
 *
 * @return N/A
 */
void printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	if (IS_ENABLED(CONFIG_LOG_PRINTK)) {
		log_printk(fmt, ap);
	} else {
		vprintk(fmt, ap);
	}
	va_end(ap);
}
#endif /* CONFIG_PRINTK */

struct str_context {
	char *str;
	int max;
	int count;
};

static int str_out(int c, struct str_context *ctx)
{
	if (ctx->str == NULL || ctx->count >= ctx->max) {
		ctx->count++;
		return c;
	}

	if (ctx->count == ctx->max - 1) {
		ctx->str[ctx->count++] = '\0';
	} else {
		ctx->str[ctx->count++] = c;
	}

	return c;
}

int snprintk(char *str, size_t size, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsnprintk(str, size, fmt, ap);
	va_end(ap);

	return ret;
}

int vsnprintk(char *str, size_t size, const char *fmt, va_list ap)
{
	struct str_context ctx = { str, size, 0 };

	cbvprintf(str_out, &ctx, fmt, ap);

	if (ctx.count < ctx.max) {
		str[ctx.count] = '\0';
	}

	return ctx.count;
}
