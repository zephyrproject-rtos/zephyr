/*
 * Copyright (c) 2026 Linumiz
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
	.flags = USBH_CLASS_MATCH_VID_PID,

#define QUECTEL_VENDOR_ID		0x2c7c
#define QUECTEL_PRODUCT_EG916Q		0x6007

LOG_MODULE_REGISTER(usbh_serial, CONFIG_USBH_SERIAL_LOG_LEVEL);

static const struct usb_serial_quirk device_quirks[] = {
	{
		/* quectel EG916Q-GL */
		.vid = 0x2c7c,
		.pid = 0x6007,
		.at_iface = 2,  /* interface 2 is AT commands */
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

#if defined(CONFIG_UART_ASYNC_API)
static inline void usbh_uart_evt_rx_rdy(struct usb_serial_port *port, size_t len)
{
	struct uart_event evt = {
		.type = UART_RX_RDY,
		.data.rx.buf = (uint8_t *)port->cur_buf,
		.data.rx.offset = port->cur_off,
		.data.rx.len = len,
	};

	port->cur_off += len;

	if (port->uart_cb) {
		port->uart_cb(port->uart_dev, &evt, port->uart_cb_user_data);
	}
}

static inline void usbh_uart_evt_rx_disabled(struct usb_serial_port *port)
{
	struct uart_event evt = {.type = UART_RX_DISABLED};

	port->cur_buf = NULL;
	port->cur_len = 0;
	port->cur_off = 0;

	if (port->uart_cb) {
		port->uart_cb(port->uart_dev, &evt, port->uart_cb_user_data);
	}
}

static inline void usbh_uart_evt_rx_buf_request(struct usb_serial_port *port)
{
	struct uart_event evt = {.type = UART_RX_BUF_REQUEST};

	if (port->uart_cb) {
		port->uart_cb(port->uart_dev, &evt, port->uart_cb_user_data);
	}
}

static inline void usbh_uart_evt_rx_buf_release(struct usb_serial_port *port, int buffer_type)
{
	struct uart_event evt = {.type = UART_RX_BUF_RELEASED};

	if (buffer_type == NEXT_BUFFER && !port->next_buf) {
		return;
	}

	if (buffer_type == CURRENT_BUFFER && !port->cur_buf) {
		return;
	}

	if (buffer_type == NEXT_BUFFER) {
		evt.data.rx_buf.buf = port->next_buf;
		port->next_buf = NULL;
		port->next_len = 0;
	} else {
		evt.data.rx_buf.buf = port->cur_buf;
		port->cur_buf = NULL;
		port->cur_len = 0;
		port->cur_off = 0;
	}

	if (port->uart_cb) {
		port->uart_cb(port->uart_dev, &evt, port->uart_cb_user_data);
	}
}
#endif

static int get_endpoints(struct usb_device *udev, uint8_t iface,
			 struct usb_serial_port *port)
{
	const struct usb_desc_header *desc;
	const struct usb_ep_descriptor *ep_desc;

	desc = usbh_desc_get_iface(udev, iface);
	if (desc == NULL) {
		LOG_ERR("No descriptor found for iface : %d", iface);
		return -ENODEV;
	}

	while ((desc = usbh_desc_get_next(desc))) {
		switch (desc->bDescriptorType) {
		case USB_DESC_ENDPOINT:
			ep_desc = (const struct usb_ep_descriptor *)desc;

			if ((ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) ==
			    USB_EP_TYPE_BULK && USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
				port->bulk_in_ep = ep_desc->bEndpointAddress;
				port->bulk_in_mps = ep_desc->wMaxPacketSize;
			} else if ((ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) ==
				   USB_EP_TYPE_BULK &&
				   USB_EP_DIR_IS_OUT(ep_desc->bEndpointAddress)) {
				port->bulk_out_ep = ep_desc->bEndpointAddress;
				port->bulk_out_mps = ep_desc->wMaxPacketSize;

			}

			if (port->bulk_out_ep && port->bulk_in_ep) {
				LOG_INF("Found both endpoints for iface : %d", iface);
				LOG_INF("ep in : 0x%02x, ep out 0x%02x", port->bulk_in_ep,
					port->bulk_out_ep);
				return 0;
			}
		default:
			break;
		}
	}
	return -ENODEV;
}

static int usbh_rx_xfer(struct usb_serial_port *port, size_t req_len)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret = 0;

	xfer = usbh_xfer_alloc(port->udev, port->bulk_in_ep,
			       usbh_rx_cb, port);
	if (!xfer) {
		LOG_ERR("Rx xfer alloc failed !");
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(port->udev, req_len);
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
	}

	return ret;
}

static int usbh_tx_xfer(struct usb_serial_port *port,
			const uint8_t *data, size_t len)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret = 0;

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
	net_buf_add(buf, len);
	ret = usbh_xfer_buf_add(port->udev, xfer, buf);
	if (ret) {
		LOG_ERR("tx buf add err :%d", ret);
		usbh_xfer_buf_free(port->udev, buf);
		usbh_xfer_free(port->udev, xfer);
		return ret;
	}

	ret = usbh_xfer_enqueue(port->udev, xfer);
	if (ret) {
		LOG_ERR("Tx enqueue failed : %d", ret);
		usbh_xfer_buf_free(port->udev, buf);
		usbh_xfer_free(port->udev, xfer);
		return ret;
	}

	return ret;
}

static int usbh_rx_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct usb_serial_port *port = (struct usb_serial_port *)xfer->priv;
	size_t space, b_copied;
	int ret = 0;

	if (xfer->err) {
		LOG_WRN("RX cb error: %d", xfer->err);
		usbh_uart_evt_rx_buf_release(port, CURRENT_BUFFER);
		usbh_uart_evt_rx_buf_release(port, NEXT_BUFFER);
		usbh_uart_evt_rx_disabled(port);
		goto out;
	}

	if (!xfer->buf || xfer->buf->len == 0){
		goto out;
	}

	if ((port->uart_cb) && (!port->cur_buf || port->cur_len == 0 || port->cur_off >= port->cur_len)) {
		goto out;
	}

	space = port->cur_len - port->cur_off;
	b_copied = MIN(space, xfer->buf->len);

	memcpy(port->cur_buf + port->cur_off, xfer->buf->data, b_copied);

	/* mark rx rdy for received data !! */
	usbh_uart_evt_rx_rdy(port, b_copied);

	/* release the current buff */
	usbh_uart_evt_rx_buf_release(port, CURRENT_BUFFER);

	if (!port->next_buf) {
		usbh_uart_evt_rx_disabled(port);
		goto out;
	}

	/* load the next_buf to the cur_buf */
	port->cur_buf = port->next_buf;
	port->cur_len = port->next_len;
	port->cur_off = 0;
	port->next_buf = NULL;
	port->next_len = 0;

	/* request next buf */
	usbh_uart_evt_rx_buf_request(port);

	if (port->in_use && port->udev && port->cur_buf) {
		size_t rem = port->cur_len - port->cur_off;
		size_t req = MIN(rem, port->bulk_in_mps);
		if (req > 0){
			(void)usbh_rx_xfer(port, req);
		}
	}

out:
	if (xfer->buf){
		usbh_xfer_buf_free(port->udev, xfer->buf);
	}
	usbh_xfer_free(port->udev, xfer);

	return ret;
}

static int usbh_tx_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct usb_serial_port *port = (struct usb_serial_port *)xfer->priv;
	struct uart_event evt = {
		.type = xfer->err ? UART_TX_ABORTED : UART_TX_DONE,
		.data.tx.buf = port->tx_buf,
		.data.tx.len = port->tx_len,
	};
	port->tx_buf = NULL;
	port->tx_len = 0;

	if (evt.type == UART_TX_ABORTED) {
		LOG_ERR("TX TRANSFER FAILED: %d", xfer->err);
	} else {
		LOG_INF("TX completed: %d bytes", xfer->buf->len);
	}

	if (port->uart_cb) {
		port->uart_cb(port->uart_dev, &evt, port->uart_cb_user_data);
	}

	usbh_xfer_buf_free(port->udev, xfer->buf);
	usbh_xfer_free(port->udev, xfer);

	return 0;
}

static int usbh_serial_uart_rx_enable(const struct device *dev, uint8_t *buf,
				      size_t len, int32_t timeout)
{
	struct usb_serial_port *port = (struct usb_serial_port *)dev->data;
	size_t space, req_len;
	int ret = 0;

	ARG_UNUSED(timeout);

	if (!buf || len == 0) {
		return -EINVAL;
	}

	if (!port->in_use || !port->udev) {
		return -ENOTCONN;
	}

	if (port->cur_len != 0)
	{
		return -EBUSY;
	}

	port->cur_buf = buf;
	port->cur_len = len;
	port->cur_off = 0;

	usbh_uart_evt_rx_buf_request(port);

	space = port->cur_len - port->cur_off;
	req_len = MIN((size_t)port->bulk_in_mps, space);

	ret = usbh_rx_xfer(port, req_len);
	if (ret) {
		LOG_ERR("Rx queue err : %d", ret);
		port->cur_buf = NULL;
		port->cur_len = 0;
	}

	return ret;
}

static int usbh_serial_uart_rx_disable(const struct device *dev)
{
	struct usb_serial_port *port = (struct usb_serial_port *)dev->data;

	usbh_uart_evt_rx_buf_release(port, CURRENT_BUFFER);
	usbh_uart_evt_rx_buf_release(port, NEXT_BUFFER);
	usbh_uart_evt_rx_disabled(port);

	return 0;
}

static int usbh_serial_uart_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct usb_serial_port *port = (struct usb_serial_port *)dev->data;

	if (!buf || len == 0) {
		return -EINVAL;
	}

	if (port->cur_len == 0 || port->next_len != 0)
	{
		return -EACCES;
	}

	port->next_buf = buf;
	port->next_len = len;

	return 0;
}

static int usbh_serial_uart_tx(const struct device *dev, const uint8_t *buf, size_t len,
			       int32_t timeout)
{
	struct usb_serial_port *port = (struct usb_serial_port *)dev->data;
	int ret = 0;

	k_mutex_lock(&port->lock, K_USEC(timeout));

	if (!buf || len == 0 || len > port->bulk_out_mps) {
		ret = -EINVAL;
		goto unlock;
	}

	if (!port->in_use || !port->udev) {
		ret = -ENODEV;
		goto unlock;
	}

	if (port->tx_len != 0)
	{
		ret = -EBUSY;
		goto unlock;
	}

	port->tx_buf = buf;
	port->tx_len = len;

	ret = usbh_tx_xfer(port, buf, len);
	if (ret < 0) {
		LOG_ERR("Tx queue err : %d", ret);
		port->tx_buf = NULL;
		port->tx_len = 0;
	}
unlock:
	k_mutex_unlock(&port->lock);

	return ret;
}

static int usbh_serial_uart_tx_abort(const struct device *dev)
{
	struct usb_serial_port *port = (struct usb_serial_port *)dev->data;
	struct uart_event event = {.type = UART_TX_ABORTED,
		.data.tx.buf = port->tx_buf,
		.data.tx.len = port->tx_len};
	port->tx_buf = NULL;
	port->tx_len = 0;

	if (port->uart_cb) {
		port->uart_cb(port->uart_dev, &event, port->uart_cb_user_data);
	}
	return 0;
}

static int usbh_serial_uart_cb_set(const struct device *dev,
				   uart_callback_t callback, void *user_data)
{
	struct usb_serial_port *port = (struct usb_serial_port *)dev->data;

	port->uart_cb = callback;
	port->uart_cb_user_data = user_data;
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
	int ret = 0;

	vid = udev->dev_desc.idVendor;
	pid = udev->dev_desc.idProduct;
	quirk = find_device_quirk(vid, pid);
	if (!quirk) {
		LOG_ERR("Device %04x:%04x not in quirk table", vid, pid);
		return -EINVAL;
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

	return ret;
}

static int usbh_serial_removed(struct usbh_class_data *c_data)
{
	struct usb_serial_port *port = c_data->priv;

	if (!port->in_use) {
		return 0;
	}
	usbh_uart_evt_rx_disabled(port);

	port->in_use = false;
	port->udev = NULL;
	port->uart_cb = NULL;
	port->cur_buf = NULL;
	port->next_buf = NULL;
	port->tx_buf = NULL;

	return 0;
}

static int usbh_serial_init(struct usbh_class_data *const c_data,
			    struct usbh_context *const uhs_ctx)
{
	return 0;
}

static int usb_serial_uart_init(const struct device *dev)
{
	struct usb_serial_port *port = dev->data;
	port->uart_dev = dev;

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

static DEVICE_API(uart, usbh_serial_uart_api) = {
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = usbh_serial_uart_cb_set,
	.tx = usbh_serial_uart_tx,
	.tx_abort = usbh_serial_uart_tx_abort,
	.rx_enable = usbh_serial_uart_rx_enable,
	.rx_buf_rsp = usbh_serial_uart_rx_buf_rsp,
	.rx_disable = usbh_serial_uart_rx_disable,
#endif
};

static struct usbh_class_filter usb_serial_filters[] = {
	{ USB_DEVICE_AND_IFACE_INFO(QUECTEL_VENDOR_ID, QUECTEL_PRODUCT_EG916Q,
				    USB_BCC_VENDOR, 0, 0) },
};

#define USBH_SERIAL_DEFINE(n)									\
												\
	static struct usb_serial_port serial_port_##n = {					\
		.lock = Z_MUTEX_INITIALIZER(serial_port_##n.lock),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, usb_serial_uart_init, NULL, &serial_port_##n, NULL,		\
			      POST_KERNEL, CONFIG_USBH_INIT_PRIO, &usbh_serial_uart_api);	\
												\
	USBH_DEFINE_CLASS(usbh_serial, &usbh_serial_class_api, &serial_port_##n,		\
			  usb_serial_filters);							\

DT_INST_FOREACH_STATUS_OKAY(USBH_SERIAL_DEFINE)
