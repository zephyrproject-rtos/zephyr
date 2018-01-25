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
#include <mips/regdef.h>
#include <mips/asm.h>
#include <mips/cpu.h>
#include <mips/hal.h>

# Used to support an alternate entry point that overlays the TLB refill
# exception entry point.  This flag must be cleared before exceptions
# are ready to be handled.
.data
EXPORTS(__first_boot, 4)
	.word	0x1

_TEXT_SECTION

#
# FUNCTION:	__register_excpt_handler
#
# DESCRIPTION: Register __exception_entry at EBASE+0x180. Return the new
#	       value for C0_SR.
#
WLEAF(__register_excpt_handler)
	.set	push
	.set	noat

	# Fetch initial status
	mfc0	$5, C0_SR

	# Get into a sane state.
	# Important things: base mode is kernel and ERL, ESL, IE are clear
	# Set BEV=1 to allow changing EBASE later
	lui	$10, %hi(SR_BEV)
	mtc0	$10, C0_SR
	ehb

	# Enable use of a boot state hook
	# $4 = Boot time RA
	# $5 = Boot time SR
	# $6 = Current RA. There is no stack so get the callee to pass this
	#      back.
.weak	__register_excpt_boot
	LA	$9, __register_excpt_boot
	beqz	$9, 1f
	move	$6, ra
	jalr	$9
	move	ra, $2
1:
	# Clear first boot flag
	LA	$9, __first_boot
	sw	$0, 0($9)

	mfc0	$12, C0_CONFIG3
#if defined (__mips_micromips)
	# Set Config3.ISAOnExc for micromips
	lui	$4, 1			/* 0x10000 */
	or	$4, $4, $12
	mtc0	$4, C0_CONFIG3
#endif

	# Set desired EBASE
	LA	$10, __excpt_ebase
	# Always set the write gate as the requested EBASE may not be in kseg0.
	# This may or may not exist in hardware but if it doesn't then the
	# ebase address will simply get masked with inevitable consequences.
	ori	$10, $10, EBASE_WG
	PTR_MTC0 $10, C0_EBASE
	ehb

	# Set up new empty status value
	move	$2, $0

	# Set up vector spacing
	LA	$9, __isr_vec_space

	# Check for vectored interrupt support
	ext	$10, $12, CFG3_VI_SHIFT, 1
	ext     $11, $12, CFG3_VEIC_SHIFT, 1
	or	$10, $10, $11
	# Skip vector spacing setup if neither VINT nor VEIC is present
	beqz	$10, 1f

	# Set vector spacing
	mfc0	$10, C0_INTCTL
	ins	$10, $9, 0, 10
	mtc0	$10, C0_INTCTL
	b	2f
1:
	# Check non-zero vector spacing without vectored interrupt support.
	# If so, do not enable interrupts.
	bnez	$9, 3f
2:
	# Turn on use of the special exception vector and enable interrupts
	lui	$9, %hi(CR_IV)
	mtc0	$9, C0_CAUSE
	ehb

	# Check for VEIC and do not enable interrupts if EIC is active
	ext     $10, $12, CFG3_VEIC_SHIFT, 1
	bnez	$10, 3f

	# Enable interrupts in the new status value
	ori	$2, $2, SR_IE
3:
	jr	$31

	.set	pop
WEND(__register_excpt_handler)
