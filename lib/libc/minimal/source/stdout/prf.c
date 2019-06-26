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
#include <sys/types.h>
#include <sys/util.h>

#ifndef MAXFLD
#define	MAXFLD	200
#endif

#ifndef EOF
#define EOF  -1
#endif

#ifdef CONFIG_MINIMAL_LIBC_LL_PRINTF
#define VALTYPE long long
#define SIZEOF_VALTYPE __SIZEOF_LONG_LONG__
#else
#define VALTYPE long
#define SIZEOF_VALTYPE __SIZEOF_LONG__
#endif

/* this has to fit max range octal display */
#if MAXFLD < (1 + (SIZEOF_VALTYPE*8 + 2)/3)
#error buffer size MAXFLD is too small
#endif

static void _uc(char *buf)
{
	for (/**/; *buf; buf++) {
		if (*buf >= 'a' && *buf <= 'z') {
			*buf += 'A' - 'a';
		}
	}
}

/* Convention note: "end" as passed in is the standard "byte after
 * last character" style, but...
 */
static int _reverse_and_pad(char *start, char *end, int minlen)
{
	int len;

	while (end - start < minlen) {
		*end++ = '0';
	}

	*end = 0;
	len = end - start;
	for (end--; end > start; end--, start++) {
		char tmp = *end;
		*end = *start;
		*start = tmp;
	}
	return len;
}

/* Writes the specified number into the buffer in the given base,
 * using the digit characters 0-9a-z (i.e. base>36 will start writing
 * odd bytes), padding with leading zeros up to the minimum length.
 */
static int _to_x(char *buf, unsigned VALTYPE n, unsigned int base, int minlen)
{
	char *buf0 = buf;

	do {
		unsigned int d = n % base;

		n /= base;
		*buf++ = '0' + d + (d > 9 ? ('a' - '0' - 10) : 0);
	} while (n);
	return _reverse_and_pad(buf0, buf, minlen);
}

static int _to_hex(char *buf, unsigned VALTYPE value,
		   int alt_form, int precision, int prefix)
{
	int len;
	char *buf0 = buf;

	if (alt_form) {
		*buf++ = '0';
		*buf++ = 'x';
	}

	len = _to_x(buf, value, 16, precision);
	if (prefix == 'X') {
		_uc(buf0);
	}

	return len + (buf - buf0);
}

static int _to_octal(char *buf, unsigned VALTYPE value,
		     int alt_form, int precision)
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
	return (buf - buf0) + _to_x(buf, value, 8, precision);
}

static int _to_udec(char *buf, unsigned VALTYPE value, int precision)
{
	return _to_x(buf, value, 10, precision);
}

static int _to_dec(char *buf, VALTYPE value, int fplus, int fspace, int precision)
{
	char *start = buf;

	if (value < 0) {
		*buf++ = '-';
		value = -value;
	} else if (fplus) {
		*buf++ = '+';
	} else if (fspace) {
		*buf++ = ' ';
	} else {
		/* unreachable */
	}

	return (buf + _to_udec(buf, value, precision)) - start;
}

static	void _rlrshift(uint64_t *v)
{
	*v = (*v & 1) + (*v >> 1);
}

/* Tiny integer divide-by-five routine.  The full 64 bit division
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
	uint32_t i, hi;
	uint64_t rem = *v, quot = 0U, q;
	static const char shifts[] = { 32, 3, 0 };

	/* Usage in this file wants rounded behavior, not truncation.  So add
	 * two to get the threshold right.
	 */
	rem += 2U;

	for (i = 0U; i < 3; i++) {
		hi = rem >> shifts[i];
		q = (uint64_t)(hi / 5U) << shifts[i];
		rem -= q * 5U;
		quot += q;
	}

	*v = quot;
}

static	char _get_digit(uint64_t *fr, int *digit_count)
{
	int		rval;

	if (*digit_count > 0) {
		*digit_count -= 1;
		*fr = *fr * 10U;
		rval = ((*fr >> 60) & 0xF) + '0';
		*fr &= 0x0FFFFFFFFFFFFFFFull;
	} else {
		rval = '0';
	}

	return (char) (rval);
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
 */

/*
 *	The following two constants define the simulated binary floating
 *	point limit for the first stage of the conversion (fraction times
 *	power of two becomes fraction times power of 10), and the second
 *	stage (pulling the resulting decimal digits outs).
 */

#define	MAXFP1	0xFFFFFFFF	/* Largest # if first fp format */
#define HIGHBIT64 (1ull<<63)

static int _to_float(char *buf, uint64_t double_temp, int c,
					 int falt, int fplus, int fspace, int precision)
{
	register int    decexp;
	register int    exp;
	int             sign;
	int             digit_count;
	uint64_t        fract;
	uint64_t        ltemp;
	int             prune_zero;
	char           *start = buf;

	exp = double_temp >> 52 & 0x7ff;
	fract = (double_temp << 11) & ~HIGHBIT64;
	sign = !!(double_temp & HIGHBIT64);


	if (exp == 0x7ff) {
		if (sign) {
			*buf++ = '-';
		}
		if (!fract) {
			if (isupper(c)) {
				*buf++ = 'I';
				*buf++ = 'N';
				*buf++ = 'F';
			} else {
				*buf++ = 'i';
				*buf++ = 'n';
				*buf++ = 'f';
			}
		} else {
			if (isupper(c)) {
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
		exp -= (1023 - 1);	/* +1 since .1 vs 1. */
		fract |= HIGHBIT64;
		decexp = true;		/* Wasn't zero */
	} else {
		decexp = false;		/* It was zero */
	}

	if (decexp && sign) {
		*buf++ = '-';
	} else if (fplus) {
		*buf++ = '+';
	} else if (fspace) {
		*buf++ = ' ';
	} else {
		/* unreachable */
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
		if (!falt && (precision > 0)) {
			prune_zero = true;
		}
		if ((decexp < (-4 + 1)) || (decexp > (precision + 1))) {
			if (c == 'g') {
				c = 'e';
			} else {
				c = 'E';
			}
		} else {
			c = 'f';
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
			while (decexp > 0) {
				*buf++ = _get_digit(&fract, &digit_count);
				decexp--;
			}
		} else {
			*buf++ = '0';
		}
		if (falt || (precision > 0)) {
			*buf++ = '.';
		}
		while (precision-- > 0) {
			if (decexp < 0) {
				*buf++ = '0';
				decexp++;
			} else {
				*buf++ = _get_digit(&fract, &digit_count);
			}
		}
	} else {
		*buf = _get_digit(&fract, &digit_count);
		if (*buf++ != '0') {
			decexp--;
		}
		if (falt || (precision > 0)) {
			*buf++ = '.';
		}
		while (precision-- > 0) {
			*buf++ = _get_digit(&fract, &digit_count);
		}
	}

	if (prune_zero) {
		while (*--buf == '0') {
			;
		}

		if (*buf != '.') {
			buf++;
		}
	}

	if ((c == 'e') || (c == 'E')) {
		*buf++ = (char) c;
		if (decexp < 0) {
			decexp = -decexp;
			*buf++ = '-';
		} else {
			*buf++ = '+';
		}
		*buf++ = (char) ((decexp / 10) + '0');
		decexp %= 10;
		*buf++ = (char) (decexp + '0');
	}
	*buf = 0;

	return buf - start;
}

static int _atoi(char **sptr)
{
	register char *p;
	register int   i;

	i = 0;
	p = *sptr;
	p--;
	while (isdigit(((int) *p))) {
		i = 10 * i + *p++ - '0';
	}
	*sptr = p;
	return i;
}

int z_prf(int (*func)(), void *dest, char *format, va_list vargs)
{
	/*
	 * Due the fact that buffer is passed to functions in this file,
	 * they assume that its size is MAXFLD + 1. In need of change
	 * the buffer size, either MAXFLD should be changed or the change
	 * has to be propagated across the file
	 */
	char			buf[MAXFLD + 1];
	register int	c;
	int				count;
	register char	*cptr;
	int				falt;
	int				fminus;
	int				fplus;
	int				fspace;
	register int	i;
	int				need_justifying;
	char			pad;
	int				precision;
	int				prefix;
	int				width;
	VALTYPE			val;
	char			*cptr_temp;
	uint64_t			double_temp;

	count = 0;

	while ((c = *format++)) {
		if (c != '%') {
			if ((*func) (c, dest) == EOF) {
				return EOF;
			}

			count++;

		} else {
			fminus = fplus = fspace = falt = false;
			pad = ' ';		/* Default pad character    */
			precision = -1;	/* No precision specified   */

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
					pad = '0';
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
			} else if (!isdigit(c)) {
				width = 0;
			} else {
				width = _atoi(&format);	/* Find width */
				c = *format++;
			}

			/*
			 * If <width> is INT_MIN, then its absolute value can
			 * not be expressed as a positive number using 32-bit
			 * two's complement.  To cover that case, cast it to
			 * an unsigned before comparing it against MAXFLD.
			 */
			if ((unsigned) width > MAXFLD) {
				width = MAXFLD;
			}

			if (c == '.') {
				c = *format++;
				if (c == '*') {
					precision = va_arg(vargs, int);
				} else {
					precision = _atoi(&format);
				}

				if (precision > MAXFLD) {
					precision = -1;
				}

				c = *format++;
			}

			/*
			 * This implementation only supports the following
			 * length modifiers:
			 *    h: short
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
				}
			}

			need_justifying = false;
			prefix = 0;
			switch (c) {
			case 'c':
				buf[0] = va_arg(vargs, int);
				buf[1] = '\0';
				need_justifying = true;
				c = 1;
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
				default:
					val = va_arg(vargs, int);
					break;
				}
				c = _to_dec(buf, val, fplus, fspace, precision);
				if (fplus || fspace || val < 0) {
					prefix = 1;
				}
				need_justifying = true;
				if (precision != -1) {
					pad = ' ';
				}
				break;

			case 'e':
			case 'E':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
				/* standard platforms which supports double */
			{
#ifdef CONFIG_X86_64
				/* Can't use a double here because
				 * we're operating in -mno-sse and
				 * va_arg() will expect this to be a
				 * register argument.
				 */
				double_temp = va_arg(vargs, uint64_t);
#else
				union {
					double d;
					uint64_t i;
				} u;

				u.d = (double) va_arg(vargs, double);
				double_temp = u.i;
#endif
			}

				c = _to_float(buf, double_temp, c, falt, fplus,
					      fspace, precision);
				if (fplus || fspace || (buf[0] == '-')) {
					prefix = 1;
				}
				need_justifying = true;
				break;

			case 'n':
				switch (i) {
				case 'h':
					*va_arg(vargs, short *) = count;
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
				break;

			case 'p':
				val = (uintptr_t) va_arg(vargs, void *);
				c = _to_hex(buf, val, true, 2*sizeof(void *), 'x');
				need_justifying = true;
				if (precision != -1) {
					pad = ' ';
				}
				break;

			case 's':
				cptr_temp = va_arg(vargs, char *);
				/* Get the string length */
				for (c = 0; c < MAXFLD; c++) {
					if (cptr_temp[c] == '\0') {
						break;
					}
				}
				if ((precision >= 0) && (precision < c)) {
					c = precision;
				}
				if (c > 0) {
					memcpy(buf, cptr_temp, (size_t) c);
					need_justifying = true;
				}
				break;

				need_justifying = true;
				if (precision != -1) {
					pad = ' ';
				}
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
				default:
					val = va_arg(vargs, unsigned int);
					break;
				}
				if (c == 'o') {
					c = _to_octal(buf, val, falt, precision);
				} else if (c == 'u') {
					c = _to_udec(buf, val, precision);
				} else {
					c = _to_hex(buf, val, falt, precision, c);
					if (falt) {
						prefix = 2;
					}
				}
				need_justifying = true;
				if (precision != -1) {
					pad = ' ';
				}
				break;

			case '%':
				if ((*func)('%', dest) == EOF) {
					return EOF;
				}

				count++;
				break;

			case 0:
				return count;
			}

			if (c >= MAXFLD + 1) {
				return EOF;
			}

			if (need_justifying) {
				if (c < width) {
					if (fminus) {
						/* Left justify? */
						for (i = c; i < width; i++) {
							buf[i] = ' ';
						}
					} else {
						/* Right justify */
						(void) memmove((buf + (width - c)), buf, (size_t) (c
										+ 1));
						if (pad == ' ') {
							prefix = 0;
						}

						c = width - c + prefix;
						for (i = prefix; i < c; i++) {
							buf[i] = pad;
						}
					}
					c = width;
				}

				for (cptr = buf; c > 0; c--, cptr++, count++) {
					if ((*func)(*cptr, dest) == EOF) {
						return EOF;
					}
				}
			}
		}
	}
	return count;
}
