/*
 * SPDX-FileCopyrightText: Copyright 2025-2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_NXP_IMX_IMX943_M7_0_SOC_H_
#define _SOC_NXP_IMX_IMX943_M7_0_SOC_H_

#include <fsl_device_registers.h>

#define SOC_CLK_NO_PARENT_CLK 0xffffffff

struct soc_clk {
	int clk_id;
	uint32_t parent_id;
	uint32_t flags;
	bool on;
	uint64_t rate;
};

#endif /* _SOC_NXP_IMX_IMX943_M7_0_SOC_H_ */
