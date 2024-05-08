/*
 * Copyright 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adp5585

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/mfd/adp5585.h>

LOG_MODULE_REGISTER(adp5585, CONFIG_GPIO_LOG_LEVEL);

static int mfd_adp5585_software_reset(const struct device *dev)
{
	const struct mfd_adp5585_config *config = dev->config;
	int ret = 0;

	/** Set CONFIG to gpio by default */
	uint8_t pin_config_buf[] = { ADP5585_PIN_CONFIG_A, 0x00U, 0x00U };

	ret = i2c_write_dt(&config->i2c_bus, pin_config_buf, sizeof(pin_config_buf));
	if (ret) {
		goto out;
	}

out:
	if (ret) {
		LOG_ERR("%s: software reset failed: %d", dev->name, ret);
	}
	return ret;
}

static void mfd_adp5585_int_gpio_handler(const struct device *dev, struct gpio_callback *gpio_cb,
					 uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct mfd_adp5585_data *data = CONTAINER_OF(gpio_cb, struct mfd_adp5585_data, int_gpio_cb);

	k_work_submit(&data->work);
}

static void mfd_adp5585_work_handler(struct k_work *work)
{
	struct mfd_adp5585_data *data = CONTAINER_OF(work, struct mfd_adp5585_data, work);
	const struct mfd_adp5585_config *config = data->dev->config;
	uint8_t reg_int_status;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);
	/* Read Interrput Flag */
	if (ret == 0) {
		ret = i2c_reg_read_byte_dt(&config->i2c_bus, ADP5585_INT_STATUS, &reg_int_status);
	}
	/* Clear Interrput Flag */
	if (ret == 0) {
		ret = i2c_reg_write_byte_dt(&config->i2c_bus, ADP5585_INT_STATUS, reg_int_status);
	}

	k_sem_give(&data->lock);

#ifdef CONFIG_GPIO_ADP5585
	if ((reg_int_status & ADP5585_INT_GPI) && device_is_ready(data->child.gpio_dev)) {
		(void)gpio_adp5585_irq_handler(data->child.gpio_dev);
	}
#endif /* CONFIG_GPIO_ADP5585 */
}

static int mfd_adp5585_init(const struct device *dev)
{
	const struct mfd_adp5585_config *config = dev->config;
	struct mfd_adp5585_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		return -ENODEV;
	}

	/* reset gpio can be left float */
	if (gpio_is_ready_dt(&config->reset_gpio)) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("%s: configure reset pin failed: %d", dev->name, ret);
			return ret;
		}
	} else {
		LOG_WRN("%s: reset pin not configured", dev->name);
	}

	ret = mfd_adp5585_software_reset(dev);
	if (ret) {
		return ret;
	}

	if (gpio_is_ready_dt(&config->nint_gpio)) {
		ret = gpio_pin_configure_dt(&config->nint_gpio, GPIO_INPUT);
		if (ret < 0) {
			return ret;
		}
		ret = gpio_pin_interrupt_configure_dt(&config->nint_gpio, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret != 0) {
			LOG_ERR("%s: failed to configure INT interrupt: %d", dev->name, ret);
			return ret;
		}

		gpio_init_callback(&data->int_gpio_cb, mfd_adp5585_int_gpio_handler,
				   BIT(config->nint_gpio.pin));
		ret = gpio_add_callback_dt(&config->nint_gpio, &data->int_gpio_cb);
		if (ret != 0) {
			LOG_ERR("%s: failed to add INT callback: %d", dev->name, ret);
			return ret;
		}
	} else {
		LOG_WRN("%s: nint pin not configured", dev->name);
	}

	LOG_DBG("%s: init ok\r\n", dev->name);

	return 0;
}

#define MFD_ADP5585_DEFINE(inst)                                                                   \
	static const struct mfd_adp5585_config mfd_adp5585_config_##inst = {                       \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.nint_gpio = GPIO_DT_SPEC_INST_GET_OR(n, nint_gpios, {0}),                         \
		.i2c_bus = I2C_DT_SPEC_INST_GET(inst),                                             \
	};                                                                                         \
	static struct mfd_adp5585_data mfd_adp5585_data_##inst = {                                 \
		.work = Z_WORK_INITIALIZER(mfd_adp5585_work_handler),                              \
		.lock = Z_SEM_INITIALIZER(mfd_adp5585_data_##inst.lock, 1, 1),                     \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_adp5585_init, NULL, &mfd_adp5585_data_##inst,              \
			      &mfd_adp5585_config_##inst, POST_KERNEL,                             \
			      CONFIG_MFD_ADP5585_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_ADP5585_DEFINE);
