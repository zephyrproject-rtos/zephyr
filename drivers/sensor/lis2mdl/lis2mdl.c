/* ST Microelectronics LIS2MDL 3-axis magnetometer sensor
 *
 * Copyright (c) 2018-2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2mdl.pdf
 */

#define DT_DRV_COMPAT st_lis2mdl

#include <init.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <drivers/sensor.h>
#include <string.h>
#include <logging/log.h>
#include "lis2mdl.h"

struct lis2mdl_data lis2mdl_data;

LOG_MODULE_REGISTER(LIS2MDL, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_LIS2MDL_MAG_ODR_RUNTIME
static int lis2mdl_set_odr(const struct device *dev,
			   const struct sensor_value *val)
{
	struct lis2mdl_data *lis2mdl = dev->data;
	lis2mdl_odr_t odr;

	switch (val->val1) {
	case 10:
		odr = LIS2MDL_ODR_10Hz;
		break;
	case 20:
		odr = LIS2MDL_ODR_20Hz;
		break;
	case 50:
		odr = LIS2MDL_ODR_50Hz;
		break;
	case 100:
		odr = LIS2MDL_ODR_100Hz;
		break;
	default:
		return -EINVAL;
	}

	if (lis2mdl_data_rate_set(lis2mdl->ctx, odr)) {
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_LIS2MDL_MAG_ODR_RUNTIME */

static int lis2mdl_set_hard_iron(const struct device *dev,
				   enum sensor_channel chan,
				   const struct sensor_value *val)
{
	struct lis2mdl_data *lis2mdl = dev->data;
	uint8_t i;
	int16_t offset[3];

	for (i = 0U; i < 3; i++) {
		offset[i] = sys_cpu_to_le16(val->val1);
		val++;
	}

	return lis2mdl_mag_user_offset_set(lis2mdl->ctx, offset);
}

static void lis2mdl_channel_get_mag(const struct device *dev,
				      enum sensor_channel chan,
				      struct sensor_value *val)
{
	int32_t cval;
	int i;
	uint8_t ofs_start, ofs_stop;
	struct lis2mdl_data *lis2mdl = dev->data;
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
		cval = lis2mdl->mag[i] * 1500;
		pval->val1 = cval / 1000000;
		pval->val2 = cval % 1000000;
		pval++;
	}
}

/* read internal temperature */
static void lis2mdl_channel_get_temp(const struct device *dev,
				       struct sensor_value *val)
{
	struct lis2mdl_data *drv_data = dev->data;

	val->val1 = drv_data->temp_sample / 100;
	val->val2 = (drv_data->temp_sample % 100) * 10000;
}

static int lis2mdl_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		lis2mdl_channel_get_mag(dev, chan, val);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		lis2mdl_channel_get_temp(dev, val);
		break;
	default:
		LOG_DBG("Channel not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2mdl_config(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
#ifdef CONFIG_LIS2MDL_MAG_ODR_RUNTIME
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lis2mdl_set_odr(dev, val);
#endif /* CONFIG_LIS2MDL_MAG_ODR_RUNTIME */
	case SENSOR_ATTR_OFFSET:
		return lis2mdl_set_hard_iron(dev, chan, val);
	default:
		LOG_DBG("Mag attribute not supported");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2mdl_attr_set(const struct device *dev,
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
		return lis2mdl_config(dev, chan, attr, val);
	default:
		LOG_DBG("attr_set() not supported on %d channel", chan);
		return -ENOTSUP;
	}

	return 0;
}

static int lis2mdl_sample_fetch_mag(const struct device *dev)
{
	struct lis2mdl_data *lis2mdl = dev->data;
	int16_t raw_mag[3];

	/* fetch raw data sample */
	if (lis2mdl_magnetic_raw_get(lis2mdl->ctx, raw_mag) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	lis2mdl->mag[0] = sys_le16_to_cpu(raw_mag[0]);
	lis2mdl->mag[1] = sys_le16_to_cpu(raw_mag[1]);
	lis2mdl->mag[2] = sys_le16_to_cpu(raw_mag[2]);

	return 0;
}

static int lis2mdl_sample_fetch_temp(const struct device *dev)
{
	struct lis2mdl_data *lis2mdl = dev->data;
	int16_t raw_temp;
	int32_t temp;

	/* fetch raw temperature sample */
	if (lis2mdl_temperature_raw_get(lis2mdl->ctx, &raw_temp) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	/* formula is temp = 25 + (temp / 8) C */
	temp = (sys_le16_to_cpu(raw_temp) & 0x8FFF);
	lis2mdl->temp_sample = 2500 + (temp * 100) / 8;

	return 0;
}

static int lis2mdl_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		lis2mdl_sample_fetch_mag(dev);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		lis2mdl_sample_fetch_temp(dev);
		break;
	case SENSOR_CHAN_ALL:
		lis2mdl_sample_fetch_mag(dev);
		lis2mdl_sample_fetch_temp(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lis2mdl_driver_api = {
	.attr_set = lis2mdl_attr_set,
#if CONFIG_LIS2MDL_TRIGGER
	.trigger_set = lis2mdl_trigger_set,
#endif
	.sample_fetch = lis2mdl_sample_fetch,
	.channel_get = lis2mdl_channel_get,
};

static int lis2mdl_init_interface(const struct device *dev)
{
	const struct lis2mdl_config *const config = dev->config;
	struct lis2mdl_data *lis2mdl = dev->data;

	lis2mdl->bus = device_get_binding(config->master_dev_name);
	if (!lis2mdl->bus) {
		LOG_DBG("Could not get pointer to %s device",
			    config->master_dev_name);
		return -EINVAL;
	}

	return config->bus_init(dev);
}

static const struct lis2mdl_config lis2mdl_dev_config = {
	.master_dev_name = DT_INST_BUS_LABEL(0),
#ifdef CONFIG_LIS2MDL_TRIGGER
	.gpio_name = DT_INST_GPIO_LABEL(0, irq_gpios),
	.gpio_pin = DT_INST_GPIO_PIN(0, irq_gpios),
	.gpio_flags = DT_INST_GPIO_FLAGS(0, irq_gpios),
#endif  /* CONFIG_LIS2MDL_TRIGGER */
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	.bus_init = lis2mdl_spi_init,
	.spi_conf.frequency = DT_INST_PROP(0, spi_max_frequency),
	.spi_conf.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
			       SPI_MODE_CPHA | SPI_WORD_SET(8) |
			       SPI_LINES_SINGLE),
	.spi_conf.slave     = DT_INST_REG_ADDR(0),
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	.gpio_cs_port	    = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
	.cs_gpio	    = DT_INST_SPI_DEV_CS_GPIOS_PIN(0),
	.cs_gpio_flags	    = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0),

	.spi_conf.cs        =  &lis2mdl_data.cs_ctrl,
#else
	.spi_conf.cs        = NULL,
#endif
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	.bus_init = lis2mdl_i2c_init,
	.i2c_slv_addr = DT_INST_REG_ADDR(0),
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif
};

static int lis2mdl_init(const struct device *dev)
{
	struct lis2mdl_data *lis2mdl = dev->data;
	uint8_t wai;

	lis2mdl->dev = dev;

	if (lis2mdl_init_interface(dev)) {
		return -EINVAL;
	}

	/* check chip ID */
	if (lis2mdl_device_id_get(lis2mdl->ctx, &wai) < 0) {
		return -EIO;
	}

	if (wai != LIS2MDL_ID) {
		LOG_DBG("Invalid chip ID: %02x\n", wai);
		return -EINVAL;
	}

	/* reset sensor configuration */
	if (lis2mdl_reset_set(lis2mdl->ctx, PROPERTY_ENABLE) < 0) {
		LOG_DBG("s/w reset failed\n");
		return -EIO;
	}

	k_busy_wait(100);

#if CONFIG_LIS2MDL_SPI_FULL_DUPLEX
	/* After s/w reset set SPI 4wires again if the case */
	if (lis2mdl_spi_mode_set(lis2mdl->ctx, LIS2MDL_SPI_4_WIRE) < 0) {
		return -EIO;
	}
#endif

	/* enable BDU */
	if (lis2mdl_block_data_update_set(lis2mdl->ctx, PROPERTY_ENABLE) < 0) {
		LOG_DBG("setting bdu failed\n");
		return -EIO;
	}

	/* Set Output Data Rate */
	if (lis2mdl_data_rate_set(lis2mdl->ctx, LIS2MDL_ODR_10Hz)) {
		LOG_DBG("set odr failed\n");
		return -EIO;
	}

	/* Set / Reset sensor mode */
	if (lis2mdl_set_rst_mode_set(lis2mdl->ctx,
				     LIS2MDL_SENS_OFF_CANC_EVERY_ODR)) {
		LOG_DBG("reset sensor mode failed\n");
		return -EIO;
	}

	/* Enable temperature compensation */
	if (lis2mdl_offset_temp_comp_set(lis2mdl->ctx, PROPERTY_ENABLE)) {
		LOG_DBG("enable temp compensation failed\n");
		return -EIO;
	}

	/* Set device in continuous mode */
	if (lis2mdl_operating_mode_set(lis2mdl->ctx, LIS2MDL_CONTINUOUS_MODE)) {
		LOG_DBG("set continuos mode failed\n");
		return -EIO;
	}

#ifdef CONFIG_PM_DEVICE
	lis2mdl->power_state = DEVICE_PM_ACTIVE_STATE;
#endif

#ifdef CONFIG_LIS2MDL_TRIGGER
	if (lis2mdl_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupts");
		return -EIO;
	}
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int lis2mdl_set_power_state(struct lis2mdl_data *lis2mdl,
		uint32_t new_state)
{
	int status = 0;

	if (new_state == DEVICE_PM_ACTIVE_STATE) {
		status = lis2mdl_operating_mode_set(lis2mdl->ctx,
				LIS2MDL_CONTINUOUS_MODE);
		if (status) {
			LOG_ERR("Power up failed");
		}
		lis2mdl->power_state = DEVICE_PM_ACTIVE_STATE;
		LOG_DBG("State changed to active");
	} else {
		__ASSERT_NO_MSG(new_state == DEVICE_PM_LOW_POWER_STATE ||
				new_state == DEVICE_PM_SUSPEND_STATE ||
				new_state == DEVICE_PM_OFF_STATE);
		status = lis2mdl_operating_mode_set(lis2mdl->ctx,
				LIS2MDL_POWER_DOWN);
		if (status) {
			LOG_ERR("Power down failed");
		}
		lis2mdl->power_state = new_state;
		LOG_DBG("State changed to inactive");
	}

	return status;
}

static int lis2mdl_pm_control(const struct device *dev, uint32_t ctrl_command,
				void *context, device_pm_cb cb, void *arg)
{
	struct lis2mdl_data *lis2mdl = dev->data;
	uint32_t current_state = lis2mdl->power_state;
	int status = 0;
	uint32_t new_state;

	switch (ctrl_command) {
	case DEVICE_PM_SET_POWER_STATE:
		new_state = *((const uint32_t *)context);
		if (new_state != current_state) {
			status = lis2mdl_set_power_state(lis2mdl, new_state);
		}
		break;
	case DEVICE_PM_GET_POWER_STATE:
		*((uint32_t *)context) = current_state;
		break;
	default:
		LOG_ERR("Got unknown power management control command");
		status = -EINVAL;
	}

	if (cb) {
		cb(dev, status, context, arg);
	}

	return status;
}
#endif /* CONFIG_PM_DEVICE */

DEVICE_DT_INST_DEFINE(0, lis2mdl_init,
		lis2mdl_pm_control, &lis2mdl_data, &lis2mdl_dev_config,
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &lis2mdl_driver_api);
