/*
 * Copyright (c) 2024 Bootlin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nintendo_nunchuk

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/timing/timing.h>

LOG_MODULE_REGISTER(input_nunchuk, CONFIG_INPUT_LOG_LEVEL);

struct nunchuk_config {
	struct i2c_dt_spec i2c_bus;
	k_timeout_t polling_interval;
};

struct nunchuk_data {
	const struct device *dev;
	uint8_t joystick_x;
	uint8_t joystick_y;
	bool button_c;
	bool button_z;
	struct k_work_delayable work;
};

int nunchuk_read_registers(const struct device *dev, uint8_t *buffer)
{
	const struct nunchuk_config *cfg = dev->config;
	int ret;
	uint8_t value = 0;

	ret = i2c_write_dt(&cfg->i2c_bus, &value, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(10);
	ret = i2c_read_dt(&cfg->i2c_bus, buffer, 6);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

void nunchuk_poll(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct nunchuk_data *data = CONTAINER_OF(dwork, struct nunchuk_data, work);
	const struct device *dev = data->dev;
	const struct nunchuk_config *cfg = dev->config;
	uint8_t joystick_x, joystick_y;
	bool button_c, button_z;
	uint8_t buffer[6];
	int ret;

	nunchuk_read_registers(dev, buffer);

	joystick_x = buffer[0];
	if (joystick_x != data->joystick_x) {
		data->joystick_x = joystick_x;
		ret = input_report_abs(dev, INPUT_ABS_X, data->joystick_x, false, K_FOREVER);
	}

	joystick_y = buffer[1];
	if (joystick_y != data->joystick_y) {
		data->joystick_y = joystick_y;
		ret = input_report_abs(dev, INPUT_ABS_Y, data->joystick_y, false, K_FOREVER);
	}

	button_z = buffer[5] & BIT(0);
	if (button_z != data->button_z) {
		data->button_z = button_z;
		ret = input_report_key(dev, INPUT_KEY_Z, data->button_z, true, K_FOREVER);
	}

	button_c = buffer[5] & BIT(1);
	if (button_c != data->button_c) {
		data->button_c = button_c;
		ret = input_report_key(dev, INPUT_KEY_C, data->button_c, true, K_FOREVER);
	}

	k_work_reschedule(dwork, cfg->polling_interval);
}

int nunchuk_init(const struct device *dev)
{
	const struct nunchuk_config *cfg = dev->config;
	struct nunchuk_data *data = dev->data;
	int ret;

	uint8_t init_seq_1[2] = {0xf0, 0x55};
	uint8_t init_seq_2[2] = {0xfb, 0x00};
	uint8_t buffer[6];

	data->dev = dev;

	if (!i2c_is_ready_dt(&cfg->i2c_bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	/* Send the unencrypted init sequence */
	ret = i2c_write_dt(&cfg->i2c_bus, init_seq_1, 2);
	if (ret < 0) {
		LOG_ERR("I2C write failed.");
		return ret;
	}

	k_msleep(1);
	ret = i2c_write_dt(&cfg->i2c_bus, init_seq_2, 2);
	if (ret < 0) {
		return ret;
	}

	k_msleep(1);
	ret = nunchuk_read_registers(dev, buffer);
	if (ret < 0) {
		return ret;
	}

	/* Sometimes, the first read gives unexpected results, so we make another one. */
	k_msleep(1);
	ret = nunchuk_read_registers(dev, buffer);
	if (ret < 0) {
		return ret;
	}

	data->joystick_x = buffer[0];
	data->joystick_y = buffer[1];
	data->button_z = buffer[5] & BIT(0);
	data->button_c = buffer[5] & BIT(1);

	k_work_init_delayable(&data->work, nunchuk_poll);
	ret = k_work_reschedule(&data->work, cfg->polling_interval);

	return ret;
}

#define NUNCHUK_INIT(inst)                                                                         \
	static const struct nunchuk_config nunchuk_config##inst = {                                \
		.i2c_bus = I2C_DT_SPEC_INST_GET(inst),                                             \
		.polling_interval = K_MSEC(MAX(DT_INST_PROP(inst, polling_interval_ms) - 11, 10)), \
	};                                                                                         \
                                                                                                   \
	static struct nunchuk_data nunchuk_data##inst;                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &nunchuk_init, NULL, &nunchuk_data##inst,                      \
			      &nunchuk_config##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,      \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(NUNCHUK_INIT)
