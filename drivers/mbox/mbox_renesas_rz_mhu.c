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

LOG_MODULE_REGISTER(mbox_renesas_rz_mhu, CONFIG_MBOX_LOG_LEVEL);

/* Global dummy value required for FSP driver implementation */
#define MHU_SHM_START_ADDR 0
const uint32_t *const __mhu_shmem_start = (uint32_t *)MHU_SHM_START_ADDR;

/* FSP interrupt handlers. */
void mhu_ns_int_isr(void);

static volatile uint32_t callback_msg;
static void mhu_ns_callback(mhu_callback_args_t *p_args)
{
	callback_msg = p_args->msg;
}

struct mbox_rz_mhu_config {
	const mhu_api_t *fsp_api;
	uint16_t mhu_ch_size;
	/* Number of supported channels */
	uint32_t num_channels;
	/* TX channels mask */
	uint32_t tx_mask;
	/* RX channels mask */
	uint32_t rx_mask;
};

struct mbox_rz_mhu_data {
	const struct device *dev;
	mhu_ns_instance_ctrl_t *fsp_ctrl;
	mhu_cfg_t *fsp_cfg;
	mbox_callback_t cb;
	void *user_data;
	uint32_t channel_id;
};

/**
 * @brief Return true if the channel of the MBOX device is an inbound channel.
 */
static inline bool is_rx_channel_valid(const struct device *dev, uint32_t ch)
{
	const struct mbox_rz_mhu_config *config = dev->config;

	return ((ch < config->num_channels) && (config->rx_mask & BIT(ch)));
}

/**
 * @brief Return true if the channel of the MBOX device is an outbound channel.
 */
static inline bool is_tx_channel_valid(const struct device *dev, uint32_t ch)
{
	const struct mbox_rz_mhu_config *config = dev->config;

	return ((ch < config->num_channels) && (config->tx_mask & BIT(ch)));
}

/**
 * Interrupt handler
 */
static void mbox_rz_mhu_isr(const struct device *dev)
{
	struct mbox_rz_mhu_data *data = dev->data;
	struct mbox_msg msg;

	mhu_ns_int_isr();
	if (data->cb && data->fsp_cfg->p_shared_memory) {
		uint32_t local_msg = callback_msg;

		msg.data = &local_msg;

		/* On the receiving end, the size of the message is always 4 bytes since the FSP MHU
		 * driver requires the message to be of type uint32_t
		 */
		msg.size = sizeof(local_msg);

		data->cb(dev, data->channel_id, data->user_data, &msg);
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

	if (!is_tx_channel_valid(dev, channel_id)) {
		if (!is_rx_channel_valid(dev, channel_id)) {
			/* Channel is neither RX nor TX */
			LOG_ERR("Invalid MBOX channel number: %d", channel_id);
			return -EINVAL;
		}

		/* Channel is a RX channel, but this function only accepts TX */
		LOG_ERR("Channel ID %d is a RX channel, but only TX channels are allowed",
			channel_id);
		return -ENOSYS;
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

	if (data->fsp_cfg->p_shared_memory) {

#if CONFIG_MBOX_BUSY_WAIT_TIMEOUT_US > 0
		/* The FSP MHU "msgSend" API continuously polls until the
		 * previous message is consumed before sending a new one. To avoid
		 * blocking indefinitely, we need to check if the remote clears the message
		 * within the allowed time before sending a new one
		 */
		if (MHU_SEND_TYPE_MSG == data->fsp_ctrl->send_type) {
			if (data->fsp_ctrl->p_regs->MSG_INT_STSn != 0) {
				k_busy_wait(CONFIG_MBOX_BUSY_WAIT_TIMEOUT_US);
				if (data->fsp_ctrl->p_regs->MSG_INT_STSn != 0) {
					LOG_ERR("Remote is busy");
					return -EBUSY;
				}
			}
		} else {
			if (data->fsp_ctrl->p_regs->RSP_INT_STSn != 0) {
				k_busy_wait(CONFIG_MBOX_BUSY_WAIT_TIMEOUT_US);
				if (data->fsp_ctrl->p_regs->RSP_INT_STSn != 0) {
					LOG_ERR("Remote is busy");
					return -EBUSY;
				}
			}
		}
#endif

		/* Send message to shared memory, this will also invoke interrupt on the receiving
		 * core
		 */
		fsp_err = config->fsp_api->msgSend(data->fsp_ctrl, message);
	}

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

	if (!is_rx_channel_valid(dev, channel_id)) {
		if (!is_tx_channel_valid(dev, channel_id)) {
			/* Channel is neither RX nor TX */
			LOG_ERR("Invalid MBOX channel number: %d", channel_id);
			return -EINVAL;
		}

		/* Channel is a TX channel, but this function only accepts RX */
		LOG_ERR("Channel ID %d is a TX channel, but only RX channels are allowed",
			channel_id);
		return -ENOSYS;
	}

	if (!cb) {
		LOG_ERR("Must provide callback");
		return -EINVAL;
	}

	data->cb = cb;
	data->user_data = user_data;
	data->channel_id = channel_id;

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

	fsp_err = config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);

	if (fsp_err) {
		LOG_ERR("MBOX initialization failed");
		return -EIO;
	}

	return 0;
}

/**
 * @brief Enable (disable) interrupts and callbacks for inbound channels.
 */
static int mbox_rz_mhu_set_enabled(const struct device *dev, mbox_channel_id_t channel_id,
				   bool enabled)
{
	if (!is_rx_channel_valid(dev, channel_id)) {
		if (!is_tx_channel_valid(dev, channel_id)) {
			/* Channel is neither RX nor TX */
			LOG_ERR("Invalid MBOX channel number: %d", channel_id);
			return -EINVAL;
		}

		/* Channel is a TX channel, but this function only accepts RX */
		LOG_ERR("Channel ID %d is a TX channel, but only RX channels are allowed",
			channel_id);
		return -ENOSYS;
	}

	ARG_UNUSED(enabled);
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

#define MHU_RZG_IRQ_CONNECT(idx, irq_name, isr)                                                    \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, irq_name, irq),                               \
			    DT_INST_IRQ_BY_NAME(idx, irq_name, priority), isr,                     \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, irq_name, irq));                               \
	} while (0)

#define MHU_RZG_CONFIG_FUNC(idx) MHU_RZG_IRQ_CONNECT(idx, mhuns, mbox_rz_mhu_isr);

#define MHU_RZG_INIT(idx)                                                                          \
	static mhu_ns_instance_ctrl_t g_mhu_ns##idx##_ctrl;                                        \
	static mhu_cfg_t g_mhu_ns##idx##_cfg = {                                                   \
		.channel = DT_INST_PROP(idx, channel),                                             \
		.rx_ipl = DT_INST_IRQ_BY_NAME(idx, mhuns, priority),                               \
		.rx_irq = DT_INST_IRQ_BY_NAME(idx, mhuns, irq),                                    \
		.p_callback = mhu_ns_callback,                                                     \
		.p_context = NULL,                                                                 \
		.p_shared_memory = (void *)COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, shared_memory),  \
		      (DT_REG_ADDR(DT_INST_PHANDLE(idx, shared_memory))), (NULL)),                 \
	};                                                                                         \
	static const struct mbox_rz_mhu_config mbox_rz_mhu_config_##idx = {                        \
		.fsp_api = &g_mhu_ns_on_mhu_ns,                                                    \
		.mhu_ch_size = 4,                                                                  \
		.num_channels = DT_INST_PROP(idx, channels_count),                                 \
		.tx_mask = DT_INST_PROP(idx, tx_mask),                                             \
		.rx_mask = DT_INST_PROP(idx, rx_mask),                                             \
	};                                                                                         \
	static struct mbox_rz_mhu_data mbox_rz_mhu_data_##idx = {                                  \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
		.fsp_ctrl = &g_mhu_ns##idx##_ctrl,                                                 \
		.fsp_cfg = &g_mhu_ns##idx##_cfg,                                                   \
	};                                                                                         \
	static int mbox_rz_mhu_init_##idx(const struct device *dev)                                \
	{                                                                                          \
		MHU_RZG_CONFIG_FUNC(idx)                                                           \
		return mbox_rz_mhu_init(dev);                                                      \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(idx, mbox_rz_mhu_init_##idx, NULL, &mbox_rz_mhu_data_##idx,          \
			      &mbox_rz_mhu_config_##idx, PRE_KERNEL_1,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mbox_rz_mhu_driver_api)

DT_INST_FOREACH_STATUS_OKAY(MHU_RZG_INIT);
