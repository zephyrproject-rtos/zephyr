/*
 * Copyright (c) 2017-2018 ARM Limited
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SMSC911x/SMSC9220 driver. Partly based on mbedOS driver. */

#define LOG_MODULE_NAME eth_smsc911x
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <soc.h>
#include <device.h>
#include <errno.h>
#include <init.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/sys_io.h>
#include <net/ethernet.h>
#include "ethernet/eth_stats.h"

#ifdef CONFIG_SHARED_IRQ
#include <shared_irq.h>
#endif

#include "eth_smsc911x_priv.h"

#define RESET_TIMEOUT     K_MSEC(10)
#define PHY_RESET_TIMEOUT K_MSEC(100)
#define REG_WRITE_TIMEOUT K_MSEC(50)

/* Controller has only one PHY with address 1 */
#define PHY_ADDR 1

struct eth_context {
	struct net_if *iface;
	u8_t mac[6];
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

/* SMSC911x helper functions */

static int smsc_mac_regread(u8_t reg, u32_t *val)
{
	u32_t cmd = MAC_CSR_CMD_BUSY | MAC_CSR_CMD_READ | reg;

	SMSC9220->MAC_CSR_CMD = cmd;

	while ((SMSC9220->MAC_CSR_CMD & MAC_CSR_CMD_BUSY) != 0) {
	}

	*val = SMSC9220->MAC_CSR_DATA;

	return 0;
}

static int smsc_mac_regwrite(u8_t reg, u32_t val)
{
	u32_t cmd = MAC_CSR_CMD_BUSY | MAC_CSR_CMD_WRITE | reg;

	SMSC9220->MAC_CSR_DATA = val;

	SMSC9220->MAC_CSR_CMD = cmd;

	while ((SMSC9220->MAC_CSR_CMD & MAC_CSR_CMD_BUSY) != 0) {
	}

	return 0;
}

int smsc_phy_regread(u8_t regoffset, u32_t *data)
{
	u32_t val = 0U;
	u32_t phycmd = 0U;
	unsigned int time_out = REG_WRITE_TIMEOUT;

	if (smsc_mac_regread(SMSC9220_MAC_MII_ACC, &val) < 0) {
		return -1;
	}

	if (val & MAC_MII_ACC_MIIBZY) {
		*data = 0U;
		return -EBUSY;
	}

	phycmd = 0U;
	phycmd |= PHY_ADDR << 11;
	phycmd |= (regoffset & 0x1F) << 6;
	phycmd |= MAC_MII_ACC_READ;
	phycmd |= MAC_MII_ACC_MIIBZY; /* Operation start */

	if (smsc_mac_regwrite(SMSC9220_MAC_MII_ACC, phycmd)) {
		return -1;
	}

	val = 0U;
	do {
		k_sleep(K_MSEC(1));
		time_out--;
		if (smsc_mac_regread(SMSC9220_MAC_MII_ACC, &val)) {
			return -1;
		}
	} while (time_out != 0U && (val & MAC_MII_ACC_MIIBZY));

	if (time_out == 0U) {
		return -ETIMEDOUT;
	}

	if (smsc_mac_regread(SMSC9220_MAC_MII_DATA, data) < 0) {
		return -1;
	}

	return 0;
}

int smsc_phy_regwrite(u8_t regoffset, u32_t data)
{
	u32_t val = 0U;
	u32_t phycmd = 0U;
	unsigned int time_out = REG_WRITE_TIMEOUT;

	if (smsc_mac_regread(SMSC9220_MAC_MII_ACC, &val) < 0) {
		return -1;
	}

	if (val & MAC_MII_ACC_MIIBZY) {
		return -EBUSY;
	}

	if (smsc_mac_regwrite(SMSC9220_MAC_MII_DATA, data & 0xFFFF) < 0) {
		return -1;
	}

	phycmd |= PHY_ADDR << 11;
	phycmd |= (regoffset & 0x1F) << 6;
	phycmd |= MAC_MII_ACC_WRITE;
	phycmd |= MAC_MII_ACC_MIIBZY; /* Operation start */

	if (smsc_mac_regwrite(SMSC9220_MAC_MII_ACC, phycmd) < 0) {
		return -1;
	}

	do {
		k_sleep(K_MSEC(1));
		time_out--;
		if (smsc_mac_regread(SMSC9220_MAC_MII_ACC, &phycmd)) {
			return -1;
		}
	} while (time_out != 0U && (phycmd & MAC_MII_ACC_MIIBZY));

	if (time_out == 0U) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int smsc_read_mac_address(u8_t *mac)
{
	u32_t tmp;
	int res;

	res = smsc_mac_regread(SMSC9220_MAC_ADDRL, &tmp);
	if (res < 0) {
		return res;
	}

	mac[0] = (u8_t)(tmp >> 0);
	mac[1] = (u8_t)(tmp >> 8);
	mac[2] = (u8_t)(tmp >> 16);
	mac[3] = (u8_t)(tmp >> 24);

	res = smsc_mac_regread(SMSC9220_MAC_ADDRH, &tmp);
	if (res < 0) {
		return res;
	}

	mac[4] = (u8_t)(tmp >> 0);
	mac[5] = (u8_t)(tmp >> 8);

	return 0;
}

static int smsc_check_id(void)
{
	u32_t id = SMSC9220->ID_REV;

	/* If bottom and top halves of the word are the same,
	 * the hardware is (likely) not present.
	 */
	if (((id >> 16) & 0xFFFF) == (id & 0xFFFF)) {
		return -1;
	}

	switch (((id >> 16) & 0xFFFF)) {
	case 0x9220: /* SMSC9220 on MPS2 */
	case 0x0118: /* SMS9118 as emulated by QEMU */
		break;

	default:
		return -1;
	}

	return 0;
}

static int smsc_soft_reset(void)
{
	unsigned int time_out = RESET_TIMEOUT;

	SMSC9220->HW_CFG |= HW_CFG_SRST;

	do {
		k_sleep(K_MSEC(1));
		time_out--;
	} while (time_out != 0U && (SMSC9220->HW_CFG & HW_CFG_SRST));

	if (time_out == 0U) {
		return -1;
	}

	return 0;
}

void smsc_set_txfifo(unsigned int val)
{
	/* 2kb minimum, 14kb maximum */
	if (val >= 2U && val <= 14U) {
		SMSC9220->HW_CFG = val << 16;
	}
}

void smsc_init_irqs(void)
{
	SMSC9220->INT_EN = 0;
	/* Clear all interrupts */
	SMSC9220->INT_STS = 0xFFFFFFFF;
	/* Polarity config which works with QEMU */
	/* IRQ deassertion at 220 usecs and master IRQ enable */
	SMSC9220->IRQ_CFG = 0x22000111;
}

static int smsc_check_phy(void)
{
	u32_t phyid1, phyid2;

	if (smsc_phy_regread(SMSC9220_PHY_ID1, &phyid1)) {
		return -1;
	}

	if (smsc_phy_regread(SMSC9220_PHY_ID2, &phyid2)) {
		return -1;
	}

	return ((phyid1 == 0xFFFF && phyid2 == 0xFFFF) ||
	    (phyid1 == 0x0 && phyid2 == 0x0));
}

int smsc_reset_phy(void)
{
	u32_t val;

	if (smsc_phy_regread(SMSC9220_PHY_BCONTROL, &val)) {
		return -1;
	}

	val |= 1 << 15;

	if (smsc_phy_regwrite(SMSC9220_PHY_BCONTROL, val)) {
		return -1;
	}

	return 0;
}

/**
 * Advertise all speeds and pause capabilities
 */
void smsc_advertise_caps(void)
{
	u32_t aneg_adv = 0U;

	smsc_phy_regread(SMSC9220_PHY_ANEG_ADV, &aneg_adv);
	aneg_adv |= 0xDE0;

	smsc_phy_regwrite(SMSC9220_PHY_ANEG_ADV, aneg_adv);
	smsc_phy_regread(SMSC9220_PHY_ANEG_ADV, &aneg_adv);
}

void smsc_establish_link(void)
{
	u32_t bcr = 0U;
	u32_t hw_cfg = 0U;

	smsc_phy_regread(SMSC9220_PHY_BCONTROL, &bcr);
	bcr |= (1 << 12) | (1 << 9);
	smsc_phy_regwrite(SMSC9220_PHY_BCONTROL, bcr);
	smsc_phy_regread(SMSC9220_PHY_BCONTROL, &bcr);

	hw_cfg = SMSC9220->HW_CFG;
	hw_cfg &= 0xF0000;
	hw_cfg |= (1 << 20);
	SMSC9220->HW_CFG = hw_cfg;
}

static inline void smsc_enable_xmit(void)
{
	SMSC9220->TX_CFG = 0x2 /*TX_CFG_TX_ON*/;
}

void smsc_enable_mac_xmit(void)
{
	u32_t mac_cr = 0U;

	smsc_mac_regread(SMSC9220_MAC_CR, &mac_cr);

	mac_cr |= (1 << 3);     /* xmit enable */
	mac_cr |= (1 << 28);    /* Heartbeat disable */

	smsc_mac_regwrite(SMSC9220_MAC_CR, mac_cr);
}

void smsc_enable_mac_recv(void)
{
	u32_t mac_cr = 0U;

	smsc_mac_regread(SMSC9220_MAC_CR, &mac_cr);
	mac_cr |= (1 << 2);     /* Recv enable */
	smsc_mac_regwrite(SMSC9220_MAC_CR, mac_cr);
}

int smsc_init(void)
{
	unsigned int phyreset = 0U;

	if (smsc_check_id() < 0) {
		return -1;
	}

	if (smsc_soft_reset() < 0) {
		return -1;
	}

	smsc_set_txfifo(5);

	/* Sets automatic flow control thresholds, and backpressure */
	/* threshold to defaults specified. */
	SMSC9220->AFC_CFG = 0x006E3740;

	/* May need to initialize EEPROM/read MAC from it on real HW. */

	/* Configure GPIOs as LED outputs. */
	SMSC9220->GPIO_CFG = 0x70070000;

	smsc_init_irqs();

	/* Configure MAC addresses here if needed. */

	if (smsc_check_phy() < 0) {
		return -1;
	}

	if (smsc_reset_phy() < 0) {
		return -1;
	}

	k_sleep(PHY_RESET_TIMEOUT);
	/* Checking whether phy reset completed successfully.*/
	if (smsc_phy_regread(SMSC9220_PHY_BCONTROL, &phyreset)) {
		return 1;
	}

	if (phyreset & (1 << 15)) {
		return 1;
	}

	smsc_advertise_caps();
	/* bit [12] of BCONTROL seems self-clearing. */
	/* Although it's not so in the manual. */
	smsc_establish_link();

	/* Interrupt threshold */
	SMSC9220->FIFO_INT = 0xFF000000;

	smsc_enable_mac_xmit();
	smsc_enable_xmit();
	SMSC9220->RX_CFG = 0;
	smsc_enable_mac_recv();

	/* Rx status FIFO level irq threshold */
	SMSC9220->FIFO_INT &= ~(0xFF);  /* Clear 2 bottom nibbles */

	/* This sleep is compulsory otherwise txmit/receive will fail. */
	k_sleep(K_MSEC(2000));

	return 0;
}

/* Driver functions */

static enum ethernet_hw_caps eth_smsc911x_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *get_stats(struct device *dev)
{
	struct eth_context *context = dev->driver_data;

	return &context->stats;
}
#endif

static void eth_initialize(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->driver_data;

	LOG_DBG("eth_initialize");

	smsc_read_mac_address(context->mac);

	SMSC9220->INT_EN |= BIT(SMSC9220_INTERRUPT_RXSTATUS_FIFO_LEVEL);

	net_if_set_link_addr(iface, context->mac, sizeof(context->mac),
			     NET_LINK_ETHERNET);

	context->iface = iface;

	ethernet_init(iface);
}

static int smsc_write_tx_fifo(const u8_t *buf, u32_t len, bool is_last)
{
	u32_t *buf32;

	__ASSERT_NO_MSG(((uintptr_t)buf & 3) == 0);

	if (is_last) {
		/* Last fragment may be not full */
		len = (len + 3) & ~3;
	}

	if ((len & 3) != 0U || len == 0U) {
		LOG_ERR("Chunk size not aligned: %u", len);
		return -1;
	}

	buf32 = (u32_t *)buf;
	len /= 4U;
	do {
		SMSC9220->TX_DATA_PORT = *buf32++;
	} while (--len);

	return 0;
}

static int eth_tx(struct device *dev, struct net_pkt *pkt)
{
	u16_t total_len = net_pkt_get_len(pkt);
	static u8_t tx_buf[NET_ETH_MAX_FRAME_SIZE] __aligned(4);
	u32_t txcmd_a, txcmd_b;
	u32_t tx_stat;
	int res;

	txcmd_a = (1/*is_first_segment*/ << 13) | (1/*is_last_segment*/ << 12)
		  | total_len;
	/* Use len as a tag */
	txcmd_b = total_len << 16 | total_len;
	SMSC9220->TX_DATA_PORT = txcmd_a;
	SMSC9220->TX_DATA_PORT = txcmd_b;

	if (net_pkt_read(pkt, tx_buf, total_len)) {
		goto error;
	}

	res = smsc_write_tx_fifo(tx_buf, total_len, true);
	if (res < 0) {
		goto error;
	}

	tx_stat = SMSC9220->TX_STAT_PORT;
	LOG_DBG("TX_STAT: %x", tx_stat);

	return 0;

error:
	LOG_ERR("Writing pkt to FIFO failed");
	return -1;
}

static const struct ethernet_api api_funcs = {
	.iface_api.init = eth_initialize,

	.get_capabilities = eth_smsc911x_get_capabilities,
	.send = eth_tx,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = get_stats,
#endif
};

static void smsc_discard_pkt(void)
{
	/* TODO: */
	/* Datasheet p.43: */
	/* When performing a fast-forward, there must be at least 4 DWORDs
	 * of data in the RX data FIFO for the packet being discarded. For
	 * less than 4 DWORDs do not use RX_FFWD. In this case data must be
	 * read from the RX data FIFO and discarded using standard PIO read
	 * operations.
	 */
	SMSC9220->RX_DP_CTRL = RX_DP_CTRL_RX_FFWD;
}

static inline void smsc_wait_discard_pkt(void)
{
	while ((SMSC9220->RX_DP_CTRL & RX_DP_CTRL_RX_FFWD) != 0) {
	}
}

static int smsc_read_rx_fifo(struct net_pkt *pkt, u32_t len)
{
	u32_t buf32;

	__ASSERT_NO_MSG((len & 3) == 0U && len >= 4U);

	len /= 4U;

	do {
		buf32 = SMSC9220->RX_DATA_PORT;

		if (net_pkt_write(pkt, &buf32, sizeof(u32_t))) {
			return -1;
		}
	} while (--len);

	return 0;
}

static struct net_pkt *smsc_recv_pkt(struct device *dev, u32_t pkt_size)
{
	struct eth_context *context = dev->driver_data;
	struct net_pkt *pkt;
	u32_t rem_size;

	/* Round up to next DWORD size */
	rem_size = (pkt_size + 3) & ~3;
	/* Don't account for FCS when filling net pkt */
	rem_size -= 4U;

	pkt = net_pkt_rx_alloc_with_buffer(context->iface, rem_size,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Failed to obtain RX buffer");
		smsc_discard_pkt();
		return NULL;
	}

	if (smsc_read_rx_fifo(pkt, rem_size) < 0) {
		smsc_discard_pkt();
		net_pkt_unref(pkt);
		return NULL;
	}

	/* Discard FCS */
	{
		u32_t __unused dummy = SMSC9220->RX_DATA_PORT;
	}

	/* Adjust len of the last buf down for DWORD alignment */
	if (pkt_size & 3) {
		net_pkt_update_length(pkt, net_pkt_get_len(pkt) -
				      (4 - (pkt_size & 3)));
	}

	return pkt;
}

static void eth_smsc911x_isr(struct device *dev)
{
	u32_t int_status = SMSC9220->INT_STS;
	struct eth_context *context = dev->driver_data;

	LOG_DBG("%s: INT_STS=%x INT_EN=%x", __func__,
		int_status, SMSC9220->INT_EN);

	if (int_status & BIT(SMSC9220_INTERRUPT_RXSTATUS_FIFO_LEVEL)) {
		struct net_pkt *pkt;
		u32_t pkt_size, val;
		u32_t rx_stat;

		val = SMSC9220->RX_FIFO_INF;
		u32_t pkt_pending = BFIELD(val, RX_FIFO_INF_RXSUSED);

		LOG_DBG("in RX FIFO: pkts: %u, bytes: %u",
			pkt_pending,
			BFIELD(val, RX_FIFO_INF_RXDUSED));

		/* Ack rxstatus_fifo_level only when no packets pending. The
		 * idea is to serve 1 packet per interrupt (e.g. to allow
		 * higher priority interrupts to fire) by keeping interrupt
		 * pending for as long as there're packets in FIFO. And when
		 * there's none, finally acknowledge it.
		 */
		if (pkt_pending == 0U) {
			goto done;
		}

		int_status &= ~BIT(SMSC9220_INTERRUPT_RXSTATUS_FIFO_LEVEL);

		/* Make sure that any previously started discard op is
		 * finished.
		 */
		smsc_wait_discard_pkt();

		rx_stat = SMSC9220->RX_STAT_PORT;
		pkt_size = BFIELD(rx_stat, RX_STAT_PORT_PKT_LEN);
		LOG_DBG("pkt sz: %u", pkt_size);

		pkt = smsc_recv_pkt(dev, pkt_size);

		LOG_DBG("out RX FIFO: pkts: %u, bytes: %u",
			SMSC9220_BFIELD(RX_FIFO_INF, RXSUSED),
			SMSC9220_BFIELD(RX_FIFO_INF, RXDUSED));

		if (pkt != NULL) {
			int res = net_recv_data(context->iface, pkt);

			if (res < 0) {
				LOG_ERR("net_recv_data: %d", res);
				net_pkt_unref(pkt);
			}
		}
	}

done:
	/* Ack pending interrupts */
	SMSC9220->INT_STS = int_status;

}

/* Bindings to the platform */

static struct device DEVICE_NAME_GET(eth_smsc911x_0);

int eth_init(struct device *dev)
{
	IRQ_CONNECT(DT_INST_0_SMSC_LAN9220_IRQ_0,
		    DT_INST_0_SMSC_LAN9220_IRQ_0_PRIORITY,
		    eth_smsc911x_isr, DEVICE_GET(eth_smsc911x_0), 0);

	int ret = smsc_init();

	if (ret != 0) {
		LOG_ERR("smsc911x failed to initialize");
		return -ENODEV;
	}

	irq_enable(DT_INST_0_SMSC_LAN9220_IRQ_0);

	return ret;
}

static struct eth_context eth_0_context;

ETH_NET_DEVICE_INIT(eth_smsc911x_0, "smsc911x_0",
		eth_init, &eth_0_context,
		NULL /*&eth_config_0*/, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		NET_ETH_MTU /*MTU*/);
