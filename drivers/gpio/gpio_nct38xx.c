/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct38xx_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_nct38xx.h>
#include <zephyr/drivers/mfd/nct38xx.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#include "gpio_nct38xx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_ntc38xx, CONFIG_GPIO_LOG_LEVEL);

/* Driver config */
struct gpio_nct38xx_config {
	/* Multi-function device, parent to the NCT38xx GPIO controller */
	const struct device *mfd;
	/* GPIO ports */
	const struct device **sub_gpio_dev;
	uint8_t sub_gpio_port_num;
	/* Alert handler */
	const struct device *alert_dev;
};

/* Driver data */
struct gpio_nct38xx_data {
	/* NCT38XX device */
	const struct device *dev;
	/* lock NCT38xx register access */
	struct k_sem *lock;
	/* I2C device for the MFD parent */
	const struct i2c_dt_spec *i2c_dev;
};

void nct38xx_gpio_alert_handler(const struct device *dev)
{
	const struct gpio_nct38xx_config *const config = dev->config;

	for (int i = 0; i < config->sub_gpio_port_num; i++) {
		gpio_nct38xx_dispatch_port_isr(config->sub_gpio_dev[i]);
	}
}

static int nct38xx_init_interrupt(const struct device *dev)
{
	uint16_t alert, alert_mask = 0;
	int ret = 0;
	struct gpio_nct38xx_data *data = dev->data;

	k_sem_take(data->lock, K_FOREVER);

	/* Disable all interrupt */
	if (i2c_burst_write_dt(data->i2c_dev, NCT38XX_REG_ALERT_MASK, (uint8_t *)&alert_mask,
			       sizeof(alert_mask))) {
		ret = -EIO;
		goto unlock;
	}

	/* Enable vendor-defined alert for GPIO. */
	alert_mask |= BIT(NCT38XX_REG_ALERT_MASK_VENDOR_DEFINDED_ALERT);

	/* Clear alert */
	if (i2c_burst_read_dt(data->i2c_dev, NCT38XX_REG_ALERT, (uint8_t *)&alert, sizeof(alert))) {
		ret = -EIO;
		goto unlock;
	}
	alert &= alert_mask;
	if (alert) {
		if (i2c_burst_write_dt(data->i2c_dev, NCT38XX_REG_ALERT, (uint8_t *)&alert,
				       sizeof(alert))) {
			ret = -EIO;
			goto unlock;
		}
	}

	if (i2c_burst_write_dt(data->i2c_dev, NCT38XX_REG_ALERT_MASK, (uint8_t *)&alert_mask,
			       sizeof(alert_mask))) {
		ret = -EIO;
		goto unlock;
	}

unlock:
	k_sem_give(data->lock);
	return ret;
}

static int nct38xx_gpio_init(const struct device *dev)
{
	const struct gpio_nct38xx_config *const config = dev->config;
	struct gpio_nct38xx_data *data = dev->data;

	/* Verify multi-function parent is ready */
	if (!device_is_ready(config->mfd)) {
		LOG_ERR("%s device not ready", config->mfd->name);
		return -ENODEV;
	}

	data->lock = mfd_nct38xx_get_lock_reference(config->mfd);
	data->i2c_dev = mfd_nct38xx_get_i2c_dt_spec(config->mfd);

	if (IS_ENABLED(CONFIG_GPIO_NCT38XX_ALERT)) {
		nct38xx_init_interrupt(dev);
	}

	return 0;
}

#define GPIO_NCT38XX_DEVICE_INSTANCE(inst)                                                         \
	static const struct device *sub_gpio_dev_##inst[] = {                                      \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(inst, DEVICE_DT_GET, (,))                    \
	};                                                                                         \
	static const struct gpio_nct38xx_config gpio_nct38xx_cfg_##inst = {                        \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.sub_gpio_dev = sub_gpio_dev_##inst,                                               \
		.sub_gpio_port_num = ARRAY_SIZE(sub_gpio_dev_##inst),                              \
	};                                                                                         \
	static struct gpio_nct38xx_data gpio_nct38xx_data_##inst = {                               \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, nct38xx_gpio_init, NULL, &gpio_nct38xx_data_##inst,            \
			      &gpio_nct38xx_cfg_##inst, POST_KERNEL,                               \
			      CONFIG_GPIO_NCT38XX_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NCT38XX_DEVICE_INSTANCE)

/* The nct38xx MFD parent must be initialized before this driver */
BUILD_ASSERT(CONFIG_GPIO_NCT38XX_INIT_PRIORITY > CONFIG_MFD_INIT_PRIORITY);
