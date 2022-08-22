/*
 * Copyright (c) 2020 George Gkinis
 * Copyright (c) 2021 Jan Gnip
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT avia_hx711

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/hx711.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/zephyr.h>

struct hx711_data {
	struct gpio_callback dout_gpio_cb;
	struct k_sem dout_sem;

	const struct hx711_config *cfg;

	int32_t reading;

	int offset;
	struct sensor_value slope;
	struct sensor_value calibration_weight;
	int sample_fetch_timeout;
	char gain;
	enum hx711_rate rate;
};

struct hx711_config {
	int instance;

	struct gpio_dt_spec dout;
	struct gpio_dt_spec sck;
	struct gpio_dt_spec rate;
};

LOG_MODULE_REGISTER(HX711, CONFIG_SENSOR_LOG_LEVEL);

static void hx711_gpio_callback(const struct device *dev,
				struct gpio_callback *cb, uint32_t pins)
{
	struct hx711_data *data =
		CONTAINER_OF(cb, struct hx711_data, dout_gpio_cb);
	const struct hx711_config *cfg = data->cfg;

	gpio_pin_interrupt_configure_dt(&cfg->dout, GPIO_INT_DISABLE);

	/* Signal thread that data is now ready */
	k_sem_give(&data->dout_sem);
}

/**
 * @brief Send a pulse on the SCK pin.
 *
 * @param dev Pointer to the hx711 device structure
 *
 * @retval The value of the DOUT pin (HIGH or LOW)
 *
 */
static int hx711_cycle(const struct device *dev)
{
	const struct hx711_config *cfg = dev->config;

	/* SCK set HIGH */
	gpio_pin_set_dt(&cfg->sck, true);
	/* From the datasheet page 5 : PD_SCK high time = PD_SCK low time = 1μs (TYP) */
	k_busy_wait(1);

	/* SCK set LOW */
	gpio_pin_set_dt(&cfg->sck, false);
	/* From the datasheet page 5 : PD_SCK high time = PD_SCK low time = 1μs (TYP) */
	k_busy_wait(1);

	/* Return DOUT pin state */
	return gpio_pin_get_dt(&cfg->dout);
}

/**
 * @brief Read HX711 data. Also sets GAIN for the next cycle.
 *
 * @param dev Pointer to the hx711 device structure
 * @param chan Channel to fetch data for.
 *             Only SENSOR_CHANNEL_WEIGHT is available.
 *
 * @retval 0 on success,
 * @retval -EACCES error if module is not powered up.
 * @retval -EIO error if sample_fetch_timeout msec's elapsed with no data
 * available.
 *
 */
static int hx711_sample_fetch(const struct device *dev,
			      enum sensor_channel chan)
{

	int ret = 0;
	uint32_t count = 0;
	int i;

	struct hx711_data *data = dev->data;
	const struct hx711_config *cfg = dev->config;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	if (gpio_pin_get_dt(&cfg->dout) == 1) {
		k_sem_take(&data->dout_sem, K_NO_WAIT);

		if (gpio_pin_interrupt_configure_dt(
			    &cfg->dout, GPIO_INT_EDGE_TO_INACTIVE) < 0) {
			LOG_ERR("Failed to set dout GPIO interrupt");
		}

		/* Wait until reading is ready */
		if (k_sem_take(&data->dout_sem,
			       K_MSEC(data->sample_fetch_timeout))) {
			LOG_ERR("Weight data not ready within %d ms. "
				"Is the device properly connected?",
				data->sample_fetch_timeout);
			return -EIO;
		}
	}

	/* Clock data out.
	 * HX711 is a 24bit ADC.
	 * Clock out 24 bits.
	 */
	for (i = 0; i < 24; i++) {
		count = count << 1;
		if (hx711_cycle(dev)) {
			count++;
		}
	}

	/* set GAIN for next read */
	for (i = 0; i < data->gain; i++) {
		hx711_cycle(dev);
	}

	/* Add padding to 24bit value to make a 32bit one. */
	count ^= 0x800000;

	data->reading = count;

	LOG_DBG("Raw reading : %d", data->reading);

	return pm_device_runtime_put(dev);
}

/**
 * @brief Set HX711 gain.
 *
 * @param dev Pointer to the hx711 device structure
 * @param val sensor_value struct. Only val1 is used.
 *            valid values are :
 *                HX711_GAIN_128X (default),
 *                HX711_GAIN_32X,
 *                HX711_GAIN_64X,
 *
 * @retval 0 on success,
 * @retval -ENOTSUP error if an invalid GAIN is provided or
 *         -EACCES if sample_fetch_timeout msec's are elapsed with no data
 * available.
 *
 */
static int hx711_attr_set_gain(const struct device *dev,
			       const struct sensor_value *val)
{
	struct hx711_data *data = dev->data;

	switch (val->val1) {
	case HX711_GAIN_128X:
		data->gain = HX711_GAIN_128X;
		break;
	case HX711_GAIN_32X:
		data->gain = HX711_GAIN_32X;
		break;
	case HX711_GAIN_64X:
		data->gain = HX711_GAIN_64X;
		break;
	default:
		return -ENOTSUP;
	}
	/* Get a sample to set gain and channel for next measurement */
	return hx711_sample_fetch(dev, SENSOR_CHAN_WEIGHT);
}

/**
 * @brief Set HX711 rate.
 *
 * @param dev Pointer to the hx711 device structure
 * @param val sensor_value struct. Only val1 is used.
 *            valid values are :
 *               HX711_RATE_10HZ (default),
 *               HX711_RATE_80HZ
 *
 * @retval 0 on success,
 * @retval -EINVAL error if it fails to get RATE device.
 * @retval -ENOTSUP error if an invalid rate value is passed.
 *
 */
static int hx711_attr_set_rate(const struct device *dev,
			       const struct sensor_value *val)
{
	struct hx711_data *data = dev->data;
	const struct hx711_config *cfg = dev->config;
	int ret;

	if (cfg->rate.port == NULL) {
		LOG_ERR("Rate pin not configured for instance %d",
			cfg->instance);
		return -ENOTSUP;
	}
	switch (val->val1) {
	case HX711_RATE_10HZ:
	case HX711_RATE_80HZ:
		data->rate = val->val1;
		ret = gpio_pin_set_dt(&cfg->rate, data->rate);
		return ret;
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Get HX711 attributes.
 *
 * @param dev Pointer to the hx711 device structure
 * @param chan Channel to get.
 *             Supported channels :
 *               SENSOR_CHAN_WEIGHT
 *               SENSOR_CHAN_ALL
 * @param attr Attribute to get.
 *             Supported attributes :
 *               SENSOR_ATTR_SAMPLING_FREQUENCY
 *               SENSOR_ATTR_OFFSET
 *               SENSOR_ATTR_CALIBRATION
 *               SENSOR_ATTR_SLOPE
 *               SENSOR_ATTR_GAIN
 * @param val   Value to get.
 * @retval 0 on success
 * @retval -ENOTSUP if an invalid attribute is given
 *
 */
static int hx711_attr_get(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, struct sensor_value *val)
{
	int ret = 0;
	struct hx711_data *data = dev->data;

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		LOG_DBG("Attribute RATE is to %d", data->rate);
		val->val1 = data->rate;
		return ret;
	case SENSOR_ATTR_OFFSET:
		val->val1 = data->offset;
		LOG_DBG("Attribute OFFSET is to %d", data->offset);
		return ret;
	case SENSOR_ATTR_CALIBRATION:
		val->val1 = data->calibration_weight.val1;
		val->val2 = data->calibration_weight.val2;
		LOG_DBG("Attribute CALIBRATION is %d.%d",
			data->calibration_weight.val1,
			data->calibration_weight.val2);
		return ret;
	case SENSOR_ATTR_SLOPE:
		val->val1 = data->slope.val1;
		val->val2 = data->slope.val2;
		LOG_DBG("Attribute SLOPE is %d.%d", data->slope.val1,
			data->slope.val2);
		return ret;
	case SENSOR_ATTR_GAIN:
		val->val1 = (uint32_t)data->gain;
		LOG_DBG("Attribute GAIN is %d", data->gain);
		return ret;
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Set HX711 attributes.
 *
 * @param dev Pointer to the hx711 device structure
 * @param chan Channel to set.
 *             Supported channels :
 *               SENSOR_CHAN_WEIGHT
 *               SENSOR_CHAN_ALL
 * @param attr Attribute to change.
 *             Supported attributes :
 *               SENSOR_ATTR_SAMPLING_FREQUENCY
 *               SENSOR_ATTR_OFFSET
 *               SENSOR_ATTR_CALIBRATION
 *               SENSOR_ATTR_SLOPE
 *               SENSOR_ATTR_GAIN
 * @param val   Value to set.
 * @retval 0 on success
 * @retval -ENOTSUP if an invalid attribute is given
 *
 */
static int hx711_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr,
			  const struct sensor_value *val)
{
	int ret = 0;
	struct hx711_data *data = dev->data;

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = hx711_attr_set_rate(dev, val);
		if (ret == 0) {
			LOG_DBG("Attribute RATE set to %d", data->rate);
		}
		return ret;
	case SENSOR_ATTR_OFFSET:
		data->offset = val->val1;
		LOG_DBG("Attribute OFFSET set to %d", data->offset);
		return ret;
	case SENSOR_ATTR_CALIBRATION:
		data->calibration_weight = *val;
		LOG_DBG("Attribute CALIBRATION set to %d.%d",
			data->calibration_weight.val1,
			data->calibration_weight.val2);
		return ret;
	case SENSOR_ATTR_SLOPE:
		data->slope = *val;
		LOG_DBG("Attribute SLOPE set to %d.%d", data->slope.val1,
			data->slope.val2);
		return ret;
	case SENSOR_ATTR_GAIN:
		ret = hx711_attr_set_gain(dev, val);
		if (ret == 0) {
			LOG_DBG("Attribute GAIN set to %d", data->gain);
		}
		return ret;
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Get HX711 reading.
 *
 * @param dev Pointer to the hx711 device structure
 * @param chan Channel to set.
 *             Supported channels :
 *               SENSOR_CHAN_WEIGHT
 *               SENSOR_CHAN_ALL
 *
 * @param val  Value to write weight value to.
 *        Formula is :
 *          weight = slope * (reading - offset)
 *
 * @retval 0 on success
 * @retval  -ENOTSUP if an invalid channel is given
 *
 */
static int hx711_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct hx711_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_WEIGHT: {
		sensor_value_from_double(
			val, (sensor_value_to_double(&data->slope) *
			      (double)(data->reading - data->offset)));
		return 0;
	}
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Initialise HX711.
 *
 * @param dev Pointer to the hx711 device structure
 *
 * @retval 0 on success
 * @retval -EINVAL if an invalid argument is given
 *
 */
static int hx711_init(const struct device *dev)
{
	LOG_DBG("Initialising HX711");

	int ret = 0;
	struct hx711_data *data = dev->data;
	const struct hx711_config *cfg = dev->config;

	/* enable device runtime power management */
	ret = pm_device_runtime_enable(dev);
	if ((ret < 0) && (ret != -ENOSYS)) {
		return ret;
	}

	LOG_DBG("HX711 Instance: %d", cfg->instance);
	LOG_DBG("SCK GPIO port : %s", cfg->sck.port->name);
	LOG_DBG("SCK Pin : %d", cfg->sck.pin);
	LOG_DBG("DOUT GPIO port : %s", cfg->dout.port->name);
	LOG_DBG("DOUT Pin : %d", cfg->dout.pin);

	if (cfg->rate.port != NULL) {
		LOG_DBG("RATE GPIO port : %s", cfg->rate.port->name);
		LOG_DBG("RATE Pin : %d", cfg->rate.pin);
	}

	LOG_DBG("Gain : %d", data->gain);

	/* Configure SCK as output, LOW */
	LOG_DBG("SCK pin controller name is %s", cfg->sck.port->name);

	ret = gpio_pin_configure_dt(&cfg->sck,
				    GPIO_OUTPUT_INACTIVE | cfg->sck.dt_flags);
	if (ret != 0) {
		return ret;
	}

	if (cfg->rate.port != NULL) {
		/* Configure RATE as output, LOW */
		LOG_DBG("RATE pin controller name is %s", cfg->rate.port->name);
		ret = gpio_pin_configure_dt(
			&cfg->rate, GPIO_OUTPUT_INACTIVE | cfg->rate.dt_flags);
		if (ret != 0) {
			return ret;
		}

		ret = gpio_pin_set_dt(&cfg->rate, data->rate);
		if (ret != 0) {
			return ret;
		}
	}

	/* Configure DOUT as input */
	LOG_DBG("DOUT pin controller name is %s", cfg->dout.port->name);
	ret = gpio_pin_configure_dt(&cfg->dout,
				    GPIO_INPUT | cfg->dout.dt_flags);
	if (ret != 0) {
		return ret;
	}
	LOG_DBG("Set DOUT pin : %d", cfg->dout.pin);

	/* Pointer to cfg is needed within hx711_gpio_callback. */
	data->cfg = cfg;

	k_sem_init(&data->dout_sem, 1, 1);
	gpio_init_callback(&data->dout_gpio_cb, hx711_gpio_callback,
			   BIT(cfg->dout.pin));

	if (gpio_add_callback(cfg->dout.port, &data->dout_gpio_cb) < 0) {
		LOG_DBG("Failed to set GPIO callback");
		return -EIO;
	}

	/* Get a reading to set GAIN */
	ret = hx711_sample_fetch(dev, SENSOR_CHAN_WEIGHT);

	return ret;
}

/**
 * @brief Zero the HX711.
 *
 * @param dev Pointer to the hx711 device structure
 * @param readings Number of readings to get average offset.
 *        5~10 readings should be enough, although more are allowed.
 * @retval The offset value
 *
 */
int avia_hx711_tare(const struct device *dev, uint8_t readings)
{
	int32_t avg = 0;
	struct hx711_data *data = dev->data;

	if (readings == 0) {
		readings = 1;
	}

	for (int i = 0; i < readings; i++) {
		hx711_sample_fetch(dev, SENSOR_CHAN_WEIGHT);
		avg += data->reading;
	}
	LOG_DBG("Average before division : %d", avg);
	avg = avg / readings;
	LOG_DBG("Average after division : %d", avg);
	data->offset = avg;

	LOG_DBG("Offset set to %d", data->offset);

	return data->offset;
}

/**
 * @brief Callibrate the HX711.
 *
 * Given a target value of a known weight the slope gets calculated.
 * This is actually unit agnostic.
 * If the target weight is given in grams, lb, Kg or any other weight unit,
 * the slope will be calculated accordingly.
 * This function should be invoked before any readings are taken.
 *
 * @param dev Pointer to the hx711 device structure
 * @param readings Number of readings to take for calibration.
 *        5~10 readings should be enough, although more are allowed.
 * @retval The slope value
 *
 */
struct sensor_value avia_hx711_calibrate(const struct device *dev,
					 uint8_t readings)
{
	int32_t avg = 0;
	struct hx711_data *data = dev->data;

	LOG_DBG("Calibration weight : %d.%06d", data->calibration_weight.val1,
		data->calibration_weight.val2);
	if (readings == 0) {
		readings = 1;
	}

	for (int i = 0; i < readings; i++) {
		hx711_sample_fetch(dev, SENSOR_CHAN_WEIGHT);
		avg += data->reading;
	}

	LOG_DBG("Average before division : %d", avg);
	avg = avg / readings;

	LOG_DBG("Average after division : %d", avg);
	LOG_DBG("Target set to : %d.%06d", data->calibration_weight.val1,
		data->calibration_weight.val2);
	double slope = sensor_value_to_double(&data->calibration_weight) /
		       (double)(avg - data->offset);

	sensor_value_from_double(&data->slope, slope);

	LOG_DBG("Slope set to : %d.%06d", data->slope.val1, data->slope.val2);

	return data->slope;
}

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Set the Device Power Management State.
 *
 * @param dev - The device structure.
 * @param action - power management state
 * @retval 0 on success
 * @retval -ENOTSUP if an unsupported action is given
 *
 */
static int hx711_pm_action(const struct device *dev,
			   enum pm_device_action action)
{
	int ret;
	const struct hx711_config *cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = gpio_pin_set_dt(&cfg->sck, 0);
		if (ret < 0) {
			return ret;
		}
		/* Fetch a sample to set GAIN again.
		 * GAIN is set to 128 channel A after RESET
		 */
		LOG_DBG("Setting GAIN. Ignore the next measurement.");
		hx711_sample_fetch(dev, SENSOR_CHAN_WEIGHT);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_SUSPEND:
		return gpio_pin_set_dt(&cfg->sck, 1);
	default:
		return -ENOTSUP;
	}
	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static const struct sensor_driver_api hx711_api = {
	.sample_fetch = hx711_sample_fetch,
	.channel_get = hx711_channel_get,
	.attr_set = hx711_attr_set,
	.attr_get = hx711_attr_get,
};

#define HX711_INIT(index)                                                      \
	static struct hx711_data hx711_data_##index = {                        \
		.reading = 0,                                                  \
		.sample_fetch_timeout =                                        \
			DT_INST_PROP(index, sample_fetch_timeout_ms),          \
		.gain = DT_INST_PROP(index, gain),                             \
		.rate = DT_INST_PROP_OR(index, rate_hz, HX711_RATE_10HZ),      \
	};                                                                     \
	static const struct hx711_config hx711_config_##index = {              \
		.instance = index,                                             \
		.dout = GPIO_DT_SPEC_INST_GET(index, dout_gpios),              \
		.sck = GPIO_DT_SPEC_INST_GET(index, sck_gpios),                \
		.rate = GPIO_DT_SPEC_INST_GET_OR(index, rate_gpios, {})};      \
                                                                               \
	PM_DEVICE_DT_INST_DEFINE(index, hx711_pm_action);                      \
                                                                               \
	DEVICE_DT_INST_DEFINE(index, hx711_init, PM_DEVICE_DT_INST_GET(index), \
			      &hx711_data_##index, &hx711_config_##index,      \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,        \
			      &hx711_api);

DT_INST_FOREACH_STATUS_OKAY(HX711_INIT)
