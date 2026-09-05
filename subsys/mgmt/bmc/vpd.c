/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/sys/uuid.h>
#include <zephyr/version.h>

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

/* Taken from zephyr/kernel/banner.c */
#if defined(BUILD_VERSION) && !IS_EMPTY(BUILD_VERSION)
#define BMC_BANNER_VERSION STRINGIFY(BUILD_VERSION)
#else
#define BMC_BANNER_VERSION KERNEL_VERSION_STRING
#endif

/*
 * Used when the SoC has no readable hardware identifier, or when
 * CONFIG_BMC_UUID is disabled.
 */
#define BMC_UUID_FALLBACK "58893887-8974-2487-2389-389233423423"

#if defined(CONFIG_BMC_UUID)
static char bmc_uuid_str[UUID_STR_LEN] = BMC_UUID_FALLBACK;
#else
static const char bmc_uuid_str[] = BMC_UUID_FALLBACK;
#endif

const char *bmc_uuid_get(void)
{
	return bmc_uuid_str;
}

const char *bmc_firmware_version(void)
{
	return BMC_BANNER_VERSION;
}

#if defined(CONFIG_BMC_UUID)
static int bmc_vpd_init(void)
{
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
		LOG_ERR("Could not generate device UUID (err=%d)", ret);
		return ret;
	}

	ret = uuid_to_string(&bmc_uuid, bmc_uuid_str);
	if (ret < 0) {
		LOG_ERR("Could not convert UUID to string (err=%d)", ret);
		return ret;
	}

	return 0;
}

BMC_COMPONENT_DEFINE(bmc_vpd, BMC_INIT_PHASE_PLATFORM, bmc_vpd_init, true);
#endif /* CONFIG_BMC_UUID */

#if defined(CONFIG_SHELL)
#include <zephyr/shell/shell.h>

static int cmd_bmc_vpd_show(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "BMC UUID: %s", bmc_uuid_get());
	shell_print(sh, "BMC firmware version: %s", bmc_firmware_version());

	return 0;
}

SHELL_SUBCMD_SET_CREATE(bmc_vpd_subcmds, (bmc, vpd));
SHELL_SUBCMD_ADD((bmc, vpd), show, NULL, "Show vital product data.", cmd_bmc_vpd_show, 1, 0);
SHELL_SUBCMD_ADD((bmc), vpd, &bmc_vpd_subcmds, "Vital product data.", NULL, 1, 0);
#endif /* CONFIG_SHELL */
