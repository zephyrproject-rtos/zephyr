/*
 * Copyright (c) 2025 James Bennion-Pedley <james@bojit.org>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch32_ethernet

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ethernet_wch, CONFIG_ETHERNET_LOG_LEVEL);

#include <errno.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/lldp.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <ethernet/eth_stats.h>

#include <hal_ch32fun.h>

#include "eth.h"

/* Internal 10BASE-T PHY 50R*4 pull-up resistance enable or disable */
#define ETH_Internal_Pull_Up_Res_Enable  ((uint32_t)0x00100000)
#define ETH_Internal_Pull_Up_Res_Disable ((uint32_t)0x00000000)

/* MAC autoNegotiation enable or disable */
#define ETH_AutoNegotiation_Enable  ((uint32_t)0x00000001)
#define ETH_AutoNegotiation_Disable ((uint32_t)0x00000000)

/* MAC watchdog enable or disable */
#define ETH_Watchdog_Enable  ((uint32_t)0x00000000)
#define ETH_Watchdog_Disable ((uint32_t)0x00800000)

/* Bit description - MAC jabber enable or disable */
#define ETH_Jabber_Enable  ((uint32_t)0x00000000)
#define ETH_Jabber_Disable ((uint32_t)0x00400000)

/* Value of minimum IFG between frames during transmission */
#define ETH_InterFrameGap_96Bit ((uint32_t)0x00000000)
#define ETH_InterFrameGap_88Bit ((uint32_t)0x00020000)
#define ETH_InterFrameGap_80Bit ((uint32_t)0x00040000)
#define ETH_InterFrameGap_72Bit ((uint32_t)0x00060000)
#define ETH_InterFrameGap_64Bit ((uint32_t)0x00080000)
#define ETH_InterFrameGap_56Bit ((uint32_t)0x000A0000)
#define ETH_InterFrameGap_48Bit ((uint32_t)0x000C0000)
#define ETH_InterFrameGap_40Bit ((uint32_t)0x000E0000)

/* MAC carrier sense enable or disable */
#define ETH_CarrierSense_Enable  ((uint32_t)0x00000000)
#define ETH_CarrierSense_Disable ((uint32_t)0x00010000)

/* MAC speed */
#define ETH_Speed_10M   ((uint32_t)0x00000000)
#define ETH_Speed_100M  ((uint32_t)0x00004000)
#define ETH_Speed_1000M ((uint32_t)0x00008000)
#define ETH_Speed_Mask  ((uint32_t)0x0000C000)

/* MAC receive own enable or disable */
#define ETH_ReceiveOwn_Enable  ((uint32_t)0x00000000)
#define ETH_ReceiveOwn_Disable ((uint32_t)0x00002000)

/* MAC Loopback mode enable or disable */
#define ETH_LoopbackMode_Enable  ((uint32_t)0x00001000)
#define ETH_LoopbackMode_Disable ((uint32_t)0x00000000)

/* MAC full duplex or half duplex */
#define ETH_Mode_FullDuplex ((uint32_t)0x00000800)
#define ETH_Mode_HalfDuplex ((uint32_t)0x00000000)

/* MAC offload IP (IPCO) checksum enable or disable */
#define ETH_ChecksumOffload_Enable  ((uint32_t)0x00000400)
#define ETH_ChecksumOffload_Disable ((uint32_t)0x00000000)

/* MAC transmission retry enable or disable */
#define ETH_RetryTransmission_Enable  ((uint32_t)0x00000000)
#define ETH_RetryTransmission_Disable ((uint32_t)0x00000200)

/* MAC automatic pad CRC strip enable or disable */
#define ETH_AutomaticPadCRCStrip_Enable  ((uint32_t)0x00000080)
#define ETH_AutomaticPadCRCStrip_Disable ((uint32_t)0x00000000)

/* MAC backoff limitation */
#define ETH_BackOffLimit_10 ((uint32_t)0x00000000)
#define ETH_BackOffLimit_8  ((uint32_t)0x00000020)
#define ETH_BackOffLimit_4  ((uint32_t)0x00000040)
#define ETH_BackOffLimit_1  ((uint32_t)0x00000060)

/* MAC deferral check enable or disable */
#define ETH_DeferralCheck_Enable  ((uint32_t)0x00000010)
#define ETH_DeferralCheck_Disable ((uint32_t)0x00000000)

/* Bit description: MAC receive all frame enable or disable */
#define ETH_ReceiveAll_Enable  ((uint32_t)0x80000000)
#define ETH_ReceiveAll_Disable ((uint32_t)0x00000000)

/* MAC backoff limitation */
#define ETH_SourceAddrFilter_Normal_Enable  ((uint32_t)0x00000200)
#define ETH_SourceAddrFilter_Inverse_Enable ((uint32_t)0x00000300)
#define ETH_SourceAddrFilter_Disable        ((uint32_t)0x00000000)

/* MAC Pass control frames */
#define ETH_PassControlFrames_BlockAll                ((uint32_t)0x00000040)
#define ETH_PassControlFrames_ForwardAll              ((uint32_t)0x00000080)
#define ETH_PassControlFrames_ForwardPassedAddrFilter ((uint32_t)0x000000C0)

/* MAC broadcast frames reception */
#define ETH_BroadcastFramesReception_Enable  ((uint32_t)0x00000000)
#define ETH_BroadcastFramesReception_Disable ((uint32_t)0x00000020)

/* MAC destination address filter */
#define ETH_DestinationAddrFilter_Normal  ((uint32_t)0x00000000)
#define ETH_DestinationAddrFilter_Inverse ((uint32_t)0x00000008)

/* MAC Promiscuous mode enable or disable */
#define ETH_PromiscuousMode_Enable  ((uint32_t)0x00000001)
#define ETH_PromiscuousMode_Disable ((uint32_t)0x00000000)

/* MAC multicast frames filter */
#define ETH_MulticastFramesFilter_PerfectHashTable ((uint32_t)0x00000404)
#define ETH_MulticastFramesFilter_HashTable        ((uint32_t)0x00000004)
#define ETH_MulticastFramesFilter_Perfect          ((uint32_t)0x00000000)
#define ETH_MulticastFramesFilter_None             ((uint32_t)0x00000010)

/* MAC unicast frames filter */
#define ETH_UnicastFramesFilter_PerfectHashTable ((uint32_t)0x00000402)
#define ETH_UnicastFramesFilter_HashTable        ((uint32_t)0x00000002)
#define ETH_UnicastFramesFilter_Perfect          ((uint32_t)0x00000000)

/* Bit description  : MAC zero quanta pause */
#define ETH_ZeroQuantaPause_Enable  ((uint32_t)0x00000000)
#define ETH_ZeroQuantaPause_Disable ((uint32_t)0x00000080)

/* Field description  : MAC pause low threshold */
#define ETH_PauseLowThreshold_Minus4   ((uint32_t)0x00000000)
#define ETH_PauseLowThreshold_Minus28  ((uint32_t)0x00000010)
#define ETH_PauseLowThreshold_Minus144 ((uint32_t)0x00000020)
#define ETH_PauseLowThreshold_Minus256 ((uint32_t)0x00000030)

/* MAC unicast pause frame detect enable or disable*/
#define ETH_UnicastPauseFrameDetect_Enable  ((uint32_t)0x00000008)
#define ETH_UnicastPauseFrameDetect_Disable ((uint32_t)0x00000000)

/* MAC receive flow control frame enable or disable */
#define ETH_ReceiveFlowControl_Enable  ((uint32_t)0x00000004)
#define ETH_ReceiveFlowControl_Disable ((uint32_t)0x00000000)

/* MAC transmit flow control enable or disable */
#define ETH_TransmitFlowControl_Enable  ((uint32_t)0x00000002)
#define ETH_TransmitFlowControl_Disable ((uint32_t)0x00000000)

/* MAC VLAN tag comparison */
#define ETH_VLANTagComparison_12Bit ((uint32_t)0x00010000)
#define ETH_VLANTagComparison_16Bit ((uint32_t)0x00000000)

/* MAC flag */
#define ETH_MAC_FLAG_TST  ((uint32_t)0x00000200)
#define ETH_MAC_FLAG_MMCT ((uint32_t)0x00000040)
#define ETH_MAC_FLAG_MMCR ((uint32_t)0x00000020)
#define ETH_MAC_FLAG_MMC  ((uint32_t)0x00000010)
#define ETH_MAC_FLAG_PMT  ((uint32_t)0x00000008)

/* MAC interrupt */
#define ETH_MAC_IT_TST  ((uint32_t)0x00000200)
#define ETH_MAC_IT_MMCT ((uint32_t)0x00000040)
#define ETH_MAC_IT_MMCR ((uint32_t)0x00000020)
#define ETH_MAC_IT_MMC  ((uint32_t)0x00000010)
#define ETH_MAC_IT_PMT  ((uint32_t)0x00000008)

/* MAC address */
#define ETH_MAC_Address0 ((uint32_t)0x00000000)
#define ETH_MAC_Address1 ((uint32_t)0x00000008)
#define ETH_MAC_Address2 ((uint32_t)0x00000010)
#define ETH_MAC_Address3 ((uint32_t)0x00000018)

/* MAC address filter select */
#define ETH_MAC_AddressFilter_SA ((uint32_t)0x00000000)
#define ETH_MAC_AddressFilter_DA ((uint32_t)0x00000008)

/* MAC address mask */
#define ETH_MAC_AddressMask_Byte6 ((uint32_t)0x20000000)
#define ETH_MAC_AddressMask_Byte5 ((uint32_t)0x10000000)
#define ETH_MAC_AddressMask_Byte4 ((uint32_t)0x08000000)
#define ETH_MAC_AddressMask_Byte3 ((uint32_t)0x04000000)
#define ETH_MAC_AddressMask_Byte2 ((uint32_t)0x02000000)
#define ETH_MAC_AddressMask_Byte1 ((uint32_t)0x01000000)

#define ETH_DMA_TX_TIMEOUT_MS (20U) /* transmit timeout in milliseconds */

#define ETH_RXBUF_NB   (4U)
#define ETH_TXBUF_NB   (4U)
#define ETH_RXBUF_SIZE ETH_MAX_PACKET_SIZE /* Currently must be MTU-sized */
#define ETH_TXBUF_SIZE ETH_MAX_PACKET_SIZE /* Can be smaller if required */

struct eth_wch_config {
	ETH_TypeDef *regs;
	const struct device *phy_dev;

	const struct device *clk_dev;
	uint8_t clk_id;
	const struct device *clk_tx_dev;
	uint8_t clk_tx_id;
	const struct device *clk_rx_dev;
	uint8_t clk_rx_id;

	bool use_random_mac;
	bool use_internal_phy;
	uint8_t internal_phy_pllmul;
	uint8_t internal_phy_prediv;

	const struct pinctrl_dev_config *pin_cfg;
	void (*irq_config_func)(const struct device *dev);
};

struct eth_wch_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	struct k_mutex tx_mutex;
	struct k_sem rx_int_sem;
	struct k_sem tx_int_sem;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_WCH_HAL_RX_THREAD_STACK_SIZE);

	struct k_thread rx_thread;
#if defined(CONFIG_ETH_WCH_MULTICAST_FILTER)
	uint8_t hash_index_cnt[64];
#endif /* CONFIG_ETH_WCH_MULTICAST_FILTER */
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
};

struct eth_dma_desc {
	uint32_t volatile Status;     /* Status */
	uint32_t ControlBufferSize;   /* Control and Buffer1, Buffer2 lengths */
	uint32_t Buffer1Addr;         /* Buffer1 address pointer */
	uint32_t Buffer2NextDescAddr; /* Buffer2 or next descriptor address pointer */
};

/* NOTE: PLLMUL of 12.5 unreachable */
static const uint8_t phy_pllmul_lut[] = {2, 0, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 20};
static const uint8_t phy_prediv_lut[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
#define WCH_PHY_PLL3MUL_VAL(mul) (((mul) << 0x0C) & CFGR2_PLL3MUL)
#define WCH_PHY_PREDIV2_VAL(mul) (((mul) << 0x04) & CFGR2_PREDIV2)

/* TODO move these to instance macro to make instance-specific */
static struct eth_dma_desc __aligned(4) dma_rx_desc_tab[ETH_RXBUF_NB];
static struct eth_dma_desc __aligned(4) dma_tx_desc_tab[ETH_TXBUF_NB];
static struct eth_dma_desc *dma_rx_desc_current;
static struct eth_dma_desc *dma_tx_desc_current;
static uint8_t __aligned(4) dma_rx_buffer[ETH_RXBUF_NB * ETH_RXBUF_SIZE];
static uint8_t __aligned(4) dma_tx_buffer[ETH_TXBUF_NB * ETH_TXBUF_SIZE];

BUILD_ASSERT(ETH_RXBUF_SIZE % 4 == 0, "Buffer size must be a multiple of 4");
BUILD_ASSERT(ETH_TXBUF_SIZE % 4 == 0, "Buffer size must be a multiple of 4");

#if defined(CONFIG_ETH_WCH_MULTICAST_FILTER)
static void setup_multicast_filter(const struct device *dev, const struct ethernet_filter *filter)
{
	/* TODO implement */
}
#endif /* CONFIG_ETH_WCH_MULTICAST_FILTER */

static void setup_mac_filter(ETH_TypeDef *eth)
{
	/* TODO implement */
}

static void init_tx_dma_desc(ETH_TypeDef *eth)
{
	for (int i = 0; i < ETH_TXBUF_NB; i++) {
		dma_tx_desc_tab[i].Status = ETH_DMATxDesc_TCH | ETH_DMATxDesc_IC;
		dma_tx_desc_tab[i].Buffer1Addr = (uint32_t)(&dma_tx_buffer[i * ETH_TXBUF_SIZE]);
		dma_tx_desc_tab[i].ControlBufferSize = 0;
		dma_tx_desc_tab[i].Buffer2NextDescAddr = (uint32_t)(&dma_tx_desc_tab[i + 1]);
	}
	/* Chain buffers in a ring */
	dma_tx_desc_tab[ETH_TXBUF_NB - 1].Buffer2NextDescAddr = (uint32_t)(&dma_tx_desc_tab[0]);
	dma_tx_desc_current = dma_tx_desc_tab;
	eth->DMATDLAR = (uint32_t)dma_tx_desc_tab; /* pointer to start of desc. list */
}

/**
 * @brief Initialise the receive descriptor ring with generic attributes
 */
static void init_rx_dma_desc(ETH_TypeDef *eth)
{
	for (int i = 0; i < ETH_RXBUF_NB; i++) {
		dma_rx_desc_tab[i].Status = ETH_DMARxDesc_OWN;
		dma_rx_desc_tab[i].ControlBufferSize = ETH_DMARxDesc_RCH | ETH_RXBUF_SIZE;
		dma_rx_desc_tab[i].Buffer1Addr = (uint32_t)(&dma_rx_buffer[i * ETH_RXBUF_SIZE]);
		dma_rx_desc_tab[i].Buffer2NextDescAddr = (uint32_t)(&dma_rx_desc_tab[i + 1]);
	}
	/* Chain buffers in a ring */
	dma_rx_desc_tab[ETH_RXBUF_NB - 1].Buffer2NextDescAddr = (uint32_t)(&dma_rx_desc_tab[0]);
	dma_rx_desc_current = dma_rx_desc_tab;
	eth->DMARDLAR = (uint32_t)dma_rx_desc_tab; /* pointer to start of desc. list */
}

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_wch_data *data = dev->data;
	const struct eth_wch_config *config = dev->config;
	ETH_TypeDef *eth = config->regs;
	int res = 0;

	__ASSERT_NO_MSG(pkt != NULL);
	__ASSERT_NO_MSG(pkt->frags != NULL);

	/* Get full length of packet */
	size_t total_len = net_pkt_get_len(pkt);

	LOG_DBG("Sending Packet: %p of Length: %u", pkt, total_len);
	if (total_len > (ETH_TXBUF_SIZE * ETH_TXBUF_NB)) {
		eth_stats_update_errors_tx(data->iface);
		LOG_ERR("Packet spans all available descriptors");
		return -ENOBUFS;
	}

	k_mutex_lock(&data->tx_mutex, K_FOREVER); /* Only one packet can be transmitted at once */
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	k_sem_reset(&data->tx_int_sem);

	size_t bytes_remaining = total_len;

	do {
		if (dma_tx_desc_current->Status & ETH_DMATxDesc_OWN) {
			eth_stats_update_errors_tx(data->iface);
			LOG_ERR("No Descriptors Available");
			res = -EBUSY;
			goto error;
		}

		/* Copy Packet to TX Buf */
		size_t chunk_size =
			bytes_remaining > ETH_TXBUF_SIZE ? ETH_TXBUF_SIZE : bytes_remaining;
		if (net_pkt_read(pkt, (void *)(dma_tx_desc_current->Buffer1Addr), chunk_size)) {
			eth_stats_update_errors_tx(data->iface);
			LOG_ERR("Could not read descriptor buffer!");
			res = -ENOBUFS;
			goto error;
		}

		/* Set descriptor bits and hand to DMA engine */
		if (bytes_remaining == total_len) {
			dma_tx_desc_current->Status |= ETH_DMATxDesc_FS;
		}

		dma_tx_desc_current->ControlBufferSize = (chunk_size & ETH_DMATxDesc_TBS1);
		bytes_remaining -= chunk_size;

		if (bytes_remaining == 0) {
			dma_tx_desc_current->Status |= ETH_DMATxDesc_LS;
		}

		dma_tx_desc_current->Status |= ETH_DMATxDesc_OWN;

		/* Restart TX DMA if halted */
		if (eth->DMASR & ETH_DMASR_TBUS) {
			eth->DMASR = ETH_DMASR_TBUS;
			eth->DMATPDR = 0;
		}

		dma_tx_desc_current =
			(struct eth_dma_desc *)(dma_tx_desc_current->Buffer2NextDescAddr);
	} while (bytes_remaining > 0);

	/* Wait for end of TX buffer transmission */
	if (k_sem_take(&data->tx_int_sem, K_MSEC(ETH_DMA_TX_TIMEOUT_MS)) != 0) {
		eth_stats_update_errors_tx(data->iface);
		LOG_DBG("TX ISR Timeout");
		res = -EIO;
		goto error;
	}

	res = 0;
error:
	k_mutex_unlock(&data->tx_mutex);
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);

	return res;
}

static struct net_pkt *eth_rx(const struct device *dev)
{
	struct eth_wch_data *data = dev->data;
	const struct eth_wch_config *config = dev->config;
	ETH_TypeDef *eth = config->regs;
	struct net_pkt *pkt = NULL;

	if (dma_rx_desc_current->Status & ETH_DMARxDesc_OWN) {
		return NULL; /* Not error, simply packet has not arrived yet */
	}

	if ((dma_rx_desc_current->Status & ETH_DMARxDesc_ES) ||
	    ((dma_rx_desc_current->Status & (ETH_DMARxDesc_FS | ETH_DMARxDesc_LS)) !=
	     (ETH_DMARxDesc_FS | ETH_DMARxDesc_LS))) {
		goto release_desc; /* Drop descriptor if it is corrupt, or not a full frame */
	}

	size_t total_len = ((dma_rx_desc_current->Status & ETH_DMARxDesc_FL) >>
			    ETH_DMARXDESC_FRAME_LENGTHSHIFT) -
			   sizeof(uint32_t); /* This discards CRC (checked by hardware) */

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, total_len, AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("Failed to obtain RX buffer");
		goto release_desc;
	}

	if (net_pkt_write(pkt, (void *)(dma_rx_desc_current->Buffer1Addr), total_len)) {
		LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		pkt = NULL;
		goto release_desc;
	}

	LOG_DBG("Receiving Packet: %p", pkt);

release_desc:
	dma_rx_desc_current->Status |= ETH_DMARxDesc_OWN;
	dma_rx_desc_current = (struct eth_dma_desc *)(dma_rx_desc_current->Buffer2NextDescAddr);

	/* Restart RX DMA if halted */
	if (eth->DMASR & ETH_DMASR_RBUS) {
		eth->DMASR = ETH_DMASR_RBUS;
		eth->DMARPDR = 0;
	}

	if (!pkt) {
		eth_stats_update_errors_rx(data->iface);
	}

	return pkt;
}

static void rx_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = (const struct device *)arg1;
	struct eth_wch_data *data = dev->data;
	struct net_if *iface;
	struct net_pkt *pkt = NULL;
	int res;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (1) {
		res = k_sem_take(&data->rx_int_sem, K_FOREVER);
		if (res == 0) {
			while ((pkt = eth_rx(dev)) != NULL) {
				iface = net_pkt_iface(pkt);
				res = net_recv_data(iface, pkt);
				if (res < 0) {
					eth_stats_update_errors_rx(net_pkt_iface(pkt));
					LOG_ERR("Failed to enqueue "
						"frame "
						"into RX queue: %d",
						res);
					net_pkt_unref(pkt);
				}
			}
		}
	}
}

static void eth_isr(const struct device *dev)
{
	struct eth_wch_data *data = dev->data;
	const struct eth_wch_config *config = dev->config;
	ETH_TypeDef *eth = config->regs;

	uint32_t status_flags = eth->DMASR;

	/* Error Flags */
	if (status_flags & ETH_DMA_IT_AIS) {
		if (status_flags & ETH_DMA_IT_RBU) {
			eth->DMARPDR = 0; /* Re-trigger DMA Rx */
			eth->DMASR = ETH_DMA_IT_RBU;
		}
		eth->DMASR = ETH_DMA_IT_AIS;
	}

	/* Standard Flags */
	if (status_flags & ETH_DMA_IT_NIS) {
		if (status_flags & ETH_DMA_IT_R) {
			k_sem_give(&data->rx_int_sem);
			eth->DMASR = ETH_DMA_IT_R;
		}
		if (status_flags & ETH_DMA_IT_T) {
			k_sem_give(&data->tx_int_sem);
			eth->DMASR = ETH_DMA_IT_T;
		}
		if (status_flags & ETH_DMA_IT_PHYLINK) {
			/* for compatibility, simply use polling in the MDIO subsystem */
			eth->DMASR = ETH_DMA_IT_PHYLINK;
		}
		eth->DMASR = ETH_DMA_IT_NIS;
	}
}

static int eth_wch_start(const struct device *dev)
{
	const struct eth_wch_config *config = dev->config;
	ETH_TypeDef *eth = config->regs;

	LOG_DBG("Starting ETH HAL driver");

	eth->MACCR |= ETH_MACCR_TE | ETH_MACCR_RE;
	eth->DMAOMR |= ETH_DMAOMR_FTF | ETH_DMAOMR_ST | ETH_DMAOMR_SR;

	return 0;
}

static int eth_wch_stop(const struct device *dev)
{
	const struct eth_wch_config *config = dev->config;
	ETH_TypeDef *eth = config->regs;

	LOG_DBG("Stopping ETH HAL driver");

	eth->MACCR &= ~(ETH_MACCR_TE | ETH_MACCR_RE);
	eth->DMAOMR &= ~(ETH_DMAOMR_ST | ETH_DMAOMR_SR);

	return 0;
}

static void generate_mac(uint8_t *mac_addr, bool random)
{
	/* WCH uses this as a MAC address, unclear if it is an IEEE-assigned MAC */
	uint8_t *mac_base = (uint8_t *)(ROM_CFG_USERADR_ID);

	for (size_t i = 0; i < NET_ETH_ADDR_LEN; i++) {
		mac_addr[i] = mac_base[NET_ETH_ADDR_LEN - 1 - i];
	}

	if (random) {
		/* Generate random address based on WCH OUI (burned into the chip) */
		gen_random_mac(mac_addr, mac_addr[0], mac_addr[1], mac_addr[2]);
	}
}

static void set_mac_config(const struct device *dev, struct phy_link_state *state)
{
	const struct eth_wch_config *config = dev->config;
	ETH_TypeDef *eth = config->regs;

	/* Configure Speed and Duplex Mode */
	uint32_t tmpreg = eth->MACCR;

	tmpreg &= ~ETH_Mode_FullDuplex;
	tmpreg |= PHY_LINK_IS_FULL_DUPLEX(state->speed) ? ETH_Mode_FullDuplex : ETH_Mode_HalfDuplex;

	tmpreg &= ~ETH_Speed_Mask;
	tmpreg |= PHY_LINK_IS_SPEED_1000M(state->speed)  ? ETH_Speed_1000M
		  : PHY_LINK_IS_SPEED_100M(state->speed) ? ETH_Speed_100M
							 : ETH_Speed_10M;
}

static void phy_link_state_changed(const struct device *phy_dev, struct phy_link_state *state,
				   void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct eth_wch_data *data = dev->data;

	ARG_UNUSED(phy_dev);

	/*
	 * The MAC also needs to be stopped before changing the MAC
	 * config. The speed can change without receiving a link down
	 * callback before.
	 */
	eth_wch_stop(dev);
	if (state->is_up) {
		set_mac_config(dev, state);
		eth_wch_start(dev);
		net_eth_carrier_on(data->iface);
	} else {
		net_eth_carrier_off(data->iface);
	}
}

static int eth_mac_init(const struct device *dev)
{
	struct eth_wch_data *data = dev->data;
	const struct eth_wch_config *config = dev->config;
	ETH_TypeDef *eth = config->regs;

	eth->DMABMR |= ETH_DMABMR_SR;
	while (eth->DMABMR & ETH_DMABMR_SR) {
		;
	}

	/* Set MAC Address in Hardware */
	eth->MACA0HR = (data->mac_addr[5] << 8) | data->mac_addr[4];
	eth->MACA0LR = (data->mac_addr[3] << 24) | (data->mac_addr[2] << 16) |
		       (data->mac_addr[1] << 8) | data->mac_addr[0];

	/* Configure ethernet MAC */
	eth->MACCR = (ETH_Watchdog_Enable | ETH_Jabber_Enable | ETH_InterFrameGap_96Bit |
		      ETH_AutomaticPadCRCStrip_Disable | ETH_LoopbackMode_Disable
#if defined(CONFIG_ETH_WCH_HW_CHECKSUM)
		      | ETH_ChecksumOffload_Enable
#endif /* defined(CONFIG_ETH_WCH_HW_CHECKSUM) */
	);

	if (config->use_internal_phy) {
		eth->MACCR |= ETH_Internal_Pull_Up_Res_Enable;
	}

	eth->MACFFR = (ETH_ReceiveAll_Enable | ETH_PromiscuousMode_Disable |
		       ETH_BroadcastFramesReception_Enable | ETH_MulticastFramesFilter_Perfect |
		       ETH_UnicastFramesFilter_Perfect | ETH_PassControlFrames_BlockAll |
		       ETH_DestinationAddrFilter_Normal | ETH_SourceAddrFilter_Disable
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		       | ETH_PromiscuousMode_Enable
#endif /* defined(CONFIG_NET_PROMISCUOUS_MODE) */
	);

	eth->MACHTHR = 0x0;
	eth->MACHTLR = 0x0;

	eth->MACFCR = ETH_UnicastPauseFrameDetect_Disable | ETH_ReceiveFlowControl_Disable |
		      ETH_TransmitFlowControl_Disable;

	eth->MACVLANTR = ETH_VLANTagComparison_16Bit;

	eth->DMAOMR = ETH_DropTCPIPChecksumErrorFrame_Enable | ETH_TransmitStoreForward_Enable |
		      ETH_ForwardErrorFrames_Enable | ETH_ForwardUndersizedGoodFrames_Enable;

	/* Disable unwanted MMC interrupts */
	eth->MMCTIMR = ETH_MMCTIMR_TGFM;
	eth->MMCRIMR = ETH_MMCRIMR_RGUFM | ETH_MMCRIMR_RFCEM;

	eth->DMAIER =
		(ETH_DMA_IT_NIS | ETH_DMA_IT_R | ETH_DMA_IT_T | ETH_DMA_IT_AIS | ETH_DMA_IT_RBU);

	if (config->use_internal_phy) {
		eth->DMAIER |= ETH_DMA_IT_PHYLINK;
	}

	init_tx_dma_desc(eth);
	init_rx_dma_desc(eth);

	return 0;
}

static void eth_wch_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_wch_data *data = dev->data;
	const struct eth_wch_config *config = dev->config;

	/* Certain initialisation is only done once per interface */
	bool is_first_init = false;

	if (data->iface == NULL) {
		data->iface = iface;
		is_first_init = true;
	}

	/* Register Ethernet MAC Address with the upper layer */
	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);

	/* Initialise interface with relevant hardware settings */
	ethernet_init(iface);
	eth_mac_init(dev);
	setup_mac_filter(config->regs);

	net_if_carrier_off(iface);
	net_lldp_set_lldpdu(iface);

	/* Initialize semaphores */
	k_mutex_init(&data->tx_mutex);
	k_sem_init(&data->rx_int_sem, 0, K_SEM_MAX_LIMIT);
	k_sem_init(&data->tx_int_sem, 0, K_SEM_MAX_LIMIT);

	if (device_is_ready(config->phy_dev)) {
		phy_link_callback_set(config->phy_dev, phy_link_state_changed, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}

	if (is_first_init) {
		/* Now that the iface is setup, we are safe to enable IRQs. */
		__ASSERT_NO_MSG(config->irq_config_func != NULL);
		config->irq_config_func(dev);

		/* Start interruption-poll thread */
		k_thread_create(&data->rx_thread, data->rx_thread_stack,
				K_KERNEL_STACK_SIZEOF(data->rx_thread_stack), rx_thread,
				(void *)dev, NULL, NULL,
				K_PRIO_COOP(CONFIG_ETH_WCH_HAL_RX_THREAD_PRIO), 0, K_NO_WAIT);

		k_thread_name_set(&data->rx_thread, dev->name);
	}
}

static enum ethernet_hw_caps eth_wch_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_NET_VLAN)
	       | ETHERNET_HW_VLAN
#endif
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	       | ETHERNET_PROMISC_MODE
#endif
#if defined(CONFIG_NET_LLDP)
	       | ETHERNET_LLDP
#endif
		;
}

static int eth_wch_set_config(const struct device *dev, enum ethernet_config_type type,
			      const struct ethernet_config *config)
{
	struct eth_wch_data *data = dev->data;
	const struct eth_wch_config *dev_config = dev->config;
	ETH_TypeDef *eth = dev_config->regs;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, config->mac_address.addr, 6);
		eth->MACA0HR = (data->mac_addr[5] << 8) | data->mac_addr[4];
		eth->MACA0LR = (data->mac_addr[3] << 24) | (data->mac_addr[2] << 16) |
			       (data->mac_addr[1] << 8) | data->mac_addr[0];
		net_if_set_link_addr(data->iface, data->mac_addr, sizeof(data->mac_addr),
				     NET_LINK_ETHERNET);
		return 0;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (config->promisc_mode) {
			eth->MACFFR |= ETH_MACFFR_PM;
		} else {
			eth->MACFFR &= ~ETH_MACFFR_PM;
		}
		return 0;
#endif /* CONFIG_NET_PROMISCUOUS_MODE */
#if defined(CONFIG_ETH_WCH_MULTICAST_FILTER)
	case ETHERNET_CONFIG_TYPE_FILTER:
		setup_multicast_filter(dev, &config->filter);
		return 0;
#endif /* CONFIG_ETH_WCH_MULTICAST_FILTER */
	default:
		break;
	}

	return -ENOTSUP;
}

static const struct device *eth_wch_get_phy(const struct device *dev)
{
	const struct eth_wch_config *config = dev->config;

	return config->phy_dev;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_wch_get_stats(const struct device *dev)
{
	struct eth_wch_data *data = dev->data;

	return &data->stats;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

static int eth_wch_init(const struct device *dev)
{
	struct eth_wch_data *data = dev->data;
	const struct eth_wch_config *config = dev->config;
	clock_control_subsys_t clock_sys;

	int ret = 0;

	/* enable clocks */
	clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clk_id;
	ret = clock_control_on(config->clk_dev, clock_sys);
	clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clk_tx_id;
	ret |= clock_control_on(config->clk_tx_dev, clock_sys);
	clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clk_rx_id;
	ret |= clock_control_on(config->clk_rx_dev, clock_sys);

	if (ret) {
		LOG_ERR("Failed to enable ethernet clocks");
		return -EIO;
	}

	if (config->use_internal_phy) {
		/* NOTE: Internal PHY is clocked by separate PLL3, independently of sysclk */

		uint8_t pllmul = 0x0; /* Default Reset Value */

		for (size_t i = 0; i < ARRAY_SIZE(phy_pllmul_lut); i++) {
			if (phy_pllmul_lut[i] == config->internal_phy_pllmul) {
				pllmul = i;
			}
		}

		uint8_t prediv = 0x0; /* Default Reset Value */

		for (size_t i = 0; i < ARRAY_SIZE(phy_prediv_lut); i++) {
			if (phy_prediv_lut[i] == config->internal_phy_prediv) {
				prediv = i;
			}
		}

		RCC->CTLR &= ~RCC_PLL3ON;
		RCC->CFGR2 &= ~CFGR2_PREDIV2;
		RCC->CFGR2 |= WCH_PHY_PREDIV2_VAL(prediv);
		RCC->CFGR2 &= ~CFGR2_PLL3MUL;
		RCC->CFGR2 |= WCH_PHY_PLL3MUL_VAL(pllmul);
		RCC->CTLR |= RCC_PLL3ON;
		while (RCC->CTLR & RCC_PLL3RDY) {
			;
		}

		EXTEN->EXTEN_CTR |= EXTEN_ETH_10M_EN;
	}

	/* Software Reset of MAC peripherals */
	RCC->AHBRSTR |= RCC_ETHMACRST;
	RCC->AHBRSTR &= ~RCC_ETHMACRST;

	/* configure pinmux */
	ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins (%d)", ret);
		return ret;
	}

	/* Generate MAC address (once at boot) */
	generate_mac(data->mac_addr, config->use_random_mac);
	LOG_INF("MAC %02x:%02x:%02x:%02x:%02x:%02x", data->mac_addr[0], data->mac_addr[1],
		data->mac_addr[2], data->mac_addr[3], data->mac_addr[4], data->mac_addr[5]);

	return 0;
}

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_wch_iface_init,
	.get_capabilities = eth_wch_get_capabilities,
	.set_config = eth_wch_set_config,
	.get_phy = eth_wch_get_phy,
	.send = eth_tx,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_wch_get_stats,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
};

#define ETH_WCH_IRQ_HANDLER_DECL(idx)                                                              \
	static void eth_wch_irq_config_func_##idx(const struct device *dev);
#define ETH_WCH_IRQ_HANDLER_FUNC(idx) .irq_config_func = eth_wch_irq_config_func_##idx,
#define ETH_WCH_IRQ_HANDLER(idx)                                                                   \
	static void eth_wch_irq_config_func_##idx(const struct device *dev)                        \
	{                                                                                          \
		/* IRQ 0 is core Interrupt (IRQ 1 is wakeup) */                                    \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), eth_isr,                \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQN(idx));                                                     \
	}

#define ETH_WCH_DEVICE(inst)                                                                       \
	BUILD_ASSERT(DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, mii) ||                     \
			     DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, rmii) ||            \
			     DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, rgmii) ||           \
			     DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, internal),          \
		     "Unsupported PHY connection type");                                           \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	ETH_WCH_IRQ_HANDLER_DECL(inst)                                                             \
	static const struct eth_wch_config eth_wch_config_##inst = {                               \
		.regs = (ETH_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(inst)),                          \
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, phy_handle)),                       \
		.clk_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(inst))),                    \
		.clk_id = DT_CLOCKS_CELL(DT_INST_PARENT(inst), id),                                \
		.clk_tx_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 0)),                  \
		.clk_tx_id = DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, id),                              \
		.clk_rx_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 1)),                  \
		.clk_rx_id = DT_INST_CLOCKS_CELL_BY_IDX(inst, 1, id),                              \
		.use_random_mac = DT_INST_PROP(inst, zephyr_random_mac_address),                   \
		.use_internal_phy = DT_INST_ENUM_HAS_VALUE(inst, phy_connection_type, internal),   \
		.internal_phy_pllmul = DT_INST_PROP(inst, internal_phy_pllmul),                    \
		.internal_phy_prediv = DT_INST_PROP(inst, internal_phy_prediv),                    \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                   \
		ETH_WCH_IRQ_HANDLER_FUNC(inst)};                                                   \
	static struct eth_wch_data eth_wch_data_##inst;                                            \
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, eth_wch_init, NULL, &eth_wch_data_##inst,              \
				      &eth_wch_config_##inst, CONFIG_ETH_INIT_PRIORITY, &eth_api,  \
				      NET_ETH_MTU);                                                \
	ETH_WCH_IRQ_HANDLER(inst)

DT_INST_FOREACH_STATUS_OKAY(ETH_WCH_DEVICE)
