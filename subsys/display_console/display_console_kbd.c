/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Keyboard front end: the key events of every input device become the bytes
 * a terminal keyboard sends, queued on the receive side of the console UART.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_hid.h>
#include <zephyr/usb/class/hid.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(display_console, CONFIG_DISPLAY_CONSOLE_LOG_LEVEL);

/* Largest number of bytes one key event produces. */
#define DISPLAY_CONSOLE_KBD_MAX_BYTES 4

/* US layout, indexed by HID usage from HID_KEY_MINUS to HID_KEY_SLASH. */
static const char display_console_kbd_punct[] = "-=[]\\#;'`,./";
static const char display_console_kbd_punct_shift[] = "_+{}|~:\"~<>?";
/* Indexed by HID usage from HID_KEY_KPSLASH to HID_KEY_KPDOT. */
static const char display_console_kbd_keypad[] = "/*-+\r1234567890.";

/* Modifier keys held, as a combination of the HID keyboard modifier bits. */
static uint8_t display_console_kbd_modifiers;

static size_t display_console_kbd_translate(int16_t code, uint8_t modifiers, char *out, size_t size)
{
	const bool shift =
		(modifiers & (HID_KBD_MODIFIER_LEFT_SHIFT | HID_KBD_MODIFIER_RIGHT_SHIFT)) != 0;
	const bool ctrl =
		(modifiers & (HID_KBD_MODIFIER_LEFT_CTRL | HID_KBD_MODIFIER_RIGHT_CTRL)) != 0;
	const char *seq = NULL;
	size_t len;
	char c;

	if (code >= HID_KEY_A && code <= HID_KEY_Z) {
		c = (char)('a' + (code - HID_KEY_A));
		if (ctrl) {
			c = (char)(c - 'a' + 1);
		} else if (shift) {
			c = (char)(c - 'a' + 'A');
		}
	} else if (code >= HID_KEY_1 && code <= HID_KEY_0) {
		c = (shift ? "!@#$%^&*()" : "1234567890")[code - HID_KEY_1];
	} else if (code >= HID_KEY_MINUS && code <= HID_KEY_SLASH) {
		c = (shift ? display_console_kbd_punct_shift
			   : display_console_kbd_punct)[code - HID_KEY_MINUS];
	} else if (code >= HID_KEY_KPSLASH && code <= HID_KEY_KPDOT) {
		c = display_console_kbd_keypad[code - HID_KEY_KPSLASH];
	} else {
		switch (code) {
		case HID_KEY_ENTER:
			c = '\r';
			break;
		case HID_KEY_ESC:
			c = 0x1b;
			break;
		case HID_KEY_BACKSPACE:
			c = '\b';
			break;
		case HID_KEY_TAB:
			c = '\t';
			break;
		case HID_KEY_SPACE:
			c = ' ';
			break;
		case HID_KEY_UP:
			seq = "\x1b[A";
			break;
		case HID_KEY_DOWN:
			seq = "\x1b[B";
			break;
		case HID_KEY_RIGHT:
			seq = "\x1b[C";
			break;
		case HID_KEY_LEFT:
			seq = "\x1b[D";
			break;
		case HID_KEY_HOME:
			seq = "\x1b[H";
			break;
		case HID_KEY_END:
			seq = "\x1b[F";
			break;
		case HID_KEY_DELETE:
			seq = "\x1b[3~";
			break;
		default:
			return 0;
		}
	}

	if (seq == NULL) {
		seq = &c;
		len = 1;
	} else {
		len = strlen(seq);
	}

	if (len > size) {
		return 0;
	}

	memcpy(out, seq, len);

	return len;
}

/*
 * Translate a key event into the bytes a terminal keyboard sends. Modifier
 * keys and key releases produce no bytes; repeats do.
 */
static size_t display_console_kbd_bytes(const struct input_event *evt, char *out, size_t size)
{
	uint8_t modifier;

	if (evt->type != INPUT_EV_KEY) {
		return 0;
	}

	modifier = input_to_hid_modifier(evt->code);
	if (modifier != 0U) {
		if (evt->value != 0) {
			display_console_kbd_modifiers |= modifier;
		} else {
			display_console_kbd_modifiers &= (uint8_t)~modifier;
		}
		return 0;
	}

	if (evt->value == 0) {
		return 0;
	}

	return display_console_kbd_translate(input_to_hid_code(evt->code),
					     display_console_kbd_modifiers, out, size);
}

static void display_console_kbd_cb(struct input_event *evt, void *user_data)
{
	const struct device *uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	char out[DISPLAY_CONSOLE_KBD_MAX_BYTES];
	size_t len;
	uint32_t n;

	ARG_UNUSED(user_data);

	len = display_console_kbd_bytes(evt, out, sizeof(out));
	if (len == 0U) {
		return;
	}

	n = uart_emul_put_rx_data(uart, (const uint8_t *)out, len);
	if (n < len) {
		LOG_WRN("%s RX FIFO full, %zu bytes dropped", uart->name, len - n);
	}
}

INPUT_CALLBACK_DEFINE(NULL, display_console_kbd_cb, NULL);
