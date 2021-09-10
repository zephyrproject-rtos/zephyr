/*
 * Copyright (c) 2021, Silvano Cortesi
 * Copyright (c) 2021, ETH Zürich
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max30208

#include <sys/byteorder.h>
#include <logging/log.h>

#include "max30208.h"

LOG_MODULE_REGISTER(MAX30208, CONFIG_SENSOR_LOG_LEVEL);

static int max30208_start_measurement(const struct device *dev)
{
	const struct max30208_config *config = dev->config;

	/* Trigger one measurement */
	int ret = i2c_reg_write_byte_dt(&config->bus, MAX30208_REG_TEMP_SETUP,
					MAX30208_CONVERT_T_MASK);

	return ret;
}

static int max30208_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max30208_data *data = dev->data;
	uint8_t local_buffer[MAX30208_BYTES_PER_VAL] = { 0 };

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* If new value in ring buffer available, get it */
	if (ring_buf_size_get(&data->raw_buffer) < MAX30208_BYTES_PER_VAL) {
		return -ENODATA;
	}

	uint32_t ret = ring_buf_get(&data->raw_buffer, local_buffer, MAX30208_BYTES_PER_VAL);
	if (ret < 2) {
		LOG_ERR("Couldn't get data from ringbuffer");
	}

	/* Concat two bytes to a int16 value */
	int16_t temp_val = (int16_t)sys_get_be16(local_buffer);

	/* Convert raw value into temperature */
	val->val1 = temp_val / MAX30208_ONE_DEGREE;
	val->val2 = (temp_val % MAX30208_ONE_DEGREE) * MAX30208_LSB_e6;

	return 0;
}

#ifndef CONFIG_MAX30208_TRIGGER
static int max30208_data_ready(const struct device *dev, uint8_t *ready)
{
	const struct max30208_config *config = dev->config;
	struct max30208_status status;

	/* Read data ready */
	int ret = i2c_reg_read_byte_dt(&config->bus, MAX30208_REG_INT_STS,
				       MAX30208_STC_TO_P(status));
	if (ret < 0) {
		LOG_ERR("Could not read from MAX30208");
		return ret;
	}

	*ready = status.temp_rdy;

	return 0;
}

#else
int max30208_readout_batch(const struct device *dev)
{
	const struct max30208_config *config = dev->config;
	struct max30208_data *data = dev->data;

	uint8_t fifo_data_counter = 0U;
	uint8_t local_buffer[MAX30208_BYTES_PER_VAL * MAX30208_FIFO_SIZE];

	/* Read data ready */
	int ret = i2c_reg_read_byte_dt(&config->bus,
				       MAX30208_REG_FIFO_DATA_CTR,
				       &fifo_data_counter);
	if (ret < 0) {
		LOG_ERR("Could not read from MAX30208");
		return ret;
	}


	ret = i2c_burst_read_dt(&config->bus, MAX30208_REG_FIFO_DATA, local_buffer,
				fifo_data_counter * MAX30208_BYTES_PER_VAL);
	if (ret < 0) {
		LOG_ERR("Could not batch read sensor values");
		return ret;
	}

	/* Put both bytes into ring buffer */
	for (uint8_t i = 0; i < fifo_data_counter; i++) {
		if (ring_buf_space_get(&data->raw_buffer) < MAX30208_BYTES_PER_VAL) {
			struct sensor_value lost_val;

			ret = max30208_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &lost_val);
			if (ret < 0) {
				return ret;
			}
			LOG_INF("Buffer size to small. Value %d.%d is lost", lost_val.val1,
				lost_val.val2);
		}

		ret = ring_buf_put(&data->raw_buffer, &local_buffer[i * MAX30208_BYTES_PER_VAL],
				   MAX30208_BYTES_PER_VAL);
		if (ret < 0) {
			LOG_ERR("Couldn't put data to ringbuffer");
			return ret;
		}
	}

	return 0;
}
#endif

int max30208_readout_sample(const struct device *dev)
{
	const struct max30208_config *config = dev->config;
	struct max30208_data *data = dev->data;
	uint8_t local_buffer[MAX30208_BYTES_PER_VAL] = { 0 };

	/* Read data ready */
	int ret = i2c_burst_read_dt(&config->bus, MAX30208_REG_FIFO_DATA, local_buffer,
				    MAX30208_BYTES_PER_VAL);
	if (ret < 0) {
		LOG_ERR("Could not read sensor value");
		return ret;
	}

	/* Put both bytes into ring buffer */
	if (ring_buf_space_get(&data->raw_buffer) < MAX30208_BYTES_PER_VAL) {
		struct sensor_value lost_val;

		ret = max30208_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &lost_val);
		if (ret < 0) {
			return ret;
		}
		LOG_INF("Buffer size to small. Value %d.%d is lost", lost_val.val1, lost_val.val2);
	}

	ret = ring_buf_put(&data->raw_buffer, local_buffer, MAX30208_BYTES_PER_VAL);
	if (ret < 0) {
		LOG_ERR("Couldn't put data to ringbuffer");
		return ret;
	}

	return 0;
}

static int max30208_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* Trigger for one sample */
	int ret = max30208_start_measurement(dev);
	if (ret < 0) {
		return ret;
	}

#ifndef CONFIG_MAX30208_TRIGGER
	/* Give sensor time for measurement */
	k_sleep(K_MSEC(MAX30208_TMP_MEAS_TIME));

	/* Poll until conversion finishes */
	for (uint8_t i = 0, ready = 0; ready == 0; i++) {
		ret = max30208_data_ready(dev, &ready);
		if (ret < 0) {
			return ret;
		}
		if (i == MAX30208_POLL_TRIES - 1) {
			return -EIO;
		}
		k_sleep(K_MSEC(MAX30208_POLL_TIME));
	}

	/* Read one sample */
	ret = max30208_readout_sample(dev);
	if (ret < 0) {
		return ret;
	}
#endif

	return 0;
}

static const struct sensor_driver_api max30208_driver_api = {
	.sample_fetch = max30208_sample_fetch,
	.channel_get = max30208_channel_get,
#ifdef CONFIG_MAX30208_TRIGGER
	.trigger_set = max30208_trigger_set,
	.attr_set = max30208_attr_set,
#endif
};

static int max30208_init(const struct device *dev)
{
	const struct max30208_config *config = dev->config;

	uint8_t part_id;
	uint8_t sys_ctrl;

	/* Wait for I2C bus */
	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C dev %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	/* Check the part id to make sure this is MAX30208 */
	int ret = i2c_reg_read_byte_dt(&config->bus, MAX30208_REG_PART_ID, &part_id);
	if (ret < 0) {
		LOG_ERR("Could not get Part ID");
		return ret;
	}

	if (part_id != MAX30208_PART_ID) {
		LOG_ERR("Got Part ID 0x%02x, expected 0x%02x", part_id, MAX30208_PART_ID);
		return -EIO;
	}

	/* Reset the sensor */
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30208_REG_SYS_CTRL, MAX30208_RESET_MASK);
	if (ret < 0) {
		LOG_ERR("Failed to reset sensor!");
		return ret;
	}
	/* Wait for reset to be cleared */
	k_sleep(K_MSEC(MAX30208_RESET_TIME));
	do {
		ret = i2c_reg_read_byte_dt(&config->bus, MAX30208_REG_SYS_CTRL, &sys_ctrl);
		if (ret < 0) {
			LOG_ERR("Couldn't read system control after reset");
			return ret;
		}
	} while (sys_ctrl & MAX30208_RESET_MASK);

	/* Write the FIFO configuration register */
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30208_REG_FIFO_CFG1, config->fifo.fifo_a_full);
	if (ret < 0) {
		LOG_ERR("Failed to initialize FIFO config 1!");
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30208_REG_FIFO_CFG2,
				    MAX30208_STC_TO_B(config->fifo.config2));
	if (ret < 0) {
		LOG_ERR("Failed to initialize FIFO config 2!");
		return ret;
	}

	/* Write the GPIO configuration register */
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30208_REG_GPIO_SETUP,
				    MAX30208_STC_TO_B(config->gpio_setup));
	if (ret < 0) {
		LOG_ERR("Failed to initialize GPIO setup!");
		return ret;
	}

#ifdef CONFIG_MAX30208_TRIGGER

	ret = max30208_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return ret;
	}
#endif

	return 0;
}

#ifdef CONFIG_MAX30208_TRIGGER
#define MAX30208_GPIO_TRIGGER(n) .gpio_int = GPIO_DT_SPEC_INST_GET(n, int_gpios),
#else
#define MAX30208_GPIO_TRIGGER(n)
#endif

#define MAX30208_INIT(n)									\
	static struct max30208_config max30208_config_##n = {					\
		.bus = I2C_DT_SPEC_INST_GET(n),							\
		.fifo = {									\
			.fifo_a_full = DT_INST_PROP(n, fifo_a_full),				\
			.config2 = {								\
				.fifo_ro = DT_INST_PROP(n, fifo_rollover_en),			\
				.a_full_type = DT_INST_PROP(n, fifo_a_full_type),		\
				.fifo_stat_clr = DT_INST_PROP(n, fifo_stat_clr),		\
				.flush_fifo = 0,						\
			},									\
		},										\
		.gpio_setup = {									\
			.gpio0_mode = DT_INST_PROP(n, gpio0_mode),				\
			.gpio1_mode = DT_INST_PROP(n, gpio1_mode),				\
		},										\
		MAX30208_GPIO_TRIGGER(n)							\
	};											\
												\
	static struct max30208_data max30208_data_##n = {					\
		.raw_buffer = {									\
			.size = MAX30208_ARRAY_SIZE,						\
			.buf = { .buf8 = max30208_data_##n._ring_buffer_data_raw_buffer }	\
		}										\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, max30208_init, NULL, &max30208_data_##n, &max30208_config_##n,	\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &max30208_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX30208_INIT)
