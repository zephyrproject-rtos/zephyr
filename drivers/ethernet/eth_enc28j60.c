/* ENC28J60 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_enc28j60

#define LOG_MODULE_NAME eth_enc28j60
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <drivers/gpio.h>
#include <drivers/spi.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <ethernet/eth_stats.h>

#include "eth_enc28j60_priv.h"

#define D10D24S 11

static int eth_enc28j60_soft_reset(const struct device *dev)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint8_t buf[2] = { ENC28J60_SPI_SC, 0xFF };
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 1,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	return spi_write(context->spi, &context->spi_cfg, &tx);
}

static void eth_enc28j60_set_bank(const struct device *dev, uint16_t reg_addr)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint8_t buf[2];
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 2
	};
	const struct spi_buf rx_buf = {
		.buf = buf,
		.len = 2
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	buf[0] = ENC28J60_SPI_RCR | ENC28J60_REG_ECON1;
	buf[1] = 0x0;

	if (!spi_transceive(context->spi, &context->spi_cfg, &tx, &rx)) {
		buf[0] = ENC28J60_SPI_WCR | ENC28J60_REG_ECON1;
		buf[1] = (buf[1] & 0xFC) | ((reg_addr >> 8) & 0x0F);

		spi_write(context->spi, &context->spi_cfg, &tx);
	} else {
		LOG_DBG("Failure while setting bank to 0x%04x", reg_addr);
	}
}

static void eth_enc28j60_write_reg(const struct device *dev,
				   uint16_t reg_addr,
				   uint8_t value)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint8_t buf[2];
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 2
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	buf[0] = ENC28J60_SPI_WCR | (reg_addr & 0xFF);
	buf[1] = value;

	spi_write(context->spi, &context->spi_cfg, &tx);
}

static void eth_enc28j60_read_reg(const struct device *dev, uint16_t reg_addr,
				  uint8_t *value)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint8_t buf[3];
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 2
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf = {
		.buf = buf,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};
	uint8_t rx_size = 2U;

	if (reg_addr & 0xF000) {
		rx_size = 3U;
	}

	rx_buf.len = rx_size;

	buf[0] = ENC28J60_SPI_RCR | (reg_addr & 0xFF);
	buf[1] = 0x0;

	if (!spi_transceive(context->spi, &context->spi_cfg, &tx, &rx)) {
		*value = buf[rx_size - 1];
	} else {
		LOG_DBG("Failure while reading register 0x%04x", reg_addr);
		*value = 0U;
	}
}

static void eth_enc28j60_set_eth_reg(const struct device *dev,
				     uint16_t reg_addr,
				     uint8_t value)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint8_t buf[2];
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 2
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	buf[0] = ENC28J60_SPI_BFS | (reg_addr & 0xFF);
	buf[1] = value;

	spi_write(context->spi, &context->spi_cfg, &tx);
}


static void eth_enc28j60_clear_eth_reg(const struct device *dev,
				       uint16_t reg_addr,
				       uint8_t value)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint8_t buf[2];
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 2
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	buf[0] = ENC28J60_SPI_BFC | (reg_addr & 0xFF);
	buf[1] = value;

	spi_write(context->spi, &context->spi_cfg, &tx);
}

static void eth_enc28j60_write_mem(const struct device *dev,
				   uint8_t *data_buffer,
				   uint16_t buf_len)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint8_t buf[1] = { ENC28J60_SPI_WBM };
	struct spi_buf tx_buf[2] = {
		{
			.buf = buf,
			.len = 1
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2
	};
	uint16_t num_segments;
	uint16_t num_remaining;
	int i;

	num_segments = buf_len / MAX_BUFFER_LENGTH;
	num_remaining = buf_len - MAX_BUFFER_LENGTH * num_segments;

	for (i = 0; i < num_segments; i++, data_buffer += MAX_BUFFER_LENGTH) {
		tx_buf[1].buf = data_buffer;
		tx_buf[1].len = MAX_BUFFER_LENGTH;

		if (spi_write(context->spi, &context->spi_cfg, &tx)) {
			LOG_ERR("Failed to write memory");
			return;
		}
	}

	if (num_remaining > 0) {
		tx_buf[1].buf = data_buffer;
		tx_buf[1].len = num_remaining;

		if (spi_write(context->spi, &context->spi_cfg, &tx)) {
			LOG_ERR("Failed to write memory");
		}
	}
}

static void eth_enc28j60_read_mem(const struct device *dev,
				  uint8_t *data_buffer,
				  uint16_t buf_len)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint8_t buf[1] = { ENC28J60_SPI_RBM };
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
			.len = 1
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};
	uint16_t num_segments;
	uint16_t num_remaining;
	int i;

	num_segments = buf_len / MAX_BUFFER_LENGTH;
	num_remaining = buf_len - MAX_BUFFER_LENGTH * num_segments;

	for (i = 0; i < num_segments; i++, data_buffer += MAX_BUFFER_LENGTH) {

		rx_buf[1].buf = data_buffer;
		rx_buf[1].len = MAX_BUFFER_LENGTH;

		if (spi_transceive(context->spi, &context->spi_cfg, &tx, &rx)) {
			LOG_ERR("Failed to read memory");
			return;
		}
	}

	if (num_remaining > 0) {
		rx_buf[1].buf = data_buffer;
		rx_buf[1].len = num_remaining;

		if (spi_transceive(context->spi, &context->spi_cfg, &tx, &rx)) {
			LOG_ERR("Failed to read memory");
		}
	}
}

static void eth_enc28j60_write_phy(const struct device *dev,
				   uint16_t reg_addr,
				   int16_t data)
{
	uint8_t data_mistat;

	eth_enc28j60_set_bank(dev, ENC28J60_REG_MIREGADR);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MIREGADR, reg_addr);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MIWRL, data & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MIWRH, data >> 8);
	eth_enc28j60_set_bank(dev, ENC28J60_REG_MISTAT);

	do {
		/* wait 10.24 useconds */
		k_busy_wait(D10D24S);
		eth_enc28j60_read_reg(dev, ENC28J60_REG_MISTAT,
				      &data_mistat);
	} while ((data_mistat & ENC28J60_BIT_MISTAT_BUSY));
}

static void eth_enc28j60_gpio_callback(const struct device *dev,
				       struct gpio_callback *cb,
				       uint32_t pins)
{
	struct eth_enc28j60_runtime *context =
		CONTAINER_OF(cb, struct eth_enc28j60_runtime, gpio_cb);

	k_sem_give(&context->int_sem);
}

static void eth_enc28j60_init_buffers(const struct device *dev)
{
	uint8_t data_estat;

	/* Reception buffers initialization */
	eth_enc28j60_set_bank(dev, ENC28J60_REG_ERXSTL);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ERXSTL,
			       ENC28J60_RXSTART & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ERXSTH,
			       ENC28J60_RXSTART >> 8);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ERXRDPTL,
			       ENC28J60_RXSTART & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ERXRDPTH,
			       ENC28J60_RXSTART >> 8);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ERXNDL,
			       ENC28J60_RXEND & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ERXNDH,
			       ENC28J60_RXEND >> 8);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ETXSTL,
			       ENC28J60_TXSTART & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ETXSTH,
			       ENC28J60_TXSTART >> 8);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ETXNDL,
			       ENC28J60_TXEND & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ETXNDH,
			       ENC28J60_TXEND >> 8);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ERDPTL,
			       ENC28J60_RXSTART & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ERDPTH,
			       ENC28J60_RXSTART >> 8);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_EWRPTL,
			       ENC28J60_TXSTART & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_EWRPTH,
			       ENC28J60_TXSTART >> 8);

	eth_enc28j60_set_bank(dev, ENC28J60_REG_ERXFCON);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ERXFCON,
			       ENC28J60_RECEIVE_FILTERS);

	/* Waiting for OST */
	do {
		/* wait 10.24 useconds */
		k_busy_wait(D10D24S);
		eth_enc28j60_read_reg(dev, ENC28J60_REG_ESTAT, &data_estat);
	} while (!(data_estat & ENC28J60_BIT_ESTAT_CLKRDY));
}

static void eth_enc28j60_init_mac(const struct device *dev)
{
	const struct eth_enc28j60_config *config = dev->config;
	struct eth_enc28j60_runtime *context = dev->data;
	uint8_t data_macon;

	eth_enc28j60_set_bank(dev, ENC28J60_REG_MACON1);

	/* Set MARXEN to enable MAC to receive frames */
	eth_enc28j60_read_reg(dev, ENC28J60_REG_MACON1, &data_macon);
	data_macon |= ENC28J60_BIT_MACON1_MARXEN | ENC28J60_BIT_MACON1_RXPAUS
		      | ENC28J60_BIT_MACON1_TXPAUS;
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MACON1, data_macon);

	data_macon = ENC28J60_MAC_CONFIG;

	if (config->full_duplex) {
		data_macon |= ENC28J60_BIT_MACON3_FULDPX;
	}

	eth_enc28j60_write_reg(dev, ENC28J60_REG_MACON3, data_macon);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAIPGL, ENC28J60_MAC_NBBIPGL);

	if (config->full_duplex) {
		eth_enc28j60_write_reg(dev, ENC28J60_REG_MAIPGH,
				       ENC28J60_MAC_NBBIPGH);
		eth_enc28j60_write_reg(dev, ENC28J60_REG_MABBIPG,
				       ENC28J60_MAC_BBIPG_FD);
	} else {
		eth_enc28j60_write_reg(dev, ENC28J60_REG_MABBIPG,
				       ENC28J60_MAC_BBIPG_HD);
		eth_enc28j60_write_reg(dev, ENC28J60_REG_MACON4, 1 << 6);
	}

	/* Configure MAC address */
	eth_enc28j60_set_bank(dev, ENC28J60_REG_MAADR1);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR6,
			       context->mac_address[5]);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR5,
			       context->mac_address[4]);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR4,
			       context->mac_address[3]);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR3,
			       context->mac_address[2]);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR2,
			       context->mac_address[1]);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR1,
			       context->mac_address[0]);
}

static void eth_enc28j60_init_phy(const struct device *dev)
{
	const struct eth_enc28j60_config *config = dev->config;

	if (config->full_duplex) {
		eth_enc28j60_write_phy(dev, ENC28J60_PHY_PHCON1,
				       ENC28J60_BIT_PHCON1_PDPXMD);
		eth_enc28j60_write_phy(dev, ENC28J60_PHY_PHCON2, 0x0);
	} else {
		eth_enc28j60_write_phy(dev, ENC28J60_PHY_PHCON1, 0x0);
		eth_enc28j60_write_phy(dev, ENC28J60_PHY_PHCON2,
				       ENC28J60_BIT_PHCON2_HDLDIS);
	}
}

static struct net_if *get_iface(struct eth_enc28j60_runtime *ctx,
				uint16_t vlan_tag)
{
#if defined(CONFIG_NET_VLAN)
	struct net_if *iface;

	iface = net_eth_get_vlan_iface(ctx->iface, vlan_tag);
	if (!iface) {
		return ctx->iface;
	}

	return iface;
#else
	ARG_UNUSED(vlan_tag);

	return ctx->iface;
#endif
}

static int eth_enc28j60_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint16_t tx_bufaddr = ENC28J60_TXSTART;
	uint16_t len = net_pkt_get_len(pkt);
	uint8_t per_packet_control;
	uint16_t tx_bufaddr_end;
	struct net_buf *frag;
	uint8_t tx_end;

	LOG_DBG("pkt %p (len %u)", pkt, len);

	k_sem_take(&context->tx_rx_sem, K_FOREVER);

	/* Latest errata sheet: DS80349C
	* always reset transmit logic (Errata Issue 12)
	* the Microchip TCP/IP stack implementation used to first check
	* whether TXERIF is set and only then reset the transmit logic
	* but this has been changed in later versions; possibly they
	* have a reason for this; they don't mention this in the errata
	* sheet
	*/
	eth_enc28j60_set_eth_reg(dev, ENC28J60_REG_ECON1,
				 ENC28J60_BIT_ECON1_TXRST);
	eth_enc28j60_clear_eth_reg(dev, ENC28J60_REG_ECON1,
				   ENC28J60_BIT_ECON1_TXRST);

	/* Write the buffer content into the transmission buffer */
	eth_enc28j60_set_bank(dev, ENC28J60_REG_ETXSTL);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_EWRPTL, tx_bufaddr & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_EWRPTH, tx_bufaddr >> 8);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ETXSTL, tx_bufaddr & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ETXSTH, tx_bufaddr >> 8);

	/* Write the data into the buffer */
	per_packet_control = ENC28J60_PPCTL_BYTE;
	eth_enc28j60_write_mem(dev, &per_packet_control, 1);

	for (frag = pkt->frags; frag; frag = frag->frags) {
		eth_enc28j60_write_mem(dev, frag->data, frag->len);
	}

	tx_bufaddr_end = tx_bufaddr + len;
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ETXNDL,
			       tx_bufaddr_end & 0xFF);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_ETXNDH, tx_bufaddr_end >> 8);

	/* Signal ENC28J60 to send the buffer */
	eth_enc28j60_set_eth_reg(dev, ENC28J60_REG_ECON1,
				 ENC28J60_BIT_ECON1_TXRTS);

	do {
		/* wait 10.24 useconds */
		k_busy_wait(D10D24S);
		eth_enc28j60_read_reg(dev, ENC28J60_REG_EIR, &tx_end);
		tx_end &= ENC28J60_BIT_EIR_TXIF;
	} while (!tx_end);


	eth_enc28j60_read_reg(dev, ENC28J60_REG_ESTAT, &tx_end);

	k_sem_give(&context->tx_rx_sem);

	if (tx_end & ENC28J60_BIT_ESTAT_TXABRT) {
		LOG_ERR("TX failed!");
		return -EIO;
	}

	LOG_DBG("Tx successful");

	return 0;
}

static void enc28j60_read_packet(const struct device *dev, uint16_t *vlan_tag,
				 uint16_t frm_len)
{
	const struct eth_enc28j60_config *config = dev->config;
	struct eth_enc28j60_runtime *context = dev->data;
	struct net_buf *pkt_buf;
	struct net_pkt *pkt;
	uint16_t lengthfr;
	uint8_t dummy[4];

	/* Get the frame from the buffer */
	pkt = net_pkt_rx_alloc_with_buffer(get_iface(context, *vlan_tag), frm_len,
					   AF_UNSPEC, 0, K_MSEC(config->timeout));
	if (!pkt) {
		LOG_ERR("Could not allocate rx buffer");
		eth_stats_update_errors_rx(get_iface(context, *vlan_tag));
		return;
	}

	pkt_buf = pkt->buffer;
	lengthfr = frm_len;

	do {
		size_t frag_len;
		uint8_t *data_ptr;
		size_t spi_frame_len;

		data_ptr = pkt_buf->data;

		/* Review the space available for the new frag */
		frag_len = net_buf_tailroom(pkt_buf);

		if (frm_len > frag_len) {
			spi_frame_len = frag_len;
		} else {
			spi_frame_len = frm_len;
		}

		eth_enc28j60_read_mem(dev, data_ptr, spi_frame_len);

		net_buf_add(pkt_buf, spi_frame_len);

		/* One fragment has been written via SPI */
		frm_len -= spi_frame_len;
		pkt_buf = pkt_buf->frags;
	} while (frm_len > 0);

	/* Let's pop the useless CRC */
	eth_enc28j60_read_mem(dev, dummy, 4);

	/* Pops one padding byte from spi circular buffer
	 * introduced by the device when the frame length is odd
	 */
	if (lengthfr & 0x01) {
		eth_enc28j60_read_mem(dev, dummy, 1);
	}

#if defined(CONFIG_NET_VLAN)
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

	if (ntohs(hdr->type) == NET_ETH_PTYPE_VLAN) {
		struct net_eth_vlan_hdr *hdr_vlan =
			(struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);

		net_pkt_set_vlan_tci(pkt, ntohs(hdr_vlan->vlan.tci));
		*vlan_tag = net_pkt_vlan_tag(pkt);

#if CONFIG_NET_TC_RX_COUNT > 1
		enum net_priority prio;

		prio = net_vlan2priority(net_pkt_vlan_priority(pkt));
		net_pkt_set_priority(pkt, prio);
#endif
	} else {
		net_pkt_set_iface(pkt, context->iface);
	}
#else /* CONFIG_NET_VLAN */
	net_pkt_set_iface(pkt, context->iface);
#endif /* CONFIG_NET_VLAN */

	/* Feed buffer frame to IP stack */
	LOG_DBG("Received packet of length %u", lengthfr);
	if (net_recv_data(net_pkt_iface(pkt), pkt) < 0) {
		net_pkt_unref(pkt);
	}
}

static int eth_enc28j60_rx(const struct device *dev, uint16_t *vlan_tag)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint8_t counter;

	/* Errata 6. The Receive Packet Pending Interrupt Flag (EIR.PKTIF)
	 * does not reliably/accurately report the status of pending packet.
	 * Use EPKTCNT register instead.
	*/
	eth_enc28j60_set_bank(dev, ENC28J60_REG_EPKTCNT);
	eth_enc28j60_read_reg(dev, ENC28J60_REG_EPKTCNT, &counter);
	if (!counter) {
		return 0;
	}

	k_sem_take(&context->tx_rx_sem, K_FOREVER);

	do {
		uint16_t frm_len = 0U;
		uint8_t info[RSV_SIZE];
		uint16_t next_packet;
		uint8_t rdptl = 0U;
		uint8_t rdpth = 0U;

		/* remove read fifo address to packet header address */
		eth_enc28j60_set_bank(dev, ENC28J60_REG_ERXRDPTL);
		eth_enc28j60_read_reg(dev, ENC28J60_REG_ERXRDPTL, &rdptl);
		eth_enc28j60_read_reg(dev, ENC28J60_REG_ERXRDPTH, &rdpth);
		eth_enc28j60_write_reg(dev, ENC28J60_REG_ERDPTL, rdptl);
		eth_enc28j60_write_reg(dev, ENC28J60_REG_ERDPTH, rdpth);

		/* Read address for next packet */
		eth_enc28j60_read_mem(dev, info, 2);
		next_packet = info[0] | (uint16_t)info[1] << 8;

		/* Errata 14. Even values in ERXRDPT
		 * may corrupt receive buffer.
		 No need adjust next packet
		if (next_packet == 0) {
			next_packet = ENC28J60_RXEND;
		} else if (!(next_packet & 0x01)) {
			next_packet--;
		}*/

		/* Read reception status vector */
		eth_enc28j60_read_mem(dev, info, 4);

		/* Get the frame length from the rx status vector,
		 * minus CRC size at the end which is always present
		 */
		frm_len = sys_get_le16(info) - 4;

		enc28j60_read_packet(dev, vlan_tag, frm_len);

		/* Free buffer memory and decrement rx counter */
		eth_enc28j60_set_bank(dev, ENC28J60_REG_ERXRDPTL);
		eth_enc28j60_write_reg(dev, ENC28J60_REG_ERXRDPTL,
				       next_packet & 0xFF);
		eth_enc28j60_write_reg(dev, ENC28J60_REG_ERXRDPTH,
				       next_packet >> 8);
		eth_enc28j60_set_eth_reg(dev, ENC28J60_REG_ECON2,
					 ENC28J60_BIT_ECON2_PKTDEC);

		/* Check if there are frames to clean from the buffer */
		eth_enc28j60_set_bank(dev, ENC28J60_REG_EPKTCNT);
		eth_enc28j60_read_reg(dev, ENC28J60_REG_EPKTCNT, &counter);
	} while (counter);

	k_sem_give(&context->tx_rx_sem);

	return 0;
}

static void eth_enc28j60_rx_thread(const struct device *dev)
{
	struct eth_enc28j60_runtime *context = dev->data;
	uint16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	uint8_t int_stat;

	while (true) {
		k_sem_take(&context->int_sem, K_FOREVER);

		eth_enc28j60_read_reg(dev, ENC28J60_REG_EIR, &int_stat);
		if (int_stat & ENC28J60_BIT_EIR_PKTIF) {
			eth_enc28j60_rx(dev, &vlan_tag);
			/* Clear rx interruption flag */
			eth_enc28j60_clear_eth_reg(dev, ENC28J60_REG_EIR,
						   ENC28J60_BIT_EIR_PKTIF
						   | ENC28J60_BIT_EIR_RXERIF);
		}
	}
}

static enum ethernet_hw_caps eth_enc28j60_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T
#if defined(CONFIG_NET_VLAN)
		| ETHERNET_HW_VLAN
#endif
		;
}

static void eth_enc28j60_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_enc28j60_runtime *context = dev->data;

	net_if_set_link_addr(iface, context->mac_address,
			     sizeof(context->mac_address),
			     NET_LINK_ETHERNET);

	/* For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (context->iface == NULL) {
		context->iface = iface;
	}

	ethernet_init(iface);
}

static const struct ethernet_api api_funcs = {
	.iface_api.init		= eth_enc28j60_iface_init,

	.get_capabilities	= eth_enc28j60_get_capabilities,
	.send			= eth_enc28j60_tx,
};

static int eth_enc28j60_init(const struct device *dev)
{
	const struct eth_enc28j60_config *config = dev->config;
	struct eth_enc28j60_runtime *context = dev->data;

	/* SPI config */
	context->spi_cfg.operation = SPI_WORD_SET(8);
	context->spi_cfg.frequency = config->spi_freq;
	context->spi_cfg.slave = config->spi_slave;

	context->spi = device_get_binding((char *)config->spi_port);
	if (!context->spi) {

		LOG_ERR("SPI master port %s not found", config->spi_port);
		return -EINVAL;
	}

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	context->spi_cs.gpio_dev =
		device_get_binding((char *)config->spi_cs_port);
	if (!context->spi_cs.gpio_dev) {
		LOG_ERR("SPI CS port %s not found", config->spi_cs_port);
		return -EINVAL;
	}

	context->spi_cs.gpio_pin = config->spi_cs_pin;
	context->spi_cs.gpio_dt_flags = config->spi_cs_dt_flags;
	context->spi_cfg.cs = &context->spi_cs;
#endif

	/* Initialize GPIO */
	context->gpio = device_get_binding((char *)config->gpio_port);
	if (!context->gpio) {
		LOG_ERR("GPIO port %s not found", config->gpio_port);
		return -EINVAL;
	}

	if (gpio_pin_configure(context->gpio, config->gpio_pin,
			       GPIO_INPUT | config->gpio_flags)) {
		LOG_ERR("Unable to configure GPIO pin %u", config->gpio_pin);
		return -EINVAL;
	}

	gpio_init_callback(&(context->gpio_cb), eth_enc28j60_gpio_callback,
			   BIT(config->gpio_pin));

	if (gpio_add_callback(context->gpio, &(context->gpio_cb))) {
		return -EINVAL;
	}

	gpio_pin_interrupt_configure(context->gpio,
				     config->gpio_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);

	if (eth_enc28j60_soft_reset(dev)) {
		LOG_ERR("Soft-reset failed");
		return -EIO;
	}

	/* Errata B7/1 */
	k_busy_wait(D10D24S);

	/* Assign octets not previously taken from devicetree */
	context->mac_address[0] = MICROCHIP_OUI_B0;
	context->mac_address[1] = MICROCHIP_OUI_B1;
	context->mac_address[2] = MICROCHIP_OUI_B2;

	eth_enc28j60_init_buffers(dev);
	eth_enc28j60_init_mac(dev);
	eth_enc28j60_init_phy(dev);

	/* Enable interruptions */
	eth_enc28j60_set_eth_reg(dev, ENC28J60_REG_EIE, ENC28J60_BIT_EIE_INTIE);
	eth_enc28j60_set_eth_reg(dev, ENC28J60_REG_EIE, ENC28J60_BIT_EIE_PKTIE);

	/* Enable Reception */
	eth_enc28j60_set_eth_reg(dev, ENC28J60_REG_ECON1,
				 ENC28J60_BIT_ECON1_RXEN);

	/* Start interruption-poll thread */
	k_thread_create(&context->thread, context->thread_stack,
			CONFIG_ETH_ENC28J60_RX_THREAD_STACK_SIZE,
			(k_thread_entry_t)eth_enc28j60_rx_thread,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_ENC28J60_RX_THREAD_PRIO),
			0, K_NO_WAIT);

	LOG_INF("ENC28J60 Initialized");

	return 0;
}

#ifdef CONFIG_ETH_ENC28J60_0

static struct eth_enc28j60_runtime eth_enc28j60_0_runtime = {
	.mac_address = DT_INST_PROP(0, local_mac_address),
	.tx_rx_sem = Z_SEM_INITIALIZER(eth_enc28j60_0_runtime.tx_rx_sem,
					1,  UINT_MAX),
	.int_sem  = Z_SEM_INITIALIZER(eth_enc28j60_0_runtime.int_sem,
				       0, UINT_MAX),
};

static const struct eth_enc28j60_config eth_enc28j60_0_config = {
	.gpio_port = DT_INST_GPIO_LABEL(0, int_gpios),
	.gpio_pin = DT_INST_GPIO_PIN(0, int_gpios),
	.gpio_flags = DT_INST_GPIO_FLAGS(0, int_gpios),
	.spi_port = DT_INST_BUS_LABEL(0),
	.spi_freq  = DT_INST_PROP(0, spi_max_frequency),
	.spi_slave = DT_INST_REG_ADDR(0),
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	.spi_cs_port = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
	.spi_cs_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0),
	.spi_cs_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0),
#endif
	.full_duplex = IS_ENABLED(CONFIG_ETH_ENC28J60_0_FULL_DUPLEX),
	.timeout = CONFIG_ETH_ENC28J60_TIMEOUT,
};

ETH_NET_DEVICE_DT_INST_DEFINE(0,
		    eth_enc28j60_init, device_pm_control_nop,
		    &eth_enc28j60_0_runtime, &eth_enc28j60_0_config,
		    CONFIG_ETH_INIT_PRIORITY, &api_funcs, NET_ETH_MTU);

#endif /* CONFIG_ETH_ENC28J60_0 */
