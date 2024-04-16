/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>

#include <gd32_dma.h>
#include <zephyr/irq.h>

#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
#define DT_DRV_COMPAT gd_gd32_dma_v1
#elif DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma)
#define DT_DRV_COMPAT gd_gd32_dma
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
#define CHXCTL_PERIEN_OFFSET	  ((uint32_t)25U)
#define GD32_DMA_CHXCTL_DIR	  BIT(6)
#define GD32_DMA_CHXCTL_M2M	  BIT(7)
#define GD32_DMA_INTERRUPT_ERRORS (DMA_CHXCTL_SDEIE | DMA_CHXCTL_TAEIE)
#define GD32_DMA_FLAG_ERRORS	  (DMA_FLAG_SDE | DMA_FLAG_TAE)
#else
#define GD32_DMA_CHXCTL_DIR	  BIT(4)
#define GD32_DMA_CHXCTL_M2M	  BIT(14)
#define GD32_DMA_INTERRUPT_ERRORS DMA_CHXCTL_ERRIE
#define GD32_DMA_FLAG_ERRORS	  DMA_FLAG_ERR
#endif

#ifdef CONFIG_SOC_SERIES_GD32F3X0
#undef DMA_INTF
#undef DMA_INTC
#undef DMA_CHCTL
#undef DMA_CHCNT
#undef DMA_CHPADDR
#undef DMA_CHMADDR

#define DMA_INTF(dma)	     REG32(dma + 0x00UL)
#define DMA_INTC(dma)	     REG32(dma + 0x04UL)
#define DMA_CHCTL(dma, ch)   REG32((dma + 0x08UL) + 0x14UL * (uint32_t)(ch))
#define DMA_CHCNT(dma, ch)   REG32((dma + 0x0CUL) + 0x14UL * (uint32_t)(ch))
#define DMA_CHPADDR(dma, ch) REG32((dma + 0x10UL) + 0x14UL * (uint32_t)(ch))
#define DMA_CHMADDR(dma, ch) REG32((dma + 0x14UL) + 0x14UL * (uint32_t)(ch))
#endif

#define GD32_DMA_INTF(dma)	  DMA_INTF(dma)
#define GD32_DMA_INTC(dma)	  DMA_INTC(dma)
#define GD32_DMA_CHCTL(dma, ch)	  DMA_CHCTL((dma), (ch))
#define GD32_DMA_CHCNT(dma, ch)	  DMA_CHCNT((dma), (ch))
#define GD32_DMA_CHPADDR(dma, ch) DMA_CHPADDR((dma), (ch))
#define GD32_DMA_CHMADDR(dma, ch) DMA_CHMADDR((dma), (ch))

LOG_MODULE_REGISTER(dma_gd32, CONFIG_DMA_LOG_LEVEL);

struct dma_gd32_config {
	uint32_t reg;
	uint32_t channels;
	uint16_t clkid;
	bool mem2mem;
#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
	struct reset_dt_spec reset;
#endif
	void (*irq_configure)(void);
};

struct dma_gd32_channel {
	dma_callback_t callback;
	void *user_data;
	uint32_t direction;
	bool busy;
};

struct dma_gd32_data {
	struct dma_context ctx;
	struct dma_gd32_channel *channels;
};

struct dma_gd32_srcdst_config {
	uint32_t addr;
	uint32_t adj;
	uint32_t width;
};

/*
 * Register access functions
 */

static inline void
gd32_dma_periph_increase_enable(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) |= DMA_CHXCTL_PNAGA;
}

static inline void
gd32_dma_periph_increase_disable(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) &= ~DMA_CHXCTL_PNAGA;
}

static inline void
gd32_dma_transfer_set_memory_to_memory(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) |= GD32_DMA_CHXCTL_M2M;
	GD32_DMA_CHCTL(reg, ch) &= ~GD32_DMA_CHXCTL_DIR;
}

static inline void
gd32_dma_transfer_set_memory_to_periph(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) &= ~GD32_DMA_CHXCTL_M2M;
	GD32_DMA_CHCTL(reg, ch) |= GD32_DMA_CHXCTL_DIR;
}

static inline void
gd32_dma_transfer_set_periph_to_memory(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) &= ~GD32_DMA_CHXCTL_M2M;
	GD32_DMA_CHCTL(reg, ch) &= ~GD32_DMA_CHXCTL_DIR;
}

static inline void
gd32_dma_memory_increase_enable(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) |= DMA_CHXCTL_MNAGA;
}

static inline void
gd32_dma_memory_increase_disable(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) &= ~DMA_CHXCTL_MNAGA;
}

static inline void
gd32_dma_circulation_enable(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) |= DMA_CHXCTL_CMEN;
}

static inline void
gd32_dma_circulation_disable(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) &= ~DMA_CHXCTL_CMEN;
}

static inline void gd32_dma_channel_enable(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) |= DMA_CHXCTL_CHEN;
}

static inline void gd32_dma_channel_disable(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) &= ~DMA_CHXCTL_CHEN;
}

static inline void
gd32_dma_interrupt_enable(uint32_t reg, dma_channel_enum ch, uint32_t source)
{
	GD32_DMA_CHCTL(reg, ch) |= source;
}

static inline void
gd32_dma_interrupt_disable(uint32_t reg, dma_channel_enum ch, uint32_t source)
{
	GD32_DMA_CHCTL(reg, ch) &= ~source;
}

static inline void
gd32_dma_priority_config(uint32_t reg, dma_channel_enum ch, uint32_t priority)
{
	uint32_t ctl = GD32_DMA_CHCTL(reg, ch);

	GD32_DMA_CHCTL(reg, ch) = (ctl & (~DMA_CHXCTL_PRIO)) | priority;
}

static inline void
gd32_dma_memory_width_config(uint32_t reg, dma_channel_enum ch, uint32_t mwidth)
{
	uint32_t ctl = GD32_DMA_CHCTL(reg, ch);

	GD32_DMA_CHCTL(reg, ch) = (ctl & (~DMA_CHXCTL_MWIDTH)) | mwidth;
}

static inline void
gd32_dma_periph_width_config(uint32_t reg, dma_channel_enum ch, uint32_t pwidth)
{
	uint32_t ctl = GD32_DMA_CHCTL(reg, ch);

	GD32_DMA_CHCTL(reg, ch) = (ctl & (~DMA_CHXCTL_PWIDTH)) | pwidth;
}

#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
static inline void
gd32_dma_channel_subperipheral_select(uint32_t reg, dma_channel_enum ch,
				      dma_subperipheral_enum sub_periph)
{
	uint32_t ctl = GD32_DMA_CHCTL(reg, ch);

	GD32_DMA_CHCTL(reg, ch) =
		(ctl & (~DMA_CHXCTL_PERIEN)) |
		((uint32_t)sub_periph << CHXCTL_PERIEN_OFFSET);
}
#endif

static inline void
gd32_dma_periph_address_config(uint32_t reg, dma_channel_enum ch, uint32_t addr)
{
	GD32_DMA_CHPADDR(reg, ch) = addr;
}

static inline void
gd32_dma_memory_address_config(uint32_t reg, dma_channel_enum ch, uint32_t addr)
{
#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
	DMA_CHM0ADDR(reg, ch) = addr;
#else
	GD32_DMA_CHMADDR(reg, ch) = addr;
#endif
}

static inline void
gd32_dma_transfer_number_config(uint32_t reg, dma_channel_enum ch, uint32_t num)
{
	GD32_DMA_CHCNT(reg, ch) = (num & DMA_CHXCNT_CNT);
}

static inline uint32_t
gd32_dma_transfer_number_get(uint32_t reg, dma_channel_enum ch)
{
	return GD32_DMA_CHCNT(reg, ch);
}

static inline void
gd32_dma_interrupt_flag_clear(uint32_t reg, dma_channel_enum ch, uint32_t flag)
{
#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
	if (ch < DMA_CH4) {
		DMA_INTC0(reg) |= DMA_FLAG_ADD(flag, ch);
	} else {
		DMA_INTC1(reg) |= DMA_FLAG_ADD(flag, ch - DMA_CH4);
	}
#else
	GD32_DMA_INTC(reg) |= DMA_FLAG_ADD(flag, ch);
#endif
}

static inline void
gd32_dma_flag_clear(uint32_t reg, dma_channel_enum ch, uint32_t flag)
{
#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
	if (ch < DMA_CH4) {
		DMA_INTC0(reg) |= DMA_FLAG_ADD(flag, ch);
	} else {
		DMA_INTC1(reg) |= DMA_FLAG_ADD(flag, ch - DMA_CH4);
	}
#else
	GD32_DMA_INTC(reg) |= DMA_FLAG_ADD(flag, ch);
#endif
}

static inline uint32_t
gd32_dma_interrupt_flag_get(uint32_t reg, dma_channel_enum ch, uint32_t flag)
{
#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
	if (ch < DMA_CH4) {
		return (DMA_INTF0(reg) & DMA_FLAG_ADD(flag, ch));
	} else {
		return (DMA_INTF1(reg) & DMA_FLAG_ADD(flag, ch - DMA_CH4));
	}
#else
	return (GD32_DMA_INTF(reg) & DMA_FLAG_ADD(flag, ch));
#endif
}

static inline void gd32_dma_deinit(uint32_t reg, dma_channel_enum ch)
{
	GD32_DMA_CHCTL(reg, ch) &= ~DMA_CHXCTL_CHEN;

	GD32_DMA_CHCTL(reg, ch) = DMA_CHCTL_RESET_VALUE;
	GD32_DMA_CHCNT(reg, ch) = DMA_CHCNT_RESET_VALUE;
	GD32_DMA_CHPADDR(reg, ch) = DMA_CHPADDR_RESET_VALUE;
#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
	DMA_CHM0ADDR(reg, ch) = DMA_CHMADDR_RESET_VALUE;
	DMA_CHFCTL(reg, ch) = DMA_CHFCTL_RESET_VALUE;
	if (ch < DMA_CH4) {
		DMA_INTC0(reg) |= DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, ch);
	} else {
		DMA_INTC1(reg) |=
			DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, ch - DMA_CH4);
	}
#else
	GD32_DMA_CHMADDR(reg, ch) = DMA_CHMADDR_RESET_VALUE;
	GD32_DMA_INTC(reg) |= DMA_FLAG_ADD(DMA_CHINTF_RESET_VALUE, ch);
#endif
}

/*
 * Utility functions
 */

static inline uint32_t dma_gd32_priority(uint32_t prio)
{
	return CHCTL_PRIO(prio);
}

static inline uint32_t dma_gd32_memory_width(uint32_t width)
{
	switch (width) {
	case 4:
		return CHCTL_MWIDTH(2);
	case 2:
		return CHCTL_MWIDTH(1);
	default:
		return CHCTL_MWIDTH(0);
	}
}

static inline uint32_t dma_gd32_periph_width(uint32_t width)
{
	switch (width) {
	case 4:
		return CHCTL_PWIDTH(2);
	case 2:
		return CHCTL_PWIDTH(1);
	default:
		return CHCTL_PWIDTH(0);
	}
}

/*
 * API functions
 */

static int dma_gd32_config(const struct device *dev, uint32_t channel,
			   struct dma_config *dma_cfg)
{
	const struct dma_gd32_config *cfg = dev->config;
	struct dma_gd32_data *data = dev->data;
	struct dma_gd32_srcdst_config src_cfg;
	struct dma_gd32_srcdst_config dst_cfg;
	struct dma_gd32_srcdst_config *memory_cfg = NULL;
	struct dma_gd32_srcdst_config *periph_cfg = NULL;

	if (channel >= cfg->channels) {
		LOG_ERR("channel must be < %" PRIu32 " (%" PRIu32 ")",
			cfg->channels, channel);
		return -EINVAL;
	}

	if (dma_cfg->block_count != 1) {
		LOG_ERR("chained block transfer not supported.");
		return -ENOTSUP;
	}

	if (dma_cfg->channel_priority > 3) {
		LOG_ERR("channel_priority must be < 4 (%" PRIu32 ")",
			dma_cfg->channel_priority);
		return -EINVAL;
	}

	if (dma_cfg->head_block->source_addr_adj == DMA_ADDR_ADJ_DECREMENT) {
		LOG_ERR("source_addr_adj not supported DMA_ADDR_ADJ_DECREMENT");
		return -ENOTSUP;
	}

	if (dma_cfg->head_block->dest_addr_adj == DMA_ADDR_ADJ_DECREMENT) {
		LOG_ERR("dest_addr_adj not supported DMA_ADDR_ADJ_DECREMENT");
		return -ENOTSUP;
	}

	if (dma_cfg->head_block->source_addr_adj != DMA_ADDR_ADJ_INCREMENT &&
	    dma_cfg->head_block->source_addr_adj != DMA_ADDR_ADJ_NO_CHANGE) {
		LOG_ERR("invalid source_addr_adj %" PRIu16,
			dma_cfg->head_block->source_addr_adj);
		return -ENOTSUP;
	}
	if (dma_cfg->head_block->dest_addr_adj != DMA_ADDR_ADJ_INCREMENT &&
	    dma_cfg->head_block->dest_addr_adj != DMA_ADDR_ADJ_NO_CHANGE) {
		LOG_ERR("invalid dest_addr_adj %" PRIu16,
			dma_cfg->head_block->dest_addr_adj);
		return -ENOTSUP;
	}

	if (dma_cfg->source_data_size != 1 && dma_cfg->source_data_size != 2 &&
	    dma_cfg->source_data_size != 4) {
		LOG_ERR("source_data_size must be 1, 2, or 4 (%" PRIu32 ")",
			dma_cfg->source_data_size);
		return -EINVAL;
	}

	if (dma_cfg->dest_data_size != 1 && dma_cfg->dest_data_size != 2 &&
	    dma_cfg->dest_data_size != 4) {
		LOG_ERR("dest_data_size must be 1, 2, or 4 (%" PRIu32 ")",
			dma_cfg->dest_data_size);
		return -EINVAL;
	}

	if (dma_cfg->channel_direction > PERIPHERAL_TO_MEMORY) {
		LOG_ERR("channel_direction must be MEMORY_TO_MEMORY, "
			"MEMORY_TO_PERIPHERAL or PERIPHERAL_TO_MEMORY (%" PRIu32
			")",
			dma_cfg->channel_direction);
		return -ENOTSUP;
	}

	if (dma_cfg->channel_direction == MEMORY_TO_MEMORY && !cfg->mem2mem) {
		LOG_ERR("not supporting MEMORY_TO_MEMORY");
		return -ENOTSUP;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
	if (dma_cfg->dma_slot > 0xF) {
		LOG_ERR("dma_slot must be <7 (%" PRIu32 ")",
			dma_cfg->dma_slot);
		return -EINVAL;
	}
#endif

	gd32_dma_deinit(cfg->reg, channel);

	src_cfg.addr = dma_cfg->head_block->source_address;
	src_cfg.adj = dma_cfg->head_block->source_addr_adj;
	src_cfg.width = dma_cfg->source_data_size;

	dst_cfg.addr = dma_cfg->head_block->dest_address;
	dst_cfg.adj = dma_cfg->head_block->dest_addr_adj;
	dst_cfg.width = dma_cfg->dest_data_size;

	switch (dma_cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
		gd32_dma_transfer_set_memory_to_memory(cfg->reg, channel);
		memory_cfg = &dst_cfg;
		periph_cfg = &src_cfg;
		break;
	case PERIPHERAL_TO_MEMORY:
		gd32_dma_transfer_set_periph_to_memory(cfg->reg, channel);
		memory_cfg = &dst_cfg;
		periph_cfg = &src_cfg;
		break;
	case MEMORY_TO_PERIPHERAL:
		gd32_dma_transfer_set_memory_to_periph(cfg->reg, channel);
		memory_cfg = &src_cfg;
		periph_cfg = &dst_cfg;
		break;
	}

	gd32_dma_memory_address_config(cfg->reg, channel, memory_cfg->addr);
	if (memory_cfg->adj == DMA_ADDR_ADJ_INCREMENT) {
		gd32_dma_memory_increase_enable(cfg->reg, channel);
	} else {
		gd32_dma_memory_increase_disable(cfg->reg, channel);
	}

	gd32_dma_periph_address_config(cfg->reg, channel, periph_cfg->addr);
	if (periph_cfg->adj == DMA_ADDR_ADJ_INCREMENT) {
		gd32_dma_periph_increase_enable(cfg->reg, channel);
	} else {
		gd32_dma_periph_increase_disable(cfg->reg, channel);
	}

	gd32_dma_transfer_number_config(cfg->reg, channel,
					dma_cfg->head_block->block_size);
	gd32_dma_priority_config(cfg->reg, channel,
				 dma_gd32_priority(dma_cfg->channel_priority));
	gd32_dma_memory_width_config(cfg->reg, channel,
				     dma_gd32_memory_width(memory_cfg->width));
	gd32_dma_periph_width_config(cfg->reg, channel,
				     dma_gd32_periph_width(periph_cfg->width));
	gd32_dma_circulation_disable(cfg->reg, channel);
#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
	if (dma_cfg->channel_direction != MEMORY_TO_MEMORY) {
		gd32_dma_channel_subperipheral_select(cfg->reg, channel,
						      dma_cfg->dma_slot);
	}
#endif

	data->channels[channel].callback = dma_cfg->dma_callback;
	data->channels[channel].user_data = dma_cfg->user_data;
	data->channels[channel].direction = dma_cfg->channel_direction;

	return 0;
}

static int dma_gd32_reload(const struct device *dev, uint32_t ch, uint32_t src,
			   uint32_t dst, size_t size)
{
	const struct dma_gd32_config *cfg = dev->config;
	struct dma_gd32_data *data = dev->data;

	if (ch >= cfg->channels) {
		LOG_ERR("reload channel must be < %" PRIu32 " (%" PRIu32 ")",
			cfg->channels, ch);
		return -EINVAL;
	}

	if (data->channels[ch].busy) {
		return -EBUSY;
	}

	gd32_dma_channel_disable(cfg->reg, ch);

	gd32_dma_transfer_number_config(cfg->reg, ch, size);

	switch (data->channels[ch].direction) {
	case MEMORY_TO_MEMORY:
	case PERIPHERAL_TO_MEMORY:
		gd32_dma_memory_address_config(cfg->reg, ch, dst);
		gd32_dma_periph_address_config(cfg->reg, ch, src);
		break;
	case MEMORY_TO_PERIPHERAL:
		gd32_dma_memory_address_config(cfg->reg, ch, src);
		gd32_dma_periph_address_config(cfg->reg, ch, dst);
		break;
	}

	gd32_dma_channel_enable(cfg->reg, ch);

	return 0;
}

static int dma_gd32_start(const struct device *dev, uint32_t ch)
{
	const struct dma_gd32_config *cfg = dev->config;
	struct dma_gd32_data *data = dev->data;

	if (ch >= cfg->channels) {
		LOG_ERR("start channel must be < %" PRIu32 " (%" PRIu32 ")",
			cfg->channels, ch);
		return -EINVAL;
	}

	gd32_dma_interrupt_enable(cfg->reg, ch,
				  DMA_CHXCTL_FTFIE | GD32_DMA_INTERRUPT_ERRORS);
	gd32_dma_channel_enable(cfg->reg, ch);
	data->channels[ch].busy = true;

	return 0;
}

static int dma_gd32_stop(const struct device *dev, uint32_t ch)
{
	const struct dma_gd32_config *cfg = dev->config;
	struct dma_gd32_data *data = dev->data;

	if (ch >= cfg->channels) {
		LOG_ERR("stop channel must be < %" PRIu32 " (%" PRIu32 ")",
			cfg->channels, ch);
		return -EINVAL;
	}

	gd32_dma_interrupt_disable(
		cfg->reg, ch, DMA_CHXCTL_FTFIE | GD32_DMA_INTERRUPT_ERRORS);
	gd32_dma_interrupt_flag_clear(cfg->reg, ch,
				      DMA_FLAG_FTF | GD32_DMA_FLAG_ERRORS);
	gd32_dma_channel_disable(cfg->reg, ch);
	data->channels[ch].busy = false;

	return 0;
}

static int dma_gd32_get_status(const struct device *dev, uint32_t ch,
			       struct dma_status *stat)
{
	const struct dma_gd32_config *cfg = dev->config;
	struct dma_gd32_data *data = dev->data;

	if (ch >= cfg->channels) {
		LOG_ERR("channel must be < %" PRIu32 " (%" PRIu32 ")",
			cfg->channels, ch);
		return -EINVAL;
	}

	stat->pending_length = gd32_dma_transfer_number_get(cfg->reg, ch);
	stat->dir = data->channels[ch].direction;
	stat->busy = data->channels[ch].busy;

	return 0;
}

static bool dma_gd32_api_chan_filter(const struct device *dev, int ch,
				     void *filter_param)
{
	uint32_t filter;

	if (!filter_param) {
		LOG_ERR("filter_param must not be NULL");
		return false;
	}

	filter = *((uint32_t *)filter_param);

	return (filter & BIT(ch));
}

static int dma_gd32_init(const struct device *dev)
{
	const struct dma_gd32_config *cfg = dev->config;

	(void)clock_control_on(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clkid);

#if DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1)
	(void)reset_line_toggle_dt(&cfg->reset);
#endif

	for (uint32_t i = 0; i < cfg->channels; i++) {
		gd32_dma_interrupt_disable(cfg->reg, i,
			   DMA_CHXCTL_FTFIE | GD32_DMA_INTERRUPT_ERRORS);
		gd32_dma_deinit(cfg->reg, i);
	}

	cfg->irq_configure();

	return 0;
}

static void dma_gd32_isr(const struct device *dev)
{
	const struct dma_gd32_config *cfg = dev->config;
	struct dma_gd32_data *data = dev->data;
	uint32_t errflag, ftfflag;
	int err = 0;

	for (uint32_t i = 0; i < cfg->channels; i++) {
		errflag = gd32_dma_interrupt_flag_get(cfg->reg, i,
						      GD32_DMA_FLAG_ERRORS);
		ftfflag =
			gd32_dma_interrupt_flag_get(cfg->reg, i, DMA_FLAG_FTF);

		if (errflag == 0 && ftfflag == 0) {
			continue;
		}

		if (errflag) {
			err = -EIO;
		}

		gd32_dma_interrupt_flag_clear(
			cfg->reg, i, DMA_FLAG_FTF | GD32_DMA_FLAG_ERRORS);
		data->channels[i].busy = false;

		if (data->channels[i].callback) {
			data->channels[i].callback(
				dev, data->channels[i].user_data, i, err);
		}
	}
}

static const struct dma_driver_api dma_gd32_driver_api = {
	.config = dma_gd32_config,
	.reload = dma_gd32_reload,
	.start = dma_gd32_start,
	.stop = dma_gd32_stop,
	.get_status = dma_gd32_get_status,
	.chan_filter = dma_gd32_api_chan_filter,
};

#define IRQ_CONFIGURE(n, inst)                                                 \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq),                          \
		    DT_INST_IRQ_BY_IDX(inst, n, priority), dma_gd32_isr,       \
		    DEVICE_DT_INST_GET(inst), 0);                              \
	irq_enable(DT_INST_IRQ_BY_IDX(inst, n, irq));

#define CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, IRQ_CONFIGURE, (), inst)

#define GD32_DMA_INIT(inst)                                                    \
	static void dma_gd32##inst##_irq_configure(void)                       \
	{                                                                      \
		CONFIGURE_ALL_IRQS(inst, DT_NUM_IRQS(DT_DRV_INST(inst)));      \
	}                                                                      \
	static const struct dma_gd32_config dma_gd32##inst##_config = {        \
		.reg = DT_INST_REG_ADDR(inst),                                 \
		.channels = DT_INST_PROP(inst, dma_channels),                  \
		.clkid = DT_INST_CLOCKS_CELL(inst, id),                        \
		.mem2mem = DT_INST_PROP(inst, gd_mem2mem),                     \
		IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(gd_gd32_dma_v1),          \
			   (.reset = RESET_DT_SPEC_INST_GET(inst),))           \
		.irq_configure = dma_gd32##inst##_irq_configure,               \
	};                                                                     \
                                                                               \
	static struct dma_gd32_channel                                         \
		dma_gd32##inst##_channels[DT_INST_PROP(inst, dma_channels)];   \
	ATOMIC_DEFINE(dma_gd32_atomic##inst,                                   \
		      DT_INST_PROP(inst, dma_channels));                       \
	static struct dma_gd32_data dma_gd32##inst##_data = {                  \
		.ctx =  {                                                      \
			.magic = DMA_MAGIC,                                    \
			.atomic = dma_gd32_atomic##inst,                       \
			.dma_channels = DT_INST_PROP(inst, dma_channels),      \
		},                                                             \
		.channels = dma_gd32##inst##_channels,                         \
	};                                                                     \
                                                                               \
	DEVICE_DT_INST_DEFINE(inst, &dma_gd32_init, NULL,                      \
			      &dma_gd32##inst##_data,                          \
			      &dma_gd32##inst##_config, POST_KERNEL,           \
			      CONFIG_DMA_INIT_PRIORITY, &dma_gd32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GD32_DMA_INIT)
