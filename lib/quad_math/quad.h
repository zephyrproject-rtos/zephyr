/*-
 * Copyright (c) 1992, 1993
 * The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $OpenBSD: quad.h,v 1.7 2009/11/07 23:09:35 jsg Exp $
 */

/*
 * Quad arithmetic.
 *
 * This library makes the following assumptions:
 *
 *  - The type long long (aka quad_t) exists.
 *
 *  - A quad variable is exactly twice as long as `int'.
 *
 *  - The machine's arithmetic is two's complement.
 *
 * This library can provide 128-bit arithmetic on a machine with 128-bit
 * quads and 64-bit ints, for instance, or 96-bit arithmetic on machines
 * with 48-bit ints.
 */

#ifndef QUAD_H
#define QUAD_H

#include <sys/types.h>
#include <limits.h>

#include <machine/endian.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef char ___QUAD_ASSERT__LENGHTS[sizeof (long long) == 2*sizeof (int) ? +1 : -1];
typedef char ___QUAD_ASSERT__2COMPLEMENT[-1234 == (~1234 + 1) ? +1 : -1];

typedef long long quad_t;
typedef unsigned long long u_quad_t;
typedef    unsigned int    u_int;

#define NULL 0

/*
 * Define high and low parts of a quad_t.
 */

#if BYTE_ORDER == LITTLE_ENDIAN
#   define L (0)
#   define H (1)
#elif BYTE_ORDER == BIG_ENDIAN
#   define L (1)
#   define H (0)
#else
#   error "BYTE_ORDER must be either LITTLE_ENDIAN or BIG_ENDIAN!"
#endif

#define QUAD_MIN (LLONG_MIN)
#define QUAD_MAX (LLONG_MAX)
#define UQUAD_MAX (ULLONG_MAX)

/**
 * Depending on the desired operation, we view a `long long' (aka quad_t) in
 * one or more of the following formats.
 */
union uu {
    quad_t  q;      /**< as a (signed) quad */
    u_quad_t uq;    /**< as an unsigned quad */
    int sl[2];      /**< as two signed ints */
    u_int   ul[2];  /**< as two unsigned ints */
};

/*
 * Total number of bits in a quad_t and in the pieces that make it up.
 * These are used for shifting, and also below for halfword extraction
 * and assembly.
 */
#define QUAD_BITS   (sizeof(quad_t) * CHAR_BIT)
#define INT_BITS    (sizeof(int) * CHAR_BIT)
#define HALF_BITS   (sizeof(int) * CHAR_BIT / 2)

/*
 * Extract high and low shortwords from longword, and move low shortword of
 * longword to upper half of long, i.e., produce the upper longword of
 * ((quad_t)(x) << (number_of_bits_in_int/2)).  (`x' must actually be u_int.)
 *
 * These are used in the multiply code, to split a longword into upper
 * and lower halves, and to reassemble a product as a quad_t, shifted left
 * (sizeof(int)*CHAR_BIT/2).
 */
#define HHALF(x)    ((u_int) (x) >> HALF_BITS)
#define LHALF(x)    ((u_int) (x) & (((int) 1 << HALF_BITS) - 1))
#define LHUP(x)     ((u_int) (x) << HALF_BITS)

typedef unsigned int    qshift_t;

quad_t __adddi3(quad_t, quad_t);
quad_t __anddi3(quad_t, quad_t);
quad_t __ashldi3(quad_t, qshift_t);
quad_t __ashrdi3(quad_t, qshift_t);
int __cmpdi2(quad_t, quad_t);
quad_t __divdi3(quad_t, quad_t);
quad_t __fixdfdi(double);
quad_t __fixsfdi(float);
u_quad_t __fixunsdfdi(double);
u_quad_t __fixunssfdi(float);
double __floatdidf(quad_t);
float __floatdisf(quad_t);
double __floatunsdidf(u_quad_t);
quad_t __iordi3(quad_t, quad_t);
quad_t __lshldi3(quad_t, qshift_t);
quad_t __lshrdi3(quad_t, qshift_t);
quad_t __moddi3(quad_t, quad_t);
quad_t __muldi3(quad_t, quad_t);
quad_t __negdi2(quad_t);
quad_t __one_cmpldi2(quad_t);
u_quad_t __qdivrem(u_quad_t, u_quad_t, u_quad_t *);
quad_t __subdi3(quad_t, quad_t);
int __ucmpdi2(u_quad_t, u_quad_t);
u_quad_t __udivdi3(u_quad_t, u_quad_t );
u_quad_t __umoddi3(u_quad_t, u_quad_t );
quad_t __xordi3(quad_t, quad_t);

#ifdef __cplusplus
}
#endif

#endif /* QUAD_H */
