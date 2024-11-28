/* Copyright (c) 2024 Daniel Kampert
 * Author: Daniel Kampert <DanielKampert@kampis-Elektroecke.de>
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

struct apds9306_data {
	uint32_t light;
};

struct apds9306_config {
	struct i2c_dt_spec i2c;
	uint8_t resolution;
	uint16_t frequency;
	uint8_t gain;
};

struct apds9306_worker_item_t {
	struct k_work_delayable dwork;
	const struct device *dev;
} apds9306_worker_item;

static uint32_t apds9306_get_time_for_resolution(uint8_t value)
{
	switch (value) {
	case 0:
		return 400;
	case 1:
		return 200;
	case 2:
		return 100;
	case 3:
		return 50;
	case 4:
		return 25;
	case 5:
		return 4;
	default:
		return 100;
	}
}

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

	LOG_DBG("Last measurement: %u", data->light);
}

static int apds9306_attr_set(const struct device *dev, enum sensor_channel channel,
			     enum sensor_attribute attribute, const struct sensor_value *value)
{
	uint8_t reg;
	uint8_t mask;
	uint8_t temp;
	const struct apds9306_config *config = dev->config;

	if (channel != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	if (attribute == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		reg = APDS9306_REGISTER_ALS_MEAS_RATE;
		mask = GENMASK(2, 0);
		temp = FIELD_PREP(0x07, value->val1);
	} else if (attribute == SENSOR_ATTR_GAIN) {
		reg = APDS9306_REGISTER_ALS_GAIN;
		mask = GENMASK(2, 0);
		temp = FIELD_PREP(0x07, value->val1);
	} else if (attribute == SENSOR_ATTR_RESOLUTION) {
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

	return 0;
}

static int apds9306_attr_get(const struct device *dev, enum sensor_channel channel,
			     enum sensor_attribute attribute, struct sensor_value *value)
{
	uint8_t mask;
	uint8_t temp;
	uint8_t reg;
	const struct apds9306_config *config = dev->config;

	if (channel != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	if (attribute == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		reg = APDS9306_REGISTER_ALS_MEAS_RATE;
		mask = 0x00;
	} else if (attribute == SENSOR_ATTR_GAIN) {
		reg = APDS9306_REGISTER_ALS_GAIN;
		mask = 0x00;
	} else if (attribute == SENSOR_ATTR_RESOLUTION) {
		reg = APDS9306_REGISTER_ALS_MEAS_RATE;
		mask = 0x04;
	} else {
		return -ENOTSUP;
	}

	if (i2c_reg_read_byte_dt(&config->i2c, reg, &temp)) {
		LOG_ERR("Failed to read sensor attribute!");
		return -EFAULT;
	}

	value->val1 = (temp >> mask) & 0x07;
	value->val2 = 0;

	return 0;
}

static int apds9306_sample_fetch(const struct device *dev, enum sensor_channel channel)
{
	uint8_t buffer;
	uint8_t resolution;
	uint16_t delay;
	const struct apds9306_config *config = dev->config;

	if ((channel != SENSOR_CHAN_ALL) && (channel != SENSOR_CHAN_LIGHT)) {
		return -ENOTSUP;
	}

	LOG_DBG("Start a new measurement...");
	if (apds9306_enable(dev) != 0) {
		LOG_ERR("Can not enable ALS!");
		return -EFAULT;
	}

	/* Get the measurement resolution. */
	if (i2c_reg_read_byte_dt(&config->i2c, APDS9306_REGISTER_ALS_MEAS_RATE, &buffer)) {
		LOG_ERR("Failed reading resolution");
		return -EFAULT;
	}

	/* Convert the resolution into a delay time and wait for the result. */
	resolution = (buffer >> 4) & 0x07;
	delay = apds9306_get_time_for_resolution(resolution);
	LOG_DBG("Measurement resolution: %u", resolution);
	LOG_DBG("Wait for %u ms", delay);

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

	if (channel != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	value->val1 = data->light;
	value->val2 = 0;

	return 0;
}

static int apds9306_sensor_setup(const struct device *dev)
{
	uint32_t now;
	uint8_t temp;
	const struct apds9306_config *config = dev->config;

	/* Wait for the device to become ready after a possible power cycle. */
	now = k_uptime_get_32();
	do {
		i2c_reg_read_byte_dt(&config->i2c, APDS9306_REGISTER_MAIN_STATUS, &temp);

		/* We wait 100 ms maximum for the device to become ready. */
		if ((k_uptime_get_32() - now) > 100) {
			LOG_ERR("Sensor timeout!");
			return -EFAULT;
		}

		k_msleep(10);
	} while (temp & APDS9306_BIT_POWER_ON_STATUS);

	if (i2c_reg_read_byte_dt(&config->i2c, APDS9306_REGISTER_PART_ID, &temp)) {
		LOG_ERR("Failed reading chip id!");
		return -EFAULT;
	}

	if ((temp != APDS_9306_CHIP_ID) && (temp != APDS_9306_065_CHIP_ID)) {
		LOG_ERR("Invalid chip id! Found 0x%X!", temp);
		return -EFAULT;
	}

	if (temp == APDS_9306_CHIP_ID) {
		LOG_DBG("APDS-9306 found!");
	} else if (temp == APDS_9306_065_CHIP_ID) {
		LOG_DBG("APDS-9306-065 found!");
	}

	/* Reset the sensor. */
	if (i2c_reg_write_byte_dt(&config->i2c, APDS9306_REGISTER_MAIN_CTRL,
				  APDS9306_BIT_SW_RESET)) {
		LOG_ERR("Can not reset the sensor!");
		return -EFAULT;
	}
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

	LOG_DBG("Start to initialize APDS9306...");

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("Bus device is not ready!");
		return -EINVAL;
	}

	if (apds9306_sensor_setup(dev)) {
		LOG_ERR("Failed to setup device!");
		return -EFAULT;
	}

	value = ((config->resolution & 0x07) << 4) | (config->frequency & 0x0F);
	LOG_DBG("Write configuration 0x%x to register 0x%x", value,
		APDS9306_REGISTER_ALS_MEAS_RATE);
	if (i2c_reg_write_byte_dt(&config->i2c, APDS9306_REGISTER_ALS_MEAS_RATE, value)) {
		return -EFAULT;
	}

	value = config->gain;
	LOG_DBG("Write configuration 0x%x to register 0x%x", value, APDS9306_REGISTER_ALS_GAIN);
	if (i2c_reg_write_byte_dt(&config->i2c, APDS9306_REGISTER_ALS_GAIN, value)) {
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
		.resolution = DT_INST_PROP(inst, resolution),                                      \
		.gain = DT_INST_PROP(inst, gain),                                                  \
		.frequency = DT_INST_PROP(inst, frequency),                                        \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, apds9306_init, NULL, &apds9306_data_##inst,             \
				     &apds9306_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &apds9306_driver_api);

DT_INST_FOREACH_STATUS_OKAY(APDS9306)
