/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file  uhc_virtual.c
 * @brief Virtual USB host controller (UHC) driver
 *
 * Virtual device controller does not emulate any hardware
 * and can only communicate with the virtual device controllers
 * through virtual bus.
 */

#include "uhc_common.h"
#include "../uvb/uvb.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/usb/uhc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uhc_vrt, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define FRAME_MAX_TRANSFERS 16

/*
 * USB 2.0 limits periodic (interrupt/isochronous) transfers to at most
 * 90% of a (micro)frame's time budget, leaving the rest for control/bulk.
 */
#define FRAME_NSECS_FS 1000000UL
#define FRAME_NSECS_HS 125000UL
#define FRAME_PERIODIC_BUDGET(speed) \
	(((speed) == USB_SPEED_SPEED_HS ? FRAME_NSECS_HS : FRAME_NSECS_FS) * 9UL / 10UL)

/*
 * Approximate bus time for a periodic transaction, in nanoseconds
 * (USB 2.0 5.11.3).
 *
 * VRT_BIT_STUFF_TERM() is (3.167 + BitStuffTime(Data_bc)) * 1000: BitStuffTime
 * is the worst-case bit-stuffed bit count (7/6 stuff ratio times 8 bits per
 * byte), and 3.167 is a few extra fixed bit-times folded into the same term.
 * Dividing by 1000 below floors it back to a plain bit-time count.
 *
 * 2083 and 8354 are the High-/Full-speed bit time (1000/480e6 s and
 * 1000/12e6 s, the latter using the spec's own published rounding) scaled
 * up so the bit-time count can be multiplied by them in integers; dividing
 * by 1000 or 100 afterwards undoes that scaling and leaves nanoseconds.
 */
#define VRT_BIT_STUFF_TERM(bytecount) (3167UL + 9334UL * (bytecount))

#define VRT_HS_NSECS(bytecount) \
	((2083UL * (55UL * 8UL + VRT_BIT_STUFF_TERM(bytecount) / 1000UL)) / 1000UL + 5UL)
#define VRT_HS_NSECS_ISO(bytecount) \
	((2083UL * (38UL * 8UL + VRT_BIT_STUFF_TERM(bytecount) / 1000UL)) / 1000UL + 5UL)
#define VRT_FS_NSECS(bytecount) \
	(9107UL + (8354UL * (VRT_BIT_STUFF_TERM(bytecount) / 1000UL)) / 100UL)
#define VRT_FS_NSECS_ISO(bytecount, fixed) \
	((fixed) + (8354UL * (VRT_BIT_STUFF_TERM(bytecount) / 1000UL)) / 100UL)

static uint32_t vrt_xfer_bus_time(const struct uhc_transfer *const xfer,
				  const enum usb_device_speed speed)
{
	bool isoc = xfer->type == USB_EP_TYPE_ISO;
	uint32_t bc = xfer->mps;

	if (speed == USB_SPEED_SPEED_HS) {
		return isoc ? VRT_HS_NSECS_ISO(bc) : VRT_HS_NSECS(bc);
	}

	/* Full speed. Low speed is not supported yet. */
	if (isoc) {
		return VRT_FS_NSECS_ISO(bc, USB_EP_DIR_IS_IN(xfer->ep) ? 7268UL : 6265UL);
	}

	return VRT_FS_NSECS(bc);
}

/*
 * UVB has no propagation delay, and this timeout is much higher than specified
 * in USB 2.0 (7.1.19.2) So, it should be good enough even under high CPU load.
 */
#define UHC_VRT_XFER_TIMEOUT K_MSEC(1)

struct uhc_vrt_config {
	k_thread_stack_t *thread_stack;
	size_t stack_size;
};

struct uhc_vrt_slot {
	sys_dnode_t node;
	struct uhc_transfer *xfer;
};

struct uhc_vrt_frame {
	struct uhc_vrt_slot slots[FRAME_MAX_TRANSFERS];
	sys_dnode_t *ptr;
	sys_dlist_t list;
	uint8_t count;
};

struct uhc_vrt_data {
	const struct device *dev;
	struct uvb_node *host_node;
	struct k_thread thread_data;
	struct k_fifo fifo;
	struct uhc_transfer *last_xfer;
	struct uvb_packet *last_pkt;
	k_timepoint_t xfer_timeout;
	struct uhc_vrt_frame frame;
	struct k_timer sof_timer;
	k_timeout_t sof_period;
	uint16_t frame_number;
	uint8_t req;
	enum usb_device_speed speed;
};

enum uhc_vrt_event_type {
	/* SoF generator event */
	UHC_VRT_EVT_SOF,
	/* Request reply received */
	UHC_VRT_EVT_REPLY,
};

/* Structure for driver's endpoint events */
struct uhc_vrt_event {
	sys_snode_t node;
	enum uhc_vrt_event_type type;
	struct uvb_packet *pkt;
};

K_MEM_SLAB_DEFINE_TYPE(uhc_vrt_slab, struct uhc_vrt_event, 16);

static void vrt_event_submit(const struct device *dev,
			     const enum uhc_vrt_event_type type,
			     const void *data)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);
	struct uhc_vrt_event *event;
	int ret;

	ret = k_mem_slab_alloc(&uhc_vrt_slab, (void **)&event, K_NO_WAIT);
	__ASSERT(ret == 0, "Failed to allocate slab");

	event->type = type;
	event->pkt = (struct uvb_packet *const)data;
	k_fifo_put(&priv->fifo, event);
}

static int vrt_advert_pkt(struct uhc_vrt_data *const priv,
			  struct uvb_packet *const pkt)
{
	/*
	 * The device may get disconnected and there would not be any reply.
	 * Track the packet of the last transaction and clean it up in
	 * vrt_xfer_drop_active() to prevent a packet leak on device disconnect.
	 */
	priv->last_pkt = pkt;
	/*
	 * If the device does not respond for different reasons, check timeout
	 * on SOF and cleanup.
	 */
	priv->xfer_timeout = sys_timepoint_calc(UHC_VRT_XFER_TIMEOUT);

	return uvb_advert_pkt(priv->host_node, pkt);
}

static int vrt_xfer_control(const struct device *dev,
			    struct uhc_transfer *const xfer)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);
	struct net_buf *buf = xfer->buf;
	struct uvb_packet *uvb_pkt;
	uint8_t *data = NULL;
	size_t length = 0;

	if (xfer->stage == UHC_CONTROL_STAGE_SETUP) {
		LOG_DBG("Handle SETUP stage");
		uvb_pkt = uvb_alloc_pkt(UVB_REQUEST_SETUP,
					xfer->udev->addr, USB_CONTROL_EP_OUT,
					xfer->setup_pkt, 8);
		if (uvb_pkt == NULL) {
			LOG_ERR("Failed to allocate UVB packet");
			return -ENOMEM;
		}

		priv->req = UVB_REQUEST_SETUP;

		return vrt_advert_pkt(priv, uvb_pkt);
	}

	if (buf != NULL && xfer->stage == UHC_CONTROL_STAGE_DATA) {
		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			length = MIN(net_buf_tailroom(buf), xfer->mps);
			data = net_buf_tail(buf);
		} else {
			length = MIN(buf->len, xfer->mps);
			data = buf->data;
		}

		LOG_DBG("Handle DATA stage");
		uvb_pkt = uvb_alloc_pkt(UVB_REQUEST_DATA,
					xfer->udev->addr, xfer->ep,
					data, length);
		if (uvb_pkt == NULL) {
			LOG_ERR("Failed to allocate UVB packet");
			return -ENOMEM;
		}

		priv->req = UVB_REQUEST_DATA;

		return vrt_advert_pkt(priv, uvb_pkt);
	}

	if (xfer->stage == UHC_CONTROL_STAGE_STATUS) {
		uint8_t ep;

		LOG_DBG("Handle STATUS stage");
		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			ep = USB_CONTROL_EP_OUT;
		} else {
			ep = USB_CONTROL_EP_IN;
		}

		uvb_pkt = uvb_alloc_pkt(UVB_REQUEST_DATA,
					xfer->udev->addr, ep,
					NULL, 0);
		if (uvb_pkt == NULL) {
			LOG_ERR("Failed to allocate UVB packet");
			return -ENOMEM;
		}

		priv->req = UVB_REQUEST_DATA;

		return vrt_advert_pkt(priv, uvb_pkt);
	}

	return -EINVAL;
}

static int vrt_xfer_bulk(const struct device *dev,
			 struct uhc_transfer *const xfer)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);
	struct net_buf *buf = xfer->buf;
	struct uvb_packet *uvb_pkt;
	uint8_t *data;
	size_t length;

	if (USB_EP_DIR_IS_IN(xfer->ep)) {
		length = MIN(net_buf_tailroom(buf), xfer->mps);
		data = net_buf_tail(buf);
	} else {
		length = MIN(buf->len, xfer->mps);
		data = buf->data;
	}

	uvb_pkt = uvb_alloc_pkt(UVB_REQUEST_DATA, xfer->udev->addr, xfer->ep,
				data, length);
	if (uvb_pkt == NULL) {
		LOG_ERR("Failed to allocate UVB packet");
		return -ENOMEM;
	}

	return vrt_advert_pkt(priv, uvb_pkt);
}

static inline uint8_t get_xfer_ep_idx(const uint8_t ep)
{
	/* We do not need to differentiate the direction for the control
	 * transfers because they are handled as a whole.
	 */
	if (USB_EP_DIR_IS_OUT(ep) || USB_EP_GET_IDX(ep) == 0) {
		return USB_EP_GET_IDX(ep & BIT_MASK(4));
	}

	return USB_EP_GET_IDX(ep & BIT_MASK(4)) + 16U;
}

static void vrt_assemble_frame(const struct device *dev)
{
	struct uhc_vrt_data *const priv = uhc_get_private(dev);
	struct uhc_vrt_frame *const frame = &priv->frame;
	struct uhc_data *const data = dev->data;
	struct uhc_transfer *tmp;
	unsigned int n = 0;
	unsigned int key;
	uint32_t bm = 0;
	uint32_t periodic_used = 0;
	uint32_t budget = FRAME_PERIODIC_BUDGET(priv->speed);

	sys_dlist_init(&frame->list);
	frame->ptr = NULL;
	frame->count = 0;
	key = irq_lock();

	SYS_DLIST_FOR_EACH_CONTAINER(&data->ctrl_xfers, tmp, node) {
		uint8_t idx = get_xfer_ep_idx(tmp->ep);

		/* There could be multiple transfers queued for the same
		 * endpoint, for now we only allow one to be scheduled per frame.
		 */
		if (bm & BIT(idx)) {
			continue;
		}

		if (tmp->interval) {
			uint32_t bus_time;

			if (tmp->start_frame != priv->frame_number) {
				continue;
			}

			bus_time = vrt_xfer_bus_time(tmp, priv->speed);
			if (periodic_used + bus_time > budget) {
				/* Frame's periodic budget is full, retry next frame. */
				tmp->start_frame = priv->frame_number + 1;
				continue;
			}

			periodic_used += bus_time;
			tmp->start_frame = priv->frame_number + tmp->interval;
			LOG_DBG("Interrupt transfer s.f. %u f.n. %u interval %u",
				tmp->start_frame, priv->frame_number, tmp->interval);
		}

		bm |= BIT(idx);
		frame->slots[n].xfer = tmp;
		sys_dlist_append(&frame->list, &frame->slots[n].node);
		n++;

		if (n >= FRAME_MAX_TRANSFERS) {
			/* No more free slots */
			break;
		}
	}

	irq_unlock(key);
}

static int vrt_schedule_frame(const struct device *dev)
{
	struct uhc_vrt_data *const priv = uhc_get_private(dev);
	struct uhc_vrt_frame *const frame = &priv->frame;
	struct uhc_vrt_slot *slot;

	if (priv->last_xfer == NULL) {
		if (frame->count >= FRAME_MAX_TRANSFERS) {
			LOG_DBG("Frame finished");
			return 0;
		}

		frame->ptr = sys_dlist_get(&frame->list);
		slot = SYS_DLIST_CONTAINER(frame->ptr, slot, node);
		if (slot == NULL) {
			LOG_DBG("No more transfers for the frame");
			return 0;
		}

		priv->last_xfer = slot->xfer;
		frame->count++;
		LOG_DBG("Next transfer is %p (count %u)",
			(void *)priv->last_xfer, frame->count);
	}

	if (USB_EP_GET_IDX(priv->last_xfer->ep) == 0) {
		return vrt_xfer_control(dev, priv->last_xfer);
	}

	return vrt_xfer_bulk(dev, priv->last_xfer);
}

static void vrt_hrslt_success(const struct device *dev,
			      struct uvb_packet *const pkt)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);
	struct uhc_transfer *const xfer = priv->last_xfer;
	struct net_buf *buf = xfer->buf;
	bool finished = false;
	size_t length;

	switch (pkt->request) {
	case UVB_REQUEST_SETUP:
		if (xfer->buf != NULL) {
			xfer->stage = UHC_CONTROL_STAGE_DATA;
		} else {
			if (xfer->no_status) {
				finished = true;
			} else {
				xfer->stage = UHC_CONTROL_STAGE_STATUS;
			}
		}

		break;
	case UVB_REQUEST_DATA:
		if (xfer->stage == UHC_CONTROL_STAGE_STATUS) {
			LOG_DBG("Status stage finished");
			finished = true;
			break;
		}

		if (USB_EP_DIR_IS_OUT(pkt->ep)) {
			length = MIN(buf->len, xfer->mps);
			net_buf_pull(buf, length);
			LOG_DBG("OUT chunk %zu out of %u", length, buf->len);
			if (buf->len == 0) {
				if (pkt->ep == USB_CONTROL_EP_OUT && !xfer->no_status) {
					xfer->stage = UHC_CONTROL_STAGE_STATUS;
				} else {
					finished = true;
				}
			}
		} else {
			length = MIN(net_buf_tailroom(buf), pkt->length);
			net_buf_add(buf, length);
			if (pkt->length > xfer->mps) {
				LOG_ERR("Ambiguous packet with the length %zu",
					pkt->length);
			}

			LOG_DBG("IN chunk %zu out of %zu", length, net_buf_tailroom(buf));
			if (pkt->length < xfer->mps || !net_buf_tailroom(buf)) {
				if (pkt->ep == USB_CONTROL_EP_IN && !xfer->no_status) {
					xfer->stage = UHC_CONTROL_STAGE_STATUS;
				} else {
					finished = true;
				}
			}
		}
		break;
	}

	if (finished) {
		LOG_DBG("Transfer finished");
		uhc_xfer_return(dev, xfer, 0);
		priv->last_xfer = NULL;
	}
}

static void vrt_xfer_drop_active(const struct device *dev, int err)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);

	if (priv->last_pkt != NULL) {
		uvb_free_pkt(priv->last_pkt);
		priv->last_pkt = NULL;
	}

	if (priv->last_xfer) {
		uhc_xfer_return(dev, priv->last_xfer, err);
		priv->last_xfer = NULL;
	}
}

static void vrt_xfer_check_timeout(const struct device *dev)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);

	if (priv->last_pkt == NULL || !sys_timepoint_expired(priv->xfer_timeout)) {
		return;
	}

	LOG_WRN("Transaction on ep 0x%02x timed out",
		priv->last_xfer != NULL ? priv->last_xfer->ep : 0);
	vrt_xfer_drop_active(dev, -ETIMEDOUT);
}

static int vrt_handle_reply(const struct device *dev,
			    struct uvb_packet *const pkt)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);
	struct uhc_vrt_frame *const frame = &priv->frame;
	struct uhc_transfer *const xfer = priv->last_xfer;
	int ret = 0;

	if (priv->last_pkt == NULL) {
		LOG_DBG("Ignore reply for a dropped transfer");
		return 0;
	}

	/* Clear the reference to avoid double free in vrt_xfer_drop_active() */
	priv->last_pkt = NULL;

	if (xfer == NULL) {
		LOG_ERR("No transfers to handle");
		ret = -ENODATA;
		goto handle_reply_err;
	}

	switch (pkt->reply) {
	case UVB_REPLY_NACK:
		/* Move the transfer back to the list. */
		sys_dlist_append(&frame->list, frame->ptr);
		priv->last_xfer = NULL;
		LOG_DBG("NACK 0x%02x count %u", xfer->ep, frame->count);
		break;
	case UVB_REPLY_STALL:
		vrt_xfer_drop_active(dev, -EPIPE);
		break;
	case UVB_REPLY_ACK:
		vrt_hrslt_success(dev, pkt);
		break;
	default:
		vrt_xfer_drop_active(dev, -EINVAL);
		ret = -EINVAL;
		break;
	}

handle_reply_err:
	uvb_free_pkt(pkt);
	return ret;
}

static void vrt_xfer_cleanup_cancelled(const struct device *dev)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);
	struct uhc_data *data = dev->data;
	struct uhc_transfer *tmp;

	if (priv->last_xfer != NULL && priv->last_xfer->err == -ECONNRESET) {
		vrt_xfer_drop_active(dev, -ECONNRESET);
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&data->ctrl_xfers, tmp, node) {
		if (tmp->err == -ECONNRESET) {
			uhc_xfer_return(dev, tmp, -ECONNRESET);
		}
	}
}

static void uhc_vrt_thread_handler(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct uhc_vrt_data *priv = uhc_get_private(dev);

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		struct uhc_vrt_event *ev;
		bool schedule = false;
		int err;

		ev = k_fifo_get(&priv->fifo, K_FOREVER);

		switch (ev->type) {
		case UHC_VRT_EVT_SOF:
			priv->frame_number++;
			err = uvb_advert(priv->host_node, UVB_EVT_SOF,
					 INT_TO_POINTER(priv->frame_number));
			if (unlikely(err)) {
				uhc_submit_event(dev, UHC_EVT_ERROR, err);
			}

			vrt_xfer_cleanup_cancelled(dev);
			vrt_xfer_check_timeout(dev);
			vrt_assemble_frame(dev);
			schedule = true;
			break;
		case UHC_VRT_EVT_REPLY:
			err = vrt_handle_reply(dev, ev->pkt);
			if (unlikely(err)) {
				uhc_submit_event(dev, UHC_EVT_ERROR, err);
			}

			schedule = true;
			break;
		default:
			break;
		}

		if (schedule) {
			err = vrt_schedule_frame(dev);
			if (unlikely(err)) {
				uhc_submit_event(dev, UHC_EVT_ERROR, err);
			}

		}

		k_mem_slab_free(&uhc_vrt_slab, (void *)ev);
	}
}

static void sof_timer_handler(struct k_timer *timer)
{
	struct uhc_vrt_data *priv = CONTAINER_OF(timer, struct uhc_vrt_data, sof_timer);

	vrt_event_submit(priv->dev, UHC_VRT_EVT_SOF, NULL);
}

static void vrt_device_act(const struct device *dev,
			   const enum uvb_device_act act)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);
	enum uhc_event_type type;

	switch (act) {
	case UVB_DEVICE_ACT_RWUP:
		type = UHC_EVT_RWUP;
		break;
	case UVB_DEVICE_ACT_FS:
		type = UHC_EVT_DEV_CONNECTED_FS;
		priv->speed = USB_SPEED_SPEED_FS;
		priv->sof_period = K_MSEC(1);
		k_timer_start(&priv->sof_timer, priv->sof_period, priv->sof_period);
		break;
	case UVB_DEVICE_ACT_HS:
		type = UHC_EVT_DEV_CONNECTED_HS;
		priv->speed = USB_SPEED_SPEED_HS;
		priv->sof_period = K_USEC(125);
		k_timer_start(&priv->sof_timer, priv->sof_period, priv->sof_period);
		break;
	case UVB_DEVICE_ACT_REMOVED:
		type = UHC_EVT_DEV_REMOVED;
		break;
	default:
		type = UHC_EVT_ERROR;
	}

	uhc_submit_event(dev, type, 0);
}

static void uhc_vrt_uvb_cb(const void *const vrt_priv,
			   const enum uvb_event_type type,
			   const void *data)
{
	const struct device *dev = vrt_priv;

	if (type == UVB_EVT_REPLY) {
		vrt_event_submit(dev, UHC_VRT_EVT_REPLY, data);
	} else if (type == UVB_EVT_DEVICE_ACT) {
		vrt_device_act(dev, POINTER_TO_INT(data));
	} else {
		LOG_ERR("Unknown event %d for %p", type, dev);
	}
}

static int uhc_vrt_sof_enable(const struct device *dev)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);

	k_timer_start(&priv->sof_timer, priv->sof_period, priv->sof_period);

	return 0;
}

/* Disable SOF generator and suspend bus */
static int uhc_vrt_bus_suspend(const struct device *dev)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);

	k_timer_stop(&priv->sof_timer);

	return uvb_advert(priv->host_node, UVB_EVT_SUSPEND, NULL);
}

static int uhc_vrt_bus_reset(const struct device *dev)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);
	int ret;

	k_timer_stop(&priv->sof_timer);
	ret = uvb_advert(priv->host_node, UVB_EVT_RESET, NULL);
	/* TDRSTR */
	k_msleep(50);
	k_timer_start(&priv->sof_timer, priv->sof_period, priv->sof_period);

	return ret;
}

static int uhc_vrt_bus_resume(const struct device *dev)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);

	k_timer_start(&priv->sof_timer, priv->sof_period, priv->sof_period);

	return uvb_advert(priv->host_node, UVB_EVT_RESUME, NULL);
}

static int uhc_vrt_enqueue(const struct device *dev,
			   struct uhc_transfer *const xfer)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);

	if (xfer->interval) {
		xfer->start_frame = priv->frame_number + xfer->interval;
		LOG_DBG("New interrupt transfer s.f. %u f.n. %u interval %u",
			xfer->start_frame, priv->frame_number, xfer->interval);
	}

	uhc_xfer_append(dev, xfer);

	return 0;
}

static int uhc_vrt_dequeue(const struct device *dev,
			    struct uhc_transfer *const xfer)
{
	struct uhc_data *data = dev->data;
	struct uhc_transfer *tmp;
	unsigned int key;

	key = irq_lock();

	SYS_DLIST_FOR_EACH_CONTAINER(&data->ctrl_xfers, tmp, node) {
		if (xfer == tmp) {
			tmp->err = -ECONNRESET;
		}
	}

	irq_unlock(key);

	return 0;
}

static int uhc_vrt_init(const struct device *dev)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);

	priv->sof_period = K_MSEC(1);

	return 0;
}

static int uhc_vrt_enable(const struct device *dev)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);

	return uvb_advert(priv->host_node, UVB_EVT_VBUS_READY, NULL);
}

static int uhc_vrt_disable(const struct device *dev)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);

	return uvb_advert(priv->host_node, UVB_EVT_VBUS_REMOVED, NULL);
}

static int uhc_vrt_shutdown(const struct device *dev)
{
	return 0;
}

static int uhc_vrt_lock(const struct device *dev)
{
	return uhc_lock_internal(dev, K_FOREVER);
}

static int uhc_vrt_unlock(const struct device *dev)
{

	return uhc_unlock_internal(dev);
}

static int uhc_vrt_driver_preinit(const struct device *dev)
{
	struct uhc_vrt_data *priv = uhc_get_private(dev);
	const struct uhc_vrt_config *config = dev->config;
	struct uhc_data *data = dev->data;

	priv->dev = dev;
	k_mutex_init(&data->mutex);

	priv->host_node->priv = dev;
	k_fifo_init(&priv->fifo);
	k_timer_init(&priv->sof_timer, sof_timer_handler, NULL);

	k_thread_create(&priv->thread_data, config->thread_stack,
			config->stack_size, uhc_vrt_thread_handler,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_UHC_VIRTUAL_THREAD_PRIORITY),
			K_ESSENTIAL, K_NO_WAIT);
	k_thread_name_set(&priv->thread_data, dev->name);

	LOG_DBG("Virtual UHC pre-initialized");

	return 0;
}

static DEVICE_API(uhc, uhc_vrt_api) = {
	.lock = uhc_vrt_lock,
	.unlock = uhc_vrt_unlock,
	.init = uhc_vrt_init,
	.enable = uhc_vrt_enable,
	.disable = uhc_vrt_disable,
	.shutdown = uhc_vrt_shutdown,

	.bus_reset = uhc_vrt_bus_reset,
	.sof_enable  = uhc_vrt_sof_enable,
	.bus_suspend = uhc_vrt_bus_suspend,
	.bus_resume = uhc_vrt_bus_resume,

	.ep_enqueue = uhc_vrt_enqueue,
	.ep_dequeue = uhc_vrt_dequeue,
};

#define DT_DRV_COMPAT zephyr_uhc_virtual

#define UHC_VRT_DEVICE_DEFINE(n)						\
	K_THREAD_STACK_DEFINE(uhc_vrt_stack_area_##n,				\
			       CONFIG_UHC_VIRTUAL_STACK_SIZE);			\
										\
	UVB_HOST_NODE_DEFINE(uhc_bc_##n,					\
			     DT_NODE_FULL_NAME(DT_DRV_INST(n)),			\
			     uhc_vrt_uvb_cb);					\
										\
	static const struct uhc_vrt_config uhc_vrt_config_##n = {		\
		.thread_stack = uhc_vrt_stack_area_##n,				\
		.stack_size = K_THREAD_STACK_SIZEOF(uhc_vrt_stack_area_##n),	\
	};									\
										\
	static struct uhc_vrt_data uhc_priv_##n = {				\
		.host_node = &uhc_bc_##n,					\
	};									\
										\
	static struct uhc_data uhc_data_##n = {					\
		.priv = &uhc_priv_##n,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, uhc_vrt_driver_preinit, NULL,			\
			      &uhc_data_##n, &uhc_vrt_config_##n,		\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
			      &uhc_vrt_api);

DT_INST_FOREACH_STATUS_OKAY(UHC_VRT_DEVICE_DEFINE)
