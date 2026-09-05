/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Bluetooth HCI USB transport layer implementation.
 *
 * The implementation supports the optional USB bulk serialization mode. In
 * this mode, only the bulk endpoints are used. If the host supports bulk
 * serialization mode, it can enable it by selecting an alternate setting for
 * the first interface.
 *
 * In the default interface setting, three endpoints are used
 *  - HCI commands through control endpoint (host-to-device only)
 *  - HCI events through interrupt IN endpoint
 *  - ACL data through one bulk IN and one bulk OUT endpoints
 *
 * In the alternate interface setting, two endpoints are used
 *  - HCI commands, ACL OUT, and ISO OUT through bulk OUT endpoint
 *  - HCI events, ACL IN, and ISO IN through bulk IN endpoint
 *
 * Limitations:
 *  - Remote wakeup before IN transfer is not yet supported.
 */

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/net_buf.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_raw.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci, CONFIG_USBD_BT_HCI_LOG_LEVEL);

#define BT_HCI_SUBCLASS		0x01
#define BT_HCI_PROTOCOL		0x01

/*
 * The actual endpoint addresses may differ from the following because
 * the resources may be reallocated by the stack depending on the number
 * of functions in a configuration and the properties of the controller.
 */
#define BT_HCI_EP_EVENTS		0x81
#define BT_HCI_EP_ACL_DATA_IN		0x82
#define BT_HCI_EP_ACL_DATA_OUT		0x02
#define BT_HCI_EP_VOICE_IN		0x83
#define BT_HCI_EP_VOICE_OUT		0x03

#define BT_HCI_EP_MPS_EVENTS		16
#define BT_HCI_EP_FS_MPS_ACL_DATA	64
#define BT_HCI_EP_HS_MPS_ACL_DATA	512
#define BT_HCI_EP_MPS_VOICE		9

#define BT_HCI_EP_INTERVAL_EVENTS	1
#define BT_HCI_EP_INTERVAL_VOICE	3

#define BT_HCI_CLASS_ENABLED		0
#define BT_HCI_BULK_SER_MODE		1

static K_FIFO_DEFINE(bt_hci_rx_queue);
static K_FIFO_DEFINE(bt_hci_tx_queue);

/* This pool is used exclusively for the OUT transfers */
UDC_BUF_POOL_DEFINE(bt_hci_out_ep_pool,
		    CONFIG_USBD_BT_HCI_OUT_BUF_COUNT, USBD_MAX_BULK_MPS,
		    sizeof(struct udc_buf_info), NULL);

/*
 * The TX queue carries Controller-to-Host ACL data and events.
 * In the bulk serialization mode, it also includes ISO data.
 * Size the pool for the largest complete HCI packet.
 */
UDC_BUF_POOL_DEFINE(bt_hci_in_pool,
		    CONFIG_USBD_BT_HCI_IN_BUF_COUNT, BT_BUF_RX_SIZE,
		    sizeof(struct udc_buf_info), NULL);

/* HCI RX/TX threads */
static K_KERNEL_STACK_DEFINE(rx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread rx_thread_data;
static K_KERNEL_STACK_DEFINE(tx_thread_stack, CONFIG_USBD_BT_HCI_TX_STACK_SIZE);
static struct k_thread tx_thread_data;

/*
 * We do not support voice channels and we do not implement
 * isochronous endpoints handling, these are only available to match
 * the recomendet configuration in the Bluetooth specification and
 * avoid issues with Linux kernel btusb driver.
 */
struct usbd_bt_hci_desc {
	struct usb_association_descriptor iad;
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_int_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_ep_descriptor if0_hs_out_ep;

	struct usb_if_descriptor if0_1;
	struct usb_ep_descriptor if0_1_in_ep;
	struct usb_ep_descriptor if0_1_out_ep;
	struct usb_ep_descriptor if0_1_hs_in_ep;
	struct usb_ep_descriptor if0_1_hs_out_ep;

	struct usb_if_descriptor if1_0;
	struct usb_ep_descriptor if1_0_iso_in_ep;
	struct usb_ep_descriptor if1_0_iso_out_ep;
	struct usb_if_descriptor if1_1;
	struct usb_ep_descriptor if1_1_iso_in_ep;
	struct usb_ep_descriptor if1_1_iso_out_ep;

	struct usb_desc_header nil_desc;
};

struct bt_hci_data {
	struct net_buf *out_buf;
	struct usbd_bt_hci_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	uint16_t out_rem;
	atomic_t state;
};

/*
 * Make supported device request visible for the stack
 * bRequest 0x00 and bRequest 0xE0.
 */
static const struct usbd_cctx_vendor_req bt_hci_vregs =
	USBD_VENDOR_REQ(0x00, 0xe0);

static uint8_t bt_hci_get_int_in(struct usbd_class_data *const c_data)
{
	struct bt_hci_data *data = usbd_class_get_private(c_data);
	struct usbd_bt_hci_desc *desc = data->desc;

	return desc->if0_int_ep.bEndpointAddress;
}

static uint8_t bt_hci_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct bt_hci_data *data = usbd_class_get_private(c_data);
	struct usbd_bt_hci_desc *desc = data->desc;

	if (atomic_test_bit(&data->state, BT_HCI_BULK_SER_MODE)) {
		if (USBD_SUPPORTS_HIGH_SPEED &&
		    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
			return desc->if0_1_hs_in_ep.bEndpointAddress;
		}

		return desc->if0_1_in_ep.bEndpointAddress;
	}

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_in_ep.bEndpointAddress;
	}

	return desc->if0_in_ep.bEndpointAddress;
}

static uint8_t bt_hci_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct bt_hci_data *data = usbd_class_get_private(c_data);
	struct usbd_bt_hci_desc *desc = data->desc;

	if (atomic_test_bit(&data->state, BT_HCI_BULK_SER_MODE)) {
		if (USBD_SUPPORTS_HIGH_SPEED &&
		    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
			return desc->if0_1_hs_out_ep.bEndpointAddress;
		}

		return desc->if0_1_out_ep.bEndpointAddress;
	}

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_out_ep.bEndpointAddress;
	}

	return desc->if0_out_ep.bEndpointAddress;
}

static size_t bt_hci_get_bulk_mps(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return BT_HCI_EP_HS_MPS_ACL_DATA;
	}

	return BT_HCI_EP_FS_MPS_ACL_DATA;
}

static void bt_hci_tx_in(struct usbd_class_data *const c_data,
			 struct net_buf *const bt_buf, const uint8_t ep)
{
	struct udc_buf_info *bi;
	struct net_buf *buf;

	buf = net_buf_alloc(&bt_hci_in_pool, K_FOREVER);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return;
	}

	bi = udc_get_buf_info(buf);
	bi->ep = ep;

	net_buf_add_mem(buf, bt_buf->data, bt_buf->len);
	if (usbd_ep_enqueue(c_data, buf)) {
		LOG_ERR("Failed to enqueue transfer");
		net_buf_unref(buf);
	}
}

static void bt_hci_tx_thread(void *p1, void *p2, void *p3)
{
	struct usbd_class_data *const c_data = p1;
	struct bt_hci_data *data = usbd_class_get_private(c_data);

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct net_buf *bt_buf;
		uint8_t type;
		uint8_t ep;

		bt_buf = k_fifo_get(&bt_hci_tx_queue, K_FOREVER);

		if (atomic_test_bit(&data->state, BT_HCI_BULK_SER_MODE)) {
			/*
			 * In bulk serialization mode, HCI packet indicator
			 * should be prefixed to each packet. That is already
			 * the case. No need to pull or replace the type.
			 */
			ep = bt_hci_get_bulk_in(c_data);
			bt_hci_tx_in(c_data, bt_buf, ep);
			net_buf_unref(bt_buf);
			continue;
		}

		/*
		 * In legacy mode, packets are transferred over separate
		 * channels, and HCI packet indicator must be removed.
		 */
		type = net_buf_pull_u8(bt_buf);

		switch (type) {
		case BT_HCI_H4_EVT:
			ep = bt_hci_get_int_in(c_data);
			bt_hci_tx_in(c_data, bt_buf, ep);
			break;
		case BT_HCI_H4_ACL:
			ep = bt_hci_get_bulk_in(c_data);
			bt_hci_tx_in(c_data, bt_buf, ep);
			break;
		default:
			LOG_ERR("Unsupported type %u", type);
			break;
		}

		net_buf_unref(bt_buf);
	}
}

static void bt_hci_rx_thread(void *a, void *b, void *c)
{
	while (true) {
		struct net_buf *buf;
		int err;

		/* FIXME: Do we need a separate thread for bt_send()? */
		buf = k_fifo_get(&bt_hci_rx_queue, K_FOREVER);

		err = bt_send(buf);
		if (err) {
			LOG_ERR("Error sending to driver");
			net_buf_unref(buf);
		}
	}
}

static int bt_hci_acl_out_start(struct usbd_class_data *const c_data)
{
	struct bt_hci_data *hci_data = usbd_class_get_private(c_data);
	struct udc_buf_info *bi;
	struct net_buf *buf;
	int ret;

	if (!atomic_test_bit(&hci_data->state, BT_HCI_CLASS_ENABLED)) {
		return -EPERM;
	}

	while (true) {
		buf = net_buf_alloc(&bt_hci_out_ep_pool, K_NO_WAIT);
		if (!buf) {
			return 0;
		}

		bi = udc_get_buf_info(buf);
		bi->ep = bt_hci_get_bulk_out(c_data);

		/* Shrink the buffer size if operating on a full speed bus */
		buf->size = MIN(bt_hci_get_bulk_mps(c_data), buf->size);
		ret = usbd_ep_enqueue(c_data, buf);
		if (ret) {
			LOG_ERR("Failed to enqueue net_buf for 0x%02x", bi->ep);
			net_buf_unref(buf);
			return ret;
		}
	}

	return 0;
}

static uint16_t hci_pkt_get_len(const uint8_t h4_type,
				const uint8_t *data, const size_t size)
{
	size_t hdr_len = 0;
	size_t len = 0;

	switch (h4_type) {
	case BT_HCI_H4_CMD: {
		struct bt_hci_cmd_hdr *cmd_hdr;

		hdr_len = sizeof(*cmd_hdr);
		if (size >= hdr_len) {
			cmd_hdr = (struct bt_hci_cmd_hdr *)data;
			len = cmd_hdr->param_len + hdr_len;
		}
		break;
	}
	case BT_HCI_H4_ACL: {
		struct bt_hci_acl_hdr *acl_hdr;

		hdr_len = sizeof(*acl_hdr);
		if (size >= hdr_len) {
			acl_hdr = (struct bt_hci_acl_hdr *)data;
			len = sys_le16_to_cpu(acl_hdr->len) + hdr_len;
		}
		break;
	}
	case BT_HCI_H4_ISO: {
		struct bt_hci_iso_hdr *iso_hdr;

		hdr_len = sizeof(*iso_hdr);
		if (size >= hdr_len) {
			iso_hdr = (struct bt_hci_iso_hdr *)data;
			len = bt_iso_hdr_len(sys_le16_to_cpu(iso_hdr->len)) + hdr_len;
		}
		break;
	}
	default:
		LOG_ERR("Unknown H4 buffer type");
		return 0;
	}

	if (len > UINT16_MAX) {
		LOG_ERR("Packet length %zu out of range", len);
		return 0;
	}

	return len;
}

static int bt_hci_acl_out_cb(struct usbd_class_data *const c_data,
			     struct net_buf *const buf, const int err)
{
	struct bt_hci_data *hci_data = usbd_class_get_private(c_data);

	if (err || buf->len == 0) {
		net_buf_drop(&hci_data->out_buf);
		hci_data->out_rem = 0;
		goto restart_out_transfer;
	}

	if (hci_data->out_buf == NULL) {
		enum bt_buf_type type;
		uint8_t h4_type;
		uint16_t len;

		if (hci_data->out_rem != 0) {
			/* Resync to the next HCI packet */
			hci_data->out_rem -= MIN(hci_data->out_rem, buf->len);
			goto restart_out_transfer;
		}

		if (atomic_test_bit(&hci_data->state, BT_HCI_BULK_SER_MODE)) {
			h4_type = net_buf_pull_u8(buf);
		} else {
			h4_type = BT_HCI_H4_ACL;
		}

		len = hci_pkt_get_len(h4_type, buf->data, buf->len);
		if (len == 0 || len < buf->len) {
			goto restart_out_transfer;
		}

		hci_data->out_rem = len - buf->len;
		type = bt_buf_type_from_h4(h4_type, BT_BUF_OUT);
		hci_data->out_buf = bt_buf_get_tx(type, K_FOREVER,
						  buf->data, buf->len);
		if (hci_data->out_buf == NULL) {
			LOG_ERR("Failed to allocate net_buf");
			goto restart_out_transfer;
		}

		LOG_DBG("out_rem %u, chunk %u", hci_data->out_rem, buf->len);
	} else {
		uint16_t len = MIN(buf->len, hci_data->out_rem);

		if (net_buf_tailroom(hci_data->out_buf) < len) {
			LOG_ERR("Buffer tailroom too small");
			net_buf_drop(&hci_data->out_buf);
			goto restart_out_transfer;
		}

		net_buf_add_mem(hci_data->out_buf, buf->data, len);
		hci_data->out_rem -= len;
		LOG_DBG("len %u, chunk %u", hci_data->out_buf->len, len);
	}

	if (hci_data->out_buf != NULL && hci_data->out_rem == 0) {
		k_fifo_put(&bt_hci_rx_queue, hci_data->out_buf);
		hci_data->out_buf = NULL;
	}

restart_out_transfer:
	net_buf_unref(buf);

	return bt_hci_acl_out_start(c_data);
}

static int bt_hci_request(struct usbd_class_data *const c_data,
			  struct net_buf *buf, int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);

	if (err) {
		if (err == -ECONNABORTED) {
			LOG_DBG("ep 0x%02x transfer cancelled", bi->ep);
		} else {
			LOG_ERR("ep 0x%02x transfer failed", bi->ep);
		}
	}

	if (USB_EP_DIR_IS_OUT(bi->ep)) {
		return bt_hci_acl_out_cb(c_data, buf, err);
	}

	return usbd_ep_buf_free(uds_ctx, buf);
}

static void bt_hci_update(struct usbd_class_data *const c_data,
			  uint8_t iface, uint8_t alternate)
{
	struct bt_hci_data *data = usbd_class_get_private(c_data);
	struct usbd_bt_hci_desc *desc = data->desc;
	const uint8_t first_iface = desc->if0.bInterfaceNumber;
	const uint8_t first_alt = desc->if0_1.bAlternateSetting;

	LOG_DBG("New configuration, interface %u alternate %u",
		iface, alternate);

	if (iface == first_iface && alternate == first_alt) {
		LOG_INF("Enable bulk serialization mode");
		atomic_set_bit(&data->state, BT_HCI_BULK_SER_MODE);
	}

	if (iface == first_iface && alternate == 0) {
		LOG_INF("Enable legacy mode");
		atomic_clear_bit(&data->state, BT_HCI_BULK_SER_MODE);
	}

	/*
	 * TODO: Reject alternate setting update of the second interface if
	 * bulk serialization mode is enabled.
	 */
}

static void bt_hci_enable(struct usbd_class_data *const c_data)
{
	struct bt_hci_data *hci_data = usbd_class_get_private(c_data);

	/*
	 * Drop the Bluetooth buffer from the OUT reassembly path if there is
	 * one after the previous enable/disable cycle.
	 */
	net_buf_drop(&hci_data->out_buf);
	hci_data->out_rem = 0;

	atomic_clear_bit(&hci_data->state, BT_HCI_BULK_SER_MODE);
	atomic_set_bit(&hci_data->state, BT_HCI_CLASS_ENABLED);
	LOG_INF("Configuration enabled");

	if (bt_hci_acl_out_start(c_data)) {
		LOG_ERR("Failed to start ACL OUT transfer");
	}
}

static void bt_hci_disable(struct usbd_class_data *const c_data)
{
	struct bt_hci_data *hci_data = usbd_class_get_private(c_data);

	atomic_clear_bit(&hci_data->state, BT_HCI_CLASS_ENABLED);
	LOG_INF("Configuration disabled");
}

static int bt_hci_ctd(struct usbd_class_data *const c_data,
		      const struct usb_setup_packet *const setup,
		      const struct net_buf *const buf)
{
	struct bt_hci_data *data = usbd_class_get_private(c_data);
	struct net_buf *cmd_buf;

	/* We expect host-to-device class request */
	if (setup->RequestType.type != USB_REQTYPE_TYPE_CLASS) {
		return -ENOTSUP;
	}

	if (atomic_test_bit(&data->state, BT_HCI_BULK_SER_MODE)) {
		/* Reject any HCI commands if bulk serialization mode is enabled. */
		return -ENOTSUP;
	}

	if (setup->wLength && (buf == NULL)) {
		/* Data OUT can be received */
		return 0;
	}

	LOG_DBG("bmRequestType 0x%02x bRequest 0x%02x",
		setup->bmRequestType, setup->bRequest);

	cmd_buf = bt_buf_get_tx(BT_BUF_CMD, K_NO_WAIT, buf->data, buf->len);
	if (!cmd_buf) {
		LOG_ERR("Cannot get free buffer");

		return -ENOMEM;
	}

	k_fifo_put(&bt_hci_rx_queue, cmd_buf);

	return 0;
}

static void *bt_hci_get_desc(struct usbd_class_data *const c_data,
			     const enum usbd_speed speed)
{
	struct bt_hci_data *data = usbd_class_get_private(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

static int bt_hci_init(struct usbd_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static struct usbd_class_api bt_hci_api = {
	.request = bt_hci_request,
	.update = bt_hci_update,
	.enable = bt_hci_enable,
	.disable = bt_hci_disable,
	.control_to_dev = bt_hci_ctd,
	.get_desc = bt_hci_get_desc,
	.init = bt_hci_init,
};

#define BT_HCI_DESCRIPTOR_DEFINE(n)						\
static struct usbd_bt_hci_desc bt_hci_desc_##n = {				\
	.iad = {								\
		.bLength = sizeof(struct usb_association_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,			\
		.bFirstInterface = 0,						\
		.bInterfaceCount = 0x02,					\
		.bFunctionClass = USB_BCC_WIRELESS_CONTROLLER,			\
		.bFunctionSubClass = BT_HCI_SUBCLASS,				\
		.bFunctionProtocol = BT_HCI_PROTOCOL,				\
		.iFunction = 0,							\
	},									\
										\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 3,						\
		.bInterfaceClass = USB_BCC_WIRELESS_CONTROLLER,			\
		.bInterfaceSubClass = BT_HCI_SUBCLASS,				\
		.bInterfaceProtocol = BT_HCI_PROTOCOL,				\
		.iInterface = 0,						\
	},									\
										\
	.if0_int_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_EVENTS,				\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_MPS_EVENTS),	\
		.bInterval = BT_HCI_EP_INTERVAL_EVENTS,				\
	},									\
										\
	.if0_in_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_ACL_DATA_IN,			\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_FS_MPS_ACL_DATA),	\
		.bInterval = 0,							\
	},									\
										\
	.if0_out_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_ACL_DATA_OUT,			\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_FS_MPS_ACL_DATA),	\
		.bInterval = 0,							\
	},									\
										\
	.if0_hs_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_ACL_DATA_IN,			\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_HS_MPS_ACL_DATA),	\
		.bInterval = 0,							\
	},									\
										\
	.if0_hs_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_ACL_DATA_OUT,			\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_HS_MPS_ACL_DATA),	\
		.bInterval = 0,							\
	},									\
										\
	.if0_1 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 1,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_WIRELESS_CONTROLLER,			\
		.bInterfaceSubClass = BT_HCI_SUBCLASS,				\
		.bInterfaceProtocol = BT_HCI_PROTOCOL,				\
		.iInterface = 0,						\
	},									\
										\
	.if0_1_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_ACL_DATA_IN,			\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_FS_MPS_ACL_DATA),	\
		.bInterval = 0,							\
	},									\
										\
	.if0_1_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_ACL_DATA_OUT,			\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_FS_MPS_ACL_DATA),	\
		.bInterval = 0,							\
	},									\
										\
	.if0_1_hs_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_ACL_DATA_IN,			\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_HS_MPS_ACL_DATA),	\
		.bInterval = 0,							\
	},									\
										\
	.if0_1_hs_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_ACL_DATA_OUT,			\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_HS_MPS_ACL_DATA),	\
		.bInterval = 0,							\
	},									\
										\
	.if1_0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 1,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_WIRELESS_CONTROLLER,			\
		.bInterfaceSubClass = BT_HCI_SUBCLASS,				\
		.bInterfaceProtocol = BT_HCI_PROTOCOL,				\
		.iInterface = 0,						\
	},									\
										\
	.if1_0_iso_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_VOICE_IN,				\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = sys_cpu_to_le16(0),				\
		.bInterval = BT_HCI_EP_INTERVAL_VOICE,				\
	},									\
										\
	.if1_0_iso_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_VOICE_OUT,			\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = sys_cpu_to_le16(0),				\
		.bInterval = BT_HCI_EP_INTERVAL_VOICE,				\
	},									\
										\
	.if1_1 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 1,						\
		.bAlternateSetting = 1,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_WIRELESS_CONTROLLER,			\
		.bInterfaceSubClass = BT_HCI_SUBCLASS,				\
		.bInterfaceProtocol = BT_HCI_PROTOCOL,				\
		.iInterface = 0,						\
	},									\
										\
	.if1_1_iso_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_VOICE_IN,				\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_MPS_VOICE),		\
		.bInterval = BT_HCI_EP_INTERVAL_VOICE,				\
	},									\
										\
	.if1_1_iso_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_VOICE_OUT,			\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_MPS_VOICE),		\
		.bInterval = BT_HCI_EP_INTERVAL_VOICE,				\
	},									\
										\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};										\
										\
const static struct usb_desc_header *bt_hci_fs_desc_##n[] = {			\
	(struct usb_desc_header *) &bt_hci_desc_##n.iad,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_int_ep,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_in_ep,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_out_ep,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_1,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_1_in_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_1_out_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_0,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_0_iso_in_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_0_iso_out_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_1,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_1_iso_in_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_1_iso_out_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.nil_desc,			\
};										\
										\
const static __maybe_unused struct usb_desc_header *bt_hci_hs_desc_##n[] = {	\
	(struct usb_desc_header *) &bt_hci_desc_##n.iad,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_int_ep,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_hs_in_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_hs_out_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_1,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_1_hs_in_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if0_1_hs_out_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_0,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_0_iso_in_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_0_iso_out_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_1,			\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_1_iso_in_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.if1_1_iso_out_ep,		\
	(struct usb_desc_header *) &bt_hci_desc_##n.nil_desc,			\
};

#define BT_HCI_CLASS_DATA_DEFINE(n)						\
	static struct bt_hci_data bt_hci_data_##n = {				\
		.desc = &bt_hci_desc_##n,					\
		.fs_desc = bt_hci_fs_desc_##n,					\
		.hs_desc = COND_CODE_1(USBD_SUPPORTS_HIGH_SPEED,		\
				       (bt_hci_hs_desc_##n), (NULL)),		\
	};									\
										\
	USBD_DEFINE_CLASS(bt_hci_##n, &bt_hci_api,				\
			  &bt_hci_data_##n, &bt_hci_vregs);

/*
 * Bluetooth subsystem does not support multiple HCI instances,
 * but we are almost ready for it.
 */
BT_HCI_DESCRIPTOR_DEFINE(0)
BT_HCI_CLASS_DATA_DEFINE(0)

static int bt_hci_preinit(void)
{
	int ret;

	ret = bt_enable_raw(&bt_hci_tx_queue);
	if (ret) {
		LOG_ERR("Failed to open Bluetooth raw channel: %d", ret);
		return ret;
	}

	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(rx_thread_stack),
			bt_hci_rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_USBD_BT_HCI_RX_THREAD_PRIORITY),
			0, K_NO_WAIT);

	k_thread_name_set(&rx_thread_data, "bt_hci_rx");

	k_thread_create(&tx_thread_data, tx_thread_stack,
			K_KERNEL_STACK_SIZEOF(tx_thread_stack),
			bt_hci_tx_thread, (void *)&bt_hci_0, NULL, NULL,
			K_PRIO_COOP(CONFIG_USBD_BT_HCI_TX_THREAD_PRIORITY),
			0, K_NO_WAIT);

	k_thread_name_set(&tx_thread_data, "bt_hci_tx");

	return 0;
}

SYS_INIT(bt_hci_preinit, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
