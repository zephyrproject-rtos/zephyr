/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Unified DMA driver for TI LPF3 family (CC23x0, CC27xx).
 * Both SOC lines share the same uDMA IP block and driverlib API.
 */

#define DT_DRV_COMPAT ti_lpf3_dma

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_lpf3, CONFIG_DMA_LOG_LEVEL);

#include <zephyr/arch/arm/cortex_m/memory_map.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>

#include <driverlib/clkctl.h>
#include <driverlib/udma.h>
#include <driverlib/evtsvt.h>

#include <inc/hw_evtsvt.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>

/*
 * DCH (Dedicated Channel) channels have a fixed connection to a specific
 * peripheral's µDMA interface. Only the peripherals listed in the EVTSVT
 * DMACH*SEL register for that channel can trigger it (IPID field, 4-bit).
 *
 * ECH (Event Channel) channels connect to the generic event fabric. Any event
 * publisher in the system can trigger them (PUBID field, 7-bit).
 *
 * Both channel types can be triggered in software via DMA.SOFTREQ
 * with no event source configured.
 *
 * Both channel types support any transfer direction (M2M, M2P, P2M). The
 * direction is determined by how the source/destination addresses are set up.
 *
 * For CC23XX: DCH channels 0-5, ECH channels 6-7.
 * For CC27XX: DCH channels 0-7, ECH channels 8-11.
 */

#if CONFIG_SOC_SERIES_CC27XX
#define DMA_LPF3_PERIPH_CH_MAX 7
#define DMA_LPF3_ECH_CH_MIN    8
#define DMA_LPF3_ECH_CH_MAX    11
#define DMA_LPF3_EVT_PUB_MIN   0x2
#define DMA_LPF3_EVT_PUB_MAX   0x4C
#elif CONFIG_SOC_SERIES_CC23X0
#define DMA_LPF3_PERIPH_CH_MAX 5
#define DMA_LPF3_ECH_CH_MIN    6
#define DMA_LPF3_ECH_CH_MAX    7
#define DMA_LPF3_EVT_PUB_MIN   0x2
#define DMA_LPF3_EVT_PUB_MAX   0x39
#endif

#define DMA_LPF3_IS_ECH_CH(ch) ((ch) >= DMA_LPF3_ECH_CH_MIN)
#define DMA_LPF3_NUM_CHANNELS  (DMA_LPF3_ECH_CH_MAX + 1)

/*
 * Only valid for DCH channels (0-5 on CC23X0, 0-7 on CC27XX). On CC27XX, the
 * EVTSVT registers for ECH channels 8-11 are not sequential (CH10 and CH11
 * precede CH8 and CH9 in the address map), so this formula must not be used
 * for ECH channels.
 */
#define DMA_LPF3_CHXSEL_REG(ch) HWREG(EVTSVT_BASE + EVTSVT_O_DMACH0SEL + sizeof(uint32_t) * (ch))

#ifdef CONFIG_PM_DEVICE
#define DMA_LPF3_ALL_CH_MASK GENMASK(DMA_LPF3_ECH_CH_MAX, 0)
#endif

static const uint32_t dma_lpf3_evtsvt_offsets[] = {
#if CONFIG_SOC_SERIES_CC23X0 || CONFIG_SOC_SERIES_CC27XX
	EVTSVT_O_DMACH0SEL, EVTSVT_O_DMACH1SEL, EVTSVT_O_DMACH2SEL,  EVTSVT_O_DMACH3SEL,
	EVTSVT_O_DMACH4SEL, EVTSVT_O_DMACH5SEL, EVTSVT_O_DMACH6SEL,  EVTSVT_O_DMACH7SEL,
#endif
#if CONFIG_SOC_SERIES_CC27XX
	EVTSVT_O_DMACH8SEL, EVTSVT_O_DMACH9SEL, EVTSVT_O_DMACH10SEL, EVTSVT_O_DMACH11SEL,
#endif
};

/* Valid values for the source_handshake field */
#define HW_TRIGGERED_TRANSFER 0
#define SW_TRIGGERED_TRANSFER 1

#define DMA_LPF3_IS_PERIPH_ADDR(addr)                                                              \
	(((uintptr_t)(addr) >= 0x40000000) && ((uintptr_t)(addr) <= 0x4BFFFFFF))

static uint32_t dma_lpf3_get_evtsvt_offset(uint32_t channel)
{
	return dma_lpf3_evtsvt_offsets[channel];
}

struct dma_lpf3_channel {
	uint8_t data_size;
	uint8_t mode;
	bool trigger;
	dma_callback_t cb;
	void *user_data;
	uint32_t total_size;
#ifdef CONFIG_PM_DEVICE
	bool configured;
	struct dma_block_config dma_blk_cfg;
	struct dma_config dma_cfg;
#endif
};

struct dma_lpf3_data {
	/* dma_context must be the first member for dma_request_channel() */
	struct dma_context ctx;

	ATOMIC_DEFINE(channels_atomic, DMA_LPF3_NUM_CHANNELS);

	__aligned(512) uDMAControlTableEntry desc[UDMA_ALT_SELECT + DMA_LPF3_NUM_CHANNELS];
	struct dma_lpf3_channel channels[DMA_LPF3_NUM_CHANNELS];
};

static inline void dma_cc23x0_pm_policy_state_lock_get(void)
{
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
}

static inline void dma_cc23x0_pm_policy_state_lock_put(void)
{
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
}

/*
 * If the channel is a software channel, then the completion will be signaled
 * on this DMA dedicated interrupt.
 * If a peripheral channel is used, then the completion will be signaled on the
 * peripheral's interrupt.
 */
static void dma_lpf3_isr(const struct device *dev)
{
	struct dma_lpf3_data *data = dev->data;
	struct dma_lpf3_channel *ch_data;
	uint32_t done_flags;
	int i;

	done_flags = uDMAIntStatus();

	for (i = 0; i < DMA_LPF3_NUM_CHANNELS; i++) {
		if ((done_flags & BIT(i))) {
			LOG_DBG("DMA transfer completed on channel %d", i);

			ch_data = &data->channels[i];
			if (ch_data->cb) {
				ch_data->cb(dev, ch_data->user_data, i, DMA_STATUS_COMPLETE);
			}

			uDMAClearInt(BIT(i));
		}

		LOG_DBG("DMA transfer completed on channel %u", i);

		ch_data = &data->channels[i];
		if (ch_data->cb) {
			ch_data->cb(dev, ch_data->user_data, i, DMA_STATUS_COMPLETE);
		}

		uDMAClearInt(done_flags & BIT(i));
	}
}

static uint32_t dma_lpf3_set_addr_adj(uint32_t *control, uint16_t addr_adj, uint32_t inc_flags,
				      uint32_t no_inc_flags, uint32_t inc_mask)
{
	*control = *control & ~inc_mask;
	switch (addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		*control |= inc_flags;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		*control |= no_inc_flags;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int dma_lpf3_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	struct dma_lpf3_data *data = dev->data;
	struct dma_lpf3_channel *ch_data;
	struct dma_block_config *block = config->head_block;
	uint32_t control;
	uint32_t data_size;
	uint32_t src_inc_flags;
	uint32_t dst_inc_flags;
	uint32_t xfer_size;
	uint32_t burst_len;
	int ret;
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state pm_state;
#endif

	if (channel >= DMA_LPF3_NUM_CHANNELS) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	/*
	 * DCH channels accept an IPID (4-bit peripheral trigger ID).
	 * ECH channels accept a PUBID in [EVT_PUB_MIN, EVT_PUB_MAX].
	 */
	if (config->source_handshake == HW_TRIGGERED_TRANSFER) {
		if (DMA_LPF3_IS_ECH_CH(channel)) {
			if (config->dma_slot < DMA_LPF3_EVT_PUB_MIN ||
			    config->dma_slot > DMA_LPF3_EVT_PUB_MAX) {
				LOG_ERR("Channel %d: invalid PUBID %d (valid %d-%d)", channel,
					config->dma_slot, DMA_LPF3_EVT_PUB_MIN,
					DMA_LPF3_EVT_PUB_MAX);
				return -EINVAL;
			}
		} else {
			if (config->dma_slot > EVTSVT_IPID_MAX_VAL) {
				LOG_ERR("Channel %d: invalid IPID %d (max %d)", channel,
					config->dma_slot, EVTSVT_IPID_MAX_VAL);
				return -EINVAL;
			}
		}
	}

	if (config->block_count > 1) {
		LOG_ERR("Chained transfers not supported");
		return -ENOTSUP;
	}

	switch (config->source_data_size) {
	case 1:
		src_inc_flags = UDMA_SRC_INC_8;
		break;
	case 2:
		src_inc_flags = UDMA_SRC_INC_16;
		break;
	case 4:
		src_inc_flags = UDMA_SRC_INC_32;
		break;
	default:
		LOG_ERR("Invalid source data size (%u)", config->source_data_size);
		return -EINVAL;
	}

	switch (config->dest_data_size) {
	case 1:
		dst_inc_flags = UDMA_DST_INC_8;
		break;
	case 2:
		dst_inc_flags = UDMA_DST_INC_16;
		break;
	case 4:
		dst_inc_flags = UDMA_DST_INC_32;
		break;
	default:
		LOG_ERR("Invalid destination data size (%u)", config->dest_data_size);
		return -EINVAL;
	}

	data_size = MIN(config->source_data_size, config->dest_data_size);

	switch (data_size) {
	case 1:
		control = UDMA_SIZE_8;
		break;
	case 2:
		control = UDMA_SIZE_16;
		break;
	case 4:
		control = UDMA_SIZE_32;
		break;
	default:
		LOG_ERR("Invalid data size (%u)", data_size);
		return -EINVAL;
	}

	ret = dma_lpf3_set_addr_adj(&control, block->source_addr_adj, src_inc_flags,
				    UDMA_SRC_INC_NONE, UDMA_SRC_INC_M);
	if (ret) {
		LOG_ERR("Invalid source address adjustment type (%u)", block->source_addr_adj);
		return ret;
	}

	ret = dma_lpf3_set_addr_adj(&control, block->dest_addr_adj, dst_inc_flags,
				    UDMA_DST_INC_NONE, UDMA_DST_INC_M);
	if (ret) {
		LOG_ERR("Invalid dest address adjustment type (%u)", block->dest_addr_adj);
		return ret;
	}

	xfer_size = block->block_size / data_size;
	if (!xfer_size || xfer_size > UDMA_XFER_SIZE_MAX) {
		LOG_ERR("Invalid block size (must be in range %d to %d)", data_size,
			data_size * UDMA_XFER_SIZE_MAX);
		return -EINVAL;
	}

	burst_len = config->source_burst_length / data_size;
	if (config->source_burst_length != data_size * burst_len) {
		LOG_ERR("Source burst length is not a multiple of data size");
		return -EINVAL;
	} else if (config->source_burst_length != config->dest_burst_length) {
		LOG_ERR("Source and destination burst lengths are not equal");
		return -EINVAL;
	} else if ((burst_len <= UDMA_XFER_SIZE_MAX) && IS_POWER_OF_TWO(burst_len)) {
		control |= LOG2(burst_len) << UDMA_ARB_S;
	} else {
		LOG_ERR("Computed burst length must be a power of 2 between %d and %d)", data_size,
			data_size * UDMA_XFER_SIZE_MAX);
		return -EINVAL;
	}

	/*
	 * Reject a reconfigure of a still-enabled channel before touching any
	 * shared channel state below.
	 */
	if (uDMAIsChannelEnabled(BIT(channel))) {
		return -EBUSY;
	}

	ch_data = &data->channels[channel];
	ch_data->data_size = data_size;
	ch_data->total_size = block->block_size;

	/* Interpret source chaining as auto mode */
	ch_data->mode = config->source_chaining_en ? UDMA_MODE_AUTO : UDMA_MODE_BASIC;

	/* Interpret source handshake as hardware or software triggered transfers */
	ch_data->trigger = config->source_handshake;
	ch_data->cb = config->dma_callback;
	ch_data->user_data = config->user_data;

	if (ch_data->trigger == SW_TRIGGERED_TRANSFER) {
		uDMAEnableSwEventInt(BIT(channel));
	} else {
		uint32_t evtsvt_ch = dma_lpf3_get_evtsvt_offset(channel);

		LOG_DBG("Channel %d: configuring EVTSVT trigger %d", channel, config->dma_slot);
		EVTSVTConfigureDma(evtsvt_ch, config->dma_slot);
	}

	uDMASetChannelControl(&data->desc[channel], control);
	uDMASetChannelTransfer(&data->desc[channel], ch_data->mode, (void *)block->source_address,
			       (void *)block->dest_address, xfer_size);

#ifdef CONFIG_PM_DEVICE
	pm_device_state_get(dev, &pm_state);

	/*
	 * Save context only if current function is not being called
	 * from resume operation for restoring channel configuration
	 */
	if (pm_state == PM_DEVICE_STATE_ACTIVE) {
		ch_data->configured = true;

		ch_data->dma_blk_cfg.source_address = block->source_address;
		ch_data->dma_blk_cfg.dest_address = block->dest_address;
		ch_data->dma_blk_cfg.source_addr_adj = block->source_addr_adj;
		ch_data->dma_blk_cfg.dest_addr_adj = block->dest_addr_adj;
		ch_data->dma_blk_cfg.block_size = block->block_size;

		ch_data->dma_cfg.dma_slot = config->dma_slot;
		ch_data->dma_cfg.channel_direction = config->channel_direction;
		ch_data->dma_cfg.source_handshake = config->source_handshake;
		ch_data->dma_cfg.dest_handshake = config->dest_handshake;
		ch_data->dma_cfg.block_count = config->block_count;
		ch_data->dma_cfg.head_block = &ch_data->dma_blk_cfg;
		ch_data->dma_cfg.source_data_size = config->source_data_size;
		ch_data->dma_cfg.dest_data_size = config->dest_data_size;
		ch_data->dma_cfg.source_burst_length = config->source_burst_length;
		ch_data->dma_cfg.dest_burst_length = config->dest_burst_length;
		ch_data->dma_cfg.dma_callback = config->dma_callback;
		ch_data->dma_cfg.user_data = config->user_data;

		LOG_DBG("Configured channel %d for %08x to %08x (%u bytes)", channel,
			block->source_address, block->dest_address, block->block_size);
	}
#else
	LOG_DBG("Configured channel %d for %08x to %08x (%u bytes)", channel, block->source_address,
		block->dest_address, block->block_size);
#endif

	return 0;
}

static int dma_lpf3_stop(const struct device *dev, uint32_t channel)
{
	uDMADisableChannel(BIT(channel));

	return 0;
}

static int dma_lpf3_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			   size_t size)
{
	struct dma_lpf3_data *data = dev->data;
	struct dma_lpf3_channel *ch_data = &data->channels[channel];
	uint32_t xfer_size = size / ch_data->data_size;

	if (uDMAIsChannelEnabled(BIT(channel))) {
		return -EBUSY;
	}

	uDMASetChannelTransfer(&data->desc[channel], ch_data->mode, (void *)src, (void *)dst,
			       xfer_size);

	ch_data->total_size = size;

#ifdef CONFIG_PM_DEVICE
	/* Save context */
	ch_data->dma_blk_cfg.source_address = src;
	ch_data->dma_blk_cfg.dest_address = dst;
	ch_data->dma_blk_cfg.block_size = size;
#endif

	LOG_DBG("Reloaded channel %d for %08x to %08x (%u bytes)", channel, src, dst, size);

	return 0;
}

static int dma_lpf3_get_status(const struct device *dev, uint32_t channel, struct dma_status *stat)
{
	struct dma_lpf3_data *data = dev->data;
	struct dma_lpf3_channel *ch_data;

	const volatile uDMAControlTableEntry *desc;
	bool isSrcPeriph;
	bool isDstPeriph;

	if (channel >= DMA_LPF3_NUM_CHANNELS || !stat) {
		return -EINVAL;
	}

	ch_data = &data->channels[channel];

	/*
	 * Report the real remaining transfer count. uDMAGetChannelSize() returns the
	 * number of items still to transfer (0 when complete); scale by the item size
	 * to get bytes.
	 */
	stat->busy = uDMAIsChannelEnabled(BIT(channel));
	stat->pending_length = uDMAGetChannelSize(&data->desc[channel]) * ch_data->data_size;
	stat->total_copied = ch_data->total_size - stat->pending_length;

	/*
	 * This controller does not support circular buffers (chained transfers
	 * are rejected in dma_lpf3_config()), so these fields are not applicable.
	 */
	stat->free = 0;
	stat->write_position = 0;
	stat->read_position = 0;

	desc = &data->desc[channel];
	isSrcPeriph = DMA_LPF3_IS_PERIPH_ADDR(desc->pSrcEndAddr);
	isDstPeriph = DMA_LPF3_IS_PERIPH_ADDR(desc->pDstEndAddr);

	if (!isSrcPeriph && isDstPeriph) {
		stat->dir = MEMORY_TO_PERIPHERAL;
	} else if (isSrcPeriph && !isDstPeriph) {
		stat->dir = PERIPHERAL_TO_MEMORY;
	} else {
		stat->dir = MEMORY_TO_MEMORY;
	}

	return 0;
}

static int dma_lpf3_start(const struct device *dev, uint32_t channel)
{
	struct dma_lpf3_data *data = dev->data;
	struct dma_lpf3_channel *ch_data = &data->channels[channel];

	if (uDMAIsChannelEnabled(BIT(channel))) {
		return -EBUSY;
	}

	uDMAEnable();
	uDMAEnableChannel(BIT(channel));

	if (ch_data->trigger == SW_TRIGGERED_TRANSFER) {
		LOG_DBG("Starting SW triggered transfer on channel %d", channel);
		uDMARequestChannel(BIT(channel));
	}

	return 0;
}

static bool dma_lpf3_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t filter;

	/* NULL means no filter, take any channel */
	if (!filter_param) {
		return true;
	}

	filter = *((uint32_t *)filter_param);

	return (filter & BIT(channel));
}

static int dma_lpf3_enable(struct dma_lpf3_data *data)
{
	CLKCTLEnable(CLKCTL_BASE, CLKCTL_DMA);
	uDMAEnable();
	/* Set base address for channel control table (descriptors) */
	uDMASetControlBase(data->desc);

	return 0;
}

static int dma_lpf3_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dma_lpf3_isr, DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQN(0));

	return dma_lpf3_enable(dev->data);
}

#ifdef CONFIG_PM_DEVICE

static int dma_lpf3_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct dma_lpf3_data *data = dev->data;
	int i = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/*
		 * We assume that DMA clients (peripheral drivers or applications)
		 * should take care of PM lock/unlock (pm_policy_state_lock_get/put).
		 * This assumption is made for that SoC because:
		 * - If a peripheral channel is used, then the transfer completion is
		 * signaled on the peripheral's interrupt (handled in the DMA client
		 * driver). This operating mode is specific to this SoC.
		 * - If a software channel is used (memory-to-memory transfer), then
		 * the transfer completion can be signaled to the application through
		 * a callback.
		 * Thus, in both cases, the PM can be unlocked at the right time by the
		 * DMA client. When this point is reached, there should not be ongoing
		 * transfer.
		 *
		 * Despite this assumption, ensure that none transfer is ongoing in case
		 * PM state lock was not properly handled by DMA clients.
		 */
		if (uDMAIsChannelEnabled(DMA_LPF3_ALL_CH_MASK)) {
			return -EBUSY;
		}

		uDMADisable();
		CLKCTLDisable(CLKCTL_BASE, CLKCTL_DMA);

		return 0;
	case PM_DEVICE_ACTION_RESUME:
		dma_lpf3_enable(data);

		/* Restore context for the channels that were configured before */
		ARRAY_FOR_EACH_PTR(data->channels, ch_data) {
			if (ch_data->configured) {
				dma_lpf3_config(dev, i, &ch_data->dma_cfg);
			}
			i++;
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

static struct dma_lpf3_data lpf3_data = {
	.ctx = {
		.magic = DMA_MAGIC,
		.dma_channels = DMA_LPF3_NUM_CHANNELS,
		.atomic = lpf3_data.channels_atomic,
	},
};

static DEVICE_API(dma, dma_lpf3_api) = {
	.config = dma_lpf3_config,
	.start = dma_lpf3_start,
	.stop = dma_lpf3_stop,
	.reload = dma_lpf3_reload,
	.get_status = dma_lpf3_get_status,
	.chan_filter = dma_lpf3_chan_filter,
};

PM_DEVICE_DT_INST_DEFINE(0, dma_lpf3_pm_action);

DEVICE_DT_INST_DEFINE(0, &dma_lpf3_init, PM_DEVICE_DT_INST_GET(0), &lpf3_data, NULL, PRE_KERNEL_1,
		      CONFIG_DMA_INIT_PRIORITY, &dma_lpf3_api);
