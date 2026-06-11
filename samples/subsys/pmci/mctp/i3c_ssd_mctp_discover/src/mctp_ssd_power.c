/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pmci/mctp/mctp_i3c_controller.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(mctp_get_eid, CONFIG_APP_LOG_LEVEL);

/* Bus enumeration if successful takes around 10 ms
 * but SSD low power exit may take up-to 100 ms
 */
#define MAX_I3C_INIT_RETRIES	3U
#define I3C_INIT_DELAY_RETRY_MS	40U
#define SET_MWL_DELAY_MS	700U

/* SSD GPIO power (generic phandle under /zephyr,user) */
#define SSD_PWR_NODE DT_PATH(zephyr_user)
#if DT_NODE_HAS_PROP(SSD_PWR_NODE, ssd_pwr_gpios)
static const struct gpio_dt_spec ssd_pwr_gpio = GPIO_DT_SPEC_GET(SSD_PWR_NODE, ssd_pwr_gpios);
#endif /* DT_NODE_HAS_PROP(SSD_PWR_NODE, ssd_pwr_gpios) */

#define I3C_DEV_BUS			DT_ALIAS(ssd_i3c_bus)
#define MCTP_ENDPOINT0_NODE		DT_NODELABEL(mctp_endpoint0)
static const struct device *const i3c_bus_dev = DEVICE_DT_GET(I3C_DEV_BUS);


void mctp_ssd_power_init(void)
{
#if DT_NODE_HAS_PROP(SSD_PWR_NODE, ssd_pwr_gpios)
	int ret;

	LOG_DBG("Initialize SSD power GPIO %d", ssd_pwr_gpio.pin);
	if (!gpio_is_ready_dt(&ssd_pwr_gpio)) {
		return;
	}

	ret = gpio_pin_configure_dt(&ssd_pwr_gpio, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("Unable to config %d: %d", ssd_pwr_gpio.pin, ret);
		return;
	}

	ret = gpio_pin_set_dt(&ssd_pwr_gpio, 0);
	if (ret) {
		LOG_ERR("Unable to initialize %d", ssd_pwr_gpio.pin);
		return;
	}
#endif
}

void mctp_i3c_initialization(void)
{
	if (DT_PROP_OR(I3C_DEV_BUS, zephyr_deferred_init, 0U) == 0) {
		LOG_WRN("I3C deferred not needed, skipping initialization");
		return;
	}

	LOG_DBG("Deferring I3C device init");

	bool loaded = false;
	int retries = 0;
	int ret = 0;
	struct i3c_config_controller ctrl_config;

	k_msleep(I3C_INIT_DELAY_RETRY_MS);

	do {
		loaded = device_is_ready(i3c_bus_dev);
		if (loaded) {
			LOG_INF("I3C device is ready");
			break;
		}

		LOG_DBG("Device init attempt %d", (retries+1));
		device_init(i3c_bus_dev);
		k_msleep(I3C_INIT_DELAY_RETRY_MS);
		retries++;
	} while (retries < MAX_I3C_INIT_RETRIES);

	LOG_INF("I3C driver loaded: %d  retries: %d", loaded, retries);

	ret = i3c_config_get(i3c_bus_dev, I3C_CONFIG_CONTROLLER, &ctrl_config);
	if (ret) {
		LOG_ERR("Unable to get I3C config %d", ret);
	} else {
		LOG_INF("I3C bus configured to %d Hz", ctrl_config.scl.i3c);
	}

	const struct device *dev = DEVICE_DT_GET_OR_NULL(MCTP_ENDPOINT0_NODE);

	if (dev) {
		device_init(dev);
	} else {
		LOG_ERR("Unable to get MCTP endpoint device");
	}
}
