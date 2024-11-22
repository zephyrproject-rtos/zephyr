/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Telink TLx SoCs
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-tool.h>

MEMORY
{
#if SOC_SERIES_RISCV_TELINK_TLX_RETENTION

    RAM_ILM_N    (rwx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ram_ilm_nonretention)), LENGTH = DT_REG_SIZE(DT_NODELABEL(ram_ilm_nonretention))

#endif /* SOC_SERIES_RISCV_TELINK_TLX_RETENTION */
}

#include <zephyr/arch/riscv/common/linker.ld>

#ifdef CONFIG_MCUBOOT
	ASSERT(__rom_region_size <= DT_REG_SIZE(DT_NODELABEL(boot_partition)), "boot_partition overflows")
#endif
