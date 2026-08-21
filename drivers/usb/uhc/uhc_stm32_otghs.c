/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Narek Aydinyan
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_otghs

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usb_ch9.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/uhc.h>
#include <soc.h>
#include <stm32cube_hal.h>
#include <stm32_bitops.h>
#include <stm32_usb_common.h>

#include "uhc_common.h"
#include "uhc_stm32_otghs.h"

LOG_MODULE_REGISTER(uhc_stm32_otghs, CONFIG_UHC_DRIVER_LOG_LEVEL);

/* Constants */

#define USB_EP0_MPS_8  8u
#define USB_EP0_MPS_16 16u
#define USB_EP0_MPS_32 32u
#define USB_EP0_MPS_64 64u

#define STM32_OTGHS_EP_INDEX_MASK       0x0Fu
#define STM32_OTGHS_DMA_ALIGNMENT       4u

#define STM32_OTGHS_VBUS_SETTLE_US      2000u
#define STM32_OTGHS_POST_START_DELAY_MS 20u
#define STM32_OTGHS_POST_RESET_DELAY_MS 20u
#define STM32_OTGHS_RESET_FALLBACK_MS   250u

/*
 * Retry only HAL-parked NAK_WAIT channels from software.
 * 1 ms is a good starting point for USB-Ethernet receive idle periods.
 */
#define STM32_OTGHS_NAK_WAIT_RETRY_MS 1u

/*
 * Periodic wakeups are only used to re-wake the worker for HAL-parked
 * NAK_WAIT channels. When SOF support is enabled, the driver can use the
 * 1 kHz SOF interrupt instead of this timer.
 */
#define STM32_OTGHS_KICK_PERIOD_MS 1u

#define STM32_OTGHS_THREAD_PRIO_DEFAULT 14
#define STM32_OTGHS_STACK_SIZE_DEFAULT  4096u

#define STM32_MPS_DEFAULT_BULK 512u
#define STM32_MPS_DEFAULT_INTR 16u

/* Fallback for some HALs which lack these */
#ifndef CH_OUT_DIR
#define CH_OUT_DIR 0U
#endif

#ifndef CH_IN_DIR
#define CH_IN_DIR 1U
#endif

/*
 * HAL_HCD_HC_SubmitRequest() takes a token selector, not the raw HC_PID_* value:
 * 0 selects SETUP, any nonzero value selects DATA.
 */
#define STM32_HCD_TOKEN_SETUP 0U
#define STM32_HCD_TOKEN_DATA  1U

#if defined(USE_HAL_HCD_IN_NAK_AUTO_ACTIVATE_DISABLE) &&                                       \
	(USE_HAL_HCD_IN_NAK_AUTO_ACTIVATE_DISABLE == 1) && defined(HAL_HCD_CHANNEL_NAK_COUNT) &&   \
	(HAL_HCD_CHANNEL_NAK_COUNT >= 1)
#define STM32_OTGHS_HAL_IN_NAK_LIMITER_AVAILABLE 1
#define STM32_OTGHS_HAL_IN_NAK_WAIT_LIMIT        HAL_HCD_CHANNEL_NAK_COUNT
#else
#define STM32_OTGHS_HAL_IN_NAK_LIMITER_AVAILABLE 0
#define STM32_OTGHS_HAL_IN_NAK_WAIT_LIMIT        0u
#endif

#if !IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA) && !STM32_OTGHS_HAL_IN_NAK_LIMITER_AVAILABLE
/*
 * Non-DMA builds require STM32 HAL support for the IN NAK limiter path.
 * The limiter parks IN BULK/CTRL channels as URB_NAK_WAIT after repeated NAKs,
 * then this driver retries from software at a lower rate. OUT retry handling
 * remains managed by the HAL. DMA builds do not require this path for direct
 * HS BULK/CTRL IN traffic.
 */
#error "Non-DMA STM32 OTGHS build requires HAL IN NAK limiter support."
#endif

#if IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA)
#define STM32_OTGHS_DIRECT_IN_NAK_PARKING 0
#else
#define STM32_OTGHS_DIRECT_IN_NAK_PARKING STM32_OTGHS_HAL_IN_NAK_LIMITER_AVAILABLE
#endif

#define STM32_EP_GET_IDX(ep) (USB_EP_GET_IDX(ep) & STM32_OTGHS_EP_INDEX_MASK)

/* Common helpers */

static void uhc_stm32_wake_worker_thread(struct stm32_otghs_data *d)
{
	k_event_post(&d->events, STM32_EVT_KICK);
}

static void uhc_stm32_signal_event_to_worker(HCD_HandleTypeDef *hhcd, uint32_t bit)
{
	struct stm32_otghs_data *d = hhcd->pData;

	__ASSERT_NO_MSG(d != NULL);

	k_event_post(&d->events, bit | STM32_EVT_KICK);
}

static void uhc_stm32_pipe_clear(struct stm32_pipe *p)
{
	p->xfer = NULL;
	p->hc = -1;
	p->ep = 0;
	p->len = 0;
	p->buf_off = 0;
	p->halted_cnt = 0;
	p->retry_timepoint = sys_timepoint_calc(K_NO_WAIT);
	memset(p->setup_words, 0, sizeof(p->setup_words));
}

static void uhc_stm32_hc_act_clear(struct stm32_hc_active *a)
{
	a->xfer = NULL;
	a->ep = 0;
	a->ep_type = 0;
	a->dir_in = 0;
	a->req_len = 0;
	a->buf_off = 0;
	a->halted_cnt = 0;
	a->mps = 0;
	a->retry_timepoint = sys_timepoint_calc(K_NO_WAIT);
}

static void uhc_stm32_hc_act_clear_all(struct stm32_otghs_data *d)
{
	for (uint8_t i = 0; i < d->host_channels; i++) {
		uhc_stm32_hc_act_clear(&d->hc_act[i]);
	}
}

static bool uhc_stm32_xfer_is_active(struct stm32_otghs_data *d, struct uhc_transfer *x)
{
	for (uint8_t i = 0; i < d->host_channels; i++) {
		if (d->hc_act[i].xfer == x) {
			return true;
		}
	}
	return false;
}

static int uhc_stm32_find_active_hc_for_xfer(struct stm32_otghs_data *d, struct uhc_transfer *x)
{
	for (uint8_t i = 0; i < d->host_channels; i++) {
		if (d->hc_act[i].xfer == x) {
			return (int)i;
		}
	}
	return -ENOENT;
}

static bool uhc_stm32_urb_waiting_on_device(HCD_URBStateTypeDef urb_state)
{
	return (urb_state == URB_NOTREADY) || (urb_state == URB_NYET) || (urb_state == URB_IDLE);
}

static bool uhc_stm32_any_nak_wait(struct stm32_otghs_data *d)
{
	if ((d->ctrl.xfer != NULL) && (d->ctrl.hc >= 0)) {
		if (HAL_HCD_HC_GetURBState(&d->hhcd, (uint8_t)d->ctrl.hc) == URB_NAK_WAIT) {
			return true;
		}
	}

	for (uint8_t i = 0; i < d->host_channels; i++) {
		if ((d->hc_act[i].xfer != NULL) &&
		    (HAL_HCD_HC_GetURBState(&d->hhcd, i) == URB_NAK_WAIT)) {
			return true;
		}
	}

	return false;
}

static void uhc_stm32_pipe_finish(const struct device *dev, struct stm32_pipe *p, int status)
{
	struct uhc_transfer *completed = p->xfer;

	uhc_stm32_pipe_clear(p);
	if (completed != NULL) {
		uhc_xfer_return(dev, completed, status);
	}
}

static void uhc_stm32_active_xfer_finish(const struct device *dev, struct stm32_hc_active *a,
				     int status)
{
	struct uhc_transfer *completed = a->xfer;

	uhc_stm32_hc_act_clear(a);
	if (completed != NULL) {
		uhc_xfer_return(dev, completed, status);
	}
}

static void uhc_stm32_hc_halt(struct stm32_otghs_data *d, uint8_t hc)
{
	const stm32_status_t status = HAL_HCD_HC_Halt(&d->hhcd, hc);

	if (status != HAL_OK) {
		LOG_DBG("HAL_HCD_HC_Halt(%u) failed: %d", (unsigned int)hc, (int)status);
	}
}

static void uhc_stm32_pipe_cancel_and_return(const struct device *dev, struct stm32_otghs_data *d,
				   struct stm32_pipe *p, int status)
{
	if (p->xfer != NULL) {
		if (p->hc >= 0) {
			uhc_stm32_hc_halt(d, (uint8_t)p->hc);
		}
		uhc_stm32_pipe_finish(dev, p, status);
	}
}

static int uhc_stm32_hc_submit_request(struct stm32_otghs_data *d, uint8_t hc, uint8_t direction,
				   uint8_t ep_type, uint8_t token, uint8_t *data_ptr,
				   uint16_t transfer_len)
{
	if (IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA) && (data_ptr != NULL) &&
	    !IS_ALIGNED((uintptr_t)data_ptr, STM32_OTGHS_DMA_ALIGNMENT)) {
		LOG_ERR("DMA buffer for HC %u is not %u-byte aligned: %p", (unsigned int)hc,
			STM32_OTGHS_DMA_ALIGNMENT, data_ptr);
		return -EINVAL;
	}

	const stm32_status_t status = HAL_HCD_HC_SubmitRequest(
		&d->hhcd, hc, direction, ep_type, token, data_ptr, transfer_len, 0u);

	return (status == HAL_OK) ? 0 : -EIO;
}

static uint32_t uhc_stm32_hc_done_bytes(struct stm32_otghs_data *d, uint8_t hc, uint16_t req_len,
					bool dir_in)
{
	uint32_t completed;

	if (IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA) && dir_in) {
		const USB_OTG_HostChannelTypeDef *regs =
			(USB_OTG_HostChannelTypeDef *)((uintptr_t)d->hhcd.Instance +
						       (uintptr_t)USB_OTG_HOST_CHANNEL_BASE +
						       ((uintptr_t)hc *
							(uintptr_t)USB_OTG_HOST_CHANNEL_SIZE));
		const uint32_t remaining = regs->HCTSIZ & USB_OTG_HCTSIZ_XFRSIZ;
		const uint32_t programmed = d->hhcd.hc[hc].XferSize;

		completed = (programmed >= remaining) ? (programmed - remaining) : 0u;
	} else {
		completed = HAL_HCD_HC_GetXferCount(&d->hhcd, hc);
	}

	return MIN(completed, (uint32_t)req_len);
}

static bool uhc_stm32_hc_is_enabled(const struct stm32_otghs_data *d, uint8_t hc)
{
	const USB_OTG_HostChannelTypeDef *regs =
		(USB_OTG_HostChannelTypeDef *)((uintptr_t)d->hhcd.Instance +
					       (uintptr_t)USB_OTG_HOST_CHANNEL_BASE +
					       ((uintptr_t)hc *
						(uintptr_t)USB_OTG_HOST_CHANNEL_SIZE));

	return (regs->HCCHAR & USB_OTG_HCCHAR_CHENA) != 0u;
}

static bool uhc_stm32_nonctrl_needs_sw_retry(struct stm32_otghs_data *d, uint8_t hc,
					 HCD_URBStateTypeDef urb_state)
{
	struct stm32_hc_active *a = &d->hc_act[hc];

	if ((a->xfer == NULL) || a->dir_in) {
		return false;
	}

	if ((urb_state != URB_NOTREADY) && (urb_state != URB_IDLE)) {
		return false;
	}

	/*
	 * HAL auto-reactivates several OUT retry cases itself by setting CHENA
	 * again in the CHH handler. Only resubmit from software once the channel
	 * is actually halted and left disabled (for example, HS NYET / ping ACK).
	 */
	return !uhc_stm32_hc_is_enabled(d, hc);
}

static bool uhc_stm32_ctrl_needs_sw_retry(struct stm32_otghs_data *d, uint8_t hc,
				      HCD_URBStateTypeDef urb_state)
{
	if ((d->ctrl.xfer == NULL) || (d->ctrl.hc != (int)hc)) {
		return false;
	}

	if (USB_EP_DIR_IS_IN(d->ctrl.ep)) {
		return false;
	}

	if ((urb_state != URB_NOTREADY) && (urb_state != URB_IDLE)) {
		return false;
	}

	return !uhc_stm32_hc_is_enabled(d, hc);
}

static int uhc_stm32_commit_halted_bytes(struct uhc_transfer *xfer, uint16_t req_len,
					 uint16_t *buf_off, uint16_t *halted_cnt,
					 uint32_t hal_cnt)
{
	if ((xfer == NULL) || (xfer->buf == NULL)) {
		return 0;
	}

	if (hal_cnt < (uint32_t)*halted_cnt) {
		return -EIO;
	}

	const uint32_t add = hal_cnt - (uint32_t)*halted_cnt;

	if (add == 0u) {
		return 0;
	}

	if ((add > (uint32_t)net_buf_tailroom(xfer->buf)) ||
	    (add > ((uint32_t)req_len - (uint32_t)*halted_cnt))) {
		return -EIO;
	}

	(void)net_buf_add(xfer->buf, add);
	*buf_off += (uint16_t)add;
	*halted_cnt += (uint16_t)add;

	return 0;
}

/* Port / speed helpers */

static uint32_t uhc_stm32_read_hprt(struct stm32_otghs_data *d)
{
	volatile uint32_t *hprt = (volatile uint32_t *)((uintptr_t)d->hhcd.Instance +
						       (uintptr_t)USB_OTG_HOST_PORT_BASE);

	return stm32_reg_read(hprt);
}

static void uhc_stm32_dump_hprt(struct stm32_otghs_data *d, const char *tag)
{
	const uint32_t h = uhc_stm32_read_hprt(d);

	const unsigned int pcsts __maybe_unused = !!(h & USB_OTG_HPRT_PCSTS);
	const unsigned int pena __maybe_unused = !!(h & USB_OTG_HPRT_PENA);
	const unsigned int prst __maybe_unused = !!(h & USB_OTG_HPRT_PRST);
	const unsigned int ppwr __maybe_unused = !!(h & USB_OTG_HPRT_PPWR);
	const unsigned int pspd __maybe_unused = _FLD2VAL(USB_OTG_HPRT_PSPD, h);
	const unsigned int plsts __maybe_unused = _FLD2VAL(USB_OTG_HPRT_PLSTS, h);

	LOG_INF("[%s] HPRT=0x%08x pcsts=%u pena=%u prst=%u ppwr=%u pspd=%u plsts=%u", tag,
		(unsigned int)h, pcsts, pena, prst, ppwr, pspd, plsts);
}

static void uhc_stm32_update_dev_speed_from_hal(struct stm32_otghs_data *d)
{
	const uint32_t spd = HAL_HCD_GetCurrentSpeed(&d->hhcd);

	switch (spd) {
	case HCD_DEVICE_SPEED_HIGH:
		d->speed = HCD_DEVICE_SPEED_HIGH;
		break;
	case HCD_DEVICE_SPEED_LOW:
		d->speed = HCD_DEVICE_SPEED_LOW;
		break;
	default:
		d->speed = HCD_DEVICE_SPEED_FULL;
		break;
	}
}

/* VBUS helpers */

static void uhc_stm32_disable_vbus_sensing(struct stm32_otghs_data *d)
{
	USB_OTG_GlobalTypeDef *usb = (USB_OTG_GlobalTypeDef *)d->hhcd.Instance;

#ifdef USB_OTG_GCCFG_NOVBUSSENS
	stm32_reg_set_bits(&usb->GCCFG, USB_OTG_GCCFG_NOVBUSSENS);
#else
#ifdef USB_OTG_GCCFG_VBDEN
	/*
	 * U5 exposes this as VBDEN instead of NOVBUSSENS. Keep the documented
	 * detector bit set and force session validity through GOTGCTL below.
	 */
	stm32_reg_set_bits(&usb->GCCFG, USB_OTG_GCCFG_VBDEN);
#endif
#endif
#ifdef USB_OTG_GCCFG_DCDEN
	stm32_reg_clear_bits(&usb->GCCFG, USB_OTG_GCCFG_DCDEN);
#endif
#ifdef USB_OTG_GCCFG_PDEN
	stm32_reg_clear_bits(&usb->GCCFG, USB_OTG_GCCFG_PDEN);
#endif

	k_busy_wait(STM32_OTGHS_VBUS_SETTLE_US);
	LOG_INF("[VBUS-SENS] GCCFG=0x%08x", (unsigned int)usb->GCCFG);
}

static void uhc_stm32_enable_vbus_sensing(struct stm32_otghs_data *d)
{
	USB_OTG_GlobalTypeDef *usb = (USB_OTG_GlobalTypeDef *)d->hhcd.Instance;

#ifdef USB_OTG_GCCFG_NOVBUSSENS
	stm32_reg_clear_bits(&usb->GCCFG, USB_OTG_GCCFG_NOVBUSSENS);
#endif
#ifdef USB_OTG_GCCFG_VBDEN
	stm32_reg_set_bits(&usb->GCCFG, USB_OTG_GCCFG_VBDEN);
#endif
#ifdef USB_OTG_GCCFG_DCDEN
	stm32_reg_clear_bits(&usb->GCCFG, USB_OTG_GCCFG_DCDEN);
#endif
#ifdef USB_OTG_GCCFG_PDEN
	stm32_reg_clear_bits(&usb->GCCFG, USB_OTG_GCCFG_PDEN);
#endif
#ifdef USB_OTG_GCCFG_VBVALEXTOEN
	stm32_reg_clear_bits(&usb->GCCFG, USB_OTG_GCCFG_VBVALEXTOEN);
#endif
#ifdef USB_OTG_GCCFG_VBVALOVAL
	stm32_reg_clear_bits(&usb->GCCFG, USB_OTG_GCCFG_VBVALOVAL);
#endif

	stm32_reg_clear_bits(&usb->GOTGCTL,
			     USB_OTG_GOTGCTL_AVALOEN | USB_OTG_GOTGCTL_AVALOVAL |
				     USB_OTG_GOTGCTL_BVALOEN | USB_OTG_GOTGCTL_BVALOVAL);

	k_busy_wait(STM32_OTGHS_VBUS_SETTLE_US);
	LOG_INF("[VBUS-SENS] GCCFG=0x%08x GOTGCTL=0x%08x", (unsigned int)usb->GCCFG,
		(unsigned int)usb->GOTGCTL);
}

static void uhc_stm32_force_session_valid(struct stm32_otghs_data *d)
{
	USB_OTG_GlobalTypeDef *usb = (USB_OTG_GlobalTypeDef *)d->hhcd.Instance;

	stm32_reg_set_bits(&usb->GOTGCTL,
			   USB_OTG_GOTGCTL_AVALOEN | USB_OTG_GOTGCTL_AVALOVAL |
				   USB_OTG_GOTGCTL_BVALOEN | USB_OTG_GOTGCTL_BVALOVAL);

	k_busy_wait(STM32_OTGHS_VBUS_SETTLE_US);
	LOG_INF("[VBUS-OVR] GOTGCTL=0x%08x", (unsigned int)usb->GOTGCTL);
}

/* Zephyr list helpers */

static struct uhc_transfer *uhc_stm32_peek_next_ctrl_xfer(const struct device *dev)
{
	struct uhc_data *data = dev->data;
	struct uhc_transfer *x;

	return SYS_DLIST_PEEK_HEAD_CONTAINER(&data->ctrl_xfers, x, node);
}

static bool uhc_stm32_list_contains_xfer(sys_dlist_t *list, struct uhc_transfer *xfer)
{
	struct uhc_transfer *tmp;

	SYS_DLIST_FOR_EACH_CONTAINER(list, tmp, node) {
		if (tmp == xfer) {
			return true;
		}
	}

	return false;
}

/* Channel management */

struct stm32_chan_params {
	uint8_t dev_addr;
	uint8_t ep;
	uint8_t ep_num;
	uint8_t ep_type;
	uint16_t mps;
	bool dir_in;
	uint8_t hal_ep;
};

static int uhc_stm32_find_chan(struct stm32_otghs_data *d, uint8_t dev_addr, uint8_t ep,
			       uint8_t ep_type, uint16_t mps)
{
	const bool ep0 = STM32_EP_GET_IDX(ep) == 0u;

	for (uint8_t i = 0; i < d->host_channels; i++) {
		/*
		 * EP0 channel lookup intentionally ignores MPS because EP0 MPS
		 * can be reinitialized during enumeration, while non-EP0
		 * channels still require an exact MPS match.
		 */
		if (d->ch[i].in_use && (d->ch[i].dev_addr == dev_addr) &&
		    (d->ch[i].ep == ep) && (d->ch[i].ep_type == ep_type) &&
		    (ep0 || (d->ch[i].mps == mps))) {
			return (int)d->ch[i].hc_num;
		}
	}

	return -ENOENT;
}

static void uhc_stm32_free_all_channels(struct stm32_otghs_data *d)
{
	for (uint8_t i = 0; i < d->host_channels; i++) {
		if (d->ch[i].in_use) {
			uhc_stm32_hc_halt(d, i);
			d->ch[i].in_use = false;
		}
	}
}

static int uhc_stm32_find_idle_chan_match_nonctrl(struct stm32_otghs_data *d, uint8_t dev_addr,
					      uint8_t ep, uint8_t ep_type, uint16_t mps)
{
	for (uint8_t i = 0; i < d->host_channels; i++) {
		if (d->ch[i].in_use && (d->hc_act[i].xfer == NULL) &&
		    (d->ch[i].dev_addr == dev_addr) && (d->ch[i].ep == ep) &&
		    (d->ch[i].ep_type == ep_type) && (d->ch[i].mps == mps)) {
			return (int)d->ch[i].hc_num;
		}
	}

	return -ENOENT;
}

static int uhc_stm32_init_free_chan(struct stm32_otghs_data *d, uint8_t hc,
				    const struct stm32_chan_params *params)
{
	stm32_status_t status;

	d->ch[hc].in_use = true;
	d->ch[hc].dev_addr = params->dev_addr;
	d->ch[hc].ep = params->ep;
	d->ch[hc].ep_num = params->ep_num;
	d->ch[hc].ep_type = params->ep_type;
	d->ch[hc].mps = params->mps;
	d->ch[hc].dir_in = params->dir_in ? 1u : 0u;
	d->ch[hc].hc_num = hc;

	status = HAL_HCD_HC_Init(&d->hhcd, hc, params->hal_ep, params->dev_addr,
				 (uint32_t)d->speed, params->ep_type, params->mps);
	if (status != HAL_OK) {
		d->ch[hc].in_use = false;
		return -EIO;
	}

	return (int)hc;
}

static int uhc_stm32_alloc_free_chan(struct stm32_otghs_data *d,
				     const struct stm32_chan_params *params)
{
	for (uint8_t i = 0; i < d->host_channels; i++) {
		if (!d->ch[i].in_use) {
			return uhc_stm32_init_free_chan(d, i, params);
		}
	}

	return -ENOMEM;
}

static int uhc_stm32_reinit_ep0_chan_if_needed(struct stm32_otghs_data *d, uint8_t hc,
					       const struct stm32_chan_params *params)
{
	stm32_status_t status;

	if (d->ch[hc].mps == params->mps) {
		return 0;
	}

	d->ch[hc].mps = params->mps;
	uhc_stm32_hc_halt(d, hc);

	status = HAL_HCD_HC_Init(&d->hhcd, hc, params->hal_ep, params->dev_addr,
				 (uint32_t)d->speed, params->ep_type, params->mps);

	return (status == HAL_OK) ? 0 : -EIO;
}

static int uhc_stm32_alloc_or_reinit_ctrl_chan(struct stm32_otghs_data *d, uint8_t dev_addr,
					       const struct stm32_chan_params *params)
{
	const bool ep0 = (params->ep_num == 0u);
	const int existing =
		uhc_stm32_find_chan(d, dev_addr, params->ep, params->ep_type, params->mps);

	if (existing >= 0) {
		if (ep0) {
			const int ret =
				uhc_stm32_reinit_ep0_chan_if_needed(d, (uint8_t)existing, params);

			if (ret != 0) {
				return ret;
			}
		}

		return existing;
	}

	return uhc_stm32_alloc_free_chan(d, params);
}

static int uhc_stm32_alloc_or_reinit_chan(struct stm32_otghs_data *d, uint8_t dev_addr, uint8_t ep,
					  uint8_t ep_type, uint16_t mps)
{
	const bool dir_in = USB_EP_DIR_IS_IN(ep);
	const uint8_t ep_num = STM32_EP_GET_IDX(ep);
	const struct stm32_chan_params params = {
		.dev_addr = dev_addr,
		.ep = ep,
		.ep_num = ep_num,
		.ep_type = ep_type,
		.mps = mps,
		.dir_in = dir_in,
		.hal_ep = ep_num | (dir_in ? USB_EP_DIR_IN : USB_EP_DIR_OUT),
	};

	if ((ep_num == 0u) || (ep_type == EP_TYPE_CTRL)) {
		return uhc_stm32_alloc_or_reinit_ctrl_chan(d, dev_addr, &params);
	}

	const int idle = uhc_stm32_find_idle_chan_match_nonctrl(d, dev_addr, ep, ep_type, mps);

	if (idle >= 0) {
		return idle;
	}

	return uhc_stm32_alloc_free_chan(d, &params);
}

/* Submit helpers */

static uint16_t uhc_stm32_ep0_mps_default(const struct stm32_otghs_data *d,
					  const struct uhc_transfer *xfer)
{
	if (xfer->mps != 0u) {
		return xfer->mps;
	}
	if (d->ep0_mps != 0u) {
		return d->ep0_mps;
	}
	return USB_EP0_MPS_8;
}

static bool uhc_stm32_usb_req_is_in(const struct uhc_transfer *xfer)
{
	struct usb_setup_packet setup;

	memcpy(&setup, xfer->setup_pkt, sizeof(setup));
	return usb_reqtype_is_to_host(&setup);
}

static uint16_t uhc_stm32_usb_setup_wlength(const struct uhc_transfer *xfer)
{
	struct usb_setup_packet setup;

	memcpy(&setup, xfer->setup_pkt, sizeof(setup));
	return sys_le16_to_cpu(setup.wLength);
}

static bool uhc_stm32_ep_has_active_xfer(struct stm32_otghs_data *d, uint8_t dev_addr, uint8_t ep)
{
	for (uint8_t i = 0; i < d->host_channels; i++) {
		if ((d->hc_act[i].xfer != NULL) && (d->hc_act[i].xfer->udev != NULL) &&
		    (d->hc_act[i].xfer->udev->addr == dev_addr) &&
		    (d->hc_act[i].ep == ep)) {
			return true;
		}
	}

	return false;
}

static uint32_t uhc_stm32_out_submit_len(uint8_t ep_type, uint16_t mps,
					 uint32_t remaining)
{
	if (!IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA) && (ep_type == EP_TYPE_BULK) && (mps > 0u)) {
		return MIN(remaining, (uint32_t)mps);
	}

	return remaining;
}

static int uhc_stm32_submit_control(struct stm32_otghs_data *d, struct uhc_transfer *xfer)
{
	if ((d == NULL) || (xfer == NULL) || (xfer->udev == NULL)) {
		return -EINVAL;
	}

	const uint8_t dev_addr = xfer->udev->addr;
	const uint16_t wLength = uhc_stm32_usb_setup_wlength(xfer);

	const bool req_in = uhc_stm32_usb_req_is_in(xfer);
	const bool has_data = (wLength != 0u);

	const uint16_t mps = uhc_stm32_ep0_mps_default(d, xfer);

	uint8_t ep = 0u;
	uint8_t dir = 0u;
	uint8_t token = 0u;
	uint8_t *data_ptr = NULL;
	uint32_t transfer_len = 0u;

	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		ep = USB_CONTROL_EP_OUT;
		dir = CH_OUT_DIR;
		token = STM32_HCD_TOKEN_SETUP;
		memcpy(d->ctrl.setup_words, xfer->setup_pkt, sizeof(xfer->setup_pkt));
		data_ptr = (uint8_t *)d->ctrl.setup_words;
		transfer_len = sizeof(xfer->setup_pkt);

	} else if ((xfer->stage == UHC_CONTROL_STAGE_DATA) && (xfer->buf != NULL) && has_data) {
		ep = req_in ? USB_CONTROL_EP_IN : USB_CONTROL_EP_OUT;
		dir = req_in ? CH_IN_DIR : CH_OUT_DIR;
		token = STM32_HCD_TOKEN_DATA;

		/*
		 * For IN, buf->len may carry the requested receive length. Reset it
		 * before arming the HAL transfer so completion can append actual bytes.
		 * For OUT, buf->len is already the payload length to send.
		 */
		if (req_in) {
			uint32_t want = (xfer->buf->len != 0u) ? (uint32_t)xfer->buf->len
							       : (uint32_t)wLength;
			want = MIN(want, (uint32_t)wLength);

			xfer->buf->len = 0u;
			want = MIN(want, (uint32_t)net_buf_tailroom(xfer->buf));
			data_ptr = xfer->buf->data;
			transfer_len = want;
		} else {
			data_ptr = xfer->buf->data;
			transfer_len = MIN((uint32_t)wLength, (uint32_t)xfer->buf->len);
		}

		if (transfer_len == 0u) {
			return -EINVAL;
		}

	} else if (xfer->stage == UHC_CONTROL_STAGE_STATUS) {
		const bool status_in = !req_in;

		ep = status_in ? USB_CONTROL_EP_IN : USB_CONTROL_EP_OUT;
		dir = status_in ? CH_IN_DIR : CH_OUT_DIR;
		token = STM32_HCD_TOKEN_DATA;
		data_ptr = (uint8_t *)&d->zlp_dummy;
		transfer_len = 0u;

	} else {
		return -EINVAL;
	}

	const int hc = uhc_stm32_alloc_or_reinit_chan(d, dev_addr, ep, EP_TYPE_CTRL, mps);

	if (hc < 0) {
		return hc;
	}

	const int ret = uhc_stm32_hc_submit_request(d, (uint8_t)hc, dir, EP_TYPE_CTRL, token,
						data_ptr, (uint16_t)transfer_len);
	if (ret != 0) {
		return ret;
	}

	d->ctrl.xfer = xfer;
	d->ctrl.hc = hc;
	d->ctrl.ep = ep;
	d->ctrl.len = (uint16_t)transfer_len;
	d->ctrl.buf_off = 0u;
	d->ctrl.halted_cnt = 0u;
	d->ctrl.retry_timepoint = sys_timepoint_calc(K_MSEC(STM32_OTGHS_NAK_WAIT_RETRY_MS));

	return 0;
}

static int uhc_stm32_resubmit_control_same_hc(struct stm32_otghs_data *d)
{
	struct stm32_pipe *p = &d->ctrl;
	struct uhc_transfer *xfer = p->xfer;
	uint16_t transfer_len = p->len;
	uint32_t buf_limit;
	uint8_t *data_ptr;

	if ((xfer == NULL) || (xfer->udev == NULL) || (p->hc < 0)) {
		return -EINVAL;
	}

	if ((xfer->stage != UHC_CONTROL_STAGE_DATA) || !uhc_stm32_usb_req_is_in(xfer) ||
	    (xfer->buf == NULL)) {
		return uhc_stm32_submit_control(d, xfer);
	}

	if (p->halted_cnt > transfer_len) {
		return -EINVAL;
	}

	transfer_len -= p->halted_cnt;
	buf_limit = (uint32_t)xfer->buf->len + (uint32_t)net_buf_tailroom(xfer->buf);
	if (((uint32_t)p->buf_off + (uint32_t)transfer_len) > buf_limit) {
		return -EINVAL;
	}
	data_ptr = xfer->buf->data + p->buf_off;

	const int ret = uhc_stm32_hc_submit_request(d, (uint8_t)p->hc, CH_IN_DIR, EP_TYPE_CTRL,
						STM32_HCD_TOKEN_DATA, data_ptr, transfer_len);
	if (ret != 0) {
		return ret;
	}

	p->len = transfer_len;
	p->halted_cnt = 0u;
	p->retry_timepoint = sys_timepoint_calc(K_MSEC(STM32_OTGHS_NAK_WAIT_RETRY_MS));

	return 0;
}

static int uhc_stm32_submit_nonctrl(struct stm32_otghs_data *d, struct uhc_transfer *xfer)
{
	if ((d == NULL) || (xfer == NULL) || (xfer->udev == NULL) || (xfer->buf == NULL)) {
		return -EINVAL;
	}

	const uint8_t dev_addr = xfer->udev->addr;
	const uint8_t ep = xfer->ep;
	const bool dir_in = USB_EP_DIR_IS_IN(ep);

	uint8_t ep_type;

	switch (xfer->type) {
	case USB_EP_TYPE_BULK:
		ep_type = EP_TYPE_BULK;
		break;
	case USB_EP_TYPE_INTERRUPT:
		ep_type = EP_TYPE_INTR;
		break;
	default:
		return -ENOTSUP;
	}

	uint16_t mps = xfer->mps;

	if (mps == 0u) {
		mps = (ep_type == EP_TYPE_INTR) ? STM32_MPS_DEFAULT_INTR : STM32_MPS_DEFAULT_BULK;
	}

	uint32_t transfer_len = 0u;
	uint8_t *data_ptr = xfer->buf->data;

	if (dir_in) {
		const uint32_t cap =
			(uint32_t)net_buf_tailroom(xfer->buf) + (uint32_t)xfer->buf->len;
		uint32_t want = (xfer->buf->len != 0u) ? (uint32_t)xfer->buf->len : cap;

		xfer->buf->len = 0u;
		want = MIN(want, cap);
		transfer_len = want;
	} else {
		transfer_len = uhc_stm32_out_submit_len(ep_type, mps, (uint32_t)xfer->buf->len);
	}

	if (transfer_len == 0u) {
		return -EINVAL;
	}

	/* Toggle state is tracked per endpoint, so keep one active transfer per endpoint. */
	if (uhc_stm32_ep_has_active_xfer(d, dev_addr, ep)) {
		return -EBUSY;
	}

	const int hc = uhc_stm32_alloc_or_reinit_chan(d, dev_addr, ep, ep_type, mps);

	if (hc < 0) {
		return hc;
	}

	if (d->hc_act[hc].xfer != NULL) {
		return -EBUSY;
	}

	const int ret = uhc_stm32_hc_submit_request(d, (uint8_t)hc,
						dir_in ? CH_IN_DIR : CH_OUT_DIR, ep_type,
						STM32_HCD_TOKEN_DATA, data_ptr,
						(uint16_t)transfer_len);
	if (ret != 0) {
		return ret;
	}

	d->hc_act[hc].xfer = xfer;
	d->hc_act[hc].ep = ep;
	d->hc_act[hc].ep_type = ep_type;
	d->hc_act[hc].dir_in = dir_in ? 1u : 0u;
	d->hc_act[hc].req_len = (uint16_t)transfer_len;
	d->hc_act[hc].buf_off = 0u;
	d->hc_act[hc].halted_cnt = 0u;
	d->hc_act[hc].mps = mps;
	d->hc_act[hc].retry_timepoint =
		sys_timepoint_calc(K_MSEC(STM32_OTGHS_NAK_WAIT_RETRY_MS));

	return 0;
}

static int uhc_stm32_resubmit_nonctrl_same_hc(struct stm32_otghs_data *d, int hc)
{
	struct stm32_hc_active *a = &d->hc_act[hc];
	struct uhc_transfer *xfer = a->xfer;
	uint16_t req_len = a->req_len;
	uint32_t buf_limit;

	if ((xfer == NULL) || (xfer->udev == NULL) || (xfer->buf == NULL)) {
		return -EINVAL;
	}

	if (a->dir_in) {
		if (a->halted_cnt > req_len) {
			return -EINVAL;
		}

		req_len -= a->halted_cnt;
		buf_limit = (uint32_t)xfer->buf->len + (uint32_t)net_buf_tailroom(xfer->buf);
	} else {
		buf_limit = (uint32_t)xfer->buf->len;
	}

	if (((uint32_t)a->buf_off + (uint32_t)req_len) > buf_limit) {
		return -EINVAL;
	}

	uint8_t *data_ptr = xfer->buf->data + a->buf_off;

	const int ret = uhc_stm32_hc_submit_request(d, (uint8_t)hc,
						a->dir_in ? CH_IN_DIR : CH_OUT_DIR, a->ep_type,
						STM32_HCD_TOKEN_DATA, data_ptr, req_len);
	if (ret != 0) {
		return ret;
	}

	a->req_len = req_len;
	a->halted_cnt = 0u;
	a->retry_timepoint = sys_timepoint_calc(K_MSEC(STM32_OTGHS_NAK_WAIT_RETRY_MS));
	return 0;
}

/* Completion */

static bool uhc_stm32_is_std_get_device_desc(const struct uhc_transfer *xfer)
{
	struct usb_setup_packet setup;

	memcpy(&setup, xfer->setup_pkt, sizeof(setup));

	const bool std_type = (USB_REQTYPE_GET_TYPE(setup.bmRequestType) ==
			       USB_REQTYPE_TYPE_STANDARD);
	const bool get_desc = (setup.bRequest == USB_SREQ_GET_DESCRIPTOR);
	const bool device_desc = (USB_GET_DESCRIPTOR_TYPE(sys_le16_to_cpu(setup.wValue)) ==
				  USB_DESC_DEVICE);

	return std_type && get_desc && device_desc;
}

static void uhc_stm32_ep0_force_data1_after_setup(struct stm32_otghs_data *d, uint8_t dev_addr)
{
	/*
	 * HAL keeps separate channel state for EP0 IN and OUT, but a SETUP packet
	 * resets both control data toggles. Seed both cached directions so the
	 * following DATA or STATUS stage starts with DATA1.
	 */
	const int hc_out =
		uhc_stm32_find_chan(d, dev_addr, USB_CONTROL_EP_OUT, EP_TYPE_CTRL, d->ep0_mps);
	const int hc_in =
		uhc_stm32_find_chan(d, dev_addr, USB_CONTROL_EP_IN, EP_TYPE_CTRL, d->ep0_mps);

	if (hc_out >= 0) {
		d->hhcd.hc[hc_out].toggle_in = 1u;
		d->hhcd.hc[hc_out].toggle_out = 1u;
	}
	if (hc_in >= 0) {
		d->hhcd.hc[hc_in].toggle_in = 1u;
		d->hhcd.hc[hc_in].toggle_out = 1u;
	}
}

static uint8_t uhc_stm32_ctrl_stage_ep(const struct uhc_transfer *xfer, bool req_in)
{
	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		return USB_CONTROL_EP_OUT;
	}

	if (xfer->stage == UHC_CONTROL_STAGE_DATA) {
		return req_in ? USB_CONTROL_EP_IN : USB_CONTROL_EP_OUT;
	}

	return req_in ? USB_CONTROL_EP_OUT : USB_CONTROL_EP_IN;
}

static bool uhc_stm32_ctrl_handle_wait_or_error(const struct device *dev,
					    struct stm32_otghs_data *d,
					    struct stm32_pipe *p,
					    struct uhc_transfer *xfer, uint8_t hc,
					    HCD_URBStateTypeDef urb_state,
					    uint32_t hal_cnt)
{
	const bool req_in = uhc_stm32_usb_req_is_in(xfer);

	if (uhc_stm32_ctrl_needs_sw_retry(d, hc, urb_state)) {
		const int r = uhc_stm32_submit_control(d, xfer);

		if (r != 0) {
			uhc_stm32_pipe_finish(dev, p, r);
		}
		return true;
	}

	if (uhc_stm32_urb_waiting_on_device(urb_state)) {
		return true;
	}

	if (urb_state == URB_NAK_WAIT) {
		if ((xfer->stage == UHC_CONTROL_STAGE_DATA) && req_in) {
			const int pr = uhc_stm32_commit_halted_bytes(
				p->xfer, p->len, &p->buf_off, &p->halted_cnt, hal_cnt);

			if (pr != 0) {
				uhc_stm32_pipe_finish(dev, p, pr);
				return true;
			}
		}

		if (!sys_timepoint_expired(p->retry_timepoint)) {
			return true;
		}

		const int r = uhc_stm32_resubmit_control_same_hc(d);

		if (r != 0) {
			uhc_stm32_pipe_finish(dev, p, r);
		}
		return true;
	}

	if (urb_state == URB_STALL) {
		uhc_stm32_pipe_finish(dev, p, -EPIPE);
		return true;
	}

	if (urb_state != URB_DONE) {
		uhc_stm32_pipe_finish(dev, p, -EIO);
		return true;
	}

	return false;
}

static void uhc_stm32_ctrl_complete_setup(const struct device *dev, struct stm32_otghs_data *d,
				      struct stm32_pipe *p, struct uhc_transfer *xfer,
				      uint8_t dev_addr, bool has_data)
{
	uhc_stm32_ep0_force_data1_after_setup(d, dev_addr);

	if (has_data) {
		if (xfer->buf == NULL) {
			uhc_stm32_pipe_finish(dev, p, -EINVAL);
			return;
		}
		xfer->stage = UHC_CONTROL_STAGE_DATA;
	} else {
		xfer->stage = UHC_CONTROL_STAGE_STATUS;
	}

	const int r = uhc_stm32_submit_control(d, xfer);

	if (r != 0) {
		uhc_stm32_pipe_finish(dev, p, r);
	}
}

static void uhc_stm32_ctrl_maybe_update_ep0_mps(struct stm32_otghs_data *d,
						struct uhc_transfer *xfer)
{
	if ((xfer->buf->len < USB_EP0_MPS_8) || !uhc_stm32_is_std_get_device_desc(xfer)) {
		return;
	}

	const struct usb_device_descriptor *dev_desc = (const void *)xfer->buf->data;
	const uint8_t mps0 = dev_desc->bMaxPacketSize0;

	if ((mps0 == USB_EP0_MPS_8) || (mps0 == USB_EP0_MPS_16) ||
	    (mps0 == USB_EP0_MPS_32) || (mps0 == USB_EP0_MPS_64)) {
		d->ep0_mps = mps0;
	}
}

static void uhc_stm32_ctrl_complete_data(const struct device *dev, struct stm32_otghs_data *d,
				     struct stm32_pipe *p, struct uhc_transfer *xfer, bool req_in,
				     uint32_t hal_cnt)
{
	if (req_in && (xfer->buf != NULL) && (hal_cnt > 0u)) {
		const uint32_t add = MIN(hal_cnt, (uint32_t)net_buf_tailroom(xfer->buf));

		(void)net_buf_add(xfer->buf, add);
		uhc_stm32_ctrl_maybe_update_ep0_mps(d, xfer);
	}

	xfer->stage = UHC_CONTROL_STAGE_STATUS;

	const int r = uhc_stm32_submit_control(d, xfer);

	if (r != 0) {
		uhc_stm32_pipe_finish(dev, p, r);
	}
}

static void uhc_stm32_complete_control_pipe(const struct device *dev, struct stm32_otghs_data *d,
					struct stm32_pipe *p)
{
	struct uhc_transfer *xfer = p->xfer;

	if ((xfer == NULL) || (xfer->udev == NULL)) {
		return;
	}

	const uint8_t dev_addr = xfer->udev->addr;
	const uint16_t wLength = uhc_stm32_usb_setup_wlength(xfer);
	const bool req_in = uhc_stm32_usb_req_is_in(xfer);
	const bool has_data = (wLength != 0u);
	const uint16_t mps = uhc_stm32_ep0_mps_default(d, xfer);
	const uint8_t stage_ep = uhc_stm32_ctrl_stage_ep(xfer, req_in);

	const int hc = uhc_stm32_find_chan(d, dev_addr, stage_ep, EP_TYPE_CTRL, mps);

	if (hc < 0) {
		uhc_stm32_pipe_finish(dev, p, -EIO);
		return;
	}

	const HCD_URBStateTypeDef urb_state = HAL_HCD_HC_GetURBState(&d->hhcd, (uint8_t)hc);
	const uint32_t hal_cnt =
		uhc_stm32_hc_done_bytes(d, (uint8_t)hc, p->len, USB_EP_DIR_IS_IN(stage_ep));

	if (uhc_stm32_ctrl_handle_wait_or_error(dev, d, p, xfer, (uint8_t)hc, urb_state, hal_cnt)) {
		return;
	}

	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		uhc_stm32_ctrl_complete_setup(dev, d, p, xfer, dev_addr, has_data);
		return;
	}

	if (xfer->stage == UHC_CONTROL_STAGE_DATA) {
		uhc_stm32_ctrl_complete_data(dev, d, p, xfer, req_in, hal_cnt);
		return;
	}

	uhc_stm32_pipe_finish(dev, p, 0);
}

static void uhc_stm32_cancel_all_nonctrl(const struct device *dev, struct stm32_otghs_data *d,
					 int st)
{
	for (uint8_t i = 0; i < d->host_channels; i++) {
		if (d->hc_act[i].xfer != NULL) {
			uhc_stm32_hc_halt(d, i);
			uhc_xfer_return(dev, d->hc_act[i].xfer, st);
			uhc_stm32_hc_act_clear(&d->hc_act[i]);
		}
	}
}

static bool uhc_stm32_nonctrl_continue_out(const struct device *dev, struct stm32_otghs_data *d,
				       struct stm32_hc_active *a, int hc)
{
	struct uhc_transfer *xfer = a->xfer;
	const uint32_t total = (uint32_t)xfer->buf->len;
	const uint32_t next_off = (uint32_t)a->buf_off + (uint32_t)a->req_len;

	if (next_off >= total) {
		return false;
	}

	const uint32_t remaining = total - next_off;
	const uint32_t next_len = uhc_stm32_out_submit_len(a->ep_type, a->mps, remaining);
	int r;

	a->buf_off = (uint16_t)next_off;
	a->req_len = (uint16_t)next_len;

	r = uhc_stm32_resubmit_nonctrl_same_hc(d, hc);
	if (r != 0) {
		uhc_stm32_active_xfer_finish(dev, a, r);
	}

	return true;
}

static void uhc_stm32_nonctrl_complete_done(const struct device *dev, struct stm32_otghs_data *d,
					struct stm32_hc_active *a, int hc, uint32_t hal_cnt)
{
	struct uhc_transfer *xfer = a->xfer;

	if (a->dir_in && (xfer->buf != NULL) && (hal_cnt > 0u)) {
		const uint32_t add = MIN(hal_cnt, (uint32_t)net_buf_tailroom(xfer->buf));

		(void)net_buf_add(xfer->buf, add);
	}

	/*
	 * STM32 HAL non-DMA toggle tracking bug for OUT transfers only.
	 *
	 * OUT (non-DMA): the CHH handler XORs toggle_out exactly once per
	 * transfer completion regardless of packet count. For even-packet
	 * transfers the two per-packet flips cancel, so the net toggle must
	 * be unchanged but HAL has already flipped it once.
	 *
	 * IN (non-DMA): HAL already updates toggle_in per packet path.
	 */
	if (!IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA) && !a->dir_in && (a->mps != 0u) &&
	    (a->req_len != 0u) && ((DIV_ROUND_UP(a->req_len, a->mps) & 1u) == 0u)) {
		d->hhcd.hc[hc].toggle_out ^= 1u;
	}

	if (!a->dir_in && (xfer->buf != NULL) &&
	    uhc_stm32_nonctrl_continue_out(dev, d, a, hc)) {
		return;
	}

	uhc_stm32_active_xfer_finish(dev, a, 0);
}

static bool uhc_stm32_nonctrl_handle_wait_or_error(const struct device *dev,
						   struct stm32_otghs_data *d,
						   struct stm32_hc_active *a, int hc,
						   HCD_URBStateTypeDef urb_state,
						   uint32_t hal_cnt)
{
	if (uhc_stm32_nonctrl_needs_sw_retry(d, (uint8_t)hc, urb_state)) {
		const int r = uhc_stm32_resubmit_nonctrl_same_hc(d, hc);

		if (r != 0) {
			uhc_stm32_active_xfer_finish(dev, a, r);
		}
		return true;
	}

	if (uhc_stm32_urb_waiting_on_device(urb_state)) {
		return true;
	}

	if (urb_state == URB_NAK_WAIT) {
		int pr = 0;

		if (a->dir_in) {
			pr = uhc_stm32_commit_halted_bytes(a->xfer, a->req_len, &a->buf_off,
							   &a->halted_cnt, hal_cnt);
		}

		if (pr != 0) {
			uhc_stm32_active_xfer_finish(dev, a, pr);
			return true;
		}

		if (!sys_timepoint_expired(a->retry_timepoint)) {
			return true;
		}

		const int r = uhc_stm32_resubmit_nonctrl_same_hc(d, hc);

		if (r != 0) {
			uhc_stm32_active_xfer_finish(dev, a, r);
		}
		return true;
	}

	if (urb_state == URB_STALL) {
		uhc_stm32_active_xfer_finish(dev, a, -EPIPE);
		return true;
	}

	if (urb_state != URB_DONE) {
		uhc_stm32_active_xfer_finish(dev, a, -EIO);
		return true;
	}

	return false;
}

static void uhc_stm32_complete_one_hc(const struct device *dev, struct stm32_otghs_data *d, int hc)
{
	struct stm32_hc_active *a = &d->hc_act[hc];
	struct uhc_transfer *xfer = a->xfer;

	if ((xfer == NULL) || (xfer->udev == NULL)) {
		return;
	}

	const HCD_URBStateTypeDef urb_state = HAL_HCD_HC_GetURBState(&d->hhcd, (uint8_t)hc);
	const uint32_t hal_cnt =
		uhc_stm32_hc_done_bytes(d, (uint8_t)hc, a->req_len, a->dir_in != 0u);

	if (uhc_stm32_nonctrl_handle_wait_or_error(dev, d, a, hc, urb_state, hal_cnt)) {
		return;
	}

	uhc_stm32_nonctrl_complete_done(dev, d, a, hc, hal_cnt);
}

/* Scheduling */

static void uhc_stm32_schedule_nonctrl_fill(const struct device *dev, struct stm32_otghs_data *d)
{
	struct uhc_data *ud = dev->data;

	if (sys_dlist_is_empty(&ud->bulk_xfers)) {
		return;
	}

	while (true) {
		uint32_t active = 0;
		bool saw_candidate = false;
		bool made_progress = false;
		struct uhc_transfer *x;

		for (uint8_t i = 0; i < d->host_channels; i++) {
			active += (d->hc_act[i].xfer != NULL) ? 1u : 0u;
		}

		if (active >= STM32_OTGHS_MAX_INFLIGHT_BULK) {
			break;
		}

		SYS_DLIST_FOR_EACH_CONTAINER(&ud->bulk_xfers, x, node) {
			if ((STM32_EP_GET_IDX(x->ep) == 0u) || uhc_stm32_xfer_is_active(d, x)) {
				continue;
			}

			saw_candidate = true;

			const int r = uhc_stm32_submit_nonctrl(d, x);

			if (r == 0) {
				made_progress = true;
				break;
			}

			if (r == -EBUSY) {
				/* Same endpoint already active; try another queued transfer. */
				continue;
			}

			uhc_xfer_return(dev, x, r);
		}

		if (!saw_candidate || !made_progress) {
			break;
		}
	}
}

static void uhc_stm32_schedule_xfers(const struct device *dev, struct stm32_otghs_data *d)
{
	if (!d->connected || !d->bus_ready) {
		return;
	}

	if (d->ctrl.xfer == NULL) {
		struct uhc_transfer *x = uhc_stm32_peek_next_ctrl_xfer(dev);

		if (x != NULL) {
			const int r = uhc_stm32_submit_control(d, x);

			if (r == -EBUSY) {
				return;
			}
			if (r != 0) {
				uhc_xfer_return(dev, x, r);
			}
		}
	}

	uhc_stm32_schedule_nonctrl_fill(dev, d);
}

/* Timer callback */

static void uhc_stm32_periodic_wake_stop(struct stm32_otghs_data *d)
{
	atomic_set(&d->sof_tick_active, 0);
	k_timer_stop(&d->kick_timer);
}

static void uhc_stm32_periodic_wake_update(struct stm32_otghs_data *d)
{
	const bool need_periodic = d->connected && d->bus_ready && uhc_stm32_any_nak_wait(d);

	if (atomic_get(&d->sof_irq_enabled) != 0) {
		atomic_set(&d->sof_tick_active, need_periodic ? 1 : 0);
		k_timer_stop(&d->kick_timer);
		return;
	}

	atomic_set(&d->sof_tick_active, 0);
	if (need_periodic) {
		k_timer_start(&d->kick_timer, K_MSEC(STM32_OTGHS_KICK_PERIOD_MS),
			      K_MSEC(STM32_OTGHS_KICK_PERIOD_MS));
	} else {
		k_timer_stop(&d->kick_timer);
	}
}

static void uhc_stm32_kick_timer_cb(struct k_timer *t)
{
	struct stm32_otghs_data *d = (struct stm32_otghs_data *)k_timer_user_data_get(t);

	if (d->connected && d->bus_ready && uhc_stm32_any_nak_wait(d)) {
		uhc_stm32_wake_worker_thread(d);
	}
}

/* ISR */

static void uhc_stm32_otghs_isr(const struct device *dev)
{
	struct stm32_otghs_data *d = uhc_get_private(dev);

	__ASSERT_NO_MSG(d != NULL);
	__ASSERT_NO_MSG(d->hhcd.Instance != NULL);

	HAL_HCD_IRQHandler(&d->hhcd);
}

/* HAL callbacks */

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd)
{
	uhc_stm32_signal_event_to_worker(hhcd, STM32_EVT_PEN);
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd)
{
	uhc_stm32_signal_event_to_worker(hhcd, STM32_EVT_PDIS);
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
	uhc_stm32_signal_event_to_worker(hhcd, STM32_EVT_CONN);
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
	uhc_stm32_signal_event_to_worker(hhcd, STM32_EVT_DISCONN);
}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
	struct stm32_otghs_data *d = hhcd->pData;

	__ASSERT_NO_MSG(d != NULL);

	if (atomic_get(&d->sof_tick_active) != 0) {
		uhc_stm32_wake_worker_thread(d);
	}
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum,
					 HCD_URBStateTypeDef urb_state)
{
	struct stm32_otghs_data *d = hhcd->pData;

	__ASSERT_NO_MSG(d != NULL);

	/*
	 * Do not wake the worker for every transient URB_NOTREADY / URB_IDLE /
	 * URB_NYET. That causes useless thread churn under NAK storms.
	 *
	 * Wake immediately only when HAL has halted an OUT/control-OUT channel
	 * and left it disabled, which means software must submit again
	 * (for example, HS NYET or ping/ACK paths). HAL-parking in URB_NAK_WAIT
	 * also wakes the worker; the timer then rate-limits retries.
	 */
	switch (urb_state) {
	case URB_NOTREADY:
	case URB_IDLE:
		if (uhc_stm32_nonctrl_needs_sw_retry(d, chnum, urb_state) ||
		    uhc_stm32_ctrl_needs_sw_retry(d, chnum, urb_state)) {
			uhc_stm32_wake_worker_thread(d);
		}
		break;

	case URB_NYET:
		break;

	default:
		uhc_stm32_wake_worker_thread(d);
		break;
	}
}

/* Thread */

static void uhc_stm32_thread_handle_disconnect(const struct device *dev, struct stm32_otghs_data *d,
					   bool *was_present)
{
	uhc_stm32_periodic_wake_stop(d);

	d->connected = false;
	d->bus_ready = false;
	d->reset_pending = false;
	d->ep0_mps = 0u;

	uhc_stm32_free_all_channels(d);
	uhc_stm32_pipe_cancel_and_return(dev, d, &d->ctrl, -ENODEV);
	uhc_stm32_cancel_all_nonctrl(dev, d, -ENODEV);

	if (*was_present) {
		uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
	}

	*was_present = false;
}

static void uhc_stm32_thread_handle_connect(const struct device *dev, struct stm32_otghs_data *d,
					bool *was_present)
{
	uhc_stm32_dump_hprt(d, "EVT:CONNECT");

	d->connected = true;
	d->bus_ready = false;
	d->reset_pending = false;
	d->ep0_mps = USB_EP0_MPS_8;
	d->zlp_dummy = 0u;

	uhc_stm32_periodic_wake_stop(d);
	uhc_stm32_free_all_channels(d);
	uhc_stm32_pipe_clear(&d->ctrl);
	uhc_stm32_hc_act_clear_all(d);
	uhc_stm32_update_dev_speed_from_hal(d);

	if (d->speed == HCD_DEVICE_SPEED_HIGH) {
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_HS, 0);
	} else if (d->speed == HCD_DEVICE_SPEED_LOW) {
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_LS, 0);
	} else {
		uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0);
	}

	*was_present = true;
}

static void uhc_stm32_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct stm32_otghs_data *d = p1;
	const struct device *dev = d->dev;
	bool was_present = false;

	while (true) {
		const uint32_t flags = k_event_wait_safe(&d->events, UINT32_MAX, false,
							 K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);

		if ((flags & (STM32_EVT_DISCONN | STM32_EVT_PDIS)) != 0u) {
			uhc_stm32_thread_handle_disconnect(dev, d, &was_present);
			uhc_unlock_internal(dev);
			continue;
		}

		if ((flags & STM32_EVT_CONN) != 0u) {
			uhc_stm32_thread_handle_connect(dev, d, &was_present);
		}

		if ((flags & STM32_EVT_PEN) != 0u) {
			uhc_stm32_dump_hprt(d, "EVT:PEN");
			uhc_stm32_update_dev_speed_from_hal(d);

			if (d->reset_pending) {
				d->reset_pending = false;
				k_msleep(STM32_OTGHS_POST_RESET_DELAY_MS);

				d->bus_ready = true;
				uhc_stm32_periodic_wake_update(d);

				uhc_submit_event(dev, UHC_EVT_RESETED, 0);
			}
		}

		if (d->reset_pending && sys_timepoint_expired(d->reset_timepoint) &&
		    ((uhc_stm32_read_hprt(d) & USB_OTG_HPRT_PENA) != 0u)) {
			d->reset_pending = false;
			d->bus_ready = true;
			uhc_stm32_periodic_wake_update(d);

			uhc_submit_event(dev, UHC_EVT_RESETED, 0);
		}

		if (!d->connected || !d->bus_ready) {
			uhc_unlock_internal(dev);
			continue;
		}

		if (d->ctrl.xfer != NULL) {
			uhc_stm32_complete_control_pipe(dev, d, &d->ctrl);
		}

		for (uint8_t hc = 0; hc < d->host_channels; hc++) {
			if (d->hc_act[hc].xfer != NULL) {
				uhc_stm32_complete_one_hc(dev, d, (int)hc);
			}
		}

		uhc_stm32_schedule_xfers(dev, d);
		uhc_stm32_periodic_wake_update(d);

		uhc_unlock_internal(dev);
	}
}

/* Hardware init helpers */

static void uhc_stm32_log_feature_config(const struct stm32_otghs_data *d __maybe_unused)
{
	const bool sof_irq_enabled __maybe_unused = (atomic_get(&d->sof_irq_enabled) != 0);

	LOG_INF("[CFG] dma=%s sof=%s wake=%s force_vbus_valid=%s "
		"vbus_sensing=%s in_nak_park=%s",
		IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA) ? "on" : "off",
		sof_irq_enabled ? "on" : "off",
		sof_irq_enabled ? "sof" : "timer",
		IS_ENABLED(CONFIG_UHC_STM32_OTGHS_FORCE_VBUS_VALID) ? "on" : "off",
		IS_ENABLED(CONFIG_UHC_STM32_OTGHS_FORCE_VBUS_VALID) ? "off" : "on",
		STM32_OTGHS_DIRECT_IN_NAK_PARKING ? "on" : "off");

	if (STM32_OTGHS_DIRECT_IN_NAK_PARKING) {
		LOG_INF("[CFG] in_nak_wait_limit=%u retry_ms=%u dma_align=%u",
			(unsigned int)STM32_OTGHS_HAL_IN_NAK_WAIT_LIMIT,
			(unsigned int)STM32_OTGHS_NAK_WAIT_RETRY_MS,
			(unsigned int)STM32_OTGHS_DMA_ALIGNMENT);
	} else {
		LOG_INF("[CFG] in_nak_wait_limit=disabled retry_ms=%u dma_align=%u",
			(unsigned int)STM32_OTGHS_NAK_WAIT_RETRY_MS,
			(unsigned int)STM32_OTGHS_DMA_ALIGNMENT);
	}
}

static int uhc_stm32_clock_enable(const struct stm32_otghs_config *cfg)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int ret;

	if (cfg->num_clocks > 1) {
		ret = clock_control_configure(clk, &cfg->pclken[1], NULL);
		if (ret != 0) {
			LOG_ERR("Could not select USB domain clock");
			return ret;
		}
	}

	ret = clock_control_on(clk, &cfg->pclken[0]);
	if (ret != 0) {
		LOG_ERR("Unable to enable USB clock");
		return ret;
	}

	if (cfg->phy != NULL) {
		ret = cfg->phy->enable(cfg->phy);
		if (ret != 0) {
			int off_ret;

			LOG_ERR("Failed to enable USB PHY: %d", ret);
			off_ret = clock_control_off(clk, &cfg->pclken[0]);
			if (off_ret != 0) {
				LOG_WRN("Unable to roll back USB clock enable: %d", off_ret);
			}
			return ret;
		}
	}

	return 0;
}

static int uhc_stm32_clock_disable(const struct stm32_otghs_config *cfg)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	int ret = 0;
	int err;

	if (cfg->phy != NULL) {
		err = cfg->phy->disable(cfg->phy);
		if (err != 0) {
			LOG_ERR("Failed to disable USB PHY: %d", err);
			ret = err;
		}
	}

	err = clock_control_off(clk, &cfg->pclken[0]);
	if (err != 0) {
		LOG_ERR("Unable to disable USB clock: %d", err);
		if (ret == 0) {
			ret = err;
		}
	}

	return ret;
}

static int uhc_stm32_clock_power_disable(const struct stm32_otghs_config *cfg)
{
	int ret = 0;
	int err;

	err = uhc_stm32_clock_disable(cfg);
	if (err != 0) {
		ret = err;
	}

	err = stm32_usb_pwr_disable();
	if (err != 0) {
		LOG_ERR("Unable to disable USB power: %d", err);
		if (ret == 0) {
			ret = err;
		}
	}

	return ret;
}

/* UHC API */

static int uhc_stm32_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int uhc_stm32_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

static int uhc_stm32_init(const struct device *dev)
{
	struct stm32_otghs_data *d = uhc_get_private(dev);
	const struct stm32_otghs_config *cfg = dev->config;
	int ret;

	d->connected = false;
	d->bus_ready = false;
	d->reset_pending = false;
	d->host_channels = cfg->host_channels;
	atomic_set(&d->sof_irq_enabled, 0);
	atomic_set(&d->sof_tick_active, 0);

	uhc_stm32_pipe_clear(&d->ctrl);
	uhc_stm32_hc_act_clear_all(d);

	uhc_stm32_free_all_channels(d);

	d->ep0_mps = USB_EP0_MPS_8;
	d->zlp_dummy = 0u;

	ret = stm32_usb_pwr_enable();
	if (ret != 0) {
		LOG_ERR("Error enabling USB power: %d", ret);
		return ret;
	}

	ret = uhc_stm32_clock_enable(cfg);
	if (ret != 0) {
		const int disable_ret = stm32_usb_pwr_disable();

		if (disable_ret != 0) {
			LOG_WRN("Unable to roll back USB power enable: %d", disable_ret);
		}
		return ret;
	}

	memset(&d->hhcd, 0, sizeof(d->hhcd));
	d->hhcd.Instance = cfg->base;

	d->hhcd.Init.Host_channels = cfg->host_channels;
	d->hhcd.Init.speed = HCD_SPEED_HIGH;
	d->speed = HCD_DEVICE_SPEED_HIGH;

	d->hhcd.Init.dma_enable = IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA);
	d->hhcd.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
	d->hhcd.Init.Sof_enable = IS_ENABLED(CONFIG_UHC_STM32_OTGHS_SOF);
	d->hhcd.Init.low_power_enable = DISABLE;
	d->hhcd.Init.use_external_vbus = !IS_ENABLED(CONFIG_UHC_STM32_OTGHS_FORCE_VBUS_VALID);
	d->hhcd.Init.vbus_sensing_enable = !IS_ENABLED(CONFIG_UHC_STM32_OTGHS_FORCE_VBUS_VALID);

	d->hhcd.pData = d;

	stm32_status_t status = HAL_HCD_Init(&d->hhcd);

	if (status != HAL_OK) {
		int cleanup_ret;

		LOG_ERR("HAL_HCD_Init failed");
		cleanup_ret = uhc_stm32_clock_power_disable(cfg);
		if (cleanup_ret != 0) {
			LOG_WRN("USB cleanup after HAL_HCD_Init failure returned %d", cleanup_ret);
		}
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_UHC_STM32_OTGHS_FORCE_VBUS_VALID)) {
		uhc_stm32_disable_vbus_sensing(d);
		uhc_stm32_force_session_valid(d);
	} else {
		uhc_stm32_enable_vbus_sensing(d);
	}

	status = HAL_HCD_Start(&d->hhcd);
	if (status != HAL_OK) {
		int cleanup_ret;
		stm32_status_t deinit_status;

		LOG_ERR("HAL_HCD_Start failed");
		deinit_status = HAL_HCD_DeInit(&d->hhcd);
		if (deinit_status != HAL_OK) {
			LOG_WRN("HAL_HCD_DeInit after start failure returned %d",
				(int)deinit_status);
		}
		cleanup_ret = uhc_stm32_clock_power_disable(cfg);
		if (cleanup_ret != 0) {
			LOG_WRN("USB cleanup after HAL_HCD_Start failure returned %d", cleanup_ret);
		}
		return -EIO;
	}

	stm32_reg_write(&d->hhcd.Instance->GINTSTS, UINT32_MAX);
	atomic_set(&d->sof_irq_enabled, IS_ENABLED(CONFIG_UHC_STM32_OTGHS_SOF));
	uhc_stm32_log_feature_config(d);

	k_msleep(STM32_OTGHS_POST_START_DELAY_MS);
	uhc_stm32_dump_hprt(d, "INIT");

	return 0;
}

static int uhc_stm32_enable(const struct device *dev)
{
	uhc_stm32_wake_worker_thread(uhc_get_private(dev));
	return 0;
}

static int uhc_stm32_disable(const struct device *dev)
{
	uhc_stm32_periodic_wake_stop(uhc_get_private(dev));
	return 0;
}

static int uhc_stm32_shutdown(const struct device *dev)
{
	struct stm32_otghs_data *d = uhc_get_private(dev);
	const struct stm32_otghs_config *cfg = dev->config;
	stm32_status_t stop_status;
	stm32_status_t deinit_status;
	int ret = 0;
	int cleanup_ret;

	uhc_stm32_periodic_wake_stop(d);

	stop_status = HAL_HCD_Stop(&d->hhcd);
	deinit_status = HAL_HCD_DeInit(&d->hhcd);

	if (stop_status != HAL_OK) {
		LOG_ERR("HAL_HCD_Stop failed: %d", (int)stop_status);
		ret = -EIO;
	}

	if (deinit_status != HAL_OK) {
		LOG_ERR("HAL_HCD_DeInit failed: %d", (int)deinit_status);
		if (ret == 0) {
			ret = -EIO;
		}
	}

	cleanup_ret = uhc_stm32_clock_power_disable(cfg);
	if ((cleanup_ret != 0) && (ret == 0)) {
		ret = cleanup_ret;
	}

	return ret;
}

static int uhc_stm32_bus_reset(const struct device *dev)
{
	struct stm32_otghs_data *d = uhc_get_private(dev);

	d->bus_ready = false;
	d->reset_pending = true;
	d->reset_timepoint = sys_timepoint_calc(K_MSEC(STM32_OTGHS_RESET_FALLBACK_MS));
	uhc_stm32_periodic_wake_stop(d);

	uhc_stm32_pipe_cancel_and_return(dev, d, &d->ctrl, -EAGAIN);
	uhc_stm32_cancel_all_nonctrl(dev, d, -EAGAIN);
	uhc_stm32_free_all_channels(d);

	const stm32_status_t status = HAL_HCD_ResetPort(&d->hhcd);

	if (status != HAL_OK) {
		d->reset_pending = false;
		LOG_ERR("HAL_HCD_ResetPort failed");
		return -EIO;
	}

	return 0;
}

static int uhc_stm32_sof_enable(const struct device *dev)
{
	struct stm32_otghs_data *d = uhc_get_private(dev);

	if (atomic_get(&d->sof_irq_enabled) != 0) {
		return -EALREADY;
	}

	stm32_reg_set_bits(&d->hhcd.Instance->GINTMSK, USB_OTG_GINTMSK_SOFM);
	d->hhcd.Init.Sof_enable = ENABLE;
	atomic_set(&d->sof_irq_enabled, 1);
	uhc_stm32_periodic_wake_update(d);
	LOG_INF("[CFG] SOF interrupt delivery enabled at runtime");
	LOG_INF("[CFG] dma=%s sof=%s wake=%s force_vbus_valid=%s",
		IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA) ? "on" : "off",
		(atomic_get(&d->sof_irq_enabled) != 0) ? "on" : "off",
		(atomic_get(&d->sof_irq_enabled) != 0) ? "sof" : "timer",
		IS_ENABLED(CONFIG_UHC_STM32_OTGHS_FORCE_VBUS_VALID) ? "on" : "off");

	return 0;
}

static int uhc_stm32_bus_suspend(const struct device *dev)
{
	uhc_stm32_periodic_wake_stop(uhc_get_private(dev));
	uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);
	return 0;
}

static int uhc_stm32_bus_resume(const struct device *dev)
{
	uhc_stm32_periodic_wake_update(uhc_get_private(dev));
	uhc_submit_event(dev, UHC_EVT_RESUMED, 0);
	return 0;
}

static int uhc_stm32_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_data *data = dev->data;
	struct stm32_otghs_data *d = uhc_get_private(dev);

	if (STM32_EP_GET_IDX(xfer->ep) == 0u) {
		sys_dlist_append(&data->ctrl_xfers, &xfer->node);
	} else {
		sys_dlist_append(&data->bulk_xfers, &xfer->node);
	}

	uhc_stm32_wake_worker_thread(d);
	return 0;
}

static int uhc_stm32_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_data *data = dev->data;
	struct stm32_otghs_data *d = uhc_get_private(dev);

	if (xfer == NULL) {
		return -EINVAL;
	}

	if (uhc_stm32_list_contains_xfer(&data->ctrl_xfers, xfer) ||
	    uhc_stm32_list_contains_xfer(&data->bulk_xfers, xfer)) {
		/* uhc_xfer_return() removes the transfer from its queued list node. */
		uhc_xfer_return(dev, xfer, -ECONNRESET);
		uhc_stm32_wake_worker_thread(d);
		return 0;
	}

	if (d->ctrl.xfer == xfer) {
		uhc_stm32_pipe_cancel_and_return(dev, d, &d->ctrl, -ECONNRESET);
		uhc_stm32_wake_worker_thread(d);
		return 0;
	}

	const int hc = uhc_stm32_find_active_hc_for_xfer(d, xfer);

	if (hc >= 0) {
		uhc_stm32_hc_halt(d, (uint8_t)hc);
		uhc_xfer_return(dev, xfer, -ECONNRESET);
		uhc_stm32_hc_act_clear(&d->hc_act[hc]);

		uhc_stm32_wake_worker_thread(d);
	}

	return 0;
}

static const struct uhc_api stm32_uhc_api = {
	.lock = uhc_stm32_lock,
	.unlock = uhc_stm32_unlock,
	.init = uhc_stm32_init,
	.enable = uhc_stm32_enable,
	.disable = uhc_stm32_disable,
	.shutdown = uhc_stm32_shutdown,

	.bus_reset = uhc_stm32_bus_reset,
	.sof_enable = uhc_stm32_sof_enable,
	.bus_suspend = uhc_stm32_bus_suspend,
	.bus_resume = uhc_stm32_bus_resume,

	.ep_enqueue = uhc_stm32_ep_enqueue,
	.ep_dequeue = uhc_stm32_ep_dequeue,
};

/* Device init / registration */

static int uhc_stm32_driver_preinit(const struct device *dev)
{
	struct uhc_data *data = dev->data;
	struct stm32_otghs_data *d = data->priv;
	const struct stm32_otghs_config *cfg = dev->config;
	int ret;

	k_mutex_init(&data->mutex);
	d->dev = dev;
	d->host_channels = cfg->host_channels;

	k_event_init(&d->events);

	k_timer_init(&d->kick_timer, uhc_stm32_kick_timer_cb, NULL);
	k_timer_user_data_set(&d->kick_timer, d);

	k_thread_create(&d->thread, cfg->stack, cfg->stack_size, uhc_stm32_thread, d, NULL,
			NULL, cfg->thread_prio, 0, K_NO_WAIT);
	k_thread_name_set(&d->thread, dev->name);

	cfg->irq_connect();

	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);

	/* Ignore -ENOENT returned on series without pinctrl */
	if (ret < 0 && ret != -ENOENT) {
		LOG_ERR("pinctrl apply failed: %d", ret);
		return ret;
	}

	uhc_stm32_wake_worker_thread(d);
	return 0;
}

#define STM32_OTGHS_INST_DEFINE(n)							\
	BUILD_ASSERT(DT_INST_PROP(n, num_host_channels) <= STM32_OTGHS_MAX_CH,		\
		     "STM32 OTGHS host channel count exceeds driver channel "		\
		     "array size");							\
											\
	static void uhc_stm32_irq_connect_##n(void)					\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    uhc_stm32_otghs_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
											\
	static K_KERNEL_STACK_DEFINE(stm32_otghs_stack_##n,				\
				     STM32_OTGHS_STACK_SIZE_DEFAULT);			\
											\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	static const struct stm32_pclken stm32_otghs_pclken_##n[] =			\
		STM32_DT_CLOCKS(DT_DRV_INST(n));					\
											\
	static struct stm32_otghs_data stm32_otghs_priv_##n;				\
											\
	static struct uhc_data stm32_uhc_data_##n = {					\
		.priv = &stm32_otghs_priv_##n,						\
	};										\
											\
	static const struct stm32_otghs_config stm32_otghs_cfg_##n = {			\
		.base = (void *)DT_INST_REG_ADDR(n),					\
		.irq_connect = uhc_stm32_irq_connect_##n,				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.pclken = (struct stm32_pclken *)stm32_otghs_pclken_##n,		\
		.num_clocks = DT_INST_NUM_CLOCKS(n),					\
		.phy = USB_STM32_PHY_PSEUDODEV_GET_OR_NULL(DT_DRV_INST(n)),		\
		.host_channels = DT_INST_PROP(n, num_host_channels),			\
		.stack = stm32_otghs_stack_##n,						\
		.stack_size = K_KERNEL_STACK_SIZEOF(stm32_otghs_stack_##n),		\
		.thread_prio = K_PRIO_PREEMPT(STM32_OTGHS_THREAD_PRIO_DEFAULT),	        \
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, uhc_stm32_driver_preinit, NULL,			\
			      &stm32_uhc_data_##n, &stm32_otghs_cfg_##n,		\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &stm32_uhc_api);

#define STM32_OTGHS_INST_DEFINE_IF_HOST(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, num_host_channels),			\
		    (STM32_OTGHS_INST_DEFINE(n)), ())

DT_INST_FOREACH_STATUS_OKAY(STM32_OTGHS_INST_DEFINE_IF_HOST)
