/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pn53x

#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "pn53x.h"

LOG_MODULE_DECLARE(nfc_pn53x, CONFIG_NFC_DRIVERS_LOG_LEVEL);

#define PN53X_I2C_READY BIT(0)

static int pn53x_i2c_write(const struct device *dev, const uint8_t *buf, size_t len)
{
	const struct pn53x_config *cfg = dev->config;
	const struct i2c_dt_spec *i2c = cfg->bus;

	return i2c_write_dt(i2c, buf, len);
}

static int pn53x_i2c_read(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct pn53x_config *cfg = dev->config;
	const struct i2c_dt_spec *i2c = cfg->bus;
	uint8_t tmp[PN53X_FRAME_MAXLEN + 1U];
	int ret;

	if (len + 1U > sizeof(tmp)) {
		return -EINVAL;
	}

	ret = i2c_read_dt(i2c, tmp, len + 1U);
	if (ret < 0) {
		return ret;
	}

	memcpy(buf, &tmp[1], len);

	return 0;
}

static int pn53x_i2c_poll_ready(const struct device *dev, k_timeout_t timeout)
{
	const struct pn53x_config *cfg = dev->config;
	const struct i2c_dt_spec *i2c = cfg->bus;
	k_timepoint_t end = sys_timepoint_calc(timeout);
	uint8_t status;

	do {
		if (i2c_read_dt(i2c, &status, sizeof(status)) == 0 &&
		    (status & PN53X_I2C_READY) != 0U) {
			return 0;
		}
		k_sleep(K_MSEC(1));
	} while (!sys_timepoint_expired(end));

	return -ETIMEDOUT;
}

static int pn53x_i2c_wait_ready(const struct device *dev, k_timeout_t timeout)
{
	const struct pn53x_config *cfg = dev->config;
	struct pn53x_data *data = dev->data;

	if (cfg->irq_gpio.port == NULL) {
		return pn53x_i2c_poll_ready(dev, timeout);
	}

	if (gpio_pin_get_dt(&cfg->irq_gpio) == 1) {
		return 0;
	}

	return (k_sem_take(&data->irq_sem, timeout) == 0) ? 0 : -ETIMEDOUT;
}

static const struct pn53x_transport_api pn53x_i2c_transport = {
	.write = pn53x_i2c_write,
	.read = pn53x_i2c_read,
	.wait_ready = pn53x_i2c_wait_ready,
};

#define PN53X_I2C_DEFINE(inst)                                                                     \
	static const struct i2c_dt_spec pn53x_i2c_bus_##inst = I2C_DT_SPEC_INST_GET(inst);         \
	static K_THREAD_STACK_DEFINE(pn53x_poll_stack_##inst, CONFIG_NFC_PN53X_POLL_STACK_SIZE);   \
	static struct pn53x_data pn53x_data_##inst;                                                \
	static const struct pn53x_config pn53x_config_##inst = {                                   \
		.transport = &pn53x_i2c_transport,                                                 \
		.bus = &pn53x_i2c_bus_##inst,                                                      \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}),                        \
		.poll_stack = pn53x_poll_stack_##inst,                                             \
		.poll_stack_size = K_THREAD_STACK_SIZEOF(pn53x_poll_stack_##inst),                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, pn53x_init_common, NULL, &pn53x_data_##inst,                   \
			      &pn53x_config_##inst, POST_KERNEL, CONFIG_NFC_INIT_PRIORITY,         \
			      &pn53x_nfc_api);

DT_INST_FOREACH_STATUS_OKAY(PN53X_I2C_DEFINE)
