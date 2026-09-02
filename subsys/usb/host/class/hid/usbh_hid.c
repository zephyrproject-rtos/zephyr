/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_usbh_hid_device

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/input/input.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/usb/class/usbh_hid.h>

#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_ch9.h"
#include "usbh_device.h"

LOG_MODULE_REGISTER(usbh_hid, CONFIG_USBH_HID_LOG_LEVEL);

#define KEYBOARD_BOOT_PROTOCOL_MODIFIER_INDEX 0u
#define KEYBOARD_BOOT_PROTOCOL_KEY_INDEX      2u
#define KEYBOARD_BOOT_PROTOCOL_KEYS           6u
#define KEYBOARD_BOOT_PROTOCOL_REPORT_SIZE    8u
#define KEYBOARD_BOOT_PROTOCOL_MODIFIERS      8u
#define MOUSE_BOOT_PROTOCOL_BUTTONS_INDEX     0u
#define MOUSE_BOOT_PROTOCOL_X_INDEX           1u
#define MOUSE_BOOT_PROTOCOL_Y_INDEX           2u
#define MOUSE_BOOT_PROTOCOL_REPORT_SIZE       3u
#define MOUSE_BOOT_PROTOCOL_BUTTONS           3u

struct usbh_hid_config {
	/* Size of the idle rates array */
	size_t idle_rates_ms_size;
	/* Idle period in milliseconds */
	uint16_t idle_rates_ms[CONFIG_USBH_HID_REPORT_MAX_VARIANTS * 2];
	/* Whether this device starts with boot protocol */
	bool start_with_boot_protocol;
};

struct usbh_hid_data {
	/* USB host class device */
	const struct device *dev;
	/* Connected usb device */
	struct usb_device *udev;
	/* Mutual exclusion lock */
	struct k_mutex lock;
	/* Sync semaphore for waiting on asynchronous operations */
	struct k_sem sync;
	/* Interrupt IN endpoint address */
	const struct usb_ep_descriptor *input_ep;
	/* Interrupt OUT endpoint address */
	const struct usb_ep_descriptor *output_ep;
	/* Interface index */
	uint8_t target_iface;
	/* Report descriptor */
	struct usbh_hid_report report;
	/* Interrupt report request */
	struct uhc_transfer *interrupt_in_xfer;
	/* Whether the interrupt report request is in progress */
	bool input_interrupt_report_on;
	/* Whether the currently configured protocol is the simplified boot protocol */
	bool boot_protocol;
	/* Whether the input endpoint is currently stalled */
	bool stalled_input_endpoint;
	/* Work item to clear a stalled input endpoint and resume input requests */
	struct k_work stall_recovery_work;
#if CONFIG_USBH_HID_ROUTE_TO_INPUT
	/* Previous input reports */
	struct {
		uint8_t id;
		uint8_t data[CONFIG_USBH_HID_MAX_INPUT_REPORT_SIZE];
	} previous_reports[CONFIG_USBH_HID_REPORT_MAX_VARIANTS];
#endif
	/* Callback to be invoked with input data */
	usbh_hid_report_cb_t input_cb;
	/* User data */
	void *user_data;
};

static int req_interrupt_input(const struct device *dev);
static void stall_recovery_work_handler(struct k_work *work);
static int driver_stop_input_reports(const struct device *dev);
static int driver_set_idle_rate(const struct device *dev, uint8_t report_id,
				uint16_t idle_period_ms);
static int driver_set_protocol(const struct device *dev, uint8_t protocol_code);
static int usbh_class_remove(struct usbh_class_data *const c_data);

/*
 * Dedicated work queue for deferred, off-USB-host-thread work (currently just stalled input
 * endpoint recovery, see `stall_recovery_work_handler`). Kept separate from the system work
 * queue so that HID recovery work never has to wait behind unrelated system work items, and
 * vice versa.
 */
static struct k_work_q usbh_hid_workq;
static K_KERNEL_STACK_DEFINE(usbh_hid_workq_stack, CONFIG_USBH_HID_WORKQUEUE_STACK_SIZE);

static int usbh_hid_workq_init(void)
{
	const struct k_work_queue_config cfg = {
		.name = "usbh_hid",
	};

	k_work_queue_init(&usbh_hid_workq);
	k_work_queue_start(&usbh_hid_workq, usbh_hid_workq_stack,
			   K_KERNEL_STACK_SIZEOF(usbh_hid_workq_stack),
			   K_PRIO_COOP(CONFIG_USBH_HID_WORKQUEUE_PRIORITY), &cfg);

	return 0;
}

SYS_INIT(usbh_hid_workq_init, POST_KERNEL, CONFIG_USBH_HID_INIT_PRIORITY);

#if CONFIG_USBH_HID_ROUTE_TO_INPUT
/*
 * Check whether the connected device is a keyboard
 */
static inline bool is_keyboard(struct usbh_hid_data *const dev_data)
{
	const struct usb_if_descriptor *interface =
		usbh_desc_get_iface(dev_data->udev, dev_data->target_iface);
	return interface->bInterfaceProtocol == HID_BOOT_IFACE_CODE_KEYBOARD;
}

/*
 * Check whether the connected device is a mouse
 */
static inline bool is_mouse(struct usbh_hid_data *const dev_data)
{
	const struct usb_if_descriptor *interface =
		usbh_desc_get_iface(dev_data->udev, dev_data->target_iface);
	return interface->bInterfaceProtocol == HID_BOOT_IFACE_CODE_MOUSE;
}
/**
 * Conversion map between the HID key usage IDs and Zephyr's input keys
 */
static const uint16_t hid_key_to_input_map[] = {
	INPUT_KEY_A,          INPUT_KEY_B,          INPUT_KEY_C,         INPUT_KEY_D,
	INPUT_KEY_E,          INPUT_KEY_F,          INPUT_KEY_G,         INPUT_KEY_H,
	INPUT_KEY_I,          INPUT_KEY_J,          INPUT_KEY_K,         INPUT_KEY_L,
	INPUT_KEY_M,          INPUT_KEY_N,          INPUT_KEY_O,         INPUT_KEY_P,
	INPUT_KEY_Q,          INPUT_KEY_R,          INPUT_KEY_S,         INPUT_KEY_T,
	INPUT_KEY_U,          INPUT_KEY_V,          INPUT_KEY_W,         INPUT_KEY_X,
	INPUT_KEY_Y,          INPUT_KEY_Z,          INPUT_KEY_1,         INPUT_KEY_2,
	INPUT_KEY_3,          INPUT_KEY_4,          INPUT_KEY_5,         INPUT_KEY_6,
	INPUT_KEY_7,          INPUT_KEY_8,          INPUT_KEY_9,         INPUT_KEY_0,
	INPUT_KEY_ENTER,      INPUT_KEY_ESC,        INPUT_KEY_BACKSPACE, INPUT_KEY_TAB,
	INPUT_KEY_SPACE,      INPUT_KEY_MINUS,      INPUT_KEY_EQUAL,     INPUT_KEY_RESERVED,
	INPUT_KEY_RESERVED,   INPUT_KEY_BACKSLASH,  INPUT_KEY_RESERVED,  INPUT_KEY_SEMICOLON,
	INPUT_KEY_APOSTROPHE, INPUT_KEY_GRAVE,      INPUT_KEY_COMMA,     INPUT_KEY_DOT,
	INPUT_KEY_SLASH,      INPUT_KEY_CAPSLOCK,   INPUT_KEY_F1,        INPUT_KEY_F2,
	INPUT_KEY_F3,         INPUT_KEY_F4,         INPUT_KEY_F5,        INPUT_KEY_F6,
	INPUT_KEY_F7,         INPUT_KEY_F8,         INPUT_KEY_F9,        INPUT_KEY_F10,
	INPUT_KEY_F11,        INPUT_KEY_F12,        INPUT_KEY_PRINT,     INPUT_KEY_SCROLLLOCK,
	INPUT_KEY_PAUSE,      INPUT_KEY_INSERT,     INPUT_KEY_HOME,      INPUT_KEY_PAGEUP,
	INPUT_KEY_DELETE,     INPUT_KEY_END,        INPUT_KEY_PAGEDOWN,  INPUT_KEY_RIGHT,
	INPUT_KEY_LEFT,       INPUT_KEY_DOWN,       INPUT_KEY_UP,        INPUT_KEY_NUMLOCK,
	INPUT_KEY_KPSLASH,    INPUT_KEY_KPASTERISK, INPUT_KEY_KPMINUS,   INPUT_KEY_KPPLUS,
	INPUT_KEY_KPENTER,    INPUT_KEY_KP1,        INPUT_KEY_KP2,       INPUT_KEY_KP3,
	INPUT_KEY_KP4,        INPUT_KEY_KP5,        INPUT_KEY_KP6,       INPUT_KEY_KP7,
	INPUT_KEY_KP8,        INPUT_KEY_KP9,        INPUT_KEY_KP0,       INPUT_KEY_KPDOT,
};

/**
 * Conversion map between the HID modifier key usage IDs and Zephyr's input keys
 */
static const uint16_t hid_modifier_to_input_map[] = {
	INPUT_KEY_LEFTCTRL,  INPUT_KEY_LEFTSHIFT,  INPUT_KEY_LEFTALT,  INPUT_KEY_LEFTMETA,
	INPUT_KEY_RIGHTCTRL, INPUT_KEY_RIGHTSHIFT, INPUT_KEY_RIGHTALT, INPUT_KEY_RIGHTMETA,
};

/**
 * Conversion map between the HID button usage IDs and Zephyr's input buttons
 */
static const uint16_t hid_button_to_input_map[] = {
	INPUT_BTN_0, INPUT_BTN_1, INPUT_BTN_2, INPUT_BTN_3, INPUT_BTN_4,
	INPUT_BTN_5, INPUT_BTN_6, INPUT_BTN_7, INPUT_BTN_8, INPUT_BTN_9,
};

/*
 * Forward an HID key or button press to the input subsystem
 */
static void report_event(const struct device *dev, size_t map_length,
			 uint16_t const map[map_length], uint16_t map_base, uint8_t usage_id,
			 bool value, bool sync)
{
	size_t input_index = 0;

	if (usage_id < map_base) {
		/* Out of range */
		return;
	}

	input_index = usage_id - map_base;
	if (input_index >= map_length) {
		/* Out of range */
		return;
	}

	if (map[input_index] == INPUT_KEY_RESERVED) {
		/* To be ignored */
		return;
	}

	input_report_key(dev, map[input_index], value, sync, K_FOREVER);
}

/*
 * Forward an HID key press to the input subsystem
 */
static void report_key(const struct device *dev, uint8_t usage_id, bool value, bool sync)
{
	report_event(dev, ARRAY_SIZE(hid_key_to_input_map), hid_key_to_input_map, HID_KEY_A,
		     usage_id, value, sync);
}

/*
 * Forward an HID modifier key press to the input subsystem
 */
static void report_modifier_key(const struct device *dev, uint8_t usage_id, bool value, bool sync)
{
	report_event(dev, ARRAY_SIZE(hid_modifier_to_input_map), hid_modifier_to_input_map,
		     HID_USAGE_GEN_DESKTOP_KEYBOARD_LEFT_CTRL, usage_id, value, sync);
}

/*
 * Forward an HID button press to the input subsystem
 */
static void report_button(const struct device *dev, uint8_t usage_id, bool value, bool sync)
{
	report_event(dev, ARRAY_SIZE(hid_button_to_input_map), hid_button_to_input_map, HID_BTN_1,
		     usage_id, value, sync);
}

/*
 * Get a pointer to the buffer where the previous report is stored
 * returns -ENOMEM if there are no buffers avaialble for the provided report ID
 */
static int get_previous_report_buffer(const struct usbh_hid_data *dev_data, uint8_t report_id)
{
	int ret = -ENOMEM;
	const struct usbh_hid_report *report = &dev_data->report;

	/* Check if the current report actually has multiple variants */
	if (report->num_reports > 1u) {
		for (size_t report_index = 0u; report_index < report->num_reports; report_index++) {
			if (dev_data->previous_reports[report_index].id == report_id ||
			    dev_data->previous_reports[report_index].id == 0) {
				ret = report_index;
				break;
			}
		}
	} else {
		ret = 0;
	}

	return ret;
}

/*
 * Checks whether a key was previously pressed and now not anymore
 */
static bool was_array_key_released(const uint8_t *prev_value_ptr, const uint8_t *value_ptr,
				   size_t prev_key_index, size_t count)
{
	bool released = false;

	/* The key was previously pressed, so now it may be released */
	if (prev_value_ptr[prev_key_index] != 0u) {
		bool found = false;

		for (size_t key_index = 0u; key_index < count; key_index++) {
			/* Key is still pressed */
			if (value_ptr[key_index] == prev_value_ptr[prev_key_index]) {
				found = true;
				break;
			}
		}

		/* If not found, it is released */
		released = !found;
	}

	return released;
}

/*
 * Report keys from an array report field
 */
static void report_array_keys(const struct device *dev, const uint8_t *prev_value_ptr,
			      const uint8_t *value_ptr, size_t bit_shift, size_t count)
{

	/* Active keys */
	for (size_t key_index = 0u; key_index < count; key_index++) {
		if (value_ptr[key_index] != 0u) {
			report_key(dev, value_ptr[key_index], true, false);
		}
	}

	/* Inactive keys */
	for (size_t prev_key_index = 0u; prev_key_index < count; prev_key_index++) {
		if (was_array_key_released(prev_value_ptr, value_ptr, prev_key_index, count)) {
			report_key(dev, prev_value_ptr[prev_key_index], false, false);
		}
	}
}

/*
 * Report a key from a variable report field
 */
static void report_variable_key(const struct device *dev, const uint8_t *prev_value_ptr,
				const uint8_t *value_ptr, size_t key_position, uint16_t usage_id)
{
	/* Pinpoint the exact bit */
	size_t key_index = key_position / 8u;
	size_t key_shift = key_position % 8u;

	/* Active key */
	if ((value_ptr[key_index] & (1u << key_shift)) > 0u) {
		report_modifier_key(dev, usage_id, true, false);
	}
	/* Inactive key */
	else if ((prev_value_ptr[key_index] & (1u << key_shift)) > 0u) {
		report_modifier_key(dev, usage_id, false, false);
	}
}

/*
 * Report a button from a variable report field
 */
static void report_variable_button(const struct device *dev, const uint8_t *prev_value_ptr,
				   const uint8_t *value_ptr, size_t button_position,
				   uint16_t usage_id)
{
	/* Pinpoint the exact bit */
	size_t button_index = button_position / 8u;
	size_t button_shift = button_position % 8u;

	/* Active buttons */
	if ((value_ptr[button_index] & (1u << button_shift)) > 0u) {
		report_button(dev, usage_id, true, false);
	}
	/* Inactive buttons */
	else if ((prev_value_ptr[button_index] & (1u << button_shift)) > 0u) {
		report_button(dev, usage_id, false, false);
	}
}

/*
 * Report events from a report field
 */
static int report_events(const struct usbh_hid_report_field *field, uint8_t report_id,
			 size_t data_length, const uint8_t *data, size_t bit_index, void *user_data)
{
	size_t value_index = bit_index / 8;
	size_t value_bit_shift = bit_index % 8;
	int ret = 0;
	struct usbh_hid_data *const dev_data = user_data;
	const struct device *dev = dev_data->dev;
	const uint8_t *value_ptr = &data[value_index];
	size_t remaining_length = data_length - value_index;
	const uint8_t *prev_value_ptr = NULL;
	int32_t signed_value = 0u;

	ret = get_previous_report_buffer(dev_data, report_id);
	if (ret < 0) {
		LOG_WRN("Unable to get previous buffer for report ID 0x%02X", report_id);
		return ret;
	}

	prev_value_ptr = &dev_data->previous_reports[ret].data[value_index];

	/* Keyboard input */
	if (usbh_hid_report_match_usage_page(field, HID_USAGE_GEN_KEYBOARD)) {
		/* Keyboard array, each element is a button press */
		if (USBH_HID_REPORT_DATA_IS_ARRAY(field->flags) && field->size == 8u) {
			report_array_keys(dev, prev_value_ptr, value_ptr, value_bit_shift,
					  field->count);
		}
		/* Keyboard variable, mostly for modifiers */
		else if (USBH_HID_REPORT_DATA_IS_VARIABLE(field->flags) && field->size == 1u) {
			for (size_t key_position = 0u; key_position < field->count;
			     key_position++) {
				report_variable_key(dev, prev_value_ptr, value_ptr,
						    value_bit_shift + key_position,
						    usbh_hid_report_field_get_usage_id_by_index(
							    field, key_position));
			}
		}
	}
	/* Mouse buttons */
	else if (usbh_hid_report_match_usage_page(field, HID_USAGE_GEN_BUTTON)) {
		if (USBH_HID_REPORT_DATA_IS_VARIABLE(field->flags) && field->size == 1u) {
			for (size_t button_position = 0u; button_position < field->count;
			     button_position++) {
				report_variable_button(dev, prev_value_ptr, value_ptr,
						       value_bit_shift + button_position,
						       usbh_hid_report_field_get_usage_id_by_index(
							       field, button_position));
			}
		}
	} else {
		/* Mouse relative X movement */
		if (USBH_HID_REPORT_DATA_IS_RELATIVE(field->flags) &&
		    usbh_hid_report_get_usage_id_i32(
			    field, remaining_length, value_ptr, value_bit_shift,
			    (HID_USAGE_GEN_DESKTOP << 16u) | HID_USAGE_GEN_DESKTOP_X,
			    &signed_value) == 0) {
			if (signed_value != 0u) {
				input_report_rel(dev, INPUT_REL_X, signed_value, false, K_FOREVER);
			}
		}
		/* Mouse relative Y movement */
		if (USBH_HID_REPORT_DATA_IS_RELATIVE(field->flags) &&
		    usbh_hid_report_get_usage_id_i32(
			    field, remaining_length, value_ptr, value_bit_shift,
			    (HID_USAGE_GEN_DESKTOP << 16) | HID_USAGE_GEN_DESKTOP_Y,
			    &signed_value) == 0) {
			if (signed_value != 0u) {
				input_report_rel(dev, INPUT_REL_Y, signed_value, false, K_FOREVER);
			}
		}
		/* Mouse relative wheel movement */
		if (usbh_hid_report_get_usage_id_i32(
			    field, remaining_length, value_ptr, value_bit_shift,
			    (HID_USAGE_GEN_DESKTOP << 16u) | HID_USAGE_GEN_DESKTOP_WHEEL,
			    &signed_value) == 0) {
			if (signed_value != 0u) {
				input_report_rel(dev, INPUT_REL_WHEEL, signed_value, false,
						 K_FOREVER);
			}
		}
	}

	return 0;
}
#endif

/*
 * Manage an input report
 *
 */
static int handle_input_report(const struct usbh_hid_config *dev_config,
			       struct usbh_hid_data *dev_data, size_t data_length,
			       uint8_t const data[data_length])
{
#if CONFIG_USBH_HID_ROUTE_TO_INPUT
	int input_size = 0;
	size_t previous_report_index = 0u;
	uint8_t report_id = 0u;
	const struct device *dev = dev_data->dev;
	int ret = 0;
#endif

	LOG_HEXDUMP_DBG(data, data_length, "RX  : ");

#if CONFIG_USBH_HID_ROUTE_TO_INPUT
	if (dev_data->boot_protocol) {
		report_id = 0;

		/* Mouse boot protocol */
		if (is_mouse(dev_data)) {
			int8_t x = 0;
			int8_t y = 0;

			/* Not enough data */
			if (data_length < MOUSE_BOOT_PROTOCOL_REPORT_SIZE) {
				return -EIO;
			}

			x = data[MOUSE_BOOT_PROTOCOL_X_INDEX];
			y = data[MOUSE_BOOT_PROTOCOL_Y_INDEX];
			input_size = MOUSE_BOOT_PROTOCOL_REPORT_SIZE;

			/* Keyboard variable, mostly for modifiers */
			for (size_t key_position = 0u; key_position < MOUSE_BOOT_PROTOCOL_BUTTONS;
			     key_position++) {
				report_variable_button(
					dev,
					&dev_data->previous_reports[0u]
						 .data[MOUSE_BOOT_PROTOCOL_BUTTONS_INDEX],
					&data[MOUSE_BOOT_PROTOCOL_BUTTONS_INDEX], key_position,
					HID_BTN_1 + key_position);
			}

			if (x != 0) {
				input_report_rel(dev, INPUT_REL_X, x, false, K_FOREVER);
			}
			if (y != 0) {
				input_report_rel(dev, INPUT_REL_Y, y, true, K_FOREVER);
			}
		}
		/* Keyboard boot protocol */
		else if (is_keyboard(dev_data)) {
			/* Not enough data */
			if (data_length < KEYBOARD_BOOT_PROTOCOL_REPORT_SIZE) {
				return -EIO;
			}

			input_size = KEYBOARD_BOOT_PROTOCOL_REPORT_SIZE;
			/* Keyboard array, each element is a button press */
			report_array_keys(dev,
					  &dev_data->previous_reports[0u]
						   .data[KEYBOARD_BOOT_PROTOCOL_KEY_INDEX],
					  &data[KEYBOARD_BOOT_PROTOCOL_KEY_INDEX], 0u,
					  KEYBOARD_BOOT_PROTOCOL_KEYS);

			/* Keyboard variable, mostly for modifiers */
			for (size_t key_position = 0u;
			     key_position < KEYBOARD_BOOT_PROTOCOL_MODIFIERS; key_position++) {
				report_variable_key(
					dev,
					&dev_data->previous_reports[0u]
						 .data[KEYBOARD_BOOT_PROTOCOL_MODIFIER_INDEX],
					&data[KEYBOARD_BOOT_PROTOCOL_MODIFIER_INDEX], key_position,
					HID_USAGE_GEN_DESKTOP_KEYBOARD_LEFT_CTRL + key_position);
			}
		}
	}
	/* Report dynamic parsing and handling */
	else {
		/* If multiple reports are specified pick the first byte of the data as the ID */
		if (dev_data->report.num_reports > 1u) {
			report_id = data[0u];
		}

		ret = get_previous_report_buffer(dev_data, report_id);
		if (ret < 0) {
			LOG_WRN("Unable to get previous buffer for report ID 0x%02X", report_id);
			return ret;
		}
		previous_report_index = ret;

		input_size = usbh_hid_report_get_input_size(&dev_data->report, report_id);

		if (input_size < 0) {
			LOG_ERR("Unable to get input size: %i", input_size);
			return input_size;
		}

		if (input_size == data_length) {
			usbh_hid_report_input_iterate(&dev_data->report, data_length, data,
						      report_events, dev_data);
		} else {
			LOG_WRN("Length mismatch between report descriptor "
				"and actual report: %i vs %i",
				input_size, data_length);
		}
	}

	/* Update the previous report buffer */
	memcpy(&dev_data->previous_reports[previous_report_index].data, data, input_size);
	dev_data->previous_reports[previous_report_index].id = report_id;
#endif

	/* If specified invoke the user provided callback */
	if (dev_data->input_cb) {
		usbh_hid_report_input_iterate(&dev_data->report, data_length, data,
					      dev_data->input_cb, dev_data->user_data);
	}

	return 0;
}

/*
 * Callback on completion of input interrupt requests
 */
static int input_interrupt_transfer_cb(struct usb_device *const udev,
				       struct uhc_transfer *const xfer)
{
	struct device *const dev = xfer->priv;
	struct usbh_hid_data *dev_data = dev->data;
	const struct usbh_hid_config *dev_config = dev->config;
	bool is_current = false;
	int ret = 0;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	is_current = dev_data->interrupt_in_xfer == xfer;

	/* Clear the saved xfer reference, if it's the one currently being stored */
	if (is_current) {
		dev_data->interrupt_in_xfer = NULL;
	}

	if (xfer->err == 0) {
		/* The net_buf contains the received data */
		struct net_buf *buf = xfer->buf;

		if (USB_EP_DIR_IS_IN(xfer->ep)) {
			if (buf && buf->len > 0u) {
				ret = handle_input_report(dev_config, dev_data, buf->len,
							  buf->data);
				if (ret != 0) {
					LOG_WRN("Error handling input: %i", ret);
				}
			}
		}
	}
	/* Endpoint halt condition to be cleared */
	else if (xfer->err == -EPIPE) {
		LOG_DBG("Endpoint in stalled, deferring clear and retry");
		dev_data->stalled_input_endpoint = true;

		ret = k_work_submit_to_queue(&usbh_hid_workq, &dev_data->stall_recovery_work);
		if (ret < 0) {
			LOG_ERR("Unable to submit endpoint stall recovery to workq: %i", ret);
		}

		ret = -EPIPE;
	} else if (xfer->err == -ECONNRESET) {
		/* Request was cancelled, nothing to do */
	} else {
		/* Continue with the error */
		LOG_WRN("IN endpoint request failed: %i", xfer->err);
		ret = xfer->err;
	}

	if (ret == 0 && dev_data->input_interrupt_report_on && is_current) {
		ret = req_interrupt_input(dev);
	} else if (ret == -EPIPE) {
		LOG_INF("Input endpoint stalled, recovery scheduled");
	} else {
		LOG_INF("Stopping input requests");
	}

	/* Finally free the request */
	if (xfer->buf != NULL) {
		usbh_xfer_buf_free(dev_data->udev, xfer->buf);
	}
	usbh_xfer_free(dev_data->udev, xfer);

	k_mutex_unlock(&dev_data->lock);

	return ret;
}

/*
 * Clear a stalled input endpoint and resume input requests, run deferred on the driver's own
 * work queue (see the comment on `stall_recovery_work` for why)
 */
static void stall_recovery_work_handler(struct k_work *work)
{
	struct usbh_hid_data *dev_data =
		CONTAINER_OF(work, struct usbh_hid_data, stall_recovery_work);
	const struct device *dev = dev_data->dev;
	struct usb_device *udev;
	uint8_t ep_addr;
	int ret;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	/* Device disconnected, or already recovered/stopped in the meantime */
	if (dev_data->udev == NULL || !dev_data->stalled_input_endpoint) {
		k_mutex_unlock(&dev_data->lock);
		return;
	}

	/* Snapshot what's needed and release the lock before the blocking control request
	 * below, so a concurrent disconnect (usbh_class_remove(), which also needs this lock)
	 * doesn't have to wait behind it.
	 */
	udev = dev_data->udev;
	ep_addr = dev_data->input_ep->bEndpointAddress;

	k_mutex_unlock(&dev_data->lock);

	ret = usbh_req_clear_sfs_halt(udev, ep_addr);

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	/* The device was removed while unlocked or the input endpoint was otherwise unstalled. No
	 * further operation is necessary
	 */
	if (dev_data->udev != udev || !dev_data->stalled_input_endpoint) {
		k_mutex_unlock(&dev_data->lock);
		return;
	}

	if (ret != 0) {
		LOG_ERR("Could not clear stalled input endpoint: %i", ret);
		k_mutex_unlock(&dev_data->lock);
		return;
	}

	dev_data->stalled_input_endpoint = false;

	if (dev_data->input_interrupt_report_on) {
		ret = req_interrupt_input(dev);
		if (ret != -EALREADY && ret != 0) {
			LOG_ERR("Could not resume input requests after stall recovery: %i", ret);
		}
	}

	k_mutex_unlock(&dev_data->lock);
}

/*
 * Enqueue an interrupt input request, kickstarting a periodic update
 */
static int req_interrupt_input(const struct device *dev)
{
	struct uhc_transfer *xfer = NULL;
	struct usbh_hid_data *dev_data = (void *)dev->data;
	int ret = 0;

	if (dev_data->interrupt_in_xfer != NULL) {
		return -EALREADY;
	}

	/* Allocate with enough buffer for one interrupt packet */
	xfer = usbh_xfer_alloc_with_buf(dev_data->udev, dev_data->input_ep->bEndpointAddress,
					input_interrupt_transfer_cb, (void *)dev,
					dev_data->input_ep->wMaxPacketSize);
	if (xfer == NULL) {
		LOG_WRN("Unable to allocate memory buffer");
		return -ENOMEM;
	}

	/* Enqueue the transfer */
	ret = usbh_xfer_enqueue(dev_data->udev, xfer);
	if (ret != 0) {
		usbh_xfer_free(dev_data->udev, xfer);
		LOG_WRN("Unable to enqueue interrupt input request: %i", ret);
		return ret;
	}

	dev_data->interrupt_in_xfer = xfer;

	return 0;
}

/*
 * Enqueue a request for an interface descriptor
 */
static int req_iface_desc(struct usb_device *const udev, uint8_t const type, uint8_t const index,
			  uint16_t const id, uint16_t const len, struct net_buf *const buf)
{
	const uint8_t bmRequestType =
		(USB_REQTYPE_DIR_TO_HOST << 7u) | (USB_REQTYPE_RECIPIENT_INTERFACE << 0u);
	const uint8_t bRequest = USB_SREQ_GET_DESCRIPTOR;
	const uint16_t wValue = (type << 8u) | index;

	return usbh_req_setup(udev, bmRequestType, bRequest, wValue, id, len, buf);
}

/*
 * Enqueue a HID class request
 */
static int hid_class_request(struct usb_device *const udev, uint8_t const iface,
			     uint8_t const direction, uint8_t const request, uint16_t const value,
			     size_t data_length, struct net_buf *buf)
{
	const uint8_t bmRequestType = (direction << 7u) | (USB_REQTYPE_TYPE_CLASS << 5u) |
				      (USB_REQTYPE_RECIPIENT_INTERFACE << 0u);

	return usbh_req_setup(udev, bmRequestType, request, value, iface, data_length, buf);
}

/*
 * Analyze the various descriptors of the USB device interface
 */
static int scan_descriptors(const struct usbh_hid_config *dev_config,
			    struct usbh_hid_data *const dev_data, uint8_t const iface)
{
	int ret = 0;
	struct net_buf *buf = NULL;
	const struct usb_desc_header *dhp = NULL;
	uint16_t report_descriptor_length = 0u;

	/*
	 * Fetch the size of the HID descriptor first by traversing the descriptors from the
	 * selected interface
	 */
	dhp = (const struct usb_desc_header *)usbh_desc_get_iface(dev_data->udev, iface);
	if (dhp == NULL) {
		LOG_ERR("Failed to find interface %u", iface);
		return -ENOSYS;
	}
	dhp = usbh_desc_get_next(dhp);

	while (dhp != NULL && dhp->bDescriptorType != USB_DESC_INTERFACE) {
		/* Report descriptor */
		if (dhp->bDescriptorType == USB_DESC_HID) {
			const uint8_t *descriptor_data = (const uint8_t *)dhp;
			/* Fetch the report descriptor length */
			report_descriptor_length = sys_get_le16(&descriptor_data[7u]);
		}
		/* Interrupt in endpoint */
		else if (dhp->bDescriptorType == USB_DESC_ENDPOINT) {
			const struct usb_ep_descriptor *const ep =
				(const struct usb_ep_descriptor *)dhp;
			/* Note down the interrupt IN endpoint */
			if (USB_EP_DIR_IS_IN(ep->bEndpointAddress) &&
			    (ep->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) ==
				    USB_EP_TYPE_INTERRUPT) {
				LOG_INF("Found input endpoint at 0x%02X", ep->bEndpointAddress);
				dev_data->input_ep = ep;
			}
			/* Optional interrupt OUT endpoint for output and feature reports */
			else {
				LOG_INF("Found output endpoint at 0x%02X", ep->bEndpointAddress);
				dev_data->output_ep = ep;
			}
		}

		dhp = usbh_desc_get_next(dhp);
	}

	if (dev_data->input_ep == NULL) {
		LOG_ERR("No input endpoint found");
		return -ENOSYS;
	}

	/* Fetch report descriptor */
	if (report_descriptor_length == 0) {
		LOG_ERR("No report descriptor found");
		return -ENOSYS;
	}

	buf = usbh_xfer_buf_alloc(dev_data->udev, report_descriptor_length);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = req_iface_desc(dev_data->udev, USB_DESC_HID_REPORT, 0u, iface,
			     report_descriptor_length, buf);
	if (ret != 0) {
		LOG_WRN("Request for HID report descriptor failed: %i", ret);
		goto error_cleanup;
	}

	LOG_HEXDUMP_INF(buf->data, buf->len, "Report: ");

	ret = usbh_hid_report_parse(&dev_data->report, buf->len, buf->data);
	if (ret != 0) {
		LOG_WRN("Parsing of the HID report descriptor failed: %i", ret);
		goto error_cleanup;
	}

error_cleanup:
	usbh_xfer_buf_free(dev_data->udev, buf);

	return ret;
}

/*
 * Initialize the HID host class driver
 */
static int usbh_class_init(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_hid_data *dev_data = (void *)dev->data;
	int ret = 0;

	LOG_INF("Initializing HID host data");

	memset(dev_data, 0x00, sizeof(*dev_data));

	dev_data->dev = dev;

	ret = k_mutex_init(&dev_data->lock);
	if (ret != 0) {
		return ret;
	}

	ret = k_sem_init(&dev_data->sync, 0, 1);
	if (ret != 0) {
		return ret;
	}

	k_work_init(&dev_data->stall_recovery_work, stall_recovery_work_handler);

	LOG_INF("HID host data initialized successfully");
	return 0;
}

/*
 * Probe the USB class driver after a device has been found
 */
static int usbh_class_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
			    uint8_t iface)
{
	const struct device *dev = c_data->priv;
	struct usbh_hid_data *dev_data = dev->data;
	const struct usbh_hid_config *dev_config = dev->config;
	uint8_t target_iface = 0;
	int ret = 0;

	if ((udev == NULL) || (udev->state != USB_STATE_CONFIGURED)) {
		LOG_ERR("USB device not properly configured");
		return -ENODEV;
	}

	if (dev_data == NULL) {
		LOG_ERR("No HID device instance available");
		return -ENODEV;
	}

	/* Convert device-level match to interface 0 */
	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		target_iface = 0u;
	} else {
		target_iface = iface;
	}

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	/* Clear some data before starting */
	dev_data->input_ep = NULL;
	dev_data->output_ep = NULL;
	memset(&dev_data->report, 0, sizeof(dev_data->report));

	dev_data->udev = udev;
	dev_data->target_iface = target_iface;

	/* Fetch the relevant information */
	ret = scan_descriptors(dev_config, dev_data, target_iface);
	if (ret != 0) {
		goto error_cleanup;
	}

	/* Select the boot protocol */
	if (dev_config->start_with_boot_protocol) {
		ret = driver_set_protocol(dev, HID_PROTOCOL_BOOT);
		if (ret != 0) {
			LOG_ERR("Failed to set boot protocol: %i", ret);
			goto error_cleanup;
		}
	}
	/* Set the generic protocol */
	else {
		ret = driver_set_protocol(dev, HID_PROTOCOL_REPORT);
		if (ret == -EPIPE) {
			/* Set report protocol not supported, can ignore */
		} else if (ret != 0) {
			LOG_ERR("Failed to set report protocol: %i", ret);
			goto error_cleanup;
		}
	}

	/* Set the idle rate */
	for (size_t idle_rate_index = 0; idle_rate_index < dev_config->idle_rates_ms_size / 2;
	     idle_rate_index++) {
		uint8_t report_id = dev_config->idle_rates_ms[idle_rate_index * 2u];
		uint16_t idle_period_ms = dev_config->idle_rates_ms[idle_rate_index * 2u + 1u];

		ret = driver_set_idle_rate(dev, report_id, idle_period_ms);
		if (ret == -EPIPE) {
			/* Set idle not supported, can ignore */
		}
		/* Other error */
		else if (ret != 0) {
			LOG_WRN("Failed to set idle rate for report ID 0x%02X: %i", report_id, ret);
			goto error_cleanup;
		}
	}

	LOG_INF("HID device (addr=%d) initialization completed for iface %i", dev_data->udev->addr,
		target_iface);

	k_mutex_unlock(&dev_data->lock);

	return 0;

error_cleanup:
	k_mutex_unlock(&dev_data->lock);

	usbh_class_remove(c_data);

	return ret;
}

/*
 * Remove the USB class driver on disconnection
 */
static int usbh_class_remove(struct usbh_class_data *const c_data)
{
	const struct device *dev = c_data->priv;
	struct usbh_hid_data *dev_data = (void *)dev->data;

	driver_stop_input_reports(dev);

	k_work_cancel(&dev_data->stall_recovery_work);

	k_mutex_lock(&dev_data->lock, K_FOREVER);

#if CONFIG_USBH_HID_ROUTE_TO_INPUT
	memset(dev_data->previous_reports, 0u, sizeof(dev_data->previous_reports));
#endif

	memset(&dev_data->report, 0u, sizeof(dev_data->report));

	dev_data->input_ep = NULL;
	dev_data->output_ep = NULL;
	dev_data->interrupt_in_xfer = NULL;
	dev_data->input_interrupt_report_on = false;
	dev_data->stalled_input_endpoint = false;
	dev_data->target_iface = 0;
	dev_data->udev = NULL;

	k_mutex_unlock(&dev_data->lock);

	LOG_INF("HID device removal completed");

	return 0;
}

/*
 * USB Host class API vtable
 */
static __maybe_unused struct usbh_class_api usbh_class_api = {
	.init = usbh_class_init,
	.probe = usbh_class_probe,
	.removed = usbh_class_remove,
};

/*
 * USB Host class filters
 */
static __maybe_unused struct usbh_class_filter const generic_hid_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HID,
		.sub = USB_HID_SUBCLASS_NONE,
		.proto = HID_BOOT_IFACE_CODE_NONE,
	},
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HID,
		.sub = USB_HID_SUBCLASS_NONE,
		.proto = HID_BOOT_IFACE_CODE_MOUSE,
	},
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HID,
		.sub = USB_HID_SUBCLASS_BOOT,
		.proto = HID_BOOT_IFACE_CODE_MOUSE,
	},
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HID,
		.sub = USB_HID_SUBCLASS_BOOT,
		.proto = HID_BOOT_IFACE_CODE_KEYBOARD,
	},
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_HID,
		.sub = USB_HID_SUBCLASS_NONE,
		.proto = HID_BOOT_IFACE_CODE_KEYBOARD,
	},
	{0u},
};

/*
 * Synchronization callback to wait for completion of asynchronous transfers
 * Should be passed to `usbh_xfer_alloc_with_buf` or `usbh_xfer_alloc` before queuing the
 * transfer, to then block on `dev_data->sync` in order to wait for completion. This
 * function only gives way to the semaphore; it doesn't analyze or deallocate anything.
 */
static int sync_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_hid_data *dev_data = xfer->priv;

	if (xfer->err != 0) {
		LOG_DBG("Request finished %p, err %d, sem %i", xfer, xfer->err,
			k_sem_count_get(&dev_data->sync));
	}

	/* If the transfer was cancelled we deallocate it here */
	if (xfer->err == -ECONNRESET) {
		LOG_INF("Transfer %p cancelled", (void *)xfer);
		usbh_xfer_free(udev, xfer);

		return 0;
	}

	k_sem_give(&dev_data->sync);

	return 0;
}

/*
 * Block on `dev_data->sync` waiting for the `sync_cb` callback
 * This function waits for the last transfer enqueued with `sync_cb` as completion to be
 * done. If it actually completes it returns the error code; in the event of a timeout it
 * makes sure the transfer is no longer pending.
 */
static int wait_for_sync(struct usbh_hid_data *dev_data, struct uhc_transfer *xfer)
{
	int ret = 0;

	if (k_sem_take(&dev_data->sync, K_MSEC(100u)) != 0) {
		LOG_ERR("Timeout");

		ret = usbh_xfer_dequeue(dev_data->udev, xfer);
		/* While the semaphore take timed out, the transfer was actually already
		 * done and the callback on its way.
		 */
		if (ret == -EALREADY) {
			/* Take the semaphore again to be sure that the callback is done */
			if (k_sem_take(&dev_data->sync, K_MSEC(100u)) != 0) {
				/* Should not happen */
				LOG_ERR("Double timeout");
			}
		}
		/* Dequeue failed */
		else if (ret != 0) {
			LOG_ERR("Failed to cancel transfer");
		}
		/* Dequeue succeeded, do nothing */
		else {
		}

		/* The USB host driver may still need to work with the transfer, so we leave it to
		 * the callback to deallocate it.
		 */

		return -ETIMEDOUT;
	}

	/* The transfer was successful, store the ret and free it */
	ret = xfer->err;
	usbh_xfer_free(dev_data->udev, xfer);

	return ret;
}

static int driver_get_report_descriptor(const struct device *dev, struct usbh_hid_report *report)
{
	struct usbh_hid_data *dev_data = (void *)dev->data;

	if (report == NULL) {
		return -EINVAL;
	}

	/* Copy the report descriptor */
	k_mutex_lock(&dev_data->lock, K_FOREVER);

	if (dev_data->udev == NULL) {
		/* Device not yet connected */
		k_mutex_unlock(&dev_data->lock);
		return -ENOTCONN;
	}

	memcpy(report, &dev_data->report, sizeof(struct usbh_hid_report));
	k_mutex_unlock(&dev_data->lock);

	return 0;
}

static int driver_get_report(const struct device *dev, enum usbh_hid_report_field_type type,
			     uint8_t report_id, size_t length, uint8_t *buffer)
{
	struct usbh_hid_data *dev_data = (void *)dev->data;
	struct net_buf *buf = NULL;
	int ret = 0;

	k_mutex_lock(&dev_data->lock, K_FOREVER);
	buf = usbh_xfer_buf_alloc(dev_data->udev, length);
	if (buf == NULL) {
		k_mutex_unlock(&dev_data->lock);
		return -ENOMEM;
	}

	ret = hid_class_request(dev_data->udev, dev_data->target_iface, USB_REQTYPE_DIR_TO_HOST,
				USB_HID_GET_REPORT, (type << 8u) | report_id, length, buf);

	if (ret == 0) {
		memcpy(buffer, buf->data, buf->len);
	}

	usbh_xfer_buf_free(dev_data->udev, buf);
	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int driver_set_report(const struct device *dev, enum usbh_hid_report_field_type type,
			     uint8_t report_id, size_t data_length, uint8_t const data[data_length])
{
	struct usbh_hid_data *dev_data = (void *)dev->data;
	struct net_buf *buf = NULL;
	int ret = 0;

	if (data == NULL || data_length == 0u) {
		return -EINVAL;
	}

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	if (dev_data->udev == NULL) {
		/* Device not yet connected */
		k_mutex_unlock(&dev_data->lock);
		return -ENOTCONN;
	}

	/* If an interrupt OUT endpoint is specified use that */
	if (dev_data->output_ep) {
		size_t buffer_length = data_length;

		/* If through the output interrupt and with a non null ID the report ID must be the
		 * first byte
		 */
		if (report_id != 0) {
			buffer_length++;
		}

		buf = usbh_xfer_buf_alloc(dev_data->udev, buffer_length);
		if (buf == NULL) {
			ret = -ENOMEM;
			goto error_cleanup;
		}

		if (report_id != 0) {
			net_buf_add_u8(buf, report_id);
		}
		net_buf_add_mem(buf, data, data_length);

		struct uhc_transfer *xfer = usbh_xfer_alloc(
			dev_data->udev, dev_data->output_ep->bEndpointAddress, sync_cb, dev_data);
		if (xfer == NULL) {
			ret = -ENOMEM;
			goto error_cleanup;
		}

		ret = usbh_xfer_buf_add(dev_data->udev, xfer, buf);
		if (ret != 0) {
			usbh_xfer_free(dev_data->udev, xfer);
			goto error_cleanup;
		}

		LOG_DBG("Sending report ID 0x%02X on output endpoint", report_id);
		ret = usbh_xfer_enqueue(dev_data->udev, xfer);
		if (ret != 0) {
			usbh_xfer_free(dev_data->udev, xfer);
			goto error_cleanup;
		}

		/* Wait for completion, deallocation handled automatically */
		ret = wait_for_sync(dev_data, xfer);
	}
	/* No interrupt OUT endpoint, fall back to a control request */
	else {
		buf = usbh_xfer_buf_alloc(dev_data->udev, data_length);
		if (buf == NULL) {
			ret = -ENOMEM;
			goto error_cleanup;
		}

		net_buf_add_mem(buf, data, data_length);

		LOG_DBG("Sending report ID 0x%02X on control endpoint", report_id);
		ret = hid_class_request(dev_data->udev, dev_data->target_iface,
					USB_REQTYPE_DIR_TO_DEVICE, USB_HID_SET_REPORT,
					((uint16_t)type << 8u) | report_id, data_length, buf);
	}

error_cleanup:
	if (buf != NULL) {
		usbh_xfer_buf_free(dev_data->udev, buf);
	}

	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int driver_start_input_reports(const struct device *dev)
{
	struct usbh_hid_data *dev_data = (void *)dev->data;
	int ret = 0;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	if (dev_data->udev == NULL) {
		/* Device not yet connected */
		k_mutex_unlock(&dev_data->lock);
		return -ENOTCONN;
	}

	if (dev_data->input_interrupt_report_on) {
		/* Process already started */
		k_mutex_unlock(&dev_data->lock);
		return -EINPROGRESS;
	}

	if (dev_data->stalled_input_endpoint) {
		/* Release the lock across the blocking control request below, for the same
		 * reason as in stall_recovery_work_handler(): it can take several seconds to
		 * time out, and a concurrent usbh_class_remove() needs this same lock.
		 */
		struct usb_device *udev = dev_data->udev;
		uint8_t ep_addr = dev_data->input_ep->bEndpointAddress;

		k_mutex_unlock(&dev_data->lock);

		ret = usbh_req_clear_sfs_halt(udev, ep_addr);

		k_mutex_lock(&dev_data->lock, K_FOREVER);

		/* The device was removed (or a different one reconnected) while unlocked */
		if (dev_data->udev != udev) {
			k_mutex_unlock(&dev_data->lock);
			return -ENOTCONN;
		}

		if (ret == 0) {
			dev_data->stalled_input_endpoint = false;
		} else {
			LOG_ERR("Could not restore input endpoint: %i", ret);
		}

		/* Re-check in case there was a concurrent call to this function while we left the
		 * lock
		 */
		if (dev_data->input_interrupt_report_on) {
			k_mutex_unlock(&dev_data->lock);
			return -EINPROGRESS;
		}
	}

	if (ret == 0) {
		dev_data->input_interrupt_report_on = true;

		/* Start input interrupt */
		ret = req_interrupt_input(dev);
		/* The interrupt was technically started, silence the error */
		if (ret == -EALREADY) {
			ret = 0;
		}
	}

	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int driver_stop_input_reports(const struct device *dev)
{
	struct usbh_hid_data *dev_data = (void *)dev->data;
	int ret = 0;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	if (dev_data->udev == NULL) {
		/* Device not yet connected */
		k_mutex_unlock(&dev_data->lock);
		return -ENOTCONN;
	}

	if (!dev_data->input_interrupt_report_on) {
		/* Process not started */
		k_mutex_unlock(&dev_data->lock);
		return -ENOTCONN;
	}

	dev_data->input_interrupt_report_on = false;
	/* No transfer to cancel while waiting on stall recovery; the deferred work checks
	 * input_interrupt_report_on before resuming
	 */
	if (dev_data->interrupt_in_xfer != NULL) {
		ret = usbh_xfer_dequeue(dev_data->udev, dev_data->interrupt_in_xfer);
	}

	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int driver_set_input_callback(const struct device *dev, usbh_hid_report_cb_t callback,
				     void *user_data)
{
	struct usbh_hid_data *dev_data = (void *)dev->data;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	dev_data->input_cb = callback;
	dev_data->user_data = user_data;

	k_mutex_unlock(&dev_data->lock);

	return 0;
}

static bool has_report_id(const struct usbh_hid_report *report, uint8_t report_id)
{
	/* 0 means all report IDs; always present */
	if (report_id == 0) {
		return true;
	}
	/* If a specific report ID is given check if it actually matches one in the report
	 * descriptor
	 */
	else {
		bool found = false;

		for (size_t report_index = 0; report_index < report->num_reports; report_index++) {
			if (report->reports[report_index] == report_id) {
				found = true;
				break;
			}
		}

		return found;
	}
}

static int driver_set_idle_rate(const struct device *dev, uint8_t report_id,
				uint16_t idle_period_ms)
{
	struct usbh_hid_data *dev_data = (void *)dev->data;
	int ret = 0;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	if (dev_data->udev == NULL) {
		/* Device not yet connected */
		ret = -ENOTCONN;
		goto error_cleanup;
	}

	if (!has_report_id(&dev_data->report, report_id)) {
		LOG_ERR("Report ID 0x%02X doesn't exist", report_id);
		ret = -EINVAL;
		goto error_cleanup;
	}

	ret = hid_class_request(dev_data->udev, dev_data->target_iface, USB_REQTYPE_DIR_TO_DEVICE,
				USB_HID_SET_IDLE, ((idle_period_ms / 4u) << 8u) | report_id, 0,
				NULL);
	/* Set idle not supported, can ignore */
	if (ret == -EPIPE) {
		LOG_INF("HID does not support SET_IDLE");
		goto error_cleanup;
	}
	/* Any other error is a problem */
	else if (ret != 0) {
		LOG_WRN("Failed to set idle rate: %i", ret);
		goto error_cleanup;
	}

error_cleanup:
	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int driver_get_idle_rate(const struct device *dev, uint8_t report_id,
				uint16_t *idle_period_ms)
{
	struct usbh_hid_data *dev_data = (void *)dev->data;
	int ret = 0;
	struct net_buf *buf = NULL;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	if (dev_data->udev == NULL) {
		/* Device not yet connected */
		ret = -ENOTCONN;
		goto error_cleanup;
	}

	if (!has_report_id(&dev_data->report, report_id)) {
		LOG_ERR("Report ID 0x%02X doesn't exist", report_id);
		ret = -EINVAL;
		goto error_cleanup;
	}

	buf = usbh_xfer_buf_alloc(dev_data->udev, 1);
	if (buf == NULL) {
		ret = -ENOMEM;
		goto error_cleanup;
	}
	ret = hid_class_request(dev_data->udev, dev_data->target_iface, USB_REQTYPE_DIR_TO_HOST,
				USB_HID_GET_IDLE, report_id, 1, buf);
	/* Set idle not supported, can ignore */
	if (ret == -EPIPE) {
		LOG_INF("HID does not support GET_IDLE");
		goto error_cleanup;
	}
	/* Any other error is a problem */
	else if (ret != 0) {
		LOG_WRN("Failed to get idle rate: %i", ret);
		goto error_cleanup;
	}

	*idle_period_ms = buf->data[0] * 4;

error_cleanup:
	if (buf != NULL) {
		usbh_xfer_buf_free(dev_data->udev, buf);
	}

	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int driver_set_protocol(const struct device *dev, uint8_t protocol_code)
{
	struct usbh_hid_data *dev_data = (void *)dev->data;
	int ret = 0;

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	if (dev_data->udev == NULL) {
		/* Device not yet connected */
		ret = -ENOTCONN;
		goto error_cleanup;
	}

	if (protocol_code == HID_PROTOCOL_BOOT) {
		const struct usb_if_descriptor *interface =
			usbh_desc_get_iface(dev_data->udev, dev_data->target_iface);
		/* The device may not support the boot protocol, in which case we abort */
		if (interface->bInterfaceSubClass != USB_HID_SUBCLASS_BOOT) {
			LOG_WRN("Device doesn't support boot protocol!");
			ret = -ENOTSUP;
			goto error_cleanup;
		}
	}

	/* Set the requested protocol */
	ret = hid_class_request(dev_data->udev, dev_data->target_iface, USB_REQTYPE_DIR_TO_DEVICE,
				USB_HID_SET_PROTOCOL, protocol_code, 0, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to set protocol %i: %i", protocol_code, ret);
		goto error_cleanup;
	}

	/* Save which protocol is currently enabled for the input event subsystem */
	if (protocol_code == HID_PROTOCOL_BOOT) {
		dev_data->boot_protocol = true;
	} else {
		dev_data->boot_protocol = false;
	}

error_cleanup:
	k_mutex_unlock(&dev_data->lock);

	return ret;
}

static int driver_get_protocol(const struct device *dev, uint8_t *protocol_code)
{
	struct usbh_hid_data *dev_data = (void *)dev->data;
	struct net_buf *buf = NULL;
	int ret = 0;

	if (protocol_code == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	if (dev_data->udev == NULL) {
		/* Device not yet connected */
		goto error_cleanup;
		return -ENOTCONN;
	}

	buf = usbh_xfer_buf_alloc(dev_data->udev, 1);
	if (buf == NULL) {
		k_mutex_unlock(&dev_data->lock);
		return -ENOMEM;
	}

	/* Set the requested protocol */
	ret = hid_class_request(dev_data->udev, dev_data->target_iface, USB_REQTYPE_DIR_TO_HOST,
				USB_HID_GET_PROTOCOL, 0, 1, buf);
	if (ret != 0) {
		LOG_ERR("Failed to get protocol: %i", ret);
		goto error_cleanup;
	}

	*protocol_code = buf->data[0];

error_cleanup:
	if (buf != NULL) {
		usbh_xfer_buf_free(dev_data->udev, buf);
	}

	k_mutex_unlock(&dev_data->lock);

	return ret;
}

/*
 * USB Host HID driver API vtable
 */
static DEVICE_API(usbh_hid, driver_api) = {
	.get_report_descriptor = driver_get_report_descriptor,
	.get_report = driver_get_report,
	.set_report = driver_set_report,
	.start_input_reports = driver_start_input_reports,
	.stop_input_reports = driver_stop_input_reports,
	.set_idle_rate = driver_set_idle_rate,
	.get_idle_rate = driver_get_idle_rate,
	.set_protocol = driver_set_protocol,
	.get_protocol = driver_get_protocol,
	.set_input_callback = driver_set_input_callback,
};

#define USBH_DEVICE_DEFINE(index)                                                                  \
	static struct usbh_hid_data dev_data_##index = {0u};                                       \
	static struct usbh_hid_config const dev_config_##index = {                                 \
		.idle_rates_ms = DT_INST_PROP(index, in_idle_rate_ms),                             \
		.idle_rates_ms_size = DT_INST_PROP_LEN(index, in_idle_rate_ms),                    \
		.start_with_boot_protocol = DT_INST_PROP(index, start_with_boot_protocol),         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, NULL, NULL, &dev_data_##index, &dev_config_##index,           \
			      POST_KERNEL, CONFIG_USBH_HID_INIT_PRIORITY, &driver_api);            \
                                                                                                   \
	COND_CODE_0(DT_INST_PROP(index, match_class),                                              \
	(static struct usbh_class_filter const hid_filters_##index[] = {                           \
		{                                                                                  \
			.flags = USBH_CLASS_MATCH_VID_PID,                                         \
			.vid = (DT_INST_REG_ADDR(index) >> 16u) & 0xFFFFu,                         \
			.pid = DT_INST_REG_ADDR(index) & 0xFFFFu,                                  \
		},                                                                                 \
		{0u},                                                                              \
	};), ()                                                                                    \
	) \
                                                                                                   \
	USBH_DEFINE_CLASS(usbh_hid_data_##index, &usbh_class_api,                                  \
			  (void *)DEVICE_DT_INST_GET(index),                                       \
			  COND_CODE_1(DT_INST_PROP(index, match_class),                            \
				  (generic_hid_filters), (hid_filters_##index)));

DT_INST_FOREACH_STATUS_OKAY(USBH_DEVICE_DEFINE)
