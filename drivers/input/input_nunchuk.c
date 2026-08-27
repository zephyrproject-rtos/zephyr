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
#include <zephyr/input/input_nunchuk.h>

LOG_MODULE_REGISTER(input_nunchuk, CONFIG_INPUT_LOG_LEVEL);

#define NUNCHUK_DELAY_MS  10
#define NUNCHUK_READ_SIZE 6

struct nunchuk_config {
	struct i2c_dt_spec i2c_bus;
	int polling_interval_ms;
};

struct nunchuk_data {
	struct k_work_delayable work;
	k_timeout_t interval_ms;
	const struct device *dev;
#ifdef CONFIG_INPUT_NUNCHUK_ACCELERATION
	uint16_t accel_x, accel_y, accel_z;
#endif /* CONFIG_INPUT_NUNCHUK_ACCELERATION */
	uint8_t joystick_x;
	uint8_t joystick_y;
	bool button_c;
	bool button_z;
};

static int nunchuk_read_registers(const struct device *dev, uint8_t *buffer)
{
	const struct nunchuk_config *cfg = dev->config;
	int ret;
	uint8_t value = 0;

	ret = i2c_write_dt(&cfg->i2c_bus, &value, sizeof(value));
	if (ret < 0) {
		return ret;
	}

	k_msleep(NUNCHUK_DELAY_MS);
	ret = i2c_read_dt(&cfg->i2c_bus, buffer, NUNCHUK_READ_SIZE);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void nunchuk_poll(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct nunchuk_data *data = CONTAINER_OF(dwork, struct nunchuk_data, work);
	const struct device *dev = data->dev;
	uint8_t buffer[NUNCHUK_READ_SIZE];
	uint8_t joystick_x, joystick_y;
	bool button_c, button_z;
	bool y_changed;

	nunchuk_read_registers(dev, buffer);

	joystick_x = buffer[0];
	joystick_y = buffer[1];
	y_changed = (joystick_y != data->joystick_y);

	if (joystick_x != data->joystick_x) {
		data->joystick_x = joystick_x;
		input_report_abs(dev, INPUT_ABS_X, data->joystick_x, !y_changed, K_FOREVER);
	}

	if (y_changed) {
		data->joystick_y = joystick_y;
		input_report_abs(dev, INPUT_ABS_Y, data->joystick_y, true, K_FOREVER);
	}

#ifdef CONFIG_INPUT_NUNCHUK_ACCELERATION
	uint16_t accel_x = (uint16_t)(buffer[2]) << 2 | ((buffer[5] >> 2) & 0x03);
	uint16_t accel_y = (uint16_t)(buffer[3]) << 2 | ((buffer[5] >> 4) & 0x03);
	uint16_t accel_z = (uint16_t)(buffer[4]) << 2 | ((buffer[5] >> 6) & 0x03);

	bool x_accel_changed = accel_x != data->accel_x;
	bool y_accel_changed = accel_y != data->accel_y;
	bool z_accel_changed = accel_z != data->accel_z;

	if (x_accel_changed) {
		input_report(dev, INPUT_EV_DEVICE, INPUT_NUNCHUK_ACCEL_X, accel_x,
			     !y_accel_changed && !z_accel_changed, K_FOREVER);
	}
	if (y_accel_changed) {
		input_report(dev, INPUT_EV_DEVICE, INPUT_NUNCHUK_ACCEL_Y, accel_y, !z_accel_changed,
			     K_FOREVER);
	}
	if (z_accel_changed) {
		input_report(dev, INPUT_EV_DEVICE, INPUT_NUNCHUK_ACCEL_Z, accel_z, true, K_FOREVER);
	}

	data->accel_x = accel_x;
	data->accel_y = accel_y;
	data->accel_z = accel_z;
#endif /* CONFIG_INPUT_NUNCHUK_ACCELERATION */

	button_z = buffer[5] & BIT(0);
	if (button_z != data->button_z) {
		data->button_z = button_z;
		input_report_key(dev, INPUT_BTN_Z, !data->button_z, true, K_FOREVER);
	}

	button_c = buffer[5] & BIT(1);
	if (button_c != data->button_c) {
		data->button_c = button_c;
		input_report_key(dev, INPUT_BTN_C, !data->button_c, true, K_FOREVER);
	}

	if (data->interval_ms.ticks != 0) {
		k_work_reschedule(dwork, data->interval_ms);
	}
}

int nunchuk_read(const struct device *dev)
{
	struct nunchuk_data *data = dev->data;

	if (data->interval_ms.ticks != 0) {
		return -EALREADY;
	}

	k_work_reschedule(&data->work, K_NO_WAIT);
	return 0;
}

static int nunchuk_init(const struct device *dev)
{
	const struct nunchuk_config *cfg = dev->config;
	struct nunchuk_data *data = dev->data;
	int ret;

	uint8_t init_seq_1[2] = {0xf0, 0x55};
	uint8_t init_seq_2[2] = {0xfb, 0x00};
	uint8_t buffer[NUNCHUK_READ_SIZE];

	data->dev = dev;

	data->interval_ms = K_MSEC(cfg->polling_interval_ms - (NUNCHUK_DELAY_MS + 1));

	if (!i2c_is_ready_dt(&cfg->i2c_bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c_bus.bus);
		return -ENODEV;
	}

	/* Send the unencrypted init sequence */
	ret = i2c_write_dt(&cfg->i2c_bus, init_seq_1, sizeof(init_seq_1));
	if (ret < 0) {
		LOG_ERR("I2C write failed (%d).", ret);
		return ret;
	}

	k_msleep(1);
	ret = i2c_write_dt(&cfg->i2c_bus, init_seq_2, sizeof(init_seq_2));
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
#ifdef CONFIG_INPUT_NUNCHUK_ACCELERATION
	data->accel_x = (uint16_t)(buffer[2]) << 2 | ((buffer[5] >> 2) & 0x03);
	data->accel_y = (uint16_t)(buffer[3]) << 2 | ((buffer[5] >> 4) & 0x03);
	data->accel_z = (uint16_t)(buffer[4]) << 2 | ((buffer[5] >> 6) & 0x03);
#endif /* CONFIG_INPUT_NUNCHUK_ACCELERATION */

	k_work_init_delayable(&data->work, nunchuk_poll);
	if (data->interval_ms.ticks != 0) {
		return k_work_reschedule(&data->work, data->interval_ms);
	}

	return 0;
}

#define NUNCHUK_INIT(inst)                                                                         \
	static const struct nunchuk_config nunchuk_config_##inst = {                               \
		.i2c_bus = I2C_DT_SPEC_INST_GET(inst),                                             \
		.polling_interval_ms = DT_INST_PROP(inst, polling_interval_ms),                    \
	};                                                                                         \
	BUILD_ASSERT(DT_INST_PROP(inst, polling_interval_ms) > 20 ||                               \
		     DT_INST_PROP(inst, polling_interval_ms) == 0);                                \
                                                                                                   \
	static struct nunchuk_data nunchuk_data_##inst;                                            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &nunchuk_init, NULL, &nunchuk_data_##inst,                     \
			      &nunchuk_config_##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,     \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(NUNCHUK_INIT)
