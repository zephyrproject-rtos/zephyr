/*
 * Copyright (c) 2023 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_dmac

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_mchp_xec, CONFIG_DMA_LOG_LEVEL);

#define XEC_DMA_MAX_CHANS CONFIG_DMA_MCHP_XEC_DMAC_MAX_CHANNELS

/* Hardware has no buffer address/size or copy alignment restriction */
#define XEC_DMA_BUF_ADDR_ALIGNMENT 1U
#define XEC_DMA_BUF_SIZE_ALIGNMENT 1U
#define XEC_DMA_COPY_ALIGNMENT     1U

/* Hardware and driver support one block transfer per channel */
#define XEC_DMA_MAX_BLOCK_COUNT 1U

#define XEC_DMA_ABORT_WAIT_LOOPS		32

#define XEC_DMA_MAIN_REGS_SIZE			0x40
#define XEC_DMA_CHAN_REGS_SIZE			0x40

#define XEC_DMA_CHAN_REGS_ADDR(base, channel)	\
	(((uintptr_t)(base) + (XEC_DMA_MAIN_REGS_SIZE)) + \
	 ((uintptr_t)(channel) * XEC_DMA_CHAN_REGS_SIZE))

/* main control */
#define XEC_DMA_MAIN_CTRL_REG_MSK		0x3u
#define XEC_DMA_MAIN_CTRL_EN_POS		0
#define XEC_DMA_MAIN_CTRL_SRST_POS		1

/* channel activate register */
#define XEC_DMA_CHAN_ACTV_EN_POS		0
/* channel control register */
#define XEC_DMA_CHAN_CTRL_REG_MSK		0x037fff27u
#define XEC_DMA_CHAN_CTRL_HWFL_RUN_POS		0
#define XEC_DMA_CHAN_CTRL_REQ_POS		1
#define XEC_DMA_CHAN_CTRL_DONE_POS		2
#define XEC_DMA_CHAN_CTRL_BUSY_POS		5
#define XEC_DMA_CHAN_CTRL_M2D_POS		8
#define XEC_DMA_CHAN_CTRL_HWFL_DEV_POS		9
#define XEC_DMA_CHAN_CTRL_HWFL_DEV_MSK		0xfe00u
#define XEC_DMA_CHAN_CTRL_HWFL_DEV_MSK0		0x7fu
#define XEC_DMA_CHAN_CTRL_INCR_MEM_POS		16
#define XEC_DMA_CHAN_CTRL_INCR_DEV_POS		17
#define XEC_DMA_CHAN_CTRL_LOCK_ARB_POS		18
#define XEC_DMA_CHAN_CTRL_DIS_HWFL_POS		19
#define XEC_DMA_CHAN_CTRL_XFR_UNIT_POS		20
#define XEC_DMA_CHAN_CTRL_XFR_UNIT_MSK		0x700000u
#define XEC_DMA_CHAN_CTRL_XFR_UNIT_MSK0		0x7u
#define XEC_DMA_CHAN_CTRL_SWFL_GO_POS		24
#define XEC_DMA_CHAN_CTRL_ABORT_POS		25
/* channel interrupt status and enable registers */
#define XEC_DMA_CHAN_IES_REG_MSK		0xfu
#define XEC_DMA_CHAN_IES_BERR_POS		0
#define XEC_DMA_CHAN_IES_OVFL_ERR_POS		1
#define XEC_DMA_CHAN_IES_DONE_POS		2
#define XEC_DMA_CHAN_IES_DEV_TERM_POS		3
/* channel fsm (RO) */
#define XEC_DMA_CHAN_FSM_REG_MSK		0xffffu
#define XEC_DMA_CHAN_FSM_ARB_STATE_POS		0
#define XEC_DMA_CHAN_FSM_ARB_STATE_MSK		0xffu
#define XEC_DMA_CHAN_FSM_CTRL_STATE_POS		8
#define XEC_DMA_CHAN_FSM_CTRL_STATE_MSK		0xff00u
#define XEC_DMA_CHAN_FSM_CTRL_STATE_IDLE	0
#define XEC_DMA_CHAN_FSM_CTRL_STATE_ARB_REQ	0x100u
#define XEC_DMA_CHAN_FSM_CTRL_STATE_RD_ACT	0x200u
#define XEC_DMA_CHAN_FSM_CTRL_STATE_WR_ACT	0x300u
#define XEC_DMA_CHAN_FSM_CTRL_STATE_WAIT_DONE	0x400u

#define XEC_DMA_HWFL_DEV_VAL(d)			\
	(((uint32_t)(d) & XEC_DMA_CHAN_CTRL_HWFL_DEV_MSK0) << XEC_DMA_CHAN_CTRL_HWFL_DEV_POS)

#define XEC_DMA_CHAN_CTRL_UNIT_VAL(u)		\
	(((uint32_t)(u) & XEC_DMA_CHAN_CTRL_XFR_UNIT_MSK0) << XEC_DMA_CHAN_CTRL_XFR_UNIT_POS)

/* main block register offsets */
#define XEC_DMA_MAIN_CTRL		0x00u
#define XEC_DMA_MAIN_PKT		0x04u

/* per-channel register offsets, relative to the channel's register block */
#define XEC_DMA_CHAN_ACTV		0x00u
#define XEC_DMA_CHAN_MEM_ADDR		0x04u
#define XEC_DMA_CHAN_MEM_ADDR_END	0x08u
#define XEC_DMA_CHAN_DEV_ADDR		0x0cu
#define XEC_DMA_CHAN_CTRL		0x10u
#define XEC_DMA_CHAN_ISTATUS		0x14u
#define XEC_DMA_CHAN_IENABLE		0x18u
#define XEC_DMA_CHAN_ISTATUS_MSK	0x0fu
#define XEC_DMA_CHAN_FSM		0x1cu

struct dma_xec_irq_info {
	uint8_t gid;	/* GIRQ id [8, 26] */
	uint8_t gpos;   /* bit position in GIRQ [0, 31] */
};

struct dma_xec_config {
	mm_reg_t regs;
	uint8_t dma_requests;
	uint16_t enc_pcr;
	int irq_info_size;
	const struct dma_xec_irq_info *irq_info_list;
	void (*irq_connect)(const struct device *dev);
};

struct dma_xec_channel {
	uint32_t control;
	uint32_t mstart;
	uint32_t mend;
	uint32_t dstart;
	volatile uint32_t isr_hw_status;
	uint32_t block_count;
	uint8_t unit_size;
	uint8_t dir;
	uint8_t flags;
	uint8_t rsvd[1];
	dma_callback_t cb;
	void *user_data;
};

#define DMA_XEC_CHAN_FLAGS_CB_EOB_POS		0
#define DMA_XEC_CHAN_FLAGS_CB_ERR_DIS_POS	1

struct dma_xec_data {
	struct dma_context ctx;
	struct dma_xec_channel *channels;
};

static inline mm_reg_t xec_chan_regs(mm_reg_t regs, uint32_t chan)
{
	return regs + XEC_DMA_MAIN_REGS_SIZE + (chan * XEC_DMA_CHAN_REGS_SIZE);
}

static inline
struct dma_xec_irq_info const *xec_chan_irq_info(const struct dma_xec_config *devcfg,
						 uint32_t channel)
{
	return &devcfg->irq_info_list[channel];
}

static int is_dma_data_size_valid(uint32_t datasz)
{
	if ((datasz == 1U) || (datasz == 2U) || (datasz == 4U)) {
		return 1;
	}

	return 0;
}

/* HW requires if unit size is 2 or 4 bytes the source/destination addresses
 * to be aligned >= 2 or 4 bytes.
 */
static int is_data_aligned(uint32_t src, uint32_t dest, uint32_t unitsz)
{
	if (unitsz == 1) {
		return 1;
	}

	if ((src | dest) & (unitsz - 1U)) {
		return 0;
	}

	return 1;
}

static bool xec_dma_chan_is_busy(mm_reg_t chregs)
{
	uint32_t ctrl = sys_read32(chregs + XEC_DMA_CHAN_CTRL);

	if ((ctrl & (BIT(XEC_DMA_CHAN_CTRL_HWFL_RUN_POS) | BIT(XEC_DMA_CHAN_CTRL_SWFL_GO_POS))) &&
	    (ctrl & BIT(XEC_DMA_CHAN_CTRL_BUSY_POS))) {
		return true;
	}

	return false;
}

static void dma_xec_chan_reset(const struct device *dev, uint32_t chan)
{
	const struct dma_xec_config *devcfg = dev->config;
	int wait_loops = XEC_DMA_ABORT_WAIT_LOOPS;

	mm_reg_t chregs = xec_chan_regs(devcfg->regs, chan);
	const struct dma_xec_irq_info *info = xec_chan_irq_info(devcfg, chan);

	sys_set_bit(chregs + XEC_DMA_CHAN_CTRL, XEC_DMA_CHAN_CTRL_ABORT_POS);
	/* HW stops on next unit boundary (1, 2, or 4 bytes) */
	while ((sys_test_bit(chregs + XEC_DMA_CHAN_CTRL, XEC_DMA_CHAN_CTRL_BUSY_POS) != 0) &&
	       (wait_loops-- > 0)) {
	}

	sys_clear_bit(chregs + XEC_DMA_CHAN_ACTV, XEC_DMA_CHAN_ACTV_EN_POS);
	sys_write32(0, chregs + XEC_DMA_CHAN_CTRL);
	/* mem end address before mem start address to ensure msa >= mea */
	sys_write32(0, chregs + XEC_DMA_CHAN_MEM_ADDR_END);
	sys_write32(0, chregs + XEC_DMA_CHAN_MEM_ADDR);
	sys_write32(0, chregs + XEC_DMA_CHAN_DEV_ADDR);
	sys_write32(0, chregs + XEC_DMA_CHAN_IENABLE);
	sys_write32(XEC_DMA_CHAN_ISTATUS_MSK, chregs + XEC_DMA_CHAN_ISTATUS);

	soc_ecia_girq_status_clear(info->gid, info->gpos);
}

static int is_dma_config_valid(const struct device *dev, struct dma_config *config)
{
	const struct dma_xec_config * const devcfg = dev->config;

	if (config->dma_slot >= (uint32_t)devcfg->dma_requests) {
		LOG_ERR("XEC DMA config dma slot > exceeds number of request lines");
		return 0;
	}

	if (config->half_complete_callback_en != 0) {
		LOG_ERR("XEC DMA does not support half-complete callback");
		return 0;
	}

	if (config->source_data_size != config->dest_data_size) {
		LOG_ERR("XEC DMA requires source and dest data size identical");
		return 0;
	}

	if (!((config->channel_direction == MEMORY_TO_MEMORY) ||
	      (config->channel_direction == MEMORY_TO_PERIPHERAL) ||
	      (config->channel_direction == PERIPHERAL_TO_MEMORY))) {
		LOG_ERR("XEC DMA only support M2M, M2P, P2M");
		return 0;
	}

	if (config->source_handshake != config->dest_handshake) {
		LOG_ERR("Source & Dest. handshake not match");
		return 0;
	}

	if (!is_dma_data_size_valid(config->source_data_size) ||
	    !is_dma_data_size_valid(config->dest_data_size)) {
		LOG_ERR("XEC DMA requires xfr unit size of 1, 2 or 4 bytes");
		return 0;
	}

	if (config->block_count != 1) {
		LOG_ERR("XEC DMA block count != 1");
		return 0;
	}

	return 1;
}

static int check_blocks(struct dma_xec_channel *chdata, struct dma_block_config *block,
			uint32_t block_count, uint32_t unit_size)
{
	if (!block || !chdata) {
		LOG_ERR("bad pointer");
		return -EINVAL;
	}

	for (uint32_t i = 0; i < block_count; i++) {
		if ((block->source_addr_adj == DMA_ADDR_ADJ_DECREMENT) ||
		    (block->dest_addr_adj == DMA_ADDR_ADJ_DECREMENT)) {
			LOG_ERR("XEC DMA HW does not support address decrement. Block index %u", i);
			return -EINVAL;
		}

		if (!is_data_aligned(block->source_address, block->dest_address, unit_size)) {
			LOG_ERR("XEC DMA block at index %u violates source/dest unit size", i);
			return -EINVAL;
		}

		if (block->block_size == 0u) {
			LOG_ERR("XEC DMA block at index %u has zero size", i);
			return -EINVAL;
		}

		if (block->next_block != NULL) {
			LOG_ERR("XEC DMA supports a single block; next_block must be NULL");
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * struct dma_config flags
 * dma_slot - peripheral source/target ID. Not used for Mem2Mem
 * channel_direction - HW supports Mem2Mem, Mem2Periph, and Periph2Mem
 * complete_callback_en - if true invoke callback on completion (no error)
 * error_callback_dis - if true disable callback on error
 * source_handshake - 0=HW, 1=SW
 * dest_handshake - 0=HW, 1=SW
 * channel_priority - 4-bit field. HW implements round-robin only.
 * source_chaining_en - Chaining channel together
 * dest_chaining_en - HW does not support channel chaining.
 * linked_channel - HW does not support
 * cyclic - HW does not support cyclic buffer. Would have to emulate with SW.
 * source_data_size - unit size of source data. HW supports 1, 2, or 4 bytes
 * dest_data_size - unit size of dest data. HW requires same as source_data_size
 * source_burst_length - HW does not support
 * dest_burst_length - HW does not support
 * block_count -
 * user_data -
 * dma_callback -
 * head_block - pointer to struct dma_block_config
 *
 * struct dma_block_config
 * source_address -
 * source_gather_interval - N/A
 * dest_address -
 * dest_scatter_interval - N/A
 * dest_scatter_count - N/A
 * source_gather_count - N/A
 * block_size
 * config - flags
 *	source_gather_en - N/A
 *	dest_scatter_en - N/A
 *	source_addr_adj - 0(increment), 1(decrement), 2(no change)
 *	dest_addr_adj - 0(increment), 1(decrement), 2(no change)
 *	source_reload_en - reload source address at end of block
 *	dest_reload_en - reload destination address at end of block
 *	fifo_mode_control - N/A
 *	flow_control_mode - 0(source req service on data available) HW does this
 *			    1(source req postposed until dest req happens) N/A
 *
 *
 * DMA channel implements memory start address, memory end address,
 * and peripheral address registers. No peripheral end address.
 * Transfer ends when memory start address increments and reaches
 * memory end address.
 *
 * Memory to Memory: copy from source_address to dest_address
 *	chan direction = Mem2Dev. chan.control b[8]=1
 *	chan mem_addr = source_address
 *	chan mem_addr_end = source_address + block_size
 *	chan dev_addr = dest_address
 *
 * Memory to Peripheral: copy from source_address(memory) to dest_address(peripheral)
 *	chan direction = Mem2Dev. chan.control b[8]=1
 *	chan mem_addr = source_address
 *	chan mem_addr_end = chan mem_addr + block_size
 *	chan dev_addr = dest_address
 *
 * Peripheral to Memory:
 *	chan direction = Dev2Mem. chan.control b[8]=1
 *	chan mem_addr = dest_address
 *	chan mem_addr_end = chan mem_addr + block_size
 *	chan dev_addr = source_address
 */
static int dma_xec_configure(const struct device *dev, uint32_t channel,
			     struct dma_config *config)
{
	const struct dma_xec_config * const devcfg = dev->config;
	mm_reg_t const regs = devcfg->regs;
	struct dma_xec_data * const data = dev->data;
	uint32_t ctrl, mstart, mend, dstart, unit_size;
	int ret;

	if (!config || (channel >= XEC_DMA_MAX_CHANS)) {
		return -EINVAL;
	}

	mm_reg_t const chregs = xec_chan_regs(regs, channel);
	struct dma_xec_channel *chdata = &data->channels[channel];

	/* Do not reconfigure a channel with a transfer in progress */
	if (xec_dma_chan_is_busy(chregs)) {
		return -EBUSY;
	}

	if (!is_dma_config_valid(dev, config)) {
		return -EINVAL;
	}

	struct dma_block_config *block = config->head_block;

	ret = check_blocks(chdata, block, config->block_count, config->source_data_size);
	if (ret) {
		return ret;
	}

	unit_size = config->source_data_size;
	chdata->unit_size = unit_size;
	chdata->block_count = config->block_count;
	chdata->dir = config->channel_direction;

	chdata->flags = 0;
	chdata->cb = config->dma_callback;
	chdata->user_data = config->user_data;

	/* invoke callback on completion of each block instead of all blocks ? */
	if (config->complete_callback_en) {
		chdata->flags |= BIT(DMA_XEC_CHAN_FLAGS_CB_EOB_POS);
	}
	if (config->error_callback_dis) { /* disable callback on errors ? */
		chdata->flags |= BIT(DMA_XEC_CHAN_FLAGS_CB_ERR_DIS_POS);
	}

	/* Use the control member of struct dma_xec_channel to
	 * store control register value containing fields invariant
	 * for all buffers: HW flow control device, direction, unit size, ...
	 * derived from struct dma_config
	 */
	ctrl = XEC_DMA_CHAN_CTRL_UNIT_VAL(unit_size);
	if (config->channel_direction == MEMORY_TO_MEMORY) {
		ctrl |= BIT(XEC_DMA_CHAN_CTRL_DIS_HWFL_POS);
	} else {
		ctrl |= XEC_DMA_HWFL_DEV_VAL(config->dma_slot);
	}

	if (config->channel_direction == PERIPHERAL_TO_MEMORY) {
		mstart = block->dest_address;
		mend = block->dest_address + block->block_size;
		dstart = block->source_address;
		if (block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			ctrl |= BIT(XEC_DMA_CHAN_CTRL_INCR_DEV_POS);
		}
		if (block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			ctrl |= BIT(XEC_DMA_CHAN_CTRL_INCR_MEM_POS);
		}
	} else {
		mstart = block->source_address;
		mend = block->source_address + block->block_size;
		dstart = block->dest_address;
		ctrl |= BIT(XEC_DMA_CHAN_CTRL_M2D_POS);
		if (block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			ctrl |= BIT(XEC_DMA_CHAN_CTRL_INCR_MEM_POS);
		}
		if (block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT) {
			ctrl |= BIT(XEC_DMA_CHAN_CTRL_INCR_DEV_POS);
		}
	}

	chdata->control = ctrl;
	chdata->mstart = mstart;
	chdata->mend = mend;
	chdata->dstart = dstart;

	dma_xec_chan_reset(dev, channel);

	sys_write32(mstart, chregs + XEC_DMA_CHAN_MEM_ADDR);
	sys_write32(mend, chregs + XEC_DMA_CHAN_MEM_ADDR_END);
	sys_write32(dstart, chregs + XEC_DMA_CHAN_DEV_ADDR);

	sys_write32(ctrl, chregs + XEC_DMA_CHAN_CTRL);

	return 0;
}

/* Update previously configured DMA channel with new data source address,
 * data destination address, and size in bytes.
 * src = source address for DMA transfer
 * dst = destination address for DMA transfer
 * size = size of DMA transfer. Assume this is in bytes.
 * We assume the caller will pass src, dst, and size that matches
 * the unit size from the previous configure call.
 */
static int dma_xec_reload(const struct device *dev, uint32_t channel,
			  uint32_t src, uint32_t dst, size_t size)
{
	const struct dma_xec_config * const devcfg = dev->config;
	struct dma_xec_data * const data = dev->data;
	mm_reg_t const regs = devcfg->regs;
	uint32_t ctrl;

	if (channel >= XEC_DMA_MAX_CHANS) {
		return -EINVAL;
	}

	struct dma_xec_channel *chdata = &data->channels[channel];
	mm_reg_t chregs = xec_chan_regs(regs, channel);

	if (xec_dma_chan_is_busy(chregs)) {
		return -EBUSY;
	}

	ctrl = sys_read32(chregs + XEC_DMA_CHAN_CTRL) &
	       ~(BIT(XEC_DMA_CHAN_CTRL_HWFL_RUN_POS) | BIT(XEC_DMA_CHAN_CTRL_SWFL_GO_POS));

	dma_xec_chan_reset(dev, channel);

	if ((ctrl & (BIT(XEC_DMA_CHAN_CTRL_M2D_POS) | BIT(XEC_DMA_CHAN_CTRL_DIS_HWFL_POS)))) {
		/* memory to memory or memory to peripheral */
		chdata->mstart = src;
		chdata->dstart = dst;
	} else {
		chdata->mstart = dst;
		chdata->dstart = src;
	}

	chdata->mend = chdata->mstart + size;

	sys_write32(chdata->mstart, chregs + XEC_DMA_CHAN_MEM_ADDR);
	sys_write32(chdata->mend, chregs + XEC_DMA_CHAN_MEM_ADDR_END);
	sys_write32(chdata->dstart, chregs + XEC_DMA_CHAN_DEV_ADDR);
	sys_write32(ctrl, chregs + XEC_DMA_CHAN_CTRL);

	return 0;
}

static int dma_xec_start(const struct device *dev, uint32_t channel)
{
	const struct dma_xec_config * const devcfg = dev->config;
	struct dma_xec_data *const data = dev->data;
	mm_reg_t const regs = devcfg->regs;
	uint32_t chan_ctrl = 0U;

	if (channel >= XEC_DMA_MAX_CHANS) {
		return -EINVAL;
	}

	mm_reg_t chregs = xec_chan_regs(regs, channel);

	if (xec_dma_chan_is_busy(chregs)) {
		return -EBUSY;
	}

	data->channels[channel].isr_hw_status = 0;

	sys_set_bit(chregs + XEC_DMA_CHAN_ACTV, XEC_DMA_CHAN_ACTV_EN_POS);
	sys_write32(XEC_DMA_CHAN_ISTATUS_MSK, chregs + XEC_DMA_CHAN_ISTATUS);
	sys_write32(BIT(XEC_DMA_CHAN_IES_BERR_POS) | BIT(XEC_DMA_CHAN_IES_DONE_POS),
		    chregs + XEC_DMA_CHAN_IENABLE);
	chan_ctrl = sys_read32(chregs + XEC_DMA_CHAN_CTRL);

	if (chan_ctrl & BIT(XEC_DMA_CHAN_CTRL_DIS_HWFL_POS)) {
		chan_ctrl |= BIT(XEC_DMA_CHAN_CTRL_SWFL_GO_POS);
	} else {
		chan_ctrl |= BIT(XEC_DMA_CHAN_CTRL_HWFL_RUN_POS);
	}

	sys_write32(chan_ctrl, chregs + XEC_DMA_CHAN_CTRL);

	return 0;
}

static int dma_xec_stop(const struct device *dev, uint32_t channel)
{
	if (channel >= XEC_DMA_MAX_CHANS) {
		return -EINVAL;
	}

	dma_xec_chan_reset(dev, channel);

	return 0;
}

/* Get DMA transfer status.
 * HW supports: MEMORY_TO_MEMORY, MEMORY_TO_PERIPHERAL, or
 * PERIPHERAL_TO_MEMORY
 * current DMA runtime status structure
 *
 * busy				- is current DMA transfer busy or idle
 * dir				- DMA transfer direction
 * pending_length		- data length pending to be transferred in bytes
 *					or platform dependent.
 * We don't implement a circular buffer
 * free                         - free buffer space
 * write_position               - write position in a circular dma buffer
 * read_position                - read position in a circular dma buffer
 *
 */
static int dma_xec_get_status(const struct device *dev, uint32_t channel,
			      struct dma_status *status)
{
	const struct dma_xec_config * const devcfg = dev->config;
	struct dma_xec_data * const data = dev->data;
	mm_reg_t const regs = devcfg->regs;
	uint32_t chan_ctrl = 0U;

	if ((channel >= XEC_DMA_MAX_CHANS) || (!status)) {
		LOG_ERR("unsupported channel");
		return -EINVAL;
	}

	struct dma_xec_channel *chan_data = &data->channels[channel];
	mm_reg_t chregs = xec_chan_regs(regs, channel);

	status->busy = false;
	if (xec_dma_chan_is_busy(chregs)) {
		status->busy = true;
	}

	chan_ctrl = sys_read32(chregs + XEC_DMA_CHAN_CTRL);

	status->free = 0;
	status->write_position = 0;
	status->read_position = 0;

	uint32_t msa = sys_read32(chregs + XEC_DMA_CHAN_MEM_ADDR);
	uint32_t mea = sys_read32(chregs + XEC_DMA_CHAN_MEM_ADDR_END);

	status->pending_length = 0;
	if (mea > msa) {
		status->pending_length = mea - msa;
	}

	if (chan_ctrl & BIT(XEC_DMA_CHAN_CTRL_DIS_HWFL_POS)) {
		status->dir = MEMORY_TO_MEMORY;
	} else if (chan_ctrl & BIT(XEC_DMA_CHAN_CTRL_M2D_POS)) {
		status->dir = MEMORY_TO_PERIPHERAL;
	} else {
		status->dir = PERIPHERAL_TO_MEMORY;
	}

	status->total_copied = 0;

	/* Report a latched bus error from the last transfer */
	if (chan_data->isr_hw_status & BIT(XEC_DMA_CHAN_IES_BERR_POS)) {
		return -EIO;
	}

	return 0;
}

int xec_dma_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	ARG_UNUSED(dev);

	if (value == NULL) {
		return -EINVAL;
	}

	switch (type) {
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = XEC_DMA_BUF_ADDR_ALIGNMENT;
		break;
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = XEC_DMA_BUF_SIZE_ALIGNMENT;
		break;
	case DMA_ATTR_COPY_ALIGNMENT:
		*value = XEC_DMA_COPY_ALIGNMENT;
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = XEC_DMA_MAX_BLOCK_COUNT;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static bool dma_xec_chan_filter(const struct device *dev, int ch, void *filter_param)
{
	if ((ch < 0) || (ch >= XEC_DMA_MAX_CHANS)) {
		return false;
	}

	if (filter_param == NULL) { /* allow any valid channel */
		return true;
	}

	/* Hardware only supports normal channels */
	return (*((enum dma_channel_filter *)filter_param) == DMA_CHANNEL_NORMAL);
}

/* API - HW does not stupport suspend/resume */
static DEVICE_API(dma, dma_xec_api) = {
	.config = dma_xec_configure,
	.reload = dma_xec_reload,
	.start = dma_xec_start,
	.stop = dma_xec_stop,
	.get_status = dma_xec_get_status,
	.chan_filter = dma_xec_chan_filter,
	.get_attribute = xec_dma_get_attribute,
};

#ifdef CONFIG_PM_DEVICE
/* TODO - DMA block has one PCR SLP_EN and one CLK_REQ.
 * If any channel is running the block's CLK_REQ is asserted.
 * CLK_REQ will not clear until all channels are done or disabled.
 * Clearing the DMA Main activate will kill DMA transactions resulting
 * possible data corruption and HW flow control device malfunctions.
 */
static int dmac_xec_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	const struct dma_xec_config * const devcfg = dev->config;
	mm_reg_t const regs = devcfg->regs;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		sys_set_bit(regs + XEC_DMA_MAIN_CTRL, XEC_DMA_MAIN_CTRL_EN_POS);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static void dma_xec_irq_handler(const struct device *dev, uint32_t channel)
{
	const struct dma_xec_config * const devcfg = dev->config;
	const struct dma_xec_irq_info *info = devcfg->irq_info_list;
	struct dma_xec_data * const data = dev->data;
	struct dma_xec_channel *chan_data = &data->channels[channel];
	mm_reg_t const regs = xec_chan_regs(devcfg->regs, channel);
	uint32_t sts = sys_read32(regs + XEC_DMA_CHAN_ISTATUS);
	int cb_status = 0;
	bool invoke_cb = true;

	LOG_DBG("maddr=0x%08x mend=0x%08x daddr=0x%08x ctrl=0x%08x sts=0x%02x",
		sys_read32(regs + XEC_DMA_CHAN_MEM_ADDR),
		sys_read32(regs + XEC_DMA_CHAN_MEM_ADDR_END),
		sys_read32(regs + XEC_DMA_CHAN_DEV_ADDR),
		sys_read32(regs + XEC_DMA_CHAN_CTRL), sts);

	sys_write32(0u, regs + XEC_DMA_CHAN_IENABLE);
	sys_write32(sts, regs + XEC_DMA_CHAN_ISTATUS);
	soc_ecia_girq_status_clear(info[channel].gid, info[channel].gpos);

	chan_data->isr_hw_status = sts;

	if (sts & BIT(XEC_DMA_CHAN_IES_BERR_POS)) {/* Bus Error? */
		cb_status = -EIO;
		if (chan_data->flags & BIT(DMA_XEC_CHAN_FLAGS_CB_ERR_DIS_POS)) {
			invoke_cb = false;
		}
	}

	if (invoke_cb && chan_data->cb) {
		chan_data->cb(dev, chan_data->user_data, channel, cb_status);
	}
}

static int dma_xec_init(const struct device *dev)
{
	const struct dma_xec_config * const devcfg = dev->config;
	mm_reg_t const regs = devcfg->regs;

	LOG_DBG("driver init");

	soc_xec_pcr_sleep_en_clear(devcfg->enc_pcr);

	/* Soft reset is self-clearing and also clears the block enable.
	 * Wait for it to complete before re-enabling the controller,
	 * otherwise the enable write can be lost while the reset is in
	 * progress.
	 */
	sys_write32(BIT(XEC_DMA_MAIN_CTRL_SRST_POS), regs + XEC_DMA_MAIN_CTRL);
	while (sys_test_bit(regs + XEC_DMA_MAIN_CTRL, XEC_DMA_MAIN_CTRL_SRST_POS) != 0) {
	}
	sys_write32(BIT(XEC_DMA_MAIN_CTRL_EN_POS), regs + XEC_DMA_MAIN_CTRL);

	if (devcfg->irq_connect != NULL) {
		devcfg->irq_connect(dev);
	}

	return 0;
}

/* n = node-id, p = property, i = index */
#define DMA_XEC_GID(n, p, i) MCHP_XEC_ECIA_GIRQ(DT_PROP_BY_IDX(n, p, i))
#define DMA_XEC_GPOS(n, p, i) MCHP_XEC_ECIA_GIRQ_POS(DT_PROP_BY_IDX(n, p, i))

#define DMA_XEC_GIRQ_INFO(n, p, i)						\
	{									\
		.gid = DMA_XEC_GID(n, p, i),					\
		.gpos = DMA_XEC_GPOS(n, p, i),					\
	},

/* node_id, p = property (interrupt-names), i = interrupt/channel index */
#define DMA_XEC_IRQ_DECLARE(node_id, p, i)                                                         \
	static void dma_xec_chan_##i##_isr(const struct device *dev)                               \
	{                                                                                          \
		dma_xec_irq_handler(dev, i);                                                       \
	}

#define DMA_XEC_IRQ_CONNECT_SUB(node_id, p, i)                                                     \
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, i, irq), DT_IRQ_BY_IDX(node_id, i, priority),           \
		    dma_xec_chan_##i##_isr, DEVICE_DT_GET(node_id), 0);                            \
	irq_enable(DT_IRQ_BY_IDX(node_id, i, irq));                                                \
	soc_ecia_girq_ctrl(devcfg->irq_info_list[i].gid, devcfg->irq_info_list[i].gpos, 1);

/* i = instance number of DMA controller. The connect loop iterates the
 * interrupt-names property so the index aligns 1:1 with the interrupts
 * entries; the GIRQ enable is looked up from the config array by index.
 */
#define DMA_XEC_IRQ_CONNECT(inst)                                                                  \
	DT_INST_FOREACH_PROP_ELEM(inst, interrupt_names, DMA_XEC_IRQ_DECLARE)                      \
	void dma_xec_irq_connect##inst(const struct device *dev)                                   \
	{                                                                                          \
		const struct dma_xec_config *const devcfg = dev->config;                           \
		DT_INST_FOREACH_PROP_ELEM(inst, interrupt_names, DMA_XEC_IRQ_CONNECT_SUB)          \
	}

#define DMA_XEC_DEVICE(i)                                                                          \
	BUILD_ASSERT(DT_INST_PROP(i, dma_requests) <= XEC_DMA_MAX_CHANS,                           \
		     "XEC DMA dma-requests exceeds max");                                          \
                                                                                                   \
	static struct dma_xec_channel dma_xec_ctrl##i##_chans[DT_INST_PROP(i, dma_channels)];      \
	ATOMIC_DEFINE(dma_xec_atomic##i, DT_INST_PROP(i, dma_channels));                           \
                                                                                                   \
	static struct dma_xec_data dma_xec_data##i = {                                             \
		.ctx.magic = DMA_MAGIC,                                                            \
		.ctx.dma_channels = DT_INST_PROP(i, dma_channels),                                 \
		.ctx.atomic = dma_xec_atomic##i,                                                   \
		.channels = dma_xec_ctrl##i##_chans,                                               \
	};                                                                                         \
                                                                                                   \
	DMA_XEC_IRQ_CONNECT(i)                                                                     \
                                                                                                   \
	static const struct dma_xec_irq_info dma_xec_irqi##i[] = {                                 \
		DT_INST_FOREACH_PROP_ELEM(i, girqs, DMA_XEC_GIRQ_INFO)};                           \
	static const struct dma_xec_config dma_xec_cfg##i = {                                      \
		.regs = DT_INST_REG_ADDR(i),                                                       \
		.dma_requests = DT_INST_PROP(i, dma_requests),                                     \
		.enc_pcr = DT_INST_PROP(i, pcr_scr),                                               \
		.irq_info_size = ARRAY_SIZE(dma_xec_irqi##i),                                      \
		.irq_info_list = dma_xec_irqi##i,                                                  \
		.irq_connect = dma_xec_irq_connect##i,                                             \
	};                                                                                         \
	PM_DEVICE_DT_DEFINE(i, dmac_xec_pm_action);                                                \
	DEVICE_DT_INST_DEFINE(i, dma_xec_init, PM_DEVICE_DT_GET(i), &dma_xec_data##i,              \
			      &dma_xec_cfg##i, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,             \
			      &dma_xec_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_XEC_DEVICE)
