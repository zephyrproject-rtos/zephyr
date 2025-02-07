/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Driver for the USBFSOTG device controller which can be found on
 * devices like Kinetis K64F.
 */
#define DT_DRV_COMPAT nxp_kinetis_usbd

#include <soc.h>
#include <string.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/usb/udc.h>

#include "udc_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbfsotg, CONFIG_UDC_DRIVER_LOG_LEVEL);

#define USBFSOTG_BD_OWN		BIT(5)
#define USBFSOTG_BD_DATA1	BIT(4)
#define USBFSOTG_BD_KEEP	BIT(3)
#define USBFSOTG_BD_NINC	BIT(2)
#define USBFSOTG_BD_DTS		BIT(1)
#define USBFSOTG_BD_STALL	BIT(0)

#define USBFSOTG_SETUP_TOKEN	0x0D
#define USBFSOTG_IN_TOKEN	0x09
#define USBFSOTG_OUT_TOKEN	0x01

#define USBFSOTG_PERID		0x04
#define USBFSOTG_REV		0x33

/*
 * There is no real advantage to change control endpoint size
 * but we can use it for testing UDC driver API and higher layers.
 */
#define USBFSOTG_MPS0		UDC_MPS0_64
#define USBFSOTG_EP0_SIZE	64

/*
 * Buffer Descriptor (BD) entry provides endpoint buffer control
 * information for USBFSOTG controller. Every endpoint direction requires
 * two BD entries.
 */
struct usbfsotg_bd {
	union {
		uint32_t bd_fields;

		struct {
			uint32_t reserved_1_0 : 2;
			uint32_t tok_pid : 4;
			uint32_t data1 : 1;
			uint32_t own : 1;
			uint32_t reserved_15_8 : 8;
			uint32_t bc : 16;
		} get __packed;

		struct {
			uint32_t reserved_1_0 : 2;
			uint32_t bd_ctrl : 6;
			uint32_t reserved_15_8 : 8;
			uint32_t bc : 16;
		} set __packed;

	} __packed;
	uint32_t   buf_addr;
} __packed;

struct usbfsotg_config {
	USB_Type *base;
	/*
	 * Pointer to Buffer Descriptor Table for the endpoints
	 * buffer management. The driver configuration with 16 fully
	 * bidirectional endpoints would require four BD entries
	 * per endpoint and 512 bytes of memory.
	 */
	struct usbfsotg_bd *bdt;
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
};

enum usbfsotg_event_type {
	/* Trigger next transfer, must not be used for control OUT */
	USBFSOTG_EVT_XFER,
	/* Setup packet received */
	USBFSOTG_EVT_SETUP,
	/* OUT transaction for specific endpoint is finished */
	USBFSOTG_EVT_DOUT,
	/* IN transaction for specific endpoint is finished */
	USBFSOTG_EVT_DIN,
	/* Workaround for clear halt in ISR */
	USBFSOTG_EVT_CLEAR_HALT,
};

/* Structure for driver's endpoint events */
struct usbfsotg_ep_event {
	sys_snode_t node;
	const struct device *dev;
	enum usbfsotg_event_type event;
	uint8_t ep;
};

K_MEM_SLAB_DEFINE(usbfsotg_ee_slab, sizeof(struct usbfsotg_ep_event),
		  CONFIG_UDC_KINETIS_EVENT_COUNT, sizeof(void *));

struct usbfsotg_data {
	struct k_work work;
	struct k_fifo fifo;
	/*
	 * Buffer pointers and busy flags used only for control OUT
	 * to map the buffers to BDs when both are occupied
	 */
	struct net_buf *out_buf[2];
	bool busy[2];
};

static int usbfsotg_ep_clear_halt(const struct device *dev,
				  struct udc_ep_config *const cfg);

/* Get buffer descriptor (BD) based on endpoint address */
static struct usbfsotg_bd *usbfsotg_get_ebd(const struct device *const dev,
					    struct udc_ep_config *const cfg,
					    const bool opposite)
{
	const struct usbfsotg_config *config = dev->config;
	uint8_t bd_idx;

	bd_idx = USB_EP_GET_IDX(cfg->addr) * 4U + (cfg->stat.odd ^ opposite);
	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		bd_idx += 2U;
	}

	return &config->bdt[bd_idx];
}

static bool usbfsotg_bd_is_busy(const struct usbfsotg_bd *const bd)
{
	/* Do not use it for control OUT endpoint */
	return bd->get.own;
}

static void usbfsotg_bd_set_ctrl(struct usbfsotg_bd *const bd,
				 const size_t bc,
				 uint8_t *const data,
				 const bool data1)
{
	bd->set.bc = bc;
	bd->buf_addr = POINTER_TO_UINT(data);

	if (data1) {
		bd->set.bd_ctrl = USBFSOTG_BD_OWN | USBFSOTG_BD_DATA1 |
				  USBFSOTG_BD_DTS;
	} else {
		bd->set.bd_ctrl = USBFSOTG_BD_OWN | USBFSOTG_BD_DTS;
	}

}

/* Resume TX token processing, see USBx_CTL field descriptions */
static ALWAYS_INLINE void usbfsotg_resume_tx(const struct device *dev)
{
	const struct usbfsotg_config *config = dev->config;
	USB_Type *base = config->base;

	base->CTL &= ~USB_CTL_TXSUSPENDTOKENBUSY_MASK;
}

static int usbfsotg_xfer_continue(const struct device *dev,
				  struct udc_ep_config *const cfg,
				  struct net_buf *const buf)
{
	const struct usbfsotg_config *config = dev->config;
	USB_Type *base = config->base;
	struct usbfsotg_bd *bd;
	uint8_t *data_ptr;
	size_t len;

	bd = usbfsotg_get_ebd(dev, cfg, false);
	if (unlikely(usbfsotg_bd_is_busy(bd))) {
		LOG_ERR("ep 0x%02x buf busy", cfg->addr);
		__ASSERT_NO_MSG(false);
		return -EBUSY;
	}

	if (USB_EP_DIR_IS_OUT(cfg->addr)) {
		len = MIN(net_buf_tailroom(buf), udc_mps_ep_size(cfg));
		data_ptr = net_buf_tail(buf);
	} else {
		len = MIN(buf->len, udc_mps_ep_size(cfg));
		data_ptr = buf->data;
	}

	usbfsotg_bd_set_ctrl(bd, len, data_ptr, cfg->stat.data1);

	if (USB_EP_GET_IDX(cfg->addr) == 0U) {
		usbfsotg_resume_tx(dev);
	}

	LOG_DBG("xfer %p, bd %p, ENDPT 0x%x, bd field 0x%02x",
		buf, bd, base->ENDPOINT[USB_EP_GET_IDX(cfg->addr)].ENDPT,
		bd->bd_fields);

	return 0;
}

/* Initiate a new transfer, must not be used for control endpoint OUT */
static int usbfsotg_xfer_next(const struct device *dev,
			      struct udc_ep_config *const cfg)
{
	struct net_buf *buf;

	buf = udc_buf_peek(dev, cfg->addr);
	if (buf == NULL) {
		return -ENODATA;
	}

	return usbfsotg_xfer_continue(dev, cfg, buf);
}

static inline int usbfsotg_ctrl_feed_start(const struct device *dev,
					   struct net_buf *const buf)
{
	struct usbfsotg_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg;
	struct usbfsotg_bd *bd;
	size_t length;

	cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	if (priv->busy[cfg->stat.odd]) {
		return -EBUSY;
	}

	bd = usbfsotg_get_ebd(dev, cfg, false);
	length = MIN(net_buf_tailroom(buf), udc_mps_ep_size(cfg));

	priv->out_buf[cfg->stat.odd] = buf;
	priv->busy[cfg->stat.odd] = true;
	usbfsotg_bd_set_ctrl(bd, length, net_buf_tail(buf), cfg->stat.data1);
	LOG_DBG("ep0 %p|odd: %u|d: %u", buf, cfg->stat.odd, cfg->stat.data1);

	return 0;
}

static inline int usbfsotg_ctrl_feed_start_next(const struct device *dev,
						struct net_buf *const buf)
{
	struct usbfsotg_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg;
	struct usbfsotg_bd *bd;
	size_t length;

	cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
	if (priv->busy[!cfg->stat.odd]) {
		return -EBUSY;
	}

	bd = usbfsotg_get_ebd(dev, cfg, true);
	length = MIN(net_buf_tailroom(buf), udc_mps_ep_size(cfg));

	priv->out_buf[!cfg->stat.odd] = buf;
	priv->busy[!cfg->stat.odd] = true;
	usbfsotg_bd_set_ctrl(bd, length, net_buf_tail(buf), cfg->stat.data1);
	LOG_DBG("ep0 %p|odd: %u|d: %u (n)", buf, cfg->stat.odd, cfg->stat.data1);

	return 0;
}

/*
 * Allocate buffer and initiate a new control OUT transfer,
 * use successive buffer descriptor when next is true.
 */
static int usbfsotg_ctrl_feed_dout(const struct device *dev,
				   const size_t length,
				   const bool next,
				   const bool resume_tx)
{
	struct net_buf *buf;
	int ret;

	buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	if (next) {
		ret = usbfsotg_ctrl_feed_start_next(dev, buf);
	} else {
		ret = usbfsotg_ctrl_feed_start(dev, buf);
	}

	if (ret) {
		net_buf_unref(buf);
		return ret;
	}

	if (resume_tx) {
		usbfsotg_resume_tx(dev);
	}

	return 0;
}

static inline int work_handler_setup(const struct device *dev)
{
	struct net_buf *buf;
	int err;

	buf = udc_buf_get(dev, USB_CONTROL_EP_OUT);
	if (buf == NULL) {
		return -ENODATA;
	}

	/* Update to next stage of control transfer */
	udc_ctrl_update_stage(dev, buf);

	if (udc_ctrl_stage_is_data_out(dev)) {
		/*  Allocate and feed buffer for data OUT stage */
		LOG_DBG("s:%p|feed for -out-", buf);
		err = usbfsotg_ctrl_feed_dout(dev, udc_data_stage_length(buf),
					      false, true);
		if (err == -ENOMEM) {
			err = udc_submit_ep_event(dev, buf, err);
		}
	} else if (udc_ctrl_stage_is_data_in(dev)) {
		/*
		 * Here we have to feed both descriptor tables so that
		 * no setup packets are lost in case of successive
		 * status OUT stage and next setup.
		 */
		LOG_DBG("s:%p|feed for -in-status >setup", buf);
		err = usbfsotg_ctrl_feed_dout(dev, 8U, false, false);
		if (err == 0) {
			err = usbfsotg_ctrl_feed_dout(dev, 8U, true, true);
		}

		/* Finally alloc buffer for IN and submit to upper layer */
		if (err == 0) {
			err = udc_ctrl_submit_s_in_status(dev);
		}
	} else {
		LOG_DBG("s:%p|feed >setup", buf);
		/*
		 * For all other cases we feed with a buffer
		 * large enough for setup packet.
		 */
		err = usbfsotg_ctrl_feed_dout(dev, 8U, false, true);
		if (err == 0) {
			err = udc_ctrl_submit_s_status(dev);
		}
	}

	return err;
}

static inline int work_handler_out(const struct device *dev,
				   const uint8_t ep)
{
	struct net_buf *buf;
	int err = 0;

	buf = udc_buf_get(dev, ep);
	if (buf == NULL) {
		return -ENODATA;
	}

	if (ep == USB_CONTROL_EP_OUT) {
		if (udc_ctrl_stage_is_status_out(dev)) {
			/* s-in-status finished, next bd is already fed */
			LOG_DBG("dout:%p|no feed", buf);
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		} else {
			/*
			 * For all other cases we feed with a buffer
			 * large enough for setup packet.
			 */
			LOG_DBG("dout:%p|feed >setup", buf);
			err = usbfsotg_ctrl_feed_dout(dev, 8U, false, false);
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

static inline int work_handler_in(const struct device *dev,
				  const uint8_t ep)
{
	struct net_buf *buf;

	buf = udc_buf_get(dev, ep);
	if (buf == NULL) {
		return -ENODATA;
	}

	if (ep == USB_CONTROL_EP_IN) {
		if (udc_ctrl_stage_is_status_in(dev) ||
		    udc_ctrl_stage_is_no_data(dev)) {
			/* Status stage finished, notify upper layer */
			udc_ctrl_submit_status(dev, buf);
		}

		/* Update to next stage of control transfer */
		udc_ctrl_update_stage(dev, buf);

		if (udc_ctrl_stage_is_status_out(dev)) {
			/*
			 * IN transfer finished, release buffer,
			 * control OUT buffer should be already fed.
			 */
			net_buf_unref(buf);
		}

		return 0;
	}

	return udc_submit_ep_event(dev, buf, 0);
}

static void usbfsotg_event_submit(const struct device *dev,
				 const uint8_t ep,
				 const enum usbfsotg_event_type event)
{
	struct usbfsotg_data *priv = udc_get_private(dev);
	struct usbfsotg_ep_event *ev;
	int ret;

	ret = k_mem_slab_alloc(&usbfsotg_ee_slab, (void **)&ev, K_NO_WAIT);
	if (ret) {
		udc_submit_event(dev, UDC_EVT_ERROR, ret);
		LOG_ERR("Failed to allocate slab");
		return;
	}

	ev->dev = dev;
	ev->ep = ep;
	ev->event = event;
	k_fifo_put(&priv->fifo, ev);
	k_work_submit_to_queue(udc_get_work_q(), &priv->work);
}

static void xfer_work_handler(struct k_work *item)
{
	struct usbfsotg_ep_event *ev;
	struct usbfsotg_data *priv;

	priv = CONTAINER_OF(item, struct usbfsotg_data, work);
	while ((ev = k_fifo_get(&priv->fifo, K_NO_WAIT)) != NULL) {
		struct udc_ep_config *ep_cfg;
		int err = 0;

		LOG_DBG("dev %p, ep 0x%02x, event %u",
			ev->dev, ev->ep, ev->event);
		ep_cfg = udc_get_ep_cfg(ev->dev, ev->ep);
		if (unlikely(ep_cfg == NULL)) {
			udc_submit_event(ev->dev, UDC_EVT_ERROR, -ENODATA);
			goto xfer_work_error;
		}

		switch (ev->event) {
		case USBFSOTG_EVT_SETUP:
			err = work_handler_setup(ev->dev);
			break;
		case USBFSOTG_EVT_DOUT:
			err = work_handler_out(ev->dev, ev->ep);
			udc_ep_set_busy(ev->dev, ev->ep, false);
			break;
		case USBFSOTG_EVT_DIN:
			err = work_handler_in(ev->dev, ev->ep);
			udc_ep_set_busy(ev->dev, ev->ep, false);
			break;
		case USBFSOTG_EVT_CLEAR_HALT:
			err = usbfsotg_ep_clear_halt(ev->dev, ep_cfg);
		case USBFSOTG_EVT_XFER:
		default:
			break;
		}

		if (unlikely(err)) {
			udc_submit_event(ev->dev, UDC_EVT_ERROR, err);
		}

		/* Peek next transfer */
		if (ev->ep != USB_CONTROL_EP_OUT && !udc_ep_is_busy(ev->dev, ev->ep)) {
			if (usbfsotg_xfer_next(ev->dev, ep_cfg) == 0) {
				udc_ep_set_busy(ev->dev, ev->ep, true);
			}
		}

xfer_work_error:
		k_mem_slab_free(&usbfsotg_ee_slab, (void *)ev);
	}
}

static ALWAYS_INLINE uint8_t stat_reg_get_ep(const uint8_t status)
{
	uint8_t ep_idx = status >> USB_STAT_ENDP_SHIFT;

	return (status & USB_STAT_TX_MASK) ? (USB_EP_DIR_IN | ep_idx) : ep_idx;
}

static ALWAYS_INLINE bool stat_reg_is_odd(const uint8_t status)
{
	return (status & USB_STAT_ODD_MASK) >> USB_STAT_ODD_SHIFT;
}

static ALWAYS_INLINE void set_control_in_pid_data1(const struct device *dev)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);

	/* Set DATA1 PID for data or status stage */
	ep_cfg->stat.data1 = true;
}

static ALWAYS_INLINE void isr_handle_xfer_done(const struct device *dev,
					       const uint8_t istatus,
					       const uint8_t status)
{
	struct usbfsotg_data *priv = udc_get_private(dev);
	uint8_t ep = stat_reg_get_ep(status);
	bool odd = stat_reg_is_odd(status);
	struct usbfsotg_bd *bd, *bd_op;
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	uint8_t token_pid;
	bool data1;
	size_t len;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	bd = usbfsotg_get_ebd(dev, ep_cfg, false);
	bd_op = usbfsotg_get_ebd(dev, ep_cfg, true);
	token_pid = bd->get.tok_pid;
	len  = bd->get.bc;
	data1 = bd->get.data1 ? true : false;

	LOG_DBG("TOKDNE, ep 0x%02x len %u odd %u data1 %u",
		ep, len, odd, data1);

	switch (token_pid) {
	case USBFSOTG_SETUP_TOKEN:
		ep_cfg->stat.odd = !odd;
		ep_cfg->stat.data1 = true;
		set_control_in_pid_data1(dev);

		if (priv->out_buf[odd] != NULL) {
			net_buf_add(priv->out_buf[odd], len);
			udc_ep_buf_set_setup(priv->out_buf[odd]);
			udc_buf_put(ep_cfg, priv->out_buf[odd]);
			priv->busy[odd] = false;
			priv->out_buf[odd] = NULL;
			usbfsotg_event_submit(dev, ep, USBFSOTG_EVT_SETUP);
		} else {
			LOG_ERR("No buffer for ep 0x00");
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
		}

		break;
	case USBFSOTG_OUT_TOKEN:
		ep_cfg->stat.odd = !odd;
		ep_cfg->stat.data1 = !data1;

		if (ep == USB_CONTROL_EP_OUT) {
			buf = priv->out_buf[odd];
			priv->busy[odd] = false;
			priv->out_buf[odd] = NULL;
		} else {
			buf = udc_buf_peek(dev, ep_cfg->addr);
		}

		if (buf == NULL) {
			LOG_ERR("No buffer for ep 0x%02x", ep);
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			break;
		}

		net_buf_add(buf, len);
		if (net_buf_tailroom(buf) >= udc_mps_ep_size(ep_cfg) &&
		    len == udc_mps_ep_size(ep_cfg)) {
			if (ep == USB_CONTROL_EP_OUT) {
				usbfsotg_ctrl_feed_start(dev, buf);
			} else {
				usbfsotg_xfer_continue(dev, ep_cfg, buf);
			}
		} else {
			if (ep == USB_CONTROL_EP_OUT) {
				udc_buf_put(ep_cfg, buf);
			}

			usbfsotg_event_submit(dev, ep, USBFSOTG_EVT_DOUT);
		}

		break;
	case USBFSOTG_IN_TOKEN:
		ep_cfg->stat.odd = !odd;
		ep_cfg->stat.data1 = !data1;

		buf = udc_buf_peek(dev, ep_cfg->addr);
		if (buf == NULL) {
			LOG_ERR("No buffer for ep 0x%02x", ep);
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			break;
		}

		net_buf_pull(buf, len);
		if (buf->len) {
			usbfsotg_xfer_continue(dev, ep_cfg, buf);
		} else {
			if (udc_ep_buf_has_zlp(buf)) {
				usbfsotg_xfer_continue(dev, ep_cfg, buf);
				udc_ep_buf_clear_zlp(buf);
				break;
			}

			usbfsotg_event_submit(dev, ep, USBFSOTG_EVT_DIN);
		}

		break;
	default:
		break;
	}
}

static void usbfsotg_isr_handler(const struct device *dev)
{
	const struct usbfsotg_config *config = dev->config;
	USB_Type *base = config->base;
	const uint8_t istatus  = base->ISTAT;
	const uint8_t status  = base->STAT;

	if (istatus & USB_ISTAT_USBRST_MASK) {
		base->ADDR = 0U;
		udc_submit_event(dev, UDC_EVT_RESET, 0);
	}

	if (istatus == USB_ISTAT_SOFTOK_MASK) {
		udc_submit_event(dev, UDC_EVT_SOF, 0);
	}

	if (istatus == USB_ISTAT_ERROR_MASK) {
		LOG_DBG("ERROR IRQ 0x%02x", base->ERRSTAT);
		udc_submit_event(dev, UDC_EVT_ERROR, base->ERRSTAT);
		base->ERRSTAT = 0xFF;
	}

	if (istatus & USB_ISTAT_STALL_MASK) {
		struct udc_ep_config *ep_cfg;

		LOG_DBG("STALL sent");

		ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);
		if (ep_cfg->stat.halted) {
			/*
			 * usbfsotg_ep_clear_halt(dev, ep_cfg); cannot
			 * be called in ISR context
			 */
			usbfsotg_event_submit(dev, USB_CONTROL_EP_OUT,
					      USBFSOTG_EVT_CLEAR_HALT);
		}

		ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
		if (ep_cfg->stat.halted) {
			usbfsotg_event_submit(dev, USB_CONTROL_EP_IN,
					      USBFSOTG_EVT_CLEAR_HALT);
		}
	}

	if (istatus & USB_ISTAT_TOKDNE_MASK) {
		isr_handle_xfer_done(dev, istatus, status);
	}

	if (istatus & USB_ISTAT_SLEEP_MASK) {
		LOG_DBG("SLEEP IRQ");
		/* Enable resume interrupt */
		base->INTEN |= USB_INTEN_RESUMEEN_MASK;

		udc_set_suspended(dev, true);
		udc_submit_event(dev, UDC_EVT_SUSPEND, 0);
	}

	if (istatus & USB_ISTAT_RESUME_MASK) {
		LOG_DBG("RESUME IRQ");
		/* Disable resume interrupt */
		base->INTEN &= ~USB_INTEN_RESUMEEN_MASK;

		udc_set_suspended(dev, false);
		udc_submit_event(dev, UDC_EVT_RESUME, 0);
	}

	/* Clear interrupt status bits */
	base->ISTAT = istatus;
}

static int usbfsotg_ep_enqueue(const struct device *dev,
			       struct udc_ep_config *const cfg,
			       struct net_buf *const buf)
{

	udc_buf_put(cfg, buf);
	if (cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted", cfg->addr);
		return 0;
	}

	usbfsotg_event_submit(dev, cfg->addr, USBFSOTG_EVT_XFER);

	return 0;
}

static int usbfsotg_ep_dequeue(const struct device *dev,
			       struct udc_ep_config *const cfg)
{
	struct usbfsotg_bd *bd;
	unsigned int lock_key;
	struct net_buf *buf;

	bd = usbfsotg_get_ebd(dev, cfg, false);

	lock_key = irq_lock();
	bd->set.bd_ctrl = USBFSOTG_BD_DTS;
	irq_unlock(lock_key);

	cfg->stat.halted = false;
	buf = udc_buf_get_all(dev, cfg->addr);
	if (buf) {
		udc_submit_ep_event(dev, buf, -ECONNABORTED);
	}

	udc_ep_set_busy(dev, cfg->addr, false);

	return 0;
}

static void ctrl_drop_out_successor(const struct device *dev)
{
	struct usbfsotg_data *priv = udc_get_private(dev);
	struct udc_ep_config *cfg;
	struct usbfsotg_bd *bd;
	struct net_buf *buf;

	cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_OUT);

	if (priv->busy[!cfg->stat.odd]) {
		bd = usbfsotg_get_ebd(dev, cfg, true);
		buf = priv->out_buf[!cfg->stat.odd];

		bd->bd_fields = 0U;
		priv->busy[!cfg->stat.odd] = false;
		if (buf) {
			net_buf_unref(buf);
		}
	}
}

static int usbfsotg_ep_set_halt(const struct device *dev,
				struct udc_ep_config *const cfg)
{
	struct usbfsotg_bd *bd;

	bd = usbfsotg_get_ebd(dev, cfg, false);
	bd->set.bd_ctrl = USBFSOTG_BD_STALL | USBFSOTG_BD_DTS | USBFSOTG_BD_OWN;
	cfg->stat.halted = true;
	LOG_DBG("Halt ep 0x%02x bd %p", cfg->addr, bd);

	if (cfg->addr == USB_CONTROL_EP_IN) {
		/* Drop subsequent out transfer, current can be re-used */
		ctrl_drop_out_successor(dev);
	}

	if (USB_EP_GET_IDX(cfg->addr) == 0U) {
		usbfsotg_resume_tx(dev);
	}

	return 0;
}

static int usbfsotg_ep_clear_halt(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	const struct usbfsotg_config *config = dev->config;
	struct usbfsotg_data *priv = udc_get_private(dev);
	USB_Type *base = config->base;
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	struct usbfsotg_bd *bd;

	LOG_DBG("Clear halt ep 0x%02x", cfg->addr);
	bd = usbfsotg_get_ebd(dev, cfg, false);

	if (bd->set.bd_ctrl & USBFSOTG_BD_STALL) {
		LOG_DBG("bd %p: %x", bd, bd->set.bd_ctrl);
		bd->set.bd_ctrl = USBFSOTG_BD_DTS;
	} else {
		LOG_WRN("bd %p is not halted", bd);
	}

	cfg->stat.data1 = false;
	cfg->stat.halted = false;
	base->ENDPOINT[ep_idx].ENDPT &= ~USB_ENDPT_EPSTALL_MASK;

	if (cfg->addr == USB_CONTROL_EP_OUT) {
		if (priv->busy[cfg->stat.odd]) {
			LOG_DBG("bd %p restarted", bd);
			bd->set.bd_ctrl = USBFSOTG_BD_DTS | USBFSOTG_BD_OWN;
		} else {
			usbfsotg_ctrl_feed_dout(dev, 8U, false, false);
		}
	}

	if (USB_EP_GET_IDX(cfg->addr) == 0U) {
		usbfsotg_resume_tx(dev);
	} else {
		/* trigger queued transfers */
		usbfsotg_event_submit(dev, cfg->addr, USBFSOTG_EVT_XFER);
	}

	return 0;
}

static int usbfsotg_ep_enable(const struct device *dev,
			      struct udc_ep_config *const cfg)
{
	const struct usbfsotg_config *config = dev->config;
	struct usbfsotg_data *priv = udc_get_private(dev);
	USB_Type *base = config->base;
	const uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	struct usbfsotg_bd *bd_even, *bd_odd;

	LOG_DBG("Enable ep 0x%02x", cfg->addr);
	bd_even = usbfsotg_get_ebd(dev, cfg, false);
	bd_odd = usbfsotg_get_ebd(dev, cfg, true);

	bd_even->bd_fields = 0U;
	bd_even->buf_addr = 0U;
	bd_odd->bd_fields = 0U;
	bd_odd->buf_addr = 0U;

	switch (cfg->attributes & USB_EP_TRANSFER_TYPE_MASK) {
	case USB_EP_TYPE_CONTROL:
		base->ENDPOINT[ep_idx].ENDPT = (USB_ENDPT_EPHSHK_MASK |
						USB_ENDPT_EPRXEN_MASK |
						USB_ENDPT_EPTXEN_MASK);
		break;
	case USB_EP_TYPE_BULK:
	case USB_EP_TYPE_INTERRUPT:
		base->ENDPOINT[ep_idx].ENDPT |= USB_ENDPT_EPHSHK_MASK;
		if (USB_EP_DIR_IS_OUT(cfg->addr)) {
			base->ENDPOINT[ep_idx].ENDPT |= USB_ENDPT_EPRXEN_MASK;
		} else {
			base->ENDPOINT[ep_idx].ENDPT |= USB_ENDPT_EPTXEN_MASK;
		}
		break;
	case USB_EP_TYPE_ISO:
		if (USB_EP_DIR_IS_OUT(cfg->addr)) {
			base->ENDPOINT[ep_idx].ENDPT |= USB_ENDPT_EPRXEN_MASK;
		} else {
			base->ENDPOINT[ep_idx].ENDPT |= USB_ENDPT_EPTXEN_MASK;
		}
		break;
	default:
		return -EINVAL;
	}

	if (cfg->addr == USB_CONTROL_EP_OUT) {
		struct net_buf *buf;

		priv->busy[0] = false;
		priv->busy[1] = false;
		buf = udc_ctrl_alloc(dev, USB_CONTROL_EP_OUT, USBFSOTG_EP0_SIZE);
		usbfsotg_bd_set_ctrl(bd_even, buf->size, buf->data, false);
		priv->out_buf[0] = buf;
	}

	return 0;
}

static int usbfsotg_ep_disable(const struct device *dev,
			       struct udc_ep_config *const cfg)
{
	const struct usbfsotg_config *config = dev->config;
	USB_Type *base = config->base;
	uint8_t ep_idx = USB_EP_GET_IDX(cfg->addr);
	struct usbfsotg_bd *bd_even, *bd_odd;

	bd_even = usbfsotg_get_ebd(dev, cfg, false);
	bd_odd = usbfsotg_get_ebd(dev, cfg, true);

	if (USB_EP_DIR_IS_OUT(cfg->addr)) {
		base->ENDPOINT[ep_idx].ENDPT &= ~USB_ENDPT_EPRXEN_MASK;
	} else {
		base->ENDPOINT[ep_idx].ENDPT &= ~USB_ENDPT_EPTXEN_MASK;
	}

	if (usbfsotg_bd_is_busy(bd_even) || usbfsotg_bd_is_busy(bd_odd)) {
		LOG_DBG("Endpoint buffer is busy");
	}

	bd_even->bd_fields = 0U;
	bd_even->buf_addr = 0U;
	bd_odd->bd_fields = 0U;
	bd_odd->buf_addr = 0U;

	LOG_DBG("Disable ep 0x%02x", cfg->addr);

	return 0;
}

static int usbfsotg_host_wakeup(const struct device *dev)
{
	return -ENOTSUP;
}

static int usbfsotg_set_address(const struct device *dev, const uint8_t addr)
{
	const struct usbfsotg_config *config = dev->config;
	USB_Type *base = config->base;

	base->ADDR = addr;

	return 0;
}

static int usbfsotg_enable(const struct device *dev)
{
	const struct usbfsotg_config *config = dev->config;
	USB_Type *base = config->base;

	/* non-OTG device mode, enable DP Pullup */
	base->CONTROL = USB_CONTROL_DPPULLUPNONOTG_MASK;

	return 0;
}

static int usbfsotg_disable(const struct device *dev)
{
	const struct usbfsotg_config *config = dev->config;
	USB_Type *base = config->base;

	/* disable USB and DP Pullup */
	base->CTL  &= ~USB_CTL_USBENSOFEN_MASK;
	base->CONTROL &= ~USB_CONTROL_DPPULLUPNONOTG_MASK;

	return 0;
}

static bool usbfsotg_is_supported(const struct device *dev)
{
	const struct usbfsotg_config *config = dev->config;
	USB_Type *base = config->base;

	if ((base->PERID != USBFSOTG_PERID) || (base->REV != USBFSOTG_REV)) {
		return false;
	}

	return true;
}

static int usbfsotg_init(const struct device *dev)
{
	const struct usbfsotg_config *config = dev->config;
	USB_Type *base = config->base;

#if !DT_ANY_INST_HAS_PROP_STATUS_OKAY(no_voltage_regulator)
	/* (FIXME) Enable USB voltage regulator */
	SIM->SOPT1 |= SIM_SOPT1_USBREGEN_MASK;
#endif

	/* Reset USB module */
	base->USBTRC0 |= USB_USBTRC0_USBRESET_MASK;
	k_busy_wait(2000);

	/* enable USB module, AKA USBEN bit in CTL1 register */
	base->CTL = USB_CTL_USBENSOFEN_MASK;

	if (!usbfsotg_is_supported(dev)) {
		return -ENOTSUP;
	}

	for (uint8_t i = 0; i < 16U; i++) {
		base->ENDPOINT[i].ENDPT = 0;
	}

	base->BDTPAGE1 = (uint8_t)(POINTER_TO_UINT(config->bdt) >> 8);
	base->BDTPAGE2 = (uint8_t)(POINTER_TO_UINT(config->bdt) >> 16);
	base->BDTPAGE3 = (uint8_t)(POINTER_TO_UINT(config->bdt) >> 24);

	/* (FIXME) Enables the weak pulldowns on the USB transceiver */
	base->USBCTRL = USB_USBCTRL_PDE_MASK;

	/* Clear interrupt flags */
	base->ISTAT = 0xFF;
	/* Clear error flags */
	base->ERRSTAT = 0xFF;

	/* Enable all error interrupt sources */
	base->ERREN = 0xFF;
	/* Enable reset interrupt */
	base->INTEN = (USB_INTEN_SLEEPEN_MASK  |
		       USB_INTEN_STALLEN_MASK |
		       USB_INTEN_TOKDNEEN_MASK |
		       USB_INTEN_SOFTOKEN_MASK |
		       USB_INTEN_ERROREN_MASK |
		       USB_INTEN_USBRSTEN_MASK);

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_OUT,
				   USB_EP_TYPE_CONTROL,
				   USBFSOTG_EP0_SIZE, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	if (udc_ep_enable_internal(dev, USB_CONTROL_EP_IN,
				   USB_EP_TYPE_CONTROL,
				   USBFSOTG_EP0_SIZE, 0)) {
		LOG_ERR("Failed to enable control endpoint");
		return -EIO;
	}

	/* Connect and enable USB interrupt */
	config->irq_enable_func(dev);

	LOG_DBG("Initialized USB controller %p", base);

	return 0;
}

static int usbfsotg_shutdown(const struct device *dev)
{
	const struct usbfsotg_config *config = dev->config;

	config->irq_disable_func(dev);

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_OUT)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	if (udc_ep_disable_internal(dev, USB_CONTROL_EP_IN)) {
		LOG_ERR("Failed to disable control endpoint");
		return -EIO;
	}

	/* Disable USB module */
	config->base->CTL = 0;

#if !DT_ANY_INST_HAS_PROP_STATUS_OKAY(no_voltage_regulator)
	/* Disable USB voltage regulator */
	SIM->SOPT1 &= ~SIM_SOPT1_USBREGEN_MASK;
#endif

	return 0;
}

static void usbfsotg_lock(const struct device *dev)
{
	udc_lock_internal(dev, K_FOREVER);
}

static void usbfsotg_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
}

static int usbfsotg_driver_preinit(const struct device *dev)
{
	const struct usbfsotg_config *config = dev->config;
	struct udc_data *data = dev->data;
	struct usbfsotg_data *priv = data->priv;
	int err;

	k_mutex_init(&data->mutex);
	k_fifo_init(&priv->fifo);
	k_work_init(&priv->work, xfer_work_handler);

	for (int i = 0; i < config->num_of_eps; i++) {
		config->ep_cfg_out[i].caps.out = 1;
		if (i == 0) {
			config->ep_cfg_out[i].caps.control = 1;
			config->ep_cfg_out[i].caps.mps = 64;
		} else {
			config->ep_cfg_out[i].caps.bulk = 1;
			config->ep_cfg_out[i].caps.interrupt = 1;
			config->ep_cfg_out[i].caps.iso = 1;
			config->ep_cfg_out[i].caps.mps = 1023;
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
			config->ep_cfg_in[i].caps.mps = 1023;
		}

		config->ep_cfg_in[i].addr = USB_EP_DIR_IN | i;
		err = udc_register_ep(dev, &config->ep_cfg_in[i]);
		if (err != 0) {
			LOG_ERR("Failed to register endpoint");
			return err;
		}
	}

	data->caps.rwup = false;
	data->caps.mps0 = USBFSOTG_MPS0;

	return 0;
}

static const struct udc_api usbfsotg_api = {
	.ep_enqueue = usbfsotg_ep_enqueue,
	.ep_dequeue = usbfsotg_ep_dequeue,
	.ep_set_halt = usbfsotg_ep_set_halt,
	.ep_clear_halt = usbfsotg_ep_clear_halt,
	.ep_try_config = NULL,
	.ep_enable = usbfsotg_ep_enable,
	.ep_disable = usbfsotg_ep_disable,
	.host_wakeup = usbfsotg_host_wakeup,
	.set_address = usbfsotg_set_address,
	.enable = usbfsotg_enable,
	.disable = usbfsotg_disable,
	.init = usbfsotg_init,
	.shutdown = usbfsotg_shutdown,
	.lock = usbfsotg_lock,
	.unlock = usbfsotg_unlock,
};

#define USBFSOTG_DEVICE_DEFINE(n)						\
	static void udc_irq_enable_func##n(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    usbfsotg_isr_handler,				\
			    DEVICE_DT_INST_GET(n), 0);				\
										\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static void udc_irq_disable_func##n(const struct device *dev)		\
	{									\
		irq_disable(DT_INST_IRQN(n));					\
	}									\
										\
	static struct usbfsotg_bd __aligned(512)				\
		bdt_##n[DT_INST_PROP(n, num_bidir_endpoints) * 2 * 2];		\
										\
	static struct udc_ep_config						\
		ep_cfg_out[DT_INST_PROP(n, num_bidir_endpoints)];		\
	static struct udc_ep_config						\
		ep_cfg_in[DT_INST_PROP(n, num_bidir_endpoints)];		\
										\
	static struct usbfsotg_config priv_config_##n = {			\
		.base = (USB_Type *)DT_INST_REG_ADDR(n),			\
		.bdt = bdt_##n,							\
		.irq_enable_func = udc_irq_enable_func##n,			\
		.irq_disable_func = udc_irq_disable_func##n,			\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),		\
		.ep_cfg_in = ep_cfg_out,					\
		.ep_cfg_out = ep_cfg_in,					\
	};									\
										\
	static struct usbfsotg_data priv_data_##n = {				\
	};									\
										\
	static struct udc_data udc_data_##n = {					\
		.mutex = Z_MUTEX_INITIALIZER(udc_data_##n.mutex),		\
		.priv = &priv_data_##n,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, usbfsotg_driver_preinit, NULL,			\
			      &udc_data_##n, &priv_config_##n,			\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &usbfsotg_api);

DT_INST_FOREACH_STATUS_OKAY(USBFSOTG_DEVICE_DEFINE)
