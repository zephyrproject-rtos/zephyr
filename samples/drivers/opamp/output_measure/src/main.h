/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MAIN_H
#define MAIN_H

#include <zephyr/drivers/opamp.h>

#define OPAMP_SUPPORT_PROGRAMMABLE_GAIN				\
	DT_PROP_OR(DT_PHANDLE(DT_PATH(zephyr_user), opamp),	\
		support_programmable_gain, 0)

#if OPAMP_SUPPORT_PROGRAMMABLE_GAIN
#if defined(CONFIG_BOARD_FRDM_MCXN947) ||	\
	defined(CONFIG_BOARD_FRDM_MCXA156) ||	\
	defined(CONFIG_BOARD_LPCXPRESSO55S36)
static const enum opamp_gain test_gain[] = {
	OPAMP_GAIN_1, OPAMP_GAIN_2, OPAMP_GAIN_4
};
#endif

#endif

#endif /* MAIN_H */
