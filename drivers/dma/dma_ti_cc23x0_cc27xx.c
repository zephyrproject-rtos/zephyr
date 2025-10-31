/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_cc27xx_dma

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_cc23x0_cc27xx, CONFIG_DMA_LOG_LEVEL);

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
 * For CC23XX Channels 0 to 5 are DCH channels assigned to peripherals.
 * Channels 6 and 7 are ECH channels with no specific assignment.
 * For CC27XX Channels 0 to 7 are DCH channels assigned to peripherals.
 * Channels 8 to 11 are ECH channels with no specific assignment.
 */

#if CONFIG_SOC_SERIES_CC27XX
	#define DMA_CC23X0_CC27XX_PERIPH_CH_MAX  7
	#define DMA_CC23X0_CC27XX_ECH_CH_MIN     8
	#define DMA_CC23X0_CC27XX_ECH_CH_MAX     11
#elif CONFIG_SOC_SERIES_CC23X0
	#define DMA_CC23X0_CC27XX_PERIPH_CH_MAX  5
	#define DMA_CC23X0_CC27XX_ECH_CH_MIN     6
	#define DMA_CC23X0_CC27XX_ECH_CH_MAX     7
#endif

#define DMA_CC23X0_CC27XX_IS_ECH_CH(ch) ((ch) >= DMA_CC23X0_CC27XX_ECH_CH_MIN)

/*
 * In basic mode, the DMA controller performs transfers as long as there are more items
 * to transfer, and a transfer request is present. This mode is used with peripherals that
 * assert a DMA request signal whenever the peripheral is ready for a data transfer.
 * Auto mode is similar to basic mode, except that when a transfer request is received,
 * the transfer completes, even if the DMA request is removed. This mode is suitable for
 * software-triggered transfers.
 *
 * Other DMA modes are currently not supported.
 */
#define DMA_CC23X0_CC27XX_MODE(ch)                                                                 \
	(DMA_CC23X0_CC27XX_IS_ECH_CH(ch) ? UDMA_MODE_AUTO : UDMA_MODE_BASIC)

#define DMA_CC23X0_CC27XX_CHXSEL_REG(ch)                                                           \
	HWREG(EVTSVT_BASE + EVTSVT_O_DMACH0SEL + sizeof(uint32_t) * (ch))

#ifdef CONFIG_PM_DEVICE
#define DMA_CC23X0_CC27XX_ALL_CH_MASK GENMASK(DMA_CC23X0_CC27XX_ECH_CH_MAX, 0)
#endif

struct dma_cc23x0_cc27xx_channel {
	uint8_t data_size;
	dma_callback_t cb;
	void *user_data;
};

/* Alternate Control table entries are currently not supported */
struct dma_cc23x0_cc27xx_data {
	__aligned(1024) uDMAControlTableEntry desc[UDMA_NUM_CHANNELS];
	struct dma_cc23x0_cc27xx_channel channels[UDMA_NUM_CHANNELS];
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
static void dma_cc23x0_cc27xx_isr(const struct device *dev)
{
	struct dma_cc23x0_cc27xx_data *data = dev->data;
	struct dma_cc23x0_cc27xx_channel *ch_data;
	uint32_t done_flags;
	int i;

	done_flags = uDMAIntStatus();

	for (i = 0; i < UDMA_NUM_CHANNELS; i++) {
		if ((done_flags & BIT(i)) && !DMA_CC23X0_CC27XX_IS_ECH_CH(i)) {
			LOG_DBG("DMA transfer completed on channel %d", i);

			ch_data = &data->channels[i];
			if (ch_data->cb) {
				ch_data->cb(dev, ch_data->user_data, i, DMA_STATUS_COMPLETE);
			}

			uDMAClearInt(done_flags & BIT(i));
		}

		LOG_DBG("DMA transfer completed on channel %u", i);

		ch_data = &data->channels[i];
		if (ch_data->cb) {
			ch_data->cb(dev, ch_data->user_data, i, DMA_STATUS_COMPLETE);
		}

		uDMAClearInt(done_flags & BIT(i));
	}
}

static uint32_t dma_cc23x0_cc27xx_set_addr_adj(uint32_t *control, uint16_t addr_adj,
				uint32_t inc_flags, uint32_t no_inc_flags, uint32_t inc_mask)
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

static int dma_cc23x0_cc27xx_config(const struct device *dev, uint32_t channel,
				    struct dma_config *config)
{
	struct dma_cc23x0_cc27xx_data *data = dev->data;
	struct dma_cc23x0_cc27xx_channel *ch_data;
	struct dma_block_config *block = config->head_block;
	uint32_t control;
	uint32_t data_size;
	uint32_t src_inc_flags;
	uint32_t dst_inc_flags;
	uint32_t xfer_size;
	uint32_t burst_len;
	int ret;

	if (channel >= UDMA_NUM_CHANNELS) {
		LOG_ERR("Invalid channel (%u)", channel);
		return -EINVAL;
	}

	if (config->dma_slot > EVTSVT_IPID_MAX_VAL) {
		LOG_ERR("Invalid trigger");
		return -EINVAL;
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

	ret = dma_cc23x0_cc27xx_set_addr_adj(&control, block->source_addr_adj, src_inc_flags,
						UDMA_SRC_INC_NONE, UDMA_SRC_INC_M);
	if (ret) {
		LOG_ERR("Invalid source address adjustment type (%u)", block->source_addr_adj);
		return ret;
	}

	ret = dma_cc23x0_cc27xx_set_addr_adj(&control, block->dest_addr_adj, dst_inc_flags,
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
		LOG_ERR("Computed burst length must be a power of 2 between %d and %d)",
				 data_size, data_size * UDMA_XFER_SIZE_MAX);
		return -EINVAL;
	}

	ch_data = &data->channels[channel];
	ch_data->data_size = data_size;
	ch_data->cb = config->dma_callback;
	ch_data->user_data = config->user_data;

	if (uDMAIsChannelEnabled(BIT(channel))) {
		return -EBUSY;
	}

	if (DMA_CC23X0_CC27XX_IS_ECH_CH(channel)) {
		LOG_ERR("ECH channels are not supported");
		return -ENOTSUP;
	} else {
		/* Select peripheral */
		LOG_DBG("Using DCH Channel %d with trigger %d", channel, config->dma_slot);
		EVTSVTConfigureDma(EVTSVT_O_DMACH0SEL + sizeof(uint32_t) * (channel),
							config->dma_slot);
	}
	uDMASetChannelControl(&data->desc[channel], control);
	uDMASetChannelTransfer(&data->desc[channel], DMA_CC23X0_CC27XX_MODE(channel),
			(void *)block->source_address, (void *)block->dest_address,
			xfer_size);

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

static int dma_cc23x0_cc27xx_stop(const struct device *dev, uint32_t channel)
{

	if (DMA_CC23X0_CC27XX_IS_ECH_CH(channel)) {
		LOG_ERR("ECH channels are not supported");
		return -ENOTSUP;
	}

	uDMADisableChannel(BIT(channel));

	return 0;
}

static int dma_cc23x0_cc27xx_reload(const struct device *dev, uint32_t channel, uint32_t src,
				    uint32_t dst, size_t size)
{
	struct dma_cc23x0_cc27xx_data *data = dev->data;
	struct dma_cc23x0_cc27xx_channel *ch_data = &data->channels[channel];
	uint32_t xfer_size = size / ch_data->data_size;

	if (DMA_CC23X0_CC27XX_IS_ECH_CH(channel)) {
		LOG_ERR("ECH channels are not supported");
		return -ENOTSUP;
	}

	if (uDMAIsChannelEnabled(BIT(channel))) {
		return -EBUSY;
	}

	uDMASetChannelTransfer(&data->desc[channel], DMA_CC23X0_CC27XX_MODE(channel), (void *)src,
			       (void *)dst, xfer_size);

#ifdef CONFIG_PM_DEVICE
	/* Save context */
	ch_data->dma_blk_cfg.source_address = src;
	ch_data->dma_blk_cfg.dest_address = dst;
	ch_data->dma_blk_cfg.block_size = size;
#endif

	LOG_DBG("Reloaded channel %d for %08x to %08x (%u bytes)", channel, src, dst, size);

	return 0;
}

static int dma_cc23x0_cc27xx_get_status(const struct device *dev, uint32_t channel,
					struct dma_status *stat)
{
	uint8_t ch_sel;

	if (DMA_CC23X0_CC27XX_IS_ECH_CH(channel)) {
		LOG_ERR("ECH channels are not supported");
		return -ENOTSUP;
	}

	if (channel >= UDMA_NUM_CHANNELS || !stat) {
		return -EINVAL;
	}

	ch_sel = DMA_CC23X0_CC27XX_CHXSEL_REG(channel) & EVTSVT_IPID_MAX_VAL;
	switch (ch_sel) {
	case EVTSVT_DMA_TRIG_UART0RXTRG:
	case EVTSVT_DMA_TRIG_SPI0RXTRG:
	case EVTSVT_DMA_TRIG_LAESTRGB:
		stat->dir = PERIPHERAL_TO_MEMORY;
		break;
	case EVTSVT_DMA_TRIG_LAESTRGA:
	case EVTSVT_DMA_TRIG_ADC0TRG:
	case EVTSVT_DMA_TRIG_SPI0TXTRG:
	case EVTSVT_DMA_TRIG_UART0TXTRG:
		stat->dir = MEMORY_TO_PERIPHERAL;
		break;
	#if CONFIG_SOC_SERIES_CC27XX
	case EVTSVT_DMA_TRIG_UART1RXTRG:
	case EVTSVT_DMA_TRIG_SPI1RXTRG:
	case EVTSVT_DMA_TRIG_CANTRGB:
		stat->dir = PERIPHERAL_TO_MEMORY;
		break;
	case EVTSVT_DMA_TRIG_UART1TXTRG:
	case EVTSVT_DMA_TRIG_SPI1TXTRG:
	case EVTSVT_DMA_TRIG_CANTRGA:
		stat->dir = MEMORY_TO_PERIPHERAL;
		break;
	case EVTSVT_DMA_TRIG_LRFDTRG:
		stat->dir = MEMORY_TO_MEMORY;
	#endif
	default:
		stat->dir = MEMORY_TO_MEMORY;
		break;
	}

	return 0;
}

static int dma_cc23x0_cc27xx_start(const struct device *dev, uint32_t channel)
{

	if (DMA_CC23X0_CC27XX_IS_ECH_CH(channel)) {
		LOG_ERR("ECH channels are not supported");
		return -ENOTSUP;
	}

	if (uDMAIsChannelEnabled(BIT(channel))) {
		return -EBUSY;
	}

	uDMAEnable();
	uDMAEnableChannel(BIT(channel));

	struct dma_status status;

	if (dma_cc23x0_cc27xx_get_status(dev, channel, &status) >= 0) {
		LOG_DBG("Channel %d has direction %d", channel, status.dir);
		if (status.dir == MEMORY_TO_MEMORY) {
			uDMARequestChannel(BIT(channel));
		}
	}

	return 0;
}

static int dma_cc23x0_cc27xx_enable(struct dma_cc23x0_cc27xx_data *data)
{
	struct dma_cc23x0_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		uDMADisable();
		CLKCTLDisable(CLKCTL_BASE, CLKCTL_DMA);
		dma_cc23x0_pm_policy_state_lock_put();
		break;
	case PM_DEVICE_ACTION_RESUME:
		dma_cc23x0_pm_policy_state_lock_get();
		CLKCTLEnable(CLKCTL_BASE, CLKCTL_DMA);
		uDMAEnable();
		/* Set base address for channel control table (descriptors) */
		uDMASetControlBase(data->desc);
		break;
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int dma_cc23x0_cc27xx_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dma_cc23x0_cc27xx_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return dma_cc23x0_cc27xx_enable(dev->data);
}

#ifdef CONFIG_PM_DEVICE

static int dma_cc23x0_cc27xx_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct dma_cc23x0_cc27xx_data *data = dev->data;
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
		if (uDMAIsChannelEnabled(DMA_CC23X0_CC27XX_ALL_CH_MASK)) {
			return -EBUSY;
		}

		uDMADisable();
		CLKCTLDisable(CLKCTL_BASE, CLKCTL_DMA);

		return 0;
	case PM_DEVICE_ACTION_RESUME:
		dma_cc23x0_cc27xx_enable(data);

		/* Restore context for the channels that were configured before */
		ARRAY_FOR_EACH_PTR(data->channels, ch_data) {
			if (ch_data->configured) {
				dma_cc23x0_cc27xx_config(dev, i, &ch_data->dma_cfg);
			}
			i++;
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

static struct dma_cc23x0_cc27xx_data cc23x0_data;

static const struct dma_driver_api dma_cc23x0_cc27xx_api = {
	.config = dma_cc23x0_cc27xx_config,
	.start = dma_cc23x0_cc27xx_start,
	.stop = dma_cc23x0_cc27xx_stop,
	.reload = dma_cc23x0_cc27xx_reload,
	.get_status = dma_cc23x0_cc27xx_get_status,
};

PM_DEVICE_DT_INST_DEFINE(0, dma_cc23x0_cc27xx_pm_action);

DEVICE_DT_INST_DEFINE(0, &dma_cc23x0_cc27xx_init, PM_DEVICE_DT_INST_GET(0), &cc23x0_data, NULL,
		      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY, &dma_cc23x0_cc27xx_api);
