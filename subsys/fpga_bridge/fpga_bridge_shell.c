/*
 * Copyright (c) 2023, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * FPGA-HPS bridge client shell to communicate with sip_svc service
 * to enable/disable the bridges.
 */

#include <zephyr/shell/shell.h>
#include <zephyr/fpga_bridge/bridge.h>

#define BASE			10u
#define ENABLE_STRING	"enable"
#define DISABLE_STRING	"disable"

static int32_t do_bridge(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t mask = BRIDGES_MASK;
	int32_t ret = 0;

	if (argc == 2) {
		int error;

		mask = (uint32_t)shell_strtol(argv[1], BASE, &error);
		if (error != 0) {
			shell_error(sh, "Failed to parse offset: %d", error);
			return error;
		}
	}

	if (!strcmp(argv[0], ENABLE_STRING)) {
		ret = do_bridge_reset(1, mask);
		if (!ret)
			shell_print(sh, "Bridge enable success");
	} else if (!strcmp(argv[0], DISABLE_STRING)) {
		ret = do_bridge_reset(0, mask);
		if (!ret)
			shell_print(sh, "Bridge disable success");
	} else {
		return -EINVAL;
	}

	/* FPGA operation status */
	if (ret) {
		if (ret == -EBUSY) {
			shell_print(sh, "FPGA not ready. Bridge reset aborted!");
		} else if (ret == -ENOMSG) {
			shell_print(sh, "Bridge reset failed");
		} else if (ret == -EIO) {
			shell_print(sh, "FPGA not configured");
		} else if (ret == -ENOTSUP) {
			shell_print(sh, "Please provide mask in correct range");
		} else {
			shell_print(sh, "Failed");
		}
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_bridge,
			       SHELL_CMD_ARG(enable, NULL, "enable [mask]", do_bridge, 1, 1),
			       SHELL_CMD_ARG(disable, NULL, "disable [mask]", do_bridge, 1, 1),
			       SHELL_SUBCMD_SET_END /* Array terminated */
);

SHELL_CMD_REGISTER(bridge, &sub_bridge, "FPGA bridge commands", NULL);
