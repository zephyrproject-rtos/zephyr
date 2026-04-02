/*
 * Copyright (c) 2024-2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_can

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <can.h>
#include <wrap_max32_can.h>

LOG_MODULE_REGISTER(can_max32, CONFIG_CAN_LOG_LEVEL);

#define MAX32_CAN_MAX_DLC              8
#define MAX32_TX_CNT                   1
#define MAX32_CAN_DRIVER_RXTX_BUF_SIZE 64

/* dev_list will be used only in object_event_callback() unit_event_callback() */
static const struct device *dev_list[MXC_CAN_INSTANCES];

struct max32_req_data {
	mxc_can_req_t req;
	mxc_can_msg_info_t info;
	uint8_t buf[MAX32_CAN_DRIVER_RXTX_BUF_SIZE];
};

struct max32_can_tx_callback {
	can_tx_callback_t function;
	void *user_data;
};

struct max32_can_rx_callback {
	can_rx_callback_t function;
	void *user_data;
	struct can_filter filter;
};

struct max32_can_data {
	struct can_driver_data common;

	enum can_state state;
	struct k_mutex inst_mutex;

	struct k_sem tx_sem;
	struct max32_can_tx_callback tx_callback;
	struct max32_req_data tx_data;

	uint32_t filter_usage;
	struct max32_can_rx_callback rx_callbacks[CONFIG_CAN_MAX32_MAX_FILTERS];
	struct max32_req_data rx_data;
};

struct max32_can_config {
	struct can_driver_config common;

	mxc_can_regs_t *can;
	uint8_t can_id;

	uint8_t irqn;
	void (*irq_config_func)(const struct device *dev);

	const struct device *clock;
	struct max32_perclk perclk;
	const struct pinctrl_dev_config *pcfg;
};

static void can_max32_convert_canframe_to_req(const struct can_frame *msg, mxc_can_req_t *req)
{
	mxc_can_msg_info_t *info = req->msg_info;

	if (msg->flags & CAN_FRAME_IDE) {
		info->msg_id = MXC_CAN_EXTENDED_ID(msg->id);
	} else {
		info->msg_id = MXC_CAN_STANDARD_ID(msg->id);
	}

	info->rtr = ((msg->flags & CAN_FRAME_RTR) != 0) ? 1 : 0;
	info->dlc = msg->dlc;

	req->data_sz = MIN(CAN_MAX_DLEN, can_dlc_to_bytes(msg->dlc));
	memcpy(req->data, msg->data, req->data_sz * sizeof(uint8_t));
}

static void can_max32_convert_req_to_canframe(const mxc_can_req_t *req, struct can_frame *msg)
{
	mxc_can_msg_info_t *info = req->msg_info;

	memset(msg, 0, sizeof(struct can_frame));

	if (info->msg_id & MXC_CAN_MSG_INFO_IDE_BIT) {
		msg->id = (info->msg_id & CAN_EXT_ID_MASK);
		msg->flags |= CAN_FRAME_IDE;
	} else {
		msg->id = (info->msg_id & CAN_STD_ID_MASK);
	}

	if (info->rtr) {
		msg->flags |= CAN_FRAME_RTR;
	}

	msg->dlc = info->dlc;

	int dlc_bytes = MIN(CAN_MAX_DLEN, can_dlc_to_bytes(info->dlc));

	memcpy(msg->data, req->data, dlc_bytes * sizeof(uint8_t));
}

static int can_max32_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK | CAN_MODE_LISTENONLY;

	return 0;
}

static int can_max32_set_mode(const struct device *dev, can_mode_t mode)
{
	struct max32_can_data *dev_data = dev->data;
	can_mode_t cap;

	(void)can_get_capabilities(dev, &cap);

	if (dev_data->common.started) {
		return -EBUSY;
	}

	if ((mode & ~(cap)) != 0) {
		LOG_ERR("unsupported mode: 0x%08x; Capabilities: 0x%08x", mode, cap);
		return -ENOTSUP;
	}

	dev_data->common.mode = mode;

	return 0;
}

static int can_max32_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct max32_can_config *dev_cfg = dev->config;

	*rate = MXC_CAN_GetClock(dev_cfg->can_id);

	return 0;
}

static int can_max32_init_timing_struct(struct can_timing *timing, const struct device *dev)
{
	const struct max32_can_config *dev_cfg = dev->config;
	int ret = 0;

	ret = can_calc_timing(dev, timing, dev_cfg->common.bitrate,
				dev_cfg->common.sample_point);
	if (ret < 0) {
		LOG_ERR("can_calc_timing error sample_point: %d!",
			dev_cfg->common.sample_point);
	}
	LOG_DBG("Bitrate: %d", dev_cfg->common.bitrate);
	LOG_DBG("Presc: %d, PG1: %d, PG2: %d", timing->prescaler,
		timing->prop_seg + timing->phase_seg1, timing->phase_seg2);

	return ret;
}

static int can_max32_set_timing(const struct device *dev, const struct can_timing *timing)
{
	const struct max32_can_config *dev_cfg = dev->config;
	struct max32_can_data *dev_data = dev->data;
	mxc_can_regs_t *can = dev_cfg->can;
	uint32_t nbt_reg = 0;

	if (!timing) {
		LOG_ERR("timing structure is null");
		return -EINVAL;
	}

	if (dev_data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&dev_data->inst_mutex, K_FOREVER);

	can->mode |= MXC_F_CAN_MODE_RST;

	nbt_reg |= FIELD_PREP(MXC_F_CAN_NBT_NBRP, timing->prescaler - 1);
	nbt_reg |= FIELD_PREP(MXC_F_CAN_NBT_NSEG1, timing->prop_seg + timing->phase_seg1 - 1);
	nbt_reg |= FIELD_PREP(MXC_F_CAN_NBT_NSEG2, timing->phase_seg2 - 1);
	nbt_reg |= FIELD_PREP(MXC_F_CAN_NBT_NSJW, timing->sjw - 1);

	can->nbt = nbt_reg;

	can->mode &= ~MXC_F_CAN_MODE_RST;

	k_mutex_unlock(&dev_data->inst_mutex);

	return 0;
}

static int can_max32_start(const struct device *dev)
{
	const struct max32_can_config *dev_cfg = dev->config;
	struct max32_can_data *dev_data = dev->data;
	can_mode_t mode;
	int ret = 0;

	k_mutex_lock(&dev_data->inst_mutex, K_FOREVER);

	if (dev_data->common.started) {
		ret = -EALREADY;
		goto unlock;
	}

	mode = dev_data->common.mode;

	if (dev_cfg->common.phy != NULL) {
		ret = can_transceiver_enable(dev_cfg->common.phy, mode);
		if (ret != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", ret);
			goto unlock;
		}
	}

	CAN_STATS_RESET(dev);

	if (((mode & CAN_MODE_LOOPBACK) != 0) && ((mode & CAN_MODE_LISTENONLY) != 0)) {
		ret = MXC_CAN_SetMode(dev_cfg->can_id, MXC_CAN_MODE_LOOPBACK_W_TXD);
	} else if ((mode & CAN_MODE_LOOPBACK) != 0) {
		ret = MXC_CAN_SetMode(dev_cfg->can_id, MXC_CAN_MODE_LOOPBACK);
	} else if ((mode & CAN_MODE_LISTENONLY) != 0) {
		ret = MXC_CAN_SetMode(dev_cfg->can_id, MXC_CAN_MODE_MONITOR);
	} else {
		ret = MXC_CAN_SetMode(dev_cfg->can_id, MXC_CAN_MODE_NORMAL);
	}

	dev_data->common.started = true;

unlock:
	k_mutex_unlock(&dev_data->inst_mutex);

	return ret;
}

static int can_max32_stop(const struct device *dev)
{
	const struct max32_can_config *dev_cfg = dev->config;
	struct max32_can_data *dev_data = dev->data;
	int ret = 0;

	k_mutex_lock(&dev_data->inst_mutex, K_FOREVER);

	if (!dev_data->common.started) {
		ret = -EALREADY;
		goto unlock;
	}

	if (dev_cfg->common.phy != NULL) {
		ret = can_transceiver_disable(dev_cfg->common.phy);
		if (ret != 0) {
			LOG_ERR("failed to enable CAN transceiver (err %d)", ret);
			goto unlock;
		}
	}

	ret = MXC_CAN_SetMode(dev_cfg->can_id, MXC_CAN_MODE_INITIALIZATION);
	if (ret < 0) {
		LOG_ERR("failed to stop CAN controller (err %d)", ret);
		goto unlock;
	}

	dev_data->state = CAN_STATE_STOPPED;
	dev_data->common.started = false;

unlock:
	k_mutex_unlock(&dev_data->inst_mutex);

	return ret;
}

static int can_max32_send(const struct device *dev, const struct can_frame *msg,
			  k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	struct max32_can_data *dev_data = dev->data;
	int ret = 0;
	unsigned int key;

	LOG_DBG("Sending %d bytes. Id: 0x%x, ID type: %s %s", can_dlc_to_bytes(msg->dlc),
		msg->id, (msg->flags & CAN_FRAME_IDE) ? "extended" : "standard",
		(msg->flags & CAN_FRAME_RTR) ? "RTR" : "");

	if (msg->dlc > MAX32_CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", msg->dlc, MAX32_CAN_MAX_DLC);
		return -EINVAL;
	}

	if (!dev_data->common.started) {
		return -ENETDOWN;
	}

	if (dev_data->state == CAN_STATE_BUS_OFF) {
		return -ENETUNREACH;
	}

	if ((msg->flags & (CAN_FRAME_FDF | CAN_FRAME_BRS)) != 0) {
		return -ENOTSUP;
	}

	if (k_sem_take(&dev_data->tx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	k_mutex_lock(&dev_data->inst_mutex, K_FOREVER);

	key = irq_lock();
	dev_data->tx_callback.function = callback;
	dev_data->tx_callback.user_data = user_data;
	irq_unlock(key);

	can_max32_convert_canframe_to_req(msg, &dev_data->tx_data.req);

	ret = MXC_CAN_MessageSendAsync(0, &dev_data->tx_data.req);
	if (ret < 0) {
		LOG_ERR("MXC_CAN_MessageSendAsync error (err %d)", ret);
		k_sem_give(&dev_data->tx_sem);
	}

	k_mutex_unlock(&dev_data->inst_mutex);

	return 0;
}

static int can_max32_add_rx_filter(const struct device *dev, can_rx_callback_t callback,
				   void *user_data, const struct can_filter *filter)
{
	struct max32_can_data *dev_data = dev->data;
	int filter_idx = 0;

	__ASSERT(callback != NULL, "rx_filter callback can not be null");

	if ((filter->flags & ~CAN_FILTER_IDE) != 0) {
		LOG_ERR("Unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	k_mutex_lock(&dev_data->inst_mutex, K_FOREVER);

	/* find free filter */
	while ((BIT(filter_idx) & dev_data->filter_usage) &&
		(filter_idx < CONFIG_CAN_MAX32_MAX_FILTERS)) {
		filter_idx++;
	}

	/* setup filter */
	if (filter_idx < CONFIG_CAN_MAX32_MAX_FILTERS) {
		unsigned int key = irq_lock();

		dev_data->filter_usage |= BIT(filter_idx);

		dev_data->rx_callbacks[filter_idx].function = callback;
		dev_data->rx_callbacks[filter_idx].user_data = user_data;
		dev_data->rx_callbacks[filter_idx].filter = *filter;

		irq_unlock(key);

		LOG_DBG("Set filter id:%08X mask:%08X", filter->id, filter->mask);
	} else {
		filter_idx = -ENOSPC;
		LOG_WRN("All filters are used CONFIG_CAN_MAX32_MAX_FILTERS=%d",
			CONFIG_CAN_MAX32_MAX_FILTERS);
	}

	k_mutex_unlock(&dev_data->inst_mutex);

	return filter_idx;
}

static void can_max32_remove_rx_filter(const struct device *dev, int filter_idx)
{
	struct max32_can_data *dev_data = dev->data;
	unsigned int key;

	if ((filter_idx < 0) || (filter_idx >= CONFIG_CAN_MAX32_MAX_FILTERS)) {
		LOG_ERR("Filter ID %d out of bounds", filter_idx);
		return;
	}

	k_mutex_lock(&dev_data->inst_mutex, K_FOREVER);

	if ((dev_data->filter_usage & BIT(filter_idx)) == 0) {
		k_mutex_unlock(&dev_data->inst_mutex);
		LOG_WRN("Filter is already not used filter_id=%d", filter_idx);
		return;
	}

	key = irq_lock();

	dev_data->filter_usage &= ~BIT(filter_idx);
	dev_data->rx_callbacks[filter_idx].function = NULL;
	dev_data->rx_callbacks[filter_idx].user_data = NULL;
	dev_data->rx_callbacks[filter_idx].filter = (struct can_filter){0};

	irq_unlock(key);

	k_mutex_unlock(&dev_data->inst_mutex);
}

static void can_max32_state_change_handler(const struct device *dev, enum can_state old_state)
{
	struct max32_can_data *dev_data = dev->data;
	struct can_bus_err_cnt err_cnt;
	enum can_state new_state;
	can_state_change_callback_t state_change_cb;

	state_change_cb = dev_data->common.state_change_cb;

	can_get_state(dev, &new_state, &err_cnt);
	if (old_state != new_state) {
		if (state_change_cb) {
			state_change_cb(dev, new_state, err_cnt,
					dev_data->common.state_change_cb_user_data);
		}
	}
}

static int can_max32_get_state(const struct device *dev, enum can_state *state,
			       struct can_bus_err_cnt *err_cnt)
{
	const struct max32_can_config *dev_cfg = dev->config;
	struct max32_can_data *dev_data = dev->data;
	mxc_can_regs_t *can = dev_cfg->can;

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt = can->txerr;
		err_cnt->rx_err_cnt = can->rxerr;
	}

	if (state != NULL) {
		*state = dev_data->state;
	}

	return 0;
}

static void can_max32_set_state_change_callback(const struct device *dev,
						can_state_change_callback_t cb, void *user_data)
{
	struct max32_can_data *dev_data = dev->data;
	unsigned int key;

	key = irq_lock();

	dev_data->common.state_change_cb = cb;
	dev_data->common.state_change_cb_user_data = user_data;

	irq_unlock(key);
}

static int can_max32_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ide);

	return CONFIG_CAN_MAX32_MAX_FILTERS;
}

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static int can_max32_recover(const struct device *dev, k_timeout_t timeout)
{
	const struct max32_can_data *dev_data = dev->data;

	ARG_UNUSED(timeout);

	if (!dev_data->common.started) {
		return -ENETDOWN;
	}

	/* The MAX32 CAN controller does not support manual recovery. */
	return -ENOSYS;
}
#endif

static void can_max32_init_req(struct max32_req_data *data)
{
	mxc_can_req_t *req = &data->req;

	req->msg_info = &data->info;
	req->data = data->buf;
	req->data_sz = sizeof(data->buf);
}

/* MAX32 CAN hardware has a very limited hardware filtering feature.
 * This function implements a software filter to match the received CAN frames
 * against the registered filters.
 */
static void can_max32_rx_soft_filter(const struct device *dev, struct can_frame *frame)
{
	struct max32_can_data *dev_data = dev->data;
	uint8_t filter_id = 0U;
	can_rx_callback_t callback;
	struct can_frame tmp_frame;

#ifndef CONFIG_CAN_ACCEPT_RTR
	if ((frame->flags & CAN_FRAME_RTR) != 0U) {
		return;
	}
#endif /* !CONFIG_CAN_ACCEPT_RTR */

	for (; filter_id < CONFIG_CAN_MAX32_MAX_FILTERS; filter_id++) {
		if (!(BIT(filter_id) & dev_data->filter_usage)) {
			continue; /* filter slot empty */
		}

		struct max32_can_rx_callback *rx_cb = &dev_data->rx_callbacks[filter_id];

		if (!can_frame_matches_filter(frame, &rx_cb->filter)) {
			continue; /* filter did not match */
		}

		callback = rx_cb->function;
		/*Make a temporary copy in case the user modifies the message*/
		tmp_frame = *frame;

		callback(dev, &tmp_frame, rx_cb->user_data);
	}
}

void can_max32_rx_handler(const struct device *dev)
{
	struct max32_can_data *dev_data = dev->data;
	struct can_frame msg;

	can_max32_convert_req_to_canframe(&dev_data->rx_data.req, &msg);
	can_max32_rx_soft_filter(dev, &msg);
}

void can_max32_tx_handler(const struct device *dev, int status)
{
	struct max32_can_data *dev_data = dev->data;
	can_tx_callback_t callback = dev_data->tx_callback.function;

	if (callback != NULL) {
		callback(dev, status, dev_data->tx_callback.user_data);
		dev_data->tx_callback.function = NULL;
		dev_data->tx_callback.user_data = NULL;
	}

	k_sem_give(&dev_data->tx_sem); /* to allow next tx request */
}

static void can_max32_isr(const struct device *dev)
{
	const struct max32_can_config *dev_cfg = dev->config;

	MXC_CAN_Handler(dev_cfg->can_id);
}

void object_event_callback(uint32_t can_idx, uint32_t event)
{
	const struct device *dev = dev_list[can_idx];

	if (event == MXC_CAN_OBJ_EVT_TX_COMPLETE) { /* Message send complete */
		can_max32_tx_handler(dev, 0);
	}

	if (event == MXC_CAN_OBJ_EVT_RX) { /* Message receive complete */
		can_max32_rx_handler(dev);
	}
}

void unit_event_callback(uint32_t can_idx, uint32_t event)
{
	const struct device *dev = dev_list[can_idx];
	struct max32_can_data *dev_data = dev->data;
	enum can_state old_state = dev_data->state;

	switch (event) {
	case MXC_CAN_UNIT_EVT_INACTIVE:
		dev_data->state = CAN_STATE_STOPPED;
		break;
	case MXC_CAN_UNIT_EVT_ACTIVE:
		dev_data->state = CAN_STATE_ERROR_ACTIVE;
		break;
	case MXC_CAN_UNIT_EVT_WARNING:
		dev_data->state = CAN_STATE_ERROR_WARNING;
		break;
	case MXC_CAN_UNIT_EVT_PASSIVE:
		dev_data->state = CAN_STATE_ERROR_PASSIVE;
		break;
	case MXC_CAN_UNIT_EVT_BUS_OFF:
		dev_data->state = CAN_STATE_BUS_OFF;
		break;
	default:
	}

	can_max32_state_change_handler(dev, old_state);
}

static int can_max32_init(const struct device *dev)
{
	const struct max32_can_config *dev_cfg = dev->config;
	struct max32_can_data *dev_data = dev->data;
	struct can_timing timing = {0};
	int ret = 0;

	k_mutex_init(&dev_data->inst_mutex);
	k_sem_init(&dev_data->tx_sem, 1, 1);

	if (dev_cfg->common.phy != NULL) {
		if (!device_is_ready(dev_cfg->common.phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	if (!device_is_ready(dev_cfg->clock)) {
		LOG_ERR("CAN clock is not ready");
		return -ENODEV;
	}

	ret = clock_control_on(dev_cfg->clock, (clock_control_subsys_t)&dev_cfg->perclk);
	if (ret) {
		LOG_ERR("CAN clock is not on");
		return -EIO;
	}

	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("CAN pinctrl apply error:%d", ret);
		return ret;
	}

	dev_data->state = CAN_STATE_STOPPED;

	dev_cfg->irq_config_func(dev);

	dev_list[dev_cfg->can_id] = dev;

	ret = Wrap_MXC_CAN_Init(dev_cfg->can_id, MXC_CAN_OBJ_CFG_TXRX, unit_event_callback,
				object_event_callback);
	if (ret < 0) {
		LOG_ERR("Wrap_MXC_CAN_Init() failed:%d", ret);
		return ret;
	}

	ret = can_max32_init_timing_struct(&timing, dev);
	if (ret < 0) {
		LOG_ERR("can_max32_init_timing_struct failed:%d", ret);
		return ret;
	}

	ret = can_set_timing(dev, &timing);
	if (ret < 0) {
		LOG_ERR("can_set_timing failed:%d", ret);
		return ret;
	}

	/* Since two hw filter exists all EXT and STD frames are allowed */
	MXC_CAN_ObjectSetFilter(dev_cfg->can_id,
				MXC_CAN_FILT_CFG_MASK_ADD | MXC_CAN_FILT_CFG_DUAL1_STD_ID,
				CAN_STD_ID_MASK, 0);
	MXC_CAN_ObjectSetFilter(dev_cfg->can_id,
				MXC_CAN_FILT_CFG_MASK_ADD | MXC_CAN_FILT_CFG_DUAL2_EXT_ID,
				CAN_EXT_ID_MASK, 0);

	/* Initialize the async rx and tx request structures */
	can_max32_init_req(&dev_data->tx_data);
	can_max32_init_req(&dev_data->rx_data);

	ret = can_set_mode(dev, CAN_MODE_NORMAL);
	if (ret) {
		return ret;
	}

	/* No need to call in each time a frame is received */
	ret = MXC_CAN_MessageReadAsync(dev_cfg->can_id, &dev_data->rx_data.req);

	return ret;
}

static DEVICE_API(can, can_max32_api) = {
	.get_capabilities = can_max32_get_capabilities,
	.set_mode = can_max32_set_mode,
	.set_timing = can_max32_set_timing,
	.start = can_max32_start,
	.stop = can_max32_stop,
	.send = can_max32_send,
	.add_rx_filter = can_max32_add_rx_filter,
	.remove_rx_filter = can_max32_remove_rx_filter,
#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
	.recover = can_max32_recover,
#endif
	.get_state = can_max32_get_state,
	.set_state_change_callback = can_max32_set_state_change_callback,
	.get_core_clock = can_max32_get_core_clock,
	.get_max_filters = can_max32_get_max_filters,
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 1,
	},
	.timing_max = {
		.sjw = 4,
		.prop_seg = 0,
		.phase_seg1 = 16,
		.phase_seg2 = 8,
		.prescaler = 64,
	},
};

#define CAN_MAX32_IRQ_CONFIG_FUNC(n)                                                               \
	static void can_max32_irq_config_func_##n(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), can_max32_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define DEFINE_CAN_MAX32(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	CAN_MAX32_IRQ_CONFIG_FUNC(inst)                                                            \
                                                                                                   \
	static struct max32_can_data max32_can_data_##inst;                                        \
	static const struct max32_can_config max32_can_config_##inst = {                           \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 1000000),                         \
		.can = (mxc_can_regs_t *)DT_INST_REG_ADDR(inst),                                   \
		.can_id = MXC_CAN_GET_IDX((mxc_can_regs_t *)DT_INST_REG_ADDR(inst)),               \
		.irq_config_func = can_max32_irq_config_func_##inst,				   \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),				   \
		.perclk.bus = DT_INST_CLOCKS_CELL(inst, offset),                                   \
		.perclk.bit = DT_INST_CLOCKS_CELL(inst, bit),                                      \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, can_max32_init, NULL, &max32_can_data_##inst,              \
				  &max32_can_config_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY, \
				  &can_max32_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_CAN_MAX32)
