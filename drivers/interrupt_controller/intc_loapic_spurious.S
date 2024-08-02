/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LOAPIC spurious interrupt handler
 */

#include <zephyr/kernel_structs.h>
#include <zephyr/arch/x86/ia32/asm.h>

	GTEXT(z_loapic_spurious_handler)

SECTION_FUNC(PINNED_TEXT, z_loapic_spurious_handler)
	iret

