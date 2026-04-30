/* W6300 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2026 WIZnet Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wiznet_w6300

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_w6300, CONFIG_ETHERNET_LOG_LEVEL);

#include <stddef.h>
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
#include "eth_w6300_priv.h"

/* MOD bits for Quad SPI mode in instruction byte */
#define W6300_SPI_MOD_QUAD 0x80 /* MOD[1:0] = 10 → bits[7:6] = 0b10 */

static int w6300_spi_read(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct w6300_config *cfg = dev->config;

	uint8_t cmd[4] = {
		W6300_SPI_MOD_QUAD | W6300_SPI_READ | (addr & 0x1F), (addr >> 16) & 0xFF,
		(addr >> 8) & 0xFF, 0x00, /* dummy */
	};

	const struct spi_buf tx_buf = {.buf = cmd, .len = sizeof(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	const struct spi_buf rx_buf = {.buf = data, .len = len};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	int ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	return ret;
}

static int w6300_spi_write(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct w6300_config *cfg = dev->config;

	uint8_t cmd[4] = {
		W6300_SPI_MOD_QUAD | W6300_SPI_WRITE | (addr & 0x1F), (addr >> 16) & 0xFF,
		(addr >> 8) & 0xFF, 0x00, /* dummy */
	};

	const struct spi_buf tx_bufs[] = {
		{.buf = cmd, .len = sizeof(cmd)},
		{.buf = data, .len = len},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	int ret = spi_write_dt(&cfg->spi, &tx);
	return ret;
}

/*
 * W6300 buffer I/O: The chip handles circular buffer wrapping internally,
 * so we simply address (offset << 8) | block and let the chip auto-increment.
 */
static int w6300_readbuf(const struct device *dev, uint16_t offset, uint8_t *buf, size_t len)
{
	uint32_t addr = ((uint32_t)offset << 8) | W6300_S0_RX_MEM_BLOCK;

	return w6300_spi_read(dev, addr, buf, len);
}

static int w6300_writebuf(const struct device *dev, uint16_t offset, uint8_t *buf, size_t len)
{
	uint32_t addr = ((uint32_t)offset << 8) | W6300_S0_TX_MEM_BLOCK;

	return w6300_spi_write(dev, addr, buf, len);
}

static int w6300_command(const struct device *dev, uint8_t cmd)
{
	uint8_t reg;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(100));

	w6300_spi_write(dev, W6300_S0_CR, &cmd, 1);
	while (true) {
		w6300_spi_read(dev, W6300_S0_CR, &reg, 1);
		if (!reg) {
			break;
		}
		if (sys_timepoint_expired(end)) {
			LOG_ERR_RATELIMIT_RATE(1000, "%s: S0_CR timeout cmd=0x%02x cr=0x%02x",
					       dev->name, cmd, reg);
			return -EIO;
		}
		/*
		 * Yield instead of busy-waiting: the CR register clears
		 * within a few microseconds, but busy-waiting blocks other
		 * threads (including the net TX thread) from running.
		 * k_yield() releases the CPU while keeping the poll tight
		 * enough to detect completion quickly.
		 */
		k_yield();
	}
	return 0;
}

/*
 * w6300_send_cmd - issue a socket command with S0_CR serialization.
 *
 * The net TX thread (w6300_tx) and the RX/IRQ thread (w6300_thread) both
 * write to S0_CR.  On the 33 MHz PIO QSPI bus the k_yield() calls inside
 * the QSPI driver allow these two threads to interleave SPI transactions,
 * so a SEND issued by w6300_tx can be overwritten by a concurrent RECV
 * from w6300_thread (or vice versa).  cmd_lock prevents this.
 */
static int w6300_send_cmd(const struct device *dev, uint8_t cmd)
{
	struct w6300_runtime *ctx = dev->data;
	int ret;

	k_mutex_lock(&ctx->cmd_lock, K_FOREVER);
	ret = w6300_command(dev, cmd);
	k_mutex_unlock(&ctx->cmd_lock);
	return ret;
}

static int w6300_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct w6300_runtime *ctx = dev->data;
	uint16_t len = (uint16_t)net_pkt_get_len(pkt);
	uint16_t offset;
	uint8_t off[2];
	int ret = 0;

	k_mutex_lock(&ctx->tx_lock, K_FOREVER);
	w6300_spi_read(dev, W6300_S0_TX_WR, off, 2);
	offset = sys_get_be16(off);

	if (net_pkt_read(pkt, ctx->buf, len)) {
		ret = -EIO;
		goto out;
	}

	ret = w6300_writebuf(dev, offset, ctx->buf, len);
	if (ret < 0) {
		goto out;
	}

	sys_put_be16(offset + len, off);
	w6300_spi_write(dev, W6300_S0_TX_WR, off, 2);

	k_sem_reset(&ctx->tx_sem);
	ret = w6300_send_cmd(dev, S0_CR_SEND);
	if (ret < 0) {
		goto out;
	}

	if (k_sem_take(&ctx->tx_sem, K_MSEC(100))) {
		uint8_t ir, sr;

		w6300_spi_read(dev, W6300_S0_IR, &ir, 1);
		w6300_spi_read(dev, W6300_S0_SR, &sr, 1);
		LOG_ERR_RATELIMIT_RATE(1000, "%s: TX timeout ir=0x%02x sr=0x%02x len=%u", dev->name,
				       ir, sr, len);
		ret = -EIO;
		goto out;
	}

out:
	k_mutex_unlock(&ctx->tx_lock);
	return ret;
}

static void w6300_rx(const struct device *dev)
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
	struct w6300_runtime *ctx = dev->data;
	const struct w6300_config *config = dev->config;

	/*
	 * Consume one frame per call.
	 *
	 * Under sustained traffic the RX buffer can stay non-empty for long
	 * periods. Draining the whole buffer in one pass delays the next S0_IR
	 * read and can leave SENDOK pending long enough for TCP to retransmit.
	 * The interrupt line remains asserted while more RX data is pending, so
	 * the caller's IRQ loop will re-enter and fetch the next frame.
	 */
	w6300_spi_read(dev, W6300_S0_RX_RSR, tmp, 2);
	rx_buf_len = sys_get_be16(tmp);

	if (rx_buf_len == 0) {
		return;
	}

	w6300_spi_read(dev, W6300_S0_RX_RD, tmp, 2);
	off = sys_get_be16(tmp);

	w6300_readbuf(dev, off, header, 2);
	rx_len = sys_get_be16(header) - 2;

	/* Sanity check on rx_len */
	if (rx_len > NET_ETH_MAX_FRAME_SIZE || rx_len == 0) {
		LOG_ERR_RATELIMIT_RATE(1000, "%s: invalid RX len=%u rsr=%u off=%u", dev->name,
				       rx_len, rx_buf_len, off);
		sys_put_be16(off + rx_buf_len, tmp);
		w6300_spi_write(dev, W6300_S0_RX_RD, tmp, 2);
		w6300_send_cmd(dev, S0_CR_RECV);
		return;
	}

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, rx_len, AF_UNSPEC, 0,
					   K_MSEC(config->timeout));
	if (!pkt) {
		eth_stats_update_errors_rx(ctx->iface);
		LOG_ERR_RATELIMIT_RATE(1000, "%s: RX alloc timeout len=%u rsr=%u off=%u", dev->name,
				       rx_len, rx_buf_len, off);

		/* Must still advance RX pointer to unblock the chip */
		sys_put_be16(off + 2 + rx_len, tmp);
		w6300_spi_write(dev, W6300_S0_RX_RD, tmp, 2);
		w6300_send_cmd(dev, S0_CR_RECV);
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

		w6300_readbuf(dev, reader, data_ptr, frame_len);
		net_buf_add(pkt_buf, frame_len);
		reader += (uint16_t)frame_len;

		read_len -= (uint16_t)frame_len;
		pkt_buf = pkt_buf->frags;
	} while (read_len > 0);

	if (net_recv_data(ctx->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	sys_put_be16(off + 2 + rx_len, tmp);
	w6300_spi_write(dev, W6300_S0_RX_RD, tmp, 2);
	w6300_send_cmd(dev, S0_CR_RECV);
}

static void w6300_update_link_status(const struct device *dev)
{
	uint8_t physr;
	struct w6300_runtime *ctx = dev->data;

	if (w6300_spi_read(dev, W6300_PHYSR, &physr, 1) < 0) {
		return;
	}

	if (physr & PHYSR_LNK) {
		if (ctx->link_up != true) {
			LOG_INF("%s: Link up", dev->name);
			ctx->link_up = true;
			net_eth_carrier_on(ctx->iface);
		}
	} else {
		if (ctx->link_up != false) {
			LOG_INF("%s: Link down", dev->name);
			ctx->link_up = false;
			net_eth_carrier_off(ctx->iface);
		}
	}
}

static void w6300_handle_interrupt_status(const struct device *dev, uint8_t ir)
{
	struct w6300_runtime *ctx = dev->data;

	w6300_spi_write(dev, W6300_S0_IRCLR, &ir, 1);

	LOG_DBG("IR received");

	if (ir & S0_IR_SENDOK) {
		k_sem_give(&ctx->tx_sem);
		LOG_DBG("TX Done");
	}

	if (ir & S0_IR_RECV) {
		w6300_rx(dev);
		LOG_DBG("RX Done");
	}
}

static void w6300_process_interrupts(const struct device *dev)
{
	uint8_t ir;
	const struct w6300_config *config = dev->config;

	while (gpio_pin_get_dt(&(config->interrupt))) {
		w6300_spi_read(dev, W6300_S0_IR, &ir, 1);

		if (!ir) {
			continue;
		}

		w6300_handle_interrupt_status(dev, ir);
	}
}

static void w6300_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	int res;
	struct w6300_runtime *ctx = dev->data;

	while (true) {
		res = k_sem_take(&ctx->int_sem, K_MSEC(CONFIG_ETH_W6300_MONITOR_PERIOD));

		if (res == -EAGAIN) {
			w6300_update_link_status(dev);
			continue;
		}

		if (res != 0) {
			continue;
		}

		if (!ctx->link_up) {
			w6300_update_link_status(dev);
		}

		w6300_process_interrupts(dev);
	}
}

static void w6300_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct w6300_runtime *ctx = dev->data;
	bool start_thread = false;

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);

	if (!ctx->iface) {
		ctx->iface = iface;
		start_thread = true;
	}

	ethernet_init(iface);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);
	ctx->link_up = false;

	if (start_thread) {
		/* Fetch initial link status only after iface is fully initialised. */
		w6300_update_link_status(dev);

		k_thread_create(&ctx->thread, ctx->thread_stack,
				CONFIG_ETH_W6300_RX_THREAD_STACK_SIZE, w6300_thread, (void *)dev,
				NULL, NULL, K_PRIO_PREEMPT(CONFIG_ETH_W6300_RX_THREAD_PRIO), 0,
				K_NO_WAIT);
		k_thread_name_set(&ctx->thread, "eth_w6300");
	}
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

static int w6300_set_config(const struct device *dev, enum ethernet_config_type type,
			    const struct ethernet_config *config)
{
	struct w6300_runtime *ctx = dev->data;
	uint8_t val;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(ctx->mac_addr, config->mac_address.addr, sizeof(ctx->mac_addr));
		/* Unlock network registers before writing MAC */
		val = NETLCKR_UNLOCK;
		w6300_spi_write(dev, W6300_NETLCKR, &val, 1);
		w6300_spi_write(dev, W6300_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name, ctx->mac_addr[0],
			ctx->mac_addr[1], ctx->mac_addr[2], ctx->mac_addr[3], ctx->mac_addr[4],
			ctx->mac_addr[5]);

		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			uint8_t mode;
			uint8_t mr = W6300_S0_MR_MF;

			w6300_spi_read(dev, W6300_S0_MR, &mode, 1);

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

			return w6300_spi_write(dev, W6300_S0_MR, &mode, 1);
		}

		return -ENOTSUP;
	default:
		return -ENOTSUP;
	}
}

static int w6300_hw_start(const struct device *dev)
{
	uint8_t mode = S0_MR_MACRAW | BIT(W6300_S0_MR_MF);
	uint8_t mask = IR_S0;

	/* Configure Socket 0 with MACRAW mode and MAC filtering enabled */
	w6300_spi_write(dev, W6300_S0_MR, &mode, 1);
	w6300_command(dev, S0_CR_OPEN);

	/* Enable socket 0 interrupt */
	w6300_spi_write(dev, W6300_SIMR, &mask, 1);

	return 0;
}

static int w6300_hw_stop(const struct device *dev)
{
	uint8_t mask = 0;

	/* Disable socket interrupt */
	w6300_spi_write(dev, W6300_SIMR, &mask, 1);
	w6300_command(dev, S0_CR_CLOSE);

	return 0;
}

static const struct ethernet_api w6300_api_funcs = {
	.iface_api.init = w6300_iface_init,
	.get_capabilities = w6300_get_capabilities,
	.set_config = w6300_set_config,
	.start = w6300_hw_start,
	.stop = w6300_hw_stop,
	.send = w6300_tx,
};

#define SYCR0_NORMAL BIT(7) /* 1 = normal operation */

static int w6300_soft_reset(const struct device *dev)
{
	int ret;
	uint8_t val;

	/* Unlock chip configuration registers */
	val = CHPLCKR_UNLOCK;
	ret = w6300_spi_write(dev, W6300_CHPLCKR, &val, 1);
	if (ret < 0) {
		return ret;
	}

	/* Assert software reset */
	val = SYCR0_RST; /* 0x00 */
	ret = w6300_spi_write(dev, W6300_SYCR0, &val, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(1);

	/* Release reset: normal operation */
	val = SYCR0_NORMAL; /* BIT(7) */
	ret = w6300_spi_write(dev, W6300_SYCR0, &val, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(5);

	/* Unlock again if reset relocks registers */
	val = CHPLCKR_UNLOCK;
	ret = w6300_spi_write(dev, W6300_CHPLCKR, &val, 1);
	if (ret < 0) {
		return ret;
	}

	/* Enable global interrupt */
	val = SYCR1_IEN;
	ret = w6300_spi_write(dev, W6300_SYCR1, &val, 1);
	if (ret < 0) {
		return ret;
	}

	/* Disable socket interrupts initially */
	val = 0;
	return w6300_spi_write(dev, W6300_SIMR, &val, 1);
}

static void w6300_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct w6300_runtime *ctx =
		(struct w6300_runtime *)((char *)cb - offsetof(struct w6300_runtime, gpio_cb));

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	k_sem_give(&ctx->int_sem);
}

static void w6300_set_macaddr(const struct device *dev)
{
	struct w6300_runtime *ctx = dev->data;
	uint8_t val;

	/* Unlock network registers before writing MAC */
	val = NETLCKR_UNLOCK;
	w6300_spi_write(dev, W6300_NETLCKR, &val, 1);

	w6300_spi_write(dev, W6300_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
}

static void w6300_memory_configure(const struct device *dev)
{
	uint8_t mem = 16;

	/* Configure Socket 0 RX & TX buffer to 16K */
	w6300_spi_write(dev, W6300_Sn_RX_BSR(0), &mem, 1);
	w6300_spi_write(dev, W6300_Sn_TX_BSR(0), &mem, 1);

	mem = 0;
	for (int i = 1; i < 8; i++) {
		w6300_spi_write(dev, W6300_Sn_RX_BSR(i), &mem, 1);
		w6300_spi_write(dev, W6300_Sn_TX_BSR(i), &mem, 1);
	}
}

static int w6300_init(const struct device *dev)
{
	int err;
	uint8_t rtr[2];
	const struct w6300_config *config = dev->config;
	struct w6300_runtime *ctx = dev->data;

	ctx->link_up = false;
	k_mutex_init(&ctx->tx_lock);
	k_mutex_init(&ctx->cmd_lock);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI master port %s not ready", config->spi.bus->name);
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&config->interrupt)) {
		LOG_ERR("GPIO port %s not ready", config->interrupt.port->name);
		return -EINVAL;
	}

	if (gpio_pin_configure_dt(&config->interrupt, GPIO_INPUT)) {
		LOG_ERR("Unable to configure GPIO pin %u", config->interrupt.pin);
		return -EINVAL;
	}
	gpio_init_callback(&(ctx->gpio_cb), w6300_gpio_callback, BIT(config->interrupt.pin));

	if (gpio_add_callback(config->interrupt.port, &(ctx->gpio_cb))) {
		return -EINVAL;
	}
	gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_EDGE_FALLING);

	if (config->reset.port) {
		if (!gpio_is_ready_dt(&config->reset)) {
			LOG_ERR("GPIO port %s not ready", config->reset.port->name);
			return -EINVAL;
		}
		if (gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT)) {
			LOG_ERR("Unable to configure GPIO pin %u", config->reset.pin);
			return -EINVAL;
		}

		gpio_pin_set_dt(&config->reset, 1);
		k_usleep(500);
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(70);
	}

	err = w6300_soft_reset(dev);
	if (err) {
		LOG_ERR("Reset failed");
		return err;
	}

	(void)net_eth_mac_load(&config->mac_cfg, ctx->mac_addr);
	w6300_set_macaddr(dev);
	w6300_memory_configure(dev);

	/* Check retry time value to verify chip communication */
	w6300_spi_read(dev, W6300_RTR, rtr, 2);
	if (sys_get_be16(rtr) != RTR_DEFAULT) {
		LOG_ERR("Unable to read RTR register");
		return -ENODEV;
	}

	LOG_INF("W6300 Initialized");

	return 0;
}

static struct w6300_runtime w6300_0_runtime = {
	.tx_sem = Z_SEM_INITIALIZER(w6300_0_runtime.tx_sem, 0, UINT_MAX),
	.int_sem = Z_SEM_INITIALIZER(w6300_0_runtime.int_sem, 0, UINT_MAX),
};

static const struct w6300_config w6300_0_config = {
	.spi = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8) | SPI_HALF_DUPLEX),
	.interrupt = GPIO_DT_SPEC_INST_GET(0, int_gpios),
	.reset = GPIO_DT_SPEC_INST_GET_OR(0, reset_gpios, {0}),
	.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0),
	.timeout = CONFIG_ETH_W6300_TIMEOUT,
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, w6300_init, NULL, &w6300_0_runtime, &w6300_0_config,
			      CONFIG_ETH_INIT_PRIORITY, &w6300_api_funcs, NET_ETH_MTU);
