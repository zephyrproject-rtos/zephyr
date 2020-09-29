/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <drivers/flash.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>
#include <power/reboot.h>
#include "mgmt/hawkbit.h"
#include "hawkbit_firmware.h"
#include "hawkbit_device.h"

static void cmd_run(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_INFO, "Starting Hawkbit run...\n");
	switch (hawkbit_probe()) {
	case HAWKBIT_UNCONFIRMED_IMAGE:
		shell_fprintf(
			shell, SHELL_ERROR,
			"Image is unconfirmed."
			"Rebooting to revert back to previous confirmed image\n");
		sys_reboot(SYS_REBOOT_WARM);
		break;

	case HAWKBIT_CANCEL_UPDATE:
		shell_fprintf(shell, SHELL_INFO,
			      "Hawkbit update Cancelled from server\n");
		break;

	case HAWKBIT_NO_UPDATE:
		shell_fprintf(shell, SHELL_INFO, "No update found\n");
		break;

	case HAWKBIT_OK:
		shell_fprintf(shell, SHELL_INFO, "Image Already updated\n");
		break;

	case HAWKBIT_UPDATE_INSTALLED:
		shell_fprintf(shell, SHELL_INFO, "Update Installed\n");
		break;

	case HAWKBIT_DOWNLOAD_ERROR:
		shell_fprintf(shell, SHELL_INFO, "Download Error\n");
		break;

	case HAWKBIT_NETWORKING_ERROR:
		shell_fprintf(shell, SHELL_INFO, "Networking Error\n");
		break;

	case HAWKBIT_METADATA_ERROR:
		shell_fprintf(shell, SHELL_INFO, "Metadata Error\n");
		break;

	default:
		shell_fprintf(shell, SHELL_ERROR, "Invalid response\n");
		break;
	}
	k_sleep(K_MSEC(1));
}

static int cmd_info(const struct shell *shell, size_t argc, char *argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	char device_id[DEVICE_ID_HEX_MAX_SIZE] = {0},
	     firmware_version[BOOT_IMG_VER_STRLEN_MAX] = {0};

	hawkbit_get_firmware_version(firmware_version, BOOT_IMG_VER_STRLEN_MAX);
	hawkbit_get_device_identity(device_id, DEVICE_ID_HEX_MAX_SIZE);

	shell_fprintf(shell, SHELL_NORMAL, "Unique device id: %s\n", device_id);
	shell_fprintf(shell, SHELL_NORMAL, "Firmware Version: %s\n",
		      firmware_version);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_hawkbit,
	SHELL_CMD(info, NULL, "Dump Hawkbit information", cmd_info),
	SHELL_CMD(run, NULL, "Trigger an Hawkbit update run", cmd_run),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(hawkbit, &sub_hawkbit, "Hawkbit commands", NULL);
