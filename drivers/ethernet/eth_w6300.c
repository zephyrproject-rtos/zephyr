/* W6300 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2025 WIZnet Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wiznet_w6300

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_w6300, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <ethernet/eth_stats.h>

#include "eth.h"
#include "eth_w6300_priv.h"

static inline uint8_t w6300_spi_instr(uint8_t rwb, uint8_t bsb)
{
	return (uint8_t)((W6300_SPI_MOD_SINGLE << 6) | ((rwb & 0x1) << 5) |
			 (bsb & 0x1f));
}

static int w6300_spi_read(const struct device *dev, uint8_t bsb,
			  uint16_t addr, uint8_t *data, size_t len)
{
	const struct w6300_config *cfg = dev->config;
	/* W6300 SPI read: 3-byte command + 1 dummy byte before data */
	uint8_t header[4] = {
		w6300_spi_instr(W6300_SPI_RWB_READ, bsb),
		(uint8_t)(addr >> 8),
		(uint8_t)addr,
		0x00, /* dummy byte */
	};
	const struct spi_buf tx_buf = {
		.buf = header,
		.len = sizeof(header),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_bufs[2] = {
		{
			.buf = NULL,
			.len = sizeof(header),
		},
		{
			.buf = data,
			.len = len,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};

	return spi_transceive_dt(&cfg->spi, &tx, &rx);
}

static int w6300_spi_write(const struct device *dev, uint8_t bsb,
			   uint16_t addr, const uint8_t *data, size_t len)
{
	const struct w6300_config *cfg = dev->config;
	uint8_t header[3] = {
		w6300_spi_instr(W6300_SPI_RWB_WRITE, bsb),
		(uint8_t)(addr >> 8),
		(uint8_t)addr,
	};
	const struct spi_buf tx_bufs[2] = {
		{
			.buf = header,
			.len = sizeof(header),
		},
		{
			.buf = (void *)data,
			.len = len,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	return spi_write_dt(&cfg->spi, &tx);
}

static int w6300_buf_read(const struct device *dev, uint8_t bsb,
			  uint16_t offset, uint8_t *buf, size_t len,
			  uint16_t buf_size)
{
	uint16_t off;
	size_t first;
	int ret;

	if (buf_size == 0) {
		return -EINVAL;
	}

	off = (uint16_t)(offset % buf_size);
	first = MIN(len, (size_t)(buf_size - off));

	ret = w6300_spi_read(dev, bsb, off, buf, first);
	if (ret || first == len) {
		return ret;
	}

	return w6300_spi_read(dev, bsb, 0, buf + first, len - first);
}

static int w6300_buf_write(const struct device *dev, uint8_t bsb,
			   uint16_t offset, const uint8_t *buf, size_t len,
			   uint16_t buf_size)
{
	uint16_t off;
	size_t first;
	int ret;

	if (buf_size == 0) {
		return -EINVAL;
	}

	off = (uint16_t)(offset % buf_size);
	first = MIN(len, (size_t)(buf_size - off));

	ret = w6300_spi_write(dev, bsb, off, buf, first);
	if (ret || first == len) {
		return ret;
	}

	return w6300_spi_write(dev, bsb, 0, buf + first, len - first);
}

static int w6300_command(const struct device *dev, uint8_t cmd)
{
	uint8_t reg;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(W6300_CMD_TIMEOUT_MS));
	int ret;

	ret = w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_CR, &cmd, 1);
	if (ret < 0) {
		return ret;
	}

	while (true) {
		ret = w6300_spi_read(dev, W6300_BSB_SOCK(0), W6300_Sn_CR, &reg, 1);
		if (ret < 0) {
			return ret;
		}

		if (reg == 0) {
			return 0;
		}

		if (sys_timepoint_expired(end)) {
			return -EIO;
		}

		k_busy_wait(W6300_CMD_POLL_US);
	}
}

static int w6300_set_macaddr(const struct device *dev)
{
	struct w6300_runtime *ctx = dev->data;

	return w6300_spi_write(dev, W6300_BSB_COMMON, W6300_SHAR,
			       ctx->mac_addr, sizeof(ctx->mac_addr));
}

static int w6300_set_buffer_sizes(const struct device *dev)
{
	struct w6300_runtime *ctx = dev->data;
	uint8_t bsr;
	int ret;

	ret = w6300_spi_read(dev, W6300_BSB_SOCK(0), W6300_Sn_TX_BSR, &bsr, 1);
	if (ret < 0 || bsr == 0) {
		bsr = W6300_DEFAULT_BSR_KB;
		ret = w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_TX_BSR, &bsr, 1);
		if (ret < 0) {
			return ret;
		}
	}
	ctx->tx_buf_size = W6300_BSR_TO_BYTES(bsr);

	ret = w6300_spi_read(dev, W6300_BSB_SOCK(0), W6300_Sn_RX_BSR, &bsr, 1);
	if (ret < 0 || bsr == 0) {
		bsr = W6300_DEFAULT_BSR_KB;
		ret = w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_RX_BSR, &bsr, 1);
		if (ret < 0) {
			return ret;
		}
	}
	ctx->rx_buf_size = W6300_BSR_TO_BYTES(bsr);

	return 0;
}

static int w6300_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct w6300_runtime *ctx = dev->data;
	uint16_t len = (uint16_t)net_pkt_get_len(pkt);
	uint16_t offset;
	uint8_t tmp[2];
	int ret;

	if (len > ctx->tx_buf_size) {
		return -EMSGSIZE;
	}

	ret = w6300_spi_read(dev, W6300_BSB_SOCK(0), W6300_Sn_TX_WR, tmp, 2);
	if (ret < 0) {
		return ret;
	}
	offset = sys_get_be16(tmp);

	if (net_pkt_read(pkt, ctx->buf, len)) {
		return -EIO;
	}

	ret = w6300_buf_write(dev, W6300_BSB_TX(0), offset, ctx->buf, len,
			      ctx->tx_buf_size);
	if (ret < 0) {
		return ret;
	}

	offset += len;
	sys_put_be16(offset, tmp);
	ret = w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_TX_WR, tmp, 2);
	if (ret < 0) {
		return ret;
	}

	ret = w6300_command(dev, W6300_Sn_CR_SEND);
	if (ret < 0) {
		return ret;
	}

	if (k_sem_take(&ctx->tx_sem, K_MSEC(W6300_TX_SEM_TIMEOUT_MS))) {
		return -EIO;
	}

	return 0;
}

static void w6300_drop_rx(const struct device *dev, uint16_t off,
			  uint16_t drop_len)
{
	uint8_t tmp[2];

	sys_put_be16(off + drop_len, tmp);
	w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_RX_RD, tmp, 2);
	w6300_command(dev, W6300_Sn_CR_RECV);
}

static void w6300_rx(const struct device *dev)
{
	uint8_t hdr[W6300_PKT_INFO_LEN];
	uint8_t tmp[2];
	uint16_t off;
	uint16_t rx_buf_len;
	uint16_t frame_len;
	uint16_t total_len;
	uint16_t reader;
	uint16_t read_len;
	struct net_buf *pkt_buf = NULL;
	struct net_pkt *pkt;
	struct w6300_runtime *ctx = dev->data;

	if (w6300_spi_read(dev, W6300_BSB_SOCK(0), W6300_Sn_RX_RSR, tmp, 2) < 0) {
		return;
	}
	rx_buf_len = sys_get_be16(tmp);

	if (rx_buf_len < W6300_PKT_INFO_LEN) {
		return;
	}

	if (w6300_spi_read(dev, W6300_BSB_SOCK(0), W6300_Sn_RX_RD, tmp, 2) < 0) {
		return;
	}
	off = sys_get_be16(tmp);

	if (w6300_buf_read(dev, W6300_BSB_RX(0), off, hdr, sizeof(hdr),
			   ctx->rx_buf_size) < 0) {
		w6300_drop_rx(dev, off, rx_buf_len);
		eth_stats_update_errors_rx(ctx->iface);
		return;
	}

	frame_len = sys_get_be16(hdr);
	total_len = frame_len + W6300_PKT_INFO_LEN;

	if (frame_len < W6300_ETH_MIN_FRAME_LEN ||
	    frame_len > NET_ETH_MAX_FRAME_SIZE ||
	    total_len > rx_buf_len) {
		w6300_drop_rx(dev, off, rx_buf_len);
		eth_stats_update_errors_rx(ctx->iface);
		return;
	}

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, frame_len, NET_AF_UNSPEC, 0,
					   K_MSEC(CONFIG_ETH_W6300_TIMEOUT));
	if (!pkt) {
		eth_stats_update_errors_rx(ctx->iface);
		w6300_drop_rx(dev, off, total_len);
		return;
	}

	pkt_buf = pkt->buffer;
	read_len = frame_len;
	reader = off + W6300_PKT_INFO_LEN;

	while (read_len > 0) {
		size_t frag_len;
		size_t chunk_len;
		uint8_t *data_ptr;

		data_ptr = pkt_buf->data;
		frag_len = net_buf_tailroom(pkt_buf);
		chunk_len = MIN(read_len, (uint16_t)frag_len);

		if (w6300_buf_read(dev, W6300_BSB_RX(0), reader, data_ptr, chunk_len,
				   ctx->rx_buf_size) < 0) {
			eth_stats_update_errors_rx(ctx->iface);
			net_pkt_unref(pkt);
			w6300_drop_rx(dev, off, rx_buf_len);
			return;
		}

		net_buf_add(pkt_buf, chunk_len);
		reader += (uint16_t)chunk_len;
		read_len -= (uint16_t)chunk_len;
		pkt_buf = pkt_buf->frags;
	}

	if (net_recv_data(ctx->iface, pkt) < 0) {
		net_pkt_unref(pkt);
		eth_stats_update_errors_rx(ctx->iface);
	}

	w6300_drop_rx(dev, off, total_len);
}

static void w6300_update_link_status(const struct device *dev)
{
	uint8_t physr;
	struct w6300_runtime *ctx = dev->data;
	enum phy_link_speed speed;

	if (w6300_spi_read(dev, W6300_BSB_COMMON, W6300_PHYSR, &physr, 1) < 0) {
		return;
	}

	if (physr & W6300_PHYSR_LNK) {
		if (ctx->state.is_up != true) {
			LOG_INF("%s: Link up", dev->name);
			ctx->state.is_up = true;
			net_eth_carrier_on(ctx->iface);
		}

		if (physr & W6300_PHYSR_SPD) {
			speed = (physr & W6300_PHYSR_DPX) ? LINK_HALF_10BASE
							: LINK_FULL_10BASE;
		} else {
			speed = (physr & W6300_PHYSR_DPX) ? LINK_HALF_100BASE
							: LINK_FULL_100BASE;
		}

		if (ctx->state.speed != speed) {
			ctx->state.speed = speed;
			LOG_INF("%s: Link speed %s Mb, %s duplex", dev->name,
				PHY_LINK_IS_SPEED_100M(speed) ? "100" : "10",
				PHY_LINK_IS_FULL_DUPLEX(speed) ? "full" : "half");
		}
	} else {
		if (ctx->state.is_up != false) {
			LOG_INF("%s: Link down", dev->name);
			ctx->state.is_up = false;
			ctx->state.speed = 0;
			net_eth_carrier_off(ctx->iface);
		}
	}
}

static void w6300_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	uint8_t ir;
	int res;
	struct w6300_runtime *ctx = dev->data;
	const struct w6300_config *config = dev->config;

	while (true) {
		res = k_sem_take(&ctx->int_sem,
				 K_MSEC(CONFIG_ETH_W6300_MONITOR_PERIOD));

		if (res == 0) {
			if (ctx->state.is_up != true) {
				w6300_update_link_status(dev);
			}

			while (gpio_pin_get_dt(&config->interrupt)) {
				if (w6300_spi_read(dev, W6300_BSB_SOCK(0), W6300_Sn_IR,
						   &ir, 1) < 0) {
					break;
				}

				if (!ir) {
					break;
				}

				w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_IRCLR, &ir, 1);

				if (ir & W6300_Sn_IR_SENDOK) {
					k_sem_give(&ctx->tx_sem);
				}

				if (ir & W6300_Sn_IR_RECV) {
					w6300_rx(dev);
				}
			}
		} else if (res == -EAGAIN) {
			w6300_update_link_status(dev);
		}
	}
}

static void w6300_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct w6300_runtime *ctx = dev->data;

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr),
			     NET_LINK_ETHERNET);

	if (!ctx->iface) {
		ctx->iface = iface;
	}

	ethernet_init(iface);
	/* Do not start the interface until PHY link is up. */
	net_if_carrier_off(iface);
}

static enum ethernet_hw_caps w6300_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
	;
}

static int w6300_set_config(const struct device *dev,
			    enum ethernet_config_type type,
			    const struct ethernet_config *config)
{
	struct w6300_runtime *ctx = dev->data;
	uint8_t mode;
	int ret;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(ctx->mac_addr, config->mac_address.addr, sizeof(ctx->mac_addr));
		ret = w6300_set_macaddr(dev);
		if (ret < 0) {
			return ret;
		}

		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x",
			dev->name, ctx->mac_addr[0], ctx->mac_addr[1],
			ctx->mac_addr[2], ctx->mac_addr[3],
			ctx->mac_addr[4], ctx->mac_addr[5]);

		net_if_set_link_addr(ctx->iface, ctx->mac_addr,
				     sizeof(ctx->mac_addr),
				     NET_LINK_ETHERNET);
		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (!IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			return -ENOTSUP;
		}

		if (w6300_spi_read(dev, W6300_BSB_SOCK(0), W6300_Sn_MR, &mode, 1) < 0) {
			return -EIO;
		}

		if (config->promisc_mode) {
			mode &= (uint8_t)~W6300_Sn_MR_MF;
		} else {
			mode |= W6300_Sn_MR_MF;
		}

		return w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_MR,
					&mode, 1);
	default:
		return -ENOTSUP;
	}
}

static int w6300_hw_start(const struct device *dev)
{
	uint8_t mode = W6300_Sn_MR_MACRAW | W6300_Sn_MR_MF;
	uint8_t imr = W6300_Sn_IR_SENDOK | W6300_Sn_IR_RECV;
	uint8_t simr = BIT(0);
	uint8_t irclr = 0xFF;
	int ret;

	ret = w6300_set_buffer_sizes(dev);
	if (ret < 0) {
		return ret;
	}

	w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_MR, &mode, 1);
	ret = w6300_command(dev, W6300_Sn_CR_OPEN);
	if (ret < 0) {
		return ret;
	}

	w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_IRCLR, &irclr, 1);
	w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_IMR, &imr, 1);
	w6300_spi_write(dev, W6300_BSB_COMMON, W6300_SIMR, &simr, 1);

	return 0;
}

static int w6300_hw_stop(const struct device *dev)
{
	uint8_t mask = 0;

	w6300_spi_write(dev, W6300_BSB_COMMON, W6300_SIMR, &mask, 1);
	w6300_spi_write(dev, W6300_BSB_SOCK(0), W6300_Sn_IMR, &mask, 1);
	w6300_command(dev, W6300_Sn_CR_CLOSE);

	return 0;
}

static const struct device *w6300_get_phy(const struct device *dev)
{
	const struct w6300_config *config = dev->config;

	return config->phy_dev;
}

static const struct ethernet_api w6300_api_funcs = {
	.iface_api.init = w6300_iface_init,
	.get_capabilities = w6300_get_capabilities,
	.set_config = w6300_set_config,
	.start = w6300_hw_start,
	.stop = w6300_hw_stop,
	.get_phy = w6300_get_phy,
	.send = w6300_tx,
};

static int w6300_get_link_state(const struct device *dev,
				struct phy_link_state *state)
{
	struct w6300_runtime *const data = dev->data;

	state->speed = data->state.speed;
	state->is_up = data->state.is_up;

	return 0;
}

static DEVICE_API(ethphy, w6300_phy_driver_api) = {
	.get_link = w6300_get_link_state,
};

static void w6300_gpio_callback(const struct device *dev,
				struct gpio_callback *cb,
				uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct w6300_runtime *ctx =
		CONTAINER_OF(cb, struct w6300_runtime, gpio_cb);

	k_sem_give(&ctx->int_sem);
}

static int w6300_soft_reset(const struct device *dev)
{
	uint8_t reg = W6300_SYCR0_RST;
	int ret;

	ret = w6300_spi_write(dev, W6300_BSB_COMMON, W6300_SYCR0, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(1);
	return 0;
}

static int w6300_configure_defaults(const struct device *dev)
{
	uint8_t reg;

	if (w6300_spi_read(dev, W6300_BSB_COMMON, W6300_SYCR1, &reg, 1) < 0) {
		return -EIO;
	}

	reg |= W6300_SYCR1_IEN;
	if (w6300_spi_write(dev, W6300_BSB_COMMON, W6300_SYCR1, &reg, 1) < 0) {
		return -EIO;
	}

	reg = 0xFF;
	w6300_spi_write(dev, W6300_BSB_COMMON, W6300_IRCLR, &reg, 1);

	return 0;
}

static int w6300_init(const struct device *dev)
{
	int err;
	uint8_t cidr[2];
	uint8_t cidr2;
	const struct w6300_config *config = dev->config;
	struct w6300_runtime *ctx = dev->data;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI master port %s not ready", config->spi.bus->name);
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&config->interrupt)) {
		LOG_ERR("GPIO port %s not ready", config->interrupt.port->name);
		return -EINVAL;
	}

	err = gpio_pin_configure_dt(&config->interrupt, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Unable to configure GPIO pin %u", config->interrupt.pin);
		return err;
	}

	gpio_init_callback(&ctx->gpio_cb, w6300_gpio_callback,
			   BIT(config->interrupt.pin));
	err = gpio_add_callback(config->interrupt.port, &ctx->gpio_cb);
	if (err < 0) {
		LOG_ERR("Unable to add GPIO callback %u", config->interrupt.pin);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->interrupt,
				      GPIO_INT_EDGE_FALLING);
	if (err < 0) {
		LOG_ERR("Unable to enable GPIO INT %u", config->interrupt.pin);
		return err;
	}

	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			LOG_ERR("GPIO port %s not ready", config->reset.port->name);
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
		if (err < 0) {
			LOG_ERR("Unable to configure GPIO pin %u", config->reset.pin);
			return err;
		}

		gpio_pin_set_dt(&config->reset, 1);
		k_usleep(500);
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(1);
	}

	if (config->io2_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->io2_gpio)) {
			LOG_ERR("GPIO port %s not ready", config->io2_gpio.port->name);
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&config->io2_gpio, GPIO_INPUT);
		if (err < 0) {
			LOG_ERR("Unable to configure GPIO pin %u", config->io2_gpio.pin);
			return err;
		}
	}

	if (config->io3_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->io3_gpio)) {
			LOG_ERR("GPIO port %s not ready", config->io3_gpio.port->name);
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&config->io3_gpio, GPIO_INPUT);
		if (err < 0) {
			LOG_ERR("Unable to configure GPIO pin %u", config->io3_gpio.pin);
			return err;
		}
	}

	err = w6300_soft_reset(dev);
	if (err != 0) {
		LOG_ERR("Reset failed");
		return err;
	}

	if (w6300_spi_read(dev, W6300_BSB_COMMON, W6300_CIDR0, cidr, 2) < 0) {
		LOG_ERR("Unable to read CIDR");
		return -EIO;
	}

	if (cidr[0] != 0x61 || cidr[1] != 0x00) {
		LOG_ERR("Unexpected CIDR %02x %02x", cidr[0], cidr[1]);
		return -ENODEV;
	}

	if (w6300_spi_read(dev, W6300_BSB_COMMON, W6300_CIDR2, &cidr2, 1) == 0) {
		LOG_INF("CIDR2 0x%02x", cidr2);
	}

	if (w6300_configure_defaults(dev) < 0) {
		LOG_ERR("Default configuration failed");
		return -EIO;
	}

	if (net_eth_mac_load(&config->mac_cfg, ctx->mac_addr) < 0) {
		LOG_ERR("Failed to load MAC address");
		return -EINVAL;
	}

	if (w6300_set_macaddr(dev) < 0) {
		LOG_ERR("Unable to set MAC address");
		return -EIO;
	}

	k_thread_create(&ctx->thread, ctx->thread_stack,
			CONFIG_ETH_W6300_RX_THREAD_STACK_SIZE,
			w6300_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_W6300_RX_THREAD_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&ctx->thread, "eth_w6300");

	LOG_INF("W6300 initialized");

	return 0;
}

#define W6300_INST_DEFINE(inst) \
	DEVICE_DECLARE(eth_w6300_phy_##inst); \
	static struct w6300_runtime w6300_runtime_##inst = { \
		.tx_sem = Z_SEM_INITIALIZER(w6300_runtime_##inst.tx_sem, 1, UINT_MAX), \
		.int_sem = Z_SEM_INITIALIZER(w6300_runtime_##inst.int_sem, 0, UINT_MAX), \
	}; \
	static const struct w6300_config w6300_config_##inst = { \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)), \
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios), \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, { 0 }), \
		.io2_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, io2_gpios, { 0 }), \
		.io3_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, io3_gpios, { 0 }), \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst), \
		.phy_dev = DEVICE_GET(eth_w6300_phy_##inst), \
	}; \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, \
				      w6300_init, NULL, \
				      &w6300_runtime_##inst, &w6300_config_##inst, \
				      CONFIG_ETH_INIT_PRIORITY, &w6300_api_funcs, \
				      NET_ETH_MTU); \
	DEVICE_DEFINE(eth_w6300_phy_##inst, \
		      DEVICE_DT_NAME(DT_DRV_INST(inst)) "_phy", \
		      NULL, NULL, &w6300_runtime_##inst, &w6300_config_##inst, \
		      POST_KERNEL, CONFIG_ETH_INIT_PRIORITY, &w6300_phy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(W6300_INST_DEFINE)
