/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm20689

#include <zephyr/dt-bindings/sensor/icm20689.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include "icm20689.h"
#include "icm20689_reg.h"
#include "icm20689_spi.h"
#include "icm20689_trigger.h"

#include <zephyr/logging/log.h>

#define ICM20689_POWER_UP_REG_ACCESS_TIME_MS 100
#define ICM20689_RESET_DELAY_MS              100
#define ICM20689_ACCEL_STARTUP_TIME_MS       20
#define ICM20689_GYRO_STARTUP_TIME_MS        35

LOG_MODULE_REGISTER(ICM20689, CONFIG_SENSOR_LOG_LEVEL);

/* Full-scale selector to scaling factor mappings. */
static const uint8_t icm20689_accel_sensitivity_shift[] = {
	[_ICM20689_ACCEL_CONFIG_ACCEL_FS_SEL_2G] = 14,
	[_ICM20689_ACCEL_CONFIG_ACCEL_FS_SEL_4G] = 13,
	[_ICM20689_ACCEL_CONFIG_ACCEL_FS_SEL_8G] = 12,
	[_ICM20689_ACCEL_CONFIG_ACCEL_FS_SEL_16G] = 11,
};

static const uint16_t icm20689_gyro_sensitivity_x10[] = {
	[_ICM20689_GYRO_CONFIG_FS_SEL_250_DPS] = 1310,
	[_ICM20689_GYRO_CONFIG_FS_SEL_500_DPS] = 655,
	[_ICM20689_GYRO_CONFIG_FS_SEL_1000_DPS] = 328,
	[_ICM20689_GYRO_CONFIG_FS_SEL_2000_DPS] = 164,
};

struct icm20689_reg_val {
	int32_t val;
	uint8_t reg;
};

/* ICM20689_DT_ACCEL_FS_* values; see datasheet section 11.12, register 28. */
static const struct icm20689_reg_val icm20689_accel_fs_map[] = {
	{.val = 2, .reg = ICM20689_ACCEL_CONFIG_ACCEL_FS_SEL_2G},
	{.val = 4, .reg = ICM20689_ACCEL_CONFIG_ACCEL_FS_SEL_4G},
	{.val = 8, .reg = ICM20689_ACCEL_CONFIG_ACCEL_FS_SEL_8G},
	{.val = 16, .reg = ICM20689_ACCEL_CONFIG_ACCEL_FS_SEL_16G},
};

/* ICM20689_DT_GYRO_FS_* values; see datasheet section 11.11, register 27. */
static const struct icm20689_reg_val icm20689_gyro_fs_map[] = {
	{.val = 250, .reg = ICM20689_GYRO_CONFIG_FS_SEL_250_DPS},
	{.val = 500, .reg = ICM20689_GYRO_CONFIG_FS_SEL_500_DPS},
	{.val = 1000, .reg = ICM20689_GYRO_CONFIG_FS_SEL_1000_DPS},
	{.val = 2000, .reg = ICM20689_GYRO_CONFIG_FS_SEL_2000_DPS},
};

/* ICM20689_DT_ACCEL_DLPF_* values; see datasheet section 11.13, register 29. */
static const struct icm20689_reg_val icm20689_accel_dlpf_map[] = {
	{.val = 0x00, .reg = ICM20689_ACCEL_CONFIG2_A_DLPF_CFG_218_1_0HZ},
	{.val = 0x01, .reg = ICM20689_ACCEL_CONFIG2_A_DLPF_CFG_218_1_1HZ},
	{.val = 0x02, .reg = ICM20689_ACCEL_CONFIG2_A_DLPF_CFG_99HZ},
	{.val = 0x03, .reg = ICM20689_ACCEL_CONFIG2_A_DLPF_CFG_44HZ},
	{.val = 0x04, .reg = ICM20689_ACCEL_CONFIG2_A_DLPF_CFG_21_2HZ},
	{.val = 0x05, .reg = ICM20689_ACCEL_CONFIG2_A_DLPF_CFG_10_2HZ},
	{.val = 0x06, .reg = ICM20689_ACCEL_CONFIG2_A_DLPF_CFG_5_1HZ},
	{.val = 0x07, .reg = ICM20689_ACCEL_CONFIG2_A_DLPF_CFG_420HZ},
	{.val = 0x08, .reg = ICM20689_ACCEL_CONFIG2_BIT_ACCEL_FCHOICE_B},
};

struct icm20689_gyro_dlpf_setting {
	uint8_t selector;
	uint8_t gyro_config;
	uint8_t config;
};

/* ICM20689_DT_GYRO_DLPF_* values; see datasheet sections 11.10 and 11.11. */
static const struct icm20689_gyro_dlpf_setting icm20689_gyro_dlpf_map[] = {
	{
		.selector = 0x00,
		.gyro_config = ICM20689_GYRO_CONFIG_FCHOICE_B_LOWPASS,
		.config = ICM20689_CONFIG_DLPF_CFG_250HZ,
	},
	{
		.selector = 0x01,
		.gyro_config = ICM20689_GYRO_CONFIG_FCHOICE_B_LOWPASS,
		.config = ICM20689_CONFIG_DLPF_CFG_176HZ,
	},
	{
		.selector = 0x02,
		.gyro_config = ICM20689_GYRO_CONFIG_FCHOICE_B_LOWPASS,
		.config = ICM20689_CONFIG_DLPF_CFG_92HZ,
	},
	{
		.selector = 0x03,
		.gyro_config = ICM20689_GYRO_CONFIG_FCHOICE_B_LOWPASS,
		.config = ICM20689_CONFIG_DLPF_CFG_41HZ,
	},
	{
		.selector = 0x04,
		.gyro_config = ICM20689_GYRO_CONFIG_FCHOICE_B_LOWPASS,
		.config = ICM20689_CONFIG_DLPF_CFG_20HZ,
	},
	{
		.selector = 0x05,
		.gyro_config = ICM20689_GYRO_CONFIG_FCHOICE_B_LOWPASS,
		.config = ICM20689_CONFIG_DLPF_CFG_10HZ,
	},
	{
		.selector = 0x06,
		.gyro_config = ICM20689_GYRO_CONFIG_FCHOICE_B_LOWPASS,
		.config = ICM20689_CONFIG_DLPF_CFG_5HZ,
	},
	{
		.selector = 0x07,
		.gyro_config = ICM20689_GYRO_CONFIG_FCHOICE_B_LOWPASS,
		.config = ICM20689_CONFIG_DLPF_CFG_3281HZ,
	},
	{
		.selector = 0x08,
		.gyro_config = ICM20689_GYRO_CONFIG_FCHOICE_B_8173HZ,
		.config = ICM20689_CONFIG_DLPF_CFG_250HZ,
	},
	{
		.selector = 0x10,
		.gyro_config = ICM20689_GYRO_CONFIG_FCHOICE_B_3281HZ,
		.config = ICM20689_CONFIG_DLPF_CFG_250HZ,
	},
};

static const struct icm20689_gyro_dlpf_setting *icm20689_find_gyro_dlpf_setting(uint8_t selector)
{
	for (size_t i = 0; i < ARRAY_SIZE(icm20689_gyro_dlpf_map); i++) {
		if (icm20689_gyro_dlpf_map[i].selector == selector) {
			return &icm20689_gyro_dlpf_map[i];
		}
	}

	return NULL;
}

static int icm20689_val_to_reg(const struct icm20689_reg_val *map, size_t map_size, int32_t val,
			       uint8_t *reg)
{
	for (size_t i = 0; i < map_size; i++) {
		if (map[i].val == val) {
			*reg = map[i].reg;
			return 0;
		}
	}

	return -EINVAL;
}

static int icm20689_set_accel_fs(const struct device *dev, uint16_t fs)
{
	const struct icm20689_config *cfg = dev->config;
	struct icm20689_data *data = dev->data;
	uint8_t reg;
	int ret;

	ret = icm20689_val_to_reg(icm20689_accel_fs_map, ARRAY_SIZE(icm20689_accel_fs_map), fs,
				  &reg);
	if (ret != 0) {
		LOG_ERR("Unsupported accelerometer range: %u g", fs);
		return ret;
	}

	ret = icm20689_spi_update_register(&cfg->spi, ICM20689_REG_ACCEL_CONFIG,
					   ICM20689_ACCEL_CONFIG_MASK_ACCEL_FS_SEL, reg);
	if (ret != 0) {
		return ret;
	}

	data->accel_sensitivity_shift =
		icm20689_accel_sensitivity_shift[reg >> ICM20689_ACCEL_CONFIG_SHIFT_ACCEL_FS_SEL];

	return 0;
}

static int icm20689_set_gyro_fs(const struct device *dev, uint16_t fs)
{
	const struct icm20689_config *cfg = dev->config;
	struct icm20689_data *data = dev->data;
	uint8_t reg;
	int ret;

	ret = icm20689_val_to_reg(icm20689_gyro_fs_map, ARRAY_SIZE(icm20689_gyro_fs_map), fs, &reg);
	if (ret != 0) {
		LOG_ERR("Unsupported gyroscope range: %u dps", fs);
		return ret;
	}

	ret = icm20689_spi_update_register(&cfg->spi, ICM20689_REG_GYRO_CONFIG,
					   ICM20689_GYRO_CONFIG_MASK_FS_SEL, reg);
	if (ret != 0) {
		return ret;
	}

	data->gyro_sensitivity_x10 =
		icm20689_gyro_sensitivity_x10[reg >> ICM20689_GYRO_CONFIG_SHIFT_FS_SEL];

	return 0;
}

static int icm20689_set_accel_dlpf(const struct device *dev, uint8_t selector)
{
	const struct icm20689_config *cfg = dev->config;
	const uint8_t mask = ICM20689_ACCEL_CONFIG2_MASK_A_DLPF_CFG |
			     ICM20689_ACCEL_CONFIG2_MASK_ACCEL_FCHOICE_B;
	uint8_t reg;
	int ret;

	ret = icm20689_val_to_reg(icm20689_accel_dlpf_map, ARRAY_SIZE(icm20689_accel_dlpf_map),
				  selector, &reg);
	if (ret != 0) {
		LOG_ERR("Unsupported accelerometer DLPF selector: 0x%02x", selector);
		return ret;
	}

	return icm20689_spi_update_register(&cfg->spi, ICM20689_REG_ACCEL_CONFIG2, mask, reg);
}

static int icm20689_set_gyro_dlpf(const struct device *dev, uint8_t selector)
{
	const struct icm20689_config *cfg = dev->config;
	const struct icm20689_gyro_dlpf_setting *setting;
	uint8_t old_registers[2];
	uint8_t new_gyro_config;
	uint8_t new_config;
	int rollback_ret;
	int ret;

	setting = icm20689_find_gyro_dlpf_setting(selector);
	if (setting == NULL) {
		LOG_ERR("Unsupported gyroscope DLPF selector: 0x%02x", selector);
		return -EINVAL;
	}

	ret = icm20689_spi_read(&cfg->spi, ICM20689_REG_CONFIG, old_registers,
				sizeof(old_registers));
	if (ret != 0) {
		return ret;
	}

	new_config = (old_registers[0] & ~ICM20689_CONFIG_MASK_DLPF_CFG) | setting->config;
	new_gyro_config =
		(old_registers[1] & ~ICM20689_GYRO_CONFIG_MASK_FCHOICE_B) | setting->gyro_config;

	ret = icm20689_spi_single_write(&cfg->spi, ICM20689_REG_GYRO_CONFIG, new_gyro_config);
	if (ret != 0) {
		rollback_ret = icm20689_spi_single_write(&cfg->spi, ICM20689_REG_GYRO_CONFIG,
							 old_registers[1]);
		if (rollback_ret != 0) {
			LOG_ERR("Failed to restore GYRO_CONFIG: %d", rollback_ret);
		}

		return ret;
	}

	ret = icm20689_spi_single_write(&cfg->spi, ICM20689_REG_CONFIG, new_config);
	if (ret == 0) {
		return 0;
	}

	rollback_ret =
		icm20689_spi_single_write(&cfg->spi, ICM20689_REG_GYRO_CONFIG, old_registers[1]);
	if (rollback_ret != 0) {
		LOG_ERR("Failed to restore GYRO_CONFIG: %d", rollback_ret);
	}

	rollback_ret = icm20689_spi_single_write(&cfg->spi, ICM20689_REG_CONFIG, old_registers[0]);
	if (rollback_ret != 0) {
		LOG_ERR("Failed to restore CONFIG: %d", rollback_ret);
	}

	return ret;
}

static int icm20689_set_smplrt_div(const struct device *dev, uint8_t divider)
{
	const struct icm20689_config *cfg = dev->config;

	return icm20689_spi_single_write(&cfg->spi, ICM20689_REG_SMPLRT_DIV,
					 divider << ICM20689_SMPLRT_DIV_SHIFT_SMPLRT_DIV);
}

static int icm20689_sensor_init(const struct device *dev)
{
	const struct icm20689_config *cfg = dev->config;
	const uint8_t sensor_standby_mask =
		ICM20689_PWR_MGMT_2_MASK_STBY_ZG | ICM20689_PWR_MGMT_2_MASK_STBY_YG |
		ICM20689_PWR_MGMT_2_MASK_STBY_XG | ICM20689_PWR_MGMT_2_MASK_STBY_ZA |
		ICM20689_PWR_MGMT_2_MASK_STBY_YA | ICM20689_PWR_MGMT_2_MASK_STBY_XA;
	uint8_t val;
	int ret;

	/* Maximum start-up time for register access after power-on. */
	k_msleep(ICM20689_POWER_UP_REG_ACCESS_TIME_MS);

	/* Perform a soft reset; the reset bit clears automatically. */
	ret = icm20689_spi_single_write(&cfg->spi, ICM20689_REG_PWR_MGMT_1,
					ICM20689_PWR_MGMT_1_BIT_DEVICE_RESET);
	if (ret != 0) {
		LOG_ERR("Failed to reset device");
		return ret;
	}

	/* Wait until register access is available again after reset. */
	k_msleep(ICM20689_RESET_DELAY_MS);

	/* Prevent the serial interface from switching from SPI to I2C mode. */
	ret = icm20689_spi_single_write(&cfg->spi, ICM20689_REG_USER_CTRL,
					ICM20689_USER_CTRL_BIT_I2C_IF_DIS);
	if (ret != 0) {
		return ret;
	}

	ret = icm20689_spi_read(&cfg->spi, ICM20689_REG_WHO_AM_I, &val, 1);
	if (ret != 0) {
		return ret;
	}

	if (val != ICM20689_DEVICE_ID) {
		LOG_ERR("invalid WHO_AM_I value, was %i but expected %i", val, ICM20689_DEVICE_ID);
		return -ENODEV;
	}

	LOG_INF("Found ICM20689 sensor, DEVICE ID: 0x%02X", val);

	ret = icm20689_spi_update_register(
		&cfg->spi, ICM20689_REG_PWR_MGMT_1,
		ICM20689_PWR_MGMT_1_MASK_CLKSEL | ICM20689_PWR_MGMT_1_MASK_TEMP_DIS |
			ICM20689_PWR_MGMT_1_MASK_GYRO_STANDBY |
			ICM20689_PWR_MGMT_1_MASK_ACCEL_CYCLE | ICM20689_PWR_MGMT_1_MASK_SLEEP,
		ICM20689_PWR_MGMT_1_CLKSEL_AUTOSEL_0);
	if (ret != 0) {
		return ret;
	}

	ret = icm20689_spi_update_register(&cfg->spi, ICM20689_REG_PWR_MGMT_2, sensor_standby_mask,
					   0U);
	if (ret != 0) {
		return ret;
	}

	k_msleep(MAX(ICM20689_ACCEL_STARTUP_TIME_MS, ICM20689_GYRO_STARTUP_TIME_MS));

	return 0;
}

static int icm20689_sample_fetch_temp(const struct device *dev)
{
	const struct icm20689_config *cfg = dev->config;
	struct icm20689_data *data = dev->data;
	uint8_t buffer[ICM20689_TEMP_DATA_SIZE];

	int res = icm20689_spi_read(&cfg->spi, ICM20689_REG_TEMP_OUT_H, buffer,
				    ICM20689_TEMP_DATA_SIZE);

	if (res) {
		return res;
	}

	data->temp = (int16_t)sys_get_be16(&buffer[0]);

	return 0;
}

static int32_t icm20689_align_axis(const int16_t raw[3],
				   const struct icm20689_axis_alignment *alignment)
{
	return alignment->sign * (int32_t)raw[alignment->index];
}

static void icm20689_convert_accel(struct sensor_value *val, int32_t raw_val,
				   uint16_t sensitivity_shift)
{
	int64_t conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;

	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

static void icm20689_convert_gyro(struct sensor_value *val, int32_t raw_val,
				  uint16_t sensitivity_x10)
{
	int64_t conv_val = ((int64_t)raw_val * SENSOR_PI * 10) / (sensitivity_x10 * 180LL);

	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

static inline void icm20689_convert_temp(struct sensor_value *val, int16_t raw_val)
{
	int64_t temperature = ((int64_t)raw_val * 10 * 1000000) / 3268 + (25 * 1000000);

	(void)sensor_value_from_micro(val, temperature);
}

static int icm20689_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	int res = 0;
	const struct icm20689_data *data = dev->data;
	const struct icm20689_config *cfg = dev->config;
	int16_t accel[3];
	int16_t gyro[3];

	icm20689_lock(dev);

	accel[0] = data->accel_x;
	accel[1] = data->accel_y;
	accel[2] = data->accel_z;
	gyro[0] = data->gyro_x;
	gyro[1] = data->gyro_y;
	gyro[2] = data->gyro_z;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		icm20689_convert_accel(&val[0], icm20689_align_axis(accel, &cfg->axis_align[0]),
				       data->accel_sensitivity_shift);
		icm20689_convert_accel(&val[1], icm20689_align_axis(accel, &cfg->axis_align[1]),
				       data->accel_sensitivity_shift);
		icm20689_convert_accel(&val[2], icm20689_align_axis(accel, &cfg->axis_align[2]),
				       data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm20689_convert_accel(val, icm20689_align_axis(accel, &cfg->axis_align[0]),
				       data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm20689_convert_accel(val, icm20689_align_axis(accel, &cfg->axis_align[1]),
				       data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm20689_convert_accel(val, icm20689_align_axis(accel, &cfg->axis_align[2]),
				       data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm20689_convert_gyro(&val[0], icm20689_align_axis(gyro, &cfg->axis_align[0]),
				      data->gyro_sensitivity_x10);
		icm20689_convert_gyro(&val[1], icm20689_align_axis(gyro, &cfg->axis_align[1]),
				      data->gyro_sensitivity_x10);
		icm20689_convert_gyro(&val[2], icm20689_align_axis(gyro, &cfg->axis_align[2]),
				      data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm20689_convert_gyro(val, icm20689_align_axis(gyro, &cfg->axis_align[0]),
				      data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm20689_convert_gyro(val, icm20689_align_axis(gyro, &cfg->axis_align[1]),
				      data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm20689_convert_gyro(val, icm20689_align_axis(gyro, &cfg->axis_align[2]),
				      data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm20689_convert_temp(val, data->temp);
		break;
	default:
		res = -ENOTSUP;
		break;
	}

	icm20689_unlock(dev);

	return res;
}

static int icm20689_sample_fetch_accel(const struct device *dev)
{
	const struct icm20689_config *cfg = dev->config;
	struct icm20689_data *data = dev->data;
	uint8_t buffer[ICM20689_ACCEL_DATA_SIZE];

	int res = icm20689_spi_read(&cfg->spi, ICM20689_REG_ACCEL_XOUT_H, buffer,
				    ICM20689_ACCEL_DATA_SIZE);

	if (res) {
		return res;
	}

	data->accel_x = (int16_t)sys_get_be16(&buffer[0]);
	data->accel_y = (int16_t)sys_get_be16(&buffer[2]);
	data->accel_z = (int16_t)sys_get_be16(&buffer[4]);

	return 0;
}

static int icm20689_sample_fetch_gyro(const struct device *dev)
{
	const struct icm20689_config *cfg = dev->config;
	struct icm20689_data *data = dev->data;
	uint8_t buffer[ICM20689_GYRO_DATA_SIZE];

	int res = icm20689_spi_read(&cfg->spi, ICM20689_REG_GYRO_XOUT_H, buffer,
				    ICM20689_GYRO_DATA_SIZE);

	if (res) {
		return res;
	}

	data->gyro_x = (int16_t)sys_get_be16(&buffer[0]);
	data->gyro_y = (int16_t)sys_get_be16(&buffer[2]);
	data->gyro_z = (int16_t)sys_get_be16(&buffer[4]);

	return 0;
}

static int icm20689_sample_fetch_all(const struct device *dev)
{
	const struct icm20689_config *cfg = dev->config;
	struct icm20689_data *data = dev->data;
	uint8_t buffer[ICM20689_ACCEL_DATA_SIZE + ICM20689_TEMP_DATA_SIZE +
		       ICM20689_GYRO_DATA_SIZE];
	int ret;

	ret = icm20689_spi_read(&cfg->spi, ICM20689_REG_ACCEL_XOUT_H, buffer, sizeof(buffer));
	if (ret != 0) {
		return ret;
	}

	data->accel_x = (int16_t)sys_get_be16(&buffer[0]);
	data->accel_y = (int16_t)sys_get_be16(&buffer[2]);
	data->accel_z = (int16_t)sys_get_be16(&buffer[4]);
	data->temp = (int16_t)sys_get_be16(&buffer[6]);
	data->gyro_x = (int16_t)sys_get_be16(&buffer[8]);
	data->gyro_y = (int16_t)sys_get_be16(&buffer[10]);
	data->gyro_z = (int16_t)sys_get_be16(&buffer[12]);

	return 0;
}

static int icm20689_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int res;

	icm20689_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ALL:
		res = icm20689_sample_fetch_all(dev);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		res = icm20689_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		res = icm20689_sample_fetch_gyro(dev);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		res = icm20689_sample_fetch_temp(dev);
		break;
	default:
		res = -ENOTSUP;
		break;
	}

	icm20689_unlock(dev);
	return res;
}

static int icm20689_frequency_to_divider(const struct sensor_value *frequency, uint8_t *divider)
{
	const int64_t requested_micro_hz = sensor_value_to_micro(frequency);
	const int64_t internal_rate_micro_hz = 1000LL * 1000000LL;
	const int64_t minimum_rate_micro_hz = internal_rate_micro_hz / (UINT8_MAX + 1U);
	int64_t best_difference = INT64_MAX;
	uint8_t best_divider = 0U;

	if ((requested_micro_hz < minimum_rate_micro_hz) ||
	    (requested_micro_hz > internal_rate_micro_hz)) {
		return -EINVAL;
	}

	for (uint16_t candidate = 0; candidate <= UINT8_MAX; candidate++) {
		const int64_t candidate_micro_hz = internal_rate_micro_hz / (candidate + 1U);
		const int64_t difference = candidate_micro_hz > requested_micro_hz
						   ? candidate_micro_hz - requested_micro_hz
						   : requested_micro_hz - candidate_micro_hz;

		if (difference < best_difference) {
			best_difference = difference;
			best_divider = (uint8_t)candidate;

			if (difference == 0) {
				break;
			}
		}
	}

	*divider = best_divider;
	return 0;
}

static int icm20689_divider_to_frequency(uint8_t divider, struct sensor_value *frequency)
{
	const int64_t internal_rate_micro_hz = 1000LL * 1000000LL;

	return sensor_value_from_micro(frequency, internal_rate_micro_hz / (divider + 1U));
}

enum icm20689_sensor_type {
	ICM20689_SENSOR_ACCEL,
	ICM20689_SENSOR_GYRO,
};

struct icm20689_sample_rate_mode {
	bool divider_enabled;
	int64_t fixed_micro_hz;
};

static int icm20689_get_sample_rate_mode(const struct icm20689_data *data,
					 enum icm20689_sensor_type sensor,
					 struct icm20689_sample_rate_mode *mode)
{
	if (sensor == ICM20689_SENSOR_ACCEL) {
		if (data->active.accel_dlpf <= ICM20689_DT_ACCEL_DLPF_420HZ) {
			mode->divider_enabled = true;
			mode->fixed_micro_hz = 0;
			return 0;
		}

		if (data->active.accel_dlpf == ICM20689_DT_ACCEL_BYPASS_1046HZ) {
			mode->divider_enabled = false;
			mode->fixed_micro_hz = 4000LL * 1000000LL;
			return 0;
		}

		return -EINVAL;
	}

	switch (data->active.gyro_dlpf) {
	case ICM20689_DT_GYRO_DLPF_176HZ:
	case ICM20689_DT_GYRO_DLPF_92HZ:
	case ICM20689_DT_GYRO_DLPF_41HZ:
	case ICM20689_DT_GYRO_DLPF_20HZ:
	case ICM20689_DT_GYRO_DLPF_10HZ:
	case ICM20689_DT_GYRO_DLPF_5HZ:
		mode->divider_enabled = true;
		mode->fixed_micro_hz = 0;
		return 0;

	case ICM20689_DT_GYRO_DLPF_250HZ:
	case ICM20689_DT_GYRO_DLPF_3281HZ_8KHZ:
		mode->divider_enabled = false;
		mode->fixed_micro_hz = 8000LL * 1000000LL;
		return 0;

	case ICM20689_DT_GYRO_BYPASS_8173HZ_32KHZ:
	case ICM20689_DT_GYRO_BYPASS_3281HZ_32KHZ:
		mode->divider_enabled = false;
		mode->fixed_micro_hz = 32000LL * 1000000LL;
		return 0;

	default:
		return -EINVAL;
	}
}

static int icm20689_sampling_frequency_set(const struct device *dev,
					   enum icm20689_sensor_type sensor,
					   const struct sensor_value *frequency)
{
	struct icm20689_data *data = dev->data;
	struct icm20689_sample_rate_mode mode;
	uint8_t divider;
	int ret;

	ret = icm20689_get_sample_rate_mode(data, sensor, &mode);
	if (ret != 0) {
		return ret;
	}

	if (!mode.divider_enabled) {
		return sensor_value_to_micro(frequency) == mode.fixed_micro_hz ? 0 : -EINVAL;
	}

	ret = icm20689_frequency_to_divider(frequency, &divider);
	if (ret != 0) {
		return ret;
	}

	ret = icm20689_set_smplrt_div(dev, divider);
	if (ret == 0) {
		data->active.smplrt_div = divider;
	}

	return ret;
}

static int icm20689_sampling_frequency_get(const struct icm20689_data *data,
					   enum icm20689_sensor_type sensor,
					   struct sensor_value *frequency)
{
	struct icm20689_sample_rate_mode mode;
	int ret;

	ret = icm20689_get_sample_rate_mode(data, sensor, &mode);
	if (ret != 0) {
		return ret;
	}

	if (mode.divider_enabled) {
		return icm20689_divider_to_frequency(data->active.smplrt_div, frequency);
	}

	return sensor_value_from_micro(frequency, mode.fixed_micro_hz);
}

static int icm20689_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct icm20689_data *data = dev->data;
	struct sensor_value expected;
	int32_t fs;
	int ret;

	__ASSERT_NO_MSG(val != NULL);

	icm20689_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_FULL_SCALE) {
			fs = sensor_ms2_to_g(val);
			sensor_g_to_ms2(fs, &expected);
			if (sensor_value_to_micro(&expected) != sensor_value_to_micro(val)) {
				ret = -EINVAL;
				break;
			}

			ret = icm20689_set_accel_fs(dev, (uint16_t)fs);
			if (ret == 0) {
				data->active.accel_fs = (uint16_t)fs;
			}
		} else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			ret = icm20689_sampling_frequency_set(dev, ICM20689_SENSOR_ACCEL, val);
		} else {
			ret = -ENOTSUP;
		}
		break;

	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_FULL_SCALE) {
			fs = sensor_rad_to_degrees(val);
			sensor_degrees_to_rad(fs, &expected);
			if (sensor_value_to_micro(&expected) != sensor_value_to_micro(val)) {
				ret = -EINVAL;
				break;
			}

			ret = icm20689_set_gyro_fs(dev, (uint16_t)fs);
			if (ret == 0) {
				data->active.gyro_fs = (uint16_t)fs;
			}
		} else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			ret = icm20689_sampling_frequency_set(dev, ICM20689_SENSOR_GYRO, val);
		} else {
			ret = -ENOTSUP;
		}
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	icm20689_unlock(dev);
	return ret;
}

static int icm20689_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct icm20689_data *data = dev->data;
	int ret;

	__ASSERT_NO_MSG(val != NULL);

	icm20689_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_FULL_SCALE) {
			sensor_g_to_ms2(data->active.accel_fs, val);
			ret = 0;
		} else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			ret = icm20689_sampling_frequency_get(data, ICM20689_SENSOR_ACCEL, val);
		} else {
			ret = -ENOTSUP;
		}
		break;

	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_FULL_SCALE) {
			sensor_degrees_to_rad(data->active.gyro_fs, val);
			ret = 0;
		} else if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			ret = icm20689_sampling_frequency_get(data, ICM20689_SENSOR_GYRO, val);
		} else {
			ret = -ENOTSUP;
		}
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	icm20689_unlock(dev);
	return ret;
}

static int icm20689_apply_config(const struct device *dev,
				 const struct icm20689_sensor_config *config)
{
	int ret;

	ret = icm20689_set_accel_fs(dev, config->accel_fs);
	if (ret != 0) {
		return ret;
	}

	ret = icm20689_set_gyro_fs(dev, config->gyro_fs);
	if (ret != 0) {
		return ret;
	}

	ret = icm20689_set_accel_dlpf(dev, config->accel_dlpf);
	if (ret != 0) {
		return ret;
	}

	ret = icm20689_set_gyro_dlpf(dev, config->gyro_dlpf);
	if (ret != 0) {
		return ret;
	}

	if (config->smplrt_div != 0U) {
		if (config->accel_dlpf == ICM20689_DT_ACCEL_BYPASS_1046HZ) {
			LOG_WRN("smplrt-div=%u does not affect the accelerometer in bypass mode",
				config->smplrt_div);
		}

		if ((config->gyro_dlpf < ICM20689_DT_GYRO_DLPF_176HZ) ||
		    (config->gyro_dlpf > ICM20689_DT_GYRO_DLPF_5HZ)) {
			LOG_WRN("smplrt-div=%u does not affect the gyroscope in the selected "
				"filter mode",
				config->smplrt_div);
		}
	}

	return icm20689_set_smplrt_div(dev, config->smplrt_div);
}

static int icm20689_init(const struct device *dev)
{
	const struct icm20689_config *config = dev->config;
	struct icm20689_data *data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	ret = icm20689_sensor_init(dev);
	if (ret != 0) {
		LOG_ERR("could not initialize sensor");
		return ret;
	}

	ret = icm20689_apply_config(dev, &config->initial);
	if (ret != 0) {
		LOG_ERR("could not configure sensor");
		return ret;
	}

	data->active = config->initial;

#ifdef CONFIG_ICM20689_TRIGGER
	ret = icm20689_trigger_init(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize interrupts");
		return ret;
	}
#endif

	return 0;
}

#ifndef CONFIG_ICM20689_TRIGGER

void icm20689_lock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

void icm20689_unlock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

#endif

static DEVICE_API(sensor, icm20689_driver_api) = {
	.sample_fetch = icm20689_sample_fetch,
	.channel_get = icm20689_channel_get,
	.attr_set = icm20689_attr_set,
	.attr_get = icm20689_attr_get,
#ifdef CONFIG_ICM20689_TRIGGER
	.trigger_set = icm20689_trigger_set,
#endif
};

/* device defaults to spi mode 0/3 support */
#define ICM20689_SPI_CFG                                                                           \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define ICM20689_SENSOR_CONFIG_INIT(inst)                                                          \
	{                                                                                          \
		.accel_fs = DT_INST_PROP(inst, accel_fs),                                          \
		.gyro_fs = DT_INST_PROP(inst, gyro_fs),                                            \
		.accel_dlpf = DT_INST_PROP(inst, accel_dlpf),                                      \
		.gyro_dlpf = DT_INST_PROP(inst, gyro_dlpf),                                        \
		.smplrt_div = DT_INST_PROP(inst, smplrt_div),                                      \
	}

#define ICM20689_INIT(inst)                                                                        \
	static struct icm20689_data icm20689_driver_##inst;                                        \
                                                                                                   \
	static const struct icm20689_config icm20689_cfg_##inst = {                                \
		.spi = SPI_DT_SPEC_INST_GET(inst, ICM20689_SPI_CFG),                               \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
		.axis_align[0].index = DT_INST_PROP(inst, axis_align_x),                           \
		.axis_align[1].index = DT_INST_PROP(inst, axis_align_y),                           \
		.axis_align[2].index = DT_INST_PROP(inst, axis_align_z),                           \
		.axis_align[0].sign = DT_INST_PROP(inst, axis_align_x_sign) - 1,                   \
		.axis_align[1].sign = DT_INST_PROP(inst, axis_align_y_sign) - 1,                   \
		.axis_align[2].sign = DT_INST_PROP(inst, axis_align_z_sign) - 1,                   \
		.initial = ICM20689_SENSOR_CONFIG_INIT(inst),                                      \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm20689_init, NULL, &icm20689_driver_##inst,           \
				     &icm20689_cfg_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &icm20689_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ICM20689_INIT)
