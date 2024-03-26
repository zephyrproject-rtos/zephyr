/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_loopback, CONFIG_USBD_LOOPBACK_LOG_LEVEL);

/*
 * NOTE: this class is experimental and is in development.
 * Primary purpose currently is testing of the class initialization and
 * interface and endpoint configuration.
 */

/* Internal buffer for intermediate test data */
static uint8_t lb_buf[1024];

#define LB_VENDOR_REQ_OUT		0x5b
#define LB_VENDOR_REQ_IN		0x5c

#define LB_ISO_EP_MPS			256
#define LB_ISO_EP_INTERVAL		1

/* Make supported vendor request visible for the device stack */
static const struct usbd_cctx_vendor_req lb_vregs =
	USBD_VENDOR_REQ(LB_VENDOR_REQ_OUT, LB_VENDOR_REQ_IN);

struct loopback_desc {
	struct usb_association_descriptor iad;
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_if_descriptor if1;
	struct usb_ep_descriptor if1_int_out_ep;
	struct usb_ep_descriptor if1_int_in_ep;
	struct usb_if_descriptor if2_0;
	struct usb_ep_descriptor if2_0_iso_in_ep;
	struct usb_ep_descriptor if2_0_iso_out_ep;
	struct usb_if_descriptor if2_1;
	struct usb_ep_descriptor if2_1_iso_in_ep;
	struct usb_ep_descriptor if2_1_iso_out_ep;
	struct usb_desc_header nil_desc;
};

struct lb_data {
	struct loopback_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	atomic_t state;
};

static void lb_update(struct usbd_class_node *c_nd,
		      uint8_t iface, uint8_t alternate)
{
	LOG_DBG("Instance %p, interface %u alternate %u changed",
		c_nd, iface, alternate);
}

static int lb_control_to_host(struct usbd_class_node *c_nd,
			      const struct usb_setup_packet *const setup,
			      struct net_buf *const buf)
{
	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_DEVICE) {
		errno = -ENOTSUP;
		return 0;
	}

	if (setup->bRequest == LB_VENDOR_REQ_IN) {
		net_buf_add_mem(buf, lb_buf,
				MIN(sizeof(lb_buf), setup->wLength));

		LOG_WRN("Device-to-Host, wLength %u | %zu", setup->wLength,
			MIN(sizeof(lb_buf), setup->wLength));

		return 0;
	}

	LOG_ERR("Class request 0x%x not supported", setup->bRequest);
	errno = -ENOTSUP;

	return 0;
}

static int lb_control_to_dev(struct usbd_class_node *c_nd,
			     const struct usb_setup_packet *const setup,
			     const struct net_buf *const buf)
{
	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_DEVICE) {
		errno = -ENOTSUP;
		return 0;
	}

	if (setup->bRequest == LB_VENDOR_REQ_OUT) {
		LOG_WRN("Host-to-Device, wLength %u | %zu", setup->wLength,
			MIN(sizeof(lb_buf), buf->len));
		memcpy(lb_buf, buf->data, MIN(sizeof(lb_buf), buf->len));
		return 0;
	}

	LOG_ERR("Class request 0x%x not supported", setup->bRequest);
	errno = -ENOTSUP;

	return 0;
}

static int lb_request_handler(struct usbd_class_node *c_nd,
			      struct net_buf *buf, int err)
{
	struct usbd_contex *uds_ctx = usbd_class_get_ctx(c_nd);
	struct udc_buf_info *bi = NULL;

	bi = (struct udc_buf_info *)net_buf_user_data(buf);
	LOG_DBG("%p -> ep 0x%02x, len %u, err %d", c_nd, bi->ep, buf->len, err);

	return usbd_ep_buf_free(uds_ctx, buf);
}

static void *lb_get_desc(struct usbd_class_node *const c_nd,
			 const enum usbd_speed speed)
{
	struct lb_data *data = usbd_class_get_private(c_nd);

	if (speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

static int lb_init(struct usbd_class_node *c_nd)
{
	struct lb_data *data = usbd_class_get_private(c_nd);
	struct loopback_desc *desc = data->desc;

	desc->iad.bFirstInterface = desc->if0.bInterfaceNumber;

	LOG_DBG("Init class instance %p", c_nd);

	return 0;
}

struct usbd_class_api lb_api = {
	.update = lb_update,
	.control_to_host = lb_control_to_host,
	.control_to_dev = lb_control_to_dev,
	.request = lb_request_handler,
	.get_desc = lb_get_desc,
	.init = lb_init,
};

#define DEFINE_LOOPBACK_DESCRIPTOR(x, _)					\
static struct loopback_desc lb_desc_##x = {					\
	.iad = {								\
		.bLength = sizeof(struct usb_association_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,			\
		.bFirstInterface = 0,						\
		.bInterfaceCount = 3,						\
		.bFunctionClass = USB_BCC_VENDOR,				\
		.bFunctionSubClass = 0,						\
		.bFunctionProtocol = 0,						\
		.iFunction = 0,							\
	},									\
										\
	/* Interface descriptor 0 */						\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_VENDOR,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	/* Data Endpoint OUT */							\
	.if0_out_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Data Endpoint IN */							\
	.if0_in_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Data Endpoint OUT */							\
	.if0_hs_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Data Endpoint IN */							\
	.if0_hs_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Interface descriptor 1 */						\
	.if1 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 1,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_VENDOR,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	/* Interface Interrupt Endpoint OUT */					\
	.if1_int_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x02,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = sys_cpu_to_le16(64),				\
		.bInterval = 0x01,						\
	},									\
										\
	/* Interrupt Interrupt Endpoint IN */					\
	.if1_int_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x82,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = sys_cpu_to_le16(64),				\
		.bInterval = 0x01,						\
	},									\
										\
	.if2_0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 2,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_VENDOR,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if2_0_iso_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x83,					\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = 0,						\
		.bInterval = LB_ISO_EP_INTERVAL,				\
	},									\
										\
	.if2_0_iso_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x03,					\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = 0,						\
		.bInterval = LB_ISO_EP_INTERVAL,				\
	},									\
										\
	.if2_1 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 2,						\
		.bAlternateSetting = 1,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_VENDOR,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if2_1_iso_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x83,					\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = sys_cpu_to_le16(LB_ISO_EP_MPS),		\
		.bInterval = LB_ISO_EP_INTERVAL,				\
	},									\
										\
	.if2_1_iso_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x03,					\
		.bmAttributes = USB_EP_TYPE_ISO,				\
		.wMaxPacketSize = sys_cpu_to_le16(LB_ISO_EP_MPS),		\
		.bInterval = LB_ISO_EP_INTERVAL,				\
	},									\
										\
	/* Termination descriptor */						\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};										\
										\
const static struct usb_desc_header *lb_fs_desc_##x[] = {			\
	(struct usb_desc_header *) &lb_desc_##x.iad,				\
	(struct usb_desc_header *) &lb_desc_##x.if0,				\
	(struct usb_desc_header *) &lb_desc_##x.if0_in_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if0_out_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if1,				\
	(struct usb_desc_header *) &lb_desc_##x.if1_int_in_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if1_int_out_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if2_0,				\
	(struct usb_desc_header *) &lb_desc_##x.if2_0_iso_in_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_0_iso_out_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_1,				\
	(struct usb_desc_header *) &lb_desc_##x.if2_1_iso_in_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_1_iso_out_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.nil_desc,			\
};										\
										\
const static struct usb_desc_header *lb_hs_desc_##x[] = {			\
	(struct usb_desc_header *) &lb_desc_##x.iad,				\
	(struct usb_desc_header *) &lb_desc_##x.if0,				\
	(struct usb_desc_header *) &lb_desc_##x.if0_hs_in_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if0_hs_out_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if1,				\
	(struct usb_desc_header *) &lb_desc_##x.if1_int_in_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if1_int_out_ep,			\
	(struct usb_desc_header *) &lb_desc_##x.if2_0,				\
	(struct usb_desc_header *) &lb_desc_##x.if2_0_iso_in_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_0_iso_out_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_1,				\
	(struct usb_desc_header *) &lb_desc_##x.if2_1_iso_in_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.if2_1_iso_out_ep,		\
	(struct usb_desc_header *) &lb_desc_##x.nil_desc,			\
};


#define DEFINE_LOOPBACK_CLASS_DATA(x, _)					\
	static struct lb_data lb_data_##x = {					\
		.desc = &lb_desc_##x,						\
		.fs_desc = lb_fs_desc_##x,					\
		.hs_desc = lb_hs_desc_##x,					\
	};									\
										\
	static struct usbd_class_data lb_class_##x = {				\
		.priv = &lb_data_##x,						\
		.v_reqs = &lb_vregs,						\
	};									\
										\
	USBD_DEFINE_CLASS(loopback_##x, &lb_api, &lb_class_##x);

LISTIFY(CONFIG_USBD_LOOPBACK_INSTANCES_COUNT, DEFINE_LOOPBACK_DESCRIPTOR, ())
LISTIFY(CONFIG_USBD_LOOPBACK_INSTANCES_COUNT, DEFINE_LOOPBACK_CLASS_DATA, ())
