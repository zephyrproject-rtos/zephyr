/*
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm42670

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include "icm42670.h"
#include "icm42670_reg.h"
#include "icm42670_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42670, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Gyro FS to scaling factor mapping.
 * See datasheet section 3.1 for details
 */
static const uint16_t icm42670_gyro_sensitivity_x10[] = {
	164, /* BIT_GYRO_UI_FS_2000 */
	328, /* BIT_GYRO_UI_FS_1000 */
	655, /* BIT_GYRO_UI_FS_500 */
	1310, /* BIT_GYRO_UI_FS_250 */
};

static int icm42670_set_accel_fs(const struct device *dev, uint16_t fs)
{
	const struct icm42670_config *cfg = dev->config;
	struct icm42670_data *data = dev->data;
	uint8_t temp;

	if ((fs > 16) || (fs < 2)) {
		LOG_ERR("Unsupported range");
		return -ENOTSUP;
	}

	if (fs > 8) {
		temp = BIT_ACCEL_UI_FS_16;
	} else if (fs > 4) {
		temp = BIT_ACCEL_UI_FS_8;
	} else if (fs > 2) {
		temp = BIT_ACCEL_UI_FS_4;
	} else {
		temp = BIT_ACCEL_UI_FS_2;
	}

	data->accel_sensitivity_shift = MIN_ACCEL_SENS_SHIFT + temp;

	return cfg->bus_io->update(&cfg->bus, REG_ACCEL_CONFIG0,
					    (uint8_t)MASK_ACCEL_UI_FS_SEL, temp);
}

static int icm42670_set_gyro_fs(const struct device *dev, uint16_t fs)
{
	const struct icm42670_config *cfg = dev->config;
	struct icm42670_data *data = dev->data;
	uint8_t temp;

	if ((fs > 2000) || (fs < 250)) {
		LOG_ERR("Unsupported range");
		return -ENOTSUP;
	}

	if (fs > 1000) {
		temp = BIT_GYRO_UI_FS_2000;
	} else if (fs > 500) {
		temp = BIT_GYRO_UI_FS_1000;
	} else if (fs > 250) {
		temp = BIT_GYRO_UI_FS_500;
	} else {
		temp = BIT_GYRO_UI_FS_250;
	}

	data->gyro_sensitivity_x10 = icm42670_gyro_sensitivity_x10[temp];

	return cfg->bus_io->update(&cfg->bus, REG_GYRO_CONFIG0,
					    (uint8_t)MASK_GYRO_UI_FS_SEL, temp);
}

static int icm42670_set_accel_odr(const struct device *dev, uint16_t rate)
{
	const struct icm42670_config *cfg = dev->config;
	uint8_t temp;

	if ((rate > 1600) || (rate < 1)) {
		LOG_ERR("Unsupported frequency");
		return -ENOTSUP;
	}

	if (rate > 800) {
		temp = BIT_ACCEL_ODR_1600;
	} else if (rate > 400) {
		temp = BIT_ACCEL_ODR_800;
	} else if (rate > 200) {
		temp = BIT_ACCEL_ODR_400;
	} else if (rate > 100) {
		temp = BIT_ACCEL_ODR_200;
	} else if (rate > 50) {
		temp = BIT_ACCEL_ODR_100;
	} else if (rate > 25) {
		temp = BIT_ACCEL_ODR_50;
	} else if (rate > 12) {
		temp = BIT_ACCEL_ODR_25;
	} else if (rate > 6) {
		temp = BIT_ACCEL_ODR_12;
	} else if (rate > 3) {
		temp = BIT_ACCEL_ODR_6;
	} else if (rate > 1) {
		temp = BIT_ACCEL_ODR_3;
	} else {
		temp = BIT_ACCEL_ODR_1;
	}

	return cfg->bus_io->update(&cfg->bus, REG_ACCEL_CONFIG0, (uint8_t)MASK_ACCEL_ODR,
					    temp);
}

static int icm42670_set_gyro_odr(const struct device *dev, uint16_t rate)
{
	const struct icm42670_config *cfg = dev->config;
	uint8_t temp;

	if ((rate > 1600) || (rate < 12)) {
		LOG_ERR("Unsupported frequency");
		return -ENOTSUP;
	}

	if (rate > 800) {
		temp = BIT_GYRO_ODR_1600;
	} else if (rate > 400) {
		temp = BIT_GYRO_ODR_800;
	} else if (rate > 200) {
		temp = BIT_GYRO_ODR_400;
	} else if (rate > 100) {
		temp = BIT_GYRO_ODR_200;
	} else if (rate > 50) {
		temp = BIT_GYRO_ODR_100;
	} else if (rate > 25) {
		temp = BIT_GYRO_ODR_50;
	} else if (rate > 12) {
		temp = BIT_GYRO_ODR_25;
	} else {
		temp = BIT_GYRO_ODR_12;
	}

	return cfg->bus_io->update(&cfg->bus, REG_GYRO_CONFIG0, (uint8_t)MASK_GYRO_ODR,
					    temp);
}

static int icm42670_enable_mclk(const struct device *dev)
{
	const struct icm42670_config *cfg = dev->config;

	/* switch on MCLK by setting the IDLE bit */
	int res = cfg->bus_io->write(&cfg->bus, REG_PWR_MGMT0, BIT_IDLE);

	if (res) {
		return res;
	}

	/* wait for the MCLK to stabilize by polling MCLK_RDY register */
	for (int i = 0; i < MCLK_POLL_ATTEMPTS; i++) {
		uint8_t value = 0;

		k_usleep(MCLK_POLL_INTERVAL_US);
		res = cfg->bus_io->read(&cfg->bus, REG_MCLK_RDY, &value, 1);

		if (res) {
			return res;
		}

		if (FIELD_GET(BIT_MCLK_RDY, value)) {
			return 0;
		}
	}

	return -EIO;
}

static int icm42670_sensor_init(const struct device *dev)
{
	int res;
	uint8_t value;
	const struct icm42670_config *cfg = dev->config;

	/* start up time for register read/write after POR is 1ms and supply ramp time is 3ms */
	k_msleep(3);

	/* perform a soft reset to ensure a clean slate, reset bit will auto-clear */
	res = cfg->bus_io->write(&cfg->bus, REG_SIGNAL_PATH_RESET, BIT_SOFT_RESET);

	if (res) {
		LOG_ERR("write REG_SIGNAL_PATH_RESET failed");
		return res;
	}

	/* wait for soft reset to take effect */
	k_msleep(SOFT_RESET_TIME_MS);

	/* force SPI-4w hardware configuration (so that next read is correct) */
	res = cfg->bus_io->write(&cfg->bus, REG_DEVICE_CONFIG, BIT_SPI_AP_4WIRE);

	if (res) {
		return res;
	}

	/* always use internal RC oscillator */
	res = cfg->bus_io->write(&cfg->bus, REG_INTF_CONFIG1,
					(uint8_t)FIELD_PREP(MASK_CLKSEL, BIT_CLKSEL_INT_RC));

	if (res) {
		return res;
	}

	/* clear reset done int flag */
	res = cfg->bus_io->read(&cfg->bus, REG_INT_STATUS, &value, 1);

	if (res) {
		return res;
	}

	if (FIELD_GET(BIT_STATUS_RESET_DONE_INT, value) != 1) {
		LOG_ERR("unexpected RESET_DONE_INT value, %i", value);
		return -EINVAL;
	}

	/* enable the master clock to ensure proper operation */
	res = icm42670_enable_mclk(dev);

	if (res) {
		return res;
	}

	res = cfg->bus_io->read(&cfg->bus, REG_WHO_AM_I, &value, 1);

	if (res) {
		return res;
	}

	if (value != WHO_AM_I_ICM42670) {
		LOG_ERR("invalid WHO_AM_I value, was %i but expected %i", value, WHO_AM_I_ICM42670);
		return -EINVAL;
	}

	LOG_DBG("device id: 0x%02X", value);

	return 0;
}

static int icm42670_turn_on_sensor(const struct device *dev)
{
	struct icm42670_data *data = dev->data;
	const struct icm42670_config *cfg = dev->config;
	uint8_t value;
	int res;

	value = FIELD_PREP(MASK_ACCEL_MODE, BIT_ACCEL_MODE_LNM) |
		FIELD_PREP(MASK_GYRO_MODE, BIT_GYRO_MODE_LNM);

	res = cfg->bus_io->update(&cfg->bus, REG_PWR_MGMT0,
					   (uint8_t)(MASK_ACCEL_MODE | MASK_GYRO_MODE), value);

	if (res) {
		return res;
	}

	res = icm42670_set_accel_fs(dev, data->accel_fs);

	if (res) {
		return res;
	}

	res = icm42670_set_accel_odr(dev, data->accel_hz);

	if (res) {
		return res;
	}

	res = icm42670_set_gyro_fs(dev, data->gyro_fs);

	if (res) {
		return res;
	}

	res = icm42670_set_gyro_odr(dev, data->gyro_hz);

	if (res) {
		return res;
	}

	/*
	 * Accelerometer sensor need at least 10ms startup time
	 * Gyroscope sensor need at least 30ms startup time
	 */
	k_msleep(100);

	return 0;
}

static void icm42670_convert_accel(struct sensor_value *val, int16_t raw_val,
				   uint16_t sensitivity_shift)
{
	/* see datasheet section 3.2 for details */
	int64_t conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;

	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

static void icm42670_convert_gyro(struct sensor_value *val, int16_t raw_val,
				  uint16_t sensitivity_x10)
{
	/* see datasheet section 3.1 for details */
	int64_t conv_val = ((int64_t)raw_val * SENSOR_PI * 10) / (sensitivity_x10 * 180LL);

	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

static inline void icm42670_convert_temp(struct sensor_value *val, int16_t raw_val)
{
	/* see datasheet section 15.9 for details */
	val->val1 = (((int64_t)raw_val * 100) / 12800) + 25;
	val->val2 = ((((int64_t)raw_val * 100) % 12800) * 1000000) / 12800;

	if (val->val2 < 0) {
		val->val1--;
		val->val2 += 1000000;
	} else if (val->val2 >= 1000000) {
		val->val1++;
		val->val2 -= 1000000;
	}
}

static int icm42670_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	int res = 0;
	const struct icm42670_data *data = dev->data;

	icm42670_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		icm42670_convert_accel(&val[0], data->accel_x, data->accel_sensitivity_shift);
		icm42670_convert_accel(&val[1], data->accel_y, data->accel_sensitivity_shift);
		icm42670_convert_accel(&val[2], data->accel_z, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm42670_convert_accel(val, data->accel_x, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm42670_convert_accel(val, data->accel_y, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm42670_convert_accel(val, data->accel_z, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm42670_convert_gyro(&val[0], data->gyro_x, data->gyro_sensitivity_x10);
		icm42670_convert_gyro(&val[1], data->gyro_y, data->gyro_sensitivity_x10);
		icm42670_convert_gyro(&val[2], data->gyro_z, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm42670_convert_gyro(val, data->gyro_x, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm42670_convert_gyro(val, data->gyro_y, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm42670_convert_gyro(val, data->gyro_z, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm42670_convert_temp(val, data->temp);
		break;
	default:
		res = -ENOTSUP;
		break;
	}

	icm42670_unlock(dev);

	return res;
}

static int icm42670_sample_fetch_accel(const struct device *dev)
{
	const struct icm42670_config *cfg = dev->config;
	struct icm42670_data *data = dev->data;
	uint8_t buffer[ACCEL_DATA_SIZE];

	int res = cfg->bus_io->read(&cfg->bus, REG_ACCEL_DATA_X1, buffer, ACCEL_DATA_SIZE);

	if (res) {
		return res;
	}

	data->accel_x = (int16_t)sys_get_be16(&buffer[0]);
	data->accel_y = (int16_t)sys_get_be16(&buffer[2]);
	data->accel_z = (int16_t)sys_get_be16(&buffer[4]);

	return 0;
}

static int icm42670_sample_fetch_gyro(const struct device *dev)
{
	const struct icm42670_config *cfg = dev->config;
	struct icm42670_data *data = dev->data;
	uint8_t buffer[GYRO_DATA_SIZE];

	int res = cfg->bus_io->read(&cfg->bus, REG_GYRO_DATA_X1, buffer, GYRO_DATA_SIZE);

	if (res) {
		return res;
	}

	data->gyro_x = (int16_t)sys_get_be16(&buffer[0]);
	data->gyro_y = (int16_t)sys_get_be16(&buffer[2]);
	data->gyro_z = (int16_t)sys_get_be16(&buffer[4]);

	return 0;
}

static int icm42670_sample_fetch_temp(const struct device *dev)
{
	const struct icm42670_config *cfg = dev->config;
	struct icm42670_data *data = dev->data;
	uint8_t buffer[TEMP_DATA_SIZE];

	int res = cfg->bus_io->read(&cfg->bus, REG_TEMP_DATA1, buffer, TEMP_DATA_SIZE);

	if (res) {
		return res;
	}

	data->temp = (int16_t)sys_get_be16(&buffer[0]);

	return 0;
}

static int icm42670_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	uint8_t status;
	const struct icm42670_config *cfg = dev->config;

	icm42670_lock(dev);

	int res = cfg->bus_io->read(&cfg->bus, REG_INT_STATUS_DRDY, &status, 1);

	if (res) {
		goto cleanup;
	}

	if (!FIELD_GET(BIT_INT_STATUS_DATA_DRDY, status)) {
		res = -EBUSY;
		goto cleanup;
	}

	switch (chan) {
	case SENSOR_CHAN_ALL:
		res |= icm42670_sample_fetch_accel(dev);
		res |= icm42670_sample_fetch_gyro(dev);
		res |= icm42670_sample_fetch_temp(dev);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		res = icm42670_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		res = icm42670_sample_fetch_gyro(dev);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		res = icm42670_sample_fetch_temp(dev);
		break;
	default:
		res = -ENOTSUP;
		break;
	}

cleanup:
	icm42670_unlock(dev);
	return res;
}

static int icm42670_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	int res = 0;
	struct icm42670_data *data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	icm42670_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			res = icm42670_set_accel_odr(dev, data->accel_hz);

			if (res) {
				LOG_ERR("Incorrect sampling value");
			} else {
				data->accel_hz = val->val1;
			}
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			res = icm42670_set_accel_fs(dev, data->accel_fs);

			if (res) {
				LOG_ERR("Incorrect fullscale value");
			} else {
				data->accel_fs = val->val1;
			}
		} else {
			LOG_ERR("Unsupported attribute");
			res = -ENOTSUP;
		}
		break;

	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			res = icm42670_set_gyro_odr(dev, data->gyro_hz);

			if (res) {
				LOG_ERR("Incorrect sampling value");
			} else {
				data->gyro_hz = val->val1;
			}
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			res = icm42670_set_gyro_fs(dev, data->gyro_fs);

			if (res) {
				LOG_ERR("Incorrect fullscale value");
			} else {
				data->gyro_fs = val->val1;
			}
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;

	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	icm42670_unlock(dev);

	return res;
}

static int icm42670_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct icm42670_data *data = dev->data;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	icm42670_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			val->val1 = data->accel_hz;
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			val->val1 = data->accel_fs;
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;

	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			val->val1 = data->gyro_hz;
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			val->val1 = data->gyro_fs;
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;

	default:
		LOG_ERR("Unsupported channel");
		res = -EINVAL;
		break;
	}

	icm42670_unlock(dev);

	return res;
}

static inline int icm42670_bus_check(const struct device *dev)
{
	const struct icm42670_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static int icm42670_init(const struct device *dev)
{
	struct icm42670_data *data = dev->data;

	if (icm42670_bus_check(dev) < 0) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	data->accel_x = 0;
	data->accel_y = 0;
	data->accel_z = 0;
	data->gyro_x = 0;
	data->gyro_y = 0;
	data->gyro_z = 0;
	data->temp = 0;

	if (icm42670_sensor_init(dev)) {
		LOG_ERR("could not initialize sensor");
		return -EIO;
	}

#ifdef CONFIG_ICM42670_TRIGGER
	if (icm42670_trigger_init(dev)) {
		LOG_ERR("Failed to initialize interrupts.");
		return -EIO;
	}
#endif

	int res = icm42670_turn_on_sensor(dev);

#ifdef CONFIG_ICM42670_TRIGGER
	if (icm42670_trigger_enable_interrupt(dev)) {
		LOG_ERR("Failed to enable interrupts");
		return -EIO;
	}
#endif

	return res;
}

#ifndef CONFIG_ICM42670_TRIGGER

void icm42670_lock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

void icm42670_unlock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

#endif

static const struct sensor_driver_api icm42670_driver_api = {
#ifdef CONFIG_ICM42670_TRIGGER
	.trigger_set = icm42670_trigger_set,
#endif
	.sample_fetch = icm42670_sample_fetch,
	.channel_get = icm42670_channel_get,
	.attr_set = icm42670_attr_set,
	.attr_get = icm42670_attr_get,
};

/* device defaults to spi mode 0/3 support */
#define ICM42670_SPI_CFG                                                                           \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_TRANSFER_MSB

/* Initializes the bus members for an instance on a SPI bus. */
#define ICM42670_CONFIG_SPI(inst)                                                                  \
	.bus.spi = SPI_DT_SPEC_INST_GET(inst, ICM42670_SPI_CFG, 0),                                \
	.bus_io = &icm42670_bus_io_spi,

/* Initializes the bus members for an instance on an I2C bus. */
#define ICM42670_CONFIG_I2C(inst)                                                                  \
	.bus.i2c = I2C_DT_SPEC_INST_GET(inst),                                                     \
	.bus_io = &icm42670_bus_io_i2c,

#define ICM42670_INIT(inst)                                                                        \
	static struct icm42670_data icm42670_driver_##inst = {                                     \
		.accel_hz = DT_INST_PROP(inst, accel_hz),                                          \
		.accel_fs = DT_INST_PROP(inst, accel_fs),                                          \
		.gyro_hz = DT_INST_PROP(inst, gyro_hz),                                            \
		.gyro_fs = DT_INST_PROP(inst, gyro_fs),                                            \
	};                                                                                         \
                                                                                                   \
	static const struct icm42670_config icm42670_cfg_##inst = {                                \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                             \
			(ICM42670_CONFIG_SPI(inst)),                                               \
			(ICM42670_CONFIG_I2C(inst)))                                               \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm42670_init, NULL, &icm42670_driver_##inst,           \
			      &icm42670_cfg_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,      \
			      &icm42670_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ICM42670_INIT)
