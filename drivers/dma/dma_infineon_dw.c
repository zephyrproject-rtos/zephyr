/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_dw_dma

#include <zephyr/cache.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(dma_ifx_dw, CONFIG_DMA_LOG_LEVEL);

#define DW_CTL          0x0000U
#define DW_CTL_ENABLED  BIT(31)
#define DW_CH(ch)       (0x8000U + (ch) * 0x40U)
#define DW_CH_CTL       0x00U
#define DW_CH_STATUS    0x04U
#define DW_CH_IDX       0x08U
#define DW_CH_CURR_PTR  0x0CU
#define DW_CH_INTR      0x10U
#define DW_CH_INTR_MASK 0x18U
#define DW_CH_TR_CMD    0x28U

#define DW_CH_CTL_PRIO(prio)    ((uint32_t)(prio) << 8)
#define DW_CH_CTL_PRIO_MAX      3U
#define DW_CH_CTL_ENABLED       BIT(31)
#define DW_CH_STATUS_INTR_CAUSE GENMASK(3, 0)
#define DW_CH_STATUS_PENDING    BIT(31)
#define DW_CH_IDX_X             GENMASK(7, 0)
#define DW_CH_IDX_Y             GENMASK(15, 8)
#define DW_CH_INTR_CH           BIT(0)
#define DW_CH_TR_CMD_ACTIVATE   BIT(0)
#define DW_CAUSE_COMPLETION     1U

#define DW_DESCR_INTR_TYPE   GENMASK(3, 2)
#define DW_DESCR_TR_OUT_TYPE GENMASK(5, 4)
#define DW_DESCR_TR_IN_TYPE  GENMASK(7, 6)
#define DW_DESCR_CH_DISABLE  BIT(24)
#define DW_DESCR_SRC_WORD    BIT(26)
#define DW_DESCR_DST_WORD    BIT(27)
#define DW_DESCR_DATA_SIZE   GENMASK(29, 28)
#define DW_DESCR_TYPE        GENMASK(31, 30)
#define DW_DESCR_TYPE_1D     1U
#define DW_DESCR_TYPE_2D     2U
#define DW_DESCR_SRC_INCR    GENMASK(11, 0)
#define DW_DESCR_DST_INCR    GENMASK(23, 12)
#define DW_DESCR_COUNT       GENMASK(31, 24)
#define DW_TRIG_DESCR        2U
#define DW_TRIG_DESCR_CHAIN  3U

#define DW_DESCR_LOOP(src_incr, dst_incr, count)                                                   \
	(FIELD_PREP(DW_DESCR_SRC_INCR, src_incr) | FIELD_PREP(DW_DESCR_DST_INCR, dst_incr) |       \
	 FIELD_PREP(DW_DESCR_COUNT, count))

struct ifx_dw_dma_descr {
	uint32_t ctl;
	uint32_t src;
	uint32_t dst;
	uint32_t x_ctl;
	uint32_t y_ctl;
	uint32_t next;
};

struct ifx_dw_dma_channel {
	struct ifx_dw_dma_descr descr[CONFIG_DMA_INFINEON_DW_DESCRIPTOR_COUNT];
	dma_callback_t callback;
	void *user_data;
	uint8_t dir;
	bool error_callback_dis;
	bool sw_triggered;
	uint8_t prio;
};

struct ifx_dw_dma_config {
	mm_reg_t base;
	void (*irq_configure)(void);
	uint16_t irq;
	uint8_t num_channels;
	bool enable_chaining;
};

static uint32_t ifx_dw_dma_incr(uint16_t addr_adj)
{
	if (addr_adj == DMA_ADDR_ADJ_INCREMENT) {
		return 1U;
	}

	if (addr_adj == DMA_ADDR_ADJ_DECREMENT) {
		return UINT32_MAX;
	}

	return 0U;
}

static const struct ifx_dw_dma_descr *ifx_dw_dma_next(const struct ifx_dw_dma_descr *descr)
{
	uint32_t next = (FIELD_GET(DW_DESCR_TYPE, descr->ctl) == DW_DESCR_TYPE_2D) ? descr->next
										   : descr->y_ctl;

	if (next == (uint32_t)(uintptr_t)descr) {
		return NULL;
	}

	return (const struct ifx_dw_dma_descr *)(uintptr_t)next;
}

static uint32_t ifx_dw_dma_count(uint32_t loop_ctl)
{
	return FIELD_GET(DW_DESCR_COUNT, loop_ctl) + 1U;
}

static int ifx_dw_dma_config(const struct device *dev, uint32_t channel, struct dma_config *cfg)
{
	const struct ifx_dw_dma_config *config = dev->config;
	struct ifx_dw_dma_channel *ch = (struct ifx_dw_dma_channel *)dev->data + channel;
	struct dma_block_config *block = cfg->head_block;
	struct ifx_dw_dma_descr *descr = ch->descr;
	uint32_t size = cfg->dest_data_size;
	uint32_t burst = cfg->dest_burst_length;
	uint32_t count = cfg->block_count;
	uint32_t prio = cfg->channel_priority;
	uint32_t *next;
	uint32_t ctl;
	uint32_t elements;
	uint32_t src_incr;
	uint32_t dst_incr;
	mm_reg_t ch_base;

	if (channel >= config->num_channels) {
		return -EINVAL;
	}

	if ((size != cfg->source_data_size) || ((size != 1U) && (size != 2U) && (size != 4U)) ||
	    (burst > 256U) || ((burst <= 1U) && (block->block_size > 256U)) ||
	    ((burst > 0U) && (block->block_size > (burst * 256U))) || (prio > DW_CH_CTL_PRIO_MAX)) {
		LOG_ERR("Unsupported configuration");
		return -EINVAL;
	}

	if (count > CONFIG_DMA_INFINEON_DW_DESCRIPTOR_COUNT) {
		LOG_ERR("Not enough descriptors");
		return -ENOMEM;
	}

	ch->callback = cfg->dma_callback;
	ch->user_data = cfg->user_data;
	ch->dir = (uint8_t)cfg->channel_direction;
	ch->error_callback_dis = cfg->error_callback_dis != 0U;
	ch->sw_triggered =
		(cfg->channel_direction == MEMORY_TO_MEMORY) || (cfg->source_handshake != 0U);
	ch->prio = (uint8_t)prio;

	ctl = FIELD_PREP(DW_DESCR_INTR_TYPE,
			 (cfg->complete_callback_en != 0U) ? DW_TRIG_DESCR : DW_TRIG_DESCR_CHAIN) |
	      FIELD_PREP(DW_DESCR_TR_OUT_TYPE, DW_TRIG_DESCR_CHAIN) |
	      FIELD_PREP(DW_DESCR_DATA_SIZE, size >> 1) |
	      FIELD_PREP(DW_DESCR_TYPE, (burst != 0U) ? DW_DESCR_TYPE_2D : DW_DESCR_TYPE_1D);

	if ((cfg->source_burst_length != 1U) || (cfg->channel_direction == MEMORY_TO_MEMORY)) {
		ctl |= FIELD_PREP(DW_DESCR_TR_IN_TYPE, DW_TRIG_DESCR_CHAIN);
	}

	if (count <= 1U) {
		ctl |= DW_DESCR_CH_DISABLE;
	}

	if (cfg->channel_direction == PERIPHERAL_TO_MEMORY) {
		ctl |= DW_DESCR_SRC_WORD;
	} else if (cfg->channel_direction == MEMORY_TO_PERIPHERAL) {
		ctl |= DW_DESCR_DST_WORD;
	}

	for (uint32_t i = 1U; i <= count; i++) {
		src_incr = ifx_dw_dma_incr(block->source_addr_adj);
		dst_incr = ifx_dw_dma_incr(block->dest_addr_adj);
		elements = block->block_size >> (size >> 1);

		descr->ctl = ctl;
		descr->src = block->source_address;
		descr->dst = block->dest_address;

		if (burst != 0U) {
			descr->x_ctl = DW_DESCR_LOOP(src_incr, dst_incr, burst - 1U);
			descr->y_ctl = DW_DESCR_LOOP(src_incr * burst, dst_incr * burst,
						     DIV_ROUND_UP(elements, burst) - 1U);
			next = &descr->next;
		} else {
			if (block->dest_scatter_interval != 0U) {
				dst_incr *= block->dest_scatter_interval;
			}
			descr->x_ctl = DW_DESCR_LOOP(src_incr, dst_incr, elements - 1U);
			next = &descr->y_ctl;
		}

		if (i < count) {
			*next = (uint32_t)(uintptr_t)(descr + 1);
		} else {
			*next = config->enable_chaining ? (uint32_t)(uintptr_t)descr : 0U;
		}

		block = block->next_block;
		descr++;
	}

	ch_base = config->base + DW_CH(channel);
	sys_write32((uint32_t)(uintptr_t)ch->descr, ch_base + DW_CH_CURR_PTR);
	sys_write32(0U, ch_base + DW_CH_IDX);
	sys_write32(DW_CH_CTL_PRIO(prio), ch_base + DW_CH_CTL);
	sys_write32(DW_CH_INTR_CH, ch_base + DW_CH_INTR_MASK);

	irq_enable(irq_to_level_2(irq_from_level_2(config->irq) + channel) |
		   irq_parent_level_2(config->irq));

	return 0;
}

static int ifx_dw_dma_start(const struct device *dev, uint32_t channel)
{
	const struct ifx_dw_dma_config *config = dev->config;
	const struct ifx_dw_dma_channel *ch = (struct ifx_dw_dma_channel *)dev->data + channel;
	mm_reg_t ch_base = config->base + DW_CH(channel);

	if (channel >= config->num_channels) {
		return -EINVAL;
	}

	(void)sys_cache_data_flush_and_invd_all();

	sys_write32(DW_CH_CTL_PRIO(ch->prio) | DW_CH_CTL_ENABLED, ch_base + DW_CH_CTL);

	if (ch->sw_triggered) {
		sys_write32(DW_CH_TR_CMD_ACTIVATE, ch_base + DW_CH_TR_CMD);
	}

	return 0;
}

static int ifx_dw_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct ifx_dw_dma_config *config = dev->config;
	const struct ifx_dw_dma_channel *ch = (struct ifx_dw_dma_channel *)dev->data + channel;

	if (channel >= config->num_channels) {
		return -EINVAL;
	}

	sys_write32(DW_CH_CTL_PRIO(ch->prio), config->base + DW_CH(channel) + DW_CH_CTL);

	return 0;
}

static int ifx_dw_dma_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			     size_t size)
{
	const struct ifx_dw_dma_config *config = dev->config;
	struct ifx_dw_dma_channel *ch = (struct ifx_dw_dma_channel *)dev->data + channel;

	if (channel >= config->num_channels) {
		return -EINVAL;
	}

	ch->descr[0].src = src;
	ch->descr[0].dst = dst;

	(void)sys_cache_data_flush_and_invd_range((void *)(uintptr_t)src, size);

	sys_write32(DW_CH_CTL_PRIO(ch->prio) | DW_CH_CTL_ENABLED,
		    config->base + DW_CH(channel) + DW_CH_CTL);

	return 0;
}

static int ifx_dw_dma_get_status(const struct device *dev, uint32_t channel,
				 struct dma_status *stat)
{
	const struct ifx_dw_dma_config *config = dev->config;
	const struct ifx_dw_dma_channel *ch = (struct ifx_dw_dma_channel *)dev->data + channel;
	const struct ifx_dw_dma_descr *descr;
	const struct ifx_dw_dma_descr *curr;
	mm_reg_t ch_base = config->base + DW_CH(channel);
	uint32_t total = 0U;
	uint32_t done = 0U;
	uint32_t idx;

	if ((channel >= config->num_channels) || (stat == NULL)) {
		return -EINVAL;
	}

	stat->busy = (sys_read32(ch_base + DW_CH_STATUS) & DW_CH_STATUS_PENDING) != 0U;
	stat->pending_length = 0U;

	if ((ch->descr[0].src != 0U) || (ch->descr[0].dst != 0U)) {
		for (descr = ch->descr; descr != NULL; descr = ifx_dw_dma_next(descr)) {
			total += ifx_dw_dma_count(descr->x_ctl) *
				 ((FIELD_GET(DW_DESCR_TYPE, descr->ctl) == DW_DESCR_TYPE_2D)
					  ? ifx_dw_dma_count(descr->y_ctl)
					  : 1U);
		}

		curr = (const struct ifx_dw_dma_descr *)(uintptr_t)sys_read32(ch_base +
									      DW_CH_CURR_PTR);
		if (curr != NULL) {
			for (descr = ch->descr; (descr != NULL) && (descr != curr);
			     descr = ifx_dw_dma_next(descr)) {
				done += ifx_dw_dma_count(descr->x_ctl) *
					ifx_dw_dma_count(descr->y_ctl);
			}

			idx = sys_read32(ch_base + DW_CH_IDX);
			done += FIELD_GET(DW_CH_IDX_X, idx) +
				FIELD_GET(DW_CH_IDX_Y, idx) * ifx_dw_dma_count(curr->x_ctl);
		}

		stat->pending_length = total - done;
	}

	stat->dir = (enum dma_channel_direction)ch->dir;

	return 0;
}

static void ifx_dw_dma_isr(const struct device *dev, uint32_t channel)
{
	const struct ifx_dw_dma_config *config = dev->config;
	const struct ifx_dw_dma_channel *ch = (struct ifx_dw_dma_channel *)dev->data + channel;
	mm_reg_t ch_base = config->base + DW_CH(channel);
	uint32_t cause = FIELD_GET(DW_CH_STATUS_INTR_CAUSE, sys_read32(ch_base + DW_CH_STATUS));
	int status = 0;

	if (cause != DW_CAUSE_COMPLETION) {
		LOG_ERR("Channel %u error cause %u", channel, cause);
		status = -EIO;
	}

	sys_write32(DW_CH_INTR_CH, ch_base + DW_CH_INTR);
	(void)sys_read32(ch_base + DW_CH_INTR);

	if ((ch->callback == NULL) || ((status != 0) && ch->error_callback_dis)) {
		return;
	}

	ch->callback(dev, ch->user_data, channel, status);
}

static int ifx_dw_dma_init(const struct device *dev)
{
	const struct ifx_dw_dma_config *config = dev->config;
	mm_reg_t base = config->base;

	sys_write32(sys_read32(base + DW_CTL) | DW_CTL_ENABLED, base + DW_CTL);

	config->irq_configure();

	return 0;
}

static DEVICE_API(dma, ifx_dw_dma_api) = {
	.config = ifx_dw_dma_config,
	.start = ifx_dw_dma_start,
	.stop = ifx_dw_dma_stop,
	.reload = ifx_dw_dma_reload,
	.get_status = ifx_dw_dma_get_status,
};

#define IFX_DW_DMA_IRQ_CHECK(i, n)                                                                 \
	BUILD_ASSERT(DT_INST_IRQN_BY_IDX(n, i) ==                                                  \
			     (IRQ_TO_L2(DT_INST_IRQ_BY_IDX(n, 0, irq) + i) |                       \
			      (DT_INST_IRQN(n) & BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS))),      \
		     "channel interrupts must be consecutive on one interrupt line")

#define IFX_DW_DMA_IRQ_CONNECT(i, n)                                                               \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, i), DT_INST_IRQ_BY_IDX(n, i, priority),                 \
		    ifx_dw_dma_isr_##n, (const void *)(uintptr_t)i, 0)

#define IFX_DW_DMA_INIT(n)                                                                         \
	BUILD_ASSERT(DT_INST_PROP(n, dma_channels) == DT_NUM_IRQS(DT_DRV_INST(n)),                 \
		     "dma-channels must match the number of interrupts");                          \
	LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), IFX_DW_DMA_IRQ_CHECK, (;), n);                        \
                                                                                                   \
	static void ifx_dw_dma_isr_##n(const void *arg)                                            \
	{                                                                                          \
		ifx_dw_dma_isr(DEVICE_DT_INST_GET(n), (uint32_t)(uintptr_t)arg);                   \
	}                                                                                          \
                                                                                                   \
	static void ifx_dw_dma_irq_configure_##n(void)                                             \
	{                                                                                          \
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), IFX_DW_DMA_IRQ_CONNECT, (;), n);              \
	}                                                                                          \
                                                                                                   \
	static struct ifx_dw_dma_channel ifx_dw_dma_channels_##n[DT_INST_PROP(n, dma_channels)];   \
                                                                                                   \
	static const struct ifx_dw_dma_config ifx_dw_dma_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_configure = ifx_dw_dma_irq_configure_##n,                                     \
		.irq = DT_INST_IRQN(n),                                                            \
		.num_channels = DT_INST_PROP(n, dma_channels),                                     \
		.enable_chaining = DT_INST_PROP(n, enable_chaining),                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_dw_dma_init, NULL, ifx_dw_dma_channels_##n,                   \
			      &ifx_dw_dma_config_##n, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,      \
			      &ifx_dw_dma_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_DW_DMA_INIT)
