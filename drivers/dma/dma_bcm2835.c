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
 * its `next` field; the engine stops when it loads `next = 0`.
 *
 * Each block of the caller's dma_block_config list becomes one CB. In
 * cyclic mode the last CB loops back to the first, so the ring runs
 * continuously without re-arm latency between blocks, which
 * audio-class drivers (e.g. I2S) need.
 *
 * Completion is IRQ-driven. A channel has a single INT flag, so blocks
 * that finish before the handler runs raise one interrupt between
 * them. The handler therefore takes the engine's position from
 * CONBLK_AD and reports every block finished since it last ran:
 * DMA_STATUS_BLOCK for intermediate blocks when complete_callback_en
 * asks for them, DMA_STATUS_COMPLETE for the last block of the list
 * (in cyclic mode, of each pass round the ring). Without per-block
 * callbacks only the last CB raises an interrupt.
 *
 * Three silicon facts shape this driver:
 *
 *   - VideoCore bus addresses. The CONBLK_AD / SOURCE_AD / DEST_AD
 *     registers take *bus* addresses, not ARM-physical ones, and SDRAM
 *     appears on the VideoCore bus at a per-SoC alias. See
 *     dma_bcm2835_bus_addr(); this is the first thing to re-check if a
 *     transfer silently moves the wrong data.
 *
 *   - No cache coherency. The DMA engine does not snoop the A53 data
 *     caches. The driver cleans its control blocks to memory before
 *     starting a channel; cleaning and invalidating the source and
 *     destination buffers is left to the caller, as for any
 *     non-coherent DMA master.
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
#include <zephyr/spinlock.h>
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
 *   - SDRAM appears at a VC-bus alias that differs by SoC, taken from
 *     the soc node's dma-ranges, as in the Linux dtsi:
 *       BCM2835: 0x40000000  (L2-coherent alias, ARM manages own L1)
 *       BCM2710: 0xC0000000  (uncached alias, ARM manages own L1+L2)
 *     ARM memory is identity-mapped (VA == PA) on both, so OR-ing the
 *     alias onto an ARM address yields the bus address.
 *
 *   - The peripheral block is reached at VC bus 0x7E000000. On BCM2710
 *     the ARM view is 0x3F000000..0x3FFFFFFF; on BCM2835 it is
 *     0x20000000..0x20FFFFFF. The DMA node's own reg address sits inside
 *     the peripheral window, so masking its top byte gives the ARM base
 *     at build time without a per-SoC #ifdef.
 *
 * Confirmed against the BCM2835 ARM Peripherals datasheet ch. 1.2.1 and
 * Linux arch/arm/boot/dts/broadcom/{bcm2835,bcm2710}.dtsi `dma-ranges`.
 */
#define DMA_BUS_ALIAS \
	((uint32_t)DT_DMA_RANGES_CHILD_BUS_ADDRESS_BY_IDX(DT_PATH(soc), 0))
#define DMA_PERIPH_ARM  ((uint32_t)DT_INST_REG_ADDR(0) & 0xFF000000U)
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

/* Channel states, as in the DMA API state machine. CS.ACTIVE cannot
 * stand in for them: if a chain ends between the handler reading CS
 * and writing it back, ACTIVE is left set on a channel that has
 * nothing to run (CONBLK_AD is 0).
 */
enum dma_bcm2835_state {
	DMA_BCM2835_CONFIGURED,
	DMA_BCM2835_RUNNING,
	DMA_BCM2835_SUSPENDED,
};

struct dma_bcm2835_channel {
	dma_callback_t callback;
	void *user_data;
	uint32_t direction;
	uint32_t n_blocks;  /* 1..MAX_CBS_PER_CHAN, 0 until configured */
	uint32_t next_blk;  /* first block not yet reported */
	uint32_t seq;       /* bumped by start and stop */
	enum dma_bcm2835_state state;
	bool cyclic;
	bool per_block;     /* every CB interrupts: a callback per block */
	bool error_cb_dis;
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
	struct k_spinlock lock;
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

/* Build the TI (transfer information) word for one block. INT_EN is
 * added per CB by the caller.
 */
static uint32_t dma_bcm2835_build_ti(uint32_t direction, uint32_t dma_slot,
				     uint8_t src_adj, uint8_t dst_adj)
{
	uint32_t ti = TI_WAIT_RESP;

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
	k_spinlock_key_t key;
	bool per_block;
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

	/* The engine may be walking the CBs of a running or suspended
	 * channel. A channel left half-built by a failed config stays
	 * unconfigured (n_blocks == 0).
	 */
	key = k_spin_lock(&data->lock);
	if (chan->state != DMA_BCM2835_CONFIGURED) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	chan->n_blocks = 0U;
	k_spin_unlock(&data->lock, key);

	/* A callback per block needs every CB to interrupt; otherwise only
	 * the last one does, once per list (per pass in cyclic mode).
	 */
	per_block = cfg->complete_callback_en && cfg->block_count > 1U;

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
		if (per_block || n == cfg->block_count - 1U) {
			ti |= TI_INT_EN;
		}

		cbs[n].info = ti;
		cbs[n].src = dma_bcm2835_bus_addr((uintptr_t)blk->source_address);
		cbs[n].dst = dma_bcm2835_bus_addr((uintptr_t)blk->dest_address);
		cbs[n].length = blk->block_size;
		cbs[n].stride = 0U;
		cbs[n].next = 0U; /* set after the loop */
		cbs[n].pad[0] = 0U;
		cbs[n].pad[1] = 0U;

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

	key = k_spin_lock(&data->lock);
	chan->callback = cfg->dma_callback;
	chan->user_data = cfg->user_data;
	chan->direction = cfg->channel_direction;
	chan->n_blocks = cfg->block_count;
	chan->next_blk = 0U;
	chan->cyclic = cfg->cyclic != 0U;
	chan->per_block = per_block;
	chan->error_cb_dis = cfg->error_callback_dis != 0U;
	k_spin_unlock(&data->lock, key);

	return 0;
}

/* Reload updates the first CB's src / dst / length in place, for
 * re-arming a single-block transfer between runs.
 */
static int dma_bcm2835_reload(const struct device *dev, uint32_t channel,
			      uint32_t src, uint32_t dst, size_t size)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct bcm2835_dma_cb *cbs;
	k_spinlock_key_t key;
	int ret = 0;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	cbs = data->cbs[channel];

	key = k_spin_lock(&data->lock);
	if (data->channels[channel].state != DMA_BCM2835_CONFIGURED) {
		ret = -EBUSY;
	} else {
		cbs[0].src = dma_bcm2835_bus_addr(src);
		cbs[0].dst = dma_bcm2835_bus_addr(dst);
		cbs[0].length = size;
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int dma_bcm2835_start(const struct device *dev, uint32_t channel)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan;
	struct bcm2835_dma_cb *cbs;
	k_spinlock_key_t key;
	int ret = 0;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	chan = &data->channels[channel];
	cbs = data->cbs[channel];

	key = k_spin_lock(&data->lock);

	if (chan->state != DMA_BCM2835_CONFIGURED) {
		/* Starting a started channel is allowed and changes nothing. */
		goto out;
	}

	if (chan->n_blocks == 0U) {
		ret = -EINVAL;
		goto out;
	}

	/* The engine reads the CBs from memory, so clean them there. The
	 * data buffers are the caller's to maintain.
	 */
	sys_cache_data_flush_range(cbs, chan->n_blocks * sizeof(*cbs));

	/* RESET clears anything a previous run left behind (a stale INT,
	 * ACTIVE written back after the chain ended); then point the
	 * channel at the first CB and go.
	 */
	dma_wr(dev, chan_off(channel, DMA_CS), CS_RESET);
	dma_wr(dev, chan_off(channel, DMA_CONBLK_AD),
	       dma_bcm2835_bus_addr((uintptr_t)&cbs[0]));
	chan->next_blk = 0U;
	chan->seq++;
	chan->state = DMA_BCM2835_RUNNING;
	dma_wr(dev, chan_off(channel, DMA_CS), CS_ACTIVE);

out:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int dma_bcm2835_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan;
	k_spinlock_key_t key;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	chan = &data->channels[channel];

	/* RESET is write-only and self-clearing; it returns the channel
	 * to a known idle state and drops any pending interrupt. Linux
	 * performs a more careful paused-abort sequence for DREQ-stalled
	 * transfers -- not needed for the cases this driver handles yet.
	 */
	key = k_spin_lock(&data->lock);
	dma_wr(dev, chan_off(channel, DMA_CS), CS_RESET);
	chan->state = DMA_BCM2835_CONFIGURED;
	chan->seq++;
	k_spin_unlock(&data->lock, key);

	return 0;
}

/* Clearing ACTIVE pauses the engine where it is: CONBLK_AD, the byte
 * count and any pending interrupt are kept, so blocks that finished
 * before the pause are still reported.
 */
static int dma_bcm2835_suspend(const struct device *dev, uint32_t channel)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan;
	k_spinlock_key_t key;
	int ret = 0;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	chan = &data->channels[channel];

	key = k_spin_lock(&data->lock);
	switch (chan->state) {
	case DMA_BCM2835_RUNNING:
		/* INT and END are write-1-to-clear, so writing 0 leaves a
		 * pending interrupt for the handler.
		 */
		dma_wr(dev, chan_off(channel, DMA_CS), 0U);
		chan->state = DMA_BCM2835_SUSPENDED;
		break;
	case DMA_BCM2835_SUSPENDED:
		break;
	default:
		ret = -EINVAL;
		break;
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

/* A non-cyclic transfer continues exactly where it was paused, so
 * nothing already sent is sent again. A cyclic ring instead restarts
 * the block it was paused in: the pause leaves the engine partway
 * through that block, and a consumer that recycles the ring while it
 * is suspended (tests/drivers/dma/cyclic does) would otherwise get a
 * block whose first part was never rewritten. RESET discards the
 * partial block and the ring carries on from the same CB, so block
 * accounting stays in step. If a block finished just before the pause,
 * its interrupt is still pending and RESET would drop it, so the ring
 * then simply continues.
 */
static int dma_bcm2835_resume(const struct device *dev, uint32_t channel)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan;
	k_spinlock_key_t key;
	uint32_t cb;
	int ret = 0;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	chan = &data->channels[channel];

	key = k_spin_lock(&data->lock);
	switch (chan->state) {
	case DMA_BCM2835_SUSPENDED:
		if (chan->cyclic &&
		    (dma_rd(dev, chan_off(channel, DMA_CS)) & CS_INT) == 0U) {
			cb = dma_rd(dev, chan_off(channel, DMA_CONBLK_AD));
			dma_wr(dev, chan_off(channel, DMA_CS), CS_RESET);
			dma_wr(dev, chan_off(channel, DMA_CONBLK_AD), cb);
		}
		dma_wr(dev, chan_off(channel, DMA_CS), CS_ACTIVE);
		chan->state = DMA_BCM2835_RUNNING;
		break;
	case DMA_BCM2835_RUNNING:
		break;
	default:
		ret = -EINVAL;
		break;
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int dma_bcm2835_get_status(const struct device *dev, uint32_t channel,
				  struct dma_status *stat)
{
	const struct dma_bcm2835_config *dcfg = dev->config;
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan;
	k_spinlock_key_t key;

	if (!chan_valid(dcfg, channel)) {
		return -EINVAL;
	}

	chan = &data->channels[channel];

	key = k_spin_lock(&data->lock);
	stat->busy = chan->state == DMA_BCM2835_RUNNING;
	stat->dir = chan->direction;
	stat->pending_length = dma_rd(dev, chan_off(channel, DMA_TXFR_LEN));
	k_spin_unlock(&data->lock, key);

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

/* Blocks finished since the handler last ran, from the engine's
 * position. CONBLK_AD holds the CB being executed -- the next one as
 * soon as a CB finishes -- and reads 0 once a non-cyclic chain has
 * ended, which is also what Linux's handler keys completion on.
 */
static uint32_t dma_bcm2835_blocks_done(const struct device *dev, uint32_t ch,
					const struct dma_bcm2835_channel *chan)
{
	struct dma_bcm2835_data *data = dev->data;
	uint32_t cb = dma_rd(dev, chan_off(ch, DMA_CONBLK_AD));
	uint32_t pos;

	if (cb == 0U) {
		return chan->n_blocks - chan->next_blk;
	}

	if (!chan->per_block) {
		/* Only the last CB interrupts: in a ring each interrupt is
		 * one pass, and a chain still running has nothing to report.
		 */
		return chan->cyclic ? chan->n_blocks : 0U;
	}

	/* An unchanged position means nothing new: the interrupt came from
	 * a block the handler already counted on its previous run. A whole
	 * pass round a ring between two interrupts is therefore not seen;
	 * blocks must be long enough for the handler to run once a pass.
	 */
	pos = (cb - dma_bcm2835_bus_addr((uintptr_t)&data->cbs[ch][0])) /
	      sizeof(struct bcm2835_dma_cb);

	return (pos + chan->n_blocks - chan->next_blk) % chan->n_blocks;
}

static void dma_bcm2835_chan_isr(const struct device *dev, uint32_t ch)
{
	struct dma_bcm2835_data *data = dev->data;
	struct dma_bcm2835_channel *chan = &data->channels[ch];
	dma_callback_t callback;
	void *user_data;
	k_spinlock_key_t key;
	uint32_t cs, first, count, n, seq;
	bool per_block, report, stale;

	key = k_spin_lock(&data->lock);

	cs = dma_rd(dev, chan_off(ch, DMA_CS));
	if ((cs & CS_INT) == 0U) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	/* Ack: INT and END are W1C. ACTIVE (bit 0) is R/W and writing 0
	 * to it pauses the engine, so it is written back as read.
	 */
	dma_wr(dev, chan_off(ch, DMA_CS), cs | CS_INT | CS_END);

	if (chan->state == DMA_BCM2835_CONFIGURED) {
		/* Late interrupt from a chain already reported or stopped. */
		k_spin_unlock(&data->lock, key);
		return;
	}

	callback = chan->callback;
	user_data = chan->user_data;

	if ((cs & CS_ERROR) != 0U) {
		report = !chan->error_cb_dis;
		dma_wr(dev, chan_off(ch, DMA_CS), CS_RESET);
		chan->state = DMA_BCM2835_CONFIGURED;
		chan->seq++;
		k_spin_unlock(&data->lock, key);

		if (report && callback != NULL) {
			callback(dev, user_data, ch, -EIO);
		}
		return;
	}

	first = chan->next_blk;
	count = dma_bcm2835_blocks_done(dev, ch, chan);
	n = chan->n_blocks;
	per_block = chan->per_block;
	seq = chan->seq;

	if (chan->cyclic) {
		chan->next_blk = (first + count) % n;
	} else {
		chan->next_blk = first + count;
		if (chan->next_blk == n) {
			/* Configured again before the callback runs, so the
			 * callback can reconfigure and restart the channel.
			 */
			chan->state = DMA_BCM2835_CONFIGURED;
		}
	}

	k_spin_unlock(&data->lock, key);

	for (uint32_t i = 0; i < count && callback != NULL; i++) {
		uint32_t blk = (first + i) % n;
		bool last = blk == n - 1U;

		if (!last && !per_block) {
			continue;
		}

		/* Stop if a callback has restarted or stopped the channel. */
		key = k_spin_lock(&data->lock);
		stale = chan->seq != seq;
		k_spin_unlock(&data->lock, key);
		if (stale) {
			break;
		}

		callback(dev, user_data, ch,
			 last ? DMA_STATUS_COMPLETE : DMA_STATUS_BLOCK);
	}
}

static void dma_bcm2835_isr(const struct device *dev)
{
	const struct dma_bcm2835_config *dcfg = dev->config;

	/* Channels can share an interrupt line, so check every one. */
	for (uint32_t ch = 0; ch < dcfg->num_channels; ch++) {
		if ((dcfg->channel_mask & BIT(ch)) != 0U) {
			dma_bcm2835_chan_isr(dev, ch);
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
