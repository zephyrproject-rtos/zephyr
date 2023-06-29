#include <zephyr/sensing/sensing.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

#define SENSING_INFO_HELP "Get sensor info, such as vendor and model name, for all sensors."

#define SENSING_OPEN_HELP                                                                          \
	"Open a new connection or list current open connections if no argument"                    \
	" is provided:\n"                                                                          \
	"[sensor_index]"

#define SENSING_CLOSE_HELP "Close an existing connection:\n<connection_index>"

#define SENSING_CONFIG_HELP                                                                        \
	"Configure an existing connection:\n"                                                      \
	"<connection_index> <attribute_name> <value>"

#define SENSOR_TYPE_TO_STRING(type)                                                                \
	case type:                                                                                 \
		return #type

static const char *get_sensor_type_string(int32_t type)
{
	switch (type) {
		SENSOR_TYPE_TO_STRING(SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D);
		SENSOR_TYPE_TO_STRING(SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D);
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
};
static struct shell_cmd_connection open_connections[CONFIG_SENSING_MAX_CONNECTIONS];

static const struct sensing_callback_list callback_list = {
	.on_data_event = NULL,
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
				 &open_connections[connection_idx].handle);

	if (rc != 0) {
		shell_error(sh, "Failed to open connection");
		return rc;
	}

	open_connections[connection_idx].is_used = true;
	shell_print(sh, "New connection [%d] to sensor [%ld] created", connection_idx, sensor_index);

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
	char *endptr;
	long connection_index = strtol(argv[1], &endptr, 0);

	if (!open_connections[connection_index].is_used) {
		shell_error(
			sh,
			"Invalid connection number, run 'sensing open' to see current connections");
		return -EINVAL;
	}

	enum sensor_attribute attribute;
	if (strcmp("sampling_frequency", argv[2]) == 0) {
		attribute = SENSOR_ATTR_SAMPLING_FREQUENCY;
	} else {
		shell_error(sh, "Invalid attribute '%s'", argv[2]);
		return -EINVAL;
	}

	struct sensing_sensor_attribute config = {
		.attribute = attribute,
	};
	int rc = parse_sensor_value(sh, argv[3], &config.value, &config.shift);

	if (rc != 0) {
		shell_error(sh, "Invalid value '%s'", argv[3]);
		return -EINVAL;
	}

	rc = sensing_set_attributes(open_connections[connection_index].handle, &config, 1);
	if (rc != 0) {
		shell_error(sh, "Failed to set attribute '%s' to '%s'", argv[2], argv[3]);
		return rc;
	}
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sensing, SHELL_CMD_ARG(info, NULL, SENSING_INFO_HELP, cmd_get_sensor_info, 1, 0),
	SHELL_CMD_ARG(open, NULL, SENSING_OPEN_HELP, cmd_open_connection, 1, 1),
	SHELL_CMD_ARG(close, NULL, SENSING_CLOSE_HELP, cmd_close_connection, 2, 0),
	SHELL_CMD_ARG(config, NULL, SENSING_CONFIG_HELP, cmd_config, 4, 0), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(sensing, &sub_sensing, "Sensing subsystem commands", NULL);
