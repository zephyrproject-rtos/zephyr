/*
 * Copyright (c) 2026 Tolt Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbd_cdc_eth.h>

#include <usbh_ch9.h>
#include <usbh_device.h>

#define TEST_LANGID         0x0409
#define TEST_MAX_STRING_IDX 16

USBD_CONFIGURATION_DEFINE(test_fs_config, 0, 200, NULL);
USBD_CONFIGURATION_DEFINE(test_hs_config, 0, 200, NULL);
USBD_DESC_LANG_DEFINE(test_lang);

USBD_DEVICE_DEFINE(test_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2fe3, 0xffff);
USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static const struct device *const ecm_dev = DEVICE_DT_GET(DT_NODELABEL(cdc_ecm_eth0));
static const struct device *const ncm_dev = DEVICE_DT_GET(DT_NODELABEL(cdc_ncm_eth0));

static const uint8_t ecm_mac[NET_ETH_ADDR_LEN] = {0x02, 0x00, 0x5e, 0x00, 0x53, 0xa1};
static const uint8_t ncm_mac[NET_ETH_ADDR_LEN] = {0x02, 0x00, 0x5e, 0x00, 0x53, 0xb2};

static bool test_string_matches(struct net_buf *const buf, const char *const str)
{
	const size_t len = strlen(str);
	const size_t blength = 2 + 2 * len;

	if (buf->len != blength || net_buf_pull_u8(buf) != blength ||
	    net_buf_pull_u8(buf) != USB_DESC_STRING) {
		return false;
	}

	for (size_t i = 0; i < len; i++) {
		if (net_buf_pull_le16(buf) != str[i]) {
			return false;
		}
	}

	return true;
}

static bool test_host_sees_string(const char *const str)
{
	struct usb_device *udev = usbh_device_get_any(&uhs_ctx);
	struct net_buf *buf;
	bool found = false;

	zassert_not_null(udev, "No USB device available");
	buf = usbh_xfer_buf_alloc(udev, UINT8_MAX);
	zassert_not_null(buf, "Failed to allocate buffer");
	zassert_ok(k_mutex_lock(&udev->mutex, K_MSEC(200)), "Failed to lock device");

	for (uint8_t idx = 1; idx < TEST_MAX_STRING_IDX && !found; idx++) {
		net_buf_reset(buf);
		if (usbh_req_desc(udev, USB_DESC_STRING, idx, TEST_LANGID, UINT8_MAX, buf)) {
			break;
		}

		found = test_string_matches(buf, str);
	}

	k_mutex_unlock(&udev->mutex);
	usbh_xfer_buf_free(udev, buf);

	return found;
}

static void test_register_classes(const enum usbd_speed speed,
				  struct usbd_config_node *const config)
{
	zassert_ok(usbd_add_configuration(&test_usbd, speed, config));
	zassert_ok(usbd_register_class(&test_usbd, "cdc_ecm_0", speed, 1));
	zassert_ok(usbd_register_class(&test_usbd, "cdc_ncm_0", speed, 1));
}

ZTEST(cdc_eth, test_set_remote_mac_rejects_invalid)
{
	static const uint8_t zero[NET_ETH_ADDR_LEN];
	static const uint8_t multicast[NET_ETH_ADDR_LEN] = {0x03, 0x00, 0x5e, 0x00, 0x53, 0x01};

	zassert_equal(usbd_cdc_ecm_set_remote_mac(ecm_dev, zero), -EINVAL);
	zassert_equal(usbd_cdc_ecm_set_remote_mac(ecm_dev, multicast), -EINVAL);
	zassert_equal(usbd_cdc_ncm_set_remote_mac(ncm_dev, zero), -EINVAL);
	zassert_equal(usbd_cdc_ncm_set_remote_mac(ncm_dev, multicast), -EINVAL);
}

ZTEST(cdc_eth, test_set_remote_mac_reaches_host)
{
	zassert_ok(usbd_cdc_ecm_set_remote_mac(ecm_dev, ecm_mac));
	zassert_ok(usbd_cdc_ncm_set_remote_mac(ncm_dev, ncm_mac));

	zassert_ok(usbd_init(&test_usbd));
	zassert_equal(usbd_cdc_ecm_set_remote_mac(ecm_dev, ncm_mac), -EBUSY);
	zassert_equal(usbd_cdc_ncm_set_remote_mac(ncm_dev, ecm_mac), -EBUSY);

	zassert_ok(usbd_enable(&test_usbd));
	k_msleep(200);

	zassert_true(test_host_sees_string("02005E0053A1"), "ECM iMACAddress not found");
	zassert_true(test_host_sees_string("02005E0053B2"), "NCM iMACAddress not found");

	zassert_ok(usbd_disable(&test_usbd));
	zassert_ok(usbd_shutdown(&test_usbd));
	zassert_ok(usbd_cdc_ecm_set_remote_mac(ecm_dev, ecm_mac));
	zassert_ok(usbd_cdc_ncm_set_remote_mac(ncm_dev, ncm_mac));
}

static void *test_setup(void)
{
	zassert_ok(usbh_init(&uhs_ctx));
	zassert_ok(usbh_enable(&uhs_ctx));
	zassert_ok(uhc_bus_reset(uhs_ctx.dev));
	zassert_ok(uhc_bus_resume(uhs_ctx.dev));
	zassert_ok(uhc_sof_enable(uhs_ctx.dev));

	zassert_ok(usbd_add_descriptor(&test_usbd, &test_lang));
	if (USBD_SUPPORTS_HIGH_SPEED && usbd_caps_speed(&test_usbd) == USBD_SPEED_HS) {
		test_register_classes(USBD_SPEED_HS, &test_hs_config);
	}

	test_register_classes(USBD_SPEED_FS, &test_fs_config);

	return NULL;
}

ZTEST_SUITE(cdc_eth, NULL, test_setup, NULL, NULL, NULL);
