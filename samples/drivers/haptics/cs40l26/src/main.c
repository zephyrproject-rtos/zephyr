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
#define CS40L26_CALIBRATE_HELP      SHELL_HELP("Run calibration routine", NULL)
#define CS40L26_CONFIGURE_BUZZ_HELP SHELL_HELP("Configure buzz output", "<freq> <level> <dur>")
#define CS40L26_GAIN_HELP           SHELL_HELP("Update gain", "<gain %%>")
#define CS40L26_SELECT_HELP         SHELL_HELP("Select haptic effect", "<bank> <index>")
#define CS40L26_START_HELP          SHELL_HELP("Start haptic playback", NULL)
#define CS40L26_STOP_HELP           SHELL_HELP("Stop haptic playback", NULL)

static int cmd_calibrate(const struct shell *sh, size_t argc, char **argv)
{
	return cs40l26_calibrate(cs40l26);
}

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

static int cmd_gain(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t gain;

	gain = strtoul(argv[1], NULL, 10);

	if (gain > 100) {
		shell_error(sh, "Gain must be between 0 and 100%%");
		return -EINVAL;
	}

	return cs40l26_set_gain(cs40l26, (uint8_t)gain);
}

static int cmd_select(const struct shell *sh, size_t argc, char **argv)
{
	enum cs40l26_bank bank;
	uint8_t index;

	if (strcmp(argv[1], "ROM") == 0) {
		bank = CS40L26_ROM_BANK;
	} else if (strcmp(argv[1], "BUZ") == 0) {
		bank = CS40L26_BUZ_BANK;
	} else {
		shell_error(sh, "Bank must be 'ROM' or 'BUZ'");
		return -EINVAL;
	}

	index = strtoul(argv[2], NULL, 10);

	return cs40l26_select_output(cs40l26, bank, index);
}

static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	return haptics_start_output(cs40l26);
}

static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	return haptics_stop_output(cs40l26);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	cs40l26_cmds, SHELL_CMD_ARG(calibrate, NULL, CS40L26_CALIBRATE_HELP, cmd_calibrate, 1, 0),
	SHELL_CMD_ARG(configure_buzz, NULL, CS40L26_CONFIGURE_BUZZ_HELP, cmd_configure_buzz, 4, 0),
	SHELL_CMD_ARG(gain, NULL, CS40L26_GAIN_HELP, cmd_gain, 2, 0),
	SHELL_CMD_ARG(select, NULL, CS40L26_SELECT_HELP, cmd_select, 3, 0),
	SHELL_CMD_ARG(start, NULL, CS40L26_START_HELP, cmd_start, 1, 0),
	SHELL_CMD_ARG(stop, NULL, CS40L26_STOP_HELP, cmd_stop, 1, 0), SHELL_SUBCMD_SET_END);

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
		ret = cs40l26_calibrate(cs40l26);
		if (ret < 0) {
			return ret;
		}

		(void)k_msleep(100);

		ret = cs40l26_configure_buzz(cs40l26, CS40L26_DEMO_FREQUENCY, CS40L26_DEMO_LEVEL,
					     CS40L26_DEMO_DURATION);
		if (ret < 0) {
			return ret;
		}

		ret = cs40l26_select_output(cs40l26, CS40L26_BUZ_BANK, 0);
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
