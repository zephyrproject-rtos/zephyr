/*
 * Copyright (c) 2025 Core Devices LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_dmac

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/toolchain.h>

#include <register.h>

LOG_MODULE_REGISTER(dma_sf32lb, CONFIG_DMA_LOG_LEVEL);

#define DMAC_MAX_LEN DMAC_CNDTR1_NDT
#define DMAC_MAX_PL  3U

#define DMAC_ISR       offsetof(DMAC_TypeDef, ISR)
#define DMAC_IFCR      offsetof(DMAC_TypeDef, IFCR)
#define DMAC_CCR1      offsetof(DMAC_TypeDef, CCR1)
#define DMAC_CCR2      offsetof(DMAC_TypeDef, CCR2)
#define DMAC_CCRX(n)   (DMAC_CCR1 + (DMAC_CCR2 - DMAC_CCR1) * (n))
#define DMAC_CNDTR1    offsetof(DMAC_TypeDef, CNDTR1)
#define DMAC_CNDTR2    offsetof(DMAC_TypeDef, CNDTR2)
#define DMAC_CNDTRX(n) (DMAC_CNDTR1 + (DMAC_CNDTR2 - DMAC_CNDTR1) * (n))
#define DMAC_CPAR1     offsetof(DMAC_TypeDef, CPAR1)
#define DMAC_CPAR2     offsetof(DMAC_TypeDef, CPAR2)
#define DMAC_CPARX(n)  (DMAC_CPAR1 + (DMAC_CPAR2 - DMAC_CPAR1) * (n))
#define DMAC_CM0AR1    offsetof(DMAC_TypeDef, CM0AR1)
#define DMAC_CM0AR2    offsetof(DMAC_TypeDef, CM0AR2)
#define DMAC_CM0ARX(n) (DMAC_CM0AR1 + (DMAC_CM0AR2 - DMAC_CM0AR1) * (n))
#define DMAC_CBSR1     offsetof(DMAC_TypeDef, CBSR1)
#define DMAC_CBSR2     offsetof(DMAC_TypeDef, CBSR2)
#define DMAC_CBSRX(n)  (DMAC_CBSR1 + (DMAC_CBSR2 - DMAC_CBSR1) * (n))
#define DMAC_CSELR1    offsetof(DMAC_TypeDef, CSELR1)
#define DMAC_CSELR2    offsetof(DMAC_TypeDef, CSELR2)

#define DMAC_ISR_TCIF(n) (DMAC_ISR_TCIF1_Msk << (n * 4U))

#define DMAC_IFCR_ALL(n)                                                                           \
	((DMAC_IFCR_CGIF1_Msk | DMAC_IFCR_CTCIF1_Msk | DMAC_IFCR_CHTIF1_Msk |                      \
	  DMAC_IFCR_CTEIF1_Msk)                                                                    \
	 << (n * 4U))
#define DMAC_IFCR_CTCIF(n) (DMAC_IFCR_CTCIF1_Msk << (n * 4U))
#define DMAC_IFCR_CTEIF(n) (DMAC_IFCR_CTEIF1_Msk << (n * 4U))

#define DMAC_CCRX_PSIZE(n) FIELD_PREP(DMAC_CCR1_PSIZE_Msk, LOG2CEIL(n))
#define DMAC_CCRX_MSIZE(n) FIELD_PREP(DMAC_CCR1_MSIZE_Msk, LOG2CEIL(n))

struct dma_sf32lb_irq_ctx {
	const struct device *dev;
	uint8_t channel;
};

struct dma_sf32lb_config {
	uintptr_t dmac;
	uint8_t n_channels;
	uint8_t n_requests;
	struct sf32lb_clock_dt_spec clock;
	void (*irq_configure)(void);
	struct dma_sf32lb_channel *channels;
};

struct dma_sf32lb_channel {
	dma_callback_t callback;
	void *user_data;
	enum dma_channel_direction direction;
};

struct dma_sf32lb_data {
	struct dma_context ctx;
	struct k_spinlock lock;
};

static void dma_sf32lb_isr(const struct device *dev, uint8_t channel)
{
	const struct dma_sf32lb_config *config = dev->config;
	uint32_t isr;
	int status;

	isr = sys_read32(config->dmac + DMAC_ISR);
	if ((isr & DMAC_ISR_TCIF(channel)) != 0U) {
		status = DMA_STATUS_COMPLETE;
	} else {
		status = -EIO;
	}

	config->channels[channel].callback(dev, config->channels[channel].user_data, channel,
					   status);

	sys_write32(DMAC_IFCR_ALL(channel), config->dmac + DMAC_IFCR);
}

#define DMA_SF32LB_IRQ_DEFINE(n, _)                                                                \
	static void dma_sf32lb_isr_ch##n(const struct device *dev)                                 \
	{                                                                                          \
		dma_sf32lb_isr(dev, n);                                                            \
	}

LISTIFY(8, DMA_SF32LB_IRQ_DEFINE, ())

static int dma_sf32lb_config(const struct device *dev, uint32_t channel,
			     struct dma_config *config_dma)
{
	const struct dma_sf32lb_config *config = dev->config;
	struct dma_sf32lb_data *data = dev->data;
	uint32_t ccrx;
	uint32_t cselrx;
	uint32_t cparx;
	uint32_t cm0arx;

	if (channel >= config->n_channels) {
		LOG_ERR("Invalid channel (%" PRIu32 ", max %" PRIu32 ")", channel,
			config->n_channels);
		return -EINVAL;
	}

	if (config_dma->block_count != 1U) {
		LOG_ERR("Chained block transfer not supported (%" PRIu32 ", max 1)",
			config_dma->block_count);
		return -ENOTSUP;
	}

	if (config_dma->head_block->block_size > DMAC_MAX_LEN) {
		LOG_ERR("Block size exceeds maximum (%" PRIu32 ", max %lu)",
			config_dma->head_block->block_size, DMAC_MAX_LEN);
		return -EINVAL;
	}

	if (config_dma->dma_slot >= config->n_requests) {
		LOG_ERR("Invalid DMA slot (%" PRIu32 ", max %" PRIu32 ")", config_dma->dma_slot,
			config->n_requests);
		return -EINVAL;
	}

	if (config_dma->channel_priority > DMAC_MAX_PL) {
		LOG_ERR("Invalid channel priority (%" PRIu32 ", max %" PRIu32 ")",
			config_dma->channel_priority, DMAC_MAX_PL);
		return -EINVAL;
	}

	if ((config_dma->head_block->source_addr_adj == DMA_ADDR_ADJ_DECREMENT) |
	    (config_dma->head_block->dest_addr_adj == DMA_ADDR_ADJ_DECREMENT)) {
		LOG_ERR("Address decrement not supported");
		return -ENOTSUP;
	}

	if ((config_dma->source_data_size != 1U) && (config_dma->source_data_size != 2U) &&
	    (config_dma->source_data_size != 4U)) {
		LOG_ERR("Invalid source data size (%" PRIu32 ", must be 1, 2, or 4)",
			config_dma->source_data_size);
		return -EINVAL;
	}

	if ((config_dma->dest_data_size != 1U) && (config_dma->dest_data_size != 2U) &&
	    (config_dma->dest_data_size != 4U)) {
		LOG_ERR("Invalid destination data size (%" PRIu32 ", must be 1, 2, or 4)",
			config_dma->dest_data_size);
		return -EINVAL;
	}

	/* configure transfer parameters */
	ccrx = sys_read32(config->dmac + DMAC_CCRX(channel));
	if ((ccrx & DMAC_CCR1_EN) != 0U) {
		LOG_ERR("Configuration not possible with DMA enabled");
		return -EIO;
	}

	ccrx &= ~(DMAC_CCR1_TCIE | DMAC_CCR1_HTIE | DMAC_CCR1_TEIE | DMAC_CCR1_DIR_Msk |
		  DMAC_CCR1_CIRC_Msk | DMAC_CCR1_PINC_Msk | DMAC_CCR1_MINC_Msk |
		  DMAC_CCR1_PSIZE_Msk | DMAC_CCR1_MSIZE_Msk | DMAC_CCR1_PL_Msk |
		  DMAC_CCR1_MEM2MEM_Msk);

	ccrx |= FIELD_PREP(DMAC_CCR1_PL_Msk, config_dma->channel_priority);

	switch (config_dma->channel_direction) {
	case MEMORY_TO_MEMORY:
		ccrx |= DMAC_CCR1_MEM2MEM;
		__fallthrough;
	case PERIPHERAL_TO_MEMORY:
		ccrx |= DMAC_CCRX_PSIZE(config_dma->source_data_size) |
			DMAC_CCRX_MSIZE(config_dma->dest_data_size);

		if (config_dma->head_block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			ccrx |= DMAC_CCR1_PINC;
		}
		if (config_dma->head_block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			ccrx |= DMAC_CCR1_MINC;
		}

		cparx = config_dma->head_block->source_address;
		cm0arx = config_dma->head_block->dest_address;
		break;
	case MEMORY_TO_PERIPHERAL:
		ccrx |= DMAC_CCR1_DIR | DMAC_CCRX_PSIZE(config_dma->dest_data_size) |
			DMAC_CCRX_MSIZE(config_dma->source_data_size);

		if (config_dma->head_block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			ccrx |= DMAC_CCR1_MINC;
		}
		if (config_dma->head_block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			ccrx |= DMAC_CCR1_PINC;
		}

		cparx = config_dma->head_block->dest_address;
		cm0arx = config_dma->head_block->source_address;
		break;
	default:
		return -ENOTSUP;
	}

	sys_write32(ccrx, config->dmac + DMAC_CCRX(channel));

	/* single transfer */
	sys_write32(FIELD_PREP(DMAC_CBSR1_BS_Msk, 0U), config->dmac + DMAC_CBSRX(channel));

	/* configure transfer size, src/dst addresses */
	sys_write32(config_dma->head_block->block_size, config->dmac + DMAC_CNDTRX(channel));
	sys_write32(cparx, config->dmac + DMAC_CPARX(channel));
	sys_write32(cm0arx, config->dmac + DMAC_CM0ARX(channel));

	/* configure request */
	K_SPINLOCK(&data->lock) {
		if (channel < 4U) {
			cselrx = sys_read32(config->dmac + DMAC_CSELR1);
			cselrx &= ~(DMAC_CSELR1_C1S_Msk << (channel * 8U));
			cselrx |= FIELD_PREP(DMAC_CSELR1_C1S_Msk << (channel * 8U),
					     config_dma->dma_slot);
			sys_write32(cselrx, config->dmac + DMAC_CSELR1);
		} else {
			cselrx = sys_read32(config->dmac + DMAC_CSELR2);
			cselrx &= ~(DMAC_CSELR1_C1S_Msk << ((channel - 4U) * 8U));
			cselrx |= FIELD_PREP(DMAC_CSELR1_C1S_Msk << ((channel - 4U) * 8U),
					     config_dma->dma_slot);
			sys_write32(cselrx, config->dmac + DMAC_CSELR2);
		}
	}

	config->channels[channel].callback = config_dma->dma_callback;
	config->channels[channel].user_data = config_dma->user_data;
	config->channels[channel].direction = config_dma->channel_direction;

	return 0;
}

static int dma_sf32lb_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			     size_t size)
{
	const struct dma_sf32lb_config *config = dev->config;
	uint32_t ccrx;
	uint32_t cparx;
	uint32_t cm0arx;

	if (channel >= config->n_channels) {
		LOG_ERR("Invalid channel (%" PRIu32 ", max %" PRIu32 ")", channel,
			config->n_channels);
		return -EINVAL;
	}

	if (size > DMAC_MAX_LEN) {
		LOG_ERR("Block size exceeds maximum (%" PRIu32 ", max %lu)", size, DMAC_MAX_LEN);
		return -EINVAL;
	}

	ccrx = sys_read32(config->dmac + DMAC_CCRX(channel));
	if ((ccrx & DMAC_CCR1_EN) != 0U) {
		LOG_ERR("Channel %" PRIu32 " is busy", channel);
		return -EBUSY;
	}

	/* configure size, src/dst addresses */
	sys_write32(size, config->dmac + DMAC_CNDTRX(channel));

	switch (config->channels[channel].direction) {
	case MEMORY_TO_MEMORY:
	case PERIPHERAL_TO_MEMORY:
		cparx = src;
		cm0arx = dst;
		break;
	case MEMORY_TO_PERIPHERAL:
		cparx = dst;
		cm0arx = src;
		break;
	default:
		__ASSERT_NO_MSG(false);
		return -ENOTSUP;
	}

	sys_write32(cparx, config->dmac + DMAC_CPARX(channel));
	sys_write32(cm0arx, config->dmac + DMAC_CM0ARX(channel));

	return 0;
}

static int dma_sf32lb_start(const struct device *dev, uint32_t channel)
{
	const struct dma_sf32lb_config *config = dev->config;
	uint32_t ccrx;

	if (channel >= config->n_channels) {
		LOG_ERR("Invalid channel (%" PRIu32 ", max %" PRIu32 ")", channel,
			config->n_channels);
		return -EINVAL;
	}

	ccrx = sys_read32(config->dmac + DMAC_CCRX(channel));
	if ((ccrx & DMAC_CCR1_EN) != 0U) {
		return 0;
	}

	/* clear all transfer flags */
	sys_write32(DMAC_IFCR_ALL(channel), config->dmac + DMAC_IFCR);

	/* enable DMA, complete/error IRQs if callback configured */
	ccrx |= DMAC_CCR1_EN;
	if (config->channels[channel].callback != NULL) {
		ccrx |= DMAC_CCR1_TCIE | DMAC_CCR1_TEIE;
	}
	sys_write32(ccrx, config->dmac + DMAC_CCRX(channel));

	return 0;
}

static int dma_sf32lb_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_sf32lb_config *config = dev->config;
	uint32_t ccrx;

	if (channel >= config->n_channels) {
		LOG_ERR("Invalid channel (%" PRIu32 ", max %" PRIu32 ")", channel,
			config->n_channels);
		return -EINVAL;
	}

	/* disable DMA and complete/error IRQs */
	ccrx = sys_read32(config->dmac + DMAC_CCRX(channel));
	ccrx &= ~(DMAC_CCR1_EN | DMAC_CCR1_TCIE | DMAC_CCR1_TEIE);
	sys_write32(ccrx, config->dmac + DMAC_CCRX(channel));

	return 0;
}

static int dma_sf32lb_get_status(const struct device *dev, uint32_t channel,
				 struct dma_status *stat)
{
	const struct dma_sf32lb_config *config = dev->config;
	uint32_t isr;

	if (channel >= config->n_channels) {
		LOG_ERR("Invalid channel (%" PRIu32 ", max %" PRIu32 ")", channel,
			config->n_channels);
		return -EINVAL;
	}

	isr = sys_read32(config->dmac + DMAC_ISR);
	if ((isr & DMAC_IFCR_CTEIF(channel)) != 0U) {
		return -EIO;
	}

	stat->busy = (isr & DMAC_IFCR_CTCIF(channel)) == 0U;
	stat->dir = config->channels[channel].direction;
	stat->pending_length = sys_read32(config->dmac + DMAC_CNDTRX(channel));

	return 0;
}

static DEVICE_API(dma, dma_sf32lb_driver_api) = {
	.config = dma_sf32lb_config,
	.reload = dma_sf32lb_reload,
	.start = dma_sf32lb_start,
	.stop = dma_sf32lb_stop,
	.get_status = dma_sf32lb_get_status,
};

static int dma_sf32lb_init(const struct device *dev)
{
	const struct dma_sf32lb_config *config = dev->config;

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	(void)sf32lb_clock_control_on_dt(&config->clock);

	for (uint8_t channel = 0U; channel < config->n_channels; channel++) {
		uint32_t ccrx;

		ccrx = sys_read32(config->dmac + DMAC_CCRX(channel));
		ccrx &= ~(DMAC_CCR1_EN | DMAC_CCR1_TCIE | DMAC_CCR1_HTIE | DMAC_CCR1_TEIE);
		sys_write32(ccrx, config->dmac + DMAC_CCRX(channel));
	}

	config->irq_configure();

	return 0;
}

#define DMA_SF32LB_IRQ_CONFIGURE(n, inst)                                                          \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    dma_sf32lb_isr_ch##n, DEVICE_DT_INST_GET(inst), 0);                            \
	irq_enable(DT_INST_IRQ_BY_IDX(inst, n, irq));

#define DMA_SF32LB_CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, DMA_SF32LB_IRQ_CONFIGURE, (), inst)

#define DMA_SF32LB_DEFINE(inst)                                                                    \
	static void irq_configure##inst(void)                                                      \
	{                                                                                          \
		DMA_SF32LB_CONFIGURE_ALL_IRQS(inst, DT_INST_NUM_IRQS(inst));                       \
	}                                                                                          \
                                                                                                   \
	static struct dma_sf32lb_channel channels##inst[DT_INST_PROP(inst, dma_channels)];         \
                                                                                                   \
	static const struct dma_sf32lb_config config##inst = {                                     \
		.dmac = DT_INST_REG_ADDR(inst),                                                    \
		.n_channels = DT_INST_PROP(inst, dma_channels),                                    \
		.n_requests = DT_INST_PROP(inst, dma_requests),                                    \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(inst),                                      \
		.irq_configure = irq_configure##inst,                                              \
		.channels = channels##inst,                                                        \
	};                                                                                         \
                                                                                                   \
	ATOMIC_DEFINE(atomic##inst, DT_INST_PROP(inst, dma_channels));                             \
                                                                                                   \
	static struct dma_sf32lb_data data##inst = {                                               \
		.ctx =                                                                             \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = atomic##inst,                                            \
				.dma_channels = DT_INST_PROP(inst, dma_channels),                  \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, dma_sf32lb_init, NULL, &data##inst, &config##inst,             \
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY, &dma_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_SF32LB_DEFINE)
