/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct38xx_gpio_alert

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_nct38xx.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#include "gpio_nct38xx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(gpio_ntc38xx, CONFIG_GPIO_LOG_LEVEL);

/* Driver config */
struct nct38xx_alert_config {
	/* Alert GPIO pin */
	const struct gpio_dt_spec irq_gpio;
	/* NCT38XX devices which share the same alert pin */
	const struct device **nct38xx_dev;
	/* Number of NCT38XX devices on the alert pin */
	uint32_t nct38xx_num;
};

/* Driver data */
struct nct38xx_alert_data {
	/* Alert handler device */
	const struct device *alert_dev;
	/* Alert pin callback */
	struct gpio_callback gpio_cb;
	/* Alert worker */
	struct k_work alert_worker;
};

static void nct38xx_alert_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	ARG_UNUSED(pins);
	struct nct38xx_alert_data *data = CONTAINER_OF(cb, struct nct38xx_alert_data, gpio_cb);

	k_work_submit(&data->alert_worker);
}

static void nct38xx_alert_worker(struct k_work *work)
{
	struct nct38xx_alert_data *const data =
		CONTAINER_OF(work, struct nct38xx_alert_data, alert_worker);
	const struct nct38xx_alert_config *const config = data->alert_dev->config;
	uint16_t alert, mask;

	do {
		/* NCT38XX device handler */
		for (int i = 0; i < config->nct38xx_num; i++) {
			/* Clear alert */
			if (nct38xx_reg_burst_read(config->nct38xx_dev[i], NCT38XX_REG_ALERT,
						   (uint8_t *)&alert, sizeof(alert))) {
				LOG_ERR("i2c access failed");
				return;
			}
			if (nct38xx_reg_burst_read(config->nct38xx_dev[i], NCT38XX_REG_ALERT_MASK,
						   (uint8_t *)&mask, sizeof(mask))) {
				LOG_ERR("i2c access failed");
				return;
			}

			alert &= mask;
			if (alert) {
				if (nct38xx_reg_burst_write(config->nct38xx_dev[i],
							    NCT38XX_REG_ALERT, (uint8_t *)&alert,
							    sizeof(alert))) {
					LOG_ERR("i2c access failed");
					return;
				}
			}

			if (alert & BIT(NCT38XX_REG_ALERT_VENDOR_DEFINDED_ALERT)) {
				nct38xx_gpio_alert_handler(config->nct38xx_dev[i]);
			}
		}
		/* While the interrupt signal is still active; we have more work to do. */
	} while (gpio_pin_get_dt(&config->irq_gpio));
}

static int nct38xx_alert_init(const struct device *dev)
{
	const struct nct38xx_alert_config *const config = dev->config;
	struct nct38xx_alert_data *const data = dev->data;
	int ret;

	/* Check NCT38XX devices are all ready. */
	for (int i = 0; i < config->nct38xx_num; i++) {
		if (!device_is_ready(config->nct38xx_dev[i])) {
			LOG_ERR("%s device not ready", config->nct38xx_dev[i]->name);
			return -ENODEV;
		}
	}

	/* Set the alert pin for handling the interrupt */
	k_work_init(&data->alert_worker, nct38xx_alert_worker);

	if (!device_is_ready(config->irq_gpio.port)) {
		LOG_ERR("%s device not ready", config->irq_gpio.port->name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT);

	gpio_init_callback(&data->gpio_cb, nct38xx_alert_callback, BIT(config->irq_gpio.pin));

	ret = gpio_add_callback(config->irq_gpio.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	gpio_pin_interrupt_configure_dt(&config->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

/* NCT38XX alert driver must be initialized after NCT38XX GPIO driver */
BUILD_ASSERT(CONFIG_GPIO_NCT38XX_ALERT_INIT_PRIORITY > CONFIG_GPIO_NCT38XX_INIT_PRIORITY);

#define NCT38XX_DEV_AND_COMMA(node_id, prop, idx)                                                  \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define NCT38XX_ALERT_DEVICE_INSTANCE(inst)                                                        \
	const struct device *nct38xx_dev_##inst[] = { DT_INST_FOREACH_PROP_ELEM(                   \
		inst, nct38xx_dev, NCT38XX_DEV_AND_COMMA) };                                       \
	static const struct nct38xx_alert_config nct38xx_alert_cfg_##inst = {                      \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                                \
		.nct38xx_dev = &nct38xx_dev_##inst[0],                                             \
		.nct38xx_num = DT_INST_PROP_LEN(inst, nct38xx_dev),                                \
	};                                                                                         \
	static struct nct38xx_alert_data nct38xx_alert_data_##inst = {                             \
		.alert_dev = DEVICE_DT_INST_GET(inst),                                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, nct38xx_alert_init, NULL, &nct38xx_alert_data_##inst,          \
			      &nct38xx_alert_cfg_##inst, POST_KERNEL,                              \
			      CONFIG_GPIO_NCT38XX_ALERT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NCT38XX_ALERT_DEVICE_INSTANCE)
