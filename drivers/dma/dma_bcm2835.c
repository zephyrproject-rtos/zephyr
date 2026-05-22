/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Broadcom BCM2835 / BCM2710 / BCM2837 DMA controller.
 *
 * The BCM283x DMA engine is a control-block (linked-list descriptor)
 * controller: each channel has a register block at base + ch*0x100,
 * and a transfer is driven by a 32-byte Control Block (CB) in memory
 * that the hardware reads and walks. This driver builds one CB per
 * channel and runs single-block transfers (block_count == 1).
 *
 * Completion is IRQ-driven; the SoC interrupt cascade
 * (soc/brcm/bcm2710/soc_irq.c) is already up, and each channel has a
 * line in the ARMC PEND1 bank.
 *
 * Three silicon facts shape this driver:
 *
 *   - VideoCore bus addresses. The CONBLK_AD / SOURCE_AD / DEST_AD
 *     registers take *bus* addresses, not ARM-physical ones. SDRAM
 *     appears on the VideoCore bus at the 0xC0000000 alias (VC L2
 *     disabled -- the correct alias when the ARM manages its own
 *     caches). See dma_bcm2835_bus_addr(); this is the first thing to
 *     re-check if a transfer silently moves the wrong data.
 *
 *   - No cache coherency. The DMA engine does not snoop the A53 data
 *     caches. The driver cleans source buffers and the CB to DRAM
 *     before a transfer and invalidates destination buffers after,
 *     via the sys_cache_* API. DMA buffers must be cache-line aligned
 *     (dma-buf-addr-alignment = <64> in DT) so a partial-line
 *     invalidate cannot discard a neighbour's data.
 *
 *   - Reserved channels. The VideoCore firmware uses some DMA
 *     channels and a few have special functionality;
 *     brcm,dma-channel-mask in DT marks the channels safe for this
 *     driver, and chan_filter() enforces it.
 *
 * Scope: MEMORY_TO_MEMORY is wired and exercised by tests/drivers/dma.
 * The MEMORY_TO_PERIPHERAL / PERIPHERAL_TO_MEMORY DREQ paths are wired
 * (PER_MAP from dma_slot, plus the S/D_DREQ bits) but are not yet
 * covered by an in-tree consumer. 2D mode, chained CBs, and the
 * careful mid-transfer paused-abort dance Linux performs are not
 * implemented.
 *
 * Reference: Linux drivers/dma/bcm2835-dma.c; BCM2835 ARM Peripherals
 * datasheet ch. 4 (DMA Controller).
 */

#define DT_DRV_COMPAT brcm_bcm2835_dma

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/cache.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(dma_bcm2835, CONFIG_DMA_LOG_LEVEL);

/* Per-channel register block: channel N lives at base + N * 0x100. */
#define DMA_CHAN_STRIDE 0x100U
#define DMA_CS          0x00U /* control and status */
#define DMA_CONBLK_AD   0x04U /* control block address */
#define DMA_TI          0x08U /* transfer information (shadow of CB.info) */
#define DMA_SOURCE_AD   0x0cU
#define DMA_DEST_AD     0x10U
#define DMA_TXFR_LEN    0x14U
#define DMA_STRIDE      0x18U
#define DMA_NEXTCONBK   0x1cU
#define DMA_DEBUG       0x20U

/* CS (control / status) bits. */
#define CS_ACTIVE BIT(0)
#define CS_END    BIT(1)  /* W1C */
#define CS_INT    BIT(2)  /* W1C */
#define CS_ERROR  BIT(8)
#define CS_ABORT  BIT(30) /* WO */
#define CS_RESET  BIT(31) /* WO, self-clearing */

/* TI (transfer information) bits -- also the CB.info field. */
#define TI_INT_EN    BIT(0)
#define TI_WAIT_RESP BIT(3)
#define TI_DEST_INC  BIT(4)
#define TI_DEST_DREQ BIT(6)
#define TI_SRC_INC   BIT(8)
#define TI_SRC_DREQ  BIT(10)
#define TI_PERMAP(x) (((x) & 0x1fU) << 16)

/* The DMA engine is a VideoCore-bus master and does not see ARM-physical
 * addresses directly. Two ranges matter:
 *
 *   - SDRAM appears at the 0xC0000000 alias (VC L2 disabled -- the
 *     correct alias when the ARM manages its own caches). ARM memory is
 *     identity-mapped (VA == PA) on this SoC, so OR-ing the alias onto
 *     an ARM address yields the bus address.
 *
 *   - The peripheral block lives at ARM-physical 0x3F000000..0x3FFFFFFF
 *     but is reached by the DMA master at the 0x7E000000 VideoCore
 *     peripheral window. A peripheral DREQ transfer (an I2S / SPI FIFO
 *     <-> memory copy) hands the driver an ARM peripheral address that
 *     must be re-based here -- OR-ing the SDRAM alias onto it would
 *     point the engine at bogus SDRAM.
 *
 * Confirmed against soc/brcm/bcm2710/mmu_regions.c and the BCM2835 ARM
 * Peripherals datasheet ch. 1.2.1 (ARM physical / VC bus address map).
 */
#define DMA_BUS_ALIAS   0xC0000000U
#define DMA_PERIPH_ARM  0x3F000000U
#define DMA_PERIPH_BUS  0x7E000000U
#define DMA_PERIPH_SIZE 0x01000000U

static inline uint32_t dma_bcm2835_bus_addr(uintptr_t arm_addr)
{
	if (arm_addr >= DMA_PERIPH_ARM &&
	    arm_addr < DMA_PERIPH_ARM + DMA_PERIPH_SIZE) {
		return (uint32_t)(arm_addr - DMA_PERIPH_ARM) | DMA_PERIPH_BUS;
	}

	return (uint32_t)arm_addr | DMA_BUS_ALIAS;
}

/* 32-byte hardware control block. The engine reads this from DRAM, so
 * it must be 32-byte aligned and cleaned to memory before use.
 */
struct bcm2835_dma_cb {
	uint32_t info;   /* TI */
	uint32_t src;    /* SOURCE_AD -- bus address */
	uint32_t dst;    /* DEST_AD -- bus address */
	uint32_t length; /* TXFR_LEN */
	uint32_t stride; /* STRIDE -- 2D mode only, 0 here */
	uint32_t next;   /* NEXTCONBK -- 0, single block */
	uint32_t pad[2];
};

struct dma_bcm2835_channel {
	dma_callback_t callback;
	void *user_data;
	uint32_t direction;
	/* ARM-side addresses + length, kept for cache maintenance: the
	 * CB itself holds bus addresses, which sys_cache_* cannot use.
	 */
	uintptr_t src;
	uintptr_t dst;
	size_t len;
	bool busy;
};

struct dma_bcm2835_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	uint32_t channel_mask;
	uint32_t num_channels;
	void (*irq_config)(void);
};

struct dma_bcm2835_data {
	struct dma_context ctx; /* must be first -- see dma_request_channel() */
	DEVICE_MMIO_NAMED_RAM(reg_base);
	struct dma_bcm2835_channel *channels;
	struct bcm2835_dma_cb *cbs;
};

/* The plain DEVICE_MMIO_RAM macro stores the mapped register base at
 * offset 0 of dev->data -- but that is struct dma_context, whose magic
 * field dma_request_channel() checks. The NAMED variant puts the mapped
 * base in its own member so dma_context stays first; it resolves that
 * member through DEV_DATA / DEV_CFG.
 */
#define DEV_CFG(dev)  ((const struct dma_bcm2835_config *)(dev)->config)
#define DEV_DATA(dev) ((struct dma_bcm2835_data *)(dev)->data)

static inline uint32_t dma_rd(const struct device *dev, uint32_t off)
{
	return sys_read32(DEVICE_MMIO_NAMED_GET(dev, reg_base) + off);
}

static inline void dma_wr(const struct device *dev, uint32_t off, uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_NAMED_GET(dev, reg_base) + off);
}

static inline uint32_t chan_off(uint32_t channel, uint32_t reg)
{
	return channel * DMA_CHAN_STRIDE + reg;
}

static inline bool chan_valid(const struct dma_bcm2835_config *cfg, uint32_t channel)
{
	return channel < cfg->num_channels && (cfg->channel_mask & BIT(channel)) != 0U;
}

static int dma_bcm2835_config(const struct device *dev, uint32_t channel,
			      struct dma_config *cfg)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_block_config *blk = cfg->head_block;
	struct dma_bcm2835_channel *chan;
	struct bcm2835_dma_cb *cb;
	uint32_t ti;

	if (!chan_valid(dcfg, channel)) {
		LOG_ERR("channel %u unavailable (mask 0x%x)", channel, dcfg->channel_mask);
		return -EINVAL;
	}

	if (cfg->block_count != 1U || blk == NULL) {
		LOG_ERR("only single-block transfers are supported");
		return -ENOTSUP;
	}

	if (blk->source_addr_adj == DMA_ADDR_ADJ_DECREMENT ||
	    blk->dest_addr_adj == DMA_ADDR_ADJ_DECREMENT) {
		LOG_ERR("address decrement not supported");
		return -ENOTSUP;
	}

	ti = TI_INT_EN | TI_WAIT_RESP;
	if (blk->source_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
		ti |= TI_SRC_INC;
	}
	if (blk->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
		ti |= TI_DEST_INC;
	}

	switch (cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
		break;
	case MEMORY_TO_PERIPHERAL:
		ti |= TI_DEST_DREQ | TI_PERMAP(cfg->dma_slot);
		break;
	case PERIPHERAL_TO_MEMORY:
		ti |= TI_SRC_DREQ | TI_PERMAP(cfg->dma_slot);
		break;
	default:
		LOG_ERR("channel_direction %u not supported", cfg->channel_direction);
		return -ENOTSUP;
	}

	cb = &data->cbs[channel];
	cb->info = ti;
	cb->src = dma_bcm2835_bus_addr((uintptr_t)blk->source_address);
	cb->dst = dma_bcm2835_bus_addr((uintptr_t)blk->dest_address);
	cb->length = blk->block_size;
	cb->stride = 0U;
	cb->next = 0U;
	cb->pad[0] = 0U;
	cb->pad[1] = 0U;

	chan = &data->channels[channel];
	chan->callback = cfg->dma_callback;
	chan->user_data = cfg->user_data;
	chan->direction = cfg->channel_direction;
	chan->src = (uintptr_t)blk->source_address;
	chan->dst = (uintptr_t)blk->dest_address;
	chan->len = blk->block_size;
	chan->busy = false;

	return 0;
}

static int dma_bcm2835_reload(const struct device *dev, uint32_t channel,
			      uint32_t src, uint32_t dst, size_t size)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan;
	struct bcm2835_dma_cb *cb;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	if (dma_rd(dev, chan_off(channel, DMA_CS)) & CS_ACTIVE) {
		return -EBUSY;
	}

	cb = &data->cbs[channel];
	cb->src = dma_bcm2835_bus_addr(src);
	cb->dst = dma_bcm2835_bus_addr(dst);
	cb->length = size;

	chan = &data->channels[channel];
	chan->src = src;
	chan->dst = dst;
	chan->len = size;

	return 0;
}

static int dma_bcm2835_start(const struct device *dev, uint32_t channel)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan;
	struct bcm2835_dma_cb *cb;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	chan = &data->channels[channel];
	cb = &data->cbs[channel];

	/* The DMA engine does not snoop the A53 caches. Clean source
	 * data to DRAM; clean+invalidate the destination so no dirty
	 * line later writes back over the DMA result, and so the CPU
	 * re-reads fresh after completion.
	 */
	if (chan->direction == MEMORY_TO_MEMORY ||
	    chan->direction == MEMORY_TO_PERIPHERAL) {
		sys_cache_data_flush_range((void *)chan->src, chan->len);
	}
	if (chan->direction == MEMORY_TO_MEMORY ||
	    chan->direction == PERIPHERAL_TO_MEMORY) {
		sys_cache_data_flush_and_invd_range((void *)chan->dst, chan->len);
	}
	/* The CB is itself read from DRAM by the engine. */
	sys_cache_data_flush_range(cb, sizeof(*cb));

	/* Clear stale status, point the channel at the CB, then go. */
	dma_wr(dev, chan_off(channel, DMA_CS), CS_INT | CS_END);
	dma_wr(dev, chan_off(channel, DMA_CONBLK_AD),
	       dma_bcm2835_bus_addr((uintptr_t)cb));
	chan->busy = true;
	dma_wr(dev, chan_off(channel, DMA_CS), CS_ACTIVE);

	return 0;
}

static int dma_bcm2835_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	/* RESET is write-only and self-clearing; it returns the channel
	 * to a known idle state. Linux performs a more careful
	 * paused-abort sequence for DREQ-stalled transfers -- not needed
	 * for the cases this driver handles yet.
	 */
	dma_wr(dev, chan_off(channel, DMA_CS), CS_RESET);
	data->channels[channel].busy = false;

	return 0;
}

static int dma_bcm2835_get_status(const struct device *dev, uint32_t channel,
				  struct dma_status *stat)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	uint32_t cs;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	cs = dma_rd(dev, chan_off(channel, DMA_CS));
	stat->busy = (cs & CS_ACTIVE) != 0U;
	stat->dir = data->channels[channel].direction;
	stat->pending_length = dma_rd(dev, chan_off(channel, DMA_TXFR_LEN));

	return 0;
}

static bool dma_bcm2835_chan_filter(const struct device *dev, int channel,
				    void *filter_param)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	bool ok;

	if (channel < 0 || (uint32_t)channel >= dcfg->num_channels) {
		return false;
	}

	ok = (dcfg->channel_mask & BIT(channel)) != 0U;
	if (filter_param != NULL) {
		ok = ok && ((*(uint32_t *)filter_param & BIT(channel)) != 0U);
	}

	return ok;
}

static void dma_bcm2835_isr(const struct device *dev)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;

	for (uint32_t ch = 0; ch < dcfg->num_channels; ch++) {
		struct dma_bcm2835_channel *chan;
		uint32_t cs;
		int status;

		if (!(dcfg->channel_mask & BIT(ch))) {
			continue;
		}

		cs = dma_rd(dev, chan_off(ch, DMA_CS));
		if (!(cs & CS_INT)) {
			continue;
		}

		/* Ack: INT and END are write-1-to-clear. */
		dma_wr(dev, chan_off(ch, DMA_CS), CS_INT | CS_END);

		chan = &data->channels[ch];
		status = (cs & CS_ERROR) ? -EIO : DMA_STATUS_COMPLETE;

		/* The destination is now fresh in DRAM; drop any line the
		 * CPU may have speculatively pulled back since start().
		 */
		if (status == DMA_STATUS_COMPLETE &&
		    (chan->direction == MEMORY_TO_MEMORY ||
		     chan->direction == PERIPHERAL_TO_MEMORY)) {
			sys_cache_data_invd_range((void *)chan->dst, chan->len);
		}

		chan->busy = false;

		if (chan->callback != NULL) {
			chan->callback(dev, chan->user_data, ch, status);
		}
	}
}

static int dma_bcm2835_init(const struct device *dev)
{
	const struct dma_bcm2835_config *dcfg = dev->config;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);

	/* Park every channel this driver may use in a known idle state. */
	for (uint32_t ch = 0; ch < dcfg->num_channels; ch++) {
		if (dcfg->channel_mask & BIT(ch)) {
			dma_wr(dev, chan_off(ch, DMA_CS), CS_RESET);
		}
	}

	dcfg->irq_config();

	return 0;
}

static DEVICE_API(dma, dma_bcm2835_driver_api) = {
	.config = dma_bcm2835_config,
	.reload = dma_bcm2835_reload,
	.start = dma_bcm2835_start,
	.stop = dma_bcm2835_stop,
	.get_status = dma_bcm2835_get_status,
	.chan_filter = dma_bcm2835_chan_filter,
};

#define IRQ_CONNECT_DMA(idx, n)                                                \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq),                           \
		    DT_INST_IRQ_BY_IDX(n, idx, priority), dma_bcm2835_isr,      \
		    DEVICE_DT_INST_GET(n), 0);                                 \
	irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));

#define DMA_BCM2835_INIT(n)                                                    \
	static void dma_bcm2835_irq_config_##n(void)                           \
	{                                                                      \
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), IRQ_CONNECT_DMA, (), n)    \
	}                                                                      \
                                                                               \
	static struct dma_bcm2835_channel                                      \
		dma_bcm2835_channels_##n[DT_INST_PROP(n, dma_channels)];        \
	static struct bcm2835_dma_cb                                           \
		dma_bcm2835_cbs_##n[DT_INST_PROP(n, dma_channels)] __aligned(32); \
	ATOMIC_DEFINE(dma_bcm2835_atomic_##n, DT_INST_PROP(n, dma_channels));   \
                                                                               \
	static struct dma_bcm2835_data dma_bcm2835_data_##n = {                \
		.ctx = {                                                       \
			.magic = DMA_MAGIC,                                    \
			.atomic = dma_bcm2835_atomic_##n,                      \
			.dma_channels = DT_INST_PROP(n, dma_channels),         \
		},                                                             \
		.channels = dma_bcm2835_channels_##n,                          \
		.cbs = dma_bcm2835_cbs_##n,                                    \
	};                                                                     \
                                                                               \
	static const struct dma_bcm2835_config dma_bcm2835_config_##n = {       \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),          \
		.channel_mask = DT_INST_PROP(n, brcm_dma_channel_mask),        \
		.num_channels = DT_INST_PROP(n, dma_channels),                 \
		.irq_config = dma_bcm2835_irq_config_##n,                      \
	};                                                                     \
                                                                               \
	DEVICE_DT_INST_DEFINE(n, dma_bcm2835_init, NULL, &dma_bcm2835_data_##n, \
			      &dma_bcm2835_config_##n, POST_KERNEL,            \
			      CONFIG_DMA_INIT_PRIORITY, &dma_bcm2835_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_BCM2835_INIT)
