/* KSZ8851SNL Stand-alone Ethernet Controller with SPI
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_ksz8851snl

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_ksz8851snl, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/sys/bit_rev.h>
#include <zephyr/sys/crc.h>
#include <ethernet/eth_stats.h>

#include "eth_ksz8851snl_priv.h"

/*
 * The first 4 bytes read back after the FIFO read opcode are always
 * dummy/undefined data and must be discarded, followed by 4 more bytes
 * which duplicate the frame's status word and byte count (already read
 * through the RXFHSR/RXFHBCR registers), see datasheet section 3.5.6.
 */
#define KSZ8851_FIFO_RX_HEADER_LEN	8
/* Number of trailing CRC bytes included in the RX byte count */
#define KSZ8851_RX_CRC_LEN		4
/* Frame header (control word + byte count) written ahead of TX payload */
#define KSZ8851_TX_HEADER_LEN		4

#define KSZ8851_RESET_DELAY_MS		10
#define KSZ8851_STARTUP_TIMEOUT_MS	1000
#define KSZ8851_STARTUP_POLL_MS		10

#define KSZ8851_TXCR			(TXCR_TXFCE | TXCR_TXPE | TXCR_TXCE)
/* Leave checksum validation to the stack. Hardware RX checksum filtering
 * discards valid IPv6 UDP traffic on this controller.
 */
#define KSZ8851_RXCR1			(RXCR1_RXPAFMA | RXCR1_RXFCE | RXCR1_RXBE | \
					 RXCR1_RXME | RXCR1_RXUE)
#define KSZ8851_RXQCR			(RXQCR_RXFCTE | RXQCR_ADRFE)

/* Maximum number of net_buf fragments transferred to/from the FIFO at once */
#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
#define KSZ8851_MAX_FRAGS		(DIV_ROUND_UP(NET_ETH_MAX_FRAME_SIZE, \
						      CONFIG_NET_BUF_DATA_SIZE) + 1)
#else
#define KSZ8851_MAX_FRAGS		4
#endif

/* Opcode/header, payload fragments, trailing CRC/padding */
#define KSZ8851_MAX_SPI_BUFS		(KSZ8851_MAX_FRAGS + 2)

static uint16_t ksz8851snl_cmd(uint8_t op, uint8_t be, uint8_t addr)
{
	return ((uint16_t)op << 8) | ((uint16_t)be << 10) | ((uint16_t)addr << 2);
}

static uint8_t ksz8851snl_be16(uint8_t addr)
{
	return (addr & 0x02U) ? KSZ8851_BE_WORD_HIGH : KSZ8851_BE_WORD_LOW;
}

/* At runtime, TX, configuration and interrupt handlers own ctx->lock.
 * Register and FIFO helpers require that lock.
 */
static int ksz8851snl_read(const struct device *dev, uint8_t addr, uint8_t be,
			   uint8_t *data, size_t len)
{
	const struct ksz8851snl_config *cfg = dev->config;
	uint8_t cmd[2];
	const struct spi_buf tx_buf = {
		.buf = cmd,
		.len = sizeof(cmd),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_bufs[2] = {
		{
			.buf = NULL,
			.len = sizeof(cmd),
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

	sys_put_be16(ksz8851snl_cmd(KSZ8851_CMD_READ, be, addr), cmd);

	return spi_transceive_dt(&cfg->spi, &tx, &rx);
}

static int ksz8851snl_reg_read(const struct device *dev, uint8_t addr, uint16_t *val)
{
	uint8_t data[2];
	int ret;

	ret = ksz8851snl_read(dev, addr, ksz8851snl_be16(addr), data, sizeof(data));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_le16(data);

	return 0;
}

static int ksz8851snl_reg_write(const struct device *dev, uint8_t addr, uint16_t val)
{
	const struct ksz8851snl_config *cfg = dev->config;
	uint8_t data[4];
	const struct spi_buf tx_buf = {
		.buf = data,
		.len = sizeof(data),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	sys_put_be16(ksz8851snl_cmd(KSZ8851_CMD_WRITE, ksz8851snl_be16(addr), addr), data);
	sys_put_le16(val, &data[2]);

	return spi_write_dt(&cfg->spi, &tx);
}

/* Run a TX/RX queue transfer with the Start DMA Access bit set. Clearing it
 * again also enqueues a written TX frame (TXQCR_AETFE).
 */
static int ksz8851snl_fifo_xfer(const struct device *dev, const struct spi_buf_set *tx,
				const struct spi_buf_set *rx)
{
	const struct ksz8851snl_config *cfg = dev->config;
	int ret;
	int end_ret;

	ret = ksz8851snl_reg_write(dev, KSZ8851_REG_RXQCR, KSZ8851_RXQCR | RXQCR_SDA);
	if (ret < 0) {
		return ret;
	}

	ret = spi_transceive_dt(&cfg->spi, tx, rx);

	end_ret = ksz8851snl_reg_write(dev, KSZ8851_REG_RXQCR, KSZ8851_RXQCR);
	if (end_ret < 0) {
		LOG_ERR("%s: ending FIFO access failed (%d)", dev->name, end_ret);
	}

	return ret < 0 ? ret : end_ret;
}

static int ksz8851snl_set_macaddr(const struct device *dev, const uint8_t *mac_addr)
{
	int ret;

	/* MARH holds the first two address bytes, MARL the last two */
	for (uint8_t i = 0U; i < 3U; i++) {
		ret = ksz8851snl_reg_write(dev, KSZ8851_REG_MARH - 2U * i,
					   sys_get_be16(&mac_addr[2U * i]));
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static size_t ksz8851snl_frag_count(struct net_pkt *pkt)
{
	size_t count = 0;

	for (struct net_buf *frag = pkt->buffer; frag != NULL; frag = frag->frags) {
		if (frag->len > 0U) {
			count++;
		}
	}

	return count;
}

static int ksz8851snl_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct ksz8851snl_runtime *ctx = dev->data;
	uint8_t hdr[1 + KSZ8851_TX_HEADER_LEN] = { KSZ8851_CMD_FIFO_WRITE };
	struct spi_buf bufs[KSZ8851_MAX_SPI_BUFS];
	struct spi_buf_set tx = {
		.buffers = bufs,
		.count = 0,
	};
	size_t len = net_pkt_get_len(pkt);
	uint16_t needed = ROUND_UP(len, 4) + KSZ8851_TX_HEADER_LEN;
	int ret;

	if (len > NET_ETH_MAX_FRAME_SIZE) {
		LOG_ERR("%s: frame too large (%zu)", dev->name, len);
		return -EMSGSIZE;
	}

	if (ksz8851snl_frag_count(pkt) > KSZ8851_MAX_FRAGS) {
		net_pkt_compact(pkt);
		if (ksz8851snl_frag_count(pkt) > KSZ8851_MAX_FRAGS) {
			LOG_ERR("%s: too many TX fragments", dev->name);
			return -ENOMEM;
		}
	}

	/* Control word 0: no TX-done interrupt requested */
	sys_put_le16((uint16_t)len, &hdr[3]);
	bufs[tx.count++] = (struct spi_buf){ .buf = hdr, .len = sizeof(hdr) };

	for (struct net_buf *frag = pkt->buffer; frag != NULL; frag = frag->frags) {
		if (frag->len > 0U) {
			bufs[tx.count++] = (struct spi_buf){ .buf = frag->data, .len = frag->len };
		}
	}

	/* The FIFO is written in multiples of 4 bytes, pad with dummy bytes */
	if (ROUND_UP(len, 4) > len) {
		bufs[tx.count++] = (struct spi_buf){ .len = ROUND_UP(len, 4) - len };
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);

	if (ctx->tx_space < needed) {
		uint16_t txmir;

		ret = ksz8851snl_reg_read(dev, KSZ8851_REG_TXMIR, &txmir);
		if (ret < 0) {
			goto unlock;
		}

		ctx->tx_space = txmir & TXMIR_TXMA_MASK;
		if (ctx->tx_space < needed) {
			LOG_WRN("%s: TX queue full, dropping frame", dev->name);
			ret = -EIO;
			goto unlock;
		}
	}

	ret = ksz8851snl_fifo_xfer(dev, &tx, NULL);
	if (ret < 0) {
		LOG_ERR("%s: TX FIFO write failed (%d)", dev->name, ret);
		goto unlock;
	}

	ctx->tx_space -= needed;

unlock:
	k_mutex_unlock(&ctx->lock);

	return ret;
}

/* Discard the frame at the head of the RX queue without reading it */
static int ksz8851snl_rx_release(const struct device *dev)
{
	return ksz8851snl_reg_write(dev, KSZ8851_REG_RXQCR, KSZ8851_RXQCR | RXQCR_RRXEF);
}

static int ksz8851snl_rx_frame(const struct device *dev, uint16_t status, uint16_t byte_count)
{
	struct ksz8851snl_runtime *ctx = dev->data;
	uint8_t cmd = KSZ8851_CMD_FIFO_READ;
	const struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = sizeof(cmd),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	struct spi_buf bufs[KSZ8851_MAX_SPI_BUFS];
	struct spi_buf_set rx = {
		.buffers = bufs,
		.count = 0,
	};
	struct net_pkt *pkt;
	size_t len;
	size_t remaining;
	int ret;

	if (!(status & RXFHSR_RXFV) || (status & RXFHSR_RX_ERRORS) ||
	    byte_count <= KSZ8851_RX_CRC_LEN ||
	    byte_count > NET_ETH_MAX_FRAME_SIZE + KSZ8851_RX_CRC_LEN) {
		LOG_WRN("%s: dropping bad RX frame (status 0x%04x, length %u)",
			dev->name, status, byte_count);
		eth_stats_update_errors_rx(ctx->iface);
		return ksz8851snl_rx_release(dev);
	}

	len = byte_count - KSZ8851_RX_CRC_LEN;

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, len, NET_AF_UNSPEC, 0,
					   K_MSEC(CONFIG_ETH_KSZ8851SNL_TIMEOUT));
	if (pkt == NULL) {
		LOG_WRN("%s: no packet buffer for RX frame (%zu bytes)", dev->name, len);
		eth_stats_update_errors_rx(ctx->iface);
		return ksz8851snl_rx_release(dev);
	}

	bufs[rx.count] = (struct spi_buf){
		.buf = NULL,
		.len = sizeof(cmd) + KSZ8851_FIFO_RX_HEADER_LEN,
	};
	rx.count++;

	remaining = len;
	for (struct net_buf *frag = pkt->buffer; frag != NULL && remaining > 0U;
	     frag = frag->frags) {
		size_t chunk = MIN(net_buf_tailroom(frag), remaining);

		if (chunk == 0U) {
			continue;
		}

		if (rx.count == ARRAY_SIZE(bufs) - 1U) {
			LOG_ERR("%s: too many RX fragments", dev->name);
			net_pkt_unref(pkt);
			eth_stats_update_errors_rx(ctx->iface);
			return ksz8851snl_rx_release(dev);
		}

		bufs[rx.count] = (struct spi_buf){
			.buf = net_buf_add(frag, chunk),
			.len = chunk,
		};
		rx.count++;
		remaining -= chunk;
	}

	/* The whole frame (incl. CRC) must be drained, in multiples of 4 bytes */
	bufs[rx.count] = (struct spi_buf){
		.buf = NULL,
		.len = ROUND_UP(byte_count, 4) - len,
	};
	rx.count++;

	ret = ksz8851snl_reg_write(dev, KSZ8851_REG_RXFDPR, RXFDPR_RXFPAI);
	if (ret == 0) {
		ret = ksz8851snl_fifo_xfer(dev, &tx, &rx);
	}

	if (ret < 0) {
		LOG_ERR("%s: RX FIFO read failed (%d)", dev->name, ret);
		net_pkt_unref(pkt);
		eth_stats_update_errors_rx(ctx->iface);
		return ret;
	}

	if (net_recv_data(ctx->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	return 0;
}

static int ksz8851snl_rx(const struct device *dev)
{
	uint16_t rxfctr;
	uint8_t frame_count;
	int ret;

	ret = ksz8851snl_reg_read(dev, KSZ8851_REG_RXFCTR, &rxfctr);
	if (ret < 0) {
		return ret;
	}

	frame_count = (uint8_t)(rxfctr >> 8);

	while (frame_count-- > 0U) {
		uint8_t hdr[4];

		/* RXFHSR and RXFHBCR in one 32 bit access */
		ret = ksz8851snl_read(dev, KSZ8851_REG_RXFHSR, KSZ8851_BE_DWORD, hdr, sizeof(hdr));
		if (ret < 0) {
			break;
		}

		ret = ksz8851snl_rx_frame(dev, sys_get_le16(&hdr[0]),
					  sys_get_le16(&hdr[2]) & RXFHBCR_RXBC_MASK);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

static void ksz8851snl_update_link_status(const struct device *dev)
{
	struct ksz8851snl_runtime *ctx = dev->data;
	uint16_t p1sr;
	enum phy_link_speed speed;

	if (ksz8851snl_reg_read(dev, KSZ8851_REG_P1SR, &p1sr) < 0) {
		return;
	}

	if (p1sr & P1SR_LINK_GOOD) {
		if (!ctx->state.is_up) {
			ctx->state.is_up = true;
			net_eth_carrier_on(ctx->iface);
		}

		speed = (p1sr & P1SR_OPERATION_SPEED) ?
			((p1sr & P1SR_OPERATION_DUPLEX) ? LINK_FULL_100BASE
							 : LINK_HALF_100BASE) :
			((p1sr & P1SR_OPERATION_DUPLEX) ? LINK_FULL_10BASE
							 : LINK_HALF_10BASE);

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

static int ksz8851snl_handle_interrupt(const struct device *dev)
{
	struct ksz8851snl_runtime *ctx = dev->data;
	uint16_t isr;
	int ret;

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ret = ksz8851snl_reg_read(dev, KSZ8851_REG_ISR, &isr);
	if (ret < 0) {
		goto unlock;
	}

	if (isr == 0U) {
		goto unlock;
	}

	ret = ksz8851snl_reg_write(dev, KSZ8851_REG_ISR, isr);
	if (ret < 0) {
		goto unlock;
	}

	if (isr & IER_LCIE) {
		ksz8851snl_update_link_status(dev);
	}

	if (isr & IER_RXOIE) {
		LOG_WRN("%s: RX overrun", dev->name);
	}

	if (isr & IER_RXIE) {
		ret = ksz8851snl_rx(dev);
	}

unlock:
	if (ret < 0) {
		LOG_ERR("%s: interrupt handling failed (%d)", dev->name, ret);
	}

	k_mutex_unlock(&ctx->lock);

	return ret;
}

static void ksz8851snl_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct ksz8851snl_runtime *ctx = dev->data;
	const struct ksz8851snl_config *cfg = dev->config;

	while (true) {
		k_sem_take(&ctx->int_sem, K_FOREVER);

		do {
			int ret = ksz8851snl_handle_interrupt(dev);

			if (ret < 0) {
				k_msleep(1);
			}
		} while (gpio_pin_get_dt(&cfg->interrupt) != 0);
	}
}

static void ksz8851snl_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				      uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct ksz8851snl_runtime *ctx = CONTAINER_OF(cb, struct ksz8851snl_runtime, gpio_cb);

	k_sem_give(&ctx->int_sem);
}

static void ksz8851snl_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct ksz8851snl_config *config = dev->config;
	struct ksz8851snl_runtime *ctx = dev->data;
	uint8_t mac_addr[NET_ETH_ADDR_LEN] = { 0 };
	int ret;

	net_eth_mac_load(&config->mac_cfg, mac_addr);

	k_mutex_lock(&ctx->lock, K_FOREVER);
	ret = ksz8851snl_set_macaddr(dev, mac_addr);
	k_mutex_unlock(&ctx->lock);
	if (ret < 0) {
		LOG_ERR("%s: setting MAC address failed (%d)", dev->name, ret);
	}

	net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);

	ctx->iface = iface;

	ethernet_init(iface);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);

	k_mutex_lock(&ctx->lock, K_FOREVER);
	ksz8851snl_update_link_status(dev);
	k_mutex_unlock(&ctx->lock);

	k_thread_create(&ctx->thread, ctx->thread_stack,
			 CONFIG_ETH_KSZ8851SNL_RX_THREAD_STACK_SIZE, ksz8851snl_thread,
			 (void *)dev, NULL, NULL,
			 /* Preemptible: a cooperative priority here can starve the
			  * shell and other threads indefinitely under sustained
			  * broadcast/multicast traffic, since k_yield() only cedes
			  * to threads of the same or higher priority.
			  */
			 K_PRIO_PREEMPT(CONFIG_ETH_KSZ8851SNL_RX_THREAD_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(&ctx->thread, "eth_ksz8851snl");
}

static enum ethernet_hw_caps ksz8851snl_get_capabilities(const struct device *dev __unused,
							  struct net_if *iface __unused)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_HW_FILTERING;
}

/* The chip indexes a 64 bit hash table with the upper 6 bits of the
 * big-endian Ethernet CRC32 (Linux ether_crc()) of the destination address.
 */
static uint8_t ksz8851snl_mcast_hash(const uint8_t *addr)
{
	return sys_bit_rev32(~crc32_ieee(addr, NET_ETH_ADDR_LEN)) >> 26U;
}

static void ksz8851snl_mcast_cb(struct net_if *iface, const struct net_eth_mcast_addr *mcast_addr,
				void *user_data)
{
	uint16_t *table = user_data;
	uint8_t hash = ksz8851snl_mcast_hash(mcast_addr->addr.addr);

	ARG_UNUSED(iface);

	table[hash / 16U] |= BIT(hash % 16U);
}

static int ksz8851snl_set_mcast_filter(const struct device *dev)
{
	struct ksz8851snl_runtime *ctx = dev->data;
	uint16_t table[4] = { 0 };
	int ret;
	int restore_ret;

	net_eth_mcast_addr_foreach(ctx->iface, ksz8851snl_mcast_cb, table);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	/* The hash table may only be modified while RX is stopped. */
	ret = ksz8851snl_reg_write(dev, KSZ8851_REG_RXCR1, KSZ8851_RXCR1);
	if (ret < 0) {
		goto unlock;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(table) && ret == 0; i++) {
		ret = ksz8851snl_reg_write(dev, KSZ8851_REG_MAHTR0 + 2U * i, table[i]);
	}

	restore_ret = ksz8851snl_reg_write(dev, KSZ8851_REG_RXCR1, KSZ8851_RXCR1 | RXCR1_RXE);
	if (restore_ret < 0) {
		LOG_ERR("%s: restoring RX after multicast filter update failed (%d)",
			dev->name, restore_ret);
		if (ret == 0) {
			ret = restore_ret;
		}
	}

unlock:
	k_mutex_unlock(&ctx->lock);

	if (ret < 0) {
		LOG_ERR("%s: multicast filter update failed (%d)", dev->name, ret);
	}

	return ret;
}

static int ksz8851snl_set_config(const struct device *dev, struct net_if *iface __unused,
				  enum ethernet_config_type type,
				  const struct ethernet_config *config)
{
	struct ksz8851snl_runtime *ctx = dev->data;
	int ret;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		k_mutex_lock(&ctx->lock, K_FOREVER);
		ret = ksz8851snl_set_macaddr(dev, config->mac_address.addr);
		k_mutex_unlock(&ctx->lock);
		if (ret < 0) {
			LOG_ERR("%s: setting MAC address failed (%d)", dev->name, ret);
			return ret;
		}

		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name,
			config->mac_address.addr[0], config->mac_address.addr[1],
			config->mac_address.addr[2], config->mac_address.addr[3],
			config->mac_address.addr[4], config->mac_address.addr[5]);

		return 0;
	case ETHERNET_CONFIG_TYPE_FILTER:
		if (config->filter.type != ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS) {
			return -ENOTSUP;
		}

		return ksz8851snl_set_mcast_filter(dev);
	default:
		return -ENOTSUP;
	}
}

static const struct ethernet_api ksz8851snl_api_funcs = {
	.iface_api.init = ksz8851snl_iface_init,
	.get_capabilities = ksz8851snl_get_capabilities,
	.set_config = ksz8851snl_set_config,
	.send = ksz8851snl_tx,
};

/* The chip ID only reads back correctly once the chip's clock has settled */
static int ksz8851snl_wait_ready(const struct device *dev)
{
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(KSZ8851_STARTUP_TIMEOUT_MS));
	uint16_t cider = 0U;
	int ret;

	do {
		ret = ksz8851snl_reg_read(dev, KSZ8851_REG_CIDER, &cider);
		if (ret == 0 && (cider & CIDER_CHIP_ID_MASK) == CIDER_FAMILY_ID) {
			return 0;
		}

		k_msleep(KSZ8851_STARTUP_POLL_MS);
	} while (!sys_timepoint_expired(end));

	LOG_ERR("Unexpected chip ID 0x%04x (%d)", cider, ret);

	return -ENODEV;
}

/* Use the reset pin if available, otherwise fall back to a soft reset */
static int ksz8851snl_reset(const struct device *dev)
{
	const struct ksz8851snl_config *config = dev->config;
	int err;

	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			LOG_ERR("GPIO port %s not ready", config->reset.port->name);
			return -EINVAL;
		}

		err = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
		if (err < 0) {
			return err;
		}

		k_msleep(KSZ8851_RESET_DELAY_MS);

		return gpio_pin_set_dt(&config->reset, 0);
	}

	err = ksz8851snl_wait_ready(dev);
	if (err < 0) {
		return err;
	}

	err = ksz8851snl_reg_write(dev, KSZ8851_REG_GRR, GRR_GLOBAL_SOFT_RESET);
	if (err < 0) {
		return err;
	}

	k_msleep(KSZ8851_RESET_DELAY_MS);

	return ksz8851snl_reg_write(dev, KSZ8851_REG_GRR, 0);
}

static const struct {
	uint8_t reg;
	uint16_t val;
} ksz8851snl_init_regs[] = {
	{ KSZ8851_REG_TXFDPR, TXFDPR_TXFPAI },
	{ KSZ8851_REG_TXCR, KSZ8851_TXCR },
	{ KSZ8851_REG_TXQCR, TXQCR_AETFE },
	{ KSZ8851_REG_RXFDPR, RXFDPR_RXFPAI },
	{ KSZ8851_REG_RXCR1, KSZ8851_RXCR1 },
	{ KSZ8851_REG_RXCR2, RXCR2_SRDBL_FRAME },
	{ KSZ8851_REG_RXQCR, KSZ8851_RXQCR },
	/* Generate a RX interrupt as soon as a single frame is received */
	{ KSZ8851_REG_RXFCTR, 1 },
	{ KSZ8851_REG_ISR, ISR_CLEAR_ALL },
	{ KSZ8851_REG_IER, KSZ8851_IRQ_MASK },
	{ KSZ8851_REG_TXCR, KSZ8851_TXCR | TXCR_TXE },
	{ KSZ8851_REG_RXCR1, KSZ8851_RXCR1 | RXCR1_RXE },
};

static int ksz8851snl_init(const struct device *dev)
{
	const struct ksz8851snl_config *config = dev->config;
	struct ksz8851snl_runtime *ctx = dev->data;
	int err;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI master port %s not ready", config->spi.bus->name);
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&config->interrupt)) {
		LOG_ERR("GPIO port %s not ready", config->interrupt.port->name);
		return -EINVAL;
	}

	err = ksz8851snl_reset(dev);
	if (err < 0) {
		LOG_ERR("Reset failed (%d)", err);
		return err;
	}

	err = ksz8851snl_wait_ready(dev);
	if (err < 0) {
		return err;
	}

	err = gpio_pin_configure_dt(&config->interrupt, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Unable to configure GPIO pin %u", config->interrupt.pin);
		return err;
	}

	gpio_init_callback(&ctx->gpio_cb, ksz8851snl_gpio_callback, BIT(config->interrupt.pin));
	err = gpio_add_callback(config->interrupt.port, &ctx->gpio_cb);
	if (err < 0) {
		LOG_ERR("Unable to add GPIO callback %u", config->interrupt.pin);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_EDGE_FALLING);
	if (err < 0) {
		LOG_ERR("Unable to enable GPIO INT %u", config->interrupt.pin);
		return err;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ksz8851snl_init_regs); i++) {
		err = ksz8851snl_reg_write(dev, ksz8851snl_init_regs[i].reg,
					   ksz8851snl_init_regs[i].val);
		if (err < 0) {
			return err;
		}
	}

	LOG_INF("KSZ8851SNL Initialized");

	return 0;
}

#define KSZ8851SNL_INST_DEFINE(inst)						\
	static struct ksz8851snl_runtime ksz8851snl_runtime_##inst = {		\
		.int_sem = Z_SEM_INITIALIZER(ksz8851snl_runtime_##inst.int_sem, 0, 1),	\
		.lock = Z_MUTEX_INITIALIZER(ksz8851snl_runtime_##inst.lock),	\
	};									\
	static const struct ksz8851snl_config ksz8851snl_config_##inst = {	\
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)),		\
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),		\
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),	\
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),		\
	};									\
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, ksz8851snl_init, NULL,		\
				       &ksz8851snl_runtime_##inst,		\
				       &ksz8851snl_config_##inst,		\
				       CONFIG_ETH_INIT_PRIORITY,		\
				       &ksz8851snl_api_funcs, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(KSZ8851SNL_INST_DEFINE)
