/*
 * Copyright (c) 2026 Hamza Bahri
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  uhc_stm32.c
 * @brief STM32 USB host controller (UHC) driver for the USB_DRD_FS IP.
 *
 * Bus level: init() powers and initializes the HAL HCD, enable() starts it
 * and unmasks the IRQ, and the HAL connect/disconnect/port-enabled callbacks
 * are translated into UHC events (connect/disconnect debounced).
 *
 * Transfer engine: event driven, one transfer in flight per host channel.
 * ep_enqueue() appends a transfer and wakes the worker thread, which starts
 * every queued transfer whose channel is free. Completions arrive through
 * HAL_HCD_HC_NotifyURBChange_Callback() as per-channel event bits; the
 * thread then advances the transfer (next control stage, NAK retry, or
 * completion). Control transfers own the fixed EP0 channel pair and are
 * serialised; bulk and interrupt endpoints get a channel of their own on
 * first use so the HAL data-toggle state persists across transfers.
 * Isochronous endpoints are not supported (-ENOTSUP).
 */

#include <soc.h>
#include <stm32cube_hal.h>
#include <string.h>
#include <stm32_usb_common.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>

#include "uhc_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_stm32, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_usb_drd

#define UHC_STM32_IRQ     DT_INST_IRQ_BY_NAME(0, usb, irq)
#define UHC_STM32_IRQ_PRI DT_INST_IRQ_BY_NAME(0, usb, priority)

/*
 * ch_num 0/1 are reserved by the HAL for control transfers.
 * Values starting from 2 can be used for any other endpoint.
 *
 * The IP has 8 channel registers, each holding an OUT and an IN physical
 * channel (16 in total); HAL_HCD_HC_Init() binds a ch_num to a free one.
 * Its check for pairing the IN and OUT halves of one endpoint in a register
 * never matches (the endpoint number is compared unshifted), so each data
 * ch_num occupies a whole register: at most 7 besides EP0.
 */
#define UHC_STM32_CTRL_OUT_CHNUM  0
#define UHC_STM32_CTRL_IN_CHNUM   1
#define UHC_STM32_FIRST_APP_CHNUM 2
#define UHC_STM32_NUM_CHNUM       16

/* HAL_HCD_HC_SubmitRequest() token and direction arguments. */
#define HC_TOKEN_SETUP 0U
#define HC_TOKEN_DATA  1U
#define HC_DIR_OUT     0U
#define HC_DIR_IN      1U

/* Worker thread event bits: one per channel, plus queue and port events. */
#define UHC_STM32_EVT_CH(ch)    BIT(ch)
#define UHC_STM32_EVT_XFER      BIT(UHC_STM32_NUM_CHNUM)
#define UHC_STM32_EVT_REMOVED   BIT(UHC_STM32_NUM_CHNUM + 1)
#define UHC_STM32_EVT_CONNECTED BIT(UHC_STM32_NUM_CHNUM + 2)

/* Delay before re-sending a transaction the device NAKed (OUT) or that hit a
 * recoverable error; interrupt IN polls use the endpoint's bInterval instead.
 */
#define UHC_STM32_NAK_RETRY K_MSEC(1)

/* Post-reset recovery: USB 2.0 (9.2.6.2) grants the device 10 ms after reset
 * before it must respond; the ST reference host waits 100 ms. The host stack
 * enumerates immediately after bus_reset() returns, so the wait lives here.
 * A transaction sent too early gets no handshake, which the DRD IP reports
 * only via ISTR_ERR (no URB state change) -- i.e. a silent timeout.
 */
#define UHC_STM32_RESET_RECOVERY K_MSEC(100)

struct uhc_stm32_chan {
	/* Transfer in flight, NULL when idle */
	struct uhc_transfer *xfer;

	/* Delayed resubmit after NAK */
	struct k_work_delayable retry_work;

	/* Index of this channel in uhc_stm32_data.ch[] */
	uint8_t idx;

	/* Endpoint bound to this channel (data channels) */
	uint8_t ep;

	/* Next channel event resubmits instead of completing */
	bool retry;
};

/* Per-instance mutable state. The HAL handle is embedded so we can recover
 * the driver state from a HAL callback via CONTAINER_OF.
 */
struct uhc_stm32_data {
	HCD_HandleTypeDef hcd;
	const struct device *dev;
	struct k_thread thread;

	/* Channel completions, queue and port events */
	struct k_event events;

	/* Connect/disconnect debounce */
	struct k_work_delayable conn_work;

	/* Per-channel metadata */
	struct uhc_stm32_chan ch[UHC_STM32_NUM_CHNUM];

	/* Data channels opened by HAL_HCD_HC_Init(); their toggle state is live */
	uint32_t used_channels;

	/* Disconnect edge not yet reported to the host stack */
	atomic_t removal_pending;

	/* EP0 channel running the current control stage */
	uint8_t ctrl_ch;
};

/* Single controller instance on this SoC; one shared worker stack. */
K_KERNEL_STACK_DEFINE(uhc_stm32_thread_stack, CONFIG_UHC_STM32_STACK_SIZE);

/* Per-instance constant configuration. */
struct uhc_stm32_config {
	const struct pinctrl_dev_config *pcfg;
	void (*irq_connect)(void);
};

#define hhcd2data(hhcd) CONTAINER_OF(hhcd, struct uhc_stm32_data, hcd)

/*
 * ---- Clock --------------------------------------------------------------
 */

static const struct stm32_pclken pclken[] = STM32_DT_INST_CLOCKS(0);

static int uhc_stm32_clock_enable(void)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (DT_INST_NUM_CLOCKS(0) > 1) {
		if (clock_control_configure(clk, (clock_control_subsys_t)&pclken[1], NULL) != 0) {
			LOG_ERR("Could not select USB domain clock");
			return -EIO;
		}
	}

	if (clock_control_on(clk, (clock_control_subsys_t)&pclken[0]) != 0) {
		LOG_ERR("Unable to enable USB clock");
		return -EIO;
	}

	return 0;
}

static int uhc_stm32_clock_disable(void)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (clock_control_off(clk, (clock_control_subsys_t)&pclken[0]) != 0) {
		LOG_ERR("Unable to disable USB clock");
		return -EIO;
	}

	return 0;
}

/*
 * ---- HAL callbacks (HAL -> us), run in ISR context ----------------------
 *
 * uhc_submit_event() forwards to the host stack, whose callback only does a
 * k_msgq_put(K_NO_WAIT), so calling it from the ISR is safe.
 *
 * Connect/disconnect are debounced: this board bounces the line on plug
 * events, so we only act once the line has been stable for the debounce
 * delay. A disconnect inside that window is still reported before the
 * reconnect.
 */

static void uhc_stm32_conn_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uhc_stm32_data *priv = CONTAINER_OF(dwork, struct uhc_stm32_data, conn_work);

	/* Channel state belongs to the worker thread: it fails the outstanding
	 * transfers and reports the removal, before any reconnect.
	 */
	if (atomic_clear(&priv->removal_pending) != 0) {
		LOG_INF("USB device removed");
		k_event_post(&priv->events, UHC_STM32_EVT_REMOVED);
	}

	/* Act on the settled line state, not on the last edge: enable() also
	 * schedules this work for a device that stayed attached.
	 */
	if ((priv->hcd.Instance->ISTR & USB_ISTR_DCON_STAT) != 0U) {
		/* As HCD_Port_IRQHandler() does, so the next ResetPort() reports
		 * the port enabled again.
		 */
		priv->hcd.HostState = HCD_HCD_STATE_CONNECTED;
		k_event_post(&priv->events, UHC_STM32_EVT_CONNECTED);
	}
}

/* On disconnect the HAL forgets its physical channel mapping but leaves the
 * channels armed; disable them or a replugged device answers into unmapped
 * channels (reported as chnum 0xff, stored by the HAL in hc[15]).
 */
static void uhc_stm32_halt_phy_channels(struct uhc_stm32_data *priv)
{
	for (uint8_t phy = 0; phy < priv->hcd.Init.Host_channels; phy++) {
		(void)USB_HC_IN_Halt(priv->hcd.Instance, phy);
		(void)USB_HC_OUT_Halt(priv->hcd.Instance, phy);
	}
}

void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{
	struct uhc_stm32_data *priv = hhcd2data(hhcd);

	k_work_reschedule(&priv->conn_work, K_MSEC(CONFIG_UHC_STM32_DEBOUNCE_DELAY_MS));
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{
	struct uhc_stm32_data *priv = hhcd2data(hhcd);

	uhc_stm32_halt_phy_channels(priv);
	atomic_set(&priv->removal_pending, 1);
	k_work_reschedule(&priv->conn_work, K_MSEC(CONFIG_UHC_STM32_DEBOUNCE_DELAY_MS));
}

void HAL_HCD_PortEnabled_Callback(HCD_HandleTypeDef *hhcd)
{
	struct uhc_stm32_data *priv = hhcd2data(hhcd);

	/* Fires on the first SOF after reset: proof SOF generation is running. */
	LOG_INF("port enabled, SOF active");
	uhc_submit_event(priv->dev, UHC_EVT_RESETED, 0);
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum,
					 HCD_URBStateTypeDef urb_state)
{
	struct uhc_stm32_data *priv = hhcd2data(hhcd);

	LOG_DBG("URB cb ch%u state%u", chnum, urb_state);

	/* chnum 0xff: the ISR could not map the physical channel back to a
	 * ch_num (HAL bookkeeping lost); nothing we can attribute it to.
	 */
	if (chnum >= UHC_STM32_NUM_CHNUM) {
		LOG_WRN("URB event on unmapped channel (state %u)", urb_state);
		return;
	}

	k_event_post(&priv->events, UHC_STM32_EVT_CH(chnum));
}

/* Delayed NAK/interrupt-poll retry: hand the channel back to the thread. */
static void uhc_stm32_retry_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uhc_stm32_chan *chan = CONTAINER_OF(dwork, struct uhc_stm32_chan, retry_work);
	struct uhc_stm32_data *priv = CONTAINER_OF(chan, struct uhc_stm32_data, ch[chan->idx]);

	k_event_post(&priv->events, UHC_STM32_EVT_CH(chan->idx));
}

/*
 * ---- Transfer engine ----------------------------------------------------
 *
 * All functions below run in the worker thread with the driver lock held,
 * except where noted. They never block on the bus.
 */

/* Open (or re-open) a host channel.
 *
 * On the USB_DRD_FS, HAL_HCD_HC_Init() allocates the physical channel and its
 * packet memory (PMA) internally -- including the special dual-direction EP0
 * allocation -- guarded so re-opening an already-used channel does not
 * re-allocate. We must therefore NOT call HAL_HCD_PMAlloc() ourselves; doing so
 * double-allocates and corrupts the PMA lookup table. HAL_ERROR means the IP
 * is out of physical channels or PMA.
 */
static int uhc_stm32_ch_open(struct uhc_stm32_data *priv, uint8_t ch, uint8_t epaddr,
			     uint8_t dev_addr, uint8_t speed, uint8_t ep_type, uint16_t mps)
{
	if (HAL_HCD_HC_Init(&priv->hcd, ch, epaddr, dev_addr, speed, ep_type, mps) != HAL_OK) {
		LOG_ERR("no free host channel for ep 0x%02x", epaddr);
		return -ENOMEM;
	}

	return 0;
}

/* Drop a channel's endpoint binding and fail its transfer, if any. */
static void uhc_stm32_ch_release(const struct device *dev, uint8_t ch)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_stm32_chan *chan = &priv->ch[ch];
	struct uhc_transfer *xfer = chan->xfer;

	k_work_cancel_delayable(&chan->retry_work);
	chan->retry = false;
	chan->ep = 0;
	chan->xfer = NULL;
	priv->used_channels &= ~BIT(ch);

	if (xfer != NULL) {
		uhc_xfer_return(dev, xfer, -ECONNRESET);
	}
}

/* HAL device speed for the device this transfer addresses. */
static uint8_t uhc_stm32_dev_speed(const struct uhc_transfer *const xfer)
{
	if (xfer->udev->speed == USB_SPEED_SPEED_LS) {
		return HCD_DEVICE_SPEED_LOW;
	}

	return HCD_DEVICE_SPEED_FULL;
}

/* HAL endpoint type for a transfer. The host stack fills xfer->type from the
 * endpoint descriptor at allocation (uhc_xfer_alloc), so no descriptor walk
 * is needed here.
 */
static uint8_t uhc_stm32_ep_type(const struct uhc_transfer *const xfer)
{
	switch (xfer->type) {
	case USB_EP_TYPE_CONTROL:
		return EP_TYPE_CTRL;
	case USB_EP_TYPE_INTERRUPT:
		return EP_TYPE_INTR;
	case USB_EP_TYPE_ISO:
		return EP_TYPE_ISOC;
	default:
		return EP_TYPE_BULK;
	}
}

/* Start one transaction on a channel; the result arrives later as a channel
 * event. The HAL handles multi-packet transfers itself.
 */
static int uhc_stm32_submit(struct uhc_stm32_data *priv, uint8_t ch, uint8_t dir, uint8_t ep_type,
			    uint8_t token, uint8_t *buf, uint16_t len)
{
	if (HAL_HCD_HC_SubmitRequest(&priv->hcd, ch, dir, ep_type, token, buf, len, 0) != HAL_OK) {
		return -EIO;
	}

	return 0;
}

static void uhc_stm32_ch_schedule_retry(struct uhc_stm32_chan *chan, k_timeout_t delay)
{
	chan->retry = true;
	k_work_reschedule(&chan->retry_work, delay);
}

/* Whether a URB_NOTREADY on this channel needs a software resubmit.
 *
 * On the DRD the hardware keeps a channel armed after a NAK or a recoverable
 * transaction error and retries by itself (which is how the HAL can count up
 * to 3 errors without help), with two exceptions where the HAL disables the
 * channel and software must re-send: an OUT NAK, and an interrupt-IN NAK.
 *
 * Only in those two cases is hc->state stable enough to read: a control or
 * bulk IN reports NOTREADY on a NAK and then keeps going, so by the time the
 * thread looks, hc->state may already reflect a later packet of the SAME
 * transfer -- resubmitting there restarts the HAL byte count mid-transfer
 * and corrupts the data (seen as "Configuration descriptor read mismatch").
 */
static bool uhc_stm32_needs_resubmit(struct uhc_stm32_data *priv, uint8_t ch, bool is_in)
{
	const USB_DRD_HCTypeDef *hc = &priv->hcd.hc[ch];

	if (is_in && hc->ep_type != EP_TYPE_INTR) {
		return false;
	}

	return hc->state == HC_NAK;
}

static bool uhc_stm32_xfer_is_active(struct uhc_stm32_data *priv, const struct uhc_transfer *xfer)
{
	for (int ch = 0; ch < UHC_STM32_NUM_CHNUM; ch++) {
		if (priv->ch[ch].xfer == xfer) {
			return true;
		}
	}

	return false;
}

/*
 * ---- Control transfers (EP0 channel pair, one at a time) ----------------
 */

/* Submit the transaction for the transfer's current stage and remember which
 * EP0 channel it runs on. The status stage runs opposite to the data stage
 * and IN when there was no data stage.
 */
static int uhc_stm32_ctrl_submit_stage(struct uhc_stm32_data *priv, struct uhc_transfer *const xfer)
{
	const struct usb_setup_packet *setup = (const void *)xfer->setup_pkt;
	const bool data_in = usb_reqtype_is_to_host(setup);

	switch (xfer->stage) {
	case UHC_CONTROL_STAGE_SETUP:
		/* Always 8 bytes: setup_pkt itself is padded to the cache-line
		 * size upstream, so sizeof() must not be used.
		 */
		priv->ctrl_ch = UHC_STM32_CTRL_OUT_CHNUM;
		return uhc_stm32_submit(priv, UHC_STM32_CTRL_OUT_CHNUM, HC_DIR_OUT, EP_TYPE_CTRL,
					HC_TOKEN_SETUP, xfer->setup_pkt,
					sizeof(struct usb_setup_packet));
	case UHC_CONTROL_STAGE_DATA:
		if (data_in) {
			/* Ask for wLength exactly (checked against the buffer at
			 * start): asking for more never ends a full-length reply
			 * that has no short packet.
			 */
			priv->ctrl_ch = UHC_STM32_CTRL_IN_CHNUM;
			return uhc_stm32_submit(priv, UHC_STM32_CTRL_IN_CHNUM, HC_DIR_IN,
						EP_TYPE_CTRL, HC_TOKEN_DATA,
						net_buf_tail(xfer->buf),
						sys_le16_to_cpu(setup->wLength));
		}

		priv->ctrl_ch = UHC_STM32_CTRL_OUT_CHNUM;
		return uhc_stm32_submit(priv, UHC_STM32_CTRL_OUT_CHNUM, HC_DIR_OUT, EP_TYPE_CTRL,
					HC_TOKEN_DATA, xfer->buf->data, xfer->buf->len);
	case UHC_CONTROL_STAGE_STATUS:
	default:
		if (xfer->buf != NULL && data_in) {
			priv->ctrl_ch = UHC_STM32_CTRL_OUT_CHNUM;
			return uhc_stm32_submit(priv, UHC_STM32_CTRL_OUT_CHNUM, HC_DIR_OUT,
						EP_TYPE_CTRL, HC_TOKEN_DATA, NULL, 0);
		}

		priv->ctrl_ch = UHC_STM32_CTRL_IN_CHNUM;
		return uhc_stm32_submit(priv, UHC_STM32_CTRL_IN_CHNUM, HC_DIR_IN, EP_TYPE_CTRL,
					HC_TOKEN_DATA, NULL, 0);
	}
}

static void uhc_stm32_ctrl_finish(const struct device *dev, int err)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_stm32_chan *chan = &priv->ch[UHC_STM32_CTRL_OUT_CHNUM];
	struct uhc_transfer *xfer = chan->xfer;

	chan->xfer = NULL;
	chan->retry = false;
	priv->ch[UHC_STM32_CTRL_IN_CHNUM].retry = false;
	uhc_xfer_return(dev, xfer, err);
}

/* The control transfer in flight is tracked on the OUT channel. */
static int uhc_stm32_ctrl_start(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_stm32_chan *chan = &priv->ch[UHC_STM32_CTRL_OUT_CHNUM];
	const struct usb_setup_packet *setup = (const void *)xfer->setup_pkt;
	const uint8_t addr = xfer->udev->addr;
	const uint8_t speed = uhc_stm32_dev_speed(xfer);
	int err;

	if (chan->xfer != NULL) {
		return -EBUSY;
	}

	if (usb_reqtype_is_to_host(setup) && xfer->buf != NULL &&
	    sys_le16_to_cpu(setup->wLength) > net_buf_tailroom(xfer->buf)) {
		LOG_ERR("Control IN buffer too small: wLength %u, tailroom %zu",
			sys_le16_to_cpu(setup->wLength), net_buf_tailroom(xfer->buf));
		return -EINVAL;
	}

	/* Re-init EP0 channels each transfer: the device address and EP0 max
	 * packet size change during enumeration, and this also resets the HAL
	 * data-toggle flags as required for a fresh control transfer.
	 *
	 * ORDER MATTERS: IN (0x80) must be opened before OUT (0x00). Opening
	 * OUT first takes the HAL's dual EP0 PMA-alloc path, after which every
	 * EP0 HC_Init overwrites BOTH direction registrations of phy channel 0
	 * with its own channel number -- OUT completions are then credited to
	 * the IN channel and the OUT channel never leaves URB_IDLE. The ST
	 * Cube host stack always opens pipe_in first for the same reason.
	 */
	err = uhc_stm32_ch_open(priv, UHC_STM32_CTRL_IN_CHNUM, 0x80, addr, speed, EP_TYPE_CTRL,
				xfer->mps);
	if (err != 0) {
		return err;
	}

	err = uhc_stm32_ch_open(priv, UHC_STM32_CTRL_OUT_CHNUM, 0x00, addr, speed, EP_TYPE_CTRL,
				xfer->mps);
	if (err != 0) {
		return err;
	}

	xfer->stage = UHC_CONTROL_STAGE_SETUP;
	chan->xfer = xfer;

	err = uhc_stm32_ctrl_submit_stage(priv, xfer);
	if (err != 0) {
		chan->xfer = NULL;
	}

	return err;
}

/* Channel event on one of the EP0 channels: advance the control transfer. */
static void uhc_stm32_ctrl_handle_event(const struct device *dev, uint8_t ch, bool resubmit)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_transfer *xfer = priv->ch[UHC_STM32_CTRL_OUT_CHNUM].xfer;
	const bool is_in = (ch == UHC_STM32_CTRL_IN_CHNUM);
	HCD_URBStateTypeDef urb;
	int err;

	/* Nothing in flight, or the other EP0 channel: spurious. */
	if (xfer == NULL || ch != priv->ctrl_ch) {
		return;
	}

	if (resubmit) {
		err = uhc_stm32_ctrl_submit_stage(priv, xfer);
		if (err != 0) {
			uhc_stm32_ctrl_finish(dev, err);
		}
		return;
	}

	urb = HAL_HCD_HC_GetURBState(&priv->hcd, ch);
	switch (urb) {
	case URB_IDLE:
		return;
	case URB_NOTREADY:
		if (uhc_stm32_needs_resubmit(priv, ch, is_in)) {
			uhc_stm32_ch_schedule_retry(&priv->ch[ch], UHC_STM32_NAK_RETRY);
		}
		return;
	case URB_DONE:
		break;
	case URB_STALL:
		LOG_DBG("control STALL, stage %u", xfer->stage);
		uhc_stm32_ctrl_finish(dev, -EPIPE);
		return;
	default:
		LOG_ERR("control error ch%u stage%u urb%u ErrCnt%u", ch, xfer->stage, urb,
			priv->hcd.hc[ch].ErrCnt);
		uhc_stm32_ctrl_finish(dev, -EIO);
		return;
	}

	switch (xfer->stage) {
	case UHC_CONTROL_STAGE_SETUP:
		xfer->stage =
			(xfer->buf != NULL) ? UHC_CONTROL_STAGE_DATA : UHC_CONTROL_STAGE_STATUS;
		break;
	case UHC_CONTROL_STAGE_DATA:
		if (is_in) {
			net_buf_add(xfer->buf, MIN(HAL_HCD_HC_GetXferCount(&priv->hcd, ch),
						   net_buf_tailroom(xfer->buf)));
		}

		if (xfer->no_status) {
			uhc_stm32_ctrl_finish(dev, 0);
			return;
		}

		xfer->stage = UHC_CONTROL_STAGE_STATUS;
		break;
	case UHC_CONTROL_STAGE_STATUS:
	default:
		uhc_stm32_ctrl_finish(dev, 0);
		return;
	}

	err = uhc_stm32_ctrl_submit_stage(priv, xfer);
	if (err != 0) {
		uhc_stm32_ctrl_finish(dev, err);
	}
}

/*
 * ---- Bulk and interrupt transfers (one channel per endpoint) ------------
 */

/* Find the channel bound to xfer->ep, allocating a free one on first use.
 * Channels are initialized once and kept, so the HAL data-toggle state
 * survives across transfers (re-initializing would corrupt DATA0/1).
 */
static int uhc_stm32_data_channel(struct uhc_stm32_data *priv, struct uhc_transfer *const xfer)
{
	for (int ch = UHC_STM32_FIRST_APP_CHNUM; ch < UHC_STM32_NUM_CHNUM; ch++) {
		if ((priv->used_channels & BIT(ch)) != 0U && priv->ch[ch].ep == xfer->ep) {
			return ch;
		}
	}

	for (int ch = UHC_STM32_FIRST_APP_CHNUM; ch < UHC_STM32_NUM_CHNUM; ch++) {
		int err;

		if ((priv->used_channels & BIT(ch)) != 0U) {
			continue;
		}

		err = uhc_stm32_ch_open(priv, ch, xfer->ep, xfer->udev->addr,
					uhc_stm32_dev_speed(xfer), uhc_stm32_ep_type(xfer),
					xfer->mps);
		if (err != 0) {
			return err;
		}

		/* HC_Init keeps the toggles of the channel's previous owner. */
		priv->hcd.hc[ch].toggle_in = 0U;
		priv->hcd.hc[ch].toggle_out = 0U;
		priv->used_channels |= BIT(ch);
		priv->ch[ch].ep = xfer->ep;
		return ch;
	}

	return -ENOMEM;
}

static int uhc_stm32_data_submit(struct uhc_stm32_data *priv, uint8_t ch,
				 struct uhc_transfer *const xfer)
{
	const uint8_t ep_type = uhc_stm32_ep_type(xfer);

	if (xfer->buf == NULL) {
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		return uhc_stm32_submit(priv, ch, HC_DIR_IN, ep_type, HC_TOKEN_DATA,
					net_buf_tail(xfer->buf), net_buf_tailroom(xfer->buf));
	}

	return uhc_stm32_submit(priv, ch, HC_DIR_OUT, ep_type, HC_TOKEN_DATA, xfer->buf->data,
				xfer->buf->len);
}

static void uhc_stm32_data_finish(const struct device *dev, struct uhc_stm32_chan *chan, int err)
{
	struct uhc_transfer *xfer = chan->xfer;

	chan->xfer = NULL;
	chan->retry = false;
	uhc_xfer_return(dev, xfer, err);
}

static int uhc_stm32_data_start(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_stm32_chan *chan;
	int ch, err;

	ch = uhc_stm32_data_channel(priv, xfer);
	if (ch < 0) {
		return ch;
	}

	chan = &priv->ch[ch];
	if (chan->xfer != NULL) {
		return -EBUSY;
	}

	chan->xfer = xfer;
	err = uhc_stm32_data_submit(priv, ch, xfer);
	if (err != 0) {
		chan->xfer = NULL;
	}

	return err;
}

static void uhc_stm32_data_handle_event(const struct device *dev, uint8_t ch, bool resubmit)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_stm32_chan *chan = &priv->ch[ch];
	struct uhc_transfer *xfer = chan->xfer;
	HCD_URBStateTypeDef urb;
	int err;

	if (xfer == NULL) {
		return;
	}

	if (resubmit) {
		err = uhc_stm32_data_submit(priv, ch, xfer);
		if (err != 0) {
			uhc_stm32_data_finish(dev, chan, err);
		}
		return;
	}

	urb = HAL_HCD_HC_GetURBState(&priv->hcd, ch);
	switch (urb) {
	case URB_IDLE:
		return;
	case URB_NOTREADY:
		if (uhc_stm32_needs_resubmit(priv, ch, USB_EP_DIR_IS_IN(xfer->ep))) {
			/* Interrupt endpoints are polled at their bInterval
			 * (frames, i.e. ms at full speed).
			 */
			k_timeout_t delay = (xfer->type == USB_EP_TYPE_INTERRUPT)
						    ? K_MSEC(MAX(xfer->interval, 1))
						    : UHC_STM32_NAK_RETRY;

			uhc_stm32_ch_schedule_retry(chan, delay);
		}
		return;
	case URB_DONE:
		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			net_buf_add(xfer->buf, MIN(HAL_HCD_HC_GetXferCount(&priv->hcd, ch),
						   net_buf_tailroom(xfer->buf)));
		}
		uhc_stm32_data_finish(dev, chan, 0);
		return;
	case URB_STALL:
		LOG_DBG("ep 0x%02x STALL", xfer->ep);
		uhc_stm32_data_finish(dev, chan, -EPIPE);
		return;
	default:
		LOG_ERR("ep 0x%02x error ch%u urb%u ErrCnt%u", xfer->ep, ch, urb,
			priv->hcd.hc[ch].ErrCnt);
		uhc_stm32_data_finish(dev, chan, -EIO);
		return;
	}
}

/*
 * ---- Worker thread ------------------------------------------------------
 */

static void uhc_stm32_ch_handle_event(const struct device *dev, uint8_t ch)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_stm32_chan *chan = &priv->ch[ch];
	const bool resubmit = chan->retry;

	chan->retry = false;

	if (ch == UHC_STM32_CTRL_OUT_CHNUM || ch == UHC_STM32_CTRL_IN_CHNUM) {
		uhc_stm32_ctrl_handle_event(dev, ch, resubmit);
	} else {
		uhc_stm32_data_handle_event(dev, ch, resubmit);
	}
}

/* Start every queued transfer whose channel is free. */
static void uhc_stm32_submit_pending(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *xfer, *tmp;
	int err;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->ctrl_xfers, xfer, tmp, node) {
		if (xfer->err == -ECONNRESET || uhc_stm32_xfer_is_active(priv, xfer)) {
			continue;
		}

		switch (xfer->type) {
		case USB_EP_TYPE_CONTROL:
			err = uhc_stm32_ctrl_start(dev, xfer);
			break;
		case USB_EP_TYPE_ISO:
			err = -ENOTSUP;
			break;
		default:
			err = uhc_stm32_data_start(dev, xfer);
			break;
		}

		/* Channel busy: stays queued, retried on the next event. */
		if (err == -EBUSY) {
			continue;
		}

		if (err != 0) {
			LOG_ERR("cannot start xfer ep 0x%02x (%d)", xfer->ep, err);
			uhc_xfer_return(dev, xfer, err);
		}
	}
}

/* Return transfers marked by ep_dequeue(). An in-flight one has its channel
 * halted first so a late completion cannot be credited to it.
 */
static void uhc_stm32_dequeue_cancelled(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *xfer, *tmp;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->ctrl_xfers, xfer, tmp, node) {
		if (xfer->err != -ECONNRESET) {
			continue;
		}

		for (int ch = 0; ch < UHC_STM32_NUM_CHNUM; ch++) {
			struct uhc_stm32_chan *chan = &priv->ch[ch];

			if (chan->xfer != xfer) {
				continue;
			}

			/* Control stages may run on the other EP0 channel. */
			HAL_HCD_HC_Halt(&priv->hcd,
					(ch == UHC_STM32_CTRL_OUT_CHNUM) ? priv->ctrl_ch : ch);
			k_work_cancel_delayable(&chan->retry_work);
			chan->retry = false;
			chan->xfer = NULL;
		}

		uhc_xfer_return(dev, xfer, -ECONNRESET);
	}
}

/* Device gone (debounced disconnect, or controller disabled): the HAL has
 * released the physical channels and reset the PMA. Fail everything
 * outstanding for the device, drop our channel bindings, then tell the host
 * stack. The transfer completions are queued to the stack before the removal
 * event so class drivers see them while the device object still exists.
 */
static void uhc_stm32_handle_removed(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *xfer, *tmp;

	for (uint8_t ch = 0; ch < UHC_STM32_NUM_CHNUM; ch++) {
		uhc_stm32_ch_release(dev, ch);
	}

	/* A NAK retry may have re-armed a channel during the debounce window. */
	uhc_stm32_halt_phy_channels(priv);

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&data->ctrl_xfers, xfer, tmp, node) {
		uhc_xfer_return(dev, xfer, -ECONNRESET);
	}

	uhc_submit_event(dev, UHC_EVT_DEV_REMOVED, 0);
}

static void uhc_stm32_handle_connected(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	enum uhc_event_type type;

	if (HAL_HCD_GetCurrentSpeed(&priv->hcd) == HCD_DEVICE_SPEED_LOW) {
		type = UHC_EVT_DEV_CONNECTED_LS;
	} else {
		type = UHC_EVT_DEV_CONNECTED_FS;
	}

	LOG_INF("USB device connected (%s)",
		type == UHC_EVT_DEV_CONNECTED_LS ? "low speed" : "full speed");
	uhc_submit_event(dev, type, 0);
}

static void uhc_stm32_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	uint32_t events;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		events = k_event_wait_safe(&priv->events, UINT32_MAX, false, K_FOREVER);

		uhc_lock_internal(dev, K_FOREVER);

		if ((events & UHC_STM32_EVT_REMOVED) != 0U) {
			uhc_stm32_handle_removed(dev);
		}

		/* A connect debounced just before disable() must not be reported. */
		if ((events & UHC_STM32_EVT_CONNECTED) != 0U && uhc_is_enabled(dev)) {
			uhc_stm32_handle_connected(dev);
		}

		for (uint8_t ch = 0; ch < UHC_STM32_NUM_CHNUM; ch++) {
			if ((events & UHC_STM32_EVT_CH(ch)) != 0U) {
				uhc_stm32_ch_handle_event(dev, ch);
			}
		}

		if ((events & UHC_STM32_EVT_XFER) != 0U) {
			uhc_stm32_dequeue_cancelled(dev);
		}

		uhc_stm32_submit_pending(dev);

		uhc_unlock_internal(dev);
	}
}

/*
 * ---- API entry points, in struct uhc_driver_api order ------------------
 */

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
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	HAL_StatusTypeDef status;
	int ret;

	ret = stm32_usb_pwr_enable();
	if (ret != 0) {
		LOG_ERR("Error enabling USB power: %d", ret);
		return ret;
	}

	ret = uhc_stm32_clock_enable();
	if (ret != 0) {
		(void)stm32_usb_pwr_disable();
		return ret;
	}

	memset(&priv->hcd, 0, sizeof(priv->hcd));
	priv->hcd.Instance = USB_DRD_FS;
	priv->hcd.Init.Host_channels = USB_DRD_USED_CHANNELS;
	priv->hcd.Init.dma_enable = 0;
	priv->hcd.Init.speed = HCD_SPEED_FULL;
	priv->hcd.Init.phy_itface = HCD_PHY_EMBEDDED;
	priv->hcd.Init.Sof_enable = 0;
	priv->hcd.Init.low_power_enable = 0;

	status = HAL_HCD_Init(&priv->hcd);
	if (status != HAL_OK) {
		LOG_ERR("HAL_HCD_Init failed (%d)", (int)status);
		(void)uhc_stm32_clock_disable();
		(void)stm32_usb_pwr_disable();
		return -EIO;
	}

	LOG_DBG("HCD initialized");

	return 0;
}

static int uhc_stm32_enable(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	HAL_StatusTypeDef status;

	status = HAL_HCD_Start(&priv->hcd);
	if (status != HAL_OK) {
		LOG_ERR("HAL_HCD_Start failed (%d)", (int)status);
		return -EIO;
	}

	irq_enable(UHC_STM32_IRQ);

	/* DCON only fires on line changes: a device already attached (e.g.
	 * left plugged in across disable()) raises none.
	 */
	k_work_reschedule(&priv->conn_work, K_MSEC(CONFIG_UHC_STM32_DEBOUNCE_DELAY_MS));

	LOG_DBG("HCD started");

	return 0;
}

static int uhc_stm32_disable(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	HAL_StatusTypeDef status;

	k_work_cancel_delayable(&priv->conn_work);
	irq_disable(UHC_STM32_IRQ);

	/* HAL_HCD_Stop() drops every channel mapping: fail what is outstanding
	 * and report the device gone, as an unplug would.
	 */
	atomic_clear(&priv->removal_pending);
	uhc_stm32_handle_removed(dev);

	status = HAL_HCD_Stop(&priv->hcd);
	if (status != HAL_OK) {
		LOG_ERR("HAL_HCD_Stop failed (%d)", (int)status);
		return -EIO;
	}

	return 0;
}

static int uhc_stm32_shutdown(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	int err, ret;

	HAL_HCD_DeInit(&priv->hcd);

	err = uhc_stm32_clock_disable();

	ret = stm32_usb_pwr_disable();
	if (ret != 0) {
		LOG_ERR("Error disabling USB power: %d", ret);
	}

	return (err != 0) ? err : ret;
}

static int uhc_stm32_bus_reset(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	HAL_StatusTypeDef status;

	/* The reset puts every device endpoint back to DATA0 (and the device
	 * to address 0): close the data channels so their next use reopens
	 * them with fresh toggles and address, and fail what was in flight.
	 */
	for (uint8_t ch = UHC_STM32_FIRST_APP_CHNUM; ch < UHC_STM32_NUM_CHNUM; ch++) {
		if ((priv->used_channels & BIT(ch)) != 0U) {
			(void)HAL_HCD_HC_Close(&priv->hcd, ch);
			uhc_stm32_ch_release(dev, ch);
		}
	}

	status = HAL_HCD_ResetPort(&priv->hcd);
	if (status != HAL_OK) {
		LOG_ERR("HAL_HCD_ResetPort failed (%d)", (int)status);
		return -EIO;
	}

	k_sleep(UHC_STM32_RESET_RECOVERY);

	return 0;
}

static int uhc_stm32_sof_enable(const struct device *dev)
{
	/* The DRD IP starts SOF generation itself once the port is enabled
	 * after a bus reset; there is nothing to do here.
	 */
	return 0;
}

static int uhc_stm32_bus_suspend(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	if (HAL_HCD_Suspend(&priv->hcd) != HAL_OK) {
		return -EIO;
	}

	/* HAL_HCD_Suspend() is synchronous, report completion right away. */
	uhc_submit_event(dev, UHC_EVT_SUSPENDED, 0);

	return 0;
}

static int uhc_stm32_bus_resume(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	if (HAL_HCD_Resume(&priv->hcd) != HAL_OK) {
		return -EIO;
	}

	uhc_submit_event(dev, UHC_EVT_RESUMED, 0);

	return 0;
}

/* Called by the host stack with the driver lock held. */
static int uhc_stm32_ep_enqueue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);

	uhc_xfer_append(dev, xfer);
	k_event_post(&priv->events, UHC_STM32_EVT_XFER);

	return 0;
}

/* Called by the host stack with the driver lock held: mark the transfer and
 * let the thread cancel it (it owns the channel state).
 */
static int uhc_stm32_ep_dequeue(const struct device *dev, struct uhc_transfer *const xfer)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *tmp;

	SYS_DLIST_FOR_EACH_CONTAINER(&data->ctrl_xfers, tmp, node) {
		if (tmp == xfer) {
			tmp->err = -ECONNRESET;
		}
	}

	k_event_post(&priv->events, UHC_STM32_EVT_XFER);

	return 0;
}

/*
 * ---- Bring-up -----------------------------------------------------------
 */

static int uhc_stm32_driver_preinit(const struct device *dev)
{
	struct uhc_stm32_data *priv = uhc_get_private(dev);
	const struct uhc_stm32_config *cfg = dev->config;
	struct uhc_data *data = dev->data;
	int ret;

	priv->dev = dev;
	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	k_work_init_delayable(&priv->conn_work, uhc_stm32_conn_work);

	for (int ch = 0; ch < UHC_STM32_NUM_CHNUM; ch++) {
		priv->ch[ch].idx = ch;
		k_work_init_delayable(&priv->ch[ch].retry_work, uhc_stm32_retry_work);
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("USB pinctrl setup failed (%d)", ret);
		return ret;
	}

	k_thread_create(&priv->thread, uhc_stm32_thread_stack,
			K_KERNEL_STACK_SIZEOF(uhc_stm32_thread_stack), uhc_stm32_thread,
			(void *)dev, NULL, NULL, K_PRIO_COOP(CONFIG_UHC_STM32_THREAD_PRIORITY), 0,
			K_NO_WAIT);
	k_thread_name_set(&priv->thread, "uhc_stm32");

	cfg->irq_connect();

	LOG_DBG("STM32 UHC pre-initialized");

	return 0;
}

static DEVICE_API(uhc, uhc_stm32_api) = {
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

PINCTRL_DT_INST_DEFINE(0);

#define UHC_STM32_DEVICE_DEFINE(n)                                                                 \
	static struct uhc_stm32_data uhc_stm32_priv_##n;                                           \
                                                                                                   \
	static void uhc_stm32_irq_connect_##n(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, usb, irq),                                      \
			    DT_INST_IRQ_BY_NAME(n, usb, priority), HAL_HCD_IRQHandler,             \
			    &uhc_stm32_priv_##n.hcd, 0);                                           \
	}                                                                                          \
                                                                                                   \
	static const struct uhc_stm32_config uhc_stm32_config_##n = {                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.irq_connect = uhc_stm32_irq_connect_##n,                                          \
	};                                                                                         \
                                                                                                   \
	static struct uhc_data uhc_stm32_data_##n = {                                              \
		.priv = &uhc_stm32_priv_##n,                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uhc_stm32_driver_preinit, NULL, &uhc_stm32_data_##n,              \
			      &uhc_stm32_config_##n, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uhc_stm32_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_STM32_DEVICE_DEFINE)
