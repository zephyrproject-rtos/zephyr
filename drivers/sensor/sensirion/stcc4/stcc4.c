/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_stcc4

#include "stcc4.h"
#include "../sensirion_core/sensirion_common.h"
#include "../sensirion_core/sensirion_i2c.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/sensor/stcc4.h>

LOG_MODULE_REGISTER(STCC4, CONFIG_SENSOR_LOG_LEVEL);

static const struct i2c_dt_spec *i2c_spec;

static uint8_t communication_buffer[18] = {0};

int stcc4_start_continuous_measurement(void)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(
		buffer_ptr, local_offset, STCC4_START_CONTINUOUS_MEASUREMENT_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	return local_error;
}

int stcc4_read_measurement_raw(int16_t *co2_concentration_raw, uint16_t *temperature_raw,
			       uint16_t *relative_humidity_raw, uint16_t *sensor_status_raw)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
							     STCC4_READ_MEASUREMENT_RAW_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(1);
	local_error = sensirion_i2c_read_data_inplace(i2c_spec, buffer_ptr, 8);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	*co2_concentration_raw = sensirion_common_bytes_to_int16_t(&buffer_ptr[0]);
	*temperature_raw = sensirion_common_bytes_to_uint16_t(&buffer_ptr[2]);
	*relative_humidity_raw = sensirion_common_bytes_to_uint16_t(&buffer_ptr[4]);
	*sensor_status_raw = sensirion_common_bytes_to_uint16_t(&buffer_ptr[6]);
	return local_error;
}

int stcc4_stop_continuous_measurement(void)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(
		buffer_ptr, local_offset, STCC4_STOP_CONTINUOUS_MEASUREMENT_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(1200);
	return local_error;
}

int stcc4_measure_single_shot(void)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
							     STCC4_MEASURE_SINGLE_SHOT_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(500);
	return local_error;
}

int stcc4_perform_forced_recalibration(int16_t target_co2_concentration, int16_t *frc_correction)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(
		buffer_ptr, local_offset, STCC4_PERFORM_FORCED_RECALIBRATION_CMD_ID);
	local_offset = sensirion_i2c_add_int16_t_to_buffer(buffer_ptr, local_offset,
							   target_co2_concentration);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(90);
	local_error = sensirion_i2c_read_data_inplace(i2c_spec, buffer_ptr, 2);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	*frc_correction = sensirion_common_bytes_to_int16_t(&buffer_ptr[0]);
	return local_error;
}

int stcc4_get_product_id(uint32_t *product_id, uint64_t *serial_number)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
							     STCC4_GET_PRODUCT_ID_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(1);
	local_error = sensirion_i2c_read_data_inplace(i2c_spec, buffer_ptr, 12);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	*product_id = sensirion_common_bytes_to_uint32_t(&buffer_ptr[0]);
	sensirion_common_to_integer(&buffer_ptr[4], (uint8_t *)serial_number, LONG_INTEGER, 8);
	return local_error;
}

int stcc4_set_rht_compensation(uint16_t raw_temperature, uint16_t raw_humidity)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
							     STCC4_SET_RHT_COMPENSATION_CMD_ID);
	local_offset =
		sensirion_i2c_add_uint16_t_to_buffer(buffer_ptr, local_offset, raw_temperature);
	local_offset = sensirion_i2c_add_uint16_t_to_buffer(buffer_ptr, local_offset, raw_humidity);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(1);
	return local_error;
}

int stcc4_set_pressure_compensation_raw(uint16_t pressure)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(
		buffer_ptr, local_offset, STCC4_SET_PRESSURE_COMPENSATION_RAW_CMD_ID);
	local_offset = sensirion_i2c_add_uint16_t_to_buffer(buffer_ptr, local_offset, pressure);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(1);
	return local_error;
}

int stcc4_perform_self_test(uint16_t *test_result)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
							     STCC4_PERFORM_SELF_TEST_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(360);
	local_error = sensirion_i2c_read_data_inplace(i2c_spec, buffer_ptr, 2);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	*test_result = sensirion_common_bytes_to_uint16_t(&buffer_ptr[0]);
	return local_error;
}

int stcc4_perform_conditioning(void)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
							     STCC4_PERFORM_CONDITIONING_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(22000);
	return local_error;
}

int stcc4_enter_sleep_mode(void)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
							     STCC4_ENTER_SLEEP_MODE_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(2);
	return local_error;
}

int stcc4_exit_sleep_mode(void)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command8_to_buffer(buffer_ptr, local_offset,
							    STCC4_EXIT_SLEEP_MODE_CMD_ID);
	sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	k_msleep(5);
	return local_error;
}

int stcc4_enable_testing_mode(void)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
							     STCC4_ENABLE_TESTING_MODE_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	return local_error;
}

int stcc4_disable_testing_mode(void)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
							     STCC4_DISABLE_TESTING_MODE_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	return local_error;
}

int stcc4_perform_factory_reset(uint16_t *factory_reset_result)
{
	int local_error = NO_ERROR;
	uint8_t *buffer_ptr = communication_buffer;
	uint16_t local_offset = 0;

	local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
							     STCC4_PERFORM_FACTORY_RESET_CMD_ID);
	local_error = sensirion_i2c_write_data(i2c_spec, buffer_ptr, local_offset);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	k_msleep(90);
	local_error = sensirion_i2c_read_data_inplace(i2c_spec, buffer_ptr, 2);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	*factory_reset_result = sensirion_common_bytes_to_uint16_t(&buffer_ptr[0]);
	return local_error;
}

float stcc4_signal_temperature(uint16_t raw_temperature)
{
	float temperature = 0.0;

	temperature = -45.0 + ((175.0 * raw_temperature) / 65535.0);
	return temperature;
}

float stcc4_signal_relative_humidity(uint16_t raw_relative_humidity)
{
	float relative_humidity = 0.0;

	relative_humidity = -6.0 + ((125.0 * raw_relative_humidity) / 65535.0);
	return relative_humidity;
}

int stcc4_read_measurement(int16_t *co2_concentration, float *temperature, float *relative_humidity,
			   uint16_t *sensor_status)
{
	int16_t raw_co2_concentration = 0;
	uint16_t raw_temperature = 0;
	uint16_t raw_relative_humidity = 0;
	uint16_t sensor_status_raw = 0;
	int local_error = 0;

	local_error = stcc4_read_measurement_raw(&raw_co2_concentration, &raw_temperature,
						 &raw_relative_humidity, &sensor_status_raw);
	if (local_error != NO_ERROR) {
		return local_error;
	}
	*co2_concentration = raw_co2_concentration;
	*temperature = stcc4_signal_temperature(raw_temperature);
	*relative_humidity = stcc4_signal_relative_humidity(raw_relative_humidity);
	*sensor_status = sensor_status_raw;
	return local_error;
}

int stcc4_set_pressure_compensation(uint32_t pressure)
{
	int local_error = 0;

	local_error = stcc4_set_pressure_compensation_raw((uint32_t)(pressure / 2));
	if (local_error != NO_ERROR) {
		return local_error;
	}
	return local_error;
}

int stcc4_select_rht_compensation(uint16_t temperature_compensation, uint16_t humidity_compensation)
{
	int local_error = 0;

	if (temperature_compensation || humidity_compensation) {
		local_error =
			stcc4_set_rht_compensation(temperature_compensation, humidity_compensation);
		if (local_error != NO_ERROR) {
			return local_error;
		}
	}
	return local_error;
}

int stcc4_select_perform_conditioning(bool do_perform_conditioning)
{
	int local_error = 0;

	if (do_perform_conditioning) {
		local_error = stcc4_perform_conditioning();
		if (local_error != NO_ERROR) {
			return local_error;
		}
	}
	return local_error;
}

bool stcc4_check_self_test(void)
{
	int local_error = 0;
	uint16_t self_test_result = 0;

	local_error = stcc4_perform_self_test(&self_test_result);
	if (local_error != NO_ERROR) {
		return false;
	}

	return self_test_result == 0;
}

static int stcc4_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct stcc4_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_CO2 && chan != SENSOR_CHAN_HUMIDITY &&
	    chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Channel not supported.");
		return -ENOTSUP;
	}
	int ret =
		stcc4_read_measurement_raw(&data->co2_concentration_raw, &data->temperature_raw,
					   &data->relative_humidity_raw, &data->sensor_status_raw);
	if (ret < 0) {
		LOG_ERR("Failed to sample fetch.");
		return ret;
	}
	return 0;
};

static int stcc4_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct stcc4_data *data = dev->data;
	int ret = 0;

	switch (chan) {
	case SENSOR_CHAN_CO2: {
		int16_t local_var = data->co2_concentration_raw;

		val->val1 = local_var;
		val->val2 = 0;
	} break;
	case SENSOR_CHAN_HUMIDITY: {
		float local_var = stcc4_signal_relative_humidity(data->relative_humidity_raw);

		ret = sensor_value_from_float(val, local_var);
	} break;
	case SENSOR_CHAN_AMBIENT_TEMP: {
		float local_var = stcc4_signal_temperature(data->temperature_raw);

		ret = sensor_value_from_float(val, local_var);
	} break;
	default:
		LOG_ERR("Channel not supported.");
		return -ENOTSUP;
	}

	if (ret != 0) {
		LOG_ERR("Failed to convert value.");
		return ret;
	}

	return 0;
}

int stcc4_init(const struct device *dev)
{
	int local_error = 0;

	const struct stcc4_config *cfg = dev->config;

	i2c_spec = &cfg->bus;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	local_error = stcc4_stop_continuous_measurement();
	if (local_error != NO_ERROR) {
		LOG_ERR("error executing stop_continuous_measurement(): %i\n", local_error);
		return local_error;
	}
	if (!stcc4_check_self_test()) {
		return -EIO;
	}
	local_error = stcc4_set_pressure_compensation(cfg->pressure);
	if (local_error != NO_ERROR) {
		LOG_ERR("error executing set_pressure_compensation(): %i\n", local_error);
		return local_error;
	}
	local_error = stcc4_select_rht_compensation(cfg->temperature_compensation,
						    cfg->humidity_compensation);
	if (local_error != NO_ERROR) {
		LOG_ERR("error executing select_rht_compensation(): %i\n", local_error);
		return local_error;
	}
	local_error = stcc4_select_perform_conditioning(cfg->do_perform_conditioning);
	if (local_error != NO_ERROR) {
		LOG_ERR("error executing select_perform_conditioning(): %i\n", local_error);
		return local_error;
	}
	local_error = stcc4_start_continuous_measurement();
	if (local_error != NO_ERROR) {
		LOG_ERR("error executing start_continuous_measurement(): %i\n", local_error);
		return local_error;
	}
	return 0;
}

static DEVICE_API(sensor, stcc4_api) = {
	.sample_fetch = stcc4_sample_fetch,
	.channel_get = stcc4_channel_get,
};

#define STCC4_INIT(inst)                                                                           \
	static struct stcc4_data stcc4_data_##inst;                                                \
                                                                                                   \
	static const struct stcc4_config stcc4_config_##inst = {                                   \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.pressure = DT_INST_PROP(inst, pressure),                                          \
		.humidity_compensation = DT_INST_PROP(inst, humidity_compensation),                \
		.temperature_compensation = DT_INST_PROP(inst, temperature_compensation),          \
		.do_perform_conditioning = DT_INST_PROP(inst, do_perform_conditioning),            \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, stcc4_init, NULL, &stcc4_data_##inst,                   \
				     &stcc4_config_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &stcc4_api);

DT_INST_FOREACH_STATUS_OKAY(STCC4_INIT)
