/*
 * Copyright (c) 2024 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8801_mfd

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/mfd_ite_it8801.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mfd_ite_it8801, CONFIG_MFD_LOG_LEVEL);

struct mfd_it8801_config {
	const struct i2c_dt_spec i2c_dev;
	/* Alert GPIO pin */
	const struct gpio_dt_spec irq_gpios;
};

struct mfd_it8801_data {
	struct k_work gpio_isr_worker;
	/* Alert pin callback */
	struct gpio_callback gpio_cb;
	/* MFD sub device */
	const struct device *sub_mfd_dev;
	it8801_callback_handler_t mfd_cb;
};

static int it8801_check_vendor_id(const struct device *dev)
{
	const struct mfd_it8801_config *config = dev->config;
	int i, ret;
	uint8_t val;

	/*  Verify vendor ID registers(16-bits). */
	for (i = 0; i < ARRAY_SIZE(it8801_id_verify); i++) {
		ret = i2c_reg_read_byte_dt(&config->i2c_dev, it8801_id_verify[i].reg, &val);

		if (ret != 0) {
			LOG_ERR("Failed to read vendoer ID (ret %d)", ret);
			return ret;
		}

		if (val != it8801_id_verify[i].chip_id) {
			LOG_ERR("The IT8801 vendor ID is wrong");
			return -ENODEV;
		}
	}

	return ret;
}

void it8801_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(pins);
	struct mfd_it8801_data *data = CONTAINER_OF(cb, struct mfd_it8801_data, gpio_cb);

	k_work_submit(&data->gpio_isr_worker);
}

void mfd_it8801_register_interrupt_callback(const struct device *mfd, const struct device *child,
					    it8801_callback_handler_t cb)
{
	struct mfd_it8801_data *data = mfd->data;

	/* Save the sub node device of mfd for the callback function */
	data->sub_mfd_dev = child;
	data->mfd_cb = cb;
}

static void it8801_gpio_alert_worker(struct k_work *work)
{
	struct mfd_it8801_data *data = CONTAINER_OF(work, struct mfd_it8801_data, gpio_isr_worker);

	data->mfd_cb(data->sub_mfd_dev);
}

static int mfd_it8801_init(const struct device *dev)
{
	const struct mfd_it8801_config *config = dev->config;
	struct mfd_it8801_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c_dev)) {
		LOG_ERR("I2C bus %s is not ready", config->i2c_dev.bus->name);
		return -ENODEV;
	}

	/*  Verify Vendor ID registers. */
	ret = it8801_check_vendor_id(dev);
	if (ret) {
		LOG_ERR("Failed to read IT8801 vendor id %x", ret);
		return ret;
	}

	k_work_init(&data->gpio_isr_worker, it8801_gpio_alert_worker);

	/* Alert response enable */
	ret = i2c_reg_write_byte_dt(&config->i2c_dev, IT8801_REG_SMBCR, IT8801_REG_MASK_ARE);
	if (ret != 0) {
		LOG_ERR("Failed to initialization setting (ret %d)", ret);
		return ret;
	}

	gpio_pin_configure_dt(&config->irq_gpios, GPIO_INPUT);

	/* Initialize GPIO interrupt callback */
	gpio_init_callback(&data->gpio_cb, it8801_gpio_callback, BIT(config->irq_gpios.pin));

	ret = gpio_add_callback(config->irq_gpios.port, &data->gpio_cb);
	if (ret != 0) {
		LOG_ERR("Failed to add INT callback: %d", ret);
		return ret;
	}
	gpio_pin_interrupt_configure_dt(&config->irq_gpios, GPIO_INT_MODE_EDGE | GPIO_INT_TRIG_LOW);

	return 0;
}

#define MFD_IT8801_DEFINE(inst)                                                                    \
	static struct mfd_it8801_data it8801_data_##inst;                                          \
	static const struct mfd_it8801_config it8801_cfg_##inst = {                                \
		.i2c_dev = I2C_DT_SPEC_INST_GET(inst),                                             \
		.irq_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}),                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, mfd_it8801_init, NULL, &it8801_data_##inst,                    \
			      &it8801_cfg_##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_IT8801_DEFINE)
