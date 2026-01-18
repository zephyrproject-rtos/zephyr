/*
 * Copyright (c) 2025 James Bennion-Pedley <james@bojit.org>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ethernet

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

#define ETH_MACCR_PR BIT(20)

#define ETH_MACCR_FES_10M   ((uint32_t)0x00000000)
#define ETH_MACCR_FES_100M  ((uint32_t)0x00004000)
#define ETH_MACCR_FES_1000M ((uint32_t)0x00008000)
#define ETH_MACCR_FES_MASK  ((uint32_t)0x0000C000)

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

	struct net_eth_mac_config mac_cfg;
	bool use_internal_phy;
	uint8_t internal_phy_pllmul;
	uint8_t internal_phy_prediv;

	const struct pinctrl_dev_config *pin_cfg;
	void (*irq_config_func)(const struct device *dev);
};

struct eth_wch_data {
	struct net_if *iface;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
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

/* NOTE for multiple ETH instances move these to instance macro */
static struct eth_dma_desc __aligned(4) dma_rx_desc_tab[ETH_RXBUF_NB];
static struct eth_dma_desc __aligned(4) dma_tx_desc_tab[ETH_TXBUF_NB];
static struct eth_dma_desc *dma_rx_desc_current;
static struct eth_dma_desc *dma_tx_desc_current;
static uint8_t __aligned(4) dma_rx_buffer[ETH_RXBUF_NB * ETH_RXBUF_SIZE];
static uint8_t __aligned(4) dma_tx_buffer[ETH_TXBUF_NB * ETH_TXBUF_SIZE];

BUILD_ASSERT(ETH_RXBUF_SIZE % 4 == 0, "Buffer size must be a multiple of 4");
BUILD_ASSERT(ETH_TXBUF_SIZE % 4 == 0, "Buffer size must be a multiple of 4");

#if defined(CONFIG_ETH_WCH_MULTICAST_FILTER)
static inline uint32_t reverse_bit_u32(uint32_t x)
{
	/* Equivalent of ARM `rbit` instruction */
	size_t bit_count = sizeof(x) * 8;
	uint32_t x_rev = 0;

	for (size_t i = 0; i < bit_count; i++) {
		if ((x & (1 << i))) {
			x_rev |= 1 << ((x_rev - 1) - i);
		}
	}

	return x_rev;
}

static void setup_multicast_filter(const struct device *dev, const struct ethernet_filter *filter)
{
	struct eth_wch_data *data = dev->data;
	const struct eth_wch_config *config = dev->config;
	ETH_TypeDef *eth = config->regs;
	uint32_t crc;
	uint32_t hash_table[2];
	uint32_t hash_index;

	crc = reverse_bit_u32(crc32_ieee(filter->mac_address.addr, sizeof(struct net_eth_addr)));
	hash_index = (crc >> 26) & 0x3f;

	__ASSERT_NO_MSG(hash_index < ARRAY_SIZE(data->hash_index_cnt));

	hash_table[0] = eth->MACHTLR;
	hash_table[1] = eth->MACHTHR;

	if (filter->set) {
		data->hash_index_cnt[hash_index]++;
		hash_table[hash_index / 32] |= (1 << (hash_index % 32));
	} else {
		if (data->hash_index_cnt[hash_index] == 0) {
			return; /* No hash at index to remove */
		}

		data->hash_index_cnt[hash_index]--;
		if (data->hash_index_cnt[hash_index] == 0) {
			hash_table[hash_index / 32] &= ~(1 << (hash_index % 32));
		}
	}

	eth->MACHTLR = hash_table[0];
	eth->MACHTHR = hash_table[1];
}
#endif /* CONFIG_ETH_WCH_MULTICAST_FILTER */

static void setup_mac_filter(ETH_TypeDef *eth)
{
	eth->MACFFR = (ETH_MACFFR_PCF_BlockAll
#if defined(CONFIG_ETH_WCH_MULTICAST_FILTER)
		       | ETH_MACFFR_HM
#else
		       | ETH_MACFFR_PAM
#endif /* CONFIG_ETH_WCH_MULTICAST_FILTER */
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		       | ETH_MACFFR_PM
#endif /* defined(CONFIG_NET_PROMISCUOUS_MODE) */
	);
}

static void set_mac_addr(ETH_TypeDef *eth, uint8_t mac[NET_ETH_ADDR_LEN], struct net_if *iface)
{
	eth->MACA0HR = (mac[5] << 8) | mac[4];
	eth->MACA0LR = (mac[3] << 24) | (mac[2] << 16) | (mac[1] << 8) | mac[0];
	net_if_set_link_addr(iface, mac, NET_ETH_ADDR_LEN, NET_LINK_ETHERNET);
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
		if ((dma_tx_desc_current->Status & ETH_DMATxDesc_OWN) != 0U) {
			eth_stats_update_errors_tx(data->iface);
			LOG_ERR("No Descriptors Available");
			res = -EBUSY;
			goto error;
		}

		/* Copy Packet to TX Buf */
		size_t chunk_size =
			bytes_remaining > ETH_TXBUF_SIZE ? ETH_TXBUF_SIZE : bytes_remaining;
		if (net_pkt_read(pkt, (void *)(dma_tx_desc_current->Buffer1Addr), chunk_size) !=
		    0U) {
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
		if ((eth->DMASR & ETH_DMASR_TBUS) != 0U) {
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

	if ((dma_rx_desc_current->Status & ETH_DMARxDesc_OWN) != 0U) {
		return NULL; /* Not error, simply packet has not arrived yet */
	}

	if (((dma_rx_desc_current->Status & ETH_DMARxDesc_ES) != 0U) ||
	    ((dma_rx_desc_current->Status & (ETH_DMARxDesc_FS | ETH_DMARxDesc_LS)) !=
	     (ETH_DMARxDesc_FS | ETH_DMARxDesc_LS))) {
		goto release_desc; /* Drop descriptor if it is corrupt, or not a full frame */
	}

	size_t total_len = ((dma_rx_desc_current->Status & ETH_DMARxDesc_FL) >>
			    ETH_DMARXDESC_FRAME_LENGTHSHIFT) -
			   sizeof(uint32_t); /* This discards CRC (checked by hardware) */

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, total_len, AF_UNSPEC, 0, K_MSEC(100));
	if (pkt == 0U) {
		LOG_ERR("Failed to obtain RX buffer");
		goto release_desc;
	}

	if (net_pkt_write(pkt, (void *)(dma_rx_desc_current->Buffer1Addr), total_len) != 0) {
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

	if (pkt == 0) {
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
	if ((status_flags & ETH_DMA_IT_AIS) != 0U) {
		if ((status_flags & ETH_DMA_IT_RBU) != 0U) {
			eth->DMARPDR = 0; /* Re-trigger DMA Rx */
			eth->DMASR = ETH_DMA_IT_RBU;
		}
		eth->DMASR = ETH_DMA_IT_AIS;
	}

	/* Standard Flags */
	if ((status_flags & ETH_DMA_IT_NIS) != 0U) {
		if ((status_flags & ETH_DMA_IT_R) != 0U) {
			k_sem_give(&data->rx_int_sem);
			eth->DMASR = ETH_DMA_IT_R;
		}
		if ((status_flags & ETH_DMA_IT_T) != 0U) {
			k_sem_give(&data->tx_int_sem);
			eth->DMASR = ETH_DMA_IT_T;
		}
		if ((status_flags & ETH_DMA_IT_PHYLINK) != 0U) {
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

static int get_hw_mac(uint8_t *mac_addr)
{
	uint8_t *mac_base = (uint8_t *)(ROM_CFG_USERADR_ID);

	for (size_t i = 0; i < NET_ETH_ADDR_LEN; i++) {
		mac_addr[i] = mac_base[NET_ETH_ADDR_LEN - 1 - i];
	}

	return 0;
}

static void set_mac_config(const struct device *dev, struct phy_link_state *state)
{
	const struct eth_wch_config *config = dev->config;
	ETH_TypeDef *eth = config->regs;

	/* Configure Speed and Duplex Mode */
	uint32_t tmpreg = eth->MACCR;

	tmpreg &= ~ETH_MACCR_DM;
	if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
		tmpreg |= ETH_MACCR_DM;
	}

	tmpreg &= ~ETH_MACCR_FES_MASK;
	tmpreg |= PHY_LINK_IS_SPEED_1000M(state->speed)  ? ETH_MACCR_FES_1000M
		  : PHY_LINK_IS_SPEED_100M(state->speed) ? ETH_MACCR_FES_100M
							 : ETH_MACCR_FES_10M;
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
	const struct eth_wch_config *config = dev->config;
	ETH_TypeDef *eth = config->regs;

	eth->DMABMR |= ETH_DMABMR_SR;
	while (eth->DMABMR & ETH_DMABMR_SR) {
		;
	}

	/* Configure ethernet MAC */
	eth->MACCR = 0x0;
#if defined(CONFIG_ETH_WCH_HW_CHECKSUM)
	eth->MACCR |= ETH_MACCR_IPCO;
#endif /* defined(CONFIG_ETH_WCH_HW_CHECKSUM) */

	eth->MACHTHR = 0x0;
	eth->MACHTLR = 0x0;
	eth->MACFCR = 0x0;
	eth->MACVLANTR = 0x0;

	eth->DMAOMR = ETH_DMAOMR_TSF | ETH_DMAOMR_FEF | ETH_DMAOMR_FUGF;

	/* Disable unwanted MMC interrupts */
	eth->MMCTIMR = ETH_MMCTIMR_TGFM;
	eth->MMCRIMR = ETH_MMCRIMR_RGUFM | ETH_MMCRIMR_RFCEM;

	eth->DMAIER =
		(ETH_DMA_IT_NIS | ETH_DMA_IT_R | ETH_DMA_IT_T | ETH_DMA_IT_AIS | ETH_DMA_IT_RBU);

	if (config->use_internal_phy) {
		eth->MACCR |= ETH_MACCR_PR;
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

	if (is_first_init) {
		/* Start interruption-poll thread */
		k_thread_create(&data->rx_thread, data->rx_thread_stack,
				K_KERNEL_STACK_SIZEOF(data->rx_thread_stack), rx_thread,
				(void *)dev, NULL, NULL,
				K_PRIO_COOP(CONFIG_ETH_WCH_HAL_RX_THREAD_PRIO), 0, K_NO_WAIT);

		k_thread_name_set(&data->rx_thread, dev->name);
	}

	/* Initialise interface with relevant hardware settings */
	ethernet_init(iface);
	eth_mac_init(dev);
	set_mac_addr(config->regs, data->mac_addr, iface);
	setup_mac_filter(config->regs);

	net_if_carrier_off(iface);
	net_lldp_set_lldpdu(iface);

	if (device_is_ready(config->phy_dev)) {
		phy_link_callback_set(config->phy_dev, phy_link_state_changed, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}
}

static enum ethernet_hw_caps eth_wch_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_ETH_WCH_HW_CHECKSUM)
	       | ETHERNET_HW_TX_CHKSUM_OFFLOAD | ETHERNET_HW_RX_CHKSUM_OFFLOAD
#endif
#if defined(CONFIG_ETH_WCH_MULTICAST_FILTER)
	       | ETHERNET_HW_FILTERING
#endif
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
		set_mac_addr(eth, data->mac_addr, data->iface);
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

	int ret;

	/* enable clocks */
	clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clk_id;
	ret = clock_control_on(config->clk_dev, clock_sys);
	clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clk_tx_id;
	ret |= clock_control_on(config->clk_tx_dev, clock_sys);
	clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clk_rx_id;
	ret |= clock_control_on(config->clk_rx_dev, clock_sys);

	if (ret < 0) {
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

	/* Configure MAC Address */
	ret = net_eth_mac_load(&config->mac_cfg, data->mac_addr);
	if (ret == -ENODATA) {
		ret = get_hw_mac(data->mac_addr);
	}

	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x", data->mac_addr[0], data->mac_addr[1],
		data->mac_addr[2], data->mac_addr[3], data->mac_addr[4], data->mac_addr[5]);

	/* Initialize semaphores */
	k_mutex_init(&data->tx_mutex);
	k_sem_init(&data->rx_int_sem, 0, K_SEM_MAX_LIMIT);
	k_sem_init(&data->tx_int_sem, 0, K_SEM_MAX_LIMIT);

	/* IRQ config */
	__ASSERT_NO_MSG(config->irq_config_func != NULL);
	config->irq_config_func(dev);

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
		.clk_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(DT_INST_PARENT(inst), mac)),       \
		.clk_id = DT_CLOCKS_CELL_BY_NAME(DT_INST_PARENT(inst), mac, id),                   \
		.clk_tx_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(DT_INST_PARENT(inst), tx)),     \
		.clk_tx_id = DT_CLOCKS_CELL_BY_NAME(DT_INST_PARENT(inst), tx, id),                 \
		.clk_rx_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(DT_INST_PARENT(inst), rx)),     \
		.clk_rx_id = DT_CLOCKS_CELL_BY_NAME(DT_INST_PARENT(inst), rx, id),                 \
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),                                  \
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
