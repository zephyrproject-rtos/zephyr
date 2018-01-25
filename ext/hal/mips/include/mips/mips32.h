/*
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
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

#ifndef _MIPS32_H_
#define _MIPS32_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__

#if ! __mips16

/* C interface to clz/clo instructions */

/* count leading zeros */
# define mips_clz(x) __builtin_clz (x)

/* count trailing zeros */
# define mips_ctz(x) __builtin_ctz (x)

#define mips_clo(x) __extension__({ \
    unsigned int __x = (x); \
    unsigned int __v; \
    __asm__ ("clo %0,%1" : "=d" (__v) : "d" (__x)); \
    __v; \
})

#ifndef __mips64

/* Simulate 64-bit count leading zeroes */
#define mips_dclz(x) __extension__({ \
    unsigned long long __x = (x); \
    unsigned int __hx = (__x >> 32); \
    __hx ? mips_clz(__hx) : 32 + mips_clz(__x); \
})

/* Simulate 64-bit count leading ones */
#define mips_dclo(x) __extension__({ \
    unsigned long long __x = (x); \
    unsigned int __hx = (__x >> 32); \
    (~__hx) ? mips_clo(__hx) : 32 + mips_clo(__x); \
})

/* Simulate 64-bit count trailing zeroes */
#define mips_dctz(x) __extension__({ \
    unsigned long long __dx = (x); \
    unsigned int __ldx = __dx; \
    unsigned int __hdx = __dx >> 32; \
    __ldx ? mips_ctz(__ldx) : (63 ^ mips_clz(__hdx & -__hdx)); \
   })
#endif

/* MIPS32r2 wsbh opcode */
#define _mips32r2_wsbh(x) __extension__({ \
    unsigned int __x = (x), __v; \
    __asm__ ("wsbh %0,%1" \
	     : "=d" (__v) \
	     : "d" (__x)); \
    __v; \
})

/* MIPS32r2 byte-swap word */
#define _mips32r2_bswapw(x) __extension__({ \
    unsigned int __x = (x), __v; \
    __asm__ ("wsbh %0,%1; rotr %0,16" \
	     : "=d" (__v) \
	     : "d" (__x)); \
    __v; \
})

/* MIPS32r2 insert bits */
#define _mips32r2_ins(tgt,val,pos,sz) __extension__({ \
    unsigned int __t = (tgt), __v = (val); \
    __asm__ ("ins %0,%z1,%2,%3" \
	     : "+d" (__t) \
	     : "dJ" (__v), "I" (pos), "I" (sz)); \
    __t; \
})

/* MIPS32r2 extract bits */
#define _mips32r2_ext(x,pos,sz) __extension__({ \
    unsigned int __x = (x), __v; \
    __asm__ ("ext %0,%z1,%2,%3" \
	     : "=d" (__v) \
	     : "dJ" (__x), "I" (pos), "I" (sz)); \
    __v; \
})

#if __mips_isa_rev < 6

/* MIPS32r2 jr.hb */
#if _MIPS_SIM == _ABIO32 || _MIPS_SIM == _ABIN32
#define mips32_jr_hb() __asm__ __volatile__(	\
       "bltzal	$0,0f\n"			\
"0:     addiu	$31,1f-0b\n"			\
"       jr.hb	$31\n"				\
"1:"						\
	: : : "$31")
#elif _MIPS_SIM == _ABI64
#define mips32_jr_hb() __asm__ __volatile__(	\
       "bltzal	$0,0f\n"			\
"0:     daddiu	$31,1f-0b\n"			\
"       jr.hb	$31\n"				\
"1:"						\
	: : : "$31")
#else
#error Unknown ABI
#endif

#else /*  __mips_isa_rev < 6  */

/* MIP32r6 jr.hb */
#if _MIPS_SIM == _ABIO32 ||  _MIPS_SIM == _ABIN32
#define mips32_jr_hb() __asm__ __volatile__(	\
       "auipc	$24,%pcrel_hi(1f)\n"		\
       "addiu	$24,%pcrel_lo(1f + 4)\n"	\
       "jr.hb	$24\n"				\
"1:"						\
       : : : "$24")
#elif _MIPS_SIM == _ABI64
#define mips32_jr_hb() __asm__ __volatile__(	\
       "auipc	$24,%pcrel_hi(1f)\n"		\
       "daddiu	$24,%pcrel_lo(1f + 4)\n"	\
       "jr.hb	$24\n"				\
"1:"						\
       : : : "$24")
#else
#error Unknown ABI
#endif

#endif /* __mips_isa_rev < 6 */

#endif /* ! __mips16 */

#endif /* __ASSEMBLER__ */

#ifdef __cplusplus
}
#endif

#endif /* _MIPS32_H_ */
