/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>

#include <sl_rail.h>

#define DT_DRV_COMPAT silabs_pti

struct silabs_pti_config {
	const struct pinctrl_dev_config *pcfg;
	sl_rail_pti_mode_t mode;
	uint32_t baud;
};

int silabs_pti_init(const struct device *dev)
{
	const struct silabs_pti_config *config = dev->config;
	const struct pinctrl_state *state;
	sl_rail_pti_config_t pti_config = {
		.mode = config->mode,
		.baud = config->baud,
	};
	sl_rail_status_t status;
	int err;

	/* The RAIL API to configure PTI requires GPIO port and pin as part of its configuration
	 * struct, and does the pin configuration itself internally. Create the configuration
	 * struct from the pinctrl node instead of using pinctrl_apply_state().
	 */
	err = pinctrl_lookup_state(config->pcfg, PINCTRL_STATE_DEFAULT, &state);
	if (err < 0) {
		return err;
	}
	for (int i = 0; i < state->pin_cnt; i++) {
		switch (state->pins[i].en_bit) {
		case _GPIO_FRC_ROUTEEN_DCLKPEN_SHIFT:
			pti_config.dclk_port = state->pins[i].port;
			pti_config.dclk_pin = state->pins[i].pin;
			break;
		case _GPIO_FRC_ROUTEEN_DFRAMEPEN_SHIFT:
			pti_config.dframe_port = state->pins[i].port;
			pti_config.dframe_pin = state->pins[i].pin;
			break;
		case _GPIO_FRC_ROUTEEN_DOUTPEN_SHIFT:
			pti_config.dout_port = state->pins[i].port;
			pti_config.dout_pin = state->pins[i].pin;
			break;
		default:
			return -EINVAL;
		}
	}
	status = sl_rail_config_pti(SL_RAIL_EFR32_HANDLE, &pti_config);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return -EIO;
	}

	return 0;
}

#define SILABS_PTI_INIT(idx)                                                                       \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
                                                                                                   \
	static const struct silabs_pti_config silabs_pti_config_##idx = {                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.mode = DT_INST_ENUM_IDX(idx, mode),                                               \
		.baud = DT_INST_PROP(idx, clock_frequency),                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, &silabs_pti_init, NULL, NULL, &silabs_pti_config_##idx,         \
			      POST_KERNEL, CONFIG_DEBUG_DRIVER_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(SILABS_PTI_INIT)
