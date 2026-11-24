/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>

#include <usbh_ch9.h>
#include <usbh_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_test, LOG_LEVEL_INF);

#define TEST_DEFAULT_INTERFACE		0
#define TEST_DEFAULT_ALTERNATE		1
#define TEST_BULK_OUT			0x01
#define TEST_BULK_IN			0x81

USBD_CONFIGURATION_DEFINE(test_fs_config,
			  USB_SCD_SELF_POWERED | USB_SCD_REMOTE_WAKEUP,
			  200, NULL);

USBD_CONFIGURATION_DEFINE(test_hs_config,
			  USB_SCD_SELF_POWERED | USB_SCD_REMOTE_WAKEUP,
			  200, NULL);


USBD_DESC_LANG_DEFINE(test_lang);
USBD_DESC_STRING_DEFINE(test_mfg, "ZEPHYR", 1);
USBD_DESC_STRING_DEFINE(test_product, "Zephyr USB Test", 2);
USBD_DESC_STRING_DEFINE(test_sn, "0123456789ABCDEF", 3);

USBD_DEVICE_DEFINE(test_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0xffff);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static int test_cmp_string_desc(struct net_buf *const buf, const int idx)
{
	static struct usbd_desc_node *desc_nd;
	size_t len;

	if (idx == test_mfg.str.idx) {
		desc_nd = &test_mfg;
	} else if (idx == test_product.str.idx) {
		desc_nd = &test_product;
	} else if (idx == test_sn.str.idx) {
		desc_nd = &test_sn;
	} else {
		return -ENOTSUP;
	}

	if (net_buf_pull_u8(buf) != desc_nd->bLength) {
		return -EINVAL;
	}

	if (net_buf_pull_u8(buf) != USB_DESC_STRING) {
		return -EINVAL;
	}

	LOG_HEXDUMP_DBG(buf->data, buf->len, "");
	len = MIN(buf->len / 2, desc_nd->bLength / 2);
	for (size_t i = 0; i < len; i++) {
		uint16_t a = net_buf_pull_le16(buf);
		uint16_t b = ((uint8_t *)(desc_nd->ptr))[i];

		if (a != b) {
			LOG_INF("%c != %c", a, b);
			return -EINVAL;
		}
	}

	return 0;
}

ZTEST(device_next, test_get_desc_string)
{
	const uint8_t type = USB_DESC_STRING;
	const uint16_t id = 0x0409;
	static struct usb_device *udev;
	struct net_buf *buf;
	int err;

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");

	buf = usbh_xfer_buf_alloc(udev, UINT8_MAX);
	zassert_not_null(udev, "Failed to allocate buffer");

	err = k_mutex_lock(&udev->mutex, K_MSEC(200));
	zassert_equal(err, 0, "Failed to lock device");

	err = usbh_req_desc(udev, type, 1, id, UINT8_MAX, buf);
	zassert_equal(err, 0, "Transfer status is an error");
	err = test_cmp_string_desc(buf, 1);
	zassert_equal(err, 0, "Descriptor comparison failed");

	net_buf_reset(buf);
	err = usbh_req_desc(udev, type, 2, id, UINT8_MAX, buf);
	zassert_equal(err, 0, "Transfer status is an error");
	err = test_cmp_string_desc(buf, 2);
	zassert_equal(err, 0, "Descriptor comparison failed");

	net_buf_reset(buf);
	err = usbh_req_desc(udev, type, 3, id, UINT8_MAX, buf);
	zassert_equal(err, 0, "Transfer status is an error");
	err = test_cmp_string_desc(buf, 3);
	zassert_equal(err, 0, "Descriptor comparison failed");

	k_mutex_unlock(&udev->mutex);
	usbh_xfer_buf_free(udev, buf);
}

ZTEST(device_next, test_vendor_control_in)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
				      (USB_REQTYPE_TYPE_VENDOR << 5);
	static struct usb_device *udev;
	const uint8_t bRequest = 0x5c;
	const uint16_t wLength = 64;
	struct net_buf *buf;
	int err;

	if (!IS_ENABLED(CONFIG_UHC_VIRTUAL)) {
		LOG_WRN("The test was skipped, controller is not supported.");
		return;
	}

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");

	buf = usbh_xfer_buf_alloc(udev, wLength);
	zassert_not_null(udev, "Failed to allocate buffer");

	err = k_mutex_lock(&udev->mutex, K_MSEC(200));
	zassert_equal(err, 0, "Failed to lock device");

	/* Perform regular vendor IN transfer */
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT; i++) {
		net_buf_reset(buf);
		err = usbh_req_setup(udev, bmRequestType, bRequest, 0, 0, wLength, buf);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	/* Perform vendor IN transfer but omit status stage*/
	usbh_req_omit_status(true);
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT * 2; i++) {
		net_buf_reset(buf);
		err = usbh_req_setup(udev, bmRequestType, bRequest, 0, 0, wLength, buf);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	/* Perform vendor IN requests but omit data and status stage*/
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT * 2; i++) {
		err = usbh_req_setup(udev, bmRequestType, bRequest, 0, 0, wLength, NULL);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	usbh_req_omit_status(false);

	/* Perform regular vendor IN transfer again */
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT; i++) {
		net_buf_reset(buf);
		err = usbh_req_setup(udev, bmRequestType, bRequest, 0, 0, wLength, buf);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	k_mutex_unlock(&udev->mutex);
	usbh_xfer_buf_free(udev, buf);
}

ZTEST(device_next, test_vendor_control_out)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_DEVICE << 7) |
				      (USB_REQTYPE_TYPE_VENDOR << 5);
	const uint8_t bRequest = 0x5b;
	static struct usb_device *udev;
	const uint16_t wLength = 64;
	struct net_buf *buf;
	int err;

	if (!IS_ENABLED(CONFIG_UHC_VIRTUAL)) {
		LOG_WRN("The test was skipped, controller is not supported.");
		return;
	}

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");

	buf = usbh_xfer_buf_alloc(udev, wLength);
	zassert_not_null(udev, "Failed to allocate buffer");

	err = k_mutex_lock(&udev->mutex, K_MSEC(200));
	zassert_equal(err, 0, "Failed to lock device");

	/* Perform regular vendor OUT transfer */
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT; i++) {
		net_buf_reset(buf);
		for (uint32_t n = 0; n < wLength; n++) {
			net_buf_add_u8(buf, n);
		}

		err = usbh_req_setup(udev, bmRequestType, bRequest, 0, 0, wLength, buf);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	/* Perform vendor OUT transfer but omit status stage*/
	usbh_req_omit_status(true);
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT * 2; i++) {
		net_buf_reset(buf);
		for (uint32_t n = 0; n < wLength; n++) {
			net_buf_add_u8(buf, n);
		}

		err = usbh_req_setup(udev, bmRequestType, bRequest, 0, 0, wLength, buf);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	/* Perform vendor OUT requests but omit data and status stage*/
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT * 2; i++) {
		err = usbh_req_setup(udev, bmRequestType, bRequest, 0, 0, wLength, NULL);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	usbh_req_omit_status(false);

	/* Perform regular vendor OUT transfer again */
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT; i++) {
		net_buf_reset(buf);
		for (uint32_t n = 0; n < wLength; n++) {
			net_buf_add_u8(buf, n);
		}

		err = usbh_req_setup(udev, bmRequestType, bRequest, 0, 0, wLength, buf);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	k_mutex_unlock(&udev->mutex);
	usbh_xfer_buf_free(udev, buf);
}

ZTEST(device_next, test_control_nodata)
{
	const uint8_t bmRequestType = USB_REQTYPE_RECIPIENT_ENDPOINT;
	const uint8_t bRequest = USB_SREQ_CLEAR_FEATURE;
	const uint16_t wValue = USB_SFS_ENDPOINT_HALT;
	const uint16_t wIndex = USB_CONTROL_EP_OUT;
	static struct usb_device *udev;
	int err;

	if (!IS_ENABLED(CONFIG_UHC_VIRTUAL)) {
		LOG_WRN("The test was skipped, controller is not supported.");
		return;
	}

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");

	err = k_mutex_lock(&udev->mutex, K_MSEC(200));
	zassert_equal(err, 0, "Failed to lock device");

	/* Perform regular control transfer */
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT; i++) {
		err = usbh_req_setup(udev,
				     bmRequestType, bRequest, wValue, wIndex, 0,
				     NULL);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	/* Perform transfer but omit status stage*/
	usbh_req_omit_status(true);
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT * 2; i++) {
		err = usbh_req_setup(udev,
				     bmRequestType, bRequest, wValue, wIndex, 0,
				     NULL);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	usbh_req_omit_status(false);

	/* Perform regular control transfer again */
	for (uint32_t i = 0; i < CONFIG_UDC_BUF_COUNT; i++) {
		err = usbh_req_setup(udev,
				     bmRequestType, bRequest, wValue, wIndex, 0,
				     NULL);
		zassert_equal(err, 0, "Transfer status is an error");
	}

	k_mutex_unlock(&udev->mutex);
}

/* Get Configuration request test */
ZTEST(device_next, test_get_configuration)
{
	struct usb_device *udev;
	uint8_t cfg = 0;
	int err;

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");

	err = k_mutex_lock(&udev->mutex, K_MSEC(200));
	zassert_equal(err, 0, "Failed to lock device");

	err = usbh_req_get_cfg(udev, &cfg);
	k_mutex_unlock(&udev->mutex);

	switch (udev->state) {
	case USB_STATE_DEFAULT:
		/* Not specified, expect protocol error */
		zassert_equal(err, -EPIPE, "Transfer status is not a protocol error");
		break;
	case USB_STATE_ADDRESSED:
		/* TODO: Expect zero value */
		zassert_equal(err, 0, "Transfer status is an error");
		zassert_equal(cfg, 0, "Device not in address state");
		break;
	case USB_STATE_CONFIGURED:
		/* TODO: Expect non-zero valid configuration value */
		zassert_equal(err, 0, "Transfer status is an error");
		zassert_not_equal(cfg, 0, "Device not in configured state");
		break;
	default:
		break;
	}
}

static void check_loopback_configuration(const struct net_buf *buf, uint8_t type, uint16_t mps)
{
	static const struct {
		uint8_t addr;
		uint8_t type;
	} endpoints[] = {
		{TEST_BULK_OUT, USB_EP_TYPE_BULK},
		{TEST_BULK_IN, USB_EP_TYPE_BULK},
		{0x02, USB_EP_TYPE_INTERRUPT},
		{0x82, USB_EP_TYPE_INTERRUPT},
		{0x03, USB_EP_TYPE_ISO},
		{0x83, USB_EP_TYPE_ISO},
	};
	const size_t active_count = IS_ENABLED(CONFIG_USBD_LOOPBACK_BULK_ONLY) ? 2 :
				    IS_ENABLED(CONFIG_USBD_LOOPBACK_ENABLE_ISO) ? 6 : 4;
	const struct usb_cfg_descriptor *cfg;
	unsigned int alt = 0;
	uint32_t seen = 0;

	zassert_true(buf->len >= sizeof(*cfg));
	cfg = (const struct usb_cfg_descriptor *)buf->data;
	zassert_equal(cfg->bLength, sizeof(*cfg));
	zassert_equal(cfg->bDescriptorType, type);
	zassert_equal(sys_le16_to_cpu(cfg->wTotalLength), buf->len);
	zassert_equal(cfg->bNumInterfaces, 1);

	for (size_t offset = cfg->bLength; offset < buf->len;) {
		const struct usb_desc_header *desc = (const void *)(buf->data + offset);

		zassert_true(buf->len - offset >= sizeof(*desc));
		zassert_true(desc->bLength >= sizeof(*desc));
		zassert_true(desc->bLength <= buf->len - offset);
		zassert_not_equal(desc->bDescriptorType, USB_DESC_INTERFACE_ASSOC);
		if (desc->bDescriptorType == USB_DESC_INTERFACE) {
			const struct usb_if_descriptor *iface = (const void *)desc;

			zassert_equal(desc->bLength, sizeof(*iface));
			zassert_true(alt <= TEST_DEFAULT_ALTERNATE, "Unexpected interface");
			zassert_equal(iface->bInterfaceNumber, TEST_DEFAULT_INTERFACE);
			zassert_equal(iface->bAlternateSetting, alt);
			zassert_equal(iface->bNumEndpoints, alt == 0 ? 0 : active_count);
			alt++;
		} else if (desc->bDescriptorType == USB_DESC_ENDPOINT) {
			const struct usb_ep_descriptor *ep = (const void *)desc;
			size_t i;

			zassert_equal(alt, TEST_DEFAULT_ALTERNATE + 1,
				      "Endpoint outside the active alternate setting");
			zassert_equal(desc->bLength, sizeof(*ep));
			for (i = 0; i < active_count; i++) {
				if (ep->bEndpointAddress == endpoints[i].addr) {
					break;
				}
			}
			zassert_true(i < active_count, "Unexpected endpoint 0x%02x",
				     ep->bEndpointAddress);
			zassert_false(seen & BIT(i), "Duplicate endpoint 0x%02x",
				      ep->bEndpointAddress);
			seen |= BIT(i);
			zassert_equal(ep->bmAttributes & USB_EP_TRANSFER_TYPE_MASK,
				      endpoints[i].type);
			if (endpoints[i].type == USB_EP_TYPE_BULK) {
				zassert_equal(ep->bmAttributes, USB_EP_TYPE_BULK);
				zassert_equal(sys_le16_to_cpu(ep->wMaxPacketSize), mps);
				zassert_equal(ep->bInterval, 0);
			}
		}
		offset += desc->bLength;
	}

	zassert_equal(alt, TEST_DEFAULT_ALTERNATE + 1);
	zassert_equal(seen, BIT_MASK(active_count), "Missing endpoints in active alternate");
}

ZTEST(device_next, test_loopback_configuration_descriptor)
{
	struct usb_device *udev = usbh_device_get_any(&uhs_ctx);
	struct net_buf *buf;
	bool high_speed;
	int err;

	zassert_not_null(udev);
	high_speed = udev->speed == USB_SPEED_SPEED_HS;
	buf = usbh_xfer_buf_alloc(udev, UINT8_MAX);
	zassert_not_null(buf);
	zassert_ok(k_mutex_lock(&udev->mutex, K_MSEC(200)));
	err = usbh_req_desc(udev, USB_DESC_CONFIGURATION, 0, 0, UINT8_MAX, buf);
	k_mutex_unlock(&udev->mutex);
	zassert_ok(err);
	check_loopback_configuration(buf, USB_DESC_CONFIGURATION, high_speed ? 512 : 64);

	if (USBD_SUPPORTS_HIGH_SPEED && usbd_caps_speed(&test_usbd) == USBD_SPEED_HS) {
		net_buf_reset(buf);
		zassert_ok(k_mutex_lock(&udev->mutex, K_MSEC(200)));
		err = usbh_req_desc(udev, USB_DESC_OTHER_SPEED, 0, 0, UINT8_MAX, buf);
		k_mutex_unlock(&udev->mutex);
		zassert_ok(err);
		check_loopback_configuration(buf, USB_DESC_OTHER_SPEED, high_speed ? 64 : 512);
	}
	usbh_xfer_buf_free(udev, buf);
}

/* Set Interface request test */
ZTEST(device_next, test_set_interface)
{
	struct usb_device *udev;
	int err;

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");

	err = k_mutex_lock(&udev->mutex, K_MSEC(200));
	zassert_equal(err, 0, "Failed to lock device");

	err = usbh_req_set_alt(udev, TEST_DEFAULT_INTERFACE,
			       TEST_DEFAULT_ALTERNATE);
	k_mutex_unlock(&udev->mutex);

	switch (udev->state) {
	case USB_STATE_DEFAULT:
		/* Not specified, expect protocol error */
	case USB_STATE_ADDRESSED:
		/* Expect protocol error */
		zassert_equal(err, -EPIPE, "Transfer status is not a protocol error");
		break;
	case USB_STATE_CONFIGURED:
		/* TODO */
	default:
		break;
	}
}

static int test_get_interface(struct usb_device *const udev, const uint8_t iface,
			      uint8_t *const alt)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
				      USB_REQTYPE_RECIPIENT_INTERFACE;
	struct net_buf *buf;
	int err;

	buf = usbh_xfer_buf_alloc(udev, 1);
	if (buf == NULL) {
		return -ENOMEM;
	}

	err = usbh_req_setup(udev, bmRequestType, USB_SREQ_GET_INTERFACE,
			     0, iface, 1, buf);
	if (err == 0) {
		if (buf->len == 1) {
			*alt = net_buf_pull_u8(buf);
		} else {
			err = -EINVAL;
		}
	}

	usbh_xfer_buf_free(udev, buf);

	return err;
}

static int bulk_complete(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct k_sem *done = xfer->priv;

	ARG_UNUSED(udev);
	k_sem_give(done);
	return 0;
}

static int bulk_transfer(struct usb_device *udev, uint8_t ep, struct net_buf *buf)
{
	struct uhc_transfer *xfer;
	struct k_sem done;
	int err;

	k_sem_init(&done, 0, 1);
	xfer = usbh_xfer_alloc(udev, ep, bulk_complete, &done);
	if (xfer == NULL) {
		return -ENOMEM;
	}

	err = usbh_xfer_buf_add(udev, xfer, buf);
	if (err == 0) {
		err = usbh_xfer_enqueue(udev, xfer);
	}
	if (err == 0) {
		if (k_sem_take(&done, K_SECONDS(1)) == 0) {
			err = xfer->err;
		} else {
			usbh_xfer_dequeue(udev, xfer);
			/* The virtual host returns the transfer asynchronously after dequeue. */
			k_sem_take(&done, K_FOREVER);
			err = -ETIMEDOUT;
		}
	}
	(void)uhc_xfer_unref(xfer);

	return err;
}

ZTEST(device_next, test_loopback_bulk_only_transfer)
{
	static const uint8_t payload[] = {0xa5, 0x39, 0x00, 0xc7, 0x5a};
	struct usb_device *udev;
	struct net_buf *buf;
	uint8_t alt = UINT8_MAX;
	int err;

	if (!IS_ENABLED(CONFIG_USBD_LOOPBACK_BULK_ONLY) || !IS_ENABLED(CONFIG_UHC_VIRTUAL)) {
		ztest_test_skip();
	}

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev);
	zassert_equal(udev->state, USB_STATE_CONFIGURED);
	/* SET_INTERFACE alone does not populate the host's endpoint table. */
	zassert_ok(usbh_device_interface_set(udev, TEST_DEFAULT_INTERFACE, 0, false));
	zassert_ok(usbh_device_interface_set(udev, TEST_DEFAULT_INTERFACE,
					    TEST_DEFAULT_ALTERNATE, false));
	zassert_ok(k_mutex_lock(&udev->mutex, K_MSEC(200)));
	err = test_get_interface(udev, TEST_DEFAULT_INTERFACE, &alt);
	k_mutex_unlock(&udev->mutex);
	zassert_ok(err);
	zassert_equal(alt, TEST_DEFAULT_ALTERNATE);

	buf = usbh_xfer_buf_alloc(udev, sizeof(payload));
	zassert_not_null(buf);
	net_buf_add_mem(buf, payload, sizeof(payload));
	err = bulk_transfer(udev, TEST_BULK_OUT, buf);
	zassert_ok(err, "Bulk OUT failed");
	zassert_equal(buf->len, 0, "Bulk OUT was not consumed");

	/* The first IN buffer was queued on alt selection, before the OUT payload. */
	for (size_t i = 0; i < 2; i++) {
		net_buf_reset(buf);
		err = bulk_transfer(udev, TEST_BULK_IN, buf);
		zassert_ok(err, "Bulk IN failed");
		zassert_equal(buf->len, sizeof(payload), "Unexpected bulk IN length");
	}
	zassert_mem_equal(buf->data, payload, sizeof(payload), "Bulk loopback payload mismatch");
	usbh_xfer_buf_free(udev, buf);

	zassert_ok(usbh_device_interface_set(udev, TEST_DEFAULT_INTERFACE, 0, false));
}

static int test_get_endpoint_status(struct usb_device *const udev, const uint8_t ep,
				    uint16_t *const status)
{
	const uint8_t bmRequestType = (USB_REQTYPE_DIR_TO_HOST << 7) |
				      USB_REQTYPE_RECIPIENT_ENDPOINT;
	struct net_buf *buf;
	int err;

	buf = usbh_xfer_buf_alloc(udev, sizeof(*status));
	if (buf == NULL) {
		return -ENOMEM;
	}

	err = usbh_req_setup(udev, bmRequestType, USB_SREQ_GET_STATUS,
			     0, ep, sizeof(*status), buf);
	if (err == 0) {
		if (buf->len == sizeof(*status)) {
			*status = net_buf_pull_le16(buf);
		} else {
			err = -EINVAL;
		}
	}

	usbh_xfer_buf_free(udev, buf);

	return err;
}

/* Set Configuration request for the active configuration test */
ZTEST(device_next, test_set_configuration_again)
{
	const uint8_t endpoints[] = {0x01, 0x81};
	struct usb_device *udev;
	uint16_t status = UINT16_MAX;
	uint8_t alt = UINT8_MAX;
	uint8_t cfg = 0;
	int err;

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "No USB device available");

	if (udev->state != USB_STATE_CONFIGURED) {
		ztest_test_skip();
	}

	err = k_mutex_lock(&udev->mutex, K_MSEC(200));
	zassert_equal(err, 0, "Failed to lock device");

	err = usbh_req_get_cfg(udev, &cfg);
	zassert_equal(err, 0, "Transfer status is an error");

	err = usbh_req_set_alt(udev, TEST_DEFAULT_INTERFACE,
			       TEST_DEFAULT_ALTERNATE);
	zassert_equal(err, 0, "Transfer status is an error");

	err = test_get_interface(udev, TEST_DEFAULT_INTERFACE, &alt);
	zassert_equal(err, 0, "Transfer status is an error");
	zassert_equal(alt, TEST_DEFAULT_ALTERNATE,
		      "Alternate setting was not selected");

	for (size_t i = 0; i < ARRAY_SIZE(endpoints); i++) {
		err = usbh_req_set_sfs_halt(udev, endpoints[i]);
		zassert_equal(err, 0, "Transfer status is an error");

		err = test_get_endpoint_status(udev, endpoints[i], &status);
		zassert_equal(err, 0, "Transfer status is an error");
		zassert_equal(status, BIT(0), "Endpoint 0x%02x is not halted",
			      endpoints[i]);
	}

	err = usbh_req_set_cfg(udev, cfg);
	zassert_equal(err, 0, "Transfer status is an error");

	err = test_get_interface(udev, TEST_DEFAULT_INTERFACE, &alt);
	zassert_equal(err, 0, "Transfer status is an error");
	zassert_equal(alt, 0, "Alternate setting was not reset");

	/* Alternate setting zero has no endpoints, select the default again */
	err = usbh_req_set_alt(udev, TEST_DEFAULT_INTERFACE,
			       TEST_DEFAULT_ALTERNATE);
	zassert_equal(err, 0, "Transfer status is an error");

	for (size_t i = 0; i < ARRAY_SIZE(endpoints); i++) {
		err = test_get_endpoint_status(udev, endpoints[i], &status);
		zassert_equal(err, 0, "Transfer status is an error");
		zassert_equal(status, 0, "Endpoint 0x%02x is still halted",
			      endpoints[i]);
	}

	k_mutex_unlock(&udev->mutex);
}

static void *usb_test_enable(void)
{
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

	LOG_INF("Host controller enabled");

	err = usbd_add_descriptor(&test_usbd, &test_lang);
	zassert_equal(err, 0, "Failed to initialize descriptor (%d)", err);

	err = usbd_add_descriptor(&test_usbd, &test_mfg);
	zassert_equal(err, 0, "Failed to initialize descriptor (%d)", err);

	err = usbd_add_descriptor(&test_usbd, &test_product);
	zassert_equal(err, 0, "Failed to initialize descriptor (%d)", err);

	err = usbd_add_descriptor(&test_usbd, &test_sn);
	zassert_equal(err, 0, "Failed to initialize descriptor (%d)", err);

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&test_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&test_usbd, USBD_SPEED_HS, &test_hs_config);
		zassert_equal(err, 0, "Failed to add configuration (%d)", err);
	}

	err = usbd_add_configuration(&test_usbd, USBD_SPEED_FS, &test_fs_config);
	zassert_equal(err, 0, "Failed to add configuration (%d)", err);

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&test_usbd) == USBD_SPEED_HS) {
		err = usbd_register_all_classes(&test_usbd, USBD_SPEED_HS, 1, NULL);
		zassert_equal(err, 0, "Failed to unregister all instances(%d)", err);

		err = usbd_unregister_all_classes(&test_usbd, USBD_SPEED_HS, 1);
		zassert_equal(err, 0, "Failed to unregister all instances(%d)", err);

		err = usbd_register_class(&test_usbd, "loopback_0", USBD_SPEED_HS, 1);
		zassert_equal(err, 0, "Failed to register loopback_0 class (%d)", err);
	}

	err = usbd_register_all_classes(&test_usbd, USBD_SPEED_FS, 1, NULL);
	zassert_equal(err, 0, "Failed to unregister all instances(%d)", err);

	err = usbd_unregister_all_classes(&test_usbd, USBD_SPEED_FS, 1);
	zassert_equal(err, 0, "Failed to unregister all instances(%d)", err);

	err = usbd_register_class(&test_usbd, "loopback_0", USBD_SPEED_FS, 1);
	zassert_equal(err, 0, "Failed to register loopback_0 class (%d)", err);

	err = usbd_init(&test_usbd);
	zassert_equal(err, 0, "Failed to initialize device support");

	err = usbd_enable(&test_usbd);
	zassert_equal(err, 0, "Failed to enable device support");

	LOG_INF("Device support enabled");

	/* Allow the host time to reset the device. */
	k_msleep(200);

	return NULL;
}

static void usb_test_shutdown(void *f)
{
	int err;

	err = usbd_disable(&test_usbd);
	zassert_equal(err, 0, "Failed to enable device support");

	err = usbd_shutdown(&test_usbd);
	zassert_equal(err, 0, "Failed to shutdown device support");

	LOG_INF("Device support disabled");

	err = usbh_disable(&uhs_ctx);
	zassert_equal(err, 0, "Failed to disable USB host");

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(device_next, NULL, usb_test_enable, NULL, NULL, usb_test_shutdown);
