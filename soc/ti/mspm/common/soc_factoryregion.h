/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_TI_MSPM_COMMON_SOC_FACTORYREGION_H_
#define ZEPHYR_SOC_TI_MSPM_COMMON_SOC_FACTORYREGION_H_

#include <zephyr/devicetree.h>

/**
 * @brief MSPM Factory Region Register Offsets
 *
 * Read-only flash region containing factory-programmed calibration data.
 * Must disable CPUSS instruction cache before accessing (see soc_cpuss.h).
 */
#if defined(CONFIG_SOC_SERIES_MSPM33C)
#define FACTORY_PLLSTARTUP0_4_8_OFFSET   0x20 /**< PLL param0 4-8 MHz */
#define FACTORY_PLLSTARTUP1_4_8_OFFSET   0x24 /**< PLL param1 4-8 MHz */
#define FACTORY_PLLSTARTUP0_8_16_OFFSET  0x28 /**< PLL param0 8-16 MHz */
#define FACTORY_PLLSTARTUP1_8_16_OFFSET  0x2C /**< PLL param1 8-16 MHz */
#define FACTORY_PLLSTARTUP0_16_32_OFFSET 0x30 /**< PLL param0 16-32 MHz */
#define FACTORY_PLLSTARTUP1_16_32_OFFSET 0x34 /**< PLL param1 16-32 MHz */
#define FACTORY_PLLSTARTUP0_32_48_OFFSET 0x38 /**< PLL param0 32-48 MHz */
#define FACTORY_PLLSTARTUP1_32_48_OFFSET 0x3C /**< PLL param1 32-48 MHz */

#define FACTORY_SYSPLLPARAM2_OFFSET  0x54 /**< PLL param2 */
#define FACTORY_SYSPLLLDOCTL_OFFSET  0x58 /**< PLL LDO ctl */
#define FACTORY_SYSPLLLDOPROG_OFFSET 0x5C /**< PLL LDO VOUT PROG */
#else
#define FACTORY_PLLSTARTUP0_4_8_OFFSET   0x1C /**< PLL param0 4-8 MHz */
#define FACTORY_PLLSTARTUP1_4_8_OFFSET   0x20 /**< PLL param1 4-8 MHz */
#define FACTORY_PLLSTARTUP0_8_16_OFFSET  0x24 /**< PLL param0 8-16 MHz */
#define FACTORY_PLLSTARTUP1_8_16_OFFSET  0x28 /**< PLL param1 8-16 MHz */
#define FACTORY_PLLSTARTUP0_16_32_OFFSET 0x2C /**< PLL param0 16-32 MHz */
#define FACTORY_PLLSTARTUP1_16_32_OFFSET 0x30 /**< PLL param1 16-32 MHz */
#define FACTORY_PLLSTARTUP0_32_48_OFFSET 0x34 /**< PLL param0 32-48 MHz */
#define FACTORY_PLLSTARTUP1_32_48_OFFSET 0x38 /**< PLL param1 32-48 MHz */
#endif

#endif /* ZEPHYR_SOC_TI_MSPM_COMMON_SOC_FACTORYREGION_H_ */
