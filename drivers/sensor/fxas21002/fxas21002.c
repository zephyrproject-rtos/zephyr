/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_fxas21002

#include "fxas21002.h"
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(FXAS21002, CONFIG_SENSOR_LOG_LEVEL);

/* Sample period in microseconds, indexed by output data rate encoding (DR) */
static const uint32_t sample_period[] = {
	1250, 2500, 5000, 10000, 20000, 40000, 80000, 80000
};

static int fxas21002_sample_fetch(const struct device *dev,
				  enum sensor_channel chan)
{
	const struct fxas21002_config *config = dev->config;
	struct fxas21002_data *data = dev->data;
	uint8_t buffer[FXAS21002_MAX_NUM_BYTES];
	int16_t *raw;
	int ret = 0;
	int i;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* Read all the channels in one I2C transaction. */
	if (i2c_burst_read_dt(&config->i2c, FXAS21002_REG_OUTXMSB, buffer,
			      sizeof(buffer))) {
		LOG_ERR("Could not fetch sample");
		ret = -EIO;
		goto exit;
	}

	/* Parse the buffer into raw channel data (16-bit integers). To save
	 * RAM, store the data in raw format and wait to convert to the
	 * normalized sensor_value type until later.
	 */
	raw = &data->raw[0];

	for (i = 0; i < sizeof(buffer); i += 2) {
		*raw++ = (buffer[i] << 8) | (buffer[i+1]);
	}

exit:
	k_sem_give(&data->sem);

	return ret;
}

static void fxas21002_convert(struct sensor_value *val, int16_t raw,
			      enum fxas21002_range range)
{
	int32_t micro_rad;

	/* Convert units to micro radians per second.
	 * 62500 micro dps * 2*pi/360 = 1091 micro radians per second
	 */
	micro_rad = (raw * 1091) >> range;

	val->val1 = micro_rad / 1000000;
	val->val2 = micro_rad % 1000000;
}

static int fxas21002_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	const struct fxas21002_config *config = dev->config;
	struct fxas21002_data *data = dev->data;
	int start_channel;
	int num_channels;
	int16_t *raw;
	int ret;
	int i;

	k_sem_take(&data->sem, K_FOREVER);

	/* Start with an error return code by default, then clear it if we find
	 * a supported sensor channel.
	 */
	ret = -ENOTSUP;

	/* Convert raw gyroscope data to the normalized sensor_value type. */
	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		start_channel = FXAS21002_CHANNEL_GYRO_X;
		num_channels = 1;
		break;
	case SENSOR_CHAN_GYRO_Y:
		start_channel = FXAS21002_CHANNEL_GYRO_Y;
		num_channels = 1;
		break;
	case SENSOR_CHAN_GYRO_Z:
		start_channel = FXAS21002_CHANNEL_GYRO_Z;
		num_channels = 1;
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		start_channel = FXAS21002_CHANNEL_GYRO_X;
		num_channels = 3;
		break;
	default:
		start_channel = 0;
		num_channels = 0;
		break;
	}

	raw = &data->raw[start_channel];
	for (i = 0; i < num_channels; i++) {
		fxas21002_convert(val++, *raw++, config->range);
	}

	if (num_channels > 0) {
		ret = 0;
	}

	if (ret != 0) {
		LOG_ERR("Unsupported sensor channel");
	}

	k_sem_give(&data->sem);

	return ret;
}

int fxas21002_get_power(const struct device *dev, enum fxas21002_power *power)
{
	const struct fxas21002_config *config = dev->config;
	uint8_t val = *power;

	if (i2c_reg_read_byte_dt(&config->i2c, FXAS21002_REG_CTRLREG1, &val)) {
		LOG_ERR("Could not get power setting");
		return -EIO;
	}
	val &= FXAS21002_CTRLREG1_POWER_MASK;
	*power = val;

	return 0;
}

int fxas21002_set_power(const struct device *dev, enum fxas21002_power power)
{
	const struct fxas21002_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, FXAS21002_REG_CTRLREG1,
				      FXAS21002_CTRLREG1_POWER_MASK, power);
}

uint32_t fxas21002_get_transition_time(enum fxas21002_power start,
				       enum fxas21002_power end,
				       uint8_t dr)
{
	uint32_t transition_time;

	/* If not transitioning to active mode, then don't need to wait */
	if (end != FXAS21002_POWER_ACTIVE) {
		return 0;
	}

	/* Otherwise, the transition time depends on which state we're
	 * transitioning from. These times are defined by the datasheet.
	 */
	transition_time = sample_period[dr];

	if (start == FXAS21002_POWER_READY) {
		transition_time += 5000U;
	} else {
		transition_time += 60000U;
	}

	return transition_time;
}

static int fxas21002_init(const struct device *dev)
{
	const struct fxas21002_config *config = dev->config;
	struct fxas21002_data *data = dev->data;
	uint32_t transition_time;
	uint8_t whoami;
	uint8_t ctrlreg1;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* Read the WHOAMI register to make sure we are talking to FXAS21002
	 * and not some other type of device that happens to have the same I2C
	 * address.
	 */
	if (i2c_reg_read_byte_dt(&config->i2c, FXAS21002_REG_WHOAMI, &whoami)) {
		LOG_ERR("Could not get WHOAMI value");
		return -EIO;
	}

	if (whoami != config->whoami) {
		LOG_ERR("WHOAMI value received 0x%x, expected 0x%x",
			    whoami, config->whoami);
		return -EIO;
	}

	/* Reset the sensor. Upon issuing a software reset command over the I2C
	 * interface, the sensor immediately resets and does not send any
	 * acknowledgment (ACK) of the written byte to the master. Therefore,
	 * do not check the return code of the I2C transaction.
	 */
	i2c_reg_write_byte_dt(&config->i2c, FXAS21002_REG_CTRLREG1,
			      FXAS21002_CTRLREG1_RST_MASK);

	/* Wait for the reset sequence to complete */
	do {
		if (i2c_reg_read_byte_dt(&config->i2c, FXAS21002_REG_CTRLREG1,
					 &ctrlreg1)) {
			LOG_ERR("Could not get ctrlreg1 value");
			return -EIO;
		}
	} while (ctrlreg1 & FXAS21002_CTRLREG1_RST_MASK);

	/* Set the full-scale range */
	if (i2c_reg_update_byte_dt(&config->i2c, FXAS21002_REG_CTRLREG0,
				   FXAS21002_CTRLREG0_FS_MASK, config->range)) {
		LOG_ERR("Could not set range");
		return -EIO;
	}

	/* Set the output data rate */
	if (i2c_reg_update_byte_dt(&config->i2c, FXAS21002_REG_CTRLREG1,
				   FXAS21002_CTRLREG1_DR_MASK,
				   config->dr << FXAS21002_CTRLREG1_DR_SHIFT)) {
		LOG_ERR("Could not set output data rate");
		return -EIO;
	}

	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

#if CONFIG_FXAS21002_TRIGGER
	if (fxas21002_trigger_init(dev)) {
		LOG_ERR("Could not initialize interrupts");
		return -EIO;
	}
#endif

	/* Set active */
	if (fxas21002_set_power(dev, FXAS21002_POWER_ACTIVE)) {
		LOG_ERR("Could not set active");
		return -EIO;
	}

	/* Wait the transition time from standby to active mode */
	transition_time = fxas21002_get_transition_time(FXAS21002_POWER_STANDBY,
							FXAS21002_POWER_ACTIVE,
							config->dr);
	k_busy_wait(transition_time);
	k_sem_give(&data->sem);

	LOG_DBG("Init complete");

	return 0;
}

static const struct sensor_driver_api fxas21002_driver_api = {
	.sample_fetch = fxas21002_sample_fetch,
	.channel_get = fxas21002_channel_get,
#if CONFIG_FXAS21002_TRIGGER
	.trigger_set = fxas21002_trigger_set,
#endif
};

static const struct fxas21002_config fxas21002_config = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
	.whoami = CONFIG_FXAS21002_WHOAMI,
	.range = CONFIG_FXAS21002_RANGE,
	.dr = CONFIG_FXAS21002_DR,
#ifdef CONFIG_FXAS21002_TRIGGER
#ifdef CONFIG_FXAS21002_DRDY_INT1
	.int_gpio = GPIO_DT_SPEC_INST_GET(0, int1_gpios),
#else
	.int_gpio = GPIO_DT_SPEC_INST_GET(0, int2_gpios),
#endif
#endif
};

static struct fxas21002_data fxas21002_data;

DEVICE_DT_INST_DEFINE(0, fxas21002_init, NULL,
		    &fxas21002_data, &fxas21002_config,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &fxas21002_driver_api);
