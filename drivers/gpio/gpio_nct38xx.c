/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct38xx_gpio

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/gpio/gpio_nct38xx.h>
#include <kernel.h>
#include <sys/util_macro.h>

#include "gpio_nct38xx.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_ntc38xx, CONFIG_GPIO_LOG_LEVEL);

/* Driver convenience defines */
#define DRV_CONFIG(dev) ((const struct gpio_nct38xx_config *)(dev)->config)
#define DRV_DATA(dev) ((struct gpio_nct38xx_data *)(dev)->data)

void nct38xx_gpio_alert_handler(const struct device *dev)
{
	const struct gpio_nct38xx_config *const config = DRV_CONFIG(dev);

	for (int i = 0; i < config->sub_gpio_port_num; i++) {
		gpio_nct38xx_dispatch_port_isr(config->sub_gpio_dev[i]);
	}
}

static int nct38xx_init_interrupt(const struct device *dev)
{
	uint16_t alert, alert_mask = 0;

	/* Disable all interrupt */
	if (nct38xx_reg_burst_write(dev, NCT38XX_REG_ALERT_MASK, (uint8_t *)&alert_mask,
				    sizeof(alert_mask))) {
		return -EIO;
	}

	/* Enable vendor-defined alert for GPIO. */
	alert_mask |= BIT(NCT38XX_REG_ALERT_MASK_VENDOR_DEFINDED_ALERT);

	/* Clear alert */
	if (nct38xx_reg_burst_read(dev, NCT38XX_REG_ALERT, (uint8_t *)&alert, sizeof(alert))) {
		return -EIO;
	}
	alert &= alert_mask;
	if (alert) {
		if (nct38xx_reg_burst_write(dev, NCT38XX_REG_ALERT, (uint8_t *)&alert,
					    sizeof(alert))) {
			return -EIO;
		}
	}

	if (nct38xx_reg_burst_write(dev, NCT38XX_REG_ALERT_MASK, (uint8_t *)&alert_mask,
				    sizeof(alert_mask))) {
		return -EIO;
	}

	return 0;
}

static int nct38xx_gpio_init(const struct device *dev)
{
	const struct gpio_nct38xx_config *const config = DRV_CONFIG(dev);

	/* Check I2C is ready  */
	if (!device_is_ready(config->i2c_dev.bus)) {
		LOG_ERR("%s device not ready", config->i2c_dev.bus->name);
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_GPIO_NCT38XX_INTERRUPT)) {
		nct38xx_init_interrupt(dev);
	}

	return 0;
}

#define GPIO_DEV_AND_COMMA(node_id) DEVICE_DT_GET(node_id),

#define GPIO_NCT38XX_DEVICE_INSTANCE(inst)                                                         \
	static const struct device *sub_gpio_dev_##inst[] = { DT_FOREACH_CHILD_STATUS_OKAY(        \
		DT_DRV_INST(inst), GPIO_DEV_AND_COMMA) };                                          \
	static const struct gpio_nct38xx_config gpio_nct38xx_cfg_##inst = {                        \
		.i2c_dev = I2C_DT_SPEC_INST_GET(inst),                                             \
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
