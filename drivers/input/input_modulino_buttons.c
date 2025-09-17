/*
 * Copyright 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arduino_modulino_buttons

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(modulino_buttons, CONFIG_INPUT_LOG_LEVEL);

#define MODULINO_NUM_BUTTONS 3

struct modulino_buttons_config {
	struct i2c_dt_spec bus;
	uint32_t poll_period_ms;
	uint32_t zephyr_code[MODULINO_NUM_BUTTONS];
};

struct modulino_buttons_data {
	const struct device *dev;
	struct k_work_delayable poll_work;
	uint8_t prev_state[MODULINO_NUM_BUTTONS];
};

static void modulino_buttons_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct modulino_buttons_data *data = CONTAINER_OF(
			dwork, struct modulino_buttons_data, poll_work);
	const struct device *dev = data->dev;
	const struct modulino_buttons_config *cfg = dev->config;
	int ret;
	uint8_t buf[MODULINO_NUM_BUTTONS + 1];

	ret = i2c_read_dt(&cfg->bus, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("i2c read error: %d", ret);
		goto out;
	}

	for (uint8_t i = 0; i < MODULINO_NUM_BUTTONS; i++) {
		uint8_t state = buf[i + 1];

		if (data->prev_state[i] != state) {
			input_report_key(dev, cfg->zephyr_code[i], state, true, K_FOREVER);
		}
	}

	memcpy(data->prev_state, &buf[1], sizeof(data->prev_state));

out:
	k_work_reschedule(dwork, K_MSEC(cfg->poll_period_ms));
}

static int modulino_buttons_init(const struct device *dev)
{
	const struct modulino_buttons_config *cfg = dev->config;
	struct modulino_buttons_data *data = dev->data;

	data->dev = dev;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	k_work_init_delayable(&data->poll_work, modulino_buttons_handler);
	k_work_reschedule(&data->poll_work, K_MSEC(cfg->poll_period_ms));

	return 0;
}

#define MODULINO_BUTTONS_INIT(inst)							\
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, zephyr_codes) == MODULINO_NUM_BUTTONS,	\
		     "zephyr,codes must specify three key codes");			\
											\
	static const struct modulino_buttons_config modulino_buttons_cfg_##inst = {	\
		.bus = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),				\
		.poll_period_ms = DT_INST_PROP(inst, poll_period_ms),			\
		.zephyr_code = DT_INST_PROP(inst, zephyr_codes),			\
	};										\
											\
	static struct modulino_buttons_data modulino_buttons_data_##inst;		\
											\
	DEVICE_DT_INST_DEFINE(inst, modulino_buttons_init, NULL,			\
			      &modulino_buttons_data_##inst,				\
			      &modulino_buttons_cfg_##inst,				\
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);


DT_INST_FOREACH_STATUS_OKAY(MODULINO_BUTTONS_INIT)
