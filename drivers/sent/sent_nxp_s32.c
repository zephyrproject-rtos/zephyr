/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_sent

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nxp_s32_sent, CONFIG_SENT_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/sent/sent.h>

#include <zephyr/drivers/sent/sent.h>

#include "Srx_Ip.h"

#define CHANNEL_ID_INVAL 0xFF

#if (SRX_IP_TIMESTAMP_FEATURE_ENABLE == STD_ON)
#define TIMESTAMP_FEATURE_ENABLE 1
#endif

struct sent_nxp_s32_config {
	uint8_t ctrl_inst;
	uint8_t ctrl_id;
	uint8_t num_channels;
	uint8_t channel_map[SRX_CNL_COUNT];
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pin_cfg;
	void (*irq_config_func)(void);
};

struct sent_nxp_s32_channel_data {
	uint8_t channel_id;
	bool started;
	struct sent_rx_callback_configs callback_configs;
	uint32_t serial_frame_cnt;
	uint32_t fast_frame_cnt;
	struct k_mutex lock;
};

struct sent_nxp_s32_data {
	struct sent_nxp_s32_channel_data channel_data[SRX_CNL_COUNT];
};

static int sent_nxp_s32_start_listening(const struct device *dev, uint8_t channel)
{
	const struct sent_nxp_s32_config *config = dev->config;
	struct sent_nxp_s32_data *data = dev->data;
	struct sent_nxp_s32_channel_data *channel_data = &data->channel_data[channel];
	Srx_Ip_StatusType err;

	if (channel_data->channel_id == CHANNEL_ID_INVAL) {
		return -EINVAL;
	}

	if (channel_data->started) {
		return -EALREADY;
	}

	k_mutex_lock(&channel_data->lock, K_FOREVER);

	err = Srx_Ip_StartChannelReceiving(config->ctrl_id, channel_data->channel_id);

	if (err) {
		LOG_ERR("Failed to start SENT %d channel %d", config->ctrl_inst, channel);
		k_mutex_unlock(&channel_data->lock);
		return -EIO;
	}

	channel_data->started = true;

	k_mutex_unlock(&channel_data->lock);

	return 0;
}

static int sent_nxp_s32_stop_listening(const struct device *dev, uint8_t channel)
{
	const struct sent_nxp_s32_config *config = dev->config;
	struct sent_nxp_s32_data *data = dev->data;
	struct sent_nxp_s32_channel_data *channel_data = &data->channel_data[channel];
	Srx_Ip_StatusType err;

	if (channel_data->channel_id == CHANNEL_ID_INVAL) {
		return -EINVAL;
	}

	if (!channel_data->started) {
		return -EALREADY;
	}

	k_mutex_lock(&channel_data->lock, K_FOREVER);

	err = Srx_Ip_StopChannelReceiving(config->ctrl_id, channel_data->channel_id);

	if (err) {
		LOG_ERR("Failed to stop SENT %d channel %d", config->ctrl_inst, channel);
		k_mutex_unlock(&channel_data->lock);
		return -EIO;
	}

	channel_data->started = false;

	k_mutex_unlock(&channel_data->lock);

	return 0;
}

static int sent_nxp_s32_register_callback(const struct device *dev, uint8_t channel,
					  struct sent_rx_callback_configs callback_configs)
{
	struct sent_nxp_s32_data *data = dev->data;
	struct sent_nxp_s32_channel_data *channel_data = &data->channel_data[channel];

	if (channel_data->channel_id == CHANNEL_ID_INVAL) {
		return -EINVAL;
	}

	k_mutex_lock(&channel_data->lock, K_FOREVER);

	channel_data->callback_configs = callback_configs;

	k_mutex_unlock(&channel_data->lock);

	return 0;
}

static DEVICE_API(sent, sent_nxp_s32_driver_api) = {
	.start_listening = sent_nxp_s32_start_listening,
	.stop_listening = sent_nxp_s32_stop_listening,
	.register_callback = sent_nxp_s32_register_callback,
};

static void sent_nxp_s32_isr_serial_msg(const struct device *dev)
{
	const struct sent_nxp_s32_config *config = dev->config;

	Srx_Ip_ProcessMsgCombinedInterrupt(config->ctrl_inst, SRX_IP_SERIAL_MSG_ONLY);
}

static void sent_nxp_s32_isr_fast_msg(const struct device *dev)
{
	const struct sent_nxp_s32_config *config = dev->config;

	Srx_Ip_ProcessMsgCombinedInterrupt(config->ctrl_inst, SRX_IP_FAST_MSG_ONLY);
}

static void sent_nxp_s32_isr_error(const struct device *dev)
{
	const struct sent_nxp_s32_config *config = dev->config;

	Srx_Ip_ProcessErrorCombinedInterrupt(config->ctrl_inst);
}

#define SENT_NXP_S32_HW_INSTANCE_CHECK(i, n)                                                       \
	((DT_INST_REG_ADDR(n) == IP_SRX_##i##_BASE) ? i : 0)

#define SENT_NXP_S32_HW_INSTANCE(n)                                                                \
	LISTIFY(SRX_INSTANCE_COUNT, SENT_NXP_S32_HW_INSTANCE_CHECK, (|), n)

#define SENT_NXP_S32_CHANNEL_NODE(n, i) DT_INST_CHILD(n, DT_CAT(ch_, i))

#define SENT_NXP_S32_CHANNEL_ID_CNT(i, node_id, n)                                                 \
	(DT_NODE_HAS_STATUS(SENT_NXP_S32_CHANNEL_NODE(n, i), okay) &&                              \
			 (DT_REG_ADDR(SENT_NXP_S32_CHANNEL_NODE(n, i)) < DT_REG_ADDR(node_id))     \
		 ? 1                                                                               \
		 : 0)

#define SENT_NXP_S32_CHANNEL_ID(node_id, n)                                                        \
	LISTIFY(SRX_CNL_COUNT, SENT_NXP_S32_CHANNEL_ID_CNT, (+), node_id, n)

#define SENT_NXP_S32_CHANNEL_CONFIG(node_id, n)                                                    \
	BUILD_ASSERT(DT_PROP(node_id, fast_crc) == FAST_CRC_DISABLE ||                             \
		DT_PROP(node_id, fast_crc) == FAST_CRC_RECOMMENDED_IMPLEMENTATION ||               \
		DT_PROP(node_id, fast_crc) == FAST_CRC_LEGACY_IMPLEMENTATION ||                    \
		DT_PROP(node_id, fast_crc) == (FAST_CRC_RECOMMENDED_IMPLEMENTATION |               \
						FAST_CRC_STATUS_INCLUDE) ||                        \
		DT_PROP(node_id, fast_crc) == (FAST_CRC_LEGACY_IMPLEMENTATION |                    \
						FAST_CRC_STATUS_INCLUDE),                          \
		"Fast CRC configuration is invalid");                                              \
	BUILD_ASSERT(DT_PROP(node_id, short_serial_crc) == SHORT_CRC_RECOMMENDED_IMPLEMENTATION || \
		DT_PROP(node_id, short_serial_crc) == SHORT_CRC_LEGACY_IMPLEMENTATION,             \
		"Short serial CRC configuration is invalid");                                      \
	Srx_Ip_ChannelUserConfigType _CONCAT(sent_nxp_s32_channel_config, node_id) = {             \
		.ControllerId = n,                                                                 \
		.ControllerHwOffset = SENT_NXP_S32_HW_INSTANCE(n),                                 \
		.ChannelId = SENT_NXP_S32_CHANNEL_ID(node_id, n),                                  \
		.ChannelHwOffset = DT_REG_ADDR(node_id),                                           \
		.ChannelDataLength = DT_PROP(node_id, num_data_nibbles),                           \
		.ChannelTickLengthUs = DT_PROP(node_id, clock_tick_length_us),                     \
		.ChannelConfigReg =                                                                \
			{                                                                          \
				.BusTimeout = COND_CODE_0(DT_PROP(node_id, bus_timeout_cycles),    \
						(SRX_IP_BUS_TIMEOUT_DISABLED),                     \
						(UTIL_CAT(SRX_IP_RECEIVER_CLOCK_TICK_COUNTS_,      \
							DT_PROP(node_id, bus_timeout_cycles)))),   \
				.FastCrcCheckOff = DT_PROP(node_id, fast_crc) ==                   \
							FAST_CRC_DISABLE ? true : false,           \
				.FastCrcType = DT_PROP(node_id, fast_crc) &                        \
						FAST_CRC_RECOMMENDED_IMPLEMENTATION ?              \
							SRX_IP_RECOMMENDED_IMPLEMENTATION :        \
							SRX_IP_LEGACY_IMPLEMENTATION,              \
				.SlowCrcType = DT_PROP(node_id, short_serial_crc) ==               \
						SHORT_CRC_RECOMMENDED_IMPLEMENTATION ?             \
							SRX_IP_RECOMMENDED_IMPLEMENTATION :        \
							SRX_IP_LEGACY_IMPLEMENTATION,              \
				.SuccessiveCalibCheck =                                            \
					DT_PROP(node_id, successive_calib_pulse_method) == 1 ?     \
					SRX_IP_OPTION_1_PREFERRED : SRX_IP_OPTION_2_LOW_LATENCY,   \
				.SentValidCalibrationPulse =                                       \
					DT_PROP(node_id, calib_pulse_tolerance_percent) == 20 ?    \
					SRX_IP_RANGE_20 : SRX_IP_RANGE_25,                         \
				.CrcStatusNibbleIncluding = DT_PROP(node_id, fast_crc) &           \
					FAST_CRC_STATUS_INCLUDE ? true : false,                    \
			},                                                                         \
	};

#define SENT_NXP_S32_CHANNEL_CONFIG_PTR(node_id)                                                   \
	&_CONCAT(sent_nxp_s32_channel_config, node_id),

/* Define array channel configuration */
#define SENT_NXP_S32_ARRAY_CHANNEL_CONFIG(n)                                                       \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, SENT_NXP_S32_CHANNEL_CONFIG, n)                 \
	static Srx_Ip_ChannelUserConfigType const *const                                           \
		sent_nxp_s32_channel_array_config_##n[DT_INST_CHILD_NUM_STATUS_OKAY(n)] = {        \
			DT_INST_FOREACH_CHILD_STATUS_OKAY(n,                                       \
							  SENT_NXP_S32_CHANNEL_CONFIG_PTR)};

#define SENT_NXP_S32_CALLBACK(n)                                                                   \
	void sent_nxp_s32_cb_fast_msg_##n(uint8 ctrl_id, uint8 channel_id,                         \
					      Srx_Ip_FastMsgType * fast_frame)                     \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(n);                                  \
		struct sent_nxp_s32_data *data = dev->data;                                        \
		const struct sent_nxp_s32_config *config = dev->config;                            \
		uint8_t channel = config->channel_map[channel_id];                                 \
		uint32_t *frame_cnt = &data->channel_data[channel].fast_frame_cnt;                 \
		struct sent_rx_callback_config *rx_callback =                                      \
			data->channel_data[channel].callback_configs.fast;                         \
                                                                                                   \
		if (rx_callback) {                                                                 \
			for (int i = 0; i < fast_frame->Length; i++) {                             \
				rx_callback->frame[*frame_cnt].fast.data_nibbles[i] =              \
					fast_frame->DataNibble[i];                                 \
			}                                                                          \
			rx_callback->frame[*frame_cnt].type = SENT_FAST_FRAME;                     \
			IF_ENABLED(TIMESTAMP_FEATURE_ENABLE,                                       \
				(rx_callback->frame[*frame_cnt].timestamp =                        \
					fast_frame->TimestampFast;))                               \
			rx_callback->frame[*frame_cnt].crc = fast_frame->FastCrc;                  \
                                                                                                   \
			(*frame_cnt)++;                                                            \
                                                                                                   \
			if (*frame_cnt == rx_callback->max_num_frame) {                            \
				rx_callback->callback(dev, channel,                                \
						      *frame_cnt, rx_callback->user_data);         \
				*frame_cnt = 0;                                                    \
			}                                                                          \
		}                                                                                  \
	}                                                                                          \
	void sent_nxp_s32_cb_serial_msg_##n(uint8 ctrl_id, uint8 channel_id,                       \
						Srx_Ip_SerialMsgType * serial_frame)               \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(n);                                  \
		struct sent_nxp_s32_data *data = dev->data;                                        \
		const struct sent_nxp_s32_config *config = dev->config;                            \
		uint8_t channel = config->channel_map[channel_id];                                 \
		uint32_t *frame_cnt = &data->channel_data[channel].serial_frame_cnt;               \
		struct sent_rx_callback_config *rx_callback =                                      \
			data->channel_data[channel].callback_configs.serial;                       \
                                                                                                   \
		if (rx_callback) {                                                                 \
			rx_callback->frame[*frame_cnt].type = serial_frame->MsgType ==             \
								SRX_IP_SHORT_SERIAL ?              \
						SENT_SHORT_SERIAL_FRAME :                          \
						(serial_frame->MsgType ==                          \
								SRX_IP_ENHANCED_SERIAL_4_ID ?      \
							SENT_ENHANCED_SERIAL_FRAME_4_BIT_ID :      \
							SENT_ENHANCED_SERIAL_FRAME_8_BIT_ID);      \
			rx_callback->frame[*frame_cnt].serial.id = serial_frame->MessageId;        \
			rx_callback->frame[*frame_cnt].serial.data = serial_frame->MessageData;    \
			IF_ENABLED(TIMESTAMP_FEATURE_ENABLE,                                       \
				(rx_callback->frame[*frame_cnt].timestamp =                        \
					serial_frame->TimestampSerial;))                           \
			rx_callback->frame[*frame_cnt].crc = serial_frame->SerialCrc;              \
                                                                                                   \
			(*frame_cnt)++;                                                            \
                                                                                                   \
			if (*frame_cnt == rx_callback->max_num_frame) {                            \
				rx_callback->callback(dev, channel,                                \
						      *frame_cnt, rx_callback->user_data);         \
				*frame_cnt = 0;                                                    \
			}                                                                          \
		}                                                                                  \
	}                                                                                          \
	void sent_nxp_s32_error_cb_fast_msg_##n(uint8 ctrl_id, uint8 channel_id,                   \
						    Srx_Ip_ChannelStatusType event)                \
	{                                                                                          \
		ARG_UNUSED(event);                                                                 \
		const struct device *dev = DEVICE_DT_INST_GET(n);                                  \
		struct sent_nxp_s32_data *data = dev->data;                                        \
		const struct sent_nxp_s32_config *config = dev->config;                            \
		uint8_t channel = config->channel_map[channel_id];                                 \
		uint32_t *frame_cnt = &data->channel_data[channel].fast_frame_cnt;                 \
		struct sent_rx_callback_config *rx_callback =                                      \
			data->channel_data[channel].callback_configs.fast;                         \
                                                                                                   \
		if (rx_callback) {                                                                 \
			rx_callback->callback(dev, channel,                                        \
					      *frame_cnt, rx_callback->user_data);                 \
			*frame_cnt = 0;                                                            \
		}                                                                                  \
	}                                                                                          \
	void sent_nxp_s32_error_cb_serial_msg_##n(uint8 ctrl_id, uint8 channel_id,                 \
						      Srx_Ip_ChannelStatusType event)              \
	{                                                                                          \
		ARG_UNUSED(event);                                                                 \
		const struct device *dev = DEVICE_DT_INST_GET(n);                                  \
		struct sent_nxp_s32_data *data = dev->data;                                        \
		const struct sent_nxp_s32_config *config = dev->config;                            \
		uint8_t channel = config->channel_map[channel_id];                                 \
		uint32_t *frame_cnt = &data->channel_data[channel].serial_frame_cnt;               \
		struct sent_rx_callback_config *rx_callback =                                      \
			data->channel_data[channel].callback_configs.serial;                       \
                                                                                                   \
		if (rx_callback) {                                                                 \
			rx_callback->callback(dev, channel,                                        \
					      *frame_cnt, rx_callback->user_data);                 \
			*frame_cnt = 0;                                                            \
		}                                                                                  \
	}

#define _SENT_NXP_S32_IRQ_CONFIG(node_id, prop, idx)                                               \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq),                                      \
			    DT_IRQ_BY_IDX(node_id, idx, priority),                                 \
			    UTIL_CAT(sent_nxp_s32_isr_,                                            \
				     DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)),                  \
			    DEVICE_DT_GET(node_id), DT_IRQ_BY_IDX(node_id, idx, flags));           \
		irq_enable(DT_IRQ_BY_IDX(node_id, idx, irq));                                      \
	} while (false);

#define SENT_NXP_S32_IRQ_CONFIG(n)                                                                 \
	static void sent_irq_config_##n(void)                                                      \
	{                                                                                          \
		DT_INST_FOREACH_PROP_ELEM(n, interrupt_names, _SENT_NXP_S32_IRQ_CONFIG);           \
	}

#define DEV_SENT_NXP_S32_INIT(n)                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	SENT_NXP_S32_IRQ_CONFIG(n)                                                                 \
	SENT_NXP_S32_ARRAY_CHANNEL_CONFIG(n)                                                       \
	SENT_NXP_S32_CALLBACK(n)                                                                   \
	static struct sent_nxp_s32_config sent_nxp_s32_config_##n = {                              \
		.ctrl_inst = SENT_NXP_S32_HW_INSTANCE(n),                                          \
		.ctrl_id = n,                                                                      \
		.num_channels = DT_INST_CHILD_NUM_STATUS_OKAY(n),                                  \
		.channel_map = {DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, DT_REG_ADDR, (,))},       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
		.irq_config_func = sent_irq_config_##n,                                            \
	};                                                                                         \
	static struct sent_nxp_s32_data sent_nxp_s32_data_##n;                                     \
	static Srx_Ip_ControllerConfigType sent_nxp_s32_controller_config_##n = {                  \
		.ControllerId = n,                                                                 \
		.ControllerHwOffset = SENT_NXP_S32_HW_INSTANCE(n),                                 \
		.ControllerMode = SRX_IP_INTERRUPT,                                                \
		.NumberChnlConfigured = DT_INST_CHILD_NUM_STATUS_OKAY(n),                          \
		.ChnlConfig = &sent_nxp_s32_channel_array_config_##n[0],                           \
		.FastErrorNotification = &sent_nxp_s32_error_cb_fast_msg_##n,                      \
		.SerialErrorNotification = &sent_nxp_s32_error_cb_serial_msg_##n,                  \
		.FastFrameNotification = &sent_nxp_s32_cb_fast_msg_##n,                            \
		.SerialFrameNotification = &sent_nxp_s32_cb_serial_msg_##n,                        \
	};                                                                                         \
	static int sent_nxp_s32_init_##n(const struct device *dev)                                 \
	{                                                                                          \
		const struct sent_nxp_s32_config *config = dev->config;                            \
		struct sent_nxp_s32_data *data = dev->data;                                        \
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
                                                                                                   \
		memcpy((uint32_t *)&sent_nxp_s32_controller_config_##n.HighFreqRxClock, &rate,     \
		       sizeof(uint32_t));                                                          \
                                                                                                   \
		err = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);                 \
		if (err < 0) {                                                                     \
			LOG_ERR("SENT pinctrl setup failed (%d)", err);                            \
			return err;                                                                \
		}                                                                                  \
                                                                                                   \
		for (int ch = 0; ch < SRX_CNL_COUNT; ch++) {                                       \
			data->channel_data[ch].channel_id = CHANNEL_ID_INVAL;                      \
		}                                                                                  \
                                                                                                   \
		/* Update the channel ID and initialize the mutex for the enabled channel */       \
		for (int ch_id = 0; ch_id < config->num_channels ; ch_id++) {                      \
			data->channel_data[config->channel_map[ch_id]].channel_id = ch_id;         \
			k_mutex_init(&data->channel_data[config->channel_map[ch_id]].lock);        \
		}                                                                                  \
                                                                                                   \
		Srx_Ip_InitController(&sent_nxp_s32_controller_config_##n);                        \
                                                                                                   \
		config->irq_config_func();                                                         \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, sent_nxp_s32_init_##n, NULL, &sent_nxp_s32_data_##n,              \
			      &sent_nxp_s32_config_##n, POST_KERNEL,                               \
			      CONFIG_SENT_INIT_PRIORITY, &sent_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEV_SENT_NXP_S32_INIT)
