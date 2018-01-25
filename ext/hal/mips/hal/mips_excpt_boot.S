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
#include <mips/cpu.h>
#include <mips/hal.h>
#include <mips/endian.h>
#include <mips/regdef.h>

	# Create space to store k0, k1, ra and sp
	.data
	.global	__start_ctx
	.balign	SZREG
__start_ctx:
	.space	SZREG * 18
#define	start_ctx_sr	(SZREG * 0)
#define	start_ctx_s0	(SZREG * 1)
#define	start_ctx_s1	(SZREG * 2)
#define	start_ctx_s2	(SZREG * 3)
#define	start_ctx_s3	(SZREG * 4)
#define	start_ctx_s4	(SZREG * 5)
#define	start_ctx_s5	(SZREG * 6)
#define	start_ctx_s6	(SZREG * 7)
#define	start_ctx_s7	(SZREG * 8)
#define	start_ctx_k0	(SZREG * 9)
#define	start_ctx_k1	(SZREG * 10)
#define	start_ctx_gp	(SZREG * 11)
#define	start_ctx_sp	(SZREG * 12)
#define	start_ctx_fp	(SZREG * 13)
#define	start_ctx_ra	(SZREG * 14)
#define	start_ctx_ictl	(SZREG * 15)
#define	start_ctx_ebase	(SZREG * 16)	/* saved EBASE */
#define	chain_ebase	(SZREG * 17)	/* chained EBASE */

#if defined (__mips_micromips)
	.space	SZREG
#define	start_ctx_conf3	(SZREG * 18)	/* saved Config3 $16,3 for micromips */
#endif

#
# FUNCTION:	__register_excpt_boot
#
# DESCRIPTION: Save all boot state. Some state is already clobbered:
#              $4 = Boot ra
#	       $5 = Boot SR
#	       $6 = caller's RA to be preserved and returned in $2
#
WLEAF(__register_excpt_boot)
	.set	push
	.set	noat

	# Save C0_SR IE and BEV
	LA	$9, __start_ctx
	REG_S	$4, start_ctx_ra($9)	/* $4 holds $31 */
	REG_S	$5, start_ctx_sr($9)	/* $5 holds SR */

	REG_S	$16, start_ctx_s0($9)
	REG_S	$17, start_ctx_s1($9)
	REG_S	$18, start_ctx_s2($9)
	REG_S	$19, start_ctx_s3($9)
	REG_S	$20, start_ctx_s4($9)
	REG_S	$21, start_ctx_s5($9)
	REG_S	$22, start_ctx_s6($9)
	REG_S	$23, start_ctx_s7($9)
	REG_S	$26, start_ctx_k0($9)
	REG_S	$27, start_ctx_k1($9)
	REG_S	$28, start_ctx_gp($9)
	REG_S	$29, start_ctx_sp($9)
	REG_S	$30, start_ctx_fp($9)

	mfc0	$12, C0_CONFIG3
#if defined (__mips_micromips)
	# Save Config3
	REG_S	$12, start_ctx_conf3($9)
#endif
	mfc0	$12, C0_INTCTL
	REG_S	$12, start_ctx_ictl($9)

	# Save C0_EBASE
	PTR_MFC0 $10, C0_EBASE
	REG_S	$10, start_ctx_ebase($9)

	# Check if we booted with BEV==1
	lui	$11, %hi(SR_BEV)
	and	$11, $8, $11
	beqz	$11, 1f

	# BEV==0 - set chain_ebase to 0xbfc00200
	# Apply the offset of 0x200 so that the boot vector entries line up
	# with the offsets in a non-boot vector
	lui	$10, 0xbfc0
	ori	$10, $10, 0x200

	# No - set chain_ebase to C0_EBASE
1:	REG_S	$10, chain_ebase($9)

	# Return the third argument
	move	$2, $6
	jr	$31

	.set	pop
WEND(__register_excpt_boot)

#
# FUNCTION:	__return_to_boot (int exit_code)
#
# DESCRIPTION: UHI EXIT wasn't handled, return back to caller of _start
#
WLEAF(__return_to_boot)
	.set	push
	.set	noat
	# Disable interrupts for safety
	di
	ehb
	# Set BEV=1 to allow changing EBASE
	mfc0	$9, C0_SR
	lui	$10, %hi(SR_BEV)
	or	$9, $9, $10
	mtc0	$9, C0_SR
	ehb

	# Restore C0_EBASE
	LA	$9, __start_ctx
	REG_L	$9, start_ctx_ebase($9)
	# Set the write gate to potentially change upper bits
	ori	$10, $9, EBASE_WG
	PTR_MTC0 $10, C0_EBASE
	# Check if the write gate was set on startup
	andi	$11, $9, EBASE_WG
	bnez	$11, 1f

	# If write gate wasn't set then clear the write gate again
	PTR_MTC0 $9, C0_EBASE
1:	ehb

	# Restore original state
	LA	$9, __start_ctx
	REG_L	$16, start_ctx_s0($9)
	REG_L	$17, start_ctx_s1($9)
	REG_L	$18, start_ctx_s2($9)
	REG_L	$19, start_ctx_s3($9)
	REG_L	$20, start_ctx_s4($9)
	REG_L	$21, start_ctx_s5($9)
	REG_L	$22, start_ctx_s6($9)
	REG_L	$23, start_ctx_s7($9)
	REG_L	$26, start_ctx_k0($9)
	REG_L	$27, start_ctx_k1($9)
	REG_L	$28, start_ctx_gp($9)
	REG_L	$29, start_ctx_sp($9)
	REG_L	$30, start_ctx_fp($9)
	REG_L	$31, start_ctx_ra($9)

#if defined (__mips_micromips)
	# Restore Config3
	REG_L	$2, start_ctx_conf3($9)
	mtc0	$2, C0_CONFIG3
#endif
	# Restore IntCtl
	REG_L	$2, start_ctx_ictl($9)
	mtc0	$2, C0_INTCTL

	REG_L	$9, start_ctx_sr($9)

	# Restore C0_STATUS IE and BEV to boot value
	mtc0	$9, C0_SR
	mtc0	$0, C0_CAUSE

	# Return with exit code
	move	$2, $4
	jr.hb	$31
	.set	pop
WEND(__return_to_boot)

#
# FUNCTION:	int __chain_uhi_excpt (struct gpctx *ctx);
#
# DESCRIPTION: Call exception handler of the boot
#
WLEAF(__chain_uhi_excpt)
	.set	push
	.set	noat

	# Move context pointer into position.  Use $3 as scratch
	# as it is the only register that is clobbered by all
	# UHI calls and is not used as an input.
	move	$3, $4

#if (__mips_isa_rev < 6)
	REG_L	$9, CTX_HI0($3)
	REG_L	$10, CTX_LO0($3)
	mthi	$9
	mtlo	$10
#endif

	lw	$9, CTX_STATUS($3)
	mtc0	$9, C0_SR
	REG_L	$9, CTX_EPC($3)
	PTR_MTC0 $9, C0_EPC
	ehb

	# Restore the common context
	REG_L	$1, CTX_REG(1)($3)
	REG_L	$2, CTX_REG(2)($3)
	REG_L	$4, CTX_REG(4)($3)
	REG_L	$5, CTX_REG(5)($3)
	REG_L	$6, CTX_REG(6)($3)
	REG_L	$7, CTX_REG(7)($3)
	REG_L	$8, CTX_REG(8)($3)
	REG_L	$9, CTX_REG(9)($3)
	REG_L	$10, CTX_REG(10)($3)
	REG_L	$11, CTX_REG(11)($3)
	REG_L	$12, CTX_REG(12)($3)
	REG_L	$13, CTX_REG(13)($3)
	REG_L	$14, CTX_REG(14)($3)
	REG_L	$15, CTX_REG(15)($3)
	REG_L	$16, CTX_REG(16)($3)
	REG_L	$17, CTX_REG(17)($3)
	REG_L	$18, CTX_REG(18)($3)
	REG_L	$19, CTX_REG(19)($3)
	REG_L	$20, CTX_REG(20)($3)
	REG_L	$21, CTX_REG(21)($3)
	REG_L	$22, CTX_REG(22)($3)
	REG_L	$23, CTX_REG(23)($3)
	REG_L	$24, CTX_REG(24)($3)
	REG_L	$25, CTX_REG(25)($3)
	REG_L	$28, CTX_REG(28)($3)
	REG_L	$29, CTX_REG(29)($3)
	REG_L	$30, CTX_REG(30)($3)
	REG_L	$31, CTX_REG(31)($3)

	# Restore chained exception handlers kernel regs
	LA	$3, __start_ctx
	REG_L	$26, start_ctx_k0($3)
	REG_L	$27, start_ctx_k1($3)

#if defined (__mips_micromips)
	# OR the address with Config3.ISAOnExc bit
	REG_L	$3, start_ctx_conf3($3)
	srl	$3, $3, 16
	andi	$3, $3, 1
	beqz	$3, 1f

	# Compute exception vector
	LA	$3, __start_ctx
	REG_L	$3, chain_ebase($3)
	PTR_ADDU $3, $3, 0x181		# OR ISAOnExc bit

	# Chain
	jr	$3
1:
	# Compute exception vector
	LA	$3, __start_ctx
#endif

	REG_L	$3, chain_ebase($3)
	PTR_ADDU $3, $3, 0x180

	# Chain
	jr	$3

	.set	pop
WEND(__chain_uhi_excpt)

#
# FUNCTION:	int __get_startup_BEV (void)
#
# DESCRIPTION: Return value of BEV flag saved in
#	       __register_excpt_handler.
#
WLEAF(__get_startup_BEV)
	.set	push
	.set	noat

	LA	$2, __start_ctx
	REG_L	$2, start_ctx_sr($2)
	lui	$3, %hi(SR_BEV)
	and	$2, $2, $3
	jr	$31

	.set	pop
WEND(__get_startup_BEV)


EXPORTS(__MIPS_UHI_BAD_POINTER, 32)
	.ascii "UHI: BAD POINTER\000"

#
# FUNCTION: __convert_argv_pointers
#
# DESCRIPTION: Convert 64bit pointers to 32bit.
#
#if _MIPS_SIM==_ABIO32 || _MIPS_SIM==_ABIN32
WLEAF(__convert_argv_pointers)
	/* Early out if a0 <= 0 */
	blez	a0, .Lend

	/* Verify we came from 64-bit mode */
	LA      t0, __start_ctx
	REG_L   t0, start_ctx_sr(t0)
	ext	t1, t0, SR_KX_SHIFT, 1
	beqz	t1, .Lend

	/* Set up stack pointer */
	move	t0, a0
	sll	t1, t0, 2
	addiu   t1, t1, ALSZ            /* Round to stack alignment */
	and     t1, t1, ALMASK

	PTR_SUBU sp, sp, t1
	move	t2, sp
	move	t3, a1
	li	t1, -1

.Lloop:
#if BYTE_ORDER == LITTLE_ENDIAN
	lw	t8, 0(t3)
	lw	t9, 4(t3)
#elif BYTE_ORDER == BIG_ENDIAN
	lw	t9, 0(t3)
	lw	t8, 4(t3)
#else
#error BYTE_ORDER
#endif
	/* if s1 != 0 && s1 != 0xFFFFFFFF */
	beqz	t9, .LGoodp
	beq	t9, t1, .LGoodp
	/* Overwrite bad pointer with stock bad value */
	LA	t8, __MIPS_UHI_BAD_POINTER
.LGoodp:
	sw	t8, 0(t2)

	PTR_ADDU t2, t2, 4
	PTR_ADDU t3, t3, 8
	addiu	t0, t0, -1
	bnez	t0, .Lloop

	move	a1, sp
	PTR_SUBU sp, sp, (NARGSAVE*SZARG)

	move	a2, zero
.Lend:
	jr	ra
WEND(__convert_argv_pointers)
#endif /* ABI TEST */
