/*
 * Copyright (c) 2023-2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Telink B9x SoCs
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-tool.h>

MEMORY
{
#if !CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION

	RAMILM       (rwx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ram_ilm)),              LENGTH = DT_REG_SIZE(DT_NODELABEL(ram_ilm))

#else

    RAM_ILM_N    (rwx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ram_ilm_nonretention)), LENGTH = DT_REG_SIZE(DT_NODELABEL(ram_ilm_nonretention))
    RAM_DLM      (rwx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ram_dlm)),              LENGTH = DT_REG_SIZE(DT_NODELABEL(ram_dlm))

#endif /* CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION */
}

#include <zephyr/arch/riscv/common/linker.ld>

#ifdef CONFIG_MCUBOOT
	ASSERT(__rom_region_size <= DT_REG_SIZE(DT_NODELABEL(boot_partition)), "boot_partition overflows")
#endif
