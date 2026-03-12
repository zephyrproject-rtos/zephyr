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

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_can.h>
#else
#error "Unsupported Realtek Bee SoC series"
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_bee, CONFIG_CAN_LOG_LEVEL);
struct can_bee_mb_data {
	uint8_t idx;
	bool is_busy;
	int status;
	union {
		can_tx_callback_t tx_callback;
		can_rx_callback_t rx_callback;
	};
	void *user_data;
};

struct can_bee_data {
	struct can_driver_data common;
	struct k_mutex inst_mutex;
	struct k_sem tx_int_sem;
	CAN_InitTypeDef init_struct;
	struct can_bee_mb_data tx_mb_list[CONFIG_CAN_BEE_TX_MSG_BUF_NUM];
	struct can_bee_mb_data rx_mb_list[CONFIG_CAN_BEE_RX_MSG_BUF_NUM];
	enum can_state state;
};

struct can_bee_config {
	const struct can_driver_config common;
	CAN_TypeDef *can;
	const struct pinctrl_dev_config *pcfg;
	uint16_t clkid;
	void (*irq_config_func)();
};

static int can_bee_wait_ram_state(k_timeout_t timeout)
{
	int64_t start_time;

	start_time = k_uptime_ticks();
	while (CAN_GetRamState() != CAN_RAM_STATE_IDLE) {
		if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
		    k_uptime_ticks() - start_time >= timeout.ticks) {
			return -EAGAIN;
		}
	}

	return 0;
}

static int can_bee_check_bus_on(k_timeout_t timeout)
{
	int64_t start_time;

	start_time = k_uptime_ticks();
	while (CAN_GetBusState() != CAN_BUS_STATE_ON) {
		if (!K_TIMEOUT_EQ(timeout, K_FOREVER) &&
		    k_uptime_ticks() - start_time >= timeout.ticks) {
			return -EAGAIN;
		}
	}

	return 0;
}

static void can_bee_tx_complete(const struct device *dev, int mb_list_idx)
{
	struct can_bee_data *data = dev->data;
	can_tx_callback_t callback = data->tx_mb_list[mb_list_idx].tx_callback;

	if (data->tx_mb_list[mb_list_idx].is_busy) {
		data->tx_mb_list[mb_list_idx].is_busy = false;
		k_sem_give(&data->tx_int_sem);
		if (callback != NULL) {
			data->tx_mb_list[mb_list_idx].tx_callback = NULL;
			callback(dev, data->tx_mb_list[mb_list_idx].status,
				 data->tx_mb_list[mb_list_idx].user_data);
		}
		data->tx_mb_list[mb_list_idx].status = 0;
	}
}

static void can_bee_rx_complete(const struct device *dev, int mb_list_idx)
{
	struct can_bee_data *data = dev->data;
	can_rx_callback_t callback = NULL;
	CANMsgBufInfo_TypeDef mb_info;
	CANDataFrameSel_TypeDef frame_type;
	struct can_frame frame;

	CAN_GetMsgBufInfo(data->rx_mb_list[mb_list_idx].idx, &mb_info);
	memset(frame.data, 0, 8);
	CAN_GetRamData(mb_info.data_length, frame.data);
	frame_type = CAN_CheckFrameType(mb_info.rtr_bit, mb_info.ide_bit);

	frame.dlc = mb_info.data_length;
	frame.flags = (mb_info.rtr_bit ? CAN_FRAME_RTR : 0) | (mb_info.ide_bit ? CAN_FRAME_IDE : 0);
	frame.id = mb_info.ide_bit ? ((mb_info.standard_frame_id << CAN_STD_FRAME_ID_POS) |
				      mb_info.extend_frame_id)
				   : mb_info.standard_frame_id;
	callback = data->rx_mb_list[mb_list_idx].rx_callback;
	if (callback) {
		callback(dev, &frame, data->rx_mb_list[mb_list_idx].user_data);
	}
}

static int can_bee_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT;

	if (IS_ENABLED(CONFIG_CAN_MANUAL_RECOVERY_MODE)) {
		*cap |= CAN_MODE_MANUAL_RECOVERY;
	}

	return 0;
}

static int can_bee_start(const struct device *dev)
{
	const struct can_bee_config *cfg = dev->config;
	struct can_bee_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (data->common.started) {
		ret = -EALREADY;
		goto unlock;
	}

	if (cfg->common.phy != NULL) {
		ret = can_transceiver_enable(cfg->common.phy, true);
		if (ret != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", ret);
			goto unlock;
		}
	}

	CAN_Cmd(ENABLE);

	ret = can_bee_check_bus_on(K_MSEC(10));
	if (ret < 0) {
		LOG_ERR("CAN bus off");
		CAN_Cmd(DISABLE);
		if (cfg->common.phy != NULL) {
			/* Attempt to disable the CAN transceiver in case of error */
			(void)can_transceiver_disable(cfg->common.phy);
		}
		ret = -EIO;
		goto unlock;
	}

	CAN_INTConfig((CAN_BUS_OFF_INT | CAN_ERROR_INT | CAN_RX_INT | CAN_TX_INT), ENABLE);
	data->common.started = true;

unlock:
	k_mutex_unlock(&data->inst_mutex);

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

	for (uint8_t i = 0; i < CONFIG_CAN_BEE_RX_MSG_BUF_NUM; i++) {
		data->rx_mb_list[i].is_busy = false;
	}
	for (uint8_t i = 0; i < CONFIG_CAN_BEE_TX_MSG_BUF_NUM; i++) {
		data->tx_mb_list[i].is_busy = false;
	}

	if (cfg->common.phy != NULL) {
		ret = can_transceiver_disable(cfg->common.phy);
		if (ret != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", ret);
			goto unlock;
		}
	}

	data->common.started = false;

unlock:
	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_bee_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_bee_data *data = dev->data;
	can_mode_t supported = CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY | CAN_MODE_ONE_SHOT;
	CAN_InitTypeDef *init_struct = &(data->init_struct);

	LOG_DBG("[%s] dev %s mode 0x%x", __func__, dev->name, mode);

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

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if ((mode & CAN_MODE_MANUAL_RECOVERY) != 0) {
		data->common.mode |= CAN_MODE_MANUAL_RECOVERY;
	}

	if ((mode & CAN_MODE_LOOPBACK) != 0) {
		init_struct->CAN_TestModeSel = CAN_TEST_MODE_INT_LOOPBACK;
	} else if ((mode & CAN_MODE_LISTENONLY) != 0) {
		init_struct->CAN_TestModeSel = CAN_TEST_MODE_SILENCE;
	} else {
		init_struct->CAN_TestModeSel = CAN_TEST_MODE_NONE;
	}

	CAN_SetTestMode(init_struct->CAN_TestModeSel);

	if ((mode & CAN_MODE_ONE_SHOT) != 0) {
		init_struct->CAN_AutoReTxEn = false;
	} else {
		init_struct->CAN_AutoReTxEn = true;
	}

	CAN_AutoReTxCmd(init_struct->CAN_AutoReTxEn);

	data->common.mode = mode;

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static int can_bee_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_bee_data *data = dev->data;
	CAN_InitTypeDef *init_struct = &(data->init_struct);

	if (data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	init_struct->CAN_BitTiming.b.can_brp = timing->prescaler - 1;
	init_struct->CAN_BitTiming.b.can_sjw = timing->sjw;
	init_struct->CAN_BitTiming.b.can_tseg1 = timing->phase_seg1 - 1;
	init_struct->CAN_BitTiming.b.can_tseg2 = timing->phase_seg2 - 1;

	CAN_SetTiming(&(init_struct->CAN_BitTiming));

	k_mutex_unlock(&data->inst_mutex);

	return 0;
}

static int can_bee_request_message_buffer(const struct device *dev, bool is_tx)
{
	struct can_bee_data *data = dev->data;
	uint8_t mb_num;
	struct can_bee_mb_data *mb_list;

	mb_num = is_tx ? CONFIG_CAN_BEE_TX_MSG_BUF_NUM : CONFIG_CAN_BEE_RX_MSG_BUF_NUM;
	mb_list = is_tx ? data->tx_mb_list : data->rx_mb_list;

	for (uint8_t i = 0; i < mb_num; i++) {
		if (!mb_list[i].is_busy) {
			return i;
		}
	}

	return -EAGAIN;
}

static int can_bee_send(const struct device *dev, const struct can_frame *frame,
			k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	const struct can_bee_config *cfg = dev->config;
	struct can_bee_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;
	CANTxFrame_TypeDef tx_frame_type;
	CANError_TypeDef tx_error;
	int mb_list_idx;
	int mb_idx;
	int ret;

	LOG_DBG("Sending %d bytes %s frame on %s: %s id: 0x%x", frame->dlc,
		(frame->flags & CAN_FRAME_RTR) != 0 ? "remote" : "data", dev->name,
		(frame->flags & CAN_FRAME_IDE) != 0 ? "extended" : "standard", frame->id);

	if (!callback) {
		LOG_ERR("No valid callback");
		return -EINVAL;
	}

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC (%d) exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0) {
		LOG_ERR("Unsupported CAN frame flags (0x%x)", frame->flags);
		return -ENOTSUP;
	}

	if (!data->common.started) {
		LOG_ERR("CAN not start");
		return -ENETDOWN;
	}

	if (CAN_GetBusState(can) != CAN_BUS_STATE_ON) {
		LOG_ERR("CAN buss off");
		return -ENETUNREACH;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	if (k_sem_take(&data->tx_int_sem, timeout)) {
		k_mutex_unlock(&data->inst_mutex);
		LOG_ERR("Take CAN tx message buffer sem fail");
		return -EAGAIN;
	}

	ret = 0;
	mb_list_idx = can_bee_request_message_buffer(dev, true);
	if (mb_list_idx < 0) {
		ret = -EAGAIN;
		LOG_ERR("There is no free CAN tx message buffer");
		goto fail;
	}

	mb_idx = data->tx_mb_list[mb_list_idx].idx;

	if (frame->flags & CAN_FRAME_IDE) {
		if (frame->flags & CAN_FRAME_RTR) {
			tx_frame_type.frame_type = CAN_EXT_REMOTE_FRAME;
		} else {
			tx_frame_type.frame_type = CAN_EXT_DATA_FRAME;
		}
		tx_frame_type.standard_frame_id =
			(frame->id >> CAN_STD_FRAME_ID_POS) & CAN_STAND_FRAME_ID_MAX_VALUE;
		tx_frame_type.extend_frame_id = frame->id & CAN_EXTEND_FRAME_ID_MAX_VALUE;
	} else {
		if (frame->flags & CAN_FRAME_RTR) {
			tx_frame_type.frame_type = CAN_STD_REMOTE_FRAME;
		} else {
			tx_frame_type.frame_type = CAN_STD_DATA_FRAME;
		}
		tx_frame_type.standard_frame_id = frame->id & CAN_STAND_FRAME_ID_MAX_VALUE;
		tx_frame_type.extend_frame_id = 0;
	}

	tx_frame_type.msg_buf_id = mb_idx;
	tx_frame_type.auto_reply_bit = DISABLE;
	tx_error = CAN_SetMsgBufTxMode(&tx_frame_type, frame->data, frame->dlc);

	ret = can_bee_wait_ram_state(K_MSEC(10));
	if (ret < 0) {
		LOG_ERR("Wait CAN RAM state timeout");
		ret = -EIO;
		goto fail;
	}

	if (tx_error != CAN_NO_ERR) {
		ret = -EIO;
		goto fail;
	}

	data->tx_mb_list[mb_list_idx].tx_callback = callback;
	data->tx_mb_list[mb_list_idx].user_data = user_data;
	data->tx_mb_list[mb_list_idx].is_busy = true;
	CAN_MBTxINTConfig(tx_frame_type.msg_buf_id, ENABLE);
fail:
	if (ret < 0) {
		LOG_ERR("CAN send fail ret%d", ret);
		k_sem_give(&data->tx_int_sem);
	}

	k_mutex_unlock(&data->inst_mutex);

	return ret;
}

static int can_bee_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
				 void *user_data, const struct can_filter *filter)
{
	struct can_bee_data *data = dev->data;
	CANRxFrame_TypeDef rx_frame_type;
	CANError_TypeDef rx_error;
	int mb_list_idx;
	int ret;

	LOG_DBG("[%s] filter->id=0x%x, filter->flags=0x%x, filter->mask=0x%x", __func__, filter->id,
		filter->flags, filter->mask);

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0) {
		LOG_ERR("Unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->inst_mutex, K_FOREVER);

	mb_list_idx = can_bee_request_message_buffer(dev, false);
	if (mb_list_idx < 0) {
		mb_list_idx = -ENOSPC;
		LOG_ERR("There is no free CAN rx filters");
		goto unlock;
	}

	rx_frame_type.msg_buf_id = data->rx_mb_list[mb_list_idx].idx;
	rx_frame_type.frame_ide_mask = 0;
	rx_frame_type.frame_ide_bit = (filter->flags & CAN_FILTER_IDE) ? 1 : 0;
	rx_frame_type.frame_rtr_mask = (IS_ENABLED(CONFIG_CAN_ACCEPT_RTR));
	rx_frame_type.frame_rtr_bit = 0;
	if (rx_frame_type.frame_ide_bit) {
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

	rx_error = CAN_SetMsgBufRxMode(&rx_frame_type);

	ret = can_bee_wait_ram_state(K_MSEC(10));
	if (ret < 0) {
		LOG_ERR("Wait CAN RAM state timeout");
		mb_list_idx = -EIO;
		goto unlock;
	}

	if (rx_error != CAN_NO_ERR) {
		mb_list_idx = -ENOSPC;
		goto unlock;
	}

	data->rx_mb_list[mb_list_idx].rx_callback = callback;
	data->rx_mb_list[mb_list_idx].user_data = user_data;
	data->rx_mb_list[mb_list_idx].is_busy = true;
	CAN_MBRxINTConfig(rx_frame_type.msg_buf_id, ENABLE);

unlock:
	k_mutex_unlock(&data->inst_mutex);
	return mb_list_idx;
}

static void can_bee_remove_rx_filter(const struct device *dev, int mb_list_idx)
{
	struct can_bee_data *data = dev->data;
	CANRxFrame_TypeDef rx_frame_type;
	int ret;

	LOG_DBG("[%s]", __func__);

	k_mutex_lock(&data->inst_mutex, K_FOREVER);
	rx_frame_type.msg_buf_id = data->rx_mb_list[mb_list_idx].idx;
	rx_frame_type.rx_msg_buf_enable = false;
	CAN_SetMsgBufRxMode(&rx_frame_type);
	CAN_MBRxINTConfig(data->rx_mb_list[mb_list_idx].idx, DISABLE);

	ret = can_bee_wait_ram_state(K_MSEC(10));
	if (ret < 0) {
		LOG_ERR("Wait CAN RAM state timeout");
		goto unlock;
	}

	data->rx_mb_list[mb_list_idx].rx_callback = NULL;
	data->rx_mb_list[mb_list_idx].user_data = NULL;
	data->rx_mb_list[mb_list_idx].is_busy = false;

unlock:
	k_mutex_unlock(&data->inst_mutex);
}

static int can_bee_get_state(const struct device *dev, enum can_state *state,
			     struct can_bus_err_cnt *err_cnt)
{
	const struct can_bee_config *cfg = dev->config;
	struct can_bee_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;

	if (state != NULL) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else if (CAN_GetBusState(can) != CAN_BUS_STATE_ON) {
			*state = CAN_STATE_BUS_OFF;
		} else if (CAN_GetErrorPassiveStatus(can)) {
			*state = CAN_STATE_ERROR_PASSIVE;
		} else if (CAN_GetErrorWarningStatus(can)) {
			*state = CAN_STATE_ERROR_WARNING;
		} else {
			*state = CAN_STATE_ERROR_ACTIVE;
		}
	}

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt = CAN_GetTxErrorCnt(can);
		err_cnt->rx_err_cnt = CAN_GetRxErrorCnt(can);
	}

	return 0;
}

#if defined(CONFIG_CAN_MANUAL_RECOVERY_MODE)
static int can_bee_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct can_bee_config *cfg = dev->config;
	struct can_bee_data *data = dev->data;
	CAN_TypeDef *can = cfg->can;
	int ret = 0;

	if (!data->common.started) {
		return -ENETDOWN;
	}

	if ((data->common.mode & CAN_MODE_MANUAL_RECOVERY) == 0U) {
		return -ENOTSUP;
	}

	if (CAN_GetBusState(can) == CAN_BUS_STATE_ON) {
		return 0;
	}

	if (k_mutex_lock(&data->inst_mutex, K_FOREVER)) {
		return -EAGAIN;
	}

	CAN_Cmd(ENABLE);

	ret = can_bee_check_bus_on(timeout);
	if (ret < 0) {
		LOG_ERR("CAN bus off");
		ret = -EIO;
	}

	k_mutex_unlock(&data->inst_mutex);

	return ret;
}
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

static void can_bee_set_state_change_callback(const struct device *dev,
					      can_state_change_callback_t cb, void *user_data)
{
	struct can_bee_data *data = dev->data;

	data->common.state_change_cb = cb;
	data->common.state_change_cb_user_data = user_data;

	if (cb == NULL) {
		CAN_INTConfig((CAN_BUS_OFF_INT | CAN_ERROR_INT | CAN_RX_INT | CAN_TX_INT), DISABLE);
	} else {
		CAN_INTConfig((CAN_BUS_OFF_INT | CAN_ERROR_INT | CAN_RX_INT | CAN_TX_INT), ENABLE);
	}
}

static int can_bee_get_core_clock(const struct device *dev, uint32_t *rate)
{
	ARG_UNUSED(dev);

	*rate = 40000000;

	return 0;
}

static int can_bee_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CONFIG_CAN_BEE_RX_MSG_BUF_NUM;
}

static inline void can_bee_isr(const struct device *dev)
{
	struct can_bee_data *data = dev->data;
	struct can_bus_err_cnt err_cnt;
	enum can_state state = 0;
	const can_state_change_callback_t cb = data->common.state_change_cb;
	void *state_change_cb_data = data->common.state_change_cb_user_data;

	if (SET == CAN_GetINTStatus(CAN_BUS_OFF_INT_FLAG)) {
		CAN_ClearINTPendingBit(CAN_BUS_OFF_INT_FLAG);
		for (uint8_t mb_list_idx = 0; mb_list_idx < CONFIG_CAN_BEE_TX_MSG_BUF_NUM;
		     mb_list_idx++) {
			data->tx_mb_list[mb_list_idx].status = -ENETUNREACH;
			can_bee_tx_complete(dev, mb_list_idx);
		}
	}

	if (SET == CAN_GetINTStatus(CAN_ERROR_INT_FLAG)) {
		CAN_ClearINTPendingBit(CAN_ERROR_INT_FLAG);
		if (SET == CAN_GetErrorStatus(CAN_ERROR_RX)) {
			CAN_CLearErrorStatus(CAN_ERROR_RX);
		}

		if (SET == CAN_GetErrorStatus(CAN_ERROR_TX)) {
			CAN_CLearErrorStatus(CAN_ERROR_TX);
			for (uint8_t mb_list_idx = 0; mb_list_idx < CONFIG_CAN_BEE_TX_MSG_BUF_NUM;
			     mb_list_idx++) {
				if (SET ==
				    CAN_GetMBnTxErrorFlag(data->tx_mb_list[mb_list_idx].idx)) {
					CAN_ClearMBnTxErrorFlag(data->tx_mb_list[mb_list_idx].idx);
					data->tx_mb_list[mb_list_idx].status = -EIO;
					can_bee_tx_complete(dev, mb_list_idx);
				}
			}
		}

#ifdef CONFIG_CAN_STATS
		if (SET == CAN_GetErrorStatus(CAN_ERROR_ACK)) {
			CAN_CLearErrorStatus(CAN_ERROR_ACK);
			CAN_STATS_ACK_ERROR_INC(dev);
		}
		if (SET == CAN_GetErrorStatus(CAN_ERROR_STUFF)) {
			CAN_CLearErrorStatus(CAN_ERROR_STUFF);
			CAN_STATS_STUFF_ERROR_INC(dev);
		}
		if (SET == CAN_GetErrorStatus(CAN_ERROR_CRC)) {
			CAN_CLearErrorStatus(CAN_ERROR_CRC);
			CAN_STATS_CRC_ERROR_INC(dev);
		}
		if (SET == CAN_GetErrorStatus(CAN_ERROR_FORM)) {
			CAN_CLearErrorStatus(CAN_ERROR_FORM);
			CAN_STATS_FORM_ERROR_INC(dev);
		}
		if (SET == CAN_GetErrorStatus(CAN_ERROR_BIT1)) {
			CAN_CLearErrorStatus(CAN_ERROR_BIT1);
			CAN_STATS_BIT1_ERROR_INC(dev);
		}
		if (SET == CAN_GetErrorStatus(CAN_ERROR_BIT0)) {
			CAN_CLearErrorStatus(CAN_ERROR_BIT0);
			CAN_STATS_BIT0_ERROR_INC(dev);
		}

#endif /* CONFIG_CAN_STATS */
		(void)can_bee_get_state(dev, &state, &err_cnt);
		if (state != data->state) {
			data->state = state;
			if (cb != NULL) {
				cb(dev, state, err_cnt, state_change_cb_data);
			}
		}
	}

	if (SET == CAN_GetINTStatus(CAN_RX_INT_FLAG)) {
		CAN_ClearINTPendingBit(CAN_RX_INT_FLAG);
		for (uint8_t mb_list_idx = 0; mb_list_idx < CONFIG_CAN_BEE_RX_MSG_BUF_NUM;
		     mb_list_idx++) {
			if (SET == CAN_GetMBnRxDoneFlag(data->rx_mb_list[mb_list_idx].idx)) {
				CAN_ClearMBnRxDoneFlag(data->rx_mb_list[mb_list_idx].idx);
				can_bee_rx_complete(dev, mb_list_idx);
			}
		}
	}

	if (SET == CAN_GetINTStatus(CAN_TX_INT_FLAG)) {
		CAN_ClearINTPendingBit(CAN_TX_INT_FLAG);
		for (uint8_t mb_list_idx = 0; mb_list_idx < CONFIG_CAN_BEE_TX_MSG_BUF_NUM;
		     mb_list_idx++) {
			if (SET == CAN_GetMBnTxDoneFlag(data->tx_mb_list[mb_list_idx].idx)) {
				CAN_ClearMBnTxDoneFlag(data->tx_mb_list[mb_list_idx].idx);
				can_bee_tx_complete(dev, mb_list_idx);
			}
		}
	}
}
static int can_bee_init(const struct device *dev)
{
	const struct can_bee_config *cfg = dev->config;
	struct can_bee_data *data = dev->data;
	struct can_timing timing = {0};
	CAN_InitTypeDef *init_struct = &(data->init_struct);
	int ret;

	k_mutex_init(&data->inst_mutex);
	k_sem_init(&data->tx_int_sem, CONFIG_CAN_BEE_TX_MSG_BUF_NUM, CONFIG_CAN_BEE_TX_MSG_BUF_NUM);

	for (uint8_t i = 0; i < CONFIG_CAN_BEE_TX_MSG_BUF_NUM; i++) {
		data->tx_mb_list[i].idx = i;
		data->tx_mb_list[i].is_busy = 0;
		data->tx_mb_list[i].status = 0;
		data->tx_mb_list[i].tx_callback = 0;
		data->tx_mb_list[i].user_data = 0;
	}

	for (uint8_t i = 0; i < CONFIG_CAN_BEE_RX_MSG_BUF_NUM; i++) {
		data->rx_mb_list[i].idx = i + CONFIG_CAN_BEE_TX_MSG_BUF_NUM;
		data->rx_mb_list[i].is_busy = 0;
		data->rx_mb_list[i].status = 0;
		data->rx_mb_list[i].rx_callback = 0;
		data->rx_mb_list[i].user_data = 0;
	}

	if (cfg->common.phy != NULL) {
		if (!device_is_ready(cfg->common.phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	(void)clock_control_on(BEE_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);

	ret = can_calc_timing(dev, &timing, cfg->common.bitrate, cfg->common.sample_point);
	if (ret == -EINVAL) {
		LOG_ERR("Can't find timing for given param");
		return -EIO;
	}
	LOG_DBG("Presc: %d, SJW: %d, TS1: %d, TS2: %d", timing.prescaler, timing.sjw,
		timing.phase_seg1, timing.phase_seg2);

	CAN_StructInit(init_struct);
	init_struct->CAN_AutoReTxEn = ENABLE;
	init_struct->CAN_ErrorWarnThd = 128;
#if CONFIG_CAN_RX_TIMESTAMP
	init_struct->CAN_TimeStamp.b.can_time_stamp_en = ENABLE;
	init_struct->CAN_TimeStamp.b.can_time_stamp_div = 200;
#endif
	/* br = (core_clock / prescaler) / (1 + prop_seg + phase_seg1 + phase_seg2) */
	/* can speed = 40mhz / ((brp + 1)*(1 + tseg1 + 1 + tseg2 + 1)) */
	init_struct->CAN_BitTiming.b.can_brp = timing.prescaler - 1;
	init_struct->CAN_BitTiming.b.can_sjw = timing.sjw;
	init_struct->CAN_BitTiming.b.can_tseg1 = timing.phase_seg1 - 1;
	init_struct->CAN_BitTiming.b.can_tseg2 = timing.phase_seg2 - 1;
	CAN_Init(init_struct);

	ret = can_set_timing(dev, &timing);
	if (ret) {
		LOG_WRN("Set timing error: %d", ret);
		return ret;
	}

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
#if defined(CONFIG_CAN_MANUAL_RECOVERY_MODE)
	.recover = can_bee_recover,
#endif
	.set_state_change_callback = can_bee_set_state_change_callback,
	.get_core_clock = can_bee_get_core_clock,
	.get_max_filters = can_bee_get_max_filters,
	.timing_min = {.sjw = 0x1,
		       .prop_seg = 0x00,
		       .phase_seg1 = 0x01,
		       .phase_seg2 = 0x01,
		       .prescaler = 0x01},
	.timing_max = {.sjw = 0x08,
		       .prop_seg = 0x00,
		       .phase_seg1 = 0x100,
		       .phase_seg2 = 0x100,
		       .prescaler = 0x100}};

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
