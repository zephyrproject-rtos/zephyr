/* prf.c */

/*
 * Copyright (c) 1997-2010, 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/util.h>

#ifndef EOF
#define EOF  -1
#endif

#ifdef CONFIG_MINIMAL_LIBC_LL_PRINTF
#define VALTYPE long long
#else
#define VALTYPE long
#endif

static void _uc(char *buf)
{
	do {
		if (*buf >= 'a' && *buf <= 'z') {
			*buf += 'A' - 'a';
		}
	} while (*buf++);
}

/*
 * Writes the specified number into the buffer in the given base,
 * using the digit characters 0-9a-z (i.e. base>36 will start writing
 * odd bytes).
 */
static int _to_x(char *buf, unsigned VALTYPE n, unsigned int base)
{
	char *start = buf;
	int len;

	do {
		unsigned int d = n % base;

		n /= base;
		*buf++ = '0' + d + (d > 9 ? ('a' - '0' - 10) : 0);
	} while (n);

	*buf = 0;
	len = buf - start;

	for (buf--; buf > start; buf--, start++) {
		char tmp = *buf;
		*buf = *start;
		*start = tmp;
	}

	return len;
}

static int _to_hex(char *buf, unsigned VALTYPE value, bool alt_form, char prefix)
{
	int len;
	char *buf0 = buf;

	if (alt_form) {
		*buf++ = '0';
		*buf++ = 'x';
	}

	len = _to_x(buf, value, 16);
	if (prefix == 'X') {
		_uc(buf0);
	}

	return len + (buf - buf0);
}

static int _to_octal(char *buf, unsigned VALTYPE value, bool alt_form)
{
	char *buf0 = buf;

	if (alt_form) {
		*buf++ = '0';
		if (!value) {
			/* So we don't return "00" for a value == 0. */
			*buf++ = 0;
			return 1;
		}
	}
	return (buf - buf0) + _to_x(buf, value, 8);
}

static int _to_udec(char *buf, unsigned VALTYPE value)
{
	return _to_x(buf, value, 10);
}

static int _to_dec(char *buf, VALTYPE value, bool fplus, bool fspace)
{
	char *start = buf;

	if (value < 0) {
		*buf++ = '-';
		value = -value;
	} else if (fplus) {
		*buf++ = '+';
	} else if (fspace) {
		*buf++ = ' ';
	}

	return (buf + _to_udec(buf, value)) - start;
}

static	void _rlrshift(uint64_t *v)
{
	*v = (*v & 1) + (*v >> 1);
}

/*
 * Tiny integer divide-by-five routine.  The full 64 bit division
 * implementations in libgcc are very large on some architectures, and
 * currently nothing in Zephyr pulls it into the link.  So it makes
 * sense to define this much smaller special case here to avoid
 * including it just for printf.
 *
 * It works by iteratively dividing the most significant 32 bits of
 * the 64 bit value by 5.  This will leave a remainder of 0-4
 * (i.e. three significant bits), ensuring that the top 29 bits of the
 * remainder are zero for the next iteration.  Thus in the second
 * iteration only 35 significant bits remain, and in the third only
 * six.  This was tested exhaustively through the first ~10B values in
 * the input space, and for ~2e12 (4 hours runtime) random inputs
 * taken from the full 64 bit space.
 */
static void _ldiv5(uint64_t *v)
{
	uint32_t hi;
	uint64_t rem = *v, quot = 0U, q;
	int i;

	static const char shifts[] = { 32, 3, 0 };

	/*
	 * Usage in this file wants rounded behavior, not truncation.  So add
	 * two to get the threshold right.
	 */
	rem += 2U;

	for (i = 0; i < 3; i++) {
		hi = rem >> shifts[i];
		q = (uint64_t)(hi / 5U) << shifts[i];
		rem -= q * 5U;
		quot += q;
	}

	*v = quot;
}

static	char _get_digit(uint64_t *fr, int *digit_count)
{
	char rval;

	if (*digit_count > 0) {
		*digit_count -= 1;
		*fr = *fr * 10U;
		rval = ((*fr >> 60) & 0xF) + '0';
		*fr &= 0x0FFFFFFFFFFFFFFFull;
	} else {
		rval = '0';
	}

	return rval;
}

/*
 *	_to_float
 *
 *	Convert a floating point # to ASCII.
 *
 *	Parameters:
 *		"buf"		Buffer to write result into.
 *		"double_temp"	# to convert (either IEEE single or double).
 *		"c"		The conversion type (one of e,E,f,g,G).
 *		"falt"		TRUE if "#" conversion flag in effect.
 *		"fplus"		TRUE if "+" conversion flag in effect.
 *		"fspace"	TRUE if " " conversion flag in effect.
 *		"precision"	Desired precision (negative if undefined).
 *		"zeropad"	To store padding info to be inserted later
 */

/*
 *	The following two constants define the simulated binary floating
 *	point limit for the first stage of the conversion (fraction times
 *	power of two becomes fraction times power of 10), and the second
 *	stage (pulling the resulting decimal digits outs).
 */

#define	MAXFP1	0xFFFFFFFF	/* Largest # if first fp format */
#define HIGHBIT64 (1ull<<63)

struct zero_padding { int predot, postdot, trail; };

static int _to_float(char *buf, uint64_t double_temp, char c,
		     bool falt, bool fplus, bool fspace, int precision,
		     struct zero_padding *zp)
{
	int decexp;
	int exp;
	bool sign;
	int digit_count;
	uint64_t fract;
	uint64_t ltemp;
	bool prune_zero;
	char *start = buf;

	exp = double_temp >> 52 & 0x7ff;
	fract = (double_temp << 11) & ~HIGHBIT64;
	sign = !!(double_temp & HIGHBIT64);

	if (sign) {
		*buf++ = '-';
	} else if (fplus) {
		*buf++ = '+';
	} else if (fspace) {
		*buf++ = ' ';
	}

	if (exp == 0x7ff) {
		if (!fract) {
			if (isupper((int)c)) {
				*buf++ = 'I';
				*buf++ = 'N';
				*buf++ = 'F';
			} else {
				*buf++ = 'i';
				*buf++ = 'n';
				*buf++ = 'f';
			}
		} else {
			if (isupper((int)c)) {
				*buf++ = 'N';
				*buf++ = 'A';
				*buf++ = 'N';
			} else {
				*buf++ = 'n';
				*buf++ = 'a';
				*buf++ = 'n';
			}
		}
		*buf = 0;
		return buf - start;
	}

	if (c == 'F') {
		c = 'f';
	}

	if ((exp | fract) != 0) {
		if (exp == 0) {
			/* this is a denormal */
			while (((fract <<= 1) & HIGHBIT64) == 0) {
				exp--;
			}
		}
		exp -= (1023 - 1);	/* +1 since .1 vs 1. */
		fract |= HIGHBIT64;
	}

	decexp = 0;
	while (exp <= -3) {
		while ((fract >> 32) >= (MAXFP1 / 5)) {
			_rlrshift(&fract);
			exp++;
		}
		fract *= 5U;
		exp++;
		decexp--;

		while ((fract >> 32) <= (MAXFP1 / 2)) {
			fract <<= 1;
			exp--;
		}
	}

	while (exp > 0) {
		_ldiv5(&fract);
		exp--;
		decexp++;
		while ((fract >> 32) <= (MAXFP1 / 2)) {
			fract <<= 1;
			exp--;
		}
	}

	while (exp < (0 + 4)) {
		_rlrshift(&fract);
		exp++;
	}

	if (precision < 0) {
		precision = 6;		/* Default precision if none given */
	}

	prune_zero = false;		/* Assume trailing 0's allowed     */
	if ((c == 'g') || (c == 'G')) {
		if (decexp < (-4 + 1) || decexp > precision) {
			c += 'e' - 'g';
			if (precision > 0) {
				precision--;
			}
		} else {
			c = 'f';
			precision -= decexp;
		}
		if (!falt && (precision > 0)) {
			prune_zero = true;
		}
	}

	if (c == 'f') {
		exp = precision + decexp;
		if (exp < 0) {
			exp = 0;
		}
	} else {
		exp = precision + 1;
	}
	digit_count = 16;
	if (exp > 16) {
		exp = 16;
	}

	ltemp = 0x0800000000000000;
	while (exp--) {
		_ldiv5(&ltemp);
		_rlrshift(&ltemp);
	}

	fract += ltemp;
	if ((fract >> 32) & 0xF0000000) {
		_ldiv5(&fract);
		_rlrshift(&fract);
		decexp++;
	}

	if (c == 'f') {
		if (decexp > 0) {
			while (decexp > 0 && digit_count > 0) {
				*buf++ = _get_digit(&fract, &digit_count);
				decexp--;
			}
			zp->predot = decexp;
			decexp = 0;
		} else {
			*buf++ = '0';
		}
		if (falt || (precision > 0)) {
			*buf++ = '.';
		}
		if (decexp < 0 && precision > 0) {
			zp->postdot = -decexp;
			if (zp->postdot > precision) {
				zp->postdot = precision;
			}
			precision -= zp->postdot;
		}
		while (precision > 0 && digit_count > 0) {
			*buf++ = _get_digit(&fract, &digit_count);
			precision--;
		}
		zp->trail = precision;
	} else {
		*buf = _get_digit(&fract, &digit_count);
		if (*buf++ != '0') {
			decexp--;
		}
		if (falt || (precision > 0)) {
			*buf++ = '.';
		}
		while (precision > 0 && digit_count > 0) {
			*buf++ = _get_digit(&fract, &digit_count);
			precision--;
		}
		zp->trail = precision;
	}

	if (prune_zero) {
		zp->trail = 0;
		while (*--buf == '0')
			;
		if (*buf != '.') {
			buf++;
		}
	}

	if ((c == 'e') || (c == 'E')) {
		*buf++ = c;
		if (decexp < 0) {
			decexp = -decexp;
			*buf++ = '-';
		} else {
			*buf++ = '+';
		}
		if (decexp >= 100) {
			*buf++ = (decexp / 100) + '0';
			decexp %= 100;
		}
		*buf++ = (decexp / 10) + '0';
		decexp %= 10;
		*buf++ = decexp + '0';
	}
	*buf = 0;

	return buf - start;
}

static int _atoi(const char **sptr)
{
	const char *p = *sptr - 1;
	int i = 0;

	while (isdigit((int)*p)) {
		i = 10 * i + *p++ - '0';
	}
	*sptr = p;
	return i;
}

int z_prf(int (*func)(), void *dest, const char *format, va_list vargs)
{
	/*
	 * The work buffer has to accommodate for the largest data length.
	 * The max range octal length is one prefix + 3 bits per digit
	 * meaning 12 bytes on 32-bit and 23 bytes on 64-bit.
	 * The float code may extract up to 16 digits, plus a prefix,
	 * a leading 0, a dot, and an exponent in the form e+xxx for
	 * a total of 24. Add a trailing NULL so it is 25.
	 */
	char buf[25];
	char c;
	int count;
	char *cptr;
	bool falt, fminus, fplus, fspace, fzero;
	int i;
	int width, precision;
	int clen, prefix, zero_head;
	struct zero_padding zero;
	VALTYPE val;

#define PUTC(c)	do { if ((*func)(c, dest) == EOF) return EOF; } while (false)

	count = 0;

	while ((c = *format++)) {
		if (c != '%') {
			PUTC(c);
			count++;
		} else {
			fminus = fplus = fspace = falt = fzero = false;
			while (strchr("-+ #0", (c = *format++)) != NULL) {
				switch (c) {
				case '-':
					fminus = true;
					break;

				case '+':
					fplus = true;
					break;

				case ' ':
					fspace = true;
					break;

				case '#':
					falt = true;
					break;

				case '0':
					fzero = true;
					break;

				case '\0':
					return count;
				}
			}

			if (c == '*') {
				/* Is the width a parameter? */
				width = va_arg(vargs, int);
				if (width < 0) {
					fminus = true;
					width = -width;
				}
				c = *format++;
			} else if (!isdigit((int)c)) {
				width = 0;
			} else {
				width = _atoi(&format);	/* Find width */
				c = *format++;
			}

			precision = -1;
			if (c == '.') {
				c = *format++;
				if (c == '*') {
					precision = va_arg(vargs, int);
				} else {
					precision = _atoi(&format);
				}

				c = *format++;
			}

			/*
			 * This implementation only supports the following
			 * length modifiers:
			 *    h: short
			 *   hh: char
			 *    l: long
			 *   ll: long long
			 *    z: size_t or ssize_t
			 */
			i = 0;
			if (strchr("hlz", c) != NULL) {
				i = c;
				c = *format++;
				if (IS_ENABLED(CONFIG_MINIMAL_LIBC_LL_PRINTF) &&
				    i == 'l' && c == 'l') {
					i = 'L';
					c = *format++;
				} else if (i == 'h' && c == 'h') {
					i = 'H';
					c = *format++;
				}
			}

			cptr = buf;
			prefix = 0;
			zero.predot = zero.postdot = zero.trail = 0;

			switch (c) {
			case 'c':
				buf[0] = va_arg(vargs, int);
				clen = 1;
				precision = 0;
				break;

			case 'd':
			case 'i':
				switch (i) {
				case 'l':
					val = va_arg(vargs, long);
					break;
#ifdef CONFIG_MINIMAL_LIBC_LL_PRINTF
				case 'L':
					val = va_arg(vargs, long long);
					break;
#endif
				case 'z':
					val = va_arg(vargs, ssize_t);
					break;
				case 'h':
				case 'H':
				default:
					val = va_arg(vargs, int);
					break;
				}
				clen = _to_dec(buf, val, fplus, fspace);
				if (fplus || fspace || val < 0) {
					prefix = 1;
				}
				break;

			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			{
				uint64_t double_val;

				/* standard platforms which supports double */
				union {
					double d;
					uint64_t i;
				} u;

				u.d = va_arg(vargs, double);
				double_val = u.i;

				clen = _to_float(buf, double_val, c, falt,
						 fplus, fspace, precision,
						 &zero);
				if (fplus || fspace || (buf[0] == '-')) {
					prefix = 1;
				}
				clen += zero.predot + zero.postdot + zero.trail;
				if (!isdigit((int)buf[prefix])) {
					/* inf or nan: no zero padding */
					fzero = false;
				}
				precision = -1;
				break;
			}

			case 'n':
				switch (i) {
				case 'h':
					*va_arg(vargs, short *) = count;
					break;
				case 'H':
					*va_arg(vargs, char *) = count;
					break;
				case 'l':
					*va_arg(vargs, long *) = count;
					break;
#ifdef CONFIG_MINIMAL_LIBC_LL_PRINTF
				case 'L':
					*va_arg(vargs, long long *) = count;
					break;
#endif
				case 'z':
					*va_arg(vargs, ssize_t *) = count;
					break;
				default:
					*va_arg(vargs, int *) = count;
					break;
				}
				continue;

			case 'p':
				val = (uintptr_t) va_arg(vargs, void *);
				clen = _to_hex(buf, val, true, 'x');
				prefix = 2;
				break;

			case 's':
				cptr = va_arg(vargs, char *);
				/* Get the string length */
				if (precision < 0) {
					precision = INT_MAX;
				}
				for (clen = 0; clen < precision; clen++) {
					if (cptr[clen] == '\0') {
						break;
					}
				}
				precision = 0;
				break;

			case 'o':
			case 'u':
			case 'x':
			case 'X':
				switch (i) {
				case 'l':
					val = va_arg(vargs, unsigned long);
					break;
#ifdef CONFIG_MINIMAL_LIBC_LL_PRINTF
				case 'L':
					val = va_arg(vargs, unsigned long long);
					break;
#endif
				case 'z':
					val = va_arg(vargs, size_t);
					break;
				case 'h':
				case 'H':
				default:
					val = va_arg(vargs, unsigned int);
					break;
				}
				if (c == 'o') {
					clen = _to_octal(buf, val, falt);
				} else if (c == 'u') {
					clen = _to_udec(buf, val);
				} else {
					clen = _to_hex(buf, val, falt, c);
					if (falt) {
						prefix = 2;
					}
				}
				break;

			case '%':
				PUTC('%');
				count++;
				continue;

			default:
				PUTC('%');
				PUTC(c);
				count += 2;
				continue;

			case 0:
				return count;
			}

			if (precision >= 0) {
				zero_head = precision - clen + prefix;
			} else if (fzero) {
				zero_head = width - clen;
			} else {
				zero_head = 0;
			}
			if (zero_head < 0) {
				zero_head = 0;
			}
			width -= clen + zero_head;

			/* padding for right justification */
			if (!fminus && width > 0) {
				count += width;
				while (width-- > 0) {
					PUTC(' ');
				}
			}

			/* data prefix */
			clen -= prefix;
			count += prefix;
			while (prefix-- > 0) {
				PUTC(*cptr++);
			}

			/* zero-padded head */
			count += zero_head;
			while (zero_head-- > 0) {
				PUTC('0');
			}

			/*
			 * main data:
			 *
			 * In the case of floats, 3 possible zero-padding
			 * are included in the clen count, either with
			 *	xxxxxx<zero.predot>.<zero.postdot>
			 * or with
			 *	x.<zero.postdot>xxxxxx<zero.trail>[e+xx]
			 * In the non-float cases, those predot, postdot and
			 * tail params are equal to 0.
			 */
			count += clen;
			if (zero.predot) {
				c = *cptr;
				while (isdigit((int)c)) {
					PUTC(c);
					clen--;
					c = *++cptr;
				}
				clen -= zero.predot;
				while (zero.predot-- > 0) {
					PUTC('0');
				}
			}
			if (zero.postdot) {
				do {
					c = *cptr++;
					PUTC(c);
					clen--;
				} while (c != '.');
				clen -= zero.postdot;
				while (zero.postdot-- > 0) {
					PUTC('0');
				}
			}
			if (zero.trail) {
				c = *cptr;
				while (isdigit((int)c) || c == '.') {
					PUTC(c);
					clen--;
					c = *++cptr;
				}
				clen -= zero.trail;
				while (zero.trail-- > 0) {
					PUTC('0');
				}
			}
			while (clen-- > 0) {
				PUTC(*cptr++);
			}

			/* padding for left justification */
			if (width > 0) {
				count += width;
				while (width-- > 0) {
					PUTC(' ');
				}
			}
		}
	}
	return count;

#undef PUTC
}
