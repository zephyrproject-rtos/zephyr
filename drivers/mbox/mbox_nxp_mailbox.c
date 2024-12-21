/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Wrapper of NXP Mailbox driver for Zephyr's MBOX model.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util_macro.h>
#include <fsl_mailbox.h>

#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_mbox_mailbox);

#define DT_DRV_COMPAT nxp_mbox_mailbox

#define MAILBOX_MAX_CHANNELS 4
#define MAILBOX_MBOX_SIZE    3

#if (defined(LPC55S69_cm33_core0_SERIES) || defined(LPC55S69_cm33_core1_SERIES))
#ifdef LPC55S69_cm33_core0_SERIES
#define MAILBOX_ID_THIS_CPU  kMAILBOX_CM33_Core0
#define MAILBOX_ID_OTHER_CPU kMAILBOX_CM33_Core1
#else
#define MAILBOX_ID_THIS_CPU  kMAILBOX_CM33_Core1
#define MAILBOX_ID_OTHER_CPU kMAILBOX_CM33_Core0
#endif
#else
#if defined(__CM4_CMSIS_VERSION)
#define MAILBOX_ID_THIS_CPU  kMAILBOX_CM4
#define MAILBOX_ID_OTHER_CPU kMAILBOX_CM0Plus
#else
#define MAILBOX_ID_THIS_CPU  kMAILBOX_CM0Plus
#define MAILBOX_ID_OTHER_CPU kMAILBOX_CM4
#endif
#endif

#define GENIRQ_SHIFT     (28U)
#define GEN0_IRQ_TRIGGER BIT(GENIRQ_SHIFT + 3U) /*!< General interrupt 3. */
#define GEN1_IRQ_TRIGGER BIT(GENIRQ_SHIFT + 2U) /*!< General interrupt 2. */
#define GEN2_IRQ_TRIGGER BIT(GENIRQ_SHIFT + 1U) /*!< General interrupt 1. */
#define GEN3_IRQ_TRIGGER BIT(GENIRQ_SHIFT + 0U) /*!< General interrupt 0. */

#define DATA_MASK         BIT_MASK(24U)
#define DATAIRQ_SHIFT     (24U)
#define DATA0_IRQ_TRIGGER BIT(DATAIRQ_SHIFT + 3U) /*!< Data interrupt 3. */
#define DATA1_IRQ_TRIGGER BIT(DATAIRQ_SHIFT + 2U) /*!< Data interrupt 2. */
#define DATA2_IRQ_TRIGGER BIT(DATAIRQ_SHIFT + 1U) /*!< Data interrupt 1. */
#define DATA3_IRQ_TRIGGER BIT(DATAIRQ_SHIFT + 0U) /*!< Data interrupt 0. */

struct nxp_mailbox_data {
	mbox_callback_t cb[MAILBOX_MAX_CHANNELS];
	void *user_data[MAILBOX_MAX_CHANNELS];
	bool channel_enable[MAILBOX_MAX_CHANNELS];
	uint32_t received_data;
};

struct nxp_mailbox_config {
	MAILBOX_Type *base;
};

static void mailbox_isr(const struct device *dev)
{
	struct nxp_mailbox_data *data = dev->data;
	const struct nxp_mailbox_config *config = dev->config;
	mailbox_cpu_id_t cpu_id;

	cpu_id = MAILBOX_ID_THIS_CPU;

	volatile uint32_t mailbox_value = MAILBOX_GetValue(config->base, cpu_id);
	uint32_t flags = mailbox_value & (~DATA_MASK);

	/* Clear or the interrupt gets called intermittently */
	MAILBOX_ClearValueBits(config->base, cpu_id, mailbox_value);

	for (int i_channel = 0; i_channel < MAILBOX_MAX_CHANNELS; i_channel++) {
		/* Continue to next channel if channel is not enabled */
		if (!data->channel_enable[i_channel]) {
			continue;
		}

		if ((flags & (DATA0_IRQ_TRIGGER >> i_channel))) {
			data->received_data = mailbox_value & DATA_MASK;
			struct mbox_msg msg = {(const void *)&data->received_data,
					       MAILBOX_MBOX_SIZE};

			if (data->cb[i_channel]) {
				data->cb[i_channel](dev, i_channel, data->user_data[i_channel],
						    &msg);
			}
		} else if ((flags & (GEN0_IRQ_TRIGGER >> i_channel))) {
			if (data->cb[i_channel]) {
				data->cb[i_channel](dev, i_channel, data->user_data[i_channel],
						    NULL);
			}
		}
	}

	/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F
	 * Store immediate overlapping exception return operation
	 * might vector to incorrect interrupt
	 */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
	barrier_dsync_fence_full();
#endif
}

static int nxp_mailbox_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	uint32_t __aligned(4) data32;
	const struct nxp_mailbox_config *cfg = dev->config;

	if (channel >= MAILBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	/* Signalling mode. */
	if (msg == NULL) {
		MAILBOX_SetValueBits(cfg->base, MAILBOX_ID_OTHER_CPU, GEN0_IRQ_TRIGGER >> channel);
		return 0;
	}

	/* Data transfer mode. */
	if (msg->size != MAILBOX_MBOX_SIZE) {
		/* We can only send this many bytes at a time. */
		return -EMSGSIZE;
	}

	/* memcpy to avoid issues when msg->data is not word-aligned. */
	memcpy(&data32, msg->data, msg->size);

	MAILBOX_SetValueBits(cfg->base, MAILBOX_ID_OTHER_CPU,
			     (DATA0_IRQ_TRIGGER >> channel) | (data32 & DATA_MASK));

	return 0;
}

static int nxp_mailbox_register_callback(const struct device *dev, uint32_t channel,
					 mbox_callback_t cb, void *user_data)
{
	struct nxp_mailbox_data *data = dev->data;

	if (channel >= MAILBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	data->cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
}

static int nxp_mailbox_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MAILBOX_MBOX_SIZE;
}

static uint32_t nxp_mailbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);
	return MAILBOX_MAX_CHANNELS;
}

static int nxp_mailbox_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	struct nxp_mailbox_data *data = dev->data;

	if (channel >= MAILBOX_MAX_CHANNELS) {
		return -EINVAL;
	}

	data->channel_enable[channel] = enable;

	return 0;
}

static DEVICE_API(mbox, nxp_mailbox_driver_api) = {
	.send = nxp_mailbox_send,
	.register_callback = nxp_mailbox_register_callback,
	.mtu_get = nxp_mailbox_mtu_get,
	.max_channels_get = nxp_mailbox_max_channels_get,
	.set_enabled = nxp_mailbox_set_enabled,
};

#define MAILBOX_INSTANCE_DEFINE(idx)                                                               \
	static struct nxp_mailbox_data nxp_mailbox_##idx##_data;                                   \
	const static struct nxp_mailbox_config nxp_mailbox_##idx##_config = {                      \
		.base = (MAILBOX_Type *)DT_INST_REG_ADDR(idx),                                     \
	};                                                                                         \
	static int nxp_mailbox_##idx##_init(const struct device *dev)                              \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		MAILBOX_Init(nxp_mailbox_##idx##_config.base);                                     \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), mailbox_isr,            \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQN(idx));                                                     \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(idx, nxp_mailbox_##idx##_init, NULL, &nxp_mailbox_##idx##_data,      \
			      &nxp_mailbox_##idx##_config, POST_KERNEL, CONFIG_MBOX_INIT_PRIORITY, \
			      &nxp_mailbox_driver_api)

#define MAILBOX_INST(idx) MAILBOX_INSTANCE_DEFINE(idx);

DT_INST_FOREACH_STATUS_OKAY(MAILBOX_INST)
