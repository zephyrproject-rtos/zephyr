/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>

#define SWJ_NODE DT_NODELABEL(swj_port)

PINCTRL_DT_DEFINE(SWJ_NODE);

const struct pinctrl_dev_config *swj_pcfg = PINCTRL_DT_DEV_CONFIG_GET(SWJ_NODE);

/*
 * Serial Wire / JTAG port pins are enabled as part of SoC default configuration.
 * When debug access is not needed and in case power consumption performance is
 * expected, configure matching pins to analog in order to save power.
 */

static int swj_to_analog(void)
{
	int err;

	/* Set Serial Wire / JTAG port pins to analog mode */
	err = pinctrl_apply_state(swj_pcfg, PINCTRL_STATE_SLEEP);
	if (err < 0) {
		__ASSERT(0, "SWJ pinctrl setup failed");
		return err;
	}

	return 0;
}

/* Run this routine as the earliest pin configuration in the target,
 * to avoid potential conflicts with devices accessing SWJ-DG pins for
 * their own needs.
 */
SYS_INIT(swj_to_analog, PRE_KERNEL_1, CONFIG_SWJ_ANALOG_PRIORITY);
