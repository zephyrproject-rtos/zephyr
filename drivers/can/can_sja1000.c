/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "can_sja1000.h"
#include "can_sja1000_priv.h"
#include "can_utils.h"

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_sja1000, CONFIG_CAN_LOG_LEVEL);

/* Timeout for entering/leaving reset mode */
#define CAN_SJA1000_RESET_MODE_TIMEOUT_USEC 1000
#define CAN_SJA1000_RESET_MODE_RETRIES      100
#define CAN_SJA1000_RESET_MODE_DELAY	    \
	K_USEC(CAN_SJA1000_RESET_MODE_TIMEOUT_USEC / CAN_SJA1000_RESET_MODE_RETRIES)

static inline void can_sja1000_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct can_sja1000_config *config = dev->config;

	LOG_DBG("write reg %d = 0x%02x", reg, val);
	return config->write_reg(dev, reg, val);
}

static inline uint8_t can_sja1000_read_reg(const struct device *dev, uint8_t reg)
{
	const struct can_sja1000_config *config = dev->config;
	uint8_t val;

	val = config->read_reg(dev, reg);
	LOG_DBG("read reg %d = 0x%02x", reg, val);

	return val;
}

static inline int can_sja1000_enter_reset_mode(const struct device *dev)
{
	int retries = CAN_SJA1000_RESET_MODE_RETRIES;
	uint8_t mod;

	mod = can_sja1000_read_reg(dev, CAN_SJA1000_MOD);

	while ((mod & CAN_SJA1000_MOD_RM) == 0) {
		if (--retries < 0) {
			return -EIO;
		}

		can_sja1000_write_reg(dev, CAN_SJA1000_MOD, mod | CAN_SJA1000_MOD_RM);
		k_sleep(CAN_SJA1000_RESET_MODE_DELAY);
		mod = can_sja1000_read_reg(dev, CAN_SJA1000_MOD);
	};

	return 0;
}

static inline int can_sja1000_leave_reset_mode(const struct device *dev)
{
	int retries = CAN_SJA1000_RESET_MODE_RETRIES;
	uint8_t mod;

	mod = can_sja1000_read_reg(dev, CAN_SJA1000_MOD);

	while ((mod & CAN_SJA1000_MOD_RM) == 1) {
		if (--retries < 0) {
			return -EIO;
		}

		can_sja1000_write_reg(dev, CAN_SJA1000_MOD, mod & ~(CAN_SJA1000_MOD_RM));
		k_sleep(CAN_SJA1000_RESET_MODE_DELAY);
		mod = can_sja1000_read_reg(dev, CAN_SJA1000_MOD);
	};

	return 0;
}

static inline void can_sja1000_clear_errors(const struct device *dev)
{
	/* Clear error counters */
	can_sja1000_write_reg(dev, CAN_SJA1000_RXERR, 0);
	can_sja1000_write_reg(dev, CAN_SJA1000_TXERR, 0);

	/* Clear error capture */
	(void)can_sja1000_read_reg(dev, CAN_SJA1000_ECC);
}

static void can_sja1000_tx_done(const struct device *dev, int status)
{
	struct can_sja1000_data *data = dev->data;
	can_tx_callback_t callback = data->tx_callback;
	void *user_data = data->tx_user_data;

	if (callback != NULL) {
		data->tx_callback = NULL;
		callback(dev, status, user_data);
	}

	k_sem_give(&data->tx_idle);
}

int can_sja1000_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_sja1000_data *data = dev->data;
	uint8_t btr0;
	uint8_t btr1;
	uint8_t sjw;

	__ASSERT_NO_MSG(timing->sjw == CAN_SJW_NO_CHANGE || (timing->sjw >= 1 && timing->sjw <= 4));
	__ASSERT_NO_MSG(timing->prop_seg == 0);
	__ASSERT_NO_MSG(timing->phase_seg1 >= 1 && timing->phase_seg1 <= 16);
	__ASSERT_NO_MSG(timing->phase_seg2 >= 1 && timing->phase_seg2 <= 8);
	__ASSERT_NO_MSG(timing->prescaler >= 1 && timing->prescaler <= 64);

	if (data->started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->mod_lock, K_FOREVER);

	if (timing->sjw == CAN_SJW_NO_CHANGE) {
		sjw = data->sjw;
	} else {
		sjw = timing->sjw;
		data->sjw = timing->sjw;
	}

	btr0 = CAN_SJA1000_BTR0_BRP_PREP(timing->prescaler - 1) |
	       CAN_SJA1000_BTR0_SJW_PREP(sjw - 1);
	btr1 = CAN_SJA1000_BTR1_TSEG1_PREP(timing->phase_seg1 - 1) |
	       CAN_SJA1000_BTR1_TSEG2_PREP(timing->phase_seg2 - 1);

	if ((data->mode & CAN_MODE_3_SAMPLES) != 0) {
		btr1 |= CAN_SJA1000_BTR1_SAM;
	}

	can_sja1000_write_reg(dev, CAN_SJA1000_BTR0, btr0);
	can_sja1000_write_reg(dev, CAN_SJA1000_BTR1, btr1);

	k_mutex_unlock(&data->mod_lock);

	return 0;
}

int can_sja1000_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY |
	       CAN_MODE_ONE_SHOT | CAN_MODE_3_SAMPLES;

	return 0;
}

int can_sja1000_start(const struct device *dev)
{
	const struct can_sja1000_config *config = dev->config;
	struct can_sja1000_data *data = dev->data;
	int err;

	if (data->started) {
		return -EALREADY;
	}

	if (config->phy != NULL) {
		err = can_transceiver_enable(config->phy);
		if (err != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", err);
			return err;
		}
	}

	can_sja1000_clear_errors(dev);

	err = can_sja1000_leave_reset_mode(dev);
	if (err != 0) {
		if (config->phy != NULL) {
			/* Attempt to disable the CAN transceiver in case of error */
			(void)can_transceiver_disable(config->phy);
		}

		return err;
	}

	data->started = true;

	return 0;
}

int can_sja1000_stop(const struct device *dev)
{
	const struct can_sja1000_config *config = dev->config;
	struct can_sja1000_data *data = dev->data;
	int err;

	if (!data->started) {
		return -EALREADY;
	}

	/* Entering reset mode aborts current transmission, if any */
	err = can_sja1000_enter_reset_mode(dev);
	if (err != 0) {
		return err;
	}

	if (config->phy != NULL) {
		err = can_transceiver_disable(config->phy);
		if (err != 0) {
			LOG_ERR("failed to disable CAN transceiver (err %d)", err);
			return err;
		}
	}

	data->started = false;

	can_sja1000_tx_done(dev, -ENETDOWN);

	return 0;
}

int can_sja1000_set_mode(const struct device *dev, can_mode_t mode)
{
	const struct can_sja1000_config *config = dev->config;
	struct can_sja1000_data *data = dev->data;
	uint8_t btr1;
	uint8_t mod;

	if ((mode & ~(CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT |
		      CAN_MODE_3_SAMPLES)) != 0) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	if (data->started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->mod_lock, K_FOREVER);

	mod = can_sja1000_read_reg(dev, CAN_SJA1000_MOD);
	mod |= CAN_SJA1000_MOD_AFM;

	if ((mode & CAN_MODE_LOOPBACK) != 0) {
		/* (Local) self test mode */
		mod |= CAN_SJA1000_MOD_STM;
	} else {
		mod &= ~(CAN_SJA1000_MOD_STM);
	}

	if ((mode & CAN_MODE_LISTENONLY) != 0) {
		mod |= CAN_SJA1000_MOD_LOM;
	} else {
		mod &= ~(CAN_SJA1000_MOD_LOM);
	}

	btr1 = can_sja1000_read_reg(dev, CAN_SJA1000_BTR1);
	if ((mode & CAN_MODE_3_SAMPLES) != 0) {
		btr1 |= CAN_SJA1000_BTR1_SAM;
	} else {
		btr1 &= ~(CAN_SJA1000_BTR1_SAM);
	}

	can_sja1000_write_reg(dev, CAN_SJA1000_MOD, mod);
	can_sja1000_write_reg(dev, CAN_SJA1000_BTR1, btr1);

	data->mode = mode;

	k_mutex_unlock(&data->mod_lock);

	return 0;
}

static void can_sja1000_read_frame(const struct device *dev, struct can_frame *frame)
{
	uint8_t info;
	int i;

	memset(frame, 0, sizeof(*frame));

	info = can_sja1000_read_reg(dev, CAN_SJA1000_FRAME_INFO);

	if ((info & CAN_SJA1000_FRAME_INFO_RTR) != 0) {
		frame->rtr = CAN_REMOTEREQUEST;
	} else {
		frame->rtr = CAN_DATAFRAME;
	}

	frame->dlc = CAN_SJA1000_FRAME_INFO_DLC_GET(info);
	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("RX frame DLC %u exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return;
	}

	if ((info & CAN_SJA1000_FRAME_INFO_FF) != 0) {
		frame->id_type = CAN_EXTENDED_IDENTIFIER;

		frame->id = FIELD_PREP(GENMASK(28, 21),
				can_sja1000_read_reg(dev, CAN_SJA1000_XFF_ID1));
		frame->id |= FIELD_PREP(GENMASK(20, 13),
				can_sja1000_read_reg(dev, CAN_SJA1000_XFF_ID2));
		frame->id |= FIELD_PREP(GENMASK(12, 5),
				can_sja1000_read_reg(dev, CAN_SJA1000_EFF_ID3));
		frame->id |= FIELD_PREP(GENMASK(4, 0),
				can_sja1000_read_reg(dev, CAN_SJA1000_EFF_ID4) >> 3);

		for (i = 0; i < frame->dlc; i++) {
			frame->data[i] = can_sja1000_read_reg(dev, CAN_SJA1000_EFF_DATA + i);
		}
	} else {
		frame->id_type = CAN_STANDARD_IDENTIFIER;

		frame->id = FIELD_PREP(GENMASK(10, 3),
				can_sja1000_read_reg(dev, CAN_SJA1000_XFF_ID1));
		frame->id |= FIELD_PREP(GENMASK(2, 0),
				can_sja1000_read_reg(dev, CAN_SJA1000_XFF_ID2) >> 5);

		for (i = 0; i < frame->dlc; i++) {
			frame->data[i] = can_sja1000_read_reg(dev, CAN_SJA1000_SFF_DATA + i);
		}
	}
}

void can_sja1000_write_frame(const struct device *dev, const struct can_frame *frame)
{
	uint8_t info;
	int i;

	info = CAN_SJA1000_FRAME_INFO_DLC_PREP(frame->dlc);

	if (frame->rtr == CAN_REMOTEREQUEST) {
		info |= CAN_SJA1000_FRAME_INFO_RTR;
	}

	if (frame->id_type == CAN_EXTENDED_IDENTIFIER) {
		info |= CAN_SJA1000_FRAME_INFO_FF;
	}

	can_sja1000_write_reg(dev, CAN_SJA1000_FRAME_INFO, info);

	if (frame->id_type == CAN_EXTENDED_IDENTIFIER) {
		can_sja1000_write_reg(dev, CAN_SJA1000_XFF_ID1,
				FIELD_GET(GENMASK(28, 21), frame->id));
		can_sja1000_write_reg(dev, CAN_SJA1000_XFF_ID2,
				FIELD_GET(GENMASK(20, 13), frame->id));
		can_sja1000_write_reg(dev, CAN_SJA1000_EFF_ID3,
				FIELD_GET(GENMASK(12, 5), frame->id));
		can_sja1000_write_reg(dev, CAN_SJA1000_EFF_ID4,
				FIELD_GET(GENMASK(4, 0), frame->id) << 3);

		for (i = 0; i < frame->dlc; i++) {
			can_sja1000_write_reg(dev, CAN_SJA1000_EFF_DATA + i, frame->data[i]);
		}
	} else {
		can_sja1000_write_reg(dev, CAN_SJA1000_XFF_ID1,
				FIELD_GET(GENMASK(10, 3), frame->id));
		can_sja1000_write_reg(dev, CAN_SJA1000_XFF_ID2,
				FIELD_GET(GENMASK(2, 0), frame->id) << 5);

		for (i = 0; i < frame->dlc; i++) {
			can_sja1000_write_reg(dev, CAN_SJA1000_SFF_DATA + i, frame->data[i]);
		}
	}
}

int can_sja1000_send(const struct device *dev, const struct can_frame *frame, k_timeout_t timeout,
		     can_tx_callback_t callback, void *user_data)
{
	struct can_sja1000_data *data = dev->data;
	uint8_t cmr;
	uint8_t sr;

	__ASSERT_NO_MSG(callback != NULL);

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("TX frame DLC %u exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if (!data->started) {
		return -ENETDOWN;
	}

	if (data->state == CAN_STATE_BUS_OFF) {
		LOG_DBG("transmit failed, bus-off");
		return -ENETUNREACH;
	}

	if (k_sem_take(&data->tx_idle, timeout) != 0) {
		return -EAGAIN;
	}

	sr = can_sja1000_read_reg(dev, CAN_SJA1000_SR);
	if ((sr & CAN_SJA1000_SR_TBS) == 0) {
		LOG_ERR("transmit buffer locked, sr = 0x%02x", sr);
		return -EIO;
	}

	data->tx_callback = callback;
	data->tx_user_data = user_data;

	can_sja1000_write_frame(dev, frame);

	if ((data->mode & CAN_MODE_LOOPBACK) != 0) {
		cmr = CAN_SJA1000_CMR_SRR;
	} else {
		cmr = CAN_SJA1000_CMR_TR;
	}

	if ((data->mode & CAN_MODE_ONE_SHOT) != 0) {
		cmr |= CAN_SJA1000_CMR_AT;
	}

	can_sja1000_write_reg(dev, CAN_SJA1000_CMR, cmr);

	return 0;
}

int can_sja1000_add_rx_filter(const struct device *dev, can_rx_callback_t callback, void *user_data,
			      const struct can_filter *filter)
{
	struct can_sja1000_data *data = dev->data;
	int filter_id = -ENOSPC;
	int i;

	for (i = 0; i < ARRAY_SIZE(data->filters); i++) {
		if (!atomic_test_and_set_bit(data->rx_allocs, i)) {
			filter_id = i;
			break;
		}
	}

	if (filter_id >= 0) {
		data->filters[filter_id].filter = *filter;
		data->filters[filter_id].user_data = user_data;
		data->filters[filter_id].callback = callback;
	}

	return filter_id;
}

void can_sja1000_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_sja1000_data *data = dev->data;

	if (filter_id < 0 || filter_id >= ARRAY_SIZE(data->filters)) {
		LOG_ERR("filter ID %d out of bounds", filter_id);
		return;
	}

	if (atomic_test_and_clear_bit(data->rx_allocs, filter_id)) {
		data->filters[filter_id].callback = NULL;
		data->filters[filter_id].user_data = NULL;
		data->filters[filter_id].filter = (struct can_filter){0};
	}
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
int can_sja1000_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_sja1000_data *data = dev->data;
	int64_t start_ticks;
	uint8_t sr;
	int err;

	if (!data->started) {
		return -ENETDOWN;
	}

	sr = can_sja1000_read_reg(dev, CAN_SJA1000_SR);
	if ((sr & CAN_SJA1000_SR_BS) == 0) {
		return 0;
	}

	start_ticks = k_uptime_ticks();

	err = k_mutex_lock(&data->mod_lock, timeout);
	if (err != 0) {
		LOG_WRN("failed to acquire MOD lock");
		return err;
	}

	err = can_sja1000_leave_reset_mode(dev);
	if (err != 0) {
		LOG_ERR("failed to initiate bus recovery");
		k_mutex_unlock(&data->mod_lock);
		return err;
	}

	k_mutex_unlock(&data->mod_lock);

	while ((sr & CAN_SJA1000_SR_BS) != 0) {
		if (k_uptime_ticks() - start_ticks > timeout.ticks) {
			LOG_WRN("bus recovery timed out");
			return -EAGAIN;
		}

		sr = can_sja1000_read_reg(dev, CAN_SJA1000_SR);
	}

	return 0;
}
#endif /* !CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

int can_sja1000_get_state(const struct device *dev, enum can_state *state,
			  struct can_bus_err_cnt *err_cnt)
{
	struct can_sja1000_data *data = dev->data;

	if (state != NULL) {
		if (!data->started) {
			*state = CAN_STATE_STOPPED;
		} else {
			*state = data->state;
		}
	}

	if (err_cnt != NULL) {
		err_cnt->rx_err_cnt = can_sja1000_read_reg(dev, CAN_SJA1000_RXERR);
		err_cnt->tx_err_cnt = can_sja1000_read_reg(dev, CAN_SJA1000_TXERR);
	}

	return 0;
}

void can_sja1000_set_state_change_callback(const struct device *dev,
					   can_state_change_callback_t callback, void *user_data)
{
	struct can_sja1000_data *data = dev->data;

	data->state_change_cb = callback;
	data->state_change_cb_data = user_data;
}

int can_sja1000_get_max_filters(const struct device *dev, enum can_ide id_type)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id_type);

	return CONFIG_CAN_MAX_FILTER;
}

int can_sja1000_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct can_sja1000_config *config = dev->config;

	*max_bitrate = config->max_bitrate;

	return 0;
}

static void can_sja1000_handle_receive_irq(const struct device *dev)
{
	struct can_sja1000_data *data = dev->data;
	struct can_frame frame;
	can_rx_callback_t callback;
	uint8_t sr;
	int i;

	do {
		can_sja1000_read_frame(dev, &frame);

		for (i = 0; i < ARRAY_SIZE(data->filters); i++) {
			if (!atomic_test_bit(data->rx_allocs, i)) {
				continue;
			}

			if (can_utils_filter_match(&frame, &data->filters[i].filter)) {
				callback = data->filters[i].callback;
				if (callback != NULL) {
					callback(dev, &frame, data->filters[i].user_data);
				}
			}
		}

		can_sja1000_write_reg(dev, CAN_SJA1000_CMR, CAN_SJA1000_CMR_RRB);
		sr = can_sja1000_read_reg(dev, CAN_SJA1000_SR);
	} while ((sr & CAN_SJA1000_SR_RBS) != 0);
}

static void can_sja1000_handle_transmit_irq(const struct device *dev)
{
	int status = 0;
	uint8_t sr;

	sr = can_sja1000_read_reg(dev, CAN_SJA1000_SR);
	if ((sr & CAN_SJA1000_SR_TCS) == 0) {
		status = -EIO;
	}

	can_sja1000_tx_done(dev, status);
}

static void can_sja1000_handle_error_warning_irq(const struct device *dev)
{
	struct can_sja1000_data *data = dev->data;
	uint8_t sr;
	int err;

	sr = can_sja1000_read_reg(dev, CAN_SJA1000_SR);
	if ((sr & CAN_SJA1000_SR_BS) != 0) {
		data->state = CAN_STATE_BUS_OFF;
		can_sja1000_tx_done(dev, -ENETUNREACH);
#ifdef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
		if (data->started) {
			(void)can_sja1000_leave_reset_mode(dev);
		}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
	} else if ((sr & CAN_SJA1000_SR_ES) != 0) {
		data->state = CAN_STATE_ERROR_WARNING;
	} else {
		data->state = CAN_STATE_ERROR_ACTIVE;
	}
}

static void can_sja1000_handle_error_passive_irq(const struct device *dev)
{
	struct can_sja1000_data *data = dev->data;

	if (data->state == CAN_STATE_ERROR_PASSIVE) {
		data->state = CAN_STATE_ERROR_WARNING;
	} else {
		data->state = CAN_STATE_ERROR_PASSIVE;
	}
}

void can_sja1000_isr(const struct device *dev)
{
	struct can_sja1000_data *data = dev->data;
	const can_state_change_callback_t cb = data->state_change_cb;
	void *cb_data = data->state_change_cb_data;
	enum can_state prev_state = data->state;
	struct can_bus_err_cnt err_cnt;
	uint8_t ir;

	ir = can_sja1000_read_reg(dev, CAN_SJA1000_IR);

	if ((ir & CAN_SJA1000_IR_TI) != 0) {
		can_sja1000_handle_transmit_irq(dev);
	}

	if ((ir & CAN_SJA1000_IR_RI) != 0) {
		can_sja1000_handle_receive_irq(dev);
	}

	if ((ir & CAN_SJA1000_IR_EI) != 0) {
		can_sja1000_handle_error_warning_irq(dev);
	}

	if ((ir & CAN_SJA1000_IR_EPI) != 0) {
		can_sja1000_handle_error_passive_irq(dev);
	}

	if (prev_state != data->state && cb != NULL) {
		err_cnt.rx_err_cnt = can_sja1000_read_reg(dev, CAN_SJA1000_RXERR);
		err_cnt.tx_err_cnt = can_sja1000_read_reg(dev, CAN_SJA1000_TXERR);
		cb(dev, data->state, err_cnt, cb_data);
	}
}

int can_sja1000_init(const struct device *dev)
{
	const struct can_sja1000_config *config = dev->config;
	struct can_sja1000_data *data = dev->data;
	struct can_timing timing;
	int err;

	__ASSERT_NO_MSG(config->read_reg != NULL);
	__ASSERT_NO_MSG(config->write_reg != NULL);

	if (config->phy != NULL) {
		if (!device_is_ready(config->phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	k_mutex_init(&data->mod_lock);
	k_sem_init(&data->tx_idle, 1, 1);

	data->state = CAN_STATE_ERROR_ACTIVE;

	/* See NXP SJA1000 Application Note AN97076 (figure 12) for initialization sequence */

	/* Enter reset mode */
	err = can_sja1000_enter_reset_mode(dev);
	if (err != 0) {
		return err;
	}

	/* Set PeliCAN mode */
	can_sja1000_write_reg(dev, CAN_SJA1000_CDR, config->cdr | CAN_SJA1000_CDR_CAN_MODE);

	/* Set up acceptance code and mask to match any frame (software filtering) */
	can_sja1000_write_reg(dev, CAN_SJA1000_ACR0, 0x00);
	can_sja1000_write_reg(dev, CAN_SJA1000_ACR1, 0x00);
	can_sja1000_write_reg(dev, CAN_SJA1000_ACR2, 0x00);
	can_sja1000_write_reg(dev, CAN_SJA1000_ACR3, 0x00);

	can_sja1000_write_reg(dev, CAN_SJA1000_AMR0, 0xFF);
	can_sja1000_write_reg(dev, CAN_SJA1000_AMR1, 0xFF);
	can_sja1000_write_reg(dev, CAN_SJA1000_AMR2, 0xFF);
	can_sja1000_write_reg(dev, CAN_SJA1000_AMR3, 0xFF);

	/* Calculate initial timing parameters */
	data->sjw = config->sjw;
	timing.sjw = CAN_SJW_NO_CHANGE;

	if (config->sample_point != 0) {
		err = can_calc_timing(dev, &timing, config->bitrate, config->sample_point);
		if (err == -EINVAL) {
			LOG_ERR("bitrate/sample point cannot be met (err %d)", err);
			return err;
		}

		LOG_DBG("initial sample point error: %d", err);
	} else {
		timing.prop_seg = 0;
		timing.phase_seg1 = config->phase_seg1;
		timing.phase_seg2 = config->phase_seg2;

		err = can_calc_prescaler(dev, &timing, config->bitrate);
		if (err != 0) {
			LOG_WRN("initial bitrate error: %d", err);
		}
	}

	/* Configure timing */
	err = can_sja1000_set_timing(dev, &timing);
	if (err != 0) {
		LOG_ERR("timing parameters cannot be met (err %d)", err);
		return err;
	}

	/* Set output control */
	can_sja1000_write_reg(dev, CAN_SJA1000_OCR, config->ocr);

	/* Clear error counters and error capture */
	can_sja1000_clear_errors(dev);

	/* Set error warning limit */
	can_sja1000_write_reg(dev, CAN_SJA1000_EWLR, 96);

	/* Set normal mode */
	data->mode = CAN_MODE_NORMAL;
	err = can_sja1000_set_mode(dev, CAN_MODE_NORMAL);
	if (err != 0) {
		return err;
	}

	/* Enable interrupts */
	can_sja1000_write_reg(dev, CAN_SJA1000_IER,
			      CAN_SJA1000_IER_RIE | CAN_SJA1000_IER_TIE |
			      CAN_SJA1000_IER_EIE | CAN_SJA1000_IER_EPIE);

	return 0;
}
