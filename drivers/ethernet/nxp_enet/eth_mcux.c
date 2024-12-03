/* MCUX Ethernet Driver
 *
 *  Copyright (c) 2016-2017 ARM Ltd
 *  Copyright (c) 2016 Linaro Ltd
 *  Copyright (c) 2018 Intel Corporation
 *  Copyright 2023 NXP
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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>
#include <zephyr/pm/device.h>
#include <zephyr/irq.h>

#if defined(CONFIG_PTP_CLOCK_MCUX)
#include <zephyr/drivers/ptp_clock.h>
#endif

#if defined(CONFIG_NET_DSA)
#include <zephyr/net/dsa.h>
#endif

#include "fsl_enet.h"
#include "fsl_phy.h"
#include "fsl_phyksz8081.h"
#include "fsl_enet_mdio.h"
#if defined(CONFIG_NET_POWER_MANAGEMENT)
#include "fsl_clock.h"
#include <zephyr/drivers/clock_control.h>
#endif
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>
#endif

#include "../eth.h"

#define PHY_OMS_OVERRIDE_REG 0x16U      /* The PHY Operation Mode Strap Override register. */
#define PHY_OMS_STATUS_REG 0x17U        /* The PHY Operation Mode Strap Status register. */

#define PHY_OMS_NANDTREE_MASK 0x0020U     /* The PHY NAND Tree Strap-In Override/Status mask. */
#define PHY_OMS_FACTORY_MODE_MASK 0x8000U /* The factory mode Override/Status mask. */

/* Defines the PHY KSZ8081 vendor defined registers. */
#define PHY_CONTROL1_REG 0x1EU /* The PHY control one register. */
#define PHY_CONTROL2_REG 0x1FU /* The PHY control two register. */

/* Defines the PHY KSZ8081 ID number. */
#define PHY_CONTROL_ID1 0x22U /* The PHY ID1 */

/* Defines the mask flag of operation mode in control registers */
#define PHY_CTL2_REMOTELOOP_MASK    0x0004U /* The PHY remote loopback mask. */
#define PHY_CTL2_REFCLK_SELECT_MASK 0x0080U /* The PHY RMII reference clock select. */
#define PHY_CTL1_10HALFDUPLEX_MASK  0x0001U /* The PHY 10M half duplex mask. */
#define PHY_CTL1_100HALFDUPLEX_MASK 0x0002U /* The PHY 100M half duplex mask. */
#define PHY_CTL1_10FULLDUPLEX_MASK  0x0005U /* The PHY 10M full duplex mask. */
#define PHY_CTL1_100FULLDUPLEX_MASK 0x0006U /* The PHY 100M full duplex mask. */
#define PHY_CTL1_SPEEDUPLX_MASK     0x0007U /* The PHY speed and duplex mask. */
#define PHY_CTL1_ENERGYDETECT_MASK  0x10U   /* The PHY signal present on rx differential pair. */
#define PHY_CTL1_LINKUP_MASK        0x100U  /* The PHY link up. */
#define PHY_LINK_READY_MASK         (PHY_CTL1_ENERGYDETECT_MASK | PHY_CTL1_LINKUP_MASK)

/* Defines the timeout macro. */
#define PHY_READID_TIMEOUT_COUNT 1000U

/* Define RX and TX thread stack sizes */
#define ETH_MCUX_RX_THREAD_STACK_SIZE 1600
#define ETH_MCUX_TX_THREAD_STACK_SIZE 1600

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

struct _phy_resource {
	mdioWrite write;
	mdioRead read;
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
		return "ETH_0";
#if DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(1))
	case DT_INST_REG_ADDR(1):
		return "ETH_1";
#endif
	default:
		return "unknown";
	}
}

struct eth_context {
	ENET_Type *base;
	void (*config_func)(void);
	struct net_if *iface;
#if defined(CONFIG_NET_POWER_MANAGEMENT)
	clock_ip_name_t clock;
	const struct device *clock_dev;
#endif
	enet_handle_t enet_handle;
#if defined(CONFIG_PTP_CLOCK_MCUX)
	const struct device *ptp_clock;
	enet_ptp_config_t ptp_config;
	double clk_ratio;
	struct k_mutex ptp_mutex;
	struct k_sem ptp_ts_sem;
#endif
	struct k_sem tx_buf_sem;
	phy_handle_t *phy_handle;
	struct _phy_resource *phy_config;
	struct k_sem rx_thread_sem;
	enum eth_mcux_phy_state phy_state;
	bool enabled;
	bool link_up;
	uint32_t phy_addr;
	uint32_t rx_irq_num;
	uint32_t tx_irq_num;
	phy_duplex_t phy_duplex;
	phy_speed_t phy_speed;
	uint8_t mac_addr[6];
	void (*generate_mac)(uint8_t *);
	struct k_work phy_work;
	struct k_work_delayable delayed_phy_work;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, ETH_MCUX_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;

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
	struct k_mutex tx_frame_buf_mutex;
	struct k_mutex rx_frame_buf_mutex;
	uint8_t *tx_frame_buf; /* Max MTU + ethernet header */
	uint8_t *rx_frame_buf; /* Max MTU + ethernet header */
#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pincfg;
#endif
#if defined(CONFIG_ETH_MCUX_PHY_RESET)
	const struct gpio_dt_spec int_gpio;
	const struct gpio_dt_spec reset_gpio;
#endif
};

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

#ifdef CONFIG_SOC_FAMILY_KINETIS
#if defined(CONFIG_NET_POWER_MANAGEMENT)
static void eth_mcux_phy_enter_reset(struct eth_context *context);
void eth_mcux_phy_stop(struct eth_context *context);

static int eth_mcux_device_pm_action(const struct device *dev,
				     enum pm_device_action action)
{
	struct eth_context *eth_ctx = dev->data;
	int ret = 0;

	if (!device_is_ready(eth_ctx->clock_dev)) {
		LOG_ERR("No CLOCK dev");

		ret = -EIO;
		goto out;
	}

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
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
		break;
	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("Resuming");

		clock_control_on(eth_ctx->clock_dev,
			(clock_control_subsys_t)eth_ctx->clock);
		eth_mcux_init(dev);
		net_if_resume(eth_ctx->iface);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

out:

	return ret;
}
#endif /* CONFIG_NET_POWER_MANAGEMENT */
#endif /* CONFIG_SOC_FAMILY_KINETIS */

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

static inline struct net_if *get_iface(struct eth_context *ctx)
{
	return ctx->iface;
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
		context->phy_handle->phyAddr = context->phy_addr;
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
#if defined(CONFIG_SOC_SERIES_IMXRT10XX) || defined(CONFIG_SOC_SERIES_IMXRT11XX)
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
		k_work_cancel_delayable(&context->delayed_phy_work);
		/* @todo, actually power down the PHY ? */
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
#if defined(CONFIG_SOC_SERIES_IMXRT10XX) || defined(CONFIG_SOC_SERIES_IMXRT11XX)
	status_t res;
	uint16_t ctrl2;
#endif
	phy_duplex_t phy_duplex = kPHY_FullDuplex;
	phy_speed_t phy_speed = kPHY_Speed100M;

#if defined(CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG)
	LOG_DBG("%s phy_state=%s", eth_name(context->base),
		phy_state_name(context->phy_state));
#endif
	switch (context->phy_state) {
	case eth_mcux_phy_state_initial:
#if defined(CONFIG_SOC_SERIES_IMXRT10XX) || defined(CONFIG_SOC_SERIES_IMXRT11XX)
		ENET_DisableInterrupts(context->base, ENET_EIR_MII_MASK);
		res = PHY_Read(context->phy_handle, PHY_CONTROL2_REG, &ctrl2);
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

		k_work_reschedule(&context->delayed_phy_work, K_MSEC(1));
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
				    PHY_10BASETX_HALFDUPLEX_MASK |
					PHY_IEEE802_3_SELECTOR_MASK));
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
		if (link_up && !context->link_up && context->iface != NULL) {
			/* Start reading the PHY control register. */
#if !defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
			ENET_StartSMIRead(context->base, context->phy_addr,
					  PHY_CONTROL1_REG,
					  kENET_MiiReadValidFrame);
#endif
			context->link_up = link_up;
			context->phy_state = eth_mcux_phy_state_read_duplex;
			net_eth_carrier_on(context->iface);
			k_msleep(1);
#if defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
			k_work_submit(&context->phy_work);
#endif
		} else if (!link_up && context->link_up && context->iface != NULL) {
			LOG_INF("%s link down", eth_name(context->base));
			context->link_up = link_up;
			k_work_reschedule(&context->delayed_phy_work,
					  K_MSEC(CONFIG_ETH_MCUX_PHY_TICK_MS));
			context->phy_state = eth_mcux_phy_state_wait;
			net_eth_carrier_off(context->iface);
		} else {
			k_work_reschedule(&context->delayed_phy_work,
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
		k_work_reschedule(&context->delayed_phy_work,
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
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct eth_context *context =
		CONTAINER_OF(dwork, struct eth_context, delayed_phy_work);

	eth_mcux_phy_event(context);
}

static void eth_mcux_phy_setup(struct eth_context *context)
{
#if defined(CONFIG_SOC_SERIES_IMXRT10XX) || defined(CONFIG_SOC_SERIES_IMXRT11XX)
	status_t res;
	uint16_t oms_override;

	/* Disable MII interrupts to prevent triggering PHY events. */
	ENET_DisableInterrupts(context->base, ENET_EIR_MII_MASK);

	res = PHY_Read(context->phy_handle,
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

		res = PHY_Write(context->phy_handle,
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

	if (ntohs(NET_ETH_HDR(pkt)->type) != NET_ETH_PTYPE_PTP) {
		return false;
	}

	eth_hlen = sizeof(struct net_eth_hdr);

	net_pkt_set_priority(pkt, NET_PRIORITY_CA);

	return true;
}
#endif /* CONFIG_PTP_CLOCK_MCUX */

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_context *context = dev->data;
	uint16_t total_len = net_pkt_get_len(pkt);
	status_t status;

#if defined(CONFIG_PTP_CLOCK_MCUX)
	bool timestamped_frame;
#endif

	/* Wait for a TX buffer descriptor to be available */
	k_sem_take(&context->tx_buf_sem, K_FOREVER);

	k_mutex_lock(&context->tx_frame_buf_mutex, K_FOREVER);

	if (net_pkt_read(pkt, context->tx_frame_buf, total_len)) {
		k_mutex_unlock(&context->tx_frame_buf_mutex);
		return -EIO;
	}


#if defined(CONFIG_PTP_CLOCK_MCUX)
	timestamped_frame = eth_get_ptp_data(net_pkt_iface(pkt), pkt);
	if (timestamped_frame) {
		status = ENET_SendFrame(context->base, &context->enet_handle,
					  context->tx_frame_buf, total_len, RING_ID, true, pkt);
		if (!status) {
			net_pkt_ref(pkt);
			/*
			 * Network stack will modify the packet upon return,
			 * so wait for the packet to be timestamped,
			 * which will occur within the TX ISR, before
			 * returning
			 */
			k_sem_take(&context->ptp_ts_sem, K_FOREVER);
		}

	} else
#endif
	{
		status = ENET_SendFrame(context->base, &context->enet_handle,
					context->tx_frame_buf, total_len, RING_ID, false, NULL);
	}

	if (status) {
		LOG_ERR("ENET_SendFrame error: %d", (int)status);
		k_mutex_unlock(&context->tx_frame_buf_mutex);
		ENET_ReclaimTxDescriptor(context->base,
					&context->enet_handle, RING_ID);
		return -1;
	}

	k_mutex_unlock(&context->tx_frame_buf_mutex);

	return 0;
}

static int eth_rx(struct eth_context *context)
{
	uint32_t frame_length = 0U;
	struct net_if *iface;
	struct net_pkt *pkt;
	status_t status;
	uint32_t ts;

#if defined(CONFIG_PTP_CLOCK_MCUX)
	enet_ptp_time_t ptpTimeData;
#endif

	status = ENET_GetRxFrameSize(&context->enet_handle,
				     (uint32_t *)&frame_length, RING_ID);
	if (status == kStatus_ENET_RxFrameEmpty) {
		return 0;
	} else if (status == kStatus_ENET_RxFrameError) {
		enet_data_error_stats_t error_stats;

		LOG_ERR("ENET_GetRxFrameSize return: %d", (int)status);

		ENET_GetRxErrBeforeReadFrame(&context->enet_handle,
					     &error_stats, RING_ID);
		goto flush;
	}

	if (frame_length > NET_ETH_MAX_FRAME_SIZE) {
		LOG_ERR("frame too large (%d)", frame_length);
		goto flush;
	}

	/* Using root iface. It will be updated in net_recv_data() */
	pkt = net_pkt_rx_alloc_with_buffer(context->iface, frame_length,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		goto flush;
	}

	/* in case multiply thread access
	 * we need to protect it with mutex.
	 */
	k_mutex_lock(&context->rx_frame_buf_mutex, K_FOREVER);

	status = ENET_ReadFrame(context->base, &context->enet_handle,
				context->rx_frame_buf, frame_length, RING_ID, &ts);
	if (status) {
		LOG_ERR("ENET_ReadFrame failed: %d", (int)status);
		net_pkt_unref(pkt);

		k_mutex_unlock(&context->rx_frame_buf_mutex);
		goto error;
	}

	if (net_pkt_write(pkt, context->rx_frame_buf, frame_length)) {
		LOG_ERR("Unable to write frame into the pkt");
		net_pkt_unref(pkt);
		k_mutex_unlock(&context->rx_frame_buf_mutex);
		goto error;
	}

	k_mutex_unlock(&context->rx_frame_buf_mutex);

	/*
	 * Use MAC timestamp
	 */
#if defined(CONFIG_PTP_CLOCK_MCUX)
	k_mutex_lock(&context->ptp_mutex, K_FOREVER);
	if (eth_get_ptp_data(get_iface(context), pkt)) {
		ENET_Ptp1588GetTimer(context->base, &context->enet_handle,
					&ptpTimeData);
		/* If latest timestamp reloads after getting from Rx BD,
		 * then second - 1 to make sure the actual Rx timestamp is
		 * accurate
		 */
		if (ptpTimeData.nanosecond < ts) {
			ptpTimeData.second--;
		}

		pkt->timestamp.nanosecond = ts;
		pkt->timestamp.second = ptpTimeData.second;
	} else {
		/* Invalid value. */
		pkt->timestamp.nanosecond = UINT32_MAX;
		pkt->timestamp.second = UINT64_MAX;
	}
	k_mutex_unlock(&context->ptp_mutex);
#endif /* CONFIG_PTP_CLOCK_MCUX */

	iface = get_iface(context);
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
	status = ENET_ReadFrame(context->base, &context->enet_handle, NULL,
					0, RING_ID, NULL);
	__ASSERT_NO_MSG(status == kStatus_Success);
error:
	eth_stats_update_errors_rx(get_iface(context));
	return -EIO;
}

#if defined(CONFIG_PTP_CLOCK_MCUX) && defined(CONFIG_NET_L2_PTP)
static inline void ts_register_tx_event(struct eth_context *context,
					 enet_frame_info_t *frameinfo)
{
	struct net_pkt *pkt;

	pkt = frameinfo->context;
	if (pkt && atomic_get(&pkt->atomic_ref) > 0) {
		if (eth_get_ptp_data(net_pkt_iface(pkt), pkt)) {
			if (frameinfo->isTsAvail) {
				k_mutex_lock(&context->ptp_mutex, K_FOREVER);

				pkt->timestamp.nanosecond =
					frameinfo->timeStamp.nanosecond;
				pkt->timestamp.second =
					frameinfo->timeStamp.second;

				net_if_add_tx_timestamp(pkt);
				k_sem_give(&context->ptp_ts_sem);
				k_mutex_unlock(&context->ptp_mutex);
			}
		}

		net_pkt_unref(pkt);
	} else {
		if (IS_ENABLED(CONFIG_ETH_MCUX_PHY_EXTRA_DEBUG) && pkt) {
			LOG_ERR("pkt %p already freed", pkt);
		}
	}

}
#endif /* CONFIG_PTP_CLOCK_MCUX && CONFIG_NET_L2_PTP */

static void eth_callback(ENET_Type *base, enet_handle_t *handle,
#if FSL_FEATURE_ENET_QUEUE > 1
			 uint32_t ringId,
#endif /* FSL_FEATURE_ENET_QUEUE > 1 */
			 enet_event_t event, enet_frame_info_t *frameinfo, void *param)
{
	struct eth_context *context = param;

	switch (event) {
	case kENET_RxEvent:
		k_sem_give(&context->rx_thread_sem);
		break;
	case kENET_TxEvent:
#if defined(CONFIG_PTP_CLOCK_MCUX) && defined(CONFIG_NET_L2_PTP)
		/* Register event */
		ts_register_tx_event(context, frameinfo);
#endif /* CONFIG_PTP_CLOCK_MCUX && CONFIG_NET_L2_PTP */
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

static void eth_rx_thread(void *arg1, void *unused1, void *unused2)
{
	struct eth_context *context = (struct eth_context *)arg1;

	while (1) {
		if (k_sem_take(&context->rx_thread_sem, K_FOREVER) == 0) {
			while (eth_rx(context) == 1) {
				;
			}
			/* enable the IRQ for RX */
			ENET_EnableInterrupts(context->base,
			  kENET_RxFrameInterrupt | kENET_RxBufferInterrupt);
		}
	}
}

#if defined(CONFIG_ETH_MCUX_PHY_RESET)
static int eth_phy_reset(const struct device *dev)
{
	int err;
	struct eth_context *context = dev->data;

	/* pull up the ENET_INT before RESET. */
	err =  gpio_pin_configure_dt(&context->int_gpio, GPIO_OUTPUT_ACTIVE);
	if (err) {
		return err;
	}
	return gpio_pin_configure_dt(&context->reset_gpio, GPIO_OUTPUT_INACTIVE);
}

static int eth_phy_init(const struct device *dev)
{
	struct eth_context *context = dev->data;

	/* RESET PHY chip. */
	k_busy_wait(USEC_PER_MSEC * 500);
	return gpio_pin_set_dt(&context->reset_gpio, 1);
}
#endif

static void eth_mcux_init(const struct device *dev)
{
	struct eth_context *context = dev->data;
	const enet_buffer_config_t *buffer_config = dev->config;
	enet_config_t enet_config;
	uint32_t sys_clock;
#if defined(CONFIG_PTP_CLOCK_MCUX)
	uint8_t ptp_multicast[6] = { 0x01, 0x1B, 0x19, 0x00, 0x00, 0x00 };
	uint8_t ptp_peer_multicast[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E };
#endif
#if defined(CONFIG_MDNS_RESPONDER) || defined(CONFIG_MDNS_RESOLVER)
	/* standard multicast MAC address */
	uint8_t mdns_multicast[6] = { 0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB };
#endif

	context->phy_state = eth_mcux_phy_state_initial;
	context->phy_handle->ops = &phyksz8081_ops;

#if defined(CONFIG_SOC_SERIES_IMXRT10XX)
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(enet))
	sys_clock = CLOCK_GetFreq(kCLOCK_IpgClk);
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(enet2))
	sys_clock = CLOCK_GetFreq(kCLOCK_EnetPll1Clk);
#endif
#elif defined(CONFIG_SOC_SERIES_IMXRT11XX)
	sys_clock = CLOCK_GetRootClockFreq(kCLOCK_Root_Bus);
#else
	sys_clock = CLOCK_GetFreq(kCLOCK_CoreSysClk);
#endif

	ENET_GetDefaultConfig(&enet_config);
	enet_config.interrupt |= kENET_RxFrameInterrupt;
	enet_config.interrupt |= kENET_TxFrameInterrupt;
#if !defined(CONFIG_ETH_MCUX_NO_PHY_SMI)
	enet_config.interrupt |= kENET_MiiInterrupt;
#endif
	enet_config.miiMode = kENET_RmiiMode;
	enet_config.callback = eth_callback;
	enet_config.userData = context;

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
	ENET_AddMulticastGroup(context->base, ptp_peer_multicast);

	/* only for ERRATA_2579 */
	context->ptp_config.channel = kENET_PtpTimerChannel3;
	context->ptp_config.ptp1588ClockSrc_Hz =
					CONFIG_ETH_MCUX_PTP_CLOCK_SRC_HZ;
	context->clk_ratio = 1.0;

	ENET_Ptp1588SetChannelMode(context->base, kENET_PtpTimerChannel3,
			kENET_PtpChannelPulseHighonCompare, true);
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

	eth_mcux_phy_start(context);
}

static int eth_init(const struct device *dev)
{
	struct eth_context *context = dev->data;
#if defined(CONFIG_PINCTRL)
	int err;

	err = pinctrl_apply_state(context->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}
#endif /* CONFIG_PINCTRL */

#if defined(CONFIG_NET_POWER_MANAGEMENT)
	const uint32_t inst = ENET_GetInstance(context->base);

	context->clock = enet_clocks[inst];
#endif

#if defined(CONFIG_ETH_MCUX_PHY_RESET)
	eth_phy_reset(dev);
	eth_phy_init(dev);
#endif

#if defined(CONFIG_PTP_CLOCK_MCUX)
	k_mutex_init(&context->ptp_mutex);
	k_sem_init(&context->ptp_ts_sem, 0, 1);
#endif
	k_mutex_init(&context->rx_frame_buf_mutex);
	k_mutex_init(&context->tx_frame_buf_mutex);

	k_sem_init(&context->rx_thread_sem, 0, CONFIG_ETH_MCUX_RX_BUFFERS);
	k_sem_init(&context->tx_buf_sem,
		   CONFIG_ETH_MCUX_TX_BUFFERS, CONFIG_ETH_MCUX_TX_BUFFERS);
	k_work_init(&context->phy_work, eth_mcux_phy_work);
	k_work_init_delayable(&context->delayed_phy_work,
			      eth_mcux_delayed_phy_work);

	/* Start interruption-poll thread */
	k_thread_create(&context->rx_thread, context->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(context->rx_thread_stack),
			eth_rx_thread, (void *) context, NULL, NULL,
			K_PRIO_COOP(2),
			0, K_NO_WAIT);
	k_thread_name_set(&context->rx_thread, "mcux_eth_rx");
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

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->data;

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	if (context->iface == NULL) {
		context->iface = iface;
	}

#if defined(CONFIG_NET_DSA)
	dsa_register_master_tx(iface, &eth_tx);
#endif
	ethernet_init(iface);
	net_if_carrier_off(iface);

	context->config_func();
}

static enum ethernet_hw_caps eth_mcux_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T |
		ETHERNET_HW_FILTERING |
#if defined(CONFIG_NET_VLAN)
		ETHERNET_HW_VLAN |
#endif
#if defined(CONFIG_PTP_CLOCK_MCUX)
		ETHERNET_PTP |
#endif
#if defined(CONFIG_NET_DSA)
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
	case ETHERNET_CONFIG_TYPE_FILTER:
		/* The ENET driver does not modify the address buffer but the API is not const */
		if (config->filter.set) {
			ENET_AddMulticastGroup(context->base,
					       (uint8_t *)config->filter.mac_address.addr);
		} else {
			ENET_LeaveMulticastGroup(context->base,
						 (uint8_t *)config->filter.mac_address.addr);
		}
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
#if defined(CONFIG_NET_DSA)
	.send                   = dsa_tx,
#else
	.send			= eth_tx,
#endif
};

#if defined(CONFIG_PTP_CLOCK_MCUX)
static void eth_mcux_ptp_isr(const struct device *dev)
{
	struct eth_context *context = dev->data;
	unsigned int irq_lock_key = irq_lock();
	enet_ptp_timer_channel_t channel;

	/* clear channel */
	for (channel = kENET_PtpTimerChannel1; channel <= kENET_PtpTimerChannel4; channel++) {
		if (ENET_Ptp1588GetChannelStatus(context->base, channel)) {
			ENET_Ptp1588ClearChannelStatus(context->base, channel);
		}
	}
	ENET_TimeStampIRQHandler(context->base, &context->enet_handle);
	irq_unlock(irq_lock_key);
}
#endif

#if DT_INST_IRQ_HAS_NAME(0, common) || DT_INST_IRQ_HAS_NAME(1, common)
static void eth_mcux_common_isr(const struct device *dev)
{
	struct eth_context *context = dev->data;
	uint32_t EIR = ENET_GetInterruptStatus(context->base);
	unsigned int irq_lock_key = irq_lock();

	if (EIR & (kENET_RxBufferInterrupt | kENET_RxFrameInterrupt)) {
		/* disable the IRQ for RX */
		context->rx_irq_num++;
#if FSL_FEATURE_ENET_QUEUE > 1
		/* Only use ring 0 in this driver */
		ENET_ReceiveIRQHandler(context->base, &context->enet_handle, 0);
#else
		ENET_ReceiveIRQHandler(context->base, &context->enet_handle);
#endif
		ENET_DisableInterrupts(context->base, kENET_RxFrameInterrupt |
			kENET_RxBufferInterrupt);
	}

	if (EIR & kENET_TxFrameInterrupt) {
#if FSL_FEATURE_ENET_QUEUE > 1
		ENET_TransmitIRQHandler(context->base, &context->enet_handle, 0);
#else
		ENET_TransmitIRQHandler(context->base, &context->enet_handle);
#endif
	}

	if (EIR | kENET_TxBufferInterrupt) {
		ENET_ClearInterruptStatus(context->base, kENET_TxBufferInterrupt);
		ENET_DisableInterrupts(context->base, kENET_TxBufferInterrupt);
	}

	if (EIR & ENET_EIR_MII_MASK) {
		k_work_submit(&context->phy_work);
		ENET_ClearInterruptStatus(context->base, kENET_MiiInterrupt);
	}
#if defined(CONFIG_PTP_CLOCK_MCUX)
	if (EIR & ENET_TS_INTERRUPT) {
		ENET_TimeStampIRQHandler(context->base, &context->enet_handle);
	}
#endif
	irq_unlock(irq_lock_key);
}
#endif

#if DT_INST_IRQ_HAS_NAME(0, rx) || DT_INST_IRQ_HAS_NAME(1, rx)
static void eth_mcux_rx_isr(const struct device *dev)
{
	struct eth_context *context = dev->data;

	ENET_DisableInterrupts(context->base, kENET_RxFrameInterrupt | kENET_RxBufferInterrupt);
	ENET_ReceiveIRQHandler(context->base, &context->enet_handle);
}
#endif

#if DT_INST_IRQ_HAS_NAME(0, tx) || DT_INST_IRQ_HAS_NAME(1, tx)
static void eth_mcux_tx_isr(const struct device *dev)
{
	struct eth_context *context = dev->data;
#if FSL_FEATURE_ENET_QUEUE > 1
	ENET_TransmitIRQHandler(context->base, &context->enet_handle, 0);
#else
	ENET_TransmitIRQHandler(context->base, &context->enet_handle);
#endif
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

#if defined(CONFIG_SOC_SERIES_IMXRT10XX)
#define ETH_MCUX_UNIQUE_ID	(OCOTP->CFG1 ^ OCOTP->CFG2)
#elif defined(CONFIG_SOC_SERIES_IMXRT11XX)
#define ETH_MCUX_UNIQUE_ID	(OCOTP->FUSEN[40].FUSE)
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
	} while (false)

#define ETH_MCUX_IRQ(n, name)						\
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(n, name),			\
		    (ETH_MCUX_IRQ_INIT(n, name)),			\
		    (ETH_MCUX_NONE))

#if defined(CONFIG_PTP_CLOCK_MCUX)
#define PTP_INST_NODEID(n) DT_INST_CHILD(n, ptp)

#define ETH_MCUX_IRQ_PTP_INIT(n)							\
	do {										\
		IRQ_CONNECT(DT_IRQ_BY_NAME(PTP_INST_NODEID(n), ieee1588_tmr, irq),	\
			    DT_IRQ_BY_NAME(PTP_INST_NODEID(n), ieee1588_tmr, priority),	\
			    eth_mcux_ptp_isr,						\
			    DEVICE_DT_INST_GET(n),					\
			    0);								\
		irq_enable(DT_IRQ_BY_NAME(PTP_INST_NODEID(n), ieee1588_tmr, irq));	\
	} while (false)

#define ETH_MCUX_IRQ_PTP(n)						\
	COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(PTP_INST_NODEID(n)),	\
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

#ifdef CONFIG_SOC_FAMILY_KINETIS
#define ETH_MCUX_POWER_INIT(n)						\
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\

#define ETH_MCUX_POWER(n)						\
	COND_CODE_1(CONFIG_NET_POWER_MANAGEMENT,			\
		    (ETH_MCUX_POWER_INIT(n)),				\
		    (ETH_MCUX_NONE))
#define ETH_MCUX_PM_DEVICE_INIT(n)					\
	PM_DEVICE_DT_INST_DEFINE(n, eth_mcux_device_pm_action);
#define ETH_MCUX_PM_DEVICE_GET(n) PM_DEVICE_DT_INST_GET(n)
#else
#define ETH_MCUX_POWER(n)
#define ETH_MCUX_PM_DEVICE_INIT(n)
#define ETH_MCUX_PM_DEVICE_GET(n) NULL
#endif /* CONFIG_SOC_FAMILY_KINETIS */

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

#if defined(CONFIG_PINCTRL)
#define ETH_MCUX_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define ETH_MCUX_PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define ETH_MCUX_PINCTRL_DEFINE(n)
#define ETH_MCUX_PINCTRL_INIT(n)
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_CHOSEN(zephyr_dtcm)) && \
	CONFIG_ETH_MCUX_USE_DTCM_FOR_DMA_BUFFER
/* Use DTCM for hardware DMA buffers */
#define _mcux_dma_desc __dtcm_bss_section
#define _mcux_dma_buffer __dtcm_noinit_section
#define _mcux_driver_buffer __dtcm_noinit_section
#elif defined(CONFIG_NOCACHE_MEMORY)
#define _mcux_dma_desc __nocache
#define _mcux_dma_buffer __nocache
#define _mcux_driver_buffer
#else
#define _mcux_dma_desc
#define _mcux_dma_buffer
#define _mcux_driver_buffer
#endif

#if defined(CONFIG_ETH_MCUX_PHY_RESET)
#define ETH_MCUX_PHY_GPIOS(n)						\
	.int_gpio = GPIO_DT_SPEC_INST_GET(n, int_gpios),		\
	.reset_gpio = GPIO_DT_SPEC_INST_GET(n, reset_gpios),
#else
#define ETH_MCUX_PHY_GPIOS(n)
#endif

#define ETH_MCUX_INIT(n)						\
	ETH_MCUX_GEN_MAC(n)                                             \
									\
	ETH_MCUX_PINCTRL_DEFINE(n)					\
									\
	static void eth##n##_config_func(void);				\
	static _mcux_driver_buffer uint8_t				\
		tx_enet_frame_##n##_buf[NET_ETH_MAX_FRAME_SIZE];	\
	static _mcux_driver_buffer uint8_t				\
		rx_enet_frame_##n##_buf[NET_ETH_MAX_FRAME_SIZE];	\
	static status_t _MDIO_Write(uint8_t phyAddr, uint8_t regAddr, uint16_t data) \
	{								\
		return ENET_MDIOWrite((ENET_Type *)DT_INST_REG_ADDR(n), phyAddr, regAddr, data);\
	};								\
									\
	static status_t _MDIO_Read(uint8_t phyAddr, uint8_t regAddr, uint16_t *pData) \
	{  \
		return ENET_MDIORead((ENET_Type *)DT_INST_REG_ADDR(n), phyAddr, regAddr, pData); \
	}; \
									\
	static struct _phy_resource eth##n##_phy_resource = {		\
		.read = _MDIO_Read,					\
		.write = _MDIO_Write					\
	};								\
	static phy_handle_t eth##n##_phy_handle = {			\
		.resource = (void *)&eth##n##_phy_resource		\
	};								\
	static struct _phy_resource eth##n##_phy_config;		\
									\
	static struct eth_context eth##n##_context = {			\
		.base = (ENET_Type *)DT_INST_REG_ADDR(n),		\
		.config_func = eth##n##_config_func,			\
		.phy_config = &eth##n##_phy_config,			\
		.phy_addr = DT_INST_PROP(n, phy_addr),		        \
		.phy_duplex = kPHY_FullDuplex,				\
		.phy_speed = kPHY_Speed100M,				\
		.phy_handle = &eth##n##_phy_handle,			\
		.tx_frame_buf = tx_enet_frame_##n##_buf,		\
		.rx_frame_buf = rx_enet_frame_##n##_buf,		\
		ETH_MCUX_PINCTRL_INIT(n)				\
		ETH_MCUX_PHY_GPIOS(n)					\
		ETH_MCUX_MAC_ADDR(n)					\
		ETH_MCUX_POWER(n)					\
	};								\
									\
	static __aligned(ENET_BUFF_ALIGNMENT)				\
		_mcux_dma_desc						\
		enet_rx_bd_struct_t					\
		eth##n##_rx_buffer_desc[CONFIG_ETH_MCUX_RX_BUFFERS];	\
									\
	static __aligned(ENET_BUFF_ALIGNMENT)				\
		_mcux_dma_desc						\
		enet_tx_bd_struct_t					\
		eth##n##_tx_buffer_desc[CONFIG_ETH_MCUX_TX_BUFFERS];	\
									\
	static uint8_t __aligned(ENET_BUFF_ALIGNMENT)			\
		_mcux_dma_buffer					\
		eth##n##_rx_buffer[CONFIG_ETH_MCUX_RX_BUFFERS]		\
				  [ETH_MCUX_BUFFER_SIZE];		\
									\
	static uint8_t __aligned(ENET_BUFF_ALIGNMENT)			\
		_mcux_dma_buffer					\
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
	ETH_MCUX_PM_DEVICE_INIT(n)					\
									\
	ETH_NET_DEVICE_DT_INST_DEFINE(n,				\
			    eth_init,					\
			    ETH_MCUX_PM_DEVICE_GET(n),			\
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
#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pincfg;
#endif /* CONFIG_PINCTRL */
};

#if defined(CONFIG_PINCTRL)
#define ETH_MCUX_PTP_PINCTRL_DEFINE(n) PINCTRL_DT_DEFINE(n);
#define ETH_MCUX_PTP_PINCTRL_INIT(n) .pincfg = PINCTRL_DT_DEV_CONFIG_GET(n),
#else
#define ETH_MCUX_PTP_PINCTRL_DEFINE(n)
#define ETH_MCUX_PTP_PINCTRL_INIT(n)
#endif /* CONFIG_PINCTRL */

ETH_MCUX_PTP_PINCTRL_DEFINE(DT_NODELABEL(ptp))

static struct ptp_context ptp_mcux_0_context = {
	ETH_MCUX_PTP_PINCTRL_INIT(DT_NODELABEL(ptp))
};

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

	if ((increment <= (int32_t)(-NSEC_PER_SEC)) ||
			(increment >= (int32_t)NSEC_PER_SEC)) {
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

static int ptp_clock_mcux_rate_adjust(const struct device *dev, double ratio)
{
	const int hw_inc = NSEC_PER_SEC / CONFIG_ETH_MCUX_PTP_CLOCK_SRC_HZ;
	struct ptp_context *ptp_context = dev->data;
	struct eth_context *context = ptp_context->eth_context;
	int corr;
	int32_t mul;
	double val;

	/* No change needed. */
	if ((ratio > 1.0 && ratio - 1.0 < 0.00000001) ||
	   (ratio < 1.0 && 1.0 - ratio < 0.00000001)) {
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
		val = 1.0 / (hw_inc * (ratio - 1.0));
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
	k_mutex_lock(&context->ptp_mutex, K_FOREVER);
	ENET_Ptp1588AdjustTimer(context->base, corr, mul);
	k_mutex_unlock(&context->ptp_mutex);

	return 0;
}

static DEVICE_API(ptp_clock, api) = {
	.set = ptp_clock_mcux_set,
	.get = ptp_clock_mcux_get,
	.adjust = ptp_clock_mcux_adjust,
	.rate_adjust = ptp_clock_mcux_rate_adjust,
};

static int ptp_mcux_init(const struct device *port)
{
	const struct device *const eth_dev = DEVICE_DT_GET(DT_NODELABEL(enet));
	struct eth_context *context = eth_dev->data;
	struct ptp_context *ptp_context = port->data;
#if defined(CONFIG_PINCTRL)
	int err;

	err = pinctrl_apply_state(ptp_context->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}
#endif /* CONFIG_PINCTRL */

	context->ptp_clock = port;
	ptp_context->eth_context = context;

	return 0;
}

DEVICE_DEFINE(mcux_ptp_clock_0, PTP_CLOCK_NAME, ptp_mcux_init,
		NULL, &ptp_mcux_0_context, NULL, POST_KERNEL,
		CONFIG_ETH_MCUX_PTP_CLOCK_INIT_PRIO, &api);

#endif /* CONFIG_PTP_CLOCK_MCUX */
