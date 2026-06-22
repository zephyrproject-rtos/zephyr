/*
 * Copyright (c) 2026 GigaDevice Semiconductor Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_GD_GD32_GD32C2X1_GD32_REGS_H_
#define ZEPHYR_SOC_GD_GD32_GD32C2X1_GD32_REGS_H_

#include <zephyr/sys/util_macro.h>

/* RCU register offsets */
#define RCU_CFG0_OFFSET   0x04U
#define RCU_AHB1EN_OFFSET 0x30U
#define RCU_AHB2EN_OFFSET 0x34U
#define RCU_APB1EN_OFFSET 0x44U

#define RCU_CFG0_AHBPSC_POS  4U
#define RCU_CFG0_AHBPSC_MSK  (BIT_MASK(4) << RCU_CFG0_AHBPSC_POS)
#define RCU_CFG0_APB1PSC_POS 11U
#define RCU_CFG0_APB1PSC_MSK (BIT_MASK(3) << RCU_CFG0_APB1PSC_POS)

#endif /* ZEPHYR_SOC_GD_GD32_GD32C2X1_GD32_REGS_H_ */
