/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_mhu

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include "r_mhu_ns.h"

LOG_MODULE_REGISTER(ipm_renesas_rz, CONFIG_IPM_LOG_LEVEL);

#define MHU_MAX_ID_VAL 0

/* Global dummy value required for FSP driver implementation */
#define MHU_SHM_START_ADDR 0
const uint32_t *const __mhu_shmem_start = (uint32_t *)MHU_SHM_START_ADDR;

/* FSP interrupt handlers. */
void mhu_ns_int_isr(void);
static volatile uint32_t callback_msg;

void mhu_ns_callback(mhu_callback_args_t *p_args)
{
	callback_msg = p_args->msg;
}

struct mhu_rz_config {
	const mhu_api_t *fsp_api;
	uint16_t mhu_ch_size;
};

struct mhu_rz_data {
	const struct device *dev;
	mhu_ns_instance_ctrl_t *fsp_ctrl;
	mhu_cfg_t *fsp_cfg;
	ipm_callback_t cb;
	void *user_data;
};

/**
 * Interrupt handler
 */
static void mhu_rz_isr(const struct device *dev)
{
	struct mhu_rz_data *data = dev->data;

	mhu_ns_int_isr();
	if (data->cb && data->fsp_cfg->p_shared_memory) {
		data->cb(dev, data->user_data, 0, &callback_msg);
	}
}

/**
 * @brief Try to send a message over the IPM device.
 */
static int mhu_rz_send(const struct device *dev, int wait, uint32_t cpu_id, const void *buf,
		       int size)
{
	const struct mhu_rz_config *config = dev->config;
	struct mhu_rz_data *data = dev->data;
	fsp_ip_t fsp_err = FSP_SUCCESS;

	ARG_UNUSED(wait);
	/* Maximum size allowed is 4 bytes */
	if (size > config->mhu_ch_size) {
		LOG_ERR("Size %d is not valid. Maximum size is 4 bytes", size);
		return -EINVAL;
	}

	/* FSP driver implementation requires the message to be of type uint32_t */
	uint32_t message = 0;

	if (data->fsp_cfg->p_shared_memory) {
		if (buf && size) {
			/* Copy message */
			memcpy(&message, buf, size);
		} else {
			/*Clear message*/
			message = 0;
		}

		/* Send message to shared memory*/
		fsp_err = config->fsp_api->msgSend(data->fsp_ctrl, message);
	}

	if (fsp_err) {
		return -EIO;
	}

	return 0;
}

/**
 * @brief Register a callback function for incoming messages.
 */
static void mhu_rz_reg_callback(const struct device *dev, ipm_callback_t cb, void *user_data)
{
	struct mhu_rz_data *data = dev->data;

	data->cb = cb;
	data->user_data = user_data;
}

/**
 * @brief Initialize the module.
 */
static int mhu_rz_init(const struct device *dev)
{
	const struct mhu_rz_config *config = dev->config;
	struct mhu_rz_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;

	fsp_err = config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);

	if (fsp_err) {
		return -EIO;
	}

	return 0;
}

/**
 * @brief Enable interrupts and callbacks for inbound channels. Not implemented.
 */
static int mhu_rz_set_enabled(const struct device *dev, int enable)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(enable);
	return 0;
}

/**
 * @return Maximum possible size of a message in bytes.
 */
static int mhu_rz_max_data_size_get(const struct device *dev)
{
	const struct mhu_rz_config *config = dev->config;

	return config->mhu_ch_size;
}

/**
 * @return Maximum possible value of a message ID. Not implemented.
 */
static uint32_t mhu_rz_max_id_val_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return MHU_MAX_ID_VAL;
}

static DEVICE_API(ipm, mhu_rz_driver_api) = {
	.send = mhu_rz_send,
	.register_callback = mhu_rz_reg_callback,
	.max_data_size_get = mhu_rz_max_data_size_get,
	.max_id_val_get = mhu_rz_max_id_val_get,
	.set_enabled = mhu_rz_set_enabled,
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

#define MHU_RZG_CONFIG_FUNC(idx) MHU_RZG_IRQ_CONNECT(idx, mhuns, mhu_rz_isr);

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
	static const struct mhu_rz_config mhu_rz_config_##idx = {                                  \
		.fsp_api = &g_mhu_ns_on_mhu_ns,                                                    \
		.mhu_ch_size = 4,                                                                  \
	};                                                                                         \
	static struct mhu_rz_data mhu_rz_data_##idx = {                                            \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
		.fsp_ctrl = &g_mhu_ns##idx##_ctrl,                                                 \
		.fsp_cfg = &g_mhu_ns##idx##_cfg,                                                   \
	};                                                                                         \
	static int mhu_rz_init_##idx(const struct device *dev)                                     \
	{                                                                                          \
		MHU_RZG_CONFIG_FUNC(idx)                                                           \
		return mhu_rz_init(dev);                                                           \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(idx, mhu_rz_init_##idx, NULL, &mhu_rz_data_##idx,                    \
			      &mhu_rz_config_##idx, PRE_KERNEL_1,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mhu_rz_driver_api)

DT_INST_FOREACH_STATUS_OKAY(MHU_RZG_INIT);
