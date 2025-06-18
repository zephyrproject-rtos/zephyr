/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Disable syscall tracing for all calls from this compilation unit to avoid
 * undefined symbols as the macros are not expanded recursively
 */
#define DISABLE_SYSCALL_TRACING

#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <tracing_core.h>
#include <tracing_buffer.h>
#include <tracing_backend.h>

/* Single bounce buffer for bulk IN transfer */
UDC_BUF_POOL_DEFINE(tracing_data_pool, 1, CONFIG_TRACING_BUFFER_SIZE,
		    sizeof(struct udc_buf_info), NULL);

struct tracing_func_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
};

struct tracing_func_data {
	struct tracing_func_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	struct k_sem sync_sem;
	atomic_t state;
};

#define TRACING_FUNCTION_ENABLED		0

static uint8_t tracing_func_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct tracing_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct tracing_func_desc *desc = data->desc;

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_out_ep.bEndpointAddress;
	}

	return desc->if0_out_ep.bEndpointAddress;
}

static uint8_t tracing_func_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct tracing_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct tracing_func_desc *desc = data->desc;

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_in_ep.bEndpointAddress;
	}

	return desc->if0_in_ep.bEndpointAddress;
}

static void tracing_func_out_next(struct usbd_class_data *const c_data)
{
	struct tracing_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct net_buf *buf;
	uint8_t ep;

	if (!atomic_test_bit(&data->state, TRACING_FUNCTION_ENABLED)) {
		return;
	}

	ep = tracing_func_get_bulk_out(c_data);
	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		buf = usbd_ep_buf_alloc(c_data, ep, 512);
	} else {
		buf = usbd_ep_buf_alloc(c_data, ep, 64);
	}

	if (buf == NULL) {
		return;
	}

	if (usbd_ep_enqueue(c_data, buf)) {
		net_buf_unref(buf);
	}
}

static int tracing_func_request_handler(struct usbd_class_data *const c_data,
					struct net_buf *const buf, const int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct tracing_func_data *data = usbd_class_get_private(c_data);
	struct udc_buf_info *bi = NULL;

	bi = (struct udc_buf_info *)net_buf_user_data(buf);

	if (bi->ep == tracing_func_get_bulk_out(c_data)) {
		if (!err) {
			tracing_cmd_handle(buf->data, buf->len);
		}

		usbd_ep_buf_free(uds_ctx, buf);
		tracing_func_out_next(c_data);
	}

	if (bi->ep == tracing_func_get_bulk_in(c_data)) {
		usbd_ep_buf_free(uds_ctx, buf);
		k_sem_give(&data->sync_sem);
	}

	return 0;
}

static void *tracing_func_get_desc(struct usbd_class_data *const c_data,
				   const enum usbd_speed speed)
{
	struct tracing_func_data *data = usbd_class_get_private(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

static void tracing_func_enable(struct usbd_class_data *const c_data)
{
	struct tracing_func_data *data = usbd_class_get_private(c_data);

	if (!atomic_test_and_set_bit(&data->state, TRACING_FUNCTION_ENABLED)) {
		tracing_func_out_next(c_data);
	}
}

static void tracing_func_disable(struct usbd_class_data *const c_data)
{
	struct tracing_func_data *data = usbd_class_get_private(c_data);

	atomic_clear_bit(&data->state, TRACING_FUNCTION_ENABLED);
}

static int tracing_func_init(struct usbd_class_data *c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

struct usbd_class_api tracing_func_api = {
	.request = tracing_func_request_handler,
	.get_desc = tracing_func_get_desc,
	.enable = tracing_func_enable,
	.disable = tracing_func_disable,
	.init = tracing_func_init,
};

static struct tracing_func_desc func_desc = {
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_DESC_INTERFACE,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_BCC_VENDOR,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},

	.if0_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x81,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(64),
		.bInterval = 0x00,
	},

	.if0_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x01,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(64),
		.bInterval = 0x00,
	},

	/* High-speed Endpoint IN */
	.if0_hs_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x81,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(512),
		.bInterval = 0x00,
	},

	/* High-speed Endpoint OUT */
	.if0_hs_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_DESC_ENDPOINT,
		.bEndpointAddress = 0x01,
		.bmAttributes = USB_EP_TYPE_BULK,
		.wMaxPacketSize = sys_cpu_to_le16(512),
		.bInterval = 0x00,
	},
};

const static struct usb_desc_header *tracing_func_fs_desc[] = {
	(struct usb_desc_header *) &func_desc.if0,
	(struct usb_desc_header *) &func_desc.if0_in_ep,
	(struct usb_desc_header *) &func_desc.if0_out_ep,
	NULL,
};

const static __maybe_unused struct usb_desc_header *tracing_func_hs_desc[] = {
	(struct usb_desc_header *) &func_desc.if0,
	(struct usb_desc_header *) &func_desc.if0_hs_in_ep,
	(struct usb_desc_header *) &func_desc.if0_hs_out_ep,
	NULL,
};

static struct tracing_func_data func_data = {
	.desc = &func_desc,
	.fs_desc = tracing_func_fs_desc,
	.hs_desc = COND_CODE_1(USBD_SUPPORTS_HIGH_SPEED, (tracing_func_hs_desc), (NULL)),
	.sync_sem = Z_SEM_INITIALIZER(func_data.sync_sem, 0, 1),
};

USBD_DEFINE_CLASS(tracing_func, &tracing_func_api, &func_data, NULL);

struct net_buf *tracing_func_buf_alloc(struct usbd_class_data *const c_data)
{
	struct udc_buf_info *bi;
	struct net_buf *buf;

	buf = net_buf_alloc(&tracing_data_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	bi->ep = tracing_func_get_bulk_in(c_data);

	return buf;
}

static void tracing_backend_usb_output(const struct tracing_backend *backend,
				       uint8_t *data, uint32_t length)
{
	int ret = 0;
	uint32_t bytes;
	struct net_buf *buf;

	while (length > 0) {
		if (!atomic_test_bit(&func_data.state, TRACING_FUNCTION_ENABLED) ||
		    !is_tracing_enabled()) {
			return;
		}

		buf = tracing_func_buf_alloc(&tracing_func);
		if (buf == NULL) {
			return;
		}

		bytes = MIN(length, net_buf_tailroom(buf));
		net_buf_add_mem(buf, data, bytes);

		ret = usbd_ep_enqueue(&tracing_func, buf);
		if (ret) {
			net_buf_unref(buf);
			continue;
		}

		data += bytes;
		length -= bytes;
		k_sem_take(&func_data.sync_sem, K_FOREVER);
	}
}

const struct tracing_backend_api tracing_backend_usb_api = {
	.output = tracing_backend_usb_output
};

TRACING_BACKEND_DEFINE(tracing_backend_usb, tracing_backend_usb_api);
