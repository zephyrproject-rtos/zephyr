/*
 * Copyright (c) 2018 Aapo Vienamo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/gpio.h>
#include <sys/printk.h>

#define VDD_PWR_CTRL_GPIO_PIN 30
#define CCS_VDD_PWR_CTRL_GPIO_PIN 10

struct pwr_ctrl_cfg {
	const char *port;
	uint32_t pin;
};

static int pwr_ctrl_init(const struct device *dev)
{
	const struct pwr_ctrl_cfg *cfg = dev->config;
	const struct device *gpio;

	gpio = device_get_binding(cfg->port);
	if (!gpio) {
		printk("Could not bind device \"%s\"\n", cfg->port);
		return -ENODEV;
	}

	gpio_pin_configure(gpio, cfg->pin, GPIO_OUTPUT_HIGH);

	k_sleep(K_MSEC(1)); /* Wait for the rail to come up and stabilize */

	return 0;
}

/*
 * The CCS811 sensor is connected to the CCS_VDD power rail, which is downstream
 * from the VDD power rail. Both of these power rails need to be enabled before
 * the sensor driver init can be performed. The VDD rail also has to be powered up
 * before the CCS_VDD rail. These checks are to enforce the power up sequence
 * constraints.
 */

#if CONFIG_BOARD_VDD_PWR_CTRL_INIT_PRIORITY <= CONFIG_GPIO_NRF_INIT_PRIORITY
#error GPIO_NRF_INIT_PRIORITY must be lower than \
	BOARD_VDD_PWR_CTRL_INIT_PRIORITY
#endif

static const struct pwr_ctrl_cfg vdd_pwr_ctrl_cfg = {
	.port = DT_LABEL(DT_NODELABEL(gpio0)),
	.pin = VDD_PWR_CTRL_GPIO_PIN,
};

DEVICE_DEFINE(vdd_pwr_ctrl_init, "", pwr_ctrl_init, NULL, NULL,
	      &vdd_pwr_ctrl_cfg,
	      POST_KERNEL, CONFIG_BOARD_VDD_PWR_CTRL_INIT_PRIORITY,
	      NULL);

#ifdef CONFIG_SENSOR

#if CONFIG_BOARD_CCS_VDD_PWR_CTRL_INIT_PRIORITY <= CONFIG_BOARD_VDD_PWR_CTRL_INIT_PRIORITY
#error BOARD_VDD_PWR_CTRL_INIT_PRIORITY must be lower than BOARD_CCS_VDD_PWR_CTRL_INIT_PRIORITY
#endif

#if CONFIG_SENSOR_INIT_PRIORITY <= CONFIG_BOARD_CCS_VDD_PWR_CTRL_INIT_PRIORITY
#error BOARD_CCS_VDD_PWR_CTRL_INIT_PRIORITY must be lower than SENSOR_INIT_PRIORITY
#endif

static const struct pwr_ctrl_cfg ccs_vdd_pwr_ctrl_cfg = {
	.port = DT_LABEL(DT_INST(0, semtech_sx1509b)),
	.pin = CCS_VDD_PWR_CTRL_GPIO_PIN,
};

DEVICE_DEFINE(ccs_vdd_pwr_ctrl_init, "", pwr_ctrl_init, NULL, NULL,
	      &ccs_vdd_pwr_ctrl_cfg, POST_KERNEL,
	      CONFIG_BOARD_CCS_VDD_PWR_CTRL_INIT_PRIORITY, NULL);

#endif
