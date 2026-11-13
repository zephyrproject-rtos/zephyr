/*
 * Copyright (c) 2026 Igalia S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_RPI_PICO_CYW43_ZEPHYR_CYW43_SPI_H
#define ZEPHYR_DRIVERS_WIFI_RPI_PICO_CYW43_ZEPHYR_CYW43_SPI_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>

struct cyw43_wifi_dev_config {
	struct spi_dt_spec bus_spi;
	struct gpio_dt_spec wl_on_gpio;
	struct gpio_dt_spec host_wake_gpio;
	struct gpio_dt_spec bus_select_gpio;
	const struct pinctrl_dev_config *pcfg;
};

extern struct cyw43_wifi_dev_config cyw43_wifi_dev_cfg;

void cyw43_irq_enable(const struct cyw43_wifi_dev_config *dev_cfg, bool enable);

#endif /* ZEPHYR_DRIVERS_WIFI_RPI_PICO_CYW43_ZEPHYR_CYW43_SPI_H */
