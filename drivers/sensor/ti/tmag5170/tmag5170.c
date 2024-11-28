/*
 * Copyright (c) 2023 Michal Morsisko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmag5170

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#if defined(CONFIG_TMAG5170_CRC)
#include <zephyr/sys/crc.h>
#endif

#include "tmag5170.h"

#define TMAG5170_REG_DEVICE_CONFIG	0x0U
#define TMAG5170_REG_SENSOR_CONFIG	0x1U
#define TMAG5170_REG_SYSTEM_CONFIG	0x2U
#define TMAG5170_REG_ALERT_CONFIG	0x3U
#define TMAG5170_REG_X_THRX_CONFIG	0x4U
#define TMAG5170_REG_Y_THRX_CONFIG	0x5U
#define TMAG5170_REG_Z_THRX_CONFIG	0x6U
#define TMAG5170_REG_T_THRX_CONFIG	0x7U
#define TMAG5170_REG_CONV_STATUS	0x8U
#define TMAG5170_REG_X_CH_RESULT	0x9U
#define TMAG5170_REG_Y_CH_RESULT	0xAU
#define TMAG5170_REG_Z_CH_RESULT	0xBU
#define TMAG5170_REG_TEMP_RESULT	0xCU
#define TMAG5170_REG_AFE_STATUS		0xDU
#define TMAG5170_REG_SYS_STATUS		0xEU
#define TMAG5170_REG_TEST_CONFIG	0xFU
#define TMAG5170_REG_OSC_MONITOR	0x10U
#define TMAG5170_REG_MAG_GAIN_CONFIG	0x11U
#define TMAG5170_REG_MAG_OFFSET_CONFIG	0x12U
#define TMAG5170_REG_ANGLE_RESULT	0x13U
#define TMAG5170_REG_MAGNITUDE_RESULT	0x14U

#define TMAG5170_CONV_AVG_POS		12U
#define TMAG5170_CONV_AVG_MASK		(BIT_MASK(3U) << TMAG5170_CONV_AVG_POS)
#define TMAG5170_CONV_AVG_SET(value)	(((value) << TMAG5170_CONV_AVG_POS) &\
					TMAG5170_CONV_AVG_MASK)

#define TMAG5170_MAG_TEMPCO_POS		8U
#define TMAG5170_MAG_TEMPCO_MASK	(BIT_MASK(2U) << TMAG5170_MAG_TEMPCO_POS)
#define TMAG5170_MAG_TEMPCO_SET(value)	(((value) << TMAG5170_MAG_TEMPCO_POS) &\
					TMAG5170_MAG_TEMPCO_MASK)

#define TMAG5170_OPERATING_MODE_POS		4U
#define TMAG5170_OPERATING_MODE_MASK		(BIT_MASK(3U) << TMAG5170_OPERATING_MODE_POS)
#define TMAG5170_OPERATING_MODE_SET(value)	(((value) << TMAG5170_OPERATING_MODE_POS) &\
						TMAG5170_OPERATING_MODE_MASK)

#define TMAG5170_T_CH_EN_POS		3U
#define TMAG5170_T_CH_EN_MASK		(BIT_MASK(1U) << TMAG5170_T_CH_EN_POS)
#define TMAG5170_T_CH_EN_SET(value)	(((value) << TMAG5170_T_CH_EN_POS) &\
					TMAG5170_T_CH_EN_MASK)

#define TMAG5170_T_RATE_POS		2U
#define TMAG5170_T_RATE_MASK		(BIT_MASK(1U) << TMAG5170_T_RATE_POS)
#define TMAG5170_T_RATE_SET(value)	(((value) << TMAG5170_T_RATE_POS) &\
					TMAG5170_T_RATE_MASK)

#define TMAG5170_ANGLE_EN_POS		14U
#define TMAG5170_ANGLE_EN_MASK		(BIT_MASK(2U) << TMAG5170_ANGLE_EN_POS)
#define TMAG5170_ANGLE_EN_SET(value)	(((value) << TMAG5170_ANGLE_EN_POS) &\
					TMAG5170_ANGLE_EN_MASK)

#define TMAG5170_SLEEPTIME_POS	10U
#define TMAG5170_SLEEPTIME_MASK		(BIT_MASK(4U) << TMAG5170_SLEEPTIME_POS)
#define TMAG5170_SLEEPTIME_SET(value)	(((value) << TMAG5170_SLEEPTIME_POS) &\
					TMAG5170_SLEEPTIME_MASK)

#define TMAG5170_MAG_CH_EN_POS	6U
#define TMAG5170_MAG_CH_EN_MASK		(BIT_MASK(4U) << TMAG5170_MAG_CH_EN_POS)
#define TMAG5170_MAG_CH_EN_SET(value)	(((value) << TMAG5170_MAG_CH_EN_POS) &\
					TMAG5170_MAG_CH_EN_MASK)

#define TMAG5170_Z_RANGE_POS		4U
#define TMAG5170_Z_RANGE_MASK		(BIT_MASK(2U) << TMAG5170_Z_RANGE_POS)
#define TMAG5170_Z_RANGE_SET(value)	(((value) << TMAG5170_Z_RANGE_POS) &\
					TMAG5170_Z_RANGE_MASK)

#define TMAG5170_Y_RANGE_POS		2U
#define TMAG5170_Y_RANGE_MASK		(BIT_MASK(2U) << TMAG5170_Y_RANGE_POS)
#define TMAG5170_Y_RANGE_SET(value)	(((value) << TMAG5170_Y_RANGE_POS) &\
					TMAG5170_Y_RANGE_MASK)

#define TMAG5170_X_RANGE_POS		0U
#define TMAG5170_X_RANGE_MASK		(BIT_MASK(2U) << TMAG5170_X_RANGE_POS)
#define TMAG5170_X_RANGE_SET(value)	(((value) << TMAG5170_X_RANGE_POS) &\
					TMAG5170_X_RANGE_MASK)

#define TMAG5170_RSLT_ALRT_POS		8U
#define TMAG5170_RSLT_ALRT_MASK		(BIT_MASK(1U) << TMAG5170_RSLT_ALRT_POS)
#define TMAG5170_RSLT_ALRT_SET(value)	(((value) << TMAG5170_RSLT_ALRT_POS) &\
					TMAG5170_RSLT_ALRT_MASK)

#define TMAG5170_VER_POS	4U
#define TMAG5170_VER_MASK	(BIT_MASK(2U) << TMAG5170_VER_POS)
#define TMAG5170_VER_GET(value)	(((value) & TMAG5170_VER_MASK) >> TMAG5170_VER_POS)

#define TMAG5170_A1_REV				0x0U
#define TMAG5170_A2_REV				0x1U

#define TMAG5170_MAX_RANGE_50MT_IDX		0x0U
#define TMAG5170_MAX_RANGE_25MT_IDX		0x1U
#define TMAG5170_MAX_RANGE_100MT_IDX		0x2U
#define TMAG5170_MAX_RANGE_EXTEND_FACTOR	0x3U

#define TMAG5170_CONFIGURATION_MODE		0x0U
#define TMAG5170_STAND_BY_MODE			0x1U
#define TMAG5170_ACTIVE_TRIGGER_MODE		0x3U
#define TMAG5170_SLEEP_MODE			0x5U
#define TMAG5170_DEEP_SLEEP_MODE		0x6U

#define TMAG5170_MT_TO_GAUSS_RATIO		10U
#define TMAG5170_T_SENS_T0			25U
#define TMAG5170_T_ADC_T0			17522U
#define TMAG5170_T_ADC_RES			60U

#define TMAG5170_CMD_TRIGGER_CONVERSION		BIT(0U)

#define TMAG5170_CRC_SEED			0xFU
#define TMAG5170_CRC_POLY			0x3U
#define TMAG5170_SPI_BUFFER_LEN			4U
#define TMAG5170_SET_CRC(buf, crc)		((uint8_t *)(buf))[3] |= (crc & 0x0F)
#define TMAG5170_ZERO_CRC(buf)			((uint8_t *)(buf))[3] &= 0xF0
#define TMAG5170_GET_CRC(buf)			((uint8_t *)(buf))[3] & 0x0F

LOG_MODULE_REGISTER(TMAG5170, CONFIG_SENSOR_LOG_LEVEL);

static int tmag5170_transmit_raw(const struct tmag5170_dev_config *config,
				 uint8_t *buffer_tx,
				 uint8_t *buffer_rx)
{
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = TMAG5170_SPI_BUFFER_LEN,
	};

	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	const struct spi_buf rx_buf = {
		.buf = buffer_rx,
		.len = TMAG5170_SPI_BUFFER_LEN,
	};

	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	int ret = spi_transceive_dt(&config->bus, &tx, &rx);

	return ret;
}

static int tmag5170_transmit(const struct device *dev, uint8_t *buffer_tx, uint8_t *buffer_rx)
{
#if defined(CONFIG_TMAG5170_CRC)
	TMAG5170_ZERO_CRC(buffer_tx);
	uint8_t crc = crc4_ti(TMAG5170_CRC_SEED, buffer_tx, TMAG5170_SPI_BUFFER_LEN);

	TMAG5170_SET_CRC(buffer_tx, crc);
#endif
	int ret = tmag5170_transmit_raw(dev->config, buffer_tx, buffer_rx);
#if defined(CONFIG_TMAG5170_CRC)
	if (buffer_rx != NULL && ret == 0) {
		uint8_t read_crc = TMAG5170_GET_CRC(buffer_rx);

		TMAG5170_ZERO_CRC(buffer_rx);
		crc = crc4_ti(TMAG5170_CRC_SEED, buffer_rx, TMAG5170_SPI_BUFFER_LEN);
		if (read_crc != crc) {
			return -EIO;
		}
	}
#endif

	return ret;
}

static int tmag5170_write_register(const struct device *dev, uint32_t reg, uint16_t data)
{
	uint8_t buffer_tx[4] = { reg, (data >> 8) & 0xFF, data & 0xFF, 0x00U };

	return tmag5170_transmit(dev, buffer_tx, NULL);
}

static int tmag5170_read_register(const struct device *dev,
				  uint32_t reg,
				  uint16_t *output,
				  uint8_t cmd)
{
	uint8_t buffer_tx[4] = { BIT(7) | reg, 0x00U, 0x00U, (cmd & BIT_MASK(4U)) << 4U };
	uint8_t buffer_rx[4] = { 0x00U };

	int ret = tmag5170_transmit(dev, buffer_tx, buffer_rx);

	*output = (buffer_rx[1] << 8) | buffer_rx[2];

	return ret;
}

static int tmag5170_convert_magn_reading_to_gauss(struct sensor_value *output,
						  uint16_t chan_reading,
						  uint8_t chan_range,
						  uint8_t chip_revision)
{
	uint16_t max_range_mt = 0U;

	if (chan_range == TMAG5170_MAX_RANGE_50MT_IDX) {
		max_range_mt = 50U;
	} else if (chan_range == TMAG5170_MAX_RANGE_25MT_IDX) {
		max_range_mt = 25U;
	} else if (chan_range == TMAG5170_MAX_RANGE_100MT_IDX) {
		max_range_mt = 100U;
	} else {
		return -ENOTSUP;
	}

	if (chip_revision == TMAG5170_A2_REV) {
		max_range_mt *= TMAG5170_MAX_RANGE_EXTEND_FACTOR;
	}

	max_range_mt *= 2U;

	/* The sensor returns data in mT, we need to convert it to Gauss */
	uint32_t max_range_gauss = max_range_mt * TMAG5170_MT_TO_GAUSS_RATIO;

	/* Convert from 2's complementary system */
	int64_t result = chan_reading - ((chan_reading & 0x8000) << 1);

	result *= max_range_gauss;

	/* Scale to increase accuracy */
	result *= 100000;

	/* Divide as it is shown in datasheet */
	result /= 65536;

	/* Remove scale from the final result */
	output->val1 = result / 100000;
	output->val2 = result % 100000;

	return 0;
}

static void tmag5170_convert_temp_reading_to_celsius(struct sensor_value *output,
						     uint16_t chan_reading)
{
	int32_t result = chan_reading - TMAG5170_T_ADC_T0;

	result = (TMAG5170_T_SENS_T0 * 100000) + (100000 * result / (int32_t)TMAG5170_T_ADC_RES);

	output->val1 = result / 100000;
	output->val2 = (result % 100000) * 10;
}

static void tmag5170_covert_angle_reading_to_degrees(struct sensor_value *output,
						     uint16_t chan_reading)
{
	/* 12 MSBs store the integer part of the result,
	 * 4 LSBs store the fractional part of the result
	 */
	output->val1 = chan_reading >> 4;
	output->val2 = ((chan_reading & 0xF) * 1000000) / 16;
}

static int tmag5170_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	const struct tmag5170_dev_config *cfg = dev->config;
	struct tmag5170_data *drv_data = dev->data;
	int ret = 0;

	if (cfg->operating_mode == TMAG5170_STAND_BY_MODE ||
	    cfg->operating_mode == TMAG5170_ACTIVE_TRIGGER_MODE) {
		uint16_t read_status;

		tmag5170_read_register(dev,
				       TMAG5170_REG_SYS_STATUS,
				       &read_status,
				       TMAG5170_CMD_TRIGGER_CONVERSION);

		/* Wait for the measurement to be ready.
		 * The waiting time will vary depending on the configuration
		 */
		k_sleep(K_MSEC(5U));
	}

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		ret = tmag5170_read_register(dev, TMAG5170_REG_X_CH_RESULT, &drv_data->x, 0U);
		break;
	case SENSOR_CHAN_MAGN_Y:
		ret = tmag5170_read_register(dev, TMAG5170_REG_Y_CH_RESULT, &drv_data->y, 0U);
		break;
	case SENSOR_CHAN_MAGN_Z:
		ret = tmag5170_read_register(dev, TMAG5170_REG_Z_CH_RESULT, &drv_data->z, 0U);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		ret = tmag5170_read_register(dev, TMAG5170_REG_X_CH_RESULT, &drv_data->x, 0U);

		if (ret == 0) {
			ret = tmag5170_read_register(dev,
						     TMAG5170_REG_Y_CH_RESULT,
						     &drv_data->y,
						     0U);
		}
		if (ret == 0) {
			ret = tmag5170_read_register(dev,
						     TMAG5170_REG_Z_CH_RESULT,
						     &drv_data->z,
						     0U);
		}
		break;
	case SENSOR_CHAN_ROTATION:
		ret = tmag5170_read_register(dev,
					     TMAG5170_REG_ANGLE_RESULT,
					     &drv_data->angle,
					     0U);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		ret = tmag5170_read_register(dev,
					TMAG5170_REG_TEMP_RESULT,
					&drv_data->temperature,
					0U);
		break;
	case SENSOR_CHAN_ALL:
		ret = tmag5170_read_register(dev,
					     TMAG5170_REG_TEMP_RESULT,
					     &drv_data->temperature,
					     0U);

		if (ret == 0) {
			ret = tmag5170_read_register(dev,
						     TMAG5170_REG_ANGLE_RESULT,
						     &drv_data->angle,
						     0U);
		}

		if (ret == 0) {
			ret = tmag5170_read_register(dev,
						     TMAG5170_REG_X_CH_RESULT,
						     &drv_data->x,
						     0U);
		}

		if (ret == 0) {
			ret = tmag5170_read_register(dev,
						     TMAG5170_REG_Y_CH_RESULT,
						     &drv_data->y,
						     0U);
		}

		if (ret == 0) {
			ret = tmag5170_read_register(dev,
						     TMAG5170_REG_Z_CH_RESULT,
						     &drv_data->z,
						     0U);
		}

		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int tmag5170_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct tmag5170_dev_config *cfg = dev->config;
	struct tmag5170_data *drv_data = dev->data;
	int ret = 0;

	switch (chan) {
	case SENSOR_CHAN_MAGN_XYZ:
		ret = tmag5170_convert_magn_reading_to_gauss(val,
							     drv_data->x,
							     cfg->x_range,
							     drv_data->chip_revision);

		if (ret == 0) {
			ret = tmag5170_convert_magn_reading_to_gauss(val + 1,
								     drv_data->y,
								     cfg->y_range,
								     drv_data->chip_revision);
		}

		if (ret == 0) {
			ret = tmag5170_convert_magn_reading_to_gauss(val + 2,
								     drv_data->z,
								     cfg->z_range,
								     drv_data->chip_revision);
		}
		break;
	case SENSOR_CHAN_MAGN_X:
		ret = tmag5170_convert_magn_reading_to_gauss(val,
							     drv_data->x,
							     cfg->x_range,
							     drv_data->chip_revision);
		break;
	case SENSOR_CHAN_MAGN_Y:
		ret = tmag5170_convert_magn_reading_to_gauss(val,
							     drv_data->y,
							     cfg->y_range,
							     drv_data->chip_revision);
		break;
	case SENSOR_CHAN_MAGN_Z:
		ret = tmag5170_convert_magn_reading_to_gauss(val,
							     drv_data->z,
							     cfg->z_range,
							     drv_data->chip_revision);
		break;
	case SENSOR_CHAN_ROTATION:
		tmag5170_covert_angle_reading_to_degrees(val, drv_data->angle);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		tmag5170_convert_temp_reading_to_celsius(val, drv_data->temperature);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int tmag5170_init_registers(const struct device *dev)
{
	const struct tmag5170_dev_config *cfg = dev->config;
	struct tmag5170_data *drv_data = dev->data;
	uint16_t test_cfg_reg = 0U;
	int ret = 0;

#if !defined(CONFIG_TMAG5170_CRC)
	const uint8_t disable_crc_packet[4] = { 0x0FU, 0x0U, 0x04U, 0x07U };

	ret = tmag5170_transmit_raw(cfg, disable_crc_packet, NULL);
#endif
	if (ret == 0) {
		ret = tmag5170_read_register(dev, TMAG5170_REG_TEST_CONFIG, &test_cfg_reg, 0U);
	}

	if (ret == 0) {
		drv_data->chip_revision = TMAG5170_VER_GET(test_cfg_reg);

		ret = tmag5170_write_register(dev,
				TMAG5170_REG_SENSOR_CONFIG,
				TMAG5170_ANGLE_EN_SET(cfg->angle_measurement) |
				TMAG5170_SLEEPTIME_SET(cfg->sleep_time) |
				TMAG5170_MAG_CH_EN_SET(cfg->magnetic_channels) |
				TMAG5170_Z_RANGE_SET(cfg->z_range) |
				TMAG5170_Y_RANGE_SET(cfg->y_range) |
				TMAG5170_X_RANGE_SET(cfg->x_range));
	}

#if defined(CONFIG_TMAG5170_TRIGGER)
	if (ret == 0) {
		ret = tmag5170_write_register(dev,
				TMAG5170_REG_ALERT_CONFIG,
				TMAG5170_RSLT_ALRT_SET(1U));
	}
#endif
	if (ret == 0) {
		ret = tmag5170_write_register(dev,
				TMAG5170_REG_DEVICE_CONFIG,
				TMAG5170_OPERATING_MODE_SET(cfg->operating_mode) |
				TMAG5170_CONV_AVG_SET(cfg->oversampling) |
				TMAG5170_MAG_TEMPCO_SET(cfg->magnet_type) |
				TMAG5170_T_CH_EN_SET(cfg->tempeature_measurement) |
				TMAG5170_T_RATE_SET(cfg->disable_temperature_oversampling));
	}

	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int tmag5170_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	int ret_val = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		tmag5170_write_register(dev,
					TMAG5170_REG_DEVICE_CONFIG,
					TMAG5170_OPERATING_MODE_SET(TMAG5170_CONFIGURATION_MODE));
		/* As per datasheet, waking up from deep-sleep can take up to 500us */
		k_sleep(K_USEC(500));
		ret_val = tmag5170_init_registers(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret_val = tmag5170_write_register(dev,
					TMAG5170_REG_DEVICE_CONFIG,
					TMAG5170_OPERATING_MODE_SET(TMAG5170_DEEP_SLEEP_MODE));
		break;
	default:
		ret_val = -ENOTSUP;
	}

	return ret_val;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(sensor, tmag5170_driver_api) = {
	.sample_fetch = tmag5170_sample_fetch,
	.channel_get = tmag5170_channel_get,
#if defined(CONFIG_TMAG5170_TRIGGER)
	.trigger_set = tmag5170_trigger_set
#endif
};

static int tmag5170_init(const struct device *dev)
{
	const struct tmag5170_dev_config *cfg = dev->config;
	int ret = 0;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI dev %s not ready", cfg->bus.bus->name);
		return -ENODEV;
	}

	ret = tmag5170_init_registers(dev);
	if (ret != 0) {
		return ret;
	}

#if defined(CONFIG_TMAG5170_TRIGGER)
	if (cfg->int_gpio.port) {
		ret = tmag5170_trigger_init(dev);
	}
#endif

	return ret;
}

#define DEFINE_TMAG5170(_num)								   \
	static struct tmag5170_data tmag5170_data_##_num;				   \
	static const struct tmag5170_dev_config tmag5170_config_##_num = {		   \
		.bus = SPI_DT_SPEC_INST_GET(_num,					   \
					   SPI_OP_MODE_MASTER |				   \
					   SPI_TRANSFER_MSB |				   \
					   SPI_WORD_SET(32),				   \
					   0),						   \
		.magnetic_channels = DT_INST_ENUM_IDX(_num, magnetic_channels),		   \
		.x_range = DT_INST_ENUM_IDX(_num, x_range),				   \
		.y_range = DT_INST_ENUM_IDX(_num, y_range),				   \
		.z_range = DT_INST_ENUM_IDX(_num, z_range),				   \
		.operating_mode = DT_INST_PROP(_num, operating_mode),			   \
		.oversampling = DT_INST_ENUM_IDX(_num, oversampling),			   \
		.tempeature_measurement = DT_INST_PROP(_num, enable_temperature_channel),  \
		.magnet_type = DT_INST_ENUM_IDX(_num, magnet_type),			   \
		.angle_measurement = DT_INST_ENUM_IDX(_num, angle_measurement),		   \
		.disable_temperature_oversampling = DT_INST_PROP(_num,			   \
							disable_temperature_oversampling), \
		.sleep_time = DT_INST_ENUM_IDX(_num, sleep_time),			   \
		IF_ENABLED(CONFIG_TMAG5170_TRIGGER,					   \
			(.int_gpio = GPIO_DT_SPEC_INST_GET_OR(_num, int_gpios, { 0 }),))   \
	};										   \
	PM_DEVICE_DT_INST_DEFINE(_num, tmag5170_pm_action);				   \
											   \
	SENSOR_DEVICE_DT_INST_DEFINE(_num,						   \
				tmag5170_init,						   \
				PM_DEVICE_DT_INST_GET(_num),				   \
				&tmag5170_data_##_num,					   \
				&tmag5170_config_##_num,				   \
				POST_KERNEL,						   \
				CONFIG_SENSOR_INIT_PRIORITY,				   \
				&tmag5170_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_TMAG5170)
