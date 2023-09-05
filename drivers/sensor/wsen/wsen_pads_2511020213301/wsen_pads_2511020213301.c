/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_pads_2511020213301

#include <string.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "wsen_pads_2511020213301.h"

LOG_MODULE_REGISTER(WSEN_PADS_2511020213301, CONFIG_SENSOR_LOG_LEVEL);

/*
 * List of supported output data rates. Index into this list is used as
 * argument for PADS_setOutputDataRate()
 */
static const int32_t pads_2511020213301_odr_list[] = {
	0, 1, 10, 25, 50, 75, 100, 200,
};

#define SAMPLES_TO_DISCARD (uint8_t)2

#define MAX_POLL_STEP_COUNT 10

static int pads_2511020213301_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	struct pads_2511020213301_data *data = dev->data;
	const struct pads_2511020213301_config *cfg = dev->config;

	switch (channel) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_AMBIENT_TEMP:
	case SENSOR_CHAN_PRESS:
		break;
	default:
		LOG_ERR("Fetching is not supported on channel %d.", channel);
		return -ENOTSUP;
	}

	if (data->sensor_odr == PADS_outputDataRatePowerDown) {
		if (PADS_enableOneShot(&data->sensor_interface, PADS_enable) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "pressure");
			return -EIO;
		}

		switch (cfg->configuration) {
		case PADS_lowPower:
			k_sleep(K_USEC(4700));
			break;
		case PADS_lowNoise:
			k_sleep(K_USEC(13200));
			break;
		default:
			LOG_ERR("Invalid sensor configuration");
			return -EIO;
		}

		PADS_state_t one_shot_state;

		do {
			if (PADS_isOneShotEnabled(&data->sensor_interface, &one_shot_state) !=
			    WE_SUCCESS) {
				LOG_ERR("Failed to check for data ready");
				return -EIO;
			}
		} while (PADS_enable == one_shot_state);
	} else {

		bool data_ready = false;
		int step_count = 0;
		uint32_t step_sleep_duration =
			((uint32_t)1000000000 /
			 (pads_2511020213301_odr_list[data->sensor_odr] * 1000)) /
			MAX_POLL_STEP_COUNT;

		while (1) {
			PADS_state_t pressure_state, temp_state;

			pressure_state = temp_state = PADS_disable;

			if (PADS_isDataAvailable(&data->sensor_interface, &temp_state,
						 &pressure_state) != WE_SUCCESS) {
				LOG_ERR("Failed to check for data available");
				return -EIO;
			}

			switch (channel) {
			case SENSOR_CHAN_ALL:
				data_ready = (pressure_state == PADS_enable &&
					      temp_state == PADS_enable);
				break;
			case SENSOR_CHAN_AMBIENT_TEMP:
				data_ready = (temp_state == PADS_enable);
				break;
			case SENSOR_CHAN_PRESS:
				data_ready = (pressure_state == PADS_enable);
				break;
			default:
				break;
			}

			if (data_ready) {
				break;
			} else if (step_count >= MAX_POLL_STEP_COUNT) {
				return -EIO;
			}

			step_count++;
			k_sleep(K_USEC(step_sleep_duration));
		}
	}

	switch (channel) {
	case SENSOR_CHAN_ALL: {

		if (PADS_getPressure_int(&data->sensor_interface, &data->pressure) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "pressure");
			return -EIO;
		}

		if (PADS_getTemperature_int(&data->sensor_interface, &data->temperature) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "temperature");
			return -EIO;
		}

		break;
	}
	case SENSOR_CHAN_AMBIENT_TEMP: {

		if (PADS_getTemperature_int(&data->sensor_interface, &data->temperature) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "temperature");
			return -EIO;
		}

		break;
	}
	case SENSOR_CHAN_PRESS: {

		if (PADS_getPressure_int(&data->sensor_interface, &data->pressure) != WE_SUCCESS) {
			LOG_ERR("Failed to fetch %s sample.", "pressure");
			return -EIO;
		}

		break;
	}
	default:
		break;
	}

	return 0;
}

static int pads_2511020213301_channel_get(const struct device *dev, enum sensor_channel channel,
					  struct sensor_value *value)
{
	struct pads_2511020213301_data *data = dev->data;

	switch (channel) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		/* Convert temperature from 0.01 degrees Celsius to degrees Celsius */
		value->val1 = data->temperature / 100;
		value->val2 = ((int32_t)data->temperature % 100) * (1000000 / 100);
		break;
	case SENSOR_CHAN_PRESS:
		/* Convert pressure from Pa to kPa */
		value->val1 = data->pressure / 1000;
		value->val2 = ((int32_t)data->pressure % 1000) * (1000000 / 1000);
		break;
	default:
		LOG_ERR("Channel not supported %d", channel);
		return -ENOTSUP;
	}

	return 0;
}

/* Set output data rate. See pads_2511020213301_odr_list for allowed values. */
static int pads_2511020213301_odr_set(const struct device *dev, const struct sensor_value *odr)
{
	struct pads_2511020213301_data *data = dev->data;
	const struct pads_2511020213301_config *cfg = dev->config;
	int odr_index;

	for (odr_index = 0; odr_index < ARRAY_SIZE(pads_2511020213301_odr_list); odr_index++) {
		if (odr->val1 == pads_2511020213301_odr_list[odr_index] && odr->val2 == 0) {
			break;
		}
	}

	if (odr_index == ARRAY_SIZE(pads_2511020213301_odr_list)) {
		/* ODR not allowed (was not found in pads_2511020213301_odr_list) */
		LOG_ERR("Bad sampling frequency %d.%d", odr->val1, odr->val2);
		return -EINVAL;
	}

	if (cfg->configuration == PADS_lowNoise &&
	    (PADS_outputDataRate_t)odr_index > PADS_outputDataRate75Hz) {
		LOG_ERR("Failed to set ODR > 75Hz is not possible with low noise sensor "
			"configuration.");
		return -EIO;
	}

	if (PADS_setOutputDataRate(&data->sensor_interface, (PADS_outputDataRate_t)odr_index) !=
	    WE_SUCCESS) {
		LOG_ERR("Failed to set output data rate");
		return -EIO;
	}

	if (PADS_enableBlockDataUpdate(&data->sensor_interface,
				       (PADS_outputDataRate_t)odr_index !=
						       PADS_outputDataRatePowerDown
					       ? PADS_enable
					       : PADS_disable) != WE_SUCCESS) {
		LOG_ERR("Failed to enable block data update.");
		return -EIO;
	}

	data->sensor_odr = (PADS_outputDataRate_t)odr_index;

	return 0;
}

/* Get output data rate. */
static int pads_2511020213301_odr_get(const struct device *dev, struct sensor_value *odr)
{

	struct pads_2511020213301_data *data = dev->data;

	PADS_outputDataRate_t odr_index;

	if (PADS_getOutputDataRate(&data->sensor_interface, &odr_index) != WE_SUCCESS) {
		LOG_ERR("Failed to get output data rate");
		return -EIO;
	}

	data->sensor_odr = odr_index;

	odr->val1 = pads_2511020213301_odr_list[odr_index];
	odr->val2 = 0;

	return 0;
}

static int pads_2511020213301_attr_get(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, struct sensor_value *val)
{

	if (val == NULL) {
		LOG_WRN("address of passed value is NULL.");
		return -EFAULT;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (chan != SENSOR_CHAN_ALL) {
			LOG_ERR("attr_get() is not supported on channel %d.", chan);
			return -ENOTSUP;
		}
		return pads_2511020213301_odr_get(dev, val);
#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD
	case SENSOR_ATTR_WSEN_PADS_2511020213301_REFERENCE_POINT:
		if (chan != SENSOR_CHAN_PRESS) {
			LOG_ERR("attr_get() is not supported on channel %d.", chan);
			return -ENOTSUP;
		}
		return pads_2511020213301_reference_point_get(dev, val);
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_LOWER_THRESH:
		if (chan != SENSOR_CHAN_PRESS) {
			LOG_ERR("attr_get() is not supported on channel %d.", chan);
			return -ENOTSUP;
		}
		return pads_2511020213301_threshold_get(dev, val);
#endif
	default:
		LOG_ERR("Operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int pads_2511020213301_attr_set(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, const struct sensor_value *val)
{
	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (chan != SENSOR_CHAN_ALL) {
			LOG_ERR("attr_set() is not supported on channel %d.", chan);
			return -ENOTSUP;
		}
		return pads_2511020213301_odr_set(dev, val);
#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD
	case SENSOR_ATTR_WSEN_PADS_2511020213301_REFERENCE_POINT:
		if (chan != SENSOR_CHAN_PRESS) {
			LOG_ERR("attr_set() is not supported on channel %d.", chan);
			return -ENOTSUP;
		}
		return pads_2511020213301_reference_point_set(dev, val);
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_LOWER_THRESH:
		if (chan != SENSOR_CHAN_PRESS) {
			LOG_ERR("attr_set() is not supported on channel %d.", chan);
			return -ENOTSUP;
		}
		return pads_2511020213301_threshold_set(dev, val);
#endif
	default:
		LOG_ERR("Operation not supported.");
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, pads_2511020213301_driver_api) = {
	.attr_set = pads_2511020213301_attr_set,
#if CONFIG_WSEN_PADS_2511020213301_TRIGGER
	.trigger_set = pads_2511020213301_trigger_set,
#endif
	.attr_get = pads_2511020213301_attr_get,
	.sample_fetch = pads_2511020213301_sample_fetch,
	.channel_get = pads_2511020213301_channel_get,
};

static int pads_2511020213301_init(const struct device *dev)
{
	const struct pads_2511020213301_config *config = dev->config;
	struct pads_2511020213301_data *data = dev->data;
	struct sensor_value odr;
	uint8_t device_id;

	/* Initialize WE sensor interface */
	WE_sensorInterfaceType_t interface_type = data->sensor_interface.interfaceType;

	PADS_getDefaultInterface(&data->sensor_interface);
	data->sensor_interface.interfaceType = interface_type;

	switch (data->sensor_interface.interfaceType) {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	case WE_i2c:
		if (!i2c_is_ready_dt(&config->bus_cfg.i2c)) {
			LOG_ERR("I2C bus device not ready");
			return -ENODEV;
		}
		data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;
		break;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	case WE_spi:
		if (!spi_is_ready_dt(&config->bus_cfg.spi)) {
			LOG_ERR("SPI bus device not ready");
			return -ENODEV;
		}
		data->sensor_interface.handle = (void *)&config->bus_cfg.spi;
		break;
#endif
	default:
		LOG_ERR("Invalid interface type");
		return -EINVAL;
	}

	/* needed after power up */
	k_sleep(K_USEC(4500));

	PADS_state_t boot_state = PADS_enable;

	do {
		if (PADS_getBootStatus(&data->sensor_interface, &boot_state) != WE_SUCCESS) {
			LOG_ERR("Failed to get sensor reset state.");
			return -EIO;
		}
	} while (PADS_enable == boot_state);

	/* First communication test - check device ID */
	if (PADS_getDeviceID(&data->sensor_interface, &device_id) != WE_SUCCESS) {
		LOG_ERR("Failed to read device ID.");
		return -EIO;
	}

	if (device_id != PADS_DEVICE_ID_VALUE) {
		LOG_ERR("Invalid device ID 0x%x.", device_id);
		return -EINVAL;
	}

	/* Reset sensor */
	PADS_softReset(&data->sensor_interface, PADS_enable);
	k_sleep(K_USEC(50));

	PADS_state_t sw_reset;

	do {
		if (PADS_getSoftResetState(&data->sensor_interface, &sw_reset) != WE_SUCCESS) {
			LOG_ERR("Failed to get sensor reset state.");
			return -EIO;
		}
	} while (PADS_enable == sw_reset);

	if (PADS_setPowerMode(&data->sensor_interface, config->configuration) != WE_SUCCESS) {
		LOG_ERR("Failed to set sensor configuration.");
		return -EIO;
	}

	odr.val1 = pads_2511020213301_odr_list[config->odr];
	odr.val2 = 0;

	if (pads_2511020213301_odr_set(dev, &odr) < 0) {
		LOG_ERR("Failed to set output data rate.");
		return -EIO;
	}

	if (PADS_enableLowPassFilter(&data->sensor_interface, config->alpf) != WE_SUCCESS) {
		LOG_ERR("Failed to set additional low pass filter.");
		return -EIO;
	}

	if (config->alpf == PADS_enable) {
		if (PADS_setLowPassFilterConfig(&data->sensor_interface,
						config->alpf_configuration) != WE_SUCCESS) {
			LOG_ERR("Failed to set additional low pass filter configuration.");
			return -EIO;
		}

		for (uint8_t i = 0; i < SAMPLES_TO_DISCARD; i++) {
			pads_2511020213301_sample_fetch(dev, SENSOR_CHAN_ALL);
		}

		data->pressure = 0;
		data->temperature = 0;
	}

#if CONFIG_WSEN_PADS_2511020213301_TRIGGER
	if (pads_2511020213301_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "PADS driver enabled without any devices"
#endif

#ifdef CONFIG_WSEN_PADS_2511020213301_TRIGGER
#define PADS_2511020213301_CFG_IRQ(inst)                                                           \
	.interrupt_gpio = GPIO_DT_SPEC_INST_GET(inst, interrupt_gpios)
#else
#define PADS_2511020213301_CFG_IRQ(inst)
#endif /* CONFIG_WSEN_PADS_2511020213301_TRIGGER */

#define PADS_2511020213301_CFG_ALPF(inst)                                                          \
	.alpf = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, additional_low_pass_filter),            \
		(PADS_enable), (PADS_disable))

#define PADS_2511020213301_CONFIG_COMMON(inst)                                                     \
	.odr = (PADS_outputDataRate_t)(DT_INST_ENUM_IDX(inst, odr)),                               \
	.configuration = (PADS_powerMode_t)(DT_INST_ENUM_IDX(inst, configuration)),                \
	.alpf_configuration =                                                                      \
		(PADS_filterConf_t)DT_INST_PROP(inst, additional_low_pass_filter_configuration),   \
	PADS_2511020213301_CFG_ALPF(inst),                                                         \
	IF_ENABLED(CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD, \
	(.threshold = (uint16_t)DT_INST_PROP_OR(inst, threshold, 0),))                            \
			    COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, interrupt_gpios),		\
	(PADS_2511020213301_CFG_IRQ(inst)), ())

/*
 * Instantiation macros used when device is on SPI bus.
 */

#define PADS_2511020213301_SPI_OPERATION                                                           \
	(SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define PADS_2511020213301_CONFIG_SPI(inst)                                                        \
	{.bus_cfg =                                                                                \
		 {                                                                                 \
			 .spi = SPI_DT_SPEC_INST_GET(inst, PADS_2511020213301_SPI_OPERATION, 0),   \
		 },                                                                                \
	 PADS_2511020213301_CONFIG_COMMON(inst)}

/*
 * Instantiation macros used when device is on I2C bus.
 */

#define PADS_2511020213301_CONFIG_I2C(inst)                                                        \
	{.bus_cfg =                                                                                \
		 {                                                                                 \
			 .i2c = I2C_DT_SPEC_INST_GET(inst),                                        \
		 },                                                                                \
	 PADS_2511020213301_CONFIG_COMMON(inst)}

#define PADS_2511020213301_CONFIG_WE_INTERFACE(inst)                                               \
	{COND_CODE_1(DT_INST_ON_BUS(inst, i2c), \
		(.sensor_interface = {.interfaceType = WE_i2c}), \
		()) COND_CODE_1(DT_INST_ON_BUS(inst, spi),  \
		(.sensor_interface = {.interfaceType = WE_spi}), \
		()) }

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */
#define PADS_2511020213301_DEFINE(inst)                                                            \
	static struct pads_2511020213301_data pads_2511020213301_data_##inst =                     \
		PADS_2511020213301_CONFIG_WE_INTERFACE(inst);                                      \
	static const struct pads_2511020213301_config pads_2511020213301_config_##inst =           \
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c),\
		(PADS_2511020213301_CONFIG_I2C(inst)), \
		())                                            \
				COND_CODE_1(DT_INST_ON_BUS(inst, spi),  \
		(PADS_2511020213301_CONFIG_SPI(inst)),\
		());       \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, pads_2511020213301_init, NULL,                          \
				     &pads_2511020213301_data_##inst,                              \
				     &pads_2511020213301_config_##inst, POST_KERNEL,               \
				     CONFIG_SENSOR_INIT_PRIORITY, &pads_2511020213301_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PADS_2511020213301_DEFINE)
