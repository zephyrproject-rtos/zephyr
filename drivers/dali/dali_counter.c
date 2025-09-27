/*
 * Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zephyr/sys/util.h"
#define DT_DRV_COMPAT zephyr_dali_counter

#include <zephyr/drivers/dali.h> /* api */

#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dali_low_level, CONFIG_DALI_LOW_LEVEL_LOG_LEVEL);

#include "timings.h" /* timing constants from DALI standard */

#define DALI_MAX_DATA_LENGTH (32U)
/* start bit, 32 data bits, 1 stop bit */
#define TICKS_ARRAY_SIZE     (2U + DALI_MAX_DATA_LENGTH * 2U)
#define EXTEND_CORRUPT_PHASE 4
#define TX_CORRUPT_BIT_US    ((DALI_TX_CORRUPT_BIT_MAX_US + DALI_TX_CORRUPT_BIT_MIN_US) / 2)
#define DALI_TX_IDLE         false
#define DALI_TX_ACTIVE       true
#define DALI_RX_IDLE         false
#define DALI_RX_ACTIVE       true

// TODO(sven) grey areas are system specific -> move to devicetree
#define GREY_AREA_BITS_US  (20U)
#define GREY_AREA_FRAME_US (800U)

static struct k_work_q dali_counter_work_queue;
static K_KERNEL_STACK_DEFINE(dali_counter_work_queue_stack, CONFIG_DALI_COUNTER_STACK_SIZE);

struct dali_counter_tx_callback {
	dali_tx_callback_t function;
	void *user_data;
};

struct dali_counter_tx_alarms {
	uint32_t ticks[TICKS_ARRAY_SIZE];
	uint_fast8_t index_next;
	uint_fast8_t index_max;
	bool state_now;
	bool output;
};

struct dali_counter_tx_timings {
	uint32_t half_bit_ticks;
	uint32_t full_bit_ticks;
	uint32_t corrupt_bit_ticks;
	int32_t flank_shift_ticks;
	uint32_t start_offset_ticks;
	uint32_t top_ticks;
};

struct dali_counter_tx_data {
	struct dali_counter_tx_callback cb;
	struct dali_counter_tx_alarms alarms;
	struct dali_counter_tx_timings timing;
	bool is_collision_detection_active;
};

enum rx_state {
	IDLE = 0,
	START_BIT_START,
	START_BIT_INSIDE,
	DATA_BIT_START,
	DATA_BIT_INSIDE,
	ERROR_IN_FRAME,
	//	DESTROY_FRAME,
	//	BUS_LOW,
	//	BUS_FAILURE_DETECT,
	//	TRANSMIT_BACKFRAME,
	//	STOPBIT_BACKFRAME,
};

enum rx_event {
	CAPTURE,
	STOPBIT,
};

struct dali_counter_rx_callback {
	dali_rx_callback_t function;
	void *user_data;
};

struct dali_counter_rx_timings {
	uint32_t half_bit_max_ticks;
	uint32_t half_bit_min_ticks;
	uint32_t full_bit_max_ticks;
	uint32_t full_bit_min_ticks;
	uint32_t stopbit_ticks;
	uint32_t top_ticks;
	int32_t flank_shift_ticks;
};

struct dali_counter_rx_data {
	struct dali_counter_rx_callback cb;
	struct gpio_callback gpio_cb;
	struct k_work work;
	struct dali_counter_rx_timings timing;
	bool last_data_bit;
	uint8_t payload_length;
	uint32_t data;
	uint32_t last_edge_ticks;
	uint32_t edge_ticks;
	enum rx_event event;
	enum rx_state status;
};

struct dali_counter_data {
	const struct device *dev;
	struct dali_counter_tx_data tx;
	struct dali_counter_rx_data rx;
};

struct dali_counter_config {
	const struct device *counter;
	const struct gpio_dt_spec tx_pin;
	const struct gpio_dt_spec rx_pin;
	uint8_t chan_id_tx;
	uint8_t chan_id_collision;
	uint8_t chan_id_stop;
	int32_t tx_flank_shift_us;
	int32_t rx_flank_shift_us;
	unsigned int tx_rx_propagation_max_us;
};

static void dali_counter_next_alarm(uint32_t *counter_now, const uint32_t alarm_delay, bool flank,
				    const struct dali_counter_tx_timings timing)
{
	*counter_now = *counter_now + alarm_delay;
	if (flank) {
		*counter_now -= timing.flank_shift_ticks;
	} else {
		*counter_now += timing.flank_shift_ticks;
	}
	if (*counter_now > timing.top_ticks) {
		*counter_now = *counter_now - timing.top_ticks;
	}
}

static int dali_counter_calculate_counts(const struct device *dev, const struct dali_frame *frame)
{
	uint_fast8_t length = 0;
	uint32_t payload = frame->data;
	const struct dali_counter_config *config = dev->config;
	struct dali_counter_data *data = dev->data;
	struct dali_counter_tx_alarms *alarms = &data->tx.alarms;
	int ret = 0;

	uint32_t counter_now;
	ret = counter_get_value(config->counter, &counter_now);
	if (ret != 0) {
		return ret;
	}
	dali_counter_next_alarm(&counter_now, data->tx.timing.start_offset_ticks, true,
				data->tx.timing);

	switch (frame->event_type) {
	case DALI_FRAME_CORRUPT:
		length = DALI_FRAME_BACKWARD_LENGTH;
		payload = 0;
		data->tx.is_collision_detection_active = false;
		return ret;
	case DALI_FRAME_BACKWARD:
		length = DALI_FRAME_BACKWARD_LENGTH;
		data->tx.is_collision_detection_active = false;
		break;
	case DALI_FRAME_GEAR:
		length = DALI_FRAME_GEAR_LENGTH;
		data->tx.is_collision_detection_active = true;
		break;
	case DALI_FRAME_DEVICE:
		length = DALI_FRAME_DEVICE_LENGTH;
		data->tx.is_collision_detection_active = true;
		break;
	default:
		return -EINVAL;
	}

	/* add start bit */
	alarms->state_now = true;
	alarms->index_max = 0;
	alarms->index_next = 0;
	alarms->ticks[alarms->index_max++] = counter_now;
	dali_counter_next_alarm(&counter_now, data->tx.timing.half_bit_ticks, false,
				data->tx.timing);
	alarms->ticks[alarms->index_max++] = counter_now;
	dali_counter_next_alarm(&counter_now, data->tx.timing.half_bit_ticks, true,
				data->tx.timing);
	alarms->ticks[alarms->index_max++] = counter_now;

	/* add data bits */
	bool current_state = true;
	for (int_fast8_t i = (length - 1); i >= 0; i--) {
		bool current_data = payload & (1 << i);
		if (current_state == current_data) {
			if (frame->event_type == DALI_FRAME_CORRUPT && i == EXTEND_CORRUPT_PHASE) {
				dali_counter_next_alarm(&counter_now,
							data->tx.timing.corrupt_bit_ticks,
							!current_data, data->tx.timing);
			} else {
				dali_counter_next_alarm(&counter_now,
							data->tx.timing.half_bit_ticks,
							!current_data, data->tx.timing);
			}
			alarms->ticks[alarms->index_max++] = counter_now;
			dali_counter_next_alarm(&counter_now, data->tx.timing.half_bit_ticks,
						current_data, data->tx.timing);
			alarms->ticks[alarms->index_max++] = counter_now;
		} else {
			counter_now = alarms->ticks[(alarms->index_max) - 2];
			dali_counter_next_alarm(&counter_now, data->tx.timing.full_bit_ticks,
						!current_data, data->tx.timing);
			alarms->ticks[(alarms->index_max) - 1] = counter_now;
			dali_counter_next_alarm(&counter_now, data->tx.timing.half_bit_ticks,
						current_data, data->tx.timing);
			alarms->ticks[alarms->index_max++] = counter_now;
			current_state = current_data;
		}
	}

	return 0;
}

static void dali_counter_tx_alarm_callback(const struct device *counter, uint8_t chan_id,
					   uint32_t ticks, void *user_data)
{
	const struct device *dev = user_data;
	const struct dali_counter_config *config = dev->config;
	struct dali_counter_data *data = dev->data;
	struct dali_counter_tx_alarms *alarms = &data->tx.alarms;

	/* toggle pin */
	if (alarms->index_next < alarms->index_max) {
		if (alarms->output == DALI_TX_IDLE) {
			alarms->output = DALI_TX_ACTIVE;
		} else {
			alarms->output = DALI_TX_IDLE;
		}
		gpio_pin_set_dt(&config->tx_pin, alarms->output);

		struct counter_alarm_cfg cfg = {
			.callback = dali_counter_tx_alarm_callback,
			.ticks = alarms->ticks[alarms->index_next++],
			.flags = COUNTER_ALARM_CFG_ABSOLUTE,
			.user_data = (void *)dev,
		};
		int ret = counter_set_channel_alarm(config->counter, config->chan_id_tx, &cfg);
		if (ret < 0) {
			LOG_ERR("Error %d setting the Tx alarm for %s to %u ticks", ret, dev->name,
				cfg.ticks);
			gpio_pin_set_dt(&config->tx_pin, DALI_TX_IDLE);
			alarms->output = DALI_TX_IDLE;
		}
	} else {
		gpio_pin_set_dt(&config->tx_pin, DALI_TX_IDLE);
		alarms->output = DALI_TX_IDLE;
	}
}

static void dali_counter_rx_reset(struct dali_counter_data *data)
{
	data->rx.status = IDLE;
	data->rx.last_data_bit = true;
	data->rx.data = 0;
	data->rx.payload_length = 0;
}

static void dali_counter_stopbit_alarm_callback(const struct device *counter, uint8_t chan_id,
						uint32_t ticks, void *user_data)
{
	const struct device *dev = user_data;
	struct dali_counter_data *data = dev->data;

	data->rx.event = STOPBIT;
	k_work_submit_to_queue(&dali_counter_work_queue, &data->rx.work);
}

static int dali_counter_restart_stopbit_alarm(const struct device *dev)
{
	const struct dali_counter_config *config = dev->config;
	struct dali_counter_data *data = dev->data;

	int ret = counter_cancel_channel_alarm(config->counter, config->chan_id_stop);
	if (ret < 0) {
		LOG_ERR("Error %d could not cancel stopbit alarm for %s", ret, dev->name);
		return ret;
	}

	struct counter_alarm_cfg cfg = {
		.callback = dali_counter_stopbit_alarm_callback,
		.ticks = data->rx.edge_ticks + data->rx.timing.stopbit_ticks,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
		.user_data = (void *)dev,
	};

	if (cfg.ticks > data->rx.timing.top_ticks) {
		cfg.ticks -= data->rx.timing.top_ticks;
	}

	ret = counter_set_channel_alarm(config->counter, config->chan_id_stop, &cfg);
	if (ret < 0) {
		LOG_ERR("Error %d setting the stopbit alarm for %s to %u ticks", ret, dev->name,
			cfg.ticks);
	}
	return ret;
}

static uint32_t dali_counter_get_time_difference_ticks(struct dali_counter_data *data,
						       bool flank_direction)
{
	uint32_t raw_difference_ticks = data->rx.edge_ticks - data->rx.last_edge_ticks;
	/* correct for roll-over of counter */
	if (raw_difference_ticks > data->rx.timing.top_ticks) {
		raw_difference_ticks += data->rx.timing.top_ticks;
	}
	/* correct for hardware transient time */
	if (data->rx.last_data_bit != flank_direction) {
		return raw_difference_ticks - data->rx.timing.flank_shift_ticks;
	}
	return raw_difference_ticks + data->rx.timing.flank_shift_ticks;
}

static bool dali_counter_is_valid_halfbit(struct dali_counter_data *data,
					  const uint32_t time_difference_ticks)
{
	if ((time_difference_ticks > data->rx.timing.half_bit_max_ticks) ||
	    (time_difference_ticks < data->rx.timing.half_bit_min_ticks)) {
		return false;
	}
	return true;
}

static bool dali_counter_is_valid_fullbit(struct dali_counter_data *data,
					  const uint32_t time_difference_ticks)
{
	if ((time_difference_ticks > data->rx.timing.full_bit_max_ticks) ||
	    (time_difference_ticks < data->rx.timing.full_bit_min_ticks)) {
		return false;
	}
	return true;
}

static void dali_counter_set_status(struct dali_counter_data *data, enum rx_state new)
{
	// allow change from error only into IDLE
	if (data->rx.status == ERROR_IN_FRAME) {
		if (new != IDLE) {
			return;
		}
	}
	data->rx.status = new;
}

static void dali_counter_add_bit_to_rx_data(struct dali_counter_data *data)
{
	data->rx.data = (data->rx.data << 1U) | (data->rx.last_data_bit ? (1U) : (0U));
	data->rx.payload_length++;
	if (data->rx.payload_length > DALI_MAX_BIT_PER_FRAME) {
		dali_counter_set_status(data, ERROR_IN_FRAME);
	}
}

static void dali_counter_process_start_timing(struct dali_counter_data *data)
{
	const uint32_t time_difference_ticks = dali_counter_get_time_difference_ticks(data, false);

	if (!dali_counter_is_valid_halfbit(data, time_difference_ticks)) {
		LOG_ERR("invalid start timing %u ticks, rx-status: %d, bit: %d",
			time_difference_ticks, data->rx.status, data->rx.payload_length);
		dali_counter_set_status(data, ERROR_IN_FRAME);
		return;
	}
	if (data->rx.status == DATA_BIT_START) {
		dali_counter_add_bit_to_rx_data(data);
		dali_counter_set_status(data, DATA_BIT_INSIDE);
		return;
	}
	dali_counter_set_status(data, START_BIT_INSIDE);
}

static void dali_counter_process_inside_timing(struct dali_counter_data *data)
{
	const uint32_t time_difference_ticks = dali_counter_get_time_difference_ticks(data, true);

	if (dali_counter_is_valid_halfbit(data, time_difference_ticks)) {
		dali_counter_set_status(data, DATA_BIT_START);
		return;
	}
	if (dali_counter_is_valid_fullbit(data, time_difference_ticks)) {
		data->rx.last_data_bit = !data->rx.last_data_bit;
		dali_counter_add_bit_to_rx_data(data);
		dali_counter_set_status(data, DATA_BIT_INSIDE);
		return;
	}
	LOG_ERR("invalid inside timing %u ticks, rx-status: %d, bit: %d", time_difference_ticks,
		data->rx.status, data->rx.payload_length);
	dali_counter_set_status(data, ERROR_IN_FRAME);
}

static void dali_counter_process_capture_event(const struct device *dev)
{
	const struct dali_counter_config *config = dev->config;
	struct dali_counter_data *data = dev->data;

	dali_counter_restart_stopbit_alarm(dev);

	switch (data->rx.status) {
	case IDLE:
		if (gpio_pin_get(config->rx_pin.port, config->rx_pin.pin) == DALI_RX_ACTIVE) {
			dali_counter_set_status(data, START_BIT_START);
		}
		break;
	case START_BIT_START:
	case DATA_BIT_START:
		dali_counter_process_start_timing(data);
		break;
	case START_BIT_INSIDE:
	case DATA_BIT_INSIDE:
		dali_counter_process_inside_timing(data);
		break;
	case ERROR_IN_FRAME:
		break;
	default:
		__ASSERT(false, "invalid status");
	}
}

static void dali_counter_process_stopbit_event(const struct device *dev)
{
	struct dali_counter_data *data = dev->data;
	struct dali_frame frame = {
		.data = data->rx.data,
		.event_type = DALI_EVENT_NONE,
	};
	bool rx_cb = false;

	switch (data->rx.status) {
	case START_BIT_START:
	case START_BIT_INSIDE:
	case DATA_BIT_START:
	case DATA_BIT_INSIDE:
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
		rx_cb = true;
		dali_counter_rx_reset(data);
		break;
	case ERROR_IN_FRAME:
		frame.data = 0, frame.event_type = DALI_FRAME_CORRUPT;
		rx_cb = true;
		dali_counter_rx_reset(data);
		break;
	// case BUS_FAILURE_DETECT:
	// 	LOG_INF("bus failure");
	// 	frame.data = 0, frame.event_type = DALI_EVENT_BUS_FAILURE;
	// 	err = -EAGAIN;
	// 	rx_cb = true;
	// 	break;
	case IDLE:
		LOG_INF("bus idle");
		frame.data = 0, frame.event_type = DALI_EVENT_BUS_IDLE;
		rx_cb = true;
		dali_counter_rx_reset(data);
		break;
	default:
		__ASSERT(false, "invalid state");
	};
	if (data->rx.cb.function && rx_cb) {
		(data->rx.cb.function)(dev, frame, data->rx.cb.user_data);
	};
}

static void dali_counter_handle_work_queue(struct k_work *item)
{
	struct dali_counter_data *data = CONTAINER_OF(item, struct dali_counter_data, rx.work);

	switch (data->rx.event) {
	// case CAPTURE:
	// 	dali_counter_process_capture_event(data->dev);
	// 	break;
	case STOPBIT:
		dali_counter_process_stopbit_event(data->dev);
		break;
	default:
		__ASSERT(false, "invalid event type");
	}
}

static void dali_counter_rx_irq_handler(const struct device *port, struct gpio_callback *cb,
					gpio_port_pins_t pins)
{
	struct dali_counter_data *data = CONTAINER_OF(cb, struct dali_counter_data, rx.gpio_cb);
	const struct dali_counter_config *config = data->dev->config;

	data->rx.last_edge_ticks = data->rx.edge_ticks;
	counter_get_value(config->counter, &data->rx.edge_ticks);
	dali_counter_process_capture_event(data->dev);
}

static int dali_counter_receive(const struct device *dev, dali_rx_callback_t callback,
				void *user_data)
{
	struct dali_counter_data *data = dev->data;

	LOG_DBG("Register receive callback.");
	data->rx.cb.function = callback;
	data->rx.cb.user_data = user_data;

	return 0;
}

static int dali_counter_send(const struct device *dev, const struct dali_frame *frame,
			     dali_tx_callback_t callback, void *user_data)
{
	const struct dali_counter_config *config = dev->config;
	struct dali_counter_data *data = dev->data;
	struct dali_counter_tx_alarms *alarms = &data->tx.alarms;

	switch (frame->event_type) {
	case DALI_EVENT_NONE:
		break;
	case DALI_FRAME_GEAR:
	case DALI_FRAME_DEVICE:
	case DALI_FRAME_FIRMWARE:
	case DALI_FRAME_CORRUPT:
	case DALI_FRAME_BACKWARD:
		break;
	default:
		return -EINVAL;
	}

	int ret = dali_counter_calculate_counts(dev, frame);
	if (ret < 0) {
		return ret;
	}

	struct counter_alarm_cfg cfg = {
		.callback = dali_counter_tx_alarm_callback,
		.ticks = alarms->ticks[alarms->index_next++],
		.flags = COUNTER_ALARM_CFG_ABSOLUTE | COUNTER_CONFIG_INFO_COUNT_UP,
		.user_data = (void *)dev,
	};
	alarms->output = DALI_TX_IDLE;
	ret = counter_set_channel_alarm(config->counter, config->chan_id_tx, &cfg);

	return ret;
}

static void dali_counter_abort(const struct device *dev)
{
}

static int dali_counter_init_counter(const struct device *dev)
{
	const struct dali_counter_config *config = dev->config;
	struct dali_counter_data *data = dev->data;

	if (!device_is_ready(config->counter)) {
		LOG_ERR("Counter %s for device %s is not ready", config->counter->name, dev->name);
		return -ENODEV;
	}

	const uint32_t frequency = counter_get_frequency(config->counter);
	if (frequency < USEC_PER_SEC) {
		LOG_ERR("Counter frequency too low (%d Hz), need at least 1 MHz", frequency);
		return -ENODEV;
	}

	/* calculate tx timings in ticks */
	data->tx.timing.half_bit_ticks = counter_us_to_ticks(config->counter, DALI_TX_HALF_BIT_US);
	data->tx.timing.full_bit_ticks = counter_us_to_ticks(config->counter, DALI_TX_FULL_BIT_US);
	data->tx.timing.corrupt_bit_ticks = counter_us_to_ticks(config->counter, TX_CORRUPT_BIT_US);
	if (config->tx_flank_shift_us < 0) {
		data->tx.timing.flank_shift_ticks =
			-counter_us_to_ticks(config->counter, -config->tx_flank_shift_us);
	} else {
		data->tx.timing.flank_shift_ticks =
			counter_us_to_ticks(config->counter, config->tx_flank_shift_us);
	}
	data->tx.timing.start_offset_ticks = counter_us_to_ticks(config->counter, 400);
	data->tx.timing.top_ticks = counter_get_top_value(config->counter);

	/* calculate rx timing in ticks */
	data->rx.timing.half_bit_min_ticks = counter_us_to_ticks(
		config->counter, DALI_RX_BIT_TIME_HALF_MIN_US - GREY_AREA_BITS_US);
	data->rx.timing.half_bit_max_ticks = counter_us_to_ticks(
		config->counter, DALI_RX_BIT_TIME_HALF_MAX_US + GREY_AREA_BITS_US);
	data->rx.timing.full_bit_min_ticks = counter_us_to_ticks(
		config->counter, DALI_RX_BIT_TIME_FULL_MIN_US - GREY_AREA_BITS_US);
	data->rx.timing.full_bit_max_ticks = counter_us_to_ticks(
		config->counter, DALI_RX_BIT_TIME_FULL_MAX_US + GREY_AREA_BITS_US);
	data->rx.timing.stopbit_ticks =
		counter_us_to_ticks(config->counter, DALI_RX_BIT_TIME_STOP_US - GREY_AREA_FRAME_US);
	if (config->rx_flank_shift_us < 0) {
		data->rx.timing.flank_shift_ticks =
			-counter_us_to_ticks(config->counter, -config->rx_flank_shift_us);
	} else {
		data->rx.timing.flank_shift_ticks =
			counter_us_to_ticks(config->counter, config->rx_flank_shift_us);
	}
	data->rx.timing.top_ticks = counter_get_top_value(config->counter);

	/* start the counter */
	return counter_start(config->counter);
}

static int dali_counter_init_tx(const struct device *dev)
{
	const struct dali_counter_config *config = dev->config;
	struct dali_counter_data *data = dev->data;
	struct dali_counter_tx_alarms *alarms = &data->tx.alarms;

	if (!gpio_is_ready_dt(&config->tx_pin)) {
		LOG_ERR("Tx pin for decice %s is not ready", dev->name);
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&config->tx_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Can not configure Tx pin for device %s", dev->name);
		return ret;
	}

	ret = gpio_pin_set_dt(&config->tx_pin, DALI_TX_IDLE);
	if (ret < 0) {
		LOG_ERR("Can not set Tx pin for device %s", dev->name);
		return ret;
	}
	alarms->output = DALI_TX_IDLE;

	return ret;
}

static int dali_counter_init_rx(const struct device *dev)
{
	const struct dali_counter_config *config = dev->config;
	struct dali_counter_data *data = dev->data;

	if (!gpio_is_ready_dt(&config->rx_pin)) {
		LOG_ERR("Rx pin for device %s is not ready", dev->name);
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&config->rx_pin, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Can not configure Rx pin for device %s", dev->name);
		return ret;
	}

	gpio_init_callback(&data->rx.gpio_cb, dali_counter_rx_irq_handler, BIT(config->rx_pin.pin));

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
	return ret;
}

static int dali_counter_init_workqueue(const struct device *dev)
{
	struct dali_counter_data *data = dev->data;

	const struct k_work_queue_config cfg = {
		.name = "DALI work",
		.no_yield = true,
		.essential = false,
	};
	k_work_queue_start(&dali_counter_work_queue, dali_counter_work_queue_stack,
			   K_KERNEL_STACK_SIZEOF(dali_counter_work_queue_stack),
			   CONFIG_DALI_COUNTER_PRIORITY, &cfg);
	k_work_init(&data->rx.work, dali_counter_handle_work_queue);

	return 0;
}

static int dali_counter_init(const struct device *dev)
{
	struct dali_counter_data *data = dev->data;

	/* connect to device */
	data->dev = dev;
	dali_counter_rx_reset(data);
	int ret = dali_counter_init_workqueue(dev);

	if (ret < 0) {
		return ret;
	}

	ret = dali_counter_init_tx(dev);
	if (ret < 0) {
		return ret;
	}

	ret = dali_counter_init_counter(dev);
	if (ret < 0) {
		return ret;
	}

	return dali_counter_init_rx(dev);
}

static DEVICE_API(dali, dali_counter_driver_api) = {
	.receive = dali_counter_receive,
	.send = dali_counter_send,
	.abort = dali_counter_abort,
};

#define DALI_COUNTER_INIT(idx)                                                                     \
	static struct dali_counter_data dali_counter_data_##idx = {0};                             \
	static const struct dali_counter_config dali_counter_config_##idx = {                      \
		.counter = DEVICE_DT_GET(DT_INST_PHANDLE(idx, counter)),                           \
		.chan_id_tx = DT_INST_PROP_OR(idx, chan_id_tx, 0),                                 \
		.chan_id_collision = DT_INST_PROP_OR(idx, chan_id_collision, 1),                   \
		.chan_id_stop = DT_INST_PROP_OR(idx, chan_id_stop, 2),                             \
		.rx_pin = GPIO_DT_SPEC_INST_GET(idx, rx_gpios),                                    \
		.tx_pin = GPIO_DT_SPEC_INST_GET(idx, tx_gpios),                                    \
		.tx_flank_shift_us = DT_INST_PROP_OR(idx, tx_flank_shift_us, 0),                   \
		.rx_flank_shift_us = DT_INST_PROP_OR(idx, rx_flank_shift_us, 0),                   \
		.tx_rx_propagation_max_us = DT_INST_PROP_OR(idx, tx_rx_propagation_max_us, 200),   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, dali_counter_init, NULL, &dali_counter_data_##idx,              \
			      &dali_counter_config_##idx, POST_KERNEL,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dali_counter_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DALI_COUNTER_INIT);
