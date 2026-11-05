/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rh850_intif_ored

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/interrupt_controller/intc_rh850_intif_ored.h>

#define INTIF_ORED_NUM_MASK           GENMASK(7, 0)
#define INTIF_ORED_BIT_POSITION_MASK  GENMASK(15, 8)

LOG_MODULE_REGISTER(rh850_intif_ored, CONFIG_INTC_LOG_LEVEL);

#define INTIF_ORED_MAX_BITS 10

struct intc_rh850_intif_ored_data {
	/* Array to store callback */
	intc_rh850_intif_ored_callback_t callback[INTIF_ORED_MAX_BITS];
	/* Array to store context */
	void *context[INTIF_ORED_MAX_BITS];
};

struct intc_rh850_intif_ored_config {
	/* INTIF ORed channel */
	const uint8_t channel;
};

/* Interrupt handler */
static void intc_rh850_intif_ored_isr(const struct device *dev)
{
	const struct intc_rh850_intif_ored_config *config = dev->config;
	struct intc_rh850_intif_ored_data *data = dev->data;

	IRQn_Type irq_ored = R_FSP_ORed_CurrentIrqGet();
	uint8_t ored_bit_pos = BSP_ORED_IRQ_TO_CHANNEL(irq_ored);

	if (ored_bit_pos >= INTIF_ORED_MAX_BITS) {
		LOG_ERR("Invalid ORed bit position: %d", ored_bit_pos);
		return;
	}

	if (data->callback[ored_bit_pos]) {
		/* Call the registered callback, if there is one */
		data->callback[ored_bit_pos](data->context[ored_bit_pos]);
	} else {
		uint8_t intif_ored_channel = config->channel;

		/* If there is no callback registered for this event, mask the interrupt bit
		 * inside ORed INTIF
		 */
		R_BSP_ORed_IrqCfgDisable(irq_ored);
		LOG_ERR("No callback registered for INTSEL ORed channel %d, bit position %d.",
			intif_ored_channel, ored_bit_pos);
	}
}

int intc_rh850_intif_ored_set_enabled(const struct device *dev, bsp_ored_t source,
							uint16_t priority, bool enabled)
{
	const struct intc_rh850_intif_ored_config *config = dev->config;
	uint8_t intif_ored_channel = FIELD_GET(INTIF_ORED_NUM_MASK, source);

	if (config->channel != intif_ored_channel) {
		LOG_ERR("Invalid ORed Interrupt selection source. Expected channel %d but got %d.",
			config->channel, intif_ored_channel);
		return -EINVAL;
	}

	if (enabled) {
		R_BSP_ORed_IrqCfgEnable(source, priority, (void *)dev);
	} else {
		R_BSP_ORed_IrqCfgDisable(source);
	}
	return 0;
}

int intc_rh850_intif_ored_set_callback(const struct device *dev, bsp_ored_t source,
				   intc_rh850_intif_ored_callback_t cb, void *arg)
{
	const struct intc_rh850_intif_ored_config *config = dev->config;
	struct intc_rh850_intif_ored_data *data = dev->data;

	uint8_t intsel_ored_bit_pos = FIELD_GET(INTIF_ORED_BIT_POSITION_MASK, source);
	uint8_t intif_ored_channel = FIELD_GET(INTIF_ORED_NUM_MASK, source);

	if (config->channel != intif_ored_channel) {
		LOG_ERR("Invalid INTSEL ORed source. Expected group %d but got %d.",
			config->channel, intif_ored_channel);
		return -EINVAL;
	}

	if (!cb) {
		LOG_ERR("Must provide callback");
		return -EINVAL;
	}

	if (intsel_ored_bit_pos >= INTIF_ORED_MAX_BITS) {
		LOG_ERR("Invalid bit position extracted from source");
		return -EINVAL;
	}

	data->callback[intsel_ored_bit_pos] = cb;
	data->context[intsel_ored_bit_pos] = arg;

	return 0;
}

static int intc_rh850_intif_ored_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* DRIVER REGISTRATION*/

#define INTIF_ORED_RH850_IRQ_CONNECT(idx, isr)                                       \
	do {                                                                             \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(idx, 0, irq), DT_INST_IRQ_BY_IDX(idx, 0, priority), \
			    isr, DEVICE_DT_INST_GET(idx), 0);                                    \
		irq_enable(DT_INST_IRQ_BY_IDX(idx, 0, irq));                                 \
	} while (0)

#define INTIF_ORED_RH850_CONFIG_FUNC(idx) INTIF_ORED_RH850_IRQ_CONNECT(idx,          \
								intc_rh850_intif_ored_isr); \

#define INTC_RH850_INTIF_ORED_INIT(idx)                                                     \
	static const struct intc_rh850_intif_ored_config intc_rh850_intif_ored_config##idx = {  \
		.channel = DT_INST_PROP(idx, channel),                                        \
	};                                                                                \
                                                                                      \
	static struct intc_rh850_intif_ored_data intc_rh850_intif_ored_data##idx = {};    \
                                                                                      \
	static int intc_rh850_intif_ored_init_##idx(const struct device *dev)             \
	{                                                                                 \
		INTIF_ORED_RH850_CONFIG_FUNC(idx)                                             \
		return intc_rh850_intif_ored_init(dev);                                       \
	}                                                                                 \
	DEVICE_DT_INST_DEFINE(idx, intc_rh850_intif_ored_init_##idx, NULL,                \
			    &intc_rh850_intif_ored_data##idx, &intc_rh850_intif_ored_config##idx, \
			    PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INTC_RH850_INTIF_ORED_INIT)
