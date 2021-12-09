/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max30101

#include <sys/byteorder.h>
#include <logging/log.h>

#include "max30101.h"

LOG_MODULE_REGISTER(MAX30101, CONFIG_SENSOR_LOG_LEVEL);

static int max30101_fifo_channel_get(const struct device *dev,
					int fifo_chan,
					struct sensor_value *val)
{
	struct max30101_data *data = dev->data;
	uint8_t local_buffer[MAX30101_BYTES_PER_CHANNEL] = { 0 };

	/* Check if the led channel is active by looking up the associated fifo
	 * channel. If the fifo channel isn't valid, then the led channel
	 * isn't active.
	 */
	if (fifo_chan >= MAX30101_MAX_NUM_CHANNELS) {
		LOG_ERR("Inactive sensor channel");
		return -ENOTSUP;
	}

	/* If new value in ring buffer available, get it */
	if (ring_buf_size_get(&data->raw_buffer[fifo_chan]) < MAX30101_BYTES_PER_CHANNEL) {
		return -ENODATA;
	}

	uint32_t ret = ring_buf_get(&data->raw_buffer[fifo_chan], local_buffer,
			MAX30101_BYTES_PER_CHANNEL);

	if (ret == 0) {
		LOG_ERR("Couldn't get data from ringbuffer");
	}

	/* TODO: Scale the raw data to standard units */
	val->val1 = (int32_t)sys_get_be24(local_buffer);
	val->val2 = 0;

	return 0;
}

#ifdef CONFIG_MAX30101_TRIGGER
int max30101_readout_batch(const struct device *dev)
{
	const struct max30101_config *config = dev->config;

	uint8_t fifo_rd_ptr;
	uint8_t fifo_wr_ptr;

	/* Check write pointer */
	int ret = i2c_reg_read_byte_dt(&config->bus,
					MAX30101_REG_FIFO_WR,
					&fifo_wr_ptr);
	if (ret < 0) {
		LOG_ERR("Could not read fifo pointer from MAX30101");
		return ret;
	}

	/* Check read pointer */
	ret = i2c_reg_read_byte_dt(&config->bus,
				MAX30101_REG_FIFO_RD,
				&fifo_rd_ptr);
	if (ret < 0) {
		LOG_ERR("Could not read fifo pointer from MAX30101");
		return ret;
	}

	/* Difference between the two pointers is amount of samples to read, */
	/* including handling of a pointer overflow */
	int samples_to_read = (fifo_wr_ptr > fifo_rd_ptr) ? (fifo_wr_ptr - fifo_rd_ptr) :
				(fifo_wr_ptr + (MAX30101_FIFO_SIZE - fifo_rd_ptr));
	for (int i = 0; i < samples_to_read; i++) {
		ret = sensor_sample_fetch(dev);
		if (ret < 0) {
			LOG_ERR("Couldn't fetch data from MAX30101");
			return ret;
		}
	}

	return 0;
}
#endif

static int max30101_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct max30101_data *data = dev->data;
	const struct max30101_config *config = dev->config;
	uint8_t buffer[MAX30101_MAX_BYTES_PER_SAMPLE];
	uint32_t fifo_data;
	int fifo_chan;
	int num_bytes;
	int i;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}
	/* Read all the active channels for one sample */
	num_bytes = data->num_channels * MAX30101_BYTES_PER_CHANNEL;

	int ret = i2c_burst_read_dt(&config->bus, MAX30101_REG_FIFO_DATA, buffer, num_bytes);

	if (ret < 0) {
		LOG_ERR("Could not fetch sample");
		return ret;
	}

	fifo_chan = 0;
	for (i = 0; i < num_bytes; i += 3) {
		/* Each channel is 18-bits */
		fifo_data = sys_get_be24(&buffer[i]);
		fifo_data &= MAX30101_FIFO_DATA_MASK;
		sys_put_be24(fifo_data, &buffer[i]);

		/* Put both bytes into ring buffer */
		if (ring_buf_space_get(&data->raw_buffer[fifo_chan]) < MAX30101_BYTES_PER_CHANNEL) {
			struct sensor_value lost_val;

			ret = max30101_fifo_channel_get(dev, fifo_chan, &lost_val);
			if (ret < 0) {
				return ret;
			}
			LOG_INF("Buffer size to small. Value %d.%d is lost",
					lost_val.val1, lost_val.val2);
		}

		ret = ring_buf_put(&data->raw_buffer[fifo_chan++], &buffer[i],
				MAX30101_BYTES_PER_CHANNEL);
		if (ret < 0) {
			LOG_ERR("Couldn't put data to ringbuffer");
			return ret;
		}
	}

	return 0;
}

static int max30101_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct max30101_data *data = dev->data;
	enum max30101_led_channel led_chan;
	int fifo_chan;

	switch (chan) {
	case SENSOR_CHAN_RED:
		led_chan = MAX30101_LED_CHANNEL_RED;
		break;

	case SENSOR_CHAN_IR:
		led_chan = MAX30101_LED_CHANNEL_IR;
		break;

	case SENSOR_CHAN_GREEN:
		led_chan = MAX30101_LED_CHANNEL_GREEN;
		break;

	default:
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	/* Map LED to fifo channel and get desired data */
	fifo_chan = data->map[led_chan];
	return max30101_fifo_channel_get(dev, fifo_chan, val);
}

static const struct sensor_driver_api max30101_driver_api = {
	.sample_fetch = max30101_sample_fetch,
	.channel_get = max30101_channel_get,
#ifdef CONFIG_MAX30101_TRIGGER
	.trigger_set = max30101_trigger_set,
#endif
};

static int max30101_init(const struct device *dev)
{
	const struct max30101_config *config = dev->config;
	struct max30101_data *data = dev->data;
	uint8_t part_id;
	uint8_t mode_cfg;
	uint32_t led_chan;
	int fifo_chan;

	/* Wait for I2C bus */
	if (!device_is_ready(config->bus.bus)) {
		LOG_ERR("I2C dev %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	/* Check the part id to make sure this is MAX30101 */
	int ret = i2c_reg_read_byte_dt(&config->bus, MAX30101_REG_PART_ID, &part_id);

	if (ret < 0) {
		LOG_ERR("Could not get Part ID");
		return ret;
	}
	if (part_id != MAX30101_PART_ID) {
		LOG_ERR("Got Part ID 0x%02x, expected 0x%02x",
		part_id, MAX30101_PART_ID);
		return -EIO;
	}

	/* Reset the sensor */
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30101_REG_MODE_CFG,
				MAX30101_MODE_CFG_RESET_MASK);
	if (ret < 0) {
		return ret;
	}

	/* Wait for reset to be cleared */
	do {
		ret = i2c_reg_read_byte_dt(&config->bus, MAX30101_REG_MODE_CFG, &mode_cfg);
		if (ret < 0) {
			LOG_ERR("Could read mode cfg after reset");
			return ret;
		}
	} while (mode_cfg & MAX30101_MODE_CFG_RESET_MASK);

	/* Write the FIFO configuration register */
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30101_REG_FIFO_CFG, config->fifo);
	if (ret < 0) {
		return ret;
	}

	/* Write the mode configuration register */
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30101_REG_MODE_CFG, config->mode);
	if (ret < 0) {
		return ret;
	}

	/* Write the SpO2 configuration register */
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30101_REG_SPO2_CFG, config->spo2);
	if (ret < 0) {
		return ret;
	}

	/* Write the LED pulse amplitude registers */
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30101_REG_LED1_PA, config->led_pa[0]);
	if (ret < 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30101_REG_LED2_PA, config->led_pa[1]);
	if (ret < 0) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&config->bus, MAX30101_REG_LED3_PA, config->led_pa[2]);
	if (ret < 0) {
		return ret;
	}

	if (config->mode == MAX30101_MODE_MULTI_LED) {
		uint8_t multi_led[2];

		/* Write the multi-LED mode control registers */
		multi_led[0] = (config->slot[1] << 4) | (config->slot[0]);
		multi_led[1] = (config->slot[3] << 4) | (config->slot[2]);

		ret = i2c_reg_write_byte_dt(&config->bus, MAX30101_REG_MULTI_LED, multi_led[0]);
		if (ret < 0) {
			return ret;
		}
		ret = i2c_reg_write_byte_dt(&config->bus, MAX30101_REG_MULTI_LED + 1, multi_led[1]);
		if (ret < 0) {
			return ret;
		}
	}

	 /* Initialize the interrupt registers */
#ifdef CONFIG_MAX30101_TRIGGER
	ret = max30101_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return ret;
	}
#endif

	/* Initialize the channel map and active channel count */
	data->num_channels = 0U;
	for (led_chan = 0U; led_chan < MAX30101_MAX_NUM_CHANNELS; led_chan++) {
		data->map[led_chan] = MAX30101_MAX_NUM_CHANNELS;
	}

	/* Count the number of active channels and build a map that translates
	 * the LED channel number (red/ir/green) to the fifo channel number.
	 */
	for (fifo_chan = 0; fifo_chan < MAX30101_MAX_NUM_CHANNELS;
				fifo_chan++) {
		led_chan = (config->slot[fifo_chan] & MAX30101_SLOT_LED_MASK)-1;
		if (led_chan < MAX30101_MAX_NUM_CHANNELS) {
			data->map[led_chan] = fifo_chan;
			data->num_channels++;
		}
	}

	return 0;
}

#ifdef CONFIG_MAX30101_TRIGGER
#define MAX30101_GPIO_TRIGGER(n) .gpio_int = GPIO_DT_SPEC_INST_GET(n, int_gpios),
#else
#define MAX30101_GPIO_TRIGGER(n)
#endif

#define MAX30101_RING_BUF(n, k)								\
	{										\
		.size = MAX30101_ARRAY_SIZE,						\
		.buf = { .buf8 = max30101_data_##n._ring_buffer_data_raw_buffer[k] }	\
	}

#define MAX30101_INIT(n)									\
	static struct max30101_config max30101_config_##n = {					\
		.bus = I2C_DT_SPEC_INST_GET(n),							\
		.fifo = (DT_INST_PROP(n, smp_ave) << MAX30101_FIFO_CFG_SMP_AVE_SHIFT) |		\
				(DT_INST_PROP(n, fifo_rollover_en) <<				\
				MAX30101_FIFO_CFG_ROLLOVER_EN_SHIFT) |				\
				(DT_INST_PROP(n, fifo_a_full) <<				\
				MAX30101_FIFO_CFG_FIFO_FULL_SHIFT),				\
		.mode = DT_INST_PROP(n, led_mode),						\
		.slot[0] = (DT_INST_PROP(n, led_mode) != MAX30101_MODE_MULTI_LED) ?		\
				MAX30101_SLOT_RED_LED1_PA : DT_INST_PROP(n, led_slot1),		\
		.slot[1] = (DT_INST_PROP(n, led_mode) == MAX30101_MODE_SPO2) ?			\
				MAX30101_SLOT_IR_LED2_PA : DT_INST_PROP(n, led_slot2),		\
		.slot[2] = DT_INST_PROP(n, led_slot3),						\
		.slot[3] = DT_INST_PROP(n, led_slot4),						\
		.spo2 = (DT_INST_PROP(n, adc_rge) << MAX30101_SPO2_ADC_RGE_SHIFT) |		\
				(DT_INST_PROP(n, smp_rate_ctrl) << MAX30101_SPO2_SR_SHIFT) |	\
				(MAX30101_PW_18BITS << MAX30101_SPO2_PW_SHIFT),			\
		.led_pa[0] = DT_INST_PROP(n, led1_pa),						\
		.led_pa[1] = DT_INST_PROP(n, led2_pa),						\
		.led_pa[2] = DT_INST_PROP(n, led3_pa),						\
		MAX30101_GPIO_TRIGGER(n)							\
	};											\
												\
	static struct max30101_data max30101_data_##n = {					\
		.raw_buffer = {									\
			MAX30101_RING_BUF(n, 0), MAX30101_RING_BUF(n, 1),			\
			MAX30101_RING_BUF(n, 2),						\
		}										\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, max30101_init, NULL,						\
				&max30101_data_##n, &max30101_config_##n,			\
				POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
				&max30101_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX30101_INIT)
