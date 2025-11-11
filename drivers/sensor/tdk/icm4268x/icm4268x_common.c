/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/icm4268x.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include "icm4268x.h"
#include "icm4268x_reg.h"
#include "icm4268x_spi.h"
#include "icm4268x_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM4268X_LL, CONFIG_SENSOR_LOG_LEVEL);

int icm4268x_reset(const struct device *dev)
{
	int res;
	uint8_t value;
	const struct icm4268x_dev_cfg *dev_cfg = dev->config;
	struct icm4268x_dev_data *dev_data = dev->data;
	uint8_t expected_who_am_i;

	/* start up time for register read/write after POR is 1ms and supply ramp time is 3ms */
	k_msleep(3);

	/* perform a soft reset to ensure a clean slate, reset bit will auto-clear */
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_DEVICE_CONFIG, BIT_SOFT_RESET_CONFIG);

	if (res) {
		LOG_ERR("write REG_SIGNAL_PATH_RESET failed");
		return res;
	}

	/* wait for soft reset to take effect */
	k_msleep(SOFT_RESET_TIME_MS);

	/* clear reset done int flag */
	res = icm4268x_spi_read(&dev_cfg->spi, REG_INT_STATUS, &value, 1);

	if (res) {
		return res;
	}

	if (FIELD_GET(BIT_RESET_DONE_INT, value) != 1) {
		LOG_ERR("unexpected RESET_DONE value, %i", value);
		return -EINVAL;
	}

	res = icm4268x_spi_read(&dev_cfg->spi, REG_WHO_AM_I, &value, 1);
	if (res) {
		return res;
	}

	switch (dev_data->cfg.variant) {
	case ICM4268X_VARIANT_ICM42688:
		expected_who_am_i = WHO_AM_I_ICM42688;
		break;
	case ICM4268X_VARIANT_ICM42686:
		expected_who_am_i = WHO_AM_I_ICM42686;
		break;
	default:
		LOG_ERR("Invalid variant: %d", dev_data->cfg.variant);
		return -EINVAL;
	}

	if (value != expected_who_am_i) {
		LOG_ERR("invalid WHO_AM_I value, was %i but expected %i", value, expected_who_am_i);
		return -EINVAL;
	}

	return 0;
}

static uint16_t icm4268x_compute_fifo_wm(const struct icm4268x_cfg *cfg)
{
	const bool accel_enabled = cfg->accel_pwr_mode != ICM42688_DT_ACCEL_OFF;
	const bool gyro_enabled = cfg->gyro_pwr_mode != ICM42688_DT_GYRO_OFF;
	int accel_modr = 0;
	int gyro_modr = 0;
	uint8_t pkt_size;
	int64_t modr;

	if (cfg->fifo_hires) {
		pkt_size = 20;
	} else if (accel_enabled && gyro_enabled) {
		pkt_size = 16;
	} else {
		pkt_size = 8;
	}

	if (cfg->batch_ticks == 0 || (!accel_enabled && !gyro_enabled)) {
		return 0;
	}

	if (accel_enabled) {
		struct sensor_value val = {0};

		icm4268x_accel_reg_to_hz(cfg->accel_odr, &val);
		accel_modr = sensor_value_to_micro(&val) / 1000;
	}
	if (gyro_enabled) {
		struct sensor_value val = {0};

		icm4268x_gyro_reg_to_odr(cfg->gyro_odr, &val);
		gyro_modr = sensor_value_to_micro(&val) / 1000;
	}

	if (accel_modr == 0) {
		modr = gyro_modr;
	} else if (gyro_modr == 0) {
		modr = accel_modr;
	} else {
		/* Need to find the least common multiplier (LCM) */
		int n1 = accel_modr;
		int n2 = gyro_modr;

		while (n1 != n2) {
			if (n1 > n2) {
				n1 -= n2;
			} else {
				n2 -= n1;
			}
		}
		LOG_DBG("GCD=%d", n1);
		modr = ((int64_t)accel_modr * (int64_t)gyro_modr) / n1;
	}
	/* At this point we have 'modr' as mHz which is 1 / msec. */

	/* Convert 'modr' to bytes * batch_ticks / msec */
	modr *= (int64_t)cfg->batch_ticks * pkt_size;

	/* 'modr' = byte_ticks_per_msec / kticks_per_sec */
	modr = DIV_ROUND_UP(modr, CONFIG_SYS_CLOCK_TICKS_PER_SEC * INT64_C(1000));

	return (uint16_t)MIN(modr, 0x7ff);
}

int icm4268x_configure(const struct device *dev, struct icm4268x_cfg *cfg)
{
	struct icm4268x_dev_data *dev_data = dev->data;
	const struct icm4268x_dev_cfg *dev_cfg = dev->config;
	int res;

	/* Disable interrupts, reconfigured at end */
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INT_SOURCE0, 0);

	/* if fifo is enabled right now, disable and flush */
	if (dev_data->cfg.fifo_en) {
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG,
						FIELD_PREP(MASK_FIFO_MODE, BIT_FIFO_MODE_BYPASS));

		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG");
			return -EINVAL;
		}

		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_SIGNAL_PATH_RESET,
						FIELD_PREP(BIT_FIFO_FLUSH, 1));

		if (res != 0) {
			LOG_ERR("Error flushing fifo");
			return -EINVAL;
		}
	}

	/* TODO maybe do the next few steps intelligently by checking current config */

	/* Select register bank 1 */
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_BANK_SEL, BIT_BANK1);
	if (res != 0) {
		LOG_ERR("Error selecting register bank 1");
		return -EINVAL;
	}

	/* Set pin 9 function */
	uint8_t intf_config5 = FIELD_PREP(MASK_PIN9_FUNCTION, cfg->pin9_function);

	LOG_DBG("INTF_CONFIG5 (0x%lx) 0x%x", FIELD_GET(REG_ADDRESS_MASK, REG_INTF_CONFIG5),
		intf_config5);
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INTF_CONFIG5, intf_config5);
	if (res != 0) {
		LOG_ERR("Error writing INTF_CONFIG5");
		return -EINVAL;
	}

	/* Select register bank 0 */
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_BANK_SEL, BIT_BANK0);
	if (res != 0) {
		LOG_ERR("Error selecting register bank 0");
		return -EINVAL;
	}

	bool is_pin9_clkin = cfg->pin9_function == ICM4268X_PIN9_FUNCTION_CLKIN;
	uint8_t intf_config1 = 0x91 | FIELD_PREP(BIT_RTC_MODE, is_pin9_clkin);

	LOG_DBG("INTF_CONFIG1 (0x%x) 0x%x", REG_INTF_CONFIG1, intf_config1);
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INTF_CONFIG1, intf_config1);
	if (res != 0) {
		LOG_ERR("Error writing INTF_CONFIG1");
		return -EINVAL;
	}

	/* Power management to set gyro/accel modes */
	uint8_t pwr_mgmt0 = FIELD_PREP(MASK_GYRO_MODE, cfg->gyro_pwr_mode) |
			    FIELD_PREP(MASK_ACCEL_MODE, cfg->accel_pwr_mode) |
			    FIELD_PREP(BIT_TEMP_DIS, cfg->temp_dis);

	LOG_DBG("PWR_MGMT0 (0x%x) 0x%x", REG_PWR_MGMT0, pwr_mgmt0);
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_PWR_MGMT0, pwr_mgmt0);

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
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_ACCEL_CONFIG0, accel_config0);
	if (res != 0) {
		LOG_ERR("Error writing ACCEL_CONFIG0");
		return -EINVAL;
	}

	uint8_t gyro_config0 = FIELD_PREP(MASK_GYRO_ODR, cfg->gyro_odr) |
			       FIELD_PREP(MASK_GYRO_UI_FS_SEL, cfg->gyro_fs);

	LOG_DBG("GYRO_CONFIG0 (0x%x) 0x%x", REG_GYRO_CONFIG0, gyro_config0);
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_GYRO_CONFIG0, gyro_config0);
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
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG, fifo_config_bypass);
	if (res != 0) {
		LOG_ERR("Error writing FIFO_CONFIG");
		return -EINVAL;
	}

	/* Disable FSYNC */
	uint8_t tmst_config;

	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FSYNC_CONFIG, 0);
	if (res != 0) {
		LOG_ERR("Error writing FSYNC_CONFIG");
		return -EINVAL;
	}
	res = icm4268x_spi_read(&dev_cfg->spi, REG_TMST_CONFIG, &tmst_config, 1);
	if (res != 0) {
		LOG_ERR("Error reading TMST_CONFIG");
		return -EINVAL;
	}
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_TMST_CONFIG, tmst_config & ~BIT(1));
	if (res != 0) {
		LOG_ERR("Error writing TMST_CONFIG");
		return -EINVAL;
	}

	/* Pulse mode with async reset (resets interrupt line on int status read) */
	if (IS_ENABLED(CONFIG_ICM4268X_TRIGGER)) {
		res = icm4268x_trigger_enable_interrupt(dev, cfg);
	} else {
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INT_CONFIG,
						BIT_INT1_DRIVE_CIRCUIT | BIT_INT1_POLARITY);
	}
	if (res) {
		LOG_ERR("Error writing to INT_CONFIG");
		return res;
	}

	uint8_t int_config1 = 0;

	if ((cfg->accel_odr <= ICM42688_DT_ACCEL_ODR_4000 ||
	     cfg->gyro_odr <= ICM42688_DT_GYRO_ODR_4000)) {
		int_config1 = FIELD_PREP(BIT_INT_TPULSE_DURATION, 1) |
			      FIELD_PREP(BIT_INT_TDEASSERT_DISABLE, 1);
	}

	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INT_CONFIG1, int_config1);
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
			FIELD_PREP(BIT_FIFO_TEMP_EN, 1) |
			FIELD_PREP(BIT_FIFO_GYRO_EN, 1) |
			FIELD_PREP(BIT_FIFO_ACCEL_EN, 1) |
			FIELD_PREP(BIT_FIFO_TMST_FSYNC_EN, 1) |
			FIELD_PREP(BIT_FIFO_HIRES_EN, cfg->fifo_hires);

		LOG_DBG("HIRES MODE ENABLED?: %d", cfg->fifo_hires);

		LOG_DBG("FIFO_CONFIG1 (0x%x) 0x%x", REG_FIFO_CONFIG1, fifo_cfg1);
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG1, fifo_cfg1);
		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG1");
			return -EINVAL;
		}

		/* Set watermark and interrupt handling first */
		cfg->fifo_wm = icm4268x_compute_fifo_wm(cfg);

		uint8_t fifo_wml = cfg->fifo_wm & 0xFF;
		uint8_t fifo_wmh = (cfg->fifo_wm >> 8) & 0x0F;

		LOG_DBG("FIFO_CONFIG2( (0x%x)) (WM Low) 0x%x", REG_FIFO_CONFIG2, fifo_wml);
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG2, fifo_wml);
		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG2");
			return -EINVAL;
		}

		LOG_DBG("FIFO_CONFIG3 (0x%x) (WM High) 0x%x", REG_FIFO_CONFIG3, fifo_wmh);
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG3, fifo_wmh);
		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG3");
			return -EINVAL;
		}

		/* Begin streaming */
		uint8_t fifo_config = FIELD_PREP(MASK_FIFO_MODE, BIT_FIFO_MODE_STREAM);

		LOG_DBG("FIFO_CONFIG (0x%x) 0x%x", REG_FIFO_CONFIG, 1 << 6);
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG, fifo_config);

		/* Config interrupt source to only be fifo wm/full */
		uint8_t int_source0 = BIT_FIFO_FULL_INT1_EN | BIT_FIFO_THS_INT1_EN;

		LOG_DBG("INT_SOURCE0 (0x%x) 0x%x", REG_INT_SOURCE0, int_source0);
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INT_SOURCE0, int_source0);
		if (res) {
			return res;
		}
	} else {
		LOG_INF("FIFO DISABLED");

		/* No fifo mode so set data ready as interrupt source */
		uint8_t int_source0 = BIT_UI_DRDY_INT1_EN;

		LOG_DBG("INT_SOURCE0 (0x%x) 0x%x", REG_INT_SOURCE0, int_source0);
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INT_SOURCE0, int_source0);
		if (res) {
			return res;
		}
	}

	return res;
}

int icm4268x_safely_configure(const struct device *dev, struct icm4268x_cfg *cfg)
{
	struct icm4268x_dev_data *drv_data = dev->data;
	int ret = icm4268x_configure(dev, cfg);

	if (ret == 0) {
		drv_data->cfg = *cfg;
	} else {
		ret = icm4268x_configure(dev, &drv_data->cfg);
	}

	return ret;
}

int icm4268x_read_all(const struct device *dev, uint8_t data[14])
{
	const struct icm4268x_dev_cfg *dev_cfg = dev->config;
	int res;

	res = icm4268x_spi_read(&dev_cfg->spi, REG_TEMP_DATA1, data, 14);

	return res;
}
