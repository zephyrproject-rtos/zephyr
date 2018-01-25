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

.set nomips16
#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/m32c0.h>
#include <mips/m32tlb.h>
#include <mips/asm.h>

/*
 * void mips_tlbwi2(tlbhi_t hi, tlblo_t lo0, tlblo_t lo1, unsigned mask,
 *			unsigned idx)
 *
 * Writes hi,lo0,lo1 and msk in to the TLB entry specified by index.
 *
 */
LEAF(mips_tlbwi2)
#if _MIPS_SIM==_ABIO32
	lw	ta0, 16(sp)
#endif
	mtc0	a0, C0_ENTRYHI
	mtc0	a1, C0_ENTRYLO0
	mtc0	a2, C0_ENTRYLO1
	mtc0	a3, C0_PAGEMASK
	mtc0	ta0, C0_INDEX
	ehb				# mtc0, Hazard on tlbwi

	tlbwi
	.set	push
	.set	noreorder
	jr.hb	ra
	nop
	.set	pop
END(mips_tlbwi2)


/*
 * void mips_tlbwr2(tlbhi_t hi, tlblo_t lo0, tlblo_t lo1, unsigned mask)
 *
 * Writes hi, lo0, lo1 and msk into the TLB entry specified by the
 * Random register.
 *
 */
LEAF(mips_tlbwr2)
	mtc0	a0, C0_ENTRYHI
	mtc0	a1, C0_ENTRYLO0
	mtc0	a2, C0_ENTRYLO1
	mtc0	a3, C0_PAGEMASK

	ehb				# mtc0, hazard on tlbwr
	tlbwr
	.set	push
	.set	noreorder
	jr.hb	ra
	nop
	.set	pop
END(mips_tlbwr2)

/*
 * int mips_tlbrwr2(tlbhi_t hi, tlblo_t lo0, tlblo_t lo1, unsigned mask)
 *
 * Probes the TLB for an entry matching hi and if present rewrites that
 * entry, otherwise updates a random entry. A safe way to update the TLB.
 *
 */
LEAF(mips_tlbrwr2)
	mfc0	t0, C0_ENTRYHI
	mtc0	a0, C0_ENTRYHI
	ehb		# MTCO, hazard on tlbp

	tlbp
	ehb		# tlbp, hazard on MFCO C0_INDEX

	mfc0	v0, C0_INDEX
	mtc0	a1, C0_ENTRYLO0
	mtc0	a2, C0_ENTRYLO1
	mtc0	a3, C0_PAGEMASK

	ehb		# mtc0, hazard on tlbwi
	bltz	v0, 1f	# no matching entry

	tlbwi
	mtc0	t0, C0_ENTRYHI
	.set	push
	.set	noreorder
	jr.hb	ra
	nop
	.set	pop

1:	tlbwr
	mtc0	t0, C0_ENTRYHI
	.set	push
	.set	noreorder
	jr.hb	ra
	nop
	.set	pop
END(mips_tlbrwr2)


/*
 * void mips_tlbri2(tlbhi_t *hi, tlblo_t *lo0, tlblo_t *lo1, unsigned *mask,
 *		unsigned index)
 *
 * Reads the TLB entry with specified by index, and returns the EntryHi, EntryLo0,
 * EntryLo1 and PageMask parts in *phi, *plo0, *plo1 and *pmsk respectively.
 *
 */
LEAF(mips_tlbri2)
#if _MIPS_SIM==_ABIO32
	lw	ta0,16(sp)      	# index
#endif
	mtc0	ta0, C0_INDEX
	ehb				# mtc0, hazard on tlbr

	tlbr
	ehb				# tlbr, hazard on entry*, pagemask

	mfc0	t0, C0_ENTRYHI
	mfc0	t1, C0_ENTRYLO0
	mfc0	t2, C0_ENTRYLO1
	mfc0	t3, C0_PAGEMASK
	sw	t0, 0(a0)
	sw	t1, 0(a1)
	sw	t2, 0(a2)
	sw	t3, 0(a3)
	jr	ra
END(mips_tlbri2)


/*
 * int mips_tlbprobe2(tlbhi_t hi, tlblo_t *lo0, tlblo_t *lo1,
 * 	unsigned int *mask)
 *
 * Probes the TLB for an entry matching hi and returns its index, or -1 if
 * not found. If found, then the EntryLo0, EntryLo1 and PageMask parts of the
 * entry are also returned in *plo0, *plo1 and *pmsk respectively.
 *
 */
LEAF(mips_tlbprobe2)
	mfc0	t0, C0_ENTRYHI
	mtc0	a0, C0_ENTRYHI
	ehb				# mtc0, hazard on tlbp

	tlbp
	ehb				# tlpb, hazard on index.

	mfc0	v0, C0_INDEX
	bltz	v0, 1f			# Return -1 if not found.

	tlbr
	ehb				# tlbr, hazard on entry*, pagemask

	mfc0	v1, C0_ENTRYLO0
	mfc0	t1, C0_ENTRYLO1
	mfc0	t2, C0_PAGEMASK

	mtc0	t0, C0_ENTRYHI		# restore entry hi

	sw	v1, 0(a1)
	sw	t1, 0(a2)
	sw	t2, 0(a3)
	.set	push
	.set	noreorder
	jr.hb	ra
	nop
	.set	pop

1:	mtc0	t0, C0_ENTRYHI		# restore entry hi
	li	v0, -1
	.set	push
	.set	noreorder
	jr.hb	ra
	nop
	.set	pop
END(mips_tlbprobe2)


/*
 * void mips_tlbinval(tlbhi_t a0)
 *
 * Probes the TLB for an entry matching hi, and if present invalidates it.
 *
 */
LEAF(mips_tlbinval)
	mfc0	t0, C0_ENTRYHI		# save old entry hi
	mtc0	a0, C0_ENTRYHI
	ehb				# mtc0, Hazard on tlbp

	tlbp
	ehb				# tlbp, Hazard on index, entry*

	mfc0	v0, C0_INDEX
	bltz	v0, 4f

	mtc0	zero, C0_ENTRYLO0
	mtc0	zero, C0_ENTRYLO1

	mfc0	t1, C0_CONFIG3
	ext	t1, t1, CFG3_M_SHIFT, 1
	beqz	t1, 2f

	mfc0	t1, C0_CONFIG4
	ext	t1, t1, CFG4_IE_SHIFT, CFG4_IE_BITS
	beqz	t1, 2f

	li	t1, C0_ENTRYHI_EHINV_MASK
	b	3f

2:	li	t1, (KSEG0_BASE - 2<<13)
5:	addiu	t1, t1, 2<<13
	mtc0	t1, C0_ENTRYHI
	ehb				# mtc0, Hazard on tlbp

	tlbp
	ehb				# tlbp, hazard on index

	mfc0	t2, C0_INDEX
	bgez	t2, 5b

	mtc0	v0, C0_INDEX

3:	mtc0	t1, C0_ENTRYHI
	ehb				# mtco, hazard on tlbwi

	tlbwi
	ehb				# tlbwi, hazard

4:	mtc0	t0,C0_ENTRYHI		# restore entry hi
	.set	push
	.set	noreorder
	jr.hb	ra
	nop
	.set	pop

END(mips_tlbinval)
