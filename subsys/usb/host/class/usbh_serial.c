/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	zephyr_usbh_serial

#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usb_serial.h>
#include <zephyr/drivers/usb/uhc.h>


#include "usbh_class.h"
#include "usbh_device.h"
#include "usbh_desc.h"
#include "usbh_ch9.h"

#define USB_DEVICE_AND_IFACE_INFO(vend, prod, cl, sc, pr) 				\
	.vid = (vend),									\
	.pid = (prod),									\
	.class = (cl),									\
	.sub = (sc),									\
	.proto = (pr),									\
	.flags = USBH_CLASS_MATCH_VID | USBH_CLASS_MATCH_PID | USBH_CLASS_MATCH_CLASS,

#define QUECTEL_VENDOR_ID		0x2c7c
#define QUECTEL_PRODUCT_EG916Q		0x6007

LOG_MODULE_REGISTER(usbh_serial, CONFIG_USBH_SERIAL_LOG_LEVEL);

static const struct usb_serial_quirk device_quirks[] = {
	{
		/* Quectel EG916Q-GL */
		.vid = 0x2c7c,
		.pid = 0x6007,
		.at_iface = 2,  /* ✓ Interface 2 is AT commands */
		.desc = "Quectel EG916Q-GL"
	},
};

static int usbh_rx_cb(struct usb_device *udev, struct uhc_transfer *xfer);
static int usbh_tx_cb(struct usb_device *udev, struct uhc_transfer *xfer);

static const struct usb_serial_quirk *find_device_quirk(uint16_t vid, uint16_t pid)
{
	for (size_t i = 0; i < ARRAY_SIZE(device_quirks); i++) {
		if (device_quirks[i].vid == vid && device_quirks[i].pid == pid) {
			return &device_quirks[i];
		}
	}
	return NULL;
}

static int get_endpoints(struct usb_device *udev, uint8_t iface_num,
			 struct usb_serial_port *port)
{
	const void *cfg_start = usbh_desc_get_cfg_beg(udev);
	const void *cfg_end = usbh_desc_get_cfg_end(udev);
	const struct usb_desc_header *desc = cfg_start;
	bool in_target_iface = false;

	port->bulk_in_ep = 0;
	port->bulk_out_ep = 0;

	while ((desc = usbh_desc_get_next(desc, cfg_end)) != NULL) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE) {
			struct usb_if_descriptor *iface_desc =
				(struct usb_if_descriptor *)desc;

			if (port->bulk_in_ep && port->bulk_out_ep &&
			    iface_desc->bInterfaceNumber != iface_num) {
				return 0;
			}

			in_target_iface = (iface_desc->bInterfaceNumber == iface_num);
		}

		if (!in_target_iface) {
			continue;
		}

		if (desc->bDescriptorType == USB_DESC_ENDPOINT) {
			struct usb_ep_descriptor *ep_desc =
				(struct usb_ep_descriptor *)desc;

			if (ep_desc->bmAttributes == USB_EP_TYPE_BULK) {
				if (USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
					port->bulk_in_ep = ep_desc->bEndpointAddress;
					port->bulk_in_mps = ep_desc->wMaxPacketSize;
				} else {
					port->bulk_out_ep = ep_desc->bEndpointAddress;
					port->bulk_out_mps = ep_desc->wMaxPacketSize;
				}

				if (port->bulk_in_ep && port->bulk_out_ep) {
					LOG_INF("Found both endpoints for interface %u", iface_num);
					return 0;
				}
			}
		}
	}

	if (port->bulk_in_ep == 0 || port->bulk_out_ep == 0) {
		LOG_ERR("Interface %u missing bulk endpoints", iface_num);
		return -ENODEV;
	}

	return 0;
}


static int usbh_rx_xfer(struct usb_serial_port *port)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	xfer = usbh_xfer_alloc(port->udev, port->bulk_in_ep,
			       usbh_rx_cb, port);
	if (!xfer) {
		LOG_ERR("Rx xfer alloc failed !");
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(port->udev, port->bulk_in_mps);
	if (!buf) {
		LOG_ERR("rx buf alloc failed !");
		usbh_xfer_free(port->udev, xfer);
		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(port->udev, xfer, buf);
	if (ret) {
		LOG_ERR("rx buf add err :%d", ret);
		usbh_xfer_buf_free(port->udev, buf);
		usbh_xfer_free(port->udev, xfer);
		return ret;
	}

	ret = usbh_xfer_enqueue(port->udev, xfer);
	if (ret) {
		LOG_ERR("Rx enqueue failed : %d", ret);
		usbh_xfer_buf_free(port->udev, buf);
		usbh_xfer_free(port->udev, xfer);
		return ret;
	}

	return 0;
}

static int usbh_tx_xfer(struct usb_serial_port *port,
			const uint8_t *data, size_t len)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	xfer = usbh_xfer_alloc(port->udev, port->bulk_out_ep, usbh_tx_cb, port);
	if (!xfer) {
		LOG_ERR("Tx xfer alloc failed !");
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(port->udev, len);
	if (!buf) {
		LOG_ERR("tx buf alloc failed !");
		usbh_xfer_free(port->udev, xfer);
		return -ENOMEM;
	}

	memcpy(buf->data, data, len);
	ret = usbh_xfer_buf_add(port->udev, xfer, buf);
	if (ret) {
		LOG_ERR("tx buf add err :%d", ret);
		usbh_xfer_buf_free(port->udev, buf);
		usbh_xfer_free(port->udev, xfer);
		return ret;
	}

	if (k_mutex_lock(&port->port_mutex, K_MSEC(1000)) != 0) {
		usbh_xfer_buf_free(port->udev, buf);
		usbh_xfer_free(port->udev, xfer);
		return -EBUSY;
	}

	ret = usbh_xfer_enqueue(port->udev, xfer);
	if (ret) {
		LOG_ERR("Tx enqueue failed : %d", ret);
		usbh_xfer_buf_free(port->udev, buf);
		usbh_xfer_free(port->udev, xfer);
		return ret;
	}

	k_mutex_unlock(&port->port_mutex);

	if (k_sem_take(&port->sync_sem, K_MSEC(5000)) != 0) {
		return -ETIMEDOUT;
	}

	return len;
}

static int usbh_rx_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct usb_serial_port *port = (struct usb_serial_port *)xfer->priv;
	uint32_t written;
	int ret;

	if (xfer->err) {
		LOG_WRN("RX cb error: %d", xfer->err);
		goto cleanup;
	}

	if (xfer->buf && xfer->buf->len > 0) {
		written = ring_buf_put(&port->ringbuf, xfer->buf->data, xfer->buf->len);

		if (written < xfer->buf->len) {
			LOG_WRN("Ring buffer full! Lost %d bytes", xfer->buf->len - written);
		}

		if (port->rx_notify_cb && written > 0) {
			port->rx_notify_cb(port->rx_notify_user_data);
		}
	}

cleanup:
	usbh_xfer_buf_free(port->udev, xfer->buf);
	usbh_xfer_free(port->udev, xfer);

	if (port->in_use && port->udev) {
		ret = usbh_rx_xfer(port);
		if (ret) {
			LOG_ERR("serial rx init : %d", ret);
			port->in_use = false;
			port->udev = NULL;
			return ret;
		}
	}
	return 0;
}

static int usbh_tx_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct usb_serial_port *port = (struct usb_serial_port *)xfer->priv;

	if (xfer->err) {
		LOG_ERR("TX TRANSFER FAILED: %d", xfer->err);
	} else {
		LOG_INF("TX completed: %d bytes", xfer->buf->len);
	}

	usbh_xfer_buf_free(port->udev, xfer->buf);
	usbh_xfer_free(port->udev, xfer);

	k_sem_give(&port->sync_sem);
	return 0;
}

static int usbh_serial_read(const struct device *dev, uint8_t *buf,
			    size_t max_len)
{
	struct usb_serial_port *port = (struct usb_serial_port *)dev->data;

	if (!buf || max_len == 0) {
		return -EINVAL;
	}

	if (!port->in_use || !port->udev) {
		return -ENOTCONN;
	}

	return ring_buf_get(&port->ringbuf, buf, max_len);
}

static int usbh_serial_write(const struct device *dev, const uint8_t *buf, size_t len)
{
	struct usb_serial_port *port = (struct usb_serial_port *)dev->data;
	int ret;

	if (!buf || len == 0 || len > port->bulk_out_mps) {
		return -EINVAL;
	}

	if (!port->in_use || !port->udev) {
		return -ENODEV;
	}

	ret = usbh_tx_xfer(port, buf, len);
	if (ret < 0) {
		LOG_ERR("Tx queue err : %d", ret);
		return ret;
	}

	return len;
}

static int usbh_serial_set_tx_cb(const struct device *dev,
				 void (*cb)(void *user_data), void *user_data)
{
	struct usb_serial_port *port = (struct usb_serial_port *)dev->data;

	port->tx_notify_cb = cb;
	port->tx_notify_user_data = user_data;
	return 0;
}

static int usbh_serial_set_rx_cb(const struct device *dev,
				 void (*cb)(void *user_data), void *user_data)
{
	struct usb_serial_port *port = (struct usb_serial_port *)dev->data;

	port->rx_notify_cb = cb;
	port->rx_notify_user_data = user_data;
	return 0;
}

static int usbh_serial_probe(struct usbh_class_data *const c_data,
			     struct usb_device *const udev,
			     const uint8_t iface)
{
	struct usb_serial_port *port = c_data->priv;
	const struct usb_serial_quirk *quirk;
	uint16_t vid, pid;
	uint8_t target_iface;
	int ret;

	vid = udev->dev_desc.idVendor;
	pid = udev->dev_desc.idProduct;

	quirk = find_device_quirk(vid, pid);
	if (!quirk) {
		LOG_WRN("Device %04x:%04x not in quirk table using default", vid, pid);
		target_iface = 2;
	} else {
		target_iface = quirk->at_iface;
		LOG_INF("Found %s - AT iface is %u", quirk->desc, target_iface);
	}

	if (port->in_use) {
		LOG_INF("Port Busy, rejecting interface %u", iface);
		return -ENOTSUP;
	}

	ret = get_endpoints(udev, target_iface, port);
	if (ret != 0) {
		LOG_INF("No bulk endpoints found");
		return ret;
	}

	port->udev = udev;
	port->iface_num = target_iface;
	port->in_use = true;

	ret = usbh_rx_xfer(port);
	if (ret) {
		LOG_ERR("serial rx init : %d", ret);
		port->in_use = false;
		port->udev = NULL;
		return ret;
	}

	LOG_INF("USB Serial device probed successfully");
	return 0;
}

static int usbh_serial_removed(struct usbh_class_data *c_data)
{
	struct usb_serial_port *port = c_data->priv;

	if (port->in_use) {
		LOG_INF("Removing USB serial device");
		port->in_use = false;
	}
	return 0;
}

static int usbh_serial_init(struct usbh_class_data *const c_data,
			    struct usbh_context *const uhs_ctx)
{
	struct usb_serial_port *port = (struct usb_serial_port *)c_data->priv;

	k_mutex_init(&port->port_mutex);
	k_sem_init(&port->sync_sem, 0, 1);

	ring_buf_init(&port->ringbuf, RX_RINGBUF_SIZE, port->rx_buf);
	return 0;
}

static struct usbh_class_api usbh_serial_class_api = {
	.init = usbh_serial_init,
	.probe = usbh_serial_probe,
	.removed = usbh_serial_removed,
	.completion_cb = NULL,
	.suspended = NULL,
	.resumed = NULL,
};

static struct usb_serial_api usbh_serial_device_api = {
	.write = usbh_serial_write,
	.read = usbh_serial_read,
	.set_rx_cb = usbh_serial_set_rx_cb,
	.set_tx_cb = usbh_serial_set_tx_cb,
};

static struct usbh_class_filter usb_serial_filters[] = {
	{ USB_DEVICE_AND_IFACE_INFO(QUECTEL_VENDOR_ID, QUECTEL_PRODUCT_EG916Q,
				    USB_BCC_VENDOR, 0, 0) },
};

#define USBH_SERIAL_DEFINE(n)									\
												\
	static struct usb_serial_port serial_port_##n;						\
												\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &serial_port_##n, NULL,				\
			      POST_KERNEL, CONFIG_USBH_INIT_PRIO, &usbh_serial_device_api);	\
												\
	USBH_DEFINE_CLASS(usbh_serial, &usbh_serial_class_api, &serial_port_##n,		\
			  usb_serial_filters, ARRAY_SIZE(usb_serial_filters));			\

DT_INST_FOREACH_STATUS_OKAY(USBH_SERIAL_DEFINE)
