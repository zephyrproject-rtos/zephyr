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

 /*
  * m32tlb.h: MIPS32 TLB support functions
  */

#ifndef _M32TLB_H_
#define _M32TLB_H_

#if __mips != 32 && __mips != 64
#error use -mips32 or -mips64 option with this file
#endif

#include <mips/notlb.h>
#ifndef __ASSEMBLER__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int tlbhi_t;
typedef unsigned int tlblo_t;

// Returns the size of the TLB.
int mips_tlb_size (void);
 // Probes the TLB for an entry matching hi, and if present invalidates it.
void mips_tlbinval (tlbhi_t hi);

// Invalidate the whole TLB.
void mips_tlbinvalall (void);

// Reads the TLB entry with specified by index, and returns the EntryHi,
// EntryLo0, EntryLo1 and PageMask parts in *phi, *plo0, *plo1 and *pmsk
// respectively.
void mips_tlbri2 (tlbhi_t *phi, tlblo_t *plo0, tlblo_t *plo1, unsigned *pmsk,
		  int index);

// Writes hi, lo0, lo1 and msk into the TLB entry specified by index.
void mips_tlbwi2 (tlbhi_t hi, tlblo_t lo0, tlblo_t lo1, unsigned msk,
		  int index);

// Writes hi, lo0, lo1 and msk into the TLB entry specified by the
// Random register.
void mips_tlbwr2 (tlbhi_t hi, tlblo_t lo0, tlblo_t lo1, unsigned msk);

// Probes the TLB for an entry matching hi and returns its index, or -1 if
// not found. If found, then the EntryLo0, EntryLo1 and PageMask parts of the
// entry are also returned in *plo0, *plo1 and *pmsk respectively
int mips_tlbprobe2 (tlbhi_t hi, tlblo_t *plo0, tlblo_t *plo1, unsigned *pmsk);

// Probes the TLB for an entry matching hi and if present rewrites that entry,
// otherwise updates a random entry. A safe way to update the TLB.
int mips_tlbrwr2 (tlbhi_t hi, tlblo_t lo0, tlblo_t lo1, unsigned msk);

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLER__ */

#endif /* _M32TLB_H_ */
