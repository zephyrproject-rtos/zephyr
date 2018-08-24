/* ENC28J60 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ETHERNET_LEVEL
#define SYS_LOG_DOMAIN "dev/enc28j60"
#include <logging/sys_log.h>

#include <zephyr.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <gpio.h>
#include <spi.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>

#include "eth_enc28j60_priv.h"

#define D10D24S 11

static int eth_enc28j60_soft_reset(struct device *dev)
{
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u8_t buf[2] = { ENC28J60_SPI_SC, 0xFF };
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

static void eth_enc28j60_set_bank(struct device *dev, u16_t reg_addr)
{
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u8_t buf[2];
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
		SYS_LOG_DBG("Failure while setting bank to 0x%04x", reg_addr);
	}
}

static void eth_enc28j60_write_reg(struct device *dev, u16_t reg_addr,
				   u8_t value)
{
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u8_t buf[2];
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

static void eth_enc28j60_read_reg(struct device *dev, u16_t reg_addr,
				  u8_t *value)
{
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u8_t buf[3];
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
	u8_t rx_size = 2;

	if (reg_addr & 0xF000) {
		rx_size = 3;
	}

	rx_buf.len = rx_size;

	buf[0] = ENC28J60_SPI_RCR | (reg_addr & 0xFF);
	buf[1] = 0x0;

	if (!spi_transceive(context->spi, &context->spi_cfg, &tx, &rx)) {
		*value = buf[rx_size - 1];
	} else {
		SYS_LOG_DBG("Failure while reading register 0x%04x", reg_addr);
		*value = 0;
	}
}

static void eth_enc28j60_set_eth_reg(struct device *dev, u16_t reg_addr,
				     u8_t value)
{
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u8_t buf[2];
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


static void eth_enc28j60_clear_eth_reg(struct device *dev, u16_t reg_addr,
				       u8_t value)
{
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u8_t buf[2];
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

static void eth_enc28j60_write_mem(struct device *dev, u8_t *data_buffer,
				   u16_t buf_len)
{
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u8_t buf[1] = { ENC28J60_SPI_WBM };
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
	u16_t num_segments;
	u16_t num_remaining;
	int i;

	num_segments = buf_len / MAX_BUFFER_LENGTH;
	num_remaining = buf_len - MAX_BUFFER_LENGTH * num_segments;

	for (i = 0; i < num_segments; i++, data_buffer += MAX_BUFFER_LENGTH) {
		tx_buf[1].buf = data_buffer;
		tx_buf[1].len = MAX_BUFFER_LENGTH;

		if (spi_write(context->spi, &context->spi_cfg, &tx)) {
			SYS_LOG_ERR("Failed to write memory");
			return;
		}
	}

	if (num_remaining > 0) {
		tx_buf[1].buf = data_buffer;
		tx_buf[1].len = num_remaining;

		if (spi_write(context->spi, &context->spi_cfg, &tx)) {
			SYS_LOG_ERR("Failed to write memory");
		}
	}
}

static void eth_enc28j60_read_mem(struct device *dev, u8_t *data_buffer,
				  u16_t buf_len)
{
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u8_t buf[1] = { ENC28J60_SPI_RBM };
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
	u16_t num_segments;
	u16_t num_remaining;
	int i;

	num_segments = buf_len / MAX_BUFFER_LENGTH;
	num_remaining = buf_len - MAX_BUFFER_LENGTH * num_segments;

	for (i = 0; i < num_segments; i++, data_buffer += MAX_BUFFER_LENGTH) {

		rx_buf[1].buf = data_buffer;
		rx_buf[1].len = MAX_BUFFER_LENGTH;

		if (spi_transceive(context->spi, &context->spi_cfg, &tx, &rx)) {
			SYS_LOG_ERR("Failed to read memory");
			return;
		}
	}

	if (num_remaining > 0) {
		rx_buf[1].buf = data_buffer;
		rx_buf[1].len = num_remaining;

		if (spi_transceive(context->spi, &context->spi_cfg, &tx, &rx)) {
			SYS_LOG_ERR("Failed to read memory");
		}
	}
}

static void eth_enc28j60_write_phy(struct device *dev, u16_t reg_addr,
				   s16_t data)
{
	u8_t data_mistat;

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

static void eth_enc28j60_gpio_callback(struct device *dev,
				       struct gpio_callback *cb,
				       u32_t pins)
{
	struct eth_enc28j60_runtime *context =
		CONTAINER_OF(cb, struct eth_enc28j60_runtime, gpio_cb);

	k_sem_give(&context->int_sem);
}

static void eth_enc28j60_init_buffers(struct device *dev)
{
	u8_t data_estat;

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

static void eth_enc28j60_init_mac(struct device *dev)
{
	const struct eth_enc28j60_config *config = dev->config->config_info;
	u8_t data_macon;

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
	eth_enc28j60_set_bank(dev, ENC28J60_REG_MAADR0);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR0,
			       CONFIG_ETH_ENC28J60_0_MAC5);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR1,
			       CONFIG_ETH_ENC28J60_0_MAC4);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR2,
			       CONFIG_ETH_ENC28J60_0_MAC3);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR3, MICROCHIP_OUI_B2);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR4, MICROCHIP_OUI_B1);
	eth_enc28j60_write_reg(dev, ENC28J60_REG_MAADR5, MICROCHIP_OUI_B0);
}

static void eth_enc28j60_init_phy(struct device *dev)
{
	const struct eth_enc28j60_config *config = dev->config->config_info;

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

static int eth_enc28j60_tx(struct net_if *iface, struct net_pkt *pkt)
{
	struct device *dev = net_if_get_device(iface);
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u16_t len = net_pkt_ll_reserve(pkt) + net_pkt_get_len(pkt);
	u16_t tx_bufaddr = ENC28J60_TXSTART;
	bool first_frag = true;
	u8_t per_packet_control;
	u16_t tx_bufaddr_end;
	struct net_buf *frag;
	u8_t tx_end;

	SYS_LOG_DBG("pkt %p (len %u)", pkt, len);

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
		u8_t *data_ptr;
		u16_t data_len;

		if (first_frag) {
			data_ptr = net_pkt_ll(pkt);
			data_len = net_pkt_ll_reserve(pkt) + frag->len;
			first_frag = false;
		} else {
			data_ptr = frag->data;
			data_len = frag->len;
		}

		eth_enc28j60_write_mem(dev, data_ptr, data_len);
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
		SYS_LOG_ERR("TX failed!");
		return -EIO;
	}

	net_pkt_unref(pkt);

	SYS_LOG_DBG("Tx successful");

	return 0;
}

static int eth_enc28j60_rx(struct device *dev)
{
	const struct eth_enc28j60_config *config = dev->config->config_info;
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u16_t lengthfr;
	u8_t counter;

	/* Errata 6. The Receive Packet Pending Interrupt Flag (EIR.PKTIF)
	 * does not reliably/accurately report the status of pending packet.
	 * Use EPKTCNT register instead.
	*/
	eth_enc28j60_set_bank(dev, ENC28J60_REG_EPKTCNT);
	eth_enc28j60_read_reg(dev, ENC28J60_REG_EPKTCNT, &counter);
	if (!counter) {
		return 0;
	}

	SYS_LOG_DBG("");

	k_sem_take(&context->tx_rx_sem, K_FOREVER);

	do {
		struct net_buf *pkt_buf = NULL;
		struct net_buf *last_buf = NULL;
		u16_t frm_len = 0;
		u8_t info[RSV_SIZE];
		struct net_pkt *pkt;
		u16_t next_packet;
		u8_t rdptl = 0;
		u8_t rdpth = 0;

		/* remove read fifo address to packet header address */
		eth_enc28j60_set_bank(dev, ENC28J60_REG_ERXRDPTL);
		eth_enc28j60_read_reg(dev, ENC28J60_REG_ERXRDPTL, &rdptl);
		eth_enc28j60_read_reg(dev, ENC28J60_REG_ERXRDPTH, &rdpth);
		eth_enc28j60_write_reg(dev, ENC28J60_REG_ERDPTL, rdptl);
		eth_enc28j60_write_reg(dev, ENC28J60_REG_ERDPTH, rdpth);

		/* Read address for next packet */
		eth_enc28j60_read_mem(dev, info, 2);
		next_packet = info[0] | (u16_t)info[1] << 8;

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
		frm_len = (info[1] << 8) | (info[0] - 4);
		lengthfr = frm_len;

		/* Get the frame from the buffer */
		pkt = net_pkt_get_reserve_rx(0, config->timeout);
		if (!pkt) {
			SYS_LOG_ERR("Could not allocate rx buffer");
			goto done;
		}

		do {
			size_t frag_len;
			u8_t *data_ptr;
			size_t spi_frame_len;

			/* Reserve a data frag to receive the frame */
			pkt_buf = net_pkt_get_frag(pkt, config->timeout);
			if (!pkt_buf) {
				SYS_LOG_ERR("Could not allocate data buffer");
				net_pkt_unref(pkt);

				goto done;
			}

			if (!last_buf) {
				net_pkt_frag_insert(pkt, pkt_buf);
			} else {
				net_buf_frag_insert(last_buf, pkt_buf);
			}

			last_buf = pkt_buf;

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
		} while (frm_len > 0);

		/* Let's pop the useless CRC */
		eth_enc28j60_read_mem(dev, NULL, 4);

		/* Pops one padding byte from spi circular buffer
		 * introduced by the device when the frame length is odd
		 */
		if (lengthfr & 0x01) {
			eth_enc28j60_read_mem(dev, NULL, 1);
		}

		/* Feed buffer frame to IP stack */
		SYS_LOG_DBG("Received packet of length %u", lengthfr);
		net_recv_data(context->iface, pkt);
done:
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

static void eth_enc28j60_rx_thread(struct device *dev)
{
	struct eth_enc28j60_runtime *context = dev->driver_data;
	u8_t int_stat;

	while (true) {
		k_sem_take(&context->int_sem, K_FOREVER);

		eth_enc28j60_read_reg(dev, ENC28J60_REG_EIR, &int_stat);
		if (int_stat & ENC28J60_BIT_EIR_PKTIF) {
			eth_enc28j60_rx(dev);
			/* Clear rx interruption flag */
			eth_enc28j60_clear_eth_reg(dev, ENC28J60_REG_EIR,
						   ENC28J60_BIT_EIR_PKTIF
						   | ENC28J60_BIT_EIR_RXERIF);
		}
	}
}

static enum ethernet_hw_caps eth_enc28j60_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T;
}

static void eth_enc28j60_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct eth_enc28j60_runtime *context = dev->driver_data;

	SYS_LOG_DBG("");

	net_if_set_link_addr(iface, context->mac_address,
			     sizeof(context->mac_address),
			     NET_LINK_ETHERNET);
	context->iface = iface;
}

static const struct ethernet_api api_funcs = {
	.iface_api.init		= eth_enc28j60_iface_init,
	.iface_api.send		= eth_enc28j60_tx,

	.get_capabilities	= eth_enc28j60_get_capabilities,
};

static int eth_enc28j60_init(struct device *dev)
{
	const struct eth_enc28j60_config *config = dev->config->config_info;
	struct eth_enc28j60_runtime *context = dev->driver_data;

	/* SPI config */
	context->spi_cfg.operation = SPI_WORD_SET(8);
	context->spi_cfg.frequency = config->spi_freq;
	context->spi_cfg.slave = config->spi_slave;

	context->spi = device_get_binding((char *)config->spi_port);
	if (!context->spi) {

		SYS_LOG_ERR("SPI master port %s not found", config->spi_port);
		return -EINVAL;
	}

#ifdef CONFIG_ETH_ENC28J60_0_GPIO_SPI_CS
	context->spi_cs.gpio_dev =
		device_get_binding((char *)config->spi_cs_port);
	if (!context->spi_cs.gpio_dev) {
		SYS_LOG_ERR("SPI CS port %s not found", config->spi_cs_port);
		return -EINVAL;
	}

	context->spi_cs.gpio_pin = config->spi_cs_pin;
	context->spi_cfg.cs = &context->spi_cs;
#endif /* CONFIG_ETH_ENC28J60_0_GPIO_SPI_CS */

	/* Initialize GPIO */
	context->gpio = device_get_binding((char *)config->gpio_port);
	if (!context->gpio) {
		SYS_LOG_ERR("GPIO port %s not found", config->gpio_port);
		return -EINVAL;
	}

	if (gpio_pin_configure(context->gpio, config->gpio_pin,
			       (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE
			       | GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE))) {
		SYS_LOG_ERR("Unable to configure GPIO pin %u",
			    config->gpio_pin);
		return -EINVAL;
	}

	gpio_init_callback(&(context->gpio_cb), eth_enc28j60_gpio_callback,
			   BIT(config->gpio_pin));

	if (gpio_add_callback(context->gpio, &(context->gpio_cb))) {
		return -EINVAL;
	}

	if (gpio_pin_enable_callback(context->gpio, config->gpio_pin)) {
		return -EINVAL;
	}

	if (eth_enc28j60_soft_reset(dev)) {
		SYS_LOG_ERR("Soft-reset failed");
		return -EIO;
	}

	/* Errata B7/1 */
	k_busy_wait(D10D24S);

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

	SYS_LOG_INF("ENC28J60 Initialized");

	return 0;
}

#ifdef CONFIG_ETH_ENC28J60_0

static struct eth_enc28j60_runtime eth_enc28j60_0_runtime = {
	.mac_address = {
		MICROCHIP_OUI_B0,
		MICROCHIP_OUI_B1,
		MICROCHIP_OUI_B2,
		CONFIG_ETH_ENC28J60_0_MAC3,
		CONFIG_ETH_ENC28J60_0_MAC4,
		CONFIG_ETH_ENC28J60_0_MAC5
	},
	.tx_rx_sem = _K_SEM_INITIALIZER(eth_enc28j60_0_runtime.tx_rx_sem,
					1,  UINT_MAX),
	.int_sem  = _K_SEM_INITIALIZER(eth_enc28j60_0_runtime.int_sem,
				       0, UINT_MAX),
};

static const struct eth_enc28j60_config eth_enc28j60_0_config = {
	.gpio_port = CONFIG_ETH_ENC28J60_0_GPIO_PORT_NAME,
	.gpio_pin = CONFIG_ETH_ENC28J60_0_GPIO_PIN,
	.spi_port = CONFIG_ETH_ENC28J60_0_SPI_PORT_NAME,
	.spi_freq  = CONFIG_ETH_ENC28J60_0_SPI_BUS_FREQ,
	.spi_slave = CONFIG_ETH_ENC28J60_0_SLAVE,
#ifdef CONFIG_ETH_ENC28J60_0_GPIO_SPI_CS
	.spi_cs_port = CONFIG_ETH_ENC28J60_0_SPI_CS_PORT_NAME,
	.spi_cs_pin = CONFIG_ETH_ENC28J60_0_SPI_CS_PIN,
#endif /* CONFIG_ETH_ENC28J60_0_GPIO_SPI_CS */
	.full_duplex = CONFIG_ETH_EN28J60_0_FULL_DUPLEX,
	.timeout = CONFIG_ETH_EN28J60_TIMEOUT,
};

NET_DEVICE_INIT(enc28j60_0, CONFIG_ETH_ENC28J60_0_NAME,
		eth_enc28j60_init, &eth_enc28j60_0_runtime,
		&eth_enc28j60_0_config, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2), 1500);

#endif /* CONFIG_ETH_ENC28J60_0 */
