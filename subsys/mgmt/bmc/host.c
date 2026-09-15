/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/host.h>

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

static const struct bmc_host_ops *host_ops;

int bmc_host_ops_register(const struct bmc_host_ops *ops)
{
	host_ops = ops;

	return 0;
}

int bmc_host_power_set(bool on)
{
	int ret;

	if (host_ops == NULL || host_ops->power_set == NULL) {
		return -ENOTSUP;
	}

	ret = host_ops->power_set(on);
	if (ret < 0) {
		LOG_ERR("Could not set host power state (err=%d)", ret);
		return ret;
	}

	LOG_INF("Host power state changed to %s", on ? "ON" : "OFF");
	bmc_event_notify(BMC_EVENT_HOST_POWER_CHANGED, &on, sizeof(on));

	return 0;
}

bool bmc_host_power_get(void)
{
	bool on = false;

	if (host_ops == NULL || host_ops->power_get == NULL) {
		return false;
	}

	if (host_ops->power_get(&on) < 0) {
		return false;
	}

	return on;
}

int bmc_host_reset(void)
{
	if (host_ops == NULL || host_ops->reset == NULL) {
		return -ENOTSUP;
	}

	return host_ops->reset();
}

int bmc_host_status_led_set(bool on)
{
	if (host_ops == NULL || host_ops->status_led_set == NULL) {
		return -ENOTSUP;
	}

	return host_ops->status_led_set(on);
}

#if defined(CONFIG_SHELL)
#include <zephyr/shell/shell.h>

static int cmd_bmc_host_on(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = bmc_host_power_set(true);
	if (ret < 0) {
		shell_error(sh, "Could not power on the host (err=%d)", ret);
	}

	return ret;
}

static int cmd_bmc_host_off(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = bmc_host_power_set(false);
	if (ret < 0) {
		shell_error(sh, "Could not power off the host (err=%d)", ret);
	}

	return ret;
}

static int cmd_bmc_host_reset(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	ret = bmc_host_reset();
	if (ret < 0) {
		shell_error(sh, "Could not reset the host (err=%d)", ret);
	}

	return ret;
}

static int cmd_bmc_host_show(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Host power state: %s", bmc_host_power_get() ? "on" : "off");

	return 0;
}

SHELL_SUBCMD_SET_CREATE(bmc_host_subcmds, (bmc, host));
SHELL_SUBCMD_ADD((bmc, host), on, NULL, "Power on the host.", cmd_bmc_host_on, 1, 0);
SHELL_SUBCMD_ADD((bmc, host), off, NULL, "Power off the host.", cmd_bmc_host_off, 1, 0);
SHELL_SUBCMD_ADD((bmc, host), reset, NULL, "Reset the host.", cmd_bmc_host_reset, 1, 0);
SHELL_SUBCMD_ADD((bmc, host), show, NULL, "Show host state.", cmd_bmc_host_show, 1, 0);
SHELL_SUBCMD_ADD((bmc), host, &bmc_host_subcmds, "Host control commands.", NULL, 1, 0);
#endif /* CONFIG_SHELL */
