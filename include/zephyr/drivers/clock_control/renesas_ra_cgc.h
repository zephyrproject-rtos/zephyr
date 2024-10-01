/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RA_CGC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RA_CGC_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/ra_clock.h>

struct clock_control_ra_pclk_cfg {
	uint32_t clk_src;
	uint32_t clk_div;
};

struct clock_control_ra_subsys_cfg {
	uint32_t mstp;
	uint32_t stop_bit;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RA_CGC_H_ */
