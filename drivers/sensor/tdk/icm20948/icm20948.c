/*
 * Copyright (c) 2026 T3 Gemstone
 *	T3 Gemstone Developer Team <support@t3gemstone.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm20948

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "icm20948.h"
#include "icm20948_reg.h"

LOG_MODULE_REGISTER(ICM20948, CONFIG_SENSOR_LOG_LEVEL);

#define ICM20948_SPI_CFG                                                                           \
	(SPI_OP_MODE_CONTROLLER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) |                \
	 SPI_TRANSFER_MSB)

/* The datasheet gives 100 ms as the worst case start-up time for register access */
#define ICM20948_RESET_DELAY_MS 100

/* Gyroscope sensitivity in LSB/dps, scaled by ten to stay integral */
static const uint16_t icm20948_gyro_sensitivity_x10[] = {1310, 655, 328, 164};

/* Maps the index of the accel-dlpf enum to ACCEL_DLPFCFG. Configurations 0
 * and 1 give the same bandwidth, so only the first one is exposed.
 */
static const uint8_t icm20948_accel_dlpf[] = {0, 2, 3, 4, 5, 6, 7};

static void icm20948_convert_accel(struct sensor_value *val, int16_t raw, uint8_t shift)
{
	int64_t conv = ((int64_t)raw * SENSOR_G) >> shift;

	val->val1 = conv / 1000000;
	val->val2 = conv % 1000000;
}

static void icm20948_convert_gyro(struct sensor_value *val, int16_t raw, uint16_t sensitivity_x10)
{
	int64_t conv = ((int64_t)raw * SENSOR_PI * 10) / (sensitivity_x10 * 180);

	val->val1 = conv / 1000000;
	val->val2 = conv % 1000000;
}

static void icm20948_convert_temp(struct sensor_value *val, int16_t raw)
{
	int64_t conv = ((int64_t)raw * 100 * 1000000) / 33387 + 21000000;

	val->val1 = conv / 1000000;
	val->val2 = conv % 1000000;
}

static int icm20948_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct icm20948_data *data = dev->data;
	uint8_t buf[ICM20948_DATA_LEN];
	int ret;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);
	ret = icm20948_read(dev, REG_ACCEL_XOUT_H, buf, sizeof(buf));
	k_sem_give(&data->lock);

	if (ret < 0) {
		LOG_ERR("failed to read sample: %d", ret);
		return ret;
	}

	for (int i = 0; i < 3; i++) {
		data->accel[i] = (int16_t)sys_get_be16(&buf[i * 2]);
		data->gyro[i] = (int16_t)sys_get_be16(&buf[6 + i * 2]);
	}
	data->temp = (int16_t)sys_get_be16(&buf[12]);

	return 0;
}

static int icm20948_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct icm20948_config *cfg = dev->config;
	struct icm20948_data *data = dev->data;
	uint8_t shift = 14 - cfg->accel_fs;
	uint16_t sens = icm20948_gyro_sensitivity_x10[cfg->gyro_fs];

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		icm20948_convert_accel(&val[0], data->accel[0], shift);
		icm20948_convert_accel(&val[1], data->accel[1], shift);
		icm20948_convert_accel(&val[2], data->accel[2], shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm20948_convert_accel(val, data->accel[0], shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm20948_convert_accel(val, data->accel[1], shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm20948_convert_accel(val, data->accel[2], shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm20948_convert_gyro(&val[0], data->gyro[0], sens);
		icm20948_convert_gyro(&val[1], data->gyro[1], sens);
		icm20948_convert_gyro(&val[2], data->gyro[2], sens);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm20948_convert_gyro(val, data->gyro[0], sens);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm20948_convert_gyro(val, data->gyro[1], sens);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm20948_convert_gyro(val, data->gyro[2], sens);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm20948_convert_temp(val, data->temp);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int icm20948_init(const struct device *dev)
{
	const struct icm20948_config *cfg = dev->config;
	struct icm20948_data *data = dev->data;
	uint8_t accel_dlpf = icm20948_accel_dlpf[cfg->accel_dlpf];
	uint8_t id;
	int ret;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);
	data->bank = ICM20948_BANK_UNKNOWN;

	ret = icm20948_read(dev, REG_WHO_AM_I, &id, sizeof(id));
	if (ret < 0) {
		LOG_ERR("failed to read WHO_AM_I: %d", ret);
		return ret;
	}

	if (id != WHO_AM_I_ICM20948) {
		LOG_ERR("unexpected WHO_AM_I 0x%02x", id);
		return -ENODEV;
	}

	ret = icm20948_write(dev, REG_PWR_MGMT_1, BIT_DEVICE_RESET);
	if (ret < 0) {
		LOG_ERR("failed to reset the device: %d", ret);
		return ret;
	}

	/* The reset restores the default register values, the selected bank
	 * among them, so the cached value is no longer valid.
	 */
	data->bank = ICM20948_BANK_UNKNOWN;
	k_msleep(ICM20948_RESET_DELAY_MS);

	/* Leaves sleep mode and lets the device pick its own clock source */
	ret = icm20948_write(dev, REG_PWR_MGMT_1, FIELD_PREP(MASK_CLKSEL, CLKSEL_AUTO));
	if (ret < 0) {
		LOG_ERR("failed to wake the device: %d", ret);
		return ret;
	}

	ret = icm20948_write(dev, REG_GYRO_CONFIG_1,
			     FIELD_PREP(MASK_GYRO_DLPFCFG, cfg->gyro_dlpf) |
				     FIELD_PREP(MASK_GYRO_FS_SEL, cfg->gyro_fs) | BIT_GYRO_FCHOICE);
	if (ret < 0) {
		LOG_ERR("failed to configure the gyroscope: %d", ret);
		return ret;
	}

	ret = icm20948_write(dev, REG_ACCEL_CONFIG,
			     FIELD_PREP(MASK_ACCEL_DLPFCFG, accel_dlpf) |
				     FIELD_PREP(MASK_ACCEL_FS_SEL, cfg->accel_fs) |
				     BIT_ACCEL_FCHOICE);
	if (ret < 0) {
		LOG_ERR("failed to configure the accelerometer: %d", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, icm20948_driver_api) = {
	.sample_fetch = icm20948_sample_fetch,
	.channel_get = icm20948_channel_get,
};

#define ICM20948_DEFINE(inst)                                                                      \
	static struct icm20948_data icm20948_data_##inst;                                          \
                                                                                                   \
	static const struct icm20948_config icm20948_config_##inst = {                             \
		.spi = SPI_DT_SPEC_INST_GET(inst, ICM20948_SPI_CFG),                               \
		.gyro_fs = DT_INST_ENUM_IDX(inst, gyro_fs),                                        \
		.accel_fs = DT_INST_ENUM_IDX(inst, accel_fs),                                      \
		.gyro_dlpf = DT_INST_ENUM_IDX(inst, gyro_dlpf),                                    \
		.accel_dlpf = DT_INST_ENUM_IDX(inst, accel_dlpf),                                  \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm20948_init, NULL, &icm20948_data_##inst,             \
				     &icm20948_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &icm20948_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ICM20948_DEFINE)
