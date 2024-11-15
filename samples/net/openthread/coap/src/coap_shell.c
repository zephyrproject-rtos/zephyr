/*
 * Copyright (c) 2024 Alexandre Bailon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>

#include "led.h"
#include "button.h"

static int coap_cmd_led_set(const struct shell *sh, size_t argc, char **argv)
{
	int led_id;
	int led_state;
	const char *addr;
	int err = 0;

	led_id = shell_strtol(argv[1], 10, &err);
	if (err) {
		shell_error(sh, "Failed to get a valid LED id: %s", argv[1]);
		return err;
	}

	if (!strcmp(argv[2], "on")) {
		led_state = LED_MSG_STATE_ON;
	} else if (!strcmp(argv[2], "off")) {
		led_state = LED_MSG_STATE_OFF;
	} else if (!strcmp(argv[2], "toggle")) {
		led_state = LED_MSG_STATE_TOGGLE;
	} else {
		shell_error(sh, "Failed to get a valid LED state");
		return -EINVAL;
	}

	if (argc <= 3) {
		addr = "ff03::1";
	} else {
		addr = argv[3];
	}

	return coap_led_set_state(addr, led_id, led_state);
}

static int coap_cmd_led_get(const struct shell *sh, size_t argc, char **argv)
{
	int led_id;
	int led_state;
	const char *addr;
	int err = 0;

	led_id = shell_strtol(argv[1], 10, &err);
	if (err) {
		shell_error(sh, "Failed to get a valid LED id: %s", argv[1]);
		return err;
	}

	if (argc <= 3) {
		addr = "ff03::1";
	} else {
		addr = argv[3];
	}

	err = coap_led_get_state(addr, led_id, &led_state);
	if (err) {
		return err;
	}

	if (led_state == LED_MSG_STATE_ON) {
		shell_info(sh, "on");
	} else {
		shell_info(sh, "off");
	}

	return 0;
}

static int coap_cmd_btn_get(const struct shell *sh, size_t argc, char **argv)
{
	int btn_id;
	int btn_state;
	const char *addr;
	int err = 0;

	btn_id = shell_strtol(argv[1], 10, &err);
	if (err) {
		shell_error(sh, "Failed to get a valid button id: %s", argv[1]);
		return err;
	}

	if (argc <= 3) {
		addr = "ff03::1";
	} else {
		addr = argv[3];
	}

	err = coap_btn_get_state(addr, btn_id, &btn_state);
	if (err) {
		return err;
	}

	if (btn_state == BTN_MSG_STATE_ON) {
		shell_info(sh, "on");
	} else {
		shell_info(sh, "off");
	}

	return 0;
}

static const char coap_cmd_led_set_help[] =
	"Set a LED state using CoAP\n"
	"led set <led_id> <led_state> [addr]\n"
	"\tled_id: a number defining the LED to control from CoAP server\n"
	"\tled_state: on, off or toggle\n"
	"\taddr: the IPv6 address of CoAP server. If not defined, it will broadcast to all CoAP "
	"servers\n";

static const char coap_cmd_led_get_help[] =
	"Get a LED state using CoAP\n"
	"led get <led_id> [addr]\n"
	"\tled_id: a number defining the LED to get from CoAP server\n"
	"\taddr: the IPv6 address of CoAP server. If not defined, it "
	"will broadcast to all CoAP servers\n";

SHELL_STATIC_SUBCMD_SET_CREATE(
	coap_led_subcmd, SHELL_CMD_ARG(set, NULL, coap_cmd_led_set_help, coap_cmd_led_set, 3, 1),
	SHELL_CMD_ARG(get, NULL, coap_cmd_led_get_help, coap_cmd_led_get, 2, 1),
	SHELL_SUBCMD_SET_END);

static const char coap_cmd_btn_get_help[] =
	"Get a button state using CoAP\n"
	"btn get <btn_id> [addr]\n"
	"\tbtn_id: a number defining the button to get from CoAP server\n"
	"\taddr: the IPv6 address of CoAP server. If not defined, it "
	"will broadcast to all CoAP servers\n";

SHELL_STATIC_SUBCMD_SET_CREATE(coap_btn_subcmd,
			       SHELL_CMD_ARG(get, NULL, coap_cmd_btn_get_help, coap_cmd_btn_get, 2,
					     1),
			       SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(coap_subcmd,
			       SHELL_CMD(led, &coap_led_subcmd, "Manage a LED using CoAP", NULL),
			       SHELL_CMD(btn, &coap_btn_subcmd, "Manage a button using CoAP", NULL),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ot_coap, &coap_subcmd, "CoAP sample client", NULL);
