/*
 * Copyright (c) 2025 Meta Platforms, Inc. and its affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa/coreasm.h>
#include <xtensa_s2ram.h>

.global p2sram_entry
.weak p2sram_saved_area

	.text
	.align 4
p2sram_entry:
        movi a4, p2sram_saved_area
        beqz a4, p2sram_entry_no_saved_area
        l32i a4, a4, XTENSA_S2RAM_SYSTEM_OFF_OFFSET

        callx0 a4
        ill
p2sram_entry_no_saved_area:
        retw

	.size	p2sram_entry, . - p2sram_entry
