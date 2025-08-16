/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MAIN_H
#define MAIN_H

#include <zephyr/drivers/opamp.h>

#if defined(CONFIG_BOARD_FRDM_MCXN947) || defined(CONFIG_BOARD_FRDM_MCXA156) ||	\
	defined(CONFIG_BOARD_LPCXPRESSO55S36)
struct opamp_gain_cfg gain_cfg[] = {
	[0] = {
			.inverting_gain = 1,
			.non_inverting_gain = 1,
		},

	[1] = {
			.inverting_gain = 2,
			.non_inverting_gain = 2,
		},

	[2] = {
			.inverting_gain = 4,
			.non_inverting_gain = 4,
		},
};
#endif

#if defined(CONFIG_BOARD_FRDM_MCXA166) || defined(CONFIG_BOARD_FRDM_MCXA276)
struct opamp_gain_cfg gain_cfg[] = {
	[0] = {
			.inverting_gain = 10,
			.non_inverting_gain = 10,
		},
};
#endif

#endif /* MAIN_H */
