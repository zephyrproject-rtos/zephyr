/*
 * Copyright (c) 2023 Glenn Andrews
 * State Machine example copyright (c) Miro Samek
 *
 * Implementation of the statechart in Figure 2.11 of
 * Practical UML Statecharts in C/C++, 2nd Edition by Miro Samek
 * https://www.state-machine.com/psicc2
 * Used with permission of the author.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include "hsm_psicc2_thread.h"
#include <stdlib.h>
#include <ctype.h>

static int cmd_hsm_psicc2_event(const struct shell *sh, size_t argc, char **argv)
{
	struct hsm_psicc2_event event;
	int event_id = toupper(argv[1][0]);

	switch (event_id) {
	case 'A':
		event.event_id = EVENT_A;
		break;
	case 'B':
		event.event_id = EVENT_B;
		break;
	case 'C':
		event.event_id = EVENT_C;
		break;
	case 'D':
		event.event_id = EVENT_D;
		break;
	case 'E':
		event.event_id = EVENT_E;
		break;
	case 'F':
		event.event_id = EVENT_F;
		break;
	case 'G':
		event.event_id = EVENT_G;
		break;
	case 'H':
		event.event_id = EVENT_H;
		break;
	case 'I':
		event.event_id = EVENT_I;
		break;
	default:
		shell_error(sh, "Invalid argument %s", argv[1]);
		return -1;
	}

	int rc = k_msgq_put(&hsm_psicc2_msgq, &event, K_NO_WAIT);

	if (rc == 0) {
		shell_print(sh, "Event %c posted", event_id);
	} else {
		shell_error(sh, "error posting event: %d", rc);
	}
	return rc;
}

static int cmd_hsm_psicc2_terminate(const struct shell *sh, size_t argc, char **argv)
{
	struct hsm_psicc2_event event = {.event_id = EVENT_TERMINATE};
	int rc = k_msgq_put(&hsm_psicc2_msgq, &event, K_NO_WAIT);

	if (rc == 0) {
		shell_print(sh, "Terminate event posted");
	} else {
		shell_error(sh, "error posting terminate event: %d", rc);
	}
	return rc;
}

/* Creating subcommands (level 1 command) array for command "demo". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_hsm_psicc2,
			       SHELL_CMD_ARG(event, NULL, "Send event to State Machine",
					     cmd_hsm_psicc2_event, 2, 0),
			       SHELL_CMD_ARG(terminate, NULL,
					     "Send terminate event to State Machine",
					     cmd_hsm_psicc2_terminate, 1, 0),
			       SHELL_SUBCMD_SET_END);

/* Creating root (level 0) command "demo" */
SHELL_CMD_REGISTER(hsm_psicc2, &sub_hsm_psicc2, "PSICC2 demo hierarchical state machine commands",
		   NULL);
