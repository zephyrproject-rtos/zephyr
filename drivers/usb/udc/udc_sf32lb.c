/*
 * Copyright (c) 2026 SiFli Technologies(Nanjing) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_usbd

#include "udc_common.h"

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <sf32lb/usb_dc_sf32lb.h>

LOG_MODULE_REGISTER(udc_sf32lb, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define HWREG(x)                 SF32LB_USBDC_HWREG(x)
#define HWREGH(x)                SF32LB_USBDC_HWREGH(x)
#define HWREGB(x)                SF32LB_USBDC_HWREGB(x)

#define SF32LB_SETUP_QUEUE_DEPTH 8U

enum sf32lb_ep0_state {
	SF32LB_EP0_IDLE,
	SF32LB_EP0_SETUP_IN_PENDING,
	SF32LB_EP0_IN_DATA,
	SF32LB_EP0_OUT_DATA,
	SF32LB_EP0_STATUS_IN_PENDING,
	SF32LB_EP0_STATUS_PENDING,
};

struct sf32lb_xfer {
	struct net_buf *buf;
	uint8_t *data;
	uint32_t remaining;
	bool zlp;
	bool active;
};

struct sf32lb_setup_msg {
	struct usb_setup_packet setup;
	uint32_t epoch;
};

struct udc_sf32lb_data {
	struct usb_setup_packet setup;
	struct sf32lb_xfer in_xfer[SF32LB_USBDC_MAX_ENDPOINTS];
	struct sf32lb_xfer out_xfer[SF32LB_USBDC_MAX_ENDPOINTS];
	enum sf32lb_ep0_state ep0_state;
	struct k_work setup_work;
	struct k_msgq setup_msgq;
	char setup_msgq_buffer[SF32LB_SETUP_QUEUE_DEPTH * sizeof(struct sf32lb_setup_msg)];
	struct k_spinlock setup_lock;
	uint32_t setup_epoch;
	const struct device *dev;
};

struct udc_sf32lb_config {
	uintptr_t base;
	uintptr_t cfg;
	const struct pinctrl_dev_config *pcfg;
	struct sf32lb_clock_dt_spec clock;
	void (*irq_config_func)(void);
	uint32_t eps_in_cnt;
	uint32_t eps_out_cnt;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	int speed_idx;
};

static void sf32lb_complete_ctrl_out(const struct device *dev, uint16_t rx_count);
static void sf32lb_reset_setup_queue(const struct device *dev);

static void sf32lb_xfer_reset(struct sf32lb_xfer *xfer)
{
	memset(xfer, 0, sizeof(*xfer));
}

static void sf32lb_flush_ctrl_fifo(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	uint8_t csr0 = HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0));

	if ((csr0 & (USB_CSRL0_RXRDY | USB_CSRL0_TXRDY)) == 0U) {
		return;
	}

	HWREGB(cfg->base + USB_TXCSRH_OFFSETX(0)) = USB_CSRH0_FLUSH;
	if (HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) & (USB_CSRL0_RXRDY | USB_CSRL0_TXRDY)) {
		HWREGB(cfg->base + USB_TXCSRH_OFFSETX(0)) = USB_CSRH0_FLUSH;
	}
}

static void sf32lb_flush_in_fifo(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	uint8_t csr = HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx));
	uint8_t flush = csr & USB_TXCSRL1_STALL;

	if ((csr & (USB_TXCSRL1_TXRDY | USB_TXCSRL1_FIFONE)) == 0U) {
		return;
	}

	flush |= USB_TXCSRL1_FLUSH;
	HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) = flush;
	if (HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) & (USB_TXCSRL1_TXRDY | USB_TXCSRL1_FIFONE)) {
		HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) = flush;
	}
}

static void sf32lb_flush_out_fifo(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	uint8_t csr = HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx));
	uint8_t flush = csr & USB_RXCSRL1_STALL;

	if ((csr & USB_RXCSRL1_RXRDY) == 0U) {
		return;
	}

	flush |= USB_RXCSRL1_FLUSH;
	HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) = flush;
	if (HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) & USB_RXCSRL1_RXRDY) {
		HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) = flush;
	}
}

static void sf32lb_flush_ep_fifo(const struct device *dev, uint8_t ep_idx, bool in)
{
	if (ep_idx == 0U) {
		sf32lb_flush_ctrl_fifo(dev);
		return;
	}

	if (in) {
		sf32lb_flush_in_fifo(dev, ep_idx);
	} else {
		sf32lb_flush_out_fifo(dev, ep_idx);
	}
}

static void sf32lb_reset_setup_queue(const struct device *dev)
{
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	struct sf32lb_setup_msg msg;
	k_spinlock_key_t key;

	key = k_spin_lock(&priv->setup_lock);
	priv->setup_epoch++;
	k_spin_unlock(&priv->setup_lock, key);

	while (k_msgq_get(&priv->setup_msgq, &msg, K_NO_WAIT) == 0) {
	}
}

static int sf32lb_queue_setup(const struct device *dev, const struct usb_setup_packet *setup)
{
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	struct sf32lb_setup_msg msg = {
		.setup = *setup,
	};
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&priv->setup_lock);
	msg.epoch = priv->setup_epoch;
	k_spin_unlock(&priv->setup_lock, key);

	ret = k_msgq_put(&priv->setup_msgq, &msg, K_NO_WAIT);
	if (ret != 0) {
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return -ENOBUFS;
	}

	ret = k_work_submit_to_queue(udc_get_work_q(), &priv->setup_work);
	if (ret < 0) {
		udc_submit_event(dev, UDC_EVT_ERROR, ret);
		return ret;
	}

	return 0;
}

static struct udc_ep_config *sf32lb_ep_cfg_from_idx(const struct device *dev, uint8_t ep_idx,
						    bool in)
{
	return udc_get_ep_cfg(dev, in ? (USB_EP_DIR_IN | ep_idx) : ep_idx);
}

static void sf32lb_reset_ep0_buffers(const struct device *dev)
{
	struct udc_sf32lb_data *priv = udc_get_private(dev);

	sf32lb_xfer_reset(&priv->out_xfer[0]);
	sf32lb_xfer_reset(&priv->in_xfer[0]);
	udc_ep_set_busy(udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT), false);
	udc_ep_set_busy(udc_get_ep_cfg(dev, USB_CONTROL_EP_IN), false);
}

static void sf32lb_reset_ep0_xfers(const struct device *dev)
{
	struct udc_sf32lb_data *priv = udc_get_private(dev);

	sf32lb_reset_ep0_buffers(dev);
	priv->ep0_state = SF32LB_EP0_IDLE;
}

static bool sf32lb_ep0_setup_expects_out_data(const struct udc_sf32lb_data *priv)
{
	return ((priv->setup.bmRequestType & USB_EP_DIR_IN) == 0U) && (priv->setup.wLength != 0U);
}

static bool sf32lb_ep0_setup_expects_in_data(const struct udc_sf32lb_data *priv)
{
	return ((priv->setup.bmRequestType & USB_EP_DIR_IN) != 0U) && (priv->setup.wLength != 0U);
}

static void sf32lb_ep0_commit_status_in(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);

	HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) = USB_CSRL0_RXRDYC | USB_CSRL0_DATAEND;
	priv->ep0_state = SF32LB_EP0_STATUS_PENDING;
}

static void sf32lb_ep0_commit_stall(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	uint8_t csr = USB_CSRL0_STALL;
	uint8_t csr0 = HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0));

	if ((csr0 & USB_CSRL0_RXRDY) != 0U || priv->ep0_state == SF32LB_EP0_SETUP_IN_PENDING ||
	    priv->ep0_state == SF32LB_EP0_STATUS_IN_PENDING) {
		csr |= USB_CSRL0_RXRDYC;
	}

	HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) = csr;
}

static void sf32lb_fail_ctrl_out(const struct device *dev, int err, bool stall)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct sf32lb_xfer *xfer = &priv->out_xfer[0];
	struct net_buf *buf = xfer->buf;

	if (stall) {
		HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) = USB_CSRL0_RXRDYC | USB_CSRL0_STALL;
	} else {
		sf32lb_flush_ep_fifo(dev, 0, false);
	}

	buf = udc_buf_get(ep_cfg);
	sf32lb_reset_ep0_xfers(dev);

	if (buf != NULL) {
		(void)udc_submit_ep_event(dev, buf, err);
	}
}

static void sf32lb_fail_out_overflow(const struct device *dev, struct udc_ep_config *ep_cfg,
				     uint8_t ep_idx, uint16_t count)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	struct sf32lb_xfer *xfer = &priv->out_xfer[ep_idx];
	struct net_buf *buf = xfer->buf;

	LOG_ERR("ep 0x%02x overflow, packet %u > remaining %u", ep_cfg->addr, count,
		xfer->remaining);

	sf32lb_flush_out_fifo(dev, ep_idx);
	HWREGH(cfg->base + MUSB_RXIE_OFFSET) &= ~BIT(ep_idx);

	buf = udc_buf_get(ep_cfg);
	sf32lb_xfer_reset(xfer);
	udc_ep_set_busy(ep_cfg, false);

	if (buf != NULL) {
		(void)udc_submit_ep_event(dev, buf, -EOVERFLOW);
	}

	udc_ep_cancel_queued(dev, ep_cfg);
}

static void sf32lb_abort_ep_xfer(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	const struct udc_sf32lb_config *cfg = dev->config;
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		sf32lb_flush_ep_fifo(dev, ep_idx, true);
		HWREGH(cfg->base + MUSB_TXIE_OFFSET) &= ~BIT(ep_idx);
		sf32lb_xfer_reset(&priv->in_xfer[ep_idx]);
	} else {
		sf32lb_flush_ep_fifo(dev, ep_idx, false);
		HWREGH(cfg->base + MUSB_RXIE_OFFSET) &= ~BIT(ep_idx);
		sf32lb_xfer_reset(&priv->out_xfer[ep_idx]);
	}

	udc_ep_set_busy(ep_cfg, false);

	if (ep_idx == 0U) {
		sf32lb_reset_ep0_xfers(dev);
	}
}

static int sf32lb_tx_start(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	struct sf32lb_xfer *xfer;
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);
	uint32_t len;
	uint8_t csr;

	xfer = &priv->in_xfer[ep_idx];
	if (!xfer->active) {
		xfer->buf = udc_buf_peek(ep_cfg);
		if (xfer->buf == NULL) {
			return -ENODATA;
		}

		xfer->data = xfer->buf->data;
		xfer->remaining = xfer->buf->len;
		xfer->zlp = (ep_idx != 0U) && udc_ep_buf_has_zlp(xfer->buf);
		xfer->active = true;
		udc_ep_set_busy(ep_cfg, true);

		if (ep_idx == 0U) {
			priv->ep0_state = SF32LB_EP0_IN_DATA;
		}
	}

	if (ep_idx == 0U) {
		if (priv->ep0_state == SF32LB_EP0_SETUP_IN_PENDING) {
			HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) = USB_CSRL0_RXRDYC;
		}

		if (HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) & USB_CSRL0_TXRDY) {
			return -EBUSY;
		}
	} else if (HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) & USB_TXCSRL1_TXRDY) {
		return -EBUSY;
	}

	len = MIN(xfer->remaining, (uint32_t)ep_cfg->mps);
	if (len != 0U) {
		sf32lb_usbdc_write_packet(cfg->base, ep_idx, xfer->data, len);
		xfer->data += len;
		xfer->remaining -= len;
	}

	if (ep_idx == 0U) {
		csr = USB_CSRL0_TXRDY;
		if (xfer->remaining == 0U) {
			csr |= USB_CSRL0_DATAEND;
		}
		HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) = csr;
		return 0;
	}

	HWREGH(cfg->base + MUSB_TXIE_OFFSET) |= BIT(ep_idx);
	HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) = USB_TXCSRL1_TXRDY;

	return 0;
}

static int sf32lb_rx_start(const struct device *dev, struct udc_ep_config *ep_cfg)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	struct sf32lb_xfer *xfer;
	struct net_buf *buf;
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);

	xfer = &priv->out_xfer[ep_idx];
	if (xfer->active) {
		return 0;
	}

	buf = udc_buf_peek(ep_cfg);
	if (buf == NULL) {
		return -ENODATA;
	}

	buf->len = 0U;
	xfer->buf = buf;
	xfer->remaining = buf->size;
	xfer->active = true;
	udc_ep_set_busy(ep_cfg, true);

	if (ep_idx == 0U) {
		priv->ep0_state = SF32LB_EP0_OUT_DATA;

		/*
		 * Control OUT data may already be sitting in FIFO by the time the
		 * USB stack has processed SETUP and queued the receive buffer.
		 * Consume it immediately instead of waiting for a new interrupt.
		 */
		if (HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) & USB_CSRL0_RXRDY) {
			uint16_t count = HWREGH(cfg->base + USB_RXCOUNT_OFFSETX(0));

			sf32lb_complete_ctrl_out(dev, count);
		}

		return 0;
	}

	HWREGH(cfg->base + MUSB_RXIE_OFFSET) |= BIT(ep_idx);

	return 0;
}

static void sf32lb_handle_setup_work(struct k_work *work)
{
	struct udc_sf32lb_data *priv = CONTAINER_OF(work, struct udc_sf32lb_data, setup_work);
	struct sf32lb_setup_msg msg;

	while (k_msgq_get(&priv->setup_msgq, &msg, K_NO_WAIT) == 0) {
		if (msg.epoch == priv->setup_epoch) {
			udc_setup_received(priv->dev, &msg.setup);
		}
	}
}

static void sf32lb_complete_ctrl_in(const struct device *dev)
{
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
	struct sf32lb_xfer *xfer = &priv->in_xfer[0];
	struct net_buf *buf;

	if (xfer->remaining != 0U) {
		(void)sf32lb_tx_start(dev, ep_cfg);
		return;
	}

	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		udc_ep_set_busy(ep_cfg, false);
		sf32lb_xfer_reset(xfer);
		priv->ep0_state = SF32LB_EP0_IDLE;
		return;
	}

	sf32lb_xfer_reset(xfer);
	udc_ep_set_busy(ep_cfg, false);
	priv->ep0_state = SF32LB_EP0_IDLE;

	(void)udc_submit_ep_event(dev, buf, 0);
}

static void sf32lb_complete_ctrl_out(const struct device *dev, uint16_t rx_count)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	struct sf32lb_xfer *xfer = &priv->out_xfer[0];
	struct net_buf *buf = xfer->buf;
	bool done;

	if (buf == NULL) {
		sf32lb_fail_ctrl_out(dev, -ENOBUFS, true);
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return;
	}

	if (rx_count > xfer->remaining) {
		sf32lb_fail_ctrl_out(dev, -EOVERFLOW, true);
		return;
	}

	if (rx_count != 0U) {
		sf32lb_usbdc_read_packet(cfg->base, 0, net_buf_tail(buf), rx_count);
		net_buf_add(buf, rx_count);
		xfer->remaining -= rx_count;
	}

	done = (rx_count < ep_cfg->mps) || (xfer->remaining == 0U);
	if (done) {
		buf = udc_buf_get(ep_cfg);

		if (buf == NULL) {
			sf32lb_reset_ep0_xfers(dev);
			sf32lb_ep0_commit_stall(dev);
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			return;
		}

		udc_ep_set_busy(ep_cfg, false);
		sf32lb_xfer_reset(xfer);
		priv->ep0_state = SF32LB_EP0_STATUS_IN_PENDING;

		(void)udc_submit_ep_event(dev, buf, 0);
		return;
	}

	HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) = USB_CSRL0_RXRDYC;
}

static void sf32lb_handle_ep0_irq(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	uint8_t csr0 = HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0));
	uint16_t count;

	if (csr0 & USB_CSRL0_STALLED) {
		HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) &= ~USB_CSRL0_STALLED;
		priv->ep0_state = SF32LB_EP0_IDLE;
		return;
	}

	if (csr0 & USB_CSRL0_SETEND) {
		HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) = USB_CSRL0_SETENDC;
		sf32lb_reset_ep0_xfers(dev);
		csr0 = HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0));
	}

	switch (priv->ep0_state) {
	case SF32LB_EP0_OUT_DATA:
		if (csr0 & USB_CSRL0_RXRDY) {
			count = HWREGH(cfg->base + USB_RXCOUNT_OFFSETX(0));
			sf32lb_complete_ctrl_out(dev, count);
		}
		break;
	case SF32LB_EP0_SETUP_IN_PENDING:
	case SF32LB_EP0_STATUS_IN_PENDING:
		break;
	case SF32LB_EP0_IN_DATA:
		sf32lb_complete_ctrl_in(dev);
		break;
	case SF32LB_EP0_STATUS_PENDING:
	case SF32LB_EP0_IDLE:
	default:
		if (priv->in_xfer[0].active) {
			struct net_buf *buf = priv->in_xfer[0].buf;
			struct udc_buf_info *bi;

			if (buf != NULL) {
				bi = udc_get_buf_info(buf);
				if (bi->status && !(csr0 & USB_CSRL0_RXRDY)) {
					sf32lb_complete_ctrl_in(dev);
					break;
				}
			}
		}

		if (!(csr0 & USB_CSRL0_RXRDY)) {
			priv->ep0_state = SF32LB_EP0_STATUS_PENDING;
			break;
		}

		count = HWREGH(cfg->base + USB_RXCOUNT_OFFSETX(0));
		if (count != sizeof(struct usb_setup_packet)) {
			/*
			 * For control OUT requests the host may send the
			 * data stage before the USB stack has enqueued the
			 * receive buffer. Keep the payload in FIFO until
			 * sf32lb_rx_start() can drain it.
			 */
			if (sf32lb_ep0_setup_expects_out_data(priv)) {
				break;
			}

			HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) =
				USB_CSRL0_RXRDYC | USB_CSRL0_STALL;
			break;
		}

		sf32lb_usbdc_read_packet(cfg->base, 0, (uint8_t *)&priv->setup, count);
		sf32lb_reset_ep0_buffers(dev);
		if (sf32lb_ep0_setup_expects_out_data(priv)) {
			priv->ep0_state = SF32LB_EP0_IDLE;
			HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) = USB_CSRL0_RXRDYC;
		} else if (sf32lb_ep0_setup_expects_in_data(priv)) {
			priv->ep0_state = SF32LB_EP0_SETUP_IN_PENDING;
		} else {
			priv->ep0_state = SF32LB_EP0_STATUS_IN_PENDING;
		}

		if (sf32lb_queue_setup(dev, &priv->setup) != 0) {
			sf32lb_reset_ep0_xfers(dev);
			sf32lb_ep0_commit_stall(dev);
			break;
		}
		break;
	}
}

static void sf32lb_complete_in(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg = sf32lb_ep_cfg_from_idx(dev, ep_idx, true);
	struct sf32lb_xfer *xfer;
	struct net_buf *buf;

	if (ep_cfg == NULL) {
		return;
	}

	if (ep_idx == 0U) {
		sf32lb_handle_ep0_irq(dev);
		return;
	}

	xfer = &priv->in_xfer[ep_idx];
	if (!xfer->active) {
		return;
	}

	if (HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) & USB_TXCSRL1_UNDRN) {
		HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) &= ~USB_TXCSRL1_UNDRN;
	}

	if (xfer->remaining != 0U) {
		(void)sf32lb_tx_start(dev, ep_cfg);
		return;
	}

	if (xfer->zlp) {
		xfer->zlp = false;
		(void)sf32lb_tx_start(dev, ep_cfg);
		return;
	}

	buf = udc_buf_get(ep_cfg);
	sf32lb_xfer_reset(xfer);
	udc_ep_set_busy(ep_cfg, false);
	HWREGH(cfg->base + MUSB_TXIE_OFFSET) &= ~BIT(ep_idx);

	if (buf == NULL) {
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return;
	}

	(void)udc_submit_ep_event(dev, buf, 0);

	if (!ep_cfg->stat.halted && udc_buf_peek(ep_cfg) != NULL) {
		(void)sf32lb_tx_start(dev, ep_cfg);
	}
}

static void sf32lb_complete_out(const struct device *dev, uint8_t ep_idx)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg = sf32lb_ep_cfg_from_idx(dev, ep_idx, false);
	struct sf32lb_xfer *xfer;
	struct net_buf *buf;
	uint16_t count;
	bool done;

	if (ep_cfg == NULL) {
		return;
	}

	xfer = &priv->out_xfer[ep_idx];
	if (!xfer->active) {
		HWREGH(cfg->base + MUSB_RXIE_OFFSET) &= ~BIT(ep_idx);
		return;
	}

	if (!(HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) & USB_RXCSRL1_RXRDY)) {
		return;
	}

	buf = xfer->buf;
	if (buf == NULL) {
		sf32lb_flush_out_fifo(dev, ep_idx);
		HWREGH(cfg->base + MUSB_RXIE_OFFSET) &= ~BIT(ep_idx);
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		sf32lb_xfer_reset(xfer);
		udc_ep_set_busy(ep_cfg, false);
		return;
	}

	count = HWREGH(cfg->base + USB_RXCOUNT_OFFSETX(ep_idx));
	if (count > xfer->remaining) {
		sf32lb_fail_out_overflow(dev, ep_cfg, ep_idx, count);
		return;
	}

	if (count != 0U) {
		sf32lb_usbdc_read_packet(cfg->base, ep_idx, net_buf_tail(buf), count);
		net_buf_add(buf, count);
		xfer->remaining -= count;
	}

	HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) &= ~USB_RXCSRL1_RXRDY;

	done = (count < ep_cfg->mps) || (xfer->remaining == 0U);
	if (!done) {
		return;
	}

	buf = udc_buf_get(ep_cfg);
	sf32lb_xfer_reset(xfer);
	udc_ep_set_busy(ep_cfg, false);
	HWREGH(cfg->base + MUSB_RXIE_OFFSET) &= ~BIT(ep_idx);

	if (buf == NULL) {
		udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		return;
	}

	(void)udc_submit_ep_event(dev, buf, 0);

	if (!ep_cfg->stat.halted && udc_buf_peek(ep_cfg) != NULL) {
		(void)sf32lb_rx_start(dev, ep_cfg);
	}
}

static void sf32lb_handle_bus_reset(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_data *data = dev->data;
	struct udc_sf32lb_data *priv = udc_get_private(dev);

	for (uint8_t ep_idx = 0U; ep_idx < SF32LB_USBDC_MAX_ENDPOINTS; ep_idx++) {
		sf32lb_flush_ep_fifo(dev, ep_idx, true);
		if (ep_idx != 0U) {
			sf32lb_flush_ep_fifo(dev, ep_idx, false);
		}
	}

	sf32lb_reset_ep0_xfers(dev);
	sf32lb_reset_setup_queue(dev);
	memset(data->setup, 0, sizeof(data->setup));
	data->setup_pending = false;
	data->setup_valid = false;
	memset(&priv->setup, 0, sizeof(priv->setup));
	memset(priv->in_xfer, 0, sizeof(priv->in_xfer));
	memset(priv->out_xfer, 0, sizeof(priv->out_xfer));
	priv->ep0_state = SF32LB_EP0_IDLE;

	for (uint8_t i = 0U; i < cfg->eps_in_cnt; i++) {
		udc_ep_set_busy(&cfg->ep_cfg_in[i], false);
		cfg->ep_cfg_in[i].stat.halted = false;
	}

	for (uint8_t i = 0U; i < cfg->eps_out_cnt; i++) {
		udc_ep_set_busy(&cfg->ep_cfg_out[i], false);
		cfg->ep_cfg_out[i].stat.halted = false;
	}

	HWREGB(cfg->base + MUSB_FADDR_OFFSET) = 0U;
	HWREGH(cfg->base + MUSB_TXIE_OFFSET) = USB_TXIE_EP0;
	HWREGH(cfg->base + MUSB_RXIE_OFFSET) = 0U;

	udc_submit_event(dev, UDC_EVT_RESET, 0);
}

static void sf32lb_udc_isr(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	uint8_t is;
	uint16_t txis;
	uint16_t rxis;

	is = HWREGB(cfg->base + MUSB_IS_OFFSET);
	txis = HWREGH(cfg->base + MUSB_TXIS_OFFSET);
	rxis = HWREGH(cfg->base + MUSB_RXIS_OFFSET);

	HWREGB(cfg->base + MUSB_IS_OFFSET) = is;

	if (is & USB_INTR_RESET) {
		sf32lb_handle_bus_reset(dev);
		return;
	}

#ifdef CONFIG_UDC_ENABLE_SOF
	if (is & USB_INTR_SOF) {
		udc_submit_sof_event(dev);
	}
#endif

	if (is & USB_INTR_SUSPEND) {
		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	}

	if (is & USB_INTR_RESUME) {
		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}

	txis &= HWREGH(cfg->base + MUSB_TXIE_OFFSET);
	if (txis & USB_TXIE_EP0) {
		HWREGH(cfg->base + MUSB_TXIS_OFFSET) = USB_TXIE_EP0;
		sf32lb_handle_ep0_irq(dev);
		txis &= ~USB_TXIE_EP0;
	}

	for (uint8_t ep_idx = 1U; ep_idx < SF32LB_USBDC_MAX_ENDPOINTS; ep_idx++) {
		if ((txis & BIT(ep_idx)) == 0U) {
			continue;
		}

		HWREGH(cfg->base + MUSB_TXIS_OFFSET) = BIT(ep_idx);
		sf32lb_complete_in(dev, ep_idx);
	}

	rxis &= HWREGH(cfg->base + MUSB_RXIE_OFFSET);
	for (uint8_t ep_idx = 1U; ep_idx < SF32LB_USBDC_MAX_ENDPOINTS; ep_idx++) {
		if ((rxis & BIT(ep_idx)) == 0U) {
			continue;
		}

		HWREGH(cfg->base + MUSB_RXIS_OFFSET) = BIT(ep_idx);
		sf32lb_complete_out(dev, ep_idx);
	}
}

static enum udc_bus_speed sf32lb_udc_device_speed(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;

	return sf32lb_usbdc_device_speed(cfg->base);
}

static int sf32lb_udc_ep_enqueue(const struct device *dev, struct udc_ep_config *const ep_cfg,
				 struct net_buf *const buf)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	const struct udc_buf_info *bi = udc_get_buf_info(buf);
	unsigned int key;
	int ret = 0;

	udc_buf_put(ep_cfg, buf);
	if (ep_cfg->stat.halted) {
		return 0;
	}

	if (ep_cfg->addr == USB_CONTROL_EP_OUT && bi->setup) {
		return 0;
	}

	if (udc_ep_is_busy(ep_cfg)) {
		return 0;
	}

	key = irq_lock();
	if (!udc_ep_is_busy(ep_cfg)) {
		if (ep_cfg->addr == USB_CONTROL_EP_IN && bi->status && buf->len == 0U) {
			struct sf32lb_xfer *xfer = &priv->in_xfer[0];

			xfer->buf = buf;
			xfer->data = buf->data;
			xfer->remaining = 0U;
			xfer->zlp = false;
			xfer->active = true;
			udc_ep_set_busy(ep_cfg, true);

			if (priv->ep0_state == SF32LB_EP0_STATUS_IN_PENDING) {
				sf32lb_ep0_commit_status_in(dev);
			} else if (priv->ep0_state == SF32LB_EP0_STATUS_PENDING &&
				   !(HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) & USB_CSRL0_RXRDY)) {
				sf32lb_complete_ctrl_in(dev);
			}
		} else if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
			ret = sf32lb_tx_start(dev, ep_cfg);
		} else {
			ret = sf32lb_rx_start(dev, ep_cfg);
		}
	}
	irq_unlock(key);

	return (ret == -ENODATA) ? 0 : ret;
}

static int sf32lb_udc_ep_dequeue(const struct device *dev, struct udc_ep_config *const cfg_ep)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, cfg_ep->addr);
	unsigned int key;

	if (ep_cfg == NULL) {
		return -EINVAL;
	}

	key = irq_lock();
	sf32lb_abort_ep_xfer(dev, ep_cfg);
	udc_ep_cancel_queued(dev, ep_cfg);
	irq_unlock(key);

	return 0;
}

static int sf32lb_udc_ep_set_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);

	if (ep_idx == 0U) {
		sf32lb_ep0_commit_stall(dev);
	} else if (USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
		HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) |= USB_RXCSRL1_STALL;
	} else {
		HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) |= USB_TXCSRL1_STALL;
	}

	if (ep_idx != 0U) {
		ep_cfg->stat.halted = true;
	}

	return 0;
}

static int sf32lb_udc_ep_clear_halt(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);

	if (USB_EP_DIR_IS_OUT(ep_cfg->addr)) {
		if (ep_idx == 0U) {
			HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) &= ~USB_CSRL0_STALLED;
		} else {
			HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) &=
				~(USB_RXCSRL1_STALL | USB_RXCSRL1_STALLED);
			HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) |= USB_RXCSRL1_CLRDT;
		}
	} else {
		if (ep_idx == 0U) {
			HWREGB(cfg->base + USB_TXCSRL_OFFSETX(0)) &= ~USB_CSRL0_STALLED;
		} else {
			HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) &=
				~(USB_TXCSRL1_STALL | USB_TXCSRL1_STALLED);
			HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) |= USB_TXCSRL1_CLRDT;
		}
	}

	ep_cfg->stat.halted = false;

	if (!udc_ep_is_busy(ep_cfg) && udc_buf_peek(ep_cfg) != NULL) {
		if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
			(void)sf32lb_tx_start(dev, ep_cfg);
		} else {
			(void)sf32lb_rx_start(dev, ep_cfg);
		}
	}

	return 0;
}

static int sf32lb_udc_ep_enable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);
	uint8_t ep_type = ep_cfg->attributes & USB_EP_TRANSFER_TYPE_MASK;

	if (ep_idx == 0U) {
		udc_ep_set_busy(ep_cfg, false);
		ep_cfg->stat.halted = false;
		return 0;
	}

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		if (HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) & USB_TXCSRL1_TXRDY) {
			HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) =
				USB_TXCSRL1_CLRDT | USB_TXCSRL1_FLUSH;
		} else {
			HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) = USB_TXCSRL1_CLRDT;
		}

		HWREGH(cfg->base + USB_TXMAXP_OFFSETX(ep_idx)) = ep_cfg->mps;
		HWREGB(cfg->base + USB_TXCSRH_OFFSETX(ep_idx)) =
			USB_TXCSRH1_MODE | ((ep_type == USB_EP_TYPE_ISO) ? USB_TXCSRH1_ISO : 0U);
	} else {
		if (HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) & USB_RXCSRL1_RXRDY) {
			HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) =
				USB_RXCSRL1_CLRDT | USB_RXCSRL1_FLUSH;
		} else {
			HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) = USB_RXCSRL1_CLRDT;
		}

		HWREGH(cfg->base + USB_RXMAXP_OFFSETX(ep_idx)) = ep_cfg->mps;
		HWREGB(cfg->base + USB_RXCSRH_OFFSETX(ep_idx)) =
			(ep_type == USB_EP_TYPE_ISO) ? USB_RXCSRH1_ISO : 0U;
	}

	udc_ep_set_busy(ep_cfg, false);
	ep_cfg->stat.halted = false;

	return 0;
}

static int sf32lb_udc_ep_disable(const struct device *dev, struct udc_ep_config *const ep_cfg)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	uint8_t ep_idx = USB_EP_GET_IDX(ep_cfg->addr);

	sf32lb_abort_ep_xfer(dev, ep_cfg);

	if (ep_idx == 0U) {
		return 0;
	}

	if (USB_EP_DIR_IS_IN(ep_cfg->addr)) {
		HWREGH(cfg->base + MUSB_TXIE_OFFSET) &= ~BIT(ep_idx);
		HWREGH(cfg->base + USB_TXMAXP_OFFSETX(ep_idx)) = 0U;
		HWREGB(cfg->base + USB_TXCSRL_OFFSETX(ep_idx)) = 0U;
		HWREGB(cfg->base + USB_TXCSRH_OFFSETX(ep_idx)) = 0U;
	} else {
		HWREGH(cfg->base + MUSB_RXIE_OFFSET) &= ~BIT(ep_idx);
		HWREGH(cfg->base + USB_RXMAXP_OFFSETX(ep_idx)) = 0U;
		HWREGB(cfg->base + USB_RXCSRL_OFFSETX(ep_idx)) = 0U;
		HWREGB(cfg->base + USB_RXCSRH_OFFSETX(ep_idx)) = 0U;
	}

	return 0;
}

static int sf32lb_udc_host_wakeup(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;

	HWREGB(cfg->base + MUSB_POWER_OFFSET) |= USB_POWER_RESUME;
	k_busy_wait(10000);
	HWREGB(cfg->base + MUSB_POWER_OFFSET) &= ~USB_POWER_RESUME;

	return 0;
}

static int sf32lb_udc_set_address(const struct device *dev, const uint8_t addr)
{
	const struct udc_sf32lb_config *cfg = dev->config;

	HWREGB(cfg->base + MUSB_FADDR_OFFSET) = addr;

	return 0;
}

static int sf32lb_udc_enable(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;

	HWREGB(cfg->base + MUSB_POWER_OFFSET) |= USB_POWER_SOFTCONN;

	return 0;
}

static int sf32lb_udc_disable(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;

	HWREGB(cfg->base + MUSB_POWER_OFFSET) &= ~USB_POWER_SOFTCONN;

	return 0;
}

static int sf32lb_udc_init(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;
	int ret;

	HWREGB(cfg->base + MUSB_POWER_OFFSET) &= ~USB_POWER_HSENAB;
	HWREGB(cfg->base + MUSB_POWER_OFFSET) &= ~USB_POWER_SOFTCONN;
	HWREGB(cfg->base + MUSB_FADDR_OFFSET) = 0U;
	HWREGB(cfg->base + MUSB_DEVCTL_OFFSET) |= USB_DEVCTL_SESSION;

	HWREGB(cfg->base + MUSB_IE_OFFSET) = USB_IE_RESET | USB_IE_SUSPND | USB_IE_RESUME;
#ifdef CONFIG_UDC_ENABLE_SOF
	HWREGB(cfg->base + MUSB_IE_OFFSET) |= USB_IE_SOF;
#endif
	HWREGH(cfg->base + MUSB_TXIE_OFFSET) = USB_TXIE_EP0;
	HWREGH(cfg->base + MUSB_RXIE_OFFSET) = 0U;

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT, USB_EP_TYPE_CONTROL,
				     SF32LB_USBDC_EP_MPS_FS, 0);
	if (ret != 0) {
		return ret;
	}

	ret = udc_ep_enable_internal(dev, USB_CONTROL_EP_IN, USB_EP_TYPE_CONTROL,
				     SF32LB_USBDC_EP_MPS_FS, 0);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int sf32lb_udc_shutdown(const struct device *dev)
{
	const struct udc_sf32lb_config *cfg = dev->config;

	sf32lb_reset_setup_queue(dev);
	HWREGB(cfg->base + MUSB_POWER_OFFSET) &= ~USB_POWER_SOFTCONN;
	HWREGB(cfg->base + MUSB_IE_OFFSET) = 0U;
	HWREGH(cfg->base + MUSB_TXIE_OFFSET) = 0U;
	HWREGH(cfg->base + MUSB_RXIE_OFFSET) = 0U;

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT) != 0) {
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN) != 0) {
		return -EIO;
	}

	return 0;
}

static void sf32lb_udc_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void sf32lb_udc_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static const struct udc_api sf32lb_udc_api = {
	.device_speed = sf32lb_udc_device_speed,
	.ep_enqueue = sf32lb_udc_ep_enqueue,
	.ep_dequeue = sf32lb_udc_ep_dequeue,
	.ep_set_halt = sf32lb_udc_ep_set_halt,
	.ep_clear_halt = sf32lb_udc_ep_clear_halt,
	.ep_enable = sf32lb_udc_ep_enable,
	.ep_disable = sf32lb_udc_ep_disable,
	.host_wakeup = sf32lb_udc_host_wakeup,
	.set_address = sf32lb_udc_set_address,
	.enable = sf32lb_udc_enable,
	.disable = sf32lb_udc_disable,
	.init = sf32lb_udc_init,
	.shutdown = sf32lb_udc_shutdown,
	.lock = sf32lb_udc_lock,
	.unlock = sf32lb_udc_unlock,
};

static int udc_sf32lb_driver_init(const struct device *dev)
{
	const struct udc_sf32lb_config *config = dev->config;
	struct udc_data *data = dev->data;
	struct udc_sf32lb_data *priv = udc_get_private(dev);
	uint16_t mps = SF32LB_USBDC_EP_MPS_FS;
	int ret;

	memset(priv, 0, sizeof(*priv));
	priv->ep0_state = SF32LB_EP0_IDLE;
	priv->dev = dev;
	k_work_init(&priv->setup_work, sf32lb_handle_setup_work);
	k_msgq_init(&priv->setup_msgq, priv->setup_msgq_buffer, sizeof(struct sf32lb_setup_msg),
		    SF32LB_SETUP_QUEUE_DEPTH);
	priv->setup_epoch = 1U;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	ret = sf32lb_clock_control_on_dt(&config->clock);
	if (ret < 0) {
		return ret;
	}

	sf32lb_usbdc_phy_init(config->base, config->cfg);

	config->irq_config_func();

	data->caps.addr_before_status = false;
	data->caps.out_ack = true;
	data->caps.mps0 = UDC_MPS0_64;
	data->caps.rwup = true;

	if (config->speed_idx > UDC_BUS_SPEED_FS) {
		LOG_WRN("maximum-speed > full-speed is not supported yet");
	}

	for (uint8_t i = 0U; i < config->eps_out_cnt; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0U) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = SF32LB_USBDC_EP_MPS_FS;
			config->ep_cfg_out[i].addr = USB_CONTROL_EP_OUT;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = mps;
			/*
			 * SF32LB52 endpoint topology is asymmetric:
			 *   - EP0 and EP1 are bidirectional
			 *   - EP2-EP4 are OUT only
			 *   - EP5-EP7 are IN only
			 *
			 * EP0 and EP1 share a single bidirectional resource and
			 * therefore cannot be used for simultaneous IN and OUT
			 * transfers.
			 *
			 * Expose only the dedicated OUT-capable endpoints here to
			 * avoid allocating endpoint numbers the hardware cannot
			 * sustain for concurrent traffic.
			 */
			config->ep_cfg_out[i].addr = i + 1U;
		}

		ret = udc_register_ep(dev, &config->ep_cfg_out[i]);
		if (ret != 0) {
			return ret;
		}
	}

	for (uint8_t i = 0U; i < config->eps_in_cnt; i++) {
		config->ep_cfg_in[i].caps.in = 1;
		if (i == 0U) {
			config->ep_cfg_in[i].caps.control = 1;
			config->ep_cfg_in[i].caps.mps = SF32LB_USBDC_EP_MPS_FS;
			config->ep_cfg_in[i].addr = USB_CONTROL_EP_IN;
		} else {
			config->ep_cfg_in[i].caps.bulk = 1;
			config->ep_cfg_in[i].caps.interrupt = 1;
			config->ep_cfg_in[i].caps.iso = 1;
			config->ep_cfg_in[i].caps.mps = mps;
			/* Keep EP1 for the shared bidirectional slot, then use the
			 * dedicated IN-only endpoints EP5-EP7.
			 */
			config->ep_cfg_in[i].addr = (i == 1U) ? 0x81U : (USB_EP_DIR_IN | (i + 3U));
		}

		ret = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

#define UDC_SF32LB_DEFINE(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct udc_ep_config ep_cfg_out_##inst[DT_INST_PROP(inst, num_out_endpoints)];      \
	static struct udc_ep_config ep_cfg_in_##inst[DT_INST_PROP(inst, num_in_endpoints)];        \
	static void udc_sf32lb_irq_config_func_##inst(void)                                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), sf32lb_udc_isr,       \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
	static struct udc_sf32lb_data udc_sf32lb_data_##inst;                                      \
	static struct udc_data udc_data_##inst = {                                                 \
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##inst.mutex),                               \
		.priv = &udc_sf32lb_data_##inst,                                                   \
	};                                                                                         \
	static const struct udc_sf32lb_config udc_sf32lb_config_##inst = {                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.cfg = DT_REG_ADDR(DT_INST_PHANDLE(inst, sifli_cfg)),                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(inst),                                      \
		.irq_config_func = udc_sf32lb_irq_config_func_##inst,                              \
		.eps_in_cnt = DT_INST_PROP(inst, num_in_endpoints),                                \
		.eps_out_cnt = DT_INST_PROP(inst, num_out_endpoints),                              \
		.ep_cfg_in = ep_cfg_in_##inst,                                                     \
		.ep_cfg_out = ep_cfg_out_##inst,                                                   \
		.speed_idx = DT_ENUM_IDX_OR(DT_DRV_INST(inst), maximum_speed, UDC_BUS_SPEED_FS),   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, udc_sf32lb_driver_init, NULL, &udc_data_##inst,                \
			      &udc_sf32lb_config_##inst, POST_KERNEL,                              \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &sf32lb_udc_api);

DT_INST_FOREACH_STATUS_OKAY(UDC_SF32LB_DEFINE)
