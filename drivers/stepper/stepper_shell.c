/*
 * Copyright (c) 2024, Fabian Blatz <fabianblatz@gmail.com>
 * Copyright (c) 2024, Jilay Sandeep Pandya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_shell, CONFIG_STEPPER_LOG_LEVEL);

enum {
	ARG_IDX_DEV = 1,
	ARG_IDX_PARAM = 2,
	ARG_IDX_VALUE = 3,
};

struct stepper_microstep_map {
	const char *name;
	enum stepper_micro_step_resolution microstep;
};

struct stepper_direction_map {
	const char *name;
	enum stepper_direction direction;
};

#define STEPPER_DIRECTION_MAP_ENTRY(_name, _dir)                                                   \
	{                                                                                          \
		.name = _name,                                                                     \
		.direction = _dir,                                                                 \
	}

#define STEPPER_MICROSTEP_MAP(_name, _microstep)                                                   \
	{                                                                                          \
		.name = _name,                                                                     \
		.microstep = _microstep,                                                           \
	}

static void print_callback(const struct device *dev, const enum stepper_event event,
			   void *user_data)
{
	const struct shell *sh = user_data;
	if (!sh) {
		return;
	}

	switch (event) {
	case STEPPER_EVENT_STEPS_COMPLETED:
		shell_info(sh, "%s: Steps completed.", dev->name);
		break;
	case STEPPER_EVENT_STALL_DETECTED:
		shell_info(sh, "%s: Stall detected.", dev->name);
		break;
	case STEPPER_EVENT_LEFT_END_STOP_DETECTED:
		shell_info(sh, "%s: Left limit switch pressed.", dev->name);
		break;
	case STEPPER_EVENT_RIGHT_END_STOP_DETECTED:
		shell_info(sh, "%s: Right limit switch pressed.", dev->name);
		break;
	default:
		shell_info(sh, "%s: Unknown signal received.", dev->name);
		break;
	}
}

static const struct stepper_direction_map stepper_direction_map[] = {
	STEPPER_DIRECTION_MAP_ENTRY("positive", STEPPER_DIRECTION_POSITIVE),
	STEPPER_DIRECTION_MAP_ENTRY("negative", STEPPER_DIRECTION_NEGATIVE),
};

static const struct stepper_microstep_map stepper_microstep_map[] = {
	STEPPER_MICROSTEP_MAP("1", STEPPER_MICRO_STEP_1),
	STEPPER_MICROSTEP_MAP("2", STEPPER_MICRO_STEP_2),
	STEPPER_MICROSTEP_MAP("4", STEPPER_MICRO_STEP_4),
	STEPPER_MICROSTEP_MAP("8", STEPPER_MICRO_STEP_8),
	STEPPER_MICROSTEP_MAP("16", STEPPER_MICRO_STEP_16),
	STEPPER_MICROSTEP_MAP("32", STEPPER_MICRO_STEP_32),
	STEPPER_MICROSTEP_MAP("64", STEPPER_MICRO_STEP_64),
	STEPPER_MICROSTEP_MAP("128", STEPPER_MICRO_STEP_128),
	STEPPER_MICROSTEP_MAP("256", STEPPER_MICRO_STEP_256),
};

static void cmd_stepper_direction(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(stepper_direction_map)) {
		entry->syntax = stepper_direction_map[idx].name;
	} else {
		entry->syntax = NULL;
	}
	entry->handler = NULL;
	entry->help = "Stepper direction";
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_stepper_direction, cmd_stepper_direction);

static void cmd_stepper_microstep(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(stepper_microstep_map)) {
		entry->syntax = stepper_microstep_map[idx].name;
	} else {
		entry->syntax = NULL;
	}
	entry->handler = NULL;
	entry->help = "Stepper microstep resolution";
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_stepper_microstep, cmd_stepper_microstep);

static void cmd_pos_stepper_motor_name(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = "List Devices";
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_pos_stepper_motor_name, cmd_pos_stepper_motor_name);

static void cmd_pos_stepper_motor_name_dir(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	if (dev != NULL) {
		entry->syntax = dev->name;
	} else {
		entry->syntax = NULL;
	}
	entry->handler = NULL;
	entry->help = "List Devices";
	entry->subcmd = &dsub_stepper_direction;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_pos_stepper_motor_name_dir, cmd_pos_stepper_motor_name_dir);

static void cmd_pos_stepper_motor_name_microstep(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	if (dev != NULL) {
		entry->syntax = dev->name;
	} else {
		entry->syntax = NULL;
	}
	entry->handler = NULL;
	entry->help = "List Devices";
	entry->subcmd = &dsub_stepper_microstep;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_pos_stepper_motor_name_microstep,
			 cmd_pos_stepper_motor_name_microstep);

static int parse_device_arg(const struct shell *sh, char **argv, const struct device **dev)
{
	*dev = device_get_binding(argv[ARG_IDX_DEV]);
	if (!*dev) {
		shell_error(sh, "Stepper device %s not found", argv[ARG_IDX_DEV]);
		return -ENODEV;
	}
	return 0;
}

static int cmd_stepper_enable(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err = 0;
	bool enable = shell_strtobool(argv[ARG_IDX_PARAM], 10, &err);

	if (err < 0) {
		return err;
	}

	err = parse_device_arg(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = stepper_enable(dev, enable);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_stepper_move(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err = 0;

	int32_t micro_steps = shell_strtol(argv[ARG_IDX_PARAM], 10, &err);

	if (err < 0) {
		return err;
	}

	err = parse_device_arg(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = stepper_set_event_callback(dev, print_callback, (void *)sh);
	if (err != 0) {
		shell_error(sh, "Failed to set callback: %d", err);
	}

	err = stepper_move(dev, micro_steps);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_stepper_set_max_velocity(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err = 0;
	uint32_t velocity = shell_strtoul(argv[ARG_IDX_PARAM], 10, &err);

	if (err < 0) {
		return err;
	}

	err = parse_device_arg(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = stepper_set_max_velocity(dev, velocity);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_stepper_set_micro_step_res(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	enum stepper_micro_step_resolution resolution;
	int err = -EINVAL;

	for (int i = 0; i < ARRAY_SIZE(stepper_microstep_map); i++) {
		if (strcmp(argv[ARG_IDX_PARAM], stepper_microstep_map[i].name) == 0) {
			resolution = stepper_microstep_map[i].microstep;
			err = 0;
			break;
		}
	}
	if (err != 0) {
		shell_error(sh, "Invalid microstep value %s", argv[ARG_IDX_PARAM]);
		return err;
	}

	err = parse_device_arg(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = stepper_set_micro_step_res(dev, resolution);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_stepper_get_micro_step_res(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;
	enum stepper_micro_step_resolution micro_step_res;

	err = parse_device_arg(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = stepper_get_micro_step_res(dev, &micro_step_res);
	if (err < 0) {
		shell_warn(sh, "Failed to get micro-step resolution: %d", err);
	} else {
		shell_print(sh, "Micro-step Resolution: %d", micro_step_res);
	}

	return err;
}

static int cmd_stepper_set_reference_position(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err = 0;
	int32_t position = shell_strtol(argv[ARG_IDX_PARAM], 10, &err);

	if (err < 0) {
		return err;
	}

	err = parse_device_arg(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = stepper_set_reference_position(dev, position);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_stepper_get_actual_position(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;
	int32_t actual_position;

	err = parse_device_arg(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = stepper_get_actual_position(dev, &actual_position);
	if (err < 0) {
		shell_warn(sh, "Failed to get actual position: %d", err);
	} else {
		shell_print(sh, "Actual Position: %d", actual_position);
	}

	return err;
}

static int cmd_stepper_set_target_position(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err = 0;
	const int32_t position = shell_strtol(argv[ARG_IDX_PARAM], 10, &err);

	if (err < 0) {
		return err;
	}

	err = parse_device_arg(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = stepper_set_event_callback(dev, print_callback, (void *)sh);
	if (err != 0) {
		shell_error(sh, "Failed to set callback: %d", err);
	}

	err = stepper_set_target_position(dev, position);
	if (err) {
		shell_error(sh, "Error: %d", err);
	}

	return err;
}

static int cmd_stepper_run(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err = -EINVAL;
	enum stepper_direction direction = STEPPER_DIRECTION_POSITIVE;

	for (int i = 0; i < ARRAY_SIZE(stepper_direction_map); i++) {
		if (strcmp(argv[ARG_IDX_PARAM], stepper_direction_map[i].name) == 0) {
			direction = stepper_direction_map[i].direction;
			err = 0;
			break;
		}
	}
	if (err != 0) {
		shell_error(sh, "Invalid direction %s", argv[ARG_IDX_PARAM]);
		return err;
	}

	uint32_t velocity = shell_strtoul(argv[ARG_IDX_VALUE], 10, &err);

	if (err < 0) {
		return err;
	}

	err = parse_device_arg(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	err = stepper_set_event_callback(dev, print_callback, (void *)sh);
	if (err != 0) {
		shell_error(sh, "Failed to set callback: %d", err);
	}

	err = stepper_run(dev, direction, velocity);
	if (err) {
		shell_error(sh, "Error: %d", err);
		return err;
	}

	return 0;
}

static int cmd_stepper_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	int err;
	bool is_moving;
	int32_t actual_position;
	enum stepper_micro_step_resolution micro_step_res;

	err = parse_device_arg(sh, argv, &dev);
	if (err < 0) {
		return err;
	}

	shell_print(sh, "Stepper Info:");
	shell_print(sh, "Device: %s", dev->name);

	err = stepper_get_actual_position(dev, &actual_position);
	if (err < 0) {
		shell_warn(sh, "Failed to get actual position: %d", err);
	} else {
		shell_print(sh, "Actual Position: %d", actual_position);
	}

	err = stepper_get_micro_step_res(dev, &micro_step_res);
	if (err < 0) {
		shell_warn(sh, "Failed to get micro-step resolution: %d", err);
	} else {
		shell_print(sh, "Micro-step Resolution: %d", micro_step_res);
	}

	err = stepper_is_moving(dev, &is_moving);
	if (err < 0) {
		shell_warn(sh, "Failed to check if the motor is moving: %d", err);
	} else {
		shell_print(sh, "Is Moving: %s", is_moving ? "Yes" : "No");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	stepper_cmds,
	SHELL_CMD_ARG(enable, &dsub_pos_stepper_motor_name, "<device> <on/off>", cmd_stepper_enable,
		      3, 0),
	SHELL_CMD_ARG(move, &dsub_pos_stepper_motor_name, "<device> <micro_steps>",
		      cmd_stepper_move, 3, 0),
	SHELL_CMD_ARG(set_max_velocity, &dsub_pos_stepper_motor_name, "<device> <velocity>",
		      cmd_stepper_set_max_velocity, 3, 0),
	SHELL_CMD_ARG(set_micro_step_res, &dsub_pos_stepper_motor_name_microstep,
		      "<device> <resolution>", cmd_stepper_set_micro_step_res, 3, 0),
	SHELL_CMD_ARG(get_micro_step_res, &dsub_pos_stepper_motor_name, "<device>",
		      cmd_stepper_get_micro_step_res, 2, 0),
	SHELL_CMD_ARG(set_reference_position, &dsub_pos_stepper_motor_name, "<device> <position>",
		      cmd_stepper_set_reference_position, 3, 0),
	SHELL_CMD_ARG(get_actual_position, &dsub_pos_stepper_motor_name, "<device>",
		      cmd_stepper_get_actual_position, 2, 0),
	SHELL_CMD_ARG(set_target_position, &dsub_pos_stepper_motor_name, "<device> <micro_steps>",
		      cmd_stepper_set_target_position, 3, 0),
	SHELL_CMD_ARG(run, &dsub_pos_stepper_motor_name_dir, "<device> <direction> <velocity>",
		      cmd_stepper_run, 4, 0),
	SHELL_CMD_ARG(info, &dsub_pos_stepper_motor_name, "<device>", cmd_stepper_info, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(stepper, &stepper_cmds, "Stepper motor commands", NULL);
