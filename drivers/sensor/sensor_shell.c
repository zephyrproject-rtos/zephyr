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
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/rtio_executor_simple.h>
#include <zephyr/shell/shell.h>

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

enum dynamic_command_context {
	NONE,
	CTX_GET,
	CTX_ATTR_GET_SET,
};

static enum dynamic_command_context current_cmd_ctx = NONE;

/* Crate a single common config for one-shot reading */
static enum sensor_channel iodev_sensor_shell_channels[SENSOR_CHAN_ALL];
static struct sensor_read_config iodev_sensor_shell_read_config = {
	.sensor = NULL,
	.channels = iodev_sensor_shell_channels,
	.num_channels = 0,
	.max_channels = ARRAY_SIZE(iodev_sensor_shell_channels),
};
RTIO_IODEV_DEFINE(iodev_sensor_shell_read, &__sensor_iodev_api, &iodev_sensor_shell_read_config);

/* Create the RTIO context to service the reading */
RTIO_EXECUTOR_SIMPLE_DEFINE(sensor_shell_executor);
RTIO_DEFINE_WITH_MEMPOOL(sensor_read_rtio, &sensor_shell_executor, 1, 1, 8, 64, 4);

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
	struct sensor_decoder_api decoder;
	sensor_frame_iterator_t fit = {0};
	sensor_channel_iterator_t cit = {0};
	uint64_t timestamp;
	enum sensor_channel channel;
	q31_t q;
	int rc;

	ARG_UNUSED(buf_len);

	if (result != 0) {
		shell_error(ctx->sh, "Read failed");
		return;
	}

	rc = sensor_get_decoder(ctx->dev, &decoder);
	if (rc != 0) {
		shell_error(ctx->sh, "Failed to get decoder for '%s'", ctx->dev->name);
		return;
	}

	rc = decoder.get_timestamp(buf, &timestamp);
	if (rc != 0) {
		shell_error(ctx->sh, "Failed to get fetch timestamp for '%s'", ctx->dev->name);
		return;
	}
	shell_print(ctx->sh, "Got samples at %" PRIu64 " ticks", timestamp);

	while (decoder.decode(buf, &fit, &cit, &channel, &q, 1) > 0) {
		int8_t shift;

		rc = decoder.get_shift(buf, channel, &shift);
		if (rc != 0) {
			shell_error(ctx->sh, "Failed to get bitshift for channel %d", channel);
			continue;
		}

		int64_t scaled_value = (int64_t)q << shift;
		bool is_negative = scaled_value < 0;
		int numerator;
		int denominator;

		scaled_value = llabs(scaled_value);
		numerator = (int)FIELD_GET(GENMASK64(31 + shift, 31), scaled_value);
		denominator =
			(int)((FIELD_GET(GENMASK64(30, 0), scaled_value) * 1000000) / INT32_MAX);

		if (channel >= ARRAY_SIZE(sensor_channel_name)) {
			shell_print(ctx->sh, "channel idx=%d value=%s%d.%06d", channel,
				    is_negative ? "-" : "", numerator, denominator);
		} else {
			shell_print(ctx->sh, "channel idx=%d %s value=%s%d.%06d", channel,
				    sensor_channel_name[channel], is_negative ? "-" : "", numerator,
				    denominator);
		}
	}
}

static int cmd_get_sensor(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev;
	int err;

	dev = device_get_binding(argv[1]);
	if (dev == NULL) {
		shell_error(sh, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	if (argc == 2) {
		/* read all channels */
		for (int i = 0; i < ARRAY_SIZE(iodev_sensor_shell_channels); ++i) {
			iodev_sensor_shell_channels[i] = i;
		}
		iodev_sensor_shell_read_config.num_channels =
			iodev_sensor_shell_read_config.max_channels;
	} else {
		/* read specific channels */
		iodev_sensor_shell_read_config.num_channels = 0;
		for (int i = 2; i < argc; ++i) {
			int chan = parse_named_int(argv[i], sensor_channel_name,
						   ARRAY_SIZE(sensor_channel_name));

			if (chan < 0) {
				shell_error(sh, "Failed to read channel (%s)", argv[i]);
				continue;
			}
			iodev_sensor_shell_channels[iodev_sensor_shell_read_config.num_channels++] =
				chan;
		}
	}

	/* TODO(yperess) use sensor_reconfigure_read_iodev? */
	if (iodev_sensor_shell_read_config.num_channels == 0) {
		shell_error(sh, "No channels to read, bailing");
		return -EINVAL;
	}
	iodev_sensor_shell_read_config.sensor = dev;

	struct sensor_shell_processing_context ctx = {
		.dev = dev,
		.sh = sh,
	};
	err = sensor_read(&iodev_sensor_shell_read, &sensor_read_rtio, &ctx);
	if (err < 0) {
		shell_error(sh, "Failed to read sensor: %d", err);
	}
	z_sensor_processing_loop(&sensor_read_rtio, sensor_shell_processing_callback);

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

static int cmd_get_sensor_info(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#ifdef CONFIG_SENSOR_INFO
	const char *null_str = "(null)";

	STRUCT_SECTION_FOREACH(sensor_info, sensor)
	{
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
	SHELL_SUBCMD_SET_END
	);
/* clang-format on */

SHELL_CMD_REGISTER(sensor, &sub_sensor, "Sensor commands", NULL);
