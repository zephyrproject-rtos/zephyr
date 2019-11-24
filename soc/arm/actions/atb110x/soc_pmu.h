/*
 * Copyright (c) 2019 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file PMU configuration macros for ATB110X
 */

#ifndef _ACTIONS_SOC_PMU_H_
#define _ACTIONS_SOC_PMU_H_

#include <stdbool.h>
#include "soc_regs.h"

#define VD12_CTL			(PMU_REG_BASE + 0x00)
#define VDD_CTL				(PMU_REG_BASE + 0x04)
#define BLE_CTL				(PMU_REG_BASE + 0x48)
#define BLE_STATE			(PMU_REG_BASE + 0x4c)
#define BLE_INT_CTL			(PMU_REG_BASE + 0x50)
#define WAKE_PD				(PMU_REG_BASE + 0x54)

#define COREPLL_DBGCTL			(CMU_ANALOG_REG_BASE + 0x0008)

#define VD12_CTL_VD12PD_EN		14
#define VD12_CTL_VD12PD_SET_E		13
#define VD12_CTL_VD12PD_SET_SHIFT	12
#define VD12_CTL_VD12PD_SET_MASK	(0x3 << 12)
#define VD12_CTL_VD12_BIAS_SET_E	10
#define VD12_CTL_VD12_BIAS_SET_SHIFT	9
#define VD12_CTL_VD12_BIAS_SET_MASK	(0x3 << 9)
#define VD12_CTL_VD12_LGBIAS_EN		8
#define VD12_CTL_VD12_EN		4
#define VD12_CTL_VD12_SET_E		2
#define VD12_CTL_VD12_SET_SHIFT		0
#define VD12_CTL_VD12_SET_MASK		(0x7 << 0)

#define VDD_CTL_VDD_BIAS_S3_E		5
#define VDD_CTL_VDD_BIAS_S3_SHIFT	4
#define VDD_CTL_VDD_BIAS_S3_MASK	(0x3 << 4)
#define VDD_CTL_VDD_SET_E		3
#define VDD_CTL_VDD_SET_SHIFT		0
#define VDD_CTL_VDD_SET_MASK		(0xf << 0)

#define BLE_CTL_BLE_SWV1V_REQ		5
#define BLE_CTL_BAT_STAT_REQ		4
#define BLE_CTL_LLCC_CLK_REQ_WK_EN	2
#define BLE_CTL_BLE_SLEEP_REQ		1
#define BLE_CTL_BLE_WAKE_REQ		0

#define BLE_STATE_LLCC_CLK_O		4
#define BLE_STATE_BLE_SWV1V_O		3
#define BLE_STATE_BLE_BATSTAT_O		2
#define BLE_STATE_BLE_AWAKE_O		1
#define BLE_STATE_BLE_3V		0

#define BLE_INT_CTL_LLCC_CLK_ENABLE_REQ_INT_EN	0

#define WAKE_PD_ADC_WKPD		3
#define WAKE_PD_DEEPSLEEPPD		2
#define WAKE_PD_LLCC_CLK_ENABLE_REQ_PD	1
#define WAKE_PD_POWER_ON_PD		0

void acts_request_vd12_pd(bool enable);
void acts_request_vd12_largebias(bool enable);
void acts_request_vd12(bool enable);

enum vdd_val {
	VDD_80 = 0x0,
	VDD_85,
	VDD_90,
	VDD_95,
	VDD_100,
	VDD_105,
	VDD_110,
	VDD_115,
	VDD_120,
	VDD_125,
	VDD_130,
};

void acts_set_vdd(enum vdd_val val);

#endif /* _ACTIONS_SOC_PMU_H_	*/
