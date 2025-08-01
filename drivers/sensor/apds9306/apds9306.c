/* Copyright (c) 2024 Daniel Kampert
 * Author: Daniel Kampert <DanielKampert@kampis-elektroecke.de>
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define APDS9306_REGISTER_MAIN_CTRL       0x00
#define APDS9306_REGISTER_ALS_MEAS_RATE   0x04
#define APDS9306_REGISTER_ALS_GAIN        0x05
#define APDS9306_REGISTER_PART_ID         0x06
#define APDS9306_REGISTER_MAIN_STATUS     0x07
#define APDS9306_REGISTER_CLEAR_DATA_0    0x0A
#define APDS9306_REGISTER_CLEAR_DATA_1    0x0B
#define APDS9306_REGISTER_CLEAR_DATA_2    0x0C
#define APDS9306_REGISTER_ALS_DATA_0      0x0D
#define APDS9306_REGISTER_ALS_DATA_1      0x0E
#define APDS9306_REGISTER_ALS_DATA_2      0x0F
#define APDS9306_REGISTER_INT_CFG         0x19
#define APDS9306_REGISTER_INT_PERSISTENCE 0x1A
#define APDS9306_REGISTER_ALS_THRES_UP_0  0x21
#define APDS9306_REGISTER_ALS_THRES_UP_1  0x22
#define APDS9306_REGISTER_ALS_THRES_UP_2  0x23
#define APDS9306_REGISTER_ALS_THRES_LOW_0 0x24
#define APDS9306_REGISTER_ALS_THRES_LOW_1 0x25
#define APDS9306_REGISTER_ALS_THRES_LOW_2 0x26
#define APDS9306_REGISTER_ALS_THRES_VAR   0x27

#define ADPS9306_BIT_ALS_EN               BIT(0x01)
#define ADPS9306_BIT_ALS_DATA_STATUS      BIT(0x03)
#define APDS9306_BIT_SW_RESET             BIT(0x04)
#define ADPS9306_BIT_ALS_INTERRUPT_STATUS BIT(0x03)
#define APDS9306_BIT_POWER_ON_STATUS      BIT(0x05)

#define APDS_9306_065_CHIP_ID 0xB3
#define APDS_9306_CHIP_ID     0xB1

#define DT_DRV_COMPAT avago_apds9306

LOG_MODULE_REGISTER(avago_apds9306, CONFIG_SENSOR_LOG_LEVEL);

/* Array length for the measurement period values. Aligned with avago,apds9306.yaml */
static const uint8_t AVAGO_APDS_9306_MEASUREMENT_PERIOD_ARRAY_LENGTH = 7;

/* Array length for the resolution values. Aligned with avago,apds9306.yaml */
static const uint8_t AVAGO_APDS_9306_RESOLUTION_ARRAY_LENGTH = 6;

/* See datasheet for the values. Aligned with avago,apds9306.yaml */
static const uint8_t avago_apds9306_gain[] = {1, 3, 6, 9, 18};
static const uint8_t AVAGO_APDS_9306_GAIN_ARRAY_LENGTH = ARRAY_SIZE(avago_apds9306_gain);

/* See datasheet for the values. */
/* Last value is rounded up to prevent floating point operations. */
static const uint16_t avago_apds9306_integration_time[] = {400, 200, 100, 50, 25, 4};

/* These values represent the gain based on the integration time. */
/* A gain of 1 is used for a time of 3.125 ms (13 bits). */
/* This results in a gain of 8 (2^3) for a time if 25 ms (16 bits), etc. */
static const uint16_t avago_apds9306_integration_time_gain[] = {128, 64, 32, 16, 8, 1};

struct apds9306_data {
	uint32_t light;
	uint8_t measurement_period_idx; /* This field holds the index of the current */
					/* period measurement */
	uint8_t gain_idx;       /* This field holds the index of the current sampling gain. */
	uint8_t resolution_idx; /* This field holds the index of the current sampling */
				/*  resolution.*/
	uint8_t chip_id;
};

struct apds9306_config {
	struct i2c_dt_spec i2c;
	uint8_t resolution_idx;
	uint8_t measurement_period_idx;
	uint8_t gain_idx;
};

struct apds9306_worker_item_t {
	struct k_work_delayable dwork;
	const struct device *dev;
} apds9306_worker_item;

static int apds9306_enable(const struct device *dev)
{
	const struct apds9306_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, APDS9306_REGISTER_MAIN_CTRL,
				      ADPS9306_BIT_ALS_EN, ADPS9306_BIT_ALS_EN);
}

static int apds9306_standby(const struct device *dev)
{
	const struct apds9306_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, APDS9306_REGISTER_MAIN_CTRL,
				      ADPS9306_BIT_ALS_EN, 0x00);
}

static void apds9306_worker(struct k_work *p_work)
{
	uint8_t buffer[3];
	uint8_t reg;
	struct k_work_delayable *dwork = k_work_delayable_from_work(p_work);
	struct apds9306_worker_item_t *item =
		CONTAINER_OF(dwork, struct apds9306_worker_item_t, dwork);
	struct apds9306_data *data = item->dev->data;
	const struct apds9306_config *config = item->dev->config;

	if (i2c_reg_read_byte_dt(&config->i2c, APDS9306_REGISTER_MAIN_STATUS, &buffer[0])) {
		LOG_ERR("Failed to read ALS status!");
		return;
	}

	if (!(buffer[0] & ADPS9306_BIT_ALS_DATA_STATUS)) {
		LOG_DBG("No data ready!");
		return;
	}

	if (apds9306_standby(item->dev) != 0) {
		LOG_ERR("Can not disable ALS!");
		return;
	}

	reg = APDS9306_REGISTER_ALS_DATA_0;
	if (i2c_write_read_dt(&config->i2c, &reg, sizeof(reg), &buffer, sizeof(buffer)) < 0) {
		return;
	}

	data->light = sys_get_le24(buffer);
	LOG_DBG("Last raw measurement: %u", data->light);

	/* Based on the formula from the APDS-9309 datasheet, page 4:
	 * https://docs.broadcom.com/doc/AV02-3689EN
	 *
	 *  Illuminance [Lux] = Data * (1 / (Gain * Integration Time)) * Factor [Lux]
	 *
	 * The factor is calculated with the given values from the
	 * APDS-9306 datasheet, page 4.
	 * 1. Convert the E value from uW/sqcm to Lux
	 *   - 340.134 for the APDS-9306
	 *   - 293.69 for the APDS-9306-065
	 * 2. Use the formula from the APDS-9309 datasheet to get the factor by using
	 *   - Gain = 3
	 *   - Integration time = 100 ms
	 * Caution: The unit is ms. We need a unit without a dimension to prevent wrong
	 * units. So it must be converted into a value without dimension. This is done by converting
	 * it into a bit value based on the resolution gain (=32).
	 *   - ADC count = 2000
	 * 3. Repeat it for both sensor types to get the factors (converted for integer operations)
	 *   - APDS-9306: 16
	 *   - APDS-9306-065: 14
	 */
	uint32_t gain = avago_apds9306_gain[data->gain_idx];
	uint32_t integration_time = avago_apds9306_integration_time_gain[data->resolution_idx];
	uint32_t factor = 16;

	if (data->chip_id == APDS_9306_065_CHIP_ID) {
		factor = 14;
	}

	data->light = (data->light * factor) / (gain * integration_time);

	LOG_DBG("Gain: %u", gain);
	LOG_DBG("Integration time: %u", integration_time);
	LOG_DBG("Last measurement: %u", data->light);
}

static int apds9306_attr_set(const struct device *dev, enum sensor_channel channel,
			     enum sensor_attribute attribute, const struct sensor_value *value)
{
	uint8_t reg;
	uint8_t mask;
	uint8_t temp;
	const struct apds9306_config *config = dev->config;
	struct apds9306_data *data = dev->data;

	if ((channel != SENSOR_CHAN_ALL) && (channel != SENSOR_CHAN_LIGHT)) {
		return -ENOTSUP;
	}

	if (attribute == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		if (value->val1 > (AVAGO_APDS_9306_MEASUREMENT_PERIOD_ARRAY_LENGTH - 1)) {
			return -EINVAL;
		}
		reg = APDS9306_REGISTER_ALS_MEAS_RATE;
		mask = GENMASK(2, 0);
		temp = FIELD_PREP(0x07, value->val1);
	} else if (attribute == SENSOR_ATTR_GAIN) {
		if (value->val1 > (AVAGO_APDS_9306_GAIN_ARRAY_LENGTH - 1)) {
			return -EINVAL;
		}
		reg = APDS9306_REGISTER_ALS_GAIN;
		mask = GENMASK(2, 0);
		temp = FIELD_PREP(0x07, value->val1);
	} else if (attribute == SENSOR_ATTR_RESOLUTION) {
		if (value->val1 > (AVAGO_APDS_9306_RESOLUTION_ARRAY_LENGTH - 1)) {
			return -EINVAL;
		}
		reg = APDS9306_REGISTER_ALS_MEAS_RATE;
		mask = GENMASK(7, 4);
		temp = FIELD_PREP(0x07, value->val1) << 0x04;
	} else {
		return -ENOTSUP;
	}

	if (i2c_reg_update_byte_dt(&config->i2c, reg, mask, temp)) {
		LOG_ERR("Failed to set sensor attribute!");
		return -EFAULT;
	}

	/* We only save the new values when no error occurs to prevent invalid settings. */
	if (attribute == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		data->measurement_period_idx = value->val1;
	} else if (attribute == SENSOR_ATTR_GAIN) {
		data->gain_idx = value->val1;
	} else if (attribute == SENSOR_ATTR_RESOLUTION) {
		data->resolution_idx = value->val1;
	}

	return 0;
}

static int apds9306_attr_get(const struct device *dev, enum sensor_channel channel,
			     enum sensor_attribute attribute, struct sensor_value *value)
{
	struct apds9306_data *data = dev->data;

	if ((channel != SENSOR_CHAN_ALL) && (channel != SENSOR_CHAN_LIGHT)) {
		return -ENOTSUP;
	}

	if (attribute == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		value->val1 = data->measurement_period_idx;
	} else if (attribute == SENSOR_ATTR_GAIN) {
		value->val1 = data->gain_idx;
	} else if (attribute == SENSOR_ATTR_RESOLUTION) {
		value->val1 = data->resolution_idx;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int apds9306_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	uint16_t delay;
	struct apds9306_data *data = dev->data;

	if ((channel != SENSOR_CHAN_ALL) && (channel != SENSOR_CHAN_LIGHT)) {
		return -ENOTSUP;
	}

	LOG_DBG("Start a new measurement...");
	if (apds9306_enable(dev) != 0) {
		LOG_ERR("Can not enable ALS!");
		return -EFAULT;
	}

	/* Convert the resolution into a delay time and wait for the result. */
	delay = avago_apds9306_integration_time[data->resolution_idx];
	LOG_DBG("Measurement resolution index: %u", data->resolution_idx);
	LOG_DBG("Wait for %d ms", delay);

	/* We add a bit more delay to cover the startup time etc. */
	if (!k_work_delayable_is_pending(&apds9306_worker_item.dwork)) {
		LOG_DBG("Schedule new work");

		apds9306_worker_item.dev = dev;
		k_work_init_delayable(&apds9306_worker_item.dwork, apds9306_worker);
		k_work_schedule(&apds9306_worker_item.dwork, K_MSEC(delay + 100));
	} else {
		LOG_DBG("Work pending. Wait for completion.");
	}

	return 0;
}

static int apds9306_channel_get(const struct device *dev, enum sensor_channel channel,
				struct sensor_value *value)
{
	struct apds9306_data *data = dev->data;

	switch (channel) {
	case SENSOR_CHAN_LIGHT:
		value->val1 = data->light;
		value->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int apds9306_sensor_setup(const struct device *dev)
{
	uint32_t now;
	uint8_t temp;
	const struct apds9306_config *config = dev->config;
	struct apds9306_data *data = dev->data;

	/* Wait for the device to become ready after a possible power cycle. */
	now = k_uptime_get_32();
	do {
		if (i2c_reg_read_byte_dt(&config->i2c, APDS9306_REGISTER_MAIN_STATUS, &temp)) {
			LOG_ERR("Failed reading sensor status!");
			return -EFAULT;
		}

		/* We wait 100 ms maximum for the device to become ready. */
		if ((k_uptime_get_32() - now) > 100) {
			LOG_ERR("Sensor timeout!");
			return -EFAULT;
		}

		k_msleep(10);
	} while (temp & APDS9306_BIT_POWER_ON_STATUS);

	if (i2c_reg_read_byte_dt(&config->i2c, APDS9306_REGISTER_PART_ID, &data->chip_id)) {
		LOG_ERR("Failed reading chip id!");
		return -EFAULT;
	}

	if ((data->chip_id != APDS_9306_CHIP_ID) && (data->chip_id != APDS_9306_065_CHIP_ID)) {
		LOG_ERR("Invalid chip id! Found 0x%X!", data->chip_id);
		return -EFAULT;
	}

	if (data->chip_id == APDS_9306_CHIP_ID) {
		LOG_DBG("APDS-9306 found!");
	} else if (data->chip_id == APDS_9306_065_CHIP_ID) {
		LOG_DBG("APDS-9306-065 found!");
	}

	/* Reset the sensor. */
	i2c_reg_write_byte_dt(&config->i2c, APDS9306_REGISTER_MAIN_CTRL, APDS9306_BIT_SW_RESET);
	k_msleep(10);

	/* Perform a dummy read to avoid bus errors after the reset. See */
	/* https://lore.kernel.org/lkml/ab1d9746-4d23-efcc-0ee1-d2b8c634becd@tweaklogic.com/ */
	i2c_reg_read_byte_dt(&config->i2c, APDS9306_REGISTER_PART_ID, &temp);

	return 0;
}

static int apds9306_init(const struct device *dev)
{
	uint8_t value;
	const struct apds9306_config *config = dev->config;
	struct apds9306_data *data = dev->data;

	LOG_DBG("Start to initialize APDS9306...");

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("Bus device is not ready!");
		return -EINVAL;
	}

	if (apds9306_sensor_setup(dev)) {
		LOG_ERR("Failed to setup device!");
		return -EFAULT;
	}

	data->measurement_period_idx = config->measurement_period_idx;
	data->resolution_idx = config->resolution_idx;
	value = ((data->resolution_idx & 0x07) << 4) | (data->measurement_period_idx & 0x07);
	LOG_DBG("Write configuration 0x%x to register 0x%x", value,
		APDS9306_REGISTER_ALS_MEAS_RATE);
	if (i2c_reg_write_byte_dt(&config->i2c, APDS9306_REGISTER_ALS_MEAS_RATE, value)) {
		return -EFAULT;
	}

	data->gain_idx = config->gain_idx;
	LOG_DBG("Write configuration 0x%x to register 0x%x", value, APDS9306_REGISTER_ALS_GAIN);
	if (i2c_reg_write_byte_dt(&config->i2c, APDS9306_REGISTER_ALS_GAIN, data->gain_idx)) {
		return -EFAULT;
	}

	LOG_DBG("APDS9306 initialization successful!");

	return 0;
}

static DEVICE_API(sensor, apds9306_driver_api) = {
	.attr_set = apds9306_attr_set,
	.attr_get = apds9306_attr_get,
	.sample_fetch = apds9306_sample_fetch,
	.channel_get = apds9306_channel_get,
};

#define APDS9306(inst)                                                                             \
	static struct apds9306_data apds9306_data_##inst;                                          \
	static const struct apds9306_config apds9306_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.resolution_idx = DT_INST_ENUM_IDX(inst, resolution),                              \
		.gain_idx = DT_INST_ENUM_IDX(inst, gain),                                          \
		.measurement_period_idx = DT_INST_ENUM_IDX(inst, measurement_period),              \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, apds9306_init, NULL, &apds9306_data_##inst,             \
				     &apds9306_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &apds9306_driver_api);

DT_INST_FOREACH_STATUS_OKAY(APDS9306)
