/*
 * Copyright (c) 2020 George Gkinis
 * Copyright (c) 2021 Jan Gnip
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/hx7xx.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

/* Typical wait time 1us taken from the datasheet */
#define BUSY_WAIT_US (1u)

/* HX7xx ADC have 24bit resolution */
#define ADC_RESOLUTION (24u)

struct hx7xx_quirks {
	/**
	 * @brief Set an device gain
	 *
	 * @param dev Pointer to the device
	 * @param gain New gain value to set
	 *
	 * @return 0 if successful, negative errno code if failure.
	 */
	int (*set_gain)(const struct device *dev, enum hx7xx_gain gain);

	/**
	 * @brief Set an device sampling rate
	 *
	 * @param dev Pointer to the device
	 * @param gain New sampling rate value to set
	 *
	 * @return 0 if successful, negative errno code if failure.
	 */
	int (*set_sampling_rate)(const struct device *dev, enum hx7xx_rate rate);

	/**
	 * @brief Device-specific call to make device power-on
	 *
	 * @param dev Pointer to the device
	 *
	 * @return 0 if successful, negative errno code if failure.
	 */
	int (*power_on)(const struct device *dev);

	/**
	 * @brief Device-specific call to make device power-off
	 *
	 * @param dev Pointer to the device
	 *
	 * @return 0 if successful, negative errno code if failure.
	 */
	int (*power_off)(const struct device *dev);

	/* Flag indicating the driver should fetch new data after rate change. */
	bool fetch_after_rate_change;
};

struct hx7xx_data {
	struct gpio_callback dout_gpio_cb;
	struct k_sem data_out_sem;
	int32_t sample;
	int32_t offset;
	double slope;
	enum hx7xx_gain gain;
	enum hx7xx_rate rate;
	uint8_t extra_clock_num;
};

struct hx7xx_config {
	const struct hx7xx_quirks *quirks;
	uint32_t sample_fetch_timeout_ms;
	struct gpio_dt_spec data_out;
	struct gpio_dt_spec serial_clock;
#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx711) || DT_HAS_COMPAT_STATUS_OKAY(avia_hx717)
	struct gpio_dt_spec rate_sel;
#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx717)
	struct gpio_dt_spec rate_sel2;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(avia_hx717) */
#endif /* DT_HAS_COMPAT_STATUS_OKAY(avia_hx711) || DT_HAS_COMPAT_STATUS_OKAY(avia_hx717) */
};

LOG_MODULE_REGISTER(HX7XX, CONFIG_SENSOR_LOG_LEVEL);

static void hx7xx_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct hx7xx_data *data = CONTAINER_OF(cb, struct hx7xx_data, dout_gpio_cb);

	gpio_pin_interrupt_configure(dev, LOG2(pins), GPIO_INT_DISABLE);

	/* Signal thread that data is now ready */
	k_sem_give(&data->data_out_sem);
}

/**
 * @brief Send a pulse on the SCK pin.
 *
 * @param dev Pointer to the hx7xx device structure
 *
 * @retval The value of the DOUT pin (HIGH or LOW)
 *
 */
static int hx7xx_cycle(const struct device *dev)
{
	const struct hx7xx_config *cfg = dev->config;
#ifdef CONFIG_HX7XX_DISABLE_INTERRUPTS_WHILE_POLLING
	uint32_t key = irq_lock();
#endif /* CONFIG_HX7XX_DISABLE_INTERRUPTS_WHILE_POLLING */
	/* SCK set HIGH */
	gpio_pin_set_dt(&cfg->serial_clock, 1);
	k_busy_wait(BUSY_WAIT_US);

	/* SCK set LOW */
	gpio_pin_set_dt(&cfg->serial_clock, 0);
	k_busy_wait(BUSY_WAIT_US);
#ifdef CONFIG_HX7XX_DISABLE_INTERRUPTS_WHILE_POLLING
	irq_unlock(key);
#endif /* CONFIG_HX7XX_DISABLE_INTERRUPTS_WHILE_POLLING */
	/* Return DOUT pin state */
	return gpio_pin_get_dt(&cfg->data_out);
}

/**
 * @brief Read HX7xx data. Also sets GAIN for the next cycle.
 *
 * @param dev Pointer to the hx7xx device structure
 * @param chan Channel to fetch data for.
 *             Only SENSOR_CHANNEL_MASS is available.
 *
 * @retval 0 on success,
 * @retval -EACCES error if module is not powered up.
 * @retval -EIO error if sample_fetch_timeout_ms msec's elapsed with no data
 * available.
 *
 */
static int hx7xx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct hx7xx_config *cfg = dev->config;
	struct hx7xx_data *data = dev->data;
	uint32_t count = 0;
	int i, ret;

	ret = pm_device_runtime_get(dev);
	if (ret) {
		return ret;
	}

	if (cfg->sample_fetch_timeout_ms != 0 && gpio_pin_get_dt(&cfg->data_out) == 1) {
		ret = gpio_pin_interrupt_configure_dt(&cfg->data_out, GPIO_INT_EDGE_TO_INACTIVE);
		if (ret) {
			LOG_ERR("Failed to set dout GPIO interrupt");
			goto exit_fail;
		}

		/* Wait until reading is ready */
		if (k_sem_take(&data->data_out_sem, K_MSEC(cfg->sample_fetch_timeout_ms))) {
			LOG_ERR("Data not ready within %d ms. Is the device properly connected?",
				cfg->sample_fetch_timeout_ms);
			ret = -EIO;
			goto exit_fail;
		}
	}

	/* Generate clock signal for ADC_RESOLUTION bits of data */
	for (i = 0; i < ADC_RESOLUTION; i++) {
		count <<= 1;
		count |= hx7xx_cycle(dev);
	}

	/* set configuration for next read (GAIN or RATE) */
	for (i = 0; i <= data->extra_clock_num; i++) {
		hx7xx_cycle(dev);
	}

	/* Add padding to 24bit value to make a 32bit one. */
	count ^= 0x800000;
	data->sample = count;
	LOG_DBG("Raw reading : %d", data->sample);

exit_fail:
	i = pm_device_runtime_put(dev);
	return ret ? ret : i;
}

/**
 * @brief Set HX7xx gain.
 *
 * @param dev Pointer to the hx7xx device structure
 * @param val sensor_value struct.
 * @retval 0 on success,
 * @retval -ENOTSUP error if an invalid GAIN is provided or
 * @retval         -EACCES if sample_fetch_timeout msec's are elapsed with no data
 * available.
 *
 */
static int hx7xx_attr_set_gain(const struct device *dev, enum hx7xx_gain gain)
{
	const struct hx7xx_config *cfg = dev->config;
	struct hx7xx_data *data = dev->data;
	int ret;

	if (cfg->quirks->set_gain == NULL) {
		return -ENOTSUP;
	}

	ret = cfg->quirks->set_gain(dev, gain);
	if (ret) {
		return ret;
	}

	data->gain = gain;

	/* Get a sample to set gain and channel for next measurement */
	return hx7xx_sample_fetch(dev, SENSOR_CHAN_MASS);
}

/**
 * @brief Set HX7xx rate.
 *
 * @param dev Pointer to the hx7xx device structure
 * @param val sensor_value struct. Only val1 is used.
 *            valid values are :
 *               HX7xx_RATE_10HZ (default),
 *               HX7xx_RATE_80HZ
 *
 * @retval 0 on success,
 * @retval -EINVAL error if it fails to get RATE device.
 * @retval -ENOTSUP error if an invalid rate value is passed.
 * @retval a value from set_sampling_rate() quirk
 *
 */
static int hx7xx_attr_set_rate(const struct device *dev, enum hx7xx_rate rate)
{
	const struct hx7xx_config *cfg = dev->config;
	struct hx7xx_data *data = dev->data;
	int ret;

	if (cfg->quirks->set_sampling_rate == NULL) {
		return -ENOTSUP;
	}

	ret = cfg->quirks->set_sampling_rate(dev, rate);
	if (ret) {
		return ret;
	}

	data->rate = rate;
	LOG_DBG("Attribute RATE set to %d", data->rate);

	if (cfg->quirks->fetch_after_rate_change) {
		return hx7xx_sample_fetch(dev, SENSOR_CHAN_MASS);
	}

	return ret;
}

/**
 * @brief Get HX7xx attributes.
 *
 * @param dev Pointer to the hx7xx device structure
 * @param chan This param is ignored.
 * @param attr Attribute to get.
 *             Supported attributes :
 *               SENSOR_ATTR_SAMPLING_FREQUENCY
 *               SENSOR_ATTR_OFFSET
 *               SENSOR_ATTR_CALIBRATION
 *               SENSOR_ATTR_GAIN
 * @param val   Value to get.
 * @retval 0 on success
 * @retval -ENOTSUP if an invalid attribute is given
 *
 */
static int hx7xx_attr_get(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, struct sensor_value *val)
{
	struct hx7xx_data *data = dev->data;
	int ret = 0;

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		LOG_DBG("Attribute RATE is to %d", data->rate);
		val->val1 = data->rate;
		break;

	case SENSOR_ATTR_OFFSET:
		val->val1 = data->offset;
		LOG_DBG("Attribute OFFSET is to %d", data->offset);
		break;

	case SENSOR_ATTR_CALIBRATION:
		sensor_value_from_double(val, data->slope);
		LOG_DBG("Attribute SLOPE is %f", data->slope);
		break;

	case SENSOR_ATTR_GAIN:
		val->val1 = (int32_t)data->gain;
		LOG_DBG("Attribute GAIN is %d", data->gain);
		break;

	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return ret;
}

/**
 * @brief Set HX7xx attributes.
 *
 * @param dev Pointer to the hx7xx device structure
 * @param chan This param is ignored.
 * @param attr Attribute to change.
 *             Supported attributes :
 *               SENSOR_ATTR_SAMPLING_FREQUENCY
 *               SENSOR_ATTR_OFFSET
 *               SENSOR_ATTR_CALIBRATION
 *               SENSOR_ATTR_GAIN
 * @param val   Value to set.
 * @retval 0 on success
 * @retval -ENOTSUP if an invalid attribute is given
 * @retval -ENODEV if attribute cannot be changed because of HW configuration
 *
 */
static int hx7xx_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	struct hx7xx_data *data = dev->data;
	int ret = 0;

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = hx7xx_attr_set_rate(dev, val->val1);
		break;

	case SENSOR_ATTR_OFFSET:
		data->offset = val->val1;
		LOG_DBG("Attribute OFFSET set to %d", data->offset);
		break;

	case SENSOR_ATTR_CALIBRATION:
		data->slope = sensor_value_to_double(val);
		LOG_DBG("Attribute SLOPE set to %f", data->slope);
		break;

	case SENSOR_ATTR_GAIN:
		ret = hx7xx_attr_set_gain(dev, val->val1);
		break;

	default:
		return -ENOTSUP;
	}

	return ret;
}

/**
 * @brief Get HX7xx sample.
 *
 * @param dev Pointer to the hx7xx device structure
 * @param chan Channel to get.
 *             Supported channels :
 *               SENSOR_CHAN_MASS
 *               SENSOR_CHAN_ALL
 *
 * @param val  Value to write weight in grams value to.
 *        Formula is :
 *          weight = slope * (sample - offset)
 *
 * @retval 0 on success
 * @retval  -ENOTSUP if an invalid channel is given
 *
 */
static int hx7xx_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct hx7xx_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_MASS:
		sensor_value_from_double(val, (data->slope * (data->sample - data->offset)));
		return 0;

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
static int hx7xx_init(const struct device *dev)
{
	const struct hx7xx_config *cfg = dev->config;
	struct hx7xx_data *data = dev->data;
	int ret = 0;

	LOG_DBG("Initialising HX7xx %s", dev->name);

	/* enable device runtime power management */
	ret = pm_device_runtime_enable(dev);
	if (ret) {
		return ret;
	}

	LOG_DBG("SCK GPIO port : %s, pin %d", cfg->serial_clock.port->name, cfg->serial_clock.pin);
	LOG_DBG("DOUT GPIO port : %s, pin %d", cfg->data_out.port->name, cfg->data_out.pin);
	LOG_DBG("Gain : %d", data->gain);

	/* Configure SCK as output, LOW */
	ret = gpio_pin_configure_dt(&cfg->serial_clock, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx711) || DT_HAS_COMPAT_STATUS_OKAY(avia_hx717)
	if (cfg->rate_sel.port != NULL) {
		/* Configure RATE as output, LOW */
		LOG_DBG("RATE GPIO port : %s, pin %d", cfg->rate_sel.port->name, cfg->rate_sel.pin);
		ret = gpio_pin_configure_dt(&cfg->rate_sel, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("Failed to set rate pin (%d)", ret);
			return ret;
		}
	}
#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx717)
	if (cfg->rate_sel2.port != NULL) {
		/* Configure RATE as output, LOW */
		LOG_DBG("RATE2 GPIO port : %s, pin %d", cfg->rate_sel2.port->name,
			cfg->rate_sel2.pin);
		ret = gpio_pin_configure_dt(&cfg->rate_sel2, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
	}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(avia_hx717) */
#endif /* DT_HAS_COMPAT_STATUS_OKAY(avia_hx711) || DT_HAS_COMPAT_STATUS_OKAY(avia_hx717) */

	/* Configure DOUT as input */
	ret = gpio_pin_configure_dt(&cfg->data_out, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	k_sem_init(&data->data_out_sem, 1, 1);
	gpio_init_callback(&data->dout_gpio_cb, hx7xx_gpio_callback, BIT(cfg->data_out.pin));

	if (gpio_add_callback(cfg->data_out.port, &data->dout_gpio_cb) < 0) {
		LOG_DBG("Failed to set GPIO callback");
		return -EIO;
	}

	ret = hx7xx_attr_set_gain(dev, data->gain);
	if (ret && ret != -ENOTSUP) {
		return ret;
	}

	ret = hx7xx_attr_set_rate(dev, data->rate);
	if (ret && ret != -ENODEV) {
		return ret;
	}

	return 0;
}

static int hx7xx_bulk_read(const struct device *dev, uint8_t *readings, uint32_t *sum)
{
	struct hx7xx_data *data = dev->data;
	uint8_t valid_samples = 0;
	int ret;

	__ASSERT(readings != NULL, "readings cannot be NULL");
	__ASSERT(sum != NULL, "sum cannot be NULL");

	if (!dev) {
		return -EINVAL;
	}

	*readings = MAX(*readings, 1);
	*sum = 0;

	for (int i = 0; i < *readings; i++) {
		ret = hx7xx_sample_fetch(dev, SENSOR_CHAN_MASS);
		if (ret) {
			LOG_WRN("Fetching sample %d was not successful (%d)", i, ret);
			return ret;
		}

		*sum += data->sample;
		valid_samples++;
	}

	if (valid_samples == 0) {
		return -EIO;
	}

	*readings = valid_samples;

	return 0;
}

int avia_hx7xx_tare(const struct device *dev, uint8_t readings)
{
	struct hx7xx_data *data = dev->data;
	int32_t avg;
	int ret;

	ret = hx7xx_bulk_read(dev, &readings, &avg);
	if (ret) {
		return ret;
	}

	LOG_DBG("Sum of samples : %d", avg);
	avg /= readings;
	data->offset = avg;
	LOG_DBG("Offset set to %d", data->offset);

	return 0;
}

int avia_hx7xx_calibrate(const struct device *dev, uint8_t readings, double calibration_weight)
{
	struct hx7xx_data *data = dev->data;
	int32_t avg;
	int ret;

	if (calibration_weight <= 0) {
		return -EINVAL;
	}

	LOG_DBG("Calibration weight : %f", calibration_weight);
	ret = hx7xx_bulk_read(dev, &readings, &avg);
	if (ret) {
		return ret;
	}

	LOG_DBG("Sum of samples : %d", avg);
	avg /= readings;
	LOG_DBG("Average of samples : %d", avg);
	data->slope = calibration_weight / (avg - data->offset);
	LOG_DBG("Slope set to : %f", data->slope);

	return 0;
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
static int hx7xx_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct hx7xx_config *cfg = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = gpio_pin_set_dt(&cfg->serial_clock, 0);
		if (ret) {
			return ret;
		}

		if (cfg->quirks->power_on == NULL) {
			break;
		}

		ret = cfg->quirks->power_on(dev);
		if (ret) {
			return ret;
		}

		break;

	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_SUSPEND:
		if (cfg->quirks->power_off) {
			return cfg->quirks->power_off(dev);
		}

		return gpio_pin_set_dt(&cfg->serial_clock, 1);

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(sensor, hx7xx_api) = {
	.sample_fetch = hx7xx_sample_fetch,
	.channel_get = hx7xx_channel_get,
	.attr_set = hx7xx_attr_set,
	.attr_get = hx7xx_attr_get,
};

#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx711) || DT_HAS_COMPAT_STATUS_OKAY(avia_hx717)
static int hx711_set_gain(const struct device *dev, enum hx7xx_gain gain)
{
	struct hx7xx_data *data = dev->data;

	/* Currently we are not able to distinguish between channel A and B, so set configuration
	 * only for channel A
	 */
	switch (gain) {
	case HX7xx_GAIN_128X:
		/* Channel A GAIN 128 requires 25 clocks */
		data->extra_clock_num = 0;
		break;
	case HX7xx_GAIN_64X:
		/* Channel A GAIN 64 requires 27 clocks */
		data->extra_clock_num = 2;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(avia_hx711) || DT_HAS_COMPAT_STATUS_OKAY(avia_hx717) */

#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx711)
static int hx711_set_rate(const struct device *dev, enum hx7xx_rate rate)
{
	const struct hx7xx_config *cfg = dev->config;
	int ret = -ENODEV;
	uint8_t new_rate;

	if (cfg->rate_sel.port == NULL) {
		LOG_ERR("Rate pin not configured for instance %s", dev->name);
		return ret;
	}

	switch (rate) {
	case HX7xx_RATE_10HZ:
		/* S0: 0 - 10Hz */
		new_rate = 0;
		break;

	case HX7xx_RATE_80HZ:
		/* S0: 1 - 80Hz */
		new_rate = 1;
		break;

	default:
		return ret;
	}

	ret = gpio_pin_set_dt(&cfg->rate_sel, new_rate);

	return ret;
}

static int hx711_power_on(const struct device *dev)
{
	/* Fetch a sample to set GAIN again for HX711/HX717.
	 * GAIN is set to 128 channel A after RESET
	 */
	LOG_DBG("Setting GAIN. Ignore the next measurement.");
	return hx7xx_sample_fetch(dev, SENSOR_CHAN_MASS);
}

static const struct hx7xx_quirks hx711_quirks = {
	.fetch_after_rate_change = false,
	.set_sampling_rate = hx711_set_rate,
	.set_gain = hx711_set_gain,
	.power_on = hx711_power_on,
};
#endif /* DT_HAS_COMPAT_STATUS_OKAY(avia_hx711) */

#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx717) || DT_HAS_COMPAT_STATUS_OKAY(avia_hx71708)
static int hx717_power_off(const struct device *dev)
{
	const struct hx7xx_config *cfg = dev->config;
	int i, ret = 0;

	/* HX717/HX71708 are put to power-off state at 30th rising edge when kept in high state. */
	for (i = 0; i < 29; i++) {
		ret |= hx7xx_cycle(dev);
	}

	k_busy_wait(BUSY_WAIT_US);
	ret |= gpio_pin_set_dt(&cfg->serial_clock, 1);
	return ret;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(avia_hx717) || DT_HAS_COMPAT_STATUS_OKAY(avia_hx71708) */

#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx717)
static int hx717_set_rate(const struct device *dev, enum hx7xx_rate rate)
{
	const struct hx7xx_config *cfg = dev->config;
	uint8_t rate_cfg;
	int ret = -ENODEV;

	if (cfg->rate_sel2.port == NULL || cfg->rate_sel2.port == NULL) {
		LOG_ERR("Rate pin(s) not configured for instance %s", dev->name);
		return ret;
	}

	switch (rate) {
	case HX7xx_RATE_10HZ:
		/* S1S0: 00 - 10Hz */
		rate_cfg = 0;
		break;

	case HX7xx_RATE_20HZ:
		/* S1S0: 01 - 20Hz */
		rate_cfg = 1;
		break;

	case HX7xx_RATE_80HZ:
		/* S1S0: 10 - 80Hz */
		rate_cfg = 2;
		break;

	case HX7xx_RATE_320HZ:
		/* S1S0: 11 - 320Hz */
		rate_cfg = 3;
		break;

	default:
		return ret;
	}

	ret = gpio_pin_set_dt(&cfg->rate_sel2, rate_cfg & BIT(0));
	if (ret) {
		return ret;
	}

	ret = gpio_pin_set_dt(&cfg->rate_sel2, rate_cfg & BIT(1));

	return ret;
}

static const struct hx7xx_quirks hx717_quirks = {
	.fetch_after_rate_change = false,
	.set_sampling_rate = hx717_set_rate,
	.set_gain = hx711_set_gain, /* settings for channel A is same as in HX711 */
	.power_off = hx717_power_off,
};
#endif /* DT_HAS_COMPAT_STATUS_OKAY(avia_hx717) */

#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx710)
static int hx710_set_rate(const struct device *dev, enum hx7xx_rate rate)
{
	struct hx7xx_data *data = dev->data;
	int ret = -ENOTSUP;
	uint8_t new_rate;

	switch (rate) {
	case HX7xx_RATE_10HZ:
		/* 25 clocks - 10Hz */
		new_rate = 0;
		break;

	case HX7xx_RATE_40HZ:
		/* 27 clocks - 40Hz */
		new_rate = 2;
		break;

	default:
		return ret;
	}

	data->extra_clock_num = new_rate;

	return 0;
}

static const struct hx7xx_quirks hx710_quirks = {
	.fetch_after_rate_change = true,
	.set_sampling_rate = hx710_set_rate,
};
#endif /* DT_HAS_COMPAT_STATUS_OKAY(avia_hx710) */

#if DT_HAS_COMPAT_STATUS_OKAY(avia_hx71708)
static int hx71708_set_rate(const struct device *dev, enum hx7xx_rate rate)
{
	struct hx7xx_data *data = dev->data;
	int ret = -ENOTSUP;
	uint8_t new_rate;

	switch (rate) {
	case HX7xx_RATE_10HZ:
		/* 25 clocks - 10Hz */
		new_rate = 0;
		break;

	case HX7xx_RATE_20HZ:
		/* 26 clocks - 20Hz */
		new_rate = 1;
		break;

	case HX7xx_RATE_80HZ:
		/* 27 clocks - 80Hz */
		new_rate = 2;
		break;

	case HX7xx_RATE_320HZ:
		/* 28 clocks - 320Hz */
		new_rate = 3;
		break;

	default:
		return ret;
	}

	data->extra_clock_num = new_rate;

	return 0;
}

static const struct hx7xx_quirks hx71708_quirks = {
	.fetch_after_rate_change = true,
	.set_sampling_rate = hx71708_set_rate,
	.power_off = hx717_power_off, /* power-off sequence is same as for HX717 */
};
#endif /* DT_HAS_COMPAT_STATUS_OKAY(avia_hx71708) */

#define HX710_RATE_PIN_DEFINE(node)

#define HX711_RATE_PIN_DEFINE(node) .rate_sel = GPIO_DT_SPEC_GET_OR(node, rate_gpios, {}),

#define HX717_RATE_PINS_DEFINE(node)                                                               \
	.rate_sel = GPIO_DT_SPEC_GET_BY_IDX_OR(node, rate_gpios, 0, {}),                           \
	.rate_sel2 = GPIO_DT_SPEC_GET_BY_IDX_OR(node, rate_gpios, 1, {}),

#define HX7xx_INIT(node, rate_fn, quirks_ptr)                                                      \
	static struct hx7xx_data hx7xx_data_##node = {                                             \
		.gain = DT_PROP_OR(node, gain, HX7xx_GAIN_128X),                                   \
		.rate = DT_PROP(node, rate_hz),                                                    \
	};                                                                                         \
	static const struct hx7xx_config hx7xx_config_##node = {                                   \
		.sample_fetch_timeout_ms = DT_PROP(node, sample_fetch_timeout_ms),                 \
		.data_out = GPIO_DT_SPEC_GET(node, dout_gpios),                                    \
		.serial_clock = GPIO_DT_SPEC_GET(node, sck_gpios),                                 \
		.quirks = quirks_ptr,                                                              \
		rate_fn(node)};                                                                    \
                                                                                                   \
	PM_DEVICE_DT_DEFINE(node, hx7xx_pm_action);                                                \
                                                                                                   \
	DEVICE_DT_DEFINE(node, hx7xx_init, PM_DEVICE_DT_GET(node), &hx7xx_data_##node,             \
			 &hx7xx_config_##node, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,           \
			 &hx7xx_api);

/* HX710, HX712, HX720 */
DT_FOREACH_STATUS_OKAY_VARGS(avia_hx710, HX7xx_INIT, HX710_RATE_PIN_DEFINE, &hx710_quirks)

/* HX711 */
DT_FOREACH_STATUS_OKAY_VARGS(avia_hx711, HX7xx_INIT, HX711_RATE_PIN_DEFINE, &hx711_quirks)

/* HX717 */
DT_FOREACH_STATUS_OKAY_VARGS(avia_hx717, HX7xx_INIT, HX717_RATE_PINS_DEFINE, &hx717_quirks)

/* HX71708 */
DT_FOREACH_STATUS_OKAY_VARGS(avia_hx71708, HX7xx_INIT, HX710_RATE_PIN_DEFINE, &hx71708_quirks)
