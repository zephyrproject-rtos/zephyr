/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_usbd

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/dt-bindings/usb/usb.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_dc_numaker, CONFIG_USB_DRIVER_LOG_LEVEL);

#include <soc.h>
#include <NuMicro.h>

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

/* Maximum number of EP contexts across all instances
 * This is to static-allocate EP contexts which can accommodate all instances.
 * The number of effective EP contexts per instance is passed on through its
 * num_bidir_endpoints, which must not be larger than this.
 */
#define NUMAKER_USBD_EP_MAXNUM 25ul

/* Message type */
#define NUMAKER_USBD_MSG_TYPE_SW_RECONN 0 /* S/W reconnect */
#define NUMAKER_USBD_MSG_TYPE_CB_STATE  1 /* Callback for usb_dc_status_code */
#define NUMAKER_USBD_MSG_TYPE_CB_EP     2 /* Callback for usb_dc_ep_cb_status_code */

/* Message structure */
struct numaker_usbd_msg {
	uint32_t type;
	union {
		struct {
			enum usb_dc_status_code status_code;
		} cb_device;
		struct {
			uint8_t ep;
			enum usb_dc_ep_cb_status_code status_code;
		} cb_ep;
	};
};

/* Immutable device context */
struct numaker_usbd_config {
	USBD_T *base;
	const struct reset_dt_spec reset;
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clkctrl_dev;
	void (*irq_config_func)(const struct device *dev);
	void (*irq_unconfig_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
	uint32_t num_bidir_endpoints;
	uint32_t dmabuf_size;
	bool disallow_iso_inout_same;
};

/* EP context */
struct numaker_usbd_ep {
	bool valid;

	bool nak_clr; /* NAK cleared (ACK next transaction) */

	const struct device *dev; /* Pointer to the containing device */

	uint8_t ep_hw_idx;  /* BSP USBD driver EP index EP0, EP1, EP2, etc */
	uint32_t ep_hw_cfg; /* BSP USBD driver EP configuration */

	/* EP DMA buffer */
	bool dmabuf_valid;
	uint32_t dmabuf_base;
	uint32_t dmabuf_size;

	/* On USBD, no H/W FIFO. Simulate based on above DMA buffer with
	 * one-shot implementation
	 */
	uint32_t read_fifo_pos;
	uint32_t read_fifo_used;
	uint32_t write_fifo_pos;
	uint32_t write_fifo_free;

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

	usb_dc_ep_callback cb; /* EP callback function */
};

/* EP context manager */
struct numaker_usbd_ep_mgmt {
	/* EP context management
	 *
	 * Allocate-only, and de-allocate all on re-initialize in usb_dc_attach().
	 */
	uint8_t ep_idx;

	/* DMA buffer management
	 *
	 * Allocate-only, and de-allocate all on re-initialize in usb_dc_attach().
	 */
	uint32_t dmabuf_pos;

	/* Pass Setup packet from ISR to thread */
	bool new_setup;
	struct usb_setup_packet setup_packet;

	struct numaker_usbd_ep ep_pool[NUMAKER_USBD_EP_MAXNUM];
};

/* Mutable device context */
struct numaker_usbd_data {
	uint8_t addr; /* Host assigned USB device address */

	struct k_mutex sync_mutex;

	/* Enable interrupt top/bottom halves processing
	 *
	 * Registered callbacks may use mutex or other kernel functions which are not supported
	 * in interrupt context
	 */
	struct k_msgq msgq;
	struct numaker_usbd_msg msgq_buf[CONFIG_USB_DC_NUMAKER_MSG_QUEUE_SIZE];

	K_KERNEL_STACK_MEMBER(msg_hdlr_thread_stack,
			      CONFIG_USB_DC_NUMAKER_MSG_HANDLER_THREAD_STACK_SIZE);
	struct k_thread msg_hdlr_thread;

	usb_dc_status_callback status_cb; /* Status callback function */

	struct numaker_usbd_ep_mgmt ep_mgmt; /* EP management */
};

static inline const struct device *numaker_usbd_device_get(void);

static inline void numaker_usbd_lock(const struct device *dev)
{
	struct numaker_usbd_data *data = dev->data;

	k_mutex_lock(&data->sync_mutex, K_FOREVER);
}

static inline void numaker_usbd_unlock(const struct device *dev)
{
	struct numaker_usbd_data *data = dev->data;

	k_mutex_unlock(&data->sync_mutex);
}

static inline void numaker_usbd_sw_connect(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	USBD_T *const base = config->base;

	/* Clear all interrupts first for clean */
	base->INTSTS = base->INTSTS;

	/* Enable relevant interrupts */
	base->INTEN = USBD_INT_BUS | USBD_INT_USB | USBD_INT_FLDET | USBD_INT_WAKEUP | USBD_INT_SOF;

	/* Clear SE0 for connect */
	base->SE0 &= ~USBD_DRVSE0;
}

static inline void numaker_usbd_sw_disconnect(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	USBD_T *const base = config->base;

	/* Set SE0 for disconnect */
	base->SE0 |= USBD_DRVSE0;
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
	const struct numaker_usbd_config *config = dev->config;
	struct numaker_usbd_data *data = dev->data;
	USBD_T *const base = config->base;

	base->FADDR = 0;
	data->addr = 0;
}

static inline void numaker_usbd_set_addr(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	struct numaker_usbd_data *data = dev->data;
	USBD_T *const base = config->base;

	if (base->FADDR != data->addr) {
		base->FADDR = data->addr;
	}
}

/* USBD EP base by e.g. EP0, EP1, ... */
static inline USBD_EP_T *numaker_usbd_ep_base(const struct device *dev, uint32_t ep_hw_idx)
{
	const struct numaker_usbd_config *config = dev->config;
	USBD_T *const base = config->base;

	return base->EP + ep_hw_idx;
}

static inline uint32_t numaker_usbd_ep_fifo_max(struct numaker_usbd_ep *ep_cur)
{
	/* NOTE: For one-shot implementation, effective size of EP FIFO is limited to EP MPS */
	__ASSERT_NO_MSG(ep_cur->dmabuf_valid);
	__ASSERT_NO_MSG(ep_cur->mps_valid);
	__ASSERT_NO_MSG(ep_cur->mps <= ep_cur->dmabuf_size);
	return ep_cur->mps;
}

static inline uint32_t numaker_usbd_ep_fifo_used(struct numaker_usbd_ep *ep_cur)
{
	__ASSERT_NO_MSG(ep_cur->dmabuf_valid);

	return USB_EP_DIR_IS_OUT(ep_cur->addr)
		       ? ep_cur->read_fifo_used
		       : numaker_usbd_ep_fifo_max(ep_cur) - ep_cur->write_fifo_free;
}

/* Reset EP FIFO
 *
 * NOTE: EP FIFO is based on EP DMA buffer, which may not be configured yet.
 */
static void numaker_usbd_ep_fifo_reset(struct numaker_usbd_ep *ep_cur)
{
	if (ep_cur->dmabuf_valid && ep_cur->mps_valid) {
		if (USB_EP_DIR_IS_OUT(ep_cur->addr)) {
			/* Read FIFO */
			ep_cur->read_fifo_pos = ep_cur->dmabuf_base;
			ep_cur->read_fifo_used = 0;
		} else {
			/* Write FIFO */
			ep_cur->write_fifo_pos = ep_cur->dmabuf_base;
			ep_cur->write_fifo_free = numaker_usbd_ep_fifo_max(ep_cur);
		}
	}
}

static inline void numaker_usbd_ep_set_stall(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	/* Set EP to stalled */
	ep_base->CFGP |= USBD_CFGP_SSTALL_Msk;
}

/* Reset EP to unstalled and data toggle bit to 0 */
static inline void numaker_usbd_ep_clear_stall_n_data_toggle(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	/* Reset EP to unstalled */
	ep_base->CFGP &= ~USBD_CFGP_SSTALL_Msk;

	/* Reset EP data toggle bit to 0 */
	ep_base->CFG &= ~USBD_CFG_DSQSYNC_Msk;
}

static inline bool numaker_usbd_ep_is_stalled(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	return ep_base->CFGP & USBD_CFGP_SSTALL_Msk;
}

static int numaker_usbd_send_msg(const struct device *dev, const struct numaker_usbd_msg *msg)
{
	struct numaker_usbd_data *data = dev->data;
	int rc;

	rc = k_msgq_put(&data->msgq, msg, K_NO_WAIT);
	if (rc < 0) {
		/* Try to recover by S/W reconnect */
		struct numaker_usbd_msg msg_reconn = {
			.type = NUMAKER_USBD_MSG_TYPE_SW_RECONN,
		};

		LOG_ERR("Message queue overflow");

		/* Discard all not yet received messages for error recovery below */
		k_msgq_purge(&data->msgq);

		rc = k_msgq_put(&data->msgq, &msg_reconn, K_NO_WAIT);
		if (rc < 0) {
			LOG_ERR("Message queue overflow again");
		}
	}

	return rc;
}

static int numaker_usbd_hw_setup(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	USBD_T *const base = config->base;
	int rc;
	struct numaker_scc_subsys scc_subsys;

	/* Reset controller ready? */
	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("Reset controller not ready");
		return -ENODEV;
	}

	SYS_UnlockReg();

	/* Configure USB PHY for USBD */
	SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) |
		      (SYS_USBPHY_USBROLE_STD_USBD | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk);

	/* Invoke Clock controller to enable module clock */
	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = config->clk_modidx;
	scc_subsys.pcc.clk_src = config->clk_src;
	scc_subsys.pcc.clk_div = config->clk_div;

	/* Equivalent to CLK_EnableModuleClock() */
	rc = clock_control_on(config->clkctrl_dev, (clock_control_subsys_t)&scc_subsys);
	if (rc < 0) {
		goto cleanup;
	}
	/* Equivalent to CLK_SetModuleClock() */
	rc = clock_control_configure(config->clkctrl_dev, (clock_control_subsys_t)&scc_subsys,
				     NULL);
	if (rc < 0) {
		goto cleanup;
	}

	/* Configure pinmux (NuMaker's SYS MFP) */
	rc = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		goto cleanup;
	}

	/* Invoke Reset controller to reset module to default state */
	/* Equivalent to SYS_ResetModule()
	 */
	reset_line_toggle_dt(&config->reset);

	/* Initialize USBD engine */
	/* NOTE: BSP USBD driver: ATTR = 0x7D0 */
	base->ATTR = USBD_ATTR_BYTEM_Msk | BIT(9) | USBD_ATTR_DPPUEN_Msk | USBD_ATTR_USBEN_Msk |
		     BIT(6) | USBD_ATTR_PHYEN_Msk;

	/* Set SE0 for S/W disconnect */
	numaker_usbd_sw_disconnect(dev);

	/* NOTE: Ignore DT maximum-speed with USBD fixed to full-speed */

	/* Initialize IRQ */
	config->irq_config_func(dev);

cleanup:

	SYS_LockReg();

	return rc;
}

static void numaker_usbd_hw_shutdown(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	USBD_T *const base = config->base;
	struct numaker_scc_subsys scc_subsys;

	SYS_UnlockReg();

	/* Uninitialize IRQ */
	config->irq_unconfig_func(dev);

	/* Set SE0 for S/W disconnect */
	numaker_usbd_sw_disconnect(dev);

	/* Disable USB PHY */
	base->ATTR &= ~USBD_PHY_EN;

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

/* Interrupt top half processing for bus reset */
static void numaker_usbd_bus_reset_th(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	USBD_EP_T *ep_base;

	for (uint32_t i = 0ul; i < config->num_bidir_endpoints; i++) {
		ep_base = numaker_usbd_ep_base(dev, EP0 + i);

		/* Cancel EP on-going transaction */
		ep_base->CFGP |= USBD_CFGP_CLRRDY_Msk;

		/* Reset EP to unstalled */
		ep_base->CFGP &= ~USBD_CFGP_SSTALL_Msk;

		/* Reset EP data toggle bit to 0 */
		ep_base->CFG &= ~USBD_CFG_DSQSYNC_Msk;

		/* Except EP0/EP1 kept resident for CTRL OUT/IN, disable all other EPs */
		if (i >= 2) {
			ep_base->CFG = 0;
		}
	}

	numaker_usbd_reset_addr(dev);
}

static void numaker_usbd_remote_wakeup(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	USBD_T *const base = config->base;

	/* Enable back USB/PHY first */
	base->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;

	/* Then generate 'K' */
	base->ATTR |= USBD_ATTR_RWAKEUP_Msk;
	k_sleep(K_USEC(NUMAKER_USBD_BUS_RESUME_DRV_K_US));
	base->ATTR ^= USBD_ATTR_RWAKEUP_Msk;
}

/* USBD SRAM base for DMA */
static inline uint32_t numaker_usbd_buf_base(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	USBD_T *const base = config->base;

	return ((uint32_t)base + 0x800ul);
}

/* Copy to user buffer from Setup FIFO */
static void numaker_usbd_setup_fifo_copy_to_user(const struct device *dev, uint8_t *usrbuf)
{
	const struct numaker_usbd_config *config = dev->config;
	USBD_T *const base = config->base;
	uint32_t dmabuf_addr;

	dmabuf_addr = numaker_usbd_buf_base(dev) + (base->STBUFSEG & USBD_STBUFSEG_STBUFSEG_Msk);

	bytecpy(usrbuf, (uint8_t *)dmabuf_addr, 8ul);
}

/* Copy data to user buffer from EP FIFO
 *
 * size_p holds size to copy/copied on input/output
 */
static int numaker_usbd_ep_fifo_copy_to_user(struct numaker_usbd_ep *ep_cur, uint8_t *usrbuf,
					     uint32_t *size_p)
{
	const struct device *dev = ep_cur->dev;
	uint32_t dmabuf_addr;

	__ASSERT_NO_MSG(size_p);
	__ASSERT_NO_MSG(ep_cur->dmabuf_valid);

	dmabuf_addr = numaker_usbd_buf_base(dev) + ep_cur->read_fifo_pos;

	/* Clamp to read FIFO used count */
	*size_p = MIN(*size_p, numaker_usbd_ep_fifo_used(ep_cur));

	bytecpy(usrbuf, (uint8_t *)dmabuf_addr, *size_p);

	/* Advance read FIFO */
	ep_cur->read_fifo_pos += *size_p;
	ep_cur->read_fifo_used -= *size_p;
	if (ep_cur->read_fifo_used == 0) {
		ep_cur->read_fifo_pos = ep_cur->dmabuf_base;
	}

	return 0;
}

/* Copy data from user buffer to EP FIFO
 *
 * size_p holds size to copy/copied on input/output
 */
static int numaker_usbd_ep_fifo_copy_from_user(struct numaker_usbd_ep *ep_cur,
					       const uint8_t *usrbuf, uint32_t *size_p)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);
	uint32_t dmabuf_addr;
	uint32_t fifo_free;

	__ASSERT_NO_MSG(size_p);
	__ASSERT_NO_MSG(ep_cur->dmabuf_valid);
	__ASSERT_NO_MSG(ep_cur->mps_valid);
	__ASSERT_NO_MSG(ep_cur->mps <= ep_cur->dmabuf_size);

	dmabuf_addr = numaker_usbd_buf_base(dev) + ep_base->BUFSEG;
	fifo_free = numaker_usbd_ep_fifo_max(ep_cur) - numaker_usbd_ep_fifo_used(ep_cur);

	*size_p = MIN(*size_p, fifo_free);

	bytecpy((uint8_t *)dmabuf_addr, (uint8_t *)usrbuf, *size_p);

	/* Advance write FIFO */
	ep_cur->write_fifo_pos += *size_p;
	ep_cur->write_fifo_free -= *size_p;
	if (ep_cur->write_fifo_free == 0) {
		ep_cur->write_fifo_pos = ep_cur->dmabuf_base;
	}

	return 0;
}

/* Update EP read/write FIFO on DATA OUT/IN completed */
static void numaker_usbd_ep_fifo_update(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	__ASSERT_NO_MSG(ep_cur->addr_valid);
	__ASSERT_NO_MSG(ep_cur->dmabuf_valid);

	if (USB_EP_DIR_IS_OUT(ep_cur->addr)) {
		/* Read FIFO */
		/* NOTE: For one-shot implementation, FIFO gets updated from empty. */
		ep_cur->read_fifo_pos = ep_cur->dmabuf_base;
		/* NOTE: See comment on mxpld_ctrlout for why make one copy of CTRL OUT's MXPLD */
		if (USB_EP_GET_IDX(ep_cur->addr) == 0) {
			ep_cur->read_fifo_used = ep_cur->mxpld_ctrlout;
		} else {
			ep_cur->read_fifo_used = ep_base->MXPLD;
		}
	} else {
		/* Write FIFO */
		/* NOTE: For one-shot implementation, FIFO gets to empty. */
		ep_cur->write_fifo_pos = ep_cur->dmabuf_base;
		ep_cur->write_fifo_free = numaker_usbd_ep_fifo_max(ep_cur);
	}
}

static void numaker_usbd_ep_config_dmabuf(struct numaker_usbd_ep *ep_cur, uint32_t dmabuf_base,
					  uint32_t dmabuf_size)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	ep_base->BUFSEG = dmabuf_base;

	ep_cur->dmabuf_valid = true;
	ep_cur->dmabuf_base = dmabuf_base;
	ep_cur->dmabuf_size = dmabuf_size;
}

static void numaker_usbd_ep_abort(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	/* Abort EP on-going transaction */
	ep_base->CFGP |= USBD_CFGP_CLRRDY_Msk;

	/* Need to clear NAK for next transaction */
	ep_cur->nak_clr = false;
}

/* Configure EP major common parts */
static void numaker_usbd_ep_config_major(struct numaker_usbd_ep *ep_cur,
					 const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	ep_cur->mps_valid = true;
	ep_cur->mps = ep_cfg->ep_mps;

	/* Configure EP transfer type, DATA0/1 toggle, direction, number, etc. */
	ep_cur->ep_hw_cfg = 0;

	/* Clear STALL Response in Setup stage */
	if (ep_cfg->ep_type == USB_DC_EP_CONTROL) {
		ep_cur->ep_hw_cfg |= USBD_CFG_CSTALL;
	}

	/* Default to DATA0 */
	ep_cur->ep_hw_cfg &= ~USBD_CFG_DSQSYNC_Msk;

	/* Endpoint IN/OUT, though, default to disabled */
	ep_cur->ep_hw_cfg |= USBD_CFG_EPMODE_DISABLE;

	/* Isochronous or not */
	if (ep_cfg->ep_type == USB_DC_EP_ISOCHRONOUS) {
		ep_cur->ep_hw_cfg |= USBD_CFG_TYPE_ISO;
	}

	/* Endpoint index */
	ep_cur->ep_hw_cfg |=
		(USB_EP_GET_IDX(ep_cfg->ep_addr) << USBD_CFG_EPNUM_Pos) & USBD_CFG_EPNUM_Msk;

	ep_base->CFG = ep_cur->ep_hw_cfg;
}

static void numaker_usbd_ep_enable(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	/* For safe, EP (re-)enable from clean state */
	numaker_usbd_ep_abort(ep_cur);
	numaker_usbd_ep_clear_stall_n_data_toggle(ep_cur);
	numaker_usbd_ep_fifo_reset(ep_cur);

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

static void numaker_usbd_ep_disable(struct numaker_usbd_ep *ep_cur)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	/* For USBD, no separate EP interrupt control */

	/* Disable EP */
	ep_cur->ep_hw_cfg = (ep_cur->ep_hw_cfg & ~USBD_CFG_STATE_Msk) | USBD_CFG_EPMODE_DISABLE;
	ep_base->CFG = ep_cur->ep_hw_cfg;
}

/* Start EP data transaction */
static void numaker_usbd_ep_trigger(struct numaker_usbd_ep *ep_cur, uint32_t len)
{
	const struct device *dev = ep_cur->dev;
	USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_cur->ep_hw_idx);

	ep_base->MXPLD = len;
}

static struct numaker_usbd_ep *numaker_usbd_ep_mgmt_alloc_ep(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	struct numaker_usbd_data *data = dev->data;
	struct numaker_usbd_ep_mgmt *ep_mgmt = &data->ep_mgmt;
	struct numaker_usbd_ep *ep_cur = NULL;

	if (ep_mgmt->ep_idx < config->num_bidir_endpoints) {
		ep_cur = ep_mgmt->ep_pool + ep_mgmt->ep_idx;
		ep_mgmt->ep_idx++;

		__ASSERT_NO_MSG(!ep_cur->valid);

		/* Indicate this EP context is allocated */
		ep_cur->valid = true;
	}

	return ep_cur;
}

/* Allocate DMA buffer
 *
 * Return -ENOMEM  on OOM error, or 0 on success with DMA buffer base/size (rounded up) allocated
 */
static int numaker_usbd_ep_mgmt_alloc_dmabuf(const struct device *dev, uint32_t size,
					     uint32_t *dmabuf_base_p, uint32_t *dmabuf_size_p)
{
	const struct numaker_usbd_config *config = dev->config;
	struct numaker_usbd_data *data = dev->data;
	struct numaker_usbd_ep_mgmt *ep_mgmt = &data->ep_mgmt;

	__ASSERT_NO_MSG(dmabuf_base_p);
	__ASSERT_NO_MSG(dmabuf_size_p);

	/* Required to be 8-byte aligned */
	size = ROUND_UP(size, 8);

	ep_mgmt->dmabuf_pos += size;
	if (ep_mgmt->dmabuf_pos > config->dmabuf_size) {
		ep_mgmt->dmabuf_pos -= size;
		return -ENOMEM;
	}

	*dmabuf_base_p = ep_mgmt->dmabuf_pos - size;
	*dmabuf_size_p = size;
	return 0;
}

/* Initialize all endpoint-related */
static void numaker_usbd_ep_mgmt_init(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	struct numaker_usbd_data *data = dev->data;
	USBD_T *const base = config->base;
	struct numaker_usbd_ep_mgmt *ep_mgmt = &data->ep_mgmt;

	struct numaker_usbd_ep *ep_cur;
	struct numaker_usbd_ep *ep_end;

	/* Initialize all fields to zero except persistent */
	memset(ep_mgmt, 0x00, sizeof(*ep_mgmt));

	ep_cur = ep_mgmt->ep_pool;
	ep_end = ep_mgmt->ep_pool + config->num_bidir_endpoints;

	/* Initialize all EP contexts */
	for (; ep_cur != ep_end; ep_cur++) {
		/* Pointer to the containing device */
		ep_cur->dev = dev;

		/* BSP USBD driver EP handle */
		ep_cur->ep_hw_idx = EP0 + (ep_cur - ep_mgmt->ep_pool);
	}

	/* Reserve 1st/2nd EP contexts (BSP USBD driver EP0/EP1) for CTRL OUT/IN */
	ep_mgmt->ep_idx = 2;

	/* Reserve DMA buffer for Setup/CTRL OUT/CTRL IN, starting from 0 */
	ep_mgmt->dmabuf_pos = 0;

	/* Configure DMA buffer for Setup packet */
	base->STBUFSEG = ep_mgmt->dmabuf_pos;
	ep_mgmt->dmabuf_pos += NUMAKER_USBD_DMABUF_SIZE_SETUP;

	/* Reserve 1st EP context (BSP USBD driver EP0) for CTRL OUT */
	ep_cur = ep_mgmt->ep_pool + 0;
	ep_cur->valid = true;
	ep_cur->addr_valid = true;
	ep_cur->addr = USB_EP_GET_ADDR(0, USB_EP_DIR_OUT);
	numaker_usbd_ep_config_dmabuf(ep_cur, ep_mgmt->dmabuf_pos,
				      NUMAKER_USBD_DMABUF_SIZE_CTRLOUT);
	ep_mgmt->dmabuf_pos += NUMAKER_USBD_DMABUF_SIZE_CTRLOUT;
	ep_cur->mps_valid = true;
	ep_cur->mps = NUMAKER_USBD_DMABUF_SIZE_CTRLOUT;

	/* Reserve 2nd EP context (BSP USBD driver EP1) for CTRL IN */
	ep_cur = ep_mgmt->ep_pool + 1;
	ep_cur->valid = true;
	ep_cur->addr_valid = true;
	ep_cur->addr = USB_EP_GET_ADDR(0, USB_EP_DIR_IN);
	numaker_usbd_ep_config_dmabuf(ep_cur, ep_mgmt->dmabuf_pos, NUMAKER_USBD_DMABUF_SIZE_CTRLIN);
	ep_mgmt->dmabuf_pos += NUMAKER_USBD_DMABUF_SIZE_CTRLIN;
	ep_cur->mps_valid = true;
	ep_cur->mps = NUMAKER_USBD_DMABUF_SIZE_CTRLIN;
}

/* Find EP context by EP address */
static struct numaker_usbd_ep *numaker_usbd_ep_mgmt_find_ep(const struct device *dev,
							    const uint8_t ep)
{
	const struct numaker_usbd_config *config = dev->config;
	struct numaker_usbd_data *data = dev->data;
	struct numaker_usbd_ep_mgmt *ep_mgmt = &data->ep_mgmt;
	struct numaker_usbd_ep *ep_cur = ep_mgmt->ep_pool;
	struct numaker_usbd_ep *ep_end = ep_mgmt->ep_pool + config->num_bidir_endpoints;

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

/* Bind EP context to EP address */
static struct numaker_usbd_ep *numaker_usbd_ep_mgmt_bind_ep(const struct device *dev,
							    const uint8_t ep)
{
	struct numaker_usbd_ep *ep_cur = numaker_usbd_ep_mgmt_find_ep(dev, ep);

	if (!ep_cur) {
		ep_cur = numaker_usbd_ep_mgmt_alloc_ep(dev);

		if (!ep_cur) {
			return NULL;
		}

		/* Bind EP context to EP address */
		ep_cur->addr = ep;
		ep_cur->addr_valid = true;
	}

	/* Assert EP context bound to EP address */
	__ASSERT_NO_MSG(ep_cur->valid);
	__ASSERT_NO_MSG(ep_cur->addr_valid);
	__ASSERT_NO_MSG(ep_cur->addr == ep);

	return ep_cur;
}

/* Interrupt bottom half processing for bus reset */
static void numaker_usbd_bus_reset_bh(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	struct numaker_usbd_data *data = dev->data;
	struct numaker_usbd_ep_mgmt *ep_mgmt = &data->ep_mgmt;

	struct numaker_usbd_ep *ep_cur = ep_mgmt->ep_pool;
	struct numaker_usbd_ep *ep_end = ep_mgmt->ep_pool + config->num_bidir_endpoints;

	for (; ep_cur != ep_end; ep_cur++) {
		/* Reset EP FIFO */
		numaker_usbd_ep_fifo_reset(ep_cur);

		/* Abort EP on-going transaction and signal H/W relinquishes DMA buffer ownership */
		numaker_usbd_ep_abort(ep_cur);

		/* Reset EP to unstalled and data toggle bit to 0 */
		numaker_usbd_ep_clear_stall_n_data_toggle(ep_cur);
	}

	numaker_usbd_reset_addr(dev);
}

/* Interrupt bottom half processing for Setup/EP data transaction */
static void numaker_usbd_ep_bh(struct numaker_usbd_ep *ep_cur,
			       enum usb_dc_ep_cb_status_code status_code)
{
	const struct device *dev = ep_cur->dev;
	struct numaker_usbd_data *data = dev->data;
	struct numaker_usbd_ep_mgmt *ep_mgmt = &data->ep_mgmt;

	if (status_code == USB_DC_EP_SETUP) {
		/* Zephyr USB device stack passes Setup packet via CTRL OUT EP. */
		__ASSERT_NO_MSG(ep_cur->addr == USB_EP_GET_ADDR(0, USB_EP_DIR_OUT));

		if (numaker_usbd_ep_fifo_used(ep_cur)) {
			LOG_WRN("New Setup will override previous Control OUT data");
		}

		/* We should have reserved 1st/2nd EP contexts for CTRL OUT/IN */
		__ASSERT_NO_MSG(ep_cur->addr == USB_EP_GET_ADDR(0, USB_EP_DIR_OUT));
		__ASSERT_NO_MSG((ep_cur + 1)->addr == USB_EP_GET_ADDR(0, USB_EP_DIR_IN));

		/* Reset CTRL OUT/IN FIFO due to new Setup packet */
		numaker_usbd_ep_fifo_reset(ep_cur);
		numaker_usbd_ep_fifo_reset(ep_cur + 1);

		/* Relinquish CTRL OUT/IN DMA buffer ownership on behalf of H/W */
		numaker_usbd_ep_abort(ep_cur);
		numaker_usbd_ep_abort(ep_cur + 1);

		/* Mark new Setup packet for read */
		numaker_usbd_setup_fifo_copy_to_user(dev, (uint8_t *)&ep_mgmt->setup_packet);
		ep_mgmt->new_setup = true;
	} else if (status_code == USB_DC_EP_DATA_OUT) {
		__ASSERT_NO_MSG(USB_EP_DIR_IS_OUT(ep_cur->addr));

		/* Update EP read FIFO */
		numaker_usbd_ep_fifo_update(ep_cur);

		/* Need to clear NAK for next transaction */
		ep_cur->nak_clr = false;
	} else if (status_code == USB_DC_EP_DATA_IN) {
		__ASSERT_NO_MSG(USB_EP_DIR_IS_IN(ep_cur->addr));

		/* Update EP write FIFO */
		numaker_usbd_ep_fifo_update(ep_cur);

		/* Need to clear NAK for next transaction */
		ep_cur->nak_clr = false;
	}
}

/* Message handler for S/W reconnect */
static void numaker_usbd_msg_sw_reconn(const struct device *dev, struct numaker_usbd_msg *msg)
{
	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_SW_RECONN);

	/* S/W reconnect for error recovery */
	numaker_usbd_lock(dev);
	numaker_usbd_sw_reconnect(dev);
	numaker_usbd_unlock(dev);
}

/* Message handler for callback for usb_dc_status_code */
static void numaker_usbd_msg_cb_state(const struct device *dev, struct numaker_usbd_msg *msg)
{
	struct numaker_usbd_data *data = dev->data;

	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_CB_STATE);

	/* Interrupt bottom half processing for bus reset */
	if (msg->cb_device.status_code == USB_DC_RESET) {
		numaker_usbd_lock(dev);
		numaker_usbd_bus_reset_bh(dev);
		numaker_usbd_unlock(dev);
	}

	/* NOTE: Don't run callback with our mutex locked, or we may encounter
	 * deadlock because the Zephyr USB device stack can have its own
	 * synchronization.
	 */
	if (data->status_cb) {
		data->status_cb(msg->cb_device.status_code, NULL);
	} else {
		LOG_WRN("No status callback: status_code=%d", msg->cb_device.status_code);
	}
}

/* Message handler for callback for usb_dc_ep_cb_status_code */
static void numaker_usbd_msg_cb_ep(const struct device *dev, struct numaker_usbd_msg *msg)
{
	uint8_t ep;
	struct numaker_usbd_ep *ep_cur;

	__ASSERT_NO_MSG(msg->type == NUMAKER_USBD_MSG_TYPE_CB_EP);

	ep = msg->cb_ep.ep;

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		return;
	}

	/* Interrupt bottom half processing for EP */
	numaker_usbd_lock(dev);
	numaker_usbd_ep_bh(ep_cur, msg->cb_ep.status_code);
	numaker_usbd_unlock(dev);

	/* NOTE: Same as above, don't run callback with our mutex locked */
	if (ep_cur->cb) {
		ep_cur->cb(ep, msg->cb_ep.status_code);
	} else {
		LOG_WRN("No EP callback: ep=0x%02x, status_code=%d", ep, msg->cb_ep.status_code);
	}
}

/* Interrupt bottom half processing
 *
 * This thread is used to not run Zephyr USB device stack and callbacks in interrupt
 * context. This is because callbacks from this stack may use mutex or other kernel functions
 * which are not supported in interrupt context.
 */
static void numaker_usbd_msg_hdlr_thread_main(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = (const struct device *)arg1;
	struct numaker_usbd_data *data = dev->data;

	struct numaker_usbd_msg msg;

	__ASSERT_NO_MSG(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		if (k_msgq_get(&data->msgq, &msg, K_FOREVER)) {
			continue;
		}

		switch (msg.type) {
		case NUMAKER_USBD_MSG_TYPE_SW_RECONN:
			numaker_usbd_msg_sw_reconn(dev, &msg);
			break;

		case NUMAKER_USBD_MSG_TYPE_CB_STATE:
			numaker_usbd_msg_cb_state(dev, &msg);
			break;

		case NUMAKER_USBD_MSG_TYPE_CB_EP:
			numaker_usbd_msg_cb_ep(dev, &msg);
			break;

		default:
			__ASSERT_NO_MSG(false);
		}
	}
}

static void numaker_udbd_isr(const struct device *dev)
{
	const struct numaker_usbd_config *config = dev->config;
	struct numaker_usbd_data *data = dev->data;
	USBD_T *const base = config->base;
	struct numaker_usbd_ep_mgmt *ep_mgmt = &data->ep_mgmt;

	struct numaker_usbd_msg msg = {0};

	uint32_t volatile usbd_intsts = base->INTSTS;
	uint32_t volatile usbd_bus_state = base->ATTR;

	/* USB plug-in/unplug */
	if (usbd_intsts & USBD_INTSTS_FLDET) {
		/* Floating detect */
		base->INTSTS = USBD_INTSTS_FLDET;

		if (base->VBUSDET & USBD_VBUSDET_VBUSDET_Msk) {
			/* USB plug-in */

			/* Enable back USB/PHY */
			base->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;

			/* Message for bottom-half processing */
			msg.type = NUMAKER_USBD_MSG_TYPE_CB_STATE;
			msg.cb_device.status_code = USB_DC_CONNECTED;
			numaker_usbd_send_msg(dev, &msg);

			LOG_DBG("USB plug-in");
		} else {
			/* USB unplug */

			/* Disable USB */
			base->ATTR &= ~USBD_USB_EN;

			/* Message for bottom-half processing */
			msg.type = NUMAKER_USBD_MSG_TYPE_CB_STATE;
			msg.cb_device.status_code = USB_DC_DISCONNECTED;
			numaker_usbd_send_msg(dev, &msg);

			LOG_DBG("USB unplug");
		}
	}

	/* USB wake-up */
	if (usbd_intsts & USBD_INTSTS_WAKEUP) {
		/* Clear event flag */
		base->INTSTS = USBD_INTSTS_WAKEUP;

		LOG_DBG("USB wake-up");
	}

	/* USB reset/suspend/resume */
	if (usbd_intsts & USBD_INTSTS_BUS) {
		/* Clear event flag */
		base->INTSTS = USBD_INTSTS_BUS;

		if (usbd_bus_state & USBD_STATE_USBRST) {
			/* Bus reset */

			/* Enable back USB/PHY */
			base->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;

			/* Bus reset top half */
			numaker_usbd_bus_reset_th(dev);

			/* Message for bottom-half processing */
			msg.type = NUMAKER_USBD_MSG_TYPE_CB_STATE;
			msg.cb_device.status_code = USB_DC_RESET;
			numaker_usbd_send_msg(dev, &msg);

			LOG_DBG("USB reset");
		}
		if (usbd_bus_state & USBD_STATE_SUSPEND) {
			/* Enable USB but disable PHY */
			base->ATTR &= ~USBD_PHY_EN;

			/* Message for bottom-half processing */
			msg.type = NUMAKER_USBD_MSG_TYPE_CB_STATE;
			msg.cb_device.status_code = USB_DC_SUSPEND;
			numaker_usbd_send_msg(dev, &msg);

			LOG_DBG("USB suspend");
		}
		if (usbd_bus_state & USBD_STATE_RESUME) {
			/* Enable back USB/PHY */
			base->ATTR |= USBD_ATTR_USBEN_Msk | USBD_ATTR_PHYEN_Msk;

			/* Message for bottom-half processing */
			msg.type = NUMAKER_USBD_MSG_TYPE_CB_STATE;
			msg.cb_device.status_code = USB_DC_RESUME;
			numaker_usbd_send_msg(dev, &msg);

			LOG_DBG("USB resume");
		}
	}

	/* USB SOF */
	if (usbd_intsts & USBD_INTSTS_SOFIF_Msk) {
		/* Clear event flag */
		base->INTSTS = USBD_INTSTS_SOFIF_Msk;

		/* Message for bottom-half processing */
		msg.type = NUMAKER_USBD_MSG_TYPE_CB_STATE;
		msg.cb_device.status_code = USB_DC_SOF;
		numaker_usbd_send_msg(dev, &msg);
	}

	/* USB Setup/EP */
	if (usbd_intsts & USBD_INTSTS_USB) {
		uint32_t epintsts;

		/* Setup event */
		if (usbd_intsts & USBD_INTSTS_SETUP) {
			USBD_EP_T *ep0_base = numaker_usbd_ep_base(dev, EP0);
			USBD_EP_T *ep1_base = numaker_usbd_ep_base(dev, EP1);

			/* Clear event flag */
			base->INTSTS = USBD_INTSTS_SETUP;

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
			msg.type = NUMAKER_USBD_MSG_TYPE_CB_EP;
			msg.cb_ep.ep = USB_EP_GET_ADDR(0, USB_EP_DIR_OUT);
			msg.cb_ep.status_code = USB_DC_EP_SETUP;
			numaker_usbd_send_msg(dev, &msg);
		}

		/* EP events */
		epintsts = base->EPINTSTS;

		base->EPINTSTS = epintsts;

		while (epintsts) {
			uint32_t ep_hw_idx = u32_count_trailing_zeros(epintsts);
			USBD_EP_T *ep_base = numaker_usbd_ep_base(dev, ep_hw_idx);
			uint8_t ep_dir;
			uint8_t ep_idx;
			uint8_t ep;

			/* We don't enable INNAKEN interrupt, so as long as EP event occurs,
			 * we can just regard one data transaction has completed (ACK for
			 * CTRL/BULK/INT or no-ACK for Iso), that is, no need to check EPSTS0,
			 * EPSTS1, etc.
			 */

			/* EP direction, number, and address */
			ep_dir = ((ep_base->CFG & USBD_CFG_STATE_Msk) == USBD_CFG_EPMODE_IN)
					 ? USB_EP_DIR_IN
					 : USB_EP_DIR_OUT;
			ep_idx = (ep_base->CFG & USBD_CFG_EPNUM_Msk) >> USBD_CFG_EPNUM_Pos;
			ep = USB_EP_GET_ADDR(ep_idx, ep_dir);

			/* NOTE: See comment in usb_dc_set_address()'s implementation
			 * for safe place to change USB device address
			 */
			if (ep == USB_EP_GET_ADDR(0, USB_EP_DIR_IN)) {
				numaker_usbd_set_addr(dev);
			}

			/* NOTE: See comment on mxpld_ctrlout for why make one copy of
			 * CTRL OUT's MXPLD
			 */
			if (ep == USB_EP_GET_ADDR(0, USB_EP_DIR_OUT)) {
				struct numaker_usbd_ep *ep_ctrlout = ep_mgmt->ep_pool + 0;
				USBD_EP_T *ep_ctrlout_base =
					numaker_usbd_ep_base(dev, ep_ctrlout->ep_hw_idx);

				ep_ctrlout->mxpld_ctrlout = ep_ctrlout_base->MXPLD;
			}

			/* Message for bottom-half processing */
			msg.type = NUMAKER_USBD_MSG_TYPE_CB_EP;
			msg.cb_ep.ep = ep;
			msg.cb_ep.status_code =
				USB_EP_DIR_IS_IN(ep) ? USB_DC_EP_DATA_IN : USB_DC_EP_DATA_OUT;
			numaker_usbd_send_msg(dev, &msg);

			/* Have handled this EP and go next */
			epintsts &= ~BIT(ep_hw_idx);
		}
	}
}

/* Zephyr USB device controller API implementation */

int usb_dc_attach(void)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc;

	numaker_usbd_lock(dev);

	/* Initialize USB DC H/W */
	rc = numaker_usbd_hw_setup(dev);
	if (rc < 0) {
		LOG_ERR("Set up H/W");
		goto cleanup;
	}

	/* USB device address defaults to 0 */
	numaker_usbd_reset_addr(dev);

	/* Initialize all EPs */
	numaker_usbd_ep_mgmt_init(dev);

	/* S/W connect */
	numaker_usbd_sw_connect(dev);

	LOG_DBG("attached");

cleanup:

	if (rc < 0) {
		usb_dc_detach();
	}

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_detach(void)
{
	const struct device *dev = numaker_usbd_device_get();
	struct numaker_usbd_data *data = dev->data;

	LOG_DBG("detached");

	numaker_usbd_lock(dev);

	/* S/W disconnect */
	numaker_usbd_sw_disconnect(dev);

	/* Uninitialize USB DC H/W */
	numaker_usbd_hw_shutdown(numaker_usbd_device_get());

	/* Purge message queue */
	k_msgq_purge(&data->msgq);

	numaker_usbd_unlock(dev);

	return 0;
}

int usb_dc_reset(void)
{
	const struct device *dev = numaker_usbd_device_get();

	LOG_DBG("usb_dc_reset");

	numaker_usbd_lock(dev);

	usb_dc_detach();
	usb_dc_attach();

	numaker_usbd_unlock(dev);

	return 0;
}

int usb_dc_set_address(const uint8_t addr)
{
	const struct device *dev = numaker_usbd_device_get();
	struct numaker_usbd_data *data = dev->data;

	LOG_DBG("USB device address=%u (0x%02x)", addr, addr);

	numaker_usbd_lock(dev);

	/* NOTE: Timing for configuring USB device address into H/W is critical. It must be done
	 * in-between SET_ADDRESS control transfer and next transfer. For this, it is done in
	 * IN ACK ISR of SET_ADDRESS control transfer.
	 */
	data->addr = addr;

	numaker_usbd_unlock(dev);

	return 0;
}

void usb_dc_set_status_callback(const usb_dc_status_callback cb)
{
	const struct device *dev = numaker_usbd_device_get();
	struct numaker_usbd_data *data = dev->data;

	numaker_usbd_lock(dev);

	data->status_cb = cb;

	numaker_usbd_unlock(dev);
}

int usb_dc_ep_check_cap(const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	const struct device *dev = numaker_usbd_device_get();
	const struct numaker_usbd_config *config = dev->config;
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;

	numaker_usbd_lock(dev);

	/* For safe, require EP number for control transfer to be 0 */
	if ((ep_cfg->ep_type == USB_DC_EP_CONTROL) && USB_EP_GET_IDX(ep_cfg->ep_addr) != 0) {
		LOG_ERR("EP number for control transfer must be 0");
		rc = -ENOTSUP;
		goto cleanup;
	}

	/* Some soc series don't allow ISO IN/OUT to be assigned the same EP number.
	 * This is addressed by limiting all OUT/IN EP addresses in top/bottom halves,
	 * except CTRL OUT/IN.
	 */
	if (config->disallow_iso_inout_same && ep_cfg->ep_type != USB_DC_EP_CONTROL) {
		/* Limit all OUT EP addresses in top-half, except CTRL OUT */
		if (USB_EP_DIR_IS_OUT(ep_cfg->ep_addr) && USB_EP_GET_IDX(ep_cfg->ep_addr) >= 8) {
			LOG_DBG("Support only ISO OUT EP address 0x01~0x07: 0x%02x",
				ep_cfg->ep_addr);
			rc = -ENOTSUP;
			goto cleanup;
		}

		/* Limit all IN EP addresses in bottom-half , except CTRL IN */
		if (USB_EP_DIR_IS_IN(ep_cfg->ep_addr) && USB_EP_GET_IDX(ep_cfg->ep_addr) < 8) {
			LOG_DBG("Support only ISO IN EP address 0x88~0x8F: 0x%02x",
				ep_cfg->ep_addr);
			rc = -ENOTSUP;
			goto cleanup;
		}
	}

	/* To respect this capability check, pre-bind EP context to EP address,
	 * and pre-determined its type
	 */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep_cfg->ep_addr);
	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep_cfg->ep_addr);
		rc = -ENOMEM;
		goto cleanup;
	}

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_set_callback(const uint8_t ep, const usb_dc_ep_callback cb)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	ep_cur->cb = cb;

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_configure(const struct usb_dc_ep_cfg_data *const ep_cfg)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	uint32_t dmabuf_base;
	uint32_t dmabuf_size;
	struct numaker_usbd_ep *ep_cur;

	LOG_DBG("EP=0x%02x, MPS=%d, Type=%d", ep_cfg->ep_addr, ep_cfg->ep_mps, ep_cfg->ep_type);

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep_cfg->ep_addr);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep_cfg->ep_addr);
		rc = -ENOMEM;
		goto cleanup;
	}

	/* Configure EP DMA buffer */
	if (!ep_cur->dmabuf_valid || ep_cur->dmabuf_size < ep_cfg->ep_mps) {
		/* Allocate DMA buffer */
		rc = numaker_usbd_ep_mgmt_alloc_dmabuf(dev, ep_cfg->ep_mps, &dmabuf_base,
						       &dmabuf_size);
		if (rc < 0) {
			LOG_ERR("Allocate DMA buffer failed");
			goto cleanup;
		}

		/* Configure EP DMA buffer */
		numaker_usbd_ep_config_dmabuf(ep_cur, dmabuf_base, dmabuf_size);
	}

	/* Configure EP majorly */
	numaker_usbd_ep_config_major(ep_cur, ep_cfg);

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_set_stall(const uint8_t ep)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;

	LOG_DBG("Set stall: ep=0x%02x", ep);

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	/* Set EP to stalled */
	numaker_usbd_ep_set_stall(ep_cur);

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_clear_stall(const uint8_t ep)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;

	LOG_DBG("Clear stall: ep=0x%02x", ep);

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	/* Reset EP to unstalled and data toggle bit to 0 */
	numaker_usbd_ep_clear_stall_n_data_toggle(ep_cur);

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_is_stalled(const uint8_t ep, uint8_t *const stalled)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;

	if (!stalled) {
		return -EINVAL;
	}

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	*stalled = numaker_usbd_ep_is_stalled(ep_cur);

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_halt(const uint8_t ep)
{
	return usb_dc_ep_set_stall(ep);
}

int usb_dc_ep_enable(const uint8_t ep)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;

	LOG_DBG("Enable: ep=0x%02x", ep);

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	numaker_usbd_ep_enable(ep_cur);

	/* Trigger OUT transaction manually, or H/W will continue to reply NAK because
	 * Zephyr USB device stack is unclear on kicking off by invoking usb_dc_ep_read()
	 * or friends. We needn't do this for CTRL OUT because Setup sequence will involve
	 * this.
	 */
	if (USB_EP_DIR_IS_OUT(ep) && USB_EP_GET_IDX(ep) != 0) {
		rc = usb_dc_ep_read_continue(ep);
		if (rc < 0) {
			goto cleanup;
		}
	}

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_disable(const uint8_t ep)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;

	LOG_DBG("Disable: ep=0x%02x", ep);

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	numaker_usbd_ep_disable(ep_cur);

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_flush(const uint8_t ep)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;

	LOG_DBG("ep=0x%02x", ep);

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	numaker_usbd_ep_fifo_reset(ep_cur);

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_write(const uint8_t ep, const uint8_t *const data_buf, const uint32_t data_len,
		    uint32_t *const ret_bytes)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;
	uint32_t data_len_act;

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	if (!USB_EP_DIR_IS_IN(ep)) {
		LOG_ERR("Invalid EP address 0x%02x for write", ep);
		rc = -EINVAL;
		goto cleanup;
	}

	/* For USBD, avoid duplicate NAK clear */
	if (ep_cur->nak_clr) {
		LOG_WRN("ep 0x%02x busy", ep);
		rc = -EAGAIN;
		goto cleanup;
	}

	/* For one-shot implementation, don't trigger next DATA IN with write FIFO not empty. */
	if (numaker_usbd_ep_fifo_used(ep_cur)) {
		LOG_WRN("ep 0x%02x: Write FIFO not empty for one-shot implementation", ep);
		rc = -EAGAIN;
		goto cleanup;
	}

	/* NOTE: Null data or zero data length are valid, used for ZLP */
	if (data_buf && data_len) {
		data_len_act = data_len;
		rc = numaker_usbd_ep_fifo_copy_from_user(ep_cur, data_buf, &data_len_act);
		if (rc < 0) {
			LOG_ERR("Copy to FIFO from user buffer");
			goto cleanup;
		}
	} else {
		data_len_act = 0;
	}

	/* Now H/W actually owns EP DMA buffer */
	numaker_usbd_ep_trigger(ep_cur, data_len_act);

	/* NOTE: For one-shot implementation, at most MPS size can be written, though,
	 * null 'ret_bytes' requires all data written.
	 */
	if (ret_bytes) {
		*ret_bytes = data_len_act;
	} else if (data_len_act != data_len) {
		LOG_ERR("Expected write all %d bytes, but actual %d bytes written", data_len,
			data_len_act);
		rc = -EIO;
		goto cleanup;
	}

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_read(const uint8_t ep, uint8_t *const data, const uint32_t max_data_len,
		   uint32_t *const read_bytes)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;

	numaker_usbd_lock(dev);

	rc = usb_dc_ep_read_wait(ep, data, max_data_len, read_bytes);
	if (rc < 0) {
		goto cleanup;
	}

	rc = usb_dc_ep_read_continue(ep);
	if (rc < 0) {
		goto cleanup;
	}

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_read_wait(uint8_t ep, uint8_t *data_buf, uint32_t max_data_len, uint32_t *read_bytes)
{
	const struct device *dev = numaker_usbd_device_get();
	struct numaker_usbd_data *data = dev->data;
	int rc = 0;
	struct numaker_usbd_ep_mgmt *ep_mgmt = &data->ep_mgmt;
	struct numaker_usbd_ep *ep_cur;
	uint32_t data_len_act = 0;

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	if (!USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Invalid EP address 0x%02x for read", ep);
		rc = -EINVAL;
		goto cleanup;
	}

	/* Special handling for USB_CONTROL_EP_OUT on Setup packet */
	if (ep == USB_CONTROL_EP_OUT && ep_mgmt->new_setup) {
		if (!data_buf || max_data_len != 8) {
			LOG_ERR("Invalid parameter for reading Setup packet");
			rc = -EINVAL;
			goto cleanup;
		}

		memcpy(data_buf, &ep_mgmt->setup_packet, 8);
		ep_mgmt->new_setup = false;

		if (read_bytes) {
			*read_bytes = 8;
		}

		goto cleanup;
	}

	/* For one-shot implementation, don't read FIFO with EP busy. */
	if (ep_cur->nak_clr) {
		LOG_WRN("ep 0x%02x busy", ep);
		rc = -EAGAIN;
		goto cleanup;
	}

	/* NOTE: Null data and zero data length is valid, used for returning number of
	 * available bytes for read
	 */
	if (data_buf) {
		data_len_act = max_data_len;
		rc = numaker_usbd_ep_fifo_copy_to_user(ep_cur, data_buf, &data_len_act);
		if (rc < 0) {
			LOG_ERR("Copy from FIFO to user buffer");
			goto cleanup;
		}

		if (read_bytes) {
			*read_bytes = data_len_act;
		}
	} else if (max_data_len) {
		LOG_ERR("Null data but non-zero data length");
		rc = -EINVAL;
		goto cleanup;
	} else {
		if (read_bytes) {
			*read_bytes = numaker_usbd_ep_fifo_used(ep_cur);
		}
	}

	/* Suppress further USB_DC_EP_DATA_OUT events by replying NAK or disabling interrupt
	 *
	 * For USBD, further control is unnecessary because NAK is automatically replied until
	 * next USBD_SET_PAYLOAD_LEN().
	 */

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_read_continue(uint8_t ep)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	if (!USB_EP_DIR_IS_OUT(ep)) {
		LOG_ERR("Invalid EP address 0x%02x for read", ep);
		rc = -EINVAL;
		goto cleanup;
	}

	/* Avoid duplicate NAK clear */
	if (ep_cur->nak_clr) {
		rc = 0;
		goto cleanup;
	}

	/* For one-shot implementation, don't trigger next DATA OUT, or overwrite. */
	if (numaker_usbd_ep_fifo_used(ep_cur)) {
		goto cleanup;
	}

	__ASSERT_NO_MSG(ep_cur->mps_valid);
	numaker_usbd_ep_trigger(ep_cur, ep_cur->mps);

cleanup:

	numaker_usbd_unlock(dev);

	return rc;
}

int usb_dc_ep_mps(const uint8_t ep)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;
	struct numaker_usbd_ep *ep_cur;
	uint16_t ep_mps = 0;

	numaker_usbd_lock(dev);

	/* Bind EP context to EP address */
	ep_cur = numaker_usbd_ep_mgmt_bind_ep(dev, ep);

	if (!ep_cur) {
		LOG_ERR("Bind EP context: ep=0x%02x", ep);
		rc = -ENOMEM;
		goto cleanup;
	}

	__ASSERT_NO_MSG(ep_cur->mps_valid);
	ep_mps = ep_cur->mps;

cleanup:

	numaker_usbd_unlock(dev);

	return rc == 0 ? ep_mps : rc;
}

int usb_dc_wakeup_request(void)
{
	const struct device *dev = numaker_usbd_device_get();
	int rc = 0;

	LOG_DBG("Remote wakeup");

	numaker_usbd_lock(dev);

	numaker_usbd_remote_wakeup(dev);

	numaker_usbd_unlock(dev);

	return rc;
}

static int numaker_udbd_init(const struct device *dev)
{
	struct numaker_usbd_data *data = dev->data;
	int rc = 0;

	/* Initialize all fields to zero */
	memset(data, 0x00, sizeof(*data));

	k_mutex_init(&data->sync_mutex);

	/* Set up interrupt top/bottom halves processing */

	k_msgq_init(&data->msgq, (char *)data->msgq_buf, sizeof(struct numaker_usbd_msg),
		    CONFIG_USB_DC_NUMAKER_MSG_QUEUE_SIZE);

	k_thread_create(&data->msg_hdlr_thread, data->msg_hdlr_thread_stack,
			CONFIG_USB_DC_NUMAKER_MSG_HANDLER_THREAD_STACK_SIZE,
			numaker_usbd_msg_hdlr_thread_main, (void *)dev, NULL, NULL, K_PRIO_COOP(2),
			0, K_NO_WAIT);

	k_thread_name_set(&data->msg_hdlr_thread, "numaker_usbd");

	return rc;
}

#define USB_DC_NUMAKER_INIT(inst)                                                                  \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static void numaker_usbd_irq_config_func_##inst(const struct device *dev)                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), numaker_udbd_isr,     \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
                                                                                                   \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	static void numaker_uusbd_irq_unconfig_func_##inst(const struct device *dev)               \
	{                                                                                          \
		irq_disable(DT_INST_IRQN(inst));                                                   \
	}                                                                                          \
                                                                                                   \
	static const struct numaker_usbd_config numaker_usbd_config_##inst = {                     \
		.base = (USBD_T *)DT_INST_REG_ADDR(inst),                                          \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),                       \
		.clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),                                \
		.clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),                               \
		.clkctrl_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(inst))),                \
		.irq_config_func = numaker_usbd_irq_config_func_##inst,                            \
		.irq_unconfig_func = numaker_uusbd_irq_unconfig_func_##inst,                       \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.num_bidir_endpoints = DT_INST_PROP(inst, num_bidir_endpoints),                    \
		.dmabuf_size = DT_INST_PROP(inst, dma_buffer_size),                                \
		.disallow_iso_inout_same = DT_INST_PROP(inst, disallow_iso_in_out_same_number),    \
	};                                                                                         \
                                                                                                   \
	static struct numaker_usbd_data numaker_usbd_data_##inst;                                  \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP(inst, num_bidir_endpoints) <= NUMAKER_USBD_EP_MAXNUM,            \
		     "num_bidir_endpoints exceeds support limit by USBD driver");                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, numaker_udbd_init, NULL, &numaker_usbd_data_##inst,            \
			      &numaker_usbd_config_##inst, POST_KERNEL,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

USB_DC_NUMAKER_INIT(0);

/* Get USB DC device context instance 0 */
static inline const struct device *numaker_usbd_device_get(void)
{
	return DEVICE_DT_INST_GET(0);
}
