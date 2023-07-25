/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm1300_led

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/mfd/npm1300.h>

/* nPM1300 LED base address */
#define NPM_LED_BASE 0x0AU

/* nPM1300 LED register offsets */
#define NPM_LED_OFFSET_MODE 0x00U
#define NPM_LED_OFFSET_SET  0x03U
#define NPM_LED_OFFSET_CLR  0x04U

/* nPM1300 Channel counts */
#define NPM1300_LED_PINS 3U

/* nPM1300 LED modes */
#define NPM_LED_HOST 2U

struct led_npm1300_config {
	const struct device *mfd;
	uint8_t mode[NPM1300_LED_PINS];
};

static int led_npm1300_on(const struct device *dev, uint32_t led)
{
	const struct led_npm1300_config *config = dev->config;

	if (led >= NPM1300_LED_PINS) {
		return -EINVAL;
	}

	if (config->mode[led] != NPM_LED_HOST) {
		return -EPERM;
	}

	return mfd_npm1300_reg_write(config->mfd, NPM_LED_BASE, NPM_LED_OFFSET_SET + (led * 2U),
				     1U);
}

static int led_npm1300_off(const struct device *dev, uint32_t led)
{
	const struct led_npm1300_config *config = dev->config;

	if (led >= NPM1300_LED_PINS) {
		return -EINVAL;
	}

	if (config->mode[led] != NPM_LED_HOST) {
		return -EPERM;
	}

	return mfd_npm1300_reg_write(config->mfd, NPM_LED_BASE, NPM_LED_OFFSET_CLR + (led * 2U),
				     1U);
}

static const struct led_driver_api led_npm1300_api = {
	.on = led_npm1300_on,
	.off = led_npm1300_off,
};

static int led_npm1300_init(const struct device *dev)
{
	const struct led_npm1300_config *config = dev->config;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	for (uint8_t led = 0U; led < NPM1300_LED_PINS; led++) {
		int ret = mfd_npm1300_reg_write(config->mfd, NPM_LED_BASE,
						NPM_LED_OFFSET_MODE + led, config->mode[led]);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#define LED_NPM1300_DEFINE(n)                                                                      \
	static const struct led_npm1300_config led_npm1300_config##n = {                           \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.mode = {DT_INST_ENUM_IDX(n, nordic_led0_mode),                                    \
			 DT_INST_ENUM_IDX(n, nordic_led1_mode),                                    \
			 DT_INST_ENUM_IDX(n, nordic_led2_mode)}};                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &led_npm1300_init, NULL, NULL, &led_npm1300_config##n,            \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &led_npm1300_api);

DT_INST_FOREACH_STATUS_OKAY(LED_NPM1300_DEFINE)
