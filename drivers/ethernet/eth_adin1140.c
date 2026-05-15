/*
 * Copyright (c) 2024-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Ethernet driver for the ADIN1140 10BASE-T1S MAC-PHY.
 *
 * The ADIN1140 is an integrated MAC-PHY connected via SPI using the
 * Open Alliance TC6 (OA TC6) protocol. This driver handles device
 * initialization, frame transmission and reception, MAC address
 * filtering, and configuration. A dedicated thread processes
 * incoming data triggered by the IRQn GPIO interrupt.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_adin1140, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <ethernet/eth_stats.h>
#include <zephyr/net/phy.h>

#include "eth_adin1140_priv.h"

#define DT_DRV_COMPAT adi_adin1140

/**
 * Read a PHY register using MDIO Clause 22.
 */
int eth_adin1140_mdio_c22_read(const struct device *dev, uint8_t prtad, uint8_t regad,
			       uint16_t *data)
{
	struct adin1140_data *ctx = dev->data;

	return oa_tc6_mdio_read(ctx->tc6, prtad, regad, data);
}

/**
 * Write a PHY register using MDIO Clause 22.
 */
int eth_adin1140_mdio_c22_write(const struct device *dev, uint8_t prtad, uint8_t regad,
				uint16_t data)
{
	struct adin1140_data *ctx = dev->data;

	return oa_tc6_mdio_write(ctx->tc6, prtad, regad, data);
}

/**
 * Read a PHY register using MDIO Clause 45.
 */
int eth_adin1140_mdio_c45_read(const struct device *dev, uint8_t prtad, uint8_t devad,
			       uint16_t regad, uint16_t *data)
{
	struct adin1140_data *ctx = dev->data;

	return oa_tc6_mdio_read_c45(ctx->tc6, prtad, devad, regad, data);
}

/**
 * Write a PHY register using MDIO Clause 45.
 */
int eth_adin1140_mdio_c45_write(const struct device *dev, uint8_t prtad, uint8_t devad,
				uint16_t regad, uint16_t data)
{
	struct adin1140_data *ctx = dev->data;

	return oa_tc6_mdio_write_c45(ctx->tc6, prtad, devad, regad, data);
}

/**
 * Test SPI comms between host and MAC-PHY by reading
 * and verifying the value of the PHYID register.
 */
static int adin1140_spi_test(const struct device *dev)
{
	int rc;
	struct adin1140_data *ctx = dev->data;
	uint32_t val;

	rc = oa_tc6_reg_read(ctx->tc6, OA_PHYID, &val);
	if (rc < 0) {
		return rc;
	}

	if (val != ADIN1140_PHYID_RST_VAL) {
		rc = -ENODEV;
	}

	return rc;
}

/**
 * Perform a full-duplex SPI transfer for OA TC6 data transactions.
 */
static int adin1140_data_spi_transfer(const struct device *dev, uint8_t *buf_rx, uint8_t *buf_tx,
				      uint32_t len)
{
	int rc;
	const struct adin1140_config *cfg = dev->config;

	struct spi_buf tx_buf[1];
	struct spi_buf rx_buf[1];
	struct spi_buf_set tx;
	struct spi_buf_set rx;

	tx_buf[0].buf = buf_tx;
	tx_buf[0].len = len;
	rx_buf[0].buf = buf_rx;
	rx_buf[0].len = len;

	rx.buffers = rx_buf;
	rx.count = 1;
	tx.buffers = tx_buf;
	tx.count = 1;

	rc = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (rc < 0) {
		LOG_ERR("SPI transfer failed! [%d]", rc);
	}

	return rc;
}

/**
 * Perform software reset the MAC-PHY.
 */
static int adin1140_sw_reset(const struct device *dev)
{
	int rc;
	struct adin1140_data *ctx = dev->data;
	uint32_t val;

	rc = oa_tc6_reg_write(ctx->tc6, OA_RESET, OA_RESET_SWRESET);
	if (rc < 0) {
		return rc;
	}

	/* Await completion of reset */
	k_sleep(K_USEC(ADIN1140_RESET_CYCLE_TIME_US));

	/* Read reset status register */
	rc = oa_tc6_reg_read(ctx->tc6, ADIN1140_MAC_RST_STATUS, &val);
	if (rc < 0) {
		return rc;
	}

	/* Verify that MAC Crystal and MAC Oscillator clocks are ready */
	if ((val & ADIN1140_MAC_RST_STATUS_MASK) != ADIN1140_RST_COMPLETE) {
		return -ETIMEDOUT;
	}

	/* Clear RESETC in STATUS0 */
	return oa_tc6_reg_write(ctx->tc6, OA_STATUS0, OA_STATUS0_RESETC);
}

/**
 *  Write a MAC address to the filter table
 *  to enable reception of frames with that MAC address.
 */
static int adin1140_mac_filter_write(const struct device *dev,
				     const struct net_eth_addr *mac_address, int slot)
{
	int rc;
	struct adin1140_data *ctx = dev->data;

	if (slot > ADIN1140_MAC_FILT_TABLE_MAX_SLOT) {
		LOG_ERR("Failed to add address to filter table - max capacity (16) reached");
		return -ENOSPC;
	}

	/* Write to upper register must precede write to lower register */
	rc = oa_tc6_reg_write(ctx->tc6, (ADIN1140_MAC_ADDR_FILT_UPR + slot),
			      sys_get_be16(&mac_address->addr[0]) |
				      ADIN1140_MAC_ADDR_FILT_APPLY2PORT1 |
				      ADIN1140_MAC_ADDR_FILT_TO_HOST);
	if (rc < 0) {
		return rc;
	}

	return oa_tc6_reg_write(ctx->tc6, (ADIN1140_MAC_ADDR_FILT_LWR + slot),
				sys_get_be32(&mac_address->addr[2]));
}

/**
 * Search MAC filter table for an existing MAC address entry.
 */
static int adin1140_mac_filter_find(const struct device *dev,
				    const struct net_eth_addr *mac_address)
{
	int rc;
	struct adin1140_data *ctx = dev->data;
	uint32_t val;
	int slot = ADIN1140_MAC_FILT_TABLE_SLOT_SIZE;

	while (slot <= ADIN1140_MAC_FILT_TABLE_MAX_SLOT) {
		rc = oa_tc6_reg_read(ctx->tc6, (ADIN1140_MAC_ADDR_FILT_UPR + slot), &val);
		if (rc < 0) {
			return rc;
		}

		if ((val & GENMASK(15, 0)) == sys_get_be16(&mac_address->addr[0])) {
			rc = oa_tc6_reg_read(ctx->tc6, (ADIN1140_MAC_ADDR_FILT_LWR + slot), &val);
			if (rc < 0) {
				return rc;
			}

			if (val == sys_get_be32(&mac_address->addr[2])) {
				return slot;
			}
		}

		slot += ADIN1140_MAC_FILT_TABLE_SLOT_SIZE;
	}

	return -ENOENT;
}

/**
 * Find an empty slot and write MAC address to MAC filter table.
 */
static int adin1140_mac_filter_set(const struct device *dev, const struct net_eth_addr *mac_address)
{
	int rc;
	struct adin1140_data *ctx = dev->data;
	uint32_t val;
	int slot = ADIN1140_MAC_FILT_TABLE_BASE_SLOT;

	rc = adin1140_mac_filter_find(dev, mac_address);
	if (rc >= 0) {
		LOG_WRN("MAC address already in filter table!");
		return 0;
	}
	if (rc != -ENOENT) {
		return rc;
	}

	while (slot <= ADIN1140_MAC_FILT_TABLE_MAX_SLOT) {
		rc = oa_tc6_reg_read(ctx->tc6, (ADIN1140_MAC_ADDR_FILT_UPR + slot), &val);
		if (rc < 0) {
			return rc;
		}

		/* Empty slot */
		if (val == 0) {
			return adin1140_mac_filter_write(dev, mac_address, slot);
		}

		slot += ADIN1140_MAC_FILT_TABLE_SLOT_SIZE;
	}

	LOG_WRN("No free slots in MAC filter table!");
	return -ENOSPC;
}

/**
 * Clear entry from MAC filter table.
 */
static int adin1140_mac_filter_clear(const struct device *dev,
				     const struct net_eth_addr *mac_address)
{
	int slot, rc;
	struct adin1140_data *ctx = dev->data;

	slot = adin1140_mac_filter_find(dev, mac_address);
	if (slot < 0) {
		LOG_ERR("Could not find entry in MAC filter table!");
		return slot;
	}

	/* Write to upper register must precede write to lower register */
	rc = oa_tc6_reg_write(ctx->tc6, (ADIN1140_MAC_ADDR_FILT_UPR + slot), 0x0);
	if (rc < 0) {
		return rc;
	}

	return oa_tc6_reg_write(ctx->tc6, (ADIN1140_MAC_ADDR_FILT_LWR + slot), 0x0);
}

/**
 * Enable reception of unicast frames.
 */
static int adin1140_filter_unicast(const struct device *dev)
{
	struct adin1140_data *ctx = dev->data;
	struct net_eth_addr unicast_addr;

	memcpy(unicast_addr.addr, ctx->mac_address, NET_ETH_ADDR_LEN);

	return adin1140_mac_filter_write(dev, &unicast_addr, ADIN1140_MAC_FILT_TABLE_UNICAST_SLOT);
}

/**
 * Enable reception of multicast frames.
 */
static int adin1140_filter_multicast(const struct device *dev)
{
	int rc;
	struct adin1140_data *ctx = dev->data;
	struct net_eth_addr multicast_addr = {.addr = {1U, 0, 0, 0, 0, 0}};
	uint8_t mask[NET_ETH_ADDR_LEN] = {1U, 0, 0, 0, 0, 0};

	rc = adin1140_mac_filter_write(dev, &multicast_addr,
				       ADIN1140_MAC_FILT_TABLE_MULTICAST_SLOT);
	if (rc < 0) {
		return rc;
	}

	rc = oa_tc6_reg_write(ctx->tc6,
			      (ADIN1140_MAC_ADDR_MASK_UPR + ADIN1140_MAC_FILT_TABLE_MULTICAST_SLOT),
			      sys_get_be16(&mask[0]));
	if (rc < 0) {
		return rc;
	}

	return oa_tc6_reg_write(
		ctx->tc6, (ADIN1140_MAC_ADDR_MASK_LWR + ADIN1140_MAC_FILT_TABLE_MULTICAST_SLOT),
		sys_get_be32(&mask[2]));
}

/**
 * Set up default MAC address filter table entries.
 */
static int adin1140_default_filter_config(const struct device *dev)
{
	int rc;
	struct net_eth_addr broadcast_addr = {.addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

	/* Enable reception of broadcast frames */
	rc = adin1140_mac_filter_write(dev, &broadcast_addr,
				       ADIN1140_MAC_FILT_TABLE_BROADCAST_SLOT);
	if (rc < 0) {
		return rc;
	}

	/* Enable reception of unicast frames */
	rc = adin1140_filter_unicast(dev);
	if (rc < 0) {
		return rc;
	}

	/* Enable reception of multicast frames */
	return adin1140_filter_multicast(dev);
}

/**
 * Set promiscuous mode.
 * Frames with an unknown destination MAC address are forwarded to SPI host.
 */
static int adin1140_set_promiscuous_mode(const struct device *dev, bool enable)
{
	int rc;
	struct adin1140_data *ctx = dev->data;
	uint32_t val;

	rc = oa_tc6_reg_read(ctx->tc6, OA_CONFIG2, &val);
	if (rc < 0) {
		return rc;
	}

	if (enable) {
		val |= OA_CONFIG2_FWD_UNK2HOST;
	} else {
		val &= ~OA_CONFIG2_FWD_UNK2HOST;
	}

	return oa_tc6_reg_write(ctx->tc6, OA_CONFIG2, val);
}

/**
 * Indicate the MAC/PHY is configured by setting
 * the CONFIG0.SYNC bit.
 */
static int adin1140_set_sync(const struct device *dev)
{
	int rc;
	struct adin1140_data *ctx = dev->data;
	uint32_t val;

	rc = oa_tc6_reg_read(ctx->tc6, OA_CONFIG0, &val);
	if (rc < 0) {
		return rc;
	}

	val |= OA_CONFIG0_SYNC;

	return oa_tc6_reg_write(ctx->tc6, OA_CONFIG0, val);
}

/**
 * Return supported ethernet hardware capabilities.
 */
static enum ethernet_hw_caps adin1140_get_capabilities(const struct device *dev,
						       struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);
	return (ETHERNET_LINK_10BASE | ETHERNET_PROMISC_MODE | ETHERNET_HW_FILTERING |
		ETHERNET_PRIORITY_QUEUES);
}

/**
 * Handle ethernet configuration requests.
 *
 * Supports promiscuous mode, MAC address, T1S PLCA parameters,
 * and destination MAC address filtering.
 */
static int adin1140_set_config(const struct device *dev, struct net_if *iface,
			       enum ethernet_config_type type, const struct ethernet_config *config)
{
	int rc = -ENOTSUP;
	struct adin1140_data *ctx = dev->data;

	ARG_UNUSED(iface);

	switch (type) {
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		rc = adin1140_set_promiscuous_mode(dev, config->promisc_mode);
		if (rc < 0) {
			LOG_ERR("Failed to set promiscuous mode [%d]", rc);
		}
		break;

	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		/* Set MAC of Zephyr network interface */
		memcpy(ctx->mac_address, config->mac_address.addr, sizeof(ctx->mac_address));

		/* Update unicast address in MAC filter table */
		rc = adin1140_filter_unicast(dev);
		if (rc < 0) {
			LOG_ERR("Failed to update unicast filter [%d]", rc);
		}

		LOG_INF("MAC address set to: [%02X:%02X:%02X:%02X:%02X:%02X]", ctx->mac_address[0],
			ctx->mac_address[1], ctx->mac_address[2], ctx->mac_address[3],
			ctx->mac_address[4], ctx->mac_address[5]);
		break;

	case ETHERNET_CONFIG_TYPE_FILTER:
		if (config->filter.type != ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS) {
			LOG_ERR("MAC address filtering supports destination addresses only!");
			break;
		}

		if (config->filter.set) {
			rc = adin1140_mac_filter_set(dev, &config->filter.mac_address);
			if (rc < 0) {
				LOG_ERR("Failed to add MAC addr to filter table! [%d]", rc);
			}
		} else {
			rc = adin1140_mac_filter_clear(dev, &config->filter.mac_address);
			if (rc < 0) {
				LOG_ERR("Failed to remove MAC addr from filter table! [%d]", rc);
			}
		}
		break;
	default:
		break;
	}

	return rc;
}

/**
 * Process received OA TC6 chunks from oa_rx_buf and pass complete
 * frames to the network stack. Updates tc6 status from footers.
 */
static int adin1140_process_rx_chunks(const struct device *dev, uint8_t num_chunks)
{
	int rc = 0;
	struct adin1140_data *ctx = dev->data;
	struct oa_tc6 *tc6 = ctx->tc6;
	struct net_pkt *pkt;
	uint32_t ftr;
	uint16_t len, rx_idx, pkt_len;
	static uint16_t pkt_idx;
	uint8_t chunk, ebo;

	for (chunk = 0, rx_idx = 0; chunk < num_chunks; chunk++) {
		ftr = sys_be32_to_cpu(*(uint32_t *)&ctx->oa_rx_buf[rx_idx + tc6->cps]);

		if (oa_tc6_get_parity(ftr)) {
			LOG_ERR("OA Rx: Footer parity error!");
			pkt_idx = 0;
			return -EIO;
		}

		/* Update status from every valid footer */
		tc6->exst = FIELD_GET(OA_DATA_FTR_EXST, ftr);
		tc6->sync = FIELD_GET(OA_DATA_FTR_SYNC, ftr);
		tc6->rca = FIELD_GET(OA_DATA_FTR_RCA, ftr);
		tc6->txc = FIELD_GET(OA_DATA_FTR_TXC, ftr);

		if (!tc6->sync) {
			LOG_ERR("OA Rx: Configuration not SYNC'd!");
			pkt_idx = 0;
			return -EIO;
		}

		/* Skip chunks with no valid RX data (normal during piggybacked RX) */
		if (!FIELD_GET(OA_DATA_FTR_DV, ftr)) {
			rx_idx += tc6->cps + sizeof(uint32_t);
			continue;
		}

		if (FIELD_GET(OA_DATA_FTR_SV, ftr)) {
			if (FIELD_GET(OA_DATA_FTR_SWO, ftr) != 0) {
				LOG_ERR("OA Rx: Misaligned start of frame!");
				pkt_idx = 0;
				return -EIO;
			}
			pkt_idx = 0;
		}

		ebo = FIELD_GET(OA_DATA_FTR_EBO, ftr) + 1;
		len = FIELD_GET(OA_DATA_FTR_EV, ftr) ? ebo : tc6->cps;

		if (pkt_idx + len > ADIN1140_MAX_RX_FRAME_SIZE) {
			pkt_idx = 0;
			LOG_ERR("OA Rx: Frame exceeds maximum size!");
			continue;
		}

		memcpy(&ctx->pkt_buf[pkt_idx], &ctx->oa_rx_buf[rx_idx], len);
		pkt_idx += len;

		if (FIELD_GET(OA_DATA_FTR_EV, ftr)) {
			/* Minus Ethernet FCS (CRC) size */
			pkt_len = pkt_idx - sizeof(uint32_t);

			pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, pkt_len, AF_UNSPEC, 0,
							   K_MSEC(CONFIG_ETH_ADIN1140_TIMEOUT));
			if (!pkt) {
				LOG_WRN("Failed to allocate buffer for Rx packet");
				pkt_idx = 0;
				rx_idx += tc6->cps + sizeof(uint32_t);
				continue;
			}

			rc = net_pkt_write(pkt, ctx->pkt_buf, pkt_len);
			if (rc < 0) {
				net_pkt_unref(pkt);
				LOG_ERR("Failed to write net_pkt [%d]", rc);
				return rc;
			}

			rc = net_recv_data(ctx->iface, pkt);
			if (rc < 0) {
				LOG_ERR("Failed to process network Rx packet");
				net_pkt_unref(pkt);
			}
		}
		rx_idx += tc6->cps + sizeof(uint32_t);
	}

	return rc;
}

/**
 * Transform a net_pkt into OA TC6 chunks and send via SPI.
 */
static int adin1140_send_frame(const struct device *dev, struct net_pkt *pkt)
{
	int rc, ebo;
	struct adin1140_data *ctx = dev->data;
	struct oa_tc6 *tc6 = ctx->tc6;
	uint32_t hdr;
	uint16_t pkt_len = net_pkt_get_len(pkt);
	uint16_t frame_len, copy_len;
	uint8_t total_chunks, chunk;

	if (pkt_len == 0) {
		return -ENODATA;
	}

	total_chunks = pkt_len / tc6->cps;
	if (pkt_len % tc6->cps) {
		total_chunks++;
	}

	if (total_chunks > 1) {
		ebo = (pkt_len % tc6->cps) - 1;
		if (ebo < 0) {
			ebo += tc6->cps;
		}
	} else {
		ebo = tc6->cps - 1;
	}

	for (int retry = 0; total_chunks > tc6->txc && retry < 10; retry++) {
		uint32_t poll_hdr = FIELD_PREP(OA_DATA_HDR_DNC, 1);

		poll_hdr |= FIELD_PREP(OA_DATA_HDR_P, oa_tc6_get_parity(poll_hdr));

		memset(ctx->oa_tx_buf, 0, sizeof(uint32_t) + tc6->cps);
		*(uint32_t *)ctx->oa_tx_buf = sys_cpu_to_be32(poll_hdr);

		rc = adin1140_data_spi_transfer(dev, ctx->oa_rx_buf, ctx->oa_tx_buf,
						sizeof(uint32_t) + tc6->cps);
		if (rc == 0) {
			adin1140_process_rx_chunks(dev, 1);
		}
	}

	if (total_chunks > tc6->txc) {
		return -EIO;
	}

	/* Transform net_pkt into a buffer of TC6 chunks */
	for (chunk = 1, frame_len = 0; chunk <= total_chunks; chunk++) {
		hdr = FIELD_PREP(OA_DATA_HDR_DNC, 1) | FIELD_PREP(OA_DATA_HDR_DV, 1) |
		      FIELD_PREP(OA_DATA_HDR_SWO, 0);

		/* First chunk in frame - set SV */
		if (chunk == 1) {
			hdr |= FIELD_PREP(OA_DATA_HDR_SV, 1);
		}

		/* Last chunk in frame - set EV and EBO */
		if (chunk == total_chunks) {
			hdr |= FIELD_PREP(OA_DATA_HDR_EV, 1) | FIELD_PREP(OA_DATA_HDR_EBO, ebo);
		}

		/* Get parity bit and place chunk header in Tx buffer */
		hdr |= FIELD_PREP(OA_DATA_HDR_P, oa_tc6_get_parity(hdr));

		*(uint32_t *)&ctx->oa_tx_buf[frame_len] = sys_cpu_to_be32(hdr);
		frame_len += sizeof(uint32_t);

		/* Place chunk payload in Tx buffer */
		copy_len = pkt_len > tc6->cps ? tc6->cps : pkt_len;
		rc = net_pkt_read(pkt, &ctx->oa_tx_buf[frame_len], copy_len);
		if (rc < 0) {
			return rc;
		}

		frame_len += tc6->cps;
		pkt_len -= copy_len;
	}

	rc = adin1140_data_spi_transfer(dev, ctx->oa_rx_buf, ctx->oa_tx_buf, frame_len);
	if (rc < 0) {
		return rc;
	}

	return adin1140_process_rx_chunks(dev, total_chunks);
}

/**
 * Read received frames from MAC-PHY Rx FIFO and pass to the network stack.
 */
static int adin1140_rx_fifo_read(const struct device *dev)
{
	int rc;
	struct adin1140_data *ctx = dev->data;
	struct oa_tc6 *tc6 = ctx->tc6;
	uint8_t num_chunks = tc6->rca;
	uint32_t hdr;
	uint16_t len;
	uint8_t chunk;

	/* Prepare all OA Tx headers */
	for (chunk = 0, len = 0; chunk < num_chunks; chunk++) {
		hdr = FIELD_PREP(OA_DATA_HDR_DNC, 1);
		hdr |= FIELD_PREP(OA_DATA_HDR_P, oa_tc6_get_parity(hdr));

		*(uint32_t *)&ctx->oa_tx_buf[len] = sys_cpu_to_be32(hdr);
		len += sizeof(uint32_t) + tc6->cps;
	}

	rc = adin1140_data_spi_transfer(dev, ctx->oa_rx_buf, ctx->oa_tx_buf, len);
	if (rc < 0) {
		LOG_ERR("Failed to read Rx FIFO [%d]", rc);
		return rc;
	}

	return adin1140_process_rx_chunks(dev, num_chunks);
}

/**
 * Send an ethernet frame with mutex protection and statistics.
 */
static int adin1140_send(const struct device *dev, struct net_pkt *pkt)
{
	int rc;
	struct adin1140_data *ctx = dev->data;

	k_mutex_lock(&ctx->lock, K_FOREVER);

	rc = adin1140_send_frame(dev, pkt);

	/* Drain any RX data that arrived during TX (piggybacked on SPI) */
	while (ctx->tc6->rca > 0) {
		if (adin1140_rx_fifo_read(dev) < 0) {
			break;
		}
	}

	k_mutex_unlock(&ctx->lock);

	/* Wake RX thread if extended status needs handling */
	if (ctx->tc6->exst) {
		k_sem_give(&ctx->irq_sem);
	}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	if (rc < 0) {
		eth_stats_update_errors_tx(net_pkt_iface(pkt));
	} else {
		eth_stats_update_pkts_tx(ctx->iface);
	}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

	return rc;
}

/**
 * GPIO interrupt handler for MAC-PHY IRQn pin.
 */
static void adin1140_irq_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct adin1140_data *ctx = CONTAINER_OF(cb, struct adin1140_data, gpio_irq_callback);

	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	k_sem_give(&ctx->irq_sem);
}

/**
 * Enable interrupts from the MAC-PHY using IMASK0/IMASK1 registers.
 */
static int adin1140_interrupts_en(const struct device *dev)
{
	struct adin1140_data *ctx = dev->data;
	uint32_t val;

	val = ADIN1140_IMASK0_RST_VAL & ~GENMASK(12, 0);

	/* Unmask all interrupts in IMASK0 */
	return oa_tc6_reg_write(ctx->tc6, OA_IMASK0, val);
}

/**
 * Configure the features and settings of the MAC-PHY.
 */
static int adin1140_configure(const struct device *dev)
{
	int rc;
	struct adin1140_data *ctx = dev->data;
	uint32_t val, ftr;

	/* CONFIG0 */
	rc = oa_tc6_reg_read(ctx->tc6, OA_CONFIG0, &val);
	if (rc < 0) {
		return rc;
	}

	/* Zero-Align Receive Frame Enable */
	val |= OA_CONFIG0_RFA_ZARFE;

	/* Transmit Credit Threshold */
	val |= (ADIN1140_CONFIG0_TXCTHRESH_CREDIT_8 << OA_CONFIG0_TXCTHRESH);

	/* Transmit Frame Check Sequence Validation must be disabled
	 * to allow CRC appending by MAC (CONFIG2.CRC_APPEND)
	 */
	val &= ~OA_CONFIG0_TXFCSVE;

	if (ctx->tx_cut_through_en) {
		val |= OA_CONFIG0_TXCTE;
	}

	if (ctx->rx_cut_through_en) {
		val |= OA_CONFIG0_RXCTE;
	}

	/* Chunk Payload Size (CPS) */
	val &= ~GENMASK(2, 0);

	switch (ctx->tc6->cps) {
	case 64:
		val |= 0x6;
		break;
	case 32:
		val |= 0x5;
		break;
	case 16:
		val |= 0x4;
		break;
	case 8:
		val |= 0x3;
		break;
	default:
		val |= 0x6;
		LOG_WRN("Unsupported CPS! Using default (64)");
		break;
	}

	rc = oa_tc6_reg_write(ctx->tc6, OA_CONFIG0, val);
	if (rc < 0) {
		LOG_ERR("Failed to update CONFIG0 register [%d]", rc);
		return rc;
	}

	/* CONFIG2 */
	rc = oa_tc6_reg_read(ctx->tc6, OA_CONFIG2, &val);
	if (rc < 0) {
		return rc;
	}

	/* Enable CRC appending by MAC on low priority Tx queue */
	val |= OA_CONFIG2_LO_PRIO_FIFO_CRC_APPEND;

	rc = oa_tc6_reg_write(ctx->tc6, OA_CONFIG2, val);
	if (rc < 0) {
		LOG_ERR("Failed to update CONFIG2 register [%d]", rc);
		return rc;
	}

	/* Enable wake-up */
	rc = oa_tc6_reg_write(
		ctx->tc6, ADIN1140_A0_CFG_FIELDS_1,
		(ADIN1140_A0_CFG_FIELDS_1_RST_VAL | ADIN1140_A0_CFG_FIELDS_1_CFG_VALID));
	if (rc < 0) {
		LOG_ERR("Failed to enable wake-up [%d]", rc);
		return rc;
	}

	/* Enable MAC-PHY interrupts */
	rc = adin1140_interrupts_en(dev);
	if (rc < 0) {
		LOG_ERR("Failed to enable interrupts [%d]", rc);
		return rc;
	}

	/* Configure default MAC address filters */
	rc = adin1140_default_filter_config(dev);
	if (rc < 0) {
		LOG_ERR("Failed to set up MAC filter table [%d]", rc);
		return rc;
	}

	/* MAC-PHY is configured */
	rc = adin1140_set_sync(dev);
	if (rc < 0) {
		LOG_ERR("Failed to sync MAC-PHY config [%d]", rc);
		return rc;
	}

	/* Get Tx credit count (buffers) so transmission can begin */
	rc = oa_tc6_read_status(ctx->tc6, &ftr);
	if (rc < 0) {
		LOG_ERR("Failed to read MAC-PHY status [%d]", rc);
	}

	return rc;
}

/**
 * Thread dedicated to handling IRQs and incoming network data
 * from the MAC-PHY. Runs each time the IRQn pin is asserted.
 */
static void adin1140_rx_thread(const struct device *dev)
{
	int rc;
	struct adin1140_data *ctx = dev->data;
	uint32_t ftr;

	while (1) {
		/* Await IRQ from MAC-PHY */
		k_sem_take(&ctx->irq_sem, K_FOREVER);

		k_mutex_lock(&ctx->lock, K_FOREVER);

		oa_tc6_read_status(ctx->tc6, &ftr);

		/* Drain all available Rx data from FIFO */
		while (ctx->tc6->rca > 0) {
			rc = adin1140_rx_fifo_read(dev);

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
			if (rc < 0) {
				eth_stats_update_errors_rx(ctx->iface);
			} else {
				eth_stats_update_pkts_rx(ctx->iface);
			}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

			if (rc < 0) {
				break;
			}
		}

		/* Clear interrupts from the status registers.
		 * If there is a SYNC error, reset and reconfigure the MAC-PHY
		 */
		rc = oa_tc6_check_status(ctx->tc6);
		if (rc < 0) {
			rc = adin1140_sw_reset(dev);
			if (rc < 0) {
				LOG_ERR("Failed to reset MAC-PHY [%d]", rc);
			} else {
				rc = adin1140_configure(dev);
				if (rc < 0) {
					LOG_ERR("Failed to reconfigure MAC-PHY [%d]", rc);
				}
			}
		}

		k_mutex_unlock(&ctx->lock);
	}
}

/**
 * Initialize the network interface.
 */
static void adin1140_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct adin1140_data *ctx = dev->data;

	net_if_set_link_addr(iface, ctx->mac_address, sizeof(ctx->mac_address), NET_LINK_ETHERNET);

	ctx->iface = iface;

	ethernet_init(iface);

	/* Configure the MAC-PHY features and settings */
	int rc;

	rc = adin1140_configure(dev);
	if (rc < 0) {
		LOG_ERR("Failed to configure MAC-PHY [%d]", rc);
		return;
	}

	/* Start up thread to offload IRQ/Rx data handling  */
	k_thread_create(&ctx->rx_thread, ctx->rx_thread_stack,
			CONFIG_ETH_ADIN1140_RX_THREAD_STACK_SIZE,
			(k_thread_entry_t)adin1140_rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_ADIN1140_RX_THREAD_PRIO), K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&ctx->rx_thread, "eth_adin1140_rx");
}

/**
 * Device driver init API.
 */
static int adin1140_init(const struct device *dev)
{
	int rc;
	const struct adin1140_config *cfg = dev->config;
	struct adin1140_data *ctx = dev->data;

	rc = net_eth_mac_load(&cfg->mac_cfg, ctx->mac_address);

	if (rc == -ENODATA) {
		LOG_DBG("No MAC address configured for %s", dev->name);
	} else if (rc < 0) {
		LOG_ERR("Failed to load MAC address (%d)", rc);
		return rc;
	}

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI %s is not ready", cfg->spi.bus->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->interrupt)) {
		LOG_ERR("Interrupt GPIO %s is not ready", cfg->interrupt.port->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->wake)) {
		LOG_ERR("WAKE GPIO %s is not ready", cfg->wake.port->name);
		return -ENODEV;
	}

	/* Verify SPI comms between host and MAC-PHY */
	rc = adin1140_spi_test(dev);
	if (rc < 0) {
		LOG_ERR("Failed to communicate with MAC-PHY over SPI [%d]", rc);
		return rc;
	}

	/* Reset MAC-PHY */
	rc = adin1140_sw_reset(dev);
	if (rc < 0) {
		LOG_ERR("Failed to reset MAC-PHY [%d]", rc);
		return rc;
	}

	/* Config MAC-PHY IRQn interrupt service routine */
	rc = gpio_pin_configure_dt(&cfg->interrupt, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Failed to configure IRQn GPIO [%d]", rc);
		return rc;
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc != 0) {
		LOG_ERR("Failed to configure interrupt on IRQn pin [%d]", rc);
		return rc;
	}

	gpio_init_callback(&(ctx->gpio_irq_callback), adin1140_irq_callback,
			   BIT(cfg->interrupt.pin));

	rc = gpio_add_callback(cfg->interrupt.port, &ctx->gpio_irq_callback);
	if (rc < 0) {
		LOG_ERR("Failed to add IRQn callback [%d]", rc);
		return rc;
	}

	/* Configure WAKE pin */
	rc = gpio_pin_configure_dt(&cfg->wake, GPIO_OUTPUT_INACTIVE);
	if (rc < 0) {
		LOG_ERR("Failed to configure WAKE GPIO pin [%d]", rc);
		return rc;
	}

	LOG_INF("10BASE-T1S Initialized [%02X:%02X:%02X:%02X:%02X:%02X]", ctx->mac_address[0],
		ctx->mac_address[1], ctx->mac_address[2], ctx->mac_address[3], ctx->mac_address[4],
		ctx->mac_address[5]);

	return rc;
}

static const struct ethernet_api adin1140_eth_api = {
	.iface_api.init = adin1140_iface_init,
	.get_capabilities = adin1140_get_capabilities,
	.set_config = adin1140_set_config,
	.send = adin1140_send,
};

#define ADIN1140_DEFINE(inst)                                                                      \
	BUILD_ASSERT(DT_INST_PROP(inst, spi_max_frequency) <= ADIN1140_SPI_MAX_FREQUENCY,          \
		     "SPI clock frequency exceeds supported maximum");                             \
	static uint8_t __aligned(4) buffer_##inst[ADIN1140_MAX_RX_FRAME_SIZE];                     \
	static const struct adin1140_config adin1140_config_##inst = {                             \
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)),                                \
		.interrupt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                               \
		.wake = GPIO_DT_SPEC_INST_GET(inst, wake_gpios),                                   \
		.phy = DEVICE_DT_GET(DT_INST_PHANDLE(inst, phy_handle)),                           \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),                                  \
	};                                                                                         \
	static struct oa_tc6 oa_tc6_##inst = {                                                     \
		.cps = 64,                                                                         \
		.protected = 1,                                                                    \
		.spi = &adin1140_config_##inst.spi,                                                \
	};                                                                                         \
	static struct adin1140_data adin1140_data_##inst = {                                       \
		.lock = Z_MUTEX_INITIALIZER((adin1140_data_##inst).lock),                          \
		.irq_sem = Z_SEM_INITIALIZER((adin1140_data_##inst).irq_sem, 0, 1),                \
		.tc6 = &oa_tc6_##inst,                                                             \
		.pkt_buf = buffer_##inst,                                                          \
		.tx_cut_through_en = DT_INST_PROP(inst, tx_cut_through_en),                        \
		.rx_cut_through_en = DT_INST_PROP(inst, rx_cut_through_en),                        \
	};                                                                                         \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, adin1140_init, NULL, &adin1140_data_##inst,            \
				      &adin1140_config_##inst, CONFIG_ETH_INIT_PRIORITY,           \
				      &adin1140_eth_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(ADIN1140_DEFINE);
