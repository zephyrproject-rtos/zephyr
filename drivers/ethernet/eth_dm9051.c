/* Davicom DM9051 Ethernet Controller with SPI interface
 *
 * Copyright (c) 2026 Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	davicom_dm9051

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_dm9051, CONFIG_ETHERNET_LOG_LEVEL);

#include <string.h>
#include <errno.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_pkt.h>
#include <ethernet/eth_stats.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/clock.h>
#include <zephyr/net/phy.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include "eth.h"

/* DM9051 Product ID */
#define DM9051_ID			0x9051

/* DM9051 PHY Address */
#define DM9051_PHY_ADDR			1

/* DM9051 Registers */
/* NCR - Network Control Register */
#define DM9051_NCR			0x00
/* NSR - Network Status Register */
#define DM9051_NSR			0x01
/* TCR - TX Control Register */
#define DM9051_TCR			0x02
/* RCR - RX Control Register */
#define DM9051_RCR			0x05
/* EPCR - EEPROM & PHY Control Register */
#define DM9051_EPCR			0x0B
/* EPAR - EEPROM & PHY Address Register */
#define DM9051_EPAR			0x0C
/* EPDRL - EEPROM & PHY Low Byte Data Register */
#define DM9051_EPDRL			0x0D
/* PAR - Physical Address Register */
#define DM9051_PAR			0x10
/* MAR - Multicast Address Hash Table Register */
#define DM9051_MAR			0x16
/* GPR - General Purpose Register */
#define DM9051_GPR			0x1F
/* PIDL - Product ID Low Byte Register */
#define DM9051_PIDL			0x2A
/* INTCR - INT Pin Control Register */
#define DM9051_INTCR			0x39
/* MRCMDX - Memory Data Pre-Fetch Read Command Without Address Increment Register */
#define DM9051_MRCMDX			0x70
/* MRCMD - Memory Data Read Command With Address Increment Register */
#define DM9051_MRCMD			0x72
/* MWCMD - Memory Data Write Command With Address Increment Register */
#define DM9051_MWCMD			0x78
/* TXPLL - TX Packet Length Low Byte Register */
#define DM9051_TXPLL			0x7C
/* ISR - Interrupt Status Register */
#define DM9051_ISR			0x7E
/* IMR - Interrupt Mask Register */
#define DM9051_IMR			0x7F

/* 0x00 */
/* FDX - Duplex Mode of the Internal PHY */
#define DM9051_NCR_FDX			BIT(3)
/* RST - Software Reset and Auto-Clear after 10us */
#define DM9051_NCR_RST			BIT(0)

/* 0x01 */
/* SPEED - Speed of Internal PHY */
#define DM9051_NSR_SPEED		BIT(7)
/* LINKST - Link Status of Internal PHY */
#define DM9051_NSR_LINKST		BIT(6)
/* TX2END - TX Packet Index II Complete Status */
#define DM9051_NSR_TX2END		BIT(3)
/* TX1END - TX Packet Index I Complete Status */
#define DM9051_NSR_TX1END		BIT(2)

/* 0x02 */
/* TXREQ - TX Request. Auto-Clear after Sending Completely */
#define DM9051_TCR_TXREQ		BIT(0)

/* 0x05 */
/* DIS_LONG - Discard Long Packet */
#define DM9051_RCR_DIS_LONG		BIT(5)
/* DIS_CRC - Discard CRC Error Packet */
#define DM9051_RCR_DIS_CRC		BIT(4)
/* PRMSC - Promiscuous Mode */
#define DM9051_RCR_PRMSC		BIT(1)
/* RXEN - RX Enable */
#define DM9051_RCR_RXEN			BIT(0)

/* 0x06 */
/* MF - Multicast Frame */
#define DM9051_RSR_MF			BIT(6)

/* 0x0B */
/* EPOS - EEPROM or PHY Operation Select */
#define DM9051_EPCR_EPOS		BIT(3)
/* ERPRR - EEPROM Read or PHY Register Read Command */
#define DM9051_EPCR_ERPRR		BIT(2)
/* ERPRW - EEPROM Write or PHY Register Write Command */
#define DM9051_EPCR_ERPRW		BIT(1)
/* ERRE - EEPROM Access Status or PHY Access Status */
#define DM9051_EPCR_ERRE		BIT(0)

/* 0x0C */
/* PHY Address bit shift */
#define DM9051_EPAR_PHY_ADR_SHIFT	6

/* 0x16 + 7 */
/* Bit 7 = Enable all broadcast packets */
#define DM9051_MAR_7_BCAST_EN		0x80

/* 0x1F */
#define DM9051_GPR_PHY_ON		0x00
#define DM9051_GPR_PHY_OFF		0x01

/* 0x39 */
#define DM9051_INTCR_POL_HIGH		0x00
#define DM9051_INTCR_POL_LOW		0x01

/* 0x7E */
/* LNKCHG - Link Status Change */
#define DM9051_ISR_LNKCHG		BIT(5)
/* PT - Packet Transmitted */
#define DM9051_ISR_PT			BIT(1)
/* PR - Packet Received */
#define DM9051_ISR_PR			BIT(0)

/* 0x7F */
/* PAR - Pointer Auto-Return Mode */
#define DM9051_IMR_PAR			BIT(7)
/* LNKCHGI - Enable Link Status Change Interrupt */
#define DM9051_IMR_LNKCHGI		BIT(5)
/* PTI - Enable Packet Transmitted Interrupt */
#define DM9051_IMR_PTI			BIT(1)
/* PRI - Enable Packet Received Interrupt */
#define DM9051_IMR_PRI			BIT(0)

/* Packet received flag value */
#define DM9051_FLAG_RX_PKT		0x01

/* SPI Read Opcode */
#define DM9051_SPI_RD			0x00
/* SPI Write Opcode */
#define DM9051_SPI_WR			0x80

/* Max time to wait for EEPROM or PHY access completion */
#define DM9051_EPCR_POLL_TIMEOUT	K_MSEC(10)
/* Delay between EPCR status polls */
#define DM9051_EPCR_POLL_INTERVAL	K_USEC(100)
/* Max time to wait for TX completion */
#define DM9051_NSR_POLL_TIMEOUT		K_USEC(20)
/* Delay between NSR status polls */
#define DM9051_NSR_POLL_INTERVAL	K_USEC(1)

struct eth_dm9051_config {
	struct net_eth_mac_config mac_cfg;
	struct gpio_dt_spec gpio_int;
	const struct device *phy_dev;
	struct spi_dt_spec spi;
};

struct eth_dm9051_data {
	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_DM9051_RX_THREAD_STACK_SIZE);
	uint8_t tx_buf[NET_ETH_MAX_FRAME_SIZE];
	uint8_t rx_buf[NET_ETH_MAX_FRAME_SIZE];
	struct gpio_callback gpio_cb;
	struct phy_link_state state;
	struct k_thread rx_thread;
	const struct device *dev;
	struct k_mutex spi_lock;
	struct k_sem int_event;
	struct net_if *iface;
	struct k_sem tx_done;
	uint8_t mac_addr[6];
};

struct eth_dm9051_rxhdr {
	uint8_t flag;
	uint8_t status;
	uint8_t len[2];
};

static int eth_dm9051_spi_read(const struct device *dev, uint8_t reg,
			       uint8_t *data, size_t len)
{
	const struct eth_dm9051_config *config = dev->config;

	uint8_t cmd = reg | DM9051_SPI_RD;
	struct spi_buf txb = {
		.buf = &cmd,
		.len = 1,
	};
	struct spi_buf_set txbs = {
		.buffers = &txb,
		.count = 1,
	};

	struct spi_buf rxb[2] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = data,
			.len = len,
		},
	};
	struct spi_buf_set rxbs = {
		.buffers = rxb,
		.count = ARRAY_SIZE(rxb),
	};

	return spi_transceive_dt(&config->spi, &txbs, &rxbs);
}

static int eth_dm9051_spi_write(const struct device *dev, uint8_t reg,
				uint8_t *data, size_t len)
{
	const struct eth_dm9051_config *config = dev->config;

	uint8_t cmd = reg | DM9051_SPI_WR;
	struct spi_buf txb[2] = {
		{
			.buf = &cmd,
			.len = 1,
		},
		{
			.buf = data,
			.len = len,
		},
	};
	struct spi_buf_set txbs = {
		.buffers = txb,
		.count = ARRAY_SIZE(txb),
	};

	return spi_write_dt(&config->spi, &txbs);
}

static inline int eth_dm9051_spi_read_mem(const struct device *dev, uint8_t reg,
					  uint8_t *data, size_t len)
{
	return eth_dm9051_spi_read(dev, reg, data, len);
}

static inline int eth_dm9051_spi_write_mem(const struct device *dev, uint8_t reg,
					   uint8_t *data, size_t len)
{
	return eth_dm9051_spi_write(dev, reg, data, len);
}

static inline int eth_dm9051_spi_read_reg(const struct device *dev, uint8_t reg, uint8_t *data)
{
	return eth_dm9051_spi_read(dev, reg, data, 1);
}

static inline int eth_dm9051_spi_write_reg(const struct device *dev, uint8_t reg, uint8_t data)
{
	return eth_dm9051_spi_write(dev, reg, &data, 1);
}

static inline int eth_dm9051_spi_read_regs(const struct device *dev, uint8_t reg,
					   uint8_t *data, size_t len)
{
	int ret = 0;

	for (size_t i = 0; i < len; i++) {
		ret = eth_dm9051_spi_read_reg(dev, (uint8_t)(reg + i), &data[i]);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

static inline int eth_dm9051_spi_write_regs(const struct device *dev, uint8_t reg,
					    uint8_t *data, size_t len)
{
	int ret = 0;

	for (size_t i = 0; i < len; i++) {
		ret = eth_dm9051_spi_write_reg(dev, (uint8_t)(reg + i), data[i]);
		if (ret < 0) {
			break;
		}
	}

	return ret;
}

static int eth_dm9051_check_id(const struct device *dev)
{
	uint16_t pid;
	int ret;

	/* Read product ID */
	ret = eth_dm9051_spi_read_regs(dev, DM9051_PIDL, (void *)&pid, 2);
	if (ret < 0) {
		return ret;
	}

	/* Check product ID */
	if (sys_le16_to_cpu(pid) != DM9051_ID) {
		LOG_ERR("%s: Found ID: %04x, Expected ID: %04x\n", dev->name, pid, DM9051_ID);
		return -EIO;
	}

	LOG_INF("%s: Found ID: %04x\n", dev->name, pid);

	return ret;
}

static int eth_dm9051_epcr_poll(const struct device *dev, k_timeout_t timeout)
{
	k_timepoint_t timepoint = sys_timepoint_calc(timeout);
	uint8_t epcr;
	int ret;

	do {
		ret = eth_dm9051_spi_read_reg(dev, DM9051_EPCR, &epcr);
		if (ret || ((epcr & DM9051_EPCR_ERRE) == 0)) {
			return ret;
		}
		k_sleep(DM9051_EPCR_POLL_INTERVAL);
	} while (!sys_timepoint_expired(timepoint));

	return -ETIMEDOUT;
}

static int eth_dm9051_nsr_poll(const struct device *dev, k_timeout_t timeout)
{
	k_timepoint_t timepoint = sys_timepoint_calc(timeout);
	uint8_t nsr;
	int ret;

	do {
		ret = eth_dm9051_spi_read_reg(dev, DM9051_NSR, &nsr);
		if (ret || ((nsr & (DM9051_NSR_TX1END | DM9051_NSR_TX2END)) != 0)) {
			return ret;
		}
		k_sleep(DM9051_NSR_POLL_INTERVAL);
	} while (!sys_timepoint_expired(timepoint));

	return -ETIMEDOUT;
}

static int eth_dm9051_hw_start(const struct device *dev)
{
	const uint8_t imr = DM9051_IMR_PRI | DM9051_IMR_PTI | DM9051_IMR_LNKCHGI | DM9051_IMR_PAR;
	const uint8_t rcr = DM9051_RCR_RXEN | DM9051_RCR_DIS_CRC | DM9051_RCR_DIS_LONG;
	const struct eth_dm9051_config *config = dev->config;
	uint8_t gpr;
	int ret;

	ret = eth_dm9051_spi_read_reg(dev, DM9051_GPR, &gpr);
	if (ret < 0) {
		return ret;
	}

	if (gpr & DM9051_GPR_PHY_OFF) {
		/*
		 * If this bit is updated from ‘1’ to ‘0’, the whole MAC
		 * and PHY Registers can not be accessed within 1 ms.
		 */
		ret = eth_dm9051_spi_write_reg(dev, DM9051_GPR, DM9051_GPR_PHY_ON);
		if (ret < 0) {
			return ret;
		}

		k_msleep(1);
	}

	/* Software Reset and Auto-Clear after 10 us */
	ret = eth_dm9051_spi_write_reg(dev, DM9051_NCR, DM9051_NCR_RST);
	if (ret < 0) {
		return ret;
	}

	k_usleep(10);

	/* Set gpio pin polarity based on gpio dt flags */
	ret = eth_dm9051_spi_write_reg(dev, DM9051_INTCR,
				       (config->gpio_int.dt_flags & GPIO_ACTIVE_LOW) > 0 ?
				       DM9051_INTCR_POL_LOW : DM9051_INTCR_POL_HIGH);
	if (ret < 0) {
		return ret;
	}

	/* Enable all broadcast packets */
	ret = eth_dm9051_spi_write_reg(dev, DM9051_MAR + 7, DM9051_MAR_7_BCAST_EN);
	if (ret < 0) {
		return ret;
	}

	/* Enable RX */
	ret = eth_dm9051_spi_write_reg(dev, DM9051_RCR, rcr);
	if (ret < 0) {
		return ret;
	}

	/* Enable interrupts */
	return eth_dm9051_spi_write_reg(dev, DM9051_IMR, imr);
}

static int eth_dm9051_hw_stop(const struct device *dev)
{
	int ret;

	/* Power off the internal phy */
	ret = eth_dm9051_spi_write_reg(dev, DM9051_GPR, DM9051_GPR_PHY_OFF);
	if (ret < 0) {
		return ret;
	}

	/* Disable RX */
	return eth_dm9051_spi_write_reg(dev, DM9051_RCR, 0);
}

static int eth_dm9051_tx(const struct device *dev, struct net_pkt *pkt)
{
	uint16_t len = (uint16_t)net_pkt_get_len(pkt);
	struct eth_dm9051_data *data = dev->data;
	uint16_t len16;
	int ret;

	/* Read TX data from net_pkt */
	if (net_pkt_read(pkt, data->tx_buf, len)) {
		ret = -EIO;
		goto out_update_errors_tx;
	}

	k_mutex_lock(&data->spi_lock, K_FOREVER);

	ret = eth_dm9051_nsr_poll(dev, DM9051_NSR_POLL_TIMEOUT);
	if (ret < 0) {
		goto out_spi_unlock;
	}

	/* Write TX data to TX SRAM */
	ret = eth_dm9051_spi_write_mem(dev, DM9051_MWCMD, data->tx_buf, len);
	if (ret < 0) {
		goto out_spi_unlock;
	}

	/* Write TX data length */
	len16 = sys_cpu_to_le16(len);
	ret = eth_dm9051_spi_write_regs(dev, DM9051_TXPLL, (void *)&len16, 2);
	if (ret < 0) {
		goto out_spi_unlock;
	}

	/* TX request */
	ret = eth_dm9051_spi_write_reg(dev, DM9051_TCR, DM9051_TCR_TXREQ);
	if (ret < 0) {
		goto out_spi_unlock;
	}

	k_mutex_unlock(&data->spi_lock);

	if (k_sem_take(&data->tx_done, K_MSEC(10))) {
		ret = -EIO;
		goto out_update_errors_tx;
	}

	/* Update ethernet statistics */
	eth_stats_update_bytes_tx(data->iface, len);
	eth_stats_update_pkts_tx(data->iface);
	if (net_eth_is_addr_broadcast(&NET_ETH_HDR(pkt)->dst)) {
		eth_stats_update_broadcast_tx(data->iface);
	} else if (net_eth_is_addr_multicast(&NET_ETH_HDR(pkt)->dst)) {
		eth_stats_update_multicast_tx(data->iface);
	}  else {
		/* Unicast frame */
	}

	return 0;

out_spi_unlock:
	k_mutex_unlock(&data->spi_lock);
out_update_errors_tx:
	eth_stats_update_errors_tx(data->iface);
	return ret;
}

static struct net_pkt *eth_dm9051_recv_pkt(const struct device *dev)
{
	struct eth_dm9051_data *data = dev->data;
	struct eth_dm9051_rxhdr rxhdr;
	struct net_pkt *pkt;
	uint16_t rx_len;
	int ret;

	/* Read RX header */
	ret = eth_dm9051_spi_read_mem(data->dev, DM9051_MRCMD, (void *)&rxhdr,
				      sizeof(rxhdr));
	if (ret < 0) {
		return NULL;
	}

	rx_len = sys_get_le16(rxhdr.len);

	/* Check for RX errors */
	if ((rxhdr.status & ~DM9051_RSR_MF) > 0 || (rx_len > NET_ETH_MAX_FRAME_SIZE)) {
		if ((rxhdr.status & ~DM9051_RSR_MF) > 0) {
			LOG_DBG("%s: RX status error: %02x", dev->name, rxhdr.status);
		}

		if (rx_len > NET_ETH_MAX_FRAME_SIZE) {
			LOG_DBG("%s: RX length too large: %u > %u", dev->name,
				rx_len, NET_ETH_MAX_FRAME_SIZE);
		}

		(void)eth_dm9051_hw_start(dev);
		return NULL;
	}

	/* Alloc RX net_pkt and subtract 4 from RX length to discard CRC */
	pkt = net_pkt_rx_alloc_with_buffer(data->iface, rx_len - 4, NET_AF_UNSPEC, 0,
					   K_MSEC(CONFIG_ETH_DM9051_TIMEOUT));
	if (!pkt) {
		/* Discard received data */
		(void)eth_dm9051_spi_read_mem(dev, DM9051_MRCMD, NULL, rx_len);
		return NULL;
	}

	/* Read RX data from RX SRAM */
	ret = eth_dm9051_spi_read_mem(dev, DM9051_MRCMD, data->rx_buf, rx_len);
	if (ret < 0) {
		goto out_net_pkt_unref;
	}

	/* Write RX data to net_pkt */
	ret = net_pkt_write(pkt, data->rx_buf, rx_len - 4);
	if (ret < 0) {
		goto out_net_pkt_unref;
	}

	return pkt;

out_net_pkt_unref:
	net_pkt_unref(pkt);
	return NULL;
}

static int eth_dm9051_rx(const struct device *dev)
{
	struct eth_dm9051_data *data = dev->data;
	struct net_pkt *pkt;
	uint16_t flag;
	int ret;

	k_mutex_lock(&data->spi_lock, K_FOREVER);

	while (true) {
		/* Read RX flag */
		ret = eth_dm9051_spi_read_mem(data->dev, DM9051_MRCMDX, (void *)&flag, 2);
		if (ret < 0) {
			goto out_update_errors_rx;
		}

		/* Check if RX data is available in RX SRAM */
		if (!(sys_be16_to_cpu(flag) & DM9051_FLAG_RX_PKT)) {
			break;
		}

		/* Get received packet */
		pkt = eth_dm9051_recv_pkt(dev);
		if (!pkt) {
			ret = -EIO;
			goto out_update_errors_rx;
		}

		/* Push the net_pkt in the network stack */
		ret = net_recv_data(data->iface, pkt);
		if (ret < 0) {
			net_pkt_unref(pkt);
			goto out_update_errors_rx;
		}

		/* Update ethernet statistics */
		eth_stats_update_bytes_rx(data->iface, net_pkt_get_len(pkt));
		eth_stats_update_pkts_rx(data->iface);
		if (net_eth_is_addr_broadcast(&NET_ETH_HDR(pkt)->dst)) {
			eth_stats_update_broadcast_rx(data->iface);
		} else if (net_eth_is_addr_multicast(&NET_ETH_HDR(pkt)->dst)) {
			eth_stats_update_multicast_rx(data->iface);
		} else {
			/* Unicast frame */
		}
	}

	goto out_spi_unlock;

out_update_errors_rx:
	eth_stats_update_errors_rx(data->iface);
out_spi_unlock:
	k_mutex_unlock(&data->spi_lock);
	return ret;
}

static void eth_dm9051_update_link_status(const struct device *dev)
{
	struct eth_dm9051_data *data = dev->data;
	enum phy_link_speed speed;
	uint8_t nsr;
	uint8_t ncr;

	if (eth_dm9051_spi_read_reg(dev, DM9051_NSR, &nsr) < 0) {
		return;
	}

	if (eth_dm9051_spi_read_reg(dev, DM9051_NCR, &ncr) < 0) {
		return;
	}

	if ((nsr & DM9051_NSR_LINKST) > 0) {
		if (data->state.is_up != true) {
			LOG_INF("%s: Link up", dev->name);
			data->state.is_up = true;
			net_eth_carrier_on(data->iface);
		}

		if ((nsr & DM9051_NSR_SPEED) > 0) {
			speed = ((ncr & DM9051_NCR_FDX) > 0) ? LINK_FULL_10BASE :
							       LINK_HALF_10BASE;
		} else {
			speed = ((ncr & DM9051_NCR_FDX) > 0) ? LINK_FULL_100BASE :
							       LINK_HALF_100BASE;
		}

		if (data->state.speed != speed) {
			data->state.speed = speed;
			LOG_INF("%s: Link speed %s Mb, %s duplex", dev->name,
				PHY_LINK_IS_SPEED_100M(speed) ? "100" : "10",
				PHY_LINK_IS_FULL_DUPLEX(speed) ? "full" : "half");
		}
	} else {
		if (data->state.is_up != false) {
			LOG_INF("%s: Link down", dev->name);
			data->state.is_up = false;
			data->state.speed = 0;
			net_eth_carrier_off(data->iface);
		}
	}
}

static void eth_dm9051_rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct eth_dm9051_data *data;
	struct device *dev;
	uint8_t isr = 0;

	dev = p1;
	data = dev->data;

	while (true) {
		k_sem_take(&data->int_event, K_FOREVER);

		(void)eth_dm9051_spi_read_reg(dev, DM9051_ISR, &isr);
		(void)eth_dm9051_spi_write_reg(dev, DM9051_ISR, isr);

		if ((isr & DM9051_ISR_PT) > 0) {
			k_sem_give(&data->tx_done);
			LOG_DBG("%s: Packet Transmitted", dev->name);
		}

		if ((isr & DM9051_ISR_PR) > 0) {
			(void)eth_dm9051_rx(dev);
			LOG_DBG("%s: Packet Received", dev->name);
		}

		if ((isr & DM9051_ISR_LNKCHG) > 0) {
			eth_dm9051_update_link_status(dev);
			LOG_DBG("%s: Link changed", dev->name);
		}
	}
}

static void eth_dm9051_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_dm9051_data *data = dev->data;

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	data->iface = iface;

	ethernet_init(iface);

	/* Do not start the interface until PHY link is up */
	net_if_carrier_off(iface);

	k_thread_create(&data->rx_thread, data->rx_thread_stack,
			CONFIG_ETH_DM9051_RX_THREAD_STACK_SIZE,
			eth_dm9051_rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_DM9051_RX_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&data->rx_thread, "eth_dm9051");
}

static enum ethernet_hw_caps eth_dm9051_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
	;
}

static int eth_dm9051_set_config(const struct device *dev,
				 enum ethernet_config_type type,
				 const struct ethernet_config *config)
{
	struct eth_dm9051_data *data = dev->data;
	uint8_t rcr;
	int ret;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));

		ret = eth_dm9051_spi_write_regs(dev, DM9051_PAR, data->mac_addr,
						sizeof(data->mac_addr));
		if (ret < 0) {
			return ret;
		}

		LOG_INF("%s: MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name,
			data->mac_addr[0], data->mac_addr[1],
			data->mac_addr[2], data->mac_addr[3],
			data->mac_addr[4], data->mac_addr[5]);

		return ret;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (!IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			return -ENOTSUP;
		}

		ret = eth_dm9051_spi_read_reg(dev, DM9051_RCR, &rcr);
		if (ret < 0) {
			return ret;
		}

		if (config->promisc_mode == ((rcr & DM9051_RCR_PRMSC) > 0)) {
			return -EALREADY;
		}

		rcr = (rcr & ~DM9051_RCR_PRMSC) | (config->promisc_mode ? DM9051_RCR_PRMSC : 0);
		return eth_dm9051_spi_write_reg(dev, DM9051_RCR, rcr);
	default:
		return -ENOTSUP;
	}
}

static const struct device *eth_dm9051_get_phy(const struct device *dev)
{
	const struct eth_dm9051_config *config = dev->config;

	return config->phy_dev;
}

static const struct ethernet_api eth_dm9051_api = {
	.iface_api.init = eth_dm9051_iface_init,
	.get_capabilities = eth_dm9051_get_capabilities,
	.set_config = eth_dm9051_set_config,
	.start = eth_dm9051_hw_start,
	.stop = eth_dm9051_hw_stop,
	.get_phy = eth_dm9051_get_phy,
	.send = eth_dm9051_tx,
};

static int eth_dm9051_get_link_state(const struct device *dev,
				     struct phy_link_state *state)
{
	struct eth_dm9051_data *const data = dev->data;

	state->speed = data->state.speed;
	state->is_up = data->state.is_up;

	return 0;
}

static int eth_dm9051_phy_read(const struct device *dev, uint16_t reg_addr, uint32_t *data)
{
	uint16_t phy_data;
	int ret;

	ret = eth_dm9051_spi_write_reg(dev, DM9051_EPAR, (uint8_t)(reg_addr & 0x1F) |
				       (DM9051_PHY_ADDR << DM9051_EPAR_PHY_ADR_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = eth_dm9051_spi_write_reg(dev, DM9051_EPCR, DM9051_EPCR_ERPRR | DM9051_EPCR_EPOS);
	if (ret < 0) {
		return ret;
	}

	ret = eth_dm9051_epcr_poll(dev, DM9051_EPCR_POLL_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	ret = eth_dm9051_spi_read_regs(dev, DM9051_EPDRL, (void *)&phy_data, 2);
	if (ret) {
		return ret;
	}

	*data = sys_le16_to_cpu(phy_data);

	return 0;
}

static int eth_dm9051_phy_write(const struct device *dev, uint16_t reg_addr, uint32_t data)
{
	uint16_t phy_data = sys_cpu_to_le16((uint16_t)data);
	int ret;

	ret = eth_dm9051_spi_write_reg(dev, DM9051_EPAR, (uint8_t)(reg_addr & 0x1F) |
				       (DM9051_PHY_ADDR << DM9051_EPAR_PHY_ADR_SHIFT));
	if (ret < 0) {
		return ret;
	}

	ret = eth_dm9051_spi_write_regs(dev, DM9051_EPDRL, (void *)&phy_data, 2);
	if (ret < 0) {
		return ret;
	}

	ret = eth_dm9051_spi_write_reg(dev, DM9051_EPCR, DM9051_EPCR_ERPRW | DM9051_EPCR_EPOS);
	if (ret < 0) {
		return ret;
	}

	return eth_dm9051_epcr_poll(dev, K_MSEC(10));
}

static DEVICE_API(ethphy, ethphy_dm9051_api) = {
	.get_link = eth_dm9051_get_link_state,
	.read = eth_dm9051_phy_read,
	.write = eth_dm9051_phy_write,
};

static void eth_dm9051_gpio_callback(const struct device *dev,
				     struct gpio_callback *cb,
				     uint32_t pins)
{
	struct eth_dm9051_data *data = CONTAINER_OF(cb, struct eth_dm9051_data, gpio_cb);

	k_sem_give(&data->int_event);
}

static int eth_dm9051_set_mac_addr(const struct device *dev)
{
	const struct eth_dm9051_config *config = dev->config;
	struct eth_dm9051_data *data = dev->data;
	int ret;

	/* Set MAC address if it is statically defined or if random is set in device tree */
	ret = net_eth_mac_load(&config->mac_cfg, data->mac_addr);
	if (ret == 0) {
		/* Write the MAC address into device */
		return eth_dm9051_spi_write_regs(dev, DM9051_PAR, data->mac_addr,
						 sizeof(data->mac_addr));
	}

	/* Read the MAC address from DM9051_PAR registers */
	ret = eth_dm9051_spi_read_regs(dev, DM9051_PAR, data->mac_addr, sizeof(data->mac_addr));
	if (ret < 0) {
		return ret;
	}

	/* Check if MAC address is not 00:00:00:00:00:00 */
	if (UNALIGNED_GET((uint32_t *)(data->mac_addr + 0)) == 0x0 &&
	    UNALIGNED_GET((uint16_t *)(data->mac_addr + 4)) == 0x0) {
		return -EINVAL;
	}

	/* Check if MAC address if not multicast address */
	if ((data->mac_addr[0] & 0x1) > 0) {
		return -EINVAL;
	}

	return 0;
}

static int eth_dm9051_init(const struct device *dev)
{
	const struct eth_dm9051_config *config = dev->config;
	struct eth_dm9051_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_sem_init(&data->int_event, 0, UINT_MAX);
	k_sem_init(&data->tx_done, 1, UINT_MAX);
	k_mutex_init(&data->spi_lock);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("%s: SPI master port %s is not ready", dev->name, config->spi.bus->name);
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(&config->gpio_int)) {
		LOG_ERR("%s: GPIO port %s is not ready", dev->name, config->gpio_int.port->name);
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("%s: Unable to configure GPIO pin (%d)", dev->name, ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, eth_dm9051_gpio_callback, BIT(config->gpio_int.pin));

	ret = gpio_add_callback(config->gpio_int.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("%s: Unable to add GPIO callback (%d)", dev->name, ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: Unable to enable GPIO interrupt (%d)", dev->name, ret);
		return ret;
	}

	ret = eth_dm9051_check_id(dev);
	if (ret < 0) {
		return ret;
	}

	ret = eth_dm9051_set_mac_addr(dev);
	if (ret < 0) {
		LOG_WRN("%s: Unable to set MAC address", dev->name);
	}

	LOG_INF("%s: Device initialized", dev->name);

	return 0;
}

#define ETH_DM9051_INIT(inst)									\
	DEVICE_DECLARE(eth_dm9051_phy_##inst);							\
												\
	static struct eth_dm9051_data eth_dm9051_data_##inst;					\
												\
	static const struct eth_dm9051_config eth_dm9051_config_##inst = {			\
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),				\
		.gpio_int = GPIO_DT_SPEC_INST_GET(inst, int_gpios),				\
		.phy_dev = DEVICE_GET(eth_dm9051_phy_##inst),					\
		.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8)),				\
	};											\
												\
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, eth_dm9051_init, NULL, &eth_dm9051_data_##inst,	\
				      &eth_dm9051_config_##inst, CONFIG_ETH_INIT_PRIORITY,	\
				      &eth_dm9051_api, NET_ETH_MTU);				\
												\
	DEVICE_DEFINE(eth_dm9051_phy_##inst, DEVICE_DT_NAME(DT_DRV_INST(inst)) "_phy",		\
		      NULL, NULL, &eth_dm9051_data_##inst, &eth_dm9051_config_##inst,		\
		      POST_KERNEL, CONFIG_ETH_INIT_PRIORITY, &ethphy_dm9051_api);

DT_INST_FOREACH_STATUS_OKAY(ETH_DM9051_INIT)
