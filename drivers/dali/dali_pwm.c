/*
 * Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_dali_pwm

#include <zephyr/drivers/dali.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dali_low_level, CONFIG_DALI_LOW_LEVEL_LOG_LEVEL);

/* timing constants from DALI standard */
#include "timings.h"

#define DALI_RX_IDLE   false
#define DALI_RX_ACTIVE true

static struct k_work_q dali_pwm_work_queue;
static K_KERNEL_STACK_DEFINE(dali_pwm_work_queue_stack, CONFIG_DALI_PWM_STACK_SIZE);

enum pwm_states {
	NONE,  /* Disable sending. */
	LH,    /* 2 half bits long, next same as current */
	LHH,   /* 3 half bits long, current 1, next 0, next after 0 */
	LLH,   /* 3 half bits long, current 0, next 1, next after 1 */
	LLHH,  /* 4 half bits long, 3 bit toggle */
	LLLLH, /* 5 half bits long, invalid sequence for corrupted frames */
};

enum dali_pwm_rx_state {
	DALI_PWM_RX_IDLE,
	DALI_PWM_RX_START_BIT_START,
	DALI_PWM_RX_START_BIT_INSIDE,
	DALI_PWM_RX_DATA_BIT_START,
	DALI_PWM_RX_DATA_BIT_INSIDE,
	DALI_PWM_RX_ERROR_IN_FRAME,
	DALI_PWM_RX_BUS_LOW,
	DALI_PWM_RX_BUS_FAILURE,
};

enum dali_pwm_tx_state {
	DALI_PWM_TX_IDLE,
	DALI_PWM_TX_START,
	DALI_PWM_TX_INSIDE,
	DALI_PWM_TX_FINISH,
	DALI_PWM_TX_STOP,
	DALI_PWM_TX_DESTROY,
};

struct dali_pwm_limits {
	uint32_t min;
	uint32_t max;
};

/** bit times converted from us to counter ticks. To reduce stress on pwm interrupt. */
struct dali_pwm_rx_timings {
	struct dali_pwm_limits half_bit;
	struct dali_pwm_limits full_bit;
	struct dali_pwm_limits destroy_1;
	struct dali_pwm_limits destroy_2;
	struct dali_pwm_limits destroy_3;
	uint32_t stop_bit;
	uint32_t failure;
	int32_t flank_shift;
	uint32_t top;
	uint32_t tx_rx_propagation;
};

struct dali_pwm_tx_timings {
	uint32_t half_bit;
	int32_t flank_shift;
};

struct dali_pwm_frame {
	/**
	 * DALI frame will be split into a sequence of PWM settings and put
	 * into this struct. Each PWM setting must be send out one after another
	 * for this to work without disruptions.
	 */
	enum pwm_states signals[DALI_MAX_BIT_PER_FRAME + 2]; /* plus startbit, plus finish */
	uint8_t position; /**< Where we are on sending out the entries */
};

struct dali_pwm_tx_callback {
	dali_tx_callback_t function;
	void *user_data;
};

struct dali_pwm_rx_callback {
	dali_rx_callback_t function;
	void *user_data;
};

struct dali_pwm_tx_data {
	struct dali_pwm_tx_callback cb;
	struct dali_pwm_tx_timings pwm_ticks;
	struct dali_pwm_tx_timings counter_ticks;
	struct dali_pwm_frame pwm_frame;
	enum dali_pwm_tx_state status;
	bool collision_detection;
	int err_code;
};

struct dali_pwm_rx_data {
	struct dali_pwm_rx_callback cb;
	struct dali_pwm_rx_timings ticks;
	struct k_work work;
	struct gpio_callback gpio_cb;
	bool last_data_bit;
	uint8_t payload_length;
	uint32_t data;
	uint32_t last_edge_ticks;
	uint32_t edge_ticks;
	uint32_t inside_ticks;
	uint32_t start_ticks;
	enum dali_pwm_rx_state status;
};

struct dali_pwm_data {
	/** DALI device */
	const struct device *dev;
	struct dali_pwm_tx_data tx;
	struct dali_pwm_rx_data rx;
};

struct dali_pwm_config {
	uint32_t time;
	const struct device *rx_counter;
	const struct pwm_dt_spec tx_pwm;
	const struct gpio_dt_spec rx_pin;
	uint8_t chan_id_rx;
	uint8_t chan_id_tx;
	int32_t tx_shift_us;
	int32_t rx_shift_us;
	uint32_t tx_rx_propagation_max_us;
	uint32_t rx_max_latency_us;
	uint32_t rx_grey_area_us;
};

static void dali_pwm_execute_rx_callback(const struct dali_pwm_data *data, struct dali_frame frame)
{
	if (data->rx.cb.function) {
		(data->rx.cb.function)(data->dev, frame, data->rx.cb.user_data);
	}
}

static void dali_pwm_execute_tx_callback(const struct dali_pwm_data *data)
{
	if (data->tx.cb.function) {
		(data->tx.cb.function)(data->dev, data->tx.err_code, data->tx.cb.user_data);
	}
}

static void dali_pwm_rx_reset(struct dali_pwm_data *data)
{
	data->rx.status = DALI_PWM_RX_IDLE;
	data->rx.last_data_bit = true;
	data->rx.data = 0;
	data->rx.payload_length = 0;
}

static void dali_pwm_rx_alarm_callback(const struct device *counter, uint8_t chan_id,
				       uint32_t ticks, void *user_data)
{
	struct dali_pwm_data *data = user_data;

	k_work_submit_to_queue(&dali_pwm_work_queue, &data->rx.work);
}

static int dali_pwm_restart_rx_alarm(struct dali_pwm_data *data,
				     const struct dali_pwm_config *config, uint32_t absolute_ticks)
{
	int ret = counter_cancel_channel_alarm(config->rx_counter, config->chan_id_rx);

	if (ret < 0) {
		LOG_ERR("Error %d could not cancel rx alarm for %s", ret, data->dev->name);
		return ret;
	}

	struct counter_alarm_cfg cfg = {
		.callback = dali_pwm_rx_alarm_callback,
		.ticks = absolute_ticks,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
		.user_data = data,
	};

	if (cfg.ticks > data->rx.ticks.top) {
		cfg.ticks -= data->rx.ticks.top;
	}

	ret = counter_set_channel_alarm(config->rx_counter, config->chan_id_rx, &cfg);
	if (ret < 0) {
		LOG_ERR("Error %d setting the rx alarm for %s to %u ticks", ret, data->dev->name,
			cfg.ticks);
	}
	return ret;
}

static int dali_pwm_restart_stopbit_alarm(struct dali_pwm_data *data,
					  const struct dali_pwm_config *config)
{
	uint32_t absolute_ticks = data->rx.edge_ticks + data->rx.ticks.stop_bit;

	return dali_pwm_restart_rx_alarm(data, config, absolute_ticks);
}

static uint32_t dali_pwm_get_time_difference_ticks(struct dali_pwm_data *data, bool flank_direction)
{
	uint32_t raw_difference_ticks = data->rx.edge_ticks - data->rx.last_edge_ticks;
	/* correct for roll-over of counter */
	if (raw_difference_ticks > data->rx.ticks.top) {
		raw_difference_ticks += data->rx.ticks.top;
	}
	/* correct for hardware transient time */
	if (data->rx.last_data_bit != flank_direction) {
		return raw_difference_ticks - data->rx.ticks.flank_shift;
	}
	return raw_difference_ticks + data->rx.ticks.flank_shift;
}

static bool dali_pwm_in_limits(const struct dali_pwm_limits limits, uint32_t value)
{
	if ((value > limits.max) || (value < limits.min)) {
		return false;
	}
	return true;
}

static void dali_pwm_set_status(struct dali_pwm_data *data, enum dali_pwm_rx_state new)
{
	/* allow change from error only into IDLE */
	if (data->rx.status == DALI_PWM_RX_ERROR_IN_FRAME && new != DALI_PWM_RX_IDLE) {
		return;
	}
	data->rx.status = new;
}

static void dali_pwm_add_bit_to_rx_data(struct dali_pwm_data *data)
{
	data->rx.data = (data->rx.data << 1U) | (data->rx.last_data_bit ? (1U) : (0U));
	data->rx.payload_length++;
	if (data->rx.payload_length > DALI_MAX_BIT_PER_FRAME) {
		dali_pwm_set_status(data, DALI_PWM_RX_ERROR_IN_FRAME);
	}
}

static void dali_pwm_process_start_timing(struct dali_pwm_data *data)
{
	const uint32_t time_difference_ticks = dali_pwm_get_time_difference_ticks(data, false);

	if (!dali_pwm_in_limits(data->rx.ticks.half_bit, time_difference_ticks)) {
		LOG_ERR("invalid start timing %u ticks, rx-status: %d, bit: %d",
			time_difference_ticks, data->rx.status, data->rx.payload_length);
		dali_pwm_set_status(data, DALI_PWM_RX_ERROR_IN_FRAME);
		return;
	}
	if (data->rx.status == DALI_PWM_RX_DATA_BIT_START) {
		dali_pwm_add_bit_to_rx_data(data);
		dali_pwm_set_status(data, DALI_PWM_RX_DATA_BIT_INSIDE);
		return;
	}
	dali_pwm_set_status(data, DALI_PWM_RX_START_BIT_INSIDE);
}

static void dali_pwm_process_inside_timing(struct dali_pwm_data *data)
{
	const uint32_t time_difference_ticks = dali_pwm_get_time_difference_ticks(data, true);

	if (dali_pwm_in_limits(data->rx.ticks.half_bit, time_difference_ticks)) {
		dali_pwm_set_status(data, DALI_PWM_RX_DATA_BIT_START);
		return;
	}
	if (dali_pwm_in_limits(data->rx.ticks.full_bit, time_difference_ticks)) {
		data->rx.last_data_bit = !data->rx.last_data_bit;
		dali_pwm_add_bit_to_rx_data(data);
		dali_pwm_set_status(data, DALI_PWM_RX_DATA_BIT_INSIDE);
		return;
	}
	LOG_ERR("invalid inside timing %u ticks, rx-status: %d, bit: %d", time_difference_ticks,
		data->rx.status, data->rx.payload_length);
	dali_pwm_set_status(data, DALI_PWM_RX_ERROR_IN_FRAME);
}

static void dali_pwm_process_capture_event(struct dali_pwm_data *data,
					   const struct dali_pwm_config *config)
{
	dali_pwm_restart_stopbit_alarm(data, config);

	switch (data->rx.status) {
	case DALI_PWM_RX_IDLE:
		if (gpio_pin_get(config->rx_pin.port, config->rx_pin.pin) == DALI_RX_ACTIVE) {
			dali_pwm_set_status(data, DALI_PWM_RX_START_BIT_START);
		}
		break;
	case DALI_PWM_RX_START_BIT_START:
	case DALI_PWM_RX_DATA_BIT_START:
		dali_pwm_process_start_timing(data);
		break;
	case DALI_PWM_RX_START_BIT_INSIDE:
	case DALI_PWM_RX_DATA_BIT_INSIDE:
		dali_pwm_process_inside_timing(data);
		break;
	case DALI_PWM_RX_BUS_LOW:
	case DALI_PWM_RX_BUS_FAILURE:
		if (gpio_pin_get(config->rx_pin.port, config->rx_pin.pin) == DALI_RX_IDLE) {
			dali_pwm_set_status(data, DALI_PWM_RX_IDLE);
		}
		break;
	case DALI_PWM_RX_ERROR_IN_FRAME:
		break;
	default:
		__ASSERT(false, "invalid status");
	}
}

static void dali_pwm_process_stopbit_event(const struct device *dev)
{
	struct dali_pwm_data *data = dev->data;
	const struct dali_pwm_config *config = data->dev->config;
	struct dali_frame frame = {
		.data = data->rx.data,
		.event_type = DALI_EVENT_NONE,
	};

	if (gpio_pin_get(config->rx_pin.port, config->rx_pin.pin) == DALI_RX_ACTIVE) {
		switch (data->rx.status) {
		case DALI_PWM_RX_BUS_LOW:
		case DALI_PWM_RX_BUS_FAILURE:
			break;
		default: {
			uint32_t absolute_ticks = data->rx.edge_ticks + data->rx.ticks.failure;

			dali_pwm_set_status(data, DALI_PWM_RX_BUS_LOW);
			dali_pwm_restart_rx_alarm(data, config, absolute_ticks);
			return;
		}
		}
	}

	switch (data->rx.status) {
	case DALI_PWM_RX_START_BIT_START:
	case DALI_PWM_RX_START_BIT_INSIDE:
	case DALI_PWM_RX_DATA_BIT_START:
	case DALI_PWM_RX_DATA_BIT_INSIDE:
		LOG_INF("{%08x:%02x %08x}", k_uptime_get_32(), data->rx.payload_length,
			data->rx.data);
		switch (data->rx.payload_length) {
		case DALI_FRAME_BACKWARD_LENGTH:
			frame.event_type = DALI_FRAME_BACKWARD;
			break;
		case DALI_FRAME_GEAR_LENGTH:
			frame.event_type = DALI_FRAME_GEAR;
			break;
		case DALI_FRAME_DEVICE_LENGTH:
			frame.event_type = DALI_FRAME_DEVICE;
			break;
		case DALI_FRAME_UPDATE_LENGTH:
			frame.event_type = DALI_FRAME_FIRMWARE;
			break;
		default:
			LOG_INF("invalid frame length %d bits", data->rx.payload_length);
			frame.data = 0, frame.event_type = DALI_FRAME_CORRUPT;
		}
		dali_pwm_rx_reset(data);
		break;
	case DALI_PWM_RX_ERROR_IN_FRAME:
		frame.data = 0, frame.event_type = DALI_FRAME_CORRUPT;
		dali_pwm_rx_reset(data);
		break;
	case DALI_PWM_RX_BUS_LOW:
	case DALI_PWM_RX_BUS_FAILURE:
		frame.data = 0, frame.event_type = DALI_EVENT_BUS_FAILURE;
		dali_pwm_set_status(data, DALI_PWM_RX_BUS_FAILURE);
		break;
	case DALI_PWM_RX_IDLE:
		frame.data = 0, frame.event_type = DALI_EVENT_BUS_IDLE;
		dali_pwm_rx_reset(data);
		break;
	default:
		__ASSERT(false, "invalid state");
	}
	if (data->tx.status == DALI_PWM_TX_FINISH) {
		dali_pwm_execute_tx_callback(data);
		data->tx.status = DALI_PWM_TX_IDLE;
	}
	dali_pwm_execute_rx_callback(data, frame);
}

/* forward declaration of alarm callback */
static void dali_pwm_tx_alarm_callback(const struct device *counter, uint8_t chan_id,
				       uint32_t ticks, void *user_data);

static int dali_pwm_set_cycles(const struct device *dev, const struct pwm_dt_spec *spec,
			       enum pwm_states state)
{
	const struct dali_pwm_config *config = dev->config;
	struct dali_pwm_data *data = dev->data;
	uint8_t period = 0, pulse = 0;
	int ret = 0;

	switch (state) {
	case LH:
		period = 2;
		pulse = 1;
		break;
	case LHH:
		period = 3;
		pulse = 1;
		break;
	case LLH:
		period = 3;
		pulse = 2;
		break;
	case LLHH:
		period = 4;
		pulse = 2;
		break;
	case LLLLH:
		period = 5;
		pulse = 4;
		break;
	case NONE:
	default:
		break;
	}

	const uint32_t pwm_period = period * data->tx.pwm_ticks.half_bit;
	uint32_t pwm_pulse = pulse * data->tx.pwm_ticks.half_bit;

	if (pulse) {
		pwm_pulse += data->tx.pwm_ticks.flank_shift;
	}
	ret = pwm_set_cycles(spec->dev, spec->channel, pwm_period, pwm_pulse, spec->flags);
	if (ret < 0) {
		LOG_ERR("Error %d setting pwm cycle for device %s", ret, data->dev->name);
	}

	if (period) {
		const uint32_t counter_period = (period * data->tx.counter_ticks.half_bit) +
						data->tx.counter_ticks.flank_shift;
		struct counter_alarm_cfg cfg = {
			.callback = dali_pwm_tx_alarm_callback,
			.ticks = counter_period,
			.flags = 0,
			.user_data = data,
		};

		ret = counter_set_channel_alarm(config->rx_counter, config->chan_id_tx, &cfg);
		if (ret < 0) {
			LOG_ERR("Error %d setting the tx next alarm for %s to %u ticks", ret,
				data->dev->name, cfg.ticks);
		}
	}

	return ret;
}

static void dali_pwm_tx_alarm_callback(const struct device *counter, uint8_t chan_id,
				       uint32_t ticks, void *user_data)
{
	struct dali_pwm_data *data = user_data;
	const struct dali_pwm_config *config = data->dev->config;

	const enum pwm_states next_pwm_state =
		data->tx.pwm_frame.signals[data->tx.pwm_frame.position++];

	dali_pwm_set_cycles(data->dev, &config->tx_pwm, next_pwm_state);
	if (next_pwm_state == NONE) {
		data->tx.status = DALI_PWM_TX_FINISH;
	} else {
		data->tx.status = DALI_PWM_TX_INSIDE;
	}
}

static void dali_pwm_rx_irq_handler(const struct device *port, struct gpio_callback *cb,
				    gpio_port_pins_t pins)
{
	struct dali_pwm_data *data = CONTAINER_OF(cb, struct dali_pwm_data, rx.gpio_cb);
	const struct dali_pwm_config *config = data->dev->config;

	data->rx.last_edge_ticks = data->rx.edge_ticks;
	counter_get_value(config->rx_counter, &data->rx.edge_ticks);
	dali_pwm_process_capture_event(data, config);
}

static void dali_pwm_handle_work_queue(struct k_work *item)
{
	struct dali_pwm_data *data = CONTAINER_OF(item, struct dali_pwm_data, rx.work);

	/* for now we just handle stopbit and Rx callback here */
	dali_pwm_process_stopbit_event(data->dev);
}

static void dali_pwm_generate_corrupt_frame(struct dali_pwm_frame *pwm)
{
	/* reset the frame buffer */
	*pwm = (struct dali_pwm_frame){0};

	/* Send all ones, except for the second, where we stretch the active state
	 * over the corrupt threshold
	 */
	uint8_t signal_length = 0;

	for (int i = 0; i < DALI_FRAME_BACKWARD_LENGTH + 1; i++) {
		pwm->signals[signal_length++] = i == 2 ? LLLLH : LH;
	}
}

/**
 * @brief construct PWM patterns for dali frame and return them.
 *
 */
static int dali_pwm_generate_frame(const struct dali_frame *frame, struct dali_pwm_frame *pwm)
{
	int frame_length = 0;

	switch (frame->event_type) {
	case DALI_FRAME_CORRUPT:
		dali_pwm_generate_corrupt_frame(pwm);
		return 0;
	case DALI_FRAME_BACKWARD:
		frame_length = DALI_FRAME_BACKWARD_LENGTH;
		break;
	case DALI_FRAME_GEAR:
		frame_length = DALI_FRAME_GEAR_LENGTH;
		break;
	case DALI_FRAME_DEVICE:
		frame_length = DALI_FRAME_DEVICE_LENGTH;
		break;
	case DALI_FRAME_FIRMWARE:
		frame_length = DALI_FRAME_UPDATE_LENGTH;
		break;
	default:
		return -EINVAL;
	}

	/* reset the frame buffer */
	*pwm = (struct dali_pwm_frame){0};

	/* we iterate over the frame in full and half bits. */
	int shift_half_bit = 0;
	uint8_t signal_length = 0;

	/* startbit is 1 and is added here. */
	bool current_bit = true;
	bool next_bit = !!(frame->data & (1 << (frame_length - 1)));
	bool next_next_bit = !!(frame->data & (1 << (frame_length - 2)));

	LOG_DBG("data=0x%08x and length=%dd", frame->data, frame_length);
	while (frame_length > 0) {
		if (current_bit == next_bit) {
			pwm->signals[signal_length] = LH;
			shift_half_bit += 2;
		} else if (current_bit == next_next_bit && shift_half_bit == 1) {
			pwm->signals[signal_length] = LLHH;
			shift_half_bit += 4;
		} else if (current_bit) {
			pwm->signals[signal_length] = LHH;
			shift_half_bit += 3;
		} else {
			pwm->signals[signal_length] = LLH;
			shift_half_bit += 3;
		}
		signal_length++;
		while (shift_half_bit > 1) {
			frame_length--;
			current_bit = next_bit;
			next_bit = next_next_bit;
			if (frame_length > 1) {
				next_next_bit = !!(frame->data & (1 << (frame_length - 2)));
			}
			/* no need for an else branch, we want the next_next_bit to be the
			 * same as the next_bit, which is already the case.
			 */
			shift_half_bit -= 2;
		}
	}

	/* check if there is a signal missing at the end */
	if (shift_half_bit || (current_bit && next_bit && frame_length == 0)) {
		/* We need to add the signal for the last bit.
		 * it could either be, that we are in the middle of the 0 bit and need
		 * to send the last half of the zero, or that we are missing a full 1.
		 *  Signal could also be LHH, it just needs a short low bit, we disable the
		 *  pwm after this and high level is disabled.
		 */
		pwm->signals[signal_length++] = LH;
	}

	/* add a finish */
	pwm->signals[signal_length] = NONE;

	return 0;
}

static int dali_pwm_us_to_cycles(int us, uint64_t cycles_per_sec)
{
	return ((int64_t)cycles_per_sec * (int64_t)us) / USEC_PER_SEC;
}

static int dali_pwm_receive(const struct device *dev, dali_rx_callback_t callback, void *user_data)
{
	struct dali_pwm_data *data = dev->data;

	LOG_DBG("Register receive callback.");
	data->rx.cb.function = callback;
	data->rx.cb.user_data = user_data;

	return 0;
}

static int dali_pwm_send(const struct device *dev, const struct dali_frame *frame,
			 dali_tx_callback_t callback, void *user_data)
{
	const struct dali_pwm_config *config = dev->config;
	struct dali_pwm_data *data = dev->data;
	int ret = 0;

	data->tx.collision_detection = true;
	data->tx.err_code = 0;
	switch (frame->event_type) {
	case DALI_EVENT_NONE:
		break;
	case DALI_FRAME_CORRUPT:
	case DALI_FRAME_BACKWARD:
		data->tx.collision_detection = false;
	case DALI_FRAME_GEAR:
	case DALI_FRAME_DEVICE:
	case DALI_FRAME_FIRMWARE:
		if (data->tx.collision_detection && data->rx.status != DALI_PWM_RX_IDLE) {
			ret = -EBUSY;
			break;
		}
		data->tx.cb.function = callback;
		data->tx.cb.user_data = user_data;
		data->tx.status = DALI_PWM_TX_INSIDE;
		data->tx.err_code = 0;
		ret = dali_pwm_generate_frame(frame, &data->tx.pwm_frame);
		if (ret < 0) {
			break;
		}
		ret = dali_pwm_set_cycles(
			dev, &config->tx_pwm,
			data->tx.pwm_frame.signals[data->tx.pwm_frame.position++]);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void dali_pwm_abort(const struct device *dev)
{
	const struct dali_pwm_config *config = dev->config;
	struct dali_pwm_data *data = dev->data;

	dali_pwm_set_cycles(dev, &config->tx_pwm, NONE);
	data->tx.status = DALI_PWM_TX_FINISH;
}

static int dali_pwm_rx_init(const struct device *dev)
{
	const struct dali_pwm_config *config = dev->config;
	struct dali_pwm_data *data = dev->data;
	int ret;

	/* configure the gpio pin */
	if (!gpio_is_ready_dt(&config->rx_pin)) {
		LOG_ERR("GPIO dev %s is not ready", dev->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->rx_pin, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Can not configure Rx pin for device %s", dev->name);
		return ret;
	}

	gpio_init_callback(&data->rx.gpio_cb, dali_pwm_rx_irq_handler, BIT(config->rx_pin.pin));
	ret = gpio_add_callback(config->rx_pin.port, &data->rx.gpio_cb);
	if (ret < 0) {
		LOG_ERR("Can not add Rx callback for device %s", dev->name);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->rx_pin, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("Can not configure Rx irq for device %s", dev->name);
		return ret;
	}

	/* configure the counter */
	if (!device_is_ready(config->rx_counter)) {
		LOG_ERR("Counter %s for device %s is not ready", config->rx_counter->name,
			dev->name);
		return -ENODEV;
	}

	const uint64_t cycles = counter_get_frequency(config->rx_counter);

	if (cycles < USEC_PER_SEC) {
		LOG_ERR("Counter frequency too low (%" PRIu64 "Hz), need at least 1 MHz", cycles);
		return -ENODEV;
	}

	/* convert microseconds timings into counter ticks */
	data->rx.ticks.half_bit.min = dali_pwm_us_to_cycles(
		DALI_RX_BIT_TIME_HALF_MIN_US - config->rx_max_latency_us, cycles);
	data->rx.ticks.half_bit.max = dali_pwm_us_to_cycles(
		DALI_RX_BIT_TIME_HALF_MAX_US + config->rx_max_latency_us, cycles);
	data->rx.ticks.full_bit.min = dali_pwm_us_to_cycles(
		DALI_RX_BIT_TIME_FULL_MIN_US - config->rx_max_latency_us, cycles);
	data->rx.ticks.full_bit.max = dali_pwm_us_to_cycles(
		DALI_RX_BIT_TIME_FULL_MAX_US + config->rx_max_latency_us, cycles);
	data->rx.ticks.stop_bit =
		dali_pwm_us_to_cycles(DALI_RX_BIT_TIME_STOP_US - config->rx_grey_area_us, cycles);
	data->rx.ticks.destroy_1.min = dali_pwm_us_to_cycles(DALI_TX_DESTROY_1_MIN_US, cycles);
	data->rx.ticks.destroy_1.max =
		dali_pwm_us_to_cycles(DALI_TX_DESTROY_1_MAX_US - config->rx_max_latency_us, cycles);
	data->rx.ticks.destroy_2.min =
		dali_pwm_us_to_cycles(DALI_TX_DESTROY_2_MIN_US + config->rx_max_latency_us, cycles);
	data->rx.ticks.destroy_2.max = dali_pwm_us_to_cycles(DALI_TX_DESTROY_2_MAX_US, cycles);
	data->rx.ticks.destroy_3.min = dali_pwm_us_to_cycles(DALI_TX_DESTROY_3_MIN_US, cycles);
	data->rx.ticks.failure = dali_pwm_us_to_cycles(DALI_FAILURE_CONDITION_US, cycles);
	data->rx.ticks.flank_shift = dali_pwm_us_to_cycles(config->rx_shift_us, cycles);
	data->rx.ticks.tx_rx_propagation =
		dali_pwm_us_to_cycles(config->tx_rx_propagation_max_us, cycles);
	data->rx.ticks.top = counter_get_top_value(config->rx_counter);

	/* set up the receive work queue */
	const struct k_work_queue_config cfg = {
		.name = "DALI Rx work",
		.no_yield = true,
		.essential = false,
	};
	k_work_queue_start(&dali_pwm_work_queue, dali_pwm_work_queue_stack,
			   K_KERNEL_STACK_SIZEOF(dali_pwm_work_queue_stack),
			   CONFIG_DALI_PWM_PRIORITY, &cfg);
	k_work_init(&data->rx.work, dali_pwm_handle_work_queue);

	return counter_start(config->rx_counter);
}

static int dali_pwm_tx_init(const struct device *dev)
{
	const struct dali_pwm_config *config = dev->config;
	struct dali_pwm_data *data = dev->data;
	uint64_t cycles;
	int ret;

	/* initialize pwm peripheral */
	if (!pwm_is_ready_dt(&config->tx_pwm)) {
		LOG_ERR("PWM device %s is not ready", dev->name);
		return -ENODEV;
	}

	/* set the pwm to idle */
	dali_pwm_set_cycles(dev, &config->tx_pwm, NONE);

	ret = pwm_get_cycles_per_sec(config->tx_pwm.dev, config->tx_pwm.channel, &cycles);
	if (ret < 0) {
		LOG_ERR("PWM device %s can not get cycles", dev->name);
		return ret;
	}
	if (cycles < 200000) {
		LOG_ERR("PWM timer is not accurate enough. Need at least 200kHz. Have "
			"%" PRIu64 " Hz",
			cycles);
		return -ERANGE;
	}

	/* convert micro second timings into pwm ticks */
	data->tx.pwm_ticks.half_bit = dali_pwm_us_to_cycles(DALI_TX_HALF_BIT_US, cycles);
	data->tx.pwm_ticks.flank_shift = dali_pwm_us_to_cycles(config->tx_shift_us, cycles);

	/* convert micro-second timings into counter ticks */
	cycles = counter_get_frequency(config->rx_counter);
	data->tx.counter_ticks.half_bit = dali_pwm_us_to_cycles(DALI_TX_HALF_BIT_US, cycles);
	data->tx.counter_ticks.flank_shift = dali_pwm_us_to_cycles(config->tx_shift_us, cycles);

	return 0;
}

static int dali_pwm_init_status(const struct device *dev)
{
	const struct dali_pwm_config *config = dev->config;
	struct dali_pwm_data *data = dev->data;

	dali_pwm_rx_reset(data);
	if (gpio_pin_get(config->rx_pin.port, config->rx_pin.pin) == DALI_RX_ACTIVE) {
		data->rx.status = DALI_PWM_RX_BUS_LOW;
		uint32_t counter_now;
		uint32_t absolute_ticks;

		counter_get_value(config->rx_counter, &counter_now);
		absolute_ticks = counter_now + data->rx.ticks.failure;
		dali_pwm_restart_rx_alarm(data, config, absolute_ticks);
	} else {
		data->rx.status = DALI_PWM_RX_IDLE;
	}
	return 0;
}

static int dali_pwm_init(const struct device *dev)
{
	struct dali_pwm_data *data = dev->data;
	int ret;

	/* connect to device */
	data->dev = dev;

	ret = dali_pwm_rx_init(dev);
	if (ret < 0) {
		return ret;
	}

	ret = dali_pwm_tx_init(dev);
	if (ret < 0) {
		return ret;
	}

	return dali_pwm_init_status(dev);
}

static DEVICE_API(dali, dali_pwm_driver_api) = {
	.receive = dali_pwm_receive,
	.send = dali_pwm_send,
	.abort = dali_pwm_abort,
};

#define DALI_PWM_INIT(idx)                                                                         \
	static struct dali_pwm_data dali_pwm_data_##idx = {0};                                     \
	static const struct dali_pwm_config dali_pwm_config_##idx = {                              \
		.rx_counter = DEVICE_DT_GET(DT_INST_PHANDLE(idx, counter)),                        \
		.chan_id_rx = DT_INST_PROP_OR(idx, chan_id_rx, 0),                                 \
		.chan_id_tx = DT_INST_PROP_OR(idx, chan_id_tx, 1),                                 \
		.rx_pin = GPIO_DT_SPEC_INST_GET_BY_IDX(idx, rx_gpios, 0),                          \
		.tx_pwm = PWM_DT_SPEC_GET_BY_IDX(DT_DRV_INST(idx), 0),                             \
		.tx_shift_us = DT_INST_PROP(idx, tx_flank_shift_us),                               \
		.rx_shift_us = DT_INST_PROP(idx, rx_flank_shift_us),                               \
		.tx_rx_propagation_max_us = DT_INST_PROP(idx, tx_rx_propagation_max_us),           \
		.rx_max_latency_us = DT_INST_PROP(idx, rx_max_latency_us),                         \
		.rx_grey_area_us = DT_INST_PROP(idx, rx_grey_area_us),                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, dali_pwm_init, NULL, &dali_pwm_data_##idx,                      \
			      &dali_pwm_config_##idx, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dali_pwm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DALI_PWM_INIT);
