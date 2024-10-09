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
#include <zephyr/mgmt/hawkbit/hawkbit.h>
#include <zephyr/mgmt/hawkbit/config.h>
#include <zephyr/mgmt/hawkbit/autohandler.h>
#include "hawkbit_firmware.h"
#include "hawkbit_device.h"

LOG_MODULE_DECLARE(hawkbit, CONFIG_HAWKBIT_LOG_LEVEL);

static void cmd_run(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	LOG_INF("Run started from %s", sh->name);
	shell_info(sh, "Starting hawkBit run...");

	hawkbit_autohandler(false);

	switch (hawkbit_autohandler_wait(UINT32_MAX, K_FOREVER)) {
	case HAWKBIT_UNCONFIRMED_IMAGE:
		shell_error(sh, "Image is unconfirmed."
				"Rebooting to revert back to previous confirmed image");
		break;

	case HAWKBIT_NO_UPDATE:
		shell_info(sh, "No update found");
		break;

	case HAWKBIT_UPDATE_INSTALLED:
		shell_info(sh, "Update installed");
		break;

	case HAWKBIT_DOWNLOAD_ERROR:
		shell_error(sh, "Download error");
		break;

	case HAWKBIT_NETWORKING_ERROR:
		shell_error(sh, "Networking error");
		break;

	case HAWKBIT_METADATA_ERROR:
		shell_error(sh, "Metadata error");
		break;

	case HAWKBIT_NOT_INITIALIZED:
		shell_error(sh, "hawkBit not initialized");
		break;

	default:
		shell_error(sh, "Invalid response");
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
	shell_print(sh, "Unique device id: %s", device_id);
	shell_print(sh, "Firmware Version: %s", firmware_version);
	shell_print(sh, "Server address: %s", hawkbit_get_server_addr());
	shell_print(sh, "Server port: %d", hawkbit_get_server_port());
	shell_print(sh, "DDI security token: %s",
		    (IS_ENABLED(CONFIG_HAWKBIT_DDI_NO_SECURITY)
			     ? "<disabled>"
			     : hawkbit_get_ddi_security_token()));

	return 0;
}

static int cmd_init(const struct shell *sh, size_t argc, char *argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_info(sh, "Init hawkBit ...");

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
		shell_error(sh, "Invalid number of arguments");
		return -EINVAL;
	}

	hawkbit_set_server_addr(argv[1]);

	return 0;
}

static int cmd_set_port(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Invalid number of arguments");
		return -EINVAL;
	}

	hawkbit_set_server_port(atoi(argv[1]));

	return 0;
}

#ifndef CONFIG_HAWKBIT_DDI_NO_SECURITY
static int cmd_set_token(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		shell_error(sh, "Invalid number of arguments");
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
