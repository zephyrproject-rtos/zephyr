/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Bluetooth HCI USB transport layer implementation with following
 * endpoint configuration:
 *  - HCI commands through control endpoint (host-to-device only)
 *  - HCI events through interrupt IN endpoint
 *  - ACL data through one bulk IN and one bulk OUT endpoints
 *
 * TODO: revise transfer handling so that Bluetooth net_bufs can be used
 * directly without intermediate copying.
 *
 * Limitations:
 *  - Remote wakeup before IN transfer is not yet supported.
 *  - H4 transport layer is not yet supported
 */

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/net/buf.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_raw.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>
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
#define BT_HCI_EP_MPS_ACL_DATA		0	/* Get maximum supported */
#define BT_HCI_EP_MPS_VOICE		9

#define BT_HCI_EP_INTERVAL_EVENTS	1
#define BT_HCI_EP_INTERVAL_VOICE	3

#define BT_HCI_CLASS_ENABLED		0
#define BT_HCI_ACL_RX_ENGAGED		1

static K_FIFO_DEFINE(bt_hci_rx_queue);
static K_FIFO_DEFINE(bt_hci_tx_queue);

/*
 * Transfers through three endpoints proceed in a synchronous manner,
 * with maximum packet size of high speed bulk endpoint.
 *
 * REVISE: global (bulk, interrupt, iso) specific pools would be more
 * RAM usage efficient.
 */
NET_BUF_POOL_FIXED_DEFINE(bt_hci_ep_pool,
			  3, 512,
			  sizeof(struct udc_buf_info), NULL);
/* HCI RX/TX threads */
static K_KERNEL_STACK_DEFINE(rx_thread_stack, CONFIG_BT_HCI_TX_STACK_SIZE);
static struct k_thread rx_thread_data;
static K_KERNEL_STACK_DEFINE(tx_thread_stack, CONFIG_USBD_BT_HCI_TX_STACK_SIZE);
static struct k_thread tx_thread_data;

struct bt_hci_data {
	struct net_buf *acl_buf;
	uint16_t acl_len;
	struct k_sem sync_sem;
	atomic_t state;
};

/*
 * Make supported device request visible for the stack
 * bRequest 0x00 and bRequest 0xE0.
 */
static const struct usbd_cctx_vendor_req bt_hci_vregs =
	USBD_VENDOR_REQ(0x00, 0xe0);

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

	struct usb_if_descriptor if1_0;
	struct usb_ep_descriptor if1_0_iso_in_ep;
	struct usb_ep_descriptor if1_0_iso_out_ep;
	struct usb_if_descriptor if1_1;
	struct usb_ep_descriptor if1_1_iso_in_ep;
	struct usb_ep_descriptor if1_1_iso_out_ep;

	struct usb_desc_header nil_desc;
} __packed;

static uint8_t bt_hci_get_int_in(struct usbd_class_node *const c_nd)
{
	struct usbd_bt_hci_desc *desc = c_nd->data->desc;

	return desc->if0_int_ep.bEndpointAddress;
}

static uint8_t bt_hci_get_bulk_in(struct usbd_class_node *const c_nd)
{
	struct usbd_bt_hci_desc *desc = c_nd->data->desc;

	return desc->if0_in_ep.bEndpointAddress;
}

static uint8_t bt_hci_get_bulk_out(struct usbd_class_node *const c_nd)
{
	struct usbd_bt_hci_desc *desc = c_nd->data->desc;

	return desc->if0_out_ep.bEndpointAddress;
}

static void bt_hci_update_iad(struct usbd_class_node *const c_nd)
{
	struct usbd_bt_hci_desc *desc = c_nd->data->desc;

	desc->iad.bFirstInterface = desc->if0.bInterfaceNumber;
}

struct net_buf *bt_hci_buf_alloc(const uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&bt_hci_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;

	return buf;
}

static void bt_hci_tx_sync_in(struct usbd_class_node *const c_nd,
			      struct net_buf *const bt_buf, const uint8_t ep)
{
	struct bt_hci_data *hci_data = c_nd->data->priv;
	struct net_buf *buf;

	buf = bt_hci_buf_alloc(ep);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return;
	}

	net_buf_add_mem(buf, bt_buf->data, bt_buf->len);
	usbd_ep_enqueue(c_nd, buf);
	k_sem_take(&hci_data->sync_sem, K_FOREVER);
	net_buf_unref(buf);
}

static void bt_hci_tx_thread(void *p1, void *p2, void *p3)
{
	struct usbd_class_node *const c_nd = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct net_buf *bt_buf;
		uint8_t ep;

		bt_buf = net_buf_get(&bt_hci_tx_queue, K_FOREVER);

		switch (bt_buf_get_type(bt_buf)) {
		case BT_BUF_EVT:
			ep = bt_hci_get_int_in(c_nd);
			break;
		case BT_BUF_ACL_IN:
			ep = bt_hci_get_bulk_in(c_nd);
			break;
		default:
			LOG_ERR("Unknown type %u", bt_buf_get_type(bt_buf));
			continue;
		}


		bt_hci_tx_sync_in(c_nd, bt_buf, ep);
		net_buf_unref(bt_buf);
	}
}

static void bt_hci_rx_thread(void *a, void *b, void *c)
{
	while (true) {
		struct net_buf *buf;
		int err;

		/* FIXME: Do we need a separate thread for bt_send()? */
		buf = net_buf_get(&bt_hci_rx_queue, K_FOREVER);

		err = bt_send(buf);
		if (err) {
			LOG_ERR("Error sending to driver");
			net_buf_unref(buf);
		}
	}
}

static int bt_hci_acl_out_start(struct usbd_class_node *const c_nd)
{
	struct bt_hci_data *hci_data = c_nd->data->priv;
	struct net_buf *buf;
	uint8_t ep;
	int ret;

	if (!atomic_test_bit(&hci_data->state, BT_HCI_CLASS_ENABLED)) {
		return -EPERM;
	}

	if (atomic_test_and_set_bit(&hci_data->state, BT_HCI_ACL_RX_ENGAGED)) {
		return -EBUSY;
	}

	ep = bt_hci_get_bulk_out(c_nd);
	buf = bt_hci_buf_alloc(ep);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbd_ep_enqueue(c_nd, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
	}

	return  ret;
}

static uint16_t hci_pkt_get_len(struct net_buf *const buf,
				const uint8_t *data, const size_t size)
{
	size_t hdr_len = 0;
	uint16_t len = 0;

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_CMD: {
		struct bt_hci_cmd_hdr *cmd_hdr;

		hdr_len = sizeof(*cmd_hdr);
		cmd_hdr = (struct bt_hci_cmd_hdr *)data;
		len = cmd_hdr->param_len + hdr_len;
		break;
	}
	case BT_BUF_ACL_OUT: {
		struct bt_hci_acl_hdr *acl_hdr;

		hdr_len = sizeof(*acl_hdr);
		acl_hdr = (struct bt_hci_acl_hdr *)data;
		len = sys_le16_to_cpu(acl_hdr->len) + hdr_len;
		break;
	}
	case BT_BUF_ISO_OUT: {
		struct bt_hci_iso_hdr *iso_hdr;

		hdr_len = sizeof(*iso_hdr);
		iso_hdr = (struct bt_hci_iso_hdr *)data;
		len = bt_iso_hdr_len(sys_le16_to_cpu(iso_hdr->len)) + hdr_len;
		break;
	}
	default:
		LOG_ERR("Unknown BT buffer type");
		return 0;
	}

	return (size < hdr_len) ? 0 : len;
}

static int bt_hci_acl_out_cb(struct usbd_class_node *const c_nd,
			     struct net_buf *const buf, const int err)
{
	struct bt_hci_data *hci_data = c_nd->data->priv;

	if (err) {
		goto restart_out_transfer;
	}

	if (hci_data->acl_buf == NULL) {
		hci_data->acl_buf = bt_buf_get_tx(BT_BUF_ACL_OUT, K_FOREVER,
						  buf->data, buf->len);
		if (hci_data->acl_buf == NULL) {
			LOG_ERR("Failed to allocate net_buf");
			goto restart_out_transfer;
		}

		hci_data->acl_len = hci_pkt_get_len(hci_data->acl_buf,
						    buf->data,
						    buf->len);
		LOG_DBG("acl_len %u, chunk %u", hci_data->acl_len, buf->len);

		if (hci_data->acl_len == 0) {
			LOG_ERR("Failed to get packet length");
			net_buf_unref(hci_data->acl_buf);
			hci_data->acl_buf = NULL;
		}
	} else {
		if (net_buf_tailroom(hci_data->acl_buf) < buf->len) {
			LOG_ERR("Buffer tailroom too small");
			net_buf_unref(hci_data->acl_buf);
			hci_data->acl_buf = NULL;
			goto restart_out_transfer;
		}

		/*
		 * Take over the next chunk if HCI packet is
		 * larger than USB_MAX_FS_BULK_MPS.
		 */
		net_buf_add_mem(hci_data->acl_buf, buf->data, buf->len);
		LOG_INF("len %u, chunk %u", hci_data->acl_buf->len, buf->len);
	}

	if (hci_data->acl_buf != NULL && hci_data->acl_len == hci_data->acl_buf->len) {
		net_buf_put(&bt_hci_rx_queue, hci_data->acl_buf);
		hci_data->acl_buf = NULL;
		hci_data->acl_len = 0;
	}

restart_out_transfer:
	/* TODO: add function to reset transfer buffer and allow to reuse it */
	net_buf_unref(buf);
	atomic_clear_bit(&hci_data->state, BT_HCI_ACL_RX_ENGAGED);

	return bt_hci_acl_out_start(c_nd);
}

static int bt_hci_request(struct usbd_class_node *const c_nd,
			  struct net_buf *buf, int err)
{
	struct usbd_contex *uds_ctx = c_nd->data->uds_ctx;
	struct bt_hci_data *hci_data = c_nd->data->priv;
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);

	if (bi->ep == bt_hci_get_bulk_out(c_nd)) {
		return bt_hci_acl_out_cb(c_nd, buf, err);
	}

	if (bi->ep == bt_hci_get_bulk_in(c_nd) ||
	    bi->ep == bt_hci_get_int_in(c_nd)) {
		k_sem_give(&hci_data->sync_sem);

		return 0;
	}

	return usbd_ep_buf_free(uds_ctx, buf);
}

static void bt_hci_update(struct usbd_class_node *const c_nd,
			  uint8_t iface, uint8_t alternate)
{
	LOG_DBG("New configuration, interface %u alternate %u",
		iface, alternate);
}

static void bt_hci_enable(struct usbd_class_node *const c_nd)
{
	struct bt_hci_data *hci_data = c_nd->data->priv;

	atomic_set_bit(&hci_data->state, BT_HCI_CLASS_ENABLED);
	LOG_INF("Configuration enabled");

	if (bt_hci_acl_out_start(c_nd)) {
		LOG_ERR("Failed to start ACL OUT transfer");
	}
}

static void bt_hci_disable(struct usbd_class_node *const c_nd)
{
	struct bt_hci_data *hci_data = c_nd->data->priv;

	atomic_clear_bit(&hci_data->state, BT_HCI_CLASS_ENABLED);
	LOG_INF("Configuration disabled");
}

static int bt_hci_ctd(struct usbd_class_node *const c_nd,
		      const struct usb_setup_packet *const setup,
		      const struct net_buf *const buf)
{
	struct net_buf *cmd_buf;

	/* We expect host-to-device class request */
	if (setup->RequestType.type != USB_REQTYPE_TYPE_CLASS) {
		errno = -ENOTSUP;

		return 0;
	}

	LOG_DBG("bmRequestType 0x%02x bRequest 0x%02x",
		setup->bmRequestType, setup->bRequest);

	cmd_buf = bt_buf_get_tx(BT_BUF_CMD, K_NO_WAIT, buf->data, buf->len);
	if (!cmd_buf) {
		LOG_ERR("Cannot get free buffer");

		return -ENOMEM;
	}

	net_buf_put(&bt_hci_rx_queue, cmd_buf);

	return 0;
}

static int bt_hci_init(struct usbd_class_node *const c_nd)
{

	bt_hci_update_iad(c_nd);

	return 0;
}

static struct usbd_class_api bt_hci_api = {
	.request = bt_hci_request,
	.update = bt_hci_update,
	.enable = bt_hci_enable,
	.disable = bt_hci_disable,
	.control_to_dev = bt_hci_ctd,
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
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_MPS_ACL_DATA),	\
		.bInterval = 0,							\
	},									\
										\
	.if0_out_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_ACL_DATA_OUT,			\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(BT_HCI_EP_MPS_ACL_DATA),	\
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
		.wMaxPacketSize = 0,						\
		.bInterval = BT_HCI_EP_INTERVAL_VOICE,				\
	},									\
										\
	.if1_0_iso_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_VOICE_OUT,			\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = 0,						\
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
		.wMaxPacketSize = BT_HCI_EP_MPS_VOICE,				\
		.bInterval = BT_HCI_EP_INTERVAL_VOICE,				\
	},									\
										\
	.if1_1_iso_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = BT_HCI_EP_VOICE_OUT,			\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = BT_HCI_EP_MPS_VOICE,				\
		.bInterval = BT_HCI_EP_INTERVAL_VOICE,				\
	},									\
										\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};

#define BT_HCI_CLASS_DATA_DEFINE(n)						\
	static struct bt_hci_data bt_hci_data_##n = {				\
		.sync_sem = Z_SEM_INITIALIZER(bt_hci_data_##n.sync_sem, 0, 1),	\
	};									\
										\
	static struct usbd_class_data bt_hci_class_##n = {			\
		.desc = (struct usb_desc_header *)&bt_hci_desc_##n,		\
		.priv = &bt_hci_data_##n,					\
		.v_reqs = &bt_hci_vregs,					\
	};									\
										\
	USBD_DEFINE_CLASS(bt_hci_##n, &bt_hci_api, &bt_hci_class_##n);

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
