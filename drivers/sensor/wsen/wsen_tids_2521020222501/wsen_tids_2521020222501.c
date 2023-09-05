/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_tids_2521020222501

#include <stdlib.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "wsen_tids_2521020222501.h"

LOG_MODULE_REGISTER(WSEN_TIDS_2521020222501, CONFIG_SENSOR_LOG_LEVEL);

static const struct sensor_value tids_2521020222501_odr_list[] = {{.val1 = 0, .val2 = 0},
								  {.val1 = 25, .val2 = 0},
								  {.val1 = 50, .val2 = 0},
								  {.val1 = 100, .val2 = 0},
								  {.val1 = 200, .val2 = 0}};

static int tids_2521020222501_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct tids_2521020222501_data *data = dev->data;
	int16_t raw_temperature;

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
		break;
	default:
		LOG_ERR("Fetching is not supported on channel %d.", chan);
		return -ENOTSUP;
	}
	if (data->sensor_odr == ((uint8_t)tids_2521020222501_odr_list[0].val1)) {

		TIDS_softReset(&data->sensor_interface, TIDS_enable);

		k_sleep(K_USEC(5));

		TIDS_softReset(&data->sensor_interface, TIDS_disable);

		if (TIDS_enableOneShot(&data->sensor_interface, TIDS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable one shot");
			return -EIO;
		}

		TIDS_state_t busy = TIDS_enable;

		do {
			if (TIDS_isBusy(&data->sensor_interface, &busy) != WE_SUCCESS) {
				LOG_ERR("Failed to check for data ready");
				return -EIO;
			}
		} while (TIDS_enable == busy);
	}

	if (TIDS_getRawTemperature(&data->sensor_interface, &raw_temperature) != WE_SUCCESS) {
		LOG_ERR("Failed to fetch data sample");
		return -EIO;
	}

	data->temperature = raw_temperature;

	return 0;
}

static int tids_2521020222501_channel_get(const struct device *dev, enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct tids_2521020222501_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Convert temperature from 0.01 degrees Celsius to degrees Celsius */
		val->val1 = data->temperature / 100;
		val->val2 = ((int32_t)data->temperature % 100) * (1000000 / 100);
		break;
	default:
		LOG_ERR("Channel not supported %d", chan);
		return -ENOTSUP;
	}

	return 0;
}

/* Set output data rate. See tids_2521020222501_odr_list for allowed values. */
static int tids_2521020222501_odr_set(const struct device *dev, const struct sensor_value *odr)
{
	struct tids_2521020222501_data *data = dev->data;

	int odr_index;

	for (odr_index = 0; odr_index < ARRAY_SIZE(tids_2521020222501_odr_list); odr_index++) {
		if (odr->val1 == tids_2521020222501_odr_list[odr_index].val1 &&
		    odr->val2 == tids_2521020222501_odr_list[odr_index].val2) {
			break;
		}
	}

	if (odr_index == ARRAY_SIZE(tids_2521020222501_odr_list)) {
		/* ODR not allowed (was not found in tids_2521020222501_odr_list) */
		LOG_ERR("Bad sampling frequency %d.%d", odr->val1, odr->val2);
		return -EINVAL;
	}

	if (odr->val1 == tids_2521020222501_odr_list[0].val1) {
		if (TIDS_enableBlockDataUpdate(&data->sensor_interface, TIDS_disable) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to enable block data update.");
			return -EIO;
		}

		if (TIDS_enableContinuousMode(&data->sensor_interface, TIDS_disable) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to enable continuous mode.");
			return -EIO;
		}
	} else {
		if (TIDS_setOutputDataRate(&data->sensor_interface,
					   (TIDS_outputDataRate_t)(odr->val1 - 1)) != WE_SUCCESS) {
			LOG_ERR("Failed to set output data rate");
			return -EIO;
		}

		if (TIDS_enableBlockDataUpdate(&data->sensor_interface, TIDS_enable) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to enable block data update.");
			return -EIO;
		}

		if (TIDS_enableContinuousMode(&data->sensor_interface, TIDS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to enable continuous mode.");
			return -EIO;
		}
	}

	data->sensor_odr = (uint8_t)odr->val1;

	return 0;
}

/* Get output data rate. */
static int tids_2521020222501_odr_get(const struct device *dev, struct sensor_value *odr)
{

	struct tids_2521020222501_data *data = dev->data;

	TIDS_state_t continuous_mode_state;

	if (TIDS_isContinuousModeEnabled(&data->sensor_interface, &continuous_mode_state) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to get continuous mode.");
		return -EIO;
	}

	if (continuous_mode_state == TIDS_disable) {
		odr->val1 = tids_2521020222501_odr_list[0].val1;
	} else {
		TIDS_outputDataRate_t odrIndex;

		if (TIDS_getOutputDataRate(&data->sensor_interface, &odrIndex) != WE_SUCCESS) {
			LOG_ERR("Failed to get output data rate");
			return -EIO;
		}

		odr->val1 = tids_2521020222501_odr_list[odrIndex + 1].val1;
	}

	data->sensor_odr = (uint8_t)odr->val1;

	odr->val2 = 0;

	return 0;
}

static int tids_2521020222501_attr_set(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, const struct sensor_value *val)
{

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
		break;
	default:
		LOG_ERR("attr_set() is not supported on channel %d.", chan);
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return tids_2521020222501_odr_set(dev, val);
#ifdef CONFIG_WSEN_TIDS_2521020222501_TRIGGER
	case SENSOR_ATTR_LOWER_THRESH:
		return tids_2521020222501_threshold_lower_set(dev, val);
	case SENSOR_ATTR_UPPER_THRESH:
		return tids_2521020222501_threshold_upper_set(dev, val);
#endif /* CONFIG_WSEN_TIDS_2521020222501_TRIGGER */

	default:
		LOG_ERR("Operation not supported.");
		return -ENOTSUP;
	}
}

static int tids_2521020222501_attr_get(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, struct sensor_value *val)
{

	if (val == NULL) {
		LOG_WRN("address of passed value is NULL.");
		return -EFAULT;
	}

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
		break;
	default:
		LOG_ERR("attr_get() is not supported on channel %d.", chan);
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return tids_2521020222501_odr_get(dev, val);
#ifdef CONFIG_WSEN_TIDS_2521020222501_TRIGGER
	case SENSOR_ATTR_LOWER_THRESH:
		return tids_2521020222501_threshold_lower_get(dev, val);
	case SENSOR_ATTR_UPPER_THRESH:
		return tids_2521020222501_threshold_upper_get(dev, val);
#endif /* CONFIG_WSEN_TIDS_2521020222501_TRIGGER */
	default:
		LOG_ERR("Operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, tids_2521020222501_driver_api) = {
	.attr_set = tids_2521020222501_attr_set,
#if CONFIG_WSEN_TIDS_2521020222501_TRIGGER
	.trigger_set = tids_2521020222501_trigger_set,
#endif
	.attr_get = tids_2521020222501_attr_get,
	.sample_fetch = tids_2521020222501_sample_fetch,
	.channel_get = tids_2521020222501_channel_get,
};

static int tids_2521020222501_init(const struct device *dev)
{
	const struct tids_2521020222501_config *const config = dev->config;
	struct tids_2521020222501_data *data = dev->data;
	uint8_t device_id;

	/* Initialize WE sensor interface */
	TIDS_getDefaultInterface(&data->sensor_interface);
	data->sensor_interface.interfaceType = WE_i2c;
	if (!i2c_is_ready_dt(&config->bus_cfg.i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}
	data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;

	/* Needed after power up */
	k_sleep(K_MSEC(12));

	/* First communication test - check device ID */
	if (TIDS_getDeviceID(&data->sensor_interface, &device_id) != WE_SUCCESS) {
		LOG_ERR("Failed to read device ID.");
		return -EIO;
	}

	if (device_id != TIDS_DEVICE_ID_VALUE) {
		LOG_ERR("Invalid device ID 0x%x.", device_id);
		return -EIO;
	}

	/* Reset the sensor with an arbitrary off time of 5 us */
	TIDS_softReset(&data->sensor_interface, TIDS_enable);
	k_sleep(K_USEC(5));
	TIDS_softReset(&data->sensor_interface, TIDS_disable);

	if (tids_2521020222501_odr_set(dev, &tids_2521020222501_odr_list[config->odr]) < 0) {
		LOG_ERR("Failed to set output data rate.");
		return -EIO;
	}

#ifdef CONFIG_WSEN_TIDS_2521020222501_TRIGGER
	if (tids_2521020222501_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

#ifdef CONFIG_WSEN_TIDS_2521020222501_TRIGGER
#define TIDS_2521020222501_CFG_IRQ(inst)                                                           \
	.interrupt_gpio = GPIO_DT_SPEC_INST_GET(inst, interrupt_gpios),                            \
	.high_threshold = DT_INST_PROP(inst, temp_high_threshold),                                 \
	.low_threshold = DT_INST_PROP(inst, temp_low_threshold)
#else
#define TIDS_2521020222501_CFG_IRQ(inst)
#endif /* CONFIG_WSEN_TIDS_2521020222501_TRIGGER */

/*
 * Main instantiation macro.
 */
#define TIDS_2521020222501_DEFINE(inst)                                                            \
	static struct tids_2521020222501_data tids_2521020222501_data_##inst;                      \
	static const struct tids_2521020222501_config tids_2521020222501_config_##inst = {         \
		.bus_cfg =                                                                         \
			{                                                                          \
				.i2c = I2C_DT_SPEC_INST_GET(inst),                                 \
			},                                                                         \
		.odr = (uint8_t)(DT_INST_ENUM_IDX(inst, odr)),                                     \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, interrupt_gpios),                          \
			    (TIDS_2521020222501_CFG_IRQ(inst)), ())};         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tids_2521020222501_init, NULL,                          \
				     &tids_2521020222501_data_##inst,                              \
				     &tids_2521020222501_config_##inst, POST_KERNEL,               \
				     CONFIG_SENSOR_INIT_PRIORITY, &tids_2521020222501_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TIDS_2521020222501_DEFINE)
