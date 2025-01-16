/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#include <stdint.h>

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RTS5912_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RTS5912_H_

struct rts5912_sccon_subsys {
	uint32_t clk_grp;
	uint32_t clk_idx;
	uint32_t clk_src;
	uint32_t clk_div;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RTS591X_H_ */
