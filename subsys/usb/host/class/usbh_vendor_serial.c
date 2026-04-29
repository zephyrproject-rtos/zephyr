/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/logging/log.h>

#include "usbh_class.h"
#include "usbh_device.h"
#include "usbh_desc.h"

LOG_MODULE_REGISTER(usbh_vendor_serial, CONFIG_USBH_VENDOR_SERIAL_LOG_LEVEL);

#define BUF_CURRENT		0
#define BUF_NEXT		1

struct vendor_serial_data {
	struct usb_device *udev;
	const struct device *dev;

	uint8_t bulk_in_ep;
	uint8_t bulk_out_ep;
	uint16_t bulk_in_mps;

	uart_callback_t uart_cb;
	void *uart_cb_data;

	uint8_t *cur_buf;
	uint8_t *next_buf;
	size_t cur_len;
	size_t cur_off;
	size_t next_len;

	const uint8_t *tx_buf;
	size_t tx_len;

	struct k_mutex lock;
};

static void notify_uart(struct vendor_serial_data *data,
			struct uart_event *evt)
{
	if (data->uart_cb != NULL) {
		data->uart_cb(data->dev, evt, data->uart_cb_data);
	}
}

static void evt_rx_rdy(struct vendor_serial_data *data, size_t len)
{
	struct uart_event evt = {
		.type = UART_RX_RDY,
		.data.rx.buf = data->cur_buf,
		.data.rx.offset = data->cur_off,
		.data.rx.len = len,
	};

	data->cur_off += len;
	notify_uart(data, &evt);
}

static void evt_rx_disabled(struct vendor_serial_data *data)
{
	struct uart_event evt = {.type = UART_RX_DISABLED};

	data->cur_buf = NULL;
	data->cur_len = 0;
	data->cur_off = 0;
	notify_uart(data, &evt);
}

static void evt_rx_buf_request(struct vendor_serial_data *data)
{
	struct uart_event evt = {.type = UART_RX_BUF_REQUEST};

	notify_uart(data, &evt);
}

static void evt_rx_buf_release(struct vendor_serial_data *data, int which)
{
	struct uart_event evt = {.type = UART_RX_BUF_RELEASED};

	if (which == BUF_NEXT) {
		if (data->next_buf == NULL) {
			return;
		}
		evt.data.rx_buf.buf = data->next_buf;
		data->next_buf = NULL;
		data->next_len = 0;
	} else {
		if (data->cur_buf == NULL) {
			return;
		}
		evt.data.rx_buf.buf = data->cur_buf;
		data->cur_buf = NULL;
		data->cur_len = 0;
		data->cur_off = 0;
	}

	notify_uart(data, &evt);
}

static int rx_cb(struct usb_device *udev, struct uhc_transfer *xfer);
static int tx_cb(struct usb_device *udev, struct uhc_transfer *xfer);

static int submit_rx(struct vendor_serial_data *data, const size_t len)
{
	struct uhc_transfer *xfer;
	struct net_buf *nbuf;
	int ret;

	xfer = usbh_xfer_alloc(data->udev, data->bulk_in_ep, rx_cb, data);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	nbuf = usbh_xfer_buf_alloc(data->udev, len);
	if (nbuf == NULL) {
		ret = -ENOMEM;
		goto free_xfer;
	}

	ret = usbh_xfer_buf_add(data->udev, xfer, nbuf);
	if (ret != 0) {
		goto free_buf;
	}

	ret = usbh_xfer_enqueue(data->udev, xfer);
	if (ret != 0) {
		goto free_buf;
	}

	return 0;

free_buf:
	usbh_xfer_buf_free(data->udev, nbuf);
free_xfer:
	usbh_xfer_free(data->udev, xfer);

	return ret;
}

static int submit_tx(struct vendor_serial_data *data,
		     const uint8_t *const buf, const size_t len)
{
	struct uhc_transfer *xfer;
	struct net_buf *nbuf;
	int ret;

	xfer = usbh_xfer_alloc(data->udev, data->bulk_out_ep, tx_cb, data);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	nbuf = usbh_xfer_buf_alloc(data->udev, len);
	if (nbuf == NULL) {
		ret = -ENOMEM;
		goto free_xfer;
	}

	net_buf_add_mem(nbuf, buf, len);

	ret = usbh_xfer_buf_add(data->udev, xfer, nbuf);
	if (ret != 0) {
		goto free_buf;
	}

	ret = usbh_xfer_enqueue(data->udev, xfer);
	if (ret != 0) {
		goto free_buf;
	}

	return 0;

free_buf:
	usbh_xfer_buf_free(data->udev, nbuf);
free_xfer:
	usbh_xfer_free(data->udev, xfer);

	return ret;
}

static int rx_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct vendor_serial_data *data = xfer->priv;
	size_t space, copied;

	if (xfer->err != 0) {
		LOG_WRN("RX error: %d", xfer->err);
		evt_rx_buf_release(data, BUF_CURRENT);
		evt_rx_buf_release(data, BUF_NEXT);
		evt_rx_disabled(data);
		goto free;
	}

	if (xfer->buf == NULL || xfer->buf->len == 0) {
		goto free;
	}

	if (data->cur_buf == NULL || data->cur_off >= data->cur_len) {
		goto free;
	}

	space = data->cur_len - data->cur_off;
	copied = MIN(space, xfer->buf->len);
	memcpy(data->cur_buf + data->cur_off, xfer->buf->data, copied);

	evt_rx_rdy(data, copied);
	evt_rx_buf_release(data, BUF_CURRENT);

	if (data->next_buf == NULL) {
		evt_rx_disabled(data);
		goto free;
	}

	data->cur_buf = data->next_buf;
	data->cur_len = data->next_len;
	data->cur_off = 0;
	data->next_buf = NULL;
	data->next_len = 0;

	evt_rx_buf_request(data);

	if (data->udev != NULL && data->cur_buf != NULL) {
		size_t rem = data->cur_len - data->cur_off;
		size_t req = MIN(rem, (size_t)data->bulk_in_mps);

		if (req > 0) {
			(void)submit_rx(data, req);
		}
	}

free:
	if (xfer->buf != NULL) {
		usbh_xfer_buf_free(udev, xfer->buf);
	}
	usbh_xfer_free(udev, xfer);

	return 0;
}

static int tx_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct vendor_serial_data *data = xfer->priv;
	struct uart_event evt = {
		.type = xfer->err != 0 ? UART_TX_ABORTED : UART_TX_DONE,
		.data.tx.buf = data->tx_buf,
		.data.tx.len = data->tx_len,
	};

	data->tx_buf = NULL;
	data->tx_len = 0;

	notify_uart(data, &evt);

	if (xfer->buf != NULL) {
		usbh_xfer_buf_free(udev, xfer->buf);
	}
	usbh_xfer_free(udev, xfer);

	return 0;
}

static int find_bulk_endpoints(struct usb_device *udev, uint8_t iface,
			       struct vendor_serial_data *data)
{
	const struct usb_desc_header *desc;
	const struct usb_ep_descriptor *ep;

	desc = usbh_desc_get_iface(udev, iface);
	if (desc == NULL) {
		return -ENODEV;
	}

	while ((desc = usbh_desc_get_next(desc))) {
		if (desc->bDescriptorType == USB_DESC_INTERFACE ||
		    desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
			break;
		}

		if (desc->bDescriptorType != USB_DESC_ENDPOINT) {
			continue;
		}

		ep = (const struct usb_ep_descriptor *)desc;
		if ((ep->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) != USB_EP_TYPE_BULK) {
			continue;
		}

		if (USB_EP_DIR_IS_IN(ep->bEndpointAddress)) {
			data->bulk_in_ep = ep->bEndpointAddress;
			data->bulk_in_mps = ep->wMaxPacketSize;
		} else {
			data->bulk_out_ep = ep->bEndpointAddress;
		}

		if (data->bulk_in_ep != 0 && data->bulk_out_ep != 0) {
			LOG_INF("Interface %u: ep_in 0x%02x ep_out 0x%02x",
				iface, data->bulk_in_ep, data->bulk_out_ep);
			return 0;
		}
	}

	return -ENODEV;
}

/* UART async API */
static int vs_uart_callback_set(const struct device *dev,
				uart_callback_t cb, void *user_data)
{
	struct vendor_serial_data *data = dev->data;

	data->uart_cb = cb;
	data->uart_cb_data = user_data;

	return 0;
}

static int vs_uart_tx(const struct device *dev, const uint8_t *buf,
		      size_t len, int32_t timeout)
{
	struct vendor_serial_data *data = dev->data;
	int ret;

	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, timeout < 0 ? K_FOREVER : K_USEC(timeout));
	if (ret != 0) {
		return -EAGAIN;
	}

	if (data->udev == NULL) {
		ret = -ENODEV;
		goto unlock;
	}

	if (data->tx_buf != NULL) {
		ret = -EBUSY;
		goto unlock;
	}

	data->tx_buf = buf;
	data->tx_len = len;

	ret = submit_tx(data, buf, len);
	if (ret != 0) {
		data->tx_buf = NULL;
		data->tx_len = 0;
	}

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int vs_uart_tx_abort(const struct device *dev)
{
	struct vendor_serial_data *data = dev->data;
	struct uart_event evt = {
		.type = UART_TX_ABORTED,
		.data.tx.buf = data->tx_buf,
		.data.tx.len = data->tx_len,
	};

	data->tx_buf = NULL;
	data->tx_len = 0;
	notify_uart(data, &evt);

	return 0;
}

static int vs_uart_rx_enable(const struct device *dev, uint8_t *buf,
			     size_t len, int32_t timeout)
{
	struct vendor_serial_data *data = dev->data;
	size_t req_len;
	int ret;

	ARG_UNUSED(timeout);

	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	if (data->udev == NULL) {
		return -ENOTCONN;
	}

	if (data->cur_buf != NULL) {
		return -EBUSY;
	}

	data->cur_buf = buf;
	data->cur_len = len;
	data->cur_off = 0;

	evt_rx_buf_request(data);
	req_len = MIN(len, (size_t)data->bulk_in_mps);

	ret = submit_rx(data, req_len);
	if (ret != 0) {
		data->cur_buf = NULL;
		data->cur_len = 0;
	}

	return ret;
}

static int vs_uart_rx_disable(const struct device *dev)
{
	struct vendor_serial_data *data = dev->data;

	evt_rx_buf_release(data, BUF_CURRENT);
	evt_rx_buf_release(data, BUF_NEXT);
	evt_rx_disabled(data);

	return 0;
}

static int vs_uart_rx_buf_rsp(const struct device *dev,
			      uint8_t *buf, size_t len)
{
	struct vendor_serial_data *data = dev->data;

	if (buf == NULL || len == 0) {
		return -EINVAL;
	}

	if (data->cur_buf == NULL || data->next_buf != NULL) {
		return -EBUSY;
	}

	data->next_buf = buf;
	data->next_len = len;

	return 0;
}

static int vs_uart_init(const struct device *dev)
{
	struct vendor_serial_data *data = dev->data;

	data->dev = dev;
	return 0;
}

static DEVICE_API(uart, vendor_serial_uart_api) = {
	.callback_set = vs_uart_callback_set,
	.tx = vs_uart_tx,
	.tx_abort = vs_uart_tx_abort,
	.rx_enable = vs_uart_rx_enable,
	.rx_buf_rsp = vs_uart_rx_buf_rsp,
	.rx_disable = vs_uart_rx_disable,
};

/* USB host class callbacks */

static int vs_class_init(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static int vs_class_probe(struct usbh_class_data *const c_data,
			  struct usb_device *const udev,
			  const uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct vendor_serial_data *data = dev->data;
	int ret;

	if (data->udev != NULL) {
		return -ENOTSUP;
	}

	ret = find_bulk_endpoints(udev, iface, data);
	if (ret != 0) {
		return -ENOTSUP;
	}

	data->udev = udev;

	LOG_INF("Claimed %04x:%04x iface %u", udev->dev_desc.idVendor,
		udev->dev_desc.idProduct, iface);

	return 0;
}

static int vs_class_removed(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct vendor_serial_data *data = dev->data;

	if (data->udev == NULL) {
		return 0;
	}

	if (data->cur_buf != NULL) {
		evt_rx_disabled(data);
	}

	data->udev = NULL;
	data->cur_buf = NULL;
	data->next_buf = NULL;
	data->tx_buf = NULL;
	data->bulk_in_ep = 0;
	data->bulk_out_ep = 0;
	data->bulk_in_mps = 0;

	return 0;
}

static struct usbh_class_api vendor_serial_class_api = {
	.init = vs_class_init,
	.probe = vs_class_probe,
	.removed = vs_class_removed,
};

static struct usbh_class_filter vendor_serial_filters[] = {
	{
		.class = USB_BCC_VENDOR,
		.sub = 0x00,
		.proto = 0x00,
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
	},
	{0},
};

#define USBH_VENDOR_SERIAL_DEFINE(n, _)							\
											\
	static struct vendor_serial_data vs_data_##n = {				\
		.lock = Z_MUTEX_INITIALIZER(vs_data_##n.lock),				\
	};										\
											\
	DEVICE_DEFINE(usbh_vs_##n, "usbh_vs_" #n, vs_uart_init, NULL,			\
		      &vs_data_##n, NULL, POST_KERNEL,					\
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,				\
		      &vendor_serial_uart_api);						\
											\
	USBH_DEFINE_CLASS(usbh_vs_class_##n, &vendor_serial_class_api,			\
			  (void *)DEVICE_GET(usbh_vs_##n),				\
			  vendor_serial_filters)

LISTIFY(CONFIG_USBH_VENDOR_SERIAL_INSTANCES_COUNT, USBH_VENDOR_SERIAL_DEFINE, (;), _)
