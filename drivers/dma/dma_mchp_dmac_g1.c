/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dma_mchp_dmac_g1.c
 * @brief Zephyr DMA driver for Microchip G1 peripherals
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

#define DT_DRV_COMPAT microchip_dmac_g1_dma

LOG_MODULE_REGISTER(dma_mchp_dmac_g1, CONFIG_DMA_LOG_LEVEL);

#define DMAC_REG ((const struct dma_mchp_dev_config *)(dev)->config)->regs

#define DMAC_BUF_ADDR_ALIGNMENT 4
#define DMAC_BUF_SIZE_ALIGNMENT 4
#define DMAC_COPY_ALIGNMENT     4
#define DMAC_MAX_BLOCK_COUNT    65535

/* Timeout values for WAIT_FOR macro */
#define TIMEOUT_VALUE_US 1000
#define DELAY_US         2

enum dma_mchp_int_sts {
	DMA_MCHP_INT_ERROR = -1,
	DMA_MCHP_INT_SUCCESS = 0,
	DMA_MCHP_INT_SUSPENDED = 1,
};

enum dma_mchp_ch_state {
	DMA_MCHP_CH_IDLE,
	DMA_MCHP_CH_PENDING,
	DMA_MCHP_CH_SUSPENDED,
	DMA_MCHP_CH_ACTIVE,
	DMA_MCHP_CH_PREPARED,
};

enum dma_mchp_attribute_type {
	DMA_MCHP_ATTR_BUFFER_ADDRESS_ALIGNMENT,
	DMA_MCHP_ATTR_BUFFER_SIZE_ALIGNMENT,
	DMA_MCHP_ATTR_COPY_ALIGNMENT,
	DMA_MCHP_ATTR_MAX_BLOCK_COUNT,
};

struct dma_mchp_channel_config {
	dma_callback_t cb;
	void *user_data;
	bool is_configured;
};

struct dma_mchp_dmac {
	/* DMA descriptors for channel configurations (must be 16-byte aligned). */
	__aligned(16) dmac_descriptor_registers_t descriptors[DMAC_CH_NUM];

	/* DMA write-back descriptors for tracking completed transfers (16-byte aligned). */
	__aligned(16) dmac_descriptor_registers_t descriptors_wb[DMAC_CH_NUM];
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

static int8_t dmac_interrupt_handle_status(dmac_registers_t *dmac_reg, int *channel)
{
	/* Read interrupt status */
	uint16_t pend = dmac_reg->DMAC_INTPEND;
	int8_t interrupt_status_code;

	/* Get the channel number */
	*channel = (pend & DMAC_INTPEND_ID_Msk) >> DMAC_INTPEND_ID_Pos;

	/* Acknowledge all interrupts */
	dmac_reg->DMAC_INTPEND = pend;

	/* Determine the status code */
	if (pend & DMAC_INTPEND_TERR_Msk) {
		interrupt_status_code = DMA_MCHP_INT_ERROR;
	} else if (pend & DMAC_INTPEND_TCMPL_Msk) {
		interrupt_status_code = DMA_MCHP_INT_SUCCESS;
	} else {
		interrupt_status_code = DMA_MCHP_INT_SUCCESS;
	}

	return interrupt_status_code;
}

static inline void dmac_controller_reset(dmac_registers_t *dmac_reg)
{
	dmac_reg->DMAC_CTRL &= ~DMAC_CTRL_DMAENABLE_Msk;
	dmac_reg->DMAC_CTRL |= DMAC_CTRL_SWRST_Msk;

	if (!WAIT_FOR(((dmac_reg->DMAC_CTRL & DMAC_CTRL_SWRST_Msk) == 0), TIMEOUT_VALUE_US,
		      k_busy_wait(DELAY_US))) {
		LOG_ERR("DMAC reset timed out");
	}
}

static inline void dmac_enable(dmac_registers_t *dmac_reg)
{
	dmac_reg->DMAC_CTRL = DMAC_CTRL_DMAENABLE(1) | DMAC_CTRL_LVLEN(0x0F);
}

static inline void dmac_disable(dmac_registers_t *dmac_reg)
{
	dmac_reg->DMAC_CTRL &= ~DMAC_CTRL_DMAENABLE_Msk;
}

static inline void dmac_set_default_priority(dmac_registers_t *dmac_reg)
{
	dmac_reg->DMAC_PRICTRL0 = DMAC_PRICTRL0_LVLPRI0(0) | DMAC_PRICTRL0_LVLPRI1(1) |
				  DMAC_PRICTRL0_LVLPRI2(2) | DMAC_PRICTRL0_LVLPRI3(3);
}

static int8_t dmac_ch_set_trig_src_n_dir(dmac_registers_t *dmac_reg, uint8_t channel,
					 uint8_t trig_src,
					 enum dma_channel_direction channel_direction)
{
	/* Validate trigger source */
	if (trig_src >= DMAC_TRIG_NUM) {
		LOG_ERR("Invalid parameter for DMA trigger source : %d", trig_src);
		return -EINVAL;
	}

	if (channel_direction == MEMORY_TO_MEMORY) {
		/* A single software trigger will start the transfer */
		dmac_reg->CHANNEL[channel].DMAC_CHCTRLA =
			DMAC_CHCTRLA_TRIGACT_TRANSACTION | DMAC_CHCTRLA_TRIGSRC(trig_src);

	} else if ((channel_direction == MEMORY_TO_PERIPHERAL) ||
		   (channel_direction == PERIPHERAL_TO_MEMORY)) {
		/* One peripheral trigger per beat */
		dmac_reg->CHANNEL[channel].DMAC_CHCTRLA =
			DMAC_CHCTRLA_TRIGACT_BURST | DMAC_CHCTRLA_TRIGSRC(trig_src);

	} else {
		LOG_ERR("Invalid parameter for DMA channel direction");
		return -EINVAL;
	}

	return 0;
}

static inline int8_t dmac_ch_set_priority(dmac_registers_t *dmac_reg, uint8_t channel,
					  uint8_t priority)
{
	if (priority >= DMAC_LVL_NUM) {
		LOG_ERR("Invalid parameter for DMA priority level : %d", priority);
		return -EINVAL;
	}

	dmac_reg->CHANNEL[channel].DMAC_CHPRILVL = DMAC_CHPRILVL_PRILVL(priority);

	return 0;
}

static int8_t dmac_ch_set_burst_length(dmac_registers_t *dmac_reg, uint8_t channel,
				       uint32_t source_burst_length, uint32_t dest_burst_length)
{
	/* Validate source and destination burst lengths */
	if (source_burst_length != dest_burst_length) {
		LOG_ERR("Source and destination burst lengths do not match");
		return -EINVAL;
	}

	/* Validate burst length limit */
	if (source_burst_length > 16U) {
		LOG_ERR("Burst length exceeds maximum allowed value : %d", source_burst_length);
		return -EINVAL;
	}

	if (source_burst_length > 0U) {
		dmac_reg->CHANNEL[channel].DMAC_CHCTRLA |=
			DMAC_CHCTRLA_BURSTLEN(source_burst_length - 1U);
	}

	return 0;
}

static void dmac_ch_interrupt_enable(dmac_registers_t *dmac_reg, uint8_t channel,
				     bool disable_err_interrupt)
{
	/* Enable transfer complete interrupt */
	dmac_reg->CHANNEL[channel].DMAC_CHINTENSET = DMAC_CHINTENSET_TCMPL(1);

	/* Enable or disable transfer error interrupt based on flag */
	if (disable_err_interrupt == false) {
		dmac_reg->CHANNEL[channel].DMAC_CHINTENSET = DMAC_CHINTENSET_TERR(1);
	} else {
		dmac_reg->CHANNEL[channel].DMAC_CHINTENCLR = DMAC_CHINTENSET_TERR(1);
	}

	/* Clear any pending interrupt flags */
	dmac_reg->CHANNEL[channel].DMAC_CHINTFLAG =
		DMAC_CHINTFLAG_TERR_Msk | DMAC_CHINTFLAG_TCMPL_Msk;
}

static inline void dmac_ch_enable(dmac_registers_t *dmac_reg, uint8_t channel)
{
	dmac_reg->CHANNEL[channel].DMAC_CHCTRLA |= DMAC_CHCTRLA_ENABLE(1);
	if ((DMAC_CHCTRLA_TRIGSRC_Msk & dmac_reg->CHANNEL[channel].DMAC_CHCTRLA) == 0) {
		/* Trigger via software */
		dmac_reg->DMAC_SWTRIGCTRL = BIT(channel);
	}
}

static inline void dmac_ch_disable(dmac_registers_t *dmac_reg, uint8_t channel)
{
	dmac_reg->CHANNEL[channel].DMAC_CHCTRLA &= ~DMAC_CHCTRLA_ENABLE(1);
}

static inline void dmac_ch_suspend(dmac_registers_t *dmac_reg, uint8_t channel)
{

	uint32_t chctrlb = dmac_reg->CHANNEL[channel].DMAC_CHCTRLB;

	chctrlb &= ~DMAC_CHCTRLB_CMD_Msk;
	chctrlb |= DMAC_CHCTRLB_CMD_SUSPEND;
	dmac_reg->CHANNEL[channel].DMAC_CHCTRLB = chctrlb;
}

static inline void dmac_ch_resume(dmac_registers_t *dmac_reg, uint8_t channel)
{
	uint32_t chctrlb = dmac_reg->CHANNEL[channel].DMAC_CHCTRLB;

	chctrlb &= ~DMAC_CHCTRLB_CMD_Msk;
	chctrlb |= DMAC_CHCTRLB_CMD_RESUME;
	dmac_reg->CHANNEL[channel].DMAC_CHCTRLB = chctrlb;

	/* Clear the SUSPEND Interrupt Flag */
	dmac_reg->CHANNEL[channel].DMAC_CHINTFLAG |= DMAC_CHINTFLAG_SUSP(1);
}

static enum dma_mchp_ch_state dmac_ch_get_state(dmac_registers_t *dmac_reg, uint32_t channel)
{
	enum dma_mchp_ch_state ch_state;
	uint32_t active_status;
	uint8_t ch_int_flag, ch_status;

	/* Read channel status and interrupt flag */
	ch_status = dmac_reg->CHANNEL[channel].DMAC_CHSTATUS;
	ch_int_flag = dmac_reg->CHANNEL[channel].DMAC_CHINTFLAG;

	/* Check if the channel is busy */
	if ((ch_status & DMAC_CHSTATUS_BUSY_Msk) == DMAC_CHSTATUS_BUSY_Msk) {
		active_status = dmac_reg->DMAC_ACTIVE;

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

static int8_t dmac_ch_get_status(dmac_registers_t *dmac_reg, struct dma_mchp_dmac *data,
				 uint8_t channel, struct dma_status *stat)
{
	/* Check if the channel is busy and retrieve pending transfer length */
	if (dmac_ch_get_state(dmac_reg, channel) == DMA_MCHP_CH_ACTIVE) {
		stat->busy = true;
		stat->pending_length =
			(dmac_reg->DMAC_ACTIVE & DMAC_ACTIVE_BTCNT_Msk) >> DMAC_ACTIVE_BTCNT_Pos;
	} else {
		stat->busy = false;
		stat->pending_length = data->descriptors_wb[channel].DMAC_BTCNT;
	}

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

static inline void dmac_desc_init(const struct device *dev)
{
	struct dma_mchp_dmac *data =
		((const struct dma_mchp_dev_data *)(dev)->data)->dmac_desc_data;

	DMAC_REG->DMAC_BASEADDR = (uintptr_t)data->descriptors;
	DMAC_REG->DMAC_WRBADDR = (uintptr_t)data->descriptors_wb;
}

static int8_t dmac_desc_block_config(struct dma_block_config *block, void *desc_ptr,
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

static int8_t dmac_desc_reload_block(struct dma_mchp_dmac *data, uint32_t channel, uint32_t src,
				     uint32_t dst, size_t size)
{
	dmac_descriptor_registers_t *desc = &data->descriptors[channel];

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

	return 0;
}

static int dmac_get_hw_attribute(uint32_t type, uint32_t *value)
{
	enum dma_mchp_attribute_type attr = (enum dma_mchp_attribute_type)type;

	switch (attr) {
	case DMA_MCHP_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = DMAC_BUF_ADDR_ALIGNMENT;
		break;

	case DMA_MCHP_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = DMAC_BUF_SIZE_ALIGNMENT;
		break;

	case DMA_MCHP_ATTR_COPY_ALIGNMENT:
		*value = DMAC_COPY_ALIGNMENT;
		break;

	case DMA_MCHP_ATTR_MAX_BLOCK_COUNT:
		*value = DMAC_MAX_BLOCK_COUNT;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int dma_mchp_validate(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	if (dmac_ch_get_state(DMAC_REG, channel) == DMA_MCHP_CH_ACTIVE) {
		LOG_ERR("DMA channel %d is already in use", channel);
		return -EBUSY;
	}

	if (config->source_data_size != config->dest_data_size) {
		LOG_ERR("Source and destination data sizes do not match");
		return -EINVAL;
	}

	return 0;
}

static int dma_mchp_setup_channel(const struct device *dev, uint32_t channel,
				  const struct dma_config *config)
{
	int ret;

	/* Configure trigger source and direction */
	ret = dmac_ch_set_trig_src_n_dir(DMAC_REG, channel, config->dma_slot,
					 config->channel_direction);
	if (ret != 0) {
		LOG_ERR("Failed to configure trigger source/direction for channel %u", channel);
		return ret;
	}

	/* Set channel priority */
	ret = dmac_ch_set_priority(DMAC_REG, channel, config->channel_priority);
	if (ret != 0) {
		LOG_ERR("Failed to set priority for channel %u", channel);
		return ret;
	}

	/* Configure burst length */
	ret = dmac_ch_set_burst_length(DMAC_REG, channel, config->source_burst_length,
				       config->dest_burst_length);
	if (ret != 0) {
		LOG_ERR("Failed to set burst length for channel %u", channel);
		return ret;
	}

	/* Enable or disable interrupts (depending on config) */
	dmac_ch_interrupt_enable(DMAC_REG, channel, config->error_callback_dis);

	return 0;
}

/* Build descriptor chain */
static int dma_mchp_desc_setup(struct dma_mchp_dev_data *dev_data, struct dma_config *config,
			       void *base_desc)
{
	struct dma_block_config *block = config->head_block;
	void *desc = base_desc;
	int ret;

	/* Configure head block */
	ret = dmac_desc_block_config(block, desc, NULL, config->source_data_size);
	if (ret < 0) {
		LOG_ERR("DMA Error: Block 1 configuration failed!");
	}

	return ret;
}

/**
 * @brief DMA interrupt service routine (ISR).
 *
 * This function handles DMA interrupts and delegates processing
 * to the appropriate ISR handler for the given DMA instance.
 *
 * @param dev Pointer to the DMA device structure.
 */
static void dma_mchp_isr(const struct device *dev)
{
	/* Retrieve device runtime data */
	struct dma_mchp_dev_data *const dev_data = ((struct dma_mchp_dev_data *const)(dev)->data);
	struct dma_mchp_channel_config *channel_config;
	uint32_t channel;

	/* Handle interrupt and get status */
	int status = dmac_interrupt_handle_status(DMAC_REG, &channel);

	channel_config = &dev_data->dma_channel_config[channel];

	if (channel_config->cb != NULL) {
		if (status == DMA_MCHP_INT_SUCCESS) {
			channel_config->cb(dev, channel_config->user_data, channel,
					   DMA_STATUS_COMPLETE);
		} else {
			channel_config->cb(dev, channel_config->user_data, channel, -1);
		}
	}
}

/**
 * @brief Configures a DMA channel with the given settings.
 *
 * This function initializes and configures a specified DMA channel, including setting up
 * the trigger source, priority, burst length, and linked descriptors for multi-block transfers.
 *
 * @param dev Pointer to the DMA device structure.
 * @param channel DMA channel number to configure.
 * @param config Pointer to the DMA configuration structure.
 *
 * @return 0 on successful configuration.
 * @return -EINVAL if an invalid parameter is provided.
 * @return -EBUSY if the requested channel is already active.
 * @return -ENOMEM if memory allocation for descriptors fails.
 */
static int dma_mchp_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	struct dma_mchp_dev_data *const dev_data = dev->data;
	struct dma_mchp_channel_config *channel_config;
	int ret;

	ret = dma_mchp_validate(dev, channel, config);
	if (ret != 0) {
		return ret;
	}

	atomic_test_and_set_bit(dev_data->dma_ctx.atomic, channel);

	ret = dma_mchp_setup_channel(dev, channel, config);
	if (ret != 0) {
		return ret;
	}

	void *base_desc = &dev_data->dmac_desc_data->descriptors[channel];

	ret = dma_mchp_desc_setup(dev_data, config, base_desc);
	if (ret != 0) {
		return ret;
	}

	channel_config = &dev_data->dma_channel_config[channel];
	channel_config->cb = config->dma_callback;
	channel_config->user_data = config->user_data;
	channel_config->is_configured = true;

	return ret;
}

/**
 * @brief Starts a DMA transfer on a specified channel.
 *
 * This function checks if the channel is valid, idle, and properly configured before
 * enabling the DMA transfer. It ensures atomic operations using interrupt locking.
 *
 * @param dev Pointer to the device structure for the DMA driver.
 * @param channel DMA channel number to start the transfer.
 *
 * @return 0 on success, indicating the DMA transfer started successfully.
 * @return -EINVAL if the channel number is invalid or descriptors are not configured.
 * @return -EBUSY if the DMA channel is not idle.
 */
static int dma_mchp_start(const struct device *dev, uint32_t channel)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;

	/* Validate channel number */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check if the channel is already in use */
	if (dmac_ch_get_state(DMAC_REG, channel) == DMA_MCHP_CH_ACTIVE) {
		LOG_ERR("DMA channel:%d is currently busy", channel);
		return -EBUSY;
	}

	/* Check if the channel is configured */
	if (dev_data->dma_channel_config[channel].is_configured != true) {
		LOG_ERR("DMA descriptors not configured for channel : %d", channel);
		return -EINVAL;
	}

	/* Enable the DMA channel and start the transfer */
	dmac_ch_enable(DMAC_REG, channel);

	return 0;
}

/**
 * @brief Stops a DMA transfer on the specified channel.
 *
 * This function disables the DMA channel, returns any allocated descriptors
 * to the pool, and marks the channel as unconfigured. It ensures atomic
 * operation using interrupt locking.
 *
 * @param dev Pointer to the device structure for the DMA driver.
 * @param channel DMA channel number to stop the transfer.
 *
 * @return 0 on success, indicating the DMA channel was successfully stopped.
 * @return -EINVAL if the provided channel number is invalid.
 */
static int dma_mchp_stop(const struct device *dev, uint32_t channel)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Disable the channel */
	dmac_ch_disable(DMAC_REG, channel);

	return 0;
}

/**
 * @brief Reloads a DMA transfer for the specified channel.
 *
 * This function updates the source and destination addresses for a DMA transfer
 * and reloads the descriptor with the new transfer size.
 *
 * @param dev Pointer to the DMA device structure.
 * @param channel DMA channel number to be reloaded.
 * @param src Source address for the DMA transfer.
 * @param dst Destination address for the DMA transfer.
 * @param size Transfer size in bytes.
 *
 * @return 0 on success, -EINVAL if the channel is invalid.
 */
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
	if (dmac_ch_get_state(DMAC_REG, channel) == DMA_MCHP_CH_ACTIVE) {
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

/**
 * @brief Suspends an active DMA transfer on the specified channel.
 *
 * This function temporarily halts the DMA transfer on the given channel
 * without resetting its configuration, allowing it to be resumed later.
 *
 * @param dev Pointer to the DMA device structure.
 * @param channel DMA channel number to suspend.
 *
 * @return 0 on success, -EINVAL if the channel number is invalid.
 */
static int dma_mchp_suspend(const struct device *dev, uint32_t channel)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	enum dma_mchp_ch_state ch_state;

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check if the channel is currently active */
	ch_state = dmac_ch_get_state(DMAC_REG, channel);
	if (ch_state != DMA_MCHP_CH_ACTIVE) {
		LOG_INF("nothing to suspend as dma channel %u is not busy", channel);
	}

	/* Suspend the specified DMA channel */
	dmac_ch_suspend(DMAC_REG, channel);
	LOG_DBG("channel %u is suspended", channel);

	return 0;
}

/**
 * @brief Resumes a previously suspended DMA transfer on the specified channel.
 *
 * This function resumes a DMA transfer that was previously suspended using
 * dma_mchp_suspend(). The transfer continues from where it was paused.
 *
 * @param dev Pointer to the DMA device structure.
 * @param channel DMA channel number to resume.
 *
 * @return 0 on success, -EINVAL if the channel number is invalid.
 */
static int dma_mchp_resume(const struct device *dev, uint32_t channel)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	enum dma_mchp_ch_state ch_state;

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Check if the channel is already suspended */
	ch_state = dmac_ch_get_state(DMAC_REG, channel);
	if (ch_state != DMA_MCHP_CH_SUSPENDED) {
		LOG_INF("DMA channel %d is not in suspended state so cannot resume channel",
			channel);
		return -EINVAL;
	}

	/* Resume the specified DMA channel */
	dmac_ch_resume(DMAC_REG, channel);
	LOG_DBG("channel %u is resumed", channel);

	return 0;
}

/**
 * @brief Retrieves the status of a DMA channel.
 *
 * This function checks the status of the specified DMA channel and fills
 * the provided dma_status structure with relevant information.
 *
 * @param dev Pointer to the DMA device structure.
 * @param channel DMA channel number to retrieve the status for.
 * @param stat Pointer to a dma_status structure to store the status.
 *
 * @return 0 on success, or an error code if the status retrieval fails.
 */
static int dma_mchp_get_status(const struct device *dev, uint32_t channel, struct dma_status *stat)
{
	/* Get dev data */
	struct dma_mchp_dev_data *const dev_data = dev->data;
	int ret;

	/* Validate the DMA channel */
	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("Unsupported channel");
		return -EINVAL;
	}

	/* Retrieve DMA channel status */
	ret = dmac_ch_get_status(DMAC_REG, dev_data->dmac_desc_data, channel, stat);

	return ret;
}

/**
 * @brief DMA channel filter function.
 *
 * This function is used by the DMA framework to determine whether a given
 * channel is suitable for allocation based on the optional user-provided
 * filter parameter.
 *
 * If no filter parameter is provided (i.e., filter_param is NULL), the
 * function returns true, allowing any available channel to be selected.
 * If a filter parameter is provided, it must point to a uint32_t value
 * representing the desired channel number. The function returns true only
 * if the current channel matches the requested one.
 *
 * @param dev Pointer to the DMA device.
 * @param channel The channel index being evaluated.
 * @param filter_param Optional user-provided parameter (uint32_t* expected)
 *                     specifying a preferred channel number.
 *
 * @return true if the channel matches the filter criteria or no filter is provided;
 *         false otherwise.
 */
static bool dma_mchp_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t requested_channel;
	bool ret;

	/* If no specific channel is requested, allow any available channel */
	if (filter_param == NULL) {
		return true;
	}

	requested_channel = *(uint32_t *)filter_param;
	/* Allow only if current channel matches the requested one */
	ret = (bool)(channel == requested_channel);

	return ret;
}

/**
 * @brief Get DMA attribute from device.
 *
 * This function retrieves the specified attribute related to DMA hardware constraints.
 *
 * @param dev Pointer to the DMA device.
 * @param type DMA attribute type (from dma_mchp_attribute_type).
 * @param value Pointer to the variable where the attribute value will be stored.
 * @return 0 on success, -ENOTSUP if the attribute is not supported.
 */
static int dma_mchp_get_attribute(const struct device *dev, uint32_t type, uint32_t *value)
{
	/* Device is not used in current logic, but kept for API compatibility */
	ARG_UNUSED(dev);

	return dmac_get_hw_attribute((enum dma_mchp_attribute_type)type, value);
}

/**
 * @brief Initializes the DMA controller.
 *
 * This function enables the DMA clock, resets the DMA controller, initializes
 * descriptors, sets default priority levels, enables the DMA module, and configures
 * the DMA interrupt.
 *
 * @param dev Pointer to the DMA device structure.
 *
 * @return 0 on successful initialization.
 * @return -ENOMEM if the descriptor pool creation fails due to memory allocation failure.
 * @return -EINVAL if an invalid parameter is encountered.
 */
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
	dmac_controller_reset(DMAC_REG);

	/* Initialize DMA descriptors */
	dmac_desc_init(dev);

	/* Set default priority levels */
	dmac_set_default_priority(DMAC_REG);

	/* Enable the DMA controller */
	dmac_enable(DMAC_REG);

	/* Configure DMA interrupt */
	dev_cfg->irq_config();

	/* If everything OK but clocks were already on, return 0 */
	return (ret == -EALREADY) ? 0 : ret;
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
		LISTIFY(                                                                      \
			DT_NUM_IRQS(DT_DRV_INST(n)),                                          \
			DMA_MCHP_IRQ_CONNECT,                                                 \
			(),                                                                   \
			n                                                                     \
		)    \
	}

/* DMA runtime data structure. */
#define DMA_MCHP_DATA_DEFN(n)                                                                      \
	static struct dma_mchp_dmac dmac_desc_data_##n;                                            \
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
