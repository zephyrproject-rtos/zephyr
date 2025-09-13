/*
 * Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_dali_lpc11u6x

#include <zephyr/drivers/dali.h> /* api */

#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dali_low_level, CONFIG_DALI_LOW_LEVEL_LOG_LEVEL);

#include "lpc11u6x.h" /* chip register definitions */
#include "timings.h"  /* timing constants from DALI standard */

#define DALI_TIMER_RATE_HZ      (1000000U)
#define DALI_RX_PORT            (0U)
#define DALI_RX_PIN             (12U)
#define DALI_RX_BIT             (1U << DALI_RX_PIN)
#define DALI_TX_PORT            (0U)
#define DALI_TX_PIN             (11U)
#define DALI_TX_BIT             (1U << DALI_TX_PIN)
#define GREY_AREA_BITTIMING_US  (18U)
#define GREY_AREA_INTERFRAME_US (800U)
#define DALI_TX_IDLE            false
#define DALI_TX_ACTIVE          true

#define DALI_MAX_DATA_LENGTH (32U)
/* start bit, 32 data bits, 1 stop bit */
#define COUNT_ARRAY_SIZE     (2U + DALI_MAX_DATA_LENGTH * 2U + 1U)

#define EXTEND_CORRUPT_PHASE 2
#define TX_CORRUPT_BIT_US    ((DALI_TX_CORRUPT_BIT_MAX_US + DALI_TX_CORRUPT_BIT_MIN_US) / 2)

static struct k_work_q dali_work_queue;
static K_KERNEL_STACK_DEFINE(dali_work_queue_stack, CONFIG_DALI_LPC11U6X_STACK_SIZE);

/**
 * @brief States for receive state machine.
 */
enum rx_state {
	IDLE,
	START_BIT_START,
	START_BIT_INSIDE,
	DATA_BIT_START,
	DATA_BIT_INSIDE,
	ERROR_IN_FRAME,
	DESTROY_FRAME,
	BUS_LOW,
	BUS_FAILURE_DETECT,
	TRANSMIT_BACKFRAME,
	STOPBIT_BACKFRAME,
};

enum rx_counter_event {
	CAPTURE,
	STOPBIT,
};

struct dali_lpc11u6x_tx_callback {
	dali_tx_callback_t function;
	void *user_data;
};

struct dali_lpc11u6x_rx_callback {
	dali_rx_callback_t function;
	void *user_data;
};

struct dali_lpc11u6x_callbacks {
	struct dali_lpc11u6x_tx_callback tx;
	struct dali_lpc11u6x_rx_callback rx;
};

struct dali_lpc11u6x_tx_data {
	uint32_t count[COUNT_ARRAY_SIZE];
	uint_fast8_t index_next;
	uint_fast8_t index_max;
	bool state_now;
	bool collision_detection;
	bool transmission_stop;
};

struct dali_lpc11u6x_rx_data {
	enum rx_state status;
	uint32_t data;
	uint8_t frame_length;
	bool last_data_bit;
	uint32_t tx_count_on_capture;
	uint32_t last_edge_count;
	uint32_t edge_count;
	enum rx_counter_event event;
};

struct dali_lpc11u6x_config {
	int32_t tx_rise_fall_delta_us;
	int32_t rx_rise_fall_delta_us;
	int32_t tx_rx_propagation_min_us;
	int32_t tx_rx_propagation_max_us;
	void (*irq_config_function)(void);
};

struct dali_lpc11u6x_data {
	const struct device *dev;
	struct dali_lpc11u6x_callbacks cb;
	struct dali_lpc11u6x_tx_data tx;
	struct dali_lpc11u6x_rx_data rx;
	struct k_work rx_work;
	struct k_mutex tx_mutex;
};

enum toggle_change {
	NOTHING,
	DISABLE_TOGGLE
};

static void dali_lpc11u6x_init_peripheral_clock(void)
{
	LPC_SYSCON->SYSAHBCLKCTRL |= (SYSAHBCLKCTRL_CT32B0 | SYSAHBCLKCTRL_CT32B1);
}

static void dali_lpc11u6x_init_io_pins(void)
{
	LPC_GPIO_PORT->dir[DALI_TX_PORT] |= DALI_TX_BIT;
	LPC_IOCON->pio0_12 =
		(IOCON_DAPIN_FUNC_MASK & IOCON_PIN0_12_FUNC_PIO) << IOCON_DAPIN_FUNC_SHIFT |
		(IOCON_DAPIN_MODE_MASK & IOCON_DAPIN_MODE_PULLUP) << IOCON_DAPIN_MODE_SHIFT |
		(IOCON_DAPIN_SMODE_MASK & 3U) << IOCON_DAPIN_SMODE_SHIFT |
		(IOCON_DAPIN_CLKDIV_MASK & 6U) << IOCON_DAPIN_CLKDIV_SHIFT;
}

static bool dali_lpc11u6x_get_rx_pin(void)
{
	return !(LPC_GPIO_PORT->pin[DALI_RX_PORT] & DALI_RX_BIT);
}

static void dali_lpc11u6x_set_tx_pin(bool state)
{
	LPC_IOCON->pio0_11 =
		(IOCON_DAPIN_FUNC_MASK & IOCON_PIN0_11_FUNC_PIO) << IOCON_DAPIN_FUNC_SHIFT |
		(IOCON_DAPIN_MODE_MASK & IOCON_DAPIN_MODE_PULLDOWN) << IOCON_DAPIN_MODE_SHIFT |
		(IOCON_DAPIN_ADMODE);
	if (state) {
		LPC_GPIO_PORT->set[DALI_TX_PORT] |= DALI_TX_BIT;
	} else {
		LPC_GPIO_PORT->clr[DALI_TX_PORT] |= DALI_TX_BIT;
	}
}

static void dali_lpc11u6x_stop_tx_counter(void)
{
	LPC_CT32B0->mcr = (LPC_CT32B0->mcr &
			   ~(CT32_MCR_MR0I | CT32_MCR_MR3I | CT32_MCR_MR3R | CT32_MCR_MR3S));
	LPC_CT32B0->emr = (CT32_EMR_EM2 | (CT32_EMR_EMCTR_NOTHING << CT32_EMR_EMC2_SHIFT));
}

static void dali_lpc11u6x_set_next_match_tx_counter(uint32_t count, enum toggle_change toggle)
{
	LPC_CT32B0->mr3 = count;
	if (toggle == DISABLE_TOGGLE) {
		LPC_CT32B0->emr &= ~(CT32_EMR_EMCTR_TOGGLE << CT32_EMR_EMC3_SHIFT);
	}
}

static uint32_t dali_lpc11u6x_get_tx_counter(void)
{
	return LPC_CT32B0->tc;
}

static void dali_lpc11u6x_set_collision_counter(uint32_t count)
{
	LPC_CT32B0->mr0 = count;
}

static void dali_lpc11u6x_init_tx_counter(uint32_t count, bool check_collision)
{
	LPC_CT32B0->tcr = CT32_TCR_CRST;
	dali_lpc11u6x_stop_tx_counter();
	dali_lpc11u6x_set_next_match_tx_counter(count, NOTHING);
	/* set prescaler to base rate */
	LPC_CT32B0->pr = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / DALI_TIMER_RATE_HZ) - 1;
	/* set timer mode */
	LPC_CT32B0->ctcr = 0;
	/* on MR3 match: IRQ */
	LPC_CT32B0->mcr = (LPC_CT32B0->mcr & ~(CT32_MCR_MR3I | CT32_MCR_MR3R | CT32_MCR_MR3S)) |
			  (CT32_MCR_MR3I);
	/* for collision checking enable MR0 */
	if (check_collision) {
		LPC_CT32B0->mcr |= CT32_MCR_MR0I;
	} else {
		LPC_CT32B0->mcr &= ~CT32_MCR_MR0I;
	}
	/* on MR3 match: toggle output - start with DALI active */
	LPC_CT32B0->emr = CT32_EMR_EM3 | (CT32_EMR_EMCTR_TOGGLE << CT32_EMR_EMC3_SHIFT);
	/* outputs are controlled by EMx */
	LPC_CT32B0->pwmc = 0;
	/* pin function: CT32B0_MAT3 */
	/* function mode: no pull up/down resistors */
	/* hysteresis disabled */
	/* standard gpio */
	LPC_IOCON->pio0_11 =
		(IOCON_DAPIN_FUNC_MASK & IOCON_PIN0_11_FUNC_MAT3) << IOCON_DAPIN_FUNC_SHIFT |
		(IOCON_DAPIN_MODE_MASK & IOCON_DAPIN_MODE_PULLDOWN) << IOCON_DAPIN_MODE_SHIFT |
		(IOCON_DAPIN_ADMODE);
	/* start timer */
	LPC_CT32B0->tcr = CT32_TCR_CEN;
}

static uint32_t dali_lpc116x_get_rx_counter(void)
{
	return LPC_CT32B1->tc;
}

static uint32_t dali_lpc11u6x_get_rx_capture(void)
{
	return LPC_CT32B1->cr0;
}

static void dali_lpc11u6x_set_rx_counter(uint32_t match_count)
{
	LPC_CT32B1->mr0 = match_count;
}

static void dali_lpc11u6x_enable_rx_counter(bool enable)
{
	if (enable) {
		LPC_CT32B1->mcr |= (CT32_MCR_MR0I);
	} else {
		LPC_CT32B1->mcr &= ~(CT32_MCR_MR0I);
	}
}

void dali_lpc11u6x_init_rx_counter(void)
{
	LPC_CT32B1->tcr = CT32_TCR_CRST;
	/* set prescaler to base rate */
	LPC_CT32B1->pr = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / DALI_TIMER_RATE_HZ) - 1;
	/* set timer mode */
	LPC_CT32B1->ctcr = 0;
	/* capture both edges, trigger IRQ */
	LPC_CT32B1->ccr = (CT32_CCR_CAP0FE | CT32_CCR_CAP0RE | CT32_CCR_CAP0I);
	/* disable event matches */
	dali_lpc11u6x_enable_rx_counter(false);
	/* pin function: CT32B1_CAP0 */
	/* function mode: enable pull up resistor */
	/* hysteresis disabled */
	/* digital function mode, standard gpio */
	LPC_IOCON->pio0_12 =
		(IOCON_DAPIN_FUNC_MASK & IOCON_PIN0_12_FUNC_CAP0) << IOCON_DAPIN_FUNC_SHIFT |
		(IOCON_DAPIN_MODE_MASK & IOCON_DAPIN_MODE_PULLDOWN) << IOCON_DAPIN_MODE_SHIFT |
		(IOCON_DAPIN_ADMODE);
	/* start timer */
	LPC_CT32B1->tcr = CT32_TCR_CEN;
}

static void dali_lpc11u6x_add_signal_phase(struct dali_lpc11u6x_tx_data *tx, uint32_t duration_us,
					   bool change_last_phase)
{
	if (tx->index_max >= COUNT_ARRAY_SIZE || (change_last_phase && tx->index_max == 0)) {
		__ASSERT(true, "Access beyond array bounds");
		return;
	}

	if (change_last_phase) {
		tx->index_max--;
	}
	uint32_t count_now =
		(tx->index_max) ? (tx->count[tx->index_max - 1] + duration_us) : duration_us;
	tx->count[tx->index_max++] = count_now;
}

static void dali_lpc11u6x_add_bit(const struct device *dev, bool value)
{
	const struct dali_lpc11u6x_config *config = dev->config;
	struct dali_lpc11u6x_data *data = dev->data;

	uint32_t phase_one;
	uint32_t phase_two;
	bool change_previous = false;

	if (data->tx.state_now == value) {
		if (data->tx.state_now) {
			phase_one = DALI_TX_HALF_BIT_US + config->tx_rise_fall_delta_us;
			phase_two = DALI_TX_HALF_BIT_US - config->tx_rise_fall_delta_us;
		} else {
			phase_one = DALI_TX_HALF_BIT_US - config->tx_rise_fall_delta_us;
			phase_two = DALI_TX_HALF_BIT_US + config->tx_rise_fall_delta_us;
		}
	} else {
		change_previous = true;
		if (data->tx.state_now) {
			phase_one = DALI_TX_FULL_BIT_US - config->tx_rise_fall_delta_us;
			phase_two = DALI_TX_HALF_BIT_US + config->tx_rise_fall_delta_us;
		} else {
			phase_one = DALI_TX_FULL_BIT_US + config->tx_rise_fall_delta_us;
			phase_two = DALI_TX_HALF_BIT_US - config->tx_rise_fall_delta_us;
		}
	}
	dali_lpc11u6x_add_signal_phase(&data->tx, phase_one, change_previous);
	dali_lpc11u6x_add_signal_phase(&data->tx, phase_two, false);
	data->tx.state_now = value;
}

static void dali_lpc11u6x_add_stop_condition(struct dali_lpc11u6x_tx_data *tx)
{
	if (tx->state_now) {
		dali_lpc11u6x_add_signal_phase(tx, DALI_TX_STOP_BIT_US, true);
		tx->index_max--;
	} else {
		dali_lpc11u6x_add_signal_phase(tx, DALI_TX_STOP_BIT_US, false);
		tx->index_max--;
	}
}

static int dali_lpc11u6x_calculate_counts(const struct device *dev, const struct dali_frame *frame)
{
	uint_fast8_t length = 0;
	struct dali_lpc11u6x_data *data = dev->data;

	switch (frame->event_type) {
	case DALI_FRAME_CORRUPT:
		for (int_fast8_t i = 0; i < (2 * DALI_FRAME_BACKWARD_LENGTH); i++) {
			if (i == EXTEND_CORRUPT_PHASE) {
				dali_lpc11u6x_add_signal_phase(&data->tx, TX_CORRUPT_BIT_US, false);
			} else {
				dali_lpc11u6x_add_signal_phase(&data->tx, DALI_TX_HALF_BIT_US,
							       false);
			}
		}
		data->tx.collision_detection = false;
		return 0;
	case DALI_FRAME_BACKWARD:
		length = DALI_FRAME_BACKWARD_LENGTH;
		data->tx.collision_detection = false;
		break;
	case DALI_FRAME_GEAR:
		length = DALI_FRAME_GEAR_LENGTH;
		data->tx.collision_detection = true;
		break;
	case DALI_FRAME_DEVICE:
		length = DALI_FRAME_DEVICE_LENGTH;
		data->tx.collision_detection = true;
		break;
	default:
		return -EINVAL;
	}

	/* add start bit */
	dali_lpc11u6x_add_bit(dev, true);
	/* add data bits */
	for (int_fast8_t i = (length - 1); i >= 0; i--) {
		dali_lpc11u6x_add_bit(dev, frame->data & (1 << i));
	}

	dali_lpc11u6x_add_stop_condition(&data->tx);

	return 0;
}

static void dali_lpc11u6x_stop_tx(struct dali_lpc11u6x_data *data)
{
	if (data->tx.collision_detection && data->tx.index_next) {
		data->tx.transmission_stop = true;
		dali_lpc11u6x_stop_tx_counter();
		dali_lpc11u6x_set_tx_pin(DALI_TX_IDLE);
	}
}

static void dali_lpc11u6x_destroy_frame(struct dali_lpc11u6x_data *data)
{
	if (data->rx.status == DESTROY_FRAME) {
		return;
	}
	dali_lpc11u6x_stop_tx_counter();
	dali_lpc11u6x_set_tx_pin(DALI_TX_ACTIVE);
	data->rx.status = DESTROY_FRAME;

	/* use stopbit counter to time break condition. The timing is rather unrelieable and easily
	 * disturbed - it would be better to utilize the tx pwm
	 */
	const uint32_t break_count = dali_lpc116x_get_rx_counter() + DALI_TX_BREAK_MIN_US;

	dali_lpc11u6x_set_rx_counter(break_count);
	dali_lpc11u6x_enable_rx_counter(true);
}

void dali_lpc11u6x_handle_tx_callback_irq(const struct device *dev)
{
	struct dali_lpc11u6x_data *data = dev->data;
	const struct dali_lpc11u6x_config *config = data->dev->config;
	struct dali_lpc11u6x_tx_data *tx = &data->tx;

	/* schedule collision check for current transition */
	if (tx->collision_detection) {
		const uint32_t last_transition = tx->count[tx->index_next - 1];

		dali_lpc11u6x_set_collision_counter(last_transition +
						    config->tx_rx_propagation_min_us);
		tx->state_now = !tx->state_now;
	}

	/* schedule next level transition */
	if (tx->index_next < tx->index_max) {
		const uint32_t next_transition = tx->count[tx->index_next++];

		dali_lpc11u6x_set_next_match_tx_counter(next_transition, NOTHING);
		return;
	}

	/* schedule the last transition */
	if (tx->index_next == tx->index_max) {
		const uint32_t next_transition = tx->count[tx->index_next++];

		dali_lpc11u6x_set_next_match_tx_counter(next_transition, DISABLE_TOGGLE);
		if (data->rx.status == TRANSMIT_BACKFRAME) {
			data->rx.status = STOPBIT_BACKFRAME;
		}
		return;
	}

	/* activities at end of frame */
	dali_lpc11u6x_set_tx_pin(DALI_TX_IDLE);
	dali_lpc11u6x_stop_tx_counter();
}

void dali_lpc11u6x_handle_collision_callback_irq(const struct device *dev)
{
	struct dali_lpc11u6x_data *data = dev->data;

	if (data->tx.index_next && dali_lpc11u6x_get_rx_pin() == data->tx.state_now) {
		dali_lpc11u6x_stop_tx(data);
		LOG_ERR("unexpected bus state while sending period %d -- stop transmission",
			data->tx.index_next);
	}
}

static void dali_lpc11u6x_handle_tx_irq(const struct device *dev)
{
	if (LPC_CT32B0->ir & CT32_IR_MR3INT) {
		LPC_CT32B0->ir = CT32_IR_MR3INT;
		dali_lpc11u6x_handle_tx_callback_irq(dev);
	}
	if (LPC_CT32B0->ir & CT32_IR_MR0INT) {
		LPC_CT32B0->ir = CT32_IR_MR0INT;
		dali_lpc11u6x_handle_collision_callback_irq(dev);
	}
}

static void dali_lpc11u6x_start_tx(struct dali_lpc11u6x_tx_data *tx)
{
	LOG_DBG("start_tx - index_max: %d", tx->index_max);
	tx->index_next = 1;
	tx->state_now = true;
	tx->transmission_stop = false;
	dali_lpc11u6x_init_tx_counter(tx->count[0], tx->collision_detection);
}

static void dali_lpc11u6x_reset_tx(struct dali_lpc11u6x_data *data)
{
	/* reset callbacks */
	data->cb.tx = (struct dali_lpc11u6x_tx_callback){0};
	/* reset pwm counts */
	data->tx = (struct dali_lpc11u6x_tx_data){.state_now = true};
}

static void dali_lpc11u6x_finish_rx_frame(struct dali_lpc11u6x_data *data)
{
	struct dali_frame frame = {
		.data = data->rx.data,
		.event_type = DALI_EVENT_NONE,
	};
	bool rx_cb = false;
	bool tx_cb = data->tx.transmission_stop;
	int err = tx_cb ? -ECANCELED : 0;

	switch (data->rx.status) {
	case START_BIT_START:
	case START_BIT_INSIDE:
	case DATA_BIT_START:
	case DATA_BIT_INSIDE:
		LOG_INF("{%08x:%02x %08x}", k_uptime_get_32(), data->rx.frame_length,
			data->rx.data);
		switch (data->rx.frame_length) {
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
			LOG_INF("invalid frame length %d bits", data->rx.frame_length);
			frame.data = 0, frame.event_type = DALI_FRAME_CORRUPT;
		}
		data->rx.status = IDLE;
		if (data->tx.collision_detection) {
			tx_cb = true;
		}
		rx_cb = true;
		break;
	case ERROR_IN_FRAME:
		if (data->tx.transmission_stop) {
			err = -ECOMM;
		} else {
			frame.data = 0, frame.event_type = DALI_FRAME_CORRUPT;
		}
		tx_cb = true;
		rx_cb = true;
		data->rx.status = IDLE;
		break;
	case BUS_FAILURE_DETECT:
		LOG_INF("bus failure");
		frame.data = 0, frame.event_type = DALI_EVENT_BUS_FAILURE;
		err = -EAGAIN;
		rx_cb = true;
		tx_cb = true;
		break;
	case IDLE:
		LOG_INF("bus idle");
		frame.data = 0, frame.event_type = DALI_EVENT_BUS_IDLE;
		rx_cb = true;
		break;
	default:
		__ASSERT(false, "invalid state");
	}
	if (data->cb.tx.function && tx_cb) {
		(data->cb.tx.function)(data->dev, err, data->cb.tx.user_data);
	}
	if (data->cb.rx.function && rx_cb) {
		(data->cb.rx.function)(data->dev, frame, data->cb.rx.user_data);
	}
}

static bool dali_lpc11u6x_is_valid_halfbit_timing(const uint32_t time_difference_us)
{
	if ((time_difference_us > DALI_RX_BIT_TIME_HALF_MAX_US + GREY_AREA_BITTIMING_US) ||
	    (time_difference_us < DALI_RX_BIT_TIME_HALF_MIN_US - GREY_AREA_BITTIMING_US)) {
		return false;
	}
	return true;
}

static bool dali_lpc11u6x_is_valid_fullbit_timing(const uint32_t time_difference_us)
{
	if ((time_difference_us > DALI_RX_BIT_TIME_FULL_MAX_US + GREY_AREA_BITTIMING_US) ||
	    (time_difference_us < DALI_RX_BIT_TIME_FULL_MIN_US - GREY_AREA_BITTIMING_US)) {
		return false;
	}
	return true;
}

static bool dali_lpc11u6x_is_destroy_start(const uint32_t time_difference_us)
{
	if ((time_difference_us > DALI_TX_DESTROY_1_MIN_US - GREY_AREA_BITTIMING_US) &&
	    (time_difference_us < DALI_TX_DESTROY_1_MAX_US + GREY_AREA_BITTIMING_US)) {
		return true;
	}
	if ((time_difference_us > DALI_TX_DESTROY_2_MIN_US + GREY_AREA_BITTIMING_US)) {
		return true;
	}
	return false;
}

static bool dali_lpc11u6x_is_destroy_inside(const uint32_t time_difference_us)
{
	if ((time_difference_us > DALI_TX_DESTROY_1_MIN_US - GREY_AREA_BITTIMING_US) &&
	    (time_difference_us < DALI_TX_DESTROY_1_MAX_US + GREY_AREA_BITTIMING_US)) {
		return true;
	}
	if ((time_difference_us > DALI_TX_DESTROY_2_MIN_US - GREY_AREA_BITTIMING_US) &&
	    (time_difference_us < DALI_TX_DESTROY_2_MAX_US + GREY_AREA_BITTIMING_US)) {
		return true;
	}
	if ((time_difference_us > DALI_TX_DESTROY_3_MIN_US - GREY_AREA_BITTIMING_US)) {
		return true;
	}
	return false;
}

static uint32_t dali_lpc11u6x_get_time_difference_us(const struct dali_lpc11u6x_data *data,
						     bool invert)
{
	const struct dali_lpc11u6x_config *config = data->dev->config;
	const uint32_t time_difference_us = data->rx.edge_count - data->rx.last_edge_count;

	if (data->rx.last_data_bit != invert) {
		return time_difference_us - config->rx_rise_fall_delta_us;
	}
	return time_difference_us + config->rx_rise_fall_delta_us;
}

static void dali_lpc11u6x_set_status(struct dali_lpc11u6x_data *data, enum rx_state new_status)
{
	switch (data->rx.status) {
	case ERROR_IN_FRAME:
	case DESTROY_FRAME:
		return;
	default:
		data->rx.status = new_status;
	}
}

static void dali_lpc11u6x_add_bit_to_rx_data(struct dali_lpc11u6x_data *data)
{
	data->rx.data = (data->rx.data << 1U) | (data->rx.last_data_bit ? (1U) : (0U));
	data->rx.frame_length++;
	if (data->rx.frame_length > DALI_MAX_BIT_PER_FRAME) {
		dali_lpc11u6x_set_status(data, ERROR_IN_FRAME);
	}
}

static enum rx_state dali_lpc11u6x_check_start_timing(struct dali_lpc11u6x_data *data)
{
	const uint32_t time_difference_us = dali_lpc11u6x_get_time_difference_us(data, false);

	if (data->tx.collision_detection && dali_lpc11u6x_is_destroy_start(time_difference_us)) {
		dali_lpc11u6x_destroy_frame(data);
		if (data->rx.status == START_BIT_START) {
			LOG_ERR("start bit collision, timing %d us, destroy frame",
				time_difference_us);
		} else {
			LOG_ERR("data bit %d collision, timing %d us, destroy frame",
				data->rx.frame_length, time_difference_us);
		}
		return DESTROY_FRAME;
	}

	if (!dali_lpc11u6x_is_valid_halfbit_timing(time_difference_us)) {
		dali_lpc11u6x_set_status(data, ERROR_IN_FRAME);
		if (data->rx.status == START_BIT_START) {
			LOG_ERR("start bit timing %d us, corrupt frame", time_difference_us);
		} else {
			LOG_ERR("data bit %d timing %d us, corrupt frame", data->rx.frame_length,
				time_difference_us);
		}
		return START_BIT_INSIDE;
	}

	if (data->rx.status == DATA_BIT_START) {
		dali_lpc11u6x_add_bit_to_rx_data(data);
		return DATA_BIT_INSIDE;
	}
	return START_BIT_INSIDE;
}

static enum rx_state dali_lpc11u6x_check_inside_timing(struct dali_lpc11u6x_data *data)
{
	uint32_t time_difference_us = dali_lpc11u6x_get_time_difference_us(data, true);

	if (data->tx.collision_detection && dali_lpc11u6x_is_destroy_inside(time_difference_us)) {
		dali_lpc11u6x_destroy_frame(data);
		if (data->rx.status == START_BIT_INSIDE) {
			LOG_ERR("inside start bit collision, timing %d us, destroy frame",
				time_difference_us);
		} else {
			LOG_ERR("inside bit %d collision, timing %d us, destroy frame",
				data->rx.frame_length, time_difference_us);
		}
		return DESTROY_FRAME;
	}

	if (dali_lpc11u6x_is_valid_halfbit_timing(time_difference_us)) {
		return DATA_BIT_START;
	}
	if (dali_lpc11u6x_is_valid_fullbit_timing(time_difference_us)) {
		data->rx.last_data_bit = !data->rx.last_data_bit;
		dali_lpc11u6x_add_bit_to_rx_data(data);
		return DATA_BIT_INSIDE;
	}
	if (data->rx.status == START_BIT_INSIDE) {
		LOG_ERR("inside start bit timing error %d us", time_difference_us);
	} else {
		LOG_ERR("inside data bit %d timing error %d us", data->rx.frame_length,
			time_difference_us);
	}
	return ERROR_IN_FRAME;
}

static void dali_lpc11u6x_check_collision(struct dali_lpc11u6x_data *data)
{
	if (!data->tx.collision_detection) {
		return;
	}

	const struct dali_lpc11u6x_config *config = data->dev->config;
	const uint32_t expected_count = data->tx.count[data->tx.index_next - 2];
	const int32_t delay = data->rx.tx_count_on_capture - expected_count;

	if (delay < 0 || delay > config->tx_rx_propagation_max_us) {
		dali_lpc11u6x_stop_tx(data);
		LOG_ERR("unexpected capture with delay of %d us while receiving bit %d, stop "
			"transmission",
			delay, data->rx.frame_length);
	}
}

static void dali_lpc11u6x_process_capture_event(struct dali_lpc11u6x_data *data)
{
	data->rx.edge_count = dali_lpc11u6x_get_rx_capture();

	const uint32_t stop_timeout_count =
		data->rx.edge_count + DALI_RX_BIT_TIME_STOP_US - GREY_AREA_INTERFRAME_US;

	dali_lpc11u6x_set_rx_counter(stop_timeout_count);
	dali_lpc11u6x_enable_rx_counter(true);

	if (data->rx.status == DESTROY_FRAME || data->rx.status == ERROR_IN_FRAME) {
		data->rx.last_edge_count = dali_lpc116x_get_rx_counter();
		return;
	}

	if (data->rx.status == TRANSMIT_BACKFRAME || data->rx.status == STOPBIT_BACKFRAME) {
		data->rx.last_edge_count = data->rx.edge_count;
		return;
	}

	switch (data->rx.status) {
	case IDLE:
		if (!dali_lpc11u6x_get_rx_pin()) {
			dali_lpc11u6x_set_status(data, START_BIT_START);
			data->rx.last_data_bit = true;
			data->rx.data = 0;
			data->rx.frame_length = 0;
		}
		break;
	case START_BIT_START:
	case DATA_BIT_START:
		dali_lpc11u6x_check_collision(data);
		dali_lpc11u6x_set_status(data, dali_lpc11u6x_check_start_timing(data));
		break;
	case START_BIT_INSIDE:
	case DATA_BIT_INSIDE:
		dali_lpc11u6x_check_collision(data);
		dali_lpc11u6x_set_status(data, dali_lpc11u6x_check_inside_timing(data));
		break;
	case STOPBIT_BACKFRAME:
		break;
	case BUS_LOW:
		if (dali_lpc11u6x_get_rx_pin()) {
			dali_lpc11u6x_set_status(data, ERROR_IN_FRAME);
			dali_lpc11u6x_finish_rx_frame(data);
		}
		break;
	case BUS_FAILURE_DETECT:
		if (dali_lpc11u6x_get_rx_pin()) {
			data->rx.status = IDLE;
			dali_lpc11u6x_finish_rx_frame(data);
		}
		break;
	default:
		__ASSERT(false, "invalid state");
	}
	data->rx.last_edge_count = data->rx.edge_count;
}

static void dali_lpc11u6x_process_stopbit_event(struct dali_lpc11u6x_data *data)
{
	/* this can happen with extensive long bus active periods */
	if (data->rx.status == TRANSMIT_BACKFRAME) {
		/* re-start counter */
		const uint32_t stop_timeout_count =
			data->rx.edge_count + DALI_RX_BIT_TIME_STOP_US - GREY_AREA_INTERFRAME_US;

		dali_lpc11u6x_set_rx_counter(stop_timeout_count);
		dali_lpc11u6x_enable_rx_counter(true);
		return;
	}

	if (data->rx.status == DESTROY_FRAME) {
		dali_lpc11u6x_set_tx_pin(DALI_TX_IDLE);
		data->rx.status = ERROR_IN_FRAME;
		/* re-start counter */
		const uint32_t stop_timeout_count =
			data->rx.edge_count + DALI_RX_BIT_TIME_STOP_US - GREY_AREA_INTERFRAME_US;

		dali_lpc11u6x_set_rx_counter(stop_timeout_count);
		dali_lpc11u6x_enable_rx_counter(true);
		return;
	}

	if (dali_lpc11u6x_get_rx_pin()) {
		switch (data->rx.status) {
		case ERROR_IN_FRAME:
		case IDLE:
		case START_BIT_START:
		case START_BIT_INSIDE:
		case DATA_BIT_START:
		case DATA_BIT_INSIDE:
		case BUS_LOW:
		case BUS_FAILURE_DETECT:
			dali_lpc11u6x_finish_rx_frame(data);
			return;
		case STOPBIT_BACKFRAME:
			data->rx.status = IDLE;
			return;
		default:
			__ASSERT(false, "invalid state");
			return;
		}
	}
	switch (data->rx.status) {
	case BUS_LOW:
		data->rx.status = BUS_FAILURE_DETECT;
		dali_lpc11u6x_finish_rx_frame(data);
		break;
	case BUS_FAILURE_DETECT:
		break;
	default:
		dali_lpc11u6x_set_rx_counter(data->rx.edge_count + DALI_FAILURE_CONDITION_US);
		dali_lpc11u6x_enable_rx_counter(true);
		data->rx.status = BUS_LOW;
	}
}

static void dali_lpc11u6x_handle_work_queue(struct k_work *item)
{
	struct dali_lpc11u6x_data *data = CONTAINER_OF(item, struct dali_lpc11u6x_data, rx_work);

	switch (data->rx.event) {
	case CAPTURE:
		dali_lpc11u6x_process_capture_event(data);
		break;
	case STOPBIT:
		dali_lpc11u6x_process_stopbit_event(data);
		break;
	default:
		__ASSERT(false, "invalid event type");
	}
}

static void dali_lpc11u6x_handle_rx_irq(const struct device *dev)
{
	struct dali_lpc11u6x_data *data = dev->data;

	if (LPC_CT32B1->ir & CT32_IR_MR0INT) {
		LPC_CT32B1->ir = CT32_IR_MR0INT;
		dali_lpc11u6x_enable_rx_counter(false);

		data->rx.event = STOPBIT;
		k_work_submit_to_queue(&dali_work_queue, &data->rx_work);
	}
	if (LPC_CT32B1->ir & CT32_IR_CR0INT) {
		LPC_CT32B1->ir = CT32_IR_CR0INT;
		data->rx.event = CAPTURE;
		data->rx.tx_count_on_capture = dali_lpc11u6x_get_tx_counter();
		k_work_submit_to_queue(&dali_work_queue, &data->rx_work);
	}
}

static int dali_lpc11u6x_receive(const struct device *dev, dali_rx_callback_t callback,
				 void *user_data)
{
	struct dali_lpc11u6x_data *data = dev->data;

	LOG_DBG("Register receive callback.");
	data->cb.rx.function = callback;
	data->cb.rx.user_data = user_data;

	return 0;
}

static int dali_lpc11u6x_send(const struct device *dev, const struct dali_frame *frame,
			      dali_tx_callback_t callback, void *user_data)
{
	struct dali_lpc11u6x_data *data = dev->data;
	int ret = 0;

	switch (frame->event_type) {
	case DALI_EVENT_NONE:
		break;
	case DALI_FRAME_GEAR:
	case DALI_FRAME_DEVICE:
	case DALI_FRAME_FIRMWARE:
		if (data->rx.status != IDLE) {
			ret = -EBUSY;
			break;
		}
	case DALI_FRAME_CORRUPT:
	case DALI_FRAME_BACKWARD:
		k_mutex_lock(&data->tx_mutex, K_FOREVER);
		dali_lpc11u6x_reset_tx(data);
		data->cb.tx.function = callback;
		data->cb.tx.user_data = user_data;
		ret = dali_lpc11u6x_calculate_counts(dev, frame);
		if (ret == 0) {
			dali_lpc11u6x_start_tx(&data->tx);
		}
		k_mutex_unlock(&data->tx_mutex);
	default:
		ret = -EINVAL;
	}

	return ret;
}

static void dali_lpc11u6x_abort(const struct device *dev)
{
	dali_lpc11u6x_stop_tx_counter();
	dali_lpc11u6x_set_tx_pin(DALI_TX_IDLE);
}

static int dali_lpc11u6x_init(const struct device *dev)
{
	struct dali_lpc11u6x_data *data = dev->data;
	const struct dali_lpc11u6x_config *config = dev->config;

	/* connect to device */
	data->dev = dev;

	/* configure interrupts */
	if (config->irq_config_function != NULL) {
		config->irq_config_function();
	}

	/* set up send mutex */
	k_mutex_init(&data->tx_mutex);

	/* set up receive work queue */
	const struct k_work_queue_config cfg = {
		.name = "DALI work",
		.no_yield = true,
		.essential = false,
	};
	k_work_queue_start(&dali_work_queue, dali_work_queue_stack,
			   K_KERNEL_STACK_SIZEOF(dali_work_queue_stack),
			   CONFIG_DALI_LPC11U6X_PRIORITY, &cfg);
	k_work_init(&data->rx_work, dali_lpc11u6x_handle_work_queue);

	/* initialize peripherals */
	dali_lpc11u6x_init_peripheral_clock();
	dali_lpc11u6x_init_io_pins();
	dali_lpc11u6x_set_tx_pin(DALI_TX_IDLE);
	dali_lpc11u6x_init_rx_counter();

	/* examine initial bus state */
	if (dali_lpc11u6x_get_rx_pin()) {
		data->rx.status = IDLE;
	} else {
		data->rx.status = BUS_LOW;
		dali_lpc11u6x_set_rx_counter(DALI_FAILURE_CONDITION_US);
		dali_lpc11u6x_enable_rx_counter(true);
	}
	return 0;
}

static DEVICE_API(dali, dali_lpc11u6x_driver_api) = {
	.receive = dali_lpc11u6x_receive,
	.send = dali_lpc11u6x_send,
	.abort = dali_lpc11u6x_abort,
};

#define DALI_LPC11U6X_INIT(idx)                                                                    \
	static void dali_lpc11u6x_config_irq_##idx(void)                                           \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQ_BY_IDX(DT_DRV_INST(idx), 0, irq),                               \
			    DT_IRQ_BY_IDX(DT_DRV_INST(idx), 0, priority),                          \
			    dali_lpc11u6x_handle_tx_irq, DEVICE_DT_INST_GET(idx), 0);              \
		irq_enable(DT_IRQ_BY_IDX(DT_DRV_INST(idx), 0, irq));                               \
		IRQ_CONNECT(DT_IRQ_BY_IDX(DT_DRV_INST(idx), 1, irq),                               \
			    DT_IRQ_BY_IDX(DT_DRV_INST(idx), 1, priority),                          \
			    dali_lpc11u6x_handle_rx_irq, DEVICE_DT_INST_GET(idx), 0);              \
		irq_enable(DT_IRQ_BY_IDX(DT_DRV_INST(idx), 1, irq));                               \
	};                                                                                         \
	static struct dali_lpc11u6x_data dali_lpc11u6x_data_##idx = {0};                           \
	static const struct dali_lpc11u6x_config dali_lpc11u6x_config_##idx = {                    \
		.tx_rise_fall_delta_us = DT_INST_PROP_OR(idx, tx_rise_fall_delta_us, 0),           \
		.rx_rise_fall_delta_us = DT_INST_PROP_OR(idx, rx_rise_fall_delta_us, 0),           \
		.tx_rx_propagation_min_us = DT_INST_PROP_OR(idx, tx_rx_propagation_min_us, 1),     \
		.tx_rx_propagation_max_us = DT_INST_PROP_OR(idx, tx_rx_propagation_max_us, 100),   \
		.irq_config_function = dali_lpc11u6x_config_irq_##idx,                             \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, dali_lpc11u6x_init, NULL, &dali_lpc11u6x_data_##idx,            \
			      &dali_lpc11u6x_config_##idx, POST_KERNEL,                            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dali_lpc11u6x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DALI_LPC11U6X_INIT);
