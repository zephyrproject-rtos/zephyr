/*
 * Copyright (c) 2010, 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2021 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/cbprintf.h>
#include <sys/types.h>
#include <sys/util.h>

#ifdef CONFIG_CBPRINTF_FULL_INTEGRAL
typedef intmax_t int_value_type;
typedef uintmax_t uint_value_type;
#define DIGITS_BUFLEN 21
BUILD_ASSERT(sizeof(uint_value_type) <= 8U,
	     "DIGITS_BUFLEN may be incorrect");
#else
typedef int32_t int_value_type;
typedef uint32_t uint_value_type;
#define DIGITS_BUFLEN 10
#endif

#define ALPHA(fmt) (((fmt) & 0x60) - '0' - 10 + 1)

/* Convert value to string, storing characters downwards */
static inline int convert_value(uint_value_type num, unsigned int base,
				unsigned int alpha, char *buftop)
{
	int i = 0;

	do {
		unsigned int c = num % base;
		if (c >= 10) {
			c += alpha;
		}
		buftop[--i] = c + '0';
		num /= base;
	} while (num);

	return -i;
}

#define OUTC(_c) do { \
	out((int)(_c), ctx); \
	if (IS_ENABLED(CONFIG_CBPRINTF_LIBC_SUBSTS)) { \
		++count; \
	} \
} while (false)

#define PAD_ZERO	BIT(0)
#define PAD_TAIL	BIT(1)

int z_cbprintf(cbprintf_cb out, void *ctx, const char *fmt,
		union z_cbprintf_args *args, bool use_valist)
{
	size_t count = 0;
	char buf[DIGITS_BUFLEN];
	char *prefix, *data;
	int min_width, precision, data_len;
	char padding_mode, length_mod, special;

	/* we pre-increment in the loop  afterwards */
	fmt--;

	if (!IS_ENABLED(CONFIG_CBPRINTF_PACKAGE_PACKED) && !use_valist) {
		return -ENOTSUP;
	}

start:
	while (*++fmt != '%') {
		if (*fmt == '\0') {
			return count;
		}
		OUTC(*fmt);
	}

	min_width = -1;
	precision = -1;
	prefix = "";
	padding_mode = 0;
	length_mod = 0;
	special = 0;

	for (fmt++ ; ; fmt++) {
		switch (*fmt) {
		case 0:
			return count;

		case '%':
			OUTC('%');
			goto start;

		case '-':
			padding_mode = PAD_TAIL;
			continue;

		case '.':
			precision = 0;
			padding_mode &= (char)~PAD_ZERO;
			continue;

		case '0':
			if (min_width < 0 && precision < 0 && !padding_mode) {
				padding_mode = PAD_ZERO;
				continue;
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
			if (precision >= 0) {
				precision = 10 * precision + *fmt - '0';
			} else {
				if (min_width < 0) {
					min_width = 0;
				}
				min_width = 10 * min_width + *fmt - '0';
			}
			continue;

		case '*':
			if (precision >= 0) {
				Z_CBPRINTF_GET_ARG(precision, args, use_valist,
						   int);
			} else {
				Z_CBPRINTF_GET_ARG(min_width, args, use_valist,
						   int);
				if (min_width < 0) {
					min_width = -min_width;
					padding_mode = PAD_TAIL;
				}
			}
			continue;

		case '+':
		case ' ':
		case '#':
			special = *fmt;
			continue;

		case 'h':
		case 'l':
		case 'z':

			if (*fmt == 'h' && length_mod == 'h') {
				length_mod = 'H';
			} else if (*fmt == 'l' && length_mod == 'l') {
				length_mod = 'L';
			} else if (length_mod == '\0') {
				length_mod = *fmt;
			} else {
				OUTC('%');
				OUTC(*fmt);
				goto start;
			}
			continue;

		case 'd':
		case 'i':
		case 'u': {
			uint_value_type d;

			if (length_mod == 'z') {
				Z_CBPRINTF_GET_ARG(d, args, use_valist, ssize_t);
			} else if (length_mod == 'l') {
				Z_CBPRINTF_GET_ARG(d, args, use_valist, long);
			} else if (length_mod == 'L') {
				long long lld;

				Z_CBPRINTF_GET_ARG(lld, args, use_valist, long long);

				if (sizeof(int_value_type) < 8U &&
				    lld != (int_value_type) lld) {
					data = "ERR";
					data_len = 3;
					precision = 0;
					break;
				}
				d = (uint_value_type) lld;
			} else if (*fmt == 'u') {
				Z_CBPRINTF_GET_ARG(d, args, use_valist, unsigned int);
			} else {
				Z_CBPRINTF_GET_ARG(d, args, use_valist, int);
			}

			if (*fmt != 'u' && (int_value_type)d < 0) {
				d = -d;
				prefix = "-";
				min_width--;
			} else if (special == ' ') {
				prefix = " ";
				min_width--;
			} else if (special == '+') {
				prefix = "+";
				min_width--;
			} else {
				;
			}
			data_len = convert_value(d, 10, 0, buf + sizeof(buf));
			data = buf + sizeof(buf) - data_len;
			break;
		}

		case 'p':
		case 'x':
		case 'X': {
			uint_value_type x;

			if (*fmt == 'p') {
				Z_CBPRINTF_GET_ARG_CAST(x, args, use_valist,
							void *, uintptr_t);
				if (x == (uint_value_type)0) {
					data = "(nil)";
					data_len = 5;
					precision = 0;
					break;
				}
				special = '#';
			} else if (length_mod == 'l') {
				Z_CBPRINTF_GET_ARG(x, args, use_valist,
						   unsigned long);
			} else if (length_mod == 'L') {
				Z_CBPRINTF_GET_ARG(x, args, use_valist,
						   unsigned long long);
			} else {
				Z_CBPRINTF_GET_ARG(x, args, use_valist,
						   unsigned int);
			}
			if (special == '#') {
				prefix = (*fmt & 0x20) ? "0x" : "0X";
				min_width -= 2;
			}
			data_len = convert_value(x, 16, ALPHA(*fmt),
						 buf + sizeof(buf));
			data = buf + sizeof(buf) - data_len;
			break;
		}

		case 's': {
			Z_CBPRINTF_GET_ARG(data, args, use_valist, char *);
			data_len = strlen(data);
			if (precision >= 0 && data_len > precision) {
				data_len = precision;
			}
			precision = 0;
			break;
		}

		case 'c': {
			int c;

			Z_CBPRINTF_GET_ARG(c, args, use_valist, int);

			buf[0] = c;
			data = buf;
			data_len = 1;
			precision = 0;
			break;
		}

		default:
			OUTC('%');
			OUTC(*fmt);
			goto start;
		}

		if (precision < 0 && (padding_mode & PAD_ZERO)) {
			precision = min_width;
		}
		min_width -= data_len;
		precision -= data_len;
		if (precision > 0) {
			min_width -= precision;
		}

		if (!(padding_mode & PAD_TAIL)) {
			while (--min_width >= 0) {
				OUTC(' ');
			}
		}
		while (*prefix) {
			OUTC(*prefix++);
		}
		while (--precision >= 0) {
			OUTC('0');
		}
		while (--data_len >= 0) {
			OUTC(*data++);
		}
		while (--min_width >= 0) {
			OUTC(' ');
		}

		goto start;
	}
}
