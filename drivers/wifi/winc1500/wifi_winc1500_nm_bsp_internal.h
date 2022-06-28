/**
 * Copyright (c) 2017 IpTronix
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_WINC1500_WIFI_WINC1500_NM_BSP_INTERNAL_H_
#define ZEPHYR_DRIVERS_WIFI_WINC1500_WIFI_WINC1500_NM_BSP_INTERNAL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include "wifi_winc1500_config.h"
#include <bus_wrapper/include/nm_bus_wrapper.h>

extern tstrNmBusCapabilities egstrNmBusCapabilities;

#if defined(CONFIG_WINC1500_DRV_USE_OLD_SPI_SW)
#define USE_OLD_SPI_SW
#endif /*CONFIG_WINC1500_DRV_USE_OLD_SPI_SW*/

#define NM_EDGE_INTERRUPT	(1)

#define NM_DEBUG		CONF_WINC_DEBUG
#define NM_BSP_PRINTF		CONF_WINC_PRINTF

enum winc1500_gpio_index {
	WINC1500_GPIO_IDX_CHIP_EN = 0,
	WINC1500_GPIO_IDX_IRQN,
	WINC1500_GPIO_IDX_RESET_N,

	WINC1500_GPIO_IDX_MAX
};

struct winc1500_cfg {
	struct spi_dt_spec spi;
	struct gpio_dt_spec chip_en_gpio;
	struct gpio_dt_spec irq_gpio;
	struct gpio_dt_spec reset_gpio;
};

struct winc1500_device {
	struct gpio_callback			gpio_cb;
};

extern struct winc1500_device winc1500;
extern const struct winc1500_cfg winc1500_config;

#endif /* ZEPHYR_DRIVERS_WIFI_WINC1500_WIFI_WINC1500_NM_BSP_INTERNAL_H_ */
