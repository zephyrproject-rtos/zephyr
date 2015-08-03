/* dtls -- a very basic DTLS implementation
 *
 * Copyright (C) 2011 Olaf Bergmann <bergmann@tzi.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _DTLS_NUMERIC_H_
#define _DTLS_NUMERIC_H_

#include <stdint.h>

#ifndef min
#define min(A,B) ((A) <= (B) ? (A) : (B))
#endif

#ifndef max
#define max(A,B) ((A) < (B) ? (B) : (A))
#endif

/* this one is for consistency... */
static inline int dtls_int_to_uint8(unsigned char *field, uint8_t value)
{
  field[0] = value & 0xff;
  return 1;
}

static inline int dtls_int_to_uint16(unsigned char *field, uint16_t value)
{
  field[0] = (value >> 8) & 0xff;
  field[1] = value & 0xff;
  return 2;
}

static inline int dtls_int_to_uint24(unsigned char *field, uint32_t value)
{
  field[0] = (value >> 16) & 0xff;
  field[1] = (value >> 8) & 0xff;
  field[2] = value & 0xff;
  return 3;
}

static inline int dtls_int_to_uint32(unsigned char *field, uint32_t value)
{
  field[0] = (value >> 24) & 0xff;
  field[1] = (value >> 16) & 0xff;
  field[2] = (value >> 8) & 0xff;
  field[3] = value & 0xff;
  return 4;
}

static inline int dtls_int_to_uint48(unsigned char *field, uint64_t value)
{
  field[0] = (value >> 40) & 0xff;
  field[1] = (value >> 32) & 0xff;
  field[2] = (value >> 24) & 0xff;
  field[3] = (value >> 16) & 0xff;
  field[4] = (value >> 8) & 0xff;
  field[5] = value & 0xff;
  return 6;
}

static inline int dtls_int_to_uint64(unsigned char *field, uint64_t value)
{
  field[0] = (value >> 56) & 0xff;
  field[1] = (value >> 48) & 0xff;
  field[2] = (value >> 40) & 0xff;
  field[3] = (value >> 32) & 0xff;
  field[4] = (value >> 24) & 0xff;
  field[5] = (value >> 16) & 0xff;
  field[6] = (value >> 8) & 0xff;
  field[7] = value & 0xff;
  return 8;
}

static inline uint8_t dtls_uint8_to_int(const unsigned char *field)
{
  return (uint8_t)field[0];
}

static inline uint16_t dtls_uint16_to_int(const unsigned char *field)
{
  return ((uint16_t)field[0] << 8)
	 | (uint16_t)field[1];
}

static inline uint32_t dtls_uint24_to_int(const unsigned char *field)
{
  return ((uint32_t)field[0] << 16)
	 | ((uint32_t)field[1] << 8)
	 | (uint32_t)field[2];
}

static inline uint32_t dtls_uint32_to_int(const unsigned char *field)
{
  return ((uint32_t)field[0] << 24)
	 | ((uint32_t)field[1] << 16)
	 | ((uint32_t)field[2] << 8)
	 | (uint32_t)field[3];
}

static inline uint64_t dtls_uint48_to_int(const unsigned char *field)
{
  return ((uint64_t)field[0] << 40)
	 | ((uint64_t)field[1] << 32)
	 | ((uint64_t)field[2] << 24)
	 | ((uint64_t)field[3] << 16)
	 | ((uint64_t)field[4] << 8)
	 | (uint64_t)field[5];
}

static inline uint64_t dtls_uint64_to_int(const unsigned char *field)
{
  return ((uint64_t)field[0] << 56)
	 | ((uint64_t)field[1] << 48)
	 | ((uint64_t)field[2] << 40)
	 | ((uint64_t)field[3] << 32)
	 | ((uint64_t)field[4] << 24)
	 | ((uint64_t)field[5] << 16)
	 | ((uint64_t)field[6] << 8)
	 | (uint64_t)field[7];
}

#endif /* _DTLS_NUMERIC_H_ */
