/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_twaifd

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <soc.h>
#include <esp_clk_tree.h>
#include <hal/twai_hal.h>
#include <hal/twaifd_ll.h>
#include <hal/twai_types.h>

LOG_MODULE_REGISTER(can_esp32_twaifd, CONFIG_CAN_LOG_LEVEL);

#define CAN_ESP32_TWAIFD_MAX_FILTER CONFIG_CAN_ESP32_TWAIFD_MAX_FILTERS

/*
 * Number of hardware TXT buffers used as a transmit pool. The controller has
 * up to 4 TXT buffers on ESP32-C5. All buffers are left at equal priority so
 * the controller arbitrates pending frames on the bus by CAN ID, the same way
 * the Bosch M_CAN based drivers rely on hardware arbitration rather than a
 * software priority field.
 */
#define CAN_ESP32_TWAIFD_TX_BUFFERS 4U

/* TXT buffer status codes returned by twaifd_ll_get_tx_buffer_status(). */
#define CAN_ESP32_TWAIFD_TXT_TOK 0x4U
#define CAN_ESP32_TWAIFD_TXT_ERR 0x6U

/* Secondary sample point offset register is 8 bits wide. */
#define CAN_ESP32_TWAIFD_SSP_MAX 255U

struct can_esp32_twaifd_filter {
	struct can_filter filter;
	can_rx_callback_t cb;
	void *user_data;
};

struct can_esp32_twaifd_tx_ctx {
	can_tx_callback_t cb;
	void *user_data;
};

struct can_esp32_twaifd_config {
	const struct can_driver_config common;
	mm_reg_t base;
	int controller_id;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	int irq_source;
	int irq_priority;
	int irq_flags;
};

struct can_esp32_twaifd_data {
	struct can_driver_data common;
	twai_hal_context_t hal;
	struct k_mutex lock;
	struct k_sem tx_sem;
	struct can_esp32_twaifd_tx_ctx tx_ctx[CAN_ESP32_TWAIFD_TX_BUFFERS];
	struct can_esp32_twaifd_filter filters[CAN_ESP32_TWAIFD_MAX_FILTER];
	uint32_t clock_source_hz;
	uint8_t tx_buffer_count;
	enum can_state state;
	intr_handle_t intr_handle;
};

static int can_esp32_twaifd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	struct can_esp32_twaifd_data *data = dev->data;

	*rate = data->clock_source_hz;
	return 0;
}

static int can_esp32_twaifd_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT;

	if (IS_ENABLED(CONFIG_CAN_FD_MODE)) {
		*cap |= CAN_MODE_FD;
	}

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		*cap |= CAN_MODE_MANUAL_RECOVERY;
	}

	return 0;
}

static int can_esp32_twaifd_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CAN_ESP32_TWAIFD_MAX_FILTER;
}

static int can_esp32_twaifd_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_esp32_twaifd_data *data = dev->data;
	can_mode_t caps = 0;
	int err;

	if (data->common.started) {
		return -EBUSY;
	}

	err = can_esp32_twaifd_get_capabilities(dev, &caps);
	if (err != 0) {
		return err;
	}

	if ((mode & ~caps) != 0) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	data->common.mode = mode;

	/*
	 * No HAL setter for mode after init; program the LL directly. Self-test
	 * is enabled together with internal loopback so the controller ACKs its
	 * own frames; without it, loopback tests on a single TX/RX wire (no
	 * transceiver) would fail with ACK errors. The Zephyr CAN mode API has
	 * no separate self-test bit, so the two are tied.
	 */
	twaifd_ll_set_mode(data->hal.dev, !!(mode & CAN_MODE_LISTENONLY),
			   !!(mode & CAN_MODE_LOOPBACK), !!(mode & CAN_MODE_LOOPBACK));
	twaifd_ll_set_tx_retrans_limit(data->hal.dev, (mode & CAN_MODE_ONE_SHOT) ? 0 : -1);

	/*
	 * Mirror mode into the HAL context so HAL state-tracking helpers see
	 * the new configuration. This couples to HAL-private fields; if the
	 * HAL ever exposes a twai_hal_set_mode() helper, switch to it.
	 */
	data->hal.enable_loopback = !!(mode & CAN_MODE_LOOPBACK);
	data->hal.enable_self_test = !!(mode & CAN_MODE_LOOPBACK);
	data->hal.enable_listen_only = !!(mode & CAN_MODE_LISTENONLY);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int can_esp32_twaifd_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_esp32_twaifd_data *data = dev->data;
	twai_timing_advanced_config_t t = {
		.brp = timing->prescaler,
		.prop_seg = timing->prop_seg,
		.tseg_1 = timing->phase_seg1,
		.tseg_2 = timing->phase_seg2,
		.sjw = timing->sjw,
		.ssp_offset = 0,
	};

	if (data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	twai_hal_configure_timing(&data->hal, &t);
	k_mutex_unlock(&data->lock);

	return 0;
}

#ifdef CONFIG_CAN_FD_MODE
static int can_esp32_twaifd_set_timing_data(const struct device *dev,
					    const struct can_timing *timing_data)
{
	struct can_esp32_twaifd_data *data = dev->data;
	twai_timing_advanced_config_t t = {
		.brp = timing_data->prescaler,
		.prop_seg = timing_data->prop_seg,
		.tseg_1 = timing_data->phase_seg1,
		.tseg_2 = timing_data->phase_seg2,
		.sjw = timing_data->sjw,
		/* Transmitter delay compensation: the secondary sample point is
		 * the controller-measured transmitter delay (TDC) plus a fixed
		 * offset (TDCO). Only the TDCO is configured here; ssp_offset
		 * carries it in time quanta. CAN_CALC_TDCO() returns the offset
		 * in minimum time quanta (clock cycles), so divide by the
		 * prescaler to convert to time quanta. The TDCO max is scaled by
		 * the prescaler so the clamp lands on CAN_ESP32_TWAIFD_SSP_MAX
		 * after the division.
		 */
		.ssp_offset = CAN_CALC_TDCO(timing_data, 0U,
					    CAN_ESP32_TWAIFD_SSP_MAX * timing_data->prescaler) /
			      timing_data->prescaler,
	};

	if (data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	twai_hal_configure_timing_fd(&data->hal, &t);
	k_mutex_unlock(&data->lock);

	return 0;
}
#endif /* CONFIG_CAN_FD_MODE */

static int can_esp32_twaifd_start(const struct device *dev)
{
	const struct can_esp32_twaifd_config *cfg = dev->config;
	struct can_esp32_twaifd_data *data = dev->data;
	int err;

	if (data->common.started) {
		return -EALREADY;
	}

	if (cfg->common.phy != NULL) {
		err = can_transceiver_enable(cfg->common.phy, data->common.mode);
		if (err != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", err);
			return err;
		}
	}

	CAN_STATS_RESET(dev);

	k_mutex_lock(&data->lock, K_FOREVER);
	/*
	 * twai_hal_start() polls the fault-state-change interrupt to wait
	 * for the 11 recessive bit bus integration. Keep the ISR disabled
	 * across this window so it cannot consume the FCSI status bit
	 * before the poll observes it; re-enable right after.
	 */
	twai_hal_start(&data->hal);
	data->state = CAN_STATE_ERROR_ACTIVE;
	k_mutex_unlock(&data->lock);

	err = esp_intr_enable(data->intr_handle);
	if (err != 0) {
		LOG_ERR("failed to enable TWAI interrupt (err %d)", err);
		k_mutex_lock(&data->lock, K_FOREVER);
		twai_hal_stop(&data->hal);
		k_mutex_unlock(&data->lock);
		if (cfg->common.phy != NULL) {
			(void)can_transceiver_disable(cfg->common.phy);
		}
		return err;
	}

	/*
	 * Flip started last so send() cannot observe a startable controller
	 * until the ISR is live to complete the TX.
	 */
	data->common.started = true;

	return 0;
}

static int can_esp32_twaifd_stop(const struct device *dev)
{
	const struct can_esp32_twaifd_config *cfg = dev->config;
	struct can_esp32_twaifd_data *data = dev->data;
	int err;

	if (!data->common.started) {
		return -EALREADY;
	}

	err = esp_intr_disable(data->intr_handle);
	if (err != 0) {
		LOG_ERR("failed to disable TWAI interrupt (err %d)", err);
		return err;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	twai_hal_stop(&data->hal);
	data->common.started = false;

	/*
	 * Abort any pending transmissions. twai_hal_stop() disables the
	 * controller which returns every TXT buffer to the Empty state, so
	 * the callbacks installed here will not race a late TX-done.
	 */
	for (uint8_t i = 0; i < data->tx_buffer_count; i++) {
		can_tx_callback_t cb = data->tx_ctx[i].cb;
		void *user_data = data->tx_ctx[i].user_data;

		if (cb != NULL) {
			data->tx_ctx[i].cb = NULL;
			cb(dev, -ENETDOWN, user_data);
			k_sem_give(&data->tx_sem);
		}
	}
	k_mutex_unlock(&data->lock);

	if (cfg->common.phy != NULL) {
		err = can_transceiver_disable(cfg->common.phy);
		if (err != 0) {
			LOG_ERR("failed to disable CAN transceiver (err %d)", err);
			return err;
		}
	}

	return 0;
}

static int can_esp32_twaifd_get_state(const struct device *dev, enum can_state *state,
				      struct can_bus_err_cnt *err_cnt)
{
	struct can_esp32_twaifd_data *data = dev->data;

	if (state != NULL) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else {
			*state = data->state;
		}
	}

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt = twai_hal_get_tec(&data->hal);
		err_cnt->rx_err_cnt = twai_hal_get_rec(&data->hal);
	}

	return 0;
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int can_esp32_twaifd_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_esp32_twaifd_data *data = dev->data;

	ARG_UNUSED(timeout);

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if ((data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	twai_hal_start_bus_recovery(&data->hal);
	k_mutex_unlock(&data->lock);

	return 0;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

/*
 * The state-change callback is dispatched directly from ISR context, so it
 * must not call blocking Zephyr APIs. This matches the Zephyr CAN API
 * contract for the TX and RX callbacks, which are also invoked from the ISR.
 */
static void can_esp32_twaifd_set_state_change_callback(const struct device *dev,
						       can_state_change_callback_t cb,
						       void *user_data)
{
	struct can_esp32_twaifd_data *data = dev->data;
	unsigned int key;

	/* Pair update atomic vs the ISR reading both fields. */
	key = irq_lock();
	data->common.state_change_cb = cb;
	data->common.state_change_cb_user_data = user_data;
	irq_unlock(key);
}

static int can_esp32_twaifd_send(const struct device *dev, const struct can_frame *frame,
				 k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	struct can_esp32_twaifd_data *data = dev->data;
	uint8_t frame_buf[CAN_MAX_DLEN];
	twai_hal_frame_t hw_frame = {0};
	twai_frame_header_t header = {0};
	twai_hal_trans_desc_t desc = {
		.frame.header = &header,
		.frame.buffer = frame_buf,
		.frame.buffer_len = 0,
	};
	uint8_t dlc;
	uint8_t data_len;
	int buffer_id = -1;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR | CAN_FRAME_FDF | CAN_FRAME_BRS)) !=
	    0) {
		return -ENOTSUP;
	}

	if ((frame->flags & CAN_FRAME_FDF) != 0) {
		if (!IS_ENABLED(CONFIG_CAN_FD_MODE)) {
			return -ENOTSUP;
		}
		if ((data->common.mode & CAN_MODE_FD) == 0) {
			return -ENOTSUP;
		}
		if (frame->dlc > CANFD_MAX_DLC) {
			return -EINVAL;
		}
	} else {
		if (frame->dlc > CAN_MAX_DLC) {
			return -EINVAL;
		}
		if ((frame->flags & CAN_FRAME_BRS) != 0) {
			return -ENOTSUP;
		}
	}

	dlc = frame->dlc;
	data_len = can_dlc_to_bytes(dlc);

	header.id = frame->id;
	header.dlc = dlc;
	header.ide = !!(frame->flags & CAN_FRAME_IDE);
	header.rtr = !!(frame->flags & CAN_FRAME_RTR);
	header.fdf = !!(frame->flags & CAN_FRAME_FDF);
	header.brs = !!(frame->flags & CAN_FRAME_BRS);

	if (!header.rtr) {
		memcpy(frame_buf, frame->data, data_len);
		desc.frame.buffer_len = data_len;
	}

	if (k_sem_take(&data->tx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/*
	 * A free TXT buffer is guaranteed because tx_sem (counting available
	 * buffers) was just acquired. The mutex serializes concurrent send()
	 * and stop() callers; ISR safety instead relies on writing cb before
	 * issuing the transmit, and on the ISR clearing the slot and giving
	 * the sem before this slot can be reselected.
	 */
	for (uint8_t i = 0; i < data->tx_buffer_count; i++) {
		if (data->tx_ctx[i].cb == NULL) {
			buffer_id = i;
			break;
		}
	}
	__ASSERT_NO_MSG(buffer_id >= 0);

	desc.config.retry_cnt = (data->common.mode & CAN_MODE_ONE_SHOT) ? 0 : -1;
	data->tx_ctx[buffer_id].cb = callback;
	data->tx_ctx[buffer_id].user_data = user_data;

	twai_hal_format_frame(&desc, &hw_frame);
	twai_hal_set_tx_buffer_and_transmit(&data->hal, &hw_frame, (uint8_t)buffer_id);
	k_mutex_unlock(&data->lock);

	return 0;
}

static int can_esp32_twaifd_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
					  void *user_data, const struct can_filter *filter)
{
	struct can_esp32_twaifd_data *data = dev->data;
	int filter_id = -ENOSPC;

	if (callback == NULL) {
		return -EINVAL;
	}

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	for (int i = 0; i < CAN_ESP32_TWAIFD_MAX_FILTER; i++) {
		if (data->filters[i].cb == NULL) {
			data->filters[i].filter = *filter;
			data->filters[i].user_data = user_data;
			/*
			 * cb is written last so the ISR observing cb != NULL
			 * also observes the matching filter and user_data.
			 */
			data->filters[i].cb = callback;
			filter_id = i;
			break;
		}
	}
	k_mutex_unlock(&data->lock);

	return filter_id;
}

static void can_esp32_twaifd_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_esp32_twaifd_data *data = dev->data;

	if (filter_id < 0 || filter_id >= CAN_ESP32_TWAIFD_MAX_FILTER) {
		return;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	/*
	 * cb is cleared first so the ISR sees the slot as inactive before
	 * filter and user_data become stale.
	 */
	data->filters[filter_id].cb = NULL;
	data->filters[filter_id].user_data = NULL;
	data->filters[filter_id].filter = (struct can_filter){0};
	k_mutex_unlock(&data->lock);
}

static void IRAM_ATTR can_esp32_twaifd_handle_rx(const struct device *dev)
{
	struct can_esp32_twaifd_data *data = dev->data;
	twai_hal_frame_t rx_frame;
	twai_frame_header_t header;
	struct can_frame frame;
	uint8_t buffer[CAN_MAX_DLEN];

	while (twai_hal_get_rx_msg_count(&data->hal) > 0) {
		if (!twai_hal_read_rx_fifo(&data->hal, &rx_frame)) {
			continue;
		}

		memset(&frame, 0, sizeof(frame));
		twai_hal_parse_frame(&data->hal, &rx_frame, &header, buffer, sizeof(buffer));

		frame.id = header.id;
		frame.dlc = header.dlc;
		frame.flags = (header.ide ? CAN_FRAME_IDE : 0) | (header.rtr ? CAN_FRAME_RTR : 0) |
			      (header.fdf ? CAN_FRAME_FDF : 0) | (header.brs ? CAN_FRAME_BRS : 0) |
			      (header.esi ? CAN_FRAME_ESI : 0);

		if (!header.rtr) {
			memcpy(frame.data, buffer,
			       MIN(can_dlc_to_bytes(header.dlc), sizeof(frame.data)));
		}

#ifdef CONFIG_CAN_RX_TIMESTAMP
		/*
		 * The hardware timestamp is 64-bit; can_frame.timestamp is
		 * 16-bit by API design so the low word wraps every ~65k
		 * ticks of the configured CAN timer.
		 */
		frame.timestamp = (uint16_t)header.timestamp;
#endif

		if (!IS_ENABLED(CONFIG_CAN_ACCEPT_RTR) && (frame.flags & CAN_FRAME_RTR) != 0) {
			continue;
		}

		for (int i = 0; i < CAN_ESP32_TWAIFD_MAX_FILTER; i++) {
			can_rx_callback_t cb = data->filters[i].cb;
			void *cb_user_data = data->filters[i].user_data;
			struct can_filter f;

			if (cb == NULL) {
				continue;
			}
			f = data->filters[i].filter;
			if (can_frame_matches_filter(&frame, &f)) {
				cb(dev, &frame, cb_user_data);
			}
		}
	}
}

/*
 * The TX-done interrupt is global: it does not identify which TXT buffer
 * completed. Scan the in-use buffers, read each one's status register, and
 * complete the ones that reached the TX-OK or TX-Failed terminal state.
 */
static void IRAM_ATTR can_esp32_twaifd_handle_tx(const struct device *dev)
{
	struct can_esp32_twaifd_data *data = dev->data;

	for (uint8_t i = 0; i < data->tx_buffer_count; i++) {
		can_tx_callback_t cb = data->tx_ctx[i].cb;
		void *user_data = data->tx_ctx[i].user_data;
		uint32_t raw;
		uint32_t status;
		int tx_status;

		if (cb == NULL) {
			continue;
		}

		raw = twaifd_ll_get_tx_buffer_status(data->hal.dev, i);
		status = (raw >> (i * TWAIFD_TX2S_S)) & TWAIFD_TX2S_V;

		if (status != CAN_ESP32_TWAIFD_TXT_TOK && status != CAN_ESP32_TWAIFD_TXT_ERR) {
			continue;
		}

		tx_status = (status == CAN_ESP32_TWAIFD_TXT_TOK) ? 0 : -EIO;

		/*
		 * Return the TXT buffer to the Empty state so the next transmit
		 * request can transition it to Ready. The hardware disallows a
		 * direct TX_OK -> Ready transition.
		 */
		twaifd_ll_set_tx_buffer_cmd(data->hal.dev, i, TWAIFD_LL_TX_CMD_EMPTY);

		/*
		 * Clear cb before signaling the buffer is free; the next send()
		 * can only claim this slot after the sem is given, so it always
		 * observes the cleared slot before installing its own callback.
		 */
		data->tx_ctx[i].cb = NULL;
		cb(dev, tx_status, user_data);
		k_sem_give(&data->tx_sem);
	}
}

static void IRAM_ATTR can_esp32_twaifd_update_state(const struct device *dev, uint32_t events)
{
	struct can_esp32_twaifd_data *data = dev->data;
	can_state_change_callback_t cb;
	void *user_data;
	enum can_state new_state = data->state;
	struct can_bus_err_cnt err_cnt;

	if (events & TWAI_HAL_EVENT_BUS_OFF) {
		new_state = CAN_STATE_BUS_OFF;
	} else if (events & TWAI_HAL_EVENT_ERROR_PASSIVE) {
		new_state = CAN_STATE_ERROR_PASSIVE;
	} else if (events & TWAI_HAL_EVENT_ERROR_WARNING) {
		new_state = CAN_STATE_ERROR_WARNING;
	} else if (events & TWAI_HAL_EVENT_ERROR_ACTIVE) {
		new_state = CAN_STATE_ERROR_ACTIVE;
	}

	if (new_state == data->state) {
		return;
	}

	LOG_DBG("bus state change: %d -> %d", data->state, new_state);
	data->state = new_state;

	/*
	 * On bus-off the controller forces every TXT buffer to the TX-Failed
	 * state, so abort the pending transmissions and release their slots.
	 * When manual recovery is not requested, trigger recovery from here so
	 * the controller re-integrates automatically after the bus heals.
	 */
	if (new_state == CAN_STATE_BUS_OFF) {
		for (uint8_t i = 0; i < data->tx_buffer_count; i++) {
			can_tx_callback_t tx_cb = data->tx_ctx[i].cb;
			void *tx_user_data = data->tx_ctx[i].user_data;

			if (tx_cb != NULL) {
				data->tx_ctx[i].cb = NULL;
				tx_cb(dev, -ENETUNREACH, tx_user_data);
				k_sem_give(&data->tx_sem);
			}
		}

		if (!IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE) ||
		    (data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
			twai_hal_start_bus_recovery(&data->hal);
		}
	}

	cb = data->common.state_change_cb;
	user_data = data->common.state_change_cb_user_data;
	if (cb == NULL) {
		return;
	}

	err_cnt.tx_err_cnt = twai_hal_get_tec(&data->hal);
	err_cnt.rx_err_cnt = twai_hal_get_rec(&data->hal);

	cb(dev, new_state, err_cnt, user_data);
}

static void IRAM_ATTR can_esp32_twaifd_isr(void *arg)
{
	const struct device *dev = arg;
	struct can_esp32_twaifd_data *data = dev->data;
	uint32_t events;

	events = twai_hal_get_events(&data->hal);

	if (events & TWAI_HAL_EVENT_RX_BUFF_FRAME) {
		can_esp32_twaifd_handle_rx(dev);
	}

	if (events & TWAI_HAL_EVENT_TX_BUFF_FREE) {
		can_esp32_twaifd_handle_tx(dev);
	}

	if (events & TWAI_HAL_EVENT_BUS_ERR) {
		twai_error_flags_t err_flags = twai_hal_get_err_flags(&data->hal);

		if (err_flags.bit_err) {
			CAN_STATS_BIT_ERROR_INC(dev);
		}
		if (err_flags.stuff_err) {
			CAN_STATS_STUFF_ERROR_INC(dev);
		}
		if (err_flags.form_err) {
			CAN_STATS_FORM_ERROR_INC(dev);
		}
		if (err_flags.ack_err) {
			CAN_STATS_ACK_ERROR_INC(dev);
		}
	}

	if (events & (TWAI_HAL_EVENT_BUS_OFF | TWAI_HAL_EVENT_ERROR_PASSIVE |
		      TWAI_HAL_EVENT_ERROR_WARNING | TWAI_HAL_EVENT_ERROR_ACTIVE)) {
		can_esp32_twaifd_update_state(dev, events);
	}
}

static int can_esp32_twaifd_init(const struct device *dev)
{
	const struct can_esp32_twaifd_config *cfg = dev->config;
	struct can_esp32_twaifd_data *data = dev->data;
	twai_hal_config_t hal_cfg;
	struct can_timing timing = {0};
	int err;
#ifdef CONFIG_CAN_FD_MODE
	struct can_timing timing_data = {0};
#endif

	k_mutex_init(&data->lock);

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->clock_dev);
		return -ENODEV;
	}

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		LOG_ERR("pinctrl apply failed (err %d)", err);
		return err;
	}

	err = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (err != 0) {
		LOG_ERR("failed to enable TWAI clock (err %d)", err);
		return err;
	}

	err = esp_clk_tree_src_get_freq_hz(TWAI_CLK_SRC_DEFAULT,
					   ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
					   &data->clock_source_hz);
	if (err != 0) {
		LOG_ERR("failed to query TWAI clock frequency (err %d)", err);
		goto err_clock_off;
	}

	hal_cfg = (twai_hal_config_t){
		.controller_id = cfg->controller_id,
		.clock_source_hz = data->clock_source_hz,
		.intr_mask = TWAI_LL_DRIVER_INTERRUPTS | TWAIFD_LL_INTR_TX_FRAME,
		.retry_cnt = -1,
	};
	if (!twai_hal_init(&data->hal, &hal_cfg)) {
		LOG_ERR("twai_hal_init failed");
		err = -EIO;
		goto err_clock_off;
	}

	/*
	 * Cap the software TX pool at the number of TXT buffers the IP exposes.
	 * tx_sem counts available buffers so send() blocks once all are queued.
	 */
	data->tx_buffer_count = (uint8_t)MIN(twaifd_ll_get_tx_buffer_total(data->hal.dev),
					     CAN_ESP32_TWAIFD_TX_BUFFERS);
	k_sem_init(&data->tx_sem, data->tx_buffer_count, data->tx_buffer_count);

	err = can_calc_timing(dev, &timing, cfg->common.bitrate, cfg->common.sample_point);
	if (err < 0) {
		LOG_ERR("failed to compute timing (err %d)", err);
		goto err_clock_off;
	}
	err = can_esp32_twaifd_set_timing(dev, &timing);
	if (err != 0) {
		LOG_ERR("failed to apply nominal timing (err %d)", err);
		goto err_clock_off;
	}

#ifdef CONFIG_CAN_FD_MODE
	if (cfg->common.bitrate_data != 0) {
		err = can_calc_timing_data(dev, &timing_data, cfg->common.bitrate_data,
					   cfg->common.sample_point_data);
		if (err < 0) {
			LOG_ERR("failed to compute FD timing (err %d)", err);
			goto err_clock_off;
		}
		err = can_esp32_twaifd_set_timing_data(dev, &timing_data);
		if (err != 0) {
			LOG_ERR("failed to apply data timing (err %d)", err);
			goto err_clock_off;
		}
	}
#endif

	err = esp_intr_alloc(cfg->irq_source,
			     ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
				     ESP_INT_FLAGS_CHECK(cfg->irq_flags) | ESP_INTR_FLAG_IRAM |
				     ESP_INTR_FLAG_INTRDISABLED,
			     can_esp32_twaifd_isr, (void *)dev, &data->intr_handle);
	if (err != 0) {
		LOG_ERR("intr alloc failed (err %d)", err);
		goto err_clock_off;
	}

	data->state = CAN_STATE_STOPPED;
	return 0;

err_clock_off:
	(void)clock_control_off(cfg->clock_dev, cfg->clock_subsys);
	return err;
}

static DEVICE_API(can, can_esp32_twaifd_api) = {
	.get_capabilities = can_esp32_twaifd_get_capabilities,
	.start = can_esp32_twaifd_start,
	.stop = can_esp32_twaifd_stop,
	.set_mode = can_esp32_twaifd_set_mode,
	.set_timing = can_esp32_twaifd_set_timing,
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_esp32_twaifd_set_timing_data,
#endif
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_esp32_twaifd_recover,
#endif
	.send = can_esp32_twaifd_send,
	.add_rx_filter = can_esp32_twaifd_add_rx_filter,
	.remove_rx_filter = can_esp32_twaifd_remove_rx_filter,
	.get_state = can_esp32_twaifd_get_state,
	.set_state_change_callback = can_esp32_twaifd_set_state_change_callback,
	.get_core_clock = can_esp32_twaifd_get_core_clock,
	.get_max_filters = can_esp32_twaifd_get_max_filters,
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 1,
	},
	.timing_max = {
		.sjw = 31,
		.prop_seg = 127,
		.phase_seg1 = 63,
		.phase_seg2 = 63,
		.prescaler = 255,
	},
#ifdef CONFIG_CAN_FD_MODE
	.timing_data_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 1,
	},
	.timing_data_max = {
		.sjw = 16,
		.prop_seg = 31,
		.phase_seg1 = 31,
		.phase_seg2 = 16,
		.prescaler = 255,
	},
#endif
};

/*
 * Derive the hardware controller index (0 or 1) from the node's reg address
 * by matching it against DR_REG_TWAI0_BASE and DR_REG_TWAI1_BASE. The reg
 * address in devicetree is already the identity - no separate controller-id
 * property needed in the binding. Returns -1 if the address matches neither
 * known instance so a BUILD_ASSERT() can flag the bad configuration.
 */
#define CAN_ESP32_TWAIFD_HW_INSTANCE(inst)                                                         \
	(DT_INST_REG_ADDR(inst) == DR_REG_TWAI0_BASE   ? 0                                         \
	 : DT_INST_REG_ADDR(inst) == DR_REG_TWAI1_BASE ? 1                                         \
						       : -1)

#define CAN_ESP32_TWAIFD_INIT(inst)                                                                \
	BUILD_ASSERT(CAN_ESP32_TWAIFD_HW_INSTANCE(inst) >= 0,                                      \
		     "TWAI-FD reg address does not match a known controller base");                \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct can_esp32_twaifd_config can_esp32_twaifd_cfg_##inst = {                \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 5000000),                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.controller_id = CAN_ESP32_TWAIFD_HW_INSTANCE(inst),                               \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, offset),         \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.irq_source = DT_INST_IRQ_BY_IDX(inst, 0, irq),                                    \
		.irq_priority = DT_INST_IRQ_BY_IDX(inst, 0, priority),                             \
		.irq_flags = DT_INST_IRQ_BY_IDX(inst, 0, flags),                                   \
	};                                                                                         \
                                                                                                   \
	static struct can_esp32_twaifd_data can_esp32_twaifd_data_##inst;                          \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_esp32_twaifd_init, NULL,                               \
				  &can_esp32_twaifd_data_##inst, &can_esp32_twaifd_cfg_##inst,     \
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY, &can_esp32_twaifd_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_ESP32_TWAIFD_INIT)
