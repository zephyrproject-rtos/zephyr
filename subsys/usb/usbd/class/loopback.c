/*
 * USB loopback function
 *
 * Copyright (c) 2018-2020 Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include <usb/usbd.h>
#include <usbd_internal.h>
#include <net/buf.h>

#define LOG_LEVEL CONFIG_USBD_LOOPBACK_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_loopback);

#define LOOPBACK_BULK_EP_MPS		64
/* Default addresses to define endpoint direction */
#define LOOPBACK_OUT_EP_ADDR		0x01
#define LOOPBACK_IN_EP_ADDR		0x81
#define LOOPBACK_OUT2_EP_ADDR		0x02
#define LOOPBACK_IN2_EP_ADDR		0x82

/* Internal buffer for intermidiate test data */
static uint8_t lb_buf[1024];

#define TEST_VENDOR_REQ_OUT		0x5b
#define TEST_VENDOR_REQ_IN		0x5c

/* Make supported vendor request visible for the device stack */
static const struct usbd_cctx_vendor_req lb_vregs =
	USBD_VENDOR_REQ(TEST_VENDOR_REQ_OUT, TEST_VENDOR_REQ_IN);

/*
 * FIXME: only if0 prefixed interfaces and endpoints are usable,
 * others are for core's endpoint validation only.
 */
struct loopback_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_if_descriptor if1;
	struct usb_ep_descriptor if1_out_ep;
	struct usb_if_descriptor if2;
	struct usb_ep_descriptor if2_out_ep;
	struct usb_ep_descriptor if2_in_ep;
	struct usb_desc_header term_desc;
} __packed;

#define DEFINE_LOOPBACK_DESCRIPTOR(x, _)			\
static struct loopback_desc lb_desc_##x = {			\
	/* Interface descriptor 0 */				\
	.if0 = {						\
		.bLength = sizeof(struct usb_if_descriptor),	\
		.bDescriptorType = USB_DESC_INTERFACE,		\
		.bInterfaceNumber = 0,				\
		.bAlternateSetting = 0,				\
		.bNumEndpoints = 2,				\
		.bInterfaceClass = USB_BCC_VENDOR,		\
		.bInterfaceSubClass = 0,			\
		.bInterfaceProtocol = 0,			\
		.iInterface = 0,				\
	},							\
								\
	/* Data Endpoint OUT */					\
	.if0_out_ep = {						\
		.bLength = sizeof(struct usb_ep_descriptor),	\
		.bDescriptorType = USB_DESC_ENDPOINT,		\
		.bEndpointAddress = LOOPBACK_OUT_EP_ADDR,	\
		.bmAttributes = USB_DC_EP_BULK,			\
		.wMaxPacketSize = sys_cpu_to_le16(LOOPBACK_BULK_EP_MPS), \
		.bInterval = 0x00,				\
	},							\
								\
	/* Data Endpoint IN */					\
	.if0_in_ep = {						\
		.bLength = sizeof(struct usb_ep_descriptor),	\
		.bDescriptorType = USB_DESC_ENDPOINT,		\
		.bEndpointAddress = LOOPBACK_IN_EP_ADDR,	\
		.bmAttributes = USB_DC_EP_BULK,			\
		.wMaxPacketSize = sys_cpu_to_le16(LOOPBACK_BULK_EP_MPS), \
		.bInterval = 0x00,				\
	},							\
								\
	/* Interface descriptor 0 */				\
	.if1 = {						\
		.bLength = sizeof(struct usb_if_descriptor),	\
		.bDescriptorType = USB_DESC_INTERFACE,		\
		.bInterfaceNumber = 0,				\
		.bAlternateSetting = 1,				\
		.bNumEndpoints = 1,				\
		.bInterfaceClass = USB_BCC_VENDOR,		\
		.bInterfaceSubClass = 0,			\
		.bInterfaceProtocol = 0,			\
		.iInterface = 0,				\
	},							\
	\
	/* Data Endpoint OUT */					\
	.if1_out_ep = {						\
		.bLength = sizeof(struct usb_ep_descriptor),	\
		.bDescriptorType = USB_DESC_ENDPOINT,		\
		.bEndpointAddress = LOOPBACK_OUT_EP_ADDR,	\
		.bmAttributes = USB_DC_EP_BULK,			\
		.wMaxPacketSize = sys_cpu_to_le16(LOOPBACK_BULK_EP_MPS), \
		.bInterval = 0x00,				\
	},							\
								\
	/* Interface descriptor 0 */				\
	.if2 = {						\
		.bLength = sizeof(struct usb_if_descriptor),	\
		.bDescriptorType = USB_DESC_INTERFACE,		\
		.bInterfaceNumber = 1,				\
		.bAlternateSetting = 0,				\
		.bNumEndpoints = 2,				\
		.bInterfaceClass = USB_BCC_VENDOR,		\
		.bInterfaceSubClass = 0,			\
		.bInterfaceProtocol = 0,			\
		.iInterface = 0,				\
	},							\
								\
	/* Data Endpoint OUT */					\
	.if2_out_ep = {						\
		.bLength = sizeof(struct usb_ep_descriptor),	\
		.bDescriptorType = USB_DESC_ENDPOINT,		\
		.bEndpointAddress = LOOPBACK_OUT2_EP_ADDR,	\
		.bmAttributes = USB_DC_EP_BULK,			\
		.wMaxPacketSize = sys_cpu_to_le16(LOOPBACK_BULK_EP_MPS), \
		.bInterval = 0x00,				\
	},							\
								\
	/* Data Endpoint IN */					\
	.if2_in_ep = {						\
		.bLength = sizeof(struct usb_ep_descriptor),	\
		.bDescriptorType = USB_DESC_ENDPOINT,		\
		.bEndpointAddress = LOOPBACK_IN2_EP_ADDR,	\
		.bmAttributes = USB_DC_EP_BULK,			\
		.wMaxPacketSize = sys_cpu_to_le16(LOOPBACK_BULK_EP_MPS), \
		.bInterval = 0x00,				\
	},							\
								\
	/* Termination descriptor */				\
	.term_desc = {						\
		.bLength = 0,					\
		.bDescriptorType = 0,				\
	},							\
};								\

static void lb_pme_handler(struct usbd_class_ctx *cctx,
			   enum usbd_pme_code event)
{
	switch (event) {
	case USBD_PME_DETACHED:
		LOG_DBG("USB detached");
		break;
	case USBD_PME_SUSPEND:
		LOG_DBG("USB suspend");
		break;
	case USBD_PME_RESUME:
		LOG_DBG("USB resume");
		break;
	default:
		break;
	}
}

/*
 * Configuration has been changed, information about it
 * can be obtained from setup packet.
 * This can be used to restart transfers if necessary.
 *
 * Buffer handling here is a temporary solution
 * until the driver API was revised.
 */
static void lb_cfg_update(struct usbd_class_ctx *cctx,
			  struct usb_setup_packet *setup)
{
	struct net_buf *buf;
	uint8_t ep = 0;
	uint16_t mps = 0;
	struct loopback_desc *lb_desc;

	lb_desc = (struct loopback_desc *)cctx->class_desc;

	switch (setup->bRequest) {
	case USB_SREQ_SET_CONFIGURATION:
		LOG_WRN("Configured");
		if (cctx_restart_out_eps(cctx, 0, true) != 0) {
			LOG_ERR("Failed to restart transfer");
		}

		break;
	case USB_SREQ_SET_INTERFACE:
		LOG_WRN("Interface %u alternate %u changed",
			setup->wIndex, setup->wValue);
		if (cctx_restart_out_eps(cctx, setup->wIndex, false) != 0) {
			LOG_ERR("Failed to restart transfer");
		}

		ep = lb_desc->if0_in_ep.bEndpointAddress;
		mps = lb_desc->if0_in_ep.wMaxPacketSize;
		buf = usbd_tbuf_alloc(ep, mps);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			break;
		}

		net_buf_add_mem(buf, lb_buf, mps);
		usbd_tbuf_submit(buf, false);

		break;
	}
}

/*
 * Common handler for all endpoints.
 *
 * Buffer handling here is a temporary solution
 * until the driver API was revised.
 */
static void lb_ep_event_handler(struct usbd_class_ctx *cctx,
				struct net_buf *buf, int err)
{
	struct usbd_buf_ud *ud = NULL;
	uint8_t ep = 0;
	uint16_t mps = 0;
	struct loopback_desc *lb_desc;
	struct net_buf *new_buf;

	ud = (struct usbd_buf_ud *)net_buf_user_data(buf);
	lb_desc = (struct loopback_desc *)cctx->class_desc;
	LOG_DBG("-> ep 0x%02x, len %u, err %d", ud->ep, buf->len, err);

	if (err != 0) {
		LOG_ERR("Transfer failed with %d", err);
		net_buf_unref(buf);
		return;
	}

	if (ud->ep == lb_desc->if0_out_ep.bEndpointAddress) {
		ep = lb_desc->if0_out_ep.bEndpointAddress;
		mps = lb_desc->if0_out_ep.wMaxPacketSize;
		memcpy(lb_buf, buf->data, buf->len);
	} else if (ud->ep == lb_desc->if2_out_ep.bEndpointAddress) {
		ep = lb_desc->if2_out_ep.bEndpointAddress;
		mps = lb_desc->if2_out_ep.wMaxPacketSize;
	} else if (ud->ep == lb_desc->if0_in_ep.bEndpointAddress) {
		ep = lb_desc->if0_in_ep.bEndpointAddress;
		mps = lb_desc->if0_in_ep.wMaxPacketSize;
	} else if (ud->ep == lb_desc->if2_in_ep.bEndpointAddress) {
		ep = lb_desc->if2_in_ep.bEndpointAddress;
		mps = lb_desc->if2_in_ep.wMaxPacketSize;
	} else {
		LOG_ERR("Unknown endpoint, skip");
		net_buf_unref(buf);
		return;
	}

	net_buf_unref(buf);

	new_buf = usbd_tbuf_alloc(ep, mps);
	if (new_buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		net_buf_unref(buf);
		return;
	}

	if (USB_EP_DIR_IS_IN(ep)) {
		net_buf_add_mem(new_buf, lb_buf, mps);
	}

	usbd_tbuf_submit(new_buf, false);
	LOG_DBG("<- ep 0x%02x, len %u", ep, mps);
}

/*
 * Common handler for all control requests.
 *
 * Regardless requests recipient (interface or endpoint)
 * the usb device stack will identify
 * proper class context and call this handler.
 * For the vendor type request USBD_VENDOR_REQ must be used
 * to identify the context, if more than one class instance is
 * present, only the first one will be called.
 *
 * Buffer handling here is a temporary solution
 * until the driver API was revised.
 */
static int lb_req_handler(struct usbd_class_ctx *cctx,
			  struct usb_setup_packet *setup,
			  struct net_buf *buf)
{
	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_DEVICE) {
		return -ENOTSUP;
	}

	if (usb_reqtype_is_to_device(setup) &&
	    setup->bRequest == TEST_VENDOR_REQ_OUT) {
		LOG_WRN("Host-to-Device, wLength %u | %u", setup->wLength,
			MIN(sizeof(lb_buf), buf->len));
		memcpy(lb_buf, buf->data, MIN(sizeof(lb_buf), buf->len));
		return 0;
	}

	if (usb_reqtype_is_to_host(setup) &&
	    setup->bRequest == TEST_VENDOR_REQ_IN) {
		struct net_buf *buf;

		buf = usbd_tbuf_alloc(USB_CONTROL_EP_IN, setup->wLength);
		if (buf == NULL) {
			return -ENOMEM;
		}

		net_buf_add_mem(buf, lb_buf,
				MIN(sizeof(lb_buf), setup->wLength));
		usbd_tbuf_submit(buf, false);

		LOG_WRN("Device-to-Host, wLength %u | %u", setup->wLength,
			MIN(sizeof(lb_buf), setup->wLength));
		return 0;
	}

	LOG_ERR("Class request 0x%x not supported", setup->bRequest);

	return -ENOTSUP;
}

static int lb_init(struct usbd_class_ctx *cctx)
{
	LOG_WRN("Class instance %p init", cctx);

	return 0;
}

#define DEFINE_LOOPBACK_CLASS_DATA(x, _)				\
static struct usbd_class_ctx lb_class_##x = {				\
		.class_desc = (struct usb_desc_header*)&lb_desc_##x,	\
		.string_desc = NULL,					\
		.v_reqs = &lb_vregs,					\
		.ops = {						\
			.req_handler = lb_req_handler,			\
			.ep_cb = lb_ep_event_handler,			\
			.cfg_update = lb_cfg_update,			\
			.pm_event = lb_pme_handler,			\
			.init = lb_init,				\
		},							\
		.ep_bm = 0,						\
	};								\

/*
 * Device API is used to identify specific instance
 * for usbd_cctx_register(), usbd_cctx_unregister() API functions.
 */
static int usbd_lb_device_init(struct device *dev)
{
	LOG_DBG("Init loopback device %s", dev->name);

	return 0;
}

static const struct usbd_lb_device_api {
	void (*init)(void);
} lb_api;

#define DEFINE_LOOPBACK_DEV_DATA(x, _)					\
	DEVICE_AND_API_INIT(usbd_class_lb##x,				\
			    "USBD_CLASS_LB_" #x,			\
			    &usbd_lb_device_init,			\
			    NULL,					\
			    &lb_class_##x,				\
			    POST_KERNEL,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &lb_api);

UTIL_LISTIFY(CONFIG_USBD_LOOPBACK_DEVICE_COUNT, DEFINE_LOOPBACK_DESCRIPTOR, _)
UTIL_LISTIFY(CONFIG_USBD_LOOPBACK_DEVICE_COUNT, DEFINE_LOOPBACK_CLASS_DATA, _)
UTIL_LISTIFY(CONFIG_USBD_LOOPBACK_DEVICE_COUNT, DEFINE_LOOPBACK_DEV_DATA, _)
