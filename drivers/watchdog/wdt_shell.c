/*
 * Copyright (c) 2023 Centralp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/watchdog.h>

#define WDT_SETUP_HELP                                                                             \
	"Set up watchdog instance. Syntax:\n"                                                      \
	"<device>"

#define WDT_DISABLE_HELP                                                                           \
	"Disable watchdog instance. Syntax:\n"                                                     \
	"<device>"

#define WDT_TIMEOUT_HELP                                                                           \
	"Install a new timeout. Syntax:\n"                                                         \
	"<device> <none|cpu|soc> <min_ms> <max_ms>"

#define WDT_FEED_HELP                                                                              \
	"Feed specified watchdog timeout. Syntax:\n"                                               \
	"<device> <channel_id>"

static const char *const wdt_reset_name[] = {
	[WDT_FLAG_RESET_NONE] = "none",
	[WDT_FLAG_RESET_CPU_CORE] = "cpu",
	[WDT_FLAG_RESET_SOC] = "soc",
};

struct args_index {
	uint8_t device;
	uint8_t reset;
	uint8_t timeout_min;
	uint8_t timeout_max;
	uint8_t channel_id;
};

static const struct args_index args_indx = {
	.device = 1,
	.reset = 2,
	.timeout_min = 3,
	.timeout_max = 4,
	.channel_id = 2,
};

static int parse_named_int(const char *name, const char *const keystack[], size_t count)
{
	char *endptr;
	int i;

	/* Attempt to parse name as a number first */
	i = strtoul(name, &endptr, 0);

	if (*endptr == '\0') {
		return i;
	}

	/* Name is not a number, look it up */
	for (i = 0; i < count; i++) {
		if (strcmp(name, keystack[i]) == 0) {
			return i;
		}
	}

	return -ENOTSUP;
}

static int cmd_setup(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;

	dev = shell_device_get_binding(argv[args_indx.device]);
	if (!dev) {
		shell_error(sh, "WDT device not found");
		return -ENODEV;
	}

	return wdt_setup(dev, 0);
}

static int cmd_disable(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;

	dev = shell_device_get_binding(argv[args_indx.device]);
	if (!dev) {
		shell_error(sh, "WDT device not found");
		return -ENODEV;
	}

	return wdt_disable(dev);
}

static int cmd_timeout(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int flags;
	int timeout_min;
	int timeout_max;
	struct wdt_timeout_cfg cfg;
	int rc;

	dev = shell_device_get_binding(argv[args_indx.device]);
	if (!dev) {
		shell_error(sh, "WDT device not found");
		return -ENODEV;
	}

	flags = parse_named_int(argv[args_indx.reset], wdt_reset_name, ARRAY_SIZE(wdt_reset_name));
	if (flags < 0) {
		shell_error(sh, "Reset mode '%s' unknown", argv[args_indx.reset]);
		return -EINVAL;
	}

	timeout_min = parse_named_int(argv[args_indx.timeout_min], NULL, 0);
	if (timeout_min < 0) {
		shell_error(sh, "Unable to convert '%s' to integer", argv[args_indx.timeout_min]);
		return -EINVAL;
	}

	timeout_max = parse_named_int(argv[args_indx.timeout_max], NULL, 0);
	if (timeout_max < 0) {
		shell_error(sh, "Unable to convert '%s' to integer", argv[args_indx.timeout_max]);
		return -EINVAL;
	}

	cfg.window.min = timeout_min;
	cfg.window.max = timeout_max;
	cfg.callback = NULL;
	cfg.flags = flags;

	rc = wdt_install_timeout(dev, &cfg);
	if (rc >= 0) {
		shell_print(sh, "Channel ID = %d", rc);
	}

	return rc;
}

static int cmd_feed(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int channel_id;

	dev = shell_device_get_binding(argv[args_indx.device]);
	if (!dev) {
		shell_error(sh, "WDT device not found");
		return -ENODEV;
	}

	channel_id = parse_named_int(argv[args_indx.channel_id], NULL, 0);
	if (channel_id < 0) {
		shell_error(sh, "Unable to convert '%s' to integer", argv[args_indx.channel_id]);
		return -EINVAL;
	}

	return wdt_feed(dev, channel_id);
}

static bool device_is_wdt(const struct device *dev)
{
	return DEVICE_API_IS(wdt, dev);
}

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_wdt);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_wdt,
	SHELL_CMD_ARG(setup, &dsub_device_name, WDT_SETUP_HELP, cmd_setup,
			2, 0),
	SHELL_CMD_ARG(disable, &dsub_device_name, WDT_DISABLE_HELP, cmd_disable,
			2, 0),
	SHELL_CMD_ARG(timeout, &dsub_device_name, WDT_TIMEOUT_HELP, cmd_timeout,
			5, 0),
	SHELL_CMD_ARG(feed, &dsub_device_name, WDT_FEED_HELP, cmd_feed,
			3, 0),
	SHELL_SUBCMD_SET_END
);
/* clang-format on */

SHELL_CMD_REGISTER(wdt, &sub_wdt, "Watchdog commands", NULL);
