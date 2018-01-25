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

#ifndef _MIPS_ASE_H_
#define _MIPS_ASE_H_

/* 32-bit DSP Control register */
#define DSPCTL_POS_SHIFT	0
#define DSPCTL_POS_MASK		0x0000007f
#define DSPCTL_SCOUNT_SHIFT	7
#define DSPCTL_SCOUNT_MASK	0x00001f80
#define DSPCTL_C		0x00002000
#define DSPCTL_OU_SHIFT		16
#define DSPCTL_OU_MASK		0x00ff0000
#define  DSPCTL_OU_AC0OVF	 0x00010000
#define  DSPCTL_OU_AC1OVF	 0x00020000
#define  DSPCTL_OU_AC2OVF	 0x00040000
#define  DSPCTL_OU_AC3OVF	 0x00080000
#define  DSPCTL_OU_ALUOVF	 0x00100000
#define  DSPCTL_OU_MULOVF	 0x00200000
#define  DSPCTL_OU_SHFOVF	 0x00400000
#define  DSPCTL_OU_EXTOVF	 0x00800000
#define DSPCTL_CCOND_SHIFT	24
#define DSPCTL_CCOND_MASK	0x0f000000

/* RDDSP/WRDSP instruction mask bits */
#define RWDSP_POS		0x01
#define RWDSP_SCOUNT		0x02
#define RWDSP_C			0x04
#define RWDSP_OU		0x08
#define RWDSP_CCOND		0x10

#endif /* _MIPS_DSP_H_ */
