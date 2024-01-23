/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_H
#define SOC_H

#include <zephyr/sys/util_macro.h>

/* Registers block */
#define CRMU_MCU_EXTRA_EVENT_STATUS	0x40070054
#define CRMU_MCU_EXTRA_EVENT_CLEAR	0x4007005c
#define CRMU_MCU_EXTRA_EVENT_MASK	0x40070064
#define PCIE0_PERST_INTR		BIT(4)
#define PCIE0_PERST_INB_INTR		BIT(6)

#define PCIE_PERSTB_INTR_CTL_STS	0x400700f0
#define PCIE0_PERST_FE_INTR		BIT(1)
#define PCIE0_PERST_INB_FE_INTR		BIT(3)

#define PMON_LITE_PCIE_BASE			0x48180000

#define LS_ICFG_PMON_LITE_CLK_CTRL		0x482f00bc
#define PCIE_PMON_LITE_CLK_ENABLE		(BIT(0) | BIT(2))
#define LS_ICFG_PMON_LITE_SW_RESETN		0x482f0120
#define PCIE_PMON_LITE_SW_RESETN		BIT(0)

#endif
