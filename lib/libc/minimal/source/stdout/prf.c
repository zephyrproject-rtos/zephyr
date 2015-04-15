/* prf.c */

/*
 * Copyright (c) 1997-2010, 2012-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#ifndef FALSE
#define	FALSE	0
#define	TRUE	1
#endif

#ifndef MAXFLD
#define	MAXFLD	200
#endif

#ifndef EOF
#define EOF  -1
#endif

#ifndef DOUBLE
#define DOUBLE 1
#endif

static int _to_hex(char *buf, uint32_t value, int alt_form, int precision, int prefix)
{
	register int	i;
	register int	temp;
	char *start = buf;

#if (MAXFLD < (2 + 7))
  #error buffer size MAXFLD is too small
#endif

	if (precision < 0)
		precision = 1;
	*buf = '\0';
	if (alt_form) {
		buf[0] = '0';
		buf[1] = (prefix == 'X') ? 'X' : 'x';
		buf += 2;
	}
	for (i = 7; i >= 0; i--) {
		temp = (value >> (i * 4)) & 0xF;
		if ((precision > i) || (temp != 0)) {
			precision = i;
			if (temp < 10)
				*buf++ = (char) (temp + '0');
			else {
				if (prefix == 'X')
					*buf++ = (char) (temp - 10 + 'A');
				else
					*buf++ = (char) (temp - 10 + 'a');
			}
		}
	}
	*buf = 0;

	return buf - start;
}

static int _to_octal(char *buf, uint32_t value, int alt_form, int precision)
{
	register int	i;
	register int	temp;
	char *start = buf;

#if (MAXFLD < 10)
  #error buffer size MAXFLD is too small
#endif

	if (precision < 0)
		precision = 1;
	*buf = '\0';
	for (i = 10; i >= 0; i--) {
		temp = (value >> (i * 3));
		if (i == 10)
			temp &= 0x3;
		else
			temp &= 0x7;
		if ((precision > i) || (temp != 0)) {
			precision = i;
			if ((temp != 0) && alt_form)
				*buf++ = '0';
			alt_form = FALSE;
			*buf++ = (char) (temp + '0');
		}
	}
	*buf = 0;

	return buf - start;
}

static int _to_udec(char *buf, uint32_t value, int precision)
{
	register uint32_t	divisor;
	register int	i;
	register int	temp;
	char *start = buf;

#if (MAXFLD < 9)
  #error buffer size MAXFLD is too small
#endif

	divisor = 1000000000;
	if (precision < 0)
		precision = 1;
	for (i = 9; i >= 0; i--, divisor /= 10) {
		temp = value / divisor;
		value = value % divisor;
		if ((precision > i) || (temp != 0)) {
			precision = i;
			*buf++ = (char) (temp + '0');
		}
	}
	*buf = 0;

	return buf - start;
}

static int _to_dec(char *buf, int32_t value, int fplus, int fspace, int precision)
{
	char *start = buf;

#if (MAXFLD < 10)
  #error buffer size MAXFLD is too small
#endif

	if (value < 0) {
		*buf++ = '-';
		if (value != 0x80000000)
			value = -value;
	} else if (fplus)
		*buf++ = '+';
	else if (fspace)
		*buf++ = ' ';

	return (buf + _to_udec(buf, (uint32_t) value, precision)) - start;
}

static void _llshift(uint32_t value[])
{
	if (value[0] & 0x80000000)
		value[1] = (value[1] << 1) | 1;
	else
		value[1] <<= 1;
	value[0] <<= 1;
}

static void _lrshift(uint32_t value[])
{
	if (value[1] & 1)
		value[0] = (value[0] >> 1) | 0x80000000;
	else
		value[0] = (value[0] >> 1) & 0x7FFFFFFF;
	value[1] = (value[1] >> 1) & 0x7FFFFFFF;
}

static void _ladd(uint32_t result[], uint32_t value[])
{
	uint32_t carry;
	uint32_t temp;

	carry = 0;
	temp = result[0] + value[0];
	if (result[0] & 0x80000000) {
		if ((value[0] & 0x80000000) || ((temp & 0x80000000) == 0))
			carry = 1;
	} else {
		if ((value[0] & 0x80000000) && ((temp & 0x80000000) == 0))
			carry = 1;
	}
	result[0] = temp;
	result[1] = result[1] + value[1] + carry;
}

static	void _rlrshift(uint32_t value[])
{
	uint32_t temp[2];

	temp[0] = value[0] & 1;
	temp[1] = 0;
	_lrshift(value);
	_ladd(value, temp);
}

 /*
    64 bit divide by 5 function for _to_float.
    The result is ROUNDED, not TRUNCATED.
  */

static	void _ldiv5(uint32_t value[])
{
	uint32_t      result[2];
	register int  shift;
	uint32_t      temp1[2];
	uint32_t      temp2[2];

	result[0] = 0;		/* Result accumulator */
	result[1] = value[1] / 5;
	temp1[0] = value[0];	/* Dividend for this pass */
	temp1[1] = value[1] % 5;
	temp2[1] = 0;

	while (1) {
		for (shift = 0; temp1[1] != 0; shift++)
			_lrshift(temp1);
		temp2[0] = temp1[0] / 5;
		if (temp2[0] == 0) {
			if (temp1[0] % 5 > (5 / 2)) {
				temp1[0] = 1;
				_ladd(result, temp1);
			}
			break;
		}
		temp1[0] = temp2[0];
		while (shift-- != 0)
			_llshift(temp1);
		_ladd(result, temp1);	/* Update result accumulator */
		temp1[0] = result[0];
		temp1[1] = result[1];
		_llshift(temp1);	/* Compute (current_result*5) */
		_llshift(temp1);
		_ladd(temp1, result);
		temp1[0] = ~temp1[0];	/* Compute -(current_result*5) */
		temp1[1] = ~temp1[1];
		temp2[0] = 1;
		_ladd(temp1, temp2);
		_ladd(temp1, value);	/* Compute #-(current_result*5) */
	}
	value[0] = result[0];
	value[1] = result[1];
}

static	char _get_digit(uint32_t fract[], int *digit_count)
{
	int		rval;
	uint32_t	temp[2];

	if (*digit_count > 0) {
		*digit_count -= 1;
		temp[0] = fract[0];
		temp[1] = fract[1];
		_llshift(fract);	/* Multiply by 10 */
		_llshift(fract);
		_ladd(fract, temp);
		_llshift(fract);
		rval = ((fract[1] >> 28) & 0xF) + '0';
		fract[1] &= 0x0FFFFFFF;
	} else
		rval = '0';
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
 *		"full"		TRUE if IEEE double, else IEEE single.
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
#define	MAXFP2	0x0FFFFFFF	/* Largest # in second fp format */

static int _to_float(char *buf, uint32_t double_temp[], int full, int c, int falt, int fplus, int fspace, int precision)
{
	register int    decexp;
	register int    exp;
	int             digit_count;
	uint32_t        fract[2];
	uint32_t        ltemp[2];
	int             prune_zero;
	char           *start = buf;

	if (full) {			/* IEEE double */
		exp = (double_temp[1] >> 20) & 0x7FF;
		fract[1] = (double_temp[1] << 11) & 0x7FFFF800;
		fract[1] |= ((double_temp[0] >> 21) & 0x000007FF);
		fract[0] = double_temp[0] << 11;
	} else {
	/* IEEE float  */
		exp = (double_temp[0] >> 23) & 0xFF;
		fract[1] = (double_temp[0] << 8) & 0x7FFFFF00;
		fract[0] = 0;
	}

	if ((full && (exp == 0x7FF)) || ((!full) && (exp == 0xFF))) {
		if ((fract[1] | fract[0]) == 0) {
			if ((full && (double_temp[1] & 0x80000000))
				|| (!full && (double_temp[0] & 0x80000000))) {
				*buf++ = '-';
				*buf++ = 'I';
				*buf++ = 'N';
				*buf++ = 'F';
			} else {
				*buf++ = '+';
				*buf++ = 'I';
				*buf++ = 'N';
				*buf++ = 'F';
			}
		} else {
			*buf++ = 'N';
			*buf++ = 'a';
			*buf++ = 'N';
		}
		*buf = 0;
		return buf - start;
	}

	if ((exp | fract[1] | fract[0]) != 0) {
		if (full)
			exp -= (1023 - 1);	/* +1 since .1 vs 1. */
		else
			exp -= (127 - 1);	/* +1 since .1 vs 1. */
		fract[1] |= 0x80000000;
		decexp = TRUE;		/* Wasn't zero */
	} else
		decexp = FALSE;		/* It was zero */

	if (decexp && ((full && (double_temp[1] & 0x80000000))
					|| (!full && (double_temp[0] & 0x80000000)))) {
		*buf++ = '-';
	} else if (fplus)
		*buf++ = '+';
	else if (fspace)
		*buf++ = ' ';

	decexp = 0;
	while (exp <= -3) {
		while (fract[1] >= (MAXFP1 / 5)) {
			_rlrshift(fract);
			exp++;
		}
		ltemp[0] = fract[0];
		ltemp[1] = fract[1];
		_llshift(fract);
		_llshift(fract);
		_ladd(fract, ltemp);
		exp++;
		decexp--;

		while (fract[1] <= (MAXFP1 / 2)) {
			_llshift(fract);
			exp--;
		}
	}

	while (exp > 0) {
		_ldiv5(fract);
		exp--;
		decexp++;
		while (fract[1] <= (MAXFP1 / 2)) {
			_llshift(fract);
			exp--;
		}
	}

	while (exp < (0 + 4)) {
		_rlrshift(fract);
		exp++;
	}

	if (precision < 0)
		precision = 6;		/* Default precision if none given */
	prune_zero = FALSE;		/* Assume trailing 0's allowed     */
	if ((c == 'g') || (c == 'G')) {
		if (!falt && (precision > 0))
			prune_zero = TRUE;
		if ((decexp < (-4 + 1)) || (decexp > (precision + 1))) {
			if (c == 'g')
				c = 'e';
			else
				c = 'E';
		} else
			c = 'f';
	}

	if (c == 'f') {
		exp = precision + decexp;
		if (exp < 0)
			exp = 0;
	} else
		exp = precision + 1;
	if (full) {
		digit_count = 16;
		if (exp > 16)
			exp = 16;
	} else {
		digit_count = 8;
		if (exp > 8)
			exp = 8;
	}

	ltemp[0] = 0;
	ltemp[1] = 0x08000000;
	while (exp--) {
		_ldiv5(ltemp);
		_rlrshift(ltemp);
	}

	_ladd(fract, ltemp);
	if (fract[1] & 0xF0000000) {
		_ldiv5(fract);
		_rlrshift(fract);
		decexp++;
	}

	if (c == 'f') {
		if (decexp > 0) {
			while (decexp > 0) {
				*buf++ = _get_digit(fract, &digit_count);
				decexp--;
			}
		} else
			*buf++ = '0';
		if (falt || (precision > 0))
			*buf++ = '.';
		while (precision-- > 0) {
			if (decexp < 0) {
				*buf++ = '0';
				decexp++;
			} else
				*buf++ = _get_digit(fract, &digit_count);
		}
	} else {
		*buf = _get_digit(fract, &digit_count);
		if (*buf++ != '0')
			decexp--;
		if (falt || (precision > 0))
			*buf++ = '.';
		while (precision-- > 0)
			*buf++ = _get_digit(fract, &digit_count);
	}

	if (prune_zero) {
		while (*--buf == '0')
			;
		if (*buf != '.')
			buf++;
	}

	if ((c == 'e') || (c == 'E')) {
		*buf++ = (char) c;
		if (decexp < 0) {
			decexp = -decexp;
			*buf++ = '-';
		} else
			*buf++ = '+';
		*buf++ = (char) ((decexp / 100) + '0');
		decexp %= 100;
		*buf++ = (char) ((decexp / 10) + '0');
		decexp %= 10;
		*buf++ = (char) (decexp + '0');
	}
	*buf = 0;

	return buf - start;
}

/*******************************************************************************
*
* _isdigit - is the input value an ASCII digit character?
*
* This function provides a traditional implementation of the isdigit()
* primitive that is footprint conversative, i.e. it does not utilize a
* lookup table.
*
* RETURNS: non-zero if input integer in an ASCII digit character
*
* INTERNAL
*/

static inline int _isdigit(int c)
{
	return ((c >= '0') && (c <= '9'));
}

static int _atoi(char **sptr)
{
	register char *p;
	register int   i;

	i = 0;
	p = *sptr;
	p--;
	while (_isdigit(((int) *p)))
		i = 10 * i + *p++ - '0';
	*sptr = p;
	return i;
}

int _prf(int (*func)(), void *dest, char *format, va_list vargs)
{
	/*
	 * Due the fact that buffer is passed to functions in this file,
	 * they assume that it's size if MAXFLD + 1. In need of change
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
	char			*cptr_temp;
	int32_t			*int32ptr_temp;
	int32_t			int32_temp;
	uint32_t			uint32_temp;
	uint32_t			double_temp[2];

	count = 0;

	while ((c = *format++)) {
		if (c != '%') {
			if ((*func) (c, dest) == EOF)
				return EOF;
			else
				count++;
		} else {
			fminus = fplus = fspace = falt = FALSE;
			pad = ' ';		/* Default pad character    */
			precision = -1;	/* No precision specified   */

			while (strchr("-+ #0", (c = *format++)) != NULL) {
				switch (c) {
				case '-':
					fminus = TRUE;
					break;

				case '+':
					fplus = TRUE;
					break;

				case ' ':
					fspace = TRUE;
					break;

				case '#':
					falt = TRUE;
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
				width = (int32_t) va_arg(vargs, int32_t);
				if (width < 0) {
					fminus = TRUE;
					width = -width;
				}
				c = *format++;
			} else if (!_isdigit(c))
				width = 0;
			else {
				width = _atoi(&format);	/* Find width */
				c = *format++;
			}

			if (width > MAXFLD)
				width = MAXFLD;

			if (c == '.') {
				c = *format++;
				if (c == '*') {
					precision = (int32_t)
					va_arg(vargs, int32_t);
				} else
					precision = _atoi(&format);

				if (precision > MAXFLD)
					precision = -1;
				c = *format++;
			}

			/*
			 * This implementation only checks that the following format
			 * specifiers are followed by an appropriate type:
			 *    h: short
			 *    l: long
			 *    L: long double
			 * No further special processing is done for them.
			 */

			if (strchr("hlL", c) != NULL) {
				i = c;
				c = *format++;
				switch (i) {
				case 'h':
					if (strchr("diouxX", c) == NULL)
						break;
					break;

				case 'l':
					if (strchr("diouxX", c) == NULL)
						break;
					break;

				case 'L':
					if (strchr("eEfgG", c) == NULL)
						break;
					break;
				}
			}

			need_justifying = FALSE;
			prefix = 0;
			switch (c) {
			case 'c':
				buf[0] = (char) ((int32_t) va_arg(vargs, int32_t));
				buf[1] = '\0';
				need_justifying = TRUE;
				c = 1;
				break;

			case 'd':
			case 'i':
				int32_temp = (int32_t) va_arg(vargs, int32_t);
				c = _to_dec(buf, int32_temp, fplus, fspace, precision);
				if (fplus || fspace || (int32_temp < 0))
					prefix = 1;
				need_justifying = TRUE;
				if (precision != -1)
					pad = ' ';
				break;

			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				/* standard platforms which supports double */
			{
				union {
					double d;
					struct {
						uint32_t u1;
						uint32_t u2;
						} s;
				} u;

				u.d = (double) va_arg(vargs, double);
#if defined(_BIG_ENDIAN)
				double_temp[0] = u.s.u2;
				double_temp[1] = u.s.u1;
#else
				double_temp[0] = u.s.u1;
				double_temp[1] = u.s.u2;
#endif
			}

				c = _to_float(buf, double_temp, DOUBLE, c, falt, fplus,
								fspace, precision);
				if (fplus || fspace || (buf[0] == '-'))
					prefix = 1;
				need_justifying = TRUE;
				break;

			case 'n':
				int32ptr_temp = (int32_t *)va_arg(vargs, int32_t *);
				*int32ptr_temp = count;
				break;

			case 'o':
				uint32_temp = (uint32_t) va_arg(vargs, uint32_t);
				c = _to_octal(buf, uint32_temp, falt, precision);
				need_justifying = TRUE;
				if (precision != -1)
					pad = ' ';
				break;

			case 'p':
				uint32_temp = (uint32_t) va_arg(vargs, uint32_t);
				c = _to_hex(buf, uint32_temp, TRUE, 8, (int) 'x');
				need_justifying = TRUE;
				if (precision != -1)
					pad = ' ';
				break;

			case 's':
				cptr_temp = (char *) va_arg(vargs, char *);
				/* Get the string length */
				for (c = 0; c < MAXFLD; c++) {
					if (cptr_temp[c] == '\0') {
						break;
					}
				}
				if ((precision >= 0) && (precision < c))
					c = precision;
				if (c > 0) {
					memcpy(buf, cptr_temp, (size_t) c);
					need_justifying = TRUE;
				}
				break;

			case 'u':
				uint32_temp = (uint32_t) va_arg(vargs, uint32_t);
				c = _to_udec(buf, uint32_temp, precision);
				need_justifying = TRUE;
				if (precision != -1)
					pad = ' ';
				break;

			case 'x':
			case 'X':
				uint32_temp = (uint32_t) va_arg(vargs, uint32_t);
				c = _to_hex(buf, uint32_temp, falt, precision, c);
				if (falt)
					prefix = 2;
				need_justifying = TRUE;
				if (precision != -1)
					pad = ' ';
				break;

			case '%':
				if ((*func)('%', dest) == EOF)
					return EOF;
				else
					count++;
				break;

			case 0:
				return count;
			}

			if (c >= MAXFLD + 1)
				return EOF;

			if (need_justifying) {
				if (c < width) {
					if (fminus)	{
						/* Left justify? */
						for (i = c; i < width; i++)
							buf[i] = ' ';
					} else {
						/* Right justify */
						(void) memmove((buf + (width - c)), buf, (size_t) (c
										+ 1));
						if (pad == ' ')
							prefix = 0;
						c = width - c + prefix;
						for (i = prefix; i < c; i++)
							buf[i] = pad;
					}
					c = width;
				}

				for (cptr = buf; c > 0; c--, cptr++, count++) {
					if ((*func)(*cptr, dest) == EOF)
						return EOF;
				}
			}
		}
	}
	return count;
}
