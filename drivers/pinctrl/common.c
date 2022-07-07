/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

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

#ifdef CONFIG_PINCTRL_DYNAMIC
int pinctrl_update_states(struct pinctrl_dev_config *config,
			  const struct pinctrl_state *states,
			  uint8_t state_cnt)
{
	uint8_t equal = 0U;

	/* check we are inserting same number of states */
	if (config->state_cnt != state_cnt) {
		return -EINVAL;
	}

	/* check we have the same states */
	for (uint8_t i = 0U; i < state_cnt; i++) {
		for (uint8_t j = 0U; j < config->state_cnt; j++) {
			if (states[i].id == config->states[j].id) {
				equal++;
				break;
			}
		}
	}

	if (equal != state_cnt) {
		return -EINVAL;
	}

	/* replace current states */
	config->states = states;

	return 0;
}
#endif /* CONFIG_PINCTRL_DYNAMIC */
