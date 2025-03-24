/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_psi5

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nxp_s32_psi5, CONFIG_PSI5_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/drivers/psi5/psi5.h>

#include "Psi5_Ip.h"

struct psi5_nxp_s32_config {
	uint8_t ctrl_inst;
	PSI5_Type *base;
	uint8_t channel_mask;
	const struct pinctrl_dev_config *pin_cfg;
	void (*irq_config_func)(void);
};

struct psi5_nxp_s32_tx_callback {
	psi5_tx_callback_t callback;
	void *user_data;
};

struct psi5_nxp_s32_rx_serial_frame_callback {
	psi5_rx_serial_frame_callback_t callback;
	struct psi5_frame frame;
	void *user_data;
};

struct psi5_nxp_s32_rx_data_frame_callback {
	psi5_rx_data_frame_callback_t callback;
	struct psi5_frame frame;
	void *user_data;
};

struct psi5_nxp_s32_channel_data {
	boolean started;
	bool async_mode;
	struct psi5_nxp_s32_tx_callback tx_callback;
	struct psi5_nxp_s32_rx_serial_frame_callback serial_callback;
	struct psi5_nxp_s32_rx_data_frame_callback data_callback;
	struct k_sem tx_sem;
	struct k_mutex lock;
};

struct psi5_nxp_s32_data {
	struct psi5_nxp_s32_channel_data channel_data[PSI5_CHANNEL_COUNT];
};

static int psi5_nxp_s32_start_sync(const struct device *dev, uint8_t channel)
{
	const struct psi5_nxp_s32_config *config = dev->config;
	struct psi5_nxp_s32_data *data = dev->data;
	struct psi5_nxp_s32_channel_data *channel_data = &data->channel_data[channel];
	int err;

	if (!(config->channel_mask & BIT(channel))) {
		return -EINVAL;
	}

	if (channel_data->started) {
		return -EALREADY;
	}

	k_mutex_lock(&channel_data->lock, K_FOREVER);

	err = Psi5_Ip_SetChannelSync(config->ctrl_inst, channel, true);

	if (err) {
		LOG_ERR("Failed to start sync PSI5 %d channel %d", config->ctrl_inst, channel);
		k_mutex_unlock(&channel_data->lock);
		return -EIO;
	}

	k_mutex_unlock(&channel_data->lock);

	channel_data->started = true;

	return 0;
}

static int psi5_nxp_s32_stop_sync(const struct device *dev, uint8_t channel)
{
	const struct psi5_nxp_s32_config *config = dev->config;
	struct psi5_nxp_s32_data *data = dev->data;
	struct psi5_nxp_s32_channel_data *channel_data = &data->channel_data[channel];
	int err;

	if (!(config->channel_mask & BIT(channel))) {
		return -EINVAL;
	}

	if (!channel_data->started) {
		return -EALREADY;
	}

	k_mutex_lock(&channel_data->lock, K_FOREVER);

	err = Psi5_Ip_SetChannelSync(config->ctrl_inst, channel, false);

	if (err) {
		LOG_ERR("Failed to stop sync PSI5 %d channel %d", config->ctrl_inst, channel);
		k_mutex_unlock(&channel_data->lock);
		return -EIO;
	}

	channel_data->started = false;

	k_mutex_unlock(&channel_data->lock);

	return 0;
}

static int psi5_nxp_s32_send(const struct device *dev, uint8_t channel, uint64_t psi5_data,
			     k_timeout_t timeout, psi5_tx_callback_t callback, void *user_data)
{
	const struct psi5_nxp_s32_config *config = dev->config;
	struct psi5_nxp_s32_data *data = dev->data;
	struct psi5_nxp_s32_channel_data *channel_data = &data->channel_data[channel];
	int err;
	uint64_t start_time;

	if (!(config->channel_mask & BIT(channel))) {
		return -EINVAL;
	}

	if (!channel_data->started) {
		return -ENETDOWN;
	}

	if (channel_data->async_mode) {
		return -ENOTSUP;
	}

	if (k_sem_take(&channel_data->tx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	if (callback != NULL) {
		channel_data->tx_callback.callback = callback;
		channel_data->tx_callback.user_data = user_data;
	}

	err = Psi5_Ip_Transmit(config->ctrl_inst, channel, psi5_data);
	if (err) {
		LOG_ERR("Failed to transmit PSI5 %d channel %d", config->ctrl_inst, channel);
		k_sem_give(&channel_data->tx_sem);
		return -EIO;
	}

	if (callback != NULL) {
		return 0;
	}

	start_time = k_uptime_ticks();

	while (!Psi5_Ip_GetTransmissionStatus(config->ctrl_inst, channel)) {
		if (k_uptime_ticks() - start_time >= timeout.ticks) {
			LOG_ERR("Timeout for waiting transmision PSI5 %d channel %d",
				config->ctrl_inst, channel);
			k_sem_give(&channel_data->tx_sem);
			return -EAGAIN;
		}
	}

	return 0;
}

static int psi5_nxp_s32_add_rx_callback(const struct device *dev, uint8_t channel,
					psi5_rx_serial_frame_callback_t serial_callback,
					psi5_rx_data_frame_callback_t data_callback,
					void *user_data)
{
	const struct psi5_nxp_s32_config *config = dev->config;
	struct psi5_nxp_s32_data *data = dev->data;
	struct psi5_nxp_s32_channel_data *channel_data = &data->channel_data[channel];

	if (!(config->channel_mask & BIT(channel))) {
		return -EINVAL;
	}

	k_mutex_lock(&channel_data->lock, K_FOREVER);

	channel_data->serial_callback.callback = serial_callback;
	channel_data->serial_callback.user_data = user_data;

	channel_data->data_callback.callback = data_callback;
	channel_data->data_callback.user_data = user_data;

	k_mutex_unlock(&channel_data->lock);

	return 0;
}

static DEVICE_API(psi5, psi5_nxp_s32_driver_api) = {
	.start_sync = psi5_nxp_s32_start_sync,
	.stop_sync = psi5_nxp_s32_stop_sync,
	.send = psi5_nxp_s32_send,
	.add_rx_callback = psi5_nxp_s32_add_rx_callback,
};

#define PSI5_NXP_S32_HW_INSTANCE_CHECK(i, n) ((DT_INST_REG_ADDR(n) == IP_PSI5_##i##_BASE) ? i : 0)

#define PSI5_NXP_S32_HW_INSTANCE(n)                                                                \
	LISTIFY(PSI5_INSTANCE_COUNT, PSI5_NXP_S32_HW_INSTANCE_CHECK, (|), n)

#define PSI5_NXP_S32_CHANNEL_CALLBACK(node_id)                                                     \
	void _CONCAT(psi5_nxp_s32_channel_callBack, node_id)(Psi5_EventType event)                 \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_GET(DT_PARENT(node_id));                      \
		const struct psi5_nxp_s32_config *config = dev->config;                            \
		Psi5_Ip_Psi5FrameType ip_frame;                                                    \
		Psi5_Ip_SmcFrameType ip_smc_frame;                                                 \
		uint8_t channel = DT_REG_ADDR(node_id);                                            \
		struct psi5_nxp_s32_data *data = dev->data;                                        \
		struct psi5_nxp_s32_channel_data *channel_data = &data->channel_data[channel];     \
		struct psi5_nxp_s32_tx_callback *tx_callback = &channel_data->tx_callback;         \
		struct psi5_nxp_s32_rx_serial_frame_callback *serial_callback =                    \
			&channel_data->serial_callback;                                            \
		struct psi5_nxp_s32_rx_data_frame_callback *data_callback =                        \
			&channel_data->data_callback;                                              \
                                                                                                   \
		if (event.Psi5_DriverReadyToTransmit) {                                            \
			if (tx_callback->callback) {                                               \
				tx_callback->callback(dev, channel, PSI5_TX_DONE,                  \
						      tx_callback->user_data);                     \
			}                                                                          \
			k_sem_give(&channel_data->tx_sem);                                         \
		} else if (event.Psi5_TxDataOverwrite) {                                           \
			if (tx_callback->callback) {                                               \
				tx_callback->callback(dev, channel, PSI5_TX_ERR,                   \
						      tx_callback->user_data);                     \
			}                                                                          \
			k_sem_give(&channel_data->tx_sem);                                         \
		} else if (event.Psi5_Psi5MessageReceived) {                                       \
			Psi5_Ip_GetPsi5Frame(config->ctrl_inst, channel, &ip_frame);               \
                                                                                                   \
			data_callback->frame.type = PSI5_DATA_FRAME;                               \
			data_callback->frame.data = ip_frame.DATA_REGION;                          \
			data_callback->frame.timestamp = ip_frame.TIME_STAMP;                      \
			data_callback->frame.crc = ip_frame.CRC;                                   \
			data_callback->frame.slot_number = ip_frame.SLOT_COUNTER;                  \
                                                                                                   \
			if (data_callback->callback) {                                             \
				data_callback->callback(dev, channel, &data_callback->frame,       \
							!!(ip_frame.C | ip_frame.F | ip_frame.EM | \
							   ip_frame.E | ip_frame.T)                \
								? PSI5_RX_ERR_DATA_FRAME           \
								: PSI5_RX_DATA_FRAME,              \
							data_callback->user_data);                 \
			}                                                                          \
		} else if (event.Psi5_SmcMessageReceived) {                                        \
			Psi5_Ip_GetSmcFrame(config->ctrl_inst, channel, &ip_smc_frame);            \
                                                                                                   \
			if (ip_smc_frame.C) {                                                      \
				serial_callback->frame.serial.id = ip_smc_frame.ID;                \
				serial_callback->frame.serial.data =                               \
					FIELD_PREP(GENMASK(15, 12), (ip_smc_frame.IDDATA)) |       \
					FIELD_PREP(GENMASK(11, 0), (ip_smc_frame.DATA));           \
			} else {                                                                   \
				serial_callback->frame.serial.id =                                 \
					FIELD_PREP(GENMASK(7, 4), (ip_smc_frame.ID)) |             \
					FIELD_PREP(GENMASK(3, 0), (ip_smc_frame.IDDATA));          \
				serial_callback->frame.serial.data = ip_smc_frame.DATA;            \
			}                                                                          \
			serial_callback->frame.type = PSI5_SERIAL_FRAME;                           \
			serial_callback->frame.crc = ip_smc_frame.CRC;                             \
			serial_callback->frame.slot_number = ip_smc_frame.SLOT_NO;                 \
                                                                                                   \
			if (serial_callback->callback) {                                           \
				serial_callback->callback(dev, channel, &serial_callback->frame,   \
							  !!(ip_smc_frame.CER | ip_smc_frame.OW)   \
								  ? PSI5_RX_ERR_SERIAL_FRAME       \
								  : PSI5_RX_SERIAL_FRAME,          \
							  serial_callback->user_data);             \
			}                                                                          \
		}                                                                                  \
	}

#define _PSI5_NXP_S32_CHANNEL_RX_SLOT_CONFIG(i, node_id)                                           \
	{                                                                                          \
		.slotId = UTIL_INC(i),                                                             \
		.slotLen = DT_PROP_BY_IDX(node_id, array_slot_duration_us, i),                     \
		.startOffs = DT_PROP_BY_IDX(node_id, array_slot_start_offset_us, i),               \
		.dataSize = DT_PROP_BY_IDX(node_id, array_slot_data_length, i),                    \
		.msbFirst = DT_PROP_BY_IDX(node_id, array_slot_data_msb_first, i),                 \
		.hasSMC = DT_PROP_BY_IDX(node_id, array_slot_has_smc, i),                          \
		.hasParity = DT_PROP_BY_IDX(node_id, array_slot_has_parity, i),                    \
	},

#define PSI5_NXP_S32_CHANNEL_RX_SLOT_CONFIG(node_id)                                               \
	BUILD_ASSERT(DT_PROP_LEN(node_id, array_slot_duration_us) ==                               \
				     DT_PROP_LEN(node_id, array_slot_start_offset_us) &&           \
			     DT_PROP_LEN(node_id, array_slot_duration_us) ==                       \
				     DT_PROP_LEN(node_id, array_slot_data_length) &&               \
			     DT_PROP_LEN(node_id, array_slot_duration_us) ==                       \
				     DT_PROP_LEN(node_id, array_slot_data_msb_first) &&            \
			     DT_PROP_LEN(node_id, array_slot_duration_us) ==                       \
				     DT_PROP_LEN(node_id, array_slot_has_smc) &&                   \
			     DT_PROP_LEN(node_id, array_slot_duration_us) ==                       \
				     DT_PROP_LEN(node_id, array_slot_has_parity),                  \
		     "Invalid channel RX slot configuration");                                     \
	static const Psi5_Ip_SlotConfigType _CONCAT(                                               \
		psi5_nxp_s32_channel_rx_slot_config,                                               \
		node_id)[DT_PROP_LEN(node_id, array_slot_duration_us)] = {                         \
		LISTIFY(DT_PROP_LEN(node_id, array_slot_duration_us),                              \
					 _PSI5_NXP_S32_CHANNEL_RX_SLOT_CONFIG, (), node_id)};

#define PSI5_NXP_S32_CHANNEL_RX_CONFIG(node_id)                                                    \
	const Psi5_Ip_ChannelRxConfigType _CONCAT(psi5_nxp_s32_channel_rx_config, node_id) = {     \
		.rxBufSize = DT_PROP(node_id, num_rx_buf),                                         \
		.bitRate = DT_ENUM_IDX(node_id, rx_bitrate_kbps),                                  \
		.slotConfig = &_CONCAT(psi5_nxp_s32_channel_rx_slot_config, node_id)[0],           \
		.numOfSlotConfigs = DT_PROP_LEN(node_id, array_slot_duration_us),                  \
		.watermarkInterruptLevel = GENMASK(UTIL_DEC(DT_PROP(node_id, num_rx_buf)), 0),     \
	};

#define PSI5_NXP_S32_CHANNEL_TX_CONFIG(node_id)                                                    \
	const Psi5_Ip_ChannelTxConfigType _CONCAT(psi5_nxp_s32_channel_tx_config, node_id) = {     \
		.targetPulse = DT_PROP(node_id, period_sync_pulse_us),                             \
		.decoderOffset = DT_PROP(node_id, decoder_start_offset_us),                        \
		.pulse0Width = DT_PROP(node_id, pulse_width_0_us),                                 \
		.pulse1Width = DT_PROP(node_id, pulse_width_1_us),                                 \
		.txMode = DT_ENUM_IDX(node_id, tx_mode),                                           \
		.syncState = PSI5_SYNC_STATE_2,                                                    \
		.txSize = 64, /* This setting is applicable only in NON STANDARD FRAME */          \
	};

#define PSI5_NXP_S32_CHANNEL_ERR_SEL_CONFIG(node_id)                                               \
	const Psi5_Ip_ErrorSelectConfigType _CONCAT(psi5_nxp_s32_channel_err_sel_config,           \
						    node_id) = {                                   \
		.errorSelect0 = true,                                                              \
		.errorSelect1 = true,                                                              \
		.errorSelect2 = true,                                                              \
		.errorSelect3 = true,                                                              \
		.errorSelect4 = true,                                                              \
	};

/*
 * The macro get index of array configuration that corresponds to each the ID of HW channel.
 * Assign 0xff to unused channels.
 */

#define PSI5_NXP_S32_CHANNEL_NODE(n, i) DT_INST_CHILD(n, DT_CAT(ch_, i))

#define PSI5_NXP_S32_ID_CFG_CNT(i, node_id, n)                                                     \
	(DT_NODE_HAS_STATUS(PSI5_NXP_S32_CHANNEL_NODE(n, i), okay) &&                              \
			 (DT_REG_ADDR(PSI5_NXP_S32_CHANNEL_NODE(n, i)) < (DT_REG_ADDR(node_id)))   \
		 ? 1                                                                               \
		 : 0)

#define PSI5_NXP_S32_ID_CFG(node_id, n)                                                            \
	COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),                                             \
		(LISTIFY(PSI5_CHANNEL_COUNT, PSI5_NXP_S32_ID_CFG_CNT, (+), node_id, n),), (0xff,))

#define PSI5_NXP_S32_CHANNEL_CONFIG(node_id)                                                       \
	{                                                                                          \
		.channelId = DT_REG_ADDR(node_id),                                                 \
		.channelMode = !DT_PROP(node_id, async_mode),                                      \
		.callback = _CONCAT(psi5_nxp_s32_channel_callBack, node_id),                       \
		.rxConfig = &_CONCAT(psi5_nxp_s32_channel_rx_config, node_id),                     \
		.txConfig = &_CONCAT(psi5_nxp_s32_channel_tx_config, node_id),                     \
		.errorSelectConfig = &_CONCAT(psi5_nxp_s32_channel_err_sel_config, node_id),       \
	},

/* Define array channel configuration */
#define PSI5_NXP_S32_ARRAY_CHANNEL_CONFIG(n)                                                       \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_NXP_S32_CHANNEL_CALLBACK)                        \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_NXP_S32_CHANNEL_RX_SLOT_CONFIG)                  \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_NXP_S32_CHANNEL_RX_CONFIG)                       \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_NXP_S32_CHANNEL_TX_CONFIG)                       \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_NXP_S32_CHANNEL_ERR_SEL_CONFIG)                  \
	const Psi5_Ip_ChannelConfigType                                                            \
		psi5_nxp_s32_channel_array_config_##n[DT_INST_CHILD_NUM_STATUS_OKAY(n)] = {        \
			DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_NXP_S32_CHANNEL_CONFIG)};        \
	const uint8_t psi5_nxp_s32_map_idex_array_config_##n[PSI5_CHANNEL_COUNT] = {               \
		DT_INST_FOREACH_CHILD_VARGS(n, PSI5_NXP_S32_ID_CFG, n)};

DT_INST_FOREACH_STATUS_OKAY(PSI5_NXP_S32_ARRAY_CHANNEL_CONFIG)

/* Define array instance configuration */
#define PSI5_NXP_S32_INST_CONFIG(n)                                                                \
	{                                                                                          \
		.instanceId = PSI5_NXP_S32_HW_INSTANCE(n),                                         \
		.channelConfig = &psi5_nxp_s32_channel_array_config_##n[0],                        \
		.numOfChannels = DT_INST_CHILD_NUM_STATUS_OKAY(n),                                 \
		.chHwIdToIndexArrayConfig = &psi5_nxp_s32_map_idex_array_config_##n[0],            \
	},

static const Psi5_Ip_InstanceType psi5_nxp_s32_array_inst_config[DT_NUM_INST_STATUS_OKAY(
	DT_DRV_COMPAT)] = {DT_INST_FOREACH_STATUS_OKAY(PSI5_NXP_S32_INST_CONFIG)};

/* The structure configuration for all controller instances that used for Psi5_Ip_Init() */
static const Psi5_Ip_ConfigType psi5_nxp_s32_controller_config = {
	.instancesConfig = &psi5_nxp_s32_array_inst_config[0],
	.numOfInstances = DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT),
};

#define PSI5_NXP_S32_CHANNEL_ISR(node_id)                                                          \
	static void _CONCAT(psi5_nxp_s32_channel_isr, node_id)(const struct device *dev)           \
	{                                                                                          \
		const struct psi5_nxp_s32_config *config = dev->config;                            \
                                                                                                   \
		Psi5_Ip_IRQ_Handler(config->ctrl_inst, DT_REG_ADDR(node_id));                      \
	}

#define PSI5_NXP_S32_CHANNEL_IRQ_CONFIG(node_id, n)                                                \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, 0, irq), DT_IRQ_BY_IDX(node_id, 0, priority),   \
			    _CONCAT(psi5_nxp_s32_channel_isr, node_id), DEVICE_DT_INST_GET(n),     \
			    DT_IRQ_BY_IDX(node_id, 0, flags));                                     \
		irq_enable(DT_IRQN(node_id));                                                      \
	} while (false);

#define PSI5_NXP_S32_IRQ_CONFIG(n)                                                                 \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_NXP_S32_CHANNEL_ISR)                             \
	static void psi5_irq_config_##n(void)                                                      \
	{                                                                                          \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, PSI5_NXP_S32_CHANNEL_IRQ_CONFIG, n)     \
	}

#define PSI5_NXP_S32_CHANNEL_BIT_MASK(node_id) BIT(DT_REG_ADDR(node_id))

#define PSI5_NXP_S32_CHANNEL_ASYNC_MODE(node_id)                                                   \
	data->channel_data[DT_REG_ADDR(node_id)].async_mode = DT_PROP(node_id, async_mode);

#define DEV_PSI5_NXP_S32_INIT(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	PSI5_NXP_S32_IRQ_CONFIG(n)                                                                 \
	static struct psi5_nxp_s32_config psi5_nxp_s32_config_##n = {                              \
		.ctrl_inst = PSI5_NXP_S32_HW_INSTANCE(n),                                          \
		.base = (PSI5_Type *)DT_INST_REG_ADDR(n),                                          \
		.channel_mask = DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(                             \
			n, PSI5_NXP_S32_CHANNEL_BIT_MASK, (|)),                                    \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.irq_config_func = psi5_irq_config_##n,                                            \
	};                                                                                         \
	static struct psi5_nxp_s32_data psi5_nxp_s32_data_##n;                                     \
	static int psi5_nxp_s32_init_##n(const struct device *dev)                                 \
	{                                                                                          \
		const struct psi5_nxp_s32_config *config = dev->config;                            \
		struct psi5_nxp_s32_data *data = dev->data;                                        \
		int err = 0;                                                                       \
                                                                                                   \
		err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);                 \
		if (err < 0) {                                                                     \
			LOG_ERR("PSI5 pinctrl setup failed (%d)", err);                            \
			return err;                                                                \
		}                                                                                  \
                                                                                                   \
		for (int i = 0; i < PSI5_CHANNEL_COUNT; i++) {                                     \
			k_sem_init(&data->channel_data[i].tx_sem, 1, 1);                           \
			k_mutex_init(&data->channel_data[i].lock);                                 \
		}                                                                                  \
                                                                                                   \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, PSI5_NXP_S32_CHANNEL_ASYNC_MODE)              \
                                                                                                   \
		/* Common configuration setup for all controller instances */                      \
		if (n == UTIL_DEC(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT))) {                       \
			Psi5_Ip_Init(&psi5_nxp_s32_controller_config);                             \
		}                                                                                  \
                                                                                                   \
		config->irq_config_func();                                                         \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, psi5_nxp_s32_init_##n, NULL, &psi5_nxp_s32_data_##n,              \
			      &psi5_nxp_s32_config_##n, POST_KERNEL, CONFIG_PSI5_INIT_PRIORITY,    \
			      &psi5_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEV_PSI5_NXP_S32_INIT)
