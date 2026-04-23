/*
 * Bosch BMA400 3-axis accelerometer driver
 * SPDX-FileCopyrightText: Copyright 2026 Luca Gessi lucagessi90@gmail.com
 * SPDX-FileCopyrightText: Copyright 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma400

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/rtio/work.h>

#include "bma400.h"
#include "bma400_interrupt.h"
#include "bma400_decoder.h"
#include "bma400_rtio.h"

LOG_MODULE_REGISTER(bma400, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_BMA400_STREAM

static uint16_t bma400_compute_fifo_wm(const struct bma400_runtime_config *new_cfg)
{
	int32_t bytes;

	const int pkt_size = BMA400_FIFO_A_LENGTH + BMA400_FIFO_HEADER_LENGTH;

	/* Convert batch_ticks to bytes per batch */
	bytes = (int64_t)new_cfg->batch_ticks * pkt_size;

	return (uint16_t)MIN(bytes, 0x7ff);
}

#endif /* CONFIG_BMA400_STREAM */

int bma400_configure(const struct device *dev, struct bma400_runtime_config *cfg)
{
	struct bma400_data *dev_data = dev->data;
	int res = 0;
	uint8_t value;
	uint8_t power_mode_conf = 0;

	/* If ODR is greater than 25Hz, enable normal mode */
	power_mode_conf = BMA400_MODE_NORMAL;
	/* Configure ACC_CONFIG0: bandwidth, power mode */
	value = power_mode_conf;
	value |= FIELD_PREP(BMA400_FILT_1_BW_MSK, cfg->accel_bwp);
	value |= FIELD_PREP(BMA400_OSR_LP_MSK, cfg->osr_lp);
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_ACC_CONFIG0, value);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_ACC_CONFIG0", __func__);
		return res;
	}

	/* Configure ACC_CONFIG1: range and ODR */
	value = cfg->accel_odr;
	value |= FIELD_PREP(BMA400_ACCEL_RANGE_MSK, cfg->accel_fs_range);
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_ACC_CONFIG1, value);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_ACC_CONFIG1", __func__);
		return res;
	}

#ifdef CONFIG_BMA400_WAKEUP
	/* Configure wakeup function if enabled in device tree
	 *	- WKUP_INT_CONFIG0.wkup_refu: everytime 00 or 11
	 *	- WKUP_INT_CONFIG0. num_of_samples: number of samples above threshold + 1
	 *	- WKUP_INT_CONFIG0.wkup_X/Y/Z_en: enable wakeup interrupt for axes X/Y/Z
	 *	- WKUP_INT_CONFIG1.int_wkup_thres: most significant 8 bits of the 12 bits
	 * measurement
	 *	- low_power_timeout (AUTOLOWPOW_1.auto_lp_timeout =b01): the sensor is set into
	 *		low-power mode as soon the timeout counter reaches
	 *		AUTOLOWPOW_1.auto_lp_timeout_thres.
	 *	- low_power_timeout with counter reset on activity detected
	 *		(AUTOLOWPOW_1.auto_lp_timeout =b10,b11): the timeout counter is restarted in
	 *		case generic interrupt 2 (see chapter 4.7) is asserted.
	 */
	if (cfg->auto_wakeup_en) {
		value = 0x01 | BMA400_WAKEUP_EN_AXES_MSK |
			FIELD_PREP(BMA400_SAMPLE_COUNT_MSK, cfg->wakeup_num_of_samples);
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_WKUP_INT_CONFIG0, value);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_WKUP_INT_CONFIG0", __func__);
			return res;
		}
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_WKUP_INT_CONFIG1,
						  cfg->auto_wakeup_threshold);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_WKUP_INT_CONFIG1", __func__);
			return res;
		}
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_AUTOWAKEUP_1,
						  BMA400_WAKEUP_INTERRUPT_MSK);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_AUTOWAKEUP_1", __func__);
			return res;
		}
	}
	/* Enable inactivity recognition on GEN1_INT for auto lowpower mode transition */
	if (cfg->auto_lowpower_en) {
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_AUTOLOWPOW_1,
						  BMA400_AUTO_LP_GEN1_TRIGGER);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_AUTOLOWPOW_1", __func__);
			return res;
		}
		/* Prepare generic interrupt 1 configuration */
		/* Enable all axes for motion detection */
		value = BMA400_INT_AXES_EN_MSK;
		/* Set hysteresis to 0mg */
		value |= BMA400_HYST_0_MG;
		/* Use 1Hz acceleration as reference value for motion detection */
		value |= FIELD_PREP(BMA400_INT_REFU_MSK, BMA400_UPDATE_LP_EVERY_TIME);

		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_GEN1INT_CONFIG0, value);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_GEN1INT_CONFIG0", __func__);
			return res;
		}

		/* Enable criterion activity recognition*/
		res = dev_data->hw_ops->write_reg(
			dev, BMA400_REG_GEN1INT_CONFIG1,
			FIELD_PREP(BMA400_GEN_INT_CRITERION_MSK, BMA400_INACTIVITY_INT));
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_GEN1INT_CONFIG1", __func__);
			return res;
		}

		/* Set motion detection threshold */
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_GEN1INT_CONFIG2,
						  cfg->auto_lowpower_threshold);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_GEN1INT_CONFIG2", __func__);
			return res;
		}

		/* Set motion detection duration */
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_GEN1INT_CONFIG3,
						  (cfg->auto_lowpower_duration & 0xff00) >> 8);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_GEN1INT_CONFIG3", __func__);
			return res;
		}
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_GEN1INT_CONFIG31,
						  cfg->auto_lowpower_duration & 0xFF);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_GEN1INT_CONFIG31", __func__);
			return res;
		}
	}
	if (cfg->auto_lowpower_en) {
		value = BMA400_EN_GEN1_MSK; /* Enable low power interrupt engine */
	} else {
		value = 0;
	}
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_INT_CONFIG0, value);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_INT_CONFIG0", __func__);
		return res;
	}
#else
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_INT_CONFIG0, 0);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_INT_CONFIG0", __func__);
		return res;
	}
#endif

	return 0;
}

#ifdef CONFIG_BMA400_STREAM

static inline int bma400_stream_configure_fifo(const struct device *dev,
					       struct bma400_runtime_config *cfg)
{
	struct bma400_data *dev_data = dev->data;
	int res = 0;

	if (cfg->fifo_wm_en || cfg->fifo_full_en) {
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_FIFO_CONFIG0,
						  BMA400_FIFO_X_EN | BMA400_FIFO_Y_EN |
							  BMA400_FIFO_Z_EN |
							  BMA400_FIFO_AUTO_FLUSH);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_FIFO_CONFIG0", __func__);
			return res;
		}
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_FIFO_CONFIG1,
						  cfg->fifo_wm & 0xFF);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_FIFO_CONFIG1", __func__);
			return res;
		}
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_FIFO_CONFIG2,
						  (cfg->fifo_wm >> 8) & 0x07);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_FIFO_CONFIG2", __func__);
			return res;
		}
	} else {
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_FIFO_CONFIG0, 0);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_FIFO_CONFIG0", __func__);
			return res;
		}
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_FIFO_CONFIG1, 0);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_FIFO_CONFIG1", __func__);
			return res;
		}
		res = dev_data->hw_ops->write_reg(dev, BMA400_REG_FIFO_CONFIG2, 0);
		if (res) {
			LOG_ERR("%s could not write BMA400_REG_FIFO_CONFIG2", __func__);
			return res;
		}
	}
	return 0;
}

int bma400_stream_configure(const struct device *dev, struct bma400_runtime_config *cfg)
{
	struct bma400_data *dev_data = dev->data;
	int res;
	uint8_t value = 0;

	/* Disable interrupts, reconfigured at end */
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_INT_CONFIG0, 0);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_INT_CONFIG0", __func__);
		return res;
	}
	/* BMA400_REG_INT_CONFIG1 contains latch bit (1 enables latching) */
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_INT_CONFIG1, 0);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_INT_CONFIG1", __func__);
		return res;
	}
	/* FIFO and motion detection mapping is done in stream specific functions */
	/* Map all interrupts on INT1 */
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_INT1_MAP, cfg->int1_map);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_INT1_MAP", __func__);
		return res;
	}
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_INT2_MAP, cfg->int2_map);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_INT2_MAP", __func__);
		return res;
	}
#ifdef CONFIG_BMA400_WAKEUP
	if (cfg->auto_lowpower_en) {
		value = BMA400_EN_GEN1_MSK; /* Enable sleep interrupt engine */
	}
#endif
	if (cfg->motion_detection_en) {
		value |= BMA400_EN_GEN2_MSK;
	}
	if (cfg->fifo_wm_en) {
		value |= BMA400_EN_FIFO_WM_MSK;
	}
	if (cfg->fifo_full_en) {
		value |= BMA400_EN_FIFO_FULL_MSK;
	}

	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_INT_CONFIG0, value);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_INT_CONFIG0", __func__);
		return res;
	}
	/* Prepare generic interrupt 2 configuration */
	value = BMA400_INT_AXES_EN_MSK; /* Enable all axes for motion detection */
	value |= BMA400_HYST_96_MG;     /* Set hysteresis to 96mg */
	/* Use 1Hz acceleration as ref value for motion detection */
	value |= FIELD_PREP(BMA400_INT_REFU_MSK, BMA400_UPDATE_LP_EVERY_TIME);
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_GEN2INT_CONFIG0, value);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_GEN2INT_CONFIG0", __func__);
		return res;
	}

	/* Enable criterion activity recognition*/
	res = dev_data->hw_ops->write_reg(
		dev, BMA400_REG_GEN2INT_CONFIG1,
		FIELD_PREP(BMA400_GEN_INT_CRITERION_MSK, BMA400_ACTIVITY_INT));
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_GEN2INT_CONFIG1", __func__);
		return res;
	}

	/* Set motion detection threshold */
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_GEN2INT_CONFIG2, cfg->motion_threshold);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_GEN2INT_CONFIG2", __func__);
		return res;
	}

	/* Set motion detection duration */
	res = dev_data->hw_ops->write_reg(dev, BMA400_REG_GEN2INT_CONFIG31, cfg->motion_duration);
	if (res) {
		LOG_ERR("%s could not write BMA400_REG_GEN2INT_CONFIG31", __func__);
		return res;
	}

	/* Configure FIFO */
	return bma400_stream_configure_fifo(dev, cfg);
}
#endif /* CONFIG_BMA400_STREAM */

int bma400_safely_configure(const struct device *dev, struct bma400_runtime_config *cfg)
{
	struct bma400_data *drv_data = dev->data;
	int ret = bma400_configure(dev, cfg);

	if (ret == 0) {
		memcpy(&drv_data->cfg, cfg, sizeof(struct bma400_runtime_config));
	} else {
		ret = bma400_configure(dev, &drv_data->cfg);
	}

	return ret;
}

/**
 * @brief Map of ODR register values to their corresponding rates in microhertz
 */
static const uint32_t odr_to_reg_map[] = {
	0,         /* Invalid */
	0,         /* Invalid */
	0,         /* Invalid */
	0,         /* Invalid */
	0,         /* Invalid */
	12500000,  /* 12.5 Hz (25/2) => 0x5 */
	25000000,  /* 25 Hz => 0x6 */
	50000000,  /* 50 Hz => 0x7*/
	100000000, /* 100 Hz => 0x8*/
	200000000, /* 200 Hz => 0x9*/
	400000000, /* 400 Hz => 0xa*/
	800000000, /* 800 Hz => 0xb*/
};

/**
 * @brief Convert an ODR rate in Hz to a register value
 */
static int bma400_odr_to_reg(uint32_t microhertz, uint8_t *reg_val)
{
	if (microhertz == 0) {
		/* Illegal ODR value */
		return -ERANGE;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(odr_to_reg_map); i++) {
		if (microhertz <= odr_to_reg_map[i]) {
			*reg_val = i;
			return 0;
		}
	}

	/* Requested ODR too high */
	return -ERANGE;
}

/**
 * Set the sensor's acceleration offset (per axis).
 */
static int bma400_attr_set_odr(const struct sensor_value *val,
			       struct bma400_runtime_config *new_config)
{
	int status;
	uint8_t reg_val;

	/* Convert ODR Hz value to microhertz and round up to closest register setting */
	status = bma400_odr_to_reg(val->val1 * 1000000 + val->val2, &reg_val);
	if (status < 0) {
		return status;
	}

	new_config->accel_odr = reg_val;

	return 0;
}

#ifdef CONFIG_BMA400_STREAM
/**
 * Set the sensor's acceleration motion event duration
 */
static int bma400_attr_set_motion_duration(const struct sensor_value *val,
					   struct bma400_runtime_config *new_config)
{
	int status;
	struct sensor_value odr;
	float reg_float;
	/* Convert ODR Hz value to microhertz and round up to closest register setting */
	status = bma400_accel_reg_to_hz(new_config->accel_odr, &odr);
	if (status < 0) {
		return status;
	}
	reg_float = sensor_value_to_float(val) * (sensor_value_to_float(&odr));
	if (reg_float > 255.0f) {
		return -ERANGE;
	}
	new_config->motion_duration = (uint8_t)reg_float;
	return 0;
}

/**
 * Set the sensor's acceleration threshold for motion detection
 */
static int bma400_attr_set_motion_threshold(const struct sensor_value *val,
					    struct bma400_runtime_config *new_config)
{
	float reg_float;
	/* Convert mg to register value, multiple of 8mg */
	reg_float = sensor_value_to_float(val) / 8.0f;
	if (reg_float > 255.0f) {
		return -ERANGE;
	}
	new_config->motion_threshold = (uint8_t)reg_float;
	return 0;
}
#endif /* CONFIG_BMA400_STREAM */

static const uint32_t fs_to_reg_map[] = {
	2000000,  /* +/-2G => 0x0 */
	4000000,  /* +/-4G => 0x1 */
	8000000,  /* +/-8G => 0x2 */
	16000000, /* +/-16G => 0x3 */
};

static int bma400_fs_to_reg(int32_t range_ug, uint8_t *reg_val)
{
	if (range_ug == 0) {
		/* Illegal value */
		return -ERANGE;
	}

	range_ug = abs(range_ug);

	for (uint8_t i = 0; i < 4; i++) {
		if (range_ug <= fs_to_reg_map[i]) {
			*reg_val = i;
			return 0;
		}
	}

	/* Requested range too high */
	return -ERANGE;
}

/**
 * Set the sensor's full-scale range
 */
static int bma400_attr_set_range(const struct sensor_value *val,
				 struct bma400_runtime_config *new_config)
{
	int status;
	uint8_t reg_val;

	/* Convert g to micro-G's and find closest register setting */
	status = bma400_fs_to_reg(val->val1 * 1000000, &reg_val);
	if (status < 0) {
		return status;
	}

	new_config->accel_fs_range = reg_val;

	return 0;
}

/**
 * Set the sensor's bandwidth parameter (one of BMA400_BWP_*)
 */
static int bma400_attr_set_bwp(const struct sensor_value *val,
			       struct bma400_runtime_config *new_config)
{
	/* Require that `val2` is unused, and that `val1` is in range of a valid BWP */
	if (val->val2 || val->val1 < BMA400_ACCEL_FILT1_BW_0 ||
	    val->val1 > BMA400_ACCEL_FILT1_BW_1) {
		return -EINVAL;
	}

	new_config->accel_bwp = ((uint8_t)val->val1);

	return 0;
}

/**
 * @brief Implement the sensor API attribute set method.
 */
static int bma400_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct bma400_data *data = dev->data;
	struct bma400_runtime_config new_config = data->cfg;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			res = bma400_attr_set_odr(val, &new_config);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			res = bma400_attr_set_range(val, &new_config);
		} else if (attr == SENSOR_ATTR_CONFIGURATION) {
			res = bma400_attr_set_bwp(val, &new_config);
		}
#ifdef CONFIG_BMA400_STREAM
		else if (attr == SENSOR_ATTR_UPPER_THRESH) {
			if (val->val1 <= 0 || val->val2 != 0) {
				LOG_ERR("Not valid duration value");
				return -EINVAL;
			}
			res = bma400_attr_set_motion_threshold(val, &new_config);
		} else if (attr == SENSOR_ATTR_SLOPE_DUR) {
			if (val->val1 != 0 || val->val2 == 0 || val->val2 < 0) {
				LOG_ERR("Not valid duration value");
				return -EINVAL;
			}
			res = bma400_attr_set_motion_duration(val, &new_config);
		}
#endif
		else {
			LOG_ERR("Unsupported attribute");
			res = -ENOTSUP;
		}
		break;
#ifdef CONFIG_BMA400_STREAM
	case SENSOR_CHAN_ALL:
		if (attr == SENSOR_ATTR_BATCH_DURATION) {
			if (val->val1 < 0 || val->val2 != 0) {
				return -EINVAL;
			}
			new_config.batch_ticks = val->val1;
			new_config.fifo_wm = bma400_compute_fifo_wm(&new_config);
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;
#endif
	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	if (res) {
		LOG_ERR("Failed to set attribute");
		return res;
	}

	return bma400_safely_configure(dev, &new_config);
}

static int bma400_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	const struct bma400_data *data = dev->data;
	int res = 0;
	uint8_t reg_val;

	val->val1 = 0;
	val->val2 = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			/* Get current ODR setting */
			res = data->hw_ops->read_reg(dev, BMA400_REG_ACC_CONFIG1, &reg_val);
			if (res) {
				break;
			}
			bma400_accel_reg_to_hz(reg_val & BMA400_ACCEL_ODR_MSK, val);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			/* Get current full-scale range setting */
			res = data->hw_ops->read_reg(dev, BMA400_REG_ACC_CONFIG1, &reg_val);
			if (res) {
				break;
			}
			val->val1 = fs_to_reg_map[(reg_val & BMA400_ACCEL_RANGE_MSK) >>
						  BMA400_ACCEL_RANGE_POS];
		} else if (attr == SENSOR_ATTR_CONFIGURATION) {
			/* Get current bandwidth parameter (BWP) */
			res = data->hw_ops->read_reg(dev, BMA400_REG_ACC_CONFIG0, &reg_val);
			if (res) {
				break;
			}
			val->val1 = (reg_val & BMA400_FILT_1_BW_MSK) >> BMA400_FILT_1_BW_POS;
		} else {
			LOG_ERR("Unsupported attribute");
			res = -ENOTSUP;
		}
		break;
	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	if (res) {
		LOG_ERR("Failed to get attribute");
		return res;
	}
	return 0;
}

/**
 * Internal device initialization function for both bus types.
 */
static int bma400_chip_init(const struct device *dev)
{
	struct bma400_data *bma400 = dev->data;
	const struct bma400_config *cfg = dev->config;
	int status;

	/* Sensor bus-specific initialization */
	status = cfg->bus_init(dev);
	if (status) {
		LOG_ERR("Failed to initialize bus: %d", status);
		return status;
	}

	/* Read Chip ID */
	status = bma400->hw_ops->read_reg(dev, BMA400_REG_CHIP_ID, &bma400->chip_id);
	if (status) {
		LOG_ERR("could not read chip_id: %d", status);
		return status;
	}
	LOG_DBG("chip_id is 0x%02x", bma400->chip_id);

	if (bma400->chip_id != BMA400_CHIP_ID) {
		return -ENODEV;
	}

	/* Issue soft reset command */
	status = bma400->hw_ops->write_reg(dev, BMA400_REG_CMD, BMA400_SOFT_RESET_CMD);
	if (status) {
		LOG_ERR("Could not soft-reset chip: %d", status);
		return status;
	}
	/* Wait for reset to complete */
	status = !WAIT_FOR((bma400->hw_ops->read_reg(dev, BMA400_REG_CHIP_ID, &bma400->chip_id) ==
			    0) && (bma400->chip_id == BMA400_CHIP_ID),
			   100 * USEC_PER_MSEC, k_msleep(10));

	if (status) {
		LOG_ERR("Failed to reset bma400");
		return -ETIMEDOUT;
	}

	if (IS_ENABLED(CONFIG_BMA400_STREAM)) {
		status = bma400_init_interrupt(dev);
		if (status != 0) {
			LOG_ERR("Failed to initialize bma400 interrupt");
			return status;
		}
	}

	status = bma400_configure(dev, &bma400->cfg);
	if (status) {
		LOG_ERR("Failed to initialize bma400 trigger");
		return status;
	}
#ifdef CONFIG_BMA400_STREAM
	status = bma400_stream_configure(dev, &bma400->cfg);
#endif
	return status;
}

/*
 * Sensor driver API
 */

static DEVICE_API(sensor, bma400_driver_api) = {
	.attr_set = bma400_attr_set,
	.attr_get = bma400_attr_get,
	.get_decoder = bma400_get_decoder,
	.submit = bma400_submit,
};

/*
 * Device instantiation macros
 */

#define BMA400_SPI_OPERATION (SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER)

#define BMA400_RTIO_SPI_DEFINE(inst)                                                               \
	SPI_DT_IODEV_DEFINE(bma400_iodev_##inst, DT_DRV_INST(inst), BMA400_SPI_OPERATION);         \
	RTIO_DEFINE(bma400_rtio_##inst, 8, 8);

#define BMA400_DEFINE_RTIO(inst) BMA400_RTIO_SPI_DEFINE(inst)

/* Initializes a struct bma400_config for an instance on a SPI bus.
 */
#define BMA400_CONFIG_SPI(inst)                                                                    \
	.bus_cfg.spi = SPI_DT_SPEC_INST_GET(inst, BMA400_SPI_OPERATION),                           \
	.bus_init = &bma400_spi_init, .bus_type = BMA400_BUS_SPI,

#define BMA400_DEFINE_BUS(inst) BMA400_CONFIG_SPI(inst)

#ifdef CONFIG_BMA400_STREAM
#define BMA400_CFG_STREAM(inst)                                                                    \
	.fifo_en = true, .batch_ticks = 0, .fifo_wm_en = false, .fifo_full_en = false,             \
	.motion_detection_en = false,                                                              \
	.int1_map = (DT_INST_PROP(inst, fifo_pin_sel) == 1                                         \
			     ? (BMA400_EN_FIFO_WM_MSK | BMA400_EN_FIFO_FULL_MSK)                   \
			     : 0) |                                                                \
		    (DT_INST_PROP(inst, motion_detect_pin_sel) == 1 ? BMA400_EN_GEN2_MSK : 0),     \
	.int2_map = (DT_INST_PROP(inst, fifo_pin_sel) == 2                                         \
			     ? (BMA400_EN_FIFO_WM_MSK | BMA400_EN_FIFO_FULL_MSK)                   \
			     : 0) |                                                                \
		    (DT_INST_PROP(inst, motion_detect_pin_sel) == 2 ? BMA400_EN_GEN2_MSK : 0),     \
	.motion_threshold = DT_INST_PROP(inst, motion_threshold),                                  \
	.motion_duration = DT_INST_PROP(inst, motion_duration),                                    \
	.fifo_wm = DT_INST_PROP(inst, fifo_watermark),
#else
#define BMA400_CFG_STREAM(inst) .fifo_en = false,
#endif

/* --- AUTO WAKEUP --- */
#ifdef CONFIG_BMA400_WAKEUP
#define BMA400_CFG_WAKEUP(inst)                                                                    \
	.auto_wakeup_en = DT_INST_PROP(inst, auto_wakeup_en),                                      \
	.auto_wakeup_threshold = DT_INST_PROP(inst, auto_wakeup_threshold),                        \
	.wakeup_num_of_samples = DT_INST_PROP(inst, auto_wakeup_num_of_samples),                   \
	.auto_lowpower_en = DT_INST_PROP(inst, auto_lowpower_en),                                  \
	.auto_lowpower_duration = DT_INST_PROP(inst, auto_lowpower_duration),                      \
	.auto_lowpower_threshold = DT_INST_PROP(inst, auto_lowpower_threshold),
#else
#define BMA400_CFG_WAKEUP(inst)
#endif

#define BMA400_DT_CONFIG_INIT(inst)                                                                \
	{                                                                                          \
		BMA400_CFG_STREAM(inst) BMA400_CFG_WAKEUP(inst).accel_fs_range =                   \
			DT_INST_PROP(inst, fullscale_range),                                       \
		.accel_odr = DT_INST_PROP(inst, odr) + BMA400_ODR_12_5HZ,                          \
		.accel_bwp = DT_INST_PROP(inst, filt1_bw),                                         \
		.osr_lp = DT_INST_PROP(inst, osr_lp),                                              \
	}

#define BMA400_DEFINE_DATA(inst)                                                                   \
	BMA400_DEFINE_RTIO(inst);                                                                  \
	static struct bma400_data bma400_driver_##inst = {                                         \
		.cfg = BMA400_DT_CONFIG_INIT(inst),                                                \
		.r = &bma400_rtio_##inst,                                                          \
		.iodev = &bma400_iodev_##inst,                                                     \
	};

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define BMA400_INIT(inst)                                                                          \
	BMA400_DEFINE_DATA(inst);                                                                  \
                                                                                                   \
	static const struct bma400_config bma400_config_##inst = {                                 \
		BMA400_DEFINE_BUS(inst) IF_ENABLED(CONFIG_BMA400_STREAM, ( \
		.gpio_interrupt1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}),          \
		.gpio_interrupt2 = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {0}))) };            \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bma400_chip_init, NULL, &bma400_driver_##inst,          \
				     &bma400_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &bma400_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BMA400_INIT);
