/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dma_mchp_xdmac_g1.c
 * @brief Zephyr DMA driver for Microchip XDMAC G1 peripheral.
 *
 */

#define DT_DRV_COMPAT microchip_xdmac_g1_dma

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(dma_mchp_xdmac_g1, CONFIG_DMA_LOG_LEVEL);

/* Per-channel register block: starts at BASE + 0x60, stride 0x40 */
#define XDMAC_CH_OFST(ch)								\
		(offsetof(xdmac_registers_t, XDMAC_CHID) + ((ch) * sizeof(xdmac_chid_registers_t)))

/* PERID value used for memory-to-memory transfers (no hardware trigger) */
#define XDMAC_PERID_MEM2MEM 0x7FU

/* Interrupt error mask – enabled for all channels unless error cb disabled */
#define XDMAC_INT_ERR (XDMAC_CIE_RBIE_Msk | XDMAC_CIE_WBIE_Msk | XDMAC_CIE_ROIE_Msk)

/* Maximum number of channels this driver can manage (hardware maximum) */
#define XDMAC_CHANNELS_MAX 32U

/* Read/write a global register at BASE + offset */
static inline uint32_t mchp_xdmac_glob_read(mm_reg_t base, uint32_t off)
{
	return sys_read32(base + off);
}

static inline void mchp_xdmac_glob_write(mm_reg_t base, uint32_t off, uint32_t val)
{
	sys_write32(val, base + off);
}

/* Read/write a per-channel register */
static inline uint32_t mchp_xdmac_ch_read(mm_reg_t base, uint32_t ch, uint32_t off)
{
	return sys_read32(base + XDMAC_CH_OFST(ch) + off);
}

static inline void mchp_xdmac_ch_write(mm_reg_t base, uint32_t ch, uint32_t off, uint32_t val)
{
	sys_write32(val, base + XDMAC_CH_OFST(ch) + off);
}

/*
 * -----------------------------------------------------------------------------
 * Driver data structures
 * -----------------------------------------------------------------------------
 */

/** Channel state machine */
enum xdmac_ch_state {
	XDMAC_CH_STATE_INIT = 0,
	XDMAC_CH_STATE_CONFIGURED,
	XDMAC_CH_STATE_RUNNING,
	XDMAC_CH_STATE_SUSPENDED,
};

/** Per-channel run-time data */
struct mchp_xdmac_chan {
	dma_callback_t callback;
	void *user_data;
	uint32_t data_size; /* log2 of element width (0=byte,1=hw,2=word) */
	enum xdmac_ch_state state;
};

/** Device constant configuration (built from DT) */
struct mchp_xdmac_cfg {
	DEVICE_MMIO_ROM;
	void (*irq_config)(void);
	const struct sam_clk_cfg clock_cfg;
	uint8_t irq_id;
};

/** Device run-time data */
struct mchp_xdmac_data {
	DEVICE_MMIO_RAM;
	struct dma_context dma_ctx;
	struct mchp_xdmac_chan channels[XDMAC_CHANNELS_MAX];
};

/*
 * -----------------------------------------------------------------------------
 * Low-level channel helpers
 * -----------------------------------------------------------------------------
 */

/**
 * @brief Return true if the given channel is currently enabled (running).
 */
static inline bool xdmac_ch_is_enabled(mm_reg_t base, uint32_t ch)
{
	return (mchp_xdmac_glob_read(base, XDMAC_GS_REG_OFST) & BIT(ch)) != 0U;
}

/**
 * @brief Disable all channel-level interrupts and clear pending status.
 */
static void xdmac_ch_int_disable_all(mm_reg_t base, uint32_t ch)
{
	/* Write to CID to disable all channel interrupts */
	mchp_xdmac_ch_write(base, ch, XDMAC_CID_REG_OFST, XDMAC_CID_Msk);

	/* Read CIS to clear any latched status bits */
	(void)mchp_xdmac_ch_read(base, ch, XDMAC_CIS_REG_OFST);
}

/* ISR for XDMAC */
static void mchp_xdmac_isr(const struct device *dev)
{
	struct mchp_xdmac_data *data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t num_ch = data->dma_ctx.dma_channels;
	uint32_t gis;
	uint32_t cis;
	int err;

	/* Read and latch global interrupt status */
	gis = mchp_xdmac_glob_read(base, XDMAC_GIS_REG_OFST);

	for (uint32_t ch = 0U; ch < num_ch; ch++) {
		if ((gis & BIT(ch)) == 0U) {
			continue;
		}

		/* Read per-channel interrupt status (self-clearing on read) */
		cis = mchp_xdmac_ch_read(base, ch, XDMAC_CIS_REG_OFST);

		/* Transition channel back to CONFIGURED so it may be reused */
		data->channels[ch].state = XDMAC_CH_STATE_CONFIGURED;

		/* Determine error condition */
		err = (cis & XDMAC_INT_ERR) != 0U ? -EIO : DMA_STATUS_COMPLETE;

		if (data->channels[ch].callback != NULL) {
			data->channels[ch].callback(dev, data->channels[ch].user_data, ch, err);
		}
	}
}

/* DMA API – config */
static int mchp_xdmac_config(const struct device *dev, uint32_t channel, struct dma_config *cfg)
{
	struct mchp_xdmac_data *dev_data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t num_ch = dev_data->dma_ctx.dma_channels;
	uint32_t sam = XDMAC_CC_SAM_FIXED_AM;
	uint32_t dam = XDMAC_CC_DAM_FIXED_AM;
	uint32_t burst_size;
	uint32_t data_size;
	uint32_t cc;
	uint32_t cie;

	if (channel >= num_ch) {
		LOG_ERR("channel %u out of range (max %u)", channel, num_ch - 1U);
		return -EINVAL;
	}

	if (dev_data->channels[channel].state != XDMAC_CH_STATE_INIT &&
	    dev_data->channels[channel].state != XDMAC_CH_STATE_CONFIGURED) {
		LOG_ERR("channel %u not in configurable", channel);
		return -EBUSY;
	}

	if (cfg->source_data_size != cfg->dest_data_size) {
		LOG_ERR("source_data_size (%u) != dest_data_size (%u)", cfg->source_data_size,
			cfg->dest_data_size);
		return -EINVAL;
	}

	if (cfg->source_data_size != 1U && cfg->source_data_size != 2U &&
	    cfg->source_data_size != 4U && cfg->source_data_size != 8U &&
	    cfg->source_data_size != 16U) {
		LOG_ERR("unsupported source_data_size %u (must be 1, 2 or 4)",
			cfg->source_data_size);
		return -EINVAL;
	}

	if (cfg->source_burst_length != cfg->dest_burst_length) {
		LOG_ERR("source_burst_length (%u) != dest_burst_length (%u)",
			cfg->source_burst_length, cfg->dest_burst_length);
		return -EINVAL;
	}

	if (cfg->block_count != 1U) {
		LOG_ERR("only single-block transfers are supported (block_count=%u)",
			cfg->block_count);
		return -EINVAL;
	}

	/* Check hardware is not already running this channel */
	if (xdmac_ch_is_enabled(base, channel)) {
		LOG_ERR("channel %u is currently running", channel);
		return -EBUSY;
	}

	/* ------------------------------------------------------------------ */
	/* Compute XDMAC_CC register value                                    */
	/* ------------------------------------------------------------------ */

	/*
	 * burst_size: CSIZE field encodes 1,2,4,8,16 as 0..4.
	 * The Zephyr DMA API passes the literal burst length, so use
	 * find_msb_set() to get the log2 exponent.
	 */
	burst_size = find_msb_set(cfg->source_burst_length) - 1U;

	/* data_size: DWIDTH field 0=byte, 1=halfword, 2=word */
	data_size = find_msb_set(cfg->source_data_size) - 1U;

	/* Store for reload() */
	dev_data->channels[channel].data_size = data_size;

	/* Source and destination address increment mode */

	if (cfg->head_block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
		sam = XDMAC_CC_SAM_INCREMENTED_AM;
	}
	if (cfg->head_block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
		dam = XDMAC_CC_DAM_INCREMENTED_AM;
	}

	switch (cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
		cc = XDMAC_CC_TYPE_MEM_TRAN |
		     XDMAC_CC_MBSIZE(burst_size <= 1U ? 0U : burst_size - 1U) |
		     XDMAC_CC_SAM_INCREMENTED_AM | XDMAC_CC_DAM_INCREMENTED_AM;

		/* When a memory-to-memory transfer is performed, configure PERID to 0x7F */
		cfg->dma_slot = XDMAC_PERID_MEM2MEM;
		break;

	case MEMORY_TO_PERIPHERAL:
		cc = XDMAC_CC_TYPE_PER_TRAN | XDMAC_CC_CSIZE(burst_size) | XDMAC_CC_DSYNC_MEM2PER |
		     sam | dam;
		break;

	case PERIPHERAL_TO_MEMORY:
		cc = XDMAC_CC_TYPE_PER_TRAN | XDMAC_CC_CSIZE(burst_size) | XDMAC_CC_DSYNC_PER2MEM |
		     sam | dam;
		break;

	default:
		LOG_ERR("unsupported channel_direction %d", cfg->channel_direction);
		return -EINVAL;
	}

	/* Apply common fields: data width, protection (unsecured), PERID */
	cc |= XDMAC_CC_DWIDTH(data_size) | XDMAC_CC_PROT_UNSEC | XDMAC_CC_PERID(cfg->dma_slot);

	/* ------------------------------------------------------------------ */
	/* Programme hardware registers                                       */
	/* ------------------------------------------------------------------ */

	/* Disable all channel interrupts and clear pending status */
	xdmac_ch_int_disable_all(base, channel);

	/* Write channel configuration */
	mchp_xdmac_ch_write(base, channel, XDMAC_CC_REG_OFST, cc);
	mchp_xdmac_ch_write(base, channel, XDMAC_CDS_MSP_REG_OFST, 0U);
	mchp_xdmac_ch_write(base, channel, XDMAC_CSUS_REG_OFST, 0U);
	mchp_xdmac_ch_write(base, channel, XDMAC_CDUS_REG_OFST, 0U);

	/* Write transfer addresses */
	mchp_xdmac_ch_write(base, channel, XDMAC_CSA_REG_OFST, cfg->head_block->source_address);
	mchp_xdmac_ch_write(base, channel, XDMAC_CDA_REG_OFST, cfg->head_block->dest_address);

	/* Microblock length: number of data elements (block_size / element_bytes) */
	mchp_xdmac_ch_write(base, channel, XDMAC_CUBC_REG_OFST,
			    cfg->head_block->block_size >> data_size);

	/* Block length: (BLEN+1) microblocks; single block -> BLEN = 0 */
	mchp_xdmac_ch_write(base, channel, XDMAC_CBC_REG_OFST, 0U);

	/* No linked-list; disable next-descriptor fetch */
	mchp_xdmac_ch_write(base, channel, XDMAC_CNDA_REG_OFST, 0U);
	mchp_xdmac_ch_write(base, channel, XDMAC_CNDC_REG_OFST, 0U);

	cie = (cfg->complete_callback_en ? XDMAC_CIE_BIE_Msk : XDMAC_CIE_LIE_Msk) |
	      XDMAC_CIE_BIE_Msk | (cfg->error_callback_dis ? 0U : XDMAC_INT_ERR);
	mchp_xdmac_ch_write(base, channel, XDMAC_CIE_REG_OFST, cie);

	/* ------------------------------------------------------------------ */
	/* Save callback info and advance state                               */
	/* ------------------------------------------------------------------ */
	dev_data->channels[channel].callback = cfg->dma_callback;
	dev_data->channels[channel].user_data = cfg->user_data;
	dev_data->channels[channel].state = XDMAC_CH_STATE_CONFIGURED;

	return 0;
}

/* DMA API – reload */
static int mchp_xdmac_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			     size_t size)
{
	struct mchp_xdmac_data *dev_data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t data_sz = dev_data->channels[channel].data_size;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("channel %u out of range", channel);
		return -EINVAL;
	}

	if (xdmac_ch_is_enabled(base, channel)) {
		LOG_ERR("channel %u is currently running", channel);
		return -EBUSY;
	}

	if (dev_data->channels[channel].state != XDMAC_CH_STATE_CONFIGURED &&
	    dev_data->channels[channel].state != XDMAC_CH_STATE_RUNNING) {
		LOG_ERR("channel %u not in a reloadable state", channel);
		return -EINVAL;
	}

	mchp_xdmac_ch_write(base, channel, XDMAC_CSA_REG_OFST, src);
	mchp_xdmac_ch_write(base, channel, XDMAC_CDA_REG_OFST, dst);
	mchp_xdmac_ch_write(base, channel, XDMAC_CUBC_REG_OFST, size >> data_sz);

	dev_data->channels[channel].state = XDMAC_CH_STATE_CONFIGURED;

	return 0;
}

/* DMA API – start */
static int mchp_xdmac_start(const struct device *dev, uint32_t channel)
{
	struct mchp_xdmac_data *dev_data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t num_ch = dev_data->dma_ctx.dma_channels;

	if (channel >= num_ch) {
		LOG_ERR("channel %u out of range", channel);
		return -EINVAL;
	}

	if (dev_data->channels[channel].state != XDMAC_CH_STATE_CONFIGURED &&
	    dev_data->channels[channel].state != XDMAC_CH_STATE_RUNNING) {
		LOG_ERR("channel %u not configured", channel);
		return -EINVAL;
	}

	if (xdmac_ch_is_enabled(base, channel)) {
		LOG_DBG("channel %u already enabled", channel);
		return -EBUSY;
	}

	/* Enable global interrupt for this channel */
	mchp_xdmac_glob_write(base, XDMAC_GIE_REG_OFST, BIT(channel));
	/* Enable (start) the channel */
	mchp_xdmac_glob_write(base, XDMAC_GE_REG_OFST, BIT(channel));

	dev_data->channels[channel].state = XDMAC_CH_STATE_RUNNING;

	return 0;
}

/* DMA API – stop */
static int mchp_xdmac_stop(const struct device *dev, uint32_t channel)
{
	struct mchp_xdmac_data *dev_data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t num_ch = dev_data->dma_ctx.dma_channels;

	if (channel >= num_ch) {
		LOG_ERR("channel %u out of range", channel);
		return -EINVAL;
	}

	if (dev_data->channels[channel].state == XDMAC_CH_STATE_INIT) {
		LOG_WRN("channel %u not configured", channel);
		return -EINVAL;
	}

	if (!xdmac_ch_is_enabled(base, channel)) {
		/* Already stopped; nothing to do */
		return 0;
	}

	/* Disable the channel */
	mchp_xdmac_glob_write(base, XDMAC_GD_REG_OFST, BIT(channel));
	/* Disable global interrupt for this channel */
	mchp_xdmac_glob_write(base, XDMAC_GID_REG_OFST, BIT(channel));
	/* Disable all per-channel interrupts and clear status */
	xdmac_ch_int_disable_all(base, channel);

	dev_data->channels[channel].state = XDMAC_CH_STATE_CONFIGURED;

	return 0;
}

/* DMA API – suspend */
static int mchp_xdmac_suspend(const struct device *dev, uint32_t channel)
{
	struct mchp_xdmac_data *dev_data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t num_ch = dev_data->dma_ctx.dma_channels;

	if (channel >= num_ch) {
		LOG_ERR("channel %u out of range", channel);
		return -EINVAL;
	}

	switch (dev_data->channels[channel].state) {
	case XDMAC_CH_STATE_RUNNING:
		break;
	case XDMAC_CH_STATE_SUSPENDED:
		return 0;
	default:
		LOG_ERR("cannot suspend channel %u – not running", channel);
		return -EINVAL;
	}

	if (!xdmac_ch_is_enabled(base, channel)) {
		LOG_DBG("channel %u not enabled", channel);
		return -EINVAL;
	}

	if ((mchp_xdmac_glob_read(base, XDMAC_GRSS_REG_OFST) & BIT(channel)) ||
	    (mchp_xdmac_glob_read(base, XDMAC_GWSS_REG_OFST) & BIT(channel))) {
		LOG_DBG("channel %u already suspended", channel);
		return 0;
	}

	/* Assert read+write suspend */
	mchp_xdmac_glob_write(base, XDMAC_GRWS_REG_OFST, BIT(channel));

	dev_data->channels[channel].state = XDMAC_CH_STATE_SUSPENDED;

	return 0;
}

/* DMA API – resume */
static int mchp_xdmac_resume(const struct device *dev, uint32_t channel)
{
	struct mchp_xdmac_data *dev_data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t num_ch = dev_data->dma_ctx.dma_channels;

	if (channel >= num_ch) {
		LOG_ERR("channel %u out of range", channel);
		return -EINVAL;
	}

	switch (dev_data->channels[channel].state) {
	case XDMAC_CH_STATE_SUSPENDED:
		break;
	case XDMAC_CH_STATE_RUNNING:
		return 0;
	default:
		LOG_ERR("resume channel %u at invalidate state", channel);
		return -EINVAL;
	}

	if (!xdmac_ch_is_enabled(base, channel)) {
		LOG_DBG("channel %u not enabled", channel);
		return -EINVAL;
	}

	if (!(mchp_xdmac_glob_read(base, XDMAC_GRSS_REG_OFST) & BIT(channel)) &&
	    !(mchp_xdmac_glob_read(base, XDMAC_GWSS_REG_OFST) & BIT(channel))) {
		LOG_DBG("channel %u not actually suspended", channel);
		return 0;
	}

	/* De-assert suspend: write to the resume register */
	mchp_xdmac_glob_write(base, XDMAC_GRWR_REG_OFST, BIT(channel));

	dev_data->channels[channel].state = XDMAC_CH_STATE_RUNNING;

	return 0;
}

/* DMA API – get_status */
static int mchp_xdmac_get_status(const struct device *dev, uint32_t channel,
				 struct dma_status *status)
{
	struct mchp_xdmac_data *dev_data = dev->data;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t num_ch = dev_data->dma_ctx.dma_channels;
	uint32_t cc;
	uint32_t ublen;

	if (channel >= num_ch) {
		LOG_ERR("channel %u out of range", channel);
		return -EINVAL;
	}

	cc = mchp_xdmac_ch_read(base, channel, XDMAC_CC_REG_OFST);
	ublen = FIELD_GET(XDMAC_CUBC_UBLEN_Msk,
			  mchp_xdmac_ch_read(base, channel, XDMAC_CUBC_REG_OFST));

	/* Determine transfer direction from CC.TYPE and CC.DSYNC */
	if (FIELD_GET(XDMAC_CC_TYPE_Msk, cc) == XDMAC_CC_TYPE_MEM_TRAN_Val) {
		status->dir = MEMORY_TO_MEMORY;
	} else {
		if (FIELD_GET(XDMAC_CC_DSYNC_Msk, cc) == XDMAC_CC_DSYNC_MEM2PER_Val) {
			status->dir = MEMORY_TO_PERIPHERAL;
		} else {
			status->dir = PERIPHERAL_TO_MEMORY;
		}
	}

	status->busy = ((cc & XDMAC_CC_INITD_Msk) != 0U) || (ublen > 0U);
	status->pending_length = ublen;

	return 0;
}

static int mchp_xdmac_init(const struct device *dev)
{
	const struct mchp_xdmac_cfg *dev_cfg = dev->config;
	struct mchp_xdmac_data *dev_data = dev->data;
	mm_reg_t base;
	uint32_t gtype;
	uint32_t nb_ch;

	/* Enable the XDMAC peripheral clock in the PMC */
	(void)clock_control_on(DEVICE_DT_GET(DT_NODELABEL(pmc)),
			       (clock_control_subsys_t)&dev_cfg->clock_cfg);

	/* Map the MMIO region into the virtual address space */
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	base = DEVICE_MMIO_GET(dev);

	/* Discover actual channel count from XDMAC_GTYPE.NB_CH (value is N-1) */
	gtype = mchp_xdmac_glob_read(base, XDMAC_GTYPE_REG_OFST);
	nb_ch = FIELD_GET(XDMAC_GTYPE_NB_CH_Msk, gtype) + 1U;

	if (nb_ch > XDMAC_CHANNELS_MAX) {
		LOG_ERR("hardware reports %u channels; driver supports max %u", nb_ch,
			XDMAC_CHANNELS_MAX);
		return -EINVAL;
	}

	dev_data->dma_ctx.dma_channels = nb_ch;

	/* Disable all channels and their global interrupts */
	mchp_xdmac_glob_write(base, XDMAC_GD_REG_OFST, XDMAC_GD_DI_Msk);
	mchp_xdmac_glob_write(base, XDMAC_GID_REG_OFST, XDMAC_GID_ID_Msk);

	/* Configure and enable the IRQ line */
	dev_cfg->irq_config();
	irq_enable(dev_cfg->irq_id);

	LOG_INF("%s: %u channels, base 0x%08lx", dev->name, nb_ch, (unsigned long)base);

	return 0;
}

static DEVICE_API(dma, mchp_xdmac_driver_api) = {
	.config = mchp_xdmac_config,
	.reload = mchp_xdmac_reload,
	.start = mchp_xdmac_start,
	.stop = mchp_xdmac_stop,
	.suspend = mchp_xdmac_suspend,
	.resume = mchp_xdmac_resume,
	.get_status = mchp_xdmac_get_status,
};

#define MCHP_XDMAC_INIT(n)								\
	static void mchp_xdmac_irq_config_##n(void)					\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mchp_xdmac_isr,	\
			    DEVICE_DT_INST_GET(n), DT_INST_IRQ(n, flags));		\
	}										\
											\
	static const struct mchp_xdmac_cfg mchp_xdmac_cfg_##n = {			\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),					\
		.irq_config = mchp_xdmac_irq_config_##n,				\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),				\
		.irq_id = DT_INST_IRQN(n),						\
	};										\
											\
	static ATOMIC_DEFINE(mchp_xdmac_atomic_##n, XDMAC_CHANNELS_MAX);		\
											\
	static struct mchp_xdmac_data mchp_xdmac_data_##n = {				\
		.dma_ctx = {								\
			.magic  = DMA_MAGIC,						\
			.atomic = mchp_xdmac_atomic_##n,				\
		},									\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n,							\
			      &mchp_xdmac_init,						\
			      NULL,							\
			      &mchp_xdmac_data_##n,					\
			      &mchp_xdmac_cfg_##n,					\
			      POST_KERNEL,						\
			      CONFIG_DMA_INIT_PRIORITY,					\
			      &mchp_xdmac_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCHP_XDMAC_INIT)
