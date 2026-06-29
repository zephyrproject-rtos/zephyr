/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>

#include <zephyr/drivers/usb/uhc.h>

#include "usbh_class.h"
#include "usbh_ch9.h"
#include "usbh_desc.h"
#include "usbh_device.h"
#include "usbh_testusb.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_testusb, CONFIG_USBH_TESTUSB_LOG_LEVEL);

/* Host test function driver, the counterpart to the device side loopback */

#define TESTUSB_VID			0x2fe3
#define TESTUSB_PID			0x0009

#define TESTUSB_VENDOR_REQ_OUT		0x5b
#define TESTUSB_VENDOR_REQ_IN		0x5c

#define TESTUSB_CTRL_MAX_LEN		1024

/* Timeout to wait for queued transfers to drain when the device is removed */
#define TESTUSB_DRAIN_TIMEOUT		K_MSEC(1000)

/* Transfer test timeout */
#define TESTUSB_IO_TIMEOUT		K_MSEC(10000)

/* Length of the single transfer used to probe the halt feature */
#define TESTUSB_HALT_CHECK_IO_LEN	512

#define TESTUSB_BUF_COUNT							\
	(CONFIG_USBH_TESTUSB_QUEUE_DEPTH * CONFIG_USBH_TESTUSB_INSTANCES_COUNT)

#define TESTUSB_BUF_POOL_SIZE							\
	((CONFIG_USBH_TESTUSB_QUEUE_DEPTH + 1) *				\
	 CONFIG_USBH_TESTUSB_INSTANCES_COUNT *					\
	 USB_BUF_ROUND_UP(CONFIG_USBH_TESTUSB_BUF_SIZE))

USB_BUF_POOL_VAR_DEFINE(testusb_buf_pool,
			TESTUSB_BUF_COUNT, TESTUSB_BUF_POOL_SIZE, 0, NULL);

struct testusb_data {
	struct usb_device *udev;
	struct usbh_class_data *c_data;
	/* Bulk IN and OUT endpoints */
	uint8_t in_ep;
	uint8_t out_ep;
	/* Interrupt IN and OUT endpoints */
	uint8_t int_in_ep;
	uint8_t int_out_ep;
	/* Current test parameter */
	uint8_t ep;
	uint32_t max;
	uint32_t length;
	uint32_t vary;
	/* Number of transfers still to submit */
	atomic_t to_submit;
	/*
	 * Current test completed transfers. Updated in the completion callback
	 * only and read by the test thread once all transfers have drained.
	 */
	uint32_t success;
	uint32_t error;
	int first_err;
};

static const struct usbh_class_filter testusb_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_VID_PID,
		.vid = TESTUSB_VID,
		.pid = TESTUSB_PID,
	},
	{0},
};

static struct usbh_class_api usbh_testusb_api;
static int testusb_xfer_cb(struct usb_device *udev, struct uhc_transfer *xfer);

/* Advance len by vary, wrapping within [1, max], like Linux usbtest. */
static uint32_t testusb_vary_len(uint32_t len, const uint32_t vary, const uint32_t max)
{
	uint64_t next;

	if (vary == 0 || max == 0) {
		return len;
	}

	next = (uint64_t)len + vary;
	next %= max;
	if (next == 0) {
		next = max;
	}

	return (uint32_t)next;
}

/*
 * Submit the next transfer of the current test. Returns 0 once all transfers
 * are finished. To be called from completion callback testusb_xfer_cb() and
 * testusb_io().
 */
static int testusb_submit(struct testusb_data *const data)
{
	struct usb_device *const udev = data->udev;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	atomic_val_t left;
	int ret;

	if (udev == NULL) {
		return -ENODEV;
	}

	/* Claim next transfer slot */
	left = atomic_dec(&data->to_submit);
	if (left <= 0) {
		return 0;
	}

	xfer = usbh_xfer_alloc(udev, data->ep, testusb_xfer_cb, data);
	if (xfer == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	buf = net_buf_alloc_len(&testusb_buf_pool, data->length, K_NO_WAIT);
	if (buf == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	if (USB_EP_DIR_IS_OUT(data->ep)) {
		for (uint32_t i = 0; i < data->length; i++) {
			net_buf_add_u8(buf, (uint8_t)i);
		}
	}

	ret = usbh_xfer_buf_add(udev, xfer, buf);
	if (ret != 0) {
		net_buf_unref(buf);
		goto err;
	}

	usbh_class_xfer_anchor(data->c_data, xfer);

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		usbh_class_xfer_unanchor(xfer);
		net_buf_unref(buf);
		goto err;
	}

	data->length = testusb_vary_len(data->length, data->vary, data->max);

	return 0;

err:
	if (xfer != NULL) {
		(void)uhc_xfer_unref(xfer);
	}

	atomic_clear(&data->to_submit);
	return ret;
}

static int testusb_xfer_cb(struct usb_device *const udev,
			   struct uhc_transfer *const xfer)
{
	struct testusb_data *const data = xfer->priv;
	const int err = xfer->err;

	net_buf_unref(xfer->buf);
	(void)uhc_xfer_unref(xfer);

	if (err == 0) {
		data->success++;
	} else {
		data->error++;
		if (data->first_err == 0) {
			data->first_err = err;
		}

		/* Stop re-submitting on the first error */
		atomic_clear(&data->to_submit);
	}

	/* Continue until all transfers have been submitted */
	(void)testusb_submit(data);

	return 0;
}

/*
 * Run a bulk or interrupt transfer test on the given endpoint.
 *
 * The minimum of the parameter param->count and the parameter depth determines
 * the number of transfers queued immediately. The completion callback then
 * continues until the count is reached.
 */
static int testusb_io(struct testusb_data *const data,
		      const uint8_t ep, const uint32_t depth,
		      const struct usbh_testusb_param *const param)
{
	int err = 0;

	if (param->length == 0 || param->length > CONFIG_USBH_TESTUSB_BUF_SIZE) {
		return -EINVAL;
	}

	data->ep = ep;
	data->max = param->length;
	data->length = param->length;
	data->vary = param->vary;
	data->success = 0;
	data->error = 0;
	data->first_err = 0;
	atomic_set(&data->to_submit, (atomic_val_t)param->count);

	for (uint32_t i = 0; i < MIN(param->count, depth); i++) {
		err = testusb_submit(data);
		if (err != 0) {
			break;
		}
	}

	/* Wait for all transfers, including those re-submitted in the callback. */
	if (usbh_class_xfer_drain(data->c_data, TESTUSB_IO_TIMEOUT) != 0) {
		LOG_ERR("Bulk/Interrupt transfer test 0x%02x timed out", ep);

		/* Stop submitting, cancel anchored transfers and let them drain. */
		atomic_clear(&data->to_submit);
		usbh_class_xfer_dequeue_all_anchored(data->c_data);
		(void)usbh_class_xfer_drain(data->c_data, TESTUSB_DRAIN_TIMEOUT);
		if (data->first_err == 0) {
			data->first_err = -ETIMEDOUT;
		}

		return data->first_err;
	}

	if (err != 0 && data->first_err == 0) {
		data->first_err = err;
	}

	LOG_DBG("Transfer test finished, ep 0x%02x, %u transfers, %u errors",
		ep, data->success + data->error, data->error);

	return data->first_err;
}

/*
 * Run control OUT and IN requests in a loop. The number of iterations and the
 * length of the DATA stage varies based on the test parameter.
 */
static int testusb_control(struct testusb_data *const data,
			   const struct usbh_testusb_param *const param)
{
	const uint8_t bmRequestType = (USB_REQTYPE_TYPE_VENDOR << 5) |
				      USB_REQTYPE_RECIPIENT_DEVICE;
	struct usb_device *const udev = data->udev;
	uint32_t length = param->length;
	int err = 0;

	if (param->length == 0 ||
	    param->length > MIN(CONFIG_USBH_TESTUSB_BUF_SIZE, TESTUSB_CTRL_MAX_LEN)) {
		return -EINVAL;
	}

	for (uint32_t i = 0; i < param->count; i++) {
		struct net_buf *buf;

		buf = usbh_xfer_buf_alloc(udev, length);
		if (buf == NULL) {
			return -ENOMEM;
		}

		for (uint32_t j = 0; j < length; j++) {
			net_buf_add_u8(buf, i + j);
		}

		err = usbh_req_setup(udev,
				     bmRequestType | (USB_REQTYPE_DIR_TO_DEVICE << 7),
				     TESTUSB_VENDOR_REQ_OUT, 0, 0,
				     length,
				     buf);
		usbh_xfer_buf_free(udev, buf);
		if (err != 0) {
			LOG_ERR("Control OUT failed (%d)", err);
			break;
		}

		buf = usbh_xfer_buf_alloc(udev, length);
		if (buf == NULL) {
			return -ENOMEM;
		}

		err = usbh_req_setup(udev,
				     bmRequestType | (USB_REQTYPE_DIR_TO_HOST << 7),
				     TESTUSB_VENDOR_REQ_IN, 0, 0, length,
				     buf);
		if (err != 0) {
			LOG_ERR("Control IN failed (%d)", err);
		} else if (buf->len != length) {
			LOG_ERR("Read back %u bytes, expected %u", buf->len, length);
			err = -EIO;
		} else {
			for (uint32_t j = 0; j < length; j++) {
				if (buf->data[j] != (uint8_t)(i + j)) {
					LOG_ERR("Data mismatch at byte %u", j);
					err = -EIO;
					break;
				}
			}
		}

		usbh_xfer_buf_free(udev, buf);
		if (err != 0) {
			break;
		}

		length = testusb_vary_len(length, param->vary, param->length);
	}

	LOG_DBG("Control: %u transfers, %s", param->count, err ? "failed" : "ok");

	return err;
}

/*
 * Read the endpoint halt feature. Returns 1 if halted, 0 if not, negative
 * value on error.
 */
static int testusb_ep_halted(struct usb_device *const udev, const uint8_t ep)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
				      USB_REQTYPE_RECIPIENT_ENDPOINT;
	const uint8_t bRequest = USB_SREQ_GET_STATUS;
	struct net_buf *buf;
	int err;

	buf = usbh_xfer_buf_alloc(udev, sizeof(uint16_t));
	if (buf == NULL) {
		return -ENOMEM;
	}

	err = usbh_req_setup(udev,
			     bmRequestType, bRequest, 0, ep, sizeof(uint16_t),
			     buf);
	if (err == 0) {
		uint16_t status = net_buf_pull_le16(buf);

		err = (status & BIT(USB_SFS_ENDPOINT_HALT)) ? 1 : 0;
	}

	usbh_xfer_buf_free(udev, buf);

	return err;
}

static int testusb_io_once(struct testusb_data *const data, const uint8_t ep)
{
	const struct usbh_testusb_param param = {
		.length = TESTUSB_HALT_CHECK_IO_LEN,
		.count = 1,
		.vary = 0,
	};

	return testusb_io(data, ep, 1, &param);
}

/*
 * Verify that a bulk endpoint is not halted by checking the state and
 * endpoint response.
 */
static int testusb_verify_not_halted(struct testusb_data *const data, const uint8_t ep)
{
	int err;

	err = testusb_ep_halted(data->udev, ep);
	if (err < 0) {
		return err;
	}

	if (err == 1) {
		LOG_ERR("ep 0x%02x status reports halted", ep);
		return -EIO;
	}

	err = testusb_io_once(data, ep);
	if (err != 0) {
		LOG_ERR("ep 0x%02x transfer failed (%d), expected success", ep, err);
		return err;
	}

	return 0;
}

/*
 * Verify that a bulk endpoint has been halted by checking the state and
 * endpoint response.
 */
static int testusb_verify_halted(struct testusb_data *const data, const uint8_t ep)
{
	int err;

	err = testusb_ep_halted(data->udev, ep);
	if (err < 0) {
		return err;
	}

	if (err == 0) {
		LOG_ERR("ep 0x%02x status does not report halted", ep);
		return -EIO;
	}

	err = testusb_io_once(data, ep);
	if (err != -EPIPE) {
		LOG_ERR("ep 0x%02x transfer returned %d, expected -EPIPE (STALL)",
			ep, err);
		return -EIO;
	}

	return 0;
}

/*
 * This test first verifies that a bulk endpoint is not halted, then sets
 * feature halt, verifies that endpoint responds with a stall handshake, clears
 * set feature, and finally verifies that endpoint handles transfers.
 */
static int testusb_halt(struct testusb_data *const data, const uint8_t ep)
{
	struct usb_device *const udev = data->udev;
	int err;

	err = testusb_verify_not_halted(data, ep);
	if (err != 0) {
		return err;
	}

	err = usbh_req_set_sfs_halt(udev, ep);
	if (err != 0) {
		LOG_ERR("Set halt on ep 0x%02x failed (%d)", ep, err);
		return err;
	}

	err = testusb_verify_halted(data, ep);
	if (err != 0) {
		return err;
	}

	err = usbh_req_clear_sfs_halt(udev, ep);
	if (err != 0) {
		LOG_ERR("Clear halt on ep 0x%02x failed (%d)", ep, err);
		return err;
	}

	return testusb_verify_not_halted(data, ep);
}

static int testusb_halt_simple(struct testusb_data *const data)
{
	int err;

	err = testusb_halt(data, data->in_ep);
	if (err != 0) {
		return err;
	}

	return testusb_halt(data, data->out_ep);
}

static int testusb_exec(const struct usbh_class_data *const c_data,
			const struct usbh_testusb_param *const param)
{
	struct testusb_data *const data = c_data->priv;

	switch (param->test) {
	case 0:
		/* NOP */
		return 0;
	case 1:
		return testusb_io(data, data->out_ep,
				  CONFIG_USBH_TESTUSB_QUEUE_DEPTH, param);
	case 2:
		return testusb_io(data, data->in_ep,
				  CONFIG_USBH_TESTUSB_QUEUE_DEPTH, param);
	case 3:
		if (param->vary == 0) {
			return -EINVAL;
		}

		return testusb_io(data, data->out_ep,
				  CONFIG_USBH_TESTUSB_QUEUE_DEPTH, param);
	case 4:
		if (param->vary == 0) {
			return -EINVAL;
		}

		return testusb_io(data, data->in_ep,
				  CONFIG_USBH_TESTUSB_QUEUE_DEPTH, param);
	case 13:
		return testusb_halt_simple(data);
	case 14:
		return testusb_control(data, param);
	case 25:
		if (data->int_out_ep == 0) {
			return -ENODEV;
		}

		return testusb_io(data, data->int_out_ep, 1, param);
	case 26:
		if (data->int_in_ep == 0) {
			return -ENODEV;
		}

		return testusb_io(data, data->int_in_ep, 1, param);
	default:
		return -ENOSYS;
	}
}

/* Find a bound instance by device address */
static struct usbh_class_data *testusb_find_class(const uint8_t addr, const bool any)
{
	STRUCT_SECTION_FOREACH(usbh_class_node, c_node) {
		struct usbh_class_data *c_data = c_node->c_data;

		if (c_node->c_data->api != &usbh_testusb_api) {
			continue;
		}

		if (c_data->udev == NULL) {
			continue;
		}

		if (any || c_data->udev->addr == addr) {
			return c_data;
		}
	}

	return NULL;
}

int usbh_testusb_exec(const uint8_t addr, const struct usbh_testusb_param *const param)
{
	struct usbh_class_data *c_data;

	c_data = testusb_find_class(addr, addr == 0);
	if (c_data == NULL) {
		return -ENODEV;
	}

	return testusb_exec(c_data, param);
}

static int testusb_parse_descriptors(struct testusb_data *const data,
				     struct usb_device *const udev)
{
	const struct usb_desc_header *desc = udev->cfg_desc;

	data->in_ep = 0;
	data->out_ep = 0;
	data->int_in_ep = 0;
	data->int_out_ep = 0;

	while (desc != NULL) {
		const struct usb_ep_descriptor *ep_desc;
		uint8_t ep_type;

		if (desc->bDescriptorType != USB_DESC_ENDPOINT) {
			desc = usbh_desc_get_next(desc);
			continue;
		}

		ep_desc = (const struct usb_ep_descriptor *)desc;
		ep_type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

		if (ep_type == USB_EP_TYPE_BULK) {
			if (USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
				data->in_ep = ep_desc->bEndpointAddress;
			} else {
				data->out_ep = ep_desc->bEndpointAddress;
			}
		}

		if (ep_type == USB_EP_TYPE_INTERRUPT) {
			if (USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
				data->int_in_ep = ep_desc->bEndpointAddress;
			} else {
				data->int_out_ep = ep_desc->bEndpointAddress;
			}
		}

		/* The isochronous endpoints are ignored */

		desc = usbh_desc_get_next(desc);
	}

	if (data->in_ep == 0 || data->out_ep == 0) {
		LOG_ERR("Bulk endpoints not found");
		return -ENODEV;
	}

	if (data->int_in_ep == 0 || data->int_out_ep == 0) {
		LOG_WRN("Interrupt endpoints not found");
	}

	LOG_DBG("Bulk in 0x%02x out 0x%02x, interrupt in 0x%02x out 0x%02x",
		data->in_ep, data->out_ep, data->int_in_ep, data->int_out_ep);

	return 0;
}

static int testusb_probe(struct usbh_class_data *const c_data,
			 struct usb_device *const udev, const uint8_t iface)
{
	struct testusb_data *const data = c_data->priv;

	LOG_DBG("Probe %s, bInterfaceNumber %u", c_data->name, iface);

	if (testusb_parse_descriptors(data, udev) != 0) {
		return -ENOTSUP;
	}

	data->udev = udev;

	return 0;
}

static int testusb_init(struct usbh_class_data *const c_data)
{
	struct testusb_data *const data = c_data->priv;

	LOG_DBG("Init function driver %s", c_data->name);

	data->c_data = c_data;

	return 0;
}

static int testusb_removed(struct usbh_class_data *const c_data)
{
	struct testusb_data *const data = c_data->priv;

	LOG_DBG("Removed %s", c_data->name);

	data->udev = NULL;
	data->in_ep = 0;
	data->out_ep = 0;
	data->int_in_ep = 0;
	data->int_out_ep = 0;

	return 0;
}

static struct usbh_class_api usbh_testusb_api = {
	.probe = testusb_probe,
	.init = testusb_init,
	.removed = testusb_removed,
};

#define USBH_TESTUSB_DEFINE(inst, _)						\
	static struct testusb_data testusb_data_##inst;				\
	USBH_DEFINE_CLASS(testusb_##inst, &usbh_testusb_api,			\
			  &testusb_data_##inst, testusb_filters)

LISTIFY(CONFIG_USBH_TESTUSB_INSTANCES_COUNT, USBH_TESTUSB_DEFINE, (;), _);
