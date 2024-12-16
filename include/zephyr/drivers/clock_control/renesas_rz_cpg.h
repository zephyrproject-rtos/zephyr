/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZ_CPG_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZ_CPG_H_

#include "bsp_clocks.h"

struct clock_control_rz_subsys_cfg {
	fsp_ip_t ip;
	uint32_t channel;
	uint32_t clk_src;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZ_CPG_H_ */
