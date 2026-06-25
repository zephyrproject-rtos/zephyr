/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/haptics/cs40l26.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/shell/shell.h>

#if CONFIG_SHELL
#include <stdlib.h>
#endif /* CONFIG_SHELL */

const struct device *cs40l26 = DEVICE_DT_GET(DT_ALIAS(haptic0));

#define CS40L26_DEMO_FREQUENCY 165
#define CS40L26_DEMO_LEVEL     64
#define CS40L26_DEMO_DURATION  1000

#if CONFIG_SHELL
#define CS40L26_HELP                SHELL_HELP("CS40L26 haptics commands", NULL)
#define CS40L26_CONFIGURE_BUZZ_HELP SHELL_HELP("Configure buzz output", "<freq> <level> <dur>")

static int cmd_configure_buzz(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t duration, frequency, level;

	frequency = strtoul(argv[1], NULL, 10);
	level = strtoul(argv[2], NULL, 10);
	duration = strtoul(argv[3], NULL, 10);

	if (level > UINT8_MAX) {
		shell_error(sh, "Level must be between 0 and 255");
		return -EINVAL;
	}

	return cs40l26_configure_buzz(cs40l26, frequency, (uint8_t)level, duration);
}

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(cs40l26_cmds,
	SHELL_CMD_ARG(configure_buzz, NULL, CS40L26_CONFIGURE_BUZZ_HELP, cmd_configure_buzz, 4, 0),
	SHELL_SUBCMD_SET_END);
/* clang-format on */

SHELL_CMD_REGISTER(cs40l26, &cs40l26_cmds, "CS40L26 shell commands", NULL);
#endif /* CONFIG_SHELL */

static void cs40l26_dummy_callback(const struct device *const dev, const uint32_t errors,
				   void *const user_data)
{
	printk("fatal errors detected (0x%08X)", errors);
}

int main(void)
{
	int ret;

	if (!cs40l26 || !device_is_ready(cs40l26)) {
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		ret = pm_device_runtime_enable(cs40l26);
		if (ret < 0) {
			return ret;
		}
	}

	(void)haptics_register_error_callback(cs40l26, cs40l26_dummy_callback, NULL);

	/* Basic demonstration if not using the custom shell interface. */
	if (!IS_ENABLED(CONFIG_SHELL)) {
		ret = haptics_calibrate(cs40l26, 0);
		if (ret < 0) {
			return ret;
		}

		(void)k_msleep(100);

		ret = cs40l26_configure_buzz(cs40l26, CS40L26_DEMO_FREQUENCY, CS40L26_DEMO_LEVEL,
					     CS40L26_DEMO_DURATION);
		if (ret < 0) {
			return ret;
		}

		ret = haptics_select_source(cs40l26, (int)CS40L26_SOURCE_BUZ, 0);
		if (ret < 0) {
			return ret;
		}

		ret = haptics_start_output(cs40l26);
		if (ret < 0) {
			return ret;
		}

		(void)k_msleep(100);

		ret = haptics_stop_output(cs40l26);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
