/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pinctrl.h>

int pinctrl_lookup_state(const struct pinctrl_dev_config *config, uint8_t id,
			 const struct pinctrl_state **state)
{
	*state = &config->states[0];
	while (*state <= &config->states[config->state_cnt - 1U]) {
		if (id == (*state)->id) {
			return 0;
		}

		(*state)++;
	}

	return -ENOENT;
}
