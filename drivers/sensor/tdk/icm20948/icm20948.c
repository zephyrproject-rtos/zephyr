/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm20948.h"
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT invensense_icm20948

LOG_MODULE_REGISTER(icm20948, CONFIG_SENSOR_LOG_LEVEL);

/* Accel sensitivity shift values for each FS setting (LSB/g as power of 2) */
static const uint16_t accel_sensitivity_shift[] = {
	14, /* ±2g:  16384 LSB/g = 2^14 */
	13, /* ±4g:  8192 LSB/g  = 2^13 */
	12, /* ±8g:  4096 LSB/g  = 2^12 */
	11, /* ±16g: 2048 LSB/g  = 2^11 */
};

/* Gyro sensitivity x10 for each FS setting (to avoid floats) */
static const uint16_t gyro_sensitivity_x10[] = {
	1310, /* ±250 dps:  131.0 LSB/°/s */
	655,  /* ±500 dps:  65.5 LSB/°/s */
	328,  /* ±1000 dps: 32.8 LSB/°/s */
	164,  /* ±2000 dps: 16.4 LSB/°/s */
};

static void icm20948_convert_accel(struct sensor_value *val, int16_t raw_val,
				   uint16_t sensitivity_shift)
{
	int64_t conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;

	val->val1 = conv_val / SENSOR_VALUE_SCALE;
	val->val2 = conv_val % SENSOR_VALUE_SCALE;
}

static void icm20948_convert_gyro(struct sensor_value *val, int16_t raw_val,
				  uint16_t sensitivity_x10)
{
	int64_t conv_val = ((int64_t)raw_val * SENSOR_PI * 10) / (sensitivity_x10 * 180U);

	val->val1 = conv_val / SENSOR_VALUE_SCALE;
	val->val2 = conv_val % SENSOR_VALUE_SCALE;
}

static inline void icm20948_convert_temp(struct sensor_value *val, int16_t raw_val)
{
	/* ICM-20948 temperature formula from datasheet:
	 * Temp_degC = ((raw - RoomTemp_Offset) / Temp_Sensitivity) + 21
	 * RoomTemp_Offset = 0, Temp_Sensitivity = 333.87 LSB/°C
	 * Simplified: Temp_degC = (raw / 333.87) + 21
	 * Using fixed point: Temp_degC = (raw * 1000000 / 333870) + 21
	 */
	int64_t temp_uc = ((int64_t)raw_val * SENSOR_VALUE_SCALE) / 33387LL + 21000000LL;

	val->val1 = temp_uc / SENSOR_VALUE_SCALE;
	val->val2 = temp_uc % SENSOR_VALUE_SCALE;
}

static int icm20948_set_accel_div(const struct device *dev, uint16_t div)
{
	int ret;

	ret = icm20948_write_reg(dev, ICM20948_REG_ACCEL_SMPLRT_DIV_1, (div >> 8) & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to set accel sample rate divider MSB");
		return ret;
	}

	ret = icm20948_write_reg(dev, ICM20948_REG_ACCEL_SMPLRT_DIV_2, div & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to set accel sample rate divider LSB");
		return ret;
	}

	return 0;
}

static int icm20948_compute_accel_div(const struct device *dev, float hz)
{
	uint16_t divider;

	if (hz > 1125.0f || hz < 0.27f) {
		LOG_ERR("Invalid ODR for accel");
		return -EINVAL;
	}

	LOG_DBG("Setting accel ODR to %.2f Hz", (double)hz);
	divider = (uint16_t)((1100.0f / hz) - 1.0f);

	return icm20948_set_accel_div(dev, divider);
}

static int icm20948_compute_gyro_div(const struct device *dev, float hz)
{
	uint8_t divider;

	if (hz > 1100.0f || hz < 4.4f) {
		LOG_ERR("Invalid ODR for gyro");
		return -EINVAL;
	}

	LOG_DBG("Setting gyro ODR to %.2f Hz", (double)hz);
	divider = (uint8_t)((1100.0f / hz) - 1.0f);

	return icm20948_write_reg(dev, ICM20948_REG_GYRO_SMPLRT_DIV, divider);
}

static int icm20948_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	float hz = 0.0f;

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		hz = sensor_value_to_float(val);
		if (chan == SENSOR_CHAN_ACCEL_XYZ) {
			icm20948_compute_accel_div(dev, hz);
		} else if (chan == SENSOR_CHAN_GYRO_XYZ) {
			icm20948_compute_gyro_div(dev, hz);
		} else if (chan == SENSOR_CHAN_MAGN_XYZ) {
			/* Magnetometer ODR is fixed by mag-mode in devicetree */
		} else {
			return -ENOTSUP;
		}
		break;

#ifdef CONFIG_ICM20948_TRIGGER
	case SENSOR_ATTR_SLOPE_TH:
		/* Wake-on-Motion threshold in mg (val1 = mg) */
		if (chan == SENSOR_CHAN_ACCEL_XYZ) {
			return icm20948_config_wom(dev, (uint8_t)val->val1);
		}
		return -ENOTSUP;
#endif

	default:
		return -ENOTSUP;
	}
	return 0;
}

static int icm20948_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icm20948_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		icm20948_convert_accel(val, data->accel_x, data->accel_sensitivity_shift);
		icm20948_convert_accel(val + 1, data->accel_y, data->accel_sensitivity_shift);
		icm20948_convert_accel(val + 2, data->accel_z, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm20948_convert_accel(val, data->accel_x, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm20948_convert_accel(val, data->accel_y, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm20948_convert_accel(val, data->accel_z, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm20948_convert_gyro(val, data->gyro_x, data->gyro_sensitivity_x10);
		icm20948_convert_gyro(val + 1, data->gyro_y, data->gyro_sensitivity_x10);
		icm20948_convert_gyro(val + 2, data->gyro_z, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm20948_convert_gyro(val, data->gyro_x, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm20948_convert_gyro(val, data->gyro_y, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm20948_convert_gyro(val, data->gyro_z, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm20948_convert_temp(val, data->temp);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		if (ak09916_convert_magn(val, data->magn_x, data->magn_scale_x, data->magn_st2) <
		    0) {
			return -EOVERFLOW;
		}
		if (ak09916_convert_magn(val + 1, data->magn_y, data->magn_scale_y,
					 data->magn_st2) < 0) {
			return -EOVERFLOW;
		}
		if (ak09916_convert_magn(val + 2, data->magn_z, data->magn_scale_z,
					 data->magn_st2) < 0) {
			return -EOVERFLOW;
		}
		break;
	case SENSOR_CHAN_MAGN_X:
		return ak09916_convert_magn(val, data->magn_x, data->magn_scale_x, data->magn_st2);
	case SENSOR_CHAN_MAGN_Y:
		return ak09916_convert_magn(val, data->magn_y, data->magn_scale_y, data->magn_st2);
	case SENSOR_CHAN_MAGN_Z:
		return ak09916_convert_magn(val, data->magn_z, data->magn_scale_z, data->magn_st2);
	default:
		return -ENOTSUP;
	}
	return 0;
}

/* External sensor data register where AK09916 magnetometer data is stored (Bank 0) */
#define ICM20948_REG_EXT_SLV_SENS_DATA_00 0x003B

static int icm20948_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct icm20948_data *data = dev->data;
	const struct icm20948_config *cfg = dev->config;

	int16_t buf[ICM20948_ACCEL_BYTES / 2 + ICM20948_GYRO_BYTES / 2 + ICM20948_TEMP_BYTES / 2];
	int ret;

	ret = icm20948_read_block(dev, ICM20948_REG_DATA_START, (uint8_t *)buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read data sample.");
		return ret;
	}

	/* Register order: ACCEL_X, ACCEL_Y, ACCEL_Z, GYRO_X, GYRO_Y, GYRO_Z, TEMP */
	data->accel_x = sys_be16_to_cpu(buf[0]);
	data->accel_y = sys_be16_to_cpu(buf[1]);
	data->accel_z = sys_be16_to_cpu(buf[2]);
	data->gyro_x = sys_be16_to_cpu(buf[3]);
	data->gyro_y = sys_be16_to_cpu(buf[4]);
	data->gyro_z = sys_be16_to_cpu(buf[5]);
	data->temp = sys_be16_to_cpu(buf[6]);

	if (cfg->mag_mode != 0) {
		/* Read magnetometer data from external sensor data registers.
		 * SLV0 is configured to automatically read AK09916 data into these registers.
		 * Data order: HXL, HXH, HYL, HYH, HZL, HZH, DUMMY, ST2 (8 bytes)
		 * Note: Register 0x17 is a dummy/reserved register between HZH and ST2.
		 */
		uint8_t magn_buf[8];

		ret = icm20948_read_block(dev, ICM20948_REG_EXT_SLV_SENS_DATA_00, magn_buf,
					  sizeof(magn_buf));
		if (ret < 0) {
			LOG_ERR("Failed to read magnetometer data.");
			return ret;
		}

		/* AK09916 data is little-endian */
		data->magn_x = sys_le16_to_cpu(*(int16_t *)&magn_buf[0]);
		data->magn_y = sys_le16_to_cpu(*(int16_t *)&magn_buf[2]);
		data->magn_z = sys_le16_to_cpu(*(int16_t *)&magn_buf[4]);
		data->magn_st2 = magn_buf[7]; /* ST2 is at index 7, after dummy byte */
	}

	return 0;
}

static int wake(const struct device *dev)
{
	uint8_t reg;
	int ret;

	/* Wake up the chip: clear SLEEP bit and use auto clock select. */
	reg = FIELD_PREP(ICM20948_PWR_MGMT_1_SLEEP_MASK, 0) |
	      FIELD_PREP(ICM20948_PWR_MGMT_1_CLKSEL_MASK, ICM20948_PWR_MGMT_1_CLKSEL_AUTO);
	ret = icm20948_write_reg(dev, ICM20948_REG_PWR_MGMT_1, reg);
	if (ret < 0) {
		return ret;
	}

	k_msleep(ICM20948_STARTUP_TIME_MS);

	return 0;
}

static int icm20948_init(const struct device *dev)
{
	const struct icm20948_config *cfg = dev->config;
	struct icm20948_data *data = dev->data;
	uint8_t chip_id = 0;
	int ret;

	if (cfg->bus_type == ICM20948_BUS_I2C) {
		if (!device_is_ready(cfg->i2c.bus)) {
			LOG_ERR("I2C dev %s not ready", cfg->i2c.bus->name);
			return -ENODEV;
		}
	} else {
		if (!device_is_ready(cfg->spi.bus)) {
			LOG_ERR("SPI dev %s not ready", cfg->spi.bus->name);
			return -ENODEV;
		}
	}

	/* Reset the chip */
	ret = icm20948_write_field(dev, ICM20948_REG_PWR_MGMT_1, ICM20948_PWR_MGMT_1_RESET_MASK, 1);

	if (ret < 0) {
		LOG_ERR("Failed to reset.");
		return ret;
	}

	uint16_t counter = 0;
	uint8_t rst;

	do {
		k_usleep(ICM20948_RESET_POLL_DELAY_US);
		ret = icm20948_read_field(dev, ICM20948_REG_PWR_MGMT_1,
					  ICM20948_PWR_MGMT_1_RESET_MASK, &rst);
		if (counter++ > ICM20948_RESET_TIMEOUT_LOOPS) {
			LOG_ERR("Timeout waiting for reset");
			return -ETIMEDOUT;
		}
	} while (rst);

	/* Wait for oscillator to stabilize after reset */
	k_msleep(ICM20948_OSC_STABILIZE_MS);

	ret = wake(dev);
	if (ret < 0) {
		return ret;
	}

	/* Verify chip ID */
	ret = icm20948_read_reg(dev, ICM20948_REG_WHO_AM_I, &chip_id);
	if (ret < 0) {
		LOG_ERR("Failed to read WHO_AM_I register");
		return ret;
	}

	if (chip_id != ICM20948_WHO_AM_I_VAL) {
		LOG_ERR("Invalid chip ID: 0x%02X (expected 0x%02X)", chip_id,
			ICM20948_WHO_AM_I_VAL);
		return -ENODEV;
	}
	LOG_INF("ICM20948 detected (chip ID: 0x%02X)", chip_id);

	/* Enable all sensors (accel + gyro) */
	ret = icm20948_write_reg(dev, ICM20948_REG_PWR_MGMT_2, ICM20948_PWR_MGMT_2_ALL_ON);
	if (ret < 0) {
		LOG_ERR("Failed to enable sensors");
		return ret;
	}

	/* Configure gyroscope: FS and DLPF */
	uint8_t reg = FIELD_PREP(ICM20948_GYRO_CONFIG_1_FCHOICE_MASK, cfg->gyro_dlpf == 8 ? 0 : 1) |
		      FIELD_PREP(ICM20948_GYRO_CONFIG_1_FS_SEL_MASK, cfg->gyro_fs) |
		      FIELD_PREP(ICM20948_GYRO_CONFIG_1_DLPFCFG_MASK, cfg->gyro_dlpf);
	ret = icm20948_write_reg(dev, ICM20948_REG_GYRO_CONFIG_1, reg);
	if (ret < 0) {
		LOG_ERR("Failed to configure gyroscope");
		return ret;
	}

	/* Configure gyroscope sample rate divider */
	ret = icm20948_write_reg(dev, ICM20948_REG_GYRO_SMPLRT_DIV, cfg->gyro_div);
	if (ret < 0) {
		LOG_ERR("Failed to set gyro sample rate divider");
		return ret;
	}

	/* Configure accelerometer: FS and DLPF */
	reg = FIELD_PREP(ICM20948_ACCEL_CONFIG_FCHOICE_MASK, cfg->accel_dlpf == 8 ? 0 : 1) |
	      FIELD_PREP(ICM20948_ACCEL_CONFIG_FS_SEL_MASK, cfg->accel_fs) |
	      FIELD_PREP(ICM20948_ACCEL_CONFIG_DLPFCFG_MASK, cfg->accel_dlpf);
	ret = icm20948_write_reg(dev, ICM20948_REG_ACCEL_CONFIG, reg);
	if (ret < 0) {
		LOG_ERR("Failed to configure accelerometer");
		return ret;
	}

	/* Configure accelerometer sample rate divider */
	ret = icm20948_set_accel_div(dev, cfg->accel_div);

	/* Initialize sensitivity values based on configured full-scale ranges */
	data->accel_sensitivity_shift = accel_sensitivity_shift[cfg->accel_fs];
	data->gyro_sensitivity_x10 = gyro_sensitivity_x10[cfg->gyro_fs];

	LOG_DBG("Accel FS: %d (shift=%d), Gyro FS: %d (sens_x10=%d)", cfg->accel_fs,
		data->accel_sensitivity_shift, cfg->gyro_fs, data->gyro_sensitivity_x10);

	/* Initialize magnetometer (AK09916) via I2C master bridge */
	if (cfg->mag_mode != 0) {
		ret = ak09916_init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize AK09916 magnetometer.");
			return ret;
		}
	}

#ifdef CONFIG_ICM20948_TRIGGER
	ret = icm20948_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupts.");
		return ret;
	}
#endif

	return 0;
}

static DEVICE_API(sensor, icm20948_api) = {
#ifdef CONFIG_ICM20948_TRIGGER
	.trigger_set = icm20948_trigger_set,
#endif
	.sample_fetch = icm20948_sample_fetch,
	.channel_get = icm20948_channel_get,
	.attr_set = icm20948_attr_set,
};

#define ICM20948_DEFINE(inst)                                                                      \
	static struct icm20948_data icm20948_data_##inst;                                          \
	static const struct icm20948_config icm20948_cfg_##inst = {                                \
		.bus_type = COND_CODE_1(DT_INST_ON_BUS(inst, i2c), \
		(ICM20948_BUS_I2C), \
		(ICM20948_BUS_SPI)),                                    \
			 COND_CODE_1(DT_INST_ON_BUS(inst, i2c),	\
			({.i2c = I2C_DT_SPEC_INST_GET(inst)}), \
			({.spi =	SPI_DT_SPEC_INST_GET(inst, \
								SPI_OP_MODE_MASTER | \
								SPI_MODE_CPOL | \
								SPI_MODE_CPHA | \
								SPI_WORD_SET(8) | \
								SPI_TRANSFER_MSB)})),    \
				     .gyro_div = DT_INST_PROP_OR(inst, gyro_div, 0),               \
				     .gyro_dlpf = DT_INST_ENUM_IDX_OR(inst, gyro_dlpf, 0),         \
				     .gyro_fs = DT_INST_ENUM_IDX_OR(inst, gyro_fs, 0),             \
				     .accel_div = DT_INST_PROP_OR(inst, accel_div, 0),             \
				     .accel_dlpf = DT_INST_ENUM_IDX_OR(inst, accel_dlpf, 0),       \
				     .accel_fs = DT_INST_ENUM_IDX_OR(inst, accel_fs, 0),           \
				     .mag_mode = DT_INST_PROP_OR(inst, mag_mode, 0),               \
				     IF_ENABLED(CONFIG_ICM20948_TRIGGER, \
	(.int_pin = GPIO_DT_SPEC_INST_GET(inst, int_gpios),)) };   \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm20948_init, NULL, &icm20948_data_##inst,             \
				     &icm20948_cfg_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &icm20948_api);

DT_INST_FOREACH_STATUS_OKAY(ICM20948_DEFINE)
