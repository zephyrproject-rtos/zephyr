/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bluetooth HCI transport over a USB Bluetooth controller (USB host side).
 *
 * The controller is a Wireless Controller class (0xE0/0x01/0x01) function
 * as defined in Bluetooth Core Specification Vol 4, Part B: HCI commands are
 * class requests on the default control endpoint, HCI events arrive on an
 * interrupt IN endpoint and ACL data uses a pair of bulk endpoints.
 * Synchronous (isochronous endpoint) data is not supported.
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/logging/log.h>

#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"

LOG_MODULE_REGISTER(usbh_bt_hci, CONFIG_USBH_BT_HCI_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_bt_hci_usbh

/* Bluetooth Core Specification Vol 4, Part B, 2.2 */
#define USBH_BT_HCI_CMD_REQTYPE (USB_REQTYPE_TYPE_CLASS << 5)
#define USBH_BT_HCI_CMD_REQUEST 0x00U

/* Completed IN transfers waiting for the RX thread, one per IN endpoint */
#define USBH_BT_HCI_RX_QUEUE_LEN 4

enum usbh_bt_hci_flag {
	/* A controller is bound to this instance */
	USBH_BT_HCI_CONNECTED,
	/* The HCI transport is open */
	USBH_BT_HCI_OPEN,
};

/* Reassembly state of one IN endpoint */
struct usbh_bt_hci_rx {
	/* H:4 packet type delivered by the endpoint */
	uint8_t h4_type;
	/* Pending IN transfer, NULL if none */
	struct uhc_transfer *xfer;
	/* Received bytes of the packet being reassembled */
	size_t len;
	/* Bytes left to discard of a packet too large for frame */
	size_t skip;
	/* Packet being reassembled, header included */
	uint8_t frame[BT_BUF_RX_SIZE];
};

struct usbh_bt_hci_config {
	/* bt_hci_driver_config must be first */
	struct bt_hci_driver_config common;
	k_thread_stack_t *stack;
	size_t stack_size;
};

struct usbh_bt_hci_data {
	/* bt_hci_driver_data must be first */
	struct bt_hci_driver_data common;
	/* Protects udev, the endpoint addresses and the pending transfers */
	struct k_mutex lock;
	struct usb_device *udev;
	uint8_t evt_ep;
	uint8_t acl_in_ep;
	uint8_t acl_out_ep;
	atomic_t flags;
	struct k_sem connected;
	struct k_msgq rx_msgq;
	struct uhc_transfer *rx_msgq_buf[USBH_BT_HCI_RX_QUEUE_LEN];
	struct usbh_bt_hci_rx evt_rx;
	struct usbh_bt_hci_rx acl_rx;
	struct k_thread rx_thread;
};

static bool usbh_bt_hci_evt_is_discardable(const uint8_t *const frame)
{
	const struct bt_hci_evt_hdr *const hdr = (const void *)frame;

	switch (hdr->evt) {
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
	case BT_HCI_EVT_LE_META_EVENT:
		return hdr->len > 0U &&
		       frame[BT_HCI_EVT_HDR_SIZE] == BT_HCI_EVT_LE_ADVERTISING_REPORT;
	default:
		return false;
	}
}

/* Total length of the packet in frame, 0 if the header is not complete yet */
static size_t usbh_bt_hci_frame_len(const struct usbh_bt_hci_rx *const rx)
{
	if (rx->h4_type == BT_HCI_H4_EVT) {
		const struct bt_hci_evt_hdr *const hdr = (const void *)rx->frame;

		if (rx->len < BT_HCI_EVT_HDR_SIZE) {
			return 0;
		}

		return BT_HCI_EVT_HDR_SIZE + hdr->len;
	}

	const struct bt_hci_acl_hdr *const hdr = (const void *)rx->frame;

	if (rx->len < BT_HCI_ACL_HDR_SIZE) {
		return 0;
	}

	return BT_HCI_ACL_HDR_SIZE + sys_le16_to_cpu(hdr->len);
}

static void usbh_bt_hci_deliver(const struct device *dev, struct usbh_bt_hci_rx *const rx,
				const size_t len)
{
	struct usbh_bt_hci_data *const data = dev->data;
	struct net_buf *buf;

	if (rx->h4_type == BT_HCI_H4_EVT) {
		const bool discardable = usbh_bt_hci_evt_is_discardable(rx->frame);

		do {
			buf = bt_buf_get_evt(rx->frame[0], discardable,
					     discardable ? K_NO_WAIT : K_MSEC(100));
		} while (buf == NULL && !discardable &&
			 atomic_test_bit(&data->flags, USBH_BT_HCI_OPEN));
	} else {
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	}

	if (buf == NULL) {
		LOG_WRN("No buffer for H:4 type %u, dropped", rx->h4_type);
		return;
	}

	if (net_buf_tailroom(buf) < len) {
		LOG_ERR("Packet of %zu bytes does not fit, dropped", len);
		net_buf_unref(buf);
		return;
	}

	net_buf_add_mem(buf, rx->frame, len);
	bt_hci_recv(dev, buf);
}

static void usbh_bt_hci_reassemble(const struct device *dev, struct usbh_bt_hci_rx *const rx,
				   const uint8_t *src, size_t src_len)
{
	while (src_len > 0U) {
		size_t frame_len = usbh_bt_hci_frame_len(rx);
		size_t chunk;

		if (rx->skip > 0U) {
			chunk = MIN(rx->skip, src_len);
			rx->skip -= chunk;
			src += chunk;
			src_len -= chunk;
			continue;
		}

		if (frame_len == 0U) {
			/* Collect the header one byte at a time */
			chunk = 1U;
		} else {
			chunk = MIN(frame_len - rx->len, src_len);
		}

		memcpy(&rx->frame[rx->len], src, chunk);
		rx->len += chunk;
		src += chunk;
		src_len -= chunk;

		frame_len = usbh_bt_hci_frame_len(rx);
		if (frame_len > sizeof(rx->frame)) {
			LOG_ERR("Packet of %zu bytes too large, dropped", frame_len);
			rx->skip = frame_len - rx->len;
			rx->len = 0U;
		} else if (frame_len != 0U && rx->len == frame_len) {
			usbh_bt_hci_deliver(dev, rx, frame_len);
			rx->len = 0U;
		}
	}
}

static int usbh_bt_hci_rx_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	const struct device *dev = xfer->priv;
	struct usbh_bt_hci_data *const data = dev->data;

	ARG_UNUSED(udev);

	if (k_msgq_put(&data->rx_msgq, &xfer, K_NO_WAIT) != 0) {
		LOG_ERR("RX queue full");
		net_buf_unref(xfer->buf);
		(void)uhc_xfer_unref(xfer);
	}

	return 0;
}

static int usbh_bt_hci_tx_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	ARG_UNUSED(udev);

	LOG_DBG("TX ep 0x%02x err %d", xfer->ep, xfer->err);
	if (xfer->err != 0) {
		LOG_ERR("Transfer to endpoint 0x%02x failed (%d)", xfer->ep, xfer->err);
	}

	net_buf_unref(xfer->buf);
	(void)uhc_xfer_unref(xfer);

	return 0;
}

/* Must be called with data->lock held */
static int usbh_bt_hci_rx_submit(const struct device *dev, struct usbh_bt_hci_rx *const rx,
				 const uint8_t ep)
{
	struct usbh_bt_hci_data *const data = dev->data;
	const struct usb_ep_descriptor *ep_desc;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	if (rx->xfer != NULL) {
		return 0;
	}

	ep_desc = usbh_desc_get_endpoint(data->udev, ep);
	if (ep_desc == NULL) {
		return -ENODEV;
	}

	/*
	 * One packet per transfer: a transfer only completes on a short
	 * packet, and the controller sends no zero-length packet after an HCI
	 * packet that is a multiple of the packet size.
	 */
	buf = usbh_xfer_buf_alloc(data->udev, sys_le16_to_cpu(ep_desc->wMaxPacketSize));
	if (buf == NULL) {
		return -ENOMEM;
	}

	xfer = usbh_xfer_alloc(data->udev, ep, usbh_bt_hci_rx_cb, (void *)dev);
	if (xfer == NULL) {
		net_buf_unref(buf);
		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(data->udev, xfer, buf);
	if (ret != 0) {
		goto err;
	}

	ret = usbh_xfer_enqueue(data->udev, xfer);
	if (ret != 0) {
		goto err;
	}

	rx->xfer = xfer;

	return 0;

err:
	net_buf_unref(buf);
	(void)uhc_xfer_unref(xfer);

	return ret;
}

/* Must be called with data->lock held */
static void usbh_bt_hci_rx_cancel(struct usbh_bt_hci_data *const data,
				  struct usbh_bt_hci_rx *const rx)
{
	if (rx->xfer != NULL && data->udev != NULL) {
		/* The transfer returns through usbh_bt_hci_rx_cb() */
		if (usbh_xfer_dequeue(data->udev, rx->xfer) != 0) {
			LOG_ERR("Failed to cancel transfer");
		}
	}
}

static void usbh_bt_hci_rx_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct usbh_bt_hci_data *const data = dev->data;
	struct uhc_transfer *xfer;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct usbh_bt_hci_rx *rx;
		uint8_t ep;
		int ret;

		k_msgq_get(&data->rx_msgq, &xfer, K_FOREVER);

		k_mutex_lock(&data->lock, K_FOREVER);

		if (xfer->ep == data->evt_ep) {
			rx = &data->evt_rx;
		} else {
			rx = &data->acl_rx;
		}

		ep = xfer->ep;
		LOG_DBG("ep 0x%02x err %d len %u", ep, xfer->err, xfer->buf->len);

		/* A transfer cancelled by close may return after a new open */
		if (rx->xfer == xfer) {
			rx->xfer = NULL;
		} else {
			rx = NULL;
		}

		if (rx != NULL && atomic_test_bit(&data->flags, USBH_BT_HCI_OPEN)) {
			if (xfer->err == 0) {
				usbh_bt_hci_reassemble(dev, rx, xfer->buf->data, xfer->buf->len);
			} else if (xfer->err != -ECONNRESET) {
				LOG_WRN("Endpoint 0x%02x transfer failed (%d)", ep, xfer->err);
			}
		}

		net_buf_unref(xfer->buf);
		(void)uhc_xfer_unref(xfer);

		if (rx != NULL && atomic_test_bit(&data->flags, USBH_BT_HCI_OPEN) &&
		    atomic_test_bit(&data->flags, USBH_BT_HCI_CONNECTED)) {
			ret = usbh_bt_hci_rx_submit(dev, rx, ep);
			if (ret != 0) {
				LOG_ERR("Failed to resubmit endpoint 0x%02x (%d)", ep, ret);
			}
		}

		k_mutex_unlock(&data->lock);
	}
}

static int usbh_bt_hci_open(const struct device *dev)
{
	struct usbh_bt_hci_data *const data = dev->data;
	int ret;

	if (!atomic_test_bit(&data->flags, USBH_BT_HCI_CONNECTED)) {
		LOG_INF("Waiting for a USB Bluetooth controller");
		(void)k_sem_take(&data->connected, K_MSEC(CONFIG_USBH_BT_HCI_OPEN_TIMEOUT));
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!atomic_test_bit(&data->flags, USBH_BT_HCI_CONNECTED)) {
		ret = -ENODEV;
		goto unlock;
	}

	data->evt_rx.len = 0U;
	data->evt_rx.skip = 0U;
	data->acl_rx.len = 0U;
	data->acl_rx.skip = 0U;
	atomic_set_bit(&data->flags, USBH_BT_HCI_OPEN);

	ret = usbh_bt_hci_rx_submit(dev, &data->evt_rx, data->evt_ep);
	if (ret == 0) {
		ret = usbh_bt_hci_rx_submit(dev, &data->acl_rx, data->acl_in_ep);
	}

	if (ret != 0) {
		LOG_ERR("Failed to start IN transfers (%d)", ret);
		atomic_clear_bit(&data->flags, USBH_BT_HCI_OPEN);
		usbh_bt_hci_rx_cancel(data, &data->evt_rx);
		usbh_bt_hci_rx_cancel(data, &data->acl_rx);
	}

unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int usbh_bt_hci_close(const struct device *dev)
{
	struct usbh_bt_hci_data *const data = dev->data;

	/* Cleared first so that the RX thread stops waiting for a buffer */
	atomic_clear_bit(&data->flags, USBH_BT_HCI_OPEN);

	k_mutex_lock(&data->lock, K_FOREVER);
	usbh_bt_hci_rx_cancel(data, &data->evt_rx);
	usbh_bt_hci_rx_cancel(data, &data->acl_rx);
	k_mutex_unlock(&data->lock);

	return 0;
}

static int usbh_bt_hci_send(const struct device *dev, struct net_buf *buf)
{
	struct usbh_bt_hci_data *const data = dev->data;
	struct uhc_transfer *xfer = NULL;
	struct net_buf *xfer_buf = NULL;
	uint8_t h4_type;
	size_t len;
	uint8_t ep;
	int ret;

	if (buf->len < 1U) {
		return -EINVAL;
	}

	h4_type = buf->data[0];
	len = buf->len - 1U;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!atomic_test_bit(&data->flags, USBH_BT_HCI_CONNECTED)) {
		ret = -ENODEV;
		goto unlock;
	}

	switch (h4_type) {
	case BT_HCI_H4_CMD:
		ep = USB_CONTROL_EP_OUT;
		break;
	case BT_HCI_H4_ACL:
		ep = data->acl_out_ep;
		break;
	default:
		LOG_ERR("Unsupported H:4 type %u", h4_type);
		ret = -EINVAL;
		goto unlock;
	}

	xfer_buf = usbh_xfer_buf_alloc(data->udev, len);
	if (xfer_buf == NULL) {
		ret = -ENOMEM;
		goto unlock;
	}

	net_buf_add_mem(xfer_buf, &buf->data[1], len);

	xfer = usbh_xfer_alloc(data->udev, ep, usbh_bt_hci_tx_cb, (void *)dev);
	if (xfer == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	if (h4_type == BT_HCI_H4_CMD) {
		struct usb_setup_packet *const setup = (void *)xfer->setup_pkt;

		setup->bmRequestType = USBH_BT_HCI_CMD_REQTYPE;
		setup->bRequest = USBH_BT_HCI_CMD_REQUEST;
		setup->wValue = 0U;
		setup->wIndex = 0U;
		setup->wLength = sys_cpu_to_le16(len);
	}

	ret = usbh_xfer_buf_add(data->udev, xfer, xfer_buf);
	if (ret != 0) {
		goto err;
	}

	ret = usbh_xfer_enqueue(data->udev, xfer);
	if (ret != 0) {
		goto err;
	}

	k_mutex_unlock(&data->lock);
	net_buf_unref(buf);

	return 0;

err:
	net_buf_unref(xfer_buf);
	if (xfer != NULL) {
		(void)uhc_xfer_unref(xfer);
	}

unlock:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int usbh_bt_hci_class_init(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static int usbh_bt_hci_class_probe(struct usbh_class_data *const c_data,
				   struct usb_device *const udev, const uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct usbh_bt_hci_data *const data = dev->data;
	const uint8_t ifnum = (iface == USBH_CLASS_IFNUM_DEVICE) ? 0U : iface;
	const struct usb_if_descriptor *if_desc;
	const struct usb_desc_header *desc;
	uint8_t evt_ep = 0U;
	uint8_t acl_in_ep = 0U;
	uint8_t acl_out_ep = 0U;

	if_desc = usbh_desc_get_iface(udev, ifnum);
	if (if_desc == NULL || if_desc->bInterfaceClass != USB_BCC_WIRELESS_CONTROLLER ||
	    if_desc->bInterfaceSubClass != 0x01U || if_desc->bInterfaceProtocol != 0x01U) {
		return -ENOTSUP;
	}

	for (desc = usbh_desc_get_next(if_desc);
	     desc != NULL && desc->bDescriptorType != USB_DESC_INTERFACE;
	     desc = usbh_desc_get_next(desc)) {
		const struct usb_ep_descriptor *const ep_desc = (const void *)desc;
		uint8_t type;

		if (!usbh_desc_is_valid_endpoint(desc)) {
			continue;
		}

		type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

		if (type == USB_EP_TYPE_INTERRUPT && USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
			evt_ep = ep_desc->bEndpointAddress;
		} else if (type == USB_EP_TYPE_BULK) {
			if (USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
				acl_in_ep = ep_desc->bEndpointAddress;
			} else {
				acl_out_ep = ep_desc->bEndpointAddress;
			}
		}
	}

	if (evt_ep == 0U || acl_in_ep == 0U || acl_out_ep == 0U) {
		LOG_ERR("Interface %u lacks the HCI endpoints", ifnum);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (atomic_test_bit(&data->flags, USBH_BT_HCI_CONNECTED)) {
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	data->udev = udev;
	data->evt_ep = evt_ep;
	data->acl_in_ep = acl_in_ep;
	data->acl_out_ep = acl_out_ep;
	atomic_set_bit(&data->flags, USBH_BT_HCI_CONNECTED);

	k_mutex_unlock(&data->lock);

	LOG_INF("Controller %04x:%04x, event 0x%02x, ACL 0x%02x/0x%02x", udev->dev_desc.idVendor,
		udev->dev_desc.idProduct, evt_ep, acl_in_ep, acl_out_ep);

	k_sem_give(&data->connected);

	return 0;
}

static int usbh_bt_hci_class_removed(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_bt_hci_data *const data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* The host controller returns the pending transfers with -ECONNRESET */
	atomic_clear_bit(&data->flags, USBH_BT_HCI_CONNECTED);
	data->udev = NULL;
	k_sem_reset(&data->connected);

	k_mutex_unlock(&data->lock);

	if (atomic_test_bit(&data->flags, USBH_BT_HCI_OPEN)) {
		LOG_WRN("Controller removed while the transport is open");
	} else {
		LOG_INF("Controller removed");
	}

	return 0;
}

static struct usbh_class_api usbh_bt_hci_class_api = {
	.init = usbh_bt_hci_class_init,
	.probe = usbh_bt_hci_class_probe,
	.removed = usbh_bt_hci_class_removed,
};

static const struct usbh_class_filter usbh_bt_hci_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_WIRELESS_CONTROLLER,
		.sub = 0x01,
		.proto = 0x01,
	},
	{0},
};

static DEVICE_API(bt_hci, usbh_bt_hci_api) = {
	.open = usbh_bt_hci_open,
	.close = usbh_bt_hci_close,
	.send = usbh_bt_hci_send,
};

static int usbh_bt_hci_init(const struct device *dev)
{
	const struct usbh_bt_hci_config *const config = dev->config;
	struct usbh_bt_hci_data *const data = dev->data;
	k_tid_t tid;

	k_mutex_init(&data->lock);
	k_sem_init(&data->connected, 0, 1);
	k_msgq_init(&data->rx_msgq, (char *)data->rx_msgq_buf, sizeof(data->rx_msgq_buf[0]),
		    ARRAY_SIZE(data->rx_msgq_buf));
	data->evt_rx.h4_type = BT_HCI_H4_EVT;
	data->acl_rx.h4_type = BT_HCI_H4_ACL;

	tid = k_thread_create(&data->rx_thread, config->stack, config->stack_size,
			      usbh_bt_hci_rx_thread, (void *)dev, NULL, NULL,
			      K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(tid, dev->name);

	return 0;
}

#define USBH_BT_HCI_DEFINE(n)                                                                      \
	static K_KERNEL_STACK_DEFINE(usbh_bt_hci_stack_##n, CONFIG_USBH_BT_HCI_RX_STACK_SIZE);     \
                                                                                                   \
	static const struct usbh_bt_hci_config usbh_bt_hci_config_##n = {                          \
		.common = BT_DT_HCI_DRIVER_CONFIG_INST_GET(n),                                     \
		.stack = usbh_bt_hci_stack_##n,                                                    \
		.stack_size = K_KERNEL_STACK_SIZEOF(usbh_bt_hci_stack_##n),                        \
	};                                                                                         \
                                                                                                   \
	static struct usbh_bt_hci_data usbh_bt_hci_data_##n;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, usbh_bt_hci_init, NULL, &usbh_bt_hci_data_##n,                    \
			      &usbh_bt_hci_config_##n, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &usbh_bt_hci_api);               \
                                                                                                   \
	USBH_DEFINE_CLASS(usbh_bt_hci_c_data_##n, &usbh_bt_hci_class_api,                          \
			  (void *)DEVICE_DT_INST_GET(n), usbh_bt_hci_filters)

DT_INST_FOREACH_STATUS_OKAY(USBH_BT_HCI_DEFINE);
