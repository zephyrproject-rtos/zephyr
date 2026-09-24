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
	void (*make_thread)(const struct device *dev);
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
};

enum usbfsotg_event_type {
	/* Setup packet received */
	USBFSOTG_EVT_SETUP,
	/* Trigger new transfer (except control endpoints) */
	USBFSOTG_EVT_XFER_NEW,
	/* Transfer for specific endpoint is finished */
	USBFSOTG_EVT_XFER_FINISHED,
};

struct usbfsotg_data {
	struct k_thread thread_data;
	struct k_event events;
	/*
	 * xfer_new and xfer_finished contain information on which endpoints
	 * events USBFSOTG_EVT_XFER_NEW or USBFSOTG_EVT_XFER_FINISHED are
	 * triggered. The mapping is bits 31..16 for IN endpoints and bits
	 * 15..0 for OUT endpoints.
	 */
	atomic_t xfer_new;
	atomic_t xfer_finished;
	struct usb_setup_packet setup;
	uint32_t odd;
	bool setup_valid;
};

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
	}

	return USB_EP_DIR_OUT | bit;
}

static uint8_t usbfsotg_get_odd_bit(const uint8_t ep)
{
	return USB_EP_DIR_IS_IN(ep) * 16U + USB_EP_GET_IDX(ep);
}

/* Get buffer descriptor (BD) based on endpoint address */
static struct usbfsotg_bd *usbfsotg_get_ebd(const struct device *const dev,
					    struct udc_ep_config *const cfg,
					    const bool opposite)
{
	const struct usbfsotg_config *config = dev->config;
	struct usbfsotg_data *priv = udc_get_private(dev);
	bool odd = priv->odd & BIT(usbfsotg_get_odd_bit(cfg->addr));
	uint8_t bd_idx;

	bd_idx = USB_EP_GET_IDX(cfg->addr) * 4U + (odd ^ opposite);
	if (USB_EP_DIR_IS_IN(cfg->addr)) {
		bd_idx += 2U;
	}

	return &config->bdt[bd_idx];
}

static void usbfsotg_ebd_ctrl_discard(const struct device *const dev)
{
	const struct usbfsotg_config *config = dev->config;

	config->bdt[0].set.bd_ctrl = 0;
	config->bdt[1].set.bd_ctrl = 0;
	config->bdt[2].set.bd_ctrl = 0;
	config->bdt[3].set.bd_ctrl = 0;
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

static bool usbfsotg_is_tx_suspended(const struct device *dev)
{
	const struct usbfsotg_config *config = dev->config;
	USB_Type *base = config->base;

	return base->CTL & USB_CTL_TXSUSPENDTOKENBUSY_MASK;
}

static ALWAYS_INLINE void set_control_in_pid_data1(const struct device *dev)
{
	struct udc_ep_config *ep_cfg = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);

	/* Set DATA1 PID for data or status stage */
	ep_cfg->stat.data1 = true;
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

	buf = udc_buf_peek(cfg);
	if (buf == NULL) {
		return -ENODATA;
	}

	return usbfsotg_xfer_continue(dev, cfg, buf);
}

static inline int handle_evt_setup(const struct device *dev)
{
	struct usbfsotg_data *priv = udc_get_private(dev);

	usbfsotg_ebd_ctrl_discard(dev);
	set_control_in_pid_data1(dev);

	udc_setup_received(dev, priv->setup_valid ? &priv->setup : NULL);

	return 0;
}

static inline int handle_evt_finished(const struct device *dev,
				     struct udc_ep_config *ep_cfg)
{
	struct net_buf *buf;

	buf = udc_buf_get(ep_cfg);
	if (buf == NULL) {
		return -ENODATA;
	}

	return udc_submit_ep_event(dev, buf, 0);
}

static void usbfsotg_xfer_new_submit(const struct device *dev, const uint8_t ep)
{
	struct usbfsotg_data *priv = udc_get_private(dev);

	atomic_set_bit(&priv->xfer_new, udc_ep_to_bnum(ep));
	k_event_post(&priv->events, BIT(USBFSOTG_EVT_XFER_NEW));
}

static void usbfsotg_xfer_finished_submit(const struct device *dev, const uint8_t ep)
{
	struct usbfsotg_data *priv = udc_get_private(dev);

	atomic_set_bit(&priv->xfer_finished, udc_ep_to_bnum(ep));
	k_event_post(&priv->events, BIT(USBFSOTG_EVT_XFER_FINISHED));
}

static ALWAYS_INLINE void usbfsotg_thread_handler(const struct device *const dev)
{
	struct usbfsotg_data *priv = udc_get_private(dev);
	struct udc_ep_config *ep_cfg;
	uint32_t evt;
	uint32_t eps;
	uint8_t ep;
	int err;

	evt = k_event_wait(&priv->events, UINT32_MAX, false, K_FOREVER);
	udc_lock_internal(dev, K_FOREVER);

	if (evt & BIT(USBFSOTG_EVT_XFER_FINISHED)) {
		k_event_clear(&priv->events, BIT(USBFSOTG_EVT_XFER_FINISHED));
		eps = atomic_clear(&priv->xfer_finished);

		while (eps) {
			ep = udc_pull_ep_from_bmsk(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);
			LOG_DBG("Finished event ep 0x%02x", ep);

			err = handle_evt_finished(dev, ep_cfg);
			if (unlikely(err)) {
				udc_submit_event(dev, UDC_EVT_ERROR, err);
			}

			udc_ep_set_busy(ep_cfg, false);

			/* Peek next transfer */
			if (ep != USB_CONTROL_EP_OUT) {
				if (usbfsotg_xfer_next(dev, ep_cfg) == 0) {
					udc_ep_set_busy(ep_cfg, true);
				}
			}
		}
	}

	if (evt & BIT(USBFSOTG_EVT_XFER_NEW)) {
		k_event_clear(&priv->events, BIT(USBFSOTG_EVT_XFER_NEW));
		eps = atomic_clear(&priv->xfer_new);

		while (eps) {
			ep = udc_pull_ep_from_bmsk(&eps);
			ep_cfg = udc_get_ep_cfg(dev, ep);
			LOG_DBG("New transfer ep 0x%02x in the queue", ep);

			if (!udc_ep_is_busy(ep_cfg)) {
				if (usbfsotg_xfer_next(dev, ep_cfg) == 0) {
					udc_ep_set_busy(ep_cfg, true);
				}
			}
		}
	}

	if (evt & BIT(USBFSOTG_EVT_SETUP)) {
		k_event_clear(&priv->events, BIT(USBFSOTG_EVT_SETUP));

		err = handle_evt_setup(dev);
		if (unlikely(err)) {
			udc_submit_event(dev, UDC_EVT_ERROR, err);
		}
	}

	udc_unlock_internal(dev);
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

static ALWAYS_INLINE void isr_handle_xfer_done(const struct device *dev,
					       const uint8_t istatus,
					       const uint8_t status)
{
	struct usbfsotg_data *priv = udc_get_private(dev);
	uint8_t ep = stat_reg_get_ep(status);
	const uint8_t odd_bit = usbfsotg_get_odd_bit(ep);
	bool odd = stat_reg_is_odd(status);
	struct usbfsotg_bd *bd;
	struct udc_ep_config *ep_cfg;
	struct net_buf *buf;
	uint8_t token_pid;
	bool data1;
	size_t len;

	ep_cfg = udc_get_ep_cfg(dev, ep);
	bd = usbfsotg_get_ebd(dev, ep_cfg, false);
	token_pid = bd->get.tok_pid;
	len  = bd->get.bc;
	data1 = bd->get.data1 ? true : false;

	LOG_DBG("TOKDNE, ep 0x%02x len %u odd %u data1 %u",
		ep, len, odd, data1);

	switch (token_pid) {
	case USBFSOTG_SETUP_TOKEN:
		WRITE_BIT(priv->odd, odd_bit, !odd);
		ep_cfg->stat.data1 = true;

		/* Record whether the number of received setup data was valid */
		priv->setup_valid = len == sizeof(struct usb_setup_packet);

		/* Only copy the SETUP data if it is valid length */
		if (len == sizeof(priv->setup)) {
			memcpy(&priv->setup, UINT_TO_POINTER(bd->buf_addr), sizeof(priv->setup));
		}

		k_event_post(&priv->events, BIT(USBFSOTG_EVT_SETUP));
		break;
	case USBFSOTG_OUT_TOKEN:
		WRITE_BIT(priv->odd, odd_bit, !odd);
		ep_cfg->stat.data1 = !data1;

		buf = udc_buf_peek(ep_cfg);

		if (buf == NULL) {
			LOG_ERR("No buffer for ep 0x%02x", ep);
			udc_submit_event(dev, UDC_EVT_ERROR, -ENOBUFS);
			break;
		}

		net_buf_add(buf, len);
		if (net_buf_tailroom(buf) >= udc_mps_ep_size(ep_cfg) &&
		    len == udc_mps_ep_size(ep_cfg)) {
			usbfsotg_xfer_continue(dev, ep_cfg, buf);
		} else {
			usbfsotg_xfer_finished_submit(dev, ep);
		}

		break;
	case USBFSOTG_IN_TOKEN:
		WRITE_BIT(priv->odd, odd_bit, !odd);
		ep_cfg->stat.data1 = !data1;

		buf = udc_buf_peek(ep_cfg);
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

			usbfsotg_xfer_finished_submit(dev, ep);
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
		udc_submit_sof_event(dev);
	}

	if (istatus == USB_ISTAT_ERROR_MASK) {
		LOG_DBG("ERROR IRQ 0x%02x", base->ERRSTAT);
		udc_submit_event(dev, UDC_EVT_ERROR, base->ERRSTAT);
		base->ERRSTAT = 0xFF;
	}

	if (istatus & USB_ISTAT_STALL_MASK) {
		LOG_DBG("STALL sent");
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
	/* Control transfer handling code synchronization relies on USBFSOTG CTL
	 * TXSUSPENDTOKENBUSY bit and USB stack enqueue order.
	 */
	if (cfg->addr == USB_CONTROL_EP_OUT) {
		struct udc_buf_info *bi = udc_get_buf_info(buf);
		struct usbfsotg_bd *bd;
		size_t length;
		bool data1 = bi->setup ? false : true;
		bool tx_suspended = usbfsotg_is_tx_suspended(dev);

		if (udc_buf_peek(cfg)) {
			/* STATUS OUT is already queued */
			bd = usbfsotg_get_ebd(dev, cfg, true);
		} else {
			bd = usbfsotg_get_ebd(dev, cfg, false);
		}

		udc_buf_put(cfg, buf);

		length = MIN(net_buf_tailroom(buf), udc_mps_ep_size(cfg));
		usbfsotg_bd_set_ctrl(bd, length, net_buf_tail(buf), data1);

		LOG_DBG("xfer %p, bd %p, ep 0x%02x, bd field 0x%02x",
			buf, bd, cfg->addr, bd->bd_fields);

		if (bi->setup) {
			struct udc_ep_config *cfg_in;

			cfg_in = udc_get_ep_cfg(dev, USB_CONTROL_EP_IN);
			usbfsotg_xfer_next(dev, cfg_in);
		}

		if (bi->data || bi->setup) {
			/* Signal resume only if CTL TXSUSPENDTOKENBUSY was set
			 * before EP0OUT BD was updated.
			 */
			if (tx_suspended) {
				usbfsotg_resume_tx(dev);
			}
		}

		return 0;
	}

	udc_buf_put(cfg, buf);

	if (cfg->addr == USB_CONTROL_EP_IN) {
		/* Update BDT after USB stack is ready to process next SETUP */
		return 0;
	}

	if (cfg->stat.halted) {
		LOG_DBG("ep 0x%02x halted", cfg->addr);
		return 0;
	}

	usbfsotg_xfer_new_submit(dev, cfg->addr);

	return 0;
}

static int usbfsotg_ep_dequeue(const struct device *dev,
			       struct udc_ep_config *const cfg)
{
	struct usbfsotg_bd *bd;
	unsigned int lock_key;

	bd = usbfsotg_get_ebd(dev, cfg, false);

	lock_key = irq_lock();
	bd->set.bd_ctrl = USBFSOTG_BD_DTS;
	irq_unlock(lock_key);

	cfg->stat.halted = false;
	udc_ep_cancel_queued(dev, cfg);

	udc_ep_set_busy(cfg, false);

	return 0;
}

static int usbfsotg_ep_set_halt(const struct device *dev,
				struct udc_ep_config *const cfg)
{
	struct usbfsotg_bd *bd;

	bd = usbfsotg_get_ebd(dev, cfg, false);
	bd->set.bd_ctrl = USBFSOTG_BD_STALL | USBFSOTG_BD_DTS | USBFSOTG_BD_OWN;
	cfg->stat.halted = true;
	LOG_DBG("Halt ep 0x%02x bd %p", cfg->addr, bd);

	return 0;
}

static int usbfsotg_ep_clear_halt(const struct device *dev,
				  struct udc_ep_config *const cfg)
{
	const struct usbfsotg_config *config = dev->config;
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

	if (USB_EP_GET_IDX(cfg->addr) != 0) {
		/* trigger queued transfers */
		usbfsotg_xfer_new_submit(dev, cfg->addr);
	}

	return 0;
}

static int usbfsotg_ep_enable(const struct device *dev,
			      struct udc_ep_config *const cfg)
{
	const struct usbfsotg_config *config = dev->config;
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

	/* Disable DP Pullup */
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
		       IF_ENABLED(CONFIG_UDC_ENABLE_SOF, (USB_INTEN_SOFTOKEN_MASK |))
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
	struct usbfsotg_data *priv = udc_get_private(dev);

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

	/* Cleanup BDT */
	memset(config->bdt, 0, sizeof(struct usbfsotg_bd) * config->num_of_eps * 2 * 2);

	/* All controller odd bits are reset, update internal tracking */
	priv->odd = 0;

	return 0;
}

static void usbfsotg_lock(const struct device *dev)
{
	k_sched_lock();
	udc_lock_internal(dev, K_FOREVER);
}

static void usbfsotg_unlock(const struct device *dev)
{
	udc_unlock_internal(dev);
	k_sched_unlock();
}

static int usbfsotg_driver_preinit(const struct device *dev)
{
	const struct usbfsotg_config *config = dev->config;
	struct udc_data *data = dev->data;
	struct usbfsotg_data *priv = data->priv;
	int err;

	k_mutex_init(&data->mutex);
	k_event_init(&priv->events);
	atomic_clear(&priv->xfer_new);
	atomic_clear(&priv->xfer_finished);

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

	config->make_thread(dev);

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

#define USBFSOTG_IRQ_DEFINE_OR(n)						\
	COND_CODE_1(CONFIG_UHC_NXP_KHCI,					\
	(irq_connect_dynamic(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			     (void (*)(const void *))usbfsotg_isr_handler,	\
			     DEVICE_DT_INST_GET(n), 0)),			\
	(IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
		     usbfsotg_isr_handler,					\
		     DEVICE_DT_INST_GET(n), 0)))

#define USBFSOTG_DEVICE_DEFINE(n)						\
	K_THREAD_STACK_DEFINE(usbfsotg_stack_##n,				\
			      CONFIG_UDC_KINETIS_STACK_SIZE);			\
										\
	static void usbfsotg_thread_##n(void *dev, void *arg1, void *arg2)	\
	{									\
		while (true) {							\
			usbfsotg_thread_handler(dev);				\
		}								\
	}									\
										\
	static void usbfsotg_make_thread_##n(const struct device *dev)		\
	{									\
		struct usbfsotg_data *priv = udc_get_private(dev);		\
										\
		k_thread_create(&priv->thread_data,				\
				usbfsotg_stack_##n,				\
				K_THREAD_STACK_SIZEOF(usbfsotg_stack_##n),	\
				usbfsotg_thread_##n,				\
				(void *)dev, NULL, NULL,			\
				K_PRIO_COOP(CONFIG_UDC_KINETIS_THREAD_PRIORITY),\
				K_ESSENTIAL,					\
				K_NO_WAIT);					\
		k_thread_name_set(&priv->thread_data, dev->name);		\
	}									\
										\
	static void udc_irq_enable_func##n(const struct device *dev)		\
	{									\
		USBFSOTG_IRQ_DEFINE_OR(n);					\
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
		.make_thread = usbfsotg_make_thread_##n,			\
		.num_of_eps = DT_INST_PROP(n, num_bidir_endpoints),		\
		.ep_cfg_in = ep_cfg_in,						\
		.ep_cfg_out = ep_cfg_out,					\
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
