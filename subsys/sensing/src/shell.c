#include <zephyr/sensing/sensing.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define SENSING_INFO_HELP "Get sensor info, such as vendor and model name, for all sensors."

#define SENSING_OPEN_HELP                                                                          \
	"Open a new connection or list current open connections if no argument"                    \
	" is provided:\n"                                                                          \
	"[sensor_index]"

#define SENSING_CLOSE_HELP "Close an existing connection:\n<connection_index>"

#define SENSING_CONFIG_HELP                                                                        \
	"Configure an existing connection:\n"                                                      \
	"<connection_index> <mode> [<attribute_name_0> <value_0> ... <attribute_name_N> "          \
	"<value_N>]"

static const char *mode_string_map[] = {
	[SENSING_SENSOR_MODE_CONTINUOUS] = "continuous",
	[SENSING_SENSOR_MODE_ONE_SHOT] = "one_shot",
	[SENSING_SENSOR_MODE_PASSIVE_CONTINUOUS] = "passive_continuous",
	[SENSING_SENSOR_MODE_PASSIVE_ONE_SHOT] = "passive_one_shot",
	[SENSING_SENSOR_MODE_DONE] = "done",
};

static int parse_sensor_mode(const char *arg, enum sensing_sensor_mode *mode)
{
	for (int i = 0; i < ARRAY_SIZE(mode_string_map); ++i) {
		if (strcmp(mode_string_map[i], arg) == 0) {
			*mode = i;
			return 0;
		}
	}
	return -1;
}

static const char *sensor_attribute_name[SENSOR_ATTR_COMMON_COUNT] = {
	[SENSOR_ATTR_SAMPLING_FREQUENCY] = "sampling_frequency",
	[SENSOR_ATTR_LOWER_THRESH] = "lower_thresh",
	[SENSOR_ATTR_UPPER_THRESH] = "upper_thresh",
	[SENSOR_ATTR_SLOPE_TH] = "slope_th",
	[SENSOR_ATTR_SLOPE_DUR] = "slope_dur",
	[SENSOR_ATTR_HYSTERESIS] = "hysteresis",
	[SENSOR_ATTR_OVERSAMPLING] = "oversampling",
	[SENSOR_ATTR_FULL_SCALE] = "full_scale",
	[SENSOR_ATTR_OFFSET] = "offset",
	[SENSOR_ATTR_CALIB_TARGET] = "calib_target",
	[SENSOR_ATTR_CONFIGURATION] = "configuration",
	[SENSOR_ATTR_CALIBRATION] = "calibration",
	[SENSOR_ATTR_FEATURE_MASK] = "feature_mask",
	[SENSOR_ATTR_ALERT] = "alert",
	[SENSOR_ATTR_FF_DUR] = "ff_dur",
	[SENSOR_ATTR_FIFO_WATERMARK] = "fifo_wm",
};

static int parse_sensor_attribute(const char *arg, enum sensor_attribute *attr)
{
	for (int i = 0; i < ARRAY_SIZE(sensor_attribute_name); ++i) {
		if (strcmp(sensor_attribute_name[i], arg) == 0) {
			*attr = i;
			return 0;
		}
	}
	return -1;
}

#define SENSOR_TYPE_TO_STRING(type)                                                                \
	case type:                                                                                 \
		return #type

static const char *get_sensor_type_string(int32_t type)
{
	switch (type) {
		SENSOR_TYPE_TO_STRING(SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D);
		SENSOR_TYPE_TO_STRING(SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D);
		SENSOR_TYPE_TO_STRING(SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE);
	default:
		return "UNKNOWN";
	}
}

static inline void print_sensor_info(const struct shell *sh, int index,
				     const struct sensing_sensor_info *sensor)
{
	const char *null_str = "(null)";

	shell_print(sh,
		    "[%d] %s\n    device name: %s, vendor: %s, model: %s, "
		    "friendly name: %s",
		    index, get_sensor_type_string(sensor->type),
		    sensor->info->dev ? sensor->info->dev->name : "VIRTUAL",
		    sensor->info->vendor ? sensor->info->vendor : null_str,
		    sensor->info->model ? sensor->info->model : null_str,
		    sensor->info->friendly_name ? sensor->info->friendly_name : null_str);
}

static int cmd_get_sensor_info(const struct shell *sh, size_t argc, char **argv)
{
	const struct sensing_sensor_info *sensors;
	int num_sensors;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int rc = sensing_get_sensors(&num_sensors, &sensors);
	if (rc != 0) {
		shell_error(sh, "Failed to get sensor list");
		return rc;
	}

	if (num_sensors == 0) {
		shell_warn(sh, "No sensors found");
		return 0;
	}

	for (int i = 0; i < num_sensors; ++i) {
		print_sensor_info(sh, i, &sensors[i]);
	}
	return 0;
}

struct shell_cmd_connection {
	sensing_sensor_handle_t handle;
	bool is_used;
	char shell_name[5];
	const struct shell *owning_shell;
};
static struct shell_cmd_connection open_connections[CONFIG_SENSING_MAX_DYNAMIC_CONNECTIONS];

static void sensing_shell_print_three_axis_data(const struct shell *sh,
						const struct sensing_sensor_info *info,
						const struct sensing_sensor_three_axis_data *data)
{
	int64_t intermediate[] = {
		(int64_t)data->readings[0].values[0],
		(int64_t)data->readings[0].values[1],
		(int64_t)data->readings[0].values[2],
	};

//	shell_info(sh,
//		   "%s: shift=%d, mask=0x%016" PRIx64 " ("
//		   "0x%08" PRIx32 "/0x%016" PRIx64 ", "
//		   "0x%08" PRIx32 "/0x%016" PRIx64 ", "
//		   "0x%08" PRIx32 "/0x%016" PRIx64 ")",
//		   get_sensor_type_string(info->type), data->shift, GENMASK64(63, 31),
//		   data->readings[0].values[0], intermediate[0], data->readings[0].values[1],
//		   intermediate[1], data->readings[0].values[2], intermediate[2]);

	intermediate[0] = (data->shift >= 0) ? (intermediate[0] << data->shift)
					     : (intermediate[0] >> -data->shift);
	intermediate[1] = (data->shift >= 0) ? (intermediate[1] << data->shift)
					     : (intermediate[1] >> -data->shift);
	intermediate[2] = (data->shift >= 0) ? (intermediate[2] << data->shift)
					     : (intermediate[2] >> -data->shift);
	//	shell_info(sh,
	//		   "%s: %" PRIi32 ".%06" PRIu32 ", %" PRIi32 ".%06" PRIu32 ", %" PRIi32
	//		   ".%06" PRIu32,
	//		   get_sensor_type_string(info->type),
	//		   (int32_t)FIELD_GET(GENMASK64(63, 31), intermediate[0]),
	//		   (uint32_t)((uint64_t)FIELD_GET(GENMASK64(30, 0), intermediate[0]) *
	//			      INT64_C(1000000) / INT32_MAX),
	//		   (int32_t)FIELD_GET(GENMASK64(63, 31), intermediate[1]),
	//		   (uint32_t)((uint64_t)FIELD_GET(GENMASK64(30, 0), intermediate[1]) *
	//			      INT64_C(1000000) / INT32_MAX),
	//		   (int32_t)FIELD_GET(GENMASK64(63, 31), intermediate[2]),
	//		   (uint32_t)((uint64_t)FIELD_GET(GENMASK64(30, 0), intermediate[2]) *
	//			      INT64_C(1000000) / INT32_MAX));
	shell_info(sh,
		   "%s %" PRIi32 ".%06" PRIu32 ", %" PRIi32 ".%06" PRIu32 ", %" PRIi32
		   ".%06" PRIu32,
		   get_sensor_type_string(info->type), (int32_t)(intermediate[0] / INT32_MAX),
		   (uint32_t)((intermediate[0] % INT32_MAX) * INT64_C(1000000) / INT32_MAX),
		   (int32_t)(intermediate[1] / INT32_MAX),
		   (uint32_t)((intermediate[1] % INT32_MAX) * INT64_C(1000000) / INT32_MAX),
		   (int32_t)(intermediate[2] / INT32_MAX),
		   (uint32_t)((intermediate[2] % INT32_MAX) * INT64_C(1000000) / INT32_MAX));
}

static void sensing_shell_print_float_data(const struct shell *sh,
					   const struct sensing_sensor_info *info,
					   const struct sensing_sensor_float_data *data)
{
	int64_t intermediate = (int64_t)data->readings[0].v;

	intermediate =
		(data->shift >= 0) ? (intermediate << data->shift) : (intermediate >> -data->shift);

	shell_info(sh, "%s: %" PRIi32 ".%06" PRIu32, get_sensor_type_string(info->type),
		   (int32_t)(intermediate / INT32_MAX),
		   (uint32_t)((intermediate % INT32_MAX) * INT64_C(1000000) / INT32_MAX));
}

static void sensing_shell_on_data_event(sensing_sensor_handle_t handle, const void *data,
					void *userdata)
{
	struct shell_cmd_connection *connection = userdata;
	const struct sensing_sensor_info *info = sensing_get_sensor_info(handle);

	switch (info->type) {
	case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
	case SENSING_SENSOR_TYPE_MOTION_UNCALIB_ACCELEROMETER_3D:
	case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
		sensing_shell_print_three_axis_data(connection->owning_shell, info, data);
		break;
	case SENSING_SENSOR_TYPE_MOTION_HINGE_ANGLE:
		sensing_shell_print_float_data(connection->owning_shell, info, data);
		break;
	default:
		shell_info(connection->owning_shell, "Got data for '%s' at %p",
			   get_sensor_type_string(info->type), data);
	}
}

static const struct sensing_callback_list callback_list = {
	.on_data_event = sensing_shell_on_data_event,
};

static int cmd_open_connection(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		/* List open connections */
		bool has_connections = false;
		for (int i = 0; i < ARRAY_SIZE(open_connections); ++i) {
			if (open_connections[i].is_used) {
				print_sensor_info(
					sh, i, sensing_get_sensor_info(open_connections[i].handle));
				has_connections = true;
			}
		}
		if (!has_connections) {
			shell_print(sh, "No open connections");
		}
		return 0;
	}

	const struct sensing_sensor_info *sensors;
	int num_sensors;
	char *endptr;
	long sensor_index = strtol(argv[1], &endptr, 0);
	int rc = sensing_get_sensors(&num_sensors, &sensors);

	if (rc != 0) {
		shell_error(sh, "Failed to get sensor list");
		return rc;
	}

	if (sensor_index < 0 || sensor_index >= num_sensors) {
		shell_error(sh, "Sensor index (%ld) out of bounds, valid range is 0-%d",
			    sensor_index, num_sensors - 1);
		return -EINVAL;
	}

	int connection_idx = -1;
	for (int i = 0; i < ARRAY_SIZE(open_connections); ++i) {
		if (!open_connections[i].is_used) {
			connection_idx = i;
			break;
		}
	}

	if (connection_idx < 0) {
		shell_error(sh,
			    "No more memory for connections, close a connection then try again");
		return -ENOMEM;
	}

	rc = sensing_open_sensor(&sensors[sensor_index], &callback_list,
				 &open_connections[connection_idx].handle,
				 &open_connections[connection_idx]);

	if (rc != 0) {
		shell_error(sh, "Failed to open connection");
		return rc;
	}

	open_connections[connection_idx].is_used = true;
	open_connections[connection_idx].owning_shell = sh;
	shell_print(sh, "New connection [%d] to sensor [%ld] created", connection_idx,
		    sensor_index);

	return 0;
}

static int cmd_close_connection(const struct shell *sh, size_t argc, char **argv)
{
	char *endptr;
	long connection_index = strtol(argv[1], &endptr, 0);

	if (!open_connections[connection_index].is_used) {
		shell_error(
			sh,
			"Invalid connection number, run 'sensing open' to see current connections");
		return -EINVAL;
	}

	int rc = sensing_close_sensor(open_connections[connection_index].handle);
	if (rc != 0) {
		shell_error(sh, "Failed to close connection (%ld)", connection_index);
		return rc;
	}
	open_connections[connection_index].is_used = false;
	return 0;
}

static int parse_sensor_value(const struct shell *sh, const char *val_str, q31_t *q, int8_t *shift)
{
	const bool is_negative = val_str[0] == '-';
	const char *decimal_pos = strchr(val_str, '.');
	long value;
	int64_t micro_value;
	char *endptr;

	/* Parse int portion */
	value = strtol(val_str, &endptr, 0);

	if (*endptr != '\0' && *endptr != '.') {
		return -EINVAL;
	}
	if (value > INT32_MAX || value < INT32_MIN) {
		return -EINVAL;
	}

	*shift = ilog2(labs(value) - 1) + 1;
	micro_value = value * 1000000;

	if (decimal_pos == NULL) {
		goto end;
	}

	/* Parse the decimal portion */
	value = strtoul(decimal_pos + 1, &endptr, 0);
	if (*endptr != '\0') {
		return -EINVAL;
	}
	while (value < 100000) {
		value *= 10;
	}
	if (value > INT32_C(999999)) {
		return -EINVAL;
	}
	if (is_negative) {
		value *= -1;
	}
	micro_value += value;
end:
	*q = ((micro_value * (INT64_C(1) << 31)) / 1000000) >> *shift;
	*shift += 1;
	return 0;
}

static int cmd_config(const struct shell *sh, size_t argc, char **argv)
{
	int rc;
	char *endptr;
	long connection_index = strtol(argv[1], &endptr, 0);

	if (!open_connections[connection_index].is_used) {
		shell_error(
			sh,
			"Invalid connection number, run 'sensing open' to see current connections");
		return -EINVAL;
	}

	enum sensing_sensor_mode mode;
	rc = parse_sensor_mode(argv[2], &mode);
	if (rc != 0) {
		shell_error(sh, "Invalid mode '%s'", argv[2]);
		return -EINVAL;
	}

	int config_count = 0;
	struct sensing_sensor_attribute configs[4] = {0};
	if (argc % 2 == 0) {
		shell_error(sh, "Invalid config, must use pairs of <attr> <val>");
		return -EINVAL;
	}
	for (int i = 3; i < argc; i += 2) {
		rc = parse_sensor_attribute(argv[i], &configs[config_count].attribute);
		if (rc != 0) {
			shell_error(sh, "Invalid attribute '%s'", argv[i]);
			return -EINVAL;
		}
		rc = parse_sensor_value(sh, argv[i + 1], &configs[config_count].value,
					&configs[config_count].shift);
		if (rc != 0) {
			shell_error(sh, "Invalid value '%s'", argv[i + 1]);
			return -EINVAL;
		}
		config_count++;
	}

	rc = sensing_set_attributes(open_connections[connection_index].handle, mode, configs,
				    config_count);
	if (rc != 0) {
		shell_error(sh, "Failed to set attribute '%s' to '%s'", argv[2], argv[3]);
		return rc;
	}
	return 0;
}

static void sensing_node_index_get(size_t idx, struct shell_static_entry *entry)
{
	size_t count;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	STRUCT_SECTION_START_EXTERN(sensing_sensor_info);
	STRUCT_SECTION_COUNT(sensing_sensor_info, &count);
	if (idx >= count || idx > 9999) {
		return;
	}
	snprintk(STRUCT_SECTION_START(sensing_sensor_info)[idx].shell_name, 5, "%d", (int)idx);
	entry->syntax = STRUCT_SECTION_START(sensing_sensor_info)[idx].shell_name;
}
SHELL_DYNAMIC_CMD_CREATE(dsub_node_index, sensing_node_index_get);

static void sensing_connection_node_index_get(size_t idx, struct shell_static_entry *entry,
					      const union shell_cmd_entry *subcmd)
{
	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	if (idx >= ARRAY_SIZE(open_connections) || !open_connections[idx].is_used) {
		return;
	}

	snprintk(open_connections[idx].shell_name, 5, "%d", (int)idx);
	entry->syntax = open_connections[idx].shell_name;
	entry->subcmd = subcmd;
}

static void sensing_connection_node_index_get_for_close(size_t idx,
							struct shell_static_entry *entry)
{
	sensing_connection_node_index_get(idx, entry, NULL);
}
SHELL_DYNAMIC_CMD_CREATE(dsub_connection_node_index_for_close,
			 sensing_connection_node_index_get_for_close);

// static void sensing_attr_for_config(size_t idx, struct shell_static_entry *entry);
// SHELL_DYNAMIC_CMD_CREATE(dsub_sensor_attr_for_config, sensing_attr_for_config);
//
// static void sensing_attr_val_for_config(size_t idx, struct shell_static_entry *entry)
//{
//	entry->syntax = NULL;
//	entry->subcmd = &dsub_sensor_attr_for_config;
// }
// SHELL_DYNAMIC_CMD_CREATE(dsub_sensor_attr_val_for_config, sensing_attr_val_for_config);
//
// static void sensing_attr_for_config(size_t idx, struct shell_static_entry *entry)
//{
//	if (idx >= ARRAY_SIZE(sensor_attribute_name)) {
//		entry->syntax = NULL;
//		return;
//	}
//
//	entry->syntax = sensor_attribute_name[idx];
//	entry->subcmd = &dsub_sensor_attr_val_for_config;
// }

static void sensing_sensor_mode_for_config(size_t idx, struct shell_static_entry *entry)
{
	if (idx >= ARRAY_SIZE(mode_string_map)) {
		entry->syntax = NULL;
		return;
	}

	entry->syntax = mode_string_map[idx];
	entry->subcmd = NULL; //&dsub_sensor_attr_for_config;
}
SHELL_DYNAMIC_CMD_CREATE(dsub_sensor_mode_for_config, sensing_sensor_mode_for_config);

static void sensing_connection_node_index_get_for_config(size_t idx,
							 struct shell_static_entry *entry)
{
	sensing_connection_node_index_get(idx, entry, &dsub_sensor_mode_for_config);
}
SHELL_DYNAMIC_CMD_CREATE(dsub_connection_node_index_for_config,
			 sensing_connection_node_index_get_for_config);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sensing, SHELL_CMD_ARG(info, NULL, SENSING_INFO_HELP, cmd_get_sensor_info, 1, 0),
	SHELL_CMD_ARG(open, &dsub_node_index, SENSING_OPEN_HELP, cmd_open_connection, 1, 1),
	SHELL_CMD_ARG(close, &dsub_connection_node_index_for_close, SENSING_CLOSE_HELP,
		      cmd_close_connection, 2, 0),
	SHELL_CMD_ARG(config, &dsub_connection_node_index_for_config, SENSING_CONFIG_HELP,
		      cmd_config, 3, 11),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(sensing, &sub_sensing, "Sensing subsystem commands", NULL);
