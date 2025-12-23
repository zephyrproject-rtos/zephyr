/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_NXP_IMX_IMX943_M33_SOC_H_
#define _SOC_NXP_IMX_IMX943_M33_SOC_H_

#include <stdbool.h>
#include <fsl_device_registers.h>
#include <soc_common.h>

#define NXP_XCACHE_INSTR M33S_CACHE_CTRLPC
#define NXP_XCACHE_DATA M33S_CACHE_CTRLPS

struct soc_clk {
	int clk_id;
	uint32_t parent_id;
	uint32_t flags;
	bool on;
	uint64_t rate;
};

#endif /* _SOC_NXP_IMX_IMX943_M33_SOC_H_ */
