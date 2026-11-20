/*
 * Copyright (c) 2025 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_ameba_can

#include <ameba_soc.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(can_ameba, CONFIG_CAN_LOG_LEVEL);

#define CAN_TODO 0
static u32 can_ameba_ram_buffer_map[] = {0x0,  0x6,  0xc,  0x12, 0x18, 0x1e, 0x24, 0x2a, 0x30,
					 0x36, 0x3c, 0x42, 0x48, 0x4e, 0x54, 0x5a, 0x60};

#define CAN_AMEBA_CAN_TIMING_MIN                                                                   \
	{                                                                                          \
		.sjw = 1,                                                                          \
		.prop_seg = 2,                                                                     \
		.phase_seg1 = 2,                                                                   \
		.phase_seg2 = 2,                                                                   \
		.prescaler = 1,                                                                    \
	}

#define CAN_AMEBA_CAN_TIMING_MAX                                                                   \
	{                                                                                          \
		.sjw = 4,                                                                          \
		.prop_seg = 6,                                                                     \
		.phase_seg1 = 8,                                                                   \
		.phase_seg2 = 8,                                                                   \
		.prescaler = 32,                                                                   \
	}

/*
 * Mutex to prevent simultaneous access to filter registers shared between CAN1
 * and CAN2.
 */

#define CAN_AMEBA_TX_PENDING_DEPTH 4

struct can_ameba_pending_tx {
	struct can_frame frame;
	can_tx_callback_t callback;
	void *user_data;
};

/**
 * @brief Ameba-CAN driver internal RX filter structure.
 */
struct can_ameba_rx_filter {
	struct can_filter filter;
	can_rx_callback_t callback;
	void *user_data;
};

struct can_ameba_config {
	const struct can_driver_config common;
	CAN_TypeDef *base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	void (*config_irq)(void);
};

struct can_ameba_data {
	struct can_driver_data common;
	struct k_mutex inst_mutex;

	ATOMIC_DEFINE(rx_allocs, CONFIG_CAN_AMEBA_MAX_FILTERS);
	struct can_ameba_rx_filter filters[CONFIG_CAN_AMEBA_MAX_FILTERS];
	struct k_sem tx_idle;
	can_tx_callback_t tx_callback;
	void *tx_user_data;
	enum can_state state;

	struct can_ameba_pending_tx tx_pending[CAN_AMEBA_TX_PENDING_DEPTH];
	uint8_t tx_pending_head;
	uint8_t tx_pending_tail;
	uint8_t tx_pending_count;
};

static void can_ameba_hw_tx_start(const struct device *dev, const struct can_frame *frame)
{
	const struct can_ameba_config *can_config = dev->config;
	CAN_TypeDef *can = can_config->base;
	u32 can_ram_cmd, can_ram_arb, can_ram_cs;

	can_ram_cmd = (CAN_BIT_RAM_BUFFER_EN | CAN_BIT_RAM_ACC_ARB | CAN_BIT_RAM_ACC_CS |
		       CAN_BIT_RAM_ACC_MASK | CAN_BIT_RAM_ACC_DATA_MASK | CAN_BIT_RAM_DIR);
	can_ram_cmd |= CAN_RAM_ACC_NUM(0);
	can->CAN_RAM_CMD = can_ram_cmd;

	can_ram_arb = can->CAN_RAM_ARB;
	can_ram_arb &= (~(CAN_BIT_RAM_RTR | CAN_BIT_RAM_IDE | CAN_MASK_RAM_ID));
	if (frame->flags & CAN_FRAME_RTR) {
		can_ram_arb |= CAN_BIT_RAM_RTR;
	}
	if (frame->flags & CAN_FRAME_IDE) {
		can_ram_arb |= frame->id | CAN_BIT_RAM_IDE;
	} else {
		can_ram_arb |= (frame->id & 0x7FF) << 18;
	}
	can->CAN_RAM_ARB = can_ram_arb;

	can_ram_cs = can->CAN_RAM_CS;
	can_ram_cs &= (~(CAN_MASK_RAM_DLC | CAN_BIT_RAM_AUTOREPLY));
	can_ram_cs |= CAN_RAM_DLC(frame->dlc);
	can_ram_cs |= CAN_BIT_RAM_RXTX;
	can->CAN_RAM_CS = can_ram_cs;

	if ((frame->flags & CAN_FRAME_RTR) == 0) {
		can->CAN_RAM_DATA_x[15] = frame->data_32[0];
		can->CAN_RAM_DATA_x[14] = frame->data_32[1];
	}
	can->CAN_RAM_CMD |= CAN_BIT_RAM_START;
	while (can->CAN_RAM_CMD & CAN_BIT_RAM_START) {
	}
}

static void can_ameba_tx_done(const struct device *dev, int status)
{
	struct can_ameba_data *can_data = dev->data;
	can_tx_callback_t cb = can_data->tx_callback;
	void *user_data = can_data->tx_user_data;

	can_data->tx_callback = NULL;

	if (cb) {
		cb(dev, status, user_data);
	}

	/*
	 * On TX success or recoverable error, start the next queued frame (if
	 * any) so the hardware buffer stays busy without a round-trip through
	 * the application.  On fatal conditions (bus-off, stopped) drain the
	 * entire pending queue and return the semaphore.
	 */
	unsigned int key = irq_lock();

	while (can_data->tx_pending_count > 0) {
		struct can_ameba_pending_tx pt = can_data->tx_pending[can_data->tx_pending_head];

		can_data->tx_pending_head =
			(can_data->tx_pending_head + 1) % CAN_AMEBA_TX_PENDING_DEPTH;
		can_data->tx_pending_count--;

		if (status == 0 || status == -EIO) {
			can_data->tx_callback = pt.callback;
			can_data->tx_user_data = pt.user_data;
			irq_unlock(key);
			can_ameba_hw_tx_start(dev, &pt.frame);
			return; /* tx_idle stays taken */
		}

		/* Fatal error: drain remaining with same status */
		irq_unlock(key);
		if (pt.callback) {
			pt.callback(dev, status, pt.user_data);
		}
		key = irq_lock();
	}

	irq_unlock(key);
	k_sem_give(&can_data->tx_idle);
}

static void can_ameba_status_reset(CAN_TypeDef *can)
{
	CAN_TXErrCntClear(can);
	CAN_RXErrCntClear(can);
	CAN_ClearErrStatus(can, CAN_BIT_ERROR_RX | CAN_BIT_ERROR_TX | CAN_BIT_ERROR_ACK |
					CAN_BIT_ERROR_STUFF | CAN_BIT_ERROR_CRC |
					CAN_BIT_ERROR_FORM | CAN_BIT_ERROR_BIT1 |
					CAN_BIT_ERROR_BIT0);
	CAN_TxMsgBufErrClear(can, CAN_MASK_TX_ERROR_FLAG);
	CAN_TxDoneStatusClear(can, CAN_MASK_TX_DONE);
	CAN_RxDoneStatusClear(can, CAN_MASK_RX_DONE);
}
static int can_ameba_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT |
	       CAN_MODE_3_SAMPLES;

	return 0;
}

static int can_ameba_start(const struct device *dev)
{
	const struct can_ameba_config *can_config = dev->config;
	struct can_ameba_data *can_data = dev->data;
	int ret = 0;

	k_mutex_lock(&can_data->inst_mutex, K_FOREVER);

	if (can_data->common.started) {
		ret = -EALREADY;
		goto unlock;
	}

	CAN_STATS_RESET(dev);

	/* 1.Bus off */
	CAN_BusCmd(can_config->base, DISABLE);

	/* 2.Clear Tec&Rec count/error status/msg buffer status */
	can_ameba_status_reset(can_config->base);

	/* 3.Bus On */
	CAN_BusCmd(can_config->base, ENABLE);

	/* Reset software error state — hardware error counters were cleared
	 * above, so the controller is back to error-active on start.
	 */
	can_data->state = CAN_STATE_ERROR_ACTIVE;
	can_data->common.started = true;

unlock:
	k_mutex_unlock(&can_data->inst_mutex);

	return ret;
}

static int can_ameba_stop(const struct device *dev)
{
	const struct can_ameba_config *can_config = dev->config;
	struct can_ameba_data *can_data = dev->data;

	int ret = 0;

	k_mutex_lock(&can_data->inst_mutex, K_FOREVER);

	if (!can_data->common.started) {
		ret = -EALREADY;
		goto unlock;
	}
	/* 1.Clear irq flags/Tec&Rec count/error status/msg buffer status */
	CAN_ClearAllINT(can_config->base);

	/* 2.Close bus */
	CAN_BusCmd(can_config->base, DISABLE);

	/* 3.Update status */
	can_data->common.started = false;

	can_ameba_tx_done(dev, -ENETDOWN);

unlock:
	k_mutex_unlock(&can_data->inst_mutex);

	return ret;
}

static int can_ameba_set_mode(const struct device *dev, can_mode_t mode)
{
	can_mode_t supported = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY |
			       CAN_MODE_ONE_SHOT | CAN_MODE_3_SAMPLES;
	const struct can_ameba_config *can_config = dev->config;
	CAN_TypeDef *can = can_config->base;
	struct can_ameba_data *data = dev->data;

	LOG_DBG("Set mode %d", mode);

	if ((mode & ~(supported)) != 0) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	if (data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	/* set work Mode */
	if ((mode & CAN_MODE_NORMAL) != 0) {
		/* Normal mode */
		can->CAN_CTL &= ~CAN_BIT_TEST_MODE_EN;

	} else {
		can->CAN_CTL |= CAN_BIT_TEST_MODE_EN;
		can->CAN_TEST &= ~CAN_MASK_TEST_CFG;

		/* Loopback mode: use internal loopback so TX frames are
		 * self-ACK'd by hardware — no external CAN node required.
		 */
		if ((mode & CAN_MODE_LOOPBACK) != 0) {
			can->CAN_TEST |= CAN_TEST_CFG(CAN_INT_LOOPBACK_MODE);
		}
		/* Silence mode*/
		if ((mode & CAN_MODE_LISTENONLY) != 0) {
			can->CAN_TEST |= CAN_TEST_CFG(CAN_SILENCE_MODE);
		}
	}

	/* auto reply */
	if ((mode & CAN_MODE_ONE_SHOT) != 0) {
		/* No automatic retransmission */
		can->CAN_CTL &= ~CAN_BIT_AUTO_RE_TX_EN;
	} else {
		can->CAN_CTL |= CAN_BIT_AUTO_RE_TX_EN;
	}

	/* tri-sample */
	if ((mode & CAN_MODE_3_SAMPLES) != 0) {
		can->CAN_CTL |= CAN_BIT_TRI_SAMPLE;
	} else {
		can->CAN_CTL &= ~CAN_BIT_TRI_SAMPLE;
	}

	data->common.mode = mode;

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static int can_ameba_send(const struct device *dev, const struct can_frame *frame,
			  k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	struct can_ameba_data *can_data = dev->data;

	LOG_DBG("Sending %d bytes on %s. "
		"Id: 0x%x, "
		"ID type: %s, "
		"Remote Frame: %s",
		frame->dlc, dev->name, frame->id,
		(frame->flags & CAN_FRAME_IDE) != 0 ? "extended" : "standard",
		(frame->flags & CAN_FRAME_RTR) != 0 ? "yes" : "no");

	if ((frame->flags & (CAN_FRAME_FDF | CAN_FRAME_BRS | CAN_FRAME_ESI)) != 0) {
		LOG_ERR("CAN-FD Not Available");
		return -ENOTSUP;
	}

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("TX frame DLC %u exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if (!can_data->common.started) {
		return -ENETDOWN;
	}

	if (can_data->state == CAN_STATE_BUS_OFF) {
		LOG_DBG("transmit failed, bus-off");
		return -ENETUNREACH;
	}

	if (k_sem_take(&can_data->tx_idle, K_NO_WAIT) != 0) {
		/*
		 * Hardware TX buffer busy.  Try to place the frame in the
		 * software pending queue so the caller doesn't see -EAGAIN
		 * just because the single hardware slot is temporarily occupied.
		 * can_ameba_tx_done() will dequeue and start it automatically.
		 */
		unsigned int irq_key = irq_lock();

		if (can_data->tx_pending_count < CAN_AMEBA_TX_PENDING_DEPTH) {
			struct can_ameba_pending_tx *pt =
				&can_data->tx_pending[can_data->tx_pending_tail];

			pt->frame = *frame;
			pt->callback = callback;
			pt->user_data = user_data;
			can_data->tx_pending_tail =
				(can_data->tx_pending_tail + 1) % CAN_AMEBA_TX_PENDING_DEPTH;
			can_data->tx_pending_count++;
			irq_unlock(irq_key);
			return 0;
		}

		irq_unlock(irq_key);

		/* Pending queue also full — fall back to blocking on tx_idle */
		if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
			return -EAGAIN;
		}
		if (k_sem_take(&can_data->tx_idle, timeout) != 0) {
			return -EAGAIN;
		}
	}

	can_data->tx_callback = callback;
	can_data->tx_user_data = user_data;

	k_mutex_lock(&can_data->inst_mutex, K_FOREVER);
	can_ameba_hw_tx_start(dev, frame);
	k_mutex_unlock(&can_data->inst_mutex);

	return 0;
}

static int can_ameba_set_timing(const struct device *dev, const struct can_timing *timing)
{
	const struct can_ameba_config *can_config = dev->config;
	struct can_ameba_data *can_data = dev->data;
	CAN_TypeDef *can = can_config->base;

	if (can_data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&can_data->inst_mutex, K_FOREVER);

	can->CAN_BIT_TIMING &= ~(CAN_MASK_BRP | CAN_MASK_SJW | CAN_MASK_TSEG2 | CAN_MASK_TSEG1);
	can->CAN_BIT_TIMING |= CAN_BRP(timing->prescaler - 1) | CAN_SJW(timing->sjw) |
			       CAN_TSEG1(timing->prop_seg + timing->phase_seg1 - 1) |
			       CAN_TSEG2(timing->phase_seg2 - 1);

	if ((can_data->common.mode & CAN_MODE_3_SAMPLES) != 0) {
		can->CAN_CTL |= CAN_BIT_TRI_SAMPLE;
	}

	k_mutex_unlock(&can_data->inst_mutex);

	return 0;
}

static int can_ameba_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	*rate = XTAL_40M;

	return 0;
}
void can_ameba_read_msg(CAN_TypeDef *CANx, struct can_frame *frame, uint8_t msg_idx)
{
	u32 can_ram_arb, can_ram_cmd, can_ram_cs;

	/* Enable msg buffer access*/
	can_ram_cmd = (CAN_BIT_RAM_BUFFER_EN | CAN_BIT_RAM_ACC_ARB | CAN_BIT_RAM_ACC_CS |
		       CAN_BIT_RAM_ACC_MASK | CAN_BIT_RAM_ACC_DATA_MASK);
	can_ram_cmd |= msg_idx;
	/* Read frame into register from ram message buffer */
	can_ram_cmd |= CAN_BIT_RAM_START;
	CANx->CAN_RAM_CMD = can_ram_cmd;

	/* Read frame flags and ID */
	can_ram_arb = CANx->CAN_RAM_ARB;
	if (can_ram_arb & CAN_BIT_RAM_RTR) {
		frame->flags |= CAN_FRAME_RTR;
	}

	if (can_ram_arb & CAN_BIT_RAM_IDE) {
		frame->flags |= CAN_FRAME_IDE;
		frame->id = CAN_GET_RAM_ID(can_ram_arb);
	} else {
		frame->id = CAN_GET_RAM_ID(can_ram_arb) >> 18;
	}
	/* Read data length of frame*/
	can_ram_cs = CANx->CAN_RAM_CS;
#if defined(CONFIG_CAN_RX_TIMESTAMP)
	frame->timestamp = CAN_GET_RAM_TIMESTAMP(can_ram_cs);
#endif

	if (frame->flags & CAN_FRAME_RTR) {
		frame->dlc = 0;
	} else {
		frame->dlc = CAN_GET_RAM_DLC(can_ram_cs);
	}

	/* Get Data: can2.0 8 bytes, can fd 64 bytes */
	if ((frame->flags & CAN_FRAME_RTR) == 0) {
		frame->data_32[0] = CANx->CAN_RAM_DATA_x[15];
		frame->data_32[1] = CANx->CAN_RAM_DATA_x[14];
	}
}

static void can_ameba_rx_msg(const struct device *dev)
{
	const struct can_ameba_config *can_config = dev->config;
	struct can_ameba_data *can_data = dev->data;
	CAN_TypeDef *can = can_config->base;
	struct can_frame frame;
	can_rx_callback_t callback;

	for (int msg_buf_idx = CAN_MESSAGE_BUFFER_SIZE - 1; msg_buf_idx >= 0; msg_buf_idx--) {
		memset(&frame, 0, sizeof(struct can_frame));
		if (CAN_MsgBufRxDoneStatusGet(can, msg_buf_idx)) {
			CAN_MsgBufRxDoneStatusClear(can, msg_buf_idx);
			can_ameba_read_msg(can, &frame, msg_buf_idx);

#ifndef CONFIG_CAN_ACCEPT_RTR
			if ((frame.flags & CAN_FRAME_RTR) == 0U) {
#endif /* !CONFIG_CAN_ACCEPT_RTR */
				for (int i = 0; i < ARRAY_SIZE(can_data->filters); i++) {
					if (!atomic_test_bit(can_data->rx_allocs, i)) {
						continue;
					}
					if (!can_frame_matches_filter(
						    &frame, &can_data->filters[i].filter)) {
						continue;
					}

					callback = can_data->filters[i].callback;
					if (callback != NULL) {
						callback(dev, &frame,
							 can_data->filters[i].user_data);
					}
				}
#ifndef CONFIG_CAN_ACCEPT_RTR
			}
#endif /* !CONFIG_CAN_ACCEPT_RTR */
		}
	}
}

static void can_ameba_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct can_ameba_config *can_config = dev->config;
	struct can_ameba_data *can_data = dev->data;
	CAN_TypeDef *can = can_config->base;
	u32 IntStatus, ErrStatus, TxErCnt, RxErCnt, ErrPassive, ErrBusoff, ErrWarning;

	IntStatus = CAN_GetINTStatus(can);
	/* ram move done interrupt */
	if (IntStatus & CAN_RAM_MOVE_DONE_INT) {
		CAN_ClearINT(can, CAN_BIT_RAM_MOVE_DONE_INT_FLAG);
		LOG_DBG("RAM MOVE DONE INT");
	}

	/* tx interrupt */
	if (IntStatus & CAN_TX_INT) {
		CAN_ClearINT(can, CAN_BIT_TX_INT_FLAG);
		can_ameba_tx_done(dev, 0);
	}

	/* rx interrupt */
	if (IntStatus & CAN_RX_INT) {
		CAN_ClearINT(can, CAN_BIT_RX_INT_FLAG);
		LOG_DBG("RX INT");
		/* get current error status */
		TxErCnt = CAN_TXErrCntGet(can);
		RxErCnt = CAN_RXErrCntGet(can);
		ErrPassive = (can->CAN_ERR_CNT_STS & CAN_BIT_ERROR_PASSIVE) >> 28;
		ErrBusoff = (can->CAN_ERR_CNT_STS & CAN_BIT_ERROR_BUSOFF) >> 29;
		ErrWarning = (can->CAN_ERR_CNT_STS & CAN_BIT_ERROR_WARNING) >> 30;
		can_ameba_rx_msg(dev);
	}

	/* bus off interrupt */
	if (IntStatus & CAN_BUSOFF_INT) {
		CAN_ClearINT(can, CAN_BIT_BUSOFF_INT_FLAG);
		can_data->state = CAN_STATE_BUS_OFF;
		can_ameba_tx_done(dev, -ENETUNREACH);
		LOG_DBG("CAN: bus off");
	}

	/* wakeup interrupt */
	if (IntStatus & CAN_WKUP_INT) {
		CAN_ClearINT(can, CAN_BIT_WAKEUP_INT_FLAG);
		LOG_DBG("CAN: wake up");
	}

	/* error interrupt */
	if (IntStatus & CAN_ERR_INT) {
		CAN_ClearINT(can, CAN_BIT_ERROR_INT_FLAG);
		LOG_DBG("CAN: ERR_INT status = %x", CAN_GetINTStatus(can));

		ErrStatus = CAN_GetErrStatus(can);
		TxErCnt = CAN_TXErrCntGet(can);
		RxErCnt = CAN_RXErrCntGet(can);
		ErrPassive = (can->CAN_ERR_CNT_STS & CAN_BIT_ERROR_PASSIVE) >> 28;
		ErrBusoff = (can->CAN_ERR_CNT_STS & CAN_BIT_ERROR_BUSOFF) >> 29;
		ErrWarning = (can->CAN_ERR_CNT_STS & CAN_BIT_ERROR_WARNING) >> 30;

		if (ErrStatus & CAN_BIT_ERROR_BIT0) {
			CAN_ClearErrStatus(can, CAN_BIT_ERROR_BIT0);
			LOG_ERR("bit 0 error: tx = 0, but rx = 1");
		}
		if (ErrStatus & CAN_BIT_ERROR_BIT1) {
			CAN_ClearErrStatus(can, CAN_BIT_ERROR_BIT1);
			LOG_ERR("bit 1 error: tx = 1, but rx = 0");
		}
		if (ErrStatus & CAN_BIT_ERROR_FORM) {
			CAN_ClearErrStatus(can, CAN_BIT_ERROR_FORM);
			LOG_ERR("form error");
		}
		if (ErrStatus & CAN_BIT_ERROR_CRC) {
			CAN_ClearErrStatus(can, CAN_BIT_ERROR_CRC);
			LOG_ERR("CRC error");
		}
		if (ErrStatus & CAN_BIT_ERROR_STUFF) {
			CAN_ClearErrStatus(can, CAN_BIT_ERROR_STUFF);
			LOG_ERR("stuff error");
		}
		if (ErrStatus & CAN_BIT_ERROR_ACK) {
			CAN_ClearErrStatus(can, CAN_BIT_ERROR_ACK);
			LOG_ERR("ACK error");
		}
		if (ErrStatus & CAN_BIT_ERROR_TX) {
			CAN_ClearErrStatus(can, CAN_BIT_ERROR_TX);
			LOG_ERR("tx error");
		}
		if (ErrStatus & CAN_BIT_ERROR_RX) {
			CAN_ClearErrStatus(can, CAN_BIT_ERROR_RX);
			LOG_ERR("rx error");
		}

		LOG_DBG("ErrStatus = %x, TEC = %d, REC = %d, ErrPassive = %d, ErrBusoff = %d, "
			"ErrWarning = %d",
			ErrStatus, TxErCnt, RxErCnt, ErrPassive, ErrBusoff, ErrWarning);

		/*
		 * Release a pending TX only when auto-retry is disabled.
		 * With auto-retry enabled (CAN_MODE_NORMAL) the hardware will
		 * keep retrying the frame; calling tx_done here would invoke the
		 * application callback prematurely and swallow the eventual
		 * CAN_TX_INT success notification.
		 */
		if ((ErrStatus & (CAN_BIT_ERROR_TX | CAN_BIT_ERROR_ACK)) &&
		    !(can->CAN_CTL & CAN_BIT_AUTO_RE_TX_EN)) {
			can_ameba_tx_done(dev, -EIO);
		}
	}
}

static int can_ameba_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
				   void *user_data, const struct can_filter *filter)
{
	const struct can_ameba_config *can_config = dev->config;
	struct can_ameba_data *can_data = dev->data;
	CAN_TypeDef *can = can_config->base;
	uint32_t can_ram_cmd, can_ram_arb;
	int filter_id = -ENOSPC;
	int i;

	for (i = 0; i < ARRAY_SIZE(can_data->filters); i++) {
		if (!atomic_test_and_set_bit(can_data->rx_allocs, i)) {
			filter_id = i;
			break;
		}
	}
	k_mutex_lock(&can_data->inst_mutex, K_FOREVER);

	if (filter_id >= 0) {
		can_data->filters[filter_id].filter = *filter;
		can_data->filters[filter_id].user_data = user_data;
		can_data->filters[filter_id].callback = callback;

		/* Enable Msg Buffer access*/
		can_ram_cmd = (CAN_BIT_RAM_BUFFER_EN | CAN_BIT_RAM_ACC_ARB | CAN_BIT_RAM_ACC_CS |
			       CAN_BIT_RAM_ACC_MASK | CAN_BIT_RAM_ACC_DATA_MASK | CAN_BIT_RAM_DIR);
		can_ram_cmd &= ~CAN_MASK_RAM_ACC_NUM;
		can_ram_cmd |= CAN_RAM_ACC_NUM(CAN_MESSAGE_BUFFER_SIZE - filter_id - 1);

		/*Config frame header*/
		if (filter->flags & CAN_FILTER_IDE) {
			can_ram_arb = CAN_RAM_ID(filter->id) | CAN_BIT_RAM_IDE;
		} else {
			can_ram_arb = CAN_RAM_ID((filter->id & 0x7FF) << 18);
		}

		if (filter->flags & CAN_FRAME_RTR) {
			can_ram_arb |= CAN_BIT_RAM_RTR;
		}

		can->CAN_RAM_ARB = can_ram_arb;

		/* Enable the current buffer to receive data */
		can->CAN_RAM_CS &= ~CAN_BIT_RAM_RXTX;

		/* Set the mask ID to block unwanted data frames */
		if (filter->flags & CAN_FILTER_IDE) {
			can->CAN_RAM_MASK = CAN_BIT_RAM_IDE_MASK | CAN_RAM_ID_MASK(filter->mask);
		} else {
			can->CAN_RAM_MASK = CAN_RAM_ID_MASK(filter->mask << 18);
		}

		/* Write RX setting into the ram message buffer */
		can_ram_cmd |= CAN_BIT_RAM_START;
		can->CAN_RAM_CMD = can_ram_cmd;
		LOG_DBG("RX Settings: ARB = %x, CMD = %x, MSK = %x", can->CAN_RAM_ARB,
			can->CAN_RAM_CMD, can->CAN_RAM_MASK);
		while (can->CAN_RAM_CMD & CAN_BIT_RAM_START) {
		}
	}
	k_mutex_unlock(&can_data->inst_mutex);
	return filter_id;
}

static void can_ameba_remove_rx_filter(const struct device *dev, int filter_id)
{
	const struct can_ameba_config *can_config = dev->config;
	struct can_ameba_data *can_data = dev->data;
	CAN_TypeDef *can = can_config->base;
	uint32_t can_ram_cmd;
	if (filter_id < 0 || filter_id >= ARRAY_SIZE(can_data->filters)) {
		LOG_ERR("filter ID %d out of bounds", filter_id);
		return;
	}

	if (atomic_test_and_clear_bit(can_data->rx_allocs, filter_id)) {
		can_data->filters[filter_id].callback = NULL;
		can_data->filters[filter_id].user_data = NULL;
		can_data->filters[filter_id].filter = (struct can_filter){0};

		/* Enable msg buf[x] access */
		can_ram_cmd = CAN_RAM_ACC_NUM(CAN_MESSAGE_BUFFER_SIZE - filter_id - 1);

		/* Reset RAM ARB/CS/MASK/ reg */
		can->CAN_RAM_ARB = 0;
		can->CAN_RAM_CS = 0;
		can->CAN_RAM_MASK = 0;

		/* Write RX setting into the ram message buffer */
		can_ram_cmd |= CAN_BIT_RAM_START;
		can->CAN_RAM_CMD = can_ram_cmd;
		LOG_DBG("ARB = %x, CMD = %x, MSK = %x", can->CAN_RAM_ARB, can->CAN_RAM_CMD,
			can->CAN_RAM_MASK);
	}
}

static int can_ameba_get_state(const struct device *dev, enum can_state *state,
			       struct can_bus_err_cnt *err_cnt)
{
	const struct can_ameba_config *can_config = dev->config;
	struct can_ameba_data *can_data = dev->data;

	if (state != NULL) {
		if (!can_data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else {
			*state = can_data->state;
		}
	}

	if (err_cnt != NULL) {
		err_cnt->rx_err_cnt = CAN_RXErrCntGet(can_config->base);
		err_cnt->tx_err_cnt = CAN_TXErrCntGet(can_config->base);
	}

	return 0;
}
#if 0
static int can_ameba_state_change_callbacks_enabled(const struct device *dev, bool enabled)
{
	const struct can_ameba_config *can_config = dev->config;

	/*
	 * Bus-off and error-passive/warning reporting is driven solely by the
	 * bus-off and error interrupts.  They are masked until the application
	 * registers a state change callback, and masked again once the last
	 * callback is removed.
	 */
	CAN_INTConfig(can_config->base, CAN_BUSOFF_INT | CAN_ERR_INT,
		      enabled ? ENABLE : DISABLE);

	return 0;
}
#endif

static int can_ameba_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CONFIG_CAN_AMEBA_MAX_FILTERS;
}

static int can_ameba_init(const struct device *dev)
{
	const struct can_ameba_config *can_config = dev->config;
	struct can_ameba_data *can_data = dev->data;
	CAN_TypeDef *can = can_config->base;
	CAN_InitTypeDef CAN_InitStruct;
	int err = 0;

	if (!device_is_ready(can_config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = pinctrl_apply_state(can_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		LOG_ERR("failed to configure CAN pins (err %d)", err);
		return err;
	}

	err = clock_control_on(can_config->clock_dev, can_config->clock_subsys);
	if (err < 0 && err != -EALREADY) {
		LOG_ERR("failed to enable CAN clock (err %d)", err);
		return err;
	}
	RCC_PeriphClockDividerFENSet(USB_PLL_CAN, DISABLE);
	RCC_PeriphClockDividerFENSet(SYS_PLL_CAN, DISABLE);
	RCC_PeriphClockSourceSet(CAN, XTAL);

	/* Config basic parameters */
	CAN_StructInit(&CAN_InitStruct);
	CAN_Init(can, &CAN_InitStruct);

	/* Initialize message buffer addr */
	CAN_RamBufferMapConfig(can_config->base, can_ameba_ram_buffer_map);

	k_sem_init(&can_data->tx_idle, 1, 1);
	/* Enable Interrupt */
	can_config->config_irq();

	/* Enable CAN */
	CAN_Cmd(can, ENABLE);
	k_mutex_init(&can_data->inst_mutex);

	return 0;
}

DEVICE_API(can, can_ameba_driver_api) = {
	.get_capabilities = can_ameba_get_capabilities,
	.start = can_ameba_start,
	.stop = can_ameba_stop,
	.set_mode = can_ameba_set_mode,
	.set_timing = can_ameba_set_timing,
	.send = can_ameba_send,
	.add_rx_filter = can_ameba_add_rx_filter,
	.remove_rx_filter = can_ameba_remove_rx_filter,
	.get_state = can_ameba_get_state,
	.get_core_clock = can_ameba_get_core_clock,
	.get_max_filters = can_ameba_get_max_filters,
	.timing_min = CAN_AMEBA_CAN_TIMING_MIN,
	.timing_max = CAN_AMEBA_CAN_TIMING_MAX,
};

#define CAN_AMEBA_CAN_IRQ_INST(inst)                                                               \
	static void config_can_ameba_##inst##_irq(void)                                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(inst, irq), DT_INST_IRQ(inst, priority), can_ameba_isr,    \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ(inst, irq));                                                \
		CAN_INTConfig((CAN_TypeDef *)DT_INST_REG_ADDR(inst),                               \
			      CAN_TX_INT | CAN_RX_INT | CAN_ERR_INT | CAN_WKUP_INT |               \
				      CAN_BUSOFF_INT | CAN_RAM_MOVE_DONE_INT,                      \
			      ENABLE);                                                             \
		CAN_TxMsgBufINTConfig((CAN_TypeDef *)DT_INST_REG_ADDR(inst),                       \
				      CAN_MB_TXINT_EN(0xFFFF), ENABLE);                            \
		CAN_RxMsgBufINTConfig((CAN_TypeDef *)DT_INST_REG_ADDR(inst),                       \
				      CAN_MB_RXINT_EN(0xFFFF), ENABLE);                            \
	}

#define CAN_AMEBA_CAN_INIT(inst)                                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	CAN_AMEBA_CAN_IRQ_INST(inst)                                                               \
	const struct can_ameba_config can_ameba_config_##inst = {                                  \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 1000000),                         \
		.base = (CAN_TypeDef *)DT_INST_REG_ADDR(inst),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, idx),            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.config_irq = config_can_ameba_##inst##_irq};                                      \
                                                                                                   \
	static struct can_ameba_data can_ameba_data_##inst = {                                     \
		.common = {0},                                                                     \
		.state = 0,                                                                        \
	};                                                                                         \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_ameba_init, NULL, &can_ameba_data_##inst,              \
				  &can_ameba_config_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY, \
				  &can_ameba_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_AMEBA_CAN_INIT)
