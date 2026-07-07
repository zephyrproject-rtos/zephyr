/*
 * Copyright (c) 2026 Linumiz.
 *
 * Infineon Mailbox driver for Zephyr's MBOX model.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "cy_ipc_drv.h"

LOG_MODULE_REGISTER(mbox_ifx_cat1);

#define DT_DRV_COMPAT infineon_cat1_mbox_pdl

#define MAILBOX_MBOX_SIZE	4
#define MAILBOX_MAX_CHANNELS	DT_INST_NUM_IRQS(0)

BUILD_ASSERT(DT_INST_PROP_LEN(0, ipc_channels) == MAILBOX_MAX_CHANNELS,
	     "ipc-channels length must match number of interrupts");
BUILD_ASSERT(DT_INST_PROP_LEN(0, ipc_intr_structs) == MAILBOX_MAX_CHANNELS,
	     "ipc-intr-structs length must match number of interrupts");

/* Mask helpers scoped per PDL IPC instance. */
#define IFX_MBOX_CH_MASK(ipc_ch)     (1UL << ((ipc_ch) % CY_IPC_CHANNELS_PER_INSTANCE))
#define IFX_MBOX_INTR_MASK(ipc_intr) (1UL << ((ipc_intr) % CY_IPC_INTERRUPTS_PER_INSTANCE))

struct ifx_cat1_mailbox_data {
	mbox_callback_t cb[MAILBOX_MAX_CHANNELS];
	void *user_data[MAILBOX_MAX_CHANNELS];
	bool channel_enable[MAILBOX_MAX_CHANNELS];
	uint32_t received_data[MAILBOX_MAX_CHANNELS];
};

struct ifx_cat1_mailbox_config {
	void (*irq_config)(void);
	const uint32_t channel_irq[MAILBOX_MAX_CHANNELS];
	const uint8_t ipc_channel[MAILBOX_MAX_CHANNELS];
	const uint8_t ipc_intr_struct[MAILBOX_MAX_CHANNELS];
};

static void ifx_cat1_mbox_handle_channel(const struct device *dev, uint32_t channel)
{
	const struct ifx_cat1_mailbox_config *cfg = dev->config;
	struct ifx_cat1_mailbox_data *data = dev->data;
	uint32_t ipc_ch = cfg->ipc_channel[channel];
	uint32_t ipc_intr = cfg->ipc_intr_struct[channel];
	IPC_INTR_STRUCT_Type *intr = Cy_IPC_Drv_GetIntrBaseAddr(ipc_intr);
	IPC_STRUCT_Type *ipc = Cy_IPC_Drv_GetIpcBaseAddress(ipc_ch);
	struct mbox_msg msg;

	if (!data->channel_enable[channel]) {
		Cy_IPC_Drv_ClearInterrupt(intr, CY_IPC_NO_NOTIFICATION,
					  IFX_MBOX_CH_MASK(ipc_ch));
		Cy_IPC_Drv_LockRelease(ipc, CY_IPC_NO_NOTIFICATION);
		return;
	}

	Cy_IPC_Drv_ClearInterrupt(intr, CY_IPC_NO_NOTIFICATION, IFX_MBOX_CH_MASK(ipc_ch));

	if (Cy_IPC_Drv_ReadMsgWord(ipc, &data->received_data[channel]) != CY_IPC_DRV_SUCCESS) {
		Cy_IPC_Drv_LockRelease(ipc, CY_IPC_NO_NOTIFICATION);
		return;
	}

	if (data->cb[channel] != NULL) {
		msg.data = (const void *)&data->received_data[channel];
		msg.size = MAILBOX_MBOX_SIZE;
		data->cb[channel](dev, channel, data->user_data[channel], &msg);
	}

	Cy_IPC_Drv_LockRelease(ipc, CY_IPC_NO_NOTIFICATION);
}

static int ifx_cat1_mailbox_send(const struct device *dev, uint32_t channel,
				 const struct mbox_msg *msg)
{
	const struct ifx_cat1_mailbox_config *cfg = dev->config;
	IPC_STRUCT_Type *ipc;
	cy_en_ipcdrv_status_t ipc_status;
	uint32_t notify_mask;

	if (channel >= MAILBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	if (msg == NULL) {
		LOG_ERR("Signalling mode not supported");
		return -EINVAL;
	}

	if (msg->size != MAILBOX_MBOX_SIZE) {
		return -EMSGSIZE;
	}

	ipc = Cy_IPC_Drv_GetIpcBaseAddress(cfg->ipc_channel[channel]);
	notify_mask = IFX_MBOX_INTR_MASK(cfg->ipc_intr_struct[channel]);

	ipc_status = Cy_IPC_Drv_SendMsgWord(ipc, notify_mask,
					    *(const uint32_t *)msg->data);
	if (ipc_status != CY_IPC_DRV_SUCCESS) {
		LOG_ERR("Send failed (0x%x)", ipc_status);
		return -EIO;
	}

	return 0;
}

static int ifx_cat1_mailbox_register_callback(const struct device *dev, uint32_t channel,
					      mbox_callback_t cb, void *user_data)
{
	struct ifx_cat1_mailbox_data *data = dev->data;

	if (channel >= MAILBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	data->cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
}

static int ifx_cat1_mailbox_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return MAILBOX_MBOX_SIZE;
}

static uint32_t ifx_cat1_mailbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return MAILBOX_MAX_CHANNELS;
}

static int ifx_cat1_mailbox_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	const struct ifx_cat1_mailbox_config *cfg = dev->config;
	struct ifx_cat1_mailbox_data *data = dev->data;
	uint32_t ipc_ch;
	uint32_t ipc_intr;
	IPC_INTR_STRUCT_Type *intr;
	uint32_t notify_mask;
	uint32_t release_mask;

	if (channel >= MAILBOX_MAX_CHANNELS) {
		LOG_ERR("Channel exceeds max number of channels");
		return -EINVAL;
	}

	ipc_ch = cfg->ipc_channel[channel];
	ipc_intr = cfg->ipc_intr_struct[channel];
	intr = Cy_IPC_Drv_GetIntrBaseAddr(ipc_intr);

	/* Preserve existing masks on this intr struct so multiple mbox channels can
	 * share one intr struct if the DT is configured that way.
	 */
	release_mask = Cy_IPC_Drv_ExtractReleaseMask(Cy_IPC_Drv_GetInterruptMask(intr));
	notify_mask = Cy_IPC_Drv_ExtractAcquireMask(Cy_IPC_Drv_GetInterruptMask(intr));

	if (enable) {
		notify_mask |= IFX_MBOX_CH_MASK(ipc_ch);
		Cy_IPC_Drv_SetInterruptMask(intr, release_mask, notify_mask);
		data->channel_enable[channel] = true;
		irq_enable(cfg->channel_irq[channel]);
	} else {
		irq_disable(cfg->channel_irq[channel]);
		data->channel_enable[channel] = false;
		notify_mask &= ~IFX_MBOX_CH_MASK(ipc_ch);
		Cy_IPC_Drv_SetInterruptMask(intr, release_mask, notify_mask);
	}

	return 0;
}

static DEVICE_API(mbox, ifx_cat1_mailbox_driver_api) = {
	.send = ifx_cat1_mailbox_send,
	.register_callback = ifx_cat1_mailbox_register_callback,
	.mtu_get = ifx_cat1_mailbox_mtu_get,
	.max_channels_get = ifx_cat1_mailbox_max_channels_get,
	.set_enabled = ifx_cat1_mailbox_set_enabled,
};

#define MBOX_CH_ISR_DEFINE(n, _)							\
	static void ifx_cat1_mbox_ch##n##_isr(const struct device *dev)			\
	{										\
		ifx_cat1_mbox_handle_channel(dev, n);					\
	}

LISTIFY(MAILBOX_MAX_CHANNELS, MBOX_CH_ISR_DEFINE, (;));

#define MBOX_CH_IRQ_CONNECT(n, idx)							\
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(idx, n),					\
		    DT_INST_IRQ_BY_IDX(idx, n, priority),				\
		    ifx_cat1_mbox_ch##n##_isr,						\
		    DEVICE_DT_INST_GET(idx), 0)

#define MBOX_CH_IRQN(n, idx)         DT_INST_IRQN_BY_IDX(idx, n)
#define MBOX_IPC_CHANNEL(n, idx)     DT_INST_PROP_BY_IDX(idx, ipc_channels, n)
#define MBOX_IPC_INTR_STRUCT(n, idx) DT_INST_PROP_BY_IDX(idx, ipc_intr_structs, n)

#define INFINEON_MAILBOX_INSTANCE_DEFINE(idx)						\
	static struct ifx_cat1_mailbox_data ifx_cat1_mbox_data_##idx;			\
											\
	static void ifx_cat1_mbox_irq_config_##idx(void)				\
	{										\
		LISTIFY(MAILBOX_MAX_CHANNELS, MBOX_CH_IRQ_CONNECT, (;), idx);		\
	}										\
											\
	static const struct ifx_cat1_mailbox_config ifx_cat1_mbox_cfg_##idx = {		\
		.irq_config = ifx_cat1_mbox_irq_config_##idx,				\
		.channel_irq = {							\
			LISTIFY(MAILBOX_MAX_CHANNELS, MBOX_CH_IRQN, (,), idx)		\
		},									\
		.ipc_channel = {							\
			LISTIFY(MAILBOX_MAX_CHANNELS, MBOX_IPC_CHANNEL, (,), idx)	\
		},									\
		.ipc_intr_struct = {							\
			LISTIFY(MAILBOX_MAX_CHANNELS, MBOX_IPC_INTR_STRUCT, (,), idx)	\
		},									\
	};										\
											\
	static int ifx_cat1_mbox_init_##idx(const struct device *dev)			\
	{										\
		const struct ifx_cat1_mailbox_config *cfg = dev->config;		\
		cfg->irq_config();							\
		return 0;								\
	}										\
											\
	DEVICE_DT_INST_DEFINE(idx, ifx_cat1_mbox_init_##idx, NULL,			\
			      &ifx_cat1_mbox_data_##idx,				\
			      &ifx_cat1_mbox_cfg_##idx, POST_KERNEL,			\
			      CONFIG_MBOX_INIT_PRIORITY,				\
			      &ifx_cat1_mailbox_driver_api)

DT_INST_FOREACH_STATUS_OKAY(INFINEON_MAILBOX_INSTANCE_DEFINE)
