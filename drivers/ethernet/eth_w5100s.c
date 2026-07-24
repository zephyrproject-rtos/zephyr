/* W5100S Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wiznet_w5100s

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_w5100s, CONFIG_ETHERNET_LOG_LEVEL);

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
#include "eth_w5100s_priv.h"

#define W5100S_SPI_READ_OPCODE  0x0F
#define W5100S_SPI_WRITE_OPCODE 0xF0

/*
 * W5100S SPI frame: opcode (read/write) + 16-bit offset address + N data bytes.
 * The internal address auto-increments, so a single frame streams a burst.
 */
static int w5100s_spi_read(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct w5100s_config *cfg = dev->config;
	int ret;

	uint8_t cmd[3] = {
		W5100S_SPI_READ_OPCODE,
		addr >> 8,
		addr,
	};
	const struct spi_buf tx_buf = {
		.buf = cmd,
		.len = ARRAY_SIZE(cmd),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	/* skip the bytes clocked out during the command phase */
	const struct spi_buf rx_buf[2] = {
		{.buf = NULL, .len = ARRAY_SIZE(cmd)},
		{.buf = data, .len = len},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);

	return ret;
}

static int w5100s_spi_write(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct w5100s_config *cfg = dev->config;
	int ret;
	uint8_t cmd[3] = {
		W5100S_SPI_WRITE_OPCODE,
		addr >> 8,
		addr,
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

static int w5100s_readbuf(const struct device *dev, uint16_t offset, uint8_t *buf, size_t len)
{
	uint32_t addr;
	size_t remain = 0;
	int ret;
	const uint32_t mem_start = W5100S_S0_RX_MEM_START;
	const uint32_t mem_size = W5100S_RX_MEM_SIZE;

	offset %= mem_size;
	addr = mem_start + offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = w5100s_spi_read(dev, addr, buf, len);
	if (ret || !remain) {
		return ret;
	}

	return w5100s_spi_read(dev, mem_start, buf + len, remain);
}

static int w5100s_writebuf(const struct device *dev, uint16_t offset, uint8_t *buf, size_t len)
{
	uint32_t addr;
	size_t remain = 0;
	int ret;
	const uint32_t mem_start = W5100S_S0_TX_MEM_START;
	const uint32_t mem_size = W5100S_TX_MEM_SIZE;

	offset %= mem_size;
	addr = mem_start + offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = w5100s_spi_write(dev, addr, buf, len);
	if (ret || !remain) {
		return ret;
	}

	return w5100s_spi_write(dev, mem_start, buf + len, remain);
}

static int w5100s_command(const struct device *dev, uint8_t cmd)
{
	uint8_t reg;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(100));

	w5100s_spi_write(dev, W5100S_S0_CR, &cmd, 1);
	while (true) {
		w5100s_spi_read(dev, W5100S_S0_CR, &reg, 1);
		if (!reg) {
			break;
		}
		if (sys_timepoint_expired(end)) {
			return -EIO;
		}
		k_busy_wait(W5100S_PHY_ACCESS_DELAY);
	}
	return 0;
}

static int w5100s_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct w5100s_runtime *ctx = dev->data;
	uint16_t len = (uint16_t)net_pkt_get_len(pkt);
	uint16_t offset;
	uint8_t off[2];
	int ret;

	w5100s_spi_read(dev, W5100S_S0_TX_WR, off, 2);
	offset = sys_get_be16(off);

	if (net_pkt_read(pkt, ctx->buf, len)) {
		return -EIO;
	}

	ret = w5100s_writebuf(dev, offset, ctx->buf, len);
	if (ret < 0) {
		return ret;
	}

	sys_put_be16(offset + len, off);
	w5100s_spi_write(dev, W5100S_S0_TX_WR, off, 2);

	w5100s_command(dev, S0_CR_SEND);
	if (k_sem_take(&ctx->tx_sem, K_MSEC(10))) {
		return -EIO;
	}

	return 0;
}

static void w5100s_rx(const struct device *dev)
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
	struct w5100s_runtime *ctx = dev->data;

	w5100s_spi_read(dev, W5100S_S0_RX_RSR, tmp, 2);
	rx_buf_len = sys_get_be16(tmp);

	if (rx_buf_len == 0) {
		return;
	}

	w5100s_spi_read(dev, W5100S_S0_RX_RD, tmp, 2);
	off = sys_get_be16(tmp);

	w5100s_readbuf(dev, off, header, 2);
	rx_len = sys_get_be16(header) - 2;

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, rx_len, NET_AF_UNSPEC, 0,
					   K_MSEC(CONFIG_ETH_W5100S_TIMEOUT));
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

		w5100s_readbuf(dev, reader, data_ptr, frame_len);
		net_buf_add(pkt_buf, frame_len);
		reader += (uint16_t)frame_len;

		read_len -= (uint16_t)frame_len;
		pkt_buf = pkt_buf->frags;
	} while (read_len > 0);

	if (net_recv_data(ctx->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	sys_put_be16(off + 2 + rx_len, tmp);
	w5100s_spi_write(dev, W5100S_S0_RX_RD, tmp, 2);
	w5100s_command(dev, S0_CR_RECV);
}

static void w5100s_update_link_status(const struct device *dev)
{
	uint8_t physr0;
	struct w5100s_runtime *ctx = dev->data;
	enum phy_link_speed speed;
	bool is_100m;
	bool is_full;

	if (w5100s_spi_read(dev, W5100S_PHYSR0, &physr0, 1) < 0) {
		return;
	}

	if (IS_BIT_SET(physr0, W5100S_PHYSR0_LNK_BIT)) {
		if (ctx->state.is_up != true) {
			ctx->state.is_up = true;
			net_eth_carrier_on(ctx->iface);
		}

		/* PHYSR0 flags: FSPD 1 = 10Mbps, FDPX 1 = half duplex. */
		is_100m = !IS_BIT_SET(physr0, W5100S_PHYSR0_FSPD_BIT);
		is_full = !IS_BIT_SET(physr0, W5100S_PHYSR0_FDPX_BIT);

		if (is_100m) {
			speed = is_full ? LINK_FULL_100BASE : LINK_HALF_100BASE;
		} else {
			speed = is_full ? LINK_FULL_10BASE : LINK_HALF_10BASE;
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

static uint8_t w5100s_check_for_ir(const struct device *dev)
{
	uint8_t ir;
	struct w5100s_runtime *ctx = dev->data;

	w5100s_spi_read(dev, W5100S_S0_IR, &ir, 1);

	if (ir != 0U) {
		w5100s_spi_write(dev, W5100S_S0_IR, &ir, 1);
		LOG_DBG("IR received");

		if ((ir & S0_IR_SENDOK) != 0U) {
			k_sem_give(&ctx->tx_sem);
			LOG_DBG("TX Done");
		}

		if ((ir & S0_IR_RECV) != 0U) {
			w5100s_rx(dev);
			LOG_DBG("RX Done");
		}
	}

	return ir;
}

#if !DT_ALL_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void w5100s_thread_poll(const struct device *dev)
{
	struct w5100s_runtime *ctx = dev->data;

	if (!ctx->state.is_up) {
		k_msleep(CONFIG_ETH_W5100S_POLL_PERIOD);
		w5100s_update_link_status(dev);
		return;
	}

	k_msleep(CONFIG_ETH_W5100S_POLL_PERIOD);

	if (w5100s_check_for_ir(dev) == 0U) {
		w5100s_update_link_status(dev);
	}
}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void w5100s_thread_interrupt(const struct device *dev)
{
	int res;
	struct w5100s_runtime *ctx = dev->data;
	const struct w5100s_config *config = dev->config;

	res = k_sem_take(&ctx->int_sem, K_MSEC(CONFIG_ETH_W5100S_MONITOR_PERIOD));

	if (res == 0) {
		if (!ctx->state.is_up) {
			w5100s_update_link_status(dev);
		}

		while (gpio_pin_get_dt(&config->interrupt)) {
			w5100s_check_for_ir(dev);
		}
	} else if (res == -EAGAIN) {
		w5100s_update_link_status(dev);
	}
}
#endif

static void w5100s_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev __maybe_unused = p1;

	while (true) {

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
		const struct w5100s_config *config = dev->config;

		if (DT_ALL_INST_HAS_PROP_STATUS_OKAY(int_gpios) ||
		    (config->interrupt.port != NULL)) {
			w5100s_thread_interrupt(dev);
			continue;
		}
#endif /* DT_ANY_INST_HAS_PROP_STATUS_OKAY */

#if !DT_ALL_INST_HAS_PROP_STATUS_OKAY(int_gpios)
		/* polling mode, no INT pin is defined */
		w5100s_thread_poll(dev);
#endif /* !DT_ALL_INST_HAS_PROP_STATUS_OKAY */
	}
}

static void w5100s_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct w5100s_runtime *ctx = dev->data;

	/* configure Socket 0 with MACRAW mode and MAC filtering enabled */
	uint8_t mode = S0_MR_MACRAW | BIT(W5100S_S0_MR_MF);

	w5100s_spi_write(dev, W5100S_S0_MR, &mode, 1);

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);

	ctx->iface = iface;

	ethernet_init(iface);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);

	/* Fetch initial link status */
	w5100s_update_link_status(dev);

	k_thread_create(&ctx->thread, ctx->thread_stack, CONFIG_ETH_W5100S_RX_THREAD_STACK_SIZE,
			w5100s_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_W5100S_RX_THREAD_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&ctx->thread, "eth_w5100s");
}

static enum ethernet_hw_caps w5100s_get_capabilities(const struct device *dev __unused,
						     struct net_if *iface __unused)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	       | ETHERNET_PROMISC_MODE
#endif
		;
}

static void w5100s_set_macaddr(const struct device *dev)
{
	struct w5100s_runtime *ctx = dev->data;
	uint8_t unlock = NETLCKR_UNLOCK;

	/* GWR/SUBR/SHAR/SIPR are writable only while NETLCKR is unlocked. */
	w5100s_spi_write(dev, W5100S_NETLCKR, &unlock, 1);
	w5100s_spi_write(dev, W5100S_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
}

static int w5100s_set_config(const struct device *dev, struct net_if *iface __unused,
			     enum ethernet_config_type type, const struct ethernet_config *config)
{
	struct w5100s_runtime *ctx = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(ctx->mac_addr, config->mac_address.addr, sizeof(ctx->mac_addr));
		w5100s_set_macaddr(dev);
		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name, ctx->mac_addr[0],
			ctx->mac_addr[1], ctx->mac_addr[2], ctx->mac_addr[3], ctx->mac_addr[4],
			ctx->mac_addr[5]);

		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			uint8_t mode;
			uint8_t mr = W5100S_S0_MR_MF;

			w5100s_spi_read(dev, W5100S_S0_MR, &mode, 1);

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

			return w5100s_spi_write(dev, W5100S_S0_MR, &mode, 1);
		}

		return -ENOTSUP;
	default:
		return -ENOTSUP;
	}
}

static int w5100s_hw_start(const struct device *dev, struct net_if *iface __unused)
{
	uint8_t mask = IR_S0;

	w5100s_command(dev, S0_CR_OPEN);

	/* enable socket 0 interrupt */
	w5100s_spi_write(dev, W5100S_IMR, &mask, 1);

	return 0;
}

static int w5100s_hw_stop(const struct device *dev, struct net_if *iface __unused)
{
	uint8_t mask = 0;

	/* disable interrupt */
	w5100s_spi_write(dev, W5100S_IMR, &mask, 1);
	w5100s_command(dev, S0_CR_CLOSE);

	return 0;
}

static const struct device *w5100s_get_phy(const struct device *dev, struct net_if *iface __unused)
{
	const struct w5100s_config *config = dev->config;

	return config->phy_dev;
}

static const struct ethernet_api w5100s_api_funcs = {
	.iface_api.init = w5100s_iface_init,
	.get_capabilities = w5100s_get_capabilities,
	.set_config = w5100s_set_config,
	.start = w5100s_hw_start,
	.stop = w5100s_hw_stop,
	.get_phy = w5100s_get_phy,
	.send = w5100s_tx,
};

static int w5100s_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct w5100s_runtime *const data = dev->data;

	state->speed = data->state.speed;
	state->is_up = data->state.is_up;

	return 0;
}

static DEVICE_API(ethphy, w5100s_phy_driver_api) = {
	.get_link = w5100s_get_link_state,
};

static int w5100s_soft_reset(const struct device *dev)
{
	int ret;
	uint8_t mask = 0;
	uint8_t tmp = MR_RST;

	ret = w5100s_spi_write(dev, W5100S_MR, &tmp, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(5);

	/* disable interrupt */
	return w5100s_spi_write(dev, W5100S_IMR, &mask, 1);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)
static void w5100s_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct w5100s_runtime *ctx = CONTAINER_OF(cb, struct w5100s_runtime, gpio_cb);

	k_sem_give(&ctx->int_sem);
}
#endif

static void w5100s_memory_configure(const struct device *dev)
{
	uint8_t mem = W5100S_RX_MEM_SIZE / 1024;

	/* Assign the whole 8 KB RX/TX memory to socket 0 */
	w5100s_spi_write(dev, W5100S_Sn_RXBUF_SIZE(0), &mem, 1);
	w5100s_spi_write(dev, W5100S_Sn_TXBUF_SIZE(0), &mem, 1);

	mem = 0;
	for (int i = 1; i < W5100S_SOCKET_COUNT; i++) {
		w5100s_spi_write(dev, W5100S_Sn_RXBUF_SIZE(i), &mem, 1);
		w5100s_spi_write(dev, W5100S_Sn_TXBUF_SIZE(i), &mem, 1);
	}
}

static int w5100s_init(const struct device *dev)
{
	int err;
	uint8_t verr;
	const struct w5100s_config *config = dev->config;
	struct w5100s_runtime *ctx = dev->data;

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

		gpio_init_callback(&(ctx->gpio_cb), w5100s_gpio_callback,
				   BIT(config->interrupt.pin));
		err = gpio_add_callback(config->interrupt.port, &(ctx->gpio_cb));
		if (err < 0) {
			LOG_ERR("Unable to add GPIO callback %u", config->interrupt.pin);
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_EDGE_FALLING);
		if (err < 0) {
			LOG_ERR("Unable to enable GPIO INT %u", config->interrupt.pin);
			return err;
		}
		LOG_INF("%s: interrupt mode", dev->name);
	} else {
		LOG_INF("%s: polling mode", dev->name);
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

		gpio_pin_set_dt(&config->reset, 1);
		k_usleep(500);
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(100);
	}

	err = w5100s_soft_reset(dev);
	if (err != 0) {
		LOG_ERR("Reset failed");
		return err;
	}

	if (net_eth_mac_load(&config->mac_cfg, ctx->mac_addr) < 0) {
		LOG_ERR("Failed to load MAC address");
		return -EINVAL;
	}
	w5100s_set_macaddr(dev);
	w5100s_memory_configure(dev);

	/* verify the chip is present and talking */
	w5100s_spi_read(dev, W5100S_VERR, &verr, 1);
	if (verr != W5100S_VERSION) {
		LOG_ERR("Unexpected chip version 0x%02x", verr);
		return -ENODEV;
	}

	LOG_INF("W5100S Initialized");

	return 0;
}

/* clang-format off */
#define W5100S_INST_DEFINE(inst) \
	DEVICE_DECLARE(eth_w5100s_phy_##inst); \
	static struct w5100s_runtime w5100s_runtime_##inst = { \
		.tx_sem = Z_SEM_INITIALIZER(w5100s_runtime_##inst.tx_sem, 1, UINT_MAX), \
		.int_sem = Z_SEM_INITIALIZER(w5100s_runtime_##inst.int_sem, 0, UINT_MAX), \
	}; \
	static const struct w5100s_config w5100s_config_##inst = { \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)), \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, int_gpios), \
			(.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),)) \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, { 0 }), \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst), \
		.phy_dev = DEVICE_GET(eth_w5100s_phy_##inst), \
	}; \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, w5100s_init, NULL, \
		&w5100s_runtime_##inst, &w5100s_config_##inst, \
		CONFIG_ETH_INIT_PRIORITY, &w5100s_api_funcs, \
		NET_ETH_MTU); \
	DEVICE_DEFINE(eth_w5100s_phy_##inst, \
		DEVICE_DT_NAME(DT_DRV_INST(inst)) "_phy", \
		NULL, NULL, &w5100s_runtime_##inst, &w5100s_config_##inst, \
		POST_KERNEL, CONFIG_ETH_INIT_PRIORITY, &w5100s_phy_driver_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(W5100S_INST_DEFINE)
