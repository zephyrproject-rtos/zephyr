/* W6300 Stand-alone Ethernet Controller with SPI/QSPI
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
#include <stddef.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi)
#include <zephyr/drivers/mspi/devicetree.h>
#endif
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <ethernet/eth_stats.h>

#include "eth.h"
#include "eth_w6300_priv.h"

static inline uint8_t w6300_instr(uint8_t mod, uint8_t rwb, uint8_t bsb)
{
	return (uint8_t)((mod << 6) | ((rwb & 0x1) << 5) | (bsb & 0x1f));
}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

static int w6300_spi_bus_init(const struct device *dev)
{
	const struct w6300_config *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->bus.spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	return 0;
}

static int w6300_spi_read(const struct device *dev, uint8_t bsb,
			  uint16_t addr, uint8_t *data, size_t len)
{
	const struct w6300_config *cfg = dev->config;
	/* W6300 SPI read: 3-byte command + 1 dummy byte before data */
	uint8_t header[4] = {
		w6300_instr(W6300_SPI_MOD_SINGLE, W6300_SPI_RWB_READ, bsb),
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

	return spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
}

static int w6300_spi_write(const struct device *dev, uint8_t bsb,
			   uint16_t addr, uint8_t *data, size_t len)
{
	const struct w6300_config *cfg = dev->config;
	/* W6300 SPI write: 3-byte command + 1 dummy byte before data */
	uint8_t header[4] = {
		w6300_instr(W6300_SPI_MOD_SINGLE, W6300_SPI_RWB_WRITE, bsb),
		(uint8_t)(addr >> 8),
		(uint8_t)addr,
		0x00, /* dummy byte */
	};
	const struct spi_buf tx_bufs[2] = {
		{
			.buf = header,
			.len = sizeof(header),
		},
		{
			.buf = data,
			.len = len,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	return spi_write_dt(&cfg->bus.spi, &tx);
}

static const struct w6300_bus_io w6300_bus_io_spi = {
	.init = w6300_spi_bus_init,
	.read = w6300_spi_read,
	.write = w6300_spi_write,
};

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi)

/*
 * Both read and write include one dummy byte between the address and data
 * phases.  In quad mode one byte = 2 clock cycles, so dummy = 2 clocks.
 */
#define W6300_MSPI_DUMMY_CLOCKS 2U

static int w6300_mspi_bus_init(const struct device *dev)
{
	const struct w6300_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->bus.mspi.ctlr)) {
		LOG_ERR("MSPI bus not ready");
		return -ENODEV;
	}

	ret = mspi_dev_config(cfg->bus.mspi.ctlr, &cfg->bus.mspi.dev_id,
			      MSPI_DEVICE_CONFIG_ALL, &cfg->bus.mspi.dev_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure W6300 on MSPI bus: %d", ret);
	}

	return ret;
}

static int w6300_mspi_read(const struct device *dev, uint8_t bsb,
			   uint16_t addr, uint8_t *data, size_t len)
{
	const struct w6300_config *cfg = dev->config;
	struct mspi_xfer_packet pkt = {
		.dir = MSPI_RX,
		.cmd = w6300_instr(W6300_SPI_MOD_QUAD, W6300_SPI_RWB_READ, bsb),
		.address = addr,
		.data_buf = data,
		.num_bytes = len,
	};
	struct mspi_xfer xfer = {
		.async = false,
		.xfer_mode = MSPI_PIO,
		.rx_dummy = W6300_MSPI_DUMMY_CLOCKS,
		.tx_dummy = 0,
		.cmd_length = 1,
		.addr_length = 2,
		.hold_ce = false,
		.packets = &pkt,
		.num_packet = 1,
		.timeout = CONFIG_ETH_W6300_TIMEOUT,
	};

	return mspi_transceive(cfg->bus.mspi.ctlr, &cfg->bus.mspi.dev_id, &xfer);
}

static int w6300_mspi_write(const struct device *dev, uint8_t bsb,
			    uint16_t addr, uint8_t *data, size_t len)
{
	const struct w6300_config *cfg = dev->config;
	struct mspi_xfer_packet pkt = {
		.dir = MSPI_TX,
		.cmd = w6300_instr(W6300_SPI_MOD_QUAD, W6300_SPI_RWB_WRITE, bsb),
		.address = addr,
		.data_buf = data,
		.num_bytes = len,
	};
	struct mspi_xfer xfer = {
		.async = false,
		.xfer_mode = MSPI_PIO,
		.rx_dummy = 0,
		.tx_dummy = W6300_MSPI_DUMMY_CLOCKS,
		.cmd_length = 1,
		.addr_length = 2,
		.hold_ce = false,
		.packets = &pkt,
		.num_packet = 1,
		.timeout = CONFIG_ETH_W6300_TIMEOUT,
	};

	return mspi_transceive(cfg->bus.mspi.ctlr, &cfg->bus.mspi.dev_id, &xfer);
}

static const struct w6300_bus_io w6300_bus_io_mspi = {
	.init = w6300_mspi_bus_init,
	.read = w6300_mspi_read,
	.write = w6300_mspi_write,
};

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(mspi) */

static inline int w6300_bus_read(const struct device *dev, uint8_t bsb,
				 uint16_t addr, uint8_t *data, size_t len)
{
	const struct w6300_config *cfg = dev->config;

	return cfg->bus_io->read(dev, bsb, addr, data, len);
}

static inline int w6300_bus_write(const struct device *dev, uint8_t bsb,
				  uint16_t addr, uint8_t *data, size_t len)
{
	const struct w6300_config *cfg = dev->config;

	return cfg->bus_io->write(dev, bsb, addr, data, len);
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

	ret = w6300_bus_read(dev, bsb, off, buf, first);
	if (ret || first == len) {
		return ret;
	}

	return w6300_bus_read(dev, bsb, 0, buf + first, len - first);
}

static int w6300_buf_write(const struct device *dev, uint8_t bsb,
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

	ret = w6300_bus_write(dev, bsb, off, buf, first);
	if (ret || first == len) {
		return ret;
	}

	return w6300_bus_write(dev, bsb, 0, buf + first, len - first);
}

static int w6300_command(const struct device *dev, uint8_t cmd)
{
	struct w6300_runtime *ctx = dev->data;
	uint8_t reg;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(W6300_CMD_TIMEOUT_MS));
	int ret;

	k_mutex_lock(&ctx->cmd_lock, K_FOREVER);

	ret = w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_CR, &cmd, 1);
	if (ret < 0) {
		goto out;
	}

	while (true) {
		ret = w6300_bus_read(dev, W6300_BSB_SOCK(0), W6300_Sn_CR, &reg, 1);
		if (ret < 0) {
			goto out;
		}

		if (reg == 0) {
			ret = 0;
			goto out;
		}

		if (sys_timepoint_expired(end)) {
			ret = -EIO;
			goto out;
		}

		k_busy_wait(W6300_CMD_POLL_US);
	}

out:
	k_mutex_unlock(&ctx->cmd_lock);

	return ret;
}

static int w6300_set_macaddr(const struct device *dev)
{
	struct w6300_runtime *ctx = dev->data;

	return w6300_bus_write(dev, W6300_BSB_COMMON, W6300_SHAR,
			       ctx->mac_addr, sizeof(ctx->mac_addr));
}

static int w6300_set_buffer_sizes(const struct device *dev)
{
	struct w6300_runtime *ctx = dev->data;
	uint8_t bsr;
	int ret;

	/*
	 * Socket 0 runs in MACRAW mode and the other hardware sockets are
	 * unused, so give socket 0 the entire 16 KB TX and RX buffer memory
	 * and assign none to sockets 1-7.  A large socket-0 RX buffer is
	 * required under load: the W6300 silently drops incoming frames when
	 * its socket RX buffer fills, which appears as lost ICMP/UDP during
	 * stress.  This mirrors the upstream W5500 driver's
	 * w5500_memory_configure().
	 */
	bsr = W6300_SOCK0_BSR_KB;
	ret = w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_TX_BSR, &bsr, 1);
	if (ret < 0) {
		return ret;
	}
	ret = w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_RX_BSR, &bsr, 1);
	if (ret < 0) {
		return ret;
	}
	ctx->tx_buf_size = W6300_BSR_TO_BYTES(bsr);
	ctx->rx_buf_size = W6300_BSR_TO_BYTES(bsr);

	bsr = 0;
	for (uint8_t i = 1; i < W6300_NUM_SOCKETS; i++) {
		ret = w6300_bus_write(dev, W6300_BSB_SOCK(i), W6300_Sn_TX_BSR, &bsr, 1);
		if (ret < 0) {
			return ret;
		}
		ret = w6300_bus_write(dev, W6300_BSB_SOCK(i), W6300_Sn_RX_BSR, &bsr, 1);
		if (ret < 0) {
			return ret;
		}
	}

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

	k_sem_reset(&ctx->tx_sem);

	ret = w6300_bus_read(dev, W6300_BSB_SOCK(0), W6300_Sn_TX_WR, tmp, 2);
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
	ret = w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_TX_WR, tmp, 2);
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

static int w6300_drop_rx(const struct device *dev, uint16_t off,
			 uint16_t drop_len)
{
	uint8_t tmp[2];
	int ret;

	sys_put_be16(off + drop_len, tmp);
	ret = w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_RX_RD, tmp, 2);
	if (ret < 0) {
		return ret;
	}

	return w6300_command(dev, W6300_Sn_CR_RECV);
}

static void w6300_rx(const struct device *dev)
{
	uint8_t hdr[W6300_PKT_INFO_LEN];
	uint8_t ring[6];
	struct w6300_runtime *ctx = dev->data;

	while (1) {
		uint16_t rx_buf_len;
		uint16_t off;
		uint16_t consumed = 0;
		unsigned int batch = 0;

		/*
		 * Sn_RX_RSR and Sn_RX_RD are four bytes apart, so one six byte
		 * read picks up both and gives a consistent snapshot of the ring
		 * at the cost of a single transaction.
		 */
		if (w6300_bus_read(dev, W6300_BSB_SOCK(0), W6300_Sn_RX_RSR,
				   ring, sizeof(ring)) < 0) {
			return;
		}

		rx_buf_len = sys_get_be16(&ring[0]);
		if (rx_buf_len < W6300_PKT_INFO_LEN) {
			break;
		}

		off = sys_get_be16(&ring[4]);

		while (batch < W6300_RX_BATCH_MAX &&
		       consumed < W6300_RX_BATCH_BYTES &&
		       (uint16_t)(rx_buf_len - consumed) >= W6300_PKT_INFO_LEN) {
			uint16_t reader = off + consumed;
			uint16_t total_len;
			uint16_t frame_len;
			uint16_t read_len;
			struct net_buf *pkt_buf;
			struct net_pkt *pkt;
			bool read_ok = true;

			if (w6300_buf_read(dev, W6300_BSB_RX(0), reader, hdr,
					   sizeof(hdr), ctx->rx_buf_size) < 0) {
				eth_stats_update_errors_rx(ctx->iface);
				consumed = rx_buf_len;
				break;
			}

			/* MACRAW length includes the 2-byte packet info header. */
			total_len = sys_get_be16(hdr);
			frame_len = (total_len >= W6300_PKT_INFO_LEN)
				  ? (uint16_t)(total_len - W6300_PKT_INFO_LEN) : 0;

			if (frame_len < W6300_ETH_MIN_FRAME_LEN ||
			    frame_len > NET_ETH_MAX_FRAME_SIZE ||
			    total_len > (uint16_t)(rx_buf_len - consumed)) {
				/*
				 * The length field is the only thing tying one frame
				 * to the next, so a bad one leaves no way to find the
				 * frame after it: give up on the rest of the buffer.
				 */
				eth_stats_update_errors_rx(ctx->iface);
				consumed = rx_buf_len;
				break;
			}

			consumed += total_len;
			batch++;

			pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, frame_len,
							   NET_AF_UNSPEC, 0,
							   K_MSEC(CONFIG_ETH_W6300_TIMEOUT));
			if (!pkt) {
				eth_stats_update_errors_rx(ctx->iface);
				continue;
			}

			pkt_buf = pkt->buffer;
			read_len = frame_len;
			reader += W6300_PKT_INFO_LEN;

			while (read_len > 0 && pkt_buf != NULL) {
				size_t chunk_len = MIN((size_t)read_len,
						       net_buf_tailroom(pkt_buf));

				if (w6300_buf_read(dev, W6300_BSB_RX(0), reader,
						   pkt_buf->data, chunk_len,
						   ctx->rx_buf_size) < 0) {
					eth_stats_update_errors_rx(ctx->iface);
					net_pkt_unref(pkt);
					read_ok = false;
					break;
				}

				net_buf_add(pkt_buf, chunk_len);
				reader += (uint16_t)chunk_len;
				read_len -= (uint16_t)chunk_len;
				pkt_buf = pkt_buf->frags;
			}

			if (!read_ok || read_len > 0) {
				if (read_len > 0) {
					eth_stats_update_errors_rx(ctx->iface);
					net_pkt_unref(pkt);
				}
				continue;
			}

			if (net_recv_data(ctx->iface, pkt) < 0) {
				net_pkt_unref(pkt);
				eth_stats_update_errors_rx(ctx->iface);
			}
		}

		if (consumed == 0) {
			break;
		}

		if (w6300_drop_rx(dev, off, consumed) < 0) {
			return;
		}
	}
}

static void w6300_update_link_status(const struct device *dev)
{
	uint8_t physr;
	struct w6300_runtime *ctx = dev->data;
	enum phy_link_speed speed;

	if (w6300_bus_read(dev, W6300_BSB_COMMON, W6300_PHYSR, &physr, 1) < 0) {
		return;
	}

	if (physr & W6300_PHYSR_LNK) {
		if (!ctx->state.is_up) {
			ctx->state.is_up = true;
			net_eth_carrier_on(ctx->iface);
		}

		/* W6300 PHYSR reports SPD as 1=10M and DPX as 1=half duplex. */
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
		if (ctx->state.is_up) {
			ctx->state.is_up = false;
			ctx->state.speed = 0;
			net_eth_carrier_off(ctx->iface);
		}
	}
}

static void w6300_handle_interrupts(const struct device *dev,
				    const struct gpio_dt_spec *interrupt)
{
	struct w6300_runtime *ctx = dev->data;

	while (gpio_pin_get_dt(interrupt) > 0) {
		uint8_t ir;

		if (w6300_bus_read(dev, W6300_BSB_SOCK(0), W6300_Sn_IR,
				   &ir, 1) < 0 || ir == 0U) {
			break;
		}

		w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_IRCLR, &ir, 1);

		if ((ir & W6300_Sn_IR_SENDOK) != 0U) {
			k_sem_give(&ctx->tx_sem);
		}

		if ((ir & W6300_Sn_IR_RECV) != 0U) {
			w6300_rx(dev);
		}
	}
}

static void w6300_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	int res;
	struct w6300_runtime *ctx = dev->data;
	const struct w6300_config *config = dev->config;

	while (true) {
		res = k_sem_take(&ctx->int_sem,
				 K_MSEC(CONFIG_ETH_W6300_MONITOR_PERIOD));

		/*
		 * Polling the PHY costs a bus transaction, so only do it in the
		 * idle slot.  Servicing runs either way: w6300_handle_interrupts()
		 * checks the interrupt line itself and returns immediately when
		 * there is nothing pending.
		 */
		if (res == -EAGAIN) {
			w6300_update_link_status(dev);
		}

		if (ctx->state.is_up) {
			w6300_handle_interrupts(dev, &config->interrupt);
		}
	}
}

static void w6300_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct w6300_runtime *ctx = dev->data;

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr),
			     NET_LINK_ETHERNET);

	ctx->iface = iface;

	ethernet_init(iface);

	/* Do not start the interface until PHY link is up. */
	net_if_carrier_off(iface);

	/* Create RX thread after iface is set */
	k_thread_create(&ctx->thread, ctx->thread_stack,
			CONFIG_ETH_W6300_RX_THREAD_STACK_SIZE,
			w6300_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_W6300_RX_THREAD_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&ctx->thread, "eth_w6300");
}

static enum ethernet_hw_caps w6300_get_capabilities(const struct device *dev,
						    struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
	;
}

static int w6300_set_config(const struct device *dev,
			    struct net_if *iface,
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

		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (!IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			return -ENOTSUP;
		}

		if (w6300_bus_read(dev, W6300_BSB_SOCK(0), W6300_Sn_MR, &mode, 1) < 0) {
			return -EIO;
		}

		if (config->promisc_mode) {
			mode &= (uint8_t)~W6300_Sn_MR_MF;
		} else {
			mode |= W6300_Sn_MR_MF;
		}

		return w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_MR,
					&mode, 1);
	default:
		return -ENOTSUP;
	}
}

static int w6300_hw_start(const struct device *dev, struct net_if *iface)
{
	uint8_t mode = W6300_Sn_MR_MACRAW | W6300_Sn_MR_MF;
	uint8_t imr = W6300_Sn_IR_SENDOK | W6300_Sn_IR_RECV;
	uint8_t simr = BIT(0);
	uint8_t irclr = 0xFF;
	int ret;

	ARG_UNUSED(iface);

	ret = w6300_set_buffer_sizes(dev);
	if (ret < 0) {
		return ret;
	}

	w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_MR, &mode, 1);
	ret = w6300_command(dev, W6300_Sn_CR_OPEN);
	if (ret < 0) {
		return ret;
	}

	w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_IRCLR, &irclr, 1);
	w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_IMR, &imr, 1);
	w6300_bus_write(dev, W6300_BSB_COMMON, W6300_SIMR, &simr, 1);

	return 0;
}

static int w6300_hw_stop(const struct device *dev, struct net_if *iface)
{
	uint8_t mask = 0;

	ARG_UNUSED(iface);

	w6300_bus_write(dev, W6300_BSB_COMMON, W6300_SIMR, &mask, 1);
	w6300_bus_write(dev, W6300_BSB_SOCK(0), W6300_Sn_IMR, &mask, 1);
	w6300_command(dev, W6300_Sn_CR_CLOSE);

	return 0;
}

static const struct device *w6300_get_phy(const struct device *dev,
					  struct net_if *iface)
{
	const struct w6300_config *config = dev->config;

	ARG_UNUSED(iface);

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

	ret = w6300_bus_write(dev, W6300_BSB_COMMON, W6300_SYCR0, &reg, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(1);
	return 0;
}

static int w6300_configure_defaults(const struct device *dev)
{
	uint8_t reg;

	if (w6300_bus_read(dev, W6300_BSB_COMMON, W6300_SYCR1, &reg, 1) < 0) {
		return -EIO;
	}

	reg |= W6300_SYCR1_IEN;
	if (w6300_bus_write(dev, W6300_BSB_COMMON, W6300_SYCR1, &reg, 1) < 0) {
		return -EIO;
	}

	reg = 0xFF;
	w6300_bus_write(dev, W6300_BSB_COMMON, W6300_IRCLR, &reg, 1);

	return 0;
}

static int w6300_init(const struct device *dev)
{
	int err;
	uint8_t cidr[2];
	uint8_t cidr2;
	const struct w6300_config *config = dev->config;
	struct w6300_runtime *ctx = dev->data;

	err = config->bus_io->init(dev);
	if (err < 0) {
		return err;
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
		k_msleep(10);
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(150);
	}

	err = w6300_soft_reset(dev);
	if (err != 0) {
		LOG_ERR("Reset failed");
		return err;
	}

	if (w6300_bus_read(dev, W6300_BSB_COMMON, W6300_CIDR0, cidr, 2) < 0) {
		LOG_ERR("Unable to read CIDR");
		return -EIO;
	}

	if (cidr[0] != 0x61 || cidr[1] != 0x00) {
		LOG_ERR("Unexpected CIDR %02x %02x", cidr[0], cidr[1]);
		return -ENODEV;
	}

	if (w6300_bus_read(dev, W6300_BSB_COMMON, W6300_CIDR2, &cidr2, 1) == 0) {
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

	LOG_INF("W6300 initialized");

	return 0;
}

#define W6300_BUS_CFG_SPI(inst) \
	.bus_io = &w6300_bus_io_spi, \
	.bus.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)),

#define W6300_BUS_CFG_MSPI(inst) \
	.bus_io = &w6300_bus_io_mspi, \
	.bus.mspi = { \
		.ctlr = DEVICE_DT_GET(DT_INST_BUS(inst)), \
		.dev_id = MSPI_DEVICE_ID_DT_INST(inst), \
		.dev_cfg = MSPI_DEVICE_CONFIG_DT_INST(inst), \
	},

#define W6300_BUS_CFG(inst) \
	COND_CODE_1(DT_INST_ON_BUS(inst, mspi), \
		    (W6300_BUS_CFG_MSPI(inst)), \
		    (W6300_BUS_CFG_SPI(inst)))

#define W6300_INST_DEFINE(inst) \
	DEVICE_DECLARE(eth_w6300_phy_##inst); \
	static struct w6300_runtime w6300_runtime_##inst = { \
		.cmd_lock = Z_MUTEX_INITIALIZER(w6300_runtime_##inst.cmd_lock), \
		.tx_sem = Z_SEM_INITIALIZER(w6300_runtime_##inst.tx_sem, 0, UINT_MAX), \
		.int_sem = Z_SEM_INITIALIZER(w6300_runtime_##inst.int_sem, 0, UINT_MAX), \
	}; \
	static const struct w6300_config w6300_config_##inst = { \
		W6300_BUS_CFG(inst) \
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios), \
		.reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, { 0 }), \
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
