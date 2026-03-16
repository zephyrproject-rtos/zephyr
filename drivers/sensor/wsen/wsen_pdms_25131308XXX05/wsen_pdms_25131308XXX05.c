/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_pdms_25131308xxx05

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <string.h>

#include "wsen_pdms_25131308XXX05.h"

LOG_MODULE_REGISTER(WSEN_PDMS_25131308XXX05, CONFIG_SENSOR_LOG_LEVEL);

static int pdms_25131308XXX05_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_PRESS: {
		break;
	}
	default:
		LOG_ERR("Invalid channel.");
		return -ENOTSUP;
	}

	const struct pdms_25131308XXX05_config *const config = dev->config;
	struct pdms_25131308XXX05_data *data = dev->data;

	uint16_t status;

	switch (data->sensor_interface.interfaceType) {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	case WE_i2c: {
		if (config->crc) {
			if (PDMS_I2C_GetRawPressureAndTemperature_WithCRC(
				    &data->sensor_interface, &data->pressure_data,
				    &data->temperature_data, &status) != WE_SUCCESS) {
				LOG_ERR("Failed to retrieve data from the sensor.");
				return -EIO;
			}
		} else {
			if (PDMS_I2C_GetRawPressureAndTemperature(
				    &data->sensor_interface, &data->pressure_data,
				    &data->temperature_data, &status) != WE_SUCCESS) {
				LOG_ERR("Failed to retrieve data from the sensor.");
				return -EIO;
			}
		}
		break;
	}
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	case WE_spi: {
		if (config->crc) {
			if (PDMS_SPI_getRawPressureAndTemperature_WithCRC(
				    &data->sensor_interface, &data->pressure_data,
				    &data->temperature_data, &status) != WE_SUCCESS) {
				LOG_ERR("Failed to retrieve data from the sensor.");
				return -EIO;
			}
		} else {
			if (PDMS_SPI_GetRawPressureAndTemperature(
				    &data->sensor_interface, &data->pressure_data,
				    &data->temperature_data, &status) != WE_SUCCESS) {
				LOG_ERR("Failed to retrieve data from the sensor.");
				return -EIO;
			}
		}
		break;
	}
#endif
	default:
		return -EIO;
	}

	return 0;
}

static int pdms_25131308XXX05_channel_get(const struct device *dev, enum sensor_channel chan,
					  struct sensor_value *val)
{
	const struct pdms_25131308XXX05_config *const config = dev->config;
	struct pdms_25131308XXX05_data *data = dev->data;

	int status = 0;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP: {
		int64_t temp = ((int64_t)(data->temperature_data - T_MIN_TYP_VAL_PDMS)) * 4272;

		status = sensor_value_from_micro(val, temp);
		break;
	}
	case SENSOR_CHAN_PRESS: {
		int64_t pascal = (int64_t)(data->pressure_data - P_MIN_TYP_VAL_PDMS);

		/*
		 * these values are conversion factors based on the sensor type defined in the user
		 * manual of the respective sensor
		 */
		switch (config->sensor_type) {
		case PDMS_pdms0:
			pascal = ((pascal * 763) / 10000) - 1000;
			break;
		case PDMS_pdms1:
			pascal = ((pascal * 763) / 1000) - 10000;
			break;
		case PDMS_pdms2:
			pascal = (((pascal * 2670) / 1000) - 35000);
			break;
		case PDMS_pdms3:
			pascal = ((pascal * 381) / 100);
			break;
		case PDMS_pdms4:
			pascal = ((pascal * 4190) / 100) - 100000;
			break;
		default:
			LOG_ERR("Sensor type doesn't exist");
			return -ENOTSUP;
		}

		status = sensor_value_from_milli(val, pascal);
		break;
	}
	default:
		LOG_ERR("Invalid channel.");
		return -ENOTSUP;
	}

	return status;
}

static int pdms_25131308XXX05_init(const struct device *dev)
{
	const struct pdms_25131308XXX05_config *const config = dev->config;
	struct pdms_25131308XXX05_data *data = dev->data;

	/* Initialize WE sensor interface */
	WE_sensorInterfaceType_t interface_type = data->sensor_interface.interfaceType;

	if (PDMS_getDefaultInterface(&data->sensor_interface) != WE_SUCCESS) {
		return -EIO;
	}

	data->sensor_interface.interfaceType = interface_type;

	switch (data->sensor_interface.interfaceType) {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	case WE_i2c: {
		if (!i2c_is_ready_dt(&config->bus_cfg.i2c)) {
			LOG_ERR("I2C bus device not ready");
			return -ENODEV;
		}

		switch (config->bus_cfg.i2c.addr) {
		case PDMS_I2C_ADDRESS_CRC: {
			if (!config->crc) {
				LOG_ERR("I2C with CRC disabled but the wrong I2C address is "
					"chosen.");
				return -ENODEV;
			}
			break;
		}
		case PDMS_I2C_ADDRESS: {
			if (config->crc) {
				LOG_ERR("I2C with CRC enabled but the wrong I2C address is "
					"chosen.");
				return -ENODEV;
			}
			break;
		}
		default:
			LOG_ERR("Invalid I2C address.");
			return -ENODEV;
		}

		data->sensor_interface.options.i2c.address = config->bus_cfg.i2c.addr;
		data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;
		break;
	}
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	case WE_spi: {
		if (!spi_is_ready_dt(&config->bus_cfg.spi)) {
			LOG_ERR("SPI bus device not ready");
			return -ENODEV;
		}

		data->spi_crc = (config->crc ? PDMS_SPI_withCRC : PDMS_SPI_withoutCRC);
		data->sensor_interface.options.spi.sensorSpecificSettings = (void *)&data->spi_crc;
		data->sensor_interface.options.spi.duplexMode = 1;
		data->sensor_interface.options.spi.burstMode = 1;
		data->sensor_interface.handle = (void *)&config->bus_cfg.spi;
		break;
	}
#endif
	default:
		LOG_ERR("Invalid interface type");
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(sensor, pdms_25131308XXX05_driver_api) = {
	.sample_fetch = pdms_25131308XXX05_sample_fetch,
	.channel_get = pdms_25131308XXX05_channel_get,
};

/* clang-format off */

#define PDMS_25131308XXX05_CONFIG_WE_INTERFACE(inst) \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c), \
		(.sensor_interface = { \
			.interfaceType = WE_i2c \
		}), \
		()) \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), \
		(.sensor_interface = { \
			.interfaceType = WE_spi \
		}), \
		())

#define PDMS_25131308XXX05_SPI_OPERATION \
	(SPI_WORD_SET(8) | SPI_OP_MODE_MASTER)

#define PDMS_25131308XXX05_CONFIG_BUS(inst) \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c), \
		(.bus_cfg = { \
			.i2c = I2C_DT_SPEC_INST_GET(inst) \
		},), \
		()) \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi), \
		(.bus_cfg = { \
			.spi = SPI_DT_SPEC_INST_GET( \
				inst, \
				PDMS_25131308XXX05_SPI_OPERATION, \
				0 \
			) \
		},), \
		())

#define PDMS_25131308XXX05_CONFIG_SENSOR_TYPE(inst) \
	.sensor_type = (PDMS_SensorType_t)DT_INST_ENUM_IDX(inst, sensor_type),

#define PDMS_25131308XXX05_CONFIG_CRC(inst) \
	.crc = DT_INST_PROP(inst, crc),

#define PDMS_25131308XXX05_CONFIG(inst) \
	PDMS_25131308XXX05_CONFIG_BUS(inst) \
	PDMS_25131308XXX05_CONFIG_SENSOR_TYPE(inst) \
	PDMS_25131308XXX05_CONFIG_CRC(inst)

/*
 * Main instantiation macro.
 */
#define PDMS_25131308XXX05_DEFINE(inst) \
	static struct pdms_25131308XXX05_data pdms_25131308XXX05_data_##inst = { \
		PDMS_25131308XXX05_CONFIG_WE_INTERFACE(inst) \
	}; \
	static const struct pdms_25131308XXX05_config pdms_25131308XXX05_config_##inst = { \
		PDMS_25131308XXX05_CONFIG(inst) \
	}; \
	SENSOR_DEVICE_DT_INST_DEFINE( \
		inst, \
		pdms_25131308XXX05_init, \
		NULL, \
		&pdms_25131308XXX05_data_##inst, \
		&pdms_25131308XXX05_config_##inst, \
		POST_KERNEL, \
		CONFIG_SENSOR_INIT_PRIORITY, \
		&pdms_25131308XXX05_driver_api \
	)

DT_INST_FOREACH_STATUS_OKAY(PDMS_25131308XXX05_DEFINE)

/* clang-format on */
