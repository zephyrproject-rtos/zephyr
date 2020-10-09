/** @file
 *  @brief Bluetooth Call Control Profile client shell
 *
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <ctype.h>
#include <zephyr.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <bluetooth/gatt.h>

#include "../host/audio/tbs.h"
#include "../host/audio/ccp.h"

#include "bt.h"

#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include "../host/audio/ccp.h"

static int cmd_ccp_discover(const struct shell *shell, size_t argc,
			    char *argv[])
{
	int result;
	int subscribe = 1;

	if (argc > 1) {
		subscribe = strtol(argv[1], NULL, 0);

		if (subscribe < 0 || subscribe > 1) {
			shell_error(shell, "Invalid parameter");
			return -ENOEXEC;
		}
	}


	result = bt_ccp_discover(default_conn, (bool)subscribe);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_ccp_set_signal_strength_interval(
	const struct shell *shell, size_t argc, char *argv[])
{
	int result;
	int interval;
	int inst_index;

	if (argc > 2) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	interval = strtol(argv[argc - 1], NULL, 0);

	if (interval < 0 || interval > UINT8_MAX) {
		shell_error(shell, "Invalid interval");
		return -ENOEXEC;
	}

	result = bt_ccp_set_signal_strength_interval(default_conn, inst_index,
						     interval);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_hold(const struct shell *shell, size_t argc, char *argv[])
{
	int result;
	int inst_index;
	int call_index;

	if (argc > 2) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	call_index = strtol(argv[argc - 1], NULL, 0);

	if (call_index < 0 || call_index > UINT8_MAX) {
		shell_error(shell, "Invalid parameter");
		return -ENOEXEC;
	}

	result = bt_ccp_hold_call(default_conn, inst_index, call_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_retrieve(const struct shell *shell, size_t argc,
			    char *argv[])
{
	int result;
	int inst_index;
	int call_index;

	if (argc > 2) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	call_index = strtol(argv[argc - 1], NULL, 0);

	if (call_index < 0 || call_index > UINT8_MAX) {
		shell_error(shell, "Invalid parameter");
		return -ENOEXEC;
	}

	result = bt_ccp_retrieve_call(default_conn, inst_index, call_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_accept(const struct shell *shell, size_t argc, char *argv[])
{
	int result;
	int inst_index;
	int call_index;

	if (argc > 2) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	call_index = strtol(argv[argc - 1], NULL, 0);

	if (call_index < 0 || call_index > UINT8_MAX) {
		shell_error(shell, "Invalid parameter");
		return -ENOEXEC;
	}

	result = bt_ccp_accept_call(default_conn, inst_index, call_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_join(const struct shell *shell, size_t argc, char *argv[])
{
	int result;
	uint8_t call_indexes[CONFIG_BT_CCP_MAX_CALLS];
	int call_index;
	int inst_index;

	if (!strcmp(argv[1], "gtbs")) {
		inst_index = BT_CCP_GTBS_INDEX;
	} else {
		inst_index = strtol(argv[1], NULL, 0);

		if (inst_index < 0 || inst_index > UINT8_MAX) {
			shell_error(shell, "Invalid index");
			return -ENOEXEC;
		}
	}

	for (int i = 2; i < argc; i++) {
		call_index = (int)strtol(argv[i], NULL, 0);
		if ((call_index < 0) || (call_index > UINT8_MAX)) {
			shell_error(shell, "Invalid parameter %s [%d]", argv[i],
				    call_index);
			return -ENOEXEC;
		}
		call_indexes[i - 2] = (uint8_t)call_index;
	}

	result = bt_ccp_join_calls(default_conn, inst_index,
				   call_indexes, argc - 2);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_terminate(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int result;
	int inst_index;
	int call_index;

	if (argc > 2) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	call_index = strtol(argv[argc - 1], NULL, 0);

	if (call_index < 0 || call_index > UINT8_MAX) {
		shell_error(shell, "Invalid parameter");
		return -ENOEXEC;
	}

	result = bt_ccp_terminate_call(default_conn, inst_index, call_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_originate(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int result;
	int inst_index;

	if (argc > 2) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_originate_call(default_conn, inst_index,
				       argv[argc - 1]);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_bearer_provider_name(const struct shell *shell,
					     size_t argc, char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_bearer_provider_name(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_bearer_uci(const struct shell *shell, size_t argc,
				   char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_bearer_uci(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_technology(const struct shell *shell, size_t argc,
				   char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_technology(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_uri_list(const struct shell *shell, size_t argc,
				 char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_uri_list(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_signal_strength(const struct shell *shell, size_t argc,
					char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_signal_strength(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_signal_interval(const struct shell *shell, size_t argc,
					char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_signal_interval(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_current_calls(const struct shell *shell, size_t argc,
				      char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_current_calls(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_ccid(const struct shell *shell, size_t argc,
			     char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_ccid(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_status_flags(const struct shell *shell, size_t argc,
				     char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_status_flags(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_uri(const struct shell *shell, size_t argc,
			    char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_remote_uri(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_call_state(const struct shell *shell, size_t argc,
				   char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_call_state(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_remote_uri(const struct shell *shell, size_t argc,
				   char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_remote_uri(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_friendly_name(const struct shell *shell, size_t argc,
				      char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_friendly_name(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp_read_optional_opcodes(const struct shell *shell, size_t argc,
					 char *argv[])
{
	int result;
	int inst_index;

	if (argc > 1) {
		if (!strcmp(argv[1], "gtbs")) {
			inst_index = BT_CCP_GTBS_INDEX;
		} else {
			inst_index = strtol(argv[1], NULL, 0);

			if (inst_index < 0 || inst_index > UINT8_MAX) {
				shell_error(shell, "Invalid index");
				return -ENOEXEC;
			}
		}
	} else {
		inst_index = 0;
	}

	result = bt_ccp_read_optional_opcodes(default_conn, inst_index);

	if (result) {
		shell_print(shell, "Fail: %d", result);
	}

	return result;
}

static int cmd_ccp(const struct shell *shell, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(shell, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(shell, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ccp_cmds,
	SHELL_CMD_ARG(discover, NULL,
		      "Discover TBS [subscribe]",
		      cmd_ccp_discover, 1, 1),
	SHELL_CMD_ARG(set_signal_reporting_interval, NULL,
		      "Set the signal reporting interval "
		      "[<{instance_index, gtbs}>] <interval>",
		      cmd_ccp_set_signal_strength_interval, 2, 1),
	SHELL_CMD_ARG(originate, NULL,
		      "Originate a call [<{instance_index, gtbs}>] <uri>",
		      cmd_ccp_originate, 2, 1),
	SHELL_CMD_ARG(terminate, NULL,
		      "terminate a call [<{instance_index, gtbs}>] <id>",
		      cmd_ccp_terminate, 2, 1),
	SHELL_CMD_ARG(accept, NULL,
		      "Accept a call [<{instance_index, gtbs}>] <id>",
		      cmd_ccp_accept, 2, 1),
	SHELL_CMD_ARG(hold, NULL,
		      "Place a call on hold [<{instance_index, gtbs}>] <id>",
		      cmd_ccp_hold, 2, 1),
	SHELL_CMD_ARG(retrieve, NULL,
		      "Retrieve a held call [<{instance_index, gtbs}>] <id>",
		      cmd_ccp_retrieve, 2, 1),
#if CONFIG_BT_CCP_MAX_CALLS > 1
	SHELL_CMD_ARG(join, NULL,
		      "Join calls <{instance_index, gtbs}> "
		      "<id> <id> [<id> [<id> [...]]]",
		      cmd_ccp_join, 4, CONFIG_BT_CCP_MAX_CALLS - 2),
#endif /* CONFIG_BT_CCP_MAX_CALLS > 1 */
	SHELL_CMD_ARG(read_provider_name, NULL,
		      "Read the bearer name [<{instance_index, gtbs}>]",
		      cmd_ccp_read_bearer_provider_name, 1, 1),
	SHELL_CMD_ARG(read_bearer_uci, NULL,
		      "Read the bearer UCI [<{instance_index, gtbs}>]",
		      cmd_ccp_read_bearer_uci, 1, 1),
	SHELL_CMD_ARG(read_technology, NULL,
		      "Read the bearer technology [<{instance_index, gtbs}>]",
		      cmd_ccp_read_technology, 1, 1),
	SHELL_CMD_ARG(read_uri_list, NULL,
		      "Read the bearer's supported URI list "
		      "[<{instance_index, gtbs}>]",
		      cmd_ccp_read_uri_list, 1, 1),
	SHELL_CMD_ARG(read_signal_strength, NULL,
		      "Read the bearer signal strength "
		      "[<{instance_index, gtbs}>]",
		      cmd_ccp_read_signal_strength, 1, 1),
	SHELL_CMD_ARG(read_signal_interval, NULL,
		      "Read the bearer signal strength reporting interval "
		      "[<{instance_index, gtbs}>]",
		      cmd_ccp_read_signal_interval, 1, 1),
	SHELL_CMD_ARG(read_current_calls, NULL,
		      "Read the current calls [<{instance_index, gtbs}>]",
		      cmd_ccp_read_current_calls, 1, 1),
	SHELL_CMD_ARG(read_ccid, NULL,
		      "Read the CCID [<{instance_index, gtbs}>]",
		      cmd_ccp_read_ccid, 1, 1),
	SHELL_CMD_ARG(read_status_flags, NULL,
		      "Read the in feature and status value "
		      "[<{instance_index, gtbs}>]",
		      cmd_ccp_read_status_flags, 1, 1),
	SHELL_CMD_ARG(read_uri, NULL,
		      "Read the incoming call target URI "
		      "[<{instance_index, gtbs}>]",
		      cmd_ccp_read_uri, 1, 1),
	SHELL_CMD_ARG(read_call_state, NULL,
		      "Read the call state [<{instance_index, gtbs}>]",
		      cmd_ccp_read_call_state, 1, 1),
	SHELL_CMD_ARG(read_remote_uri, NULL,
		      "Read the incoming remote URI [<{instance_index, gtbs}>]",
		      cmd_ccp_read_remote_uri, 1, 1),
	SHELL_CMD_ARG(read_friendly_name, NULL,
		      "Read the friendly name of an incoming call "
		      "[<{instance_index, gtbs}>]",
		      cmd_ccp_read_friendly_name, 1, 1),
	SHELL_CMD_ARG(read_optional_opcodes, NULL,
		      "Read the optional opcodes [<{instance_index, gtbs}>]",
		      cmd_ccp_read_optional_opcodes, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(ccp, &ccp_cmds, "Bluetooth CCP shell commands",
		       cmd_ccp, 1, 1);
