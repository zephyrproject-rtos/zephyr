/*
 * Copyright (c) 2024 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT awinic_aw9523b

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/aw9523b.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>

#define AW9523B_ID_VALUE          0x23
#define AW9523B_RESET_PULSE_WIDTH 20

struct mfd_aw9523b_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
};

struct mfd_aw9523b_data {
	struct k_sem lock;
};

static int mfd_aw9523b_hw_reset(const struct device *dev)
{
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	const struct mfd_aw9523b_config *config = dev->config;
	int ret;

	if (!config->reset_gpio.port) {
		return 0;
	}

	if (!gpio_is_ready_dt(&config->reset_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

	k_busy_wait(AW9523B_RESET_PULSE_WIDTH);

	ret = gpio_pin_set_dt(&config->reset_gpio, 0);
	if (ret) {
		return ret;
	}

#endif
	return 0;
}

static int mfd_aw9523b_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct mfd_aw9523b_config *config = dev->config;
	uint8_t reg;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		ret = i2c_reg_read_byte_dt(&config->i2c, AW9523B_REG_ID, &reg);

		if (ret) {
			return ret;
		};

		if (reg != AW9523B_ID_VALUE) {
			return -EIO;
		}

		return mfd_aw9523b_hw_reset(dev);
	case PM_DEVICE_ACTION_TURN_OFF:
		/* No action needed on turn off */
		break;
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_RESUME:
		/* Suspend and resume have no impact: hub handles USB suspend automatically */
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mfd_aw9523b_init(const struct device *dev)
{
	const struct mfd_aw9523b_config *config = dev->config;
	struct mfd_aw9523b_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	return pm_device_driver_init(dev, mfd_aw9523b_pm_action);
}

struct k_sem *aw9523b_get_lock(const struct device *dev)
{
	struct mfd_aw9523b_data *data = dev->data;

	return &data->lock;
}

#define MFD_AW9523B_DEFINE(inst)                                                                   \
	static const struct mfd_aw9523b_config config##inst = {                                    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		IF_ENABLED(DT_INST_PROP_HAS_IDX(inst, reset_gpios, 0), (                           \
			   .reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                 \
		)) };                 \
                                                                                                   \
	static struct mfd_aw9523b_data data##inst;                                                 \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, mfd_aw9523b_pm_action);                                     \
	DEVICE_DT_INST_DEFINE(inst, mfd_aw9523b_init, PM_DEVICE_DT_INST_GET(inst), &data##inst,    \
			      &config##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_AW9523B_DEFINE)
