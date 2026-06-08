/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief DMA driver for Infineon M-DMA.
 */

#define DT_DRV_COMPAT infineon_mdma

#include <infineon_kconfig.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <zephyr/sys/util.h>

#include <soc.h>

LOG_MODULE_REGISTER(ifx_mdma, CONFIG_DMA_LOG_LEVEL);

#define DESCRIPTOR_COUNT (CONFIG_DMA_INFINEON_MDMA_DESCRIPTOR_COUNT)

/* Descriptor type (SAXI_DMAC_CH.DESCR_CTL.DESCR_TYPE). */
enum ifx_mdma_descriptor_type {
	IFX_MDMA_1D_MEMORY_COPY = 0,
	IFX_MDMA_2D_MEMORY_COPY = 1,
	IFX_MDMA_3D_MEMORY_COPY = 2,
};

/* Interrupt, trigger-in and trigger-out event types. */
enum ifx_mdma_trigger_type {
	IFX_MDMA_M_LOOP = 0,
	IFX_MDMA_X_LOOP = 1,
	IFX_MDMA_DESCR = 2,
	IFX_MDMA_DESCR_CHAIN = 3,
};

/* Descriptor retrigger behaviour. */
enum ifx_mdma_retrigger {
	IFX_MDMA_RETRIG_IM = 0,
	IFX_MDMA_RETRIG_4CYC = 1,
	IFX_MDMA_RETRIG_16CYC = 2,
	IFX_MDMA_WAIT_FOR_REACT = 3,
};

/* Channel state on completion of a descriptor. */
enum ifx_mdma_channel_state {
	IFX_MDMA_CHANNEL_ENABLED = 0,
	IFX_MDMA_CHANNEL_DISABLED = 1,
};

/* All channel interrupt sources handled by the driver. */
#define IFX_MDMA_INTR_MASK                                                                         \
	(SAXI_DMAC_CH_INTR_COMPLETION_Msk | SAXI_DMAC_CH_INTR_SRC_BUS_ERROR_Msk |                  \
	 SAXI_DMAC_CH_INTR_DST_BUS_ERROR_Msk | SAXI_DMAC_CH_INTR_INVALID_DESCR_TYPE_Msk |          \
	 SAXI_DMAC_CH_INTR_CURR_PTR_NULL_Msk | SAXI_DMAC_CH_INTR_ACTIVE_CH_DISABLED_Msk |          \
	 SAXI_DMAC_CH_INTR_DESCR_BUS_ERROR_Msk)

/* AXIDMAC descriptor structure type. It is a user-declared structure allocated in RAM.
 * The AXIDMAC HW requires a pointer to this structure to work with it.
 */
struct ifx_mdma_descriptor {
	uint32_t ctl;      /* Descriptor control */
	uint32_t src;      /* Source base address */
	uint32_t dst;      /* Destination base address */
	uint32_t m_size;   /* M loop data count (stored as count - 1) */
	uint32_t x_size;   /* X loop data count (stored as count - 1) */
	uint32_t x_incr;   /* X loop source and destination increments */
	uint32_t y_size;   /* Y loop data count (stored as count - 1) */
	uint32_t y_incr;   /* Y loop source and destination increments */
	uint32_t next_ptr; /* Next descriptor pointer */
};

/* The M-DMA descriptors need to be 8-byte aligned */
struct mdma_descriptor_wrapper {
	struct ifx_mdma_descriptor descriptor;
} __aligned(8);

struct ifx_mdma_channel {
	struct mdma_descriptor_wrapper descr[DESCRIPTOR_COUNT];

	uint32_t channel_direction: 3;
	uint32_t complete_callback_en: 1;
	uint32_t error_callback_dis: 1;

	IRQn_Type irq;

	dma_callback_t callback;
	void *user_data;
};

struct ifx_mdma_data {
	struct ifx_mdma_channel *channels;
};

struct ifx_mdma_config {
	SAXI_DMAC_Type *regs;
	void (*irq_configure)(void);
	uint8_t num_channels;
};

/* Configuration values used to initialize a single descriptor. */
struct ifx_mdma_descriptor_config {
	enum ifx_mdma_retrigger retrigger;
	enum ifx_mdma_trigger_type interrupt_type;
	enum ifx_mdma_trigger_type trigger_out_type;
	enum ifx_mdma_channel_state channel_state;
	enum ifx_mdma_trigger_type trigger_in_type;
	bool data_prefetch;
	enum ifx_mdma_descriptor_type descriptor_type;
	uint32_t src_address;
	uint32_t dst_address;
	uint32_t m_count;
	int16_t src_x_increment;
	int16_t dst_x_increment;
	uint32_t x_count;
	int16_t src_y_increment;
	int16_t dst_y_increment;
	uint32_t y_count;
	struct ifx_mdma_descriptor *next_descriptor;
};

static enum ifx_mdma_descriptor_type get_descriptor_type(const struct ifx_mdma_descriptor *descr)
{
	return (enum ifx_mdma_descriptor_type)FIELD_GET(SAXI_DMAC_CH_DESCR_CTL_DESCR_TYPE_Msk,
							descr->ctl);
}

static uint32_t get_mloop_count(const struct ifx_mdma_descriptor *descr)
{
	return FIELD_GET(SAXI_DMAC_CH_DESCR_M_SIZE_M_COUNT_Msk, descr->m_size) + 1U;
}

static uint32_t get_xloop_count(const struct ifx_mdma_descriptor *descr)
{
	return FIELD_GET(SAXI_DMAC_CH_DESCR_X_SIZE_X_COUNT_Msk, descr->x_size) + 1U;
}

static uint32_t get_yloop_count(const struct ifx_mdma_descriptor *descr)
{
	return FIELD_GET(SAXI_DMAC_CH_DESCR_Y_SIZE_Y_COUNT_Msk, descr->y_size) + 1U;
}

static uint32_t get_src_address(const struct ifx_mdma_descriptor *descr)
{
	return descr->src;
}

static uint32_t get_dst_address(const struct ifx_mdma_descriptor *descr)
{
	return descr->dst;
}

/* Word index at which the next-descriptor pointer is packed for each type. */
static uint32_t descriptor_next_index(const struct ifx_mdma_descriptor *descr)
{
	switch (get_descriptor_type(descr)) {
	case IFX_MDMA_1D_MEMORY_COPY:
		return 4U;
	case IFX_MDMA_2D_MEMORY_COPY:
		return 6U;
	default:
		return 8U;
	}
}

static struct ifx_mdma_descriptor *get_next_descriptor(const struct ifx_mdma_descriptor *descr)
{
	const uint32_t *words = (const uint32_t *)descr;

	return (struct ifx_mdma_descriptor *)(uintptr_t)words[descriptor_next_index(descr)];
}

static void set_next_descriptor(struct ifx_mdma_descriptor *descr, struct ifx_mdma_descriptor *next)
{
	uint32_t *words = (uint32_t *)descr;

	words[descriptor_next_index(descr)] = (uint32_t)(uintptr_t)next;
}

static void set_src_address(struct ifx_mdma_descriptor *descr, uint32_t src)
{
	descr->src = src;
}

static void set_dst_address(struct ifx_mdma_descriptor *descr, uint32_t dst)
{
	descr->dst = dst;
}

static void set_mloop_count(struct ifx_mdma_descriptor *descr, uint32_t count)
{
	descr->m_size = FIELD_PREP(SAXI_DMAC_CH_DESCR_M_SIZE_M_COUNT_Msk, count - 1U);
}

static int descriptor_init(struct ifx_mdma_descriptor *descr,
			   const struct ifx_mdma_descriptor_config *config)
{
	if (descr == NULL || config == NULL) {
		return -EINVAL;
	}

	descr->ctl = FIELD_PREP(SAXI_DMAC_CH_DESCR_CTL_WAIT_FOR_DEACT_Msk, config->retrigger) |
		     FIELD_PREP(SAXI_DMAC_CH_DESCR_CTL_INTR_TYPE_Msk, config->interrupt_type) |
		     FIELD_PREP(SAXI_DMAC_CH_DESCR_CTL_TR_OUT_TYPE_Msk, config->trigger_out_type) |
		     FIELD_PREP(SAXI_DMAC_CH_DESCR_CTL_TR_IN_TYPE_Msk, config->trigger_in_type) |
		     FIELD_PREP(SAXI_DMAC_CH_DESCR_CTL_DATA_PREFETCH_Msk,
				config->data_prefetch ? 1U : 0U) |
		     FIELD_PREP(SAXI_DMAC_CH_DESCR_CTL_CH_DISABLE_Msk, config->channel_state) |
		     FIELD_PREP(SAXI_DMAC_CH_DESCR_CTL_DESCR_TYPE_Msk, config->descriptor_type);

	descr->src = config->src_address;
	set_dst_address(descr, config->dst_address);
	set_mloop_count(descr, config->m_count);

	if (config->descriptor_type != IFX_MDMA_1D_MEMORY_COPY) {
		descr->x_size = FIELD_PREP(SAXI_DMAC_CH_DESCR_X_SIZE_X_COUNT_Msk,
					   config->x_count - 1U);
	}

	/* Reads the descriptor type from ctl to place the pointer at the right offset. */
	set_next_descriptor(descr, config->next_descriptor);

	if (config->descriptor_type == IFX_MDMA_2D_MEMORY_COPY ||
	    config->descriptor_type == IFX_MDMA_3D_MEMORY_COPY) {
		descr->x_incr = FIELD_PREP(SAXI_DMAC_CH_DESCR_X_INCR_SRC_X_Msk,
					   (uint16_t)config->src_x_increment) |
				FIELD_PREP(SAXI_DMAC_CH_DESCR_X_INCR_DST_X_Msk,
					   (uint16_t)config->dst_x_increment);
	}

	if (config->descriptor_type == IFX_MDMA_3D_MEMORY_COPY) {
		descr->y_size = FIELD_PREP(SAXI_DMAC_CH_DESCR_Y_SIZE_Y_COUNT_Msk,
					   config->y_count - 1U);
		descr->y_incr = FIELD_PREP(SAXI_DMAC_CH_DESCR_Y_INCR_SRC_Y_Msk,
					   (uint16_t)config->src_y_increment) |
				FIELD_PREP(SAXI_DMAC_CH_DESCR_Y_INCR_DST_Y_Msk,
					   (uint16_t)config->dst_y_increment);
	}

	return 0;
}

/* Configuration values used to initialize a single channel. */
struct ifx_mdma_channel_config {
	struct ifx_mdma_descriptor *descriptor;
	uint32_t priority;
	bool enable;
	bool bufferable;
};

static void mdma_enable(SAXI_DMAC_Type *regs)
{
	regs->CTL |= SAXI_DMAC_CTL_ENABLED_Msk;
}

static uint32_t get_active_channels(SAXI_DMAC_Type *regs)
{
	return FIELD_GET(SAXI_DMAC_ACTIVE_ACTIVE_Msk, regs->ACTIVE);
}

static void channel_init(SAXI_DMAC_Type *regs, uint32_t channel,
			 const struct ifx_mdma_channel_config *config)
{
	regs->CH[channel].CURR = (uint32_t)(uintptr_t)config->descriptor;
	regs->CH[channel].CTL = FIELD_PREP(SAXI_DMAC_CH_CTL_PRIO_Msk, config->priority) |
				FIELD_PREP(SAXI_DMAC_CH_CTL_B_Msk, config->bufferable ? 1U : 0U) |
				FIELD_PREP(SAXI_DMAC_CH_CTL_ENABLED_Msk, config->enable ? 1U : 0U);
}

static void channel_set_descriptor(SAXI_DMAC_Type *regs, uint32_t channel,
				   struct ifx_mdma_descriptor *descr)
{
	regs->CH[channel].CURR = (uint32_t)(uintptr_t)descr;
}

static void channel_enable(SAXI_DMAC_Type *regs, uint32_t channel)
{
	regs->CH[channel].CTL |= SAXI_DMAC_CH_CTL_ENABLED_Msk;
}

static void channel_disable(SAXI_DMAC_Type *regs, uint32_t channel)
{
	regs->CH[channel].CTL &= (uint32_t)~SAXI_DMAC_CH_CTL_ENABLED_Msk;
}

static void channel_set_sw_trigger(SAXI_DMAC_Type *regs, uint32_t channel)
{
	regs->CH[channel].TR_CMD = SAXI_DMAC_CH_TR_CMD_ACTIVATE_Msk;
}

static struct ifx_mdma_descriptor *channel_get_current_descriptor(SAXI_DMAC_Type *regs,
								  uint32_t channel)
{
	return (struct ifx_mdma_descriptor *)(uintptr_t)regs->CH[channel].CURR;
}

static uint32_t channel_get_current_xloop_index(SAXI_DMAC_Type *regs, uint32_t channel)
{
	return FIELD_GET(SAXI_DMAC_CH_IDX_X_Msk, regs->CH[channel].IDX);
}

static uint32_t channel_get_current_yloop_index(SAXI_DMAC_Type *regs, uint32_t channel)
{
	return FIELD_GET(SAXI_DMAC_CH_IDX_Y_Msk, regs->CH[channel].IDX);
}

static uint32_t channel_get_current_mloop_index(SAXI_DMAC_Type *regs, uint32_t channel)
{
	return FIELD_GET(SAXI_DMAC_CH_M_IDX_M_Msk, regs->CH[channel].M_IDX);
}

static uint32_t channel_get_intr_status_masked(SAXI_DMAC_Type *regs, uint32_t channel)
{
	return regs->CH[channel].INTR_MASKED;
}

static void channel_clear_interrupt(SAXI_DMAC_Type *regs, uint32_t channel, uint32_t interrupt)
{
	regs->CH[channel].INTR = interrupt;
	/* Dummy read guarantees the interrupt is cleared before returning. */
	(void)regs->CH[channel].INTR;
}

static void channel_set_interrupt_mask(SAXI_DMAC_Type *regs, uint32_t channel, uint32_t interrupt)
{
	regs->CH[channel].INTR_MASK = interrupt;
}

static int convert_xy_increment(uint32_t addr_adj)
{
	switch (addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		return 1;
	case DMA_ADDR_ADJ_DECREMENT:
		return -1;
	case DMA_ADDR_ADJ_NO_CHANGE:
		return 0;
	default:
		return 0;
	}
}

#if defined(CONFIG_CPU_HAS_DCACHE) && defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
static int flush_descriptor(struct ifx_mdma_descriptor *descr)
{
	struct ifx_mdma_descriptor *curr_descr = descr;
	struct ifx_mdma_descriptor *first_descr = curr_descr;

	while (curr_descr != NULL) {
		/* flush the cache at the descriptor address */
		sys_cache_data_flush_range((void *)curr_descr, sizeof(*curr_descr));

		curr_descr = get_next_descriptor(curr_descr);

		/* If cyclic, stop when we loop back to the first descriptor. */
		if (curr_descr == first_descr) {
			break;
		}
	}

	return 0;
}
#endif

static int ifx_mdma_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	int ret;
	struct ifx_mdma_data *data = dev->data;
	const struct ifx_mdma_config *const cfg = dev->config;
	struct ifx_mdma_channel_config channel_config = {0};
	struct ifx_mdma_descriptor_config desc_config = {0};
	struct ifx_mdma_descriptor *descriptor;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (config->head_block == NULL) {
		return -EINVAL;
	}

	if (config->half_complete_callback_en) {
		return -ENOTSUP;
	}

	/* M-DMA has no per-descriptor data-size field; require matching widths. */
	if (config->dest_data_size != config->source_data_size) {
		LOG_ERR("Source and dest data size differ.");
		return -EINVAL;
	}

	if (config->block_count > DESCRIPTOR_COUNT) {
		LOG_ERR("block_count %u exceeds DESCRIPTOR_COUNT %u", config->block_count,
			DESCRIPTOR_COUNT);
		return -EINVAL;
	}

	if (config->channel_direction != MEMORY_TO_MEMORY) {
		LOG_ERR("Only MEMORY_TO_MEMORY direction supported");
		return -EINVAL;
	}

	data->channels[channel].callback = config->dma_callback;
	data->channels[channel].user_data = config->user_data;
	data->channels[channel].channel_direction = config->channel_direction;
	data->channels[channel].complete_callback_en = config->complete_callback_en;
	data->channels[channel].error_callback_dis = config->error_callback_dis;

	descriptor = &data->channels[channel].descr[0].descriptor;

	desc_config.retrigger = IFX_MDMA_WAIT_FOR_REACT;
	desc_config.data_prefetch = false;

	/* Per-block interrupt if complete_callback_en, otherwise only at chain end. */
	desc_config.interrupt_type =
		config->complete_callback_en ? IFX_MDMA_DESCR : IFX_MDMA_DESCR_CHAIN;

	desc_config.trigger_out_type = IFX_MDMA_DESCR_CHAIN;
	desc_config.trigger_in_type = IFX_MDMA_DESCR_CHAIN;

	struct dma_block_config *block_config = config->head_block;

	for (uint32_t i = 0U; i < config->block_count; i++) {
		uint32_t total_elements = block_config->block_size;

		if (block_config->source_addr_adj == DMA_ADDR_ADJ_DECREMENT ||
		    block_config->dest_addr_adj == DMA_ADDR_ADJ_DECREMENT) {
			return -ENOTSUP;
		}
		int src_incr = convert_xy_increment(block_config->source_addr_adj);
		int dst_incr = convert_xy_increment(block_config->dest_addr_adj);

		if (config->dest_burst_length != 0) {
			uint32_t burst = config->dest_burst_length;

			if (burst >= (1 << 24)) {
				return -EINVAL;
			}
			uint32_t outer_loop_count = DIV_ROUND_UP(total_elements, burst);

			if (outer_loop_count >= (1 << 16)) {
				return -EINVAL;
			}

			desc_config.descriptor_type = IFX_MDMA_2D_MEMORY_COPY;
			desc_config.m_count = burst;
			desc_config.x_count = outer_loop_count;
			desc_config.src_x_increment = (int16_t)(src_incr * burst);
			desc_config.dst_x_increment = (int16_t)(dst_incr * burst);
		} else {
			if (total_elements >= (1 << 24)) {
				return -EINVAL;
			}
			desc_config.descriptor_type = IFX_MDMA_1D_MEMORY_COPY;
			desc_config.m_count = total_elements;

			desc_config.src_x_increment = (int16_t)src_incr;
			/* For 1D transfers, use dest_scatter_interval to specify the
			 * destination address increment value.
			 */
			if (block_config->dest_scatter_interval != 0) {
				desc_config.dst_x_increment =
					(int16_t)(dst_incr * block_config->dest_scatter_interval);
			} else {
				desc_config.dst_x_increment = (int16_t)dst_incr;
			}
		}

		desc_config.src_address = block_config->source_address;
		desc_config.dst_address = block_config->dest_address;

		if (i + 1U < config->block_count) {
			if (i + 1U >= DESCRIPTOR_COUNT) {
				LOG_ERR("Not enough descriptors available for channel %d", channel);
				return -ENOMEM;
			}
			desc_config.next_descriptor =
				&data->channels[channel].descr[i + 1U].descriptor;
			desc_config.channel_state = IFX_MDMA_CHANNEL_ENABLED;
		} else if (config->cyclic) {
			desc_config.next_descriptor = &data->channels[channel].descr[0].descriptor;
			desc_config.channel_state = IFX_MDMA_CHANNEL_ENABLED;
		} else {
			/* Last descriptor of a non-cyclic chain: disable the channel so a
			 * subsequent trigger does not follow the NULL next-descriptor
			 * pointer and raise CURR_PTR_NULL.
			 */
			desc_config.next_descriptor = NULL;
			desc_config.channel_state = IFX_MDMA_CHANNEL_DISABLED;
		}

		ret = descriptor_init(descriptor, &desc_config);
		if (ret != 0) {
			LOG_ERR("Descriptor init failed (%d)", ret);
			return ret;
		}

		block_config = block_config->next_block;
		descriptor = desc_config.next_descriptor;
	}

	channel_config.descriptor = &data->channels[channel].descr[0].descriptor;
	if (config->channel_priority > 3) {
		LOG_WRN("Hardware supports priority levels 0-3");
		channel_config.priority = 3;
	} else {
		channel_config.priority = config->channel_priority;
	}
	channel_config.enable = false;
	channel_config.bufferable = false;

	channel_init(cfg->regs, channel, &channel_config);

	channel_set_descriptor(cfg->regs, channel, &data->channels[channel].descr[0].descriptor);

	channel_set_interrupt_mask(cfg->regs, channel, IFX_MDMA_INTR_MASK);

	irq_enable(data->channels[channel].irq);

	return 0;
}

static int ifx_mdma_start(const struct device *dev, uint32_t channel)
{
	const struct ifx_mdma_config *const cfg = dev->config;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

#if defined(CONFIG_CPU_HAS_DCACHE) && defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
	struct ifx_mdma_data *data = dev->data;

	flush_descriptor(&data->channels[channel].descr[0].descriptor);
#endif

	channel_enable(cfg->regs, channel);
	channel_set_sw_trigger(cfg->regs, channel);

	return 0;
}

static int ifx_mdma_stop(const struct device *dev, uint32_t channel)
{
	const struct ifx_mdma_config *const cfg = dev->config;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	channel_disable(cfg->regs, channel);

	return 0;
}

static int ifx_mdma_reload(const struct device *dev, uint32_t channel, uint32_t src,
			   uint32_t dst, size_t size)
{
	struct ifx_mdma_data *data = dev->data;
	const struct ifx_mdma_config *const cfg = dev->config;
	struct ifx_mdma_descriptor *descriptor;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	descriptor = &data->channels[channel].descr[0].descriptor;

	set_src_address(descriptor, src);
	set_dst_address(descriptor, dst);

	/* The user supplies size in bytes; descriptor counts are in elements. The
	 * caller is responsible for matching the element width configured earlier.
	 */
	if (get_descriptor_type(descriptor) == IFX_MDMA_1D_MEMORY_COPY) {
		set_mloop_count(descriptor, size);
	} else {
		return -ENOTSUP;
	}

#if defined(CONFIG_CPU_HAS_DCACHE) && defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
	flush_descriptor(&data->channels[channel].descr[0].descriptor);
#endif

	channel_set_descriptor(cfg->regs, channel, &data->channels[channel].descr[0].descriptor);
	channel_enable(cfg->regs, channel);
	channel_set_sw_trigger(cfg->regs, channel);

	return 0;
}

static uint32_t get_descriptor_elements(struct ifx_mdma_descriptor *descr)
{
	uint32_t m = get_mloop_count(descr);
	enum ifx_mdma_descriptor_type type = get_descriptor_type(descr);
	uint32_t x = 1U;
	uint32_t y = 1U;

	if (type != IFX_MDMA_1D_MEMORY_COPY) {
		x = get_xloop_count(descr);
	}
	if (type == IFX_MDMA_3D_MEMORY_COPY) {
		y = get_yloop_count(descr);
	}

	return m * x * y;
}

static uint32_t get_total_size(const struct device *dev, uint32_t channel)
{
	struct ifx_mdma_data *data = dev->data;
	const struct ifx_mdma_config *const cfg = dev->config;
	struct ifx_mdma_descriptor *first_descr;
	struct ifx_mdma_descriptor *curr_descr;
	uint32_t total = 0;

	if (channel >= cfg->num_channels) {
		return 0;
	}

	curr_descr = &data->channels[channel].descr[0].descriptor;
	first_descr = curr_descr;

	while (curr_descr != NULL) {
		total += get_descriptor_elements(curr_descr);
		curr_descr = get_next_descriptor(curr_descr);

		/* If cyclic, stop when we loop back to the first descriptor. */
		if (curr_descr == first_descr) {
			break;
		}
	}

	return total;
}

static uint32_t get_transferred_size(const struct device *dev, uint32_t channel)
{
	const struct ifx_mdma_config *const cfg = dev->config;
	struct ifx_mdma_data *data = dev->data;
	struct ifx_mdma_descriptor *next_descr = &data->channels[channel].descr[0].descriptor;
	struct ifx_mdma_descriptor *curr_descr = channel_get_current_descriptor(cfg->regs, channel);
	uint32_t transferred = 0;

	if (next_descr == NULL || curr_descr == NULL) {
		return 0;
	}

	while ((next_descr != NULL) && (next_descr != curr_descr)) {
		transferred += get_descriptor_elements(next_descr);
		next_descr = get_next_descriptor(next_descr);
	}

	/* Add progress inside the current descriptor using the loop index registers. */
	{
		enum ifx_mdma_descriptor_type type = get_descriptor_type(curr_descr);
		uint32_t m = get_mloop_count(curr_descr);
		uint32_t x_idx = channel_get_current_xloop_index(cfg->regs, channel);
		uint32_t y_idx = channel_get_current_yloop_index(cfg->regs, channel);
		uint32_t m_idx = channel_get_current_mloop_index(cfg->regs, channel);
		uint32_t x = 1U;

		if (type != IFX_MDMA_1D_MEMORY_COPY) {
			x = get_xloop_count(curr_descr);
		}

		transferred += m_idx + (x_idx * m) + (y_idx * m * x);
	}

	return transferred;
}

static int ifx_mdma_get_status(const struct device *dev, uint32_t channel, struct dma_status *stat)
{
	struct ifx_mdma_data *data = dev->data;
	const struct ifx_mdma_config *const cfg = dev->config;
	struct ifx_mdma_descriptor *head;

	if (channel >= cfg->num_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (stat == NULL) {
		return -EINVAL;
	}

	stat->busy = (get_active_channels(cfg->regs) & BIT(channel)) != 0;

	/* pending length is the remaining bytes in the current discriptor chain */
	head = &data->channels[channel].descr[0].descriptor;
	if (head != NULL && (get_src_address(head) != 0 || get_dst_address(head) != 0)) {
		uint32_t total = get_total_size(dev, channel);
		uint32_t done = get_transferred_size(dev, channel);

		stat->pending_length = (total > done) ? (total - done) : 0U;
	} else {
		stat->pending_length = 0;
	}

	stat->dir = data->channels[channel].channel_direction;

	return 0;
}

static int ifx_mdma_init(const struct device *dev)
{
	const struct ifx_mdma_config *const cfg = dev->config;

	mdma_enable(cfg->regs);

	cfg->irq_configure();

	return 0;
}

/* Handles DMA interrupts and dispatches to the individual channel */
struct ifx_mdma_irq_context {
	const struct device *dev;
	uint32_t channel;
};

static void mdma_isr(struct ifx_mdma_irq_context *irq_context)
{
	uint32_t channel = irq_context->channel;
	struct ifx_mdma_data *data = irq_context->dev->data;
	const struct ifx_mdma_config *cfg = irq_context->dev->config;
	dma_callback_t callback = data->channels[channel].callback;
	uint32_t intr_status = channel_get_intr_status_masked(cfg->regs, channel);
	int status = 0;

	if (intr_status & SAXI_DMAC_CH_INTR_SRC_BUS_ERROR_Msk) {
		LOG_ERR("MDMA ch%u: source bus error", channel);
		status = -EIO;
	} else if (intr_status & SAXI_DMAC_CH_INTR_DST_BUS_ERROR_Msk) {
		LOG_ERR("MDMA ch%u: destination bus error", channel);
		status = -EIO;
	} else if (intr_status & SAXI_DMAC_CH_INTR_DESCR_BUS_ERROR_Msk) {
		LOG_ERR("MDMA ch%u: descriptor bus error", channel);
		status = -EIO;
	} else if (intr_status & SAXI_DMAC_CH_INTR_INVALID_DESCR_TYPE_Msk) {
		LOG_ERR("MDMA ch%u: invalid descriptor type", channel);
		status = -EIO;
	} else if (intr_status & SAXI_DMAC_CH_INTR_CURR_PTR_NULL_Msk) {
		LOG_ERR("MDMA ch%u: current descriptor pointer NULL", channel);
		status = -EIO;
	} else if (intr_status & SAXI_DMAC_CH_INTR_ACTIVE_CH_DISABLED_Msk) {
		LOG_ERR("MDMA ch%u: active channel disabled", channel);
		status = -EIO;
	}

	channel_clear_interrupt(cfg->regs, channel, intr_status);

	if (callback == NULL) {
		return;
	}

	if (status != 0 && data->channels[channel].error_callback_dis) {
		return;
	}

	if (status == 0) {
		status = data->channels[channel].complete_callback_en ? DMA_STATUS_BLOCK
								      : DMA_STATUS_COMPLETE;
	}

	callback(irq_context->dev, data->channels[channel].user_data, channel, status);
}

static DEVICE_API(dma, ifx_mdma_api) = {
	.config = ifx_mdma_config,
	.start = ifx_mdma_start,
	.stop = ifx_mdma_stop,
	.reload = ifx_mdma_reload,
	.get_status = ifx_mdma_get_status,
};

#define IRQ_CONFIGURE(n, inst)                                                                     \
	static const struct ifx_mdma_irq_context irq_context##inst##n = {                          \
		.dev = DEVICE_DT_INST_GET(inst),                                                   \
		.channel = n,                                                                      \
	};                                                                                         \
                                                                                                   \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    mdma_isr, &irq_context##inst##n, 0);                                           \
                                                                                                   \
	ifx_mdma_channels##inst[n].irq = DT_INST_IRQ_BY_IDX(inst, n, irq);

#define CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, IRQ_CONFIGURE, (), inst)

#define INFINEON_MDMA_INIT(n)                                                                      \
                                                                                                   \
	BUILD_ASSERT(DT_INST_PROP(n, dma_channels) == DT_NUM_IRQS(DT_DRV_INST(n)),                 \
		     "dma-channels must match the number of interrupts");                          \
                                                                                                   \
	static void ifx_mdma_irq_configure##n(void);                                               \
                                                                                                   \
	static struct ifx_mdma_channel ifx_mdma_channels##n[DT_INST_PROP(n, dma_channels)];        \
                                                                                                   \
	static __aligned(32) struct ifx_mdma_data ifx_mdma_data##n = {                             \
		.channels = ifx_mdma_channels##n,                                                  \
	};                                                                                         \
                                                                                                   \
	static const struct ifx_mdma_config ifx_mdma_config##n = {                                 \
		.regs = (SAXI_DMAC_Type *)DT_INST_REG_ADDR(n),                                     \
		.irq_configure = ifx_mdma_irq_configure##n,                                        \
		.num_channels = DT_INST_PROP(n, dma_channels),                                     \
	};                                                                                         \
                                                                                                   \
	static void ifx_mdma_irq_configure##n(void)                                                \
	{                                                                                          \
		extern struct ifx_mdma_channel ifx_mdma_channels##n[];                             \
		CONFIGURE_ALL_IRQS(n, DT_NUM_IRQS(DT_DRV_INST(n)));                                \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ifx_mdma_init, NULL, &ifx_mdma_data##n, &ifx_mdma_config##n,     \
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY, &ifx_mdma_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_MDMA_INIT)
