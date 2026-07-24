/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_TI_MSPM_COMMON_SOC_FACTORYREGION_H_
#define ZEPHYR_SOC_TI_MSPM_COMMON_SOC_FACTORYREGION_H_

#include <zephyr/devicetree.h>

/**
 * @brief MSPM Factory Region Register Layout
 *
 * Read-only flash region containing factory-programmed calibration data.
 * Must disable CPUSS instruction cache before dereferencing (see soc_cpuss.h).
 */
struct mspm_factoryregion_regs {
#if defined(CONFIG_SOC_MSPM33_DEVICE_FAMILY_C321X)
	uint8_t RESERVED0[0x20]; /* Reserved, offset: 0x0000 - 0x0020 */
#else
	uint8_t RESERVED0[0x1C]; /* Reserved, offset: 0x0000 - 0x001C */
#endif

	volatile const uint32_t PLLSTARTUP0_4_8;   /* PLL param0 4-8 MH */
	volatile const uint32_t PLLSTARTUP1_4_8;   /* PLL param1 4-8 MHz */
	volatile const uint32_t PLLSTARTUP0_8_16;  /* PLL param0 8-16 MHz */
	volatile const uint32_t PLLSTARTUP1_8_16;  /* PLL param1 8-16 MHz */
	volatile const uint32_t PLLSTARTUP0_16_32; /* PLL param0 16-32 MHz */
	volatile const uint32_t PLLSTARTUP1_16_32; /* PLL param1 16-32 MHz */
	volatile const uint32_t PLLSTARTUP0_32_48; /* PLL param0 32-48 MHz */
	volatile const uint32_t PLLSTARTUP1_32_48; /* PLL param1 32-48 MHz */

#if defined(CONFIG_SOC_MSPM33_DEVICE_FAMILY_C321X)
	uint8_t RESERVED1[0x14];               /* Reserved, offset: 0x0040 - 0x0054 */
	volatile const uint32_t SYSPLLPARAM2;  /* PLL param2, offset: 0x0054 */
	volatile const uint32_t SYSPLLLDOCTL;  /* PLL LDO CTL, offset: 0x0058 */
	volatile const uint32_t SYSPLLLDOPROG; /* PLL LDO VOUT PROG, offset: 0x005C */
#endif
};

#define MSPM_FACTORY_NODE DT_NODELABEL(factoryregion)
#define MSPM_FACTORY_ADDR DT_REG_ADDR(MSPM_FACTORY_NODE)
#define MSPM_FACTORY_REGS ((volatile struct mspm_factoryregion_regs *)MSPM_FACTORY_ADDR)

#endif /* ZEPHYR_SOC_TI_MSPM_COMMON_SOC_FACTORYREGION_H_ */
