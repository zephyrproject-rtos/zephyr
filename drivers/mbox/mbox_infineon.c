/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief MBOX driver for supported Infineon MCU platforms.
 *
 * MBOX driver implementation based on PDL IPC PIPE layer.
 * A pipe is a communication channel between two endpoints.
 *
 * On a edge family devices, there are two IPC instances (IPC0 and IPC1).
 * Each instance supports 16 IPC channels and 8 IPC interrupts.
 * PDL driver handles instances by having one channel pool - from 0 to 31,
 * where first 16 belong to IPC0 and second 16 channels belong to IPC1.
 * Following IPC resources are for application:
 *   IPC0 CH  - 3 to 7 and 11 to 15
 *   IPC0 INT - 3 to 5
 *   IPC1 CH  - 17 to 31
 *   IPC1 INT - 9 to 15
 *
 */

#define DT_DRV_COMPAT infineon_mbox_pipe

#include <zephyr/kernel.h>

#include <zephyr/drivers/mbox.h>
#include <cy_pdl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_mbox, CONFIG_MBOX_LOG_LEVEL);

#define IFX_MBOX_CHAN_NUM (CONFIG_MBOX_INFINEON_CHANNEL_NUM)
#define IFX_MBOX_MTU_SIZE (0xffffffff) /* size_t */

/* IPC PIPE massage structure */
typedef struct {
	uint8_t client_id;
	uint8_t cpu_status;
	uint16_t intr_mask;
	void *data;
	size_t data_size;
} __packed __aligned(4) ipc_pipe_msg_t;

/* Device config structure */
struct ifx_mbox_config {
	uint32_t pipe_intr_mask;
	cy_stc_ipc_pipe_config_t pipe_config;
	cy_ipc_pipe_callback_ptr_t pipe_cb;
	cy_ipc_pipe_relcallback_ptr_t rel_cb;
	ipc_pipe_msg_t *ipc_msg;
};

/* Data structure */
struct ifx_mbox_data {
	/* Info for registration cback (zephyr implementation) */
	mbox_callback_t user_cb[IFX_MBOX_CHAN_NUM];
	void *user_cb_data[IFX_MBOX_CHAN_NUM];

	/* Mbox driver implementation data */
	uint32_t enabled_mask;
	uint8_t active_tx_channel;
	uint8_t ep_addr;
	uint8_t ep_r_addr;
	struct k_sem tx_semaphore;
};

static int ifx_mbox_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	cy_en_ipc_pipe_status_t rtl = CY_IPC_PIPE_SUCCESS;
	struct ifx_mbox_data *data = dev->data;
	const struct ifx_mbox_config *config = dev->config;

	if (channel >= IFX_MBOX_CHAN_NUM) {
		return -EINVAL;
	}

	/* Check that the IPC-channel is available before modifying ipc_msg */
	if (k_sem_take(&data->tx_semaphore, K_NO_WAIT) == -EBUSY) {
		return -EBUSY;
	}

	ipc_pipe_msg_t *ipc_msg = &config->ipc_msg[channel];

	ipc_msg->client_id = channel;
	ipc_msg->intr_mask = config->pipe_intr_mask;
	ipc_msg->data = (void *)msg->data;
	ipc_msg->data_size = msg->size;

	uint32_t to_ep_addr = (data->enabled_mask & BIT(channel)) ? data->ep_addr : data->ep_r_addr;

	data->active_tx_channel = channel;

	rtl = Cy_IPC_Pipe_SendMessage(to_ep_addr, data->ep_addr, (void *)ipc_msg, config->rel_cb);

	if (rtl != CY_IPC_PIPE_SUCCESS) {
		data->active_tx_channel = 0xFF;
		k_sem_give(&data->tx_semaphore);
	}

	/*  Returns EBUSY if the remote hasn't yet read the last data sent. */
	if (rtl == CY_IPC_PIPE_ERROR_SEND_BUSY) {
		return -EBUSY;
	}

	return (rtl != CY_IPC_PIPE_SUCCESS) ? -EINVAL : 0;
}

static void ifx_mbox_callback_handler_wrapper(const struct device *dev, uint32_t *msg)
{
	ipc_pipe_msg_t *ipc_msg = (ipc_pipe_msg_t *)msg;
	struct ifx_mbox_data *data = dev->data;
	uint32_t channel = ipc_msg->client_id;

	if ((channel < IFX_MBOX_CHAN_NUM) && (data->user_cb[channel] != NULL)) {
		data->user_cb[channel](dev, channel, data->user_cb_data[channel],
				       (struct mbox_msg *)&ipc_msg->data);
	}
}

static void ifx_mbox_rel_callback_handler_wrapper(const struct device *dev)
{
	struct ifx_mbox_data *data = dev->data;
	uint8_t tx_channel = data->active_tx_channel;

	k_sem_give(&data->tx_semaphore);

	/* Clearing the active channel here to allow mbox_send invocation from the callback */
	data->active_tx_channel = 0xFF;

	if (data->user_cb[tx_channel] != NULL) {
		data->user_cb[tx_channel](dev, tx_channel, data->user_cb_data[tx_channel], NULL);
	}
}

static int ifx_mbox_register_callback(const struct device *dev, uint32_t channel,
				      mbox_callback_t cb, void *user_data)
{
	struct ifx_mbox_data *data = dev->data;

	if (channel >= IFX_MBOX_CHAN_NUM) {
		return -EINVAL;
	}

	data->user_cb[channel] = cb;
	data->user_cb_data[channel] = user_data;

	return 0;
}

static int ifx_mbox_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	cy_en_ipc_pipe_status_t rtl;
	struct ifx_mbox_data *data = dev->data;
	const struct ifx_mbox_config *config = dev->config;

	if (channel >= IFX_MBOX_CHAN_NUM) {
		return -EINVAL;
	}

	if ((enable == 0 && (!(data->enabled_mask & BIT(channel)))) ||
	    (enable != 0 && (data->enabled_mask & BIT(channel)))) {
		return -EALREADY;
	}

	if (enable && (data->user_cb[channel] == NULL)) {
		LOG_WRN("Enabling channel without a registered callback");
	}

	if (enable) {
		data->enabled_mask |= BIT(channel);
	} else {
		data->enabled_mask &= ~BIT(channel);
	}

	if (enable) {
		rtl = Cy_IPC_Pipe_RegisterCallback(data->ep_addr, config->pipe_cb, channel);
	} else {
		rtl = Cy_IPC_Pipe_RegisterCallback(data->ep_addr, NULL, channel);
	}

	return (rtl != CY_IPC_PIPE_SUCCESS) ? -EINVAL : 0;
}

static int ifx_mbox_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return IFX_MBOX_MTU_SIZE;
}

static uint32_t ifx_mbox_max_channels_get(const struct device *dev)
{
	return IFX_MBOX_CHAN_NUM;
}

static int ifx_mbox_init(const struct device *dev)
{
	struct ifx_mbox_data *data = dev->data;
	const struct ifx_mbox_config *config = dev->config;

	data->ep_addr = config->pipe_config.ep0ConfigData.epAddress;
	data->ep_r_addr = config->pipe_config.ep1ConfigData.epAddress;

	Cy_IPC_Pipe_Init(&config->pipe_config);

	k_sem_init(&data->tx_semaphore, 1, 1);

	return 0;
};

static DEVICE_API(mbox, ifx_mbox_api) = {
	.send = ifx_mbox_send,
	.register_callback = ifx_mbox_register_callback,
	.mtu_get = ifx_mbox_mtu_get,
	.max_channels_get = ifx_mbox_max_channels_get,
	.set_enabled = ifx_mbox_set_enabled,
};

#define _MBOX_IPC_PIPE_EP0_ADDR(inst) DT_INST_PROP(inst, ipc_pipe_ep0_addr)
#define _MBOX_IPC_PIPE_EP1_ADDR(inst) DT_INST_PROP(inst, ipc_pipe_ep1_addr)

#define _MBOX_IPC_PIPE_EP0_CHAN(inst)                                                              \
	DT_PHA_BY_IDX(DT_INST(inst, DT_DRV_COMPAT), ipc_configs, 0, channel)
#define _MBOX_IPC_PIPE_EP1_CHAN(inst)                                                              \
	DT_PHA_BY_IDX(DT_INST(inst, DT_DRV_COMPAT), ipc_configs, 1, channel)

#define _MBOX_IPC_PIPE_EP0_INTR(inst)                                                              \
	DT_PHA_BY_IDX(DT_INST(inst, DT_DRV_COMPAT), ipc_configs, 0, interrupt)
#define _MBOX_IPC_PIPE_EP0_INTR_PRIOR(inst)                                                        \
	DT_PHA_BY_IDX(DT_INST(inst, DT_DRV_COMPAT), ipc_configs, 0, interrupt_prior)
#define _MBOX_IPC_PIPE_INTR_MUX(inst) DT_INST_PROP_OR(inst, cm0_intr_mux, 2)

#define _MBOX_IPC_PIPE_EP1_INTR(inst)                                                              \
	DT_PHA_BY_IDX(DT_INST(inst, DT_DRV_COMPAT), ipc_configs, 1, interrupt)
#define _MBOX_IPC_PIPE_EP1_INTR_PRIOR(inst)                                                        \
	DT_PHA_BY_IDX(DT_INST(inst, DT_DRV_COMPAT), ipc_configs, 1, interrupt_prior)

#ifndef CY_IPC_INTERRUPTS_PER_INSTANCE
#define CY_IPC_INTERRUPTS_PER_INSTANCE (16)
#endif

#define _MBOX_PIPE_INTR_MASK(inst)                                                                 \
	(CY_IPC_CH_MASK(_MBOX_IPC_PIPE_EP0_CHAN(inst)) |                                           \
	 CY_IPC_CH_MASK(_MBOX_IPC_PIPE_EP1_CHAN(inst)))

#if (CY_IPC_INSTANCES > 1U)
#define _MBOX_PIPE_EP0_CONFIG(inst)                                                                \
	{.epChannel = _MBOX_IPC_PIPE_EP0_CHAN(inst),                                               \
	 .epIntr = _MBOX_IPC_PIPE_EP0_INTR(inst),                                                  \
	 .epIntrmask = _MBOX_PIPE_INTR_MASK(inst)}

#define _MBOX_PIPE_EP1_CONFIG(inst)                                                                \
	{.epChannel = _MBOX_IPC_PIPE_EP1_CHAN(inst),                                               \
	 .epIntr = _MBOX_IPC_PIPE_EP1_INTR(inst),                                                  \
	 .epIntrmask = _MBOX_PIPE_INTR_MASK(inst)}
#else
#define _MBOX_PIPE_EP0_CONFIG(inst)                                                                \
	(_VAL2FLD(CY_IPC_PIPE_CFG_IMASK, _MBOX_PIPE_INTR_MASK(inst)) |                             \
	 _VAL2FLD(CY_IPC_PIPE_CFG_INTR, _MBOX_IPC_PIPE_EP0_INTR(inst)) |                           \
	 _VAL2FLD(CY_IPC_PIPE_CFG_CHAN, _MBOX_IPC_PIPE_EP0_CHAN(inst)))

#define _MBOX_PIPE_EP1_CONFIG(inst)                                                                \
	(_VAL2FLD(CY_IPC_PIPE_CFG_IMASK, _MBOX_PIPE_INTR_MASK(inst)) |                             \
	 _VAL2FLD(CY_IPC_PIPE_CFG_INTR, _MBOX_IPC_PIPE_EP1_INTR(inst)) |                           \
	 _VAL2FLD(CY_IPC_PIPE_CFG_CHAN, _MBOX_IPC_PIPE_EP1_CHAN(inst)))
#endif

#if defined(CONFIG_CPU_CORTEX_M4) || defined(CONFIG_CPU_CORTEX_M33)
#define _MBOX_IPC_PIPE_RECEIVER_INTR(n)       _MBOX_IPC_PIPE_EP0_INTR(n)
#define _MBOX_IPC_PIPE_RECEIVER_INTR_PRIOR(n) _MBOX_IPC_PIPE_EP0_INTR_PRIOR(n)
#define _MBOX_IPC_PIPE_RECEIVER_INTR_MUX(n)   CY_IPC_INTR_MUX(_MBOX_IPC_PIPE_EP0_INTR(n))
#define _MBOX_IPC_PIPE_RECEIVER_ADDR(n)       _MBOX_IPC_PIPE_EP0_ADDR(n)
#define _MBOX_PIPE_RECEIVER_CONFIG(n)         _MBOX_PIPE_EP0_CONFIG(n)

#define _MBOX_IPC_PIPE_SENDER_INTR(n)       _MBOX_IPC_PIPE_EP1_INTR(n)
#define _MBOX_IPC_PIPE_SENDER_INTR_PRIOR(n) _MBOX_IPC_PIPE_EP1_INTR_PRIOR(n)
#define _MBOX_IPC_PIPE_SENDER_INTR_MUX(n)   CY_IPC_INTR_MUX(_MBOX_IPC_PIPE_EP1_INTR(n))
#define _MBOX_IPC_PIPE_SENDER_ADDR(n)       _MBOX_IPC_PIPE_EP1_ADDR(n)
#define _MBOX_PIPE_SENDER_CONFIG(n)         _MBOX_PIPE_EP1_CONFIG(n)
#define _MBOX_PIPE_INTR_MASK_EP(n)          CY_IPC_INTR_MASK(_MBOX_IPC_PIPE_EP0_INTR(n))
#else
#define _MBOX_IPC_PIPE_RECEIVER_INTR(n)       _MBOX_IPC_PIPE_EP1_INTR(n)
#define _MBOX_IPC_PIPE_RECEIVER_INTR_PRIOR(n) _MBOX_IPC_PIPE_EP1_INTR_PRIOR(n)
#define _MBOX_IPC_PIPE_RECEIVER_INTR_MUX(n)   CY_IPC_INTR_MUX(_MBOX_IPC_PIPE_EP1_INTR(n))
#define _MBOX_IPC_PIPE_RECEIVER_ADDR(n)       _MBOX_IPC_PIPE_EP1_ADDR(n)
#define _MBOX_PIPE_RECEIVER_CONFIG(n)         _MBOX_PIPE_EP1_CONFIG(n)

#define _MBOX_IPC_PIPE_SENDER_INTR(n)       _MBOX_IPC_PIPE_EP0_INTR(n)
#define _MBOX_IPC_PIPE_SENDER_INTR_PRIOR(n) _MBOX_IPC_PIPE_EP0_INTR_PRIOR(n)
#define _MBOX_IPC_PIPE_SENDER_INTR_MUX(n)   CY_IPC_INTR_MUX(_MBOX_IPC_PIPE_EP0_INTR(n))
#define _MBOX_IPC_PIPE_SENDER_ADDR(n)       _MBOX_IPC_PIPE_EP0_ADDR(n)
#define _MBOX_PIPE_SENDER_CONFIG(n)         _MBOX_PIPE_EP0_CONFIG(n)
#define _MBOX_PIPE_INTR_MASK_EP(n)          CY_IPC_INTR_MASK(_MBOX_IPC_PIPE_EP1_INTR(n))
#endif

#define INFINEON_MBOX_INIT(n)                                                                      \
	static void ifx_mbox##n##_ipc_pipe_isr(void);                                              \
	static void ifx_mbox##n##_callback_handler(uint32_t *msg);                                 \
	static void ifx_mbox##n##_rel_callback_handler(void);                                      \
	static ipc_pipe_msg_t ifx_mbox##n##ipc_msg[IFX_MBOX_CHAN_NUM] CY_SECTION_SHAREDMEM;        \
                                                                                                   \
	cy_ipc_pipe_callback_ptr_t ifx_mbox##n##_ipc_pipe_cb_array[IFX_MBOX_CHAN_NUM];             \
	static const struct ifx_mbox_config _ifx_mbox##n##_config = {                              \
		.pipe_config =                                                                     \
			{                                                                          \
				.ep0ConfigData =                                                   \
					{                                                          \
						.ipcNotifierNumber =                               \
							_MBOX_IPC_PIPE_RECEIVER_INTR(n),           \
						.ipcNotifierPriority =                             \
							_MBOX_IPC_PIPE_RECEIVER_INTR_PRIOR(n),     \
						.ipcNotifierMuxNumber =                            \
							_MBOX_IPC_PIPE_RECEIVER_INTR_MUX(n),       \
						.epAddress = _MBOX_IPC_PIPE_RECEIVER_ADDR(n),      \
						.epConfig = _MBOX_PIPE_RECEIVER_CONFIG(n),         \
					},                                                         \
				.ep1ConfigData =                                                   \
					{                                                          \
						.ipcNotifierNumber =                               \
							_MBOX_IPC_PIPE_SENDER_INTR(n),             \
						.ipcNotifierPriority =                             \
							_MBOX_IPC_PIPE_SENDER_INTR_PRIOR(n),       \
						.ipcNotifierMuxNumber =                            \
							_MBOX_IPC_PIPE_SENDER_INTR_MUX(n),         \
						.epAddress = _MBOX_IPC_PIPE_SENDER_ADDR(n),        \
						.epConfig = _MBOX_PIPE_SENDER_CONFIG(n),           \
					},                                                         \
                                                                                                   \
				.endpointClientsCount = IFX_MBOX_CHAN_NUM,                         \
				.endpointsCallbacksArray = ifx_mbox##n##_ipc_pipe_cb_array,        \
				.userPipeIsrHandler = &ifx_mbox##n##_ipc_pipe_isr,                 \
			},                                                                         \
		.pipe_intr_mask = _MBOX_PIPE_INTR_MASK_EP(n),                                      \
		.pipe_cb = ifx_mbox##n##_callback_handler,                                         \
		.rel_cb = ifx_mbox##n##_rel_callback_handler,                                      \
		.ipc_msg = ifx_mbox##n##ipc_msg,                                                   \
	};                                                                                         \
	static struct ifx_mbox_data _ifx_mbox##n##_data = {.active_tx_channel = 0xFF};             \
                                                                                                   \
	static void ifx_mbox##n##_ipc_pipe_isr(void)                                               \
	{                                                                                          \
		Cy_IPC_Pipe_ExecuteCallback(_ifx_mbox##n##_data.ep_addr);                          \
	}                                                                                          \
                                                                                                   \
	static void ifx_mbox##n##_callback_handler(uint32_t *msg)                                  \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(n);                                  \
		ifx_mbox_callback_handler_wrapper(dev, msg);                                       \
	}                                                                                          \
                                                                                                   \
	static void ifx_mbox##n##_rel_callback_handler(void)                                       \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(n);                                  \
		ifx_mbox_rel_callback_handler_wrapper(dev);                                        \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_mbox_init, NULL, &_ifx_mbox##n##_data,                       \
			      &_ifx_mbox##n##_config, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ifx_mbox_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_MBOX_INIT)

cy_en_sysint_status_t Cy_SysInt_Init(const cy_stc_sysint_t *config, cy_israddress userIsr)
{
	CY_ASSERT_L3(CY_SYSINT_IS_PRIORITY_VALID(config->intrPriority));
	cy_en_sysint_status_t status = CY_SYSINT_SUCCESS;

	/* The interrupt vector will be relocated only if the vector table was
	 * moved to SRAM (CONFIG_DYNAMIC_INTERRUPTS and CONFIG_GEN_ISR_TABLES
	 * must be enabled). Otherwise it is ignored.
	 */

#if defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES)
	if (config != NULL) {
		uint32_t priority;

		/* NOTE:
		 * PendSV IRQ (which is used in Cortex-M variants to implement thread
		 * context-switching) is assigned the lowest IRQ priority level.
		 * If priority is same as PendSV, we will catch assertion in
		 * z_arm_irq_priority_set function. To avoid this, change priority
		 * to IRQ_PRIO_LOWEST, if it > IRQ_PRIO_LOWEST. Macro IRQ_PRIO_LOWEST
		 * takes in to account PendSV specific.
		 */
		priority = (config->intrPriority > IRQ_PRIO_LOWEST) ? IRQ_PRIO_LOWEST
								    : config->intrPriority;

		/* Configure a dynamic interrupt */
		(void)irq_connect_dynamic(config->intrSrc, priority, (void *)userIsr, NULL, 0);
	} else {
		status = CY_SYSINT_BAD_PARAM;
	}
#endif /* defined(CONFIG_DYNAMIC_INTERRUPTS) && defined(CONFIG_GEN_ISR_TABLES) */

	return (status);
}
