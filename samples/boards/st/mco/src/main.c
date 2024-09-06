/*
 * Copyright (c) 2024 Joakim Andersson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>

/* Define the pinctrl information for the MCO pin. */
PINCTRL_DT_DEFINE(DT_PATH(zephyr_user));

int main(void)
{
	/* Configure the MCO pin using pinctrl in order to set the alternate function of the pin. */
	const struct pinctrl_dev_config *pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_PATH(zephyr_user));
	(void)pinctrl_apply_state(pcfg, PINCTRL_STATE_DEFAULT);

	printk("\nMCO pin configured, end of example.\n");
	return 0;
}
