/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Titan Chen <titan.chen@realtek.com>
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>

#define SWJ_NODE DT_NODELABEL(swj_port)

PINCTRL_DT_DEFINE(SWJ_NODE);

const struct pinctrl_dev_config *swj_pcfg = PINCTRL_DT_DEV_CONFIG_GET(SWJ_NODE);

int swj_connector_init(void)
{
	int err;

	err = pinctrl_apply_state(swj_pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	return 0;
}
