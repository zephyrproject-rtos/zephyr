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
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_mchp_xec, CONFIG_DMA_LOG_LEVEL);

#define XEC_DMA_DEBUG				1
#ifdef XEC_DMA_DEBUG
#include <string.h>
#endif

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

struct dma_xec_chan_regs {
	volatile uint32_t actv;
	volatile uint32_t mem_addr;
	volatile uint32_t mem_addr_end;
	volatile uint32_t dev_addr;
	volatile uint32_t control;
	volatile uint32_t istatus;
	volatile uint32_t ienable;
	volatile uint32_t fsm;
	uint32_t rsvd_20_3f[8];
};

struct dma_xec_regs {
	volatile uint32_t mctrl;
	volatile uint32_t mpkt;
	uint32_t rsvd_08_3f[14];
};

struct dma_xec_irq_info {
	uint8_t gid;	/* GIRQ id [8, 26] */
	uint8_t gpos;	/* bit position in GIRQ [0, 31] */
	uint8_t anid;	/* aggregated external NVIC input */
	uint8_t dnid;	/* direct NVIC input */
};

struct dma_xec_config {
	struct dma_xec_regs *regs;
	uint8_t dma_channels;
	uint8_t dma_requests;
	uint8_t pcr_idx;
	uint8_t pcr_pos;
	int irq_info_size;
	const struct dma_xec_irq_info *irq_info_list;
	void (*irq_connect)(void);
};

struct dma_xec_channel {
	uint32_t control;
	uint32_t mstart;
	uint32_t mend;
	uint32_t dstart;
	uint32_t isr_hw_status;
	uint32_t block_count;
	uint8_t unit_size;
	uint8_t dir;
	uint8_t flags;
	uint8_t rsvd[1];
	struct dma_block_config *head;
	struct dma_block_config *curr;
	dma_callback_t cb;
	void *user_data;
	uint32_t total_req_xfr_len;
	uint32_t total_curr_xfr_len;
};

#define DMA_XEC_CHAN_FLAGS_CB_EOB_POS		0
#define DMA_XEC_CHAN_FLAGS_CB_ERR_DIS_POS	1

struct dma_xec_data {
	struct dma_context ctx;
	struct dma_xec_channel *channels;
};

#ifdef XEC_DMA_DEBUG
static void xec_dma_debug_clean(void);
#endif

static inline struct dma_xec_chan_regs *xec_chan_regs(struct dma_xec_regs *regs, uint32_t chan)
{
	uint8_t *pregs = (uint8_t *)regs + XEC_DMA_MAIN_REGS_SIZE;

	pregs += (chan * (XEC_DMA_CHAN_REGS_SIZE));

	return (struct dma_xec_chan_regs *)pregs;
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

static void xec_dma_chan_clr(struct dma_xec_chan_regs * const chregs,
			     const struct dma_xec_irq_info *info)
{
	chregs->actv = 0;
	chregs->control = 0;
	chregs->mem_addr = 0;
	chregs->mem_addr_end = 0;
	chregs->dev_addr = 0;
	chregs->control = 0;
	chregs->ienable = 0;
	chregs->istatus = 0xffu;
	mchp_xec_ecia_girq_src_clr(info->gid, info->gpos);
}

static int is_dma_config_valid(const struct device *dev, struct dma_config *config)
{
	const struct dma_xec_config * const devcfg = dev->config;

	if (config->dma_slot >= (uint32_t)devcfg->dma_requests) {
		LOG_ERR("XEC DMA config dma slot > exceeds number of request lines");
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

	if (!is_dma_data_size_valid(config->source_data_size)) {
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

	chdata->total_req_xfr_len = 0;

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

		chdata->total_req_xfr_len += block->block_size;
	}

	return 0;
}

/*
 * struct dma_config flags
 * dma_slot - peripheral source/target ID. Not used for Mem2Mem
 * channel_direction - HW supports Mem2Mem, Mem2Periph, and Periph2Mem
 * complete_callback_en - if true invoke callback on completion (no error)
 * error_callback_en - if true invoke callback on error
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
 *	chan direction = Dev2Mem. chan.contronl b[8]=1
 *	chan mem_addr = dest_address
 *	chan mem_addr_end = chan mem_addr + block_size
 *	chan dev_addr = source_address
 */
static int dma_xec_configure(const struct device *dev, uint32_t channel,
			     struct dma_config *config)
{
	const struct dma_xec_config * const devcfg = dev->config;
	struct dma_xec_regs * const regs = devcfg->regs;
	struct dma_xec_data * const data = dev->data;
	uint32_t ctrl, mstart, mend, dstart, unit_size;
	int ret;

	if (!dev || !config || (channel >= (uint32_t)devcfg->dma_channels)) {
		return -EINVAL;
	}

#ifdef XEC_DMA_DEBUG
	xec_dma_debug_clean();
#endif

	const struct dma_xec_irq_info *info = xec_chan_irq_info(devcfg, channel);
	struct dma_xec_chan_regs * const chregs = xec_chan_regs(regs, channel);
	struct dma_xec_channel *chdata = &data->channels[channel];

	chdata->total_req_xfr_len = 0;
	chdata->total_curr_xfr_len = 0;

	xec_dma_chan_clr(chregs, info);

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
	chdata->head = block;
	chdata->curr = block;
	chdata->block_count = config->block_count;
	chdata->dir = config->channel_direction;

	chdata->flags = 0;
	chdata->cb = config->dma_callback;
	chdata->user_data = config->user_data;

	/* invoke callback on completion of each block instead of all blocks ? */
	if (config->complete_callback_en) {
		chdata->flags |= BIT(DMA_XEC_CHAN_FLAGS_CB_EOB_POS);
	}
	if (config->error_callback_en) { /* disable callback on errors ? */
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

	chregs->actv &= ~BIT(XEC_DMA_CHAN_ACTV_EN_POS);
	chregs->mem_addr = mstart;
	chregs->mem_addr_end = mend;
	chregs->dev_addr = dstart;

	chregs->control = ctrl;
	chregs->ienable = BIT(XEC_DMA_CHAN_IES_BERR_POS) | BIT(XEC_DMA_CHAN_IES_DONE_POS);
	chregs->actv |= BIT(XEC_DMA_CHAN_ACTV_EN_POS);

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
	struct dma_xec_regs * const regs = devcfg->regs;
	uint32_t ctrl;

	if (channel >= (uint32_t)devcfg->dma_channels) {
		return -EINVAL;
	}

	struct dma_xec_channel *chdata = &data->channels[channel];
	struct dma_xec_chan_regs *chregs = xec_chan_regs(regs, channel);

	if (chregs->control & BIT(XEC_DMA_CHAN_CTRL_BUSY_POS)) {
		return -EBUSY;
	}

	ctrl = chregs->control & ~(BIT(XEC_DMA_CHAN_CTRL_HWFL_RUN_POS)
				   | BIT(XEC_DMA_CHAN_CTRL_SWFL_GO_POS));
	chregs->ienable = 0;
	chregs->control = 0;
	chregs->istatus = 0xffu;

	if (ctrl & BIT(XEC_DMA_CHAN_CTRL_M2D_POS)) { /* Memory to Device */
		chdata->mstart = src;
		chdata->dstart = dst;
	} else {
		chdata->mstart = dst;
		chdata->dstart = src;
	}

	chdata->mend = chdata->mstart + size;
	chdata->total_req_xfr_len = size;
	chdata->total_curr_xfr_len = 0;

	chregs->mem_addr = chdata->mstart;
	chregs->mem_addr_end = chdata->mend;
	chregs->dev_addr = chdata->dstart;
	chregs->control = ctrl;

	return 0;
}

static int dma_xec_start(const struct device *dev, uint32_t channel)
{
	const struct dma_xec_config * const devcfg = dev->config;
	struct dma_xec_regs * const regs = devcfg->regs;
	uint32_t chan_ctrl = 0U;

	if (channel >= (uint32_t)devcfg->dma_channels) {
		return -EINVAL;
	}

	struct dma_xec_chan_regs *chregs = xec_chan_regs(regs, channel);

	if (chregs->control & BIT(XEC_DMA_CHAN_CTRL_BUSY_POS)) {
		return -EBUSY;
	}

	chregs->ienable = 0u;
	chregs->istatus = 0xffu;
	chan_ctrl = chregs->control;

	if (chan_ctrl & BIT(XEC_DMA_CHAN_CTRL_DIS_HWFL_POS)) {
		chan_ctrl |= BIT(XEC_DMA_CHAN_CTRL_SWFL_GO_POS);
	} else {
		chan_ctrl |= BIT(XEC_DMA_CHAN_CTRL_HWFL_RUN_POS);
	}

	chregs->ienable = BIT(XEC_DMA_CHAN_IES_BERR_POS) | BIT(XEC_DMA_CHAN_IES_DONE_POS);
	chregs->control = chan_ctrl;
	chregs->actv |= BIT(XEC_DMA_CHAN_ACTV_EN_POS);

	return 0;
}

static int dma_xec_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_xec_config * const devcfg = dev->config;
	struct dma_xec_regs * const regs = devcfg->regs;
	int wait_loops = XEC_DMA_ABORT_WAIT_LOOPS;

	if (channel >= (uint32_t)devcfg->dma_channels) {
		return -EINVAL;
	}

	struct dma_xec_chan_regs *chregs = xec_chan_regs(regs, channel);

	chregs->ienable = 0;

	if (chregs->control & BIT(XEC_DMA_CHAN_CTRL_BUSY_POS)) {
		chregs->ienable = 0;
		chregs->control |= BIT(XEC_DMA_CHAN_CTRL_ABORT_POS);
		/* HW stops on next unit boundary (1, 2, or 4 bytes) */

		do {
			if (!(chregs->control & BIT(XEC_DMA_CHAN_CTRL_BUSY_POS))) {
				break;
			}
		} while (wait_loops--);
	}

	chregs->mem_addr = chregs->mem_addr_end;
	chregs->fsm = 0; /* delay */
	chregs->control = 0;
	chregs->istatus = 0xffu;
	chregs->actv = 0;

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
	struct dma_xec_regs * const regs = devcfg->regs;
	uint32_t chan_ctrl = 0U;

	if ((channel >= (uint32_t)devcfg->dma_channels) || (!status)) {
		LOG_ERR("unsupported channel");
		return -EINVAL;
	}

	struct dma_xec_channel *chan_data = &data->channels[channel];
	struct dma_xec_chan_regs *chregs = xec_chan_regs(regs, channel);

	chan_ctrl = chregs->control;

	if (chan_ctrl & BIT(XEC_DMA_CHAN_CTRL_BUSY_POS)) {
		status->busy = true;
		/* number of bytes remaining in channel */
		status->pending_length = chan_data->total_req_xfr_len -
						(chregs->mem_addr_end - chregs->mem_addr);
	} else {
		status->pending_length = chan_data->total_req_xfr_len -
						chan_data->total_curr_xfr_len;
		status->busy = false;
	}

	if (chan_ctrl & BIT(XEC_DMA_CHAN_CTRL_DIS_HWFL_POS)) {
		status->dir = MEMORY_TO_MEMORY;
	} else if (chan_ctrl & BIT(XEC_DMA_CHAN_CTRL_M2D_POS)) {
		status->dir = MEMORY_TO_PERIPHERAL;
	} else {
		status->dir = PERIPHERAL_TO_MEMORY;
	}

	status->total_copied = chan_data->total_curr_xfr_len;

	return 0;
}

int xec_dma_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	if ((type == DMA_ATTR_MAX_BLOCK_COUNT) && value) {
		*value = 1;
		return 0;
	}

	return -EINVAL;
}

/* returns true if filter matched otherwise returns false */
static bool dma_xec_chan_filter(const struct device *dev, int ch, void *filter_param)
{
	const struct dma_xec_config * const devcfg = dev->config;
	uint32_t filter = 0u;

	if (!filter_param && devcfg->dma_channels) {
		filter = GENMASK(devcfg->dma_channels-1u, 0);
	} else {
		filter = *((uint32_t *)filter_param);
	}

	return (filter & BIT(ch));
}

/* API - HW does not stupport suspend/resume */
static const struct dma_driver_api dma_xec_api = {
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
	struct dma_xec_regs * const regs = devcfg->regs;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		regs->mctrl |= BIT(XEC_DMA_MAIN_CTRL_EN_POS);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		/* regs->mctrl &= ~BIT(XEC_DMA_MAIN_CTRL_EN_POS); */
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

/* DMA channel interrupt handler called by ISR.
 * Callback flags in struct dma_config
 * completion_callback_en
 *	0 = invoke at completion of all blocks
 *	1 = invoke at completin of each block
 * error_callback_en
 *	0 = invoke on all errors
 *	1 = disabled, do not invoke on errors
 */
/* DEBUG */
#ifdef XEC_DMA_DEBUG
static volatile uint8_t channel_isr_idx[16];
static volatile uint8_t channel_isr_sts[16][16];
static volatile uint32_t channel_isr_ctrl[16][16];

static void xec_dma_debug_clean(void)
{
	memset((void *)channel_isr_idx, 0, sizeof(channel_isr_idx));
	memset((void *)channel_isr_sts, 0, sizeof(channel_isr_sts));
	memset((void *)channel_isr_ctrl, 0, sizeof(channel_isr_ctrl));
}
#endif

static void dma_xec_irq_handler(const struct device *dev, uint32_t channel)
{
	const struct dma_xec_config * const devcfg = dev->config;
	const struct dma_xec_irq_info *info = devcfg->irq_info_list;
	struct dma_xec_data * const data = dev->data;
	struct dma_xec_channel *chan_data = &data->channels[channel];
	struct dma_xec_chan_regs * const regs = xec_chan_regs(devcfg->regs, channel);
	uint32_t sts = regs->istatus;
	int cb_status = 0;

#ifdef XEC_DMA_DEBUG
	uint8_t idx = channel_isr_idx[channel];

	if (idx < 16) {
		channel_isr_sts[channel][idx] = sts;
		channel_isr_ctrl[channel][idx] = regs->control;
		channel_isr_idx[channel] = ++idx;
	}
#endif
	LOG_DBG("maddr=0x%08x mend=0x%08x daddr=0x%08x ctrl=0x%08x sts=0x%02x", regs->mem_addr,
		regs->mem_addr_end, regs->dev_addr, regs->control, sts);

	regs->ienable = 0u;
	regs->istatus = 0xffu;
	mchp_xec_ecia_girq_src_clr(info[channel].gid, info[channel].gpos);

	chan_data->isr_hw_status = sts;
	chan_data->total_curr_xfr_len += (regs->mem_addr - chan_data->mstart);

	if (sts & BIT(XEC_DMA_CHAN_IES_BERR_POS)) {/* Bus Error? */
		if (!(chan_data->flags & BIT(DMA_XEC_CHAN_FLAGS_CB_ERR_DIS_POS))) {
			cb_status = -EIO;
		}
	}

	if (chan_data->cb) {
		chan_data->cb(dev, chan_data->user_data, channel, cb_status);
	}
}

static int dma_xec_init(const struct device *dev)
{
	const struct dma_xec_config * const devcfg = dev->config;
	struct dma_xec_regs * const regs = devcfg->regs;

	LOG_DBG("driver init");

	z_mchp_xec_pcr_periph_sleep(devcfg->pcr_idx, devcfg->pcr_pos, 0);

	/* soft reset, self-clearing */
	regs->mctrl = BIT(XEC_DMA_MAIN_CTRL_SRST_POS);
	regs->mpkt = 0u; /* I/O delay, write to read-only register */
	regs->mctrl = BIT(XEC_DMA_MAIN_CTRL_EN_POS);

	devcfg->irq_connect();

	return 0;
}

/* n = node-id, p = property, i = index */
#define DMA_XEC_GID(n, p, i) MCHP_XEC_ECIA_GIRQ(DT_PROP_BY_IDX(n, p, i))
#define DMA_XEC_GPOS(n, p, i) MCHP_XEC_ECIA_GIRQ_POS(DT_PROP_BY_IDX(n, p, i))

#define DMA_XEC_GIRQ_INFO(n, p, i)						\
	{									\
		.gid = DMA_XEC_GID(n, p, i),					\
		.gpos = DMA_XEC_GPOS(n, p, i),					\
		.anid = MCHP_XEC_ECIA_NVIC_AGGR(DT_PROP_BY_IDX(n, p, i)),	\
		.dnid = MCHP_XEC_ECIA_NVIC_DIRECT(DT_PROP_BY_IDX(n, p, i)),	\
	},

/* n = node-id, p = property, i = index(channel?) */
#define DMA_XEC_IRQ_DECLARE(node_id, p, i)					\
	static void dma_xec_chan_##i##_isr(const struct device *dev)		\
	{									\
		dma_xec_irq_handler(dev, i);					\
	}									\

#define DMA_XEC_IRQ_CONNECT_SUB(node_id, p, i)					\
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, i, irq),				\
		    DT_IRQ_BY_IDX(node_id, i, priority),			\
		    dma_xec_chan_##i##_isr,					\
		    DEVICE_DT_GET(node_id), 0);					\
	irq_enable(DT_IRQ_BY_IDX(node_id, i, irq));				\
	mchp_xec_ecia_enable(DMA_XEC_GID(node_id, p, i), DMA_XEC_GPOS(node_id, p, i));

/* i = instance number of DMA controller */
#define DMA_XEC_IRQ_CONNECT(inst)							\
	DT_INST_FOREACH_PROP_ELEM(inst, girqs, DMA_XEC_IRQ_DECLARE)			\
	void dma_xec_irq_connect##inst(void)						\
	{										\
		DT_INST_FOREACH_PROP_ELEM(inst, girqs, DMA_XEC_IRQ_CONNECT_SUB)		\
	}

#define DMA_XEC_DEVICE(i)								\
	BUILD_ASSERT(DT_INST_PROP(i, dma_channels) <= 16, "XEC DMA dma-channels > 16");	\
	BUILD_ASSERT(DT_INST_PROP(i, dma_requests) <= 16, "XEC DMA dma-requests > 16");	\
											\
	static struct dma_xec_channel							\
		dma_xec_ctrl##i##_chans[DT_INST_PROP(i, dma_channels)];			\
	ATOMIC_DEFINE(dma_xec_atomic##i, DT_INST_PROP(i, dma_channels));		\
											\
	static struct dma_xec_data dma_xec_data##i = {					\
		.ctx.magic = DMA_MAGIC,							\
		.ctx.dma_channels = DT_INST_PROP(i, dma_channels),			\
		.ctx.atomic = dma_xec_atomic##i,					\
		.channels = dma_xec_ctrl##i##_chans,					\
	};										\
											\
	DMA_XEC_IRQ_CONNECT(i)								\
											\
	static const struct dma_xec_irq_info dma_xec_irqi##i[] = {			\
		DT_INST_FOREACH_PROP_ELEM(i, girqs, DMA_XEC_GIRQ_INFO)			\
	};										\
	static const struct dma_xec_config dma_xec_cfg##i = {				\
		.regs = (struct dma_xec_regs *)DT_INST_REG_ADDR(i),			\
		.dma_channels = DT_INST_PROP(i, dma_channels),				\
		.dma_requests = DT_INST_PROP(i, dma_requests),				\
		.pcr_idx = DT_INST_PROP_BY_IDX(i, pcrs, 0),				\
		.pcr_pos = DT_INST_PROP_BY_IDX(i, pcrs, 1),				\
		.irq_info_size = ARRAY_SIZE(dma_xec_irqi##i),				\
		.irq_info_list = dma_xec_irqi##i,					\
		.irq_connect = dma_xec_irq_connect##i,					\
	};										\
	PM_DEVICE_DT_DEFINE(i, dmac_xec_pm_action);					\
	DEVICE_DT_INST_DEFINE(i, &dma_xec_init,						\
		PM_DEVICE_DT_GET(i),							\
		&dma_xec_data##i, &dma_xec_cfg##i,					\
		PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,					\
		&dma_xec_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_XEC_DEVICE)
