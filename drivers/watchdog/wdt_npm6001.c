/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm6001_wdt

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

/* nPM6001 Watchdog related registers */
#define NPM6001_WDARMEDVALUE	0x54U
#define NPM6001_WDARMEDSTROBE	0x55U
#define NPM6001_WDTRIGGERVALUE0 0x56U
#define NPM6001_WDTRIGGERVALUE1 0x57U
#define NPM6001_WDTRIGGERVALUE2 0x58U
#define NPM6001_WDDATASTROBE	0x5DU
#define NPM6001_WDPWRUPVALUE	0x5EU
#define NPM6001_WDPWRUPSTROBE	0x5FU
#define NPM6001_WDKICK		0x60U
#define NPM6001_WDREQPOWERDOWN	0x62U

/* nPM6001 WDTRIGGERVALUEx ms/LSB, min/max values */
#define NPM6001_WDTRIGGERVALUE_MS_LSB 4000U
#define NPM6001_WDTRIGGERVALUE_MIN    0x2U
#define NPM6001_WDTRIGGERVALUE_MAX    0xFFFFFFU

/* nPM6001 WDPWRUPVALUE fields */
#define NPM6001_WDPWRUPVALUE_OSC_ENABLE	    BIT(0)
#define NPM6001_WDPWRUPVALUE_COUNTER_ENABLE BIT(1)
#define NPM6001_WDPWRUPVALUE_LS_ENABLE	    BIT(2)

struct wdt_npm6001_config {
	struct i2c_dt_spec bus;
};

static int wdt_npm6001_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(options);

	return 0;
}

static int wdt_npm6001_disable(const struct device *dev)
{
	const struct wdt_npm6001_config *config = dev->config;
	uint8_t buf[4] = {NPM6001_WDARMEDVALUE, 1U, NPM6001_WDARMEDSTROBE, 1U};

	return i2c_write_dt(&config->bus, buf, sizeof(buf));
}

static int wdt_npm6001_install_timeout(const struct device *dev,
				       const struct wdt_timeout_cfg *timeout)
{
	const struct wdt_npm6001_config *config = dev->config;
	uint32_t window;
	uint8_t buf[2];
	int ret;

	if (timeout->window.min != 0U) {
		return -EINVAL;
	}

	/* round-up timeout, e.g. 5s -> 8s */
	window = (((timeout->window.max + NPM6001_WDTRIGGERVALUE_MS_LSB - 1U) /
		   NPM6001_WDTRIGGERVALUE_MS_LSB) +
		  1U);
	if ((window < NPM6001_WDTRIGGERVALUE_MIN) ||
	    (window > NPM6001_WDTRIGGERVALUE_MAX)) {
		return -EINVAL;
	}

	/* enable OSC/COUNTER/LS */
	buf[0] = NPM6001_WDPWRUPVALUE;
	buf[1] = NPM6001_WDPWRUPVALUE_OSC_ENABLE |
		 NPM6001_WDPWRUPVALUE_COUNTER_ENABLE |
		 NPM6001_WDPWRUPVALUE_LS_ENABLE;
	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	buf[0] = NPM6001_WDPWRUPSTROBE;
	buf[1] = 1U;
	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	/* write trigger value */
	buf[0] = NPM6001_WDTRIGGERVALUE0;
	buf[1] = (uint8_t)window;
	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	buf[0] = NPM6001_WDTRIGGERVALUE1;
	buf[1] = (uint8_t)(window >> 8U);
	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	buf[0] = NPM6001_WDTRIGGERVALUE2;
	buf[1] = (uint8_t)(window >> 16U);
	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	buf[0] = NPM6001_WDDATASTROBE;
	buf[1] = 1U;
	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	/* arm watchdog & kick */
	buf[0] = NPM6001_WDARMEDVALUE;
	buf[1] = 1U;
	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	buf[0] = NPM6001_WDARMEDSTROBE;
	buf[1] = 1U;
	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	buf[0] = NPM6001_WDKICK;
	buf[1] = 1U;
	ret = i2c_write_dt(&config->bus, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int wdt_npm6001_feed(const struct device *dev, int channel_id)
{
	const struct wdt_npm6001_config *config = dev->config;
	uint8_t buf[2] = {NPM6001_WDKICK, 1U};

	ARG_UNUSED(channel_id);

	return i2c_write_dt(&config->bus, buf, sizeof(buf));
}

static const struct wdt_driver_api wdt_npm6001_api = {
	.setup = wdt_npm6001_setup,
	.disable = wdt_npm6001_disable,
	.install_timeout = wdt_npm6001_install_timeout,
	.feed = wdt_npm6001_feed,
};

static int wdt_npm6001_init(const struct device *dev)
{
	const struct wdt_npm6001_config *config = dev->config;

	if (!device_is_ready(config->bus.bus)) {
		return -ENODEV;
	}

	return 0;
}

#define WDT_NPM6001_DEFINE(n)                                                  \
	static const struct wdt_npm6001_config wdt_npm6001_config##n = {       \
		.bus = I2C_DT_SPEC_GET(DT_INST_PARENT(n)),                     \
	};                                                                     \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, &wdt_npm6001_init, NULL, NULL,                \
			      &wdt_npm6001_config##n, POST_KERNEL,             \
			      CONFIG_WDT_NPM6001_INIT_PRIORITY,                \
			      &wdt_npm6001_api);

DT_INST_FOREACH_STATUS_OKAY(WDT_NPM6001_DEFINE)
