/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * FPGA manager mailbox client shell to communicate sip_svc service.
 */

#include <zephyr/fpga_manager/fpga_manager.h>
#include <zephyr/shell/shell.h>

static int32_t cmd_fpga_load(const struct shell *sh, size_t argc, char **argv)
{
	int32_t err = 0;
	uint32_t config_type = 0;
	char *endptr;

	config_type = (uint32_t)strtol(argv[2], &endptr, 10);
	if (*endptr != '\0') {
		shell_error(sh, "Please provide correct configuration type");
		return -EINVAL;
	}

	err = fpga_load_file(argv[1], config_type);

	if (err) {
		if (err == -EBUSY) {
			shell_print(sh, "FPGA manager is busy !!");
		} else if (err == -ENOSR) {
			shell_print(sh, "Insufficient memory");
		} else if (err == -ENOSYS) {
			shell_print(sh, "Vendor API not implemented !!");
		} else if (err == -ENOENT) {
			shell_print(sh, "No such file or directory %s", argv[1]);
		} else if (err == -ENOTSUP) {
			shell_print(sh, "FPGA configuration not supported");
		} else {
			shell_print(sh, "Failed to start the reconfiguration");
		}
	}

	return err;
}

static int32_t cmd_fpga_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int32_t ret = 0;

	void *config_status = k_malloc(FPGA_RECONFIG_STATUS_BUF_SIZE);

	if (!config_status) {
		shell_print(sh, "Failed to allocate the memory");
		return -EFAULT;
	}

	ret = fpga_get_status(config_status);
	if (ret) {
		if (ret == -ECANCELED) {
			shell_print(sh, "Failed to get the status");
		} else if (ret == -EBUSY) {
			shell_print(sh, "FPGA manager is busy !!");
		} else if (ret == -ENOSYS) {
			shell_print(sh, "Vendor API not implemented !!");
		} else if (ret == -ENOMEM) {
			shell_print(sh, "Invalid Memory Address");
		} else {
			shell_print(sh, "Failed to open connection");
		}
	} else {
		shell_print(sh, "%s", (char *)config_status);
	}

	k_free(config_status);
	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_fpga_manager,
			       SHELL_CMD_ARG(load, NULL,
					     "Configure FPGA <filename> [type 0:FULL 1:PARTIAL]",
					     cmd_fpga_load, 3, 0),
			       SHELL_CMD_ARG(status, NULL, "Get FPGA configuration status",
					     cmd_fpga_status, 1, 0),
			       SHELL_SUBCMD_SET_END /* Array terminated */
);

SHELL_CMD_REGISTER(fpga_manager, &sub_fpga_manager, "FPGA manager commands", NULL);
