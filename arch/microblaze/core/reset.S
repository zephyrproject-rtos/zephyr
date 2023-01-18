/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/toolchain.h>

// import
GTEXT(_start)
// export
GTEXT(__start)

	.globl _reset
	.section .vectors.reset, "ax"
	.balign 4
	.ent _reset
	.type _reset, @function
_reset:
__start:
	brai    _start
	.end _reset
