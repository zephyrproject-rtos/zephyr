/*
 * SPDX-FileCopyrightText: Copyright 2020 Linumiz
 * SPDX-FileCopyrightText: Copyright 2026 Sayed Naser Moravej
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	wiznet_w6100

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_w6100, CONFIG_ETHERNET_LOG_LEVEL);

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
#include "eth_w6100_priv.h"

#define W6100_SPI_BLOCK_SELECT(addr)	(((addr) >> 16) & 0x1f)
#define W6100_SPI_READ_CONTROL(addr)	(W6100_SPI_BLOCK_SELECT(addr) << 3)
#define W6100_SPI_WRITE_CONTROL(addr)   \
	((W6100_SPI_BLOCK_SELECT(addr) << 3) | BIT(2))

static int w6100_spi_read(const struct device *dev, uint32_t addr,
			  uint8_t *data, size_t len)
{
	const struct w6100_config *cfg = dev->config;

	uint8_t cmd[3] = {
		addr >> 8,
		addr,
		W6100_SPI_READ_CONTROL(addr)
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

	return spi_transceive_dt(&cfg->spi, &tx, &rx);
}

static int w6100_spi_write(const struct device *dev, uint32_t addr,
			   uint8_t *data, size_t len)
{
	const struct w6100_config *cfg = dev->config;
	uint8_t cmd[3] = {
		addr >> 8,
		addr,
		W6100_SPI_WRITE_CONTROL(addr),
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

	return spi_write_dt(&cfg->spi, &tx);

}

static int w6100_readbuf(const struct device *dev, uint16_t offset, uint8_t *buf,
			 size_t len)
{
	uint32_t addr;
	size_t remain = 0;
	int ret;
	const uint32_t mem_start = W6100_Sn_RX_MEM_START;
	const uint32_t mem_size = W6100_RX_MEM_SIZE;

	offset %= mem_size;
	addr = mem_start + offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = w6100_spi_read(dev, addr, buf, len);
	if (ret < 0 || remain == 0U) {
		return ret;
	}

	return w6100_spi_read(dev, mem_start, buf + len, remain);
}

static int w6100_writebuf(const struct device *dev, uint16_t offset, uint8_t *buf,
			  size_t len)
{
	uint32_t addr;
	const uint32_t mem_start = W6100_Sn_TX_MEM_START;

	__ASSERT_NO_MSG(offset + len <= W6100_TX_MEM_SIZE);
	addr = mem_start + offset;

	return w6100_spi_write(dev, addr, buf, len);
}

static int w6100_command(const struct device *dev, uint8_t cmd)
{
	uint8_t reg;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(CONFIG_ETH_W6100_TIMEOUT));

	w6100_spi_write(dev, W6100_S0_CR, &cmd, 1);
	while (true) {
		w6100_spi_read(dev, W6100_S0_CR, &reg, 1);
		if (!reg) {
			break;
		}
		if (sys_timepoint_expired(end)) {
			return -EIO;
		}
		k_busy_wait(W6100_PHY_ACCESS_DELAY);
	}
	return 0;
}

static int w6100_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct w6100_runtime *ctx = dev->data;
	uint16_t len = (uint16_t)net_pkt_get_len(pkt);
	uint16_t offset;
	uint8_t off[2];
	int ret;

	w6100_spi_read(dev, W6100_S0_TX_WR, off, 2);
	offset = sys_get_be16(off);

	if (net_pkt_read(pkt, ctx->buf, len)) {
		return -EIO;
	}
	ret = w6100_writebuf(dev, offset, ctx->buf, len);
	if (ret < 0) {
		LOG_ERR("returning");
		return ret;
	}

	sys_put_be16(offset + len, off);
	w6100_spi_write(dev, W6100_S0_TX_WR, off, 2);

	w6100_command(dev, S0_CR_SEND);
	if (k_sem_take(&ctx->tx_sem, K_MSEC(10))) {
		return -EIO;
	}

	return 0;
}

static void w6100_rx(const struct device *dev)
{
	uint8_t header[2];
	uint8_t tmp[2];
	uint16_t off;
	uint16_t rx_len;
	uint16_t rx_buf_len;
	uint16_t read_len;
	uint16_t reader;
	struct net_buf *pkt_buf = NULL;
	struct net_pkt *pkt;
	struct w6100_runtime *ctx = dev->data;

	w6100_spi_read(dev, W6100_S0_RX_RSR, tmp, 2);
	rx_buf_len = sys_get_be16(tmp);
	if (rx_buf_len == 0) {
		return;
	}

	w6100_spi_read(dev, W6100_S0_RX_RD, tmp, 2);
	off = sys_get_be16(tmp);

	w6100_readbuf(dev, off, header, 2);
	rx_len = sys_get_be16(header) - 2;

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, rx_len,
			NET_AF_UNSPEC, 0, K_MSEC(CONFIG_ETH_W6100_TIMEOUT));
	if (!pkt) {
		eth_stats_update_errors_rx(ctx->iface);
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

		w6100_readbuf(dev, reader, data_ptr, frame_len);
		net_buf_add(pkt_buf, frame_len);
		reader += (uint16_t)frame_len;

		read_len -= (uint16_t)frame_len;
		pkt_buf = pkt_buf->frags;
	} while (read_len > 0);

	if (net_recv_data(ctx->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	sys_put_be16(off + 2 + rx_len, tmp);
	w6100_spi_write(dev, W6100_S0_RX_RD, tmp, 2);
	w6100_command(dev, S0_CR_RECV);
}

static void w6100_update_link_status(const struct device *dev)
{
	uint8_t physr;
	struct w6100_runtime *ctx = dev->data;
	enum phy_link_speed speed;

	if (w6100_spi_read(dev, W6100_PHYSR, &physr, 1) < 0) {
		return;
	}

	if (IS_BIT_SET(physr, W6100_PHYSR_LNK_BIT)) {
		if (ctx->state.is_up != true) {
			LOG_INF("%s: Link up", dev->name);
			ctx->state.is_up = true;
			net_eth_carrier_on(ctx->iface);
		}

		if (IS_BIT_SET(physr, W6100_PHYSR_SPD_BIT)) {
			speed = IS_BIT_SET(physr, W6100_PHYSR_DPX_BIT) ? LINK_FULL_100BASE
									   : LINK_HALF_100BASE;
		} else {
			speed = IS_BIT_SET(physr, W6100_PHYSR_DPX_BIT) ? LINK_FULL_10BASE
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
		LOG_INF("%s: Link down", dev->name);
		ctx->state.is_up = false;
		ctx->state.speed = 0;
		net_eth_carrier_off(ctx->iface);
	}
}

static void w6100_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	uint8_t ir;
	uint8_t slir;
	int res;
	struct w6100_runtime *ctx = dev->data;
	const struct w6100_config *config = dev->config;

	while (true) {

		res = k_sem_take(&ctx->int_sem, K_MSEC(CONFIG_ETH_W6100_MONITOR_PERIOD));

		if (res == -EAGAIN) {
			/* semaphore timeout period expired, check link status */
			w6100_update_link_status(dev);
			continue;
		}

		if (res != 0) {
			continue;
		}

		/* semaphore taken, update link status and receive packets */
		if (ctx->state.is_up != true) {
			w6100_update_link_status(dev);
		}

		while (gpio_pin_get_dt(&(config->interrupt))) {
			/* Read interrupt */
			w6100_spi_read(dev, W6100_S0_IR, &ir, 1);
			w6100_spi_read(dev, W6100_SLIR, &slir, 1);
			w6100_spi_write(dev, W6100_SLIRCLR, &slir, 1);


			if (ir) {
				/* Clear interrupt */
				w6100_spi_write(dev, W6100_S0_IR + 8, &ir, 1);
			}

			if (ir & S0_IR_SENDOK) {
				k_sem_give(&ctx->tx_sem);
			}

			if (ir & S0_IR_RECV) {
				w6100_rx(dev);
			}
		}
	}
}

static void w6100_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct w6100_runtime *ctx = dev->data;

	net_if_set_link_addr(iface, ctx->mac_addr,
			     sizeof(ctx->mac_addr),
			     NET_LINK_ETHERNET);

	if (ctx->iface == NULL) {
		ctx->iface = iface;
	}

	ethernet_init(iface);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);
}

static enum ethernet_hw_caps w6100_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_HW_FILTERING
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
	;
}

static int w6100_set_config(const struct device *dev,
			    enum ethernet_config_type type,
			    const struct ethernet_config *config)
{
	struct w6100_runtime *ctx = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(ctx->mac_addr,
			config->mac_address.addr,
			sizeof(ctx->mac_addr));
		uint8_t netlckrStatus = NETLCKR_UNLOCK;

		w6100_spi_write(dev, W6100_NETLCKR, &netlckrStatus, 1);
		w6100_spi_write(dev, W6100_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
		netlckrStatus = NETLCKR_LOCK;
		w6100_spi_write(dev, W6100_NETLCKR, &netlckrStatus, 1);
		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x",
			dev->name,
			ctx->mac_addr[0], ctx->mac_addr[1],
			ctx->mac_addr[2], ctx->mac_addr[3],
			ctx->mac_addr[4], ctx->mac_addr[5]);

		/* Register Ethernet MAC Address with the upper layer */
		net_if_set_link_addr(ctx->iface, ctx->mac_addr,
			sizeof(ctx->mac_addr),
			NET_LINK_ETHERNET);

		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			uint8_t mode;
			uint8_t mr = W6100_S0_MR_MF;

			w6100_spi_read(dev, W6100_S0_MR, &mode, 1);

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

			return w6100_spi_write(dev, W6100_S0_MR, &mode, 1);
		}

		return -ENOTSUP;
	default:
		return -ENOTSUP;
	}
}

static int w6100_hw_start(const struct device *dev)
{
	uint8_t mode = S0_MR_MACRAW | BIT(W6100_S0_MR_MF);
	uint8_t mask = IR_S0;

	/* configure Socket 0 with MACRAW mode and MAC filtering enabled */
	w6100_spi_write(dev, W6100_S0_MR, &mode, 1);
	w6100_command(dev, S0_CR_OPEN);

	/* enable interrupt */
	w6100_spi_write(dev, W6100_SIMR, &mask, 1);
	return 0;
}

static int w6100_hw_stop(const struct device *dev)
{
	uint8_t mask = 0;

	/* disable interrupt */
	w6100_spi_write(dev, W6100_SIMR, &mask, 1);
	w6100_command(dev, S0_CR_CLOSE);

	return 0;
}

static const struct device *w6100_get_phy(const struct device *dev)
{
	const struct w6100_config *config = dev->config;

	return config->phy_dev;
}

static const struct ethernet_api w6100_api_funcs = {
	.iface_api.init = w6100_iface_init,
	.get_capabilities = w6100_get_capabilities,
	.set_config = w6100_set_config,
	.start = w6100_hw_start,
	.stop = w6100_hw_stop,
	.get_phy = w6100_get_phy,
	.send = w6100_tx,
};

static int w6100_get_link_state(const struct device *dev,
				  struct phy_link_state *state)
{
	struct w6100_runtime *const data = dev->data;

	state->speed = data->state.speed;
	state->is_up = data->state.is_up;

	return 0;
}

static DEVICE_API(ethphy, w6100_phy_driver_api) = {
	.get_link = w6100_get_link_state,
};

static int w6100_soft_reset(const struct device *dev)
{
	int ret;

	/*Before writing any command in the SYCR0 and the SYCR1 registers,*/
	/*a CHPLCKR_UNLOCK command should be written into the CHPLCKR register*/
	uint8_t cmd = CHPLCKR_UNLOCK;

	w6100_spi_write(dev, W6100_CHPLCKR, &cmd, 1);
	cmd = SYCR0_RST;
	w6100_spi_write(dev, W6100_SYCR0, &cmd, 1);
	cmd = SYCR0_NORMAL;
	w6100_spi_write(dev, W6100_SYCR0, &cmd, 1);
	cmd = CHPLCKR_LOCK;
	ret = w6100_spi_write(dev, W6100_CHPLCKR, &cmd, 1);

	if (ret < 0) {
		return ret;
	}

	uint8_t mask = 0;

	/* disable interrupt */
	return w6100_spi_write(dev, W6100_SIMR, &mask, 1);
}

static void w6100_gpio_callback(const struct device *dev,
				struct gpio_callback *cb,
				uint32_t pins)
{
	struct w6100_runtime *ctx =
		CONTAINER_OF(cb, struct w6100_runtime, gpio_cb);

	k_sem_give(&ctx->int_sem);
}

static void w6100_set_macaddr(const struct device *dev)
{
	struct w6100_runtime *ctx = dev->data;
	uint8_t netlckrStatus = NETLCKR_UNLOCK;

	w6100_spi_write(dev, W6100_NETLCKR, &netlckrStatus, 1);
	w6100_spi_write(dev, W6100_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
	netlckrStatus = NETLCKR_LOCK;
	w6100_spi_write(dev, W6100_NETLCKR, &netlckrStatus, 1);
}

static void w6100_memory_configure(const struct device *dev)
{
	uint8_t mem = 0x10;

	/* Configure RX & TX memory to 16K */
	w6100_spi_write(dev, W6100_Sn_RXMEM_SIZE(0), &mem, 1);
	w6100_spi_write(dev, W6100_Sn_TXMEM_SIZE(0), &mem, 1);

	mem = 0;
	for (int i = 1; i < 8; i++) {
		w6100_spi_write(dev, W6100_Sn_RXMEM_SIZE(i), &mem, 1);
		w6100_spi_write(dev, W6100_Sn_TXMEM_SIZE(i), &mem, 1);
	}
}

static int w6100_init(const struct device *dev)
{
	int err;
	uint8_t rtr[2];
	const struct w6100_config *config = dev->config;
	struct w6100_runtime *ctx = dev->data;

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

	gpio_init_callback(&(ctx->gpio_cb), w6100_gpio_callback,
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
		k_usleep(T_RST_US);
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(T_STA_mS);
	}

	err = w6100_soft_reset(dev);

	if (err != 0) {
		LOG_ERR("Reset failed");
		return err;
	}

	(void)net_eth_mac_load(&config->mac_cfg, ctx->mac_addr);
	w6100_set_macaddr(dev);
	w6100_memory_configure(dev);

	/* check retry time value */
	w6100_spi_read(dev, W6100_RTR, rtr, 2);
		if (sys_get_be16(rtr) != RTR_DEFAULT) {
			LOG_ERR("Unable to read RTR register");
			return -ENODEV;
		}

	k_thread_create(&ctx->thread, ctx->thread_stack,
			CONFIG_ETH_W6100_RX_THREAD_STACK_SIZE,
			w6100_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_W6100_RX_THREAD_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&ctx->thread, "eth_w6100");

	LOG_INF("W6100 Initialized");

	return 0;
}

#define W6100_INST_DEFINE(inst) \
	DEVICE_DECLARE(eth_w6100_phy_##inst); \
	static struct w6100_runtime w6100_runtime_##inst = { \
		.tx_sem = Z_SEM_INITIALIZER(w6100_runtime_##inst.tx_sem, 1, UINT_MAX), \
		.int_sem  = Z_SEM_INITIALIZER(w6100_runtime_##inst.int_sem, 0, UINT_MAX), \
	}; \
	static const struct w6100_config w6100_config_##inst = { \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)), \
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios), \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, { 0 }), \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst), \
		.phy_dev = DEVICE_GET(eth_w6100_phy_##inst), \
	}; \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, \
			w6100_init, NULL, \
			&w6100_runtime_##inst, &w6100_config_##inst, \
			CONFIG_ETH_INIT_PRIORITY, &w6100_api_funcs, \
			NET_ETH_MTU); \
	DEVICE_DEFINE(eth_w6100_phy_##inst, \
		DEVICE_DT_NAME(DT_DRV_INST(inst)) "_phy", \
		NULL, NULL, &w6100_runtime_##inst, &w6100_config_##inst, \
		POST_KERNEL, CONFIG_ETH_INIT_PRIORITY, &w6100_phy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(W6100_INST_DEFINE)
