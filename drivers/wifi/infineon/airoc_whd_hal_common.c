/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "airoc_whd_hal_common.h"
#include "airoc_wifi.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_DECLARE(infineon_airoc_wifi, CONFIG_WIFI_LOG_LEVEL);

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                 Function
 ******************************************************/

int airoc_wifi_power_on(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, wifi_reg_on_gpios)
	int ret;
	const struct airoc_wifi_config *config = dev->config;

	/* Check WIFI REG_ON gpio instance */
	if (!device_is_ready(config->wifi_reg_on_gpio.port)) {
		LOG_ERR("Error: failed to configure wifi_reg_on %s pin %d",
			config->wifi_reg_on_gpio.port->name, config->wifi_reg_on_gpio.pin);
		return -EIO;
	}

	/* Configure wifi_reg_on as output  */
	ret = gpio_pin_configure_dt(&config->wifi_reg_on_gpio, GPIO_OUTPUT);
	if (ret) {
		LOG_ERR("Error %d: failed to configure wifi_reg_on %s pin %d", ret,
			config->wifi_reg_on_gpio.port->name, config->wifi_reg_on_gpio.pin);
		return ret;
	}
	ret = gpio_pin_set_dt(&config->wifi_reg_on_gpio, 0);
	if (ret) {
		return ret;
	}

	/* Allow CBUCK regulator to discharge */
	k_msleep(WLAN_CBUCK_DISCHARGE_MS);

	/* WIFI power on */
	ret = gpio_pin_set_dt(&config->wifi_reg_on_gpio, 1);
	if (ret) {
		return ret;
	}
	k_msleep(WLAN_POWER_UP_DELAY_MS);
#endif /* DT_INST_NODE_HAS_PROP(0, reg_on_gpios) */

	return 0;
}

/*
 * Implement WHD memory wrappers
 */

inline void *whd_mem_malloc(size_t size)
{
	return k_malloc(size);
}

inline void *whd_mem_calloc(size_t nitems, size_t size)
{
	return k_calloc(nitems, size);
}

inline void whd_mem_free(void *ptr)
{
	k_free(ptr);
}

inline void whd_mem_memcpy(void *dest, const void *src, size_t len)
{
	memcpy(dest, src, len);
}

inline void whd_mem_memset(void *buf, int val, size_t len)
{
	memset(buf, val, len);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
