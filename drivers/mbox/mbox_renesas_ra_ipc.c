/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_ipc_mbox

#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>

#include <soc.h>
#include <r_ipc.h>

#define IPC_MBOX_FIFO_SIZE 4
#define IPC_MAX_CHANNELS   8

LOG_MODULE_REGISTER(mbox_renesas_ra_ipc, CONFIG_MBOX_LOG_LEVEL);

extern void ipc_isr(void);

struct renesas_ra_ipc_data {
	ipc_instance_ctrl_t ipc_ctrl;
	ipc_cfg_t fsp_config;
	mbox_callback_t user_cb[IPC_MAX_CHANNELS];
	void *user_cb_data[IPC_MAX_CHANNELS];
	uint32_t enabled_channel_mask;
	uint32_t received_data;
};

struct renesas_ra_ipc_config {
	const uint32_t channel_mask;
	const uint8_t message_channel;
};

static void mbox_renesas_ra_callback(ipc_callback_args_t *fsp_args)
{
	const struct device *dev = fsp_args->p_context;
	const struct renesas_ra_ipc_config *config = dev->config;
	struct renesas_ra_ipc_data *data = dev->data;

	if (fsp_args->event == IPC_EVENT_MESSAGE_RECEIVED) {
		if (!(data->enabled_channel_mask & BIT(config->message_channel))) {
			return;
		}

		data->received_data = fsp_args->message;
		struct mbox_msg msg = {
			.data = (const void *)(&data->received_data),
			.size = IPC_MBOX_FIFO_SIZE,
		};

		if (data->user_cb[config->message_channel] != NULL) {
			data->user_cb[config->message_channel](
				dev, config->message_channel,
				data->user_cb_data[config->message_channel], &msg);
		}
	}

	if (fsp_args->event >= IPC_EVENT_IRQ0 && fsp_args->event <= IPC_EVENT_IRQ7) {
		/* FSP guarantees IPC_EVENT_IRQn are one-hot (1u<<n). */
		__ASSERT((fsp_args->event & (fsp_args->event - 1U)) == 0U,
			 "IRQ event must be one-hot");

		uint8_t channel_id = __builtin_ctz(fsp_args->event);

		if (!(data->enabled_channel_mask & (uint32_t)fsp_args->event)) {
			return;
		}

		if (data->user_cb[channel_id] != NULL) {
			data->user_cb[channel_id](dev, channel_id, data->user_cb_data[channel_id],
						  NULL);
		}
	}
}

static int renesas_ra_ipc_send(const struct device *dev, mbox_channel_id_t channel_id,
			       const struct mbox_msg *msg)
{
	const struct renesas_ra_ipc_config *config = dev->config;
	struct renesas_ra_ipc_data *data = dev->data;
	fsp_err_t fsp_err;

	if (channel_id >= IPC_MAX_CHANNELS) {
		return -EINVAL;
	}

	if (!(BIT(channel_id) & config->channel_mask)) {
		LOG_ERR("Channel %d is not available", channel_id);
		return -EINVAL;
	}

	/* Signalling mode. */
	if (msg == NULL) {
		fsp_err = R_IPC_EventGenerate((ipc_ctrl_t *)(&data->ipc_ctrl), BIT(channel_id));
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("Failed to send signal on channel %u", channel_id);
			return -EIO;
		}
		return 0;
	}

	if (channel_id == config->message_channel) {
		uint32_t msg_tmp = 0;

		if (msg->size > IPC_MBOX_FIFO_SIZE) {
			LOG_ERR("Invalid message size: %zu, expected <= %zu", msg->size,
				IPC_MBOX_FIFO_SIZE);
			return -EMSGSIZE;
		}

		if (msg->data == NULL && msg->size > 0) {
			LOG_ERR("Message data is NULL");
			return -EINVAL;
		}

		memcpy(&msg_tmp, msg->data, msg->size);

		fsp_err = R_IPC_MessageSend((ipc_ctrl_t *const)(&data->ipc_ctrl), msg_tmp);
		if (fsp_err != FSP_SUCCESS) {
			LOG_ERR("Failed to send message on channel %u", channel_id);
			if (fsp_err == FSP_ERR_OVERFLOW) {
				return -EBUSY;
			}
			return -EIO;
		}
	} else {
		LOG_ERR("Channel %u is valid but unsupported for message transfer", channel_id);
		return -ENOTSUP;
	}

	return 0;
}

static int renesas_ra_ipc_reg_callback(const struct device *dev, mbox_channel_id_t channel_id,
				       mbox_callback_t cb, void *user_data)
{
	const struct renesas_ra_ipc_config *config = dev->config;
	struct renesas_ra_ipc_data *data = dev->data;

	if (channel_id >= IPC_MAX_CHANNELS) {
		return -EINVAL;
	}

	if (!(BIT(channel_id) & config->channel_mask)) {
		LOG_ERR("Channel %d is not available", channel_id);
		return -EINVAL;
	}

	uint32_t lock = irq_lock();

	data->user_cb[channel_id] = cb;
	data->user_cb_data[channel_id] = user_data;

	irq_unlock(lock);

	return 0;
}

static int renesas_ra_ipc_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return IPC_MBOX_FIFO_SIZE;
}

static uint32_t renesas_ra_ipc_max_channels_get(const struct device *dev)
{
	const struct renesas_ra_ipc_config *config = dev->config;

	return POPCOUNT(config->channel_mask);
}

static int renesas_ra_ipc_set_enabled(const struct device *dev, mbox_channel_id_t channel_id,
				      bool enabled)
{
	const struct renesas_ra_ipc_config *config = dev->config;
	struct renesas_ra_ipc_data *data = dev->data;

	if (channel_id >= IPC_MAX_CHANNELS) {
		return -EINVAL;
	}

	if (!(BIT(channel_id) & config->channel_mask)) {
		LOG_ERR("Channel %d is not available", channel_id);
		return -EINVAL;
	}

	if (enabled) {
		data->enabled_channel_mask |= BIT(channel_id);
	} else {
		data->enabled_channel_mask &= ~BIT(channel_id);
	}

	return 0;
}

static int renesas_ra_ipc_init(const struct device *dev)
{
	struct renesas_ra_ipc_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;

	fsp_err = R_IPC_Open(&data->ipc_ctrl, &data->fsp_config);

	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("MBOX initialization failed");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(mbox, renesas_ra_ipc_driver_api) = {
	.send = renesas_ra_ipc_send,
	.register_callback = renesas_ra_ipc_reg_callback,
	.mtu_get = renesas_ra_ipc_mtu_get,
	.max_channels_get = renesas_ra_ipc_max_channels_get,
	.set_enabled = renesas_ra_ipc_set_enabled,
};

#define IPC_RENESAS_RA_IRQ_INIT(idx)                                                               \
	{                                                                                          \
		R_ICU->IELSR_b[DT_INST_IRQ(idx, irq)].IELS =                                       \
			BSP_PRV_IELS_ENUM(CONCAT(EVENT_IPC_IRQ, DT_INST_PROP(idx, unit)));         \
		IRQ_CONNECT(DT_INST_IRQ(idx, irq), DT_INST_IRQ(idx, priority), ipc_isr, NULL, 0);  \
		irq_enable(DT_INST_IRQ(idx, irq));                                                 \
	}

#define IPC_RENESAS_RA_INIT(idx)                                                                   \
	static struct renesas_ra_ipc_data ipc_renesas_ra_data_##idx = {                            \
		.fsp_config =                                                                      \
			{                                                                          \
				.channel = DT_INST_PROP(idx, unit),                                \
				.irq = DT_INST_IRQ(idx, irq),                                      \
				.ipl = DT_INST_IRQ(idx, priority),                                 \
				.p_callback = mbox_renesas_ra_callback,                            \
				.p_context = (void *)DEVICE_DT_INST_GET(idx),                      \
			},                                                                         \
	};                                                                                         \
	static const struct renesas_ra_ipc_config ipc_renesas_ra_config_##idx = {                  \
		.channel_mask = DT_INST_PROP(idx, channel_mask),                                   \
		.message_channel = DT_INST_PROP(idx, message_channel_select),                      \
	};                                                                                         \
	static int ipc_renesas_ra_init##idx(const struct device *dev)                              \
	{                                                                                          \
		int err = renesas_ra_ipc_init(dev);                                                \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		IPC_RENESAS_RA_IRQ_INIT(idx);                                                      \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(idx, ipc_renesas_ra_init##idx, NULL, &ipc_renesas_ra_data_##idx,     \
			      &ipc_renesas_ra_config_##idx, POST_KERNEL,                           \
			      CONFIG_MBOX_INIT_PRIORITY, &renesas_ra_ipc_driver_api)

DT_INST_FOREACH_STATUS_OKAY(IPC_RENESAS_RA_INIT);
