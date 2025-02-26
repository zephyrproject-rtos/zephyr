/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm45686

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include "icm45686.h"
#include "icm45686_reg.h"
#ifdef CONFIG_I2C
#include "icm45686_i2c.h"
#elif CONFIG_SPI
#include "icm45686_spi.h"
#endif
#include "icm45686_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM45686, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Gyro FS to scaling factor mapping.
 * See datasheet section 3.1 for details
 */
static const uint16_t icm45686_gyro_sensitivity_x10[] = {
	82,    /* BIT_GYRO_UI_FS_4000 */
	164,   /* BIT_GYRO_UI_FS_2000 */
	328,   /* BIT_GYRO_UI_FS_1000 */
	655,   /* BIT_GYRO_UI_FS_500 */
	1310,  /* BIT_GYRO_UI_FS_250 */
	2620,  /* BIT_GYRO_UI_FS_125 */
	5243,  /* BIT_GYRO_UI_FS_62_5 */
	10486, /* BIT_GYRO_UI_FS_31_25 */
	20972  /* BIT_GYRO_UI_FS_15_625 */
};

static int icm45686_set_accel_fs(const struct device *dev, uint16_t fs)
{
	const struct icm45686_config *cfg = dev->config;
	struct icm45686_data *data = dev->data;
	uint8_t temp;

	if ((fs > 32) || (fs < 2)) {
		printk("Unsupported range");
		return -ENOTSUP;
	}

	if (fs > 16) {
		temp = BIT_ACCEL_UI_FS_32;
	} else if (fs > 8) {
		temp = BIT_ACCEL_UI_FS_16;
	} else if (fs > 4) {
		temp = BIT_ACCEL_UI_FS_8;
	} else if (fs > 2) {
		temp = BIT_ACCEL_UI_FS_4;
	} else {
		temp = BIT_ACCEL_UI_FS_2;
	}

	data->accel_sensitivity_shift = MIN_ACCEL_SENS_SHIFT + temp;

	return icm45686_update_register(&cfg->bus, REG_ACCEL_CONFIG0, (uint8_t)MASK_ACCEL_UI_FS_SEL,
					temp);
}

static int icm45686_set_gyro_fs(const struct device *dev, uint16_t fs)
{
	const struct icm45686_config *cfg = dev->config;
	struct icm45686_data *data = dev->data;
	uint8_t temp;

	if ((fs > 4000) || (fs < 16)) {
		LOG_ERR("Unsupported range");
		return -ENOTSUP;
	}

	if (fs > 2000) {
		temp = BIT_GYRO_UI_FS_4000;
	} else if (fs > 1000) {
		temp = BIT_GYRO_UI_FS_2000;
	} else if (fs > 500) {
		temp = BIT_GYRO_UI_FS_1000;
	} else if (fs > 250) {
		temp = BIT_GYRO_UI_FS_500;
	} else if (fs > 125) {
		temp = BIT_GYRO_UI_FS_250;
	} else if (fs > 62) {
		temp = BIT_GYRO_UI_FS_125;
	} else if (fs > 31) {
		temp = BIT_GYRO_UI_FS_62_5;
	} else if (fs > 16) {
		temp = BIT_GYRO_UI_FS_31_25;
	} else {
		temp = BIT_GYRO_UI_FS_16_625;
	}

	data->gyro_sensitivity_x10 = icm45686_gyro_sensitivity_x10[temp];

	return icm45686_update_register(&cfg->bus, REG_GYRO_CONFIG0, (uint8_t)MASK_GYRO_UI_FS_SEL,
					temp);
}

static int icm45686_set_accel_odr(const struct device *dev, uint16_t rate)
{
	const struct icm45686_config *cfg = dev->config;
	uint8_t temp;

	if ((rate > 1600) || (rate < 1)) {
		printk("Unsupported frequency");
		return -ENOTSUP;
	}

	if (rate > 3200) {
		temp = BIT_ACCEL_ODR_1600;
	} else if (rate > 1600) {
		temp = BIT_ACCEL_ODR_1600;
	} else if (rate > 800) {
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

	return icm45686_update_register(&cfg->bus, REG_ACCEL_CONFIG0, (uint8_t)MASK_ACCEL_ODR,
					temp);
}

static int icm45686_set_gyro_odr(const struct device *dev, uint16_t rate)
{
	const struct icm45686_config *cfg = dev->config;
	uint8_t temp;

	if ((rate > 1600) || (rate < 12)) {
		LOG_ERR("Unsupported frequency");
		return -ENOTSUP;
	}

	if (rate > 3200) {
		temp = BIT_GYRO_ODR_6400;
	} else if (rate > 1600) {
		temp = BIT_GYRO_ODR_3200;
	} else if (rate > 800) {
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

	return icm45686_update_register(&cfg->bus, REG_GYRO_CONFIG0, (uint8_t)MASK_GYRO_ODR, temp);
}
static int icm45686_sensor_init(const struct device *dev)
{
	int res;
	uint8_t i = 0;
	uint8_t value;
	const struct icm45686_config *cfg = dev->config;

	/* start up time for register read/write after POR is 1ms and supply ramp time is 3ms */
	k_msleep(200);

	/* clear reset done int flag */
	res = icm45686_read(&cfg->bus, REG_INT1_STATUS0, &value, 1);

	if (res) {
		return res;
	}

	res = icm45686_read(&cfg->bus, REG_WHO_AM_I, &value, 1);

	if (res) {
		printk("failed who am i read");
		return res;
	}

	if (value != WHO_AM_I_ICM45688S) {
		printk("invalid WHO_AM_I value, was %X but expected %X\n", value,
		       WHO_AM_I_ICM45688S);
		return -EINVAL;
	}

	printk("device ID : 0x%X\n", value);

	res = icm45686_single_write(&cfg->bus, REG_MISC2, BIT_SOFT_RST);
	if (res) {
		printk("Failed soft reset");
		return res;
	}

	value = 0;

	while (!(value & REG_INT2_STATUS0_INT2_STATUS_RESET_DONE_MASK)) {
		k_msleep(10);
		res = icm45686_read(&cfg->bus, REG_INT1_STATUS0, &value, 1);
		if (res) {
			printk("Cannot read REG_INT1_STATUS0");
			return res;
		}
		i++;
		if (i > 10) {
			printk("10 times read failed for INT1_STATUS0");
			return -1;
		}
	}

	/* INT pin configuration */
	value = (INT_POLARITY << SHIFT_INT1_POLARITY) | (INT_MODE << SHIFT_INT1_MODE);

	res = icm45686_single_write(&cfg->bus, REG_INT1_CONFIG2, value);

	if (res) {
		printk("Interrupt pin configure is failed\n");
		return res;
	}

	/* disable sensors */
	res = icm45686_single_write(&cfg->bus, REG_PWR_MGMT0, 0);
	if (res) {
		printk("failed to write REG_PWR_MGMT0 , value %X\n", 0);
		return res;
	}

	printk("Init done for icm45688S\n");

	return 0;
}

int icm45686_turn_on_sensor(const struct device *dev)
{
	struct icm45686_data *data = dev->data;
	const struct icm45686_config *cfg = dev->config;
	uint8_t value;
	int res;

	if (data->sensor_started) {
		return -EALREADY;
	}

	res = icm45686_set_accel_fs(dev, data->accel_fs);

	if (res) {
		return res;
	}

	res = icm45686_set_accel_odr(dev, data->accel_hz);

	if (res) {
		return res;
	}

	res = icm45686_set_gyro_fs(dev, data->gyro_fs);

	if (res) {
		return res;
	}

	res = icm45686_set_gyro_odr(dev, data->gyro_hz);

	if (res) {
		return res;
	}

	value = FIELD_PREP(MASK_ACCEL_MODE, BIT_ACCEL_MODE_LNM) |
		FIELD_PREP(MASK_GYRO_MODE, BIT_GYRO_MODE_LNM);

	res = icm45686_update_register(&cfg->bus, REG_PWR_MGMT0,
				       (uint8_t)(MASK_ACCEL_MODE | MASK_GYRO_MODE), value);
	if (res) {
		return res;
	}

	/*
	 * Accelerometer sensor need at least 10ms - 20ms startup time
	 */
	k_msleep(20);

	data->sensor_started = true;

	return 0;
}

static void icm45686_convert_accel(struct sensor_value *val, int16_t raw_val,
				   uint16_t sensitivity_shift)
{
	/* see datasheet section 3.2 for details */
	int64_t conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;

	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

static void icm45686_convert_gyro(struct sensor_value *val, int16_t raw_val,
				  uint16_t sensitivity_x10)
{
	/* see datasheet section 3.1 for details */
	int64_t conv_val = ((int64_t)raw_val * SENSOR_PI * 10) / (sensitivity_x10 * 180LL);

	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

static int icm45686_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	int res = 0;
	const struct icm45686_data *data = dev->data;

	icm45686_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		icm45686_convert_accel(&val[0], data->accel_x, data->accel_sensitivity_shift);
		icm45686_convert_accel(&val[1], data->accel_y, data->accel_sensitivity_shift);
		icm45686_convert_accel(&val[2], data->accel_z, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm45686_convert_accel(val, data->accel_x, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm45686_convert_accel(val, data->accel_y, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm45686_convert_accel(val, data->accel_z, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm45686_convert_gyro(&val[0], data->gyro_x, data->gyro_sensitivity_x10);
		icm45686_convert_gyro(&val[1], data->gyro_y, data->gyro_sensitivity_x10);
		icm45686_convert_gyro(&val[2], data->gyro_z, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm45686_convert_gyro(val, data->gyro_x, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm45686_convert_gyro(val, data->gyro_y, data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm45686_convert_gyro(val, data->gyro_z, data->gyro_sensitivity_x10);
		break;

	case SENSOR_CHAN_DIE_TEMP:
		break;
	default:
		res = -ENOTSUP;
		break;
	}

	icm45686_unlock(dev);

	return res;
}

int icm45686_motion_fetch(const struct device *dev)
{
	return 0;
}

static int icm45686_sample_fetch_accel(const struct device *dev)
{
	const struct icm45686_config *cfg = dev->config;
	struct icm45686_data *data = dev->data;
	uint8_t buffer[ACCEL_DATA_SIZE];

	int res = icm45686_read(&cfg->bus, REG_ACCEL_DATA_X1, buffer, ACCEL_DATA_SIZE);

	if (res) {
		return res;
	}

	data->accel_x = (int16_t)sys_get_le16(&buffer[0]);
	data->accel_y = (int16_t)sys_get_le16(&buffer[2]);
	data->accel_z = (int16_t)sys_get_le16(&buffer[4]);

	return 0;
}

static int icm45686_sample_fetch_gyro(const struct device *dev)
{
	const struct icm45686_config *cfg = dev->config;
	struct icm45686_data *data = dev->data;
	uint8_t buffer[GYRO_DATA_SIZE];

	int res = icm45686_read(&cfg->bus, REG_GYRO_DATA_X1, buffer, GYRO_DATA_SIZE);

	if (res) {
		return res;
	}

	data->gyro_x = (int16_t)sys_get_le16(&buffer[0]);
	data->gyro_y = (int16_t)sys_get_le16(&buffer[2]);
	data->gyro_z = (int16_t)sys_get_le16(&buffer[4]);
	return 0;
}

static int icm45686_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	uint8_t status;
	const struct icm45686_config *cfg = dev->config;

	icm45686_lock(dev);

	int res = icm45686_read(&cfg->bus, REG_INT1_STATUS0, &status, 1);

	if (res) {
		goto cleanup;
	}

	if (!FIELD_GET(BIT_INT_STATUS_DATA_DRDY, status)) {
		res = -EBUSY;
		goto cleanup;
	}

	switch (chan) {
	case SENSOR_CHAN_ALL:
		res |= icm45686_sample_fetch_accel(dev);
		res |= icm45686_sample_fetch_gyro(dev);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		res = icm45686_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
		res = icm45686_sample_fetch_gyro(dev);
		break;

	case SENSOR_CHAN_DIE_TEMP:
		break;
	default:
		res = -ENOTSUP;
		break;
	}

cleanup:
	icm45686_unlock(dev);
	return res;
}

static int icm45686_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	int res = 0;
	struct icm45686_data *data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	icm45686_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			res = icm45686_set_accel_odr(dev, data->accel_hz);

			if (res) {
				printk("Incorrect sampling value");
			} else {
				data->accel_hz = val->val1;
			}
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			res = icm45686_set_accel_fs(dev, data->accel_fs);

			if (res) {
				printk("Incorrect fullscale value");
			} else {
				data->accel_fs = val->val1;
			}
		} else {
			printk("Unsupported attribute");
			res = -ENOTSUP;
		}
		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			res = icm45686_set_gyro_odr(dev, data->gyro_hz);

			if (res) {
				LOG_ERR("Incorrect sampling value");
			} else {
				data->gyro_hz = val->val1;
			}
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			res = icm45686_set_gyro_fs(dev, data->gyro_fs);

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
		printk("Unsupported channel");
		res = -EINVAL;
		break;
	}

	icm45686_unlock(dev);

	return res;
}

static int icm45686_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct icm45686_data *data = dev->data;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	icm45686_lock(dev);

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
			printk("Unsupported attribute");
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
		printk("Unsupported channel");
		res = -EINVAL;
		break;
	}

	icm45686_unlock(dev);

	return res;
}

static int icm45686_init(const struct device *dev)
{
	struct icm45686_data *data = dev->data;
	const struct icm45686_config *cfg = dev->config;

#ifdef CONFIG_I2C
	if (!i2c_is_ready_dt(&cfg->bus)) {
		printk("i2c bus is not ready");
#elif CONFIG_SPI
	if (!spi_is_ready_dt(&cfg->bus)) {
		printk("SPI bus is not ready");
#else
#error "Only support I2C/SPI"
#endif
		return -ENODEV;
	}

	data->accel_x = 0;
	data->accel_y = 0;
	data->accel_z = 0;
	data->temp = 0;
	data->sensor_started = false;

	if (icm45686_sensor_init(dev)) {
		printk("could not initialize sensor");
		return -EIO;
	}

#ifdef CONFIG_ICM45686_TRIGGER
	if (icm45686_trigger_init(dev)) {
		printk("Failed to initialize interrupts.");
		return -EIO;
	}
#endif

#ifdef CONFIG_ICM45686_TRIGGER
	if (icm45686_trigger_enable_interrupt(dev)) {
		printk("Failed to enable interrupts");
		return -EIO;
	}
#endif
	return 0;
}

#ifndef CONFIG_ICM45686_TRIGGER

void icm45686_lock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

void icm45686_unlock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

#endif

static const struct sensor_driver_api icm45686_driver_api = {
#ifdef CONFIG_ICM45686_TRIGGER
	.trigger_set = icm45686_trigger_set,
#endif
	.sample_fetch = icm45686_sample_fetch,
	.channel_get = icm45686_channel_get,
	.attr_set = icm45686_attr_set,
	.attr_get = icm45686_attr_get,
};

#define ICM45686_SPI_CFG                                                                           \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define ICM45686_CONFIG_SPI(inst)                                                                  \
	{                                                                                          \
		.bus = SPI_DT_SPEC_INST_GET(inst, ICM45686_SPI_CFG, 0U),                           \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
	}

#define ICM45686_CONFIG_I2C(inst)                                                                  \
	{                                                                                          \
		.bus = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
	}

#define ICM45686_INIT(inst)                                                                        \
	static struct icm45686_data icm45686_driver_##inst = {                                     \
		.accel_hz = DT_INST_PROP(inst, accel_hz),                                          \
		.accel_fs = DT_INST_PROP(inst, accel_fs),                                          \
		.gyro_hz = DT_INST_PROP(inst, gyro_hz),                                            \
		.gyro_fs = DT_INST_PROP(inst, gyro_fs),                                            \
	};                                                                                         \
                                                                                                   \
	static const struct icm45686_config icm45686_cfg_##inst = COND_CODE_1(DT_INST_ON_BUS(inst, spi),						   \
				(ICM45686_CONFIG_SPI(inst)),					   \
				(ICM45686_CONFIG_I2C(inst)));             \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm45686_init, NULL, &icm45686_driver_##inst,           \
				     &icm45686_cfg_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &icm45686_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ICM45686_INIT)
