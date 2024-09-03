/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * **************************************************************************
 * MSPI flash controller driver for stm32 serie with multi-SPI periherals
 * This driver is based on the stm32Cube HAL XSPI driver
 * with one mspi DTS NODE
 * **************************************************************************
 */
#define DT_DRV_COMPAT st_stm32_mspi_controller

#include <errno.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mspi_stm32, CONFIG_MSPI_LOG_LEVEL);


//#define MSPI_STM32_NODE DT_INST(0)
#define MSPI_NOR_STM32_NODE DT_CHILD(0)

/*#define DT_MSPI_IO_PORT_PROP_OR(prop, default_value)					\
	COND_CODE_1(DT_NODE_HAS_PROP(MSPI_STM32_NODE, prop),				\
		    (_CONCAT(HAL_XSPIM_, DT_STRING_TOKEN(MSPI_STM32_NODE, prop))),	\
		    ((default_value)))
*/
/* Get the base address of the flash from the DTS node */
#define MSPI_STM32_BASE_ADDRESS DT_INST_REG_ADDR(0)

#define MSPI_STM32_RESET_GPIO DT_INST_NODE_HAS_PROP(0, reset_gpios)

//#define MSPI_STM32_DLYB_BYPASSED DT_PROP(MSPI_STM32_NODE, dlyb_bypass)

#define MSPI_STM32_USE_DMA DT_NODE_HAS_PROP(0, dmas)

#if MSPI_STM32_USE_DMA
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <stm32_ll_dma.h>
#endif /* MSPI_STM32_USE_DMA */

#include "mspi_stm32.h"

static inline void mspi_lock_thread(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);
}

static inline void mspi_unlock_thread(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
}

static inline void mspi_context_release(struct mspi_context *ctx)
{
	ctx->owner = NULL;
	k_sem_give(&ctx->lock);
}

static inline int mspi_context_lock(struct mspi_context *ctx,
				    const struct mspi_dev_id *req,
				    const struct mspi_xfer *xfer,
				    mspi_callback_handler_t callback,
				    struct mspi_callback_context *callback_ctx,
				    bool lockon)
{
	int ret = 1;

	if ((k_sem_count_get(&ctx->lock) == 0) && !lockon &&
	    (ctx->owner == req)) {
		return 0;
	}

	if (k_sem_take(&ctx->lock, K_MSEC(xfer->timeout))) {
		return -EBUSY;
	}
	if (ctx->xfer.async) {
		if ((xfer->tx_dummy == ctx->xfer.tx_dummy) &&
		    (xfer->rx_dummy == ctx->xfer.rx_dummy) &&
		    (xfer->cmd_length == ctx->xfer.cmd_length) &&
		    (xfer->addr_length == ctx->xfer.addr_length)) {
			ret = 0;
		} else if (ctx->packets_left == 0) {
			if (ctx->callback_ctx) {
				volatile struct mspi_event_data *evt_data;

				evt_data = &ctx->callback_ctx->mspi_evt.evt_data;
				while (evt_data->status != 0) {
				}
				ret = 1;
			} else {
				ret = 0;
			}
		} else {
			return -EIO;
		}
	}
	ctx->owner           = req;
	ctx->xfer            = *xfer;
	ctx->packets_done    = 0;
	ctx->packets_left    = ctx->xfer.num_packet;
	ctx->callback        = callback;
	ctx->callback_ctx    = callback_ctx;
	return ret;
}

/**
 * Check if the MSPI bus is busy.
 *
 * @param controller MSPI emulation controller device.
 * @return true The MSPI bus is busy.
 * @return false The MSPI bus is idle.
 */
static inline bool mspi_is_inp(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	return (k_sem_count_get(&dev_data->ctx.lock) == 0);
}

static uint32_t mspi_stm32_hal_address_size(const struct device *controller)
{
	const struct mspi_stm32_conf *dev_cfg = controller->config;
	struct mspi_stm32_data *dev_data = controller->data;

	if (dev_data->dev_cfg.addr_length == 4U) {
		return HAL_XSPI_ADDRESS_32_BITS;
	}

	return HAL_XSPI_ADDRESS_24_BITS;
}

/**
 * Configure hardware before a transfer.
 *
 * @param controller Pointer to the MSPI controller instance.
 * @param xfer Pointer to the MSPI transfer started by the request entity.
 * @return 0 if successful.
 */
static int mspi_xfer_config(const struct device *controller,
			    const struct mspi_xfer *xfer)
{
	struct mspi_stm32_data *data = controller->data;

	data->dev_cfg.cmd_length      = xfer->cmd_length;
	data->dev_cfg.addr_length     = xfer->addr_length;
	data->dev_cfg.tx_dummy        = xfer->tx_dummy;
	data->dev_cfg.rx_dummy        = xfer->rx_dummy;

	return 0;
}

/*
 * Gives a XSPI_RegularCmdTypeDef with all parameters set
 * except Instruction, Address, NbData
 */
static XSPI_RegularCmdTypeDef mspi_stm32_prepare_cmd(uint8_t cfg_mode, uint8_t cfg_rate)
{
	XSPI_RegularCmdTypeDef cmd_tmp;

	cmd_tmp.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
	cmd_tmp.InstructionWidth = ((cfg_mode == MSPI_IO_MODE_OCTAL)
			? HAL_XSPI_INSTRUCTION_16_BITS
			: HAL_XSPI_INSTRUCTION_8_BITS);
	cmd_tmp.InstructionDTRMode = ((cfg_rate == MSPI_DATA_RATE_DUAL)
				? HAL_XSPI_INSTRUCTION_DTR_ENABLE
				: HAL_XSPI_INSTRUCTION_DTR_DISABLE);
	cmd_tmp.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
	cmd_tmp.AddressDTRMode = ((cfg_rate == MSPI_DATA_RATE_DUAL)
				? HAL_XSPI_ADDRESS_DTR_ENABLE
				: HAL_XSPI_ADDRESS_DTR_DISABLE);
	cmd_tmp.DataDTRMode = ((cfg_rate == MSPI_DATA_RATE_DUAL)
			? HAL_XSPI_DATA_DTR_ENABLE
			: HAL_XSPI_DATA_DTR_DISABLE);
	/* AddressWidth must be set to 32bits for init and mem config phase */
	cmd_tmp.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
	cmd_tmp.DataDTRMode = ((cfg_rate == MSPI_DATA_RATE_DUAL)
				? HAL_XSPI_DATA_DTR_ENABLE
				: HAL_XSPI_DATA_DTR_DISABLE);
	cmd_tmp.DQSMode = ((cfg_rate == MSPI_DATA_RATE_DUAL)
			? HAL_XSPI_DQS_ENABLE
			: HAL_XSPI_DQS_DISABLE);
	cmd_tmp.SIOOMode = HAL_XSPI_SIOO_INST_EVERY_CMD;

	switch (cfg_mode) {
	case MSPI_IO_MODE_OCTAL: {
		cmd_tmp.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
		cmd_tmp.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
		cmd_tmp.DataMode = HAL_XSPI_DATA_8_LINES;
		break;
	}
	case MSPI_IO_MODE_QUAD: {
		cmd_tmp.InstructionMode = HAL_XSPI_INSTRUCTION_4_LINES;
		cmd_tmp.AddressMode = HAL_XSPI_ADDRESS_4_LINES;
		cmd_tmp.DataMode = HAL_XSPI_DATA_4_LINES;
		break;
	}
	case MSPI_IO_MODE_DUAL: {
		cmd_tmp.InstructionMode = HAL_XSPI_INSTRUCTION_2_LINES;
		cmd_tmp.AddressMode = HAL_XSPI_ADDRESS_2_LINES;
		cmd_tmp.DataMode = HAL_XSPI_DATA_2_LINES;
		break;
	}
	default: {
		cmd_tmp.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
		cmd_tmp.DataMode = HAL_XSPI_DATA_1_LINE;
		break;
	}
	}

	return cmd_tmp;
}

/* Send a Command to the NOR and Receive data if relevant */
static int mspi_stm32_read_access(const struct device *dev,
				  const struct mspi_xfer_packet *packet,
				  uint8_t access_mode)
{
	struct mspi_stm32_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;
	XSPI_RegularCmdTypeDef cmd = mspi_stm32_prepare_cmd(dev_data->dev_cfg.io_mode,
							    dev_data->dev_cfg.data_rate);

	cmd.DataLength = packet->num_bytes;
	cmd.Instruction = packet->cmd;
	cmd.DummyCycles = dev_data->ctx.xfer.tx_dummy;

	LOG_DBG("Instruction 0x%x", cmd.Instruction);

	if (dev_data->dev_cfg.io_mode != MSPI_IO_MODE_OCTAL) {
		switch (dev_data->dev_cfg.read_cmd) {
		case JESD216_MODE_112: {
			cmd.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
			cmd.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
			cmd.DataMode = HAL_XSPI_DATA_2_LINES;
			break;
		}
		case JESD216_MODE_122: {
			cmd.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
			cmd.AddressMode = HAL_XSPI_ADDRESS_2_LINES;
			cmd.DataMode = HAL_XSPI_DATA_2_LINES;
			break;
		}
		case JESD216_MODE_114: {
			cmd.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
			cmd.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
			cmd.DataMode = HAL_XSPI_DATA_4_LINES;
			break;
		}
		case JESD216_MODE_144: {
			cmd.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
			cmd.AddressMode = HAL_XSPI_ADDRESS_4_LINES;
			cmd.DataMode = HAL_XSPI_DATA_4_LINES;
			break;
		}
		default:
			/* use the mode from prepare_cmd */
			break;
		}
	}

	/* Instruction and DummyCycles are set below */
	cmd.Address = packet->address; /* AddressSize is 32bits in OPSI mode */
	cmd.AddressWidth = mspi_stm32_hal_address_size(dev);
	/* DataSize is set by the read cmd */

	/* Configure other parameters */
	if (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_DUAL) {
		/* DTR transfer rate (==> Octal mode) */
		cmd.Instruction = SPI_NOR_OCMD_DTR_RD;
		cmd.DummyCycles = SPI_NOR_DUMMY_RD_OCTAL_DTR;
	} else {
		/* STR transfer rate */
		if (dev_data->dev_cfg.io_mode == MSPI_IO_MODE_OCTAL) {
			/* OPI and STR */
			cmd.Instruction = SPI_NOR_OCMD_RD;
			cmd.DummyCycles = SPI_NOR_DUMMY_RD_OCTAL;
		} else {
			/* use SFDP:BFP read instruction */
			cmd.Instruction = dev_data->dev_cfg.read_cmd;
			cmd.DummyCycles = dev_data->dev_cfg.rx_dummy;
			/* in SPI and STR : expecting SPI_NOR_CMD_READ_FAST_4B */
		}
	}
	LOG_DBG("Instruction 0x%x", cmd.Instruction);

	cmd.DataLength = packet->num_bytes;

	dev_data->cmd_status = 0;

	hal_ret = HAL_XSPI_Command(&dev_data->hmspi, &cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send XSPI instruction", hal_ret);
		return -EIO;
	}

	if (packet->num_bytes == 0) {
		/* no data to receive : done */
		goto end_rd_access;
	}

	switch (access_mode) {
	case MSPI_ACCESS_SYNC:
		hal_ret = HAL_XSPI_Receive(&dev_data->hmspi,
					packet->data_buf,
					HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
		goto end_rd_access;
		break;
	case MSPI_ACCESS_ASYNC:
		hal_ret = HAL_XSPI_Receive_IT(&dev_data->hmspi, packet->data_buf);
		break;
#if MSPI_STM32_USE_DMA
	case MSPI_ACCESS_DMA:
		hal_ret = HAL_XSPI_Receive_DMA(&dev_data->hmspi, packet->data_buf);
		break;
#endif /* MSPI_STM32_USE_DMA */
	default:
		/* Not correct */
		hal_ret = HAL_BUSY;
		break;
	}

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}

	LOG_DBG("Read %zu data at 0x%lx",
		packet->num_bytes,
		(long)(packet->address));

	/* sem taken by the calling function */
	k_sem_take(&dev_data->sync, K_FOREVER);

end_rd_access:
	return dev_data->cmd_status;
}

/* Send a Command to the NOR and Transmit data if relevant */
static int mspi_stm32_write_access(const struct device *dev,
				  const struct mspi_xfer_packet *packet,
				  uint8_t access_mode)
{
	struct mspi_stm32_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;
	XSPI_RegularCmdTypeDef cmd = mspi_stm32_prepare_cmd(dev_data->dev_cfg.io_mode,
							    dev_data->dev_cfg.data_rate);

	cmd.DataLength = packet->num_bytes;
	cmd.Instruction = packet->cmd;
	cmd.DummyCycles = dev_data->ctx.xfer.tx_dummy;

	LOG_DBG("Instruction 0x%x", cmd.Instruction);

	dev_data->cmd_status = 0;

	/* in OPI/STR the 3-byte AddressWidth is not supported by the NOR flash */
	if ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_OCTAL) &&
		(cmd.AddressWidth != HAL_XSPI_ADDRESS_32_BITS)) {
		LOG_ERR("XSPI wr in OPI/STR mode is for 32bit address only");
		return -EIO;
	}

	hal_ret = HAL_XSPI_Command(&dev_data->hmspi, &cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send XSPI instruction", hal_ret);
		return -EIO;
	}

	if (packet->num_bytes == 0) {
		/* Only command to send, no data : done */
		goto end_wr_access;
	}

	switch (access_mode) {
	case MSPI_ACCESS_SYNC:
		hal_ret = HAL_XSPI_Transmit(&dev_data->hmspi,
					packet->data_buf,
					HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
		goto end_wr_access;
		break;
	case MSPI_ACCESS_ASYNC:
		hal_ret = HAL_XSPI_Transmit_IT(&dev_data->hmspi, packet->data_buf);
		break;
#if MSPI_STM32_USE_DMA
	case MSPI_ACCESS_DMA:
		hal_ret = HAL_XSPI_Transmit_DMA(&dev_data->hmspi, packet->data_buf);
		break;
#endif /* MSPI_STM32_USE_DMA */
	default:
		/* Not correct */
		hal_ret = HAL_BUSY;
		break;
	}

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to write data", hal_ret);
		return -EIO;
	}

	/* sem taken by the calling function */
	k_sem_take(&dev_data->sync, K_FOREVER);

end_wr_access:
	return dev_data->cmd_status;
}

static int mspi_stm32_wait_auto_polling(const struct device *dev,
		XSPI_AutoPollingTypeDef *s_config, uint32_t timeout_ms)
{
	struct mspi_stm32_data *dev_data = dev->data;

	dev_data->cmd_status = 0;

	if (HAL_XSPI_AutoPolling_IT(&dev_data->hmspi, s_config) != HAL_OK) {
		LOG_ERR("MSPI AutoPoll failed");
		return -EIO;
	}

	if (k_sem_take(&dev_data->sync, K_MSEC(timeout_ms)) != 0) {
		LOG_ERR("MSPI AutoPoll wait failed");
		HAL_XSPI_Abort(&dev_data->hmspi);
		k_sem_reset(&dev_data->sync);
		return -EIO;
	}

	/* HAL_XSPI_AutoPolling_IT enables transfer error interrupt which sets
	 * cmd_status.
	 */
	return dev_data->cmd_status;
}

#if 0
/*
 * This function Polls the WEL (write enable latch) bit to become to 0
 * When the Chip Erase Cycle is completed, the Write Enable Latch (WEL) bit is cleared.
 * in cfg_mode SPI/OPI MSPI_IO_MODE_SINGLE or MSPI_IO_MODE_OCTAL
 * and cfg_rate transfer STR/DTR MSPI_DATA_RATE_SINGLE or MSPI_DATA_RATE_DUAL
 */
static int mspi_stm32_mem_erased(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;
	uint8_t cfg_mode = dev_data->dev_cfg.io_mode;
	uint8_t cfg_rate = dev_data->dev_cfg.data_rate;

	XSPI_AutoPollingTypeDef s_config = {0};
	XSPI_RegularCmdTypeDef s_command = flash_mspi_stm32_prepare_cmd(cfg_mode, cfg_rate);

	/* Configure automatic polling mode command to wait for memory ready */
	if (cfg_mode == MSPI_IO_MODE_OCTAL) {
		s_command.Instruction = SPI_NOR_OCMD_RDSR;
		s_command.DummyCycles = (cfg_rate == MSPI_DATA_RATE_DUAL)
					? SPI_NOR_DUMMY_REG_OCTAL_DTR
					: SPI_NOR_DUMMY_REG_OCTAL;
	} else {
		s_command.Instruction = SPI_NOR_CMD_RDSR;
		/* force 1-line InstructionMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
		s_command.AddressMode = HAL_XSPI_ADDRESS_NONE;
		/* force 1-line DataMode for any non-OSPI transfer */
		s_command.DataMode = HAL_XSPI_DATA_1_LINE;
		s_command.DummyCycles = 0;
	}
	s_command.DataLength = ((cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U);
	s_command.Address = 0U;

	/* Set the mask to  0x02 to mask all Status REG bits except WEL */
	/* Set the match to 0x00 to check if the WEL bit is Reset */
	s_config.MatchValue         = SPI_NOR_WEL_MATCH;
	s_config.MatchMask          = SPI_NOR_WEL_MASK; /* Write Enable Latch */

	s_config.MatchMode          = HAL_XSPI_MATCH_MODE_AND;
	s_config.IntervalTime       = SPI_NOR_AUTO_POLLING_INTERVAL;
	s_config.AutomaticStop      = HAL_XSPI_AUTOMATIC_STOP_ENABLE;

	if (HAL_XSPI_Command(&dev_data->hmspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI AutoPoll command (WEL) failed");
		return -EIO;
	}

	/* Start Automatic-Polling mode to wait until the memory is totally erased */
	return flash_mspi_stm32_wait_auto_polling(dev,
			&s_config, MSPI_STM32_BULK_ERASE_MAX_TIME);
}

#endif

/*
 * This function Polls the WIP(Write In Progress) bit to become to 0
 * in cfg_mode SPI/OPI MSPI_IO_MODE_SINGLE or MSPI_IO_MODE_OCTAL
 * and cfg_rate transfer STR/DTR MSPI_DATA_RATE_SINGLE or MSPI_DATA_RATE_DUAL
 */
static int mspi_stm32_mem_ready(const struct device *dev, uint8_t cfg_mode,
		uint8_t cfg_rate)
{
	struct mspi_stm32_data *dev_data = dev->data;

	XSPI_AutoPollingTypeDef s_config = {0};
	XSPI_RegularCmdTypeDef s_command = mspi_stm32_prepare_cmd(cfg_mode, cfg_rate);

	/* Configure automatic polling mode command to wait for memory ready */
	if (cfg_mode == MSPI_IO_MODE_OCTAL) {
		s_command.Instruction = SPI_NOR_OCMD_RDSR;
		s_command.DummyCycles = (cfg_rate == MSPI_DATA_RATE_DUAL)
					? SPI_NOR_DUMMY_REG_OCTAL_DTR
					: SPI_NOR_DUMMY_REG_OCTAL;
	} else {
		s_command.Instruction = SPI_NOR_CMD_RDSR;
		/* force 1-line InstructionMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
		s_command.AddressMode = HAL_XSPI_ADDRESS_NONE;
		/* force 1-line DataMode for any non-OSPI transfer */
		s_command.DataMode = HAL_XSPI_DATA_1_LINE;
		s_command.DummyCycles = 0;
	}
	s_command.DataLength = ((cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U);
	s_command.Address = 0U;

	/* Set the mask to  0x01 to mask all Status REG bits except WIP */
	/* Set the match to 0x00 to check if the WIP bit is Reset */
	s_config.MatchValue         = SPI_NOR_MEM_RDY_MATCH;
	s_config.MatchMask          = SPI_NOR_MEM_RDY_MASK; /* Write in progress */
	s_config.MatchMode          = HAL_XSPI_MATCH_MODE_AND;
	s_config.IntervalTime       = SPI_NOR_AUTO_POLLING_INTERVAL;
	s_config.AutomaticStop      = HAL_XSPI_AUTOMATIC_STOP_ENABLE;

	if (HAL_XSPI_Command(&dev_data->hmspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI AutoPoll command failed");
		return -EIO;
	}

	/* Start Automatic-Polling mode to wait until the memory is ready WIP=0 */
	return mspi_stm32_wait_auto_polling(dev, &s_config, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
}

/* Enables writing to the memory sending a Write Enable and wait it is effective */
static int mspi_stm32_write_enable_cmd(const struct device *dev,
		uint8_t cfg_mode, uint8_t cfg_rate)
{
	struct mspi_stm32_data *dev_data = dev->data;

	XSPI_AutoPollingTypeDef s_config = {0};
	XSPI_RegularCmdTypeDef s_command = mspi_stm32_prepare_cmd(cfg_mode, cfg_rate);

	/* Initialize the write enable command */
	if (cfg_mode == MSPI_IO_MODE_OCTAL) {
		s_command.Instruction = SPI_NOR_OCMD_WREN;
	} else {
		s_command.Instruction = SPI_NOR_CMD_WREN;
		/* force 1-line InstructionMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
	}
	s_command.AddressMode = HAL_XSPI_ADDRESS_NONE;
	s_command.DataMode    = HAL_XSPI_DATA_NONE;
	s_command.DummyCycles = 0U;

	if (HAL_XSPI_Command(&dev_data->hmspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI flash write enable cmd failed");
		return -EIO;
	}

	/* New command to Configure automatic polling mode to wait for write enabling */
	if (cfg_mode == MSPI_IO_MODE_OCTAL) {
		s_command.Instruction = SPI_NOR_OCMD_RDSR;
		s_command.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
		s_command.DataMode = HAL_XSPI_DATA_8_LINES;
		s_command.DummyCycles = (cfg_rate == MSPI_DATA_RATE_DUAL)
				? SPI_NOR_DUMMY_REG_OCTAL_DTR
						: SPI_NOR_DUMMY_REG_OCTAL;
	} else {
		s_command.Instruction = SPI_NOR_CMD_RDSR;
		/* force 1-line DataMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
		s_command.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
		s_command.DataMode = HAL_XSPI_DATA_1_LINE;
		s_command.DummyCycles = 0;

		/* DummyCycles remains 0 */
	}
	s_command.DataLength = (cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U;
	s_command.Address = 0U;

	if (HAL_XSPI_Command(&dev_data->hmspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI config auto polling cmd failed");
		return -EIO;
	}

	s_config.MatchValue      = SPI_NOR_WREN_MATCH;
	s_config.MatchMask       = SPI_NOR_WREN_MASK;
	s_config.MatchMode       = HAL_XSPI_MATCH_MODE_AND;
	s_config.IntervalTime    = SPI_NOR_AUTO_POLLING_INTERVAL;
	s_config.AutomaticStop   = HAL_XSPI_AUTOMATIC_STOP_ENABLE;

	return mspi_stm32_wait_auto_polling(dev, &s_config, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
}

/* Write Flash configuration register 2 with new dummy cycles */
static int mspi_stm32_write_cfg2reg_dummy(XSPI_HandleTypeDef *hmspi,
					uint8_t cfg_mode, uint8_t cfg_rate)
{
	uint8_t transmit_data = SPI_NOR_CR2_DUMMY_CYCLES_66MHZ;
	XSPI_RegularCmdTypeDef s_command = mspi_stm32_prepare_cmd(cfg_mode, cfg_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (cfg_mode == MSPI_IO_MODE_SINGLE)
				? SPI_NOR_CMD_WR_CFGREG2
				: SPI_NOR_OCMD_WR_CFGREG2;
	s_command.Address = SPI_NOR_REG2_ADDR3;
	s_command.DummyCycles = 0U;
	s_command.DataLength = (cfg_mode == MSPI_IO_MODE_SINGLE) ? 1U
			: ((cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U);

	if (HAL_XSPI_Command(hmspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI transmit cmd");
		return -EIO;
	}

	if (HAL_XSPI_Transmit(hmspi, &transmit_data,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI transmit ");
		return -EIO;
	}

	return 0;
}

/* Write Flash configuration register 2 with new single or octal SPI protocol */
static int mspi_stm32_write_cfg2reg_io(XSPI_HandleTypeDef *hmspi,
				       uint8_t cfg_mode, uint8_t cfg_rate, uint8_t op_enable)
{
	XSPI_RegularCmdTypeDef s_command = mspi_stm32_prepare_cmd(cfg_mode, cfg_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (cfg_mode == MSPI_IO_MODE_SINGLE)
				? SPI_NOR_CMD_WR_CFGREG2
				: SPI_NOR_OCMD_WR_CFGREG2;
	s_command.Address = SPI_NOR_REG2_ADDR1;
	s_command.DummyCycles = 0U;
	s_command.DataLength = (cfg_mode == MSPI_IO_MODE_SINGLE) ? 1U
		: ((cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U);

	if (HAL_XSPI_Command(hmspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	if (HAL_XSPI_Transmit(hmspi, &op_enable,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	return 0;
}

/* Read Flash configuration register 2 with new single or octal SPI protocol */
static int mspi_stm32_read_cfg2reg(XSPI_HandleTypeDef *hmspi,
				   uint8_t cfg_mode, uint8_t cfg_rate, uint8_t *value)
{
	XSPI_RegularCmdTypeDef s_command = mspi_stm32_prepare_cmd(cfg_mode, cfg_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (cfg_mode == MSPI_IO_MODE_SINGLE)
				? SPI_NOR_CMD_RD_CFGREG2
				: SPI_NOR_OCMD_RD_CFGREG2;
	s_command.Address = SPI_NOR_REG2_ADDR1;
	s_command.DummyCycles = (cfg_mode == MSPI_IO_MODE_SINGLE)
				? 0U
				: ((cfg_rate == MSPI_DATA_RATE_DUAL)
					? SPI_NOR_DUMMY_REG_OCTAL_DTR
					: SPI_NOR_DUMMY_REG_OCTAL);
	s_command.DataLength = (cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U;

	if (HAL_XSPI_Command(hmspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Read Flash configuration reg2 failed");
		return -EIO;
	}

	if (HAL_XSPI_Receive(hmspi, value, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Read Flash configuration reg2 failed");
		return -EIO;
	}

	return 0;
}

/* Set the NOR Flash to desired Interface mode : SPI/OSPI and STR/DTR according to the DTS */
static int mspi_stm32_config_mem(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;
	uint8_t reg[2];

	/* MSPI_IO_MODE_SINGLE/MSPI_DATA_RATE_SINGLE is already done */
	if ((cfg_mode == MSPI_IO_MODE_SINGLE)
			&& (cfg_rate == MSPI_DATA_RATE_SINGLE)) {
		LOG_INF("XSPI flash config is SPI / STR");
		return 0;
	}

	/* Going to set the MPI mode (STR or DTR transfer rate) */
	LOG_DBG("XSPI configuring Octo SPI mode");

	if (mspi_stm32_write_enable_cmd(dev,
		MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE) != 0) {
		LOG_ERR("MSPI write Enable failed");
		return -EIO;
	}

	/* Write Configuration register 2 (with new dummy cycles) */
	if (mspi_stm32_write_cfg2reg_dummy(&dev_data->hmspi,
		MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE) != 0) {
		LOG_ERR("MSPI write CFGR2 failed");
		return -EIO;
	}
	if (mspi_stm32_mem_ready(dev,
		MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE) != 0) {
		LOG_ERR("MSPI autopolling failed");
		return -EIO;
	}
	if (mspi_stm32_write_enable_cmd(dev,
		MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE) != 0) {
		LOG_ERR("MSPI write Enable 2 failed");
		return -EIO;
	}

	/* Write Configuration register 2 (with Octal I/O SPI protocol : choose STR or DTR) */
	uint8_t mode_enable = ((dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_DUAL)
				? SPI_NOR_CR2_DTR_OPI_EN
				: SPI_NOR_CR2_STR_OPI_EN);
	if (mspi_stm32_write_cfg2reg_io(&dev_data->hmspi,
		MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE, mode_enable) != 0) {
		LOG_ERR("MSPI write CFGR2 failed");
		return -EIO;
	}

	/* Wait that the configuration is effective and check that memory is ready */
	k_busy_wait(MSPI_STM32_WRITE_REG_MAX_TIME * USEC_PER_MSEC);

	/* Reconfigure the memory type of the peripheral */
	dev_data->hmspi.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
	dev_data->hmspi.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
	if (HAL_XSPI_Init(&dev_data->hmspi) != HAL_OK) {
		LOG_ERR("MSPI mem type MACRONIX failed");
		return -EIO;
	}

	if (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE) {
		if (mspi_stm32_mem_ready(dev,
			MSPI_IO_MODE_OCTAL, MSPI_DATA_RATE_SINGLE) != 0) {
			/* Check Flash busy ? */
			LOG_ERR("MSPI flash busy failed");
			return -EIO;
		}

		if (mspi_stm32_read_cfg2reg(&dev_data->hmspi,
			MSPI_IO_MODE_OCTAL, MSPI_DATA_RATE_SINGLE, reg) != 0) {
			/* Check the configuration has been correctly done on SPI_NOR_REG2_ADDR1 */
			LOG_ERR("MSPI flash config read failed");
			return -EIO;
		}

		LOG_INF("MSPI flash config is OCTO / STR");
	}

	if (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_DUAL) {
		if (mspi_stm32_mem_ready(dev,
			MSPI_IO_MODE_OCTAL, MSPI_DATA_RATE_DUAL) != 0) {
			/* Check Flash busy ? */
			LOG_ERR("MSPI flash busy failed");
			return -EIO;
		}

		LOG_INF("MSPI flash config is OCTO / DTR");
	}

	return 0;
}

/* gpio or send the different reset command to the NOR flash in SPI/OSPI and STR/DTR */
static int mspi_stm32_mem_reset(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

#if MSPI_STM32_RESET_GPIO
	const struct mspi_stm32_conf *dev_cfg = dev->config;

	/* Generate RESETn pulse for the flash memory */
	gpio_pin_configure_dt(&dev_cfg->reset, GPIO_OUTPUT_ACTIVE);
	k_msleep(DT_INST_PROP(0, reset_gpios_duration));
	gpio_pin_set_dt(&dev_cfg->reset, 0);
#else

	/* Reset command sent sucessively for each mode SPI/OPS & STR/DTR */
	XSPI_RegularCmdTypeDef s_command = {
		.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG,
		.AddressMode = HAL_XSPI_ADDRESS_NONE,
		.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE,
		.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE,
		.Instruction = SPI_NOR_CMD_RESET_EN,
		.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS,
		.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE,
		.DataLength = HAL_XSPI_DATA_NONE,
		.DummyCycles = 0U,
		.DQSMode = HAL_XSPI_DQS_DISABLE,
		.SIOOMode = HAL_XSPI_SIOO_INST_EVERY_CMD,
	};

	/* Reset enable in SPI mode and STR transfer mode */
	if (HAL_XSPI_Command(&dev_data->hmspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI reset enable (SPI/STR) failed");
		return -EIO;
	}

	/* Reset memory in SPI mode and STR transfer mode */
	s_command.Instruction = SPI_NOR_CMD_RESET_MEM;
	if (HAL_XSPI_Command(&dev_data->hmspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI reset memory (SPI/STR) failed");
		return -EIO;
	}

	/* Reset enable in OPI mode and STR transfer mode */
	s_command.InstructionMode    = HAL_XSPI_INSTRUCTION_8_LINES;
	s_command.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
	s_command.Instruction = SPI_NOR_OCMD_RESET_EN;
	s_command.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
	if (HAL_XSPI_Command(&dev_data->hmspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI reset enable (OCTO/STR) failed");
		return -EIO;
	}

	/* Reset memory in OPI mode and STR transfer mode */
	s_command.Instruction = SPI_NOR_OCMD_RESET_MEM;
	if (HAL_XSPI_Command(&dev_data->hmspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI reset memory (OCTO/STR) failed");
		return -EIO;
	}

	/* Reset enable in OPI mode and DTR transfer mode */
	s_command.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
	s_command.Instruction = SPI_NOR_OCMD_RESET_EN;
	if (HAL_XSPI_Command(&dev_data->hmspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI reset enable (OCTO/DTR) failed");
		return -EIO;
	}

	/* Reset memory in OPI mode and DTR transfer mode */
	s_command.Instruction = SPI_NOR_OCMD_RESET_MEM;
	if (HAL_XSPI_Command(&dev_data->hmspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI reset memory (OCTO/DTR) failed");
		return -EIO;
	}

#endif /* MSPI_STM32_RESET_GPIO */
	/* Wait after SWreset CMD, in case SWReset occurred during erase operation */
	k_busy_wait(MSPI_STM32_RESET_MAX_TIME * USEC_PER_MSEC);

	return 0;
}

static const struct flash_parameters mspi_stm32_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff
};

#if 0

#if defined(CONFIG_FLASH_JESD216_API)
/*
 * Read the JEDEC ID data from the external Flash at init
 * and store in the jedec_id Table of the mspi_stm32_data
 * The JEDEC ID is not given by a DTS property
 */
static int mspi_stm32_read_jedec_id(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	/* This is a SPI/STR command to issue to the external Flash device */
	XSPI_RegularCmdTypeDef cmd = mspi_prepare_cmd(MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE);

	cmd.Instruction = JESD216_CMD_READ_ID;
	cmd.AddressWidth = mspi_stm32_hal_address_size(dev);
	cmd.AddressMode = HAL_XSPI_ADDRESS_NONE;
	cmd.DataLength = JESD216_READ_ID_LEN; /* 3 bytes in the READ ID */

	HAL_StatusTypeDef hal_ret;

	hal_ret = HAL_XSPI_Command(&dev_data->hmspi, &cmd,
				   HAL_XSPI_TIMEOUT_DEFAULT_VALUE);

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send XSPI instruction", hal_ret);
		return -EIO;
	}

	/* Place the received data directly into the jedec Table */
	hal_ret = HAL_XSPI_Receive(&dev_data->hmspi, dev_data->jedec_id,
				   HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}

	LOG_DBG("Jedec ID = [%02x %02x %02x]",
		dev_data->jedec_id[0], dev_data->jedec_id[1], dev_data->jedec_id[2]);

	dev_data->cmd_status = 0;

	return 0;
}

/*
 * Read Serial Flash ID :
 * just gives the values received by the external Flash
 */
static int mspi_read_jedec_id(const struct device *dev,  uint8_t *id)
{
	struct mspi_stm32_data *dev_data = dev->data;

	/* Take jedec Id values from the table (issued from the octoFlash) */
	memcpy(id, dev_data->jedec_id, JESD216_READ_ID_LEN);

	LOG_INF("Manuf ID = %02x   Memory Type = %02x   Memory Density = %02x",
		id[0], id[1], id[2]);

	return 0;
}
#endif /* CONFIG_FLASH_JESD216_API */


/*
 * Read Serial Flash Discovery Parameter from the external Flash at init :
 * perform a read access over SPI bus for SDFP (DataMode is already set)
 * The SFDP table is not given by a DTS property
 */
static int mspi_stm32_read_sfdp(const struct device *dev, off_t addr,
				void *data,
				size_t size)
{
	struct mspi_stm32_data *dev_data = dev->data;

	XSPI_RegularCmdTypeDef cmd = mspi_prepare_cmd(dev_data->dev_cfg.io_mode,
						      dev_data->dev_cfg.data_rate);
	if (dev_data->dev_cfg.io_mode == MSPI_IO_MODE_OCTAL) {
		cmd.Instruction = JESD216_OCMD_READ_SFDP;
		cmd.DummyCycles = 20U;
		cmd.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
	} else {
		cmd.Instruction = JESD216_CMD_READ_SFDP;
		cmd.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
		cmd.DataMode = HAL_XSPI_DATA_1_LINE;
		cmd.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
		cmd.DummyCycles = 8U;
		cmd.AddressWidth = HAL_XSPI_ADDRESS_24_BITS;
	}
	cmd.Address = addr;
	cmd.DataLength = size;

	HAL_StatusTypeDef hal_ret;

	hal_ret = HAL_XSPI_Command(&dev_data->hmspi, &cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send XSPI instruction", hal_ret);
		return -EIO;
	}

	hal_ret = HAL_XSPI_Receive(&dev_data->hmspi, (uint8_t *)data,
				   HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}

	dev_data->cmd_status = 0;

	return 0;
}

/*
 * Read Serial Flash Discovery Parameter :
 * perform a read access over SPI bus for SDFP (DataMode is already set)
 */
static int mspi_read_sfdp(const struct device *dev, off_t addr, void *data,
			  size_t size)
{
	LOG_INF("Read SFDP from externalFlash");
	/* Get the SFDP from the external Flash (no sfdp-bfp table in the DeviceTree) */
	if (mspi_stm32_read_sfdp(dev, addr, data, size) == 0) {
		/* If valid, then ignore any table from the DTS */
		return 0;
	}
	LOG_INF("Error reading SFDP from external Flash and none in the DTS");
	return -EINVAL;
}
#endif

static void mspi_stm32_isr(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	HAL_XSPI_IRQHandler(&dev_data->hmspi);
}

#if !defined(CONFIG_SOC_SERIES_STM32H7X)
/* weak function required for HAL compilation */
__weak HAL_StatusTypeDef HAL_DMA_Abort_IT(DMA_HandleTypeDef *hdma)
{
	return HAL_OK;
}

/* weak function required for HAL compilation */
__weak HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef *hdma)
{
	return HAL_OK;
}
#endif /* !CONFIG_SOC_SERIES_STM32H7X */

/* This function is executed in the interrupt context */
#if MSPI_STM32_USE_DMA
static void mspi_dma_callback(const struct device *dev, void *arg,
			 uint32_t channel, int status)
{
	DMA_HandleTypeDef *hdma = arg;

	ARG_UNUSED(dev);

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);
	}

	HAL_DMA_IRQHandler(hdma);
}
#endif


/*
 * Transfer Error callback.
 */
void HAL_XSPI_ErrorCallback(XSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data =
		CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Error cb");

	dev_data->cmd_status = -EIO;

	k_sem_give(&dev_data->sync);
}

/*
 * Command completed callback.
 */
void HAL_XSPI_CmdCpltCallback(XSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data =
		CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Cmd Cplt cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Rx Transfer completed callback.
 */
void HAL_XSPI_RxCpltCallback(XSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data =
		CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Rx Cplt cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Tx Transfer completed callback.
 */
void HAL_XSPI_TxCpltCallback(XSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data =
		CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Tx Cplt cb");

	dev_data->ctx.packets_done++;

	k_sem_give(&dev_data->sync);
}

/*
 * Status Match callback.
 */
void HAL_XSPI_StatusMatchCallback(XSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data =
		CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Status Match cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Timeout callback.
 */
void HAL_XSPI_TimeOutCallback(XSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data =
		CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Timeout cb");

	dev_data->cmd_status = -EIO;

	k_sem_give(&dev_data->sync);
}

#if 0
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void mspi_stm32_pages_layout(const struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	struct mspi_stm32_data *dev_data = dev->data;

	*layout = &dev_data->layout;
	*layout_size = 1;
}
#endif

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static int setup_pages_layout(const struct device *dev)
{
	const struct mspi_stm32_conf *dev_cfg = dev->config;
	struct mspi_stm32_data *data = dev->data;
	const size_t flash_size = dev_cfg->flash_size;
	uint32_t layout_page_size = data->page_size;
	uint8_t value = 0;
	int rv = 0;

	/* Find the smallest erase size. */
	for (size_t i = 0; i < ARRAY_SIZE(data->erase_types); ++i) {
		const struct jesd216_erase_type *etp = &data->erase_types[i];

		if ((etp->cmd != 0)
		    && ((value == 0) || (etp->exp < value))) {
			value = etp->exp;
		}
	}

	uint32_t erase_size = BIT(value);

	if (erase_size == 0) {
		erase_size = SPI_NOR_SECTOR_SIZE;
	}

	/* We need layout page size to be compatible with erase size */
	if ((layout_page_size % erase_size) != 0) {
		LOG_DBG("layout page %u not compatible with erase size %u",
			layout_page_size, erase_size);
		LOG_DBG("erase size will be used as layout page size");
		layout_page_size = erase_size;
	}

	/* Warn but accept layout page sizes that leave inaccessible
	 * space.
	 */
	if ((flash_size % layout_page_size) != 0) {
		LOG_DBG("layout page %u wastes space with device size %zu",
			layout_page_size, flash_size);
	}

	data->layout.pages_size = layout_page_size;
	data->layout.pages_count = flash_size / layout_page_size;
	LOG_DBG("layout %u x %u By pages", data->layout.pages_count,
					   data->layout.pages_size);

	return rv;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
#endif

#if 0
static int mspi_stm32_read_status_register(const struct device *dev, uint8_t reg_num, uint8_t *reg)
{
	XSPI_RegularCmdTypeDef s_command = {
		.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE,
		.DataMode = HAL_XSPI_DATA_1_LINE,
	};

	switch (reg_num) {
	case 1U:
		s_command.Instruction = SPI_NOR_CMD_RDSR;
		break;
	case 2U:
		s_command.Instruction = SPI_NOR_CMD_RDSR2;
		break;
	case 3U:
		s_command.Instruction = SPI_NOR_CMD_RDSR3;
		break;
	default:
		return -EINVAL;
	}

	return mspi_read_access(dev, &s_command, reg, sizeof(*reg));
}

static int mspi_stm32_write_status_register(const struct device *dev, uint8_t reg_num, uint8_t reg)
{
	struct mspi_stm32_data *data = dev->data;
	XSPI_RegularCmdTypeDef s_command = {
		.Instruction = SPI_NOR_CMD_WRSR,
		.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE,
		.DataMode = HAL_XSPI_DATA_1_LINE
	};
	size_t size;
	uint8_t regs[4] = { 0 };
	uint8_t *regs_p;
	int ret;

	if (reg_num == 1U) {
		size = 1U;
		regs[0] = reg;
		regs_p = &regs[0];
		/* 1 byte write clears SR2, write SR2 as well */
		if (data->qer_type == JESD216_DW15_QER_S2B1v1) {
			ret = mspi_stm32_read_status_register(dev, 2, &regs[1]);
			if (ret < 0) {
				return ret;
			}
			size = 2U;
		}
	} else if (reg_num == 2U) {
		s_command.Instruction = SPI_NOR_CMD_WRSR2;
		size = 1U;
		regs[1] = reg;
		regs_p = &regs[1];
		/* if SR2 write needs SR1 */
		if ((data->qer_type == JESD216_DW15_QER_VAL_S2B1v1) ||
		    (data->qer_type == JESD216_DW15_QER_VAL_S2B1v4) ||
		    (data->qer_type == JESD216_DW15_QER_VAL_S2B1v5)) {
			ret = mspi_stm32_read_status_register(dev, 1, &regs[0]);
			if (ret < 0) {
				return ret;
			}
			s_command.Instruction = SPI_NOR_CMD_WRSR;
			size = 2U;
			regs_p = &regs[0];
		}
	} else if (reg_num == 3U) {
		s_command.Instruction = SPI_NOR_CMD_WRSR3;
		size = 1U;
		regs[2] = reg;
		regs_p = &regs[2];
	} else {
		return -EINVAL;
	}

	return mspi_write_access(dev, &s_command, regs_p, size);
}

static int mspi_stm32_enable_qe(const struct device *dev)
{
	struct mspi_stm32_data *data = dev->data;
	uint8_t qe_reg_num;
	uint8_t qe_bit;
	uint8_t reg;
	int ret;

	switch (data->qer_type) {
	case JESD216_DW15_QER_NONE:
		/* no QE bit, device detects reads based on opcode */
		return 0;
	case JESD216_DW15_QER_S1B6:
		qe_reg_num = 1U;
		qe_bit = BIT(6U);
		break;
	case JESD216_DW15_QER_S2B7:
		qe_reg_num = 2U;
		qe_bit = BIT(7U);
		break;
	case JESD216_DW15_QER_S2B1v1:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v4:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v5:
		__fallthrough;
	case JESD216_DW15_QER_S2B1v6:
		qe_reg_num = 2U;
		qe_bit = BIT(1U);
		break;
	default:
		return -ENOTSUP;
	}

	ret = mspi_stm32_read_status_register(dev, qe_reg_num, &reg);
	if (ret < 0) {
		return ret;
	}

	/* exit early if QE bit is already set */
	if ((reg & qe_bit) != 0U) {
		return 0;
	}

	ret = mspi_stm32_write_enable(dev, MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE);
	if (ret < 0) {
		return ret;
	}

	reg |= qe_bit;

	ret = mspi_stm32_write_status_register(dev, qe_reg_num, reg);
	if (ret < 0) {
		return ret;
	}

	ret = mspi_stm32_mem_ready(dev, MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE);
	if (ret < 0) {
		return ret;
	}

	/* validate that QE bit is set */
	ret = mspi_stm32_read_status_register(dev, qe_reg_num, &reg);
	if (ret < 0) {
		return ret;
	}

	if ((reg & qe_bit) == 0U) {
		LOG_ERR("Status Register %u [0x%02x] not set", qe_reg_num, reg);
		ret = -EIO;
	}

	return ret;
}

static void spi_nor_process_bfp_addrbytes(const struct device *dev,
					  const uint8_t jesd216_bfp_addrbytes)
{
	struct mspi_stm32_data *data = dev->data;

	if ((jesd216_bfp_addrbytes == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B) ||
	    (jesd216_bfp_addrbytes == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B4B)) {
		data->cfg.addr_length = 4U;
	} else {
		data->cfg.addr_length = 3U;
	}
}
#endi
static inline uint8_t spi_nor_convert_read_to_4b(const uint8_t opcode)
{
	switch (opcode) {
	case SPI_NOR_CMD_READ:
		return SPI_NOR_CMD_READ_4B;
	case SPI_NOR_CMD_DREAD:
		return SPI_NOR_CMD_DREAD_4B;
	case SPI_NOR_CMD_2READ:
		return SPI_NOR_CMD_2READ_4B;
	case SPI_NOR_CMD_QREAD:
		return SPI_NOR_CMD_QREAD_4B;
	case SPI_NOR_CMD_4READ:
		return SPI_NOR_CMD_4READ_4B;
	default:
		/* use provided */
		return opcode;
	}
}

static inline uint8_t spi_nor_convert_write_to_4b(const uint8_t opcode)
{
	switch (opcode) {
	case SPI_NOR_CMD_PP:
		return SPI_NOR_CMD_PP_4B;
	case SPI_NOR_CMD_PP_1_1_4:
		return SPI_NOR_CMD_PP_1_1_4_4B;
	case SPI_NOR_CMD_PP_1_4_4:
		return SPI_NOR_CMD_PP_1_4_4_4B;
	default:
		/* use provided */
		return opcode;
	}
}

static int spi_nor_process_bfp(const struct device *dev,
			       const struct jesd216_param_header *php,
			       const struct jesd216_bfp *bfp)
{
	const struct mspi_stm32_conf *dev_cfg = dev->config;
	struct mspi_stm32_data *data = dev->data;
	/* must be kept in data mode order, ignore 1-1-1 (always supported) */
	const enum jesd216_mode_type supported_read_modes[] = { JESD216_MODE_112, JESD216_MODE_122,
								JESD216_MODE_114,
								JESD216_MODE_144 };
	size_t supported_read_modes_max_idx;
	struct jesd216_erase_type *etp = data->erase_types;
	size_t idx;
	const size_t flash_size = jesd216_bfp_density(bfp) / 8U;
	struct jesd216_instr read_instr = { 0 };
	struct jesd216_bfp_dw15 dw15;

	if (flash_size != dev_cfg->flash_size) {
		LOG_DBG("Unexpected flash size: %u", flash_size);
	}

	LOG_DBG("%s: %u MiBy flash", dev->name, (uint32_t)(flash_size >> 20));

	/* Copy over the erase types, preserving their order.  (The
	 * Sector Map Parameter table references them by index.)
	 */
	memset(data->erase_types, 0, sizeof(data->erase_types));
	for (idx = 1U; idx <= ARRAY_SIZE(data->erase_types); ++idx) {
		if (jesd216_bfp_erase(bfp, idx, etp) == 0) {
			LOG_DBG("Erase %u with %02x",
					(uint32_t)BIT(etp->exp), etp->cmd);
		}
		++etp;
	}

	spi_nor_process_bfp_addrbytes(dev, jesd216_bfp_addrbytes(bfp));
	LOG_DBG("Address width: %u Bytes", data->cfg.addr_length);

	/* use PP opcode based on configured data mode if nothing is set in DTS */
	if (data->cfg.write_cmd == SPI_NOR_WRITEOC_NONE) {
		switch (data->dev_cfg.io_mode) {
		case MSPI_IO_MODE_OCTAL:
			data->cfg.write_cmd = SPI_NOR_OCMD_PAGE_PRG;
			break;
		case MSPI_IO_MODE_QUAD:
			data->cfg.write_cmd = SPI_NOR_CMD_PP_1_4_4;
			break;
		case MSPI_IO_MODE_DUAL:
			data->cfg.write_cmd = SPI_NOR_CMD_PP_1_1_2;
			break;
		default:
			data->cfg.write_cmd = SPI_NOR_CMD_PP;
			break;
		}
	}

	if (data->dev_cfg.io_mode != MSPI_IO_MODE_OCTAL) {
		/* determine supported read modes, begin from the slowest */
		data->read_mode = JESD216_MODE_111;
		data->cfg.read_cmd = SPI_NOR_CMD_READ;
		data->cfg.rx_dummy = 0U;

		if (data->dev_cfg.io_mode != MSPI_IO_MODE_SINGLE) {
			if (data->dev_cfg.io_mode == MSPI_IO_MODE_DUAL) {
				/* the index of JESD216_MODE_114 in supported_read_modes */
				supported_read_modes_max_idx = 2U;
			} else {
				supported_read_modes_max_idx = ARRAY_SIZE(supported_read_modes);
			}

			for (idx = 0U; idx < supported_read_modes_max_idx; ++idx) {
				if (jesd216_bfp_read_support(php, bfp, supported_read_modes[idx],
							     &read_instr) < 0) {
					/* not supported */
					continue;
				}

				LOG_DBG("Supports read mode: %d, instr: 0x%X",
					supported_read_modes[idx], read_instr.instr);
				data->read_mode = supported_read_modes[idx];
				data->cfg.read_cmd = read_instr.instr;
				data->cfg.rx_dummy =
					(read_instr.wait_states + read_instr.mode_clocks);
			}
		}

		/* convert 3-Byte opcodes to 4-Byte (if required) */
		if (IS_ENABLED(DT_INST_PROP(0, four_byte_opcodes))) {
			if (data->cfg.addr_length != 4U) {
				LOG_DBG("4-Byte opcodes require 4-Byte address width");
				return -ENOTSUP;
			}
			data->cfg.read_cmd = spi_nor_convert_read_to_4b(data->cfg.read_cmd);
			data->cfg.write_cmd = spi_nor_convert_write_to_4b(data->cfg.write_cmd);
		}

		/* enable quad mode (if required) */
		if (data->dev_cfg.io_mode == MSPI_IO_MODE_QUAD) {
			if (jesd216_bfp_decode_dw15(php, bfp, &dw15) < 0) {
				/* will use QER from DTS or default (refer to device data) */
				LOG_WRN("Unable to decode QE requirement [DW15]");
			} else {
				/* bypass DTS QER value */
				data->qer_type = dw15.qer;
			}

			LOG_DBG("QE requirement mode: %x", data->qer_type);

			if (mspi_stm32_enable_qe(dev) < 0) {
				LOG_ERR("Failed to enable QUAD mode");
				return -EIO;
			}

			LOG_DBG("QUAD mode enabled");
		}
	}

	data->page_size = jesd216_bfp_page_size(php, bfp);

	LOG_DBG("Page size %u bytes", data->page_size);
	LOG_DBG("Flash size %zu bytes", flash_size);
	LOG_DBG("Using read mode: %d, instr: 0x%X, dummy cycles: %u",
		data->read_mode, data->cfg.read_cmd, data->cfg.rx_dummy);
	LOG_DBG("Using write instr: 0x%X", data->cfg.write_cmd);

	return 0;
}
#endif

#if MSPI_STM32_USE_DMA
static int mspi_stm32_dma_init(DMA_HandleTypeDef *hdma, struct stream *dma_stream)
{
	int ret;
	/*
	 * DMA configuration
	 * Due to use of XSPI HAL API in current driver,
	 * both HAL and Zephyr DMA drivers should be configured.
	 * The required configuration for Zephyr DMA driver should only provide
	 * the minimum information to inform the DMA slot will be in used and
	 * how to route callbacks.
	 */

	if (!device_is_ready(dma_stream->dev)) {
		LOG_ERR("DMA %s device not ready", dma_stream->dev->name);
		return -ENODEV;
	}
	/* Proceed to the minimum Zephyr DMA driver init of the channel */
	dma_stream->cfg.user_data = hdma;
	/* HACK: This field is used to inform driver that it is overridden */
	dma_stream->cfg.linked_channel = STM32_DMA_HAL_OVERRIDE;
	/* Because of the STREAM OFFSET, the DMA channel given here is from 1 - 8 */
	ret = dma_config(dma_stream->dev,
			 (dma_stream->channel + STM32_DMA_STREAM_OFFSET), &dma_stream->cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d",
			dma_stream->channel + STM32_DMA_STREAM_OFFSET);
		return ret;
	}

	/* Proceed to the HAL DMA driver init */
	if (dma_stream->cfg.source_data_size != dma_stream->cfg.dest_data_size) {
		LOG_ERR("DMA Source and destination data sizes not aligned");
		return -EINVAL;
	}

	hdma->Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD; /* Fixed value */
	hdma->Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD; /* Fixed value */
	hdma->Init.SrcInc = (dma_stream->src_addr_increment)
		? DMA_SINC_INCREMENTED
		: DMA_SINC_FIXED;
	hdma->Init.DestInc = (dma_stream->dst_addr_increment)
		? DMA_DINC_INCREMENTED
		: DMA_DINC_FIXED;
	hdma->Init.SrcBurstLength = 4;
	hdma->Init.DestBurstLength = 4;
	hdma->Init.Priority = table_priority[dma_stream->cfg.channel_priority];
	hdma->Init.Direction = table_direction[dma_stream->cfg.channel_direction];
	hdma->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_SRC_ALLOCATED_PORT1;
	hdma->Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
	hdma->Init.Mode = DMA_NORMAL;
	hdma->Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
	hdma->Init.Request = dma_stream->cfg.dma_slot;

	/*
	 * HAL expects a valid DMA channel (not DMAMUX).
	 * The channel is from 0 to 7 because of the STM32_DMA_STREAM_OFFSET
	 * in the dma_stm32 driver
	 */
	hdma->Instance = LL_DMA_GET_CHANNEL_INSTANCE(dma_stream->reg,
						    dma_stream->channel);

	/* Initialize DMA HAL */
	if (HAL_DMA_Init(hdma) != HAL_OK) {
		LOG_ERR("XSPI DMA Init failed");
		return -EIO;
	}

	if (HAL_DMA_ConfigChannelAttributes(hdma, DMA_CHANNEL_NPRIV) != HAL_OK) {
		LOG_ERR("XSPI DMA Init failed");
		return -EIO;
	}

	LOG_DBG("XSPI with DMA transfer");
	return 0;
}
#endif /* MSPI_STM32_USE_DMA */

/**
 * Check and save dev_cfg to controller data->dev_cfg.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param param_mask Macro definition of what to be configured in cfg.
 * @param dev_cfg The device runtime configuration for the MSPI controller.
 * @return 0 MSPI device configuration successful.
 * @return -Error MSPI device configuration fail.
 */
static inline int mspi_dev_cfg_check_save(const struct device *controller,
					  const enum mspi_dev_cfg_mask param_mask,
					  const struct mspi_dev_cfg *dev_cfg)
{
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;

	if (param_mask & MSPI_DEVICE_CONFIG_CE_NUM) {
		data->dev_cfg.ce_num = dev_cfg->ce_num;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		if (dev_cfg->freq > MSPI_MAX_FREQ) {
			LOG_ERR("%u, freq is too large.", __LINE__);
			return -ENOTSUP;
		}
		data->dev_cfg.freq = dev_cfg->freq;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		if (dev_cfg->io_mode >= MSPI_IO_MODE_MAX) {
			LOG_ERR("%u, Invalid io_mode.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.io_mode = dev_cfg->io_mode;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		if (dev_cfg->data_rate >= MSPI_DATA_RATE_MAX) {
			LOG_ERR("%u, Invalid data_rate.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.data_rate = dev_cfg->data_rate;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		if (dev_cfg->cpp > MSPI_CPP_MODE_3) {
			LOG_ERR("%u, Invalid cpp.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.cpp = dev_cfg->cpp;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_ENDIAN) {
		if (dev_cfg->endian > MSPI_XFER_BIG_ENDIAN) {
			LOG_ERR("%u, Invalid endian.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.endian = dev_cfg->endian;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CE_POL) {
		if (dev_cfg->ce_polarity > MSPI_CE_ACTIVE_HIGH) {
			LOG_ERR("%u, Invalid ce_polarity.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.ce_polarity = dev_cfg->ce_polarity;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		if (dev_cfg->dqs_enable && !cfg->mspicfg.dqs_support) {
			LOG_ERR("%u, DQS mode not supported.", __LINE__);
			return -ENOTSUP;
		}
		data->dev_cfg.dqs_enable = dev_cfg->dqs_enable;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) {
		data->dev_cfg.rx_dummy = dev_cfg->rx_dummy;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) {
		data->dev_cfg.tx_dummy = dev_cfg->tx_dummy;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_READ_CMD) {
		data->dev_cfg.read_cmd = dev_cfg->read_cmd;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_WRITE_CMD) {
		data->dev_cfg.write_cmd = dev_cfg->write_cmd;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) {
		data->dev_cfg.cmd_length = dev_cfg->cmd_length;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) {
		data->dev_cfg.addr_length = dev_cfg->addr_length;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) {
		data->dev_cfg.mem_boundary = dev_cfg->mem_boundary;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) {
		data->dev_cfg.time_to_break = dev_cfg->time_to_break;
	}

	return 0;
}

/**
 * Verify if the device with dev_id is on this MSPI bus.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @return 0 The device is on this MSPI bus.
 * @return -ENODEV The device is not on this MSPI bus.
 */
static inline int mspi_verify_device(const struct device *controller,
				     const struct mspi_dev_id *dev_id)
{
	const struct mspi_stm32_conf *cfg = controller->config;
	int device_index = cfg->mspicfg.num_periph;
	int ret = 0;

	if (cfg->mspicfg.num_ce_gpios != 0) {
		for (int i = 0; i < cfg->mspicfg.num_periph; i++) {
			if (dev_id->ce.port == cfg->mspicfg.ce_group[i].port &&
			    dev_id->ce.pin == cfg->mspicfg.ce_group[i].pin &&
			    dev_id->ce.dt_flags == cfg->mspicfg.ce_group[i].dt_flags) {
				device_index = i;
			}
		}

		if (device_index >= cfg->mspicfg.num_periph ||
		    device_index != dev_id->dev_idx) {
			LOG_ERR("%u, invalid device ID.", __LINE__);
			return -ENODEV;
		}
	} else {
		if (dev_id->dev_idx >= cfg->mspicfg.num_periph) {
			LOG_ERR("%u, invalid device ID.", __LINE__);
			return -ENODEV;
		}
	}

	return ret;
}

/**
 * API implementation of mspi_dev_config : controller device specific configuration
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param param_mask Macro definition of what to be configured in cfg.
 * @param dev_cfg The device runtime configuration for the MSPI controller.
 *
 * @retval 0 if successful.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by MSPI peripheral.
 */
static int mspi_stm32_dev_config(const struct device *controller,
				const struct mspi_dev_id *dev_id,
				const enum mspi_dev_cfg_mask param_mask,
				const struct mspi_dev_cfg *dev_cfg)
{
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;
	int ret = 0;

	if (data->dev_id != dev_id) {
		if (k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE))) {
			LOG_ERR("MSPI config failed to access controller.");
			return -EBUSY;
		}

		ret = mspi_verify_device(controller, dev_id);
		if (ret) {
			goto e_return;
		}
	}

	if (mspi_is_inp(controller)) {
		ret = -EBUSY;
		goto e_return;
	}

	if (param_mask == MSPI_DEVICE_CONFIG_NONE &&
	    !cfg->mspicfg.sw_multi_periph) {
		/* Do nothing except obtaining the controller lock */
		data->dev_id = (struct mspi_dev_id *)dev_id;
		return ret;

	} else if (param_mask != MSPI_DEVICE_CONFIG_ALL) {
		if (data->dev_id != dev_id) {
			/* MSPI_DEVICE_CONFIG_ALL should be used */
			LOG_ERR("MSPI config failed : must be the same device.");
			ret = -ENOTSUP;
			goto e_return;
		}
		ret = mspi_dev_cfg_check_save(controller, param_mask, dev_cfg);
		if (ret) {
			goto e_return;
		}
	} else if (param_mask == MSPI_DEVICE_CONFIG_ALL) {
		ret = mspi_dev_cfg_check_save(controller, param_mask, dev_cfg);
		if (ret) {
			goto e_return;
		}
		if (data->dev_id != dev_id) {
			/* Conduct device switching */
		}
	} else {
		LOG_ERR("MSPI config failed :  Invalid param_mask.");
		ret = -EINVAL;
		goto e_return;
	}

	data->dev_id = dev_id;

e_return:
	k_mutex_unlock(&data->lock);

	return ret;
}

/* Set the device back in command mode */
static int mspi_stm32_memmap_off(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	if (HAL_XSPI_Abort(&dev_data->hmspi) != HAL_OK) {
			LOG_ERR("MemMapped abort failed");
			return -EIO;
		}
	return 0;
}

/* Set the device in MemMapped mode */
static int mspi_stm32_memmap_on(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;
	XSPI_RegularCmdTypeDef s_command;
	XSPI_MemoryMappedTypeDef s_MemMappedCfg;
	int ret;

	/* Configure in MemoryMapped mode */
	if ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE) &&
		(mspi_stm32_hal_address_size(controller) == HAL_XSPI_ADDRESS_24_BITS)) {
		/* OPI mode and 3-bytes address size not supported by memory */
		LOG_ERR("MSPI_IO_MODE_SINGLE in 3Bytes addressing is not supported");
		return -EIO;
	}

	/* Initialize the read command */
	s_command.OperationType = HAL_XSPI_OPTYPE_READ_CFG;
	s_command.InstructionMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
					? HAL_XSPI_INSTRUCTION_1_LINE
					: HAL_XSPI_INSTRUCTION_8_LINES)
				: HAL_XSPI_INSTRUCTION_8_LINES;
	s_command.InstructionDTRMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? HAL_XSPI_INSTRUCTION_DTR_DISABLE
				: HAL_XSPI_INSTRUCTION_DTR_ENABLE;
	s_command.InstructionWidth = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
					? HAL_XSPI_INSTRUCTION_8_BITS
					: HAL_XSPI_INSTRUCTION_16_BITS)
				: HAL_XSPI_INSTRUCTION_16_BITS;
	s_command.Instruction = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
					? ((mspi_stm32_hal_address_size(controller) ==
					HAL_XSPI_ADDRESS_24_BITS)
						? SPI_NOR_CMD_READ_FAST
						: SPI_NOR_CMD_READ_FAST_4B)
					: dev_data->dev_cfg.read_cmd)
				: SPI_NOR_OCMD_DTR_RD;
	s_command.AddressMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
					? HAL_XSPI_ADDRESS_1_LINE
					: HAL_XSPI_ADDRESS_8_LINES)
				: HAL_XSPI_ADDRESS_8_LINES;
	s_command.AddressDTRMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? HAL_XSPI_ADDRESS_DTR_DISABLE
					: HAL_XSPI_ADDRESS_DTR_ENABLE;
	s_command.AddressWidth = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? mspi_stm32_hal_address_size(controller)
				: HAL_XSPI_ADDRESS_32_BITS;
	s_command.DataMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
					? HAL_XSPI_DATA_1_LINE
					: HAL_XSPI_DATA_8_LINES)
				: HAL_XSPI_DATA_8_LINES;
	s_command.DataDTRMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? HAL_XSPI_DATA_DTR_DISABLE
				: HAL_XSPI_DATA_DTR_ENABLE;
	s_command.DummyCycles = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
					? SPI_NOR_DUMMY_RD
					: SPI_NOR_DUMMY_RD_OCTAL)
				: SPI_NOR_DUMMY_RD_OCTAL_DTR;
	s_command.DQSMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				? HAL_XSPI_DQS_DISABLE
				: HAL_XSPI_DQS_ENABLE;
#ifdef XSPI_CCR_SIOO
	s_command.SIOOMode = HAL_XSPI_SIOO_INST_EVERY_CMD;
#endif /* XSPI_CCR_SIOO */

	ret = HAL_XSPI_Command(&dev_data->hmspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to set memory map");
		return -EIO;
	}

	/* Initialize the program command */
	s_command.OperationType = HAL_XSPI_OPTYPE_WRITE_CFG;
	if (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE) {
		s_command.Instruction = (dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
					? ((mspi_stm32_hal_address_size(controller) ==
					HAL_XSPI_ADDRESS_24_BITS)
						? SPI_NOR_CMD_PP
						: SPI_NOR_CMD_PP_4B)
				: SPI_NOR_OCMD_PAGE_PRG;
	} else {
		s_command.Instruction = SPI_NOR_OCMD_PAGE_PRG;
	}
	s_command.DQSMode = HAL_XSPI_DQS_DISABLE;
	ret = HAL_XSPI_Command(&dev_data->hmspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to set memory mapped");
		return -EIO;
	}

	/* Enable the memory-mapping */
	s_MemMappedCfg.TimeOutActivation = HAL_XSPI_TIMEOUT_COUNTER_DISABLE;
	ret = HAL_XSPI_MemoryMapped(&dev_data->hmspi, &s_MemMappedCfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to enable memory mapped");
		return -EIO;
	}

	return 0;
}

/**
 * API implementation of mspi_xip_config : XIP configuration
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param xip_cfg The controller XIP configuration for MSPI.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 */
static int mspi_stm32_xip_config(const struct device *controller,
				const struct mspi_dev_id *dev_id,
				const struct mspi_xip_cfg *xip_cfg)
{
	const struct mspi_stm32_conf *dev_cfg = controller->config;
	struct mspi_stm32_data *dev_data = controller->data;
	int ret = 0;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("dev_id don't match");
		return -ESTALE;
	}

	if (!xip_cfg->enable) {
		/* This is for aborting */
		ret = mspi_stm32_memmap_off(controller);
	} else {
		ret = mspi_stm32_memmap_on(controller);
	}

	if (ret == 0) {
		dev_data->xip_cfg = *xip_cfg;
		LOG_INF("XIP configured %d", xip_cfg->enable);
	}
	return ret;
}

/**
 * API implementation of mspi_timing_config.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param param_mask The macro definition of what should be configured in cfg.
 * @param timing_cfg The controller timing configuration for MSPI.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 * @retval -ENOTSUP param_mask value is not supported.
 */
static int mspi_stm32_timing_config(const struct device *controller,
				   const struct mspi_dev_id *dev_id,
				   const uint32_t param_mask,
				   void *timing_cfg)
{
	struct mspi_stm32_data *dev_data = controller->data;

	if (mspi_is_inp(controller)) {
		return -EBUSY;
	}

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("timing config : dev_id don't match");
		return -ESTALE;
	}

	if (param_mask == MSPI_TIMING_PARAM_DUMMY) {
		dev_data->timing_cfg = *(struct mspi_timing_cfg *)timing_cfg;
	} else {
		LOG_ERR("param_mask  %dnot supported.", param_mask);
		return -ENOTSUP;
	}

//	dev_data->hal_dev_cfg = hal_dev_cfg;
	return 0;
}

/**
 * API implementation of mspi_get_channel_status.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param ch Not used.
 *
 * @retval 0 if successful.
 * @retval -EBUSY MSPI bus is busy
 */
static int mspi_stm32_get_channel_status(const struct device *controller, uint8_t ch)
{
	struct mspi_stm32_data *dev_data = controller->data;

	ARG_UNUSED(ch);

	if (mspi_is_inp(controller)) {
		return -EBUSY;
	}

	k_mutex_unlock(&dev_data->lock);
	dev_data->dev_id = NULL;

	return 0;
}

/**
 * API implementation of mspi_register_callback.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param evt_type The event type associated the callback.
 * @param cb Pointer to the user implemented callback function.
 * @param ctx Pointer to the callback context.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 * @retval -ENOTSUP evt_type not supported.
 */
static int mspi_stm32_register_callback(const struct device *controller,
				       const struct mspi_dev_id *dev_id,
				       const enum mspi_bus_event evt_type,
				       mspi_callback_handler_t cb,
				       struct mspi_callback_context *ctx)
{
	struct mspi_stm32_data *data = controller->data;

	if (mspi_is_inp(controller)) {
		return -EBUSY;
	}

	if (dev_id != data->dev_id) {
		LOG_ERR("reg cb : dev_id don't match");
		return -ESTALE;
	}

	if (evt_type >= MSPI_BUS_EVENT_MAX) {
		LOG_ERR("callback types %d not supported.", evt_type);
		return -ENOTSUP;
	}

	data->cbs[evt_type] = cb;
	data->cb_ctxs[evt_type] = ctx;
	return 0;
}

/**
 * API implementation of mspi_scramble_config.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param scramble_cfg The controller scramble configuration for MSPI.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 */
static int mspi_stm32_scramble_config(const struct device *controller,
				     const struct mspi_dev_id *dev_id,
				     const struct mspi_scramble_cfg *scramble_cfg)
{

	struct mspi_stm32_data *data = controller->data;
	int ret = 0;

	if (mspi_is_inp(controller)) {
		return -EBUSY;
	}
	if (dev_id != data->dev_id) {
		LOG_ERR("scramble config: dev_id don't match");
		return -ESTALE;
	}

	data->scramble_cfg = *scramble_cfg;
	return ret;
}

static int mspi_stm32_pio_transceive(const struct device *controller,
			       const struct mspi_xfer *xfer,
			       mspi_callback_handler_t cb,
			       struct mspi_callback_context *cb_ctx)
{
	const struct mspi_stm32_config *dev_cfg = controller->config;
	struct mspi_stm32_data *dev_data = controller->data;
	struct mspi_context *ctx = &dev_data->ctx;
	const struct mspi_xfer_packet *packet;
	uint32_t packet_idx;
	int ret = 0;
	int cfg_flag = 0;

	if (xfer->num_packet == 0 ||
	    !xfer->packets ||
	    xfer->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		return -EFAULT;
	}

	/* DummyCycle to give to the mspi_stm32_read_access/mspi_stm32_write_access */
	cfg_flag = mspi_context_lock(ctx, dev_data->dev_id, xfer, cb, cb_ctx, true);
	/** For async, user must make sure when cfg_flag = 0 the dummy and instr addr length
	 * in mspi_xfer of the two calls are the same if the first one has not finished yet.
	 */
	if (cfg_flag) {
		if (cfg_flag != 1) {
			ret = cfg_flag;
			goto pio_err;
		}
	}

	/* PIO mode : Synchronous transfer is for command mode with Timeout */
	if (!ctx->xfer.async) {
		/* Synchronous transfer */
		while (ctx->packets_left > 0) {
			packet_idx = ctx->xfer.num_packet - ctx->packets_left;
			packet = &ctx->xfer.packets[packet_idx];
			/* Always starts with a command, then payload is given by the xfer->num_packet */
			if (packet->dir == MSPI_RX) {
				ret = mspi_stm32_read_access(controller,
					packet,
					MSPI_ACCESS_SYNC);
			} else {
				ret = mspi_stm32_write_access(controller,
					packet,
					MSPI_ACCESS_SYNC);
			}

			ctx->packets_left--;
			if (ret) {
				ret = -EIO;
				goto pio_err;
			}
		}

	} else {
		/* Asynchronous transfer call read/write with IT and callback function */
		HAL_XSPI_ENABLE_IT(&dev_data->hmspi, HAL_XSPI_IT_TC | HAL_XSPI_IT_FT | HAL_XSPI_IT_TE);

		while (ctx->packets_left > 0) {
			packet_idx = ctx->xfer.num_packet - ctx->packets_left;
			packet = &ctx->xfer.packets[packet_idx];

			if (ctx->callback && packet->cb_mask == MSPI_BUS_XFER_COMPLETE_CB) {
				ctx->callback_ctx->mspi_evt.evt_type = MSPI_BUS_XFER_COMPLETE;
				ctx->callback_ctx->mspi_evt.evt_data.controller = controller;
				ctx->callback_ctx->mspi_evt.evt_data.dev_id = dev_data->ctx.owner;
				ctx->callback_ctx->mspi_evt.evt_data.packet = packet;
				ctx->callback_ctx->mspi_evt.evt_data.packet_idx = packet_idx;
				ctx->callback_ctx->mspi_evt.evt_data.status = ~0;
			}

			mspi_callback_handler_t callback = NULL;

			if (packet->cb_mask == MSPI_BUS_XFER_COMPLETE_CB) {
				callback = (mspi_callback_handler_t)ctx->callback;
			}


			if (packet->dir == MSPI_RX) {
				ret = mspi_stm32_read_access(controller,
					packet,
					MSPI_ACCESS_ASYNC);
			} else {
				ret = mspi_stm32_write_access(controller,
					packet,
					MSPI_ACCESS_ASYNC);
			}

			ctx->packets_left--;
			if (ret) {
				goto pio_err;
			}
		}
	}

pio_err:
	mspi_context_release(ctx);
	return ret;
}

static int mspi_stm32_dma_transceive(const struct device *controller,
			       const struct mspi_xfer *xfer,
			       mspi_callback_handler_t cb,
			       struct mspi_callback_context *cb_ctx)
{
	const struct mspi_stm32_config *dev_cfg = controller->config;
	struct mspi_stm32_data *dev_data = controller->data;
	struct mspi_context *ctx = &dev_data->ctx;
	XSPI_RegularCmdTypeDef transf;
	int ret = 0;
	int cfg_flag = 0;

	if (xfer->num_packet == 0 ||
	    !xfer->packets ||
	    xfer->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		return -EFAULT;
	}

	cfg_flag = mspi_context_lock(ctx, dev_data->dev_id, xfer, cb, cb_ctx, true);
	/** For async, user must make sure when cfg_flag = 0 the dummy and instr addr length
	 * in mspi_xfer of the two calls are the same if the first one has not finished yet.
	 */
	if (cfg_flag) {
		if (cfg_flag == 1) {
			ret = mspi_xfer_config(controller, xfer);
			if (ret) {
				goto dma_err;
			}
		} else {
			ret = cfg_flag;
			goto dma_err;
		}
	}

	/* TODO: enable DMA it */
	HAL_XSPI_ENABLE_IT(&dev_data->hmspi, HAL_XSPI_IT_TC | HAL_XSPI_IT_FT | HAL_XSPI_IT_TE);

	while (ctx->packets_left > 0) {
		uint32_t packet_idx = ctx->xfer.num_packet - ctx->packets_left;
		const struct mspi_xfer_packet *packet;

		packet = &ctx->xfer.packets[packet_idx];

		if (!ctx->xfer.async) {
			/* Synchronous transfer */
			if (packet->dir == MSPI_RX) {
				ret = mspi_stm32_read_access(controller,
					packet,
					MSPI_ACCESS_DMA);
			} else {
				ret = mspi_stm32_write_access(controller,
					packet,
					MSPI_ACCESS_DMA);
			}
		} else {
			/* Asynchronous transfer DMA irq  with cb */
			/* TODO what is the async DMA : NOT SUPPORTED */
			goto dma_err;

			if (ctx->callback && packet->cb_mask == MSPI_BUS_XFER_COMPLETE_CB) {
				ctx->callback_ctx->mspi_evt.evt_type = MSPI_BUS_XFER_COMPLETE;
				ctx->callback_ctx->mspi_evt.evt_data.controller = controller;
				ctx->callback_ctx->mspi_evt.evt_data.dev_id = dev_data->ctx.owner;
				ctx->callback_ctx->mspi_evt.evt_data.packet = packet;
				ctx->callback_ctx->mspi_evt.evt_data.packet_idx = packet_idx;
				ctx->callback_ctx->mspi_evt.evt_data.status = ~0;
			}

			mspi_callback_handler_t callback = NULL;

			if (packet->cb_mask == MSPI_BUS_XFER_COMPLETE_CB) {
				callback = (mspi_callback_handler_t)ctx->callback;
			}

			if (packet->dir == MSPI_RX) {
				ret = mspi_stm32_read_access(controller,
					packet,
					MSPI_ACCESS_DMA);
			} else {
				ret = mspi_stm32_write_access(controller,
					packet,
					MSPI_ACCESS_DMA);
			}
		}
		ctx->packets_left--;
		if (ret) {
			goto dma_err;
		}
	}

	if (!ctx->xfer.async) {
		while (ctx->packets_done < ctx->xfer.num_packet) {
			k_busy_wait(10);
		}
	}

dma_err:
	mspi_context_release(ctx);
	return ret;
}

/**
 * API implementation of mspi_transceive.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param xfer Pointer to the MSPI transfer started by dev_id.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 * @retval -Error transfer failed.
 */
static int mspi_stm32_transceive(const struct device *controller,
				const struct mspi_dev_id *dev_id,
				const struct mspi_xfer *xfer)
{
	const struct mspi_stm32_config *dev_cfg = controller->config;
	struct mspi_stm32_data *dev_data = controller->data;
	mspi_callback_handler_t cb = NULL;
	struct mspi_callback_context *cb_ctx = NULL;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("transceive : dev_id don't match");
		return -ESTALE;
	}

	/*
	 * async + MSPI_PIO : Use callback on Irq if PIO
	 * sync + MSPI_PIO use timeout (mainly for NOR command and param
	 * MSPI_DMA : async/sync is meaningless with DMA (no DMA IT function)t
	 */
	if (xfer->xfer_mode == MSPI_PIO) {
		if (xfer->async) {
			cb = dev_data->cbs[MSPI_BUS_XFER_COMPLETE];
			cb_ctx = dev_data->cb_ctxs[MSPI_BUS_XFER_COMPLETE];
		}
		return mspi_stm32_pio_transceive(controller, xfer, cb, cb_ctx);
	} else if (xfer->xfer_mode == MSPI_DMA) {
		/*  Do not care about xfer->async */
		return mspi_stm32_dma_transceive(controller, xfer, cb, cb_ctx);

	} else {
		return -EIO;
	}
}

/**
 * API implementation of mspi_config : controller configuration.
 *
 * @param spec Pointer to MSPI device tree spec.
 * @return 0 if successful.
 * @return -Error if fail.
 */
static int mspi_stm32_config(const struct mspi_dt_spec *spec)
{
	const struct mspi_cfg *config = &spec->config;
	const struct mspi_stm32_conf *dev_cfg = spec->bus->config;
	struct mspi_stm32_data *dev_data = spec->bus->data;

	uint32_t ahb_clock_freq;
	uint32_t prescaler = MSPI_STM32_CLOCK_PRESCALER_MIN;
	int ret = 0;

	/* Only Controller mode is suppported */
	if (config->op_mode != MSPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only support MSPI controller mode.");
		return -ENOTSUP;
	}

	/* Check the max possible freq. */
	if (config->max_freq > MSPI_STM32_MAX_FREQ) {
		LOG_ERR("Max_freq %d too large.", config->max_freq);
		return -ENOTSUP;
	}

	if (config->duplex != MSPI_HALF_DUPLEX) {
		LOG_ERR("Only support half duplex mode.");
		return -ENOTSUP;
	}

	if (config->num_periph > MSPI_MAX_DEVICE) {
		LOG_ERR("Invalid MSPI peripheral number.");
		return -ENOTSUP;
	}

	/* Signals configuration */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("MSPI pinctrl setup failed");
		return ret;
	}

	if (dev_data->dev_cfg.dqs_enable && !dev_cfg->mspicfg.dqs_support) {
		LOG_ERR("MSPI dqs mismatch (not supported but enabled)");
		return -ENOTSUP;
	}

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (dev_cfg->pclk_len > 3) {
		/* Max 3 domain clock are expected */
		LOG_ERR("Could not select %d XSPI domain clock", dev_cfg->pclk_len);
		return -EIO;
	}

	/* Clock configuration */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t) &dev_cfg->pclken[0]) != 0) {
		LOG_ERR("Could not enable MSPI clock");
		return -EIO;
	}
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &dev_cfg->pclken[0],
					&ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken)");
		return -EIO;
	}
	/* Alternate clock config for peripheral if any */
	if (IS_ENABLED(MSPI_STM32_DOMAIN_CLOCK_SUPPORT) && (dev_cfg->pclk_len > 1)) {
		if (clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &dev_cfg->pclken[1],
				NULL) != 0) {
			LOG_ERR("Could not select MSPI domain clock");
			return -EIO;
		}
		/*
		 * Get the clock rate from this one (update ahb_clock_freq)
		 * TODO: retrieve index in the clocks property where clocks has "mspi-ker"
		 * Assuming index is 1
		 */
		if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &dev_cfg->pclken[1],
				&ahb_clock_freq) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclken)");
			return -EIO;
		}
	}
	/* Clock domain corresponding to the IO-Mgr (XSPIM) */
	if (IS_ENABLED(MSPI_STM32_DOMAIN_CLOCK_SUPPORT) && (dev_cfg->pclk_len > 2)) {
		if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &dev_cfg->pclken[2]) != 0) {
			LOG_ERR("Could not enable XSPI Manager clock");
			return -EIO;
		}
		/* Do NOT Get the clock rate from this one */
	}

	for (; prescaler <= MSPI_STM32_CLOCK_PRESCALER_MAX; prescaler++) {
		 dev_data->dev_cfg.freq = MSPI_STM32_CLOCK_COMPUTE(ahb_clock_freq, prescaler);

		if (dev_data->dev_cfg.freq <= dev_cfg->mspicfg.max_freq) {
			break;
		}
	}
	__ASSERT_NO_MSG(prescaler >= MSPI_STM32_CLOCK_PRESCALER_MIN &&
			prescaler <= MSPI_STM32_CLOCK_PRESCALER_MAX);

	/* Initialize XSPI HAL structure completely */
	dev_data->hmspi.Init.ClockPrescaler = prescaler;
	/* The stm32 hal_mspi driver does not reduce DEVSIZE before writing the DCR1 */
	dev_data->hmspi.Init.MemorySize = find_lsb_set(dev_cfg->reg_size) - 2;
#if defined(XSPI_DCR2_WRAPSIZE)
	dev_data->hmspi.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
#endif /* XSPI_DCR2_WRAPSIZE */
	/* STR mode else Macronix for DTR mode */
	if (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_DUAL) {
		dev_data->hmspi.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
		dev_data->hmspi.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
	} else {

	}
#if MSPI_STM32_DLYB_BYPASSED
	dev_data->hmspi.Init.DelayBlockBypass = HAL_XSPI_DELAY_BLOCK_BYPASS;
#else
	dev_data->hmspi.Init.DelayBlockBypass = HAL_XSPI_DELAY_BLOCK_ON;
#endif /* MSPI_STM32_DLYB_BYPASSED */


	if (HAL_XSPI_Init(&dev_data->hmspi) != HAL_OK) {
		LOG_ERR("MSPI Init failed");
		return -EIO;
	}

	LOG_DBG("MSPI Init'd");

#if defined(HAL_XSPIM_IOPORT_1) || defined(HAL_XSPIM_IOPORT_2)
	/* XSPI I/O manager init Function */
	XSPIM_CfgTypeDef mspi_mgr_cfg;

	if (dev_data->hmspi.Instance == XSPI1) {
		mspi_mgr_cfg.IOPort = HAL_XSPIM_IOPORT_1;
	} else if (dev_data->hmspi.Instance == XSPI2) {
		mspi_mgr_cfg.IOPort = HAL_XSPIM_IOPORT_2;
	}
	mspi_mgr_cfg.nCSOverride = HAL_XSPI_CSSEL_OVR_DISABLED;
	mspi_mgr_cfg.Req2AckTime = 1;

	if (HAL_XSPIM_Config(&dev_data->hmspi, &mspi_mgr_cfg,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI M config failed");
		return -EIO;
	}

#endif /* XSPIM */

#if defined(DLYB_XSPI1) || defined(DLYB_XSPI2) || defined(DLYB_OCTOSPI1) || defined(DLYB_OCTOSPI2)
	/* XSPI delay block init Function */
	HAL_XSPI_DLYB_CfgTypeDef mspi_delay_block_cfg = {0};

	(void)HAL_XSPI_DLYB_GetClockPeriod(&dev_data->hmspi, &mspi_delay_block_cfg);
	/*  with DTR, set the PhaseSel/4 (empiric value from stm32Cube) */
	mspi_delay_block_cfg.PhaseSel /= 4;

	if (HAL_XSPI_DLYB_SetConfig(&dev_data->hmspi, &mspi_delay_block_cfg) != HAL_OK) {
		LOG_ERR("XSPI DelayBlock failed");
		return -EIO;
	}

	LOG_DBG("Delay Block Init");

#endif /* DLYB_ */

#if MSPI_STM32_USE_DMA
	/* Configure and enable the DMA channels after XSPI config */
	static DMA_HandleTypeDef hdma_tx;
	static DMA_HandleTypeDef hdma_rx;

	if (mspi_stm32_dma_init(&hdma_tx, &dev_data->dma_tx) != 0) {
		LOG_ERR("MSPI DMA Tx init failed");
		return -EIO;
	}

	/* The dma_tx handle is hold by the dma_stream.cfg.user_data */
	__HAL_LINKDMA(&dev_data->hmspi, hdmatx, hdma_tx);

	if (mspi_stm32_dma_init(&hdma_rx, &dev_data->dma_rx) != 0) {
		LOG_ERR("MSPI DMA Rx init failed");
		return -EIO;
	}

	/* The dma_rx handle is hold by the dma_stream.cfg.user_data */
	__HAL_LINKDMA(&dev_data->hmspi, hdmarx, hdma_rx);

#endif /* CONFIG_USE_STM32_HAL_DMA */
	/* Initialize semaphores */
	k_sem_init(&dev_data->sem, 1, 1);
	k_sem_init(&dev_data->sync, 0, 1);

	/* Run IRQ init */
	dev_cfg->irq_config();

	if (!k_sem_count_get(&dev_data->ctx.lock)) {
		dev_data->ctx.owner = NULL;
		k_sem_give(&dev_data->ctx.lock);
	}

	if (config->re_init) {
		k_mutex_unlock(&dev_data->lock);
	}

	return 0;
}

/**
 * Set up a new controller and add its child to the list.
 *
 * @param dev MSPI emulation controller.
 *
 * @retval 0 if successful.
 */
static int mspi_stm32_init(const struct device *controller)
{
	const struct mspi_stm32_conf *cfg = controller->config;
	const struct mspi_dt_spec spec = {
		.bus = controller,
		.config = cfg->mspicfg,
	};

	return mspi_stm32_config(&spec);
}

static struct mspi_driver_api mspi_stm32_driver_api = {
	.config                = mspi_stm32_config,
	.dev_config            = mspi_stm32_dev_config,
	.xip_config            = mspi_stm32_xip_config,
	.scramble_config       = mspi_stm32_scramble_config,
	.timing_config         = mspi_stm32_timing_config,
	.get_channel_status    = mspi_stm32_get_channel_status,
	.register_callback     = mspi_stm32_register_callback,
	.transceive            = mspi_stm32_transceive,
};

#define MSPI_CONFIG(n)									\
	{										\
		.channel_num = 0,							\
		.op_mode = DT_ENUM_IDX_OR(n, op_mode, MSPI_OP_MODE_CONTROLLER),		\
		.duplex = DT_ENUM_IDX_OR(n, duplex, MSPI_HALF_DUPLEX),			\
		.max_freq = DT_INST_PROP_OR(n, mspi_max_frequency, MSPI_STM32_MAX_FREQ),\
		.dqs_support = DT_INST_PROP_OR(n, dqs_support, false),			\
		.num_periph  = DT_INST_CHILD_NUM(n),					\
		.sw_multi_periph = DT_INST_PROP_OR(n, software_multiperipheral, false),	\
	}


#if MSPI_STM32_USE_DMA
#define DMA_CHANNEL_CONFIG(node, dir)						\
		DT_DMAS_CELL_BY_NAME(node, dir, channel_config)

#define MSPI_DMA_CHANNEL_INIT(node, dir, dir_cap, src_dev, dest_dev)		\
	.dev = DEVICE_DT_GET(DT_DMAS_CTLR(node)),				\
	.channel = DT_DMAS_CELL_BY_NAME(node, dir, channel),			\
	.reg = (DMA_TypeDef *)DT_REG_ADDR(					\
				   DT_PHANDLE_BY_NAME(node, dmas, dir)),	\
	.cfg = {								\
		.dma_slot = DT_DMAS_CELL_BY_NAME(node, dir, slot),		\
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(		\
					DMA_CHANNEL_CONFIG(node, dir)),	\
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(			\
					DMA_CHANNEL_CONFIG(node, dir)),		\
		.dma_callback = mspi_dma_callback,				\
	},									\
	.src_addr_increment = STM32_DMA_CONFIG_##src_dev##_ADDR_INC(		\
				DMA_CHANNEL_CONFIG(node, dir)),			\
	.dst_addr_increment = STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(		\
				DMA_CHANNEL_CONFIG(node, dir)),

#define MSPI_DMA_CHANNEL(node, dir, DIR, src, dest)				\
	.dma_##dir = {								\
		COND_CODE_1(DT_DMAS_HAS_NAME(node, dir),			\
			(XSPI_DMA_CHANNEL_INIT(node, dir, DIR, src, dest)),	\
			(NULL))							\
		},
#else
#define MSPI_DMA_CHANNEL(node, dir, DIR, src, dest)
#endif /* CONFIG_USE_STM32_HAL_DMA */

#define MSPI_FLASH_MODULE(drv_id, flash_id)				\
		(DT_DRV_INST(drv_id), mspi_nor_flash_##flash_id)

#define DT_WRITEOC_PROP_OR(inst, default_value)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, writeoc),					\
		    (_CONCAT(SPI_NOR_CMD_, DT_STRING_TOKEN(DT_DRV_INST(inst), writeoc))),	\
		    ((default_value)))

#define DT_QER_PROP_OR(inst, default_value)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, quad_enable_requirements),			\
		    (_CONCAT(JESD216_DW15_QER_VAL_,						\
			     DT_STRING_TOKEN(DT_DRV_INST(inst), quad_enable_requirements))),	\
		    ((default_value)))

static void mspi_stm32_irq_config_func(void);

static const struct stm32_pclken pclken[] = STM32_DT_INST_CLOCKS(0);

PINCTRL_DT_DEFINE(DT_DRV_INST(0));

static const struct mspi_stm32_conf mspi_stm32_dev_conf = {
	.reg_base              = DT_INST_REG_ADDR(0),
	.reg_size              = DT_INST_REG_SIZE(0),
	.pclken = pclken,
	.pclk_len = DT_INST_NUM_CLOCKS(0),
	.irq_config = mspi_stm32_irq_config_func,
	.mspicfg = MSPI_CONFIG(0),
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(0)),
#if MSPI_STM32_RESET_GPIO
	.reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
#endif /* MSPI_STM32_RESET_GPIO */
};

static struct mspi_stm32_data mspi_stm32_dev_data = {
	.hmspi = {
		.Instance = (XSPI_TypeDef *)DT_INST_REG_ADDR(0),
		.Init = {
			.FifoThresholdByte = MSPI_STM32_FIFO_THRESHOLD,
			.SampleShifting = (DT_INST_PROP(0, ssht_enable)
					? HAL_XSPI_SAMPLE_SHIFT_HALFCYCLE
					: HAL_XSPI_SAMPLE_SHIFT_NONE),
			.ChipSelectHighTimeCycle = 1,
			.ClockMode = HAL_XSPI_CLOCK_MODE_0,
			.ChipSelectBoundary = 0,
			.MemoryMode = HAL_XSPI_SINGLE_MEM,
#if defined(HAL_XSPIM_IOPORT_1) || defined(HAL_XSPIM_IOPORT_2)
			.MemorySelect = ((DT_INST_PROP(0, ncs_line) == 1)
					? HAL_XSPI_CSSEL_NCS1
					: HAL_XSPI_CSSEL_NCS2),
#endif
			.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE,
#if defined(OCTOSPI_DCR4_REFRESH)
			.Refresh = 0,
#endif /* OCTOSPI_DCR4_REFRESH */
		},
	},
	.dev_id = 0, /* Value matching the <reg> of the ospi-nor-flash device */
	.lock = Z_MUTEX_INITIALIZER(mspi_stm32_dev_data.lock),
	.dev_cfg = {0},
	.xip_cfg = {0},
	.scramble_cfg = {0},
	.timing_cfg = {0},
	.cbs = {0},
	.ctx.lock = Z_SEM_INITIALIZER(mspi_stm32_dev_data.ctx.lock, 0, 1),
	.ctx.callback = 0,
	.ctx.callback_ctx = 0,
	MSPI_DMA_CHANNEL(0, tx, TX, MEMORY, PERIPHERAL)
	MSPI_DMA_CHANNEL(0, rx, RX, PERIPHERAL, MEMORY)
};

static void mspi_stm32_irq_config_func(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    mspi_stm32_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

DEVICE_DT_INST_DEFINE(0, &mspi_stm32_init, NULL,
		      &mspi_stm32_dev_data, &mspi_stm32_dev_conf,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &mspi_stm32_driver_api);
