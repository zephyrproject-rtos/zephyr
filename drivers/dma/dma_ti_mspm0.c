/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_dma

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_ti_mspm0.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ti_mspm0_dma, CONFIG_DMA_LOG_LEVEL);

#define DMA_TI_MSPM0_BASE_CHANNEL_NUM	1
#define DMA_GET_CHANNEL_FROM_STATUS(status)	\
			((status) - DMA_TI_MSPM0_BASE_CHANNEL_NUM)

/*
 * DMA register map.
 * CPU_INT and GEN_EVENT share the same interrupt-controller shape.
 */
struct dma_mspm0_int_regs {
	volatile const uint32_t iidx; /**< Interrupt Index Register, offset: 0x00 */
	uint32_t reserved0;           /**< Reserved, offset: 0x04 - 0x08 */
	volatile uint32_t imask;      /**< Interrupt Mask Register, offset: 0x08 */
	uint32_t reserved1;           /**< Reserved, offset: 0x0C - 0x10 */
	volatile const uint32_t ris;  /**< Raw Interrupt Status Register, offset: 0x10 */
	uint32_t reserved2;           /**< Reserved, offset: 0x14 - 0x18 */
	volatile const uint32_t mis;  /**< Masked Interrupt Status Register, offset: 0x18 */
	uint32_t reserved3;           /**< Reserved, offset: 0x1C - 0x20 */
	volatile uint32_t iset;       /**< Interrupt Set Register, offset: 0x20 */
	uint32_t reserved4;           /**< Reserved, offset: 0x24 - 0x28 */
	volatile uint32_t iclr;       /**< Interrupt Clear Register, offset: 0x28 */
};

struct dma_mspm0_chan_regs {
	volatile uint32_t dmactl; /**< DMA Channel Control Register, offset: 0x00 */
	volatile uint32_t dmasa;  /**< DMA Channel Source Address Register, offset: 0x04 */
	volatile uint32_t dmada;  /**< DMA Channel Destination Address Register, offset: 0x08 */
	volatile uint32_t dmasz;  /**< DMA Channel Size Register, offset: 0x0C */
};

struct dma_mspm0_regs {
	uint32_t reserved0[256];                /**< Reserved, offset: 0x000 - 0x400 */
	volatile uint32_t fsub_0;               /**< Subscriber Port 0, offset: 0x400 */
	volatile uint32_t fsub_1;               /**< Subscriber Port 1, offset: 0x404 */
	uint32_t reserved1[15];                 /**< Reserved, offset: 0x408 - 0x444 */
	volatile uint32_t fpub_1;               /**< Publisher Port 1, offset: 0x444 */
	uint32_t reserved2[756];                /**< Reserved, offset: 0x448 - 0x1018 */
	volatile uint32_t pdbgctl;              /**< Peripheral Debug Control, offset: 0x1018 */
	uint32_t reserved3;                     /**< Reserved, offset: 0x101C - 0x1020 */
	struct dma_mspm0_int_regs cpu_int;      /**< CPU Interrupt Registers, offset: 0x1020 */
	uint32_t reserved4;                     /**< Reserved, offset: 0x104C - 0x1050 */
	struct dma_mspm0_int_regs gen_event;    /**< General Event Registers, offset: 0x1050 */
	uint32_t reserved5[25];                 /**< Reserved, offset: 0x107C - 0x10E0 */
	volatile uint32_t evt_mode;             /**< Event Mode Register, offset: 0x10E0 */
	uint32_t reserved6[6];                  /**< Reserved, offset: 0x10E4 - 0x10FC */
	volatile const uint32_t desc;           /**< Descriptor Register, offset: 0x10FC */
	volatile uint32_t dmaprio;              /**< Channel Priority Control, offset: 0x1100 */
	uint32_t reserved7[3];                  /**< Reserved, offset: 0x1104 - 0x1110 */
	volatile uint32_t dmatctl[16];          /**< Trigger Select Registers, offset: 0x1110 */
	uint32_t reserved8[44];                 /**< Reserved, offset: 0x1150 - 0x1200 */
	struct dma_mspm0_chan_regs dmachan[16]; /**< Channel Registers, offset: 0x1200 */
};

/* dmactl bits (per-channel) */
#define DMA_MSPM0_CTL_DMAREQ    BIT(0)
#define DMA_MSPM0_CTL_DMAEN     BIT(1)
#define DMA_MSPM0_CTL_SRCWDTH   GENMASK(10, 8)
#define DMA_MSPM0_CTL_DSTWDTH   GENMASK(14, 12)
#define DMA_MSPM0_CTL_SRCINCR   GENMASK(19, 16)
#define DMA_MSPM0_CTL_DSTINCR   GENMASK(23, 20)
#define DMA_MSPM0_CTL_EM        GENMASK(25, 24)
#define DMA_MSPM0_CTL_TM        GENMASK(29, 28)
#define DMA_MSPM0_CTL_PREIRQ    GENMASK(6, 4)

#define DMA_MSPM0_WIDTH_BYTE     0x0U
#define DMA_MSPM0_WIDTH_HALF     0x1U
#define DMA_MSPM0_WIDTH_WORD     0x2U
#define DMA_MSPM0_WIDTH_LONG     0x3U

#define DMA_MSPM0_INCR_UNCHANGED 0x0U
#define DMA_MSPM0_INCR_DECREMENT 0x2U
#define DMA_MSPM0_INCR_INCREMENT 0x3U
#define DMA_MSPM0_INCR_STRIDE(n) (0x8U + ((n) - 2U))

#define DMA_MSPM0_EM_NORMAL      0x0U
#define DMA_MSPM0_EM_GATHER      0x1U
#define DMA_MSPM0_EM_FILL        0x2U
#define DMA_MSPM0_EM_TABLE       0x3U

#define DMA_MSPM0_TM_SINGLE      0x0U
#define DMA_MSPM0_TM_BLOCK       0x1U
#define DMA_MSPM0_TM_RPTSNGL     0x2U
#define DMA_MSPM0_TM_RPTBLCK     0x3U

#define DMA_MSPM0_PREIRQ_DISABLE 0x0U
#define DMA_MSPM0_PREIRQ_HALF    0x7U

/* dmatctl bits (per-channel, dmatctl[n]) */
#define DMA_MSPM0_TCTL_DMATSEL  GENMASK(5, 0)
#define DMA_MSPM0_TCTL_DMATINT  BIT(7)
#define DMA_MSPM0_TCTL_SWREQ    0x0U

/* dmaprio bits (instance-wide) */
#define DMA_MSPM0_PRIO_ROUNDROBIN BIT(0)
#define DMA_MSPM0_PRIO_BURSTSZ    GENMASK(17, 16)
#define DMA_MSPM0_BURST_INFINITE  0x0U
#define DMA_MSPM0_BURST_8         0x1U
#define DMA_MSPM0_BURST_16        0x2U
#define DMA_MSPM0_BURST_32        0x3U

/* cpu_int / gen_event IIDX status codes:
 * channel N reports (N + 1)
 * early/pre-IRQ event reports (0x11 + N)
 */
#define DMA_MSPM0_IIDX_NONE        0x00U
#define DMA_MSPM0_IIDX_PREIRQ_BASE 0x11U
#define DMA_MSPM0_IIDX_PREIRQ_LAST 0x18U
#define DMA_MSPM0_IIDX_ADDRERR     0x19U
#define DMA_MSPM0_IIDX_DATAERR     0x1AU
#define DMA_MSPM0_IMASK_PREIRQ(n)  BIT(16U + (n))
#define DMA_MSPM0_IMASK_ADDRERR    BIT(24)
#define DMA_MSPM0_IMASK_DATAERR    BIT(25)
#define DMA_MSPM0_PREIRQ_MAX_CHANNELS \
	(DMA_MSPM0_IIDX_PREIRQ_LAST - DMA_MSPM0_IIDX_PREIRQ_BASE + 1)

/* Data Transfer Width, in bytes -- matches Zephyr's source/dest_data_size */
#define DMA_TI_MSPM0_DATAWIDTH_BYTE	1
#define DMA_TI_MSPM0_DATAWIDTH_HALF	2
#define DMA_TI_MSPM0_DATAWIDTH_WORD	4
#define DMA_TI_MSPM0_DATAWIDTH_LONG	8

struct dma_ti_mspm0_config {
	struct dma_mspm0_regs *regs;
	uint8_t dma_max_channels;
	void (*irq_config_func)(void);
	uint8_t num_full_channels;
	bool round_robin;
	uint8_t burst_size;
};

struct dma_ti_mspm0_channel_data {
	dma_callback_t dma_callback;
	void *user_data;
	uint8_t direction;
	bool busy;
	uint8_t source_data_size;
	bool cyclic;
	bool error_dis;
	bool sw_triggered;
	struct dma_block_config *next_block;
	bool per_block_cb;
};

struct dma_ti_mspm0_data {
	struct dma_context dma_ctx;
	struct k_spinlock lock;
	struct dma_ti_mspm0_channel_data *ch_data;
};

static inline int dma_ti_mspm0_get_memory_increment(uint8_t adj,
						    uint32_t *increment)
{
	if (increment == NULL) {
		return -EINVAL;
	}

	switch (adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		*increment = DMA_MSPM0_INCR_INCREMENT;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		*increment = DMA_MSPM0_INCR_UNCHANGED;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		*increment = DMA_MSPM0_INCR_DECREMENT;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * HW stride steps by n * width every transfer, with no grouping/jump concept, so it only matches
 * gather/scatter's count-then-jump semantics when count <= 1; interval is n itself (a
 * dimensionless multiplier), not a byte offset. Every stride encoding only increments, never
 * decrements, so it's only valid alongside DMA_ADDR_ADJ_INCREMENT.
 */
static inline int dma_ti_mspm0_get_stride_incr(bool gather_scatter_en, uint16_t count,
					       uint16_t interval, uint8_t addr_adj, uint32_t *incr)
{
	if (gather_scatter_en) {
		if (count > 1) {
			return -ENOTSUP;
		}

		if (interval > 9) {
			return -EINVAL;
		}

		if (interval >= 2) {
			if (addr_adj != DMA_ADDR_ADJ_INCREMENT) {
				return -EINVAL;
			}

			*incr = DMA_MSPM0_INCR_STRIDE(interval);
			return 0;
		}
	}

	return dma_ti_mspm0_get_memory_increment(addr_adj, incr);
}

static inline int dma_ti_mspm0_get_datawidth(uint8_t wd, uint32_t *width)
{
	if (width == NULL) {
		return -EINVAL;
	}

	switch (wd) {
	case DMA_TI_MSPM0_DATAWIDTH_BYTE:
		*width = DMA_MSPM0_WIDTH_BYTE;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_HALF:
		*width = DMA_MSPM0_WIDTH_HALF;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_WORD:
		*width = DMA_MSPM0_WIDTH_WORD;
		break;
	case DMA_TI_MSPM0_DATAWIDTH_LONG:
		*width = DMA_MSPM0_WIDTH_LONG;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline bool dma_ti_mspm0_is_full_channel(const struct dma_ti_mspm0_config *cfg,
						uint32_t channel)
{
	return channel < cfg->num_full_channels;
}

static inline uint32_t dma_ti_mspm0_get_transfer_mode(struct dma_config *config)
{
	switch (config->channel_direction) {
	case DMA_TI_MSPM0_DIRECTION_FILL:
	case DMA_TI_MSPM0_DIRECTION_TABLE:
	case DMA_TI_MSPM0_DIRECTION_GATHER:
		return DMA_MSPM0_TM_BLOCK;
	case MEMORY_TO_MEMORY:
		return config->cyclic ? DMA_MSPM0_TM_RPTBLCK : DMA_MSPM0_TM_BLOCK;
	default:
		return config->cyclic ? DMA_MSPM0_TM_RPTSNGL : DMA_MSPM0_TM_SINGLE;
	}
}

static inline int dma_ti_mspm0_get_extended_mode(uint32_t direction, uint32_t *em)
{
	if (em == NULL) {
		return -EINVAL;
	}

	switch (direction) {
	case DMA_TI_MSPM0_DIRECTION_FILL:
		*em = DMA_MSPM0_EM_FILL;
		break;
	case DMA_TI_MSPM0_DIRECTION_TABLE:
		*em = DMA_MSPM0_EM_TABLE;
		break;
	case DMA_TI_MSPM0_DIRECTION_GATHER:
		*em = DMA_MSPM0_EM_GATHER;
		break;
	default:
		*em = DMA_MSPM0_EM_NORMAL;
		break;
	}

	return 0;
}

static int dma_ti_mspm0_configure(const struct device *dev, uint32_t channel,
				  struct dma_config *config)
{
	uint32_t ctl = 0;
	uint32_t temp;
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *dma_data = dev->data;
	struct dma_ti_mspm0_channel_data *data = NULL;
	struct dma_block_config *b_cfg = NULL;
	uint32_t tm;
	uint32_t em;
	int ret = 0;

	if ((config == NULL) || (channel >= cfg->dma_max_channels)) {
		return -EINVAL;
	}

	b_cfg = config->head_block;
	if (b_cfg == NULL) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];

	if (data->busy != false) {
		return -EBUSY;
	}

	if (config->cyclic && b_cfg->next_block != NULL) {
		return -ENOTSUP;
	}

	ret = dma_ti_mspm0_get_stride_incr(b_cfg->source_gather_en, b_cfg->source_gather_count,
					   b_cfg->source_gather_interval, b_cfg->source_addr_adj,
					   &temp);
	if (ret < 0) {
		LOG_ERR("Invalid Source address increment");
		return ret;
	}

	ctl |= FIELD_PREP(DMA_MSPM0_CTL_SRCINCR, temp);

	ret = dma_ti_mspm0_get_stride_incr(b_cfg->dest_scatter_en, b_cfg->dest_scatter_count,
					   b_cfg->dest_scatter_interval, b_cfg->dest_addr_adj,
					   &temp);
	if (ret < 0) {
		LOG_ERR("Invalid Destination address increment");
		return ret;
	}

	ctl |= FIELD_PREP(DMA_MSPM0_CTL_DSTINCR, temp);

	dma_ti_mspm0_get_extended_mode(config->channel_direction, &em);

	if (em != DMA_MSPM0_EM_NORMAL && !dma_ti_mspm0_is_full_channel(cfg, channel)) {
		return -ENOTSUP;
	}

	if (dma_ti_mspm0_get_datawidth(config->source_data_size, &temp)) {
		LOG_ERR("Invalid Source data width");
		return -EINVAL;
	}

	if (em == DMA_MSPM0_EM_TABLE && temp != DMA_MSPM0_WIDTH_LONG) {
		LOG_ERR("Table mode requires 64-bit source width");
		return -EINVAL;
	}

	ctl |= FIELD_PREP(DMA_MSPM0_CTL_SRCWDTH, temp);

	if (dma_ti_mspm0_get_datawidth(config->dest_data_size, &temp)) {
		LOG_ERR("Invalid Destination data width");
		return -EINVAL;
	}

	if (em == DMA_MSPM0_EM_TABLE && temp != DMA_MSPM0_WIDTH_WORD) {
		LOG_ERR("Table mode requires 32-bit destination width");
		return -EINVAL;
	}

	ctl |= FIELD_PREP(DMA_MSPM0_CTL_DSTWDTH, temp);

	tm = dma_ti_mspm0_get_transfer_mode(config);

	if ((tm == DMA_MSPM0_TM_RPTSNGL || tm == DMA_MSPM0_TM_RPTBLCK) &&
	    !dma_ti_mspm0_is_full_channel(cfg, channel)) {
		return -ENOTSUP;
	}

	ctl |= FIELD_PREP(DMA_MSPM0_CTL_TM, tm);
	ctl |= FIELD_PREP(DMA_MSPM0_CTL_EM, em);

	if (config->half_complete_callback_en) {
		if (channel < DMA_MSPM0_PREIRQ_MAX_CHANNELS &&
		    dma_ti_mspm0_is_full_channel(cfg, channel)) {
			ctl |= FIELD_PREP(DMA_MSPM0_CTL_PREIRQ, DMA_MSPM0_PREIRQ_HALF);
		} else {
			return -ENOTSUP;
		}
	}

	data->direction = config->channel_direction;
	data->dma_callback = config->dma_callback;
	data->user_data = config->user_data;
	data->source_data_size = config->source_data_size;
	data->cyclic = config->cyclic;
	data->error_dis = config->error_callback_dis;
	data->sw_triggered = config->dma_slot == DMA_MSPM0_TCTL_SWREQ;
	data->next_block = b_cfg->next_block;
	data->per_block_cb = config->complete_callback_en;

	K_SPINLOCK(&dma_data->lock) {
		cfg->regs->cpu_int.imask &= ~BIT(channel);
		cfg->regs->dmachan[channel].dmasz = b_cfg->block_size / config->source_data_size;
		cfg->regs->dmatctl[channel] = FIELD_PREP(DMA_MSPM0_TCTL_DMATSEL, config->dma_slot);

		if (config->source_chaining_en || config->dest_chaining_en) {
			cfg->regs->dmatctl[config->linked_channel] =
				DMA_MSPM0_TCTL_DMATINT |
				FIELD_PREP(DMA_MSPM0_TCTL_DMATSEL, channel);
			cfg->regs->dmachan[config->linked_channel].dmactl |= DMA_MSPM0_CTL_DMAEN;
		}

		cfg->regs->dmachan[channel].dmactl = ctl;
		cfg->regs->dmachan[channel].dmasa = b_cfg->source_address;
		cfg->regs->dmachan[channel].dmada = b_cfg->dest_address;
		cfg->regs->cpu_int.imask |= BIT(channel);
		if (config->half_complete_callback_en && channel < DMA_MSPM0_PREIRQ_MAX_CHANNELS) {
			cfg->regs->cpu_int.imask |= DMA_MSPM0_IMASK_PREIRQ(channel);
		} else {
			cfg->regs->cpu_int.imask &= ~DMA_MSPM0_IMASK_PREIRQ(channel);
		}

		data->busy = true;
	}

	LOG_DBG("DMA Channel %u configured", channel);

	return 0;
}

static int dma_ti_mspm0_start(const struct device *dev, const uint32_t channel)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *dma_data = dev->data;

	if (channel >= cfg->dma_max_channels) {
		return -EINVAL;
	}

	cfg->regs->dmachan[channel].dmactl |= DMA_MSPM0_CTL_DMAEN;

	if (dma_data->ch_data[channel].sw_triggered) {
		cfg->regs->dmachan[channel].dmactl |= DMA_MSPM0_CTL_DMAREQ;
	}

	return 0;
}

static int dma_ti_mspm0_suspend(const struct device *dev, uint32_t channel)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;

	if (channel >= cfg->dma_max_channels) {
		return -EINVAL;
	}

	cfg->regs->dmachan[channel].dmactl &= ~DMA_MSPM0_CTL_DMAEN;

	return 0;
}

static int dma_ti_mspm0_stop(const struct device *dev, const uint32_t channel)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *data = dev->data;

	if (channel >= cfg->dma_max_channels) {
		return -EINVAL;
	}

	cfg->regs->dmachan[channel].dmactl &= ~DMA_MSPM0_CTL_DMAEN;
	data->ch_data[channel].busy = false;

	return 0;
}

static int dma_ti_mspm0_reload(const struct device *dev, uint32_t channel,
			       uint32_t src_addr, uint32_t dest_addr, size_t size)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_channel_data *data = NULL;
	struct dma_ti_mspm0_data *dma_data = dev->data;

	if (channel >= cfg->dma_max_channels) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];
	cfg->regs->dmachan[channel].dmasa = src_addr;
	cfg->regs->dmachan[channel].dmada = dest_addr;
	cfg->regs->dmachan[channel].dmasz = size / data->source_data_size;
	data->next_block = NULL;
	data->busy = true;

	return 0;
}

static int dma_ti_mspm0_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *stat)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *dma_data = dev->data;
	struct dma_ti_mspm0_channel_data *data;

	if (channel >= cfg->dma_max_channels) {
		return -EINVAL;
	}

	data = &dma_data->ch_data[channel];
	stat->pending_length = cfg->regs->dmachan[channel].dmasz;
	stat->dir = data->direction;
	stat->busy = data->busy;

	return 0;
}

static inline bool dma_ti_mspm0_chain_next_block(const struct dma_ti_mspm0_config *cfg,
						 struct dma_ti_mspm0_channel_data *data,
						 uint32_t channel)
{
	struct dma_block_config *block = data->next_block;

	if (block == NULL) {
		return false;
	}

	data->next_block = block->next_block;
	cfg->regs->dmachan[channel].dmasa = block->source_address;
	cfg->regs->dmachan[channel].dmada = block->dest_address;
	cfg->regs->dmachan[channel].dmasz = block->block_size / data->source_data_size;
	cfg->regs->dmachan[channel].dmactl |= DMA_MSPM0_CTL_DMAEN;
	if (data->sw_triggered) {
		cfg->regs->dmachan[channel].dmactl |= DMA_MSPM0_CTL_DMAREQ;
	}

	return true;
}

static inline void dma_ti_mspm0_isr(const struct device *dev)
{
	int status;
	uint32_t channel;
	const struct dma_ti_mspm0_config *cfg = dev->config;
	struct dma_ti_mspm0_data *dma_data = dev->data;
	struct dma_ti_mspm0_channel_data *data;

	/* Reading IIDX also latches-clears the highest priority pending flag */
	status = cfg->regs->cpu_int.iidx;
	if (status == DMA_MSPM0_IIDX_NONE) {
		return;
	}

	if (status == DMA_MSPM0_IIDX_ADDRERR || status == DMA_MSPM0_IIDX_DATAERR) {
		int err = (status == DMA_MSPM0_IIDX_ADDRERR) ? -EFAULT : -EIO;

		for (channel = 0; channel < cfg->dma_max_channels; channel++) {
			data = &dma_data->ch_data[channel];
			if (data->busy && data->dma_callback != NULL && !data->error_dis) {
				data->dma_callback(dev, data->user_data, channel, err);
			}
		}

		return;
	}

	if (status >= DMA_MSPM0_IIDX_PREIRQ_BASE && status <= DMA_MSPM0_IIDX_PREIRQ_LAST) {
		channel = status - DMA_MSPM0_IIDX_PREIRQ_BASE;
		if (channel >= cfg->dma_max_channels) {
			return;
		}

		data = &dma_data->ch_data[channel];
		if (data->dma_callback != NULL) {
			data->dma_callback(dev, data->user_data, channel,
					    DMA_STATUS_HALF_COMPLETE);
		}

		return;
	}

	channel = DMA_GET_CHANNEL_FROM_STATUS(status);
	if (channel >= cfg->dma_max_channels) {
		return;
	}

	data = &dma_data->ch_data[channel];

	if (dma_ti_mspm0_chain_next_block(cfg, data, channel)) {
		if (data->per_block_cb && data->dma_callback != NULL) {
			data->dma_callback(dev, data->user_data, channel, DMA_STATUS_COMPLETE);
		}
		return;
	}

	if (!data->cyclic) {
		cfg->regs->dmachan[channel].dmactl &= ~DMA_MSPM0_CTL_DMAEN;
		data->busy = false;
	}

	if (data->dma_callback != NULL) {
		data->dma_callback(dev, data->user_data, channel, DMA_STATUS_COMPLETE);
	}
}

static inline uint32_t dma_ti_mspm0_encode_burst_size(uint32_t burst_size)
{
	switch (burst_size) {
	case 32:
		return DMA_MSPM0_BURST_32;
	case 16:
		return DMA_MSPM0_BURST_16;
	case 8:
		return DMA_MSPM0_BURST_8;
	default:
		return DMA_MSPM0_BURST_INFINITE;
	}
}

static int dma_ti_mspm0_init(const struct device *dev)
{
	const struct dma_ti_mspm0_config *cfg = dev->config;
	uint32_t dmaprio = 0;

	if (cfg->irq_config_func != NULL) {
		cfg->irq_config_func();
	}

	if (cfg->round_robin) {
		dmaprio |= DMA_MSPM0_PRIO_ROUNDROBIN;
	}

	dmaprio |= FIELD_PREP(DMA_MSPM0_PRIO_BURSTSZ,
			      dma_ti_mspm0_encode_burst_size(cfg->burst_size));

	cfg->regs->dmaprio = dmaprio;

	cfg->regs->cpu_int.imask |= DMA_MSPM0_IMASK_ADDRERR | DMA_MSPM0_IMASK_DATAERR;

	return 0;
}

static DEVICE_API(dma, dma_ti_mspm0_api) = {
	.config		= dma_ti_mspm0_configure,
	.start		= dma_ti_mspm0_start,
	.stop		= dma_ti_mspm0_stop,
	.suspend	= dma_ti_mspm0_suspend,
	.resume		= dma_ti_mspm0_start,
	.reload		= dma_ti_mspm0_reload,
	.get_status	= dma_ti_mspm0_get_status,
};

#define MSPM0_DMA_INIT(inst)							\
										\
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, dma_channels),			\
		     "DMA channels is required");				\
										\
	BUILD_ASSERT(DT_INST_PROP(inst, ti_num_full_channels) <=			\
		     DT_INST_PROP(inst, dma_channels),				\
		     "ti,num-full-channels can't exceed dma-channels");	\
										\
	static inline void dma_ti_mspm0_irq_cfg_##inst(void)			\
	{									\
		irq_disable(DT_INST_IRQN(inst));				\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),	\
			    dma_ti_mspm0_isr, DEVICE_DT_INST_GET(inst), 0);	\
										\
		irq_enable(DT_INST_IRQN(inst));					\
	}									\
										\
	static struct dma_ti_mspm0_channel_data					\
			channel_data_##inst[DT_INST_PROP(inst, dma_channels)];	\
										\
	static const struct dma_ti_mspm0_config dma_cfg_##inst = {		\
		.num_full_channels = DT_INST_PROP(inst, ti_num_full_channels),	\
		.regs		  = (struct dma_mspm0_regs *)DT_INST_REG_ADDR(inst),	\
		.dma_max_channels = DT_INST_PROP(inst, dma_channels),		\
		.irq_config_func  = dma_ti_mspm0_irq_cfg_##inst,		\
		.round_robin	   = DT_INST_PROP(inst, ti_round_robin_priority),\
		.burst_size	   = DT_INST_PROP(inst, ti_burst_size),		\
	};									\
										\
	static struct dma_ti_mspm0_data dma_data_##inst = {			\
		.ch_data = channel_data_##inst,					\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, &dma_ti_mspm0_init, NULL,			\
			      &dma_data_##inst, &dma_cfg_##inst,		\
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,		\
			      &dma_ti_mspm0_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_DMA_INIT);
