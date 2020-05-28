/*
 * Copyright (c) 2018 Alexander Wachter.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_ens210

#include <device.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>
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

#if defined(CONFIG_ENS210_TEMPERATURE_SINGLE) \
		|| defined(CONFIG_ENS210_HUMIDITY_SINGLE)
static int ens210_measure(struct device *i2c_dev, enum sensor_channel chan)
{
	uint8_t buf;
	int ret;
	const struct ens210_sens_start sense_start = {
		.t_start = ENS210_T_START && (chan == SENSOR_CHAN_ALL
				|| chan == SENSOR_CHAN_AMBIENT_TEMP),
		.h_start = ENS210_H_START && (chan == SENSOR_CHAN_ALL
				|| chan == SENSOR_CHAN_HUMIDITY)
	};

	/* Start measuring */
	ret = i2c_reg_write_byte(i2c_dev,
			DT_INST_REG_ADDR(0),
			ENS210_REG_SENS_START, *(uint8_t *)&sense_start);

	if (ret < 0) {
		LOG_ERR("Failed to set SENS_START to 0x%x",
				*(uint8_t *)&sense_start);
		return -EIO;
	}

	/* Wait for measurement to be completed */
	do {
		k_sleep(K_MSEC(2));
		ret = i2c_reg_read_byte(i2c_dev,
				DT_INST_REG_ADDR(0),
				ENS210_REG_SENS_START, &buf);

		if (ret < 0) {
			LOG_ERR("Failed to read SENS_STAT");
		}
	} while (buf & *(uint8_t *)&sense_start);

	return ret;
}
#endif /* Single shot mode */

static int ens210_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct ens210_data *drv_data = dev->data;
	struct ens210_value_data data[2];
	int ret, cnt;

#ifdef CONFIG_ENS210_CRC_CHECK
	uint32_t temp_valid, humidity_valid;
#endif /* CONFIG_ENS210_CRC_CHECK */

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL
			|| chan == SENSOR_CHAN_AMBIENT_TEMP
			|| chan == SENSOR_CHAN_HUMIDITY);

#if defined(CONFIG_ENS210_TEMPERATURE_SINGLE) \
		|| defined(CONFIG_ENS210_HUMIDITY_SINGLE)
	ret = ens210_measure(drv_data->i2c, chan);
	if (ret < 0) {
		LOG_ERR("Failed to measure");
		return ret;
	}
#endif /* Single shot mode */

	for (cnt = 0; cnt <= CONFIG_ENS210_MAX_READ_RETRIES; cnt++) {
		ret =  i2c_burst_read(drv_data->i2c, DT_INST_REG_ADDR(0),
				ENS210_REG_T_VAL, (uint8_t *)&data, sizeof(data));
		if (ret < 0) {
			LOG_ERR("Failed to read data");
			continue;
		}

		/* Get temperature value */
		if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP) {

			if (!data[0].valid) {
				LOG_WRN("Temperature not valid");
				continue;
			}

#ifdef CONFIG_ENS210_CRC_CHECK
			temp_valid = data[0].val |
					(data[0].valid << (sizeof(data[0].val) * 8));

			if (ens210_crc7(temp_valid) != data[0].crc7) {
				LOG_WRN("Temperature CRC error");
				continue;
			}
#endif /* CONFIG_ENS210_CRC_CHECK */

			drv_data->temp = data[0];
		}

		/* Get humidity value */
		if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_HUMIDITY) {

			if (!data[1].valid) {
				LOG_WRN("Humidity not valid");
				continue;
			}

#ifdef CONFIG_ENS210_CRC_CHECK
			humidity_valid = data[1].val |
					  (data[1].valid << (sizeof(data[1].val) * 8));

			if (ens210_crc7(humidity_valid) != data[1].crc7) {
				LOG_WRN("Humidity CRC error");
				continue;
			}
#endif /* CONFIG_ENS210_CRC_CHECK */

			drv_data->humidity = data[1];
		}

		return 0;
	}

	return -EIO;
}

static int ens210_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ens210_data *drv_data = dev->data;
	int32_t temp_frac;
	int32_t humidity_frac;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Temperature is in 1/64 Kelvin. Subtract 273.15 for Celsius */
		temp_frac = sys_le16_to_cpu(drv_data->temp.val) * (1000000 / 64);
		temp_frac -= 273150000;

		val->val1 = temp_frac / 1000000;
		val->val2 = temp_frac % 1000000;
		break;
	case  SENSOR_CHAN_HUMIDITY:
		humidity_frac = sys_le16_to_cpu(drv_data->humidity.val) *
				(1000000 / 512);
		val->val1 = humidity_frac / 1000000;
		val->val2 = humidity_frac % 1000000;

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ens210_sys_reset(struct device *i2c_dev)
{
	const struct ens210_sys_ctrl sys_ctrl = {
			.low_power = 0,
			.reset = 1
	};
	int ret;

	ret = i2c_reg_write_byte(i2c_dev, DT_INST_REG_ADDR(0),
				 ENS210_REG_SYS_CTRL, *(uint8_t *)&sys_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to set SYS_CTRL to 0x%x", *(uint8_t *)&sys_ctrl);
	}
	return ret;
}

static int ens210_sys_enable(struct device *i2c_dev, uint8_t low_power)
{
	const struct ens210_sys_ctrl sys_ctrl = {
			.low_power = low_power,
			.reset = 0
	};
	int ret;

	ret = i2c_reg_write_byte(i2c_dev, DT_INST_REG_ADDR(0),
				 ENS210_REG_SYS_CTRL, *(uint8_t *)&sys_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to set SYS_CTRL to 0x%x", *(uint8_t *)&sys_ctrl);
	}
	return ret;
}

static int ens210_wait_boot(struct device *i2c_dev)
{
	int cnt;
	int ret;
	struct ens210_sys_stat sys_stat;

	for (cnt = 0; cnt <= CONFIG_ENS210_MAX_STAT_RETRIES; cnt++) {
		ret =  i2c_reg_read_byte(i2c_dev, DT_INST_REG_ADDR(0),
					 ENS210_REG_SYS_STAT,
					 (uint8_t *)&sys_stat);

		if (ret < 0) {
			k_sleep(K_MSEC(1));
			continue;
		}

		if (sys_stat.sys_active) {
			return 0;
		}

		if (cnt == 0) {
			ens210_sys_reset(i2c_dev);
		}

		ens210_sys_enable(i2c_dev, 0);

		k_sleep(K_MSEC(2));
	}

	if (ret < 0) {
		LOG_ERR("Failed to read SYS_STATE");
	}


	LOG_ERR("Sensor is not in active state");
	return -EIO;
}

static const struct sensor_driver_api en210_driver_api = {
	.sample_fetch = ens210_sample_fetch,
	.channel_get = ens210_channel_get,
};

static int ens210_init(struct device *dev)
{
	struct ens210_data *drv_data = dev->data;
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

	drv_data->i2c = device_get_binding(DT_INST_BUS_LABEL(0));
	if (drv_data->i2c == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			    DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	/* Wait until the device is ready. */
	ret = ens210_wait_boot(drv_data->i2c);
	if (ret < 0) {
		return -EIO;
	}

	/* Check Hardware ID. This is only possible after device is ready
	 * and active
	 */
	ret =  i2c_burst_read(drv_data->i2c, DT_INST_REG_ADDR(0),
			      ENS210_REG_PART_ID, (uint8_t *)&part_id,
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
		ens210_sys_enable(drv_data->i2c, 1);
	}

	/* Set measurement mode*/
	ret = i2c_reg_write_byte(drv_data->i2c, DT_INST_REG_ADDR(0),
				 ENS210_REG_SENS_RUN, *(uint8_t *)&sense_run);
	if (ret < 0) {
		LOG_ERR("Failed to set SENS_RUN to 0x%x",
			    *(uint8_t *)&sense_run);
		return -EIO;
	}

#if defined(CONFIG_ENS210_TEMPERATURE_CONTINUOUS) \
	|| defined(CONFIG_ENS210_HUMIDITY_CONTINUOUS)
	/* Start measuring */
	ret = i2c_reg_write_byte(drv_data->i2c, DT_INST_REG_ADDR(0),
				 ENS210_REG_SENS_START, *(uint8_t *)&sense_start);
	if (ret < 0) {
		LOG_ERR("Failed to set SENS_START to 0x%x",
			    *(uint8_t *)&sense_start);
		return -EIO;
	}
#endif
	return 0;
}

static struct ens210_data ens210_driver;

DEVICE_AND_API_INIT(ens210, DT_INST_LABEL(0), ens210_init, &ens210_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &en210_driver_api);
