/**
 * Copyright (c) 2017 IpTronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_WINC1500_WIFI_WINC1500_NM_BSP_INTERNAL_H_
#define ZEPHYR_DRIVERS_WIFI_WINC1500_WIFI_WINC1500_NM_BSP_INTERNAL_H_

#include <device.h>
#include <gpio.h>
#include <spi.h>

#include <wifi/winc1500.h>

#include "wifi_winc1500_config.h"
#include <bus_wrapper/include/nm_bus_wrapper.h>

extern tstrNmBusCapabilities egstrNmBusCapabilities;

#define NM_EDGE_INTERRUPT	(1)

#define NM_DEBUG		CONF_WINC_DEBUG
#define NM_BSP_PRINTF		CONF_WINC_PRINTF

struct winc1500_device {
	struct winc1500_gpio_configuration	*gpios;
	struct gpio_callback			gpio_cb;
	struct device				*spi;
	struct spi_config			spi_cfg;
};

extern struct winc1500_device winc1500;

#endif /* ZEPHYR_DRIVERS_WIFI_WINC1500_WIFI_WINC1500_NM_BSP_INTERNAL_H_ */
