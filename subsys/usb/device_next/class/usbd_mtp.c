/*
 * Copyright (c) 2025 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include "usbd_mtp_class.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_mtp, CONFIG_USBD_MTP_LOG_LEVEL);

#define MTP_SUBCLASS    0x01 /* Still Image Capture */
#define MTP_PTPPROTOCOL 0x01 /* PTP Protocol */

UDC_BUF_POOL_DEFINE(mtp_ep_pool, 2 * CONFIG_USBD_MTP_INSTANCES_COUNT, USBD_MAX_BULK_MPS,
		    sizeof(struct udc_buf_info), NULL);

struct mtp_desc {
	struct usb_if_descriptor if0;
	/* Full Speed Descriptors */
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_int_in_ep;

	/* High Speed Descriptors */
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_ep_descriptor if0_hs_int_in_ep;

	/* Termination descriptor */
	struct usb_desc_header nil_desc;
};

struct mtp_data {
	struct mtp_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	uint8_t instance_id;
	struct mtp_context mtp_ctx;
};

static const struct usbd_mtp_instance *mtp_instance_config_get(uint8_t instance_id)
{
	uint8_t current_id = 0;

	STRUCT_SECTION_FOREACH(usbd_mtp_instance, instance) {
		if (current_id == instance_id) {
			return instance;
		}

		current_id++;
	}

	return NULL;
}

static void usbd_mtp_update(struct usbd_class_data *c_data, uint8_t iface, uint8_t alternate)
{
	LOG_DBG("Instance %p, interface %u alternate %u changed", c_data, iface, alternate);
}

static uint8_t mtp_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct mtp_data *data = usbd_class_get_private(c_data);
	struct mtp_desc *desc = data->desc;

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_in_ep.bEndpointAddress;
	}

	return desc->if0_in_ep.bEndpointAddress;
}

static uint8_t mtp_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct mtp_data *data = usbd_class_get_private(c_data);
	struct mtp_desc *desc = data->desc;

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_out_ep.bEndpointAddress;
	}

	return desc->if0_out_ep.bEndpointAddress;
}

static uint16_t mtp_get_bulk_in_mps(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return 512U;
	}

	return 64U;
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
	bi->ep = ep;

	return buf;
}

static int usbd_mtp_control_to_host(struct usbd_class_data *c_data,
				    const struct usb_setup_packet *const setup,
				    struct net_buf *const buf)
{
	struct mtp_data *data = usbd_class_get_private(c_data);
	struct mtp_context *mtp_ctx = &data->mtp_ctx;

	LOG_DBG("Class request 0x%x (Recipient: %x)", setup->bRequest,
		setup->RequestType.recipient);

	switch (setup->bRequest) {
	case MTP_REQUEST_GET_DEVICE_STATUS:
		mtp_ctx->dev_status.ep_in = mtp_get_bulk_in(c_data);
		mtp_ctx->dev_status.ep_out = mtp_get_bulk_out(c_data);
		mtp_get_dev_status(mtp_ctx, buf);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int usbd_mtp_control_to_dev(struct usbd_class_data *c_data,
				   const struct usb_setup_packet *const setup,
				   const struct net_buf *const buf)
{
	struct mtp_data *data = usbd_class_get_private(c_data);
	struct mtp_context *mtp_ctx = &data->mtp_ctx;

	LOG_DBG("Class request 0x%x (Recipient: %x)", setup->bRequest,
		setup->RequestType.recipient);

	if (setup->wLength && (buf == NULL)) {
		/* All supported commands require wLength=0, do not receive Data OUT stage. */
		return -EINVAL;
	}

	switch (setup->bRequest) {
	case MTP_REQUEST_CANCEL:
		mtp_cancel(mtp_ctx, buf);
		break;
	case MTP_REQUEST_DEVICE_RESET:
		mtp_device_reset(mtp_ctx);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void usbd_mtp_enable(struct usbd_class_data *const c_data);

static void mtp_recover(struct usbd_class_data *const c_data)
{
	struct mtp_data *data = usbd_class_get_private(c_data);

	mtp_reset(&data->mtp_ctx);
	usbd_mtp_enable(c_data);
}

static void usbd_mtp_out_request_handler(struct usbd_class_data *c_data, struct mtp_context *ctx,
					 struct net_buf *buf)
{
	const uint8_t ep_in = mtp_get_bulk_in(c_data);
	struct udc_buf_info *bi = net_buf_user_data(buf);
	struct net_buf *buf_resp;
	int ret;

	LOG_DBG("\n=========[Host Sent a command]=========");
	LOG_DBG("%s: %p -> ep 0x%02x, buf: %p len %u", __func__, c_data, bi->ep, buf, buf->len);

	buf_resp = mtp_buf_alloc(ep_in);
	if (buf_resp == NULL) {
		LOG_ERR("%s: Buffer allocation failed!", __func__);
		mtp_recover(c_data);
		return;
	}

	ret = mtp_commands_handler(ctx, buf, buf_resp);
	if (ret > 0) {
		ret = usbd_ep_enqueue(c_data, buf_resp);
		if (ret) {
			LOG_ERR("Failed to enqueue net_buf %d", ret);
			net_buf_unref(buf_resp);
			mtp_recover(c_data);
		} else {
			LOG_DBG("[replied to Host ... DONE]");
		}
	} else if (ret < 0) {
		LOG_ERR("mtp_commands_handler failed");
		net_buf_unref(buf_resp);
		mtp_recover(c_data);
	} else {
		/* ret == 0: no data to send, re-arm the OUT endpoint */
		LOG_DBG("Nothing to Send!");
		net_buf_unref(buf_resp);
		usbd_mtp_enable(c_data);
	}
}

static void usbd_mtp_in_request_handler(struct usbd_class_data *c_data, struct mtp_context *ctx,
					struct net_buf *buf)
{
	const uint8_t ep_in = mtp_get_bulk_in(c_data);
	struct udc_buf_info *bi = net_buf_user_data(buf);
	struct net_buf *buf_resp;
	int ret;

	LOG_DBG("\n=============[Host ACK'd]==============");
	LOG_DBG("Host %s EP: %x[MTP_IN_EP] (buf %p)", buf->len == 0 ? "[ACK]" : "Event", bi->ep,
		buf);

	if (mtp_packet_pending(ctx)) {
		LOG_DBG("Sending Pending packet");
		buf_resp = mtp_buf_alloc(ep_in);
		if (buf_resp == NULL) {
			LOG_ERR("%s: Buffer allocation failed!", __func__);
			mtp_recover(c_data);
			return;
		}

		ret = mtp_commands_handler(ctx, NULL, buf_resp);
		if (ret < 0) {
			LOG_ERR("Failed to get pending packet %d", ret);
			net_buf_unref(buf_resp);
			mtp_recover(c_data);
			return;
		}

		ret = usbd_ep_enqueue(c_data, buf_resp);
		if (ret) {
			LOG_ERR("Failed to enqueue net_buf %d", ret);
			net_buf_unref(buf_resp);
			mtp_recover(c_data);
		}
	} else {
		LOG_DBG("No Pending packet");
		usbd_mtp_enable(c_data);
	}

	LOG_DBG("\n========[Host ACK handling END]========\n");
}

static int usbd_mtp_request_handler(struct usbd_class_data *c_data, struct net_buf *buf, int err)
{
	struct mtp_data *data = usbd_class_get_private(c_data);
	struct mtp_context *ctx = &data->mtp_ctx;
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct udc_buf_info *bi = net_buf_user_data(buf);
	const uint8_t ep_in = mtp_get_bulk_in(c_data);

	if (err) {
		LOG_ERR("Error %d on EP: 0x%02x", err, bi->ep);
		mtp_reset(ctx);
		goto out;
	}

	if (bi->ep == mtp_get_bulk_out(c_data)) {
		usbd_mtp_out_request_handler(c_data, ctx, buf);
	} else if (bi->ep == ep_in) {
		usbd_mtp_in_request_handler(c_data, ctx, buf);
	}

out:
	return usbd_ep_buf_free(uds_ctx, buf);
}

/* Class associated configuration is selected */
static void usbd_mtp_enable(struct usbd_class_data *const c_data)
{
	struct mtp_data *data = usbd_class_get_private(c_data);
	struct mtp_context *ctx = &data->mtp_ctx;
	struct net_buf *buf;
	int ret;

	LOG_DBG("Configuration enabled");
	ctx->max_packet_size = mtp_get_bulk_in_mps(c_data);

	buf = mtp_buf_alloc(mtp_get_bulk_out(c_data));
	if (buf == NULL) {
		LOG_ERR("%s: Buffer allocation failed", __func__);
		return;
	}

	ret = usbd_ep_enqueue(c_data, buf);
	if (ret) {
		LOG_ERR("Init Failed to enqueue net_buf %d", ret);
		net_buf_unref(buf);
		return;
	}

	LOG_DBG("Ready to receive from HOST");
}

/* Class associated configuration is disabled */
static void usbd_mtp_disable(struct usbd_class_data *const c_data)
{
	struct mtp_data *data = usbd_class_get_private(c_data);

	mtp_reset(&data->mtp_ctx);

	LOG_DBG("Configuration disabled");
}

static void *usbd_mtp_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	struct mtp_data *data = usbd_class_get_private(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

static int usbd_mtp_init(struct usbd_class_data *c_data)
{
	struct mtp_data *data = usbd_class_get_private(c_data);
	const struct usbd_mtp_instance *config = mtp_instance_config_get(data->instance_id);

	LOG_INF("Init class instance %p", c_data);

	LOG_DBG("MTP instance %u, PktSize: %u", data->instance_id, data->mtp_ctx.max_packet_size);

	return mtp_init(&data->mtp_ctx, config);
}

#define DEFINE_MTP_DESCRIPTOR(n, _)                                                                \
	static struct mtp_desc mtp_desc_##n = {                                                    \
		.if0 = {                                                                           \
				.bLength = sizeof(struct usb_if_descriptor),                       \
				.bDescriptorType = USB_DESC_INTERFACE,                             \
				.bInterfaceNumber = 0x00,                                          \
				.bAlternateSetting = 0x00,                                         \
				.bNumEndpoints = 0x03,                                             \
				.bInterfaceClass = USB_BCC_IMAGE,                                  \
				.bInterfaceSubClass = MTP_SUBCLASS,                                \
				.bInterfaceProtocol = MTP_PTPPROTOCOL,                             \
				.iInterface = 0x00,                                                \
			},                                                                         \
		.if0_int_in_ep = {                                                                 \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x82,                                          \
				.bmAttributes = USB_EP_TYPE_INTERRUPT,                             \
				.wMaxPacketSize = sys_cpu_to_le16(16U),                            \
				.bInterval = 0x06,                                                 \
			},                                                                         \
		.if0_in_ep = {                                                                     \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x81,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64U),                            \
				.bInterval = 0x00,                                                 \
			},                                                                         \
		.if0_out_ep = {                                                                    \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x01,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(64U),                            \
				.bInterval = 0x00,                                                 \
			},                                                                         \
		.if0_hs_in_ep = {                                                                  \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x81,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(512U),                           \
				.bInterval = 0x00,                                                 \
			},                                                                         \
		.if0_hs_out_ep = {                                                                 \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x01,                                          \
				.bmAttributes = USB_EP_TYPE_BULK,                                  \
				.wMaxPacketSize = sys_cpu_to_le16(512U),                           \
				.bInterval = 0x00,                                                 \
			},                                                                         \
		.if0_hs_int_in_ep = {                                                              \
				.bLength = sizeof(struct usb_ep_descriptor),                       \
				.bDescriptorType = USB_DESC_ENDPOINT,                              \
				.bEndpointAddress = 0x82,                                          \
				.bmAttributes = USB_EP_TYPE_INTERRUPT,                             \
				.wMaxPacketSize = sys_cpu_to_le16(16U),                            \
				.bInterval = 0x06,                                                 \
			},                                                                         \
		.nil_desc = {                                                                      \
				.bLength = 0,                                                      \
				.bDescriptorType = 0,                                              \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	const static struct usb_desc_header *mtp_fs_desc_##n[] = {                                 \
		(struct usb_desc_header *)&mtp_desc_##n.if0,                                       \
		(struct usb_desc_header *)&mtp_desc_##n.if0_in_ep,                                 \
		(struct usb_desc_header *)&mtp_desc_##n.if0_out_ep,                                \
		(struct usb_desc_header *)&mtp_desc_##n.if0_int_in_ep,                             \
		(struct usb_desc_header *)&mtp_desc_##n.nil_desc,                                  \
	};                                                                                         \
                                                                                                   \
	const static struct usb_desc_header *mtp_hs_desc_##n[] = {                                 \
		(struct usb_desc_header *)&mtp_desc_##n.if0,                                       \
		(struct usb_desc_header *)&mtp_desc_##n.if0_hs_in_ep,                              \
		(struct usb_desc_header *)&mtp_desc_##n.if0_hs_out_ep,                             \
		(struct usb_desc_header *)&mtp_desc_##n.if0_hs_int_in_ep,                          \
		(struct usb_desc_header *)&mtp_desc_##n.nil_desc,                                  \
	};

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

#define DEFINE_MTP_CLASS_DATA(x, _)                                                                \
	static struct mtp_data mtp_data_##x = {                                                    \
		.desc = &mtp_desc_##x,                                                             \
		.fs_desc = mtp_fs_desc_##x,                                                        \
		.hs_desc = mtp_hs_desc_##x,                                                        \
		.instance_id = x,                                                                  \
	};                                                                                         \
                                                                                                   \
	USBD_DEFINE_CLASS(mtp_##x, &mtp_api, &mtp_data_##x, NULL);

LISTIFY(CONFIG_USBD_MTP_INSTANCES_COUNT, DEFINE_MTP_DESCRIPTOR, ())
LISTIFY(CONFIG_USBD_MTP_INSTANCES_COUNT, DEFINE_MTP_CLASS_DATA, ())
