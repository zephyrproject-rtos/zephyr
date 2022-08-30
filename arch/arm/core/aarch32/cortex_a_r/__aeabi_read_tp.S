/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

_ASM_FILE_PROLOGUE

GTEXT(__aeabi_read_tp)

SECTION_FUNC(text, __aeabi_read_tp)
	mrc 15, 0, r0, c13, c0, 3
	bx lr
