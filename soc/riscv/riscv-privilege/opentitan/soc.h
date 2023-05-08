/*
 * Copyright (c) 2023 Rivos Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __RISCV_OPENTITAN_SOC_H_
#define __RISCV_OPENTITAN_SOC_H_

#include <soc_common.h>
#include <zephyr/devicetree.h>

/* Ibex timer registers. */
#define RV_TIMER_CTRL_REG_OFFSET        0x004
#define RV_TIMER_INTR_ENABLE_REG_OFFSET 0x100
#define RV_TIMER_CFG0_REG_OFFSET        0x10c
#define RV_TIMER_CFG0_PRESCALE_MASK     0xfff
#define RV_TIMER_CFG0_PRESCALE_OFFSET   0
#define RV_TIMER_CFG0_STEP_MASK         0xff
#define RV_TIMER_CFG0_STEP_OFFSET       16
#define RV_TIMER_LOWER0_OFFSET          0x110
#define RV_TIMER_COMPARE_LOWER0_OFFSET  0x118

#endif /* __RISCV_OPENTITAN_SOC_H_ */
