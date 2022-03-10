/* Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "bmi160.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(BMI160, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "BMI160 driver enabled without any devices"
#endif

int bmi160_read(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len)
{
	const struct bmi160_cfg *cfg = dev->config;

	return cfg->bus_io->read(dev, reg_addr, buf, len);
}

int bmi160_byte_read(const struct device *dev, uint8_t reg_addr, uint8_t *byte)
{
	return bmi160_read(dev, reg_addr, byte, 1);
}

int bmi160_word_read(const struct device *dev, uint8_t reg_addr, uint16_t *word)
{
	int rc;

	rc = bmi160_read(dev, reg_addr, word, 2);
	if (rc != 0) {
		return rc;
	}

	*word = sys_le16_to_cpu(*word);

	return 0;
}

int bmi160_write(const struct device *dev, uint8_t reg_addr, void *buf, uint8_t len)
{
	const struct bmi160_cfg *cfg = dev->config;

	return cfg->bus_io->write(dev, reg_addr, buf, len);
}

int bmi160_byte_write(const struct device *dev, uint8_t reg_addr, uint8_t byte)
{
	return bmi160_write(dev, reg_addr & BMI160_REG_MASK, &byte, 1);
}

int bmi160_word_write(const struct device *dev, uint8_t reg_addr, uint16_t word)
{
	uint8_t tx_word[2] = {(uint8_t)(word & 0xff), (uint8_t)(word >> 8)};

	return bmi160_write(dev, reg_addr & BMI160_REG_MASK, tx_word, 2);
}

int bmi160_reg_field_update(const struct device *dev, uint8_t reg_addr, uint8_t pos, uint8_t mask,
			    uint8_t val)
{
	uint8_t old_val;

	if (bmi160_byte_read(dev, reg_addr, &old_val) < 0) {
		return -EIO;
	}

	return bmi160_byte_write(dev, reg_addr, (old_val & ~mask) | ((val << pos) & mask));
}

static int bmi160_pmu_set(const struct device *dev, union bmi160_pmu_status *pmu_sts)
{
	struct {
		uint8_t cmd;
		uint16_t delay_us; /* values taken from page 82 */
	} cmds[] = {{BMI160_CMD_PMU_MAG | pmu_sts->mag, 350},
		    {BMI160_CMD_PMU_ACC | pmu_sts->acc, 3200},
		    {BMI160_CMD_PMU_GYR | pmu_sts->gyr, 55000}};
	size_t i;

	for (i = 0; i < ARRAY_SIZE(cmds); i++) {
		union bmi160_pmu_status sts;
		bool pmu_set = false;

		if (bmi160_byte_write(dev, BMI160_REG_CMD, cmds[i].cmd) < 0) {
			return -EIO;
		}

		/*
		 * Cannot use a timer here since this is called from the
		 * init function and the timeouts were not initialized yet.
		 */
		k_busy_wait(cmds[i].delay_us);

		/* make sure the PMU_STATUS was set, though */
		do {
			if (bmi160_byte_read(dev, BMI160_REG_PMU_STATUS, &sts.raw) < 0) {
				return -EIO;
			}

			if (i == 0) {
				pmu_set = (pmu_sts->mag == sts.mag);
			} else if (i == 1) {
				pmu_set = (pmu_sts->acc == sts.acc);
			} else {
				pmu_set = (pmu_sts->gyr == sts.gyr);
			}

		} while (!pmu_set);
	}

	/* set the undersampling flag for accelerometer */
	return bmi160_reg_field_update(dev, BMI160_REG_ACC_CONF, BMI160_ACC_CONF_US_POS,
				       BMI160_ACC_CONF_US_MASK, pmu_sts->acc != BMI160_PMU_NORMAL);
}

#if defined(CONFIG_BMI160_GYRO_ODR_RUNTIME) || defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME)
/*
 * Output data rate map with allowed frequencies:
 * freq = freq_int + freq_milli / 1000
 *
 * Since we don't need a finer frequency resolution than milliHz, use uint16_t
 * to save some flash.
 */
struct {
	uint16_t freq_int;
	uint16_t freq_milli; /* User should convert to uHz before setting the
			      * SENSOR_ATTR_SAMPLING_FREQUENCY attribute.
			      */
} bmi160_odr_map[] = {
	{0, 0},	 {0, 780}, {1, 562}, {3, 120}, {6, 250}, {12, 500}, {25, 0},
	{50, 0}, {100, 0}, {200, 0}, {400, 0}, {800, 0}, {1600, 0}, {3200, 0},
};

int bmi160_freq_to_odr_val(uint16_t freq_int, uint16_t freq_milli)
{
	size_t i;

	/* An ODR of 0 Hz is not allowed */
	if (freq_int == 0U && freq_milli == 0U) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(bmi160_odr_map); i++) {
		if (freq_int < bmi160_odr_map[i].freq_int ||
		    (freq_int == bmi160_odr_map[i].freq_int &&
		     freq_milli <= bmi160_odr_map[i].freq_milli)) {
			return i;
		}
	}

	return -EINVAL;
}
#endif

#if defined(CONFIG_BMI160_ACCEL_ODR_RUNTIME)
int bmi160_acc_odr_set(const struct device *dev, uint16_t freq_int, uint16_t freq_milli)
{
	struct bmi160_data *data = dev->data;
	int odr = bmi160_freq_to_odr_val(freq_int, freq_milli);

	if (odr < 0) {
		return odr;
	}

	/* some odr values cannot be set in certain power modes */
	if ((data->pmu_sts.acc == BMI160_PMU_NORMAL && odr < BMI160_ODR_25_2) ||
	    (data->pmu_sts.acc == BMI160_PMU_LOW_POWER && odr < BMI160_ODR_25_32) ||
	    odr > BMI160_ODR_1600) {
		return -ENOTSUP;
	}

	return bmi160_reg_field_update(dev, BMI160_REG_ACC_CONF, BMI160_ACC_CONF_ODR_POS,
				       BMI160_ACC_CONF_ODR_MASK, (uint8_t)odr);
}
#endif

static const struct bmi160_range bmi160_acc_range_map[] = {
	{2, BMI160_ACC_RANGE_2G},
	{4, BMI160_ACC_RANGE_4G},
	{8, BMI160_ACC_RANGE_8G},
	{16, BMI160_ACC_RANGE_16G},
};
#define BMI160_ACC_RANGE_MAP_SIZE ARRAY_SIZE(bmi160_acc_range_map)

static const struct bmi160_range bmi160_gyr_range_map[] = {
	{2000, BMI160_GYR_RANGE_2000DPS}, {1000, BMI160_GYR_RANGE_1000DPS},
	{500, BMI160_GYR_RANGE_500DPS},	  {250, BMI160_GYR_RANGE_250DPS},
	{125, BMI160_GYR_RANGE_125DPS},
};
#define BMI160_GYR_RANGE_MAP_SIZE ARRAY_SIZE(bmi160_gyr_range_map)

#if defined(CONFIG_BMI160_ACCEL_RANGE_RUNTIME) || defined(CONFIG_BMI160_GYRO_RANGE_RUNTIME)
int32_t bmi160_range_to_reg_val(uint16_t range, const struct bmi160_range *range_map,
				uint16_t range_map_size)
{
	int i;

	for (i = 0; i < range_map_size; i++) {
		if (range <= range_map[i].range) {
			return range_map[i].reg_val;
		}
	}

	return -EINVAL;
}
#endif

uint8_t bmi160_reg_val_to_range_index(uint8_t reg_val, const struct bmi160_range *range_map,
				      uint16_t range_map_size)
{
	int i;

	for (i = 0; i < range_map_size; i++) {
		if (reg_val == range_map[i].reg_val) {
			return i;
		}
	}

	__ASSERT_NO_MSG(false);
	return 0xff;
}

uint8_t bmi160_acc_reg_val_to_range_index(uint8_t reg_val)
{
	return bmi160_reg_val_to_range_index(reg_val, bmi160_acc_range_map,
					     BMI160_ACC_RANGE_MAP_SIZE);
}

uint8_t bmi160_gyr_reg_val_to_range_index(uint8_t reg_val)
{
	return bmi160_reg_val_to_range_index(reg_val, bmi160_gyr_range_map,
					     BMI160_GYR_RANGE_MAP_SIZE);
}

int bmi160_do_calibration(const struct device *dev, uint8_t foc_conf)
{
	if (bmi160_byte_write(dev, BMI160_REG_FOC_CONF, foc_conf) < 0) {
		return -EIO;
	}

	if (bmi160_byte_write(dev, BMI160_REG_CMD, BMI160_CMD_START_FOC) < 0) {
		return -EIO;
	}

	k_busy_wait(250000); /* calibration takes a maximum of 250ms */

	return 0;
}

#if defined(CONFIG_BMI160_ACCEL_RANGE_RUNTIME) || defined(CONFIG_SENSOR_VERSION_2)
int bmi160_acc_range_set(const struct device *dev, int32_t range)
{
	struct bmi160_data *data = dev->data;
	int32_t reg_val =
		bmi160_range_to_reg_val(range, bmi160_acc_range_map, BMI160_ACC_RANGE_MAP_SIZE);

	if (reg_val < 0) {
		return reg_val;
	}

	if (bmi160_byte_write(dev, BMI160_REG_ACC_RANGE, reg_val & 0xff) < 0) {
		return -EIO;
	}

	data->scale.acc = BMI160_ACC_SCALE(range);

	return 0;
}
#endif

#if defined(CONFIG_BMI160_GYRO_ODR_RUNTIME)
int bmi160_gyr_odr_set(const struct device *dev, uint16_t freq_int, uint16_t freq_milli)
{
	int odr = bmi160_freq_to_odr_val(freq_int, freq_milli);

	if (odr < 0) {
		return odr;
	}

	if (odr < BMI160_ODR_25 || odr > BMI160_ODR_3200) {
		return -ENOTSUP;
	}

	return bmi160_reg_field_update(dev, BMI160_REG_GYR_CONF, BMI160_GYR_CONF_ODR_POS,
				       BMI160_GYR_CONF_ODR_MASK, (uint8_t)odr);
}
#endif

#if defined(CONFIG_BMI160_GYRO_RANGE_RUNTIME) || defined(CONFIG_SENSOR_VERSION_2)
int bmi160_gyr_range_set(const struct device *dev, uint16_t range)
{
	struct bmi160_data *data = dev->data;
	int32_t reg_val =
		bmi160_range_to_reg_val(range, bmi160_gyr_range_map, BMI160_GYR_RANGE_MAP_SIZE);

	if (reg_val < 0) {
		return reg_val;
	}

	if (bmi160_byte_write(dev, BMI160_REG_GYR_RANGE, reg_val) < 0) {
		return -EIO;
	}

	data->scale.gyr = BMI160_GYR_SCALE(range);

	return 0;
}
#endif

int bmi160_init(const struct device *dev)
{
	const struct bmi160_cfg *cfg = dev->config;
	struct bmi160_data *data = dev->data;
	uint8_t val = 0U;
	int32_t acc_range, gyr_range;

	if (!cfg->bus_io->ready(dev)) {
		LOG_ERR("Bus not ready");
		return -EINVAL;
	}

	/* reboot the chip */
	if (bmi160_byte_write(dev, BMI160_REG_CMD, BMI160_CMD_SOFT_RESET) < 0) {
		LOG_DBG("Cannot reboot chip.");
		return -EIO;
	}

	k_busy_wait(1000);

	/* do a dummy read from 0x7F to activate SPI */
	if (bmi160_byte_read(dev, BMI160_SPI_START, &val) < 0) {
		LOG_DBG("Cannot read from 0x7F..");
		return -EIO;
	}

	k_busy_wait(150);

	if (bmi160_byte_read(dev, BMI160_REG_CHIPID, &val) < 0) {
		LOG_DBG("Failed to read chip id.");
		return -EIO;
	}

	if (val != BMI160_CHIP_ID) {
		LOG_DBG("Unsupported chip detected (0x%x)!", val);
		return -ENODEV;
	}

	/* set default PMU for gyro, accelerometer */
	data->pmu_sts.gyr = BMI160_DEFAULT_PMU_GYR;
	data->pmu_sts.acc = BMI160_DEFAULT_PMU_ACC;
	/* compass not supported, yet */
	data->pmu_sts.mag = BMI160_PMU_SUSPEND;

	/*
	 * The next command will take around 100ms (contains some necessary busy
	 * waits), but we cannot do it in a separate thread since we need to
	 * guarantee the BMI is up and running, before the app's main() is
	 * called.
	 */
	if (bmi160_pmu_set(dev, &data->pmu_sts) < 0) {
		LOG_DBG("Failed to set power mode.");
		return -EIO;
	}

	/* set accelerometer default range */
	if (bmi160_byte_write(dev, BMI160_REG_ACC_RANGE, BMI160_DEFAULT_RANGE_ACC) < 0) {
		LOG_DBG("Cannot set default range for accelerometer.");
		return -EIO;
	}

	acc_range = bmi160_acc_reg_val_to_range(BMI160_DEFAULT_RANGE_ACC);

	data->scale.acc = BMI160_ACC_SCALE(acc_range);

	/* set gyro default range */
	if (bmi160_byte_write(dev, BMI160_REG_GYR_RANGE, BMI160_DEFAULT_RANGE_GYR) < 0) {
		LOG_DBG("Cannot set default range for gyroscope.");
		return -EIO;
	}

	gyr_range = bmi160_gyr_reg_val_to_range(BMI160_DEFAULT_RANGE_GYR);

	data->scale.gyr = BMI160_GYR_SCALE(gyr_range);

	if (bmi160_reg_field_update(dev, BMI160_REG_ACC_CONF, BMI160_ACC_CONF_ODR_POS,
				    BMI160_ACC_CONF_ODR_MASK, BMI160_DEFAULT_ODR_ACC) < 0) {
		LOG_DBG("Failed to set accel's default ODR.");
		return -EIO;
	}

	if (bmi160_reg_field_update(dev, BMI160_REG_GYR_CONF, BMI160_GYR_CONF_ODR_POS,
				    BMI160_GYR_CONF_ODR_MASK, BMI160_DEFAULT_ODR_GYR) < 0) {
		LOG_DBG("Failed to set gyro's default ODR.");
		return -EIO;
	}

#ifdef CONFIG_BMI160_TRIGGER
	if (bmi160_trigger_mode_init(dev) < 0) {
		LOG_DBG("Cannot set up trigger mode.");
		return -EINVAL;
	}
#endif

	return 0;
}

int bmi160_sample_fetch(const struct device *dev)
{
	struct bmi160_data *data = dev->data;
	uint8_t status = 0;

	while ((status & BMI160_DATA_READY_BIT_MASK) == 0) {
		if (bmi160_byte_read(dev, BMI160_REG_STATUS, &status) < 0) {
			return -EIO;
		}
	}

	if (bmi160_read(dev, BMI160_SAMPLE_BURST_READ_ADDR, data->sample.raw, BMI160_BUF_SIZE) <
	    0) {
		return -EIO;
	}

	/* convert samples to cpu endianness */
	for (size_t i = 0; i < BMI160_SAMPLE_SIZE; i += 2) {
		uint16_t *sample = (uint16_t *)&data->sample.raw[i];

		*sample = sys_le16_to_cpu(*sample);
	}

	return 0;
}
