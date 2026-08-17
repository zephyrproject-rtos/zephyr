/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_mhu_mbox

#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/logging/log.h>

#include "r_mhu_ns.h"
#define MHU_MAX_CHANNELS 1
void mhu_ns_int_isr(void);

LOG_MODULE_REGISTER(mbox_renesas_rz_mhu, CONFIG_MBOX_LOG_LEVEL);

static volatile uint32_t callback_msg;
static void mhu_ns_callback(mhu_callback_args_t *p_args)
{
	callback_msg = p_args->msg;
}

/* Structure that handle the context of the ISR */
typedef struct rz_mhu_irq_ctx {
	const struct device *dev;
	uint32_t channel_id;
} rz_mhu_irq_ctx_t;

/* Structure that handle FSP MHU instances */
typedef struct rz_mhu_channel {
	mhu_ns_instance_ctrl_t fsp_ctrl;
	mhu_cfg_t fsp_cfg;
} rz_mhu_channel_t;

struct mbox_rz_mhu_config {
	const mhu_api_t *fsp_api;
	uint16_t mhu_ch_size;
	/* Number of supported channels */
	uint32_t num_channels;
	/* Bitmask of available valid channels */
	uint32_t channel_mask;
};

struct mbox_rz_mhu_data {
	rz_mhu_channel_t *channels;
	mbox_callback_t cb[MHU_MAX_CHANNELS];
	void *user_data[MHU_MAX_CHANNELS];
	/* Bitmask of enabled channels */
	uint32_t enabled_channel_mask;
};

/**
 * @brief Return true if the channel of the MBOX device is valid.
 */
static inline bool is_channel_valid(const struct device *dev, uint32_t ch)
{
	const struct mbox_rz_mhu_config *config = dev->config;

	return ((ch < config->num_channels) && (config->channel_mask & BIT(ch)));
}

/**
 * Interrupt handler
 */
static void mbox_rz_mhu_isr(const void *context)
{
	const rz_mhu_irq_ctx_t *ctx = context;
	uint32_t channel_id = ctx->channel_id;
	const struct device *dev = ctx->dev;
	struct mbox_rz_mhu_data *data = dev->data;
	struct mbox_msg msg;

	if (!(data->enabled_channel_mask & BIT(channel_id))) {
		return;
	}

	mhu_ns_int_isr();

	if (data->cb[channel_id]) {
		uint32_t local_msg = callback_msg;

		msg.data = &local_msg;

		/* On the receiving end, the size of the message is always 4 bytes since the FSP MHU
		 * driver requires the message to be of type uint32_t
		 */
		msg.size = sizeof(local_msg);

		data->cb[channel_id](dev, channel_id, data->user_data[channel_id], &msg);
	}
}

/**
 * @brief Try to send a message over the MBOX device.
 */
static int mbox_rz_mhu_send(const struct device *dev, mbox_channel_id_t channel_id,
			    const struct mbox_msg *msg)
{
	const struct mbox_rz_mhu_config *config = dev->config;
	struct mbox_rz_mhu_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;

	/* FSP driver implementation requires the message to be of type uint32_t */
	uint32_t message = 0;

	if (!is_channel_valid(dev, channel_id)) {
		LOG_ERR("Invalid MBOX channel number: %d", channel_id);
		return -EINVAL;
	}

	if (msg != NULL) {
		/* Maximum size allowed is 4 bytes */
		if (msg->size > config->mhu_ch_size) {
			LOG_ERR("Size %d is not valid. Maximum size is 4 bytes", msg->size);
			return -EMSGSIZE;
		}

		if (msg->data && msg->size) {
			/* Copy message */
			memcpy(&message, msg->data, msg->size);
		} else {
			/* Clear Message */
			message = 0;
		}
	} else {
		message = 0;
	}

#if CONFIG_MBOX_RENESAS_RZ_MHU_BUSY_WAIT_TIMEOUT_US > 0
	/* The FSP MHU "msgSend" API continuously polls until the
	 * previous message is consumed before sending a new one. To avoid
	 * blocking indefinitely, we need to check if the remote clears the message
	 * within the allowed time before sending a new one
	 */
	if (MHU_SEND_TYPE_MSG == data->channels[channel_id].fsp_ctrl.send_type) {
		if (data->channels[channel_id].fsp_ctrl.p_regs->MSG_INT_STSn != 0) {
			k_busy_wait(CONFIG_MBOX_RENESAS_RZ_MHU_BUSY_WAIT_TIMEOUT_US);
			if (data->channels[channel_id].fsp_ctrl.p_regs->MSG_INT_STSn != 0) {
				LOG_ERR("Remote is busy");
				return -EBUSY;
			}
		}
	} else {
		if (data->channels[channel_id].fsp_ctrl.p_regs->RSP_INT_STSn != 0) {
			k_busy_wait(CONFIG_MBOX_RENESAS_RZ_MHU_BUSY_WAIT_TIMEOUT_US);
			if (data->channels[channel_id].fsp_ctrl.p_regs->RSP_INT_STSn != 0) {
				LOG_ERR("Remote is busy");
				return -EBUSY;
			}
		}
	}
#endif

	/* Send message to shared memory, this will also invoke interrupt on the receiving
	 * core
	 */
	fsp_err = config->fsp_api->msgSend(&data->channels[channel_id].fsp_ctrl, message);

	if (fsp_err) {
		LOG_ERR("Message send failed");
		return -EIO;
	}

	return 0;
}

/**
 * @brief Register a callback function on a channel for incoming messages.
 */
static int mbox_rz_mhu_reg_callback(const struct device *dev, mbox_channel_id_t channel_id,
				    mbox_callback_t cb, void *user_data)
{
	struct mbox_rz_mhu_data *data = dev->data;

	if (!is_channel_valid(dev, channel_id)) {
		LOG_ERR("Invalid MBOX channel number: %d", channel_id);
		return -EINVAL;
	}

	if (!cb) {
		LOG_ERR("Must provide callback");
		return -EINVAL;
	}

	uint32_t lock = irq_lock();

	data->cb[channel_id] = cb;
	data->user_data[channel_id] = user_data;

	irq_unlock(lock);

	return 0;
}

/**
 * @brief Initialize the module.
 */
static int mbox_rz_mhu_init(const struct device *dev)
{
	const struct mbox_rz_mhu_config *config = dev->config;
	struct mbox_rz_mhu_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	uint32_t i;

	/* Open all channels inside a MBOX (MHU) device */
	for (i = 0; i < config->num_channels; i++) {
		fsp_err = config->fsp_api->open(&data->channels[i].fsp_ctrl,
						&data->channels[i].fsp_cfg);
		if (fsp_err) {
			LOG_ERR("MBOX initialization failed");
			goto error_close;
		}
	}

	return 0;

error_close:
	while (i > 0) {
		i--;
		config->fsp_api->close(&data->channels[i].fsp_ctrl);
	}
	return -EIO;
}

/**
 * @brief Enable (disable) interrupts and callbacks for inbound channels.
 */
static int mbox_rz_mhu_set_enabled(const struct device *dev, mbox_channel_id_t channel_id,
				   bool enabled)
{
	struct mbox_rz_mhu_data *data = dev->data;

	if (!is_channel_valid(dev, channel_id)) {
		LOG_ERR("Invalid MBOX channel number: %d", channel_id);
		return -EINVAL;
	}

	if (enabled == (bool)(data->enabled_channel_mask & BIT(channel_id))) {
		return -EALREADY;
	}

	if (enabled) {
		data->enabled_channel_mask |= BIT(channel_id);
	} else {
		data->enabled_channel_mask &= ~BIT(channel_id);
	}

	return 0;
}

/**
 * @brief Return the maximum number of bytes possible in an outbound message.
 */
static int mbox_rz_mhu_mtu_get(const struct device *dev)
{
	const struct mbox_rz_mhu_config *config = dev->config;

	return config->mhu_ch_size;
}

/**
 * @brief Return the maximum number of channels.
 */
static uint32_t mbox_rz_mhu_max_channels_get(const struct device *dev)
{
	const struct mbox_rz_mhu_config *config = dev->config;

	return config->num_channels;
}

static DEVICE_API(mbox, mbox_rz_mhu_driver_api) = {
	.send = mbox_rz_mhu_send,
	.register_callback = mbox_rz_mhu_reg_callback,
	.mtu_get = mbox_rz_mhu_mtu_get,
	.max_channels_get = mbox_rz_mhu_max_channels_get,
	.set_enabled = mbox_rz_mhu_set_enabled,
};

/*
 * ************************* DRIVER REGISTER SECTION ***************************
 */

#define MHU_RZ_IRQ_CONNECT(node_id, prop, i, inst)                                                 \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, i, irq), DT_IRQ_BY_IDX(node_id, i, priority),   \
			    mbox_rz_mhu_isr, &rz_mhu_irq_ctx_##inst[i], 0);                        \
		irq_enable(DT_IRQ_BY_IDX(node_id, i, irq));                                        \
	} while (0)

#define MHU_RZ_CONFIG_FUNC(inst)                                                                   \
	DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(inst, interrupt_names, MHU_RZ_IRQ_CONNECT, (;), inst)

#define RZ_MHU_INSTANCES_BY_IDX(node_id, prop, i, inst)                                            \
	{                                                                                          \
		.fsp_ctrl = {},                                                                    \
		.fsp_cfg = {                                                                       \
			.channel = DT_PROP(node_id, unit),                                         \
			.rx_ipl = DT_IRQ_BY_IDX(node_id, i, priority),                             \
			.rx_irq = DT_IRQ_BY_IDX(node_id, i, irq),                                  \
			.p_callback = mhu_ns_callback,                                             \
			.p_context = NULL,                                                         \
			.p_extend = &g_mhu_ns##inst##_cfg_extend,                                  \
			.p_shared_memory = (void *)COND_CODE_1(                                    \
			       DT_NODE_HAS_PROP(node_id, shared_memory),                           \
			      (DT_REG_ADDR(DT_PHANDLE(node_id, shared_memory))), (NULL)),          \
				},                                                                 \
			},

#define RZ_MHU_IRQ_CTX_BY_IDX(node_id, prop, i)                                                    \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.channel_id = i,                                                                   \
	},

#define MHU_RZ_INIT(inst)                                                                          \
	static const mhu_ns_extended_cfg_t g_mhu_ns##inst##_cfg_extend = {                         \
		.p_reg = (void *)DT_INST_REG_ADDR(inst),                                           \
	};                                                                                         \
	static rz_mhu_channel_t rz_mhu_channels_##inst[] = {DT_INST_FOREACH_PROP_ELEM_VARGS(       \
		inst, interrupt_names, RZ_MHU_INSTANCES_BY_IDX, inst)};                            \
	static rz_mhu_irq_ctx_t rz_mhu_irq_ctx_##inst[] = {                                        \
		DT_INST_FOREACH_PROP_ELEM(inst, interrupt_names, RZ_MHU_IRQ_CTX_BY_IDX)};          \
	static const struct mbox_rz_mhu_config mbox_rz_mhu_config_##inst = {                       \
		.fsp_api = &g_mhu_ns_on_mhu_ns,                                                    \
		.mhu_ch_size = 4,                                                                  \
		.num_channels = DT_INST_PROP(inst, channels_count),                                \
		.channel_mask = DT_INST_PROP(inst, channel_mask),                                  \
	};                                                                                         \
	static struct mbox_rz_mhu_data mbox_rz_mhu_data_##inst = {                                 \
		.channels = rz_mhu_channels_##inst,                                                \
	};                                                                                         \
	static int mbox_rz_mhu_init_##inst(const struct device *dev)                               \
	{                                                                                          \
		MHU_RZ_CONFIG_FUNC(inst);                                                          \
		return mbox_rz_mhu_init(dev);                                                      \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, mbox_rz_mhu_init_##inst, NULL, &mbox_rz_mhu_data_##inst,       \
			      &mbox_rz_mhu_config_##inst, POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY,  \
			      &mbox_rz_mhu_driver_api)

DT_INST_FOREACH_STATUS_OKAY(MHU_RZ_INIT);
