/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "i2c_tiny_usb.h"

#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/minmax.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_tiny_usb, LOG_LEVEL_INF);

/*
 * USB function compatible with the i2c-tiny-usb protocol,
 * https://github.com/harbaum/I2C-Tiny-USB, understood by the Linux
 * i2c-tiny-usb bus driver. All communication is done through vendor requests
 * on the default control pipe, the interface has no other endpoints.
 *
 * bRequest encodes the command. For CMD_I2C_IO, wValue carries the Linux I2C
 * message flags, wIndex the target address, and the data stage the payload.
 * The host marks the first and last message of a transaction with the BEGIN
 * and END bits.
 */
#define CMD_ECHO       0
#define CMD_GET_FUNC   1
#define CMD_SET_DELAY  2
#define CMD_GET_STATUS 3

#define CMD_I2C_IO       4
#define CMD_I2C_IO_BEGIN BIT(0)
#define CMD_I2C_IO_END   BIT(1)

/* Status reported by CMD_GET_STATUS */
#define STATUS_IDLE        0
#define STATUS_ADDRESS_ACK 1
#define STATUS_ADDRESS_NAK 2

/* Linux I2C message flag in wValue of CMD_I2C_IO commands */
#define TINY_USB_M_RD BIT(0)

/* Linux I2C adapter functionality reported by CMD_GET_FUNC */
#define TINY_USB_FUNC_I2C        0x00000001UL
#define TINY_USB_FUNC_SMBUS_EMUL 0x0eff0008UL

#define MAX_MSGS 8U
#define MAX_DATA 256U

/*
 * Each message of a transaction arrives in a separate vendor request. They are
 * collected here and executed as a single i2c_transfer() to preserve the
 * repeated start conditions between them.
 */
struct i2c_tiny_usb_data {
	const struct device *i2c_dev;
	struct i2c_msg msgs[MAX_MSGS];
	uint8_t data[MAX_DATA];
	uint16_t used;
	uint8_t num_msgs;
	uint16_t addr;
	uint8_t status;
};

static struct i2c_tiny_usb_data bridge = {
	.i2c_dev = DEVICE_DT_GET(DT_NODELABEL(zephyr_i2c)),
	.status = STATUS_IDLE,
};

static void transaction_reset(struct i2c_tiny_usb_data *const ctx)
{
	ctx->num_msgs = 0;
	ctx->used = 0;
}

static struct i2c_msg *msg_append(struct i2c_tiny_usb_data *const ctx,
				  const struct usb_setup_packet *const setup)
{
	struct i2c_msg *msg;

	if (setup->bRequest & CMD_I2C_IO_BEGIN) {
		transaction_reset(ctx);
	}

	if (ctx->num_msgs >= MAX_MSGS || setup->wLength > sizeof(ctx->data) - ctx->used) {
		LOG_ERR("Transaction exceeds message or data limits");
		transaction_reset(ctx);
		ctx->status = STATUS_ADDRESS_NAK;
		return NULL;
	}

	if (ctx->num_msgs == 0U) {
		ctx->addr = setup->wIndex;
	} else if (ctx->addr != setup->wIndex) {
		LOG_ERR("Multiple target addresses in a transaction are not supported");
		transaction_reset(ctx);
		ctx->status = STATUS_ADDRESS_NAK;
		return NULL;
	}

	msg = &ctx->msgs[ctx->num_msgs];
	msg->buf = &ctx->data[ctx->used];
	msg->len = setup->wLength;
	msg->flags = (setup->wValue & TINY_USB_M_RD) ? I2C_MSG_READ : I2C_MSG_WRITE;

	if (ctx->num_msgs != 0U) {
		msg->flags |= I2C_MSG_RESTART;
	}

	if (setup->bRequest & CMD_I2C_IO_END) {
		msg->flags |= I2C_MSG_STOP;
	}

	ctx->num_msgs++;
	ctx->used += setup->wLength;

	return msg;
}

static void transaction_run(struct i2c_tiny_usb_data *const ctx)
{
	int err;

	err = i2c_transfer(ctx->i2c_dev, ctx->msgs, ctx->num_msgs, ctx->addr);
	if (err) {
		LOG_WRN("Transfer to address 0x%02x failed (%d)", ctx->addr, err);
		ctx->status = STATUS_ADDRESS_NAK;
	} else {
		ctx->status = STATUS_ADDRESS_ACK;
	}

	transaction_reset(ctx);
}

static int set_delay(struct i2c_tiny_usb_data *const ctx, const uint16_t delay_us)
{
	uint32_t freq_hz;
	uint8_t speed;
	int err;

	/*
	 * The host sends the clock half-period of the original bit-banging
	 * firmware, map it to the closest supported bus speed.
	 */
	freq_hz = USEC_PER_SEC / (2U * max(delay_us, 1U));
	if (freq_hz >= MHZ(1)) {
		speed = I2C_SPEED_FAST_PLUS;
	} else if (freq_hz >= KHZ(400)) {
		speed = I2C_SPEED_FAST;
	} else {
		speed = I2C_SPEED_STANDARD;
	}

	err = i2c_configure(ctx->i2c_dev, I2C_MODE_CONTROLLER | I2C_SPEED_SET(speed));
	if (err) {
		/* Not all controllers support runtime configuration */
		LOG_WRN("Failed to configure bus speed (%d), continue anyway", err);
	}

	return 0;
}

static int cmd_to_dev(const struct usb_setup_packet *const setup, const struct net_buf *const buf)
{
	struct i2c_tiny_usb_data *ctx = &bridge;
	struct i2c_msg *msg;

	if (setup->RequestType.type != USB_REQTYPE_TYPE_VENDOR) {
		return -ENOTSUP;
	}

	if (setup->wLength != 0U && buf == NULL) {
		/* Allow the data OUT stage to be received */
		return 0;
	}

	if ((setup->bRequest & ~(CMD_I2C_IO_BEGIN | CMD_I2C_IO_END)) == CMD_I2C_IO) {
		msg = msg_append(ctx, setup);
		if (msg == NULL) {
			/* The failure is reported through CMD_GET_STATUS */
			return 0;
		}

		if (buf != NULL) {
			msg->len = min(msg->len, buf->len);
			memcpy(msg->buf, buf->data, msg->len);
		}

		if (setup->bRequest & CMD_I2C_IO_END) {
			transaction_run(ctx);
		} else {
			/* Execution is deferred until the last message */
			ctx->status = STATUS_ADDRESS_ACK;
		}

		return 0;
	}

	if (setup->bRequest == CMD_SET_DELAY) {
		return set_delay(ctx, setup->wValue);
	}

	LOG_ERR("Vendor request 0x%02x to device not supported", setup->bRequest);

	return -ENOTSUP;
}

static struct net_buf *cmd_to_host(const struct usbd_context *const uds_ctx,
				   const struct usb_setup_packet *const setup)
{
	struct i2c_tiny_usb_data *ctx = &bridge;
	struct net_buf *buf;

	if (setup->RequestType.type != USB_REQTYPE_TYPE_VENDOR) {
		return NULL;
	}

	buf = usbd_ep_ctrl_data_in_alloc(uds_ctx, setup->wLength);
	if (buf == NULL) {
		return NULL;
	}

	if ((setup->bRequest & ~(CMD_I2C_IO_BEGIN | CMD_I2C_IO_END)) == CMD_I2C_IO) {
		struct i2c_msg *msg = msg_append(ctx, setup);

		if (msg != NULL) {
			transaction_run(ctx);
		}

		if (msg != NULL && ctx->status == STATUS_ADDRESS_ACK) {
			net_buf_add_mem(buf, msg->buf, msg->len);
		} else {
			/*
			 * Always return the requested length, the host
			 * driver treats a short transfer as a fatal error.
			 */
			memset(net_buf_add(buf, setup->wLength), 0, setup->wLength);
		}

		return buf;
	}

	switch (setup->bRequest) {
	case CMD_ECHO: {
		uint16_t echo = sys_cpu_to_le16(setup->wValue);

		net_buf_add_mem(buf, &echo, min(setup->wLength, sizeof(echo)));
		return buf;
	}
	case CMD_GET_FUNC: {
		uint32_t func = sys_cpu_to_le32(TINY_USB_FUNC_I2C | TINY_USB_FUNC_SMBUS_EMUL);

		net_buf_add_mem(buf, &func, min(setup->wLength, sizeof(func)));
		return buf;
	}
	case CMD_GET_STATUS:
		net_buf_add_u8(buf, ctx->status);
		return buf;
	default:
		break;
	}

	LOG_ERR("Vendor request 0x%02x to host not supported", setup->bRequest);
	net_buf_unref(buf);

	return NULL;
}

/*
 * All commands but CMD_I2C_IO are sent with wIndex set to the interface
 * number and are therefore routed to the class instance.
 */
static int class_control_to_dev(struct usbd_class_data *const c_data,
				const struct usb_setup_packet *const setup,
				const struct net_buf *const buf)
{
	return cmd_to_dev(setup, buf);
}

static struct net_buf *class_control_to_host(struct usbd_class_data *const c_data,
					     const struct usb_setup_packet *const setup)
{
	return cmd_to_host(usbd_class_get_ctx(c_data), setup);
}

/*
 * CMD_I2C_IO requests carry the target address in wIndex, so they match no
 * interface and are handled by device vendor request nodes, one for each
 * combination of the BEGIN and END bits.
 */
static int vreq_to_dev(const struct usbd_context *const uds_ctx,
		       const struct usb_setup_packet *const setup, const struct net_buf *const buf)
{
	return cmd_to_dev(setup, buf);
}

static struct net_buf *vreq_to_host(const struct usbd_context *const uds_ctx,
				    const struct usb_setup_packet *const setup)
{
	return cmd_to_host(uds_ctx, setup);
}

USBD_VREQUEST_DEFINE(i2c_io_vreq, CMD_I2C_IO, vreq_to_host, vreq_to_dev);
USBD_VREQUEST_DEFINE(i2c_io_begin_vreq, CMD_I2C_IO | CMD_I2C_IO_BEGIN, vreq_to_host, vreq_to_dev);
USBD_VREQUEST_DEFINE(i2c_io_end_vreq, CMD_I2C_IO | CMD_I2C_IO_END, vreq_to_host, vreq_to_dev);
USBD_VREQUEST_DEFINE(i2c_io_begin_end_vreq, CMD_I2C_IO | CMD_I2C_IO_BEGIN | CMD_I2C_IO_END,
		     vreq_to_host, vreq_to_dev);

int i2c_tiny_usb_register_vreqs(struct usbd_context *const uds_ctx)
{
	struct usbd_vreq_node *const nodes[] = {
		&i2c_io_vreq,
		&i2c_io_begin_vreq,
		&i2c_io_end_vreq,
		&i2c_io_begin_end_vreq,
	};
	int err;

	if (!device_is_ready(bridge.i2c_dev)) {
		LOG_ERR("I2C device is not ready");
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(nodes); i++) {
		err = usbd_device_register_vreq(uds_ctx, nodes[i]);
		if (err) {
			LOG_ERR("Failed to register vendor request 0x%02x (%d)", nodes[i]->code,
				err);
			return err;
		}
	}

	return 0;
}

struct i2c_tiny_usb_desc {
	struct usb_if_descriptor if0;
	struct usb_desc_header nil_desc;
};

static struct i2c_tiny_usb_desc bridge_desc = {
	/* Vendor specific interface without endpoints */
	.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_DESC_INTERFACE,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = 0,
			.bInterfaceClass = USB_BCC_VENDOR,
			.bInterfaceSubClass = 0,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},

	/* Termination descriptor */
	.nil_desc = {
			.bLength = 0,
			.bDescriptorType = 0,
		},
};

static const struct usb_desc_header *bridge_desc_list[] = {
	(struct usb_desc_header *)&bridge_desc.if0,
	(struct usb_desc_header *)&bridge_desc.nil_desc,
};

static void *bridge_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	/* Without endpoints the same descriptors are used at any speed */
	return bridge_desc_list;
}

static int bridge_init(struct usbd_class_data *const c_data)
{
	LOG_DBG("Init class instance %p", (void *)c_data);

	return 0;
}

static struct usbd_class_api bridge_api = {
	.control_to_dev = class_control_to_dev,
	.control_to_host = class_control_to_host,
	.get_desc = bridge_get_desc,
	.init = bridge_init,
};

USBD_DEFINE_CLASS(i2c_tiny_usb, &bridge_api, &bridge, NULL);
