/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_tids

#include <stdlib.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "tids.h"

LOG_MODULE_REGISTER(TIDS, CONFIG_SENSOR_LOG_LEVEL);

/*
 * List of supported output data rates. Index into this list is used as
 * argument for TIDS_setOutputDataRate()
 */
static const int32_t tids_odr_list[] = { 25, 50, 100, 200 };

static int tids_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct tids_data *data = dev->data;
	int16_t raw_temperature;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (WE_SUCCESS != TIDS_getRawTemperature(&data->sensor_interface, &raw_temperature)) {
		LOG_ERR("Failed to fetch data sample");
		return -EIO;
	}

	data->temperature = raw_temperature;

	return 0;
}

static int tids_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	struct tids_data *data = dev->data;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		/* Convert temperature from 0.01 degrees Celsius to degrees Celsius */
		val->val1 = data->temperature / 100;
		val->val2 = ((int32_t)data->temperature % 100) * (1000000 / 100);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

/* Set output data rate. See tids_odr_list for allowed values. */
static int tids_odr_set(const struct device *dev, const struct sensor_value *odr)
{
	struct tids_data *data = dev->data;
	int odr_index;

	for (odr_index = 0; odr_index < ARRAY_SIZE(tids_odr_list); odr_index++) {
		if (odr->val1 == tids_odr_list[odr_index] && odr->val2 == 0) {
			break;
		}
	}

	if (odr_index == ARRAY_SIZE(tids_odr_list)) {
		/* ODR not allowed (was not found in tids_odr_list) */
		LOG_ERR("Bad sampling frequency %d.%d", odr->val1, abs(odr->val2));
		return -EINVAL;
	}

	if (WE_SUCCESS !=
	    TIDS_setOutputDataRate(&data->sensor_interface, (TIDS_outputDataRate_t)odr_index)) {
		LOG_ERR("Failed to set output data rate");
		return -EIO;
	}

	return 0;
}

static int tids_attr_set(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return tids_odr_set(dev, val);

#ifdef CONFIG_TIDS_TRIGGER
	case SENSOR_ATTR_LOWER_THRESH:
		return tids_threshold_set(dev, val, false);

	case SENSOR_ATTR_UPPER_THRESH:
		return tids_threshold_set(dev, val, true);
#endif /* CONFIG_TIDS_TRIGGER */

	default:
		LOG_ERR("Operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api tids_driver_api = { .attr_set = tids_attr_set,
#if CONFIG_TIDS_TRIGGER
							  .trigger_set = tids_trigger_set,
#endif
							  .sample_fetch = tids_sample_fetch,
							  .channel_get = tids_channel_get };

static int tids_init(const struct device *dev)
{
	const struct tids_config *const config = dev->config;
	struct tids_data *data = dev->data;
	int status;
	uint8_t device_id;
	struct sensor_value odr;

	/* Initialize WE sensor interface */
	TIDS_getDefaultInterface(&data->sensor_interface);
	data->sensor_interface.interfaceType = WE_i2c;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;
#endif

	/* First communication test - check device ID */
	if (WE_SUCCESS != TIDS_getDeviceID(&data->sensor_interface, &device_id)) {
		LOG_ERR("Failed to read device ID.");
		return -EIO;
	}

	if (device_id != TIDS_DEVICE_ID_VALUE) {
		LOG_ERR("Invalid device ID 0x%x.", device_id);
		return -EIO;
	}

	/* Reset sensor */
	TIDS_softReset(&data->sensor_interface, TIDS_enable);
	k_sleep(K_USEC(5));
	TIDS_softReset(&data->sensor_interface, TIDS_disable);

	odr.val1 = CONFIG_TIDS_ODR;
	odr.val2 = 0;
	status = tids_odr_set(dev, &odr);
	if (status < 0) {
		LOG_ERR("Failed to set output data rate.");
		return status;
	}

	if (WE_SUCCESS != TIDS_enableBlockDataUpdate(&data->sensor_interface, TIDS_enable)) {
		LOG_ERR("Failed to enable block data update.");
		return -EIO;
	}

	if (WE_SUCCESS != TIDS_enableContinuousMode(&data->sensor_interface, TIDS_enable)) {
		LOG_ERR("Failed to enable continuous mode.");
		return -EIO;
	}

#ifdef CONFIG_TIDS_TRIGGER
	status = tids_init_interrupt(dev);
	if (status < 0) {
		LOG_ERR("Failed to initialize threshold interrupt.");
		return status;
	}
#endif

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "TIDS driver enabled without any devices"
#endif

/*
 * Device creation macros
 */

#define TIDS_DEVICE_INIT(inst)                                        \
	DEVICE_DT_INST_DEFINE(inst,                                   \
				tids_init,                            \
				NULL,                                 \
				&tids_data_##inst,                    \
				&tids_config_##inst,                  \
				POST_KERNEL,                          \
				CONFIG_SENSOR_INIT_PRIORITY,	      \
				&tids_driver_api);

#ifdef CONFIG_TIDS_TRIGGER
#define TIDS_CFG_IRQ(inst) .gpio_threshold = GPIO_DT_SPEC_INST_GET(inst, int_gpios)
#else
#define TIDS_CFG_IRQ(inst)
#endif /* CONFIG_TIDS_TRIGGER */

/*
 * Main instantiation macro.
 */
#define TIDS_DEFINE(inst)                                             \
	static struct tids_data tids_data_##inst;                     \
	static const struct tids_config tids_config_##inst =          \
		{                                                     \
		.bus_cfg = {                                          \
			.i2c = I2C_DT_SPEC_INST_GET(inst),            \
		},                                                    \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int_gpios),   \
			(TIDS_CFG_IRQ(inst)), ())                     \
		};                                                    \
	TIDS_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(TIDS_DEFINE)