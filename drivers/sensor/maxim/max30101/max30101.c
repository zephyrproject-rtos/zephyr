/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max30101

#include <zephyr/logging/log.h>

#include "max30101.h"

LOG_MODULE_REGISTER(MAX30101, CONFIG_SENSOR_LOG_LEVEL);

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

	/* Read all the active channels for one sample */
	num_bytes = data->total_channels * MAX30101_BYTES_PER_CHANNEL;
	if (i2c_burst_read_dt(&config->i2c, MAX30101_REG_FIFO_DATA, buffer,
			      num_bytes)) {
		LOG_ERR("Could not fetch sample");
		return -EIO;
	}

	fifo_chan = 0;
	for (i = 0; i < num_bytes; i += 3) {
		/* Each channel is 18-bits */
		fifo_data = (buffer[i] << 16) | (buffer[i + 1] << 8) |
			    (buffer[i + 2]);
		fifo_data = (fifo_data & MAX30101_FIFO_DATA_MASK) >> config->data_shift;

		/* Save the raw data */
		data->raw[fifo_chan++] = fifo_data;
	}

#if CONFIG_MAX30101_DIE_TEMPERATURE
	/* Read the die temperature */
	if (i2c_burst_read_dt(&config->i2c, MAX30101_REG_TINT, buffer, 2)) {
		LOG_ERR("Could not fetch die temperature");
		return -EIO;
	}

	/* Save the raw data */
	data->die_temp[0] = buffer[0];
	data->die_temp[1] = buffer[1];
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_TEMP_CFG, 1)) {
		return -EIO;
	}
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */

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

#if CONFIG_MAX30101_DIE_TEMPERATURE
	case SENSOR_CHAN_DIE_TEMP:
		val->val1 = data->die_temp[0];
		val->val2 = (1000000 * data->die_temp[1]) >> MAX30101_TEMP_FRAC_SHIFT;
		return 0;
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */

	default:
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	/* Check if the led channel is active by looking up the associated fifo
	 * channel. If the fifo channel isn't valid, then the led channel
	 * isn't active.
	 */
	fifo_chan = data->num_channels[led_chan];
	if (!fifo_chan) {
		LOG_ERR("Inactive sensor channel");
		return -ENOTSUP;
	}

	val->val1 = 0;
	for (fifo_chan = 0; fifo_chan < data->num_channels[led_chan]; fifo_chan++) {
		val->val1 += data->raw[data->map[led_chan][fifo_chan]];
	}

	/* TODO: Scale the raw data to standard units */
	val->val1 /= data->num_channels[led_chan];
	val->val2 = 0;

	return 0;
}

static DEVICE_API(sensor, max30101_driver_api) = {
	.sample_fetch = max30101_sample_fetch,
	.channel_get = max30101_channel_get,
#if CONFIG_MAX30101_TRIGGER
	.trigger_set = max30101_trigger_set,
#endif
};

static int max30101_configure(const struct device *dev)
{
	const struct max30101_config *config = dev->config;

	/* Write the FIFO configuration register */
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_FIFO_CFG,
				  config->fifo)) {
		return -EIO;
	}

	/* Write the mode configuration register */
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_MODE_CFG,
				  max30101_mode_convert[config->mode])) {
		return -EIO;
	}

	/* Write the SpO2 configuration register */
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_SPO2_CFG,
				  config->spo2)) {
		return -EIO;
	}

	/* Write the LED pulse amplitude registers */
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_LED1_PA,
				  config->led_pa[0])) {
		return -EIO;
	}
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_LED2_PA,
				  config->led_pa[1])) {
		return -EIO;
	}
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_LED3_PA,
				  config->led_pa[2])) {
		return -EIO;
	}
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_LED4_PA, config->led_pa[2])) {
		return -EIO;
	}

	if (!config->mode) {
		uint8_t multi_led[2];

		/* Write the multi-LED mode control registers */
		multi_led[0] = (config->slot[1] << 4) | (config->slot[0]);
		multi_led[1] = (config->slot[3] << 4) | (config->slot[2]);

		if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_MULTI_LED, multi_led[0])) {
			return -EIO;
		}
		if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_MULTI_LED + 1, multi_led[1])) {
			return -EIO;
		}
	}

#if CONFIG_MAX30101_DIE_TEMPERATURE
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_TEMP_CFG, 1)) {
		return -EIO;
	}
#endif /* CONFIG_MAX30101_DIE_TEMPERATURE */

#if CONFIG_MAX30101_TRIGGER
	if (max30101_init_interrupts(dev)) {
		LOG_ERR("Failed to initialize interrupts");
		return -EIO;
	}
#endif

	return 0;
}

static int max30101_init(const struct device *dev)
{
	const struct max30101_config *config = dev->config;
	struct max30101_data *data = dev->data;
	uint8_t part_id;
	uint8_t mode_cfg;
	uint32_t led_chan;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	/* Check the part id to make sure this is MAX30101 */
	if (i2c_reg_read_byte_dt(&config->i2c, MAX30101_REG_PART_ID, &part_id)) {
		LOG_ERR("Could not get Part ID");
		return -EIO;
	}
	if (part_id != MAX30101_PART_ID) {
		LOG_ERR("Got Part ID 0x%02x, expected 0x%02x", part_id, MAX30101_PART_ID);
		return -EIO;
	}

	/* Reset the sensor */
	if (i2c_reg_write_byte_dt(&config->i2c, MAX30101_REG_MODE_CFG,
				  MAX30101_MODE_CFG_RESET_MASK)) {
		return -EIO;
	}

	/* Wait for reset to be cleared */
	do {
		if (i2c_reg_read_byte_dt(&config->i2c, MAX30101_REG_MODE_CFG, &mode_cfg)) {
			LOG_ERR("Could read mode cfg after reset");
			return -EIO;
		}
	} while (mode_cfg & MAX30101_MODE_CFG_RESET_MASK);

	if (max30101_configure(dev)) {
		return -EIO;
	}

	/* Count the number of active channels and build a map that translates
	 * the LED channel number (red/ir/green) to the fifo channel number.
	 */
	for (int fifo_chan = 0; fifo_chan < MAX30101_MAX_NUM_CHANNELS; fifo_chan++) {
		led_chan = (config->slot[fifo_chan] & MAX30101_SLOT_LED_MASK) - 1;
		if (led_chan >= MAX30101_MAX_NUM_CHANNELS) {
			continue;
		}

		for (int i = 0; i < MAX30101_MAX_NUM_CHANNELS; i++) {
			if (data->map[led_chan][i] == MAX30101_MAX_NUM_CHANNELS) {
				data->map[led_chan][i] = fifo_chan;
				data->num_channels[led_chan]++;
				break;
			}
		}
		data->total_channels++;
	}

	return 0;
}

#define MAX30101_CHECK(n)                                                                          \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, led_pa) == 3,                                             \
		     "MAX30101 led-pa property must have exactly 3 elements");                     \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, led_slot) == 4,                                           \
		     "MAX30101 led-slot property must have exactly 4 elements")

#define MAX30101_SLOT_CFG(n)                                                                       \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, acq_mode, heart_rate), \
		(MAX30101_HR_SLOTS), \
		(COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, acq_mode, spo2), \
			(MAX30101_SPO2_SLOTS), \
			(MAX30101_MULTI_LED(n)) \
		)) \
	)

#define MAX30101_INIT(n)                                                                           \
	MAX30101_CHECK(n);                                                                         \
	static const struct max30101_config max30101_config_##n = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.fifo = (DT_INST_ENUM_IDX(n, smp_ave) << MAX30101_FIFO_CFG_SMP_AVE_SHIFT) |        \
			(DT_INST_PROP(n, fifo_rollover_en)                                         \
			 << MAX30101_FIFO_CFG_ROLLOVER_EN_SHIFT) |                                 \
			(DT_INST_PROP(n, fifo_watermark) << MAX30101_FIFO_CFG_FIFO_FULL_SHIFT),    \
		.mode = DT_INST_ENUM_IDX(n, acq_mode),                                             \
		.spo2 = (DT_INST_ENUM_IDX(n, adc_rge) << MAX30101_SPO2_ADC_RGE_SHIFT) |            \
			(DT_INST_ENUM_IDX(n, smp_sr) << MAX30101_SPO2_SR_SHIFT) |                  \
			(DT_INST_ENUM_IDX(n, led_pw) << MAX30101_SPO2_PW_SHIFT),                   \
		.led_pa = DT_INST_PROP(n, led_pa),                                                 \
		.slot = MAX30101_SLOT_CFG(n),                                                      \
		.data_shift = MAX30101_FIFO_DATA_MAX_SHIFT - DT_INST_ENUM_IDX(n, led_pw),          \
		IF_ENABLED(CONFIG_MAX30101_TRIGGER, \
			(.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(n, irq_gpios, {0}),) \
		) };              \
	static struct max30101_data max30101_data_##n = {                                          \
		.map = {{3, 3, 3}, {3, 3, 3}, {3, 3, 3}},                                          \
		.num_channels = {0, 0, 0},                                                         \
		.total_channels = 0,                                                               \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(n, max30101_init, NULL, &max30101_data_##n,                   \
				     &max30101_config_##n, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &max30101_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX30101_INIT)
