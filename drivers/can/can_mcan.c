/*
 * Copyright (c) 2022-2023 Vestas Wind Systems A/S
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "can_mcan.h"

LOG_MODULE_REGISTER(can_mcan, CONFIG_CAN_LOG_LEVEL);

#define CAN_INIT_TIMEOUT_MS 100

int can_mcan_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct can_mcan_config *config = dev->config;
	int err;

	err = config->ops->read_reg(dev, reg, val);
	if (err != 0) {
		LOG_ERR("failed to read reg 0x%03x (err %d)", reg, err);
	} else {
		LOG_DBG("read reg 0x%03x = 0x%08x", reg, *val);
	}

	return err;
}

int can_mcan_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct can_mcan_config *config = dev->config;
	int err;

	err = config->ops->write_reg(dev, reg, val);
	if (err != 0) {
		LOG_ERR("failed to write reg 0x%03x (err %d)", reg, err);
	} else {
		LOG_DBG("write reg 0x%03x = 0x%08x", reg, val);
	}

	return err;
}

static int can_mcan_exit_sleep_mode(const struct device *dev)
{
	struct can_mcan_data *data = dev->data;
	uint32_t start_time;
	uint32_t cccr;
	int err;

	k_mutex_lock(&data->lock, K_FOREVER);

	err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
	if (err != 0) {
		goto unlock;
	}

	cccr &= ~CAN_MCAN_CCCR_CSR;

	err = can_mcan_write_reg(dev, CAN_MCAN_CCCR, cccr);
	if (err != 0) {
		goto unlock;
	}

	start_time = k_cycle_get_32();

	err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
	if (err != 0) {
		goto unlock;
	}

	while ((cccr & CAN_MCAN_CCCR_CSA) == CAN_MCAN_CCCR_CSA) {
		if (k_cycle_get_32() - start_time > k_ms_to_cyc_ceil32(CAN_INIT_TIMEOUT_MS)) {
			cccr |= CAN_MCAN_CCCR_CSR;
			err = can_mcan_write_reg(dev, CAN_MCAN_CCCR, cccr);
			if (err != 0) {
				goto unlock;
			}

			err = -EAGAIN;
			goto unlock;
		}

		err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
		if (err != 0) {
			goto unlock;
		}
	}


unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int can_mcan_enter_init_mode(const struct device *dev, k_timeout_t timeout)
{
	struct can_mcan_data *data = dev->data;
	int64_t start_time;
	uint32_t cccr;
	int err;

	k_mutex_lock(&data->lock, K_FOREVER);

	err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
	if (err != 0) {
		goto unlock;
	}

	cccr |= CAN_MCAN_CCCR_INIT;

	err = can_mcan_write_reg(dev, CAN_MCAN_CCCR, cccr);
	if (err != 0) {
		goto unlock;
	}

	start_time = k_uptime_ticks();

	err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
	if (err != 0) {
		goto unlock;
	}

	while ((cccr & CAN_MCAN_CCCR_INIT) == 0U) {
		if (k_uptime_ticks() - start_time > timeout.ticks) {
			cccr &= ~CAN_MCAN_CCCR_INIT;
			err = can_mcan_write_reg(dev, CAN_MCAN_CCCR, cccr);
			if (err != 0) {
				goto unlock;
			}

			err = -EAGAIN;
			goto unlock;
		}

		err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
		if (err != 0) {
			goto unlock;
		}
	}

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int can_mcan_leave_init_mode(const struct device *dev, k_timeout_t timeout)
{
	struct can_mcan_data *data = dev->data;
	int64_t start_time;
	uint32_t cccr;
	int err;

	k_mutex_lock(&data->lock, K_FOREVER);

	err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
	if (err != 0) {
		goto unlock;
	}

	cccr &= ~CAN_MCAN_CCCR_INIT;

	err = can_mcan_write_reg(dev, CAN_MCAN_CCCR, cccr);
	if (err != 0) {
		goto unlock;
	}

	start_time = k_uptime_ticks();

	err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
	if (err != 0) {
		goto unlock;
	}

	while ((cccr & CAN_MCAN_CCCR_INIT) != 0U) {
		if (k_uptime_ticks() - start_time > timeout.ticks) {
			err = -EAGAIN;
			goto unlock;
		}

		err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
		if (err != 0) {
			goto unlock;
		}
	}

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

int can_mcan_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_mcan_data *data = dev->data;
	uint32_t nbtp = 0U;
	int err;

	if (data->started) {
		return -EBUSY;
	}

	__ASSERT_NO_MSG(timing->prop_seg == 0U);
	__ASSERT_NO_MSG(timing->phase_seg1 <= 0x100 && timing->phase_seg1 > 0U);
	__ASSERT_NO_MSG(timing->phase_seg2 <= 0x80 && timing->phase_seg2 > 0U);
	__ASSERT_NO_MSG(timing->prescaler <= 0x200 && timing->prescaler > 0U);
	__ASSERT_NO_MSG(timing->sjw == CAN_SJW_NO_CHANGE ||
			(timing->sjw <= 0x80 && timing->sjw > 0U));

	k_mutex_lock(&data->lock, K_FOREVER);

	if (timing->sjw == CAN_SJW_NO_CHANGE) {
		err = can_mcan_read_reg(dev, CAN_MCAN_NBTP, &nbtp);
		if (err != 0) {
			goto unlock;
		}

		nbtp &= CAN_MCAN_NBTP_NSJW;
	} else {
		nbtp |= FIELD_PREP(CAN_MCAN_NBTP_NSJW, timing->sjw - 1UL);
	}

	nbtp |= FIELD_PREP(CAN_MCAN_NBTP_NTSEG1, timing->phase_seg1 - 1UL) |
		FIELD_PREP(CAN_MCAN_NBTP_NTSEG2, timing->phase_seg2 - 1UL) |
		FIELD_PREP(CAN_MCAN_NBTP_NBRP, timing->prescaler - 1UL);

	err = can_mcan_write_reg(dev, CAN_MCAN_NBTP, nbtp);
	if (err != 0) {
		goto unlock;
	}

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

#ifdef CONFIG_CAN_FD_MODE
int can_mcan_set_timing_data(const struct device *dev, const struct can_timing *timing_data)
{
	struct can_mcan_data *data = dev->data;
	uint32_t dbtp = 0U;
	int err;

	if (data->started) {
		return -EBUSY;
	}

	__ASSERT_NO_MSG(timing_data->prop_seg == 0U);
	__ASSERT_NO_MSG(timing_data->phase_seg1 <= 0x20 && timing_data->phase_seg1 > 0U);
	__ASSERT_NO_MSG(timing_data->phase_seg2 <= 0x10 && timing_data->phase_seg2 > 0U);
	__ASSERT_NO_MSG(timing_data->prescaler <= 0x20 && timing_data->prescaler > 0U);
	__ASSERT_NO_MSG(timing_data->sjw == CAN_SJW_NO_CHANGE ||
			(timing_data->sjw <= 0x80 && timing_data->sjw > 0U));

	k_mutex_lock(&data->lock, K_FOREVER);

	if (timing_data->sjw == CAN_SJW_NO_CHANGE) {
		err = can_mcan_read_reg(dev, CAN_MCAN_DBTP, &dbtp);
		if (err != 0) {
			goto unlock;
		}

		dbtp &= CAN_MCAN_DBTP_DSJW;
	} else {
		dbtp |= FIELD_PREP(CAN_MCAN_DBTP_DSJW, timing_data->sjw - 1UL);
	}

	dbtp |= FIELD_PREP(CAN_MCAN_DBTP_DTSEG1, timing_data->phase_seg1 - 1UL) |
		FIELD_PREP(CAN_MCAN_DBTP_DTSEG2, timing_data->phase_seg2 - 1UL) |
		FIELD_PREP(CAN_MCAN_DBTP_DBRP, timing_data->prescaler - 1UL);

	err = can_mcan_write_reg(dev, CAN_MCAN_DBTP, dbtp);
	if (err != 0) {
		goto unlock;
	}

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}
#endif /* CONFIG_CAN_FD_MODE */

int can_mcan_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY;

#if CONFIG_CAN_FD_MODE
	*cap |= CAN_MODE_FD;
#endif /* CONFIG_CAN_FD_MODE */

	return 0;
}

int can_mcan_start(const struct device *dev)
{
	const struct can_mcan_config *config = dev->config;
	struct can_mcan_data *data = dev->data;
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

	err = can_mcan_leave_init_mode(dev, K_MSEC(CAN_INIT_TIMEOUT_MS));
	if (err != 0) {
		LOG_ERR("failed to leave init mode");

		if (config->phy != NULL) {
			/* Attempt to disable the CAN transceiver in case of error */
			(void)can_transceiver_disable(config->phy);
		}

		return -EIO;
	}

	data->started = true;

	return 0;
}

int can_mcan_stop(const struct device *dev)
{
	const struct can_mcan_config *config = dev->config;
	struct can_mcan_data *data = dev->data;
	can_tx_callback_t tx_cb;
	uint32_t tx_idx;
	int err;

	if (!data->started) {
		return -EALREADY;
	}

	/* CAN transmissions are automatically stopped when entering init mode */
	err = can_mcan_enter_init_mode(dev, K_MSEC(CAN_INIT_TIMEOUT_MS));
	if (err != 0) {
		LOG_ERR("Failed to enter init mode");
		return -EIO;
	}

	if (config->phy != NULL) {
		err = can_transceiver_disable(config->phy);
		if (err != 0) {
			LOG_ERR("failed to disable CAN transceiver (err %d)", err);
			return err;
		}
	}

	can_mcan_enable_configuration_change(dev);

	data->started = false;

	for (tx_idx = 0U; tx_idx < ARRAY_SIZE(data->tx_fin_cb); tx_idx++) {
		tx_cb = data->tx_fin_cb[tx_idx];

		if (tx_cb != NULL) {
			data->tx_fin_cb[tx_idx] = NULL;
			tx_cb(dev, -ENETDOWN, data->tx_fin_cb_arg[tx_idx]);
			k_sem_give(&data->tx_sem);
		}
	}

	return 0;
}

int can_mcan_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_mcan_data *data = dev->data;
	uint32_t cccr;
	uint32_t test;
	int err;

#ifdef CONFIG_CAN_FD_MODE
	if ((mode & ~(CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_FD)) != 0U) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}
#else  /* CONFIG_CAN_FD_MODE */
	if ((mode & ~(CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY)) != 0U) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}
#endif /* !CONFIG_CAN_FD_MODE */

	if (data->started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
	if (err != 0) {
		goto unlock;
	}

	err = can_mcan_read_reg(dev, CAN_MCAN_TEST, &test);
	if (err != 0) {
		goto unlock;
	}

	if ((mode & CAN_MODE_LOOPBACK) != 0) {
		/* Loopback mode */
		cccr |= CAN_MCAN_CCCR_TEST;
		test |= CAN_MCAN_TEST_LBCK;
	} else {
		cccr &= ~CAN_MCAN_CCCR_TEST;
	}

	if ((mode & CAN_MODE_LISTENONLY) != 0) {
		/* Bus monitoring mode */
		cccr |= CAN_MCAN_CCCR_MON;
	} else {
		cccr &= ~CAN_MCAN_CCCR_MON;
	}

#ifdef CONFIG_CAN_FD_MODE
	if ((mode & CAN_MODE_FD) != 0) {
		cccr |= CAN_MCAN_CCCR_FDOE | CAN_MCAN_CCCR_BRSE;
		data->fd = true;
	} else {
		cccr &= ~(CAN_MCAN_CCCR_FDOE | CAN_MCAN_CCCR_BRSE);
		data->fd = false;
	}
#endif /* CONFIG_CAN_FD_MODE */

	err = can_mcan_write_reg(dev, CAN_MCAN_CCCR, cccr);
	if (err != 0) {
		goto unlock;
	}

	err = can_mcan_write_reg(dev, CAN_MCAN_TEST, test);
	if (err != 0) {
		goto unlock;
	}

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static void can_mcan_state_change_handler(const struct device *dev)
{
	struct can_mcan_data *data = dev->data;
	const can_state_change_callback_t cb = data->state_change_cb;
	void *cb_data = data->state_change_cb_data;
	struct can_bus_err_cnt err_cnt;
	enum can_state state;

	(void)can_mcan_get_state(dev, &state, &err_cnt);

	if (cb != NULL) {
		cb(dev, state, err_cnt, cb_data);
	}
}

static void can_mcan_tc_event_handler(const struct device *dev)
{
	struct can_mcan_data *data = dev->data;
	struct can_mcan_tx_event_fifo tx_event;
	can_tx_callback_t tx_cb;
	uint32_t event_idx;
	uint32_t tx_idx;
	uint32_t txefs;
	int err;

	err = can_mcan_read_reg(dev, CAN_MCAN_TXEFS, &txefs);
	if (err != 0) {
		return;
	}

	while ((txefs & CAN_MCAN_TXEFS_EFFL) != 0U) {
		event_idx = FIELD_GET(CAN_MCAN_TXEFS_EFGI, txefs);
		err = can_mcan_read_mram(dev, CAN_MCAN_MRAM_OFFSET_TX_EVENT_FIFO +
					 event_idx * sizeof(struct can_mcan_tx_event_fifo),
					 &tx_event,
					 sizeof(struct can_mcan_tx_event_fifo));
		if (err != 0) {
			LOG_ERR("failed to read tx event fifo (err %d)", err);
			return;
		}

		tx_idx = tx_event.mm;

		/* Acknowledge TX event */
		err = can_mcan_write_reg(dev, CAN_MCAN_TXEFA, event_idx);
		if (err != 0) {
			return;
		}

		k_sem_give(&data->tx_sem);

		tx_cb = data->tx_fin_cb[tx_idx];
		data->tx_fin_cb[tx_idx] = NULL;
		tx_cb(dev, 0, data->tx_fin_cb_arg[tx_idx]);

		err = can_mcan_read_reg(dev, CAN_MCAN_TXEFS, &txefs);
		if (err != 0) {
			return;
		}
	}
}

void can_mcan_line_0_isr(const struct device *dev)
{
	const uint32_t events = CAN_MCAN_IR_BO | CAN_MCAN_IR_EP | CAN_MCAN_IR_EW |
				CAN_MCAN_IR_TEFN | CAN_MCAN_IR_TEFL | CAN_MCAN_IR_ARA |
				CAN_MCAN_IR_MRAF;
	struct can_mcan_data *data = dev->data;
	uint32_t ir;
	int err;

	err = can_mcan_read_reg(dev, CAN_MCAN_IR, &ir);
	if (err != 0) {
		return;
	}

	while ((ir & events) != 0U) {
		if ((ir & (CAN_MCAN_IR_BO | CAN_MCAN_IR_EP | CAN_MCAN_IR_EW)) != 0U) {
			can_mcan_state_change_handler(dev);
		}

		/* TX event FIFO new entry */
		if ((ir & CAN_MCAN_IR_TEFN) != 0U) {
			can_mcan_tc_event_handler(dev);
		}

		if ((ir & CAN_MCAN_IR_TEFL) != 0U) {
			LOG_ERR("TX FIFO element lost");
			k_sem_give(&data->tx_sem);
		}

		if ((ir & CAN_MCAN_IR_ARA) != 0U) {
			LOG_ERR("Access to reserved address");
		}

		if (ir & CAN_MCAN_IR_MRAF) {
			LOG_ERR("Message RAM access failure");
		}

		err = can_mcan_write_reg(dev, CAN_MCAN_IR, events);
		if (err != 0) {
			return;
		}

		err = can_mcan_read_reg(dev, CAN_MCAN_IR, &ir);
		if (err != 0) {
			return;
		}
	}
}

static void can_mcan_get_message(const struct device *dev, uint16_t fifo_offset,
				 uint16_t fifo_status_reg, uint16_t fifo_ack_reg)
{
	struct can_mcan_data *data = dev->data;
	uint32_t get_idx, filt_idx;
	struct can_frame frame = {0};
	can_rx_callback_t cb;
	int data_length;
	void *cb_arg;
	struct can_mcan_rx_fifo_hdr hdr;
	bool rtr_filter_mask;
	bool rtr_filter;
	bool fd_frame_filter;
	uint32_t fifo_status;
	int err;

	err = can_mcan_read_reg(dev, fifo_status_reg, &fifo_status);
	if (err != 0) {
		return;
	}

	while (FIELD_GET(CAN_MCAN_RXF0S_F0FL, fifo_status) != 0U) {
		get_idx = FIELD_GET(CAN_MCAN_RXF0S_F0GI, fifo_status);

		err = can_mcan_read_mram(dev, fifo_offset + get_idx *
					 sizeof(struct can_mcan_rx_fifo) +
					 offsetof(struct can_mcan_rx_fifo, hdr),
					 &hdr, sizeof(struct can_mcan_rx_fifo_hdr));
		if (err != 0) {
			LOG_ERR("failed to read Rx FIFO header (err %d)", err);
			return;
		}

		frame.dlc = hdr.dlc;

		if (hdr.rtr != 0) {
			frame.flags |= CAN_FRAME_RTR;
		}

		if (hdr.fdf != 0) {
			frame.flags |= CAN_FRAME_FDF;
		}

		if (hdr.brs != 0) {
			frame.flags |= CAN_FRAME_BRS;
		}

		if (hdr.esi != 0) {
			frame.flags |= CAN_FRAME_ESI;
		}

#ifdef CONFIG_CAN_RX_TIMESTAMP
		frame.timestamp = hdr.rxts;
#endif /* CONFIG_CAN_RX_TIMESTAMP */

		filt_idx = hdr.fidx;

		if (hdr.xtd != 0) {
			frame.id = hdr.ext_id;
			frame.flags |= CAN_FRAME_IDE;
			rtr_filter_mask = (data->ext_filt_rtr_mask & BIT(filt_idx)) != 0U;
			rtr_filter = (data->ext_filt_rtr & BIT(filt_idx)) != 0;
			fd_frame_filter = (data->ext_filt_fd_frame & BIT(filt_idx)) != 0U;
		} else {
			frame.id = hdr.std_id;
			rtr_filter_mask = (data->std_filt_rtr_mask & BIT(filt_idx)) != 0U;
			rtr_filter = (data->std_filt_rtr & BIT(filt_idx)) != 0;
			fd_frame_filter = (data->std_filt_fd_frame & BIT(filt_idx)) != 0U;
		}

		if (rtr_filter_mask && (rtr_filter != ((frame.flags & CAN_FRAME_RTR) != 0U))) {
			/* RTR bit does not match filter RTR mask, drop frame */
			err = can_mcan_write_reg(dev, fifo_ack_reg, get_idx);
			if (err != 0) {
				return;
			}
			goto ack;
		} else if (fd_frame_filter != ((frame.flags & CAN_FRAME_FDF) != 0U)) {
			/* FD bit does not match filter FD frame, drop frame */
			err = can_mcan_write_reg(dev, fifo_ack_reg, get_idx);
			if (err != 0) {
				return;
			}
			goto ack;
		}

		data_length = can_dlc_to_bytes(frame.dlc);
		if (data_length <= sizeof(frame.data)) {
			err = can_mcan_read_mram(dev, fifo_offset + get_idx *
						 sizeof(struct can_mcan_rx_fifo) +
						 offsetof(struct can_mcan_rx_fifo, data_32),
						 &frame.data_32,
						 ROUND_UP(data_length, sizeof(uint32_t)));
			if (err != 0) {
				LOG_ERR("failed to read Rx FIFO data (err %d)", err);
				return;
			}

			if ((frame.flags & CAN_FRAME_IDE) != 0) {
				LOG_DBG("Frame on filter %d, ID: 0x%x",
					filt_idx + NUM_STD_FILTER_DATA, frame.id);
				cb = data->rx_cb_ext[filt_idx];
				cb_arg = data->cb_arg_ext[filt_idx];
			} else {
				LOG_DBG("Frame on filter %d, ID: 0x%x", filt_idx, frame.id);
				cb = data->rx_cb_std[filt_idx];
				cb_arg = data->cb_arg_std[filt_idx];
			}

			if (cb) {
				cb(dev, &frame, cb_arg);
			} else {
				LOG_DBG("cb missing");
			}
		} else {
			LOG_ERR("Frame is too big");
		}

ack:
		err = can_mcan_write_reg(dev, fifo_ack_reg, get_idx);
		if (err != 0) {
			return;
		}

		err = can_mcan_read_reg(dev, fifo_status_reg, &fifo_status);
		if (err != 0) {
			return;
		}
	}
}

void can_mcan_line_1_isr(const struct device *dev)
{
	const uint32_t events =
		CAN_MCAN_IR_RF0N | CAN_MCAN_IR_RF1N | CAN_MCAN_IR_RF0L | CAN_MCAN_IR_RF1L;
	uint32_t ir;
	int err;

	err = can_mcan_read_reg(dev, CAN_MCAN_IR, &ir);
	if (err != 0) {
		return;
	}

	while ((ir & events) != 0U) {
		if ((ir & CAN_MCAN_IR_RF0N) != 0U) {
			LOG_DBG("RX FIFO0 INT");
			can_mcan_get_message(dev, CAN_MCAN_MRAM_OFFSET_RX_FIFO0, CAN_MCAN_RXF0S,
					     CAN_MCAN_RXF0A);
		}

		if ((ir & CAN_MCAN_IR_RF1N) != 0U) {
			LOG_DBG("RX FIFO1 INT");
			can_mcan_get_message(dev, CAN_MCAN_MRAM_OFFSET_RX_FIFO1, CAN_MCAN_RXF1S,
					     CAN_MCAN_RXF1A);
		}

		if ((ir & CAN_MCAN_IR_RF0L) != 0U) {
			LOG_ERR("Message lost on FIFO0");
		}

		if ((ir & CAN_MCAN_IR_RF1L) != 0U) {
			LOG_ERR("Message lost on FIFO1");
		}

		err = can_mcan_write_reg(dev, CAN_MCAN_IR, events);
		if (err != 0) {
			return;
		}

		err = can_mcan_read_reg(dev, CAN_MCAN_IR, &ir);
		if (err != 0) {
			return;
		}
	}
}

int can_mcan_get_state(const struct device *dev, enum can_state *state,
		       struct can_bus_err_cnt *err_cnt)
{
	struct can_mcan_data *data = dev->data;
	uint32_t reg;
	int err;

	if (state != NULL) {
		err = can_mcan_read_reg(dev, CAN_MCAN_PSR, &reg);
		if (err != 0) {
			return err;
		}

		if (!data->started) {
			*state = CAN_STATE_STOPPED;
		} else if ((reg & CAN_MCAN_PSR_BO) != 0U) {
			*state = CAN_STATE_BUS_OFF;
		} else if ((reg & CAN_MCAN_PSR_EP) != 0U) {
			*state = CAN_STATE_ERROR_PASSIVE;
		} else if ((reg & CAN_MCAN_PSR_EW) != 0U) {
			*state = CAN_STATE_ERROR_WARNING;
		} else {
			*state = CAN_STATE_ERROR_ACTIVE;
		}
	}

	if (err_cnt != NULL) {
		err = can_mcan_read_reg(dev, CAN_MCAN_ECR, &reg);
		if (err != 0) {
			return err;
		}

		err_cnt->tx_err_cnt = FIELD_GET(CAN_MCAN_ECR_TEC, reg);
		err_cnt->rx_err_cnt = FIELD_GET(CAN_MCAN_ECR_REC, reg);
	}

	return 0;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
int can_mcan_recover(const struct device *dev, k_timeout_t timeout)
{
	struct can_mcan_data *data = dev->data;

	if (!data->started) {
		return -ENETDOWN;
	}

	return can_mcan_leave_init_mode(dev, timeout);
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

int can_mcan_send(const struct device *dev, const struct can_frame *frame, k_timeout_t timeout,
		  can_tx_callback_t callback, void *user_data)
{
	struct can_mcan_data *data = dev->data;
	size_t data_length = can_dlc_to_bytes(frame->dlc);
	struct can_mcan_tx_buffer_hdr tx_hdr = {
		.rtr = (frame->flags & CAN_FRAME_RTR) != 0U ? 1U : 0U,
		.xtd = (frame->flags & CAN_FRAME_IDE) != 0U ? 1U : 0U,
		.esi = 0U,
		.dlc = frame->dlc,
#ifdef CONFIG_CAN_FD_MODE
		.fdf = (frame->flags & CAN_FRAME_FDF) != 0U ? 1U : 0U,
		.brs = (frame->flags & CAN_FRAME_BRS) != 0U ? 1U : 0U,
#else  /* CONFIG_CAN_FD_MODE */
		.fdf = 0U,
		.brs = 0U,
#endif /* !CONFIG_CAN_FD_MODE */
		.efc = 1U,
	};
	uint32_t put_idx;
	uint32_t reg;
	int err;

	LOG_DBG("Sending %d bytes. Id: 0x%x, ID type: %s %s %s %s", data_length, frame->id,
		(frame->flags & CAN_FRAME_IDE) != 0U ? "extended" : "standard",
		(frame->flags & CAN_FRAME_RTR) != 0U ? "RTR" : "",
		(frame->flags & CAN_FRAME_FDF) != 0U ? "FD frame" : "",
		(frame->flags & CAN_FRAME_BRS) != 0U ? "BRS" : "");

	__ASSERT_NO_MSG(callback != NULL);

#ifdef CONFIG_CAN_FD_MODE
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR | CAN_FRAME_FDF | CAN_FRAME_BRS)) !=
	    0) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}

	if (!data->fd && ((frame->flags & (CAN_FRAME_FDF | CAN_FRAME_BRS)) != 0U)) {
		LOG_ERR("CAN-FD format not supported in non-FD mode");
		return -ENOTSUP;
	}
#else  /* CONFIG_CAN_FD_MODE */
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0U) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}
#endif /* !CONFIG_CAN_FD_MODE */

	if (data_length > sizeof(frame->data)) {
		LOG_ERR("data length (%zu) > max frame data length (%zu)", data_length,
			sizeof(frame->data));
		return -EINVAL;
	}

	if ((frame->flags & CAN_FRAME_FDF) != 0U) {
		if (frame->dlc > CANFD_MAX_DLC) {
			LOG_ERR("DLC of %d for CAN-FD format frame", frame->dlc);
			return -EINVAL;
		}
	} else {
		if (frame->dlc > CAN_MAX_DLC) {
			LOG_ERR("DLC of %d for non-FD format frame", frame->dlc);
			return -EINVAL;
		}
	}

	if (!data->started) {
		return -ENETDOWN;
	}

	err = can_mcan_read_reg(dev, CAN_MCAN_PSR, &reg);
	if (err != 0) {
		return err;
	}

	if ((reg & CAN_MCAN_PSR_BO) != 0U) {
		return -ENETUNREACH;
	}

	err = k_sem_take(&data->tx_sem, timeout);
	if (err != 0) {
		return -EAGAIN;
	}

	err = can_mcan_read_reg(dev, CAN_MCAN_TXFQS, &reg);
	if (err != 0) {
		return err;
	}

	__ASSERT_NO_MSG((reg & CAN_MCAN_TXFQS_TFQF) != CAN_MCAN_TXFQS_TFQF);

	k_mutex_lock(&data->tx_mtx, K_FOREVER);

	put_idx = FIELD_GET(CAN_MCAN_TXFQS_TFQPI, reg);
	tx_hdr.mm = put_idx;

	if ((frame->flags & CAN_FRAME_IDE) != 0U) {
		tx_hdr.ext_id = frame->id;
	} else {
		tx_hdr.std_id = frame->id & CAN_STD_ID_MASK;
	}

	err = can_mcan_write_mram(dev, CAN_MCAN_MRAM_OFFSET_TX_BUFFER + put_idx *
				  sizeof(struct can_mcan_tx_buffer) +
				  offsetof(struct can_mcan_tx_buffer, hdr),
				  &tx_hdr, sizeof(struct can_mcan_tx_buffer_hdr));
	if (err != 0) {
		LOG_ERR("failed to write Tx Buffer header (err %d)", err);
		return err;
	}

	err = can_mcan_write_mram(dev, CAN_MCAN_MRAM_OFFSET_TX_BUFFER + put_idx *
				  sizeof(struct can_mcan_tx_buffer) +
				  offsetof(struct can_mcan_tx_buffer, data_32),
				  &frame->data_32, ROUND_UP(data_length, sizeof(uint32_t)));
	if (err != 0) {
		LOG_ERR("failed to write Tx Buffer data (err %d)", err);
		return err;
	}

	data->tx_fin_cb[put_idx] = callback;
	data->tx_fin_cb_arg[put_idx] = user_data;

	err = can_mcan_write_reg(dev, CAN_MCAN_TXBAR, BIT(put_idx));
	if (err != 0) {
		goto unlock;
	}

unlock:
	k_mutex_unlock(&data->tx_mtx);

	return err;
}

static int can_mcan_get_free_std(const struct device *dev)
{
	struct can_mcan_std_filter filter;
	int err;
	int i;

	for (i = 0; i < NUM_STD_FILTER_DATA; ++i) {
		err = can_mcan_read_mram(dev, CAN_MCAN_MRAM_OFFSET_STD_FILTER +
					 i * sizeof(struct can_mcan_std_filter), &filter,
					 sizeof(struct can_mcan_std_filter));
		if (err != 0) {
			LOG_ERR("failed to read std filter (err %d)", err);
			return err;
		}

		if (filter.sfce == CAN_MCAN_FCE_DISABLE) {
			return i;
		}
	}

	return -ENOSPC;
}

int can_mcan_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);

	if (ide) {
		return NUM_EXT_FILTER_DATA;
	} else {
		return NUM_STD_FILTER_DATA;
	}
}

/* Use masked configuration only for simplicity. If someone needs more than
 * 28 standard filters, dual mode needs to be implemented.
 * Dual mode gets tricky, because we can only activate both filters.
 * If one of the IDs is not used anymore, we would need to mark it as unused.
 */
int can_mcan_add_rx_filter_std(const struct device *dev, can_rx_callback_t callback,
			       void *user_data, const struct can_filter *filter)
{
	struct can_mcan_data *data = dev->data;
	struct can_mcan_std_filter filter_element = {
		.id1 = filter->id,
		.id2 = filter->mask,
		.sft = CAN_MCAN_SFT_MASKED
	};
	int filter_id;
	int err;

	k_mutex_lock(&data->lock, K_FOREVER);
	filter_id = can_mcan_get_free_std(dev);

	if (filter_id < 0) {
		LOG_WRN("No free standard id filter left");
		k_mutex_unlock(&data->lock);
		return -ENOSPC;
	}

	/* TODO proper fifo balancing */
	filter_element.sfce = filter_id & 0x01 ? CAN_MCAN_FCE_FIFO1 : CAN_MCAN_FCE_FIFO0;

	err = can_mcan_write_mram(dev, CAN_MCAN_MRAM_OFFSET_STD_FILTER +
				  filter_id * sizeof(struct can_mcan_std_filter),
				  &filter_element, sizeof(filter_element));
	if (err != 0) {
		LOG_ERR("failed to write std filter element (err %d)", err);
		return err;
	}

	k_mutex_unlock(&data->lock);

	LOG_DBG("Attached std filter at %d", filter_id);

	if ((filter->flags & CAN_FILTER_RTR) != 0U) {
		data->std_filt_rtr |= (1U << filter_id);
	} else {
		data->std_filt_rtr &= ~(1U << filter_id);
	}

	if ((filter->flags & (CAN_FILTER_DATA | CAN_FILTER_RTR)) !=
	    (CAN_FILTER_DATA | CAN_FILTER_RTR)) {
		data->std_filt_rtr_mask |= (1U << filter_id);
	} else {
		data->std_filt_rtr_mask &= ~(1U << filter_id);
	}

	if ((filter->flags & CAN_FILTER_FDF) != 0U) {
		data->std_filt_fd_frame |= (1U << filter_id);
	} else {
		data->std_filt_fd_frame &= ~(1U << filter_id);
	}

	data->rx_cb_std[filter_id] = callback;
	data->cb_arg_std[filter_id] = user_data;

	return filter_id;
}

static int can_mcan_get_free_ext(const struct device *dev)
{
	struct can_mcan_ext_filter filter;
	int err;
	int i;

	for (i = 0; i < NUM_EXT_FILTER_DATA; ++i) {
		err = can_mcan_read_mram(dev, CAN_MCAN_MRAM_OFFSET_EXT_FILTER +
					 i * sizeof(struct can_mcan_ext_filter), &filter,
					 sizeof(struct can_mcan_ext_filter));
		if (err != 0) {
			LOG_ERR("failed to read ext filter (err %d)", err);
			return err;
		}

		if (filter.efce == CAN_MCAN_FCE_DISABLE) {
			return i;
		}
	}

	return -ENOSPC;
}

static int can_mcan_add_rx_filter_ext(const struct device *dev, can_rx_callback_t callback,
				      void *user_data, const struct can_filter *filter)
{
	struct can_mcan_data *data = dev->data;
	struct can_mcan_ext_filter filter_element = {
		.id2 = filter->mask,
		.id1 = filter->id,
		.eft = CAN_MCAN_EFT_MASKED
	};
	int filter_id;
	int err;

	k_mutex_lock(&data->lock, K_FOREVER);
	filter_id = can_mcan_get_free_ext(dev);

	if (filter_id < 0) {
		LOG_WRN("No free extended id filter left");
		k_mutex_unlock(&data->lock);
		return -ENOSPC;
	}

	/* TODO proper fifo balancing */
	filter_element.efce = filter_id & 0x01 ? CAN_MCAN_FCE_FIFO1 : CAN_MCAN_FCE_FIFO0;

	err = can_mcan_write_mram(dev, CAN_MCAN_MRAM_OFFSET_EXT_FILTER +
				  filter_id * sizeof(struct can_mcan_ext_filter),
				  &filter_element, sizeof(filter_element));
	if (err != 0) {
		LOG_ERR("failed to write std filter element (err %d)", err);
		return err;
	}

	k_mutex_unlock(&data->lock);

	LOG_DBG("Attached ext filter at %d", filter_id);

	if ((filter->flags & CAN_FILTER_RTR) != 0U) {
		data->ext_filt_rtr |= (1U << filter_id);
	} else {
		data->ext_filt_rtr &= ~(1U << filter_id);
	}

	if ((filter->flags & (CAN_FILTER_DATA | CAN_FILTER_RTR)) !=
	    (CAN_FILTER_DATA | CAN_FILTER_RTR)) {
		data->ext_filt_rtr_mask |= (1U << filter_id);
	} else {
		data->ext_filt_rtr_mask &= ~(1U << filter_id);
	}

	if ((filter->flags & CAN_FILTER_FDF) != 0U) {
		data->ext_filt_fd_frame |= (1U << filter_id);
	} else {
		data->ext_filt_fd_frame &= ~(1U << filter_id);
	}

	data->rx_cb_ext[filter_id] = callback;
	data->cb_arg_ext[filter_id] = user_data;

	return filter_id;
}

int can_mcan_add_rx_filter(const struct device *dev, can_rx_callback_t callback, void *user_data,
			   const struct can_filter *filter)
{
	int filter_id;

	if (callback == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_CAN_FD_MODE
	if ((filter->flags &
	     ~(CAN_FILTER_IDE | CAN_FILTER_DATA | CAN_FILTER_RTR | CAN_FILTER_FDF)) != 0U) {
#else  /* CONFIG_CAN_FD_MODE */
	if ((filter->flags & ~(CAN_FILTER_IDE | CAN_FILTER_DATA | CAN_FILTER_RTR)) != 0U) {
#endif /* !CONFIG_CAN_FD_MODE */
		LOG_ERR("unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	if ((filter->flags & CAN_FILTER_IDE) != 0U) {
		filter_id = can_mcan_add_rx_filter_ext(dev, callback, user_data, filter);
		if (filter_id >= 0) {
			filter_id += NUM_STD_FILTER_DATA;
		}
	} else {
		filter_id = can_mcan_add_rx_filter_std(dev, callback, user_data, filter);
	}

	return filter_id;
}

void can_mcan_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_mcan_data *data = dev->data;
	int err;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (filter_id >= NUM_STD_FILTER_DATA) {
		filter_id -= NUM_STD_FILTER_DATA;
		if (filter_id >= NUM_EXT_FILTER_DATA) {
			LOG_ERR("Wrong filter id");
			k_mutex_unlock(&data->lock);
			return;
		}

		err = can_mcan_clear_mram(dev, CAN_MCAN_MRAM_OFFSET_EXT_FILTER +
					filter_id * sizeof(struct can_mcan_ext_filter),
					sizeof(struct can_mcan_ext_filter));
		if (err != 0) {
			LOG_ERR("failed to clear ext filter element (err %d)", err);
		}
	} else {
		err = can_mcan_clear_mram(dev, CAN_MCAN_MRAM_OFFSET_STD_FILTER +
					filter_id * sizeof(struct can_mcan_std_filter),
					sizeof(struct can_mcan_std_filter));
		if (err != 0) {
			LOG_ERR("failed to clear std filter element (err %d)", err);
		}
	}

	k_mutex_unlock(&data->lock);
}

void can_mcan_set_state_change_callback(const struct device *dev,
					can_state_change_callback_t callback, void *user_data)
{
	struct can_mcan_data *data = dev->data;

	data->state_change_cb = callback;
	data->state_change_cb_data = user_data;
}

int can_mcan_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate)
{
	const struct can_mcan_config *config = dev->config;

	*max_bitrate = config->max_bitrate;

	return 0;
}

/* helper function allowing mcan drivers without access to private mcan
 * definitions to set CCCR_CCE, which might be needed to disable write
 * protection for some registers.
 */
void can_mcan_enable_configuration_change(const struct device *dev)
{
	struct can_mcan_data *data = dev->data;
	uint32_t cccr;
	int err;

	k_mutex_lock(&data->lock, K_FOREVER);

	err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &cccr);
	if (err != 0) {
		goto unlock;
	}

	cccr |= CAN_MCAN_CCCR_CCE;

	err = can_mcan_write_reg(dev, CAN_MCAN_CCCR, cccr);
	if (err != 0) {
		goto unlock;
	}

unlock:
	k_mutex_unlock(&data->lock);
}

int can_mcan_configure_mram(const struct device *dev, uintptr_t mrba, uintptr_t mram)
{
	uint32_t reg;
	int err;

	err = can_mcan_exit_sleep_mode(dev);
	if (err != 0) {
		LOG_ERR("Failed to exit sleep mode");
		return -EIO;
	}

	err = can_mcan_enter_init_mode(dev, K_MSEC(CAN_INIT_TIMEOUT_MS));
	if (err != 0) {
		LOG_ERR("Failed to enter init mode");
		return -EIO;
	}

	can_mcan_enable_configuration_change(dev);

	reg = ((mram - mrba + CAN_MCAN_MRAM_OFFSET_STD_FILTER) & CAN_MCAN_SIDFC_FLSSA) |
		FIELD_PREP(CAN_MCAN_SIDFC_LSS, NUM_STD_FILTER_ELEMENTS);
	err = can_mcan_write_reg(dev, CAN_MCAN_SIDFC, reg);
	if (err != 0) {
		return err;
	}

	reg = ((mram - mrba + CAN_MCAN_MRAM_OFFSET_EXT_FILTER) & CAN_MCAN_XIDFC_FLESA) |
	      FIELD_PREP(CAN_MCAN_XIDFC_LSS, NUM_EXT_FILTER_ELEMENTS);
	err = can_mcan_write_reg(dev, CAN_MCAN_XIDFC, reg);
	if (err != 0) {
		return err;
	}

	reg = ((mram - mrba + CAN_MCAN_MRAM_OFFSET_RX_FIFO0) & CAN_MCAN_RXF0C_F0SA) |
	      FIELD_PREP(CAN_MCAN_RXF0C_F0S, NUM_RX_FIFO0_ELEMENTS);
	err = can_mcan_write_reg(dev, CAN_MCAN_RXF0C, reg);
	if (err != 0) {
		return err;
	}

	reg = ((mram - mrba + CAN_MCAN_MRAM_OFFSET_RX_FIFO1) & CAN_MCAN_RXF1C_F1SA) |
	      FIELD_PREP(CAN_MCAN_RXF1C_F1S, NUM_RX_FIFO1_ELEMENTS);
	err = can_mcan_write_reg(dev, CAN_MCAN_RXF1C, reg);
	if (err != 0) {
		return err;
	}

	reg = ((mram - mrba + CAN_MCAN_MRAM_OFFSET_RX_BUFFER) & CAN_MCAN_RXBC_RBSA);
	err = can_mcan_write_reg(dev, CAN_MCAN_RXBC, reg);
	if (err != 0) {
		return err;
	}

	reg = ((mram - mrba + CAN_MCAN_MRAM_OFFSET_TX_EVENT_FIFO) & CAN_MCAN_TXEFC_EFSA) |
	      FIELD_PREP(CAN_MCAN_TXEFC_EFS, NUM_TX_EVENT_FIFO_ELEMENTS);
	err = can_mcan_write_reg(dev, CAN_MCAN_TXEFC, reg);
	if (err != 0) {
		return err;
	}

	reg = ((mram - mrba + CAN_MCAN_MRAM_OFFSET_TX_BUFFER) & CAN_MCAN_TXBC_TBSA) |
	      FIELD_PREP(CAN_MCAN_TXBC_TFQS, NUM_TX_BUF_ELEMENTS) | CAN_MCAN_TXBC_TFQM;
	err = can_mcan_write_reg(dev, CAN_MCAN_TXBC, reg);
	if (err != 0) {
		return err;
	}

	/* 64 byte Tx Buffer data fields size */
	reg = CAN_MCAN_TXESC_TBDS;
	err = can_mcan_write_reg(dev, CAN_MCAN_TXESC, reg);
	if (err != 0) {
		return err;
	}

	/* 64 byte Rx Buffer/FIFO1/FIFO0 data fields size */
	reg = CAN_MCAN_RXESC_RBDS | CAN_MCAN_RXESC_F1DS | CAN_MCAN_RXESC_F0DS;
	err = can_mcan_write_reg(dev, CAN_MCAN_RXESC, reg);
	if (err != 0) {
		return err;
	}

	return 0;
}

int can_mcan_init(const struct device *dev)
{
	const struct can_mcan_config *config = dev->config;
	struct can_mcan_data *data = dev->data;
	struct can_timing timing;
#ifdef CONFIG_CAN_FD_MODE
	struct can_timing timing_data;
#endif /* CONFIG_CAN_FD_MODE */
	uint32_t reg;
	int err;

	__ASSERT_NO_MSG(config->ops->read_reg != NULL);
	__ASSERT_NO_MSG(config->ops->write_reg != NULL);
	__ASSERT_NO_MSG(config->ops->read_mram != NULL);
	__ASSERT_NO_MSG(config->ops->write_mram != NULL);
	__ASSERT_NO_MSG(config->ops->clear_mram != NULL);

	k_mutex_init(&data->lock);
	k_mutex_init(&data->tx_mtx);
	k_sem_init(&data->tx_sem, NUM_TX_BUF_ELEMENTS, NUM_TX_BUF_ELEMENTS);

	if (config->phy != NULL) {
		if (!device_is_ready(config->phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	err = can_mcan_exit_sleep_mode(dev);
	if (err != 0) {
		LOG_ERR("Failed to exit sleep mode");
		return -EIO;
	}

	err = can_mcan_enter_init_mode(dev, K_MSEC(CAN_INIT_TIMEOUT_MS));
	if (err != 0) {
		LOG_ERR("Failed to enter init mode");
		return -EIO;
	}

	can_mcan_enable_configuration_change(dev);

#if CONFIG_CAN_LOG_LEVEL >= LOG_LEVEL_DBG
	err = can_mcan_read_reg(dev, CAN_MCAN_CREL, &reg);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("IP rel: %lu.%lu.%lu %02lu.%lu.%lu", FIELD_GET(CAN_MCAN_CREL_REL, reg),
		FIELD_GET(CAN_MCAN_CREL_STEP, reg), FIELD_GET(CAN_MCAN_CREL_SUBSTEP, reg),
		FIELD_GET(CAN_MCAN_CREL_YEAR, reg), FIELD_GET(CAN_MCAN_CREL_MON, reg),
		FIELD_GET(CAN_MCAN_CREL_DAY, reg));
#endif /* CONFIG_CAN_LOG_LEVEL >= LOG_LEVEL_DBG */

	err = can_mcan_read_reg(dev, CAN_MCAN_CCCR, &reg);
	if (err != 0) {
		return err;
	}

	reg &= ~(CAN_MCAN_CCCR_FDOE | CAN_MCAN_CCCR_BRSE | CAN_MCAN_CCCR_TEST | CAN_MCAN_CCCR_MON |
		 CAN_MCAN_CCCR_ASM);

	err = can_mcan_write_reg(dev, CAN_MCAN_CCCR, reg);
	if (err != 0) {
		return err;
	}

	err = can_mcan_read_reg(dev, CAN_MCAN_TEST, &reg);
	if (err != 0) {
		return err;
	}

	reg &= ~(CAN_MCAN_TEST_LBCK);

	err = can_mcan_write_reg(dev, CAN_MCAN_TEST, reg);
	if (err != 0) {
		return err;
	}

#if defined(CONFIG_CAN_DELAY_COMP) && defined(CONFIG_CAN_FD_MODE)
	err = can_mcan_read_reg(dev, CAN_MCAN_DBTP, &reg);
	if (err != 0) {
		return err;
	}

	reg |= CAN_MCAN_DBTP_TDC;

	err = can_mcan_write_reg(dev, CAN_MCAN_DBTP, reg);
	if (err != 0) {
		return err;
	}

	err = can_mcan_read_reg(dev, CAN_MCAN_TDCR, &reg);
	if (err != 0) {
		return err;
	}

	reg |= FIELD_PREP(CAN_MCAN_TDCR_TDCO, config->tx_delay_comp_offset);

	err = can_mcan_write_reg(dev, CAN_MCAN_TDCR, reg);
	if (err != 0) {
		return err;
	}
#endif /* defined(CONFIG_CAN_DELAY_COMP) && defined(CONFIG_CAN_FD_MODE) */

	err = can_mcan_read_reg(dev, CAN_MCAN_GFC, &reg);
	if (err != 0) {
		return err;
	}

	reg |= FIELD_PREP(CAN_MCAN_GFC_ANFE, 0x2) | FIELD_PREP(CAN_MCAN_GFC_ANFS, 0x2);

	err = can_mcan_write_reg(dev, CAN_MCAN_GFC, reg);
	if (err != 0) {
		return err;
	}

	if (config->sample_point) {
		err = can_calc_timing(dev, &timing, config->bus_speed, config->sample_point);
		if (err == -EINVAL) {
			LOG_ERR("Can't find timing for given param");
			return -EIO;
		}
		LOG_DBG("Presc: %d, TS1: %d, TS2: %d", timing.prescaler, timing.phase_seg1,
			timing.phase_seg2);
		LOG_DBG("Sample-point err : %d", err);
	} else if (config->prop_ts1) {
		timing.prop_seg = 0U;
		timing.phase_seg1 = config->prop_ts1;
		timing.phase_seg2 = config->ts2;
		err = can_calc_prescaler(dev, &timing, config->bus_speed);
		if (err != 0) {
			LOG_WRN("Bitrate error: %d", err);
		}
	}
#ifdef CONFIG_CAN_FD_MODE
	if (config->sample_point_data) {
		err = can_calc_timing_data(dev, &timing_data, config->bus_speed_data,
					   config->sample_point_data);
		if (err == -EINVAL) {
			LOG_ERR("Can't find timing for given dataphase param");
			return -EIO;
		}

		LOG_DBG("Sample-point err data phase: %d", err);
	} else if (config->prop_ts1_data) {
		timing_data.prop_seg = 0U;
		timing_data.phase_seg1 = config->prop_ts1_data;
		timing_data.phase_seg2 = config->ts2_data;
		err = can_calc_prescaler(dev, &timing_data, config->bus_speed_data);
		if (err != 0) {
			LOG_WRN("Dataphase bitrate error: %d", err);
		}
	}
#endif /* CONFIG_CAN_FD_MODE */

	timing.sjw = config->sjw;
	can_mcan_set_timing(dev, &timing);

#ifdef CONFIG_CAN_FD_MODE
	timing_data.sjw = config->sjw_data;
	can_mcan_set_timing_data(dev, &timing_data);
#endif /* CONFIG_CAN_FD_MODE */

	reg = CAN_MCAN_IE_BOE | CAN_MCAN_IE_EWE | CAN_MCAN_IE_EPE | CAN_MCAN_IE_MRAFE |
	      CAN_MCAN_IE_TEFLE | CAN_MCAN_IE_TEFNE | CAN_MCAN_IE_RF0NE | CAN_MCAN_IE_RF1NE |
	      CAN_MCAN_IE_RF0LE | CAN_MCAN_IE_RF1LE;

	err = can_mcan_write_reg(dev, CAN_MCAN_IE, reg);
	if (err != 0) {
		return err;
	}

	reg = CAN_MCAN_ILS_RF0NL | CAN_MCAN_ILS_RF1NL;
	err = can_mcan_write_reg(dev, CAN_MCAN_ILS, reg);
	if (err != 0) {
		return err;
	}

	reg = CAN_MCAN_ILE_EINT0 | CAN_MCAN_ILE_EINT1;
	err = can_mcan_write_reg(dev, CAN_MCAN_ILE, reg);
	if (err != 0) {
		return err;
	}

	/* Interrupt on every TX fifo element*/
	reg = CAN_MCAN_TXBTIE_TIE;
	err = can_mcan_write_reg(dev, CAN_MCAN_TXBTIE, reg);
	if (err != 0) {
		return err;
	}

	return can_mcan_clear_mram(dev, 0, sizeof(struct can_mcan_msg_sram));
}
