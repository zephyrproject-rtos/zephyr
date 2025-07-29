/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file dma_mchp_dmac_g1.c
 * @brief Zephyr DMA driver for Microchip G1 peripherals
 *
 * Provides DMA support using Zephyr's DMA API for the DMAC G1 controller
 * found in Microchip SAM E5x and similar MCUs.
 */

#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/kernel.h>

/*******************************************
 * @brief Devicetree definitions
 *******************************************/
#define DT_DRV_COMPAT microchip_dmac_g1_dma

/*******************************************
 * Const and Macro Defines
 *******************************************/
LOG_MODULE_REGISTER(dma_mchp_dmac_g1, CONFIG_DMA_LOG_LEVEL);

/* Shortcut to access DMA registers from device configuration */
#undef DMAC_REGS
#define DMAC_REGS ((const dma_mchp_dev_config_t *)(dev)->config)->regs
#define DMAC_DESC ((const dma_mchp_dev_data_t *)(dev)->data)->dmac_desc_data;

/**
 * @brief DMA buffer address alignment requirement.
 *
 * This macro defines the required alignment (in bytes) for the starting address
 * of a DMA buffer. For 32-bit transfers, a 4-byte alignment is required.
 */
#define DMAC_BUF_ADDR_ALIGNMENT 4

/**
 * @brief DMA buffer size alignment requirement.
 *
 * This macro defines the required alignment (in bytes) for the size (length)
 * of a DMA buffer. For 32-bit transfers, a 4-byte alignment is required.
 */
#define DMAC_BUF_SIZE_ALIGNMENT 4

/**
 * @brief DMA copy alignment requirement.
 *
 * This macro defines the required alignment (in bytes) for both source and
 * destination addresses, as well as the transfer size, when performing DMA copy
 * operations. For 32-bit transfers, a 4-byte alignment is required.
 */
#define DMAC_COPY_ALIGNMENT 4

/**
 * @brief Maximum block count for DMA transfers.
 *
 * This macro defines the maximum number of data units (blocks) that can be
 * transferred in a single DMA block transfer operation. The DMAC Block Transfer
 * Count field is 16 bits wide, allowing a maximum value of 65535 (0xFFFF).
 * The actual number of bytes transferred depends on the configured data width.
 */
#define DMAC_MAX_BLOCK_COUNT 65535

/*******************************************
 * Data Types and Static Configuration
 *******************************************/
/**
 * @enum dma_mchp_int_sts_t
 * @brief Enumeration for DMA interrupt status codes.
 *
 * This enumeration defines the possible interrupt status codes
 * that indicate the outcome of a DMA transaction.
 */
typedef enum {
	/* DMA transfer encountered an error */
	DMA_MCHP_INT_ERROR = -1,

	/* DMA transfer completed successfully */
	DMA_MCHP_INT_SUCCESS = 0,

	/* DMA transfer was suspended */
	DMA_MCHP_INT_SUSPENDED = 1,
} dma_mchp_int_sts_t;

/**
 * @enum dma_mchp_ch_state_t
 * @brief DMA channel states for Microchip DMA.
 *
 * Represents the different states a DMA channel can be in during its lifecycle.
 */
typedef enum dma_mchp_ch_state {
	/* DMA channel is idle */
	DMA_MCHP_CH_IDLE,
	/* DMA Channel is pending */
	DMA_MCHP_CH_PENDING,
	/* DMA channel is suspended */
	DMA_MCHP_CH_SUSPENDED,
	/* DMA channel is actively transferring data */
	DMA_MCHP_CH_ACTIVE,
	/* DMA channel is prepared for transfer */
	DMA_MCHP_CH_PREPARED,
} dma_mchp_ch_state_t;

/**
 * @brief DMA attribute types for Microchip MCHP DMA controller.
 *
 * This enumeration defines attribute types that describe hardware-specific
 * constraints and capabilities for DMA transfers.
 */
typedef enum dma_mchp_attribute_type {
	/* Required alignment (in bytes) of the DMA buffer address. */
	DMA_MCHP_ATTR_BUFFER_ADDRESS_ALIGNMENT,

	/* Required alignment (in bytes) of the total buffer size. */
	DMA_MCHP_ATTR_BUFFER_SIZE_ALIGNMENT,

	/* Required alignment (in bytes) for each DMA copy transfer. */
	DMA_MCHP_ATTR_COPY_ALIGNMENT,

	/* Maximum number of blocks supported per DMA transfer. */
	DMA_MCHP_ATTR_MAX_BLOCK_COUNT,
} dma_mchp_attribute_type_t;

/**
 * @brief DMAC peripheral data structure for Microchip devices.
 *
 * This structure contains the DMA descriptors, write-back descriptors, and
 * channel-specific runtime data for Microchip devices.
 */
typedef struct dma_mchp_dmac {

	/* DMA descriptors for channel configurations (must be 16-byte aligned). */
	__aligned(16) dmac_descriptor_registers_t descriptors[DMAC_CH_NUM];

	/* DMA write-back descriptors for tracking completed transfers (16-byte aligned). */
	__aligned(16) dmac_descriptor_registers_t descriptors_wb[DMAC_CH_NUM];

} dma_mchp_dmac_t;

/**
 * @brief DMA Channel Configuration Structure
 *
 * This structure holds the configuration settings for a Microchip DMA channel.
 * It includes callback function details, user-defined data, and the channel state.
 */
typedef struct dma_mchp_channel_config {

	/* Callback function for DMA transfer events. */
	dma_callback_t cb;

	/* User-defined data passed to the callback. */
	void *user_data;

	/* Channel state */
	bool is_configured;

} dma_mchp_channel_config_t;

/**
 * @brief DMA Microchip Device Configuration Structure.
 *
 * This structure holds the hardware configuration for a DMA controller.
 */
typedef struct dma_mchp_dev_config {

	/* Pointer to the DMA registers. */
	dmac_registers_t *regs;

	/* Pointer to the clock device used for controlling the DMA's clock. */
	const struct device *clock_dev;

	/* Contains the clock control configuration for the DMA subsystem. */
	clock_control_subsys_t mclk_sys;

	/* This field stores the number of interrupts associated with the DMA. */
	uint8_t num_irq;

	/* Function pointer for configuring DMA interrupts. */
	void (*irq_config)(void);

} dma_mchp_dev_config_t;

/**
 * @brief Structure holding runtime data for the Microchip DMA controller.
 *
 * This structure contains descriptor tables and channel-specific data used
 * during DMA operations.
 */
typedef struct dma_mchp_dev_data {

	/* dma context */
	struct dma_context dma_ctx;

	/* Declare the Data  */
	dma_mchp_dmac_t *dmac_desc_data;

	/* DMA Channels configuration */
	dma_mchp_channel_config_t *dma_channel_config;

	/* Pool of descriptor */
	struct k_fifo dma_desc_pool;

	/* Num of descriptors in the pool */
	uint32_t dma_desc_pool_cnt;

	/* Specifies the required DMA alignment, typically based on hardware constraints. */
	uint16_t dma_alignment;

} dma_mchp_dev_data_t;

/*******************************************
 * Static Helper Functions
 *******************************************/
/**
 * @brief Retrieves a descriptor from the DMA descriptor pool.
 *
 * This function fetches a descriptor from the DMA descriptor pool FIFO if
 * the descriptor pool count is non-zero. The function will block indefinitely
 * until a descriptor is available.
 *
 * @param dma_desc_pool Pointer to the DMA descriptor FIFO pool.
 * @param dma_desc_pool_cnt Pointer to the DMA descriptor pool counter, decremented on retrieval.
 *
 * @return Pointer to the retrieved descriptor, or NULL if the pool count is zero.
 */
void *dma_mchp_desc_get(struct k_fifo *dma_desc_pool, uint32_t *dma_desc_pool_cnt)
{
	void *desc = NULL;

	/* Ensure pool count is valid before accessing FIFO */
	if (dma_desc_pool_cnt != NULL && *dma_desc_pool_cnt != 0) {
		/* Decrement descriptor pool count */
		(*dma_desc_pool_cnt)--;
		/* Fetch a descriptor */
		desc = k_fifo_get(dma_desc_pool, K_FOREVER);
	}

	return desc;
}

/**
 * @brief Handle DMA interrupt and retrieve the associated callback and user data.
 *
 * This function reads the DMA interrupt status, determines the channel number,
 * acknowledges the interrupt, and retrieves the associated callback function
 * and user data for the given DMA channel.
 *
 * @param dmac_reg DMAC Peripheral base address.
 * @param channel Pointer to an integer where the DMA channel number will be stored.
 *
 * @return Interrupt status code, indicating success or error
 *
 */
static int8_t dmac_interrupt_handle_status(dmac_registers_t *dmac_reg, int *channel)
{
	/* Read interrupt status */
	uint16_t pend = dmac_reg->DMAC_INTPEND;
	int8_t interrupt_status_code = 0;

	/* Get the channel number */
	*channel = (pend & DMAC_INTPEND_ID_Msk) >> DMAC_INTPEND_ID_Pos;

	/* Acknowledge all interrupts */
	dmac_reg->DMAC_INTPEND = pend;

	/* Determine the status code */
	if (pend & DMAC_INTPEND_TERR_Msk) {
		interrupt_status_code = DMA_MCHP_INT_ERROR;
	} else if (pend & DMAC_INTPEND_TCMPL_Msk) {
		interrupt_status_code = DMA_MCHP_INT_SUCCESS;
	}

	return interrupt_status_code;
}

/**
 * @brief Reset the DMA controller.
 *
 * This function disables the DMA controller and performs a software reset.
 * It waits for the reset to complete before returning.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 */
static inline void dmac_controller_reset(dmac_registers_t *dmac_reg)
{
	dmac_reg->DMAC_CTRL |= DMAC_CTRL_DMAENABLE(0);
	dmac_reg->DMAC_CTRL |= DMAC_CTRL_SWRST_Msk;
	while ((dmac_reg->DMAC_CTRL & DMAC_CTRL_SWRST_Msk) >> DMAC_CTRL_SWRST_Pos) {
	}
}

/**
 * @brief Enable or disable the DMA controller.
 *
 * This function enables or disables the DMA controller based on the given flag.
 * When enabling, all priority levels are activated.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param enable Boolean value to enable (true) or disable (false) the DMA controller.
 */
static inline void dmac_enable(dmac_registers_t *dmac_reg, bool enable)
{
	if (enable == true) {
		dmac_reg->DMAC_CTRL = DMAC_CTRL_DMAENABLE(1) | DMAC_CTRL_LVLEN(0x0F);
	} else {
		dmac_reg->DMAC_CTRL = DMAC_CTRL_DMAENABLE(0);
	}
}

/**
 * @brief Set default priority levels for the DMA controller.
 *
 * This function assigns default priority levels to the four DMA priority levels.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 */
static inline void dmac_set_default_priority(dmac_registers_t *dmac_reg)
{
	dmac_reg->DMAC_PRICTRL0 = DMAC_PRICTRL0_LVLPRI0(0) | DMAC_PRICTRL0_LVLPRI1(1) |
				  DMAC_PRICTRL0_LVLPRI2(2) | DMAC_PRICTRL0_LVLPRI3(3);
}

/**
 * @brief Set the trigger source for a DMA channel.
 *
 * This function sets the trigger source for a specified DMA channel based on the
 * provided trigger source and channel direction. It validates the trigger source
 * and configures the channel accordingly.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param channel DMA channel number.
 * @param trig_src Trigger source for the DMA channel.
 * @param channel_direction Direction of the DMA channel (e.g., MEMORY_TO_MEMORY,
 * MEMORY_TO_PERIPHERAL, PERIPHERAL_TO_MEMORY).
 *
 * @retval 0 on success.
 * @retval -EINVAL Invalid error code
 */
static int8_t dmac_ch_set_trig_src_n_dir(dmac_registers_t *dmac_reg, uint8_t channel,
					 uint8_t trig_src,
					 enum dma_channel_direction channel_direction)
{
	int8_t ret_val = 0;

	/* Validate trigger source */
	if (trig_src >= DMAC_TRIG_NUM) {
		ret_val = -EINVAL;
		LOG_ERR("Invalid parameter for DMA trigger source : %d", trig_src);
	}

	/* If no error, configure trigger source */
	if (ret_val == 0) {
		if (channel_direction == MEMORY_TO_MEMORY) {
			/*
			 * A single software trigger will start the
			 * transfer
			 */
			dmac_reg->CHANNEL[channel].DMAC_CHCTRLA =
				DMAC_CHCTRLA_TRIGACT_TRANSACTION | DMAC_CHCTRLA_TRIGSRC(trig_src);

		} else if ((channel_direction == MEMORY_TO_PERIPHERAL) ||
			   (channel_direction == PERIPHERAL_TO_MEMORY)) {
			/* One peripheral trigger per beat */
			dmac_reg->CHANNEL[channel].DMAC_CHCTRLA =
				DMAC_CHCTRLA_TRIGACT_BURST | DMAC_CHCTRLA_TRIGSRC(trig_src);

		} else {
			ret_val = -EINVAL;
			LOG_ERR("Invalid parameter for DMA channel direction");
		}
	}

	return ret_val;
}

/**
 * @brief Set the priority level for a DMA channel.
 *
 * This function sets the priority level for a specified DMA channel.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param channel DMA channel number.
 * @param priority Priority level to be set.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the priority level is invalid.
 */
static inline int8_t dmac_ch_set_priority(dmac_registers_t *dmac_reg, uint8_t channel,
					  uint8_t priority)
{
	int8_t ret_val = 0;

	/* Validate the priority level */
	if (priority >= DMAC_LVL_NUM) {
		LOG_ERR("Invalid parameter for DMA priority level : %d", priority);
		ret_val = -EINVAL;
	}

	/* Apply priority setting if validation passed */
	if (ret_val == 0) {
		dmac_reg->CHANNEL[channel].DMAC_CHPRILVL = DMAC_CHPRILVL_PRILVL(priority);
	}

	return ret_val;
}

/**
 * @brief Set the burst length for a DMA channel.
 *
 * This function sets the burst length for both the source and destination of a specified
 * DMA channel. The burst lengths for the source and destination must be equal and must not
 * exceed 16.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param channel DMA channel number.
 * @param source_burst_length Burst length for the source.
 * @param dest_burst_length Burst length for the destination.
 *
 * @retval 0 on success.
 * @retval -EINVAL If invalid params.
 */
static int8_t dmac_ch_set_burst_length(dmac_registers_t *dmac_reg, uint8_t channel,
				       uint32_t source_burst_length, uint32_t dest_burst_length)
{

	int8_t ret_val = 0;

	/* Validate source and destination burst lengths */
	if (source_burst_length != dest_burst_length) {
		ret_val = -EINVAL;
		LOG_ERR("Source and destination burst lengths do not match");
	}

	/* Validate burst length limit */
	if (source_burst_length > 16U) {
		ret_val = -EINVAL;
		LOG_ERR("Burst length exceeds maximum allowed value : %d", source_burst_length);
	}

	/* Apply burst length setting if validation passed */
	if (ret_val == 0 && source_burst_length > 0U) {
		dmac_reg->CHANNEL[channel].DMAC_CHCTRLA |=
			DMAC_CHCTRLA_BURSTLEN(source_burst_length - 1U);
	}

	return ret_val;
}

/**
 * @brief Enable interrupts for a DMA channel.
 *
 * This function enables the transfer complete interrupt for a specified DMA channel.
 * It also conditionally enables or disables the transfer error interrupt based on the
 * provided parameter.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param channel DMA channel number.
 * @param disable_err_interrupt Boolean flag to disable the transfer error interrupt.
 *                              If true, the transfer error interrupt is disabled.
 *                              If false, the transfer error interrupt is enabled.
 */
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

/**
 * @brief Enable or disable a DMA channel.
 *
 * This function enables or disables a specified DMA channel based on the provided flag.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param channel DMA channel number.
 * @param enable Flag to enable (true) or disable (false) the channel.
 */
static inline void dmac_ch_enable(dmac_registers_t *dmac_reg, uint8_t channel, bool enable)
{
	if (enable == true) {
		dmac_reg->CHANNEL[channel].DMAC_CHCTRLA |= DMAC_CHCTRLA_ENABLE(1);
		if ((DMAC_CHCTRLA_TRIGSRC_Msk & dmac_reg->CHANNEL[channel].DMAC_CHCTRLA) == 0) {
			/* Trigger via software */
			dmac_reg->DMAC_SWTRIGCTRL = 1U << channel;
		}
	} else {
		dmac_reg->CHANNEL[channel].DMAC_CHCTRLA &= ~DMAC_CHCTRLA_ENABLE(1);
	}
}

/**
 * @brief Suspend a DMA channel.
 *
 * This function suspends a specified DMA channel.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param channel DMA channel number.
 */
static inline void dmac_ch_suspend(dmac_registers_t *dmac_reg, uint8_t channel)
{
	dmac_reg->CHANNEL[channel].DMAC_CHCTRLB |=
		(dmac_reg->CHANNEL[channel].DMAC_CHCTRLB & (uint8_t)(~DMAC_CHCTRLB_CMD_Msk)) |
		DMAC_CHCTRLB_CMD_SUSPEND;
}

/**
 * @brief Resume a DMA channel.
 *
 * This function resumes a specified DMA channel.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param channel DMA channel number.
 */
static inline void dmac_ch_resume(dmac_registers_t *dmac_reg, uint8_t channel)
{
	dmac_reg->CHANNEL[channel].DMAC_CHCTRLB =
		(dmac_reg->CHANNEL[channel].DMAC_CHCTRLB & (uint8_t)(~DMAC_CHCTRLB_CMD_Msk)) |
		DMAC_CHCTRLB_CMD_RESUME;

	/* Clear the SUSPEND Interrupt Flag */
	dmac_reg->CHANNEL[channel].DMAC_CHINTFLAG |= DMAC_CHINTFLAG_SUSP(1);
}

/**
 * @brief Get the state of a DMA channel.
 *
 * This function retrieves the current state of a specific DMA channel by
 * checking its status, activity, and interrupt flags.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param channel DMA channel number to check.
 *
 * @return The current state of the DMA channel, which can be:
 *         - DMA_MCHP_CH_IDLE
 *         - DMA_MCHP_CH_ACTIVE
 *         - DMA_MCHP_CH_SUSPENDED
 *         - DMA_MCHP_CH_PENDING
 */
static dma_mchp_ch_state_t dmac_ch_get_state(dmac_registers_t *dmac_reg, uint32_t channel)
{
	dma_mchp_ch_state_t ch_state = DMA_MCHP_CH_IDLE;
	uint32_t active_status = 0;
	uint8_t ch_int_flag = 0, ch_status = 0;

	/* Read channel status and interrupt flag */
	ch_status = dmac_reg->CHANNEL[channel].DMAC_CHSTATUS;
	ch_int_flag = dmac_reg->CHANNEL[channel].DMAC_CHINTFLAG;

	/* Check if the channel is busy */
	if (((ch_status & DMAC_CHSTATUS_BUSY_Msk) >> DMAC_CHSTATUS_BUSY_Pos) == 1) {
		active_status = dmac_reg->DMAC_ACTIVE;

		/* Check if the channel is currently active */
		if ((((active_status & DMAC_ACTIVE_ABUSY_Msk) >> DMAC_ACTIVE_ABUSY_Pos) == 1) &&
		    (((active_status & DMAC_ACTIVE_ID_Msk) >> DMAC_ACTIVE_ID_Pos) == channel)) {
			ch_state = DMA_MCHP_CH_ACTIVE;
		}
		/* Check if the channel is suspended */
		else if (((ch_int_flag & DMAC_CHINTFLAG_SUSP_Msk) >> DMAC_CHINTFLAG_SUSP_Pos) ==
			 1) {
			ch_state = DMA_MCHP_CH_SUSPENDED;
		}
	}
	/* Check if the channel is pending */
	else if (((ch_status & DMAC_CHSTATUS_PEND_Msk) >> DMAC_CHSTATUS_PEND_Pos) == 1) {
		ch_state = DMA_MCHP_CH_PENDING;
	}
	/* Explicitly setting to IDLE for clarity */
	else {
		ch_state = DMA_MCHP_CH_IDLE;
	}

	return ch_state;
}

/**
 * @brief Get the status of a DMA channel.
 *
 * This function retrieves the status of a specified DMA channel, including whether it is busy
 * and the pending length of data to be transferred.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param channel DMA channel number.
 * @param stat Pointer to the DMA status structure to be filled.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the beat size is invalid.
 */
static int8_t dmac_ch_get_status(dmac_registers_t *dmac_reg, dma_mchp_dmac_t *data, uint8_t channel,
				 struct dma_status *stat)
{
	int8_t ret_val = 0;

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
		ret_val = -EINVAL;
		LOG_ERR("Invalid configuration beat size");
	}

	return ret_val;
}

/**
 * @brief Set up DMA descriptor addresses.
 *
 * This function configures the DMA controller with the base and write-back addresses
 * of the descriptors used for DMA transfers.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 */
static inline void dmac_desc_init(const struct device *dev)
{
	dma_mchp_dmac_t *data = DMAC_DESC;

	DMAC_REGS->DMAC_BASEADDR = (uintptr_t)data->descriptors;
	DMAC_REGS->DMAC_WRBADDR = (uintptr_t)data->descriptors_wb;
}

/**
 * @brief Get the size of a DMA descriptor.
 *
 * This function returns the size of a DMA descriptor.
 *
 * @retval Size of the DMA descriptor in bytes.
 */
static inline int8_t dmac_get_desc_size(void)
{
	uint32_t size = sizeof(dmac_descriptor_registers_t);

	return size;
}

/**
 * @brief Configure a DMA descriptor block.
 *
 * This function sets up a DMA descriptor with the provided block configuration,
 * including source and destination addresses, data transfer size, and block size.
 * It also links the descriptor to the previous descriptor if applicable.
 *
 * @param block Pointer to the DMA block configuration structure.
 * @param desc_ptr Pointer to the current DMA descriptor.
 * @param pre_desc_ptr Pointer to the previous DMA descriptor (can be NULL).
 * @param src_data_size Size of each data transfer in bytes (1, 2, or 4).
 *
 * @retval 0 on success.
 * @retval -EINVAL If invalid params.
 */
static int8_t dmac_desc_block_config(struct dma_block_config *block, void *desc_ptr,
				     void *pre_desc_ptr, uint32_t src_data_size)
{
	/* Descriptors typecast */
	dmac_descriptor_registers_t *desc = (dmac_descriptor_registers_t *)desc_ptr;
	dmac_descriptor_registers_t *pre_desc = (dmac_descriptor_registers_t *)pre_desc_ptr;

	uint16_t btctrl = 0;
	int8_t ret_val = 0;

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
		ret_val = -EINVAL;
		LOG_ERR("Invalid parameter for DMA source data size");
		break;
	}

	if (ret_val == 0) {
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
			ret_val = -EINVAL;
			LOG_ERR("Invalid parameter for DMA source address");
			break;
		}
	}

	if (ret_val == 0) {
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
			ret_val = -EINVAL;
			LOG_ERR("Invalid parameter for DMA destination address");
			break;
		}
	}

	if (ret_val == 0) {
		btctrl |= DMAC_BTCTRL_VALID(1);
		desc->DMAC_BTCTRL = btctrl;

		/* Attach the current descriptor to the previous descriptor at the end */
		if (pre_desc != NULL) {
			pre_desc->DMAC_DESCADDR = (uint32_t)desc;
		}
	}

	return ret_val;
}

/**
 * @brief Reload a DMA descriptor block.
 *
 * This function reloads a DMA descriptor block with new source and destination addresses and size.
 *
 * @param data dmac descriptor data.
 * @param channel DMA channel number.
 * @param src Source address.
 * @param dst Destination address.
 * @param size Size of the data to be transferred.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the beat size is invalid.
 */
static int8_t dmac_desc_reload_block(dma_mchp_dmac_t *data, uint32_t channel, uint32_t src,
				     uint32_t dst, size_t size)
{
	dmac_descriptor_registers_t *desc = &data->descriptors[channel];

	int8_t ret_val = 0;

	/* check if already multiple blocks are configured */
	if (desc->DMAC_DESCADDR != 0) {
		ret_val = -EINVAL;
	}

	if (ret_val == 0) {
		/* Update the block transfer count based on beat size */
		switch (((DMAC_BTCTRL_BEATSIZE_Msk & desc->DMAC_BTCTRL) >>
			 DMAC_BTCTRL_BEATSIZE_Pos)) {
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
			ret_val = -EINVAL;
			LOG_ERR("Invalid configuration beat size");
			break;
		}
	}

	if (ret_val == 0) {
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
	}

	return ret_val;
}

/**
 * @brief Retrieve the used DMA descriptor for a given channel.
 *
 * This function retrieves the next descriptor from the used descriptor list,
 * resets its values, and updates the descriptor chain. If no descriptors
 * remain, it returns NULL.
 *
 * @param dmac_reg Pointer to the base address of the DMAC peripheral registers.
 * @param channel DMA channel number.
 *
 * @return Pointer to the used descriptor, or NULL if none are available.
 */
static void *dmac_desc_get_used(dma_mchp_dmac_t *data, uint32_t channel)
{

	/* Get the configured descriptor after use */
	dmac_descriptor_registers_t *desc =
		(dmac_descriptor_registers_t *)data->descriptors[channel].DMAC_DESCADDR;

	void *ret_desc = NULL;

	if (desc != NULL) {
		/* Store the next descriptor before resetting */
		dmac_descriptor_registers_t *next_desc =
			(dmac_descriptor_registers_t *)desc->DMAC_DESCADDR;

		/* Reset descriptor values */
		desc->DMAC_BTCTRL = 0;
		desc->DMAC_BTCNT = 0;
		desc->DMAC_SRCADDR = 0;
		desc->DMAC_DSTADDR = 0;
		desc->DMAC_DESCADDR = 0;

		/* Move to the next descriptor for the next call */
		data->descriptors[channel].DMAC_DESCADDR = (uint32_t)next_desc;

		ret_desc = desc;
	} else {
		/* Reset descriptor if no more descriptors are available */
		data->descriptors[channel].DMAC_DESCADDR = 0;
	}

	return ret_desc;
}

/**
 * @brief Enables a cyclic DMA descriptor chain.
 *
 * This function links the last descriptor in the DMA chain to the base descriptor,
 * enabling a cyclic transfer mode.
 *
 * @param last_desc_ptr Pointer to the last descriptor in the chain.
 * @param base_desc_ptr Pointer to the base (first) descriptor in the chain.
 */
static inline void dmac_enable_cyclic_desc_chain(void *last_desc_ptr, void *base_desc_ptr)
{
	/* Descriptors typecast */
	dmac_descriptor_registers_t *last_desc = (dmac_descriptor_registers_t *)last_desc_ptr;
	dmac_descriptor_registers_t *base_desc = (dmac_descriptor_registers_t *)base_desc_ptr;

	/* Link the last descriptor to the base descriptor to form a cyclic chain */
	last_desc->DMAC_DESCADDR = (uint32_t)base_desc;
}

/**
 * @brief Function to get specific DMA hardware attribute.
 *
 * @param type DMA attribute type (enum dma_mchp_attribute_type).
 * @param value Pointer to return the attribute value.
 * @return 0 on success, -ENOTSUP if unsupported type.
 */
static int dmac_get_hw_attribute(uint32_t type, uint32_t *value)
{
	int ret = 0;
	dma_mchp_attribute_type_t attr = (dma_mchp_attribute_type_t)type;

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
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

/**
 * @brief Adds a descriptor to the DMA descriptor pool.
 *
 * This function inserts a descriptor into the DMA descriptor FIFO pool
 * and increments the descriptor pool count.
 *
 * @param dma_desc_pool Pointer to the DMA descriptor FIFO pool.
 * @param dma_desc_pool_cnt Pointer to the DMA descriptor pool counter, incremented when a
 * descriptor is added.
 * @param new_desc_node Pointer to the descriptor node being added to the pool.
 */
void dma_mchp_desc_put(struct k_fifo *dma_desc_pool, uint32_t *dma_desc_pool_cnt,
		       void *new_desc_node)
{
	/* Add descriptor to the FIFO */
	k_fifo_put(dma_desc_pool, new_desc_node);
	/* Increment descriptor pool count */
	(*dma_desc_pool_cnt)++;
}

/**
 * @brief Creates a pool of DMA descriptors with the specified alignment.
 *
 * This function initializes a FIFO queue for DMA descriptors, allocates memory-aligned
 * descriptors, and adds them to the descriptor pool.
 *
 * @param dma_desc_pool Pointer to the FIFO queue that will store the descriptors.
 * @param dma_desc_pool_cnt Pointer to the variable that keeps track of the number of descriptors in
 * the pool.
 * @param dma_alignment Alignment requirement for the DMA descriptors in bytes.
 * @param desc_count Number of descriptors to allocate.
 *
 * @return 0 on success.
 * @return -ENOMEM if memory allocation fails.
 */
int8_t dma_mchp_desc_pool_create(struct k_fifo *dma_desc_pool, uint32_t *dma_desc_pool_cnt,
				 uint16_t dma_alignment, int desc_count)
{
	int8_t ret = 0;
	/* Get the required size for a DMA descriptor */
	uint32_t desc_size = dmac_get_desc_size();

	/* Initialize the FIFO queue for DMA descriptors */
	k_fifo_init(dma_desc_pool);

	/* Allocate and enqueue the requested number of descriptors */
	for (int i = 0; i < desc_count; i++) {
		/* Allocate memory for a new descriptor */
		void *new_desc_node = (void *)k_aligned_alloc(dma_alignment, desc_size);

		/* Check if memory allocation failed */
		if (new_desc_node == NULL) {
			LOG_ERR("DMA Error : DMA Descriptor pool creation failed!");
			ret = -ENOMEM;
			break;
		}

		/* Validate descriptor size to ensure it meets FIFO requirements */
		if (desc_size < sizeof(void *)) {
			LOG_ERR("DMA Error: Descriptor size too small for FIFO!");
			/* Free the allocated memory before returning */
			k_free(new_desc_node);
			ret = -ENOMEM;
			break;
		}

		/* Add the newly allocated descriptor to the pool */
		dma_mchp_desc_put(dma_desc_pool, dma_desc_pool_cnt, new_desc_node);
	}

	return ret;
}

/**
 * @brief Enqueue used DMA descriptors into the descriptor pool.
 *
 * This function retrieves used DMA descriptors from the specified channel
 * and enqueues them into the descriptor pool for reuse.
 *
 * @param dev_data Pointer to the DMA device data structure.
 * @param channel DMA channel number from which descriptors are retrieved.
 */
void dma_mchp_desc_return_pool(dma_mchp_dev_data_t *dev_data, int channel)
{
	void *desc = NULL;

	/* Retrieve used descriptors and enqueue them back into the pool */
	while ((desc = dmac_desc_get_used(dev_data->dmac_desc_data, channel)) != NULL) {
		if (dev_data->dma_desc_pool_cnt == CONFIG_DMA_MCHP_DMAC_G1_MAX_DESC) {
			break;
		}
		/* Add the descriptor to the pool for reuse */
		dma_mchp_desc_put(&dev_data->dma_desc_pool, &dev_data->dma_desc_pool_cnt, desc);
	}
}

/*******************************************
 * Interrupt Service Routines (ISRs)
 *******************************************/
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
	dma_mchp_dev_data_t *const dev_data = ((dma_mchp_dev_data_t *const)(dev)->data);
	dma_mchp_channel_config_t *channel_config = NULL;
	uint32_t channel;

	/* Handle interrupt and get status */
	int status = dmac_interrupt_handle_status(DMAC_REGS, &channel);

	/* Clear the descriptors */
	dma_mchp_desc_return_pool(dev_data, channel);

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

/*******************************************
 * Zephyr Driver API Implementations
 *******************************************/
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
	/* Get device config */
	const dma_mchp_dev_config_t *const dev_cfg = dev->config;
	/* Get dev data */
	dma_mchp_dev_data_t *const dev_data = dev->data;
	uint32_t i = 0, key = 0;
	dma_mchp_ch_state_t ch_state = DMA_MCHP_CH_IDLE;
	struct dma_block_config *block = NULL;
	dma_mchp_channel_config_t *channel_config = NULL;
	int ret = 0;
	bool irq_locked = false;
	void *desc = NULL, *base_desc = NULL;

	ARG_UNUSED(dev_cfg);

	do {
		/* Validate channel number */
		if (channel >= dev_data->dma_ctx.dma_channels) {
			LOG_ERR("Unsupported channel");
			ret = -EINVAL;
			break;
		}

		/* If dma_request_channel() was not called but dma_config() is called directly,
		 * mark the channel as used by setting the bit in the channel usage bitmap.
		 */
		atomic_test_and_set_bit(dev_data->dma_ctx.atomic, channel);

		/* Check if the channel is already in use */
		ch_state = dmac_ch_get_state(DMAC_REGS, channel);
		if (ch_state == DMA_MCHP_CH_ACTIVE) {
			LOG_ERR("DMA channel %d is already in use", channel);
			ret = -EBUSY;
			break;
		}

		/* Lock interrupts to ensure atomic operation */
		key = irq_lock();
		irq_locked = true;

		/* Configures the trigger source and transfer direction for the DMA channel.
		 * Returns an error code if the specified direction or trigger source is invalid.
		 */
		ret = dmac_ch_set_trig_src_n_dir(DMAC_REGS, channel, config->dma_slot,
						 config->channel_direction);
		if (ret < 0) {
			break;
		}

		/* Configures the channel priority. Returns an error code if the specified priority
		 * is invalid.
		 */
		ret = dmac_ch_set_priority(DMAC_REGS, channel, config->channel_priority);
		if (ret < 0) {
			break;
		}

		/* Sets the burst length for the DMA channel. Returns an error code if the specified
		 * burst length is invalid.
		 */
		ret = dmac_ch_set_burst_length(DMAC_REGS, channel, config->source_burst_length,
					       config->dest_burst_length);
		if (ret < 0) {
			break;
		}

		bool disable_err_interrupt = config->error_callback_dis ? true : false;

		/* Enable DMA interrupts for the channel */
		dmac_ch_interrupt_enable(DMAC_REGS, channel, disable_err_interrupt);

		/* Ensure source and destination data sizes match */
		if (config->source_data_size != config->dest_data_size) {
			ret = -EINVAL;
			LOG_ERR("Source and destination data sizes do not match");
			break;
		}

		LOG_DBG("Available Descriptors in the pool : %d", dev_data->dma_desc_pool_cnt);

		/* Get the channel's First descriptor base address*/
		desc = &dev_data->dmac_desc_data->descriptors[channel];
		base_desc = desc;

		LOG_DBG("Channel %d: First descriptor base address: %p", channel, desc);

		/* Get the head block */
		block = config->head_block;

		/* Configure the first block */
		LOG_DBG("BLOCK 1: Configure descriptor at address: %p", desc);
		ret = dmac_desc_block_config(block,                   /* Head block */
					     desc,                    /* current desc */
					     NULL,                    /* prev desc */
					     config->source_data_size /* Source data size */
		);
		if (ret < 0) {
			LOG_ERR("DMA Error: Block : 1 Configuration Failed!");
			break;
		}

		/* Check multi block */
		if (config->block_count > 1) {
			/* Check whether we have enough descriptors in the pool */
			if (dev_data->dma_desc_pool_cnt < config->block_count) {
				LOG_ERR("Requested number of Descriptors are not available in the "
					"Descriptor pool. Please configure more descriptors and "
					"try again\r\n");
				ret = -ENOMEM;
				break;
			}
		}

		/* Configure multiple blocks here */
		for (i = 1; i < config->block_count; i++) {
			void *prev_desc = desc;

			/* Get the Descriptor from the pool */
			desc = dma_mchp_desc_get(&dev_data->dma_desc_pool,
						 &dev_data->dma_desc_pool_cnt);
			if (desc == NULL) {
				LOG_ERR("DMA Error: Failed to get descriptor for block %d", i);
				ret = -ENOMEM;
				break;
			}

			/* Move to the next block */
			block = block->next_block;

			/* Configure the block */
			LOG_DBG("BLOCK %d: Configure descriptor at address: %p", i + 1, desc);
			ret = dmac_desc_block_config(block,                   /* block */
						     desc,                    /* current desc */
						     prev_desc,               /* prev desc */
						     config->source_data_size /* Source data size */
			);

			if (i == (config->block_count - 1)) {
				/* Last descriptor, check if we should setup a circular chain */
				if (config->cyclic == true) {
					dmac_enable_cyclic_desc_chain(desc, base_desc);
				}
			}

			if (ret < 0) {
				LOG_ERR("DMA Error: Block : %d Configuration Failed!", (i + 1));
				break;
			}
		}

		/* If any error occurred inside for loop, break from do while loop */
		if (ret < 0) {
			break;
		}

		LOG_DBG("Available Descriptors in the pool : %d", dev_data->dma_desc_pool_cnt);

		channel_config = &dev_data->dma_channel_config[channel];
		/* Register the callback function for the channel*/
		channel_config->cb = config->dma_callback;
		channel_config->user_data = config->user_data;

		/* Update channel state to configured */
		channel_config->is_configured = true;

		ret = 0;
	} while (0);

	/* Unlock the IRQ */
	if (irq_locked == true) {
		irq_unlock(key);
	}

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
	/* Retrieve device configuration */
	const dma_mchp_dev_config_t *dev_cfg = dev->config;
	/* Get dev data */
	dma_mchp_dev_data_t *const dev_data = dev->data;
	dma_mchp_channel_config_t *channel_config = NULL;
	dma_mchp_ch_state_t ch_state;
	int ret = 0;
	uint32_t key = 0;

	ARG_UNUSED(dev_cfg);

	do {
		/* Validate channel number */
		if (channel >= dev_data->dma_ctx.dma_channels) {
			LOG_ERR("Unsupported channel");
			ret = -EINVAL;
			break;
		}

		/* Check if the channel is already in use */
		ch_state = dmac_ch_get_state(DMAC_REGS, channel);
		if (ch_state == DMA_MCHP_CH_ACTIVE) {
			LOG_ERR("DMA channel:%d is currently busy", channel);
			ret = -EBUSY;
			break;
		}

		/* Check if the channel is configured */
		channel_config = &dev_data->dma_channel_config[channel];
		if (channel_config->is_configured != true) {
			LOG_ERR("DMA descriptors not configured for channel : %d", channel);
			ret = -EINVAL;
			break;
		}

		/* Lock interrupts to ensure atomic operation */
		key = irq_lock();

		/* Enable the DMA channel and start the transfer */
		dmac_ch_enable(DMAC_REGS, channel, true);

		/* Unlock interrupts */
		irq_unlock(key);

		ret = 0;

	} while (0);

	return ret;
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
	/* Retrieve device configuration */
	const dma_mchp_dev_config_t *dev_cfg = dev->config;
	/* Get dev data */
	dma_mchp_dev_data_t *const dev_data = dev->data;
	uint32_t key = 0;
	int ret = 0;

	ARG_UNUSED(dev_cfg);

	do {
		/* Validate the DMA channel */
		if (channel >= dev_data->dma_ctx.dma_channels) {
			LOG_ERR("Unsupported channel");
			ret = -EINVAL;
			break;
		}

		/* Lock interrupts to ensure atomic operation */
		key = irq_lock();

		/* Disable the channel */
		dmac_ch_enable(DMAC_REGS, channel, false);

		/* Put descriptors back to the pool, if configured and not returned to the pool */
		dma_mchp_desc_return_pool(dev_data, channel);

		/* Unlock interrupts */
		irq_unlock(key);

		ret = 0;

	} while (0);

	return ret;
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
	dma_mchp_dev_data_t *const dev_data = dev->data;
	dma_mchp_channel_config_t *channel_config = NULL;
	int ret = 0;
	uint32_t key = 0;
	bool irq_locked = false;
	dma_mchp_ch_state_t ch_state = DMA_MCHP_CH_IDLE;

	do {
		/* Validate the DMA channel */
		if (channel >= dev_data->dma_ctx.dma_channels) {
			LOG_ERR("Unsupported channel");
			ret = -EINVAL;
			break;
		}

		/* Check if the channel is configured */
		channel_config = &dev_data->dma_channel_config[channel];
		if (channel_config->is_configured != true) {
			LOG_ERR("DMA descriptors not configured for channel : %d", channel);
			ret = -EINVAL;
			break;
		}

		/* Check if the channel is already in use */
		ch_state = dmac_ch_get_state(DMAC_REGS, channel);
		if (ch_state == DMA_MCHP_CH_ACTIVE) {
			LOG_ERR("DMA channel:%d is currently busy", channel);
			ret = -EBUSY;
			break;
		}

		/* Lock interrupts to ensure atomic update */
		key = irq_lock();
		irq_locked = true;

		/* Reloads the DMA descriptor with the specified source, destination, and size.
		 * Returns an error code if the provided parameters are invalid.
		 */
		ret = dmac_desc_reload_block(dev_data->dmac_desc_data, channel, src, dst, size);
		if (ret < 0) {
			break;
		}

		LOG_DBG("Reloaded channel %d for %08X to %08X (%u)", channel, src, dst, size);
		ret = 0;
	} while (0);

	if (irq_locked == true) {
		/* Unlock the IRQ */
		irq_unlock(key);
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
	/* Get device config */
	const dma_mchp_dev_config_t *dev_cfg = dev->config;
	/* Get dev data */
	dma_mchp_dev_data_t *const dev_data = dev->data;
	unsigned int key = 0;
	dma_mchp_ch_state_t ch_state = DMA_MCHP_CH_IDLE;
	int ret = 0;

	ARG_UNUSED(dev_cfg);

	do {
		/* Validate the DMA channel */
		if (channel >= dev_data->dma_ctx.dma_channels) {
			LOG_ERR("Unsupported channel");
			ret = -EINVAL;
		}

		/* Check if the channel is currently active */
		ch_state = dmac_ch_get_state(DMAC_REGS, channel);
		if (ch_state != DMA_MCHP_CH_ACTIVE) {
			LOG_INF("nothing to suspend as dma channel %u is not busy", channel);
			ret = 0;
		}

		/* Lock interrupts to prevent race conditions */
		key = irq_lock();
		/* Suspend the specified DMA channel */
		dmac_ch_suspend(DMAC_REGS, channel);
		LOG_DBG("channel %u is suspended", channel);
		/* Unlock interrupts */
		irq_unlock(key);

		ret = 0;

	} while (0);

	return ret;
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
	/* Get device config */
	const dma_mchp_dev_config_t *dev_cfg = dev->config;
	/* Get dev data */
	dma_mchp_dev_data_t *const dev_data = dev->data;
	uint32_t key = 0;
	dma_mchp_ch_state_t ch_state = DMA_MCHP_CH_IDLE;
	int ret = 0;

	ARG_UNUSED(dev_cfg);

	do {
		/* Validate the DMA channel */
		if (channel >= dev_data->dma_ctx.dma_channels) {
			LOG_ERR("Unsupported channel");
			ret = -EINVAL;
		}

		/* Check if the channel is already suspended */
		ch_state = dmac_ch_get_state(DMAC_REGS, channel);
		if (ch_state != DMA_MCHP_CH_SUSPENDED) {
			LOG_INF("DMA channel %d is not in suspended state so cannot resume channel",
				channel);
			ret = 0;
		}

		/* Lock interrupts to prevent race conditions */
		key = irq_lock();
		/* Resume the specified DMA channel */
		dmac_ch_resume(DMAC_REGS, channel);
		LOG_DBG("channel %u is resumed", channel);
		/* Unlock interrupts */
		irq_unlock(key);

		ret = 0;

	} while (0);

	return ret;
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
	/* Get device config */
	const dma_mchp_dev_config_t *dev_cfg = dev->config;
	/* Get dev data */
	dma_mchp_dev_data_t *const dev_data = dev->data;
	int ret = 0;

	ARG_UNUSED(dev_cfg);

	do {
		/* Validate the DMA channel */
		if (channel >= dev_data->dma_ctx.dma_channels) {
			LOG_ERR("Unsupported channel");
			ret = -EINVAL;
		}

		/* Retrieve DMA channel status */
		ret = dmac_ch_get_status(DMAC_REGS, dev_data->dmac_desc_data, channel, stat);
		if (ret < 0) {
			break;
		}

	} while (0);

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
	uint32_t requested_channel = 0;
	bool ret = false;

	/* If no specific channel is requested, allow any available channel */
	if (filter_param == NULL) {
		ret = true;
	} else {
		requested_channel = *(uint32_t *)filter_param;
		/* Allow only if current channel matches the requested one */
		ret = (bool)(channel == requested_channel);
	}

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

/*******************************************
 * Driver Initialization Function
 *******************************************/
/**
 * @brief Initializes the Microchip DMA controller.
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
	const dma_mchp_dev_config_t *dev_cfg = dev->config;
	/* Get dev data */
	dma_mchp_dev_data_t *const dev_data = dev->data;
	int ret = 0;

	do {
		/* Enable DMA clock */
		ret = clock_control_on(dev_cfg->clock_dev, dev_cfg->mclk_sys);

		if (ret < 0 && ret != -EALREADY) {
			LOG_ERR("Failed to enable MCLK for DMA: %d", ret);
			break;
		}

		/* Reset the DMA controller */
		dmac_controller_reset(DMAC_REGS);

		/* Initialize DMA descriptors */
		dmac_desc_init(dev);

		/* Set default priority levels */
		dmac_set_default_priority(DMAC_REGS);

		/* Enable the DMA controller */
		dmac_enable(DMAC_REGS, true);

		/* Configure DMA interrupt */
		dev_cfg->irq_config();

		/* Create the pool of descriptors */
		ret = dma_mchp_desc_pool_create(
			&dev_data->dma_desc_pool, &dev_data->dma_desc_pool_cnt,
			dev_data->dma_alignment, CONFIG_DMA_MCHP_DMAC_G1_MAX_DESC);
		if (ret < 0) {
			LOG_ERR("DMA Pool creation failed!");
			break;
		}
		LOG_DBG("DMA Pool creation success!");
	} while (0);

	/* If everything OK but clocks were already on, return 0 */
	return (ret == -EALREADY) ? 0 : ret;
}

/*******************************************
 * Zephyr Driver API Structure
 *******************************************/
/**
 * @brief DMA driver API structure.
 *
 * This structure defines the supported API functions for the DMA driver.
 */
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

/*******************************************
 * Driver Instantiation
 *******************************************/
/**
 * @brief Declare the DMA IRQ connection handler for a specific instance.
 *
 * This macro defines a static function prototype for connecting
 * the DMA interrupt handler of a given DMA instance.
 *
 * @param n Instance index of the DMA controller.
 */
#define DMA_MCHP_IRQ_HANDLER_DECL(n) static void mchp_dma_irq_connect_##n(void)

/**
 * @brief Enable IRQ lines for the DMA controller.
 *
 * This macro connects the interrupt line to the DMA ISR and enables it
 * if the instance has an associated IRQ.
 *
 * @param idx  The index of the IRQ in the instance.
 * @param inst The instance number from the device tree.
 */
#define DMA_MCHP_IRQ_CONNECT(idx, n)                                                               \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, idx), ( \
		/** Connect the IRQ to the DMA ISR */ \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq), \
			DT_INST_IRQ_BY_IDX(n, idx, priority), \
			dma_mchp_isr, \
			DEVICE_DT_INST_GET(n), 0); \
		/** Enable the IRQ */ \
		irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq)); \
	))

/**
 * @brief Define the DMA IRQ handler function for a given instance.
 *
 * This macro generates a function to connect all IRQ lines of a DMA instance
 * by calling DMA_MCHP_IRQ_CONNECT for each available IRQ in the instance.
 *
 * @param n The DMA instance number.
 */
#define DMA_MCHP_IRQ_HANDLER(n)                                                                    \
	static void mchp_dma_irq_connect_##n(void)                                                 \
	{                                                                                          \
		/** Connect all IRQs for this instance */                                          \
		LISTIFY(\
			DT_NUM_IRQS(DT_DRV_INST(n)), \
			DMA_MCHP_IRQ_CONNECT, \
			(), \
			n\
		)                                                                          \
	}

/**
 * @brief DMA runtime data structure.
 *
 * This macro declares and initializes DMA-related data structures
 * required for a specific DMA instance.
 *
 * @param n Instance index of the DMA controller.
 *
 */
#define DMA_MCHP_DATA_DEFN(n)                                                                      \
	static dma_mchp_dmac_t dmac_desc_data_##n;                                                 \
	ATOMIC_DEFINE(dma_mchp_atomic##n, DT_INST_PROP(n, dma_channels));                          \
	static dma_mchp_channel_config_t dma_channel_config_##n[DT_INST_PROP(n, dma_channels)];    \
	static dma_mchp_dev_data_t dma_mchp_dev_data_##n = {                                       \
		.dma_ctx =                                                                         \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.atomic = dma_mchp_atomic##n,                                      \
				.dma_channels = DT_INST_PROP(n, dma_channels),                     \
			},                                                                         \
		.dmac_desc_data = &dmac_desc_data_##n,                                             \
		.dma_desc_pool_cnt = 0,                                                            \
		.dma_channel_config = dma_channel_config_##n,                                      \
		.dma_alignment = DT_INST_PROP(n, dma_alignment),                                   \
	};

/**
 * @brief DMA device configuration structure.
 *
 * This structure holds the static configuration of the DMA device.
 */
#define DMA_MCHP_CONFIG_DEFN(n)                                                                    \
	static const dma_mchp_dev_config_t dma_mchp_dev_config_##n = {                             \
		.regs = ((dmac_registers_t *)DT_INST_REG_ADDR(n)),                                 \
		.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),             \
		.num_irq = DT_NUM_IRQS(DT_DRV_INST(n)),                                            \
		.irq_config = mchp_dma_irq_connect_##n,                                            \
		.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock))};

/**
 * @brief Define and initialize the DMA device.
 *
 * This macro registers the DMA driver with the Zephyr device model.
 * It initializes the DMA controller during the pre-kernel phase.
 */
#define DMA_MCHP_DEVICE_INIT(n)                                                                    \
	DMA_MCHP_IRQ_HANDLER_DECL(n);                                                              \
	DMA_MCHP_DATA_DEFN(n);                                                                     \
	DMA_MCHP_CONFIG_DEFN(n);                                                                   \
	DEVICE_DT_INST_DEFINE(n, &dma_mchp_init, NULL, &dma_mchp_dev_data_##n,                     \
			      &dma_mchp_dev_config_##n, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,    \
			      &dma_mchp_api);                                                      \
	DMA_MCHP_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(DMA_MCHP_DEVICE_INIT);
