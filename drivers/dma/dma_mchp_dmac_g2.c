/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dma_mchp_dmac_g2.c
 * @brief Zephyr DMA driver for Microchip G2 peripherals
 *
 * Implements DMA API support for Microchip DMAC peripherals
 */

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT microchip_dmac_g2_dma

LOG_MODULE_REGISTER(dma_mchp_dmac_g2, CONFIG_DMA_LOG_LEVEL);

#define DMAC_REG ((const struct dma_mchp_dev_config *)(dev)->config)->regs

#define DMAC_BUF_ADDR_ALIGNMENT   4U
#define DMAC_BUF_SIZE_ALIGNMENT   4U
#define DMAC_COPY_ALIGNMENT       4U
#define DMAC_MAX_BLOCK_COUNT      65535U
#define DMAC_DESCRIPTOR_ALIGNMENT 16U

/* Timeout values for WAIT_FOR macro */
#define TIMEOUT_VALUE_US 1000
#define DELAY_US         2

enum dma_mchp_ch_state {
	DMA_MCHP_CH_IDLE,
	DMA_MCHP_CH_PENDING,
	DMA_MCHP_CH_SUSPENDED,
	DMA_MCHP_CH_ACTIVE,
	DMA_MCHP_CH_PREPARED,
};

struct dma_mchp_channel_config {
	dma_callback_t cb;
	void *user_data;
	bool is_err_cb_dis;
	bool is_configured;
};

struct dma_mchp_dmac {
	/* DMA descriptors for channel configurations */
	dmac_descriptor_registers_t *descriptors;

	/* DMA write-back descriptors for tracking completed transfers */
	dmac_descriptor_registers_t *descriptors_wb;
};

struct dma_mchp_dev_config {
	dmac_registers_t *regs;

	/* Pointer to the clock device used for controlling the DMA's clock. */
	const struct device *clock_dev;

	/* Contains the clock control configuration for the DMA subsystem. */
	clock_control_subsys_t mclk_sys;

	uint8_t num_irq;

	/* Function pointer for configuring DMA interrupts. */
	void (*irq_config)(void);
};

struct dma_mchp_dev_data {
	struct dma_context dma_ctx;
	struct dma_mchp_dmac *dmac_desc_data;
	struct dma_mchp_channel_config *dma_channel_config;
};

static enum dma_mchp_ch_state dmac_ch_get_state(const struct device *dev, uint32_t channel)
{
	enum dma_mchp_ch_state ch_state;
	uint32_t active_status;
	uint8_t ch_int_flag, ch_status;
	unsigned int key;

	key = irq_lock();

	/* Select the channel */
	DMAC_REG->DMAC_CHID = channel;

	/* Read channel status and interrupt flag */
	ch_status = DMAC_REG->DMAC_CHSTATUS;
	ch_int_flag = DMAC_REG->DMAC_CHINTFLAG;
	active_status = DMAC_REG->DMAC_ACTIVE;

	irq_unlock(key);

	/* Check if the channel is busy */
	if ((ch_status & DMAC_CHSTATUS_BUSY_Msk) == DMAC_CHSTATUS_BUSY_Msk) {

		if (((active_status & DMAC_ACTIVE_ABUSY_Msk) == DMAC_ACTIVE_ABUSY_Msk) &&
		    (((active_status & DMAC_ACTIVE_ID_Msk) >> DMAC_ACTIVE_ID_Pos) == channel)) {
			/* Check if the channel is currently active */
			ch_state = DMA_MCHP_CH_ACTIVE;
		} else if ((ch_int_flag & DMAC_CHINTFLAG_SUSP_Msk) == DMAC_CHINTFLAG_SUSP_Msk) {
			/* Check if the channel is suspended */
			ch_state = DMA_MCHP_CH_SUSPENDED;
		} else {
			/* Otherwise, mark as idle */
			ch_state = DMA_MCHP_CH_IDLE;
		}
	}
	/* Check if the channel is pending */
	else if ((ch_status & DMAC_CHSTATUS_PEND_Msk) == DMAC_CHSTATUS_PEND_Msk) {
		ch_state = DMA_MCHP_CH_PENDING;
	}
	/* Explicitly setting to IDLE for clarity */
	else {
		ch_state = DMA_MCHP_CH_IDLE;
	}

	return ch_state;
}

static inline void dmac_desc_init(const struct device *dev)
{
	struct dma_mchp_dmac *data =
		((const struct dma_mchp_dev_data *)(dev)->data)->dmac_desc_data;

	DMAC_REG->DMAC_BASEADDR = (uintptr_t)data->descriptors;
	DMAC_REG->DMAC_WRBADDR = (uintptr_t)data->descriptors_wb;
}

static int dmac_desc_block_config(struct dma_block_config *block, void *desc_ptr,
				  void *pre_desc_ptr, uint32_t src_data_size)
{
	/* Descriptors typecast */
	dmac_descriptor_registers_t *desc = (dmac_descriptor_registers_t *)desc_ptr;
	dmac_descriptor_registers_t *pre_desc = (dmac_descriptor_registers_t *)pre_desc_ptr;

	uint16_t btctrl = 0;

	/* Set the DMAC Block Control */
	switch (src_data_size) {
	case 1:
		btctrl |= DMAC_BTCTRL_BEATSIZE_BYTE;
		break;
	case 2:
		btctrl |= DMAC_BTCTRL_BEATSIZE_HWORD;
		break;
	case 4:
		btctrl |= DMAC_BTCTRL_BEATSIZE_WORD;
		break;
	default:
		LOG_ERR("Invalid parameter for DMA source data size");
		return -EINVAL;
	}

	/* Set the block transfer count */
	desc->DMAC_BTCNT = (uint16_t)(block->block_size / src_data_size);
	desc->DMAC_DESCADDR = 0;

	/* Set the source address */
	switch (block->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		desc->DMAC_SRCADDR = block->source_address + block->block_size;
		btctrl |= DMAC_BTCTRL_SRCINC(1);
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		desc->DMAC_SRCADDR = block->source_address;
		break;
	default:
		LOG_ERR("Invalid parameter for DMA source address");
		return -EINVAL;
	}

	/* Set the destination address */
	switch (block->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		desc->DMAC_DSTADDR = block->dest_address + block->block_size;
		btctrl |= DMAC_BTCTRL_DSTINC(1);
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		desc->DMAC_DSTADDR = block->dest_address;
		break;
	default:
		LOG_ERR("Invalid parameter for DMA destination address");
		return -EINVAL;
	}

	btctrl |= DMAC_BTCTRL_VALID(1);
	desc->DMAC_BTCTRL = btctrl;

	/* Attach the current descriptor to the previous descriptor at the end */
	if (pre_desc != NULL) {
		pre_desc->DMAC_DESCADDR = (uint32_t)desc;
	}

	return 0;
}

static int dmac_desc_reload_block(struct dma_mchp_dmac *data, uint32_t channel, uint32_t src,
				  uint32_t dst, size_t size)
{
	dmac_descriptor_registers_t *desc = &data->descriptors[channel];
	dmac_descriptor_registers_t *wb_desc = &data->descriptors_wb[channel];

	/* check if already multiple blocks are configured */
	if (desc->DMAC_DESCADDR != 0) {
		return -EINVAL;
	}

	/* Update the block transfer count based on beat size */
	switch (((DMAC_BTCTRL_BEATSIZE_Msk & desc->DMAC_BTCTRL) >> DMAC_BTCTRL_BEATSIZE_Pos)) {
	case DMAC_BTCTRL_BEATSIZE_BYTE_Val:
		desc->DMAC_BTCNT = (uint16_t)size;
		break;
	case DMAC_BTCTRL_BEATSIZE_HWORD_Val:
		desc->DMAC_BTCNT = (uint16_t)(size / 2U);
		break;
	case DMAC_BTCTRL_BEATSIZE_WORD_Val:
		desc->DMAC_BTCNT = (uint16_t)(size / 4U);
		break;
	default:
		LOG_ERR("Invalid configuration beat size");
		return -EINVAL;
	}

	/* Update the source address */
	if ((DMAC_BTCTRL_SRCINC_Msk & desc->DMAC_BTCTRL) != 0) {
		desc->DMAC_SRCADDR = src + size;
	} else {
		desc->DMAC_SRCADDR = src;
	}

	/* Update the destination address */
	if ((DMAC_BTCTRL_DSTINC_Msk & desc->DMAC_BTCTRL) != 0) {
		desc->DMAC_DSTADDR = dst + size;
	} else {
		desc->DMAC_DSTADDR = dst;
	}

	/* Sync WB registers */
	wb_desc->DMAC_BTCNT = desc->DMAC_BTCNT;
	wb_desc->DMAC_SRCADDR = desc->DMAC_SRCADDR;
	wb_desc->DMAC_DSTADDR = desc->DMAC_DSTADDR;

	return 0;
}

static int dma_mchp_validate(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dmac_ch_get_state(dev, channel) == DMA_MCHP_CH_ACTIVE) {
		LOG_ERR("DMA channel %d is already in use", channel);
		return -EBUSY;
	}

	if (config->source_data_size != config->dest_data_size) {
		LOG_ERR("Source and destination data sizes do not match");
		return -EINVAL;
	}

	if (config->dma_slot >= DMAC_TRIG_NUM) {
		LOG_ERR("Invalid parameter for DMA trigger source : %d", config->dma_slot);
		return -EINVAL;
	}

	if (config->channel_priority >= DMAC_LVL_NUM) {
		LOG_ERR("Invalid DMA priority level : %d", config->channel_priority);
		return -EINVAL;
	}

	switch (config->channel_direction) {
	case MEMORY_TO_MEMORY:
	case MEMORY_TO_PERIPHERAL:
	case PERIPHERAL_TO_MEMORY:
		break;

	default:
		LOG_ERR("Invalid DMA channel direction");
		return -EINVAL;
	}

	return 0;
}

static int dma_mchp_setup_channel(const struct device *dev, uint32_t channel,
				  const struct dma_config *config)
{
	unsigned int key;
	uint32_t chctrlb = 0;

	/* Set the trigger source */
	if (config->channel_direction == MEMORY_TO_MEMORY) {
		chctrlb = DMAC_CHCTRLB_TRIGACT_TRANSACTION | DMAC_CHCTRLB_TRIGSRC(config->dma_slot);
	} else {
		chctrlb = DMAC_CHCTRLB_TRIGACT_BEAT | DMAC_CHCTRLB_TRIGSRC(config->dma_slot);
	}
	/* Set channel priority */
	chctrlb |= DMAC_CHCTRLB_LVL(config->channel_priority);

	key = irq_lock();

	/* Select the channel */
	DMAC_REG->DMAC_CHID = channel;

	DMAC_REG->DMAC_CHCTRLB = chctrlb;

	/* Enable the interrupts */
	DMAC_REG->DMAC_CHINTENSET = DMAC_CHINTENSET_TCMPL(1);
	/* Enable or disable transfer error interrupt */
	if (!config->error_callback_dis) {
		DMAC_REG->DMAC_CHINTENSET = DMAC_CHINTENSET_TERR(1);
	} else {
		DMAC_REG->DMAC_CHINTENCLR = DMAC_CHINTENCLR_TERR(1);
	}

	/* Clear pending interrupt flags */
	DMAC_REG->DMAC_CHINTFLAG = DMAC_CHINTFLAG_TERR_Msk | DMAC_CHINTFLAG_TCMPL_Msk;

	irq_unlock(key);

	return 0;
}

static int dma_mchp_desc_setup(struct dma_mchp_dev_data *dev_data, struct dma_config *config,
			       void *base_desc)
{
	struct dma_block_config *block = config->head_block;
	void *desc = base_desc;
	int ret;

	if (config->block_count > 1) {
		LOG_ERR("Multi block transfers not supported");
		return -ENOTSUP;
	}

	/* Configure head block */
	ret = dmac_desc_block_config(block, desc, NULL, config->source_data_size);
	if (ret < 0) {
		LOG_ERR("DMA Error: Block 1 configuration failed!");
	}

	return ret;
}

static void dma_mchp_isr(const struct device *dev)
{
	struct dma_mchp_dev_data *dev_data = dev->data;
	uint16_t pend = DMAC_REG->DMAC_INTPEND;
	uint32_t channel = (pend & DMAC_INTPEND_ID_Msk) >> DMAC_INTPEND_ID_Pos;

	/* Acknowledge interrupt immediately */
	DMAC_REG->DMAC_INTPEND = pend;

	/* Ignore non TC / ERR interrupts */
	if ((pend & (DMAC_INTPEND_TERR_Msk | DMAC_INTPEND_TCMPL_Msk)) == 0) {
		return;
	}

	struct dma_mchp_channel_config *cfg = &dev_data->dma_channel_config[channel];

	if (cfg->cb == NULL) {
		return;
	}

	if (pend & DMAC_INTPEND_TERR_Msk) {
		/* Invoke error callback only if it is not disabled */
		if (cfg->is_err_cb_dis == false) {
			cfg->cb(dev, cfg->user_data, channel, -EIO);
		}
	} else {
		cfg->cb(dev, cfg->user_data, channel, DMA_STATUS_COMPLETE);
	}
}

static int dma_mchp_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	struct dma_mchp_channel_config *channel_config;
	int ret;

	ret = dma_mchp_validate(dev, channel, config);
	if (ret != 0) {
		return ret;
	}

	ret = dma_mchp_setup_channel(dev, channel, config);
	if (ret != 0) {
		return ret;
	}

	void *base_desc = &dev_data->dmac_desc_data->descriptors[channel];

	ret = dma_mchp_desc_setup(dev_data, config, base_desc);
	if (ret != 0) {
		return ret;
	}

	atomic_set_bit(dev_data->dma_ctx.atomic, channel);

	channel_config = &dev_data->dma_channel_config[channel];
	channel_config->cb = config->dma_callback;
	channel_config->user_data = config->user_data;
	channel_config->is_configured = true;
	channel_config->is_err_cb_dis = config->error_callback_dis;

	return ret;
}

static int dma_mchp_start(const struct device *dev, uint32_t channel)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	unsigned int key;

	/* Validate channel number */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check if the channel is already in use */
	if (dmac_ch_get_state(dev, channel) == DMA_MCHP_CH_ACTIVE) {
		LOG_ERR("DMA channel:%d is currently busy", channel);
		return -EBUSY;
	}

	/* Check if the channel is configured */
	if (dev_data->dma_channel_config[channel].is_configured != true) {
		LOG_ERR("DMA descriptors not configured for channel : %d", channel);
		return -EINVAL;
	}

	key = irq_lock();

	/* Select the channel */
	DMAC_REG->DMAC_CHID = channel;

	/* Enable the DMA channel and start the transfer */
	DMAC_REG->DMAC_CHCTRLA |= DMAC_CHCTRLA_ENABLE_Msk;
	/* Software trigger if no hardware trigger source */
	if ((DMAC_REG->DMAC_CHCTRLB & DMAC_CHCTRLB_TRIGSRC_Msk) == 0) {
		DMAC_REG->DMAC_SWTRIGCTRL = BIT(channel);
	}

	irq_unlock(key);

	return 0;
}

static int dma_mchp_stop(const struct device *dev, uint32_t channel)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	unsigned int key;

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	key = irq_lock();

	/* Select the channel */
	DMAC_REG->DMAC_CHID = channel;

	/* Disable the channel */
	DMAC_REG->DMAC_CHCTRLA &= ~DMAC_CHCTRLA_ENABLE_Msk;

	irq_unlock(key);

	return 0;
}

static int dma_mchp_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			   size_t size)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	int ret;

	/* Validate channel number */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check if the channel is already in use */
	if (dmac_ch_get_state(dev, channel) == DMA_MCHP_CH_ACTIVE) {
		LOG_ERR("DMA channel:%d is currently busy", channel);
		return -EBUSY;
	}

	/* Check if the channel is configured */
	if (dev_data->dma_channel_config[channel].is_configured != true) {
		LOG_ERR("DMA descriptors not configured for channel : %d", channel);
		return -EINVAL;
	}

	/* Reloads the DMA descriptor with the specified source, destination, and size.
	 * Returns an error code if the provided parameters are invalid.
	 */
	ret = dmac_desc_reload_block(dev_data->dmac_desc_data, channel, src, dst, size);
	if (ret < 0) {
		LOG_DBG("Reload channel %d for %08X to %08X (%u) failed!", channel, src, dst, size);
	} else {
		LOG_DBG("Reloaded channel %d for %08X to %08X (%u)", channel, src, dst, size);
	}

	return ret;
}

static int dma_mchp_suspend(const struct device *dev, uint32_t channel)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	enum dma_mchp_ch_state ch_state;
	uint32_t chctrlb;
	unsigned int key;

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check if the channel is currently active */
	ch_state = dmac_ch_get_state(dev, channel);
	if (ch_state != DMA_MCHP_CH_ACTIVE) {
		LOG_INF("nothing to suspend as dma channel %u is not busy", channel);
	}

	key = irq_lock();

	/* Select the channel */
	DMAC_REG->DMAC_CHID = channel;

	/* Read-modify-write CHCTRLB */
	chctrlb = DMAC_REG->DMAC_CHCTRLB;
	chctrlb &= ~DMAC_CHCTRLB_CMD_Msk;
	chctrlb |= DMAC_CHCTRLB_CMD_SUSPEND;
	DMAC_REG->DMAC_CHCTRLB = chctrlb;

	irq_unlock(key);

	LOG_DBG("channel %u is suspended", channel);

	return 0;
}

static int dma_mchp_resume(const struct device *dev, uint32_t channel)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	enum dma_mchp_ch_state ch_state;
	uint32_t chctrlb;
	unsigned int key;

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check if the channel is already suspended */
	ch_state = dmac_ch_get_state(dev, channel);
	if (ch_state != DMA_MCHP_CH_SUSPENDED) {
		LOG_INF("DMA channel %d is not in suspended state so cannot resume channel",
			channel);
		return -EINVAL;
	}

	key = irq_lock();

	/* Select the channel */
	DMAC_REG->DMAC_CHID = channel;

	/* Resume command (RMW) */
	chctrlb = DMAC_REG->DMAC_CHCTRLB;
	chctrlb &= ~DMAC_CHCTRLB_CMD_Msk;
	chctrlb |= DMAC_CHCTRLB_CMD_RESUME;
	DMAC_REG->DMAC_CHCTRLB = chctrlb;

	/* Clear the SUSP interrupt flag */
	DMAC_REG->DMAC_CHINTFLAG = DMAC_CHINTFLAG_SUSP_Msk;

	irq_unlock(key);

	return 0;
}

static int dma_mchp_get_status(const struct device *dev, uint32_t channel, struct dma_status *stat)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	struct dma_mchp_dmac *data = dev_data->dmac_desc_data;
	unsigned int key;
	uint8_t ch_status;
	uint32_t active;

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	key = irq_lock();

	/* Select the channel */
	DMAC_REG->DMAC_CHID = channel;

	/* Read registers */
	ch_status = DMAC_REG->DMAC_CHSTATUS;
	active = DMAC_REG->DMAC_ACTIVE;
	/* Evaluate state and BTCNT atomically */
	if (((ch_status & DMAC_CHSTATUS_BUSY_Msk) == DMAC_CHSTATUS_BUSY_Msk) &&
	    ((active & DMAC_ACTIVE_ABUSY_Msk) == DMAC_ACTIVE_ABUSY_Msk) &&
	    (((active & DMAC_ACTIVE_ID_Msk) >> DMAC_ACTIVE_ID_Pos) == channel)) {

		stat->busy = true;
		stat->pending_length = (active & DMAC_ACTIVE_BTCNT_Msk) >> DMAC_ACTIVE_BTCNT_Pos;
	} else {
		stat->busy = false;
		stat->pending_length = data->descriptors_wb[channel].DMAC_BTCNT;
	}

	irq_unlock(key);

	/* Adjust pending length based on beat size */
	switch (((DMAC_BTCTRL_BEATSIZE_Msk & data->descriptors[channel].DMAC_BTCTRL) >>
		 DMAC_BTCTRL_BEATSIZE_Pos)) {
	case DMAC_BTCTRL_BEATSIZE_BYTE_Val:
		/* No adjustment needed for byte-sized beats */
		break;
	case DMAC_BTCTRL_BEATSIZE_HWORD_Val:
		stat->pending_length *= 2U;
		break;
	case DMAC_BTCTRL_BEATSIZE_WORD_Val:
		stat->pending_length *= 4U;
		break;
	default:
		LOG_ERR("Invalid configuration beat size");
		return -EINVAL;
	}

	return 0;
}

static bool dma_mchp_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	/* Device is not used in current logic, but kept for API compatibility */
	ARG_UNUSED(dev);

	/* If no specific channel is requested, allow any available channel */
	if (filter_param == NULL) {
		return true;
	}

	/* Allow requested channel if free */
	return (channel == *(uint32_t *)filter_param);
}

static int dma_mchp_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	enum dma_attribute_type attr = (enum dma_attribute_type)type;

	/* Device is not used in current logic, but kept for API compatibility */
	ARG_UNUSED(dev);

	switch (attr) {
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = DMAC_BUF_ADDR_ALIGNMENT;
		break;

	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = DMAC_BUF_SIZE_ALIGNMENT;
		break;

	case DMA_ATTR_COPY_ALIGNMENT:
		*value = DMAC_COPY_ALIGNMENT;
		break;

	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = DMAC_MAX_BLOCK_COUNT;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int dma_mchp_init(const struct device *dev)
{
	/* Get device configuration */
	const struct dma_mchp_dev_config *dev_cfg = dev->config;
	int ret;

	/* Enable DMA clock */
	ret = clock_control_on(dev_cfg->clock_dev, dev_cfg->mclk_sys);

	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("Failed to enable MCLK for DMA: %d", ret);
		return ret;
	}

	/* Reset the DMA controller */
	DMAC_REG->DMAC_CTRL &= ~DMAC_CTRL_DMAENABLE_Msk;
	DMAC_REG->DMAC_CTRL |= DMAC_CTRL_SWRST_Msk;

	if (!WAIT_FOR(((DMAC_REG->DMAC_CTRL & DMAC_CTRL_SWRST_Msk) == 0), TIMEOUT_VALUE_US,
		      k_busy_wait(DELAY_US))) {
		LOG_ERR("DMAC reset timed out");
		return -ETIMEDOUT;
	}

	/* Initialize DMA descriptors */
	dmac_desc_init(dev);

	/* Set default priority levels */
	DMAC_REG->DMAC_PRICTRL0 = DMAC_PRICTRL0_LVLPRI0(0) | DMAC_PRICTRL0_LVLPRI1(1) |
				  DMAC_PRICTRL0_LVLPRI2(2) | DMAC_PRICTRL0_LVLPRI3(3);

	/* Enable the DMA controller */
	DMAC_REG->DMAC_CTRL = DMAC_CTRL_DMAENABLE(1) | DMAC_CTRL_LVLEN(0x0F);

	/* Configure DMA interrupt */
	dev_cfg->irq_config();

	/* If everything OK but clocks were already on, return 0 */
	return 0;
}

/* DMA driver API structure. */
static DEVICE_API(dma, dma_mchp_api) = {
	.config = dma_mchp_config,
	.start = dma_mchp_start,
	.stop = dma_mchp_stop,
	.reload = dma_mchp_reload,
	.get_status = dma_mchp_get_status,
	.suspend = dma_mchp_suspend,
	.resume = dma_mchp_resume,
	.chan_filter = dma_mchp_chan_filter,
	.get_attribute = dma_mchp_get_attribute,
};

/* Declare the DMA IRQ connection handler for a specific instance. */
#define DMA_MCHP_IRQ_HANDLER_DECL(n) static void mchp_dma_irq_connect_##n(void)

/* Enable IRQ lines for the DMA controller. */
#define DMA_MCHP_IRQ_CONNECT(idx, n)                                                               \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, idx), (                                                  \
		/** Connect the IRQ to the DMA ISR */                                              \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq),                                       \
			DT_INST_IRQ_BY_IDX(n, idx, priority),                                      \
			dma_mchp_isr,                                                              \
			DEVICE_DT_INST_GET(n), 0);                                                 \
		/** Enable the IRQ */                                                              \
		irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));                                       \
	))

/* Define the DMA IRQ handler function for a given instance. */
#define DMA_MCHP_IRQ_HANDLER(n)                                                                    \
	static void mchp_dma_irq_connect_##n(void)                                                 \
	{                                                                                          \
		/** Connect all IRQs for this instance */                                          \
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)),          \
			DMA_MCHP_IRQ_CONNECT,                 \
			(),                                   \
			n)                                    \
	}

/* DMA runtime data structure. */
#define DMA_MCHP_DATA_DEFN(n)                                                                      \
	static __aligned(DMAC_DESCRIPTOR_ALIGNMENT) dmac_descriptor_registers_t                    \
		dma_desc_##n[DT_INST_PROP(n, dma_channels)];                                       \
	static __aligned(DMAC_DESCRIPTOR_ALIGNMENT) dmac_descriptor_registers_t                    \
		dma_desc_wb_##n[DT_INST_PROP(n, dma_channels)];                                    \
	static struct dma_mchp_dmac dmac_desc_data_##n = {                                         \
		.descriptors = dma_desc_##n,                                                       \
		.descriptors_wb = dma_desc_wb_##n,                                                 \
	};                                                                                         \
	ATOMIC_DEFINE(dma_mchp_atomic##n, DT_INST_PROP(n, dma_channels));                          \
	static struct dma_mchp_channel_config                                                      \
		dma_channel_config_##n[DT_INST_PROP(n, dma_channels)];                             \
	static struct dma_mchp_dev_data dma_mchp_dev_data_##n = {                                  \
		.dma_ctx =                                                                         \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_mchp_atomic##n,                                      \
				.dma_channels = DT_INST_PROP(n, dma_channels),                     \
			},                                                                         \
		.dmac_desc_data = &dmac_desc_data_##n,                                             \
		.dma_channel_config = dma_channel_config_##n,                                      \
	};

/* DMA device configuration structure. */
#define DMA_MCHP_CONFIG_DEFN(n)                                                                    \
	static const struct dma_mchp_dev_config dma_mchp_dev_config_##n = {                        \
		.regs = ((dmac_registers_t *)DT_INST_REG_ADDR(n)),                                 \
		.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),             \
		.num_irq = DT_NUM_IRQS(DT_DRV_INST(n)),                                            \
		.irq_config = mchp_dma_irq_connect_##n,                                            \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock))};

/* Define and initialize the DMA device. */
#define DMA_MCHP_DEVICE_INIT(n)                                                                    \
	DMA_MCHP_IRQ_HANDLER_DECL(n);                                                              \
	DMA_MCHP_DATA_DEFN(n);                                                                     \
	DMA_MCHP_CONFIG_DEFN(n);                                                                   \
	DEVICE_DT_INST_DEFINE(n, &dma_mchp_init, NULL, &dma_mchp_dev_data_##n,                     \
			      &dma_mchp_dev_config_##n, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,    \
			      &dma_mchp_api);                                                      \
	DMA_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(DMA_MCHP_DEVICE_INIT);
