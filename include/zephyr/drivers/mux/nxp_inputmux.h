/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MUX_NXP_INPUTMUX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MUX_NXP_INPUTMUX_H_

struct inputmux_control {
	volatile uint32_t offset;
	uint32_t mask;
	uint32_t val;
};

#define NXP_INPUTMUX_CONTROL_DEFINE(node) 						\
	static struct inputmux_control inputmux_control_##node = {			\
		.reg = (uint32_t *)DT_PHA_BY_IDX(node, mux_controls, 0, offset)		\
		.mask = (uint32_t)DT_PHA_BY_IDX(node, mux_controls, 0, mask)		\
	};

#define NXP_INPUTMUX_CONTROL_GET(node)	&inputmux_control_##node

#endif /* ZEPHYR_INCLUDE_DRIVERS_MUX_NXP_INPUTMUX_H_ */
