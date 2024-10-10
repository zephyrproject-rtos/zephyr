/*
 * Copyright (c) 2024 Grant Ramsay
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Provides the initialsation sequence required for WIZ550io
 * to configure the W5500 Ethernet controller with its
 * embedded unique MAC address.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(eth_w5500, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/ethernet.h>

#define W5500_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(wiznet_w5500)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(wiznet_w5500) == 1,
	     "Only a single WIZ550io/W5500 instance is supported");

BUILD_ASSERT(!DT_NODE_HAS_PROP(W5500_NODE, reset_gpios),
	     "The reset-gpios property must be assigned to WIZ550io "
	     "rather than the W5500 device");

/* Reset GPIO */
#define WIZ550IO_RESET_GPIO_NODE DT_ALIAS(wiz550io_reset)
#define WIZ550IO_RESET_PORT      DEVICE_DT_GET(DT_PARENT(WIZ550IO_RESET_GPIO_NODE))
#define WIZ550IO_RESET_PIN       DT_GPIO_HOG_PIN_BY_IDX(WIZ550IO_RESET_GPIO_NODE, 0)

/* Optional Ready GPIO */
#define WIZ550IO_READY_GPIO_NODE DT_ALIAS(wiz550io_ready)
#define WIZ550IO_HAS_READY_GPIO  DT_NODE_EXISTS(WIZ550IO_READY_GPIO_NODE)
#define WIZ550IO_READY_PORT      DEVICE_DT_GET_OR_NULL(DT_PARENT(WIZ550IO_READY_GPIO_NODE))
#define WIZ550IO_READY_PIN                                                                         \
	COND_CODE_1(WIZ550IO_HAS_READY_GPIO,                                                       \
		    (DT_GPIO_HOG_PIN_BY_IDX(WIZ550IO_READY_GPIO_NODE, 0)), (0))

/* Use the WIZ550io unique MAC address if W5500 has neither a fixed or random
 * mac address configured.
 */
#define WIZ550IO_USE_MAC_ADDRESS                                                                   \
	(!DT_PROP(W5500_NODE, zephyr_random_mac_address) &&                                        \
	 !DT_NODE_HAS_PROP(W5500_NODE, local_mac_address))

/* W5500 registers */
#define W5500_SHAR 0x0009 /* Source MAC address */

#define W5500_SPI_BLOCK_SELECT(addr) (((addr) >> 16) & 0x1f)
#define W5500_SPI_READ_CONTROL(addr) (W5500_SPI_BLOCK_SELECT(addr) << 3)

/* WIZ550io documentation recommends 150ms delay after HW reset
 * for PIC12F519 MCU to configure the W5500.
 */
#define WIZ550IO_RESET_DELAY K_MSEC(150)

static struct net_eth_addr wiz550io_mac_addr;

static int wiz550io_spi_read_mac(struct net_eth_addr *mac_addr)
{
	struct spi_dt_spec spi = SPI_DT_SPEC_GET(W5500_NODE, SPI_WORD_SET(8), 0);
	const uint32_t addr = W5500_SHAR;
	int ret;

	if (!spi_is_ready_dt(&spi)) {
		LOG_ERR("SPI %s not ready", spi.bus->name);
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

	ret = spi_transceive_dt(&spi, &tx, &rx);

	return ret;
}

static int wiz550io_pre_init(void)
{
	int ret;

	/* Hold reset pin low for 500us to perform a hardware reset */
	gpio_pin_set(WIZ550IO_RESET_PORT, WIZ550IO_RESET_PIN, 1);
	k_usleep(500);
	gpio_pin_set(WIZ550IO_RESET_PORT, WIZ550IO_RESET_PIN, 0);

	/* Wait for the device to be ready */
	if (WIZ550IO_HAS_READY_GPIO) {
		k_timepoint_t timeout = sys_timepoint_calc(WIZ550IO_RESET_DELAY);

		while (!gpio_pin_get(WIZ550IO_READY_PORT, WIZ550IO_READY_PIN)) {
			if (sys_timepoint_expired(timeout)) {
				LOG_ERR("WIZ550io not ready");
				return -ETIMEDOUT;
			}
			k_msleep(1);
		}
	} else {
		k_sleep(WIZ550IO_RESET_DELAY);
	}

	if (WIZ550IO_USE_MAC_ADDRESS) {
		/* Read the WIZ550io unique MAC address set by the PIC12F519.
		 * It needs to be read after a hardware reset and before software reset.
		 */
		ret = wiz550io_spi_read_mac(&wiz550io_mac_addr);
		if (ret) {
			LOG_ERR("WIZ550io unable to read MAC address");
			return -EIO;
		}
	}

	return 0;
}

static int wiz550io_post_init(void)
{
	int ret = 0;

	if (WIZ550IO_USE_MAC_ADDRESS) {
		/* Assign the WIZ550io unique MAC address */
		const struct device *w5500_dev = DEVICE_DT_GET(W5500_NODE);
		const struct ethernet_api *w5500_api = w5500_dev->api;
		struct ethernet_config config = {.mac_address = wiz550io_mac_addr};

		ret = w5500_api->set_config(w5500_dev, ETHERNET_CONFIG_TYPE_MAC_ADDRESS, &config);
	}

	return ret;
}

BUILD_ASSERT(CONFIG_WIZ550IO_PRE_INIT_PRIORITY < CONFIG_ETH_INIT_PRIORITY,
	     "WIZ550io pre init must occur before W5500 init");
SYS_INIT(wiz550io_pre_init, POST_KERNEL, CONFIG_WIZ550IO_PRE_INIT_PRIORITY);

BUILD_ASSERT(CONFIG_WIZ550IO_POST_INIT_PRIORITY > CONFIG_NET_INIT_PRIO,
	     "WIZ550io post init must occur after net iface init");
SYS_INIT(wiz550io_post_init, POST_KERNEL, CONFIG_WIZ550IO_POST_INIT_PRIORITY);
