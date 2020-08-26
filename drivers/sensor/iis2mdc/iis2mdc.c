/* ST Microelectronics IIS2MDC 3-axis magnetometer sensor
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2mdc.pdf
 */

#define DT_DRV_COMPAT st_iis2mdc

#include <init.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <drivers/sensor.h>
#include <string.h>
#include <logging/log.h>
#include "iis2mdc.h"

struct iis2mdc_data iis2mdc_data;

LOG_MODULE_REGISTER(IIS2MDC, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_IIS2MDC_MAG_ODR_RUNTIME
static int iis2mdc_set_odr(struct device *dev, const struct sensor_value *val)
{
	struct iis2mdc_data *iis2mdc = dev->data;
	iis2mdc_odr_t odr;

	switch (val->val1) {
	case 10:
		odr = IIS2MDC_ODR_10Hz;
		break;
	case 20:
		odr = IIS2MDC_ODR_20Hz;
		break;
	case 50:
		odr = IIS2MDC_ODR_50Hz;
		break;
	case 100:
		odr = IIS2MDC_ODR_100Hz;
		break;
	default:
		return -EINVAL;
	}

	if (iis2mdc_data_rate_set(iis2mdc->ctx, odr)) {
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_IIS2MDC_MAG_ODR_RUNTIME */

static int iis2mdc_set_hard_iron(struct device *dev, enum sensor_channel chan,
				   const struct sensor_value *val)
{
	struct iis2mdc_data *iis2mdc = dev->data;
	uint8_t i;
	union axis3bit16_t offset;

	for (i = 0U; i < 3; i++) {
		offset.i16bit[i] = sys_cpu_to_le16(val->val1);
		val++;
	}

	return iis2mdc_mag_user_offset_set(iis2mdc->ctx, offset.u8bit);
}

static void iis2mdc_channel_get_mag(struct device *dev,
				      enum sensor_channel chan,
				      struct sensor_value *val)
{
	int32_t cval;
	int i;
	uint8_t ofs_start, ofs_stop;
	struct iis2mdc_data *iis2mdc = dev->data;
	struct sensor_value *pval = val;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		ofs_start = ofs_stop = 0U;
		break;
	case SENSOR_CHAN_MAGN_Y:
		ofs_start = ofs_stop = 1U;
		break;
	case SENSOR_CHAN_MAGN_Z:
		ofs_start = ofs_stop = 2U;
		break;
	default:
		ofs_start = 0U; ofs_stop = 2U;
		break;
	}

	for (i = ofs_start; i <= ofs_stop; i++) {
		cval = iis2mdc->mag[i] * 1500;
		pval->val1 = cval / 1000000;
		pval->val2 = cval % 1000000;
		pval++;
	}
}

/* read internal temperature */
static void iis2mdc_channel_get_temp(struct device *dev,
				       struct sensor_value *val)
{
	struct iis2mdc_data *drv_data = dev->data;

	val->val1 = drv_data->temp_sample / 100;
	val->val2 = (drv_data->temp_sample % 100) * 10000;
}

static int iis2mdc_channel_get(struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		iis2mdc_channel_get_mag(dev, chan, val);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		iis2mdc_channel_get_temp(dev, val);
		break;
	default:
		LOG_DBG("Channel not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int iis2mdc_config(struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
#ifdef CONFIG_IIS2MDC_MAG_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return iis2mdc_set_odr(dev, val);
#endif /* CONFIG_IIS2MDC_MAG_ODR_RUNTIME */
	case SENSOR_ATTR_OFFSET:
		return iis2mdc_set_hard_iron(dev, chan, val);
	default:
		LOG_DBG("Mag attribute not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int iis2mdc_attr_set(struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		return iis2mdc_config(dev, chan, attr, val);
	default:
		LOG_DBG("attr_set() not supported on %d channel", chan);
		return -ENOTSUP;
	}

	return 0;
}

static int iis2mdc_sample_fetch_mag(struct device *dev)
{
	struct iis2mdc_data *iis2mdc = dev->data;
	union axis3bit16_t raw_mag;

	/* fetch raw data sample */
	if (iis2mdc_magnetic_raw_get(iis2mdc->ctx, raw_mag.u8bit) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	iis2mdc->mag[0] = sys_le16_to_cpu(raw_mag.i16bit[0]);
	iis2mdc->mag[1] = sys_le16_to_cpu(raw_mag.i16bit[1]);
	iis2mdc->mag[2] = sys_le16_to_cpu(raw_mag.i16bit[2]);

	return 0;
}

static int iis2mdc_sample_fetch_temp(struct device *dev)
{
	struct iis2mdc_data *iis2mdc = dev->data;
	union axis1bit16_t raw_temp;
	int32_t temp;

	/* fetch raw temperature sample */
	if (iis2mdc_temperature_raw_get(iis2mdc->ctx, raw_temp.u8bit) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	/* formula is temp = 25 + (temp / 8) C */
	temp = (sys_le16_to_cpu(raw_temp.i16bit) & 0x8FFF);
	iis2mdc->temp_sample = 2500 + (temp * 100) / 8;

	return 0;
}

static int iis2mdc_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		iis2mdc_sample_fetch_mag(dev);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		iis2mdc_sample_fetch_temp(dev);
		break;
	case SENSOR_CHAN_ALL:
		iis2mdc_sample_fetch_mag(dev);
		iis2mdc_sample_fetch_temp(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api iis2mdc_driver_api = {
	.attr_set = iis2mdc_attr_set,
#if CONFIG_IIS2MDC_TRIGGER
	.trigger_set = iis2mdc_trigger_set,
#endif
	.sample_fetch = iis2mdc_sample_fetch,
	.channel_get = iis2mdc_channel_get,
};

static int iis2mdc_init_interface(struct device *dev)
{
	const struct iis2mdc_config *const config = dev->config;
	struct iis2mdc_data *iis2mdc = dev->data;

	iis2mdc->bus = device_get_binding(config->master_dev_name);
	if (!iis2mdc->bus) {
		LOG_DBG("Could not get pointer to %s device",
			    config->master_dev_name);
		return -EINVAL;
	}

	return config->bus_init(dev);
}

static const struct iis2mdc_config iis2mdc_dev_config = {
	.master_dev_name = DT_INST_BUS_LABEL(0),
#ifdef CONFIG_IIS2MDC_TRIGGER
	.drdy_port = DT_INST_GPIO_LABEL(0, drdy_gpios),
	.drdy_pin = DT_INST_GPIO_PIN(0, drdy_gpios),
	.drdy_flags = DT_INST_GPIO_FLAGS(0, drdy_gpios),
#endif  /* CONFIG_IIS2MDC_TRIGGER */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	.bus_init = iis2mdc_spi_init,
	.spi_conf.frequency = DT_INST_PROP(0, spi_max_frequency),
	.spi_conf.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
			       SPI_MODE_CPHA | SPI_WORD_SET(8) |
			       SPI_LINES_SINGLE),
	.spi_conf.slave     = DT_INST_REG_ADDR(0),
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	.gpio_cs_port	    = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
	.cs_gpio	    = DT_INST_SPI_DEV_CS_GPIOS_PIN(0),
	.cs_gpio_flags	    = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),

	.spi_conf.cs        =  &iis2mdc_data.cs_ctrl,
#else
	.spi_conf.cs        = NULL,
#endif
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	.bus_init = iis2mdc_i2c_init,
	.i2c_slv_addr = DT_INST_REG_ADDR(0),
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
};

static int iis2mdc_init(struct device *dev)
{
	struct iis2mdc_data *iis2mdc = dev->data;
	uint8_t wai;

	if (iis2mdc_init_interface(dev)) {
		return -EINVAL;
	}

	/* check chip ID */
	if (iis2mdc_device_id_get(iis2mdc->ctx, &wai) < 0) {
		return -EIO;
	}

	if (wai != IIS2MDC_ID) {
		LOG_DBG("Invalid chip ID: %02x\n", wai);
		return -EINVAL;
	}

	/* reset sensor configuration */
	if (iis2mdc_reset_set(iis2mdc->ctx, PROPERTY_ENABLE) < 0) {
		LOG_DBG("s/w reset failed\n");
		return -EIO;
	}

	k_busy_wait(100);

#if CONFIG_IIS2MDC_SPI_FULL_DUPLEX
	/* After s/w reset set SPI 4wires again if the case */
	if (iis2mdc_spi_mode_set(iis2mdc->ctx, IIS2MDC_SPI_4_WIRE) < 0) {
		return -EIO;
	}
#endif

	/* enable BDU */
	if (iis2mdc_block_data_update_set(iis2mdc->ctx, PROPERTY_ENABLE) < 0) {
		LOG_DBG("setting bdu failed\n");
		return -EIO;
	}

	/* Set Output Data Rate */
	if (iis2mdc_data_rate_set(iis2mdc->ctx, IIS2MDC_ODR_10Hz)) {
		LOG_DBG("set odr failed\n");
		return -EIO;
	}

	/* Set / Reset sensor mode */
	if (iis2mdc_set_rst_mode_set(iis2mdc->ctx,
				     IIS2MDC_SENS_OFF_CANC_EVERY_ODR)) {
		LOG_DBG("reset sensor mode failed\n");
		return -EIO;
	}

	/* Enable temperature compensation */
	if (iis2mdc_offset_temp_comp_set(iis2mdc->ctx, PROPERTY_ENABLE)) {
		LOG_DBG("enable temp compensation failed\n");
		return -EIO;
	}

	/* Set device in continuous mode */
	if (iis2mdc_operating_mode_set(iis2mdc->ctx, IIS2MDC_CONTINUOUS_MODE)) {
		LOG_DBG("set continuos mode failed\n");
		return -EIO;
	}

#ifdef CONFIG_IIS2MDC_TRIGGER
	if (iis2mdc_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupts");
		return -EIO;
	}
#endif

	return 0;
}

DEVICE_AND_API_INIT(iis2mdc, DT_INST_LABEL(0), iis2mdc_init,
		     &iis2mdc_data, &iis2mdc_dev_config, POST_KERNEL,
		     CONFIG_SENSOR_INIT_PRIORITY, &iis2mdc_driver_api);
