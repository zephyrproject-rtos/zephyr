/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_psi5_s

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nxp_s32_psi5_s, CONFIG_PSI5_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/drivers/psi5/psi5.h>

#include "Psi5_S_Ip.h"

#define PSI5_S_CHANNEL_COUNT 8

struct psi5_s_nxp_s32_config {
	uint8_t ctrl_inst;
	uint8_t channel_mask;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pin_cfg;
	void (*irq_config_func)(void);
};

struct psi5_s_nxp_s32_tx_callback {
	psi5_tx_callback_t callback;
	void *user_data;
};
struct psi5_s_nxp_s32_rx_callback {
	psi5_rx_callback_t callback;
	struct psi5_frame frame;
	void *user_data;
};

struct psi5_s_nxp_s32_channel_data {
	boolean started;
	struct psi5_s_nxp_s32_tx_callback tx_callback;
	struct psi5_s_nxp_s32_rx_callback rx_callback;
	struct k_sem tx_sem;
	struct k_mutex lock;
};

struct psi5_s_nxp_s32_data {
	struct psi5_s_nxp_s32_channel_data channel_data[PSI5_S_CHANNEL_COUNT];
};

static int psi5_s_nxp_s32_start(const struct device *dev, uint8_t channel)
{
	const struct psi5_s_nxp_s32_config *config = dev->config;
	struct psi5_s_nxp_s32_data *data = dev->data;
	struct psi5_s_nxp_s32_channel_data *channel_data = &data->channel_data[channel];
	int err;

	if (!(config->channel_mask & BIT(channel))) {
		return -EINVAL;
	}

	if (channel_data->started) {
		return -EALREADY;
	}

	k_mutex_lock(&channel_data->lock, K_FOREVER);

	err = Psi5_S_Ip_SetChannelSync(config->ctrl_inst, channel, true);

	if (err) {
		LOG_ERR("Failed to start PSI5_S %d channel %d", config->ctrl_inst, channel);
		k_mutex_unlock(&channel_data->lock);
		return -EIO;
	}

	channel_data->started = true;

	k_mutex_unlock(&channel_data->lock);

	return 0;
}

static int psi5_s_nxp_s32_stop(const struct device *dev, uint8_t channel)
{
	const struct psi5_s_nxp_s32_config *config = dev->config;
	struct psi5_s_nxp_s32_data *data = dev->data;
	struct psi5_s_nxp_s32_channel_data *channel_data = &data->channel_data[channel];
	int err;

	if (!(config->channel_mask & BIT(channel))) {
		return -EINVAL;
	}

	if (!channel_data->started) {
		return -EALREADY;
	}

	k_mutex_lock(&channel_data->lock, K_FOREVER);

	err = Psi5_S_Ip_SetChannelSync(config->ctrl_inst, channel, false);

	if (err) {
		LOG_ERR("Failed to stop PSI5_S %d channel %d", config->ctrl_inst, channel);
		k_mutex_unlock(&channel_data->lock);
		return -EIO;
	}

	channel_data->started = false;

	k_mutex_unlock(&channel_data->lock);

	return 0;
}

static int psi5_s_nxp_s32_send(const struct device *dev, uint8_t channel, uint64_t psi5_data,
			       k_timeout_t timeout, psi5_tx_callback_t callback, void *user_data)
{
	const struct psi5_s_nxp_s32_config *config = dev->config;
	struct psi5_s_nxp_s32_data *data = dev->data;
	struct psi5_s_nxp_s32_channel_data *channel_data = &data->channel_data[channel];
	int err;
	uint64_t start_time;

	if (!(config->channel_mask & BIT(channel))) {
		return -EINVAL;
	}

	if (!channel_data->started) {
		return -ENETDOWN;
	}

	if (k_sem_take(&channel_data->tx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	if (callback != NULL) {
		channel_data->tx_callback.callback = callback;
		channel_data->tx_callback.user_data = user_data;
	}

	k_mutex_lock(&channel_data->lock, K_FOREVER);

	err = Psi5_S_Ip_Transmit(config->ctrl_inst, channel, psi5_data);
	if (err) {
		LOG_ERR("Failed to transmit PSI5_S %d channel %d (err %d)", config->ctrl_inst,
			channel, err);
		k_sem_give(&channel_data->tx_sem);
		goto unlock;
		return -EIO;
	}

	if (callback != NULL) {
		goto unlock;
		return 0;
	}

	start_time = k_uptime_ticks();

	while (!Psi5_S_Ip_GetTransmissionStatus(config->ctrl_inst, channel)) {
		if (k_uptime_ticks() - start_time >= timeout.ticks) {
			LOG_ERR("Timeout for waiting transmision PSI5_S %d channel %d",
				config->ctrl_inst, channel);
			k_sem_give(&channel_data->tx_sem);
			goto unlock;
			return -EAGAIN;
		}
	}

	k_sem_give(&channel_data->tx_sem);

unlock:
	k_mutex_unlock(&channel_data->lock);

	return 0;
}

static int psi5_s_nxp_s32_add_rx_callback(const struct device *dev, uint8_t channel,
					  psi5_rx_callback_t callback, void *user_data)
{
	const struct psi5_s_nxp_s32_config *config = dev->config;
	struct psi5_s_nxp_s32_data *data = dev->data;
	struct psi5_s_nxp_s32_channel_data *channel_data = &data->channel_data[channel];

	if (!(config->channel_mask & BIT(channel))) {
		return -EINVAL;
	}

	if ((channel_data->rx_callback.callback == callback) &&
	    (channel_data->rx_callback.user_data == user_data)) {
		return 0;
	}

	if (channel_data->rx_callback.callback) {
		return -EBUSY;
	}

	k_mutex_lock(&channel_data->lock, K_FOREVER);

	channel_data->rx_callback.callback = callback;
	channel_data->rx_callback.user_data = user_data;

	k_mutex_unlock(&channel_data->lock);

	return 0;
}

static const struct psi5_driver_api psi5_s_nxp_s32_driver_api = {
	.start = psi5_s_nxp_s32_start,
	.stop = psi5_s_nxp_s32_stop,
	.send = psi5_s_nxp_s32_send,
	.add_rx_callback = psi5_s_nxp_s32_add_rx_callback,
};

#define PSI5_S_NXP_S32_HW_INSTANCE_CHECK(i, n)                                                     \
	((DT_INST_REG_ADDR(n) == IP_PSI5_S_##i##_BASE) ? i : 0)

#define PSI5_S_NXP_S32_HW_INSTANCE(n)                                                              \
	LISTIFY(PSI5_S_INSTANCE_COUNT, PSI5_S_NXP_S32_HW_INSTANCE_CHECK, (|), n)

#define PSI5_S_NXP_S32_CHANNEL_CALLBACK(node_id)                                                   \
	void _CONCAT(psi5_s_nxp_s32_channel_tx_callBack, node_id)(Psi5_S_EventType event)          \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_GET(DT_PARENT(node_id));                      \
		uint8_t channel = DT_REG_ADDR(node_id);                                            \
		struct psi5_s_nxp_s32_data *data = dev->data;                                      \
		struct psi5_s_nxp_s32_channel_data *channel_data = &data->channel_data[channel];   \
		struct psi5_s_nxp_s32_tx_callback *tx_callback = &channel_data->tx_callback;       \
		if (event.Psi5S_ReadyToTransmit) {                                                 \
			if (tx_callback->callback) {                                               \
				tx_callback->callback(dev, channel, PSI5_STATE_TX_READY,           \
						      tx_callback->user_data);                     \
			}                                                                          \
			k_sem_give(&channel_data->tx_sem);                                         \
		}                                                                                  \
		if (event.Psi5S_TxDataOverwrite) {                                                 \
			if (tx_callback->callback) {                                               \
				tx_callback->callback(dev, channel, PSI5_STATE_TX_OVERWRITE,       \
						      tx_callback->user_data);                     \
			}                                                                          \
			k_sem_give(&channel_data->tx_sem);                                         \
		}                                                                                  \
	}                                                                                          \
	void _CONCAT(psi5_s_nxp_s32_channel_rx_callBack, node_id)(                                 \
		Psi5_S_Ip_InstanceIdType Psi5SInstanceId, Psi5_S_Ip_Psi5SFrameType Psi5SFramePtr)  \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_GET(DT_PARENT(node_id));                      \
		uint8_t channel = DT_REG_ADDR(node_id);                                            \
		struct psi5_s_nxp_s32_data *data = dev->data;                                      \
		struct psi5_s_nxp_s32_channel_data *channel_data = &data->channel_data[channel];   \
		struct psi5_s_nxp_s32_rx_callback *rx_callback = &channel_data->rx_callback;       \
                                                                                                   \
		rx_callback->frame.msg.data = Psi5SFramePtr.PS_DATA;                               \
		rx_callback->frame.msg.timestamp = Psi5SFramePtr.TIME_STAMP;                       \
		rx_callback->frame.msg.crc = Psi5SFramePtr.CRC;                                    \
		if (rx_callback->callback) {                                                       \
			rx_callback->callback(dev, channel, &rx_callback->frame,                   \
					      PSI5_STATE_MSG_RECEIVED, rx_callback->user_data);    \
		}                                                                                  \
	}

#define _PSI5_S_NXP_S32_CHANNEL_RX_SLOT_CONFIG(i, node_id)                                         \
	{                                                                                          \
		.slotId = UTIL_INC(i),                                                             \
		.useCRC = true,                                                                    \
		.payloadSize = DT_PROP_BY_IDX(node_id, slots_pay_load_size, i),                    \
	},

#define PSI5_S_NXP_S32_CHANNEL_RX_SLOT_CONFIG(node_id)                                             \
	static const Psi5_S_Ip_SlotConfigType _CONCAT(                                             \
		psi5_s_nxp_s32_channel_rx_slot_config,                                             \
		node_id)[DT_PROP_LEN(node_id, slots_pay_load_size)] = {                            \
		LISTIFY(DT_PROP_LEN(node_id, slots_pay_load_size),                               \
			_PSI5_S_NXP_S32_CHANNEL_RX_SLOT_CONFIG, (), node_id)};

#define PSI5_S_NXP_S32_CHANNEL_RX_CONFIG(node_id)                                                  \
	const Psi5_S_Ip_ChannelRxConfigType _CONCAT(psi5_s_nxp_s32_channel_rx_config, node_id) = { \
		.slotConfig = &_CONCAT(psi5_s_nxp_s32_channel_rx_slot_config, node_id)[0],         \
		.numOfSlotConfigs = DT_PROP_LEN(node_id, slots_pay_load_size),                     \
	};

#define PSI5_S_NXP_S32_CHANNEL_TX_CONFIG(node_id)                                                  \
	const Psi5_S_Ip_ChannelTxConfigType _CONCAT(psi5_s_nxp_s32_channel_tx_config, node_id) = { \
		.clockSel = IPG_CLK_PS_DDTRIG,                                                     \
		.initCMD = DT_PROP(node_id, init_cmd),                                             \
		.initACMD = DT_PROP(node_id, init_acmd),                                           \
		.targetPeriod = DT_PROP(node_id, target_period),                                   \
		.counterDelay = DT_PROP(node_id, counter_delay),                                   \
		.txMode = DT_ENUM_IDX(node_id, tx_mode),                                           \
	};

/*
 * The macro get index of array configuration that corresponds to each the ID of HW channel.
 * Assign 0xff to unused channels.
 */

#define PSI5_S_NXP_S32_CHANNEL_NODE(n, i) DT_INST_CHILD(n, DT_CAT(ch_, i))

#define PSI5_S_NXP_S32_ID_CFG_CNT(i, node_id, n)                                                   \
	(DT_NODE_HAS_STATUS(PSI5_S_NXP_S32_CHANNEL_NODE(n, i), okay) &&                            \
			 (DT_REG_ADDR(PSI5_S_NXP_S32_CHANNEL_NODE(n, i)) < (DT_REG_ADDR(node_id))) \
		 ? 1                                                                               \
		 : 0)

#define PSI5_S_NXP_S32_ID_CFG(node_id, n)                                                          \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),                                             \
		    (LISTIFY(PSI5_S_CHANNEL_COUNT, PSI5_S_NXP_S32_ID_CFG_CNT, (+), node_id, n), ), \
		    (0xff, ))

#define PSI5_S_NXP_S32_CHANNEL_CONFIG(node_id)                                                     \
	{                                                                                          \
		.channelId = DT_REG_ADDR(node_id),                                                 \
		.mode = DT_PROP(node_id, async_mode),                                              \
		.callbackRx = _CONCAT(psi5_s_nxp_s32_channel_rx_callBack, node_id),                \
		.callbackTx = _CONCAT(psi5_s_nxp_s32_channel_tx_callBack, node_id),                \
		.timestamp = PSI5_S_TIME_STAMP_A,                                                  \
		.useCRC = true,                                                                    \
		.rxConfig = &_CONCAT(psi5_s_nxp_s32_channel_rx_config, node_id),                   \
		.txConfig = &_CONCAT(psi5_s_nxp_s32_channel_tx_config, node_id),                   \
	},

/* Define array channel configuration */
#define PSI5_S_NXP_S32_ARRAY_CHANNEL_CONFIG(n)                                                     \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_S_NXP_S32_CHANNEL_CALLBACK)                      \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_S_NXP_S32_CHANNEL_RX_SLOT_CONFIG)                \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_S_NXP_S32_CHANNEL_RX_CONFIG)                     \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_S_NXP_S32_CHANNEL_TX_CONFIG)                     \
	const Psi5_S_Ip_ChannelConfigType                                                          \
		psi5_s_nxp_s32_channel_array_config_##n[DT_INST_CHILD_NUM_STATUS_OKAY(n)] = {      \
			DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_S_NXP_S32_CHANNEL_CONFIG)};      \
	const uint8_t psi5_s_nxp_s32_map_idex_array_config_##n[PSI5_S_CHANNEL_COUNT] = {           \
		DT_INST_FOREACH_CHILD_VARGS(n, PSI5_S_NXP_S32_ID_CFG, n)};

DT_INST_FOREACH_STATUS_OKAY(PSI5_S_NXP_S32_ARRAY_CHANNEL_CONFIG)

#define PSI5_S_NXP_S32_UART_CONFIG(n)                                                              \
	Psi5_S_Ip_UartConfigType psi5_s_nxp_s32_uart_config_##n = {                                \
		.Uart_baud_rate = DT_INST_PROP(n, uart_baud_rate),                                 \
		.Uart_preset_timeout = DT_INST_PROP(n, uart_preset_timeout),                       \
		.Uart_tx_idle_delay_time_enable = true,                                            \
		.Uart_tx_idle_delay_time = DT_INST_PROP(n, uart_tx_idle_delay_time),               \
		.Uart_reduced_over_sampling_enable = true,                                         \
		.Uart_over_sampling_rate = DT_INST_PROP(n, uart_reduced_over_sampling),            \
		.Uart_sampling_point = DT_INST_PROP(n, uart_sampling_point),                       \
	};

DT_INST_FOREACH_STATUS_OKAY(PSI5_S_NXP_S32_UART_CONFIG)

/* Define array instance configuration */
#define PSI5_S_NXP_S32_INST_CONFIG(n)                                                              \
	{                                                                                          \
		.instanceId = PSI5_S_NXP_S32_HW_INSTANCE(n),                                       \
		.channelConfig = &psi5_s_nxp_s32_channel_array_config_##n[0],                      \
		.numOfChannels = DT_INST_CHILD_NUM_STATUS_OKAY(n),                                 \
		.chHwIdToIndexArrayConfig = &psi5_s_nxp_s32_map_idex_array_config_##n[0],          \
		.uartConfig = &psi5_s_nxp_s32_uart_config_##n,                                     \
	},

static const Psi5_S_Ip_InstanceType psi5_s_nxp_s32_array_inst_config[DT_NUM_INST_STATUS_OKAY(
	DT_DRV_COMPAT)] = {DT_INST_FOREACH_STATUS_OKAY(PSI5_S_NXP_S32_INST_CONFIG)};

/* The structure configuration for all controller instnaces that used for Psi5_S_Ip_Init() */
static const Psi5_S_Ip_ConfigType psi5_s_nxp_s32_controller_config = {
	.instancesConfig = &psi5_s_nxp_s32_array_inst_config[0],
	.numOfInstances = DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT),
};

#define PSI5_S_NXP_S32_CHANNEL_ISR(node_id)                                                        \
	static void _CONCAT(psi5_s_nxp_s32_channel_isr, node_id)(const struct device *dev)         \
	{                                                                                          \
		const struct psi5_s_nxp_s32_config *config = dev->config;                          \
                                                                                                   \
		Psi5_S_Ip_IRQ_Handler_Tx(config->ctrl_inst, DT_REG_ADDR(node_id));                 \
		Psi5_S_Ip_IRQ_Handler_Rx(config->ctrl_inst, DT_REG_ADDR(node_id));                 \
	}

#define PSI5_S_NXP_S32_CHANNEL_IRQ_CONFIG(node_id, n)                                              \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, 0, irq), DT_IRQ_BY_IDX(node_id, 0, priority),   \
			    _CONCAT(psi5_s_nxp_s32_channel_isr, node_id), DEVICE_DT_INST_GET(n),   \
			    DT_IRQ_BY_IDX(node_id, 0, flags));                                     \
		irq_enable(DT_IRQN(node_id));                                                      \
	} while (false);

#define PSI5_S_NXP_S32_IRQ_CONFIG(n)                                                               \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_S_NXP_S32_CHANNEL_ISR)                           \
	static void psi5_s_irq_config_##n(void)                                                    \
	{                                                                                          \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, PSI5_S_NXP_S32_CHANNEL_IRQ_CONFIG, n)   \
	}

#define PSI5_S_NXP_S32_CHANNEL_BIT_MASK(node_id) BIT(DT_REG_ADDR(node_id))

#define PSI5_S_NXP_S32_INIT(n)                                                                     \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	PSI5_S_NXP_S32_IRQ_CONFIG(n)                                                               \
	static struct psi5_s_nxp_s32_config psi5_s_nxp_s32_config_##n = {                          \
		.ctrl_inst = PSI5_S_NXP_S32_HW_INSTANCE(n),                                        \
		.channel_mask = DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(                             \
			n, PSI5_S_NXP_S32_CHANNEL_BIT_MASK, (|)),                                  \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.irq_config_func = psi5_s_irq_config_##n,                                          \
	};                                                                                         \
	static struct psi5_s_nxp_s32_data psi5_s_nxp_s32_data_##n;                                 \
	static int psi5_s_nxp_s32_init_##n(const struct device *dev)                               \
	{                                                                                          \
		const struct psi5_s_nxp_s32_config *config = dev->config;                          \
		struct psi5_s_nxp_s32_data *data = dev->data;                                      \
		int err = 0;                                                                       \
		uint32_t rate;                                                                     \
                                                                                                   \
		if (!device_is_ready(config->clock_dev)) {                                         \
			LOG_ERR("Clock control device not ready");                                 \
			return -ENODEV;                                                            \
		}                                                                                  \
                                                                                                   \
		err = clock_control_on(config->clock_dev, config->clock_subsys);                   \
		if (err) {                                                                         \
			LOG_ERR("Failed to enable clock");                                         \
			return err;                                                                \
		}                                                                                  \
                                                                                                   \
		err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &rate);      \
		if (err) {                                                                         \
			LOG_ERR("Failed to get clock");                                            \
			return err;                                                                \
		}                                                                                  \
		memcpy((uint32_t *)&psi5_s_nxp_s32_uart_config_##n.Uart_baud_clock, &rate,         \
		       sizeof(uint32_t));                                                          \
                                                                                                   \
		err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);                 \
		if (err < 0) {                                                                     \
			LOG_ERR("PSI5_S pinctrl setup failed (%d)", err);                          \
			return err;                                                                \
		}                                                                                  \
                                                                                                   \
		for (int i = 0; i < PSI5_S_CHANNEL_COUNT; i++) {                                   \
			k_sem_init(&data->channel_data[i].tx_sem, 1, 1);                           \
			k_mutex_init(&data->channel_data[i].lock);                                 \
		}                                                                                  \
                                                                                                   \
		/* Common configuration setup for all controller instances */                      \
		if (n == UTIL_DEC(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT))) {                       \
			Psi5_S_Ip_Init(&psi5_s_nxp_s32_controller_config);                         \
		}                                                                                  \
                                                                                                   \
		config->irq_config_func();                                                         \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, psi5_s_nxp_s32_init_##n, NULL, &psi5_s_nxp_s32_data_##n,          \
			      &psi5_s_nxp_s32_config_##n, POST_KERNEL, CONFIG_PSI5_INIT_PRIORITY,  \
			      &psi5_s_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PSI5_S_NXP_S32_INIT)
