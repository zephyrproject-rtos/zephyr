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

#ifndef _MIPS_ENDIAN_H_
#define _MIPS_ENDIAN_H_

#ifndef __ASSEMBLER__
#include <stdint.h>	/* get compiler types */
#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BYTE_ORDER
/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define LITTLE_ENDIAN   1234    /* least-significant byte first (vax) */
#define BIG_ENDIAN      4321    /* most-significant byte first (IBM, net) */

#if defined(__MIPSEB__) || defined(MIPSEB)
#define BYTE_ORDER      BIG_ENDIAN
#elif defined(__MIPSEL__) || defined(MIPSEL)
#define BYTE_ORDER      LITTLE_ENDIAN
#else
#error BYTE_ORDER
#endif

#ifndef __ASSEMBLER__

#if __mips_isa_rev >= 2 && ! __mips16

/* MIPS32r2 & MIPS64r2 can use the wsbh and rotate instructions, define
   MD_SWAP so that <sys/endian.h> will use them. */

#define MD_SWAP

#define __swap16md(x) __extension__({					\
    uint16_t __swap16md_x = (x);					\
    uint16_t __swap16md_v;						\
    __asm__ ("wsbh %0,%1" 						\
	     : "=d" (__swap16md_v) 					\
	     : "d" (__swap16md_x)); 					\
    __swap16md_v; 							\
})

#define __swap32md(x) __extension__({					\
    uint32_t __swap32md_x = (x);					\
    uint32_t __swap32md_v;						\
    __asm__ ("wsbh %0,%1; rotr %0,16" 					\
	     : "=d" (__swap32md_v) 					\
	     : "d" (__swap32md_x)); 					\
    __swap32md_v; 							\
})

#endif
#endif /* __ASSEMBLER__ */
#endif	/* BYTE_ORDER */

#ifdef __cplusplus
}
#endif

#endif /*_MIPS_ENDIAN_H_*/
