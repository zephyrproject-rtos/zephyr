/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>
#include <sample_usbd.h>

#include <usbh_ch9.h>
#include <usbh_class.h>
#include <usbh_class_api.h>
#include <usbh_desc.h>
#include <usbh_device.h>
#include <usbh_host.h>

#include <usbh_test_common.h>

LOG_MODULE_REGISTER(usbh_test, LOG_LEVEL_INF);

/* The root hub has address 1, the first device connected to it gets 2 */
#define TEST_DEV_ADDR 2U

static const struct usbh_class_filter filter_rules_vid_pid[] = {
	{
		.vid = FOO_TEST_VID,
		.pid = FOO_TEST_PID,
		.flags = USBH_CLASS_MATCH_VID_PID,
	},
	{0},
};

static const struct usbh_class_filter filter_rules_triple[] = {
	{
		.class = FOO_TEST_CLASS,
		.sub = FOO_TEST_SUB,
		.proto = FOO_TEST_PROTO,
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
	},
	{0},
};

static const struct usbh_class_filter filter_rules_either[] = {
	{
		.class = FOO_TEST_CLASS,
		.sub = FOO_TEST_SUB,
		.proto = FOO_TEST_PROTO,
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
	},
	{
		.vid = FOO_TEST_VID,
		.pid = FOO_TEST_PID,
		.flags = USBH_CLASS_MATCH_VID_PID,
	},
	{0},
};

static const struct usbh_class_filter filter_rules_empty[] = {
	{0},
};

static const struct usbh_class_filter filter_invalid_triple = {
	.vid = FOO_TEST_VID,
	.pid = FOO_TEST_PID,
};

static const struct usbh_class_filter filter_invalid_vid_triple = {
	.vid = FOO_TEST_VID + 1,
	.pid = FOO_TEST_PID,
};

static const struct usbh_class_filter filter_invalid_pid_triple = {
	.vid = FOO_TEST_VID,
	.pid = FOO_TEST_PID + 1,
};

static const struct usbh_class_filter filter_valid = {
	.vid = FOO_TEST_VID,
	.pid = FOO_TEST_PID,
	.class = FOO_TEST_CLASS,
	.sub = FOO_TEST_SUB,
	.proto = FOO_TEST_PROTO,
};

static const struct usbh_class_filter filter_invalid_vid = {
	.vid = FOO_TEST_VID + 1,
	.pid = FOO_TEST_PID,
	.class = FOO_TEST_CLASS,
	.sub = FOO_TEST_SUB,
	.proto = FOO_TEST_PROTO,
};

static const struct usbh_class_filter filter_invalid_pid = {
	.vid = FOO_TEST_VID,
	.pid = FOO_TEST_PID + 1,
	.class = FOO_TEST_CLASS,
	.sub = FOO_TEST_SUB,
	.proto = FOO_TEST_PROTO,
};

ZTEST(usbh_test, test_class_matching)
{
	/* Invalid code triple */

	zassert(usbh_class_is_matching(NULL, &filter_invalid_triple),
		"Filtering on NULL rules should match");

	zassert(!usbh_class_is_matching(filter_rules_empty, &filter_invalid_triple),
		"Filtering on empty rules should not match");

	zassert(!usbh_class_is_matching(filter_rules_vid_pid, &filter_invalid_vid_triple),
		"Filtering on invalid VID + invalid code triple should not match");

	zassert(!usbh_class_is_matching(filter_rules_vid_pid, &filter_invalid_pid_triple),
		"Filtering on invalid PID + invalid code triple should not match");

	zassert(usbh_class_is_matching(filter_rules_vid_pid, &filter_invalid_triple),
		"Filtering on valid VID:PID + invalid code triple (ignored) should match");

	zassert(!usbh_class_is_matching(filter_rules_triple, &filter_invalid_triple),
		"Filtering on valid VID:PID (ignored) + invalid code triple should not match");

	zassert(usbh_class_is_matching(filter_rules_either, &filter_invalid_triple),
		"Filtering on valid VID:PID + invalid code triple should match");

	zassert(!usbh_class_is_matching(filter_rules_either, &filter_invalid_pid_triple),
		"Filtering on invalid VID:PID + invalid code triple should not match");

	zassert(!usbh_class_is_matching(filter_rules_either, &filter_invalid_pid_triple),
		"Filtering on invalid VID:PID + invalid code triple should not match");

	/* Valid code triple */

	zassert(usbh_class_is_matching(filter_rules_vid_pid, &filter_valid),
		"Filtering on valid VID:PID + valid code triple (ignored) should match");

	zassert(!usbh_class_is_matching(filter_rules_vid_pid, &filter_invalid_vid),
		"Filtering on invalid VID + valid code triple (ignored) should not match");

	zassert(!usbh_class_is_matching(filter_rules_vid_pid, &filter_invalid_pid),
		"Filtering on invalid PID + valid code triple (ignored) should not match");

	zassert(usbh_class_is_matching(filter_rules_triple, &filter_invalid_pid),
		"Filtering on invalid PID (ignored) + valid code triple should match");

	zassert(usbh_class_is_matching(filter_rules_triple, &filter_invalid_vid),
		"Filtering on invalid VID (ignored) + valid code triple should match");

	zassert(usbh_class_is_matching(filter_rules_either, &filter_invalid_pid),
		"Filtering on invalid PID + valid code triple should match");

	zassert(usbh_class_is_matching(filter_rules_either, &filter_valid),
		"Filtering on valid VID:PID + valid code triple should match");
}

ZTEST(usbh_test, test_get_next_desc)
{
	const struct usb_desc_header *desc;
	struct usb_device *udev;

	udev = usbh_device_get(uhs_ctx, TEST_DEV_ADDR);
	zassert_not_null(udev);

	desc = udev->cfg_desc;
	zassert_not_null(desc);

	/* #0 cfg */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_CONFIGURATION);
	desc = usbh_desc_get_next(desc);

	/* #1 if0 */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE);
	desc = usbh_desc_get_next(desc);

	/* #1 if0_1 */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE);
	desc = usbh_desc_get_next(desc);

	/* #2 if0_1_out_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #3 if0_1_in_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #4 if0_1_int_out_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #5 if0_1_int_in_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	if (IS_ENABLED(CONFIG_USBD_LOOPBACK_ENABLE_ISO)) {
		/* #6 if0_0_iso_in_ep */
		zassert_not_null(desc);
		zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
		desc = usbh_desc_get_next(desc);

		/* #7 if2_0_iso_out_ep */
		zassert_not_null(desc);
		zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
		desc = usbh_desc_get_next(desc);
	}

	/* #14 nil_desc */
	zassert_is_null(desc);

	usb_device_unref(udev);
}

ZTEST(usbh_test, test_get_types)
{
	const struct usb_desc_header *desc;
	struct usb_host_ep *host_ep;
	struct usb_device *udev;
	int err;

	udev = usbh_device_get(uhs_ctx, TEST_DEV_ADDR);
	zassert_not_null(udev);

	/* if0 */
	desc = usbh_desc_get_iface(udev, 0);
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(((struct usb_if_descriptor *)desc)->bInterfaceNumber, 0);

	err = usbh_device_interface_set(udev, 0, 1, false);
	zassert_ok(err, "Failed to set interface alternate");

	/* if0_out_ep */
	host_ep = usbh_desc_get_endpoint(udev, 0x01);
	zassert_not_null(host_ep);
	zassert_equal(host_ep->addr, 0x01);

	/* if0_in_ep */
	host_ep = usbh_desc_get_endpoint(udev, 0x81);
	zassert_not_null(host_ep);
	zassert_equal(host_ep->addr, 0x81);

	usb_device_unref(udev);
}

ZTEST(usbh_test, test_get_next_function)
{
	const struct usb_desc_header *desc;
	struct usb_device *udev;

	udev = usbh_device_get(uhs_ctx, TEST_DEV_ADDR);
	zassert_not_null(udev);

	desc = udev->cfg_desc;
	zassert_not_null(desc);

	/* #1 Interface descriptor */
	desc = usbh_desc_get_next_function(desc);
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE);

	/* end */
	desc = usbh_desc_get_next_function(desc);
	zassert_is_null(desc);

	usb_device_unref(udev);
}

static struct usbd_context *test_usbd;

USBH_CONTROLLER_DEFINE(test_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

struct usbh_context *const uhs_ctx = &test_uhs_ctx;

/* Wait until the device is configured, and return it with a reference */
static struct usb_device *wait_device_configured(void)
{
	for (int i = 0; i < 100; i++) {
		struct usb_device *udev = usbh_device_get(uhs_ctx, TEST_DEV_ADDR);

		if (udev != NULL && udev->state == USB_STATE_CONFIGURED) {
			return udev;
		}

		if (udev != NULL) {
			usb_device_unref(udev);
		}

		k_msleep(10);
	}

	return NULL;
}

/* Wait until the device is gone from the list of devices */
static bool wait_device_gone(void)
{
	for (int i = 0; i < 100; i++) {
		struct usb_device *udev = usbh_device_get(uhs_ctx, TEST_DEV_ADDR);

		if (udev == NULL) {
			return true;
		}

		usb_device_unref(udev);
		k_msleep(10);
	}

	return false;
}

ZTEST(usbh_test, test_device_ref_outlives_disconnect)
{
	struct usb_device *new_udev;
	struct usb_device *udev;
	uint8_t addr;

	udev = wait_device_configured();
	zassert_not_null(udev, "No configured device");
	zassert_true(usb_device_is_connected(udev));
	addr = udev->addr;

	zassert_ok(usbd_disable(test_usbd));
	zassert_true(wait_device_gone(), "Device still in the list");

	zassert_false(usb_device_is_connected(udev));
	zassert_is_null(usbh_device_get(uhs_ctx, addr), "Lookup found a disconnected device");

	/*
	 * Connect the device again while the reference is held. A device that
	 * was released despite the reference would be reused for the new one.
	 */
	zassert_ok(usbd_enable(test_usbd));
	new_udev = wait_device_configured();
	zassert_not_null(new_udev, "Device not configured after reconnect");
	zassert_not_equal(new_udev, udev, "Referenced device was reused");

	zassert_false(usb_device_is_connected(udev));
	zassert_equal(udev->dev_desc.idVendor, CONFIG_SAMPLE_USBD_VID);
	zassert_equal(udev->dev_desc.idProduct, CONFIG_SAMPLE_USBD_PID);

	usb_device_unref(udev);
	usb_device_unref(new_udev);
}

ZTEST(usbh_test, test_device_reconnect_no_leak)
{
	struct usb_device *udev;

	/* A device that is not released would use up the device slots */
	for (int i = 0; i < 2 * CONFIG_USBH_USB_DEVICE_MAX; i++) {
		zassert_ok(usbd_disable(test_usbd));
		zassert_true(wait_device_gone(), "Device %d still in the list", i);

		zassert_ok(usbd_enable(test_usbd));
		udev = wait_device_configured();
		zassert_not_null(udev, "Device %d not configured", i);
		usb_device_unref(udev);
	}
}

/*
 * A device pointer kept across a disconnect must not refer to the device that
 * connects next. Otherwise a holder, such as the shell or USB/IP, acts on the
 * wrong device without noticing.
 */
ZTEST(usbh_test, test_device_stale_pointer_after_reconnect)
{
	struct usb_device *new_udev;
	struct usb_device *udev;

	udev = wait_device_configured();
	zassert_not_null(udev, "No configured device");

	zassert_ok(usbd_disable(test_usbd));
	zassert_true(wait_device_gone(), "Device still in the list");

	zassert_ok(usbd_enable(test_usbd));
	new_udev = wait_device_configured();
	zassert_not_null(new_udev, "Device not configured after reconnect");

	zassert_not_equal(new_udev, udev, "Kept pointer refers to the device that connected next");

	usb_device_unref(udev);
	usb_device_unref(new_udev);
}

static K_SEM_DEFINE(late_xfer_done, 0, 1);
static struct usb_device *late_xfer_udev;
static int late_xfer_err;

static int late_xfer_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	/* Keep the device for the checks, the transfer drops its reference */
	late_xfer_udev = usb_device_ref(udev);
	late_xfer_err = xfer->err;

	if (xfer->buf != NULL) {
		net_buf_unref(xfer->buf);
	}

	(void)uhc_xfer_unref(xfer);
	k_sem_give(&late_xfer_done);

	return 0;
}

/*
 * A transfer that completes after its device was disconnected must be given
 * the device it was queued for, not the device that connected next.
 */
ZTEST(usbh_test, test_xfer_completion_after_disconnect)
{
	const struct usb_setup_packet req = {
		.bmRequestType = USB_REQTYPE_DIR_TO_HOST << 7,
		.bRequest = USB_SREQ_GET_DESCRIPTOR,
		.wValue = sys_cpu_to_le16(USB_DESC_DEVICE << 8),
		.wLength = sys_cpu_to_le16(sizeof(struct usb_device_descriptor)),
	};
	struct usb_device *new_udev;
	struct uhc_transfer *xfer;
	struct usb_device *udev;
	struct net_buf *buf;

	udev = wait_device_configured();
	zassert_not_null(udev, "No configured device");

	/* Stop the frames, so that the transfer stays queued */
	zassert_ok(uhc_bus_suspend(uhs_ctx->dev));

	xfer = usbh_xfer_alloc(udev, USB_CONTROL_EP_IN, late_xfer_cb, NULL);
	zassert_not_null(xfer);
	memcpy(xfer->setup_pkt, &req, sizeof(req));
	buf = usbh_xfer_buf_alloc(udev, sizeof(struct usb_device_descriptor));
	zassert_not_null(buf);
	zassert_ok(usbh_xfer_buf_add(udev, xfer, buf));
	zassert_ok(usbh_xfer_enqueue(udev, xfer));

	/* Only the transfer holds the device now, udev is only compared */
	usb_device_unref(udev);

	/*
	 * Cancel the transfer, and disconnect the device. The controller
	 * returns the transfer at the next frame, after the device connects
	 * again and the bus is resumed.
	 */
	zassert_ok(usbh_xfer_dequeue(udev, xfer));

	zassert_ok(usbd_disable(test_usbd));
	zassert_true(wait_device_gone(), "Device still in the list");

	zassert_ok(usbd_enable(test_usbd));

	/* The first frame returns the cancelled transfer */
	zassert_ok(uhc_bus_resume(uhs_ctx->dev));

	zassert_ok(k_sem_take(&late_xfer_done, K_SECONDS(1)), "Transfer not returned");
	zassert_equal(late_xfer_err, -ECONNRESET);

	new_udev = wait_device_configured();
	zassert_not_null(new_udev, "Device not configured after reconnect");

	zassert_equal(late_xfer_udev, udev, "Completion got another device");
	zassert_not_equal(late_xfer_udev, new_udev,
			  "Completion got the device that connected next");
	zassert_false(usb_device_is_connected(late_xfer_udev));

	usb_device_unref(late_xfer_udev);
	usb_device_unref(new_udev);
}

void *usbh_test_enable(void)
{
	int ret;

	ret = usbh_init(uhs_ctx);
	zassert_ok(ret, "Failed to initialize USB host");

	ret = usbh_enable(uhs_ctx);
	zassert_ok(ret, "Failed to enable USB host");

	ret = uhc_bus_resume(uhs_ctx->dev);
	zassert_ok(ret, "Failed to signal bus resume");

	LOG_INF("Host controller enabled");

	test_usbd = sample_usbd_setup_device(NULL);
	zassert_not_null(test_usbd, "Failed to setup USB device");

	ret = usbd_init(test_usbd);
	zassert_ok(ret, "Failed to initialize device support");

	ret = usbd_enable(test_usbd);
	zassert_ok(ret, "Failed to enable device support");

	LOG_INF("Device support enabled");

	/*
	 * Allow the host time to see the device on the root hub, which it polls,
	 * and to reset it.
	 */
	k_msleep(CONFIG_USBH_HUB_ROOT_POLL_INTERVAL_MS + 200);

	return NULL;
}

void usbh_test_shutdown(void *f)
{
	int ret;

	ret = usbd_disable(test_usbd);
	zassert_ok(ret, "Failed to disable device support");

	ret = usbd_shutdown(test_usbd);
	zassert_ok(ret, "Failed to shutdown device support");

	LOG_INF("Device support disabled");

	ret = usbh_disable(uhs_ctx);
	zassert_ok(ret, "Failed to disable USB host");

	ret = usbh_shutdown(uhs_ctx);
	zassert_ok(ret, "Failed to shutdown host support");

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(usbh_test, NULL, usbh_test_enable, NULL, NULL, usbh_test_shutdown);
