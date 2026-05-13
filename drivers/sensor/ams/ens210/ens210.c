/*
 * Copyright (c) 2018 Alexander Wachter.
 * Copyright (c) 2026 Alif Semiconductor.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_ens210

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>

#include "ens210.h"

LOG_MODULE_REGISTER(ENS210, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_ENS210_CRC_CHECK
static uint32_t ens210_crc7(uint32_t bitstream)
{
	uint32_t polynomial = (ENS210_CRC7_POLY << (ENS210_CRC7_DATA_WIDTH - 1));
	uint32_t bit = ENS210_CRC7_DATA_MSB << ENS210_CRC7_WIDTH;
	uint32_t val = (bitstream << ENS210_CRC7_WIDTH) | ENS210_CRC7_IVEC;

	while (bit & (ENS210_CRC7_DATA_MASK << ENS210_CRC7_WIDTH)) {
		if (bit & val) {
			val ^= polynomial;
		}

		bit >>= 1;
		polynomial >>= 1;
	}

	return val;
}
#endif /* CONFIG_ENS210_CRC_CHECK */

int ens210_check_value(const struct ens210_value_data *data)
{
	uint32_t valid;

	if (!data->valid) {
		return -EIO;
	}

#ifdef CONFIG_ENS210_CRC_CHECK
	valid = data->val | (data->valid << (sizeof(data->val) * 8));
	if (ens210_crc7(valid) != data->crc7) {
		return -EIO;
	}
#else
	ARG_UNUSED(valid);
#endif

	return 0;
}

void ens210_convert(const struct ens210_value_data *temp,
			const struct ens210_value_data *humidity,
			struct ens210_reading *reading)
{
	uint16_t temp_val = sys_le16_to_cpu(temp->val);
	uint16_t hum_val  = sys_le16_to_cpu(humidity->val);

	reading->temperature = (((int64_t)temp_val * 1000000) / 64) - 273150000;
	reading->humidity    = (((uint64_t)hum_val * 1000000) / 512);
}

#if defined(CONFIG_ENS210_TEMPERATURE_SINGLE) \
		|| defined(CONFIG_ENS210_HUMIDITY_SINGLE)
static int ens210_measure(const struct device *dev, enum sensor_channel chan)
{
	const struct ens210_config *config = dev->config;
	uint8_t buf;
	int ret;
	const struct ens210_sens_start sense_start = {
		.t_start = ENS210_T_START && (chan == SENSOR_CHAN_ALL
				|| chan == SENSOR_CHAN_AMBIENT_TEMP),
		.h_start = ENS210_H_START && (chan == SENSOR_CHAN_ALL
				|| chan == SENSOR_CHAN_HUMIDITY)
	};

	/* Start measuring */
	ret = i2c_reg_write_byte_dt(&config->i2c, ENS210_REG_SENS_START, *(uint8_t *)&sense_start);

	if (ret < 0) {
		LOG_ERR("Failed to set SENS_START to 0x%x",
				*(uint8_t *)&sense_start);
		return -EIO;
	}

	/* Wait for measurement to be completed */
	do {
		k_sleep(K_MSEC(2));
		ret = i2c_reg_read_byte_dt(&config->i2c, ENS210_REG_SENS_START, &buf);

		if (ret < 0) {
			LOG_ERR("Failed to read SENS_STAT");
		}
	} while (buf & *(uint8_t *)&sense_start);

	return ret;
}
#endif /* Single shot mode */

static int ens210_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct ens210_data *drv_data = dev->data;
	const struct ens210_config *config = dev->config;
	struct ens210_value_data data[2];
	int ret, cnt;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL
			|| chan == SENSOR_CHAN_AMBIENT_TEMP
			|| chan == SENSOR_CHAN_HUMIDITY);

#if defined(CONFIG_ENS210_TEMPERATURE_SINGLE) \
		|| defined(CONFIG_ENS210_HUMIDITY_SINGLE)
	ret = ens210_measure(dev, chan);
	if (ret < 0) {
		LOG_ERR("Failed to measure");
		return ret;
	}
#endif /* Single shot mode */

	for (cnt = 0; cnt <= CONFIG_ENS210_MAX_READ_RETRIES; cnt++) {
		ret = i2c_burst_read_dt(&config->i2c, ENS210_REG_T_VAL, (uint8_t *)&data,
					sizeof(data));
		if (ret < 0) {
			LOG_ERR("Failed to read data");
			continue;
		}

		/* Get temperature value */
		if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP) {

			if (ens210_check_value(&data[0]) < 0) {
				continue;
			}

			drv_data->temp = data[0];
		}

		/* Get humidity value */
		if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_HUMIDITY) {

			if (ens210_check_value(&data[1]) < 0) {
				continue;
			}

			drv_data->humidity = data[1];
		}

		return 0;
	}

	return -EIO;
}

static int ens210_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct ens210_data *drv_data = dev->data;
	struct ens210_reading reading;

	ens210_convert(&drv_data->temp, &drv_data->humidity, &reading);

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		val->val1 = reading.temperature / 1000000;
		val->val2 = reading.temperature % 1000000;
		break;
	case  SENSOR_CHAN_HUMIDITY:
		val->val1 = reading.humidity / 1000000;
		val->val2 = reading.humidity % 1000000;

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ens210_sys_reset(const struct device *dev)
{
	const struct ens210_config *config = dev->config;

	const struct ens210_sys_ctrl sys_ctrl = {
			.low_power = 0,
			.reset = 1
	};
	int ret;

	ret = i2c_reg_write_byte_dt(&config->i2c, ENS210_REG_SYS_CTRL, *(uint8_t *)&sys_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to set SYS_CTRL to 0x%x", *(uint8_t *)&sys_ctrl);
	}
	return ret;
}

static int ens210_sys_enable(const struct device *dev, uint8_t low_power)
{
	const struct ens210_config *config = dev->config;

	const struct ens210_sys_ctrl sys_ctrl = {
			.low_power = low_power,
			.reset = 0
	};
	int ret;

	ret = i2c_reg_write_byte_dt(&config->i2c, ENS210_REG_SYS_CTRL, *(uint8_t *)&sys_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to set SYS_CTRL to 0x%x", *(uint8_t *)&sys_ctrl);
	}
	return ret;
}

static int ens210_wait_boot(const struct device *dev)
{
	const struct ens210_config *config = dev->config;

	int cnt;
	int ret;
	struct ens210_sys_stat sys_stat;

	for (cnt = 0; cnt <= CONFIG_ENS210_MAX_STAT_RETRIES; cnt++) {
		ret = i2c_reg_read_byte_dt(&config->i2c, ENS210_REG_SYS_STAT, (uint8_t *)&sys_stat);

		if (ret < 0) {
			k_sleep(K_MSEC(1));
			continue;
		}

		if (sys_stat.sys_active) {
			return 0;
		}

		if (cnt == 0) {
			ens210_sys_reset(dev);
		}

		ens210_sys_enable(dev, 0);

		k_sleep(K_MSEC(2));
	}

	if (ret < 0) {
		LOG_ERR("Failed to read SYS_STATE");
	}


	LOG_ERR("Sensor is not in active state");
	return -EIO;
}

static DEVICE_API(sensor, en210_driver_api) = {
	.sample_fetch = ens210_sample_fetch,
	.channel_get = ens210_channel_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = ens210_submit,
	.get_decoder = ens210_get_decoder,
#endif
};

static int ens210_init(const struct device *dev)
{
	const struct ens210_config *config = dev->config;
	const struct ens210_sens_run sense_run = {
		.t_run = ENS210_T_RUN,
		.h_run = ENS210_H_RUN
	};

#if defined(CONFIG_ENS210_TEMPERATURE_CONTINUOUS) \
	|| defined(CONFIG_ENS210_HUMIDITY_CONTINUOUS)
	const struct ens210_sens_start sense_start = {
		.t_start = ENS210_T_RUN,
		.h_start = ENS210_H_RUN
	};
#endif

	int ret;
	uint16_t part_id;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR_DEVICE_NOT_READY(config->i2c.bus);
		return -ENODEV;
	}

	/* Wait until the device is ready. */
	ret = ens210_wait_boot(dev);
	if (ret < 0) {
		return -EIO;
	}

	/* Check Hardware ID. This is only possible after device is ready
	 * and active
	 */
	ret = i2c_burst_read_dt(&config->i2c, ENS210_REG_PART_ID, (uint8_t *)&part_id,
				sizeof(part_id));
	if (ret < 0) {
		LOG_ERR("Failed to read Part ID register");
		return -EIO;
	}

	if (part_id != ENS210_PART_ID) {
		LOG_ERR("Part ID does not match. Want 0x%x, got 0x%x",
			   ENS210_PART_ID, part_id);
		return -EIO;
	}

	/* Enable low power mode */
	if ((ENS210_T_RUN | ENS210_H_RUN) == 0) {
		ens210_sys_enable(dev, 1);
	}

	/* Set measurement mode*/
	ret = i2c_reg_write_byte_dt(&config->i2c, ENS210_REG_SENS_RUN, *(uint8_t *)&sense_run);
	if (ret < 0) {
		LOG_ERR("Failed to set SENS_RUN to 0x%x",
			   *(uint8_t *)&sense_run);
		return -EIO;
	}

#if defined(CONFIG_ENS210_TEMPERATURE_CONTINUOUS) \
	|| defined(CONFIG_ENS210_HUMIDITY_CONTINUOUS)
	/* Start measuring */
	ret = i2c_reg_write_byte_dt(&config->i2c, ENS210_REG_SENS_START, *(uint8_t *)&sense_start);
	if (ret < 0) {
		LOG_ERR("Failed to set SENS_START to 0x%x",
			   *(uint8_t *)&sense_start);
		return -EIO;
	}
#endif

#ifdef CONFIG_SENSOR_ASYNC_API
	struct ens210_data *drv_data = dev->data;

	drv_data->dev = dev;
	mpsc_init(&drv_data->io_q);

#endif /* CONFIG_SENSOR_ASYNC_API */
	return 0;
}

/* RTIO context definition - one per device instance */
#ifdef CONFIG_SENSOR_ASYNC_API
#define ENS210_RTIO_DEFINE(inst)       RTIO_DEFINE(ens210_rtio_ctx_##inst, 8, 8)
#define ENS210_I2C_IODEV_DEFINE(inst)  I2C_DT_IODEV_DEFINE(ens210_iodev_##inst, DT_DRV_INST(inst))
#define ENS210_RTIO_DATA_INIT(inst)    \
		.r = &ens210_rtio_ctx_##inst,  \
		.bus_iodev = &ens210_iodev_##inst,
#else
#define ENS210_RTIO_DEFINE(inst)
#define ENS210_I2C_IODEV_DEFINE(inst)
#define ENS210_RTIO_DATA_INIT(inst)
#endif

#define ENS210_DEFINE(inst)								\
	ENS210_RTIO_DEFINE(inst);							\
	ENS210_I2C_IODEV_DEFINE(inst);							\
											\
	static struct ens210_data ens210_data_##inst = {				\
		ENS210_RTIO_DATA_INIT(inst)						\
	};										\
											\
	static const struct ens210_config ens210_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
	};										\
											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ens210_init, NULL,				\
			  &ens210_data_##inst, &ens210_config_##inst, POST_KERNEL,	\
			  CONFIG_SENSOR_INIT_PRIORITY, &en210_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(ENS210_DEFINE)
