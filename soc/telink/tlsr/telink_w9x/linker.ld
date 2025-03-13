/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Telink W91 SoCs
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-tool.h>

MEMORY
{
	RAM_ILM      (rwx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ram_ilm)),              LENGTH = DT_REG_SIZE(DT_NODELABEL(ram_ilm))
#if DT_HAS_CHOSEN(zephyr_aux_ram)
	RAM_AUX      (rwx) : ORIGIN = DT_REG_ADDR(DT_CHOSEN(zephyr_aux_ram)),          LENGTH = DT_REG_SIZE(DT_CHOSEN(zephyr_aux_ram))
#endif /* DT_HAS_CHOSEN(zephyr_aux_ram) */
}

#include <zephyr/arch/riscv/common/linker.ld>

#ifdef CONFIG_MCUBOOT
	ASSERT(__rom_region_size <= DT_REG_SIZE(DT_NODELABEL(boot_partition)), "boot_partition overflows")
#else
	ASSERT(__rom_region_end < (CONFIG_FLASH_BASE_ADDRESS + CONFIG_TELINK_W91_N22_PARTITION_ADDR), "n22_partition overlaps")
#endif
