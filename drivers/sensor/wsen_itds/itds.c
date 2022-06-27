/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_itds

#include <string.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "itds.h"

LOG_MODULE_REGISTER(ITDS, CONFIG_SENSOR_LOG_LEVEL);

/*
 * List of supported output data rates (sensor_value struct, input to
 * sensor_attr_set()). Index into this list is used as argument for
 * ITDS_setOutputDataRate().
 */
static const struct sensor_value itds_odr_list[] = {
	{ .val1 = 0, .val2 = 0 },	    { .val1 = 1, .val2 = 6 * 100000 },
	{ .val1 = 12, .val2 = 5 * 100000 }, { .val1 = 25, .val2 = 0 },
	{ .val1 = 50, .val2 = 0 },	    { .val1 = 100, .val2 = 0 },
	{ .val1 = 200, .val2 = 0 },	    { .val1 = 400, .val2 = 0 },
	{ .val1 = 800, .val2 = 0 },	    { .val1 = 1600, .val2 = 0 }
};

/*
 * List of supported full scale values (i.e. measurement ranges, in g).
 * Index into this list is used as input for ITDS_setFullScale().
 */
static const int itds_full_scale_list[] = { 2, 4, 8, 16 };

static int itds_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	struct itds_data *data = dev->data;
	int16_t temperature;

	__ASSERT_NO_MSG(channel == SENSOR_CHAN_ALL);

	if (WE_SUCCESS != ITDS_getAccelerations_int(&data->sensor_interface, 1,
						    &data->acceleration_x, &data->acceleration_y,
						    &data->acceleration_z)) {
		LOG_ERR("Failed to fetch acceleration sample.");
		return -EIO;
	}

	if (WE_SUCCESS != ITDS_getRawTemperature12bit(&data->sensor_interface, &temperature)) {
		LOG_ERR("Failed to fetch temperature sample.");
		return -EIO;
	}
	data->temperature = ((temperature * 100) / 16) + 2500;

	return 0;
}

/* Convert acceleration value from mg (int16) to m/s^2 (sensor_value). */
static inline void itds_convert_acceleration(struct sensor_value *val, int16_t raw_val)
{
	int64_t dval;

	/* Convert to m/s^2 */
	dval = (((int64_t)raw_val) * SENSOR_G) / 1000000LL;
	val->val1 = dval / 1000LL;
	val->val2 = (dval % 1000LL) * 1000;
}

static int itds_channel_get(const struct device *dev, enum sensor_channel channel,
			    struct sensor_value *value)
{
	struct itds_data *data = dev->data;
	int32_t value_converted;
	int result = -ENOTSUP;

	if (channel == SENSOR_CHAN_AMBIENT_TEMP) {
		value_converted = (int32_t)data->temperature;

		/* Convert temperature from 0.01 degrees Celsius to degrees Celsius */
		value->val1 = value_converted / 100;
		value->val2 = (value_converted % 100) * (1000000 / 100);

		result = 0;
	} else {
		/* Convert requested acceleration(s) */
		if (channel == SENSOR_CHAN_ACCEL_X || channel == SENSOR_CHAN_ACCEL_XYZ) {
			itds_convert_acceleration(value, data->acceleration_x);
			value++;
			result = 0;
		}
		if (channel == SENSOR_CHAN_ACCEL_Y || channel == SENSOR_CHAN_ACCEL_XYZ) {
			itds_convert_acceleration(value, data->acceleration_y);
			value++;
			result = 0;
		}
		if (channel == SENSOR_CHAN_ACCEL_Z || channel == SENSOR_CHAN_ACCEL_XYZ) {
			itds_convert_acceleration(value, data->acceleration_z);
			value++;
			result = 0;
		}
	}

	return result;
}

/* Set output data rate. See itds_odr_list for allowed values. */
static int itds_odr_set(const struct device *dev, const struct sensor_value *odr)
{
	struct itds_data *data = dev->data;
	int odr_index;

	for (odr_index = 0; odr_index < ARRAY_SIZE(itds_odr_list); odr_index++) {
		if (odr->val1 == itds_odr_list[odr_index].val1 &&
		    odr->val2 == itds_odr_list[odr_index].val2) {
			break;
		}
	}

	if (odr_index == ARRAY_SIZE(itds_odr_list)) {
		/* ODR not allowed (was not found in itds_odr_list) */
		LOG_ERR("Bad sampling frequency %d.%d", odr->val1, odr->val2);
		return -EINVAL;
	}

	if (WE_SUCCESS !=
	    ITDS_setOutputDataRate(&data->sensor_interface, (ITDS_outputDataRate_t)odr_index)) {
		LOG_ERR("Failed to set output data rate");
		return -EIO;
	}

	return 0;
}

/* Set full scale (measurement range). See itds_full_scale_list for allowed values. */
int itds_full_scale_set(const struct device *dev, int fs)
{
	struct itds_data *data = dev->data;
	uint8_t idx;

	for (idx = 0; idx < ARRAY_SIZE(itds_full_scale_list); idx++) {
		if (itds_full_scale_list[idx] == fs) {
			if (WE_SUCCESS ==
			    ITDS_setFullScale(&data->sensor_interface, (ITDS_fullScale_t)idx)) {
				return 0;
			}
			LOG_ERR("Failed to set full scale.");
			return -EIO;
		}
	}
	return -EINVAL;
}

static int itds_attr_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return itds_odr_set(dev, val);

	default:
		LOG_ERR("Operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api itds_driver_api = { .attr_set = itds_attr_set,
#if CONFIG_ITDS_TRIGGER
							  .trigger_set = itds_trigger_set,
#endif
							  .sample_fetch = itds_sample_fetch,
							  .channel_get = itds_channel_get };

int itds_init(const struct device *dev)
{
	const struct itds_config *config = dev->config;
	struct itds_data *data = dev->data;
	uint8_t device_id;
	int8_t status;
	ITDS_state_t sw_reset;

	/* Initialize WE sensor interface */
	WE_sensorInterfaceType_t interface_type = data->sensor_interface.interfaceType;
	ITDS_getDefaultInterface(&data->sensor_interface);
	data->sensor_interface.interfaceType = interface_type;
	if (data->sensor_interface.interfaceType == WE_i2c) {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;
#endif
	} else {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		data->sensor_interface.handle = (void *)&config->bus_cfg.spi;
#endif
	}

	/* First communication test - check device ID */
	if (WE_SUCCESS != ITDS_getDeviceID(&data->sensor_interface, &device_id)) {
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
		if (WE_SUCCESS != ITDS_getSoftResetState(&data->sensor_interface, &sw_reset)) {
			LOG_ERR("Failed to get sensor reset state.");
			return -EIO;
		}
	} while (sw_reset);

	if (WE_SUCCESS != ITDS_setOperatingMode(&data->sensor_interface,
						config->op_mode == itds_op_mode_high_performance ?
							ITDS_highPerformance :
							ITDS_normalOrLowPower)) {
		LOG_ERR("Failed to set operating mode.");
		return -EIO;
	}

	if (WE_SUCCESS != ITDS_setOutputDataRate(&data->sensor_interface, config->odr)) {
		LOG_ERR("Failed to set output data rate.");
		return -EIO;
	}

	if (config->low_noise) {
		if (WE_SUCCESS != ITDS_enableLowNoise(&data->sensor_interface, ITDS_enable)) {
			LOG_ERR("Failed to enable low-noise mode.");
			return -EIO;
		}
	}

	if (WE_SUCCESS != ITDS_enableBlockDataUpdate(&data->sensor_interface, ITDS_enable)) {
		LOG_ERR("Failed to enable block data update.");
		return -EIO;
	}

	if (WE_SUCCESS !=
	    ITDS_setPowerMode(&data->sensor_interface, config->op_mode == itds_op_mode_low_power ?
							       ITDS_lowPower :
							       ITDS_normalMode)) {
		LOG_ERR("Failed to set power mode.");
		return -EIO;
	}

	status = itds_full_scale_set(dev, config->range);
	if (status < 0) {
		return status;
	}

#if CONFIG_ITDS_TRIGGER
	status = itds_init_interrupt(dev);
	if (status < 0) {
		LOG_ERR("Failed to initialize interrupt(s).");
		return status;
	}
#endif

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "ITDS driver enabled without any devices"
#endif

/*
 * Device creation macros
 */

#define ITDS_DEVICE_INIT(inst)                                                                     \
	DEVICE_DT_INST_DEFINE(inst,					\
				itds_init,						\
				NULL,							\
				&itds_data_##inst,				\
				&itds_config_##inst,			\
				POST_KERNEL,					\
				CONFIG_SENSOR_INIT_PRIORITY,	\
				&itds_driver_api);

#ifdef CONFIG_ITDS_TRIGGER
#define ITDS_CFG_IRQ(inst)                                                                         \
	.gpio_interrupts = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                 \
	.drdy_int = DT_INST_PROP(inst, drdy_int)
#else
#define ITDS_CFG_IRQ(inst)
#endif /* CONFIG_ITDS_TRIGGER */

#ifdef CONFIG_ITDS_TAP
#define ITDS_CONFIG_TAP(inst)                                                                      \
	.tap_mode = DT_INST_PROP(inst, tap_mode),                                                  \
	.tap_threshold = DT_INST_PROP(inst, tap_threshold),                                        \
	.tap_shock = DT_INST_PROP(inst, tap_shock),                                                \
	.tap_latency = DT_INST_PROP(inst, tap_latency),                                            \
	.tap_quiet = DT_INST_PROP(inst, tap_quiet),
#else
#define ITDS_CONFIG_TAP(inst)
#endif /* CONFIG_ITDS_TAP */

#ifdef CONFIG_ITDS_FREEFALL
#define ITDS_CONFIG_FREEFALL(inst)                                                                 \
	.freefall_duration = DT_INST_PROP(inst, freefall_duration),                                \
	.freefall_threshold = (ITDS_FreeFallThreshold_t) DT_INST_ENUM_IDX(inst, freefall_threshold),
#else
#define ITDS_CONFIG_FREEFALL(inst)
#endif /* CONFIG_ITDS_FREEFALL */

#ifdef CONFIG_ITDS_DELTA
#define ITDS_CONFIG_DELTA(inst)                                                                    \
	.delta_threshold = DT_INST_PROP(inst, delta_threshold),                                    \
	.delta_duration = DT_INST_PROP(inst, delta_duration),                                      \
	.delta_offsets = DT_INST_PROP(inst, delta_offsets),                                        \
	.delta_offset_weight = DT_INST_PROP(inst, delta_offset_weight),
#else
#define ITDS_CONFIG_DELTA(inst)
#endif /* CONFIG_ITDS_DELTA */

#define ITDS_CONFIG_COMMON(inst)                                                                   \
	.odr = (ITDS_outputDataRate_t)(DT_INST_ENUM_IDX(inst, odr) + 1),                           \
	.op_mode = (itds_op_mode_t)DT_INST_ENUM_IDX(inst, op_mode),                                \
	.range = DT_INST_PROP(inst, range),                                                        \
	.low_noise = DT_INST_PROP(inst, low_noise),                                                \
	ITDS_CONFIG_TAP(inst)                                                                      \
	ITDS_CONFIG_FREEFALL(inst)                                                                 \
	ITDS_CONFIG_DELTA(inst)                                                                    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int_gpios),                                        \
		(ITDS_CFG_IRQ(inst)), ())

/*
 * Instantiation macros used when device is on SPI bus.
 */

#define ITDS_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define ITDS_CONFIG_SPI(inst)                                                                      \
	{                                                                                          \
		.bus_cfg = {                                                                       \
			.spi = SPI_DT_SPEC_INST_GET(inst,                                          \
							ITDS_SPI_OPERATION,                        \
							0),                                        \
		},                                                                                 \
		ITDS_CONFIG_COMMON(inst)                                                           \
	}

/*
 * Instantiation macros used when device is on I2C bus.
 */

#define ITDS_CONFIG_I2C(inst)                                                                      \
	{                                                                                          \
		.bus_cfg = {                                                                       \
			.i2c = I2C_DT_SPEC_INST_GET(inst),                                         \
		},                                                                                 \
		ITDS_CONFIG_COMMON(inst)                                                           \
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define ITDS_DEFINE(inst)                                                                          \
	static struct itds_data itds_data_##inst =                                                 \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                             \
			    ({ .sensor_interface = { .interfaceType = WE_spi } }),                 \
			    ({ .sensor_interface = { .interfaceType = WE_i2c } }));                \
	static const struct itds_config itds_config_##inst =                                       \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                             \
				(ITDS_CONFIG_SPI(inst)),                                           \
				(ITDS_CONFIG_I2C(inst)));                                          \
	ITDS_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(ITDS_DEFINE)
