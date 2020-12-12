/*
 * Copyright (c) 2010, 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <sys/cbprintf.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <syscall_handler.h>
#include <logging/log.h>
#include <sys/types.h>

enum pad_type {
	PAD_NONE,
	PAD_ZERO_BEFORE,
	PAD_SPACE_BEFORE,
	PAD_SPACE_AFTER,
};

#ifdef CONFIG_CBPRINTF_FULL_INTEGRAL
typedef intmax_t int_value_type;
typedef uintmax_t uint_value_type;
#else
typedef int32_t int_value_type;
typedef uint32_t uint_value_type;
#endif

/* Maximum number of digits in a printed decimal value (hex is always
 * less, obviously).  Funny formula produces 10 max digits for 32 bit,
 * 21 for 64.  It may be incorrect for other value lengths.
 */
#define DIGITS_BUFLEN (11U * (sizeof(uint_value_type) / 4U) - 1U)

BUILD_ASSERT(sizeof(uint_value_type) <= 8U,
	     "DIGITS_BUFLEN formula may be incorrect");

#define OUTC(_c) do { \
	out((int)(_c), ctx); \
	if (IS_ENABLED(CONFIG_CBPRINTF_LIBC_SUBSTS)) { \
		++count; \
	} \
} while (false)

static void print_digits(cbprintf_cb out, void *ctx, uint_value_type num,
			 unsigned int base, bool pad_before, char pad_char,
			 int min_width, size_t *countp)
{
	size_t count = 0;
	char buf[DIGITS_BUFLEN];
	int i = 0;

	/* Print it backwards, low digits first */
	do {
		char c = num % base;
		if (c >= 10) {
			c += 'a' - '0' - 10;
		}
		buf[i++] = c + '0';
		num /= base;
	} while (num);

	int pad = MAX(min_width - i, 0);

	for (/**/; pad > 0 && pad_before; pad--) {
		OUTC(pad_char);
	}
	do {
		OUTC(buf[--i]);
	} while (i > 0);
	for (/**/; pad > 0; pad--) {
		OUTC(pad_char);
	}

	*countp += count;
}

static void print_hex(cbprintf_cb out, void *ctx, uint_value_type num,
		      enum pad_type padding, int min_width, size_t *countp)
{
	print_digits(out, ctx, num, 16U, padding != PAD_SPACE_AFTER,
		     padding == PAD_ZERO_BEFORE ? '0' : ' ', min_width,
		     countp);
}

static void print_dec(cbprintf_cb out, void *ctx, uint_value_type num,
		      enum pad_type padding, int min_width, size_t *countp)
{
	print_digits(out, ctx, num, 10U, padding != PAD_SPACE_AFTER,
		     padding == PAD_ZERO_BEFORE ? '0' : ' ', min_width,
		     countp);
}

static bool ok64(cbprintf_cb out, void *ctx, long long val, size_t *countp)
{
	size_t count = 0;

	if (sizeof(int_value_type) < 8U && val != (int_value_type) val) {
		const char *cp = "ERR";
		do {
			OUTC(*cp++);
		} while (*cp);
		*countp += count;
		return false;
	}
	return true;
}

static bool negative(uint_value_type val)
{
	const uint_value_type hibit = ~(((uint_value_type) ~1) >> 1U);

	return (val & hibit) != 0U;
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
int cbvprintf(cbprintf_cb out, void *ctx, const char *fmt, va_list ap)
{
	size_t count = 0;
	int might_format = 0; /* 1 if encountered a '%' */
	enum pad_type padding = PAD_NONE;
	int padlen, min_width = -1;
	char length_mod = 0;

	/* fmt has already been adjusted if needed */
	while (*fmt) {
		if (!might_format) {
			if (*fmt != '%') {
				OUTC(*fmt);
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
					min_width = 0;
				}
				min_width = 10 * min_width + *fmt - '0';

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
					OUTC('%');
					OUTC(*fmt);
					break;
				}
				goto still_might_format;
			case 'd':
			case 'i':
			case 'u': {
				uint_value_type d;

				if (length_mod == 'z') {
					d = va_arg(ap, ssize_t);
				} else if (length_mod == 'l') {
					d = va_arg(ap, long);
				} else if (length_mod == 'L') {

					long long lld = va_arg(ap,
							       long long);
					if (!ok64(out, ctx, lld, &count)) {
						break;
					}
					d = (uint_value_type) lld;
				} else if (*fmt == 'u') {
					d = va_arg(ap, unsigned int);
				} else {
					d = va_arg(ap, int);
				}

				if (*fmt != 'u' && negative(d)) {
					OUTC('-');
					d = -d;
					min_width--;
				}
				print_dec(out, ctx, d, padding, min_width,
					  &count);
				break;
			}
			case 'p':
			case 'x':
			case 'X': {
				uint_value_type x;

				if (*fmt == 'p') {
					const char *cp;

					x = (uintptr_t)va_arg(ap, void *);
					if (x == 0) {
						cp = "(nil)";
					} else {
						cp = "0x";
					}

					do {
						OUTC(*cp++);
					} while (*cp);
					if (x == 0) {
						padlen = min_width - 5;
						goto pad_string;
					}
					min_width -= 2;
				} else if (length_mod == 'l') {
					x = va_arg(ap, unsigned long);
				} else if (length_mod == 'L') {
					x = va_arg(ap, unsigned long long);
				} else {
					x = va_arg(ap, unsigned int);
				}

				print_hex(out, ctx, x, padding, min_width,
					  &count);
				break;
			}
			case 's': {
				char *s = va_arg(ap, char *);
				char *start = s;

				while (*s) {
					OUTC(*s++);
				}
				padlen = min_width - (s - start);

pad_string:
				if (padding == PAD_SPACE_AFTER) {
					while (padlen-- > 0) {
						OUTC(' ');
					}
				}
				break;
			}
			case 'c': {
				int c = va_arg(ap, int);

				OUTC(c);
				break;
			}
			case '%': {
				OUTC('%');
				break;
			}
			default:
				OUTC('%');
				OUTC(*fmt);
				break;
			}
			might_format = 0;
		}
still_might_format:
		++fmt;
	}

	return count;
}
