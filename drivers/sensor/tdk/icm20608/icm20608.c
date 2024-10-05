/*
 * Copyright (c) 2024, Han Wu
 * email: wuhanstudios@gmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm20608

#include <stdint.h>
#include <math.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "icm20608.h"

LOG_MODULE_REGISTER(ICM20608, CONFIG_SENSOR_LOG_LEVEL);

static void icm20608_convert_temp(struct sensor_value *val, int16_t raw_val)
{

	/* Offset by 21 degrees Celsius */
	int64_t in100 = ((raw_val * 100) +
			 (ICM20608_ROOM_TEMP_OFFSET_DEG * ICM20608_TEMP_SENSITIVITY_X100)) *
			1000000LL;

	/* Whole celsius */
	val->val1 = in100 / (ICM20608_TEMP_SENSITIVITY_X100 * 1000000LL);

	/* Micro celsius */
	val->val2 = (in100 - (val->val1 * ICM20608_TEMP_SENSITIVITY_X100 * 1000000LL)) /
		    ICM20608_TEMP_SENSITIVITY_X100;
}

static int icm20608_convert_accel(const struct icm20608_config *cfg, int32_t raw_accel_value,
				  struct sensor_value *output_value)
{
	int64_t sensitivity = 0; /* value equivalent for 1g */

	switch (cfg->accel_fs) {
	case ICM20608_ACCEL_FS_SEL_2G:
		sensitivity = 16384;
		break;
	case ICM20608_ACCEL_FS_SEL_4G:
		sensitivity = 8192;
		break;
	case ICM20608_ACCEL_FS_SEL_8G:
		sensitivity = 4096;
		break;
	case ICM20608_ACCEL_FS_SEL_16G:
		sensitivity = 2048;
		break;
	default:
		return -EINVAL;
	}

	/* Convert to micrometers/s^2 */
	int64_t in_ms = raw_accel_value * SENSOR_G;

	/* meters/s^2 whole values */
	output_value->val1 = in_ms / (sensitivity * 1000000LL);

	/* micrometers/s^2 */
	output_value->val2 = (in_ms - (output_value->val1 * sensitivity * 1000000LL)) / sensitivity;

	return 0;
}

static int icm20608_convert_gyro(struct sensor_value *val, int16_t raw_val, uint8_t gyro_fs)
{
	int64_t gyro_sensitivity_10x = 0; /* value equivalent for 10x gyro reading deg/s */

	switch (gyro_fs) {
	case ICM20608_GYRO_FS_250:
		gyro_sensitivity_10x = 1310;
		break;
	case ICM20608_GYRO_FS_500:
		gyro_sensitivity_10x = 655;
		break;
	case ICM20608_GYRO_FS_1000:
		gyro_sensitivity_10x = 328;
		break;
	case ICM20608_GYRO_FS_2000:
		gyro_sensitivity_10x = 164;
		break;
	default:
		return -EINVAL;
	}

	int64_t in10_rads = (int64_t)raw_val * SENSOR_PI * 10LL;

	/* Whole rad/s */
	val->val1 = in10_rads / (gyro_sensitivity_10x * ICM20608_DEG_TO_RAD * 1000000LL);

	/* microrad/s */
	val->val2 =
		(in10_rads - (val->val1 * gyro_sensitivity_10x * ICM20608_DEG_TO_RAD * 1000000LL)) /
		(gyro_sensitivity_10x * ICM20608_DEG_TO_RAD);

	return 0;
}

static int icm20608_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icm20608_data *dev_data = dev->data;
	const struct icm20608_config *cfg = dev->config;

	switch (chan) {
	case SENSOR_CHAN_DIE_TEMP:
		icm20608_convert_temp(val, dev_data->temp);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		icm20608_convert_accel(cfg, dev_data->accel_x, val);
		icm20608_convert_accel(cfg, dev_data->accel_y, val + 1);
		icm20608_convert_accel(cfg, dev_data->accel_z, val + 2);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm20608_convert_accel(cfg, dev_data->accel_x, val);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm20608_convert_accel(cfg, dev_data->accel_y, val);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm20608_convert_accel(dev->config, dev_data->accel_z, val);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm20608_convert_gyro(val, dev_data->gyro_x, cfg->gyro_fs);
		icm20608_convert_gyro(val + 1, dev_data->gyro_y, cfg->gyro_fs);
		icm20608_convert_gyro(val + 2, dev_data->gyro_z, cfg->gyro_fs);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm20608_convert_gyro(val, dev_data->gyro_x, cfg->gyro_fs);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm20608_convert_gyro(val, dev_data->gyro_y, cfg->gyro_fs);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm20608_convert_gyro(val, dev_data->gyro_z, cfg->gyro_fs);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static bool icm20608_ready_to_read(const struct device *dev)
{
	const struct icm20608_config *cfg = dev->config;
	uint8_t ready_to_read = 0;

	int ret = i2c_reg_read_byte_dt(&cfg->i2c, ICM20608_REG_INT_STATUS, &ready_to_read);

	if (ret) {
		LOG_ERR("data not ready to read.\n");
		return false;
	}

	return ready_to_read;
}

static int icm20608_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct icm20608_data *drv_data = dev->data;
	const struct icm20608_config *cfg = dev->config;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	uint8_t read_buff[ICM20608_SENS_READ_BUFF_LEN];

	if (!icm20608_ready_to_read(dev)) {
		return -EBUSY;
	}

	int err = i2c_burst_read_dt(&cfg->i2c, ICM20608_REG_ACCEL_XOUT_H, read_buff,
				    ICM20608_SENS_READ_BUFF_LEN);

	if (err) {
		LOG_ERR("Error reading acc and gyro values");
		return err;
	}

	drv_data->accel_x = (int16_t)(read_buff[0] << 8 | read_buff[1]);
	drv_data->accel_y = (int16_t)(read_buff[2] << 8 | read_buff[3]);
	drv_data->accel_z = (int16_t)(read_buff[4] << 8 | read_buff[5]);

	drv_data->temp = (int16_t)(read_buff[6] << 8 | read_buff[7]);

	drv_data->gyro_x = (int16_t)(read_buff[8] << 8 | read_buff[9]);
	drv_data->gyro_y = (int16_t)(read_buff[10] << 8 | read_buff[11]);
	drv_data->gyro_z = (int16_t)(read_buff[12] << 8 | read_buff[13]);

	return 0;
}

static const struct sensor_driver_api icm20608_driver_api = {
#if CONFIG_ICM20608_TRIGGER
	.trigger_set = icm20608_trigger_set,
#endif
	.sample_fetch = icm20608_sample_fetch,
	.channel_get = icm20608_channel_get};

static int icm20608_wake_up(const struct device *dev)
{
	const struct icm20608_config *cfg = dev->config;

	int err = i2c_reg_update_byte_dt(&cfg->i2c, ICM20608_REG_PWR_MGMT_1, BIT(6), 0x00);

	if (err) {
		LOG_ERR("Error waking up device.");
		return err;
	}

	return 0;
}

static int icm20608_reset(const struct device *dev)
{
	const struct icm20608_config *cfg = dev->config;

	int err = i2c_reg_update_byte_dt(&cfg->i2c, ICM20608_REG_PWR_MGMT_1, BIT(7), 0xFF);

	if (err) {
		LOG_ERR("Error resetting device.");
		return err;
	}

	k_sleep(K_MSEC(120)); /* wait for sensor to ramp up after resetting */

	return 0;
}

static int icm20608_sample_rate_config(const struct device *dev)
{
	const struct icm20608_config *cfg = dev->config;

	int err = i2c_reg_update_byte_dt(&cfg->i2c, ICM20608_REG_PWR_MGMT_1, BIT(6), 0x00);

	if (err) {
		LOG_ERR("Error waking up device.");
		return err;
	}

	return 0;
}

static int icm20608_icm20608_gyro_accel_enable(const struct device *dev)
{
	const struct icm20608_config *cfg = dev->config;

	int err = i2c_reg_write_byte_dt(&cfg->i2c, ICM20608_REG_PWR_MGMT_2, 0);

	if (err) {
		LOG_ERR("Error enabling device.");
		return err;
	}

	return 0;
}

static int icm20608_gyro_config(const struct device *dev)
{
	const struct icm20608_config *cfg = dev->config;

	if (cfg->gyro_fs > ICM20608_GYRO_FS_MAX) {
		LOG_ERR("Gyro FS is too big: %d", cfg->gyro_fs);
		return -EINVAL;
	}

	int ret = i2c_reg_write_byte_dt(&cfg->i2c, ICM20608_REG_GYRO_CONFIG,
					cfg->gyro_fs << ICM20608_GYRO_FS_SHIFT);
	if (ret < 0) {
		LOG_ERR("Failed to write gyro full-scale range.");
		return ret;
	}

	if (cfg->gyro_dlpf > ICM20608_GYRO_DLPF_MAX) {
		LOG_ERR("Gyro DLPF is too big: %d", cfg->gyro_dlpf);
		return -EINVAL;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, ICM20608_REG_CONFIG, cfg->gyro_dlpf);
	if (ret < 0) {
		LOG_ERR("Failed to write gyro digital LPF settings.");
		return ret;
	}

	return 0;
}

static int icm20608_accel_config(const struct device *dev)
{
	const struct icm20608_config *cfg = dev->config;
	struct icm20608_data *drv_data = dev->data;

	if (cfg->accel_fs > ICM20608_ACCEL_FS_MAX) {
		LOG_ERR("Accel FS is too big: %d", cfg->accel_fs);
		return -EINVAL;
	}

	int ret = i2c_reg_write_byte_dt(&cfg->i2c, ICM20608_REG_ACCEL_CONFIG,
					cfg->accel_fs << ICM20608_ACCEL_FS_SHIFT);
	if (ret < 0) {
		LOG_ERR("Failed to write accel full-scale range.");
		return ret;
	}
	drv_data->accel_sensitivity_shift = 14 - cfg->accel_fs;

	if (cfg->accel_dlpf > ICM20608_ACCEL_DLPF_MAX) {
		LOG_ERR("Accel DLPF is too big: %d", cfg->accel_dlpf);
		return -EINVAL;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, ICM20608_REG_ACCEL_CONFIG2, cfg->accel_dlpf);
	if (ret < 0) {
		LOG_ERR("Failed to write accel digital LPF settings.");
		return ret;
	}

	return 0;
}

static int icm20608_init(const struct device *dev)
{
	const struct icm20608_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus is not ready.");
		return -ENODEV;
	}

	/* check chip ID */
	uint8_t id;

	int ret = i2c_reg_read_byte_dt(&cfg->i2c, ICM20608_REG_WHO_AM_I, &id);

	if (ret < 0) {
		LOG_ERR("Failed to read chip ID.");
		return ret;
	}

	if ((id != ICM20608D_DEVICE_ID) && (id != ICM20608G_DEVICE_ID)) {
		LOG_ERR("Invalid chip ID.");
		return -ENOTSUP;
	}

	int err = icm20608_reset(dev);

	if (err) {
		return err;
	}

	err = icm20608_wake_up(dev);
	if (err) {
		return err;
	}

	err = icm20608_sample_rate_config(dev);
	if (err) {
		return err;
	}

	err = icm20608_gyro_config(dev);
	if (err) {
		return err;
	}

	err = icm20608_accel_config(dev);
	if (err) {
		return err;
	}

	err = icm20608_icm20608_gyro_accel_enable(dev);
	if (err) {
		return err;
	}

#ifdef CONFIG_ICM20608_TRIGGER
	ret = icm20608_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupts.");
		return ret;
	}
#endif

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

#define ICM20608_DEFINE(inst)                                                                      \
	static struct icm20608_data icm20608_data_##inst;                                          \
                                                                                                   \
	static const struct icm20608_config icm20608_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.gyro_sr_div = DT_INST_PROP(inst, gyro_sr_div),                                    \
		.gyro_dlpf = DT_INST_ENUM_IDX(inst, gyro_dlpf),                                    \
		.gyro_fs = DT_INST_ENUM_IDX(inst, gyro_fs),                                        \
		.accel_fs = DT_INST_ENUM_IDX(inst, accel_fs),                                      \
		.accel_dlpf = DT_INST_ENUM_IDX(inst, accel_dlpf),                                  \
		IF_ENABLED(CONFIG_ICM20608_TRIGGER,                                         \
					(.irq_pin = GPIO_DT_SPEC_INST_GET(inst, irq_gpios))) }; \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm20608_init, NULL, &icm20608_data_##inst,             \
				     &icm20608_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &icm20608_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ICM20608_DEFINE)
