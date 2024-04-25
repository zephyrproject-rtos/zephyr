/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/mgmt/hawkbit.h>
#include "hawkbit_firmware.h"
#include "hawkbit_device.h"

static void cmd_run(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(sh, SHELL_INFO, "Starting hawkBit run...\n");
	switch (hawkbit_probe()) {
	case HAWKBIT_UNCONFIRMED_IMAGE:
		shell_fprintf(
			sh, SHELL_ERROR,
			"Image is unconfirmed."
			"Rebooting to revert back to previous confirmed image\n");
		hawkbit_reboot();
		break;

	case HAWKBIT_CANCEL_UPDATE:
		shell_fprintf(sh, SHELL_INFO,
			      "hawkBit update Cancelled from server\n");
		break;

	case HAWKBIT_NO_UPDATE:
		shell_fprintf(sh, SHELL_INFO, "No update found\n");
		break;

	case HAWKBIT_OK:
		shell_fprintf(sh, SHELL_INFO, "Image Already updated\n");
		break;

	case HAWKBIT_UPDATE_INSTALLED:
		shell_fprintf(sh, SHELL_INFO, "Update Installed\n");
		hawkbit_reboot();
		break;

	case HAWKBIT_DOWNLOAD_ERROR:
		shell_fprintf(sh, SHELL_INFO, "Download Error\n");
		break;

	case HAWKBIT_NETWORKING_ERROR:
		shell_fprintf(sh, SHELL_INFO, "Networking Error\n");
		break;

	case HAWKBIT_METADATA_ERROR:
		shell_fprintf(sh, SHELL_INFO, "Metadata Error\n");
		break;

	case HAWKBIT_NOT_INITIALIZED:
		shell_fprintf(sh, SHELL_ERROR, "hawkBit not initialized\n");
		break;

	default:
		shell_fprintf(sh, SHELL_ERROR, "Invalid response\n");
		break;
	}
	k_sleep(K_MSEC(1));
}

static int cmd_info(const struct shell *sh, size_t argc, char *argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	char device_id[DEVICE_ID_HEX_MAX_SIZE] = {0},
	     firmware_version[BOOT_IMG_VER_STRLEN_MAX] = {0};

	hawkbit_get_firmware_version(firmware_version, BOOT_IMG_VER_STRLEN_MAX);
	hawkbit_get_device_identity(device_id, DEVICE_ID_HEX_MAX_SIZE);

	shell_print(sh, "Action id: %d", hawkbit_get_action_id());
	shell_fprintf(sh, SHELL_NORMAL, "Unique device id: %s\n", device_id);
	shell_fprintf(sh, SHELL_NORMAL, "Firmware Version: %s\n",
		      firmware_version);
	shell_fprintf(sh, SHELL_NORMAL, "Server address: %s\n", hawkbit_get_server_addr());
	shell_fprintf(sh, SHELL_NORMAL, "Server port: %d\n", hawkbit_get_server_port());
	shell_fprintf(sh, SHELL_NORMAL, "DDI security token: %s\n",
		      (IS_ENABLED(CONFIG_HAWKBIT_DDI_NO_SECURITY)
			       ? "<disabled>"
			       : hawkbit_get_ddi_security_token()));

	return 0;
}

static int cmd_init(const struct shell *sh, size_t argc, char *argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(sh, SHELL_INFO, "Init hawkBit ...\n");

	hawkbit_init();

	return 0;
}

static int cmd_reset(const struct shell *sh, size_t argc, char *argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = hawkbit_reset_action_id();

	shell_print(sh, "Reset action id %s", (ret == 0) ? "success" : "failed");

	return 0;
}

#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME

static int cmd_set_addr(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_fprintf(sh, SHELL_ERROR, "Invalid number of arguments\n");
		return -EINVAL;
	}

	hawkbit_set_server_addr(argv[1]);

	return 0;
}

static int cmd_set_port(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_fprintf(sh, SHELL_ERROR, "Invalid number of arguments\n");
		return -EINVAL;
	}

	hawkbit_set_server_port(atoi(argv[1]));

	return 0;
}

#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
static int cmd_set_token(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_fprintf(sh, SHELL_ERROR, "Invalid number of arguments\n");
		return -EINVAL;
	}

	hawkbit_set_ddi_security_token(argv[1]);

	return 0;
}
#endif /* CONFIG_HAWKBIT_DDI_NO_SECURITY */

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_hawkbit_set,
	SHELL_CMD(addr, NULL, "Set hawkBit server address", cmd_set_addr),
	SHELL_CMD(port, NULL, "Set hawkBit server port", cmd_set_port),
#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
	SHELL_CMD(ddi_token, NULL, "Set hawkBit DDI Security token", cmd_set_token),
#endif
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME */

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_hawkbit,
	SHELL_CMD(info, NULL, "Dump hawkBit information", cmd_info),
	SHELL_CMD(init, NULL, "Initialize hawkBit", cmd_init),
	SHELL_CMD(run, NULL, "Trigger an hawkBit update run", cmd_run),
	SHELL_CMD(reset, NULL, "Reset the hawkBit action id", cmd_reset),
#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME
	SHELL_CMD(set, &sub_hawkbit_set, "Set hawkBit settings", NULL),
#endif
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(hawkbit, &sub_hawkbit, "hawkBit commands", NULL);
