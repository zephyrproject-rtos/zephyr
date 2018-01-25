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

# Keep each function in a separate named section
#define _FUNCTION_SECTIONS_
.set nomips16

#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/cpu.h>
#include <mips/hal.h>

# Context size, adjusted for ABI parameter area
#define ADJ (NARGSAVE * SZARG)
# Round up to 16-byte boundary (maximum stack alignment required for any
# supported ABI)
#define CTX_SIZEROUND ((CTX_SIZE + ALSZ) & ALMASK)
#define CTX_SIZEADJ (CTX_SIZEROUND + ADJ)

#define e_ISR	s1
#define e_CR	s3
#define e_BADV	s4
#define e_SR	s5
#define e_EPC	s6
#define e_RA	s7

# DESCRIPTION: Exception entry point. This is small because it must go at
#			   EBASE+0x180. It saves enough context to chain onwards to
#			   __exception_save.
#
LEAF(__exception_entry)
	.set	push
	.set	noat
.weak   _mips_tlb_refill
	_mips_tlb_refill = __exception_save
__tlb_refill_loop:
	# Support an alternative entry point at the start of the exception
	# vector.  Since the exception vector is normally placed first
	# in the link map this allows a user to start execution from the
	# same address that an executable is loaded to.
	LA	k1, __first_boot
	lw	k1, 0(k1)
	beqz	k1, 1f
	# The start code is responsible for clearing __first_boot prior
	# to installing the exception handlers.
	j	__start
1:
	LA	k1, _mips_tlb_refill
	beqz	k1, __tlb_refill_loop
	jr	k1

	.org 0x80
.weak   _mips_xtlb_refill
	_mips_xtlb_refill = __exception_save
__xtlb_refill_loop:
	LA	k1, _mips_xtlb_refill
	beqz	k1, __xtlb_refill_loop
	jr	k1

	.org 0x100
.weak   _mips_cache_error
__cache_error_loop:
	LA	k1, _mips_cache_error
	beqz	k1, __cache_error_loop
	jr	k1

	.org 0x180
	# Free up k1, defering sp adjustment until later
	REG_S	k1, (-CTX_SIZEROUND + CTX_K1)(sp)

	# Use k1 to invoke __exception_save
	LA	k1, _mips_general_exception
	jr	k1
	.set    pop
END(__exception_entry)

#
# FUNCTION:	__exception_save
#
# DESCRIPTION: Exception context save. Save the context, then fake up a call
#              frame.
#
ANESTED(__exception_save, _mips_general_exception, CTX_SIZEADJ, zero)
	.globl  __exception_save;
	.set	push
	.set	noat

	# k1 is already saved, so use it to save the users sp
	move	k1, sp
	# Finally adjust sp
	PTR_ADDU sp, sp, -CTX_SIZEADJ	# This should be picked up by the backtracer

	# Save context
	REG_S	$1, CTX_REG(1) + ADJ(sp)
	REG_S	$2, CTX_REG(2) + ADJ(sp)
	REG_S	$3, CTX_REG(3) + ADJ(sp)
	REG_S	$4, CTX_REG(4) + ADJ(sp)
	REG_S	$5, CTX_REG(5) + ADJ(sp)
	REG_S	$6, CTX_REG(6) + ADJ(sp)
	REG_S	$7, CTX_REG(7) + ADJ(sp)
	REG_S	$8, CTX_REG(8) + ADJ(sp)
	REG_S	$9, CTX_REG(9) + ADJ(sp)
	REG_S	$10, CTX_REG(10) + ADJ(sp)
	REG_S	$11, CTX_REG(11) + ADJ(sp)
	REG_S	$12, CTX_REG(12) + ADJ(sp)
	REG_S	$13, CTX_REG(13) + ADJ(sp)
	REG_S	$14, CTX_REG(14) + ADJ(sp)
	REG_S	$15, CTX_REG(15) + ADJ(sp)
	REG_S	$16, CTX_REG(16) + ADJ(sp)
	REG_S	$17, CTX_REG(17) + ADJ(sp)
	REG_S	$18, CTX_REG(18) + ADJ(sp)
	REG_S	$19, CTX_REG(19) + ADJ(sp)
	REG_S	$20, CTX_REG(20) + ADJ(sp)
	REG_S	$21, CTX_REG(21) + ADJ(sp)
	REG_S	$22, CTX_REG(22) + ADJ(sp)
	REG_S	$23, CTX_REG(23) + ADJ(sp)
	REG_S	$24, CTX_REG(24) + ADJ(sp)
	REG_S	$25, CTX_REG(25) + ADJ(sp)
	REG_S	$26, CTX_REG(26) + ADJ(sp)
	# k1/$27 has already been saved
	REG_S	$28, CTX_REG(28) + ADJ(sp)
	REG_S	k1, CTX_REG(29) + ADJ(sp) # Use saved sp from earlier
	REG_S	$30, CTX_REG(30) + ADJ(sp)
	REG_S	$31, CTX_REG(31) + ADJ(sp)
	PTR_S	$0, CTX_LINK + ADJ(sp) # Clear the link field

#if (__mips_isa_rev < 6)
	mfhi	$9
	mflo	$10
	REG_S	$9, CTX_HI0 + ADJ(sp)
	REG_S	$10, CTX_LO0 + ADJ(sp)
#endif

	# Trick the backtracer into stepping back to the point where the exception
	# occurred.
	PTR_MFC0 ra, C0_EPC
	mfc0	e_CR, C0_CR
	REG_S	ra, CTX_EPC + ADJ(sp)

	# Finish storing the rest of the CP0 registers
	PTR_MFC0 $9, C0_BADVADDR
	REG_S	$9, CTX_BADVADDR + ADJ(sp)
	sw	e_CR, CTX_CAUSE + ADJ(sp)

	move	$11, $0
	move	$12, $0
	mfc0	$9, C0_CONFIG3
	ext	$10, $9, CFG3_BP_SHIFT, 1
	beqz	$10, 1f
	mfc0	$11, C0_BADPINSTR
1:
	ext	$9, $9, CFG3_BI_SHIFT, 1
	beqz	$9, 1f
	mfc0	$12, C0_BADINSTR
1:
	sw	$11, CTX_BADPINSTR + ADJ(sp)
	sw	$12, CTX_BADINSTR + ADJ(sp)

	# Start computing the address of the context for a0
	move	a0, sp

	# Clear EXL.  Exceptions can now nest.
	mfc0	e_SR, C0_SR
	sw	e_SR, CTX_STATUS + ADJ(sp)
	lui	$9, %hi(~SR_EXL)
	addiu	$9, $9, %lo(~SR_EXL)
	and	e_SR, e_SR, $9
	mtc0	e_SR, C0_SR

	# Manually set up the return address to restore the context below
	LA	ra, 1f
	# Extract the cause code
	and	a1, e_CR, CR_XMASK

	# Finish computing the address of the context for a0
	addiu	a0, a0, ADJ

	# Shift exception number down into expected range
	srl	a1, a1, 2

	# Call the handler, indirect through t9 albeit not for any specific
	# reason
	LA	t9, _mips_handle_exception
	jr	t9

1:	# Return point from handler
	# Load context

#if (__mips_isa_rev < 6)
	REG_L	$9, CTX_HI0 + ADJ(sp)
	REG_L	$10, CTX_LO0 + ADJ(sp)
	mthi	$9
	mtlo	$10
#endif

	REG_L	$1, CTX_REG(1) + ADJ(sp)
	REG_L	$2, CTX_REG(2) + ADJ(sp)
	REG_L	$3, CTX_REG(3) + ADJ(sp)
	REG_L	$4, CTX_REG(4) + ADJ(sp)
	REG_L	$5, CTX_REG(5) + ADJ(sp)
	REG_L	$6, CTX_REG(6) + ADJ(sp)
	REG_L	$7, CTX_REG(7) + ADJ(sp)
	REG_L	$8, CTX_REG(8) + ADJ(sp)
	REG_L	$9, CTX_REG(9) + ADJ(sp)
	REG_L	$10, CTX_REG(10) + ADJ(sp)
	REG_L	$11, CTX_REG(11) + ADJ(sp)
	REG_L	$12, CTX_REG(12) + ADJ(sp)
	REG_L	$13, CTX_REG(13) + ADJ(sp)
	REG_L	$14, CTX_REG(14) + ADJ(sp)
	REG_L	$15, CTX_REG(15) + ADJ(sp)
	REG_L	$16, CTX_REG(16) + ADJ(sp)
	REG_L	$17, CTX_REG(17) + ADJ(sp)
	REG_L	$18, CTX_REG(18) + ADJ(sp)
	REG_L	$19, CTX_REG(19) + ADJ(sp)
	REG_L	$20, CTX_REG(20) + ADJ(sp)
	REG_L	$21, CTX_REG(21) + ADJ(sp)
	REG_L	$22, CTX_REG(22) + ADJ(sp)
	REG_L	$23, CTX_REG(23) + ADJ(sp)
	REG_L	$24, CTX_REG(24) + ADJ(sp)
	REG_L	$25, CTX_REG(25) + ADJ(sp)
	# $26/K0 and $27/K1 are restored with interrupts disabled
	REG_L	$28, CTX_REG(28) + ADJ(sp)
	# $29/SP is restored last
	REG_L	$30, CTX_REG(30) + ADJ(sp)
	REG_L	$31, CTX_REG(31) + ADJ(sp)
	di
	lw	k0, CTX_STATUS + ADJ(sp)
	REG_L	k1, CTX_EPC + ADJ(sp)
	mtc0	k0, C0_SR
	PTR_MTC0 k1, C0_EPC
	ehb
	REG_L	k0, CTX_K0 + ADJ(sp)
	REG_L	k1, CTX_K1 + ADJ(sp)
	REG_L	sp, CTX_SP + ADJ(sp)
	# Return from exception
	eret
	.set	pop
END(__exception_save)
