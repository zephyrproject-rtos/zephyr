#define DT_DRV_COMPAT zephyr_dali_pwm

#include <dali.h>
#include <dali_std.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dali_low_level, CONFIG_DALI_LOW_LEVEL_LOG_LEVEL);

/* all values in nanoseconds */

#define DALI_TX_BIT_TIME_HALF        416667
#define DALI_RX_BIT_TIME_STOP        1900000
#define DALI_RX_GREY_AREA            80000
#define DALI_RX_BIT_TIME_HALF_MIN    (333333 - DALI_RX_GREY_AREA)
#define DALI_RX_BIT_TIME_HALF_MAX    (500000 + DALI_RX_GREY_AREA)
#define DALI_RX_BIT_TIME_FULL_MIN    (666667 - DALI_RX_GREY_AREA)
#define DALI_RX_BIT_TIME_FULL_MAX    (1000000 + DALI_RX_GREY_AREA)
#define DALI_RX_BIT_TIME_CORRUPT_MIN (1300000 - DALI_RX_GREY_AREA)
#define DALI_RX_BIT_TIME_CORRUPT_MAX (2000000 + DALI_RX_GREY_AREA)

#define SETTLING_TIME_BACKWARD_FRAME_MAX     k_ns_to_cyc_floor32(10500000)
#define MAX_HALFBIT_TIMES_PER_BACKWARD_FRAME 18
#define DALI_PWM_NO_RESPONSE_RECEIVED                                                              \
	SETTLING_TIME_BACKWARD_FRAME_MAX +                                                         \
		k_ns_to_cyc_floor32(DALI_TX_BIT_TIME_HALF) * MAX_HALFBIT_TIMES_PER_BACKWARD_FRAME

#define SEND_TWICE_MAX_TIME k_ms_to_cyc_floor32(95)

const uint32_t settling_times_min[] = {
	k_ns_to_cyc_floor32(5500000),  /* Settling time backward min */
	k_ns_to_cyc_floor32(13500000), /* Settling time prio 1 min */
	k_ns_to_cyc_floor32(14900000), /* Settling time prio 2 min */
	k_ns_to_cyc_floor32(16300000), /* Settling time prio 3 min */
	k_ns_to_cyc_floor32(17900000), /* Settling time prio 4 min */
	k_ns_to_cyc_floor32(19500000), /* Settling time prio 5 min */
};

const uint32_t settling_times_length[] = {
	k_ns_to_cyc_floor32(5000000), /* Settling time backward max - min */
	k_ns_to_cyc_floor32(1200000), /* Settling time prio 1 max - min */
	k_ns_to_cyc_floor32(1200000), /* Settling time prio 2 max - min */
	k_ns_to_cyc_floor32(1400000), /* Settling time prio 3 max - min */
	k_ns_to_cyc_floor32(1400000), /* Settling time prio 4 max - min */
	k_ns_to_cyc_floor32(1600000), /* Settling time prio 5 max - min */
};

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
	uint32_t corrupt_min;
	uint32_t corrupt_max;
	uint32_t stop_time;
	int32_t rx_flank_shift;
};

enum pwm_timing_length {
	HALF_BIT_TIME = 0,
	START_TIME,
	FULL_BIT_TIME,
	CORRUPT_BIT_TIME,
	ERROR_TIME,
};

struct pwm_frame {
	/**
	 * DALI frame will be split into a sequence of PWM settings and put
	 * into this struct. each PWM setting must be send out one after another
	 * for this to work without disruptions.
	 */
	enum pwm_states signals[MAX_BIT_PER_FRAME + 1]; /* plus startbit */
	unsigned int signal_length;                     /**< How many entries are in the array */
	unsigned int position;          /**< Where we are on sending out the entries */
	enum dali_tx_priority priority; /**< Inter frame timing */
	bool is_query;                  /**< If this frame is a query */
};

struct pwm_capture_helper {
	struct k_spinlock lock;
	uint32_t data;
	uint8_t length;
	enum pwm_capture_state state;
	uint32_t last_pulse; /* Needed to detect bus_down events. */
};

struct dali_pwm_data {
	/** DALI device */
	const struct device *dev;
	/** helper for recv */
	struct k_work_delayable frame_finish_work;
	struct pwm_capture_helper capture;
	struct pwm_timings_cycles timings;
	struct k_msgq frames_queue;
	char frames_buffer[CONFIG_MAX_FRAMES_IN_QUEUE * sizeof(struct dali_frame)];
	struct k_sem tx_pwm_sem;
	int tx_shift_cyc;
	uint32_t bit_time_half_cyc;
	/** forward frames see the settling time from the last edge on the bus. */
	uint32_t last_edge_timestamp;
	/** backward frames see the settling time from the last edge of their respective forward
	 * frame */
	uint32_t last_forward_frame_edge_timestamp;
	enum pwm_timing_length latest_low;
	enum pwm_timing_length latest_high;
	struct dali_frame last_frame;
	uint32_t last_frame_timestamp;
	struct k_work_delayable no_response_work;
	struct pwm_frame forward_frame;
	struct pwm_frame backward_frame;
#if defined(CONFIG_DALI_PWM_OWN_THREAD)
	struct k_sem tx_queue_sem;
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
	int tx_shift_ns;
	int rx_shift_ns;
	unsigned int tx_rx_delay_us;
};

/** this function gets called from pwm_capture to finish the frame, after idle time on bus. */
static void finish_frame(struct dali_pwm_data *data)
{
	/* TODO(anyone) make sure, that the bus is really IDLE and not in ERROR state. */

	if (!data->capture.length) {
		/* guard against a race condition from delayed work and recv interrupt. */
		/* don't alter anything, if there is no data. */
		return;
	}
	uint32_t time_now = k_cycle_get_32();
	bool is_send_twice = true;
	if ((time_now - data->last_frame_timestamp) > SEND_TWICE_MAX_TIME) {
		is_send_twice = false;
	}

	if (data->capture.state == PWM_CAPTURE_STATE_IN_CORRUPT) {
		struct dali_frame frame = {
			.event_type = DALI_FRAME_CORRUPT,
		};

		k_work_cancel_delayable(&data->no_response_work);
		k_msgq_put(&data->frames_queue, &frame, K_NO_WAIT);
	}
	if (data->capture.state != PWM_CAPTURE_STATE_ERROR) {
		struct dali_frame frame = {
			.data = data->capture.data,
		};
		switch (data->capture.length) {
		case FRAME_BACKWARD_LENGTH:
			frame.event_type = DALI_FRAME_BACKWARD;
			k_work_cancel_delayable(&data->no_response_work);
			break;
		case FRAME_GEAR_LENGTH:
			data->last_forward_frame_edge_timestamp = data->last_edge_timestamp;
			if (data->last_frame.event_type == DALI_FRAME_GEAR &&
			    data->last_frame.data == frame.data && is_send_twice) {
				frame.event_type = DALI_FRAME_GEAR_TWICE;
			} else {
				frame.event_type = DALI_FRAME_GEAR;
			}
			break;
		case FRAME_DEVICE_LENGTH:
			data->last_forward_frame_edge_timestamp = data->last_edge_timestamp;
			if (data->last_frame.event_type == DALI_FRAME_DEVICE &&
			    data->last_frame.data == frame.data && is_send_twice) {
				frame.event_type = DALI_FRAME_DEVICE_TWICE;
			} else {
				frame.event_type = DALI_FRAME_DEVICE;
			}
			break;
		case FRAME_UPDATE_LENGTH:
			data->last_forward_frame_edge_timestamp = data->last_edge_timestamp;
			if (data->last_frame.event_type == DALI_FRAME_FIRMWARE &&
			    data->last_frame.data == frame.data && is_send_twice) {
				frame.event_type = DALI_FRAME_FIRMWARE_TWICE;
			} else {
				frame.event_type = DALI_FRAME_FIRMWARE;
			}
			break;
		default:
			frame.event_type = DALI_EVENT_NONE;
		}
		LOG_INF("{%08x%c%02x %08x}", 0, ':', data->capture.length, data->capture.data);
		k_msgq_put(&data->frames_queue, &frame, K_NO_WAIT);
		data->last_frame = frame;
		data->last_frame_timestamp = time_now;
	}
	data->capture.state = PWM_CAPTURE_STATE_IDLE;
	data->capture.data = 0;
	data->capture.length = 0;
}

static void finish_frame_work(struct k_work *_work)
{
	struct k_work_delayable *work = k_work_delayable_from_work(_work);
	struct dali_pwm_data *data = CONTAINER_OF(work, struct dali_pwm_data, frame_finish_work);

	k_spinlock_key_t capture_key = k_spin_lock(&data->capture.lock);
	finish_frame(data);
	k_spin_unlock(&data->capture.lock, capture_key);
}

static void no_response_work(struct k_work *_work)
{
	struct k_work_delayable *work = k_work_delayable_from_work(_work);
	struct dali_pwm_data *data = CONTAINER_OF(work, struct dali_pwm_data, no_response_work);

	struct dali_frame frame = {
		.event_type = DALI_EVENT_NO_ANSWER,
	};
	k_msgq_put(&data->frames_queue, &frame, K_NO_WAIT);
}

static enum pwm_timing_length pwm_time_to_length(struct pwm_timings_cycles *timings, uint32_t time,
						 bool level)
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
	if (timings->corrupt_min < time && time < timings->corrupt_max && !level) {
		return CORRUPT_BIT_TIME;
	}
	/* TODO(anyone) expand with extra-states in DALI bus for collision, etc. */
	return ERROR_TIME;
}

/*
 * This function assumes, that the access to data->capture is locked.
 */
static void continuous_capture_callback_locked(uint32_t period_cycles, uint32_t pulse_cycles,
					       int status, struct dali_pwm_data *data)
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
	k_sem_give(&data->tx_pwm_sem);
	/* wait for stop condition */
	k_work_reschedule(&data->frame_finish_work, Z_TIMEOUT_NS(DALI_RX_BIT_TIME_STOP));

	if (data->timings.rx_flank_shift < 0) {
		if (-data->timings.rx_flank_shift >= pulse_cycles) {
			data->capture.state = PWM_CAPTURE_STATE_ERROR;
			return;
		}
	} else {
		if (pulse_cycles + data->timings.rx_flank_shift >= period_cycles) {
			data->capture.state = PWM_CAPTURE_STATE_ERROR;
			return;
		}
	}
	pulse_cycles += data->timings.rx_flank_shift;

	enum pwm_timing_length high = pwm_time_to_length(&data->timings, pulse_cycles, true);
	enum pwm_timing_length low =
		pwm_time_to_length(&data->timings, period_cycles - pulse_cycles, false);

	data->latest_high = high;
	data->latest_low = low;

	if (high == START_TIME || data->capture.state == PWM_CAPTURE_STATE_IDLE) {
		if (data->capture.length) {
			LOG_INF("Frame finish was not called!");
			finish_frame(data);
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
	if (low == CORRUPT_BIT_TIME) {
		data->capture.state = PWM_CAPTURE_STATE_IN_CORRUPT;
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
	if (data->capture.length > MAX_BIT_PER_FRAME) {
		data->capture.state = PWM_CAPTURE_STATE_ERROR;
	}
}

static void continuous_capture_callback(const struct device *dev, uint32_t pwm,
					uint32_t period_cycles, uint32_t pulse_cycles, int status,
					void *user_data)
{
	struct dali_pwm_data *data = user_data;
	/* It may not be 100% accurate, but good enough. */
	uint32_t timestamp = k_cycle_get_32();
	k_spinlock_key_t capture_key = k_spin_lock(&data->capture.lock);
	continuous_capture_callback_locked(period_cycles, pulse_cycles, status, data);
	k_spin_unlock(&data->capture.lock, capture_key);
	data->last_edge_timestamp = timestamp;
}

int dali_pwm_recv(const struct device *dev, struct dali_frame *frame, k_timeout_t timeout)
{
	struct dali_pwm_data *data = dev->data;

	if (frame == NULL) {
		return -EINVAL;
	}

	if (k_msgq_get(&data->frames_queue, frame, timeout) < 0) {
		return -ENOMSG;
	}

	return 0;
}

void dali_pwm_abort(const struct device *dev)
{
	struct dali_pwm_data *data = dev->data;

	/* set signal length to 0 will abort sending and remove frame. No need to delete data */
	data->forward_frame.signal_length = 0;
	/* we don't abort backward frames, as they are not resend. */
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
		period = data->bit_time_half_cyc * 2;
		pulse = data->bit_time_half_cyc + data->tx_shift_cyc;
		break;
	case LHH:
		period = data->bit_time_half_cyc * 3;
		pulse = data->bit_time_half_cyc + data->tx_shift_cyc;
		break;
	case LLH:
		period = data->bit_time_half_cyc * 3;
		pulse = data->bit_time_half_cyc * 2 + data->tx_shift_cyc;
		break;
	case LLHH:
		period = data->bit_time_half_cyc * 4;
		pulse = data->bit_time_half_cyc * 2 + data->tx_shift_cyc;
		break;
	case LLLLH:
		period = data->bit_time_half_cyc * 5;
		pulse = data->bit_time_half_cyc * 4 + data->tx_shift_cyc;
		break;
	}

	return pwm_set_cycles(spec->dev, spec->channel, period, pulse, spec->flags);
}

static void generate_corrupted_bw_frame(struct pwm_frame *pwm)
{
	pwm->signal_length = 0;
	pwm->position = 0;
	pwm->priority = DALI_PRIORITY_BACKWARD_FRAME;

	/* Send all ones, except for the second, where we stretch the active state over
	   the corrupt threshold */
	for (int i = 0; i < FRAME_BACKWARD_LENGTH + 1; i++) {
		pwm->signals[pwm->signal_length++] = i == 2 ? LLLLH : LH;
	}
}

/**
 * @brief construct PWM patterns for dali frame and return them.
 *
 */
static int generate_pwm_frame(const struct dali_frame *frame, enum dali_tx_priority priority,
			      bool is_query, struct pwm_frame *pwm)
{
	int length = 0;
	switch (frame->event_type) {
	case DALI_FRAME_CORRUPT:
		generate_corrupted_bw_frame(pwm);
		return 0;
	case DALI_FRAME_BACKWARD:
		length = FRAME_BACKWARD_LENGTH;
		break;
	case DALI_FRAME_GEAR:
		length = FRAME_GEAR_LENGTH;
		break;
	case DALI_FRAME_DEVICE:
		length = FRAME_DEVICE_LENGTH;
		break;
	case DALI_FRAME_FIRMWARE:
		length = FRAME_UPDATE_LENGTH;
		break;
	default:
		return -EINVAL;
	}

	memset(pwm, 0, sizeof(struct pwm_frame));

	pwm->priority = priority;
	pwm->is_query = is_query;

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
			 */
			/* the next_bit, which is already the case. */
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

static inline bool is_frame_time_as_expected(enum pwm_timing_length low,
					     enum pwm_timing_length high, enum pwm_states latest,
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
	if (low == CORRUPT_BIT_TIME && (latest != LLLLH)) {
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

int dali_pwm_send(const struct device *dev, const struct dali_tx_frame *tx_frame)
{
	struct dali_pwm_data *data = dev->data;
	int ret = 0;
	struct pwm_frame *working_frame;
	enum dali_tx_priority priority;
	bool is_query;

	__ASSERT(dev, "invalid device");

	if (tx_frame->frame.event_type == DALI_EVENT_NONE) {
		return ret;
	}

	if (tx_frame->frame.event_type == DALI_FRAME_CORRUPT ||
	    tx_frame->frame.event_type == DALI_FRAME_BACKWARD) {
		/* We are currently sending, this cannot be a response anymore */
		if ((data->forward_frame.signal_length && data->forward_frame.position) ||
		    (data->backward_frame.signal_length && data->backward_frame.position)) {
			return -ETIMEDOUT;
		}
		/* it is to late the send the response */
		if (k_cycle_get_32() - data->last_forward_frame_edge_timestamp >
		    SETTLING_TIME_BACKWARD_FRAME_MAX) {
			return -ETIMEDOUT;
		}
		working_frame = &data->backward_frame;
		priority = DALI_PRIORITY_BACKWARD_FRAME;
		is_query = false;
	} else {
		working_frame = &data->forward_frame;
		if (tx_frame->priority < DALI_PRIORITY_1 || tx_frame->priority > DALI_PRIORITY_5) {
			return -EINVAL;
		}
		priority = tx_frame->priority;
		is_query = tx_frame->is_query;
	}

	/* Check if we can store the frame. */
	if (working_frame->signal_length) {
		return -EBUSY;
	}

	ret = generate_pwm_frame(&(tx_frame->frame), priority, is_query, working_frame);
	if (ret < 0) {
		return ret;
	}

	/* trigger sending out */
#if defined(CONFIG_DALI_PWM_OWN_THREAD)
	k_sem_give(&data->tx_queue_sem);
#else
	k_work_reschedule(&data->send_work, K_NO_WAIT);
#endif

	return 0;
}

static k_timeout_t process_pwm_sendout(const struct device *dev)
{
	const struct dali_pwm_config *config = dev->config;
	struct dali_pwm_data *data = dev->data;
	struct pwm_frame *frame;
	int ret;

	if (data->backward_frame.signal_length) {
		/* prioritize backward frames */
		frame = &data->backward_frame;
	} else if (data->forward_frame.signal_length) {
		/* forward frames as fallback */
		frame = &data->forward_frame;
	} else {
		/* no frame to send, we wait forever until there is an entry */
		return K_FOREVER;
	}

	uint32_t time_difference = k_cycle_get_32() - data->last_edge_timestamp;
	if (time_difference < settling_times_min[frame->priority]) {
		/* should be somehow random */
		uint32_t random =
			settling_times_length[frame->priority] * (time_difference & 3) / 4;

		uint32_t sleep_time =
			settling_times_min[frame->priority] - time_difference + random;

		return Z_TIMEOUT_CYC(sleep_time);
	}

	/* time difference is bigger then the minimal wait time, so we send out the frame */
	LOG_DBG("Sending frame with prio %d", frame->priority);

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
			if (data->latest_low != HALF_BIT_TIME || data->latest_high != START_TIME) {
				LOG_DBG("We are not receiving what we are sending.");
				pwm_set_pulse_dt(&config->tx_pwm, 0);
				break;
			}
		} else if (frame->position > 1) {
			if (!is_frame_time_as_expected(data->latest_low, data->latest_high,
						       frame->signals[frame->position - 1],
						       frame->signals[frame->position - 2])) {
				LOG_DBG("We are not receiving what we are sending.");
				pwm_set_pulse_dt(&config->tx_pwm, 0);
				break;
			}
		}
		/* Check if everything has been send. -> disable PWM */
		if (frame->position >= frame->signal_length) {
			/* send everything, stop now. */
			ret = dali_pwm_set_cycles(data, &config->tx_pwm, NONE);
			break;
		}

		/* check if the pattern has changed, if not, we don't need to do anything and can
		   just skip the reconfigure. */
		if (frame->position == 0 ||
		    frame->signals[frame->position] != frame->signals[frame->position - 1]) {
			int ret = dali_pwm_set_cycles(data, &config->tx_pwm,
						      frame->signals[frame->position]);
			if (ret < 0) {
				break;
			}
			if (frame->position == 0) {
				/* "reset" semaphore to 0, otherwise, the loop will run twice
				   before the first PWM setting is even in effect. */
				k_sem_take(&data->tx_pwm_sem, K_NO_WAIT);
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
		if (k_sem_take(&data->tx_pwm_sem, Z_TIMEOUT_NS(6 * DALI_TX_BIT_TIME_HALF)) < 0) {
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
	finish_frame(data);
	k_spin_unlock(&data->capture.lock, capture_key);
	if (frame->is_query) {
		k_work_reschedule(&data->no_response_work,
				  Z_TIMEOUT_CYC(DALI_PWM_NO_RESPONSE_RECEIVED));
	}
	/* mark frame as send and not valid anymore. */
	frame->signal_length = 0;
	return K_NO_WAIT;
}

#if defined(CONFIG_DALI_PWM_OWN_THREAD)
static void dali_tx_thread(void *arg1, void *unused1, void *unused2)
{
	k_thread_name_set(NULL, "dali_pwm_tx_thread");

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	const struct device *dev = (const struct device *)arg1;
	struct dali_pwm_data *data = dev->data;

	while (true) {
		k_timeout_t timeout = process_pwm_sendout(dev);
		/* either sleep until we are ready to send the first entry, or until
		 * there is a new entry in the list, which might be a new first entry.
		 */
		k_sem_take(&data->tx_queue_sem, timeout);
	}
}
#else
static void dali_tx_work_cb(struct k_work *_work)
{
	struct k_work_delayable *work = k_work_delayable_from_work(_work);
	struct dali_pwm_data *data = CONTAINER_OF(work, struct dali_pwm_data, send_work);

	k_timeout_t timeout = process_pwm_sendout(data->dev);
	if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		k_work_reschedule(work, timeout);
	}
}
#endif

static int dali_pwm_init(const struct device *dev)
{
	const struct dali_pwm_config *config = dev->config;
	struct dali_pwm_data *data = dev->data;
	int ret;

	data->dev = dev;

	LOG_DBG("PWM INIT");

	k_msgq_init(&data->frames_queue, data->frames_buffer, sizeof(struct dali_frame),
		    CONFIG_MAX_FRAMES_IN_QUEUE);

#if defined(CONFIG_DALI_PWM_OWN_THREAD)
	ret = k_sem_init(&data->tx_queue_sem, 0, 1);
	if (ret < 0) {
		LOG_ERR("Could not initialize send messagequeue semaphore.");
		return ret;
	}

	k_thread_create(&data->thread, data->thread_stack, CONFIG_DALI_PWM_THREAD_STACK_SIZE,
			dali_tx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_DALI_PWM_THREAD_PRIORITY), 0, K_NO_WAIT);
#else
	k_work_init_delayable(&data->send_work, dali_tx_work_cb);
#endif

	ret = k_sem_init(&data->tx_pwm_sem, 0, 1);
	if (ret < 0) {
		LOG_ERR("Could not initialize PWM semaphore.");
		return ret;
	}

	if (!pwm_is_ready_dt(&config->tx_pwm)) {
		LOG_ERR("PWM device %s is not ready", config->tx_pwm.dev->name);
		return -ENODEV;
	}

	if (!pwm_is_ready_dt(&config->rx_capture_pwm)) {
		LOG_ERR("PWM device %s is not ready", config->rx_capture_pwm.dev->name);
		return -ENODEV;
	}

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
	data->bit_time_half_cyc = cycles_per_sec * DALI_TX_BIT_TIME_HALF / NSEC_PER_SEC;
	data->tx_shift_cyc =
		((int64_t)cycles_per_sec) * ((int64_t)config->tx_shift_ns) / NSEC_PER_SEC;

	pwm_get_cycles_per_sec(config->rx_capture_pwm.dev, config->rx_capture_pwm.channel,
			       &cycles_per_sec);

	/* dali bit timings should be accurate to about 5 us => 200khz */
	if (cycles_per_sec < 200000) {
		LOG_ERR("Capture timer is not accurate enough. Need at least 200kHz. Have %lldHz",
			cycles_per_sec);
		return -ERANGE;
	}
	/* TODO(anyone) check if there is a possibility to see if it is an 8, 16, 32bit timer. */
	/* Calculate then if we can capture the longest period we need with it. */

	data->timings.half_min = cycles_per_sec * DALI_RX_BIT_TIME_HALF_MIN / NSEC_PER_SEC;
	data->timings.half_max = cycles_per_sec * DALI_RX_BIT_TIME_HALF_MAX / NSEC_PER_SEC;
	data->timings.full_min = cycles_per_sec * DALI_RX_BIT_TIME_FULL_MIN / NSEC_PER_SEC;
	data->timings.full_max = cycles_per_sec * DALI_RX_BIT_TIME_FULL_MAX / NSEC_PER_SEC;
	data->timings.corrupt_min = cycles_per_sec * DALI_RX_BIT_TIME_CORRUPT_MIN / NSEC_PER_SEC;
	data->timings.corrupt_max = cycles_per_sec * DALI_RX_BIT_TIME_CORRUPT_MAX / NSEC_PER_SEC;
	data->timings.stop_time = cycles_per_sec * DALI_RX_BIT_TIME_STOP / NSEC_PER_SEC;
	data->timings.rx_flank_shift =
		((int64_t)cycles_per_sec) * ((int64_t)config->rx_shift_ns) / NSEC_PER_SEC;

	ret = pwm_configure_capture(config->rx_capture_pwm.dev, config->rx_capture_pwm.channel,
				    PWM_CAPTURE_MODE_CONTINUOUS | PWM_CAPTURE_TYPE_BOTH |
					    config->rx_capture_pwm.flags,
				    continuous_capture_callback, data);
	if (ret < 0) {
		LOG_ERR("Could not configure capture. %s", strerror(-ret));
		return ret;
	}

	ret = pwm_enable_capture(config->rx_capture_pwm.dev, config->rx_capture_pwm.channel);
	if (ret < 0) {
		LOG_ERR("Could not configure capture. %s", strerror(-ret));
		return ret;
	}

	return 0;
}

static const struct dali_driver_api dali_pwm_driver_api = {
	.recv = dali_pwm_recv,
	.send = dali_pwm_send,
	.abort = dali_pwm_abort,
};

/* clang-format off */
#define DALI_PWM_INST(id)                                                                      \
	BUILD_ASSERT(DT_INST_PROP_OR(id, tx_flank_shift_ns, 0) < DALI_TX_BIT_TIME_HALF ||          \
			     (DT_INST_PROP_OR(id, tx_flank_shift_ns, 0) ^ 0xffffffff) <                    \
				     DALI_TX_BIT_TIME_HALF,                                                    \
		     "Edge-shift must be lower than 416us.");                                          \
	static struct dali_pwm_data dali_pwm_data_##id = {                                         \
		.backward_frame.signal_length = 0,                                                     \
		.forward_frame.signal_length = 0,                                                      \
		.capture.state = PWM_CAPTURE_STATE_IDLE,                                               \
		.capture.data = 0,                                                                     \
		.capture.length = 0,                                                                   \
		.frame_finish_work = Z_WORK_DELAYABLE_INITIALIZER(finish_frame_work),                  \
		.no_response_work = Z_WORK_DELAYABLE_INITIALIZER(no_response_work),                    \
	};                                                                                         \
	static const struct dali_pwm_config dali_pwm_config_##id = {                               \
		.rx_capture_pwm = PWM_DT_SPEC_INST_GET_BY_IDX(id, 1),                                  \
		.tx_pwm = PWM_DT_SPEC_INST_GET_BY_IDX(id, 0),                                          \
		.tx_shift_ns = DT_INST_PROP_OR(id, tx_flank_shift_ns, 0),                              \
		.rx_shift_ns = DT_INST_PROP_OR(id, rx_flank_shift_ns, 0),                              \
		.tx_rx_delay_us = DT_INST_PROP_OR(id, tx_rx_delay_us, 50),                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(id, dali_pwm_init, NULL, &dali_pwm_data_##id, &dali_pwm_config_##id, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                             \
			      &dali_pwm_driver_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(DALI_PWM_INST);
