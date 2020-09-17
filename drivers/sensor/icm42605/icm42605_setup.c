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

LOG_MODULE_REGISTER(ICM42605, CONFIG_SENSOR_LOG_LEVEL);

int icm42605_set_fs(struct device *dev, uint16_t a_sf, uint16_t g_sf)
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

int icm42605_set_odr(struct device *dev, uint16_t a_rate, uint16_t g_rate)
{
	uint8_t databuf;
	int result;

	result = inv_spi_read(REG_ACCEL_CONFIG0, &databuf, 1);

	if (result) {
		return result;
	}

	databuf &= ~BIT_ACCEL_ODR;

	if (a_rate > 500) {
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
	} else {
		/* low power mode */
		if (a_rate > 3) {
			databuf |= BIT_ACCEL_ODR_6;
		} else {
			databuf |= BIT_ACCEL_ODR_3;
		}
		/* low noise mode */
		databuf |= BIT_ACCEL_ODR_12;
	}

	result = inv_spi_single_write(REG_ACCEL_CONFIG0, &databuf);

	if (result) {
		return result;
	}

	result = inv_spi_read(REG_GYRO_CONFIG0, &databuf, 1);

	if (result) {
		return result;
	}

	databuf &= ~BIT_GYRO_ODR;

	if (g_rate > 500) {
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

int icm42605_sensor_init(struct device *dev)
{
	struct icm42605_data *drv_data = dev->data;

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

	k_msleep(100);

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

	/* 0x03 -> Disable i2c */
	v |= 0x03;

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

int icm42605_turn_on_fifo(struct device *dev)
{
	struct icm42605_data *drv_data = dev->data;

	uint8_t int0_en, int1_en, fifo_en, smd;
	uint8_t burst_read[3];
	int r;
	uint8_t v = 0;

	v = BIT_FIFO_MODE_BYPASS;
	r = inv_spi_single_write(REG_FIFO_CONFIG, &v);
	if (r) {
		return r;
	}
	v = 0;
	r = inv_spi_single_write(REG_FIFO_CONFIG1, &v);
	if (r) {
		return r;
	}
	r = inv_spi_read(REG_FIFO_COUNTH, burst_read, 2);
	if (r) {
		return r;
	}
	r = inv_spi_read(REG_FIFO_DATA, burst_read, 3);
	if (r) {
		return r;
	}
	v = BIT_FIFO_MODE_STREAM;
	r = inv_spi_single_write(REG_FIFO_CONFIG, &v);
	if (r) {
		return r;
	}
	int0_en = 0;
	int1_en = 0;
	smd = 0;

	int0_en = BIT_INT_UI_DRDY_INT1_EN;
	fifo_en |= (BIT_FIFO_ACCEL_EN  | BIT_FIFO_GYRO_EN | BIT_FIFO_WM_TH);

	r = inv_spi_single_write(REG_FIFO_CONFIG1, &fifo_en);
	if (r) {
		return r;
	}
	r = inv_spi_single_write(REG_INT_SOURCE0, &int0_en);
	if (r) {
		return r;
	}

	if (drv_data->tap_en) {
		v = BIT_TAP_ENABLE;
		r = inv_spi_single_write(REG_APEX_CONFIG0, &v);
		v = BIT_DMP_INIT_EN;
		r = inv_spi_single_write(REG_SIGNAL_PATH_RESET, &v);
		v = BIT_BANK_SEL_4;
		r = inv_spi_single_write(REG_BANK_SEL, &v);
		v = BIT_INT_STATUS_TAP_DET;
		r = inv_spi_single_write(REG_INT_SOURCE6, &v);
		v = BIT_BANK_SEL_0;
		r = inv_spi_single_write(REG_BANK_SEL, &v);
	}

	LOG_DBG("turn on fifo done");
	return r;
}

int icm42605_turn_off_fifo(struct device *dev)
{
	struct icm42605_data *drv_data = dev->data;

	uint8_t int0_en = 0;
	uint8_t burst_read[3];
	int r;
	uint8_t v = 0;

	v = BIT_FIFO_MODE_BYPASS;
	r = inv_spi_single_write(REG_FIFO_CONFIG, &v);
	if (r) {
		return r;
	}
	v = 0;
	r = inv_spi_single_write(REG_FIFO_CONFIG1, &v);
	if (r) {
		return r;
	}
	r = inv_spi_read(REG_FIFO_COUNTH, burst_read, 2);
	if (r) {
		return r;
	}
	r = inv_spi_read(REG_FIFO_DATA, burst_read, 3);
	if (r) {
		return r;
	}

	r = inv_spi_single_write(REG_INT_SOURCE0, &int0_en);
	if (r) {
		return r;
	}

	if (drv_data->tap_en) {
		v = 0;
		r = inv_spi_single_write(REG_APEX_CONFIG0, &v);
		r = inv_spi_single_write(REG_SIGNAL_PATH_RESET, &v);
		v = BIT_BANK_SEL_4;
		r = inv_spi_single_write(REG_BANK_SEL, &v);
		v = 0;
		r = inv_spi_single_write(REG_INT_SOURCE6, &v);
		v = BIT_BANK_SEL_0;
		r = inv_spi_single_write(REG_BANK_SEL, &v);
	}

	return r;
}

int icm42605_turn_on_sensor(struct device *dev)
{
	struct icm42605_data *drv_data = dev->data;

	uint8_t v = 0;
	int result = 0;


	if (drv_data->sensor_started) {
		LOG_ERR("Sensor already started");
		return 0;
	}

	icm42605_set_fs(dev, drv_data->accel_sf, drv_data->gyro_sf);

	icm42605_set_odr(dev, drv_data->accel_hz, drv_data->gyro_hz);

	v |= BIT_ACCEL_MODE_LNM;
	v |= BIT_GYRO_MODE_LNM;

	result = inv_spi_single_write(REG_PWR_MGMT0, &v);

	k_msleep(200);

	icm42605_turn_on_fifo(dev);

	drv_data->sensor_started = true;

	return 0;
}

int icm42605_turn_off_sensor(struct device *dev)
{
	uint8_t v = 0;
	int result = 0;

	result = inv_spi_read(REG_PWR_MGMT0, &v, 1);

	v ^= BIT_ACCEL_MODE_LNM;
	v ^= BIT_GYRO_MODE_LNM;

	result = inv_spi_single_write(REG_PWR_MGMT0, &v);

	k_msleep(200);

	icm42605_turn_off_fifo(dev);

	return 0;
}
