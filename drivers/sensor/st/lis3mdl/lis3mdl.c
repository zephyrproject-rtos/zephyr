/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis3mdl_magn

#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include "lis3mdl.h"

LOG_MODULE_REGISTER(LIS3MDL, CONFIG_SENSOR_LOG_LEVEL);

static void lis3mdl_convert(struct sensor_value *val, int16_t raw_val,
			    uint16_t divider)
{
	/* val = raw_val / divider */
	val->val1 = raw_val / divider;
	val->val2 = (((int64_t)raw_val % divider) * 1000000L) / divider;
}

static int lis3mdl_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lis3mdl_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_MAGN_XYZ) {
		/* magn_val = sample / magn_gain */
		lis3mdl_convert(val, drv_data->x_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
		lis3mdl_convert(val + 1, drv_data->y_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
		lis3mdl_convert(val + 2, drv_data->z_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else if (chan == SENSOR_CHAN_MAGN_X) {
		lis3mdl_convert(val, drv_data->x_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else if (chan == SENSOR_CHAN_MAGN_Y) {
		lis3mdl_convert(val, drv_data->y_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	} else if (chan == SENSOR_CHAN_MAGN_Z) {
		lis3mdl_convert(val, drv_data->z_sample,
				lis3mdl_magn_gain[LIS3MDL_FS_IDX]);
	#ifdef CONFIG_LIS3MDL_DIE_TEMP_EN
	} else if (chan == SENSOR_CHAN_DIE_TEMP) {
		/* temp_val = 25 + sample / 8 */
		lis3mdl_convert(val, drv_data->temp_sample, 8);
		val->val1 += 25;
	#endif /*CONFIG_LIS3MDL_DIE_TEMP_EN*/
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int lis3mdl_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct lis3mdl_data *drv_data = dev->data;
	#ifdef CONFIG_LIS3MDL_DIE_TEMP_EN
	int16_t buf[4];
	#else /*CONFIG_LIS3MDL_DIE_TEMP_EN*/
	int16_t buf[3];
	#endif /*CONFIG_LIS3MDL_DIE_TEMP_EN*/

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* fetch magnetometer sample */
	if (drv_data->hw_tf->read_data(dev, LIS3MDL_REG_SAMPLE_START,
			      (uint8_t *)buf, 6) < 0) {
		LOG_DBG("Failed to fetch magnetometer sample.");
		return -EIO;
	}

	#ifdef CONFIG_LIS3MDL_DIE_TEMP_EN
	/*
	 * the chip doesn't allow fetching temperature data in
	 * the same read as magnetometer data, so do another
	 * burst read to fetch the temperature sample
	 */
	if (drv_data->hw_tf->read_data(dev, LIS3MDL_REG_SAMPLE_START + 6,
			      (uint8_t *)(buf + 3), 2) < 0) {
		LOG_DBG("Failed to fetch temperature sample.");
		return -EIO;
	}
	drv_data->temp_sample = sys_le16_to_cpu(buf[3]);
	#endif /*CONFIG_LIS3MDL_DIE_TEMP_EN*/

	drv_data->x_sample = sys_le16_to_cpu(buf[0]);
	drv_data->y_sample = sys_le16_to_cpu(buf[1]);
	drv_data->z_sample = sys_le16_to_cpu(buf[2]);

	return 0;
}

static const struct sensor_driver_api lis3mdl_driver_api = {
#if CONFIG_LIS3MDL_TRIGGER
	.trigger_set = lis3mdl_trigger_set,
#endif
	.sample_fetch = lis3mdl_sample_fetch,
	.channel_get = lis3mdl_channel_get,
};

int lis3mdl_init(const struct device *dev)
{
	const struct lis3mdl_config *config = dev->config;
	const struct lis3mdl_data *data = dev->data;
	uint8_t chip_cfg[5];
	uint8_t id, idx;

	int ret;

	ret = config->bus_init(dev);
	if (ret < 0) {
		LOG_ERR("bus device not ready");
		return -ENODEV;
	}

	/* check chip ID */
	if (data->hw_tf->read_reg(dev, LIS3MDL_REG_WHO_AM_I, &id) < 0) {
		LOG_ERR("Failed to read chip ID.");
		return -EIO;
	}

	if (id != LIS3MDL_CHIP_ID) {
		LOG_ERR("Invalid chip ID.");
		return -EINVAL;
	}

	/* check if CONFIG_LIS3MDL_ODR is valid */
	for (idx = 0U; idx < ARRAY_SIZE(lis3mdl_odr_strings); idx++) {
		if (!strcmp(lis3mdl_odr_strings[idx], CONFIG_LIS3MDL_ODR)) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(lis3mdl_odr_strings)) {
		LOG_ERR("Invalid ODR value.");
		return -EINVAL;
	}

	/* Configure sensor */
	chip_cfg[0] = lis3mdl_odr_bits[idx];
	#ifdef CONFIG_LIS3MDL_DIE_TEMP_EN
	chip_cfg[0] |= LIS3MDL_TEMP_EN_MASK;
	#endif /*LIS3MDL_DIE_TEMP_EN*/
	chip_cfg[1] = LIS3MDL_FS_IDX << LIS3MDL_FS_SHIFT;
	chip_cfg[2] = LIS3MDL_MD_CONTINUOUS;
	chip_cfg[3] = ((lis3mdl_odr_bits[idx] & LIS3MDL_OM_MASK) >>
		       LIS3MDL_OM_SHIFT) << LIS3MDL_OMZ_SHIFT;
	chip_cfg[4] = LIS3MDL_BDU_EN;

	if (data->hw_tf->write_data(dev, LIS3MDL_REG_CTRL1, chip_cfg, 5) < 0) {
		LOG_DBG("Failed to configure chip.");
		return -EIO;
	}

#ifdef CONFIG_LIS3MDL_TRIGGER
	if (config->irq_gpio.port) {
		if (lis3mdl_init_interrupt(dev) < 0) {
			LOG_DBG("Failed to initialize interrupts.");
			return -EIO;
		}
	}
#endif

	return 0;
}



/*
 * Device creation macro, shared by LIS3MDL_DEFINE_SPI() and
 * LIS3MDL_DEFINE_I2C().
 */

#define LIS3MDL_DEVICE_INIT(inst)					\
		\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			    lis3mdl_init,				\
			    NULL,		\
			    &lis3mdl_data_##inst,			\
			    &lis3mdl_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &lis3mdl_driver_api);

#ifdef CONFIG_LIS3MDL_TRIGGER
#define LIS3MDL_CFG_IRQ(inst) \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios, { 0 }),
#else
#define LIS3MDL_CFG_IRQ(inst)
#endif /* CONFIG_LIS3MDL_TRIGGER */

/*
 * config macro used when a device is on a SPI bus.
 */

#define LIS3MDL_CONFIG_SPI(inst)						\
	{													\
		.bus_init = lis3mdl_spi_init,					\
		.bus_cfg.spi = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8) |	\
				      SPI_OP_MODE_MASTER |				\
				      SPI_MODE_CPOL |					\
				      SPI_MODE_CPHA, 0),				\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios),		\
		(LIS3MDL_CFG_IRQ(inst)), ())					\
	}

/*
 * config macro used when a device is on an I2C bus.
 */

#define LIS3MDL_CONFIG_I2C(inst)					\
	{								\
		.bus_init = lis3mdl_i2c_init,				\
		.bus_cfg.i2c = I2C_DT_SPEC_INST_GET(inst),		\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios),	\
		(LIS3MDL_CFG_IRQ(inst)), ())				\
	}

#define LIS3MDL_DEFINE(inst)						\
	static struct lis3mdl_data lis3mdl_data_##inst;			\
	static struct lis3mdl_config lis3mdl_config_##inst =	\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		(LIS3MDL_CONFIG_SPI(inst)),				\
		(LIS3MDL_CONFIG_I2C(inst)))				\
	LIS3MDL_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(LIS3MDL_DEFINE)
