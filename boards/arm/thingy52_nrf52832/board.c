/*
 * Copyright (c) 2018 Aapo Vienamo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define CCS_VDD_PWR_CTRL_GPIO_PIN 10

struct pwr_ctrl_cfg {
	const struct device *gpio_dev;
	uint32_t pin;
};

static int pwr_ctrl_init(const struct device *dev)
{
	const struct pwr_ctrl_cfg *cfg = dev->config;

	if (!device_is_ready(cfg->gpio_dev)) {
		return -ENODEV;
	}

	gpio_pin_configure(cfg->gpio_dev, cfg->pin, GPIO_OUTPUT_HIGH);

	k_sleep(K_MSEC(1)); /* Wait for the rail to come up and stabilize */

	return 0;
}


#if CONFIG_SENSOR_INIT_PRIORITY <= CONFIG_BOARD_CCS_VDD_PWR_CTRL_INIT_PRIORITY
#error BOARD_CCS_VDD_PWR_CTRL_INIT_PRIORITY must be lower than SENSOR_INIT_PRIORITY
#endif

static const struct pwr_ctrl_cfg ccs_vdd_pwr_ctrl_cfg = {
	.gpio_dev = DEVICE_DT_GET(DT_INST(0, semtech_sx1509b)),
	.pin = CCS_VDD_PWR_CTRL_GPIO_PIN,
};

DEVICE_DEFINE(ccs_vdd_pwr_ctrl_init, "", pwr_ctrl_init, NULL, NULL,
	      &ccs_vdd_pwr_ctrl_cfg, POST_KERNEL,
	      CONFIG_BOARD_CCS_VDD_PWR_CTRL_INIT_PRIORITY, NULL);
