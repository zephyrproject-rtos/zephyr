/*
 * Copyright (c) 2026 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_usbhs

#include "udc_common.h"

#include <string.h>

#include <soc.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/irq.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/barrier.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_sam_usbhs, CONFIG_UDC_DRIVER_LOG_LEVEL);

/*
 * FIFO base address for endpoint data access.
 * Each endpoint has a 32KB (0x8000) address range for FIFO access.
 */
#define SAM_USBHS_FIFO_BASE	DT_INST_REG_ADDR_BY_NAME(0, fifo)
#define USB_FIFO_EP_OFFSET	0x8000U

/*
 * The new Atmel DFP headers provide mode-specific interrupt register field
 * definitions. Map the existing generic definitions to these.
 */
#ifndef USBHS_DEVEPTISR_CTRL_RXSTPI
#define USBHS_DEVEPTISR_CTRL_RXSTPI	USBHS_DEVEPTISR_RXSTPI
#endif
#ifndef USBHS_DEVEPTICR_CTRL_RXSTPIC
#define USBHS_DEVEPTICR_CTRL_RXSTPIC	USBHS_DEVEPTICR_RXSTPIC
#endif
#ifndef USBHS_DEVEPTIMR_CTRL_STALLRQ
#define USBHS_DEVEPTIMR_CTRL_STALLRQ	USBHS_DEVEPTIMR_STALLRQ
#endif
#ifndef USBHS_DEVEPTIER_CTRL_RXSTPES
#define USBHS_DEVEPTIER_CTRL_RXSTPES	USBHS_DEVEPTIER_RXSTPES
#endif
#ifndef USBHS_DEVEPTIER_CTRL_STALLRQS
#define USBHS_DEVEPTIER_CTRL_STALLRQS	USBHS_DEVEPTIER_STALLRQS
#endif
#ifndef USBHS_DEVEPTIDR_CTRL_STALLRQC
#define USBHS_DEVEPTIDR_CTRL_STALLRQC	USBHS_DEVEPTIDR_STALLRQC
#endif

/*
 * Endpoint count macros.
 * SAM USBHS has direction-constrained endpoints:
 *   - EP0: Bidirectional control
 *   - Odd EPs (1,3,5,7,9): IN only
 *   - Even EPs (2,4,6,8): OUT only
 */
#define NUM_OF_IN_EPS		DT_INST_PROP(0, num_in_endpoints)
#define NUM_OF_OUT_EPS		DT_INST_PROP(0, num_out_endpoints)
/* Total HW endpoints = IN + OUT - 1 (EP0 counted in both) */
#define NUM_OF_EP_MAX		(NUM_OF_IN_EPS + NUM_OF_OUT_EPS - 1)

/*
 * High-Speed support macros.
 *
 * Check if maximum-speed property limits to full-speed.
 * Enum index: 0=low-speed, 1=full-speed, 2=high-speed, 3=super-speed
 *
 * High-Speed is enabled when:
 *   1. CONFIG_UDC_DRIVER_HIGH_SPEED_SUPPORT_ENABLED is set, AND
 *   2. maximum-speed is NOT set to "full-speed"
 */
#define UDC_SAM_USBHS_DT_FS_ONLY(node)					\
	UTIL_AND(DT_NODE_HAS_PROP(node, maximum_speed),			\
		 IS_EQ(DT_ENUM_IDX(node, maximum_speed), 1))

#define UDC_SAM_USBHS_HS_ENABLED(node)					\
	COND_CODE_1(CONFIG_UDC_DRIVER_HIGH_SPEED_SUPPORT_ENABLED,	\
		    (UTIL_NOT(UDC_SAM_USBHS_DT_FS_ONLY(node))),		\
		    (0))

/*
 * Returns max packet size for endpoints.
 * Hardware supports the maximal value allowed by the USB Specification:
 * 1024 bytes in High-Speed, 1023 bytes in Full-Speed (for isochronous)
 */
#define UDC_SAM_USBHS_EP_MPS(node)					\
	(UDC_SAM_USBHS_HS_ENABLED(node) ? 1024U : 1023U)

/* Convenience macros for instance 0 */
#define SAM_USBHS_HS_ENABLED	UDC_SAM_USBHS_HS_ENABLED(DT_DRV_INST(0))
#define SAM_USBHS_EP_MPS	UDC_SAM_USBHS_EP_MPS(DT_DRV_INST(0))

/*
 * ISR Debug Infrastructure
 * Tracks register state changes and detects ISR loops
 */
#if defined(CONFIG_UDC_DRIVER_LOG_LEVEL_DBG)

/* Device-level register state tracking */
struct sam_usbhs_dev_dbg_state {
	uint32_t devctrl;
	uint32_t devisr;
	uint32_t devimr;
	uint32_t ep0isr;  /* EP0 status to distinguish RXSTPI vs TXINI */
	uint8_t repeat_count;  /* Count consecutive identical states */
};

/* Per-endpoint register state tracking */
struct sam_usbhs_ep_dbg_state {
	uint32_t eptisr;
	uint32_t eptimr;
	uint32_t eptcfg;
	uint8_t repeat_count;  /* Count consecutive identical states */
};

static struct sam_usbhs_dev_dbg_state dev_dbg_state;
static struct sam_usbhs_ep_dbg_state ep_dbg_state[NUM_OF_EP_MAX];

static void sam_usbhs_isr_dbg_dev(Usbhs *base, uint32_t sr)
{
	uint32_t devctrl = base->USBHS_DEVCTRL;
	uint32_t devimr = base->USBHS_DEVIMR;
	uint32_t ep0isr = 0;

	/*
	 * When EP0 interrupt is pending, also track EP0's status register.
	 * This distinguishes different EP0 events (RXSTPI vs TXINI) that
	 * produce the same device-level sr, preventing false loop detection.
	 */
	if (sr & USBHS_DEVISR_PEP_0) {
		ep0isr = base->USBHS_DEVEPTISR[0];
	}

	/* Check if device registers changed (including EP0 status) */
	if (devctrl != dev_dbg_state.devctrl ||
	    sr != dev_dbg_state.devisr ||
	    devimr != dev_dbg_state.devimr ||
	    ep0isr != dev_dbg_state.ep0isr) {
		LOG_DBG("DEV: CTRL=0x%08x ISR=0x%08x IMR=0x%08x",
			devctrl, sr, devimr);

		/* State changed - update and reset repeat counter */
		dev_dbg_state.devctrl = devctrl;
		dev_dbg_state.devisr = sr;
		dev_dbg_state.devimr = devimr;
		dev_dbg_state.ep0isr = ep0isr;
		dev_dbg_state.repeat_count = 0;
	} else {
		/*
		 * Same state seen again. Increment counter but only warn
		 * after 3+ consecutive identical states. This allows for
		 * legitimate multi-packet transfers (2 packets = 2 identical
		 * states) while still catching real infinite loops.
		 */
		dev_dbg_state.repeat_count++;
		if (dev_dbg_state.repeat_count == 2) {
			LOG_WRN("DEV LOOP: CTRL=0x%08x ISR=0x%08x IMR=0x%08x",
				devctrl, sr, devimr);
		}
	}
}

static void sam_usbhs_isr_dbg_ep(Usbhs *base, uint8_t ep_idx)
{
	uint32_t eptisr = base->USBHS_DEVEPTISR[ep_idx];
	uint32_t eptimr = base->USBHS_DEVEPTIMR[ep_idx];
	uint32_t eptcfg = base->USBHS_DEVEPTCFG[ep_idx];

	/* Check if endpoint registers changed */
	if (eptisr != ep_dbg_state[ep_idx].eptisr ||
	    eptimr != ep_dbg_state[ep_idx].eptimr ||
	    eptcfg != ep_dbg_state[ep_idx].eptcfg) {
		LOG_DBG("EP%d: ISR=0x%08x IMR=0x%08x CFG=0x%08x",
			ep_idx, eptisr, eptimr, eptcfg);

		/* State changed - update and reset repeat counter */
		ep_dbg_state[ep_idx].eptisr = eptisr;
		ep_dbg_state[ep_idx].eptimr = eptimr;
		ep_dbg_state[ep_idx].eptcfg = eptcfg;
		ep_dbg_state[ep_idx].repeat_count = 0;
	} else {
		/*
		 * Same state seen again. Only warn after 3+ consecutive
		 * identical states to allow for multi-packet transfers.
		 */
		ep_dbg_state[ep_idx].repeat_count++;
		if (ep_dbg_state[ep_idx].repeat_count == 2) {
			LOG_WRN("EP%d LOOP: ISR=0x%08x IMR=0x%08x CFG=0x%08x",
				ep_idx, eptisr, eptimr, eptcfg);
		}
	}
}

#define SAM_USBHS_ISR_DBG_DEV(base, sr) sam_usbhs_isr_dbg_dev(base, sr)
#define SAM_USBHS_ISR_DBG_EP(base, ep_idx) sam_usbhs_isr_dbg_ep(base, ep_idx)

#else /* !CONFIG_UDC_DRIVER_LOG_LEVEL_DBG */

#define SAM_USBHS_ISR_DBG_DEV(base, sr)
#define SAM_USBHS_ISR_DBG_EP(base, ep_idx)

#endif /* CONFIG_UDC_DRIVER_LOG_LEVEL_DBG */

#define USBHS_EPSIZE_8		0
#define USBHS_EPSIZE_16		1
#define USBHS_EPSIZE_32		2
#define USBHS_EPSIZE_64		3
#define USBHS_EPSIZE_128	4
#define USBHS_EPSIZE_256	5
#define USBHS_EPSIZE_512	6
#define USBHS_EPSIZE_1024	7

#define USBHS_EPTYPE_CTRL	0
#define USBHS_EPTYPE_ISO	1
#define USBHS_EPTYPE_BULK	2
#define USBHS_EPTYPE_INT	3

#define USBHS_EPBK_1_BANK	0
#define USBHS_EPBK_2_BANK	1
#define USBHS_EPBK_3_BANK	2

enum sam_usbhs_event_type {
	SAM_USBHS_EVT_SETUP,
	SAM_USBHS_EVT_XFER_NEW,
	SAM_USBHS_EVT_XFER_FINISHED,
	SAM_USBHS_EVT_OUT_PENDING,
};

struct udc_sam_usbhs_config {
	Usbhs *base;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*make_thread)(const struct device *dev);
	const struct pinctrl_dev_config *pcfg;
	const struct atmel_sam_pmc_config clock_cfg;
	size_t num_of_in_eps;
	size_t num_of_out_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
};

struct udc_sam_usbhs_data {
	struct k_thread thread_data;
	struct k_event events;
	atomic_t xfer_new;
	atomic_t xfer_finished;
	atomic_t out_pending;
	atomic_t setup_pending;
	uint8_t ctrl_out_buf[64] __aligned(4);
	uint8_t setup[8];
};

/*
 * USBHS hardware endpoint mapping.
 *
 * SAM USBHS routes USB packets based on the endpoint NUMBER in the USB token,
 * NOT based on any internal mapping. This means:
 *   - USB EP0 packets -> HW EP0
 *   - USB EP1 packets -> HW EP1
 *   - USB EP2 packets -> HW EP2
 *   - etc.
 *
 * Each HW endpoint (except EP0) can only be configured as IN OR OUT, not both.
 * This means you CANNOT use both USB addresses 0x01 (EP1 OUT) and 0x81 (EP1 IN)
 * simultaneously - they both route to HW EP1 which can only be one direction.
 *
 * To avoid conflicts, the driver registers endpoints such that each USB endpoint
 * number is only used in one direction:
 *   - EP0: Control (bidirectional - hardware handles this specially)
 *   - EP1, EP3, EP5, EP7, EP9: IN only (0x81, 0x83, 0x85, 0x87, 0x89)
 *   - EP2, EP4, EP6, EP8: OUT only (0x02, 0x04, 0x06, 0x08)
 *
 * This gives 5 IN endpoints and 4 OUT endpoints beyond EP0, which is sufficient
 * for most USB class implementations.
 */
static inline uint8_t ep_addr_to_hw_ep(uint8_t ep_addr)
{
	/* Direct mapping: USB endpoint number = HW endpoint index */
	return USB_EP_GET_IDX(ep_addr);
}

/*
 * Reverse mapping: hardware endpoint index to logical endpoint address.
 * Used in ISR to get the correct endpoint configuration.
 */
static inline uint8_t hw_ep_to_ep_addr(uint8_t hw_ep, bool is_in)
{
	if (hw_ep == 0) {
		return is_in ? USB_EP_DIR_IN : 0;
	}

	/* Direct mapping: HW EP index = USB endpoint number */
	return (is_in ? USB_EP_DIR_IN : 0) | hw_ep;
}

static inline int ep_to_bit(const uint8_t ep)
{
	uint8_t hw_ep = ep_addr_to_hw_ep(ep);

	if (USB_EP_DIR_IS_IN(ep)) {
		return 16U + hw_ep;
	}

	return hw_ep;
}

static inline uint8_t bit_to_ep(uint32_t *const bitmap)
{
	unsigned int bit;
	uint8_t hw_ep;
	bool is_in;

	__ASSERT_NO_MSG(bitmap && *bitmap);

	bit = find_lsb_set(*bitmap) - 1;
	*bitmap &= ~BIT(bit);

	if (bit >= 16U) {
		hw_ep = bit - 16U;
		is_in = true;
	} else {
		hw_ep = bit;
		is_in = false;
	}

	return hw_ep_to_ep_addr(hw_ep, is_in);
}

/* Forward declaration - defined after ISR handlers */
static int sam_usbhs_process_pending_out(const struct device *dev,
					 const uint8_t ep_addr);

static inline volatile uint8_t *sam_usbhs_ep_fifo(const uint8_t ep_idx)
{
	return (volatile uint8_t *)(SAM_USBHS_FIFO_BASE + (ep_idx * USB_FIFO_EP_OFFSET));
}

static void sam_usbhs_enable_clock(void)
{
	/* Enable 480 MHz UPLL */
	PMC->CKGR_UCKR |= CKGR_UCKR_UPLLEN;

	while (!(PMC->PMC_SR & PMC_SR_LOCKU)) {
		k_yield();
	}

	LOG_DBG("USB PLL locked");

	if (!SAM_USBHS_HS_ENABLED) {
		/*
		 * Low-Power mode (Full-Speed only) requires a 48 MHz clock
		 * instead of the 480 MHz UPLL. Configure USB_48M clock as
		 * UPLLCK/10.
		 */
		PMC->PMC_USB = PMC_USB_USBDIV(9) | PMC_USB_USBS;
		PMC->PMC_SCER |= PMC_SCER_USBCLK;

		LOG_DBG("USB 48MHz clock enabled for Full-Speed mode");
	}
}

static void sam_usbhs_disable_clock(void)
{
	if (!SAM_USBHS_HS_ENABLED) {
		/* Disable USB_48M clock */
		PMC->PMC_SCER &= ~PMC_SCER_USBCLK;
	}

	PMC->CKGR_UCKR &= ~CKGR_UCKR_UPLLEN;

	LOG_DBG("USB PLL disabled");
}

static void sam_usbhs_ep_reset(const struct device *dev, const uint8_t ep_idx)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;

	base->USBHS_DEVEPT |= BIT(USBHS_DEVEPT_EPRST0_Pos + ep_idx);
	base->USBHS_DEVEPT &= ~BIT(USBHS_DEVEPT_EPRST0_Pos + ep_idx);
	barrier_dsync_fence_full();
}

static bool sam_usbhs_ep_is_configured(const struct device *dev,
				       const uint8_t ep_idx)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;

	return (base->USBHS_DEVEPTISR[ep_idx] & USBHS_DEVEPTISR_CFGOK) != 0;
}

static int sam_usbhs_prep_out(const struct device *dev,
			      struct net_buf *const buf,
			      struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *base = config->base;
	uint8_t ep_idx = ep_addr_to_hw_ep(ep_cfg->addr);
	uint32_t isr = base->USBHS_DEVEPTISR[ep_idx];
	uint32_t nbusybk = (isr & USBHS_DEVEPTISR_NBUSYBK_Msk) >>
			   USBHS_DEVEPTISR_NBUSYBK_Pos;
	uint32_t dtseq = (isr & USBHS_DEVEPTISR_DTSEQ_Msk) >>
			 USBHS_DEVEPTISR_DTSEQ_Pos;

	ARG_UNUSED(buf);

	LOG_DBG("Prepare OUT ep 0x%02x (hw=%d) ISR=0x%08x IMR=0x%08x "
		"CFG=0x%08x NBUSYBK=%d DTSEQ=%d",
		ep_cfg->addr, ep_idx, isr,
		base->USBHS_DEVEPTIMR[ep_idx],
		base->USBHS_DEVEPTCFG[ep_idx],
		nbusybk, dtseq);

	/*
	 * Clear FIFOCON to release any previously held bank and make it
	 * available for the host to write. For EP0 this is handled differently
	 * (single bank, no FIFOCON management needed).
	 */
	if (ep_idx != 0) {
		base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_FIFOCONC;
	}

	/*
	 * Ensure RXOUTE interrupt is enabled. This is necessary because:
	 * - The interrupt may have been disabled by the ISR when deferring
	 * - For multi-packet transfers, we need to keep receiving
	 */
	base->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_RXOUTES;

	LOG_DBG("After prep OUT ep 0x%02x IMR=0x%08x",
		ep_cfg->addr, base->USBHS_DEVEPTIMR[ep_idx]);

	return 0;
}

static int sam_usbhs_prep_in(const struct device *dev,
			     struct net_buf *const buf,
			     struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	uint8_t ep_idx = ep_addr_to_hw_ep(ep_cfg->addr);
	volatile uint8_t *fifo = sam_usbhs_ep_fifo(ep_idx);
	uint32_t len;

	len = MIN(buf->len, udc_mps_ep_size(ep_cfg));
	LOG_DBG("Prepare IN ep 0x%02x length %u (total %u)", ep_cfg->addr, len, buf->len);

	for (uint32_t i = 0; i < len; i++) {
		fifo[i] = buf->data[i];
	}
	barrier_dsync_fence_full();

	/*
	 * Clear the TXINI interrupt flag and enable the interrupt.
	 * The interrupt was disabled by the ISR when the previous transfer
	 * completed, so we must re-enable it for every new transfer.
	 */
	base->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_TXINIC;
	base->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_TXINES;

	if (ep_idx != 0) {
		/* Clear FIFOCON to trigger the IN transaction */
		base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_FIFOCONC;
	}

	return 0;
}

static int sam_usbhs_ctrl_feed_dout(const struct device *dev,
				    const size_t length)
{
	struct udc_ep_config *const ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	udc_buf_put(ep_cfg, buf);

	return sam_usbhs_prep_out(dev, buf, ep_cfg);
}

static void drop_control_transfers(const struct device *dev)
{
	struct net_buf *buf;

	buf = udc_buf_get_all(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT));
	if (buf != NULL) {
		net_buf_unref(buf);
	}

	buf = udc_buf_get_all(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN));
	if (buf != NULL) {
		net_buf_unref(buf);
	}
}

static int sam_usbhs_handle_evt_setup(const struct device *dev)
{
	struct udc_sam_usbhs_data *const priv = udc_get_private(dev);
	struct net_buf *buf;
	int err;

	drop_control_transfers(dev);

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, priv->setup, sizeof(priv->setup));
	udc_ep_buf_set_setup(buf);

	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		LOG_DBG("s:%p|feed for -out-", (void *)buf);

		err = sam_usbhs_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			udc_submit_ep_event(dev, buf, err);
		} else {
			return err;
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		LOG_DBG("s:%p|feed for -in-status", (void *)buf);
		err = udc_ctrl_submit_s_in_status(dev);
	} else {
		LOG_DBG("s:%p|no data", (void *)buf);
		err = udc_ctrl_submit_s_status(dev);
	}

	return err;
}

static int sam_usbhs_handle_evt_din(const struct device *dev,
				    struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;

	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep_cfg->addr);
		return -ENOBUFS;
	}

	udc_ep_set_busy(ep_cfg, false);

	if (ep_cfg->addr == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) ||
		    udc_ctrl_stage_is_no_data(dev)) {
			udc_ctrl_submit_status(dev, buf);
		}

		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {
			int err;

			net_buf_unref(buf);

			err = sam_usbhs_ctrl_feed_dout(dev, 0);
			if (err == -ENOMEM) {
				udc_submit_ep_event(dev, buf, err);
			} else {
				return err;
			}
		}

		return 0;
	}

	return udc_submit_ep_event(dev, buf, 0);
}

static int sam_usbhs_handle_evt_dout(const struct device *dev,
				     struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;
	int err = 0;

	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for OUT ep 0x%02x", ep_cfg->addr);
		return -ENODATA;
	}

	udc_ep_set_busy(ep_cfg, false);

	if (ep_cfg->addr == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev)) {
			LOG_DBG("dout:%p|status, feed >s", (void *)buf);
			udc_ctrl_submit_status(dev, buf);
		}

		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_in(dev)) {
			err = udc_ctrl_submit_s_out_status(dev, buf);
		}
	} else {
		err = udc_submit_ep_event(dev, buf, 0);
	}

	return err;
}

static void sam_usbhs_handle_xfer_next(const struct device *dev,
				       struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;
	int err;

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		return;
	}

	if (USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
		err = sam_usbhs_prep_out(dev, buf, ep_cfg);
	} else {
		err = sam_usbhs_prep_in(dev, buf, ep_cfg);
	}

	if (err != 0) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, -ECONNREFUSED);
	} else {
		udc_ep_set_busy(ep_cfg, true);
	}
}

static ALWAYS_INLINE void sam_usbhs_thread_handler(const struct device *const dev)
{
	struct udc_sam_usbhs_data *const priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	uint32_t evt;
	uint32_t eps;
	uint8_t ep;
	int err;

	evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);
	udc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(SAM_USBHS_EVT_XFER_FINISHED)) {
		k_event_clear(&priv->events, BIT(SAM_USBHS_EVT_XFER_FINISHED));

		eps = atomic_clear(&priv->xfer_finished);

		while (eps) {
			ep = bit_to_ep(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);
			LOG_DBG("Finished event ep 0x%02x", ep);

			if (USB_EP_DIR_IS_IN(ep)) {
				err = sam_usbhs_handle_evt_din(dev, ep_cfg);
			} else {
				err = sam_usbhs_handle_evt_dout(dev, ep_cfg);
			}

			if (err) {
				udc_submit_event(dev, UDC_EVT_ERROR, err);
			}

			if (!udc_ep_is_busy(ep_cfg)) {
				sam_usbhs_handle_xfer_next(dev, ep_cfg);
			} else {
				LOG_ERR("Endpoint 0x%02x busy", ep);
			}
		}
	}

	if (evt & BIT(SAM_USBHS_EVT_XFER_NEW)) {
		k_event_clear(&priv->events, BIT(SAM_USBHS_EVT_XFER_NEW));

		eps = atomic_clear(&priv->xfer_new);

		while (eps) {
			ep = bit_to_ep(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);
			LOG_DBG("New transfer ep 0x%02x in the queue", ep);

			if (!udc_ep_is_busy(ep_cfg)) {
				sam_usbhs_handle_xfer_next(dev, ep_cfg);
			} else {
				LOG_ERR("Endpoint 0x%02x busy", ep);
			}
		}
	}

	if (evt & BIT(SAM_USBHS_EVT_SETUP)) {
		k_event_clear(&priv->events, BIT(SAM_USBHS_EVT_SETUP));
		atomic_clear(&priv->setup_pending);
		err = sam_usbhs_handle_evt_setup(dev);
		if (err) {
			udc_submit_event(dev, UDC_EVT_ERROR, err);
		}
	}

	/*
	 * Process pending OUT data. This must be checked AFTER SETUP processing
	 * because SETUP allocates the buffer that pending OUT data needs.
	 */
	if (evt & BIT(SAM_USBHS_EVT_OUT_PENDING)) {
		k_event_clear(&priv->events, BIT(SAM_USBHS_EVT_OUT_PENDING));

		eps = atomic_clear(&priv->out_pending);

		while (eps) {
			ep = bit_to_ep(&eps);
			LOG_DBG("Pending OUT data for ep 0x%02x", ep);
			err = sam_usbhs_process_pending_out(dev, ep);
			if (err) {
				udc_submit_event(dev, UDC_EVT_ERROR, err);
			}
		}
	}

	udc_unlock_internal(dev);
}

static void sam_usbhs_handle_setup_isr(const struct device *dev)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	struct udc_sam_usbhs_data *const priv = udc_get_private(dev);
	volatile uint8_t *fifo = sam_usbhs_ep_fifo(0);
	uint32_t byte_count;

	byte_count = (base->USBHS_DEVEPTISR[0] & USBHS_DEVEPTISR_BYCT_Msk)
		     >> USBHS_DEVEPTISR_BYCT_Pos;

	if (byte_count != 8) {
		LOG_ERR("Wrong byte count %u for setup packet", byte_count);
	}

	for (int i = 0; i < 8; i++) {
		priv->setup[i] = fifo[i];
	}

	base->USBHS_DEVEPTICR[0] = USBHS_DEVEPTICR_CTRL_RXSTPIC;
	atomic_set(&priv->setup_pending, 1);
}

/*
 * Process deferred OUT data in thread context.
 *
 * Called when ISR received data but no buffer was available at that time.
 * The ISR disabled RXOUTE and set out_pending flag. This function retries
 * processing now that buffers may be available.
 */
static int sam_usbhs_process_pending_out(const struct device *dev,
					 const uint8_t ep_addr)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	struct udc_sam_usbhs_data *const priv = udc_get_private(dev);
	uint8_t ep_idx = ep_addr_to_hw_ep(ep_addr);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep_addr);
	volatile uint8_t *fifo = sam_usbhs_ep_fifo(ep_idx);
	struct net_buf *buf;
	uint32_t byte_count;
	uint32_t size;
	bool is_ctrl = (ep_idx == 0);

	/*
	 * Check if data is available:
	 * - Control EP0: Check RXOUTI (FIFOCON is always 0)
	 * - Bulk/Int/Iso: Check FIFOCON
	 */
	if (is_ctrl) {
		if (!(base->USBHS_DEVEPTISR[ep_idx] & USBHS_DEVEPTISR_RXOUTI)) {
			LOG_DBG("EP0 OUT no data (RXOUTI=0)");
			/* Re-enable RXOUTE since no data to process */
			base->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_RXOUTES;
			return 0;
		}
	} else {
		if (!(base->USBHS_DEVEPTIMR[ep_idx] & USBHS_DEVEPTIMR_FIFOCON)) {
			LOG_DBG("OUT ep 0x%02x no data in bank (FIFOCON=0)", ep_addr);
			return 0;
		}
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		/*
		 * No buffer available. Leave data in FIFO.
		 * Host will get NAK for subsequent packets.
		 * Keep pending bit set to retry when buffer available.
		 */
		if (is_ctrl) {
			/* For EP0, keep RXOUTE disabled, keep RXOUTI set */
			LOG_WRN("EP0 OUT no buffer - NAK until ready");
		} else {
			LOG_WRN("OUT ep 0x%02x no buffer - NAK until ready", ep_addr);
		}
		return -ENOBUFS;
	}

	/* Read byte count */
	byte_count = (base->USBHS_DEVEPTISR[ep_idx] & USBHS_DEVEPTISR_BYCT_Msk)
		     >> USBHS_DEVEPTISR_BYCT_Pos;

	LOG_DBG("Thread OUT ep 0x%02x byte_count %u room %u",
		ep_addr, byte_count, net_buf_tailroom(buf));

	size = MIN(byte_count, net_buf_tailroom(buf));

	/* Read data from FIFO */
	if (is_ctrl) {
		for (uint32_t i = 0; i < size; i++) {
			priv->ctrl_out_buf[i] = fifo[i];
		}
		net_buf_add_mem(buf, priv->ctrl_out_buf, size);
	} else {
		uint8_t *data = net_buf_add(buf, size);

		for (uint32_t i = 0; i < size; i++) {
			data[i] = fifo[i];
		}
	}

	/*
	 * Release the bank and re-enable interrupts.
	 * ISR left RXOUTI set when deferring, clear it now.
	 */
	base->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_RXOUTIC;
	if (!is_ctrl) {
		base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_FIFOCONC;
	}
	base->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_RXOUTES;

	if (!net_buf_tailroom(buf) || size < udc_mps_ep_size(ep_cfg)) {
		/* Transfer complete - buffer full or short packet received */
		atomic_set_bit(&priv->xfer_finished, ep_to_bit(ep_addr));
		k_event_post(&priv->events, BIT(SAM_USBHS_EVT_XFER_FINISHED));
	}

	return 0;
}

/*
 * OUT ISR handler - optimized for minimal thread wake-ups.
 *
 * All endpoints (including EP0) are processed in ISR when buffer available.
 * Uses NBUSYBK to process multiple packets per ISR for dual-banked endpoints.
 *
 * For CONTROL endpoints (EP0) - single bank:
 *   - Read data, clear RXOUTI
 *
 * For BULK/INT/ISO endpoints - dual bank with FIFOCON:
 *   - Loop while NBUSYBK > 0 to drain all available packets
 *   - Clear RXOUTI, read data, clear FIFOCON for each packet
 */
static void sam_usbhs_handle_out_isr(const struct device *dev,
				     const uint8_t ep_idx)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	struct udc_sam_usbhs_data *const priv = udc_get_private(dev);
	uint8_t ep_addr = hw_ep_to_ep_addr(ep_idx, false);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep_addr);
	volatile uint8_t *fifo = sam_usbhs_ep_fifo(ep_idx);
	struct net_buf *buf;
	uint32_t byte_count;
	uint32_t size;
	uint8_t *data;
	bool is_ctrl = (ep_idx == 0);

	do {
		buf = udc_buf_peek(ep_cfg);
		if (buf == NULL) {
			/*
			 * No buffer available. Disable RXOUTE to stop
			 * interrupts. Will be re-enabled when buffer queued.
			 */
			LOG_DBG("ISR OUT ep 0x%02x no buffer - defer", ep_addr);
			base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_RXOUTEC;
			atomic_set_bit(&priv->out_pending, ep_to_bit(ep_addr));
			return;
		}

		byte_count = (base->USBHS_DEVEPTISR[ep_idx] &
			      USBHS_DEVEPTISR_BYCT_Msk) >>
			     USBHS_DEVEPTISR_BYCT_Pos;

		LOG_DBG("ISR OUT ep 0x%02x byte_count %u", ep_addr, byte_count);

		size = MIN(byte_count, net_buf_tailroom(buf));
		data = net_buf_add(buf, size);

		for (uint32_t i = 0; i < size; i++) {
			data[i] = fifo[i];
		}

		/*
		 * Release the bank:
		 * - Control EP0: Clear RXOUTI only (single bank, no FIFOCON)
		 * - Bulk/Int/Iso: Clear RXOUTI first, then FIFOCON
		 */
		base->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_RXOUTIC;
		if (!is_ctrl) {
			base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_FIFOCONC;
		}

		if (!net_buf_tailroom(buf) || size < udc_mps_ep_size(ep_cfg)) {
			/*
			 * Transfer complete (buffer full or short/ZLP).
			 * Disable RXOUTE until new buffer queued.
			 */
			base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_RXOUTEC;
			atomic_set_bit(&priv->xfer_finished, ep_to_bit(ep_addr));
			return;
		}

		/*
		 * For dual-banked endpoints, check if another packet is ready.
		 * NBUSYBK indicates number of busy banks (packets waiting).
		 * Continue processing while data available and buffer has room.
		 */
	} while (!is_ctrl &&
		 ((base->USBHS_DEVEPTISR[ep_idx] & USBHS_DEVEPTISR_NBUSYBK_Msk) >>
		  USBHS_DEVEPTISR_NBUSYBK_Pos) > 0);
}

static void sam_usbhs_handle_in_isr(const struct device *dev,
				    const uint8_t ep_idx)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	struct udc_sam_usbhs_data *const priv = udc_get_private(dev);
	uint8_t ep_addr = hw_ep_to_ep_addr(ep_idx, true);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, ep_addr);
	volatile uint8_t *fifo = sam_usbhs_ep_fifo(ep_idx);
	struct net_buf *buf;
	uint32_t len;

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep_addr);
		base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_TXINEC;
		return;
	}

	len = MIN(buf->len, udc_mps_ep_size(ep_cfg));
	LOG_DBG("ISR IN ep 0x%02x sent %u", ep_addr, len);

	net_buf_pull(buf, len);

	if (buf->len) {
		len = MIN(buf->len, udc_mps_ep_size(ep_cfg));
		for (uint32_t i = 0; i < len; i++) {
			fifo[i] = buf->data[i];
		}
		barrier_dsync_fence_full();

		base->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_TXINIC;
		base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_FIFOCONC;
	} else {
		if (udc_ep_buf_has_zlp(buf)) {
			base->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_TXINIC;
			base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_FIFOCONC;
			udc_ep_buf_clear_zlp(buf);
			return;
		}

		base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_TXINEC;
		atomic_set_bit(&priv->xfer_finished, ep_to_bit(ep_addr));
	}
}

static void sam_usbhs_handle_ep_isr(const struct device *dev,
				    const uint8_t ep_idx)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	uint32_t sr = base->USBHS_DEVEPTISR[ep_idx] & base->USBHS_DEVEPTIMR[ep_idx];

	SAM_USBHS_ISR_DBG_EP(base, ep_idx);

	if (ep_idx == 0) {
		if (sr & USBHS_DEVEPTISR_CTRL_RXSTPI) {
			sam_usbhs_handle_setup_isr(dev);
			return;
		}
	}

	if (sr & USBHS_DEVEPTISR_RXOUTI) {
		sam_usbhs_handle_out_isr(dev, ep_idx);
	}

	if (sr & USBHS_DEVEPTISR_TXINI) {
		sam_usbhs_handle_in_isr(dev, ep_idx);
	}
}

static void sam_usbhs_isr_handler(const struct device *dev)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	struct udc_sam_usbhs_data *const priv = udc_get_private(dev);
	Usbhs *const base = config->base;
	uint32_t sr = base->USBHS_DEVISR & base->USBHS_DEVIMR;
	uint32_t pending_events = 0;

	SAM_USBHS_ISR_DBG_DEV(base, sr);

	if (sr & USBHS_DEVISR_WAKEUP) {
		base->USBHS_DEVICR = USBHS_DEVICR_WAKEUPC;
		/* Disable WAKEUP, enable SUSP to detect next suspend */
		base->USBHS_DEVIDR = USBHS_DEVIDR_WAKEUPEC;
		base->USBHS_DEVIER = USBHS_DEVIER_SUSPES;
		LOG_DBG("USB Wakeup ISR");
		if (udc_is_suspended(dev)) {
			LOG_DBG("USB Wakeup");
			udc_set_suspended(dev, false);
			udc_submit_event(dev, UDC_EVT_RESUME, 0);
		}
	}

	if (sr & USBHS_DEVISR_EORSM) {
		LOG_DBG("USB End Of Resume ISR");
		base->USBHS_DEVICR = USBHS_DEVICR_EORSMC;
		if (udc_is_suspended(dev)) {
			LOG_DBG("USB Resume");
			udc_set_suspended(dev, false);
			udc_submit_event(dev, UDC_EVT_RESUME, 0);
		}
	}

	if (sr & USBHS_DEVISR_EORST) {
		base->USBHS_DEVICR = USBHS_DEVICR_EORSTC;
		LOG_DBG("USB Reset");

		/*
		 * The device clears some of the configuration of EP0
		 * when it receives the End of Reset. Re-enable interrupts.
		 */
		base->USBHS_DEVEPTIER[0] = USBHS_DEVEPTIER_CTRL_RXSTPES |
					   USBHS_DEVEPTIER_RXOUTES;

		udc_submit_event(dev, UDC_EVT_RESET, 0);
	}

	if (sr & USBHS_DEVISR_SUSP) {
		LOG_DBG("USB Suspend ISR");
		base->USBHS_DEVICR = USBHS_DEVICR_SUSPC;
		/* Disable SUSP, enable WAKEUP to detect resume */
		base->USBHS_DEVIDR = USBHS_DEVIDR_SUSPEC;
		base->USBHS_DEVIER = USBHS_DEVIER_WAKEUPES;
		if (!udc_is_suspended(dev)) {
			LOG_DBG("USB Suspend");
			udc_set_suspended(dev, true);
			udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
		}
	}

	if (sr & USBHS_DEVISR_SOF) {
		base->USBHS_DEVICR = USBHS_DEVICR_SOFC;
		udc_submit_sof_event(dev);
	}

	if (sr & USBHS_DEVISR_PEP__Msk) {
		LOG_DBG("EP ISRs pending: sr=0x%08x", sr);
	}

	for (uint8_t ep_idx = 0; ep_idx < NUM_OF_EP_MAX; ep_idx++) {
		if (sr & BIT(USBHS_DEVISR_PEP_0_Pos + ep_idx)) {
			sam_usbhs_handle_ep_isr(dev, ep_idx);
		}
	}

	/*
	 * Batched thread wake-up: check all pending flags and wake thread
	 * once if any events need processing. This reduces context switches.
	 */
	if (atomic_get(&priv->setup_pending)) {
		pending_events |= BIT(SAM_USBHS_EVT_SETUP);
	}
	if (atomic_get(&priv->xfer_finished)) {
		pending_events |= BIT(SAM_USBHS_EVT_XFER_FINISHED);
	}
	if (atomic_get(&priv->out_pending)) {
		pending_events |= BIT(SAM_USBHS_EVT_OUT_PENDING);
	}
	if (atomic_get(&priv->xfer_new)) {
		pending_events |= BIT(SAM_USBHS_EVT_XFER_NEW);
	}

	if (pending_events) {
		k_event_post(&priv->events, pending_events);
	}
}

static int udc_sam_usbhs_ep_enqueue(const struct device *dev,
				    struct udc_ep_config *const ep_cfg,
				    struct net_buf *buf)
{
	struct udc_sam_usbhs_data *const priv = udc_get_private(dev);

	LOG_DBG("%s enqueue 0x%02x %p", dev->name, ep_cfg->addr, (void *)buf);
	udc_buf_put(ep_cfg, buf);

	if (ep_cfg->stat.halted) {
		LOG_WRN("ep 0x%02x is halted, not starting transfer", ep_cfg->addr);
	} else {
		atomic_set_bit(&priv->xfer_new, ep_to_bit(ep_cfg->addr));
		k_event_post(&priv->events, BIT(SAM_USBHS_EVT_XFER_NEW));
	}

	return 0;
}

static int udc_sam_usbhs_ep_dequeue(const struct device *dev,
				    struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	struct udc_sam_usbhs_data *const priv = udc_get_private(dev);
	Usbhs *const base = config->base;
	uint8_t ep_idx = ep_addr_to_hw_ep(ep_cfg->addr);
	int bit = ep_to_bit(ep_cfg->addr);
	unsigned int lock_key;
	struct net_buf *buf;

	lock_key = irq_lock();

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_TXINEC;
	} else {
		base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_RXOUTEC;
		/*
		 * Clear out_pending to prevent thread from processing
		 * stale deferred data for this cancelled transfer.
		 */
		atomic_clear_bit(&priv->out_pending, bit);
	}

	/*
	 * Clear xfer_finished to prevent thread from processing
	 * stale completion for this cancelled transfer.
	 */
	atomic_clear_bit(&priv->xfer_finished, bit);

	buf = udc_buf_get_all(ep_cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
		udc_ep_set_busy(ep_cfg, false);
	}

	irq_unlock(lock_key);

	return 0;
}

/*
 * USB 2.0 Specification Maximum Packet Sizes (Section 5.5-5.8)
 *
 * +---------------+------------------------+------------------------+
 * | Transfer Type | High-Speed (480 Mbps)  | Full-Speed (12 Mbps)   |
 * +---------------+------------------------+------------------------+
 * | Control       | 64 bytes (fixed)       | 8, 16, 32, or 64 bytes |
 * | Bulk          | 512 bytes (fixed)      | 8, 16, 32, or 64 bytes |
 * | Interrupt     | 1-1024 bytes           | 1-64 bytes             |
 * | Isochronous   | 1-1024 bytes           | 1-1023 bytes           |
 * +---------------+------------------------+------------------------+
 *
 * Notes:
 * - Control EP0 max is always 64 bytes for HS, typically 64 for FS
 * - Bulk endpoints have fixed max sizes per speed mode
 * - Interrupt/Isochronous allow variable sizes up to the maximum
 * - Hardware (USBHS) supports up to 1024 bytes for EP1-EP9
 */
static uint16_t sam_usbhs_get_max_mps(uint8_t ep_type, bool is_hs)
{
	switch (ep_type) {
	case USB_EP_TYPE_CONTROL:
		return 64U;
	case USB_EP_TYPE_BULK:
		return is_hs ? 512U : 64U;
	case USB_EP_TYPE_INTERRUPT:
		return is_hs ? 1024U : 64U;
	case USB_EP_TYPE_ISO:
		return is_hs ? 1024U : 1023U;
	default:
		return 0U;
	}
}

static int udc_sam_usbhs_ep_enable(const struct device *dev,
				   struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	struct udc_data *data = dev->data;
	Usbhs *const base = config->base;
	uint8_t ep_idx = ep_addr_to_hw_ep(ep_cfg->addr);
	uint8_t ep_type = ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK;
	uint16_t max_mps;
	uint32_t regval = 0;
	uint8_t log2ceil_mps;
	uint8_t eptype;

	LOG_DBG("Enable ep 0x%02x (hw_idx=%d, type=%d, mps=%d)",
		ep_cfg->addr, ep_idx, ep_type, ep_cfg->mps);

	/*
	 * Validate MPS against USB 2.0 specification limits.
	 * The maximum packet size depends on transfer type and bus speed.
	 */
	max_mps = sam_usbhs_get_max_mps(ep_type, data->caps.hs);
	if (ep_cfg->mps > max_mps) {
		LOG_ERR("EP 0x%02x mps %d exceeds USB spec max %d for type %d (%s)",
			ep_cfg->addr, ep_cfg->mps, max_mps, ep_type,
			data->caps.hs ? "HS" : "FS");
		return -EINVAL;
	}

	/*
	 * Validate endpoint direction constraints.
	 * SAM USBHS hardware requirement: each HW endpoint (except EP0) can
	 * only be configured as IN or OUT, not both. Following legacy driver:
	 *   - Odd endpoints (1, 3, 5, 7, 9): IN only
	 *   - Even endpoints (2, 4, 6, 8): OUT only
	 */
	if (ep_idx != 0) {
		bool is_odd = (ep_idx & 1) != 0;
		bool is_in = USB_EP_DIR_IS_IN(ep_cfg->addr);

		if (is_odd && !is_in) {
			LOG_ERR("EP%d is odd, must be IN (got 0x%02x)",
				ep_idx, ep_cfg->addr);
			return -EINVAL;
		}
		if (!is_odd && is_in) {
			LOG_ERR("EP%d is even, must be OUT (got 0x%02x)",
				ep_idx, ep_cfg->addr);
			return -EINVAL;
		}
	}

	switch (ep_type) {
	case USB_EP_TYPE_CONTROL:
		eptype = USBHS_EPTYPE_CTRL;
		break;
	case USB_EP_TYPE_ISO:
		eptype = USBHS_EPTYPE_ISO;
		break;
	case USB_EP_TYPE_BULK:
		eptype = USBHS_EPTYPE_BULK;
		break;
	case USB_EP_TYPE_INTERRUPT:
		eptype = USBHS_EPTYPE_INT;
		break;
	default:
		return -EINVAL;
	}

	sam_usbhs_ep_reset(dev, ep_idx);

	regval = USBHS_DEVEPTCFG_EPTYPE(eptype);

	/*
	 * Map the MPS to hardware buffer size. Only power-of-2 sizes from
	 * 8 to 1024 bytes are supported. Calculate log2 ceiling and subtract 3
	 * to get the EPSIZE field value (0=8B, 1=16B, ... 7=1024B).
	 * Use 64 bytes minimum for Full-Speed USB compliance.
	 */
	log2ceil_mps = 32 - __builtin_clz((MAX(ep_cfg->mps, 64) << 1) - 1) - 1;
	regval |= USBHS_DEVEPTCFG_EPSIZE(log2ceil_mps - 3);

	if (USB_EP_DIR_IS_IN(ep_cfg->addr) &&
	    (ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) != USB_EP_TYPE_CONTROL) {
		regval |= USBHS_DEVEPTCFG_EPDIR_IN;
	}

	/*
	 * Configure endpoint banks:
	 * - Control endpoints: single-bank (per datasheet recommendation)
	 * - Bulk/Interrupt/ISO endpoints: dual-bank (ping-pong mode)
	 *
	 * AUTOSW (automatic bank switching) is enabled for non-control endpoints
	 * and requires multi-bank configuration to function correctly.
	 */
	if ((ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_CONTROL) {
		regval |= USBHS_DEVEPTCFG_EPBK(USBHS_EPBK_1_BANK);
	} else {
		regval |= USBHS_DEVEPTCFG_EPBK(USBHS_EPBK_2_BANK);
	}

	if ((ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) != USB_EP_TYPE_CONTROL) {
		regval |= USBHS_DEVEPTCFG_AUTOSW;
	}

	/*
	 * ALLOC must be written in the same operation as the other configuration
	 * bits. If written separately (read-modify-write), the hardware may reset
	 * configuration bits like AUTOSW when ALLOC is not set.
	 */
	regval |= USBHS_DEVEPTCFG_ALLOC;

	LOG_DBG("EP%d config: writing 0x%08x", ep_idx, regval);

	base->USBHS_DEVEPTCFG[ep_idx] = regval;

	if (!sam_usbhs_ep_is_configured(dev, ep_idx)) {
		LOG_ERR("Endpoint %d configuration failed (CFG=0x%08x)",
			ep_idx, base->USBHS_DEVEPTCFG[ep_idx]);
		return -EINVAL;
	}

	LOG_DBG("EP%d after enable: CFG=0x%08x ISR=0x%08x",
		ep_idx, base->USBHS_DEVEPTCFG[ep_idx], base->USBHS_DEVEPTISR[ep_idx]);

	base->USBHS_DEVEPT |= BIT(USBHS_DEVEPT_EPEN0_Pos + ep_idx);
	base->USBHS_DEVIER = BIT(USBHS_DEVIER_PEP_0_Pos + ep_idx);

	if (ep_idx == 0) {
		base->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_CTRL_RXSTPES;
		base->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_RXOUTES;
	} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		base->USBHS_DEVEPTICR[ep_idx] = USBHS_DEVEPTICR_TXINIC;
		base->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_TXINES;
	} else {
		/*
		 * OUT endpoint: Enable RXOUTE to receive OUT packets.
		 * Per point 8: RXOUTE must be enabled at EP enable time.
		 * Banks are empty after configuration, FIFOCON=0 (no data).
		 */
		base->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_RXOUTES;
	}

	return 0;
}

static int udc_sam_usbhs_ep_disable(const struct device *dev,
				    struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	uint8_t ep_idx = ep_addr_to_hw_ep(ep_cfg->addr);

	LOG_DBG("Disable ep 0x%02x", ep_cfg->addr);

	base->USBHS_DEVIDR = BIT(USBHS_DEVIDR_PEP_0_Pos + ep_idx);
	base->USBHS_DEVEPT &= ~BIT(USBHS_DEVEPT_EPEN0_Pos + ep_idx);
	base->USBHS_DEVEPTCFG[ep_idx] &= ~USBHS_DEVEPTCFG_ALLOC;

	return 0;
}

static int udc_sam_usbhs_ep_set_halt(const struct device *dev,
				     struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	uint8_t ep_idx = ep_addr_to_hw_ep(ep_cfg->addr);

	LOG_DBG("Set halt ep 0x%02x", ep_cfg->addr);

	base->USBHS_DEVEPTIER[ep_idx] = USBHS_DEVEPTIER_CTRL_STALLRQS;

	if (USB_EP_GET_IDX(ep_cfg->addr) != 0) {
		ep_cfg->stat.halted = true;
	}

	return 0;
}

static int udc_sam_usbhs_ep_clear_halt(const struct device *dev,
				       struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	struct udc_sam_usbhs_data *const priv = udc_get_private(dev);
	uint8_t ep_idx = ep_addr_to_hw_ep(ep_cfg->addr);

	LOG_DBG("Clear halt ep 0x%02x", ep_cfg->addr);

	if (ep_idx == 0) {
		return 0;
	}

	/*
	 * Clear the STALL request. Per datasheet: "All following requests
	 * are discarded and handshaked with a STALL until the STALLRQ bit
	 * is cleared." After clearing, normal operation resumes.
	 */
	base->USBHS_DEVEPTIDR[ep_idx] = USBHS_DEVEPTIDR_CTRL_STALLRQC;

	ep_cfg->stat.halted = false;

	/* Resume queued transfers if any */
	if (!udc_ep_is_busy(ep_cfg) && udc_buf_peek(ep_cfg)) {
		atomic_set_bit(&priv->xfer_new, ep_to_bit(ep_cfg->addr));
		k_event_post(&priv->events, BIT(SAM_USBHS_EVT_XFER_NEW));
	}

	return 0;
}

static int udc_sam_usbhs_set_address(const struct device *dev,
				     const uint8_t addr)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;

	LOG_DBG("Set address %u for %s", addr, dev->name);

	base->USBHS_DEVCTRL &= ~(USBHS_DEVCTRL_UADD_Msk | USBHS_DEVCTRL_ADDEN);
	if (addr != 0) {
		base->USBHS_DEVCTRL |= USBHS_DEVCTRL_UADD(addr) | USBHS_DEVCTRL_ADDEN;
	}

	return 0;
}

static int udc_sam_usbhs_host_wakeup(const struct device *dev)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;

	LOG_DBG("Remote wakeup from %s", dev->name);

	base->USBHS_DEVCTRL |= USBHS_DEVCTRL_RMWKUP;

	return 0;
}

static int udc_sam_usbhs_test_mode(const struct device *dev,
				   const uint8_t mode, const bool dryrun)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;

	/* USB 2.0 spec defined test packet for Test_Packet mode */
	static const uint8_t test_packet[] = {
		/* JKJKJKJK * 9 */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		/* JJKKJJKK * 8 */
		0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
		/* JJJJKKKK * 8 */
		0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
		/* JJJJJJJKKKKKKK * 8 */
		0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF,
		/* JJJJJJJK * 8 */
		0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD,
		/* JKKKKKKK * 8, {JKKKKKKK * 10}(not used) */
		0xFC, 0x7E, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0x7E
	};

	if (mode == 0U || mode > USB_SFS_TEST_MODE_FORCE_ENABLE) {
		return -EINVAL;
	}

	if (dryrun) {
		LOG_DBG("Test Mode %u supported", mode);
		return 0;
	}

	LOG_DBG("Enable Test Mode %u", mode);

	/* Set SPDCONF to forced High-Speed for test modes */
	base->USBHS_DEVCTRL = (base->USBHS_DEVCTRL & ~USBHS_DEVCTRL_SPDCONF_Msk) |
			      USBHS_DEVCTRL_SPDCONF_HIGH_SPEED;

	switch (mode) {
	case USB_SFS_TEST_MODE_J:
		base->USBHS_DEVCTRL |= USBHS_DEVCTRL_TSTJ;
		break;
	case USB_SFS_TEST_MODE_K:
		base->USBHS_DEVCTRL |= USBHS_DEVCTRL_TSTK;
		break;
	case USB_SFS_TEST_MODE_SE0_NAK:
		/* SE0_NAK: just enable test mode, no additional bits needed */
		break;
	case USB_SFS_TEST_MODE_PACKET:
		/*
		 * Test_Packet mode sequence per datasheet:
		 * 1. TSTPCKT = 1
		 * 2. Store data in endpoint bank
		 * 3. Clear FIFOCON to start transmission
		 */
		volatile uint8_t *fifo = sam_usbhs_ep_fifo(0);

		/* Disable and reset EP0 */
		base->USBHS_DEVEPT &= ~USBHS_DEVEPT_EPEN0;
		base->USBHS_DEVEPT |= USBHS_DEVEPT_EPRST0;
		base->USBHS_DEVEPT &= ~USBHS_DEVEPT_EPRST0;

		/* Reconfigure EP0 as bulk IN, 64 bytes, single bank */
		base->USBHS_DEVEPTCFG[0] = USBHS_DEVEPTCFG_EPTYPE(USBHS_EPTYPE_BULK) |
					   USBHS_DEVEPTCFG_EPSIZE(USBHS_EPSIZE_64) |
					   USBHS_DEVEPTCFG_EPDIR_IN |
					   USBHS_DEVEPTCFG_EPBK(USBHS_EPBK_1_BANK) |
					   USBHS_DEVEPTCFG_ALLOC;

		/* Enable EP0 */
		base->USBHS_DEVEPT |= USBHS_DEVEPT_EPEN0;

		/* Step 1: Enable test packet mode */
		base->USBHS_DEVCTRL |= USBHS_DEVCTRL_TSTPCKT;

		/* Step 2: Copy test packet to FIFO */
		for (size_t i = 0; i < sizeof(test_packet); i++) {
			fifo[i] = test_packet[i];
		}

		/* Step 3: Clear FIFOCON to start repeated transmission */
		base->USBHS_DEVEPTIDR[0] = USBHS_DEVEPTIDR_FIFOCONC;
		break;
	case USB_SFS_TEST_MODE_FORCE_ENABLE:
		/* Force Enable is typically not supported in device mode */
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	return 0;
}

static enum udc_bus_speed udc_sam_usbhs_device_speed(const struct device *dev)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	uint32_t speed;

	speed = (base->USBHS_SR & USBHS_SR_SPEED_Msk) >> USBHS_SR_SPEED_Pos;

	if (speed == USBHS_SR_SPEED_HIGH_SPEED_Val) {
		return UDC_BUS_SPEED_HS;
	}

	return UDC_BUS_SPEED_FS;
}

static int udc_sam_usbhs_enable(const struct device *dev)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;
	int ret;

	LOG_DBG("Enable device %s", dev->name);

	ret = clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&config->clock_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to enable USBHS clock");
		return ret;
	}

	base->USBHS_CTRL = USBHS_CTRL_UIMOD | USBHS_CTRL_USBE | USBHS_CTRL_FRZCLK;
	barrier_dsync_fence_full();

	if (SAM_USBHS_HS_ENABLED) {
		base->USBHS_DEVCTRL = USBHS_DEVCTRL_DETACH | USBHS_DEVCTRL_SPDCONF_NORMAL;
	} else {
		base->USBHS_DEVCTRL = USBHS_DEVCTRL_DETACH | USBHS_DEVCTRL_SPDCONF_LOW_POWER;
	}

	sam_usbhs_enable_clock();

	base->USBHS_CTRL = USBHS_CTRL_UIMOD | USBHS_CTRL_USBE;

	while (!(base->USBHS_SR & USBHS_SR_CLKUSABLE)) {
		k_yield();
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				     USB_EP_TYPE_CONTROL, 64, 0);
	if (ret) {
		LOG_ERR("Failed to enable control OUT endpoint");
		return ret;
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				     USB_EP_TYPE_CONTROL, 64, 0);
	if (ret) {
		LOG_ERR("Failed to enable control IN endpoint");
		return ret;
	}

	base->USBHS_DEVIER = USBHS_DEVIER_EORSMES | USBHS_DEVIER_EORSTES |
			     USBHS_DEVIER_SUSPES;

	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF)) {
		base->USBHS_DEVIER = USBHS_DEVIER_SOFES;
	}

	config->irq_enable_func(dev);

	base->USBHS_DEVCTRL &= ~USBHS_DEVCTRL_DETACH;

	return 0;
}

static int udc_sam_usbhs_disable(const struct device *dev)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	Usbhs *const base = config->base;

	LOG_DBG("Disable device %s", dev->name);

	config->irq_disable_func(dev);

	base->USBHS_DEVCTRL |= USBHS_DEVCTRL_DETACH;

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control OUT endpoint");
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control IN endpoint");
	}

	sam_usbhs_disable_clock();

	base->USBHS_CTRL = USBHS_CTRL_UIMOD | USBHS_CTRL_FRZCLK;

	clock_control_off(SAM_DT_PMC_CONTROLLER,
			  (clock_control_subsys_t)&config->clock_cfg);

	return 0;
}

static int udc_sam_usbhs_init(const struct device *dev)
{
	LOG_DBG("Init device %s", dev->name);

	return 0;
}

static int udc_sam_usbhs_shutdown(const struct device *dev)
{
	LOG_DBG("Shutdown device %s", dev->name);

	return 0;
}

static int udc_sam_usbhs_driver_preinit(const struct device *dev)
{
	const struct udc_sam_usbhs_config *config = dev->config;
	struct udc_sam_usbhs_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	uint16_t mps = SAM_USBHS_EP_MPS;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);
	atomic_clear(&priv->out_pending);
	atomic_clear(&priv->setup_pending);

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;
	if (SAM_USBHS_HS_ENABLED) {
		data->caps.hs = true;
	}

	/*
	 * Register endpoints with direction-constrained addresses.
	 *
	 * SAM USBHS hardware routes packets based on USB endpoint number in
	 * the token. Each hardware endpoint (except EP0) can only be configured
	 * as IN OR OUT, not both simultaneously.
	 *
	 * Following the legacy driver approach, we constrain endpoints:
	 *   - EP0: Control (bidirectional - handled specially by hardware)
	 *   - Even HW EPs (2, 4, 6, 8): OUT only (0x02, 0x04, 0x06, 0x08)
	 *   - Odd HW EPs (1, 3, 5, 7, 9): IN only (0x81, 0x83, 0x85, 0x87, 0x89)
	 *
	 * This ensures no conflicts can occur and matches legacy driver behavior.
	 */

	/* Register EP0 OUT (control) */
	config->ep_cfg_out[0].caps.out = 1;
	config->ep_cfg_out[0].caps.control = 1;
	config->ep_cfg_out[0].caps.mps = 64;
	config->ep_cfg_out[0].addr = USB_EP_DIR_OUT;
	err = udc_register_ep(dev, &config->ep_cfg_out[0]);
	if (err != 0) {
		LOG_ERR("Failed to register EP0 OUT");
		return err;
	}

	/* Register even endpoints as OUT (2, 4, 6, 8) */
	int out_idx = 1;
	int num_hw_eps = config->num_of_in_eps + config->num_of_out_eps - 1;

	for (int hw_ep = 2; hw_ep < num_hw_eps && out_idx < config->num_of_out_eps; hw_ep += 2) {
		config->ep_cfg_out[out_idx].caps.out = 1;
		config->ep_cfg_out[out_idx].caps.bulk = 1;
		config->ep_cfg_out[out_idx].caps.interrupt = 1;
		config->ep_cfg_out[out_idx].caps.iso = 1;
		config->ep_cfg_out[out_idx].caps.mps = mps;
		config->ep_cfg_out[out_idx].addr = USB_EP_DIR_OUT | hw_ep;
		err = udc_register_ep(dev, &config->ep_cfg_out[out_idx]);
		if (err != 0) {
			LOG_ERR("Failed to register OUT endpoint 0x%02x",
				config->ep_cfg_out[out_idx].addr);
			return err;
		}
		out_idx++;
	}

	/* Register EP0 IN (control) */
	config->ep_cfg_in[0].caps.in = 1;
	config->ep_cfg_in[0].caps.control = 1;
	config->ep_cfg_in[0].caps.mps = 64;
	config->ep_cfg_in[0].addr = USB_EP_DIR_IN;
	err = udc_register_ep(dev, &config->ep_cfg_in[0]);
	if (err != 0) {
		LOG_ERR("Failed to register EP0 IN");
		return err;
	}

	/* Register odd endpoints as IN (1, 3, 5, 7, 9) */
	int in_idx = 1;

	for (int hw_ep = 1; hw_ep < num_hw_eps && in_idx < config->num_of_in_eps; hw_ep += 2) {
		config->ep_cfg_in[in_idx].caps.in = 1;
		config->ep_cfg_in[in_idx].caps.bulk = 1;
		config->ep_cfg_in[in_idx].caps.interrupt = 1;
		config->ep_cfg_in[in_idx].caps.iso = 1;
		config->ep_cfg_in[in_idx].caps.mps = mps;
		config->ep_cfg_in[in_idx].addr = USB_EP_DIR_IN | hw_ep;
		err = udc_register_ep(dev, &config->ep_cfg_in[in_idx]);
		if (err != 0) {
			LOG_ERR("Failed to register IN endpoint 0x%02x",
				config->ep_cfg_in[in_idx].addr);
			return err;
		}
		in_idx++;
	}

	config->make_thread(dev);

	return 0;
}

static void udc_sam_usbhs_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void udc_sam_usbhs_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static const struct udc_api udc_sam_usbhs_api = {
	.lock = udc_sam_usbhs_lock,
	.unlock = udc_sam_usbhs_unlock,
	.device_speed = udc_sam_usbhs_device_speed,
	.init = udc_sam_usbhs_init,
	.enable = udc_sam_usbhs_enable,
	.disable = udc_sam_usbhs_disable,
	.shutdown = udc_sam_usbhs_shutdown,
	.set_address = udc_sam_usbhs_set_address,
	.host_wakeup = udc_sam_usbhs_host_wakeup,
	.test_mode = udc_sam_usbhs_test_mode,
	.ep_enable = udc_sam_usbhs_ep_enable,
	.ep_disable = udc_sam_usbhs_ep_disable,
	.ep_set_halt = udc_sam_usbhs_ep_set_halt,
	.ep_clear_halt = udc_sam_usbhs_ep_clear_halt,
	.ep_enqueue = udc_sam_usbhs_ep_enqueue,
	.ep_dequeue = udc_sam_usbhs_ep_dequeue,
};

#define UDC_SAM_USBHS_DEVICE_DEFINE(n)						   \
	PINCTRL_DT_INST_DEFINE(n);						   \
										   \
	static void udc_sam_usbhs_irq_enable_##n(const struct device *dev)	   \
	{									   \
		IRQ_CONNECT(DT_INST_IRQN(n),					   \
			    DT_INST_IRQ(n, priority),				   \
			    sam_usbhs_isr_handler,				   \
			    DEVICE_DT_INST_GET(n), 0);				   \
		irq_enable(DT_INST_IRQN(n));					   \
	}									   \
										   \
	static void udc_sam_usbhs_irq_disable_##n(const struct device *dev)	   \
	{									   \
		irq_disable(DT_INST_IRQN(n));					   \
	}									   \
										   \
	K_THREAD_STACK_DEFINE(udc_sam_usbhs_stack_##n,				   \
			      CONFIG_UDC_SAM_USBHS_STACK_SIZE);			   \
										   \
	static void udc_sam_usbhs_thread_##n(void *dev, void *arg1, void *arg2)	   \
	{									   \
		while (true) {							   \
			sam_usbhs_thread_handler(dev);				   \
		}								   \
	}									   \
										   \
	static void udc_sam_usbhs_make_thread_##n(const struct device *dev)	   \
	{									   \
		struct udc_sam_usbhs_data *priv = udc_get_private(dev);		   \
										   \
		k_thread_create(&priv->thread_data,				   \
				udc_sam_usbhs_stack_##n,			   \
				K_THREAD_STACK_SIZEOF(udc_sam_usbhs_stack_##n),	   \
				udc_sam_usbhs_thread_##n,			   \
				(void *)dev, NULL, NULL,			   \
				K_PRIO_COOP(CONFIG_UDC_SAM_USBHS_THREAD_PRIORITY), \
				K_ESSENTIAL,					   \
				K_NO_WAIT);					   \
		k_thread_name_set(&priv->thread_data, dev->name);		   \
	}									   \
										   \
	static struct udc_ep_config						   \
		ep_cfg_out_##n[DT_INST_PROP(n, num_out_endpoints)];		   \
	static struct udc_ep_config						   \
		ep_cfg_in_##n[DT_INST_PROP(n, num_in_endpoints)];		   \
										   \
	static const struct udc_sam_usbhs_config udc_sam_usbhs_config_##n = {	   \
		.base = (Usbhs *)DT_INST_REG_ADDR(n),				   \
		.irq_enable_func = udc_sam_usbhs_irq_enable_##n,		   \
		.irq_disable_func = udc_sam_usbhs_irq_disable_##n,		   \
		.make_thread = udc_sam_usbhs_make_thread_##n,			   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			   \
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),			   \
		.num_of_in_eps = DT_INST_PROP(n, num_in_endpoints),		   \
		.num_of_out_eps = DT_INST_PROP(n, num_out_endpoints),		   \
		.ep_cfg_in = ep_cfg_in_##n,					   \
		.ep_cfg_out = ep_cfg_out_##n,					   \
	};									   \
										   \
	static struct udc_sam_usbhs_data udc_priv_##n;				   \
										   \
	static struct udc_data udc_data_##n = {					   \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		   \
		.priv = &udc_priv_##n,						   \
	};									   \
										   \
	DEVICE_DT_INST_DEFINE(n, udc_sam_usbhs_driver_preinit, NULL,		   \
			      &udc_data_##n, &udc_sam_usbhs_config_##n,		   \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	   \
			      &udc_sam_usbhs_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_SAM_USBHS_DEVICE_DEFINE)
