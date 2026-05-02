/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <inttypes.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/haptics/cs40l5x.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#define CS40L5X_DEMO_TRIGGER DT_NODE_HAS_PROP(DT_ALIAS(haptic0), trigger_gpios)

#if CS40L5X_DEMO_TRIGGER
#include <zephyr/drivers/gpio.h>
#endif /* CS40L5X_DEMO_TRIGGER */

#if CONFIG_SHELL
#include <stdlib.h>
#endif /* CONFIG_SHELL */

LOG_MODULE_REGISTER(main);

#define CS40L5X_DEMO_REDC              (0xCC)
#define CS40L5X_DEMO_F0                (0x258)
#define CS40L5X_DEMO_INDEX             (17)
#define CS40L5X_DEMO_INFINITE_DURATION (0)
#define CS40L5X_DEMO_FREQUENCY         (240)
#define CS40L5X_DEMO_LEVEL             (27)

const struct device *cs40l5x = DEVICE_DT_GET(DT_ALIAS(haptic0));

#if CS40L5X_DEMO_TRIGGER
const struct gpio_dt_spec demo_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_ALIAS(haptic0), trigger_gpios, 0);
#endif /* CS40L5X_DEMO_TRIGGER */

#if CONFIG_SHELL
#define CS40L5X_HELP                SHELL_HELP("CS40L5x haptics commands", NULL)
#define CS40L5X_CALIBRATE_HELP      SHELL_HELP("Run calibration routine", NULL)
#define CS40L5X_CONFIGURE_BUZZ_HELP SHELL_HELP("Configure buzz output", "<freq> <level> <dur>")
#define CS40L5X_GAIN_HELP           SHELL_HELP("Update gain", "<gain %%>")
#define CS40L5X_LOGGER_HELP         SHELL_HELP("Enable or disable runtime logging", "<state>")
#define CS40L5X_LOGGER_GET_HELP     SHELL_HELP("Get runtime logging data", "<source>")
#define CS40L5X_SELECT_HELP         SHELL_HELP("Select haptic effect", "<source>")
#define CS40L5X_START_HELP          SHELL_HELP("Start haptic playback", NULL)
#define CS40L5X_STOP_HELP           SHELL_HELP("Stop haptic playback", NULL)
#define CS40L5X_TRIGGER_HELP        SHELL_HELP("Induce trigger playback via GPIO", "<gpio> <pin>")

static int cmd_calibrate(const struct shell *sh, size_t argc, char **argv)
{
	return cs40l5x_calibrate(cs40l5x);
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

	return cs40l5x_configure_buzz(cs40l5x, frequency, (uint8_t)level, duration);
}

static int cmd_gain(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t gain;

	gain = strtoul(argv[1], NULL, 10);

	if (gain > 100) {
		shell_error(sh, "Gain must be between 0 and 100%%");
		return -EINVAL;
	}

	return cs40l5x_set_gain(cs40l5x, (uint8_t)gain);
}

static int cmd_logger(const struct shell *sh, size_t argc, char **argv, void *data)
{
	return cs40l5x_logger(cs40l5x, (int)data);
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_logger, cmd_logger, (disable, 0, "disable"),
			     (enable, 1, "enable"));

static int cmd_logger_get(const struct shell *sh, size_t argc, char **argv, void *data)
{
	enum cs40l5x_logger_source_type type;
	enum cs40l5x_logger_source source;
	int error, input;
	uint32_t value;

	input = (int)data;

	if (input < 3) {
		source = CS40L5X_LOGGER_BEMF;
	} else if (input < 6) {
		source = CS40L5X_LOGGER_VBST;
	} else {
		source = CS40L5X_LOGGER_VMON;
	}

	type = (enum cs40l5x_logger_source_type)(input % 3);

	error = cs40l5x_logger_get(cs40l5x, source, type, &value);
	if (error >= 0) {
		shell_print(sh, "%s: 0x%08X", argv[0], value);
	}

	return error;
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_logger_get, cmd_logger_get, (min_bemf, 0, "Minimum BEMF"),
			     (max_bemf, 1, "Maximum BEMF"), (mean_bemf, 2, "Mean BEMF"),
			     (min_vbst, 3, "Minimum VBST"), (max_vbst, 4, "Maximum VBST"),
			     (mean_vbst, 5, "Mean VBST"), (min_vmon, 6, "Minimum VMON"),
			     (max_vmon, 7, "Maximum VMON"), (mean_vmon, 8, "Mean VMON"));

static int cmd_select(const struct shell *sh, size_t argc, char **argv, void *data)
{
	enum cs40l5x_bank bank;
	uint8_t index;
	int source;

	source = (int)data;
	if (source < 27) {
		bank = CS40L5X_ROM_BANK;
		index = source;
	} else if (source == 27) {
		bank = CS40L5X_BUZ_BANK;
		index = 0;
	} else {
		bank = CS40L5X_CUSTOM_BANK;
		index = source % 28;
	}

	return cs40l5x_select_output(cs40l5x, bank, index);
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_select, cmd_select, (ROM0, 0, "ROM 0"), (ROM1, 1, "ROM 1"),
			     (ROM2, 2, "ROM 2"), (ROM3, 3, "ROM 3"), (ROM4, 4, "ROM 4"),
			     (ROM5, 5, "ROM 5"), (ROM6, 6, "ROM 6"), (ROM7, 7, "ROM 7"),
			     (ROM8, 8, "ROM 8"), (ROM9, 9, "ROM 9"), (ROM10, 10, "ROM 10"),
			     (ROM11, 11, "ROM 11"), (ROM12, 12, "ROM 12"), (ROM13, 13, "ROM 13"),
			     (ROM14, 14, "ROM 14"), (ROM15, 15, "ROM 15"), (ROM16, 16, "ROM 16"),
			     (ROM17, 17, "ROM 17"), (ROM18, 18, "ROM 18"), (ROM19, 19, "ROM 19"),
			     (ROM20, 20, "ROM 20"), (ROM21, 21, "ROM 21"), (ROM22, 22, "ROM 22"),
			     (ROM23, 23, "ROM 23"), (ROM24, 24, "ROM 24"), (ROM25, 25, "ROM 25"),
			     (ROM26, 26, "ROM 26"), (BUZ0, 27, "BUZ 0"), (CUSTOM0, 28, "CUSTOM 0"),
			     (CUSTOM1, 29, "CUSTOM 1"));

static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	return haptics_start_output(cs40l5x);
}

static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	return haptics_stop_output(cs40l5x);
}

#if CS40L5X_DEMO_TRIGGER
static int cmd_trigger(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t pin;

	dev = shell_device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "GPIO device not found (%d)", -EINVAL);
		return -EINVAL;
	}

	pin = strtoul(argv[2], NULL, 10);

	if (pin > UINT8_MAX) {
		shell_error(sh, "GPIO pin too large (%d)", -EINVAL);
		return -EINVAL;
	}

	return gpio_pin_toggle(dev, pin);
}

static bool device_is_gpio(const struct device *dev)
{
	return DEVICE_API_IS(gpio, dev);
}

static void gpio_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_gpio);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_gpio_name, gpio_name_get);
#endif /* CS40L5X_DEMO_TRIGGER */

SHELL_STATIC_SUBCMD_SET_CREATE(
	cs40l5x_cmds, SHELL_CMD_ARG(calibrate, NULL, CS40L5X_CALIBRATE_HELP, cmd_calibrate, 1, 0),
	SHELL_CMD_ARG(configure_buzz, NULL, CS40L5X_CONFIGURE_BUZZ_HELP, cmd_configure_buzz, 4, 0),
	SHELL_CMD_ARG(gain, NULL, CS40L5X_GAIN_HELP, cmd_gain, 2, 0),
	SHELL_CMD_ARG(logger, &sub_logger, CS40L5X_LOGGER_HELP, cmd_logger, 2, 0),
	SHELL_CMD_ARG(logger_get, &sub_logger_get, CS40L5X_LOGGER_GET_HELP, cmd_logger_get, 2, 0),
	SHELL_CMD_ARG(select, &sub_select, CS40L5X_SELECT_HELP, NULL, 2, 0),
	SHELL_CMD_ARG(start, NULL, CS40L5X_START_HELP, cmd_start, 1, 0),
	SHELL_CMD_ARG(stop, NULL, CS40L5X_STOP_HELP, cmd_stop, 1, 0),
#if CS40L5X_DEMO_TRIGGER
	SHELL_CMD_ARG(trigger, &dsub_gpio_name, CS40L5X_TRIGGER_HELP, cmd_trigger, 3, 0),
#endif /* CS40L5X_DEMO_TRIGGER */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(cs40l5x, &cs40l5x_cmds, "CS40L5x shell commands", NULL);
#endif /* CONFIG_SHELL */

static const int8_t pcm_samples[78] = {
	0x00, 0x07, 0x0D, 0x12, 0x18, 0x1E, 0x22, 0x27, 0x2B, 0x2D, 0x30, 0x31, 0x32,
	0x32, 0x31, 0x30, 0x2D, 0x2B, 0x27, 0x22, 0x1E, 0x18, 0x12, 0x0D, 0x07, 0x00,
	0xF9, 0xF3, 0xED, 0xE8, 0xE2, 0xDD, 0xD9, 0xD5, 0xD2, 0xD0, 0xCE, 0xCD, 0xCD,
	0xCE, 0xD0, 0xD2, 0xD5, 0xD9, 0xDD, 0xE2, 0xE8, 0xED, 0xF3, 0xF9, 0x00, 0x07,
	0x0D, 0x12, 0x18, 0x1E, 0x22, 0x27, 0x2B, 0x2D, 0x30, 0x31, 0x32, 0x32, 0x31,
	0x30, 0x2D, 0x2B, 0x27, 0x22, 0x1E, 0x18, 0x12, 0x0D, 0x07, 0x00, 0xF9, 0xF3,
};

static const struct cs40l5x_pwle_section pwle_sections[] = {
	{
		.level = 0x400,
	},
	{
		.duration = 0x0028,
		.level = 0x400,
		.frequency = 0x320,
		.flags = 0x1,
	},
	{
		.duration = 0,
		.level = 0x200,
		.frequency = 0x320,
		.flags = 0x1,
	},
	{
		.duration = 0x0014,
		.level = 0x200,
		.frequency = 0x640,
		.flags = 0x1,
	},
	{
		.duration = 0,
		.level = 0x400,
		.frequency = 0x190,
		.flags = 0x1,
	},
	{
		.duration = 0x00A0,
		.level = 0x400,
		.frequency = 0x7D0,
		.flags = 0x9,
	},
	{
		.duration = 0,
		.level = 0x200,
		.frequency = 0x320,
		.flags = 0x1,
	},
	{
		.duration = 0x0003,
		.level = 0x200,
		.frequency = 0x320,
		.flags = 0x3,
	},
};

static void cs40l5x_dummy_callback(const struct device *const dev, const uint32_t errors,
				   void *const user_data)
{
	LOG_INF("fatal errors detected (0x%08X)", errors);
}

int main(void)
{
	int error;

	if (!cs40l5x || !device_is_ready(cs40l5x)) {
		LOG_ERR("device not available: %s", cs40l5x->name);
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		error = pm_device_runtime_enable(cs40l5x);
		if (error < 0) {
			LOG_ERR("PM runtime disabled for %s (%d)", cs40l5x->name, error);
			return -EIO;
		}
	}

	(void)haptics_register_error_callback(cs40l5x, cs40l5x_dummy_callback, NULL);

	/* Demonstration of PCM upload (CUSTOM0) */
	error = cs40l5x_upload_pcm(cs40l5x, CS40L5X_CUSTOM_0, CS40L5X_DEMO_REDC, CS40L5X_DEMO_F0,
				   pcm_samples, ARRAY_SIZE(pcm_samples));
	if (error < 0) {
		LOG_WRN("upload PCM failure (%d)", error);
	}

	/* Demonstration of PWLE upload (CUSTOM1) */
	error = cs40l5x_upload_pwle(cs40l5x, CS40L5X_CUSTOM_1, pwle_sections,
				    ARRAY_SIZE(pwle_sections));
	if (error < 0) {
		LOG_WRN("upload PWLE failure (%d)", error);
	}

	/* Demonstration of buzz configuration (BUZ0) */
	error = cs40l5x_configure_buzz(cs40l5x, CS40L5X_DEMO_FREQUENCY, CS40L5X_DEMO_LEVEL,
				       CS40L5X_DEMO_INFINITE_DURATION);
	if (error < 0) {
		LOG_WRN("buzz configuration failure (%d)", error);
	}

#if CS40L5X_DEMO_TRIGGER
	/* Demonstration of GPIO configuration for edge-triggered haptic effects */
	error = cs40l5x_configure_trigger(cs40l5x, &demo_gpio, CS40L5X_ROM_BANK, CS40L5X_DEMO_INDEX,
					  CS40L5X_ATTENUATION_3DB, CS40L5X_RISING_EDGE);
	if (error < 0) {
		LOG_WRN("configure GPIO trigger failure (%d)", error);
	}
#endif /* CS40L5X_DEMO_TRIGGER */

	/* Basic demonstration if not using the custom shell interface. */
	if (!IS_ENABLED(CONFIG_SHELL)) {
		error = cs40l5x_select_output(cs40l5x, CS40L5X_BUZ_BANK, 0);
		if (error < 0) {
			LOG_WRN("failed to select output (%d)", error);
		}

		error = haptics_start_output(cs40l5x);
		if (error < 0) {
			LOG_WRN("failed to start output (%d)", error);
		}

		(void)k_msleep(2000);

		error = cs40l5x_select_output(cs40l5x, CS40L5X_ROM_BANK, 17);
		if (error < 0) {
			LOG_WRN("failed to select output during playback (%d)", error);
		}

		error = haptics_start_output(cs40l5x);
		if (error < 0) {
			LOG_WRN("failed to preempt output (%d)", error);
		}

		(void)k_msleep(2000);

		error = haptics_stop_output(cs40l5x);
		if (error < 0) {
			LOG_WRN("failed to stop output (%d)", error);
		}
	}

	return 0;
}
