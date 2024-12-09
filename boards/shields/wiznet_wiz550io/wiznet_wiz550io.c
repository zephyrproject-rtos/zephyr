/*
 * Copyright (c) 2024 Grant Ramsay
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Provides the initialsation sequence required for WIZ550io
 * to configure the W5500 Ethernet controller with its
 * embedded unique MAC address.
 */

#define DT_DRV_COMPAT wiznet_wiz550io

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>

LOG_MODULE_DECLARE(eth_w5500, CONFIG_ETHERNET_LOG_LEVEL);

/* WIZ550io documentation recommends 150ms delay after HW reset
 * for PIC12F519 MCU to configure the W5500.
 */
#define WIZ550IO_RESET_DELAY K_MSEC(150)

/* W5500 registers */
#define W5500_SHAR 0x0009 /* Source MAC address */

#define W5500_SPI_BLOCK_SELECT(addr) (((addr) >> 16) & 0x1f)
#define W5500_SPI_READ_CONTROL(addr) (W5500_SPI_BLOCK_SELECT(addr) << 3)

struct wiz550io_data {
	struct net_eth_addr mac_addr;
};

struct wiz550io_config {
	const struct device *w5500_dev;
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec ready;
	bool use_mac_addr;
};

static int wiz550io_spi_read_mac(const struct spi_dt_spec *spi, struct net_eth_addr *mac_addr)
{
	const uint32_t addr = W5500_SHAR;
	int ret;

	if (!spi_is_ready_dt(spi)) {
		LOG_ERR("SPI %s not ready", spi->bus->name);
		return -EINVAL;
	}

	uint8_t cmd[3] = {addr >> 8, addr, W5500_SPI_READ_CONTROL(addr)};
	const struct spi_buf tx_buf = {
		.buf = cmd,
		.len = ARRAY_SIZE(cmd),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	/* skip the default dummy 0x010203 */
	const struct spi_buf rx_buf[2] = {
		{.buf = NULL, .len = 3},
		{.buf = mac_addr->addr, .len = sizeof(mac_addr->addr)},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	ret = spi_transceive_dt(spi, &tx, &rx);

	return ret;
}

static int wiz550io_pre_w5500_init(struct wiz550io_data *data, const struct wiz550io_config *config)
{
	int ret;

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

	if (config->use_mac_addr) {
		/* Read the WIZ550io unique MAC address set by the PIC12F519.
		 * It needs to be read after a hardware reset and before software reset.
		 */
		ret = wiz550io_spi_read_mac(&config->spi, &data->mac_addr);
		if (ret) {
			LOG_ERR("WIZ550io unable to read MAC address");
			return -EIO;
		}
	}

	return 0;
}

static int wiz550io_post_net_if_init(struct wiz550io_data *data,
				     const struct wiz550io_config *config)
{
	int ret = 0;

	if (config->use_mac_addr) {
		/* Assign the WIZ550io unique MAC address */
		const struct device *w5500_dev = config->w5500_dev;
		const struct ethernet_api *w5500_api = w5500_dev->api;
		struct ethernet_config eth_config = {.mac_address = data->mac_addr};

		ret = w5500_api->set_config(w5500_dev, ETHERNET_CONFIG_TYPE_MAC_ADDRESS,
					    &eth_config);
	}

	return ret;
}

#define W5500_NODE(inst) DT_INST_PHANDLE(inst, wiznet_w5500)

#define WIZ550IO_INIT(inst)                                                                        \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(W5500_NODE(inst), reset_gpios),                             \
		     "The reset-gpios property must be assigned to WIZ550io "                      \
		     "rather than the W5500 device");                                              \
                                                                                                   \
	static struct wiz550io_data wiz550io_data_##inst;                                          \
                                                                                                   \
	static const struct wiz550io_config wiz550io_config_##inst = {                             \
		.w5500_dev = DEVICE_DT_GET(W5500_NODE(inst)),                                      \
		.spi = SPI_DT_SPEC_GET(W5500_NODE(inst), SPI_WORD_SET(8), 0),                      \
		.reset = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                                 \
		.ready = GPIO_DT_SPEC_INST_GET_OR(inst, ready_gpios, {0}),                         \
		.use_mac_addr = (!DT_PROP(W5500_NODE(inst), zephyr_random_mac_address) &&          \
				 !DT_NODE_HAS_PROP(W5500_NODE(inst), local_mac_address)),          \
	};                                                                                         \
                                                                                                   \
	static int wiz550io_pre_w5500_init_##inst(void)                                            \
	{                                                                                          \
		return wiz550io_pre_w5500_init(&wiz550io_data_##inst, &wiz550io_config_##inst);    \
	}                                                                                          \
	SYS_INIT(wiz550io_pre_w5500_init_##inst, POST_KERNEL,                                      \
		 CONFIG_WIZ550IO_PRE_W5500_INIT_PRIORITY);                                         \
                                                                                                   \
	static int wiz550io_post_net_if_init_##inst(void)                                          \
	{                                                                                          \
		return wiz550io_post_net_if_init(&wiz550io_data_##inst, &wiz550io_config_##inst);  \
	}                                                                                          \
	SYS_INIT(wiz550io_post_net_if_init_##inst, POST_KERNEL,                                    \
		 CONFIG_WIZ550IO_POST_NET_IF_INIT_PRIORITY);

DT_INST_FOREACH_STATUS_OKAY(WIZ550IO_INIT)

BUILD_ASSERT(CONFIG_WIZ550IO_PRE_W5500_INIT_PRIORITY < CONFIG_ETH_INIT_PRIORITY,
	     "WIZ550io pre W5500 init must occur before W5500 init");
BUILD_ASSERT(CONFIG_WIZ550IO_POST_NET_IF_INIT_PRIORITY > CONFIG_NET_INIT_PRIO,
	     "WIZ550io post net interface init must occur after net interface init");
