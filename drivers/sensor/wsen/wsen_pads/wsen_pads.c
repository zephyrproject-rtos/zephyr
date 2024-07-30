/*
 * Copyright (c) 2023 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_pads

#include <string.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "wsen_pads.h"

LOG_MODULE_REGISTER(WSEN_PADS, CONFIG_SENSOR_LOG_LEVEL);

/*
 * List of supported output data rates. Index into this list is used as
 * argument for PADS_setOutputDataRate()
 */
static const int32_t pads_odr_list[] = {
	0, 1, 10, 25, 50, 75, 100, 200,
};

static int pads_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	struct pads_data *data = dev->data;

	__ASSERT_NO_MSG(channel == SENSOR_CHAN_ALL);

	if (PADS_getPressure_int(&data->sensor_interface, &data->pressure) != WE_SUCCESS) {
		LOG_ERR("Failed to fetch %s sample.", "pressure");
		return -EIO;
	}

	if (PADS_getTemperature_int(&data->sensor_interface, &data->temperature) != WE_SUCCESS) {
		LOG_ERR("Failed to fetch %s sample.", "temperature");
		return -EIO;
	}

	return 0;
}

static int pads_channel_get(const struct device *dev, enum sensor_channel channel,
			    struct sensor_value *value)
{
	struct pads_data *data = dev->data;
	int32_t value_converted;

	if (channel == SENSOR_CHAN_AMBIENT_TEMP) {
		value_converted = (int32_t)data->temperature;

		/* Convert temperature from 0.01 degrees Celsius to degrees Celsius */
		value->val1 = value_converted / 100;
		value->val2 = (value_converted % 100) * (1000000 / 100);
	} else if (channel == SENSOR_CHAN_PRESS) {
		value_converted = (int32_t)data->pressure;

		/* Convert pressure from Pa to kPa */
		value->val1 = value_converted / 1000;
		value->val2 = (value_converted % 1000) * (1000000 / 1000);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

/* Set output data rate. See pads_odr_list for allowed values. */
static int pads_odr_set(const struct device *dev, const struct sensor_value *odr)
{
	struct pads_data *data = dev->data;
	int odr_index;

	for (odr_index = 0; odr_index < ARRAY_SIZE(pads_odr_list); odr_index++) {
		if (odr->val1 == pads_odr_list[odr_index] && odr->val2 == 0) {
			break;
		}
	}

	if (odr_index == ARRAY_SIZE(pads_odr_list)) {
		/* ODR not allowed (was not found in pads_odr_list) */
		LOG_ERR("Bad sampling frequency %d.%d", odr->val1, odr->val2);
		return -EINVAL;
	}

	if (PADS_setOutputDataRate(&data->sensor_interface, (PADS_outputDataRate_t)odr_index) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to set output data rate");
		return -EIO;
	}

	return 0;
}

static int pads_attr_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() is not supported on channel %d.", chan);
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		return pads_odr_set(dev, val);
	} else {
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api pads_driver_api = {
	.attr_set = pads_attr_set,
#if CONFIG_WSEN_PADS_TRIGGER
	.trigger_set = pads_trigger_set,
#endif
	.sample_fetch = pads_sample_fetch,
	.channel_get = pads_channel_get,
};

static int pads_init(const struct device *dev)
{
	const struct pads_config *config = dev->config;
	struct pads_data *data = dev->data;
	struct sensor_value odr;
	int status;
	uint8_t device_id;

	/* Initialize WE sensor interface */
	WE_sensorInterfaceType_t interface_type = data->sensor_interface.interfaceType;

	PADS_getDefaultInterface(&data->sensor_interface);
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
	if (PADS_getDeviceID(&data->sensor_interface, &device_id) != WE_SUCCESS) {
		LOG_ERR("Failed to read device ID.");
		return -EIO;
	}

	if (device_id != PADS_DEVICE_ID_VALUE) {
		LOG_ERR("Invalid device ID 0x%x.", device_id);
		return -EINVAL;
	}

	/* Reset sensor */
	PADS_softReset(&data->sensor_interface, PADS_enable);

	k_sleep(K_USEC(50));

	PADS_state_t swReset;

	do {
		if (PADS_getSoftResetState(&data->sensor_interface, &swReset) != WE_SUCCESS) {
			LOG_ERR("Failed to get sensor reset state.");
			return -EIO;
		}
	} while (PADS_enable == swReset);

	if (PADS_enableBlockDataUpdate(&data->sensor_interface, PADS_enable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable block data update.");
		return -EIO;
	}

#if CONFIG_WSEN_PADS_TRIGGER
	status = pads_init_interrupt(dev);
	if (status < 0) {
		LOG_ERR("Failed to initialize data-ready interrupt.");
		return status;
	}
#endif

	odr.val1 = pads_odr_list[config->odr];
	odr.val2 = 0;
	status = pads_odr_set(dev, &odr);
	if (status < 0) {
		LOG_ERR("Failed to set output data rate.");
		return status;
	}

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "PADS driver enabled without any devices"
#endif

/*
 * Device creation macros
 */

#define PADS_DEVICE_INIT(inst)                                                          \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                                                  \
			      pads_init,                                                \
			      NULL,                                                     \
			      &pads_data_##inst,                                        \
			      &pads_config_##inst,                                      \
			      POST_KERNEL,                                              \
			      CONFIG_SENSOR_INIT_PRIORITY,                              \
			      &pads_driver_api);

#ifdef CONFIG_WSEN_PADS_TRIGGER
#define PADS_CFG_IRQ(inst) .gpio_drdy = GPIO_DT_SPEC_INST_GET(inst, drdy_gpios)
#else
#define PADS_CFG_IRQ(inst)
#endif /* CONFIG_WSEN_PADS_TRIGGER */

#define PADS_CONFIG_COMMON(inst)                                                        \
	.odr = (PADS_outputDataRate_t)(DT_INST_ENUM_IDX(inst, odr)),                    \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, drdy_gpios),                            \
		    (PADS_CFG_IRQ(inst)), ())

/*
 * Instantiation macros used when device is on SPI bus.
 */

#define PADS_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define PADS_CONFIG_SPI(inst)                                                           \
	{                                                                               \
		.bus_cfg = {                                                            \
			.spi = SPI_DT_SPEC_INST_GET(inst,                               \
						    PADS_SPI_OPERATION,                 \
						    0),                                 \
		},                                                                      \
		PADS_CONFIG_COMMON(inst)                                                \
	}

/*
 * Instantiation macros used when device is on I2C bus.
 */

#define PADS_CONFIG_I2C(inst)                                                           \
	{                                                                               \
		.bus_cfg = {                                                            \
			.i2c = I2C_DT_SPEC_INST_GET(inst),                              \
		},                                                                      \
		PADS_CONFIG_COMMON(inst)                                                \
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define PADS_DEFINE(inst)                                                               \
	static struct pads_data pads_data_##inst =                                      \
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c),                                  \
			    ({ .sensor_interface = { .interfaceType = WE_i2c } }), ())  \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                  \
			    ({ .sensor_interface = { .interfaceType = WE_spi } }), ()); \
	static const struct pads_config pads_config_##inst =                            \
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c), (PADS_CONFIG_I2C(inst)), ())     \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (PADS_CONFIG_SPI(inst)), ());    \
	PADS_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(PADS_DEFINE)
