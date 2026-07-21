/*
 * Copyright (c) 2025 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/shell/shell.h>

#include "sen6x.h"

LOG_MODULE_REGISTER(sen6x_shell, CONFIG_SENSOR_LOG_LEVEL);

#define SEN6X_CALLBACK_HELP     SHELL_HELP("Enable callback", "<on|off>")
#define SEN6X_START_HELP        SHELL_HELP("Start continuous measurement", "<device>")
#define SEN6X_STOP_HELP         SHELL_HELP("Stop continuous measurement", "<device>")
#define SEN6X_RESET_HELP        SHELL_HELP("Reset device", "<device>")
#define SEN6X_PRODUCT_NAME_HELP SHELL_HELP("Get product name", "<device>")
#define SEN6X_SERIAL_HELP       SHELL_HELP("Get serial number", "<device>")
#define SEN6X_FIRMWARE_HELP     SHELL_HELP("Get firmware version", "<device>")
#define SEN6X_SET_TEMP_OFFSET_HELP                                                                 \
	SHELL_HELP("Set temperature offset parameters",                                            \
		   "<device> <offset> <slope> <time_constant> <slot>")
#define SEN6X_SET_TEMP_ACCEL_HELP                                                                  \
	SHELL_HELP("Set temperature acceleration parameters", "<device> <K> <P> <T1> <T2>")
#define SEN6X_SET_VOC_TUNING_HELP                                                                  \
	SHELL_HELP(                                                                                \
		"Set VOC algorithm tuning parameters",                                             \
		"<device> <index_offset> <learning_time_offset_hours> <learning_time_gain_hours> " \
		"<gating_max_duration_minutes> <std_initial> <gain_factor>")
#define SEN6X_SET_NOX_TUNING_HELP                                                                  \
	SHELL_HELP(                                                                                \
		"Set NOx algorithm tuning parameters",                                             \
		"<device> <index_offset> <learning_time_offset_hours> <learning_time_gain_hours> " \
		"<gating_max_duration_minutes> <std_initial> <gain_factor>")
#define SEN6X_SET_CO2_CALIB_HELP   SHELL_HELP("Enable CO2 automatic self calibration", "<on|off>")
#define SEN6X_SET_PRESSURE_HELP    SHELL_HELP("Set ambient pressure", "<ambient_pressure>")
#define SEN6X_SET_ALTITUDE_HELP    SHELL_HELP("Set sensor altitude", "<sensor_altitude>")
#define SEN6X_CLEAN_FAN_HELP       SHELL_HELP("Start fan cleaning", "<device>")
#define SEN6X_ACTIVATE_HEATER_HELP SHELL_HELP("Activate SHT heater", "<device>")
#define SEN6X_VOC_STATE_SAVE_HELP                                                                  \
	SHELL_HELP("Save VOC algorithm state in a global variable", "<device>")
#define SEN6X_VOC_STATE_RESTORE_HELP                                                               \
	SHELL_HELP("Restore VOC algorithm state from a global variable", "<device>")

static long sen6x_get_long(const struct shell *sh, const char *arg, int *out_ret, const char *name)
{
	int ret = 0;
	long value = shell_strtol(arg, 0, &ret);

	if (ret != 0) {
		shell_error(sh, "Invalid %s: %s", name, arg);
		*out_ret = ret;
	}

	return value;
}

static unsigned long long sen6x_get_ull(const struct shell *sh, const char *arg, int *out_ret,
					const char *name)
{
	int ret = 0;
	unsigned long long value = shell_strtoull(arg, 0, &ret);

	if (ret != 0) {
		shell_error(sh, "Invalid %s: %s", name, arg);
		*out_ret = ret;
	}

	return value;
}

static struct sen6x_voc_algorithm_state voc_algorithm_state;

static bool sen6x_device_check(const struct device *dev)
{
	return dev->api == &sen6x_driver_api;
}

static const struct device *sen6x_get_device_binding(const struct shell *sh, const char *arg)
{
	const struct device *dev = shell_device_get_binding(arg);

	if (dev == NULL || !sen6x_device_check(dev)) {
		shell_error(sh, "Sensor device unknown (%s)", arg);
		return NULL;
	}

	return dev;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, sen6x_device_check);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = "List Devices";
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static const struct shell *callback_sh;
static bool callback_used;

static void status_changed(const struct device *dev, struct sen6x_callback *callback,
			   uint32_t status)
{
	shell_print(callback_sh, "status changed to 0x%08x", status);
}

static struct sen6x_callback sen6x_callback = {
	.status_changed = status_changed,
};

static int cmd_callback(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	bool enabled;

	if (!dev) {
		return -ENODEV;
	}

	enabled = shell_strtobool(argv[2], 0, &ret);
	if (ret != 0) {
		shell_error(sh, "Invalid enabled-flag: %s", argv[2]);
		return ret;
	}

	if (enabled) {
		if (callback_used) {
			shell_error(sh, "The callback is registered to another device already");
			return -EPERM;
		}

		callback_used = true;
		callback_sh = sh;
		sen6x_add_callback(dev, &sen6x_callback);
	} else {
		sen6x_remove_callback(dev, &sen6x_callback);
		callback_used = false;
		callback_sh = NULL;
	}

	return 0;
}

static int cmd_start(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);

	if (!dev) {
		return -ENODEV;
	}

	ret = sen6x_start_continuous_measurement(dev);
	if (ret != 0) {
		shell_error(sh, "failed to start measurement: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_stop(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);

	if (!dev) {
		return -ENODEV;
	}

	ret = sen6x_stop_continuous_measurement(dev);
	if (ret != 0) {
		shell_error(sh, "failed to stop measurement: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_reset(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);

	if (!dev) {
		return -ENODEV;
	}

	ret = sen6x_device_reset(dev);
	if (ret != 0) {
		shell_error(sh, "failed to reset: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_product_name(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	static char product_name[48];

	if (!dev) {
		return -ENODEV;
	}

	ret = sen6x_get_product_name(dev, product_name, sizeof(product_name));
	if (ret != 0) {
		shell_error(sh, "failed to get product name: %d", ret);
		return ret;
	}

	shell_print(sh, "%s", product_name);
	return 0;
}

static int cmd_serial(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	static char serial_number[48];

	if (!dev) {
		return -ENODEV;
	}

	ret = sen6x_get_serial_number(dev, serial_number, sizeof(serial_number));
	if (ret != 0) {
		shell_error(sh, "failed to get serial number: %d", ret);
		return ret;
	}

	shell_print(sh, "%s", serial_number);
	return 0;
}

static int cmd_firmware(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	const struct sen6x_firmware_version *firmware_version;

	if (!dev) {
		return -ENODEV;
	}

	firmware_version = sen6x_get_firmware_version(dev);

	shell_print(sh, "%u.%u", firmware_version->major, firmware_version->minor);
	return 0;
}

static int cmd_set_temp_offset(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	struct sen6x_temperature_offset_parameters params = {0};

	if (!dev) {
		return -ENODEV;
	}

	params.offset = sen6x_get_long(sh, argv[2], &ret, "offset");
	params.slope = sen6x_get_long(sh, argv[3], &ret, "slope");
	params.time_constant = sen6x_get_ull(sh, argv[4], &ret, "time_constant");
	params.slot = sen6x_get_ull(sh, argv[5], &ret, "slot");

	if (ret != 0) {
		return ret;
	}

	ret = sen6x_set_temperature_offset_parameters(dev, &params);
	if (ret != 0) {
		shell_error(sh, "failed to set temperature offset parameters: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_set_temp_accel(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	struct sen6x_temperature_acceleration_parameters params = {0};

	if (!dev) {
		return -ENODEV;
	}

	params.K = sen6x_get_ull(sh, argv[2], &ret, "K");
	params.P = sen6x_get_ull(sh, argv[3], &ret, "P");
	params.T1 = sen6x_get_ull(sh, argv[4], &ret, "T1");
	params.T2 = sen6x_get_ull(sh, argv[5], &ret, "T2");

	if (ret != 0) {
		return ret;
	}

	ret = sen6x_set_temperature_acceleration_parameters(dev, &params);
	if (ret != 0) {
		shell_error(sh, "failed to set temperature acceleration parameters: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_set_voc_tuning(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	struct sen6x_algorithm_tuning_parameters params = {0};

	if (!dev) {
		return -ENODEV;
	}

	params.index_offset = sen6x_get_long(sh, argv[2], &ret, "index_offset");
	params.learning_time_offset_hours =
		sen6x_get_long(sh, argv[3], &ret, "learning_time_offset_hours");
	params.learning_time_gain_hours =
		sen6x_get_long(sh, argv[4], &ret, "learning_time_gain_hours");
	params.gating_max_duration_minutes =
		sen6x_get_long(sh, argv[5], &ret, "gating_max_duration_minutes");
	params.std_initial = sen6x_get_long(sh, argv[6], &ret, "std_initial");
	params.gain_factor = sen6x_get_long(sh, argv[7], &ret, "gain_factor");

	if (ret != 0) {
		return ret;
	}

	ret = sen6x_set_voc_algorithm_tuning_parameters(dev, &params);
	if (ret != 0) {
		shell_error(sh, "failed to set VOC algorithm tuning parameters: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_set_nox_tuning(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	struct sen6x_algorithm_tuning_parameters params = {0};

	if (!dev) {
		return -ENODEV;
	}

	params.index_offset = sen6x_get_long(sh, argv[2], &ret, "index_offset");
	params.learning_time_offset_hours =
		sen6x_get_long(sh, argv[3], &ret, "learning_time_offset_hours");
	params.learning_time_gain_hours =
		sen6x_get_long(sh, argv[4], &ret, "learning_time_gain_hours");
	params.gating_max_duration_minutes =
		sen6x_get_long(sh, argv[5], &ret, "gating_max_duration_minutes");
	params.std_initial = sen6x_get_long(sh, argv[6], &ret, "std_initial");
	params.gain_factor = sen6x_get_long(sh, argv[7], &ret, "gain_factor");

	if (ret != 0) {
		return ret;
	}

	ret = sen6x_set_nox_algorithm_tuning_parameters(dev, &params);
	if (ret != 0) {
		shell_error(sh, "failed to set NOx algorithm tuning parameters: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_set_co2_calib(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	bool enabled;

	if (!dev) {
		return -ENODEV;
	}

	enabled = shell_strtobool(argv[2], 0, &ret);
	if (ret != 0) {
		shell_error(sh, "Invalid enabled-flag: %s", argv[2]);
		return ret;
	}

	ret = sen6x_set_co2_automatic_self_calibration(dev, enabled);
	if (ret != 0) {
		shell_error(sh, "failed to set CO2 automatic self calibration: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_set_pressure(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	uint16_t ambient_pressure;

	if (!dev) {
		return -ENODEV;
	}

	ambient_pressure = sen6x_get_ull(sh, argv[2], &ret, "ambient_pressure");
	if (ret != 0) {
		return ret;
	}

	ret = sen6x_set_ambient_pressure(dev, ambient_pressure);
	if (ret != 0) {
		shell_error(sh, "failed to set ambient pressure: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_set_altitude(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	uint16_t sensor_altitude;

	if (!dev) {
		return -ENODEV;
	}

	sensor_altitude = sen6x_get_ull(sh, argv[2], &ret, "sensor_altitude");
	if (ret != 0) {
		return ret;
	}

	ret = sen6x_set_sensor_altitude(dev, sensor_altitude);
	if (ret != 0) {
		shell_error(sh, "failed to set sensor altitude: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_clean_fan(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);

	if (!dev) {
		return -ENODEV;
	}

	ret = sen6x_start_fan_cleaning(dev);
	if (ret != 0) {
		shell_error(sh, "failed to start fan cleaning: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_activate_heater(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);
	static struct sensor_q31_data relative_humidity = {0};
	static struct sensor_q31_data temperature = {0};

	if (!dev) {
		return -ENODEV;
	}

	ret = sen6x_activate_sht_heater(dev, &relative_humidity, &temperature);
	if (ret != 0) {
		shell_error(sh, "failed to activate SHT heater: %d", ret);
		return ret;
	}

	if (relative_humidity.header.reading_count > 0) {
		shell_print(sh, "relative humidity: %" PRIsensor_q31_data,
			    PRIsensor_q31_data_arg(relative_humidity, 0));
	} else {
		shell_print(sh, "relative humidity: <unavailable>");
	}

	if (temperature.header.reading_count > 0) {
		shell_print(sh, "temperature: %" PRIsensor_q31_data,
			    PRIsensor_q31_data_arg(temperature, 0));
	} else {
		shell_print(sh, "temperature: <unavailable>");
	}

	return 0;
}

static int cmd_voc_state_save(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);

	if (!dev) {
		return -ENODEV;
	}

	ret = sen6x_get_voc_algorithm_state(dev, &voc_algorithm_state);
	if (ret != 0) {
		shell_error(sh, "failed to get VOC algorithm state: %d", ret);
		return ret;
	}

	shell_hexdump(sh, voc_algorithm_state.buffer, sizeof(voc_algorithm_state.buffer));

	return 0;
}

static int cmd_voc_state_restore(const struct shell *sh, size_t argc, char *argv[])
{
	int ret;
	const struct device *dev = sen6x_get_device_binding(sh, argv[1]);

	if (!dev) {
		return -ENODEV;
	}

	ret = sen6x_set_voc_algorithm_state(dev, &voc_algorithm_state);
	if (ret != 0) {
		shell_error(sh, "failed to set VOC algorithm state: %d", ret);
		return ret;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_sen6x,
	SHELL_CMD_ARG(callback, &dsub_device_name, SEN6X_CALLBACK_HELP, cmd_callback, 3, 0),
	SHELL_CMD_ARG(start, &dsub_device_name, SEN6X_START_HELP, cmd_start, 2, 0),
	SHELL_CMD_ARG(stop, &dsub_device_name, SEN6X_STOP_HELP, cmd_stop, 2, 0),
	SHELL_CMD_ARG(reset, &dsub_device_name, SEN6X_RESET_HELP, cmd_reset, 2, 0),
	SHELL_CMD_ARG(product_name, &dsub_device_name, SEN6X_PRODUCT_NAME_HELP, cmd_product_name, 2,
		      0),
	SHELL_CMD_ARG(serial, &dsub_device_name, SEN6X_SERIAL_HELP, cmd_serial, 2, 0),
	SHELL_CMD_ARG(firmware, &dsub_device_name, SEN6X_FIRMWARE_HELP, cmd_firmware, 2, 0),
	SHELL_CMD_ARG(set_temp_offset, &dsub_device_name, SEN6X_SET_TEMP_OFFSET_HELP,
		      cmd_set_temp_offset, 6, 0),
	SHELL_CMD_ARG(set_temp_accel, &dsub_device_name, SEN6X_SET_TEMP_ACCEL_HELP,
		      cmd_set_temp_accel, 6, 0),
	SHELL_CMD_ARG(set_voc_tuning, &dsub_device_name, SEN6X_SET_VOC_TUNING_HELP,
		      cmd_set_voc_tuning, 8, 0),
	SHELL_CMD_ARG(set_nox_tuning, &dsub_device_name, SEN6X_SET_NOX_TUNING_HELP,
		      cmd_set_nox_tuning, 8, 0),
	SHELL_CMD_ARG(set_co2_calib, &dsub_device_name, SEN6X_SET_CO2_CALIB_HELP, cmd_set_co2_calib,
		      2, 0),
	SHELL_CMD_ARG(set_pressure, &dsub_device_name, SEN6X_SET_PRESSURE_HELP, cmd_set_pressure, 2,
		      0),
	SHELL_CMD_ARG(set_altitude, &dsub_device_name, SEN6X_SET_ALTITUDE_HELP, cmd_set_altitude, 2,
		      0),
	SHELL_CMD_ARG(clean_fan, &dsub_device_name, SEN6X_CLEAN_FAN_HELP, cmd_clean_fan, 2, 0),
	SHELL_CMD_ARG(activate_heater, &dsub_device_name, SEN6X_ACTIVATE_HEATER_HELP,
		      cmd_activate_heater, 2, 0),
	SHELL_CMD_ARG(voc_state_save, &dsub_device_name, SEN6X_VOC_STATE_SAVE_HELP,
		      cmd_voc_state_save, 2, 0),
	SHELL_CMD_ARG(voc_state_restore, &dsub_device_name, SEN6X_VOC_STATE_RESTORE_HELP,
		      cmd_voc_state_restore, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(sen6x, &sub_sen6x, "SEN6X commands", NULL);
