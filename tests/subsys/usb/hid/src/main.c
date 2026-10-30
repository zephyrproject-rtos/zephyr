/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sample_usbd.h>

#include <zephyr/ztest.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbd_hid.h>

#include <usbh_desc.h>
#include <usbh_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_test, LOG_LEVEL_INF);

#define TEST_REPORT_SIZE	DT_PROP(DT_NODELABEL(hid_dev_0), out_report_size)

static const uint8_t test_report_desc[] = {
	HID_USAGE_PAGE16(0xFF00),
	HID_USAGE(0x01),
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
		HID_USAGE(0x02),
		HID_LOGICAL_MIN8(0),
		HID_LOGICAL_MAX16(0xFF, 0x00),
		HID_REPORT_SIZE(8),
		HID_REPORT_COUNT(TEST_REPORT_SIZE),
		HID_INPUT(0x02),
		HID_USAGE(0x03),
		HID_OUTPUT(0x02),
	HID_END_COLLECTION,
};

struct test_report {
	uint16_t len;
	uint8_t data[2 * TEST_REPORT_SIZE];
};

K_MSGQ_DEFINE(test_report_msgq, sizeof(struct test_report), 4, 4);
static K_SEM_DEFINE(test_xfer_sem, 0, 1);
static int test_xfer_err;
static uint8_t test_out_ep;

static struct usbd_context *test_usbd;

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static int test_get_report(const struct device *dev,
			   const uint8_t type, const uint8_t id,
			   const uint16_t len, uint8_t *const buf)
{
	return -ENOTSUP;
}

static int test_set_report(const struct device *dev,
			   const uint8_t type, const uint8_t id,
			   const uint16_t len, const uint8_t *const buf)
{
	return -ENOTSUP;
}

static void test_output_report(const struct device *dev,
			       const uint16_t len, const uint8_t *const buf)
{
	struct test_report report = {
		.len = len,
	};

	memcpy(report.data, buf, MIN(len, sizeof(report.data)));
	(void)k_msgq_put(&test_report_msgq, &report, K_NO_WAIT);
}

static const struct hid_device_ops test_hid_ops = {
	.get_report = test_get_report,
	.set_report = test_set_report,
	.output_report = test_output_report,
};

static int test_xfer_cb(struct usb_device *const udev,
			struct uhc_transfer *const xfer)
{
	test_xfer_err = xfer->err;

	if (xfer->buf != NULL) {
		usbh_xfer_buf_free(udev, xfer->buf);
	}

	(void)uhc_xfer_unref(xfer);
	k_sem_give(&test_xfer_sem);

	return 0;
}

static uint8_t test_get_out_ep(const struct usb_device *const udev)
{
	const struct usb_cfg_descriptor *const c_desc = udev->cfg_desc;

	if (c_desc == NULL) {
		return 0;
	}

	for (unsigned int i = 0; i < c_desc->bNumInterfaces; i++) {
		const struct usb_if_descriptor *const if_d = (void *)udev->ifaces[i].dhp;
		const struct usb_desc_header *desc;

		if (if_d == NULL || if_d->bInterfaceClass != USB_BCC_HID) {
			continue;
		}

		desc = usbh_desc_get_next(if_d);
		while (desc != NULL && desc->bDescriptorType != USB_DESC_INTERFACE) {
			const struct usb_ep_descriptor *const ep_d = (void *)desc;

			if (desc->bDescriptorType == USB_DESC_ENDPOINT &&
			    USB_EP_DIR_IS_OUT(ep_d->bEndpointAddress)) {
				return ep_d->bEndpointAddress;
			}

			desc = usbh_desc_get_next(desc);
		}
	}

	return 0;
}

static void test_send_report(const uint8_t *const data)
{
	struct usb_device *udev;
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int err;

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");

	xfer = usbh_xfer_alloc(udev, test_out_ep, test_xfer_cb, NULL);
	zassert_not_null(xfer, "Failed to allocate transfer");

	buf = usbh_xfer_buf_alloc(udev, TEST_REPORT_SIZE);
	zassert_not_null(buf, "Failed to allocate buffer");

	net_buf_add_mem(buf, data, TEST_REPORT_SIZE);
	xfer->buf = buf;

	err = usbh_xfer_enqueue(udev, xfer);
	zassert_equal(err, 0, "Failed to enqueue transfer");

	err = k_sem_take(&test_xfer_sem, K_MSEC(500));
	zassert_equal(err, 0, "Transfer timeout");
	zassert_equal(test_xfer_err, 0, "Transfer status is an error");
}

static void test_check_report(const uint8_t *const data)
{
	struct test_report report;
	int err;

	err = k_msgq_get(&test_report_msgq, &report, K_MSEC(500));
	zassert_equal(err, 0, "No output report received");
	zassert_equal(report.len, TEST_REPORT_SIZE, "Output report length mismatch");
	zassert_mem_equal(report.data, data, TEST_REPORT_SIZE,
			  "Output report data mismatch");
}

/* An output report is received in one transfer of the report size */
ZTEST(usb_hid, test_output_report)
{
	const uint8_t data[TEST_REPORT_SIZE] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15};

	test_send_report(data);
	test_check_report(data);
}

/* Consecutive output reports are received separately and in order */
ZTEST(usb_hid, test_output_reports)
{
	const uint8_t data1[TEST_REPORT_SIZE] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25};
	const uint8_t data2[TEST_REPORT_SIZE] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35};

	test_send_report(data1);
	test_send_report(data2);
	test_check_report(data1);
	test_check_report(data2);
}

static void *usb_test_enable(void)
{
	struct usb_device *udev;
	int err;

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

	err = hid_device_register(DEVICE_DT_GET(DT_NODELABEL(hid_dev_0)),
				  test_report_desc, sizeof(test_report_desc),
				  &test_hid_ops);
	zassert_equal(err, 0, "Failed to register HID device (%d)", err);

	test_usbd = sample_usbd_init_device(NULL);
	zassert_not_null(test_usbd, "Failed to initialize USB device");

	err = usbd_enable(test_usbd);
	zassert_equal(err, 0, "Failed to enable device support");

	/* Allow the host time to reset the device. */
	k_msleep(200);

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");

	test_out_ep = test_get_out_ep(udev);
	zassert_not_equal(test_out_ep, 0, "No HID interrupt OUT endpoint");

	return NULL;
}

static void usb_test_before(void *f)
{
	k_msgq_purge(&test_report_msgq);
}

static void usb_test_shutdown(void *f)
{
	int err;

	err = usbd_disable(test_usbd);
	zassert_equal(err, 0, "Failed to disable device support");

	err = usbd_shutdown(test_usbd);
	zassert_equal(err, 0, "Failed to shutdown device support");

	err = usbh_disable(&uhs_ctx);
	zassert_equal(err, 0, "Failed to disable USB host");
}

ZTEST_SUITE(usb_hid, NULL, usb_test_enable, usb_test_before, NULL, usb_test_shutdown);
