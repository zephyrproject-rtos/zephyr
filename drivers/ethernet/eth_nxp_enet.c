/* NXP ENET MAC Driver
 *
 * Copyright 2023 NXP
 *
 * Inspiration from eth_mcux.c, which is:
 *  Copyright (c) 2016-2017 ARM Ltd
 *  Copyright (c) 2016 Linaro Ltd
 *  Copyright (c) 2018 Intel Corporation
 *  Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_enet_mac

/* Set up logging module for this driver */
#define LOG_MODULE_NAME eth_nxp_enet_mac
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/*
 ************
 * Includes *
 ************
 */

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/ethernet/eth_nxp_enet.h>
#include <zephyr/dt-bindings/ethernet/nxp_enet.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#if defined(CONFIG_NET_DSA)
#include <zephyr/net/dsa.h>
#endif

#include "fsl_enet.h"

/*
 ***********
 * Defines *
 ***********
 */

#define RING_ID 0

/*
 *********************
 * Driver Structures *
 *********************
 */

struct nxp_enet_mac_config {
	ENET_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*generate_mac)(uint8_t *mac_addr);
	const struct pinctrl_dev_config *pincfg;
	enet_buffer_config_t buffer_config;
	uint8_t phy_mode;
	void (*irq_config_func)(void);
	const struct device *phy_dev;
	const struct device *mdio;
};

struct nxp_enet_mac_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	enet_handle_t enet_handle;
	struct k_sem tx_buf_sem;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_NXP_ENET_RX_THREAD_STACK_SIZE);

	struct k_thread rx_thread;
	struct k_sem rx_thread_sem;
	struct k_mutex tx_frame_buf_mutex;
	struct k_mutex rx_frame_buf_mutex;
	/* TODO: FIXME. This Ethernet frame sized buffer is used for
	 * interfacing with MCUX. How it works is that hardware uses
	 * DMA scatter buffers to receive a frame, and then public
	 * MCUX call gathers them into this buffer (there's no other
	 * public interface). All this happens only for this driver
	 * to scatter this buffer again into Zephyr fragment buffers.
	 * This is not efficient, but proper resolution of this issue
	 * depends on introduction of zero-copy networking support
	 * in Zephyr, and adding needed interface to MCUX (or
	 * bypassing it and writing a more complex driver working
	 * directly with hardware).
	 *
	 * Note that we do not copy FCS into this buffer thus the
	 * size is 1514 bytes.
	 */
	uint8_t *tx_frame_buf; /* Max MTU + ethernet header */
	uint8_t *rx_frame_buf; /* Max MTU + ethernet header */
};

/*
 ********************
 * Helper Functions *
 ********************
 */

extern void nxp_enet_mdio_callback(const struct device *mdio_dev,
				enum nxp_enet_callback_reason event);

static inline struct net_if *get_iface(struct nxp_enet_mac_data *data, uint16_t vlan_tag)
{
#if defined(CONFIG_NET_VLAN)
	struct net_if *iface;

	iface = net_eth_get_vlan_iface(data->iface, vlan_tag);
	if (!iface) {
		return data->iface;
	}

	return iface;
#else
	ARG_UNUSED(vlan_tag);

	return data->iface;
#endif
}

#if defined(CONFIG_NET_NATIVE_IPV4) || defined(CONFIG_NET_NATIVE_IPV6)
static void net_if_mcast_cb(struct net_if *iface,
			    const struct net_addr *addr,
			    bool is_joined)
{
	const struct device *dev = net_if_get_device(iface);
	const struct nxp_enet_mac_config *config = dev->config;
	struct net_eth_addr mac_addr;

	if (IS_ENABLED(CONFIG_NET_IPV4) && addr->family == AF_INET) {
		net_eth_ipv4_mcast_to_mac_addr(&addr->in_addr, &mac_addr);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && addr->family == AF_INET6) {
		net_eth_ipv6_mcast_to_mac_addr(&addr->in6_addr, &mac_addr);
	} else {
		return;
	}

	if (is_joined) {
		ENET_AddMulticastGroup(config->base, mac_addr.addr);
	} else {
		ENET_LeaveMulticastGroup(config->base, mac_addr.addr);
	}
}
#endif /* CONFIG_NET_NATIVE_IPV4 || CONFIG_NET_NATIVE_IPV6 */

/*
 **************************************
 * L2 Networking Driver API Functions *
 **************************************
 */


static int eth_nxp_enet_tx(const struct device *dev, struct net_pkt *pkt)
{
	const struct nxp_enet_mac_config *config = dev->config;
	struct nxp_enet_mac_data *data = dev->data;
	uint16_t total_len = net_pkt_get_len(pkt);
	status_t status;

	/* Wait for a TX buffer descriptor to be available */
	k_sem_take(&data->tx_buf_sem, K_FOREVER);

	/* Enter critical section for TX frame buffer access */
	k_mutex_lock(&data->tx_frame_buf_mutex, K_FOREVER);

	/* Read network packet from upper layer into frame buffer */
	if (net_pkt_read(pkt, data->tx_frame_buf, total_len)) {
		k_sem_give(&data->tx_buf_sem);
		k_mutex_unlock(&data->tx_frame_buf_mutex);
		return -EIO;
	}

	/* Send frame to ring buffer for transmit */
	status = ENET_SendFrame(config->base, &data->enet_handle,
				data->tx_frame_buf, total_len, RING_ID, false, NULL);

	if (status) {
		LOG_ERR("ENET_SendFrame error: %d", (int)status);
		k_mutex_unlock(&data->tx_frame_buf_mutex);
		ENET_ReclaimTxDescriptor(config->base,
					&data->enet_handle, RING_ID);
		return -1;
	}

	/* Leave critical section for TX frame buffer access */
	k_mutex_unlock(&data->tx_frame_buf_mutex);

	return 0;
}

static void eth_nxp_enet_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct nxp_enet_mac_data *data = dev->data;
	const struct nxp_enet_mac_config *config = dev->config;
#if defined(CONFIG_NET_NATIVE_IPV4) || defined(CONFIG_NET_NATIVE_IPV6)
	static struct net_if_mcast_monitor mon;

	net_if_mcast_mon_register(&mon, iface, net_if_mcast_cb);
#endif /* CONFIG_NET_NATIVE_IPV4 || CONFIG_NET_NATIVE_IPV6 */

	net_if_set_link_addr(iface, data->mac_addr,
			     sizeof(data->mac_addr),
			     NET_LINK_ETHERNET);

	/* For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (data->iface == NULL) {
		data->iface = iface;
	}

#if defined(CONFIG_NET_DSA)
	dsa_register_master_tx(iface, &eth_nxp_enet_tx);
#endif
	ethernet_init(iface);
	net_eth_carrier_off(data->iface);

	config->irq_config_func();
}

static enum ethernet_hw_caps eth_nxp_enet_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_HW_VLAN | ETHERNET_LINK_10BASE_T |
#if defined(CONFIG_NET_DSA)
		ETHERNET_DSA_MASTER_PORT |
#endif
#if defined(CONFIG_ETH_NXP_ENET_HW_ACCELERATION)
		ETHERNET_HW_TX_CHKSUM_OFFLOAD |
		ETHERNET_HW_RX_CHKSUM_OFFLOAD |
#endif
		ETHERNET_LINK_100BASE_T;
}

static int eth_nxp_enet_set_config(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *cfg)
{
	struct nxp_enet_mac_data *data = dev->data;
	const struct nxp_enet_mac_config *config = dev->config;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr,
		       cfg->mac_address.addr,
		       sizeof(data->mac_addr));
		ENET_SetMacAddr(config->base, data->mac_addr);
		net_if_set_link_addr(data->iface, data->mac_addr,
				     sizeof(data->mac_addr),
				     NET_LINK_ETHERNET);
		LOG_DBG("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x",
			dev->name,
			data->mac_addr[0], data->mac_addr[1],
			data->mac_addr[2], data->mac_addr[3],
			data->mac_addr[4], data->mac_addr[5]);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

/*
 *****************************
 * Ethernet RX Functionality *
 *****************************
 */

static int eth_nxp_enet_rx(const struct device *dev)
{
	const struct nxp_enet_mac_config *config = dev->config;
	struct nxp_enet_mac_data *data = dev->data;
	uint16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	uint32_t frame_length = 0U;
	struct net_if *iface;
	struct net_pkt *pkt;
	status_t status;
	uint32_t ts;

	status = ENET_GetRxFrameSize(&data->enet_handle,
				     (uint32_t *)&frame_length, RING_ID);
	if (status == kStatus_ENET_RxFrameEmpty) {
		return 0;
	} else if (status == kStatus_ENET_RxFrameError) {
		enet_data_error_stats_t error_stats;

		LOG_ERR("ENET_GetRxFrameSize return: %d", (int)status);

		ENET_GetRxErrBeforeReadFrame(&data->enet_handle,
					     &error_stats, RING_ID);
		goto flush;
	}

	if (frame_length > NET_ETH_MAX_FRAME_SIZE) {
		LOG_ERR("frame too large (%d)", frame_length);
		goto flush;
	}

	/* Using root iface. It will be updated in net_recv_data() */
	pkt = net_pkt_rx_alloc_with_buffer(data->iface, frame_length,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		goto flush;
	}

	/* in case multiply thread access
	 * we need to protect it with mutex.
	 */
	k_mutex_lock(&data->rx_frame_buf_mutex, K_FOREVER);

	status = ENET_ReadFrame(config->base, &data->enet_handle,
				data->rx_frame_buf, frame_length, RING_ID, &ts);
	if (status) {
		LOG_ERR("ENET_ReadFrame failed: %d", (int)status);
		net_pkt_unref(pkt);

		k_mutex_unlock(&data->rx_frame_buf_mutex);
		goto error;
	}

	if (net_pkt_write(pkt, data->rx_frame_buf, frame_length)) {
		LOG_ERR("Unable to write frame into the pkt");
		net_pkt_unref(pkt);
		k_mutex_unlock(&data->rx_frame_buf_mutex);
		goto error;
	}

	k_mutex_unlock(&data->rx_frame_buf_mutex);

#if defined(CONFIG_NET_VLAN)
	{
		struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

		if (ntohs(hdr->type) == NET_ETH_PTYPE_VLAN) {
			struct net_eth_vlan_hdr *hdr_vlan =
				(struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);

			net_pkt_set_vlan_tci(pkt, ntohs(hdr_vlan->vlan.tci));
			vlan_tag = net_pkt_vlan_tag(pkt);

#if CONFIG_NET_TC_RX_COUNT > 1
			{
				enum net_priority prio;

				prio = net_vlan2priority(
						net_pkt_vlan_priority(pkt));
				net_pkt_set_priority(pkt, prio);
			}
#endif /* CONFIG_NET_TC_RX_COUNT > 1 */
		}
	}
#endif /* CONFIG_NET_VLAN */

	iface = get_iface(data, vlan_tag);
#if defined(CONFIG_NET_DSA)
	iface = dsa_net_recv(iface, &pkt);
#endif
	if (net_recv_data(iface, pkt) < 0) {
		net_pkt_unref(pkt);
		goto error;
	}

	return 1;
flush:
	/* Flush the current read buffer.  This operation can
	 * only report failure if there is no frame to flush,
	 * which cannot happen in this context.
	 */
	status = ENET_ReadFrame(config->base, &data->enet_handle, NULL,
					0, RING_ID, NULL);
	__ASSERT_NO_MSG(status == kStatus_Success);
error:
	eth_stats_update_errors_rx(get_iface(data, vlan_tag));
	return -EIO;
}

static void eth_nxp_enet_rx_thread(void *arg1, void *unused1, void *unused2)
{
	const struct device *dev = arg1;
	const struct nxp_enet_mac_config *config = dev->config;
	struct nxp_enet_mac_data *data = dev->data;

	while (1) {
		if (k_sem_take(&data->rx_thread_sem, K_FOREVER) == 0) {
			while (eth_nxp_enet_rx(dev) == 1) {
				;
			}
			/* enable the IRQ for RX */
			ENET_EnableInterrupts(config->base,
			  kENET_RxFrameInterrupt | kENET_RxBufferInterrupt);
		}
	}
}

/*
 ****************************
 * PHY management functions *
 ****************************
 */

static int nxp_enet_phy_reset_and_configure(const struct device *phy)
{
	int ret = 0;

	/* Reset the PHY */
	ret = phy_write(phy, MII_BMCR, MII_BMCR_RESET);
	if (ret) {
		return ret;
	}

	/* 802.3u standard says reset takes up to 0.5s */
	k_busy_wait(500000);

	/* Configure the PHY */
	ret = phy_configure_link(phy, LINK_HALF_10BASE_T |
				LINK_FULL_10BASE_T |
				LINK_HALF_100BASE_T |
				LINK_FULL_100BASE_T);
	if (ret) {
		return ret;
	}

	return ret;
}

static void nxp_enet_phy_cb(const struct device *phy,
				struct phy_link_state *state,
				void *eth_dev)
{
	const struct device *dev = eth_dev;
	struct nxp_enet_mac_data *data = dev->data;

	if (!data->iface) {
		return;
	}

	if (!state->is_up) {
		net_eth_carrier_off(data->iface);
		nxp_enet_phy_reset_and_configure(phy);
	} else {
		net_eth_carrier_on(data->iface);
	}
}


static int nxp_enet_phy_init(const struct device *dev)
{
	const struct nxp_enet_mac_config *config = dev->config;
	int ret = 0;

	ret = nxp_enet_phy_reset_and_configure(config->phy_dev);
	if (ret) {
		return ret;
	}

	ret = phy_link_callback_set(config->phy_dev, nxp_enet_phy_cb, (void *)dev);
	if (ret) {
		return ret;
	}

	return ret;
}

/*
 ****************************
 * Callbacks and interrupts *
 ****************************
 */

static void eth_callback(ENET_Type *base, enet_handle_t *handle,
#if FSL_FEATURE_ENET_QUEUE > 1
			 uint32_t ringId,
#endif /* FSL_FEATURE_ENET_QUEUE > 1 */
			 enet_event_t event, enet_frame_info_t *frameinfo, void *param)
{
	const struct device *dev = param;
	const struct nxp_enet_mac_config *config = dev->config;
	struct nxp_enet_mac_data *data = dev->data;

	switch (event) {
	case kENET_RxEvent:
		k_sem_give(&data->rx_thread_sem);
		break;
	case kENET_TxEvent:
		/* Free the TX buffer. */
		k_sem_give(&data->tx_buf_sem);
		break;
	case kENET_ErrEvent:
		/* Error event: BABR/BABT/EBERR/LC/RL/UN/PLR.  */
		break;
	case kENET_WakeUpEvent:
		/* Wake up from sleep mode event. */
		break;
	case kENET_TimeStampEvent:
		/* Time stamp event.  */
		/* Reset periodic timer to default value. */
		config->base->ATPER = NSEC_PER_SEC;
		break;
	case kENET_TimeStampAvailEvent:
		/* Time stamp available event.  */
		break;
	}
}

static void eth_nxp_enet_isr(const struct device *dev)
{
	const struct nxp_enet_mac_config *config = dev->config;
	struct nxp_enet_mac_data *data = dev->data;
	unsigned int irq_lock_key = irq_lock();

	uint32_t eir = ENET_GetInterruptStatus(config->base);

	if (eir & (kENET_RxBufferInterrupt | kENET_RxFrameInterrupt)) {
#if FSL_FEATURE_ENET_QUEUE > 1
		/* Only use ring 0 in this driver */
		ENET_ReceiveIRQHandler(config->base, &data->enet_handle, 0);
#else
		ENET_ReceiveIRQHandler(config->base, &data->enet_handle);
#endif
		ENET_DisableInterrupts(config->base, kENET_RxFrameInterrupt |
			kENET_RxBufferInterrupt);
	}

	if (eir & kENET_TxFrameInterrupt) {
#if FSL_FEATURE_ENET_QUEUE > 1
		ENET_TransmitIRQHandler(config->base, &data->enet_handle, 0);
#else
		ENET_TransmitIRQHandler(config->base, &data->enet_handle);
#endif
	}

	if (eir & kENET_TxBufferInterrupt) {
		ENET_ClearInterruptStatus(config->base, kENET_TxBufferInterrupt);
		ENET_DisableInterrupts(config->base, kENET_TxBufferInterrupt);
	}

	if (eir & ENET_EIR_MII_MASK) {
		/* Callback to MDIO driver for relevant interrupt */
		nxp_enet_mdio_callback(config->mdio, nxp_enet_interrupt);
	}

	irq_unlock(irq_lock_key);
}


/*
 ******************
 * Initialization *
 ******************
 */

static int eth_nxp_enet_init(const struct device *dev)
{
	struct nxp_enet_mac_data *data = dev->data;
	const struct nxp_enet_mac_config *config = dev->config;
	enet_config_t enet_config;
	uint32_t enet_module_clock_rate;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	/* Initialize kernel objects */
	k_mutex_init(&data->rx_frame_buf_mutex);
	k_mutex_init(&data->tx_frame_buf_mutex);
	k_sem_init(&data->rx_thread_sem, 0, CONFIG_ETH_NXP_ENET_RX_BUFFERS);
	k_sem_init(&data->tx_buf_sem,
		   CONFIG_ETH_NXP_ENET_TX_BUFFERS, CONFIG_ETH_NXP_ENET_TX_BUFFERS);

	if (config->generate_mac) {
		config->generate_mac(data->mac_addr);
	}

	/* Start interruption-poll thread */
	k_thread_create(&data->rx_thread, data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(data->rx_thread_stack),
			eth_nxp_enet_rx_thread, (void *) dev, NULL, NULL,
			K_PRIO_COOP(2),
			0, K_NO_WAIT);
	k_thread_name_set(&data->rx_thread, "eth_nxp_enet_rx");

	/* Get ENET IP module clock rate */
	err = clock_control_get_rate(config->clock_dev, config->clock_subsys,
			&enet_module_clock_rate);
	if (err) {
		return err;
	}

	/* Use HAL to set up MAC configuration */
	ENET_GetDefaultConfig(&enet_config);

	if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
		enet_config.macSpecialConfig |= kENET_ControlPromiscuousEnable;
	}

	if (IS_ENABLED(CONFIG_NET_VLAN)) {
		enet_config.macSpecialConfig |= kENET_ControlVLANTagEnable;
	}

#if defined(CONFIG_ETH_NXP_ENET_HW_ACCELERATION)
	enet_config.txAccelerConfig |=
		kENET_TxAccelIpCheckEnabled | kENET_TxAccelProtoCheckEnabled;
	enet_config.rxAccelerConfig |=
		kENET_RxAccelIpCheckEnabled | kENET_RxAccelProtoCheckEnabled;
#endif

	enet_config.interrupt |= kENET_RxFrameInterrupt;
	enet_config.interrupt |= kENET_TxFrameInterrupt;

	if (config->phy_mode == NXP_ENET_MII_MODE) {
		enet_config.miiMode = kENET_MiiMode;
	} else if (config->phy_mode == NXP_ENET_RMII_MODE) {
		enet_config.miiMode = kENET_RmiiMode;
	} else {
		return -EINVAL;
	}

	enet_config.callback = eth_callback;
	enet_config.userData = (void *)dev;

	ENET_Init(config->base,
		  &data->enet_handle,
		  &enet_config,
		  &config->buffer_config,
		  data->mac_addr,
		  enet_module_clock_rate);

	nxp_enet_mdio_callback(config->mdio, nxp_enet_module_reset);

	ENET_ActiveRead(config->base);

	err = nxp_enet_phy_init(dev);
	if (err) {
		return err;
	}

	LOG_DBG("%s MAC %02x:%02x:%02x:%02x:%02x:%02x",
		dev->name,
		data->mac_addr[0], data->mac_addr[1],
		data->mac_addr[2], data->mac_addr[3],
		data->mac_addr[4], data->mac_addr[5]);

	return 0;
}

static const struct ethernet_api api_funcs = {
	.iface_api.init		= eth_nxp_enet_iface_init,
	.get_capabilities	= eth_nxp_enet_get_capabilities,
	.set_config		= eth_nxp_enet_set_config,
	.send			= eth_nxp_enet_tx,
#if defined(CONFIG_NET_DSA)
	.send                   = dsa_tx,
#else
};

#define NXP_ENET_CONNECT_IRQ(node_id, irq_names, idx)				\
	do {									\
		IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq),			\
				DT_IRQ_BY_IDX(node_id, idx, priority),		\
				eth_nxp_enet_isr,				\
				DEVICE_DT_GET(node_id),				\
				0);						\
		irq_enable(DT_IRQ_BY_IDX(node_id, idx, irq));			\
	} while (false);

#define FREESCALE_OUI_B0 0x00
#define FREESCALE_OUI_B1 0x04
#define FREESCALE_OUI_B2 0x9f

#if defined(CONFIG_SOC_SERIES_IMX_RT10XX)
#define ETH_NXP_ENET_UNIQUE_ID	(OCOTP->CFG1 ^ OCOTP->CFG2)
#elif defined(CONFIG_SOC_SERIES_IMX_RT11XX)
#define ETH_NXP_ENET_UNIQUE_ID	(OCOTP->FUSEN[40].FUSE)
#elif defined(CONFIG_SOC_SERIES_KINETIS_K6X)
#define ETH_NXP_ENET_UNIQUE_ID	(SIM->UIDH ^ SIM->UIDMH ^ SIM->UIDML ^ SIM->UIDL)
#else
#error "Unsupported SOC"
#endif

#define NXP_ENET_GENERATE_MAC_RANDOM(n)						\
	static void generate_eth_##n##_mac(uint8_t *mac_addr)			\
	{									\
		gen_random_mac(mac_addr,					\
			       FREESCALE_OUI_B0,				\
			       FREESCALE_OUI_B1,				\
			       FREESCALE_OUI_B2);				\
	}

#define NXP_ENET_GENERATE_MAC_UNIQUE(n)						\
	static void generate_eth_##n##_mac(uint8_t *mac_addr)			\
	{									\
		uint32_t id = ETH_NXP_ENET_UNIQUE_ID;				\
										\
		mac_addr[0] = FREESCALE_OUI_B0;					\
		mac_addr[0] |= 0x02; /* force LAA bit */			\
		mac_addr[1] = FREESCALE_OUI_B1;					\
		mac_addr[2] = FREESCALE_OUI_B2;					\
		mac_addr[3] = id >> 8;						\
		mac_addr[4] = id >> 16;						\
		mac_addr[5] = id >> 0;						\
		mac_addr[5] += n;						\
	}

#define NXP_ENET_GENERATE_MAC(n)						\
	COND_CODE_1(DT_INST_PROP(n, zephyr_random_mac_address),			\
		    (NXP_ENET_GENERATE_MAC_RANDOM(n)),				\
		    (NXP_ENET_GENERATE_MAC_UNIQUE(n)))

#define NXP_ENET_DECIDE_MAC_ADDR(n)						\
	COND_CODE_1(NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(n)),			\
			(NXP_ENET_MAC_ADDR_LOCAL(n)),				\
			(NXP_ENET_MAC_ADDR_GENERATED(n)))

#define NXP_ENET_DECIDE_MAC_GEN_FUNC(n)						\
	COND_CODE_1(NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(n)),			\
			(NXP_ENET_GEN_MAC_FUNCTION_NO(n)),			\
			(NXP_ENET_GEN_MAC_FUNCTION_YES(n)))

#define NXP_ENET_MAC_ADDR_LOCAL(n)						\
	.mac_addr = DT_INST_PROP(n, local_mac_address),

#define NXP_ENET_MAC_ADDR_GENERATED(n)						\
	.mac_addr = {0},

#define NXP_ENET_GEN_MAC_FUNCTION_NO(n)						\
	.generate_mac = NULL,

#define NXP_ENET_GEN_MAC_FUNCTION_YES(n)					\
	.generate_mac = generate_eth_##n##_mac,

#define NXP_ENET_DT_PHY_DEV(node_id, phy_phandle, idx)						\
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, phy_phandle, idx))

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay) && \
	CONFIG_ETH_NXP_ENET_USE_DTCM_FOR_DMA_BUFFER
/* Use DTCM for hardware DMA buffers */
#define _nxp_enet_dma_desc_section __dtcm_bss_section
#define _nxp_enet_dma_buffer_section __dtcm_noinit_section
#define _nxp_enet_driver_buffer_section __dtcm_noinit_section
#elif defined(CONFIG_NOCACHE_MEMORY)
#define _nxp_enet_dma_desc_section __nocache
#define _nxp_enet_dma_buffer_section __nocache
#define _nxp_enet_driver_buffer_section
#else
#define _nxp_enet_dma_desc_section
#define _nxp_enet_dma_buffer_section
#define _nxp_enet_driver_buffer_section
#endif

	/* Use ENET_FRAME_MAX_VLANFRAMELEN for VLAN frame size
	 * Use ENET_FRAME_MAX_FRAMELEN for Ethernet frame size
	 */
#if defined(CONFIG_NET_VLAN)
#if !defined(ENET_FRAME_MAX_VLANFRAMELEN)
#define ENET_FRAME_MAX_VLANFRAMELEN (ENET_FRAME_MAX_FRAMELEN + 4)
#endif
#define ETH_NXP_ENET_BUFFER_SIZE \
		ROUND_UP(ENET_FRAME_MAX_VLANFRAMELEN, ENET_BUFF_ALIGNMENT)
#else
#define ETH_NXP_ENET_BUFFER_SIZE \
		ROUND_UP(ENET_FRAME_MAX_FRAMELEN, ENET_BUFF_ALIGNMENT)
#endif /* CONFIG_NET_VLAN */

#define NXP_ENET_PHY_MODE(node_id)							\
	DT_ENUM_HAS_VALUE(node_id, phy_connection_type, mii) ? NXP_ENET_MII_MODE :	\
	(DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rmii) ? NXP_ENET_RMII_MODE :	\
	NXP_ENET_INVALID_MII_MODE)

#define NXP_ENET_INIT(n)								\
		NXP_ENET_GENERATE_MAC(n)						\
											\
		PINCTRL_DT_INST_DEFINE(n);						\
											\
		static void nxp_enet_##n##_irq_config_func(void)			\
		{									\
			DT_INST_FOREACH_PROP_ELEM(n, interrupt_names,			\
						NXP_ENET_CONNECT_IRQ);			\
		}									\
											\
		volatile static __aligned(ENET_BUFF_ALIGNMENT)				\
			_nxp_enet_dma_desc_section					\
			enet_rx_bd_struct_t						\
			nxp_enet_##n##_rx_buffer_desc[CONFIG_ETH_NXP_ENET_RX_BUFFERS];	\
											\
		volatile static __aligned(ENET_BUFF_ALIGNMENT)				\
			_nxp_enet_dma_desc_section					\
			enet_tx_bd_struct_t						\
			nxp_enet_##n##_tx_buffer_desc[CONFIG_ETH_NXP_ENET_TX_BUFFERS];	\
											\
		static uint8_t __aligned(ENET_BUFF_ALIGNMENT)				\
			_nxp_enet_dma_buffer_section					\
			nxp_enet_##n##_rx_buffer[CONFIG_ETH_NXP_ENET_RX_BUFFERS]	\
						[ETH_NXP_ENET_BUFFER_SIZE];		\
											\
		static uint8_t __aligned(ENET_BUFF_ALIGNMENT)				\
			_nxp_enet_dma_buffer_section					\
			nxp_enet_##n##_tx_buffer[CONFIG_ETH_NXP_ENET_TX_BUFFERS]	\
						[ETH_NXP_ENET_BUFFER_SIZE];		\
											\
		const struct nxp_enet_mac_config nxp_enet_##n##_config = {		\
			.base = (ENET_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),		\
			.irq_config_func = nxp_enet_##n##_irq_config_func,		\
			.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(n))),	\
			.clock_subsys = (void *)DT_CLOCKS_CELL_BY_IDX(			\
						DT_INST_PARENT(n), 0, name),		\
			.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
			.buffer_config = {						\
				.rxBdNumber = CONFIG_ETH_NXP_ENET_RX_BUFFERS,		\
				.txBdNumber = CONFIG_ETH_NXP_ENET_TX_BUFFERS,		\
				.rxBuffSizeAlign = ETH_NXP_ENET_BUFFER_SIZE,		\
				.txBuffSizeAlign = ETH_NXP_ENET_BUFFER_SIZE,		\
				.rxBdStartAddrAlign = nxp_enet_##n##_rx_buffer_desc,	\
				.txBdStartAddrAlign = nxp_enet_##n##_tx_buffer_desc,	\
				.rxBufferAlign = nxp_enet_##n##_rx_buffer[0],		\
				.txBufferAlign = nxp_enet_##n##_tx_buffer[0],		\
				.rxMaintainEnable = true,				\
				.txMaintainEnable = true,				\
				.txFrameInfo = NULL,					\
			},								\
			.phy_mode = NXP_ENET_PHY_MODE(DT_DRV_INST(n)),			\
			.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),	\
			.mdio = DEVICE_DT_GET(DT_INST_PHANDLE(n, nxp_mdio)),		\
			NXP_ENET_DECIDE_MAC_GEN_FUNC(n)					\
		};									\
											\
		static _nxp_enet_driver_buffer_section uint8_t				\
			nxp_enet_##n##_tx_frame_buf[NET_ETH_MAX_FRAME_SIZE];		\
		static _nxp_enet_driver_buffer_section uint8_t				\
			nxp_enet_##n##_rx_frame_buf[NET_ETH_MAX_FRAME_SIZE];		\
											\
		struct nxp_enet_mac_data nxp_enet_##n##_data = {			\
			NXP_ENET_DECIDE_MAC_ADDR(n)					\
			.tx_frame_buf = nxp_enet_##n##_tx_frame_buf,			\
			.rx_frame_buf = nxp_enet_##n##_rx_frame_buf,			\
		};									\
											\
		ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_nxp_enet_init, NULL,		\
					&nxp_enet_##n##_data, &nxp_enet_##n##_config,	\
					CONFIG_ETH_INIT_PRIORITY,			\
					&api_funcs, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_INIT)
