/*
 * Copyright (c) 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(ec_host_cmd_usb, LOG_LEVEL_DBG);

BUILD_ASSERT((CONFIG_EC_HOST_CMD_HANDLER_BUFFER_ALIGN % UDC_BUF_ALIGN) == 0, "Buffers not aligned");

#define USB_SUBCLASS_GOOGLE_EC_HOST_CMD 0x5a
#define USB_PROTOCOL_GOOGLE_EC_HOST_CMD 0x00

/* Supported version of host commands protocol. */
#define EC_HOST_REQUEST_VERSION 3

#define EC_HOST_CMD_SEND_TIMEOUT_MS 100

#define EP_BULK_SIZE     64U
#define EP_INT_SIZE_SIZE 4U

#define INT_POOL_SIZE 4

/* Timeout for receiving entire request. */
#define OUT_TRANSFER_TIMEOUT_MS 100
/* Timeout for sending response, starting from notifing host that it is ready. */
#define IN_TRANSFER_TIMEOUT_MS  200

enum ec_host_cmd_usb_irq_type {
	USB_EC_HOST_CMD_IRQ_TYPE_EVENT = 0,
	USB_EC_HOST_CMD_IRQ_TYPE_RESP_READY = 1,
};

NET_BUF_POOL_DEFINE(ec_host_cmd_ep_pool, 2, 0, sizeof(struct udc_buf_info), NULL);
UDC_BUF_POOL_DEFINE(int_ep_pool, INT_POOL_SIZE, EP_INT_SIZE_SIZE, sizeof(struct udc_buf_info),
		    NULL);

enum ec_host_cmd_usb_state {
	/*
	 * Host commands not enabled.
	 */
	USB_EC_HOST_CMD_STATE_DISABLED,

	/*
	 * USB interface is enabled and ready to receive host request.
	 * Once the response is sent, the current state is reset to this state to accept a next
	 * request.
	 */
	USB_EC_HOST_CMD_STATE_READY_TO_RX,

	/*
	 * Receiving ongoing. The first part of the host command request has been received,
	 * potentialy waiting for the rest.
	 */
	USB_EC_HOST_CMD_STATE_RECEIVING,

	/*
	 * The host command request has been fully received and the command is being proccessed.
	 * The host command handler always has to send a respond, even if the request is invalid.
	 */
	USB_EC_HOST_CMD_STATE_PROCESSING,

	/*
	 * Processing is finished, the response is being sent.
	 */
	USB_EC_HOST_CMD_STATE_SENDING,
};

enum {
	EC_HOST_CMD_CLASS_ENABLED,
};

static const char *const state_name[] = {
	[USB_EC_HOST_CMD_STATE_DISABLED] = "DISABLED",
	[USB_EC_HOST_CMD_STATE_READY_TO_RX] = "READY_TO_RX",
	[USB_EC_HOST_CMD_STATE_RECEIVING] = "RECEIVING",
	[USB_EC_HOST_CMD_STATE_PROCESSING] = "PROCESSING",
	[USB_EC_HOST_CMD_STATE_SENDING] = "SENDING",
};

struct ec_host_cmd_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor out_ep;
	struct usb_ep_descriptor in_bulk_ep;
	struct usb_ep_descriptor in_int_ep;
	struct usb_desc_header nil_desc;
};

struct ec_host_cmd_usb_ctx {
	struct usbd_class_data *c_data;
	struct ec_host_cmd_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	struct ec_host_cmd_rx_ctx *rx_ctx;
	struct ec_host_cmd_tx_buf *tx_buf;
	uint8_t *bulk_out_buf;
	struct net_buf *usb_rx_buf;
	struct net_buf *usb_tx_buf;
	enum ec_host_cmd_usb_state state;
	atomic_t class_state;
	bool pending_event;
	struct k_work_delayable reset_work;
};

static int expected_request_len(const struct ec_host_cmd_request_header *header)
{
	/* Check host request version */
	if (header->prtcl_ver != EC_HOST_REQUEST_VERSION) {
		return 0;
	}

	/* Reserved byte should be 0 */
	if (header->reserved) {
		return 0;
	}

	return sizeof(*header) + header->data_len;
}

static inline uint8_t ec_host_cmd_get_out_ep(struct usbd_class_data *const c_data)
{
	struct ec_host_cmd_usb_ctx *ctx = usbd_class_get_private(c_data);
	struct ec_host_cmd_desc *desc = ctx->desc;

	return desc->out_ep.bEndpointAddress;
}

static inline uint8_t ec_host_cmd_get_in_bulk_ep(struct usbd_class_data *const c_data)
{
	struct ec_host_cmd_usb_ctx *ctx = usbd_class_get_private(c_data);
	struct ec_host_cmd_desc *desc = ctx->desc;

	return desc->in_bulk_ep.bEndpointAddress;
}

static inline uint8_t ec_host_cmd_get_in_int_ep(struct usbd_class_data *const c_data)
{
	struct ec_host_cmd_usb_ctx *ctx = usbd_class_get_private(c_data);
	struct ec_host_cmd_desc *desc = ctx->desc;

	return desc->in_int_ep.bEndpointAddress;
}

static struct net_buf *ec_host_cmd_buf_alloc(const uint8_t ep, const size_t size, void *const data)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	__ASSERT(IS_UDC_ALIGNED(data), "Application provided unaligned buffer");

	buf = net_buf_alloc_with_data(&ec_host_cmd_ep_pool, data, size, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	bi->ep = ep;

	if (USB_EP_DIR_IS_OUT(ep)) {
		buf->len = 0;
	}

	return buf;
}

static struct net_buf *ec_host_cmd_buf_alloc_int(const uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&int_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	bi->ep = ep;

	return buf;
}

static int ec_host_cmd_signal_event(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct ec_host_cmd_usb_ctx *ctx = usbd_class_get_private(c_data);
	struct net_buf *buf;
	int ret;

	buf = ec_host_cmd_buf_alloc_int(ec_host_cmd_get_in_int_ep(c_data));
	if (buf == NULL) {
		LOG_WRN("Event has been already signaled but not polled by the host");
		return 0;
	}
	net_buf_add_u8(buf, USB_EC_HOST_CMD_IRQ_TYPE_EVENT);
	buf->len = EP_INT_SIZE_SIZE;
	ret = usbd_ep_enqueue(c_data, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue EP IN INT: %d", ret);
		usbd_ep_buf_free(uds_ctx, buf);
		return ret;
	}
	ctx->pending_event = false;

	return 0;
}

static int handle_out_transfer(struct usbd_class_data *const c_data)
{
	int ret;
	struct ec_host_cmd_usb_ctx *ctx = usbd_class_get_private(c_data);
	size_t expected_len =
		expected_request_len((struct ec_host_cmd_request_header *)ctx->rx_ctx->buf);

	/* Notify about a new command or response with a proper error code. */
	if ((ctx->rx_ctx->len >= expected_len) || (expected_len == 0) ||
	    (expected_len > ctx->rx_ctx->len_max)) {
		k_work_cancel_delayable(&ctx->reset_work);
		if (ctx->rx_ctx->len > expected_len) {
			LOG_ERR("Received incorrect number of bytes, got: %d, expected: %d",
				ctx->rx_ctx->len, expected_len);
		}
		ctx->state = USB_EC_HOST_CMD_STATE_PROCESSING;
		ec_host_cmd_rx_notify();
	}

	/* Enqueue OUT transfer if we are still receiving. */
	if (ctx->state == USB_EC_HOST_CMD_STATE_RECEIVING) {
		if (!IS_UDC_ALIGNED(ctx->rx_ctx->len)) {
			LOG_ERR("Received unaligned OUT transfer: %d", ctx->rx_ctx->len);
			k_work_reschedule(&ctx->reset_work, K_NO_WAIT);
			return 0;
		}
		ctx->usb_rx_buf = ec_host_cmd_buf_alloc(ec_host_cmd_get_out_ep(c_data),
							expected_len - ctx->rx_ctx->len,
							ctx->rx_ctx->buf + ctx->rx_ctx->len);
		if (ctx->usb_rx_buf == NULL) {
			LOG_ERR("Failed to allocate buf OUT");
			k_work_reschedule(&ctx->reset_work, K_NO_WAIT);
			return 0;
		}
		ret = usbd_ep_enqueue(c_data, ctx->usb_rx_buf);
		if (ret) {
			net_buf_unref(ctx->usb_rx_buf);
			k_work_reschedule(&ctx->reset_work, K_NO_WAIT);
			LOG_ERR("Failed to enqueue EP OUT: %d", ret);
			return 0;
		}
	}

	return 0;
}

static int ec_host_cmd_request(struct usbd_class_data *const c_data, struct net_buf *const buf,
			       int err)
{
	struct ec_host_cmd_usb_ctx *ctx = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct udc_buf_info *bi = NULL;
	int ret;

	bi = udc_get_buf_info(buf);

	if (err) {
		if (err == -ECONNABORTED) {
			LOG_WRN("Request EP 0x%02x cancelled", bi->ep);
		} else {
			LOG_ERR("Request EP 0x%02x failed: %d", bi->ep, err);
		}

		if ((bi->ep == ec_host_cmd_get_in_int_ep(c_data)) ||
		    (bi->ep == ec_host_cmd_get_out_ep(c_data))) {
			usbd_ep_buf_free(uds_ctx, buf);
			usbd_ep_buf_free(uds_ctx, buf);
		}

		return 0;
	}

	if (bi->ep == ec_host_cmd_get_out_ep(c_data)) {
		uint16_t buf_len = buf->len;

		ret = usbd_ep_buf_free(uds_ctx, buf);
		if (ret) {
			LOG_ERR("Failed to free buf OUT");
			/* Schedule the reset work even if it hasn't been queued */
			k_work_reschedule(&ctx->reset_work, K_NO_WAIT);
			return ret;
		}

		if (ctx->state == USB_EC_HOST_CMD_STATE_READY_TO_RX) {
			if (buf_len < sizeof(struct ec_host_cmd_request_header)) {
				LOG_ERR("First transfer less than header: %d", buf_len);
				k_work_schedule(&ctx->reset_work, K_NO_WAIT);
				return 0;
			}
			ctx->state = USB_EC_HOST_CMD_STATE_RECEIVING;
			ctx->rx_ctx->len = 0;
			k_work_schedule(&ctx->reset_work, K_MSEC(OUT_TRANSFER_TIMEOUT_MS));
		}

		if (ctx->state != USB_EC_HOST_CMD_STATE_RECEIVING) {
			LOG_ERR("Unexpected transfer in state: %s", state_name[ctx->state]);
			return 0;
		}

		ctx->rx_ctx->len = ctx->rx_ctx->len + buf_len;

		return handle_out_transfer(c_data);
	}

	if (bi->ep == ec_host_cmd_get_in_bulk_ep(c_data)) {
		k_work_cancel_delayable(&ctx->reset_work);
		ctx->usb_rx_buf = ec_host_cmd_buf_alloc(ec_host_cmd_get_out_ep(c_data),
							EP_BULK_SIZE, ctx->rx_ctx->buf);
		if (ctx->usb_rx_buf == NULL) {
			LOG_ERR("Failed to allocate buf OUT");
			k_work_schedule(&ctx->reset_work, K_NO_WAIT);
			return 0;
		}

		ret = usbd_ep_enqueue(c_data, ctx->usb_rx_buf);
		if (ret) {
			LOG_ERR("Failed to enqueue EP OUT: %d", ret);
			net_buf_unref(ctx->usb_rx_buf);
			k_work_schedule(&ctx->reset_work, K_NO_WAIT);
			return 0;
		}

		ctx->state = USB_EC_HOST_CMD_STATE_READY_TO_RX;
	}

	if (bi->ep == ec_host_cmd_get_in_int_ep(c_data)) {
		usbd_ep_buf_free(uds_ctx, buf);
	}

	return 0;
}

static void *ec_host_cmd_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	const struct ec_host_cmd_usb_ctx *ctx = usbd_class_get_private(c_data);

	if (speed == USBD_SPEED_HS) {
		return NULL;
	}

	return ctx->fs_desc;
}

static void ec_host_cmd_enable(struct usbd_class_data *const c_data)
{
	int ret;
	struct ec_host_cmd_usb_ctx *ctx = usbd_class_get_private(c_data);
	struct udc_buf_info *bi;
	struct net_buf *buf;

	atomic_set_bit(&ctx->class_state, EC_HOST_CMD_CLASS_ENABLED);
	if (!ctx->usb_tx_buf) {
		LOG_ERR("Host Commands not initaliazed");
		return;
	}

	/* Update EP IN address. Buf is allocated in the backend init procedure. */
	bi = udc_get_buf_info(ctx->usb_tx_buf);
	bi->ep = ec_host_cmd_get_in_bulk_ep(c_data);

	buf = ec_host_cmd_buf_alloc(ec_host_cmd_get_out_ep(c_data), EP_BULK_SIZE, ctx->rx_ctx->buf);
	if (buf == NULL) {
		ctx->state = USB_EC_HOST_CMD_STATE_DISABLED;
		LOG_ERR("Failed to allocate buf OUT");
		return;
	}
	ctx->usb_rx_buf = buf;

	/* Enqueue OUT transfer to receive a host command request. */
	ret = usbd_ep_enqueue(c_data, ctx->usb_rx_buf);
	if (ret) {
		ctx->state = USB_EC_HOST_CMD_STATE_DISABLED;
		LOG_ERR("Failed to enqueue EP OUT: %d", ret);
		net_buf_unref(ctx->usb_rx_buf);
		return;
	}
	ctx->state = USB_EC_HOST_CMD_STATE_READY_TO_RX;

	if (ctx->pending_event) {
		ec_host_cmd_signal_event(c_data);
	}

	LOG_INF("Configuration enabled");
}

static void ec_host_cmd_resumed(struct usbd_class_data *const c_data)
{
	LOG_DBG("Configuration resumed");
	struct ec_host_cmd_usb_ctx *ctx = usbd_class_get_private(c_data);

	if (ctx->pending_event) {
		ec_host_cmd_signal_event(c_data);
	}
}

static void ec_host_cmd_suspended(struct usbd_class_data *const c_data)
{
	LOG_DBG("Configuration suspended");
}

static void ec_host_cmd_disable(struct usbd_class_data *const c_data)
{
	struct ec_host_cmd_usb_ctx *ctx = usbd_class_get_private(c_data);

	atomic_clear_bit(&ctx->class_state, EC_HOST_CMD_CLASS_ENABLED);

	k_work_cancel_delayable(&ctx->reset_work);
	if (ctx->state != USB_EC_HOST_CMD_STATE_READY_TO_RX) {
		LOG_WRN("Disabled usb in state %s", state_name[ctx->state]);
	}
	ctx->state = USB_EC_HOST_CMD_STATE_DISABLED;
}

static int ec_host_cmd_usbd_init(struct usbd_class_data *c_data)
{
	LOG_DBG("Class init");

	return 0;
}

static void ec_host_cmd_reset(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ec_host_cmd_usb_ctx *ctx =
		CONTAINER_OF(dwork, struct ec_host_cmd_usb_ctx, reset_work);
	struct usbd_class_data *const c_data = ctx->c_data;
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	int ret;

	LOG_INF("Resetting backend in state %s", state_name[ctx->state]);

	if (!ctx->usb_tx_buf) {
		LOG_ERR("Host Commands not initaliazed");
		return;
	}

	ctx->state = USB_EC_HOST_CMD_STATE_DISABLED;
	ret = usbd_ep_dequeue(uds_ctx, ec_host_cmd_get_out_ep(c_data));
	if (ret) {
		LOG_ERR("Failed to dequeue EP OUT: %d", ret);
		return;
	}
	ret = usbd_ep_dequeue(uds_ctx, ec_host_cmd_get_in_bulk_ep(c_data));
	if (ret) {
		LOG_ERR("Failed to dequeue EP IN: %d", ret);
		return;
	}
	ret = usbd_ep_dequeue(uds_ctx, ec_host_cmd_get_in_int_ep(c_data));
	if (ret) {
		LOG_ERR("Failed to dequeue EP IN INT: %d", ret);
		return;
	}

	ec_host_cmd_enable(ctx->c_data);
}

__maybe_unused static struct usbd_class_api ec_host_cmd_api = {
	.request = ec_host_cmd_request,
	.suspended = ec_host_cmd_suspended,
	.resumed = ec_host_cmd_resumed,
	.enable = ec_host_cmd_enable,
	.disable = ec_host_cmd_disable,
	.get_desc = ec_host_cmd_get_desc,
	.init = ec_host_cmd_usbd_init,
};

static int ec_host_cmd_backend_init(const struct ec_host_cmd_backend *backend,
				    struct ec_host_cmd_rx_ctx *rx_ctx,
				    struct ec_host_cmd_tx_buf *tx)
{
	struct ec_host_cmd_usb_ctx *ctx = backend->ctx;
	struct usbd_class_data *const c_data = ctx->c_data;
	struct net_buf *buf;

	ctx->rx_ctx = rx_ctx;
	ctx->tx_buf = tx;

	ctx->state = USB_EC_HOST_CMD_STATE_DISABLED;

	k_work_init_delayable(&ctx->reset_work, ec_host_cmd_reset);

	if ((ctx->rx_ctx->buf == NULL) || (ctx->tx_buf->buf == NULL)) {
		LOG_ERR("Buffers not provided");
		return -EINVAL;
	}

	/* EPs addresses can be changed, but it is updated in the enable procedure. */
	buf = ec_host_cmd_buf_alloc(ec_host_cmd_get_in_bulk_ep(c_data), ctx->tx_buf->len_max,
				    ctx->tx_buf->buf);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buf IN");
		return -ENOMEM;
	}
	ctx->usb_tx_buf = buf;

	return 0;
}

static int ec_host_cmd_backend_send(const struct ec_host_cmd_backend *backend)
{
	struct ec_host_cmd_usb_ctx *ctx = backend->ctx;
	struct usbd_class_data *const c_data = ctx->c_data;
	struct net_buf *buf = NULL;
	int ret;

	if (!atomic_test_bit(&ctx->class_state, EC_HOST_CMD_CLASS_ENABLED)) {
		LOG_ERR("Class not enabled");
		return -EACCES;
	}

	if (ctx->state != USB_EC_HOST_CMD_STATE_PROCESSING) {
		LOG_ERR("Unexpected state when sending: %s", state_name[ctx->state]);
		return -EACCES;
	}

	ctx->state = USB_EC_HOST_CMD_STATE_SENDING;

	k_work_schedule(&ctx->reset_work, K_MSEC(IN_TRANSFER_TIMEOUT_MS));

	net_buf_reset(ctx->usb_tx_buf);
	ctx->usb_tx_buf->len = ctx->tx_buf->len;
	ret = usbd_ep_enqueue(c_data, ctx->usb_tx_buf);
	if (ret) {
		LOG_ERR("Failed to enqueue EP IN: %d", ret);
		k_work_reschedule(&ctx->reset_work, K_NO_WAIT);
		return ret;
	}

	buf = ec_host_cmd_buf_alloc_int(ec_host_cmd_get_in_int_ep(c_data));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buf INT OUT: %d", ret);
		k_work_reschedule(&ctx->reset_work, K_NO_WAIT);
		return -ENOMEM;
	}

	/* Signal to host that response is ready. */
	net_buf_add_u8(buf, USB_EC_HOST_CMD_IRQ_TYPE_RESP_READY);
	buf->len = EP_INT_SIZE_SIZE;

	ret = usbd_ep_enqueue(c_data, buf);
	if (ret) {
		net_buf_unref(buf);
		LOG_ERR("Failed to enqueue EP INT OUT: %d", ret);
		k_work_reschedule(&ctx->reset_work, K_NO_WAIT);
		return ret;
	}

	return ret;
}

static const struct ec_host_cmd_backend_api ec_host_cmd_backend_api = {
	.init = ec_host_cmd_backend_init,
	.send = ec_host_cmd_backend_send,
};

BUILD_ASSERT(!USBD_SUPPORTS_HIGH_SPEED, "High speed is not supported");

static struct ec_host_cmd_desc ec_host_cmd_desc = {
	.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_DESC_INTERFACE,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = 3,
			.bInterfaceClass = USB_BCC_VENDOR,
			.bInterfaceSubClass = USB_SUBCLASS_GOOGLE_EC_HOST_CMD,
			.bInterfaceProtocol = USB_PROTOCOL_GOOGLE_EC_HOST_CMD,
			.iInterface = 0,
		},
	.out_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_DESC_ENDPOINT,
			.bEndpointAddress = 0x01,
			.bmAttributes = USB_EP_TYPE_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(EP_BULK_SIZE),
			.bInterval = 0x00,
		},
	.in_bulk_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_DESC_ENDPOINT,
			.bEndpointAddress = 0x81,
			.bmAttributes = USB_EP_TYPE_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(EP_BULK_SIZE),
			.bInterval = 0x00,
		},
	.in_int_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_DESC_ENDPOINT,
			.bEndpointAddress = 0x82,
			.bmAttributes = USB_EP_TYPE_INTERRUPT,
			.wMaxPacketSize = sys_cpu_to_le16(EP_INT_SIZE_SIZE),
			.bInterval = USB_FS_INT_EP_INTERVAL(1000),
		},
	.nil_desc = {
			.bLength = 0,
			.bDescriptorType = 0,
		},
};

static const struct usb_desc_header *ec_host_cmd_fs_desc[] = {
	(struct usb_desc_header *)&ec_host_cmd_desc.if0,
	(struct usb_desc_header *)&ec_host_cmd_desc.out_ep,
	(struct usb_desc_header *)&ec_host_cmd_desc.in_bulk_ep,
	(struct usb_desc_header *)&ec_host_cmd_desc.in_int_ep,
	(struct usb_desc_header *)&ec_host_cmd_desc.nil_desc,
};

static struct ec_host_cmd_usb_ctx ec_host_cmd_ctx;

USBD_DEFINE_CLASS(ec_host_cmd_class, &ec_host_cmd_api, (void *)&ec_host_cmd_ctx, NULL);

static uint8_t bulk_out_buf[EP_BULK_SIZE] __aligned(UDC_BUF_ALIGN);
static struct ec_host_cmd_usb_ctx ec_host_cmd_ctx = {
	.desc = &ec_host_cmd_desc,
	.fs_desc = ec_host_cmd_fs_desc,
	.c_data = &ec_host_cmd_class,
	.bulk_out_buf = bulk_out_buf,
};

static struct ec_host_cmd_backend usb_ec_host_cmd_backend = {
	.api = &ec_host_cmd_backend_api,
	.ctx = &ec_host_cmd_ctx,
};

struct ec_host_cmd_backend *ec_host_cmd_backend_get_usb(void)
{
	return &usb_ec_host_cmd_backend;
}

#ifdef CONFIG_EC_HOST_CMD_INITIALIZE_AT_BOOT
static int host_cmd_init(void)
{
	ec_host_cmd_init(ec_host_cmd_backend_get_usb());
	return 0;
}
SYS_INIT(host_cmd_init, POST_KERNEL, CONFIG_EC_HOST_CMD_INIT_PRIORITY);
#endif

void ec_host_cmd_backend_usb_trigger_event(void)
{
	int ret;
	struct ec_host_cmd_usb_ctx *ctx = ec_host_cmd_backend_get_usb()->ctx;
	struct usbd_class_data *const c_data = ctx->c_data;
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	ctx->pending_event = true;
	if (!atomic_test_bit(&ctx->class_state, EC_HOST_CMD_CLASS_ENABLED)) {
		return;
	}

	if (usbd_is_suspended(uds_ctx)) {
		if (uds_ctx->status.rwup) {
			ret = usbd_wakeup_request(uds_ctx);
			if (ret) {
				LOG_ERR("Failed to wake-up host %d", ret);
			}
		}
	} else {
		ec_host_cmd_signal_event(c_data);
	}
}
