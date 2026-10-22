/*
 * Copyright (c) 2026, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

	.section	.iram1, "ax"
	.align      4
	.global     l8ui_a0
	.type       l8ui_a0, @function
l8ui_a0:
	entry a1, 32
	mov a15, a0
	l8ui a0, a2, 0
	mov a2, a0
	mov a0, a15
	retw

	.section	.iram1, "ax"
	.align      4
	.global     l16ui_a1
	.type       l16ui_a1, @function
l16ui_a1:
	entry a1, 32
	mov a15, a1
	l16ui a1, a2, 0
	mov a2, a1
	mov a1, a15
	retw

	.section	.iram1, "ax"
	.align      4
	.global     s16i_a0
	.type       s16i_a0, @function
s16i_a0:
	entry a1, 32
	mov a15, a0
	mov a0, a3
	s16i a0, a2, 0
	mov a0, a15
	retw

	.section	.iram1, "ax"
	.align      4
	.global     s32in_a1
	.type       s32in_a1, @function
s32in_a1:
	entry a1, 32
	mov a15, a1
	mov a1, a3
	s32i.n a1, a2, 0
	mov a1, a15
	retw
