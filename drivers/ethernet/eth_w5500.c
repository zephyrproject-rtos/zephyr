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
#include <zephyr/net/net_mgmt.h>

#include "eth.h"
#include "eth_w5500.h"

#define WIZNET_OUI_B0 0x00
#define WIZNET_OUI_B1 0x08
#define WIZNET_OUI_B2 0xdc

#define W5500_SPI_BLOCK_SELECT(addr)  (((addr) >> 16) & 0x1f)
#define W5500_SPI_READ_CONTROL(addr)  (W5500_SPI_BLOCK_SELECT(addr) << 3)
#define W5500_SPI_WRITE_CONTROL(addr) ((W5500_SPI_BLOCK_SELECT(addr) << 3) | BIT(2))

static struct net_mgmt_event_callback mgmt_cb;

int w5500_spi_read(const struct device *dev, uint32_t addr, uint8_t *data, size_t len)
{
	const struct w5500_config *cfg = dev->config;
	int ret;

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
		{.buf = data, .len = len},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);

	return ret;
}

int w5500_spi_write(const struct device *dev, uint32_t addr, const uint8_t *data, size_t len)
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
			.buf = (uint8_t *)data,
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

int w5500_socket_readbuf(const struct device *dev, uint8_t sn, uint16_t offset, uint8_t *buf,
			 size_t len)
{
	uint32_t addr;
	size_t remain = 0;
	int ret;
	const uint32_t mem_start = W5500_Sn_RXBUFS(sn);
	const uint32_t mem_size = W5500_RX_MEM_SIZE;

	offset %= mem_size;
	addr = mem_start + offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = w5500_spi_read(dev, addr, buf, len);
	if (ret || !remain) {
		return ret;
	}

	return w5500_spi_read(dev, mem_start, buf + len, remain);
}

int w5500_socket_writebuf(const struct device *dev, uint8_t sn, uint16_t offset, const uint8_t *buf,
			  size_t len)
{
	uint32_t addr;
	size_t remain = 0;
	int ret = 0;
	const uint32_t mem_start = W5500_Sn_TXBUFS(sn);
	const uint32_t mem_size = W5500_TX_MEM_SIZE;

	offset %= mem_size;
	addr = mem_start + offset;

	if (offset + len > mem_size) {
		remain = (offset + len) % mem_size;
		len = mem_size - offset;
	}

	ret = w5500_spi_write(dev, addr, buf, len);
	if (ret || !remain) {
		return ret;
	}

	ret = w5500_spi_write(dev, mem_start, buf + len, remain);

	w5500_socket_command(dev, sn, W5500_Sn_CR_RECV);

	return ret;
}

int w5500_socket_command(const struct device *dev, uint8_t sn, uint8_t cmd)
{
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(100));

	w5500_spi_write_byte(dev, W5500_Sn_CR(sn), cmd);
	while (true) {
		if (!w5500_spi_read_byte(dev, W5500_Sn_CR(sn))) {
			break;
		}
		if (sys_timepoint_expired(end)) {
			return -EIO;
		}
		k_busy_wait(W5500_PHY_ACCESS_DELAY);
	}
	return 0;
}

int w5500_socket_tx(const struct device *dev, uint8_t sn, const uint8_t *buf, size_t len)
{
	uint16_t offset;
	int ret;

	offset = w5500_spi_read_two_bytes(dev, W5500_Sn_TX_WR(sn));

	ret = w5500_socket_writebuf(dev, sn, offset, buf, len);
	if (ret < 0) {
		return ret;
	}

	w5500_spi_write_two_bytes(dev, W5500_Sn_TX_WR(sn), offset + len);

	return w5500_socket_command(dev, sn, W5500_Sn_CR_SEND);
}

uint16_t w5500_socket_rx(const struct device *dev, uint8_t sn, uint8_t *buf, size_t len)
{
	uint16_t offset;

	offset = w5500_spi_read_two_bytes(dev, W5500_Sn_RX_RD(sn));
	w5500_socket_readbuf(dev, sn, offset, buf, len);

	w5500_spi_write_two_bytes(dev, W5500_Sn_RX_RD(sn), offset + len);
	w5500_socket_command(dev, sn, W5500_Sn_CR_RECV);

	return offset + len;
}

static int w5500_l2_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct w5500_runtime *ctx = dev->data;
	uint16_t len = (uint16_t)net_pkt_get_len(pkt);
	int ret;

	if (net_pkt_read(pkt, ctx->buf, len)) {
		return -EIO;
	}

	w5500_socket_tx(dev, 0, ctx->buf, len);

	k_timepoint_t end = sys_timepoint_calc(K_MSEC(100));

	while (true) {
		ret = w5500_spi_read_byte(dev, W5500_Sn_IR(0));
		if (ret & W5500_Sn_IR_SENDOK) {
			break;
		} else if (ret & W5500_Sn_IR_TIMEOUT) {
			return -ETIMEDOUT;
		} else if (sys_timepoint_expired(end)) {
			return -EIO;
		}
		k_busy_wait(W5500_PHY_ACCESS_DELAY);
	}

	return 0;
}

static void w5500_l2_rx(const struct device *dev)
{
	uint8_t header[2];
	uint16_t off;
	uint16_t rx_len;
	uint16_t rx_buf_len;
	uint16_t read_len;
	uint16_t reader;
	struct net_buf *pkt_buf = NULL;
	struct net_pkt *pkt;
	struct w5500_runtime *ctx = dev->data;
	const struct w5500_config *config = dev->config;

	rx_buf_len = w5500_spi_read_two_bytes(dev, W5500_Sn_RX_RSR(0));

	if (rx_buf_len == 0) {
		return;
	}

	off = w5500_spi_read_two_bytes(dev, W5500_Sn_RX_RD(0));

	w5500_socket_readbuf(dev, 0, off, header, 2);
	rx_len = sys_get_be16(header) - 2;

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, rx_len, AF_UNSPEC, 0,
					   K_MSEC(config->timeout));
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

		w5500_socket_readbuf(dev, 0, reader, data_ptr, frame_len);
		net_buf_add(pkt_buf, frame_len);
		reader += (uint16_t)frame_len;

		read_len -= (uint16_t)frame_len;
		pkt_buf = pkt_buf->frags;
	} while (read_len > 0);

	if (net_recv_data(ctx->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	w5500_spi_write_two_bytes(dev, W5500_Sn_RX_RD(0), off + 2 + rx_len);
	w5500_socket_command(dev, 0, W5500_Sn_CR_RECV);
}

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
static void w5500_set_ipaddr(const struct device *dev, uint32_t *ipaddr)
{
	w5500_spi_write(dev, W5500_SIPR, (uint8_t *)ipaddr, 4);
}

static void w5500_set_subnet_mask(const struct device *dev, uint32_t *mask)
{
	w5500_spi_write(dev, W5500_SUBR, (uint8_t *)mask, 4);
}

static void w5500_set_gateway(const struct device *dev, uint32_t *gw)
{
	w5500_spi_write(dev, W5500_GAR, (uint8_t *)gw, 4);
}

static void w5500_ipv4_addr_callback(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				     struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct w5500_runtime *ctx = dev->data;

	ctx->net_config_changed = true;
}

void w5500_hw_net_config(const struct device *dev)
{
	struct w5500_runtime *ctx = dev->data;

	struct net_if_ipv4 *ipv4 = ctx->iface->config.ip.ipv4;
	struct in_addr *addr, *mask, *gw;
	int16_t i = NET_IF_MAX_IPV4_ADDR - 1;
	char buf[NET_IPV4_ADDR_LEN];

	for (; i >= 0; i--) {
		if (!ipv4->unicast[i].ipv4.is_used) {
			continue;
		}

		addr = &(ipv4->unicast[i].ipv4.address.in_addr);
		mask = &(ipv4->unicast[i].netmask);
		gw = &(ipv4->gw);

		if (addr->s_addr != ctx->local_ip_addr.s_addr) {
			LOG_INF("%s: Set W5500 IPv4 address to %s", dev->name,
				net_addr_ntop(AF_INET, addr, buf, sizeof(buf)));

			LOG_INF("%s: Set W5500 netmask to %s", dev->name,
				net_addr_ntop(AF_INET, mask, buf, sizeof(buf)));

			LOG_INF("%s: Set W5500 gateway to %s", dev->name,
				net_addr_ntop(AF_INET, gw, buf, sizeof(buf)));

			w5500_set_ipaddr(dev, &addr->s_addr);
			w5500_set_subnet_mask(dev, &mask->s_addr);
			w5500_set_gateway(dev, &gw->s_addr);

			ctx->local_ip_addr.s_addr = addr->s_addr;
		}

		return;
	}

	LOG_INF("%s: Set W5500 IPv4 address to 0.0.0.0", dev->name);
	ctx->local_ip_addr.s_addr = 0;
	w5500_set_ipaddr(dev, &ctx->local_ip_addr.s_addr);
}
#endif

static void w5500_update_link_status(const struct device *dev)
{
	struct w5500_runtime *ctx = dev->data;
	uint8_t phycfgr = w5500_spi_read_byte(dev, W5500_PHYCFGR);

	if (phycfgr < 0) {
		return;
	}

	if (phycfgr & 0x01) {
		if (ctx->link_up != true) {
			LOG_INF("%s: Link up", dev->name);
			ctx->link_up = true;
#ifdef CONFIG_NET_SOCKETS_OFFLOAD
			ctx->net_config_changed = true;

			net_mgmt_init_event_callback(&mgmt_cb, w5500_ipv4_addr_callback,
						     NET_EVENT_IPV4_ADDR_ADD |
							     NET_EVENT_IPV4_ADDR_DEL);
			net_mgmt_add_event_callback(&mgmt_cb);
#endif

			net_eth_carrier_on(ctx->iface);
		}
	} else {
		if (ctx->link_up != false) {
			LOG_INF("%s: Link down", dev->name);
			ctx->link_up = false;

			net_mgmt_del_event_callback(&mgmt_cb);

			net_eth_carrier_off(ctx->iface);
		}
	}
}

static void w5500_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	uint8_t ir;
	int res;
	struct w5500_runtime *ctx = dev->data;
	const struct w5500_config *config = dev->config;
#ifdef CONFIG_NET_SOCKETS_OFFLOAD
	uint8_t sir;
#endif

	while (true) {
#ifdef CONFIG_NET_SOCKETS_OFFLOAD
		if (ctx->net_config_changed) {
			ctx->net_config_changed = false;

			w5500_hw_net_config(dev);
		}
#endif

		res = k_sem_take(&ctx->int_sem, K_MSEC(CONFIG_PHY_MONITOR_PERIOD));

		if (res == 0) {
			/* semaphore taken, update link status and receive packets */
			if (ctx->link_up != true) {
				w5500_update_link_status(dev);
			}

			while (gpio_pin_get_dt(&(config->interrupt))) {
#ifdef CONFIG_NET_SOCKETS_OFFLOAD
				sir = w5500_spi_read_byte(dev, W5500_SIR);

				if (sir & BIT(0)) {
#else
				{
#endif
					ir = w5500_socket_interrupt_status(dev, 0);

					if (ir) {
						w5500_socket_interrupt_clear(dev, 0, ir);

						if (ir & W5500_Sn_IR_RECV) {
							w5500_l2_rx(dev);
						}
					}
				}

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
				uint8_t socknum;

				for (socknum = 1; socknum < W5500_MAX_SOCK_NUM; socknum++) {
					if (!(sir & BIT(socknum))) {
						continue;
					}

					ir = w5500_socket_interrupt_status(dev, socknum);
					if (ir) {
						w5500_socket_interrupt_clear(dev, socknum, ir);

						ctx->sockets[socknum].ir |= ir;
						k_sem_give(&ctx->sockets[socknum].sint_sem);

						if (ir & W5500_Sn_IR_DISCON) {
							if (ctx->sockets[socknum].listen_ctx_ind !=
							    W5500_MAX_SOCK_NUM) {
								__w5500_handle_incoming_conn_closed(
									socknum);
							} else {
								ctx->sockets[socknum].state =
									W5500_SOCKET_STATE_ASSIGNED;
							}
						} else if (ir & W5500_Sn_IR_CON) {
							if (ctx->sockets[socknum].state ==
							    W5500_SOCKET_STATE_LISTENING) {
								__w5500_handle_incoming_conn_established(
									socknum);
							}
						}

						k_sem_give(&ctx->sockets[socknum].sint_sem);
					}
				}
#endif
			}
		} else if (res == -EAGAIN) {
			/* semaphore timeout period expired, check link status */
			w5500_update_link_status(dev);
		}
	}
}

static void w5500_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct w5500_runtime *ctx = dev->data;

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);

	if (!ctx->iface) {
		ctx->iface = iface;
	}

	ethernet_init(iface);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
	w5500_socket_offload_init(dev);
#endif
}

static enum ethernet_hw_caps w5500_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	       | ETHERNET_PROMISC_MODE
#endif
		;
}

static int w5500_set_config(const struct device *dev, enum ethernet_config_type type,
			    const struct ethernet_config *config)
{
	struct w5500_runtime *ctx = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(ctx->mac_addr, config->mac_address.addr, sizeof(ctx->mac_addr));
		w5500_spi_write(dev, W5500_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name, ctx->mac_addr[0],
			ctx->mac_addr[1], ctx->mac_addr[2], ctx->mac_addr[3], ctx->mac_addr[4],
			ctx->mac_addr[5]);

		/* Register Ethernet MAC Address with the upper layer */
		net_if_set_link_addr(ctx->iface, ctx->mac_addr, sizeof(ctx->mac_addr),
				     NET_LINK_ETHERNET);

		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			uint8_t mode;

			mode = w5500_spi_read_byte(dev, W5500_Sn_MR(0));

			if (config->promisc_mode) {
				if (!(mode & W5500_Sn_MR_MACRAW)) {
					return -EALREADY;
				}

				/* disable MAC filtering */
				mode &= ~W5500_Sn_MR_MACRAW;
			} else {
				if (mode & W5500_Sn_MR_MACRAW) {
					return -EALREADY;
				}

				/* enable MAC filtering */
				mode |= W5500_Sn_MR_MACRAW;
			}

			return w5500_spi_write_byte(dev, W5500_Sn_MR(0), mode);
		}

		return -ENOTSUP;
	default:
		return -ENOTSUP;
	}
}

static int w5500_hw_start(const struct device *dev)
{
	struct w5500_runtime *ctx = dev->data;

	/* configure Socket 0 with MACRAW mode and MAC filtering enabled */
	w5500_spi_write_byte(dev, W5500_Sn_MR(0), W5500_Sn_MR_MACRAW | W5500_Sn_MR_MFEN);
	w5500_socket_command(dev, 0, W5500_Sn_CR_OPEN);

	/* enable interrupt for Socket 0 */
	w5500_spi_write_byte(dev, W5500_SIMR, BIT(0));
	/* mask all but data recv interrupt for Socket 0 */
	w5500_spi_write_byte(dev, W5500_Sn_IMR(0), W5500_Sn_IR_RECV);

	ctx->sockets[0].type = W5500_TRANSPORT_MACRAW;
	ctx->sockets[0].state = W5500_SOCKET_STATE_OPEN;

	return 0;
}

static int w5500_hw_stop(const struct device *dev)
{
	/* disable interrupt */
	w5500_spi_write_byte(dev, W5500_SIMR, 0);
	w5500_socket_command(dev, 0, W5500_Sn_CR_CLOSE);

	return 0;
}

static const struct ethernet_api w5500_api_funcs = {
	.iface_api.init = w5500_iface_init,
	.get_capabilities = w5500_get_capabilities,
	.set_config = w5500_set_config,
	.start = w5500_hw_start,
	.stop = w5500_hw_stop,
	.send = w5500_l2_tx,
};

static int w5500_soft_reset(const struct device *dev)
{
	int ret;

	ret = w5500_spi_write_byte(dev, W5500_MR, W5500_MR_RST);
	if (ret < 0) {
		return ret;
	}

	k_msleep(5);

	w5500_spi_write_byte(dev, W5500_MR, W5500_MR_PB);

	/* disable interrupt */
	return w5500_spi_write_byte(dev, W5500_SIMR, 0);
}

static void w5500_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct w5500_runtime *ctx = CONTAINER_OF(cb, struct w5500_runtime, gpio_cb);

	k_sem_give(&ctx->int_sem);
}

static void w5500_set_macaddr(const struct device *dev)
{
	struct w5500_runtime *ctx = dev->data;

#if DT_INST_PROP(0, zephyr_random_mac_address)
	gen_random_mac(ctx->mac_addr, WIZNET_OUI_B0, WIZNET_OUI_B1, WIZNET_OUI_B2);
#endif

	w5500_spi_write(dev, W5500_SHAR, ctx->mac_addr, sizeof(ctx->mac_addr));
}

static void w5500_memory_configure(const struct device *dev, uint8_t *mem_sz)
{
	int i;

	for (i = 0; i < 8; i++) {
		w5500_spi_write_byte(dev, W5500_Sn_RXMEM_SIZE(i), mem_sz[i]);
		w5500_spi_write_byte(dev, W5500_Sn_TXMEM_SIZE(i), mem_sz[i]);
	}
}

static int w5500_init(const struct device *dev)
{
	int err;
	const struct w5500_config *config = dev->config;
	struct w5500_runtime *ctx = dev->data;

	ctx->link_up = false;

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

	gpio_init_callback(&(ctx->gpio_cb), w5500_gpio_callback, BIT(config->interrupt.pin));

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
		gpio_pin_set_dt(&config->reset, 0);
		k_usleep(500);
	}

	err = w5500_soft_reset(dev);
	if (err) {
		LOG_ERR("Reset failed");
		return err;
	}

	w5500_set_macaddr(dev);

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
	uint8_t i;
	/* Configure RX & TX memory to 2K for all 8 sockets */
	uint8_t mem_sz[8] = {2, 2, 2, 2, 2, 2, 2, 2};

	for (i = 0; i < W5500_MAX_SOCK_NUM; i++) {
		ctx->sockets[i].tx_buf_size = mem_sz[i];
		ctx->sockets[i].rx_buf_size = mem_sz[i];
	}
#else
	/* Configure RX & TX memory to 16K for Socket 0 */
	uint8_t mem_sz[8] = {16, 0, 0, 0, 0, 0, 0, 0};
	ctx->sockets[0].tx_buf_size = mem_sz[0];
	ctx->sockets[0].rx_buf_size = mem_sz[0];
#endif

	w5500_memory_configure(dev, mem_sz);

	/* check retry time value */
	if (w5500_spi_read_two_bytes(dev, W5500_RTR) != W5500_RTR_DEFAULT) {
		LOG_ERR("Unable to read RTR register");
		return -ENODEV;
	}

	k_thread_create(&ctx->thread, ctx->thread_stack, CONFIG_ETH_W5500_RX_THREAD_STACK_SIZE,
			w5500_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_W5500_RX_THREAD_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&ctx->thread, "eth_w5500");

	LOG_INF("W5500 Initialized");

	return 0;
}

static struct w5500_runtime w5500_0_runtime = {
#if NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0))
	.mac_addr = DT_INST_PROP(0, local_mac_address),
#endif
	.int_sem = Z_SEM_INITIALIZER(w5500_0_runtime.int_sem, 0, UINT_MAX),
};

static const struct w5500_config w5500_0_config = {
	.spi = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8), 0),
	.interrupt = GPIO_DT_SPEC_INST_GET(0, int_gpios),
	.reset = GPIO_DT_SPEC_INST_GET_OR(0, reset_gpios, {0}),
	.timeout = CONFIG_ETH_W5500_TIMEOUT,
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, w5500_init, NULL, &w5500_0_runtime, &w5500_0_config,
			      CONFIG_ETH_INIT_PRIORITY, &w5500_api_funcs, NET_ETH_MTU);
