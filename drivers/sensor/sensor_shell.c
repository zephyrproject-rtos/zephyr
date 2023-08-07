/*
 * Copyright (c) 2018 Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sensor_shell);

#define SENSOR_GET_HELP                                                                            \
	"Get sensor data. Channel names are optional. All channels are read "                      \
	"when no channels are provided. Syntax:\n"                                                 \
	"<device_name> <channel name 0> .. <channel name N>"

#define SENSOR_ATTR_GET_HELP                                                                       \
	"Get the sensor's channel attribute. Syntax:\n"                                            \
	"<device_name> [<channel_name 0> <attribute_name 0> .. "                                   \
	"<channel_name N> <attribute_name N>]"

#define SENSOR_ATTR_SET_HELP                                                                       \
	"Set the sensor's channel attribute.\n"                                                    \
	"<device_name> <channel_name> <attribute_name> <value>"

#define SENSOR_INFO_HELP "Get sensor info, such as vendor and model name, for all sensors."

#define SENSOR_TRIG_HELP                                                                           \
	"Get or set the trigger type on a sensor. Currently only supports `data_ready`.\n"         \
	"<device_name> <on/off> <trigger_name>"

const char *sensor_channel_name[SENSOR_CHAN_ALL] = {
	[SENSOR_CHAN_ACCEL_X] = "accel_x",
	[SENSOR_CHAN_ACCEL_Y] = "accel_y",
	[SENSOR_CHAN_ACCEL_Z] = "accel_z",
	[SENSOR_CHAN_ACCEL_XYZ] = "accel_xyz",
	[SENSOR_CHAN_GYRO_X] = "gyro_x",
	[SENSOR_CHAN_GYRO_Y] = "gyro_y",
	[SENSOR_CHAN_GYRO_Z] = "gyro_z",
	[SENSOR_CHAN_GYRO_XYZ] = "gyro_xyz",
	[SENSOR_CHAN_MAGN_X] = "magn_x",
	[SENSOR_CHAN_MAGN_Y] = "magn_y",
	[SENSOR_CHAN_MAGN_Z] = "magn_z",
	[SENSOR_CHAN_MAGN_XYZ] = "magn_xyz",
	[SENSOR_CHAN_DIE_TEMP] = "die_temp",
	[SENSOR_CHAN_AMBIENT_TEMP] = "ambient_temp",
	[SENSOR_CHAN_PRESS] = "press",
	[SENSOR_CHAN_PROX] = "prox",
	[SENSOR_CHAN_HUMIDITY] = "humidity",
	[SENSOR_CHAN_LIGHT] = "light",
	[SENSOR_CHAN_IR] = "ir",
	[SENSOR_CHAN_RED] = "red",
	[SENSOR_CHAN_GREEN] = "green",
	[SENSOR_CHAN_BLUE] = "blue",
	[SENSOR_CHAN_ALTITUDE] = "altitude",
	[SENSOR_CHAN_PM_1_0] = "pm_1_0",
	[SENSOR_CHAN_PM_2_5] = "pm_2_5",
	[SENSOR_CHAN_PM_10] = "pm_10",
	[SENSOR_CHAN_DISTANCE] = "distance",
	[SENSOR_CHAN_CO2] = "co2",
	[SENSOR_CHAN_VOC] = "voc",
	[SENSOR_CHAN_GAS_RES] = "gas_resistance",
	[SENSOR_CHAN_VOLTAGE] = "voltage",
	[SENSOR_CHAN_CURRENT] = "current",
	[SENSOR_CHAN_POWER] = "power",
	[SENSOR_CHAN_RESISTANCE] = "resistance",
	[SENSOR_CHAN_ROTATION] = "rotation",
	[SENSOR_CHAN_POS_DX] = "pos_dx",
	[SENSOR_CHAN_POS_DY] = "pos_dy",
	[SENSOR_CHAN_POS_DZ] = "pos_dz",
	[SENSOR_CHAN_RPM] = "rpm",
	[SENSOR_CHAN_GAUGE_VOLTAGE] = "gauge_voltage",
	[SENSOR_CHAN_GAUGE_AVG_CURRENT] = "gauge_avg_current",
	[SENSOR_CHAN_GAUGE_STDBY_CURRENT] = "gauge_stdby_current",
	[SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT] = "gauge_max_load_current",
	[SENSOR_CHAN_GAUGE_TEMP] = "gauge_temp",
	[SENSOR_CHAN_GAUGE_STATE_OF_CHARGE] = "gauge_state_of_charge",
	[SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY] = "gauge_full_cap",
	[SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY] = "gauge_remaining_cap",
	[SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY] = "gauge_nominal_cap",
	[SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY] = "gauge_full_avail_cap",
	[SENSOR_CHAN_GAUGE_AVG_POWER] = "gauge_avg_power",
	[SENSOR_CHAN_GAUGE_STATE_OF_HEALTH] = "gauge_state_of_health",
	[SENSOR_CHAN_GAUGE_TIME_TO_EMPTY] = "gauge_time_to_empty",
	[SENSOR_CHAN_GAUGE_TIME_TO_FULL] = "gauge_time_to_full",
	[SENSOR_CHAN_GAUGE_CYCLE_COUNT] = "gauge_cycle_count",
	[SENSOR_CHAN_GAUGE_DESIGN_VOLTAGE] = "gauge_design_voltage",
	[SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE] = "gauge_desired_voltage",
	[SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT] = "gauge_desired_charging_current",
};

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
};

/* Forward declaration */
static void data_ready_trigger_handler(const struct device *sensor,
				       const struct sensor_trigger *trigger);

#define TRIGGER_DATA_ENTRY(trig_enum, str_name, handler_func)                                      \
	[(trig_enum)] = {.name = #str_name,                                                        \
			 .handler = (handler_func),                                                \
			 .trigger = {.chan = SENSOR_CHAN_ALL, .type = (trig_enum)}}

/**
 * @brief This table stores a mapping of string trigger names along with the sensor_trigger struct
 * that gets passed to the driver to enable that trigger, plus a function pointer to a handler. If
 * that pointer is NULL, this indicates there is not currently support for that trigger type in the
 * sensor shell.
 */
static const struct {
	const char *name;
	sensor_trigger_handler_t handler;
	struct sensor_trigger trigger;
} sensor_trigger_table[SENSOR_TRIG_COMMON_COUNT] = {
	TRIGGER_DATA_ENTRY(SENSOR_TRIG_TIMER, timer, NULL),
	TRIGGER_DATA_ENTRY(SENSOR_TRIG_DATA_READY, data_ready, data_ready_trigger_handler),
	TRIGGER_DATA_ENTRY(SENSOR_TRIG_DELTA, delta, NULL),
	TRIGGER_DATA_ENTRY(SENSOR_TRIG_NEAR_FAR, near_far, NULL),
	TRIGGER_DATA_ENTRY(SENSOR_TRIG_THRESHOLD, threshold, NULL),
	TRIGGER_DATA_ENTRY(SENSOR_TRIG_TAP, tap, NULL),
	TRIGGER_DATA_ENTRY(SENSOR_TRIG_DOUBLE_TAP, double_tap, NULL),
	TRIGGER_DATA_ENTRY(SENSOR_TRIG_FREEFALL, freefall, NULL),
	TRIGGER_DATA_ENTRY(SENSOR_TRIG_MOTION, motion, NULL),
	TRIGGER_DATA_ENTRY(SENSOR_TRIG_STATIONARY, stationary, NULL),
};

enum dynamic_command_context {
	NONE,
	CTX_GET,
	CTX_ATTR_GET_SET,
};

static enum dynamic_command_context current_cmd_ctx = NONE;

/* Mutex for accessing shared RTIO/IODEV data structures */
K_MUTEX_DEFINE(cmd_get_mutex);

/* Crate a single common config for one-shot reading */
static enum sensor_channel iodev_sensor_shell_channels[SENSOR_CHAN_ALL];
static struct sensor_read_config iodev_sensor_shell_read_config = {
	.sensor = NULL,
	.channels = iodev_sensor_shell_channels,
	.count = 0,
	.max = ARRAY_SIZE(iodev_sensor_shell_channels),
};
RTIO_IODEV_DEFINE(iodev_sensor_shell_read, &__sensor_iodev_api, &iodev_sensor_shell_read_config);

/* Create the RTIO context to service the reading */
RTIO_DEFINE_WITH_MEMPOOL(sensor_read_rtio, 8, 8, 32, 64, 4);

static int parse_named_int(const char *name, const char *heystack[], size_t count)
{
	char *endptr;
	int i;

	/* Attempt to parse channel name as a number first */
	i = strtoul(name, &endptr, 0);

	if (*endptr == '\0') {
		return i;
	}

	/* Channel name is not a number, look it up */
	for (i = 0; i < count; i++) {
		if (strcmp(name, heystack[i]) == 0) {
			return i;
		}
	}

	return -ENOTSUP;
}

static int parse_sensor_value(const char *val_str, struct sensor_value *out)
{
	const bool is_negative = val_str[0] == '-';
	const char *decimal_pos = strchr(val_str, '.');
	long value;
	char *endptr;

	/* Parse int portion */
	value = strtol(val_str, &endptr, 0);

	if (*endptr != '\0' && *endptr != '.') {
		return -EINVAL;
	}
	if (value > INT32_MAX || value < INT32_MIN) {
		return -EINVAL;
	}
	out->val1 = (int32_t)value;

	if (decimal_pos == NULL) {
		return 0;
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
	out->val2 = (int32_t)value;
	if (is_negative) {
		out->val2 *= -1;
	}
	return 0;
}

struct sensor_shell_processing_context {
	const struct device *dev;
	const struct shell *sh;
};

static void sensor_shell_processing_callback(int result, uint8_t *buf, uint32_t buf_len,
					     void *userdata)
{
	struct sensor_shell_processing_context *ctx = userdata;
	const struct sensor_decoder_api *decoder;
	uint8_t decoded_buffer[128];
	int rc;

	ARG_UNUSED(buf_len);

	if (result < 0) {
		shell_error(ctx->sh, "Read failed");
		return;
	}

	rc = sensor_get_decoder(ctx->dev, &decoder);
	if (rc != 0) {
		shell_error(ctx->sh, "Failed to get decoder for '%s'", ctx->dev->name);
		return;
	}

	for (int channel = 0; channel < SENSOR_CHAN_ALL; ++channel) {
		uint32_t fit = 0;
		size_t base_size;
		size_t frame_size;
		size_t channel_idx = 0;
		uint16_t frame_count;

		if (channel == SENSOR_CHAN_ACCEL_X || channel == SENSOR_CHAN_ACCEL_Y ||
		    channel == SENSOR_CHAN_ACCEL_Z || channel == SENSOR_CHAN_GYRO_X ||
		    channel == SENSOR_CHAN_GYRO_Y || channel == SENSOR_CHAN_GYRO_Z ||
		    channel == SENSOR_CHAN_MAGN_X || channel == SENSOR_CHAN_MAGN_Y ||
		    channel == SENSOR_CHAN_MAGN_Z || channel == SENSOR_CHAN_POS_DY ||
		    channel == SENSOR_CHAN_POS_DZ) {
			continue;
		}

		rc = decoder->get_size_info(channel, &base_size, &frame_size);
		if (rc != 0) {
			/* Channel not supported, skipping */
			continue;
		}

		if (base_size > ARRAY_SIZE(decoded_buffer)) {
			shell_error(ctx->sh,
				    "Channel (%d) requires %zu bytes to decode, but only %zu are "
				    "available",
				    channel, base_size, ARRAY_SIZE(decoded_buffer));
			continue;
		}

		while (decoder->get_frame_count(buf, channel, channel_idx, &frame_count) == 0) {
			fit = 0;
			while (decoder->decode(buf, channel, channel_idx, &fit, 1, decoded_buffer) >
			       0) {

				switch (channel) {
				case SENSOR_CHAN_ACCEL_XYZ:
				case SENSOR_CHAN_GYRO_XYZ:
				case SENSOR_CHAN_MAGN_XYZ:
				case SENSOR_CHAN_POS_DX: {
					struct sensor_three_axis_data *data =
						(struct sensor_three_axis_data *)decoded_buffer;

					shell_info(ctx->sh,
						   "channel idx=%d %s shift=%d "
						   "value=%" PRIsensor_three_axis_data,
						   channel, sensor_channel_name[channel],
						   data->shift,
						   PRIsensor_three_axis_data_arg(*data, 0));
					break;
				}
				case SENSOR_CHAN_PROX: {
					struct sensor_byte_data *data =
						(struct sensor_byte_data *)decoded_buffer;

					shell_info(ctx->sh,
						   "channel idx=%d %s value=%" PRIsensor_byte_data(
							   is_near),
						   channel, sensor_channel_name[channel],
						   PRIsensor_byte_data_arg(*data, 0, is_near));
					break;
				}
				default: {
					struct sensor_q31_data *data =
						(struct sensor_q31_data *)decoded_buffer;

					shell_info(ctx->sh,
						   "channel idx=%d %s shift=%d "
						   "value=%" PRIsensor_q31_data,
						   channel,
						   (channel >= ARRAY_SIZE(sensor_channel_name))
							   ? ""
							   : sensor_channel_name[channel],
						   data->shift, PRIsensor_q31_data_arg(*data, 0));
					break;
				}
				}
			}
			++channel_idx;
		}
	}
}

static int cmd_get_sensor(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int count = 0;
	int err;

	err = k_mutex_lock(&cmd_get_mutex, K_NO_WAIT);
	if (err < 0) {
		shell_error(sh, "Another sensor reading in progress");
		return err;
	}

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Device unknown (%s)", argv[1]);
		k_mutex_unlock(&cmd_get_mutex);
		return -ENODEV;
	}

	if (argc == 2) {
		/* read all channels */
		for (int i = 0; i < ARRAY_SIZE(iodev_sensor_shell_channels); ++i) {
			if (SENSOR_CHANNEL_3_AXIS(i)) {
				continue;
			}
			iodev_sensor_shell_channels[count++] = i;
		}
	} else {
		/* read specific channels */
		for (int i = 2; i < argc; ++i) {
			int chan = parse_named_int(argv[i], sensor_channel_name,
						   ARRAY_SIZE(sensor_channel_name));

			if (chan < 0) {
				shell_error(sh, "Failed to read channel (%s)", argv[i]);
				continue;
			}
			iodev_sensor_shell_channels[count++] = chan;
		}
	}

	if (count == 0) {
		shell_error(sh, "No channels to read, bailing");
		k_mutex_unlock(&cmd_get_mutex);
		return -EINVAL;
	}
	iodev_sensor_shell_read_config.sensor = dev;
	iodev_sensor_shell_read_config.count = count;

	struct sensor_shell_processing_context ctx = {
		.dev = dev,
		.sh = sh,
	};
	err = sensor_read(&iodev_sensor_shell_read, &sensor_read_rtio, &ctx);
	if (err < 0) {
		shell_error(sh, "Failed to read sensor: %d", err);
	}
	sensor_processing_with_callback(&sensor_read_rtio, sensor_shell_processing_callback);

	k_mutex_unlock(&cmd_get_mutex);

	return 0;
}

static int cmd_sensor_attr_set(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	const struct device *dev;
	int rc;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(shell_ptr, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	for (size_t i = 2; i < argc; i += 3) {
		int channel = parse_named_int(argv[i], sensor_channel_name,
					      ARRAY_SIZE(sensor_channel_name));
		int attr = parse_named_int(argv[i + 1], sensor_attribute_name,
					   ARRAY_SIZE(sensor_attribute_name));
		struct sensor_value value = {0};

		if (channel < 0) {
			shell_error(shell_ptr, "Channel '%s' unknown", argv[i]);
			return -EINVAL;
		}
		if (attr < 0) {
			shell_error(shell_ptr, "Attribute '%s' unknown", argv[i + 1]);
			return -EINVAL;
		}
		if (parse_sensor_value(argv[i + 2], &value)) {
			shell_error(shell_ptr, "Sensor value '%s' invalid", argv[i + 2]);
			return -EINVAL;
		}

		rc = sensor_attr_set(dev, channel, attr, &value);
		if (rc) {
			shell_error(shell_ptr, "Failed to set channel(%s) attribute(%s): %d",
				    sensor_channel_name[channel], sensor_attribute_name[attr], rc);
			continue;
		}
		shell_info(shell_ptr, "%s channel=%s, attr=%s set to value=%s", dev->name,
			   sensor_channel_name[channel], sensor_attribute_name[attr], argv[i + 2]);
	}
	return 0;
}

static void cmd_sensor_attr_get_handler(const struct shell *shell_ptr, const struct device *dev,
					const char *channel_name, const char *attr_name,
					bool print_missing_attribute)
{
	int channel =
		parse_named_int(channel_name, sensor_channel_name, ARRAY_SIZE(sensor_channel_name));
	int attr = parse_named_int(attr_name, sensor_attribute_name,
				   ARRAY_SIZE(sensor_attribute_name));
	struct sensor_value value = {0};
	int rc;

	if (channel < 0) {
		shell_error(shell_ptr, "Channel '%s' unknown", channel_name);
		return;
	}
	if (attr < 0) {
		shell_error(shell_ptr, "Attribute '%s' unknown", attr_name);
		return;
	}

	rc = sensor_attr_get(dev, channel, attr, &value);

	if (rc != 0) {
		if (rc == -EINVAL && !print_missing_attribute) {
			return;
		}
		shell_error(shell_ptr, "Failed to get channel(%s) attribute(%s): %d",
			    sensor_channel_name[channel], sensor_attribute_name[attr], rc);
		return;
	}

	shell_info(shell_ptr, "%s(channel=%s, attr=%s) value=%.6f", dev->name,
		   sensor_channel_name[channel], sensor_attribute_name[attr],
		   sensor_value_to_double(&value));
}

static int cmd_sensor_attr_get(const struct shell *shell_ptr, size_t argc, char *argv[])
{
	const struct device *dev;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(shell_ptr, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	if (argc > 2) {
		for (size_t i = 2; i < argc; i += 2) {
			cmd_sensor_attr_get_handler(shell_ptr, dev, argv[i], argv[i + 1],
						    /*print_missing_attribute=*/true);
		}
	} else {
		for (size_t channel_idx = 0; channel_idx < ARRAY_SIZE(sensor_channel_name);
		     ++channel_idx) {
			for (size_t attr_idx = 0; attr_idx < ARRAY_SIZE(sensor_attribute_name);
			     ++attr_idx) {
				cmd_sensor_attr_get_handler(shell_ptr, dev,
							    sensor_channel_name[channel_idx],
							    sensor_attribute_name[attr_idx],
							    /*print_missing_attribute=*/false);
			}
		}
	}
	return 0;
}

static void channel_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_channel_name, channel_name_get);

static void attribute_name_get(size_t idx, struct shell_static_entry *entry)
{
	int cnt = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dsub_channel_name;

	for (int i = 0; i < SENSOR_ATTR_COMMON_COUNT; i++) {
		if (sensor_attribute_name[i] != NULL) {
			if (cnt == idx) {
				entry->syntax = sensor_attribute_name[i];
				break;
			}
			cnt++;
		}
	}
}
SHELL_DYNAMIC_CMD_CREATE(dsub_attribute_name, attribute_name_get);

static void channel_name_get(size_t idx, struct shell_static_entry *entry)
{
	int cnt = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	if (current_cmd_ctx == CTX_GET) {
		entry->subcmd = &dsub_channel_name;
	} else if (current_cmd_ctx == CTX_ATTR_GET_SET) {
		entry->subcmd = &dsub_attribute_name;
	} else {
		entry->subcmd = NULL;
	}

	for (int i = 0; i < SENSOR_CHAN_ALL; i++) {
		if (sensor_channel_name[i] != NULL) {
			if (cnt == idx) {
				entry->syntax = sensor_channel_name[i];
				break;
			}
			cnt++;
		}
	}
}

static void device_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	current_cmd_ctx = CTX_GET;
	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dsub_channel_name;
}

static void device_name_get_for_attr(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	current_cmd_ctx = CTX_ATTR_GET_SET;
	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dsub_channel_name;
}
SHELL_DYNAMIC_CMD_CREATE(dsub_device_name_for_attr, device_name_get_for_attr);

static void trigger_name_get(size_t idx, struct shell_static_entry *entry)
{
	int cnt = 0;

	entry->syntax = NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;

	for (int i = 0; i < SENSOR_TRIG_COMMON_COUNT; i++) {
		if (sensor_trigger_table[i].name != NULL) {
			if (cnt == idx) {
				entry->syntax = sensor_trigger_table[i].name;
				break;
			}
			cnt++;
		}
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_trigger_name, trigger_name_get);

static void trigger_on_off_get(size_t idx, struct shell_static_entry *entry)
{
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dsub_trigger_name;

	switch (idx) {
	case 0:
		entry->syntax = "on";
		break;
	case 1:
		entry->syntax = "off";
		break;
	default:
		entry->syntax = NULL;
		break;
	}
}

SHELL_DYNAMIC_CMD_CREATE(dsub_trigger_onoff, trigger_on_off_get);

static void device_name_get_for_trigger(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dsub_trigger_onoff;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_trigger, device_name_get_for_trigger);

static int cmd_get_sensor_info(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#ifdef CONFIG_SENSOR_INFO
	const char *null_str = "(null)";

	STRUCT_SECTION_FOREACH(sensor_info, sensor) {
		shell_print(sh,
			    "device name: %s, vendor: %s, model: %s, "
			    "friendly name: %s",
			    sensor->dev->name, sensor->vendor ? sensor->vendor : null_str,
			    sensor->model ? sensor->model : null_str,
			    sensor->friendly_name ? sensor->friendly_name : null_str);
	}
	return 0;
#else
	return -EINVAL;
#endif
}

enum sample_stats_state {
	SAMPLE_STATS_STATE_UNINITIALIZED = 0,
	SAMPLE_STATS_STATE_ENABLED,
	SAMPLE_STATS_STATE_DISABLED,
};

struct sample_stats {
	int64_t accumulator;
	uint32_t count;
	uint64_t sample_window_start;
	enum sample_stats_state state;
};

static void data_ready_trigger_handler(const struct device *sensor,
				       const struct sensor_trigger *trigger)
{
	static struct sample_stats stats[SENSOR_CHAN_ALL];
	const int64_t now = k_uptime_get();
	struct sensor_value value;

	if (sensor_sample_fetch(sensor)) {
		LOG_ERR("Failed to fetch samples on data ready handler");
	}
	for (int i = 0; i < SENSOR_CHAN_ALL; ++i) {
		int rc;

		/* Skip disabled channels */
		if (stats[i].state == SAMPLE_STATS_STATE_DISABLED) {
			continue;
		}
		/* Skip 3 axis channels */
		if (i == SENSOR_CHAN_ACCEL_XYZ || i == SENSOR_CHAN_GYRO_XYZ ||
		    i == SENSOR_CHAN_MAGN_XYZ) {
			continue;
		}

		rc = sensor_channel_get(sensor, i, &value);
		if (rc == -ENOTSUP && stats[i].state == SAMPLE_STATS_STATE_UNINITIALIZED) {
			/* Stop reading this channel if the driver told us it's not supported. */
			stats[i].state = SAMPLE_STATS_STATE_DISABLED;
		}
		if (rc != 0) {
			/* Skip on any error. */
			continue;
		}
		/* Do something with the data */
		stats[i].accumulator += value.val1 * INT64_C(1000000) + value.val2;
		if (stats[i].count++ == 0) {
			stats[i].sample_window_start = now;
		} else if (now > stats[i].sample_window_start +
					 CONFIG_SENSOR_SHELL_TRIG_PRINT_TIMEOUT_MS) {
			int64_t micro_value = stats[i].accumulator / stats[i].count;

			value.val1 = micro_value / 1000000;
			value.val2 = (int32_t)llabs(micro_value - (value.val1 * 1000000));
			LOG_INF("chan=%d, num_samples=%u, data=%d.%06d", i, stats[i].count,
				value.val1, value.val2);

			stats[i].accumulator = 0;
			stats[i].count = 0;
		}
	}
}

static int cmd_trig_sensor(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	enum sensor_trigger_type trigger;
	int err;

	if (argc < 4) {
		shell_error(sh, "Wrong number of args");
		return -EINVAL;
	}

	/* Parse device name */
	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	/* Map the trigger string to an enum value */
	for (trigger = 0; trigger < ARRAY_SIZE(sensor_trigger_table); trigger++) {
		if (strcmp(argv[3], sensor_trigger_table[trigger].name) == 0) {
			break;
		}
	}
	if (trigger >= SENSOR_TRIG_COMMON_COUNT || sensor_trigger_table[trigger].handler == NULL) {
		shell_error(sh, "Unsupported trigger type (%s)", argv[3]);
		return -ENOTSUP;
	}

	/* Parse on/off */
	if (strcmp(argv[2], "on") == 0) {
		err = sensor_trigger_set(dev, &sensor_trigger_table[trigger].trigger,
					 sensor_trigger_table[trigger].handler);
	} else if (strcmp(argv[2], "off") == 0) {
		/* Clear the handler for the given trigger on this device */
		err = sensor_trigger_set(dev, &sensor_trigger_table[trigger].trigger, NULL);
	} else {
		shell_error(sh, "Pass 'on' or 'off' to enable/disable trigger");
		return -EINVAL;
	}

	if (err) {
		shell_error(sh, "Error while setting trigger %d on device %s (%d)", trigger,
			    argv[1], err);
	}

	return err;
}

/* clang-format off */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_sensor,
	SHELL_CMD_ARG(get, &dsub_device_name, SENSOR_GET_HELP, cmd_get_sensor,
			2, 255),
	SHELL_CMD_ARG(attr_set, &dsub_device_name_for_attr, SENSOR_ATTR_SET_HELP,
			cmd_sensor_attr_set, 2, 255),
	SHELL_CMD_ARG(attr_get, &dsub_device_name_for_attr, SENSOR_ATTR_GET_HELP,
			cmd_sensor_attr_get, 2, 255),
	SHELL_COND_CMD(CONFIG_SENSOR_INFO, info, NULL, SENSOR_INFO_HELP,
			cmd_get_sensor_info),
	SHELL_CMD_ARG(trig, &dsub_trigger, SENSOR_TRIG_HELP, cmd_trig_sensor,
			2, 255),
	SHELL_SUBCMD_SET_END
	);
/* clang-format on */

SHELL_CMD_REGISTER(sensor, &sub_sensor, "Sensor commands", NULL);
