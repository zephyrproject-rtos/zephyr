/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_hids

#include <string.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "wsen_hids.h"

LOG_MODULE_REGISTER(WSEN_HIDS, CONFIG_SENSOR_LOG_LEVEL);

/*
 * List of supported output data rates (sensor_value struct, input to
 * sensor_attr_set()). Index into this list is used as argument for
 * HIDS_setOutputDataRate().
 */
static const struct sensor_value hids_odr_list[] = {
	{.val1 = 0, .val2 = 0},
	{.val1 = 1, .val2 = 0},
	{.val1 = 7, .val2 = 0},
	{.val1 = 12, .val2 = 5 * 100000},
};

static int hids_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	struct hids_data *data = dev->data;
	int16_t raw_humidity;
	int16_t raw_temp;

	__ASSERT_NO_MSG(channel == SENSOR_CHAN_ALL);

	if (HIDS_getRawValues(&data->sensor_interface, &raw_humidity, &raw_temp) != WE_SUCCESS) {
		LOG_ERR("Failed to %s sample.", "fetch data");
		return -EIO;
	}

	if (HIDS_convertHumidity_uint16(&data->sensor_interface, raw_humidity, &data->humidity) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to %s sample.", "convert humidity");
		return -EIO;
	}

	if (HIDS_convertTemperature_int16(&data->sensor_interface, raw_temp, &data->temperature) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to %s sample.", "convert temperature");
		return -EIO;
	}

	return 0;
}

static int hids_channel_get(const struct device *dev, enum sensor_channel channel,
			    struct sensor_value *value)
{
	struct hids_data *data = dev->data;
	int32_t value_converted;

	if (channel == SENSOR_CHAN_AMBIENT_TEMP) {
		value_converted = (int32_t)data->temperature;

		/* Convert temperature from 0.01 degrees Celsius to degrees Celsius */
		value->val1 = value_converted / 100;
		value->val2 = (value_converted % 100) * (1000000 / 100);
	} else if (channel == SENSOR_CHAN_HUMIDITY) {
		value_converted = (int32_t)data->humidity;

		/* Convert humidity from 0.01 percent to percent */
		value->val1 = value_converted / 100;
		value->val2 = (value_converted % 100) * (1000000 / 100);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

/* Set output data rate. See hids_odr_list for allowed values. */
static int hids_odr_set(const struct device *dev, const struct sensor_value *odr)
{
	struct hids_data *data = dev->data;
	int odr_index;

	for (odr_index = 0; odr_index < ARRAY_SIZE(hids_odr_list); odr_index++) {
		if (odr->val1 == hids_odr_list[odr_index].val1 &&
		    odr->val2 == hids_odr_list[odr_index].val2) {
			break;
		}
	}

	if (odr_index == ARRAY_SIZE(hids_odr_list)) {
		/* ODR not allowed (was not found in hids_odr_list) */
		LOG_ERR("Bad sampling frequency %d.%d", odr->val1, odr->val2);
		return -EINVAL;
	}

	if (HIDS_setOutputDataRate(&data->sensor_interface, (HIDS_outputDataRate_t)odr_index) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to set output data rate");
		return -EIO;
	}

	return 0;
}

static int hids_attr_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() is not supported on channel %d.", chan);
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		return hids_odr_set(dev, val);
	} else {
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api hids_driver_api = {
	.attr_set = hids_attr_set,
#if CONFIG_WSEN_HIDS_TRIGGER
	.trigger_set = hids_trigger_set,
#endif
	.sample_fetch = hids_sample_fetch,
	.channel_get = hids_channel_get,
};

static int hids_init(const struct device *dev)
{
	const struct hids_config *config = dev->config;
	struct hids_data *data = dev->data;
	uint8_t device_id;

	/* Initialize WE sensor interface */
	WE_sensorInterfaceType_t interface_type = data->sensor_interface.interfaceType;

	HIDS_getDefaultInterface(&data->sensor_interface);
	data->sensor_interface.interfaceType = interface_type;

	switch (data->sensor_interface.interfaceType) {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	case WE_i2c:
		data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;
		break;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	case WE_spi:
		data->sensor_interface.handle = (void *)&config->bus_cfg.spi;
		break;
#endif
	default:
		LOG_ERR("Invalid interface type");
		return -EINVAL;
	}

	/* First communication test - check device ID */
	if (HIDS_getDeviceID(&data->sensor_interface, &device_id) != WE_SUCCESS) {
		LOG_ERR("Failed to read device ID.");
		return -EIO;
	}

	if (device_id != HIDS_DEVICE_ID_VALUE) {
		LOG_ERR("Invalid device ID 0x%x.", device_id);
		return -EINVAL;
	}

	if (HIDS_setOutputDataRate(&data->sensor_interface, config->odr) != WE_SUCCESS) {
		LOG_ERR("Failed to set output data rate.");
		return -EIO;
	}

	if (HIDS_enableBlockDataUpdate(&data->sensor_interface, HIDS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable block data update.");
		return -EIO;
	}

	if (HIDS_setPowerMode(&data->sensor_interface, HIDS_activeMode) != WE_SUCCESS) {
		LOG_ERR("Failed to set power mode.");
		return -EIO;
	}

	if (HIDS_readCalibrationData(&data->sensor_interface) != WE_SUCCESS) {
		LOG_ERR("Failed to read calibration data.");
		return -EIO;
	}

#if CONFIG_WSEN_HIDS_TRIGGER
	int status = hids_init_interrupt(dev);

	if (status < 0) {
		LOG_ERR("Failed to initialize data-ready interrupt.");
		return status;
	}
#endif

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "HIDS driver enabled without any devices"
#endif

/*
 * Device creation macros
 */

#define HIDS_DEVICE_INIT(inst)                                                         \
	DEVICE_DT_INST_DEFINE(inst,                                                    \
			      hids_init,                                               \
			      NULL,                                                    \
			      &hids_data_##inst,                                       \
			      &hids_config_##inst,                                     \
			      POST_KERNEL,                                             \
			      CONFIG_SENSOR_INIT_PRIORITY,                             \
			      &hids_driver_api);

#ifdef CONFIG_WSEN_HIDS_TRIGGER
#define HIDS_CFG_IRQ(inst) .gpio_drdy = GPIO_DT_SPEC_INST_GET(inst, drdy_gpios)
#else
#define HIDS_CFG_IRQ(inst)
#endif /* CONFIG_WSEN_HIDS_TRIGGER */

#define HIDS_CONFIG_COMMON(inst)                                                       \
	.odr = (HIDS_outputDataRate_t)(DT_INST_ENUM_IDX(inst, odr) + 1),               \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, drdy_gpios),                           \
			(HIDS_CFG_IRQ(inst)), ())

/*
 * Instantiation macros used when device is on SPI bus.
 */

#define HIDS_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define HIDS_CONFIG_SPI(inst)                                                          \
	{                                                                              \
		.bus_cfg = {                                                           \
			.spi = SPI_DT_SPEC_INST_GET(inst,                              \
							HIDS_SPI_OPERATION,            \
							0),                            \
		},                                                                     \
		HIDS_CONFIG_COMMON(inst)                                               \
	}

/*
 * Instantiation macros used when device is on I2C bus.
 */

#define HIDS_CONFIG_I2C(inst)                                                          \
	{                                                                              \
		.bus_cfg = {                                                           \
			.i2c = I2C_DT_SPEC_INST_GET(inst),                             \
		},                                                                     \
		HIDS_CONFIG_COMMON(inst)                                               \
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define HIDS_DEFINE(inst)                                                              \
	static struct hids_data hids_data_##inst =                                     \
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c),                                 \
			    ({ .sensor_interface = { .interfaceType = WE_i2c } }), ()) \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                 \
			    ({ .sensor_interface = { .interfaceType = WE_spi } }), ());\
	static const struct hids_config hids_config_##inst =                           \
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c), (HIDS_CONFIG_I2C(inst)), ())      \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (HIDS_CONFIG_SPI(inst)), ());     \
	HIDS_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(HIDS_DEFINE)
