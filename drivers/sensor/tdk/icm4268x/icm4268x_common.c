/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include "icm4268x.h"
#include "icm4268x_reg.h"
#include "icm4268x_spi.h"
#include "icm4268x_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM4268X_LL, CONFIG_SENSOR_LOG_LEVEL);

static struct icm4268x_aaf_cfg aaf_cfg_tbl[AAF_BW_MAX_IDX] = {
	{1, 1, 15},
	{2, 4, 13},
	{3, 9, 12},
	{4, 16, 11},
	{5, 25, 10},
	{6, 36, 10},
	{7, 49, 9},
	{8, 64, 9},
	{9, 81, 9},
	{10, 100, 8},
	{11, 122, 8},
	{12, 144, 8},
	{13, 170, 8},
	{14, 196, 7},
	{15, 224, 7},
	{16, 256, 7},
	{17, 288, 7},
	{18, 324, 7},
	{19, 360, 6},
	{20, 400, 6},
	{21, 440, 6},
	{22, 488, 6},
	{23, 528, 6},
	{24, 576, 6},
	{25, 624, 6},
	{26, 680, 6},
	{27, 736, 5},
	{28, 784, 5},
	{29, 848, 5},
	{30, 896, 5},
	{31, 960, 5},
	{32, 1024, 5},
	{33, 1088, 5},
	{34, 1152, 5},
	{35, 1232, 5},
	{36, 1296, 5},
	{37, 1376, 4},
	{38, 1440, 4},
	{39, 1536, 4},
	{40, 1600, 4},
	{41, 1696, 4},
	{42, 1760, 4},
	{43, 1856, 4},
	{44, 1952, 4},
	{45, 2016, 4},
	{46, 2112, 4},
	{47, 2208, 4},
	{48, 2304, 4},
	{49, 2400, 4},
	{50, 2496, 4},
	{51, 2592, 4},
	{52, 2720, 4},
	{53, 2816, 3},
	{54, 2944, 3},
	{55, 3008, 3},
	{56, 3136, 3},
	{57, 3264, 3},
	{58, 3392, 3},
	{59, 3456, 3},
	{60, 3584, 3},
	{61, 3712, 3},
	{62, 3840, 3},
	{63, 3968, 3}
};

int icm4268x_reset(const struct device *dev)
{
	int res;
	uint8_t value;
	struct icm4268x_dev_data *data = dev->data;
	struct icm4268x_cfg cfg = data->cfg;
	const struct icm4268x_dev_cfg *dev_cfg = dev->config;

	/* start up time for register read/write after POR is 1ms and supply ramp time is 3ms */
	k_msleep(3);

	/* perform a soft reset to ensure a clean slate, reset bit will auto-clear */
	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_DEVICE_CONFIG, BIT_SOFT_RESET);

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

	if (FIELD_GET(BIT_INT_STATUS_RESET_DONE, value) != 1) {
		LOG_ERR("unexpected RESET_DONE value, %i", value);
		return -EINVAL;
	}

	res = icm4268x_spi_read(&dev_cfg->spi, REG_WHO_AM_I, &value, 1);
	if (res) {
		return res;
	}

	if (value != cfg.dev_id) {
		LOG_ERR("invalid WHO_AM_I value, was %i but expected %i", value, cfg.dev_id);
		return -EINVAL;
	}

	return 0;
}

static uint16_t icm4268x_compute_fifo_wm(const struct icm4268x_cfg *cfg)
{
	const bool accel_enabled = cfg->accel_pwr_mode != ICM4268X_DT_ACCEL_OFF;
	const bool gyro_enabled = cfg->gyro_pwr_mode != ICM4268X_DT_GYRO_OFF;
	const int pkt_size = cfg->fifo_hires ? 20 : (accel_enabled && gyro_enabled ? 16 : 8);
	int accel_modr = 0;
	int gyro_modr = 0;
	int64_t modr;

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

	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_GYRO_CONFIG1, 0);
	if (res != 0) {
		LOG_ERR("Error writing GYRO_CONFIG1");
		return -EINVAL;
	}

	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_GYRO_ACCEL_CONFIG0, 0);
	if (res != 0) {
		LOG_ERR("Error writing GYRO_ACCEL_CONFIG0");
		return -EINVAL;
	}

	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_ACCEL_CONFIG1, 0);
	if (res != 0) {
		LOG_ERR("Error writing ACCEL_CONFIG1");
		return -EINVAL;
	}

	/* Accelerometer AAF Configuration */
	uint8_t aaf_tbl_idx = ((cfg->aaf_cfg[AAF_ACCEL_IDX].aaf_delt) - 1);

	uint8_t aaf_cfg_static2 =
		((cfg->aaf_cfg[AAF_ACCEL_IDX].aaf_delt << 1) & MASK_ACCEL_AAF_DELT);

	LOG_DBG("ACCEL_CONFIG_STATIC2 (0x%x) 0x%x", ACCEL_CONFIG_STATIC2, aaf_cfg_static2);
	res = icm4268x_spi_single_write(&dev_cfg->spi, ACCEL_CONFIG_STATIC2, aaf_cfg_static2);
	if (res != 0) {
		LOG_ERR("Error writing ACCEL_CONFIG_STATIC2");
		return -EINVAL;
	}

	uint8_t aaf_deltsqr_lsb =
		(aaf_cfg_tbl[aaf_tbl_idx].aaf_deltsqr) & MASK_ACCEL_AAF_DELTSQR_LSB;

	LOG_DBG("ACCEL_CONFIG_STATIC3 (0x%x) 0x%x", ACCEL_CONFIG_STATIC3, aaf_deltsqr_lsb);
	res = icm4268x_spi_single_write(&dev_cfg->spi, ACCEL_CONFIG_STATIC3, aaf_deltsqr_lsb);
	if (res != 0) {
		LOG_ERR("Error writing ACCEL_CONFIG_STATIC3");
		return -EINVAL;
	}

	uint8_t aaf_deltsqr_msb =
		((aaf_cfg_tbl[aaf_tbl_idx].aaf_deltsqr) >> 0x8) & MASK_ACCEL_AAF_DELTSQR_MSB;
	uint8_t aaf_bitshift =
		((aaf_cfg_tbl[aaf_tbl_idx].aaf_bitshift) << 0x4) & MASK_ACCEL_AAF_BITSHIFT;
	uint8_t aaf_cfg_static4 = (aaf_bitshift | aaf_deltsqr_msb);

	LOG_DBG("ACCEL_CONFIG_STATIC4 (0x%x) 0x%x", ACCEL_CONFIG_STATIC4, aaf_cfg_static4);
	res = icm4268x_spi_single_write(&dev_cfg->spi, ACCEL_CONFIG_STATIC4, aaf_cfg_static4);
	if (res != 0) {
		LOG_ERR("Error writing ACCEL_CONFIG_STATIC4");
		return -EINVAL;
	}

	/* Gyroscope AAF Configuration */
	aaf_tbl_idx = ((cfg->aaf_cfg[AAF_GYRO_IDX].aaf_delt) - 1);

	uint8_t aaf_cfg_static3 = ((cfg->aaf_cfg[AAF_GYRO_IDX].aaf_delt << 1) & MASK_GYRO_AAF_DELT);

	LOG_DBG("GYRO_CONFIG_STATIC3 (0x%x) 0x%x", GYRO_CONFIG_STATIC3, aaf_cfg_static3);
	res = icm4268x_spi_single_write(&dev_cfg->spi, GYRO_CONFIG_STATIC3, aaf_cfg_static3);
	if (res != 0) {
		LOG_ERR("Error writing GYRO_CONFIG_STATIC3");
		return -EINVAL;
	}

	aaf_deltsqr_lsb = (aaf_cfg_tbl[aaf_tbl_idx].aaf_deltsqr) & MASK_GYRO_AAF_DELTSQR_LSB;

	LOG_DBG("GYRO_CONFIG_STATIC4 (0x%x) 0x%x", GYRO_CONFIG_STATIC4, aaf_deltsqr_lsb);
	res = icm4268x_spi_single_write(&dev_cfg->spi, GYRO_CONFIG_STATIC4, aaf_deltsqr_lsb);
	if (res != 0) {
		LOG_ERR("Error writing GYRO_CONFIG_STATIC4");
		return -EINVAL;
	}

	aaf_deltsqr_msb =
		((aaf_cfg_tbl[aaf_tbl_idx].aaf_deltsqr) >> 0x8) & MASK_GYRO_AAF_DELTSQR_MSB;
	aaf_bitshift = ((aaf_cfg_tbl[aaf_tbl_idx].aaf_bitshift) << 0x4) & MASK_GYRO_AAF_BITSHIFT;
	uint8_t aaf_cfg_static5 = (aaf_bitshift | aaf_deltsqr_msb);

	LOG_DBG("GYRO_CONFIG_STATIC5 (0x%x) 0x%x", GYRO_CONFIG_STATIC5, aaf_cfg_static5);
	res = icm4268x_spi_single_write(&dev_cfg->spi, GYRO_CONFIG_STATIC5, aaf_cfg_static5);
	if (res != 0) {
		LOG_ERR("Error writing GYRO_CONFIG_STATIC5");
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

	tmst_config |= (cfg->tmst_dis ? 0 : BIT_TMST_EN);
	tmst_config |= (cfg->tmst_fsync_en ? BIT_TMST_FSYNC_EN : 0);
	tmst_config |= (cfg->tmst_delta_en ? BIT_TMST_DELTA_EN : 0);
	tmst_config |= (cfg->tmst_res ? BIT_TMST_RES : 0);
	tmst_config |= (cfg->tmst_regs_en ? BIT_TMST_TO_REGS_EN : 0);

	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_TMST_CONFIG, tmst_config & ~BIT(1));
	if (res != 0) {
		LOG_ERR("Error writing TMST_CONFIG");
		return -EINVAL;
	}

	/* Interface Configuration */
	uint8_t intf_config0 = FIELD_PREP(MASK_UI_SIFS_CFG, BIT_UI_SIFS_CFG_DISABLE_I2C)
				| BIT_SENSOR_DATA_ENDIAN | BIT_FIFO_COUNT_ENDIAN;

	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INTF_CONFIG0, intf_config0);
	if (res != 0) {
		LOG_ERR("Error writing INTF_CONFIG0");
		return -EINVAL;
	}

	uint8_t intf_config1;

	res = icm4268x_spi_read(&dev_cfg->spi, REG_INTF_CONFIG1, &intf_config1, 1);
	if (res != 0) {
		LOG_ERR("Error reading INTF_CONFIG1");
		return -EINVAL;
	}

	/* Disable Adaptive Full Scale Range Mode */
	intf_config1 = BIT_AFSR_SET | (intf_config1 & 0x1F);

	res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INTF_CONFIG1, intf_config1);
	if (res != 0) {
		LOG_ERR("Error writing INTF_CONFIG1");
		return -EINVAL;
	}

	/* Select the interrupt mode */
	if (IS_ENABLED(CONFIG_ICM4268X_TRIGGER)) {
		res = icm4268x_trigger_enable_interrupt(dev, cfg);
	} else {
		uint8_t int1_pol = (cfg->int1_pol ? BIT_INT1_POLARITY:0);

		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INT_CONFIG,
						BIT_INT1_DRIVE_CIRCUIT | int1_pol
						| (cfg->int1_mode ? BIT_INT1_MODE : 0));
	}

	if (res) {
		LOG_ERR("Error writing to INT_CONFIG");
		return res;
	}

	uint8_t int_config1 = 0;

	if ((cfg->accel_odr <= ICM4268X_DT_ACCEL_ODR_4000 ||
	     cfg->gyro_odr <= ICM4268X_DT_GYRO_ODR_4000)) {
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

		/* Setup desired FIFO packet fields based on the other
		 * temp/accel/gyro en fields in cfg
		 */
		/* FSYNC is not used in this driver so as per 14.45 its is not
		 * mandatory to set it to 1.
		 */
		/* Enabling watermark interrupt based on REG_INT_SOURCE0
		 * Enabling High Resolution for better accuracy
		 */
		uint8_t fifo_cfg1 =
			FIELD_PREP(BIT_FIFO_TEMP_EN, 1) | FIELD_PREP(BIT_FIFO_GYRO_EN, 1) |
			FIELD_PREP(BIT_FIFO_ACCEL_EN, 1) | FIELD_PREP(BIT_FIFO_HIRES_EN, 1) |
			FIELD_PREP(BIT_FIFO_WM_GT_TH, 1);

		LOG_DBG("FIFO_CONFIG1 (0x%x) 0x%x", REG_FIFO_CONFIG1, fifo_cfg1);
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG1, fifo_cfg1);
		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG1");
			return -EINVAL;
		}

		/* Set watermark and interrupt handling first */
		uint16_t fifo_wm = icm4268x_compute_fifo_wm(cfg);
		uint8_t fifo_wml = fifo_wm & 0xFF;

		LOG_DBG("FIFO_CONFIG2( (0x%x)) (WM Low) 0x%x", REG_FIFO_CONFIG2, fifo_wml);
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG2, fifo_wml);
		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG2");
			return -EINVAL;
		}

		uint8_t fifo_wmh = (fifo_wm >> 8) & 0x0F;

		LOG_DBG("FIFO_CONFIG3 (0x%x) (WM High) 0x%x", REG_FIFO_CONFIG3, fifo_wmh);
		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG3, fifo_wmh);
		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG3");
			return -EINVAL;
		}

		/* Begin streaming */
		uint8_t fifo_config = FIELD_PREP(MASK_FIFO_MODE, cfg->fifo_mode);

		LOG_DBG("FIFO_CONFIG (0x%x) 0x%x", REG_FIFO_CONFIG, fifo_config);

		res = icm4268x_spi_single_write(&dev_cfg->spi, REG_FIFO_CONFIG, fifo_config);

		if (res != 0) {
			LOG_ERR("Error writing FIFO_CONFIG");
			return -EINVAL;
		}

		/* Select the condition to clear the interrupt line in latched mode */
		if (cfg->int1_mode == ICM4268X_DT_INT1_MODE_LATCHED) {
			uint8_t int_config0;

			int_config0 = FIELD_PREP(MASK_FIFO_FULL_INT_CLR, cfg->fifo_full_int_clr)
				| FIELD_PREP(MASK_FIFO_THS_INT_CLEAR, cfg->fifo_ths_int_clr);

			res = icm4268x_spi_single_write(&dev_cfg->spi, REG_INT_CONFIG0,
					int_config0);
			if (res != 0) {
				LOG_ERR("Error writing INT_CONFIG0");
				return -EINVAL;
			}
		}
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
