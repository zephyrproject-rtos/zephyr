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
 * that the hardware reads and walks. Each CB points to the next via
 * its `next` field; the engine stops when it loads `next = 0`. This
 * driver supports two shapes:
 *
 *   - Single-block (block_count == 1) transfers. One CB; runs once,
 *     stops, fires a completion callback. The simplest path; what
 *     tests/drivers/dma exercises.
 *   - Cyclic chains (block_count >= 1, dma_config.cyclic == 1). N CBs
 *     are built from the caller's dma_block_config list and chained
 *     into a ring (last CB's next loops back to the first). Used by
 *     audio-class drivers (e.g. I2S) where DMA must run continuously
 *     without re-arm latency between blocks. INT_EN is set on every
 *     CB, so the completion callback fires per block.
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
 * Reference: Linux drivers/dma/bcm2835-dma.c (which uses a cyclic
 * descriptor chain via dmaengine_pcm for ALSA); BCM2835 ARM
 * Peripherals datasheet ch. 4 (DMA Controller).
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

/* Maximum CBs per channel. Bounds the cyclic ring depth; single-block
 * transfers use 1. Configurable so audio paths needing deeper rings
 * (more headroom against jitter) can grow it.
 */
#define MAX_CBS_PER_CHAN CONFIG_DMA_BCM2835_MAX_CBS_PER_CHANNEL

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
	uint32_t next;   /* NEXTCONBK -- next CB's bus address, 0 to stop */
	uint32_t pad[2];
};

/* ARM-side address + length kept per CB for cache maintenance: the CB
 * itself holds bus addresses, which sys_cache_* cannot use.
 */
struct dma_bcm2835_cb_info {
	uintptr_t src;
	uintptr_t dst;
	size_t len;
};

struct dma_bcm2835_channel {
	dma_callback_t callback;
	void *user_data;
	uint32_t direction;
	uint32_t n_blocks;  /* 1..MAX_CBS_PER_CHAN */
	uint32_t cur_cb;    /* software cursor: next CB to ack */
	bool cyclic;
	bool busy;
	struct dma_bcm2835_cb_info info[MAX_CBS_PER_CHAN];
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
	struct bcm2835_dma_cb (*cbs)[MAX_CBS_PER_CHAN];
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

/* Build the TI (transfer information) word for one block. */
static uint32_t dma_bcm2835_build_ti(uint32_t direction, uint32_t dma_slot,
				     uint8_t src_adj, uint8_t dst_adj)
{
	uint32_t ti = TI_INT_EN | TI_WAIT_RESP;

	if (src_adj == DMA_ADDR_ADJ_INCREMENT) {
		ti |= TI_SRC_INC;
	}
	if (dst_adj == DMA_ADDR_ADJ_INCREMENT) {
		ti |= TI_DEST_INC;
	}

	switch (direction) {
	case MEMORY_TO_PERIPHERAL:
		ti |= TI_DEST_DREQ | TI_PERMAP(dma_slot);
		break;
	case PERIPHERAL_TO_MEMORY:
		ti |= TI_SRC_DREQ | TI_PERMAP(dma_slot);
		break;
	default:
		break;
	}

	return ti;
}

static int dma_bcm2835_config(const struct device *dev, uint32_t channel,
			      struct dma_config *cfg)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_block_config *blk;
	struct dma_bcm2835_channel *chan;
	struct bcm2835_dma_cb *cbs;
	uint32_t ti, n;

	if (!chan_valid(dcfg, channel)) {
		LOG_ERR("channel %u unavailable (mask 0x%x)", channel, dcfg->channel_mask);
		return -EINVAL;
	}

	if (cfg->block_count == 0U || cfg->block_count > MAX_CBS_PER_CHAN ||
	    cfg->head_block == NULL) {
		LOG_ERR("block_count %u out of range [1..%u]",
			cfg->block_count, MAX_CBS_PER_CHAN);
		return -ENOTSUP;
	}

	switch (cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
	case MEMORY_TO_PERIPHERAL:
	case PERIPHERAL_TO_MEMORY:
		break;
	default:
		LOG_ERR("channel_direction %u not supported", cfg->channel_direction);
		return -ENOTSUP;
	}

	chan = &data->channels[channel];
	cbs = data->cbs[channel];

	blk = cfg->head_block;
	for (n = 0; n < cfg->block_count; n++) {
		if (blk == NULL) {
			LOG_ERR("head_block chain shorter than block_count");
			return -EINVAL;
		}
		if (blk->source_addr_adj == DMA_ADDR_ADJ_DECREMENT ||
		    blk->dest_addr_adj == DMA_ADDR_ADJ_DECREMENT) {
			LOG_ERR("address decrement not supported");
			return -ENOTSUP;
		}

		ti = dma_bcm2835_build_ti(cfg->channel_direction, cfg->dma_slot,
					  blk->source_addr_adj,
					  blk->dest_addr_adj);

		cbs[n].info = ti;
		cbs[n].src = dma_bcm2835_bus_addr((uintptr_t)blk->source_address);
		cbs[n].dst = dma_bcm2835_bus_addr((uintptr_t)blk->dest_address);
		cbs[n].length = blk->block_size;
		cbs[n].stride = 0U;
		cbs[n].next = 0U; /* set after the loop */
		cbs[n].pad[0] = 0U;
		cbs[n].pad[1] = 0U;

		chan->info[n].src = (uintptr_t)blk->source_address;
		chan->info[n].dst = (uintptr_t)blk->dest_address;
		chan->info[n].len = blk->block_size;

		blk = blk->next_block;
	}

	/* Chain: each CB's next points to the next slot. The last CB
	 * either loops back (cyclic) or terminates (next == 0).
	 */
	for (n = 0; n < cfg->block_count - 1U; n++) {
		cbs[n].next = dma_bcm2835_bus_addr((uintptr_t)&cbs[n + 1]);
	}
	if (cfg->cyclic) {
		cbs[cfg->block_count - 1U].next =
			dma_bcm2835_bus_addr((uintptr_t)&cbs[0]);
	} else {
		cbs[cfg->block_count - 1U].next = 0U;
	}

	chan->callback = cfg->dma_callback;
	chan->user_data = cfg->user_data;
	chan->direction = cfg->channel_direction;
	chan->n_blocks = cfg->block_count;
	chan->cur_cb = 0U;
	chan->cyclic = cfg->cyclic != 0U;
	chan->busy = false;

	return 0;
}

/* Reload updates the first CB's src / dst / length in place. Intended
 * for non-cyclic single-block re-arming between transfers; modifying a
 * CB the engine is actively walking is a race.
 */
static int dma_bcm2835_reload(const struct device *dev, uint32_t channel,
			      uint32_t src, uint32_t dst, size_t size)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan;
	struct bcm2835_dma_cb *cbs;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	if (dma_rd(dev, chan_off(channel, DMA_CS)) & CS_ACTIVE) {
		return -EBUSY;
	}

	chan = &data->channels[channel];
	cbs = data->cbs[channel];

	cbs[0].src = dma_bcm2835_bus_addr(src);
	cbs[0].dst = dma_bcm2835_bus_addr(dst);
	cbs[0].length = size;

	chan->info[0].src = src;
	chan->info[0].dst = dst;
	chan->info[0].len = size;
	chan->cur_cb = 0U;

	return 0;
}

/* Cache-flush a CB's source range (memory-side) before DMA reads it. */
static void dma_bcm2835_flush_src(uint32_t direction,
				  const struct dma_bcm2835_cb_info *info)
{
	if (direction == MEMORY_TO_MEMORY ||
	    direction == MEMORY_TO_PERIPHERAL) {
		sys_cache_data_flush_range((void *)info->src, info->len);
	}
}

/* Cache-flush+invalidate a CB's destination range before DMA writes
 * it, so no dirty CPU line later writes back over the DMA result.
 */
static void dma_bcm2835_prep_dst(uint32_t direction,
				 const struct dma_bcm2835_cb_info *info)
{
	if (direction == MEMORY_TO_MEMORY ||
	    direction == PERIPHERAL_TO_MEMORY) {
		sys_cache_data_flush_and_invd_range((void *)info->dst, info->len);
	}
}

/* Invalidate a CB's destination range after DMA completes, so the CPU
 * re-reads fresh.
 */
static void dma_bcm2835_invd_dst(uint32_t direction,
				 const struct dma_bcm2835_cb_info *info)
{
	if (direction == MEMORY_TO_MEMORY ||
	    direction == PERIPHERAL_TO_MEMORY) {
		sys_cache_data_invd_range((void *)info->dst, info->len);
	}
}

static int dma_bcm2835_start(const struct device *dev, uint32_t channel)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan;
	struct bcm2835_dma_cb *cbs;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	chan = &data->channels[channel];
	cbs = data->cbs[channel];

	/* The DMA engine does not snoop the A53 caches. Prime every
	 * source and destination block in the chain, then flush the CBs
	 * themselves -- the engine reads them from DRAM too.
	 */
	for (uint32_t i = 0; i < chan->n_blocks; i++) {
		dma_bcm2835_flush_src(chan->direction, &chan->info[i]);
		dma_bcm2835_prep_dst(chan->direction, &chan->info[i]);
	}
	sys_cache_data_flush_range(cbs, chan->n_blocks * sizeof(*cbs));

	/* Clear stale status, point the channel at the first CB, then go. */
	dma_wr(dev, chan_off(channel, DMA_CS), CS_INT | CS_END);
	dma_wr(dev, chan_off(channel, DMA_CONBLK_AD),
	       dma_bcm2835_bus_addr((uintptr_t)&cbs[0]));
	chan->cur_cb = 0U;
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

/* CS_RESET zeros the channel state machine, including the engine's
 * internal mid-CB byte counter. Without RESET, clearing CS_ACTIVE
 * alone pauses the channel but leaves the engine partway through the
 * current CB; resuming continues those leftover bytes rather than
 * starting the CB cleanly, which leaves a stale "skipped" region in
 * the destination if the CPU touched it during suspend. RESET loses
 * mid-block progress -- the correct semantics for a "pause / play"
 * style streaming consumer.
 *
 * The CB chain itself lives in DRAM and is unaffected; resume then
 * re-points CONBLK_AD at cb[0] and sets ACTIVE.
 */
static int dma_bcm2835_suspend(const struct device *dev, uint32_t channel)
{
	const struct dma_bcm2835_config *dcfg = dev->config;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	dma_wr(dev, chan_off(channel, DMA_CS), CS_RESET);

	return 0;
}

/* On resume:
 *   - Rewind the channel to cb[0]. Cyclic suspend pauses the engine
 *     mid-block; without rewinding, the partial block before the pause
 *     point is never overwritten on the next pass and shows up as
 *     stale data. Rewinding loses mid-block progress, which matches
 *     standard "pause/play" semantics for streaming.
 *   - Re-prep destinations. The CPU may have touched destination
 *     buffers while paused (reading a captured block, or a test
 *     memset between iterations); dirty CPU lines would later evict
 *     and clobber the DMA-written data in DRAM.
 */
static int dma_bcm2835_resume(const struct device *dev, uint32_t channel)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan;
	struct bcm2835_dma_cb *cbs;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	chan = &data->channels[channel];
	cbs = data->cbs[channel];

	for (uint32_t i = 0; i < chan->n_blocks; i++) {
		dma_bcm2835_prep_dst(chan->direction, &chan->info[i]);
	}

	dma_wr(dev, chan_off(channel, DMA_CONBLK_AD),
	       dma_bcm2835_bus_addr((uintptr_t)&cbs[0]));
	chan->cur_cb = 0U;

	dma_wr(dev, chan_off(channel, DMA_CS), CS_ACTIVE);

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
		uint32_t cb_idx;

		if (!(dcfg->channel_mask & BIT(ch))) {
			continue;
		}

		cs = dma_rd(dev, chan_off(ch, DMA_CS));
		if (!(cs & CS_INT)) {
			continue;
		}

		/* Ack: INT and END are W1C. Read CS back first and OR in the
		 * two ack bits so the other bits keep their value -- in
		 * particular ACTIVE (bit 0) is R/W, and writing 0 to it
		 * pauses the DMA. A bare W1C of just INT|END would silently
		 * halt a cyclic chain after the first CB completion.
		 */
		dma_wr(dev, chan_off(ch, DMA_CS), cs | CS_INT | CS_END);

		chan = &data->channels[ch];
		status = (cs & CS_ERROR) ? -EIO : DMA_STATUS_COMPLETE;

		/* The CB that just signalled completion is the one at the
		 * current cursor; advance the cursor for the next CB.
		 */
		cb_idx = chan->cur_cb;
		if (cb_idx >= chan->n_blocks) {
			cb_idx = 0U;
		}

		if (status == DMA_STATUS_COMPLETE) {
			dma_bcm2835_invd_dst(chan->direction,
					     &chan->info[cb_idx]);
		}

		if (chan->cyclic) {
			chan->cur_cb = (cb_idx + 1U) % chan->n_blocks;
		} else if (cb_idx + 1U >= chan->n_blocks ||
			   status != DMA_STATUS_COMPLETE) {
			/* Last CB done or fault: chain has stopped. */
			chan->busy = false;
			chan->cur_cb = 0U;
		} else {
			chan->cur_cb = cb_idx + 1U;
		}

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
	.suspend = dma_bcm2835_suspend,
	.resume = dma_bcm2835_resume,
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
		dma_bcm2835_cbs_##n[DT_INST_PROP(n, dma_channels)]              \
				   [MAX_CBS_PER_CHAN] __aligned(32);            \
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
