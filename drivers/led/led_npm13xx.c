/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/mfd/npm13xx.h>

/* nPM13xx LED base address */
#define NPM_LED_BASE 0x0AU

/* nPM13xx LED register offsets */
#define NPM_LED_OFFSET_MODE 0x00U
#define NPM_LED_OFFSET_SET  0x03U
#define NPM_LED_OFFSET_CLR  0x04U

/* nPM13xx Channel counts */
#define NPM13XX_LED_PINS 3U

/* nPM13xx LED modes */
#define NPM_LED_HOST 2U

struct led_npm13xx_config {
	const struct device *mfd;
	uint8_t mode[NPM13XX_LED_PINS];
};

static int led_npm13xx_on(const struct device *dev, uint32_t led)
{
	const struct led_npm13xx_config *config = dev->config;

	if (led >= NPM13XX_LED_PINS) {
		return -EINVAL;
	}

	if (config->mode[led] != NPM_LED_HOST) {
		return -EPERM;
	}

	return mfd_npm13xx_reg_write(config->mfd, NPM_LED_BASE, NPM_LED_OFFSET_SET + (led * 2U),
				     1U);
}

static int led_npm13xx_off(const struct device *dev, uint32_t led)
{
	const struct led_npm13xx_config *config = dev->config;

	if (led >= NPM13XX_LED_PINS) {
		return -EINVAL;
	}

	if (config->mode[led] != NPM_LED_HOST) {
		return -EPERM;
	}

	return mfd_npm13xx_reg_write(config->mfd, NPM_LED_BASE, NPM_LED_OFFSET_CLR + (led * 2U),
				     1U);
}

static DEVICE_API(led, led_npm13xx_api) = {
	.on = led_npm13xx_on,
	.off = led_npm13xx_off,
};

static int led_npm13xx_init(const struct device *dev)
{
	const struct led_npm13xx_config *config = dev->config;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	for (uint8_t led = 0U; led < NPM13XX_LED_PINS; led++) {
		int ret = mfd_npm13xx_reg_write(config->mfd, NPM_LED_BASE,
						NPM_LED_OFFSET_MODE + led, config->mode[led]);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#define LED_NPM13XX_DEFINE(partno, n)                                                              \
	static const struct led_npm13xx_config led_##partno##_config##n = {                        \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                           \
		.mode = {DT_INST_ENUM_IDX(n, nordic_led0_mode),                                    \
			 DT_INST_ENUM_IDX(n, nordic_led1_mode),                                    \
			 DT_INST_ENUM_IDX(n, nordic_led2_mode)}};                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &led_npm13xx_init, NULL, NULL, &led_##partno##_config##n,         \
			      POST_KERNEL, CONFIG_LED_INIT_PRIORITY, &led_npm13xx_api);

#define DT_DRV_COMPAT nordic_npm1300_led
#define LED_NPM1300_DEFINE(n) LED_NPM13XX_DEFINE(npm1300, n)
DT_INST_FOREACH_STATUS_OKAY(LED_NPM1300_DEFINE)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nordic_npm1304_led
#define LED_NPM1304_DEFINE(n) LED_NPM13XX_DEFINE(npm1304, n)
DT_INST_FOREACH_STATUS_OKAY(LED_NPM1304_DEFINE)
