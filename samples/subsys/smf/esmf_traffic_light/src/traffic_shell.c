/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>

#include <errno.h>
#include <string.h>

#include "traffic_events.h"

static int cmd_traffic_event(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	if (strcmp(argv[1], "timer") == 0) {
		if (traffic_post_event(TRIG_TIMER, "timer(shell)", K_NO_WAIT) != 0) {
			shell_error(sh, "event queue full");
			return -EBUSY;
		}
	} else if (strcmp(argv[1], "ped") == 0) {
		if (traffic_post_event(TRIG_PED_BUTTON, "ped(shell)", K_NO_WAIT) != 0) {
			shell_error(sh, "event queue full");
			return -EBUSY;
		}
	} else if (strcmp(argv[1], "emergency") == 0) {
		if (traffic_post_event(TRIG_EMERGENCY, "emergency(shell)", K_NO_WAIT) != 0) {
			shell_error(sh, "event queue full");
			return -EBUSY;
		}
	} else if (strcmp(argv[1], "diag") == 0) {
		if (traffic_post_event(TRIG_DIAG_TICK, "diag(shell)", K_NO_WAIT) != 0) {
			shell_error(sh, "event queue full");
			return -EBUSY;
		}
	} else if (strcmp(argv[1], "reset") == 0) {
		if (traffic_post_event(TRIG_RESET, "reset(shell)", K_NO_WAIT) != 0) {
			shell_error(sh, "event queue full");
			return -EBUSY;
		}
	} else {
		shell_error(sh, "unknown event '%s'", argv[1]);
		return -EINVAL;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(traffic_subcmds,
			       SHELL_CMD_ARG(event, NULL,
					     "Send event: timer | ped | emergency | diag | reset",
					     cmd_traffic_event, 2, 0),
			       SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(traffic, &traffic_subcmds, "Traffic light ESMF demo commands", NULL);
