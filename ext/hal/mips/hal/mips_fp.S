/*
 * Copyright 2015, Imagination Technologies Limited and/or its
 *                 affiliated group companies.
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

.module hardfloat
.module doublefloat
.set nomips16
#include <mips/asm.h>
#include <mips/regdef.h>
#include <mips/m32c0.h>
#include <mips/hal.h>

#undef fp

#
# FUNCTION:	_fpctx_save
#
# DESCRIPTION:	save floating point registers to memory starting at a0
#
# RETURNS:	int
#			0:	No context saved
#			CTX_*:	Type of context stored
#
LEAF(_fpctx_save)
	PTR_S 	zero, LINKCTX_NEXT(a0)
	mfc0	t0, C0_STATUS
	li	t1, SR_CU1
	and	t1, t0, t1
	bnez	t1, 1f
	# FP not enabled, bail out
	move	v0, zero
	jr	ra

1:	# Save FP32 base
	li	t1, SR_FR
	and	t0, t0, t1
	cfc1	t2, $31
	REG_S	t2, FP32CTX_CSR(a0)
	sdc1	$f0, FP32CTX_0(a0)
	sdc1	$f2, FP32CTX_2(a0)
	sdc1	$f4, FP32CTX_4(a0)
	sdc1	$f6, FP32CTX_6(a0)
	sdc1	$f8, FP32CTX_8(a0)
	sdc1	$f10, FP32CTX_10(a0)
	sdc1	$f12, FP32CTX_12(a0)
	sdc1	$f14, FP32CTX_14(a0)
	sdc1	$f16, FP32CTX_16(a0)
	sdc1	$f18, FP32CTX_18(a0)
	sdc1	$f20, FP32CTX_20(a0)
	sdc1	$f22, FP32CTX_22(a0)
	sdc1	$f24, FP32CTX_24(a0)
	sdc1	$f26, FP32CTX_26(a0)
	sdc1	$f28, FP32CTX_28(a0)
	sdc1	$f30, FP32CTX_30(a0)
	bnez	t0, 2f
	li	v0, LINKCTX_TYPE_FP32
	REG_S	v0, LINKCTX_ID(a0)
	jr	ra

2:	# Save FP64 extra
.set	push
.set	fp=64
	sdc1	$f1, FP64CTX_1(a0)
	sdc1	$f3, FP64CTX_3(a0)
	sdc1	$f5, FP64CTX_5(a0)
	sdc1	$f7, FP64CTX_7(a0)
	sdc1	$f9, FP64CTX_9(a0)
	sdc1	$f11, FP64CTX_11(a0)
	sdc1	$f13, FP64CTX_13(a0)
	sdc1	$f15, FP64CTX_15(a0)
	sdc1	$f17, FP64CTX_17(a0)
	sdc1	$f19, FP64CTX_19(a0)
	sdc1	$f21, FP64CTX_21(a0)
	sdc1	$f23, FP64CTX_23(a0)
	sdc1	$f25, FP64CTX_25(a0)
	sdc1	$f27, FP64CTX_27(a0)
	sdc1	$f29, FP64CTX_29(a0)
	sdc1	$f31, FP64CTX_31(a0)
.set	pop
	li	v0, LINKCTX_TYPE_FP64
	REG_S	v0, LINKCTX_ID(a0)
	jr	ra
END(_fpctx_save)

#
# FUNCTION:	_fpctx_load
#
# DESCRIPTION:	load floating point registers from context chain starting at a0
#
# RETURNS:	int
#			0:	Unrecognised context
#			CTX_*:	Type of context restored
#
LEAF(_fpctx_load)
	REG_L	v0, LINKCTX_ID(a0)
	# Detect type
	li	t0, LINKCTX_TYPE_FP64
	li	t1, LINKCTX_TYPE_FP32
	li	t2, SR_CU1
	beq	v0, t0, 0f
	beq	v0, t1, 1f
	# Don't recognise this context, fail
	move	v0, zero
	jr	ra

0: 	# FP64 context
	# Enable CU1
	di	t3
	ehb
	or	t3, t3, t2
	mtc0	t3, C0_STATUS
	ehb
	# Load FP64 extra
.set	push
.set	fp=64
	ldc1	$f1, FP64CTX_1(a0)
	ldc1	$f3, FP64CTX_3(a0)
	ldc1	$f5, FP64CTX_5(a0)
	ldc1	$f7, FP64CTX_7(a0)
	ldc1	$f9, FP64CTX_9(a0)
	ldc1	$f11, FP64CTX_11(a0)
	ldc1	$f13, FP64CTX_13(a0)
	ldc1	$f15, FP64CTX_15(a0)
	ldc1	$f17, FP64CTX_17(a0)
	ldc1	$f19, FP64CTX_19(a0)
	ldc1	$f21, FP64CTX_21(a0)
	ldc1	$f23, FP64CTX_23(a0)
	ldc1	$f25, FP64CTX_25(a0)
	ldc1	$f27, FP64CTX_27(a0)
	ldc1	$f29, FP64CTX_29(a0)
	ldc1	$f31, FP64CTX_31(a0)
.set	pop
1: 	# FP32 context
	# Enable CU1
	di	t3
	ehb
	or	t3, t3, t2
	mtc0	t3, C0_STATUS
	ehb
	# Load FP32 base
	REG_L	t1, FP32CTX_CSR(a0)
	ctc1	t1, $31
	ldc1	$f0, FP32CTX_0(a0)
	ldc1	$f2, FP32CTX_2(a0)
	ldc1	$f4, FP32CTX_4(a0)
	ldc1	$f6, FP32CTX_6(a0)
	ldc1	$f8, FP32CTX_8(a0)
	ldc1	$f10, FP32CTX_10(a0)
	ldc1	$f12, FP32CTX_12(a0)
	ldc1	$f14, FP32CTX_14(a0)
	ldc1	$f16, FP32CTX_16(a0)
	ldc1	$f18, FP32CTX_18(a0)
	ldc1	$f20, FP32CTX_20(a0)
	ldc1	$f22, FP32CTX_22(a0)
	ldc1	$f24, FP32CTX_24(a0)
	ldc1	$f26, FP32CTX_26(a0)
	ldc1	$f28, FP32CTX_28(a0)
	ldc1	$f30, FP32CTX_30(a0)
	# Return CTX_FP32/64
	jr	ra
END(_fpctx_load)
