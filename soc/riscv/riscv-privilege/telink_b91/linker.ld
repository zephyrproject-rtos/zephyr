/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Telink B91 SoC
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-tool.h>

MEMORY
{
	RAMILM  (rwx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ram_ilm)), LENGTH = DT_REG_SIZE(DT_NODELABEL(ram_ilm))
}

#include <zephyr/arch/riscv/common/linker.ld>

#ifdef CONFIG_MCUBOOT
	ASSERT(__rom_region_size <= DT_REG_SIZE(DT_NODELABEL(boot_partition)), "boot_partition overflows")
#endif
