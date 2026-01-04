/* ST Microelectronics LSM6DSVXXX family IMU sensor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv80x.pdf
 */

#include "lsm6dsv80x.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(LSM6DSVXXX, CONFIG_SENSOR_LOG_LEVEL);

static bool lsm6dsv80x_is_std_fs(uint8_t fs)
{
	return (fs < 4); /* 2g/4g/8g/16g */
}

static bool lsm6dsv80x_is_hg_fs(uint8_t fs)
{
	return (fs >= 4 && fs <= 6); /* 32g/64g/80g */
}

/*
 * XL configuration
 */

static uint16_t lsm6dsv80x_accel_gain_ug(uint8_t fs)
{
	return (61 * (1 << fs));
}

/* The first nibble of fs tells if it is High-G or not */
static int lsm6dsv80x_accel_range_to_fs_val(const struct device *dev, int32_t range, uint8_t *fs)
{
	switch (range) {
	case LSM6DSV80X_DT_FS_2G:
		*fs = 0;
		break;

	case LSM6DSV80X_DT_FS_4G:
		*fs = 1;
		break;

	case LSM6DSV80X_DT_FS_8G:
		*fs = 2;
		break;

	case LSM6DSV80X_DT_FS_16G:
		*fs = 3;
		break;

	case LSM6DSV80X_DT_FS_32G:
		*fs = 4;
		break;

	case LSM6DSV80X_DT_FS_64G:
		*fs = 5;
		break;

	case LSM6DSV80X_DT_FS_80G:
		*fs = 6;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int lsm6dsv80x_accel_set_fs_raw(const struct device *dev, uint8_t fs)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsvxxx_data *data = dev->data;

	if (lsm6dsv80x_is_std_fs(fs)) { /* 2g/4g/8g/16g */
		lsm6dsv80x_xl_full_scale_t val = fs;

		if (lsm6dsv80x_xl_full_scale_set(ctx, val) < 0) {
			return -EIO;
		}

		data->out_xl = LSM6DSV80X_OUTX_L_A;
	} else if (lsm6dsv80x_is_hg_fs(fs)) { /* 32g/64g/80g */
		lsm6dsv80x_hg_xl_full_scale_t val = (fs - 4);

		if (lsm6dsv80x_hg_xl_full_scale_set(ctx, val) < 0) {
			return -EIO;
		}

		data->out_xl = LSM6DSV80X_UI_OUTX_L_A_HG;
	} else {
		return -EINVAL;
	}

	data->accel_fs = fs;
	data->acc_gain = lsm6dsv80x_accel_gain_ug(fs);

	return 0;
}

static int lsm6dsv80x_accel_set_fs(const struct device *dev, int32_t range)
{
	uint8_t fs;
	int ret;

	ret = lsm6dsv80x_accel_range_to_fs_val(dev, range, &fs);
	if (ret < 0) {
		return ret;
	}

	ret = lsm6dsv80x_accel_set_fs_raw(dev, fs);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int lsm6dsv80x_accel_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsvxxx_data *data = dev->data;

	if (cfg->accel_hg_odr != LSM6DSV80X_HG_XL_ODR_OFF) {

		if (lsm6dsv80x_hg_xl_data_rate_set(ctx, cfg->accel_hg_odr, 1) < 0) {
			return -EIO;
		}
	} else {
		if (lsm6dsv80x_xl_data_rate_set(ctx, odr) < 0) {
			return -EIO;
		}
	}

	data->accel_freq = odr;

	return 0;
}

/*
 * values taken from lsm6dsv80x_data_rate_t in hal/st module. The mode/accuracy
 * should be selected through accel-odr property in DT
 */
static const float lsm6dsv80x_odr_map[3][13] = {
	/* High Accuracy off */
	{0.0f, 1.875f, 7.5f, 15.0f, 30.0f, 60.0f, 120.0f, 240.0f, 480.0f, 960.0f, 1920.0f, 3840.0f,
	 7680.0f},

	/* High Accuracy 1 */
	{0.0f, 1.875f, 7.5f, 15.625f, 31.25f, 62.5f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f,
	 4000.0f, 8000.0f},

	/* High Accuracy 2 */
	{0.0f, 1.875f, 7.5f, 12.5f, 25.0f, 50.0f, 100.0f, 200.0f, 400.0f, 800.0f, 1600.0f, 800.0f,
	 6400.0f},
};

static uint8_t lsm6dsv80x_freq_to_odr_val(const struct device *dev, int32_t freq)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsv80x_data_rate_t odr;
	int8_t mode;
	size_t i;

	if (lsm6dsv80x_xl_data_rate_get(ctx, &odr) < 0) {
		return -EINVAL;
	}

	mode = (odr >> 4) & 0xf;

	for (i = 0; i < ARRAY_SIZE(lsm6dsv80x_odr_map[mode]); i++) {
		if (freq <= lsm6dsv80x_odr_map[mode][i]) {
			LOG_DBG("mode: %d - odr: %d", mode, i);
			return i | (mode << 4);
		}
	}

	return 0xFF;
}

static int lsm6dsv80x_accel_set_odr(const struct device *dev, int32_t freq)
{
	uint8_t odr;

	odr = lsm6dsv80x_freq_to_odr_val(dev, freq);
	if (odr == 0xFF) {
		return -EINVAL;
	}

	if (lsm6dsv80x_accel_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}

static int32_t lsm6dsv80x_accel_set_mode(const struct device *dev, int32_t mode)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	switch (mode) {
	case 0: /* High Performance */
		mode = LSM6DSV80X_XL_HIGH_PERFORMANCE_MD;
		break;
	case 1: /* High Accuracy */
		mode = LSM6DSV80X_XL_HIGH_ACCURACY_ODR_MD;
		break;
	case 3: /* ODR triggered */
		mode = LSM6DSV80X_XL_ODR_TRIGGERED_MD;
		break;
	case 4: /* Low Power 2 */
		mode = LSM6DSV80X_XL_LOW_POWER_2_AVG_MD;
		break;
	case 5: /* Low Power 4 */
		mode = LSM6DSV80X_XL_LOW_POWER_4_AVG_MD;
		break;
	case 6: /* Low Power 8 */
		mode = LSM6DSV80X_XL_LOW_POWER_8_AVG_MD;
		break;
	case 7: /* Normal */
		mode = LSM6DSV80X_XL_NORMAL_MD;
		break;
	default:
		return -EIO;
	}

	return lsm6dsv80x_xl_mode_set(ctx, mode);
}

static int32_t lsm6dsv80x_accel_get_fs(const struct device *dev, int32_t *range)
{
	return -ENOTSUP;
}

static int32_t lsm6dsv80x_accel_get_odr(const struct device *dev, int32_t *freq)
{
	return -ENOTSUP;
}

static int32_t lsm6dsv80x_accel_get_mode(const struct device *dev, int32_t *mode)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsv80x_xl_mode_t md;

	lsm6dsv80x_xl_mode_get(ctx, &md);

	switch (md) {
	case LSM6DSV80X_XL_HIGH_PERFORMANCE_MD:
		*mode = 0;
		break;
	case LSM6DSV80X_XL_HIGH_ACCURACY_ODR_MD:
		*mode = 1;
		break;
	case LSM6DSV80X_XL_ODR_TRIGGERED_MD:
		*mode = 3;
		break;
	case LSM6DSV80X_XL_LOW_POWER_2_AVG_MD:
		*mode = 4;
		break;
	case LSM6DSV80X_XL_LOW_POWER_4_AVG_MD:
		*mode = 5;
		break;
	case LSM6DSV80X_XL_LOW_POWER_8_AVG_MD:
		*mode = 6;
		break;
	case LSM6DSV80X_XL_NORMAL_MD:
		*mode = 7;
		break;
	default:
		return -EIO;
	}

	return 0;
}

/*
 * GY configuration
 */

static int lsm6dsv80x_gyro_range_to_fs_val(const struct device *dev, int32_t range, uint8_t *fs)
{
	switch (range) {
	case 0:
		*fs = 0;
		break;

	case 250:
		*fs = LSM6DSV80X_DT_FS_250DPS;
		break;

	case 500:
		*fs = LSM6DSV80X_DT_FS_500DPS;
		break;

	case 1000:
		*fs = LSM6DSV80X_DT_FS_1000DPS;
		break;

	case 2000:
		*fs = LSM6DSV80X_DT_FS_2000DPS;
		break;

	case 4000:
		*fs = LSM6DSV80X_DT_FS_4000DPS;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static uint32_t lsm6dsv80x_gyro_gain_udps(uint8_t fs)
{
	return (4375 * (1 << fs));
}

static int lsm6dsv80x_gyro_set_fs_raw(const struct device *dev, uint8_t fs)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsvxxx_data *data = dev->data;

	if (fs == 0) {
		/* skip power-up value */
		return 0;
	}

	if (lsm6dsv80x_gy_full_scale_set(ctx, fs) < 0) {
		return -EIO;
	}

	data->gyro_fs = fs;
	data->gyro_gain = lsm6dsv80x_gyro_gain_udps(fs);
	return 0;
}

static int lsm6dsv80x_gyro_set_fs(const struct device *dev, int32_t range)
{
	uint8_t fs;
	int ret;

	ret = lsm6dsv80x_gyro_range_to_fs_val(dev, range, &fs);
	if (ret < 0) {
		return ret;
	}

	if (lsm6dsv80x_gyro_set_fs_raw(dev, fs) < 0) {
		return -EIO;
	}

	return 0;
}

static int lsm6dsv80x_gyro_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm6dsvxxx_data *data = dev->data;

	if (lsm6dsv80x_gy_data_rate_set(ctx, odr) < 0) {
		return -EIO;
	}

	data->gyro_freq = odr;
	return 0;
}

static int lsm6dsv80x_gyro_set_odr(const struct device *dev, int32_t freq)
{
	uint8_t odr;

	odr = lsm6dsv80x_freq_to_odr_val(dev, freq);
	if (odr == 0xFF) {
		return -EINVAL;
	}

	if (lsm6dsv80x_gyro_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set gyroscope sampling rate");
		return -EIO;
	}

	return 0;
}

static int32_t lsm6dsv80x_gyro_set_mode(const struct device *dev, int32_t mode)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	switch (mode) {
	case 0: /* High Performance */
		mode = LSM6DSV80X_GY_HIGH_PERFORMANCE_MD;
		break;
	case 1: /* High Accuracy */
		mode = LSM6DSV80X_GY_HIGH_ACCURACY_ODR_MD;
		break;
	case 4: /* Sleep */
		mode = LSM6DSV80X_GY_SLEEP_MD;
		break;
	case 5: /* Low Power */
		mode = LSM6DSV80X_GY_LOW_POWER_MD;
		break;
	default:
		return -EIO;
	}

	return lsm6dsv80x_gy_mode_set(ctx, mode);
}

static int32_t lsm6dsv80x_gyro_get_fs(const struct device *dev, int32_t *range)
{
	return -ENOTSUP;
}

static int32_t lsm6dsv80x_gyro_get_odr(const struct device *dev, int32_t *freq)
{
	return -ENOTSUP;
}

static int32_t lsm6dsv80x_gyro_get_mode(const struct device *dev, int32_t *mode)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm6dsv80x_gy_mode_t md;

	lsm6dsv80x_gy_mode_get(ctx, &md);

	switch (md) {
	case LSM6DSV80X_GY_HIGH_PERFORMANCE_MD:
		*mode = 0;
		break;
	case LSM6DSV80X_GY_HIGH_ACCURACY_ODR_MD:
		*mode = 1;
		break;
	case LSM6DSV80X_GY_SLEEP_MD:
		*mode = 4;
		break;
	case LSM6DSV80X_GY_LOW_POWER_MD:
		*mode = 5;
		break;
	default:
		return -EIO;
	}

	return 0;
}

#if defined(CONFIG_LSM6DSVXXX_TRIGGER)
int32_t lsm6dsv80x_drdy_mode_set(const struct device *dev)
{
	const struct lsm6dsvxxx_config *config = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&config->ctx;

	/* enable drdy on int1/int2 in pulse mode */
	lsm6dsv80x_data_ready_mode_t drdy =
		(config->drdy_pulsed) ? LSM6DSV80X_DRDY_PULSED : LSM6DSV80X_DRDY_LATCHED;
	if (lsm6dsv80x_data_ready_mode_set(ctx, drdy)) {
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_LSM6DSVXXX_TRIGGER */

/* init routine */
static int lsm6dsv80x_init_chip(const struct device *dev)
{
	const struct lsm6dsvxxx_config *cfg = dev->config;
	struct lsm6dsvxxx_data *data = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t chip_id;
	uint8_t odr, fs;

#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (cfg->i3c.bus != NULL) {
		/*
		 * Need to grab the pointer to the I3C device descriptor
		 * before we can talk to the sensor.
		 */
		lsm6dsvxxx->i3c_dev = i3c_device_find(cfg->i3c.bus, &cfg->i3c.dev_id);
		if (lsm6dsvxxx->i3c_dev == NULL) {
			LOG_ERR("Cannot find I3C device descriptor");
			return -ENODEV;
		}
	}
#endif

	/* All registers except 0x01 are different between banks, including the WHO_AM_I
	 * register and the register used for a SW reset.  If the lsm6dsvxxx wasn't on the user
	 * bank when it reset, then both the chip id check and the sw reset will fail unless we
	 * set the bank now.
	 */
	if (lsm6dsv80x_mem_bank_set(ctx, LSM6DSV80X_MAIN_MEM_BANK) < 0) {
		LOG_DBG("Failed to set user bank");
		return -EIO;
	}

	if (lsm6dsv80x_device_id_get(ctx, &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
		return -EIO;
	}

	LOG_INF("chip id 0x%x", chip_id);

	if (chip_id != LSM6DSV80X_ID) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/* Resetting the whole device while using I3C will also reset the DA, therefore perform
	 * only a software reset if the bus is I3C. It should be assumed that the device was
	 * already fully reset by the I3C CCC RSTACT (whole chip) done as apart of the I3C Bus
	 * initialization.
	 */
	if (ON_I3C_BUS(cfg)) {
		/* Restore default configuration */
		lsm6dsv80x_reboot(ctx);

		/* wait 150us as reported in AN5763 */
		k_sleep(K_USEC(150));
	} else {
		/* reset device (sw_por) */
		if (lsm6dsv80x_sw_por(ctx) < 0) {
			return -EIO;
		}

		/* wait 30ms as reported in AN5763 */
		k_sleep(K_MSEC(30));
	}

	data->out_xl = LSM6DSV80X_OUTX_L_A;
	data->out_tp = LSM6DSV80X_OUT_TEMP_L;

	fs = cfg->accel_range;
	LOG_DBG("accel range is %d", fs);
	if (lsm6dsv80x_accel_set_fs_raw(dev, fs) < 0) {
		LOG_ERR("failed to set accelerometer range %d", fs);
		return -EIO;
	}

	odr = cfg->accel_odr;
	LOG_DBG("accel odr is %d", odr);
	if (lsm6dsv80x_accel_set_odr_raw(dev, odr) < 0) {
		LOG_ERR("failed to set accelerometer odr %d", odr);
		return -EIO;
	}

	fs = cfg->gyro_range;
	LOG_DBG("gyro range is %d", fs);
	if (lsm6dsv80x_gyro_set_fs_raw(dev, fs) < 0) {
		LOG_ERR("failed to set gyroscope range %d", fs);
		return -EIO;
	}

	odr = cfg->gyro_odr;
	LOG_DBG("gyro odr is %d", odr);
	if (lsm6dsv80x_gyro_set_odr_raw(dev, odr) < 0) {
		LOG_ERR("failed to set gyroscope odr %d", odr);
		return -EIO;
	}

#if LSM6DSVXXX_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (IS_ENABLED(CONFIG_LSM6DSVXXX_STREAM) && (ON_I3C_BUS(cfg))) {
		/*
		 * Set MRL to the Max Size of the FIFO so the entire FIFO can be read
		 * out at once
		 */
		struct i3c_ccc_mrl setmrl = {
			.len = 0x0700,
			.ibi_len = lsm6dsvxxx->i3c_dev->data_length.max_ibi,
		};
		if (i3c_ccc_do_setmrl(lsm6dsvxxx->i3c_dev, &setmrl) < 0) {
			LOG_ERR("failed to set mrl");
			return -EIO;
		}
	}
#endif

	if (lsm6dsv80x_block_data_update_set(ctx, 1) < 0) {
		LOG_DBG("failed to set BDU mode");
		return -EIO;
	}

	return 0;
}

#if defined(CONFIG_PM_DEVICE)
static int lsm6dsv80x_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct lsm6dsvxxx_data *data = dev->data;
	const struct lsm6dsvxxx_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret = 0;

	LOG_DBG("PM action: %d", (int)action);

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		if (lsm6dsv80x_xl_data_rate_set(ctx, data->accel_freq) < 0) {
			LOG_ERR("failed to set accelerometer odr %d", (int)data->accel_freq);
			ret = -EIO;
		}
		if (lsm6dsv80x_gy_data_rate_set(ctx, data->gyro_freq) < 0) {
			LOG_ERR("failed to set gyroscope odr %d", (int)data->gyro_freq);
			ret = -EIO;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (lsm6dsv80x_xl_data_rate_set(ctx, LSM6DSVXXX_DT_ODR_OFF) < 0) {
			LOG_ERR("failed to disable accelerometer");
			ret = -EIO;
		}
		if (lsm6dsv80x_gy_data_rate_set(ctx, LSM6DSVXXX_DT_ODR_OFF) < 0) {
			LOG_ERR("failed to disable gyroscope");
			ret = -EIO;
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

#if defined(CONFIG_LSM6DSVXXX_STREAM)
static void lsm6dsv80x_config_fifo(const struct device *dev, struct trigger_config trig_cfg)
{
	struct lsm6dsvxxx_data *data = dev->data;
	const struct lsm6dsvxxx_config *config = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&config->ctx;
	uint8_t fifo_wtm = 0;
	lsm6dsv80x_pin_int_route_t pin_int = {0};
	lsm6dsv80x_fifo_xl_batch_t xl_batch = LSM6DSVXXX_DT_XL_NOT_BATCHED;
	lsm6dsv80x_fifo_gy_batch_t gy_batch = LSM6DSVXXX_DT_GY_NOT_BATCHED;
	lsm6dsv80x_fifo_temp_batch_t temp_batch = LSM6DSVXXX_DT_TEMP_NOT_BATCHED;
	lsm6dsv80x_fifo_mode_t fifo_mode = LSM6DSV80X_BYPASS_MODE;
	lsm6dsv80x_sflp_data_rate_t sflp_odr = LSM6DSV80X_SFLP_120Hz;
	lsm6dsv80x_fifo_sflp_raw_t sflp_fifo = {0};
	lsm6dsv80x_sflp_gbias_t gbias;
	uint8_t xl_hg_batch = 0;

	/* disable FIFO as first thing */
	lsm6dsv80x_fifo_mode_set(ctx, LSM6DSV80X_BYPASS_MODE);

	pin_int.fifo_th = PROPERTY_DISABLE;
	pin_int.fifo_full = PROPERTY_DISABLE;

	if (trig_cfg.int_fifo_th || trig_cfg.int_fifo_full) {
		pin_int.fifo_th = (trig_cfg.int_fifo_th) ? PROPERTY_ENABLE : PROPERTY_DISABLE;
		pin_int.fifo_full = (trig_cfg.int_fifo_full) ? PROPERTY_ENABLE : PROPERTY_DISABLE;

		xl_batch = config->accel_batch;
		gy_batch = config->gyro_batch;
		temp_batch = config->temp_batch;
		xl_hg_batch = (lsm6dsv80x_is_hg_fs(config->accel_range)) ? 1 : 0;

		fifo_mode = LSM6DSV80X_STREAM_MODE;
		fifo_wtm = config->fifo_wtm;

		if (config->sflp_fifo_en & LSM6DSVXXX_DT_SFLP_FIFO_GAME_ROTATION) {
			sflp_fifo.game_rotation = 1;
		}

		if (config->sflp_fifo_en & LSM6DSVXXX_DT_SFLP_FIFO_GRAVITY) {
			sflp_fifo.gravity = 1;
		}

		if (config->sflp_fifo_en & LSM6DSVXXX_DT_SFLP_FIFO_GBIAS) {
			sflp_fifo.gbias = 1;
		}

		sflp_odr = config->sflp_odr;
	}

	/*
	 * Set FIFO watermark (number of unread sensor data TAG + 6 bytes
	 * stored in FIFO) to FIFO_WATERMARK samples
	 */
	lsm6dsv80x_fifo_watermark_set(ctx, config->fifo_wtm);

	/* Turn on/off FIFO */
	lsm6dsv80x_fifo_mode_set(ctx, fifo_mode);

	/* Set FIFO batch rates */
	lsm6dsv80x_fifo_xl_batch_set(ctx, xl_batch);
	data->accel_batch_odr = xl_batch;
	lsm6dsv80x_fifo_hg_xl_batch_set(ctx, xl_hg_batch);
	lsm6dsv80x_fifo_gy_batch_set(ctx, gy_batch);
	data->gyro_batch_odr = gy_batch;
#if defined(CONFIG_LSM6DSVXXX_ENABLE_TEMP)
	lsm6dsv80x_fifo_temp_batch_set(ctx, temp_batch);
	data->temp_batch_odr = temp_batch;
#endif

	lsm6dsv80x_sflp_data_rate_set(ctx, sflp_odr);
	data->sflp_batch_odr = sflp_odr;
	lsm6dsv80x_fifo_sflp_batch_set(ctx, sflp_fifo);
	lsm6dsv80x_sflp_game_rotation_set(ctx, PROPERTY_ENABLE);

	/*
	 * Temporarly set Accel and gyro odr same as sensor fusion LP in order to
	 * make the SFLP gbias setting effective. Then restore it to saved values.
	 */
	switch (sflp_odr) {
	case LSM6DSVXXX_DT_SFLP_ODR_AT_480Hz:
		lsm6dsv80x_accel_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_480Hz);
		lsm6dsv80x_gyro_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_480Hz);
		break;

	case LSM6DSVXXX_DT_SFLP_ODR_AT_240Hz:
		lsm6dsv80x_accel_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_240Hz);
		lsm6dsv80x_gyro_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_240Hz);
		break;

	case LSM6DSVXXX_DT_SFLP_ODR_AT_120Hz:
		lsm6dsv80x_accel_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_120Hz);
		lsm6dsv80x_gyro_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_120Hz);
		break;

	case LSM6DSVXXX_DT_SFLP_ODR_AT_60Hz:
		lsm6dsv80x_accel_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_60Hz);
		lsm6dsv80x_gyro_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_60Hz);
		break;

	case LSM6DSVXXX_DT_SFLP_ODR_AT_30Hz:
		lsm6dsv80x_accel_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_30Hz);
		lsm6dsv80x_gyro_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_30Hz);
		break;

	case LSM6DSVXXX_DT_SFLP_ODR_AT_15Hz:
	default:
		lsm6dsv80x_accel_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_15Hz);
		lsm6dsv80x_gyro_set_odr_raw(dev, LSM6DSVXXX_DT_ODR_AT_15Hz);
		break;
	}

	/* set sflp gbias */
	gbias.gbias_x = (float)data->gbias_x_udps / 1000000;
	gbias.gbias_y = (float)data->gbias_y_udps / 1000000;
	gbias.gbias_z = (float)data->gbias_z_udps / 1000000;
	lsm6dsv80x_sflp_game_gbias_set(ctx, &gbias);

	/* restore accel/gyro odr to saved values */
	lsm6dsv80x_accel_set_odr_raw(dev, data->accel_freq);
	lsm6dsv80x_gyro_set_odr_raw(dev, data->gyro_freq);

	/* Set pin interrupt (fifo_th could be on or off) */
	if ((config->drdy_pin == 1) || (ON_I3C_BUS(config) && (!I3C_INT_PIN(config)))) {
		lsm6dsv80x_pin_int1_route_set(ctx, &pin_int);
	} else {
		lsm6dsv80x_pin_int2_route_set(ctx, &pin_int);
	}
}

static void lsm6dsv80x_config_drdy(const struct device *dev, struct trigger_config trig_cfg)
{
	const struct lsm6dsvxxx_config *config = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&config->ctx;
	lsm6dsv80x_pin_int_route_t pin_int = {0};

	pin_int.drdy_xl = (trig_cfg.int_drdy) ? PROPERTY_ENABLE : PROPERTY_DISABLE;

	/* Set pin interrupt (fifo_th could be on or off) */
	if ((config->drdy_pin == 1) || (ON_I3C_BUS(config) && (!I3C_INT_PIN(config)))) {
		lsm6dsv80x_pin_int1_route_set(ctx, &pin_int);
	} else {
		lsm6dsv80x_pin_int2_route_set(ctx, &pin_int);
	}
}
#endif /* CONFIG_LSM6DSVXXX_STREAM */

const struct lsm6dsvxxx_chip_api st_lsm6dsv80x_chip_api = {
	.init_chip = lsm6dsv80x_init_chip,
#if defined(CONFIG_LSM6DSVXXX_TRIGGER)
	.drdy_mode_set = lsm6dsv80x_drdy_mode_set,
#endif /* CONFIG_LSM6DSVXXX_TRIGGER */
#if defined(CONFIG_PM_DEVICE)
	.pm_action = lsm6dsv80x_pm_action,
#endif /* CONFIG_PM_DEVICE */
	.accel_fs_set = lsm6dsv80x_accel_set_fs,
	.accel_odr_set = lsm6dsv80x_accel_set_odr,
	.accel_mode_set = lsm6dsv80x_accel_set_mode,
	.accel_fs_get = lsm6dsv80x_accel_get_fs,
	.accel_odr_get = lsm6dsv80x_accel_get_odr,
	.accel_mode_get = lsm6dsv80x_accel_get_mode,
	.gyro_fs_set = lsm6dsv80x_gyro_set_fs,
	.gyro_odr_set = lsm6dsv80x_gyro_set_odr,
	.gyro_mode_set = lsm6dsv80x_gyro_set_mode,
	.gyro_fs_get = lsm6dsv80x_gyro_get_fs,
	.gyro_odr_get = lsm6dsv80x_gyro_get_odr,
	.gyro_mode_get = lsm6dsv80x_gyro_get_mode,
#if defined(CONFIG_LSM6DSVXXX_STREAM)
	.config_fifo = lsm6dsv80x_config_fifo,
	.config_drdy = lsm6dsv80x_config_drdy,
	.from_f16_to_f32 = lsm6dsv80x_from_f16_to_f32,
	.from_sflp_to_mg = lsm6dsv80x_from_sflp_to_mg,
#endif /* CONFIG_LSM6DSVXXX_STREAM */
};

/* bit shift for Accelerometer for a given range value */
const int8_t st_lsm6dsv80x_accel_bit_shift[] = {
	5, /* FS_2G */
	6, /* FS_4G */
	7, /* FS_8G */
	8, /* FS_16G */
	9, /* FS_32G */
	10, /* FS_64G */
	11, /* FS_80G */
};

/*
 * Accelerometer scaling factors table for a given range value
 * GAIN_UNIT_XL is expressed in ug/LSB.
 */
const int32_t st_lsm6dsv80x_accel_scaler[] = {
	SENSOR_SCALE_UG_TO_UMS2(61),    /* FS_2G */
	SENSOR_SCALE_UG_TO_UMS2(122),   /* FS_4G */
	SENSOR_SCALE_UG_TO_UMS2(244),   /* FS_8G */
	SENSOR_SCALE_UG_TO_UMS2(488),   /* FS_16G */
	SENSOR_SCALE_UG_TO_UMS2(976),   /* FS_32G */
	SENSOR_SCALE_UG_TO_UMS2(1952),  /* FS_64G */
	SENSOR_SCALE_UG_TO_UMS2(3904),  /* FS_80G */
};
