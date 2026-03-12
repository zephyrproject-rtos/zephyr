/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_can

#include <soc.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <rtl_can.h>

LOG_MODULE_REGISTER(can_bee, CONFIG_CAN_LOG_LEVEL);

#define CAN_SRC_CLOCK_HZ 40000000U

struct can_bee_mb_tx_data {
	uint8_t idx;
	bool is_busy;
	int status;
	can_tx_callback_t tx_callback;
	void *user_data;
};

struct can_bee_mb_rx_data {
	uint8_t idx;
	bool is_busy;
	can_rx_callback_t rx_callback;
	void *user_data;
	CANRxFrame_TypeDef rx_frame_type;
};

struct can_bee_data {
	struct can_driver_data common;
	struct k_mutex inst_mutex;
	struct k_sem tx_int_sem;
	CAN_InitTypeDef init_struct;
	struct can_bee_mb_tx_data tx_mb_list[CONFIG_CAN_BEE_TX_MSG_BUF_NUM];
	struct can_bee_mb_rx_data rx_mb_list[CONFIG_CAN_BEE_RX_MSG_BUF_NUM];
	enum can_state state;
};

struct can_bee_config {
	const struct can_driver_config common;
	const struct pinctrl_dev_config *pcfg;
	CAN_TypeDef *can;
	uint8_t rx_timestamp_prescaler;
	uint16_t clkid;
	void (*irq_config_func)(void);
};

static void can_bee_wait_ram_state(void)
{
	WAIT_FOR(CAN_GetRamState() == CAN_RAM_STATE_IDLE, 100000, k_busy_wait(1));
}

static int can_bee_check_bus_on(uint32_t timeout)
{
	return WAIT_FOR(CAN_GetBusState() == CAN_BUS_STATE_ON, timeout, k_busy_wait(1)) ? 0
											: -EAGAIN;
}

static void can_bee_tx_complete_with_status(const struct device *dev, int mb_list_idx, int status)
{
	struct can_bee_data *data = dev->data;
	struct can_bee_mb_tx_data *mb = &data->tx_mb_list[mb_list_idx];
	can_tx_callback_t callback;
	void *user_data;

	if (!mb->is_busy) {
		return;
	}

	callback = mb->tx_callback;
	user_data = mb->user_data;

	mb->status = status;
	mb->is_busy = false;
	mb->tx_callback = NULL;
	mb->user_data = NULL;

	k_sem_give(&data->tx_int_sem);

	if (callback != NULL) {
		callback(dev, status, user_data);
	}
}

static void can_bee_rx_complete(const struct device *dev, int mb_list_idx)
{
	struct can_bee_data *data = dev->data;
	struct can_bee_mb_rx_data *mb = &data->rx_mb_list[mb_list_idx];
	CANMsgBufInfo_TypeDef mb_info;
	struct can_frame frame = {0};
	can_rx_callback_t callback;
	void *user_data;

	callback = mb->rx_callback;
	user_data = mb->user_data;

	if (!mb->is_busy || callback == NULL) {
		return;
	}

	CAN_GetMsgBufInfo(mb->idx, &mb_info);
	CAN_GetRamData(mb_info.data_length, frame.data);

	frame.dlc = mb_info.data_length;
	frame.flags =
		(mb_info.rtr_bit ? CAN_FRAME_RTR : 0U) | (mb_info.ide_bit ? CAN_FRAME_IDE : 0U);
	if (mb_info.ide_bit != 0U) {
		frame.id = (mb_info.standard_frame_id << CAN_STD_FRAME_ID_POS) |
			   mb_info.extend_frame_id;
	} else {
		frame.id = mb_info.standard_frame_id;
	}

#if CONFIG_CAN_RX_TIMESTAMP
	frame.timestamp = mb_info.rx_timestamp;
#endif

	callback(dev, &frame, user_data);
}

static int can_bee_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT;

	return 0;
}

static int can_bee_get_state(const struct device *dev, enum can_state *state,
			     struct can_bus_err_cnt *err_cnt)
{
	struct can_bee_data *data = dev->data;

	if (state != NULL) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else if (CAN_GetBusState() != CAN_BUS_STATE_ON) {
			*state = CAN_STATE_BUS_OFF;
		} else if (CAN_GetErrorPassiveStatus()) {
			*state = CAN_STATE_ERROR_PASSIVE;
		} else if (CAN_GetErrorWarningStatus()) {
			*state = CAN_STATE_ERROR_WARNING;
		} else {
			*state = CAN_STATE_ERROR_ACTIVE;
		}
	}

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt = CAN_GetTxErrorCnt();
		err_cnt->rx_err_cnt = CAN_GetRxErrorCnt();
	}

	return 0;
}

static void can_bee_update_state(const struct device *dev)
{
	struct can_bee_data *data = dev->data;
	struct can_bus_err_cnt err_cnt;
	enum can_state state;
	can_state_change_callback_t cb;
	void *user_data;

	can_bee_get_state(dev, &state, &err_cnt);

	if (state == data->state) {
		return;
	}

	data->state = state;

	cb = data->common.state_change_cb;
	user_data = data->common.state_change_cb_user_data;

	if (cb != NULL) {
		cb(dev, state, err_cnt, user_data);
	}
}

static int can_bee_start(const struct device *dev)
{
	const struct can_bee_config *cfg = dev->config;
	struct can_bee_data *data = dev->data;
	int ret = 0;

	CAN_STATS_RESET(dev);

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (data->common.started) {
		ret = -EALREADY;
		goto unlock;
	}

	if (cfg->common.phy != NULL) {
		ret = can_transceiver_enable(cfg->common.phy, true);
		if (ret != 0) {
			LOG_ERR("Failed to enable CAN transceiver (err %d)", ret);
			goto unlock;
		}
	}

	CAN_Cmd(ENABLE);

	ret = can_bee_check_bus_on(100000);
	if (ret < 0) {
		LOG_ERR("CAN bus off");
		ret = -EIO;
		goto disable_ctrl;
	}

	for (uint8_t i = 0; i < CONFIG_CAN_BEE_RX_MSG_BUF_NUM; i++) {
		if (!data->rx_mb_list[i].is_busy) {
			continue;
		}

		CAN_SetMsgBufRxMode(&data->rx_mb_list[i].rx_frame_type);
		can_bee_wait_ram_state();

		CAN_MBRxINTConfig(data->rx_mb_list[i].rx_frame_type.msg_buf_id, ENABLE);
	}

	CAN_INTConfig((CAN_BUS_OFF_INT | CAN_ERROR_INT | CAN_RX_INT | CAN_TX_INT), ENABLE);

	data->common.started = true;

	goto unlock;

disable_ctrl:
	CAN_Cmd(DISABLE);

	if (cfg->common.phy != NULL) {
		(void)can_transceiver_disable(cfg->common.phy);
	}

unlock:
	k_mutex_unlock(&data->inst_mutex);

	can_bee_update_state(dev);

	return ret;
}

static int can_bee_stop(const struct device *dev)
{
	const struct can_bee_config *cfg = dev->config;
	struct can_bee_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (!data->common.started) {
		ret = -EALREADY;
		goto unlock;
	}

	CAN_INTConfig((CAN_BUS_OFF_INT | CAN_ERROR_INT | CAN_RX_INT | CAN_TX_INT), DISABLE);

	for (uint8_t i = 0; i < CONFIG_CAN_BEE_RX_MSG_BUF_NUM; i++) {
		CANRxFrame_TypeDef rx_frame_type = {0};

		if (!data->rx_mb_list[i].is_busy) {
			continue;
		}

		rx_frame_type.msg_buf_id = data->rx_mb_list[i].idx;
		rx_frame_type.rx_msg_buf_enable = false;

		CAN_SetMsgBufRxMode(&rx_frame_type);
		can_bee_wait_ram_state();

		CAN_MBRxINTConfig(data->rx_mb_list[i].idx, DISABLE);
	}

	CAN_Cmd(DISABLE);

	if (cfg->common.phy != NULL) {
		ret = can_transceiver_disable(cfg->common.phy);
		if (ret != 0) {
			LOG_ERR("Failed to disable CAN transceiver (err %d)", ret);
			goto unlock;
		}
	}

	data->common.started = false;

unlock:
	k_mutex_unlock(&data->inst_mutex);

	can_bee_update_state(dev);

	return ret;
}

static int can_bee_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_bee_data *data = dev->data;
	can_mode_t supported;
	CAN_InitTypeDef *init_struct = &data->init_struct;
	int ret = 0;

	LOG_DBG("[%s] dev %s mode 0x%x", __func__, dev->name, mode);

	(void)can_bee_get_capabilities(dev, &supported);

	if ((mode & ~supported) != 0U) {
		LOG_ERR("Unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (data->common.started) {
		ret = -EBUSY;
		goto unlock;
	}

	if ((mode & CAN_MODE_LOOPBACK) != 0U) {
		init_struct->CAN_TestModeSel = CAN_TEST_MODE_INT_LOOPBACK;
	} else if ((mode & CAN_MODE_LISTENONLY) != 0U) {
		init_struct->CAN_TestModeSel = CAN_TEST_MODE_SILENCE;
	} else {
		init_struct->CAN_TestModeSel = CAN_TEST_MODE_NONE;
	}

	CAN_SetTestMode(init_struct->CAN_TestModeSel);

	init_struct->CAN_AutoReTxEn = ((mode & CAN_MODE_ONE_SHOT) == 0U);
	CAN_AutoReTxCmd(init_struct->CAN_AutoReTxEn);

	data->common.mode = mode;

unlock:
	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_bee_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_bee_data *data = dev->data;
	CAN_InitTypeDef *init_struct = &data->init_struct;
	int ret = 0;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (data->common.started) {
		ret = -EBUSY;
		goto unlock;
	}

	init_struct->CAN_BitTiming.b.can_brp = timing->prescaler - 1U;
	init_struct->CAN_BitTiming.b.can_sjw = timing->sjw;
	init_struct->CAN_BitTiming.b.can_tseg1 = timing->phase_seg1 - 1U;
	init_struct->CAN_BitTiming.b.can_tseg2 = timing->phase_seg2 - 1U;

	CAN_SetTiming(&init_struct->CAN_BitTiming);

unlock:
	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_bee_request_message_buffer_locked(const struct device *dev, bool is_tx)
{
	struct can_bee_data *data = dev->data;
	uint8_t mb_num;
	uint8_t i;

	if (is_tx) {
		mb_num = CONFIG_CAN_BEE_TX_MSG_BUF_NUM;
		for (i = 0; i < mb_num; i++) {
			if (!data->tx_mb_list[i].is_busy) {
				return i;
			}
		}
	} else {
		mb_num = CONFIG_CAN_BEE_RX_MSG_BUF_NUM;
		for (i = 0; i < mb_num; i++) {
			if (!data->rx_mb_list[i].is_busy) {
				return i;
			}
		}
	}

	return -EAGAIN;
}

static int can_bee_send(const struct device *dev, const struct can_frame *frame,
			k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	struct can_bee_data *data = dev->data;
	struct can_bee_mb_tx_data *mb;
	CANTxFrame_TypeDef tx_frame_type;
	CANError_TypeDef tx_error;
	int mb_list_idx;
	int ret = 0;

	LOG_DBG("sending %d bytes %s frame on %s: %s id: 0x%x", frame->dlc,
		(frame->flags & CAN_FRAME_RTR) ? "remote" : "data", dev->name,
		(frame->flags & CAN_FRAME_IDE) ? "extended" : "standard", frame->id);

	if ((frame->flags & CAN_FRAME_IDE) != 0U) {
		if ((frame->id & ~CAN_EXT_ID_MASK) != 0U) {
			return -EINVAL;
		}
	} else if ((frame->id & ~CAN_STD_ID_MASK) != 0U) {
		return -EINVAL;
	}

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC (%d) exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0U) {
		LOG_ERR("Unsupported CAN frame flags (0x%x)", frame->flags);
		return -ENOTSUP;
	}

	if (k_sem_take(&data->tx_int_sem, timeout) != 0) {
		LOG_DBG("Failed to acquire TX mailbox semaphore");
		return -EAGAIN;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (!data->common.started) {
		LOG_ERR("CAN controller is not started");
		ret = -ENETDOWN;
		goto fail;
	}

	if (CAN_GetBusState() != CAN_BUS_STATE_ON) {
		LOG_ERR("CAN bus off");
		ret = -ENETUNREACH;
		goto fail;
	}

	mb_list_idx = can_bee_request_message_buffer_locked(dev, true);
	if (mb_list_idx < 0) {
		LOG_ERR("There is no free CAN TX message buffer");
		ret = -EAGAIN;
		goto fail;
	}

	mb = &data->tx_mb_list[mb_list_idx];

	if ((frame->flags & CAN_FRAME_IDE) != 0U) {
		tx_frame_type.frame_type =
			(frame->flags & CAN_FRAME_RTR) ? CAN_EXT_REMOTE_FRAME : CAN_EXT_DATA_FRAME;
		tx_frame_type.standard_frame_id =
			(frame->id >> CAN_STD_FRAME_ID_POS) & CAN_STAND_FRAME_ID_MAX_VALUE;
		tx_frame_type.extend_frame_id = frame->id & CAN_EXTEND_FRAME_ID_MAX_VALUE;
	} else {
		tx_frame_type.frame_type =
			(frame->flags & CAN_FRAME_RTR) ? CAN_STD_REMOTE_FRAME : CAN_STD_DATA_FRAME;
		tx_frame_type.standard_frame_id = frame->id & CAN_STAND_FRAME_ID_MAX_VALUE;
		tx_frame_type.extend_frame_id = 0;
	}

	tx_frame_type.msg_buf_id = mb->idx;
	tx_frame_type.auto_reply_bit = DISABLE;

	tx_error = CAN_SetMsgBufTxMode(&tx_frame_type, frame->data, frame->dlc);
	can_bee_wait_ram_state();

	if (tx_error != CAN_NO_ERR) {
		LOG_ERR("Failed to configure CAN TX mailbox");
		ret = -EIO;
		goto fail;
	}

	mb->tx_callback = callback;
	mb->user_data = user_data;
	mb->status = 0;
	mb->is_busy = true;

	CAN_MBTxINTConfig(tx_frame_type.msg_buf_id, ENABLE);

	k_mutex_unlock(&data->inst_mutex);

	return 0;

fail:

	k_mutex_unlock(&data->inst_mutex);
	k_sem_give(&data->tx_int_sem);

	return ret;
}

static int can_bee_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
				 void *user_data, const struct can_filter *filter)
{
	struct can_bee_data *data = dev->data;
	CANRxFrame_TypeDef rx_frame_type = {0};
	CANError_TypeDef rx_error;
	int mb_list_idx;

	LOG_DBG("[%s] filter->id=0x%x, filter->flags=0x%x, filter->mask=0x%x", __func__, filter->id,
		filter->flags, filter->mask);

	if (callback == NULL) {
		return -EINVAL;
	}

	if ((filter->flags & ~CAN_FILTER_IDE) != 0U) {
		LOG_ERR("Unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	if ((filter->flags & CAN_FILTER_IDE) != 0U) {
		if ((filter->id & ~CAN_EXT_ID_MASK) != 0U ||
		    (filter->mask & ~CAN_EXT_ID_MASK) != 0U) {
			return -EINVAL;
		}
	} else {
		if ((filter->id & ~CAN_STD_ID_MASK) != 0U ||
		    (filter->mask & ~CAN_STD_ID_MASK) != 0U) {
			return -EINVAL;
		}
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	mb_list_idx = can_bee_request_message_buffer_locked(dev, false);
	if (mb_list_idx < 0) {
		LOG_ERR("There is no free CAN RX filter");
		mb_list_idx = -ENOSPC;
		goto unlock;
	}

	rx_frame_type.msg_buf_id = data->rx_mb_list[mb_list_idx].idx;
	rx_frame_type.frame_ide_mask = 0;
	rx_frame_type.frame_ide_bit = (filter->flags & CAN_FILTER_IDE) ? 1 : 0;
	rx_frame_type.frame_rtr_mask = IS_ENABLED(CONFIG_CAN_ACCEPT_RTR);
	rx_frame_type.frame_rtr_bit = 0;

	if (rx_frame_type.frame_ide_bit != 0U) {
		rx_frame_type.standard_frame_id =
			(filter->id >> CAN_STD_FRAME_ID_POS) & CAN_STAND_FRAME_ID_MAX_VALUE;
		rx_frame_type.extend_frame_id = filter->id & CAN_EXTEND_FRAME_ID_MAX_VALUE;
		rx_frame_type.frame_id_mask = ~(filter->mask & CAN_FRAME_ID_MASK_MAX_VALUE);
	} else {
		rx_frame_type.standard_frame_id = filter->id & CAN_STAND_FRAME_ID_MAX_VALUE;
		rx_frame_type.extend_frame_id = 0;
		rx_frame_type.frame_id_mask =
			~((filter->mask & CAN_STAND_FRAME_ID_MAX_VALUE) << CAN_STD_FRAME_ID_POS);
	}

	rx_frame_type.rx_dma_en = false;
	rx_frame_type.auto_reply_bit = false;
	rx_frame_type.rx_msg_buf_enable = true;

	if (data->common.started) {
		rx_error = CAN_SetMsgBufRxMode(&rx_frame_type);
		can_bee_wait_ram_state();

		if (rx_error != CAN_NO_ERR) {
			mb_list_idx = -EIO;
			goto unlock;
		}

		CAN_MBRxINTConfig(rx_frame_type.msg_buf_id, ENABLE);
	}

	data->rx_mb_list[mb_list_idx].rx_frame_type = rx_frame_type;
	data->rx_mb_list[mb_list_idx].rx_callback = callback;
	data->rx_mb_list[mb_list_idx].user_data = user_data;
	data->rx_mb_list[mb_list_idx].is_busy = true;

unlock:
	k_mutex_unlock(&data->inst_mutex);

	return mb_list_idx;
}

static void can_bee_remove_rx_filter(const struct device *dev, int mb_list_idx)
{
	struct can_bee_data *data = dev->data;
	CANRxFrame_TypeDef rx_frame_type = {0};

	if (mb_list_idx < 0 || mb_list_idx >= CONFIG_CAN_BEE_RX_MSG_BUF_NUM) {
		return;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (!data->rx_mb_list[mb_list_idx].is_busy) {
		goto unlock;
	}

	rx_frame_type.msg_buf_id = data->rx_mb_list[mb_list_idx].idx;
	rx_frame_type.rx_msg_buf_enable = false;
	if (data->common.started) {
		CAN_SetMsgBufRxMode(&rx_frame_type);
		can_bee_wait_ram_state();

		CAN_MBRxINTConfig(data->rx_mb_list[mb_list_idx].idx, DISABLE);
	}

	data->rx_mb_list[mb_list_idx].rx_callback = NULL;
	data->rx_mb_list[mb_list_idx].user_data = NULL;
	data->rx_mb_list[mb_list_idx].is_busy = false;
	memset(&data->rx_mb_list[mb_list_idx].rx_frame_type, 0,
	       sizeof(data->rx_mb_list[mb_list_idx].rx_frame_type));

unlock:
	k_mutex_unlock(&data->inst_mutex);
}

static void can_bee_set_state_change_callback(const struct device *dev,
					      can_state_change_callback_t cb, void *user_data)
{
	struct can_bee_data *data = dev->data;
	unsigned int key = irq_lock();

	data->common.state_change_cb = cb;
	data->common.state_change_cb_user_data = user_data;
	irq_unlock(key);
}

static int can_bee_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	*rate = CAN_SRC_CLOCK_HZ;

	return 0;
}

static int can_bee_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CONFIG_CAN_BEE_RX_MSG_BUF_NUM;
}

#ifdef CONFIG_CAN_STATS
static inline void can_bee_handle_error_stats(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (CAN_GetErrorStatus(CAN_ERROR_ACK)) {
		CAN_CLearErrorStatus(CAN_ERROR_ACK);
		CAN_STATS_ACK_ERROR_INC(dev);
	}

	if (CAN_GetErrorStatus(CAN_ERROR_STUFF)) {
		CAN_CLearErrorStatus(CAN_ERROR_STUFF);
		CAN_STATS_STUFF_ERROR_INC(dev);
	}

	if (CAN_GetErrorStatus(CAN_ERROR_CRC)) {
		CAN_CLearErrorStatus(CAN_ERROR_CRC);
		CAN_STATS_CRC_ERROR_INC(dev);
	}

	if (CAN_GetErrorStatus(CAN_ERROR_FORM)) {
		CAN_CLearErrorStatus(CAN_ERROR_FORM);
		CAN_STATS_FORM_ERROR_INC(dev);
	}

	if (CAN_GetErrorStatus(CAN_ERROR_BIT1)) {
		CAN_CLearErrorStatus(CAN_ERROR_BIT1);
		CAN_STATS_BIT1_ERROR_INC(dev);
	}

	if (CAN_GetErrorStatus(CAN_ERROR_BIT0)) {
		CAN_CLearErrorStatus(CAN_ERROR_BIT0);
		CAN_STATS_BIT0_ERROR_INC(dev);
	}
}
#endif

static inline void can_bee_handle_bus_off(const struct device *dev)
{
	struct can_bee_data *data = dev->data;

	CAN_ClearINTPendingBit(CAN_BUS_OFF_INT_FLAG);

	for (uint8_t mb_list_idx = 0; mb_list_idx < CONFIG_CAN_BEE_TX_MSG_BUF_NUM; mb_list_idx++) {
		data->tx_mb_list[mb_list_idx].status = -ENETUNREACH;
		can_bee_tx_complete_with_status(dev, mb_list_idx,
						data->tx_mb_list[mb_list_idx].status);
	}

	can_bee_update_state(dev);
}

static inline void can_bee_handle_tx_error(const struct device *dev)
{
	struct can_bee_data *data = dev->data;

	if (CAN_GetErrorStatus(CAN_ERROR_TX)) {
		CAN_CLearErrorStatus(CAN_ERROR_TX);

		for (uint8_t mb_list_idx = 0; mb_list_idx < CONFIG_CAN_BEE_TX_MSG_BUF_NUM;
		     mb_list_idx++) {
			if (CAN_GetMBnTxErrorFlag(data->tx_mb_list[mb_list_idx].idx)) {
				CAN_ClearMBnTxErrorFlag(data->tx_mb_list[mb_list_idx].idx);
				data->tx_mb_list[mb_list_idx].status = -EIO;
				can_bee_tx_complete_with_status(
					dev, mb_list_idx, data->tx_mb_list[mb_list_idx].status);
			}
		}
	}
}

static inline void can_bee_handle_error(const struct device *dev)
{
	CAN_ClearINTPendingBit(CAN_ERROR_INT_FLAG);

	if (CAN_GetErrorStatus(CAN_ERROR_RX)) {
		CAN_CLearErrorStatus(CAN_ERROR_RX);
	}

	can_bee_handle_tx_error(dev);

#ifdef CONFIG_CAN_STATS
	can_bee_handle_error_stats(dev);
#endif

	can_bee_update_state(dev);
}

static inline void can_bee_handle_rx(const struct device *dev)
{
	struct can_bee_data *data = dev->data;

	CAN_ClearINTPendingBit(CAN_RX_INT_FLAG);

	for (uint8_t mb_list_idx = 0; mb_list_idx < CONFIG_CAN_BEE_RX_MSG_BUF_NUM; mb_list_idx++) {
		if (CAN_GetMBnRxDoneFlag(data->rx_mb_list[mb_list_idx].idx)) {
			CAN_ClearMBnRxDoneFlag(data->rx_mb_list[mb_list_idx].idx);
			can_bee_rx_complete(dev, mb_list_idx);
		}
	}
}

static inline void can_bee_handle_tx(const struct device *dev)
{
	struct can_bee_data *data = dev->data;

	CAN_ClearINTPendingBit(CAN_TX_INT_FLAG);

	for (uint8_t mb_list_idx = 0; mb_list_idx < CONFIG_CAN_BEE_TX_MSG_BUF_NUM; mb_list_idx++) {
		if (CAN_GetMBnTxDoneFlag(data->tx_mb_list[mb_list_idx].idx)) {
			CAN_ClearMBnTxDoneFlag(data->tx_mb_list[mb_list_idx].idx);
			can_bee_tx_complete_with_status(dev, mb_list_idx,
							data->tx_mb_list[mb_list_idx].status);
		}
	}
}

static inline void can_bee_isr(const struct device *dev)
{
	if (CAN_GetINTStatus(CAN_BUS_OFF_INT_FLAG)) {
		can_bee_handle_bus_off(dev);
	}

	if (CAN_GetINTStatus(CAN_ERROR_INT_FLAG)) {
		can_bee_handle_error(dev);
	}

	if (CAN_GetINTStatus(CAN_RX_INT_FLAG)) {
		can_bee_handle_rx(dev);
	}

	if (CAN_GetINTStatus(CAN_TX_INT_FLAG)) {
		can_bee_handle_tx(dev);
	}
}

static int can_bee_init(const struct device *dev)
{
	const struct can_bee_config *cfg = dev->config;
	struct can_bee_data *data = dev->data;
	struct can_timing timing = {0};
	CAN_InitTypeDef *init_struct = &data->init_struct;
	int ret;

	k_mutex_init(&data->inst_mutex);
	k_sem_init(&data->tx_int_sem, CONFIG_CAN_BEE_TX_MSG_BUF_NUM, CONFIG_CAN_BEE_TX_MSG_BUF_NUM);

	for (uint8_t i = 0; i < CONFIG_CAN_BEE_TX_MSG_BUF_NUM; i++) {
		data->tx_mb_list[i].idx = i;
	}

	for (uint8_t i = 0; i < CONFIG_CAN_BEE_RX_MSG_BUF_NUM; i++) {
		data->rx_mb_list[i].idx = i + CONFIG_CAN_BEE_TX_MSG_BUF_NUM;
	}

	data->common.mode = CAN_MODE_NORMAL;
	data->state = CAN_STATE_STOPPED;

	if (cfg->common.phy != NULL && !device_is_ready(cfg->common.phy)) {
		LOG_ERR("CAN transceiver not ready");
		return -ENODEV;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);

	ret = can_calc_timing(dev, &timing, cfg->common.bitrate, cfg->common.sample_point);
	if (ret == -EINVAL) {
		LOG_ERR("Can't find timing for given parameters");
		return -EIO;
	}

	LOG_DBG("Presc: %d, SJW: %d, TS1: %d, TS2: %d", timing.prescaler, timing.sjw,
		timing.phase_seg1, timing.phase_seg2);

	CAN_StructInit(init_struct);
	init_struct->CAN_AutoReTxEn = ENABLE;
	init_struct->CAN_ErrorWarnThd = 128;

#if CONFIG_CAN_RX_TIMESTAMP
	init_struct->CAN_TimeStamp.b.can_time_stamp_en = ENABLE;
	init_struct->CAN_TimeStamp.b.can_time_stamp_div = cfg->rx_timestamp_prescaler - 1U;
#endif

	init_struct->CAN_BitTiming.b.can_brp = timing.prescaler - 1U;
	init_struct->CAN_BitTiming.b.can_sjw = timing.sjw;
	init_struct->CAN_BitTiming.b.can_tseg1 = timing.phase_seg1 - 1U;
	init_struct->CAN_BitTiming.b.can_tseg2 = timing.phase_seg2 - 1U;

	CAN_Init(init_struct);

	cfg->irq_config_func();

	return 0;
}

static DEVICE_API(can, can_bee_driver_api) = {
	.get_capabilities = can_bee_get_capabilities,
	.start = can_bee_start,
	.stop = can_bee_stop,
	.set_mode = can_bee_set_mode,
	.set_timing = can_bee_set_timing,
	.send = can_bee_send,
	.add_rx_filter = can_bee_add_rx_filter,
	.remove_rx_filter = can_bee_remove_rx_filter,
	.get_state = can_bee_get_state,
	.set_state_change_callback = can_bee_set_state_change_callback,
	.get_core_clock = can_bee_get_core_clock,
	.get_max_filters = can_bee_get_max_filters,
	/* clang-format off */
	.timing_min = {
			.sjw = 0x1,
			.prop_seg = 0x00,
			.phase_seg1 = 0x01,
			.phase_seg2 = 0x01,
			.prescaler = 0x01,
		},
	.timing_max = {
			.sjw = 0x08,
			.prop_seg = 0x00,
			.phase_seg1 = 0x100,
			.phase_seg2 = 0x100,
			.prescaler = 0x100,
		},
	/* clang-format on */
};

#define CAN_BEE_IRQ_INST(index)                                                                    \
	static void config_can_##index##_irq(void)                                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), can_bee_isr,        \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#define CAN_BEE_CONFIG_INST(index)                                                                 \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	static const struct can_bee_config can_bee_cfg_##index = {                                 \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(                                           \
			index, 0, DT_INST_CAN_TRANSCEIVER_MAX_BITRATE(index, 1000000)),            \
		.can = (CAN_TypeDef *)DT_INST_REG_ADDR(index),                                     \
		.clkid = DT_INST_CLOCKS_CELL(index, id),                                           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.irq_config_func = config_can_##index##_irq,                                       \
		.rx_timestamp_prescaler = DT_INST_PROP_OR(index, rx_timestamp_prescaler, 1),       \
	};

#define CAN_BEE_DATA_INST(index) static struct can_bee_data can_bee_dev_data_##index;

#define CAN_BEE_DEFINE_INST(index)                                                                 \
	CAN_DEVICE_DT_INST_DEFINE(index, can_bee_init, NULL, &can_bee_dev_data_##index,            \
				  &can_bee_cfg_##index, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,     \
				  &can_bee_driver_api);

#define BEE_CAN_INIT(index)                                                                        \
	CAN_BEE_IRQ_INST(index)                                                                    \
	CAN_BEE_CONFIG_INST(index)                                                                 \
	CAN_BEE_DATA_INST(index)                                                                   \
	CAN_BEE_DEFINE_INST(index)

DT_INST_FOREACH_STATUS_OKAY(BEE_CAN_INIT)
