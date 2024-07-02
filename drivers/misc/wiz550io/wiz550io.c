/*
 * Copyright (c) 2024 Grant Ramsay
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This driver provides the initialsation sequence required for
 * WIZ550io to configure the W5500 Ethernet controller with its
 * embedded unique MAC address.
 */

#define DT_DRV_COMPAT wiznet_wiz550io

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(eth_w5500, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/ethernet.h>

#include "eth_w5500_priv.h"

struct wiz550io_config {
	const struct device *w5500_dev;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec ready;
};

/* WIZ550io documentation recommends 150ms delay after HW reset
 * for PIC12F519 MCU to configure the W5500.
 */
#define WIZ550IO_RESET_DELAY K_MSEC(150)

static int wiz550io_init(const struct wiz550io_config *config)
{
	int err;
	const struct w5500_config *w5500_config = config->w5500_dev->config;
	struct w5500_runtime *w5500_runtime = config->w5500_dev->data;

	if (!spi_is_ready_dt(&w5500_config->spi)) {
		LOG_ERR("SPI master port %s not ready", w5500_config->spi.bus->name);
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&config->reset)) {
		LOG_ERR("GPIO port %s not ready", config->reset.port->name);
		return -EINVAL;
	}

	if (gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT)) {
		LOG_ERR("Unable to configure GPIO pin %u", config->reset.pin);
		return -EINVAL;
	}

	/* Hold reset pin low for 500us to perform a hardware reset */
	gpio_pin_set_dt(&config->reset, 1);
	k_usleep(500);
	gpio_pin_set_dt(&config->reset, 0);

	/* Wait for the device to be ready */
	if (config->ready.port) {
		if (!gpio_is_ready_dt(&config->ready)) {
			LOG_ERR("GPIO port %s not ready", config->ready.port->name);
			return -EINVAL;
		}
		if (gpio_pin_configure_dt(&config->ready, GPIO_INPUT)) {
			LOG_ERR("Unable to configure GPIO pin %u", config->ready.pin);
			return -EINVAL;
		}

		k_timepoint_t timeout = sys_timepoint_calc(WIZ550IO_RESET_DELAY);

		while (!gpio_pin_get_dt(&config->ready)) {
			if (sys_timepoint_expired(timeout)) {
				LOG_ERR("WIZ550io not ready");
				return -ETIMEDOUT;
			}
			k_msleep(1);
		}
	} else {
		k_sleep(WIZ550IO_RESET_DELAY);
	}

	/* Read/assign the WIZ550io unique MAC address set by the PIC12F519
	 * after hardware reset. It needs to be read after a hardware reset
	 * and before software reset.
	 */
	err = w5500_spi_read(config->w5500_dev, W5500_SHAR,
			     w5500_runtime->mac_addr,
			     sizeof(w5500_runtime->mac_addr));
	if (err) {
		LOG_ERR("WIZ550io unable to read MAC address");
		return -EIO;
	}

	LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x",
		config->w5500_dev->name,
		w5500_runtime->mac_addr[0], w5500_runtime->mac_addr[1],
		w5500_runtime->mac_addr[2], w5500_runtime->mac_addr[3],
		w5500_runtime->mac_addr[4], w5500_runtime->mac_addr[5]);

	return 0;
}

static int wiz550io_init_all(void)
{
	int res;

#define W5500_NODE(inst) DT_INST_PHANDLE(inst, wiznet_w5500)

#define WIZ550IO_INIT(inst)							\
	BUILD_ASSERT(!DT_NODE_HAS_PROP(W5500_NODE(inst), reset_gpios),		\
		     "The reset-gpios property must be assigned to WIZ550io "	\
		     "rather than the W5500 device");				\
										\
	static const struct wiz550io_config wiz550io_config_##inst = {		\
		.w5500_dev = DEVICE_DT_GET(W5500_NODE(inst)),			\
		.reset = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),		\
		.ready = GPIO_DT_SPEC_INST_GET_OR(inst, ready_gpios, { 0 }),	\
	};									\
										\
	res = wiz550io_init(&wiz550io_config_##inst);				\
	if (res != 0) {								\
		return res;							\
	}

	DT_INST_FOREACH_STATUS_OKAY(WIZ550IO_INIT)

	return 0;
}

BUILD_ASSERT(CONFIG_WIZ550IO_INIT_PRIORITY < CONFIG_ETH_INIT_PRIORITY,
	     "WIZ550io init must occur before W5500 init");

SYS_INIT(wiz550io_init_all, POST_KERNEL, CONFIG_WIZ550IO_INIT_PRIORITY);
