/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

_ASM_FILE_PROLOGUE

GTEXT(__aeabi_read_tp)

SECTION_FUNC(text, __aeabi_read_tp)
	/*
	 * Load TLS base address, which is stored in the TPIDRURO register, also
	 * known as the "Process ID" register. Refer to the code in z_arm_pendsv
	 * to see where this register is set.
	 */
	mrc 15, 0, r0, c13, c0, 3
	bx lr
