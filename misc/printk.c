/*
 * Copyright (c) 2010, 2013-2014 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Low-level debug output
 *
 * Low-level debugging output. Platform installs a character output routine at
 * init time. If no routine is installed, a nop routine is called.
 */

#include <misc/printk.h>
#include <stdarg.h>
#include <toolchain.h>
#include <sections.h>

#ifdef CONFIG_PRINTK

static void _printk_dec_ulong(const unsigned long num);
static void _printk_hex_ulong(const unsigned long num);

/**
 * @brief Default character output routine that does nothing
 * @param c Character to swallow
 *
 * @return 0
 */
static int _nop_char_out(int c)
{
	ARG_UNUSED(c);

	/* do nothing */
	return 0;
}

static int (*_char_out)(int) = _nop_char_out;


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
 * @brief Printk internals
 *
 * See printk() for description.
 * @param fmt Format string
 * @param ap Variable parameters
 *
 * @return N/A
 */
static inline void _vprintk(const char *fmt, va_list ap)
{
	int might_format = 0; /* 1 if encountered a '%' */

	/* fmt has already been adjusted if needed */

	while (*fmt) {
		if (!might_format) {
			if (*fmt != '%') {
				_char_out((int)*fmt);
			} else {
				might_format = 1;
			}
		} else {
			if (*fmt == '%') {
				_char_out((int)'%');
				might_format = 0;
			} else {
				switch (*fmt) {
				case 'd':
				case 'i': {
					long d = va_arg(ap, long);
					if (d < 0) {
						_char_out((int)'-');
						d = -d;
					}
					_printk_dec_ulong(d);
					break;
				}
				case 'u': {
					unsigned long u = va_arg(
						ap, unsigned long);
					_printk_dec_ulong(u);
					break;
				}
				case 'x':
				case 'X':
				case 'p': {
					unsigned long x = va_arg(
						ap, unsigned long);
					_printk_hex_ulong(x);
					break;
				}
				case 's': {
					char *s = va_arg(ap, char *);
					while (*s)
						_char_out((int)(*s++));
					break;
				}
				default:
					_char_out((int)'%');
					_char_out((int)*fmt);
					break;
				}
				might_format = 0;
			}
		}

		++fmt;
	}
}

/**
 * @brief Output a string
 *
 * Output a string on output installed by platform at init time. Some
 * printf-like formatting is available.
 *
 * Available formatting:
 * - %x/%X:  outputs a 32-bit number in ABCDWXYZ format. All eight digits
 *	    are printed: if less than 8 characters are needed, leading zeroes
 *	    are displayed.
 * - %s:	    output a null-terminated string
 * - %p:     pointer, same as %x
 * - %d/%i/%u: outputs a 32-bit number in unsigned decimal format.
 *
 * @param fmt formatted string to output
 *
 * @return N/A
 */
void printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_vprintk(fmt, ap);
	va_end(ap);
}

/**
 * @brief Output an unsigned long in hex format
 *
 * Output an unsigned long on output installed by platform at init time. Should
 * be able to handle an unsigned long of any size, 32 or 64 bit.
 * @param num Number to output
 *
 * @return N/A
 */
static void _printk_hex_ulong(const unsigned long num)
{
	int size = sizeof(num) * 2;

	for (; size; size--) {
		char nibble = (num >> ((size - 1) << 2) & 0xf);
		nibble += nibble > 9 ? 87 : 48;
		_char_out((int)nibble);
	}
}

/**
 * @brief Output an unsigned long (32-bit) in decimal format
 *
 * Output an unsigned long on output installed by platform at init time. Only
 * works with 32-bit values.
 * @param num Number to output
 *
 * @return N/A
 */
static void _printk_dec_ulong(const unsigned long num)
{
	unsigned long pos = 999999999;
	unsigned long remainder = num;
	int found_largest_digit = 0;

	while (pos >= 9) {
		if (found_largest_digit || remainder > pos) {
			found_largest_digit = 1;
			_char_out((int)((remainder / (pos + 1)) + 48));
		}
		remainder %= (pos + 1);
		pos /= 10;
	}
	_char_out((int)(remainder + 48));
}

#else /* CONFIG_PRINTK */

/**
 * @brief Output a string
 *
 * Debugging output is dropped if it is not to be sent to the console.
 * @param fmt Format
 *
 * @return N/A
 */
void printk(const char *fmt, ...)
{
	ARG_UNUSED(fmt);
}

#endif /* CONFIG_PRINTK */
