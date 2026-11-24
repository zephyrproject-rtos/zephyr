/* SPDX-License-Identifier: Apache-2.0 */
#include <zephyr/ztest.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbd_dfu.h>

#include "usbd_msg.h"
#include "usbh_ch9.h"
#include "usbh_device.h"

#define MESSAGE_BARRIER_STATUS 0x5342

USBD_CONFIGURATION_DEFINE(test_fs_config, USB_SCD_SELF_POWERED, 100, NULL);
USBD_CONFIGURATION_DEFINE(test_hs_config, USB_SCD_SELF_POWERED, 100, NULL);
USBD_DESC_LANG_DEFINE(test_lang);
USBD_DEVICE_DEFINE(test_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2fe3, 0xffff);
USBH_CONTROLLER_DEFINE(test_usbh, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static struct {
	bool ready;
	enum usb_dfu_state state;
	enum usb_dfu_state next;
	unsigned int writes;
	unsigned int next_calls;
} backend = {.ready = true};

static atomic_t completions;
static K_SEM_DEFINE(message_barrier, 0, 1);
static struct usb_device *udev;

static int image_write(void *priv, uint32_t block, uint16_t size,
		       const uint8_t buf[static CONFIG_USBD_DFU_TRANSFER_SIZE])
{
	backend.writes++;
	return 0;
}

static bool image_next(void *priv, enum usb_dfu_state state, enum usb_dfu_state next)
{
	backend.next_calls++;
	backend.state = state;
	backend.next = next;
	return backend.ready;
}

USBD_DFU_DEFINE_IMG(0, "requests", NULL, NULL, image_write, image_next);

static void message_cb(struct usbd_context *ctx, const struct usbd_msg *msg)
{
	if (msg->type == USBD_MSG_DFU_DOWNLOAD_COMPLETED) {
		atomic_inc(&completions);
	} else if (msg->type == USBD_MSG_DFU_APP_DETACH && msg->status == MESSAGE_BARRIER_STATUS) {
		k_sem_give(&message_barrier);
	}
}

static void drain_messages(void)
{
	/* The sentinel is delivered after any completion sent by the request. */
	usbd_msg_pub_simple(&test_usbd, USBD_MSG_DFU_APP_DETACH, MESSAGE_BARRIER_STATUS);
	zassert_ok(k_sem_take(&message_barrier, K_SECONDS(1)), "message queue did not drain");
}

static int out_request(uint8_t request, uint16_t length)
{
	struct net_buf *buf = NULL;
	int ret;

	if (length != 0) {
		buf = usbh_xfer_buf_alloc(udev, length);
		zassert_not_null(buf);
		net_buf_add_u8(buf, 0x5a);
	}
	ret = usbh_req_setup(udev, 0x21, request, 0, 0, length, buf);
	if (buf != NULL) {
		usbh_xfer_buf_free(udev, buf);
	}
	return ret;
}

static void expect_status(enum usb_dfu_status status, enum usb_dfu_state state)
{
	struct net_buf *buf = usbh_xfer_buf_alloc(udev, 6);

	zassert_not_null(buf);
	zassert_ok(usbh_req_setup(udev, 0xa1, USB_DFU_REQ_GETSTATUS, 0, 0, 6, buf));
	zassert_equal(buf->len, 6);
	zassert_equal(buf->data[0], status);
	zassert_equal(buf->data[4], state);
	usbh_xfer_buf_free(udev, buf);
}

static void expect_state(enum usb_dfu_state state)
{
	struct net_buf *buf = usbh_xfer_buf_alloc(udev, 1);

	zassert_not_null(buf);
	zassert_ok(usbh_req_setup(udev, 0xa1, USB_DFU_REQ_GETSTATE, 0, 0, 1, buf));
	zassert_equal(buf->len, 1);
	zassert_equal(buf->data[0], state);
	usbh_xfer_buf_free(udev, buf);
}

static void begin_manifest(void)
{
	backend.ready = true;
	zassert_ok(out_request(USB_DFU_REQ_DNLOAD, 1));
	expect_status(ERR_OK, DFU_DNLOAD_IDLE);
	zassert_ok(out_request(USB_DFU_REQ_DNLOAD, 0));
	expect_state(DFU_MANIFEST_SYNC);
}

ZTEST(dfu, test_manifest)
{
	const bool tolerant = IS_ENABLED(CONFIG_USBD_DFU_MANIFESTATION_TOLERANT);

	begin_manifest();
	backend.ready = false;
	expect_status(ERR_OK, DFU_MANIFEST);

	if (tolerant) {
		backend.ready = true;
		expect_status(ERR_OK, DFU_IDLE);
		drain_messages();
		zassert_equal(atomic_get(&completions), 1);
	} else {
		expect_status(ERR_OK, DFU_MANIFEST_WAIT_RST);
		expect_state(DFU_MANIFEST_WAIT_RST);
		drain_messages();
		zassert_equal(atomic_get(&completions), 0);
		zassert_equal(backend.state, DFU_MANIFEST);
		zassert_equal(backend.next, DFU_MANIFEST_WAIT_RST);
	}
	zassert_equal(backend.next_calls, 3);
}

static void *setup(void)
{
	zassert_ok(usbh_init(&test_usbh));
	zassert_ok(usbh_enable(&test_usbh));
	zassert_ok(uhc_bus_reset(test_usbh.dev));
	zassert_ok(uhc_bus_resume(test_usbh.dev));
	zassert_ok(uhc_sof_enable(test_usbh.dev));
	zassert_ok(usbd_add_descriptor(&test_usbd, &test_lang));
	zassert_ok(usbd_msg_register_cb(&test_usbd, message_cb));
	if (USBD_SUPPORTS_HIGH_SPEED) {
		zassert_ok(usbd_add_configuration(&test_usbd, USBD_SPEED_HS, &test_hs_config));
		zassert_ok(usbd_register_class(&test_usbd, "dfu_dfu", USBD_SPEED_HS, 1));
	}
	zassert_ok(usbd_add_configuration(&test_usbd, USBD_SPEED_FS, &test_fs_config));
	zassert_ok(usbd_register_class(&test_usbd, "dfu_dfu", USBD_SPEED_FS, 1));
	zassert_ok(usbd_init(&test_usbd));
	zassert_ok(usbd_enable(&test_usbd));
	/* Wait for the virtual host to configure the device. */
	k_msleep(200);
	udev = usbh_device_get_any(&test_usbh);
	zassert_not_null(udev);
	return NULL;
}

ZTEST_SUITE(dfu, NULL, setup, NULL, NULL, NULL);
