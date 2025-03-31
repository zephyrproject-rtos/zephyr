#define DT_DRV_COMPAT nxp_dali

#include <dali.h>
#include <dali_std.h>

#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dali_low_level, CONFIG_DALI_LOW_LEVEL_LOG_LEVEL);

/* chip register definitions */
#include <lpc11u6x.h>

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
#define COUNT_ARRAY_SIZE                                                                           \
	(2U + DALI_MAX_DATA_LENGTH * 2U + 1U) /* start bit, 32 data bits, 1 stop bit */

#define EXTEND_CORRUPT_PHASE 2
#define TX_CORRUPT_BIT_US    ((TX_CORRUPT_BIT_MAX_US + TX_CORRUPT_BIT_MIN_US) / 2)
#define TX_BREAK_US          ((TX_BREAK_MAX_US + TX_BREAK_MIN_US) / 2)

static struct k_work_q dali_work_queue;
static K_KERNEL_STACK_DEFINE(dali_work_queue_stack, CONFIG_DALI_LPC11U6X_STACK_SIZE);

/* states for rx state machine */
enum rx_state {
	IDLE,
	START_BIT_START,
	START_BIT_INSIDE,
	DATA_BIT_START,
	DATA_BIT_INSIDE,
	ERROR_IN_FRAME,
	STOP_TRANSMISSION,
	DESTROY_FRAME,
	BUS_LOW,
	BUS_FAILURE_DETECT,
	TRANSMIT_BACKFRAME,
	STOPBIT_BACKFRAME,
};

/* rx counter events */
enum rx_counter_event {
	CAPTURE,
	STOPBIT,
	PRIORITY,
	QUERY,
};

/* see IEC 62386-101:2022 Table 22 - Multi-master transmitter settling time values */
static const uint32_t settling_time_us[] = {
	(TX_BACKWARD_INTERFRAME_MIN_US + (GREY_AREA_INTERFRAME_US / 2)),
	(TX_PRIO_1_INTERFRAME_MIN_US + GREY_AREA_INTERFRAME_US),
	(TX_PRIO_2_INTERFRAME_MIN_US + GREY_AREA_INTERFRAME_US),
	(TX_PRIO_3_INTERFRAME_MIN_US + GREY_AREA_INTERFRAME_US),
	(TX_PRIO_4_INTERFRAME_MIN_US + GREY_AREA_INTERFRAME_US),
	(TX_PRIO_5_INTERFRAME_MIN_US + GREY_AREA_INTERFRAME_US),
	(TX_STOP_BIT_US + GREY_AREA_INTERFRAME_US)};

struct dali_tx_slot {
	uint32_t count[COUNT_ARRAY_SIZE];
	uint_fast8_t index_next;
	uint_fast8_t index_max;
	bool state_now;
	bool is_query;
	uint32_t inter_frame_idle;
};

struct dali_lpc11u6x_config {
	int32_t tx_rise_fall_delta_us;
	int32_t rx_rise_fall_delta_us;
	int32_t tx_rx_propagation_min_us;
	int32_t tx_rx_propagation_max_us;
};

struct dali_lpc11u6x_data {
	const struct dali_lpc11u6x_config *config;
	struct dali_tx_slot forward;
	struct dali_tx_slot backward;
	struct dali_tx_slot *active;
	enum rx_state rx_status;
	uint32_t last_edge_count;
	uint32_t last_full_frame_count;
	uint32_t edge_count;
	bool last_data_bit;
	struct k_work rx_work;
	enum rx_counter_event rx_event;
	struct k_msgq rx_queue;
	char rx_buffer[CONFIG_MAX_FRAMES_IN_QUEUE * sizeof(struct dali_frame)];
	uint32_t rx_data;
	uint32_t rx_timestamp;
	uint8_t rx_frame_length;
	uint32_t rx_last_timestamp;
	uint32_t rx_last_payload;
	uint32_t rx_last_frame_length;
	uint32_t tx_count_on_capture;
};

enum board_toggle {
	NOTHING,
	DISABLE_TOGGLE
};

static void mcu_setup_peripheral_clock(void)
{
	LPC_SYSCON->SYSAHBCLKCTRL |= (SYSAHBCLKCTRL_CT32B0 | SYSAHBCLKCTRL_CT32B1);
}

static void mcu_setup_dali_pins(void)
{
	LPC_GPIO_PORT->DIR[DALI_TX_PORT] |= DALI_TX_BIT;
	LPC_IOCON->PIO0_12 =
		(IOCON_DAPIN_FUNC_MASK & IOCON_PIN0_12_FUNC_PIO) << IOCON_DAPIN_FUNC_SHIFT |
		(IOCON_DAPIN_MODE_MASK & IOCON_DAPIN_MODE_PULLUP) << IOCON_DAPIN_MODE_SHIFT |
		(IOCON_DAPIN_SMODE_MASK & 3U) << IOCON_DAPIN_SMODE_SHIFT |
		(IOCON_DAPIN_CLKDIV_MASK & 6U) << IOCON_DAPIN_CLKDIV_SHIFT;
}

static bool mcu_rx_pin(void)
{
	return !(LPC_GPIO_PORT->PIN[DALI_RX_PORT] & DALI_RX_BIT);
}

static void counter_tx_set(bool state)
{
	LPC_IOCON->PIO0_11 =
		(IOCON_DAPIN_FUNC_MASK & IOCON_PIN0_11_FUNC_PIO) << IOCON_DAPIN_FUNC_SHIFT |
		(IOCON_DAPIN_MODE_MASK & IOCON_DAPIN_MODE_PULLDOWN) << IOCON_DAPIN_MODE_SHIFT |
		(IOCON_DAPIN_ADMODE);
	if (state) {
		LPC_GPIO_PORT->SET[DALI_TX_PORT] |= DALI_TX_BIT;
	} else {
		LPC_GPIO_PORT->CLR[DALI_TX_PORT] |= DALI_TX_BIT;
	}
}

static void counter_tx_timer_stop(void)
{
	LPC_CT32B0->MCR = (LPC_CT32B0->MCR &
			   ~(CT32_MCR_MR0I | CT32_MCR_MR3I | CT32_MCR_MR3R | CT32_MCR_MR3S));
	LPC_CT32B0->EMR = (CT32_EMR_EM2 | (CT32_EMR_EMCTR_NOTHING << CT32_EMR_EMC2_SHIFT));
}

static void counter_tx_timer_next(uint32_t count, enum board_toggle toggle)
{
	LPC_CT32B0->MR3 = count;
	if (toggle == DISABLE_TOGGLE) {
		LPC_CT32B0->EMR &= ~(CT32_EMR_EMCTR_TOGGLE << CT32_EMR_EMC3_SHIFT);
	}
}

static uint32_t counter_tx_get_count(void)
{
	return LPC_CT32B0->TC;
}

static void counter_collision_set(uint32_t count)
{
	LPC_CT32B0->MR0 = count;
}

static void counter_tx_setup(uint32_t count, bool check_collision)
{
	LPC_CT32B0->TCR = CT32_TCR_CRST;
	counter_tx_timer_stop();
	counter_tx_timer_next(count, NOTHING);
	/* set prescaler to base rate */
	LPC_CT32B0->PR = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / DALI_TIMER_RATE_HZ) - 1;
	/* set timer mode */
	LPC_CT32B0->CTCR = 0;
	/* on MR3 match: IRQ */
	LPC_CT32B0->MCR = (LPC_CT32B0->MCR & ~(CT32_MCR_MR3I | CT32_MCR_MR3R | CT32_MCR_MR3S)) |
			  (CT32_MCR_MR3I);
	/* for collision checking enable MR0 */
	if (check_collision) {
		LPC_CT32B0->MCR |= CT32_MCR_MR0I;
	} else {
		LPC_CT32B0->MCR &= ~CT32_MCR_MR0I;
	}
	/* on MR3 match: toggle output - start with DALI active */
	LPC_CT32B0->EMR = CT32_EMR_EM3 | (CT32_EMR_EMCTR_TOGGLE << CT32_EMR_EMC3_SHIFT);
	/* outputs are controlled by EMx */
	LPC_CT32B0->PWMC = 0;
	/* pin function: CT32B0_MAT3 */
	/* function mode: no pull up/down resistors */
	/* hysteresis disabled */
	/* standard gpio */
	LPC_IOCON->PIO0_11 =
		(IOCON_DAPIN_FUNC_MASK & IOCON_PIN0_11_FUNC_MAT3) << IOCON_DAPIN_FUNC_SHIFT |
		(IOCON_DAPIN_MODE_MASK & IOCON_DAPIN_MODE_PULLDOWN) << IOCON_DAPIN_MODE_SHIFT |
		(IOCON_DAPIN_ADMODE);
	/* start timer */
	LPC_CT32B0->TCR = CT32_TCR_CEN;
}

static uint32_t counter_rx_get_count(void)
{
	return LPC_CT32B1->TC;
}

static uint32_t counter_rx_get_capture(void)
{
	return LPC_CT32B1->CR0;
}

static void counter_rx_set_event_counter(enum rx_counter_event event, uint32_t match_count)
{
	switch (event) {
	case STOPBIT:
		LPC_CT32B1->MR0 = match_count;
		break;
	case PRIORITY:
		LPC_CT32B1->MR1 = match_count;
		break;
	case QUERY:
		LPC_CT32B1->MR2 = match_count;
		break;
	default:
		__ASSERT(false, "unexpected event code");
	}
}

static void counter_rx_event_enable(enum rx_counter_event event, bool enable)
{
	switch (event) {
	case STOPBIT:
		if (enable) {
			LPC_CT32B1->MCR |= (CT32_MCR_MR0I);
		} else {
			LPC_CT32B1->MCR &= ~(CT32_MCR_MR0I);
		}
		break;
	case PRIORITY:
		if (enable) {
			LPC_CT32B1->MCR |= (CT32_MCR_MR1I);
		} else {
			LPC_CT32B1->MCR &= ~(CT32_MCR_MR1I);
		}
		break;
	case QUERY:
		if (enable) {
			LPC_CT32B1->MCR |= (CT32_MCR_MR2I);
		} else {
			LPC_CT32B1->MCR &= ~(CT32_MCR_MR2I);
		}
		break;
	default:
		__ASSERT(false, "unexpected");
	}
}

void counter_rx_start(void)
{
	LPC_CT32B1->TCR = CT32_TCR_CRST;
	/* set prescaler to base rate */
	LPC_CT32B1->PR = (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / DALI_TIMER_RATE_HZ) - 1;
	/* set timer mode */
	LPC_CT32B1->CTCR = 0;
	/* capture both edges, trigger IRQ */
	LPC_CT32B1->CCR = (CT32_CCR_CAP0FE | CT32_CCR_CAP0RE | CT32_CCR_CAP0I);
	/* disable event matches */
	counter_rx_event_enable(STOPBIT, false);
	counter_rx_event_enable(PRIORITY, false);
	counter_rx_event_enable(QUERY, false);
	/* pin function: CT32B1_CAP0 */
	/* function mode: enable pull up resistor */
	/* hysteresis disabled */
	/* digital function mode, standard gpio */
	LPC_IOCON->PIO0_12 =
		(IOCON_DAPIN_FUNC_MASK & IOCON_PIN0_12_FUNC_CAP0) << IOCON_DAPIN_FUNC_SHIFT |
		(IOCON_DAPIN_MODE_MASK & IOCON_DAPIN_MODE_PULLDOWN) << IOCON_DAPIN_MODE_SHIFT |
		(IOCON_DAPIN_ADMODE);
	/* start timer */
	LPC_CT32B1->TCR = CT32_TCR_CEN;
}

static void tx_slot_reset(struct dali_tx_slot *slot)
{
	__ASSERT(slot, "invalid tx slot");

	*slot = (struct dali_tx_slot){
		.state_now = true,
	};
}

static bool is_tx_slot_empty(const struct dali_tx_slot *slot)
{
	return slot->index_max == 0;
}

static void add_signal_phase(struct dali_tx_slot *slot, uint32_t duration_us,
			     bool change_last_phase)
{
	__ASSERT((slot->index_max < COUNT_ARRAY_SIZE), "PWM pattern does not fit into buffer");

	if (change_last_phase) {
		slot->index_max--;
	}
	uint32_t count_now =
		(slot->index_max) ? (slot->count[slot->index_max - 1] + duration_us) : duration_us;
	slot->count[slot->index_max++] = count_now;
}

static void add_bit(const struct device *dev, struct dali_tx_slot *slot, bool value)
{
	const struct dali_lpc11u6x_config *config = dev->config;

	uint32_t phase_one;
	uint32_t phase_two;
	bool change_previous = false;

	if (slot->state_now == value) {
		if (slot->state_now) {
			phase_one = TX_HALF_BIT_US - config->tx_rise_fall_delta_us;
			phase_two = TX_HALF_BIT_US + config->tx_rise_fall_delta_us;
		} else {
			phase_one = TX_HALF_BIT_US + config->tx_rise_fall_delta_us;
			phase_two = TX_HALF_BIT_US - config->tx_rise_fall_delta_us;
		}
	} else {
		change_previous = true;
		if (slot->state_now) {
			phase_one = TX_FULL_BIT_US + config->tx_rise_fall_delta_us;
			phase_two = TX_HALF_BIT_US - config->tx_rise_fall_delta_us;
		} else {
			phase_one = TX_FULL_BIT_US - config->tx_rise_fall_delta_us;
			phase_two = TX_HALF_BIT_US + config->tx_rise_fall_delta_us;
		}
	}
	add_signal_phase(slot, phase_one, change_previous);
	add_signal_phase(slot, phase_two, false);
	slot->state_now = value;
}

static void add_stop_condition(struct dali_tx_slot *slot)
{
	if (slot->state_now) {
		add_signal_phase(slot, TX_STOP_BIT_US, true);
		slot->index_max--;
	} else {
		add_signal_phase(slot, TX_STOP_BIT_US, false);
		slot->index_max--;
	}
}

static void calculate_counts(const struct device *dev, struct dali_tx_slot *slot,
			     const struct dali_frame frame)
{
	uint_fast8_t length = 0;

	switch (frame.event_type) {
	case DALI_FRAME_CORRUPT:
		for (int_fast8_t i = 0; i < (2 * FRAME_BACKWARD_LENGTH); i++) {
			if (i == EXTEND_CORRUPT_PHASE) {
				add_signal_phase(slot, TX_CORRUPT_BIT_US, false);
			} else {
				add_signal_phase(slot, TX_HALF_BIT_US, false);
			}
		}
		return;
	case DALI_FRAME_BACKWARD:
		length = FRAME_BACKWARD_LENGTH;
		break;
	case DALI_FRAME_GEAR:
		length = FRAME_GEAR_LENGTH;
		break;
	case DALI_FRAME_DEVICE:
		length = FRAME_DEVICE_LENGTH;
		break;
	default:
		__ASSERT(false, "illegal event type");
	}

	if (length) { /* add start bit */
		add_bit(dev, slot, true);

		/* add data bits */
		for (int_fast8_t i = (length - 1); i >= 0; i--) {
			add_bit(dev, slot, frame.data & (1 << i));
		}

		add_stop_condition(slot);
	}
}

static void rx_schedule_query(void)
{
	const uint32_t counter_now = counter_rx_get_count();
	const uint32_t query_count = counter_now + RX_FORWARD_BACK_MAX_US + GREY_AREA_INTERFRAME_US;
	counter_rx_set_event_counter(QUERY, query_count);
	counter_rx_event_enable(QUERY, true);
}

static bool is_forward_transmission(struct dali_lpc11u6x_data *data)
{
	return (data->active == &data->forward && data->active->index_next);
}

static void stop_transmission(struct dali_lpc11u6x_data *data)
{
	if (is_forward_transmission(data)) {
		data->rx_status = STOP_TRANSMISSION;
		counter_tx_timer_stop();
		counter_tx_set(DALI_TX_IDLE);
	}
}

static void destroy_frame(struct dali_lpc11u6x_data *data)
{
	if (is_forward_transmission(data)) {
		if (data->rx_status == DESTROY_FRAME) {
			return;
		}
		if (data->rx_status != STOP_TRANSMISSION) {
			counter_tx_timer_stop();
			counter_tx_set(DALI_TX_ACTIVE);
		}
		data->rx_status = DESTROY_FRAME;

		/* use stopbit counter to time frame destruction */
		const uint32_t break_count = data->edge_count + TX_BREAK_US;
		counter_rx_set_event_counter(STOPBIT, break_count);
		counter_rx_event_enable(STOPBIT, true);
	}
}

void tx_irq_callback(const struct device *dev)
{
	struct dali_lpc11u6x_data *data = dev->data;
	const struct dali_lpc11u6x_config *config = data->config;
	struct dali_tx_slot *active = data->active;

	/* schedule check for current transition */
	if (is_forward_transmission(data)) {
		const uint32_t last_transition = active->count[active->index_next - 1];
		counter_collision_set(last_transition + config->tx_rx_propagation_min_us);
		active->state_now = !active->state_now;
	}

	/* schedule next level transition */
	if (active->index_next < active->index_max) {
		const uint32_t next_transition = active->count[active->index_next++];
		counter_tx_timer_next(next_transition, NOTHING);
		return;
	}

	/* schedule the last transition */
	if (active->index_next == active->index_max) {
		const uint32_t next_transition = active->count[active->index_next++];
		counter_tx_timer_next(next_transition, DISABLE_TOGGLE);
		if (data->rx_status == TRANSMIT_BACKFRAME) {
			data->rx_status = STOPBIT_BACKFRAME;
		}
		return;
	}

	/* activities at end of frame */
	counter_tx_set(DALI_TX_IDLE);
	counter_tx_timer_stop();
	if (active->is_query) {
		rx_schedule_query();
	}
	tx_slot_reset(active);
	data->active = 0;
}

void tx_collision_callback(const struct device *dev)
{
	struct dali_lpc11u6x_data *data = dev->data;
	struct dali_tx_slot *active = data->active;

	if (mcu_rx_pin() == active->state_now) {
		stop_transmission(data);
		LOG_ERR("unexpected bus state while sending period %d -- stop transmission",
			active->index_next);
	}
}

static void tx_IRQHandler(const struct device *dev)
{
	if (LPC_CT32B0->IR & CT32_IR_MR3INT) {
		LPC_CT32B0->IR = CT32_IR_MR3INT;
		tx_irq_callback(dev);
	}
	if (LPC_CT32B0->IR & CT32_IR_MR0INT) {
		LPC_CT32B0->IR = CT32_IR_MR0INT;
		tx_collision_callback(dev);
	}
}

static void tx_start_send(struct dali_lpc11u6x_data *data)
{
	if (data->active) {
		data->active->index_next = 1;
		data->active->state_now = true;
		counter_tx_setup(data->active->count[0], is_forward_transmission(data));
	}
}

static void tx_schedule_send(struct dali_lpc11u6x_data *data)
{
	/* select the frame to send - backward frame is dominant */
	if (!is_tx_slot_empty(&data->forward)) {
		data->active = &data->forward;
	}
	if (!is_tx_slot_empty(&data->backward)) {
		data->active = &data->backward;
	}

	uint32_t start_send_rx_count = data->active->inter_frame_idle;
	if (data->rx_status == TRANSMIT_BACKFRAME) {
		start_send_rx_count += data->last_full_frame_count;
	} else {
		start_send_rx_count += data->last_edge_count;
	}
	if (counter_rx_get_count() > start_send_rx_count) {
		counter_rx_event_enable(PRIORITY, false);
		tx_start_send(data);
	} else {
		counter_rx_set_event_counter(PRIORITY, start_send_rx_count);
		counter_rx_event_enable(PRIORITY, true);
	}
}

static bool rx_is_received_twice(struct dali_lpc11u6x_data *data)
{
	const uint32_t frame_duration_us = (data->rx_frame_length + 1) * TX_FULL_BIT_US;
	const uint32_t time_difference_us =
		data->rx_timestamp - data->rx_last_timestamp - frame_duration_us;
	const bool is_data_identical = (data->rx_data == data->rx_last_payload &&
					data->rx_frame_length == data->rx_last_frame_length);

	data->rx_last_timestamp = data->rx_timestamp;
	data->rx_last_payload = data->rx_data;
	data->rx_last_frame_length = data->rx_frame_length;

	if (time_difference_us > RX_TWICE_MAX_US + GREY_AREA_INTERFRAME_US) {
		return false;
	}
	return is_data_identical;
}

static void rx_reset_received_twice(struct dali_lpc11u6x_data *data)
{
	data->rx_last_payload = 0;
	data->rx_last_frame_length = 0;
}

static void rx_finish_frame(struct dali_lpc11u6x_data *data)
{
	struct dali_frame frame = {
		.data = data->rx_data,
		.event_type = DALI_EVENT_NONE,
	};
	switch (data->rx_status) {
	case START_BIT_START:
	case START_BIT_INSIDE:
	case DATA_BIT_START:
	case DATA_BIT_INSIDE:
		LOG_INF("{%08x:%02x %08x}", data->rx_timestamp, data->rx_frame_length,
			data->rx_data);
		switch (data->rx_frame_length) {
		case FRAME_BACKWARD_LENGTH:
			frame.event_type = DALI_FRAME_BACKWARD;
			break;
		case FRAME_GEAR_LENGTH:
			data->last_full_frame_count = data->last_edge_count;
			frame.event_type = rx_is_received_twice(data) ? DALI_FRAME_GEAR_TWICE
								      : DALI_FRAME_GEAR;
			break;
		case FRAME_DEVICE_LENGTH:
			data->last_full_frame_count = data->last_edge_count;
			frame.event_type = rx_is_received_twice(data) ? DALI_FRAME_DEVICE_TWICE
								      : DALI_FRAME_DEVICE;
			break;
		case FRAME_UPDATE_LENGTH:
			rx_is_received_twice(data);
			data->last_full_frame_count = data->last_edge_count;
			frame.event_type = DALI_FRAME_FIRMWARE;
			break;
		default:
			LOG_INF("invalid frame length %d bits", data->rx_frame_length);
			rx_reset_received_twice(data);
			frame.data = 0, frame.event_type = DALI_FRAME_CORRUPT;
		}
		data->rx_status = IDLE;
		break;
	case STOP_TRANSMISSION:
		frame.data = 0;
		frame.event_type = DALI_FRAME_CORRUPT;
		rx_reset_received_twice(data);
		data->rx_status = IDLE;
		/* re-schedule the current frame data */
		data->active->index_next = 0;
		data->active->state_now = true;
		data->active->inter_frame_idle = TX_RECOVER_MIN_US;
		tx_schedule_send(data);
		break;
	case BUS_FAILURE_DETECT:
		LOG_INF("bus failure");
		frame.data = 0;
		frame.event_type = DALI_EVENT_BUS_FAILURE;
		rx_reset_received_twice(data);
		break;
	case IDLE:
		LOG_INF("bus idle");
		frame.data = 0;
		frame.event_type = DALI_EVENT_BUS_IDLE;
		rx_reset_received_twice(data);
		break;
	case ERROR_IN_FRAME:
	default:
		LOG_INF("corrupt frame");
		frame.data = 0;
		frame.event_type = DALI_FRAME_CORRUPT;
		rx_reset_received_twice(data);
		data->rx_status = IDLE;
	}
	k_msgq_put(&data->rx_queue, &frame, K_NO_WAIT);
}

static bool is_valid_halfbit_timing(const uint32_t time_difference_us)
{
	if ((time_difference_us > RX_BIT_TIME_HALF_MAX_US + GREY_AREA_BITTIMING_US) ||
	    (time_difference_us < RX_BIT_TIME_HALF_MIN_US - GREY_AREA_BITTIMING_US)) {
		return false;
	}
	return true;
}

static bool is_valid_fullbit_timing(const uint32_t time_difference_us)
{
	if ((time_difference_us > RX_BIT_TIME_FULL_MAX_US + GREY_AREA_BITTIMING_US) ||
	    (time_difference_us < RX_BIT_TIME_FULL_MIN_US - GREY_AREA_BITTIMING_US)) {
		return false;
	}
	return true;
}

static bool is_destroy_start(const uint32_t time_difference_us)
{
	if ((time_difference_us > TX_DESTROY_1_MIN_US - GREY_AREA_BITTIMING_US) &&
	    (time_difference_us < TX_DESTROY_1_MAX_US + GREY_AREA_BITTIMING_US)) {
		return true;
	}
	if ((time_difference_us > TX_DESTROY_2_MIN_US + GREY_AREA_BITTIMING_US)) {
		return true;
	}
	return false;
}

static bool is_destroy_inside(const uint32_t time_difference_us)
{
	if ((time_difference_us > TX_DESTROY_1_MIN_US - GREY_AREA_BITTIMING_US) &&
	    (time_difference_us < TX_DESTROY_1_MAX_US + GREY_AREA_BITTIMING_US)) {
		return true;
	}
	if ((time_difference_us > TX_DESTROY_2_MIN_US - GREY_AREA_BITTIMING_US) &&
	    (time_difference_us < TX_DESTROY_2_MAX_US + GREY_AREA_BITTIMING_US)) {
		return true;
	}
	if ((time_difference_us > TX_DESTROY_3_MIN_US - GREY_AREA_BITTIMING_US)) {
		return true;
	}
	return false;
}

static uint32_t get_corrected_time_difference_us(const struct dali_lpc11u6x_data *data, bool invert)
{
	const struct dali_lpc11u6x_config *config = data->config;

	const uint32_t time_difference_us = data->edge_count - data->last_edge_count;
	if (data->last_data_bit ^ invert) {
		return time_difference_us + config->rx_rise_fall_delta_us;
	}
	return time_difference_us - config->rx_rise_fall_delta_us;
}

static void set_new_status(struct dali_lpc11u6x_data *data, enum rx_state new_status)
{
	switch (data->rx_status) {
	case ERROR_IN_FRAME:
	case STOP_TRANSMISSION:
	case DESTROY_FRAME:
		return;
	default:
		data->rx_status = new_status;
	}
}

static void add_bit_to_received_data(struct dali_lpc11u6x_data *data)
{
	data->rx_data = (data->rx_data << 1U) | data->last_data_bit;
	data->rx_frame_length++;
	if (data->rx_frame_length > MAX_BIT_PER_FRAME) {
		set_new_status(data, ERROR_IN_FRAME);
	}
}

static void check_start_timing(struct dali_lpc11u6x_data *data)
{
	const uint32_t time_difference_us = get_corrected_time_difference_us(data, false);
	LOG_DBG("start timing: %d us", time_difference_us);
	if (is_forward_transmission(data) && is_destroy_start(time_difference_us)) {
		destroy_frame(data);
		if (data->rx_status == START_BIT_START) {
			LOG_ERR("start bit collision, timing %d us, destroy frame",
				time_difference_us);
		} else {
			LOG_ERR("data bit %d collision, timing %d us, destroy frame",
				data->rx_frame_length, time_difference_us);
		}
		return;
	}
	if (!is_valid_halfbit_timing(time_difference_us)) {
		set_new_status(data, ERROR_IN_FRAME);
		if (data->rx_status == START_BIT_START) {
			LOG_ERR("start bit timing %d us, corrupt frame", time_difference_us);
		} else {
			LOG_ERR("data bit %d timing %d us, corrupt frame", data->rx_frame_length,
				time_difference_us);
		}
		return;
	}
	if (data->rx_status == DATA_BIT_START) {
		add_bit_to_received_data(data);
	}
}

static enum rx_state check_inside_timing(struct dali_lpc11u6x_data *data)
{
	uint32_t time_difference_us = get_corrected_time_difference_us(data, true);
	LOG_DBG("inside timing: %d us", time_difference_us);
	if (is_forward_transmission(data) && is_destroy_inside(time_difference_us)) {
		destroy_frame(data);
		if (data->rx_status == START_BIT_INSIDE) {
			LOG_ERR("inside start bit collision, timing %d us, destroy frame",
				time_difference_us);
		} else {
			LOG_ERR("inside bit %d collision, timing %d us, destroy frame",
				data->rx_frame_length, time_difference_us);
		}
		return DESTROY_FRAME;
	}
	if (is_valid_halfbit_timing(time_difference_us)) {
		return DATA_BIT_START;
	}
	if (is_valid_fullbit_timing(time_difference_us)) {
		data->last_data_bit = !data->last_data_bit;
		add_bit_to_received_data(data);
		return DATA_BIT_INSIDE;
	}
	if (data->rx_status == START_BIT_INSIDE) {
		LOG_ERR("inside start bit timing error %d us", time_difference_us);
	} else {
		LOG_ERR("inside data bit %d timing error %d us", data->rx_frame_length,
			time_difference_us);
	}
	return ERROR_IN_FRAME;
}

static void check_for_collision(struct dali_lpc11u6x_data *data)
{
	struct dali_tx_slot *active = data->active;

	if (!is_forward_transmission(data)) {
		return;
	}
	const struct dali_lpc11u6x_config *config = data->config;
	const uint32_t expected_count = active->count[active->index_next - 2];
	const int32_t delay = data->tx_count_on_capture - expected_count;
	if (delay < 0 || delay > config->tx_rx_propagation_max_us) {
		stop_transmission(data);
		LOG_ERR("unexpected capture with delay of %d us while receiving bit %d, stop "
			"transmission",
			delay, data->rx_frame_length);
	}
}

static void rx_process_capture_event(struct dali_lpc11u6x_data *data)
{
	if (data->rx_status == STOP_TRANSMISSION) {
		data->last_edge_count = counter_rx_get_count();
		return;
	}

	if (data->rx_status == DESTROY_FRAME) {
		data->last_edge_count = counter_rx_get_count();
		if (mcu_rx_pin()) {
			data->rx_status = IDLE;

			/* re-schedule the current frame data */
			data->active->index_next = 0;
			data->active->state_now = true;
			data->active->inter_frame_idle = TX_RECOVER_MIN_US;
			tx_schedule_send(data);
		}
		return;
	}

	data->edge_count = counter_rx_get_capture();
	const struct dali_lpc11u6x_config *config = data->config;
	const uint32_t stop_timeout_count =
		data->edge_count + RX_BIT_TIME_STOP_US - 2 * config->rx_rise_fall_delta_us;
	counter_rx_set_event_counter(STOPBIT, stop_timeout_count);
	counter_rx_event_enable(STOPBIT, true);

	if (data->rx_status == TRANSMIT_BACKFRAME || data->rx_status == STOPBIT_BACKFRAME) {
		data->last_edge_count = data->edge_count;
		return;
	}

	switch (data->rx_status) {
	case IDLE:
		if (!mcu_rx_pin()) {
			set_new_status(data, START_BIT_START);
			data->last_data_bit = true;
			data->rx_timestamp = counter_rx_get_count();
			data->rx_data = 0;
			data->rx_frame_length = 0;
			counter_rx_event_enable(QUERY, false);
			counter_rx_event_enable(PRIORITY, false);
		}
		break;
	case START_BIT_START:
		check_for_collision(data);
		check_start_timing(data);
		set_new_status(data, START_BIT_INSIDE);
		break;
	case START_BIT_INSIDE:
		check_for_collision(data);
		set_new_status(data, check_inside_timing(data));
		break;
	case DATA_BIT_START:
		check_for_collision(data);
		check_start_timing(data);
		set_new_status(data, DATA_BIT_INSIDE);
		break;
	case DATA_BIT_INSIDE:
		check_for_collision(data);
		set_new_status(data, check_inside_timing(data));
		break;
	case STOPBIT_BACKFRAME:
	case ERROR_IN_FRAME:
		break;
	case BUS_LOW:
		if (mcu_rx_pin()) {
			set_new_status(data, ERROR_IN_FRAME);
			rx_finish_frame(data);
		}
		break;
	case BUS_FAILURE_DETECT:
		if (mcu_rx_pin()) {
			data->rx_status = IDLE;
			rx_finish_frame(data);
		}
		break;
	default:
		__ASSERT(false, "invalid state");
	}
	data->last_edge_count = data->edge_count;
	/* if transmission pending: re-schedule the next transmission */
	if (data->active && data->active->index_next == 0) {
		tx_schedule_send(data);
	}
}

static void trigger_resend(struct dali_lpc11u6x_data *data)
{
	/* set the bus to idle */
	counter_tx_set(DALI_TX_IDLE);
	counter_rx_event_enable(STOPBIT, false);

	/* push information into receive queue */
	struct dali_frame frame = {
		.event_type = DALI_FRAME_CORRUPT,
	};
	k_msgq_put(&data->rx_queue, &frame, K_NO_WAIT);
	rx_reset_received_twice(data);
}

static void rx_process_stopbit_event(struct dali_lpc11u6x_data *data)
{
	/* this can happen with extensive long bus active periods */
	if (data->rx_status == TRANSMIT_BACKFRAME) {
		/* re-start counter */
		const uint32_t stop_timeout_count = data->edge_count + RX_BIT_TIME_STOP_US -
						    2 * data->config->rx_rise_fall_delta_us;
		counter_rx_set_event_counter(STOPBIT, stop_timeout_count);
		counter_rx_event_enable(STOPBIT, true);
		return;
	}

	if (mcu_rx_pin()) {
		switch (data->rx_status) {
		case IDLE:
		case START_BIT_START:
		case START_BIT_INSIDE:
		case DATA_BIT_START:
		case DATA_BIT_INSIDE:
		case BUS_LOW:
		case BUS_FAILURE_DETECT:
		case ERROR_IN_FRAME:
		case STOP_TRANSMISSION:
			rx_finish_frame(data);
			return;
		case STOPBIT_BACKFRAME:
			data->rx_status = IDLE;
			return;
		default:
			__ASSERT(false, "invalid state");
			return;
		}
	}
	switch (data->rx_status) {
	case DESTROY_FRAME:
		trigger_resend(data);
		return;
	case BUS_LOW:
		data->rx_status = BUS_FAILURE_DETECT;
		rx_finish_frame(data);
		return;
	case BUS_FAILURE_DETECT:
		return;
	default:
		counter_rx_set_event_counter(STOPBIT, data->edge_count + FAILURE_CONDITION_US);
		counter_rx_event_enable(STOPBIT, true);
		data->rx_status = BUS_LOW;
	}
}

static void rx_process_priority_event(struct dali_lpc11u6x_data *data)
{
	tx_start_send(data);
}

static void rx_process_query_event(struct dali_lpc11u6x_data *data)
{
	struct dali_frame timeout_event = {
		.data = 0,
		.event_type = DALI_EVENT_NO_ANSWER,
	};
	k_msgq_put(&data->rx_queue, &timeout_event, K_NO_WAIT);
}

static void rx_event_work_handler(struct k_work *item)
{
	struct dali_lpc11u6x_data *data = CONTAINER_OF(item, struct dali_lpc11u6x_data, rx_work);

	switch (data->rx_event) {
	case CAPTURE:
		rx_process_capture_event(data);
		break;
	case STOPBIT:
		rx_process_stopbit_event(data);
		break;
	case PRIORITY:
		rx_process_priority_event(data);
		break;
	case QUERY:
		rx_process_query_event(data);
		break;
	default:
		__ASSERT(false, "invalid event type");
	}
}

static void dali_rx_IRQHandler(const struct device *dev)
{

	if (LPC_CT32B1->IR & CT32_IR_MR0INT) {
		LPC_CT32B1->IR = CT32_IR_MR0INT;
		counter_rx_event_enable(STOPBIT, false);
		struct dali_lpc11u6x_data *data = dev->data;
		data->rx_event = STOPBIT;
		k_work_submit_to_queue(&dali_work_queue, &data->rx_work);
	}
	if (LPC_CT32B1->IR & CT32_IR_MR1INT) {
		LPC_CT32B1->IR = CT32_IR_MR1INT;
		counter_rx_event_enable(PRIORITY, false);
		struct dali_lpc11u6x_data *data = dev->data;
		data->rx_event = PRIORITY;
		k_work_submit_to_queue(&dali_work_queue, &data->rx_work);
	}
	if (LPC_CT32B1->IR & CT32_IR_MR2INT) {
		LPC_CT32B1->IR = CT32_IR_MR2INT;
		counter_rx_event_enable(QUERY, false);
		struct dali_lpc11u6x_data *data = dev->data;
		data->rx_event = QUERY;
		k_work_submit_to_queue(&dali_work_queue, &data->rx_work);
	}
	if (LPC_CT32B1->IR & CT32_IR_CR0INT) {
		LPC_CT32B1->IR = CT32_IR_CR0INT;
		struct dali_lpc11u6x_data *data = dev->data;
		data->rx_event = CAPTURE;
		data->tx_count_on_capture = counter_tx_get_count();
		k_work_submit_to_queue(&dali_work_queue, &data->rx_work);
	}
}

static void tx_slot_reset_all(struct dali_lpc11u6x_data *data)
{
	tx_slot_reset(&data->forward);
	tx_slot_reset(&data->backward);
	data->active = 0;
}

static int init(const struct device *dev)
{
	struct dali_lpc11u6x_data *data = dev->data;

	LOG_DBG("Initialize DALI driver");

	/* connect to config */
	data->config = dev->config;

	/* initialize transmission slots */
	tx_slot_reset_all(data);

	/* set up receive work queue */
	const struct k_work_queue_config cfg = {
		.name = "DALI work",
		.no_yield = true,
		.essential = false,
	};
	k_work_queue_start(&dali_work_queue, dali_work_queue_stack,
			   K_KERNEL_STACK_SIZEOF(dali_work_queue_stack),
			   CONFIG_DALI_LPC11U6X_PRIORITY, &cfg);
	k_work_init(&data->rx_work, rx_event_work_handler);

	/* initialize receive queue */
	k_msgq_init(&data->rx_queue, data->rx_buffer, sizeof(struct dali_frame),
		    CONFIG_MAX_FRAMES_IN_QUEUE);

	/* initialize peripherals */
	mcu_setup_peripheral_clock();
	mcu_setup_dali_pins();
	counter_tx_set(DALI_TX_IDLE);
	counter_rx_start();
	if (mcu_rx_pin()) {
		data->rx_status = IDLE;
	} else {
		data->rx_status = BUS_LOW;
		counter_rx_set_event_counter(STOPBIT, FAILURE_CONDITION_US);
		counter_rx_event_enable(STOPBIT, true);
	}
	return 0;
}

static int dali_lpc11u6x_receive(const struct device *dev, struct dali_frame *frame,
				 k_timeout_t timeout)
{
	__ASSERT(frame, "invalid frame pointer");
	__ASSERT(dev, "invalid device");

	struct dali_lpc11u6x_data *data = dev->data;
	__ASSERT(&(data->rx_queue), "invalid queue");

	if (k_msgq_get(&(data->rx_queue), frame, timeout) < 0) {
		return -ENOMSG;
	}
	return 0;
};

static int dali_lpc11u6x_send(const struct device *dev, const struct dali_tx_frame *tx_frame)
{
	LOG_DBG("");
	__ASSERT(dev, "invalid device");

	const enum dali_event_type frame_type = tx_frame->frame.event_type;
	if (frame_type == DALI_EVENT_NONE) {
		return 0;
	}

	struct dali_lpc11u6x_data *data = dev->data;
	if (data->active) {
		if (data->active->index_next) {
			LOG_ERR("send is busy sending");
			return -EBUSY;
		}
	}

	switch (frame_type) {
	case DALI_FRAME_BACKWARD:
	case DALI_FRAME_CORRUPT:
		if (tx_frame->is_query) {
			return -EINVAL;
		}
		if (is_tx_slot_empty(&data->backward)) {
			data->rx_status = TRANSMIT_BACKFRAME;
			counter_rx_event_enable(STOPBIT, false);
			calculate_counts(dev, &data->backward, tx_frame->frame);
			data->backward.inter_frame_idle = settling_time_us[0];
		} else {
			LOG_ERR("backward frame slot is busy");
			return -EBUSY;
		}
		break;
	case DALI_FRAME_DEVICE:
	case DALI_FRAME_GEAR:
	case DALI_FRAME_FIRMWARE:
		if (is_tx_slot_empty(&data->forward)) {
			calculate_counts(dev, &data->forward, tx_frame->frame);
			if (tx_frame->priority < DALI_PRIORITY_1 ||
			    tx_frame->priority > DALI_PRIORITY_5) {
				return -EINVAL;
			}
			data->forward.inter_frame_idle = settling_time_us[tx_frame->priority];
			data->forward.is_query = tx_frame->is_query;
		} else {
			LOG_ERR("forward frame slot is busy");
			return -EBUSY;
		}
		break;
	default:
		return -EINVAL;
	}
	tx_schedule_send(data);

	return 0;
}

static void dali_lpc11u6x_abort(const struct device *dev)
{
	__ASSERT(dev, "invalid device");

	counter_tx_timer_stop();
	counter_tx_set(DALI_TX_ACTIVE);

	struct dali_lpc11u6x_data *data = dev->data;
	tx_slot_reset_all(data);
}

static const struct dali_driver_api dali_lpc11u6x_driver_api = {
	.recv = dali_lpc11u6x_receive,
	.send = dali_lpc11u6x_send,
	.abort = dali_lpc11u6x_abort,
};

/* forward declaration of init */
static int init_dali_lpc11u6x(const struct device *dev);

/* clang-format off */
#define DALI_LPC11U6X_INST(id)                                                          \
	static struct dali_lpc11u6x_data dali_lpc11u6x_data_##id = {0};                     \
	static struct dali_lpc11u6x_config dali_lpc11u6x_config_##id = {                    \
		.tx_rise_fall_delta_us = DT_INST_PROP_OR(id, tx_rise_fall_delta_us, 0),         \
		.rx_rise_fall_delta_us = DT_INST_PROP_OR(id, rx_rise_fall_delta_us, 0),         \
		.tx_rx_propagation_min_us = DT_INST_PROP_OR(id, tx_rx_propagation_min_us, 1),   \
		.tx_rx_propagation_max_us = DT_INST_PROP_OR(id, tx_rx_propagation_max_us, 100), \
	};                                                                                  \
	DEVICE_DT_INST_DEFINE(id, init_dali_lpc11u6x, NULL, &dali_lpc11u6x_data_##id,       \
			      &dali_lpc11u6x_config_##id, POST_KERNEL,                              \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &dali_lpc11u6x_driver_api);       \
	static int init_dali_lpc11u6x(const struct device *dev)                             \
	{                                                                                   \
		IRQ_CONNECT(DT_IRQ_BY_IDX(DT_DRV_INST(id), 0, irq),                             \
			    DT_IRQ_BY_IDX(DT_DRV_INST(id), 0, priority), tx_IRQHandler,             \
			    DEVICE_DT_INST_GET(id), 0);                                             \
		irq_enable(DT_IRQ_BY_IDX(DT_DRV_INST(id), 0, irq));                             \
		IRQ_CONNECT(DT_IRQ_BY_IDX(DT_DRV_INST(id), 1, irq),                             \
			    DT_IRQ_BY_IDX(DT_DRV_INST(id), 1, priority), dali_rx_IRQHandler,        \
			    DEVICE_DT_INST_GET(id), 0);                                             \
		irq_enable(DT_IRQ_BY_IDX(DT_DRV_INST(id), 1, irq));                             \
		return init(dev);                                                               \
	};
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(DALI_LPC11U6X_INST);
