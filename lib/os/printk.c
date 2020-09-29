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
#include <sys/types.h>

typedef int (*out_func_t)(int c, void *ctx);

enum pad_type {
	PAD_NONE,
	PAD_ZERO_BEFORE,
	PAD_SPACE_BEFORE,
	PAD_SPACE_AFTER,
};

#ifdef CONFIG_PRINTK64
typedef uint64_t printk_val_t;
#else
typedef uint32_t printk_val_t;
#endif

/* Maximum number of digits in a printed decimal value (hex is always
 * less, obviously).  Funny formula produces 10 max digits for 32 bit,
 * 21 for 64.
 */
#define DIGITS_BUFLEN (11 * (sizeof(printk_val_t) / 4) - 1)

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

static void print_digits(out_func_t out, void *ctx, printk_val_t num, int base,
			 bool pad_before, char pad_char, int min_width)
{
	char buf[DIGITS_BUFLEN];
	int i;

	/* Print it backwards into the end of the buffer, low digits first */
	for (i = DIGITS_BUFLEN - 1; num != 0; i--) {
		buf[i] = "0123456789abcdef"[num % base];
		num /= base;
	}

	if (i == DIGITS_BUFLEN - 1) {
		buf[i] = '0';
	} else {
		i++;
	}

	int pad = MAX(min_width - (DIGITS_BUFLEN - i), 0);

	for (/**/; pad > 0 && pad_before; pad--) {
		out(pad_char, ctx);
	}
	for (/**/; i < DIGITS_BUFLEN; i++) {
		out(buf[i], ctx);
	}
	for (/**/; pad > 0; pad--) {
		out(pad_char, ctx);
	}
}

static void print_hex(out_func_t out, void *ctx, printk_val_t num,
		      enum pad_type padding, int min_width)
{
	print_digits(out, ctx, num, 16, padding != PAD_SPACE_AFTER,
		     padding == PAD_ZERO_BEFORE ? '0' : ' ', min_width);
}

static void print_dec(out_func_t out, void *ctx, printk_val_t num,
		      enum pad_type padding, int min_width)
{
	print_digits(out, ctx, num, 10, padding != PAD_SPACE_AFTER,
		     padding == PAD_ZERO_BEFORE ? '0' : ' ', min_width);
}

static bool ok64(out_func_t out, void *ctx, long long val)
{
	if (sizeof(printk_val_t) < 8 && val != (long) val) {
		out('E', ctx);
		out('R', ctx);
		out('R', ctx);
		return false;
	}
	return true;
}

static bool negative(printk_val_t val)
{
	const printk_val_t hibit = ~(((printk_val_t) ~1) >> 1);

	return (val & hibit) != 0;
}

/**
 * @brief Printk internals
 *
 * See printk() for description.
 * @param fmt Format string
 * @param ap Variable parameters
 *
 * @return N/A
 */
void z_vprintk(out_func_t out, void *ctx, const char *fmt, va_list ap)
{
	int might_format = 0; /* 1 if encountered a '%' */
	enum pad_type padding = PAD_NONE;
	int min_width = -1;
	char length_mod = 0;

	/* fmt has already been adjusted if needed */

	while (*fmt) {
		if (!might_format) {
			if (*fmt != '%') {
				out((int)*fmt, ctx);
			} else {
				might_format = 1;
				min_width = -1;
				padding = PAD_NONE;
				length_mod = 0;
			}
		} else {
			switch (*fmt) {
			case '-':
				padding = PAD_SPACE_AFTER;
				goto still_might_format;
			case '0':
				if (min_width < 0 && padding == PAD_NONE) {
					padding = PAD_ZERO_BEFORE;
					goto still_might_format;
				}
				__fallthrough;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (min_width < 0) {
					min_width = *fmt - '0';
				} else {
					min_width = 10 * min_width + *fmt - '0';
				}

				if (padding == PAD_NONE) {
					padding = PAD_SPACE_BEFORE;
				}
				goto still_might_format;
			case 'h':
			case 'l':
			case 'z':
				if (*fmt == 'h' && length_mod == 'h') {
					length_mod = 'H';
				} else if (*fmt == 'l' && length_mod == 'l') {
					length_mod = 'L';
				} else if (length_mod == 0) {
					length_mod = *fmt;
				} else {
					out((int)'%', ctx);
					out((int)*fmt, ctx);
					break;
				}
				goto still_might_format;
			case 'd':
			case 'i':
			case 'u': {
				printk_val_t d;

				if (length_mod == 'z') {
					d = va_arg(ap, ssize_t);
				} else if (length_mod == 'l') {
					d = va_arg(ap, long);
				} else if (length_mod == 'L') {
					long long lld = va_arg(ap, long long);
					if (!ok64(out, ctx, lld)) {
						break;
					}
					d = (printk_val_t) lld;
				} else if (*fmt == 'u') {
					d = va_arg(ap, unsigned int);
				} else {
					d = va_arg(ap, int);
				}

				if (*fmt != 'u' && negative(d)) {
					out((int)'-', ctx);
					d = -d;
					min_width--;
				}
				print_dec(out, ctx, d, padding, min_width);
				break;
			}
			case 'p':
				out('0', ctx);
				out('x', ctx);
				/* left-pad pointers with zeros */
				padding = PAD_ZERO_BEFORE;
				min_width = sizeof(void *) * 2;
				__fallthrough;
			case 'x':
			case 'X': {
				printk_val_t x;

				if (*fmt == 'p') {
					x = (uintptr_t)va_arg(ap, void *);
				} else if (length_mod == 'l') {
					x = va_arg(ap, unsigned long);
				} else if (length_mod == 'L') {
					x = va_arg(ap, unsigned long long);
				} else {
					x = va_arg(ap, unsigned int);
				}

				print_hex(out, ctx, x, padding, min_width);
				break;
			}
			case 's': {
				char *s = va_arg(ap, char *);
				char *start = s;

				while (*s) {
					out((int)(*s++), ctx);
				}

				if (padding == PAD_SPACE_AFTER) {
					int remaining = min_width - (s - start);
					while (remaining-- > 0) {
						out(' ', ctx);
					}
				}
				break;
			}
			case 'c': {
				int c = va_arg(ap, int);

				out(c, ctx);
				break;
			}
			case '%': {
				out((int)'%', ctx);
				break;
			}
			default:
				out((int)'%', ctx);
				out((int)*fmt, ctx);
				break;
			}
			might_format = 0;
		}
still_might_format:
		++fmt;
	}
}

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

		z_vprintk(buf_char_out, &ctx, fmt, ap);

		if (ctx.buf_count) {
			buf_flush(&ctx);
		}
	} else {
		struct out_context ctx = { 0 };
#ifdef CONFIG_PRINTK_SYNC
		k_spinlock_key_t key = k_spin_lock(&lock);
#endif

		z_vprintk(char_out, &ctx, fmt, ap);

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

	z_vprintk(char_out, &ctx, fmt, ap);

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

	z_vprintk((out_func_t)str_out, &ctx, fmt, ap);

	if (ctx.count < ctx.max) {
		str[ctx.count] = '\0';
	}

	return ctx.count;
}
