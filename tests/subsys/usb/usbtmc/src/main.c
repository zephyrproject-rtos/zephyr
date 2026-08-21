/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/class/usb_tmc.h>
#include <zephyr/usb/class/usbd_usbtmc.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/ztest.h>

#include <usbh_ch9.h>
#include <usbh_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbtmc_test, LOG_LEVEL_INF);

/* The class instance under test is the only class instance registered and
 * the stack deterministically assigns the first bulk endpoint pair.
 */
#define TEST_BULK_OUT_EP	0x01
#define TEST_BULK_IN_EP		0x81
#define TEST_INTERFACE		0

#define TEST_REQTYPE_IFACE	((USB_REQTYPE_DIR_TO_HOST << 7) |	\
				 (USB_REQTYPE_TYPE_CLASS << 5) |	\
				 USB_REQTYPE_RECIPIENT_INTERFACE)
#define TEST_REQTYPE_EP		((USB_REQTYPE_DIR_TO_HOST << 7) |	\
				 (USB_REQTYPE_TYPE_CLASS << 5) |	\
				 USB_REQTYPE_RECIPIENT_ENDPOINT)

#define TEST_RX_BUF_SIZE	4096
#define TEST_XFER_TIMEOUT	K_MSEC(1000)

USBD_CONFIGURATION_DEFINE(test_fs_config, USB_SCD_SELF_POWERED, 200, NULL);
USBD_CONFIGURATION_DEFINE(test_hs_config, USB_SCD_SELF_POWERED, 200, NULL);

USBD_DESC_LANG_DEFINE(test_lang);
USBD_DESC_STRING_DEFINE(test_mfg, "ZEPHYR", 1);
USBD_DESC_STRING_DEFINE(test_product, "Zephyr USBTMC Test", 2);
USBD_DESC_STRING_DEFINE(test_sn, "0123456789ABCDEF", 3);

USBD_DEVICE_DEFINE(test_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0xffff);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static const struct device *const usbtmc_dev =
	DEVICE_DT_GET(DT_NODELABEL(usbtmc0));

/* Device side instrument state */
static uint8_t tmc_rx_buf[TEST_RX_BUF_SIZE];
static size_t tmc_rx_len;
static uint32_t tmc_begin_count;
static uint32_t tmc_eom_count;
static uint32_t tmc_clear_count;
static uint32_t tmc_pulse_count;
static bool tmc_echo;
static K_SEM_DEFINE(tmc_eom_sem, 0, 1);

/* Host side bTag counter */
static uint8_t host_btag;

static uint8_t next_btag(void)
{
	host_btag++;
	if (host_btag == 0) {
		host_btag = 1;
	}

	return host_btag;
}

static void tmc_msg_out(const struct device *dev, const uint8_t *const data,
			const size_t len, const bool begin, const bool eom)
{
	if (begin) {
		tmc_begin_count++;
		tmc_rx_len = 0;
	}

	if (tmc_rx_len + len <= sizeof(tmc_rx_buf)) {
		memcpy(&tmc_rx_buf[tmc_rx_len], data, len);
		tmc_rx_len += len;
	}

	if (eom) {
		tmc_eom_count++;
		if (tmc_echo) {
			usbd_usbtmc_msg_write(dev, tmc_rx_buf, tmc_rx_len, true);
		}

		k_sem_give(&tmc_eom_sem);
	}
}

static void tmc_clear(const struct device *dev)
{
	tmc_clear_count++;
}

static int tmc_indicator_pulse(const struct device *dev)
{
	tmc_pulse_count++;

	return 0;
}

static const struct usbd_usbtmc_ops tmc_ops = {
	.msg_out = tmc_msg_out,
	.clear = tmc_clear,
	.indicator_pulse = tmc_indicator_pulse,
};

/* Host side synchronous bulk transfer support */
static K_SEM_DEFINE(bulk_sem, 0, 1);
static int bulk_xfer_err;

static int bulk_xfer_cb(struct usb_device *const udev,
			struct uhc_transfer *const xfer)
{
	bulk_xfer_err = xfer->err;
	usbh_xfer_free(udev, xfer);
	k_sem_give(&bulk_sem);

	return 0;
}

static int bulk_xfer(struct usb_device *const udev, const uint8_t ep,
		     struct net_buf *const buf)
{
	struct uhc_transfer *xfer;
	int err;

	xfer = usbh_xfer_alloc(udev, ep, bulk_xfer_cb, NULL);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	xfer->buf = buf;
	k_sem_reset(&bulk_sem);
	err = usbh_xfer_enqueue(udev, xfer);
	if (err != 0) {
		usbh_xfer_free(udev, xfer);
		return err;
	}

	if (k_sem_take(&bulk_sem, TEST_XFER_TIMEOUT) != 0) {
		usbh_xfer_dequeue(udev, xfer);
		k_sem_take(&bulk_sem, TEST_XFER_TIMEOUT);
		return -ETIMEDOUT;
	}

	return bulk_xfer_err;
}

static struct usb_device *test_get_udev(void)
{
	struct usb_device *udev;

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");
	zassert_equal(udev->state, USB_STATE_CONFIGURED,
		      "USB device is not in configured state");

	return udev;
}

/* Send a DEV_DEP_MSG_OUT transfer, only the last chunk has the eom flag */
static int send_dev_dep_msg_out(struct usb_device *const udev,
				const uint8_t btag,
				const uint8_t *const msg, const uint32_t size,
				const bool eom)
{
	struct usbtmc_msg_header hdr = {
		.MsgID = USBTMC_MSGID_DEV_DEP_MSG_OUT,
		.bTag = btag,
		.bTagInverse = (uint8_t)~btag,
		.dev_dep_msg_out = {
			.TransferSize = sys_cpu_to_le32(size),
			.bmTransferAttributes = eom ? USBTMC_TRANSFER_ATTRIB_EOM : 0,
		},
	};
	struct net_buf *buf;
	int err;

	buf = usbh_xfer_buf_alloc(udev, sizeof(hdr) + ROUND_UP(size, 4));
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	net_buf_add_mem(buf, msg, size);
	/* Alignment bytes */
	net_buf_add(buf, ROUND_UP(size, 4) - size);

	err = bulk_xfer(udev, TEST_BULK_OUT_EP, buf);
	usbh_xfer_buf_free(udev, buf);

	return err;
}

static int send_request_dev_dep_msg_in(struct usb_device *const udev,
				       const uint8_t btag, const uint32_t size)
{
	struct usbtmc_msg_header hdr = {
		.MsgID = USBTMC_MSGID_REQUEST_DEV_DEP_MSG_IN,
		.bTag = btag,
		.bTagInverse = (uint8_t)~btag,
		.request_dev_dep_msg_in = {
			.TransferSize = sys_cpu_to_le32(size),
		},
	};
	struct net_buf *buf;
	int err;

	buf = usbh_xfer_buf_alloc(udev, sizeof(hdr));
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	err = bulk_xfer(udev, TEST_BULK_OUT_EP, buf);
	usbh_xfer_buf_free(udev, buf);

	return err;
}

/*
 * Read a DEV_DEP_MSG_IN transfer and validate the header. On success the
 * message data is appended to the msg buffer and eom is updated.
 */
static int read_dev_dep_msg_in(struct usb_device *const udev,
			       const uint8_t btag, const uint32_t req_size,
			       uint8_t *const msg, size_t *const msg_len,
			       bool *const eom)
{
	struct usbtmc_msg_header hdr;
	struct net_buf *buf;
	uint32_t size;
	int err;

	buf = usbh_xfer_buf_alloc(udev, sizeof(hdr) + req_size);
	if (buf == NULL) {
		return -ENOMEM;
	}

	err = bulk_xfer(udev, TEST_BULK_IN_EP, buf);
	if (err != 0) {
		usbh_xfer_buf_free(udev, buf);
		return err;
	}

	zassert_true(buf->len >= sizeof(hdr), "Bulk-IN header is incomplete");
	memcpy(&hdr, buf->data, sizeof(hdr));
	net_buf_pull(buf, sizeof(hdr));

	zassert_equal(hdr.MsgID, USBTMC_MSGID_DEV_DEP_MSG_IN, "Wrong MsgID");
	zassert_equal(hdr.bTag, btag, "Wrong bTag");
	zassert_equal(hdr.bTagInverse, (uint8_t)~btag, "Wrong bTagInverse");
	zassert_equal(hdr.reserved, 0, "Reserved byte is not zero");

	size = sys_le32_to_cpu(hdr.dev_dep_msg_in.TransferSize);
	zassert_true(size <= req_size, "More data than requested");
	zassert_equal(buf->len, size, "Wrong number of message data bytes");

	memcpy(&msg[*msg_len], buf->data, size);
	*msg_len += size;
	*eom = hdr.dev_dep_msg_in.bmTransferAttributes & USBTMC_TRANSFER_ATTRIB_EOM;

	usbh_xfer_buf_free(udev, buf);

	return 0;
}

/* Read a complete message, one REQUEST_DEV_DEP_MSG_IN transfer at a time */
static int read_message(struct usb_device *const udev, const uint32_t req_size,
			uint8_t *const msg, size_t *const msg_len)
{
	bool eom = false;
	int err;

	*msg_len = 0;

	while (!eom) {
		uint8_t btag = next_btag();

		err = send_request_dev_dep_msg_in(udev, btag, req_size);
		if (err != 0) {
			return err;
		}

		err = read_dev_dep_msg_in(udev, btag, req_size, msg, msg_len, &eom);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static int class_request_in(struct usb_device *const udev,
			    const uint8_t bmRequestType, const uint8_t bRequest,
			    const uint16_t wValue, const uint16_t wIndex,
			    void *const rsp, const uint16_t wLength)
{
	struct net_buf *buf;
	int err;

	buf = usbh_xfer_buf_alloc(udev, wLength);
	if (buf == NULL) {
		return -ENOMEM;
	}

	err = usbh_req_setup(udev, bmRequestType, bRequest, wValue, wIndex,
			     wLength, buf);
	if (err == 0) {
		zassert_equal(buf->len, wLength, "Unexpected response length");
		memcpy(rsp, buf->data, buf->len);
	}

	usbh_xfer_buf_free(udev, buf);

	return err;
}

static uint16_t get_ep_status(struct usb_device *const udev, const uint8_t ep)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
				      USB_REQTYPE_RECIPIENT_ENDPOINT;
	uint16_t status = 0;
	struct net_buf *buf;
	int err;

	buf = usbh_xfer_buf_alloc(udev, sizeof(status));
	zassert_not_null(buf, "Failed to allocate buffer");

	err = usbh_req_setup(udev, bmRequestType, USB_SREQ_GET_STATUS,
			     0, ep, sizeof(status), buf);
	zassert_equal(err, 0, "Get Status transfer failed");
	status = sys_get_le16(buf->data);
	usbh_xfer_buf_free(udev, buf);

	return status;
}

/* Verify that the Bulk-OUT endpoint is halted and recover from it */
static void verify_out_halted_and_recover(struct usb_device *const udev)
{
	const uint8_t echo_msg[] = "RECOVERY";
	uint8_t msg[16];
	size_t msg_len;
	int err;

	zassert_equal(get_ep_status(udev, TEST_BULK_OUT_EP), BIT(0),
		      "Bulk-OUT endpoint is not halted");

	err = usbh_req_clear_sfs_halt(udev, TEST_BULK_OUT_EP);
	zassert_equal(err, 0, "Failed to clear Bulk-OUT endpoint halt");

	/* The next transfer is interpreted as a new header */
	err = send_dev_dep_msg_out(udev, next_btag(), echo_msg,
				   sizeof(echo_msg), true);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");

	err = read_message(udev, 64, msg, &msg_len);
	zassert_equal(err, 0, "Failed to read message");
	zassert_equal(msg_len, sizeof(echo_msg), "Wrong message length");
	zassert_mem_equal(msg, echo_msg, sizeof(echo_msg), "Wrong message data");
}

ZTEST(usbtmc, test_get_capabilities)
{
	struct usbtmc_capabilities caps;
	struct usb_device *udev;
	uint8_t status;
	int err;

	udev = test_get_udev();

	err = class_request_in(udev, TEST_REQTYPE_IFACE,
			       USBTMC_REQ_GET_CAPABILITIES, 0, TEST_INTERFACE,
			       &caps, sizeof(caps));
	zassert_equal(err, 0, "GET_CAPABILITIES transfer failed");

	zassert_equal(caps.USBTMC_status, USBTMC_STATUS_SUCCESS, "Wrong status");
	zassert_equal(caps.reserved0, 0, "Reserved byte is not zero");
	zassert_equal(sys_le16_to_cpu(caps.bcdUSBTMC), USBTMC_BCD_1_0,
		      "Wrong bcdUSBTMC");
	zassert_equal(caps.bmInterfaceCapabilities,
		      USBTMC_INTF_CAP_INDICATOR_PULSE,
		      "Wrong interface capabilities");
	zassert_equal(caps.bmDeviceCapabilities, 0, "Wrong device capabilities");

	for (size_t i = 0; i < sizeof(caps.reserved1); i++) {
		zassert_equal(caps.reserved1[i], 0, "Reserved byte is not zero");
	}

	for (size_t i = 0; i < sizeof(caps.reserved2); i++) {
		zassert_equal(caps.reserved2[i], 0, "Reserved byte is not zero");
	}

	/* The response must be clamped to wLength */
	err = class_request_in(udev, TEST_REQTYPE_IFACE,
			       USBTMC_REQ_GET_CAPABILITIES, 0, TEST_INTERFACE,
			       &status, sizeof(status));
	zassert_equal(err, 0, "GET_CAPABILITIES transfer failed");
	zassert_equal(status, USBTMC_STATUS_SUCCESS, "Wrong status");
}

ZTEST(usbtmc, test_message_echo)
{
	const uint8_t echo_msg[] = "*IDN?";
	struct usb_device *udev;
	uint8_t msg[16];
	size_t msg_len;
	int err;

	udev = test_get_udev();

	k_sem_reset(&tmc_eom_sem);
	err = send_dev_dep_msg_out(udev, next_btag(), echo_msg,
				   sizeof(echo_msg), true);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");

	zassert_equal(k_sem_take(&tmc_eom_sem, TEST_XFER_TIMEOUT), 0,
		      "Timeout waiting for the message");
	zassert_equal(tmc_rx_len, sizeof(echo_msg), "Wrong message length");
	zassert_mem_equal(tmc_rx_buf, echo_msg, sizeof(echo_msg),
			  "Wrong message data");

	err = read_message(udev, 64, msg, &msg_len);
	zassert_equal(err, 0, "Failed to read message");
	zassert_equal(msg_len, sizeof(echo_msg), "Wrong message length");
	zassert_mem_equal(msg, echo_msg, sizeof(echo_msg), "Wrong message data");
}

ZTEST(usbtmc, test_message_padding)
{
	/* Not a multiple of four bytes, three alignment bytes are added */
	const uint8_t echo_msg[] = "12345";
	struct usb_device *udev;
	uint8_t msg[16];
	size_t msg_len;
	int err;

	udev = test_get_udev();

	k_sem_reset(&tmc_eom_sem);
	err = send_dev_dep_msg_out(udev, next_btag(), echo_msg, 5, true);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");

	zassert_equal(k_sem_take(&tmc_eom_sem, TEST_XFER_TIMEOUT), 0,
		      "Timeout waiting for the message");
	zassert_equal(tmc_rx_len, 5, "Alignment bytes have not been dropped");

	err = read_message(udev, 64, msg, &msg_len);
	zassert_equal(err, 0, "Failed to read message");
	zassert_equal(msg_len, 5, "Wrong message length");
	zassert_mem_equal(msg, echo_msg, 5, "Wrong message data");
}

ZTEST(usbtmc, test_large_message_echo)
{
	static uint8_t echo_msg[2500];
	static uint8_t msg[TEST_RX_BUF_SIZE];
	struct usb_device *udev;
	size_t msg_len;
	int err;

	for (size_t i = 0; i < sizeof(echo_msg); i++) {
		echo_msg[i] = (uint8_t)i;
	}

	udev = test_get_udev();

	k_sem_reset(&tmc_eom_sem);
	err = send_dev_dep_msg_out(udev, next_btag(), echo_msg,
				   sizeof(echo_msg), true);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");

	zassert_equal(k_sem_take(&tmc_eom_sem, TEST_XFER_TIMEOUT), 0,
		      "Timeout waiting for the message");
	zassert_equal(tmc_rx_len, sizeof(echo_msg), "Wrong message length");
	zassert_mem_equal(tmc_rx_buf, echo_msg, sizeof(echo_msg),
			  "Wrong message data");

	err = read_message(udev, sizeof(msg), msg, &msg_len);
	zassert_equal(err, 0, "Failed to read message");
	zassert_equal(msg_len, sizeof(echo_msg), "Wrong message length");
	zassert_mem_equal(msg, echo_msg, sizeof(echo_msg), "Wrong message data");
}

ZTEST(usbtmc, test_multi_transfer_message)
{
	const uint8_t part1[] = "MEAS:";
	const uint8_t part2[] = "TEMP?";
	const uint32_t begin_count = tmc_begin_count;
	struct usb_device *udev;
	uint8_t msg[16];
	size_t msg_len;
	int err;

	udev = test_get_udev();

	k_sem_reset(&tmc_eom_sem);
	err = send_dev_dep_msg_out(udev, next_btag(), part1, 5, false);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");

	err = send_dev_dep_msg_out(udev, next_btag(), part2, 5, true);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");

	zassert_equal(k_sem_take(&tmc_eom_sem, TEST_XFER_TIMEOUT), 0,
		      "Timeout waiting for the message");
	zassert_equal(tmc_begin_count, begin_count + 1,
		      "Message did not span both transfers");
	zassert_equal(tmc_rx_len, 10, "Wrong message length");
	zassert_mem_equal(tmc_rx_buf, "MEAS:TEMP?", 10, "Wrong message data");

	err = read_message(udev, 64, msg, &msg_len);
	zassert_equal(err, 0, "Failed to read message");
	zassert_equal(msg_len, 10, "Wrong message length");
	zassert_mem_equal(msg, "MEAS:TEMP?", 10, "Wrong message data");
}

ZTEST(usbtmc, test_unsupported_msgid)
{
	const uint8_t btag = next_btag();
	struct usbtmc_msg_header hdr = {
		.MsgID = USBTMC_MSGID_VENDOR_SPECIFIC_OUT,
		.bTag = btag,
		.bTagInverse = (uint8_t)~btag,
		.dev_dep_msg_out = {
			.TransferSize = sys_cpu_to_le32(4),
		},
	};
	struct usb_device *udev;
	struct net_buf *buf;
	int err;

	udev = test_get_udev();

	buf = usbh_xfer_buf_alloc(udev, sizeof(hdr) + 4);
	zassert_not_null(buf, "Failed to allocate buffer");
	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	net_buf_add_le32(buf, 0);

	err = bulk_xfer(udev, TEST_BULK_OUT_EP, buf);
	usbh_xfer_buf_free(udev, buf);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");

	k_msleep(50);
	verify_out_halted_and_recover(udev);
}

ZTEST(usbtmc, test_invalid_btag_inverse)
{
	const uint8_t btag = next_btag();
	struct usbtmc_msg_header hdr = {
		.MsgID = USBTMC_MSGID_DEV_DEP_MSG_OUT,
		.bTag = btag,
		.bTagInverse = btag,
		.dev_dep_msg_out = {
			.TransferSize = sys_cpu_to_le32(4),
			.bmTransferAttributes = USBTMC_TRANSFER_ATTRIB_EOM,
		},
	};
	struct usb_device *udev;
	struct net_buf *buf;
	int err;

	udev = test_get_udev();

	buf = usbh_xfer_buf_alloc(udev, sizeof(hdr) + 4);
	zassert_not_null(buf, "Failed to allocate buffer");
	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	net_buf_add_le32(buf, 0);

	err = bulk_xfer(udev, TEST_BULK_OUT_EP, buf);
	usbh_xfer_buf_free(udev, buf);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");

	k_msleep(50);
	verify_out_halted_and_recover(udev);
}

ZTEST(usbtmc, test_invalid_transfer_size)
{
	const uint8_t btag = next_btag();
	struct usbtmc_msg_header hdr = {
		.MsgID = USBTMC_MSGID_DEV_DEP_MSG_OUT,
		.bTag = btag,
		.bTagInverse = (uint8_t)~btag,
		.dev_dep_msg_out = {
			.TransferSize = sys_cpu_to_le32(0),
			.bmTransferAttributes = USBTMC_TRANSFER_ATTRIB_EOM,
		},
	};
	struct usb_device *udev;
	struct net_buf *buf;
	int err;

	udev = test_get_udev();

	buf = usbh_xfer_buf_alloc(udev, sizeof(hdr));
	zassert_not_null(buf, "Failed to allocate buffer");
	net_buf_add_mem(buf, &hdr, sizeof(hdr));

	err = bulk_xfer(udev, TEST_BULK_OUT_EP, buf);
	usbh_xfer_buf_free(udev, buf);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");

	k_msleep(50);
	verify_out_halted_and_recover(udev);
}

ZTEST(usbtmc, test_abort_bulk_in)
{
	struct usbtmc_check_abort_bulk_in_response check_rsp;
	struct usbtmc_initiate_abort_response rsp;
	struct usb_device *udev;
	struct net_buf *buf;
	uint8_t btag;
	int err;

	udev = test_get_udev();

	/* Abort without a transfer in progress must fail */
	err = class_request_in(udev, TEST_REQTYPE_EP,
			       USBTMC_REQ_INITIATE_ABORT_BULK_IN,
			       1, TEST_BULK_IN_EP, &rsp, sizeof(rsp));
	zassert_equal(err, 0, "INITIATE_ABORT_BULK_IN transfer failed");
	zassert_equal(rsp.USBTMC_status, USBTMC_STATUS_FAILED, "Wrong status");

	/* Request a response that the instrument does not provide */
	btag = next_btag();
	err = send_request_dev_dep_msg_in(udev, btag, 64);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");
	k_msleep(50);

	/* Abort with a wrong bTag */
	err = class_request_in(udev, TEST_REQTYPE_EP,
			       USBTMC_REQ_INITIATE_ABORT_BULK_IN,
			       (uint8_t)(btag + 1), TEST_BULK_IN_EP,
			       &rsp, sizeof(rsp));
	zassert_equal(err, 0, "INITIATE_ABORT_BULK_IN transfer failed");
	zassert_equal(rsp.USBTMC_status, USBTMC_STATUS_TRANSFER_NOT_IN_PROGRESS,
		      "Wrong status");
	zassert_equal(rsp.bTag, btag, "Wrong bTag");

	err = class_request_in(udev, TEST_REQTYPE_EP,
			       USBTMC_REQ_INITIATE_ABORT_BULK_IN,
			       btag, TEST_BULK_IN_EP, &rsp, sizeof(rsp));
	zassert_equal(err, 0, "INITIATE_ABORT_BULK_IN transfer failed");
	zassert_equal(rsp.USBTMC_status, USBTMC_STATUS_SUCCESS, "Wrong status");
	zassert_equal(rsp.bTag, btag, "Wrong bTag");

	/* The device terminates the aborted transfer with a short packet */
	buf = usbh_xfer_buf_alloc(udev, 64);
	zassert_not_null(buf, "Failed to allocate buffer");
	err = bulk_xfer(udev, TEST_BULK_IN_EP, buf);
	zassert_equal(err, 0, "Bulk-IN transfer failed");
	zassert_equal(buf->len, 0, "Aborted transfer is not a short packet");
	usbh_xfer_buf_free(udev, buf);

	err = class_request_in(udev, TEST_REQTYPE_EP,
			       USBTMC_REQ_CHECK_ABORT_BULK_IN_STATUS,
			       0, TEST_BULK_IN_EP, &check_rsp, sizeof(check_rsp));
	zassert_equal(err, 0, "CHECK_ABORT_BULK_IN_STATUS transfer failed");
	zassert_equal(check_rsp.USBTMC_status, USBTMC_STATUS_SUCCESS,
		      "Wrong status");
	zassert_equal(check_rsp.bmAbortBulkIn, 0, "Wrong bmAbortBulkIn");
	zassert_equal(sys_le32_to_cpu(check_rsp.NBYTES_TXD), 0, "Wrong NBYTES_TXD");
}

ZTEST(usbtmc, test_abort_bulk_out)
{
	struct usbtmc_check_abort_bulk_out_response check_rsp;
	struct usbtmc_initiate_abort_response rsp;
	const uint8_t btag = next_btag();
	struct usbtmc_msg_header hdr = {
		.MsgID = USBTMC_MSGID_DEV_DEP_MSG_OUT,
		.bTag = btag,
		.bTagInverse = (uint8_t)~btag,
		.dev_dep_msg_out = {
			/* Message data does not follow, the transfer is
			 * aborted before it completes
			 */
			.TransferSize = sys_cpu_to_le32(2000),
			.bmTransferAttributes = USBTMC_TRANSFER_ATTRIB_EOM,
		},
	};
	struct usb_device *udev;
	struct net_buf *buf;
	size_t data_size;
	int err;

	udev = test_get_udev();

	/* Abort without a transfer in progress must fail */
	err = class_request_in(udev, TEST_REQTYPE_EP,
			       USBTMC_REQ_INITIATE_ABORT_BULK_OUT,
			       btag, TEST_BULK_OUT_EP, &rsp, sizeof(rsp));
	zassert_equal(err, 0, "INITIATE_ABORT_BULK_OUT transfer failed");
	zassert_equal(rsp.USBTMC_status, USBTMC_STATUS_FAILED, "Wrong status");

	/* Start a transfer with only the first full packet, the message data
	 * fills the packet up to the maximum packet size
	 */
	data_size = usbd_bus_speed(&test_usbd) == USBD_SPEED_HS ?
		    512 - sizeof(hdr) : 64 - sizeof(hdr);
	buf = usbh_xfer_buf_alloc(udev, sizeof(hdr) + data_size);
	zassert_not_null(buf, "Failed to allocate buffer");
	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	memset(net_buf_add(buf, data_size), 0x5a, data_size);

	err = bulk_xfer(udev, TEST_BULK_OUT_EP, buf);
	usbh_xfer_buf_free(udev, buf);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");
	k_msleep(50);

	err = class_request_in(udev, TEST_REQTYPE_EP,
			       USBTMC_REQ_INITIATE_ABORT_BULK_OUT,
			       btag, TEST_BULK_OUT_EP, &rsp, sizeof(rsp));
	zassert_equal(err, 0, "INITIATE_ABORT_BULK_OUT transfer failed");
	zassert_equal(rsp.USBTMC_status, USBTMC_STATUS_SUCCESS, "Wrong status");
	zassert_equal(rsp.bTag, btag, "Wrong bTag");

	/* Poll status until the abort is complete */
	for (int i = 0; i < 100; i++) {
		err = class_request_in(udev, TEST_REQTYPE_EP,
				       USBTMC_REQ_CHECK_ABORT_BULK_OUT_STATUS,
				       0, TEST_BULK_OUT_EP,
				       &check_rsp, sizeof(check_rsp));
		zassert_equal(err, 0,
			      "CHECK_ABORT_BULK_OUT_STATUS transfer failed");
		if (check_rsp.USBTMC_status != USBTMC_STATUS_PENDING) {
			break;
		}

		k_msleep(10);
	}

	zassert_equal(check_rsp.USBTMC_status, USBTMC_STATUS_SUCCESS,
		      "Wrong status");
	zassert_equal(sys_le32_to_cpu(check_rsp.NBYTES_RXD), data_size,
		      "Wrong NBYTES_RXD");

	verify_out_halted_and_recover(udev);
}

ZTEST(usbtmc, test_initiate_clear)
{
	struct usbtmc_check_clear_response check_rsp;
	const uint8_t staged_msg[] = "STALE";
	const uint32_t clear_count = tmc_clear_count;
	struct usb_device *udev;
	uint8_t status;
	int err;

	udev = test_get_udev();

	/* Stage message data the host never reads */
	err = usbd_usbtmc_msg_write(usbtmc_dev, staged_msg, sizeof(staged_msg),
				    true);
	zassert_equal(err, sizeof(staged_msg), "Failed to write message");

	err = class_request_in(udev, TEST_REQTYPE_IFACE,
			       USBTMC_REQ_INITIATE_CLEAR, 0, TEST_INTERFACE,
			       &status, sizeof(status));
	zassert_equal(err, 0, "INITIATE_CLEAR transfer failed");
	zassert_equal(status, USBTMC_STATUS_SUCCESS, "Wrong status");

	/* Poll status until the clear is complete */
	for (int i = 0; i < 100; i++) {
		err = class_request_in(udev, TEST_REQTYPE_IFACE,
				       USBTMC_REQ_CHECK_CLEAR_STATUS,
				       0, TEST_INTERFACE,
				       &check_rsp, sizeof(check_rsp));
		zassert_equal(err, 0, "CHECK_CLEAR_STATUS transfer failed");
		if (check_rsp.USBTMC_status != USBTMC_STATUS_PENDING) {
			break;
		}

		k_msleep(10);
	}

	zassert_equal(check_rsp.USBTMC_status, USBTMC_STATUS_SUCCESS,
		      "Wrong status");
	zassert_equal(check_rsp.bmClear, 0, "Wrong bmClear");
	zassert_equal(tmc_clear_count, clear_count + 1,
		      "Clear callback has not been called");

	/* The sequence ends with the host clearing the Bulk-OUT halt, the
	 * staged message data has been discarded and normal operation
	 * resumes.
	 */
	verify_out_halted_and_recover(udev);
}

ZTEST(usbtmc, test_request_msg_in_while_active)
{
	const uint8_t staged_msg[] = "42";
	struct usb_device *udev;
	uint8_t msg[16];
	size_t msg_len;
	uint8_t btag;
	bool eom;
	int err;

	udev = test_get_udev();

	btag = next_btag();
	err = send_request_dev_dep_msg_in(udev, btag, 64);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");
	k_msleep(50);

	/* A new command message expecting a response while a Bulk-IN transfer
	 * is in progress halts the Bulk-IN endpoint
	 */
	err = send_request_dev_dep_msg_in(udev, next_btag(), 64);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");
	k_msleep(50);

	zassert_equal(get_ep_status(udev, TEST_BULK_IN_EP), BIT(0),
		      "Bulk-IN endpoint is not halted");

	err = usbh_req_clear_sfs_halt(udev, TEST_BULK_IN_EP);
	zassert_equal(err, 0, "Failed to clear Bulk-IN endpoint halt");

	/* The device queues nothing until a new command message expecting a
	 * response is received
	 */
	err = usbd_usbtmc_msg_write(usbtmc_dev, staged_msg, sizeof(staged_msg),
				    true);
	zassert_equal(err, sizeof(staged_msg), "Failed to write message");

	btag = next_btag();
	err = send_request_dev_dep_msg_in(udev, btag, 64);
	zassert_equal(err, 0, "Bulk-OUT transfer failed");

	msg_len = 0;
	err = read_dev_dep_msg_in(udev, btag, 64, msg, &msg_len, &eom);
	zassert_equal(err, 0, "Failed to read message");
	zassert_true(eom, "EOM is not set");
	zassert_equal(msg_len, sizeof(staged_msg), "Wrong message length");
	zassert_mem_equal(msg, staged_msg, sizeof(staged_msg),
			  "Wrong message data");
}

ZTEST(usbtmc, test_indicator_pulse)
{
	const uint32_t pulse_count = tmc_pulse_count;
	struct usb_device *udev;
	uint8_t status;
	int err;

	udev = test_get_udev();

	err = class_request_in(udev, TEST_REQTYPE_IFACE,
			       USBTMC_REQ_INDICATOR_PULSE, 0, TEST_INTERFACE,
			       &status, sizeof(status));
	zassert_equal(err, 0, "INDICATOR_PULSE transfer failed");
	zassert_equal(status, USBTMC_STATUS_SUCCESS, "Wrong status");
	zassert_equal(tmc_pulse_count, pulse_count + 1,
		      "Indicator pulse callback has not been called");
}

ZTEST(usbtmc, test_unsupported_class_request)
{
	struct usb_device *udev;
	uint8_t status;
	int err;

	udev = test_get_udev();

	err = class_request_in(udev, TEST_REQTYPE_IFACE, 42, 0, TEST_INTERFACE,
			       &status, sizeof(status));
	zassert_equal(err, -EPIPE, "Unsupported request is not stalled");

	/* CHECK_ABORT_BULK_IN_STATUS without an INITIATE_ABORT_BULK_IN */
	err = class_request_in(udev, TEST_REQTYPE_EP,
			       USBTMC_REQ_CHECK_ABORT_BULK_IN_STATUS,
			       0, TEST_BULK_IN_EP, &status, sizeof(status));
	zassert_equal(err, 0, "CHECK_ABORT_BULK_IN_STATUS transfer failed");
	zassert_equal(status, USBTMC_STATUS_SPLIT_NOT_IN_PROGRESS,
		      "Wrong status");
}

ZTEST(usbtmc, test_msg_write)
{
	int err;

	/* Zero length writes are a no-op */
	err = usbd_usbtmc_msg_write(usbtmc_dev, NULL, 0, true);
	zassert_equal(err, 0, "Wrong number of bytes written");
}

static void *usbtmc_test_setup(void)
{
	struct usb_device *udev;
	int err;

	err = usbd_usbtmc_register(usbtmc_dev, &tmc_ops);
	zassert_equal(err, 0, "Failed to register USBTMC event handlers");

	err = usbh_init(&uhs_ctx);
	zassert_equal(err, 0, "Failed to initialize USB host");

	err = usbh_enable(&uhs_ctx);
	zassert_equal(err, 0, "Failed to enable USB host");

	err = uhc_bus_reset(uhs_ctx.dev);
	zassert_equal(err, 0, "Failed to signal bus reset");

	err = uhc_bus_resume(uhs_ctx.dev);
	zassert_equal(err, 0, "Failed to signal bus resume");

	err = uhc_sof_enable(uhs_ctx.dev);
	zassert_equal(err, 0, "Failed to enable SoF generator");

	err = usbd_add_descriptor(&test_usbd, &test_lang);
	zassert_equal(err, 0, "Failed to add descriptor (%d)", err);

	err = usbd_add_descriptor(&test_usbd, &test_mfg);
	zassert_equal(err, 0, "Failed to add descriptor (%d)", err);

	err = usbd_add_descriptor(&test_usbd, &test_product);
	zassert_equal(err, 0, "Failed to add descriptor (%d)", err);

	err = usbd_add_descriptor(&test_usbd, &test_sn);
	zassert_equal(err, 0, "Failed to add descriptor (%d)", err);

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&test_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&test_usbd, USBD_SPEED_HS,
					     &test_hs_config);
		zassert_equal(err, 0, "Failed to add configuration (%d)", err);

		err = usbd_register_class(&test_usbd, "usbtmc_0",
					  USBD_SPEED_HS, 1);
		zassert_equal(err, 0, "Failed to register usbtmc_0 class (%d)",
			      err);
	}

	err = usbd_add_configuration(&test_usbd, USBD_SPEED_FS, &test_fs_config);
	zassert_equal(err, 0, "Failed to add configuration (%d)", err);

	err = usbd_register_class(&test_usbd, "usbtmc_0", USBD_SPEED_FS, 1);
	zassert_equal(err, 0, "Failed to register usbtmc_0 class (%d)", err);

	err = usbd_init(&test_usbd);
	zassert_equal(err, 0, "Failed to initialize device support");

	err = usbd_enable(&test_usbd);
	zassert_equal(err, 0, "Failed to enable device support");

	/* Allow the host time to enumerate the device */
	k_msleep(200);

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");
	if (udev->state != USB_STATE_CONFIGURED) {
		err = usbh_req_set_cfg(udev, 1);
		zassert_equal(err, 0, "Failed to set configuration");
	}

	tmc_echo = true;

	return NULL;
}

ZTEST_SUITE(usbtmc, NULL, usbtmc_test_setup, NULL, NULL, NULL);
