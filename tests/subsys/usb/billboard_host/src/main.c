/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/bos.h>
#include <zephyr/usb/class/usbh_billboard.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>

LOG_MODULE_REGISTER(test_main, LOG_LEVEL_INF);
USBH_CONTROLLER_DEFINE(test_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));
struct usbh_context *const uhs_ctx = &test_uhs_ctx;

#define URL_DESC_CSTR       "https://www.zephyrproject.org/"
#define ALTERNATE_DESC_CSTR "Example alternate mode"

#ifndef USB_LANG_ENGLISH_USA
#define USB_LANG_ENGLISH_USA (0x0409)
#endif

USBD_DEVICE_DEFINE(test_billboard_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2fe3, 0xffff);
USBD_DESC_LANG_DEFINE(lang_desc);
USBD_DESC_MANUFACTURER_DEFINE(mfr_desc, "Zephyr Billboard Test");
USBD_DESC_PRODUCT_DEFINE(product_desc, "Zephyr Billboard");
USBD_DESC_STRING_DEFINE(url_desc, URL_DESC_CSTR, USBD_DUT_STRING_INTERFACE);
USBD_DESC_STRING_DEFINE(alternate_desc, ALTERNATE_DESC_CSTR, USBD_DUT_STRING_INTERFACE);
USBD_CONFIGURATION_DEFINE(billboard_cfg, 0, 100, NULL);

static struct test_usb_billboard_capability_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDevCapabilityType;
	uint8_t iAdditionalInfoURL;
	uint8_t bNumberOfAlternateOrUSB4Modes;
	uint8_t bPreferredAlternateOrUSB4Mode;
	uint16_t VCONNPower;
	uint8_t bmConfigured[32];
	uint16_t bcdVersion;
	uint8_t bAdditionalFailureInfo;
	uint8_t bReserved;
	struct usb_billboard_aum aum[1];
} __packed test_billboard_capability = {
	.bLength = sizeof(struct test_usb_billboard_capability_descriptor),
	.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
	.bDevCapabilityType = USB_BOS_CAPABILITY_BILLBOARD,
	.iAdditionalInfoURL = 0,            /* Set later in 'billboard_host_test_enable' */
	.bNumberOfAlternateOrUSB4Modes = 1, /* Single AltMode */
	.bPreferredAlternateOrUSB4Mode = 0,
	.VCONNPower = 0,
	/* 0x2 - USB_BILLBOARD_AUM_STATE_NOT_ATTEMPTED_FAILED for AltMode 0 */
	.bmConfigured = {0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
			 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
	.bcdVersion = 0x110,
	.aum = {{
		.wSVID = 0xFF01, /* SVID code for "VESA DisplayPort Alternate Mode" */
		.bAlternateOrUSB4Mode = 0,
		.iAlternateOrUSB4ModeString = 0 /* Set later in 'billboard_host_test_enable' */
	}}};

USBD_DESC_BOS_DEFINE(test_billboard_bos_desc, sizeof(test_billboard_capability),
		     &test_billboard_capability);

static int test_usbd_billboard_init(struct usbd_class_data *const c_data)
{
	return 0;
}

struct usb_if_descriptor test_billboard_if = {
	.bLength = sizeof(struct usb_if_descriptor),
	.bDescriptorType = USB_DESC_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = USB_BCC_BILLBOARD,
	.bInterfaceSubClass = USB_BILLBOARD_SUBCLASS,
	.bInterfaceProtocol = USB_BILLBOARD_RUNTIME,
	.iInterface = 0,
};

struct test_billboard_data {
	struct usb_if_descriptor *if0;
	void *sentinel;
};

struct test_billboard_data test_billboard_data = {
	.if0 = &test_billboard_if,
	.sentinel = NULL,
};

static const void *test_usbd_billboard_get_desc(struct usbd_class_data *const c_data,
						const enum usbd_speed speed)
{
	return &test_billboard_data;
}

static const struct usbd_class_api test_usbd_billboard_api = {
	.init = test_usbd_billboard_init,
	.get_desc = test_usbd_billboard_get_desc,
};

USBD_DEFINE_CLASS(billboard_c_data, &test_usbd_billboard_api, NULL, NULL);

/* Test callback context structure */
struct test_cb_ctx {
	struct device const *dev;
	/* Billboard capability descriptor was found */
	bool found_billboard;
	/* URL index was non-zero */
	bool found_url;
	/* URL string descriptor was fetched */
	bool fetched_url;
	/* URL matched expected string */
	bool matched_url;
	/* Fetched language matches English */
	bool fetched_languages;
	/* Language changed API works */
	bool changed_language;
	/* Altmode SVID matched expected data */
	bool matched_svid;
	/* Fetched altmode string and matched against expected data */
	bool matched_altmode_string;
};

/*
 * Compares a fetched UTF-16LE string descriptor against an expected ASCII string
 */
static bool test_strdesc_matches_ascii(const struct usb_string_descriptor *str_desc,
				       const char *ascii)
{
	const uint8_t *raw = (const uint8_t *)str_desc;
	size_t ascii_len = strlen(ascii);

	if (str_desc->bLength != 2 + (ascii_len * 2)) {
		return false;
	}

	for (size_t i = 0; i < ascii_len; i++) {
		if (raw[2 + (i * 2)] != (uint8_t)ascii[i]) {
			return false;
		}
	}

	return true;
}

void cmd_billboard_cb(void *cb_arg, const struct usb_bos_capability_header *desc)
{
	struct usb_billboard_capability_descriptor *billboard_desc;
	struct usb_string_descriptor *str_desc = NULL;
	struct test_cb_ctx *test_cb_ctx = (struct test_cb_ctx *)cb_arg;
	int result;

	do {
		str_desc = k_malloc(256);
		if (str_desc == NULL) {
			break;
		}

		/* Filter only USB_BOS_CAPABILITY_BILLBOARD */
		if (desc->bDevCapabilityType != USB_BOS_CAPABILITY_BILLBOARD) {
			break;
		}
		test_cb_ctx->found_billboard = true;

		/* URL string index should not be 0 */
		billboard_desc = (struct usb_billboard_capability_descriptor *)desc;
		if (billboard_desc->iAdditionalInfoURL == 0) {
			break;
		}
		test_cb_ctx->found_url = true;

		/* Fetch URL string according to iAdditionalInfoURL string index */
		result = usbh_billboard_fetch_string_desc(
			test_cb_ctx->dev, billboard_desc->iAdditionalInfoURL, str_desc, 256);
		if (result) {
			break;
		}
		test_cb_ctx->fetched_url = true;

		/* Compare fetched UTF16LE string descriptor with expected test string */
		if (!test_strdesc_matches_ascii(str_desc, URL_DESC_CSTR)) {
			break;
		}
		test_cb_ctx->matched_url = true;

		/* NOTE: According to comment in USBD_DESC_LANG_DEFINE macro:
		 * "Default and currently only supported language ID is 0x0409 English (United
		 * States)"
		 */

		/* Retrieve supported languages in the device. Should be 0x0409 */
		result = usbh_billboard_fetch_langs_desc(test_cb_ctx->dev, str_desc, 256);
		if (result || sys_get_le16((uint8_t *)&str_desc->bString) != USB_LANG_ENGLISH_USA) {
			break;
		}
		test_cb_ctx->fetched_languages = true;

		/* Change language (to only supported 0x0409), that will change language code in
		 * furtier string descriptor requests
		 */
		result = usbh_billboard_use_lang(test_cb_ctx->dev, USB_LANG_ENGLISH_USA);
		if (result) {
			break;
		}
		test_cb_ctx->changed_language = true;

		/* Iterate over supported alt modes */
		for (int i = 0; i < billboard_desc->bNumberOfAlternateOrUSB4Modes; i++) {
			test_cb_ctx->matched_svid = (billboard_desc->aum[i].wSVID == 0xFF01);
			if (billboard_desc->aum[i].iAlternateOrUSB4ModeString == 0) {
				break;
			}
			result = usbh_billboard_fetch_string_desc(
				test_cb_ctx->dev, billboard_desc->aum[i].iAlternateOrUSB4ModeString,
				str_desc, 256);
			if (result) {
				break;
			}
			if (!test_strdesc_matches_ascii(str_desc, ALTERNATE_DESC_CSTR)) {
				break;
			}
			test_cb_ctx->matched_altmode_string = true;
		}
	} while (0);

	if (str_desc != NULL) {
		k_free(str_desc);
	}
}

ZTEST(billboard_host_test, test_billboard)
{
	struct test_cb_ctx test_cb_ctx = {0};
	const struct device *dev;
	int result;

	dev = DEVICE_DT_GET(DT_NODELABEL(any_billboard_device));
	test_cb_ctx.dev = dev;

	/* Try several times, the billboard driver may not be probed yet */
	result = -EAGAIN;
	for (int i = 0; (i < 7) && (result == -EAGAIN); i++) {
		result = usbh_billboard_parse(dev, cmd_billboard_cb, &test_cb_ctx);
		/* Device was not probed yet */
		if (result == -EAGAIN) {
			k_msleep(100);
		}
	}
	zassert_ok(result, "Failed to parse billboard capability descriptor in BOS");

	/* Found billboard capatility descriptor */
	zassert_true(test_cb_ctx.found_billboard);

	/* The descriptor has URL string index */
	zassert_true(test_cb_ctx.found_url);

	/* URL string descriptor was fetched */
	zassert_true(test_cb_ctx.fetched_url);

	/* URL string descriptor matched expected data */
	zassert_true(test_cb_ctx.matched_url);

	/* Retrieved language(s) supported by the device */
	zassert_true(test_cb_ctx.fetched_languages);

	/* Call API to change language */
	zassert_true(test_cb_ctx.changed_language);

	/* String ALT mode string descriptor was fetched */
	zassert_true(test_cb_ctx.matched_altmode_string);

	/* ALtmode SVID matched expected data */
	zassert_true(test_cb_ctx.matched_svid);

	/* ALtmode string matched expected data*/
	zassert_true(test_cb_ctx.matched_altmode_string);
}

void *billboard_host_test_enable(void)
{
	int result;

	result = usbh_init(uhs_ctx);
	zassert_ok(result, "Failed to initialize USB host");

	result = usbh_enable(uhs_ctx);
	zassert_ok(result, "Failed to enable USB host");

	result = uhc_bus_reset(uhs_ctx->dev);
	zassert_ok(result, "Failed to signal bus reset");

	result = uhc_bus_resume(uhs_ctx->dev);
	zassert_ok(result, "Failed to signal bus resume");

	result = uhc_sof_enable(uhs_ctx->dev);
	zassert_ok(result, "Failed to enable SoF generator");

	LOG_INF("Host controller enabled");

	/* Setup Billboard device */
	result = usbd_add_descriptor(&test_billboard_usbd, &lang_desc);
	zassert_ok(result);

	result = usbd_device_set_bcd_usb(&test_billboard_usbd, USBD_SPEED_FS, 0x0201);
	zassert_ok(result);

	result = usbd_add_descriptor(&test_billboard_usbd, &mfr_desc);
	zassert_ok(result);

	result = usbd_add_descriptor(&test_billboard_usbd, &product_desc);
	zassert_ok(result);

	result = usbd_add_descriptor(&test_billboard_usbd, &test_billboard_bos_desc);
	zassert_ok(result);

	result = usbd_add_descriptor(&test_billboard_usbd, &url_desc);
	zassert_ok(result);

	test_billboard_capability.iAdditionalInfoURL = usbd_str_desc_get_idx(&url_desc);

	result = usbd_add_descriptor(&test_billboard_usbd, &alternate_desc);
	zassert_ok(result);

	test_billboard_capability.aum[0].iAlternateOrUSB4ModeString =
		usbd_str_desc_get_idx(&alternate_desc);

	result = usbd_add_configuration(&test_billboard_usbd, USBD_SPEED_FS, &billboard_cfg);
	zassert_ok(result);

	result = usbd_register_class(&test_billboard_usbd, "billboard_c_data", USBD_SPEED_FS, 1);
	zassert_ok(result);

	result = usbd_device_set_code_triple(&test_billboard_usbd, USBD_SPEED_FS, USB_BCC_BILLBOARD,
					     USB_BILLBOARD_SUBCLASS, USB_BILLBOARD_RUNTIME);
	zassert_ok(result);

	result = usbd_init(&test_billboard_usbd);
	zassert_ok(result);

	result = usbd_enable(&test_billboard_usbd);
	zassert_ok(result);

	LOG_INF("Billboard device enabled");

	return NULL;
}

void billboard_host_test_shutdown(void *f)
{
	int result;

	result = usbd_disable(&test_billboard_usbd);
	zassert_ok(result, "Failed to disable device");

	result = usbd_shutdown(&test_billboard_usbd);
	zassert_ok(result, "Failed to shutdown device");

	result = usbh_disable(uhs_ctx);
	zassert_ok(result, "Failed to disable USB host");

	result = usbh_shutdown(uhs_ctx);
	zassert_ok(result, "Failed to shutdown host support");

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(billboard_host_test, NULL, billboard_host_test_enable, NULL, NULL,
	    billboard_host_test_shutdown);
