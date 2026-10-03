/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_usbh_vendor_device

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/usbh_vendor.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"

LOG_MODULE_REGISTER(usbh_vendor, CONFIG_USBH_VENDOR_LOG_LEVEL);

#define USBH_VENDOR_SEM_TIMEOUT_MS (1000)

struct usbh_vendor_drv_data;

struct usbh_vendor_ept_data {
	sys_dnode_t node;
	struct usbh_vendor_drv_data *drv_data;
	void *user_ctx;
	uint8_t ept_addr;
	uint8_t alt_id;
	uint8_t type;
	usbh_vendor_callback_t cb;
	bool enabled;
	struct uhc_transfer *xfer;
	struct k_sem complete;
	uint16_t mps;
};

struct usbh_vendor_drv_data {
	/* Mutex */
	struct k_mutex lock;
	/* Device was probed */
	bool probed;
	/* Pointer to udev */
	struct usb_device *udev;
	/* Selected alt_id */
	uint8_t alt_id;
	/* Selected interface */
	uint16_t iface;
	/* Endpoint list */
	sys_dlist_t epts;
};

K_MEM_SLAB_DEFINE_STATIC(usbh_vendor_ept_slab, sizeof(struct usbh_vendor_ept_data),
			 CONFIG_USBH_VENDOR_MAX_EPTS, sizeof(void *));

static struct usbh_vendor_ept_data *usbh_vendor_get_ept_data(struct usbh_vendor_drv_data *drv_data,
							     uint8_t ept_addr, uint8_t alt_id,
							     uint8_t type)
{
	struct usbh_vendor_ept_data *ept_data;

	SYS_DLIST_FOR_EACH_CONTAINER(&drv_data->epts, ept_data, node) {
		if ((ept_data->ept_addr == ept_addr) && (ept_data->alt_id == alt_id) &&
		    (ept_data->type == type)) {
			return ept_data;
		}
	}

	return NULL;
}

static int usbh_vendor_bulk_transfer_cb(struct usb_device *const udev,
					struct uhc_transfer *const xfer)
{
	struct usbh_vendor_ept_data *ept_data = xfer->priv;

	k_sem_give(&ept_data->complete);

	return 0;
}

static int usbh_vendor_bulk_read_api(const struct device *dev, uint8_t ept_id, uint8_t *const buf,
				     size_t buf_len)
{
	struct usbh_vendor_ept_data *ept_data;
	struct usbh_vendor_drv_data *drv_data;
	struct uhc_transfer *xfer = NULL;
	bool dequeue_flow = false;
	int ret = 0;

	if ((dev == NULL) || (buf_len == 0) || (buf == NULL)) {
		return -EINVAL;
	}
	drv_data = dev->data;

	ret = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			ret = -EAGAIN;
			break;
		}

		ept_id = USB_EP_GET_IDX(ept_id);
		ept_data =
			usbh_vendor_get_ept_data(drv_data, USB_EP_GET_ADDR(ept_id, USB_EP_DIR_IN),
						 drv_data->alt_id, USB_EP_TYPE_BULK);
		if (ept_data == NULL) {
			ret = -ENOTSUP;
			break;
		}
		xfer = usbh_xfer_alloc(drv_data->udev, USB_EP_GET_ADDR(ept_id, USB_EP_DIR_IN),
				       usbh_vendor_bulk_transfer_cb, ept_data);
		if (xfer == NULL) {
			ret = -ENOMEM;
			break;
		}
		xfer->buf = usbh_xfer_buf_alloc(drv_data->udev, buf_len);
		if (xfer->buf == NULL) {
			ret = -ENOMEM;
			break;
		}

		k_sem_reset(&ept_data->complete);
		ret = usbh_xfer_enqueue(drv_data->udev, xfer);
		if (ret != 0) {
			break;
		}

		if (k_sem_take(&ept_data->complete, K_MSEC(USBH_VENDOR_SEM_TIMEOUT_MS)) != 0) {
			(void)usbh_xfer_dequeue(drv_data->udev, xfer);
			if (k_sem_take(&ept_data->complete, K_MSEC(USBH_VENDOR_SEM_TIMEOUT_MS)) !=
			    0) {
				dequeue_flow = true;
				ret = -ETIME;
				break;
			}
		}

		if (xfer->err != 0) {
			LOG_ERR("Bulk read transfer failed, err %d", xfer->err);
			ret = -EIO;
			break;
		}

		ret = net_buf_linearize(buf, buf_len, xfer->buf, 0, xfer->buf->len);
	} while (0);

	if (!dequeue_flow) {
		if (xfer != NULL && xfer->buf != NULL) {
			usbh_xfer_buf_free(drv_data->udev, xfer->buf);
		}
		if (xfer != NULL) {
			usbh_xfer_free(drv_data->udev, xfer);
		}
	}

	k_mutex_unlock(&drv_data->lock);

	return ret;
}

static int usbh_vendor_interrupt_cb(struct usb_device *const udev, struct uhc_transfer *const xfer);

static int usbh_vendor_interrupt_prepare_message(struct usbh_vendor_drv_data *drv_data,
						 struct usbh_vendor_ept_data *ept_data)
{
	struct uhc_transfer *xfer = NULL;
	int ret;

	do {
		xfer = usbh_xfer_alloc(drv_data->udev, ept_data->ept_addr, usbh_vendor_interrupt_cb,
				       ept_data);
		if (xfer == NULL) {
			ret = -ENOMEM;
			break;
		}
		xfer->buf = usbh_xfer_buf_alloc(drv_data->udev, ept_data->mps);
		if (xfer->buf == NULL) {
			ret = -ENOMEM;
			break;
		}

		ept_data->xfer = xfer;
		k_sem_reset(&ept_data->complete);
		ret = usbh_xfer_enqueue(drv_data->udev, xfer);
		if (ret != 0) {
			break;
		}
	} while (0);

	if ((ret != 0) && (xfer != NULL) && (xfer->buf != NULL)) {
		usbh_xfer_buf_free(drv_data->udev, xfer->buf);
	}

	if ((ret != 0) && (xfer != NULL)) {
		usbh_xfer_free(drv_data->udev, xfer);
		ept_data->xfer = NULL;
	}

	return ret;
}

static int usbh_vendor_interrupt_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_vendor_ept_data *ept_data = xfer->priv;
	int ret = 0;

	if ((xfer->buf != NULL) && (xfer->err == 0) && (ept_data->enabled)) {
		ept_data->cb(ept_data->user_ctx, USB_EP_GET_IDX(ept_data->ept_addr),
			     xfer->buf->data, xfer->buf->len);
	}

	if (ept_data->enabled && (xfer->err == 0)) {
		net_buf_reset(xfer->buf);
		ret = usbh_xfer_enqueue(udev, xfer);
		if (ret != 0) {
			LOG_ERR("Interrupt re-enqueue failed");
			k_sem_give(&ept_data->complete);
		}
	} else {
		k_sem_give(&ept_data->complete);
	}

	return ret;
}

static int usbh_vendor_interrupt_subscribe_api(const struct device *dev, uint8_t ept_id,
					       usbh_vendor_callback_t cb, void *user_ctx)
{
	struct usbh_vendor_ept_data *ept_data;
	struct usbh_vendor_drv_data *drv_data;
	int ret = 0;

	if (dev == NULL || cb == NULL) {
		return -EINVAL;
	}
	drv_data = dev->data;

	ret = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			ret = -EAGAIN;
			break;
		}

		ept_id = USB_EP_GET_IDX(ept_id);
		ept_data =
			usbh_vendor_get_ept_data(drv_data, USB_EP_GET_ADDR(ept_id, USB_EP_DIR_IN),
						 drv_data->alt_id, USB_EP_TYPE_INTERRUPT);
		if (ept_data == NULL) {
			ret = -ENOTSUP;
			break;
		}
		if (ept_data->enabled == true) {
			ret = -EALREADY;
			break;
		}

		ept_data->user_ctx = user_ctx;
		ept_data->cb = cb;
		ept_data->enabled = true;

		ret = usbh_vendor_interrupt_prepare_message(drv_data, ept_data);
		if (ret != 0) {
			ept_data->enabled = false;
		}
	} while (0);

	k_mutex_unlock(&drv_data->lock);

	return ret;
}

static int usbh_vendor_interrupt_unsubscribe_one(struct usbh_vendor_drv_data *drv_data,
						 struct usbh_vendor_ept_data *ept_data)
{
	bool dequeue_flow = false;
	int ret = 0;

	do {
		if (ept_data->enabled != true) {
			ret = -EALREADY;
			break;
		}

		/* Stop reloading xfer */
		ept_data->enabled = false;
		/* Complete callback to release semaphore was not called, dequeue by force */
		if (k_sem_take(&ept_data->complete, K_MSEC(USBH_VENDOR_SEM_TIMEOUT_MS)) != 0) {
			(void)usbh_xfer_dequeue(drv_data->udev, ept_data->xfer);
			if (k_sem_take(&ept_data->complete, K_MSEC(USBH_VENDOR_SEM_TIMEOUT_MS)) !=
			    0) {
				dequeue_flow = true;
				ret = -ETIME;
				break;
			}
		}

		if (!dequeue_flow) {
			if ((ept_data->xfer != NULL) && (ept_data->xfer->buf != NULL)) {
				usbh_xfer_buf_free(drv_data->udev, ept_data->xfer->buf);
				ept_data->xfer->buf = NULL;
			}

			if (ept_data->xfer != NULL) {
				usbh_xfer_free(drv_data->udev, ept_data->xfer);
				ept_data->xfer = NULL;
			}
		}
	} while (0);

	return ret;
}

static int usbh_vendor_interrupt_unsubscribe_api(const struct device *dev, uint8_t ept_id)
{
	struct usbh_vendor_ept_data *ept_data;
	struct usbh_vendor_drv_data *drv_data;
	int ret = 0;

	if (dev == NULL) {
		return -EINVAL;
	}
	drv_data = dev->data;

	ret = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			ret = -EAGAIN;
			break;
		}

		ept_id = USB_EP_GET_IDX(ept_id);
		ept_data =
			usbh_vendor_get_ept_data(drv_data, USB_EP_GET_ADDR(ept_id, USB_EP_DIR_IN),
						 drv_data->alt_id, USB_EP_TYPE_INTERRUPT);
		if (ept_data == NULL) {
			ret = -ENOTSUP;
			break;
		}
		ret = usbh_vendor_interrupt_unsubscribe_one(drv_data, ept_data);
	} while (0);

	k_mutex_unlock(&drv_data->lock);

	return ret;
}

static int usbh_vendor_bulk_write_api(const struct device *dev, uint8_t ept_id, const uint8_t *buf,
				      size_t buf_len)
{
	struct usbh_vendor_ept_data *ept_data;
	struct usbh_vendor_drv_data *drv_data;
	struct uhc_transfer *xfer = NULL;
	bool dequeue_flow = false;
	int ret = 0;

	if ((dev == NULL) || (buf_len == 0) || (buf == NULL)) {
		return -EINVAL;
	}
	drv_data = dev->data;

	ret = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			ret = -EAGAIN;
			break;
		}

		ept_id = USB_EP_GET_IDX(ept_id);
		ept_data =
			usbh_vendor_get_ept_data(drv_data, USB_EP_GET_ADDR(ept_id, USB_EP_DIR_OUT),
						 drv_data->alt_id, USB_EP_TYPE_BULK);
		if (ept_data == NULL) {
			ret = -ENOTSUP;
			break;
		}
		xfer = usbh_xfer_alloc(drv_data->udev, USB_EP_GET_ADDR(ept_id, USB_EP_DIR_OUT),
				       usbh_vendor_bulk_transfer_cb, ept_data);
		if (xfer == NULL) {
			ret = -ENOMEM;
			break;
		}
		xfer->buf = usbh_xfer_buf_alloc(drv_data->udev, buf_len);
		if (xfer->buf == NULL) {
			ret = -ENOMEM;
			break;
		}

		net_buf_add_mem(xfer->buf, buf, buf_len);

		k_sem_reset(&ept_data->complete);
		ret = usbh_xfer_enqueue(drv_data->udev, xfer);
		if (ret != 0) {
			break;
		}

		if (k_sem_take(&ept_data->complete, K_MSEC(USBH_VENDOR_SEM_TIMEOUT_MS)) != 0) {
			(void)usbh_xfer_dequeue(drv_data->udev, xfer);
			if (k_sem_take(&ept_data->complete, K_MSEC(USBH_VENDOR_SEM_TIMEOUT_MS)) !=
			    0) {
				dequeue_flow = true;
				ret = -ETIME;
				break;
			}
		}

		if (xfer->err != 0) {
			LOG_ERR("Bulk write transfer failed, err %d", xfer->err);
			ret = -EIO;
			break;
		}

	} while (0);

	if (!dequeue_flow) {
		if (xfer != NULL && xfer->buf != NULL) {
			usbh_xfer_buf_free(drv_data->udev, xfer->buf);
		}
		if (xfer != NULL) {
			usbh_xfer_free(drv_data->udev, xfer);
		}
	}

	k_mutex_unlock(&drv_data->lock);

	return ret;
}

static int usbh_vendor_interrupt_write_api(const struct device *dev, uint8_t ept_id,
					   const uint8_t *buf, size_t buf_len)
{
	struct usbh_vendor_ept_data *ept_data;
	struct usbh_vendor_drv_data *drv_data;
	struct uhc_transfer *xfer = NULL;
	bool dequeue_flow = false;
	int ret = 0;

	if ((dev == NULL) || (buf_len == 0) || (buf == NULL)) {
		return -EINVAL;
	}
	drv_data = dev->data;

	ret = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (ret) {
		return ret;
	}

	do {
		if (!drv_data->probed) {
			LOG_DBG("Driver was not probed yet");
			ret = -EAGAIN;
			break;
		}

		ept_id = USB_EP_GET_IDX(ept_id);
		ept_data =
			usbh_vendor_get_ept_data(drv_data, USB_EP_GET_ADDR(ept_id, USB_EP_DIR_OUT),
						 drv_data->alt_id, USB_EP_TYPE_INTERRUPT);
		if (ept_data == NULL) {
			ret = -ENOTSUP;
			break;
		}
		/* buffer length should not exceed endpoint buffer */
		if (buf_len > ept_data->mps) {
			ret = -EMSGSIZE;
			break;
		}

		xfer = usbh_xfer_alloc(drv_data->udev, USB_EP_GET_ADDR(ept_id, USB_EP_DIR_OUT),
				       usbh_vendor_bulk_transfer_cb, ept_data);
		if (xfer == NULL) {
			ret = -ENOMEM;
			break;
		}
		xfer->buf = usbh_xfer_buf_alloc(drv_data->udev, buf_len);
		if (xfer->buf == NULL) {
			ret = -ENOMEM;
			break;
		}

		net_buf_add_mem(xfer->buf, buf, buf_len);

		k_sem_reset(&ept_data->complete);
		ret = usbh_xfer_enqueue(drv_data->udev, xfer);
		if (ret != 0) {
			break;
		}

		if (k_sem_take(&ept_data->complete, K_MSEC(USBH_VENDOR_SEM_TIMEOUT_MS)) != 0) {
			(void)usbh_xfer_dequeue(drv_data->udev, xfer);
			if (k_sem_take(&ept_data->complete, K_MSEC(USBH_VENDOR_SEM_TIMEOUT_MS)) !=
			    0) {
				dequeue_flow = true;
				ret = -ETIME;
				break;
			}
		}

		if (xfer->err != 0) {
			LOG_ERR("Interrupt write transfer failed, err %d", xfer->err);
			ret = -EIO;
			break;
		}

	} while (0);

	if (!dequeue_flow) {
		if (xfer != NULL && xfer->buf != NULL) {
			usbh_xfer_buf_free(drv_data->udev, xfer->buf);
		}
		if (xfer != NULL) {
			usbh_xfer_free(drv_data->udev, xfer);
		}
	}

	k_mutex_unlock(&drv_data->lock);

	return ret;
}

static int usbh_vendor_class_init(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_vendor_drv_data *const drv_data = dev->data;

	k_mutex_init(&drv_data->lock);
	sys_dlist_init(&drv_data->epts);
	drv_data->probed = false;

	return 0;
}

static int usbh_vendor_alloc_ept_data(struct usbh_vendor_drv_data *const drv_data, uint8_t ept_addr,
				      uint8_t alt_id, uint8_t type, uint16_t mps)
{
	struct usbh_vendor_ept_data *ept_data;
	int ret = 0;

	if (usbh_vendor_get_ept_data(drv_data, ept_addr, alt_id, type) != NULL) {
		return -EALREADY;
	}

	ret = k_mem_slab_alloc(&usbh_vendor_ept_slab, (void **)&ept_data, K_NO_WAIT);
	if (ret != 0) {
		return ret;
	}
	memset(ept_data, 0, sizeof(*ept_data));
	ept_data->mps = mps;
	ept_data->drv_data = drv_data;
	ept_data->ept_addr = ept_addr;
	ept_data->alt_id = alt_id;
	ept_data->type = type;
	k_sem_init(&ept_data->complete, 0, 1);

	sys_dlist_append(&drv_data->epts, &ept_data->node);

	return 0;
}

static void usbh_vendor_cleanup_ept_data(struct usbh_vendor_drv_data *drv_data)
{
	struct usbh_vendor_ept_data *ept_data, *tmp_ept_data;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&drv_data->epts, ept_data, tmp_ept_data, node) {
		sys_dlist_remove(&ept_data->node);
		k_mem_slab_free(&usbh_vendor_ept_slab, ept_data);
	}
}

static void usbh_vendor_unsubscribe_all(struct usbh_vendor_drv_data *drv_data)
{
	struct usbh_vendor_ept_data *ept_data, *tmp_ept_data;

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&drv_data->epts, ept_data, tmp_ept_data, node) {
		if ((ept_data->type == USB_EP_TYPE_INTERRUPT) &&
		    (USB_EP_DIR_IS_IN(ept_data->ept_addr))) {
			usbh_vendor_interrupt_unsubscribe_one(drv_data, ept_data);
		}
	}
}

static int usbh_vendor_parse_desc(struct usbh_class_data *const c_data,
				  struct usb_device *const udev, uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct usbh_vendor_drv_data *const drv_data = dev->data;
	const struct usb_desc_header *desc;
	struct usb_ep_descriptor *ep_desc = NULL;
	bool in_interface = false;
	struct usb_if_descriptor *if_desc;
	uint32_t ept_type;
	uint16_t mps;
	uint8_t alt_id = 0;
	int ret = 0;

	desc = udev->cfg_desc;
	if ((desc == NULL) || (desc->bDescriptorType != USB_DESC_CONFIGURATION)) {
		return -EFAULT;
	}

	desc = usbh_desc_get_next(desc);
	while (desc != NULL) {
		if (usbh_desc_is_valid(desc, sizeof(struct usb_if_descriptor),
				       USB_DESC_INTERFACE)) {
			if_desc = ((struct usb_if_descriptor *)desc);
			if ((if_desc->bInterfaceClass == USB_BCC_VENDOR) &&
			    (if_desc->bInterfaceSubClass == 0xFF) &&
			    (if_desc->bInterfaceProtocol == 0xFF) &&
			    (if_desc->bInterfaceNumber == iface) &&
			    /* Only ALT0 is supported for now, the code is prepared for multiple
			     * ALTs
			     */
			    (if_desc->bAlternateSetting == 0)) {
				in_interface = true;
				alt_id = if_desc->bAlternateSetting;
			} else {
				in_interface = false;
			}
		} else if (usbh_desc_is_valid(desc, sizeof(struct usb_ep_descriptor),
					      USB_DESC_ENDPOINT) &&
			   in_interface) {
			ep_desc = (struct usb_ep_descriptor *)desc;
			ept_type = (ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK);

			mps = (sys_le16_to_cpu(ep_desc->wMaxPacketSize) & 0x7FF);
			if (ept_type == USB_EP_TYPE_BULK || ept_type == USB_EP_TYPE_INTERRUPT) {
				ret = usbh_vendor_alloc_ept_data(
					drv_data, ep_desc->bEndpointAddress, alt_id, ept_type, mps);
				if (ret != 0) {
					break;
				}
			}
		}
		desc = usbh_desc_get_next(desc);
	}

	if (ret != 0) {
		usbh_vendor_cleanup_ept_data(drv_data);
	}

	return ret;
}

static int usbh_vendor_class_probe(struct usbh_class_data *const c_data,
				   struct usb_device *const udev, uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct usbh_vendor_drv_data *drv_data = (void *)dev->data;
	int result = 0;

	LOG_DBG("Vendor device was attached");

	if ((udev == NULL) || (udev->state != USB_STATE_CONFIGURED)) {
		LOG_ERR("USB device not properly configured");
		return -ENODEV;
	}

	if (drv_data == NULL) {
		LOG_ERR("No Vendor device instance is available");
		return -ENODEV;
	}

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	drv_data->udev = udev;
	drv_data->iface = iface;
	drv_data->alt_id = 0;

	result = usbh_vendor_parse_desc(c_data, udev, iface);
	if (result == 0) {
		drv_data->probed = true;
	}

	k_mutex_unlock(&drv_data->lock);

	return result;
}

static int usbh_vendor_class_removed(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_vendor_drv_data *drv_data = (void *)dev->data;
	int result = 0;

	result = k_mutex_lock(&drv_data->lock, K_FOREVER);
	if (result) {
		return result;
	}

	usbh_vendor_unsubscribe_all(drv_data);
	usbh_vendor_cleanup_ept_data(drv_data);

	drv_data->udev = NULL;
	drv_data->probed = false;
	k_mutex_unlock(&drv_data->lock);

	LOG_DBG("Vendor device was removed");

	return result;
}

static __maybe_unused DEVICE_API(usbh_vendor, usbh_vendor_api) = {
	.bulk_read = usbh_vendor_bulk_read_api,
	.bulk_write = usbh_vendor_bulk_write_api,
	.interrupt_write = usbh_vendor_interrupt_write_api,
	.interrupt_subscribe = usbh_vendor_interrupt_subscribe_api,
	.interrupt_unsubscribe = usbh_vendor_interrupt_unsubscribe_api,
};

static __maybe_unused struct usbh_class_api usbh_vendor_class_api = {
	.init = usbh_vendor_class_init,
	.probe = usbh_vendor_class_probe,
	.removed = usbh_vendor_class_removed,
};

static __maybe_unused struct usbh_class_filter usbh_vendor_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_VENDOR,
		.sub = 0xFF,
		.proto = 0xFF,
	},
	{0},
};

#define USBH_VENDOR_DEVICE_DEFINE(n)                                                               \
	static struct usbh_vendor_drv_data usbh_vendor_drv_data##n;                                \
	COND_CODE_0(DT_INST_PROP(n, match_class),                                               \
	(static struct usbh_class_filter const usbh_vendor_vid_pid_filters_##n[] = {            \
		{                                                                               \
			.flags = USBH_CLASS_MATCH_VID_PID,                                      \
			.vid = (DT_INST_REG_ADDR(n) >> 16u) & 0xFFFFu,                          \
			.pid = DT_INST_REG_ADDR(n) & 0xFFFFu,                                   \
		},                                                                              \
		{0u}                                                                            \
	};), ())  \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &usbh_vendor_drv_data##n, NULL, POST_KERNEL,          \
			      CONFIG_USBH_VENDOR_INIT_PRIORITY, &usbh_vendor_api);                 \
	USBH_DEFINE_CLASS(                                                                         \
		vendor_host_c_data_##n, &usbh_vendor_class_api, (void *)DEVICE_DT_INST_GET(n),     \
		COND_CODE_1(DT_INST_PROP(n, match_class),                                       \
				(usbh_vendor_filters), (usbh_vendor_vid_pid_filters_##n)));

DT_INST_FOREACH_STATUS_OKAY(USBH_VENDOR_DEVICE_DEFINE)
