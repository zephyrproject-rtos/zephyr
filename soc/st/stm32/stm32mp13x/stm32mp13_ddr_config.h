/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ST_STM32_STM32MP13X_STM32MP13_DDR_CONFIG_H_
#define ZEPHYR_SOC_ST_STM32_STM32MP13X_STM32MP13_DDR_CONFIG_H_

#include <zephyr/devicetree.h>

#define STM32MP13_DDR_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(st_stm32mp13_ddr)
#define STM32MP13_DDR_SIZE DT_PROP(STM32MP13_DDR_NODE, st_mem_size)

#if STM32MP13_DDR_SIZE == 0x20000000U
#define DDR_TYPE_DDR3_4Gb
#elif STM32MP13_DDR_SIZE == 0x40000000U
#define DDR_TYPE_DDR3_8Gb
#else
#error "Unsupported STM32MP13 DDR size"
#endif

/* The Cube DDR source provides its own private ARRAY_SIZE definition. */
#undef ARRAY_SIZE

#endif /* ZEPHYR_SOC_ST_STM32_STM32MP13X_STM32MP13_DDR_CONFIG_H_ */
