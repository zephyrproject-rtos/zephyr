/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_TI_COMMON_SOC_FRI_H_
#define ZEPHYR_SOC_TI_COMMON_SOC_FRI_H_

#include <zephyr/sys/util.h>

/**
 * @brief TI FRI (Flash Read Interface) Register Offsets
 */
#define FRI_FRDCNTL_OFFSET 0x1000 /**< Flash read control */

/* frdcntl bits */
#define FRI_FRDCNTL_TRIMENGRRWAIT GENMASK(27, 24) /**< Trim engine random read waitstate */
#define FRI_FRDCNTL_RWAIT         GENMASK(11, 8)  /**< Random read waitstate */
#define FRI_FRDCNTL_WS0_MODE      BIT(0)          /**< Force flash to 0 waitstate */

#endif /* ZEPHYR_SOC_TI_COMMON_SOC_FRI_H_ */
