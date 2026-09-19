/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>

#include <zephyr/sys/util.h>
#include <sys/errno.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_device.h"
#include "zephyr/drivers/usb/uhc.h"
#include <zephyr/usb/class/hid.h>
#include <stdint.h>
#include <stdio.h>

#include "usbh_hid_boot.h"

#define USBH_HID_BOOT_SUBCLASS 1

#define USBH_HID_BOOT_REPORT_LEN_BYTES 8

LOG_MODULE_REGISTER(usbh_hid_boot, CONFIG_USBH_HID_BOOT_LOG_LEVEL);

#define USBH_HID_BOOT_DECLARE_DEVICES(n, _) DEVICE_DECLARE(usbh_hid_boot_##n);
LISTIFY(CONFIG_USBH_HID_BOOT_INSTANCES_COUNT, USBH_HID_BOOT_DECLARE_DEVICES, (;), _)

int hid_boot_report_key(struct hid_boot_data *const priv, uint16_t code, int32_t value)
{
	return input_report_key(priv->dev, code, value, true, K_FOREVER);
}

int hid_boot_report_rel(struct hid_boot_data *const priv, uint16_t code, int32_t value)
{
	return input_report_rel(priv->dev, code, value, true, K_FOREVER);
}

static uint16_t hid_to_input(uint8_t hid_key)
{
	if (hid_key >= ARRAY_SIZE(hid_to_input_map)) {
		return 0;
	}
	return hid_to_input_map[hid_key];
}

static int usbh_hid_boot_init(struct usbh_class_data *const c_data)
{
	LOG_INF("HID Boot Device Class initialized");
	return 0;
}

static int verify_desc(struct usb_device *const udev, const uint8_t iface,
		       const struct usb_if_descriptor *if_desc)
{
	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		LOG_INF("HID Boot Class driver called on the device and not function");
		return -ENOTSUP;
	}

	if (if_desc == NULL) {
		LOG_ERR("No descriptor found for iface : %d", iface);
		return -ENOTSUP;
	}

	if (!usbh_desc_is_valid_interface(if_desc)) {
		LOG_ERR("Not a valid interface descriptor for iface : %d", iface);
		return -ENOTSUP;
	}

	if (if_desc->bInterfaceClass != 3 || if_desc->bInterfaceSubClass != 1) {
		LOG_INF("Not a HID Boot function");
		return -ENOTSUP;
	}

	if (if_desc->bInterfaceProtocol != 1 && if_desc->bInterfaceProtocol != 2) {
		LOG_WRN("Invalid protocol in a HID Boot Device: %d", if_desc->bInterfaceProtocol);
		return -ENOTSUP;
	}

	return 0;
}

static const struct usb_ep_descriptor *find_int_in_ep(const void *desc)
{
	const struct usb_ep_descriptor *ep_desc = desc;

	while ((ep_desc = usbh_desc_get_next(ep_desc))) {
		if (ep_desc->bDescriptorType != USB_DESC_ENDPOINT) {
			continue;
		}

		if (!USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress) ||
		    ep_desc->Attributes.transfer != USB_EP_TYPE_INTERRUPT) {
			continue;
		}

		LOG_DBG("Found Endpoint for a HID Boot function: %02x", ep_desc->bEndpointAddress);
		break;
	}

	if (ep_desc == NULL) {
		LOG_WRN("Endpoint not found for a HID Boot function");
	}

	return ep_desc;
}

static bool is_kbd_phantom_cond(struct hid_kbd_report *report)
{
	ARRAY_FOR_EACH_PTR(report->pressed_keys, key) {
		if (*key != 1) {
			return false;
		}
	}
	return true;
}

static void handle_kbd_modifiers(struct hid_boot_data *priv, union hid_kbd_modifiers *modifiers)
{
	uint16_t keycodes[] = {
		INPUT_KEY_LEFTCTRL,  INPUT_KEY_LEFTSHIFT,  INPUT_KEY_LEFTALT,  INPUT_KEY_LEFTMETA,

		INPUT_KEY_RIGHTCTRL, INPUT_KEY_RIGHTSHIFT, INPUT_KEY_RIGHTALT, INPUT_KEY_RIGHTMETA,
	};

	uint8_t changed = modifiers->raw ^ priv->last_report.kbd.modifiers.raw;

	ARRAY_FOR_EACH(keycodes, i) {
		if (changed & BIT(i)) {
			hid_boot_report_key(priv, keycodes[i], modifiers->raw >> i & 1);
		}
	}

	priv->last_report.kbd.modifiers = *modifiers;
}

static bool is_kbd_key_pressed(struct hid_kbd_report *report, int8_t hid_key)
{
	ARRAY_FOR_EACH_PTR(report->pressed_keys, key) {
		if (*key == hid_key) {
			return true;
		}
	}
	return false;
}

static void handle_kbd_released(struct hid_boot_data *priv, struct hid_kbd_report *report)
{
	ARRAY_FOR_EACH_PTR(priv->last_report.kbd.pressed_keys, key) {
		if (*key == 0) {
			break;
		}
		if (!is_kbd_key_pressed(report, *key)) {
			uint16_t key_code = hid_to_input(*key);

			if (key_code == 0) {
				LOG_WRN("Invalid hid key code");
				continue;
			}
			hid_boot_report_key(priv, key_code, 0);
		}
	}
}

static void handle_kbd_pressed(struct hid_boot_data *priv, struct hid_kbd_report *report)
{
	ARRAY_FOR_EACH_PTR(report->pressed_keys, key) {
		if (*key == 0) {
			break;
		}
		if (!is_kbd_key_pressed(&priv->last_report.kbd, *key)) {
			uint16_t key_code = hid_to_input(*key);

			if (key_code == 0) {
				LOG_WRN("Invalid hid key code");
				continue;
			}
			hid_boot_report_key(priv, key_code, 1);
		}
	}
}

static void handle_kbd_keys(struct hid_boot_data *priv, struct hid_kbd_report *report)
{
	handle_kbd_released(priv, report);
	handle_kbd_pressed(priv, report);

	memcpy(priv->last_report.kbd.pressed_keys, report->pressed_keys,
	       sizeof(report->pressed_keys));
}

static void handle_kbd_report(struct hid_boot_data *priv, struct hid_kbd_report *report)
{
	__ASSERT_NO_MSG(priv->type == KEYBOARD);

	handle_kbd_modifiers(priv, &report->modifiers);

	if (is_kbd_phantom_cond(report)) {
		LOG_INF("Keyboard phantom condition occurred (too much keys pressed at once)");
	} else {
		handle_kbd_keys(priv, report);
	}
}

static void handle_mouse_report(struct hid_boot_data *priv, struct hid_mouse_report *report)
{
	__ASSERT_NO_MSG(priv->type == MOUSE);

	union hid_mouse_btns changed = {
		.raw = (report->btns.raw ^ priv->last_report.mouse_btns.raw)};

	if (changed.lmb) {
		hid_boot_report_key(priv, INPUT_BTN_LEFT, report->btns.lmb);
	}
	if (changed.rmb) {
		hid_boot_report_key(priv, INPUT_BTN_RIGHT, report->btns.rmb);
	}
	if (changed.mmb) {
		hid_boot_report_key(priv, INPUT_BTN_MIDDLE, report->btns.mmb);
	}

	if (report->dx != 0) {
		hid_boot_report_rel(priv, INPUT_REL_X, report->dx);
	}
	if (report->dy != 0) {
		hid_boot_report_rel(priv, INPUT_REL_Y, report->dy);
	}

	priv->last_report.mouse_btns = report->btns;
}

static void handle_hid_boot_report(struct hid_boot_data *priv, uint8_t *report, uint8_t len)
{
	switch (priv->type) {
	case KEYBOARD:
		if (len < sizeof(struct hid_kbd_report)) {
			LOG_ERR("HID Boot Keyboard report too short (should be %d bytes), "
				"currently %d bytes",
				sizeof(struct hid_kbd_report), len);
			return;
		}
		handle_kbd_report(priv, (struct hid_kbd_report *)report);
		break;
	case MOUSE:
		if (len < sizeof(struct hid_mouse_report)) {
			LOG_ERR("HID Boot Mouse report too short (should be %d bytes), currently "
				"%d bytes",
				sizeof(struct hid_mouse_report), len);
			return;
		}
		handle_mouse_report(priv, (struct hid_mouse_report *)report);
		break;
	}
}

/* Passing a NULL ptr or a xfer with no buffer is valid */
static void free_xfer_buff(struct uhc_transfer *const xfer)
{
	if (!xfer || !xfer->buf) {
		return;
	}
	usbh_xfer_buf_free(xfer->udev, xfer->buf);
	xfer->buf = NULL;
}

/* Passing a NULL ptr or a xfer with no buffer is valid */
static void free_xfer(struct uhc_transfer *const xfer)
{
	struct hid_boot_data *priv;

	if (!xfer) {
		return;
	}

	priv = xfer->priv;

	free_xfer_buff(xfer);
	if (priv) {
		priv->int_xfer = NULL;
	}
	usbh_xfer_free(xfer->udev, xfer);
}

static int continue_transfer(struct uhc_transfer *const xfer)
{
	struct net_buf *buf;
	size_t net_buf_len;
	int ret;

	net_buf_len = xfer->buf->len;
	free_xfer_buff(xfer);

	buf = usbh_xfer_buf_alloc(xfer->udev, net_buf_len);
	if (!buf) {
		LOG_ERR("Failed to allocate buffer");
		free_xfer(xfer);
		return -ENOMEM;
	}
	xfer->buf = buf;
	xfer->recived_data_len = 0;

	ret = usbh_xfer_enqueue(xfer->udev, xfer);
	if (ret != 0) {
		LOG_ERR("Enqueue failed: %d", ret);
		free_xfer(xfer);
		return -ENOMEM;
	}

	return 0;
}

static int hid_boot_req_cb(struct usb_device *const dev, struct uhc_transfer *const xfer)
{
	bool send_next_xfer_and_parse_report = true;

	if (xfer->err == -ECONNRESET) {
		LOG_INF("Interrupt transfer was canceled");
		send_next_xfer_and_parse_report = false;
	} else if (xfer->err) {
		LOG_ERR("Interrupt request failed, err %d", xfer->err);
		send_next_xfer_and_parse_report = false;
	}

	if (!xfer->priv) {
		/* Device was disconnected */
		send_next_xfer_and_parse_report = false;
	}

	if (send_next_xfer_and_parse_report) {
		handle_hid_boot_report(xfer->priv, xfer->buf->data, xfer->buf->len);
		continue_transfer(xfer);
	} else {
		LOG_INF("Stopping the polling of the device");
		free_xfer(xfer);
	}

	return 0;
}

static int initiate_transfers(struct hid_boot_data *priv, struct usb_device *const udev,
			      uint16_t len, uint8_t ep)
{
	struct net_buf *buf;
	int ret;

	priv->int_xfer =
		usbh_xfer_alloc(udev, ep, USBH_HID_BOOT_REPORT_LEN_BYTES, hid_boot_req_cb, priv);
	if (!priv->int_xfer) {
		LOG_ERR("Failed to allocate transfer");
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(udev, len);
	if (!buf) {
		LOG_ERR("Failed to allocate buffer");
		free_xfer(priv->int_xfer);
		return -ENOMEM;
	}
	priv->int_xfer->buf = buf;

	ret = usbh_xfer_enqueue(udev, priv->int_xfer);
	if (ret != 0) {
		LOG_ERR("Enqueue failed: %d", ret);
		free_xfer(priv->int_xfer);
		return ret;
	}

	return 0;
}

static int hid_boot_class_init(struct hid_boot_data *priv, struct usb_device *const udev,
			       uint8_t if_idx, uint8_t bInterfaceProtocol)
{
	if (priv == NULL) {
		LOG_ERR("Class private data should be statically allocated");
		return -EINVAL;
	}

	*priv = (struct hid_boot_data){.dev = priv->dev};

	priv->type = bInterfaceProtocol == HID_BOOT_IFACE_CODE_KEYBOARD ? KEYBOARD : MOUSE;
	priv->if_idx = if_idx;
	priv->udev = udev;

	return 0;
}

/* 7.2.6 of HID1.11 Specification */
static int hid_set_boot_protocol(struct usb_device *const udev, uint16_t if_idx)
{
	const struct usb_req_type_field bmRequestType = {.direction = USB_REQTYPE_DIR_TO_DEVICE,
							 .type = USB_REQTYPE_TYPE_CLASS,
							 .recipient =
								 USB_REQTYPE_RECIPIENT_INTERFACE};
	LOG_DBG("Sending set boot protocol request to the device.");
	return usbh_req_setup(udev, *(uint8_t *)&bmRequestType, USB_HID_SET_PROTOCOL,
			      HID_PROTOCOL_BOOT, if_idx, 0, NULL);
}

/* 7.2.4 of HID1.11 Specification */
static int hid_set_idle(struct usb_device *const udev, uint16_t if_idx)
{
	const struct usb_req_type_field bmRequestType = {.direction = USB_REQTYPE_DIR_TO_DEVICE,
							 .type = USB_REQTYPE_TYPE_CLASS,
							 .recipient =
								 USB_REQTYPE_RECIPIENT_INTERFACE};
	LOG_DBG("Sending set idle request to the device.");
	return usbh_req_setup(udev, *(uint8_t *)&bmRequestType, USB_HID_SET_IDLE, 0, if_idx, 0,
			      NULL);
}

static int usbh_hid_boot_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
			       const uint8_t iface)
{
	const struct usb_if_descriptor *if_desc = NULL;
	const struct usb_ep_descriptor *ep_desc = NULL;
	struct hid_boot_data *priv = c_data->priv;
	int ret = 0;
	uint16_t max_packet_size;
	uint8_t ep_addr;

	LOG_DBG("Connected a HID Boot device");

	if_desc = usbh_desc_get_iface(udev, iface);
	ret = verify_desc(udev, iface, if_desc);
	if (ret) {
		return ret;
	}

	ep_desc = find_int_in_ep(if_desc);
	if (ep_desc == NULL) {
		return -ENOTSUP;
	}
	ep_addr = ep_desc->bEndpointAddress;
	max_packet_size = ep_desc->wMaxPacketSize;

	ret = hid_boot_class_init(priv, udev, iface, if_desc->bInterfaceProtocol);
	if (ret) {
		return ret;
	}

	if (if_desc->bInterfaceProtocol == HID_BOOT_IFACE_CODE_KEYBOARD) {
		LOG_INF("HID Boot Keyboard detected");
	} else if (if_desc->bInterfaceProtocol == HID_BOOT_IFACE_CODE_MOUSE) {
		LOG_INF("HID Boot Mouse detected");
	}

	ret = hid_set_boot_protocol(udev, iface);
	if (ret) {
		LOG_ERR("Failed to set boot protocol");
		return ret;
	}

	if (hid_set_idle(udev, iface)) {
		LOG_WRN("Set Idle request failed. Continuing without idle configuration");
	}

	if (initiate_transfers(priv, udev, max_packet_size, ep_addr)) {
		return -ENOTSUP;
	}

	return 0;
}

int usbh_hid_boot_resumed(struct usbh_class_data *const c_data)
{
	struct hid_boot_data *priv = c_data->priv;

	hid_set_idle(priv->udev, priv->if_idx);

	return 0;
};

static int usbh_hid_boot_removed(struct usbh_class_data *c_data)
{
	struct hid_boot_data *priv = c_data->priv;

	LOG_INF("HID Boot device disconnected");

	if (priv->int_xfer) {
		usbh_xfer_dequeue(priv->udev, priv->int_xfer);
	}

	return 0;
}

static struct usbh_class_api usbh_hid_boot_class_api = {
	.init = usbh_hid_boot_init,
	.probe = usbh_hid_boot_probe,
	.removed = usbh_hid_boot_removed,
	.completion_cb = NULL,
	.suspended = NULL,
	.resumed = NULL,
};

static struct usbh_class_filter usbh_hid_boot_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HID,
		.sub = USBH_HID_BOOT_SUBCLASS,
		.proto = HID_BOOT_IFACE_CODE_KEYBOARD,
	},
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HID,
		.sub = USBH_HID_BOOT_SUBCLASS,
		.proto = HID_BOOT_IFACE_CODE_MOUSE,
	},
	{0},
};

#define USBH_HID_BOOT_CLASS_DEFINE(n, _)                                                           \
	static struct hid_boot_data hid_boot_data_##n = {.dev = DEVICE_GET(usbh_hid_boot_##n)};    \
                                                                                                   \
	DEVICE_DEFINE(usbh_hid_boot_##n, "usbh_hid_boot_" #n, NULL, NULL, NULL, NULL,              \
		      PRE_KERNEL_1, CONFIG_USBH_HID_BOOT_INIT_PRIORITY, NULL);                     \
	USBH_DEFINE_CLASS(usbh_hid_boot_##n, &usbh_hid_boot_class_api, &hid_boot_data_##n,         \
			  usbh_hid_boot_filters);

LISTIFY(CONFIG_USBH_HID_BOOT_INSTANCES_COUNT, USBH_HID_BOOT_CLASS_DEFINE, (;), _)
