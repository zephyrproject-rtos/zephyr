/*
 * Copyright (c) 2021 Bosch Sensortec GmbH
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmi260

#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "bmi260.h"
#include "bmi260_config_file.h"

LOG_MODULE_REGISTER(bmi260, CONFIG_SENSOR_LOG_LEVEL);

/* Minimum output data rate in micro hertz for which a register value is selected */
struct bmi260_odr_entry {
	int64_t min_micro_hz;
	uint8_t reg;
};

struct bmi260_osr_entry {
	int32_t ratio;
	uint8_t reg;
};

struct bmi260_range_entry {
	int32_t range;
	uint8_t reg;
};

static const struct bmi260_odr_entry bmi260_acc_odr_table[] = {
	{781250LL, BMI260_ACC_ODR_25D32_HZ},    {1562500LL, BMI260_ACC_ODR_25D16_HZ},
	{3125000LL, BMI260_ACC_ODR_25D8_HZ},    {6250000LL, BMI260_ACC_ODR_25D4_HZ},
	{12500000LL, BMI260_ACC_ODR_25D2_HZ},   {25000000LL, BMI260_ACC_ODR_25_HZ},
	{50000000LL, BMI260_ACC_ODR_50_HZ},     {100000000LL, BMI260_ACC_ODR_100_HZ},
	{200000000LL, BMI260_ACC_ODR_200_HZ},   {400000000LL, BMI260_ACC_ODR_400_HZ},
	{800000000LL, BMI260_ACC_ODR_800_HZ},   {1600000000LL, BMI260_ACC_ODR_1600_HZ},
};

static const struct bmi260_odr_entry bmi260_gyr_odr_table[] = {
	{25000000LL, BMI260_GYR_ODR_25_HZ},     {50000000LL, BMI260_GYR_ODR_50_HZ},
	{100000000LL, BMI260_GYR_ODR_100_HZ},   {200000000LL, BMI260_GYR_ODR_200_HZ},
	{400000000LL, BMI260_GYR_ODR_400_HZ},   {800000000LL, BMI260_GYR_ODR_800_HZ},
	{1600000000LL, BMI260_GYR_ODR_1600_HZ}, {3200000000LL, BMI260_GYR_ODR_3200_HZ},
};

/* Accelerometer oversampling in performance mode (filter configured for ODR >= 100 Hz) */
static const struct bmi260_osr_entry bmi260_acc_osr_perf_table[] = {
	{4, BMI260_ACC_BWP_OSR4_AVG1},
	{2, BMI260_ACC_BWP_OSR2_AVG2},
	{1, BMI260_ACC_BWP_NORM_AVG4},
};

/* Accelerometer averaging in power optimized mode */
static const struct bmi260_osr_entry bmi260_acc_osr_power_table[] = {
	{1, BMI260_ACC_BWP_OSR4_AVG1},    {2, BMI260_ACC_BWP_OSR2_AVG2},
	{4, BMI260_ACC_BWP_NORM_AVG4},    {8, BMI260_ACC_BWP_CIC_AVG8},
	{16, BMI260_ACC_BWP_RES_AVG16},   {32, BMI260_ACC_BWP_RES_AVG32},
	{64, BMI260_ACC_BWP_RES_AVG64},   {128, BMI260_ACC_BWP_RES_AVG128},
};

static const struct bmi260_osr_entry bmi260_gyr_osr_table[] = {
	{4, BMI260_GYR_BWP_OSR4},
	{2, BMI260_GYR_BWP_OSR2},
	{1, BMI260_GYR_BWP_NORM},
};

/* Full scale range in g */
static const struct bmi260_range_entry bmi260_acc_range_table[] = {
	{2, BMI260_ACC_RANGE_2G},
	{4, BMI260_ACC_RANGE_4G},
	{8, BMI260_ACC_RANGE_8G},
	{16, BMI260_ACC_RANGE_16G},
};

/* Full scale range in degrees per second */
static const struct bmi260_range_entry bmi260_gyr_range_table[] = {
	{125, BMI260_GYR_RANGE_125DPS},   {250, BMI260_GYR_RANGE_250DPS},
	{500, BMI260_GYR_RANGE_500DPS},   {1000, BMI260_GYR_RANGE_1000DPS},
	{2000, BMI260_GYR_RANGE_2000DPS},
};

static inline int bmi260_bus_check(const struct device *dev)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static inline int bmi260_bus_init(const struct device *dev)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->init(&cfg->bus);
}

int bmi260_reg_read(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t length)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, reg, data, length);
}

int bmi260_reg_write(const struct device *dev, uint8_t reg, const uint8_t *data, uint16_t length)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, data, length);
}

int bmi260_reg_write_with_delay(const struct device *dev, uint8_t reg, const uint8_t *data,
				uint16_t length, uint32_t delay_us)
{
	int ret;

	ret = bmi260_reg_write(dev, reg, data, length);
	if (ret == 0) {
		k_usleep(delay_us);
	}

	return ret;
}

/* Replace the bits selected by mask in a register value */
static inline uint8_t bmi260_set_field(uint8_t reg_val, uint8_t mask, uint8_t field_val)
{
	return (reg_val & ~mask) | FIELD_PREP(mask, field_val);
}

static int bmi260_odr_to_reg(const struct bmi260_odr_entry *table, size_t len,
			     const struct sensor_value *val, uint8_t *reg)
{
	int64_t micro_hz = sensor_value_to_micro(val);

	/* A value below the lowest supported rate selects 0, which disables the sensor */
	*reg = 0;

	for (size_t i = 0; i < len; i++) {
		if (micro_hz >= table[i].min_micro_hz) {
			*reg = table[i].reg;
		}
	}

	return 0;
}

static int bmi260_osr_to_reg(const struct bmi260_osr_entry *table, size_t len,
			     const struct sensor_value *val, uint8_t *reg)
{
	/* val->val2 is unused */
	for (size_t i = 0; i < len; i++) {
		if (table[i].ratio == val->val1) {
			*reg = table[i].reg;
			return 0;
		}
	}

	return -ENOTSUP;
}

static int bmi260_range_to_reg(const struct bmi260_range_entry *table, size_t len,
			       const struct sensor_value *val, uint8_t *reg)
{
	/* val->val2 is unused */
	for (size_t i = 0; i < len; i++) {
		if (table[i].range == val->val1) {
			*reg = table[i].reg;
			return 0;
		}
	}

	return -ENOTSUP;
}

static int bmi260_update_reg(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t field_val)
{
	uint8_t reg_val;
	int ret;

	ret = bmi260_reg_read(dev, reg, &reg_val, 1);
	if (ret != 0) {
		return ret;
	}

	reg_val = bmi260_set_field(reg_val, mask, field_val);

	return bmi260_reg_write_with_delay(dev, reg, &reg_val, 1, BMI260_INTER_WRITE_DELAY_US);
}

static void bmi260_channel_accel_convert(struct sensor_value *val, int64_t raw_val, uint8_t range)
{
	/* 16 bit accelerometer. 2^15 bits represent the range in G, converted to m/s^2 */
	raw_val = (raw_val * SENSOR_G * (int64_t)range) / INT16_MAX;

	val->val1 = raw_val / 1000000LL;
	val->val2 = raw_val % 1000000LL;
}

static void bmi260_channel_gyro_convert(struct sensor_value *val, int64_t raw_val, uint16_t range)
{
	/* 16 bit gyroscope. 2^15 bits represent the range in degrees/s, converted to rad/s */
	int64_t micro_rad = (raw_val * (int64_t)range * SENSOR_PI) / (180LL * INT16_MAX);

	val->val1 = micro_rad / 1000000LL;
	val->val2 = micro_rad % 1000000LL;
}

static int bmi260_set_odr(const struct device *dev, uint8_t conf_reg, uint8_t odr_mask,
			  uint8_t odr_reg, uint8_t pwr_en_bit, bool perf_filter,
			  bool is_gyro)
{
	uint8_t conf;
	uint8_t pwr_ctrl;
	int ret;

	ret = bmi260_reg_read(dev, conf_reg, &conf, 1);
	if (ret != 0) {
		return ret;
	}

	ret = bmi260_reg_read(dev, BMI260_REG_PWR_CTRL, &pwr_ctrl, 1);
	if (ret != 0) {
		return ret;
	}

	conf = bmi260_set_field(conf, odr_mask, odr_reg);

	if (is_gyro) {
		conf = bmi260_set_field(conf, BMI260_GYR_FILT_MSK,
					perf_filter ? BMI260_GYR_FILT_PERF_OPT
						    : BMI260_GYR_FILT_PWR_OPT);
		conf = bmi260_set_field(conf, BMI260_GYR_FILT_NOISE_MSK,
					perf_filter ? BMI260_GYR_FILT_NOISE_PERF
						    : BMI260_GYR_FILT_NOISE_PWR);
	} else {
		conf = bmi260_set_field(conf, BMI260_ACC_FILT_MSK,
					perf_filter ? BMI260_ACC_FILT_PERF_OPT
						    : BMI260_ACC_FILT_PWR_OPT);
	}

	/* An output data rate register value of 0 means the sensor is switched off */
	if (odr_reg != 0) {
		pwr_ctrl |= pwr_en_bit;
	} else {
		pwr_ctrl &= ~pwr_en_bit;
	}

	ret = bmi260_reg_write(dev, conf_reg, &conf, 1);
	if (ret != 0) {
		return ret;
	}

	/* Assuming we have advanced power save enabled */
	k_usleep(BMI260_TRANSC_DELAY_SUSPEND);

	pwr_ctrl &= BMI260_PWR_CTRL_MSK;

	return bmi260_reg_write_with_delay(dev, BMI260_REG_PWR_CTRL, &pwr_ctrl, 1,
					   BMI260_INTER_WRITE_DELAY_US);
}

static int bmi260_set_accel_odr(const struct device *dev, const struct sensor_value *val)
{
	struct bmi260_data *data = dev->data;
	uint8_t odr_reg;
	bool perf_filter;
	int ret;

	ret = bmi260_odr_to_reg(bmi260_acc_odr_table, ARRAY_SIZE(bmi260_acc_odr_table), val,
				&odr_reg);
	if (ret != 0) {
		return ret;
	}

	perf_filter = !IS_ENABLED(CONFIG_BMI260_LOW_POWER_MODE) &&
		      (odr_reg >= BMI260_ACC_ODR_100_HZ);

	ret = bmi260_set_odr(dev, BMI260_REG_ACC_CONF, BMI260_ACC_ODR_MSK, odr_reg,
			     BMI260_PWR_CTRL_ACC_EN, perf_filter, false);
	if (ret == 0) {
		data->acc_odr = odr_reg;
	}

	return ret;
}

static int bmi260_set_gyro_odr(const struct device *dev, const struct sensor_value *val)
{
	struct bmi260_data *data = dev->data;
	uint8_t odr_reg;
	bool perf_filter;
	int ret;

	ret = bmi260_odr_to_reg(bmi260_gyr_odr_table, ARRAY_SIZE(bmi260_gyr_odr_table), val,
				&odr_reg);
	if (ret != 0) {
		return ret;
	}

	perf_filter = !IS_ENABLED(CONFIG_BMI260_LOW_POWER_MODE) &&
		      (odr_reg >= BMI260_GYR_ODR_100_HZ);

	ret = bmi260_set_odr(dev, BMI260_REG_GYR_CONF, BMI260_GYR_ODR_MSK, odr_reg,
			     BMI260_PWR_CTRL_GYR_EN, perf_filter, true);
	if (ret == 0) {
		data->gyr_odr = odr_reg;
	}

	return ret;
}

static int bmi260_set_accel_osr(const struct device *dev, const struct sensor_value *val)
{
	struct bmi260_data *data = dev->data;
	uint8_t osr_reg;
	int ret;

	if (!IS_ENABLED(CONFIG_BMI260_LOW_POWER_MODE) && (data->acc_odr >= BMI260_ACC_ODR_100_HZ)) {
		ret = bmi260_osr_to_reg(bmi260_acc_osr_perf_table,
					ARRAY_SIZE(bmi260_acc_osr_perf_table), val, &osr_reg);
	} else {
		ret = bmi260_osr_to_reg(bmi260_acc_osr_power_table,
					ARRAY_SIZE(bmi260_acc_osr_power_table), val, &osr_reg);
	}

	if (ret != 0) {
		return ret;
	}

	return bmi260_update_reg(dev, BMI260_REG_ACC_CONF, BMI260_ACC_BWP_MSK, osr_reg);
}

static int bmi260_set_gyro_osr(const struct device *dev, const struct sensor_value *val)
{
	uint8_t osr_reg;
	int ret;

	ret = bmi260_osr_to_reg(bmi260_gyr_osr_table, ARRAY_SIZE(bmi260_gyr_osr_table), val,
				&osr_reg);
	if (ret != 0) {
		return ret;
	}

	return bmi260_update_reg(dev, BMI260_REG_GYR_CONF, BMI260_GYR_BWP_MSK, osr_reg);
}

static int bmi260_set_accel_range(const struct device *dev, const struct sensor_value *val)
{
	struct bmi260_data *data = dev->data;
	uint8_t range_reg;
	int ret;

	ret = bmi260_range_to_reg(bmi260_acc_range_table, ARRAY_SIZE(bmi260_acc_range_table), val,
				  &range_reg);
	if (ret != 0) {
		return ret;
	}

	ret = bmi260_update_reg(dev, BMI260_REG_ACC_RANGE, BMI260_ACC_RANGE_MSK, range_reg);
	if (ret == 0) {
		data->acc_range = (uint8_t)val->val1;
	}

	return ret;
}

static int bmi260_set_gyro_range(const struct device *dev, const struct sensor_value *val)
{
	struct bmi260_data *data = dev->data;
	uint8_t range_reg;
	int ret;

	ret = bmi260_range_to_reg(bmi260_gyr_range_table, ARRAY_SIZE(bmi260_gyr_range_table), val,
				  &range_reg);
	if (ret != 0) {
		return ret;
	}

	ret = bmi260_update_reg(dev, BMI260_REG_GYR_RANGE, BMI260_GYR_RANGE_MSK, range_reg);
	if (ret == 0) {
		data->gyr_range = (uint16_t)val->val1;
	}

	return ret;
}

static int bmi260_write_config_file(const struct device *dev)
{
	int ret = 0;

	for (size_t index = 0; (index < sizeof(bmi260_config_file)) && (ret == 0);
	     index += BMI260_WR_LEN) {
		size_t len = MIN((size_t)BMI260_WR_LEN, sizeof(bmi260_config_file) - index);
		/* The address is given as an index of 16 bit words, split over two bytes */
		uint16_t word_index = (uint16_t)(index / 2);
		uint8_t addr[2] = {
			(uint8_t)(word_index & BMI260_INIT_ADDR_0_MASK),
			(uint8_t)(word_index >> BMI260_INIT_ADDR_1_SHIFT),
		};

		ret = bmi260_reg_write_with_delay(dev, BMI260_REG_INIT_ADDR_0, addr, sizeof(addr),
						  BMI260_INTER_WRITE_DELAY_US);
		if (ret == 0) {
			ret = bmi260_reg_write_with_delay(dev, BMI260_REG_INIT_DATA,
							  &bmi260_config_file[index], len,
							  BMI260_INTER_WRITE_DELAY_US);
		}
	}

	return ret;
}

static int bmi260_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bmi260_data *data = dev->data;
	uint8_t buf[BMI260_DATA_LEN];
	int ret;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	ret = bmi260_reg_read(dev, BMI260_REG_ACC_X_LSB, buf, sizeof(buf));
	if (ret != 0) {
		data->ax = 0;
		data->ay = 0;
		data->az = 0;
		data->gx = 0;
		data->gy = 0;
		data->gz = 0;

		return ret;
	}

	data->ax = (int16_t)sys_get_le16(&buf[0]);
	data->ay = (int16_t)sys_get_le16(&buf[2]);
	data->az = (int16_t)sys_get_le16(&buf[4]);
	data->gx = (int16_t)sys_get_le16(&buf[BMI260_DATA_GYR_OFFSET]);
	data->gy = (int16_t)sys_get_le16(&buf[BMI260_DATA_GYR_OFFSET + 2]);
	data->gz = (int16_t)sys_get_le16(&buf[BMI260_DATA_GYR_OFFSET + 4]);

	ret = bmi260_reg_read(dev, BMI260_REG_TEMPERATURE_0, buf, sizeof(uint16_t));
	if (ret != 0) {
		data->temp = (int16_t)BMI260_TEMP_INVALID;
		return ret;
	}

	data->temp = (int16_t)sys_get_le16(&buf[0]);

	return 0;
}

static int bmi260_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmi260_data *data = dev->data;
	int64_t micro_c;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		bmi260_channel_accel_convert(val, data->ax, data->acc_range);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		bmi260_channel_accel_convert(val, data->ay, data->acc_range);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		bmi260_channel_accel_convert(val, data->az, data->acc_range);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		bmi260_channel_accel_convert(&val[0], data->ax, data->acc_range);
		bmi260_channel_accel_convert(&val[1], data->ay, data->acc_range);
		bmi260_channel_accel_convert(&val[2], data->az, data->acc_range);
		break;
	case SENSOR_CHAN_GYRO_X:
		bmi260_channel_gyro_convert(val, data->gx, data->gyr_range);
		break;
	case SENSOR_CHAN_GYRO_Y:
		bmi260_channel_gyro_convert(val, data->gy, data->gyr_range);
		break;
	case SENSOR_CHAN_GYRO_Z:
		bmi260_channel_gyro_convert(val, data->gz, data->gyr_range);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		bmi260_channel_gyro_convert(&val[0], data->gx, data->gyr_range);
		bmi260_channel_gyro_convert(&val[1], data->gy, data->gyr_range);
		bmi260_channel_gyro_convert(&val[2], data->gz, data->gyr_range);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		if (data->temp == (int16_t)BMI260_TEMP_INVALID) {
			return -ENODATA;
		}

		micro_c = (((int64_t)data->temp * 1000000LL) / BMI260_TEMP_LSB_PER_DEG) +
			  BMI260_TEMP_CENTER_OFFSET;
		val->val1 = micro_c / 1000000LL;
		val->val2 = micro_c % 1000000LL;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int bmi260_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			return bmi260_set_accel_odr(dev, val);
		case SENSOR_ATTR_OVERSAMPLING:
			return bmi260_set_accel_osr(dev, val);
		case SENSOR_ATTR_FULL_SCALE:
			return bmi260_set_accel_range(dev, val);
		default:
			return -ENOTSUP;
		}
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			return bmi260_set_gyro_odr(dev, val);
		case SENSOR_ATTR_OVERSAMPLING:
			return bmi260_set_gyro_osr(dev, val);
		case SENSOR_ATTR_FULL_SCALE:
			return bmi260_set_gyro_range(dev, val);
		default:
			return -ENOTSUP;
		}
	default:
		return -ENOTSUP;
	}
}

static int bmi260_init(const struct device *dev)
{
	struct bmi260_data *data = dev->data;
	uint8_t chip_id;
	uint8_t cmd;
	uint8_t init_ctrl;
	uint8_t status;
	uint8_t pwr_conf;
	uint8_t pwr_ctrl;
	int ret;

	ret = bmi260_bus_check(dev);
	if (ret < 0) {
		LOG_ERR("Bus check failed: %d", ret);
		return ret;
	}

	/* Reset values of the sensor configuration */
	data->acc_odr = BMI260_ACC_ODR_100_HZ;
	data->acc_range = 8;
	data->gyr_odr = BMI260_GYR_ODR_200_HZ;
	data->gyr_range = 2000;

	k_usleep(BMI260_POWER_ON_TIME);

	ret = bmi260_bus_init(dev);
	if (ret != 0) {
		LOG_ERR("Bus init failed: %d", ret);
		return ret;
	}

	ret = bmi260_reg_read(dev, BMI260_REG_CHIP_ID, &chip_id, 1);
	if (ret != 0) {
		LOG_ERR("CHIP_ID read failed: %d", ret);
		return ret;
	}

	if (chip_id != BMI260_CHIP_ID) {
		LOG_ERR("Unexpected chip id (0x%02x). Expected (0x%02x)", chip_id, BMI260_CHIP_ID);
		return -EIO;
	}

	cmd = BMI260_CMD_SOFT_RESET;
	ret = bmi260_reg_write(dev, BMI260_REG_CMD, &cmd, 1);
	if (ret != 0) {
		LOG_ERR("Soft reset write failed: %d", ret);
		return ret;
	}

	k_usleep(BMI260_SOFT_RESET_TIME);

	/* The bus interface has to be initialized again after a soft reset */
	ret = bmi260_bus_init(dev);
	if (ret != 0) {
		LOG_ERR("Could not initiate bus communication after reset: %d", ret);
		return ret;
	}

	/* Advanced power save has to be disabled while the configuration is loaded */
	ret = bmi260_reg_read(dev, BMI260_REG_PWR_CONF, &pwr_conf, 1);
	if (ret != 0) {
		LOG_ERR("PWR_CONF read failed: %d", ret);
		return ret;
	}

	/* After a soft reset the sensor is in advanced power save mode, which ignores
	 * accesses that are less than 450 us apart.
	 */
	k_usleep(BMI260_INTER_WRITE_DELAY_US);

	pwr_conf = bmi260_set_field(pwr_conf, BMI260_PWR_CONF_ADV_PWR_SAVE_MSK,
				    BMI260_PWR_CONF_ADV_PWR_SAVE_DIS);
	ret = bmi260_reg_write_with_delay(dev, BMI260_REG_PWR_CONF, &pwr_conf, 1,
					  BMI260_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		LOG_ERR("PWR_CONF write failed: %d", ret);
		return ret;
	}

	init_ctrl = BMI260_PREPARE_CONFIG_LOAD;
	ret = bmi260_reg_write(dev, BMI260_REG_INIT_CTRL, &init_ctrl, 1);
	if (ret != 0) {
		LOG_ERR("INIT_CTRL prepare failed: %d", ret);
		return ret;
	}

	ret = bmi260_write_config_file(dev);
	if (ret != 0) {
		LOG_ERR("Config file write failed: %d", ret);
		return ret;
	}

	init_ctrl = BMI260_COMPLETE_CONFIG_LOAD;
	ret = bmi260_reg_write(dev, BMI260_REG_INIT_CTRL, &init_ctrl, 1);
	if (ret != 0) {
		LOG_ERR("INIT_CTRL complete failed: %d", ret);
		return ret;
	}

	/* Wait for the sensor to report that the configuration was loaded */
	for (int tries = 0; tries <= BMI260_CONFIG_FILE_RETRIES; tries++) {
		ret = bmi260_reg_read(dev, BMI260_REG_INTERNAL_STATUS, &status, 1);
		if (ret != 0) {
			LOG_ERR("INTERNAL_STATUS read failed: %d", ret);
			return ret;
		}

		if ((status & BMI260_INST_MESSAGE_MSK) == BMI260_INST_MESSAGE_INIT_OK) {
			break;
		}

		if (tries == BMI260_CONFIG_FILE_RETRIES) {
			LOG_ERR("Config load timeout (INTERNAL_STATUS 0x%02x)", status);
			return -EIO;
		}

		k_usleep(BMI260_CONFIG_FILE_POLL_PERIOD_US);
	}

	/* Enable the internal temperature sensor */
	ret = bmi260_reg_read(dev, BMI260_REG_PWR_CTRL, &pwr_ctrl, 1);
	if (ret != 0) {
		LOG_ERR("PWR_CTRL read failed: %d", ret);
		return ret;
	}

	pwr_ctrl |= BMI260_PWR_CTRL_TEMP_EN;
	ret = bmi260_reg_write_with_delay(dev, BMI260_REG_PWR_CTRL, &pwr_ctrl, 1,
					  BMI260_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		LOG_ERR("PWR_CTRL write failed: %d", ret);
		return ret;
	}

	pwr_conf = bmi260_set_field(pwr_conf, BMI260_PWR_CONF_ADV_PWR_SAVE_MSK,
				    BMI260_PWR_CONF_ADV_PWR_SAVE_EN);

	return bmi260_reg_write_with_delay(dev, BMI260_REG_PWR_CONF, &pwr_conf, 1,
					   BMI260_INTER_WRITE_DELAY_US);
}

static DEVICE_API(sensor, bmi260_driver_api) = {
	.sample_fetch = bmi260_sample_fetch,
	.channel_get = bmi260_channel_get,
	.attr_set = bmi260_attr_set,
};

/* Initializes a struct bmi260_config for an instance on a SPI bus. */
#define BMI260_CONFIG_SPI(inst)                                                                    \
	.bus.spi = SPI_DT_SPEC_INST_GET(inst, BMI260_SPI_OPERATION), .bus_io = &bmi260_bus_io_spi,

/* Initializes a struct bmi260_config for an instance on an I2C bus. */
#define BMI260_CONFIG_I2C(inst) .bus.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_io = &bmi260_bus_io_i2c,

#define BMI260_CREATE_INST(inst)                                                                   \
	static struct bmi260_data bmi260_data_##inst;                                              \
                                                                                                   \
	static const struct bmi260_config bmi260_config_##inst = {                                 \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (BMI260_CONFIG_SPI(inst)),                  \
			    (BMI260_CONFIG_I2C(inst)))                                             \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmi260_init, NULL, &bmi260_data_##inst,                 \
				     &bmi260_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &bmi260_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BMI260_CREATE_INST)
