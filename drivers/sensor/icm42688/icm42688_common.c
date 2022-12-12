/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include "icm42688.h"
#include "icm42688_reg.h"
#include "icm42688_spi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42688_LL, CONFIG_SENSOR_LOG_LEVEL);

int icm42688_reset(const struct device *dev)
{
	int res;
	uint8_t value;
	const struct icm42688_dev_cfg *dev_cfg = dev->config;

	/* start up time for register read/write after POR is 1ms and supply ramp time is 3ms */
	k_msleep(3);

	/* perform a soft reset to ensure a clean slate, reset bit will auto-clear */
	res = icm42688_spi_single_write(&dev_cfg->spi, REG_DEVICE_CONFIG, BIT_SOFT_RESET);

	if (res) {
		LOG_ERR("write REG_SIGNAL_PATH_RESET failed");
		return res;
	}

	/* wait for soft reset to take effect */
	k_msleep(SOFT_RESET_TIME_MS);

	/* clear reset done int flag */
	res = icm42688_spi_read(&dev_cfg->spi, REG_INT_STATUS, &value, 1);

	if (res) {
		return res;
	}

	if (FIELD_GET(BIT_INT_STATUS_RESET_DONE, value) != 1) {
		LOG_ERR("unexpected RESET_DONE value, %i", value);
		return -EINVAL;
	}

	res = icm42688_spi_read(&dev_cfg->spi, REG_WHO_AM_I, &value, 1);

	if (res) {
		return res;
	}

	if (value != WHO_AM_I_ICM42688) {
		LOG_ERR("invalid WHO_AM_I value, was %i but expected %i", value, WHO_AM_I_ICM42688);
		return -EINVAL;
	}

	LOG_DBG("device id: 0x%02X", value);
	return res;
}

int icm42688_configure(const struct device *dev, struct icm42688_cfg *cfg)
{
	struct icm42688_dev_data *dev_data = dev->data;
	const struct icm42688_dev_cfg *dev_cfg = dev->config;
	int res;

	/* if fifo is enabled right now, disable and flush */
	if (dev_data->cfg.fifo_en) {
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG,
						FIELD_PREP(MASK_FIFO_MODE, BIT_FIFO_MODE_BYPASS));

		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG");
			return -EINVAL;
		}

		/* TODO ideally this might read out the FIFO one last time and call
		 * out to the application with the data there prior to reconfiguring?
		 */
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_SIGNAL_PATH_RESET,
						FIELD_PREP(BIT_FIFO_FLUSH, 1));

		if (res != 0) {
			LOG_ERR("Error flushing fifo");
			return -EINVAL;
		}
	}

	/* TODO maybe do the next few steps intelligently by checking current config */

	/* Power management to set gyro/accel modes */
	res = icm42688_spi_single_write(&dev_cfg->spi, REG_PWR_MGMT0,
					FIELD_PREP(MASK_GYRO_MODE, cfg->gyro_mode) |
						FIELD_PREP(MASK_ACCEL_MODE, cfg->accel_mode) |
						FIELD_PREP(BIT_TEMP_DIS, cfg->temp_dis));

	if (res != 0) {
		LOG_ERR("Error writing PWR_MGMT0");
		return -EINVAL;
	}
	dev_data->cfg.gyro_mode = cfg->gyro_mode;
	dev_data->cfg.accel_mode = cfg->accel_mode;
	dev_data->cfg.temp_dis = cfg->temp_dis;

	/* Need to wait at least 200us before updating more registers
	 * see datasheet 14.36
	 */
	k_busy_wait(250);

	res = icm42688_spi_single_write(&dev_cfg->spi, REG_ACCEL_CONFIG0,
					FIELD_PREP(MASK_ACCEL_ODR, cfg->accel_odr) |
						FIELD_PREP(MASK_ACCEL_UI_FS_SEL, cfg->accel_fs));

	if (res != 0) {
		LOG_ERR("Error writing ACCEL_CONFIG0");
		return -EINVAL;
	}
	dev_data->cfg.accel_odr = cfg->accel_odr;
	dev_data->cfg.accel_fs = cfg->accel_fs;

	res = icm42688_spi_single_write(&dev_cfg->spi, REG_GYRO_CONFIG0,
					FIELD_PREP(MASK_GYRO_ODR, cfg->gyro_odr) |
						FIELD_PREP(MASK_GYRO_UI_FS_SEL, cfg->gyro_fs));

	if (res != 0) {
		LOG_ERR("Error writing GYRO_CONFIG0");
		return -EINVAL;
	}
	dev_data->cfg.gyro_odr = cfg->gyro_odr;
	dev_data->cfg.gyro_fs = cfg->gyro_fs;

	/*
	 * Accelerometer sensor need at least 10ms startup time
	 * Gyroscope sensor need at least 30ms startup time
	 */
	k_msleep(50);

	/* fifo configuration steps if desired */
	if (cfg->fifo_en) {
		/* Set watermark and interrupt handling first */
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG2,
						cfg->fifo_wm & 0xFF);

		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG2");
			return -EINVAL;
		}

		res = icm42688_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG3,
						(cfg->fifo_wm >> 8) & 0x0F);

		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG3");
			return -EINVAL;
		}
		dev_data->cfg.fifo_wm = cfg->fifo_wm;

		/* TODO we have two interrupt lines, either can be used for watermark, choose 1 for
		 *   now...
		 */
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_INT_SOURCE0,
						FIELD_PREP(BIT_FIFO_THS_INT1_EN, 1));

		if (res != 0) {
			LOG_ERR("Error writing INT0_SOURCE");
			return -EINVAL;
		}

		/* Setup desired FIFO packet fields, maybe should base this on the other
		 * temp/accel/gyro en fields in cfg
		 */
		res = icm42688_spi_single_write(
			&dev_cfg->spi, REG_FIFO_CONFIG1,
			FIELD_PREP(BIT_FIFO_HIRES_EN, cfg->fifo_hires ? 1 : 0) |
				FIELD_PREP(BIT_FIFO_TEMP_EN, 1) | FIELD_PREP(BIT_FIFO_GYRO_EN, 1) |
				FIELD_PREP(BIT_FIFO_ACCEL_EN, 1));

		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG1");
			return -EINVAL;
		}
		dev_data->cfg.fifo_hires = cfg->fifo_hires;

		/* Begin streaming */
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG,
						FIELD_PREP(MASK_FIFO_MODE, BIT_FIFO_MODE_STREAM));
		dev_data->cfg.fifo_en = true;
	}

	return res;
}

int icm42688_read_all(const struct device *dev, uint8_t data[14])
{
	const struct icm42688_dev_cfg *dev_cfg = dev->config;
	int res;

	res = icm42688_spi_read(&dev_cfg->spi, REG_TEMP_DATA1, data, 14);

	return res;
}
