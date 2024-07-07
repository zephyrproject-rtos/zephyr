/*
 * Copyright (c) 2023 Glenn Andrews
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include "smf_calculator_thread.h"
#include <stdlib.h>

static int cmd_smf_key(const struct shell *sh, size_t argc, char **argv)
{
	struct calculator_event event;
	char c = argv[1][0];

	event.operand = c;

	if ((c >= '1') && (c <= '9')) {
		event.event_id = DIGIT_1_9;
	} else if (c == '0') {
		event.event_id = DIGIT_0;
	} else if (c == '.') {
		event.event_id = DECIMAL_POINT;
	} else if ((c == '+') || (c == '-') || (c == '*') || (c == '/')) {
		event.event_id = OPERATOR;
	} else if (c == '=') {
		event.event_id = EQUALS;
	} else if ((c == 'C') || (c == 'c')) {
		event.event_id = CANCEL_BUTTON;
	} else if ((c == 'E') || (c == 'E')) {
		event.event_id = CANCEL_ENTRY;
	} else {
		shell_error(sh, "Invalid argument %s", argv[1]);
		return -1;
	}

	int rc = post_calculator_event(&event, K_FOREVER);

	if (rc != 0) {
		shell_error(sh, "error posting key: %d", rc);
		return rc;
	}

	shell_print(sh, "Key %c posted", c);
	return 0;
}

SHELL_CMD_ARG_REGISTER(key, NULL,
		       "Send keypress to calculator\r\n"
		       "Valid keys 0-9, +-*/, '.', '=', 'E' (cancel entry) and C (clear)",
		       cmd_smf_key, 2, 0);
