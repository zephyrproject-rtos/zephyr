/*
 * Copyright (c) 2024 Ilia Kharin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cirque_pinnacle

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include "pinnacle.h"

LOG_MODULE_REGISTER(PINNACLE, CONFIG_SENSOR_LOG_LEVEL);

static int pinnacle_init(const struct device *dev)
{
	const struct pinnacle_config *config = dev->config;
	const struct spi_dt_spec *spi = &config->spi;

	int ret;
	uint8_t value;

	if (pinnacle_check_spi(spi)) {
		return -ENODEV;
	}

	ret = pinnacle_read_spi(spi, PINNACLE_REG_FIRMWARE_ID, &value);
	if (ret) {
		LOG_ERR("Failed to read FirmwareId");
		return ret;
	}

	if (value != PINNACLE_FIRMWARE_ID) {
		LOG_ERR("Incorrect Firmware ASIC ID %x", value);
		return -ENODEV;
	}

	/* Wait until the calibration is completed (SW_CC is asserted) */
	while (1) {
		ret = pinnacle_read_spi(spi, PINNACLE_REG_STATUS1, &value);
		if (ret) {
			LOG_ERR("Failed to read Status1");
			return ret;
		}
		if ((value & PINNACLE_STATUS1_SW_CC) ==
				PINNACLE_STATUS1_SW_CC) {
			break;
		}
		k_msleep(50);
	}

	/* Clear SW_CC after Power on Reset */
	if (pinnacle_write_spi(spi, PINNACLE_REG_STATUS1, 0x00)) {
		LOG_ERR("Failed to clear SW_CC in Status1");
		return -EIO;
	}

	ret = pinnacle_write_spi(spi, PINNACLE_REG_SYS_CONFIG1, 0x00);
	if (ret) {
		LOG_ERR("Failed to write SysConfig1");
		return ret;
	}
	ret = pinnacle_write_spi(spi, PINNACLE_REG_FEED_CONFIG2, 0x1F);
	if (ret) {
		LOG_ERR("Failed to write FeedConfig2");
		return ret;
	}

	/* Enable feed */
	ret = pinnacle_write_spi(spi, PINNACLE_REG_FEED_CONFIG1, 0x03);
	if (ret) {
		LOG_ERR("Failed to enable Feed in FeedConfig1");
		return ret;
	}

	/* Configure count of Z-Idle packets */
	ret = pinnacle_write_spi(spi, PINNACLE_REG_Z_IDLE,
					config->idle_packets_count);
	if (ret) {
		LOG_ERR("Failed to disable Z-Idle packets");
		return ret;
	}

#ifdef CONFIG_PINNACLE_TRIGGER
	ret = pinnacle_init_interrupt(dev);
	if (ret) {
		LOG_ERR("Failed to initialize interrupts");
		return ret;
	}
#endif

	return 0;
}

static void pinnacle_decode_sample(struct pinnacle_data *data, uint8_t *rx)
{
	data->sample.x = ((rx[2] & 0x0F) << 8) | rx[0];
	data->sample.y = ((rx[2] & 0xF0) << 4) | rx[1];
	data->sample.z = rx[3] & 0x3F;
}

static bool pinnacle_is_idle_sample(const struct pinnacle_sample *sample)
{
	return (sample->x == 0 && sample->y == 0 && sample->z == 0);
}

static void pinnacle_clip_sample(const struct pinnacle_active_range *range,
				 struct pinnacle_sample *sample)
{
	if (sample->x < range->x_min) {
		sample->x = range->x_min;
	}
	if (sample->x > range->x_max) {
		sample->x = range->x_max;
	}
	if (sample->y < range->y_min) {
		sample->y = range->y_min;
	}
	if (sample->y > range->y_max) {
		sample->y = range->y_max;
	}
}

static void pinnacle_scale_sample(const struct pinnacle_active_range *range,
				  const struct pinnacle_resolution *res,
				  struct pinnacle_sample *sample)
{
	sample->x = (uint16_t)(
		(uint32_t)(sample->x - range->x_min) * res->x
		/ (range->x_max - range->x_min));
	sample->y = (uint16_t)(
		(uint32_t)(sample->y - range->y_min) * res->y
		/ (range->y_max - range->y_min));
}

static int pinnacle_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct pinnacle_data *drv_data = dev->data;
	const struct pinnacle_config *config = dev->config;
	const struct spi_dt_spec *spi = &config->spi;

	uint8_t rx[4];
	int ret1 = 0;
	int ret2 = 0;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_POS_X ||
			chan == SENSOR_CHAN_POS_Y ||
			chan == SENSOR_CHAN_POS_Z ||
			chan == SENSOR_CHAN_POS_XYZ);

	ret1 = pinnacle_seq_read_spi(spi, PINNACLE_REG_PACKET_BYTE2, rx, 4);
	if (ret1) {
		LOG_ERR("Failed to read data from SPI device %s",
			spi->bus->name);
	} else {
		pinnacle_decode_sample(drv_data, rx);
		if (!pinnacle_is_idle_sample(&drv_data->sample)) {
			if (config->clipping_enabled) {
				pinnacle_clip_sample(
					&config->active_range,
					&drv_data->sample);
				if (config->scaling_enabled) {
					pinnacle_scale_sample(
						&config->active_range,
						&config->resolution,
						&drv_data->sample);
				}
			}

		}
	}
	ret2 = pinnacle_write_spi(spi, PINNACLE_REG_STATUS1, 0x00);
	if (ret2) {
		LOG_ERR("Failed to clear SW_CC and SW_DR for SPI device %s",
			spi->bus->name);
		return ret2;
	}
	return 0;
}

static void pinnacle_channel_convert(struct sensor_value *val,
				     int16_t raw_val)
{
	val->val1 = raw_val;
	val->val2 = 0;
}

static int pinnacle_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct pinnacle_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_POS_X) {
		pinnacle_channel_convert(val, drv_data->sample.x);
	} else if (chan == SENSOR_CHAN_POS_Y) {
		pinnacle_channel_convert(val, drv_data->sample.y);
	} else if (chan == SENSOR_CHAN_POS_Z) {
		pinnacle_channel_convert(val, drv_data->sample.z);
	} else if (chan == SENSOR_CHAN_POS_XYZ) {
		pinnacle_channel_convert(val, drv_data->sample.x);
		pinnacle_channel_convert(val + 1, drv_data->sample.y);
		pinnacle_channel_convert(val + 2, drv_data->sample.z);
	}

	return 0;
}

static const struct sensor_driver_api pinnacle_driver_api = {
#ifdef CONFIG_PINNACLE_TRIGGER
	.trigger_set = pinnacle_trigger_set,
#endif
	.sample_fetch = pinnacle_sample_fetch,
	.channel_get = pinnacle_channel_get,
};

#if defined(CONFIG_PINNACLE_TRIGGER)
#define PINNACLE_INTERRUPT_CFG(inst) \
	.dr_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, data_ready_gpios, {}),
#else
#define PINNACLE_INTERRUPT_CFG(inst)
#endif

#define PINNACLE_CLIPPING_CFG(inst)					\
	.clipping_enabled = DT_INST_PROP(inst, clipping_enable),	\
	.active_range = {						\
		.x_min = DT_INST_PROP(inst, active_range_x_min),	\
		.x_max = DT_INST_PROP(inst, active_range_x_max),	\
		.y_min = DT_INST_PROP(inst, active_range_y_min),	\
		.y_max = DT_INST_PROP(inst, active_range_y_max),	\
	},

#define PINNACLE_SCALE_CFG(inst)				\
	.scaling_enabled = DT_INST_PROP(inst, scaling_enable),	\
	.resolution = {						\
		.x = DT_INST_PROP(inst, scaling_x_resolution),	\
		.y = DT_INST_PROP(inst, scaling_y_resolution),	\
	},

#define PINNACLE_DEFINE_CONFIG(inst)					\
	static const struct pinnacle_config pinnacle_config_##inst = {	\
		.spi = SPI_DT_SPEC_INST_GET(inst,			\
					    SPI_OP_MODE_MASTER		\
					    | SPI_MODE_CPHA		\
					    | SPI_WORD_SET(8)		\
					    | SPI_TRANSFER_MSB,		\
					    0U),			\
		PINNACLE_INTERRUPT_CFG(inst)				\
		.idle_packets_count = DT_INST_PROP(			\
			inst, idle_packets_count),			\
		PINNACLE_CLIPPING_CFG(inst)				\
	}

#define PINNACLE_DEFINE(inst)						\
	PINNACLE_DEFINE_CONFIG(inst);					\
	static struct pinnacle_data pinnacle_data_##inst;		\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, pinnacle_init, NULL,		\
			&pinnacle_data_##inst,				\
			&pinnacle_config_##inst,			\
			POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	\
			&pinnacle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PINNACLE_DEFINE)
