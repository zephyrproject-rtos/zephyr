/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright (c) 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

void board_early_init_hook(void)
{
	/* Declare pin configuration state for flexio pin here */
	PINCTRL_DT_DEFINE(DT_NODELABEL(flexio));

	/* Apply pinctrl state directly, since there is no flexio device driver */
	(void)pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(flexio)),
		PINCTRL_STATE_DEFAULT);
}
