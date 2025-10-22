/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm4268x

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/icm4268x.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>

#include "icm4268x.h"
#include "icm4268x_decoder.h"
#include "icm4268x_reg.h"
#include "icm4268x_rtio.h"
#include "icm4268x_spi.h"
#include "icm4268x_trigger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ICM4268X, CONFIG_SENSOR_LOG_LEVEL);

static void icm4268x_convert_accel(struct sensor_value *val, int16_t raw_val,
				   struct icm4268x_cfg *cfg)
{
	icm4268x_accel_ms(cfg, (int32_t)raw_val, &val->val1, &val->val2);
}

static void icm4268x_convert_gyro(struct sensor_value *val, int16_t raw_val,
				  struct icm4268x_cfg *cfg)
{
	icm4268x_gyro_rads(cfg, (int32_t)raw_val, &val->val1, &val->val2);
}

static inline void icm4268x_convert_temp(struct sensor_value *val, int16_t raw_val)
{
	icm4268x_temp_c((int32_t)raw_val, &val->val1, &val->val2);
}

int icm4268x_channel_parse_readings(enum sensor_channel chan, int16_t readings[7],
				    struct icm4268x_cfg *cfg, struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		icm4268x_convert_accel(&val[0],
			cfg->axis_align[0].sign*readings[cfg->axis_align[0].index + 1], cfg);
		icm4268x_convert_accel(&val[1],
			cfg->axis_align[1].sign*readings[cfg->axis_align[1].index + 1], cfg);
		icm4268x_convert_accel(&val[2],
			cfg->axis_align[2].sign*readings[cfg->axis_align[2].index + 1], cfg);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm4268x_convert_accel(val,
			cfg->axis_align[0].sign*readings[cfg->axis_align[0].index + 1], cfg);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm4268x_convert_accel(val,
			cfg->axis_align[1].sign*readings[cfg->axis_align[1].index + 1], cfg);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm4268x_convert_accel(val,
			cfg->axis_align[2].sign*readings[cfg->axis_align[2].index + 1], cfg);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm4268x_convert_gyro(&val[0],
			cfg->axis_align[0].sign*readings[cfg->axis_align[0].index + 4], cfg);
		icm4268x_convert_gyro(&val[1],
			cfg->axis_align[1].sign*readings[cfg->axis_align[1].index + 4], cfg);
		icm4268x_convert_gyro(&val[2],
			cfg->axis_align[2].sign*readings[cfg->axis_align[2].index + 4], cfg);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm4268x_convert_gyro(val,
			cfg->axis_align[0].sign*readings[cfg->axis_align[0].index + 4], cfg);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm4268x_convert_gyro(val,
			cfg->axis_align[1].sign*readings[cfg->axis_align[1].index + 4], cfg);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm4268x_convert_gyro(val,
			cfg->axis_align[2].sign*readings[cfg->axis_align[2].index + 4], cfg);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		icm4268x_convert_temp(val, readings[0]);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int icm4268x_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icm4268x_dev_data *data = dev->data;

	return icm4268x_channel_parse_readings(chan, data->readings, &data->cfg, val);
}

static int icm4268x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	uint8_t status;
	struct icm4268x_dev_data *data = dev->data;
	const struct icm4268x_dev_cfg *cfg = dev->config;

	int res = icm4268x_spi_read(&cfg->spi, REG_INT_STATUS, &status, 1);

	if (res) {
		return res;
	}

	if (!FIELD_GET(BIT_DATA_RDY_INT, status)) {
		return -EBUSY;
	}

	uint8_t readings[14];

	res = icm4268x_read_all(dev, readings);

	if (res) {
		return res;
	}

	for (int i = 0; i < 7; i++) {
		data->readings[i] = sys_le16_to_cpu((readings[i * 2] << 8) | readings[i * 2 + 1]);
	}

	return 0;
}

static int icm4268x_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct icm4268x_dev_data *data = dev->data;
	struct icm4268x_cfg new_config = data->cfg;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			new_config.accel_odr = icm4268x_accel_hz_to_reg(val->val1);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			new_config.accel_fs = icm4268x_accel_fs_to_reg(sensor_ms2_to_g(val),
								       data->cfg.variant);
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
			new_config.gyro_odr = icm4268x_gyro_odr_to_reg(val->val1);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			new_config.gyro_fs = icm4268x_gyro_fs_to_reg(sensor_rad_to_degrees(val),
								     data->cfg.variant);
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;
	case SENSOR_CHAN_ALL:
		if (attr == SENSOR_ATTR_BATCH_DURATION) {
			if (val->val1 < 0) {
				return -EINVAL;
			}
			new_config.batch_ticks = val->val1;
		} else if ((enum sensor_attribute_icm4268x)attr ==
			   SENSOR_ATTR_ICM4268X_PIN9_FUNCTION) {
			if (val->val1 != ICM4268X_PIN9_FUNCTION_INT2 &&
			    val->val1 != ICM4268X_PIN9_FUNCTION_FSYNC &&
			    val->val1 != ICM4268X_PIN9_FUNCTION_CLKIN) {
				LOG_ERR("Unknown pin function");
				return -EINVAL;
			}

			if (val->val2 < 31000 || val->val2 > 50000) {
				LOG_ERR("RTC frequency must be between 31kHz and 50kHz");
				return -EINVAL;
			}

			/* TODO: Allow this if FSYNC is configurable later. */
			if (val->val1 == ICM4268X_PIN9_FUNCTION_FSYNC) {
				LOG_ERR("FSYNC is disabled, PIN9_FUNCTION should not be set to "
					"FSYNC");
				return -ENOTSUP;
			}

			new_config.pin9_function = val->val1;
			new_config.rtc_freq = val->val2;
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

	if (res) {
		return res;
	}
	return icm4268x_safely_configure(dev, &new_config);
}

static int icm4268x_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	const struct icm4268x_dev_data *data = dev->data;
	const struct icm4268x_cfg *cfg = &data->cfg;
	int res = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			icm4268x_accel_reg_to_hz(cfg->accel_odr, val);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			icm4268x_accel_reg_to_fs(cfg->accel_fs, data->cfg.variant, val);
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
			icm4268x_gyro_reg_to_odr(cfg->gyro_odr, val);
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			icm4268x_gyro_reg_to_fs(cfg->gyro_fs, data->cfg.variant, val);
		} else {
			LOG_ERR("Unsupported attribute");
			res = -EINVAL;
		}
		break;
	case SENSOR_CHAN_ALL:
		if (attr == SENSOR_ATTR_BATCH_DURATION) {
			val->val1 = cfg->batch_ticks;
			val->val2 = 0;
		} else if ((enum sensor_attribute_icm4268x)attr ==
			   SENSOR_ATTR_ICM4268X_PIN9_FUNCTION) {
			val->val1 = cfg->pin9_function;
			val->val2 = cfg->rtc_freq;
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

	return res;
}

static DEVICE_API(sensor, icm4268x_driver_api) = {
	.sample_fetch = icm4268x_sample_fetch,
	.channel_get = icm4268x_channel_get,
	.attr_set = icm4268x_attr_set,
	.attr_get = icm4268x_attr_get,
#ifdef CONFIG_ICM4268X_TRIGGER
	.trigger_set = icm4268x_trigger_set,
#endif
	.get_decoder = icm4268x_get_decoder,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = icm4268x_submit,
#endif
};

int icm4268x_init(const struct device *dev)
{
	struct icm4268x_dev_data *data = dev->data;
	const struct icm4268x_dev_cfg *cfg = dev->config;
	int res;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	if (icm4268x_reset(dev)) {
		LOG_ERR("could not initialize sensor");
		return -EIO;
	}

#ifdef CONFIG_ICM4268X_TRIGGER
	res = icm4268x_trigger_init(dev);
	if (res != 0) {
		LOG_ERR("Failed to initialize triggers");
		return res;
	}
#endif

	res = icm4268x_configure(dev, &data->cfg);
	if (res != 0) {
		LOG_ERR("Failed to configure");
		return res;
	}

	return 0;
}

#ifndef CONFIG_ICM4268X_TRIGGER
void icm4268x_lock(const struct device *dev)
{
	ARG_UNUSED(dev);
}
void icm4268x_unlock(const struct device *dev)
{
	ARG_UNUSED(dev);
}
#endif

/* device defaults to spi mode 0/3 support */
#define ICM4268X_SPI_CFG                                                                           \
	SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define ICM4268X_RTIO_DEFINE(inst)                                                                 \
	SPI_DT_IODEV_DEFINE(icm4268x_spi_iodev_##inst, DT_DRV_INST(inst), ICM4268X_SPI_CFG);       \
	RTIO_DEFINE(icm4268x_rtio_##inst, 32, 32);

#define ICM42688_DT_CONFIG_INIT(inst)						\
	{									\
		COND_CODE_1(DT_INST_NODE_HAS_COMPAT(inst, invensense_icm42688),	\
			(.variant = ICM4268X_VARIANT_ICM42688,), (		\
		COND_CODE_1(DT_INST_NODE_HAS_COMPAT(inst, invensense_icm42686),	\
			(.variant = ICM4268X_VARIANT_ICM42686,), ())))		\
		.accel_pwr_mode = DT_INST_PROP(inst, accel_pwr_mode),		\
		.accel_fs = DT_INST_PROP(inst, accel_fs),			\
		.accel_odr = DT_INST_PROP(inst, accel_odr),			\
		.gyro_pwr_mode = DT_INST_PROP(inst, gyro_pwr_mode),		\
		.gyro_fs = DT_INST_PROP(inst, gyro_fs),				\
		.gyro_odr = DT_INST_PROP(inst, gyro_odr),			\
		.temp_dis = false,						\
		.fifo_en = IS_ENABLED(CONFIG_ICM4268X_STREAM),			\
		.batch_ticks = 0,						\
		.fifo_hires = DT_INST_PROP(inst, fifo_hires),			\
		.interrupt1_drdy = false,					\
		.interrupt1_fifo_ths = false,					\
		.interrupt1_fifo_full = false,					\
		.pin9_function = ICM4268X_PIN9_FUNCTION_INT2,			\
		.rtc_freq = 32000,						\
		.axis_align[0].index = DT_INST_PROP(inst, axis_align_x),	\
		.axis_align[1].index = DT_INST_PROP(inst, axis_align_y),	\
		.axis_align[2].index = DT_INST_PROP(inst, axis_align_z),	\
		.axis_align[0].sign = DT_INST_PROP(inst, axis_align_x_sign)-1,	\
		.axis_align[1].sign = DT_INST_PROP(inst, axis_align_y_sign)-1,	\
		.axis_align[2].sign = DT_INST_PROP(inst, axis_align_z_sign)-1	\
	}

#define ICM4268X_DEFINE_DATA(inst)                                                                 \
	IF_ENABLED(CONFIG_ICM4268X_STREAM, (ICM4268X_RTIO_DEFINE(inst)));                          \
	static struct icm4268x_dev_data icm4268x_driver_##inst = {                                 \
		.cfg = ICM42688_DT_CONFIG_INIT(inst),                                              \
		IF_ENABLED(CONFIG_ICM4268X_STREAM,						   \
		(										   \
			.bus.rtio = {								   \
				.ctx = &icm4268x_rtio_##inst,					   \
				.iodev = &icm4268x_spi_iodev_##inst,				   \
			},									   \
		))										   \
	};

/** The rest of the Device-tree configuration is validated in the YAML
 * file. This outlier comes from the fact we're sharing the dts-properties
 * across variants, and ICM42686 has an extra enum for the accel-fs.
 */
#define ICM42688_BUILD_CONFIG_VALIDATION(inst)							   \
	BUILD_ASSERT((DT_INST_PROP(inst, accel_fs) >= ICM42688_DT_ACCEL_FS_16) &&		   \
		     (DT_INST_PROP(inst, accel_fs) <=  ICM42688_DT_ACCEL_FS_2),			   \
		"Invalid accel-fs configured for ICM42688. Please revisit DTS config-set");

#define ICM4268X_INIT(inst)                                                                        \
												   \
	BUILD_ASSERT(DT_INST_NODE_HAS_COMPAT(inst, invensense_icm42688) ||			   \
		     DT_INST_NODE_HAS_COMPAT(inst, invensense_icm42686),			   \
		"Please define additional compatible property to dts node: "			   \
		"<invensense,icm42688> or <invensense,icm42686>");				   \
												   \
												   \
	COND_CODE_1(DT_INST_NODE_HAS_COMPAT(inst, invensense_icm42688),				   \
		    (ICM42688_BUILD_CONFIG_VALIDATION(inst)), ());				   \
												   \
	ICM4268X_DEFINE_DATA(inst);                                                                \
                                                                                                   \
	static const struct icm4268x_dev_cfg icm4268x_cfg_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst, ICM4268X_SPI_CFG),                               \
		.gpio_int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                       \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, icm4268x_init, NULL, &icm4268x_driver_##inst,           \
				     &icm4268x_cfg_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &icm4268x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ICM4268X_INIT)
