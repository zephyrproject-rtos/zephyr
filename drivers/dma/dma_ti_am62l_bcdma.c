/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * TI AM62L main DMSS Block Copy DMA (BCDMA): memory to memory block-copy
 * channels and the split TX/RX channels that serve the PDMA peripherals.
 *
 * Written from the AM62L TRM (SPRUJB4A): chapter 11.1.3 and 11.3 for the
 * programming model, 14.6.1 (DMASS_BCDMA_0) for the registers.
 *
 * Model:
 * - Each channel owns one ring in "exposed ring mode with dual queues"
 *   (FLOWRT_SIZE.QMODE = 1): software adds descriptor pointers on the
 *   forward queue and rings the forward doorbell; the BCDMA returns each
 *   finished descriptor on the reverse queue of the same ring and raises the
 *   ring's COMPLETE interrupt while the reverse occupancy is not zero
 *   (TRM 11.3.4 figure, FLOWRT_INT_STATUS.COMPLETE).
 * - One TR Packet Descriptor (TRPD) per dma_config() or dma_reload(), one
 *   type 15 TR per block.
 * - The ring base and size are reached through the FLOWRT aliases and the
 *   channel through CHANRT, both reset-enabled by FLOW_RESRC.ALIAS and
 *   CHAN_RESRC (TRM 14.6.1.2.16 and 14.6.1.2.21), so no TISCI is needed.
 * - The ring interrupt is a GIC SPI, taken from the interrupt-map of the
 *   node.
 *
 * Data buffers belong to the caller: it flushes the source and invalidates
 * the destination. The driver maintains the cache for its own descriptors
 * and ring, unless the channel uses ASEL 14 or 15 (ACP, TRM 3.5).
 */

#define DT_DRV_COMPAT ti_am62l_bcdma

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "dma_ti_am62l_bcdma.h"

LOG_MODULE_REGISTER(dma_ti_am62l_bcdma, CONFIG_DMA_LOG_LEVEL);

#define BCDMA_GCFG_REVISION          0x00U
#define BCDMA_GCFG_CAP0              0x20U
#define BCDMA_GCFG_CAP2              0x28U
#define BCDMA_CAP0_TYPE15            BIT(15)
#define BCDMA_CAP2_CHAN_CNT          GENMASK(8, 0)

/* CHANRT region, 0x1000 per channel (TRM 14.6.1.2.42 - 14.6.1.2.47) */
#define BCDMA_CHANRT_STRIDE          0x1000U
#define BCDMA_CHANRT_CTL             0x00U
#define BCDMA_CHANRT_CFG             0x04U
#define BCDMA_CHANRT_STATUS0         0x40U
#define BCDMA_CHANRT_THREAD          0x68U
#define BCDMA_CHANRT_BCNT            0x408U
#define BCDMA_CHANRT_STATIC_TR_XY    0x800U
#define BCDMA_CHANRT_STATIC_TR_Z     0x804U
#define BCDMA_CHANRT_PDMA_LCNTS      0x808U
#define BCDMA_CHANRT_PDMA_STATE      0x80CU
#define BCDMA_CHANRT_PERIPH_BCNT     0x810U
#define BCDMA_CHANRT_PDMA_RT         0x220U
#define BCDMA_PDMA_FLUSH             0x09000000U
#define BCDMA_CTL_ENABLE             BIT(31)
#define BCDMA_CTL_TEARDOWN           BIT(30)
#define BCDMA_CTL_PAUSE              BIT(29)
#define BCDMA_CTL_FORCED_TEARDOWN    BIT(28)
#define BCDMA_CTL_AUTOPAIR           BIT(23)
#define BCDMA_CTL_PAIR_TDOWN_FAIL    BIT(18)
#define BCDMA_CTL_PAIR_TIMEOUT       BIT(17)
#define BCDMA_CTL_PAIR_COMPLETE      BIT(16)
#define BCDMA_CTL_ERROR              BIT(0)
#define BCDMA_CTL_PAIR_STATUS        (BCDMA_CTL_PAIR_TDOWN_FAIL | BCDMA_CTL_PAIR_TIMEOUT)
#define BCDMA_STATIC_Z               GENMASK(23, 0)
#define BCDMA_PERIPH_BCNT_MASK       GENMASK(30, 0)
#define BCDMA_CFG_CHAN_TYPE          GENMASK(19, 16)
#define BCDMA_CHAN_TYPE_BLOCK_COPY   12U

/* FLOWRT (ring) region, 0x2000 per ring (TRM Table 11-5, 14.6.1.2.2 - .14) */
#define BCDMA_RING_STRIDE            0x2000U
#define BCDMA_RING_REVERSE           0x1000U
#define BCDMA_RING_DB                0x10U
#define BCDMA_RING_OCC               0x18U
#define BCDMA_RING_BA_LO             0x40U
#define BCDMA_RING_BA_HI             0x44U
#define BCDMA_RING_SIZE              0x48U
#define BCDMA_RING_INT_ENABLE_SET    0x140U
#define BCDMA_RING_INT_ENABLE_CLR    0x148U
#define BCDMA_RING_INT_STATUS        0x158U
#define BCDMA_RING_BA_HI_ASEL        GENMASK(19, 16)
#define BCDMA_RING_BA_HI_ADDR        GENMASK(3, 0)
#define BCDMA_RING_SIZE_ELEMENTS     GENMASK(15, 0)
#define BCDMA_RING_OCC_MASK          GENMASK(16, 0)
/* Reverse ring only: OCC bit 31 is TDOWN_COMPLETE, DB bit 31 is TDOWN_ACK */
#define BCDMA_RING_TDOWN_COMPLETE    BIT(31)
#define BCDMA_RING_DB_TDOWN_ACK      BIT(31)
/* Doorbell ENTRY_CNT is a signed 8-bit count */
#define BCDMA_RING_DB_CNT            GENMASK(7, 0)
#define BCDMA_RING_DB_MAX            127U
#define BCDMA_INT_COMPLETE           BIT(0)
#define BCDMA_INT_ERROR              BIT(1)
#define BCDMA_INT_TR                 BIT(2)
#define BCDMA_INT_ALL                (BCDMA_INT_COMPLETE | BCDMA_INT_ERROR | BCDMA_INT_TR)

/* Channel index space: split channels 0-127, block-copy channels 128-143 */
#define BCDMA_BCHAN_FIRST            128U
#define BCDMA_BCHAN_MAX              16U
#define BCDMA_NUM_CHANNELS           (BCDMA_BCHAN_FIRST + BCDMA_BCHAN_MAX)

#define BCDMA_NUM_BCHAN              CONFIG_DMA_TI_AM62L_BCDMA_MEMCPY_CHANNELS
#define BCDMA_NUM_SCHAN              CONFIG_DMA_TI_AM62L_BCDMA_SPLIT_CHANNELS
#define BCDMA_CHAN_UNBOUND           UINT32_MAX
#define BCDMA_RING_ENTRIES           CONFIG_DMA_TI_AM62L_BCDMA_RING_SIZE
#define BCDMA_MAX_TR                 CONFIG_DMA_TI_AM62L_BCDMA_MAX_TR

/* Descriptor slot size, rounded to whole 64-byte lines (TRM 11.3.2 note) */
#define BCDMA_DESC_SIZE                                                                            \
	ROUND_UP(BCDMA_TR15_SIZE * (BCDMA_MAX_TR + 1U) + BCDMA_TR_RESP_SIZE * BCDMA_MAX_TR, 64U)
/* Data alignment TI recommends (TRM 11.4: 16 bytes), raised to a cache line */
#define BCDMA_ADDR_ALIGN             64U

#define BCDMA_TEARDOWN_TIMEOUT_US    1000U
#define BCDMA_PAIR_TIMEOUT_US        1000U
#define BCDMA_SPLIT_TEARDOWN_US      CONFIG_DMA_TI_AM62L_BCDMA_SPLIT_TEARDOWN_TIMEOUT_US

#define BCDMA_PDMA_MCASP_FIRST 48U
#define BCDMA_PDMA_MCASP_LAST  53U

/* dma_slot bits 3:0 select the ASEL */
#define BCDMA_DMA_SLOT_ASEL          GENMASK(3, 0)
/* ASEL 14 and 15 route through the A53 ACP and are coherent with its caches (TRM 3.5) */
#define BCDMA_ASEL_ACP               14U
#define BCDMA_ASEL_ACP_NO_ALLOC      15U

#define BCDMA_ASEL_IS_COHERENT(asel) ((asel) == BCDMA_ASEL_ACP || (asel) == BCDMA_ASEL_ACP_NO_ALLOC)

struct bcdma_chan_mem {
	uint8_t desc[BCDMA_RING_ENTRIES][BCDMA_DESC_SIZE] __aligned(64);
	uint64_t ring[BCDMA_RING_ENTRIES] __aligned(64);
};

BUILD_ASSERT(sizeof(((struct bcdma_chan_mem *)0)->ring) % 64U == 0U,
	     "ring must fill whole cache lines");
/* The free running counters index the ring modulo its size */
BUILD_ASSERT(IS_POWER_OF_TWO(BCDMA_RING_ENTRIES), "ring size must be a power of two");

struct bcdma_slot {
	uint32_t tr_count;
	uint32_t bytes;
	uint32_t rec;
};

struct bcdma_chan {
	struct bcdma_chan_mem *mem;
	struct bcdma_slot slot[BCDMA_RING_ENTRIES];
	dma_callback_t callback;
	void *user_data;
	/* Free running counters: prepared, pushed to and popped from the ring */
	uint32_t prep;
	uint32_t push;
	uint32_t pop;
	/* Value of push when the ring was last programmed */
	uint32_t ring_base;
	uint32_t id;
	enum dma_channel_direction dir;
	uint32_t static_xy;
	uint32_t static_z;
	uint32_t rx_block;
	uint32_t rx_burst;
	uint32_t pbcnt_base;
	uint8_t asel;
	bool split;
	bool enabled;
	bool cyclic;
	bool error_cb;
	bool irq_on;
};

struct bcdma_irq {
	const struct device *dev;
	uint32_t ring;
	unsigned int irq;
};

struct bcdma_config {
	DEVICE_MMIO_NAMED_ROM(gcfg);
	DEVICE_MMIO_NAMED_ROM(chanrt);
	DEVICE_MMIO_NAMED_ROM(bchanrt);
	DEVICE_MMIO_NAMED_ROM(ringrt);
	const struct bcdma_irq *irqs;
	size_t num_irqs;
	void (*irq_config)(void);
	uint8_t asel;
};

struct bcdma_data {
	/* dma_context must be first, see dma_request_channel() */
	struct dma_context ctx;
	ATOMIC_DEFINE(channels_atomic, BCDMA_NUM_CHANNELS);
	DEVICE_MMIO_NAMED_RAM(gcfg);
	DEVICE_MMIO_NAMED_RAM(chanrt);
	DEVICE_MMIO_NAMED_RAM(bchanrt);
	DEVICE_MMIO_NAMED_RAM(ringrt);
	struct k_spinlock lock;
	struct bcdma_chan bchan[BCDMA_NUM_BCHAN];
	struct bcdma_chan schan[BCDMA_NUM_SCHAN];
};

/* Used by the DEVICE_MMIO_NAMED_* accessors */
#define DEV_CFG(dev)  ((const struct bcdma_config *)(dev)->config)
#define DEV_DATA(dev) ((struct bcdma_data *)(dev)->data)

static inline uintptr_t bcdma_phys(const void *ptr)
{
	/* The A53 maps RAM 1:1 */
	return (uintptr_t)ptr;
}

static inline uintptr_t bcdma_chanrt(const struct device *dev, uint32_t channel)
{
	if (channel < BCDMA_BCHAN_FIRST) {
		return DEVICE_MMIO_NAMED_GET(dev, chanrt) + channel * BCDMA_CHANRT_STRIDE;
	}

	return DEVICE_MMIO_NAMED_GET(dev, bchanrt) +
	       (channel - BCDMA_BCHAN_FIRST) * BCDMA_CHANRT_STRIDE;
}

static inline uintptr_t bcdma_ring(const struct device *dev, uint32_t ring)
{
	return DEVICE_MMIO_NAMED_GET(dev, ringrt) + ring * BCDMA_RING_STRIDE;
}

static struct bcdma_chan *bcdma_get_chan(const struct device *dev, uint32_t channel)
{
	struct bcdma_data *data = dev->data;

	if (channel < BCDMA_BCHAN_FIRST) {
		for (size_t i = 0; i < BCDMA_NUM_SCHAN; i++) {
			if (data->schan[i].id == channel) {
				return &data->schan[i];
			}
		}
		return NULL;
	}

	if (channel >= BCDMA_BCHAN_FIRST + BCDMA_NUM_BCHAN) {
		return NULL;
	}

	return &data->bchan[channel - BCDMA_BCHAN_FIRST];
}

static struct bcdma_chan *bcdma_bind_chan(const struct device *dev, uint32_t channel)
{
	struct bcdma_data *data = dev->data;
	struct bcdma_chan *ch = bcdma_get_chan(dev, channel);

	if (ch != NULL || channel >= BCDMA_BCHAN_FIRST) {
		return ch;
	}

	for (size_t i = 0; i < BCDMA_NUM_SCHAN; i++) {
		if (data->schan[i].id == BCDMA_CHAN_UNBOUND) {
			data->schan[i].id = channel;
			data->schan[i].split = true;
			return &data->schan[i];
		}
	}

	return NULL;
}

static const struct bcdma_irq *bcdma_find_irq(const struct device *dev, uint32_t ring)
{
	const struct bcdma_config *cfg = dev->config;

	for (size_t i = 0; i < cfg->num_irqs; i++) {
		if (cfg->irqs[i].ring == ring) {
			return &cfg->irqs[i];
		}
	}

	return NULL;
}

static int bcdma_cache_flush(const struct bcdma_chan *ch, void *addr, size_t size)
{
	if (BCDMA_ASEL_IS_COHERENT(ch->asel)) {
		return 0;
	}

	return sys_cache_data_flush_range(addr, size);
}

static int bcdma_cache_invd(const struct bcdma_chan *ch, void *addr, size_t size)
{
	if (BCDMA_ASEL_IS_COHERENT(ch->asel)) {
		return 0;
	}

	return sys_cache_data_invd_range(addr, size);
}

static void bcdma_ring_db(uintptr_t db, int32_t cnt)
{
	while (cnt != 0) {
		int32_t step = CLAMP(cnt, -(int32_t)BCDMA_RING_DB_MAX, (int32_t)BCDMA_RING_DB_MAX);

		sys_write32((uint32_t)step & BCDMA_RING_DB_CNT, db);
		cnt -= step;
	}
}

static void bcdma_log_state(const struct device *dev, uint32_t channel, const char *what)
{
	uintptr_t chanrt = bcdma_chanrt(dev, channel);
	uintptr_t ring = bcdma_ring(dev, channel);

	LOG_ERR("chan %u %s: ctl 0x%08x status0 0x%08x bcnt %u focc 0x%x rocc 0x%x int 0x%x",
		channel, what, sys_read32(chanrt + BCDMA_CHANRT_CTL),
		sys_read32(chanrt + BCDMA_CHANRT_STATUS0), sys_read32(chanrt + BCDMA_CHANRT_BCNT),
		sys_read32(ring + BCDMA_RING_OCC),
		sys_read32(ring + BCDMA_RING_REVERSE + BCDMA_RING_OCC),
		sys_read32(ring + BCDMA_RING_INT_STATUS));
	if (channel < BCDMA_BCHAN_FIRST) {
		LOG_ERR("chan %u %s: thread 0x%04x static_xy 0x%08x static_z %u pdma_state 0x%08x "
			"lcnts 0x%08x periph_bcnt %u",
			channel, what, sys_read32(chanrt + BCDMA_CHANRT_THREAD),
			sys_read32(chanrt + BCDMA_CHANRT_STATIC_TR_XY),
			(unsigned int)FIELD_GET(BCDMA_STATIC_Z,
						sys_read32(chanrt + BCDMA_CHANRT_STATIC_TR_Z)),
			sys_read32(chanrt + BCDMA_CHANRT_PDMA_STATE),
			sys_read32(chanrt + BCDMA_CHANRT_PDMA_LCNTS),
			sys_read32(chanrt + BCDMA_CHANRT_PERIPH_BCNT));
	}
}

static bool bcdma_wait_ctl_clear(uintptr_t chanrt, uint32_t mask)
{
	return WAIT_FOR((sys_read32(chanrt + BCDMA_CHANRT_CTL) & mask) == 0U,
			BCDMA_TEARDOWN_TIMEOUT_US, k_busy_wait(1));
}

static void bcdma_ring_clean(const struct device *dev, uint32_t channel)
{
	uintptr_t ring = bcdma_ring(dev, channel);
	uintptr_t rev = ring + BCDMA_RING_REVERSE;

	if ((sys_read32(rev + BCDMA_RING_OCC) & BCDMA_RING_TDOWN_COMPLETE) != 0U) {
		sys_write32(BCDMA_RING_DB_TDOWN_ACK, rev + BCDMA_RING_DB);
	}
	bcdma_ring_db(rev + BCDMA_RING_DB,
		      -(int32_t)(sys_read32(rev + BCDMA_RING_OCC) & BCDMA_RING_OCC_MASK));
	bcdma_ring_db(ring + BCDMA_RING_DB,
		      -(int32_t)(sys_read32(ring + BCDMA_RING_OCC) & BCDMA_RING_OCC_MASK));
	sys_write32(BCDMA_INT_ALL, ring + BCDMA_RING_INT_STATUS);
}

static int bcdma_split_reset(const struct device *dev, uint32_t channel)
{
	uintptr_t chanrt = bcdma_chanrt(dev, channel);
	uintptr_t ctl_reg = chanrt + BCDMA_CHANRT_CTL;
	uint32_t ctl_in = sys_read32(ctl_reg);
	uint32_t ctl = ctl_in;

	sys_write32(BCDMA_INT_ALL, bcdma_ring(dev, channel) + BCDMA_RING_INT_ENABLE_CLR);

	if ((ctl & BCDMA_CTL_PAIR_COMPLETE) != 0U && (ctl & BCDMA_CTL_PAIR_STATUS) != 0U) {
		sys_write32((ctl & (BCDMA_CTL_ENABLE | BCDMA_CTL_AUTOPAIR)) |
				    BCDMA_CTL_PAIR_COMPLETE | BCDMA_CTL_PAIR_STATUS,
			    ctl_reg);
		if (!WAIT_FOR((sys_read32(ctl_reg) & BCDMA_CTL_PAIR_COMPLETE) == 0U,
			      BCDMA_PAIR_TIMEOUT_US, k_busy_wait(1))) {
			LOG_DBG("chan %u: stale pairing status wait timed out", channel);
		}
		ctl = sys_read32(ctl_reg);
		LOG_WRN("chan %u: stale pairing status cleared: ctl 0x%08x -> 0x%08x", channel,
			ctl_in, ctl);
	}

	if ((ctl & BCDMA_CTL_PAIR_COMPLETE) != 0U) {
		static const uint32_t step[] = {
			BCDMA_CTL_TEARDOWN,
			BCDMA_CTL_TEARDOWN | BCDMA_CTL_FORCED_TEARDOWN,
		};
		const struct bcdma_chan *ch = bcdma_get_chan(dev, channel);
		bool done = false;
		size_t first = 0U;

		if (ch != NULL && ch->dir == PERIPHERAL_TO_MEMORY && ch->push == ch->pop) {
			first = 1U;
		}

		for (size_t i = first; i < ARRAY_SIZE(step) && !done; i++) {
			bool en_clear;

			if (i > 0U) {
				sys_write32(BCDMA_PDMA_FLUSH, chanrt + BCDMA_CHANRT_PDMA_RT);
			}

			sys_write32(BCDMA_CTL_ENABLE | step[i] |
					    (ctl & (BCDMA_CTL_AUTOPAIR | BCDMA_CTL_PAIR_COMPLETE)) |
					    BCDMA_CTL_PAIR_STATUS,
				    ctl_reg);
			en_clear = WAIT_FOR((sys_read32(ctl_reg) & BCDMA_CTL_ENABLE) == 0U,
					    BCDMA_SPLIT_TEARDOWN_US, k_busy_wait(1));
			if (en_clear) {
				const uint32_t pair =
					BCDMA_CTL_PAIR_COMPLETE | BCDMA_CTL_PAIR_TDOWN_FAIL;

				if (!WAIT_FOR((sys_read32(ctl_reg) & pair) !=
						      BCDMA_CTL_PAIR_COMPLETE,
					      BCDMA_PAIR_TIMEOUT_US, k_busy_wait(1))) {
					LOG_DBG("chan %u: unpairing wait timed out", channel);
				}
			}
			ctl = sys_read32(ctl_reg);
			done = en_clear && (ctl & BCDMA_CTL_PAIR_TDOWN_FAIL) == 0U;
			if (!done) {
				bcdma_log_state(dev, channel,
						i == 0U ? "teardown failed"
							: "forced teardown failed");
			} else if (i == 0U || first == 1U) {
				LOG_DBG("chan %u teardown done: ctl 0x%08x (PAIR_COMPLETE %s)",
					channel, ctl,
					(ctl & BCDMA_CTL_PAIR_COMPLETE) != 0U ? "still set"
									      : "cleared");
			} else {
				LOG_WRN("chan %u forced teardown done: ctl 0x%08x "
					"(PAIR_COMPLETE %s)",
					channel, ctl,
					(ctl & BCDMA_CTL_PAIR_COMPLETE) != 0U ? "still set"
									      : "cleared");
			}
		}

		if (!done) {
			LOG_ERR("chan %u: peer still paired, channel left as is (ctl 0x%08x)",
				channel, ctl);
			return -ETIMEDOUT;
		}
	}

	sys_write32(BCDMA_CTL_PAIR_STATUS, ctl_reg);
	bcdma_ring_clean(dev, channel);

	return 0;
}

/* A forced teardown follows if the normal one does not finish (TRM 14.6.1.2.42) */
static int bcdma_chan_reset(const struct device *dev, uint32_t channel)
{
	uintptr_t chanrt = bcdma_chanrt(dev, channel);
	uintptr_t ring = bcdma_ring(dev, channel);
	uint32_t ctl = sys_read32(chanrt + BCDMA_CHANRT_CTL);
	int ret = 0;

	if (channel < BCDMA_BCHAN_FIRST) {
		return bcdma_split_reset(dev, channel);
	}

	sys_write32(BCDMA_INT_ALL, ring + BCDMA_RING_INT_ENABLE_CLR);

	if ((ctl & BCDMA_CTL_ENABLE) != 0U) {
		ctl &= ~(BCDMA_CTL_PAUSE | BCDMA_CTL_PAIR_TIMEOUT | BCDMA_CTL_PAIR_TDOWN_FAIL);
		sys_write32(ctl | BCDMA_CTL_TEARDOWN, chanrt + BCDMA_CHANRT_CTL);
		if (!bcdma_wait_ctl_clear(chanrt, BCDMA_CTL_ENABLE)) {
			LOG_WRN("chan %u: teardown timed out, forcing", channel);
			sys_write32(ctl | BCDMA_CTL_TEARDOWN | BCDMA_CTL_FORCED_TEARDOWN,
				    chanrt + BCDMA_CHANRT_CTL);
			if (!bcdma_wait_ctl_clear(chanrt, BCDMA_CTL_ENABLE)) {
				bcdma_log_state(dev, channel, "forced teardown failed");
				ret = -ETIMEDOUT;
			}
		}
		if ((sys_read32(chanrt + BCDMA_CHANRT_CTL) & BCDMA_CTL_PAIR_TDOWN_FAIL) != 0U) {
			LOG_WRN("chan %u: peer teardown failed", channel);
		}
	}

	sys_write32(0U, chanrt + BCDMA_CHANRT_CTL);
	bcdma_ring_clean(dev, channel);

	return ret;
}

static int bcdma_push(const struct device *dev, uint32_t channel, struct bcdma_chan *ch);

/* The ring base and size are written on each enable, which restarts the ring indices */
static int bcdma_chan_enable(const struct device *dev, uint32_t channel, struct bcdma_chan *ch)
{
	uintptr_t chanrt = bcdma_chanrt(dev, channel);
	uintptr_t ring = bcdma_ring(dev, channel);
	uint64_t base = bcdma_phys(ch->mem->ring);
	uint32_t ba_hi = FIELD_PREP(BCDMA_RING_BA_HI_ADDR, (uint32_t)(base >> 32)) |
			 FIELD_PREP(BCDMA_RING_BA_HI_ASEL, ch->asel);
	int ret;

	/*
	 * One ring memory serves both queues (TRM 11.3.4 figure). Table 11-5
	 * also lists reverse BA/SIZE registers at +0x1040, chapter 14 does
	 * not; program both alike so either reading holds.
	 */
	for (uintptr_t r = ring; r <= ring + BCDMA_RING_REVERSE; r += BCDMA_RING_REVERSE) {
		sys_write32((uint32_t)base, r + BCDMA_RING_BA_LO);
		sys_write32(ba_hi, r + BCDMA_RING_BA_HI);
		sys_write32(FIELD_PREP(BCDMA_RING_SIZE_ELEMENTS, BCDMA_RING_ENTRIES),
			    r + BCDMA_RING_SIZE);
	}

	/*
	 * The hardware starts at ring entry 0 again. Descriptor memory stays
	 * indexed by the free running counters; the ring position of a
	 * descriptor is its counter minus ring_base.
	 */
	ch->ring_base = ch->push;

	sys_write32(BCDMA_INT_ALL, ring + BCDMA_RING_INT_STATUS);
	sys_write32(BCDMA_INT_COMPLETE | BCDMA_INT_ERROR | (ch->cyclic ? BCDMA_INT_TR : 0U),
		    ring + BCDMA_RING_INT_ENABLE_SET);

	sys_write32(0U, chanrt + BCDMA_CHANRT_CTL);

	if (!ch->split) {
		sys_write32(BCDMA_CTL_ENABLE, chanrt + BCDMA_CHANRT_CTL);
		ch->enabled = true;
		return 0;
	}

	sys_write32(ch->static_xy, chanrt + BCDMA_CHANRT_STATIC_TR_XY);
	if (ch->dir == PERIPHERAL_TO_MEMORY) {
		sys_write32(FIELD_PREP(BCDMA_STATIC_Z, ch->static_z),
			    chanrt + BCDMA_CHANRT_STATIC_TR_Z);
	}

	ret = bcdma_push(dev, channel, ch);
	if (ret != 0) {
		return ret;
	}

	ch->pbcnt_base = sys_read32(chanrt + BCDMA_CHANRT_PERIPH_BCNT);
	sys_write32(BCDMA_CTL_ENABLE | BCDMA_CTL_AUTOPAIR, chanrt + BCDMA_CHANRT_CTL);
	if (!WAIT_FOR((sys_read32(chanrt + BCDMA_CHANRT_CTL) &
		       (BCDMA_CTL_PAIR_COMPLETE | BCDMA_CTL_PAIR_TIMEOUT)) != 0U,
		      BCDMA_PAIR_TIMEOUT_US, k_busy_wait(1)) ||
	    (sys_read32(chanrt + BCDMA_CHANRT_CTL) & BCDMA_CTL_PAIR_COMPLETE) == 0U) {
		bcdma_log_state(dev, channel, "pairing failed");
		ret = bcdma_chan_reset(dev, channel);
		if (ret != 0) {
			LOG_ERR("chan %u: reset after failed pairing: %d", channel, ret);
		}
		ch->pop = ch->push;
		return -EIO;
	}

	ch->enabled = true;

	return 0;
}

static int bcdma_push(const struct device *dev, uint32_t channel, struct bcdma_chan *ch)
{
	uint32_t n = ch->prep - ch->push;
	int ret;

	if (n == 0U) {
		return 0;
	}

	if (ch->push == ch->pop) {
		/*
		 * The ring is empty: every entry gets the first descriptor so
		 * the hardware fetches it whatever its ring index is. The
		 * ring index check in the ISR therefore only bites when a
		 * descriptor is pushed behind another one still in flight.
		 */
		uint64_t first = bcdma_addr(bcdma_phys(ch->mem->desc[ch->push %
								     BCDMA_RING_ENTRIES]),
					    ch->asel);

		for (uint32_t i = 0; i < BCDMA_RING_ENTRIES; i++) {
			ch->mem->ring[i] = first;
		}
	}

	for (uint32_t i = 0; i < n; i++) {
		uint32_t cnt = ch->push + i;

		ch->mem->ring[(cnt - ch->ring_base) % BCDMA_RING_ENTRIES] =
			bcdma_addr(bcdma_phys(ch->mem->desc[cnt % BCDMA_RING_ENTRIES]), ch->asel);
	}
	ret = bcdma_cache_flush(ch, ch->mem->ring, sizeof(ch->mem->ring));
	if (ret != 0) {
		LOG_ERR("chan %u: ring cache flush failed (%d)", channel, ret);
		return ret;
	}

	ch->push += n;
	bcdma_ring_db(bcdma_ring(dev, channel) + BCDMA_RING_DB, (int32_t)n);

	return 0;
}

static int bcdma_prepare(struct bcdma_chan *ch, const struct dma_block_config *blk,
			 uint32_t block_count)
{
	uint32_t idx = ch->prep % BCDMA_RING_ENTRIES;
	uint8_t *desc = ch->mem->desc[idx];
	uint32_t rec = ch->dir == PERIPHERAL_TO_MEMORY ? BCDMA_TR_REC_32 : BCDMA_TR15_SIZE;
	uint32_t tr_count = 0U;
	uint32_t bytes = 0U;
	int ret;

	if (ch->prep - ch->pop >= BCDMA_RING_ENTRIES) {
		return -EBUSY;
	}

	for (uint32_t b = 0; b < block_count; b++, blk = blk->next_block) {
		struct bcdma_piece piece[2];
		int n;

		if (blk == NULL) {
			LOG_ERR("block list shorter than block_count %u", block_count);
			return -EINVAL;
		}

		if ((ch->dir != MEMORY_TO_PERIPHERAL &&
		     blk->dest_addr_adj != DMA_ADDR_ADJ_INCREMENT) ||
		    (ch->dir != PERIPHERAL_TO_MEMORY &&
		     blk->source_addr_adj != DMA_ADDR_ADJ_INCREMENT) ||
		    blk->source_gather_interval != 0U || blk->dest_scatter_interval != 0U ||
		    blk->source_reload_en || blk->dest_reload_en) {
			LOG_ERR("only incrementing, contiguous blocks are supported");
			return -ENOTSUP;
		}

		if (ch->dir == PERIPHERAL_TO_MEMORY && blk->block_size != ch->rx_block) {
			LOG_ERR("RX block %u bytes, the static TR Z is set for %u", blk->block_size,
				ch->rx_block);
			return -EINVAL;
		}

		if (ch->dir == PERIPHERAL_TO_MEMORY) {
			piece[0] = (struct bcdma_piece){
				.offset = 0U,
				.icnt0 = (uint16_t)ch->rx_burst,
				.icnt1 = (uint16_t)(ch->rx_block / ch->rx_burst)};
			n = 1;
		} else {
			n = bcdma_split_block(blk->block_size, piece);
		}
		if (n < 0) {
			LOG_ERR("block %u: size %u not supported", b, blk->block_size);
			return -EINVAL;
		}
		if (tr_count + n > BCDMA_MAX_TR) {
			LOG_ERR("more than %u TRs needed", BCDMA_MAX_TR);
			return -EINVAL;
		}

		for (int i = 0; i < n; i++) {
			struct bcdma_tr15 *tr = bcdma_trpd_tr_rec(desc, tr_count + i, rec);

			if (ch->dir == MEMORY_TO_PERIPHERAL) {
				bcdma_tr1_fill(tr, blk->source_address, &piece[i], ch->asel, true);
			} else if (ch->dir == PERIPHERAL_TO_MEMORY) {
				bcdma_tr1_fill(tr, blk->dest_address, &piece[i], ch->asel, false);
			} else {
				bcdma_tr15_fill(tr, blk->source_address, blk->dest_address,
						&piece[i], ch->asel);
			}
		}
		tr_count += n;
		bytes += blk->block_size;
	}

	bcdma_trpd_init_rec(desc, tr_count, ch->cyclic, rec);
	for (uint32_t i = 0; i < tr_count; i++) {
		*bcdma_trpd_resp_rec(desc, tr_count, i, rec) = 0U;
	}
	ret = bcdma_cache_flush(ch, desc, bcdma_trpd_size_rec(tr_count, rec));
	if (ret != 0) {
		return ret;
	}

	ch->slot[idx].tr_count = tr_count;
	ch->slot[idx].bytes = bytes;
	ch->slot[idx].rec = rec;
	ch->prep++;

	return 0;
}

static int bcdma_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	const struct bcdma_config *cfg = dev->config;
	struct bcdma_data *data = dev->data;
	bool split = channel < BCDMA_BCHAN_FIRST;
	const struct bcdma_irq *irq;
	struct bcdma_chan *ch;
	k_spinlock_key_t key;
	uint32_t static_xy = 0U;
	uint32_t static_z = 0U;
	uint32_t rx_block = 0U;
	uint8_t asel;
	int ret;

	if (config == NULL || config->head_block == NULL || config->block_count == 0U) {
		return -EINVAL;
	}

	if (!split && config->channel_direction != MEMORY_TO_MEMORY) {
		LOG_ERR("chan %u: block-copy channels do memory to memory only", channel);
		return -ENOTSUP;
	}

	if (split) {
		bool tx = config->channel_direction == MEMORY_TO_PERIPHERAL;
		bool acc32 = channel >= BCDMA_PDMA_MCASP_FIRST && channel <= BCDMA_PDMA_MCASP_LAST;

		if (!tx && config->channel_direction != PERIPHERAL_TO_MEMORY) {
			LOG_ERR("chan %u: split channels serve a peripheral", channel);
			return -ENOTSUP;
		}
		ret = bcdma_static_xy(tx ? config->dest_data_size : config->source_data_size,
				      tx ? config->dest_burst_length : config->source_burst_length,
				      acc32, &static_xy);
		if (ret < 0) {
			LOG_ERR("chan %u: data size and burst length do not fit the static TR",
				channel);
			return ret;
		}
		if (!tx) {
			rx_block = config->head_block->block_size;
			static_z = rx_block / config->source_burst_length;
			if (rx_block > BCDMA_ICNT_MAX ||
			    rx_block % config->source_burst_length != 0U || static_z == 0U ||
			    static_z > FIELD_GET(BCDMA_STATIC_Z, BCDMA_STATIC_Z)) {
				LOG_ERR("chan %u: RX block %u is not 1 to 65535 bytes of whole "
					"bursts of %u",
					channel, rx_block, config->source_burst_length);
				return -EINVAL;
			}
		}
	}

	asel = FIELD_GET(BCDMA_DMA_SLOT_ASEL, config->dma_slot);
	if (asel == 0U && !split) {
		asel = cfg->asel;
	}
	if (asel != 0U && !BCDMA_ASEL_IS_COHERENT(asel)) {
		LOG_ERR("chan %u: ASEL %u not supported", channel, asel);
		return -EINVAL;
	}

	irq = bcdma_find_irq(dev, channel);
	if (irq == NULL) {
		LOG_ERR("chan %u: no interrupt-map entry", channel);
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	ch = bcdma_bind_chan(dev, channel);
	if (ch == NULL) {
		k_spin_unlock(&data->lock, key);
		LOG_ERR("chan %u: no %s channel slot left", channel,
			split ? "split" : "block-copy");
		return -EINVAL;
	}

	if (ch->push != ch->pop) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	/* An idle enabled channel stays enabled unless its ring setup changes */
	if (ch->enabled && (asel != ch->asel || (config->cyclic != 0U) != ch->cyclic ||
			    config->channel_direction != ch->dir || static_xy != ch->static_xy ||
			    static_z != ch->static_z)) {
		ret = bcdma_chan_reset(dev, channel);
		ch->enabled = false;
		if (ret != 0) {
			k_spin_unlock(&data->lock, key);
			return ret;
		}
	}

	ch->prep = ch->push;
	ch->asel = asel;
	ch->dir = config->channel_direction;
	ch->static_xy = static_xy;
	ch->static_z = static_z;
	ch->rx_block = rx_block;
	ch->rx_burst = ch->dir == PERIPHERAL_TO_MEMORY ? config->source_burst_length : 0U;
	ch->cyclic = config->cyclic != 0U;
	ch->callback = config->dma_callback;
	ch->user_data = config->user_data;
	ch->error_cb = config->error_callback_dis == 0U;

	ret = bcdma_prepare(ch, config->head_block, config->block_count);
	k_spin_unlock(&data->lock, key);

	if (ret == 0 && !ch->irq_on) {
		ch->irq_on = true;
		irq_enable(irq->irq);
	}

	return ret;
}

static int bcdma_start(const struct device *dev, uint32_t channel)
{
	struct bcdma_data *data = dev->data;
	struct bcdma_chan *ch = bcdma_get_chan(dev, channel);
	k_spinlock_key_t key;
	int ret = 0;

	if (ch == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (!ch->enabled) {
		ret = bcdma_chan_reset(dev, channel);
		if (ret == 0) {
			ret = bcdma_chan_enable(dev, channel, ch);
		}
	}
	if (ret == 0) {
		ret = bcdma_push(dev, channel, ch);
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int bcdma_stop(const struct device *dev, uint32_t channel)
{
	struct bcdma_data *data = dev->data;
	struct bcdma_chan *ch = bcdma_get_chan(dev, channel);
	k_spinlock_key_t key;
	int ret = 0;

	if (ch == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (ch->enabled) {
		ret = bcdma_chan_reset(dev, channel);
		ch->enabled = false;
	}
	/* In-flight work is gone; a configured but unstarted transfer stays */
	ch->pop = ch->push;
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int bcdma_pause(const struct device *dev, uint32_t channel, bool pause)
{
	struct bcdma_data *data = dev->data;
	struct bcdma_chan *ch = bcdma_get_chan(dev, channel);
	k_spinlock_key_t key;
	uintptr_t ctl_reg;
	uint32_t ctl;

	if (ch == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (!ch->enabled) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	ctl_reg = bcdma_chanrt(dev, channel) + BCDMA_CHANRT_CTL;
	ctl = sys_read32(ctl_reg);
	ctl = pause ? (ctl | BCDMA_CTL_PAUSE) : (ctl & ~BCDMA_CTL_PAUSE);
	sys_write32(ctl, ctl_reg);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int bcdma_suspend(const struct device *dev, uint32_t channel)
{
	return bcdma_pause(dev, channel, true);
}

static int bcdma_resume(const struct device *dev, uint32_t channel)
{
	return bcdma_pause(dev, channel, false);
}

#ifdef CONFIG_DMA_64BIT
static int bcdma_reload(const struct device *dev, uint32_t channel, uint64_t src, uint64_t dst,
			size_t size)
#else
static int bcdma_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			size_t size)
#endif
{
	struct bcdma_data *data = dev->data;
	struct bcdma_chan *ch = bcdma_get_chan(dev, channel);
	struct dma_block_config blk = {
		.source_address = src,
		.dest_address = dst,
		.block_size = size,
	};
	k_spinlock_key_t key;
	int ret;

	if (ch == NULL || ch->cyclic) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	ret = bcdma_prepare(ch, &blk, 1U);
	/* A running channel takes the descriptor at once, else at dma_start() */
	if (ret == 0 && ch->enabled && ch->push != ch->pop) {
		ret = bcdma_push(dev, channel, ch);
		if (ret != 0) {
			ch->prep--;
		}
	}
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int bcdma_get_status(const struct device *dev, uint32_t channel, struct dma_status *stat)
{
	struct bcdma_data *data = dev->data;
	struct bcdma_chan *ch = bcdma_get_chan(dev, channel);
	k_spinlock_key_t key;

	if (ch == NULL || stat == NULL) {
		return -EINVAL;
	}

	*stat = (struct dma_status){.dir = ch->dir};

	key = k_spin_lock(&data->lock);
	stat->busy = ch->push != ch->pop;
	for (uint32_t i = ch->pop; i != ch->push; i++) {
		stat->pending_length += ch->slot[i % BCDMA_RING_ENTRIES].bytes;
	}
	stat->free = BCDMA_RING_ENTRIES - (ch->prep - ch->pop);
	if (ch->split && ch->enabled) {
		stat->total_copied =
			(sys_read32(bcdma_chanrt(dev, channel) + BCDMA_CHANRT_PERIPH_BCNT) -
			 ch->pbcnt_base) &
			BCDMA_PERIPH_BCNT_MASK;
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int bcdma_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	ARG_UNUSED(dev);

	switch (type) {
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = BCDMA_ADDR_ALIGN;
		return 0;
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
	case DMA_ATTR_COPY_ALIGNMENT:
		*value = 1U;
		return 0;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = BCDMA_MAX_TR;
		return 0;
	default:
		return -EINVAL;
	}
}

static bool bcdma_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	bool memcpy_chan =
		channel >= (int)BCDMA_BCHAN_FIRST && bcdma_get_chan(dev, (uint32_t)channel) != NULL;

	if (filter_param != NULL) {
		return channel == *(const int *)filter_param && memcpy_chan;
	}

	return memcpy_chan;
}

static void bcdma_chan_release(const struct device *dev, uint32_t channel)
{
	int ret = bcdma_stop(dev, channel);

	if (ret != 0) {
		LOG_ERR("chan %u: stop on release failed (%d)", channel, ret);
	}
}

static int bcdma_desc_status(uint32_t channel, struct bcdma_chan *ch, uint32_t idx)
{
	uint8_t *desc = ch->mem->desc[idx];
	uint32_t tr_count = ch->slot[idx].tr_count;
	uint32_t rec = ch->slot[idx].rec;
	int ret = bcdma_cache_invd(ch, desc, bcdma_trpd_size_rec(tr_count, rec));

	if (ret != 0) {
		LOG_ERR("chan %u: descriptor cache invalidate failed (%d)", channel, ret);
		return ret;
	}

	for (uint32_t i = 0; i < tr_count; i++) {
		uint32_t resp = *bcdma_trpd_resp_rec(desc, tr_count, i, rec);

		if (FIELD_GET(BCDMA_TR_RESP_STATUS_TYPE, resp) != 0U) {
			LOG_ERR("chan %u: TR %u response 0x%08x (type %u info %u)", channel, i,
				resp, (unsigned int)FIELD_GET(BCDMA_TR_RESP_STATUS_TYPE, resp),
				(unsigned int)FIELD_GET(BCDMA_TR_RESP_STATUS_INFO, resp));
			ret = -EIO;
		}
	}

	return ret;
}

static void bcdma_ring_isr(const void *arg)
{
	const struct bcdma_irq *irq = arg;
	const struct device *dev = irq->dev;
	struct bcdma_data *data = dev->data;
	uint32_t channel = irq->ring;
	struct bcdma_chan *ch = bcdma_get_chan(dev, channel);
	uintptr_t ring = bcdma_ring(dev, channel);
	uintptr_t rev = ring + BCDMA_RING_REVERSE;
	int status[BCDMA_RING_ENTRIES + 1];
	uint32_t ncb = 0U;
	dma_callback_t cb;
	void *user_data;
	k_spinlock_key_t key;
	uint32_t ists;
	uint32_t rocc;
	int ring_ret = 0;

	if (ch == NULL) {
		/* Not ours: mask it so a stray line cannot storm */
		sys_write32(BCDMA_INT_ALL, ring + BCDMA_RING_INT_ENABLE_CLR);
		sys_write32(BCDMA_INT_ALL, ring + BCDMA_RING_INT_STATUS);
		return;
	}

	key = k_spin_lock(&data->lock);

	ists = sys_read32(ring + BCDMA_RING_INT_STATUS);
	rocc = sys_read32(rev + BCDMA_RING_OCC) & BCDMA_RING_OCC_MASK;
	rocc = MIN(rocc, ch->push - ch->pop);

	if (rocc > 0U) {
		ring_ret = bcdma_cache_invd(ch, ch->mem->ring, sizeof(ch->mem->ring));
		if (ring_ret != 0) {
			LOG_ERR("chan %u: ring cache invalidate failed (%d)", channel, ring_ret);
		}
	}

	for (uint32_t i = 0; i < rocc; i++) {
		uint32_t idx = ch->pop % BCDMA_RING_ENTRIES;
		uint32_t pos = (ch->pop - ch->ring_base) % BCDMA_RING_ENTRIES;
		uint64_t want = bcdma_addr(bcdma_phys(ch->mem->desc[idx]), ch->asel);

		if (ring_ret == 0 && ch->mem->ring[pos] != want) {
			LOG_WRN("chan %u: ring[%u] 0x%llx, expected 0x%llx", channel, pos,
				(unsigned long long)ch->mem->ring[pos], (unsigned long long)want);
		}

		status[ncb] = bcdma_desc_status(channel, ch, idx);
		if (status[ncb] == 0) {
			status[ncb] = ring_ret;
		}
		ncb++;
		ch->pop++;
	}
	bcdma_ring_db(rev + BCDMA_RING_DB, -(int32_t)rocc);

	/* Cyclic: each TR end is a period; the descriptor never comes back */
	if (ch->cyclic && (ists & BCDMA_INT_TR) != 0U) {
		status[ncb] = DMA_STATUS_BLOCK;
		ncb++;
	}

	if ((ists & BCDMA_INT_ERROR) != 0U && rocc == 0U) {
		uintptr_t ctl_reg = bcdma_chanrt(dev, channel) + BCDMA_CHANRT_CTL;

		bcdma_log_state(dev, channel, "error");
		/* CTL.ERROR is cleared by writing 0 to it */
		sys_write32(sys_read32(ctl_reg) & ~BCDMA_CTL_ERROR, ctl_reg);
		status[ncb] = -EIO;
		ncb++;
	}

	sys_write32(ists & BCDMA_INT_ALL, ring + BCDMA_RING_INT_STATUS);

	cb = ch->callback;
	user_data = ch->user_data;
	k_spin_unlock(&data->lock, key);

	/* The channel is consistent and idle for completed work: callbacks may reconfigure it */
	for (uint32_t i = 0; i < ncb && cb != NULL; i++) {
		if (status[i] < 0 && !ch->error_cb) {
			continue;
		}
		cb(dev, user_data, channel, status[i]);
	}
}

static int bcdma_init(const struct device *dev)
{
	const struct bcdma_config *cfg = dev->config;
	struct bcdma_data *data = dev->data;
	uintptr_t gcfg;
	uint32_t cap0, cap2;

	DEVICE_MMIO_NAMED_MAP(dev, gcfg, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, chanrt, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, bchanrt, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, ringrt, K_MEM_CACHE_NONE);

	gcfg = DEVICE_MMIO_NAMED_GET(dev, gcfg);
	cap0 = sys_read32(gcfg + BCDMA_GCFG_CAP0);
	cap2 = sys_read32(gcfg + BCDMA_GCFG_CAP2);
	LOG_DBG("BCDMA rev 0x%08x cap0 0x%08x cap2 0x%08x",
		sys_read32(gcfg + BCDMA_GCFG_REVISION), cap0, cap2);

	if ((cap0 & BCDMA_CAP0_TYPE15) == 0U ||
	    FIELD_GET(BCDMA_CAP2_CHAN_CNT, cap2) < BCDMA_NUM_BCHAN) {
		LOG_ERR("no type 15 TR or too few block-copy channels");
		return -ENODEV;
	}

	for (uint32_t i = 0; i < BCDMA_NUM_BCHAN; i++) {
		uint32_t channel = BCDMA_BCHAN_FIRST + i;
		uint32_t type = FIELD_GET(BCDMA_CFG_CHAN_TYPE,
					  sys_read32(bcdma_chanrt(dev, channel) +
						     BCDMA_CHANRT_CFG));

		if (type != BCDMA_CHAN_TYPE_BLOCK_COPY) {
			LOG_WRN("chan %u: CHAN_TYPE %u, expected %u", channel, type,
				BCDMA_CHAN_TYPE_BLOCK_COPY);
		}
	}

	for (uint32_t i = 0; i < BCDMA_NUM_BCHAN; i++) {
		data->bchan[i].id = BCDMA_BCHAN_FIRST + i;
		data->bchan[i].dir = MEMORY_TO_MEMORY;
	}
	for (uint32_t i = 0; i < BCDMA_NUM_SCHAN; i++) {
		data->schan[i].id = BCDMA_CHAN_UNBOUND;
	}

	data->ctx.magic = DMA_MAGIC;
	data->ctx.dma_channels = BCDMA_NUM_CHANNELS;
	data->ctx.atomic = data->channels_atomic;

	cfg->irq_config();

	return 0;
}

static DEVICE_API(dma, bcdma_api) = {
	.config = bcdma_config,
	.reload = bcdma_reload,
	.start = bcdma_start,
	.stop = bcdma_stop,
	.suspend = bcdma_suspend,
	.resume = bcdma_resume,
	.get_status = bcdma_get_status,
	.get_attribute = bcdma_get_attribute,
	.chan_filter = bcdma_chan_filter,
	.chan_release = bcdma_chan_release,
};

/* GIC interrupt IDs of the SPIs and the PPIs start at 32 and 16 */
#define BCDMA_GIC_SPI_BASE 32U
#define BCDMA_GIC_PPI_BASE 16U

/* interrupt-map entry: <ring &gic unit-address GIC_SPI|GIC_PPI irq flags priority> */
#define BCDMA_MAP_RING(node, idx) DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node, interrupt_map, idx, 0)
#define BCDMA_MAP_IRQ(node, idx)                                                                   \
	(DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node, interrupt_map, idx, 1) +                       \
	 ((DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node, interrupt_map, idx, 0) == 0)                 \
		  ? BCDMA_GIC_SPI_BASE                                                             \
		  : BCDMA_GIC_PPI_BASE))
#define BCDMA_MAP_FLAGS(node, idx)                                                                 \
	DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node, interrupt_map, idx, 2)
#define BCDMA_MAP_PRIO(node, idx) DT_MAP_ENTRY_PARENT_SPECIFIER_BY_IDX(node, interrupt_map, idx, 3)

#define BCDMA_IRQ_ENTRY(node, prop, idx, n)                                                        \
	{                                                                                          \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
		.ring = BCDMA_MAP_RING(node, idx),                                                 \
		.irq = BCDMA_MAP_IRQ(node, idx),                                                   \
	}

#define BCDMA_IRQ_CONNECT(node, prop, idx, n)                                                      \
	IRQ_CONNECT(BCDMA_MAP_IRQ(node, idx), BCDMA_MAP_PRIO(node, idx), bcdma_ring_isr,          \
		    &bcdma_irqs_##n[idx], BCDMA_MAP_FLAGS(node, idx));

#define BCDMA_INIT(n)                                                                              \
	static struct bcdma_chan_mem bcdma_mem_##n[BCDMA_NUM_BCHAN];                               \
	static struct bcdma_chan_mem bcdma_smem_##n[BCDMA_NUM_SCHAN];                              \
	static const struct bcdma_irq bcdma_irqs_##n[] = {DT_FOREACH_MAP_ENTRY_SEP_VARGS(          \
		DT_DRV_INST(n), interrupt_map, BCDMA_IRQ_ENTRY, (,), n)};                          \
	static void bcdma_irq_config_##n(void)                                                     \
	{                                                                                          \
		DT_FOREACH_MAP_ENTRY_VARGS(DT_DRV_INST(n), interrupt_map, BCDMA_IRQ_CONNECT, n)    \
	}                                                                                          \
	static struct bcdma_data bcdma_data_##n = {                                                \
		.bchan = {LISTIFY(BCDMA_NUM_BCHAN, BCDMA_CHAN_MEM_INIT, (,), n)},                  \
		.schan = {LISTIFY(BCDMA_NUM_SCHAN, BCDMA_SCHAN_MEM_INIT, (,), n)},                 \
	};                                                                                         \
	static const struct bcdma_config bcdma_config_##n = {                                      \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(gcfg, DT_DRV_INST(n)),                          \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(chanrt, DT_DRV_INST(n)),                        \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(bchanrt, DT_DRV_INST(n)),                       \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ringrt, DT_DRV_INST(n)),                        \
		.irqs = bcdma_irqs_##n,                                                            \
		.num_irqs = ARRAY_SIZE(bcdma_irqs_##n),                                            \
		.irq_config = bcdma_irq_config_##n,                                                \
		.asel = DT_INST_PROP(n, ti_asel),                                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, bcdma_init, NULL, &bcdma_data_##n, &bcdma_config_##n,            \
			      POST_KERNEL, CONFIG_DMA_INIT_PRIORITY, &bcdma_api);

#define BCDMA_CHAN_MEM_INIT(i, n)  {.mem = &bcdma_mem_##n[i]}
#define BCDMA_SCHAN_MEM_INIT(i, n) {.mem = &bcdma_smem_##n[i]}

DT_INST_FOREACH_STATUS_OKAY(BCDMA_INIT)
