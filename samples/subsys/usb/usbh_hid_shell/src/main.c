/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/shell/shell.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/class/usbh_hid.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if CONFIG_USB_HOST_STACK
USBH_CONTROLLER_DEFINE(sample_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));
#endif

struct device const *dev = DEVICE_DT_GET(DT_NODELABEL(any_hid_device));

static int cmd_set_report(const struct shell *sh, size_t argc, char **argv)
{
	int result = 0;
	uint8_t report_id = 0;
	enum usbh_hid_report_field_type report_type = 0;
	uint8_t buffer[64] = {0};

	if (argc < 3) {
		shell_error(sh,
			    "Not enough arguments: set_report <report_id> <report_type> <data>");
		return -EINVAL;
	}

	report_id = atoi(argv[1]);
	report_type = atoi(argv[2]);

	size_t data_length = argc - 3;

	if (data_length > sizeof(buffer)) {
		shell_error(sh, "Too much data");
		return -ENOMEM;
	}

	switch (report_type) {
	case USBH_HID_REPORT_FIELD_TYPE_INPUT:
	case USBH_HID_REPORT_FIELD_TYPE_OUTPUT:
	case USBH_HID_REPORT_FIELD_TYPE_FEATURE: {
		break;
	}

	default: {
		shell_error(sh, "Invalid report type");
		return -EINVAL;
	}
	}

	for (size_t i = 0; i < data_length; i++) {
		buffer[i] = strtol(argv[3 + i], NULL, 16);
	}

	LOG_HEXDUMP_DBG(buffer, data_length, "HEX");

	result = usbh_hid_set_report(dev, report_type, report_id, data_length, buffer);
	if (result != 0) {
		shell_error(sh, "Could not set report: %i", result);
		return result;
	}

	return 0;
}

static int cmd_get_report(const struct shell *sh, size_t argc, char **argv)
{
	int result = 0;
	uint8_t report_id = 0;
	enum usbh_hid_report_field_type report_type = 0;
	uint8_t buffer[64] = {0};
	uint8_t data_length = 0;

	if (argc < 4) {
		shell_error(sh,
			    "Not enough arguments: get_report <report_id> <report_type> <length>");
		return -EINVAL;
	}

	report_id = atoi(argv[1]);
	report_type = atoi(argv[2]);
	data_length = atoi(argv[3]);

	if (data_length > sizeof(buffer)) {
		shell_error(sh, "Too much data");
		return -ENOMEM;
	}

	switch (report_type) {
	case USBH_HID_REPORT_FIELD_TYPE_INPUT:
	case USBH_HID_REPORT_FIELD_TYPE_OUTPUT:
	case USBH_HID_REPORT_FIELD_TYPE_FEATURE: {
		break;
	}

	default: {
		shell_error(sh, "Invalid report type");
		return -EINVAL;
	}
	}

	result = usbh_hid_get_report(dev, report_type, report_id, data_length, buffer);
	if (result != 0) {
		shell_error(sh, "Could not get report: %i", result);
		return result;
	}

	shell_hexdump_line(sh, 0, buffer, data_length);

	return 0;
}

static int cmd_set_idle_rate(const struct shell *sh, size_t argc, char **argv)
{
	int result = 0;
	uint8_t report_id = 0;
	uint16_t idle_period_ms = 0;

	if (argc < 3) {
		shell_error(sh, "Not enough arguments: set_idle <report_id> <idle_period>");
		return -EINVAL;
	}

	report_id = atoi(argv[1]);
	idle_period_ms = atoi(argv[2]);

	result = usbh_hid_set_idle_rate(dev, report_id, idle_period_ms);
	if (result != 0) {
		shell_error(sh, "Could not set idle rate: %i", result);
		return result;
	}

	return 0;
}

static int cmd_get_idle_rate(const struct shell *sh, size_t argc, char **argv)
{
	int result = 0;
	uint8_t report_id = 0;
	uint16_t idle_period_ms = 0;

	if (argc < 2) {
		shell_error(sh, "Not enough arguments: get_idle <report_id>");
		return -EINVAL;
	}

	report_id = atoi(argv[1]);

	result = usbh_hid_get_idle_rate(dev, report_id, &idle_period_ms);
	if (result == 0) {
		shell_print(sh, "Idle rate for report 0x%02X: %i", report_id, idle_period_ms);
	} else {
		shell_error(sh, "Could not get idle rate: %i", result);
		return result;
	}

	return 0;
}

static int cmd_set_protocol(const struct shell *sh, size_t argc, char **argv)
{
	int result = 0;
	uint8_t protocol_code = 0;

	if (argc < 2) {
		shell_error(sh, "Not enough arguments: set_protocol <protocol_code>");
		return -EINVAL;
	}

	protocol_code = atoi(argv[1]);

	result = usbh_hid_set_protocol(dev, protocol_code);
	if (result != 0) {
		shell_error(sh, "Could not set protocol: %i", result);
		return result;
	}

	return 0;
}

static int cmd_get_protocol(const struct shell *sh, size_t argc, char **argv)
{
	int result = 0;
	uint8_t protocol_code = 0;

	if (argc < 1) {
		shell_error(sh, "Not enough arguments: get_protocol");
		return -EINVAL;
	}

	result = usbh_hid_get_protocol(dev, &protocol_code);
	if (result == 0) {
		shell_print(sh, "Protocol code for device: %i", protocol_code);
	} else {
		shell_error(sh, "Could not get protocol code: %i", result);
		return result;
	}

	return 0;
}

static int cmd_input(const struct shell *sh, size_t argc, char **argv)
{
	int result = 0;

	if (strcmp(argv[1], "start") == 0) {
		result = usbh_hid_start_input_reports(dev);
		if (result == 0) {
			shell_print(sh, "Started input reports");
		} else {
			shell_error(sh, "Could not start input reports: %i", result);
			return result;
		}
	} else if (strcmp(argv[1], "stop") == 0) {
		result = usbh_hid_stop_input_reports(dev);
		if (result == 0) {
			shell_print(sh, "Stopped input reports");
		} else {
			shell_error(sh, "Could not stop input reports: %i", result);
			return result;
		}
	} else {
		shell_error(sh, "Invalid argument, expected \"start\" or \"stop\"");
		return -EINVAL;
	}

	return 0;
}

/* Map from INPUT_KEY_* value to string name */
struct input_key_name {
	uint16_t key;
	const char *name;
};

static const struct input_key_name input_key_names[] = {
	{INPUT_KEY_0, "0"},
	{INPUT_KEY_1, "1"},
	{INPUT_KEY_2, "2"},
	{INPUT_KEY_3, "3"},
	{INPUT_KEY_4, "4"},
	{INPUT_KEY_5, "5"},
	{INPUT_KEY_6, "6"},
	{INPUT_KEY_7, "7"},
	{INPUT_KEY_8, "8"},
	{INPUT_KEY_9, "9"},
	{INPUT_KEY_A, "A"},
	{INPUT_KEY_APOSTROPHE, "Apostrophe"},
	{INPUT_KEY_B, "B"},
	{INPUT_KEY_BACK, "Back"},
	{INPUT_KEY_BACKSLASH, "Backslash"},
	{INPUT_KEY_BACKSPACE, "Backspace"},
	{INPUT_KEY_BLUETOOTH, "Bluetooth"},
	{INPUT_KEY_BRIGHTNESSDOWN, "Brightness Down"},
	{INPUT_KEY_BRIGHTNESSUP, "Brightness Up"},
	{INPUT_KEY_C, "C"},
	{INPUT_KEY_CAPSLOCK, "Caps Lock"},
	{INPUT_KEY_CLOSECD, "Close CD"},
	{INPUT_KEY_COFFEE, "Screen Saver"},
	{INPUT_KEY_COMMA, "Comma"},
	{INPUT_KEY_COMPOSE, "Compose"},
	{INPUT_KEY_CONNECT, "Connect"},
	{INPUT_KEY_D, "D"},
	{INPUT_KEY_DELETE, "Delete"},
	{INPUT_KEY_DOLLAR, "Dollar"},
	{INPUT_KEY_DOT, "Dot"},
	{INPUT_KEY_DOWN, "Down"},
	{INPUT_KEY_E, "E"},
	{INPUT_KEY_EJECTCD, "Eject CD"},
	{INPUT_KEY_EJECTCLOSECD, "Eject/Close CD"},
	{INPUT_KEY_END, "End"},
	{INPUT_KEY_ENTER, "Enter"},
	{INPUT_KEY_EQUAL, "Equal"},
	{INPUT_KEY_ESC, "Escape"},
	{INPUT_KEY_EURO, "Euro"},
	{INPUT_KEY_F, "F"},
	{INPUT_KEY_F1, "F1"},
	{INPUT_KEY_F2, "F2"},
	{INPUT_KEY_F3, "F3"},
	{INPUT_KEY_F4, "F4"},
	{INPUT_KEY_F5, "F5"},
	{INPUT_KEY_F6, "F6"},
	{INPUT_KEY_F7, "F7"},
	{INPUT_KEY_F8, "F8"},
	{INPUT_KEY_F9, "F9"},
	{INPUT_KEY_F10, "F10"},
	{INPUT_KEY_F11, "F11"},
	{INPUT_KEY_F12, "F12"},
	{INPUT_KEY_F13, "F13"},
	{INPUT_KEY_F14, "F14"},
	{INPUT_KEY_F15, "F15"},
	{INPUT_KEY_F16, "F16"},
	{INPUT_KEY_F17, "F17"},
	{INPUT_KEY_F18, "F18"},
	{INPUT_KEY_F19, "F19"},
	{INPUT_KEY_F20, "F20"},
	{INPUT_KEY_F21, "F21"},
	{INPUT_KEY_F22, "F22"},
	{INPUT_KEY_F23, "F23"},
	{INPUT_KEY_F24, "F24"},
	{INPUT_KEY_FASTFORWARD, "Fast Forward"},
	{INPUT_KEY_FAVORITES, "Favorites"},
	{INPUT_KEY_FORWARD, "Forward"},
	{INPUT_KEY_G, "G"},
	{INPUT_KEY_GRAVE, "Grave"},
	{INPUT_KEY_H, "H"},
	{INPUT_KEY_HOME, "Home"},
	{INPUT_KEY_I, "I"},
	{INPUT_KEY_INSERT, "Insert"},
	{INPUT_KEY_J, "J"},
	{INPUT_KEY_K, "K"},
	{INPUT_KEY_KP0, "Keypad 0"},
	{INPUT_KEY_KP1, "Keypad 1"},
	{INPUT_KEY_KP2, "Keypad 2"},
	{INPUT_KEY_KP3, "Keypad 3"},
	{INPUT_KEY_KP4, "Keypad 4"},
	{INPUT_KEY_KP5, "Keypad 5"},
	{INPUT_KEY_KP6, "Keypad 6"},
	{INPUT_KEY_KP7, "Keypad 7"},
	{INPUT_KEY_KP8, "Keypad 8"},
	{INPUT_KEY_KP9, "Keypad 9"},
	{INPUT_KEY_KPASTERISK, "Keypad Asterisk"},
	{INPUT_KEY_KPCOMMA, "Keypad Comma"},
	{INPUT_KEY_KPDOT, "Keypad Dot"},
	{INPUT_KEY_KPENTER, "Keypad Enter"},
	{INPUT_KEY_KPEQUAL, "Keypad Equal"},
	{INPUT_KEY_KPMINUS, "Keypad Minus"},
	{INPUT_KEY_KPPLUS, "Keypad Plus"},
	{INPUT_KEY_KPPLUSMINUS, "Keypad Plus/Minus"},
	{INPUT_KEY_KPSLASH, "Keypad Slash"},
	{INPUT_KEY_L, "L"},
	{INPUT_KEY_LEFT, "Left"},
	{INPUT_KEY_LEFTALT, "Left Alt"},
	{INPUT_KEY_LEFTBRACE, "Left Brace"},
	{INPUT_KEY_LEFTCTRL, "Left Ctrl"},
	{INPUT_KEY_LEFTMETA, "Left Meta"},
	{INPUT_KEY_LEFTSHIFT, "Left Shift"},
	{INPUT_KEY_M, "M"},
	{INPUT_KEY_MENU, "Menu"},
	{INPUT_KEY_MINUS, "Minus"},
	{INPUT_KEY_MUTE, "Mute"},
	{INPUT_KEY_N, "N"},
	{INPUT_KEY_NEXTSONG, "Next Song"},
	{INPUT_KEY_NUMLOCK, "Num Lock"},
	{INPUT_KEY_O, "O"},
	{INPUT_KEY_P, "P"},
	{INPUT_KEY_PAGEDOWN, "Page Down"},
	{INPUT_KEY_PAGEUP, "Page Up"},
	{INPUT_KEY_PAUSE, "Pause"},
	{INPUT_KEY_PLAY, "Play"},
	{INPUT_KEY_PLAYPAUSE, "Play/Pause"},
	{INPUT_KEY_POWER, "Power"},
	{INPUT_KEY_PREVIOUSSONG, "Previous Song"},
	{INPUT_KEY_PRINT, "Print"},
	{INPUT_KEY_Q, "Q"},
	{INPUT_KEY_R, "R"},
	{INPUT_KEY_RECORD, "Record"},
	{INPUT_KEY_REWIND, "Rewind"},
	{INPUT_KEY_RIGHT, "Right"},
	{INPUT_KEY_RIGHTALT, "Right Alt"},
	{INPUT_KEY_RIGHTBRACE, "Right Brace"},
	{INPUT_KEY_RIGHTCTRL, "Right Ctrl"},
	{INPUT_KEY_RIGHTMETA, "Right Meta"},
	{INPUT_KEY_RIGHTSHIFT, "Right Shift"},
	{INPUT_KEY_S, "S"},
	{INPUT_KEY_SCALE, "Scale"},
	{INPUT_KEY_SCROLLLOCK, "Scroll Lock"},
	{INPUT_KEY_SEMICOLON, "Semicolon"},
	{INPUT_KEY_SLASH, "Slash"},
	{INPUT_KEY_SLEEP, "Sleep"},
	{INPUT_KEY_SPACE, "Space"},
	{INPUT_KEY_STOPCD, "Stop CD"},
	{INPUT_KEY_SYSRQ, "SysRq"},
	{INPUT_KEY_T, "T"},
	{INPUT_KEY_TAB, "Tab"},
	{INPUT_KEY_U, "U"},
	{INPUT_KEY_UP, "Up"},
	{INPUT_KEY_UWB, "Ultra-Wideband"},
	{INPUT_KEY_V, "V"},
	{INPUT_KEY_VOLUMEDOWN, "Volume Down"},
	{INPUT_KEY_VOLUMEUP, "Volume Up"},
	{INPUT_KEY_W, "W"},
	{INPUT_KEY_WAKEUP, "Wake Up"},
	{INPUT_KEY_WLAN, "Wireless LAN"},
	{INPUT_KEY_X, "X"},
	{INPUT_KEY_Y, "Y"},
	{INPUT_KEY_Z, "Z"},
};

static const char *input_key_to_str(uint16_t key)
{
	for (size_t i = 0; i < ARRAY_SIZE(input_key_names); i++) {
		if (input_key_names[i].key == key) {
			return input_key_names[i].name;
		}
	}
	return "Unknown";
}

static void my_input_cb(struct input_event *evt, void *user_data)
{
	switch (evt->type) {
	case INPUT_EV_REL:
		if (evt->code == INPUT_REL_X) {
			LOG_INF("X: %d", evt->value);
		}
		if (evt->code == INPUT_REL_Y) {
			LOG_INF("Y: %d", evt->value);
		}
		if (evt->code == INPUT_REL_WHEEL) {
			LOG_INF("Wheel: %d", evt->value);
		}
		break;
	case INPUT_EV_KEY: {
		if ((evt->code & 0x100) > 0) {
			LOG_INF("Mouse button 0x%03X: %s", evt->code,
				evt->value ? "pressed" : "released");
		} else {
			LOG_INF("button %s: %s", input_key_to_str(evt->code),
				evt->value ? "pressed" : "released");
		}
		break;
	}
	}
}

INPUT_CALLBACK_DEFINE(NULL, my_input_cb, NULL);
SHELL_STATIC_SUBCMD_SET_CREATE(hid_cmds, SHELL_CMD(input, NULL, "Control input reports", cmd_input),
			       SHELL_CMD(set_idle, NULL, "Set idle rate", cmd_set_idle_rate),
			       SHELL_CMD(get_idle, NULL, "Get idle rate", cmd_get_idle_rate),
			       SHELL_CMD(set_protocol, NULL, "Set protocol", cmd_set_protocol),
			       SHELL_CMD(get_protocol, NULL, "Get protocol", cmd_get_protocol),
			       SHELL_CMD(set_report, NULL, "Set report", cmd_set_report),
			       SHELL_CMD(get_report, NULL, "Get report", cmd_get_report),
			       SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(hid, &hid_cmds, "HID output control commands", NULL);

/*
 * Delay for twister integration
 */
static int cmd_delay(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t ms = 0u;

	if (argc < 1) {
		shell_error(sh, "Not enough arguments: delay <ms>");
		return -EINVAL;
	}

	ms = strtoul(argv[1], NULL, 10);
	k_sleep(K_MSEC(ms));
	shell_print(sh, "delay done");

	return 0;
}
SHELL_CMD_REGISTER(delay, NULL, "delay <ms>", cmd_delay);

int main(void)
{
	return 0;
}
