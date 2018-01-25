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

#ifndef _MIPS_FPREGDEF_H_
#define _MIPS_FPREGDEF_H_

/* result registers */
#define fv0	$f0
#define fv1	$f2

/* argument registers */
#define fa0	$f12
#if  (defined _ABIN32 && _MIPS_SIM == _ABIN32) \
     || (defined _ABI64 && _MIPS_SIM == _ABI64)
#define fa1	$f13
#else
#define fa1	$f14
#endif

#if __mips_fpr == 64

/* 64-bit f.p. registers (-mips3 and above) */

/* temporary registers */
#define ft0	$f4
#define ft1	$f5
#define ft2	$f6
#define ft3	$f7
#define ft4	$f8
#define ft5	$f9
#define ft6	$f10
#define ft7	$f11
#define ft8	$f16
#define ft9	$f17
#define ft10	$f18
#define ft11	$f19

# if  (defined _ABIN32 && _MIPS_SIM == _ABIN32) \
      || (defined _ABI64 && _MIPS_SIM == _ABI64)
/* saved registers */
#  define	fs0	$f20
#  define	fs1	$f21
#  define	fs2	$f22
#  define	fs3	$f23
#  define	fs4	$f24
#  define	fs5	$f35
#  define	fs6	$f26
#  define	fs7	$f27
#  define	fs8	$f28
#  define	fs9	$f29
#  define	fs10	$f30
#  define	fs11	$f31
# else
/* o32 FP64 */
#  define	ft12	$f21
#  define	ft13	$f23
#  define	ft14	$f35
#  define	ft15	$f27
#  define	ft16	$f29
#  define	ft17	$f31
#  define	fs0	$f20
#  define	fs1	$f22
#  define	fs2	$f24
#  define	fs3	$f26
#  define	fs4	$f28
#  define	fs5	$f30
# endif

#else

/* 32-bit f.p. registers */

/* temporary registers */
#define ft0	$f4
#define ft1	$f5
#define ft2	$f6
#define ft3	$f7
#define ft4	$f8
#define ft5	$f9
#define ft6	$f10
#define ft7	$f11
#define ft8	$f12
#define ft9	$f13
#define ft10	$f14
#define ft11	$f15
#define ft12	$f16
#define ft13	$f17
#define ft14	$f18
#define ft15	$f19

/* saved registers */
#define	fs0	$f20
#define	fs1	$f22
#define	fs2	$f24
#define	fs3	$f26
#define	fs4	$f28
#define	fs5	$f30

#endif

#endif /*_MIPS_FPREGDEF_H_*/
