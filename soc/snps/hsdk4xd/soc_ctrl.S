/*
 * Copyright (c) 2023 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

GTEXT(soc_reset_hook)
SECTION_FUNC(TEXT, soc_reset_hook)
	mov  r0, 1	/* disable LPB for HS4XD */
	sr   r0, [_ARC_V2_LPB_CTRL]
