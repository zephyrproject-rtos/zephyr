/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

.global zynq_trampoline
zynq_trampoline:
	ldr r0, [pc]
	bx r0
.global zynq_trampoline_jump
zynq_trampoline_jump:
	.word
.global zynq_trampoline_end
zynq_trampoline_end:
