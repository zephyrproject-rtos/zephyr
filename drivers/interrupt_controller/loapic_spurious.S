/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LOAPIC spurious interrupt handler
 */

#include <kernel_structs.h>
#include <arch/x86/asm.h>

	GTEXT(_loapic_spurious_handler)

SECTION_FUNC(TEXT, _loapic_spurious_handler)
	iret

