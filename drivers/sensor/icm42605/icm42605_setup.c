/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/byteorder.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "icm42605.h"
#include "icm42605_reg.h"
#include "icm42605_spi.h"

LOG_MODULE_DECLARE(ICM42605, CONFIG_SENSOR_LOG_LEVEL);

int icm42605_set_fs(const struct device *dev, uint16_t a_sf, uint16_t g_sf)
{
	uint8_t databuf;
	int result;

	result = inv_spi_read(REG_ACCEL_CONFIG0, &databuf, 1);
	if (result) {
		return result;
	}
	databuf &= ~BIT_ACCEL_FSR;

	databuf |= a_sf;

	result = inv_spi_single_write(REG_ACCEL_CONFIG0, &databuf);

	result = inv_spi_read(REG_GYRO_CONFIG0, &databuf, 1);

	if (result) {
		return result;
	}

	databuf &= ~BIT_GYRO_FSR;
	databuf |= g_sf;

	result = inv_spi_single_write(REG_GYRO_CONFIG0, &databuf);

	if (result) {
		return result;
	}

	return 0;
}

int icm42605_set_odr(const struct device *dev, uint16_t a_rate, uint16_t g_rate)
{
	uint8_t databuf;
	int result;

	if (a_rate > 8000 || g_rate > 8000 ||
	    a_rate < 1 || g_rate < 12) {
		LOG_ERR("Not supported frequency");
		return -ENOTSUP;
	}

	result = inv_spi_read(REG_ACCEL_CONFIG0, &databuf, 1);

	if (result) {
		return result;
	}

	databuf &= ~BIT_ACCEL_ODR;

	if (a_rate > 4000) {
		databuf |= BIT_ACCEL_ODR_8000;
	} else if (a_rate > 2000) {
		databuf |= BIT_ACCEL_ODR_4000;
	} else if (a_rate > 1000) {
		databuf |= BIT_ACCEL_ODR_2000;
	} else if (a_rate > 500) {
		databuf |= BIT_ACCEL_ODR_1000;
	} else if (a_rate > 200) {
		databuf |= BIT_ACCEL_ODR_500;
	} else if (a_rate > 100) {
		databuf |= BIT_ACCEL_ODR_200;
	} else if (a_rate > 50) {
		databuf |= BIT_ACCEL_ODR_100;
	} else if (a_rate > 25) {
		databuf |= BIT_ACCEL_ODR_50;
	} else if (a_rate > 12) {
		databuf |= BIT_ACCEL_ODR_25;
	} else if (a_rate > 6) {
		databuf |= BIT_ACCEL_ODR_12;
	} else if (a_rate > 3) {
		databuf |= BIT_ACCEL_ODR_6;
	} else if (a_rate > 1) {
		databuf |= BIT_ACCEL_ODR_3;
	} else {
		databuf |= BIT_ACCEL_ODR_1;
	}

	result = inv_spi_single_write(REG_ACCEL_CONFIG0, &databuf);

	if (result) {
		return result;
	}

	LOG_DBG("Write Accel ODR 0x%X", databuf);

	result = inv_spi_read(REG_GYRO_CONFIG0, &databuf, 1);

	if (result) {
		return result;
	}

	databuf &= ~BIT_GYRO_ODR;

	if (g_rate > 4000) {
		databuf |= BIT_GYRO_ODR_8000;
	} else if (g_rate > 2000) {
		databuf |= BIT_GYRO_ODR_4000;
	} else if (g_rate > 1000) {
		databuf |= BIT_GYRO_ODR_2000;
	} else if (g_rate > 500) {
		databuf |= BIT_GYRO_ODR_1000;
	} else if (g_rate > 200) {
		databuf |= BIT_GYRO_ODR_500;
	} else if (g_rate > 100) {
		databuf |= BIT_GYRO_ODR_200;
	} else if (g_rate > 50) {
		databuf |= BIT_GYRO_ODR_100;
	} else if (g_rate > 25) {
		databuf |= BIT_GYRO_ODR_50;
	} else if (g_rate > 12) {
		databuf |= BIT_GYRO_ODR_25;
	} else {
		databuf |= BIT_GYRO_ODR_12;
	}

	LOG_DBG("Write GYRO ODR 0x%X", databuf);

	result = inv_spi_single_write(REG_GYRO_CONFIG0, &databuf);
	if (result) {
		return result;
	}

	return result;
}

int icm42605_sensor_init(const struct device *dev)
{
	int result = 0;
	uint8_t v;

	result = inv_spi_read(REG_WHO_AM_I, &v, 1);

	if (result) {
		return result;
	}

	LOG_DBG("WHO AM I : 0x%X", v);

	result = inv_spi_read(REG_DEVICE_CONFIG, &v, 1);

	if (result) {
		LOG_DBG("read REG_DEVICE_CONFIG_REG failed");
		return result;
	}

	v |= BIT_SOFT_RESET;

	result = inv_spi_single_write(REG_DEVICE_CONFIG, &v);

	if (result) {
		LOG_ERR("write REG_DEVICE_CONFIG failed");
		return result;
	}

	/* Need at least 10ms after soft reset */
	k_msleep(10);

	v = BIT_GYRO_AFSR_MODE_HFS | BIT_ACCEL_AFSR_MODE_HFS | BIT_CLK_SEL_PLL;

	result = inv_spi_single_write(REG_INTF_CONFIG1, &v);

	if (result) {
		LOG_ERR("write REG_INTF_CONFIG1 failed");
		return result;
	}

	v = BIT_EN_DREG_FIFO_D2A |
	    BIT_TMST_TO_REGS_EN |
	    BIT_TMST_EN;

	result = inv_spi_single_write(REG_TMST_CONFIG, &v);

	if (result) {
		LOG_ERR("Write REG_TMST_CONFIG failed");
		return result;
	}

	result = inv_spi_read(REG_INTF_CONFIG0, &v, 1);

	if (result) {
		LOG_ERR("Read REG_INTF_CONFIG0 failed");
		return result;
	}

	LOG_DBG("Read REG_INTF_CONFIG0 0x%X", v);

	v |= BIT_UI_SIFS_DISABLE_I2C;

	result = inv_spi_single_write(REG_INTF_CONFIG0, &v);

	if (result) {
		LOG_ERR("Write REG_INTF_CONFIG failed");
		return result;
	}

	v = 0;
	result = inv_spi_single_write(REG_INT_CONFIG1, &v);

	if (result) {
		return result;
	}

	result = inv_spi_single_write(REG_PWR_MGMT0, &v);

	if (result) {
		return result;
	}

	return 0;
}

int icm42605_turn_on_fifo(const struct device *dev)
{
	const struct icm42605_data *drv_data = dev->data;

	uint8_t int0_en = BIT_INT_UI_DRDY_INT1_EN;
	uint8_t fifo_en = BIT_FIFO_ACCEL_EN | BIT_FIFO_GYRO_EN | BIT_FIFO_WM_TH;
	uint8_t burst_read[3];
	int result;
	uint8_t v = 0;

	v = BIT_FIFO_MODE_BYPASS;
	result = inv_spi_single_write(REG_FIFO_CONFIG, &v);
	if (result) {
		return result;
	}

	v = 0;
	result = inv_spi_single_write(REG_FIFO_CONFIG1, &v);
	if (result) {
		return result;
	}

	result = inv_spi_read(REG_FIFO_COUNTH, burst_read, 2);
	if (result) {
		return result;
	}

	result = inv_spi_read(REG_FIFO_DATA, burst_read, 3);
	if (result) {
		return result;
	}

	v = BIT_FIFO_MODE_STREAM;
	result = inv_spi_single_write(REG_FIFO_CONFIG, &v);
	if (result) {
		return result;
	}

	result = inv_spi_single_write(REG_FIFO_CONFIG1, &fifo_en);
	if (result) {
		return result;
	}

	result = inv_spi_single_write(REG_INT_SOURCE0, &int0_en);
	if (result) {
		return result;
	}

	if (drv_data->tap_en) {
		v = BIT_TAP_ENABLE;
		result = inv_spi_single_write(REG_APEX_CONFIG0, &v);
		if (result) {
			return result;
		}

		v = BIT_DMP_INIT_EN;
		result = inv_spi_single_write(REG_SIGNAL_PATH_RESET, &v);
		if (result) {
			return result;
		}

		v = BIT_BANK_SEL_4;
		result = inv_spi_single_write(REG_BANK_SEL, &v);
		if (result) {
			return result;
		}

		v = BIT_INT_STATUS_TAP_DET;
		result = inv_spi_single_write(REG_INT_SOURCE6, &v);
		if (result) {
			return result;
		}

		v = BIT_BANK_SEL_0;
		result = inv_spi_single_write(REG_BANK_SEL, &v);
		if (result) {
			return result;
		}
	}

	LOG_DBG("turn on fifo done");
	return 0;
}

int icm42605_turn_off_fifo(const struct device *dev)
{
	const struct icm42605_data *drv_data = dev->data;

	uint8_t int0_en = 0;
	uint8_t burst_read[3];
	int result;
	uint8_t v = 0;

	v = BIT_FIFO_MODE_BYPASS;
	result = inv_spi_single_write(REG_FIFO_CONFIG, &v);
	if (result) {
		return result;
	}

	v = 0;
	result = inv_spi_single_write(REG_FIFO_CONFIG1, &v);
	if (result) {
		return result;
	}

	result = inv_spi_read(REG_FIFO_COUNTH, burst_read, 2);
	if (result) {
		return result;
	}

	result = inv_spi_read(REG_FIFO_DATA, burst_read, 3);
	if (result) {
		return result;
	}

	result = inv_spi_single_write(REG_INT_SOURCE0, &int0_en);
	if (result) {
		return result;
	}

	if (drv_data->tap_en) {
		v = 0;
		result = inv_spi_single_write(REG_APEX_CONFIG0, &v);
		if (result) {
			return result;
		}

		result = inv_spi_single_write(REG_SIGNAL_PATH_RESET, &v);
		if (result) {
			return result;
		}

		v = BIT_BANK_SEL_4;
		result = inv_spi_single_write(REG_BANK_SEL, &v);
		if (result) {
			return result;
		}

		v = 0;
		result = inv_spi_single_write(REG_INT_SOURCE6, &v);
		if (result) {
			return result;
		}

		v = BIT_BANK_SEL_0;
		result = inv_spi_single_write(REG_BANK_SEL, &v);
		if (result) {
			return result;
		}
	}

	return 0;
}

int icm42605_turn_on_sensor(const struct device *dev)
{
	struct icm42605_data *drv_data = dev->data;

	uint8_t v = 0;
	int result = 0;


	if (drv_data->sensor_started) {
		LOG_ERR("Sensor already started");
		return -EALREADY;
	}

	icm42605_set_fs(dev, drv_data->accel_sf, drv_data->gyro_sf);

	icm42605_set_odr(dev, drv_data->accel_hz, drv_data->gyro_hz);

	v |= BIT_ACCEL_MODE_LNM;
	v |= BIT_GYRO_MODE_LNM;

	result = inv_spi_single_write(REG_PWR_MGMT0, &v);
	if (result) {
		return result;
	}

	/* Accelerometer sensor need at least 10ms startup time
	 * Gyroscope sensor need at least 30ms startup time
	 */
	k_msleep(100);

	icm42605_turn_on_fifo(dev);

	drv_data->sensor_started = true;

	return 0;
}

int icm42605_turn_off_sensor(const struct device *dev)
{
	uint8_t v = 0;
	int result = 0;

	result = inv_spi_read(REG_PWR_MGMT0, &v, 1);

	v ^= BIT_ACCEL_MODE_LNM;
	v ^= BIT_GYRO_MODE_LNM;

	result = inv_spi_single_write(REG_PWR_MGMT0, &v);
	if (result) {
		return result;
	}

	/* Accelerometer sensor need at least 10ms startup time
	 * Gyroscope sensor need at least 30ms startup time
	 */
	k_msleep(100);

	icm42605_turn_off_fifo(dev);

	return 0;
}
