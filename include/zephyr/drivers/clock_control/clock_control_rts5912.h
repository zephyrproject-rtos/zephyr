/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

/**
 * @file
 * @brief Clock control definitions for Realtek RTS5912 devices.
 * @ingroup clock_control_rts5912
 */

#include <stdint.h>

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RTS5912_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RTS5912_H_

/**
 * @defgroup clock_control_rts5912 Realtek RTS5912
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @brief RTS5912 system clock controller (SCCON) subsystem selector. */
struct rts5912_sccon_subsys {
	uint32_t clk_grp; /**< Clock group. */
	uint32_t clk_idx; /**< Clock index within the group. */
	uint32_t clk_src; /**< Clock source. */
	uint32_t clk_div; /**< Clock divider. */
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RTS5912_H_ */
