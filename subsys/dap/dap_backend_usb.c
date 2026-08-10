/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/dap/dap_link.h>

#include "cmsis_dap.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dap_usb, CONFIG_DAP_LOG_LEVEL);

/*
 * This file implements CMSIS DAP USB backend function using bulk endpoints.
 */

static uint8_t response_buf[512];

NET_BUF_POOL_FIXED_DEFINE(dap_func_pool,
			  1, 0, sizeof(struct udc_buf_info), NULL);

UDC_STATIC_BUF_DEFINE(dap_func_buf, 512);

#ifdef CONFIG_DAP_SWO
NET_BUF_POOL_FIXED_DEFINE(dap_trace_pool,
			  1, 0, sizeof(struct udc_buf_info), NULL);

/* One trace-EP transfer: full-speed builds never move more than one
 * 64-byte packet per transfer, so sizing for high speed there would
 * waste 448 bytes of buffer that no code path can fill.
 */
UDC_STATIC_BUF_DEFINE(dap_trace_buf, USBD_SUPPORTS_HIGH_SPEED ? 512 : 64);

static struct k_work dap_trace_work;
#endif

struct dap_func_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
#ifdef CONFIG_DAP_SWO
	struct usb_ep_descriptor if0_swo_in_ep;
#endif
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
#ifdef CONFIG_DAP_SWO
	struct usb_ep_descriptor if0_hs_swo_in_ep;
#endif
	struct usb_desc_header nil_desc;
};

#define SAMPLE_FUNCTION_ENABLED		0
#define DAP_FUNCTION_TRACE_BUSY		1

struct dap_func_data {
	struct dap_func_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	struct usbd_desc_node *const iface_str_desc_nd;
	atomic_t state;
	struct dap_link_context *dap_link_ctx;
};

static uint8_t dap_func_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_func_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_out_ep.bEndpointAddress;
	}

	return desc->if0_out_ep.bEndpointAddress;
}

static uint8_t dap_func_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_func_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_in_ep.bEndpointAddress;
	}

	return desc->if0_in_ep.bEndpointAddress;
}

#ifdef CONFIG_DAP_SWO
static uint8_t dap_func_get_trace_in(struct usbd_class_data *const c_data)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_func_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_swo_in_ep.bEndpointAddress;
	}

	return desc->if0_swo_in_ep.bEndpointAddress;
}
#endif

static int dap_func_request_handler(struct usbd_class_data *c_data,
				  struct net_buf *buf, int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct udc_buf_info *bi = NULL;

	bi = (struct udc_buf_info *)net_buf_user_data(buf);
	LOG_DBG("Transfer finished %p -> ep 0x%02x, len %u, err %d",
		(void *)c_data, bi->ep, buf->len, err);

#ifdef CONFIG_DAP_SWO
	if (bi->ep == dap_func_get_trace_in(c_data)) {
		size_t lost = buf->len;

		usbd_ep_buf_free(uds_ctx, buf);
		atomic_clear_bit(&data->state, DAP_FUNCTION_TRACE_BUSY);
		if (err == 0 &&
		    atomic_test_bit(&data->state, SAMPLE_FUNCTION_ENABLED)) {
			k_work_submit(&dap_trace_work);
		} else if (err != 0) {
			/* The transfer's bytes were already consumed out
			 * of the trace ring and are gone with it. Silent
			 * loss must not read back as a clean capture --
			 * report it the same way as any other trace loss.
			 */
			LOG_WRN("Trace transfer failed (%d), %zu bytes lost",
				err, lost);
			dap_swo_stream_error(data->dap_link_ctx);
		}

		return 0;
	}
#endif

	if (atomic_test_bit(&data->state, SAMPLE_FUNCTION_ENABLED) && err == 0) {
		uint8_t ep = bi->ep;
		size_t len;

		memset(bi, 0, sizeof(struct udc_buf_info));
		if (ep == dap_func_get_bulk_in(c_data)) {
			bi->ep = dap_func_get_bulk_out(c_data);
			net_buf_reset(buf);
		} else {
			bi->ep = dap_func_get_bulk_in(c_data);

			len = dap_link_execute_cmd(data->dap_link_ctx,
						   buf->data, response_buf);
			net_buf_reset(buf);
			LOG_DBG("response length %u, starting with [0x%02X, 0x%02X]",
				len, response_buf[0], response_buf[1]);
			net_buf_add_mem(buf, response_buf, MIN(len, net_buf_tailroom(buf)));
		}

		if (usbd_ep_enqueue(c_data, buf)) {
			LOG_ERR("Failed to enqueue buffer");
			usbd_ep_buf_free(uds_ctx, buf);
		}
	} else {
		LOG_ERR("Function is disabled or transfer failed");
		usbd_ep_buf_free(uds_ctx, buf);
	}

	return 0;
}

static void *dap_func_get_desc(struct usbd_class_data *const c_data,
			     const enum usbd_speed speed)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);

	if (speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

struct net_buf *dap_func_buf_alloc(struct usbd_class_data *const c_data,
				const uint8_t ep)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;
	size_t size;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		size = 512U;
	} else {
		size = 64U;
	}

	buf = net_buf_alloc_with_data(&dap_func_pool, dap_func_buf, size, K_NO_WAIT);
	net_buf_reset(buf);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;

	return buf;
}

static void dap_func_enable(struct usbd_class_data *const c_data)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct net_buf *buf;

	LOG_INF("Configuration enabled");

	if (!atomic_test_and_set_bit(&data->state, SAMPLE_FUNCTION_ENABLED)) {
		if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
			dap_link_set_pkt_size(data->dap_link_ctx, 512);
		} else {
			dap_link_set_pkt_size(data->dap_link_ctx, 64);
		}

		buf = dap_func_buf_alloc(c_data, dap_func_get_bulk_out(c_data));
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return;
		}

		if (usbd_ep_enqueue(c_data, buf)) {
			LOG_ERR("Failed to enqueue buffer");
			usbd_ep_buf_free(uds_ctx, buf);
		}
	}
}

static void dap_func_disable(struct usbd_class_data *const c_data)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);

	atomic_clear_bit(&data->state, SAMPLE_FUNCTION_ENABLED);
#ifdef CONFIG_DAP_SWO
	/* DAP_FUNCTION_TRACE_BUSY is deliberately NOT cleared here: it
	 * guards the single static trace buffer against the transfer
	 * that may still be in flight when the configuration goes down.
	 * The stack completes every queued transfer through the request
	 * handler (usbd_ep_disable() dequeues with -ECONNABORTED), and
	 * that completion is the one place the bit is released --
	 * clearing it early would let a re-enable kick reuse the buffer
	 * while the controller can still be reading it out.
	 */
	/* The transport that was consuming the trace is gone (bus
	 * reset, detach, reconfiguration). Stop capture rather than
	 * let it outlive its host: a crashed or unplugged debugger
	 * never sends DAP_SWO_Control(0), and the leaked capture
	 * would hold the receiver until reboot.
	 */
	if (data->dap_link_ctx != NULL) {
		dap_swo_capture_stop(data->dap_link_ctx);
	}
#endif
	LOG_INF("Configuration disabled");
}

static int dap_func_init(struct usbd_class_data *c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct dap_func_desc *desc = data->desc;

	if (data->dap_link_ctx == NULL) {
		LOG_ERR("No DAP Link context provided");
		return -EIO;
	}

	LOG_DBG("Init class instance %p", (void *)c_data);

	if (usbd_add_descriptor(uds_ctx, data->iface_str_desc_nd)) {
		LOG_ERR("Failed to add interface string descriptor");
	} else {
		desc->if0.iInterface = usbd_str_desc_get_idx(data->iface_str_desc_nd);
	}

	return 0;
}

struct usbd_class_api dap_func_api = {
	.request = dap_func_request_handler,
	.get_desc = dap_func_get_desc,
	.enable = dap_func_enable,
	.disable = dap_func_disable,
	.init = dap_func_init,
};

#define DAP_FUNC_DESCRIPTOR_DEFINE(n, _)					\
static struct dap_func_desc dap_func_desc_##n = {				\
	/* Interface descriptor 0 */						\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = COND_CODE_1(CONFIG_DAP_SWO, (3), (2)),		\
		.bInterfaceClass = USB_BCC_VENDOR,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	/* Endpoint OUT */							\
	.if0_out_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Endpoint IN */							\
	.if0_in_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0x00,						\
	},									\
										\
	IF_ENABLED(CONFIG_DAP_SWO, (						\
	/* SWO trace Endpoint IN */						\
	.if0_swo_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x82,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0x00,						\
	},									\
	))									\
										\
	/* High-speed Endpoint OUT */						\
	.if0_hs_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* High-speed Endpoint IN */						\
	.if0_hs_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0x00,						\
	},									\
										\
	IF_ENABLED(CONFIG_DAP_SWO, (						\
	/* High-speed SWO trace Endpoint IN */					\
	.if0_hs_swo_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x82,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0x00,						\
	},									\
	))									\
										\
	/* Termination descriptor */						\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};										\
										\
const static struct usb_desc_header *dap_func_fs_desc_##n[] = {			\
	(struct usb_desc_header *) &dap_func_desc_##n.if0,			\
	(struct usb_desc_header *) &dap_func_desc_##n.if0_out_ep,		\
	(struct usb_desc_header *) &dap_func_desc_##n.if0_in_ep,		\
	IF_ENABLED(CONFIG_DAP_SWO, (						\
	(struct usb_desc_header *) &dap_func_desc_##n.if0_swo_in_ep,		\
	))									\
	(struct usb_desc_header *) &dap_func_desc_##n.nil_desc,			\
};										\
										\
const static struct usb_desc_header *dap_func_hs_desc_##n[] = {			\
	(struct usb_desc_header *) &dap_func_desc_##n.if0,			\
	(struct usb_desc_header *) &dap_func_desc_##n.if0_hs_out_ep,		\
	(struct usb_desc_header *) &dap_func_desc_##n.if0_hs_in_ep,		\
	IF_ENABLED(CONFIG_DAP_SWO, (						\
	(struct usb_desc_header *) &dap_func_desc_##n.if0_hs_swo_in_ep,		\
	))									\
	(struct usb_desc_header *) &dap_func_desc_##n.nil_desc,			\
};


#define DAP_FUNC_FUNCTION_DATA_DEFINE(n, _)					\
	USBD_DESC_STRING_DEFINE(iface_str_desc_nd_##n,				\
				"CMSIS-DAP v2",					\
				USBD_DUT_STRING_INTERFACE);			\
										\
	static struct dap_func_data dap_func_data_##n = {			\
		.desc = &dap_func_desc_##n,					\
		.fs_desc = dap_func_fs_desc_##n,				\
		.hs_desc = dap_func_hs_desc_##n,				\
		.iface_str_desc_nd = &iface_str_desc_nd_##n,			\
	};									\
										\
	USBD_DEFINE_CLASS(dap_func_##n, &dap_func_api, &dap_func_data_##n, NULL);

LISTIFY(1, DAP_FUNC_DESCRIPTOR_DEFINE, ())
LISTIFY(1, DAP_FUNC_FUNCTION_DATA_DEFINE, ())

#ifdef CONFIG_DAP_SWO
/*
 * Drain the trace buffer to the trace endpoint, one transfer in
 * flight at a time. Submitted by the SWO core on new capture data
 * (via the stream kick) and by the completion of the previous trace
 * transfer.
 */
static void dap_trace_work_handler(struct k_work *work)
{
	struct usbd_class_data *const c_data = &dap_func_0;
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct net_buf *buf;
	struct udc_buf_info *bi;
	size_t size;
	uint32_t len;

	if (!atomic_test_bit(&data->state, SAMPLE_FUNCTION_ENABLED)) {
		return;
	}

	if (atomic_test_and_set_bit(&data->state, DAP_FUNCTION_TRACE_BUSY)) {
		return;
	}

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		size = 512U;
	} else {
		size = 64U;
	}

	buf = net_buf_alloc_with_data(&dap_trace_pool, dap_trace_buf,
				      size, K_NO_WAIT);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate trace buffer");
		atomic_clear_bit(&data->state, DAP_FUNCTION_TRACE_BUSY);
		return;
	}

	net_buf_reset(buf);
	len = dap_swo_read(data->dap_link_ctx, dap_trace_buf, size);
	if (len == 0U) {
		net_buf_unref(buf);
		atomic_clear_bit(&data->state, DAP_FUNCTION_TRACE_BUSY);
		return;
	}

	net_buf_add(buf, len);
	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = dap_func_get_trace_in(c_data);

	if (usbd_ep_enqueue(c_data, buf)) {
		LOG_ERR("Failed to enqueue trace buffer");
		usbd_ep_buf_free(uds_ctx, buf);
		atomic_clear_bit(&data->state, DAP_FUNCTION_TRACE_BUSY);
	}
}

static void dap_usb_trace_kick(void)
{
	k_work_submit(&dap_trace_work);
}
#endif /* CONFIG_DAP_SWO */

int dap_link_backend_usb_init(struct dap_link_context *const dap_link_ctx)
{
	struct usbd_class_data *const c_data = &dap_func_0;
	struct dap_func_data *data = usbd_class_get_private(c_data);

	if (atomic_test_bit(&data->state, SAMPLE_FUNCTION_ENABLED)) {
		return -EALREADY;
	}

	data->dap_link_ctx = dap_link_ctx;

#ifdef CONFIG_DAP_SWO
	k_work_init(&dap_trace_work, dap_trace_work_handler);
	dap_swo_stream_bind(dap_link_ctx, dap_usb_trace_kick);
#endif

	return 0;
}
