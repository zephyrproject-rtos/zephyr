/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_TI_MSPM_COMMON_SOC_CPUSS_H_
#define ZEPHYR_SOC_TI_MSPM_COMMON_SOC_CPUSS_H_

#include <zephyr/devicetree.h>

/**
 * @brief MSPM CPUSS Register Layout
 *
 * CPU subsystem registers.
 */
struct mspm_cpuss_regs {
	uint8_t RESERVED0[0x1300]; /**< Reserved, offset: 0x0000 - 0x1300 */
	volatile uint32_t CTL;     /**< Prefetch/cache control, offset: 0x1300 */
};

#define MSPM_CPUSS_NODE DT_NODELABEL(cpuss)
#define MSPM_CPUSS_ADDR DT_REG_ADDR(MSPM_CPUSS_NODE)
#define MSPM_CPUSS_REGS ((volatile struct mspm_cpuss_regs *)(MSPM_CPUSS_ADDR))

/* CTL bits */
#define CPUSS_CTL_ICACHE BIT(1)

#endif /* ZEPHYR_SOC_TI_MSPM_COMMON_SOC_CPUSS_H_ */
