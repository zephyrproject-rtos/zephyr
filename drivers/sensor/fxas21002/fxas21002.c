/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fxas21002.h>
#include <misc/util.h>
#include <misc/__assert.h>

/* Sample period in microseconds, indexed by output data rate encoding (DR) */
static const u32_t sample_period[] = {
	1250, 2500, 5000, 10000, 20000, 40000, 80000, 80000
};

static int fxas21002_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	const struct fxas21002_config *config = dev->config->config_info;
	struct fxas21002_data *data = dev->driver_data;
	u8_t buffer[FXAS21002_MAX_NUM_BYTES];
	s16_t *raw;
	int ret = 0;
	int i;

	if (chan != SENSOR_CHAN_ALL) {
		SYS_LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* Read all the channels in one I2C transaction. */
	if (i2c_burst_read(data->i2c, config->i2c_address,
			   FXAS21002_REG_OUTXMSB, buffer, sizeof(buffer))) {
		SYS_LOG_ERR("Could not fetch sample");
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

static void fxas21002_convert(struct sensor_value *val, s16_t raw,
			      enum fxas21002_range range)
{
	s32_t micro_rad;

	/* Convert units to micro radians per second.*/
	micro_rad = (raw * 62500) >> range;

	val->val1 = micro_rad / 1000000;
	val->val2 = micro_rad % 1000000;
}

static int fxas21002_channel_get(struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	const struct fxas21002_config *config = dev->config->config_info;
	struct fxas21002_data *data = dev->driver_data;
	int start_channel;
	int num_channels;
	s16_t *raw;
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
		SYS_LOG_ERR("Unsupported sensor channel");
	}

	k_sem_give(&data->sem);

	return ret;
}

int fxas21002_get_power(struct device *dev, enum fxas21002_power *power)
{
	const struct fxas21002_config *config = dev->config->config_info;
	struct fxas21002_data *data = dev->driver_data;
	u8_t val = *power;

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXAS21002_REG_CTRLREG1,
			      &val)) {
		SYS_LOG_ERR("Could not get power setting");
		return -EIO;
	}
	val &= FXAS21002_CTRLREG1_POWER_MASK;
	*power = val;

	return 0;
}

int fxas21002_set_power(struct device *dev, enum fxas21002_power power)
{
	const struct fxas21002_config *config = dev->config->config_info;
	struct fxas21002_data *data = dev->driver_data;

	return i2c_reg_update_byte(data->i2c, config->i2c_address,
				   FXAS21002_REG_CTRLREG1,
				   FXAS21002_CTRLREG1_POWER_MASK,
				   power);
}

u32_t fxas21002_get_transition_time(enum fxas21002_power start,
				       enum fxas21002_power end,
				       u8_t dr)
{
	u32_t transition_time;

	/* If not transitioning to active mode, then don't need to wait */
	if (end != FXAS21002_POWER_ACTIVE) {
		return 0;
	}

	/* Otherwise, the transition time depends on which state we're
	 * transitioning from. These times are defined by the datasheet.
	 */
	transition_time = sample_period[dr];

	if (start == FXAS21002_POWER_READY) {
		transition_time += 5000;
	} else {
		transition_time += 60000;
	}

	return transition_time;
}

static int fxas21002_init(struct device *dev)
{
	const struct fxas21002_config *config = dev->config->config_info;
	struct fxas21002_data *data = dev->driver_data;
	u32_t transition_time;
	u8_t whoami;
	u8_t ctrlreg1;

	/* Get the I2C device */
	data->i2c = device_get_binding(config->i2c_name);
	if (data->i2c == NULL) {
		SYS_LOG_ERR("Could not find I2C device");
		return -EINVAL;
	}

	/* Read the WHOAMI register to make sure we are talking to FXAS21002
	 * and not some other type of device that happens to have the same I2C
	 * address.
	 */
	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      FXAS21002_REG_WHOAMI, &whoami)) {
		SYS_LOG_ERR("Could not get WHOAMI value");
		return -EIO;
	}

	if (whoami != config->whoami) {
		SYS_LOG_ERR("WHOAMI value received 0x%x, expected 0x%x",
			    whoami, config->whoami);
		return -EIO;
	}

	/* Reset the sensor. Upon issuing a software reset command over the I2C
	 * interface, the sensor immediately resets and does not send any
	 * acknowledgment (ACK) of the written byte to the master. Therefore,
	 * do not check the return code of the I2C transaction.
	 */
	i2c_reg_write_byte(data->i2c, config->i2c_address,
			   FXAS21002_REG_CTRLREG1, FXAS21002_CTRLREG1_RST_MASK);

	/* Wait for the reset sequence to complete */
	do {
		if (i2c_reg_read_byte(data->i2c, config->i2c_address,
				      FXAS21002_REG_CTRLREG1, &ctrlreg1)) {
			SYS_LOG_ERR("Could not get ctrlreg1 value");
			return -EIO;
		}
	} while (ctrlreg1 & FXAS21002_CTRLREG1_RST_MASK);

	/* Set the full-scale range */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXAS21002_REG_CTRLREG0,
				FXAS21002_CTRLREG0_FS_MASK,
				config->range)) {
		SYS_LOG_ERR("Could not set range");
		return -EIO;
	}

	/* Set the output data rate */
	if (i2c_reg_update_byte(data->i2c, config->i2c_address,
				FXAS21002_REG_CTRLREG1,
				FXAS21002_CTRLREG1_DR_MASK,
				config->dr << FXAS21002_CTRLREG1_DR_SHIFT)) {
		SYS_LOG_ERR("Could not set output data rate");
		return -EIO;
	}

#if CONFIG_FXAS21002_TRIGGER
	if (fxas21002_trigger_init(dev)) {
		SYS_LOG_ERR("Could not initialize interrupts");
		return -EIO;
	}
#endif

	/* Set active */
	if (fxas21002_set_power(dev, FXAS21002_POWER_ACTIVE)) {
		SYS_LOG_ERR("Could not set active");
		return -EIO;
	}

	/* Wait the transition time from standby to active mode */
	transition_time = fxas21002_get_transition_time(FXAS21002_POWER_STANDBY,
							FXAS21002_POWER_ACTIVE,
							config->dr);
	k_busy_wait(transition_time);


	k_sem_init(&data->sem, 0, UINT_MAX);
	k_sem_give(&data->sem);

	SYS_LOG_DBG("Init complete");

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
	.i2c_name = CONFIG_FXAS21002_I2C_NAME,
	.i2c_address = CONFIG_FXAS21002_I2C_ADDRESS,
	.whoami = CONFIG_FXAS21002_WHOAMI,
	.range = CONFIG_FXAS21002_RANGE,
	.dr = CONFIG_FXAS21002_DR,
#ifdef CONFIG_FXAS21002_TRIGGER
	.gpio_name = CONFIG_FXAS21002_GPIO_NAME,
	.gpio_pin = CONFIG_FXAS21002_GPIO_PIN,
#endif
};

static struct fxas21002_data fxas21002_data;

DEVICE_AND_API_INIT(fxas21002, CONFIG_FXAS21002_NAME, fxas21002_init,
		    &fxas21002_data, &fxas21002_config,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &fxas21002_driver_api);
