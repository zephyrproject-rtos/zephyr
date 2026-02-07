/*
 * Copyright (c) 2025-2026 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "udc_common.h"

#include <string.h>

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/barrier.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(udc_sam_usbc, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT atmel_sam_usbc

typedef Usbc Usbc_t;

/* Maximum number of endpoints */
#define NUM_OF_EP_MAX           8
#define NOT_ALLOC_VIRT_EP	0xFF

/*
 * SAM USBC endpoint buffer descriptor structure.
 * Each endpoint has two banks (for dual-bank buffering).
 */
struct sam_usbc_udesc_sizes {
	uint32_t byte_count:15;
	uint32_t reserved:1;
	uint32_t multi_packet_size:15;
	uint32_t auto_zlp:1;
};

struct sam_usbc_desc_table {
	uint8_t *ep_pipe_addr;
	union {
		uint32_t sizes;
		struct sam_usbc_udesc_sizes udesc_sizes;
	};
	uint32_t bk_ctrl_stat;
	uint32_t ep_ctrl_stat;
};

BUILD_ASSERT(sizeof(struct sam_usbc_desc_table) == NUM_OF_EP_MAX * 2,
	     "Broken endpoint buffer descriptor");

struct udc_sam_usbc_config {
	volatile Usbc_t *base;
	struct sam_usbc_desc_table *dt;
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	struct pinctrl_dev_config *pcfg;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	void (*make_thread)(const struct device *dev);
};

enum sam_usbc_event_type {
	/* Setup packet received */
	SAM_USBC_EVT_SETUP,
	/* Trigger new transfer */
	SAM_USBC_EVT_XFER_NEW,
	/* Transfer for specific endpoint is finished */
	SAM_USBC_EVT_XFER_FINISHED,
};

struct udc_sam_usbc_data {
	struct k_thread thread_data;
	struct k_event events;
	atomic_t xfer_new;
	atomic_t xfer_finished;
	/*
	 * Per-endpoint per-bank buffer tracking for scatter-gather DMA.
	 * Tracks which net_buf is assigned to each bank of each endpoint.
	 * Required because we cannot derive net_buf* from buf->data alone.
	 * [phys_ep][bank_idx] where bank_idx is 0 or 1 for dual-bank.
	 */
	struct net_buf *bank_buf[NUM_OF_EP_MAX][2];
	/*
	 * Physical endpoint allocation tracking for REPNB redirection.
	 * When virtual EP IN and OUT have same number (e.g., 0x81 and 0x01),
	 * they must use different physical endpoints. REPNB redirects
	 * transactions for virtual EP m to physical EP n.
	 */
	uint8_t phys_ep_used;           /* Bitmask of allocated physical EPs */
	uint8_t virt_to_phys[NUM_OF_EP_MAX * 2]; /* [0-7]=OUT, [8-15]=IN */
	/*
	 * SETUP packet buffer - DMA target for EP0 SETUP reception.
	 * Must be 4-byte aligned for USB DMA to work correctly.
	 */
	uint8_t setup[8] __aligned(4);
};

/*
 * Debug state tracking - logs register values when they change.
 * [0][ep] = previous UESTA value, [1][ep] = used to detect loops
 */
static uint32_t dev_ep_sta_dbg[2][NUM_OF_EP_MAX];

static void sam_usbc_isr_sta_dbg(volatile Usbc_t *base, uint8_t ep_idx, uint32_t sr)
{
	if (base->UESTA[ep_idx] != dev_ep_sta_dbg[0][ep_idx]) {
		dev_ep_sta_dbg[0][ep_idx] = base->UESTA[ep_idx];
		dev_ep_sta_dbg[1][ep_idx] = 0;

		LOG_INF("ISR[%d] CON=%08x INT=%08x INTE=%08x "
			"ECON=%08x ESTA=%08x CFG=%08x%s%s%s", ep_idx,
			base->UDCON, base->UDINT, base->UDINTE,
			base->UECON[ep_idx], base->UESTA[ep_idx],
			base->UECFG[ep_idx],
			((sr & USBC_UESTA0_RXSTPI) ? " STP" : ""),
			((sr & USBC_UESTA0_RXOUTI) ? " OUT" : ""),
			((sr & USBC_UESTA0_TXINI) ? " IN" : ""));
	} else if (dev_ep_sta_dbg[0][ep_idx] != dev_ep_sta_dbg[1][ep_idx]) {
		dev_ep_sta_dbg[1][ep_idx] = dev_ep_sta_dbg[0][ep_idx];

		LOG_INF("ISR[%d] CON=%08x INT=%08x INTE=%08x "
			"ECON=%08x ESTA=%08x CFG=%08x LOOP", ep_idx,
			base->UDCON, base->UDINT, base->UDINTE,
			base->UECON[ep_idx], base->UESTA[ep_idx],
			base->UECFG[ep_idx]);
	}
}

static void sam_usbc_clean_sta_dbg(void)
{
	for (int i = 0; i < NUM_OF_EP_MAX; i++) {
		dev_ep_sta_dbg[0][i] = 0;
		dev_ep_sta_dbg[1][i] = 0;
	}
}

static void sam_usbc_ep_isr_sta(volatile Usbc_t *base, uint8_t ep_idx)
{
	uint32_t sr = base->UESTA[ep_idx];

	sam_usbc_isr_sta_dbg(base, ep_idx, sr);

	if (sr & USBC_UESTA0_RAMACERI) {
		base->UESTACLR[ep_idx] = USBC_UESTA0CLR_RAMACERIC;
		LOG_ERR("ISR: EP%d RAM Access Error", ep_idx);
	}
}

static inline int udc_ep_to_bnum(const uint8_t ep)
{
	if (USB_EP_DIR_IS_IN(ep)) {
		return 16UL + USB_EP_GET_IDX(ep);
	}

	return USB_EP_GET_IDX(ep);
}

static inline uint8_t udc_pull_ep_from_bmsk(uint32_t *const bitmap)
{
	unsigned int bit;

	__ASSERT_NO_MSG(bitmap && *bitmap);

	bit = find_lsb_set(*bitmap) - 1;
	*bitmap &= ~BIT(bit);

	if (bit >= 16U) {
		return USB_EP_DIR_IN | (bit - 16U);
	} else {
		return USB_EP_DIR_OUT | bit;
	}
}

/* Clock management helpers */
static ALWAYS_INLINE void sam_usbc_freeze_clk(volatile Usbc_t *const base)
{
	base->USBCON |= USBC_USBCON_FRZCLK;
}

static ALWAYS_INLINE void sam_usbc_unfreeze_clk(volatile Usbc_t *const base)
{
	base->USBCON &= ~USBC_USBCON_FRZCLK;

	while (base->USBCON & USBC_USBCON_FRZCLK) {
		;
	}
}

static inline struct sam_usbc_desc_table *sam_usbc_get_desc(
	const struct device *dev, const uint8_t ep_idx, const uint8_t bank)
{
	const struct udc_sam_usbc_config *config = dev->config;

	/* For single-bank mode, always use bank 0 */
	return &config->dt[ep_idx * 2 + bank];
}

static uint8_t sam_usbc_get_epsize(const uint16_t mps)
{
	if (mps <= 64) {
		return 3;
	} else if (mps <= 128) {
		return 4;
	} else if (mps <= 256) {
		return 5;
	} else if (mps <= 512) {
		return 6;
	}

	return 7;
}

/* Reset bank buffer tracking (called on disable/shutdown) */
static void sam_usbc_bank_buf_reset(const struct device *dev)
{
	struct udc_sam_usbc_data *priv = udc_get_private(dev);

	for (int ep = 0; ep < NUM_OF_EP_MAX; ep++) {
		priv->bank_buf[ep][0] = NULL;
		priv->bank_buf[ep][1] = NULL;
	}
}

/*
 * Physical endpoint allocation for REPNB redirection.
 *
 * The SAM USBC uses REPNB (Redirected Endpoint Number) to allow virtual
 * endpoints with the same number but different directions (IN/OUT) to use
 * different physical endpoints. For example:
 *   - Virtual EP1 IN (0x81) -> Physical EP1 (REPNB=0, default)
 *   - Virtual EP1 OUT (0x01) -> Physical EP4 (REPNB=1, redirected)
 *
 * The descriptor table and buffers are indexed by physical EP number.
 * ISR derives virtual address from UECFGn.REPNB and UECFGn.EPDIR.
 */

/* Convert virtual address to index in virt_to_phys array */
static inline uint8_t virt_addr_to_idx(const uint8_t virt_addr)
{
	uint8_t ep_num = USB_EP_GET_IDX(virt_addr);

	if (USB_EP_DIR_IS_IN(virt_addr)) {
		return ep_num + NUM_OF_EP_MAX;
	}
	return ep_num;
}

/* Get physical EP for a virtual address, returns NOT_ALLOC_VIRT_EP if not allocated */
static inline uint8_t virt_to_phys_ep(const struct device *dev, const uint8_t virt_addr)
{
	struct udc_sam_usbc_data *priv = udc_get_private(dev);

	return priv->virt_to_phys[virt_addr_to_idx(virt_addr)];
}

/* Allocate a physical endpoint for a virtual endpoint address */
static int alloc_phys_ep(const struct device *dev, const uint8_t virt_addr)
{
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	uint8_t virt_ep_num = USB_EP_GET_IDX(virt_addr);
	uint8_t idx = virt_addr_to_idx(virt_addr);

	/* EP0 always uses physical EP0 (control endpoint, no redirection) */
	if (virt_ep_num == 0) {
		priv->phys_ep_used |= BIT(0);
		priv->virt_to_phys[idx] = 0;
		return 0;
	}

	/* Try to use physical EP = virtual EP number first (no redirection) */
	if (!(priv->phys_ep_used & BIT(virt_ep_num))) {
		priv->phys_ep_used |= BIT(virt_ep_num);
		priv->virt_to_phys[idx] = virt_ep_num;
		LOG_DBG("Alloc phys EP%d for virt 0x%02x (no redirect)",
			virt_ep_num, virt_addr);
		return virt_ep_num;
	}

	/* Conflict: find next free physical EP and use REPNB redirection */
	for (int i = 1; i < NUM_OF_EP_MAX; i++) {
		if (!(priv->phys_ep_used & BIT(i))) {
			priv->phys_ep_used |= BIT(i);
			priv->virt_to_phys[idx] = i;
			LOG_DBG("Alloc phys EP%d for virt 0x%02x (REPNB=%d)",
				i, virt_addr, virt_ep_num);
			return i;
		}
	}

	LOG_ERR("No free physical EP for virt 0x%02x", virt_addr);
	return -ENOMEM;
}

/* Free a physical endpoint */
static void free_phys_ep(const struct device *dev, const uint8_t virt_addr)
{
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	uint8_t idx = virt_addr_to_idx(virt_addr);
	uint8_t phys_ep = priv->virt_to_phys[idx];

	if (phys_ep < NUM_OF_EP_MAX) {
		priv->phys_ep_used &= ~BIT(phys_ep);
		priv->virt_to_phys[idx] = NOT_ALLOC_VIRT_EP;
		LOG_DBG("Free phys EP%d (was virt 0x%02x)", phys_ep, virt_addr);
	}
}

/* Derive virtual address from physical EP by reading REPNB+EPDIR from HW */
static uint8_t phys_ep_to_virt_addr(const struct device *dev, const uint8_t phys_ep)
{
	const struct udc_sam_usbc_config *config = dev->config;
	volatile Usbc_t *base = config->base;
	uint32_t uecfg = base->UECFG[phys_ep];
	uint8_t repnb = (uecfg >> 16) & 0xF;  /* REPNB field bits 19:16 */
	uint8_t epdir = (uecfg >> 8) & 0x1;   /* EPDIR field bit 8 */
	uint8_t virt_ep_num;

	/* REPNB=0 means no redirection, physical EP = virtual EP */
	virt_ep_num = (repnb != 0) ? repnb : phys_ep;

	/* Build full virtual address with direction bit */
	return epdir ? (USB_EP_DIR_IN | virt_ep_num) : virt_ep_num;
}

/* Reset physical EP allocation tracking */
static void sam_usbc_phys_ep_reset(const struct device *dev)
{
	struct udc_sam_usbc_data *priv = udc_get_private(dev);

	priv->phys_ep_used = 0;
	memset(priv->virt_to_phys, NOT_ALLOC_VIRT_EP, sizeof(priv->virt_to_phys));
}

/* Prepare an OUT transfer */
static int sam_usbc_prep_out(const struct device *dev,
			     struct net_buf *const buf,
			     struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	volatile Usbc_t *base = config->base;
	const uint8_t phys_ep = virt_to_phys_ep(dev, ep_cfg->addr);
	struct sam_usbc_desc_table *dt;
	uint8_t bank = 0;

	dt = sam_usbc_get_desc(dev, phys_ep, bank);

	/*
	 * Direct DMA to net_buf - no staging buffer needed.
	 * USB DMA can access any embedded RAM including net_buf memory.
	 */
	dt->ep_pipe_addr = buf->data;
	dt->sizes = 0;

	/*
	 * Set multi_packet_size for all endpoints including EP0.
	 * This enables hardware to accumulate multi-packet transfers.
	 * Hardware signals RXOUTI only when transfer is complete (byte_count
	 * reaches multi_packet_size) or a short packet is received.
	 */
	dt->udesc_sizes.multi_packet_size = net_buf_tailroom(buf);

	/* Track buffer for completion handling */
	priv->bank_buf[phys_ep][bank] = buf;

	/* Clear RXOUTI and enable OUT interrupt on physical EP */
	base->UESTACLR[phys_ep] = USBC_UESTA0CLR_RXOUTIC;
	base->UECONSET[phys_ep] = USBC_UECON0SET_RXOUTES;

	/*
	 * Clear FIFOCON to free the bank and allow hardware to receive data.
	 * This signals hardware we're ready to receive packets.
	 * (Datasheet 17.6.2.16: "Clear FIFOCON to free the bank")
	 * For multi-packet transfers, hardware accumulates data until
	 * multi_packet_size is reached or a short packet is received.
	 */
	base->UECONCLR[phys_ep] = USBC_UECON0CLR_FIFOCONC;

	LOG_DBG("Prepare OUT ep 0x%02x (phys %d bank %d) buf=%p tailroom=%u",
		ep_cfg->addr, phys_ep, bank, (void *)buf->data,
		net_buf_tailroom(buf));

	return 0;
}

/* Prepare an IN transfer (EP0 only - bulk/interrupt use scatter-gather) */
static int sam_usbc_prep_in(const struct device *dev,
			    struct net_buf *const buf,
			    struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	volatile Usbc_t *base = config->base;
	const uint8_t phys_ep = virt_to_phys_ep(dev, ep_cfg->addr);
	struct sam_usbc_desc_table *dt = sam_usbc_get_desc(dev, phys_ep, 0);
	const uint16_t len = MIN(buf->len, ep_cfg->mps);

	/*
	 * Direct DMA from net_buf - no staging buffer needed.
	 * USB DMA can access any embedded RAM including net_buf memory.
	 */
	dt->ep_pipe_addr = buf->data;
	dt->sizes = 0;
	dt->udesc_sizes.byte_count = len;

	/* Track buffer for completion handling */
	priv->bank_buf[phys_ep][0] = buf;

	barrier_dsync_fence_full();

	if (phys_ep == 0) {
		/* Control endpoint: clear TXINI, enable interrupt */
		base->UESTACLR[phys_ep] = USBC_UESTA0CLR_TXINIC;
		base->UECONSET[phys_ep] = USBC_UECON0SET_TXINES;
	} else {
		/* Non-control: clear FIFOCON to send, enable TXIN interrupt */
		base->UESTACLR[phys_ep] = USBC_UESTA0CLR_TXINIC;
		base->UECONSET[phys_ep] = USBC_UECON0SET_TXINES;
		base->UECONCLR[phys_ep] = USBC_UECON0CLR_FIFOCONC;
	}

	LOG_DBG("Prepare IN ep 0x%02x (phys %d) len %u buf=%p",
		ep_cfg->addr, phys_ep, len, (void *)buf->data);

	return 0;
}

static int sam_usbc_ctrl_feed_dout(const struct device *dev, const size_t length)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct net_buf *buf;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	udc_buf_put(ep_cfg, buf);

	return sam_usbc_prep_out(dev, buf, ep_cfg);
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

static int sam_usbc_handle_evt_setup(const struct device *dev)
{
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	struct net_buf *buf;
	int err;

	drop_control_transfers(dev);

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, 8);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, priv->setup, sizeof(priv->setup));
	udc_ep_buf_set_setup(buf);

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		LOG_DBG("s:%p|feed for -out-", (void *)buf);
		err = sam_usbc_ctrl_feed_dout(dev, udc_data_stage_length(buf));
		if (err == -ENOMEM) {
			udc_submit_ep_event(dev, buf, err);
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

static int sam_usbc_handle_evt_din(const struct device *dev,
				   struct udc_ep_config *const ep_cfg)
{
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	const uint8_t phys_ep = virt_to_phys_ep(dev, ep_cfg->addr);
	struct sam_usbc_desc_table *dt = sam_usbc_get_desc(dev, phys_ep, 0);
	struct net_buf *buf;
	uint32_t len;

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for ep 0x%02x", ep_cfg->addr);
		return -ENOBUFS;
	}

	len = dt->udesc_sizes.byte_count;
	LOG_DBG("Handle IN ep 0x%02x (phys %d) byte_count %u", ep_cfg->addr,
		phys_ep, len);
	net_buf_pull(buf, len);

	if (buf->len) {
		/* More data to send */
		return sam_usbc_prep_in(dev, buf, ep_cfg);
	}

	if (udc_ep_buf_has_zlp(buf)) {
		udc_ep_buf_clear_zlp(buf);
		return sam_usbc_prep_in(dev, buf, ep_cfg);
	}

	buf = udc_buf_get(ep_cfg);
	udc_ep_set_busy(ep_cfg, false);
	priv->bank_buf[phys_ep][0] = NULL;

	if (ep_cfg->addr == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) ||
		    udc_ctrl_stage_is_no_data(dev)) {
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);

			/*
			 * CRITICAL: Restore EP0 OUT descriptor for next SETUP.
			 * In single-bank mode, IN and OUT share the same
			 * descriptor. sam_usbc_prep_in() set ep_pipe_addr to
			 * the IN buffer. We must restore it to the setup
			 * buffer before the next SETUP arrives.
			 * EP0 always uses physical EP0.
			 */
			dt->ep_pipe_addr = priv->setup;
			dt->sizes = 0;
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {
			/*
			 * IN transfer finished, need to receive OUT ZLP
			 * for status stage
			 */
			net_buf_unref(buf);
			return sam_usbc_ctrl_feed_dout(dev, 0);
		}

		return 0;
	}

	return udc_submit_ep_event(dev, buf, 0);
}

static int sam_usbc_handle_evt_dout(const struct device *dev,
				    struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	volatile Usbc_t *base = config->base;
	const uint8_t phys_ep = virt_to_phys_ep(dev, ep_cfg->addr);
	struct sam_usbc_desc_table *dt;
	struct net_buf *buf;
	uint32_t size;
	uint8_t bank = 0;
	int err = 0;

	/* EP0 control endpoint: single-bank, read from descriptor */
	dt = sam_usbc_get_desc(dev, phys_ep, 0);

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		LOG_ERR("No buffer for OUT ep 0x%02x", ep_cfg->addr);
		return -ENODATA;
	}

	size = dt->udesc_sizes.byte_count;
	LOG_DBG("Handle OUT ep 0x%02x (phys %d bank %d) byte_count %u",
		ep_cfg->addr, phys_ep, bank, size);

	/*
	 * Data already in net_buf via DMA - just update length.
	 * USB DMA wrote directly to buf->data.
	 */
	net_buf_add(buf, size);

	/* Check if we need more data */
	if (net_buf_tailroom(buf) >= ep_cfg->mps && size == ep_cfg->mps) {
		/* More data expected */
		if (phys_ep == 0) {
			/* Control OUT: prepare for next packet */
			base->UECONSET[phys_ep] = USBC_UECON0SET_RXOUTES;
		} else {
			/*
			 * Re-arm for more data. CURRBK now points to the
			 * other bank (after FIFOCON clear above), so prep_out
			 * will arm that bank.
			 */
			return sam_usbc_prep_out(dev, buf, ep_cfg);
		}
		return 0;
	}

	buf = udc_buf_get(ep_cfg);
	udc_ep_set_busy(ep_cfg, false);
	priv->bank_buf[phys_ep][bank] = NULL;

	if (ep_cfg->addr == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev)) {
			LOG_DBG("dout:%p|status, feed >s", (void *)buf);
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);

			/*
			 * CRITICAL: Restore EP0 OUT descriptor for next SETUP.
			 * After status OUT (ZLP from host), ensure descriptor
			 * is ready for the next SETUP packet.
			 * EP0 always uses physical EP0.
			 */
			dt->ep_pipe_addr = priv->setup;
			dt->sizes = 0;
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_in(dev)) {
			err = udc_ctrl_submit_s_out_status(dev, buf);
		}
	} else {
		err = udc_submit_ep_event(dev, buf, 0);
	}

	return err;
}

static void sam_usbc_handle_xfer_next(const struct device *dev,
				      struct udc_ep_config *const ep_cfg)
{
	struct net_buf *buf;
	int err;

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		return;
	}

	if (USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
		err = sam_usbc_prep_out(dev, buf, ep_cfg);
	} else {
		err = sam_usbc_prep_in(dev, buf, ep_cfg);
	}

	if (err != 0) {
		buf = udc_buf_get(ep_cfg);
		udc_submit_ep_event(dev, buf, -ECONNREFUSED);
	} else {
		udc_ep_set_busy(ep_cfg, true);
	}
}

static ALWAYS_INLINE void sam_usbc_thread_handler(const struct device *dev)
{
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	uint32_t evt;
	uint32_t eps;
	uint8_t ep;
	int err;

	evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);
	udc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(SAM_USBC_EVT_XFER_FINISHED)) {
		k_event_clear(&priv->events, BIT(SAM_USBC_EVT_XFER_FINISHED));

		eps = atomic_clear(&priv->xfer_finished);

		while (eps) {
			ep = udc_pull_ep_from_bmsk(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);
			LOG_DBG("Finished event ep 0x%02x", ep);

			if (USB_EP_DIR_IS_IN(ep)) {
				err = sam_usbc_handle_evt_din(dev, ep_cfg);
			} else {
				err = sam_usbc_handle_evt_dout(dev, ep_cfg);
			}

			if (err) {
				udc_submit_event(dev, UDC_EVT_ERROR, err);
			}

			if (!udc_ep_is_busy(ep_cfg)) {
				sam_usbc_handle_xfer_next(dev, ep_cfg);
			}
		}
	}

	if (evt & BIT(SAM_USBC_EVT_XFER_NEW)) {
		k_event_clear(&priv->events, BIT(SAM_USBC_EVT_XFER_NEW));

		eps = atomic_clear(&priv->xfer_new);

		while (eps) {
			ep = udc_pull_ep_from_bmsk(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);
			LOG_DBG("New transfer ep 0x%02x", ep);

			if (!udc_ep_is_busy(ep_cfg)) {
				sam_usbc_handle_xfer_next(dev, ep_cfg);
			}
		}
	}

	if (evt & BIT(SAM_USBC_EVT_SETUP)) {
		k_event_clear(&priv->events, BIT(SAM_USBC_EVT_SETUP));
		err = sam_usbc_handle_evt_setup(dev);
		if (err) {
			udc_submit_event(dev, UDC_EVT_ERROR, err);
		}
	}

	udc_unlock_internal(dev);
}

/* Handle SETUP packet reception in ISR */
static void sam_usbc_handle_setup_isr(const struct device *dev)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	volatile Usbc_t *base = config->base;
	struct sam_usbc_desc_table *dt = sam_usbc_get_desc(dev, 0, 0);

	LOG_DBG("SETUP ISR: dt_addr=%p setup=%p sizes=%08x byte_count=%u",
		(void *)dt->ep_pipe_addr, (void *)priv->setup,
		dt->sizes, dt->udesc_sizes.byte_count);

	if (dt->udesc_sizes.byte_count != 8) {
		LOG_ERR("Wrong byte count %u for setup packet "
			"(UECFG[0]=%08x UESTA[0]=%08x UECON[0]=%08x)",
			dt->udesc_sizes.byte_count,
			base->UECFG[0], base->UESTA[0], base->UECON[0]);
		LOG_ERR("Descriptor: addr=%p sizes=%08x bk_ctrl=%08x",
			(void *)dt->ep_pipe_addr, dt->sizes, dt->bk_ctrl_stat);
		LOG_HEXDUMP_ERR(priv->setup, 8, "setup[]:");
	}

	/*
	 * SETUP data already in priv->setup via DMA - no copy needed.
	 * EP0 descriptor points to priv->setup for SETUP reception.
	 */

	LOG_HEXDUMP_DBG(priv->setup, 8, "setup");

	/* Clear RXSTPI to ACK the setup packet */
	base->UESTACLR[0] = USBC_UESTA0CLR_RXSTPIC;

	k_event_post(&priv->events, BIT(SAM_USBC_EVT_SETUP));
}

/* Handle OUT packet reception in ISR */
static void sam_usbc_handle_out_isr(const struct device *dev, const uint8_t phys_ep)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	volatile Usbc_t *base = config->base;
	uint8_t virt_addr;

	/*
	 * Derive virtual endpoint address. For EP0 (control), EPDIR is always
	 * OUT in UECFG but the endpoint handles both directions. Since this is
	 * the OUT handler, we know the direction is OUT.
	 */
	if (phys_ep == 0) {
		virt_addr = USB_CONTROL_EP_OUT;
	} else {
		virt_addr = phys_ep_to_virt_addr(dev, phys_ep);
	}

	/* Clear RXOUTI interrupt on physical EP */
	base->UESTACLR[phys_ep] = USBC_UESTA0CLR_RXOUTIC;

	/*
	 * For EP0 (control) or single-bank mode:
	 * Do NOT clear FIFOCON here. Keep the bank "full" so hardware NAKs
	 * incoming packets until we're ready with a new buffer.
	 * FIFOCON is cleared in prep_out() when the next buffer is queued.
	 */

	/* Signal thread to handle the data using virtual address */
	atomic_set_bit(&priv->xfer_finished, udc_ep_to_bnum(virt_addr));
	k_event_post(&priv->events, BIT(SAM_USBC_EVT_XFER_FINISHED));
}

/* Handle IN transfer complete in ISR */
static void sam_usbc_handle_in_isr(const struct device *dev, const uint8_t phys_ep)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	volatile Usbc_t *base = config->base;
	uint8_t virt_addr;

	/*
	 * Derive virtual endpoint address. For EP0 (control), EPDIR is always
	 * OUT in UECFG but the endpoint handles both directions. Since this is
	 * the IN handler, we know the direction is IN.
	 */
	if (phys_ep == 0) {
		virt_addr = USB_CONTROL_EP_IN;
	} else {
		virt_addr = phys_ep_to_virt_addr(dev, phys_ep);
	}

	/* Disable TXINI interrupt on physical EP */
	base->UECONCLR[phys_ep] = USBC_UECON0CLR_TXINEC;

	/* Signal thread to handle completion using virtual address */
	atomic_set_bit(&priv->xfer_finished, udc_ep_to_bnum(virt_addr));
	k_event_post(&priv->events, BIT(SAM_USBC_EVT_XFER_FINISHED));
}

/* Handle endpoint interrupt */
static void sam_usbc_handle_ep_isr(const struct device *dev, const uint8_t ep_idx)
{
	const struct udc_sam_usbc_config *config = dev->config;
	volatile Usbc_t *base = config->base;
	uint32_t sr = base->UESTA[ep_idx];
	uint32_t con = base->UECON[ep_idx];

	if (IS_ENABLED(CONFIG_UDC_DRIVER_LOG_LEVEL_DBG)) {
		sam_usbc_ep_isr_sta(base, ep_idx);
	}

	if (ep_idx == 0 && (sr & USBC_UESTA0_RXSTPI)) {
		sam_usbc_handle_setup_isr(dev);
		return;
	}

	if ((sr & USBC_UESTA0_RXOUTI) && (con & USBC_UECON0_RXOUTE)) {
		sam_usbc_handle_out_isr(dev, ep_idx);
	}

	if ((sr & USBC_UESTA0_TXINI) && (con & USBC_UECON0_TXINE)) {
		sam_usbc_handle_in_isr(dev, ep_idx);
	}
}

/* Main ISR handler */
static void sam_usbc_isr_handler(const struct device *dev)
{
	const struct udc_sam_usbc_config *config = dev->config;
	volatile Usbc_t *base = config->base;
	uint32_t sr = base->UDINT;
	uint32_t inte = base->UDINTE;

	/* SOF interrupt */
	if (IS_ENABLED(CONFIG_UDC_ENABLE_SOF) &&
	    (sr & USBC_UDINT_SOF) && (inte & USBC_UDINTE_SOFE)) {
		base->UDINTCLR = USBC_UDINTCLR_SOFC;
		udc_submit_sof_event(dev);
	}

	/* Endpoint interrupts (EP0-EP7) */
	for (uint8_t ep_idx = 0; ep_idx < config->num_of_eps; ep_idx++) {
		if (sr & (USBC_UDINT_EP0INT << ep_idx)) {
			sam_usbc_handle_ep_isr(dev, ep_idx);
		}
	}

	/* End of Resume */
	if ((sr & USBC_UDINT_EORSM) && (inte & USBC_UDINTE_EORSME)) {
		LOG_DBG("ISR: End Of Resume");
		base->UDINTCLR = USBC_UDINTCLR_EORSMC;
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}

	/* End of Reset */
	if ((sr & USBC_UDINT_EORST) && (inte & USBC_UDINTE_EORSTE)) {
		struct udc_sam_usbc_data *priv = udc_get_private(dev);
		struct sam_usbc_desc_table *dt;

		LOG_DBG("ISR: End Of Reset");

		base->UDINTCLR = USBC_UDINTCLR_EORSTC;

		/*
		 * After USB reset, UECFG registers are cleared by hardware.
		 * We must reconfigure EP0 immediately before host sends SETUP.
		 */

		/* Re-enable EP0 in endpoint reset register */
		base->UERST |= BIT(USBC_UERST_EPEN0_Pos);

		/* Configure EP0 as control endpoint, 64 bytes, single bank */
		base->UECFG[0] = USBC_UECFG0_EPTYPE(0) |
				 USBC_UECFG0_EPSIZE(3);

		/* Re-initialize EP0 OUT descriptor with setup buffer */
		dt = sam_usbc_get_desc(dev, 0, 0);
		dt->ep_pipe_addr = priv->setup;
		dt->sizes = 0;
		dt->bk_ctrl_stat = 0;

		/* Enable EP0 interrupt at device level */
		base->UDINTESET = USBC_UDINTESET_EP0INTES;

		/* Enable SETUP packet reception */
		base->UECONSET[0] = USBC_UECON0SET_RXSTPES;

		LOG_DBG("EP0 reconfigured: UECFG[0]=%08x UECON[0]=%08x UERST=%08x",
			base->UECFG[0], base->UECON[0], base->UERST);

		/* Clear debug state after reset */
		if (IS_ENABLED(CONFIG_UDC_DRIVER_LOG_LEVEL_DBG)) {
			sam_usbc_clean_sta_dbg();
		}

		udc_submit_event(dev, UDC_EVT_RESET, 0);
	}

	/* Suspend */
	if ((sr & USBC_UDINT_SUSP) && (inte & USBC_UDINTE_SUSPE)) {
		LOG_DBG("ISR: Suspend");
		base->UDINTCLR = USBC_UDINTCLR_SUSPC;

		/* Freeze clock for power saving */
		sam_usbc_unfreeze_clk(base);
		while (!(base->USBSTA & USBC_USBSTA_CLKUSABLE)) {
			;
		}

		base->UDINTECLR = USBC_UDINTECLR_SUSPEC;
		base->UDINTCLR = USBC_UDINTCLR_WAKEUPC;
		base->UDINTESET = USBC_UDINTESET_WAKEUPES;
		sam_usbc_freeze_clk(base);

		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	}

	/* Wakeup */
	if ((sr & USBC_UDINT_WAKEUP) && (inte & USBC_UDINTE_WAKEUPE)) {
		LOG_DBG("ISR: Wake Up");
		base->UDINTCLR = USBC_UDINTCLR_WAKEUPC;

		sam_usbc_unfreeze_clk(base);
		while (!(base->USBSTA & USBC_USBSTA_CLKUSABLE)) {
			;
		}

		base->UDINTECLR = USBC_UDINTECLR_WAKEUPEC;
		base->UDINTCLR = USBC_UDINTCLR_SUSPC;
		base->UDINTESET = USBC_UDINTESET_SUSPES;
	}

	barrier_dmem_fence_full();
}

/* UDC API: Enqueue a transfer */
static int udc_sam_usbc_ep_enqueue(const struct device *dev,
				   struct udc_ep_config *const ep_cfg,
				   struct net_buf *buf)
{
	struct udc_sam_usbc_data *priv = udc_get_private(dev);

	LOG_DBG("%s enqueue 0x%02x %p", dev->name, ep_cfg->addr, (void *)buf);
	udc_buf_put(ep_cfg, buf);

	if (!ep_cfg->stat.halted) {
		atomic_set_bit(&priv->xfer_new, udc_ep_to_bnum(ep_cfg->addr));
		k_event_post(&priv->events, BIT(SAM_USBC_EVT_XFER_NEW));
	}

	return 0;
}

/* UDC API: Dequeue transfers */
static int udc_sam_usbc_ep_dequeue(const struct device *dev,
				   struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbc_config *config = dev->config;
	volatile Usbc_t *base = config->base;
	const uint8_t phys_ep = virt_to_phys_ep(dev, ep_cfg->addr);
	unsigned int lock_key;
	struct net_buf *buf;

	lock_key = irq_lock();

	/*
	 * Disable endpoint interrupts if physical EP is still allocated.
	 * The physical EP may already be freed by ep_disable, but we still
	 * need to dequeue pending buffers below.
	 */
	if (phys_ep < NUM_OF_EP_MAX) {
		if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
			base->UECONCLR[phys_ep] = USBC_UECON0CLR_TXINEC;
		} else {
			base->UECONCLR[phys_ep] = USBC_UECON0CLR_RXOUTEC;
		}
	}

	/* Always dequeue pending buffers and notify class layer */
	buf = udc_buf_get_all(ep_cfg);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
		udc_ep_set_busy(ep_cfg, false);
	}

	irq_unlock(lock_key);

	return 0;
}

/* UDC API: Enable endpoint */
static int udc_sam_usbc_ep_enable(const struct device *dev,
				  struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	volatile Usbc_t *base = config->base;
	const uint8_t virt_ep_num = USB_EP_GET_IDX(ep_cfg->addr);
	struct sam_usbc_desc_table *dt;
	uint32_t uecfg = 0;
	uint8_t ep_type;
	int phys_ep;

	/* Map USB EP type to hardware type */
	switch (ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		ep_type = 0; /* Control */
		break;
	case USB_EP_TYPE_ISO:
		ep_type = 1; /* Isochronous */
		break;
	case USB_EP_TYPE_BULK:
		ep_type = 2; /* Bulk */
		break;
	case USB_EP_TYPE_INTERRUPT:
		ep_type = 3; /* Interrupt */
		break;
	default:
		return -EINVAL;
	}

	/*
	 * Allocate a physical endpoint for this virtual endpoint.
	 * If the same virtual EP number is already used in the other direction,
	 * REPNB redirection will be configured to use a different physical EP.
	 */
	phys_ep = alloc_phys_ep(dev, ep_cfg->addr);
	if (phys_ep < 0) {
		return phys_ep;
	}

	/* Configure endpoint type and size */
	uecfg = USBC_UECFG0_EPTYPE(ep_type);
	uecfg |= USBC_UECFG0_EPSIZE(sam_usbc_get_epsize(ep_cfg->mps));

	/* Set direction for non-control endpoints */
	if (virt_ep_num != 0 && USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		uecfg |= USBC_UECFG0_EPDIR;
	}

	/*
	 * Configure REPNB if physical EP differs from virtual EP number.
	 * REPNB redirects USB transactions for virtual EP m to physical EP n.
	 * REPNB should not be used for control endpoints (EP0).
	 */
	if (phys_ep != virt_ep_num && virt_ep_num != 0) {
		uecfg |= (virt_ep_num << 16);  /* REPNB field bits 19:16 */
		LOG_DBG("REPNB: phys EP%d redirects virt EP%d", phys_ep,
			virt_ep_num);
	}

	/* Single bank mode (EPBK = 0) */

	/* Initialize Bank0 descriptor */
	dt = sam_usbc_get_desc(dev, phys_ep, 0);
	if (phys_ep == 0 && USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
		/* EP0 OUT: point to setup buffer for SETUP packet reception */
		dt->ep_pipe_addr = priv->setup;
	}
	dt->sizes = 0;
	dt->bk_ctrl_stat = 0;
	dt->ep_ctrl_stat = 0;

	/* Enable physical endpoint */
	base->UERST |= BIT(USBC_UERST_EPEN0_Pos + phys_ep);
	base->UECFG[phys_ep] = uecfg;

	/* Enable endpoint interrupt at device level */
	base->UDINTESET = USBC_UDINTESET_EP0INTES << phys_ep;

	/* Setup control endpoint for SETUP packets */
	if (phys_ep == 0 && USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
		base->UECONSET[0] = USBC_UECON0SET_RXSTPES;
	}

	LOG_DBG("Enable ep 0x%02x -> phys EP%d UECFG=%08x",
		ep_cfg->addr, phys_ep, uecfg);

	return 0;
}

/* UDC API: Disable endpoint */
static int udc_sam_usbc_ep_disable(const struct device *dev,
				   struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	volatile Usbc_t *base = config->base;
	uint8_t phys_ep = virt_to_phys_ep(dev, ep_cfg->addr);

	if (phys_ep >= NUM_OF_EP_MAX) {
		LOG_WRN("Disable ep 0x%02x: not allocated", ep_cfg->addr);
		return 0;
	}

	/* Disable endpoint interrupt at device level */
	base->UDINTECLR = USBC_UDINTECLR_EP0INTEC << phys_ep;

	/* Disable physical endpoint */
	base->UERST &= ~BIT(USBC_UERST_EPEN0_Pos + phys_ep);

	/* Clear bank buffer tracking */
	priv->bank_buf[phys_ep][0] = NULL;
	priv->bank_buf[phys_ep][1] = NULL;

	/* Free the physical endpoint allocation */
	free_phys_ep(dev, ep_cfg->addr);

	LOG_DBG("Disable ep 0x%02x (was phys EP%d)", ep_cfg->addr, phys_ep);

	return 0;
}

/* UDC API: Set endpoint halt (STALL) */
static int udc_sam_usbc_ep_set_halt(const struct device *dev,
				    struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbc_config *config = dev->config;
	volatile Usbc_t *base = config->base;
	const uint8_t phys_ep = virt_to_phys_ep(dev, ep_cfg->addr);

	if (phys_ep >= NUM_OF_EP_MAX) {
		return -ENOENT;
	}

	/* Set STALLRQ first */
	base->UECONSET[phys_ep] = USBC_UECON0SET_STALLRQS;

	LOG_DBG("Set halt ep 0x%02x (phys %d)", ep_cfg->addr, phys_ep);

	if (phys_ep != 0) {
		ep_cfg->stat.halted = true;
	}

	return 0;
}

/* UDC API: Clear endpoint halt */
static int udc_sam_usbc_ep_clear_halt(const struct device *dev,
				      struct udc_ep_config *const ep_cfg)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	volatile Usbc_t *base = config->base;
	const uint8_t phys_ep = virt_to_phys_ep(dev, ep_cfg->addr);

	if (phys_ep >= NUM_OF_EP_MAX || phys_ep == 0) {
		return 0;
	}

	/* Clear stall request and reset data toggle on physical EP */
	base->UECONCLR[phys_ep] = USBC_UECON0CLR_STALLRQC;
	base->UECONSET[phys_ep] = USBC_UECON0SET_RSTDTS;

	ep_cfg->stat.halted = false;

	/* Resume any pending transfers if no buffer currently armed */
	if (!udc_ep_is_busy(ep_cfg) && udc_buf_peek(ep_cfg)) {
		atomic_set_bit(&priv->xfer_new, udc_ep_to_bnum(ep_cfg->addr));
		k_event_post(&priv->events, BIT(SAM_USBC_EVT_XFER_NEW));
	}

	LOG_DBG("Clear halt ep 0x%02x (phys %d)", ep_cfg->addr, phys_ep);

	return 0;
}

/* UDC API: Set device address */
static int udc_sam_usbc_set_address(const struct device *dev, const uint8_t addr)
{
	const struct udc_sam_usbc_config *config = dev->config;
	volatile Usbc_t *base = config->base;

	LOG_DBG("Set address %u for %s", addr, dev->name);

	if (addr != 0) {
		/* Set address and enable it */
		base->UDCON = (base->UDCON & ~USBC_UDCON_UADD_Msk) |
			      USBC_UDCON_UADD(addr) | USBC_UDCON_ADDEN;
	} else {
		base->UDCON &= ~(USBC_UDCON_UADD_Msk | USBC_UDCON_ADDEN);
	}

	return 0;
}

/* UDC API: Host wakeup (remote wakeup) */
static int udc_sam_usbc_host_wakeup(const struct device *dev)
{
	const struct udc_sam_usbc_config *config = dev->config;
	volatile Usbc_t *base = config->base;

	LOG_DBG("Remote wakeup from %s", dev->name);

	base->UDCON |= USBC_UDCON_RMWKUP;

	return 0;
}

/* UDC API: Get device speed */
static enum udc_bus_speed udc_sam_usbc_device_speed(const struct device *dev)
{
	/* SAM4L USBC only supports Full-Speed */
	return UDC_BUS_SPEED_FS;
}

/* UDC API: Enable controller */
static int udc_sam_usbc_enable(const struct device *dev)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	volatile Usbc_t *base = config->base;
	uint32_t pmcon;
	int ret;

	/* Enable USBC asynchronous wake-up source */
	PM->AWEN |= BIT(PM_AWEN_USBC);

	/* Always authorize asynchronous USB interrupts to exit of sleep mode */
	pmcon = BPM->PMCON | BPM_PMCON_FASTWKUP;
	BPM->UNLOCK = BPM_UNLOCK_KEY(0xAAu) |
		      BPM_UNLOCK_ADDR((uint32_t)&BPM->PMCON - (uint32_t)BPM);
	BPM->PMCON = pmcon;

	/* Enable USB clocks (PBB for regs, HSB for data) */
	soc_pmc_peripheral_enable(
		PM_CLOCK_MASK(PM_CLK_GRP_PBB, SYSCLK_USBC_REGS));
	soc_pmc_peripheral_enable(
		PM_CLOCK_MASK(PM_CLK_GRP_HSB, SYSCLK_USBC_DATA));

	/* Enable USB Generic clock */
	SCIF->GCCTRL[GEN_CLK_USBC] = 0;
	SCIF->GCCTRL[GEN_CLK_USBC] = SCIF_GCCTRL_OSCSEL(SCIF_GC_USES_CLK_HSB) |
				     SCIF_GCCTRL_CEN;

	/* Wait for clock to be usable */
	while (!(base->USBSTA & USBC_USBSTA_CLKUSABLE)) {
		;
	}

	/* Configure USB pad */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply pinctrl state (%d)", ret);
		return ret;
	}

	/* Enable USB controller in device mode */
	base->USBCON = USBC_USBCON_UIMOD | USBC_USBCON_USBE;
	sam_usbc_unfreeze_clk(base);

	/* Set descriptor address */
	base->UDESC = (uint32_t)config->dt;

	LOG_DBG("Descriptor table at %p (UDESC=%08x)",
		(void *)config->dt, base->UDESC);

	/* Configure for full-speed (LS=0), keep detached initially */
	base->UDCON = USBC_UDCON_DETACH;

	/* Reset bank buffer tracking and physical EP allocation */
	sam_usbc_bank_buf_reset(dev);
	sam_usbc_phys_ep_reset(dev);

	/* Enable control endpoints */
	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				   USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control OUT endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				   USB_EP_TYPE_CONTROL, 64, 0)) {
		LOG_ERR("Failed to enable control IN endpoint");
		return -EIO;
	}

	{
		struct sam_usbc_desc_table *dt = sam_usbc_get_desc(dev, 0, 0);

		LOG_DBG("EP0 desc: addr=%p setup=%p sizes=%08x",
			(void *)dt->ep_pipe_addr, (void *)priv->setup,
			dt->sizes);
		LOG_DBG("EP0 regs: UECFG[0]=%08x UECON[0]=%08x UERST=%08x",
			base->UECFG[0], base->UECON[0], base->UERST);
	}

	/* Clear and enable device interrupts */
	base->UDINTCLR = USBC_UDINTCLR_EORSMC |
			 USBC_UDINTCLR_EORSTC |
			 USBC_UDINTCLR_SOFC |
			 USBC_UDINTCLR_SUSPC |
			 USBC_UDINTCLR_WAKEUPC;

	base->UDINTESET = USBC_UDINTESET_EORSMES |
			  USBC_UDINTESET_EORSTES |
			  USBC_UDINTESET_SUSPES |
			  USBC_UDINTESET_WAKEUPES |
			  (IS_ENABLED(CONFIG_UDC_ENABLE_SOF) ? USBC_UDINTESET_SOFES : 0);

	config->irq_enable_func(dev);

	/* Attach device (clear DETACH) */
	base->UDCON &= ~USBC_UDCON_DETACH;

	LOG_DBG("Enable device %s", dev->name);

	return 0;
}

/* UDC API: Disable controller */
static int udc_sam_usbc_disable(const struct device *dev)
{
	const struct udc_sam_usbc_config *config = dev->config;
	volatile Usbc_t *base = config->base;

	config->irq_disable_func(dev);

	/* Detach device */
	base->UDCON |= USBC_UDCON_DETACH;

	/* Disable control endpoints */
	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control OUT endpoint");
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control IN endpoint");
	}

	/* Reset bank buffer tracking and physical EP allocation */
	sam_usbc_bank_buf_reset(dev);
	sam_usbc_phys_ep_reset(dev);

	/* Disable USB controller and freeze clock */
	base->USBCON = USBC_USBCON_UIMOD | USBC_USBCON_FRZCLK;

	/* Disable USB Generic clock */
	SCIF->GCCTRL[GEN_CLK_USBC] = 0;

	/* Disable async wakeup source */
	PM->AWEN &= ~BIT(PM_AWEN_USBC);

	/* Disable USB clocks */
	soc_pmc_peripheral_disable(
		PM_CLOCK_MASK(PM_CLK_GRP_HSB, SYSCLK_USBC_DATA));
	soc_pmc_peripheral_disable(
		PM_CLOCK_MASK(PM_CLK_GRP_PBB, SYSCLK_USBC_REGS));

	LOG_DBG("Disable device %s", dev->name);

	return 0;
}

/* UDC API: Initialize controller */
static int udc_sam_usbc_init(const struct device *dev)
{
	LOG_DBG("Init device %s", dev->name);

	return 0;
}

/* UDC API: Shutdown controller */
static int udc_sam_usbc_shutdown(const struct device *dev)
{
	LOG_DBG("Shutdown device %s", dev->name);

	return 0;
}

/* UDC API: Lock */
static void udc_sam_usbc_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

/* UDC API: Unlock */
static void udc_sam_usbc_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
	k_sched_unlock();
}

/* Driver preinit - called at boot */
static int udc_sam_usbc_driver_preinit(const struct device *dev)
{
	const struct udc_sam_usbc_config *config = dev->config;
	struct udc_sam_usbc_data *priv = udc_get_private(dev);
	struct udc_data *data = dev->data;
	uint16_t mps = 1023;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);

	/* Initialize physical EP tracking to "not allocated" */
	priv->phys_ep_used = 0;
	memset(priv->virt_to_phys, NOT_ALLOC_VIRT_EP, sizeof(priv->virt_to_phys));

	data->caps.rwup = true;
	data->caps.mps0 = UDC_MPS0_64;

	for (int i = 0; i < config->num_of_eps; i++) {
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

	for (int i = 0; i < config->num_of_eps; i++) {
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

	return 0;
}

static const struct udc_api udc_sam_usbc_api = {
	.lock = udc_sam_usbc_lock,
	.unlock = udc_sam_usbc_unlock,
	.device_speed = udc_sam_usbc_device_speed,
	.init = udc_sam_usbc_init,
	.enable = udc_sam_usbc_enable,
	.disable = udc_sam_usbc_disable,
	.shutdown = udc_sam_usbc_shutdown,
	.set_address = udc_sam_usbc_set_address,
	.host_wakeup = udc_sam_usbc_host_wakeup,
	.ep_enable = udc_sam_usbc_ep_enable,
	.ep_disable = udc_sam_usbc_ep_disable,
	.ep_set_halt = udc_sam_usbc_ep_set_halt,
	.ep_clear_halt = udc_sam_usbc_ep_clear_halt,
	.ep_enqueue = udc_sam_usbc_ep_enqueue,
	.ep_dequeue = udc_sam_usbc_ep_dequeue,
};

#define UDC_SAM_USBC_PINCTRL_DT_INST_DEFINE(n)					 \
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),			 \
		    (PINCTRL_DT_INST_DEFINE(n)), ())

#define UDC_SAM_USBC_PINCTRL_DT_INST_DEV_CONFIG_GET(n)				 \
	COND_CODE_1(DT_INST_PINCTRL_HAS_NAME(n, default),			 \
		    ((void *)PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL))

#define UDC_SAM_USBC_DEVICE_DEFINE(n)						 \
	UDC_SAM_USBC_PINCTRL_DT_INST_DEFINE(n);					 \
										 \
	static void udc_sam_usbc_irq_enable_func_##n(const struct device *dev)	 \
	{									 \
		IRQ_CONNECT(DT_INST_IRQN(n),					 \
			    DT_INST_IRQ(n, priority),				 \
			    sam_usbc_isr_handler,				 \
			    DEVICE_DT_INST_GET(n), 0);				 \
		irq_enable(DT_INST_IRQN(n));					 \
	}									 \
										 \
	static void udc_sam_usbc_irq_disable_func_##n(const struct device *dev)	 \
	{									 \
		irq_disable(DT_INST_IRQN(n));					 \
	}									 \
										 \
	K_THREAD_STACK_DEFINE(udc_sam_usbc_stack_##n,				 \
			      CONFIG_UDC_SAM_USBC_STACK_SIZE);			 \
										 \
	static void udc_sam_usbc_thread_##n(void *dev, void *arg1, void *arg2)	 \
	{									 \
		while (true) {							 \
			sam_usbc_thread_handler(dev);				 \
		}								 \
	}									 \
										 \
	static void udc_sam_usbc_make_thread_##n(const struct device *dev)	 \
	{									 \
		struct udc_sam_usbc_data *priv = udc_get_private(dev);		 \
										 \
		k_thread_create(&priv->thread_data,				 \
				udc_sam_usbc_stack_##n,				 \
				K_THREAD_STACK_SIZEOF(udc_sam_usbc_stack_##n),	 \
				udc_sam_usbc_thread_##n,			 \
				(void *)dev, NULL, NULL,			 \
				K_PRIO_COOP(CONFIG_UDC_SAM_USBC_THREAD_PRIORITY),\
				K_ESSENTIAL,					 \
				K_NO_WAIT);					 \
		k_thread_name_set(&priv->thread_data, dev->name);		 \
	}									 \
										 \
	static struct sam_usbc_desc_table					 \
		sam_usbc_dt_##n[DT_INST_PROP(n, num_bidir_endpoints) * 2]	 \
		__aligned(16);							 \
										 \
	static struct udc_ep_config						 \
		ep_cfg_out_##n[DT_INST_PROP(n, num_bidir_endpoints)];		 \
	static struct udc_ep_config						 \
		ep_cfg_in_##n[DT_INST_PROP(n, num_bidir_endpoints)];		 \
										 \
	static const struct udc_sam_usbc_config udc_sam_usbc_config_##n = {	 \
		.base = (volatile Usbc_t *)DT_INST_REG_ADDR(n),			 \
		.dt = sam_usbc_dt_##n,						 \
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),		 \
		.ep_cfg_in = ep_cfg_in_##n,					 \
		.ep_cfg_out = ep_cfg_out_##n,					 \
		.pcfg = UDC_SAM_USBC_PINCTRL_DT_INST_DEV_CONFIG_GET(n),		 \
		.irq_enable_func = udc_sam_usbc_irq_enable_func_##n,		 \
		.irq_disable_func = udc_sam_usbc_irq_disable_func_##n,		 \
		.make_thread = udc_sam_usbc_make_thread_##n,			 \
	};									 \
										 \
	static struct udc_sam_usbc_data udc_priv_##n;				 \
										 \
	static struct udc_data udc_data_##n = {					 \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		 \
		.priv = &udc_priv_##n,						 \
	};									 \
										 \
	DEVICE_DT_INST_DEFINE(n, udc_sam_usbc_driver_preinit, NULL,		 \
			      &udc_data_##n, &udc_sam_usbc_config_##n,		 \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	 \
			      &udc_sam_usbc_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_SAM_USBC_DEVICE_DEFINE)
