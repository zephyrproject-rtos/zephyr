/*
 * Copyright (c) Paul Timke <ptimkec@live.com>
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

int paj7620_byte_read(const struct device *dev, uint8_t reg, uint8_t *byte)
{
	const struct paj7620_config *config = dev->config;

	return i2c_reg_read_byte_dt(&config->i2c, reg, byte);
}

int paj7620_byte_write(const struct device *dev, uint8_t reg, uint8_t byte)
{
	const struct paj7620_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, reg, byte);
}

static int paj7620_select_register_bank(const struct device *dev, enum paj7620_mem_bank bank)
{
	int ret;
	uint8_t bank_selection;

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

	ret = paj7620_byte_write(dev, PAJ7620_REG_BANK_SEL, bank_selection);
	if (ret < 0) {
		LOG_ERR("Failed to change memory bank");
		return ret;
	}

	return 0;
}

static int paj7620_get_hw_id(const struct device *dev, uint16_t *result)
{
	uint8_t ret;
	uint8_t hw_id[2];

	/* Part ID is stored in bank 0 */
	ret = paj7620_select_register_bank(dev, PAJ7620_MEMBANK_0);
	if (ret < 0) {
		return ret;
	}

	ret = paj7620_byte_read(dev, PAJ7620_REG_PART_ID_LSB, &hw_id[0]);
	if (ret < 0) {
		LOG_ERR("Failed to read hardware ID");
		return ret;
	}

	ret = paj7620_byte_read(dev, PAJ7620_REG_PART_ID_MSB, &hw_id[1]);
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

	/**
	 * Initializes registers with default values according to section 8.1
	 * from Datasheet v1.5:
	 * https://files.seeedstudio.com/wiki/Grove_Gesture_V_1.0/res/PAJ7620U2_DS_v1.5_05012022_Confidential.pdf
	 */

	for (uint8_t i = 0; i < ARRAY_SIZE(initial_register_array); i++) {
		reg_addr = initial_register_array[i][0];
		value = initial_register_array[i][1];

		ret = paj7620_byte_write(dev, reg_addr, value);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int paj7620_check_fwd_bkwd_gest(const struct device *dev,
					enum paj7620_gesture initial_gesture,
					enum paj7620_gesture *return_gesture)
{
	int ret;
	struct paj7620_data *data = dev->data;
	uint8_t gesture_data;
	enum paj7620_gesture result = initial_gesture;

	/**
	 * Double check to see if user is executing a Z-axis gesture
	 * Entry- and exit-time delays are executed to give time for the sensor
	 * to detect the second, more complicated gesture (as lateral gestures
	 * are always detected first).
	 */

	k_msleep(data->gest_entry_time);

	ret = paj7620_byte_read(dev, PAJ7620_REG_INT_FLAG_1, &gesture_data);
	if (ret < 0) {
		return ret;
	}

	if (gesture_data == PAJ7620_MASK_INT_FLAG_1_GES_FORWARD) {
		k_msleep(data->gest_exit_time);
		result = PAJ7620_GES_FORWARD;
	} else if (gesture_data == PAJ7620_MASK_INT_FLAG_1_GES_BACKWARD) {
		k_msleep(data->gest_exit_time);
		result = PAJ7620_GES_BACKWARD;
	}

	*return_gesture = result;
	return 0;
}

static int paj7620_read_gesture(const struct device *dev, enum paj7620_gesture *result)
{
	int ret;
	struct paj7620_data *data = dev->data;
	uint8_t gest_data_reg1;
	uint8_t gest_data_reg2;

	/* We read from REG_INT_FLAG_1 and REG_INT_FLAG_2 even on polling mode
	 * (without using interrupts) because that's where the gesture
	 * detection flags are set.
	 * NOTE: A set bit means that the corresponding gesture has been detected
	 */

	ret = paj7620_byte_read(dev, PAJ7620_REG_INT_FLAG_1, &gest_data_reg1);

	if (ret < 0) {
		LOG_ERR("Failed to read gesture data");
		*result = PAJ7620_GES_NONE;
		return ret;
	}

	switch (gest_data_reg1) {
	case PAJ7620_MASK_INT_FLAG_1_GES_RIGHT:
		ret = paj7620_check_fwd_bkwd_gest(dev, PAJ7620_GES_RIGHT, result);
		break;

	case PAJ7620_MASK_INT_FLAG_1_GES_LEFT:
		ret = paj7620_check_fwd_bkwd_gest(dev, PAJ7620_GES_LEFT, result);
		break;

	case PAJ7620_MASK_INT_FLAG_1_GES_UP:
		ret = paj7620_check_fwd_bkwd_gest(dev, PAJ7620_GES_UP, result);
		break;

	case PAJ7620_MASK_INT_FLAG_1_GES_DOWN:
		ret = paj7620_check_fwd_bkwd_gest(dev, PAJ7620_GES_DOWN, result);
		break;

	case PAJ7620_MASK_INT_FLAG_1_GES_FORWARD:
		k_msleep(data->gest_exit_time);
		*result = PAJ7620_GES_FORWARD;
		break;

	case PAJ7620_MASK_INT_FLAG_1_GES_BACKWARD:
		k_msleep(data->gest_exit_time);
		*result = PAJ7620_GES_BACKWARD;
		break;

	case PAJ7620_MASK_INT_FLAG_1_GES_CLOCKWISE:
		*result = PAJ7620_GES_CLOCKWISE;
		break;

	case PAJ7620_MASK_INT_FLAG_1_GES_COUNTERCLOCKWISE:
		*result = PAJ7620_GES_COUNTERCLOCKWISE;
		break;

	default:
		/* Wave flag is stored in the other int flags register */
		ret = paj7620_byte_read(dev, PAJ7620_REG_INT_FLAG_2, &gest_data_reg2);
		if (ret == 0 && gest_data_reg2 == PAJ7620_MASK_INT_FLAG_2_GES_WAVE) {
			*result = PAJ7620_GES_WAVE;
		}
		break;
	}

	return ret;
}

static int paj7620_set_sampling_rate(const struct device *dev, const struct sensor_value *val)
{
	int ret;
	int fps;

	switch (val->val1) {
	case 120:
		fps = PAJ7620_NORMAL_SPEED;
		break;
	case 240:
		fps = PAJ7620_GAME_SPEED;
		break;
	default:
		LOG_ERR("Unsupported sample rate");
		return -ENOTSUP;
	}

	ret = paj7620_select_register_bank(dev, PAJ7620_MEMBANK_1);
	if (ret < 0) {
		return ret;
	}

	ret = paj7620_byte_write(dev, PAJ7620_REG_R_IDLE_TIME_LSB, fps);
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
	enum paj7620_gesture detected_gesture = PAJ7620_GES_NONE;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	/* Fetch gesture data */
	ret = paj7620_read_gesture(dev, &detected_gesture);
	if (ret < 0) {
		return ret;
	}

	data->gesture = detected_gesture;

	return 0;
}

static int paj7620_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct paj7620_data *data = dev->data;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_PAJ7620_GESTURES:
		val->val1 = (int32_t)data->gesture;
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
	struct paj7620_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch ((uint32_t)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = paj7620_set_sampling_rate(dev, val);
		break;

	case SENSOR_ATTR_PAJ7620_GESTURE_ENTRY_TIME:
		data->gest_entry_time = val->val1;
		break;

	case SENSOR_ATTR_PAJ7620_GESTURE_EXIT_TIME:
		data->gest_exit_time = val->val1;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int paj7620_init(const struct device *dev)
{
	int ret;
	uint16_t hw_id;
	struct paj7620_data *data = dev->data;
	const struct paj7620_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/**
	 * According to the datasheet section 8.1, we must wait this amount
	 * of time for sensor to stabilize after power up
	 */
	k_usleep(PAJ7620_SENSOR_STABILIZATION_TIME_US);

	data->gest_entry_time = PAJ7620_DEFAULT_GEST_ENTRY_TIME_MS;
	data->gest_exit_time = PAJ7620_DEFAULT_GEST_EXIT_TIME_MS;

	/**
	 * Make a write to the sensor to wake it up.
	 * Some times it can miss the first message at startup.
	 */
	(void)paj7620_select_register_bank(dev, PAJ7620_MEMBANK_0);

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

#ifdef CONFIG_PAJ7620_TRIGGER
	ret = paj7620_trigger_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to enable interrupts");
		return ret;
	}
#endif

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

#define PAJ7620_INIT(n)                                                            \
	static const struct paj7620_config paj7620_config_##n = {                  \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                    \
		IF_ENABLED(CONFIG_PAJ7620_TRIGGER,                                 \
				(.int_gpio = GPIO_DT_SPEC_INST_GET(n, int_gpios))) \
	};                                                                         \
                                                                                   \
	static struct paj7620_data paj7620_data_##n;                               \
                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n,                                            \
				paj7620_init,                                      \
				NULL,                                              \
				&paj7620_data_##n,                                 \
				&paj7620_config_##n,                               \
				POST_KERNEL,                                       \
				CONFIG_SENSOR_INIT_PRIORITY,                       \
				&paj7620_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PAJ7620_INIT);
