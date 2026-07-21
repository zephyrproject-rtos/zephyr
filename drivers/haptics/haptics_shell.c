/*
 * Copyright (c) 2025 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Haptics shell commands.
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>

#define HAPTICS_ARGS_MAX CONFIG_HAPTICS_SHELL_BUFFER_SIZE

#define HAPTICS_CALIBRATE_HELP SHELL_HELP("Run calibration", "<device> [<routine>]")
#define HAPTICS_GET_HELP       SHELL_HELP("Get monitor reading", "<device> <monitor> <type>")
#define HAPTICS_LEVEL_HELP     SHELL_HELP("Set output level", "<device> <source> [<idx>] <level>")
#define HAPTICS_SELECT_HELP    SHELL_HELP("Select haptic source", "<device> <source> [<idx>]")
#define HAPTICS_SET_HELP       SHELL_HELP("Turn monitor on/off", "<device> <monitor> <enable>")
#define HAPTICS_START_HELP     SHELL_HELP("Start haptic output", "<device>")
#define HAPTICS_STOP_HELP      SHELL_HELP("Stop haptic output", "<device>")
#define HAPTICS_STREAM_HELP    SHELL_HELP("Stream samples", "<device> [<byte1> <byte2> ...]")

#define HAPTICS_MONITORS     bemf, current, f0, re, ambient_temp, die_temp, vbat, vbst, vout
#define HAPTICS_MONITORS_ALL HAPTICS_MONITORS, all
#define HAPTICS_GET          min, max, mean, single
#define HAPTICS_SET          disable, enable
#define HAPTICS_SOURCES      rom, ram, dai, analog, pwm, control_port, all

#define HAPTICS_ARGS_DEVICE         1
#define HAPTICS_ARGS_MONITOR        2
#define HAPTICS_ARGS_ROUTINE        2
#define HAPTICS_ARGS_SOURCE         2
#define HAPTICS_ARGS_STREAM_START   2
#define HAPTICS_ARGS_ID_OR_LEVEL    3
#define HAPTICS_ARGS_MONITOR_OPTION 3
#define HAPTICS_ARGS_LEVEL          4

static const char *const monitor_units[] = {
	[HAPTICS_MONITOR_BEMF] = " V",         [HAPTICS_MONITOR_CURRENT] = " A",
	[HAPTICS_MONITOR_F0] = " Hz",          [HAPTICS_MONITOR_RE] = " Ohms",
	[HAPTICS_MONITOR_AMBIENT_TEMP] = " C", [HAPTICS_MONITOR_DIE_TEMP] = " C",
	[HAPTICS_MONITOR_VBAT] = " V",         [HAPTICS_MONITOR_VBST] = " V",
	[HAPTICS_MONITOR_VOUT] = " V",
};

static int monitor_lookup(const char *const arg, enum haptics_monitor *const monitor)
{
	int ret;

	if (strcmp(arg, "bemf") == 0) {
		*monitor = HAPTICS_MONITOR_BEMF;
	} else if (strcmp(arg, "current") == 0) {
		*monitor = HAPTICS_MONITOR_CURRENT;
	} else if (strcmp(arg, "f0") == 0) {
		*monitor = HAPTICS_MONITOR_F0;
	} else if (strcmp(arg, "re") == 0) {
		*monitor = HAPTICS_MONITOR_RE;
	} else if (strcmp(arg, "ambient_temp") == 0) {
		*monitor = HAPTICS_MONITOR_AMBIENT_TEMP;
	} else if (strcmp(arg, "die_temp") == 0) {
		*monitor = HAPTICS_MONITOR_DIE_TEMP;
	} else if (strcmp(arg, "vout") == 0) {
		*monitor = HAPTICS_MONITOR_VOUT;
	} else if (strcmp(arg, "vbat") == 0) {
		*monitor = HAPTICS_MONITOR_VBAT;
	} else if (strcmp(arg, "vbst") == 0) {
		*monitor = HAPTICS_MONITOR_VBST;
	} else if (strcmp(arg, "all") == 0) {
		*monitor = HAPTICS_MONITOR_ALL;
	} else {
		*monitor = shell_strtol(arg, 10, &ret);
		if (ret < 0) {
			return -EINVAL;
		}
	}

	return 0;
}

static int source_lookup(const char *const arg, enum haptics_source *const src)
{
	int ret;

	if (strcmp(arg, "rom") == 0) {
		*src = HAPTICS_SOURCE_ROM;
	} else if (strcmp(arg, "ram") == 0) {
		*src = HAPTICS_SOURCE_RAM;
	} else if (strcmp(arg, "dai") == 0) {
		*src = HAPTICS_SOURCE_DAI;
	} else if (strcmp(arg, "analog") == 0) {
		*src = HAPTICS_SOURCE_ANALOG;
	} else if (strcmp(arg, "pwm") == 0) {
		*src = HAPTICS_SOURCE_PWM;
	} else if (strcmp(arg, "control_port") == 0) {
		*src = HAPTICS_SOURCE_CONTROL_PORT;
	} else if (strcmp(arg, "all") == 0) {
		*src = HAPTICS_SOURCE_ALL;
	} else {
		*src = shell_strtol(arg, 10, &ret);
		if (ret < 0) {
			return -EINVAL;
		}
	}

	return 0;
}

static int cmd_calibrate(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t routine = 0;
	int ret;

	dev = shell_device_get_binding(argv[HAPTICS_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "haptic device not found");
		return -EINVAL;
	}

	if (argc > HAPTICS_ARGS_ROUTINE) {
		routine = shell_strtoul(argv[HAPTICS_ARGS_ROUTINE], 10, &ret);
		if (ret < 0) {
			shell_error(sh, "failed to parse calibration routine");
			return ret;
		}
	}

	ret = haptics_calibrate(dev, routine);
	if (ret < 0) {
		shell_error(sh, "failed to calibrate haptic driver (%d)", ret);
	}

	return ret;
}

static int cmd_get(const struct shell *sh, size_t argc, char **argv)
{
	enum haptics_monitor monitor;
	const struct device *dev;
	struct sensor_value val;
	int custom, ret;

	dev = shell_device_get_binding(argv[HAPTICS_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "haptic device not found");
		return -EINVAL;
	}

	ret = monitor_lookup(argv[HAPTICS_ARGS_MONITOR], &monitor);
	if (ret < 0) {
		shell_error(sh, "invalid monitor '%s'", argv[HAPTICS_ARGS_MONITOR]);
		return ret;
	}

	if (monitor == HAPTICS_MONITOR_ALL) {
		shell_error(sh, "invalid monitor 'all' for haptics_monitor_get()");
		return -EINVAL;
	}

	if (strcmp(argv[HAPTICS_ARGS_MONITOR_OPTION], "min") == 0) {
		ret = haptics_monitor_get(dev, monitor, HAPTICS_MONITOR_TYPE_MIN, &val);
	} else if (strcmp(argv[HAPTICS_ARGS_MONITOR_OPTION], "max") == 0) {
		ret = haptics_monitor_get(dev, monitor, HAPTICS_MONITOR_TYPE_MAX, &val);
	} else if (strcmp(argv[HAPTICS_ARGS_MONITOR_OPTION], "mean") == 0) {
		ret = haptics_monitor_get(dev, monitor, HAPTICS_MONITOR_TYPE_MEAN, &val);
	} else if (strcmp(argv[HAPTICS_ARGS_MONITOR_OPTION], "single") == 0) {
		ret = haptics_monitor_get(dev, monitor, HAPTICS_MONITOR_TYPE_SINGLE, &val);
	} else {
		custom = shell_strtol(argv[HAPTICS_ARGS_MONITOR_OPTION], 10, &ret);
		if (ret < 0) {
			return -EINVAL;
		}

		ret = haptics_monitor_get(dev, monitor, custom, &val);
	}
	if (ret < 0) {
		shell_error(sh, "failed to get sensor reading (%d)", ret);
		return ret;
	}

	shell_print(sh, "%s%d.%.6d%s", (val.val1 < 0 || val.val2 < 0) ? "-" : "", abs(val.val1),
		    abs(val.val2),
		    (monitor < HAPTICS_MONITOR_PRIV_START) ? monitor_units[monitor] : "");

	return 0;
}

static int cmd_level(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	union haptics_config cfg;
	enum haptics_source src;
	uint32_t level;
	int ret;

	dev = shell_device_get_binding(argv[HAPTICS_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "haptic device not found");
		return -EINVAL;
	}

	ret = source_lookup(argv[HAPTICS_ARGS_SOURCE], &src);
	if (ret < 0) {
		shell_error(sh, "invalid source '%s'", argv[HAPTICS_ARGS_SOURCE]);
		return ret;
	}

	if (argc > HAPTICS_ARGS_LEVEL) {
		cfg.idx = shell_strtoul(argv[HAPTICS_ARGS_ID_OR_LEVEL], 10, &ret);
		if (ret < 0) {
			shell_error(sh, "failed to parse idx (%d)", ret);
			return ret;
		}

		level = shell_strtoul(argv[HAPTICS_ARGS_LEVEL], 10, &ret);
	} else {
		level = shell_strtoul(argv[HAPTICS_ARGS_ID_OR_LEVEL], 10, &ret);
	}
	if (ret < 0) {
		shell_error(sh, "failed to parse level (%d)", ret);
		return ret;
	}

	ret = haptics_set_level(dev, src, (argc > HAPTICS_ARGS_LEVEL) ? &cfg : NULL, level);
	if (ret < 0) {
		shell_error(sh, "failed to set level (%d)", ret);
	}

	return ret;
}

static int cmd_select(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	union haptics_config cfg;
	enum haptics_source src;
	int ret;

	dev = shell_device_get_binding(argv[HAPTICS_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "haptic device not found");
		return -EINVAL;
	}

	ret = source_lookup(argv[HAPTICS_ARGS_SOURCE], &src);
	if (ret < 0) {
		shell_error(sh, "invalid source '%s'", argv[HAPTICS_ARGS_SOURCE]);
		return ret;
	}

	if (argc > HAPTICS_ARGS_ID_OR_LEVEL) {
		cfg.idx = shell_strtoul(argv[HAPTICS_ARGS_ID_OR_LEVEL], 10, &ret);
		if (ret < 0) {
			shell_error(sh, "failed to parse idx (%d)", ret);
			return ret;
		}
	}

	ret = haptics_select_source(dev, src, (argc > HAPTICS_ARGS_ID_OR_LEVEL) ? &cfg : NULL);
	if (ret < 0) {
		shell_error(sh, "failed to select playback source (%d)", ret);
	}

	return ret;
}

static int cmd_set(const struct shell *sh, size_t argc, char **argv)
{
	enum haptics_monitor monitor;
	const struct device *dev;
	int ret;

	dev = shell_device_get_binding(argv[HAPTICS_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "haptic device not found");
		return -EINVAL;
	}

	ret = monitor_lookup(argv[HAPTICS_ARGS_MONITOR], &monitor);
	if (ret < 0) {
		shell_error(sh, "invalid monitor '%s'", argv[HAPTICS_ARGS_MONITOR]);
		return ret;
	}

	if (strcmp(argv[HAPTICS_ARGS_MONITOR_OPTION], "disable") == 0) {
		ret = haptics_monitor_set(dev, monitor, false);
	} else if (strcmp(argv[HAPTICS_ARGS_MONITOR_OPTION], "enable") == 0) {
		ret = haptics_monitor_set(dev, monitor, true);
	} else {
		shell_error(sh, "invalid monitor option '%s'", argv[HAPTICS_ARGS_MONITOR_OPTION]);
		return -EINVAL;
	}
	if (ret < 0) {
		shell_error(sh, "failed to configure monitor (%d)", ret);
	}

	return ret;
}

static int cmd_start(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = shell_device_get_binding(argv[HAPTICS_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "haptic device not found");
		return -EINVAL;
	}

	ret = haptics_start_output(dev);
	if (ret < 0) {
		shell_error(sh, "failed to start haptic output (%d)", ret);
	}

	return ret;
}

static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int ret;

	dev = shell_device_get_binding(argv[HAPTICS_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "haptic device not found");
		return -EINVAL;
	}

	ret = haptics_stop_output(dev);
	if (ret < 0) {
		shell_error(sh, "failed to stop haptic output (%d)", ret);
	}

	return ret;
}

static int cmd_stream(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t samples[HAPTICS_ARGS_MAX];
	const struct device *dev;
	uint32_t byte;
	int ret;

	dev = shell_device_get_binding(argv[HAPTICS_ARGS_DEVICE]);
	if (dev == NULL) {
		shell_error(sh, "haptic device not found");
		return -EINVAL;
	}

	if (argc == HAPTICS_ARGS_STREAM_START) {
		shell_error(sh, "invalid data stream, no bytes provided");
		return -EINVAL;
	}

	for (int i = HAPTICS_ARGS_STREAM_START; i < argc; i++) {
		byte = shell_strtoul(argv[i], 16, &ret);
		if (ret < 0) {
			shell_error(sh, "failed to parse byte (%d)", ret);
			return ret;
		}

		if (byte > UINT8_MAX) {
			shell_error(sh, "invalid input '0x%X', must not exceed %u", byte,
				    UINT8_MAX);
			return -EINVAL;
		}

		samples[i - HAPTICS_ARGS_STREAM_START] = byte;
	}

	ret = haptics_stream_samples(dev, samples, argc - HAPTICS_ARGS_STREAM_START);
	if (ret < 0) {
		shell_error(sh, "failed to stream samples (%d)", ret);
	}

	return ret;
}

static bool device_is_haptics(const struct device *dev)
{
	return DEVICE_API_IS(haptics, dev);
}

#define HAPTICS_CMD_ARG(_cmd, _subcmd) SHELL_CMD_ARG(_cmd, _subcmd, NULL, NULL, 1, 0)

#define HAPTICS_STATIC_SUBCMD_SET_CREATE(_name, _subcmd, ...)                                      \
	SHELL_STATIC_SUBCMD_SET_CREATE(                                                            \
		_name, FOR_EACH_FIXED_ARG(HAPTICS_CMD_ARG, (,), _subcmd, __VA_ARGS__),             \
		SHELL_SUBCMD_SET_END)

#define HAPTICS_DYNAMIC_CMD_CREATE(_name, _subcmd)                                                 \
	static void device_##_name(size_t idx, struct shell_static_entry *entry)                   \
	{                                                                                          \
		const struct device *dev = shell_device_filter(idx, device_is_haptics);            \
		entry->syntax = (dev != NULL) ? dev->name : NULL;                                  \
		entry->handler = NULL;                                                             \
		entry->help = NULL;                                                                \
		entry->subcmd = _subcmd;                                                           \
	}                                                                                          \
                                                                                                   \
	SHELL_DYNAMIC_CMD_CREATE(dsub_device_##_name, device_##_name)

/* Create static subcommand trees for tab auto-completion. */
HAPTICS_STATIC_SUBCMD_SET_CREATE(get, NULL, HAPTICS_GET);
HAPTICS_STATIC_SUBCMD_SET_CREATE(set, NULL, HAPTICS_SET);
HAPTICS_STATIC_SUBCMD_SET_CREATE(sources, NULL, HAPTICS_SOURCES);
HAPTICS_STATIC_SUBCMD_SET_CREATE(monitors_get, &get, HAPTICS_MONITORS);
HAPTICS_STATIC_SUBCMD_SET_CREATE(monitors_set, &set, HAPTICS_MONITORS_ALL);

/* Create dynamic commands to get haptic devices and traverse subcommand trees declared above. */
HAPTICS_DYNAMIC_CMD_CREATE(only, NULL);
HAPTICS_DYNAMIC_CMD_CREATE(get, &monitors_get);
HAPTICS_DYNAMIC_CMD_CREATE(set, &monitors_set);
HAPTICS_DYNAMIC_CMD_CREATE(source, &sources);

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(haptic_cmds,
	SHELL_CMD_ARG(calibrate, &dsub_device_only, HAPTICS_CALIBRATE_HELP, cmd_calibrate, 2, 1),
	SHELL_CMD_ARG(get, &dsub_device_get, HAPTICS_GET_HELP, cmd_get, 4, 0),
	SHELL_CMD_ARG(level, &dsub_device_source, HAPTICS_LEVEL_HELP, cmd_level, 4, 1),
	SHELL_CMD_ARG(select, &dsub_device_source, HAPTICS_SELECT_HELP, cmd_select, 3, 1),
	SHELL_CMD_ARG(set, &dsub_device_set, HAPTICS_SET_HELP, cmd_set, 4, 0),
	SHELL_CMD_ARG(start, &dsub_device_only, HAPTICS_START_HELP, cmd_start, 2, 0),
	SHELL_CMD_ARG(stop, &dsub_device_only, HAPTICS_STOP_HELP, cmd_stop, 2, 0),
	SHELL_CMD_ARG(stream, &dsub_device_only, HAPTICS_STREAM_HELP, cmd_stream, 2,
		      HAPTICS_ARGS_MAX),
	SHELL_SUBCMD_SET_END);
/* clang-format on */

SHELL_CMD_REGISTER(haptics, &haptic_cmds, "Haptic shell commands", NULL);
