/*
 * Copyright (c) 2024, Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ist_tsic_xx6

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(TSIC_XX6, CONFIG_SENSOR_LOG_LEVEL);

#define FRAME_BIT_PERIOD_US 125

enum {
	FRAME_PARITIY_BIT_LSB,
	FRAME_DATA_BIT_0,
	FRAME_DATA_BIT_1,
	FRAME_DATA_BIT_2,
	FRAME_DATA_BIT_3,
	FRAME_DATA_BIT_4,
	FRAME_DATA_BIT_5,
	FRAME_DATA_BIT_6,
	FRAME_DATA_BIT_7,
	FRAME_START_BIT_LSB,
	/* Theres a single bit period between the two packets that is constant high. This bit will
	 * be part of the 2nd packet's start bit thus frame length is not affected.
	 */
	FRAME_PARITIY_BIT_MSB,
	FRAME_DATA_BIT_8,
	FRAME_DATA_BIT_9,
	FRAME_DATA_BIT_10,
	FRAME_DATA_BIT_11,
	FRAME_DATA_BIT_12,
	FRAME_DATA_BIT_13,
	FRAME_ZERO_BIT_0,
	FRAME_ZERO_BIT_1,
	FRAME_START_BIT_MSB,
	FRAME_READY_BIT,
	FRAME_FLAGS,
};

struct tsic_xx6_config {
	const struct pwm_dt_spec pwm;
	const int8_t lower_temperature_limit;
	const uint8_t higher_temperature_limit;
	const uint8_t data_bits;
};

struct tsic_xx6_data {
	uint64_t frame_cycles;
	struct sensor_value val;

	ATOMIC_DEFINE(frame, FRAME_FLAGS);
	uint32_t buf;
	uint8_t buf_index;
};

static inline void tsic_xx6_buf_reset(struct tsic_xx6_data *data)
{
	data->buf_index = FRAME_START_BIT_MSB;
}

static inline bool tsic_xx6_is_buf_reset(struct tsic_xx6_data *data)
{
	return data->buf_index == FRAME_START_BIT_MSB;
}

static inline bool tsic_xx6_is_data_line_idle(struct tsic_xx6_data *data, uint64_t period_cycles)
{
	/* If the period is larger than two frames assume the data line has been idle */
	return period_cycles > data->frame_cycles * 2;
}

static void tsic_xx6_pwm_callback(const struct device *dev, uint32_t channel,
				  uint32_t period_cycles, uint32_t pulse_cycles, int status,
				  void *user_data)
{
	const struct device *tsic_xx6_dev = user_data;
	const struct tsic_xx6_config *config = tsic_xx6_dev->config;
	struct tsic_xx6_data *data = tsic_xx6_dev->data;
	uint32_t low_cycles;
	bool val;

	if (dev != config->pwm.dev || channel != config->pwm.channel) {
		return;
	}

	if (status != 0) {
		LOG_ERR("callback failed: %d", status);
		return;
	}

	if (!tsic_xx6_is_buf_reset(data) && tsic_xx6_is_data_line_idle(data, period_cycles)) {
		LOG_ERR("unexpected data idle");
		tsic_xx6_buf_reset(data);
	}

	/*
	 * Calculate low cycles: The sensor sends the pulse in the last part of the period. The PWM
	 * capture driver triggers on rising edge with normal polarity. Therefore only the low part
	 * of the frame bit is present.
	 */
	low_cycles = period_cycles - pulse_cycles;

	/* 25 % duty cycle is 0, 75 % duty cycle is 1 */
	val = low_cycles * 2 < data->frame_cycles;
	WRITE_BIT(data->buf, data->buf_index, val);

	if (data->buf_index > 0) {
		--data->buf_index;
	} else {
		WRITE_BIT(data->buf, FRAME_READY_BIT, 1);
		(void)atomic_set(data->frame, data->buf);
		tsic_xx6_buf_reset(data);
	}
}

static inline bool tsic_xx6_parity_check(uint8_t data, bool parity)
{
	bool data_parity = false;
	size_t i;

	for (i = 0; i < 8; ++i) {
		data_parity ^= FIELD_GET(BIT(i), data);
	}

	return (parity ^ data_parity) == 0;
}

static int tsic_xx6_get_data_bits(const struct tsic_xx6_config *config, uint16_t *data_bits,
				  uint32_t frame)
{
	uint8_t frame_data_bit_high =
		config->data_bits == 14 ? FRAME_DATA_BIT_13 : FRAME_DATA_BIT_10;
	uint8_t data_msb = FIELD_GET(GENMASK(frame_data_bit_high, FRAME_DATA_BIT_8), frame);
	uint8_t data_lsb = FIELD_GET(GENMASK(FRAME_DATA_BIT_7, FRAME_DATA_BIT_0), frame);
	bool parity_msb = FIELD_GET(BIT(FRAME_PARITIY_BIT_MSB), frame);
	bool parity_lsb = BIT(FRAME_PARITIY_BIT_LSB) & frame;

	if (!tsic_xx6_parity_check(data_msb, parity_msb) ||
	    !tsic_xx6_parity_check(data_lsb, parity_lsb)) {
		return -EIO;
	}

	*data_bits = data_msb << 8 | data_lsb;

	return 0;
}

static void tsic_xx6_get_value(const struct tsic_xx6_config *config, struct tsic_xx6_data *data,
			       uint16_t data_bits)
{
	int64_t tmp;

	/* Apply the datasheet formula scaled to micro celcius */
	tmp = (int64_t)data_bits *
	      (config->higher_temperature_limit - config->lower_temperature_limit);
	tmp = tmp * 1000000 / (BIT(config->data_bits) - 1);
	tmp += (int64_t)config->lower_temperature_limit * 1000000;

	data->val.val1 = tmp / 1000000;
	data->val.val2 = tmp % 1000000;
}

static int tsic_xx6_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tsic_xx6_config *config = dev->config;
	struct tsic_xx6_data *data = dev->data;
	uint32_t frame;
	uint16_t data_bits;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	frame = atomic_and(data->frame, ~BIT(FRAME_READY_BIT));

	if (FIELD_GET(BIT(FRAME_READY_BIT), frame) == 0) {
		return -EBUSY;
	}

	rc = tsic_xx6_get_data_bits(config, &data_bits, frame);
	if (rc != 0) {
		return rc;
	}

	tsic_xx6_get_value(config, data, data_bits);

	return 0;
}

static int tsic_xx6_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct tsic_xx6_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	*val = data->val;

	return 0;
}

static DEVICE_API(sensor, tsic_xx6_driver_api) = {
	.sample_fetch = tsic_xx6_sample_fetch,
	.channel_get = tsic_xx6_channel_get,
};

static int tsic_xx6_get_frame_cycles(const struct tsic_xx6_config *config, uint64_t *frame_cycles)
{
	uint64_t tmp;
	int rc;

	rc = pwm_get_cycles_per_sec(config->pwm.dev, config->pwm.channel, &tmp);
	if (rc != 0) {
		return rc;
	}

	if (u64_mul_overflow(tmp, FRAME_BIT_PERIOD_US, &tmp)) {
		return -ERANGE;
	}

	*frame_cycles = tmp / USEC_PER_SEC;

	return 0;
}

static int tsic_xx6_init(const struct device *dev)
{
	const struct tsic_xx6_config *config = dev->config;
	struct tsic_xx6_data *data = dev->data;
	int rc;

	if (!pwm_is_ready_dt(&config->pwm)) {
		return -ENODEV;
	}

	rc = tsic_xx6_get_frame_cycles(config, &data->frame_cycles);
	if (rc != 0) {
		return rc;
	}

	rc = pwm_configure_capture(config->pwm.dev, config->pwm.channel,
				   config->pwm.flags | PWM_CAPTURE_TYPE_BOTH |
					   PWM_CAPTURE_MODE_CONTINUOUS,
				   tsic_xx6_pwm_callback, (void *)dev);
	if (rc != 0) {
		return rc;
	}

	tsic_xx6_buf_reset(data);

	rc = pwm_enable_capture(config->pwm.dev, config->pwm.channel);
	if (rc != 0) {
		return rc;
	}

	return 0;
}

#define TSIC_XX6_DEVICE(n)                                                                         \
                                                                                                   \
	static struct tsic_xx6_data tsic_xx6_data_##n;                                             \
                                                                                                   \
	static const struct tsic_xx6_config tsic_xx6_config_##n = {                                \
		.pwm = PWM_DT_SPEC_INST_GET(n),                                                    \
		.lower_temperature_limit = (int8_t)DT_INST_PROP(n, lower_temperature_limit),       \
		.higher_temperature_limit = DT_INST_PROP(n, higher_temperature_limit),             \
		.data_bits = DT_INST_PROP(n, data_bits),                                           \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, &tsic_xx6_init, NULL, &tsic_xx6_data_##n,                  \
				     &tsic_xx6_config_##n, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &tsic_xx6_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TSIC_XX6_DEVICE)
