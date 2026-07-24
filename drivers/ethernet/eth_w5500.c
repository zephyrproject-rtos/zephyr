/* W5500 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2020 Linumiz
 * Author: Parthiban Nallathambi <parthiban@linumiz.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	wiznet_w5500

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_w5500, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>

#include "eth.h"
#include "eth_w5500_priv.h"

#define WIZNET_OUI_B0	0x00
#define WIZNET_OUI_B1	0x08
#define WIZNET_OUI_B2	0xdc

#define W5500_SPI_BLOCK_SELECT(addr)	(((addr) >> 16) & 0x1f)
#define W5500_SPI_READ_CONTROL(addr)	(W5500_SPI_BLOCK_SELECT(addr) << 3)
#define W5500_SPI_WRITE_CONTROL(addr)   \
	((W5500_SPI_BLOCK_SELECT(addr) << 3) | BIT(2))

static int w5500_spi_read(const struct device *dev, uint32_t addr,
			  uint8_t *data, size_t len)
{
	const struct w5500_config *cfg = dev->config;
	int ret;

	uint8_t cmd[3] = {
		addr >> 8,
		addr,
		W5500_SPI_READ_CONTROL(addr)
	};
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
		{
			.buf = NULL,
			.len = 3
		},
		{
			.buf = data,
			.len = len
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);

	return ret;
}

static int w5500_spi_write(const struct device *dev, uint32_t addr,
			   uint8_t *data, size_t len)
{
	const struct w5500_config *cfg = dev->config;
	int ret;
	uint8_t cmd[3] = {
		addr >> 8,
		addr,
		W5500_SPI_WRITE_CONTROL(addr),
	};
	const struct spi_buf tx_buf[2] = {
		{
			.buf = cmd,
			.len = ARRAY_SIZE(cmd),
		},
		{
			.buf = data,
			.len = len,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	ret = spi_write_dt(&cfg->spi, &tx);

	return ret;
}

static int w5500_readbuf(const struct device *dev, uint16_t offset, uint8_t *buf,
			 size_t len)
{
	uint32_t addr;
	size_t remain = 0;
	int ret;
	const uint32_t mem_start = W5500_Sn_RX_MEM_START;
	const uint32_t mem_size = W5500_RX_MEM_SIZE;

	offset %= mem_size;
	addr = mem_start + offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = w5500_spi_read(dev, addr, buf, len);
	if (ret) {
		return ret;
	}

	if (remain) {
		ret = w5500_spi_read(dev, mem_start, buf + len, remain);
	}

	return ret;
}

static int w5500_writebuf(const struct device *dev, uint16_t offset, uint8_t *buf,
			  size_t len)
{
	uint32_t addr;
	size_t remain = 0;
	int ret;
	const uint32_t mem_start = W5500_Sn_TX_MEM_START;
	const uint32_t mem_size = W5500_TX_MEM_SIZE;

	offset %= mem_size;
	addr = mem_start + offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = w5500_spi_write(dev, addr, buf, len);
	if (ret) {
		return ret;
	}

	if (remain) {
		ret = w5500_spi_write(dev, mem_start, buf + len, remain);
	}

	return ret;
}

static int w5500_command(const struct device *dev, uint8_t cmd)
{
	int ret;
	uint8_t reg;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(100));

	ret = w5500_spi_write(dev, W5500_S0_CR, &cmd, 1);
	if (ret) {
		return ret;
	}

	while (true) {
		ret = w5500_spi_read(dev, W5500_S0_CR, &reg, 1);
		if (ret) {
			return ret;
		}

		if (!reg) {
			break;
		}

		if (sys_timepoint_expired(end)) {
			return -EIO;
		}

		k_busy_wait(W5500_PHY_ACCESS_DELAY);
	}

	return 0;
}

static int w5500_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct w5500_runtime *ctx = dev->data;
	uint16_t len = (uint16_t)net_pkt_get_len(pkt);
	uint16_t offset;
	uint8_t off[2];
	int ret;

	w5500_spi_read(dev, W5500_S0_TX_WR, off, 2);
	offset = sys_get_be16(off);

	ret = net_pkt_read(pkt, ctx->buf, len);
	if (ret) {
		LOG_ERR("net_pkt_read %d", ret);
		return ret;
	}

	ret = w5500_writebuf(dev, offset, ctx->buf, len);
	if (ret) {
		LOG_ERR("writebuf failed %d", ret);
		return ret;
	}

	sys_put_be16(offset + len, off);
	ret = w5500_spi_write(dev, W5500_S0_TX_WR, off, 2);
	if (ret) {
		LOG_ERR("S0_TX_WR %d", ret);
		return ret;
	}

	w5500_command(dev, S0_CR_SEND);
	if (k_sem_take(&ctx->tx_sem, K_MSEC(10))) {
		uint8_t TX_RD[2];
		uint16_t rd_off;
		uint16_t wd_off = offset + len;

		w5500_spi_read(dev, W5500_S0_TX_RD, TX_RD, 2);
		rd_off = sys_get_be16(TX_RD);
		LOG_INF("SEM timed-out tx_wd:%d tx_rd:%d", wd_off, rd_off);
		/* if wd_off == rd_off, the gpio-int trigger was missed
		 * and we can pass on, otherwise refresh the SEND command.
		 */
		if (wd_off > rd_off) {
			w5500_command(dev, S0_CR_SEND);
			if (k_sem_take(&ctx->tx_sem, K_MSEC(10))) {
				return -EIO;
			}
		}
	}

	return 0;
}

static void w5500_rx(const struct device *dev)
{
	int ret;
	uint8_t header[2];
	uint8_t tmp[2];
	uint16_t off;
	uint16_t rx_len;
	uint16_t rx_buf_len;
	uint16_t read_len;
	uint16_t reader;
	struct net_buf *pkt_buf = NULL;
	struct net_pkt *pkt;
	struct w5500_runtime *ctx = dev->data;

	ret = w5500_spi_read(dev, W5500_S0_RX_RSR, tmp, 2);
	if (ret) {
		LOG_ERR("Failed to get S0 RSR");
		return;
	}
	rx_buf_len = sys_get_be16(tmp);

	if (rx_buf_len == 0) {
		return;
	}

	ret = w5500_spi_read(dev, W5500_S0_RX_RD, tmp, 2);
	if (ret) {
		LOG_ERR("Failed to get S0 RD");
		return;
	}
	off = sys_get_be16(tmp);

	if (w5500_readbuf(dev, off, header, 2) < 0) {
		LOG_ERR("Failed to get header size");
		return;
	}

	if (sys_get_be16(header) <= 2U) {
		LOG_ERR("Invalid W5500 header size %u", sys_get_be16(header));
		return;
	}
	rx_len = sys_get_be16(header) - 2;

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, rx_len, NET_AF_UNSPEC, 0,
					   K_MSEC(CONFIG_ETH_W5500_TIMEOUT));
	if (!pkt) {
		eth_stats_update_errors_rx(ctx->iface);
		LOG_ERR("Failed rx allocation");
		return;
	}

	pkt_buf = pkt->buffer;

	read_len = rx_len;
	reader = off + 2;

	do {
		size_t frag_len;
		uint8_t *data_ptr;
		size_t frame_len;

		data_ptr = pkt_buf->data;

		frag_len = net_buf_tailroom(pkt_buf);

		if (read_len > frag_len) {
			frame_len = frag_len;
		} else {
			frame_len = read_len;
		}

		ret = w5500_readbuf(dev, reader, data_ptr, frame_len);
		if (!ret) {
			net_buf_add(pkt_buf, frame_len);
			reader += (uint16_t)frame_len;

			read_len -= (uint16_t)frame_len;
			pkt_buf = pkt_buf->frags;
		} else {
			/* something went wrong set read_len zero */
			read_len = 0;
			LOG_ERR("Failed read data blk S0");
		}
	} while (read_len > 0);

	if (net_recv_data(ctx->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	sys_put_be16(off + 2 + rx_len, tmp);
	w5500_spi_write(dev, W5500_S0_RX_RD, tmp, 2);
	w5500_command(dev, S0_CR_RECV);
}

static void w5500_update_link_status(const struct device *dev)
{
	uint8_t phycfgr;
	struct w5500_runtime *ctx = dev->data;
	enum phy_link_speed speed;

	if (w5500_spi_read(dev, W5500_PHYCFGR, &phycfgr, 1) < 0) {
		return;
	}

	if (IS_BIT_SET(phycfgr, W5500_PHYCFGR_LNK_BIT)) {
		if (ctx->state.is_up != true) {
			ctx->state.is_up = true;
			net_eth_carrier_on(ctx->iface);
		}

		if (IS_BIT_SET(phycfgr, W5500_PHYCFGR_SPD_BIT)) {
			speed = IS_BIT_SET(phycfgr, W5500_PHYCFGR_DPX_BIT) ? LINK_FULL_100BASE
									   : LINK_HALF_100BASE;
		} else {
			speed = IS_BIT_SET(phycfgr, W5500_PHYCFGR_DPX_BIT) ? LINK_FULL_10BASE
									   : LINK_HALF_10BASE;
		}

		if (ctx->state.speed != speed) {
			ctx->state.speed = speed;
			LOG_INF("%s: Link speed %s Mb, %s duplex", dev->name,
				PHY_LINK_IS_SPEED_100M(speed) ? "100" : "10",
				PHY_LINK_IS_FULL_DUPLEX(speed) ? "full" : "half");
		}

		return;
	}

	if (ctx->state.is_up) {
		ctx->state.is_up = false;
		ctx->state.speed = 0;
		net_eth_carrier_off(ctx->iface);
	}
}

static uint8_t w5500_check_for_ir(const struct device *dev)
{
	uint8_t ir;
	struct w5500_runtime *ctx = dev->data;

	w5500_spi_read(dev, W5500_S0_IR, &ir, 1);

	if (ir != 0U) {

		if ((ir & S0_IR_SENDOK) != 0U) {
			uint8_t release = S0_IR_SENDOK;

			w5500_spi_write(dev, W5500_S0_IR, &release, 1);
			k_sem_give(&ctx->tx_sem);
			LOG_DBG("TX Done");
		} else if ((ir & S0_IR_RECV) != 0U) {
			uint8_t release = S0_IR_RECV;

			w5500_spi_write(dev, W5500_S0_IR, &release, 1);
			k_sem_give(&ctx->rx_sem);
			LOG_DBG("RX Done");
		} else {
			/* release other interrupt trigger */
			w5500_spi_write(dev, W5500_S0_IR, &ir, 1);
			LOG_DBG("IR Others released");
		}
	}

	return ir;
}

#if !DT_ALL_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void w5500_thread_poll(const struct device *dev)
{
	struct w5500_runtime *ctx = dev->data;

	if (!ctx->state.is_up) {
		k_msleep(CONFIG_ETH_W5500_POLL_PERIOD);
		w5500_update_link_status(dev);
		return;
	}

	k_msleep(CONFIG_ETH_W5500_POLL_PERIOD);

	if (w5500_check_for_ir(dev) == 0U) {
		w5500_update_link_status(dev);
	}
}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void w5500_thread_interrupt(const struct device *dev)
{
	int res;
	struct w5500_runtime *ctx = dev->data;
	const struct w5500_config *config = dev->config;

	for (;;) {
		res = k_sem_take(&ctx->int_sem, K_MSEC(CONFIG_ETH_W5500_MONITOR_PERIOD));
		if (res == 0) {
			if (!ctx->state.is_up) {
				w5500_update_link_status(dev);
			}

			w5500_check_for_ir(dev);
			if (gpio_pin_get_dt(&config->interrupt)) {
				/* still have the interrupt pin low */
				k_sem_give(&ctx->int_sem);
			}
		} else if (res == -EAGAIN) {
			w5500_update_link_status(dev);
		}
	}
}
#endif

static void w5500_thread_rx(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int res;
	const struct device *dev = p1;
	struct w5500_runtime *ctx = dev->data;

	while (true) {
		res = k_sem_take(&ctx->rx_sem, K_MSEC(200));
		w5500_rx(dev);
	}
};

static void w5500_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	const struct w5500_config *config = dev->config;

	if (DT_ALL_INST_HAS_PROP_STATUS_OKAY(int_gpios) ||
		(config->interrupt.port != NULL)) {
		w5500_thread_interrupt(dev);
	} else {
		LOG_ERR("w5500 driver no interrupt pin");
	}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY */

#if !DT_ALL_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	while (true) {
		/* polling mode, no INT pin is defined */
		w5500_thread_poll(dev);
	}
#endif /* !DT_ALL_INST_HAS_PROP_STATUS_OKAY */
}

static void w5500_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct w5500_runtime *ctx = dev->data;

	/* configure Socket 0 with MACRAW mode and MAC filtering enabled */
	uint8_t mode = S0_MR_MACRAW | BIT(W5500_S0_MR_MF);

	w5500_spi_write(dev, W5500_S0_MR, &mode, 1);

	net_if_set_link_addr(iface, ctx->mac_addr,
			     sizeof(ctx->mac_addr),
			     NET_LINK_ETHERNET);

	ctx->iface = iface;

	ethernet_init(iface);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);

	/* Fetch initial link status */
	w5500_update_link_status(dev);

	k_thread_create(&ctx->thread_rx, ctx->thread_stack_rx,
			CONFIG_ETH_W5500_RX_THREAD_STACK_SIZE,
			w5500_thread_rx,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_W5500_RX_THREAD_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&ctx->thread_rx, "eth_w5500_rx");

	k_thread_create(&ctx->thread, ctx->thread_stack,
			CONFIG_ETH_W5500_RX_THREAD_STACK_SIZE,
			w5500_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_W5500_RX_THREAD_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&ctx->thread, "eth_w5500_main");

}

static enum ethernet_hw_caps w5500_get_capabilities(const struct device *dev __unused,
						    struct net_if *iface __unused)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
	;
}

static int w5500_set_config(const struct device *dev,
			    struct net_if *iface __unused,
			    enum ethernet_config_type type,
			    const struct ethernet_config *config)
{
	struct w5500_runtime *ctx = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(ctx->mac_addr,
			config->mac_address.addr,
			sizeof(ctx->mac_addr));
		w5500_spi_write(dev, W5500_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x",
			dev->name,
			ctx->mac_addr[0], ctx->mac_addr[1],
			ctx->mac_addr[2], ctx->mac_addr[3],
			ctx->mac_addr[4], ctx->mac_addr[5]);

		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			uint8_t mode;
			uint8_t mr = W5500_S0_MR_MF;

			w5500_spi_read(dev, W5500_S0_MR, &mode, 1);

			if (config->promisc_mode) {
				if (!(mode & BIT(mr))) {
					return -EALREADY;
				}

				/* disable MAC filtering */
				WRITE_BIT(mode, mr, 0);
			} else {
				if (mode & BIT(mr)) {
					return -EALREADY;
				}

				/* enable MAC filtering */
				WRITE_BIT(mode, mr, 1);
			}

			return w5500_spi_write(dev, W5500_S0_MR, &mode, 1);
		}

		return -ENOTSUP;
	default:
		return -ENOTSUP;
	}
}

static int w5500_hw_start(const struct device *dev, struct net_if *iface __unused)
{
	uint8_t mask = IR_S0;

	w5500_command(dev, S0_CR_OPEN);

	/* enable interrupt */
	w5500_spi_write(dev, W5500_SIMR, &mask, 1);

	return 0;
}

static int w5500_hw_stop(const struct device *dev, struct net_if *iface __unused)
{
	uint8_t mask = 0;

	/* disable interrupt */
	w5500_spi_write(dev, W5500_SIMR, &mask, 1);
	w5500_command(dev, S0_CR_CLOSE);

	return 0;
}

static const struct device *w5500_get_phy(const struct device *dev, struct net_if *iface __unused)
{
	const struct w5500_config *config = dev->config;

	return config->phy_dev;
}

static const struct ethernet_api w5500_api_funcs = {
	.iface_api.init = w5500_iface_init,
	.get_capabilities = w5500_get_capabilities,
	.set_config = w5500_set_config,
	.start = w5500_hw_start,
	.stop = w5500_hw_stop,
	.get_phy = w5500_get_phy,
	.send = w5500_tx,
};

static int w5500_get_link_state(const struct device *dev,
				  struct phy_link_state *state)
{
	struct w5500_runtime *const data = dev->data;

	state->speed = data->state.speed;
	state->is_up = data->state.is_up;

	return 0;
}

static DEVICE_API(ethphy, w5500_phy_driver_api) = {
	.get_link = w5500_get_link_state,
};

static int w5500_soft_reset(const struct device *dev)
{
	int ret;
	uint8_t mask = 0;
	uint8_t tmp = MR_RST;

	ret = w5500_spi_write(dev, W5500_MR, &tmp, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(5);
	tmp = MR_PB;
	w5500_spi_write(dev, W5500_MR, &tmp, 1);

	/* disable interrupt */
	return w5500_spi_write(dev, W5500_SIMR, &mask, 1);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void w5500_gpio_callback(const struct device *dev,
				struct gpio_callback *cb,
				uint32_t pins)
{
	struct w5500_runtime *ctx =
		CONTAINER_OF(cb, struct w5500_runtime, gpio_cb);

	k_sem_give(&ctx->int_sem);
}
#endif

static void w5500_set_macaddr(const struct device *dev)
{
	struct w5500_runtime *ctx = dev->data;

	w5500_spi_write(dev, W5500_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
}

static void w5500_memory_configure(const struct device *dev)
{
	int i;
	uint8_t mem = 0x10;

	/* Configure RX & TX memory to 16K */
	w5500_spi_write(dev, W5500_Sn_RXMEM_SIZE(0), &mem, 1);
	w5500_spi_write(dev, W5500_Sn_TXMEM_SIZE(0), &mem, 1);

	mem = 0;
	for (i = 1; i < 8; i++) {
		w5500_spi_write(dev, W5500_Sn_RXMEM_SIZE(i), &mem, 1);
		w5500_spi_write(dev, W5500_Sn_TXMEM_SIZE(i), &mem, 1);
	}
}

static int w5500_init(const struct device *dev)
{
	int err;
	uint8_t rtr[2];
	const struct w5500_config *config = dev->config;
	struct w5500_runtime *ctx = dev->data;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI master port %s not ready", config->spi.bus->name);
		return -EINVAL;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
	if (DT_ALL_INST_HAS_PROP_STATUS_OKAY(int_gpios) || (config->interrupt.port != NULL)) {
		if (!gpio_is_ready_dt(&config->interrupt)) {
			LOG_ERR("GPIO port %s not ready", config->interrupt.port->name);
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&config->interrupt, GPIO_INPUT);
		if (err < 0) {
			LOG_ERR("Unable to configure GPIO pin %u", config->interrupt.pin);
			return err;
		}

		gpio_init_callback(&(ctx->gpio_cb), w5500_gpio_callback,
				BIT(config->interrupt.pin));
		err = gpio_add_callback(config->interrupt.port, &(ctx->gpio_cb));
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
		LOG_INF("%s: interrupt mode", dev->name);
	} else {
		LOG_WRN("%s: polling mode", dev->name);
	}
#else
	LOG_INF("%s: polling mode", dev->name);
#endif

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

		/* See Section 5.5.1 of the W5500 datasheet
		 * Trc = 500us
		 * Tpl = 1ms
		 */
		gpio_pin_set_dt(&config->reset, 1);
		k_usleep(500);
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(1);
	}

	err = w5500_soft_reset(dev);
	if (err != 0) {
		LOG_ERR("Reset failed");
		return err;
	}

	(void)net_eth_mac_load(&config->mac_cfg, ctx->mac_addr);
	w5500_set_macaddr(dev);
	w5500_memory_configure(dev);

	/* check retry time value */
	w5500_spi_read(dev, W5500_RTR, rtr, 2);
	if (sys_get_be16(rtr) != RTR_DEFAULT) {
		LOG_ERR("Unable to read RTR register");
		return -ENODEV;
	}

	LOG_INF("W5500 Initialized");

	return 0;
}

#define W5500_INST_DEFINE(inst)                                                           \
	DEVICE_DECLARE(eth_w5500_phy_##inst);                                             \
	static struct w5500_runtime w5500_runtime_##inst = {                              \
		.tx_sem  = Z_SEM_INITIALIZER(w5500_runtime_##inst.tx_sem, 1, UINT_MAX),   \
		.rx_sem  = Z_SEM_INITIALIZER(w5500_runtime_##inst.rx_sem, 0, UINT_MAX),   \
		.int_sem = Z_SEM_INITIALIZER(w5500_runtime_##inst.int_sem, 0, UINT_MAX),  \
	};                                                                                \
	static const struct w5500_config w5500_config_##inst = {                          \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)),                       \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, int_gpios),                        \
			(.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),))   \
		.reset   = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),              \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),                         \
		.phy_dev = DEVICE_GET(eth_w5500_phy_##inst),                              \
	};                                                                                \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, w5500_init, NULL,                             \
				      &w5500_runtime_##inst, &w5500_config_##inst,        \
				      CONFIG_ETH_INIT_PRIORITY, &w5500_api_funcs,         \
				      NET_ETH_MTU);                                       \
	DEVICE_DEFINE(eth_w5500_phy_##inst,                                               \
		      DEVICE_DT_NAME(DT_DRV_INST(inst)) "_phy",                           \
		      NULL, NULL,                                                         \
		      &w5500_runtime_##inst, &w5500_config_##inst,                        \
		      POST_KERNEL, CONFIG_ETH_INIT_PRIORITY,                              \
		      &w5500_phy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(W5500_INST_DEFINE)
