/* WIZnet stand-alone Ethernet controllers, shared MACRAW core
 *
 * Copyright (c) 2020 Linumiz
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_wiznet, CONFIG_ETHERNET_LOG_LEVEL);

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#include <ethernet/eth_stats.h>

#include "eth_wiznet.h"

int wiznet_command(const struct device *dev, uint8_t cmd)
{
	const struct wiznet_config *cfg = dev->config;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(WIZNET_CMD_TIMEOUT_MS));
	uint8_t reg;

	wiznet_write(dev, cfg->regs->s0_cr, &cmd, 1);
	while (true) {
		wiznet_read(dev, cfg->regs->s0_cr, &reg, 1);
		if (!reg) {
			break;
		}
		if (sys_timepoint_expired(end)) {
			return -EIO;
		}
		k_busy_wait(WIZNET_CMD_POLL_DELAY_US);
	}

	return 0;
}

int wiznet_readbuf(const struct device *dev, uint16_t offset, uint8_t *buf, size_t len)
{
	const struct wiznet_config *cfg = dev->config;
	const uint32_t mem_start = cfg->regs->rx_mem_start;
	const uint32_t mem_size = cfg->regs->rx_mem_size;
	size_t remain = 0;
	uint32_t addr;
	int ret;

	offset %= mem_size;
	addr = mem_start + offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = wiznet_read(dev, addr, buf, len);
	if (ret || !remain) {
		return ret;
	}

	return wiznet_read(dev, mem_start, buf + len, remain);
}

int wiznet_writebuf(const struct device *dev, uint16_t offset, uint8_t *buf, size_t len)
{
	const struct wiznet_config *cfg = dev->config;
	const uint32_t mem_start = cfg->regs->tx_mem_start;
	const uint32_t mem_size = cfg->regs->tx_mem_size;
	size_t remain = 0;
	uint32_t addr;
	int ret;

	offset %= mem_size;
	addr = mem_start + offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = wiznet_write(dev, addr, buf, len);
	if (ret || !remain) {
		return ret;
	}

	return wiznet_write(dev, mem_start, buf + len, remain);
}

int wiznet_tx(const struct device *dev, struct net_pkt *pkt)
{
	const struct wiznet_config *cfg = dev->config;
	struct wiznet_runtime *ctx = dev->data;
	uint16_t len = (uint16_t)net_pkt_get_len(pkt);
	uint16_t offset;
	uint8_t off[2];
	int ret;

	wiznet_read(dev, cfg->regs->s0_tx_wr, off, 2);
	offset = sys_get_be16(off);

	if (net_pkt_read(pkt, ctx->buf, len)) {
		return -EIO;
	}

	ret = wiznet_writebuf(dev, offset, ctx->buf, len);
	if (ret < 0) {
		return ret;
	}

	sys_put_be16(offset + len, off);
	wiznet_write(dev, cfg->regs->s0_tx_wr, off, 2);

	wiznet_command(dev, WIZNET_S0_CR_SEND);
	if (k_sem_take(&ctx->tx_sem, K_MSEC(WIZNET_TX_TIMEOUT_MS))) {
		return -EIO;
	}

	return 0;
}

void wiznet_rx(const struct device *dev)
{
	const struct wiznet_config *cfg = dev->config;
	struct wiznet_runtime *ctx = dev->data;
	struct net_buf *pkt_buf = NULL;
	struct net_pkt *pkt;
	uint8_t header[2];
	uint8_t tmp[2];
	uint16_t off;
	uint16_t rx_len;
	uint16_t rx_buf_len;
	uint16_t read_len;
	uint16_t reader;

	wiznet_read(dev, cfg->regs->s0_rx_rsr, tmp, 2);
	rx_buf_len = sys_get_be16(tmp);

	if (rx_buf_len == 0) {
		return;
	}

	wiznet_read(dev, cfg->regs->s0_rx_rd, tmp, 2);
	off = sys_get_be16(tmp);

	if (wiznet_readbuf(dev, off, header, 2) < 0) {
		return;
	}
	if (sys_get_be16(header) <= 2U) {
		LOG_ERR("%s: invalid header size %u", dev->name, sys_get_be16(header));
		return;
	}
	rx_len = sys_get_be16(header) - 2;

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, rx_len, NET_AF_UNSPEC, 0,
					   K_MSEC(cfg->rx_timeout_ms));
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

		wiznet_readbuf(dev, reader, data_ptr, frame_len);
		net_buf_add(pkt_buf, frame_len);
		reader += (uint16_t)frame_len;

		read_len -= (uint16_t)frame_len;
		pkt_buf = pkt_buf->frags;
	} while (read_len > 0);

	if (net_recv_data(ctx->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	sys_put_be16(off + 2 + rx_len, tmp);
	wiznet_write(dev, cfg->regs->s0_rx_rd, tmp, 2);
	wiznet_command(dev, WIZNET_S0_CR_RECV);
}

static uint8_t wiznet_check_for_ir(const struct device *dev)
{
	const struct wiznet_config *cfg = dev->config;
	struct wiznet_runtime *ctx = dev->data;
	uint8_t ir;

	if (cfg->ops->clear_pending != NULL) {
		cfg->ops->clear_pending(dev);
	}

	wiznet_read(dev, cfg->regs->s0_ir, &ir, 1);

	if (ir != 0U) {
		wiznet_write(dev, cfg->regs->s0_irclr, &ir, 1);
		LOG_DBG("IR received");

		if ((ir & WIZNET_S0_IR_SENDOK) != 0U) {
			k_sem_give(&ctx->tx_sem);
			LOG_DBG("TX Done");
		}

		if ((ir & WIZNET_S0_IR_RECV) != 0U) {
			wiznet_rx(dev);
			LOG_DBG("RX Done");
		}
	}

	return ir;
}

static void wiznet_thread_poll(const struct device *dev)
{
	const struct wiznet_config *cfg = dev->config;
	struct wiznet_runtime *ctx = dev->data;

	if (!ctx->state.is_up) {
		k_msleep(cfg->poll_period_ms);
		cfg->ops->update_link_status(dev);
		return;
	}

	k_msleep(cfg->poll_period_ms);

	if (wiznet_check_for_ir(dev) == 0U) {
		cfg->ops->update_link_status(dev);
	}
}

static void wiznet_thread_interrupt(const struct device *dev)
{
	const struct wiznet_config *cfg = dev->config;
	struct wiznet_runtime *ctx = dev->data;
	int res;

	res = k_sem_take(&ctx->int_sem, K_MSEC(cfg->monitor_period_ms));

	if (res == 0) {
		if (!ctx->state.is_up) {
			cfg->ops->update_link_status(dev);
		}

		while (gpio_pin_get_dt(&cfg->interrupt)) {
			wiznet_check_for_ir(dev);
		}
	} else if (res == -EAGAIN) {
		cfg->ops->update_link_status(dev);
	}
}

static void wiznet_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct wiznet_runtime *ctx = CONTAINER_OF(cb, struct wiznet_runtime, gpio_cb);

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	k_sem_give(&ctx->int_sem);
}

static void wiznet_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;

	while (true) {
		const struct wiznet_config *cfg = dev->config;

		if (WIZNET_HAS_INTERRUPT(cfg)) {
			wiznet_thread_interrupt(dev);
			continue;
		}

		wiznet_thread_poll(dev);
	}
}

void wiznet_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct wiznet_config *cfg = dev->config;
	struct wiznet_runtime *ctx = dev->data;
	uint8_t mode = cfg->regs->s0_mr_macraw | BIT(cfg->regs->s0_mr_mf_bit);

	wiznet_write(dev, cfg->regs->s0_mr, &mode, 1);

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);

	ctx->iface = iface;

	ethernet_init(iface);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);

	cfg->ops->update_link_status(dev);

	k_thread_create(&ctx->thread, cfg->thread_stack, cfg->thread_stack_size, wiznet_thread,
			(void *)dev, NULL, NULL, K_PRIO_COOP(cfg->thread_prio), 0, K_NO_WAIT);
	k_thread_name_set(&ctx->thread, cfg->thread_name);
}

enum ethernet_hw_caps wiznet_get_capabilities(const struct device *dev __unused,
					      struct net_if *iface __unused)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	       | ETHERNET_PROMISC_MODE
#endif
		;
}

int wiznet_set_config(const struct device *dev, struct net_if *iface __unused,
		      enum ethernet_config_type type, const struct ethernet_config *config)
{
	const struct wiznet_config *cfg = dev->config;
	struct wiznet_runtime *ctx = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(ctx->mac_addr, config->mac_address.addr, sizeof(ctx->mac_addr));
		cfg->ops->set_macaddr(dev);
		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name, ctx->mac_addr[0],
			ctx->mac_addr[1], ctx->mac_addr[2], ctx->mac_addr[3], ctx->mac_addr[4],
			ctx->mac_addr[5]);

		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			uint8_t mr = cfg->regs->s0_mr_mf_bit;
			uint8_t mode;

			wiznet_read(dev, cfg->regs->s0_mr, &mode, 1);

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

			return wiznet_write(dev, cfg->regs->s0_mr, &mode, 1);
		}

		return -ENOTSUP;
	default:
		return -ENOTSUP;
	}
}

int wiznet_hw_start(const struct device *dev, struct net_if *iface __unused)
{
	const struct wiznet_config *cfg = dev->config;
	uint8_t mask = WIZNET_IR_S0;

	wiznet_command(dev, WIZNET_S0_CR_OPEN);
	wiznet_write(dev, cfg->regs->imr, &mask, 1);

	return 0;
}

int wiznet_hw_stop(const struct device *dev, struct net_if *iface __unused)
{
	const struct wiznet_config *cfg = dev->config;
	uint8_t mask = 0;

	wiznet_write(dev, cfg->regs->imr, &mask, 1);
	wiznet_command(dev, WIZNET_S0_CR_CLOSE);

	return 0;
}

const struct device *wiznet_get_phy(const struct device *dev, struct net_if *iface __unused)
{
	const struct wiznet_config *cfg = dev->config;

	return cfg->phy_dev;
}

int wiznet_get_link_state(const struct device *dev, struct phy_link_state *state)
{
	struct wiznet_runtime *const ctx = dev->data;

	state->speed = ctx->state.speed;
	state->is_up = ctx->state.is_up;

	return 0;
}

int wiznet_init(const struct device *dev)
{
	const struct wiznet_config *cfg = dev->config;
	struct wiznet_runtime *ctx = dev->data;
	int err;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI master port %s not ready", cfg->spi.bus->name);
		return -EINVAL;
	}

	if (WIZNET_HAS_INTERRUPT(cfg)) {
		if (!gpio_is_ready_dt(&cfg->interrupt)) {
			LOG_ERR("GPIO port %s not ready", cfg->interrupt.port->name);
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&cfg->interrupt, GPIO_INPUT);
		if (err < 0) {
			LOG_ERR("Unable to configure GPIO pin %u", cfg->interrupt.pin);
			return err;
		}

		gpio_init_callback(&ctx->gpio_cb, wiznet_gpio_callback, BIT(cfg->interrupt.pin));
		err = gpio_add_callback(cfg->interrupt.port, &ctx->gpio_cb);
		if (err < 0) {
			LOG_ERR("Unable to add GPIO callback %u", cfg->interrupt.pin);
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_FALLING);
		if (err < 0) {
			LOG_ERR("Unable to enable GPIO INT %u", cfg->interrupt.pin);
			return err;
		}
		LOG_INF("%s: interrupt mode", dev->name);
	} else {
		LOG_INF("%s: polling mode", dev->name);
	}

	if (cfg->reset.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset)) {
			LOG_ERR("GPIO port %s not ready", cfg->reset.port->name);
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&cfg->reset, GPIO_OUTPUT_INACTIVE);
		if (err < 0) {
			LOG_ERR("Unable to configure GPIO pin %u", cfg->reset.pin);
			return err;
		}

		gpio_pin_set_dt(&cfg->reset, 1);
		k_usleep(cfg->reset_pulse_us);
		gpio_pin_set_dt(&cfg->reset, 0);
		k_msleep(cfg->reset_delay_ms);
	}

	err = cfg->ops->soft_reset(dev);
	if (err != 0) {
		LOG_ERR("Reset failed");
		return err;
	}

	(void)net_eth_mac_load(&cfg->mac_cfg, ctx->mac_addr);
	cfg->ops->set_macaddr(dev);
	cfg->ops->memory_configure(dev);

	return 0;
}
