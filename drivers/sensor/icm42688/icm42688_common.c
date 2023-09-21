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

	return 0;
}

int icm42688_configure(const struct device *dev, struct icm42688_cfg *cfg)
{
	struct icm42688_dev_data *dev_data = dev->data;
	const struct icm42688_dev_cfg *dev_cfg = dev->config;
	int res;

	/* Disable interrupts, reconfigured at end */
	res = icm42688_spi_single_write(&dev_cfg->spi, REG_INT_SOURCE0, 0);

	/* if fifo is enabled right now, disable and flush */
	if (dev_data->cfg.fifo_en) {
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG,
						FIELD_PREP(MASK_FIFO_MODE, BIT_FIFO_MODE_BYPASS));

		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG");
			return -EINVAL;
		}

		res = icm42688_spi_single_write(&dev_cfg->spi, REG_SIGNAL_PATH_RESET,
						FIELD_PREP(BIT_FIFO_FLUSH, 1));

		if (res != 0) {
			LOG_ERR("Error flushing fifo");
			return -EINVAL;
		}
	}

	/* TODO maybe do the next few steps intelligently by checking current config */

	/* Power management to set gyro/accel modes */
	uint8_t pwr_mgmt0 = FIELD_PREP(MASK_GYRO_MODE, cfg->gyro_mode) |
			    FIELD_PREP(MASK_ACCEL_MODE, cfg->accel_mode) |
			    FIELD_PREP(BIT_TEMP_DIS, cfg->temp_dis);

	LOG_DBG("PWR_MGMT0 (0x%x) 0x%x", REG_PWR_MGMT0, pwr_mgmt0);
	res = icm42688_spi_single_write(&dev_cfg->spi, REG_PWR_MGMT0, pwr_mgmt0);

	if (res != 0) {
		LOG_ERR("Error writing PWR_MGMT0");
		return -EINVAL;
	}

	/* Need to wait at least 200us before updating more registers
	 * see datasheet 14.36
	 */
	k_busy_wait(250);

	uint8_t accel_config0 = FIELD_PREP(MASK_ACCEL_ODR, cfg->accel_odr) |
				FIELD_PREP(MASK_ACCEL_UI_FS_SEL, cfg->accel_fs);

	LOG_DBG("ACCEL_CONFIG0 (0x%x) 0x%x", REG_ACCEL_CONFIG0, accel_config0);
	res = icm42688_spi_single_write(&dev_cfg->spi, REG_ACCEL_CONFIG0, accel_config0);
	if (res != 0) {
		LOG_ERR("Error writing ACCEL_CONFIG0");
		return -EINVAL;
	}

	uint8_t gyro_config0 = FIELD_PREP(MASK_GYRO_ODR, cfg->gyro_odr) |
			       FIELD_PREP(MASK_GYRO_UI_FS_SEL, cfg->gyro_fs);

	LOG_DBG("GYRO_CONFIG0 (0x%x) 0x%x", REG_GYRO_CONFIG0, gyro_config0);
	res = icm42688_spi_single_write(&dev_cfg->spi, REG_GYRO_CONFIG0, gyro_config0);
	if (res != 0) {
		LOG_ERR("Error writing GYRO_CONFIG0");
		return -EINVAL;
	}

	/*
	 * Accelerometer sensor need at least 10ms startup time
	 * Gyroscope sensor need at least 30ms startup time
	 */
	k_msleep(50);

	/* Ensure FIFO is in bypass mode */
	uint8_t fifo_config_bypass = FIELD_PREP(MASK_FIFO_MODE, BIT_FIFO_MODE_BYPASS);

	LOG_DBG("FIFO_CONFIG (0x%x) 0x%x", REG_FIFO_CONFIG, fifo_config_bypass);
	res = icm42688_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG, fifo_config_bypass);
	if (res != 0) {
		LOG_ERR("Error writing FIFO_CONFIG");
		return -EINVAL;
	}

	/* Disable FSYNC */
	uint8_t tmst_config;

	res = icm42688_spi_single_write(&dev_cfg->spi, REG_FSYNC_CONFIG, 0);
	if (res != 0) {
		LOG_ERR("Error writing FSYNC_CONFIG");
		return -EINVAL;
	}
	res = icm42688_spi_read(&dev_cfg->spi, REG_TMST_CONFIG, &tmst_config, 1);
	if (res != 0) {
		LOG_ERR("Error reading TMST_CONFIG");
		return -EINVAL;
	}
	res = icm42688_spi_single_write(&dev_cfg->spi, REG_TMST_CONFIG, tmst_config & ~BIT(1));
	if (res != 0) {
		LOG_ERR("Error writing TMST_CONFIG");
		return -EINVAL;
	}

	/* Pulse mode with async reset (resets interrupt line on int status read) */
	res = icm42688_spi_single_write(&dev_cfg->spi, REG_INT_CONFIG,
					BIT_INT1_DRIVE_CIRCUIT | BIT_INT1_POLARITY);
	if (res) {
		LOG_ERR("Error writing to INT_CONFIG");
		return res;
	}

	uint8_t int_config1 = 0;

	if ((cfg->accel_odr <= ICM42688_ACCEL_ODR_4000 ||
	     cfg->gyro_odr <= ICM42688_GYRO_ODR_4000)) {
		int_config1 = FIELD_PREP(BIT_INT_TPULSE_DURATION, 1) |
			      FIELD_PREP(BIT_INT_TDEASSERT_DISABLE, 1);
	}

	res = icm42688_spi_single_write(&dev_cfg->spi, REG_INT_CONFIG1, int_config1);
	if (res) {
		LOG_ERR("Error writing to INT_CONFIG1");
		return res;
	}

	/* fifo configuration steps if desired */
	if (cfg->fifo_en) {
		LOG_INF("FIFO ENABLED");

		/* Setup desired FIFO packet fields, maybe should base this on the other
		 * temp/accel/gyro en fields in cfg
		 */
		uint8_t fifo_cfg1 =
			FIELD_PREP(BIT_FIFO_TEMP_EN, 1) | FIELD_PREP(BIT_FIFO_GYRO_EN, 1) |
			FIELD_PREP(BIT_FIFO_ACCEL_EN, 1) | FIELD_PREP(BIT_FIFO_TMST_FSYNC_EN, 1);

		LOG_DBG("FIFO_CONFIG1 (0x%x) 0x%x", REG_FIFO_CONFIG1, fifo_cfg1);
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG1, fifo_cfg1);
		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG1");
			return -EINVAL;
		}

		/* Set watermark and interrupt handling first */
		uint8_t fifo_wml = (cfg->fifo_wm) & 0xFF;

		LOG_DBG("FIFO_CONFIG2( (0x%x)) (WM Low) 0x%x", REG_FIFO_CONFIG2, fifo_wml);
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG2, fifo_wml);
		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG2");
			return -EINVAL;
		}

		uint8_t fifo_wmh = (cfg->fifo_wm >> 8) & 0x0F;

		LOG_DBG("FIFO_CONFIG3 (0x%x) (WM High) 0x%x", REG_FIFO_CONFIG3, fifo_wmh);
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG3, fifo_wmh);
		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG3");
			return -EINVAL;
		}

		/* Begin streaming */
		uint8_t fifo_config = FIELD_PREP(MASK_FIFO_MODE, BIT_FIFO_MODE_STREAM);

		LOG_DBG("FIFO_CONFIG (0x%x) 0x%x", REG_FIFO_CONFIG, 1 << 6);
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG, fifo_config);

		/* Config interrupt source to only be fifo wm/full */
		uint8_t int_source0 = BIT_FIFO_FULL_INT1_EN | BIT_FIFO_THS_INT1_EN;

		LOG_DBG("INT_SOURCE0 (0x%x) 0x%x", REG_INT_SOURCE0, int_source0);
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_INT_SOURCE0, int_source0);
		if (res) {
			return res;
		}
	} else {
		LOG_INF("FIFO DISABLED");

		/* No fifo mode so set data ready as interrupt source */
		uint8_t int_source0 = BIT_UI_DRDY_INT1_EN;

		LOG_DBG("INT_SOURCE0 (0x%x) 0x%x", REG_INT_SOURCE0, int_source0);
		res = icm42688_spi_single_write(&dev_cfg->spi, REG_INT_SOURCE0, int_source0);
		if (res) {
			return res;
		}
	}

	return res;
}

int icm42688_safely_configure(const struct device *dev, struct icm42688_cfg *cfg)
{
	struct icm42688_dev_data *drv_data = dev->data;
	int ret = icm42688_configure(dev, cfg);

	if (ret == 0) {
		drv_data->cfg = *cfg;
	} else {
		ret = icm42688_configure(dev, &drv_data->cfg);
	}

	return ret;
}

int icm42688_read_all(const struct device *dev, uint8_t data[14])
{
	const struct icm42688_dev_cfg *dev_cfg = dev->config;
	int res;

	res = icm42688_spi_read(&dev_cfg->spi, REG_TEMP_DATA1, data, 14);

	return res;
}
