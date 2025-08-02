/*
 * Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_dali_pwm

#include <zephyr/drivers/dali.h> /* api */

#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dali_low_level, CONFIG_DALI_LOW_LEVEL_LOG_LEVEL);

#include "timings.h" /* timing constants from DALI standard */

#define DALI_RX_GREY_AREA                    (20U)
#define DALI_TX_INTERFRAME_EXTRA             (800U)
#define MAX_HALFBIT_TIMES_PER_BACKWARD_FRAME 18

/**
 *      ...__   _   _   _   ___   _   _   _     ___     ___     ___   _     _   __________....
 * DALI      |_| |_| |_| |_|   |_| |_| |_| |___|   |___|   |___|   |_| |___| |_|
 *
 * TIMING    |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 * VALUE       1   1   1   1   0   0   0   0   1   0   1   0   1   0   0   1   1   STOP
 *         START |                             DATA                            | STOP
 */

enum pwm_states {
	NONE,  /* Disable sending. */
	LH,    /* 2 half bits long, next same as current */
	LHH,   /* 3 half bits long, current 1, next 0, next after 0 */
	LLH,   /* 3 half bits long, current 0, next 1, next after 1 */
	LLHH,  /* 4 half bits long, 3 bit toggle */
	LLLLH, /* 5 half bits long, invalid sequence for corrupted bw frame */
};

enum pwm_capture_state {
	PWM_CAPTURE_STATE_BUS_DOWN,
	PWM_CAPTURE_STATE_IDLE,
	PWM_CAPTURE_STATE_IN_HALFBIT,
	PWM_CAPTURE_STATE_AT_BITEND,
	PWM_CAPTURE_STATE_IN_CORRUPT,
	PWM_CAPTURE_STATE_ERROR,
};

/** bit times converted from ns to cycles of timer. To reduce stress on pwm interrupt. */
struct pwm_timings_cycles {
	uint32_t half_min;
	uint32_t half_max;
	uint32_t full_min;
	uint32_t full_max;
	uint32_t stop_time;
	int32_t flank_shift;
};

enum pwm_timing_length {
	HALF_BIT_TIME = 0,
	START_TIME,
	FULL_BIT_TIME,
	ERROR_TIME,
};

struct pwm_frame {
	/**
	 * DALI frame will be split into a sequence of PWM settings and put
	 * into this struct. each PWM setting must be send out one after another
	 * for this to work without disruptions.
	 */
	enum pwm_states signals[DALI_MAX_BIT_PER_FRAME + 1]; /* plus startbit */
	unsigned int signal_length; /**< How many entries are in the array */
	unsigned int position;      /**< Where we are on sending out the entries */
};

struct pwm_capture_helper {
	struct k_spinlock lock;
	uint32_t data;
	uint8_t length;
	enum pwm_capture_state state;
	uint32_t last_pulse; /* Needed to detect bus_down events. */
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
	struct k_sem pwm_sem;
	int32_t shift_cyc;
	uint32_t bit_time_half_cyc;
	struct pwm_frame pwm_frame;
};

struct dali_pwm_rx_data {
	struct dali_pwm_rx_callback cb;
	struct pwm_timings_cycles timings;
	enum pwm_timing_length latest_low;
	enum pwm_timing_length latest_high;
};

struct dali_pwm_data {
	/** DALI device */
	const struct device *dev;
	struct dali_pwm_tx_data tx;
	struct dali_pwm_rx_data rx;

	/** helper for receive */
	struct k_work_delayable frame_finish_work;
	struct pwm_capture_helper capture;
	/** forward frames see the settling time from the last edge on the bus. */
	uint32_t last_edge_timestamp; // TODO(Sven) oboslet?
	/** backward frames see the settling time from the last edge of their respective forward
	 * frame */
	uint32_t last_forward_frame_edge_timestamp; // TODO(Sven) oboslet?
#if defined(CONFIG_DALI_PWM_OWN_THREAD)
	struct k_sem tx_thread_sem;
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_DALI_PWM_THREAD_STACK_SIZE);
	struct k_thread thread;
#else
	struct k_work_delayable send_work;
#endif
};

struct dali_pwm_config {
	uint32_t time;
	struct pwm_dt_spec rx_capture_pwm;
	struct pwm_dt_spec tx_pwm;
	int tx_shift_us;
	int rx_shift_us;
	unsigned int tx_rx_propagation_max_us;
	unsigned int rx_finish_work_delay_us;
	unsigned int rx_capture_work_delay_us;
};

static void dali_pwm_execute_rx_callback(const struct dali_pwm_data *data, struct dali_frame frame)
{
	if (data->rx.cb.function) {
		(data->rx.cb.function)(data->dev, frame, data->rx.cb.user_data);
	}
}

/** this function gets called from pwm_capture to finish the frame, after idle time on bus. */
static void dali_pwm_finish_frame(struct dali_pwm_data *data)
{
	/* TODO(anyone) make sure, that the bus is really IDLE and not in ERROR state. */

	if (!data->capture.length) {
		/* guard against a race condition from delayed work and recv interrupt. */
		/* don't alter anything, if there is no data. */
		return;
	}

	if (data->capture.state == PWM_CAPTURE_STATE_IN_CORRUPT) {
		struct dali_frame frame = {
			.event_type = DALI_FRAME_CORRUPT,
		};

		dali_pwm_execute_rx_callback(data, frame);
	}

	if (data->capture.state != PWM_CAPTURE_STATE_ERROR) {
		struct dali_frame frame = {
			.data = data->capture.data,
		};
		switch (data->capture.length) {
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
			frame.event_type = DALI_EVENT_NONE;
		}
		LOG_INF("{%08x%c%02x %08x}", k_uptime_get_32(), ':', data->capture.length,
			data->capture.data);
		dali_pwm_execute_rx_callback(data, frame);
	}
	data->capture.state = PWM_CAPTURE_STATE_IDLE;
	data->capture.data = 0;
	data->capture.length = 0;
}

static void dali_pwm_finish_frame_work(struct k_work *_work)
{
	struct k_work_delayable *work = k_work_delayable_from_work(_work);
	struct dali_pwm_data *data = CONTAINER_OF(work, struct dali_pwm_data, frame_finish_work);

	k_spinlock_key_t capture_key = k_spin_lock(&data->capture.lock);
	dali_pwm_finish_frame(data);
	k_spin_unlock(&data->capture.lock, capture_key);
}

static enum pwm_timing_length dali_pwm_time_to_length(struct pwm_timings_cycles *timings,
						      uint32_t time, bool level)
{
	/* this might fail after a timer wrap-around */
	if ((time > timings->stop_time) && level) {
		return START_TIME;
	}
	if (timings->full_min < time && time < timings->full_max) {
		return FULL_BIT_TIME;
	}
	if (timings->half_min < time && time < timings->half_max) {
		return HALF_BIT_TIME;
	}
	return ERROR_TIME;
}

/*
 * This function assumes, that the access to data->capture is locked.
 */
static void dali_pwm_continuous_capture_callback_locked(uint32_t period_cycles,
							uint32_t pulse_cycles, int status,
							struct dali_pwm_data *data)
{
	if (status == -ERANGE) {
		/* timer overflow. Nothing to worry about */
		if (pulse_cycles != data->capture.last_pulse) {
			LOG_ERR("Bus power is lost!");
			data->capture.state = PWM_CAPTURE_STATE_BUS_DOWN;
		}
		data->capture.last_pulse = pulse_cycles;
		return;
	}
	data->capture.last_pulse = pulse_cycles;

	if (data->capture.state == PWM_CAPTURE_STATE_BUS_DOWN) {
		LOG_INF("Power back up again!");
		data->capture.state = PWM_CAPTURE_STATE_IDLE;
		return;
	}

	/* trigger PWM send */
	k_sem_give(&data->tx.pwm_sem);

	/* (re-)trigger stop condition */
	const struct device *dev = data->dev;
	const struct dali_pwm_config *config = dev->config;
	const uint32_t stopbit_timeout_us =
		DALI_RX_BIT_TIME_STOP_US - config->rx_capture_work_delay_us;
	k_work_reschedule(&data->frame_finish_work, Z_TIMEOUT_US(stopbit_timeout_us));

	if (data->rx.timings.flank_shift < 0) {
		if (-data->rx.timings.flank_shift >= pulse_cycles) {
			data->capture.state = PWM_CAPTURE_STATE_ERROR;
			return;
		}
	} else {
		if (pulse_cycles + data->rx.timings.flank_shift >= period_cycles) {
			data->capture.state = PWM_CAPTURE_STATE_ERROR;
			return;
		}
	}
	pulse_cycles += data->rx.timings.flank_shift;

	enum pwm_timing_length high =
		dali_pwm_time_to_length(&data->rx.timings, pulse_cycles, true);
	enum pwm_timing_length low =
		dali_pwm_time_to_length(&data->rx.timings, period_cycles - pulse_cycles, false);

	data->rx.latest_high = high;
	data->rx.latest_low = low;

	if (high == START_TIME || data->capture.state == PWM_CAPTURE_STATE_IDLE) {
		if (data->capture.length) {
			LOG_INF("Frame finish was not called!");
			dali_pwm_finish_frame(data);
		}
		if (low != HALF_BIT_TIME) {
			LOG_DBG("No valid start condition.");
			data->capture.state = PWM_CAPTURE_STATE_ERROR;
			return;
		}
		data->capture.state = PWM_CAPTURE_STATE_IN_HALFBIT;
		data->capture.data = 0;
		data->capture.length = 0;
		return;
	}
	if (data->capture.state == PWM_CAPTURE_STATE_ERROR ||
	    data->capture.state == PWM_CAPTURE_STATE_IN_CORRUPT) {
		return;
	}

	if (high == ERROR_TIME || low == ERROR_TIME) {
		LOG_DBG("received error condition");
		data->capture.state = PWM_CAPTURE_STATE_ERROR;
		return;
	}
	if (data->capture.state == PWM_CAPTURE_STATE_IN_HALFBIT) {
		/* .._|   */
		/*  | 1 | */
		if (high == HALF_BIT_TIME) {
			/*     _  */
			/* .._| | */
			/*  | 1 | */
			/* we already saved the one for the halfbit we are in */
			if (low == HALF_BIT_TIME) {
				/*     _    */
				/* .._| |_| */
				/*  | 1 | 1 */
				data->capture.data = (data->capture.data << 1) | 1;
				data->capture.length++;
			} else {
				/* must be FULL_BIT_TIME */
				/*     _      */
				/* .._| |___| */
				/*  | 1 | E   */
				data->capture.state = PWM_CAPTURE_STATE_ERROR;
			}
		} else {
			/* must be FULL_BIT_TIME */
			/*     ___  */
			/* .._|   | */
			/*  | 1 | 0 */
			data->capture.data = (data->capture.data << 1);
			data->capture.length++;
			if (low == HALF_BIT_TIME) {
				/*     ___    */
				/* .._|   |_| */
				/*  | 1 | 0 | */
				data->capture.state = PWM_CAPTURE_STATE_AT_BITEND;
			} else {
				/* must be FULL_BIT_TIME */
				/*     ___      */
				/* .._|   |___| */
				/*  | 1 | 0 | 1 */
				data->capture.data = (data->capture.data << 1) | 1;
				data->capture.length++;
			}
		}
	} else if (data->capture.state == PWM_CAPTURE_STATE_AT_BITEND) {
		/* .._| */
		/*  X | */
		if (high == HALF_BIT_TIME) {
			/*     _  */
			/* .._| | */
			/*  X | 0 */
			data->capture.data = (data->capture.data << 1);
			data->capture.length++;
			if (low == HALF_BIT_TIME) {
				/*     _    */
				/* .._| |_| */
				/*  X | 0 | */
				/* nothing to do */
			} else {
				/* must be FULL_BIT_TIME */
				/*     _      */
				/* .._| |___| */
				/*  X | 0 | 1 */
				data->capture.data = (data->capture.data << 1) | 1;
				data->capture.length++;
				data->capture.state = PWM_CAPTURE_STATE_IN_HALFBIT;
			}
		} else {
			/* must be FULL_BIT_TIME */
			/*     ___  */
			/* .._|   | */
			/* X  | E   */
			data->capture.state = PWM_CAPTURE_STATE_ERROR;
		}
	}
	if (data->capture.length > DALI_MAX_BIT_PER_FRAME) {
		data->capture.state = PWM_CAPTURE_STATE_ERROR;
	}
}

static void dali_pwm_continuous_capture_callback(const struct device *dev, uint32_t pwm,
						 uint32_t period_cycles, uint32_t pulse_cycles,
						 int status, void *user_data)
{
	struct dali_pwm_data *data = user_data;
	/* It may not be 100% accurate, but good enough. */
	k_spinlock_key_t capture_key = k_spin_lock(&data->capture.lock);
	dali_pwm_continuous_capture_callback_locked(period_cycles, pulse_cycles, status, data);
	k_spin_unlock(&data->capture.lock, capture_key);
}

static inline int dali_pwm_set_cycles(struct dali_pwm_data *data, const struct pwm_dt_spec *spec,
				      enum pwm_states state)
{
	uint32_t period = 0, pulse = 0;

	switch (state) {
	case NONE:
		period = 0;
		pulse = 0;
		break;
	case LH:
		period = data->tx.bit_time_half_cyc * 2;
		pulse = data->tx.bit_time_half_cyc + data->tx.shift_cyc;
		break;
	case LHH:
		period = data->tx.bit_time_half_cyc * 3;
		pulse = data->tx.bit_time_half_cyc + data->tx.shift_cyc;
		break;
	case LLH:
		period = data->tx.bit_time_half_cyc * 3;
		pulse = data->tx.bit_time_half_cyc * 2 + data->tx.shift_cyc;
		break;
	case LLHH:
		period = data->tx.bit_time_half_cyc * 4;
		pulse = data->tx.bit_time_half_cyc * 2 + data->tx.shift_cyc;
		break;
	case LLLLH:
		period = data->tx.bit_time_half_cyc * 5;
		pulse = data->tx.bit_time_half_cyc * 4 + data->tx.shift_cyc;
		break;
	}

	return pwm_set_cycles(spec->dev, spec->channel, period, pulse, spec->flags);
}

static void dali_pwm_generate_corrupt_frame(struct pwm_frame *pwm)
{
	pwm->signal_length = 0;
	pwm->position = 0;

	/* Send all ones, except for the second, where we stretch the active state over
	   the corrupt threshold */
	for (int i = 0; i < DALI_FRAME_BACKWARD_LENGTH + 1; i++) {
		pwm->signals[pwm->signal_length++] = i == 2 ? LLLLH : LH;
	}
}

/**
 * @brief construct PWM patterns for dali frame and return them.
 *
 */
static int dali_pwm_generate_frame(const struct dali_frame *frame, struct pwm_frame *pwm)
{
	int length = 0;

	switch (frame->event_type) {
	case DALI_FRAME_CORRUPT:
		dali_pwm_generate_corrupt_frame(pwm);
		return 0;
	case DALI_FRAME_BACKWARD:
		length = DALI_FRAME_BACKWARD_LENGTH;
		break;
	case DALI_FRAME_GEAR:
		length = DALI_FRAME_GEAR_LENGTH;
		break;
	case DALI_FRAME_DEVICE:
		length = DALI_FRAME_DEVICE_LENGTH;
		break;
	case DALI_FRAME_FIRMWARE:
		length = DALI_FRAME_UPDATE_LENGTH;
		break;
	default:
		return -EINVAL;
	}

	/* reset the frame buffer */
	*pwm = (struct pwm_frame){0};

	/* We iterate over the frame in full and half bits. */
	int shift_half_bit = 0;

	/* Startbit is 1 and is added here. */
	bool current_bit = true;
	bool next_bit = !!(frame->data & (1 << (length - 1)));
	bool next_next_bit = !!(frame->data & (1 << (length - 2)));
	LOG_DBG("Generating new frame with data %08x and length %d", frame->data, length);
	while (length > 0) {
		if (current_bit == next_bit) {
			pwm->signals[pwm->signal_length] = LH;
			shift_half_bit += 2;
		} else if (current_bit == next_next_bit && shift_half_bit == 1) {
			pwm->signals[pwm->signal_length] = LLHH;
			shift_half_bit += 4;
		} else if (current_bit) {
			pwm->signals[pwm->signal_length] = LHH;
			shift_half_bit += 3;
		} else {
			pwm->signals[pwm->signal_length] = LLH;
			shift_half_bit += 3;
		}
		pwm->signal_length++;
		while (shift_half_bit > 1) {
			length--;
			current_bit = next_bit;
			next_bit = next_next_bit;
			if (length > 1) {
				next_next_bit = !!(frame->data & (1 << (length - 2)));
			}
			/* no need for an else branch, we want the next_next_bit to be the same as
			 * the next_bit, which is already the case. */
			shift_half_bit -= 2;
		}
	}

	/* Check if there is a signal missing at the end */
	if (shift_half_bit || (current_bit && next_bit && length == 0)) {
		/* We need to add the signal for the last bit.
		   it could either be, that we are in the middle of the 0 bit and need
		   to send the last half of the zero, or that we are missing a full 1.
		   Signal could also be LHH, it just needs a short low bit, we disable the
		   pwm after this and high level is disabled. */
		pwm->signals[pwm->signal_length] = LH;
		pwm->signal_length++;
	}

	return 0;
}

static inline bool dali_pwm_is_frame_time_as_expected(enum pwm_timing_length low,
						      enum pwm_timing_length high,
						      enum pwm_states latest,
						      enum pwm_states second_to_latest)
{
	/**
	 * pwm capture and pwm generate are offset by the period.
	 *                                     capture high
	 * 						  capture low
	 * ______________                       __________            __________
	 *               |_____________________|          |__________|          |______________-
	 *               |        second to latest        |      latest pwm     |
	 *
	 *                                                            ^ current time-point
	 */

	if (low == ERROR_TIME || high == ERROR_TIME) {
		return false;
	}
	if (low == HALF_BIT_TIME && (latest != LH && latest != LHH)) {
		return false;
	}
	if (low == FULL_BIT_TIME && (latest != LLH && latest != LLHH)) {
		return false;
	}
	if (high == HALF_BIT_TIME &&
	    (second_to_latest != LH && second_to_latest != LLH && second_to_latest != LLLLH)) {
		return false;
	}
	if (high == FULL_BIT_TIME && (second_to_latest != LHH && second_to_latest != LLHH)) {
		return false;
	}
	return true;
}

static k_timeout_t dali_pwm_process_sendout(const struct device *dev)
{
	const struct dali_pwm_config *config = dev->config;
	struct dali_pwm_data *data = dev->data;
	struct pwm_frame *frame = &data->tx.pwm_frame;
	int ret = 0;

	/* start sending */
	while (true) {
		if (frame->position != 0 && data->capture.state == PWM_CAPTURE_STATE_ERROR) {
			/* We had an error decoding frames we send.... this must be a collision. */
			/* TODO(anyone) maybe send the break condition and retry sending the frame
			 */
			ret = pwm_set_pulse_dt(&config->tx_pwm, 0);
			LOG_DBG("Capture Error");
			break;
		}

		if (frame->position == 1) {
			if (data->rx.latest_low != HALF_BIT_TIME ||
			    data->rx.latest_high != START_TIME) {
				LOG_DBG("We are not receiving what we are sending. position = %d",
					frame->position);
				pwm_set_pulse_dt(&config->tx_pwm, 0);
				break;
			}
		} else if (frame->position > 1) {
			if (!dali_pwm_is_frame_time_as_expected(
				    data->rx.latest_low, data->rx.latest_high,
				    frame->signals[frame->position - 1],
				    frame->signals[frame->position - 2])) {
				LOG_DBG("We are not receiving what we are sending. position = %d",
					frame->position);
				pwm_set_pulse_dt(&config->tx_pwm, 0);
				break;
			}
		}
		/* Check if everything has been send. -> disable PWM */
		if (frame->position >= frame->signal_length) {
			/* send everything, stop now. */
			ret = dali_pwm_set_cycles(data, &config->tx_pwm, NONE);
			LOG_DBG("frame is sent.");
			break;
		}

		/* check if the pattern has changed, if not, we don't need to do anything and can
		   just skip the reconfigure. */
		if (frame->position == 0 ||
		    frame->signals[frame->position] != frame->signals[frame->position - 1]) {
			ret = dali_pwm_set_cycles(data, &config->tx_pwm,
						  frame->signals[frame->position]);
			if (ret < 0) {
				LOG_DBG("failed to set pwm: %d", ret);
				break;
			}
			if (frame->position == 0) {
				/* "reset" semaphore to 0, otherwise, the loop will run twice
				   before the first PWM setting is even in effect. */
				k_sem_take(&data->tx.pwm_sem, K_NO_WAIT);
				/* the semaphore is given every time the bus returns from low to
				   high, the bus on the other hand can be pulled low by either us,
				   or any other device. Since we don't know when the pwm will take
				   effect, we cannot know when we are starting to send and need to
				   reset this semaphore. This point seemed close enough with the
				   scheduler, that it should hopefully never happen, that it gets
				   falsely given. */
			}
		}
		frame->position++;

		/* wait for the flank */
		if (k_sem_take(&data->tx.pwm_sem, Z_TIMEOUT_US(config->tx_rx_propagation_max_us)) <
		    0) {
			/* Timeout while waiting for semaphore.
			   There is a bus-error going on and we need to stop sending.
			   this is only a method of last resort and should theoretically
			   never happen as we have an idle state on enter and the TX must
			   result in a flank. */
			dali_pwm_set_cycles(data, &config->tx_pwm, NONE);
			LOG_DBG("BUS error at position %d", frame->position);
			break;
		}
	}
	k_spinlock_key_t capture_key = k_spin_lock(&data->capture.lock);
	k_spin_unlock(&data->capture.lock, capture_key);
	/* mark frame as send and not valid anymore. */
	frame->signal_length = 0;
	return K_FOREVER;
}

#if defined(CONFIG_DALI_PWM_OWN_THREAD)
static void dali_pwm_tx_thread(void *arg1, void *unused1, void *unused2)
{
	k_thread_name_set(NULL, "dali_pwm_tx_thread");

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	const struct device *dev = (const struct device *)arg1;
	struct dali_pwm_data *data = dev->data;

	while (true) {
		k_timeout_t timeout = dali_pwm_process_sendout(dev);
		/* either sleep until we are ready to send the first entry, or until
		 * there is a new entry in the list, which might be a new first entry.
		 */
		k_sem_take(&data->tx_thread_sem, timeout);
	}
}
#else
static void dali_pwm_tx_work_cb(struct k_work *_work)
{
	struct k_work_delayable *work = k_work_delayable_from_work(_work);
	struct dali_pwm_data *data = CONTAINER_OF(work, struct dali_pwm_data, send_work);

	k_timeout_t timeout = dali_pwm_process_sendout(data->dev);
	if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		k_work_reschedule(work, timeout);
	}
}
#endif

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
	struct dali_pwm_data *data = dev->data;
	int ret = 0;

	switch (frame->event_type) {
	case DALI_EVENT_NONE:
		break;
	case DALI_FRAME_GEAR:
	case DALI_FRAME_DEVICE:
	case DALI_FRAME_FIRMWARE:
		// if (data->rx.status != IDLE) { <-- TODO(Sven): check if we are busy sending
		// 	ret = -EBUSY;
		// 	break;
		// }
	case DALI_FRAME_CORRUPT:
	case DALI_FRAME_BACKWARD:
		data->tx.cb.function = callback;
		data->tx.cb.user_data = user_data;
		ret = dali_pwm_generate_frame(frame, &data->tx.pwm_frame);
		if (ret < 0) {
			break;
		}

		/* trigger sending out */
#if defined(CONFIG_DALI_PWM_OWN_THREAD)
		k_sem_give(&data->tx_thread_sem);
#else
		k_work_reschedule(&data->send_work, K_NO_WAIT);
#endif
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void dali_pwm_abort(const struct device *dev)
{
	struct dali_pwm_data *data = dev->data;

	/* set signal length to 0 will abort sending and remove frame. No need to delete data */
	data->tx.pwm_frame.signal_length = 0;
	/* we don't abort backward frames, as they are not resend. */
}

static int dali_pwm_init(const struct device *dev)
{
	const struct dali_pwm_config *config = dev->config;
	struct dali_pwm_data *data = dev->data;
	int ret;

	/* connect to device */
	data->dev = dev;

#if defined(CONFIG_DALI_PWM_OWN_THREAD)
	ret = k_sem_init(&data->tx_thread_sem, 0, 1);
	if (ret < 0) {
		LOG_ERR("Could not initialize send messagequeue semaphore.");
		return ret;
	}

	k_thread_create(&data->thread, data->thread_stack, CONFIG_DALI_PWM_THREAD_STACK_SIZE,
			dali_pwm_tx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_DALI_PWM_THREAD_PRIORITY), 0, K_NO_WAIT);
#else
	k_work_init_delayable(&data->send_work, dali_pwm_tx_work_cb);
#endif

	/* initialize tx semaphore*/
	ret = k_sem_init(&data->tx.pwm_sem, 0, 1);
	if (ret < 0) {
		LOG_ERR("Could not initialize PWM semaphore.");
		return ret;
	}

	/* initialize pwm peripheral */
	if (!pwm_is_ready_dt(&config->tx_pwm)) {
		LOG_ERR("PWM device %s is not ready", config->tx_pwm.dev->name);
		return -ENODEV;
	}

	if (!pwm_is_ready_dt(&config->rx_capture_pwm)) {
		LOG_ERR("Capture device %s is not ready", config->rx_capture_pwm.dev->name);
		return -ENODEV;
	}

	/* calculate transmit timings */
	uint64_t cycles_per_sec;

	ret = pwm_get_cycles_per_sec(config->tx_pwm.dev, config->tx_pwm.channel, &cycles_per_sec);
	if (ret) {
		LOG_ERR("Could not get cycles per sec for tx channel.");
		return ret;
	}
	if (cycles_per_sec < 200000) {
		LOG_ERR("PWM timer is not accurate enough. Need at least 200kHz. Have %lldHz",
			cycles_per_sec);
		return -ERANGE;
	}
	data->tx.bit_time_half_cyc = dali_pwm_us_to_cycles(DALI_TX_HALF_BIT_US, cycles_per_sec);
	data->tx.shift_cyc = dali_pwm_us_to_cycles(config->tx_shift_us, cycles_per_sec);

	/* calculate recieve timings */
	pwm_get_cycles_per_sec(config->rx_capture_pwm.dev, config->rx_capture_pwm.channel,
			       &cycles_per_sec);
	/* dali bit timings should be accurate to about 5 us => 200khz */
	if (cycles_per_sec < 200000) {
		LOG_ERR("Capture timer is not accurate enough. Need at least 200kHz. Have %lldHz",
			cycles_per_sec);
		return -ERANGE;
	}
	data->rx.timings.half_min = dali_pwm_us_to_cycles(
		(DALI_RX_BIT_TIME_HALF_MIN_US - DALI_RX_GREY_AREA), cycles_per_sec);
	data->rx.timings.half_max = dali_pwm_us_to_cycles(
		(DALI_RX_BIT_TIME_HALF_MAX_US + DALI_RX_GREY_AREA), cycles_per_sec);
	data->rx.timings.full_min = dali_pwm_us_to_cycles(
		(DALI_RX_BIT_TIME_FULL_MIN_US - DALI_RX_GREY_AREA), cycles_per_sec);
	data->rx.timings.full_max = dali_pwm_us_to_cycles(
		(DALI_RX_BIT_TIME_FULL_MAX_US + DALI_RX_GREY_AREA), cycles_per_sec);
	data->rx.timings.stop_time =
		dali_pwm_us_to_cycles((DALI_RX_BIT_TIME_STOP_US - 50U), cycles_per_sec);
	data->rx.timings.flank_shift = dali_pwm_us_to_cycles(config->rx_shift_us, cycles_per_sec);

	/* configure and start capture */
	ret = pwm_configure_capture(config->rx_capture_pwm.dev, config->rx_capture_pwm.channel,
				    PWM_CAPTURE_MODE_CONTINUOUS | PWM_CAPTURE_TYPE_BOTH |
					    config->rx_capture_pwm.flags,
				    dali_pwm_continuous_capture_callback, data);
	if (ret < 0) {
		LOG_ERR("Could not configure capture. %s", strerror(-ret));
		return ret;
	}

	ret = pwm_enable_capture(config->rx_capture_pwm.dev, config->rx_capture_pwm.channel);
	if (ret < 0) {
		LOG_ERR("Could not enable capture. %s", strerror(-ret));
		return ret;
	}

	return 0;
}

static DEVICE_API(dali, dali_pwm_driver_api) = {
	.receive = dali_pwm_receive,
	.send = dali_pwm_send,
	.abort = dali_pwm_abort,
};

#define DALI_PWM_INIT(idx)                                                                         \
	BUILD_ASSERT(DT_INST_PROP_OR(idx, tx_flank_shift_us, 0) < DALI_TX_HALF_BIT_US ||           \
			     (DT_INST_PROP_OR(idx, tx_flank_shift_us, 0) ^ 0xffffffff) <           \
				     DALI_TX_HALF_BIT_US,                                          \
		     "Edge-shift must be lower than 416us.");                                      \
	static struct dali_pwm_data dali_pwm_data_##idx = {                                        \
		.capture.state = PWM_CAPTURE_STATE_IDLE,                                           \
		.capture.data = 0,                                                                 \
		.capture.length = 0,                                                               \
		.frame_finish_work = Z_WORK_DELAYABLE_INITIALIZER(dali_pwm_finish_frame_work),     \
	};                                                                                         \
	static const struct dali_pwm_config dali_pwm_config_##idx = {                              \
		.rx_capture_pwm = PWM_DT_SPEC_INST_GET_BY_IDX(idx, 1),                             \
		.tx_pwm = PWM_DT_SPEC_INST_GET_BY_IDX(idx, 0),                                     \
		.tx_shift_us = DT_INST_PROP_OR(idx, tx_flank_shift_us, 0),                         \
		.rx_shift_us = DT_INST_PROP_OR(idx, rx_flank_shift_us, 0),                         \
		.tx_rx_propagation_max_us = DT_INST_PROP_OR(idx, tx_rx_propagation_max_us, 200),   \
		.rx_finish_work_delay_us = DT_INST_PROP_OR(idx, rx_finish_work_delay_us, 800),     \
		.rx_capture_work_delay_us = DT_INST_PROP_OR(idx, rx_capture_work_delay_us, 200),   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, dali_pwm_init, NULL, &dali_pwm_data_##idx,                      \
			      &dali_pwm_config_##idx, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dali_pwm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DALI_PWM_INIT);
