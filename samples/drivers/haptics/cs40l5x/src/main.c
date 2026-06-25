/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/haptics/cs40l5x.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
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

#define CS40L5X_DEMO_REDC              0xCC
#define CS40L5X_DEMO_F0                0x258
#define CS40L5X_DEMO_INDEX             17
#define CS40L5X_DEMO_INFINITE_DURATION 0
#define CS40L5X_DEMO_FREQUENCY         240
#define CS40L5X_DEMO_LEVEL             27

const struct device *cs40l5x = DEVICE_DT_GET(DT_ALIAS(haptic0));

#if CS40L5X_DEMO_TRIGGER
const struct gpio_dt_spec demo_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_ALIAS(haptic0), trigger_gpios, 0);
#endif /* CS40L5X_DEMO_TRIGGER */

#if CONFIG_SHELL
#define CS40L5X_HELP                SHELL_HELP("CS40L5x haptics commands", NULL)
#define CS40L5X_CONFIGURE_BUZZ_HELP SHELL_HELP("Configure buzz output", "<freq> <level> <dur>")
#define CS40L5X_TRIGGER_HELP        SHELL_HELP("Induce trigger playback via GPIO", "<gpio> <pin>")

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

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(cs40l5x_cmds,
	SHELL_CMD_ARG(configure_buzz, NULL, CS40L5X_CONFIGURE_BUZZ_HELP, cmd_configure_buzz, 4, 0),
#if CS40L5X_DEMO_TRIGGER
	SHELL_CMD_ARG(trigger, &dsub_gpio_name, CS40L5X_TRIGGER_HELP, cmd_trigger, 3, 0),
#endif /* CS40L5X_DEMO_TRIGGER */
	SHELL_SUBCMD_SET_END);
/* clang-format off */

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
	const union haptics_config cfg = {.idx = CS40L5X_DEMO_INDEX};
	int ret;

	if (!cs40l5x || !device_is_ready(cs40l5x)) {
		LOG_ERR("device not available: %s", cs40l5x->name);
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		ret = pm_device_runtime_enable(cs40l5x);
		if (ret < 0) {
			LOG_ERR("PM runtime disabled for %s (%d)", cs40l5x->name, ret);
			return ret;
		}
	}

	(void)haptics_register_error_callback(cs40l5x, cs40l5x_dummy_callback, NULL);

	/* Demonstration of PCM upload (CUSTOM0) */
	ret = cs40l5x_upload_pcm(cs40l5x, 0, CS40L5X_DEMO_REDC, CS40L5X_DEMO_F0, pcm_samples,
				 ARRAY_SIZE(pcm_samples));
	if (ret < 0) {
		LOG_WRN("upload PCM failure (%d)", ret);
	}

	/* Demonstration of PWLE upload (CUSTOM1) */
	ret = cs40l5x_upload_pwle(cs40l5x, 1, pwle_sections, ARRAY_SIZE(pwle_sections));
	if (ret < 0) {
		LOG_WRN("upload PWLE failure (%d)", ret);
	}

	/* Demonstration of buzz configuration (BUZ0) */
	ret = cs40l5x_configure_buzz(cs40l5x, CS40L5X_DEMO_FREQUENCY, CS40L5X_DEMO_LEVEL,
				       CS40L5X_DEMO_INFINITE_DURATION);
	if (ret < 0) {
		LOG_WRN("buzz configuration failure (%d)", ret);
	}

#if CS40L5X_DEMO_TRIGGER
	/* Demonstration of GPIO configuration for edge-triggered haptic effects */
	ret = cs40l5x_configure_trigger(cs40l5x, &demo_gpio, HAPTICS_SOURCE_ROM, &cfg, 3,
					CS40L5X_RISING_EDGE);
	if (ret < 0) {
		LOG_WRN("configure GPIO trigger failure (%d)", ret);
	}
#endif /* CS40L5X_DEMO_TRIGGER */

	/* Basic demonstration if not using the custom shell interface. */
	if (!IS_ENABLED(CONFIG_SHELL)) {
		ret = haptics_select_source(cs40l5x, (int)CS40L5X_SOURCE_BUZ, 0);
		if (ret < 0) {
			LOG_WRN("failed to select output (%d)", ret);
		}

		ret = haptics_start_output(cs40l5x);
		if (ret < 0) {
			LOG_WRN("failed to start output (%d)", ret);
		}

		(void)k_msleep(2000);

		ret = haptics_select_source(cs40l5x, HAPTICS_SOURCE_ROM, &cfg);
		if (ret < 0) {
			LOG_WRN("failed to select output during playback (%d)", ret);
		}

		ret = haptics_start_output(cs40l5x);
		if (ret < 0) {
			LOG_WRN("failed to preempt output (%d)", ret);
		}

		(void)k_msleep(2000);

		ret = haptics_stop_output(cs40l5x);
		if (ret < 0) {
			LOG_WRN("failed to stop output (%d)", ret);
		}
	}

	return 0;
}
