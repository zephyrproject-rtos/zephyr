/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_vpd, LOG_LEVEL_INF);

#include <zephyr/sys/uuid.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/version.h>

#include "vpd.h"
#include "bmc_git_sha.h"

#ifdef CONFIG_SHELL
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

static int cmd_vpd_show(const struct shell *sh, size_t argc, char **argv)
{
	const char *uuid;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = get_bmc_uuid_string(&uuid);
	if (ret < 0) {
		shell_error(sh, "Could not get BMC UUID (err=%d)", ret);
		return ret;
	}

	shell_print(sh, "BMC UUID: %s", uuid);
	shell_print(sh, "BMC version: %s", PROJECT_GIT_SHA);
	shell_print(sh, "BMC Zephyr version: %s", BANNER_VERSION);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_vpd_cmds,
	SHELL_CMD(show, NULL, "Show VPD.", cmd_vpd_show),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(vpd, &sub_vpd_cmds, "Vital product data", NULL);
#endif /* CONFIG_SHELL */

#define BMC_UUID_FALLBACK "58893887-8974-2487-2389-389233423423"

#ifdef CONFIG_BMC_APP_BMC_UUID
static char bmc_uuid_str[UUID_STR_LEN] = BMC_UUID_FALLBACK;
#else
static const char *bmc_uuid_str = BMC_UUID_FALLBACK;
#endif

int get_bmc_uuid_string(const char **str)
{
	*str = bmc_uuid_str;
	return 0;
}

int vpd_init(void)
{
#ifdef CONFIG_BMC_APP_BMC_UUID
	struct uuid uuid_v5_ns;
	struct uuid bmc_uuid;
	uint8_t mcu_uid[16];
	ssize_t length;
	int ret;

	ret = uuid_from_string(CONFIG_BMC_UUID_NS, &uuid_v5_ns);
	if (ret < 0) {
		LOG_ERR("Could not parse namespace UUID %s", CONFIG_BMC_UUID_NS);
		return -EINVAL;
	}

	length = hwinfo_get_device_id(mcu_uid, sizeof(mcu_uid));
	if (length <= 0) {
		LOG_WRN("Could not get device UID (err=%d), using fallback BMC UUID",
			(int)length);
		return 0;
	}

	ret = uuid_generate_v5(&uuid_v5_ns, mcu_uid, length, &bmc_uuid);
	if (ret < 0) {
		LOG_ERR("Could not generate device UUID");
		return -EINVAL;
	}

	ret = uuid_to_string(&bmc_uuid, bmc_uuid_str);
	if (ret < 0) {
		LOG_ERR("Could not convert UUID to string");
		return -EINVAL;
	}
#endif
	return 0;
}
