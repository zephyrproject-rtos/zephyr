/*
 * Copyright (c) 2022 Carbon Robotics
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_ethernet

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_gd32_hal, CONFIG_ETHERNET_LOG_LEVEL);

/* local headers */
#include "eth.h"

/* libc headers */
#include <errno.h>
#include <stdbool.h>

/* zephyr headers */
#include <ethernet/eth_stats.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
#include <zephyr/drivers/ptp_clock.h>
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */
#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

/* 3rd party libraries */
#include <gd32_enet.h>
#include <gd32_rcu.h>
#include <gd32_syscfg.h>

#define GD_OUI_B0 0x02
#define GD_OUI_B1 0x0A
#define GD_OUI_B2 0x0F

#define ETH_GD32_HAL_FIXED_LINK_NODE DT_CHILD(DT_DRV_INST(0), fixed_link)
#define ETH_GD32_HAL_FIXED_LINK DT_NODE_EXISTS(ETH_GD32_HAL_FIXED_LINK_NODE)
#define ETH_GD32_HAL_FIXED_LINK_SPEED DT_PROP(ETH_GD32_HAL_FIXED_LINK_NODE, speed)
#define ETH_GD32_HAL_FIXED_LINK_FULL_DUPLEX DT_PROP(ETH_GD32_HAL_FIXED_LINK_NODE, full_duplex)

#define ETH_GD32_HAL_MTU NET_ETH_MTU
#define ETH_GD32_HAL_NET_PKT_ALLOC_TIMEOUT K_MSEC(CONFIG_ETH_GD32_HAL_RX_NET_PKT_ALLOC_TIMEOUT_MS)

/* Device constant configuration parameters */
struct eth_gd32_hal_dev_cfg {
	void (*config_func)(void);
	const struct pinctrl_dev_config *pcfg;
};

/* Device run time data */
struct eth_gd32_hal_dev_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	enet_mediamode_enum mediamode;
	struct k_mutex tx_mutex;
	struct k_sem rx_int_sem;

	K_KERNEL_STACK_MEMBER(rx_thread_stack,
		CONFIG_ETH_GD32_HAL_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
	bool link_up;
#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	const struct device *ptp_clock;
	float clk_ratio;
	float clk_ratio_adj;
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */
};

/* ENET RxDMA/TxDMA descriptor */
extern enet_descriptors_struct rxdesc_tab[ENET_RXBUF_NUM], txdesc_tab[ENET_TXBUF_NUM];

/* global transmit and receive descriptors pointers */
extern enet_descriptors_struct *dma_current_txdesc;
extern enet_descriptors_struct *dma_current_rxdesc;

#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
static bool eth_is_ptp_pkt(struct net_if *iface, struct net_pkt *pkt)
{
#if defined(CONFIG_NET_VLAN)
	struct net_eth_vlan_hdr *hdr_vlan;
	struct ethernet_context *eth_ctx;

	eth_ctx = net_if_l2_data(iface);
	if (net_eth_is_vlan_enabled(eth_ctx, iface)) {
		hdr_vlan = (struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);

		if (ntohs(hdr_vlan->type) != NET_ETH_PTYPE_PTP) {
			return false;
		}
	} else
#endif
	{
		if (ntohs(NET_ETH_HDR(pkt)->type) != NET_ETH_PTYPE_PTP) {
			return false;
		}
	}

	net_pkt_set_priority(pkt, NET_PRIORITY_CA);

	return true;
}
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_gd32_hal_dev_data *dev_data = dev->data;
	ErrStatus hal_ret;
	uint8_t *dma_buffer;
	int res;
	size_t total_len;
#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	enet_descriptors_struct *txdesc;
	bool timestamped_frame;
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

	k_mutex_lock(&dev_data->tx_mutex, K_FOREVER);

	total_len = net_pkt_get_len(pkt);
	if (total_len > ENET_MAX_FRAME_SIZE) {
		LOG_ERR("PKT too big");
		res = -ENOMEM;
		goto error;
	}

	while ((uint32_t)RESET != (dma_current_txdesc->status & ENET_TDES0_DAV)) {
		k_yield();
	}

	dma_buffer = (uint8_t *)(enet_desc_information_get(dma_current_txdesc,
							   TXDESC_BUFFER_1_ADDR));

	if (net_pkt_read(pkt, dma_buffer, total_len)) {
		res = -ENOBUFS;
		goto error;
	}

#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	timestamped_frame = eth_is_ptp_pkt(net_pkt_iface(pkt), pkt);
	if (timestamped_frame) {
		enet_desc_flag_set(dma_current_txdesc, ENET_TDES0_TTSEN);
		txdesc = dma_current_txdesc;
	} else {
		enet_desc_flag_clear(dma_current_txdesc, ENET_TDES0_TTSEN);
	}
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

	hal_ret = ENET_NOCOPY_FRAME_TRANSMIT(total_len);

	if (hal_ret != SUCCESS) {
		LOG_ERR("HAL transmit failed!");
		res = -EIO;
		goto error;
	}

#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	if (timestamped_frame) {
		/*
		 * TODO: ENET_NOCOPY_PTPFRAME_TRANSMIT_ENHANCED_MODE is supposed to do this,
		 * but for some reason it gets stuck and times out waiting for TTMSS.
		 */
		while (!enet_desc_flag_get(txdesc, ENET_TDES0_TTMSS)) {
			k_yield();
		}
		pkt->timestamp.second = txdesc->timestamp_high;
		pkt->timestamp.nanosecond = txdesc->timestamp_low;
		enet_desc_flag_clear(txdesc, ENET_TDES0_TTMSS);
		net_if_add_tx_timestamp(pkt);
	}
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

	res = 0;
error:
	k_mutex_unlock(&dev_data->tx_mutex);

	return res;
}

static struct net_if *get_iface(struct eth_gd32_hal_dev_data *ctx,
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

static struct net_pkt *eth_rx(const struct device *dev, uint16_t *vlan_tag)
{
	struct eth_gd32_hal_dev_data *dev_data;
	struct net_pkt *pkt;
	size_t total_len;
	uint8_t *dma_buffer;
	ErrStatus hal_ret = SUCCESS;
#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	uint32_t rx_timestamp[2];
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

	dev_data = dev->data;

	if (enet_rxframe_size_get() == 0) {
		/* no frame available */
		return NULL;
	}

	total_len = enet_desc_information_get(dma_current_rxdesc, RXDESC_FRAME_LENGTH);
	dma_buffer = (uint8_t *)(enet_desc_information_get(dma_current_rxdesc,
							   RXDESC_BUFFER_1_ADDR));

	pkt = net_pkt_rx_alloc_with_buffer(get_iface(dev_data, *vlan_tag),
					   total_len, AF_UNSPEC, 0,
					   ETH_GD32_HAL_NET_PKT_ALLOC_TIMEOUT);
	if (pkt == NULL) {
		LOG_ERR("Failed to obtain RX buffer");
		goto release_desc;
	}

	if (net_pkt_write(pkt, dma_buffer, total_len)) {
		LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		pkt = NULL;
		goto release_desc;
	}

release_desc:
	/* Release descriptors to DMA */
#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	hal_ret = ENET_NOCOPY_PTPFRAME_RECEIVE_ENHANCED_MODE(rx_timestamp);
#else
	hal_ret = ENET_NOCOPY_FRAME_RECEIVE();
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

	if (!pkt || hal_ret == ERROR) {
		goto out;
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
		net_pkt_set_iface(pkt, dev_data->iface);
	}
#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	if (eth_is_ptp_pkt(get_iface(dev_data, *vlan_tag), pkt)) {
		pkt->timestamp.second = rx_timestamp[1];
		pkt->timestamp.nanosecond = rx_timestamp[0];
	} else {
		/* Invalid value */
		pkt->timestamp.second = UINT64_MAX;
		pkt->timestamp.nanosecond = UINT32_MAX;
	}
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

out:
	if (!pkt) {
		eth_stats_update_errors_rx(get_iface(dev_data, *vlan_tag));
	}

	return pkt;
}

static void rx_thread(void *arg1, void *unused1, void *unused2)
{
	uint16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	const struct device *dev;
	struct eth_gd32_hal_dev_data *dev_data;
	struct net_pkt *pkt;
	int res;
	uint16_t status;
	ErrStatus hal_ret = SUCCESS;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	dev = (const struct device *)arg1;
	dev_data = dev->data;

	while (1) {
		res = k_sem_take(&dev_data->rx_int_sem,
			K_MSEC(CONFIG_ETH_GD32_CARRIER_CHECK_RX_IDLE_TIMEOUT_MS));
		if (res == 0) {
			/* semaphore taken, update link status and receive packets */
			if (dev_data->link_up != true) {
				dev_data->link_up = true;
				net_eth_carrier_on(get_iface(dev_data,
							     vlan_tag));
			}
			while ((pkt = eth_rx(dev, &vlan_tag)) != NULL) {
				res = net_recv_data(net_pkt_iface(pkt), pkt);
				if (res < 0) {
					eth_stats_update_errors_rx(
							net_pkt_iface(pkt));
					LOG_ERR("Failed to enqueue frame "
						"into RX queue: %d", res);
					net_pkt_unref(pkt);
				}
			}
		} else if (res == -EAGAIN) {
			/* semaphore timeout period expired, check link status */
			hal_ret = enet_phy_write_read(ENET_PHY_READ, PHY_ADDRESS, PHY_REG_BSR,
						      &status);
			if (hal_ret == SUCCESS) {
				if ((status & PHY_LINKED_STATUS) == PHY_LINKED_STATUS) {
					if (dev_data->link_up != true) {
						dev_data->link_up = true;
						net_eth_carrier_on(
							get_iface(dev_data,
								  vlan_tag));
					}
				} else {
					if (dev_data->link_up != false) {
						dev_data->link_up = false;
						net_eth_carrier_off(
							get_iface(dev_data,
								  vlan_tag));
					}
				}
			}
		}
	}
}

static void eth_isr(const struct device *dev)
{
	struct eth_gd32_hal_dev_data *dev_data;

	dev_data = dev->data;

	/* clear the enet DMA Rx interrupt pending bits */
	enet_interrupt_flag_clear(ENET_DMA_INT_FLAG_RS_CLR);
	enet_interrupt_flag_clear(ENET_DMA_INT_FLAG_NI_CLR);

	if (enet_rxframe_size_get() > 0) {
		k_sem_give(&dev_data->rx_int_sem);
	}
}

#if !NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0))
static void generate_mac(uint8_t *mac_addr)
{
	gen_random_mac(mac_addr, GD_OUI_B0, GD_OUI_B1, GD_OUI_B2);
}
#endif /* !NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0)) */

static int eth_initialize(const struct device *dev)
{
	struct eth_gd32_hal_dev_data *dev_data;
	const struct eth_gd32_hal_dev_cfg *cfg;
	int ret = 0;

	dev_data = dev->data;
	cfg = dev->config;

#if defined(CONFIG_ETH_GD32_HAL_MII)
	syscfg_enet_phy_interface_config(SYSCFG_ENET_PHY_MII);
#else
	syscfg_enet_phy_interface_config(SYSCFG_ENET_PHY_RMII);
#endif /* CONFIG_ETH_GD32_HAL_MII */

	/* enable clock */
	rcu_periph_clock_enable(RCU_ENET);
	rcu_periph_clock_enable(RCU_ENETTX);
	rcu_periph_clock_enable(RCU_ENETRX);
#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	rcu_periph_clock_enable(RCU_ENETPTP);
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

	/* configure pinmux */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins");
		return ret;
	}

	/* reset ethernet on AHB bus */
	enet_deinit();

	if (enet_software_reset() != SUCCESS) {
		LOG_ERR("Could not reset ENET software");
		return -EIO;
	}

	/* disable multicast filtering */
	enet_initpara_config(FILTER_OPTION, ENET_MULTICAST_FILTER_NONE);
#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	enet_initpara_config(DMA_OPTION, ENET_ENHANCED_DESCRIPTOR);
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

	if (enet_init(dev_data->mediamode,
		      ENET_AUTOCHECKSUM_DROP_FAILFRAMES,
		      ENET_BROADCAST_FRAMES_PASS) != SUCCESS) {
		LOG_ERR("Could not initialize ethernet");
		return -EIO;
	}

	/* enable interrupts */
	enet_interrupt_enable(ENET_DMA_INT_NIE);
	enet_interrupt_enable(ENET_DMA_INT_RIE);

#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	enet_ptp_enhanced_descriptors_chain_init(ENET_DMA_TX);
	enet_ptp_enhanced_descriptors_chain_init(ENET_DMA_RX);
#else
	enet_descriptors_chain_init(ENET_DMA_TX);
	enet_descriptors_chain_init(ENET_DMA_RX);
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

	/* Enable ethernet RX interrupts */
	for (int i = 0; i < ENET_RXBUF_NUM; i++) {
		enet_rx_desc_immediate_receive_complete_interrupt(&rxdesc_tab[i]);
	}

	/* Enable the TCP, UDP and ICMP checksum insertion for the TX frames */
	for (int i = 0; i < ENET_TXBUF_NUM; i++) {
		enet_transmit_checksum_config(&txdesc_tab[i], ENET_CHECKSUM_TCPUDPICMP_FULL);
	}

#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	/* Enable timestamping of RX packets. We enable all packets to be
	 * timestamped to cover both IEEE 1588 and gPTP.
	 */
	enet_ptp_feature_enable(ENET_ALL_RX_TIMESTAMP);
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

#if !NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0))
	generate_mac(dev_data->mac_addr);
#endif /* !NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0)) */
	enet_mac_address_set(ENET_MAC_ADDRESS0, dev_data->mac_addr);

	dev_data->link_up = false;

	/* Initialize semaphores */
	k_mutex_init(&dev_data->tx_mutex);
	k_sem_init(&dev_data->rx_int_sem, 0, K_SEM_MAX_LIMIT);

	/* Start interruption-poll thread */
	k_thread_create(&dev_data->rx_thread, dev_data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(dev_data->rx_thread_stack),
			rx_thread, (void *) dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_GD32_HAL_RX_THREAD_PRIO),
			0, K_NO_WAIT);

	k_thread_name_set(&dev_data->rx_thread, "gd32_eth");

	enet_enable();

	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		dev_data->mac_addr[0], dev_data->mac_addr[1],
		dev_data->mac_addr[2], dev_data->mac_addr[3],
		dev_data->mac_addr[4], dev_data->mac_addr[5]);

	return 0;
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev;
	struct eth_gd32_hal_dev_data *dev_data;
	bool is_first_init = false;

	dev = net_if_get_device(iface);

	dev_data = dev->data;

	/* For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (dev_data->iface == NULL) {
		dev_data->iface = iface;
		is_first_init = true;
	}

	/* Register Ethernet MAC Address with the upper layer */
	net_if_set_link_addr(iface, dev_data->mac_addr,
			     sizeof(dev_data->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	if (is_first_init) {
		const struct eth_gd32_hal_dev_cfg *cfg = dev->config;
		/* Now that the iface is setup, we are safe to enable IRQs. */
		__ASSERT_NO_MSG(cfg->config_func != NULL);
		cfg->config_func();
	}
}

static enum ethernet_hw_caps eth_gd32_hal_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T
#if defined(CONFIG_NET_VLAN)
		| ETHERNET_HW_VLAN
#endif /* CONFIG_NET_VLAN */
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif /* CONFIG_NET_PROMISCUOUS_MODE */
#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
		| ETHERNET_PTP
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */
		;
}

static int eth_gd32_hal_set_config(const struct device *dev,
				   enum ethernet_config_type type,
				   const struct ethernet_config *config)
{
	int ret = -ENOTSUP;
	struct eth_gd32_hal_dev_data *dev_data = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(dev_data->mac_addr, config->mac_address.addr, 6);
		enet_mac_address_set(ENET_MAC_ADDRESS0, dev_data->mac_addr);
		net_if_set_link_addr(dev_data->iface, dev_data->mac_addr,
				     sizeof(dev_data->mac_addr),
				     NET_LINK_ETHERNET);
		ret = 0;
		break;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		if (config->promisc_mode) {
			enet_fliter_feature_enable(ENET_PROMISCUOUS_ENABLE);
		} else {
			enet_fliter_feature_disable(ENET_PROMISCUOUS_ENABLE);
		}
		ret = 0;
#endif /* CONFIG_NET_PROMISCUOUS_MODE */
		break;
	default:
		break;
	}

	return ret;
}

#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
static const struct device *eth_gd32_get_ptp_clock(const struct device *dev)
{
	struct eth_gd32_hal_dev_data *dev_data = dev->data;

	return dev_data->ptp_clock;
}
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
#if defined(CONFIG_PTP_CLOCK_GD32_HAL)
	.get_ptp_clock = eth_gd32_get_ptp_clock,
#endif /* CONFIG_PTP_CLOCK_GD32_HAL */
	.get_capabilities = eth_gd32_hal_get_capabilities,
	.set_config = eth_gd32_hal_set_config,
	.send = eth_tx,
};

static void eth0_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), eth_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

PINCTRL_DT_INST_DEFINE(0);

static const struct eth_gd32_hal_dev_cfg eth0_config = {
	.config_func = eth0_irq_config,
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct eth_gd32_hal_dev_data eth0_data = {
#if ETH_GD32_HAL_FIXED_LINK
#if ETH_GD32_HAL_FIXED_LINK_FULL_DUPLEX
#if ETH_GD32_HAL_FIXED_LINK_SPEED == 100
	.mediamode = ENET_100M_FULLDUPLEX,
#else
	.mediamode = ENET_10M_FULLDUPLEX,
#endif /* ETH_GD32_HAL_FIXED_LINK_SPEED === 100 */
#else
#if ETH_GD32_HAL_FIXED_LINK_SPEED == 100
	.mediamode = ENET_100M_FULLDUPLEX,
#else
	.mediamode = ENET_10M_HALFDUPLEX,
#endif /* ETH_GD32_HAL_FIXED_LINK_SPEED === 100 */
#endif /* ETH_GD32_HAL_FIXED_LINK_FULL_DUPLEX */
#else
	.mediamode = ENET_AUTO_NEGOTIATION,
#endif /* ETH_GD32_HAL_FIXED_LINK */
#if NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0))
	.mac_addr = DT_INST_PROP(0, local_mac_address),
#endif /* NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0)) */
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_initialize,
			      NULL, &eth0_data, &eth0_config,
			      CONFIG_ETH_INIT_PRIORITY, &eth_api, ETH_GD32_HAL_MTU);

#if defined(CONFIG_PTP_CLOCK_GD32_HAL)

struct ptp_context {
	struct eth_gd32_hal_dev_data *eth_dev_data;
};

static struct ptp_context ptp_gd32_0_context;

static int ptp_clock_gd32_set(const struct device *dev,
			      struct net_ptp_time *tm)
{
	int ret = 0;
	int key;
	ErrStatus hal_ret;

	key = irq_lock();

	enet_ptp_timestamp_update_config(ENET_PTP_ADD_TO_TIME, tm->second, tm->nanosecond);
	hal_ret = enet_ptp_timestamp_function_config(ENET_PTP_SYSTIME_INIT);
	if (hal_ret != SUCCESS) {
		ret = -EIO;
	}

	irq_unlock(key);

	return ret;
}

static int ptp_clock_gd32_get(const struct device *dev,
			      struct net_ptp_time *tm)
{
	int key;
	enet_ptp_systime_struct hal_tm, hal_tm_2;
	uint32_t second_2;

	key = irq_lock();

	enet_ptp_system_time_get(&hal_tm);
	tm->second = hal_tm.second;
	tm->nanosecond = hal_tm.subsecond;

	enet_ptp_system_time_get(&hal_tm_2);
	second_2 = hal_tm_2.second;

	irq_unlock(key);

	if (tm->second != second_2 && tm->nanosecond < NSEC_PER_SEC / 2) {
		/* Second rollover has happened during first measurement: second register
		 * was read before second boundary and nanosecond register was read after.
		 * We will use second_2 as a new second value.
		 */
		tm->second = second_2;
	}

	return 0;
}

static int ptp_clock_gd32_adjust(const struct device *dev, int increment)
{
	int ret = 0;
	int key;
	ErrStatus hal_ret;

	if ((increment <= (int32_t)(-NSEC_PER_SEC)) ||
			(increment >= (int32_t)NSEC_PER_SEC)) {
		ret = -EINVAL;
	} else {
		key = irq_lock();

		if (increment >= 0) {
			enet_ptp_timestamp_update_config(ENET_PTP_ADD_TO_TIME, 0, increment);
		} else {
			enet_ptp_timestamp_update_config(ENET_PTP_SUBSTRACT_FROM_TIME, 0,
							 increment);
		}
		hal_ret = enet_ptp_timestamp_function_config(ENET_PTP_SYSTIME_UPDATE);
		if (hal_ret != SUCCESS) {
			ret = -EIO;
		}

		irq_unlock(key);
	}

	return ret;
}

static int ptp_clock_gd32_rate_adjust(const struct device *dev, double ratio)
{
	struct ptp_context *ptp_context = dev->data;
	struct eth_gd32_hal_dev_data *eth_dev_data = ptp_context->eth_dev_data;
	int key, ret;
	uint32_t addend_val;
	ErrStatus hal_ret;

	/* No change needed */
	if (ratio == 1.0f) {
		return 0;
	}

	key = irq_lock();

	ratio *= eth_dev_data->clk_ratio_adj;

	/* Limit possible ratio */
	if (ratio * 100 < CONFIG_ETH_GD32_HAL_PTP_CLOCK_ADJ_MIN_PCT ||
	    ratio * 100 > CONFIG_ETH_GD32_HAL_PTP_CLOCK_ADJ_MAX_PCT) {
		ret = -EINVAL;
		goto error;
	}

	/* Save new ratio */
	eth_dev_data->clk_ratio_adj = ratio;

	/* Update addend register */
	addend_val = UINT32_MAX * eth_dev_data->clk_ratio * ratio;

	enet_ptp_timestamp_addend_config(addend_val);
	hal_ret = enet_ptp_timestamp_function_config(ENET_PTP_ADDEND_UPDATE);
	if (hal_ret != SUCCESS) {
		ret = -EIO;
		goto error;
	}

	ret = 0;

error:
	irq_unlock(key);

	return ret;
}

static const struct ptp_clock_driver_api api = {
	.set = ptp_clock_gd32_set,
	.get = ptp_clock_gd32_get,
	.adjust = ptp_clock_gd32_adjust,
	.rate_adjust = ptp_clock_gd32_rate_adjust,
};

static int ptp_gd32_init(const struct device *port)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct eth_gd32_hal_dev_data *eth_dev_data = dev->data;
	struct ptp_context *ptp_context = port->data;
	ErrStatus hal_ret;
	uint32_t ptp_hclk_rate;
	uint32_t ss_incr_ns;
	uint32_t addend_val;

	eth_dev_data->ptp_clock = port;
	ptp_context->eth_dev_data = eth_dev_data;

	/* Mask the Timestamp Trigger interrupt */
	ENET_MAC_INTMSK |= ENET_MAC_INTMSK_TMSTIM;

	/* Enable timestamping */
	enet_ptp_feature_enable(ENET_RXTX_TIMESTAMP);

	/* Query ethernet clock rate */
	ptp_hclk_rate = rcu_clock_freq_get(CK_AHB);

	/* Program the subsecond increment register based on the PTP clock freq */
	if (NSEC_PER_SEC % CONFIG_ETH_GD32_HAL_PTP_CLOCK_SRC_HZ != 0) {
		LOG_ERR("PTP clock period must be an integer nanosecond value");
		return -EINVAL;
	}
	ss_incr_ns = NSEC_PER_SEC / CONFIG_ETH_GD32_HAL_PTP_CLOCK_SRC_HZ;
	if (ss_incr_ns > UINT8_MAX) {
		LOG_ERR("PTP clock period is more than %d nanoseconds", UINT8_MAX);
		return -EINVAL;
	}
	enet_ptp_subsecond_increment_config(ss_incr_ns);

	/* Program timestamp addend register */
	eth_dev_data->clk_ratio =
		((double)CONFIG_ETH_GD32_HAL_PTP_CLOCK_SRC_HZ) / ((double)ptp_hclk_rate);
	/*
	 * clk_ratio is a ratio between desired PTP clock frequency and HCLK rate.
	 * Because HCLK is defined by a physical oscillator, it might drift due
	 * to manufacturing tolerances and environmental effects (e.g. temperature).
	 * clk_ratio_adj compensates for such inaccuracies. It starts off as 1.0
	 * and gets adjusted by calling ptp_clock_gd32_rate_adjust().
	 */
	eth_dev_data->clk_ratio_adj = 1.0f;
	addend_val =
		UINT32_MAX * eth_dev_data->clk_ratio * eth_dev_data->clk_ratio_adj;
	enet_ptp_timestamp_addend_config(addend_val);
	hal_ret = enet_ptp_timestamp_function_config(ENET_PTP_ADDEND_UPDATE);
	if (hal_ret != SUCCESS) {
		return -EIO;
	}

	/* Enable fine timestamp correction method */
	hal_ret = enet_ptp_timestamp_function_config(ENET_PTP_FINEMODE);
	if (hal_ret != SUCCESS) {
		return -EIO;
	}

	/* Enable nanosecond rollover into a new second */
	hal_ret = enet_ptp_timestamp_function_config(ENET_SUBSECOND_DIGITAL_ROLLOVER);
	if (hal_ret != SUCCESS) {
		return -EIO;
	}

	/* Initialize timestamp */
	enet_ptp_timestamp_update_config(ENET_PTP_ADD_TO_TIME, 0, 0);
	hal_ret = enet_ptp_timestamp_function_config(ENET_PTP_SYSTIME_INIT);
	if (hal_ret != SUCCESS) {
		return -EIO;
	}

	return 0;
}

DEVICE_DEFINE(gd32_ptp_clock_0, PTP_CLOCK_NAME, ptp_gd32_init,
	      NULL, &ptp_gd32_0_context, NULL, POST_KERNEL,
	      CONFIG_ETH_GD32_HAL_PTP_CLOCK_INIT_PRIO, &api);

#endif /* CONFIG_PTP_CLOCK_GD32_HAL */
