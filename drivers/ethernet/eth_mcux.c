/* MCUX Ethernet Driver
 *
 *  Copyright (c) 2016-2017 ARM Ltd
 *  Copyright (c) 2016 Linaro Ltd
 *  Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_ethernet

/* Driver Limitations:
 *
 * There is no statistics collection for either normal operation or
 * error behaviour.
 */

#define LOG_MODULE_NAME eth_mcux
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#define RING_ID 0

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <device.h>
#include <sys/util.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>
#include <ethernet/eth_stats.h>

#if defined(CONFIG_PTP_CLOCK_MCUX)
#include <ptp_clock.h>
#include <net/gptp.h>
#endif

#if IS_ENABLED(CONFIG_NET_DSA)
#include <net/dsa.h>
#endif

#include "fsl_enet.h"
#include "fsl_phy.h"
#if defined(CONFIG_NET_POWER_MANAGEMENT)
#include "fsl_clock.h"
#include <drivers/clock_control.h>
#endif
#include <devicetree.h>

#include "eth.h"

#define FREESCALE_OUI_B0 0x00
#define FREESCALE_OUI_B1 0x04
#define FREESCALE_OUI_B2 0x9f

#define ETH_MCUX_FIXED_LINK_NODE \
	DT_CHILD(DT_NODELABEL(enet), fixed_link)
#define ETH_MCUX_FIXED_LINK \
	DT_NODE_EXISTS(ETH_MCUX_FIXED_LINK_NODE)
#define ETH_MCUX_FIXED_LINK_SPEED \
	DT_PROP(ETH_MCUX_FIXED_LINK_NODE, speed)
#define ETH_MCUX_FIXED_LINK_FULL_DUPLEX \
	DT_PROP(ETH_MCUX_FIXED_LINK_NODE, full_duplex)

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

#if defined(CONFIG_NET_POWER_MANAGEMENT)
extern uint32_t ENET_GetInstance(ENET_Type * base);
static const clock_ip_name_t enet_clocks[] = ENET_CLOCKS;
#endif

static void eth_mcux_init(const struct device *dev);

#if defined(CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG)
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
#endif

static const char *eth_name(ENET_Type *base)
{
	switch ((int)base) {
	case DT_INST_REG_ADDR(0):
		return DT_INST_LABEL(0);
#if DT_NODE_HAS_STATUS(DT_DRV_INST(1), okay)
	case DT_INST_REG_ADDR(1):
		return DT_INST_LABEL(1);
#endif
	default:
		return "unknown";
	}
}

struct eth_context {
	ENET_Type *base;
	void (*config_func)(void);
	/* If VLAN is enabled, there can be multiple VLAN interfaces related to
	 * this physical device. In that case, this pointer value is not really
	 * used for anything.
	 */
	struct net_if *iface;
#if defined(CONFIG_NET_POWER_MANAGEMENT)
	clock_ip_name_t clock;
	const struct device *clock_dev;
#endif
	enet_handle_t enet_handle;
#if defined(CONFIG_PTP_CLOCK_MCUX)
	const struct device *ptp_clock;
	enet_ptp_config_t ptp_config;
	float clk_ratio;
#endif
	struct k_sem tx_buf_sem;
	enum eth_mcux_phy_state phy_state;
	bool enabled;
	bool link_up;
	uint32_t phy_addr;
	phy_duplex_t phy_duplex;
	phy_speed_t phy_speed;
	uint8_t mac_addr[6];
	void (*generate_mac)(uint8_t *);
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
	uint8_t frame_buf[NET_ETH_MAX_FRAME_SIZE]; /* Max MTU + ethernet header */
};

#if defined(CONFIG_PTP_CLOCK_MCUX)
/* Packets to be timestamped. */
static struct net_pkt *ts_tx_pkt[CONFIG_ETH_MCUX_TX_BUFFERS];
static int ts_tx_rd, ts_tx_wr;
#endif

/* Use ENET_FRAME_MAX_VLANFRAMELEN for VLAN frame size
 * Use ENET_FRAME_MAX_FRAMELEN for Ethernet frame size
 */
#if defined(CONFIG_NET_VLAN)
#if !defined(ENET_FRAME_MAX_VLANFRAMELEN)
#define ENET_FRAME_MAX_VLANFRAMELEN (ENET_FRAME_MAX_FRAMELEN + 4)
#endif
#define ETH_MCUX_BUFFER_SIZE \
	ROUND_UP(ENET_FRAME_MAX_VLANFRAMELEN, ENET_BUFF_ALIGNMENT)
#else
#define ETH_MCUX_BUFFER_SIZE \
	ROUND_UP(ENET_FRAME_MAX_FRAMELEN, ENET_BUFF_ALIGNMENT)
#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_NET_POWER_MANAGEMENT)
static void eth_mcux_phy_enter_reset(struct eth_context *context);
void eth_mcux_phy_stop(struct eth_context *context);

static int eth_mcux_device_pm_control(const struct device *dev,
				      uint32_t command,
				      void *context, device_pm_cb cb, void *arg)
{
	struct eth_context *eth_ctx = (struct eth_context *)dev->data;
	int ret = 0;

	if (!eth_ctx->clock_dev) {
		LOG_ERR("No CLOCK dev");

		ret = -EIO;
		goto out;
	}

	if (command == DEVICE_PM_SET_POWER_STATE) {
		if (*(uint32_t *)context == DEVICE_PM_SUSPEND_STATE) {
			LOG_DBG("Suspending");

			ret = net_if_suspend(eth_ctx->iface);
			if (ret == -EBUSY) {
				goto out;
			}

			eth_mcux_phy_enter_reset(eth_ctx);
			eth_mcux_phy_stop(eth_ctx);

			ENET_Reset(eth_ctx->base);
			ENET_Deinit(eth_ctx->base);
			clock_control_off(eth_ctx->clock_dev,
				(clock_control_subsys_t)eth_ctx->clock);
		} else if (*(uint32_t *)context == DEVICE_PM_ACTIVE_STATE) {
			LOG_DBG("Resuming");

			clock_control_on(eth_ctx->clock_dev,
				(clock_control_subsys_t)eth_ctx->clock);
			eth_mcux_init(dev);
			net_if_resume(eth_ctx->iface);
		}
	} else {
		return -EINVAL;
	}

out:
	if (cb) {
		cb(dev, ret, context, arg);
	}

	return ret;
}

#define ETH_MCUX_PM_FUNC eth_mcux_device_pm_control

#else
#define ETH_MCUX_PM_FUNC device_pm_control_nop
#endif /* CONFIG_NET_POWER_MANAGEMENT */

#if ETH_MCUX_FIXED_LINK
static void eth_mcux_get_phy_params(phy_duplex_t *p_phy_duplex,
				    phy_speed_t *p_phy_speed)
{
	*p_phy_duplex = kPHY_HalfDuplex;
#if ETH_MCUX_FIXED_LINK_FULL_DUPLEX
	*p_phy_duplex = kPHY_FullDuplex;
#endif

	*p_phy_speed = kPHY_Speed10M;
#if ETH_MCUX_FIXED_LINK_SPEED == 100
	*p_phy_speed = kPHY_Speed100M;
#endif
}
#else

static void eth_mcux_decode_duplex_and_speed(uint32_t status,
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
#endif /* ETH_MCUX_FIXED_LINK */

static inline struct net_if *get_iface(struct eth_context *ctx, uint16_t vlan_tag)
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
	/* Reset the PHY. */
#if !defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
	ENET_StartSMIWrite(context->base, context->phy_addr,
			   PHY_BASICCONTROL_REG,
			   kENET_MiiWriteValidFrame,
			   PHY_BCTL_RESET_MASK);
#endif
	context->phy_state = eth_mcux_phy_state_reset;
#if defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
	k_work_submit(&context->phy_work);
#endif
}

static void eth_mcux_phy_start(struct eth_context *context)
{
#if defined(CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG)
	LOG_DBG("%s phy_state=%s", eth_name(context->base),
		phy_state_name(context->phy_state));
#endif

	context->enabled = true;

	switch (context->phy_state) {
	case eth_mcux_phy_state_initial:
		ENET_ActiveRead(context->base);
		/* Reset the PHY. */
#if !defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
		ENET_StartSMIWrite(context->base, context->phy_addr,
				   PHY_BASICCONTROL_REG,
				   kENET_MiiWriteValidFrame,
				   PHY_BCTL_RESET_MASK);
#else
		/*
		 * With no SMI communication one needs to wait for
		 * iface being up by the network core.
		 */
		k_work_submit(&context->phy_work);
		break;
#endif
#if defined(CONFIG_SOC_SERIES_IMX_RT)
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
#if defined(CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG)
	LOG_DBG("%s phy_state=%s", eth_name(context->base),
		phy_state_name(context->phy_state));
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
#if !(defined(CONFIG_ETH_MCUX_NO_PHY_SMI) && ETH_MCUX_FIXED_LINK)
	uint32_t status;
#endif
	bool link_up;
#if defined(CONFIG_SOC_SERIES_IMX_RT)
	status_t res;
	uint32_t ctrl2;
#endif
	phy_duplex_t phy_duplex = kPHY_FullDuplex;
	phy_speed_t phy_speed = kPHY_Speed100M;

#if defined(CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG)
	LOG_DBG("%s phy_state=%s", eth_name(context->base),
		phy_state_name(context->phy_state));
#endif
	switch (context->phy_state) {
	case eth_mcux_phy_state_initial:
#if defined(CONFIG_SOC_SERIES_IMX_RT)
		ENET_DisableInterrupts(context->base, ENET_EIR_MII_MASK);
		res = PHY_Read(context->base, context->phy_addr,
			       PHY_CONTROL2_REG, &ctrl2);
		ENET_EnableInterrupts(context->base, ENET_EIR_MII_MASK);
		if (res != kStatus_Success) {
			LOG_WRN("Reading PHY reg failed (status 0x%x)", res);
			k_work_submit(&context->phy_work);
		} else {
			ctrl2 |= PHY_CTL2_REFCLK_SELECT_MASK;
			ENET_StartSMIWrite(context->base, context->phy_addr,
					   PHY_CONTROL2_REG,
					   kENET_MiiWriteValidFrame,
					   ctrl2);
		}
		context->phy_state = eth_mcux_phy_state_reset;
#endif
#if defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
		/*
		 * When the iface is available proceed with the eth link setup,
		 * otherwise reschedule the eth_mcux_phy_event and check after
		 * 1ms
		 */
		if (context->iface) {
		       context->phy_state = eth_mcux_phy_state_reset;
		}

		k_delayed_work_submit(&context->delayed_phy_work, K_MSEC(1));
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
#if !defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
		ENET_StartSMIWrite(context->base, context->phy_addr,
				   PHY_AUTONEG_ADVERTISE_REG,
				   kENET_MiiWriteValidFrame,
				   (PHY_100BASETX_FULLDUPLEX_MASK |
				    PHY_100BASETX_HALFDUPLEX_MASK |
				    PHY_10BASETX_FULLDUPLEX_MASK |
				    PHY_10BASETX_HALFDUPLEX_MASK | 0x1U));
#endif
		context->phy_state = eth_mcux_phy_state_autoneg;
#if defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
		k_work_submit(&context->phy_work);
#endif
		break;
	case eth_mcux_phy_state_autoneg:
#if !defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
		/* Setup PHY autonegotiation. */
		ENET_StartSMIWrite(context->base, context->phy_addr,
				   PHY_BASICCONTROL_REG,
				   kENET_MiiWriteValidFrame,
				   (PHY_BCTL_AUTONEG_MASK |
				    PHY_BCTL_RESTART_AUTONEG_MASK));
#endif
		context->phy_state = eth_mcux_phy_state_restart;
#if defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
		k_work_submit(&context->phy_work);
#endif
		break;
	case eth_mcux_phy_state_wait:
	case eth_mcux_phy_state_restart:
		/* Start reading the PHY basic status. */
#if !defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
		ENET_StartSMIRead(context->base, context->phy_addr,
				  PHY_BASICSTATUS_REG,
				  kENET_MiiReadValidFrame);
#endif
		context->phy_state = eth_mcux_phy_state_read_status;
#if defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
		k_work_submit(&context->phy_work);
#endif
		break;
	case eth_mcux_phy_state_read_status:
		/* PHY Basic status is available. */
#if defined(CONFIG_ETH_MCUX_NO_PHY_SMI) && ETH_MCUX_FIXED_LINK
		link_up = true;
#else
		status = ENET_ReadSMIData(context->base);
		link_up =  status & PHY_BSTATUS_LINKSTATUS_MASK;
#endif
		if (link_up && !context->link_up) {
			/* Start reading the PHY control register. */
#if !defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
			ENET_StartSMIRead(context->base, context->phy_addr,
					  PHY_CONTROL1_REG,
					  kENET_MiiReadValidFrame);
#endif
			context->link_up = link_up;
			context->phy_state = eth_mcux_phy_state_read_duplex;

			/* Network interface might be NULL at this point */
			if (context->iface) {
				net_eth_carrier_on(context->iface);
				k_msleep(USEC_PER_MSEC);
			}
#if defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
			k_work_submit(&context->phy_work);
#endif
		} else if (!link_up && context->link_up) {
			LOG_INF("%s link down", eth_name(context->base));
			context->link_up = link_up;
			k_delayed_work_submit(&context->delayed_phy_work,
					K_MSEC(CONFIG_ETH_MCUX_PHY_TICK_MS));
			context->phy_state = eth_mcux_phy_state_wait;
			net_eth_carrier_off(context->iface);
		} else {
			k_delayed_work_submit(&context->delayed_phy_work,
					K_MSEC(CONFIG_ETH_MCUX_PHY_TICK_MS));
			context->phy_state = eth_mcux_phy_state_wait;
		}

		break;
	case eth_mcux_phy_state_read_duplex:
		/* PHY control register is available. */
#if defined(CONFIG_ETH_MCUX_NO_PHY_SMI) && ETH_MCUX_FIXED_LINK
		eth_mcux_get_phy_params(&phy_duplex, &phy_speed);
		LOG_INF("%s - Fixed Link", eth_name(context->base));
#else
		status = ENET_ReadSMIData(context->base);
		eth_mcux_decode_duplex_and_speed(status,
						 &phy_duplex,
						 &phy_speed);
#endif
		if (phy_speed != context->phy_speed ||
		    phy_duplex != context->phy_duplex) {
			context->phy_speed = phy_speed;
			context->phy_duplex = phy_duplex;
			ENET_SetMII(context->base,
				    (enet_mii_speed_t) phy_speed,
				    (enet_mii_duplex_t) phy_duplex);
		}

		LOG_INF("%s enabled %sM %s-duplex mode.",
			eth_name(context->base),
			(phy_speed ? "100" : "10"),
			(phy_duplex ? "full" : "half"));
		k_delayed_work_submit(&context->delayed_phy_work,
				      K_MSEC(CONFIG_ETH_MCUX_PHY_TICK_MS));
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

static void eth_mcux_phy_setup(struct eth_context *context)
{
#if defined(CONFIG_SOC_SERIES_IMX_RT)
	status_t res;
	uint32_t oms_override;

	/* Disable MII interrupts to prevent triggering PHY events. */
	ENET_DisableInterrupts(context->base, ENET_EIR_MII_MASK);

	res = PHY_Read(context->base, context->phy_addr,
		       PHY_OMS_OVERRIDE_REG, &oms_override);
	if (res != kStatus_Success) {
		LOG_WRN("Reading PHY reg failed (status 0x%x)", res);
	} else {
		/* Based on strap-in pins the PHY can be in factory test mode.
		 * Force normal operation.
		 */
		oms_override &= ~PHY_OMS_FACTORY_MODE_MASK;

		/* Prevent PHY entering NAND Tree mode override. */
		if (oms_override & PHY_OMS_NANDTREE_MASK) {
			oms_override &= ~PHY_OMS_NANDTREE_MASK;
		}

		res = PHY_Write(context->base, context->phy_addr,
				PHY_OMS_OVERRIDE_REG, oms_override);
		if (res != kStatus_Success) {
			LOG_WRN("Writing PHY reg failed (status 0x%x)", res);
		}
	}

	ENET_EnableInterrupts(context->base, ENET_EIR_MII_MASK);
#endif
}

#if defined(CONFIG_PTP_CLOCK_MCUX)

static bool eth_get_ptp_data(struct net_if *iface, struct net_pkt *pkt)
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

	return true;
}
#endif /* CONFIG_PTP_CLOCK_MCUX */

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_context *context = dev->data;
	uint16_t total_len = net_pkt_get_len(pkt);
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

#if defined(CONFIG_PTP_CLOCK_MCUX)
	timestamped_frame = eth_get_ptp_data(net_pkt_iface(pkt), pkt);
	if (timestamped_frame) {
		status = ENET_SendFrame(context->base, &context->enet_handle,
					  context->frame_buf, total_len, RING_ID, true, NULL);

		if (!status) {
			ts_tx_pkt[ts_tx_wr] = net_pkt_ref(pkt);
		} else {
			ts_tx_pkt[ts_tx_wr] = NULL;
		}

		ts_tx_wr++;
		if (ts_tx_wr >= CONFIG_ETH_MCUX_TX_BUFFERS) {
			ts_tx_wr = 0;
		}
	} else
#endif
	{
		status = ENET_SendFrame(context->base, &context->enet_handle,
					context->frame_buf, total_len, RING_ID, false, NULL);
	}

	irq_unlock(imask);

	if (status) {
		LOG_ERR("ENET_SendFrame error: %d", (int)status);
		return -1;
	}

	k_sem_take(&context->tx_buf_sem, K_FOREVER);

	return 0;
}

static void eth_rx(struct eth_context *context)
{
	uint16_t vlan_tag = NET_VLAN_TAG_UNSPEC;
	uint32_t frame_length = 0U;
	struct net_if *iface;
	struct net_pkt *pkt;
	status_t status;
	unsigned int imask;
	uint32_t ts;

#if defined(CONFIG_PTP_CLOCK_MCUX)
	enet_ptp_time_t ptpTimeData;
#endif

	status = ENET_GetRxFrameSize(&context->enet_handle,
				     (uint32_t *)&frame_length, RING_ID);
	if (status) {
		enet_data_error_stats_t error_stats;

		LOG_ERR("ENET_GetRxFrameSize return: %d", (int)status);

		ENET_GetRxErrBeforeReadFrame(&context->enet_handle,
					     &error_stats, RING_ID);
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

	status = ENET_ReadFrame(context->base, &context->enet_handle,
				context->frame_buf, frame_length, RING_ID, &ts);
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
#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_PTP_CLOCK_MCUX)
	if (eth_get_ptp_data(get_iface(context, vlan_tag), pkt)) {
		ENET_Ptp1588GetTimer(context->base, &context->enet_handle,
					&ptpTimeData);
		/* If latest timestamp reloads after getting from Rx BD,
		 * then second - 1 to make sure the actual Rx timestamp is
		 * accurate
		 */
		if (ptpTimeData.nanosecond < ts) {
			ptpTimeData.second--;
		}

		pkt->timestamp.nanosecond = ptpTimeData.nanosecond;
		pkt->timestamp.second = ptpTimeData.second;
	} else {
		/* Invalid value. */
		pkt->timestamp.nanosecond = UINT32_MAX;
		pkt->timestamp.second = UINT64_MAX;
	}
#endif /* CONFIG_PTP_CLOCK_MCUX */

	irq_unlock(imask);

	iface = get_iface(context, vlan_tag);
#if IS_ENABLED(CONFIG_NET_DSA)
	iface = dsa_net_recv(iface, &pkt);
#endif
	if (net_recv_data(iface, pkt) < 0) {
		net_pkt_unref(pkt);
		goto error;
	}

	return;
flush:
	/* Flush the current read buffer.  This operation can
	 * only report failure if there is no frame to flush,
	 * which cannot happen in this context.
	 */
	status = ENET_ReadFrame(context->base, &context->enet_handle, NULL,
					0, RING_ID, NULL);
	__ASSERT_NO_MSG(status == kStatus_Success);
error:
	eth_stats_update_errors_rx(get_iface(context, vlan_tag));
}

#if defined(CONFIG_PTP_CLOCK_MCUX)
static inline void ts_register_tx_event(struct eth_context *context,
					 enet_frame_info_t *frameinfo)
{
	struct net_pkt *pkt;

	pkt = ts_tx_pkt[ts_tx_rd];
	if (pkt && atomic_get(&pkt->atomic_ref) > 0) {
		if (eth_get_ptp_data(net_pkt_iface(pkt), pkt)) {
			if (frameinfo->isTsAvail) {
				pkt->timestamp.nanosecond =
					frameinfo->timeStamp.nanosecond;
				pkt->timestamp.second =
					frameinfo->timeStamp.second;

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
#endif /* CONFIG_PTP_CLOCK_MCUX */

static void eth_callback(ENET_Type *base, enet_handle_t *handle,
#if FSL_FEATURE_ENET_QUEUE > 1
			 uint32_t ringId,
#endif /* FSL_FEATURE_ENET_QUEUE > 1 */
			 enet_event_t event, enet_frame_info_t *frameinfo, void *param)
{
	struct eth_context *context = param;

	switch (event) {
	case kENET_RxEvent:
		eth_rx(context);
		break;
	case kENET_TxEvent:
#if defined(CONFIG_PTP_CLOCK_MCUX)
		/* Register event */
		ts_register_tx_event(context, frameinfo);
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
		context->base->ATPER = NSEC_PER_SEC;
		break;
	case kENET_TimeStampAvailEvent:
		/* Time stamp available event.  */
		break;
	}
}

static void eth_mcux_init(const struct device *dev)
{
	struct eth_context *context = dev->data;
	const enet_buffer_config_t *buffer_config = dev->config;
	enet_config_t enet_config;
	uint32_t sys_clock;
#if defined(CONFIG_PTP_CLOCK_MCUX)
	uint8_t ptp_multicast[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E };
#endif
#if defined(CONFIG_MDNS_RESPONDER) || defined(CONFIG_MDNS_RESOLVER)
	/* standard multicast MAC address */
	uint8_t mdns_multicast[6] = { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB };
#endif

	context->phy_state = eth_mcux_phy_state_initial;

	sys_clock = CLOCK_GetFreq(kCLOCK_CoreSysClk);

	ENET_GetDefaultConfig(&enet_config);
	enet_config.interrupt |= kENET_RxFrameInterrupt;
	enet_config.interrupt |= kENET_TxFrameInterrupt;
#if !defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
	enet_config.interrupt |= kENET_MiiInterrupt;
#endif

	if (IS_ENABLED(CONFIG_ETH_MCUX_PROMISCUOUS_MODE)) {
		enet_config.macSpecialConfig |= kENET_ControlPromiscuousEnable;
	}

	if (IS_ENABLED(CONFIG_NET_VLAN)) {
		enet_config.macSpecialConfig |= kENET_ControlVLANTagEnable;
	}

	if (IS_ENABLED(CONFIG_ETH_MCUX_HW_ACCELERATION)) {
		enet_config.txAccelerConfig |=
			kENET_TxAccelIpCheckEnabled |
			kENET_TxAccelProtoCheckEnabled;
		enet_config.rxAccelerConfig |=
			kENET_RxAccelIpCheckEnabled |
			kENET_RxAccelProtoCheckEnabled;
	}

	ENET_Init(context->base,
		  &context->enet_handle,
		  &enet_config,
		  buffer_config,
		  context->mac_addr,
		  sys_clock);

#if defined(CONFIG_PTP_CLOCK_MCUX)
	ENET_AddMulticastGroup(context->base, ptp_multicast);

	context->ptp_config.channel = kENET_PtpTimerChannel1;
	context->ptp_config.ptp1588ClockSrc_Hz =
					CONFIG_ETH_MCUX_PTP_CLOCK_SRC_HZ;
	context->clk_ratio = 1.0;

	ENET_Ptp1588Configure(context->base, &context->enet_handle,
			      &context->ptp_config);
#endif

#if defined(CONFIG_MDNS_RESPONDER) || defined(CONFIG_MDNS_RESOLVER)
	ENET_AddMulticastGroup(context->base, mdns_multicast);
#endif

#if !defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
	ENET_SetSMI(context->base, sys_clock, false);
#endif

	/* handle PHY setup after SMI initialization */
	eth_mcux_phy_setup(context);

#if defined(CONFIG_PTP_CLOCK_MCUX)
	/* Enable reclaim of tx descriptors that will have the tx timestamp */
	ENET_SetTxReclaim(&context->enet_handle, true, 0);
#endif
	ENET_SetCallback(&context->enet_handle, eth_callback, context);

	eth_mcux_phy_start(context);
}

static int eth_init(const struct device *dev)
{
	struct eth_context *context = dev->data;

#if defined(CONFIG_PTP_CLOCK_MCUX)
	ts_tx_rd = 0;
	ts_tx_wr = 0;
	(void)memset(ts_tx_pkt, 0, sizeof(ts_tx_pkt));
#endif

#if defined(CONFIG_NET_POWER_MANAGEMENT)
	const uint32_t inst = ENET_GetInstance(context->base);

	context->clock = enet_clocks[inst];
#endif

	k_sem_init(&context->tx_buf_sem,
		   0, CONFIG_ETH_MCUX_TX_BUFFERS);
	k_work_init(&context->phy_work, eth_mcux_phy_work);
	k_delayed_work_init(&context->delayed_phy_work,
			    eth_mcux_delayed_phy_work);

	if (context->generate_mac) {
		context->generate_mac(context->mac_addr);
	}

	eth_mcux_init(dev);

	LOG_DBG("%s MAC %02x:%02x:%02x:%02x:%02x:%02x",
		dev->name,
		context->mac_addr[0], context->mac_addr[1],
		context->mac_addr[2], context->mac_addr[3],
		context->mac_addr[4], context->mac_addr[5]);

	return 0;
}

#if defined(CONFIG_NET_IPV6)
static void net_if_mcast_cb(struct net_if *iface,
			    const struct in6_addr *addr,
			    bool is_joined)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->data;
	struct net_eth_addr mac_addr;

	net_eth_ipv6_mcast_to_mac_addr(addr, &mac_addr);

	if (is_joined) {
		ENET_AddMulticastGroup(context->base, mac_addr.addr);
	} else {
		ENET_LeaveMulticastGroup(context->base, mac_addr.addr);
	}
}
#endif /* CONFIG_NET_IPV6 */

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->data;

#if defined(CONFIG_NET_IPV6)
	static struct net_if_mcast_monitor mon;

	net_if_mcast_mon_register(&mon, iface, net_if_mcast_cb);
#endif /* CONFIG_NET_IPV6 */

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	/* For VLAN, this value is only used to get the correct L2 driver.
	 * The iface pointer in context should contain the main interface
	 * if the VLANs are enabled.
	 */
	if (context->iface == NULL) {
		context->iface = iface;
	}

#if IS_ENABLED(CONFIG_NET_DSA)
	dsa_register_master_tx(iface, &eth_tx);
#endif
	ethernet_init(iface);
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	context->config_func();
}

static enum ethernet_hw_caps eth_mcux_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_HW_VLAN | ETHERNET_LINK_10BASE_T |
#if defined(CONFIG_PTP_CLOCK_MCUX)
		ETHERNET_PTP |
#endif
#if IS_ENABLED(CONFIG_NET_DSA)
		ETHERNET_DSA_MASTER_PORT |
#endif
#if defined(CONFIG_ETH_MCUX_HW_ACCELERATION)
		ETHERNET_HW_TX_CHKSUM_OFFLOAD |
		ETHERNET_HW_RX_CHKSUM_OFFLOAD |
#endif
		ETHERNET_AUTO_NEGOTIATION_SET |
		ETHERNET_LINK_100BASE_T;
}

static int eth_mcux_set_config(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	struct eth_context *context = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(context->mac_addr,
		       config->mac_address.addr,
		       sizeof(context->mac_addr));
		ENET_SetMacAddr(context->base, context->mac_addr);
		net_if_set_link_addr(context->iface, context->mac_addr,
				     sizeof(context->mac_addr),
				     NET_LINK_ETHERNET);
		LOG_DBG("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x",
			dev->name,
			context->mac_addr[0], context->mac_addr[1],
			context->mac_addr[2], context->mac_addr[3],
			context->mac_addr[4], context->mac_addr[5]);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

#if defined(CONFIG_PTP_CLOCK_MCUX)
static const struct device *eth_mcux_get_ptp_clock(const struct device *dev)
{
	struct eth_context *context = dev->data;

	return context->ptp_clock;
}
#endif

static const struct ethernet_api api_funcs = {
	.iface_api.init		= eth_iface_init,
#if defined(CONFIG_PTP_CLOCK_MCUX)
	.get_ptp_clock		= eth_mcux_get_ptp_clock,
#endif
	.get_capabilities	= eth_mcux_get_capabilities,
	.set_config		= eth_mcux_set_config,
#if IS_ENABLED(CONFIG_NET_DSA)
	.send                   = dsa_tx,
#else
	.send			= eth_tx,
#endif
};

#if defined(CONFIG_PTP_CLOCK_MCUX)
static void eth_mcux_ptp_isr(const struct device *dev)
{
	struct eth_context *context = dev->data;

	ENET_TimeStampIRQHandler(context->base, &context->enet_handle);
}
#endif

#if DT_INST_IRQ_HAS_NAME(0, common) || DT_INST_IRQ_HAS_NAME(1, common)
static void eth_mcux_common_isr(const struct device *dev)
{
	struct eth_context *context = dev->data;
	uint32_t EIR = ENET_GetInterruptStatus(context->base);
	int irq_lock_key = irq_lock();

	if (EIR & (kENET_RxBufferInterrupt | kENET_RxFrameInterrupt)) {
		ENET_ReceiveIRQHandler(context->base, &context->enet_handle);
	} else if (EIR & (kENET_TxBufferInterrupt | kENET_TxFrameInterrupt)) {
		ENET_TransmitIRQHandler(context->base, &context->enet_handle);
	} else if (EIR & ENET_EIR_MII_MASK) {
		k_work_submit(&context->phy_work);
		ENET_ClearInterruptStatus(context->base, kENET_MiiInterrupt);
	} else if (EIR) {
		ENET_ClearInterruptStatus(context->base, 0xFFFFFFFF);
	}

	irq_unlock(irq_lock_key);
}
#endif

#if DT_INST_IRQ_HAS_NAME(0, rx) || DT_INST_IRQ_HAS_NAME(1, rx)
static void eth_mcux_rx_isr(const struct device *dev)
{
	struct eth_context *context = dev->data;

	ENET_ReceiveIRQHandler(context->base, &context->enet_handle);
}
#endif

#if DT_INST_IRQ_HAS_NAME(0, tx) || DT_INST_IRQ_HAS_NAME(1, tx)
static void eth_mcux_tx_isr(const struct device *dev)
{
	struct eth_context *context = dev->data;

	ENET_TransmitIRQHandler(context->base, &context->enet_handle);
}
#endif

#if DT_INST_IRQ_HAS_NAME(0, err) || DT_INST_IRQ_HAS_NAME(1, err)
static void eth_mcux_err_isr(const struct device *dev)
{
	struct eth_context *context = dev->data;
	uint32_t pending = ENET_GetInterruptStatus(context->base);

	if (pending & ENET_EIR_MII_MASK) {
		k_work_submit(&context->phy_work);
		ENET_ClearInterruptStatus(context->base, kENET_MiiInterrupt);
	}
}
#endif

#if defined(CONFIG_NOCACHE_MEMORY)
#define NOCACHE __nocache
#else
#define NOCACHE
#endif

#if defined(CONFIG_SOC_SERIES_IMX_RT)
#define ETH_MCUX_UNIQUE_ID	(OCOTP->CFG1 ^ OCOTP->CFG2)
#elif defined(CONFIG_SOC_SERIES_KINETIS_K6X)
#define ETH_MCUX_UNIQUE_ID	(SIM->UIDH ^ SIM->UIDMH ^ SIM->UIDML ^ SIM->UIDL)
#else
#error "Unsupported SOC"
#endif

#define ETH_MCUX_NONE

#define ETH_MCUX_IRQ_INIT(n, name)					\
	do {								\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, name, irq),		\
			    DT_INST_IRQ_BY_NAME(n, name, priority),	\
			    eth_mcux_##name##_isr,			\
			    DEVICE_DT_INST_GET(n),			\
			    0);						\
		irq_enable(DT_INST_IRQ_BY_NAME(n, name, irq));		\
	} while (0)

#define ETH_MCUX_IRQ(n, name)						\
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(n, name),			\
		    (ETH_MCUX_IRQ_INIT(n, name)),			\
		    (ETH_MCUX_NONE))

#if defined(CONFIG_PTP_CLOCK_MCUX)
#define PTP_INST_NODEID(n) DT_CHILD(DT_DRV_INST(n), ptp)

#define ETH_MCUX_IRQ_PTP_INIT(n)							\
	do {										\
		IRQ_CONNECT(DT_IRQ_BY_NAME(PTP_INST_NODEID(n), ieee1588_tmr, irq),	\
			    DT_IRQ_BY_NAME(PTP_INST_NODEID(n), ieee1588_tmr, priority),	\
			    eth_mcux_ptp_isr,						\
			    DEVICE_DT_INST_GET(n),					\
			    0);								\
		irq_enable(DT_IRQ_BY_NAME(PTP_INST_NODEID(n), ieee1588_tmr, irq));	\
	} while (0)

#define ETH_MCUX_IRQ_PTP(n)						\
	COND_CODE_1(DT_NODE_HAS_STATUS(PTP_INST_NODEID(n), okay),	\
		    (ETH_MCUX_IRQ_PTP_INIT(n)),				\
		    (ETH_MCUX_NONE))

#define ETH_MCUX_PTP_FRAMEINFO_ARRAY(n)					\
	static enet_frame_info_t					\
		eth##n##_tx_frameinfo_array[CONFIG_ETH_MCUX_TX_BUFFERS];

#define ETH_MCUX_PTP_FRAMEINFO(n)					\
	.txFrameInfo = eth##n##_tx_frameinfo_array,
#else
#define ETH_MCUX_IRQ_PTP(n)

#define ETH_MCUX_PTP_FRAMEINFO_ARRAY(n)

#define ETH_MCUX_PTP_FRAMEINFO(n)					\
	.txFrameInfo = NULL,
#endif

#define ETH_MCUX_GENERATE_MAC_RANDOM(n)					\
	static void generate_eth##n##_mac(uint8_t *mac_addr)		\
	{								\
		gen_random_mac(mac_addr,				\
			       FREESCALE_OUI_B0,			\
			       FREESCALE_OUI_B1,			\
			       FREESCALE_OUI_B2);			\
	}

#define ETH_MCUX_GENERATE_MAC_UNIQUE(n)					\
	static void generate_eth##n##_mac(uint8_t *mac_addr)		\
	{								\
		uint32_t id = ETH_MCUX_UNIQUE_ID;			\
									\
		mac_addr[0] = FREESCALE_OUI_B0;				\
		mac_addr[0] |= 0x02; /* force LAA bit */		\
		mac_addr[1] = FREESCALE_OUI_B1;				\
		mac_addr[2] = FREESCALE_OUI_B2;				\
		mac_addr[3] = id >> 8;					\
		mac_addr[4] = id >> 16;					\
		mac_addr[5] = id >> 0;					\
		mac_addr[5] += n;					\
	}

#define ETH_MCUX_GENERATE_MAC(n)					\
	COND_CODE_1(DT_INST_PROP(n, zephyr_random_mac_address),		\
		    (ETH_MCUX_GENERATE_MAC_RANDOM(n)),			\
		    (ETH_MCUX_GENERATE_MAC_UNIQUE(n)))

#define ETH_MCUX_MAC_ADDR_LOCAL(n)					\
	.mac_addr = DT_INST_PROP(n, local_mac_address),			\
	.generate_mac = NULL,

#define ETH_MCUX_MAC_ADDR_GENERATE(n)					\
	.mac_addr = {0},						\
	.generate_mac = generate_eth##n##_mac,

#define ETH_MCUX_MAC_ADDR(n)						\
	COND_CODE_1(ETH_MCUX_MAC_ADDR_TO_BOOL(n),			\
		    (ETH_MCUX_MAC_ADDR_LOCAL(n)),			\
		    (ETH_MCUX_MAC_ADDR_GENERATE(n)))

#define ETH_MCUX_POWER_INIT(n)						\
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\

#define ETH_MCUX_POWER(n)						\
	COND_CODE_1(CONFIG_NET_POWER_MANAGEMENT,			\
		    (ETH_MCUX_POWER_INIT(n)),				\
		    (ETH_MCUX_NONE))
#define ETH_MCUX_GEN_MAC(n)                                             \
	COND_CODE_0(ETH_MCUX_MAC_ADDR_TO_BOOL(n),                       \
		    (ETH_MCUX_GENERATE_MAC(n)),                         \
		    (ETH_MCUX_NONE))

/*
 * In the below code we explicitly define
 * ETH_MCUX_MAC_ADDR_TO_BOOL_0 for the '0' instance of enet driver.
 *
 * For instance N one shall add definition for ETH_MCUX_MAC_ADDR_TO_BOOL_N
 */
#if (NODE_HAS_VALID_MAC_ADDR(DT_DRV_INST(0))) == 0
#define ETH_MCUX_MAC_ADDR_TO_BOOL_0 0
#else
#define ETH_MCUX_MAC_ADDR_TO_BOOL_0 1
#endif
#define ETH_MCUX_MAC_ADDR_TO_BOOL(n) ETH_MCUX_MAC_ADDR_TO_BOOL_##n

#define ETH_MCUX_INIT(n)						\
	ETH_MCUX_GEN_MAC(n)                                             \
									\
	static void eth##n##_config_func(void);				\
									\
	static struct eth_context eth##n##_context = {			\
		.base = (ENET_Type *)DT_INST_REG_ADDR(n),		\
		.config_func = eth##n##_config_func,			\
		.phy_addr = 0U,						\
		.phy_duplex = kPHY_FullDuplex,				\
		.phy_speed = kPHY_Speed100M,				\
		ETH_MCUX_MAC_ADDR(n)					\
		ETH_MCUX_POWER(n)					\
	};								\
									\
	static NOCACHE __aligned(ENET_BUFF_ALIGNMENT)			\
		enet_rx_bd_struct_t					\
		eth##n##_rx_buffer_desc[CONFIG_ETH_MCUX_RX_BUFFERS];	\
									\
	static NOCACHE __aligned(ENET_BUFF_ALIGNMENT)			\
		enet_tx_bd_struct_t					\
		eth##n##_tx_buffer_desc[CONFIG_ETH_MCUX_TX_BUFFERS];	\
									\
	static uint8_t __aligned(ENET_BUFF_ALIGNMENT)			\
		eth##n##_rx_buffer[CONFIG_ETH_MCUX_RX_BUFFERS]		\
				  [ETH_MCUX_BUFFER_SIZE];		\
									\
	static uint8_t __aligned(ENET_BUFF_ALIGNMENT)			\
		eth##n##_tx_buffer[CONFIG_ETH_MCUX_TX_BUFFERS]		\
				  [ETH_MCUX_BUFFER_SIZE];		\
									\
	ETH_MCUX_PTP_FRAMEINFO_ARRAY(n)					\
									\
	static const enet_buffer_config_t eth##n##_buffer_config = {	\
		.rxBdNumber = CONFIG_ETH_MCUX_RX_BUFFERS,		\
		.txBdNumber = CONFIG_ETH_MCUX_TX_BUFFERS,		\
		.rxBuffSizeAlign = ETH_MCUX_BUFFER_SIZE,		\
		.txBuffSizeAlign = ETH_MCUX_BUFFER_SIZE,		\
		.rxBdStartAddrAlign = eth##n##_rx_buffer_desc,		\
		.txBdStartAddrAlign = eth##n##_tx_buffer_desc,		\
		.rxBufferAlign = eth##n##_rx_buffer[0],			\
		.txBufferAlign = eth##n##_tx_buffer[0],			\
		.rxMaintainEnable = true,				\
		.txMaintainEnable = true,				\
		ETH_MCUX_PTP_FRAMEINFO(n)				\
	};								\
									\
	ETH_NET_DEVICE_DT_INST_DEFINE(n,					\
			    eth_init,					\
			    ETH_MCUX_PM_FUNC,				\
			    &eth##n##_context,				\
			    &eth##n##_buffer_config,			\
			    CONFIG_ETH_INIT_PRIORITY,			\
			    &api_funcs,					\
			    NET_ETH_MTU);				\
									\
	static void eth##n##_config_func(void)				\
	{								\
		ETH_MCUX_IRQ(n, rx);					\
		ETH_MCUX_IRQ(n, tx);					\
		ETH_MCUX_IRQ(n, err);					\
		ETH_MCUX_IRQ(n, common);				\
		ETH_MCUX_IRQ_PTP(n);					\
	}								\

DT_INST_FOREACH_STATUS_OKAY(ETH_MCUX_INIT)

#if defined(CONFIG_PTP_CLOCK_MCUX)
struct ptp_context {
	struct eth_context *eth_context;
};

static struct ptp_context ptp_mcux_0_context;

static int ptp_clock_mcux_set(const struct device *dev,
			      struct net_ptp_time *tm)
{
	struct ptp_context *ptp_context = dev->data;
	struct eth_context *context = ptp_context->eth_context;
	enet_ptp_time_t enet_time;

	enet_time.second = tm->second;
	enet_time.nanosecond = tm->nanosecond;

	ENET_Ptp1588SetTimer(context->base, &context->enet_handle, &enet_time);
	return 0;
}

static int ptp_clock_mcux_get(const struct device *dev,
			      struct net_ptp_time *tm)
{
	struct ptp_context *ptp_context = dev->data;
	struct eth_context *context = ptp_context->eth_context;
	enet_ptp_time_t enet_time;

	ENET_Ptp1588GetTimer(context->base, &context->enet_handle, &enet_time);

	tm->second = enet_time.second;
	tm->nanosecond = enet_time.nanosecond;
	return 0;
}

static int ptp_clock_mcux_adjust(const struct device *dev, int increment)
{
	struct ptp_context *ptp_context = dev->data;
	struct eth_context *context = ptp_context->eth_context;
	int key, ret;

	ARG_UNUSED(dev);

	if ((increment <= -NSEC_PER_SEC) || (increment >= NSEC_PER_SEC)) {
		ret = -EINVAL;
	} else {
		key = irq_lock();
		if (context->base->ATPER != NSEC_PER_SEC) {
			ret = -EBUSY;
		} else {
			/* Seconds counter is handled by software. Change the
			 * period of one software second to adjust the clock.
			 */
			context->base->ATPER = NSEC_PER_SEC - increment;
			ret = 0;
		}
		irq_unlock(key);
	}

	return ret;
}

static int ptp_clock_mcux_rate_adjust(const struct device *dev, float ratio)
{
	const int hw_inc = NSEC_PER_SEC / CONFIG_ETH_MCUX_PTP_CLOCK_SRC_HZ;
	struct ptp_context *ptp_context = dev->data;
	struct eth_context *context = ptp_context->eth_context;
	int corr;
	int32_t mul;
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

	ENET_Ptp1588AdjustTimer(context->base, corr, mul);

	return 0;
}

static const struct ptp_clock_driver_api api = {
	.set = ptp_clock_mcux_set,
	.get = ptp_clock_mcux_get,
	.adjust = ptp_clock_mcux_adjust,
	.rate_adjust = ptp_clock_mcux_rate_adjust,
};

static int ptp_mcux_init(const struct device *port)
{
	const struct device *eth_dev = DEVICE_DT_GET(DT_NODELABEL(enet));
	struct eth_context *context = eth_dev->data;
	struct ptp_context *ptp_context = port->data;

	context->ptp_clock = port;
	ptp_context->eth_context = context;

	return 0;
}

DEVICE_DEFINE(mcux_ptp_clock_0, PTP_CLOCK_NAME, ptp_mcux_init,
		device_pm_control_nop, &ptp_mcux_0_context, NULL, POST_KERNEL,
		CONFIG_APPLICATION_INIT_PRIORITY, &api);

#endif /* CONFIG_PTP_CLOCK_MCUX */
