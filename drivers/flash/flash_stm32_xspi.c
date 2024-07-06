/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * **************************************************************************
 * xSPI flash controller driver for stm32 serie with xSPI periherals
 * This driver is based on the stm32Cube HAL XSPI driver
 * with one xspi DTS NODE
 * **************************************************************************
 */
#define DT_DRV_COMPAT st_stm32_xspi_nor

#include <errno.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/dt-bindings/flash_controller/xspi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>

#include "spi_nor.h"
#include "jesd216.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_stm32_xspi, CONFIG_FLASH_LOG_LEVEL);

#define STM32_XSPI_NODE DT_INST_PARENT(0)

#define DT_XSPI_IO_PORT_PROP_OR(prop, default_value)					\
	COND_CODE_1(DT_NODE_HAS_PROP(STM32_XSPI_NODE, prop),				\
		    (_CONCAT(HAL_XSPIM_, DT_STRING_TOKEN(STM32_XSPI_NODE, prop))),	\
		    ((default_value)))

/* Get the base address of the flash from the DTS node */
#define STM32_XSPI_BASE_ADDRESS DT_INST_REG_ADDR(0)

#define STM32_XSPI_RESET_GPIO DT_INST_NODE_HAS_PROP(0, reset_gpios)

#define STM32_XSPI_DLYB_BYPASSED DT_PROP(STM32_XSPI_NODE, dlyb_bypass)

#define STM32_XSPI_USE_DMA DT_NODE_HAS_PROP(STM32_XSPI_NODE, dmas)

#if STM32_XSPI_USE_DMA
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <stm32_ll_dma.h>
#endif /* STM32_XSPI_USE_DMA */

#include "flash_stm32_xspi.h"

static inline void xspi_lock_thread(const struct device *dev)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);
}

static inline void xspi_unlock_thread(const struct device *dev)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
}

static int xspi_send_cmd(const struct device *dev, XSPI_RegularCmdTypeDef *cmd)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	dev_data->cmd_status = 0;

	hal_ret = HAL_XSPI_Command(&dev_data->hxspi, cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send XSPI instruction", hal_ret);
		return -EIO;
	}
	LOG_DBG("CCR 0x%x", dev_data->hxspi.Instance->CCR);

	return dev_data->cmd_status;
}

static int xspi_read_access(const struct device *dev, XSPI_RegularCmdTypeDef *cmd,
			    uint8_t *data, const size_t size)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	cmd->DataLength = size;

	dev_data->cmd_status = 0;

	hal_ret = HAL_XSPI_Command(&dev_data->hxspi, cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send XSPI instruction", hal_ret);
		return -EIO;
	}

#if STM32_XSPI_USE_DMA
	hal_ret = HAL_XSPI_Receive_DMA(&dev_data->hxspi, data);
#else
	hal_ret = HAL_XSPI_Receive_IT(&dev_data->hxspi, data);
#endif

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}

	k_sem_take(&dev_data->sync, K_FOREVER);

	return dev_data->cmd_status;
}

static int xspi_write_access(const struct device *dev, XSPI_RegularCmdTypeDef *cmd,
			     const uint8_t *data, const size_t size)
{
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	cmd->DataLength = size;

	dev_data->cmd_status = 0;

	/* in OPI/STR the 3-byte AddressWidth is not supported by the NOR flash */
	if ((dev_cfg->data_mode == XSPI_OCTO_MODE) &&
		(cmd->AddressWidth != HAL_XSPI_ADDRESS_32_BITS)) {
		LOG_ERR("XSPI wr in OPI/STR mode is for 32bit address only");
		return -EIO;
	}

	hal_ret = HAL_XSPI_Command(&dev_data->hxspi, cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send XSPI instruction", hal_ret);
		return -EIO;
	}

#if STM32_XSPI_USE_DMA
	hal_ret = HAL_XSPI_Transmit_DMA(&dev_data->hxspi, (uint8_t *)data);
#else
	hal_ret = HAL_XSPI_Transmit_IT(&dev_data->hxspi, (uint8_t *)data);
#endif

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to write data", hal_ret);
		return -EIO;
	}

	k_sem_take(&dev_data->sync, K_FOREVER);

	return dev_data->cmd_status;
}

/*
 * Gives a XSPI_RegularCmdTypeDef with all parameters set
 * except Instruction, Address, DummyCycles, NbData
 */
static XSPI_RegularCmdTypeDef xspi_prepare_cmd(const uint8_t transfer_mode,
					       const uint8_t transfer_rate)
{
	XSPI_RegularCmdTypeDef cmd_tmp = {
		.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG,
		.InstructionWidth = ((transfer_mode == XSPI_OCTO_MODE)
				? HAL_XSPI_INSTRUCTION_16_BITS
				: HAL_XSPI_INSTRUCTION_8_BITS),
		.InstructionDTRMode = ((transfer_rate == XSPI_DTR_TRANSFER)
				? HAL_XSPI_INSTRUCTION_DTR_ENABLE
				: HAL_XSPI_INSTRUCTION_DTR_DISABLE),
		.AddressDTRMode = ((transfer_rate == XSPI_DTR_TRANSFER)
				? HAL_XSPI_ADDRESS_DTR_ENABLE
				: HAL_XSPI_ADDRESS_DTR_DISABLE),
		/* AddressWidth must be set to 32bits for init and mem config phase */
		.AddressWidth = HAL_XSPI_ADDRESS_32_BITS,
		.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE,
		.DataDTRMode = ((transfer_rate == XSPI_DTR_TRANSFER)
				? HAL_XSPI_DATA_DTR_ENABLE
				: HAL_XSPI_DATA_DTR_DISABLE),
		.DQSMode = (transfer_rate == XSPI_DTR_TRANSFER)
				? HAL_XSPI_DQS_ENABLE
				: HAL_XSPI_DQS_DISABLE,
		.SIOOMode = HAL_XSPI_SIOO_INST_EVERY_CMD,
	};

	switch (transfer_mode) {
	case XSPI_OCTO_MODE: {
		cmd_tmp.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
		cmd_tmp.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
		cmd_tmp.DataMode = HAL_XSPI_DATA_8_LINES;
		break;
	}
	case XSPI_QUAD_MODE: {
		cmd_tmp.InstructionMode = HAL_XSPI_INSTRUCTION_4_LINES;
		cmd_tmp.AddressMode = HAL_XSPI_ADDRESS_4_LINES;
		cmd_tmp.DataMode = HAL_XSPI_DATA_4_LINES;
		break;
	}
	case XSPI_DUAL_MODE: {
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

static uint32_t stm32_xspi_hal_address_size(const struct device *dev)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	if (dev_data->address_width == 4U) {
		return HAL_XSPI_ADDRESS_32_BITS;
	}

	return HAL_XSPI_ADDRESS_24_BITS;
}

#if defined(CONFIG_FLASH_JESD216_API)
/*
 * Read the JEDEC ID data from the external Flash at init
 * and store in the jedec_id Table of the flash_stm32_xspi_data
 * The JEDEC ID is not given by a DTS property
 */
static int stm32_xspi_read_jedec_id(const struct device *dev)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	/* This is a SPI/STR command to issue to the external Flash device */
	XSPI_RegularCmdTypeDef cmd = xspi_prepare_cmd(XSPI_SPI_MODE, XSPI_STR_TRANSFER);

	cmd.Instruction = JESD216_CMD_READ_ID;
	cmd.AddressWidth = stm32_xspi_hal_address_size(dev);
	cmd.AddressMode = HAL_XSPI_ADDRESS_NONE;
	cmd.DataLength = JESD216_READ_ID_LEN; /* 3 bytes in the READ ID */

	HAL_StatusTypeDef hal_ret;

	hal_ret = HAL_XSPI_Command(&dev_data->hxspi, &cmd,
				   HAL_XSPI_TIMEOUT_DEFAULT_VALUE);

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send XSPI instruction", hal_ret);
		return -EIO;
	}

	/* Place the received data directly into the jedec Table */
	hal_ret = HAL_XSPI_Receive(&dev_data->hxspi, dev_data->jedec_id,
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
static int xspi_read_jedec_id(const struct device *dev,  uint8_t *id)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

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
static int stm32_xspi_read_sfdp(const struct device *dev, k_off_t addr,
				void *data,
				size_t size)
{
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *dev_data = dev->data;

	XSPI_RegularCmdTypeDef cmd = xspi_prepare_cmd(dev_cfg->data_mode,
						      dev_cfg->data_rate);
	if (dev_cfg->data_mode == XSPI_OCTO_MODE) {
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

	hal_ret = HAL_XSPI_Command(&dev_data->hxspi, &cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send XSPI instruction", hal_ret);
		return -EIO;
	}

	hal_ret = HAL_XSPI_Receive(&dev_data->hxspi, (uint8_t *)data,
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
static int xspi_read_sfdp(const struct device *dev, k_off_t addr, void *data,
			  size_t size)
{
	LOG_INF("Read SFDP from externalFlash");
	/* Get the SFDP from the external Flash (no sfdp-bfp table in the DeviceTree) */
	if (stm32_xspi_read_sfdp(dev, addr, data, size) == 0) {
		/* If valid, then ignore any table from the DTS */
		return 0;
	}
	LOG_INF("Error reading SFDP from external Flash and none in the DTS");
	return -EINVAL;
}

static bool xspi_address_is_valid(const struct device *dev, k_off_t addr,
				  size_t size)
{
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	size_t flash_size = dev_cfg->flash_size;

	return (addr >= 0) && ((uint64_t)addr + (uint64_t)size <= flash_size);
}

static int stm32_xspi_wait_auto_polling(const struct device *dev,
		XSPI_AutoPollingTypeDef *s_config, uint32_t timeout_ms)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	dev_data->cmd_status = 0;

	if (HAL_XSPI_AutoPolling_IT(&dev_data->hxspi, s_config) != HAL_OK) {
		LOG_ERR("XSPI AutoPoll failed");
		return -EIO;
	}

	if (k_sem_take(&dev_data->sync, K_MSEC(timeout_ms)) != 0) {
		LOG_ERR("XSPI AutoPoll wait failed");
		HAL_XSPI_Abort(&dev_data->hxspi);
		k_sem_reset(&dev_data->sync);
		return -EIO;
	}

	/* HAL_XSPI_AutoPolling_IT enables transfer error interrupt which sets
	 * cmd_status.
	 */
	return dev_data->cmd_status;
}

/*
 * This function Polls the WEL (write enable latch) bit to become to 0
 * When the Chip Erase Cycle is completed, the Write Enable Latch (WEL) bit is cleared.
 * in nor_mode SPI/OPI XSPI_SPI_MODE or XSPI_OCTO_MODE
 * and nor_rate transfer STR/DTR XSPI_STR_TRANSFER or XSPI_DTR_TRANSFER
 */
static int stm32_xspi_mem_erased(const struct device *dev)
{
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *dev_data = dev->data;
	uint8_t nor_mode = dev_cfg->data_mode;
	uint8_t nor_rate = dev_cfg->data_rate;

	XSPI_AutoPollingTypeDef s_config = {0};
	XSPI_RegularCmdTypeDef s_command = xspi_prepare_cmd(nor_mode, nor_rate);

	/* Configure automatic polling mode command to wait for memory ready */
	if (nor_mode == XSPI_OCTO_MODE) {
		s_command.Instruction = SPI_NOR_OCMD_RDSR;
		s_command.DummyCycles = (nor_rate == XSPI_DTR_TRANSFER)
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
	s_command.DataLength = ((nor_rate == XSPI_DTR_TRANSFER) ? 2U : 1U);
	s_command.Address = 0U;

	/* Set the mask to  0x02 to mask all Status REG bits except WEL */
	/* Set the match to 0x00 to check if the WEL bit is Reset */
	s_config.MatchValue         = SPI_NOR_WEL_MATCH;
	s_config.MatchMask          = SPI_NOR_WEL_MASK; /* Write Enable Latch */

	s_config.MatchMode          = HAL_XSPI_MATCH_MODE_AND;
	s_config.IntervalTime       = SPI_NOR_AUTO_POLLING_INTERVAL;
	s_config.AutomaticStop      = HAL_XSPI_AUTOMATIC_STOP_ENABLE;

	if (HAL_XSPI_Command(&dev_data->hxspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI AutoPoll command (WEL) failed");
		return -EIO;
	}

	/* Start Automatic-Polling mode to wait until the memory is totally erased */
	return stm32_xspi_wait_auto_polling(dev,
			&s_config, STM32_XSPI_BULK_ERASE_MAX_TIME);
}

/*
 * This function Polls the WIP(Write In Progress) bit to become to 0
 * in nor_mode SPI/OPI XSPI_SPI_MODE or XSPI_OCTO_MODE
 * and nor_rate transfer STR/DTR XSPI_STR_TRANSFER or XSPI_DTR_TRANSFER
 */
static int stm32_xspi_mem_ready(const struct device *dev, uint8_t nor_mode,
		uint8_t nor_rate)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	XSPI_AutoPollingTypeDef s_config = {0};
	XSPI_RegularCmdTypeDef s_command = xspi_prepare_cmd(nor_mode, nor_rate);

	/* Configure automatic polling mode command to wait for memory ready */
	if (nor_mode == XSPI_OCTO_MODE) {
		s_command.Instruction = SPI_NOR_OCMD_RDSR;
		s_command.DummyCycles = (nor_rate == XSPI_DTR_TRANSFER)
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
	s_command.DataLength = ((nor_rate == XSPI_DTR_TRANSFER) ? 2U : 1U);
	s_command.Address = 0U;

	/* Set the mask to  0x01 to mask all Status REG bits except WIP */
	/* Set the match to 0x00 to check if the WIP bit is Reset */
	s_config.MatchValue         = SPI_NOR_MEM_RDY_MATCH;
	s_config.MatchMask          = SPI_NOR_MEM_RDY_MASK; /* Write in progress */
	s_config.MatchMode          = HAL_XSPI_MATCH_MODE_AND;
	s_config.IntervalTime       = SPI_NOR_AUTO_POLLING_INTERVAL;
	s_config.AutomaticStop      = HAL_XSPI_AUTOMATIC_STOP_ENABLE;

	if (HAL_XSPI_Command(&dev_data->hxspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI AutoPoll command failed");
		return -EIO;
	}

	/* Start Automatic-Polling mode to wait until the memory is ready WIP=0 */
	return stm32_xspi_wait_auto_polling(dev, &s_config, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
}

/* Enables writing to the memory sending a Write Enable and wait it is effective */
static int stm32_xspi_write_enable(const struct device *dev,
		uint8_t nor_mode, uint8_t nor_rate)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	XSPI_AutoPollingTypeDef s_config = {0};
	XSPI_RegularCmdTypeDef s_command = xspi_prepare_cmd(nor_mode, nor_rate);

	/* Initialize the write enable command */
	if (nor_mode == XSPI_OCTO_MODE) {
		s_command.Instruction = SPI_NOR_OCMD_WREN;
	} else {
		s_command.Instruction = SPI_NOR_CMD_WREN;
		/* force 1-line InstructionMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
	}
	s_command.AddressMode = HAL_XSPI_ADDRESS_NONE;
	s_command.DataMode    = HAL_XSPI_DATA_NONE;
	s_command.DummyCycles = 0U;

	if (HAL_XSPI_Command(&dev_data->hxspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI flash write enable cmd failed");
		return -EIO;
	}

	/* New command to Configure automatic polling mode to wait for write enabling */
	if (nor_mode == XSPI_OCTO_MODE) {
		s_command.Instruction = SPI_NOR_OCMD_RDSR;
		s_command.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
		s_command.DataMode = HAL_XSPI_DATA_8_LINES;
		s_command.DummyCycles = (nor_rate == XSPI_DTR_TRANSFER)
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
	s_command.DataLength = (nor_rate == XSPI_DTR_TRANSFER) ? 2U : 1U;
	s_command.Address = 0U;

	if (HAL_XSPI_Command(&dev_data->hxspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI config auto polling cmd failed");
		return -EIO;
	}

	s_config.MatchValue      = SPI_NOR_WREN_MATCH;
	s_config.MatchMask       = SPI_NOR_WREN_MASK;
	s_config.MatchMode       = HAL_XSPI_MATCH_MODE_AND;
	s_config.IntervalTime    = SPI_NOR_AUTO_POLLING_INTERVAL;
	s_config.AutomaticStop   = HAL_XSPI_AUTOMATIC_STOP_ENABLE;

	return stm32_xspi_wait_auto_polling(dev, &s_config, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
}

/* Write Flash configuration register 2 with new dummy cycles */
static int stm32_xspi_write_cfg2reg_dummy(XSPI_HandleTypeDef *hxspi,
					uint8_t nor_mode, uint8_t nor_rate)
{
	uint8_t transmit_data = SPI_NOR_CR2_DUMMY_CYCLES_66MHZ;
	XSPI_RegularCmdTypeDef s_command = xspi_prepare_cmd(nor_mode, nor_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (nor_mode == XSPI_SPI_MODE)
				? SPI_NOR_CMD_WR_CFGREG2
				: SPI_NOR_OCMD_WR_CFGREG2;
	s_command.Address = SPI_NOR_REG2_ADDR3;
	s_command.DummyCycles = 0U;
	s_command.DataLength = (nor_mode == XSPI_SPI_MODE) ? 1U
			: ((nor_rate == XSPI_DTR_TRANSFER) ? 2U : 1U);

	if (HAL_XSPI_Command(hxspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI transmit cmd");
		return -EIO;
	}

	if (HAL_XSPI_Transmit(hxspi, &transmit_data,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI transmit ");
		return -EIO;
	}

	return 0;
}

/* Write Flash configuration register 2 with new single or octal SPI protocol */
static int stm32_xspi_write_cfg2reg_io(XSPI_HandleTypeDef *hxspi,
				       uint8_t nor_mode, uint8_t nor_rate, uint8_t op_enable)
{
	XSPI_RegularCmdTypeDef s_command = xspi_prepare_cmd(nor_mode, nor_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (nor_mode == XSPI_SPI_MODE)
				? SPI_NOR_CMD_WR_CFGREG2
				: SPI_NOR_OCMD_WR_CFGREG2;
	s_command.Address = SPI_NOR_REG2_ADDR1;
	s_command.DummyCycles = 0U;
	s_command.DataLength = (nor_mode == XSPI_SPI_MODE) ? 1U
		: ((nor_rate == XSPI_DTR_TRANSFER) ? 2U : 1U);

	if (HAL_XSPI_Command(hxspi, &s_command,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	if (HAL_XSPI_Transmit(hxspi, &op_enable,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	return 0;
}

/* Read Flash configuration register 2 with new single or octal SPI protocol */
static int stm32_xspi_read_cfg2reg(XSPI_HandleTypeDef *hxspi,
				   uint8_t nor_mode, uint8_t nor_rate, uint8_t *value)
{
	XSPI_RegularCmdTypeDef s_command = xspi_prepare_cmd(nor_mode, nor_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (nor_mode == XSPI_SPI_MODE)
				? SPI_NOR_CMD_RD_CFGREG2
				: SPI_NOR_OCMD_RD_CFGREG2;
	s_command.Address = SPI_NOR_REG2_ADDR1;
	s_command.DummyCycles = (nor_mode == XSPI_SPI_MODE)
				? 0U
				: ((nor_rate == XSPI_DTR_TRANSFER)
					? SPI_NOR_DUMMY_REG_OCTAL_DTR
					: SPI_NOR_DUMMY_REG_OCTAL);
	s_command.DataLength = (nor_rate == XSPI_DTR_TRANSFER) ? 2U : 1U;

	if (HAL_XSPI_Command(hxspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	if (HAL_XSPI_Receive(hxspi, value, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	return 0;
}

/* Set the NOR Flash to desired Interface mode : SPI/OSPI and STR/DTR according to the DTS */
static int stm32_xspi_config_mem(const struct device *dev)
{
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *dev_data = dev->data;
	uint8_t reg[2];

	/* Going to set the SPI mode and STR transfer rate : done */
	if ((dev_cfg->data_mode != XSPI_OCTO_MODE)
		&& (dev_cfg->data_rate == XSPI_STR_TRANSFER)) {
		LOG_INF("OSPI flash config is SPI|DUAL|QUAD / STR");
		return 0;
	}

	/* Going to set the XPI mode (STR or DTR transfer rate) */
	LOG_DBG("XSPI configuring Octo SPI mode");

	if (stm32_xspi_write_enable(dev,
		XSPI_SPI_MODE, XSPI_STR_TRANSFER) != 0) {
		LOG_ERR("OSPI write Enable failed");
		return -EIO;
	}

	/* Write Configuration register 2 (with new dummy cycles) */
	if (stm32_xspi_write_cfg2reg_dummy(&dev_data->hxspi,
		XSPI_SPI_MODE, XSPI_STR_TRANSFER) != 0) {
		LOG_ERR("XSPI write CFGR2 failed");
		return -EIO;
	}
	if (stm32_xspi_mem_ready(dev,
		XSPI_SPI_MODE, XSPI_STR_TRANSFER) != 0) {
		LOG_ERR("XSPI autopolling failed");
		return -EIO;
	}
	if (stm32_xspi_write_enable(dev,
		XSPI_SPI_MODE, XSPI_STR_TRANSFER) != 0) {
		LOG_ERR("XSPI write Enable 2 failed");
		return -EIO;
	}

	/* Write Configuration register 2 (with Octal I/O SPI protocol : choose STR or DTR) */
	uint8_t mode_enable = ((dev_cfg->data_rate == XSPI_DTR_TRANSFER)
				? SPI_NOR_CR2_DTR_OPI_EN
				: SPI_NOR_CR2_STR_OPI_EN);
	if (stm32_xspi_write_cfg2reg_io(&dev_data->hxspi,
		XSPI_SPI_MODE, XSPI_STR_TRANSFER, mode_enable) != 0) {
		LOG_ERR("XSPI write CFGR2 failed");
		return -EIO;
	}

	/* Wait that the configuration is effective and check that memory is ready */
	k_busy_wait(STM32_XSPI_WRITE_REG_MAX_TIME * USEC_PER_MSEC);

	/* Reconfigure the memory type of the peripheral */
	dev_data->hxspi.Init.MemoryType            = HAL_XSPI_MEMTYPE_MACRONIX;
	dev_data->hxspi.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
	if (HAL_XSPI_Init(&dev_data->hxspi) != HAL_OK) {
		LOG_ERR("XSPI mem type MACRONIX failed");
		return -EIO;
	}

	if (dev_cfg->data_rate == XSPI_STR_TRANSFER) {
		if (stm32_xspi_mem_ready(dev,
			XSPI_OCTO_MODE, XSPI_STR_TRANSFER) != 0) {
			/* Check Flash busy ? */
			LOG_ERR("XSPI flash busy failed");
			return -EIO;
		}

		if (stm32_xspi_read_cfg2reg(&dev_data->hxspi,
			XSPI_OCTO_MODE, XSPI_STR_TRANSFER, reg) != 0) {
			/* Check the configuration has been correctly done on SPI_NOR_REG2_ADDR1 */
			LOG_ERR("XSPI flash config read failed");
			return -EIO;
		}

		LOG_INF("XSPI flash config is OCTO / STR");
	}

	if (dev_cfg->data_rate == XSPI_DTR_TRANSFER) {
		if (stm32_xspi_mem_ready(dev,
			XSPI_OCTO_MODE, XSPI_DTR_TRANSFER) != 0) {
			/* Check Flash busy ? */
			LOG_ERR("XSPI flash busy failed");
			return -EIO;
		}

		LOG_INF("XSPI flash config is OCTO / DTR");
	}

	return 0;
}

/* gpio or send the different reset command to the NOR flash in SPI/OSPI and STR/DTR */
static int stm32_xspi_mem_reset(const struct device *dev)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

#if STM32_XSPI_RESET_GPIO
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;

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
	if (HAL_XSPI_Command(&dev_data->hxspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI reset enable (SPI/STR) failed");
		return -EIO;
	}

	/* Reset memory in SPI mode and STR transfer mode */
	s_command.Instruction = SPI_NOR_CMD_RESET_MEM;
	if (HAL_XSPI_Command(&dev_data->hxspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI reset memory (SPI/STR) failed");
		return -EIO;
	}

	/* Reset enable in OPI mode and STR transfer mode */
	s_command.InstructionMode    = HAL_XSPI_INSTRUCTION_8_LINES;
	s_command.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
	s_command.Instruction = SPI_NOR_OCMD_RESET_EN;
	s_command.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
	if (HAL_XSPI_Command(&dev_data->hxspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI reset enable (OCTO/STR) failed");
		return -EIO;
	}

	/* Reset memory in OPI mode and STR transfer mode */
	s_command.Instruction = SPI_NOR_OCMD_RESET_MEM;
	if (HAL_XSPI_Command(&dev_data->hxspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI reset memory (OCTO/STR) failed");
		return -EIO;
	}

	/* Reset enable in OPI mode and DTR transfer mode */
	s_command.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
	s_command.Instruction = SPI_NOR_OCMD_RESET_EN;
	if (HAL_XSPI_Command(&dev_data->hxspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI reset enable (OCTO/DTR) failed");
		return -EIO;
	}

	/* Reset memory in OPI mode and DTR transfer mode */
	s_command.Instruction = SPI_NOR_OCMD_RESET_MEM;
	if (HAL_XSPI_Command(&dev_data->hxspi,
		&s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI reset memory (OCTO/DTR) failed");
		return -EIO;
	}

#endif /* STM32_XSPI_RESET_GPIO */
	/* Wait after SWreset CMD, in case SWReset occurred during erase operation */
	k_busy_wait(STM32_XSPI_RESET_MAX_TIME * USEC_PER_MSEC);

	return 0;
}

#ifdef CONFIG_STM32_MEMMAP
/* Function to configure the octoflash in MemoryMapped mode */
static int stm32_xspi_set_memorymap(const struct device *dev)
{
	HAL_StatusTypeDef ret;
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *dev_data = dev->data;
	XSPI_RegularCmdTypeDef s_command = {0}; /* Non-zero values disturb the command */
	XSPI_MemoryMappedTypeDef s_MemMappedCfg;

	/* Configure octoflash in MemoryMapped mode */
	if ((dev_cfg->data_mode == XSPI_SPI_MODE) &&
		(stm32_xspi_hal_address_size(dev) == HAL_XSPI_ADDRESS_24_BITS)) {
		/* OPI mode and 3-bytes address size not supported by memory */
		LOG_ERR("XSPI_SPI_MODE in 3Bytes addressing is not supported");
		return -EIO;
	}

	/* Initialize the read command */
	s_command.OperationType = HAL_XSPI_OPTYPE_READ_CFG;
	s_command.InstructionMode = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? ((dev_cfg->data_mode == XSPI_SPI_MODE)
					? HAL_XSPI_INSTRUCTION_1_LINE
					: HAL_XSPI_INSTRUCTION_8_LINES)
				: HAL_XSPI_INSTRUCTION_8_LINES;
	s_command.InstructionDTRMode = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? HAL_XSPI_INSTRUCTION_DTR_DISABLE
				: HAL_XSPI_INSTRUCTION_DTR_ENABLE;
	s_command.InstructionWidth = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? ((dev_cfg->data_mode == XSPI_SPI_MODE)
					? HAL_XSPI_INSTRUCTION_8_BITS
					: HAL_XSPI_INSTRUCTION_16_BITS)
				: HAL_XSPI_INSTRUCTION_16_BITS;
	s_command.Instruction = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? ((dev_cfg->data_mode == XSPI_SPI_MODE)
					? ((stm32_xspi_hal_address_size(dev) ==
					HAL_XSPI_ADDRESS_24_BITS)
						? SPI_NOR_CMD_READ_FAST
						: SPI_NOR_CMD_READ_FAST_4B)
					: dev_data->read_opcode)
				: SPI_NOR_OCMD_DTR_RD;
	s_command.AddressMode = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? ((dev_cfg->data_mode == XSPI_SPI_MODE)
					? HAL_XSPI_ADDRESS_1_LINE
					: HAL_XSPI_ADDRESS_8_LINES)
				: HAL_XSPI_ADDRESS_8_LINES;
	s_command.AddressDTRMode = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? HAL_XSPI_ADDRESS_DTR_DISABLE
				: HAL_XSPI_ADDRESS_DTR_ENABLE;
	s_command.AddressWidth = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? stm32_xspi_hal_address_size(dev)
				: HAL_XSPI_ADDRESS_32_BITS;
	s_command.DataMode = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? ((dev_cfg->data_mode == XSPI_SPI_MODE)
					? HAL_XSPI_DATA_1_LINE
					: HAL_XSPI_DATA_8_LINES)
				: HAL_XSPI_DATA_8_LINES;
	s_command.DataDTRMode = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? HAL_XSPI_DATA_DTR_DISABLE
				: HAL_XSPI_DATA_DTR_ENABLE;
	s_command.DummyCycles = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? ((dev_cfg->data_mode == XSPI_SPI_MODE)
					? SPI_NOR_DUMMY_RD
					: SPI_NOR_DUMMY_RD_OCTAL)
				: SPI_NOR_DUMMY_RD_OCTAL_DTR;
	s_command.DQSMode = (dev_cfg->data_rate == XSPI_STR_TRANSFER)
				? HAL_XSPI_DQS_DISABLE
				: HAL_XSPI_DQS_ENABLE;
#ifdef XSPI_CCR_SIOO
	s_command.SIOOMode = HAL_XSPI_SIOO_INST_EVERY_CMD;
#endif /* XSPI_CCR_SIOO */

	ret = HAL_XSPI_Command(&dev_data->hxspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (ret != HAL_OK) {
		LOG_ERR("%d: Failed to set memory map", ret);
		return -EIO;
	}

	/* Initialize the program command */
	s_command.OperationType = HAL_XSPI_OPTYPE_WRITE_CFG;
	if (dev_cfg->data_rate == XSPI_STR_TRANSFER) {
		s_command.Instruction = (dev_cfg->data_mode == XSPI_SPI_MODE)
					? ((stm32_xspi_hal_address_size(dev) ==
					HAL_XSPI_ADDRESS_24_BITS)
						? SPI_NOR_CMD_PP
						: SPI_NOR_CMD_PP_4B)
					: SPI_NOR_OCMD_PAGE_PRG;
	} else {
		s_command.Instruction = SPI_NOR_OCMD_PAGE_PRG;
	}
	s_command.DQSMode = HAL_XSPI_DQS_DISABLE;

	ret = HAL_XSPI_Command(&dev_data->hxspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (ret != HAL_OK) {
		LOG_ERR("%d: Failed to set memory mapped", ret);
		return -EIO;
	}

	/* Enable the memory-mapping */
	s_MemMappedCfg.TimeOutActivation = HAL_XSPI_TIMEOUT_COUNTER_DISABLE;

	ret = HAL_XSPI_MemoryMapped(&dev_data->hxspi, &s_MemMappedCfg);
	if (ret != HAL_OK) {
		LOG_ERR("%d: Failed to enable memory mapped", ret);
		return -EIO;
	}

	LOG_DBG("MemoryMap mode enabled");
	return 0;
}

/* Function to return true if the octoflash is in MemoryMapped else false */
static bool stm32_xspi_is_memorymap(const struct device *dev)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	return ((READ_BIT(dev_data->hxspi.Instance->CR,
			  XSPI_CR_FMODE) == XSPI_CR_FMODE) ?
			  true : false);
}

static int stm32_xspi_abort(const struct device *dev)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	if (HAL_XSPI_Abort(&dev_data->hxspi) != HAL_OK) {
		LOG_ERR("XSPI abort failed");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_STM32_MEMMAP */

/*
 * Function to erase the flash : chip or sector with possible OCTO/SPI and STR/DTR
 * to erase the complete chip (using dedicated command) :
 *   set size >= flash size
 *   set addr = 0
 */
static int flash_stm32_xspi_erase(const struct device *dev, k_off_t addr,
				  size_t size)
{
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *dev_data = dev->data;
	int ret = 0;

	/* Ignore zero size erase */
	if (size == 0) {
		return 0;
	}

	/* Maximise erase size : means the complete chip */
	if (size > dev_cfg->flash_size) {
		size = dev_cfg->flash_size;
	}

	if (!xspi_address_is_valid(dev, addr, size)) {
		LOG_ERR("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	if (((size % SPI_NOR_SECTOR_SIZE) != 0) && (size < dev_cfg->flash_size)) {
		LOG_ERR("Error: wrong sector size 0x%x", size);
		return -ENOTSUP;
	}

	xspi_lock_thread(dev);

#ifdef CONFIG_STM32_MEMMAP
	if (stm32_xspi_is_memorymap(dev)) {
		/* Abort ongoing transfer to force CS high/BUSY deasserted */
		ret = stm32_xspi_abort(dev);
		if (ret != 0) {
			LOG_ERR("Failed to abort memory-mapped access before erase");
			goto erase_end;
		}
	}
#endif

	XSPI_RegularCmdTypeDef cmd_erase = {
		.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG,
		.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE,
		.DataMode = HAL_XSPI_DATA_NONE,
		.DummyCycles = 0U,
		.DQSMode = HAL_XSPI_DQS_DISABLE,
		.SIOOMode = HAL_XSPI_SIOO_INST_EVERY_CMD,
	};

	if (stm32_xspi_mem_ready(dev,
		dev_cfg->data_mode, dev_cfg->data_rate) != 0) {
		LOG_ERR("Erase failed : flash busy");
		goto erase_end;
	}

	cmd_erase.InstructionMode    = (dev_cfg->data_mode == XSPI_OCTO_MODE)
					? HAL_XSPI_INSTRUCTION_8_LINES
					: HAL_XSPI_INSTRUCTION_1_LINE;
	cmd_erase.InstructionDTRMode = (dev_cfg->data_rate == XSPI_DTR_TRANSFER)
					? HAL_XSPI_INSTRUCTION_DTR_ENABLE
					: HAL_XSPI_INSTRUCTION_DTR_DISABLE;
	cmd_erase.InstructionWidth    = (dev_cfg->data_mode == XSPI_OCTO_MODE)
					? HAL_XSPI_INSTRUCTION_16_BITS
					: HAL_XSPI_INSTRUCTION_8_BITS;

	while ((size > 0) && (ret == 0)) {

		ret = stm32_xspi_write_enable(dev,
			dev_cfg->data_mode, dev_cfg->data_rate);
		if (ret != 0) {
			LOG_ERR("Erase failed : write enable");
			break;
		}

		if (size == dev_cfg->flash_size) {
			/* Chip erase */
			LOG_DBG("Chip Erase");

			cmd_erase.Address = 0;
			cmd_erase.Instruction = (dev_cfg->data_mode == XSPI_OCTO_MODE)
					? SPI_NOR_OCMD_BULKE
					: SPI_NOR_CMD_BULKE;
			cmd_erase.AddressMode = HAL_XSPI_ADDRESS_NONE;
			/* Full chip erase (Bulk) command */
			xspi_send_cmd(dev, &cmd_erase);

			size -= dev_cfg->flash_size;
			/* Chip (Bulk) erase started, wait until WEL becomes 0 */
			ret = stm32_xspi_mem_erased(dev);
			if (ret != 0) {
				LOG_ERR("Chip Erase failed");
				break;
			}
		} else {
			/* Sector or Block erase depending on the size */
			LOG_DBG("Sector/Block Erase");

			cmd_erase.AddressMode =
				(dev_cfg->data_mode == XSPI_OCTO_MODE)
				? HAL_XSPI_ADDRESS_8_LINES
				: HAL_XSPI_ADDRESS_1_LINE;
			cmd_erase.AddressDTRMode =
				(dev_cfg->data_rate == XSPI_DTR_TRANSFER)
				? HAL_XSPI_ADDRESS_DTR_ENABLE
				: HAL_XSPI_ADDRESS_DTR_DISABLE;
			cmd_erase.AddressWidth = stm32_xspi_hal_address_size(dev);
			cmd_erase.Address = addr;

			const struct jesd216_erase_type *erase_types =
							dev_data->erase_types;
			const struct jesd216_erase_type *bet = NULL;

			for (uint8_t ei = 0;
				ei < JESD216_NUM_ERASE_TYPES; ++ei) {
				const struct jesd216_erase_type *etp =
							&erase_types[ei];

				if ((etp->exp != 0)
				    && SPI_NOR_IS_ALIGNED(addr, etp->exp)
				    && (size >= BIT(etp->exp))
				    && ((bet == NULL)
					|| (etp->exp > bet->exp))) {
					bet = etp;
					cmd_erase.Instruction = bet->cmd;
				} else if (bet == NULL) {
					/* Use the default sector erase cmd */
					if (dev_cfg->data_mode == XSPI_OCTO_MODE) {
						cmd_erase.Instruction = SPI_NOR_OCMD_SE;
					} else {
						cmd_erase.Instruction =
							(stm32_xspi_hal_address_size(dev) ==
							HAL_XSPI_ADDRESS_32_BITS)
							? SPI_NOR_CMD_SE_4B
							: SPI_NOR_CMD_SE;
					}
				}
				/* Avoid using wrong erase type,
				 * if zero entries are found in erase_types
				 */
				bet = NULL;
			}
			LOG_DBG("Sector/Block Erase addr 0x%x, asize 0x%x amode 0x%x  instr 0x%x",
				cmd_erase.Address, cmd_erase.AddressWidth,
				cmd_erase.AddressMode, cmd_erase.Instruction);

			xspi_send_cmd(dev, &cmd_erase);

			if (bet != NULL) {
				addr += BIT(bet->exp);
				size -= BIT(bet->exp);
			} else {
				addr += SPI_NOR_SECTOR_SIZE;
				size -= SPI_NOR_SECTOR_SIZE;
			}

			ret = stm32_xspi_mem_ready(dev, dev_cfg->data_mode,
						dev_cfg->data_rate);
		}

	}
	/* Ends the erase operation */

erase_end:
	xspi_unlock_thread(dev);

	return ret;
}

/* Function to read the flash with possible OCTO/SPI and STR/DTR */
static int flash_stm32_xspi_read(const struct device *dev, k_off_t addr,
				 void *data, size_t size)
{
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *dev_data = dev->data;
	int ret;

	if (!xspi_address_is_valid(dev, addr, size)) {
		LOG_ERR("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	/* Ignore zero size read */
	if (size == 0) {
		return 0;
	}

#ifdef CONFIG_STM32_MEMMAP
	ARG_UNUSED(dev_cfg);
	ARG_UNUSED(dev_data);

	xspi_lock_thread(dev);

	/* Do reads through memory-mapping instead of indirect */
	if (!stm32_xspi_is_memorymap(dev)) {
		ret = stm32_xspi_set_memorymap(dev);
		if (ret != 0) {
			LOG_ERR("READ: failed to set memory mapped");
			goto read_end;
		}
	}

	__ASSERT_NO_MSG(stm32_xspi_is_memorymap(dev));

	uintptr_t mmap_addr = STM32_XSPI_BASE_ADDRESS + addr;

	LOG_DBG("Memory-mapped read from 0x%08lx, len %zu", mmap_addr, size);
	memcpy(data, (void *)mmap_addr, size);
	ret = 0;
	goto read_end;
#else
	XSPI_RegularCmdTypeDef cmd = xspi_prepare_cmd(dev_cfg->data_mode, dev_cfg->data_rate);

	if (dev_cfg->data_mode != XSPI_OCTO_MODE) {
		switch (dev_data->read_mode) {
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
			/* use the mode from ospi_prepare_cmd */
			break;
		}
	}

	/* Instruction and DummyCycles are set below */
	cmd.Address = addr; /* AddressSize is 32bits in OPSI mode */
	cmd.AddressWidth = stm32_xspi_hal_address_size(dev);
	/* DataSize is set by the read cmd */

	/* Configure other parameters */
	if (dev_cfg->data_rate == XSPI_DTR_TRANSFER) {
		/* DTR transfer rate (==> Octal mode) */
		cmd.Instruction = SPI_NOR_OCMD_DTR_RD;
		cmd.DummyCycles = SPI_NOR_DUMMY_RD_OCTAL_DTR;
	} else {
		/* STR transfer rate */
		if (dev_cfg->data_mode == XSPI_OCTO_MODE) {
			/* OPI and STR */
			cmd.Instruction = SPI_NOR_OCMD_RD;
			cmd.DummyCycles = SPI_NOR_DUMMY_RD_OCTAL;
		} else {
			/* use SFDP:BFP read instruction */
			cmd.Instruction = dev_data->read_opcode;
			cmd.DummyCycles = dev_data->read_dummy;
			/* in SPI and STR : expecting SPI_NOR_CMD_READ_FAST_4B */
		}
	}

	LOG_DBG("XSPI: read %zu data at 0x%lx", size, STM32_XSPI_BASE_ADDRESS + (long)addr);
	xspi_lock_thread(dev);

	ret = xspi_read_access(dev, &cmd, data, size);
	goto read_end;
#endif

read_end:
	xspi_unlock_thread(dev);

	return ret;
}

/* Function to write the flash (page program) : with possible OCTO/SPI and STR/DTR */
static int flash_stm32_xspi_write(const struct device *dev, k_off_t addr,
				  const void *data, size_t size)
{
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *dev_data = dev->data;
	size_t to_write;
	int ret = 0;

	if (!xspi_address_is_valid(dev, addr, size)) {
		LOG_ERR("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	/* Ignore zero size write */
	if (size == 0) {
		return 0;
	}

	xspi_lock_thread(dev);

#ifdef CONFIG_STM32_MEMMAP
	ARG_UNUSED(dev_data);

	if (stm32_xspi_is_memorymap(dev)) {
		/* Abort ongoing transfer to force CS high/BUSY deasserted */
		ret = stm32_xspi_abort(dev);
		if (ret != 0) {
			LOG_ERR("Failed to abort memory-mapped access before write");
			goto write_end;
		}
	}
#endif
	/* page program for STR or DTR mode */
	XSPI_RegularCmdTypeDef cmd_pp = xspi_prepare_cmd(dev_cfg->data_mode, dev_cfg->data_rate);

	/* using 32bits address also in SPI/STR mode */
	cmd_pp.Instruction = dev_data->write_opcode;

	if (dev_cfg->data_mode != XSPI_OCTO_MODE) {
		switch (cmd_pp.Instruction) {
		case SPI_NOR_CMD_PP_4B:
			__fallthrough;
		case SPI_NOR_CMD_PP: {
			cmd_pp.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
			cmd_pp.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
			cmd_pp.DataMode = HAL_XSPI_DATA_1_LINE;
			break;
		}
		case SPI_NOR_CMD_PP_1_1_4_4B:
			__fallthrough;
		case SPI_NOR_CMD_PP_1_1_4: {
			cmd_pp.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
			cmd_pp.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
			cmd_pp.DataMode = HAL_XSPI_DATA_4_LINES;
			break;
		}
		case SPI_NOR_CMD_PP_1_4_4_4B:
			__fallthrough;
		case SPI_NOR_CMD_PP_1_4_4: {
			cmd_pp.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
			cmd_pp.AddressMode = HAL_XSPI_ADDRESS_4_LINES;
			cmd_pp.DataMode = HAL_XSPI_DATA_4_LINES;
			break;
		}
		default:
			/* use the mode from ospi_prepare_cmd */
			break;
		}
	}

	cmd_pp.Address = addr;
	cmd_pp.AddressWidth = stm32_xspi_hal_address_size(dev);
	cmd_pp.DummyCycles = 0U;

	LOG_DBG("XSPI: write %zu data at 0x%lx", size, STM32_XSPI_BASE_ADDRESS + (long)addr);

	ret = stm32_xspi_mem_ready(dev,
				   dev_cfg->data_mode, dev_cfg->data_rate);
	if (ret != 0) {
		LOG_ERR("XSPI: write not ready");
		goto write_end;
	}

	while ((size > 0) && (ret == 0)) {
		to_write = size;
		ret = stm32_xspi_write_enable(dev,
					      dev_cfg->data_mode, dev_cfg->data_rate);
		if (ret != 0) {
			LOG_ERR("XSPI: write not enabled");
			break;
		}
		/* Don't write more than a page. */
		if (to_write >= SPI_NOR_PAGE_SIZE) {
			to_write = SPI_NOR_PAGE_SIZE;
		}

		/* Don't write across a page boundary */
		if (((addr + to_write - 1U) / SPI_NOR_PAGE_SIZE)
		    != (addr / SPI_NOR_PAGE_SIZE)) {
			to_write = SPI_NOR_PAGE_SIZE -
						(addr % SPI_NOR_PAGE_SIZE);
		}
		cmd_pp.Address = addr;

		ret = xspi_write_access(dev, &cmd_pp, data, to_write);
		if (ret != 0) {
			LOG_ERR("XSPI: write not access");
			break;
		}

		size -= to_write;
		data = (const uint8_t *)data + to_write;
		addr += to_write;

		/* Configure automatic polling mode to wait for end of program */
		ret = stm32_xspi_mem_ready(dev,
						 dev_cfg->data_mode, dev_cfg->data_rate);
		if (ret != 0) {
			LOG_ERR("XSPI: write PP not ready");
			break;
		}
	}
	/* Ends the write operation */

write_end:
	xspi_unlock_thread(dev);

	return ret;
}

static const struct flash_parameters flash_stm32_xspi_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff
};

static const struct flash_parameters *
flash_stm32_xspi_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_stm32_xspi_parameters;
}

static void flash_stm32_xspi_isr(const struct device *dev)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	HAL_XSPI_IRQHandler(&dev_data->hxspi);
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
#if STM32_XSPI_USE_DMA
static void xspi_dma_callback(const struct device *dev, void *arg,
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
void HAL_XSPI_ErrorCallback(XSPI_HandleTypeDef *hxspi)
{
	struct flash_stm32_xspi_data *dev_data =
		CONTAINER_OF(hxspi, struct flash_stm32_xspi_data, hxspi);

	LOG_DBG("Error cb");

	dev_data->cmd_status = -EIO;

	k_sem_give(&dev_data->sync);
}

/*
 * Command completed callback.
 */
void HAL_XSPI_CmdCpltCallback(XSPI_HandleTypeDef *hxspi)
{
	struct flash_stm32_xspi_data *dev_data =
		CONTAINER_OF(hxspi, struct flash_stm32_xspi_data, hxspi);

	LOG_DBG("Cmd Cplt cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Rx Transfer completed callback.
 */
void HAL_XSPI_RxCpltCallback(XSPI_HandleTypeDef *hxspi)
{
	struct flash_stm32_xspi_data *dev_data =
		CONTAINER_OF(hxspi, struct flash_stm32_xspi_data, hxspi);

	LOG_DBG("Rx Cplt cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Tx Transfer completed callback.
 */
void HAL_XSPI_TxCpltCallback(XSPI_HandleTypeDef *hxspi)
{
	struct flash_stm32_xspi_data *dev_data =
		CONTAINER_OF(hxspi, struct flash_stm32_xspi_data, hxspi);

	LOG_DBG("Tx Cplt cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Status Match callback.
 */
void HAL_XSPI_StatusMatchCallback(XSPI_HandleTypeDef *hxspi)
{
	struct flash_stm32_xspi_data *dev_data =
		CONTAINER_OF(hxspi, struct flash_stm32_xspi_data, hxspi);

	LOG_DBG("Status Match cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Timeout callback.
 */
void HAL_XSPI_TimeOutCallback(XSPI_HandleTypeDef *hxspi)
{
	struct flash_stm32_xspi_data *dev_data =
		CONTAINER_OF(hxspi, struct flash_stm32_xspi_data, hxspi);

	LOG_DBG("Timeout cb");

	dev_data->cmd_status = -EIO;

	k_sem_give(&dev_data->sync);
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_stm32_xspi_pages_layout(const struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	struct flash_stm32_xspi_data *dev_data = dev->data;

	*layout = &dev_data->layout;
	*layout_size = 1;
}
#endif

static const struct flash_driver_api flash_stm32_xspi_driver_api = {
	.read = flash_stm32_xspi_read,
	.write = flash_stm32_xspi_write,
	.erase = flash_stm32_xspi_erase,
	.get_parameters = flash_stm32_xspi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_stm32_xspi_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = xspi_read_sfdp,
	.read_jedec_id = xspi_read_jedec_id,
#endif /* CONFIG_FLASH_JESD216_API */
};

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static int setup_pages_layout(const struct device *dev)
{
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *data = dev->data;
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

static int stm32_xspi_read_status_register(const struct device *dev, uint8_t reg_num, uint8_t *reg)
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

	return xspi_read_access(dev, &s_command, reg, sizeof(*reg));
}

static int stm32_xspi_write_status_register(const struct device *dev, uint8_t reg_num, uint8_t reg)
{
	struct flash_stm32_xspi_data *data = dev->data;
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
			ret = stm32_xspi_read_status_register(dev, 2, &regs[1]);
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
			ret = stm32_xspi_read_status_register(dev, 1, &regs[0]);
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

	return xspi_write_access(dev, &s_command, regs_p, size);
}

static int stm32_xspi_enable_qe(const struct device *dev)
{
	struct flash_stm32_xspi_data *data = dev->data;
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

	ret = stm32_xspi_read_status_register(dev, qe_reg_num, &reg);
	if (ret < 0) {
		return ret;
	}

	/* exit early if QE bit is already set */
	if ((reg & qe_bit) != 0U) {
		return 0;
	}

	ret = stm32_xspi_write_enable(dev, XSPI_SPI_MODE, XSPI_STR_TRANSFER);
	if (ret < 0) {
		return ret;
	}

	reg |= qe_bit;

	ret = stm32_xspi_write_status_register(dev, qe_reg_num, reg);
	if (ret < 0) {
		return ret;
	}

	ret = stm32_xspi_mem_ready(dev, XSPI_SPI_MODE, XSPI_STR_TRANSFER);
	if (ret < 0) {
		return ret;
	}

	/* validate that QE bit is set */
	ret = stm32_xspi_read_status_register(dev, qe_reg_num, &reg);
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
	struct flash_stm32_xspi_data *data = dev->data;

	if ((jesd216_bfp_addrbytes == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B) ||
	    (jesd216_bfp_addrbytes == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B4B)) {
		data->address_width = 4U;
	} else {
		data->address_width = 3U;
	}
}

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
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *data = dev->data;
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
	LOG_DBG("Address width: %u Bytes", data->address_width);

	/* use PP opcode based on configured data mode if nothing is set in DTS */
	if (data->write_opcode == SPI_NOR_WRITEOC_NONE) {
		switch (dev_cfg->data_mode) {
		case XSPI_OCTO_MODE:
			data->write_opcode = SPI_NOR_OCMD_PAGE_PRG;
			break;
		case XSPI_QUAD_MODE:
			data->write_opcode = SPI_NOR_CMD_PP_1_4_4;
			break;
		case XSPI_DUAL_MODE:
			data->write_opcode = SPI_NOR_CMD_PP_1_1_2;
			break;
		default:
			data->write_opcode = SPI_NOR_CMD_PP;
			break;
		}
	}

	if (dev_cfg->data_mode != XSPI_OCTO_MODE) {
		/* determine supported read modes, begin from the slowest */
		data->read_mode = JESD216_MODE_111;
		data->read_opcode = SPI_NOR_CMD_READ;
		data->read_dummy = 0U;

		if (dev_cfg->data_mode != XSPI_SPI_MODE) {
			if (dev_cfg->data_mode == XSPI_DUAL_MODE) {
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
				data->read_opcode = read_instr.instr;
				data->read_dummy =
					(read_instr.wait_states + read_instr.mode_clocks);
			}
		}

		/* convert 3-Byte opcodes to 4-Byte (if required) */
		if (IS_ENABLED(DT_INST_PROP(0, four_byte_opcodes))) {
			if (data->address_width != 4U) {
				LOG_DBG("4-Byte opcodes require 4-Byte address width");
				return -ENOTSUP;
			}
			data->read_opcode = spi_nor_convert_read_to_4b(data->read_opcode);
			data->write_opcode = spi_nor_convert_write_to_4b(data->write_opcode);
		}

		/* enable quad mode (if required) */
		if (dev_cfg->data_mode == XSPI_QUAD_MODE) {
			if (jesd216_bfp_decode_dw15(php, bfp, &dw15) < 0) {
				/* will use QER from DTS or default (refer to device data) */
				LOG_WRN("Unable to decode QE requirement [DW15]");
			} else {
				/* bypass DTS QER value */
				data->qer_type = dw15.qer;
			}

			LOG_DBG("QE requirement mode: %x", data->qer_type);

			if (stm32_xspi_enable_qe(dev) < 0) {
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
		data->read_mode, data->read_opcode, data->read_dummy);
	LOG_DBG("Using write instr: 0x%X", data->write_opcode);

	return 0;
}

#if STM32_XSPI_USE_DMA
static int flash_stm32_xspi_dma_init(DMA_HandleTypeDef *hdma, struct stream *dma_stream)
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
#endif /* STM32_XSPI_USE_DMA */


static int flash_stm32_xspi_init(const struct device *dev)
{
	const struct flash_stm32_xspi_config *dev_cfg = dev->config;
	struct flash_stm32_xspi_data *dev_data = dev->data;
	uint32_t ahb_clock_freq;
	uint32_t prescaler = STM32_XSPI_CLOCK_PRESCALER_MIN;
	int ret;

	/* The SPI/DTR is not a valid config of data_mode/data_rate according to the DTS */
	if ((dev_cfg->data_mode != XSPI_OCTO_MODE)
		&& (dev_cfg->data_rate == XSPI_DTR_TRANSFER)) {
		/* already the right config, continue */
		LOG_ERR("XSPI mode SPI|DUAL|QUAD/DTR is not valid");
		return -ENOTSUP;
	}

	/* Signals configuration */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("XSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

#ifdef CONFIG_STM32_MEMMAP
	/* If MemoryMapped then configure skip init */
	if (stm32_xspi_is_memorymap(dev)) {
		LOG_DBG("NOR init'd in MemMapped mode\n");
		/* Force HAL instance in correct state */
		dev_data->hxspi.State = HAL_XSPI_STATE_BUSY_MEM_MAPPED;
		return 0;
	}
#endif /* CONFIG_STM32_MEMMAP */

	if (dev_cfg->pclk_len > 3) {
		/* Max 3 domain clock are expected */
		LOG_ERR("Could not select %d XSPI domain clock", dev_cfg->pclk_len);
		return -EIO;
	}

	/* Clock configuration */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t) &dev_cfg->pclken[0]) != 0) {
		LOG_ERR("Could not enable XSPI clock");
		return -EIO;
	}
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &dev_cfg->pclken[0],
					&ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken)");
		return -EIO;
	}
	/* Alternate clock config for peripheral if any */
	if (IS_ENABLED(STM32_XSPI_DOMAIN_CLOCK_SUPPORT) && (dev_cfg->pclk_len > 1)) {
		if (clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &dev_cfg->pclken[1],
				NULL) != 0) {
			LOG_ERR("Could not select XSPI domain clock");
			return -EIO;
		}
		/*
		 * Get the clock rate from this one (update ahb_clock_freq)
		 * TODO: retrieve index in the clocks property where clocks has "xspi-ker"
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
	if (IS_ENABLED(STM32_XSPI_DOMAIN_CLOCK_SUPPORT) && (dev_cfg->pclk_len > 2)) {
		if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &dev_cfg->pclken[2]) != 0) {
			LOG_ERR("Could not enable XSPI Manager clock");
			return -EIO;
		}
		/* Do NOT Get the clock rate from this one */
	}

	for (; prescaler <= STM32_XSPI_CLOCK_PRESCALER_MAX; prescaler++) {
		uint32_t clk = STM32_XSPI_CLOCK_COMPUTE(ahb_clock_freq, prescaler);

		if (clk <= dev_cfg->max_frequency) {
			break;
		}
	}
	__ASSERT_NO_MSG(prescaler >= STM32_XSPI_CLOCK_PRESCALER_MIN &&
			prescaler <= STM32_XSPI_CLOCK_PRESCALER_MAX);

	/* Initialize XSPI HAL structure completely */
	dev_data->hxspi.Init.ClockPrescaler = prescaler;
	/* The stm32 hal_xspi driver does not reduce DEVSIZE before writing the DCR1 */
	dev_data->hxspi.Init.MemorySize = find_lsb_set(dev_cfg->flash_size) - 2;
#if defined(XSPI_DCR2_WRAPSIZE)
	dev_data->hxspi.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
#endif /* XSPI_DCR2_WRAPSIZE */
	/* STR mode else Macronix for DTR mode */
	if (dev_cfg->data_rate == XSPI_DTR_TRANSFER) {
		dev_data->hxspi.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
		dev_data->hxspi.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
	} else {

	}
#if STM32_XSPI_DLYB_BYPASSED
	dev_data->hxspi.Init.DelayBlockBypass = HAL_XSPI_DELAY_BLOCK_BYPASS;
#else
	dev_data->hxspi.Init.DelayBlockBypass = HAL_XSPI_DELAY_BLOCK_ON;
#endif /* STM32_XSPI_DLYB_BYPASSED */


	if (HAL_XSPI_Init(&dev_data->hxspi) != HAL_OK) {
		LOG_ERR("XSPI Init failed");
		return -EIO;
	}

	LOG_DBG("XSPI Init'd");

#if defined(HAL_XSPIM_IOPORT_1) || defined(HAL_XSPIM_IOPORT_2)
	/* XSPI I/O manager init Function */
	XSPIM_CfgTypeDef xspi_mgr_cfg;

	if (dev_data->hxspi.Instance == XSPI1) {
		xspi_mgr_cfg.IOPort = HAL_XSPIM_IOPORT_1;
	} else if (dev_data->hxspi.Instance == XSPI2) {
		xspi_mgr_cfg.IOPort = HAL_XSPIM_IOPORT_2;
	}
	xspi_mgr_cfg.nCSOverride = HAL_XSPI_CSSEL_OVR_DISABLED;
	xspi_mgr_cfg.Req2AckTime = 1;

	if (HAL_XSPIM_Config(&dev_data->hxspi, &xspi_mgr_cfg,
		HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI M config failed");
		return -EIO;
	}

#endif /* XSPIM */

#if defined(DLYB_XSPI1) || defined(DLYB_XSPI2) || defined(DLYB_OCTOSPI1) || defined(DLYB_OCTOSPI2)
	/* XSPI delay block init Function */
	HAL_XSPI_DLYB_CfgTypeDef xspi_delay_block_cfg = {0};

	(void)HAL_XSPI_DLYB_GetClockPeriod(&dev_data->hxspi, &xspi_delay_block_cfg);
	/*  with DTR, set the PhaseSel/4 (empiric value from stm32Cube) */
	xspi_delay_block_cfg.PhaseSel /= 4;

	if (HAL_XSPI_DLYB_SetConfig(&dev_data->hxspi, &xspi_delay_block_cfg) != HAL_OK) {
		LOG_ERR("XSPI DelayBlock failed");
		return -EIO;
	}

	LOG_DBG("Delay Block Init");
#endif /* DLYB_ */

#if STM32_XSPI_USE_DMA
	/* Configure and enable the DMA channels after XSPI config */
	static DMA_HandleTypeDef hdma_tx;
	static DMA_HandleTypeDef hdma_rx;

	if (flash_stm32_xspi_dma_init(&hdma_tx, &dev_data->dma_tx) != 0) {
		LOG_ERR("XSPI DMA Tx init failed");
		return -EIO;
	}

	/* The dma_tx handle is hold by the dma_stream.cfg.user_data */
	__HAL_LINKDMA(&dev_data->hxspi, hdmatx, hdma_tx);

	if (flash_stm32_xspi_dma_init(&hdma_rx, &dev_data->dma_rx) != 0) {
		LOG_ERR("XSPI DMA Rx init failed");
		return -EIO;
	}

	/* The dma_rx handle is hold by the dma_stream.cfg.user_data */
	__HAL_LINKDMA(&dev_data->hxspi, hdmarx, hdma_rx);

#endif /* CONFIG_USE_STM32_HAL_DMA */
	/* Initialize semaphores */
	k_sem_init(&dev_data->sem, 1, 1);
	k_sem_init(&dev_data->sync, 0, 1);

	/* Run IRQ init */
	dev_cfg->irq_config(dev);

	/* Reset NOR flash memory : still with the SPI/STR config for the NOR */
	if (stm32_xspi_mem_reset(dev) != 0) {
		LOG_ERR("XSPI reset failed");
		return -EIO;
	}

	LOG_DBG("Reset Mem (SPI/STR)");

	/* Check if memory is ready in the SPI/STR mode */
	if (stm32_xspi_mem_ready(dev,
		XSPI_SPI_MODE, XSPI_STR_TRANSFER) != 0) {
		LOG_ERR("XSPI memory not ready");
		return -EIO;
	}

	LOG_DBG("Mem Ready (SPI/STR)");

#if defined(CONFIG_FLASH_JESD216_API)
	/* Process with the RDID (jedec read ID) instruction at init and fill jedec_id Table */
	ret = stm32_xspi_read_jedec_id(dev);
	if (ret != 0) {
		LOG_ERR("Read ID failed: %d", ret);
		return ret;
	}
#endif /* CONFIG_FLASH_JESD216_API */

	if (stm32_xspi_config_mem(dev) != 0) {
		LOG_ERR("OSPI mode not config'd (%u rate %u)",
			dev_cfg->data_mode, dev_cfg->data_rate);
		return -EIO;
	}

	/* Send the instruction to read the SFDP  */
	const uint8_t decl_nph = 2;
	union {
		/* We only process BFP so use one parameter block */
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u;
	const struct jesd216_sfdp_header *hp = &u.sfdp;

	ret = xspi_read_sfdp(dev, 0, u.raw, sizeof(u.raw));
	if (ret != 0) {
		LOG_ERR("SFDP read failed: %d", ret);
		return ret;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	if (magic != JESD216_SFDP_MAGIC) {
		LOG_ERR("SFDP magic %08x invalid", magic);
		return -EINVAL;
	}

	LOG_DBG("%s: SFDP v %u.%u AP %x with %u PH", dev->name,
		hp->rev_major, hp->rev_minor, hp->access, 1 + hp->nph);

	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_param_header *phpe = php +
						     MIN(decl_nph, 1 + hp->nph);

	while (php != phpe) {
		uint16_t id = jesd216_param_id(php);

		LOG_DBG("PH%u: %04x rev %u.%u: %u DW @ %x",
			(php - hp->phdr), id, php->rev_major, php->rev_minor,
			php->len_dw, jesd216_param_addr(php));

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			union {
				uint32_t dw[20];
				struct jesd216_bfp bfp;
			} u2;
			const struct jesd216_bfp *bfp = &u2.bfp;

			ret = xspi_read_sfdp(dev, jesd216_param_addr(php),
					     (uint8_t *)u2.dw,
					     MIN(sizeof(uint32_t) * php->len_dw, sizeof(u2.dw)));
			if (ret == 0) {
				ret = spi_nor_process_bfp(dev, php, bfp);
			}

			if (ret != 0) {
				LOG_ERR("SFDP BFP failed: %d", ret);
				break;
			}
		}
		if (id == JESD216_SFDP_PARAM_ID_4B_ADDR_INSTR) {

			if (dev_data->address_width == 4U) {
				/*
				 * Check table 4 byte address instruction table to get supported
				 * erase opcodes when running in 4 byte address mode
				 */
				union {
					uint32_t dw[2];
					struct {
						uint32_t dummy;
						uint8_t type[4];
					} types;
				} u2;
				ret = xspi_read_sfdp(dev, jesd216_param_addr(php),
					     (uint8_t *)u2.dw,
					     MIN(sizeof(uint32_t) * php->len_dw, sizeof(u2.dw)));
				if (ret != 0) {
					break;
				}
				for (uint8_t ei = 0; ei < JESD216_NUM_ERASE_TYPES; ++ei) {
					struct jesd216_erase_type *etp = &dev_data->erase_types[ei];
					const uint8_t cmd = u2.types.type[ei];
					/* 0xff means not supported */
					if (cmd == 0xff) {
						etp->exp = 0;
						etp->cmd = 0;
					} else {
						etp->cmd = cmd;
					};
				}
			}
		}
		++php;
	}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	ret = setup_pages_layout(dev);
	if (ret != 0) {
		LOG_ERR("layout setup failed: %d", ret);
		return -ENODEV;
	}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#ifdef CONFIG_STM32_MEMMAP
	ret = stm32_xspi_set_memorymap(dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable memory-mapped mode: %d", ret);
		return ret;
	}
	LOG_INF("Memory-mapped NOR-flash at 0x%lx (0x%x bytes)",
		(long)(STM32_XSPI_BASE_ADDRESS),
		dev_cfg->flash_size);
#else
	LOG_INF("NOR external-flash at 0x%lx (0x%x bytes)",
		(long)(STM32_XSPI_BASE_ADDRESS),
		dev_cfg->flash_size);
#endif /* CONFIG_STM32_MEMMAP*/
	return 0;
}


#if STM32_XSPI_USE_DMA
#define DMA_CHANNEL_CONFIG(node, dir)						\
		DT_DMAS_CELL_BY_NAME(node, dir, channel_config)

#define XSPI_DMA_CHANNEL_INIT(node, dir, dir_cap, src_dev, dest_dev)		\
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
		.dma_callback = xspi_dma_callback,				\
	},									\
	.src_addr_increment = STM32_DMA_CONFIG_##src_dev##_ADDR_INC(		\
				DMA_CHANNEL_CONFIG(node, dir)),			\
	.dst_addr_increment = STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(		\
				DMA_CHANNEL_CONFIG(node, dir)),

#define XSPI_DMA_CHANNEL(node, dir, DIR, src, dest)				\
	.dma_##dir = {								\
		COND_CODE_1(DT_DMAS_HAS_NAME(node, dir),			\
			(XSPI_DMA_CHANNEL_INIT(node, dir, DIR, src, dest)),	\
			(NULL))							\
		},
#else
#define XSPI_DMA_CHANNEL(node, dir, DIR, src, dest)
#endif /* CONFIG_USE_STM32_HAL_DMA */

#define XSPI_FLASH_MODULE(drv_id, flash_id)				\
		(DT_DRV_INST(drv_id), xspi_nor_flash_##flash_id)

#define DT_WRITEOC_PROP_OR(inst, default_value)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, writeoc),					\
		    (_CONCAT(SPI_NOR_CMD_, DT_STRING_TOKEN(DT_DRV_INST(inst), writeoc))),	\
		    ((default_value)))

#define DT_QER_PROP_OR(inst, default_value)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, quad_enable_requirements),			\
		    (_CONCAT(JESD216_DW15_QER_VAL_,						\
			     DT_STRING_TOKEN(DT_DRV_INST(inst), quad_enable_requirements))),	\
		    ((default_value)))

static void flash_stm32_xspi_irq_config_func(const struct device *dev);

static const struct stm32_pclken pclken[] = STM32_DT_CLOCKS(STM32_XSPI_NODE);

PINCTRL_DT_DEFINE(STM32_XSPI_NODE);

static const struct flash_stm32_xspi_config flash_stm32_xspi_cfg = {
	.pclken = pclken,
	.pclk_len = DT_NUM_CLOCKS(STM32_XSPI_NODE),
	.irq_config = flash_stm32_xspi_irq_config_func,
	.flash_size = DT_INST_REG_ADDR_BY_IDX(0, 1),
	.max_frequency = DT_INST_PROP(0, ospi_max_frequency),
	.data_mode = DT_INST_PROP(0, spi_bus_width), /* SPI or OPI */
	.data_rate = DT_INST_PROP(0, data_rate), /* DTR or STR */
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(STM32_XSPI_NODE),
#if STM32_XSPI_RESET_GPIO
	.reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
#endif /* STM32_XSPI_RESET_GPIO */
#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_ospi_nor), sfdp_bfp)
	.sfdp_bfp = DT_INST_PROP(0, sfdp_bfp),
#endif /* sfdp_bfp */
};

static struct flash_stm32_xspi_data flash_stm32_xspi_dev_data = {
	.hxspi = {
		.Instance = (XSPI_TypeDef *)DT_REG_ADDR(STM32_XSPI_NODE),
		.Init = {
			.FifoThresholdByte = STM32_XSPI_FIFO_THRESHOLD,
			.SampleShifting = (DT_PROP(STM32_XSPI_NODE, ssht_enable)
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
	.qer_type = DT_QER_PROP_OR(0, JESD216_DW15_QER_VAL_S1B6),
	.write_opcode = DT_WRITEOC_PROP_OR(0, SPI_NOR_WRITEOC_NONE),
	.page_size = SPI_NOR_PAGE_SIZE, /* by default, to be updated by sfdp */
#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_ospi_nor), jedec_id)
	.jedec_id = DT_INST_PROP(0, jedec_id),
#endif /* jedec_id */
	XSPI_DMA_CHANNEL(STM32_XSPI_NODE, tx, TX, MEMORY, PERIPHERAL)
	XSPI_DMA_CHANNEL(STM32_XSPI_NODE, rx, RX, PERIPHERAL, MEMORY)
};

static void flash_stm32_xspi_irq_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(STM32_XSPI_NODE), DT_IRQ(STM32_XSPI_NODE, priority),
		    flash_stm32_xspi_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_IRQN(STM32_XSPI_NODE));
}

DEVICE_DT_INST_DEFINE(0, &flash_stm32_xspi_init, NULL,
		      &flash_stm32_xspi_dev_data, &flash_stm32_xspi_cfg,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &flash_stm32_xspi_driver_api);
