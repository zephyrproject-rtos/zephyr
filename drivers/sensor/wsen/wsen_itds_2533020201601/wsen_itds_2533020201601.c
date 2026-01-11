/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_itds_2533020201601

#include <string.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <wsen_sensors_common.h>
#include "wsen_itds_2533020201601.h"

LOG_MODULE_REGISTER(WSEN_ITDS_2533020201601, CONFIG_SENSOR_LOG_LEVEL);

/*
 * List of supported output data rates (sensor_value struct, input to
 * sensor_attr_set()). Index into this list is used as argument for
 * ITDS_setOutputDataRate().
 */
static const struct sensor_value itds_2533020201601_odr_list[] = {
	{.val1 = 0, .val2 = 0},           {.val1 = 1, .val2 = 6 * 100000},
	{.val1 = 12, .val2 = 5 * 100000}, {.val1 = 25, .val2 = 0},
	{.val1 = 50, .val2 = 0},          {.val1 = 100, .val2 = 0},
	{.val1 = 200, .val2 = 0},         {.val1 = 400, .val2 = 0},
	{.val1 = 800, .val2 = 0},         {.val1 = 1600, .val2 = 0},
};

/*
 * List of supported full scale values (i.e. measurement ranges, in g).
 * Index into this list is used as input for ITDS_setFullScale().
 */
static const int itds_2533020201601_full_scale_list[] = {
	2,
	4,
	8,
	16,
};

/* Map of dts binding power mode to enum power mode*/
static const ITDS_powerMode_t power_mode_map[] = {
	[0] = ITDS_lowPower,
	[1] = ITDS_normalMode,
};

/* convert raw temperature to celsius */
static inline int16_t itds_2533020201601_raw_temp_to_celsius(int16_t raw_temp)
{
	return ((raw_temp * 100) / 16) + 2500;
}

static int itds_2533020201601_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	struct itds_2533020201601_data *data = dev->data;
	const struct itds_2533020201601_config *cfg = dev->config;
	int16_t temperature, acceleration_x, acceleration_y, acceleration_z;

	switch (channel) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		break;
	default:
		LOG_ERR("Fetching is not supported on channel %d.", channel);
		return -ENOTSUP;
	}

	uint32_t step_sleep_duration = 0;

	if (cfg->op_mode == ITDS_singleConversion) {
		if (ITDS_startSingleDataConversion(&data->sensor_interface, ITDS_enable) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to start single data conversion.");
			return -EIO;
		}

		k_sleep(K_MSEC(5));
	} else {
		if (!wsen_sensor_step_sleep_duration_milli_from_odr_hz(
			    &itds_2533020201601_odr_list[data->sensor_odr], &step_sleep_duration)) {
			LOG_ERR("Accelerometer is disabled.");
			return -ENOTSUP;
		}
	}

	ITDS_state_t acceleration_data_ready, temp_data_ready;

	acceleration_data_ready = temp_data_ready = ITDS_disable;

	bool data_ready = false;
	int step_count = 0;

	while (1) {

		switch (channel) {
		case SENSOR_CHAN_ALL: {
			if (ITDS_isAccelerationDataReady(&data->sensor_interface,
							 &acceleration_data_ready) != WE_SUCCESS) {
				LOG_ERR("Failed to check if acceleration data is ready.");
				return -EIO;
			}

			if (ITDS_isTemperatureDataReady(&data->sensor_interface,
							&temp_data_ready) != WE_SUCCESS) {
				LOG_ERR("Failed to check if temperature data is ready.");
				return -EIO;
			}

			data_ready = (acceleration_data_ready == ITDS_enable &&
				      temp_data_ready == ITDS_enable);

			break;
		}
		case SENSOR_CHAN_AMBIENT_TEMP: {
			if (ITDS_isTemperatureDataReady(&data->sensor_interface,
							&temp_data_ready) != WE_SUCCESS) {
				LOG_ERR("Failed to check if temperature data is ready.");
				return -EIO;
			}

			data_ready = (temp_data_ready == ITDS_enable);

			break;
		}
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ: {
			if (ITDS_isAccelerationDataReady(&data->sensor_interface,
							 &acceleration_data_ready) != WE_SUCCESS) {
				LOG_ERR("Failed to check if acceleration data is ready.");
				return -EIO;
			}

			data_ready = (acceleration_data_ready == ITDS_enable);

			break;
		}
		default:
			break;
		}

		if (data_ready) {
			break;
		} else if (step_count >= MAX_POLL_STEP_COUNT) {
			return -EIO;
		}

		step_count++;
		k_sleep(K_USEC(step_sleep_duration));
	}

	switch (channel) {
	case SENSOR_CHAN_ALL: {
		if (ITDS_getRawAccelerationX(&data->sensor_interface, &acceleration_x) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}

		if (ITDS_getRawAccelerationY(&data->sensor_interface, &acceleration_y) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}

		if (ITDS_getRawAccelerationZ(&data->sensor_interface, &acceleration_z) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}

		data->acceleration_x =
			ITDS_convertAcceleration_int(acceleration_x, data->sensor_range);
		data->acceleration_y =
			ITDS_convertAcceleration_int(acceleration_y, data->sensor_range);
		data->acceleration_z =
			ITDS_convertAcceleration_int(acceleration_z, data->sensor_range);

		if (ITDS_getRawTemperature12bit(&data->sensor_interface, &temperature) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "temperature");
			return -EIO;
		}
		data->temperature = itds_2533020201601_raw_temp_to_celsius(temperature);
		break;
	}
	case SENSOR_CHAN_AMBIENT_TEMP: {
		if (ITDS_getRawTemperature12bit(&data->sensor_interface, &temperature) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "temperature");
			return -EIO;
		}
		data->temperature = itds_2533020201601_raw_temp_to_celsius(temperature);
		break;
	}
	case SENSOR_CHAN_ACCEL_X: {
		if (ITDS_getRawAccelerationX(&data->sensor_interface, &acceleration_x) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}
		data->acceleration_x =
			ITDS_convertAcceleration_int(acceleration_x, data->sensor_range);
		break;
	}
	case SENSOR_CHAN_ACCEL_Y: {
		if (ITDS_getRawAccelerationY(&data->sensor_interface, &acceleration_y) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}
		data->acceleration_y =
			ITDS_convertAcceleration_int(acceleration_y, data->sensor_range);
		break;
	}
	case SENSOR_CHAN_ACCEL_Z: {
		if (ITDS_getRawAccelerationZ(&data->sensor_interface, &acceleration_z) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}
		data->acceleration_z =
			ITDS_convertAcceleration_int(acceleration_z, data->sensor_range);
		break;
	}
	case SENSOR_CHAN_ACCEL_XYZ: {
		if (ITDS_getRawAccelerationX(&data->sensor_interface, &acceleration_x) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}

		if (ITDS_getRawAccelerationY(&data->sensor_interface, &acceleration_y) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}

		if (ITDS_getRawAccelerationZ(&data->sensor_interface, &acceleration_z) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "acceleration");
			return -EIO;
		}

		data->acceleration_x =
			ITDS_convertAcceleration_int(acceleration_x, data->sensor_range);
		data->acceleration_y =
			ITDS_convertAcceleration_int(acceleration_y, data->sensor_range);
		data->acceleration_z =
			ITDS_convertAcceleration_int(acceleration_z, data->sensor_range);
		break;
	}
	default:
		break;
	}

	return 0;
}

/* Convert acceleration value from mg (int16) to m/s^2 (sensor_value). */
static inline void itds_2533020201601_convert_acceleration(struct sensor_value *val,
							   int16_t raw_val)
{
	int64_t dval;
	/* Convert to m/s^2 */
	dval = (((int64_t)raw_val) * SENSOR_G) / 1000000LL;
	val->val1 = dval / 1000LL;
	val->val2 = (dval % 1000LL) * 1000;
}

static int itds_2533020201601_channel_get(const struct device *dev, enum sensor_channel channel,
					  struct sensor_value *value)
{
	struct itds_2533020201601_data *data = dev->data;

	switch (channel) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		value->val1 = (int32_t)data->temperature / 100;
		value->val2 = ((int32_t)data->temperature % 100) * (1000000 / 100);
		break;
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		/* Convert requested acceleration(s) */
		if (channel == SENSOR_CHAN_ACCEL_X || channel == SENSOR_CHAN_ACCEL_XYZ) {
			itds_2533020201601_convert_acceleration(value, data->acceleration_x);
			value++;
		}
		if (channel == SENSOR_CHAN_ACCEL_Y || channel == SENSOR_CHAN_ACCEL_XYZ) {
			itds_2533020201601_convert_acceleration(value, data->acceleration_y);
			value++;
		}
		if (channel == SENSOR_CHAN_ACCEL_Z || channel == SENSOR_CHAN_ACCEL_XYZ) {
			itds_2533020201601_convert_acceleration(value, data->acceleration_z);
			value++;
		}
		break;
	default:
		LOG_ERR("Channel not supported %d", channel);
		return -ENOTSUP;
	}

	return 0;
}

/* Set full scale (measurement range). See itds_2533020201601_full_scale_list for allowed values. */
static int itds_2533020201601_full_scale_set(const struct device *dev,
					     const struct sensor_value *fs)
{
	struct itds_2533020201601_data *data = dev->data;

	uint8_t scaleg = (uint8_t)sensor_ms2_to_g(fs);

	uint8_t idx;

	for (idx = 0; idx < ARRAY_SIZE(itds_2533020201601_full_scale_list); idx++) {
		if (itds_2533020201601_full_scale_list[idx] == scaleg) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(itds_2533020201601_full_scale_list)) {
		/* ODR not allowed (was not found in itds_2533020201601_full_scale_list) */
		LOG_ERR("Bad scale %d", scaleg);
		return -EINVAL;
	}

	if (ITDS_setFullScale(&data->sensor_interface, (ITDS_fullScale_t)idx) != WE_SUCCESS) {
		LOG_ERR("Failed to set full scale.");
		return -EIO;
	}

	data->sensor_range = idx;

	return 0;
}

/* Get full scale (measurement range). See itds_2533020201601_full_scale_list for allowed values. */
static int itds_2533020201601_full_scale_get(const struct device *dev, struct sensor_value *fs)
{
	struct itds_2533020201601_data *data = dev->data;

	ITDS_fullScale_t full_scale;

	if (ITDS_getFullScale(&data->sensor_interface, &full_scale) != WE_SUCCESS) {
		LOG_ERR("Failed to get full scale");
		return -EIO;
	}

	data->sensor_range = full_scale;

	fs->val1 = itds_2533020201601_full_scale_list[full_scale];
	fs->val2 = 0;

	return 0;
}

/* Set output data rate. See itds_2533020201601_odr_list for allowed values. */
static int itds_2533020201601_odr_set(const struct device *dev, const struct sensor_value *odr)
{
	struct itds_2533020201601_data *data = dev->data;
	int odr_index;

	for (odr_index = 0; odr_index < ARRAY_SIZE(itds_2533020201601_odr_list); odr_index++) {
		if (odr->val1 == itds_2533020201601_odr_list[odr_index].val1 &&
		    odr->val2 == itds_2533020201601_odr_list[odr_index].val2) {
			break;
		}
	}

	if (odr_index == ARRAY_SIZE(itds_2533020201601_odr_list)) {
		/* ODR not allowed (was not found in itds_2533020201601_odr_list) */
		LOG_ERR("Bad sampling frequency %d.%d", odr->val1, odr->val2);
		return -EINVAL;
	}

	ITDS_outputDataRate_t odr_enum = (ITDS_outputDataRate_t)odr_index;

	if (ITDS_setOutputDataRate(&data->sensor_interface, odr_enum) != WE_SUCCESS) {
		LOG_ERR("Failed to set output data rate");
		return -EIO;
	}

	data->sensor_odr = odr_enum;

	return 0;
}

/* Get output data rate. See itds_2533020201601_odr_list for allowed values. */
static int itds_2533020201601_odr_get(const struct device *dev, struct sensor_value *odr)
{
	struct itds_2533020201601_data *data = dev->data;

	ITDS_outputDataRate_t odr_index;

	if (ITDS_getOutputDataRate(&data->sensor_interface, &odr_index) != WE_SUCCESS) {
		LOG_ERR("Failed to get output data rate");
		return -EIO;
	}

	data->sensor_odr = odr_index;

	odr->val1 = itds_2533020201601_odr_list[odr_index].val1;
	odr->val2 = itds_2533020201601_odr_list[odr_index].val2;

	return 0;
}

static int itds_2533020201601_attr_set(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, const struct sensor_value *val)
{
	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (chan != SENSOR_CHAN_ALL) {
			break;
		}
		return itds_2533020201601_odr_set(dev, val);
	case SENSOR_ATTR_FULL_SCALE:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			return itds_2533020201601_full_scale_set(dev, val);
		default:
			break;
		}
		break;
	default:
		break;
	}

	LOG_ERR("attr_set() is not supported on channel %d.", chan);
	return -ENOTSUP;
}

static int itds_2533020201601_attr_get(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, struct sensor_value *val)
{

	if (val == NULL) {
		LOG_WRN("address of passed value is NULL.");
		return -EFAULT;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (chan != SENSOR_CHAN_ALL) {
			break;
		}
		return itds_2533020201601_odr_get(dev, val);
	case SENSOR_ATTR_FULL_SCALE:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			return itds_2533020201601_full_scale_get(dev, val);
		default:
			break;
		}
		break;
	default:
		break;
	}

	LOG_ERR("attr_get() is not supported on channel %d.", chan);
	return -ENOTSUP;
}

static DEVICE_API(sensor, itds_2533020201601_driver_api) = {
	.attr_set = itds_2533020201601_attr_set,
#if CONFIG_WSEN_ITDS_2533020201601_TRIGGER
	.trigger_set = itds_2533020201601_trigger_set,
#endif
	.attr_get = itds_2533020201601_attr_get,
	.sample_fetch = itds_2533020201601_sample_fetch,
	.channel_get = itds_2533020201601_channel_get,
};

int itds_2533020201601_init(const struct device *dev)
{
	const struct itds_2533020201601_config *config = dev->config;
	struct itds_2533020201601_data *data = dev->data;
	struct sensor_value fs;
	uint8_t device_id;
	ITDS_state_t sw_reset;

	/* Initialize WE sensor interface */
	WE_sensorInterfaceType_t interface_type = data->sensor_interface.interfaceType;

	ITDS_getDefaultInterface(&data->sensor_interface);
	data->sensor_interface.interfaceType = interface_type;

	switch (data->sensor_interface.interfaceType) {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	case WE_i2c:
		if (!i2c_is_ready_dt(&config->bus_cfg.i2c)) {
			LOG_ERR("I2C bus device not ready");
			return -ENODEV;
		}
		data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;
		break;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	case WE_spi:
		if (!spi_is_ready_dt(&config->bus_cfg.spi)) {
			LOG_ERR("SPI bus device not ready");
			return -ENODEV;
		}
		data->sensor_interface.handle = (void *)&config->bus_cfg.spi;
		break;
#endif
	default:
		LOG_ERR("Invalid interface type");
		return -EINVAL;
	}

	/* First communication test - check device ID */
	if (ITDS_getDeviceID(&data->sensor_interface, &device_id) != WE_SUCCESS) {
		LOG_ERR("Failed to read device ID.");
		return -EIO;
	}

	if (device_id != ITDS_DEVICE_ID_VALUE) {
		LOG_ERR("Invalid device ID 0x%x.", device_id);
		return -EINVAL;
	}

	/* Perform soft reset of the sensor */
	ITDS_softReset(&data->sensor_interface, ITDS_enable);

	k_sleep(K_USEC(5));

	do {
		if (ITDS_getSoftResetState(&data->sensor_interface, &sw_reset) != WE_SUCCESS) {
			LOG_ERR("Failed to get sensor reset state.");
			return -EIO;
		}
	} while (sw_reset);

	if (ITDS_setOperatingMode(&data->sensor_interface, config->op_mode) != WE_SUCCESS) {
		LOG_ERR("Failed to set operating mode.");
		return -EIO;
	}

	if (ITDS_setPowerMode(&data->sensor_interface, config->power_mode) != WE_SUCCESS) {
		LOG_ERR("Failed to set power mode.");
		return -EIO;
	}

	if (itds_2533020201601_odr_set(dev, &itds_2533020201601_odr_list[config->odr]) < 0) {
		LOG_ERR("Failed to set output data rate.");
		return -EIO;
	}

	if (ITDS_enableLowNoise(&data->sensor_interface, config->low_noise) != WE_SUCCESS) {
		LOG_ERR("Failed to set low-noise mode.");
		return -EIO;
	}

	sensor_g_to_ms2((int32_t)config->range, &fs);

	if (itds_2533020201601_full_scale_set(dev, &fs) < 0) {
		LOG_ERR("Failed to set full scale");
		return -EIO;
	}

	if (ITDS_enableAutoIncrement(&data->sensor_interface, ITDS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable auto increment.");
		return -EIO;
	}

	if (ITDS_enableBlockDataUpdate(&data->sensor_interface, ITDS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable block data update.");
		return -EIO;
	}

	if (config->op_mode == ITDS_singleConversion) {
		if (ITDS_setSingleDataConversionTrigger(&data->sensor_interface,
							ITDS_registerTrigger) != WE_SUCCESS) {
			LOG_ERR("Failed to set single data conversion trigger.");
			return -EIO;
		}
	}

#if CONFIG_WSEN_ITDS_2533020201601_TRIGGER
	if (itds_2533020201601_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt(s).");
		return -EIO;
	}
#endif

	return 0;
}

/* clang-format off */

#ifdef CONFIG_WSEN_ITDS_2533020201601_TRIGGER
#define ITDS_2533020201601_CFG_EVENTS_IRQ(inst) \
	.events_interrupt_gpio = \
		GPIO_DT_SPEC_INST_GET(inst, events_interrupt_gpios),

#define ITDS_2533020201601_CFG_DRDY_IRQ(inst) \
	.drdy_interrupt_gpio = \
		GPIO_DT_SPEC_INST_GET(inst, drdy_interrupt_gpios),
#else
#define ITDS_2533020201601_CFG_EVENTS_IRQ(inst)
#define ITDS_2533020201601_CFG_DRDY_IRQ(inst)
#endif /* CONFIG_WSEN_ITDS_2533020201601_TRIGGER */

#ifdef CONFIG_WSEN_ITDS_2533020201601_TAP
#define ITDS_2533020201601_CONFIG_TAP(inst) \
	.tap_mode = DT_INST_PROP(inst, tap_mode), \
	.tap_threshold = DT_INST_PROP(inst, tap_threshold), \
	.tap_shock = DT_INST_PROP(inst, tap_shock), \
	.tap_latency = DT_INST_PROP(inst, tap_latency), \
	.tap_quiet = DT_INST_PROP(inst, tap_quiet),
#else
#define ITDS_2533020201601_CONFIG_TAP(inst)
#endif /* CONFIG_WSEN_ITDS_2533020201601_TAP */

#ifdef CONFIG_WSEN_ITDS_2533020201601_FREEFALL
#define ITDS_2533020201601_CONFIG_FREEFALL(inst) \
	.freefall_duration = DT_INST_PROP(inst, freefall_duration), \
	.freefall_threshold = \
		(ITDS_FreeFallThreshold_t)DT_INST_ENUM_IDX(inst, freefall_threshold),
#else
#define ITDS_2533020201601_CONFIG_FREEFALL(inst)
#endif /* CONFIG_WSEN_ITDS_2533020201601_FREEFALL */

#ifdef CONFIG_WSEN_ITDS_2533020201601_DELTA
#define ITDS_2533020201601_CONFIG_DELTA(inst) \
	.delta_threshold = DT_INST_PROP(inst, delta_threshold), \
	.delta_duration = DT_INST_PROP(inst, delta_duration), \
	.delta_offsets = DT_INST_PROP(inst, delta_offsets), \
	.delta_offset_weight = DT_INST_PROP(inst, delta_offset_weight),
#else
#define ITDS_2533020201601_CONFIG_DELTA(inst)
#endif /* CONFIG_WSEN_ITDS_2533020201601_DELTA */

#define ITDS_2533020201601_CONFIG_LN(inst) \
	.low_noise = COND_CODE_1( \
		DT_INST_NODE_HAS_PROP(inst, low_noise), \
		((ITDS_state_t)ITDS_enable), \
		((ITDS_state_t)ITDS_disable) \
	),

#define ITDS_2533020201601_CONFIG_COMMON(inst) \
	.odr = (ITDS_outputDataRate_t)(DT_INST_ENUM_IDX(inst, odr) + 1), \
	.op_mode = (ITDS_operatingMode_t)DT_INST_ENUM_IDX(inst, op_mode), \
	.power_mode = power_mode_map[DT_INST_ENUM_IDX(inst, power_mode)], \
	.range = DT_INST_PROP(inst, range), \
	ITDS_2533020201601_CONFIG_LN(inst) \
	ITDS_2533020201601_CONFIG_TAP(inst) \
	ITDS_2533020201601_CONFIG_FREEFALL(inst) \
	ITDS_2533020201601_CONFIG_DELTA(inst) \
	COND_CODE_1( \
		DT_INST_NODE_HAS_PROP(inst, events_interrupt_gpios), \
		(ITDS_2533020201601_CFG_EVENTS_IRQ(inst)), \
		() \
	) \
	COND_CODE_1( \
		DT_INST_NODE_HAS_PROP(inst, drdy_interrupt_gpios), \
		(ITDS_2533020201601_CFG_DRDY_IRQ(inst)), \
		() \
	)

/* SPI configuration */
#define ITDS_2533020201601_SPI_OPERATION \
	(SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define ITDS_2533020201601_CONFIG_SPI(inst)                                                        \
	{.bus_cfg =                                                                                \
		 {                                                                                 \
			 .spi = SPI_DT_SPEC_INST_GET(inst, ITDS_2533020201601_SPI_OPERATION),      \
		 },                                                                                \
	 ITDS_2533020201601_CONFIG_COMMON(inst)}

/* I2C configuration */
#define ITDS_2533020201601_CONFIG_I2C(inst) \
	{ \
		.bus_cfg = { \
			.i2c = I2C_DT_SPEC_INST_GET(inst), \
		}, \
		ITDS_2533020201601_CONFIG_COMMON(inst) \
	}

#define ITDS_2533020201601_CONFIG_WE_INTERFACE(inst) \
	{ \
		COND_CODE_1( \
			DT_INST_ON_BUS(inst, i2c), \
			(.sensor_interface = {.interfaceType = WE_i2c}), \
			() \
		) \
		COND_CODE_1( \
			DT_INST_ON_BUS(inst, spi), \
			(.sensor_interface = {.interfaceType = WE_spi}), \
			() \
		) \
	}

/* Main instantiation macro */
#define ITDS_2533020201601_DEFINE(inst) \
	static struct itds_2533020201601_data itds_2533020201601_data_##inst = \
		ITDS_2533020201601_CONFIG_WE_INTERFACE(inst); \
	static const struct itds_2533020201601_config itds_2533020201601_config_##inst = \
		COND_CODE_1( \
			DT_INST_ON_BUS(inst, i2c), \
			(ITDS_2533020201601_CONFIG_I2C(inst)), \
			() \
		) \
		COND_CODE_1( \
			DT_INST_ON_BUS(inst, spi), \
			(ITDS_2533020201601_CONFIG_SPI(inst)), \
			() \
		); \
	SENSOR_DEVICE_DT_INST_DEFINE( \
		inst, \
		itds_2533020201601_init, \
		NULL, \
		&itds_2533020201601_data_##inst, \
		&itds_2533020201601_config_##inst, \
		POST_KERNEL, \
		CONFIG_SENSOR_INIT_PRIORITY, \
		&itds_2533020201601_driver_api \
	);

DT_INST_FOREACH_STATUS_OKAY(ITDS_2533020201601_DEFINE)

/* clang-format on */
