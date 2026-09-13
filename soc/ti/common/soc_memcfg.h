/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_TI_COMMON_SOC_MEMCFG_H_
#define ZEPHYR_SOC_TI_COMMON_SOC_MEMCFG_H_

#include <zephyr/sys/util.h>

/**
 * @brief TI MEMCFG Register Offsets
 */
#define MEMCFG_RAM_WS_CONFIG_OFFSET 0x1008 /**< RAM wait state configuration */

/* ram_ws_config bits */
#define MEMCFG_RAM_WS_CONFIG_GLXMP_2_WS_ENABLE BIT(3) /**< GLXMP_2 RAM waitstate enable */
#define MEMCFG_RAM_WS_CONFIG_GLXMP_1_WS_ENABLE BIT(2) /**< GLXMP_1 RAM waitstate enable */
#define MEMCFG_RAM_WS_CONFIG_GLXMP_0_WS_ENABLE BIT(1) /**< GLXMP_0 RAM waitstate enable */
#define MEMCFG_RAM_WS_CONFIG_ULL_WS_ENABLE     BIT(0) /**< ULL RAM waitstate enable */

#endif /* ZEPHYR_SOC_TI_COMMON_SOC_MEMCFG_H_ */
