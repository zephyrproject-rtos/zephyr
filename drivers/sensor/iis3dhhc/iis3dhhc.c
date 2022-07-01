/* ST Microelectronics IIS3DHHC accelerometer senor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dhhc.pdf
 */

#define DT_DRV_COMPAT st_iis3dhhc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "iis3dhhc.h"

LOG_MODULE_REGISTER(IIS3DHHC, CONFIG_SENSOR_LOG_LEVEL);

static int iis3dhhc_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct iis3dhhc_data *data = dev->data;
	int16_t raw_accel[3];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	iis3dhhc_acceleration_raw_get(data->ctx, raw_accel);
	data->acc[0] = sys_le16_to_cpu(raw_accel[0]);
	data->acc[1] = sys_le16_to_cpu(raw_accel[1]);
	data->acc[2] = sys_le16_to_cpu(raw_accel[2]);

	return 0;
}

static inline void iis3dhhc_convert(struct sensor_value *val,
					int16_t raw_val)
{
	int64_t micro_ms2;

	/* Convert to m/s^2 */
	micro_ms2 = ((iis3dhhc_from_lsb_to_mg(raw_val) * SENSOR_G) / 1000LL);
	val->val1 = micro_ms2 / 1000000LL;
	val->val2 = micro_ms2 % 1000000LL;
}

static inline void iis3dhhc_channel_get_acc(const struct device *dev,
					     enum sensor_channel chan,
					     struct sensor_value *val)
{
	int i;
	uint8_t ofs_start, ofs_stop;
	struct iis3dhhc_data *iis3dhhc = dev->data;
	struct sensor_value *pval = val;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		ofs_start = ofs_stop = 0U;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		ofs_start = ofs_stop = 1U;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		ofs_start = ofs_stop = 2U;
		break;
	default:
		ofs_start = 0U; ofs_stop = 2U;
		break;
	}

	for (i = ofs_start; i <= ofs_stop ; i++) {
		iis3dhhc_convert(pval++, iis3dhhc->acc[i]);
	}
}

static int iis3dhhc_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		iis3dhhc_channel_get_acc(dev, chan, val);
		return 0;
	default:
		LOG_DBG("Channel not supported");
		break;
	}

	return -ENOTSUP;
}

static int iis3dhhc_odr_set(const struct device *dev,
			    const struct sensor_value *val)
{
	struct iis3dhhc_data *data = dev->data;
	iis3dhhc_norm_mod_en_t en;

	switch (val->val1) {
	case 0:
		en = IIS3DHHC_POWER_DOWN;
		break;
	case 1000:
		en = IIS3DHHC_1kHz1;
		break;
	default:
		return -EIO;
	}

	if (iis3dhhc_data_rate_set(data->ctx, en)) {
		LOG_DBG("failed to set sampling rate");
		return -EIO;
	}

	return 0;
}

static int iis3dhhc_attr_set(const struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return iis3dhhc_odr_set(dev, val);
	default:
		LOG_DBG("operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api iis3dhhc_api_funcs = {
	.attr_set = iis3dhhc_attr_set,
	.sample_fetch = iis3dhhc_sample_fetch,
	.channel_get = iis3dhhc_channel_get,
#if CONFIG_IIS3DHHC_TRIGGER
	.trigger_set = iis3dhhc_trigger_set,
#endif
};

static int iis3dhhc_init_chip(const struct device *dev)
{
	struct iis3dhhc_data *data = dev->data;
	uint8_t chip_id, rst;

	if (iis3dhhc_device_id_get(data->ctx, &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
		return -EIO;
	}

	if (chip_id != IIS3DHHC_ID) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/*
	 *  Restore default configuration
	 */
	iis3dhhc_reset_set(data->ctx, PROPERTY_ENABLE);
	do {
		iis3dhhc_reset_get(data->ctx, &rst);
	} while (rst);

	/* Enable Block Data Update */
	iis3dhhc_block_data_update_set(data->ctx, PROPERTY_ENABLE);

	/* Set Output Data Rate */
#ifdef CONFIG_IIS3DHHC_NORM_MODE
	iis3dhhc_data_rate_set(data->ctx, 1);
#else
	iis3dhhc_data_rate_set(data->ctx, 0);
#endif

	/* Enable temperature compensation */
	iis3dhhc_offset_temp_comp_set(data->ctx, PROPERTY_ENABLE);

	return 0;
}

static int iis3dhhc_init(const struct device *dev)
{
	const struct iis3dhhc_config * const config = dev->config;

	if (!spi_is_ready(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	config->bus_init(dev);

	if (iis3dhhc_init_chip(dev) < 0) {
		LOG_DBG("Failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_IIS3DHHC_TRIGGER
	if (iis3dhhc_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

static struct iis3dhhc_data iis3dhhc_data;

static const struct iis3dhhc_config iis3dhhc_config = {
#ifdef CONFIG_IIS3DHHC_TRIGGER
#ifdef CONFIG_IIS3DHHC_DRDY_INT1
	.int_gpio = GPIO_DT_SPEC_INST_GET_BY_IDX(0, irq_gpios, 0),
#else
	.int_gpio = GPIO_DT_SPEC_INST_GET_BY_IDX(0, irq_gpios, 1),
#endif /* CONFIG_IIS3DHHC_DRDY_INT1 */
#endif /* CONFIG_IIS3DHHC_TRIGGER */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	.bus_init = iis3dhhc_spi_init,
	.spi = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
			       SPI_MODE_CPHA | SPI_WORD_SET(8), 0U),
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
};

DEVICE_DT_INST_DEFINE(0, iis3dhhc_init, NULL,
		    &iis3dhhc_data, &iis3dhhc_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &iis3dhhc_api_funcs);
