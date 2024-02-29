/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm42370

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include "icm42370.h"
#include "icm42370_reg.h"
#ifdef CONFIG_I2C
	#include "icm42370_i2c.h"
#elif CONFIG_SPI
	#include "icm42370_spi.h"
#endif
#include "icm42370_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM42370, CONFIG_SENSOR_LOG_LEVEL);

static int icm42370_set_accel_fs(const struct device *dev, uint16_t fs)
{
	const struct icm42370_config *cfg = dev->config;
	struct icm42370_data *data = dev->data;
	uint8_t temp;

	if ((fs > 16) || (fs < 2)) {
		printk("Unsupported range");
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

	return icm42370_update_register(&cfg->bus, REG_ACCEL_CONFIG0,
					    (uint8_t)MASK_ACCEL_UI_FS_SEL, temp);
}

static int icm42370_set_accel_odr(const struct device *dev, uint16_t rate)
{
	const struct icm42370_config *cfg = dev->config;
	uint8_t temp;

	if ((rate > 1600) || (rate < 1)) {
		printk("Unsupported frequency");
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

	return icm42370_update_register(&cfg->bus, REG_ACCEL_CONFIG0, (uint8_t)MASK_ACCEL_ODR,
					    temp);
}

static int icm42370_enable_mclk(const struct device *dev)
{
	const struct icm42370_config *cfg = dev->config;

	/* switch on MCLK by setting the IDLE bit */
	int res = icm42370_single_write(&cfg->bus, REG_PWR_MGMT0, BIT_IDLE);

	if (res) {
		return res;
	}

	/* wait for the MCLK to stabilize by polling MCLK_RDY register */
	for (int i = 0; i < MCLK_POLL_ATTEMPTS; i++) {
		uint8_t value = 0;

		k_usleep(MCLK_POLL_INTERVAL_US);
		res = icm42370_read(&cfg->bus, REG_MCLK_RDY, &value, 1);

		if (res) {
			return res;
		}

		if (FIELD_GET(BIT_MCLK_RDY, value)) {
			return 0;
		}
	}

	return -EIO;
}

static int icm42370_sensor_init(const struct device *dev)
{
	int res;
	uint8_t value;
	const struct icm42370_config *cfg = dev->config;

	/* start up time for register read/write after POR is 1ms and supply ramp time is 3ms */
	k_msleep(3);

	/* perform a soft reset to ensure a clean slate, reset bit will auto-clear */
	res = icm42370_single_write(&cfg->bus, REG_SIGNAL_PATH_RESET, BIT_SOFT_RESET);

	if (res) {
		printk("write REG_SIGNAL_PATH_RESET failed");
		return res;
	}

	/* wait for soft reset to take effect */
	k_msleep(SOFT_RESET_TIME_MS);

#ifdef CONFIG_SPI
	/* force SPI-4w hardware configuration (so that next read is correct) */
	res = icm42370_single_write(&cfg->bus, REG_DEVICE_CONFIG, BIT_SPI_AP_4WIRE);

	if (res) {
		return res;
	}

	k_msleep(SOFT_RESET_TIME_MS);
#endif

	/* always use internal RC oscillator */
	res = icm42370_single_write(&cfg->bus, REG_INTF_CONFIG1,
					(uint8_t)FIELD_PREP(MASK_CLKSEL, BIT_CLKSEL_INT_RC));

	if (res) {
		return res;
	}

	/* clear reset done int flag */
	res = icm42370_read(&cfg->bus, REG_INT_STATUS, &value, 1);

	if (res) {
		return res;
	}

	if (FIELD_GET(BIT_STATUS_RESET_DONE_INT, value) != 1) {
		return -EINVAL;
	}

	/* enable the master clock to ensure proper operation */
	res = icm42370_enable_mclk(dev);

	if (res) {
		printk("enable mclk failed\n");
		return res;
	}

	res = icm42370_read(&cfg->bus, REG_WHO_AM_I, &value, 1);

	if (res) {
		printk("failed who am i read");
		return res;
	}

	if (value != WHO_AM_I_ICM42370) {
		printk("invalid WHO_AM_I value, was %X but expected %X\n", value,
		       WHO_AM_I_ICM42370);
		return -EINVAL;
	}

	printk("device id: 0x%02X", value);

	return 0;
}

int icm42370_turn_on_sensor(const struct device *dev)
{
	struct icm42370_data *data = dev->data;
	const struct icm42370_config *cfg = dev->config;
	uint8_t value;
	int res;

	if (data->sensor_started) {
		return -EALREADY;
	}

	value = FIELD_PREP(MASK_ACCEL_MODE, BIT_ACCEL_MODE_LNM);

	res = icm42370_update_register(&cfg->bus, REG_PWR_MGMT0,
					   (uint8_t)(MASK_ACCEL_MODE), value);
	if (res) {
		return res;
	}

	res = icm42370_set_accel_fs(dev, data->accel_fs);

	if (res) {
		return res;
	}

	res = icm42370_set_accel_odr(dev, data->accel_hz);

	if (res) {
		return res;
	}

	/*
	 * Accelerometer sensor need at least 10ms - 20ms startup time
	 */
	k_msleep(20);

#ifdef CONFIG_ICM42370_TRIGGER
	data->motion_en = true;
	printk("Motion enabled!!!");
	if (data->motion_en) {
		if (data->accel_hz > 50)
			value = (13) /  (data->accel_hz / 50);
		else
			value = 13;

		res = icm42370_write_mreg(&cfg->bus, REG_ACCEL_WOM_X_THR, value);
		if (res) {
			return res;
		}

		res = icm42370_write_mreg(&cfg->bus, REG_ACCEL_WOM_Y_THR, value);
		if (res) {
			return res;
		}

		res = icm42370_write_mreg(&cfg->bus, REG_ACCEL_WOM_Z_THR, value);
		if (res) {
			return res;
		}

		res = icm42370_single_write(&cfg->bus, REG_WOM_CONFIG,
					BIT_WOM_INT_MODE_AND | BIT_WOM_MODE_PREV | BIT_WOM_EN_ON);
		if (res) {
			return res;
		}

		/* Enable WOM interrupt for all axis */
		res = icm42370_single_write(&cfg->bus, REG_INT_SOURCE1, BIT_INT_WOM_XYZ_INT1_EN);
		if (res) {
			return res;
		}

	}

#endif
	data->sensor_started = true;

	return 0;
}

static void icm42370_convert_accel(struct sensor_value *val, int16_t raw_val,
				   uint16_t sensitivity_shift)
{
	/* see datasheet section 3.2 for details */
	int64_t conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;

	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

static inline void icm42370_convert_temp(struct sensor_value *val, int16_t raw_val)
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

static int icm42370_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	int res = 0;
	const struct icm42370_data *data = dev->data;

	icm42370_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		icm42370_convert_accel(&val[0], data->accel_x, data->accel_sensitivity_shift);
		icm42370_convert_accel(&val[1], data->accel_y, data->accel_sensitivity_shift);
		icm42370_convert_accel(&val[2], data->accel_z, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm42370_convert_accel(val, data->accel_x, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm42370_convert_accel(val, data->accel_y, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm42370_convert_accel(val, data->accel_z, data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm42370_convert_temp(val, data->temp);
		break;
	default:
		res = -ENOTSUP;
		break;
	}

	icm42370_unlock(dev);

	return res;
}

int icm42370_motion_fetch(const struct device *dev)
{
	int res = 0;
	uint8_t status2 = 0;
	struct icm42370_data *data = dev->data;
	const struct icm42370_config *cfg = dev->config;

	if (data->motion_en) {
		res = icm42370_read(&cfg->bus, REG_INT_STATUS2, &status2, 1);
		if (res) {
			return -1;
		}

		if (status2 != 0)
			printk("Got WOM from INT_STATUS2 %X\n", status2);
	}

	return 0;
}

static int icm42370_sample_fetch_accel(const struct device *dev)
{
	const struct icm42370_config *cfg = dev->config;
	struct icm42370_data *data = dev->data;
	uint8_t buffer[ACCEL_DATA_SIZE];

	int res = icm42370_read(&cfg->bus, REG_ACCEL_DATA_X1, buffer, ACCEL_DATA_SIZE);

	if (res) {
		return res;
	}

	data->accel_x = (int16_t)sys_get_be16(&buffer[0]);
	data->accel_y = (int16_t)sys_get_be16(&buffer[2]);
	data->accel_z = (int16_t)sys_get_be16(&buffer[4]);
	printk("Accel %d %d %d\n", data->accel_x, data->accel_y, data->accel_z);

	return 0;
}

static int icm42370_sample_fetch_temp(const struct device *dev)
{
	const struct icm42370_config *cfg = dev->config;
	struct icm42370_data *data = dev->data;
	uint8_t buffer[TEMP_DATA_SIZE];

	int res = icm42370_read(&cfg->bus, REG_TEMP_DATA1, buffer, TEMP_DATA_SIZE);

	if (res) {
		return res;
	}

	data->temp = (int16_t)sys_get_be16(&buffer[0]);

	return 0;
}

static int icm42370_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	uint8_t status;
	const struct icm42370_config *cfg = dev->config;

	icm42370_lock(dev);

	int res = icm42370_read(&cfg->bus, REG_INT_STATUS_DRDY, &status, 1);

	if (res) {
		goto cleanup;
	}

	if (!FIELD_GET(BIT_INT_STATUS_DATA_DRDY, status)) {
		res = -EBUSY;
		goto cleanup;
	}

	switch (chan) {
	case SENSOR_CHAN_ALL:
		res |= icm42370_sample_fetch_accel(dev);
		res |= icm42370_sample_fetch_temp(dev);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		res = icm42370_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		res = icm42370_sample_fetch_temp(dev);
		break;
	default:
		res = -ENOTSUP;
		break;
	}

cleanup:
	icm42370_unlock(dev);
	return res;
}

static int icm42370_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	int res = 0;
	struct icm42370_data *data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	icm42370_lock(dev);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			res = icm42370_set_accel_odr(dev, data->accel_hz);

			if (res) {
				printk("Incorrect sampling value");
			} else {
				data->accel_hz = val->val1;
			}
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			res = icm42370_set_accel_fs(dev, data->accel_fs);

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
	default:
		printk("Unsupported channel");
		res = -EINVAL;
		break;
	}

	icm42370_unlock(dev);

	return res;
}

static int icm42370_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct icm42370_data *data = dev->data;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	icm42370_lock(dev);

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
	default:
		printk("Unsupported channel");
		res = -EINVAL;
		break;
	}

	icm42370_unlock(dev);

	return res;
}

static int icm42370_init(const struct device *dev)
{
	struct icm42370_data *data = dev->data;
	const struct icm42370_config *cfg = dev->config;

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

	if (icm42370_sensor_init(dev)) {
		printk("could not initialize sensor");
		return -EIO;
	}

#ifdef CONFIG_ICM42370_TRIGGER
	if (icm42370_trigger_init(dev)) {
		printk("Failed to initialize interrupts.");
		return -EIO;
	}
#endif

#ifdef CONFIG_ICM42370_TRIGGER
	if (icm42370_trigger_enable_interrupt(dev)) {
		printk("Failed to enable interrupts");
		return -EIO;
	}
#endif

	return 0;
}

#ifndef CONFIG_ICM42370_TRIGGER

void icm42370_lock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

void icm42370_unlock(const struct device *dev)
{
	ARG_UNUSED(dev);
}

#endif

static const struct sensor_driver_api icm42370_driver_api = {
#ifdef CONFIG_ICM42370_TRIGGER
	.trigger_set = icm42370_trigger_set,
#endif
	.sample_fetch = icm42370_sample_fetch,
	.channel_get = icm42370_channel_get,
	.attr_set = icm42370_attr_set,
	.attr_get = icm42370_attr_get,
};

#define ICM42370_SPI_CFG                                                                           \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define ICM42370_CONFIG_SPI(inst)						\
	{									\
		.bus = SPI_DT_SPEC_INST_GET(inst, ICM42370_SPI_CFG, 0U),	\
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),	\
	}


#define ICM42370_CONFIG_I2C(inst)						\
	{									\
		.bus = I2C_DT_SPEC_INST_GET(inst),				\
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),	\
	}



#define ICM42370_INIT(inst)                                                                        \
	static struct icm42370_data icm42370_driver_##inst = {                                     \
		.accel_hz = DT_INST_PROP(inst, accel_hz),                                          \
		.accel_fs = DT_INST_PROP(inst, accel_fs),                                          \
	};                                                                                         \
												   \
	static const struct icm42370_config icm42370_cfg_##inst =				   \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),						   \
				(ICM42370_CONFIG_SPI(inst)),					   \
				(ICM42370_CONFIG_I2C(inst)));					   \
												   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm42370_init, NULL, &icm42370_driver_##inst,           \
			      &icm42370_cfg_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,      \
			      &icm42370_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ICM42370_INIT)
