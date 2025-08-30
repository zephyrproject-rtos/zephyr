/*
 * Copyright (c) 2025 Paul Timke <ptimkec@live.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pixart_paj7620

#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/paj7620.h>

#include "paj7620.h"
#include "paj7620_reg.h"

LOG_MODULE_REGISTER(paj7620, CONFIG_SENSOR_LOG_LEVEL);

static int paj7620_select_register_bank(const struct device *dev, enum paj7620_mem_bank bank)
{
	int ret;
	uint8_t bank_selection;
	const struct paj7620_config *config = dev->config;

	switch (bank) {
	case PAJ7620_MEMBANK_0:
		bank_selection = PAJ7620_VAL_BANK_SEL_BANK_0;
		break;
	case PAJ7620_MEMBANK_1:
		bank_selection = PAJ7620_VAL_BANK_SEL_BANK_1;
		break;
	default:
		LOG_ERR("Nonexistent memory bank %d", (int)bank);
		return -EINVAL;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, PAJ7620_REG_BANK_SEL, bank_selection);
	if (ret < 0) {
		LOG_ERR("Failed to change memory bank");
		return ret;
	}

	return 0;
}

static int paj7620_get_hw_id(const struct device *dev, uint16_t *result)
{
	int ret;
	uint8_t hw_id[2];
	const struct paj7620_config *config = dev->config;

	/* Part ID is stored in bank 0 */
	ret = paj7620_select_register_bank(dev, PAJ7620_MEMBANK_0);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, PAJ7620_REG_PART_ID_LSB, &hw_id[0]);
	if (ret < 0) {
		LOG_ERR("Failed to read hardware ID");
		return ret;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, PAJ7620_REG_PART_ID_MSB, &hw_id[1]);
	if (ret < 0) {
		LOG_ERR("Failed to read hardware ID");
		return ret;
	}

	*result = sys_get_le16(hw_id);
	LOG_DBG("Obtained hardware ID 0x%04x", *result);

	return 0;
}

static int paj7620_write_initial_reg_settings(const struct device *dev)
{
	int ret;
	uint8_t reg_addr;
	uint8_t value;
	const struct paj7620_config *config = dev->config;

	/**
	 * Initializes registers with default values according to section 8.1
	 * from Datasheet v1.5:
	 * https://files.seeedstudio.com/wiki/Grove_Gesture_V_1.0/res/PAJ7620U2_DS_v1.5_05012022_Confidential.pdf
	 */

	for (int i = 0; i < ARRAY_SIZE(initial_register_array); i++) {
		reg_addr = initial_register_array[i][0];
		value = initial_register_array[i][1];

		ret = i2c_reg_write_byte_dt(&config->i2c, reg_addr, value);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

static int paj7620_set_sampling_rate(const struct device *dev, const struct sensor_value *val)
{
	int ret;
	int fps;
	const struct paj7620_config *config = dev->config;
	int64_t uval = ((int64_t)val->val1 * (int64_t)1000000) + val->val2;

	if (uval <= 120000000) {
		fps = PAJ7620_NORMAL_SPEED;
	} else if (uval <= 240000000) {
		fps = PAJ7620_GAME_SPEED;
	} else {
		LOG_ERR("Unsupported sample rate");
		return -ENOTSUP;
	}

	ret = paj7620_select_register_bank(dev, PAJ7620_MEMBANK_1);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, PAJ7620_REG_R_IDLE_TIME_LSB, fps);
	if (ret < 0) {
		LOG_ERR("Failed to set sample rate");
		return ret;
	}

	ret = paj7620_select_register_bank(dev, PAJ7620_MEMBANK_0);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Sample rate set to %s mode", fps == PAJ7620_GAME_SPEED ? "game" : "normal");

	return 0;
}

static int paj7620_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret;
	struct paj7620_data *data = dev->data;
	const struct paj7620_config *config = dev->config;
	uint8_t gest_data[2];

	if (chan != SENSOR_CHAN_ALL
		&& ((enum sensor_channel_paj7620)chan != SENSOR_CHAN_PAJ7620_GESTURES)) {
		return -ENOTSUP;
	}

	/* We read from REG_INT_FLAG_1 and REG_INT_FLAG_2 even on polling mode
	 * (without using interrupts) because that's where the gesture
	 * detection flags are set.
	 * NOTE: A set bit means that the corresponding gesture has been detected
	 */

	ret = i2c_burst_read_dt(&config->i2c, PAJ7620_REG_INT_FLAG_1, gest_data, sizeof(gest_data));
	if (ret < 0) {
		LOG_ERR("Failed to read gesture data");
		return ret;
	}

	data->gesture_flags = sys_get_le16(gest_data);

	return 0;
}

static int paj7620_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct paj7620_data *data = dev->data;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_PAJ7620_GESTURES:
		val->val1 = data->gesture_flags;
		val->val2 = 0;
		break;
	default:
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	return 0;
}

static int paj7620_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	int ret;

	if (chan != SENSOR_CHAN_ALL
		&& ((enum sensor_channel_paj7620)chan != SENSOR_CHAN_PAJ7620_GESTURES)) {
		return -ENOTSUP;
	}

	switch ((uint32_t)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = paj7620_set_sampling_rate(dev, val);
		break;

	default:
		return -ENOTSUP;
	}

	return ret;
}

static int paj7620_init(const struct device *dev)
{
	int ret;
	uint16_t hw_id;
	const struct paj7620_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/**
	 * According to the datasheet section 8.1, we must wait this amount
	 * of time for sensor to stabilize after power up
	 */
	k_usleep(PAJ7620_POWERUP_STABILIZATION_TIME_US);

	/**
	 * Make a write to the sensor to wake it up. After waking it, the
	 * the sensor still needs some time to be ready to listen. Without it,
	 * it may NACK subsequent transactions.
	 */
	(void)paj7620_select_register_bank(dev, PAJ7620_MEMBANK_0);
	k_usleep(PAJ7620_WAKEUP_TIME_US);

	/** Verify this is not some other sensor with the same address */
	ret = paj7620_get_hw_id(dev, &hw_id);
	if (ret < 0) {
		return ret;
	}

	if (hw_id != PAJ7620_PART_ID) {
		LOG_ERR("Hardware ID 0x%04x does not match for PAJ7620", hw_id);
		return -ENOTSUP;
	}

	/** Initialize settings (it defaults to gesture mode) */
	ret = paj7620_write_initial_reg_settings(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize device registers");
		return ret;
	}

	if (IS_ENABLED(CONFIG_PAJ7620_TRIGGER)) {
		ret = paj7620_trigger_init(dev);
		if (ret < 0) {
			LOG_ERR("Failed to enable interrupts");
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(sensor, paj7620_driver_api) = {
	.sample_fetch = paj7620_sample_fetch,
	.channel_get = paj7620_channel_get,
	.attr_set = paj7620_attr_set,
#if CONFIG_PAJ7620_TRIGGER
	.trigger_set = paj7620_trigger_set,
#endif
};

#define PAJ7620_INIT(n)                                                                    \
	static const struct paj7620_config paj7620_config_##n = {                          \
		.i2c = I2C_DT_SPEC_INST_GET(n),					           \
		IF_ENABLED(CONFIG_PAJ7620_TRIGGER,                                         \
				(.int_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}))) \
	};                                                                                 \
                                                                                           \
	static struct paj7620_data paj7620_data_##n;                                       \
                                                                                           \
	SENSOR_DEVICE_DT_INST_DEFINE(n,                                                    \
				paj7620_init,                                              \
				NULL,                                                      \
				&paj7620_data_##n,                                         \
				&paj7620_config_##n,                                       \
				POST_KERNEL,                                               \
				CONFIG_SENSOR_INIT_PRIORITY,                               \
				&paj7620_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PAJ7620_INIT);
