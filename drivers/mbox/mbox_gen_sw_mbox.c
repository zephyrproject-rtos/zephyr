/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generic Software Mailbox Driver for Zephyr
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/device.h>

#define LOG_LEVEL CONFIG_MBOX_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_generic_software_mbox);

#define DT_DRV_COMPAT nxp_generic_software_mbox

#define MAX_CHANNELS	 CONFIG_MBOX_GEN_SW_CHAN

#define MBOX_SIZE	sizeof(uint32_t)

/* Hardware layout of the shared memory region */
struct gen_sw_mbox_mmio {
	volatile uint32_t rx_status[MAX_CHANNELS];
	volatile uint32_t tx_status[MAX_CHANNELS];
	volatile uint32_t txdb_status[MAX_CHANNELS];
	volatile uint32_t rx_ch[MAX_CHANNELS];
	volatile uint32_t tx_ch[MAX_CHANNELS];
	uint32_t reserved_2[MAX_CHANNELS];
	volatile uint32_t ch_ack_flags;
};

/* Channel status values */
enum gen_sw_mbox_chan_status {
	S_READY = 0,
	S_BUSY  = 1,
	S_DONE  = 2,
};

/* Driver data structure */
struct gen_sw_mbox_data {
	mbox_callback_t cb[MAX_CHANNELS];
	void *user_data[MAX_CHANNELS];
	uint32_t received_data;
};

/* Driver config structure */
struct gen_sw_mbox_config {
	struct gen_sw_mbox_mmio *mmio;
	int remote_irq;
};

static int gen_sw_mbox_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	uint32_t data32;
	const struct gen_sw_mbox_config *cfg = dev->config;
	struct gen_sw_mbox_mmio *mmio = cfg->mmio;

	if (channel >= MAX_CHANNELS) {
		return -EINVAL;
	}

	if (msg == NULL) {
		/* Signalling mode - just send an interrupt with no data */
		/* Wait until TX doorbell channel is ready */
		while (mmio->txdb_status[channel] != S_READY) {
			/* Could implement timeout here */
			k_yield();
		}

		/* Mark channel as busy */
		mmio->txdb_status[channel] = S_BUSY;
	} else {
		/* Data transfer mode */
		if (msg->size > MBOX_SIZE) {
			/* We can only send upto 4 bytes at a time */
			return -EMSGSIZE;
		}

		/* Wait until TX channel is ready */
		while (mmio->tx_status[channel] != S_READY) {
			/* Could implement timeout here */
			k_yield();
		}

		/* Copy data to TX channel */
		memcpy(&data32, msg->data, msg->size);
		mmio->tx_ch[channel] = data32;
		/* Mark channel as busy */
		mmio->tx_status[channel] = S_BUSY;
	}

	/* Trigger remote interrupt */
	arm_gic_irq_set_pending(cfg->remote_irq);

	return 0;
}

static int gen_sw_mbox_register_callback(const struct device *dev, uint32_t channel,
					 mbox_callback_t cb, void *user_data)
{
	struct gen_sw_mbox_data *data = dev->data;

	if (channel >= MAX_CHANNELS) {
		return -EINVAL;
	}

	data->cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
}

static int gen_sw_mbox_mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MBOX_SIZE;
}

static uint32_t gen_sw_mbox_max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return  MAX_CHANNELS;
}

static int gen_sw_mbox_set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	struct gen_sw_mbox_data *data = dev->data;

	if (channel >=  MAX_CHANNELS) {
		return -EINVAL;
	}

	if (enable) {
		if (data->cb[channel] == NULL) {
			LOG_WRN("Enabling channel without a registered callback");
		}
	}

	return 0;
}

static const struct mbox_driver_api gen_sw_mbox_driver_api = {
	.send = gen_sw_mbox_send,
	.register_callback = gen_sw_mbox_register_callback,
	.mtu_get = gen_sw_mbox_mtu_get,
	.max_channels_get = gen_sw_mbox_max_channels_get,
	.set_enabled = gen_sw_mbox_set_enabled,
};

static void gen_sw_mbox_isr(const struct device *dev)
{
	struct gen_sw_mbox_data *data = dev->data;
	const struct gen_sw_mbox_config *cfg = dev->config;
	struct gen_sw_mbox_mmio *mmio = cfg->mmio;

	/* Check all channels */
	for (int i = 0; i <  MAX_CHANNELS; i++) {
		/* Check if data was received */
		if (mmio->rx_status[i] != S_BUSY) {
			continue;
		}

		/* Read data from the channel */
		data->received_data = mmio->rx_ch[i];

		/* Create message struct */
		struct mbox_msg msg = {
			.data = &data->received_data,
			.size = MBOX_SIZE
		};

		/* Call the callback */
		if (data->cb[i]) {
			data->cb[i](dev, i, data->user_data[i], &msg);
		}

		if (mmio->ch_ack_flags & BIT(i)) {
			/* Mark as received */
			mmio->rx_status[i] = S_DONE;
			/* Trigger remote interrupt */
			arm_gic_irq_set_pending(cfg->remote_irq);
		} else {
			/* Reset status to ready */
			mmio->rx_status[i] = S_READY;
		}
	}
}

/* Define instances based on device tree */
#define GEN_SW_MBOX_INIT(inst)							\
	static struct gen_sw_mbox_data gen_sw_mbox_data_##inst;			\
										\
	static const struct gen_sw_mbox_config gen_sw_mbox_config_##inst = {	\
		.mmio = (struct gen_sw_mbox_mmio *)DT_INST_REG_ADDR(inst),	\
		.remote_irq = DT_INST_PROP(inst, remote_interrupt),		\
	};									\
										\
	static int gen_sw_mbox_init_##inst(const struct device *dev)		\
	{									\
		const struct gen_sw_mbox_config *cfg = dev->config;		\
		struct gen_sw_mbox_mmio *mmio = cfg->mmio;			\
		mm_reg_t mmio_va;						\
										\
		device_map((mm_reg_t *)&mmio_va, (uintptr_t)cfg->mmio,		\
			DT_INST_REG_SIZE(inst),					\
			K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);			\
		/* Initialize all channels' status */				\
		for (int i = 0; i < MAX_CHANNELS; i++) {			\
			mmio->rx_status[i] = S_READY;				\
			mmio->tx_status[i] = S_READY;				\
		}								\
										\
		/* Connect and enable the IRQ */				\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),	\
			    gen_sw_mbox_isr, DEVICE_DT_INST_GET(inst), 0);	\
		irq_enable(DT_INST_IRQN(inst));					\
										\
		return 0;							\
	}									\
										\
	DEVICE_DT_INST_DEFINE(inst,						\
			     gen_sw_mbox_init_##inst,				\
			     NULL,						\
			     &gen_sw_mbox_data_##inst,				\
			     &gen_sw_mbox_config_##inst,			\
			     PRE_KERNEL_1,					\
			     CONFIG_MBOX_INIT_PRIORITY,				\
			     &gen_sw_mbox_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GEN_SW_MBOX_INIT)
