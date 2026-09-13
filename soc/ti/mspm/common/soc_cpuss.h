/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_TI_MSPM_COMMON_SOC_CPUSS_H_
#define ZEPHYR_SOC_TI_MSPM_COMMON_SOC_CPUSS_H_

#include <zephyr/devicetree.h>

/**
 * @brief MSPM CPUSS Register Offsets
 *
 * CPU subsystem registers.
 */
#define CPUSS_CTL_OFFSET 0x1300 /**< Prefetch/cache control */

/* ctl bits */
#define CPUSS_CTL_ICACHE BIT(1)

#endif /* ZEPHYR_SOC_TI_MSPM_COMMON_SOC_CPUSS_H_ */
