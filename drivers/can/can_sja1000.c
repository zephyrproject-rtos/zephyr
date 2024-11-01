/*
 * Copyright (c) 2022 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can/can_sja1000.h>
#include "can_sja1000_priv.h"

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

	config->write_reg(dev, reg, val);
}

static inline uint8_t can_sja1000_read_reg(const struct device *dev, uint8_t reg)
{
	const struct can_sja1000_config *config = dev->config;

	return config->read_reg(dev, reg);
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

static inline void can_sja1000_leave_reset_mode_nowait(const struct device *dev)
{
	uint8_t mod;

	mod = can_sja1000_read_reg(dev, CAN_SJA1000_MOD);
	can_sja1000_write_reg(dev, CAN_SJA1000_MOD, mod & ~(CAN_SJA1000_MOD_RM));
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

	if (data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->mod_lock, K_FOREVER);

	btr0 = CAN_SJA1000_BTR0_BRP_PREP(timing->prescaler - 1) |
	       CAN_SJA1000_BTR0_SJW_PREP(timing->sjw - 1);
	btr1 = CAN_SJA1000_BTR1_TSEG1_PREP(timing->phase_seg1 - 1) |
	       CAN_SJA1000_BTR1_TSEG2_PREP(timing->phase_seg2 - 1);

	if ((data->common.mode & CAN_MODE_3_SAMPLES) != 0) {
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

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		*cap |= CAN_MODE_MANUAL_RECOVERY;
	}

	return 0;
}

int can_sja1000_start(const struct device *dev)
{
	const struct can_sja1000_config *config = dev->config;
	struct can_sja1000_data *data = dev->data;
	int err;

	if (data->common.started) {
		return -EALREADY;
	}

	if (config->common.phy != NULL) {
		err = can_transceiver_enable(config->common.phy, data->common.mode);
		if (err != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", err);
			return err;
		}
	}

	can_sja1000_clear_errors(dev);
	CAN_STATS_RESET(dev);

	err = can_sja1000_leave_reset_mode(dev);
	if (err != 0) {
		if (config->common.phy != NULL) {
			/* Attempt to disable the CAN transceiver in case of error */
			(void)can_transceiver_disable(config->common.phy);
		}

		return err;
	}

	data->common.started = true;

	return 0;
}

int can_sja1000_stop(const struct device *dev)
{
	const struct can_sja1000_config *config = dev->config;
	struct can_sja1000_data *data = dev->data;
	int err;

	if (!data->common.started) {
		return -EALREADY;
	}

	/* Entering reset mode aborts current transmission, if any */
	err = can_sja1000_enter_reset_mode(dev);
	if (err != 0) {
		return err;
	}

	if (config->common.phy != NULL) {
		err = can_transceiver_disable(config->common.phy);
		if (err != 0) {
			LOG_ERR("failed to disable CAN transceiver (err %d)", err);
			return err;
		}
	}

	data->common.started = false;

	can_sja1000_tx_done(dev, -ENETDOWN);

	return 0;
}

int can_sja1000_set_mode(const struct device *dev, can_mode_t mode)
{
	can_mode_t supported = CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT |
		CAN_MODE_3_SAMPLES;
	struct can_sja1000_data *data = dev->data;
	uint8_t btr1;
	uint8_t mod;

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		supported |= CAN_MODE_MANUAL_RECOVERY;
	}

	if ((mode & ~(supported)) != 0) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	if (data->common.started) {
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

	data->common.mode = mode;

	k_mutex_unlock(&data->mod_lock);

	return 0;
}

static void can_sja1000_read_frame(const struct device *dev, struct can_frame *frame)
{
	uint32_t id;
	uint8_t info;
	int i;

	memset(frame, 0, sizeof(*frame));

	info = can_sja1000_read_reg(dev, CAN_SJA1000_FRAME_INFO);

	if ((info & CAN_SJA1000_FRAME_INFO_RTR) != 0) {
		frame->flags |= CAN_FRAME_RTR;
	}

	frame->dlc = CAN_SJA1000_FRAME_INFO_DLC_GET(info);
	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("RX frame DLC %u exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return;
	}

	if ((info & CAN_SJA1000_FRAME_INFO_FF) != 0) {
		frame->flags |= CAN_FRAME_IDE;

		id = FIELD_PREP(GENMASK(28, 21),
				can_sja1000_read_reg(dev, CAN_SJA1000_XFF_ID1));
		id |= FIELD_PREP(GENMASK(20, 13),
				 can_sja1000_read_reg(dev, CAN_SJA1000_XFF_ID2));
		id |= FIELD_PREP(GENMASK(12, 5),
				 can_sja1000_read_reg(dev, CAN_SJA1000_EFF_ID3));
		id |= FIELD_PREP(GENMASK(4, 0),
				 can_sja1000_read_reg(dev, CAN_SJA1000_EFF_ID4) >> 3);
		frame->id = id;

		if ((frame->flags & CAN_FRAME_RTR) == 0U) {
			for (i = 0; i < frame->dlc; i++) {
				frame->data[i] = can_sja1000_read_reg(dev, CAN_SJA1000_EFF_DATA +
								      i);
			}
		}
	} else {
		id = FIELD_PREP(GENMASK(10, 3),
				can_sja1000_read_reg(dev, CAN_SJA1000_XFF_ID1));
		id |= FIELD_PREP(GENMASK(2, 0),
				 can_sja1000_read_reg(dev, CAN_SJA1000_XFF_ID2) >> 5);
		frame->id = id;

		if ((frame->flags & CAN_FRAME_RTR) == 0U) {
			for (i = 0; i < frame->dlc; i++) {
				frame->data[i] = can_sja1000_read_reg(dev, CAN_SJA1000_SFF_DATA +
								      i);
			}
		}
	}
}

void can_sja1000_write_frame(const struct device *dev, const struct can_frame *frame)
{
	uint32_t id;
	uint8_t info;
	int i;

	info = CAN_SJA1000_FRAME_INFO_DLC_PREP(frame->dlc);

	if ((frame->flags & CAN_FRAME_RTR) != 0) {
		info |= CAN_SJA1000_FRAME_INFO_RTR;
	}

	if ((frame->flags & CAN_FRAME_IDE) != 0) {
		info |= CAN_SJA1000_FRAME_INFO_FF;
	}

	can_sja1000_write_reg(dev, CAN_SJA1000_FRAME_INFO, info);

	if ((frame->flags & CAN_FRAME_IDE) != 0) {
		id = frame->id;
		can_sja1000_write_reg(dev, CAN_SJA1000_XFF_ID1,
				      FIELD_GET(GENMASK(28, 21), id));
		can_sja1000_write_reg(dev, CAN_SJA1000_XFF_ID2,
				      FIELD_GET(GENMASK(20, 13), id));
		can_sja1000_write_reg(dev, CAN_SJA1000_EFF_ID3,
				      FIELD_GET(GENMASK(12, 5), id));
		can_sja1000_write_reg(dev, CAN_SJA1000_EFF_ID4,
				      FIELD_GET(GENMASK(4, 0), id) << 3);

		if ((frame->flags & CAN_FRAME_RTR) == 0U) {
			for (i = 0; i < frame->dlc; i++) {
				can_sja1000_write_reg(dev, CAN_SJA1000_EFF_DATA + i,
						      frame->data[i]);
			}
		}
	} else {
		id = frame->id;
		can_sja1000_write_reg(dev, CAN_SJA1000_XFF_ID1,
				      FIELD_GET(GENMASK(10, 3), id));
		can_sja1000_write_reg(dev, CAN_SJA1000_XFF_ID2,
				      FIELD_GET(GENMASK(2, 0), id) << 5);

		if ((frame->flags & CAN_FRAME_RTR) == 0U) {
			for (i = 0; i < frame->dlc; i++) {
				can_sja1000_write_reg(dev, CAN_SJA1000_SFF_DATA + i,
						      frame->data[i]);
			}
		}
	}
}

int can_sja1000_send(const struct device *dev, const struct can_frame *frame, k_timeout_t timeout,
		     can_tx_callback_t callback, void *user_data)
{
	struct can_sja1000_data *data = dev->data;
	uint8_t cmr;
	uint8_t sr;

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("TX frame DLC %u exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}

	if (!data->common.started) {
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

	if ((data->common.mode & CAN_MODE_LOOPBACK) != 0) {
		cmr = CAN_SJA1000_CMR_SRR;
	} else {
		cmr = CAN_SJA1000_CMR_TR;
	}

	if ((data->common.mode & CAN_MODE_ONE_SHOT) != 0) {
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

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0) {
		LOG_ERR("unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

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

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
int can_sja1000_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_sja1000_data *data = dev->data;
	int64_t start_ticks;
	uint8_t sr;
	int err;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if ((data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
		return -ENOTSUP;
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
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

int can_sja1000_get_state(const struct device *dev, enum can_state *state,
			  struct can_bus_err_cnt *err_cnt)
{
	struct can_sja1000_data *data = dev->data;

	if (state != NULL) {
		if (!data->common.started) {
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

	data->common.state_change_cb = callback;
	data->common.state_change_cb_user_data = user_data;
}

int can_sja1000_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CONFIG_CAN_MAX_FILTER;
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

#ifndef CONFIG_CAN_ACCEPT_RTR
		if ((frame.flags & CAN_FRAME_RTR) == 0U) {
#endif /* !CONFIG_CAN_ACCEPT_RTR */
			for (i = 0; i < ARRAY_SIZE(data->filters); i++) {
				if (!atomic_test_bit(data->rx_allocs, i)) {
					continue;
				}

				if (can_frame_matches_filter(&frame, &data->filters[i].filter)) {
					callback = data->filters[i].callback;
					if (callback != NULL) {
						callback(dev, &frame, data->filters[i].user_data);
					}
				}
			}
#ifndef CONFIG_CAN_ACCEPT_RTR
		}
#endif /* !CONFIG_CAN_ACCEPT_RTR */

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

#ifdef CONFIG_CAN_STATS
static void can_sja1000_handle_data_overrun_irq(const struct device *dev)
{
	/* See NXP SJA1000 Application Note AN97076 (figure 18) for data overrun details */

	CAN_STATS_RX_OVERRUN_INC(dev);

	can_sja1000_write_reg(dev, CAN_SJA1000_CMR, CAN_SJA1000_CMR_CDO);
}

static void can_sja1000_handle_bus_error_irq(const struct device *dev)
{
	/* See NXP SJA1000 Application Note AN97076 (tables 6 and 7) for ECC details */
	uint8_t ecc;

	/* Read the Error Code Capture register to re-activate it */
	ecc = can_sja1000_read_reg(dev, CAN_SJA1000_ECC);

	if (ecc == (CAN_SJA1000_ECC_ERRC_OTHER_ERROR | CAN_SJA1000_ECC_DIR_TX |
		CAN_SJA1000_ECC_SEG_ACK_SLOT)) {
		/* Missing ACK is reported as a TX "other" error in the ACK slot */
		CAN_STATS_ACK_ERROR_INC(dev);
		return;
	}

	if (ecc == (CAN_SJA1000_ECC_ERRC_FORM_ERROR | CAN_SJA1000_ECC_DIR_RX |
		CAN_SJA1000_ECC_SEG_ACK_DELIM)) {
		/* CRC error is reported as a RX "form" error in the ACK delimiter */
		CAN_STATS_CRC_ERROR_INC(dev);
		return;
	}

	switch (ecc & CAN_SJA1000_ECC_ERRC_MASK) {
	case CAN_SJA1000_ECC_ERRC_BIT_ERROR:
		CAN_STATS_BIT_ERROR_INC(dev);
		break;

	case CAN_SJA1000_ECC_ERRC_FORM_ERROR:
		CAN_STATS_FORM_ERROR_INC(dev);
		break;
	case CAN_SJA1000_ECC_ERRC_STUFF_ERROR:
		CAN_STATS_STUFF_ERROR_INC(dev);
		break;

	case CAN_SJA1000_ECC_ERRC_OTHER_ERROR:
		__fallthrough;
	default:
		/* Other error not currently reported in CAN statistics */
		break;
	}
}
#endif /* CONFIG_CAN_STATS */

static void can_sja1000_handle_error_warning_irq(const struct device *dev)
{
	struct can_sja1000_data *data = dev->data;
	uint8_t sr;

	sr = can_sja1000_read_reg(dev, CAN_SJA1000_SR);
	if ((sr & CAN_SJA1000_SR_BS) != 0) {
		data->state = CAN_STATE_BUS_OFF;
		can_sja1000_tx_done(dev, -ENETUNREACH);

		if (data->common.started &&
			(data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
			can_sja1000_leave_reset_mode_nowait(dev);
		}
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
	const can_state_change_callback_t cb = data->common.state_change_cb;
	void *cb_data = data->common.state_change_cb_user_data;
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

#ifdef CONFIG_CAN_STATS
	if ((ir & CAN_SJA1000_IR_DOI) != 0) {
		can_sja1000_handle_data_overrun_irq(dev);
	}

	if ((ir & CAN_SJA1000_IR_BEI) != 0) {
		can_sja1000_handle_bus_error_irq(dev);
	}
#endif /* CONFIG_CAN_STATS */

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
	struct can_timing timing = { 0 };
	int err;

	__ASSERT_NO_MSG(config->read_reg != NULL);
	__ASSERT_NO_MSG(config->write_reg != NULL);

	if (config->common.phy != NULL) {
		if (!device_is_ready(config->common.phy)) {
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

	err = can_calc_timing(dev, &timing, config->common.bitrate,
			      config->common.sample_point);
	if (err == -EINVAL) {
		LOG_ERR("bitrate/sample point cannot be met (err %d)", err);
		return err;
	}

	LOG_DBG("initial sample point error: %d", err);

	/* Configure timing */
	err = can_set_timing(dev, &timing);
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
	data->common.mode = CAN_MODE_NORMAL;
	err = can_sja1000_set_mode(dev, CAN_MODE_NORMAL);
	if (err != 0) {
		return err;
	}

	/* Enable interrupts */
	can_sja1000_write_reg(dev, CAN_SJA1000_IER,
#ifdef CONFIG_CAN_STATS
			      CAN_SJA1000_IER_BEIE | CAN_SJA1000_IER_DOIE |
#endif /* CONFIG_CAN_STATS */
			      CAN_SJA1000_IER_RIE | CAN_SJA1000_IER_TIE |
			      CAN_SJA1000_IER_EIE | CAN_SJA1000_IER_EPIE);

	return 0;
}
