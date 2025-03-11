/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <usbd_desc.h>
#include "usbd_mtp_class.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_mtp, CONFIG_USBD_MTP_LOG_LEVEL);

/* Endpoint addresses */
#define MTP_IN_EP_ADDR   0x81 /* Bulk IN */
#define MTP_OUT_EP_ADDR  0x01 /* Bulk OUT */
#define MTP_INTR_EP_ADDR 0x82 /* Interrupt IN */

#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDWHITE   "\033[1m\033[35m" /* Bold White */
#define RESET       "\033[0m"

#define BUF_TRACE_DEBUG 0

#if BUF_TRACE_DEBUG
int allocated_bufs;
__unused void buf_destroyed(struct net_buf *buf)
{
	allocated_bufs--;
	struct udc_buf_info *bi = udc_get_buf_info(buf);

	net_buf_destroy(buf);
	LOG_WRN("BUF <Destroyed> %p EP: %s (Allocated bufs: %d)", buf,
		bi->ep == 0x01 ? "OUT" : "IN", allocated_bufs);
}

UDC_BUF_POOL_DEFINE(mtp_ep_pool, 3, 512, sizeof(struct udc_buf_info), buf_destroyed);
#else
UDC_BUF_POOL_DEFINE(mtp_ep_pool, 3, 512, sizeof(struct udc_buf_info), NULL);
#endif

struct mtp_desc {

	/* Full Speed Descriptors */
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_int_in_ep;

	/* High Speed Descriptors */
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;

	/* Termination descriptor */
	struct usb_desc_header nil_desc;
};

struct mtp_data {
	struct mtp_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	atomic_t state;
};

struct mtp_device_status {
	uint16_t wLength;
	uint16_t wCode;
};

static void usbd_mtp_update(struct usbd_class_data *c_data, uint8_t iface, uint8_t alternate)
{
	LOG_WRN("Instance %p, interface %u alternate %u changed", c_data, iface, alternate);
}

static uint8_t mtp_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct mtp_data *data = usbd_class_get_private(c_data);
	struct mtp_desc *desc = data->desc;

	LOG_WRN("FS EP IN: 0x%x, HS EP IN: 0x%x", desc->if0_in_ep.bEndpointAddress,
		desc->if0_hs_in_ep.bEndpointAddress);

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_in_ep.bEndpointAddress;
	}

	return desc->if0_in_ep.bEndpointAddress;
}

static uint8_t mtp_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct mtp_data *data = usbd_class_get_private(c_data);
	struct mtp_desc *desc = data->desc;

	LOG_WRN("FS EP OUT: 0x%x, HS EP OUT: 0x%x", desc->if0_out_ep.bEndpointAddress,
		desc->if0_hs_out_ep.bEndpointAddress);

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_out_ep.bEndpointAddress;
	}

	return desc->if0_out_ep.bEndpointAddress;
}

struct net_buf *mtp_buf_alloc(const uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&mtp_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;

#if BUF_TRACE_DEBUG
	allocated_bufs++;
	LOG_WRN("Buf >Allocated<: %p EP: %s (Allocated bufs: %d)", buf, ep == 0x01 ? "OUT" : "IN",
		allocated_bufs);
#endif
	return buf;
}

static int usbd_mtp_control_to_host(struct usbd_class_data *c_data,
				    const struct usb_setup_packet *const setup,
				    struct net_buf *const buf)
{
	LOG_DBG("%s: Class request 0x%x (Recipient: %x)", __func__,
		setup->bRequest, setup->RequestType.recipient);

	//FIXME: Handle Cancel transaction properly
	int ret = mtp_control_handler(setup->bRequest, buf);
	if (ret){
		usbd_ep_set_halt(usbd_class_get_ctx(c_data), 0x81);
		usbd_ep_set_halt(usbd_class_get_ctx(c_data), 0x01);
	}
	return 0;
}

static int usbd_mtp_control_to_dev(struct usbd_class_data *c_data,
	const struct usb_setup_packet *const setup,
	const struct net_buf *const buf)
{
	LOG_WRN("%s: Class request 0x%x (Recipient: %x)", __func__,
		setup->bRequest, setup->RequestType.recipient);

	//FIXME: Handle Cancel transaction properly
	int ret = mtp_control_handler(setup->bRequest, buf);
	if (ret){
		usbd_ep_set_halt(usbd_class_get_ctx(c_data), 0x81);
		usbd_ep_set_halt(usbd_class_get_ctx(c_data), 0x01);
	}
	return 0;
}

static void usbd_mtp_enable(struct usbd_class_data *const c_data);

static int usbd_mtp_request_handler(struct usbd_class_data *c_data, struct net_buf *buf, int err)
{
	LOG_INF("\n\n");
	LOG_INF(BOLDWHITE "==[mtp_request_handler]=============Entry============" RESET);

	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct udc_buf_info *bi = (struct udc_buf_info *)net_buf_user_data(buf);
	int ret = 0;

	struct net_buf *buf_resp = NULL;

	if (bi->ep == mtp_get_bulk_out(c_data)) {
		LOG_INF(BOLDWHITE
			"==[START] -> [Host Sent a command]=========================" RESET);
		LOG_INF("%s: %p -> ep 0x%02x, buf: %p len %u, err %d", __func__, c_data, bi->ep,
			buf, buf->len, err);
		buf_resp = mtp_buf_alloc(mtp_get_bulk_in(c_data));
		if (buf_resp == NULL) {
			LOG_ERR("%s: Buffer allocation failed!", __func__);
			LOG_ERR("REF COUNT %u", buf_resp->ref);
			return -1;
		}

		ret = mtp_commands_handler(buf, buf_resp);
		if (ret) {
			ret = usbd_ep_enqueue(c_data, buf_resp);
			if (ret) {
				LOG_ERR("Failed to enqueue net_buf %d", ret);
				net_buf_unref(buf_resp);
			} else {
				LOG_DBG("[replied to Host ... DONE]");
			}
		} else if (ret < 0) {
			net_buf_unref(buf_resp);
			LOG_ERR("mtp_commands_handler failed");
		} else if (ret == 0) {
			net_buf_unref(buf_resp);
			LOG_WRN("Nothing to Send!");
			usbd_mtp_enable(c_data);
		}

		if (mtp_needs_more_data(buf)) {
			LOG_INF("Alloc EXTRA Buffer");
		}

	} else if (bi->ep == mtp_get_bulk_in(c_data)) {
		LOG_WRN("Host event EP: %x[%s] (buf %p, len: %u)", bi->ep,
			bi->ep == 0x01 ? "MTP_OUT_EP_ADDR" : "MTP_IN_EP_ADDR", buf, buf->len);

		if (mtp_packet_pending()) {
			LOG_INF("Sending Pending packet");
			buf_resp = mtp_buf_alloc(mtp_get_bulk_in(c_data));
			if (buf_resp == NULL) {
				LOG_ERR("%s: Buffer allocation failed 4!", __func__);
				LOG_ERR("REF COUNT %u", buf_resp->ref);
				return -1;
			}

			send_pending_packet(buf_resp);
			ret = usbd_ep_enqueue(c_data, buf_resp);
			if (ret) {
				LOG_ERR("Failed to enqueue net_buf %d", ret);
				net_buf_unref(buf_resp);
			}
			LOG_DBG("Pending DONE");
		} else {
			LOG_WRN("No Pending packet");
			usbd_mtp_enable(c_data);
		}
		LOG_INF(BOLDWHITE
			"==[END] -> [Host Confirmed a reply]======================" RESET);
	} else {
		LOG_ERR("SHOULDN'T BE HERE! buf->len: %u", buf->len);
	}

	LOG_INF(BOLDWHITE "==[mtp_request_handler]==== Destroy buf %p EP: 0x%x [%s] =====" RESET,
		buf, udc_get_buf_info(buf)->ep,
		udc_get_buf_info(buf)->ep == 0x01 ? "MTP_OUT_EP_ADDR" : "MTP_IN_EP_ADDR");

	return usbd_ep_buf_free(uds_ctx, buf);
}

/* Class associated configuration is selected */
static void usbd_mtp_enable(struct usbd_class_data *const c_data)
{
	struct net_buf *bufp = mtp_buf_alloc(MTP_OUT_EP_ADDR);

	if (bufp == NULL) {
		LOG_ERR("%s: Buffer allocation failed! 5", __func__);
		return;
	}

	int ret = usbd_ep_enqueue(c_data, bufp);

	if (ret) {
		LOG_ERR("Init Failed to enqueue net_buf %d", ret);
		net_buf_unref(bufp);
		return;
	}

	LOG_INF("Ready to receive from HOST");
}

/* Class associated configuration is disabled */
static void usbd_mtp_disable(struct usbd_class_data *const c_data)
{
	LOG_ERR("**************Disable**************");
}

static void *usbd_mtp_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	struct mtp_data *data = usbd_class_get_private(c_data);

	if (speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

static int usbd_mtp_init(struct usbd_class_data *c_data)
{
	LOG_INF("Init class instance %p", c_data);

	char *manufacturer, *model, *device_version, *serial_number;
	struct usb_device_descriptor *usbd_desc;
	struct usbd_desc_node *d_nd;

	if (usbd_bus_speed(c_data->uds_ctx) == USBD_SPEED_HS) {
		usbd_desc = (struct usb_device_descriptor *)c_data->uds_ctx->hs_desc;
	} else {
		usbd_desc = (struct usb_device_descriptor *)c_data->uds_ctx->fs_desc;
	}

	d_nd = usbd_get_descriptor(c_data->uds_ctx, USB_DESC_STRING, usbd_desc->iManufacturer);
	manufacturer = (char *)d_nd->ptr;

	d_nd = usbd_get_descriptor(c_data->uds_ctx, USB_DESC_STRING, usbd_desc->iProduct);
	model = (char *)d_nd->ptr;

	d_nd = usbd_get_descriptor(c_data->uds_ctx, USB_DESC_STRING, usbd_desc->iSerialNumber);
	serial_number = (char *)d_nd->ptr;

	device_version = "1.0";

	LOG_DBG("Desc data: Manufacturer: %s, Product: %s, SN: %s",
			manufacturer,
			model,
			d_nd->bLength > 0 ? serial_number : "NULL");

	mtp_init(manufacturer, model, device_version, serial_number);

	return 0;
}

struct usbd_class_api mtp_api = {
	.update = usbd_mtp_update,
	.control_to_dev = usbd_mtp_control_to_dev,
	.control_to_host = usbd_mtp_control_to_host,
	.request = usbd_mtp_request_handler,
	.enable = usbd_mtp_enable,
	.disable = usbd_mtp_disable,
	.get_desc = usbd_mtp_get_desc,
	.init = usbd_mtp_init,
};

static struct mtp_desc mtp_desc_0 = {
	.if0 = {

			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_DESC_INTERFACE,
			.bInterfaceNumber = 0x00,
			.bAlternateSetting = 0x00,
			.bNumEndpoints = 0x03,
			.bInterfaceClass = USB_BCC_IMAGE,
			.bInterfaceSubClass = 0x01, /* Still Image Capture */
			.bInterfaceProtocol = 0x01, /* Picture Transfer Protocol */
			.iInterface = 0x00,
		},

	.if0_int_in_ep = {

			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_DESC_ENDPOINT,
			.bEndpointAddress = MTP_INTR_EP_ADDR,
			.bmAttributes = USB_EP_TYPE_INTERRUPT,
			.wMaxPacketSize = sys_cpu_to_le16(16U),
			.bInterval = 0x06,
		},

	/* Full Speed*/
	.if0_in_ep = {

			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_DESC_ENDPOINT,
			.bEndpointAddress = MTP_IN_EP_ADDR,
			.bmAttributes = USB_EP_TYPE_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(64U),
			.bInterval = 0x00,
		},

	.if0_out_ep = {

			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_DESC_ENDPOINT,
			.bEndpointAddress = MTP_OUT_EP_ADDR,
			.bmAttributes = USB_EP_TYPE_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(64U),
			.bInterval = 0x00,
		},

	/* High Speed */
	.if0_hs_in_ep = {

			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_DESC_ENDPOINT,
			.bEndpointAddress = MTP_IN_EP_ADDR,
			.bmAttributes = USB_EP_TYPE_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(512U),
			.bInterval = 0x00,
		},

	.if0_hs_out_ep = {

			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_DESC_ENDPOINT,
			.bEndpointAddress = MTP_OUT_EP_ADDR,
			.bmAttributes = USB_EP_TYPE_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(512U),
			.bInterval = 0x00,
		},

	.nil_desc = {

			.bLength = 0,
			.bDescriptorType = 0,
		},
};

const static struct usb_desc_header *mtp_fs_desc_0[] = {
	(struct usb_desc_header *)&mtp_desc_0.if0,
	(struct usb_desc_header *)&mtp_desc_0.if0_in_ep,
	(struct usb_desc_header *)&mtp_desc_0.if0_out_ep,
	(struct usb_desc_header *)&mtp_desc_0.if0_int_in_ep,
	(struct usb_desc_header *)&mtp_desc_0.nil_desc,
};
const static struct usb_desc_header *mtp_hs_desc_0[] = {
	(struct usb_desc_header *)&mtp_desc_0.if0,
	(struct usb_desc_header *)&mtp_desc_0.if0_hs_in_ep,
	(struct usb_desc_header *)&mtp_desc_0.if0_hs_out_ep,
	(struct usb_desc_header *)&mtp_desc_0.if0_int_in_ep,
	(struct usb_desc_header *)&mtp_desc_0.nil_desc,
};

static struct mtp_data mtp_data0 = {
	.desc = &mtp_desc_0,
	.fs_desc = mtp_fs_desc_0,
	.hs_desc = mtp_hs_desc_0,
};

USBD_DEFINE_CLASS(mtp, &mtp_api, &mtp_data0, NULL);
