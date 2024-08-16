/* NXP ENET MAC Driver
 *
 * Copyright 2023-2024 NXP
 *
 * Inspiration from eth_mcux.c, which was:
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

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/kernel/thread_stack.h>

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <ethernet/eth_stats.h>

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

#ifdef CONFIG_PTP_CLOCK
#include <zephyr/drivers/ptp_clock.h>
#endif

#ifdef CONFIG_NET_DSA
#include <zephyr/net/dsa.h>
#endif

#if defined(CONFIG_NET_POWER_MANAGEMENT) && defined(CONFIG_PM_DEVICE)
#include <zephyr/pm/device.h>
#endif

#include "../eth.h"
#include <zephyr/drivers/ethernet/eth_nxp_enet.h>
#include <zephyr/dt-bindings/ethernet/nxp_enet.h>
#include <fsl_enet.h>

#define FREESCALE_OUI_B0 0x00
#define FREESCALE_OUI_B1 0x04
#define FREESCALE_OUI_B2 0x9f

#if defined(CONFIG_SOC_SERIES_IMXRT10XX)
#define ETH_NXP_ENET_UNIQUE_ID	(OCOTP->CFG1 ^ OCOTP->CFG2)
#elif defined(CONFIG_SOC_SERIES_IMXRT11XX)
#define ETH_NXP_ENET_UNIQUE_ID	(OCOTP->FUSEN[40].FUSE)
#elif defined(CONFIG_SOC_SERIES_KINETIS_K6X)
#define ETH_NXP_ENET_UNIQUE_ID	(SIM->UIDH ^ SIM->UIDMH ^ SIM->UIDML ^ SIM->UIDL)
#elif defined(CONFIG_SOC_SERIES_RW6XX)
#define ETH_NXP_ENET_UNIQUE_ID	(OCOTP->OTP_SHADOW[46])
#else
#define ETH_NXP_ENET_UNIQUE_ID 0xFFFFFF
#endif

#define RING_ID 0

enum mac_address_source {
	MAC_ADDR_SOURCE_LOCAL,
	MAC_ADDR_SOURCE_RANDOM,
	MAC_ADDR_SOURCE_UNIQUE,
	MAC_ADDR_SOURCE_FUSED,
	MAC_ADDR_SOURCE_INVALID,
};

struct nxp_enet_mac_config {
	const struct device *module_dev;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	enum mac_address_source mac_addr_source;
	const struct pinctrl_dev_config *pincfg;
	enet_buffer_config_t buffer_config[1];
	uint8_t phy_mode;
	void (*irq_config_func)(void);
	const struct device *phy_dev;
	const struct device *mdio;
#ifdef CONFIG_PTP_CLOCK_NXP_ENET
	const struct device *ptp_clock;
#endif
};

struct nxp_enet_mac_data {
	ENET_Type *base;
	struct net_if *iface;
	uint8_t mac_addr[6];
	enet_handle_t enet_handle;
	struct k_sem tx_buf_sem;
	struct k_work rx_work;
	const struct device *dev;
	struct k_sem rx_thread_sem;
	struct k_mutex tx_frame_buf_mutex;
	struct k_mutex rx_frame_buf_mutex;
#ifdef CONFIG_PTP_CLOCK_NXP_ENET
	struct k_sem ptp_ts_sem;
	struct k_mutex *ptp_mutex; /* created in PTP driver */
#endif
	uint8_t *tx_frame_buf;
	uint8_t *rx_frame_buf;
};

static K_THREAD_STACK_DEFINE(enet_rx_stack, CONFIG_ETH_NXP_ENET_RX_THREAD_STACK_SIZE);
static struct k_work_q rx_work_queue;

static int rx_queue_init(void)
{
	struct k_work_queue_config cfg = {.name = "ENET_RX"};

	k_work_queue_init(&rx_work_queue);
	k_work_queue_start(&rx_work_queue, enet_rx_stack,
			   K_THREAD_STACK_SIZEOF(enet_rx_stack),
			   K_PRIO_COOP(CONFIG_ETH_NXP_ENET_RX_THREAD_PRIORITY),
			   &cfg);

	return 0;
}

SYS_INIT(rx_queue_init, POST_KERNEL, 0);

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

static int eth_nxp_enet_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct nxp_enet_mac_data *data = dev->data;
	uint16_t total_len = net_pkt_get_len(pkt);
	bool frame_is_timestamped;
	status_t ret;

	/* Wait for a TX buffer descriptor to be available */
	k_sem_take(&data->tx_buf_sem, K_FOREVER);

	/* Enter critical section for TX frame buffer access */
	k_mutex_lock(&data->tx_frame_buf_mutex, K_FOREVER);

	ret = net_pkt_read(pkt, data->tx_frame_buf, total_len);
	if (ret) {
		k_sem_give(&data->tx_buf_sem);
		goto exit;
	}

	frame_is_timestamped = eth_get_ptp_data(net_pkt_iface(pkt), pkt);

	ret = ENET_SendFrame(data->base, &data->enet_handle, data->tx_frame_buf,
			     total_len, RING_ID, frame_is_timestamped, pkt);
	if (ret == kStatus_Success) {
		goto exit;
	}

	if (frame_is_timestamped) {
		eth_wait_for_ptp_ts(dev, pkt);
	} else {
		LOG_ERR("ENET_SendFrame error: %d", ret);
		ENET_ReclaimTxDescriptor(data->base, &data->enet_handle, RING_ID);
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

	nxp_enet_driver_cb(config->mdio, NXP_ENET_MDIO, NXP_ENET_INTERRUPT_ENABLED, NULL);
}

static enum ethernet_hw_caps eth_nxp_enet_get_capabilities(const struct device *dev)
{
#if defined(CONFIG_ETH_NXP_ENET_1G)
	const struct nxp_enet_mac_config *config = dev->config;
#else
	ARG_UNUSED(dev);
#endif
	enum ethernet_hw_caps caps;

	caps = ETHERNET_LINK_10BASE_T |
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

	if (COND_CODE_1(IS_ENABLED(CONFIG_ETH_NXP_ENET_1G),
	   (config->phy_mode == NXP_ENET_RGMII_MODE), (0))) {
		caps |= ETHERNET_LINK_1000BASE_T;
	}

	return caps;
}

static int eth_nxp_enet_set_config(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *cfg)
{
	struct nxp_enet_mac_data *data = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr,
		       cfg->mac_address.addr,
		       sizeof(data->mac_addr));
		ENET_SetMacAddr(data->base, data->mac_addr);
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
			ENET_AddMulticastGroup(data->base,
					       (uint8_t *)cfg->filter.mac_address.addr);
		} else {
			ENET_LeaveMulticastGroup(data->base,
						 (uint8_t *)cfg->filter.mac_address.addr);
		}
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static int eth_nxp_enet_rx(const struct device *dev)
{
#if defined(CONFIG_PTP_CLOCK_NXP_ENET)
	const struct nxp_enet_mac_config *config = dev->config;
#endif
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
	status = ENET_ReadFrame(data->base, &data->enet_handle,
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
	status = ENET_ReadFrame(data->base, &data->enet_handle, NULL,
					0, RING_ID, NULL);
	__ASSERT_NO_MSG(status == kStatus_Success);
error:
	if (pkt) {
		net_pkt_unref(pkt);
	}
	eth_stats_update_errors_rx(get_iface(data));
	return -EIO;
}

static void eth_nxp_enet_rx_thread(struct k_work *work)
{
	struct nxp_enet_mac_data *data =
		CONTAINER_OF(work, struct nxp_enet_mac_data, rx_work);
	const struct device *dev = data->dev;
	int ret;

	if (k_sem_take(&data->rx_thread_sem, K_FOREVER)) {
		return;
	}

	do {
		ret = eth_nxp_enet_rx(dev);
	} while (ret == 1);

	ENET_EnableInterrupts(data->base, kENET_RxFrameInterrupt);
}

static int nxp_enet_phy_configure(const struct device *phy, uint8_t phy_mode)
{
	enum phy_link_speed speeds = LINK_HALF_10BASE_T | LINK_FULL_10BASE_T |
				       LINK_HALF_100BASE_T | LINK_FULL_100BASE_T;

	if (COND_CODE_1(IS_ENABLED(CONFIG_ETH_NXP_ENET_1G),
	   (phy_mode == NXP_ENET_RGMII_MODE), (0))) {
		speeds |= (LINK_HALF_1000BASE_T | LINK_FULL_1000BASE_T);
	}

	/* Configure the PHY */
	return phy_configure_link(phy, speeds);
}

static void nxp_enet_phy_cb(const struct device *phy,
				struct phy_link_state *state,
				void *eth_dev)
{
	const struct device *dev = eth_dev;
	struct nxp_enet_mac_data *data = dev->data;
	const struct nxp_enet_mac_config *config = dev->config;
	enet_mii_speed_t speed;
	enet_mii_duplex_t duplex;

	if (state->is_up) {
#if defined(CONFIG_ETH_NXP_ENET_1G)
		if (PHY_LINK_IS_SPEED_1000M(state->speed)) {
			speed = kENET_MiiSpeed1000M;
		} else if (PHY_LINK_IS_SPEED_100M(state->speed)) {
#else
		if (PHY_LINK_IS_SPEED_100M(state->speed)) {
#endif
			speed = kENET_MiiSpeed100M;
		} else {
			speed = kENET_MiiSpeed10M;
		}

		if (PHY_LINK_IS_FULL_DUPLEX(state->speed)) {
			duplex = kENET_MiiFullDuplex;
		} else {
			duplex = kENET_MiiHalfDuplex;
		}

		ENET_SetMII(data->base, speed, duplex);
	}

	if (!data->iface) {
		return;
	}

	LOG_INF("Link is %s", state->is_up ? "up" : "down");

	if (!state->is_up) {
		net_eth_carrier_off(data->iface);
		nxp_enet_phy_configure(phy, config->phy_mode);
	} else {
		net_eth_carrier_on(data->iface);
	}
}


static int nxp_enet_phy_init(const struct device *dev)
{
	const struct nxp_enet_mac_config *config = dev->config;
	int ret = 0;

	ret = nxp_enet_phy_configure(config->phy_dev, config->phy_mode);
	if (ret) {
		return ret;
	}

	ret = phy_link_callback_set(config->phy_dev, nxp_enet_phy_cb, (void *)dev);
	if (ret) {
		return ret;
	}

	return ret;
}

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
	struct nxp_enet_mac_data *data = dev->data;

	switch (event) {
	case kENET_RxEvent:
		k_sem_give(&data->rx_thread_sem);
		break;
	case kENET_TxEvent:
		ts_register_tx_event(dev, frameinfo);
		k_sem_give(&data->tx_buf_sem);
		break;
	case kENET_TimeStampEvent:
		/* Reset periodic timer to default value. */
		data->base->ATPER = NSEC_PER_SEC;
		break;
	default:
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

	uint32_t eir = ENET_GetInterruptStatus(data->base);

	if (eir & (kENET_RxFrameInterrupt)) {
		ENET_ReceiveIRQHandler(ENET_IRQ_HANDLER_ARGS(data->base, &data->enet_handle));
		ENET_DisableInterrupts(data->base, kENET_RxFrameInterrupt);
		k_work_submit_to_queue(&rx_work_queue, &data->rx_work);
	}

	if (eir & kENET_TxFrameInterrupt) {
		ENET_TransmitIRQHandler(ENET_IRQ_HANDLER_ARGS(data->base, &data->enet_handle));
	}

	if (eir & ENET_EIR_MII_MASK) {
		nxp_enet_driver_cb(config->mdio, NXP_ENET_MDIO, NXP_ENET_INTERRUPT, NULL);
	}

	irq_unlock(irq_lock_key);
}

static const struct device *eth_nxp_enet_get_phy(const struct device *dev)
{
	const struct nxp_enet_mac_config *config = dev->config;

	return config->phy_dev;
}

/* Note this is not universally unique, it just is probably unique on a network */
static inline void nxp_enet_unique_mac(uint8_t *mac_addr)
{
	uint32_t id = ETH_NXP_ENET_UNIQUE_ID;

	if (id == 0xFFFFFF) {
		LOG_ERR("No unique MAC can be provided in this platform");
	}

	/* Setting LAA bit because it is not guaranteed universally unique */
	mac_addr[0] = FREESCALE_OUI_B0 | 0x02;
	mac_addr[1] = FREESCALE_OUI_B1;
	mac_addr[2] = FREESCALE_OUI_B2;
	mac_addr[3] = FIELD_GET(0xFF0000, id);
	mac_addr[4] = FIELD_GET(0x00FF00, id);
	mac_addr[5] = FIELD_GET(0x0000FF, id);
}

#ifdef CONFIG_SOC_FAMILY_NXP_IMXRT
#include <fsl_ocotp.h>
#endif

static inline void nxp_enet_fused_mac(uint8_t *mac_addr)
{
#ifdef CONFIG_SOC_FAMILY_NXP_IMXRT
	uint32_t mac_addr_fuse[2] = {0};

#if defined(CONFIG_SOC_SERIES_IMXRT10XX)
	OCOTP_Init((OCOTP_Type *)OCOTP_BASE, CLOCK_GetIpgFreq());
	/* OTP bank 4, word 2: MAC0 */
	OCOTP_ReadFuseShadowRegisterExt((OCOTP_Type *)OCOTP_BASE,
		0x22, &mac_addr_fuse[0], 1);
	/* OTP bank 4, word 3: MAC1*/
	OCOTP_ReadFuseShadowRegisterExt((OCOTP_Type *)OCOTP_BASE,
		0x23, &mac_addr_fuse[1], 1);
#elif defined(CONFIG_SOC_SERIES_IMXRT11XX)
	OCOTP_Init((OCOTP_Type *)OCOTP_BASE, 0);
	OCOTP_ReadFuseShadowRegisterExt((OCOTP_Type *)OCOTP_BASE,
		0x28, &mac_addr_fuse[0], 2);
#endif
	mac_addr[0] = mac_addr_fuse[0] & 0x000000FF;
	mac_addr[1] = (mac_addr_fuse[0] & 0x0000FF00) >> 8;
	mac_addr[2] = (mac_addr_fuse[0] & 0x00FF0000) >> 16;
	mac_addr[3] = (mac_addr_fuse[0] & 0xFF000000) >> 24;
	mac_addr[4] = (mac_addr_fuse[1] & 0x00FF);
	mac_addr[5] = (mac_addr_fuse[1] & 0xFF00) >> 8;
#else
	ARG_UNUSED(mac_addr);
#endif
}

static int eth_nxp_enet_init(const struct device *dev)
{
	struct nxp_enet_mac_data *data = dev->data;
	const struct nxp_enet_mac_config *config = dev->config;
	enet_config_t enet_config;
	uint32_t enet_module_clock_rate;
	int err;

	data->base = (ENET_Type *)DEVICE_MMIO_GET(config->module_dev);

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	k_mutex_init(&data->rx_frame_buf_mutex);
	k_mutex_init(&data->tx_frame_buf_mutex);
	k_sem_init(&data->rx_thread_sem, 0, CONFIG_ETH_NXP_ENET_RX_BUFFERS);
	k_sem_init(&data->tx_buf_sem,
		   CONFIG_ETH_NXP_ENET_TX_BUFFERS, CONFIG_ETH_NXP_ENET_TX_BUFFERS);
#if defined(CONFIG_PTP_CLOCK_NXP_ENET)
	k_sem_init(&data->ptp_ts_sem, 0, 1);
#endif
	k_work_init(&data->rx_work, eth_nxp_enet_rx_thread);

	switch (config->mac_addr_source) {
	case MAC_ADDR_SOURCE_LOCAL:
		break;
	case MAC_ADDR_SOURCE_RANDOM:
		gen_random_mac(data->mac_addr,
			FREESCALE_OUI_B0, FREESCALE_OUI_B1, FREESCALE_OUI_B2);
		break;
	case MAC_ADDR_SOURCE_UNIQUE:
		nxp_enet_unique_mac(data->mac_addr);
		break;
	case MAC_ADDR_SOURCE_FUSED:
		nxp_enet_fused_mac(data->mac_addr);
		break;
	default:
		return -ENOTSUP;
	}

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys,
			&enet_module_clock_rate);
	if (err) {
		return err;
	}

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
#if defined(CONFIG_ETH_NXP_ENET_1G)
	} else if (config->phy_mode == NXP_ENET_RGMII_MODE) {
		enet_config.miiMode = kENET_RgmiiMode;
#endif
	} else {
		return -EINVAL;
	}

	enet_config.callback = eth_callback;
	enet_config.userData = (void *)dev;

	ENET_Up(data->base,
		  &data->enet_handle,
		  &enet_config,
		  config->buffer_config,
		  data->mac_addr,
		  enet_module_clock_rate);

	nxp_enet_driver_cb(config->mdio, NXP_ENET_MDIO, NXP_ENET_MODULE_RESET, NULL);

#if defined(CONFIG_PTP_CLOCK_NXP_ENET)
	nxp_enet_driver_cb(config->ptp_clock, NXP_ENET_PTP_CLOCK,
				NXP_ENET_MODULE_RESET, &data->ptp_mutex);
	ENET_SetTxReclaim(&data->enet_handle, true, 0);
#endif

	ENET_ActiveRead(data->base);

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

#if defined(CONFIG_NET_POWER_MANAGEMENT)
static int eth_nxp_enet_device_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct nxp_enet_mac_config *config = dev->config;
	struct nxp_enet_mac_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		LOG_DBG("Suspending");

		ret = net_if_suspend(data->iface);
		if (ret) {
			return ret;
		}

		ENET_Reset(data->base);
		ENET_Down(data->base);
		clock_control_off(config->clock_dev, (clock_control_subsys_t)config->clock_subsys);
	} else if (action == PM_DEVICE_ACTION_RESUME) {
		LOG_DBG("Resuming");

		clock_control_on(config->clock_dev, (clock_control_subsys_t)config->clock_subsys);
		eth_nxp_enet_init(dev);
		net_if_resume(data->iface);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

#define ETH_NXP_ENET_PM_DEVICE_INIT(n)	\
	PM_DEVICE_DT_INST_DEFINE(n, eth_nxp_enet_device_pm_action);
#define ETH_NXP_ENET_PM_DEVICE_GET(n) PM_DEVICE_DT_INST_GET(n)

#else
#define ETH_NXP_ENET_PM_DEVICE_INIT(n)
#define ETH_NXP_ENET_PM_DEVICE_GET(n) NULL
#endif /* CONFIG_NET_POWER_MANAGEMENT */

#ifdef CONFIG_NET_DSA
#define NXP_ENET_SEND_FUNC dsa_tx
#else
#define NXP_ENET_SEND_FUNC eth_nxp_enet_tx
#endif /* CONFIG_NET_DSA */

static const struct ethernet_api api_funcs = {
	.iface_api.init		= eth_nxp_enet_iface_init,
	.get_capabilities	= eth_nxp_enet_get_capabilities,
	.get_phy                = eth_nxp_enet_get_phy,
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

#define NXP_ENET_DT_PHY_DEV(node_id, phy_phandle, idx)						\
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, phy_phandle, idx))

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay) && \
	CONFIG_ETH_NXP_ENET_USE_DTCM_FOR_DMA_BUFFER
#define _nxp_enet_dma_desc_section __dtcm_bss_section
#define _nxp_enet_dma_buffer_section __dtcm_noinit_section
#define _nxp_enet_driver_buffer_section __dtcm_noinit_section
#define driver_cache_maintain	false
#elif defined(CONFIG_NOCACHE_MEMORY)
#define _nxp_enet_dma_desc_section __nocache
#define _nxp_enet_dma_buffer_section __nocache
#define _nxp_enet_driver_buffer_section
#define driver_cache_maintain	false
#else
#define _nxp_enet_dma_desc_section
#define _nxp_enet_dma_buffer_section
#define _nxp_enet_driver_buffer_section
#define driver_cache_maintain	true
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
	(DT_ENUM_HAS_VALUE(node_id, phy_connection_type, rgmii) ? NXP_ENET_RGMII_MODE :	\
	NXP_ENET_INVALID_MII_MODE))

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

#define NXP_ENET_NODE_HAS_MAC_ADDR_CHECK(n)						\
	BUILD_ASSERT(NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(n)) ||				\
			DT_INST_PROP(n, zephyr_random_mac_address) ||			\
			DT_INST_PROP(n, nxp_unique_mac) ||				\
			DT_INST_PROP(n, nxp_fused_mac),					\
			"MAC address not specified on ENET DT node");

#define NXP_ENET_NODE_PHY_MODE_CHECK(n)							\
BUILD_ASSERT(NXP_ENET_PHY_MODE(DT_DRV_INST(n)) != NXP_ENET_RGMII_MODE ||		\
			(IS_ENABLED(CONFIG_ETH_NXP_ENET_1G) &&				\
			DT_NODE_HAS_COMPAT(DT_INST_PARENT(n), nxp_enet1g)),		\
			"RGMII mode requires nxp,enet1g compatible on ENET DT node"	\
			" and CONFIG_ETH_NXP_ENET_1G enabled");

#define NXP_ENET_MAC_ADDR_SOURCE(n)							\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), local_mac_address),		\
			(MAC_ADDR_SOURCE_LOCAL),					\
	(COND_CODE_1(DT_INST_PROP(n, zephyr_random_mac_address),			\
			(MAC_ADDR_SOURCE_RANDOM),					\
	(COND_CODE_1(DT_INST_PROP(n, nxp_unique_mac), (MAC_ADDR_SOURCE_UNIQUE),		\
	(COND_CODE_1(DT_INST_PROP(n, nxp_fused_mac), (MAC_ADDR_SOURCE_FUSED),		\
	(MAC_ADDR_SOURCE_INVALID))))))))

#define NXP_ENET_MAC_INIT(n)								\
		NXP_ENET_NODE_HAS_MAC_ADDR_CHECK(n)					\
											\
		NXP_ENET_NODE_PHY_MODE_CHECK(n)						\
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
			.irq_config_func = nxp_enet_##n##_irq_config_func,		\
			.module_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),			\
			.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(n))),	\
			.clock_subsys = (void *)DT_CLOCKS_CELL_BY_IDX(			\
						DT_INST_PARENT(n), 0, name),		\
			.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
			.buffer_config = {{						\
				.rxBdNumber = CONFIG_ETH_NXP_ENET_RX_BUFFERS,		\
				.txBdNumber = CONFIG_ETH_NXP_ENET_TX_BUFFERS,		\
				.rxBuffSizeAlign = ETH_NXP_ENET_BUFFER_SIZE,		\
				.txBuffSizeAlign = ETH_NXP_ENET_BUFFER_SIZE,		\
				.rxBdStartAddrAlign = nxp_enet_##n##_rx_buffer_desc,	\
				.txBdStartAddrAlign = nxp_enet_##n##_tx_buffer_desc,	\
				.rxBufferAlign = nxp_enet_##n##_rx_buffer[0],		\
				.txBufferAlign = nxp_enet_##n##_tx_buffer[0],		\
				.rxMaintainEnable = driver_cache_maintain,		\
				.txMaintainEnable = driver_cache_maintain,		\
				NXP_ENET_FRAMEINFO(n)					\
			}},								\
			.phy_mode = NXP_ENET_PHY_MODE(DT_DRV_INST(n)),			\
			.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, phy_handle)),	\
			.mdio = DEVICE_DT_GET(DT_INST_PHANDLE(n, nxp_mdio)),		\
			NXP_ENET_PTP_DEV(n)						\
			.mac_addr_source = NXP_ENET_MAC_ADDR_SOURCE(n),			\
		};									\
											\
		static _nxp_enet_driver_buffer_section uint8_t				\
			nxp_enet_##n##_tx_frame_buf[NET_ETH_MAX_FRAME_SIZE];		\
		static _nxp_enet_driver_buffer_section uint8_t				\
			nxp_enet_##n##_rx_frame_buf[NET_ETH_MAX_FRAME_SIZE];		\
											\
		struct nxp_enet_mac_data nxp_enet_##n##_data = {			\
			.tx_frame_buf = nxp_enet_##n##_tx_frame_buf,			\
			.rx_frame_buf = nxp_enet_##n##_rx_frame_buf,			\
			.dev = DEVICE_DT_INST_GET(n),					\
			.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),		\
		};									\
											\
		ETH_NXP_ENET_PM_DEVICE_INIT(n)						\
											\
		ETH_NET_DEVICE_DT_INST_DEFINE(n, eth_nxp_enet_init,			\
					ETH_NXP_ENET_PM_DEVICE_GET(n),			\
					&nxp_enet_##n##_data, &nxp_enet_##n##_config,	\
					CONFIG_ETH_INIT_PRIORITY,			\
					&api_funcs, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(NXP_ENET_MAC_INIT)

struct nxp_enet_mod_config {
	DEVICE_MMIO_ROM;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct nxp_enet_mod_data {
	DEVICE_MMIO_RAM;
};

static int nxp_enet_mod_init(const struct device *dev)
{
	const struct nxp_enet_mod_config *config = dev->config;
	int ret;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret) {
		LOG_ERR("ENET module clock error");
		return ret;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	ENET_Reset((ENET_Type *)DEVICE_MMIO_GET(dev));

	return 0;
}

#define NXP_ENET_INIT(n, compat)							\
											\
static const struct nxp_enet_mod_config nxp_enet_mod_cfg_##n = {			\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),					\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_DRV_INST(n))),		\
		.clock_subsys = (void *) DT_CLOCKS_CELL_BY_IDX(				\
							DT_DRV_INST(n), 0, name),	\
};											\
											\
static struct nxp_enet_mod_data nxp_enet_mod_data_##n;					\
											\
/* Init the module before any of the MAC, MDIO, or PTP clock */				\
DEVICE_DT_INST_DEFINE(n, nxp_enet_mod_init, NULL,					\
		&nxp_enet_mod_data_##n, &nxp_enet_mod_cfg_##n,				\
		POST_KERNEL, 0, NULL);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_enet

DT_INST_FOREACH_STATUS_OKAY_VARGS(NXP_ENET_INIT, DT_DRV_COMPAT)

#define NXP_ENET1G_INIT(n, compat)							\
											\
static const struct nxp_enet_mod_config nxp_enet1g_mod_cfg_##n = {			\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),					\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_DRV_INST(n))),		\
		.clock_subsys = (void *) DT_CLOCKS_CELL_BY_IDX(				\
							DT_DRV_INST(n), 0, name),	\
};											\
											\
static struct nxp_enet_mod_data nxp_enet1g_mod_data_##n;				\
											\
/* Init the module before any of the MAC, MDIO, or PTP clock */				\
DEVICE_DT_INST_DEFINE(n, nxp_enet_mod_init, NULL,					\
		&nxp_enet1g_mod_data_##n, &nxp_enet1g_mod_cfg_##n,			\
		POST_KERNEL, 0, NULL);

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_enet1g

DT_INST_FOREACH_STATUS_OKAY_VARGS(NXP_ENET1G_INIT, DT_DRV_COMPAT)
