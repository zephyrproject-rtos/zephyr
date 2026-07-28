/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/shell/shell.h>

#include <nwp_fw_version.h>
#include <sl_wifi.h>

static int cmd_nwp_version(const struct shell *sh, size_t argc, char **argv)
{
	sl_wifi_firmware_version_t version;
	bool check = argc > 1 && strcmp(argv[1], "-c") == 0;
	int ret;

	ret = sl_wifi_get_firmware_version(&version);
	if (ret != SL_STATUS_OK) {
		shell_error(sh, "Failed to get NWP firmware version");
		return -EIO;
	}

	shell_print(sh, "NWP firmware version: %d.%d.%d.%d.%d.%d.%d",
		    version.rom_id, version.major, version.minor,
		    version.security_version, version.patch_num,
		    version.customer_id, version.build_num);

	if (!check) {
		return 0;
	}

	shell_print(sh, "Expected:             %d.%d.%d.%d.%d.%d.%d",
		    siwx91x_nwp_fw_expected_version.rom_id,
		    siwx91x_nwp_fw_expected_version.major,
		    siwx91x_nwp_fw_expected_version.minor,
		    siwx91x_nwp_fw_expected_version.security_version,
		    siwx91x_nwp_fw_expected_version.patch_num,
		    siwx91x_nwp_fw_expected_version.customer_id,
		    siwx91x_nwp_fw_expected_version.build_num);

	if (siwx91x_nwp_fw_expected_version.major != version.major ||
	    siwx91x_nwp_fw_expected_version.minor != version.minor ||
	    siwx91x_nwp_fw_expected_version.security_version != version.security_version) {
		shell_error(sh, "NWP firmware version mismatch");
		return -EINVAL;
	}

	return 0;
}

SHELL_SUBCMD_ADD((silabs), nwp_version, NULL,
		 SHELL_HELP("Print NWP firmware version", "[-c]"),
		 cmd_nwp_version, 1, 1);
