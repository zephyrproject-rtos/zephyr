/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

_ASM_FILE_PROLOGUE

GTEXT(__aeabi_read_tp)

SECTION_FUNC(text, __aeabi_read_tp)
	/*
	 * TPIDRURW will be used as a base pointer point to TLS aera.
	 */
	mrc p15, 0, r0, c13, c0, 2
	bx lr
