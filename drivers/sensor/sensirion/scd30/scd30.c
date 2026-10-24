/*
 * Copyright (c) 2024 Jan Fäh
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensirion_scd30

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/crc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/sensor/scd30.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SCD30, CONFIG_SENSOR_LOG_LEVEL);

#define SCD30_CMD_START_MSRMT		0x0010
#define SCD30_CMD_STOP_MSRMT		0x0104
#define SCD30_CMD_SET_MSRMT_INTV	0x4600
#define SCD30_CMD_GET_STATUS		0x0202
#define SCD30_CMD_GET_MSRMT		0x0300
/* ASC = Automatic Self-Calibration */
#define SCD30_CMD_SET_ASC		0x5306
/* FRC = Forced Recalibration */
#define SCD30_CMD_SET_FRC_VALUE		0x5204
#define SCD30_CMD_SET_TEMP_OFFSET	0x5403
#define SCD30_CMD_SET_ALTITUDE		0x5102
#define SCD30_CMD_GET_FW_VER		0xd100
#define SCD30_CMD_SOFT_RESET		0xd304

#define SCD30_TEMPERATURE_OFFSET_MAX	654
#define SCD30_SENSOR_ALTITUDE_MAX	65535
#define SCD30_AMBIENT_PRESSURE_MAX	1400
#define SCD30_AMBIENT_PRESSURE_MIN	700
#define SCD30_SAMPLING_MAX		2
#define SCD30_SAMPLING_MIN		1800
#define SCD30_FRC_MAX			2000
#define SCD30_FRC_MIN			400

#define SCD30_CRC_POLY			0x31
#define SCD30_CRC_INIT			0xFF

/* Read measurement length with CRCs + command : 18 + 2 bytes */
#define SCD30_MAX_TRANSFER_SIZE		20
/* Without CRCs */
#define SCD30_MAX_TRANSFER_SIZE_DATA	12

/* Datasheet recommends a delay of over 3ms between write and read operations */
#define SCD30_TRANSFER_PAUSE_MS		4
/* "The boot-up time is < 2 s" */
#define SCD30_RESET_TIME_MS		2000
/* Doesn't need to be any tight due to minimal 2s fetch sample time */
#define SCD30_SPIN_WAIT_MS		250
#define SCD30_STABILIZATION_TIME_S	20

struct scd30_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec ready;
	bool one_shot_measurement;
	int default_measurement_period;
};

struct scd30_data {
	k_timepoint_t reset_timeout;
	float temp_sample;
	float humi_sample;
	float co2_sample;
	uint16_t pressure_compensation;
	uint16_t measurement_period;
#ifdef CONFIG_SCD30_TRIGGER
	struct gpio_callback ready_cb;
	const struct device *dev;
	sensor_trigger_handler_t trigger_cb;
	struct sensor_trigger trigger;
	struct k_work work;
#endif
};

static inline uint8_t scd30_calc_crc(uint8_t buf[2])
{
	return crc8(buf, 2, SCD30_CRC_POLY, SCD30_CRC_INIT, false);
}

static int scd30_write(const struct device *dev, uint16_t cmd, uint8_t *buf, size_t buf_len)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t t_buf[SCD30_MAX_TRANSFER_SIZE];
	size_t j = 2;

	__ASSERT_NO_MSG((buf_len & 1) == 0);
	__ASSERT_NO_MSG(buf_len <= SCD30_MAX_TRANSFER_SIZE_DATA);

	sys_put_be16(cmd, t_buf);

	if (buf_len > 0) {
		t_buf[j] = buf[0];
		j++;
		for (size_t i = 1; i < buf_len; i++) {
			t_buf[j] = buf[i];
			j++;
			if ((i & 1) == 1) {
				t_buf[j] = scd30_calc_crc(&t_buf[j-2]);
				j++;
			}
		}
	}

	return i2c_write_dt(&cfg->bus, t_buf, j);
}


static int scd30_read(const struct device *dev, uint16_t cmd, uint8_t *buf, size_t buf_len)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t t_buf[2];
	uint8_t r_buf[SCD30_MAX_TRANSFER_SIZE];
	uint8_t crc;
	int ret;
	size_t j = 0;

	__ASSERT_NO_MSG((buf_len & 1) == 0);
	__ASSERT_NO_MSG(buf_len <= SCD30_MAX_TRANSFER_SIZE_DATA);

	sys_put_be16(cmd, t_buf);

	ret = i2c_write_dt(&cfg->bus, t_buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to write command for read.");
		return ret;
	}

	k_msleep(SCD30_TRANSFER_PAUSE_MS);

	ret = i2c_read_dt(&cfg->bus, r_buf, buf_len + (buf_len / 2));
	if (ret < 0) {
		LOG_ERR("Failed to read data.");
		return ret;
	}

	if (buf_len > 0) {
		buf[0] = r_buf[j];
		j++;
		for (size_t i = 1; i < buf_len; i++) {
			buf[i] = r_buf[j];
			j++;
			if ((i & 1) == 1) {
				crc = scd30_calc_crc(&r_buf[j-2]);
				if (crc != r_buf[j]) {
					LOG_ERR("Invalid CRC for %x %x (received %x, expected %x).",
						r_buf[j-2], r_buf[j-1], r_buf[j], crc);
					return -EIO;
				}
				j++;
			}
		}
	}

	return 0;
}

static int scd30_data_ready(const struct device *dev)
{
	const struct scd30_config *cfg = dev->config;
	uint8_t buf[2];
	int ret;

	if (cfg->ready.port != NULL) {
		ret = gpio_pin_get_dt(&cfg->ready);
		if (ret < 0) {
			LOG_ERR("Failed to read READY GPIO.");
			return ret;
		}
		return ret;
	}

	ret = scd30_read(dev, SCD30_CMD_GET_STATUS, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to get readyness.");
		return ret;
	}

	if (sys_get_be16(buf) != 0) {
		return 1;
	}

	return 0;
}

static int scd30_read_sample(const struct device *dev)
{
	struct scd30_data *data = dev->data;
	uint8_t buf[SCD30_MAX_TRANSFER_SIZE_DATA];
	union {
		uint32_t u;
		float f;
	} tmp;
	int ret;

	ret = scd30_read(dev, SCD30_CMD_GET_MSRMT, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read measurement.");
		return ret;
	}

	tmp.u = sys_get_be32(buf);
	data->co2_sample = tmp.f;
	tmp.u = sys_get_be32(&buf[4]);
	data->temp_sample = tmp.f;
	tmp.u = sys_get_be32(&buf[8]);
	data->humi_sample = tmp.f;

	return 0;
}

static int scd30_set_temperature_offset(const struct device *dev, const struct sensor_value *val)
{
	int ret;
	uint8_t buf[2];
	uint16_t offset_temp = val->val1 * 100 + val->val2 / 10000;

	sys_put_be16(offset_temp, buf);

	ret = scd30_write(dev, SCD30_CMD_SET_TEMP_OFFSET, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to set the temperature offset.");
		return ret;
	}

	return 0;
}

static int scd30_get_temperature_offset(const struct device *dev, struct sensor_value *val)
{
	int ret;
	uint8_t buf[2];
	uint16_t offset_temp;

	ret = scd30_read(dev, SCD30_CMD_SET_TEMP_OFFSET, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to get the temperature offset.");
		return ret;
	}

	offset_temp = sys_get_be16(buf);
	val->val1 = offset_temp / 100;
	val->val2 = (offset_temp - val->val1 * 100) * 10000;

	return 0;
}

static int scd30_set_sensor_altitude(const struct device *dev, const struct sensor_value *val)
{
	int ret;
	uint8_t buf[2];

	sys_put_be16(val->val1, buf);

	ret = scd30_write(dev, SCD30_CMD_SET_ALTITUDE, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to set the altitude.");
		return ret;
	}

	return 0;
}

static int scd30_get_sensor_altitude(const struct device *dev, struct sensor_value *val)
{
	int ret;
	uint8_t buf[2];

	ret = scd30_read(dev, SCD30_CMD_SET_ALTITUDE, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to get the altitude.");
		return ret;
	}

	val->val1 = sys_get_be16(buf);
	val->val2 = 0;

	return 0;
}

static int scd30_set_automatic_calib_enable(const struct device *dev, bool yes)
{
	int ret;
	uint8_t buf[2];

	if (yes) {
		sys_put_be16(1, buf);
	} else {
		sys_put_be16(0, buf);
	}

	ret = scd30_write(dev, SCD30_CMD_SET_ASC, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to set Automatic Self-Calibration.");
		return ret;
	}

	return 0;
}

static int scd30_get_automatic_calib_enable(const struct device *dev, bool *yes)
{
	int ret;
	uint8_t buf[2];

	ret = scd30_read(dev, SCD30_CMD_SET_ASC, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to get Automatic Self-Calibration.");
		return ret;
	}

	*yes = sys_get_be16(buf) == 1 ? true : false;

	return 0;
}

static int scd30_set_sampling_period(const struct device *dev, const uint16_t period)
{
	struct scd30_data *data = dev->data;
	int ret;
	uint8_t buf[2];

	sys_put_be16(period, buf);

	ret = scd30_write(dev, SCD30_CMD_SET_MSRMT_INTV, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to set sampling period.");
		return ret;
	}

	data->measurement_period = period;

	return 0;
}

static int scd30_get_sampling_period(const struct device *dev, uint16_t *period)
{
	int ret;
	uint8_t buf[2];
	uint16_t t_period;

	ret = scd30_read(dev, SCD30_CMD_SET_MSRMT_INTV, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to get sampling period.");
		return ret;
	}

	t_period = sys_get_be16(buf);
	if (t_period < SCD30_SAMPLING_MAX || t_period > SCD30_SAMPLING_MIN) {
		return -EIO;
	}

	*period = t_period;

	return 0;
}

static int scd30_set_frc(const struct device *dev, const uint16_t frc)
{
	int ret;
	uint8_t buf[2];

	sys_put_be16(frc, buf);

	ret = scd30_write(dev, SCD30_CMD_SET_FRC_VALUE, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to set FRC value.");
		return ret;
	}

	return 0;
}

static int scd30_get_frc(const struct device *dev, uint16_t *frc)
{
	int ret;
	uint8_t buf[2];
	uint16_t t_frc;

	ret = scd30_read(dev, SCD30_CMD_SET_FRC_VALUE, buf, 2);
	if (ret < 0) {
		LOG_ERR("Failed to get FRC value.");
		return ret;
	}

	t_frc = sys_get_be16(buf);
	if (t_frc > SCD30_FRC_MAX || t_frc < SCD30_FRC_MIN) {
		return -EIO;
	}

	*frc = t_frc;

	return 0;
}

static int scd30_start_measurement(const struct device *dev)
{
	struct scd30_data *data = dev->data;
	uint8_t buf[2];
	int ret;

	sys_put_be16(data->pressure_compensation, buf);

	ret = scd30_write(dev, SCD30_CMD_START_MSRMT, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to trigger the device.");
		return ret;
	}

	return 0;
}

static int scd30_stop_measurement(const struct device *dev)
{
	int ret;

	ret = scd30_write(dev, SCD30_CMD_STOP_MSRMT, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Failed to untrigger the device.");
		return ret;
	}

	return 0;
}

static int scd30_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct scd30_config *cfg = dev->config;
	struct scd30_data *data = dev->data;
	k_timepoint_t timeout;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != SENSOR_CHAN_HUMIDITY && chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	k_sleep(sys_timepoint_timeout(data->reset_timeout));

	if (cfg->one_shot_measurement) {
		ret = scd30_start_measurement(dev);
		if (ret < 0) {
			return ret;
		}
	}

	timeout = sys_timepoint_calc(K_SECONDS(data->measurement_period * 2));

	while (true) {
		ret = scd30_data_ready(dev);
		if (ret < 0) {
			LOG_ERR("Failed to check data ready.");
			goto exit;
		}
		if (ret == 1) {
			break;
		}
		if (sys_timepoint_expired(timeout)) {
			ret = -ETIMEDOUT;
			goto exit;
		}
		k_msleep(SCD30_SPIN_WAIT_MS);
	}

	ret = scd30_read_sample(dev);
	if (ret < 0) {
		LOG_ERR("Failed to get sample data.");
	}

exit:
	if (cfg->one_shot_measurement) {
		if (ret != 0) {
			scd30_stop_measurement(dev);
		} else {
			ret = scd30_stop_measurement(dev);
		}
	}

	return ret;
}

static int scd30_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct scd30_data *data = dev->data;

	switch ((enum sensor_channel)chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		sensor_value_from_float(val, data->temp_sample);
		break;
	case SENSOR_CHAN_HUMIDITY:
		sensor_value_from_float(val, data->humi_sample);
		break;
	case SENSOR_CHAN_CO2:
		sensor_value_from_float(val, data->co2_sample);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static int scd30_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct scd30_config *cfg = dev->config;
	struct scd30_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != SENSOR_CHAN_HUMIDITY && chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	k_sleep(sys_timepoint_timeout(data->reset_timeout));

	if (val->val1 < 0 || val->val2 < 0) {
		return -EINVAL;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->val1 != 0) {
			return -EINVAL;
		}
		if (val->val2 > 1000000 / SCD30_SAMPLING_MAX
		    || val->val2 < 1000000 / SCD30_SAMPLING_MIN) {
			return -EINVAL;
		}
		ret = scd30_set_sampling_period(dev,
			CLAMP(1000000 / val->val2, SCD30_SAMPLING_MAX, SCD30_SAMPLING_MIN));
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD30_TEMPERATURE_OFFSET:
		if (val->val1 > SCD30_TEMPERATURE_OFFSET_MAX) {
			return -EINVAL;
		}
		ret = scd30_set_temperature_offset(dev, val);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD30_SENSOR_ALTITUDE:
		if (val->val1 > SCD30_SENSOR_ALTITUDE_MAX) {
			return -EINVAL;
		}
		ret = scd30_set_sensor_altitude(dev, val);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD30_AMBIENT_PRESSURE:
		if ((val->val1 > SCD30_AMBIENT_PRESSURE_MAX
		    || val->val1 < SCD30_AMBIENT_PRESSURE_MIN) && val->val1 != 0) {
			return -EINVAL;
		}
		data->pressure_compensation = val->val1;
		if (!cfg->one_shot_measurement) {
			ret = scd30_start_measurement(dev);
			if (ret < 0) {
				return ret;
			}
		}
		break;
	case SENSOR_ATTR_SCD30_AUTOMATIC_CALIB_ENABLE:
		if (cfg->one_shot_measurement) {
			return -ENOTSUP;
		}
		if (val->val1 > 1) {
			return -EINVAL;
		}
		if (val->val1 == 1) {
			ret = scd30_set_automatic_calib_enable(dev, true);
		} else {
			ret = scd30_set_automatic_calib_enable(dev, false);
		}
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD30_FORCED_RECALIBRATION_VALUE:
		if (cfg->one_shot_measurement) {
			return -ENOTSUP;
		}
		if (val->val1 > SCD30_FRC_MAX || val->val1 < SCD30_FRC_MIN) {
			return -EINVAL;
		}
		ret = scd30_set_frc(dev, val->val1);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int scd30_attr_get(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, struct sensor_value *val)
{
	struct scd30_data *data = dev->data;
	bool tmp_b;
	uint16_t tmp_u16;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    chan != SENSOR_CHAN_HUMIDITY && chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	k_sleep(sys_timepoint_timeout(data->reset_timeout));

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = scd30_get_sampling_period(dev, &data->measurement_period);
		if (ret < 0) {
			return ret;
		}
		val->val1 = 0;
		val->val2 = 1000000 / data->measurement_period;
		break;
	case SENSOR_ATTR_SCD30_TEMPERATURE_OFFSET:
		ret = scd30_get_temperature_offset(dev, val);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD30_SENSOR_ALTITUDE:
		ret = scd30_get_sensor_altitude(dev, val);
		if (ret < 0) {
			return ret;
		}
		break;
	case SENSOR_ATTR_SCD30_AMBIENT_PRESSURE:
		val->val1 = data->pressure_compensation;
		val->val2 = 0;
		break;
	case SENSOR_ATTR_SCD30_AUTOMATIC_CALIB_ENABLE:
		ret = scd30_get_automatic_calib_enable(dev, &tmp_b);
		if (ret < 0) {
			return ret;
		}
		if (tmp_b) {
			val->val1 = 1;
		} else {
			val->val1 = 0;
		}
		val->val2 = 0;
		break;
	case SENSOR_ATTR_SCD30_FORCED_RECALIBRATION_VALUE:
		ret = scd30_get_frc(dev, &tmp_u16);
		if (ret < 0) {
			return ret;
		}
		val->val1 = tmp_u16;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_SCD30_TRIGGER

static void scd30_gpio_callback(const struct device *dev,
				struct gpio_callback *cb, uint32_t pins)
{
	struct scd30_data *data = CONTAINER_OF(cb, struct scd30_data, ready_cb);
	const struct scd30_config *cfg = data->dev->config;

	if ((pins & BIT(cfg->ready.pin)) == 0U) {
		return;
	}

	k_work_submit(&data->work);
}

static void scd30_work_handler(struct k_work *work)
{
	struct scd30_data *data = CONTAINER_OF(work, struct scd30_data, work);

	if (data->trigger_cb != NULL) {
		data->trigger_cb(data->dev, &data->trigger);
	}
}

static int scd30_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		      sensor_trigger_handler_t handler)
{
	const struct scd30_config *cfg = dev->config;
	struct scd30_data *data = dev->data;
	int ret;

	if (cfg->one_shot_measurement) {
		LOG_ERR("One-shot mode is not supported with trigger");
		return -ENODEV;
	}

	if (cfg->ready.port == NULL) {
		LOG_ERR("Trigger requires the READY pin to be connected");
		return -ENODEV;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	if (trig->chan != SENSOR_CHAN_ALL && trig->chan != SENSOR_CHAN_AMBIENT_TEMP &&
	    trig->chan != SENSOR_CHAN_HUMIDITY && trig->chan != SENSOR_CHAN_CO2) {
		return -ENOTSUP;
	}

	k_sleep(sys_timepoint_timeout(data->reset_timeout));

	/* First configuration */
	if (data->dev == NULL) {
		data->dev = dev;

		gpio_init_callback(&data->ready_cb, scd30_gpio_callback, BIT(cfg->ready.pin));
		if (gpio_add_callback(cfg->ready.port, &data->ready_cb) < 0) {
			LOG_ERR("Failed to set GPIO callback");
			return -EIO;
		}

		k_work_init(&data->work, scd30_work_handler);
	}

	(void)gpio_pin_interrupt_configure_dt(&cfg->ready, GPIO_INT_DISABLE);

	data->trigger_cb = handler;
	data->trigger = *trig;

	/* Clear data if needed */
	if (gpio_pin_get_dt(&cfg->ready) == 1) {
		k_work_submit(&data->work);
	}

	if (handler != NULL) {
		ret = gpio_pin_interrupt_configure_dt(&cfg->ready, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#endif

static int scd30_init(const struct device *dev)
{
	const struct scd30_config *cfg = dev->config;
	struct scd30_data *data = dev->data;
	uint8_t buf[2];
	int ret;
	bool tmp;

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->bus.bus);
		return -ENODEV;
	}

	ret = scd30_read(dev, SCD30_CMD_GET_FW_VER, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to get firmware version.");
		return ret;
	}

	LOG_INF("Sensirion SCD30 FW%u.%u", buf[0], buf[1]);

	if (cfg->ready.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->ready)) {
			LOG_ERR("%s: device %s is not ready", dev->name,
					cfg->ready.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->ready, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("Failed to configure pin: %d", ret);
			return ret;
		}
	}

	if (cfg->one_shot_measurement) {
		ret = scd30_get_automatic_calib_enable(dev, &tmp);
		if (ret < 0) {
			return ret;
		}
		if (tmp) {
			ret = scd30_set_automatic_calib_enable(dev, false);
			if (ret < 0) {
				return ret;
			}
		}
		ret = scd30_stop_measurement(dev);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = scd30_start_measurement(dev);
		if (ret < 0) {
			return ret;
		}
	}

	ret = scd30_get_sampling_period(dev, &data->measurement_period);
	if (ret < 0) {
		return ret;
	}

	if ((cfg->default_measurement_period != 0
	     && data->measurement_period != cfg->default_measurement_period)
	    || (cfg->one_shot_measurement && cfg->default_measurement_period == 0
		&& data->measurement_period != SCD30_STABILIZATION_TIME_S)) {
		if (cfg->default_measurement_period != 0) {
			data->measurement_period = cfg->default_measurement_period;
		} else {
			data->measurement_period = SCD30_STABILIZATION_TIME_S;
		}
		ret = scd30_set_sampling_period(dev, data->measurement_period);
		if (ret < 0) {
			return ret;
		}
	}

	/* Reset restarts the measurements if they were previously enabled */
	ret = scd30_write(dev, SCD30_CMD_SOFT_RESET, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Failed to reset the device.");
		return ret;
	}

	data->reset_timeout = sys_timepoint_calc(K_MSEC(SCD30_RESET_TIME_MS));

	return 0;
}

static DEVICE_API(sensor, scd30_driver_api) = {
	.sample_fetch = scd30_sample_fetch,
	.channel_get = scd30_channel_get,
	.attr_set = scd30_attr_set,
	.attr_get = scd30_attr_get,
#ifdef CONFIG_SCD30_TRIGGER
	.trigger_set = scd30_trigger_set,
#endif
};

#define SCD30_INIT(inst)									\
	static struct scd30_data scd30_data_##inst = {0};					\
	static const struct scd30_config scd30_config_##inst = {				\
		.bus = I2C_DT_SPEC_INST_GET(inst),						\
		.ready = GPIO_DT_SPEC_INST_GET_OR(inst, ready_gpios, {0}),			\
		.one_shot_measurement = DT_INST_PROP(inst, one_shot_measurement),		\
		.default_measurement_period =							\
			DT_INST_PROP_OR(inst, default_measurement_period, 0),			\
	};											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, scd30_init, NULL, &scd30_data_##inst,		\
				     &scd30_config_##inst, POST_KERNEL,				\
				     CONFIG_SENSOR_INIT_PRIORITY, &scd30_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCD30_INIT)
