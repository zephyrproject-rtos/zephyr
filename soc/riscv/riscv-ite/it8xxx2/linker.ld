/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/common/linker-head.ld>

/*
 * H2RAM must be aligned to 4kB due to hardware constraints, so place it in RAM
 * before any other sections are processed to ensure it goes at the beginning of
 * RAM. This ensures we don't waste a lot of memory aligning it later.
 */
.h2ram_pool (NOLOAD) : {
	. = ALIGN(0x1000);
	_h2ram_pool_start = .;
	KEEP(*(.h2ram_pool))
	_h2ram_pool_end = .;
} >RAM
_h2ram_pool_size = ABSOLUTE(_h2ram_pool_end - _h2ram_pool_start);

GROUP_START(ROMABLE_REGION)
#include <zephyr/arch/riscv/common/linker-rom.ld>
GROUP_END(ROMABLE_REGION)

GROUP_START(RAMABLE_REGION)
#include <zephyr/arch/riscv/common/linker-ram.ld>
GROUP_END(RAMABLE_REGION)

#include <zephyr/arch/riscv/common/linker-tail.ld>
