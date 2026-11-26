/*
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmi260

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "bmi260.h"
#include "bmi260_config_file.h"
#include "bmi260_reg_map.h"

LOG_MODULE_REGISTER(bmi260, CONFIG_SENSOR_LOG_LEVEL);

/* Pulls the six accelerometer and six gyroscope bytes out of one burst read. */
static void bmi260_unpack_axes(const uint8_t *raw, int16_t *out)
{
	for (int axis = 0; axis < 3; axis++) {
		out[axis] = (int16_t)sys_get_le16(&raw[axis * 2]);
	}
}

static int bmi260_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bmi260_data *data = dev->data;
	bool want_motion = (chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_ACCEL_XYZ) ||
			   (chan == SENSOR_CHAN_GYRO_XYZ);
	bool want_temp = (chan == SENSOR_CHAN_ALL) || (chan == SENSOR_CHAN_DIE_TEMP);
	int ret;

	if (!want_motion && !want_temp) {
		return -ENOTSUP;
	}

	if (want_motion) {
		/*
		 * The accelerometer and gyroscope registers are adjacent, so one
		 * read keeps the two sets from straddling a sample boundary.
		 */
		uint8_t raw[12];

		ret = bmi260_reg_read(dev, BMI260_REG_ACC_X_LSB, raw, sizeof(raw));
		if (ret != 0) {
			return ret;
		}

		bmi260_unpack_axes(&raw[0], data->acc);
		bmi260_unpack_axes(&raw[6], data->gyr);

		LOG_DBG("acc raw %6d %6d %6d  gyr raw %6d %6d %6d", data->acc[0], data->acc[1],
			data->acc[2], data->gyr[0], data->gyr[1], data->gyr[2]);
	}

	if (want_temp) {
		uint8_t raw[2];

		ret = bmi260_reg_read(dev, BMI260_REG_TEMPERATURE_0, raw, sizeof(raw));
		if (ret != 0) {
			return ret;
		}

		data->temp = (int16_t)sys_get_le16(raw);
	}

	return 0;
}

/*
 * Counts are full scale at INT16_MAX, so a reading is
 * count * range / INT16_MAX. The arithmetic is done in micro-units on 64 bits
 * to keep the rounding away from the result and the sign on both fields of the
 * sensor_value, which is what the API asks for.
 */
static void bmi260_to_sensor_value(int64_t micro, struct sensor_value *val)
{
	val->val1 = (int32_t)(micro / 1000000);
	val->val2 = (int32_t)(micro % 1000000);
}

static void bmi260_accel_to_ms2(const struct bmi260_data *data, int16_t raw,
				struct sensor_value *val)
{
	/* SENSOR_G is micro-m/s^2 per g, so this lands directly in micro-units. */
	int64_t micro = ((int64_t)raw * data->acc_range_g * SENSOR_G) / INT16_MAX;

	bmi260_to_sensor_value(micro, val);
}

static void bmi260_gyro_to_rad_per_s(const struct bmi260_data *data, int16_t raw,
				     struct sensor_value *val)
{
	/* SENSOR_PI / 180 degrees gives radians; scaled by 1e6 for micro-units. */
	int64_t micro = ((int64_t)raw * data->gyr_range_dps * SENSOR_PI) / (180LL * INT16_MAX);

	bmi260_to_sensor_value(micro, val);
}

static int bmi260_temp_to_celsius(int16_t raw, struct sensor_value *val)
{
	int64_t micro;

	if ((uint16_t)raw == BMI260_TEMP_INVALID) {
		return -ENODATA;
	}

	micro = BMI260_TEMP_CENTER_OFFSET + (((int64_t)raw * 1000000LL) / BMI260_TEMP_LSB_PER_DEG);
	bmi260_to_sensor_value(micro, val);

	return 0;
}

static int bmi260_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct bmi260_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		bmi260_accel_to_ms2(data, data->acc[chan - SENSOR_CHAN_ACCEL_X], val);
		return 0;
	case SENSOR_CHAN_ACCEL_XYZ:
		for (int axis = 0; axis < 3; axis++) {
			bmi260_accel_to_ms2(data, data->acc[axis], &val[axis]);
		}
		return 0;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		bmi260_gyro_to_rad_per_s(data, data->gyr[chan - SENSOR_CHAN_GYRO_X], val);
		return 0;
	case SENSOR_CHAN_GYRO_XYZ:
		for (int axis = 0; axis < 3; axis++) {
			bmi260_gyro_to_rad_per_s(data, data->gyr[axis], &val[axis]);
		}
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		return bmi260_temp_to_celsius(data->temp, val);
	default:
		return -ENOTSUP;
	}
}

static int bmi260_verify_chip_id(const struct device *dev)
{
	uint8_t chip_id;
	int ret;

	ret = bmi260_reg_read(dev, BMI260_REG_CHIP_ID, &chip_id, sizeof(chip_id));
	if (ret != 0) {
		LOG_ERR("Could not read the chip id (%d)", ret);
		return ret;
	}

	if (chip_id != BMI260_CHIP_ID) {
		LOG_ERR("Chip id 0x%02x answered, expected 0x%02x", chip_id, BMI260_CHIP_ID);
		return -ENODEV;
	}

	LOG_DBG("Chip id 0x%02x", chip_id);

	return 0;
}

static int bmi260_soft_reset(const struct device *dev)
{
	static const uint8_t cmd = BMI260_CMD_SOFT_RESET;
	int ret;

	ret = bmi260_reg_write(dev, BMI260_REG_CMD, &cmd, sizeof(cmd));
	if (ret != 0) {
		LOG_ERR("Could not reset the device (%d)", ret);
		return ret;
	}

	k_usleep(BMI260_SOFT_RESET_TIME);

	LOG_DBG("Soft reset issued");

	return 0;
}

/*
 * Advanced power save lets the device ignore accesses that arrive closer
 * together than BMI260_TRANSC_DELAY_SUSPEND, so it is switched off for as
 * long as the driver is configuring the device and switched back on after.
 */
static int bmi260_set_power_save(const struct device *dev, bool enable)
{
	uint8_t pwr_conf;
	int ret;

	ret = bmi260_reg_read(dev, BMI260_REG_PWR_CONF, &pwr_conf, sizeof(pwr_conf));
	if (ret != 0) {
		return ret;
	}

	pwr_conf = BMI260_SET_BITS_POS_0(pwr_conf, BMI260_PWR_CONF_ADV_PWR_SAVE,
					 enable ? BMI260_PWR_CONF_ADV_PWR_SAVE_EN
						: BMI260_PWR_CONF_ADV_PWR_SAVE_DIS);

	ret = bmi260_reg_write(dev, BMI260_REG_PWR_CONF, &pwr_conf, sizeof(pwr_conf));
	if (ret != 0) {
		return ret;
	}

	k_usleep(BMI260_TRANSC_DELAY_SUSPEND);

	return 0;
}

BUILD_ASSERT(sizeof(bmi260_config_file) == BMI260_CONFIG_FILE_SIZE,
	     "the configuration image is not the size the driver expects");

/*
 * Hands the configuration image to the feature engine in chunks. The address
 * written alongside each one counts 16-bit words, split across two registers:
 * the low nibble in INIT_ADDR_0 and the rest in INIT_ADDR_1.
 */
static int bmi260_write_config_file(const struct device *dev)
{
	for (uint16_t index = 0; index < sizeof(bmi260_config_file); index += BMI260_WR_LEN) {
		uint16_t word = index / 2U;
		uint8_t addr[2] = {
			(uint8_t)(word & 0x0FU),
			(uint8_t)(word >> 4U),
		};
		uint16_t len = MIN((uint16_t)BMI260_WR_LEN,
				   (uint16_t)(sizeof(bmi260_config_file) - index));
		int ret;

		ret = bmi260_reg_write_with_delay(dev, BMI260_REG_INIT_ADDR_0, addr, sizeof(addr),
						  BMI260_INTER_WRITE_DELAY_US);
		if (ret != 0) {
			LOG_ERR("Configuration upload failed at byte %u (%d)", index, ret);
			return ret;
		}

		ret = bmi260_reg_write_with_delay(dev, BMI260_REG_INIT_DATA,
						  &bmi260_config_file[index], len,
						  BMI260_INTER_WRITE_DELAY_US);
		if (ret != 0) {
			LOG_ERR("Configuration upload failed at byte %u (%d)", index, ret);
			return ret;
		}
	}

	return 0;
}

/* The engine takes a moment to digest the image and then reports on itself. */
static int bmi260_wait_for_init_ok(const struct device *dev)
{
	for (uint8_t tries = 0; tries < BMI260_CONFIG_FILE_RETRIES; tries++) {
		uint8_t status;
		int ret;

		ret = bmi260_reg_read(dev, BMI260_REG_INTERNAL_STATUS, &status, sizeof(status));
		if (ret != 0) {
			return ret;
		}

		if ((status & BMI260_INST_MESSAGE_MSK) == BMI260_INST_MESSAGE_INIT_OK) {
			LOG_DBG("Feature engine started after %u poll(s)", tries + 1U);
			return 0;
		}

		k_usleep(BMI260_CONFIG_FILE_POLL_PERIOD_US);
	}

	LOG_ERR("The feature engine did not start after the configuration upload");

	return -EIO;
}

static int bmi260_upload_config_file(const struct device *dev)
{
	static const uint8_t prepare = BMI260_PREPARE_CONFIG_LOAD;
	static const uint8_t complete = BMI260_COMPLETE_CONFIG_LOAD;
	int ret;

	ret = bmi260_reg_write(dev, BMI260_REG_INIT_CTRL, &prepare, sizeof(prepare));
	if (ret != 0) {
		return ret;
	}

	LOG_DBG("Uploading %u bytes of configuration in %u byte chunks",
		(unsigned int)sizeof(bmi260_config_file), BMI260_WR_LEN);

	ret = bmi260_write_config_file(dev);
	if (ret != 0) {
		return ret;
	}

	ret = bmi260_reg_write(dev, BMI260_REG_INIT_CTRL, &complete, sizeof(complete));
	if (ret != 0) {
		return ret;
	}

	return bmi260_wait_for_init_ok(dev);
}

/* Range in g to the ACC_RANGE field; the field counts doublings from 2 g. */
static int bmi260_acc_range_to_field(uint8_t range_g, uint8_t *field)
{
	switch (range_g) {
	case 2:
		*field = BMI260_ACC_RANGE_2G;
		return 0;
	case 4:
		*field = BMI260_ACC_RANGE_4G;
		return 0;
	case 8:
		*field = BMI260_ACC_RANGE_8G;
		return 0;
	case 16:
		*field = BMI260_ACC_RANGE_16G;
		return 0;
	default:
		return -EINVAL;
	}
}

/* Range in degrees per second to the GYR_RANGE field; it halves as it counts up. */
static int bmi260_gyr_range_to_field(uint16_t range_dps, uint8_t *field)
{
	switch (range_dps) {
	case 2000:
		*field = BMI260_GYR_RANGE_2000DPS;
		return 0;
	case 1000:
		*field = BMI260_GYR_RANGE_1000DPS;
		return 0;
	case 500:
		*field = BMI260_GYR_RANGE_500DPS;
		return 0;
	case 250:
		*field = BMI260_GYR_RANGE_250DPS;
		return 0;
	case 125:
		*field = BMI260_GYR_RANGE_125DPS;
		return 0;
	default:
		return -EINVAL;
	}
}

/*
 * Applies the ranges the devicetree asked for. The cached copies used for
 * conversion are only updated once the device has taken the setting.
 */
static int bmi260_apply_ranges(const struct device *dev, uint8_t acc_range_g,
			       uint16_t gyr_range_dps)
{
	struct bmi260_data *data = dev->data;
	uint8_t field;
	int ret;

	ret = bmi260_acc_range_to_field(acc_range_g, &field);
	if (ret != 0) {
		return ret;
	}

	ret = bmi260_reg_write(dev, BMI260_REG_ACC_RANGE, &field, sizeof(field));
	if (ret != 0) {
		return ret;
	}

	data->acc_range_g = acc_range_g;

	ret = bmi260_gyr_range_to_field(gyr_range_dps, &field);
	if (ret != 0) {
		return ret;
	}

	ret = bmi260_reg_write(dev, BMI260_REG_GYR_RANGE, &field, sizeof(field));
	if (ret != 0) {
		return ret;
	}

	data->gyr_range_dps = gyr_range_dps;

	LOG_DBG("Ranges set to %u g and %u dps", acc_range_g, gyr_range_dps);

	return 0;
}

/* Output data rate in Hz to the ODR field. Rates below 25 Hz are accelerometer only. */
static int bmi260_odr_to_field(int32_t hz, bool accel, uint8_t *field)
{
	switch (hz) {
	case 25:
		*field = BMI260_ACC_ODR_25_HZ;
		return 0;
	case 50:
		*field = BMI260_ACC_ODR_50_HZ;
		return 0;
	case 100:
		*field = BMI260_ACC_ODR_100_HZ;
		return 0;
	case 200:
		*field = BMI260_ACC_ODR_200_HZ;
		return 0;
	case 400:
		*field = BMI260_ACC_ODR_400_HZ;
		return 0;
	case 800:
		*field = BMI260_ACC_ODR_800_HZ;
		return 0;
	case 1600:
		*field = BMI260_ACC_ODR_1600_HZ;
		return 0;
	case 3200:
		if (accel) {
			return -EINVAL;
		}
		*field = BMI260_GYR_ODR_3200_HZ;
		return 0;
	default:
		return -EINVAL;
	}
}

static int bmi260_set_odr(const struct device *dev, bool accel, int32_t hz)
{
	uint8_t reg = accel ? BMI260_REG_ACC_CONF : BMI260_REG_GYR_CONF;
	uint8_t conf;
	uint8_t msk;
	uint8_t field;
	int ret;

	ret = bmi260_odr_to_field(hz, accel, &field);
	if (ret != 0) {
		return ret;
	}

	ret = bmi260_reg_read(dev, reg, &conf, sizeof(conf));
	if (ret != 0) {
		return ret;
	}

	/*
	 * Both configuration registers carry the rate in their low nibble,
	 * but under their own names, so the mask is picked per sensor rather
	 * than one standing in for the other.
	 */
	msk = accel ? BMI260_ACC_ODR_MSK : BMI260_GYR_ODR_MSK;
	conf = (conf & ~msk) | (field & msk);

	return bmi260_reg_write(dev, reg, &conf, sizeof(conf));
}

/* Brings the accelerometer, the gyroscope and the thermometer out of suspend. */
static int bmi260_enable_sensors(const struct device *dev)
{
	static const uint8_t pwr_ctrl =
		BMI260_PWR_CTRL_ACC_EN | BMI260_PWR_CTRL_GYR_EN | BMI260_PWR_CTRL_TEMP_EN;

	LOG_DBG("Enabling accelerometer, gyroscope and thermometer");

	return bmi260_reg_write(dev, BMI260_REG_PWR_CTRL, &pwr_ctrl, sizeof(pwr_ctrl));
}

static int bmi260_init(const struct device *dev)
{
	const struct bmi260_config *cfg = dev->config;
	int ret;

	ret = bmi260_bus_check(dev);
	if (ret != 0) {
		LOG_ERR("Bus is not ready (%d)", ret);
		return ret;
	}

	k_usleep(BMI260_POWER_ON_TIME);

	ret = bmi260_bus_init(dev);
	if (ret != 0) {
		LOG_ERR("Could not prepare the bus (%d)", ret);
		return ret;
	}

	ret = bmi260_verify_chip_id(dev);
	if (ret != 0) {
		return ret;
	}

	ret = bmi260_soft_reset(dev);
	if (ret != 0) {
		return ret;
	}

	/*
	 * The reset returns the device to listening on I2C, so the interface
	 * has to be latched a second time. Leaving this out makes every later
	 * access on a SPI instance talk to a device in the wrong mode.
	 */
	ret = bmi260_bus_init(dev);
	if (ret != 0) {
		LOG_ERR("Could not prepare the bus after the reset (%d)", ret);
		return ret;
	}

	ret = bmi260_set_power_save(dev, false);
	if (ret != 0) {
		LOG_ERR("Could not disable advanced power save (%d)", ret);
		return ret;
	}

	ret = bmi260_upload_config_file(dev);
	if (ret != 0) {
		return ret;
	}

	ret = bmi260_apply_ranges(dev, cfg->acc_range_g, cfg->gyr_range_dps);
	if (ret != 0) {
		LOG_ERR("Could not apply the configured ranges (%d)", ret);
		return ret;
	}

	/*
	 * The rates go on after the ranges: setting a rate also settles the
	 * power mode, so it is the last word on how the device runs.
	 */
	ret = bmi260_set_odr(dev, true, cfg->acc_odr_hz);
	if (ret != 0) {
		LOG_ERR("Could not set the accelerometer rate to %u Hz (%d)", cfg->acc_odr_hz,
			ret);
		return ret;
	}

	ret = bmi260_set_odr(dev, false, cfg->gyr_odr_hz);
	if (ret != 0) {
		LOG_ERR("Could not set the gyroscope rate to %u Hz (%d)", cfg->gyr_odr_hz, ret);
		return ret;
	}

	LOG_DBG("Rates set to %u Hz and %u Hz", cfg->acc_odr_hz, cfg->gyr_odr_hz);

	ret = bmi260_enable_sensors(dev);
	if (ret != 0) {
		LOG_ERR("Could not enable the sensors (%d)", ret);
		return ret;
	}

	ret = bmi260_set_power_save(dev, true);
	if (ret != 0) {
		LOG_ERR("Could not restore advanced power save (%d)", ret);
		return ret;
	}

	LOG_DBG("Ready: %u g, %u dps", cfg->acc_range_g, cfg->gyr_range_dps);

	if (IS_ENABLED(CONFIG_BMI260_LOG_CONFIG)) {
		uint8_t conf[4];
		uint8_t offset[8];
		uint8_t err[2];

		(void)bmi260_reg_read(dev, BMI260_REG_ACC_CONF, conf, sizeof(conf));
		(void)bmi260_reg_read(dev, BMI260_REG_NV_CONF, offset, sizeof(offset));
		(void)bmi260_reg_read(dev, BMI260_REG_ERROR, err, sizeof(err));

		LOG_DBG("ACC_CONF 0x%02x ACC_RANGE 0x%02x GYR_CONF 0x%02x GYR_RANGE 0x%02x",
			conf[0], conf[1], conf[2], conf[3]);
		LOG_DBG("NV_CONF 0x%02x OFFSET %02x %02x %02x %02x %02x %02x %02x", offset[0],
			offset[1], offset[2], offset[3], offset[4], offset[5], offset[6],
			offset[7]);
		LOG_DBG("ERROR 0x%02x STATUS 0x%02x", err[0], err[1]);
	}

	return 0;
}

static DEVICE_API(sensor, bmi260_driver_api) = {
	.sample_fetch = bmi260_sample_fetch,
	.channel_get = bmi260_channel_get,
};

/* Fills in a struct bmi260_config for an instance on an I2C bus. */
#define BMI260_CONFIG_I2C(inst)                                                                    \
	.bus.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_io = &bmi260_bus_io_i2c,

/* Fills in a struct bmi260_config for an instance on a SPI bus. */
#define BMI260_CONFIG_SPI(inst)                                                                    \
	.bus.spi = SPI_DT_SPEC_INST_GET(inst, BMI260_SPI_OPERATION), .bus_io = &bmi260_bus_io_spi,

#define BMI260_CREATE_INST(inst)                                                           \
	static struct bmi260_data bmi260_data_##inst;                                      \
                                                                                           \
	static const struct bmi260_config bmi260_config_##inst = {                         \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (BMI260_CONFIG_SPI(inst)),          \
			    (BMI260_CONFIG_I2C(inst)))                                     \
		.acc_odr_hz = DT_INST_PROP(inst, accel_odr),                               \
		.gyr_odr_hz = DT_INST_PROP(inst, gyro_odr),                                \
		.acc_range_g = DT_INST_PROP(inst, accel_range),                            \
		.gyr_range_dps = DT_INST_PROP(inst, gyro_range),                           \
	};                                                                                 \
                                                                                           \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmi260_init, NULL, &bmi260_data_##inst,          \
				     &bmi260_config_##inst, POST_KERNEL,                   \
				     CONFIG_SENSOR_INIT_PRIORITY, &bmi260_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BMI260_CREATE_INST)
