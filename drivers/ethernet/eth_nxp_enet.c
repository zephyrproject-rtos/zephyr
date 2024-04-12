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
#include <zephyr/drivers/ptp_clock.h>
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
#ifdef CONFIG_PTP_CLOCK_NXP_ENET
	const struct device *ptp_clock;
#endif
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
#ifdef CONFIG_PTP_CLOCK_NXP_ENET
	struct k_sem ptp_ts_sem;
	struct k_mutex *ptp_mutex; /* created in PTP driver */
#endif
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

static inline struct net_if *get_iface(struct nxp_enet_mac_data *data)
{
	return data->iface;
}

#if defined(CONFIG_PTP_CLOCK_NXP_ENET)
static bool eth_get_ptp_data(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_eth_vlan_hdr *hdr_vlan = (struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);
	bool pkt_is_ptp;

	if (net_eth_is_vlan_enabled(eth_ctx, iface)) {
		pkt_is_ptp = ntohs(hdr_vlan->type) == NET_ETH_PTYPE_PTP;
	} else {
		pkt_is_ptp = ntohs(NET_ETH_HDR(pkt)->type) == NET_ETH_PTYPE_PTP;
	}

	if (pkt_is_ptp) {
		net_pkt_set_priority(pkt, NET_PRIORITY_CA);
	}

	return pkt_is_ptp;
}


static inline void ts_register_tx_event(const struct device *dev,
					 enet_frame_info_t *frameinfo)
{
	struct nxp_enet_mac_data *data = dev->data;
	struct net_pkt *pkt = frameinfo->context;

	if (pkt && atomic_get(&pkt->atomic_ref) > 0) {
		if (eth_get_ptp_data(net_pkt_iface(pkt), pkt) && frameinfo->isTsAvail) {
			k_mutex_lock(data->ptp_mutex, K_FOREVER);

			pkt->timestamp.nanosecond = frameinfo->timeStamp.nanosecond;
			pkt->timestamp.second = frameinfo->timeStamp.second;

			net_if_add_tx_timestamp(pkt);
			k_sem_give(&data->ptp_ts_sem);

			k_mutex_unlock(data->ptp_mutex);
		}
		net_pkt_unref(pkt);
	}
}

static inline void eth_wait_for_ptp_ts(const struct device *dev, struct net_pkt *pkt)
{
	struct nxp_enet_mac_data *data = dev->data;

	net_pkt_ref(pkt);
	k_sem_take(&data->ptp_ts_sem, K_FOREVER);
}
#else
#define eth_get_ptp_data(...) false
#define ts_register_tx_event(...)
#define eth_wait_for_ptp_ts(...)
#endif /* CONFIG_PTP_CLOCK_NXP_ENET */

#ifdef CONFIG_PTP_CLOCK
static const struct device *eth_nxp_enet_get_ptp_clock(const struct device *dev)
{
	const struct nxp_enet_mac_config *config = dev->config;

	return config->ptp_clock;
}
#endif /* CONFIG_PTP_CLOCK */

/*
 *********************************
 * Ethernet driver API Functions *
 *********************************
 */

static int eth_nxp_enet_tx(const struct device *dev, struct net_pkt *pkt)
{
	const struct nxp_enet_mac_config *config = dev->config;
	struct nxp_enet_mac_data *data = dev->data;
	uint16_t total_len = net_pkt_get_len(pkt);
	bool frame_is_timestamped;
	status_t ret;

	/* Wait for a TX buffer descriptor to be available */
	k_sem_take(&data->tx_buf_sem, K_FOREVER);

	/* Enter critical section for TX frame buffer access */
	k_mutex_lock(&data->tx_frame_buf_mutex, K_FOREVER);

	/* Read network packet from upper layer into frame buffer */
	ret = net_pkt_read(pkt, data->tx_frame_buf, total_len);
	if (ret) {
		k_sem_give(&data->tx_buf_sem);
		goto exit;
	}

	frame_is_timestamped = eth_get_ptp_data(net_pkt_iface(pkt), pkt);

	ret = ENET_SendFrame(config->base, &data->enet_handle, data->tx_frame_buf,
			     total_len, RING_ID, frame_is_timestamped, pkt);
	if (ret == kStatus_Success) {
		goto exit;
	}

	if (frame_is_timestamped) {
		eth_wait_for_ptp_ts(dev, pkt);
	} else {
		LOG_ERR("ENET_SendFrame error: %d", ret);
		ENET_ReclaimTxDescriptor(config->base, &data->enet_handle, RING_ID);
	}

exit:
	/* Leave critical section for TX frame buffer access */
	k_mutex_unlock(&data->tx_frame_buf_mutex);

	return ret;
}

static void eth_nxp_enet_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct nxp_enet_mac_data *data = dev->data;
	const struct nxp_enet_mac_config *config = dev->config;

	net_if_set_link_addr(iface, data->mac_addr,
			     sizeof(data->mac_addr),
			     NET_LINK_ETHERNET);

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

	return ETHERNET_LINK_10BASE_T |
		ETHERNET_HW_FILTERING |
#if defined(CONFIG_NET_VLAN)
		ETHERNET_HW_VLAN |
#endif
#if defined(CONFIG_PTP_CLOCK_NXP_ENET)
		ETHERNET_PTP |
#endif
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
	case ETHERNET_CONFIG_TYPE_FILTER:
		/* The ENET driver does not modify the address buffer but the API is not const */
		if (cfg->filter.set) {
			ENET_AddMulticastGroup(config->base,
					       (uint8_t *)cfg->filter.mac_address.addr);
		} else {
			ENET_LeaveMulticastGroup(config->base,
						 (uint8_t *)cfg->filter.mac_address.addr);
		}
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
	uint32_t frame_length = 0U;
	struct net_if *iface;
	struct net_pkt *pkt = NULL;
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
		LOG_ERR("Frame too large (%d)", frame_length);
		goto flush;
	}

	/* Using root iface. It will be updated in net_recv_data() */
	pkt = net_pkt_rx_alloc_with_buffer(data->iface, frame_length,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		goto flush;
	}

	k_mutex_lock(&data->rx_frame_buf_mutex, K_FOREVER);
	status = ENET_ReadFrame(config->base, &data->enet_handle,
				data->rx_frame_buf, frame_length, RING_ID, &ts);
	k_mutex_unlock(&data->rx_frame_buf_mutex);

	if (status) {
		LOG_ERR("ENET_ReadFrame failed: %d", (int)status);
		goto error;
	}

	if (net_pkt_write(pkt, data->rx_frame_buf, frame_length)) {
		LOG_ERR("Unable to write frame into the packet");
		goto error;
	}

#if defined(CONFIG_PTP_CLOCK_NXP_ENET)
	k_mutex_lock(data->ptp_mutex, K_FOREVER);

	/* Invalid value by default. */
	pkt->timestamp.nanosecond = UINT32_MAX;
	pkt->timestamp.second = UINT64_MAX;

	/* Timestamp the packet using PTP clock */
	if (eth_get_ptp_data(get_iface(data), pkt)) {
		struct net_ptp_time ptp_time;

		ptp_clock_get(config->ptp_clock, &ptp_time);

		/* If latest timestamp reloads after getting from Rx BD,
		 * then second - 1 to make sure the actual Rx timestamp is accurate
		 */
		if (ptp_time.nanosecond < ts) {
			ptp_time.second--;
		}

		pkt->timestamp.nanosecond = ts;
		pkt->timestamp.second = ptp_time.second;
	}
	k_mutex_unlock(data->ptp_mutex);
#endif /* CONFIG_PTP_CLOCK_NXP_ENET */

	iface = get_iface(data);
#if defined(CONFIG_NET_DSA)
	iface = dsa_net_recv(iface, &pkt);
#endif
	if (net_recv_data(iface, pkt) < 0) {
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
	if (pkt) {
		net_pkt_unref(pkt);
	}
	eth_stats_update_errors_rx(get_iface(data));
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
	int ret;

	/* Reset the PHY */
	ret = phy_write(phy, MII_BMCR, MII_BMCR_RESET);
	if (ret) {
		return ret;
	}

	/* 802.3u standard says reset takes up to 0.5s */
	k_busy_wait(500000);

	/* Configure the PHY */
	return phy_configure_link(phy, LINK_HALF_10BASE_T | LINK_FULL_10BASE_T |
				       LINK_HALF_100BASE_T | LINK_FULL_100BASE_T);
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

	LOG_INF("Link is %s", state->is_up ? "up" : "down");
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

void nxp_enet_driver_cb(const struct device *dev, enum nxp_enet_driver dev_type,
				enum nxp_enet_callback_reason event, void *data)
{
	if (dev_type == NXP_ENET_MDIO) {
		nxp_enet_mdio_callback(dev, event, data);
	} else if (dev_type == NXP_ENET_PTP_CLOCK) {
		nxp_enet_ptp_clock_callback(dev, event, data);
	}
}

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
		ts_register_tx_event(dev, frameinfo);
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

#if FSL_FEATURE_ENET_QUEUE > 1
#define ENET_IRQ_HANDLER_ARGS(base, handle) base, handle, 0
#else
#define ENET_IRQ_HANDLER_ARGS(base, handle) base, handle
#endif /* FSL_FEATURE_ENET_QUEUE > 1 */

static void eth_nxp_enet_isr(const struct device *dev)
{
	const struct nxp_enet_mac_config *config = dev->config;
	struct nxp_enet_mac_data *data = dev->data;
	unsigned int irq_lock_key = irq_lock();

	uint32_t eir = ENET_GetInterruptStatus(config->base);

	if (eir & (kENET_RxBufferInterrupt | kENET_RxFrameInterrupt)) {
		ENET_ReceiveIRQHandler(ENET_IRQ_HANDLER_ARGS(config->base, &data->enet_handle));
		ENET_DisableInterrupts(config->base,
				kENET_RxFrameInterrupt | kENET_RxBufferInterrupt);
	}

	if (eir & kENET_TxFrameInterrupt) {
		ENET_TransmitIRQHandler(ENET_IRQ_HANDLER_ARGS(config->base, &data->enet_handle));
	}

	if (eir & kENET_TxBufferInterrupt) {
		ENET_ClearInterruptStatus(config->base, kENET_TxBufferInterrupt);
		ENET_DisableInterrupts(config->base, kENET_TxBufferInterrupt);
	}

	if (eir & ENET_EIR_MII_MASK) {
		nxp_enet_driver_cb(config->mdio, NXP_ENET_MDIO, NXP_ENET_INTERRUPT, NULL);
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
#if defined(CONFIG_PTP_CLOCK_NXP_ENET)
	k_sem_init(&data->ptp_ts_sem, 0, 1);
#endif

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

	if (IS_ENABLED(CONFIG_ETH_NXP_ENET_HW_ACCELERATION)) {
		enet_config.txAccelerConfig |=
			kENET_TxAccelIpCheckEnabled | kENET_TxAccelProtoCheckEnabled;
		enet_config.rxAccelerConfig |=
			kENET_RxAccelIpCheckEnabled | kENET_RxAccelProtoCheckEnabled;
	}

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

	ENET_Up(config->base,
		  &data->enet_handle,
		  &enet_config,
		  &config->buffer_config,
		  data->mac_addr,
		  enet_module_clock_rate);

	nxp_enet_driver_cb(config->mdio, NXP_ENET_MDIO, NXP_ENET_MODULE_RESET, NULL);

#if defined(CONFIG_PTP_CLOCK_NXP_ENET)
	nxp_enet_driver_cb(config->ptp_clock, NXP_ENET_PTP_CLOCK,
				NXP_ENET_MODULE_RESET, &data->ptp_mutex);
	ENET_SetTxReclaim(&data->enet_handle, true, 0);
#endif

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

#ifdef CONFIG_NET_DSA
#define NXP_ENET_SEND_FUNC dsa_tx
#else
#define NXP_ENET_SEND_FUNC eth_nxp_enet_tx
#endif /* CONFIG_NET_DSA */

static const struct ethernet_api api_funcs = {
	.iface_api.init		= eth_nxp_enet_iface_init,
	.get_capabilities	= eth_nxp_enet_get_capabilities,
	.set_config		= eth_nxp_enet_set_config,
	.send			= NXP_ENET_SEND_FUNC,
#if defined(CONFIG_PTP_CLOCK)
	.get_ptp_clock		= eth_nxp_enet_get_ptp_clock,
#endif
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

#if defined(CONFIG_SOC_SERIES_IMXRT10XX)
#define ETH_NXP_ENET_UNIQUE_ID	(OCOTP->CFG1 ^ OCOTP->CFG2)
#elif defined(CONFIG_SOC_SERIES_IMXRT11XX)
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

#ifdef CONFIG_PTP_CLOCK_NXP_ENET
#define NXP_ENET_PTP_DEV(n) .ptp_clock = DEVICE_DT_GET(DT_INST_PHANDLE(n, nxp_ptp_clock)),
#define NXP_ENET_FRAMEINFO_ARRAY(n)							\
	static enet_frame_info_t							\
		nxp_enet_##n##_tx_frameinfo_array[CONFIG_ETH_NXP_ENET_TX_BUFFERS];
#define NXP_ENET_FRAMEINFO(n)	\
	.txFrameInfo = nxp_enet_##n##_tx_frameinfo_array,
#else
#define NXP_ENET_PTP_DEV(n)
#define NXP_ENET_FRAMEINFO_ARRAY(n)
#define NXP_ENET_FRAMEINFO(n)	\
	.txFrameInfo = NULL
#endif

#define NXP_ENET_MAC_INIT(n)								\
		NXP_ENET_GENERATE_MAC(n)						\
											\
		PINCTRL_DT_INST_DEFINE(n);						\
											\
		NXP_ENET_FRAMEINFO_ARRAY(n)						\
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
				NXP_ENET_FRAMEINFO(n)					\
			},								\
			.phy_mode = NXP_ENET_PHY_MODE(DT_DRV_INST(n)),			\
			.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),	\
			.mdio = DEVICE_DT_GET(DT_INST_PHANDLE(n, nxp_mdio)),		\
			NXP_ENET_PTP_DEV(n)						\
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

DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_MAC_INIT)

/*
 * ENET module-level management
 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_enet

#define NXP_ENET_INIT(n)								\
											\
int nxp_enet_##n##_init(void)								\
{											\
	clock_control_on(DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),				\
			(void *)DT_INST_CLOCKS_CELL_BY_IDX(n, 0, name));		\
											\
	ENET_Reset((ENET_Type *)DT_INST_REG_ADDR(n));					\
											\
	return 0;									\
}											\
											\
	/* Init the module before any of the MAC, MDIO, or PTP clock */			\
	SYS_INIT(nxp_enet_##n##_init, POST_KERNEL, 0);

DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_INIT)
