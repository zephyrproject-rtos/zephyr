/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/config.h>
#include <zephyr/mgmt/bmc/host.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/reboot.h>

#include "bmc_internal.h"

LOG_MODULE_REGISTER(bmc, CONFIG_BMC_LOG_LEVEL);

static bool boot_finished;

bool bmc_is_boot_finished(void)
{
	return boot_finished;
}

static void bmc_shutdown_prepare(void)
{
	/* Flush buffered logs when the active logging mode supports it. */
	LOG_PANIC();
	k_msleep(100);
}

FUNC_NORETURN void bmc_reboot(void)
{
	LOG_WRN("Rebooting BMC");
	bmc_shutdown_prepare();

	/*
	 * Not all platforms support all reboot types, so try a warm reboot
	 * first and fall back to a cold one.
	 */
	sys_reboot(SYS_REBOOT_WARM);
	sys_reboot(SYS_REBOOT_COLD);
	k_panic();

	for (;;) {
	}
}

FUNC_NORETURN void bmc_poweroff(void)
{
	LOG_WRN("Powering off BMC");
	bmc_shutdown_prepare();

#if defined(CONFIG_POWEROFF)
	sys_poweroff();
	k_panic();
#endif

	for (;;) {
	}
}

int bmc_init(void)
{
	LOG_INF("Zephyr OS build: %s", bmc_firmware_version());

	for (uint8_t phase = 0; phase < BMC_INIT_PHASE_COUNT; phase++) {
		STRUCT_SECTION_FOREACH(bmc_component, component) {
			int ret;

			if (component->phase != phase) {
				continue;
			}

			LOG_DBG("Initialising %s", component->name);

			ret = component->init();
			if (ret == 0) {
				continue;
			}

			if (component->optional) {
				LOG_WRN("%s init failed (err=%d), continuing",
					component->name, ret);
				continue;
			}

			LOG_ERR("%s init failed (err=%d)", component->name, ret);
			return ret;
		}
	}

	/*
	 * Applied only once every component has had a chance to install its
	 * host backend, so that an application override takes effect.
	 */
	if (bmc_config_host_auto_poweron()) {
		int ret = bmc_host_power_set(true);

		if (ret < 0 && ret != -ENOTSUP) {
			LOG_WRN("Host auto power-on failed (err=%d)", ret);
		}
	}

	/* Late enough that an application authentication backend counts. */
	bmc_auth_warn_default_password();

	boot_finished = true;
	bmc_event_notify(BMC_EVENT_BOOT_DONE, NULL, 0);

	LOG_INF("BMC boot complete");

	return 0;
}

#if defined(CONFIG_SHELL)
#include <zephyr/shell/shell.h>

static int cmd_bmc_reboot(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	bmc_reboot();
}

static int cmd_bmc_poweroff(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	bmc_poweroff();
}

SHELL_SUBCMD_SET_CREATE(bmc_subcmds, (bmc));
SHELL_SUBCMD_ADD((bmc), reboot, NULL, "Reboot the BMC.", cmd_bmc_reboot, 1, 0);
SHELL_SUBCMD_ADD((bmc), poweroff, NULL, "Power off the BMC.", cmd_bmc_poweroff, 1, 0);
SHELL_CMD_REGISTER(bmc, &bmc_subcmds, "Baseboard Management Controller commands", NULL);
#endif /* CONFIG_SHELL */
