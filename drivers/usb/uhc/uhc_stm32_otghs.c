/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Narek Aydinyan
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_otghs

#include <stddef.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/uhc.h>
#include <stm32_usb_common.h>

#include "uhc_common.h"
#include "uhc_stm32_otghs.h"

LOG_MODULE_REGISTER(uhc_stm32_otghs, CONFIG_UHC_STM32_OTGHS_LOG_LEVEL);

/*
 * IMPORTANT:
 *
 * Non-DMA builds expect the STM32 HAL HS HCD build to enable:
 *
 *   #define USE_HAL_HCD_IN_NAK_AUTO_ACTIVATE_DISABLE 1
 *   #define HAL_HCD_CHANNEL_NAK_COUNT                4
 *
 * In this STM32U5 HAL, that makes the IN-side host-channel handler stop
 * auto-reactivating a BULK/CTRL channel after HAL_HCD_CHANNEL_NAK_COUNT
 * consecutive NAKs and report URB_NAK_WAIT.
 *
 * The UHC then retries from software at a low rate, while preserving any
 * partial BULK IN payload the HAL already placed into the buffer. OUT retry
 * handling remains separate and follows the HAL's normal OUT state machine.
 *
 * DMA builds do not depend on that HAL NAK parking path for direct HS
 * BULK/CTRL IN traffic. If the HAL limiter is also compiled in, it can still
 * matter for HAL split-transaction corner cases, but the UHC does not require
 * it to build or run in DMA mode.
 */

/* Constants */

#ifndef HCD_PID_SETUP
#define HCD_PID_SETUP 0u
#define HCD_PID_DATA0 2u
#define HCD_PID_DATA1 3u
#endif

#define USB_REQ_DIR_IN_BIT BIT(7)

#define USB_REQ_GET_DESCRIPTOR      0x06u
#define USB_DESC_TYPE_DEVICE        0x01u
#define USB_BMREQTYPE_TYPE_MASK     0x60u
#define USB_BMREQTYPE_TYPE_STANDARD 0x00u

#define USB_EP0_MPS_8  8u
#define USB_EP0_MPS_16 16u
#define USB_EP0_MPS_32 32u
#define USB_EP0_MPS_64 64u

#define STM32_OTGHS_EP_INDEX_MASK           0x0Fu
#define STM32_OTGHS_DESC_ENTRY_MIN_LEN      2u
#define STM32_OTGHS_EP_ATTR_TYPE_MASK       0x03u
#define STM32_OTGHS_EP_MPS_MASK             0x07FFu
#define STM32_OTGHS_SETUP_WLENGTH_OFFSET    6u
#define STM32_OTGHS_DEVICE_DESC_MPS0_OFFSET 7u
#define STM32_OTGHS_DMA_ALIGNMENT           4u

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
#define STM32_OTGHS_KICK_PERIOD_MS      1u
#define STM32_OTGHS_KICK_START_DELAY_MS STM32_OTGHS_KICK_PERIOD_MS

#define STM32_OTGHS_THREAD_PRIO_DEFAULT 14
#define STM32_OTGHS_STACK_SIZE_DEFAULT  4096u

#define STM32_OTGHS_HPRT_PLSTS_MASK  (0x3u << 10)
#define STM32_OTGHS_HPRT_PLSTS(hprt) (((hprt) & STM32_OTGHS_HPRT_PLSTS_MASK) >> 10)

#define STM32_MPS_DEFAULT_BULK 512u
#define STM32_MPS_DEFAULT_INTR 16u

#define STM32_OTGHS_GOTGCTL_AVALOEN  BIT(4)
#define STM32_OTGHS_GOTGCTL_AVALOVAL BIT(5)
#define STM32_OTGHS_GOTGCTL_BVALOEN  BIT(6)
#define STM32_OTGHS_GOTGCTL_BVALOVAL BIT(7)

#if defined(USE_HAL_HCD_IN_NAK_AUTO_ACTIVATE_DISABLE) &&                                           \
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
 * Non-DMA builds require STM32 HAL support for the IN NAK limiter path:
 * USE_HAL_HCD_IN_NAK_AUTO_ACTIVATE_DISABLE=1 and
 * HAL_HCD_CHANNEL_NAK_COUNT>=1.
 */
#error "Non-DMA STM32 OTGHS build requires HAL IN NAK limiter support."
#endif

#if IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA)
#define STM32_OTGHS_DIRECT_IN_NAK_PARKING 0
#else
#define STM32_OTGHS_DIRECT_IN_NAK_PARKING STM32_OTGHS_HAL_IN_NAK_LIMITER_AVAILABLE
#endif

static inline uint32_t otg_speed_for_hs_core(const struct stm32_otghs_data *d);
static inline void stm32_mask_nak_nyet(struct stm32_otghs_data *d, int hc, uint8_t ep_type);
static inline USB_OTG_HostChannelTypeDef *stm32_hc_regs(const struct stm32_otghs_data *d,
							uint8_t hc);
static int stm32_submit_control(struct stm32_otghs_data *d, struct uhc_transfer *xfer);

/* Common helpers */

static inline struct stm32_otghs_data *dev_data(const struct device *dev)
{
	return (struct stm32_otghs_data *)uhc_get_private(dev);
}

static inline k_spinlock_key_t hal_enter(struct stm32_otghs_data *d)
{
	return k_spin_lock(&d->hal_lock);
}

static inline void hal_exit(struct stm32_otghs_data *d, k_spinlock_key_t key)
{
	k_spin_unlock(&d->hal_lock, key);
}

static inline void irq_kick(struct stm32_otghs_data *d)
{
	if (k_sem_count_get(&d->irq_sem) == 0) {
		k_sem_give(&d->irq_sem);
	}
}

static inline bool stm32_dma_enabled(const struct stm32_otghs_data *d)
{
	return d->hhcd.Init.dma_enable != DISABLE;
}

static inline bool stm32_sof_irq_enabled(const struct stm32_otghs_data *d)
{
	return atomic_get(&d->sof_irq_enabled) != 0;
}

static inline uint8_t stm32_ep_index(uint8_t ep)
{
	return USB_EP_GET_IDX(ep) & STM32_OTGHS_EP_INDEX_MASK;
}

static inline bool is_ep0(const struct uhc_transfer *x)
{
	return stm32_ep_index(x->ep) == 0u;
}

static inline void pipe_clear(struct stm32_pipe *p)
{
	p->xfer = NULL;
	p->hc = -1;
	p->ep = 0;
	p->len = 0;
	p->buf_off = 0;
	p->halted_cnt = 0;
	p->start_ms = 0;
	memset(p->setup_words, 0, sizeof(p->setup_words));
}

static inline void hc_act_clear(struct stm32_hc_active *a)
{
	a->xfer = NULL;
	a->ep = 0;
	a->ep_type = 0;
	a->dir_in = 0;
	a->req_len = 0;
	a->buf_off = 0;
	a->halted_cnt = 0;
	a->mps = 0;
	a->start_ms = 0;
}

static void hc_act_clear_all(struct stm32_otghs_data *d)
{
	for (uint32_t i = 0; i < STM32_OTGHS_MAX_CH; i++) {
		hc_act_clear(&d->hc_act[i]);
	}
}

static bool stm32_xfer_is_active(struct stm32_otghs_data *d, struct uhc_transfer *x)
{
	for (uint32_t i = 0; i < STM32_OTGHS_MAX_CH; i++) {
		if (d->hc_act[i].xfer == x) {
			return true;
		}
	}
	return false;
}

static int stm32_find_active_hc_for_xfer(struct stm32_otghs_data *d, struct uhc_transfer *x)
{
	for (uint32_t i = 0; i < STM32_OTGHS_MAX_CH; i++) {
		if (d->hc_act[i].xfer == x) {
			return (int)i;
		}
	}
	return -1;
}

static uint32_t stm32_count_active_nonctrl(struct stm32_otghs_data *d)
{
	uint32_t n = 0;

	for (uint32_t i = 0; i < STM32_OTGHS_MAX_CH; i++) {
		if (d->hc_act[i].xfer != NULL) {
			n++;
		}
	}
	return n;
}

static inline int fix_status(int st)
{
	if (st == -EINPROGRESS) {
		return 0;
	}
	if (st > 0) {
		return -EIO;
	}
	return st;
}

static inline void return_fixed(const struct device *dev, struct uhc_transfer *xfer, int status)
{
	uhc_xfer_return(dev, xfer, fix_status(status));
}

static inline bool stm32_urb_waiting_on_device(HCD_URBStateTypeDef urb_state)
{
	return (urb_state == URB_NOTREADY) || (urb_state == URB_NYET) || (urb_state == URB_IDLE);
}

static inline void pipe_halt_hw(struct stm32_otghs_data *d, const struct stm32_pipe *p)
{
	if (p->hc >= 0) {
		k_spinlock_key_t k = hal_enter(d);
		(void)HAL_HCD_HC_Halt(&d->hhcd, (uint8_t)p->hc);
		hal_exit(d, k);
	}
}

static void stm32_pipe_finish(const struct device *dev, struct stm32_pipe *p, int status)
{
	struct uhc_transfer *completed = p->xfer;

	pipe_clear(p);
	if (completed) {
		return_fixed(dev, completed, status);
	}
}

static void stm32_active_xfer_finish(const struct device *dev, struct stm32_hc_active *a,
				     int status)
{
	struct uhc_transfer *completed = a->xfer;

	hc_act_clear(a);
	if (completed) {
		return_fixed(dev, completed, status);
	}
}

static void pipe_cancel_and_return(const struct device *dev, struct stm32_otghs_data *d,
				   struct stm32_pipe *p, int status)
{
	if (p->xfer) {
		pipe_halt_hw(d, p);
		stm32_pipe_finish(dev, p, status);
	}
}

static HCD_URBStateTypeDef stm32_get_urb_state(struct stm32_otghs_data *d, uint8_t hc)
{
	k_spinlock_key_t k = hal_enter(d);
	const HCD_URBStateTypeDef s = HAL_HCD_HC_GetURBState(&d->hhcd, hc);

	hal_exit(d, k);
	return s;
}

static uint32_t stm32_get_xfer_count(struct stm32_otghs_data *d, uint8_t hc)
{
	k_spinlock_key_t k = hal_enter(d);
	const uint32_t n = HAL_HCD_HC_GetXferCount(&d->hhcd, hc);

	hal_exit(d, k);
	return n;
}

static inline bool stm32_dma_buffer_is_aligned(const void *ptr)
{
	return IS_ALIGNED((uintptr_t)ptr, STM32_OTGHS_DMA_ALIGNMENT);
}

static int stm32_hc_submit_request(struct stm32_otghs_data *d, uint8_t hc, bool dir_in,
				   uint8_t ep_type, uint8_t pid, uint8_t *data_ptr,
				   uint16_t transfer_len)
{
	if (stm32_dma_enabled(d) && (data_ptr != NULL) && !stm32_dma_buffer_is_aligned(data_ptr)) {
		LOG_ERR("DMA buffer for HC %u is not %u-byte aligned: %p", (unsigned int)hc,
			STM32_OTGHS_DMA_ALIGNMENT, data_ptr);
		return -EINVAL;
	}

	k_spinlock_key_t k = hal_enter(d);
	const HAL_StatusTypeDef st = HAL_HCD_HC_SubmitRequest(
		&d->hhcd, hc, dir_in ? 1u : 0u, ep_type, pid, data_ptr, transfer_len, 0u);
	hal_exit(d, k);

	return (st == HAL_OK) ? 0 : -EIO;
}

static uint32_t stm32_dma_in_done_bytes(struct stm32_otghs_data *d, uint8_t hc)
{
	uint32_t completed;
	k_spinlock_key_t k = hal_enter(d);
	const uint32_t remaining = stm32_hc_regs(d, hc)->HCTSIZ & USB_OTG_HCTSIZ_XFRSIZ;
	const uint32_t programmed = d->hhcd.hc[hc].XferSize;

	hal_exit(d, k);

	completed = (programmed >= remaining) ? (programmed - remaining) : 0u;

	return completed;
}

static uint32_t stm32_hc_done_bytes(struct stm32_otghs_data *d, uint8_t hc, uint16_t req_len,
				    bool dir_in)
{
	uint32_t completed;

	if (dir_in && stm32_dma_enabled(d)) {
		completed = stm32_dma_in_done_bytes(d, hc);
	} else {
		completed = stm32_get_xfer_count(d, hc);
	}

	return MIN(completed, (uint32_t)req_len);
}

static void stm32_hc_init(struct stm32_otghs_data *d, uint8_t hc, uint8_t hal_ep, uint8_t dev_addr,
			  uint8_t ep_type, uint16_t mps)
{
	k_spinlock_key_t k = hal_enter(d);

	(void)HAL_HCD_HC_Init(&d->hhcd, hc, hal_ep, dev_addr, otg_speed_for_hs_core(d), ep_type,
			      mps);
	stm32_mask_nak_nyet(d, (int)hc, ep_type);

	hal_exit(d, k);
}

static inline USB_OTG_HostChannelTypeDef *stm32_hc_regs(const struct stm32_otghs_data *d,
							uint8_t hc)
{
	return (USB_OTG_HostChannelTypeDef *)((uintptr_t)d->hhcd.Instance +
					      (uintptr_t)USB_OTG_HOST_CHANNEL_BASE +
					      ((uintptr_t)hc *
					       (uintptr_t)USB_OTG_HOST_CHANNEL_SIZE));
}

static inline bool stm32_hc_is_enabled_nolock(const struct stm32_otghs_data *d, uint8_t hc)
{
	return (stm32_hc_regs(d, hc)->HCCHAR & USB_OTG_HCCHAR_CHENA) != 0u;
}

static bool stm32_hc_is_enabled(struct stm32_otghs_data *d, uint8_t hc)
{
	k_spinlock_key_t k = hal_enter(d);
	const bool enabled = stm32_hc_is_enabled_nolock(d, hc);

	hal_exit(d, k);
	return enabled;
}

static bool stm32_nonctrl_needs_sw_retry(struct stm32_otghs_data *d, uint8_t hc,
					 HCD_URBStateTypeDef urb_state, bool locked)
{
	struct stm32_hc_active *a = &d->hc_act[hc];

	if (!a->xfer || a->dir_in) {
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
	return locked ? !stm32_hc_is_enabled_nolock(d, hc) : !stm32_hc_is_enabled(d, hc);
}

static bool stm32_ctrl_needs_sw_retry(struct stm32_otghs_data *d, uint8_t hc,
				      HCD_URBStateTypeDef urb_state, bool locked)
{
	if (!d->ctrl.xfer || (d->ctrl.hc != (int)hc)) {
		return false;
	}

	if (USB_EP_DIR_IS_IN(d->ctrl.ep)) {
		return false;
	}

	if ((urb_state != URB_NOTREADY) && (urb_state != URB_IDLE)) {
		return false;
	}

	return locked ? !stm32_hc_is_enabled_nolock(d, hc) : !stm32_hc_is_enabled(d, hc);
}

static uint32_t stm32_nonctrl_done_bytes(struct stm32_otghs_data *d, int hc,
					 const struct stm32_hc_active *a)
{
	return stm32_hc_done_bytes(d, (uint8_t)hc, a->req_len, a->dir_in != 0u);
}

static int stm32_nonctrl_commit_halted_in(struct stm32_hc_active *a, uint32_t hal_cnt)
{
	struct uhc_transfer *xfer = a->xfer;

	if (!a->dir_in || !xfer || !xfer->buf) {
		return 0;
	}

	if (hal_cnt < (uint32_t)a->halted_cnt) {
		return -EIO;
	}

	const uint32_t add = hal_cnt - (uint32_t)a->halted_cnt;

	if (add == 0u) {
		return 0;
	}

	if ((add > (uint32_t)net_buf_tailroom(xfer->buf)) ||
	    (add > ((uint32_t)a->req_len - (uint32_t)a->halted_cnt))) {
		return -EIO;
	}

	(void)net_buf_add(xfer->buf, add);
	a->buf_off += (uint16_t)add;
	a->halted_cnt += (uint16_t)add;

	return 0;
}

static int stm32_control_commit_halted_in(struct stm32_pipe *p, uint32_t hal_cnt)
{
	struct uhc_transfer *xfer = p->xfer;

	if (!xfer || !xfer->buf) {
		return 0;
	}

	if (hal_cnt < (uint32_t)p->halted_cnt) {
		return -EIO;
	}

	const uint32_t add = hal_cnt - (uint32_t)p->halted_cnt;

	if (add == 0u) {
		return 0;
	}

	if ((add > (uint32_t)net_buf_tailroom(xfer->buf)) ||
	    (add > ((uint32_t)p->len - (uint32_t)p->halted_cnt))) {
		return -EIO;
	}

	(void)net_buf_add(xfer->buf, add);
	p->buf_off += (uint16_t)add;
	p->halted_cnt += (uint16_t)add;

	return 0;
}

/* HPRT helpers */

static inline uint32_t stm32_hprt_rw1c_mask(void)
{
	uint32_t m = 0;
#ifdef USB_OTG_HPRT_PENCHNG
	m |= USB_OTG_HPRT_PENCHNG;
#endif
#ifdef USB_OTG_HPRT_POCCHNG
	m |= USB_OTG_HPRT_POCCHNG;
#endif
#ifdef USB_OTG_HPRT_PESCCHNG
	m |= USB_OTG_HPRT_PESCCHNG;
#endif
#ifdef USB_OTG_HPRT_PPRTCHNG
	m |= USB_OTG_HPRT_PPRTCHNG;
#endif
	return m;
}

static inline volatile uint32_t *stm32_hprt_reg(const struct stm32_otghs_data *d)
{
	return (volatile uint32_t *)((uintptr_t)d->hhcd.Instance +
				     (uintptr_t)USB_OTG_HOST_PORT_BASE);
}

static inline uint32_t stm32_read_hprt(struct stm32_otghs_data *d)
{
	return *stm32_hprt_reg(d);
}

static inline void stm32_write_hprt(struct stm32_otghs_data *d, uint32_t v)
{
	v |= stm32_hprt_rw1c_mask();
	*stm32_hprt_reg(d) = v;
}

static inline void stm32_modify_hprt(struct stm32_otghs_data *d, uint32_t set, uint32_t clr)
{
	uint32_t v = stm32_read_hprt(d);

	v &= ~clr;
	v |= set;
	stm32_write_hprt(d, v);
}

static void stm32_dump_hprt(struct stm32_otghs_data *d, const char *tag)
{
	const uint32_t h = stm32_read_hprt(d);

	const unsigned int pcsts = (h & USB_OTG_HPRT_PCSTS) ? 1u : 0u;
	const unsigned int pena = (h & USB_OTG_HPRT_PENA) ? 1u : 0u;
	const unsigned int prst = (h & USB_OTG_HPRT_PRST) ? 1u : 0u;
	const unsigned int ppwr = (h & USB_OTG_HPRT_PPWR) ? 1u : 0u;

	const unsigned int pspd = (unsigned int)((h & USB_OTG_HPRT_PSPD) >> USB_OTG_HPRT_PSPD_Pos);
	const unsigned int plsts = (unsigned int)STM32_OTGHS_HPRT_PLSTS(h);

	LOG_INF("[%s] HPRT=0x%08x pcsts=%u pena=%u prst=%u ppwr=%u pspd=%u plsts=%u", tag,
		(unsigned int)h, pcsts, pena, prst, ppwr, pspd, plsts);
}

static inline void stm32_port_power_on(struct stm32_otghs_data *d)
{
#ifdef USB_OTG_HPRT_PPWR
	stm32_modify_hprt(d, USB_OTG_HPRT_PPWR, 0u);
#endif
}

static inline void stm32_update_dev_speed_from_hal(struct stm32_otghs_data *d)
{
	const uint32_t spd = HAL_HCD_GetCurrentSpeed(&d->hhcd);

	d->speed = (spd == HCD_SPEED_HIGH) ? HCD_SPEED_HIGH : HCD_SPEED_FULL;
}

static inline uint32_t otg_speed_for_hs_core(const struct stm32_otghs_data *d)
{
	return (uint32_t)d->speed;
}

/* VBUS helpers */

static void stm32_disable_vbus_sensing(struct stm32_otghs_data *d)
{
	USB_OTG_GlobalTypeDef *usb = (USB_OTG_GlobalTypeDef *)d->hhcd.Instance;

#ifdef USB_OTG_GCCFG_NOVBUSSENS
	usb->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
#endif
#ifdef USB_OTG_GCCFG_VBUSASEN
	usb->GCCFG &= ~USB_OTG_GCCFG_VBUSASEN;
#endif
#ifdef USB_OTG_GCCFG_VBUSBSEN
	usb->GCCFG &= ~USB_OTG_GCCFG_VBUSBSEN;
#endif

#ifndef USB_OTG_GCCFG_NOVBUSSENS
	usb->GCCFG |= BIT(21);
#endif
#ifndef USB_OTG_GCCFG_VBUSASEN
	usb->GCCFG &= ~BIT(18);
#endif
#ifndef USB_OTG_GCCFG_VBUSBSEN
	usb->GCCFG &= ~BIT(19);
#endif

	k_busy_wait(STM32_OTGHS_VBUS_SETTLE_US);
	LOG_INF("[VBUS-SENS] GCCFG=0x%08x", (unsigned int)usb->GCCFG);
}

static void stm32_enable_vbus_sensing(struct stm32_otghs_data *d)
{
	USB_OTG_GlobalTypeDef *usb = (USB_OTG_GlobalTypeDef *)d->hhcd.Instance;

#ifdef USB_OTG_GCCFG_NOVBUSSENS
	usb->GCCFG &= ~USB_OTG_GCCFG_NOVBUSSENS;
#endif
#ifdef USB_OTG_GCCFG_VBUSASEN
	usb->GCCFG |= USB_OTG_GCCFG_VBUSASEN;
#endif
#ifdef USB_OTG_GCCFG_VBUSBSEN
	usb->GCCFG |= USB_OTG_GCCFG_VBUSBSEN;
#endif
#ifdef USB_OTG_GCCFG_VBDEN
	usb->GCCFG |= USB_OTG_GCCFG_VBDEN;
#endif
#ifdef USB_OTG_GCCFG_VBVALEXTOEN
	usb->GCCFG &= ~USB_OTG_GCCFG_VBVALEXTOEN;
#endif
#ifdef USB_OTG_GCCFG_VBVALOVAL
	usb->GCCFG &= ~USB_OTG_GCCFG_VBVALOVAL;
#endif

#ifndef USB_OTG_GCCFG_NOVBUSSENS
	usb->GCCFG &= ~BIT(21);
#endif
#ifndef USB_OTG_GCCFG_VBUSASEN
	usb->GCCFG |= BIT(18);
#endif
#ifndef USB_OTG_GCCFG_VBUSBSEN
	usb->GCCFG |= BIT(19);
#endif

	usb->GOTGCTL &= ~(STM32_OTGHS_GOTGCTL_AVALOEN | STM32_OTGHS_GOTGCTL_AVALOVAL |
			  STM32_OTGHS_GOTGCTL_BVALOEN | STM32_OTGHS_GOTGCTL_BVALOVAL);

	k_busy_wait(STM32_OTGHS_VBUS_SETTLE_US);
	LOG_INF("[VBUS-SENS] GCCFG=0x%08x GOTGCTL=0x%08x", (unsigned int)usb->GCCFG,
		(unsigned int)usb->GOTGCTL);
}

static void stm32_force_session_valid(struct stm32_otghs_data *d)
{
	USB_OTG_GlobalTypeDef *usb = (USB_OTG_GlobalTypeDef *)d->hhcd.Instance;

	usb->GOTGCTL |= (STM32_OTGHS_GOTGCTL_AVALOEN | STM32_OTGHS_GOTGCTL_AVALOVAL |
			 STM32_OTGHS_GOTGCTL_BVALOEN | STM32_OTGHS_GOTGCTL_BVALOVAL);

	k_busy_wait(STM32_OTGHS_VBUS_SETTLE_US);
	LOG_INF("[VBUS-OVR] GOTGCTL=0x%08x", (unsigned int)usb->GOTGCTL);
}

static void stm32_otghs_msp_like_init(void)
{
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	__HAL_RCC_SYSCFG_CLK_ENABLE();

	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USBPHY;
	PeriphClkInit.UsbPhyClockSelection = RCC_USBPHYCLKSOURCE_PLL1_DIV2;
	(void)HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

	HAL_SYSCFG_SetOTGPHYReferenceClockSelection(SYSCFG_OTG_HS_PHY_CLK_SELECT_1);

	__HAL_RCC_USB_OTG_HS_CLK_ENABLE();
	__HAL_RCC_USBPHYC_CLK_ENABLE();

	if (__HAL_RCC_PWR_IS_CLK_DISABLED()) {
		__HAL_RCC_PWR_CLK_ENABLE();
	}

	HAL_PWREx_EnableVddUSB();
	HAL_PWREx_EnableUSBHSTranceiverSupply();

	HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);
}

/* Shared endpoint toggle bookkeeping (bulk only) */

static void stm32_toggle_reset_all(struct stm32_otghs_data *d)
{
	for (uint32_t i = 0; i < STM32_TOGGLE_SLOTS; i++) {
		d->tog[i].in_use = false;
		d->tog[i].dev_addr = 0;
		d->tog[i].ep = 0;
		d->tog[i].toggle = 0;
	}
}

static uint32_t stm32_packets_from_bytes(uint32_t bytes, uint16_t mps)
{
	if (mps == 0u) {
		return 0u;
	}
	return (bytes + (uint32_t)mps - 1u) / (uint32_t)mps;
}

static uint8_t stm32_bulk_pid_from_hc(struct stm32_otghs_data *d, int hc, bool dir_in)
{
	uint8_t tog;

	k_spinlock_key_t k = hal_enter(d);

	tog = dir_in ? d->hhcd.hc[hc].toggle_in : d->hhcd.hc[hc].toggle_out;
	hal_exit(d, k);

	return tog ? HCD_PID_DATA1 : HCD_PID_DATA0;
}

/* Zephyr list helpers */

static struct uhc_transfer *stm32_xfer_from_node(sys_dnode_t *node)
{
	return (struct uhc_transfer *)((uint8_t *)node - offsetof(struct uhc_transfer, node));
}

static struct uhc_transfer *peek_next_ctrl_ep0(const struct device *dev)
{
	struct uhc_data *data = dev->data;
	sys_dnode_t *node;

	if (!data || sys_dlist_is_empty(&data->ctrl_xfers)) {
		return NULL;
	}

	for (node = sys_dlist_peek_head(&data->ctrl_xfers); node != NULL;
	     node = sys_dlist_peek_next(&data->ctrl_xfers, node)) {
		struct uhc_transfer *x = stm32_xfer_from_node(node);

		if (is_ep0(x)) {
			return x;
		}
	}

	return NULL;
}

static bool stm32_list_contains_xfer(sys_dlist_t *list, struct uhc_transfer *xfer)
{
	sys_dnode_t *node;

	for (node = sys_dlist_peek_head(list); node != NULL;
	     node = sys_dlist_peek_next(list, node)) {
		struct uhc_transfer *tmp = stm32_xfer_from_node(node);

		if (tmp == xfer) {
			return true;
		}
	}

	return false;
}

/* Channel management */

static int stm32_find_chan(struct stm32_otghs_data *d, uint8_t dev_addr, uint8_t ep,
			   uint8_t ep_type, uint16_t mps)
{
	const bool ep0 = stm32_ep_index(ep) == 0u;

	for (uint32_t i = 0u; i < STM32_OTGHS_MAX_CH; i++) {
		if (!d->ch[i].in_use) {
			continue;
		}
		if (d->ch[i].dev_addr != dev_addr) {
			continue;
		}
		if (d->ch[i].ep != ep) {
			continue;
		}
		if (d->ch[i].ep_type != ep_type) {
			continue;
		}
		if (!ep0 && (d->ch[i].mps != mps)) {
			continue;
		}
		return (int)d->ch[i].hc_num;
	}

	return -ENOENT;
}

static void stm32_free_all_channels(struct stm32_otghs_data *d)
{
	for (uint32_t i = 0u; i < STM32_OTGHS_MAX_CH; i++) {
		if (d->ch[i].in_use) {
			k_spinlock_key_t k = hal_enter(d);
			(void)HAL_HCD_HC_Halt(&d->hhcd, (uint8_t)i);
			hal_exit(d, k);

			d->ch[i].in_use = false;
		}
	}
}

static int stm32_find_idle_chan_match_nonctrl(struct stm32_otghs_data *d, uint8_t dev_addr,
					      uint8_t ep, uint8_t ep_type, uint16_t mps)
{
	for (uint32_t i = 0u; i < STM32_OTGHS_MAX_CH; i++) {
		if (!d->ch[i].in_use) {
			continue;
		}
		if (d->hc_act[i].xfer != NULL) {
			continue;
		}
		if (d->ch[i].dev_addr != dev_addr) {
			continue;
		}
		if (d->ch[i].ep != ep) {
			continue;
		}
		if (d->ch[i].ep_type != ep_type) {
			continue;
		}
		if (d->ch[i].mps != mps) {
			continue;
		}
		return (int)d->ch[i].hc_num;
	}

	return -ENOENT;
}

/*
 * Do not mask NAK/NYET. Let the HAL own the HC_NAK/HC_NYET state machine,
 * including the optional IN-side URB_NAK_WAIT parking path.
 */
static inline void stm32_mask_nak_nyet(struct stm32_otghs_data *d, int hc, uint8_t ep_type)
{
	ARG_UNUSED(d);
	ARG_UNUSED(hc);
	ARG_UNUSED(ep_type);
}

static int stm32_alloc_or_reinit_chan(struct stm32_otghs_data *d, uint8_t dev_addr, uint8_t ep,
				      uint8_t ep_type, uint16_t mps)
{
	const bool dir_in = USB_EP_DIR_IS_IN(ep);
	const uint8_t ep_num = stm32_ep_index(ep);
	const bool ep0 = (ep_num == 0u);

	const uint8_t hal_ep = ep_num | (dir_in ? USB_REQ_DIR_IN_BIT : 0u);

	if (ep0 || (ep_type == EP_TYPE_CTRL)) {
		const int existing = stm32_find_chan(d, dev_addr, ep, ep_type, mps);

		if (existing >= 0) {
			if (ep0 && (d->ch[existing].mps != mps)) {
				d->ch[existing].mps = mps;

				k_spinlock_key_t k = hal_enter(d);
				(void)HAL_HCD_HC_Halt(&d->hhcd, (uint8_t)existing);
				hal_exit(d, k);

				stm32_hc_init(d, (uint8_t)existing, hal_ep, dev_addr, ep_type, mps);
			}
			return existing;
		}

		for (uint32_t i = 0u; i < STM32_OTGHS_MAX_CH; i++) {
			if (d->ch[i].in_use) {
				continue;
			}

			d->ch[i].in_use = true;
			d->ch[i].dev_addr = dev_addr;
			d->ch[i].ep = ep;
			d->ch[i].ep_num = ep_num;
			d->ch[i].ep_type = ep_type;
			d->ch[i].mps = mps;
			d->ch[i].dir_in = dir_in ? 1u : 0u;
			d->ch[i].hc_num = (uint8_t)i;

			stm32_hc_init(d, (uint8_t)i, hal_ep, dev_addr, ep_type, mps);

			return (int)i;
		}

		return -ENOMEM;
	}

	{
		const int idle = stm32_find_idle_chan_match_nonctrl(d, dev_addr, ep, ep_type, mps);

		if (idle >= 0) {
			return idle;
		}
	}

	for (uint32_t i = 0u; i < STM32_OTGHS_MAX_CH; i++) {
		if (d->ch[i].in_use) {
			continue;
		}

		d->ch[i].in_use = true;
		d->ch[i].dev_addr = dev_addr;
		d->ch[i].ep = ep;
		d->ch[i].ep_num = ep_num;
		d->ch[i].ep_type = ep_type;
		d->ch[i].mps = mps;
		d->ch[i].dir_in = dir_in ? 1u : 0u;
		d->ch[i].hc_num = (uint8_t)i;

		stm32_hc_init(d, (uint8_t)i, hal_ep, dev_addr, ep_type, mps);

		return (int)i;
	}

	return -ENOMEM;
}

/* Endpoint type discovery */

static int stm32_get_ep_type(const struct usb_device *udev, uint8_t ep_addr, uint8_t *out_ep_type)
{
	if (!udev || !udev->cfg_desc || !out_ep_type) {
		return -EINVAL;
	}

	const uint8_t *p = (const uint8_t *)udev->cfg_desc;

	if (p[1] != USB_DESC_CONFIGURATION) {
		return -EINVAL;
	}

	const struct usb_cfg_descriptor *cfg = (const struct usb_cfg_descriptor *)p;
	const uint16_t total = sys_le16_to_cpu(cfg->wTotalLength);
	const uint8_t *end = p + total;

	while ((p + STM32_OTGHS_DESC_ENTRY_MIN_LEN) <= end) {
		const uint8_t len = p[0];
		const uint8_t type = p[1];

		if ((len < STM32_OTGHS_DESC_ENTRY_MIN_LEN) || ((p + len) > end)) {
			break;
		}

		if (type == USB_DESC_ENDPOINT) {
			const struct usb_ep_descriptor *epd = (const struct usb_ep_descriptor *)p;

			if (epd->bEndpointAddress == ep_addr) {
				const uint8_t x = epd->bmAttributes & STM32_OTGHS_EP_ATTR_TYPE_MASK;

				if (x == USB_EP_TYPE_BULK) {
					*out_ep_type = EP_TYPE_BULK;
					return 0;
				}
				if (x == USB_EP_TYPE_INTERRUPT) {
					*out_ep_type = EP_TYPE_INTR;
					return 0;
				}
				return -ENOTSUP;
			}
		}

		p += len;
	}

	return -ENOENT;
}

/* Submit helpers */

static uint16_t stm32_ep0_mps_default(const struct stm32_otghs_data *d,
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

static bool usb_req_is_in(const struct uhc_transfer *xfer)
{
	return ((xfer->setup_pkt[0] & USB_REQ_DIR_IN_BIT) != 0u);
}

static uint16_t usb_setup_wlength(const struct uhc_transfer *xfer)
{
	return sys_get_le16(&xfer->setup_pkt[STM32_OTGHS_SETUP_WLENGTH_OFFSET]);
}

static bool stm32_ep_has_active_xfer(struct stm32_otghs_data *d, uint8_t dev_addr, uint8_t ep)
{
	for (uint32_t i = 0; i < STM32_OTGHS_MAX_CH; i++) {
		if (d->hc_act[i].xfer == NULL) {
			continue;
		}

		if (!d->hc_act[i].xfer->udev) {
			continue;
		}

		if (d->hc_act[i].xfer->udev->addr != dev_addr) {
			continue;
		}

		if (d->hc_act[i].ep != ep) {
			continue;
		}

		return true;
	}

	return false;
}

static uint16_t stm32_default_nonctrl_mps(uint8_t ep_type, const struct uhc_transfer *xfer)
{
	if (xfer->mps != 0u) {
		return xfer->mps;
	}

	if (ep_type == EP_TYPE_INTR) {
		return STM32_MPS_DEFAULT_INTR;
	}
	return STM32_MPS_DEFAULT_BULK;
}

static uint32_t stm32_out_submit_len(struct stm32_otghs_data *d, uint8_t ep_type, uint16_t mps,
				     uint32_t remaining)
{
	if (!stm32_dma_enabled(d) && (ep_type == EP_TYPE_BULK) && (mps > 0u)) {
		return MIN(remaining, (uint32_t)mps);
	}

	return remaining;
}

static int stm32_resubmit_control_same_hc(struct stm32_otghs_data *d)
{
	struct stm32_pipe *p = &d->ctrl;
	struct uhc_transfer *xfer = p->xfer;
	uint16_t transfer_len = p->len;
	uint32_t buf_limit;
	uint8_t *data_ptr;

	if (!xfer || !xfer->udev || (p->hc < 0)) {
		return -EINVAL;
	}

	if ((xfer->stage != UHC_CONTROL_STAGE_DATA) || !usb_req_is_in(xfer) ||
	    (xfer->buf == NULL)) {
		return stm32_submit_control(d, xfer);
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

	const int ret = stm32_hc_submit_request(d, (uint8_t)p->hc, true, EP_TYPE_CTRL,
						HCD_PID_DATA1, data_ptr, transfer_len);
	if (ret != 0) {
		return ret;
	}

	p->len = transfer_len;
	p->halted_cnt = 0u;
	p->start_ms = k_uptime_get_32();

	return 0;
}

static int stm32_submit_control(struct stm32_otghs_data *d, struct uhc_transfer *xfer)
{
	if (!d || !xfer || !xfer->udev) {
		return -EINVAL;
	}

	const uint8_t dev_addr = xfer->udev->addr;
	const uint16_t wLength = usb_setup_wlength(xfer);

	const bool req_in = usb_req_is_in(xfer);
	const bool has_data = (wLength != 0u);

	const uint16_t mps = stm32_ep0_mps_default(d, xfer);

	uint8_t ep = 0u;
	uint8_t dir = 0u;
	uint8_t pid = 0u;
	uint8_t *data_ptr = NULL;
	uint32_t transfer_len = 0u;

	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		ep = USB_CONTROL_EP_OUT;
		dir = 0u;
		pid = HCD_PID_SETUP;
		memcpy(d->ctrl.setup_words, xfer->setup_pkt, sizeof(xfer->setup_pkt));
		data_ptr = (uint8_t *)d->ctrl.setup_words;
		transfer_len = sizeof(xfer->setup_pkt);

	} else if ((xfer->stage == UHC_CONTROL_STAGE_DATA) && (xfer->buf != NULL) && has_data) {
		ep = req_in ? USB_CONTROL_EP_IN : USB_CONTROL_EP_OUT;
		dir = req_in ? 1u : 0u;
		pid = HCD_PID_DATA1;

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
		dir = status_in ? 1u : 0u;
		pid = HCD_PID_DATA1;
		data_ptr = (uint8_t *)&d->zlp_dummy;
		transfer_len = 0u;

	} else {
		return -EINVAL;
	}

	const int hc = stm32_alloc_or_reinit_chan(d, dev_addr, ep, EP_TYPE_CTRL, mps);

	if (hc < 0) {
		return hc;
	}

	const uint32_t start_ms = k_uptime_get_32();

	const int ret = stm32_hc_submit_request(d, (uint8_t)hc, dir != 0u, EP_TYPE_CTRL, pid,
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
	d->ctrl.start_ms = start_ms;

	return 0;
}

static int stm32_submit_nonctrl(struct stm32_otghs_data *d, struct uhc_transfer *xfer)
{
	if (!d || !xfer || !xfer->udev || !xfer->buf) {
		return -EINVAL;
	}

	const uint8_t dev_addr = xfer->udev->addr;
	const uint8_t ep = xfer->ep;
	const bool dir_in = USB_EP_DIR_IS_IN(ep);

	uint8_t ep_type = EP_TYPE_BULK;

	if (stm32_get_ep_type(xfer->udev, ep, &ep_type) != 0) {
		ep_type = EP_TYPE_BULK;
	}

	const uint16_t mps = stm32_default_nonctrl_mps(ep_type, xfer);

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
		transfer_len = stm32_out_submit_len(d, ep_type, mps, (uint32_t)xfer->buf->len);
	}

	if (transfer_len == 0u) {
		return -EINVAL;
	}

	/*
	 * CRITICAL FIX:
	 * Serialize per (device, endpoint). The toggle bookkeeping is per endpoint,
	 * not per outstanding transfer, so multiple in-flight transfers on the same
	 * endpoint are unsafe.
	 */
	if (stm32_ep_has_active_xfer(d, dev_addr, ep)) {
		return -EBUSY;
	}

	const int hc = stm32_alloc_or_reinit_chan(d, dev_addr, ep, ep_type, mps);

	if (hc < 0) {
		return hc;
	}

	if (d->hc_act[hc].xfer != NULL) {
		return -EBUSY;
	}

	uint8_t pid = HCD_PID_DATA0;

	if (ep_type == EP_TYPE_BULK) {
		pid = stm32_bulk_pid_from_hc(d, hc, dir_in);
	}

	const uint32_t start_ms = k_uptime_get_32();

	const int ret = stm32_hc_submit_request(d, (uint8_t)hc, dir_in, ep_type, pid, data_ptr,
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
	d->hc_act[hc].start_ms = start_ms;

	return 0;
}

static int stm32_resubmit_nonctrl_same_hc(struct stm32_otghs_data *d, int hc)
{
	struct stm32_hc_active *a = &d->hc_act[hc];
	struct uhc_transfer *xfer = a->xfer;
	uint16_t req_len = a->req_len;
	uint32_t buf_limit;

	if (!xfer || !xfer->udev || !xfer->buf) {
		return -EINVAL;
	}

	uint8_t pid = HCD_PID_DATA0;

	if (a->ep_type == EP_TYPE_BULK) {
		pid = stm32_bulk_pid_from_hc(d, hc, a->dir_in != 0u);
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

	const int ret = stm32_hc_submit_request(d, (uint8_t)hc, a->dir_in != 0u, a->ep_type, pid,
						data_ptr, req_len);
	if (ret != 0) {
		return ret;
	}

	a->req_len = req_len;
	a->halted_cnt = 0u;
	a->start_ms = k_uptime_get_32();
	return 0;
}

/* Completion */

static bool stm32_is_std_get_device_desc(const struct uhc_transfer *xfer)
{
	const uint8_t bm = xfer->setup_pkt[0];
	const uint8_t bRequest = xfer->setup_pkt[1];
	const uint16_t wValue = sys_get_le16(&xfer->setup_pkt[2]);
	const uint8_t desc_type = (uint8_t)(wValue >> 8);

	const bool std_type = ((bm & USB_BMREQTYPE_TYPE_MASK) == USB_BMREQTYPE_TYPE_STANDARD);
	const bool get_desc = (bRequest == USB_REQ_GET_DESCRIPTOR);
	const bool device_desc = (desc_type == USB_DESC_TYPE_DEVICE);

	return std_type && get_desc && device_desc;
}

static bool stm32_ep0_mps_is_valid(uint8_t mps0)
{
	return (mps0 == USB_EP0_MPS_8) || (mps0 == USB_EP0_MPS_16) || (mps0 == USB_EP0_MPS_32) ||
	       (mps0 == USB_EP0_MPS_64);
}

static void stm32_ep0_force_data1_after_setup(struct stm32_otghs_data *d, uint8_t dev_addr)
{
	const int hc_out =
		stm32_find_chan(d, dev_addr, USB_CONTROL_EP_OUT, EP_TYPE_CTRL, d->ep0_mps);
	const int hc_in = stm32_find_chan(d, dev_addr, USB_CONTROL_EP_IN, EP_TYPE_CTRL, d->ep0_mps);

	k_spinlock_key_t k = hal_enter(d);

	if (hc_out >= 0) {
		d->hhcd.hc[hc_out].toggle_in = 1u;
		d->hhcd.hc[hc_out].toggle_out = 1u;
	}
	if (hc_in >= 0) {
		d->hhcd.hc[hc_in].toggle_in = 1u;
		d->hhcd.hc[hc_in].toggle_out = 1u;
	}

	hal_exit(d, k);
}

static uint8_t stm32_ctrl_stage_ep(const struct uhc_transfer *xfer, bool req_in)
{
	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		return USB_CONTROL_EP_OUT;
	}

	if (xfer->stage == UHC_CONTROL_STAGE_DATA) {
		return req_in ? USB_CONTROL_EP_IN : USB_CONTROL_EP_OUT;
	}

	return req_in ? USB_CONTROL_EP_OUT : USB_CONTROL_EP_IN;
}

static bool stm32_ctrl_handle_wait_or_error(const struct device *dev, struct stm32_otghs_data *d,
					    struct stm32_pipe *p, struct uhc_transfer *xfer,
					    uint8_t hc, HCD_URBStateTypeDef urb_state, bool req_in,
					    uint32_t hal_cnt)
{
	if (stm32_ctrl_needs_sw_retry(d, hc, urb_state, false)) {
		const int r = stm32_submit_control(d, xfer);

		if (r != 0) {
			stm32_pipe_finish(dev, p, r);
		}
		return true;
	}

	if (stm32_urb_waiting_on_device(urb_state)) {
		return true;
	}

	if (urb_state == URB_NAK_WAIT) {
		if ((xfer->stage == UHC_CONTROL_STAGE_DATA) && req_in) {
			const int pr = stm32_control_commit_halted_in(p, hal_cnt);

			if (pr != 0) {
				stm32_pipe_finish(dev, p, pr);
				return true;
			}
		}

		if ((k_uptime_get_32() - p->start_ms) < STM32_OTGHS_NAK_WAIT_RETRY_MS) {
			return true;
		}

		const int r = stm32_resubmit_control_same_hc(d);

		if (r != 0) {
			stm32_pipe_finish(dev, p, r);
		}
		return true;
	}

	if (urb_state == URB_STALL) {
		stm32_pipe_finish(dev, p, -EPIPE);
		return true;
	}

	if (urb_state != URB_DONE) {
		stm32_pipe_finish(dev, p, -EIO);
		return true;
	}

	return false;
}

static void stm32_ctrl_complete_setup(const struct device *dev, struct stm32_otghs_data *d,
				      struct stm32_pipe *p, struct uhc_transfer *xfer,
				      uint8_t dev_addr, bool has_data)
{
	stm32_ep0_force_data1_after_setup(d, dev_addr);

	if (has_data) {
		if (xfer->buf == NULL) {
			stm32_pipe_finish(dev, p, -EINVAL);
			return;
		}
		xfer->stage = UHC_CONTROL_STAGE_DATA;
	} else {
		xfer->stage = UHC_CONTROL_STAGE_STATUS;
	}

	const int r = stm32_submit_control(d, xfer);

	if (r != 0) {
		stm32_pipe_finish(dev, p, r);
	}
}

static void stm32_ctrl_maybe_update_ep0_mps(struct stm32_otghs_data *d, struct uhc_transfer *xfer)
{
	if ((xfer->buf->len < USB_EP0_MPS_8) || !stm32_is_std_get_device_desc(xfer)) {
		return;
	}

	const uint8_t mps0 = xfer->buf->data[STM32_OTGHS_DEVICE_DESC_MPS0_OFFSET];

	if (stm32_ep0_mps_is_valid(mps0)) {
		d->ep0_mps = mps0;
	}
}

static void stm32_ctrl_complete_data(const struct device *dev, struct stm32_otghs_data *d,
				     struct stm32_pipe *p, struct uhc_transfer *xfer, bool req_in,
				     uint32_t hal_cnt)
{
	if (req_in && (xfer->buf != NULL) && (hal_cnt > 0u)) {
		const uint32_t add = MIN(hal_cnt, (uint32_t)net_buf_tailroom(xfer->buf));

		(void)net_buf_add(xfer->buf, add);
		stm32_ctrl_maybe_update_ep0_mps(d, xfer);
	}

	xfer->stage = UHC_CONTROL_STAGE_STATUS;

	const int r = stm32_submit_control(d, xfer);

	if (r != 0) {
		stm32_pipe_finish(dev, p, r);
	}
}

static void stm32_complete_control_pipe(const struct device *dev, struct stm32_otghs_data *d,
					struct stm32_pipe *p)
{
	struct uhc_transfer *xfer = p->xfer;

	if (!xfer || !xfer->udev) {
		return;
	}

	const uint8_t dev_addr = xfer->udev->addr;
	const uint16_t wLength = usb_setup_wlength(xfer);
	const bool req_in = usb_req_is_in(xfer);
	const bool has_data = (wLength != 0u);
	const uint16_t mps = stm32_ep0_mps_default(d, xfer);
	const uint8_t stage_ep = stm32_ctrl_stage_ep(xfer, req_in);

	const int hc = stm32_find_chan(d, dev_addr, stage_ep, EP_TYPE_CTRL, mps);

	if (hc < 0) {
		stm32_pipe_finish(dev, p, -EIO);
		return;
	}

	const HCD_URBStateTypeDef urb_state = stm32_get_urb_state(d, (uint8_t)hc);
	const uint32_t hal_cnt =
		stm32_hc_done_bytes(d, (uint8_t)hc, p->len, USB_EP_DIR_IS_IN(stage_ep));

	if (stm32_ctrl_handle_wait_or_error(dev, d, p, xfer, (uint8_t)hc, urb_state, req_in,
					    hal_cnt)) {
		return;
	}

	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		stm32_ctrl_complete_setup(dev, d, p, xfer, dev_addr, has_data);
		return;
	}

	if (xfer->stage == UHC_CONTROL_STAGE_DATA) {
		stm32_ctrl_complete_data(dev, d, p, xfer, req_in, hal_cnt);
		return;
	}

	stm32_pipe_finish(dev, p, 0);
}

static void stm32_cancel_all_nonctrl(const struct device *dev, struct stm32_otghs_data *d, int st)
{
	for (uint32_t i = 0; i < STM32_OTGHS_MAX_CH; i++) {
		if (d->hc_act[i].xfer) {
			k_spinlock_key_t k = hal_enter(d);
			(void)HAL_HCD_HC_Halt(&d->hhcd, (uint8_t)i);
			hal_exit(d, k);

			uhc_xfer_return(dev, d->hc_act[i].xfer, fix_status(st));
			hc_act_clear(&d->hc_act[i]);
		}
	}
}

static void stm32_nonctrl_apply_out_toggle_fix(struct stm32_otghs_data *d, int hc,
					       const struct stm32_hc_active *a)
{
	if (stm32_dma_enabled(d) || a->dir_in || (a->mps == 0u)) {
		return;
	}

	const uint32_t np = ((uint32_t)a->req_len == 0u)
				    ? 1u
				    : stm32_packets_from_bytes((uint32_t)a->req_len, a->mps);

	if ((np & 1u) == 0u) {
		k_spinlock_key_t k = hal_enter(d);

		d->hhcd.hc[hc].toggle_out ^= 1u;
		hal_exit(d, k);
	}
}

static bool stm32_nonctrl_continue_out(const struct device *dev, struct stm32_otghs_data *d,
				       struct stm32_hc_active *a, int hc)
{
	struct uhc_transfer *xfer = a->xfer;
	const uint32_t total = (uint32_t)xfer->buf->len;
	const uint32_t next_off = (uint32_t)a->buf_off + (uint32_t)a->req_len;

	if (next_off >= total) {
		return false;
	}

	const uint32_t remaining = total - next_off;
	const uint32_t next_len = stm32_out_submit_len(d, a->ep_type, a->mps, remaining);
	int r;

	a->buf_off = (uint16_t)next_off;
	a->req_len = (uint16_t)next_len;

	r = stm32_resubmit_nonctrl_same_hc(d, hc);
	if (r != 0) {
		stm32_active_xfer_finish(dev, a, r);
	}

	return true;
}

static void stm32_nonctrl_complete_done(const struct device *dev, struct stm32_otghs_data *d,
					struct stm32_hc_active *a, int hc, uint32_t hal_cnt)
{
	struct uhc_transfer *xfer = a->xfer;

	if (a->dir_in && xfer->buf && (hal_cnt > 0u)) {
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
	stm32_nonctrl_apply_out_toggle_fix(d, hc, a);

	if (!a->dir_in && xfer->buf && stm32_nonctrl_continue_out(dev, d, a, hc)) {
		return;
	}

	stm32_active_xfer_finish(dev, a, 0);
}

static bool stm32_nonctrl_handle_wait_or_error(const struct device *dev, struct stm32_otghs_data *d,
					       struct stm32_hc_active *a, int hc,
					       HCD_URBStateTypeDef urb_state, uint32_t hal_cnt)
{
	if (stm32_nonctrl_needs_sw_retry(d, (uint8_t)hc, urb_state, false)) {
		const int r = stm32_resubmit_nonctrl_same_hc(d, hc);

		if (r != 0) {
			stm32_active_xfer_finish(dev, a, r);
		}
		return true;
	}

	if (stm32_urb_waiting_on_device(urb_state)) {
		return true;
	}

	if (urb_state == URB_NAK_WAIT) {
		const int pr = stm32_nonctrl_commit_halted_in(a, hal_cnt);

		if (pr != 0) {
			stm32_active_xfer_finish(dev, a, pr);
			return true;
		}

		if ((k_uptime_get_32() - a->start_ms) < STM32_OTGHS_NAK_WAIT_RETRY_MS) {
			return true;
		}

		const int r = stm32_resubmit_nonctrl_same_hc(d, hc);

		if (r != 0) {
			stm32_active_xfer_finish(dev, a, r);
		}
		return true;
	}

	if (urb_state == URB_STALL) {
		stm32_active_xfer_finish(dev, a, -EPIPE);
		return true;
	}

	if (urb_state != URB_DONE) {
		stm32_active_xfer_finish(dev, a, -EIO);
		return true;
	}

	return false;
}

static void stm32_complete_one_hc(const struct device *dev, struct stm32_otghs_data *d, int hc)
{
	struct stm32_hc_active *a = &d->hc_act[hc];
	struct uhc_transfer *xfer = a->xfer;

	if (!xfer || !xfer->udev) {
		return;
	}

	const HCD_URBStateTypeDef urb_state = stm32_get_urb_state(d, (uint8_t)hc);
	const uint32_t hal_cnt = stm32_nonctrl_done_bytes(d, hc, a);

	if (stm32_nonctrl_handle_wait_or_error(dev, d, a, hc, urb_state, hal_cnt)) {
		return;
	}

	stm32_nonctrl_complete_done(dev, d, a, hc, hal_cnt);
}

/* Scheduling */

static void stm32_schedule_nonctrl_fill(const struct device *dev, struct stm32_otghs_data *d)
{
	struct uhc_data *ud = dev->data;

	if (!ud || sys_dlist_is_empty(&ud->bulk_xfers)) {
		return;
	}

	while (stm32_count_active_nonctrl(d) < STM32_OTGHS_MAX_INFLIGHT_BULK) {
		sys_dnode_t *node;
		bool saw_candidate = false;
		bool made_progress = false;

		for (node = sys_dlist_peek_head(&ud->bulk_xfers); node != NULL;
		     node = sys_dlist_peek_next(&ud->bulk_xfers, node)) {
			struct uhc_transfer *x = stm32_xfer_from_node(node);

			if (is_ep0(x) || stm32_xfer_is_active(d, x)) {
				continue;
			}

			saw_candidate = true;

			const int r = stm32_submit_nonctrl(d, x);

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

static int stm32_schedule_xfers(const struct device *dev, struct stm32_otghs_data *d)
{
	if (!d->connected || !d->bus_ready) {
		return 0;
	}

	if (d->ctrl.xfer == NULL) {
		struct uhc_transfer *x = peek_next_ctrl_ep0(dev);

		if (x) {
			const int r = stm32_submit_control(d, x);

			if (r == -EBUSY) {
				return 0;
			}
			if (r != 0) {
				uhc_xfer_return(dev, x, r);
			}
		}
	}

	stm32_schedule_nonctrl_fill(dev, d);
	return 0;
}

/* Timer callback */

static bool stm32_any_nak_wait(struct stm32_otghs_data *d)
{
	if (d->ctrl.xfer && d->ctrl.hc >= 0) {
		if (stm32_get_urb_state(d, (uint8_t)d->ctrl.hc) == URB_NAK_WAIT) {
			return true;
		}
	}

	for (uint32_t i = 0; i < STM32_OTGHS_MAX_CH; i++) {
		if (d->hc_act[i].xfer == NULL) {
			continue;
		}
		if (stm32_get_urb_state(d, (uint8_t)i) == URB_NAK_WAIT) {
			return true;
		}
	}

	return false;
}

static void stm32_periodic_wake_stop(struct stm32_otghs_data *d)
{
	atomic_set(&d->sof_tick_active, 0);
	k_timer_stop(&d->kick_timer);
}

static void stm32_periodic_wake_update(struct stm32_otghs_data *d)
{
	const bool need_periodic = d->connected && d->bus_ready && stm32_any_nak_wait(d);

	if (stm32_sof_irq_enabled(d)) {
		atomic_set(&d->sof_tick_active, need_periodic ? 1 : 0);
		k_timer_stop(&d->kick_timer);
		return;
	}

	atomic_set(&d->sof_tick_active, 0);
	if (need_periodic) {
		k_timer_start(&d->kick_timer, K_MSEC(STM32_OTGHS_KICK_START_DELAY_MS),
			      K_MSEC(STM32_OTGHS_KICK_PERIOD_MS));
	} else {
		k_timer_stop(&d->kick_timer);
	}
}

static void kick_timer_cb(struct k_timer *t)
{
	struct stm32_otghs_data *d = (struct stm32_otghs_data *)k_timer_user_data_get(t);

	if (!d->connected || !d->bus_ready) {
		return;
	}

	if (stm32_any_nak_wait(d)) {
		irq_kick(d);
	}
}

/* HAL callbacks */

static inline struct stm32_otghs_data *stm32_data_from_hhcd(HCD_HandleTypeDef *hhcd)
{
	return (struct stm32_otghs_data *)hhcd->pData;
}

static inline void stm32_set_event_and_kick(HCD_HandleTypeDef *hhcd, atomic_val_t bit)
{
	struct stm32_otghs_data *d = stm32_data_from_hhcd(hhcd);

	if (d == NULL) {
		return;
	}

	atomic_or(&d->evt_flags, bit);
	irq_kick(d);
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd)
{
	stm32_set_event_and_kick(hhcd, STM32_EVT_PEN);
}

void HAL_HCD_PortDisabled_Callback(HCD_HandleTypeDef *hhcd)
{
	stm32_set_event_and_kick(hhcd, STM32_EVT_PDIS);
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
	stm32_set_event_and_kick(hhcd, STM32_EVT_CONN);
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
	stm32_set_event_and_kick(hhcd, STM32_EVT_DISCONN);
}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{
	struct stm32_otghs_data *d = stm32_data_from_hhcd(hhcd);

	if (d == NULL) {
		return;
	}

	if (atomic_get(&d->sof_tick_active) != 0) {
		irq_kick(d);
	}
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum,
					 HCD_URBStateTypeDef urb_state)
{
	struct stm32_otghs_data *d = stm32_data_from_hhcd(hhcd);

	if (d == NULL) {
		return;
	}

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
		if (stm32_nonctrl_needs_sw_retry(d, chnum, urb_state, true) ||
		    stm32_ctrl_needs_sw_retry(d, chnum, urb_state, true)) {
			atomic_or(&d->evt_flags, STM32_EVT_URB);
			irq_kick(d);
		}
		break;

	case URB_NYET:
		break;

	default:
		atomic_or(&d->evt_flags, STM32_EVT_URB);
		irq_kick(d);
		break;
	}
}

/* ISR + thread */

static void stm32_otghs_isr(const struct device *dev)
{
	struct stm32_otghs_data *d = dev_data(dev);

	if (!d || !d->hhcd.Instance) {
		return;
	}

	k_spinlock_key_t k = hal_enter(d);

	HAL_HCD_IRQHandler(&d->hhcd);
	hal_exit(d, k);
}

static void stm32_update_speed_from_hal_locked(struct stm32_otghs_data *d)
{
	k_spinlock_key_t k = hal_enter(d);

	stm32_update_dev_speed_from_hal(d);
	hal_exit(d, k);
}

static bool stm32_thread_handle_disconnect(const struct device *dev, struct stm32_otghs_data *d,
					   atomic_val_t flags, bool *was_present)
{
	if ((flags & (STM32_EVT_DISCONN | STM32_EVT_PDIS)) == 0) {
		return false;
	}

	stm32_periodic_wake_stop(d);

	d->connected = false;
	d->bus_ready = false;
	d->reset_pending = false;
	d->ep0_mps = 0u;

	stm32_free_all_channels(d);
	pipe_cancel_and_return(dev, d, &d->ctrl, -ENODEV);
	stm32_cancel_all_nonctrl(dev, d, -ENODEV);
	stm32_toggle_reset_all(d);

	if (*was_present) {
		uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
	}

	*was_present = false;
	return true;
}

static bool stm32_thread_handle_connect(const struct device *dev, struct stm32_otghs_data *d,
					atomic_val_t flags, bool *was_present)
{
	if ((flags & STM32_EVT_CONN) == 0) {
		return false;
	}

	stm32_dump_hprt(d, "EVT:CONNECT");

	d->connected = true;
	d->bus_ready = false;
	d->reset_pending = false;
	d->ep0_mps = USB_EP0_MPS_8;
	d->zlp_dummy = 0u;

	stm32_periodic_wake_stop(d);
	stm32_free_all_channels(d);
	pipe_clear(&d->ctrl);
	hc_act_clear_all(d);
	stm32_toggle_reset_all(d);
	stm32_update_speed_from_hal_locked(d);

#ifdef UHC_EVT_DEV_CONNECTED_HS
	uhc_submit_event(dev,
			 (d->speed == HCD_SPEED_HIGH) ? UHC_EVT_DEV_CONNECTED_HS
						      : UHC_EVT_DEV_CONNECTED_FS,
			 0);
#else
	uhc_submit_event(dev, UHC_EVT_DEV_CONNECTED_FS, 0);
#endif

	*was_present = true;
	return true;
}

static bool stm32_thread_handle_port_enabled(const struct device *dev, struct stm32_otghs_data *d,
					     atomic_val_t flags)
{
	if ((flags & STM32_EVT_PEN) == 0) {
		return false;
	}

	stm32_dump_hprt(d, "EVT:PEN");
	stm32_update_speed_from_hal_locked(d);

	if (!d->reset_pending) {
		return false;
	}

	d->reset_pending = false;
	k_msleep(STM32_OTGHS_POST_RESET_DELAY_MS);

	d->bus_ready = true;
	stm32_periodic_wake_update(d);

	uhc_submit_event(dev, UHC_EVT_RESETED, 0);
	return true;
}

static bool stm32_thread_handle_reset_fallback(const struct device *dev, struct stm32_otghs_data *d)
{
	if (!d->reset_pending) {
		return false;
	}

	if ((k_uptime_get_32() - d->reset_start_ms) <= STM32_OTGHS_RESET_FALLBACK_MS) {
		return false;
	}

	if ((stm32_read_hprt(d) & USB_OTG_HPRT_PENA) == 0u) {
		return false;
	}

	d->reset_pending = false;
	d->bus_ready = true;
	stm32_periodic_wake_update(d);

	uhc_submit_event(dev, UHC_EVT_RESETED, 0);
	return true;
}

static void stm32_complete_all_nonctrl(const struct device *dev, struct stm32_otghs_data *d)
{
	for (uint32_t hc = 0; hc < STM32_OTGHS_MAX_CH; hc++) {
		if (d->hc_act[hc].xfer != NULL) {
			stm32_complete_one_hc(dev, d, (int)hc);
		}
	}
}

static void stm32_otghs_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = (const struct device *)p1;
	struct stm32_otghs_data *d = dev_data(dev);
	bool was_present = false;

	for (;;) {
		(void)k_sem_take(&d->irq_sem, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);
		const atomic_val_t flags = atomic_set(&d->evt_flags, 0);

		if (stm32_thread_handle_disconnect(dev, d, flags, &was_present)) {
			uhc_unlock_internal(dev);
			continue;
		}

		if (stm32_thread_handle_connect(dev, d, flags, &was_present)) {
			uhc_unlock_internal(dev);
			k_yield();
			continue;
		}

		if (stm32_thread_handle_port_enabled(dev, d, flags) ||
		    stm32_thread_handle_reset_fallback(dev, d)) {
			uhc_unlock_internal(dev);
			k_yield();
			continue;
		}

		if (!d->connected || !d->bus_ready) {
			uhc_unlock_internal(dev);
			continue;
		}

		if (d->ctrl.xfer) {
			stm32_complete_control_pipe(dev, d, &d->ctrl);
		}

		stm32_complete_all_nonctrl(dev, d);

		(void)stm32_schedule_xfers(dev, d);
		stm32_periodic_wake_update(d);

		uhc_unlock_internal(dev);
		k_yield();
	}
}

/* UHC API */

static int stm32_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int stm32_unlock(const struct device *dev)
{
	return uhc_unlock_internal(dev);
}

static void stm32_host_cfg_init(struct stm32_otghs_data *d)
{
	USB_OTG_GlobalTypeDef *g = (USB_OTG_GlobalTypeDef *)d->hhcd.Instance;
	USB_OTG_HostTypeDef *h =
		(USB_OTG_HostTypeDef *)((uintptr_t)d->hhcd.Instance + (uintptr_t)USB_OTG_HOST_BASE);

#ifdef USB_OTG_HCFG_FSLSPCS
	h->HCFG &= ~USB_OTG_HCFG_FSLSPCS;
#endif

	g->GINTSTS = ~0u;
	LOG_DBG("[HCFG] HCFG=0x%08x", (unsigned int)h->HCFG);
}

static const char *stm32_periodic_wake_source_name(const struct stm32_otghs_data *d) __maybe_unused;

static const char *stm32_periodic_wake_source_name(const struct stm32_otghs_data *d)
{
	return stm32_sof_irq_enabled(d) ? "sof" : "timer";
}

static void stm32_log_feature_config(const struct stm32_otghs_data *d)
{
	LOG_INF("[CFG] dma=%s sof=%s wake=%s force_vbus_valid=%s "
		"vbus_sensing=%s in_nak_park=%s",
		stm32_dma_enabled(d) ? "on" : "off", stm32_sof_irq_enabled(d) ? "on" : "off",
		stm32_periodic_wake_source_name(d),
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

static void stm32_set_sof_interrupt_enabled(struct stm32_otghs_data *d, bool enable)
{
	k_spinlock_key_t k = hal_enter(d);

	if (enable) {
		d->hhcd.Instance->GINTMSK |= USB_OTG_GINTMSK_SOFM;
		d->hhcd.Init.Sof_enable = ENABLE;
	} else {
		d->hhcd.Instance->GINTMSK &= ~USB_OTG_GINTMSK_SOFM;
		d->hhcd.Init.Sof_enable = DISABLE;
	}

	hal_exit(d, k);

	atomic_set(&d->sof_irq_enabled, enable ? 1 : 0);
}

static int stm32_init(const struct device *dev)
{
	struct stm32_otghs_data *d = dev_data(dev);
	const struct stm32_otghs_config *cfg = dev->config;

	d->connected = false;
	d->bus_ready = false;
	d->reset_pending = false;
	atomic_set(&d->sof_irq_enabled, 0);
	atomic_set(&d->sof_tick_active, 0);

	pipe_clear(&d->ctrl);
	hc_act_clear_all(d);
	stm32_toggle_reset_all(d);

	stm32_free_all_channels(d);

	d->ep0_mps = USB_EP0_MPS_8;
	d->zlp_dummy = 0u;

	stm32_otghs_msp_like_init();

	memset(&d->hhcd, 0, sizeof(d->hhcd));
	d->hhcd.Instance = (USB_OTG_GlobalTypeDef *)cfg->base;

	d->hhcd.Init.Host_channels = STM32_OTGHS_MAX_CH;
	d->hhcd.Init.speed = HCD_SPEED_HIGH;
	d->speed = HCD_SPEED_HIGH;

	d->hhcd.Init.dma_enable = IS_ENABLED(CONFIG_UHC_STM32_OTGHS_DMA) ? ENABLE : DISABLE;
	d->hhcd.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
	d->hhcd.Init.Sof_enable = IS_ENABLED(CONFIG_UHC_STM32_OTGHS_SOF) ? ENABLE : DISABLE;
	d->hhcd.Init.low_power_enable = DISABLE;
	d->hhcd.Init.use_external_vbus =
		IS_ENABLED(CONFIG_UHC_STM32_OTGHS_FORCE_VBUS_VALID) ? DISABLE : ENABLE;
	d->hhcd.Init.vbus_sensing_enable =
		IS_ENABLED(CONFIG_UHC_STM32_OTGHS_FORCE_VBUS_VALID) ? DISABLE : ENABLE;

	d->hhcd.pData = d;

	k_spinlock_key_t k = hal_enter(d);
	HAL_StatusTypeDef st = HAL_HCD_Init(&d->hhcd);

	hal_exit(d, k);
	if (st != HAL_OK) {
		LOG_ERR("HAL_HCD_Init failed");
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_UHC_STM32_OTGHS_FORCE_VBUS_VALID)) {
		stm32_disable_vbus_sensing(d);
		stm32_force_session_valid(d);
	} else {
		stm32_enable_vbus_sensing(d);
	}

	k = hal_enter(d);
	st = HAL_HCD_Start(&d->hhcd);
	hal_exit(d, k);
	if (st != HAL_OK) {
		LOG_ERR("HAL_HCD_Start failed");
		return -EIO;
	}

	stm32_host_cfg_init(d);
	stm32_set_sof_interrupt_enabled(d, IS_ENABLED(CONFIG_UHC_STM32_OTGHS_SOF));
	stm32_port_power_on(d);
	stm32_log_feature_config(d);

	k_msleep(STM32_OTGHS_POST_START_DELAY_MS);
	stm32_dump_hprt(d, "INIT");

	return 0;
}

static int stm32_enable(const struct device *dev)
{
	irq_kick(dev_data(dev));
	return 0;
}

static int stm32_disable(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int stm32_shutdown(const struct device *dev)
{
	struct stm32_otghs_data *d = dev_data(dev);

	stm32_periodic_wake_stop(d);
	k_spinlock_key_t k = hal_enter(d);
	(void)HAL_HCD_Stop(&d->hhcd);
	(void)HAL_HCD_DeInit(&d->hhcd);
	hal_exit(d, k);

	return 0;
}

static int stm32_bus_reset(const struct device *dev)
{
	struct stm32_otghs_data *d = dev_data(dev);

	d->bus_ready = false;
	d->reset_pending = true;
	d->reset_start_ms = k_uptime_get_32();
	stm32_periodic_wake_stop(d);

	pipe_cancel_and_return(dev, d, &d->ctrl, -EAGAIN);
	stm32_cancel_all_nonctrl(dev, d, -EAGAIN);
	stm32_free_all_channels(d);

	k_spinlock_key_t k = hal_enter(d);
	const HAL_StatusTypeDef st = HAL_HCD_ResetPort(&d->hhcd);

	hal_exit(d, k);

	if (st != HAL_OK) {
		d->reset_pending = false;
		LOG_ERR("HAL_HCD_ResetPort failed");
		return -EIO;
	}

	return 0;
}

static int stm32_sof_enable(const struct device *dev)
{
	struct stm32_otghs_data *d = dev_data(dev);

	if (stm32_sof_irq_enabled(d)) {
		return -EALREADY;
	}

	stm32_set_sof_interrupt_enabled(d, true);
	stm32_periodic_wake_update(d);
	LOG_INF("[CFG] SOF interrupt delivery enabled at runtime");
	LOG_INF("[CFG] dma=%s sof=%s wake=%s force_vbus_valid=%s",
		stm32_dma_enabled(d) ? "on" : "off", stm32_sof_irq_enabled(d) ? "on" : "off",
		stm32_periodic_wake_source_name(d),
		IS_ENABLED(CONFIG_UHC_STM32_OTGHS_FORCE_VBUS_VALID) ? "on" : "off");

	return 0;
}

static int stm32_bus_suspend(const struct device *dev)
{
	stm32_periodic_wake_stop(dev_data(dev));
	uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);
	return 0;
}

static int stm32_bus_resume(const struct device *dev)
{
	stm32_periodic_wake_update(dev_data(dev));
	uhc_submit_event(dev, UHC_EVT_RESUMED, 0);
	return 0;
}

static int stm32_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_data *data = dev->data;
	struct stm32_otghs_data *d = dev_data(dev);

	if (is_ep0(xfer)) {
		sys_dlist_append(&data->ctrl_xfers, &xfer->node);
	} else {
		sys_dlist_append(&data->bulk_xfers, &xfer->node);
	}

	irq_kick(d);
	return 0;
}

static int stm32_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_data *data = dev->data;
	struct stm32_otghs_data *d = dev_data(dev);

	if (!xfer) {
		return -EINVAL;
	}

	if (stm32_list_contains_xfer(&data->ctrl_xfers, xfer) ||
	    stm32_list_contains_xfer(&data->bulk_xfers, xfer)) {
		return_fixed(dev, xfer, -ECONNRESET);
		irq_kick(d);
		return 0;
	}

	if (d->ctrl.xfer == xfer) {
		pipe_cancel_and_return(dev, d, &d->ctrl, -ECONNRESET);
		irq_kick(d);
		return 0;
	}

	const int hc = stm32_find_active_hc_for_xfer(d, xfer);

	if (hc >= 0) {
		k_spinlock_key_t k = hal_enter(d);
		(void)HAL_HCD_HC_Halt(&d->hhcd, (uint8_t)hc);
		hal_exit(d, k);

		return_fixed(dev, xfer, -ECONNRESET);
		hc_act_clear(&d->hc_act[hc]);

		irq_kick(d);
		return 0;
	}

	return 0;
}

static const struct uhc_api stm32_uhc_api = {
	.lock = stm32_lock,
	.unlock = stm32_unlock,
	.init = stm32_init,
	.enable = stm32_enable,
	.disable = stm32_disable,
	.shutdown = stm32_shutdown,

	.bus_reset = stm32_bus_reset,
	.sof_enable = stm32_sof_enable,
	.bus_suspend = stm32_bus_suspend,
	.bus_resume = stm32_bus_resume,

	.ep_enqueue = stm32_ep_enqueue,
	.ep_dequeue = stm32_ep_dequeue,
};

/* Device init / registration */

static int stm32_driver_preinit(const struct device *dev)
{
	struct uhc_data *data = dev->data;
	struct stm32_otghs_data *d = data->priv;
	const struct stm32_otghs_config *cfg = dev->config;

	k_mutex_init(&data->mutex);

	k_sem_init(&d->irq_sem, 0, 1);
	atomic_set(&d->evt_flags, 0);

	k_timer_init(&d->kick_timer, kick_timer_cb, NULL);
	k_timer_user_data_set(&d->kick_timer, d);

	if (cfg->pinctrl) {
		const int ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);

		if (ret != 0) {
			LOG_ERR("pinctrl apply failed: %d", ret);
			return ret;
		}
	}

	return 0;
}

#define STM32_OTGHS_INST_STACK_SIZE(n)                                                             \
	DT_INST_PROP_OR(n, stack_size, STM32_OTGHS_STACK_SIZE_DEFAULT)

#define STM32_OTGHS_INST_THREAD_PRIO(n)                                                            \
	DT_INST_PROP_OR(n, thread_prio, STM32_OTGHS_THREAD_PRIO_DEFAULT)

#define STM32_OTGHS_INST_DEFINE(n)                                                                 \
	static void stm32_otghs_irq_config_##n(const struct device *dev)                           \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), stm32_otghs_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static void stm32_otghs_thread_entry_##n(void *p1, void *p2, void *p3)                     \
	{                                                                                          \
		stm32_otghs_thread(p1, p2, p3);                                                    \
	}                                                                                          \
                                                                                                   \
	static K_KERNEL_STACK_DEFINE(stm32_otghs_stack_##n, STM32_OTGHS_INST_STACK_SIZE(n));       \
	static struct k_thread stm32_otghs_thread_##n;                                             \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct stm32_otghs_data stm32_otghs_priv_##n = {                                    \
		.irq_sem = Z_SEM_INITIALIZER(stm32_otghs_priv_##n.irq_sem, 0, 1),                  \
		.speed = HCD_SPEED_HIGH,                                                           \
		.connected = false,                                                                \
		.bus_ready = false,                                                                \
		.reset_pending = false,                                                            \
		.reset_start_ms = 0u,                                                              \
		.ep0_mps = USB_EP0_MPS_8,                                                          \
		.zlp_dummy = 0u,                                                                   \
	};                                                                                         \
                                                                                                   \
	static struct uhc_data stm32_uhc_data_##n = {                                              \
		.priv = &stm32_otghs_priv_##n,                                                     \
	};                                                                                         \
                                                                                                   \
	static const struct stm32_otghs_config stm32_otghs_cfg_##n = {                             \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irqn = DT_INST_IRQN(n),                                                           \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.stack = stm32_otghs_stack_##n,                                                    \
		.stack_size = K_KERNEL_STACK_SIZEOF(stm32_otghs_stack_##n),                        \
		.thread_prio = K_PRIO_PREEMPT(STM32_OTGHS_INST_THREAD_PRIO(n)),                    \
	};                                                                                         \
                                                                                                   \
	static int stm32_otghs_dev_init_##n(const struct device *dev)                              \
	{                                                                                          \
		const int ret = stm32_driver_preinit(dev);                                         \
                                                                                                   \
		if (ret != 0) {                                                                    \
			return ret;                                                                \
		}                                                                                  \
                                                                                                   \
		stm32_otghs_irq_config_##n(dev);                                                   \
                                                                                                   \
		k_thread_create(&stm32_otghs_thread_##n, stm32_otghs_stack_##n,                    \
				K_KERNEL_STACK_SIZEOF(stm32_otghs_stack_##n),                      \
				stm32_otghs_thread_entry_##n, (void *)dev, NULL, NULL,             \
				K_PRIO_PREEMPT(STM32_OTGHS_INST_THREAD_PRIO(n)), 0, K_NO_WAIT);    \
		k_thread_name_set(&stm32_otghs_thread_##n, "uhc_stm32_otghs");                     \
                                                                                                   \
		irq_kick(dev_data(dev));                                                           \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, stm32_otghs_dev_init_##n, NULL, &stm32_uhc_data_##n,              \
			      &stm32_otghs_cfg_##n, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &stm32_uhc_api);

#define STM32_OTGHS_INST_DEFINE_IF_HOST(n)                                                         \
	COND_CODE_1(USB_STM32_NODE_HAS_HOST_ROLE(DT_DRV_INST(n)),                             \
		    (STM32_OTGHS_INST_DEFINE(n)), ())

DT_INST_FOREACH_STATUS_OKAY(STM32_OTGHS_INST_DEFINE_IF_HOST)
