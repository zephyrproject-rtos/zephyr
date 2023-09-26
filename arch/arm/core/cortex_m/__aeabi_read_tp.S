/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

_ASM_FILE_PROLOGUE

GTEXT(__aeabi_read_tp)

GDATA(z_arm_tls_ptr)

SECTION_FUNC(TEXT, __aeabi_read_tp)
	/* Grab the TLS pointer and store in R0 */
	ldr r0, =z_arm_tls_ptr
	ldr r0, [r0]
	bx lr
