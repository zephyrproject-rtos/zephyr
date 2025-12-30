/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/dma/renesas_rza2m_dma.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <zephyr/sys/mem_blocks.h>

#define DT_DRV_COMPAT renesas_rza2m_dma

#define RZA2M_DMA_CH_OFFSET      0x40
#define RZA2M_DMA_CH_8_15_OFFSET 0x400

#define RZA2M_DMA_GET_SECTION_OFFSET(n) ((n > 7) ? RZA2M_DMA_CH_8_15_OFFSET : 0)

#define RZA2M_DMA_GET_CNUM_OFFSET(n) (RZA2M_DMA_CH_OFFSET * (n & 7))

#define RZA2M_DMA_GET_CH_OFFSET(dev, n)                                                            \
	(DEVICE_MMIO_NAMED_GET(dev, reg_main) + RZA2M_DMA_GET_SECTION_OFFSET(n) +                  \
	 RZA2M_DMA_GET_CNUM_OFFSET(n))

#define RZA2M_DMA_GET_SECTION_COMMON_OFFSET(dev, n)                                                \
	(DEVICE_MMIO_NAMED_GET(dev, reg_main) + RZA2M_DMA_GET_SECTION_OFFSET(n))

#define RZA2M_DMA_N0SA(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x0)
#define RZA2M_DMA_N0DA(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x4)
#define RZA2M_DMA_N0TB(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x8)
#define RZA2M_DMA_N1SA(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0xc)
#define RZA2M_DMA_N1DA(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x10)
#define RZA2M_DMA_N1TB(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x14)
#define RZA2M_DMA_CRSA(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x18)
#define RZA2M_DMA_CRDA(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x1c)
#define RZA2M_DMA_CRTB(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x20)
#define RZA2M_DMA_CHSTAT(dev, n) (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x24)
#define RZA2M_DMA_CHCTRL(dev, n) (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x28)
#define RZA2M_DMA_CHCFG(dev, n)  (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x2c)
#define RZA2M_DMA_CHITVL(dev, n) (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x30)
#define RZA2M_DMA_CHEXT(dev, n)  (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x34)
#define RZA2M_DMA_NXLA(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x38)
#define RZA2M_DMA_CRLA(dev, n)   (RZA2M_DMA_GET_CH_OFFSET(dev, n) + 0x3c)

/* Common registers */
#define RZA2M_DMA_DCTRL(dev, n)     (RZA2M_DMA_GET_SECTION_COMMON_OFFSET(dev, n) + 0x300)
#define RZA2M_DMA_DSTAT_EN(dev, n)  (RZA2M_DMA_GET_SECTION_COMMON_OFFSET(dev, n) + 0x310)
#define RZA2M_DMA_DSTAT_ER(dev, n)  (RZA2M_DMA_GET_SECTION_COMMON_OFFSET(dev, n) + 0x314)
#define RZA2M_DMA_DSTAT_END(dev, n) (RZA2M_DMA_GET_SECTION_COMMON_OFFSET(dev, n) + 0x318)
#define RZA2M_DMA_DSTAT_TC(dev, n)  (RZA2M_DMA_GET_SECTION_COMMON_OFFSET(dev, n) + 0x31c)
#define RZA2M_DMA_DSTAT_SUS(dev, n) (RZA2M_DMA_GET_SECTION_COMMON_OFFSET(dev, n) + 0x320)

/*
 * Extended in range of 0..7 from "ext" region.
 * Reg is uint32_t with CHn in 0 -> 9 bits and CHn+1 in 16->25 bits
 */
#define RZA2M_DMA_DMARS(dev, n) (DEVICE_MMIO_NAMED_GET(dev, ext) + ((n / 0x2) * 4))

LOG_MODULE_REGISTER(dma_renesas_rza2m, CONFIG_DMA_LOG_LEVEL);

/*
 * Link mode descriptor definitions
 */
#define RZA2M_DMA_HDR_LV  BIT(0) /* Indicates whether descriptor is valid */
#define RZA2M_DMA_HDR_LE  BIT(1) /* Indicated whether link ends */
#define RZA2M_DMA_HDR_WBD BIT(2) /* Write Back Disable */
#define RZA2M_DMA_HDR_DIM BIT(3) /* Interrupt Mask */

#define RZA2M_DMAC_PRV_CHCFG_SET_DMS          (0x80000000U)
#define RZA2M_DMAC_PRV_CHCFG_SET_REN          (0x40000000U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_REN         (0x40000000U)
#define RZA2M_DMAC_PRV_CHCFG_SET_RSW          (0x20000000U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_RSW         (0x20000000U)
#define RZA2M_DMAC_PRV_CHCFG_SET_RSEL         (0x10000000U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_RSEL        (0x10000000U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_SBE         (0x08000000U)
#define RZA2M_DMAC_PRV_CHCFG_SET_DEM          (0x01000000U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_DEM         (0x01000000U)
#define RZA2M_DMAC_PRV_CHCFG_SET_TM           (0x00400000U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_DAD         (0x00200000U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_SAD         (0x00100000U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_DDS         (0x000f0000U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_SDS         (0x0000f000U)
#define RZA2M_DMAC_PRV_CHCFG_SET_AM_LEVEL     (0x00000100U)
#define RZA2M_DMAC_PRV_CHCFG_SET_AM_BUS_CYCLE (0x00000200U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_AM          (0x00000700U)
#define RZA2M_DMAC_PRV_CHCFG_SET_LVL_EDGE     (0x00000000U)
#define RZA2M_DMAC_PRV_CHCFG_SET_LVL_LEVEL    (0x00000040U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_LVL         (0x00000040U)
#define RZA2M_DMAC_PRV_CHCFG_SET_REQD_SRC     (0x00000000U)
#define RZA2M_DMAC_PRV_CHCFG_SET_REQD_DST     (0x00000008U)
#define RZA2M_DMAC_PRV_CHCFG_MASK_REQD        (0x00000008U)
#define RZA2M_DMAC_PRV_CHCFG_SHIFT_SBE        (27U)
#define RZA2M_DMAC_PRV_CHCFG_SHIFT_DAD        (21U)
#define RZA2M_DMAC_PRV_CHCFG_SHIFT_SAD        (20U)
#define RZA2M_DMAC_PRV_CHCFG_SHIFT_DDS        (16U)
#define RZA2M_DMAC_PRV_CHCFG_SHIFT_SDS        (12U)
#define RZA2M_DMAC_PRV_CHCFG_SHIFT_AM         (8U)
#define RZA2M_DMAC_PRV_CHCFG_SHIFT_REQD       (3U)
#define RZA2M_DMAC_PRV_CHCFG_SHIFT_LOEN       (4U)
#define RZA2M_DMAC_PRV_CHCFG_SHIFT_HIEN       (5U)
#define RZA2M_DMAC_PRV_CHCFG_SHIFT_LVL        (6U)

/*
 * Fields packed into the "slot" devicetree cell by the RZA2M_DMA_* macros in
 * dt-bindings/dma/renesas_rza2m_dma.h. AM, LVL and REQD are fixed properties
 * of the request source.
 */
#define RZA2M_DMA_SLOT_DMARS(slot) ((uint16_t)((slot) & 0xFFFFU))
#define RZA2M_DMA_SLOT_LVL(slot)   (((slot) & 0x10000U) != 0U)
#define RZA2M_DMA_SLOT_AM(slot)    (((slot) >> 17U) & 0x7U)
#define RZA2M_DMA_SLOT_REQD(slot)  (((slot) >> 20U) & 0x3U)

/* Values of RZA2M_DMA_SLOT_REQD() */
#define RZA2M_DMA_SLOT_REQD_AUTO (0U)
#define RZA2M_DMA_SLOT_REQD_SRC  (1U)
#define RZA2M_DMA_SLOT_REQD_DST  (2U)

/* RZA2M_DMA_CHEXT */
#define RZA2M_DMAC_PRV_CHEXT_SET_DCA_NORMAL     (0x00003000U)
#define RZA2M_DMAC_PRV_CHEXT_SET_DCA_STRONG     (0x00000000U)
#define RZA2M_DMAC_PRV_CHEXT_SET_DPR_NON_SECURE (0x00000200U)
#define RZA2M_DMAC_PRV_CHEXT_SET_SCA_NORMAL     (0x00000030U)
#define RZA2M_DMAC_PRV_CHEXT_SET_SCA_STRONG     (0x00000000U)
#define RZA2M_DMAC_PRV_CHEXT_SET_SPR_NON_SECURE (0x00000002U)

/* Address of area which is the target of setting change */
#define RZA2M_DMAC_PRV_DMA_EXTERNAL_BUS_START        (0x00000000U)
#define RZA2M_DMAC_PRV_DMA_EXTERNAL_BUS_END          (0x1FFFFFFFU)
#define RZA2M_DMAC_PRV_DMA_EXTERNAL_BUS_MIRROR_START (0x40000000U)
#define RZA2M_DMAC_PRV_DMA_EXTERNAL_BUS_MIRROR_END   (0x5FFFFFFFU)

#define RZA2M_DMA_CLRINTMSK BIT(17)
#define RZA2M_DMA_SETINTMSK BIT(16)
#define RZA2M_DMA_CLRSUS    BIT(9)
#define RZA2M_DMA_SETSUS    BIT(8)
#define RZA2M_DMA_CLRTC     BIT(6)
#define RZA2M_DMA_CLREND    BIT(5)
#define RZA2M_DMA_CLRRQ     BIT(4)
#define RZA2M_DMA_SWRST     BIT(3)
#define RZA2M_DMA_STG       BIT(2)
#define RZA2M_DMA_CLREN     BIT(1)
#define RZA2M_DMA_SETEN     BIT(0)
#define RZA2M_DMA_CHCTRL_CLEAR                                                                     \
	(RZA2M_DMA_CLRINTMSK | RZA2M_DMA_CLRSUS | RZA2M_DMA_CLRTC | RZA2M_DMA_CLREND |             \
	 RZA2M_DMA_CLRRQ | RZA2M_DMA_SWRST | RZA2M_DMA_CLREN)

#define RZA2M_DMAC_PRV_CHSTAT_MASK_DER  (BIT(10))
#define RZA2M_DMAC_PRV_CHSTAT_MASK_SR   (BIT(7))
#define RZA2M_DMAC_PRV_CHSTAT_MASK_END  (BIT(5))
#define RZA2M_DMAC_PRV_CHSTAT_MASK_ER   (BIT(4))
#define RZA2M_DMAC_PRV_CHSTAT_MASK_SUS  (BIT(3))
#define RZA2M_DMAC_PRV_CHSTAT_MASK_TACT (BIT(2))
#define RZA2M_DMAC_PRV_CHSTAT_MASK_EN   (BIT(0))

#define RZA2M_DMA_IS_SET(value, mask) (!!((value) & (mask)))

#define RZA2M_DMA_TIMEOUT_US 1000

struct dma_rza2m_link_descriptor {
	uint32_t header;
	uint32_t src_addr;
	uint32_t dest_addr;
	uint32_t trans_byte;
	uint32_t config;
	uint32_t interval;
	uint32_t extension;
	uint32_t next_link_address;
};

struct dma_rza2m_config {
	DEVICE_MMIO_NAMED_ROM(reg_main);
	DEVICE_MMIO_NAMED_ROM(ext);

	uint8_t num_channels;
	void (*irq_configure)(void);
	const struct pinctrl_dev_config *pcfg;
	uint32_t addr_alignment;
};

enum dma_rza2m_channel_mode {
	DMA_RZA2M_REGISTER_MODE = 0,
	DMA_RZA2M_LINK_MODE
};

struct dma_rza2m_channel {
	int sw_trigger;
	bool busy;
	dma_callback_t dma_callback;
	void *user_data;
	int err_callback_en;
	int complete_callback_en;
	enum dma_rza2m_channel_mode mode;
	struct dma_rza2m_link_descriptor *descrs;
	uint32_t total_bytes;
	uint32_t direction;
	struct k_spinlock lock;
};

BUILD_ASSERT(CONFIG_DMA_RZA2M_DESCRS_CHUNKS <= 32,
	     "CONFIG_DMA_RZA2M_DESCRS_CHUNKS > 32 is not supported\n");

struct dma_rza2m_data {
	/* Dma context should be the first in data structure */
	struct dma_context ctx;

	DEVICE_MMIO_NAMED_RAM(reg_main);
	DEVICE_MMIO_NAMED_RAM(ext);

	struct dma_rza2m_channel *channels;

	sys_mem_blocks_t *descr_pool;
};

#define DEV_DATA(_dev) ((struct dma_rza2m_data *)((_dev)->data))
#define DEV_CFG(_dev)  ((struct dma_rza2m_config *)((_dev)->config))

static int rza2m_dma_get_ch_sus(const struct device *dev, int ch)
{
	/* DSTAT_SUS holds one bit per channel of the 8 channel group */
	return sys_read32(RZA2M_DMA_DSTAT_SUS(dev, ch)) & BIT(ch & 7);
}

static void rza2m_dma_set_dmars(const struct device *dev, int ch, uint32_t value)
{
	uint32_t dmars32;

	dmars32 = sys_read32(RZA2M_DMA_DMARS(dev, ch));
	if (ch & 1) {
		dmars32 &= 0x0000FFFFU;
		dmars32 |= value << 16;
	} else {
		dmars32 &= 0xFFFF0000U;
		dmars32 |= value;
	}

	sys_write32(dmars32, RZA2M_DMA_DMARS(dev, ch));
}

static int dma_get_data_size(uint32_t data_size)
{
	switch (data_size) {
	case 1:
		return 0;
	case 2:
		return 1;
	case 4:
		return 2;
	case 8:
		return 3;
	case 16:
		return 4;
	case 32:
		return 5;
	case 64:
		return 6;
	case 128:
		return 7;
	default:
		return -EINVAL;
	}
}

static int rza2m_dma_get_descrs(const struct device *dev, struct dma_rza2m_link_descriptor **descr)
{
	struct dma_rza2m_data *data = dev->data;
	void *blk;
	int ret;

	ret = sys_mem_blocks_alloc(data->descr_pool, 1, &blk);
	if (ret != 0) {
		*descr = NULL;
		return ret;
	}

	*descr = (struct dma_rza2m_link_descriptor *)blk;
	return 0;
}

static void rza2m_dma_put_descrs(const struct device *dev, struct dma_rza2m_link_descriptor *descr)
{
	struct dma_rza2m_data *data = dev->data;

	if (descr == NULL) {
		return;
	}
	(void)sys_mem_blocks_free(data->descr_pool, 1, (void **)&descr);
}

static bool rza2m_dma_not_aligned(const struct device *dev, uint32_t addr)
{
	const struct dma_rza2m_config *cfg = dev->config;

	if (cfg->addr_alignment == 0) {
		return false;
	}

	return !!(addr & (cfg->addr_alignment - 1));
}

static int rza2m_dma_construct_link_chain(const struct device *dev, struct dma_config *dma_cfg,
					  struct dma_rza2m_channel *chan, uint32_t ch_cfg)
{
	struct dma_block_config *block = dma_cfg->head_block;
	uint32_t total_bytes = 0;
	int ret;

	__ASSERT(chan, "Channel should be provided");

	if (!block) {
		LOG_ERR("No dma blocks were provided");
		return -EINVAL;
	}

	ret = rza2m_dma_get_descrs(dev, &chan->descrs);
	if (ret != 0) {
		LOG_ERR("%s: unable to allocate descriptor list", __func__);
		return ret;
	}

	for (int i = 0; i < dma_cfg->block_count; i++) {
		/* Check source_address and dest_address alignment */
		if (rza2m_dma_not_aligned(dev, block->dest_address) ||
		    rza2m_dma_not_aligned(dev, block->source_address)) {
			LOG_ERR("%s: buffers are not properly aligned", __func__);
			rza2m_dma_put_descrs(dev, chan->descrs);
			chan->descrs = NULL;
			return -EINVAL;
		}

		chan->descrs[i].src_addr = K_MEM_PHYS_ADDR(block->source_address);
		chan->descrs[i].dest_addr = K_MEM_PHYS_ADDR(block->dest_address);
		chan->descrs[i].trans_byte = block->block_size;
		chan->descrs[i].interval = 0;
		chan->descrs[i].config = ch_cfg;
		chan->descrs[i].header = RZA2M_DMA_HDR_LV;

		/* DMA controller supports only 1 interval so apply the bigger one which enabled */
		if (block->source_gather_en) {
			chan->descrs[i].interval = block->source_gather_interval;
		}

		if (block->dest_scatter_en &&
		    (chan->descrs[i].interval < block->dest_scatter_interval)) {
			chan->descrs[i].interval = block->dest_scatter_interval;
		}

		/* If this is the last block */
		if (i == (dma_cfg->block_count - 1)) {
			chan->descrs[i].header = RZA2M_DMA_HDR_LV | RZA2M_DMA_HDR_LE;
			chan->descrs[i].next_link_address = 0;
		} else {
			chan->descrs[i].next_link_address =
				K_MEM_PHYS_ADDR((uint32_t)&chan->descrs[i + 1]);
		}

		total_bytes += block->block_size;
		block = block->next_block;
	}

	chan->total_bytes = total_bytes;

	return 0;
}

static void rza2m_dma_channel_free(const struct device *dev, int ch_num)
{
	struct dma_rza2m_data *data = dev->data;
	struct dma_rza2m_channel *ch = &data->channels[ch_num];

	ch->busy = false;
	rza2m_dma_put_descrs(dev, ch->descrs);
	ch->descrs = NULL;

	/* If peripheral is selected by a RZA2M_DMA_DMARS register, the interrupt requests
	 * are masked. So, we must clear DMAS register after complete.
	 */
	rza2m_dma_set_dmars(dev, ch_num, 0);
}

static inline int rza2m_dma_check_ch(const struct device *dev, uint32_t ch)
{
	const struct dma_rza2m_config *cfg = dev->config;

	if (ch >= cfg->num_channels) {
		LOG_ERR("Channel must be < %" PRIu32 " (%" PRIu32 ")", cfg->num_channels, ch);
		return -EINVAL;
	}

	return 0;
}

static int dma_rza2m_configure_chcfg(const struct device *dev, uint32_t channel,
				     struct dma_config *dma_cfg, uint32_t *chcfg)
{
	uint32_t slot = dma_cfg->dma_slot;
	int ret;

	/* Set sel */
	*chcfg |= (channel & 0x7u);

	if (dma_cfg->head_block->source_addr_adj != DMA_ADDR_ADJ_INCREMENT) {
		*chcfg |= RZA2M_DMAC_PRV_CHCFG_MASK_SAD;
	}

	if (dma_cfg->head_block->dest_addr_adj != DMA_ADDR_ADJ_INCREMENT) {
		*chcfg |= RZA2M_DMAC_PRV_CHCFG_MASK_DAD;
	}

	ret = dma_get_data_size(dma_cfg->dest_data_size);
	if (ret < 0) {
		LOG_ERR("Invalid dest_data_size %d\n", dma_cfg->dest_data_size);
		return ret;
	}

	*chcfg |= ((ret << RZA2M_DMAC_PRV_CHCFG_SHIFT_DDS) & RZA2M_DMAC_PRV_CHCFG_MASK_DDS);

	ret = dma_get_data_size(dma_cfg->source_data_size);
	if (ret < 0) {
		LOG_ERR("Invalid source_data_size %d\n", dma_cfg->source_data_size);
		return ret;
	}

	*chcfg |= ((ret << RZA2M_DMAC_PRV_CHCFG_SHIFT_SDS) & RZA2M_DMAC_PRV_CHCFG_MASK_SDS);

	if (dma_cfg->channel_direction == MEMORY_TO_MEMORY) {
		/* Software triggered transfer, block mode with bus cycle acknowledge */
		*chcfg |= RZA2M_DMAC_PRV_CHCFG_SET_AM_BUS_CYCLE | RZA2M_DMAC_PRV_CHCFG_SET_TM;
		*chcfg |= RZA2M_DMAC_PRV_CHCFG_SET_LVL_LEVEL;
		*chcfg |= RZA2M_DMAC_PRV_CHCFG_SET_REQD_DST;
	} else {
		/* AM and LVL are fixed by the request source, carried by the slot */
		*chcfg |= (RZA2M_DMA_SLOT_AM(slot) << RZA2M_DMAC_PRV_CHCFG_SHIFT_AM) &
			  RZA2M_DMAC_PRV_CHCFG_MASK_AM;

		if (RZA2M_DMA_SLOT_LVL(slot)) {
			*chcfg |= RZA2M_DMAC_PRV_CHCFG_SET_LVL_LEVEL;
		}

		/*
		 * Request sources that can only drive one side of the transfer pin
		 * REQD in the slot, the rest follow the requested direction.
		 */
		switch (RZA2M_DMA_SLOT_REQD(slot)) {
		case RZA2M_DMA_SLOT_REQD_SRC:
			*chcfg |= RZA2M_DMAC_PRV_CHCFG_SET_REQD_SRC;
			break;
		case RZA2M_DMA_SLOT_REQD_DST:
			*chcfg |= RZA2M_DMAC_PRV_CHCFG_SET_REQD_DST;
			break;
		case RZA2M_DMA_SLOT_REQD_AUTO:
		default:
			if (dma_cfg->channel_direction == PERIPHERAL_TO_MEMORY) {
				*chcfg |= RZA2M_DMAC_PRV_CHCFG_SET_REQD_DST;
			} else {
				*chcfg |= RZA2M_DMAC_PRV_CHCFG_SET_REQD_SRC;
			}
			break;
		}

		/* HIEN 1 is set for all HW slots */
		*chcfg |= 1 << RZA2M_DMAC_PRV_CHCFG_SHIFT_HIEN;
	}

	if ((*chcfg & RZA2M_DMAC_PRV_CHCFG_MASK_REQD) == RZA2M_DMAC_PRV_CHCFG_SET_REQD_SRC) {
		*chcfg |= 1 << RZA2M_DMAC_PRV_CHCFG_SHIFT_SBE; /* Takes effect only when reqd = 0 */
	}

	/* Configure for link mode or register mode */
	if (dma_cfg->block_count > 1) {
		/* Link Mode configuration */
		*chcfg |= RZA2M_DMAC_PRV_CHCFG_SET_DMS;
	} else {
		/* Register Mode configuration */
		*chcfg &= ~RZA2M_DMAC_PRV_CHCFG_SET_DMS;  /* Set register mode */
		*chcfg &= ~RZA2M_DMAC_PRV_CHCFG_SET_REN;  /* Do not switch register set */
		*chcfg &= ~RZA2M_DMAC_PRV_CHCFG_SET_RSEL; /* Select 0 register set */
		*chcfg &= ~RZA2M_DMAC_PRV_CHCFG_SET_DEM;  /* Unmask DMA end interrupt */
		*chcfg &= ~RZA2M_DMAC_PRV_CHCFG_SET_RSW;  /* No automatic register change */
	}

	return 0;
}

static uint32_t dma_rza2m_configure_chext(struct dma_config *dma_cfg)
{
	uint32_t phys_addr;
	uint32_t chext;

	chext = (RZA2M_DMAC_PRV_CHEXT_SET_DPR_NON_SECURE | RZA2M_DMAC_PRV_CHEXT_SET_SPR_NON_SECURE);

	phys_addr = K_MEM_PHYS_ADDR(dma_cfg->head_block->source_address);
	/* Set bus parameter for source */
	if ((phys_addr <= RZA2M_DMAC_PRV_DMA_EXTERNAL_BUS_END) ||
	    ((phys_addr >= RZA2M_DMAC_PRV_DMA_EXTERNAL_BUS_MIRROR_START) &&
	     (phys_addr <= RZA2M_DMAC_PRV_DMA_EXTERNAL_BUS_MIRROR_END))) {
		chext |= RZA2M_DMAC_PRV_CHEXT_SET_SCA_NORMAL;
	} else {
		chext |= RZA2M_DMAC_PRV_CHEXT_SET_SCA_STRONG;
	}

	phys_addr = K_MEM_PHYS_ADDR(dma_cfg->head_block->dest_address);
	/* Set bus parameter for destination */
	if ((phys_addr <= RZA2M_DMAC_PRV_DMA_EXTERNAL_BUS_END) ||
	    ((phys_addr >= RZA2M_DMAC_PRV_DMA_EXTERNAL_BUS_MIRROR_START) &&
	     (phys_addr <= RZA2M_DMAC_PRV_DMA_EXTERNAL_BUS_MIRROR_END))) {
		chext |= RZA2M_DMAC_PRV_CHEXT_SET_DCA_NORMAL;
	} else {
		chext |= RZA2M_DMAC_PRV_CHEXT_SET_DCA_STRONG;
	}

	return chext;
}

static int dma_rza2m_validate_config(struct dma_config *dma_cfg)
{
	if (dma_cfg == NULL || dma_cfg->head_block == NULL || dma_cfg->block_count == 0) {
		LOG_ERR("Invalid DMA config");
		return -EINVAL;
	}

	if (dma_cfg->block_count > CONFIG_DMA_RZA2M_MAX_DESCRS) {
		LOG_ERR("block_count %u exceeds max %d", dma_cfg->block_count,
			CONFIG_DMA_RZA2M_MAX_DESCRS);
		return -EINVAL;
	}

	if (dma_cfg->channel_direction != MEMORY_TO_MEMORY &&
	    RZA2M_DMA_SLOT_DMARS(dma_cfg->dma_slot) == 0U) {
		LOG_ERR("dma_slot 0x%x carries no DMARS request code", dma_cfg->dma_slot);
		return -EINVAL;
	}

	if (dma_cfg->source_chaining_en || dma_cfg->dest_chaining_en) {
		LOG_ERR("Channel chaining is not supported");
		return -EINVAL;
	}

	if (dma_cfg->channel_direction > PERIPHERAL_TO_MEMORY) {
		LOG_ERR("Channel_direction must be MEMORY_TO_MEMORY, MEMORY_TO_PERIPHERAL or "
			"PERIPHERAL_TO_MEMORY (%" PRIu32 ")",
			dma_cfg->channel_direction);
		return -ENOTSUP;
	}

	if ((dma_cfg->head_block->source_addr_adj != DMA_ADDR_ADJ_INCREMENT) &&
	    (dma_cfg->head_block->source_addr_adj != DMA_ADDR_ADJ_NO_CHANGE)) {
		LOG_ERR("Invalid source_addr_adj %" PRIu16, dma_cfg->head_block->source_addr_adj);
		return -ENOTSUP;
	}

	if ((dma_cfg->head_block->dest_addr_adj != DMA_ADDR_ADJ_INCREMENT) &&
	    (dma_cfg->head_block->dest_addr_adj != DMA_ADDR_ADJ_NO_CHANGE)) {
		LOG_ERR("Invalid dest_addr_adj %" PRIu16, dma_cfg->head_block->dest_addr_adj);
		return -ENOTSUP;
	}

	return 0;
}

static int dma_rza2m_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			    size_t size)
{
	struct dma_rza2m_data *data = dev->data;
	uint32_t chcfg;
	uint32_t chcfg_sds;
	uint32_t data_size;
	k_spinlock_key_t key;
	int ret;

	ret = rza2m_dma_check_ch(dev, channel);
	if (ret < 0) {
		return ret;
	}

	if (size == 0) {
		LOG_ERR("Invalid transfer size: %zu", size);
		return -EINVAL;
	}

	key = k_spin_lock(&data->channels[channel].lock);

	if (data->channels[channel].mode == DMA_RZA2M_LINK_MODE) {
		LOG_ERR("Unsupported reload function for LINK_MODE");
		ret = -ENOTSUP;
		goto unlock;
	}

	/* Get current transfer data size from CHCFG register */
	chcfg = sys_read32(RZA2M_DMA_CHCFG(dev, channel));
	chcfg_sds = ((chcfg & RZA2M_DMAC_PRV_CHCFG_MASK_SDS) >> RZA2M_DMAC_PRV_CHCFG_SHIFT_SDS);
	data_size = (1 << chcfg_sds);

	if (size % data_size != 0) {
		LOG_ERR("Transfer size %zu not aligned to data size %u", size, data_size);
		ret = -EINVAL;
		goto unlock;
	}

	/* Update transfer parameters in register mode */
	sys_write32(K_MEM_PHYS_ADDR(src), RZA2M_DMA_N0SA(dev, channel));
	sys_write32(K_MEM_PHYS_ADDR(dst), RZA2M_DMA_N0DA(dev, channel));
	sys_write32(size, RZA2M_DMA_N0TB(dev, channel));

	data->channels[channel].total_bytes = size;

unlock:
	k_spin_unlock(&data->channels[channel].lock, key);

	return ret;
}

static bool dma_rza2m_channel_filter(const struct device *dev, int ch, void *filter_param)
{
	ARG_UNUSED(filter_param);

	const struct dma_rza2m_config *config = dev->config;

	if (ch >= config->num_channels) {
		LOG_ERR("Channel must be < %" PRIu32 " (%" PRIu32 ")", config->num_channels, ch);
		return false;
	}

	return true;
}

static void dma_rza2m_channel_release(const struct device *dev, uint32_t ch)
{
	const struct dma_rza2m_config *config = dev->config;
	struct dma_rza2m_data *data = dev->data;
	k_spinlock_key_t key;

	/* Validate channel */
	if (ch >= config->num_channels) {
		LOG_ERR("Channel must be < %" PRIu32 " (%" PRIu32 ")", config->num_channels, ch);
		return;
	}

	key = k_spin_lock(&data->channels[ch].lock);

	rza2m_dma_set_dmars(dev, ch, 0);

	/* Disable DMA transfers on this channel */
	sys_write32(RZA2M_DMA_CLREN, RZA2M_DMA_CHCTRL(dev, ch));

	/* Channel needs to be inactive */
	if (!WAIT_FOR(!RZA2M_DMA_IS_SET(sys_read32(RZA2M_DMA_CHSTAT(dev, ch)),
					RZA2M_DMAC_PRV_CHSTAT_MASK_TACT),
		      RZA2M_DMA_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("Timed out while waiting for chan %d to become inactive", ch);
		k_spin_unlock(&data->channels[ch].lock, key);

		return;
	}

	/* Reset channel */
	sys_write32(RZA2M_DMA_SWRST, RZA2M_DMA_CHCTRL(dev, ch));

	/* Free descriptor resources if in link mode */
	if (data->channels[ch].mode == DMA_RZA2M_LINK_MODE) {
		rza2m_dma_channel_free(dev, ch);
	}

	k_spin_unlock(&data->channels[ch].lock, key);
}

static int dma_rza2m_config(const struct device *dev, uint32_t channel, struct dma_config *dma_cfg)
{
	struct dma_rza2m_data *data = dev->data;
	uint32_t channel_cfg;
	uint32_t channel_ext;
	uint32_t dctrl_cfg;
	uint32_t interval;
	k_spinlock_key_t key;
	int ret;

	/* Validate configuration parameters */
	ret = dma_rza2m_validate_config(dma_cfg);
	if (ret < 0) {
		return ret;
	}

	ret = rza2m_dma_check_ch(dev, channel);
	if (ret < 0) {
		return ret;
	}

	key = k_spin_lock(&data->channels[channel].lock);

	if (data->channels[channel].busy) {
		ret = -EBUSY;
		goto unlock;
	}

	data->channels[channel].direction = dma_cfg->channel_direction;
	if (dma_cfg->channel_direction == MEMORY_TO_MEMORY) {
		if (dma_cfg->dma_slot != RZA2M_DMA_MEM_2_MEM) {
			LOG_ERR("Invalid slot for MEM_2_MEM direction");
		}

		/* Set sw_trigger so transfer will start on Channel Enable */
		data->channels[channel].sw_trigger = 1;
	}

	/* Configure channel_cfg register */
	channel_cfg = 0;
	ret = dma_rza2m_configure_chcfg(dev, channel, dma_cfg, &channel_cfg);
	if (ret < 0) {
		goto unlock;
	}

	if (dma_cfg->block_count > 1) {
		data->channels[channel].mode = DMA_RZA2M_LINK_MODE;

		/*
		 * IMPORTANT: channel_cfg value should be set for each descriptor in the linked
		 * list. So current channel_configuration has effect only on start stage. After
		 * loading the descriptor descr->config value will be loaded to RZA2M_DMA_CHCFG
		 * register.
		 */
		ret = rza2m_dma_construct_link_chain(dev, dma_cfg, &data->channels[channel],
						     channel_cfg);
		if (ret < 0) {
			goto unlock;
		}

		sys_write32(K_MEM_PHYS_ADDR((uint32_t)&data->channels[channel].descrs[0]),
			    RZA2M_DMA_NXLA(dev, channel));
	} else {
		data->channels[channel].mode = DMA_RZA2M_REGISTER_MODE;

		/* Check source_address and dest_address alignment */
		if (rza2m_dma_not_aligned(dev, dma_cfg->head_block->dest_address) ||
		    rza2m_dma_not_aligned(dev, dma_cfg->head_block->source_address)) {
			LOG_ERR("%s: buffers are not properly aligned", __func__);
			ret = -EINVAL;
			goto unlock;
		}

		sys_write32(K_MEM_PHYS_ADDR(dma_cfg->head_block->source_address),
			    RZA2M_DMA_N0SA(dev, channel));
		sys_write32(K_MEM_PHYS_ADDR(dma_cfg->head_block->dest_address),
			    RZA2M_DMA_N0DA(dev, channel));
		sys_write32(dma_cfg->head_block->block_size, RZA2M_DMA_N0TB(dev, channel));
		data->channels[channel].total_bytes = dma_cfg->head_block->block_size;
	}

	if (dma_cfg->head_block->source_gather_en) {
		interval = dma_cfg->head_block->source_gather_interval;
	} else if (dma_cfg->head_block->dest_scatter_en) {
		interval = dma_cfg->head_block->dest_scatter_interval;
	} else {
		interval = 0;
	}

	/* 0 for Fixed Priority, Round Robin otherwise */
	dctrl_cfg = (dma_cfg->channel_priority == 0);

	/* Configure channel_ext register */
	channel_ext = dma_rza2m_configure_chext(dma_cfg);

	sys_write32(channel_cfg, RZA2M_DMA_CHCFG(dev, channel));
	sys_write32(dctrl_cfg, RZA2M_DMA_DCTRL(dev, channel));
	sys_write32(interval, RZA2M_DMA_CHITVL(dev, channel));
	sys_write32(channel_ext, RZA2M_DMA_CHEXT(dev, channel));

	/* DMARS only selects a peripheral request, clear it for software triggers */
	if (dma_cfg->channel_direction == MEMORY_TO_MEMORY) {
		rza2m_dma_set_dmars(dev, channel, 0);
	} else {
		rza2m_dma_set_dmars(dev, channel, RZA2M_DMA_SLOT_DMARS(dma_cfg->dma_slot));
	}

	data->channels[channel].dma_callback = dma_cfg->dma_callback;
	data->channels[channel].user_data = dma_cfg->user_data;
	data->channels[channel].err_callback_en = dma_cfg->error_callback_dis;
	data->channels[channel].complete_callback_en = dma_cfg->complete_callback_en;

	/* Clear status */
	sys_write32(RZA2M_DMA_SWRST, RZA2M_DMA_CHCTRL(dev, channel));
unlock:
	k_spin_unlock(&data->channels[channel].lock, key);

	return ret;
}

static int dma_rza2m_start(const struct device *dev, uint32_t ch)
{
	struct dma_rza2m_data *data = dev->data;
	uint32_t stat;
	k_spinlock_key_t key;
	int ret;

	ret = rza2m_dma_check_ch(dev, ch);
	if (ret < 0) {
		return ret;
	}

	key = k_spin_lock(&data->channels[ch].lock);
	if (data->channels[ch].busy) {
		ret = -EBUSY;
		goto unlock;
	}

	stat = sys_read32(RZA2M_DMA_CHSTAT(dev, ch));
	if (RZA2M_DMA_IS_SET(stat, RZA2M_DMAC_PRV_CHSTAT_MASK_EN) ||
	    RZA2M_DMA_IS_SET(stat, RZA2M_DMAC_PRV_CHSTAT_MASK_TACT)) {
		ret = -EBUSY;
		goto unlock;
	}

	/* Clear status */
	sys_write32(RZA2M_DMA_SWRST, RZA2M_DMA_CHCTRL(dev, ch));

	data->channels[ch].busy = true;

	/* Enable Channel */
	if (data->channels[ch].sw_trigger) {
		sys_write32(RZA2M_DMA_STG | RZA2M_DMA_SETEN, RZA2M_DMA_CHCTRL(dev, ch));
	} else {
		sys_write32(RZA2M_DMA_SETEN, RZA2M_DMA_CHCTRL(dev, ch));
	}

unlock:
	k_spin_unlock(&data->channels[ch].lock, key);

	return ret;
}

static int dma_rza2m_stop(const struct device *dev, uint32_t ch)
{
	struct dma_rza2m_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	ret = rza2m_dma_check_ch(dev, ch);
	if (ret < 0) {
		return ret;
	}

	key = k_spin_lock(&data->channels[ch].lock);

	if (!data->channels[ch].busy) {
		ret = 0;
		goto unlock;
	}

	/* Channel needs to be suspended */
	sys_write32(RZA2M_DMA_SETSUS, RZA2M_DMA_CHCTRL(dev, ch));

	if (!WAIT_FOR(RZA2M_DMA_IS_SET(sys_read32(RZA2M_DMA_CHSTAT(dev, ch)),
				       RZA2M_DMAC_PRV_CHSTAT_MASK_SUS),
		      RZA2M_DMA_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("Timed out while waiting for chan %d to become suspended", ch);
		ret = -EBUSY;
		goto unlock;
	}

	sys_write32(RZA2M_DMA_CLREN, RZA2M_DMA_CHCTRL(dev, ch));

	if (!WAIT_FOR(!RZA2M_DMA_IS_SET(sys_read32(RZA2M_DMA_CHSTAT(dev, ch)),
					RZA2M_DMAC_PRV_CHSTAT_MASK_TACT),
		      RZA2M_DMA_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("Timed out while waiting for chan %d stop", ch);
		ret = -EBUSY;
		goto unlock;
	}

	rza2m_dma_channel_free(dev, ch);

	sys_write32(RZA2M_DMA_SWRST, RZA2M_DMA_CHCTRL(dev, ch));

unlock:
	k_spin_unlock(&data->channels[ch].lock, key);

	return ret;
}

static int dma_rza2m_suspend(const struct device *dev, uint32_t ch)
{
	struct dma_rza2m_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	ret = rza2m_dma_check_ch(dev, ch);
	if (ret < 0) {
		return ret;
	}

	key = k_spin_lock(&data->channels[ch].lock);

	if (!data->channels[ch].busy) {
		ret = -EINVAL;
		goto unlock;
	}

	sys_write32(RZA2M_DMA_SETSUS, RZA2M_DMA_CHCTRL(dev, ch));

	/* Channel needs to be suspended */
	if (!WAIT_FOR(RZA2M_DMA_IS_SET(sys_read32(RZA2M_DMA_CHSTAT(dev, ch)),
				       RZA2M_DMAC_PRV_CHSTAT_MASK_SUS),
		      RZA2M_DMA_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("Timed out while waiting for chan %d to become suspended", ch);
		ret = -EBUSY;
		goto unlock;
	}

unlock:
	k_spin_unlock(&data->channels[ch].lock, key);

	return ret;
}

static int dma_rza2m_resume(const struct device *dev, uint32_t ch)
{
	struct dma_rza2m_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	ret = rza2m_dma_check_ch(dev, ch);
	if (ret < 0) {
		return ret;
	}

	key = k_spin_lock(&data->channels[ch].lock);

	if (!data->channels[ch].busy) {
		ret = -EINVAL;
	} else if (!rza2m_dma_get_ch_sus(dev, ch)) {
		ret = -EINVAL;
	} else {
		sys_write32(RZA2M_DMA_CLRSUS, RZA2M_DMA_CHCTRL(dev, ch));
	}

	k_spin_unlock(&data->channels[ch].lock, key);

	return ret;
}

static uint32_t rza2m_dma_get_copied_bytes(struct dma_rza2m_channel *chan,
					   struct dma_rza2m_link_descriptor *descr)
{
	struct dma_rza2m_link_descriptor *cur = chan->descrs;
	uint32_t copied_bytes = 0;
	bool found = false;

	while (cur) {
		if (cur == descr) {
			found = true;
			break;
		}
		copied_bytes += cur->trans_byte;
		cur = (struct dma_rza2m_link_descriptor *)K_MEM_VIRT_ADDR(cur->next_link_address);
	}

	if (!found) {
		LOG_ERR("Current link descriptor lays outside of the channel descriptor list");
		copied_bytes = 0;
	}

	return copied_bytes;
}

static struct dma_rza2m_link_descriptor *
rza2m_dma_get_previous_descr(struct dma_rza2m_channel *chan,
			     struct dma_rza2m_link_descriptor *descr)
{
	struct dma_rza2m_link_descriptor *cur = chan->descrs;
	struct dma_rza2m_link_descriptor *prev = NULL;

	/* If current descriptor is the first one, there's no previous */
	if (cur == descr) {
		return NULL;
	}

	/* Traverse the list to find the previous descriptor */
	while (cur) {
		/* Get next descriptor */
		struct dma_rza2m_link_descriptor *next =
			(struct dma_rza2m_link_descriptor *)K_MEM_VIRT_ADDR(cur->next_link_address);

		/* If next is our target, current is the previous */
		if (next == descr) {
			prev = cur;
			break;
		}

		/* Move to next descriptor */
		cur = next;
	}

	if (!prev) {
		LOG_ERR("Previous descriptor not found in the channel descriptor list");
	}

	return prev;
}

static int dma_rza2m_get_status(const struct device *dev, uint32_t ch, struct dma_status *stat)
{
	struct dma_rza2m_data *data = dev->data;
	struct dma_rza2m_link_descriptor *link;
	uint32_t channel_cfg;
	int ret;

	ret = rza2m_dma_check_ch(dev, ch);
	if (ret < 0) {
		return ret;
	}

	channel_cfg = sys_read32(RZA2M_DMA_CHCFG(dev, ch));

	stat->busy = data->channels[ch].busy;

	stat->dir = data->channels[ch].direction;

	if (RZA2M_DMA_IS_SET(channel_cfg, RZA2M_DMAC_PRV_CHCFG_SET_DMS)) {
		/* For link mode calculate already processed blocks */
		link = (struct dma_rza2m_link_descriptor *)K_MEM_VIRT_ADDR(
			sys_read32(RZA2M_DMA_CRLA(dev, ch)));
		if (link) {
			stat->total_copied = rza2m_dma_get_copied_bytes(&data->channels[ch], link);
		} else {
			stat->total_copied = 0;
		}

		stat->pending_length = data->channels[ch].total_bytes - stat->total_copied;
	} else {
		/* For register mode get data from RZA2M_DMA_CRTB register */
		stat->pending_length = sys_read32(RZA2M_DMA_CRTB(dev, ch));
		stat->total_copied = data->channels[ch].total_bytes - stat->pending_length;
	}

	return 0;
}

static int dma_rza2m_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	const struct dma_rza2m_config *cfg = dev->config;

	switch (type) {
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = cfg->addr_alignment;
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = CONFIG_DMA_RZA2M_MAX_DESCRS;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(dma, dma_renesas_rza2m_driver_api) = {
	.reload = dma_rza2m_reload,
	.config = dma_rza2m_config,
	.start = dma_rza2m_start,
	.stop = dma_rza2m_stop,
	.get_status = dma_rza2m_get_status,
	.suspend = dma_rza2m_suspend,
	.resume = dma_rza2m_resume,
	.get_attribute = dma_rza2m_get_attribute,
	.chan_filter = dma_rza2m_channel_filter,
	.chan_release = dma_rza2m_channel_release,
};

#define RZA2M_DMA_LINK_TR_END        1
#define RZA2M_DMA_LINK_TR_INPROGRESS 0

static int rza2m_dma_process_link(const struct device *dev, int ch, uint32_t stat)
{
	struct dma_rza2m_data *data = dev->data;
	struct dma_rza2m_link_descriptor *link;
	struct dma_rza2m_link_descriptor *prev_link;
	uint32_t crla;

	if (data->channels[ch].mode != DMA_RZA2M_LINK_MODE) {
		return -EINVAL;
	}

	crla = sys_read32(RZA2M_DMA_CRLA(dev, ch));
	link = (struct dma_rza2m_link_descriptor *)K_MEM_VIRT_ADDR(crla);
	if (!link) {
		return -EIO;
	}

	prev_link = rza2m_dma_get_previous_descr(&data->channels[ch], link);
	if (!prev_link) {
		return -EIO;
	}
	if (prev_link->header & RZA2M_DMA_HDR_LV) {
		/*  Write-back failed */
		return -EIO;
	}

	if ((link->header & RZA2M_DMA_HDR_LE) && (!(link->header & RZA2M_DMA_HDR_LV))) {
		/* Last transfer succeeded */
		return RZA2M_DMA_LINK_TR_END;
	}

	if (RZA2M_DMA_IS_SET(stat, RZA2M_DMAC_PRV_CHSTAT_MASK_TACT) &&
	    !RZA2M_DMA_IS_SET(stat, RZA2M_DMAC_PRV_CHSTAT_MASK_DER) &&
	    (data->channels[ch].sw_trigger)) {
		sys_write32(RZA2M_DMA_STG, RZA2M_DMA_CHCTRL(dev, ch));
	}

	return RZA2M_DMA_LINK_TR_INPROGRESS;
}

static int dma_rza2m_isr_handle_register_mode(const struct device *dev, int ch, uint32_t stat)
{
	struct dma_rza2m_data *data = dev->data;

	if (RZA2M_DMA_IS_SET(stat, RZA2M_DMAC_PRV_CHSTAT_MASK_ER)) {
		return -EIO;
	}

	if (!RZA2M_DMA_IS_SET(stat, RZA2M_DMAC_PRV_CHSTAT_MASK_END) ||
	    RZA2M_DMA_IS_SET(stat, RZA2M_DMAC_PRV_CHSTAT_MASK_EN)) {
		sys_write32(RZA2M_DMA_CHCTRL_CLEAR, RZA2M_DMA_CHCTRL(dev, ch));
		rza2m_dma_channel_free(dev, ch);

		if ((data->channels[ch].err_callback_en == 0) && data->channels[ch].dma_callback) {
			data->channels[ch].dma_callback(dev, data->channels[ch].user_data, ch,
							-EIO);
		}

		return 0;
	}

	rza2m_dma_channel_free(dev, ch);
	if ((data->channels[ch].complete_callback_en == 0) && data->channels[ch].dma_callback) {
		data->channels[ch].dma_callback(dev, data->channels[ch].user_data, ch,
						DMA_STATUS_COMPLETE);
	}

	return 0;
}

static int dma_rza2m_isr_handle_link_mode(const struct device *dev, int ch, uint32_t stat)
{
	struct dma_rza2m_data *data = dev->data;
	uint32_t chctrl;
	int ret;

	if (!RZA2M_DMA_IS_SET(stat, RZA2M_DMAC_PRV_CHSTAT_MASK_END) ||
	    RZA2M_DMA_IS_SET(stat, RZA2M_DMAC_PRV_CHSTAT_MASK_DER)) {
		return -EIO;
	}

	ret = rza2m_dma_process_link(dev, ch, stat);
	if (ret < 0) {
		return ret;
	}

	/* Send callback for block if needed */
	if ((data->channels[ch].complete_callback_en == 1) && data->channels[ch].dma_callback) {
		data->channels[ch].dma_callback(dev, data->channels[ch].user_data, ch,
						DMA_STATUS_BLOCK);
	}

	if ((ret == RZA2M_DMA_LINK_TR_END) &&
	    (RZA2M_DMA_IS_SET(stat, RZA2M_DMAC_PRV_CHSTAT_MASK_EN) == 0)) {
		chctrl = sys_read32(RZA2M_DMA_CHCTRL(dev, ch));
		sys_write32(chctrl | RZA2M_DMA_CLREND, RZA2M_DMA_CHCTRL(dev, ch));
		rza2m_dma_channel_free(dev, ch);
		/* Call final callback */
		if ((data->channels[ch].complete_callback_en == 0) &&
		    data->channels[ch].dma_callback) {
			data->channels[ch].dma_callback(dev, data->channels[ch].user_data, ch,
							DMA_STATUS_COMPLETE);
		}
	}

	return 0;
}

static void dma_rza2m_isr_common(const struct device *dev, int ch)
{
	struct dma_rza2m_data *data = dev->data;
	uint32_t stat;
	int ret;

	if (rza2m_dma_check_ch(dev, ch) < 0) {
		LOG_ERR("Invalid channel in isr handler");
		return;
	}

	if (!data->channels[ch].busy) {
		LOG_ERR("Invalid interrupt, DMA Transfer should be started");
		return;
	}

	stat = sys_read32(RZA2M_DMA_CHSTAT(dev, ch));

	if (data->channels[ch].mode == DMA_RZA2M_REGISTER_MODE) {
		ret = dma_rza2m_isr_handle_register_mode(dev, ch, stat);
	} else if (data->channels[ch].mode == DMA_RZA2M_LINK_MODE) {
		ret = dma_rza2m_isr_handle_link_mode(dev, ch, stat);
	} else {
		return;
	}

	if (ret < 0) {
		sys_write32(RZA2M_DMA_CHCTRL_CLEAR, RZA2M_DMA_CHCTRL(dev, ch));
		rza2m_dma_channel_free(dev, ch);
		if ((data->channels[ch].err_callback_en == 0) && data->channels[ch].dma_callback) {
			data->channels[ch].dma_callback(dev, data->channels[ch].user_data, ch,
							-EIO);
		}
	}
}

static void dma_rza2m_err_isr(const struct device *dev)
{
	const struct dma_rza2m_config *config = dev->config;
	struct dma_rza2m_data *data = dev->data;
	uint16_t channel_mask;
	int err_0_7, err_8_15;

	err_0_7 = sys_read32(RZA2M_DMA_DSTAT_ER(dev, 0));  /* Same value for all 0..7 channels */
	err_8_15 = sys_read32(RZA2M_DMA_DSTAT_ER(dev, 8)); /* Same value for all 8..15 channels */

	channel_mask = err_0_7 | (err_8_15 << 8);

	for (int ch = 0; ch < config->num_channels; ch++) {
		if ((channel_mask & BIT(ch)) && (data->channels[ch].err_callback_en == 0) &&
		    data->channels[ch].dma_callback) {
			data->channels[ch].dma_callback(dev, data->channels[ch].user_data, ch,
							DMA_STATUS_BLOCK);
			rza2m_dma_channel_free(dev, ch);
		}
	}
}

static int dma_rza2m_init(const struct device *dev)
{
	const struct dma_rza2m_config *cfg = dev->config;
	int ret;

	if (cfg->pcfg != NULL) {
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Unable to configure DMA pins");
			return -EINVAL;
		}
	}

	DEVICE_MMIO_NAMED_MAP(dev, reg_main, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, ext, K_MEM_CACHE_NONE);

	cfg->irq_configure();

	return 0;
}

#define DMA_RZA2M_IRQ_ERR_CONFIGURE(inst, name)                                                    \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, name, irq),                                          \
		    DT_INST_IRQ_BY_NAME(inst, name, priority), dma_rza2m_err_isr,                  \
		    DEVICE_DT_INST_GET(inst), DT_INST_IRQ_BY_NAME(inst, name, flags));             \
	irq_enable(DT_INST_IRQ_BY_NAME(inst, name, irq));

#define DMA_RZA2M_IRQ_DECLARE_ISR(n, inst)                                                         \
	static void dma_rza2m_##n##_##inst##_isr(const struct device *dev)                         \
	{                                                                                          \
		dma_rza2m_isr_common(dev, n);                                                      \
	}

#define DMA_RZA2M_IRQ_CONFIGURE(n, inst)                                                           \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    dma_rza2m_##n##_##inst##_isr, DEVICE_DT_INST_GET(inst),                        \
		    DT_INST_IRQ_BY_IDX(inst, n, flags));                                           \
	irq_enable(DT_INST_IRQ_BY_IDX(inst, n, irq));

#define DMA_RZA2M_CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, DMA_RZA2M_IRQ_CONFIGURE, (), inst)

#define DMA_RZA2M_DECLARE_ALL_IRQS(inst, n) LISTIFY(n, DMA_RZA2M_IRQ_DECLARE_ISR, (), inst)

#define DMA_RZA2M_POOL_SIZE (CONFIG_DMA_RZA2M_MAX_DESCRS * CONFIG_DMA_RZA2M_DESCRS_CHUNKS)

#define DMA_RZA2M_PINCTRL_DT_INST_DEFINE(n)                                                        \
	COND_CODE_1(DT_INST_NUM_PINCTRL_STATES(n), (PINCTRL_DT_INST_DEFINE(n);), (EMPTY))
#define DMA_RZA2M_PINCTRL_DT_INST_DEV_CONFIG_GET(n)                                                \
	COND_CODE_1(DT_INST_PINCTRL_HAS_IDX(n, 0), (PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL))

#define DMA_RZA2M_INIT(inst)                                                                       \
	DMA_RZA2M_DECLARE_ALL_IRQS(inst, DT_INST_PROP(inst, dma_channels))                         \
                                                                                                   \
	static void dma_rza2m_##inst##_irq_configure(void)                                         \
	{                                                                                          \
		DMA_RZA2M_CONFIGURE_ALL_IRQS(inst, DT_INST_PROP(inst, dma_channels));              \
                                                                                                   \
		COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, err0),        \
			(DMA_RZA2M_IRQ_ERR_CONFIGURE(inst, err0)), ())                             \
		COND_CODE_1(DT_INST_IRQ_HAS_NAME(inst, err1),        \
			(DMA_RZA2M_IRQ_ERR_CONFIGURE(inst, err1)), ())                             \
	}                                                                                          \
                                                                                                   \
	DMA_RZA2M_PINCTRL_DT_INST_DEFINE(inst);                                                    \
                                                                                                   \
	static const struct dma_rza2m_config dma_rza2m_##inst##_config = {                         \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(reg_main, DT_DRV_INST(inst)),                   \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(ext, DT_DRV_INST(inst)),                        \
		.num_channels = DT_INST_PROP(inst, dma_channels),                                  \
		.irq_configure = dma_rza2m_##inst##_irq_configure,                                 \
		.pcfg = DMA_RZA2M_PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                            \
		.addr_alignment = DMA_BUF_ADDR_ALIGNMENT(DT_DRV_INST(inst)),                       \
	};                                                                                         \
                                                                                                   \
	static struct dma_rza2m_channel                                                            \
		dma_rza2m_##inst##_channels[DT_INST_PROP(inst, dma_channels)];                     \
	ATOMIC_DEFINE(dma_rza2m_atomic##inst, DT_INST_PROP(inst, dma_channels));                   \
                                                                                                   \
	SYS_MEM_BLOCKS_DEFINE_STATIC(                                                              \
		descr_pool_##inst,                                                                 \
		ROUND_UP(CONFIG_DMA_RZA2M_MAX_DESCRS * sizeof(struct dma_rza2m_link_descriptor),   \
			 DMA_BUF_ADDR_ALIGNMENT(DT_DRV_INST(inst))),                               \
		CONFIG_DMA_RZA2M_DESCRS_CHUNKS, DMA_BUF_ADDR_ALIGNMENT(DT_DRV_INST(inst)));        \
                                                                                                   \
	static struct dma_rza2m_data dma_rza2m_##inst##_data = {                                   \
		.ctx =                                                                             \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_rza2m_atomic##inst,                                  \
				.dma_channels = DT_INST_PROP(inst, dma_channels),                  \
			},                                                                         \
		.channels = dma_rza2m_##inst##_channels,                                           \
		.descr_pool = &descr_pool_##inst,                                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, dma_rza2m_init, NULL, &dma_rza2m_##inst##_data,                \
			      &dma_rza2m_##inst##_config, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,  \
			      &dma_renesas_rza2m_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_RZA2M_INIT)
