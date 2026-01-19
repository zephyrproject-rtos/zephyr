/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_numaker, CONFIG_UDC_DRIVER_LOG_LEVEL);

#include "udc_common.h"
#include "udc_numaker.h"

/* USBD notes
 *
 * 1. Require 48MHz clock source
 *    (1) Not support HIRC48 as clock source. It involves trim with USB SOF packets
 *        and isn't suitable in HAL.
 *    (2) Instead of HICR48, core clock is required to be multiple of 48MHz e.g. 192MHz,
 *        to generate necessary 48MHz.
 */

/* For bus reset, keep 'SE0' (USB spec: SE0 >= 2.5 ms) */
#define NUMAKER_USBD_BUS_RESET_DRV_SE0_US 3000

/* For bus resume, generate 'K' (USB spec: 'K' >= 1 ms) */
#define NUMAKER_USBD_BUS_RESUME_DRV_K_US 1500

/* Reserve DMA buffer for Setup/CTRL OUT/CTRL IN, required to be 8-byte aligned */
#define NUMAKER_USBD_DMABUF_SIZE_SETUP   8
#define NUMAKER_USBD_DMABUF_SIZE_CTRLOUT 64
#define NUMAKER_USBD_DMABUF_SIZE_CTRLIN  64

/* USBD controller does not support DMA, and PHY does not require a delay after reset. */

#if defined(CONFIG_SOC_SERIES_M46X)
#if !defined(USBD_ATTR_PWRDN_Msk)
#define USBD_ATTR_PWRDN_Msk BIT(9)
#endif
#elif defined(CONFIG_SOC_SERIES_M55M1X)
#if !defined(SYS_USBPHY_USBROLE_STD_USBD)
#define SYS_USBPHY_USBROLE_STD_USBD (0x0 << SYS_USBPHY_USBROLE_Pos)
#endif
#endif

/* Per HSUSBD H/W spec, after setting HSUSBEN to enable HSUSB/PHY, user
 * should keep HSUSB/PHY at reset mode at lease 10us before changing to
 * active mode.
 */
#define NUMAKER_HSUSBD_PHY_RESET_US 10

/* Wait for USB/PHY stable timeout 100 ms */
#define NUMAKER_HSUSBD_PHY_STABLE_TIMEOUT_US 100000

#if defined(CONFIG_SOC_SERIES_M46X)
#define CEPBUFSTART CEPBUFST
#define EPBUFSTART  EPBUFST
#elif defined(CONFIG_SOC_SERIES_M55M1X)
#if !defined(SYS_USBPHY_HSUSBROLE_STD_USBD)
#define SYS_USBPHY_HSUSBROLE_STD_USBD (0x0 << SYS_USBPHY_HSUSBROLE_Pos)
#endif
#elif defined(CONFIG_SOC_SERIES_M333X)
#define CEPBUFSTART CEPBUFST
#define EPBUFSTART  EPBUFST
#endif

enum numaker_usbd_msg_type {
	/* Device plug-in */
	NUMAKER_USBD_MSG_TYPE_ATTACH,
	/* Bus reset */
	NUMAKER_USBD_MSG_TYPE_RESET,
	/* Bus resume */
	NUMAKER_USBD_MSG_TYPE_RESUME,
	/* Setup packet received */
	NUMAKER_USBD_MSG_TYPE_SETUP,
	/* OUT transaction for specific EP completed */
	NUMAKER_USBD_MSG_TYPE_OUT,
	/* IN transaction for specific EP completed */
	NUMAKER_USBD_MSG_TYPE_IN,
	/* Re-activate queued transfer for specific EP */
	NUMAKER_USBD_MSG_TYPE_XFER,
	/* S/W reconnect */
	NUMAKER_USBD_MSG_TYPE_SW_RECONN,
};

struct numaker_usbd_msg {
	enum numaker_usbd_msg_type type;
	union {
		struct {
			enum udc_event_type type;
		} udc_bus_event;
		struct {
			uint8_t packet[8];
		} setup;
		struct {
			uint8_t ep;
		} out;
		struct {
			uint8_t ep;
		} in;
		struct {
			uint8_t ep;
		} xfer;
	};
};

/* EP H/W context */
struct numaker_usbd_ep {
	bool valid;

	const struct device *dev; /* Pointer to the containing device */

	int32_t ep_hw_idx;     /* BSP USBD/HSUSBD driver EP index, e.g. EP0/EPA, EP1/EPB, etc. */
	uint32_t ep_hw_cfg;    /* BSP USBD/HSUSBD driver EP configuration */
	uint32_t ep_hw_rspctl; /* BSP HSUSBD driver RSPCTL */

	/* EP DMA buffer */
	bool dmabuf_valid;
	uint32_t dmabuf_base;
	uint32_t dmabuf_size;

	/* NOTE: On USBD, Setup and CTRL OUT are not completely separated. CTRL OUT MXPLD
	 * can be overridden to 8 by next Setup. To overcome it, we make one copy of CTRL
	 * OUT MXPLD immediately on its interrupt.
	 */
	uint32_t mxpld_ctrlout;

	/* EP address */
	bool addr_valid;
	uint8_t addr;

	/* EP MPS */
	bool mps_valid;
	uint16_t mps;
};

/* Immutable device context */
struct udc_numaker_config {
	struct udc_ep_config *ep_cfg_out;
	struct udc_ep_config *ep_cfg_in;
	uint32_t ep_cfg_out_size;
	uint32_t ep_cfg_in_size;
	void *base;
	const struct reset_dt_spec reset;
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clkctrl_dev;
	void (*irq_config_func)(const struct device *dev);
	void (*irq_unconfig_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
	uint32_t dmabuf_size;
	bool disallow_iso_inout_same;
	bool allow_disable_usb_on_unplug;
	int speed_idx;
	void (*make_thread)(const struct device *dev);
	bool is_hsusbd;
};

/* EP H/W context manager */
struct numaker_usbd_ep_mgmt {
	/* EP H/W context management
	 *
	 * Allocate-only, and de-allocate all on re-initialize in udc_numaker_init().
	 */
	uint8_t ep_idx;

	/* DMA buffer management
	 *
	 * Allocate-only, and de-allocate all on re-initialize in udc_numaker_init().
	 */
	uint32_t dmabuf_pos;
};

/* Mutable device context */
struct udc_numaker_data {
	uint8_t addr; /* Host assigned USB device address */

	struct k_msgq *msgq;

	struct numaker_usbd_ep_mgmt ep_mgmt; /* EP management */

	struct numaker_usbd_ep *ep_pool;
	uint32_t ep_pool_size;

	struct k_thread thread_data;

	/* Track end of CTRL DATA OUT/STATUS OUT stage
	 *
	 * net_buf can over-allocate for UDC_BUF_GRANULARITY requirement
	 * and net_buf_tailroom() cannot reflect free buffer room exactly
	 * as allocate request. Manually track it instead.
	 */
	uint32_t ctrlout_tailroom;

#if defined(CONFIG_UDC_NUMAKER_DMA)
	struct k_sem sem_dma_done;
#endif
};

static inline void numaker_usbd_sw_connect(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		/* Clear all interrupts first for clean */
		base->BUSINTSTS = base->BUSINTSTS;
		base->CEPINTSTS = base->CEPINTSTS;

		/* Enable relevant interrupts */
		base->GINTEN = HSUSBD_GINTEN_CEPIEN_Msk | HSUSBD_GINTEN_USBIEN_Msk;
		base->BUSINTEN = HSUSBD_BUSINTEN_VBUSDETIEN_Msk |
				 IF_ENABLED(CONFIG_UDC_NUMAKER_DMA,
					    (HSUSBD_BUSINTEN_DMADONEIEN_Msk |)) /* DMA */
				 HSUSBD_BUSINTEN_SUSPENDIEN_Msk |
				 HSUSBD_BUSINTEN_RESUMEIEN_Msk | HSUSBD_BUSINTEN_RSTIEN_Msk |
				 COND_CODE_1(CONFIG_UDC_ENABLE_SOF, (HSUSBD_BUSINTEN_SOFIEN_Msk),
					     (0)); /* CPU load concern */
		base->CEPINTEN = HSUSBD_CEPINTEN_STSDONEIEN_Msk | HSUSBD_CEPINTEN_ERRIEN_Msk |
				 HSUSBD_CEPINTEN_STALLIEN_Msk | HSUSBD_CEPINTEN_SETUPPKIEN_Msk |
				 HSUSBD_CEPINTEN_SETUPTKIEN_Msk;

		/* Enable USB handshake
		 *
		 * Being unset, USB handshake won't start, including bus events
		 * reset/suspend/resume. Per test, this bit also takes effect
		 * for full-speed;
		 */
#if defined(HSUSBD_OPER_HISHSEN_Msk)
		base->OPER |= HSUSBD_OPER_HISHSEN_Msk;
#endif

		/* Clear SE0 for connect */
		base->PHYCTL |= HSUSBD_PHYCTL_DPPUEN_Msk;
	} else {
		USBD_T *base = config->base;

		/* Clear all interrupts first for clean */
		base->INTSTS = base->INTSTS;

		/* Enable relevant interrupts */
		base->INTEN = USBD_INT_BUS | USBD_INT_USB | USBD_INT_FLDET |
			      IF_ENABLED(CONFIG_UDC_ENABLE_SOF,
					 (USBD_INT_SOF |)) /* CPU load concern */
			      USBD_INT_WAKEUP;

		/* Clear SE0 for connect */
		base->ATTR |= USBD_ATTR_DPPUEN_Msk;
		base->SE0 &= ~USBD_DRVSE0;
	}
}

static inline void numaker_usbd_sw_disconnect(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;

	/* Set SE0 for disconnect */
	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		base->PHYCTL &= ~HSUSBD_PHYCTL_DPPUEN_Msk;
	} else {
		USBD_T *base = config->base;

		base->SE0 |= USBD_DRVSE0;
	}
}

static inline void numaker_usbd_sw_reconnect(const struct device *dev)
{
	/* Keep SE0 to trigger bus reset */
	numaker_usbd_sw_disconnect(dev);
	k_sleep(K_USEC(NUMAKER_USBD_BUS_RESET_DRV_SE0_US));
	numaker_usbd_sw_connect(dev);
}

static inline void numaker_usbd_reset_addr(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	struct udc_numaker_data *priv = udc_get_private(dev);

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		base->FADDR = 0;
	} else {
		USBD_T *base = config->base;

		base->FADDR = 0;
	}

	priv->addr = 0;
}

static inline void numaker_usbd_set_addr(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	struct udc_numaker_data *priv = udc_get_private(dev);

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		if (base->FADDR != priv->addr) {
			base->FADDR = priv->addr;
		}
	} else {
		USBD_T *base = config->base;

		if (base->FADDR != priv->addr) {
			base->FADDR = priv->addr;
		}
	}
}

/* USBD/HSUSBD EP base by EP index e.g. EP0/EPA, EP1/EPB, etc. */
static inline void *numaker_usbd_ep_base(const struct device *dev, uint32_t ep_hw_idx)
{
	const struct udc_numaker_config *config = dev->config;
	void *ep_base;

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		ep_base = (ep_hw_idx == CEP) ? NULL : base->EP + (ep_hw_idx - EPA);
	} else {
		USBD_T *base = config->base;

		ep_base = base->EP + (ep_hw_idx - EP0);
	}

	return ep_base;
}

static inline void numaker_usbd_ep_sync_udc_halt(struct numaker_usbd_ep *ep_cur, bool stalled)
{
	const struct device *dev = ep_cur->dev;
	struct udc_ep_config *ep_cfg;

	__ASSERT_NO_MSG(ep_cur->addr_valid);
	ep_cfg = udc_get_ep_cfg(dev, ep_cur->addr);
	ep_cfg->stat.halted = stalled;
}

static inline void numaker_usbd_ep_set_stall(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;

	/* Set EP to stalled */
	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;
		HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		if (ep_cur->ep_hw_idx == CEP) {
			base->CEPCTL = HSUSBD_CEPCTL_STALL;
		} else {
			uint32_t eprspctl = ep_base->EPRSPCTL;

			eprspctl &= ~(HSUSBD_EPRSPCTL_HALT_Msk | HSUSBD_EPRSPCTL_TOGGLE_Msk);
			eprspctl |= HSUSBD_EP_RSPCTL_HALT;
			ep_base->EPRSPCTL = eprspctl;
		}
	} else {
		USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		ep_base->CFGP |= USBD_CFGP_SSTALL_Msk;
		numaker_usbd_ep_sync_udc_halt(ep_cur, true);
	}
}

/* Reset EP to unstalled and data toggle bit to 0 */
static inline void numaker_usbd_ep_clear_stall_n_data_toggle(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;
		HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		if (ep_cur->ep_hw_idx == CEP) {
			/* Reset EP to unstalled and H/W will care toggle bit reset */
			base->CEPCTL = 0;
		} else {
			/* Reset EP to unstalled and its data toggle bit to 0 */
			uint32_t eprspctl = ep_base->EPRSPCTL;

			eprspctl &= ~(HSUSBD_EPRSPCTL_HALT_Msk | HSUSBD_EPRSPCTL_TOGGLE_Msk);
			eprspctl |= HSUSBD_EP_RSPCTL_TOGGLE;
			ep_base->EPRSPCTL = eprspctl;
		}
	} else {
		USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		/* Reset EP to unstalled */
		ep_base->CFGP &= ~USBD_CFGP_SSTALL_Msk;
		numaker_usbd_ep_sync_udc_halt(ep_cur, false);

		/* Reset EP data toggle bit to 0 */
		ep_base->CFG &= ~USBD_CFG_DSQSYNC_Msk;
	}
}

static int numaker_usbd_send_msg(const struct device *dev, const struct numaker_usbd_msg *msg)
{
	struct udc_numaker_data *priv = udc_get_private(dev);
	int err;

	err = k_msgq_put(priv->msgq, msg, K_NO_WAIT);
	if (err < 0) {
		/* Try to recover by S/W reconnect */
		struct numaker_usbd_msg msg_reconn = {
			.type = NUMAKER_USBD_MSG_TYPE_SW_RECONN,
		};

		LOG_ERR("Message queue overflow");

		/* Discard all not yet received messages for error recovery below */
		k_msgq_purge(priv->msgq);

		err = k_msgq_put(priv->msgq, &msg_reconn, K_NO_WAIT);
		if (err < 0) {
			LOG_ERR("Message queue overflow again");
		}
	}

	return err;
}

static int numaker_usbd_enable_usb_phy(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		base->PHYCTL |= HSUSBD_PHYCTL_PHYEN_Msk;
		WAIT_FOR(base->PHYCTL & HSUSBD_PHYCTL_PHYCLKSTB_Msk,
			 NUMAKER_HSUSBD_PHY_STABLE_TIMEOUT_US,
			 ;);
		if (!(base->PHYCTL & HSUSBD_PHYCTL_PHYCLKSTB_Msk)) {
			return -EIO;
		}
	} else {
		USBD_T *base = config->base;

		base->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;
	}

	return 0;
}

static int numaker_usbd_hw_setup(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	int err;
	struct numaker_scc_subsys scc_subsys;

	/* Reset controller ready? */
	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("Reset controller not ready");
		return -ENODEV;
	}

	SYS_UnlockReg();

	/* Configure USB role as USB Device and enable USB/PHY */
	if (config->is_hsusbd) {
#if defined(CONFIG_SOC_SERIES_M46X)
		/* Configure HSUSB role as USB Device and enable HSUSB/PHY */
		SYS->USBPHY = (SYS->USBPHY &
			       ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) |
			      (SYS_USBPHY_HSUSBROLE_STD_USBD | SYS_USBPHY_HSUSBEN_Msk |
			       SYS_USBPHY_SBO_Msk);
		k_sleep(K_USEC(NUMAKER_HSUSBD_PHY_RESET_US));
		SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
#elif defined(CONFIG_SOC_SERIES_M55M1X)
		/* Configure HSUSB role as USB Device and enable HSUSB/PHY */
		SYS->USBPHY = (SYS->USBPHY &
			       ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) |
			      (SYS_USBPHY_HSUSBROLE_STD_USBD | SYS_USBPHY_HSOTGPHYEN_Msk);
		k_sleep(K_USEC(NUMAKER_HSUSBD_PHY_RESET_US));
		SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
#elif defined(CONFIG_SOC_SERIES_M333X)
		/* Configure HSUSB role as USB Device and enable HSUSB/PHY */
		SYS->USBPHY = (SYS->USBPHY &
			       ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) |
			      (SYS_USBPHY_HSUSBROLE_STD_USBD | SYS_USBPHY_HSUSBEN_Msk |
			       SYS_USBPHY_SBO_Msk);
		k_sleep(K_USEC(NUMAKER_HSUSBD_PHY_RESET_US));
		SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
#endif
	} else {
#if defined(CONFIG_SOC_SERIES_M46X)
		SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) |
			      (SYS_USBPHY_USBROLE_STD_USBD | SYS_USBPHY_USBEN_Msk |
			       SYS_USBPHY_SBO_Msk);
#elif defined(CONFIG_SOC_SERIES_M2L31X)
		SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) |
			      (SYS_USBPHY_USBROLE_STD_USBD | SYS_USBPHY_USBEN_Msk |
			       SYS_USBPHY_SBO_Msk);
#elif defined(CONFIG_SOC_SERIES_M55M1X)
		SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) |
			      (SYS_USBPHY_USBROLE_STD_USBD | SYS_USBPHY_OTGPHYEN_Msk);
#elif defined(CONFIG_SOC_SERIES_M333X)
		CODE_UNREACHABLE;
#endif
	}

	/* Invoke Clock controller to enable module clock */
	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = config->clk_modidx;
	scc_subsys.pcc.clk_src = config->clk_src;
	scc_subsys.pcc.clk_div = config->clk_div;

	/* Equivalent to CLK_EnableModuleClock() */
	err = clock_control_on(config->clkctrl_dev, (clock_control_subsys_t)&scc_subsys);
	if (err < 0) {
		goto cleanup;
	}
	/* Equivalent to CLK_SetModuleClock() */
	err = clock_control_configure(config->clkctrl_dev, (clock_control_subsys_t)&scc_subsys,
				      NULL);
	if (err < 0) {
		goto cleanup;
	}

	/* Configure pinmux (NuMaker's SYS MFP)
	 *
	 * NOTE: Take care of the case, e.g. M460 high-speed USB 2.0 device
	 * controller, whose pinouts are dedicated and needn't pinmux.
	 */
	if (config->pincfg) {
		err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err < 0) {
			goto cleanup;
		}
	}

	/* Invoke Reset controller to reset module to default state */
	/* Equivalent to SYS_ResetModule()
	 */
	reset_line_toggle_dt(&config->reset);

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		base->PHYCTL = 0;
	} else {
		USBD_T *base = config->base;

		/* Initialize USBD engine */
		/* NOTE: Per USBD spec, BIT(6) is hidden. */
		base->ATTR = USBD_ATTR_BYTEM_Msk | USBD_ATTR_PWRDN_Msk | BIT(6);
	}
	err = numaker_usbd_enable_usb_phy(dev);
	if (err < 0) {
		LOG_ERR("Enable USB/PHY failed");
		goto cleanup;
	}

	/* Set SE0 for S/W disconnect */
	numaker_usbd_sw_disconnect(dev);

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		/* Initiate high-speed negotiation (chirp during reset) */
#if defined(CONFIG_UDC_DRIVER_HIGH_SPEED_SUPPORT_ENABLED)
		switch (config->speed_idx) {
		case 0:
		case 1:
			base->OPER &= ~HSUSBD_OPER_HISPDEN_Msk;
			break;
		case 2:
		case 3:
		default:
			base->OPER |= HSUSBD_OPER_HISPDEN_Msk;
		}
#else
		base->OPER &= ~HSUSBD_OPER_HISPDEN_Msk;
#endif
	} else {
		/* NOTE: Ignore DT maximum-speed with USBD fixed to full-speed */
	}

	/* Initialize IRQ */
	config->irq_config_func(dev);
cleanup:

	SYS_LockReg();

	return err;
}

static void numaker_usbd_hw_shutdown(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	struct numaker_scc_subsys scc_subsys;

	SYS_UnlockReg();

	/* Uninitialize IRQ */
	config->irq_unconfig_func(dev);

	/* Set SE0 for S/W disconnect */
	numaker_usbd_sw_disconnect(dev);

	/* Disable USB/PHY */
	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		base->PHYCTL &= ~HSUSBD_PHYCTL_PHYEN_Msk;
	} else {
		USBD_T *base = config->base;

		base->ATTR &= ~USBD_PHY_EN;
	}

	/* Invoke Clock controller to disable module clock */
	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = config->clk_modidx;

	/* Equivalent to CLK_DisableModuleClock() */
	clock_control_off(config->clkctrl_dev, (clock_control_subsys_t)&scc_subsys);

	/* Invoke Reset controller to reset module to default state */
	/* Equivalent to SYS_ResetModule() */
	reset_line_toggle_dt(&config->reset);

	SYS_LockReg();
}

/* Interrupt top half processing for vbus plug */
static void numaker_usbd_vbus_plug_th(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	struct numaker_usbd_msg msg = {0};

	if (config->is_hsusbd) {
		/* For HSUSBD, enable back USB/PHY will be done in bottom-half for needed wait. */
	} else {
		USBD_T *base = config->base;

		/* Enable back USB/PHY */
		base->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;
	}

	/* Message for bottom-half processing */
	msg.type = NUMAKER_USBD_MSG_TYPE_ATTACH;
	numaker_usbd_send_msg(dev, &msg);

	LOG_DBG("USB plug-in");
}

/* Interrupt top half processing for vbus unplug */
static void numaker_usbd_vbus_unplug_th(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		/* Disable USB/PHY */
		if (config->allow_disable_usb_on_unplug) {
			base->PHYCTL &= ~HSUSBD_PHYCTL_PHYEN_Msk;
		}
	} else {
		USBD_T *base = config->base;

		/* Disable USB */
		if (config->allow_disable_usb_on_unplug) {
			base->ATTR &= ~USBD_USB_EN;
		}
	}

	/* UDC stack would handle bottom-half processing */
	udc_submit_event(dev, UDC_EVT_VBUS_REMOVED, 0);

	LOG_DBG("USB unplug");
}

/* Interrupt top half processing for bus wakeup */
static void numaker_usbd_bus_wakeup_th(const struct device *dev)
{
	LOG_DBG("USB wake-up");
}

/* Interrupt top half processing for bus reset */
static void numaker_usbd_bus_reset_th(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	USBD_T *base = config->base;
	struct udc_numaker_data *priv = udc_get_private(dev);
	struct numaker_usbd_ep *ep_cur = priv->ep_pool;
	struct numaker_usbd_ep *ep_end = priv->ep_pool + priv->ep_pool_size;
	USBD_EP_T *ep_base;
	struct numaker_usbd_msg msg = {0};

	/* Enable back USB/PHY */
	base->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;

	for (; ep_cur != ep_end; ep_cur++) {
		ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		/* For USBD, no separate EP interrupt control */

		/* Cancel EP on-going transaction */
		ep_base->CFGP |= USBD_CFGP_CLRRDY_Msk;

		/* Reset EP to unstalled */
		ep_base->CFGP &= ~USBD_CFGP_SSTALL_Msk;

		/* Reset EP data toggle bit to 0 */
		ep_base->CFG &= ~USBD_CFG_DSQSYNC_Msk;

		/* Except EP0/EP1 kept resident for CTRL OUT/IN, disable all other EPs */
		if (ep_cur->ep_hw_idx >= (EP0 + 2)) {
			ep_base->CFG = 0;
		}
	}

	numaker_usbd_reset_addr(dev);

	/* Message for bottom-half processing */
	msg.type = NUMAKER_USBD_MSG_TYPE_RESET;
	numaker_usbd_send_msg(dev, &msg);

	LOG_DBG("USB reset");
}

/* Interrupt top half processing for bus reset */
static void numaker_hsusbd_bus_reset_th(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	HSUSBD_T *base = config->base;
	struct udc_numaker_data *priv = udc_get_private(dev);
	struct numaker_usbd_ep *ep_cur = priv->ep_pool;
	struct numaker_usbd_ep *ep_end = priv->ep_pool + priv->ep_pool_size;
	HSUSBD_EP_T *ep_base;
	struct numaker_usbd_msg msg = {0};

	/* For HSUSBD, enable back USB/PHY will be done in bottom-half for needed wait. */

	for (; ep_cur != ep_end; ep_cur++) {
		ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		if (ep_cur->ep_hw_idx == CEP) {
			/* Disable CEP interrupt (exclude Setup) */
			base->CEPINTEN &= ~(HSUSBD_CEPINTEN_TXPKIEN_Msk |
					    HSUSBD_CEPINTEN_RXPKIEN_Msk);

			/* Flush CEP FIFO */
			base->CEPCTL = HSUSBD_CEPCTL_FLUSH | HSUSBD_CEPCTL_NAKCLR_Msk;

			/* CEP is resident and doesn't get disabled */
		} else {
			uint32_t eprspctl = ep_base->EPRSPCTL;

			/* Disable EP interrupt */
			ep_base->EPINTEN &= ~(HSUSBD_EPINTEN_TXPKIEN_Msk |
					      HSUSBD_EPINTEN_RXPKIEN_Msk);

			/* Flush EP FIFO */
			eprspctl |= HSUSBD_EP_RSPCTL_FLUSH;

			/* Reset EP to unstalled and its toggle bit to 0 */
			eprspctl &= ~(HSUSBD_EPRSPCTL_HALT_Msk | HSUSBD_EPRSPCTL_TOGGLE_Msk);
			eprspctl |= HSUSBD_EP_RSPCTL_TOGGLE;

			ep_base->EPRSPCTL = eprspctl;

			/* Disable all non-CTRL EPs */
			ep_base->EPCFG &= ~HSUSBD_EPCFG_EPEN_Msk;
		}
	}

	numaker_usbd_reset_addr(dev);

	/* Message for bottom-half processing */
	msg.type = NUMAKER_USBD_MSG_TYPE_RESET;
	numaker_usbd_send_msg(dev, &msg);

	LOG_DBG("USB reset");
}

/* Interrupt top half processing for bus suspend */
static void numaker_usbd_bus_suspend_th(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;

	if (config->is_hsusbd) {
		/* NOT disable USB/PHY
		 *
		 * For HSUSBD, unlike USBD, bus events (Reset/Suspend/Resume)
		 * will get unrecognized after USB/PHY is disabled.
		 */
	} else {
		USBD_T *base = config->base;

		/* Enable USB but disable PHY */
		base->ATTR &= ~USBD_PHY_EN;
	}

	/* UDC stack would handle bottom-half processing */
	udc_submit_event(dev, UDC_EVT_SUSPEND, 0);

	LOG_DBG("USB suspend");
}

/* Interrupt top half processing for bus resume */
static void numaker_usbd_bus_resume_th(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	struct numaker_usbd_msg msg = {0};

	if (config->is_hsusbd) {
		/* For HSUSBD, enable back USB/PHY will be done in bottom-half for needed wait. */
	} else {
		USBD_T *base = config->base;

		/* Enable back USB/PHY */
		base->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;
	}

	/* Message for bottom-half processing */
	msg.type = NUMAKER_USBD_MSG_TYPE_RESUME;
	numaker_usbd_send_msg(dev, &msg);

	LOG_DBG("USB resume");
}

/* Interrupt top half processing for SOF */
static void numaker_usbd_sof_th(const struct device *dev)
{
	/* UDC stack would handle bottom-half processing */
	udc_submit_sof_event(dev);
}

static void numaker_usbd_setup_copy_to_user(const struct device *dev, uint8_t *usrbuf);

/* Interrupt top half processing for Setup packet */
static void numaker_usbd_setup_th(const struct device *dev)
{
	USBD_EP_T *ep0_base = numaker_usbd_ep_base(dev, EP0);
	USBD_EP_T *ep1_base = numaker_usbd_ep_base(dev, EP1);
	struct numaker_usbd_msg msg = {0};

	/* Clear the data IN/OUT ready flag of control endpoints */
	ep0_base->CFGP |= USBD_CFGP_CLRRDY_Msk;
	ep1_base->CFGP |= USBD_CFGP_CLRRDY_Msk;

	/* By USB spec, following transactions, regardless of Data/Status stage,
	 * will always be DATA1
	 */
	ep0_base->CFG |= USBD_CFG_DSQSYNC_Msk;
	ep1_base->CFG |= USBD_CFG_DSQSYNC_Msk;

	/* Message for bottom-half processing */
	/* NOTE: In Zephyr USB device stack, Setup packet is passed via
	 * CTRL OUT EP
	 */
	msg.type = NUMAKER_USBD_MSG_TYPE_SETUP;
	numaker_usbd_setup_copy_to_user(dev, msg.setup.packet);
	numaker_usbd_send_msg(dev, &msg);
}

/* Interrupt top half processing for EP (excluding Setup) */
static void numaker_usbd_ep_th(const struct device *dev, uint32_t ep_hw_idx)
{
	struct udc_numaker_data *priv = udc_get_private(dev);
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_hw_idx);
	uint8_t ep_dir;
	uint8_t ep_idx;
	uint8_t ep;
	struct numaker_usbd_msg msg = {0};

	/* We don't enable INNAKEN interrupt, so as long as EP event occurs,
	 * we can just regard one data transaction has completed (ACK for
	 * CTRL/BULK/INT or no-ACK for Iso), that is, no need to check EPSTS0,
	 * EPSTS1, etc.
	 */

	/* EP direction, number, and address */
	ep_dir = ((ep_base->CFG & USBD_CFG_STATE_Msk) == USBD_CFG_EPMODE_IN) ? USB_EP_DIR_IN
									     : USB_EP_DIR_OUT;
	ep_idx = (ep_base->CFG & USBD_CFG_EPNUM_Msk) >> USBD_CFG_EPNUM_Pos;
	ep = USB_EP_GET_ADDR(ep_idx, ep_dir);

	/* NOTE: See comment in udc_numaker_set_address()'s implementation
	 * for safe place to change USB device address
	 */
	if (ep == USB_EP_GET_ADDR(0, USB_EP_DIR_IN)) {
		numaker_usbd_set_addr(dev);
	}

	/* NOTE: See comment on mxpld_ctrlout for why make one copy of
	 * CTRL OUT's MXPLD
	 */
	if (ep == USB_EP_GET_ADDR(0, USB_EP_DIR_OUT)) {
		struct numaker_usbd_ep *ep_ctrlout = priv->ep_pool + 0;

		ep_ctrlout->mxpld_ctrlout = (ep_base->MXPLD & USBD_MXPLD_MXPLD_Msk) >>
					    USBD_MXPLD_MXPLD_Pos;
	}

	/* Message for bottom-half processing */
	if (USB_EP_DIR_IS_OUT(ep)) {
		msg.type = NUMAKER_USBD_MSG_TYPE_OUT;
		msg.out.ep = ep;
	} else {
		msg.type = NUMAKER_USBD_MSG_TYPE_IN;
		msg.in.ep = ep;
	}
	numaker_usbd_send_msg(dev, &msg);
}

/* Interrupt top half processing for CTRL transfer */
static void numaker_hsusbd_cep_th(const struct device *dev, uint32_t cepintsts)
{
	const struct udc_numaker_config *config = dev->config;
	HSUSBD_T *base = config->base;
	struct numaker_usbd_msg msg = {0};

	/* Setup packet */
	if (cepintsts & HSUSBD_CEPINTSTS_SETUPPKIF_Msk) {
		/* By USB spec, following transactions, regardless of Data/Status stage,
		 * will always be DATA1. HSUBSD will handle the toggle by itself and needn't
		 * extra control.
		 */

		/* Message for bottom-half processing */
		/* NOTE: In Zephyr USB device stack, Setup packet is passed via
		 * CTRL OUT EP
		 */
		msg.type = NUMAKER_USBD_MSG_TYPE_SETUP;
		numaker_usbd_setup_copy_to_user(dev, msg.setup.packet);
		numaker_usbd_send_msg(dev, &msg);
	}

	/* Data packet received */
	if (cepintsts & HSUSBD_CEPINTSTS_RXPKIF_Msk) {
		/* Block until next CEP trigger */
		base->CEPINTEN &= ~HSUSBD_CEPINTEN_RXPKIEN_Msk;

		/* Message for bottom-half processing */
		msg.type = NUMAKER_USBD_MSG_TYPE_OUT;
		msg.out.ep = USB_CONTROL_EP_OUT;
		numaker_usbd_send_msg(dev, &msg);
	}

	/* Data packet transmitted */
	if (cepintsts & HSUSBD_CEPINTSTS_TXPKIF_Msk) {
		/* Block until next CEP trigger */
		base->CEPINTEN &= ~HSUSBD_CEPINTEN_TXPKIEN_Msk;

		/* Message for bottom-half processing */
		msg.type = NUMAKER_USBD_MSG_TYPE_IN;
		msg.in.ep = USB_CONTROL_EP_IN;
		numaker_usbd_send_msg(dev, &msg);
	}

	/* Status stage completed */
	if (cepintsts & HSUSBD_CEPINTSTS_STSDONEIF_Msk) {
		/* NOTE: See comment in udc_numaker_set_address()'s implementation
		 * for safe place to change USB device address
		 */
		if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
			numaker_usbd_set_addr(dev);
		}

		/* Message for bottom-half processing */
		if (udc_ctrl_stage_is_status_out(dev)) {
			msg.type = NUMAKER_USBD_MSG_TYPE_OUT;
			msg.out.ep = USB_CONTROL_EP_OUT;
		} else {
			msg.type = NUMAKER_USBD_MSG_TYPE_IN;
			msg.in.ep = USB_CONTROL_EP_IN;
		}
		numaker_usbd_send_msg(dev, &msg);
	}
}

/* Interrupt top half processing for BULK/INT/ISO transfer */
static void numaker_hsusbd_ep_th(const struct device *dev, uint32_t ep_hw_idx, uint32_t epintsts)
{
	HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_hw_idx);
	uint8_t ep_dir;
	uint8_t ep_idx;
	uint8_t ep;
	struct numaker_usbd_msg msg = {0};

	/* EP direction, number, and address */
	ep_dir = ((ep_base->EPCFG & HSUSBD_EPCFG_EPDIR_Msk) == HSUSBD_EP_CFG_DIR_IN)
			 ? USB_EP_DIR_IN
			 : USB_EP_DIR_OUT;
	ep_idx = (ep_base->EPCFG & HSUSBD_EPCFG_EPNUM_Msk) >> HSUSBD_EPCFG_EPNUM_Pos;
	ep = USB_EP_GET_ADDR(ep_idx, ep_dir);

	/* Block until next EP trigger */
	if (epintsts & HSUSBD_EPINTSTS_RXPKIF_Msk) {
		ep_base->EPINTEN &= ~HSUSBD_EPINTEN_RXPKIEN_Msk;
	} else {
		ep_base->EPINTEN &= ~HSUSBD_EPINTEN_TXPKIEN_Msk;
	}

	/* Message for bottom-half processing */
	if (USB_EP_DIR_IS_OUT(ep)) {
		msg.type = NUMAKER_USBD_MSG_TYPE_OUT;
		msg.out.ep = ep;
	} else {
		msg.type = NUMAKER_USBD_MSG_TYPE_IN;
		msg.in.ep = ep;
	}
	numaker_usbd_send_msg(dev, &msg);
}

/* USBD SRAM base for DMA */
static inline uint32_t numaker_usbd_buf_base(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	USBD_T *base = config->base;

	return ((uint32_t)base + 0x800ul);
}

/* Copy Setup packet to user buffer */
static void numaker_usbd_setup_copy_to_user(const struct device *dev, uint8_t *usrbuf)
{
	const struct udc_numaker_config *config = dev->config;

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		*usrbuf = (uint8_t)(base->SETUP1_0 & 0xfful);
		*(usrbuf + 1) = (uint8_t)((base->SETUP1_0 >> 8) & 0xfful);
		*(usrbuf + 2) = (uint8_t)(base->SETUP3_2 & 0xfful);
		*(usrbuf + 3) = (uint8_t)((base->SETUP3_2 >> 8) & 0xfful);
		*(usrbuf + 4) = (uint8_t)(base->SETUP5_4 & 0xfful);
		*(usrbuf + 5) = (uint8_t)((base->SETUP5_4 >> 8) & 0xfful);
		*(usrbuf + 6) = (uint8_t)(base->SETUP7_6 & 0xfful);
		*(usrbuf + 7) = (uint8_t)((base->SETUP7_6 >> 8) & 0xfful);
	} else {
		USBD_T *base = config->base;
		uint32_t dmabuf_addr;

		dmabuf_addr = numaker_usbd_buf_base(dev) +
			      (base->STBUFSEG & USBD_STBUFSEG_STBUFSEG_Msk);
		bytecpy(usrbuf, (uint8_t *)dmabuf_addr, 8ul);
	}
}

#if defined(CONFIG_UDC_NUMAKER_DMA)
/* Transfer data between user buffer and USB buffer by DMA
 *
 * size holds size to copy/copied on input/output
 */
static int numaker_hsusbd_ep_xfer_user_dma(struct numaker_usbd_ep *ep_cur, uint8_t *usrbuf,
					   uint32_t *size)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;
	struct udc_numaker_data *priv = udc_get_private(dev);
	HSUSBD_T *base = config->base;
	int err;

	/* Reset DMA semaphore */
	k_sem_reset(&priv->sem_dma_done);

	/* Reset DMA */
	base->DMACNT = 0;
	base->DMACTL = HSUSBD_DMACTL_DMARST_Msk;
	base->DMACTL = 0;
	base->BUSINTSTS = HSUSBD_BUSINTSTS_DMADONEIF_Msk;

	/* DMA memory address */
	base->DMAADDR = (uint32_t)usrbuf;

	/* DMA transfer size */
	base->DMACNT = *size;

	/* DMA EP address */
	base->DMACTL = USB_EP_DIR_IS_IN(ep_cur->addr)
			       ? (HSUSBD_DMACTL_SVINEP_Msk | HSUSBD_DMACTL_DMARD_Msk)
			       : 0;
	base->DMACTL |= USB_EP_GET_IDX(ep_cur->addr) << HSUSBD_DMACTL_EPNUM_Pos;

	/* Cache coherency */
	if (USB_EP_DIR_IS_IN(ep_cur->addr)) {
		sys_cache_data_flush_range(usrbuf, *size);
	} else {
		sys_cache_data_invd_range(usrbuf, *size);
	}

	/* Start DMA */
	base->DMACTL |= HSUSBD_DMACTL_DMAEN_Msk;

	/* Wait for DMA done */
	err = k_sem_take(&priv->sem_dma_done, K_MSEC(CONFIG_UDC_NUMAKER_DMA_TIMEOUT_MS));
	if (err != 0) {
		err = -EIO;

		/* Abort DMA for safe */
		base->DMACNT = 0;
		base->DMACTL = HSUSBD_DMACTL_DMARST_Msk;
		base->DMACTL = 0;
	}

	return err;
}
#endif

/* Copy data to user buffer
 *
 * size holds size to copy/copied on input/output
 */
static int numaker_hsusbd_ep_copy_to_user(struct numaker_usbd_ep *ep_cur, uint8_t *usrbuf,
					  uint32_t *size)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;
	HSUSBD_T *base = config->base;
	__maybe_unused HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	__ASSERT_NO_MSG(size);

	if (ep_cur->ep_hw_idx == CEP) {
		uint8_t *usrbuf_pos = usrbuf;
		uint32_t rmn = *size;

		while (rmn && !(base->CEPINTSTS & HSUSBD_CEPINTSTS_BUFEMPTYIF_Msk)) {
			*usrbuf_pos++ = base->CEPDAT_BYTE;
			rmn--;
		}

		*size -= rmn;
	} else {
#if defined(CONFIG_UDC_NUMAKER_DMA)
		int err = numaker_hsusbd_ep_xfer_user_dma(ep_cur, usrbuf, size);

		if (err < 0) {
			return err;
		}
#else
		uint8_t *usrbuf_pos = usrbuf;
		uint32_t rmn = *size;

		while (rmn && !(ep_base->EPINTSTS & HSUSBD_EPINTSTS_BUFEMPTYIF_Msk)) {
			*usrbuf_pos++ = ep_base->EPDAT_BYTE;
			rmn--;
		}

		*size -= rmn;
#endif
	}

	return 0;
}

/* Copy data from user buffer
 *
 * size holds size to copy/copied on input/output
 */
static int numaker_hsusbd_ep_copy_from_user(struct numaker_usbd_ep *ep_cur, const uint8_t *usrbuf,
					    uint32_t *size)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;
	HSUSBD_T *base = config->base;
	__maybe_unused HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	__ASSERT_NO_MSG(size);

	if (ep_cur->ep_hw_idx == CEP) {
		const uint8_t *usrbuf_pos = usrbuf;
		uint32_t rmn = *size;

		while (rmn && !(base->CEPINTSTS & HSUSBD_CEPINTSTS_BUFFULLIF_Msk)) {
			base->CEPDAT_BYTE = *usrbuf_pos++;
			rmn--;
		}

		*size -= rmn;
	} else {
#if defined(CONFIG_UDC_NUMAKER_DMA)
		int err = numaker_hsusbd_ep_xfer_user_dma(ep_cur, (uint8_t *)usrbuf, size);

		if (err < 0) {
			return err;
		}
#else
		const uint8_t *usrbuf_pos = usrbuf;
		uint32_t rmn = *size;

		while (rmn && !(ep_base->EPINTSTS & HSUSBD_EPINTSTS_BUFFULLIF_Msk)) {
			ep_base->EPDAT_BYTE = *usrbuf_pos++;
			rmn--;
		}

		*size -= rmn;
#endif
	}

	return 0;
}

/* Copy data to user buffer
 *
 * size holds size to copy/copied on input/output
 */
static int numaker_usbd_ep_copy_to_user(struct numaker_usbd_ep *ep_cur, uint8_t *usrbuf,
					uint32_t *size, uint32_t *rmn_p)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;
	uint32_t data_rmn;

	__ASSERT_NO_MSG(size);
	__ASSERT_NO_MSG(ep_cur->dmabuf_valid);

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;
		HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		if (ep_cur->ep_hw_idx == CEP) {
			data_rmn = (base->CEPDATCNT & HSUSBD_CEPDATCNT_DATCNT_Msk) >>
				   HSUSBD_CEPDATCNT_DATCNT_Pos;
		} else {
			data_rmn = (ep_base->EPDATCNT & HSUSBD_EPDATCNT_DATCNT_Msk) >>
				   HSUSBD_EPDATCNT_DATCNT_Pos;
		}
	} else {
		USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		/* NOTE: See comment on mxpld_ctrlout for why make one copy of CTRL OUT's MXPLD */
		if (ep_cur->addr == USB_CONTROL_EP_OUT) {
			data_rmn = ep_cur->mxpld_ctrlout;
		} else {
			data_rmn = (ep_base->MXPLD & USBD_MXPLD_MXPLD_Msk) >> USBD_MXPLD_MXPLD_Pos;
		}
	}

	*size = MIN(*size, data_rmn);

	if (config->is_hsusbd) {
		int err;

		err = numaker_hsusbd_ep_copy_to_user(ep_cur, usrbuf, size);
		if (err < 0) {
			return err;
		}
	} else {
		USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);
		uint32_t dmabuf_addr;

		dmabuf_addr = numaker_usbd_buf_base(dev) + ep_base->BUFSEG;
		bytecpy(usrbuf, (uint8_t *)dmabuf_addr, *size);
	}

	data_rmn -= *size;

	if (rmn_p) {
		*rmn_p = data_rmn;
	}

	return 0;
}

/* Copy data from user buffer
 *
 * size holds size to copy/copied on input/output
 */
static int numaker_usbd_ep_copy_from_user(struct numaker_usbd_ep *ep_cur, const uint8_t *usrbuf,
					  uint32_t *size)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;

	__ASSERT_NO_MSG(size);
	__ASSERT_NO_MSG(ep_cur->dmabuf_valid);
	__ASSERT_NO_MSG(ep_cur->mps_valid);
	__ASSERT_NO_MSG(ep_cur->mps <= ep_cur->dmabuf_size);

	*size = MIN(*size, ep_cur->mps);

	if (config->is_hsusbd) {
		int err;

		err = numaker_hsusbd_ep_copy_from_user(ep_cur, usrbuf, size);
		if (err < 0) {
			return err;
		}
	} else {
		USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);
		uint32_t dmabuf_addr;

		dmabuf_addr = numaker_usbd_buf_base(dev) + ep_base->BUFSEG;
		bytecpy((uint8_t *)dmabuf_addr, (uint8_t *)usrbuf, *size);
	}

	return 0;
}

static void numaker_usbd_ep_config_dmabuf(struct numaker_usbd_ep *ep_cur, uint32_t dmabuf_base,
					  uint32_t dmabuf_size)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;
		HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		if (ep_cur->ep_hw_idx == CEP) {
			base->CEPBUFSTART = dmabuf_base;
			base->CEPBUFEND = dmabuf_base + dmabuf_size - 1ul;
		} else {
			ep_base->EPBUFSTART = dmabuf_base;
			ep_base->EPBUFEND = dmabuf_base + dmabuf_size - 1ul;
		}
	} else {
		USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		ep_base->BUFSEG = dmabuf_base;
	}

	ep_cur->dmabuf_valid = true;
	ep_cur->dmabuf_base = dmabuf_base;
	ep_cur->dmabuf_size = dmabuf_size;
}

static void numaker_usbd_ep_abort(struct numaker_usbd_ep *ep_cur, bool excl_ctrl)
{
	struct udc_ep_config *ep_cfg;
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;
		HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		/* For HSUSBD, there is no control for aborting EP on-going
		 * transaction, but there is related control of flush EP FIFO.
		 */
		if (ep_cur->ep_hw_idx == CEP) {
			if (!excl_ctrl) {
				/* Flush CEP FIFO */
				base->CEPCTL = HSUSBD_CEPCTL_FLUSH | HSUSBD_CEPCTL_NAKCLR_Msk;
			}
		} else {
			/* Flush EP FIFO */
			uint32_t eprspctl = ep_base->EPRSPCTL;

			eprspctl &= ~HSUSBD_EPRSPCTL_TOGGLE_Msk;
			eprspctl |= HSUSBD_EP_RSPCTL_FLUSH;
			ep_base->EPRSPCTL = eprspctl;
		}
	} else {
		USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		/* Abort EP on-going transaction */
		if ((ep_cur->ep_hw_idx != EP0 && ep_cur->ep_hw_idx != EP1) || !excl_ctrl) {
			ep_base->CFGP |= USBD_CFGP_CLRRDY_Msk;
		}
	}

	if (ep_cur->addr_valid) {
		ep_cfg = udc_get_ep_cfg(dev, ep_cur->addr);
		udc_ep_set_busy(ep_cfg, false);
	}
}

/* Configure EP major common parts */
static void numaker_usbd_ep_config_major(struct numaker_usbd_ep *ep_cur,
					 struct udc_ep_config *const ep_cfg)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);
	uint8_t ep_type = ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK;

	ep_cur->mps_valid = true;
	ep_cur->mps = ep_cfg->mps;

	/* Configure EP transfer type, DATA0/1 toggle, direction, number, etc. */
	ep_cur->ep_hw_cfg = 0;

	/* Clear STALL Response in Setup stage */
	if (ep_type == USB_EP_TYPE_CONTROL) {
		ep_cur->ep_hw_cfg |= USBD_CFG_CSTALL;
	}

	/* Default to DATA0 */
	ep_cur->ep_hw_cfg &= ~USBD_CFG_DSQSYNC_Msk;

	/* Endpoint IN/OUT, though, default to disabled */
	ep_cur->ep_hw_cfg |= USBD_CFG_EPMODE_DISABLE;

	/* Isochronous or not */
	if (ep_type == USB_EP_TYPE_ISO) {
		ep_cur->ep_hw_cfg |= USBD_CFG_TYPE_ISO;
	}

	/* Endpoint index */
	ep_cur->ep_hw_cfg |= (USB_EP_GET_IDX(ep_cfg->addr) << USBD_CFG_EPNUM_Pos) &
			     USBD_CFG_EPNUM_Msk;

	ep_base->CFG = ep_cur->ep_hw_cfg;
}

/* Configure EP major common parts */
static void numaker_hsusbd_ep_config_major(struct numaker_usbd_ep *ep_cur,
					   struct udc_ep_config *const ep_cfg)
{
	const struct device *dev = ep_cur->dev;
	HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);
	uint8_t ep_type = ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK;

	ep_cur->mps_valid = true;
	ep_cur->mps = ep_cfg->mps;

	/* Configure EP transfer type, DATA0/1 toggle, direction, number, etc. */
	if (ep_cur->ep_hw_idx == CEP) {
		/* EP type: CONTROL */
		__ASSERT_NO_MSG(ep_type == USB_EP_TYPE_CONTROL);
	} else {
		ep_cur->ep_hw_cfg = 0;
		ep_cur->ep_hw_rspctl = 0;

		/* Default to DATA0 */
		ep_cur->ep_hw_rspctl |= HSUSBD_EPRSPCTL_TOGGLE_Msk;

		/* EP type: BULK/INT/ISO */
		switch (ep_type) {
		case USB_EP_TYPE_BULK:
			ep_cur->ep_hw_rspctl |= HSUSBD_EP_RSPCTL_MODE_AUTO;
			ep_cur->ep_hw_cfg |= HSUSBD_EP_CFG_TYPE_BULK;
			break;
		case USB_EP_TYPE_INTERRUPT:
			ep_cur->ep_hw_rspctl |= HSUSBD_EP_RSPCTL_MODE_MANUAL;
			ep_cur->ep_hw_cfg |= HSUSBD_EP_CFG_TYPE_INT;
			break;
		case USB_EP_TYPE_ISO:
			ep_cur->ep_hw_rspctl |= HSUSBD_EP_RSPCTL_MODE_FLY;
			ep_cur->ep_hw_cfg |= HSUSBD_EP_CFG_TYPE_ISO;
			break;
		default:
			__ASSERT_NO_MSG(0);
		}

		/* EP number */
		ep_cur->ep_hw_cfg |= (USB_EP_GET_IDX(ep_cfg->addr) << HSUSBD_EPCFG_EPNUM_Pos) &
				     HSUSBD_EPCFG_EPNUM_Msk;

		/* EP direction */
		if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
			ep_cur->ep_hw_cfg |= HSUSBD_EP_CFG_DIR_IN;
		} else {
			ep_cur->ep_hw_cfg |= HSUSBD_EP_CFG_DIR_OUT;
		}

		/* EP MPS */
		ep_base->EPMPS = ep_cfg->mps;

		/* Default to disabled (HSUSBD_EP_CFG_VALID unset) */

		ep_base->EPRSPCTL = ep_cur->ep_hw_rspctl;
		ep_base->EPCFG = ep_cur->ep_hw_cfg;
	}
}

static void numaker_usbd_ep_enable(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	/* For safe, EP (re-)enable from clean state */
	numaker_usbd_ep_abort(ep_cur, false);
	numaker_usbd_ep_clear_stall_n_data_toggle(ep_cur);

	/* Enable EP to IN/OUT */
	ep_cur->ep_hw_cfg &= ~USBD_CFG_STATE_Msk;
	if (USB_EP_DIR_IS_IN(ep_cur->addr)) {
		ep_cur->ep_hw_cfg |= USBD_CFG_EPMODE_IN;
	} else {
		ep_cur->ep_hw_cfg |= USBD_CFG_EPMODE_OUT;
	}
	ep_base->CFG = ep_cur->ep_hw_cfg;

	/* For USBD, no separate EP interrupt control */
}

static void numaker_hsusbd_ep_enable(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;
	HSUSBD_T *base = config->base;
	HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	/* For safe, EP (re-)enable from clean state */
	numaker_usbd_ep_abort(ep_cur, false);
	numaker_usbd_ep_clear_stall_n_data_toggle(ep_cur);

	if (ep_cur->ep_hw_idx == CEP) {
		/* CEP global interrupt should have enabled for resident. */

		/* To enable CEP local interrupt in CEP trigger */
	} else {
		/* Enable EP */
		ep_cur->ep_hw_cfg &= ~HSUSBD_EPCFG_EPEN_Msk;
		ep_cur->ep_hw_cfg |= HSUSBD_EP_CFG_VALID;
		ep_base->EPCFG = ep_cur->ep_hw_cfg;

		/* Enable EP global interrupt */
		base->GINTEN |= BIT(ep_cur->ep_hw_idx - EPA + HSUSBD_GINTEN_EPAIEN_Pos);

		/* To enable EP local interrupt in EP trigger */
	}
}

static void numaker_usbd_ep_disable(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	/* For USBD, no separate EP interrupt control */

	/* Disable EP */
	ep_cur->ep_hw_cfg = (ep_cur->ep_hw_cfg & ~USBD_CFG_STATE_Msk) | USBD_CFG_EPMODE_DISABLE;
	ep_base->CFG = ep_cur->ep_hw_cfg;
}

static void numaker_hsusbd_ep_disable(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;
	HSUSBD_T *base = config->base;
	HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	if (ep_cur->ep_hw_idx == CEP) {
		/* Disable CEP local interrupt */
		if (USB_EP_DIR_IS_IN(ep_cur->addr)) {
			base->CEPINTEN &= ~HSUSBD_CEPINTEN_TXPKIEN_Msk;
		} else {
			base->CEPINTEN &= ~HSUSBD_CEPINTEN_RXPKIEN_Msk;
		}

		/* CEP global interrupt shouldn't get disabled for resident. */
	} else {
		/* Disable EP local interrupt */
		if (USB_EP_DIR_IS_IN(ep_cur->addr)) {
			ep_base->EPINTEN &= ~HSUSBD_EPINTEN_TXPKIEN_Msk;
		} else {
			ep_base->EPINTEN &= ~HSUSBD_EPINTEN_RXPKIEN_Msk;
		}

		/* Disable EP global interrupt */
		base->GINTEN &= ~BIT(ep_cur->ep_hw_idx - EPA + HSUSBD_GINTEN_EPAIEN_Pos);

		/* Disable EP */
		ep_cur->ep_hw_cfg &= ~HSUSBD_EPCFG_EPEN_Msk;
		ep_base->EPCFG = ep_cur->ep_hw_cfg;
	}
}

/* Start EP data transaction */
static void numaker_hsusbd_ep_trigger(struct numaker_usbd_ep *ep_cur, uint32_t len)
{
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;
	HSUSBD_T *base = config->base;
	HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	if (ep_cur->ep_hw_idx == CEP) {
		if (USB_EP_DIR_IS_IN(ep_cur->addr)) {
			if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
				/* Unleash Status stage */
				base->CEPCTL = HSUSBD_CEPCTL_NAKCLR;
			}

			if (len == 0) {
				base->CEPCTL = HSUSBD_CEPCTL_ZEROLEN | HSUSBD_CEPCTL_NAKCLR_Msk;
			} else {
				__ASSERT_NO_MSG(len <= ep_cur->mps);
				base->CEPTXCNT = len;
			}

			/* Enable CEP interrupt */
			base->CEPINTEN |= HSUSBD_CEPINTEN_TXPKIEN_Msk;
		} else {
			if (udc_ctrl_stage_is_status_out(dev)) {
				/* Unleash Status stage */
				base->CEPCTL = HSUSBD_CEPCTL_NAKCLR;
			}

			/* Enable CEP interrupt */
			base->CEPINTEN |= HSUSBD_CEPINTEN_RXPKIEN_Msk;
		}
	} else {
		if (USB_EP_DIR_IS_IN(ep_cur->addr)) {
			uint32_t eprspctl = ep_base->EPRSPCTL;
			uint32_t eprspctl_mode = eprspctl & HSUSBD_EPRSPCTL_MODE_Msk;

			/* Not to change data toggle bit */
			eprspctl &= ~HSUSBD_EPRSPCTL_TOGGLE_Msk;

			if (eprspctl_mode == HSUSBD_EP_RSPCTL_MODE_AUTO) {
				if (len == 0) {
					eprspctl |= HSUSBD_EP_RSPCTL_ZEROLEN;
					ep_base->EPRSPCTL = eprspctl;
				} else if (len < ep_cur->mps) {
					eprspctl |= HSUSBD_EP_RSPCTL_SHORTTXEN;
					ep_base->EPRSPCTL = eprspctl;
				} else {
					__ASSERT_NO_MSG(len == ep_cur->mps);
					/* Tx automatic for mps size */
				}
			} else if (eprspctl_mode == HSUSBD_EP_RSPCTL_MODE_MANUAL) {
				if (len == 0) {
					eprspctl |= HSUSBD_EP_RSPCTL_ZEROLEN;
					ep_base->EPRSPCTL = eprspctl;
				} else {
					__ASSERT_NO_MSG(len <= ep_cur->mps);
					ep_base->EPTXCNT = len;
				}
			} else if (eprspctl_mode == HSUSBD_EP_RSPCTL_MODE_FLY) {
				__ASSERT_NO_MSG(len <= ep_cur->mps);
				/* Tx automatic for any size */
			} else {
				__ASSERT_NO_MSG(0);
			}

			/* Enable EP interrupt */
			ep_base->EPINTEN |= HSUSBD_EPINTEN_TXPKIEN_Msk;
		} else {
			/* Enable EP interrupt */
			ep_base->EPINTEN |= HSUSBD_EPINTEN_RXPKIEN_Msk;
		}
	}
}

/* Start EP data transaction */
static void numaker_usbd_ep_trigger(struct numaker_usbd_ep *ep_cur, uint32_t len)
{
	struct udc_ep_config *ep_cfg;
	const struct device *dev = ep_cur->dev;
	const struct udc_numaker_config *config = dev->config;

	__ASSERT_NO_MSG(ep_cur->addr_valid);

	ep_cfg = udc_get_ep_cfg(dev, ep_cur->addr);
	udc_ep_set_busy(ep_cfg, true);

	if (config->is_hsusbd) {
		numaker_hsusbd_ep_trigger(ep_cur, len);
	} else {
		USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

		ep_base->MXPLD = len;
	}
}

static struct numaker_usbd_ep *numaker_usbd_ep_mgmt_alloc_ep(const struct device *dev)
{
	struct udc_numaker_data *priv = udc_get_private(dev);
	struct numaker_usbd_ep_mgmt *ep_mgmt = &priv->ep_mgmt;
	struct numaker_usbd_ep *ep_cur = NULL;

	if (ep_mgmt->ep_idx < priv->ep_pool_size) {
		ep_cur = priv->ep_pool + ep_mgmt->ep_idx;
		ep_mgmt->ep_idx++;

		__ASSERT_NO_MSG(!ep_cur->valid);

		/* Indicate this EP H/W context is allocated */
		ep_cur->valid = true;
	}

	return ep_cur;
}

/* Allocate DMA buffer
 *
 * Return -ENOMEM  on OOM error, or 0 on success with DMA buffer base/size (rounded up) allocated
 */
static int numaker_usbd_ep_mgmt_alloc_dmabuf(const struct device *dev, uint32_t size,
					     uint32_t *dmabuf_base_p, uint32_t *dmabuf_size)
{
	const struct udc_numaker_config *config = dev->config;
	struct udc_numaker_data *priv = udc_get_private(dev);
	struct numaker_usbd_ep_mgmt *ep_mgmt = &priv->ep_mgmt;

	__ASSERT_NO_MSG(dmabuf_base_p);
	__ASSERT_NO_MSG(dmabuf_size);

	/* Required to be 8-byte aligned */
	size = ROUND_UP(size, 8);

	ep_mgmt->dmabuf_pos += size;
	if (ep_mgmt->dmabuf_pos > config->dmabuf_size) {
		ep_mgmt->dmabuf_pos -= size;
		return -ENOMEM;
	}

	*dmabuf_base_p = ep_mgmt->dmabuf_pos - size;
	*dmabuf_size = size;
	return 0;
}

/* Initialize all EP H/W contexts */
static void numaker_usbd_ep_mgmt_init(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	struct udc_numaker_data *priv = udc_get_private(dev);
	struct numaker_usbd_ep_mgmt *ep_mgmt = &priv->ep_mgmt;

	struct numaker_usbd_ep *ep_cur;
	struct numaker_usbd_ep *ep_end;

	/* Initialize all fields to zero for clean state */
	memset(ep_mgmt, 0x00, sizeof(*ep_mgmt));

	ep_cur = priv->ep_pool;
	ep_end = priv->ep_pool + priv->ep_pool_size;

	/* Initialize all EP H/W contexts */
	for (; ep_cur != ep_end; ep_cur++) {
		/* Zero-initialize */
		memset(ep_cur, 0x00, sizeof(*ep_cur));

		/* Pointer to the containing device */
		ep_cur->dev = dev;

		if (config->is_hsusbd) {
			/* BSP HSUSBD driver EP handle
			 *
			 * ep_pool[0]: CEP (CTRL OUT)
			 * ep_pool[1]: CEP (CTRL IN)
			 * ep_pool[2~]: EPA, EPB, etc.
			 */
			ep_cur->ep_hw_idx = EPA + (ep_cur - priv->ep_pool);
			if (ep_cur->ep_hw_idx == 0 || ep_cur->ep_hw_idx == 1) {
				ep_cur->ep_hw_idx = CEP;
			} else {
				ep_cur->ep_hw_idx -= 2;
			}
		} else {
			/* BSP USBD driver EP handle
			 *
			 * ep_pool[0]: EP0 (CTRL OUT)
			 * ep_pool[1]: EP1 (CTRL IN)
			 * ep_pool[2~]: EP2, EP3, etc.
			 */
			ep_cur->ep_hw_idx = EP0 + (ep_cur - priv->ep_pool);
		}
	}

	/* Reserve 1st/2nd EP H/W contexts for CTRL OUT/IN
	 *
	 * For USBD, EP0/EP1
	 * For HSUSBD, EPA/EPB
	 */
	ep_mgmt->ep_idx = 2;

	/* Reserve DMA buffer for Setup/CTRL OUT/CTRL IN, starting from 0 */
	ep_mgmt->dmabuf_pos = 0;

	/* Configure DMA buffer for Setup packet */
	if (config->is_hsusbd) {
		/* For HSUSBD, SETUP1_0, SETUP3_2, SETUP5_4, SETUP7_6 */
	} else {
		USBD_T *base = config->base;

		base->STBUFSEG = ep_mgmt->dmabuf_pos;
		ep_mgmt->dmabuf_pos += NUMAKER_USBD_DMABUF_SIZE_SETUP;
	}

	/* Reserve 1st EP H/W context for CTRL OUT */
	ep_cur = priv->ep_pool + 0;
	ep_cur->valid = true;
	ep_cur->addr_valid = true;
	ep_cur->addr = USB_EP_GET_ADDR(0, USB_EP_DIR_OUT);
	numaker_usbd_ep_config_dmabuf(ep_cur, ep_mgmt->dmabuf_pos,
				      NUMAKER_USBD_DMABUF_SIZE_CTRLOUT);
	ep_mgmt->dmabuf_pos += NUMAKER_USBD_DMABUF_SIZE_CTRLOUT;
	ep_cur->mps_valid = true;
	ep_cur->mps = NUMAKER_USBD_DMABUF_SIZE_CTRLOUT;

	/* Reserve 2nd EP H/W context for CTRL IN */
	ep_cur = priv->ep_pool + 1;
	ep_cur->valid = true;
	ep_cur->addr_valid = true;
	ep_cur->addr = USB_EP_GET_ADDR(0, USB_EP_DIR_IN);
	numaker_usbd_ep_config_dmabuf(ep_cur, ep_mgmt->dmabuf_pos, NUMAKER_USBD_DMABUF_SIZE_CTRLIN);
	ep_mgmt->dmabuf_pos += NUMAKER_USBD_DMABUF_SIZE_CTRLIN;
	ep_cur->mps_valid = true;
	ep_cur->mps = NUMAKER_USBD_DMABUF_SIZE_CTRLIN;
}

/* Find EP H/W context by EP address */
static struct numaker_usbd_ep *numaker_usbd_ep_mgmt_find_ep(const struct device *dev,
							    const uint8_t ep)
{
	struct udc_numaker_data *priv = udc_get_private(dev);
	struct numaker_usbd_ep *ep_cur = priv->ep_pool;
	struct numaker_usbd_ep *ep_end = priv->ep_pool + priv->ep_pool_size;

	for (; ep_cur != ep_end; ep_cur++) {
		if (!ep_cur->valid) {
			continue;
		}

		if (!ep_cur->addr_valid) {
			continue;
		}

		if (ep == ep_cur->addr) {
			return ep_cur;
		}
	}

	return NULL;
}

/* Bind EP H/W context to EP address */
static struct numaker_usbd_ep *numaker_usbd_ep_mgmt_bind_ep(const struct device *dev,
							    const uint8_t ep)
{
	struct numaker_usbd_ep *ep_cur = numaker_usbd_ep_mgmt_find_ep(dev, ep);

	if (!ep_cur) {
		ep_cur = numaker_usbd_ep_mgmt_alloc_ep(dev);

		if (!ep_cur) {
			return NULL;
		}

		/* Bind EP H/W context to EP address */
		ep_cur->addr = ep;
		ep_cur->addr_valid = true;
	}

	/* Assert EP H/W context bound to EP address */
	__ASSERT_NO_MSG(ep_cur->valid);
	__ASSERT_NO_MSG(ep_cur->addr_valid);
	__ASSERT_NO_MSG(ep_cur->addr == ep);

	return ep_cur;
}

static int numaker_usbd_xfer_out(const struct device *dev, uint8_t ep, bool strict)
{
	struct net_buf *buf;
	struct numaker_usbd_ep *ep_cur;
	struct udc_ep_config *ep_cfg;

	if (!USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Invalid EP address 0x%02x for data out", ep);
		return -EINVAL;
	}

	ep_cfg = udc_get_ep_cfg(dev, ep);
	if (udc_ep_is_busy(ep_cfg)) {
		if (strict) {
			LOG_ERR("EP 0x%02x busy", ep);
			return -EAGAIN;
		}

		return 0;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		if (strict) {
			LOG_ERR("No buffer queued for EP 0x%02x", ep);
			return -ENODATA;
		}

		return 0;
	}

	/* Bind EP H/W context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);
	if (!ep_cur) {
		LOG_ERR("Bind EP H/W context: ep=0x%02x", ep);
		return -ENODEV;
	}

	numaker_usbd_ep_trigger(ep_cur, ep_cur->mps);

	return 0;
}

static int numaker_usbd_xfer_in(const struct device *dev, uint8_t ep, bool strict)
{
	int err;
	struct net_buf *buf;
	struct numaker_usbd_ep *ep_cur;
	struct udc_ep_config *ep_cfg;
	uint32_t data_len;

	if (!USB_EP_DIR_IS_IN(ep)) {
		LOG_ERR("Invalid EP address 0x%02x for data in", ep);
		return -EINVAL;
	}

	ep_cfg = udc_get_ep_cfg(dev, ep);
	if (udc_ep_is_busy(ep_cfg)) {
		if (strict) {
			LOG_ERR("EP 0x%02x busy", ep);
			return -EAGAIN;
		}

		return 0;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		if (strict) {
			LOG_ERR("No buffer queued for EP 0x%02x", ep);
			return -ENODATA;
		}

		return 0;
	}

	/* Bind EP H/W context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);
	if (!ep_cur) {
		LOG_ERR("ep=0x%02x", ep);
		return -ENODEV;
	}

	data_len = buf->len;
	if (data_len) {
		err = numaker_usbd_ep_copy_from_user(ep_cur, buf->data, &data_len);
		if (err < 0) {
			LOG_ERR("Transfer to USB buffer failed: %d", err);
			return err;
		}
		net_buf_pull(buf, data_len);
	} else if (udc_ep_buf_has_zlp(buf)) {
		/* zlp, send exactly once */
		udc_ep_buf_clear_zlp(buf);
	} else {
		/* initially empty net_buf, send exactly once */
	}

	numaker_usbd_ep_trigger(ep_cur, data_len);

	return 0;
}

static int numaker_usbd_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct udc_numaker_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;

	ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	if (ep_cfg == NULL) {
		LOG_ERR("Bind udc_ep_config: ep=0x%02x", USB_CONTROL_EP_OUT);
		return -ENODEV;
	}

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		LOG_ERR("Allocate net_buf: ep=0x%02x", USB_CONTROL_EP_OUT);
		return -ENOMEM;
	}
	priv->ctrlout_tailroom = length;

	k_fifo_put(&ep_cfg->fifo, buf);

	return numaker_usbd_xfer_out(dev, ep_cfg->addr, true);
}

/* Message handler for device plug-in */
static int numaker_usbd_msg_handle_attach(const struct device *dev, struct numaker_usbd_msg *msg)
{
	const struct udc_numaker_config *config = dev->config;
	int err;

	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_ATTACH);

	if (config->is_hsusbd) {
		err = numaker_usbd_enable_usb_phy(dev);
		if (err < 0) {
			LOG_ERR("Enable USB/PHY failed");
			return -err;
		}
	} else {
		/* For USBD, enable back USB/PHY has done in ISR for unneeded wait. */
	}

	err = udc_submit_event(dev, UDC_EVT_VBUS_READY, 0);

	return err;
}

/* Message handler for bus reset */
static int numaker_usbd_msg_handle_reset(const struct device *dev, struct numaker_usbd_msg *msg)
{
	const struct udc_numaker_config *config = dev->config;
	int err;

	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_RESET);

	if (config->is_hsusbd) {
		err = numaker_usbd_enable_usb_phy(dev);
		if (err < 0) {
			LOG_ERR("Enable USB/PHY failed");
			return -err;
		}
	} else {
		/* For USBD, enable back USB/PHY has done in ISR for unneeded wait. */
	}

	/* UDC stack would handle bottom-half processing,
	 * including reset device address (udc_set_address),
	 * un-configure device (udc_ep_disable), etc.
	 */
	err = udc_submit_event(dev, UDC_EVT_RESET, 0);

	return err;
}

/* Message handler for bus resume */
static int numaker_usbd_msg_handle_resume(const struct device *dev, struct numaker_usbd_msg *msg)
{
	const struct udc_numaker_config *config = dev->config;
	int err;

	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_RESUME);

	if (config->is_hsusbd) {
		err = numaker_usbd_enable_usb_phy(dev);
		if (err < 0) {
			LOG_ERR("Enable USB/PHY failed");
			return -err;
		}
	} else {
		/* For USBD, enable back USB/PHY has done in ISR for unneeded wait. */
	}

	err = udc_submit_event(dev, UDC_EVT_RESUME, 0);

	return err;
}

/* Message handler for Setup transaction completed */
static int numaker_usbd_msg_handle_setup(const struct device *dev, struct numaker_usbd_msg *msg)
{
	const struct udc_numaker_config *config = dev->config;
	int err;
	uint8_t ep;
	struct numaker_usbd_ep *ep_cur;
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	uint8_t *data_ptr;

	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_SETUP);

	/* Recover from incomplete Control transfer
	 *
	 * Previous Control transfer can be incomplete, and causes not
	 * only net_buf leak but also logic error. This recycles dangling
	 * net_buf for new clean Control transfer.
	 */
	ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	buf = udc_buf_get_all(ep_cfg);
	if (buf != NULL) {
		net_buf_unref(buf);
	}
	ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	buf = udc_buf_get_all(ep_cfg);
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	ep = USB_CONTROL_EP_OUT;

	/* Bind EP H/W context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);
	if (!ep_cur) {
		LOG_ERR("Bind EP H/W context: ep=0x%02x", ep);
		return -ENODEV;
	}

	/* We should have reserved 1st/2nd EP H/W contexts for CTRL OUT/IN */
	__ASSERT_NO_MSG(ep_cur->addr == USB_CONTROL_EP_OUT);
	__ASSERT_NO_MSG((ep_cur + 1)->addr == USB_CONTROL_EP_IN);

	/* Abort previous CTRL OUT/IN */
	if (config->is_hsusbd) {
		/* For HSUSBD, there is timing concern between FIFO flush and
		 * immediately following Data OUT transaction. Even though FIFO flush
		 * is done in Setup token ISR (HSUSBD_CEPINTSTS_SETUPTKIF_Msk),
		 * it can still be not timely. For this, error recovery with FIFO
		 * is not done in-place here and rely on USB reset handler to do it
		 * as catch-all.
		 */
		numaker_usbd_ep_abort(ep_cur, true);
		numaker_usbd_ep_abort(ep_cur + 1, true);
	} else {
		numaker_usbd_ep_abort(ep_cur, false);
		numaker_usbd_ep_abort(ep_cur + 1, false);
	}

	/* CTRL OUT/IN reset to unstalled by H/W on receive of Setup packet */
	numaker_usbd_ep_sync_udc_halt(ep_cur, false);
	numaker_usbd_ep_sync_udc_halt(ep_cur + 1, false);

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate for Setup");
		return -ENOMEM;
	}

	udc_ep_buf_set_setup(buf);
	data_ptr = net_buf_tail(buf);
	memcpy(data_ptr, msg->setup.packet, 8);
	net_buf_add(buf, 8);

	/* Update to next stage of CTRL transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for DATA OUT stage */
		err = numaker_usbd_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		err = udc_ctrl_submit_s_status(dev);
	}

	return err;
}

/* Message handler for DATA OUT transaction completed */
static int numaker_usbd_msg_handle_out(const struct device *dev, struct numaker_usbd_msg *msg)
{
	struct udc_numaker_data *priv = udc_get_private(dev);
	int err;
	uint8_t ep;
	struct numaker_usbd_ep *ep_cur;
	struct udc_ep_config *ep_cfg;
	uint8_t ep_type;
	struct net_buf *buf;
	uint8_t *data_ptr;
	uint32_t data_len;
	uint32_t data_rmn;

	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_OUT);

	ep = msg->out.ep;
	ep_cfg = udc_get_ep_cfg(dev, ep);
	ep_type = ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK;

	udc_ep_set_busy(ep_cfg, false);

	/* Bind EP H/W context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);
	if (!ep_cur) {
		LOG_ERR("Bind EP H/W context: ep=0x%02x", ep);
		return -ENODEV;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer queued for ep=0x%02x", ep);
		return -ENODATA;
	}

	if (ep == USB_CONTROL_EP_OUT) {
		data_len = MIN(net_buf_tailroom(buf), priv->ctrlout_tailroom);
	} else {
		data_len = net_buf_tailroom(buf);
	}
	data_ptr = net_buf_tail(buf);
	err = numaker_usbd_ep_copy_to_user(ep_cur, data_ptr, &data_len, &data_rmn);
	if (err < 0) {
		LOG_ERR("Transfer from USB buffer failed: %d", err);
		return err;
	}
	net_buf_add(buf, data_len);
	if (ep == USB_CONTROL_EP_OUT) {
		__ASSERT_NO_MSG(priv->ctrlout_tailroom >= data_len);
		priv->ctrlout_tailroom -= data_len;
	}

	if (data_rmn) {
		LOG_ERR("Buffer (%p) queued for ep=0x%02x cannot accommodate packet", buf, ep);
		LOG_ERR("net_buf_tailroom(buf)=%d, data_len=%d, data_rmn=%d", net_buf_tailroom(buf),
			data_len, data_rmn);
		return -ENOBUFS;
	}

	/* CTRL DATA OUT/STATUS OUT stage completed */
	if (ep == USB_CONTROL_EP_OUT && priv->ctrlout_tailroom != 0) {
		goto next_xfer;
	}

	if (ep == USB_CONTROL_EP_OUT) {
		/* To submit the peeked buffer */
		udc_buf_get(ep_cfg);

		if (udc_ctrl_stage_is_status_out(dev)) {
			/* s-in-status finished */
			err = udc_ctrl_submit_status(dev, buf);
			if (err < 0) {
				LOG_ERR("udc_ctrl_submit_status failed for s-in-status: %d", err);
				return err;
			}
		}

		/* Update to next stage of CTRL transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_in(dev)) {
			err = udc_ctrl_submit_s_out_status(dev, buf);
			if (err < 0) {
				LOG_ERR("udc_ctrl_submit_s_out_status failed for s-out-status: %d",
					err);
				return err;
			}
		}
	} else if ((net_buf_tailroom(buf) == 0) || (data_len < ep_cfg->mps) ||
		   (ep_type == USB_EP_TYPE_ISO)) {
		/* Fix submit condition for non-control transfer
		 *
		 * Do submit when any of the following conditions is met:
		 * 1. Transfer buffer (net_buf) is full
		 * 2. Last packet size is less than mps
		 * 3. Isochronous transfer
		 */
		/* To submit the peeked buffer */
		udc_buf_get(ep_cfg);

		err = udc_submit_ep_event(dev, buf, 0);
		if (err < 0) {
			LOG_ERR("udc_submit_ep_event failed for ep=0x%02x: %d", ep, err);
			return err;
		}
	}

next_xfer:
	/* Continue with next DATA OUT transaction on request */
	numaker_usbd_xfer_out(dev, ep, false);

	return 0;
}

/* Message handler for DATA IN transaction completed */
static int numaker_usbd_msg_handle_in(const struct device *dev, struct numaker_usbd_msg *msg)
{
	int err;
	uint8_t ep;
	struct numaker_usbd_ep *ep_cur;
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;

	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_IN);

	ep = msg->in.ep;
	ep_cfg = udc_get_ep_cfg(dev, ep);

	udc_ep_set_busy(ep_cfg, false);

	/* Bind EP H/W context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);
	if (!ep_cur) {
		LOG_ERR("Bind EP H/W context: ep=0x%02x", ep);
		return -ENODEV;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		/* No DATA IN request */
		return 0;
	}

	if (buf->len || udc_ep_buf_has_zlp(buf)) {
		goto xfer_next;
	}

	/* To submit the peeked buffer */
	udc_buf_get(ep_cfg);

	if (ep == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) || udc_ctrl_stage_is_no_data(dev)) {
			/* s-out-status/s-status finished */
			err = udc_ctrl_submit_status(dev, buf);
			if (err < 0) {
				LOG_ERR("udc_ctrl_submit_status failed for s-out-status/s-status: "
					"%d",
					err);
				return err;
			}
		}

		/* Update to next stage of CTRL transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {
			/* DATA IN stage finished, release buffer */
			net_buf_unref(buf);

			/*  Allocate and feed buffer for STATUS OUT stage */
			err = numaker_usbd_ctrl_feed_dout(dev, 0);
			if (err < 0) {
				LOG_ERR("ctrl_feed_dout failed for status out: %d", err);
				return err;
			}
		}
	} else {
		err = udc_submit_ep_event(dev, buf, 0);
		if (err < 0) {
			LOG_ERR("udc_submit_ep_event failed for ep=0x%02x: %d", ep, err);
			return err;
		}
	}

xfer_next:
	/* Continue with next DATA IN transaction on request */
	numaker_usbd_xfer_in(dev, ep, false);

	return 0;
}

/* Message handler for queued transfer re-activated */
static int numaker_usbd_msg_handle_xfer(const struct device *dev, struct numaker_usbd_msg *msg)
{
	uint8_t ep;

	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_XFER);

	ep = msg->xfer.ep;

	if (USB_EP_DIR_IS_OUT(ep)) {
		numaker_usbd_xfer_out(dev, ep, false);
	} else {
		numaker_usbd_xfer_in(dev, ep, false);
	}

	return 0;
}

/* Message handler for S/W reconnect */
static int numaker_usbd_msg_handle_sw_reconn(const struct device *dev, struct numaker_usbd_msg *msg)
{
	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_SW_RECONN);

	/* S/W reconnect for error recovery */
	numaker_usbd_sw_reconnect(dev);

	return 0;
}

static void numaker_usbd_msg_handler(const struct device *dev)
{
	struct udc_numaker_data *priv = udc_get_private(dev);
	int err;
	struct numaker_usbd_msg msg;

	while (true) {
		if (k_msgq_get(priv->msgq, &msg, K_FOREVER)) {
			continue;
		}

		err = 0;

		udc_lock_internal(dev, K_FOREVER);

		switch (msg.type) {
		case NUMAKER_USBD_MSG_TYPE_ATTACH:
			err = numaker_usbd_msg_handle_attach(dev, &msg);
			break;

		case NUMAKER_USBD_MSG_TYPE_RESUME:
			err = numaker_usbd_msg_handle_resume(dev, &msg);
			break;

		case NUMAKER_USBD_MSG_TYPE_RESET:
			err = numaker_usbd_msg_handle_reset(dev, &msg);
			break;

		case NUMAKER_USBD_MSG_TYPE_SETUP:
			err = numaker_usbd_msg_handle_setup(dev, &msg);
			break;

		case NUMAKER_USBD_MSG_TYPE_OUT:
			err = numaker_usbd_msg_handle_out(dev, &msg);
			break;

		case NUMAKER_USBD_MSG_TYPE_IN:
			err = numaker_usbd_msg_handle_in(dev, &msg);
			break;

		case NUMAKER_USBD_MSG_TYPE_XFER:
			err = numaker_usbd_msg_handle_xfer(dev, &msg);
			break;

		case NUMAKER_USBD_MSG_TYPE_SW_RECONN:
			err = numaker_usbd_msg_handle_sw_reconn(dev, &msg);
			break;

		default:
			__ASSERT_NO_MSG(false);
		}

		udc_unlock_internal(dev);

		if (err) {
			udc_submit_event(dev, UDC_EVT_ERROR, err);
		}
	}
}

__maybe_unused static void numaker_usbd_isr(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	USBD_T *base = config->base;
	uint32_t usbd_intsts = base->INTSTS;
	uint32_t usbd_bus_state = base->ATTR;

	/* Focus on enabled
	 *
	 * NOTE: INTSTS has more interrupt bits than INTEN: SETUP and EPEVTx.
	 * For SETUP, it is added back for not missing.
	 * For EPEVTx, they are caught by EPINTSTS.
	 */
	usbd_intsts &= base->INTEN | USBD_INTSTS_SETUP;

	/* Clear event flag */
	base->INTSTS = usbd_intsts;

	/* USB plug-in/unplug */
	if (usbd_intsts & USBD_INTSTS_FLDET) {
		if (base->VBUSDET & USBD_VBUSDET_VBUSDET_Msk) {
			/* USB plug-in */
			numaker_usbd_vbus_plug_th(dev);
		} else {
			/* USB unplug */
			numaker_usbd_vbus_unplug_th(dev);
		}
	}

	/* USB wake-up */
	if (usbd_intsts & USBD_INTSTS_WAKEUP) {
		numaker_usbd_bus_wakeup_th(dev);
	}

	/* USB reset/suspend/resume */
	if (usbd_intsts & USBD_INTSTS_BUS) {
		/* Bus reset */
		if (usbd_bus_state & USBD_STATE_USBRST) {
			numaker_usbd_bus_reset_th(dev);
		}

		/* Bus suspend */
		if (usbd_bus_state & USBD_STATE_SUSPEND) {
			numaker_usbd_bus_suspend_th(dev);
		}

		/* Bus resume */
		if (usbd_bus_state & USBD_STATE_RESUME) {
			numaker_usbd_bus_resume_th(dev);
		}
	}

	/* USB SOF */
	if (usbd_intsts & USBD_INTSTS_SOFIF_Msk) {
		numaker_usbd_sof_th(dev);
	}

	/* USB Setup/EP */
	if (usbd_intsts & USBD_INTSTS_USB) {
		uint32_t epintsts;

		/* Setup event */
		if (usbd_intsts & USBD_INTSTS_SETUP) {
			numaker_usbd_setup_th(dev);
		}

		/* EP events */
		epintsts = base->EPINTSTS;

		/* Clear event flag */
		base->EPINTSTS = epintsts;

		while (epintsts) {
			uint32_t ep_hw_idx = u32_count_trailing_zeros(epintsts);

			numaker_usbd_ep_th(dev, ep_hw_idx);

			/* Have handled this EP and go next */
			epintsts &= ~BIT(ep_hw_idx - EP0);
		}
	}
}

__maybe_unused static void numaker_hsusbd_isr(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	struct udc_numaker_data *priv = udc_get_private(dev);
	HSUSBD_T *base = config->base;
	uint32_t gintsts = base->GINTSTS;
	uint32_t gintsts_ep = gintsts &
			      (BIT_MASK(priv->ep_pool_size - 2) << HSUSBD_GINTSTS_EPAIF_Pos);
	uint32_t busintsts = base->BUSINTSTS;
	uint32_t cepintsts = base->CEPINTSTS;

	/* Focus on enabled */
	busintsts &= base->BUSINTEN;
	cepintsts &= base->CEPINTEN;

	/* Clear event flag */
	base->BUSINTSTS = busintsts;

	/* USB plug-in/unplug */
	if (busintsts & HSUSBD_BUSINTSTS_VBUSDETIF_Msk) {
		if (base->PHYCTL & HSUSBD_PHYCTL_VBUSDET_Msk) {
			/* USB plug-in */
			numaker_usbd_vbus_plug_th(dev);
		} else {
			/* USB unplug */
			numaker_usbd_vbus_unplug_th(dev);
		}
	}

	/* Managed USB suspend interrupt
	 *
	 * For HSUSBD, on some chips e.g. M55M1, the semantics of USB suspend flag
	 * is state rather than event. To prevent CPU from overwhelming by this
	 * interrupt continuously, make it alarm one-shot instead of continuous.
	 */
	if (busintsts & (HSUSBD_BUSINTSTS_RSTIF_Msk | HSUSBD_BUSINTSTS_RESUMEIF_Msk)) {
		busintsts &= ~HSUSBD_BUSINTSTS_SUSPENDIF_Msk;
		base->BUSINTEN |= HSUSBD_BUSINTEN_SUSPENDIEN_Msk;
	} else if (busintsts & HSUSBD_BUSINTSTS_SUSPENDIF_Msk) {
		base->BUSINTEN &= ~HSUSBD_BUSINTEN_SUSPENDIEN_Msk;
	}

	/* USB reset */
	if (busintsts & HSUSBD_BUSINTSTS_RSTIF_Msk) {
		numaker_hsusbd_bus_reset_th(dev);
	}

	/* Bus suspend */
	if (busintsts & HSUSBD_BUSINTSTS_SUSPENDIF_Msk) {
		numaker_usbd_bus_suspend_th(dev);
	}

	/* Bus resume */
	if (busintsts & HSUSBD_BUSINTSTS_RESUMEIF_Msk) {
		numaker_usbd_bus_resume_th(dev);
	}

	/* USB SOF */
	if (busintsts & HSUSBD_BUSINTSTS_SOFIF_Msk) {
		numaker_usbd_sof_th(dev);
	}

	/* DMA done */
#if defined(CONFIG_UDC_NUMAKER_DMA)
	if (busintsts & HSUSBD_BUSINTSTS_DMADONEIF_Msk) {
		k_sem_give(&priv->sem_dma_done);
	}
#endif

	/* USB CEP */
	if (cepintsts) {
		/* Clear event flag */
		base->CEPINTSTS = cepintsts;

		numaker_hsusbd_cep_th(dev, cepintsts);
	}

	/* USB EP */
	if (gintsts_ep) {
		/* Iterate over EP from BIT0 position */
		uint32_t gintsts_ep_iter = gintsts_ep >> HSUSBD_GINTSTS_EPAIF_Pos;

		while (gintsts_ep_iter) {
			uint32_t ep_hw_idx = EPA + u32_count_trailing_zeros(gintsts_ep_iter);
			HSUSBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_hw_idx);
			uint32_t epintsts = ep_base->EPINTSTS;

			/* Focus on enabled */
			epintsts &= ep_base->EPINTEN;

			/* Clear event flag */
			ep_base->EPINTSTS = epintsts;

			numaker_hsusbd_ep_th(dev, ep_hw_idx, epintsts);

			/* Have handled this EP and go next */
			gintsts_ep_iter &= ~BIT(ep_hw_idx - EPA);
		}
	}
}

static enum udc_bus_speed udc_numaker_device_speed(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;

	if (config->is_hsusbd) {
		HSUSBD_T *base = config->base;

		return (base->OPER & HSUSBD_OPER_CURSPD_Msk) ? UDC_BUS_SPEED_HS : UDC_BUS_SPEED_FS;
	} else {
		return UDC_BUS_SPEED_FS;
	}
}

static int udc_numaker_ep_enqueue(const struct device *dev, struct udc_ep_config *const ep_cfg,
				  struct net_buf *buf)
{
	struct numaker_usbd_msg msg = {0};

	LOG_DBG("%p enqueue %p", dev, buf);
	udc_buf_put(ep_cfg, buf);

	/* Resume the EP's queued transfer */
	if (!ep_cfg->stat.halted) {
		msg.type = NUMAKER_USBD_MSG_TYPE_XFER;
		msg.xfer.ep = ep_cfg->addr;
		numaker_usbd_send_msg(dev, &msg);
	}

	return 0;
}

static int udc_numaker_ep_dequeue(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;
	struct numaker_usbd_ep *ep_cur;

	/* Bind EP H/W context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep_cfg->addr);
	if (!ep_cur) {
		LOG_ERR("Bind EP H/W context: ep=0x%02x", ep_cfg->addr);
		return -ENODEV;
	}

	numaker_usbd_ep_abort(ep_cur, false);

	buf = udc_buf_get_all(ep_cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	return 0;
}

static int udc_numaker_ep_set_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct numaker_usbd_ep *ep_cur;

	LOG_DBG("Set halt ep 0x%02x", ep_cfg->addr);

	/* Bind EP H/W context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep_cfg->addr);
	if (!ep_cur) {
		LOG_ERR("Bind EP H/W context: ep=0x%02x", ep_cfg->addr);
		return -ENODEV;
	}

	/* Set EP to stalled */
	numaker_usbd_ep_set_stall(ep_cur);

	return 0;
}

static int udc_numaker_ep_clear_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	struct numaker_usbd_ep *ep_cur;
	struct numaker_usbd_msg msg = {0};

	LOG_DBG("Clear halt ep 0x%02x", ep_cfg->addr);

	/* Bind EP H/W context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep_cfg->addr);
	if (!ep_cur) {
		LOG_ERR("Bind EP H/W context: ep=0x%02x", ep_cfg->addr);
		return -ENODEV;
	}

	/* Reset EP to unstalled and data toggle bit to 0 */
	numaker_usbd_ep_clear_stall_n_data_toggle(ep_cur);

	/* Resume the EP's queued transfer */
	msg.type = NUMAKER_USBD_MSG_TYPE_XFER;
	msg.xfer.ep = ep_cfg->addr;
	numaker_usbd_send_msg(dev, &msg);

	return 0;
}

static int udc_numaker_ep_enable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const struct udc_numaker_config *config = dev->config;
	int err;
	uint32_t dmabuf_base;
	uint32_t dmabuf_size;
	struct numaker_usbd_ep *ep_cur;

	LOG_DBG("Enable ep 0x%02x", ep_cfg->addr);

	/* Bind EP H/W context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep_cfg->addr);
	if (!ep_cur) {
		LOG_ERR("Bind EP H/W context: ep=0x%02x", ep_cfg->addr);
		return -ENODEV;
	}

	/* Configure EP DMA buffer */
	if (!ep_cur->dmabuf_valid || ep_cur->dmabuf_size < ep_cfg->mps) {
		/* Allocate DMA buffer */
		err = numaker_usbd_ep_mgmt_alloc_dmabuf(dev, ep_cfg->mps, &dmabuf_base,
							&dmabuf_size);
		if (err < 0) {
			LOG_ERR("Allocate DMA buffer failed");
			return err;
		}

		/* Configure EP DMA buffer */
		numaker_usbd_ep_config_dmabuf(ep_cur, dmabuf_base, dmabuf_size);
	}

	/* Configure EP majorly */
	if (config->is_hsusbd) {
		numaker_hsusbd_ep_config_major(ep_cur, ep_cfg);
	} else {
		numaker_usbd_ep_config_major(ep_cur, ep_cfg);
	}

	/* Enable EP */
	if (config->is_hsusbd) {
		numaker_hsusbd_ep_enable(ep_cur);
	} else {
		numaker_usbd_ep_enable(ep_cur);
	}

	return 0;
}

static int udc_numaker_ep_disable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const struct udc_numaker_config *config = dev->config;
	struct numaker_usbd_ep *ep_cur;

	LOG_DBG("Disable ep 0x%02x", ep_cfg->addr);

	/* Bind EP H/W context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep_cfg->addr);
	if (!ep_cur) {
		LOG_ERR("Bind EP H/W context: ep=0x%02x", ep_cfg->addr);
		return -ENODEV;
	}

	/* Disable EP */
	if (config->is_hsusbd) {
		numaker_hsusbd_ep_disable(ep_cur);
	} else {
		numaker_usbd_ep_disable(ep_cur);
	}

	return 0;
}

static void udc_numaker_usbd_gen_K(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	USBD_T *base = config->base;

	base->ATTR |= USBD_ATTR_RWAKEUP_Msk;
	k_sleep(K_USEC(NUMAKER_USBD_BUS_RESUME_DRV_K_US));
	base->ATTR ^= USBD_ATTR_RWAKEUP_Msk;
}

static void udc_numaker_hsusbd_gen_K(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	HSUSBD_T *base = config->base;

	base->OPER |= HSUSBD_OPER_RESUMEEN_Msk;
}

static int udc_numaker_host_wakeup(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	int err;

	/* Enable back USB/PHY first */
	err = numaker_usbd_enable_usb_phy(dev);
	if (err < 0) {
		LOG_ERR("Enable USB/PHY failed");
		return -EIO;
	}

	/* Then generate 'K' */
	if (config->is_hsusbd) {
		udc_numaker_hsusbd_gen_K(dev);
	} else {
		udc_numaker_usbd_gen_K(dev);
	}

	return 0;
}

static int udc_numaker_set_address(const struct device *dev, const uint8_t addr)
{
	struct udc_numaker_data *priv = udc_get_private(dev);

	LOG_DBG("Set new address %u for %p", addr, dev);

	/* NOTE: Timing for configuring USB device address into H/W is critical. It must be done
	 * in-between SET_ADDRESS control transfer and next transfer. For this, it is done in
	 * IN ACK ISR of SET_ADDRESS control transfer.
	 */
	priv->addr = addr;

	return 0;
}

static int udc_numaker_enable(const struct device *dev)
{
	LOG_DBG("Enable device %p", dev);

	/* S/W connect */
	numaker_usbd_sw_connect(dev);

	return 0;
}

static int udc_numaker_disable(const struct device *dev)
{
	LOG_DBG("Disable device %p", dev);

	/* S/W disconnect */
	numaker_usbd_sw_disconnect(dev);

	return 0;
}

static void udc_numaker_usbd_init_int_early(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	struct udc_data *data = dev->data;
	USBD_T *base = config->base;

	/* Enable VBUS detect early */
	if (data->caps.can_detect_vbus) {
		base->INTEN = USBD_INT_FLDET;
	} else {
		base->INTEN = 0;
	}

	/* Enable USB wake-up early */
	base->INTEN |= USBD_INT_WAKEUP;
}

static void udc_numaker_hsusbd_init_int_early(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	struct udc_data *data = dev->data;
	HSUSBD_T *base = config->base;

	/* Enable VBUS detect early */
	if (data->caps.can_detect_vbus) {
		base->BUSINTEN = HSUSBD_BUSINTEN_VBUSDETIEN_Msk;
	} else {
		base->BUSINTEN = 0;
	}

	/* Enable USB wake-up early */
	base->PHYCTL |= HSUSBD_PHYCTL_VBUSWKEN_Msk;
}

static int udc_numaker_init(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	int err;

	/* Initialize UDC H/W */
	err = numaker_usbd_hw_setup(dev);
	if (err < 0) {
		LOG_ERR("Set up H/W: %d", err);
		return err;
	}

	/* USB device address defaults to 0 */
	numaker_usbd_reset_addr(dev);

	/* Initialize all EP H/W contexts */
	numaker_usbd_ep_mgmt_init(dev);

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	/* Initialize interrupt early */
	if (config->is_hsusbd) {
		udc_numaker_hsusbd_init_int_early(dev);
	} else {
		udc_numaker_usbd_init_int_early(dev);
	}

	return 0;
}

static int udc_numaker_shutdown(const struct device *dev)
{
	struct udc_numaker_data *priv = udc_get_private(dev);

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	/* Uninitialize UDC H/W */
	numaker_usbd_hw_shutdown(dev);

	/* Purge message queue */
	k_msgq_purge(priv->msgq);

	return 0;
}

static void udc_numaker_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_numaker_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static int udc_numaker_driver_preinit(const struct device *dev)
{
	const struct udc_numaker_config *config = dev->config;
	struct udc_data *data = dev->data;
	__maybe_unused struct udc_numaker_data *priv = udc_get_private(dev);
	uint16_t mps = 1023;
	int err;

	if (config->is_hsusbd) {
		/* For HSUSBD, support both full-speed and high-speed */
		if (config->speed_idx >= 2) {
			data->caps.hs = true;
			mps = 1024;
		}
	} else {
		/* For USBD, support just full-speed */
	}
	data->caps.rwup = true;
	data->caps.addr_before_status = true;
	data->caps.can_detect_vbus = true;
	data->caps.mps0 = UDC_MPS0_64;

	/* Some soc series don't allow ISO IN/OUT to be assigned the same EP number.
	 * This is addressed by limiting all OUT/IN EP addresses in top/bottom halves,
	 * except CTRL OUT/IN.
	 */

	for (int i = 0; i < config->ep_cfg_out_size; i++) {
		/* Limit all OUT EP numbers to 0, 1~7 */
		if (config->disallow_iso_inout_same && i != 0 && i >= 8) {
			continue;
		}

		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = 64;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = mps;
		}

		config->ep_cfg_out[i].addr = USB_EP_DIR_OUT | i;
		err = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	for (int i = 0; i < config->ep_cfg_in_size; i++) {
		/* Limit all IN EP numbers to 0, 8~15 */
		if (config->disallow_iso_inout_same && i != 0 && i < 8) {
			continue;
		}

		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = 64;
		} else {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = mps;
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	config->make_thread(dev);

#if defined(CONFIG_UDC_NUMAKER_DMA)
	k_sem_init(&priv->sem_dma_done, 0, 1);
#endif

	return 0;
}

static const struct udc_api udc_numaker_api = {
	.device_speed = udc_numaker_device_speed,
	.ep_enqueue = udc_numaker_ep_enqueue,
	.ep_dequeue = udc_numaker_ep_dequeue,
	.ep_set_halt = udc_numaker_ep_set_halt,
	.ep_clear_halt = udc_numaker_ep_clear_halt,
	.ep_enable = udc_numaker_ep_enable,
	.ep_disable = udc_numaker_ep_disable,
	.host_wakeup = udc_numaker_host_wakeup,
	.set_address = udc_numaker_set_address,
	.enable = udc_numaker_enable,
	.disable = udc_numaker_disable,
	.init = udc_numaker_init,
	.shutdown = udc_numaker_shutdown,
	.lock = udc_numaker_lock,
	.unlock = udc_numaker_unlock,
};

#define NUMAKER_USBD_PINCTRL_DEV_CONFIG_GET(inst)                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(inst), pinctrl_0),                                \
		    (PINCTRL_DT_INST_DEV_CONFIG_GET(inst)), (NULL))

#define NUMAKER_USBD_PINCTRL_DEFINE(inst)                                                          \
	IF_ENABLED(DT_NODE_HAS_PROP(DT_DRV_INST(inst), pinctrl_0), (PINCTRL_DT_INST_DEFINE(inst)))

#define UDC_NUMAKER_ISR                                                                            \
	COND_CODE_1(UDC_NUMAKER_DEVICE_HSUSBD, (numaker_hsusbd_isr), (numaker_usbd_isr))

#define UDC_NUMAKER_SPEED_IDX_DEFAULT COND_CODE_1(UDC_NUMAKER_DEVICE_HSUSBD, (2), (1))

#define UDC_NUMAKER_DEVICE_DEFINE(inst)                                                            \
	NUMAKER_USBD_PINCTRL_DEFINE(inst);                                                         \
                                                                                                   \
	static void udc_numaker_irq_config_func_##inst(const struct device *dev)                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), UDC_NUMAKER_ISR,      \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
                                                                                                   \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	static void udc_numaker_irq_unconfig_func_##inst(const struct device *dev)                 \
	{                                                                                          \
		irq_disable(DT_INST_IRQN(inst));                                                   \
	}                                                                                          \
                                                                                                   \
	K_THREAD_STACK_DEFINE(udc_numaker_stack_##inst, CONFIG_UDC_NUMAKER_THREAD_STACK_SIZE);     \
                                                                                                   \
	static void udc_numaker_thread_##inst(void *dev, void *arg1, void *arg2)                   \
	{                                                                                          \
		ARG_UNUSED(arg1);                                                                  \
		ARG_UNUSED(arg2);                                                                  \
		numaker_usbd_msg_handler(dev);                                                     \
	}                                                                                          \
                                                                                                   \
	static void udc_numaker_make_thread_##inst(const struct device *dev)                       \
	{                                                                                          \
		struct udc_numaker_data *priv = udc_get_private(dev);                              \
                                                                                                   \
		k_thread_create(&priv->thread_data, udc_numaker_stack_##inst,                      \
				K_THREAD_STACK_SIZEOF(udc_numaker_stack_##inst),                   \
				udc_numaker_thread_##inst, (void *)dev, NULL, NULL,                \
				K_PRIO_COOP(CONFIG_UDC_NUMAKER_THREAD_PRIORITY), K_ESSENTIAL,      \
				K_NO_WAIT);                                                        \
		k_thread_name_set(&priv->thread_data, dev->name);                                  \
	}                                                                                          \
                                                                                                   \
	static struct udc_ep_config                                                                \
		ep_cfg_out_##inst[MIN(DT_INST_PROP(inst, num_bidir_endpoints), 16)];               \
	static struct udc_ep_config                                                                \
		ep_cfg_in_##inst[MIN(DT_INST_PROP(inst, num_bidir_endpoints), 16)];                \
                                                                                                   \
	static const struct udc_numaker_config udc_numaker_config_##inst = {                       \
		.ep_cfg_out = ep_cfg_out_##inst,                                                   \
		.ep_cfg_in = ep_cfg_in_##inst,                                                     \
		.ep_cfg_out_size = ARRAY_SIZE(ep_cfg_out_##inst),                                  \
		.ep_cfg_in_size = ARRAY_SIZE(ep_cfg_in_##inst),                                    \
		.make_thread = udc_numaker_make_thread_##inst,                                     \
		.base = (void *)DT_INST_REG_ADDR(inst),                                            \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),                       \
		.clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),                                \
		.clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),                               \
		.clkctrl_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),                \
		.irq_config_func = udc_numaker_irq_config_func_##inst,                             \
		.irq_unconfig_func = udc_numaker_irq_unconfig_func_##inst,                         \
		.pincfg = NUMAKER_USBD_PINCTRL_DEV_CONFIG_GET(inst),                               \
		.dmabuf_size = DT_INST_PROP(inst, dma_buffer_size),                                \
		.disallow_iso_inout_same = DT_INST_PROP_OR(inst, disallow_iso_in_out_same_number,  \
							   0),                                     \
		.allow_disable_usb_on_unplug = DT_INST_PROP_OR(inst, allow_disable_usb_on_unplug,  \
							       0),                                 \
		.speed_idx = DT_ENUM_IDX_OR(DT_DRV_INST(inst), maximum_speed,                      \
					    UDC_NUMAKER_SPEED_IDX_DEFAULT),                        \
		.is_hsusbd = IS_ENABLED(UDC_NUMAKER_DEVICE_HSUSBD),                                \
	};                                                                                         \
                                                                                                   \
	static struct numaker_usbd_ep                                                              \
		numaker_usbd_ep_pool_##inst[DT_INST_PROP(inst, num_bidir_endpoints)];              \
                                                                                                   \
	K_MSGQ_DEFINE(numaker_usbd_msgq_##inst, sizeof(struct numaker_usbd_msg),                   \
		      CONFIG_UDC_NUMAKER_MSG_QUEUE_SIZE, 4);                                       \
                                                                                                   \
	static struct udc_numaker_data udc_priv_##inst = {                                         \
		.msgq = &numaker_usbd_msgq_##inst,                                                 \
		.ep_pool = numaker_usbd_ep_pool_##inst,                                            \
		.ep_pool_size = DT_INST_PROP(inst, num_bidir_endpoints),                           \
	};                                                                                         \
                                                                                                   \
	static struct udc_data udc_data_##inst = {                                                 \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##inst.mutex),                               \
		.priv = &udc_priv_##inst,                                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, udc_numaker_driver_preinit, NULL, &udc_data_##inst,            \
			      &udc_numaker_config_##inst, POST_KERNEL,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &udc_numaker_api);

/* Define USBD devices */
#define DT_DRV_COMPAT nuvoton_numaker_usbd
DT_INST_FOREACH_STATUS_OKAY(UDC_NUMAKER_DEVICE_DEFINE)

/* Define HSUSBD devices */

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT             nuvoton_numaker_hsusbd
#define UDC_NUMAKER_DEVICE_HSUSBD 1
DT_INST_FOREACH_STATUS_OKAY(UDC_NUMAKER_DEVICE_DEFINE)
