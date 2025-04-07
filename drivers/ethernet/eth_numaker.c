/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_ethernet

#include <zephyr/kernel.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include "eth_numaker_priv.h"
#include "ethernet/eth_stats.h"
#include <soc.h>
#include <NuMicro.h>
#include <synopGMAC_network_interface.h>

#ifdef CONFIG_SOC_M467
#include <m460_eth.h>
#else
#include <numaker_eth.h>
#endif

LOG_MODULE_REGISTER(eth_numaker, CONFIG_ETHERNET_LOG_LEVEL);

/* Device EMAC Interface port */
#define NUMAKER_GMAC_INTF  0

#define NUMAKER_MASK_32    (0xFFFFFFFFU)
#define NUMAKER_MII_CONFIG (ADVERTISE_CSMA | ADVERTISE_10HALF | ADVERTISE_10FULL | \
							ADVERTISE_100HALF | ADVERTISE_100FULL)
#define NUMAKER_MII_LINKED (BMSR_ANEGCOMPLETE | BMSR_LSTATUS)

extern synopGMACdevice GMACdev[GMAC_CNT];

#ifdef CONFIG_NOCACHE_MEMORY
DmaDesc tx_desc[GMAC_CNT][TRANSMIT_DESC_SIZE] __nocache __aligned(64);
DmaDesc rx_desc[GMAC_CNT][RECEIVE_DESC_SIZE] __nocache __aligned(64);
struct sk_buff tx_buf[GMAC_CNT][TRANSMIT_DESC_SIZE] __nocache __aligned(64);
struct sk_buff rx_buf[GMAC_CNT][RECEIVE_DESC_SIZE] __nocache __aligned(64);
#else
extern struct sk_buff tx_buf[GMAC_CNT][TRANSMIT_DESC_SIZE];
extern struct sk_buff rx_buf[GMAC_CNT][RECEIVE_DESC_SIZE];
#endif

static uint32_t eth_phy_addr;

/* Device config */
struct eth_numaker_config {
	uint32_t gmac_base;
	const struct reset_dt_spec reset;
	uint32_t phy_addr;
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pincfg;
};

/* Driver context/data */
struct eth_numaker_data {
	synopGMACdevice *gmacdev;
	struct net_if *iface;
	uint8_t mac_addr[NU_HWADDR_SIZE];
	struct k_mutex tx_frame_buf_mutex;
	struct k_spinlock rx_frame_buf_lock;
};

/* Delay execution for given amount of ticks for SDK-HAL */
void plat_delay(uint32_t delay)
{
	uint32_t us_cnt = k_ticks_to_us_floor32((uint64_t)delay);

	k_busy_wait(us_cnt);
}

static void mdio_write(synopGMACdevice *gmacdev, uint32_t addr, uint32_t reg, int data)
{
	synopGMAC_write_phy_reg((u32 *)gmacdev->MacBase, addr, reg, data);
}

static int mdio_read(synopGMACdevice *gmacdev, uint32_t addr, uint32_t reg)
{
	uint16_t data;

	synopGMAC_read_phy_reg((u32 *)gmacdev->MacBase, addr, reg, &data);
	return data;
}

static int numaker_eth_link_ok(synopGMACdevice *gmacdev)
{
	/* first, a dummy read to latch */
	mdio_read(gmacdev, eth_phy_addr, MII_BMSR);
	if (mdio_read(gmacdev, eth_phy_addr, MII_BMSR) & BMSR_LSTATUS) {
		return 1;
	}
	return 0;
}

static int reset_phy(synopGMACdevice *gmacdev)
{
	uint16_t reg;
	uint32_t delay_us;
	bool ret;

	mdio_write(gmacdev, eth_phy_addr, MII_BMCR, BMCR_RESET);

	delay_us = 200000U;
	ret = WAIT_FOR(!(mdio_read(gmacdev, eth_phy_addr, MII_BMCR) & BMCR_RESET),
					delay_us, k_msleep(1));
	if (ret == false) {
		LOG_DBG("Reset phy failed");
		return -EIO;
	}

	LOG_INF("PHY ID 1:0x%x", mdio_read(gmacdev, eth_phy_addr, MII_PHYSID1));
	LOG_INF("PHY ID 2:0x%x", mdio_read(gmacdev, eth_phy_addr, MII_PHYSID2));
	delay_us = 3000000U;
	ret = WAIT_FOR(numaker_eth_link_ok(gmacdev), delay_us, k_msleep(1));
	if (ret) {
		gmacdev->LinkState = LINKUP;
		LOG_DBG("Link Up");
	} else {
		gmacdev->LinkState = LINKDOWN;
		LOG_DBG("Link Down");
		return -EIO;
	}

	mdio_write(gmacdev, eth_phy_addr, MII_ADVERTISE, NUMAKER_MII_CONFIG);
	reg = mdio_read(gmacdev, eth_phy_addr, MII_BMCR);
	mdio_write(gmacdev, eth_phy_addr, MII_BMCR, reg | BMCR_ANRESTART);
	delay_us = 3000000U;
	ret = WAIT_FOR((mdio_read(gmacdev, eth_phy_addr, MII_BMSR) &
					NUMAKER_MII_LINKED) == NUMAKER_MII_LINKED,
					delay_us, k_msleep(1));
	if (ret == false) {
		LOG_DBG("AN failed. Set to 100 FULL");
		synopGMAC_set_full_duplex(gmacdev);
		synopGMAC_set_mode(NUMAKER_GMAC_INTF, 1); /* Set mode 1: 100Mbps; 2: 10Mbps */
		return -EIO;
	}

	reg = mdio_read(gmacdev, eth_phy_addr, MII_LPA);
	if (reg & ADVERTISE_100FULL) {
		LOG_DBG("100 full");
		gmacdev->DuplexMode = FULLDUPLEX;
		gmacdev->Speed = SPEED100;
		synopGMAC_set_full_duplex(gmacdev);
		synopGMAC_set_mode(NUMAKER_GMAC_INTF, 1); /* Set mode 1: 100Mbps; 2: 10Mbps */
	} else if (reg & ADVERTISE_100HALF) {
		LOG_DBG("100 half");
		gmacdev->DuplexMode = HALFDUPLEX;
		gmacdev->Speed = SPEED100;
		synopGMAC_set_half_duplex(gmacdev);
		synopGMAC_set_mode(NUMAKER_GMAC_INTF, 1); /* Set mode 1: 100Mbps; 2: 10Mbps */
	} else if (reg & ADVERTISE_10FULL) {
		LOG_DBG("10 full");
		gmacdev->DuplexMode = FULLDUPLEX;
		gmacdev->Speed = SPEED10;
		synopGMAC_set_full_duplex(gmacdev);
		synopGMAC_set_mode(NUMAKER_GMAC_INTF, 2); /* Set mode 1: 100Mbps; 2: 10Mbps */
	} else {
		LOG_DBG("10 half");
		gmacdev->DuplexMode = HALFDUPLEX;
		gmacdev->Speed = SPEED10;
		synopGMAC_set_half_duplex(gmacdev);
		synopGMAC_set_mode(NUMAKER_GMAC_INTF, 2); /* Set mode 1: 100Mbps; 2: 10Mbps */
	}

	return 0;
}

static void m_numaker_read_mac_addr(char *mac)
{
#if DT_INST_PROP(0, zephyr_random_mac_address)
	gen_random_mac(mac, NUMAKER_OUI_B0, NUMAKER_OUI_B1, NUMAKER_OUI_B2);
#else
	uint32_t uid1;
	uint32_t word0;
	/*
	 * we only want bottom 16 bits of word1 (MAC bits 32-47)
	 * and bit 9 forced to 1, bit 8 forced to 0
	 * Locally administered MAC, reduced conflicts
	 * http://en.wikipedia.org/wiki/MAC_address
	 */
	uint32_t word1;

	/* Generate a semi-unique MAC address from the UUID */
	SYS_UnlockReg();
	/* Enable FMC ISP function */
	FMC_Open();
	uid1 = FMC_ReadUID(1);
	word1 = (uid1 & 0x003FFFFF) | ((uid1 & 0x030000) << 6) >> 8;
	word0 = ((FMC_ReadUID(0) >> 4) << 20) | ((uid1 & 0xFF) << 12) |
		(FMC_ReadUID(2) & 0xFFF);
	/* Disable FMC ISP function */
	FMC_Close();
	/* Lock protected registers */
	SYS_LockReg();

	word1 |= 0x00000200;
	word1 &= 0x0000FEFF;

	mac[0] = (word1 & 0x0000ff00) >> 8;
	mac[1] = (word1 & 0x000000ff);
	mac[2] = (word0 & 0xff000000) >> 24;
	mac[3] = (word0 & 0x00ff0000) >> 16;
	mac[4] = (word0 & 0x0000ff00) >> 8;
	mac[5] = (word0 & 0x000000ff);
#endif
	LOG_INF("mac address %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
	       mac[4], mac[5]);
}

static void m_numaker_gmacdev_enable(synopGMACdevice *gmacdev)
{

	synopGMAC_clear_interrupt(gmacdev);

	/* Enable INT & TX/RX */
	synopGMAC_enable_interrupt(gmacdev, DmaIntEnable);
	synopGMAC_enable_dma_rx(gmacdev);
	synopGMAC_enable_dma_tx(gmacdev);

	synopGMAC_tx_enable(gmacdev);
	synopGMAC_rx_enable(gmacdev);
}

static int m_numaker_gmacdev_init(synopGMACdevice *gmacdev, uint8_t *mac_addr, uint32_t gmac_base)
{
	int status;
	int i;
	uint32_t offload_needed = 0;
	struct sk_buff *skb;

	LOG_DBG("");

	/*Attach the device to MAC struct This will configure all the required base
	 * addresses such as Mac base, configuration base, phy base address(out of 32
	 * possible phys )
	 */
	synopGMAC_attach(gmacdev, gmac_base + MACBASE, gmac_base + DMABASE, DEFAULT_PHY_BASE);
	synopGMAC_disable_interrupt_all(gmacdev);

	/* Reset MAC */
	synopGMAC_reset(gmacdev);
	gmacdev->Intf = NUMAKER_GMAC_INTF;
	synopGMAC_read_version(gmacdev);

	/* Check for Phy initialization */
	synopGMAC_set_mdc_clk_div(gmacdev, GmiiCsrClk5);
	gmacdev->ClockDivMdc = synopGMAC_get_mdc_clk_div(gmacdev);

	/* Reset PHY */
	status = reset_phy(gmacdev);

	/* Set up the tx and rx descriptor queue/ring */
	synopGMAC_setup_tx_desc_queue(gmacdev, TRANSMIT_DESC_SIZE, RINGMODE);
	synopGMAC_init_tx_desc_base(gmacdev);

	synopGMAC_setup_rx_desc_queue(gmacdev, RECEIVE_DESC_SIZE, RINGMODE);
	synopGMAC_init_rx_desc_base(gmacdev);

	/* Initialize the dma interface */
	synopGMAC_dma_bus_mode_init(gmacdev,
				    DmaBurstLength32 | DmaDescriptorSkip0 | DmaDescriptor8Words);
	synopGMAC_dma_control_init(gmacdev,
				   DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);

	/* Initialize the mac interface */
	synopGMAC_mac_init(gmacdev);
	synopGMAC_promisc_enable(gmacdev);

	/* This enables the pause control in Full duplex mode of operation */
	synopGMAC_pause_control(gmacdev);

#if defined(NU_USING_HW_CHECKSUM)
	/*IPC Checksum offloading is enabled for this driver. Should only be used if
	 * Full Ip checksumm offload engine is configured in the hardware
	 */
	offload_needed = 1;

	/* Enable the offload engine in the receive path */
	synopGMAC_enable_rx_chksum_offload(gmacdev);

	/* Default configuration, DMA drops the packets if error in encapsulated ethernet payload */
	synopGMAC_rx_tcpip_chksum_drop_enable(gmacdev);
#endif

	for (i = 0; i < RECEIVE_DESC_SIZE; i++) {
		skb = &rx_buf[NUMAKER_GMAC_INTF][i];
		synopGMAC_set_rx_qptr(gmacdev, (u32)((u64)(skb->data) & NUMAKER_MASK_32),
				      sizeof(skb->data), (u32)((u64)skb & NUMAKER_MASK_32));
	}

	for (i = 0; i < TRANSMIT_DESC_SIZE; i++) {
		skb = &tx_buf[NUMAKER_GMAC_INTF][i];
		synopGMAC_set_tx_qptr(gmacdev, (u32)((u64)(skb->data) & NUMAKER_MASK_32),
				      sizeof(skb->data), (u32)((u64)skb & NUMAKER_MASK_32),
				      offload_needed, 0);
	}

	synopGMAC_set_mac_address(NUMAKER_GMAC_INTF, mac_addr);
	synopGMAC_clear_interrupt(gmacdev);

	return status;
}

static int m_numaker_gmacdev_get_rx_buf(synopGMACdevice *gmacdev, uint16_t *len, uint8_t **buf)
{
	DmaDesc *rxdesc = gmacdev->RxBusyDesc;

	LOG_DBG("start");
	if (synopGMAC_is_desc_owned_by_dma(rxdesc)) {
		return -EIO;
	}
	if (synopGMAC_is_desc_empty(rxdesc)) {
		return -EIO;
	}

	*len = synop_handle_received_data(NUMAKER_GMAC_INTF, buf);
	if (*len <= 0) {
		synopGMAC_enable_interrupt(gmacdev, DmaIntEnable);
		return -ENOSPC; /* No available RX frame */
	}

	/* length of payload should be <= 1514 */
	if (*len > (NU_ETH_MAX_FLEN - 4)) {
		LOG_DBG("unexpected long packet length=%d, buf=0x%x", *len, (uint32_t)*buf);
		*len = 0; /* Skip this unexpected long packet */
	}

	LOG_DBG("end");
	return 0;
}

static void m_numaker_gmacdev_rx_next(synopGMACdevice *gmacdev)
{
	LOG_DBG("RX Next");
	/* Already did in synop_handle_received_data
	 * No-op at this stage
	 * DmaDesc * rxdesc = (gmacdev->RxBusyDesc - 1);
	 * rxdesc->status = DescOwnByDma;
	 */
}

static void m_numaker_gmacdev_trigger_rx(synopGMACdevice *gmacdev)
{
	LOG_DBG("start");

	/* Enable the interrupt */
	synopGMAC_enable_interrupt(gmacdev, DmaIntEnable);

	/* Trigger RX DMA */
	synopGMAC_enable_dma_rx(gmacdev);
	synopGMAC_resume_dma_rx(gmacdev);
	LOG_DBG("resume RX DMA");
	LOG_DBG("end");
}

static void m_numaker_gmacdev_packet_rx(const struct device *dev)
{
	struct eth_numaker_data *data = dev->data;
	synopGMACdevice *gmacdev = data->gmacdev;
	uint8_t *buffer;
	uint16_t len;
	struct net_pkt *pkt;
	k_spinlock_key_t key;
	int res;

	/* Get exclusive access, use spin-lock instead of mutex in ISR */
	key = k_spin_lock(&data->rx_frame_buf_lock);

	/* Two approach: 1. recv all RX packets in one time.
	 *               2. recv one RX and set pending interrupt for rx-next.
	 */
	while (1) {
		/* get received frame */
		if (m_numaker_gmacdev_get_rx_buf(gmacdev, &len, &buffer) != 0) {
			break;
		}

		if (len == 0) {
			LOG_WRN("No available RX frame");
			break;
		}
		/* Allocate a memory buffer chain from buffer pool
		 * Using root iface. It will be updated in net_recv_data()
		 */
		pkt = net_pkt_rx_alloc_with_buffer(data->iface, len, AF_UNSPEC, 0, K_NO_WAIT);
		if (!pkt) {
			LOG_ERR("pkt alloc frame-len=%d failed", len);
			goto next;
		}

		LOG_DBG("length=%d, pkt=0x%x", len, (uint32_t)pkt);
		/* deliver RX packet to upper layer, pack as one net_pkt */
		if (net_pkt_write(pkt, buffer, len)) {
			LOG_ERR("Unable to write RX frame into the pkt");
			net_pkt_unref(pkt);
			goto error;
		}

		if (pkt != NULL) {
			res = net_recv_data(data->iface, pkt);
			if (res < 0) {
				LOG_ERR("net_recv_data: %d", res);
				net_pkt_unref(pkt);
				goto error;
			}
		}
next:
		m_numaker_gmacdev_rx_next(gmacdev);
	}
	m_numaker_gmacdev_trigger_rx(gmacdev);

error:
	k_spin_unlock(&data->rx_frame_buf_lock, key);
}

static uint8_t *m_numaker_gmacdev_get_tx_buf(synopGMACdevice *gmacdev)
{
	DmaDesc *txdesc = gmacdev->TxNextDesc;

	if (!synopGMAC_is_desc_empty(txdesc)) {
		return NULL;
	}

	if (synopGMAC_is_desc_owned_by_dma(txdesc)) {
		return NULL;
	}

	return (uint8_t *)(txdesc->buffer1);
}

static void m_numaker_gmacdev_trigger_tx(synopGMACdevice *gmacdev, uint16_t length)
{
	DmaDesc *txdesc = gmacdev->TxNextDesc;
	uint32_t txnext = gmacdev->TxNext;
	bool offload_needed = IS_ENABLED(NU_USING_HW_CHECKSUM);

	/* busy tx descriptor is incremented by one as it will be handed over to DMA */
	(gmacdev->BusyTxDesc)++;

	txdesc->length |= ((length << DescSize1Shift) & DescSize1Mask);
	txdesc->status |= (DescTxFirst | DescTxLast | DescTxIntEnable);
	if (offload_needed) {
		/*
		 * Make sure that the OS you are running supports the IP and TCP checksum
		 * offloading, before calling any of the functions given below.
		 */
		synopGMAC_tx_checksum_offload_tcp_pseudo(gmacdev, txdesc);
	} else {
		synopGMAC_tx_checksum_offload_bypass(gmacdev, txdesc);
	}
	__DSB();
	txdesc->status |= DescOwnByDma;

	gmacdev->TxNext = synopGMAC_is_last_tx_desc(gmacdev, txdesc) ? 0 : txnext + 1;
	gmacdev->TxNextDesc =
		synopGMAC_is_last_tx_desc(gmacdev, txdesc) ? gmacdev->TxDesc : (txdesc + 1);

	/* Enable the interrupt */
	synopGMAC_enable_interrupt(gmacdev, DmaIntEnable);
	/* Trigger TX DMA */
	synopGMAC_resume_dma_tx(gmacdev);
}

static int numaker_eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_numaker_data *data = dev->data;
	synopGMACdevice *gmacdev = data->gmacdev;
	uint16_t total_len = net_pkt_get_len(pkt);
	uint8_t *buffer;

	/* Get exclusive access */
	k_mutex_lock(&data->tx_frame_buf_mutex, K_FOREVER);
	if (total_len > NET_ETH_MAX_FRAME_SIZE) {
		/* NuMaker SDK reserve 2048 for tx_buf */
		LOG_ERR("TX packet length [%d] over max [%d]", total_len, NET_ETH_MAX_FRAME_SIZE);
		goto error;
	}

	buffer = m_numaker_gmacdev_get_tx_buf(gmacdev);
	LOG_DBG("buffer=0x%x", (uint32_t)buffer);
	if (buffer == NULL) {
		goto error;
	}

	if (net_pkt_read(pkt, buffer, total_len)) {
		goto error;
	}

	/* Prepare transmit descriptors to give to DMA */
	m_numaker_gmacdev_trigger_tx(gmacdev, total_len);

	k_mutex_unlock(&data->tx_frame_buf_mutex);

	return 0;

error:
	LOG_ERR("Writing pkt to TX descriptor failed");
	k_mutex_unlock(&data->tx_frame_buf_mutex);
	return -EIO;
}

static void numaker_eth_if_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_numaker_data *data = dev->data;

	synopGMACdevice *gmacdev = data->gmacdev;

	LOG_DBG("eth_if_init");

	/* Read mac address */
	m_numaker_read_mac_addr(data->mac_addr);

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);
	data->iface = iface;
	ethernet_init(iface);

	/* Enable GMAC device INT & TX/RX */
	m_numaker_gmacdev_enable(gmacdev);
}

static int numaker_eth_set_config(const struct device *dev, enum ethernet_config_type type,
				  const struct ethernet_config *config)
{
	struct eth_numaker_data *data = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));
		synopGMAC_set_mac_address(NUMAKER_GMAC_INTF, data->mac_addr);
		net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
				     NET_LINK_ETHERNET);
		LOG_DBG("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name, data->mac_addr[0],
			data->mac_addr[1], data->mac_addr[2], data->mac_addr[3], data->mac_addr[4],
			data->mac_addr[5]);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static enum ethernet_hw_caps numaker_eth_get_cap(const struct device *dev)
{
	ARG_UNUSED(dev);
#if defined(NU_USING_HW_CHECKSUM)
	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T | ETHERNET_HW_RX_CHKSUM_OFFLOAD;
#else
	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T;
#endif
}

static const struct ethernet_api eth_numaker_driver_api = {
	.iface_api.init = numaker_eth_if_init,
	.get_capabilities = numaker_eth_get_cap,
	.set_config = numaker_eth_set_config,
	.send = numaker_eth_tx,
};

/* EMAC IRQ Handler */
static void eth_numaker_isr(const struct device *dev)
{
	struct eth_numaker_data *data = dev->data;
	synopGMACdevice *gmacdev = data->gmacdev;
	uint32_t interrupt;
	uint32_t dma_status_reg;
	uint32_t mac_status_reg;
	int status;
	uint32_t dma_ie = DmaIntEnable;

	uint32_t volatile reg;

	/* Check GMAC interrupt */
	mac_status_reg = synopGMACReadReg((u32 *)gmacdev->MacBase, GmacInterruptStatus);
	if (mac_status_reg & GmacTSIntSts) {
		gmacdev->synopGMACNetStats.ts_int = 1;
		status = synopGMACReadReg((u32 *)gmacdev->MacBase, GmacTSStatus);
		if (!(status & BIT(1))) {
			LOG_WRN("TS alarm flag not set??");
		} else {
			LOG_DBG("TS alarm");
		}
	}

	if (mac_status_reg & GmacLPIIntSts) {
		LOG_DBG("LPI");
	}

	if (mac_status_reg & GmacRgmiiIntSts) {
		reg = synopGMACReadReg((u32 *)gmacdev->MacBase, GmacRgmiiCtrlSts);
	}

	synopGMACWriteReg((u32 *)gmacdev->MacBase, GmacInterruptStatus, mac_status_reg);
	/* Read the Dma interrupt status to know whether the interrupt got generated by
	 * our device or not
	 */
	dma_status_reg = synopGMACReadReg((u32 *)gmacdev->DmaBase, DmaStatus);
	LOG_DBG("i %08x %08x", mac_status_reg, dma_status_reg);

	if (dma_status_reg == 0) {
		return;
	}

	synopGMAC_disable_interrupt_all(gmacdev);
	LOG_DBG("Dma Status Reg: 0x%08x", dma_status_reg);
	if (dma_status_reg & GmacPmtIntr) {
		LOG_DBG("Interrupt due to PMT module");
		synopGMAC_powerup_mac(gmacdev);
	}

	if (dma_status_reg & GmacLineIntfIntr) {
		LOG_DBG("Interrupt due to GMAC LINE module");
	}

	/* Now lets handle the DMA interrupts */
	interrupt = synopGMAC_get_interrupt_type(gmacdev);
	LOG_DBG("Interrupts to be handled: 0x%08x", interrupt);
	if (interrupt & synopGMACDmaError) {
		LOG_DBG("Fatal Bus Error Interrupt Seen");
		synopGMAC_disable_dma_tx(gmacdev);
		synopGMAC_disable_dma_rx(gmacdev);

		synopGMAC_take_desc_ownership_tx(gmacdev);
		synopGMAC_take_desc_ownership_rx(gmacdev);

		synopGMAC_init_tx_rx_desc_queue(gmacdev);

		synopGMAC_reset(gmacdev); /* reset the DMA engine and the GMAC ip */
		synopGMAC_set_mac_address(NUMAKER_GMAC_INTF, data->mac_addr);
		synopGMAC_dma_bus_mode_init(gmacdev, DmaFixedBurstEnable | DmaBurstLength8 |
							     DmaDescriptorSkip0);
		synopGMAC_dma_control_init(gmacdev, DmaStoreAndForward);
		synopGMAC_init_rx_desc_base(gmacdev);
		synopGMAC_init_tx_desc_base(gmacdev);
		synopGMAC_mac_init(gmacdev);
		synopGMAC_enable_dma_rx(gmacdev);
		synopGMAC_enable_dma_tx(gmacdev);
	}

	if (interrupt & synopGMACDmaRxNormal) {
		LOG_DBG("Rx Normal");
		/* disable RX interrupt */
		dma_ie &= ~DmaIntRxNormMask;
		/* to handle received data */
		m_numaker_gmacdev_packet_rx(dev);
	}

	if (interrupt & synopGMACDmaRxAbnormal) {
		LOG_ERR("Abnormal Rx Interrupt Seen");
		/* If Mac is not in powerdown */
		if (gmacdev->GMAC_Power_down == 0) {
			gmacdev->synopGMACNetStats.rx_over_errors++;
			dma_ie &= ~DmaIntRxAbnMask;
			/* To handle GBPS with 12 descriptors. */
			synopGMAC_resume_dma_rx(gmacdev);
		}
	}

	/* Receiver gone in to stopped state */
	if (interrupt & synopGMACDmaRxStopped) {
		LOG_ERR("Receiver stopped seeing Rx interrupts");
		if (gmacdev->GMAC_Power_down == 0) {
			gmacdev->synopGMACNetStats.rx_over_errors++;
			synopGMAC_enable_dma_rx(gmacdev);
		}
	}

	if (interrupt & synopGMACDmaTxNormal) {
		LOG_DBG("Finished Normal Transmission");
		synop_handle_transmit_over(0);
		/* No-op at this stage for TX INT */
	}

	if (interrupt & synopGMACDmaTxAbnormal) {
		LOG_ERR("Abnormal Tx Interrupt Seen");
		if (gmacdev->GMAC_Power_down == 0) {
			synop_handle_transmit_over(0);
			/* No-op at this stage for TX INT */
		}
	}

	if (interrupt & synopGMACDmaTxStopped) {
		LOG_ERR("Transmitter stopped sending the packets");
		if (gmacdev->GMAC_Power_down == 0) {
			synopGMAC_disable_dma_tx(gmacdev);
			synopGMAC_take_desc_ownership_tx(gmacdev);
			synopGMAC_enable_dma_tx(gmacdev);
			LOG_ERR("Transmission Resumed");
		}
	}

	/* Enable the interrupt before returning from ISR*/
	synopGMAC_enable_interrupt(gmacdev, dma_ie);
}

/* Declare pin-ctrl __pinctrl_dev_config__device_dts_ord_xx before
 * PINCTRL_DT_INST_DEV_CONFIG_GET()
 */
PINCTRL_DT_INST_DEFINE(0);

static int eth_numaker_init(const struct device *dev)
{
	const struct eth_numaker_config *cfg = dev->config;
	struct eth_numaker_data *data = dev->data;
	synopGMACdevice *gmacdev;

	/* Init MAC Address based on UUID*/
	uint8_t mac_addr[NU_HWADDR_SIZE];
	int ret = 0;
	struct numaker_scc_subsys scc_subsys;

	gmacdev = &GMACdev[NUMAKER_GMAC_INTF];
	data->gmacdev = gmacdev;

	k_mutex_init(&data->tx_frame_buf_mutex);

	eth_phy_addr = cfg->phy_addr;

	/* CLK controller */
	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = cfg->clk_modidx;
	scc_subsys.pcc.clk_src = cfg->clk_src;
	scc_subsys.pcc.clk_div = cfg->clk_div;

	/* Equivalent to CLK_EnableModuleClock() */
	ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&scc_subsys);
	if (ret != 0) {
		goto done;
	}

	/* For EMAC, not need CLK_SetModuleClock()
	 * Validate this module's reset object
	 */
	if (!device_is_ready(cfg->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	SYS_UnlockReg();

	irq_disable(DT_INST_IRQN(0));
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to apply pinctrl state");
		goto done;
	}

	/* Reset EMAC to default state, same as BSP's SYS_ResetModule(id_rst) */
	reset_line_toggle_dt(&cfg->reset);

	/* Read mac address */
	m_numaker_read_mac_addr(mac_addr);

	/* Configure GMAC device */
	ret = m_numaker_gmacdev_init(gmacdev, mac_addr, cfg->gmac_base);
	if (ret != 0) {
		LOG_ERR("GMAC failed to initialize");
		goto done;
	}

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), eth_numaker_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));

done:
	SYS_LockReg();
	return ret;
}

static struct eth_numaker_data eth_numaker_data_inst;

/* Set config based on DTS */
static struct eth_numaker_config eth_numaker_cfg_inst = {
	.gmac_base = (uint32_t)DT_INST_REG_ADDR(0),
	.reset = RESET_DT_SPEC_INST_GET(0),
	.phy_addr = DT_INST_PROP(0, phy_addr),
	.clk_modidx = DT_INST_CLOCKS_CELL(0, clock_module_index),
	.clk_src = DT_INST_CLOCKS_CELL(0, clock_source),
	.clk_div = DT_INST_CLOCKS_CELL(0, clock_divider),
	.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(0))),
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.reset = RESET_DT_SPEC_INST_GET(0),
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_numaker_init, NULL, &eth_numaker_data_inst,
			      &eth_numaker_cfg_inst, CONFIG_ETH_INIT_PRIORITY,
			      &eth_numaker_driver_api, NET_ETH_MTU);
