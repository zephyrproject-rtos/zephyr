/* MCUX Ethernet Driver
 *
 *  Copyright (c) 2016-2017 ARM Ltd
 *  Copyright (c) 2016 Linaro Ltd
 *  Copyright (c) 2018 Intel Coporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Driver Limitations:
 *
 * There is no statistics collection for either normal operation or
 * error behaviour.
 */

#define LOG_MODULE_NAME eth_mcux
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <device.h>
#include <sys/util.h>
#include <kernel.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <ethernet/eth_stats.h>

#if defined(CONFIG_PTP_CLOCK_MCUX)
#include <ptp_clock.h>
#include <net/gptp.h>
#endif

#include "fsl_enet.h"
#include "fsl_phy.h"

#define FREESCALE_OUI_B0 0x00
#define FREESCALE_OUI_B1 0x04
#define FREESCALE_OUI_B2 0x9f

enum eth_mcux_phy_state {
	eth_mcux_phy_state_initial,
	eth_mcux_phy_state_reset,
	eth_mcux_phy_state_autoneg,
	eth_mcux_phy_state_restart,
	eth_mcux_phy_state_read_status,
	eth_mcux_phy_state_read_duplex,
	eth_mcux_phy_state_wait,
	eth_mcux_phy_state_closing

};

static const char *
phy_state_name(enum eth_mcux_phy_state state)  __attribute__((unused));

static const char *phy_state_name(enum eth_mcux_phy_state state)
{
	static const char * const name[] = {
		"initial",
		"reset",
		"autoneg",
		"restart",
		"read-status",
		"read-duplex",
		"wait",
		"closing"
	};

	return name[state];
}

struct eth_context {
	/* If VLAN is enabled, there can be multiple VLAN interfaces related to
	 * this physical device. In that case, this pointer value is not really
	 * used for anything.
	 */
	struct net_if *iface;
	enet_handle_t enet_handle;
#if defined(CONFIG_PTP_CLOCK_MCUX)
	struct device *ptp_clock;
	enet_ptp_config_t ptp_config;
	float clk_ratio;
#endif
	struct k_sem tx_buf_sem;
	enum eth_mcux_phy_state phy_state;
	bool enabled;
	bool link_up;
	phy_duplex_t phy_duplex;
	phy_speed_t phy_speed;
	u8_t mac_addr[6];
	struct k_work phy_work;
	struct k_delayed_work delayed_phy_work;
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
	u8_t frame_buf[NET_ETH_MAX_FRAME_SIZE]; /* Max MTU + ethernet header */
};

static void eth_0_config_func(void);

#ifdef CONFIG_HAS_MCUX_CACHE
static __nocache enet_rx_bd_struct_t __aligned(ENET_BUFF_ALIGNMENT)
rx_buffer_desc[CONFIG_ETH_MCUX_RX_BUFFERS];

static __nocache enet_tx_bd_struct_t __aligned(ENET_BUFF_ALIGNMENT)
tx_buffer_desc[CONFIG_ETH_MCUX_TX_BUFFERS];
#else
static enet_rx_bd_struct_t __aligned(ENET_BUFF_ALIGNMENT)
rx_buffer_desc[CONFIG_ETH_MCUX_RX_BUFFERS];

static enet_tx_bd_struct_t __aligned(ENET_BUFF_ALIGNMENT)
tx_buffer_desc[CONFIG_ETH_MCUX_TX_BUFFERS];
#endif

#if defined(CONFIG_PTP_CLOCK_MCUX)
/* Packets to be timestamped. */
static struct net_pkt *ts_tx_pkt[CONFIG_ETH_MCUX_TX_BUFFERS];
static int ts_tx_rd, ts_tx_wr;
#endif

/* Use ENET_FRAME_MAX_VALNFRAMELEN for VLAN frame size
 * Use ENET_FRAME_MAX_FRAMELEN for ethernet frame size
 */
#if defined(CONFIG_NET_VLAN)
#if !defined(ENET_FRAME_MAX_VALNFRAMELEN)
#define ENET_FRAME_MAX_VALNFRAMELEN (ENET_FRAME_MAX_FRAMELEN + 4)
#endif
#define ETH_MCUX_BUFFER_SIZE \
	ROUND_UP(ENET_FRAME_MAX_VALNFRAMELEN, ENET_BUFF_ALIGNMENT)
#else
#define ETH_MCUX_BUFFER_SIZE \
	ROUND_UP(ENET_FRAME_MAX_FRAMELEN, ENET_BUFF_ALIGNMENT)
#endif /* CONFIG_NET_VLAN */

static u8_t __aligned(ENET_BUFF_ALIGNMENT)
rx_buffer[CONFIG_ETH_MCUX_RX_BUFFERS][ETH_MCUX_BUFFER_SIZE];

static u8_t __aligned(ENET_BUFF_ALIGNMENT)
tx_buffer[CONFIG_ETH_MCUX_TX_BUFFERS][ETH_MCUX_BUFFER_SIZE];

static void eth_mcux_decode_duplex_and_speed(u32_t status,
					     phy_duplex_t *p_phy_duplex,
					     phy_speed_t *p_phy_speed)
{
	switch (status & PHY_CTL1_SPEEDUPLX_MASK) {
	case PHY_CTL1_10FULLDUPLEX_MASK:
		*p_phy_duplex = kPHY_FullDuplex;
		*p_phy_speed = kPHY_Speed10M;
		break;
	case PHY_CTL1_100FULLDUPLEX_MASK:
		*p_phy_duplex = kPHY_FullDuplex;
		*p_phy_speed = kPHY_Speed100M;
		break;
	case PHY_CTL1_100HALFDUPLEX_MASK:
		*p_phy_duplex = kPHY_HalfDuplex;
		*p_phy_speed = kPHY_Speed100M;
		break;
	case PHY_CTL1_10HALFDUPLEX_MASK:
		*p_phy_duplex = kPHY_HalfDuplex;
		*p_phy_speed = kPHY_Speed10M;
		break;
	}
}

static inline struct net_if *get_iface(struct eth_context *ctx, u16_t vlan_tag)
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

static void eth_mcux_phy_enter_reset(struct eth_context *context)
{
	const u32_t phy_addr = 0U;

	/* Reset the PHY. */
	ENET_StartSMIWrite(ENET, phy_addr, PHY_BASICCONTROL_REG,
			   kENET_MiiWriteValidFrame,
			   PHY_BCTL_RESET_MASK);
	context->phy_state = eth_mcux_phy_state_reset;
}

static void eth_mcux_phy_start(struct eth_context *context)
{
	const u32_t phy_addr = 0U;
#ifdef CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG
	LOG_DBG("phy_state=%s", phy_state_name(context->phy_state));
#endif

	context->enabled = true;

	switch (context->phy_state) {
	case eth_mcux_phy_state_initial:
		ENET_ActiveRead(ENET);
		/* Reset the PHY. */
		ENET_StartSMIWrite(ENET, phy_addr, PHY_BASICCONTROL_REG,
			   kENET_MiiWriteValidFrame,
			   PHY_BCTL_RESET_MASK);
#ifdef CONFIG_SOC_SERIES_IMX_RT
		context->phy_state = eth_mcux_phy_state_initial;
#else
		context->phy_state = eth_mcux_phy_state_reset;
#endif
		break;
	case eth_mcux_phy_state_reset:
		eth_mcux_phy_enter_reset(context);
		break;
	case eth_mcux_phy_state_autoneg:
	case eth_mcux_phy_state_restart:
	case eth_mcux_phy_state_read_status:
	case eth_mcux_phy_state_read_duplex:
	case eth_mcux_phy_state_wait:
	case eth_mcux_phy_state_closing:
		break;
	}
}

void eth_mcux_phy_stop(struct eth_context *context)
{
#ifdef CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG
	LOG_DBG("phy_state=%s", phy_state_name(context->phy_state));
#endif

	context->enabled = false;

	switch (context->phy_state) {
	case eth_mcux_phy_state_initial:
	case eth_mcux_phy_state_reset:
	case eth_mcux_phy_state_autoneg:
	case eth_mcux_phy_state_restart:
	case eth_mcux_phy_state_read_status:
	case eth_mcux_phy_state_read_duplex:
		/* Do nothing, let the current communication complete
		 * then deal with shutdown.
		 */
		context->phy_state = eth_mcux_phy_state_closing;
		break;
	case eth_mcux_phy_state_wait:
		k_delayed_work_cancel(&context->delayed_phy_work);
		/* @todo, actually power downt he PHY ? */
		context->phy_state = eth_mcux_phy_state_initial;
		break;
	case eth_mcux_phy_state_closing:
		/* We are already going down. */
		break;
	}
}

static void eth_mcux_phy_event(struct eth_context *context)
{
	u32_t status;
	bool link_up;
	phy_duplex_t phy_duplex = kPHY_FullDuplex;
	phy_speed_t phy_speed = kPHY_Speed100M;
	const u32_t phy_addr = 0U;

#ifdef CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG
	LOG_DBG("phy_state=%s", phy_state_name(context->phy_state));
#endif
	switch (context->phy_state) {
	case eth_mcux_phy_state_initial:
#ifdef CONFIG_SOC_SERIES_IMX_RT
		ENET_StartSMIRead(ENET, phy_addr, PHY_CONTROL2_REG,
			kENET_MiiReadValidFrame);
		ENET_StartSMIWrite(ENET, phy_addr, PHY_CONTROL2_REG,
			kENET_MiiWriteValidFrame, PHY_CTL2_REFCLK_SELECT_MASK);
		context->phy_state = eth_mcux_phy_state_reset;
#endif
		break;
	case eth_mcux_phy_state_closing:
		if (context->enabled) {
			eth_mcux_phy_enter_reset(context);
		} else {
			/* @todo, actually power down the PHY ? */
			context->phy_state = eth_mcux_phy_state_initial;
		}
		break;
	case eth_mcux_phy_state_reset:
		/* Setup PHY autonegotiation. */
		ENET_StartSMIWrite(ENET, phy_addr, PHY_AUTONEG_ADVERTISE_REG,
				   kENET_MiiWriteValidFrame,
				   (PHY_100BASETX_FULLDUPLEX_MASK |
				    PHY_100BASETX_HALFDUPLEX_MASK |
				    PHY_10BASETX_FULLDUPLEX_MASK |
				    PHY_10BASETX_HALFDUPLEX_MASK | 0x1U));
		context->phy_state = eth_mcux_phy_state_autoneg;
		break;
	case eth_mcux_phy_state_autoneg:
		/* Setup PHY autonegotiation. */
		ENET_StartSMIWrite(ENET, phy_addr, PHY_BASICCONTROL_REG,
				   kENET_MiiWriteValidFrame,
				   (PHY_BCTL_AUTONEG_MASK |
				    PHY_BCTL_RESTART_AUTONEG_MASK));
		context->phy_state = eth_mcux_phy_state_restart;
		break;
	case eth_mcux_phy_state_wait:
	case eth_mcux_phy_state_restart:
		/* Start reading the PHY basic status. */
		ENET_StartSMIRead(ENET, phy_addr, PHY_BASICSTATUS_REG,
				  kENET_MiiReadValidFrame);
		context->phy_state = eth_mcux_phy_state_read_status;
		break;
	case eth_mcux_phy_state_read_status:
		/* PHY Basic status is available. */
		status = ENET_ReadSMIData(ENET);
		link_up =  status & PHY_BSTATUS_LINKSTATUS_MASK;
		if (link_up && !context->link_up) {
			/* Start reading the PHY control register. */
			ENET_StartSMIRead(ENET, phy_addr, PHY_CONTROL1_REG,
					  kENET_MiiReadValidFrame);
			context->link_up = link_up;
			context->phy_state = eth_mcux_phy_state_read_duplex;
			net_eth_carrier_on(context->iface);
			k_sleep(USEC_PER_MSEC);
		} else if (!link_up && context->link_up) {
			LOG_INF("Link down");
			context->link_up = link_up;
			k_delayed_work_submit(&context->delayed_phy_work,
					      CONFIG_ETH_MCUX_PHY_TICK_MS);
			context->phy_state = eth_mcux_phy_state_wait;
			net_eth_carrier_off(context->iface);
		} else {
			k_delayed_work_submit(&context->delayed_phy_work,
					      CONFIG_ETH_MCUX_PHY_TICK_MS);
			context->phy_state = eth_mcux_phy_state_wait;
		}

		break;
	case eth_mcux_phy_state_read_duplex:
		/* PHY control register is available. */
		status = ENET_ReadSMIData(ENET);
		eth_mcux_decode_duplex_and_speed(status,
						 &phy_duplex,
						 &phy_speed);
		if (phy_speed != context->phy_speed ||
		    phy_duplex != context->phy_duplex) {
			context->phy_speed = phy_speed;
			context->phy_duplex = phy_duplex;
			ENET_SetMII(ENET,
				    (enet_mii_speed_t) phy_speed,
				    (enet_mii_duplex_t) phy_duplex);
		}

		LOG_INF("Enabled %sM %s-duplex mode.",
			(phy_speed ? "100" : "10"),
			(phy_duplex ? "full" : "half"));
		k_delayed_work_submit(&context->delayed_phy_work,
				      CONFIG_ETH_MCUX_PHY_TICK_MS);
		context->phy_state = eth_mcux_phy_state_wait;
		break;
	}
}

static void eth_mcux_phy_work(struct k_work *item)
{
	struct eth_context *context =
		CONTAINER_OF(item, struct eth_context, phy_work);

	eth_mcux_phy_event(context);
}

static void eth_mcux_delayed_phy_work(struct k_work *item)
{
	struct eth_context *context =
		CONTAINER_OF(item, struct eth_context, delayed_phy_work);

	eth_mcux_phy_event(context);
}

static void eth_mcux_phy_setup(void)
{
#ifdef CONFIG_SOC_SERIES_IMX_RT
	const u32_t phy_addr = 0U;
	u32_t status;

	/* Prevent PHY entering NAND Tree mode override*/
	ENET_StartSMIRead(ENET, phy_addr, PHY_OMS_STATUS_REG,
		kENET_MiiReadValidFrame);
	status = ENET_ReadSMIData(ENET);

	if (status & PHY_OMS_NANDTREE_MASK) {
		status &= ~PHY_OMS_NANDTREE_MASK;
		ENET_StartSMIWrite(ENET, phy_addr, PHY_OMS_OVERRIDE_REG,
			kENET_MiiWriteValidFrame, status);
	}
#endif
}

#if defined(CONFIG_PTP_CLOCK_MCUX)
static enet_ptp_time_data_t ptp_rx_buffer[CONFIG_ETH_MCUX_PTP_RX_BUFFERS];
static enet_ptp_time_data_t ptp_tx_buffer[CONFIG_ETH_MCUX_PTP_TX_BUFFERS];

static bool eth_get_ptp_data(struct net_if *iface, struct net_pkt *pkt,
			     enet_ptp_time_data_t *ptpTsData, bool is_tx)
{
	int eth_hlen;

#if defined(CONFIG_NET_VLAN)
	struct net_eth_vlan_hdr *hdr_vlan;
	struct ethernet_context *eth_ctx;
	bool vlan_enabled = false;

	eth_ctx = net_if_l2_data(iface);
	if (net_eth_is_vlan_enabled(eth_ctx, iface)) {
		hdr_vlan = (struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);
		vlan_enabled = true;

		if (ntohs(hdr_vlan->type) != NET_ETH_PTYPE_PTP) {
			return false;
		}

		eth_hlen = sizeof(struct net_eth_vlan_hdr);
	} else
#endif
	{
		if (ntohs(NET_ETH_HDR(pkt)->type) != NET_ETH_PTYPE_PTP) {
			return false;
		}

		eth_hlen = sizeof(struct net_eth_hdr);
	}

	net_pkt_set_priority(pkt, NET_PRIORITY_CA);

	if (ptpTsData) {
		/* Cannot use GPTP_HDR as net_pkt fields are not all filled */
		struct gptp_hdr *hdr;

		/* In TX, the first net_buf contains the Ethernet header
		 * and the actual gPTP header is in the second net_buf.
		 * In RX, the Ethernet header + other headers are in the
		 * first net_buf.
		 */
		if (is_tx) {
			if (pkt->frags->frags == NULL) {
				return false;
			}

			hdr = (struct gptp_hdr *)pkt->frags->frags->data;
		} else {
			hdr = (struct gptp_hdr *)(pkt->frags->data +
							   eth_hlen);
		}

		ptpTsData->version = hdr->ptp_version;
		memcpy(ptpTsData->sourcePortId, &hdr->port_id,
		       kENET_PtpSrcPortIdLen);
		ptpTsData->messageType = hdr->message_type;
		ptpTsData->sequenceId = ntohs(hdr->sequence_id);

#ifdef CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG
		LOG_DBG("PTP packet: ver %d type %d len %d seq %d",
			ptpTsData->version,
			ptpTsData->messageType,
			ntohs(hdr->message_length),
			ptpTsData->sequenceId);
		LOG_DBG("  clk %02x%02x%02x%02x%02x%02x%02x%02x port %d",
			hdr->port_id.clk_id[0],
			hdr->port_id.clk_id[1],
			hdr->port_id.clk_id[2],
			hdr->port_id.clk_id[3],
			hdr->port_id.clk_id[4],
			hdr->port_id.clk_id[5],
			hdr->port_id.clk_id[6],
			hdr->port_id.clk_id[7],
			ntohs(hdr->port_id.port_number));
#endif
	}

	return true;
}
#endif /* CONFIG_PTP_CLOCK_MCUX */

static int eth_tx(struct device *dev, struct net_pkt *pkt)
{
	struct eth_context *context = dev->driver_data;
	u16_t total_len = net_pkt_get_len(pkt);
	status_t status;
	unsigned int imask;
#if defined(CONFIG_PTP_CLOCK_MCUX)
	bool timestamped_frame;
#endif

	/* As context->frame_buf is shared resource used by both eth_tx
	 * and eth_rx, we need to protect it with irq_lock.
	 */
	imask = irq_lock();

	if (net_pkt_read(pkt, context->frame_buf, total_len)) {
		irq_unlock(imask);
		return -EIO;
	}

	/* FIXME: Dirty workaround.
	 * With current implementation of ENET_StoreTxFrameTime in the MCUX
	 * library, a frame may not be timestamped when a non-timestamped frame
	 * is sent.
	 */
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
	context->enet_handle.txBdDirtyTime[0] =
				context->enet_handle.txBdCurrent[0];
#endif

	status = ENET_SendFrame(ENET, &context->enet_handle, context->frame_buf,
				total_len);

#if defined(CONFIG_PTP_CLOCK_MCUX)
	timestamped_frame = eth_get_ptp_data(net_pkt_iface(pkt), pkt, NULL,
					     true);
	if (timestamped_frame) {
		if (!status) {
			ts_tx_pkt[ts_tx_wr] = net_pkt_ref(pkt);
		} else {
			ts_tx_pkt[ts_tx_wr] = NULL;
		}

		ts_tx_wr++;
		if (ts_tx_wr >= CONFIG_ETH_MCUX_TX_BUFFERS) {
			ts_tx_wr = 0;
		}
	}
#endif

	irq_unlock(imask);

	if (status) {
		LOG_ERR("ENET_SendFrame error: %d", (int)status);
		return -1;
	}

	k_sem_take(&context->tx_buf_sem, K_FOREVER);

	return 0;
}

static void eth_rx(struct device *iface)
{
	struct eth_context *context = iface->driver_data;
	u16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	u32_t frame_length = 0U;
	struct net_pkt *pkt;
	status_t status;
	unsigned int imask;

#if defined(CONFIG_PTP_CLOCK_MCUX)
	enet_ptp_time_data_t ptpTimeData;
#endif

	status = ENET_GetRxFrameSize(&context->enet_handle,
				     (uint32_t *)&frame_length);
	if (status) {
		enet_data_error_stats_t error_stats;

		LOG_ERR("ENET_GetRxFrameSize return: %d", (int)status);

		ENET_GetRxErrBeforeReadFrame(&context->enet_handle,
					     &error_stats);
		goto flush;
	}

	if (sizeof(context->frame_buf) < frame_length) {
		LOG_ERR("frame too large (%d)", frame_length);
		goto flush;
	}

	/* Using root iface. It will be updated in net_recv_data() */
	pkt = net_pkt_rx_alloc_with_buffer(context->iface, frame_length,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		goto flush;
	}

	/* As context->frame_buf is shared resource used by both eth_tx
	 * and eth_rx, we need to protect it with irq_lock.
	 */
	imask = irq_lock();

	status = ENET_ReadFrame(ENET, &context->enet_handle,
				context->frame_buf, frame_length);
	if (status) {
		irq_unlock(imask);
		LOG_ERR("ENET_ReadFrame failed: %d", (int)status);
		net_pkt_unref(pkt);
		goto error;
	}

	if (net_pkt_write(pkt, context->frame_buf, frame_length)) {
		irq_unlock(imask);
		LOG_ERR("Unable to write frame into the pkt");
		net_pkt_unref(pkt);
		goto error;
	}

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
#endif
		}
	}
#endif

#if defined(CONFIG_PTP_CLOCK_MCUX)
	if (eth_get_ptp_data(get_iface(context, vlan_tag), pkt,
			     &ptpTimeData, false) &&
	    (ENET_GetRxFrameTime(&context->enet_handle,
				 &ptpTimeData) == kStatus_Success)) {
		pkt->timestamp.nanosecond = ptpTimeData.timeStamp.nanosecond;
		pkt->timestamp.second = ptpTimeData.timeStamp.second;
	} else {
		/* Invalid value. */
		pkt->timestamp.nanosecond = UINT32_MAX;
		pkt->timestamp.second = UINT64_MAX;
	}
#endif /* CONFIG_PTP_CLOCK_MCUX */

	irq_unlock(imask);

	if (net_recv_data(get_iface(context, vlan_tag), pkt) < 0) {
		net_pkt_unref(pkt);
		goto error;
	}

	return;
flush:
	/* Flush the current read buffer.  This operation can
	 * only report failure if there is no frame to flush,
	 * which cannot happen in this context.
	 */
	status = ENET_ReadFrame(ENET, &context->enet_handle, NULL, 0);
	assert(status == kStatus_Success);
error:
	eth_stats_update_errors_rx(get_iface(context, vlan_tag));
}

#if defined(CONFIG_PTP_CLOCK_MCUX)
static inline void ts_register_tx_event(struct eth_context *context)
{
	struct net_pkt *pkt;
	enet_ptp_time_data_t timeData;

	pkt = ts_tx_pkt[ts_tx_rd];
	if (pkt && atomic_get(&pkt->atomic_ref) > 0) {
		if (eth_get_ptp_data(net_pkt_iface(pkt), pkt, &timeData,
				     true)) {
			int status;

			status = ENET_GetTxFrameTime(&context->enet_handle,
						     &timeData);
			if (status == kStatus_Success) {
				pkt->timestamp.nanosecond =
					timeData.timeStamp.nanosecond;
				pkt->timestamp.second =
					timeData.timeStamp.second;

				net_if_add_tx_timestamp(pkt);
			}
		}

		net_pkt_unref(pkt);
	} else {
		if (IS_ENABLED(CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG) && pkt) {
			LOG_ERR("pkt %p already freed", pkt);
		}
	}

	ts_tx_pkt[ts_tx_rd++] = NULL;

	if (ts_tx_rd >= CONFIG_ETH_MCUX_TX_BUFFERS) {
		ts_tx_rd = 0;
	}
}
#endif

static void eth_callback(ENET_Type *base, enet_handle_t *handle,
			 enet_event_t event, void *param)
{
	struct device *iface = param;
	struct eth_context *context = iface->driver_data;

	switch (event) {
	case kENET_RxEvent:
		eth_rx(iface);
		break;
	case kENET_TxEvent:
#if defined(CONFIG_PTP_CLOCK_MCUX)
		/* Register event */
		ts_register_tx_event(context);
#endif /* CONFIG_PTP_CLOCK_MCUX */

		/* Free the TX buffer. */
		k_sem_give(&context->tx_buf_sem);
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
		ENET->ATPER = NSEC_PER_SEC;
		break;
	case kENET_TimeStampAvailEvent:
		/* Time stamp available event.  */
		break;
	}
}

#if defined(CONFIG_ETH_MCUX_0_RANDOM_MAC)
static void generate_mac(u8_t *mac_addr)
{
	u32_t entropy;

	entropy = sys_rand32_get();

	mac_addr[0] |= 0x02; /* force LAA bit */

	mac_addr[3] = entropy >> 8;
	mac_addr[4] = entropy >> 16;
	mac_addr[5] = entropy >> 0;
}
#elif defined(CONFIG_ETH_MCUX_0_UNIQUE_MAC)
static void generate_mac(u8_t *mac_addr)
{
	/* Trivially "hash" up to 128 bits of MCU unique identifier */
#ifdef CONFIG_SOC_SERIES_IMX_RT
	u32_t id = OCOTP->CFG1 ^ OCOTP->CFG2;
#endif
#ifdef CONFIG_SOC_SERIES_KINETIS_K6X
	u32_t id = SIM->UIDH ^ SIM->UIDMH ^ SIM->UIDML ^ SIM->UIDL;
#endif

	mac_addr[0] |= 0x02; /* force LAA bit */

	mac_addr[3] = id >> 8;
	mac_addr[4] = id >> 16;
	mac_addr[5] = id >> 0;
}
#endif

static int eth_0_init(struct device *dev)
{
	struct eth_context *context = dev->driver_data;
	enet_config_t enet_config;
	u32_t sys_clock;
	enet_buffer_config_t buffer_config = {
		.rxBdNumber = CONFIG_ETH_MCUX_RX_BUFFERS,
		.txBdNumber = CONFIG_ETH_MCUX_TX_BUFFERS,
		.rxBuffSizeAlign = ETH_MCUX_BUFFER_SIZE,
		.txBuffSizeAlign = ETH_MCUX_BUFFER_SIZE,
		.rxBdStartAddrAlign = rx_buffer_desc,
		.txBdStartAddrAlign = tx_buffer_desc,
		.rxBufferAlign = rx_buffer[0],
		.txBufferAlign = tx_buffer[0],
	};
#if defined(CONFIG_PTP_CLOCK_MCUX)
	u8_t ptp_multicast[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E };
#endif
#if defined(CONFIG_MDNS_RESPONDER) || defined(CONFIG_MDNS_RESOLVER)
	/* standard multicast MAC address */
	u8_t mdns_multicast[6] = { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB };
#endif

#if defined(CONFIG_PTP_CLOCK_MCUX)
	ts_tx_rd = 0;
	ts_tx_wr = 0;
	(void)memset(ts_tx_pkt, 0, sizeof(ts_tx_pkt));
#endif

	k_sem_init(&context->tx_buf_sem,
		   0, CONFIG_ETH_MCUX_TX_BUFFERS);
	k_work_init(&context->phy_work, eth_mcux_phy_work);
	k_delayed_work_init(&context->delayed_phy_work,
			    eth_mcux_delayed_phy_work);

	eth_mcux_phy_setup();

	sys_clock = CLOCK_GetFreq(kCLOCK_CoreSysClk);

	ENET_GetDefaultConfig(&enet_config);
	enet_config.interrupt |= kENET_RxFrameInterrupt;
	enet_config.interrupt |= kENET_TxFrameInterrupt;
	enet_config.interrupt |= kENET_MiiInterrupt;

#ifdef CONFIG_ETH_MCUX_PROMISCUOUS_MODE
	enet_config.macSpecialConfig |= kENET_ControlPromiscuousEnable;
#endif

	/* Initialize/override OUI which may not be correct in
	 * devicetree.
	 */
	context->mac_addr[0] = FREESCALE_OUI_B0;
	context->mac_addr[1] = FREESCALE_OUI_B1;
	context->mac_addr[2] = FREESCALE_OUI_B2;
#if defined(CONFIG_ETH_MCUX_0_UNIQUE_MAC) || \
    defined(CONFIG_ETH_MCUX_0_RANDOM_MAC)
	generate_mac(context->mac_addr);
#endif

#if defined(CONFIG_NET_VLAN)
	enet_config.macSpecialConfig |= kENET_ControlVLANTagEnable;
#endif

	ENET_Init(ENET,
		  &context->enet_handle,
		  &enet_config,
		  &buffer_config,
		  context->mac_addr,
		  sys_clock);

#if defined(CONFIG_PTP_CLOCK_MCUX)
	ENET_AddMulticastGroup(ENET, ptp_multicast);

	context->ptp_config.ptpTsRxBuffNum = CONFIG_ETH_MCUX_PTP_RX_BUFFERS;
	context->ptp_config.ptpTsTxBuffNum = CONFIG_ETH_MCUX_PTP_TX_BUFFERS;
	context->ptp_config.rxPtpTsData = ptp_rx_buffer;
	context->ptp_config.txPtpTsData = ptp_tx_buffer;
	context->ptp_config.channel = kENET_PtpTimerChannel1;
	context->ptp_config.ptp1588ClockSrc_Hz =
					CONFIG_ETH_MCUX_PTP_CLOCK_SRC_HZ;
	context->clk_ratio = 1.0;

	ENET_Ptp1588Configure(ENET, &context->enet_handle,
			      &context->ptp_config);
#endif
#if defined(CONFIG_MDNS_RESPONDER) || defined(CONFIG_MDNS_RESOLVER)
	ENET_AddMulticastGroup(ENET, mdns_multicast);
#endif

	ENET_SetSMI(ENET, sys_clock, false);

	LOG_DBG("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		context->mac_addr[0], context->mac_addr[1],
		context->mac_addr[2], context->mac_addr[3],
		context->mac_addr[4], context->mac_addr[5]);

	ENET_SetCallback(&context->enet_handle, eth_callback, dev);

	eth_mcux_phy_start(context);

	return 0;
}

#if defined(CONFIG_NET_IPV6)
static void net_if_mcast_cb(struct net_if *iface,
			    const struct in6_addr *addr,
			    bool is_joined)
{
	struct net_eth_addr mac_addr;

	net_eth_ipv6_mcast_to_mac_addr(addr, &mac_addr);

	if (is_joined) {
		ENET_AddMulticastGroup(ENET, mac_addr.addr);
	} else {
		ENET_LeaveMulticastGroup(ENET, mac_addr.addr);
	}
}
#endif /* CONFIG_NET_IPV6 */

static void eth_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->driver_data;

#if defined(CONFIG_NET_IPV6)
	static struct net_if_mcast_monitor mon;

	net_if_mcast_mon_register(&mon, iface, net_if_mcast_cb);
#endif /* CONFIG_NET_IPV6 */

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	/* For VLAN, this value is only used to get the correct L2 driver */
	context->iface = iface;

	ethernet_init(iface);
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	eth_0_config_func();
}

static enum ethernet_hw_caps eth_mcux_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_HW_VLAN | ETHERNET_LINK_10BASE_T |
#if defined(CONFIG_PTP_CLOCK_MCUX)
		ETHERNET_PTP |
#endif
		ETHERNET_LINK_100BASE_T;
}

#if defined(CONFIG_PTP_CLOCK_MCUX)
static struct device *eth_mcux_get_ptp_clock(struct device *dev)
{
	struct eth_context *context = dev->driver_data;

	return context->ptp_clock;
}
#endif

static const struct ethernet_api api_funcs = {
	.iface_api.init		= eth_iface_init,
#if defined(CONFIG_PTP_CLOCK_MCUX)
	.get_ptp_clock		= eth_mcux_get_ptp_clock,
#endif
	.get_capabilities	= eth_mcux_get_capabilities,
	.send			= eth_tx,
};

#if defined(CONFIG_PTP_CLOCK_MCUX)
static void eth_mcux_ptp_isr(void *p)
{
	struct device *dev = p;
	struct eth_context *context = dev->driver_data;

	ENET_Ptp1588TimerIRQHandler(ENET, &context->enet_handle);
}
#endif

#if defined(DT_IRQ_ETH_COMMON)
static void eth_mcux_dispacher_isr(void *p)
{
	struct device *dev = p;
	struct eth_context *context = dev->driver_data;
	u32_t EIR = ENET_GetInterruptStatus(ENET);
	int irq_lock_key = irq_lock();

	if (EIR & (kENET_RxBufferInterrupt | kENET_RxFrameInterrupt)) {
		ENET_ReceiveIRQHandler(ENET, &context->enet_handle);
	} else if (EIR & (kENET_TxBufferInterrupt | kENET_TxFrameInterrupt)) {
		ENET_TransmitIRQHandler(ENET, &context->enet_handle);
	} else if (EIR & ENET_EIR_MII_MASK) {
		k_work_submit(&context->phy_work);
		ENET_ClearInterruptStatus(ENET, kENET_MiiInterrupt);
	} else if (EIR) {
		ENET_ClearInterruptStatus(ENET, 0xFFFFFFFF);
	}

	irq_unlock(irq_lock_key);
}
#endif

#if defined(DT_IRQ_ETH_RX)
static void eth_mcux_rx_isr(void *p)
{
	struct device *dev = p;
	struct eth_context *context = dev->driver_data;

	ENET_ReceiveIRQHandler(ENET, &context->enet_handle);
}
#endif

#if defined(DT_IRQ_ETH_TX)
static void eth_mcux_tx_isr(void *p)
{
	struct device *dev = p;
	struct eth_context *context = dev->driver_data;

	ENET_TransmitIRQHandler(ENET, &context->enet_handle);
}
#endif

#if defined(DT_IRQ_ETH_ERR_MISC)
static void eth_mcux_error_isr(void *p)
{
	struct device *dev = p;
	struct eth_context *context = dev->driver_data;
	u32_t pending = ENET_GetInterruptStatus(ENET);

	if (pending & ENET_EIR_MII_MASK) {
		k_work_submit(&context->phy_work);
		ENET_ClearInterruptStatus(ENET, kENET_MiiInterrupt);
	}
}
#endif

static struct eth_context eth_0_context = {
	.phy_duplex = kPHY_FullDuplex,
	.phy_speed = kPHY_Speed100M,
#if defined(CONFIG_ETH_MCUX_0_MANUAL_MAC)
	.mac_addr = DT_ETH_MCUX_0_MAC,
#endif
};

ETH_NET_DEVICE_INIT(eth_mcux_0, DT_ETH_MCUX_0_NAME, eth_0_init,
		    &eth_0_context, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs, NET_ETH_MTU);

static void eth_0_config_func(void)
{
#if defined(DT_IRQ_ETH_RX)
	IRQ_CONNECT(DT_IRQ_ETH_RX, DT_ETH_MCUX_0_IRQ_PRI,
		    eth_mcux_rx_isr, DEVICE_GET(eth_mcux_0), 0);
	irq_enable(DT_IRQ_ETH_RX);
#endif

#if defined(DT_IRQ_ETH_TX)
	IRQ_CONNECT(DT_IRQ_ETH_TX, DT_ETH_MCUX_0_IRQ_PRI,
		    eth_mcux_tx_isr, DEVICE_GET(eth_mcux_0), 0);
	irq_enable(DT_IRQ_ETH_TX);
#endif

#if defined(DT_IRQ_ETH_ERR_MISC)
	IRQ_CONNECT(DT_IRQ_ETH_ERR_MISC, DT_ETH_MCUX_0_IRQ_PRI,
		    eth_mcux_error_isr, DEVICE_GET(eth_mcux_0), 0);
	irq_enable(DT_IRQ_ETH_ERR_MISC);
#endif

#if defined(DT_IRQ_ETH_COMMON)
	IRQ_CONNECT(DT_IRQ_ETH_COMMON, DT_ETH_MCUX_0_IRQ_PRI,
		    eth_mcux_dispacher_isr, DEVICE_GET(eth_mcux_0), 0);
	irq_enable(DT_IRQ_ETH_COMMON);
#endif

#if defined(CONFIG_PTP_CLOCK_MCUX)
	IRQ_CONNECT(DT_IRQ_ETH_IEEE1588_TMR, DT_ETH_MCUX_0_IRQ_PRI,
		    eth_mcux_ptp_isr, DEVICE_GET(eth_mcux_0), 0);
	irq_enable(DT_IRQ_ETH_IEEE1588_TMR);
#endif
}

#if defined(CONFIG_PTP_CLOCK_MCUX)
struct ptp_context {
	struct eth_context *eth_context;
};

static struct ptp_context ptp_mcux_0_context;

static int ptp_clock_mcux_set(struct device *dev, struct net_ptp_time *tm)
{
	struct ptp_context *ptp_context = dev->driver_data;
	struct eth_context *context = ptp_context->eth_context;
	enet_ptp_time_t enet_time;

	enet_time.second = tm->second;
	enet_time.nanosecond = tm->nanosecond;

	ENET_Ptp1588SetTimer(ENET, &context->enet_handle, &enet_time);
	return 0;
}

static int ptp_clock_mcux_get(struct device *dev, struct net_ptp_time *tm)
{
	struct ptp_context *ptp_context = dev->driver_data;
	struct eth_context *context = ptp_context->eth_context;
	enet_ptp_time_t enet_time;

	ENET_Ptp1588GetTimer(ENET, &context->enet_handle, &enet_time);

	tm->second = enet_time.second;
	tm->nanosecond = enet_time.nanosecond;
	return 0;
}

static int ptp_clock_mcux_adjust(struct device *dev, int increment)
{
	int key, ret;

	ARG_UNUSED(dev);

	if ((increment <= -NSEC_PER_SEC) || (increment >= NSEC_PER_SEC)) {
		ret = -EINVAL;
	} else {
		key = irq_lock();
		if (ENET->ATPER != NSEC_PER_SEC) {
			ret = -EBUSY;
		} else {
			/* Seconds counter is handled by software. Change the
			 * period of one software second to adjust the clock.
			 */
			ENET->ATPER = NSEC_PER_SEC - increment;
			ret = 0;
		}
		irq_unlock(key);
	}

	return ret;
}

static int ptp_clock_mcux_rate_adjust(struct device *dev, float ratio)
{
	const int hw_inc = NSEC_PER_SEC / CONFIG_ETH_MCUX_PTP_CLOCK_SRC_HZ;
	struct ptp_context *ptp_context = dev->driver_data;
	struct eth_context *context = ptp_context->eth_context;
	int corr;
	s32_t mul;
	float val;

	/* No change needed. */
	if (ratio == 1.0) {
		return 0;
	}

	ratio *= context->clk_ratio;

	/* Limit possible ratio. */
	if ((ratio > 1.0 + 1.0/(2 * hw_inc)) ||
			(ratio < 1.0 - 1.0/(2 * hw_inc))) {
		return -EINVAL;
	}

	/* Save new ratio. */
	context->clk_ratio = ratio;

	if (ratio < 1.0) {
		corr = hw_inc - 1;
		val = 1.0 / (hw_inc * (1.0 - ratio));
	} else if (ratio > 1.0) {
		corr = hw_inc + 1;
		val = 1.0 / (hw_inc * (ratio-1.0));
	} else {
		val = 0;
		corr = hw_inc;
	}

	if (val >= INT32_MAX) {
		/* Value is too high.
		 * It is not possible to adjust the rate of the clock.
		 */
		mul = 0;
	} else {
		mul = val;
	}


	ENET_Ptp1588AdjustTimer(ENET, corr, mul);

	return 0;
}

static const struct ptp_clock_driver_api api = {
	.set = ptp_clock_mcux_set,
	.get = ptp_clock_mcux_get,
	.adjust = ptp_clock_mcux_adjust,
	.rate_adjust = ptp_clock_mcux_rate_adjust,
};

static int ptp_mcux_init(struct device *port)
{
	struct device *eth_dev = DEVICE_GET(eth_mcux_0);
	struct eth_context *context = eth_dev->driver_data;
	struct ptp_context *ptp_context = port->driver_data;

	context->ptp_clock = port;
	ptp_context->eth_context = context;

	return 0;
}

DEVICE_AND_API_INIT(mcux_ptp_clock_0, PTP_CLOCK_NAME, ptp_mcux_init,
		    &ptp_mcux_0_context, NULL, POST_KERNEL,
		    CONFIG_APPLICATION_INIT_PRIORITY, &api);

#endif /* CONFIG_PTP_CLOCK_MCUX */
