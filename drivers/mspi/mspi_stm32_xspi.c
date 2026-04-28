/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 *
 * MSPI flash controller driver for stm32 series with multi-SPI peripherals
 * This driver is based on the stm32Cube HAL XSPI driver
 *
 */

#define DT_DRV_COMPAT st_stm32_xspi_controller

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <stm32_ll_dma.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/irq.h>
#include <stm32_bitops.h>
#include "mspi_stm32.h"

LOG_MODULE_REGISTER(mspi_stm32_xspi, CONFIG_MSPI_LOG_LEVEL);

static uint32_t mspi_stm32_xspi_hal_address_size(uint8_t address_length)
{
	if (address_length == 4U) {
		return HAL_XSPI_ADDRESS_32_BITS;
	}

	return HAL_XSPI_ADDRESS_24_BITS;
}

/**
 * @brief Gives XSPI_RegularCmdTypeDef with all parameters set except Instruction, Address, NbData
 */
static XSPI_RegularCmdTypeDef mspi_stm32_xspi_prepare_cmd(uint8_t cfg_mode, uint8_t cfg_rate)
{
	/* Command empty structure */
	XSPI_RegularCmdTypeDef cmd_tmp = {0};

	cmd_tmp.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
	cmd_tmp.InstructionWidth = (cfg_mode == MSPI_IO_MODE_OCTAL) ? HAL_XSPI_INSTRUCTION_16_BITS
								    : HAL_XSPI_INSTRUCTION_8_BITS;
	cmd_tmp.InstructionDTRMode = (cfg_rate == MSPI_DATA_RATE_DUAL)
					     ? HAL_XSPI_INSTRUCTION_DTR_ENABLE
					     : HAL_XSPI_INSTRUCTION_DTR_DISABLE;
	cmd_tmp.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
	cmd_tmp.AddressDTRMode = (cfg_rate == MSPI_DATA_RATE_DUAL) ? HAL_XSPI_ADDRESS_DTR_ENABLE
								   : HAL_XSPI_ADDRESS_DTR_DISABLE;
	cmd_tmp.DataDTRMode = (cfg_rate == MSPI_DATA_RATE_DUAL) ? HAL_XSPI_DATA_DTR_ENABLE
								: HAL_XSPI_DATA_DTR_DISABLE;
	/* AddressWidth must be set to 32bits for init and mem config phase */
	cmd_tmp.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
	cmd_tmp.DataDTRMode = (cfg_rate == MSPI_DATA_RATE_DUAL) ? HAL_XSPI_DATA_DTR_ENABLE
								: HAL_XSPI_DATA_DTR_DISABLE;
	cmd_tmp.DQSMode =
		(cfg_rate == MSPI_DATA_RATE_DUAL) ? HAL_XSPI_DQS_ENABLE : HAL_XSPI_DQS_DISABLE;
#ifdef XSPI_CCR_SIOO
	cmd_tmp.SIOOMode = HAL_XSPI_SIOO_INST_EVERY_CMD;
#endif /* XSPI_CCR_SIOO */

	switch (cfg_mode) {
	case MSPI_IO_MODE_OCTAL:
		cmd_tmp.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
		cmd_tmp.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
		cmd_tmp.DataMode = HAL_XSPI_DATA_8_LINES;
		break;

	case MSPI_IO_MODE_QUAD:
		cmd_tmp.InstructionMode = HAL_XSPI_INSTRUCTION_4_LINES;
		cmd_tmp.AddressMode = HAL_XSPI_ADDRESS_4_LINES;
		cmd_tmp.DataMode = HAL_XSPI_DATA_4_LINES;
		break;

	case MSPI_IO_MODE_DUAL:
		cmd_tmp.InstructionMode = HAL_XSPI_INSTRUCTION_2_LINES;
		cmd_tmp.AddressMode = HAL_XSPI_ADDRESS_2_LINES;
		cmd_tmp.DataMode = HAL_XSPI_DATA_2_LINES;
		break;

	default:
		cmd_tmp.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
		cmd_tmp.DataMode = HAL_XSPI_DATA_1_LINE;
		break;
	}

	return cmd_tmp;
}

/**
 * @brief Checks if the flash is currently operating in memory-mapped mode.
 */
static bool mspi_stm32_xspi_is_memorymap(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	return stm32_reg_read_bits(&dev_data->hmspi.xspi.Instance->CR, XSPI_CR_FMODE) ==
	       XSPI_CR_FMODE;
}

/**
 * @brief Sets the device back in indirect mode.
 */
static int mspi_stm32_xspi_memmap_off(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	if (HAL_XSPI_Abort(&dev_data->hmspi.xspi) != HAL_OK) {
		LOG_ERR("MemMapped abort failed: %x", dev_data->hmspi.xspi.ErrorCode);
		return -EIO;
	}
	return 0;
}

/**
 * @brief Sets the device in Memory-Mapped mode.
 */
static int mspi_stm32_xspi_memmap_on(const struct device *controller)
{
	HAL_StatusTypeDef ret;
	struct mspi_stm32_data *dev_data = controller->data;
	XSPI_MemoryMappedTypeDef s_MemMappedCfg;

	if (mspi_stm32_xspi_is_memorymap(controller)) {
		return 0;
	}

	/* Configure in MemoryMapped mode */
	if ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE) &&
	    (mspi_stm32_xspi_hal_address_size(dev_data->dev_cfg.addr_length) ==
	     HAL_XSPI_ADDRESS_24_BITS)) {
		/* OPI mode and 3-bytes address size not supported by memory */
		LOG_ERR("MSPI_IO_MODE_SINGLE in 3Bytes addressing is not supported");
		return -EIO;
	}

	XSPI_RegularCmdTypeDef s_command =
		mspi_stm32_xspi_prepare_cmd(dev_data->dev_cfg.io_mode, dev_data->dev_cfg.data_rate);

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
						   ? ((mspi_stm32_xspi_hal_address_size(
							       dev_data->ctx.xfer.addr_length) ==
						       HAL_XSPI_ADDRESS_24_BITS)
							      ? MSPI_NOR_CMD_READ_FAST
							      : MSPI_NOR_CMD_READ_FAST_4B)
						   : dev_data->dev_cfg.read_cmd)
					: MSPI_NOR_OCMD_DTR_RD;
	s_command.AddressMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
						   ? HAL_XSPI_ADDRESS_1_LINE
						   : HAL_XSPI_ADDRESS_8_LINES)
					: HAL_XSPI_ADDRESS_8_LINES;
	s_command.AddressDTRMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					   ? HAL_XSPI_ADDRESS_DTR_DISABLE
					   : HAL_XSPI_ADDRESS_DTR_ENABLE;
	s_command.AddressWidth =
		(dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
			? mspi_stm32_xspi_hal_address_size(dev_data->ctx.xfer.addr_length)
			: HAL_XSPI_ADDRESS_32_BITS;
	s_command.DataMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				     ? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
						? HAL_XSPI_DATA_1_LINE
						: HAL_XSPI_DATA_8_LINES)
				     : HAL_XSPI_DATA_8_LINES;
	s_command.DataDTRMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					? HAL_XSPI_DATA_DTR_DISABLE
					: HAL_XSPI_DATA_DTR_ENABLE;
	s_command.DummyCycles = dev_data->ctx.xfer.rx_dummy;
	s_command.DQSMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
				    ? HAL_XSPI_DQS_DISABLE
				    : HAL_XSPI_DQS_ENABLE;

#ifdef XSPI_CCR_SIOO
	s_command.SIOOMode = HAL_XSPI_SIOO_INST_EVERY_CMD;
#endif /* XSPI_CCR_SIOO */

	ret = HAL_XSPI_Command(&dev_data->hmspi.xspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to set memory mapped mode");
		return -EIO;
	}

	/* Initializes the program command */
	s_command.OperationType = HAL_XSPI_OPTYPE_WRITE_CFG;
	if (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE) {
		s_command.Instruction = (dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
						? ((mspi_stm32_xspi_hal_address_size(
							    dev_data->ctx.xfer.addr_length) ==
						    HAL_XSPI_ADDRESS_24_BITS)
							   ? MSPI_NOR_CMD_PP
							   : MSPI_NOR_CMD_PP_4B)
						: MSPI_NOR_OCMD_PAGE_PRG;
	} else {
		s_command.Instruction = MSPI_NOR_OCMD_PAGE_PRG;
	}

	s_command.DQSMode = HAL_XSPI_DQS_DISABLE;
	ret = HAL_XSPI_Command(&dev_data->hmspi.xspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to set memory mapped mode");
		return -EIO;
	}

#ifdef XSPI_CR_NOPREF
	s_MemMappedCfg.NoPrefetchData = HAL_XSPI_AUTOMATIC_PREFETCH_ENABLE;
#ifdef XSPI_CR_NOPREF_AXI
	s_MemMappedCfg.NoPrefetchAXI = HAL_XSPI_AXI_PREFETCH_DISABLE;
#endif /* XSPI_CR_NOPREF_AXI */
#endif /* XSPI_CR_NOPREF */

	/* Enables the memory-mapping */
	s_MemMappedCfg.TimeOutActivation = HAL_XSPI_TIMEOUT_COUNTER_DISABLE;
	ret = HAL_XSPI_MemoryMapped(&dev_data->hmspi.xspi, &s_MemMappedCfg);
	if (ret != HAL_OK) {
		LOG_ERR("Failed to enable memory mapped mode");
		return -EIO;
	}

	return 0;
}

static int mspi_stm32_xspi_context_lock(struct mspi_stm32_context *ctx,
					const struct mspi_xfer *xfer)
{
	if (k_sem_take(&ctx->lock, K_MSEC(xfer->timeout)) < 0) {
		return -EBUSY;
	}

	ctx->xfer = *xfer;
	ctx->packets_left = ctx->xfer.num_packet;
	return 0;
}

static void mspi_stm32_xspi_context_unlock(struct mspi_stm32_context *ctx)
{
	k_sem_give(&ctx->lock);
}

/**
 * Check if the MSPI bus is busy.
 *
 * @param controller MSPI controller device.
 * @return true The MSPI bus is busy.
 * @return false The MSPI bus is idle.
 */
static bool mspi_stm32_xspi_is_inp(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	return k_sem_count_get(&dev_data->ctx.lock) == 0;
}

static int mspi_stm32_xspi_abort_memmap_if_enabled(const struct device *dev)
{
	int ret = 0;

	if (mspi_stm32_xspi_is_memorymap(dev)) {
		ret = mspi_stm32_xspi_memmap_off(dev);
		if (ret != 0) {
			LOG_ERR("Failed to abort memory-mapped mode.");
			return ret;
		}
	}

	return ret;
}

/**
 * @brief Reads/Writes in memory mapped mode.
 *
 */
static int read_write_in_memory_map_mode(const struct device *dev, struct mspi_xfer_packet *packet)
{
	int ret;
	struct mspi_stm32_data *dev_data = dev->data;

	if (packet->data_buf == NULL) {
		LOG_ERR("data buf is null : 0x%x", packet->cmd);
		return -EIO;
	}

	if (!mspi_stm32_xspi_is_memorymap(dev)) {
		ret = mspi_stm32_xspi_memmap_on(dev);
		if (ret != 0) {
			LOG_ERR("Failed to set memory mapped");
			return ret;
		}
	}

	uintptr_t mmap_addr = dev_data->memmap_base_addr + packet->address;

	if (packet->dir == MSPI_RX) {
		LOG_INF("Memory-mapped read from 0x%08lx, len %u", mmap_addr, packet->num_bytes);
		memcpy(packet->data_buf, (void *)mmap_addr, packet->num_bytes);
		k_sleep(K_MSEC(1));
		return 0;
	}

	if (!dev_data->xip_cfg.permission) {
		LOG_INF("Memory-mapped write from 0x%08lx, len %u", mmap_addr, packet->num_bytes);
		memcpy((void *)mmap_addr, packet->data_buf, packet->num_bytes);
		k_sleep(K_MSEC(1));
		return 0;
	}

	ret = mspi_stm32_xspi_abort_memmap_if_enabled(dev);
	if (ret != 0) {
		return ret;
	}

	return -EPROTONOSUPPORT;
}

static HAL_StatusTypeDef read_write_in_indirect_mode(const struct device *dev,
						     struct mspi_xfer_packet *packet,
						     uint8_t access_mode)
{
	HAL_StatusTypeDef hal_ret;
	struct mspi_stm32_data *dev_data = dev->data;

	if (packet->dir == MSPI_RX) {
		/* Receive the data */
		switch (access_mode) {
		case MSPI_ACCESS_SYNC:
			hal_ret = HAL_XSPI_Receive(&dev_data->hmspi.xspi, packet->data_buf,
						   HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
			goto end;
		case MSPI_ACCESS_ASYNC:
			hal_ret = HAL_XSPI_Receive_IT(&dev_data->hmspi.xspi, packet->data_buf);
			break;
		case MSPI_ACCESS_DMA:
			uint8_t *dma_buf = k_aligned_alloc(CONFIG_MSPI_STM32_BUFFER_ALIGNMENT,
							   packet->num_bytes);

			if (dma_buf == NULL) {
				LOG_ERR("DMA buffer allocation failed");
				return HAL_ERROR;
			}

			hal_ret = HAL_XSPI_Receive_DMA(&dev_data->hmspi.xspi, dma_buf);
			if (hal_ret == HAL_OK) {
				if (k_sem_take(&dev_data->sync, K_FOREVER) < 0) {
					LOG_ERR("Failed to take sem");
					k_free(dma_buf);
					return HAL_BUSY;
				}
				memcpy(packet->data_buf, dma_buf, packet->num_bytes);
			}

			k_free(dma_buf);
			goto end;
		default:
			/* Not correct */
			hal_ret = HAL_BUSY;
			break;
		}
	} else {
		/* Transmit the data */
		switch (access_mode) {
		case MSPI_ACCESS_SYNC:
			hal_ret = HAL_XSPI_Transmit(&dev_data->hmspi.xspi, packet->data_buf,
						    HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
			goto end;
		case MSPI_ACCESS_ASYNC:
			hal_ret = HAL_XSPI_Transmit_IT(&dev_data->hmspi.xspi, packet->data_buf);
			break;
		case MSPI_ACCESS_DMA:
			hal_ret = HAL_XSPI_Transmit_DMA(&dev_data->hmspi.xspi, packet->data_buf);
			break;
		default:
			/* Not correct */
			hal_ret = HAL_BUSY;
			break;
		}
	}

	if (hal_ret != HAL_OK) {
		goto end;
	}

	/* Lock again expecting the IRQ for end of Tx or Rx */
	if (k_sem_take(&dev_data->sync, K_FOREVER) < 0) {
		LOG_ERR("Failed to take sem");
		return HAL_BUSY;
	}

end:
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to access data");
	}

	return hal_ret;
}
/**
 * @brief Sends a Command to the NOR and Receive/Transceive data if relevant in IT or DMA mode.
 *
 */
static int mspi_stm32_xspi_access(const struct device *dev, struct mspi_xfer_packet *packet,
				  uint8_t access_mode)
{
	HAL_StatusTypeDef hal_ret;
	struct mspi_stm32_data *dev_data = dev->data;

	if (dev_data->xip_cfg.enable) {
		if ((packet->cmd == MSPI_NOR_CMD_WREN) || (packet->cmd == MSPI_NOR_OCMD_WREN) ||
		    (packet->cmd == MSPI_NOR_CMD_SE_4B) || (packet->cmd == MSPI_NOR_OCMD_SE) ||
		    (packet->cmd == MSPI_NOR_CMD_SE) ||
		    ((mspi_stm32_xspi_hal_address_size(dev_data->dev_cfg.addr_length) ==
		      HAL_XSPI_ADDRESS_24_BITS) &&
		     (dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE))) {
			LOG_DBG(" MSPI_IO_MODE_SINGLE in 3Bytes addressing is not supported in "
				"memory map mode, switching to indirect mode");

			int ret = mspi_stm32_xspi_abort_memmap_if_enabled(dev);

			if (ret != 0) {
				return ret;
			}
			goto indirect;
		}

		if (read_write_in_memory_map_mode(dev, packet) == -EPROTONOSUPPORT) {
			goto indirect;
		}
		return 0;
	}

indirect:
	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	XSPI_RegularCmdTypeDef cmd =
		mspi_stm32_xspi_prepare_cmd(dev_data->dev_cfg.io_mode, dev_data->dev_cfg.data_rate);

	cmd.DataLength = packet->num_bytes;
	cmd.Instruction = packet->cmd;
	if (packet->dir == MSPI_TX) {
		cmd.DummyCycles = dev_data->ctx.xfer.tx_dummy;
	} else {
		cmd.DummyCycles = dev_data->ctx.xfer.rx_dummy;
	}
	cmd.Address = packet->address; /* AddressSize is 32bits in OPSI mode */
	cmd.AddressWidth = mspi_stm32_xspi_hal_address_size(dev_data->ctx.xfer.addr_length);
	if (cmd.DataLength == 0) {
		cmd.DataMode = HAL_XSPI_DATA_NONE;
	}

	if ((cmd.Instruction == MSPI_NOR_CMD_WREN) || (cmd.Instruction == MSPI_NOR_OCMD_WREN)) {
		/* Write Enable only accepts HAL_XSPI_ADDRESS_NONE */
		cmd.AddressMode = HAL_XSPI_ADDRESS_NONE;
	}

	LOG_DBG("MSPI access Instruction 0x%x", cmd.Instruction);

	hal_ret = HAL_XSPI_Command(&dev_data->hmspi.xspi, &cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
	if ((hal_ret != HAL_OK) || (packet->num_bytes == 0)) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		if (hal_ret != HAL_OK) {
			LOG_ERR("Failed to send XSPI instruction");
			return -EIO;
		}
		return 0;
	}

	hal_ret = read_write_in_indirect_mode(dev, packet, access_mode);

	/* Async path: handled in ISR callback, so skip PM release here. */
	if ((hal_ret == HAL_OK) && (access_mode == MSPI_ACCESS_ASYNC)) {
		return 0;
	}

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);

	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to access data");
		return -EIO;
	}

	return 0;
}

/* Start Automatic-Polling mode to wait until the memory is setting mask/value bit */
static int mspi_stm32_xspi_wait_auto_polling(const struct device *dev, uint8_t match_value,
					     uint8_t match_mask, uint32_t timeout_ms)
{
	struct mspi_stm32_data *dev_data = dev->data;
	XSPI_AutoPollingTypeDef s_config = {0};

	/* Set the match to check if the bit is Reset */
	s_config.MatchValue = match_value;
	s_config.MatchMask = match_mask;

	s_config.MatchMode = HAL_XSPI_MATCH_MODE_AND;
	s_config.IntervalTime = MSPI_NOR_AUTO_POLLING_INTERVAL;
	s_config.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	if (HAL_XSPI_AutoPolling_IT(&dev_data->hmspi.xspi, &s_config) != HAL_OK) {
		LOG_ERR("XSPI AutoPoll failed");
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		return -EIO;
	}

	if (k_sem_take(&dev_data->sync, K_MSEC(timeout_ms)) < 0) {
		LOG_ERR("XSPI AutoPoll wait failed");
		HAL_XSPI_Abort(&dev_data->hmspi.xspi);
		k_sem_reset(&dev_data->sync);
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		return -EIO;
	}

	return 0;
}

/*
 * Reads the status reg of the device
 * Send the RDSR command (according to io_mode/data_rate)
 * Then sets the Autopolling mode with match mask/value bit.
 */
static int mspi_stm32_xspi_status_reg(const struct device *controller, const struct mspi_xfer *xfer)
{
	int ret = 0;
	struct mspi_stm32_data *dev_data = controller->data;
	struct mspi_stm32_context *ctx = &dev_data->ctx;

	if (xfer->num_packet == 0 || xfer->packets == NULL) {
		LOG_ERR("Status Reg.: wrong parameters");
		return -EFAULT;
	}

	ret = mspi_stm32_xspi_context_lock(ctx, xfer);
	if (ret != 0) {
		return ret;
	}

	(void)pm_device_runtime_get(controller);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	XSPI_RegularCmdTypeDef cmd =
		mspi_stm32_xspi_prepare_cmd(dev_data->dev_cfg.io_mode, dev_data->dev_cfg.data_rate);

	if (dev_data->dev_cfg.io_mode == MSPI_IO_MODE_OCTAL) {
		cmd.Instruction = MSPI_NOR_OCMD_RDSR;
		cmd.DummyCycles = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_DUAL)
					  ? MSPI_NOR_DUMMY_REG_OCTAL_DTR
					  : MSPI_NOR_DUMMY_REG_OCTAL;
	} else {
		cmd.Instruction = MSPI_NOR_CMD_RDSR;
		cmd.AddressMode = HAL_XSPI_ADDRESS_NONE;
		cmd.DataMode = HAL_XSPI_DATA_1_LINE;
		cmd.DummyCycles = 0;
		cmd.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
	}
	cmd.Address = 0U;
	LOG_DBG("MSPI poll status reg");

	ret = mspi_stm32_xspi_abort_memmap_if_enabled(controller);
	if (ret != 0) {
		goto status_end;
	}

	if (HAL_XSPI_Command(&dev_data->hmspi.xspi, &cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Failed to send XSPI instruction");
		ret = -EIO;
		goto status_end;
	}

	ret = mspi_stm32_xspi_wait_auto_polling(controller, MSPI_NOR_MEM_RDY_MATCH,
						MSPI_NOR_MEM_RDY_MASK,
						HAL_XSPI_TIMEOUT_DEFAULT_VALUE);

status_end:
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(controller);

	mspi_stm32_xspi_context_unlock(ctx);
	return ret;
}

/*
 * Polls the WIP(Write In Progress) bit to become to 0
 * in cfg_mode SPI/OPI MSPI_IO_MODE_SINGLE or MSPI_IO_MODE_OCTAL
 * and cfg_rate transfer STR/DTR MSPI_DATA_RATE_SINGLE or MSPI_DATA_RATE_DUAL
 */
static int mspi_stm32_xspi_mem_ready(const struct device *dev, uint8_t cfg_mode, uint8_t cfg_rate)
{
	struct mspi_stm32_data *dev_data = dev->data;

	XSPI_RegularCmdTypeDef s_command = mspi_stm32_xspi_prepare_cmd(cfg_mode, cfg_rate);

	/* Configure automatic polling mode command to wait for memory ready */
	if (cfg_mode == MSPI_IO_MODE_OCTAL) {
		s_command.Instruction = MSPI_NOR_OCMD_RDSR;
		s_command.DummyCycles = (cfg_rate == MSPI_DATA_RATE_DUAL)
						? MSPI_NOR_DUMMY_REG_OCTAL_DTR
						: MSPI_NOR_DUMMY_REG_OCTAL;
	} else {
		s_command.Instruction = MSPI_NOR_CMD_RDSR;
		/* force 1-line InstructionMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
		s_command.AddressMode = HAL_XSPI_ADDRESS_NONE;
		/* force 1-line DataMode for any non-OSPI transfer */
		s_command.DataMode = HAL_XSPI_DATA_1_LINE;
		s_command.DummyCycles = 0;
	}
	s_command.DataLength = (cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U;
	s_command.Address = 0U;

	if (HAL_XSPI_Command(&dev_data->hmspi.xspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("MSPI AutoPoll command failed");
		return -EIO;
	}

	LOG_DBG("MSPI read status reg MemRdy");
	return mspi_stm32_xspi_wait_auto_polling(dev, MSPI_NOR_MEM_RDY_MATCH, MSPI_NOR_MEM_RDY_MASK,
						 HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
}

/* Enables writing to the memory sending a Write Enable and wait till it is effective */
static int mspi_stm32_xspi_write_enable(const struct device *dev, uint8_t cfg_mode,
					uint8_t cfg_rate)
{
	struct mspi_stm32_data *dev_data = dev->data;
	XSPI_RegularCmdTypeDef s_command = mspi_stm32_xspi_prepare_cmd(cfg_mode, cfg_rate);

	if (cfg_mode == MSPI_IO_MODE_OCTAL) {
		s_command.Instruction = MSPI_NOR_OCMD_WREN;
	} else {
		s_command.Instruction = MSPI_NOR_CMD_WREN;
		/* force 1-line InstructionMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
	}
	s_command.AddressMode = HAL_XSPI_ADDRESS_NONE;
	s_command.DataMode = HAL_XSPI_DATA_NONE;
	s_command.DummyCycles = 0U;

	if (HAL_XSPI_Command(&dev_data->hmspi.xspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("MSPI flash write enable cmd failed");
		return -EIO;
	}
	LOG_DBG("MSPI write enable");

	/* New command to Configure automatic polling mode to wait for write enabling */
	if (cfg_mode == MSPI_IO_MODE_OCTAL) {
		s_command.Instruction = MSPI_NOR_OCMD_RDSR;
		s_command.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
		s_command.DataMode = HAL_XSPI_DATA_8_LINES;
		s_command.DummyCycles = (cfg_rate == MSPI_DATA_RATE_DUAL)
						? MSPI_NOR_DUMMY_REG_OCTAL_DTR
						: MSPI_NOR_DUMMY_REG_OCTAL;
	} else {
		s_command.Instruction = MSPI_NOR_CMD_RDSR;
		/* force 1-line DataMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
		s_command.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
		s_command.DataMode = HAL_XSPI_DATA_1_LINE;
		s_command.DummyCycles = 0;
	}
	s_command.DataLength = (cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U;
	s_command.Address = 0U;

	if (HAL_XSPI_Command(&dev_data->hmspi.xspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("MSPI config auto polling cmd failed");
		return -EIO;
	}
	LOG_DBG("MSPI read status reg");

	return mspi_stm32_xspi_wait_auto_polling(dev, MSPI_NOR_WREN_MATCH, MSPI_NOR_WREN_MASK,
						 HAL_XSPI_TIMEOUT_DEFAULT_VALUE);
}

/* Writes Flash configuration register 2 with new dummy cycles */
static int mspi_stm32_xspi_write_cfg2reg_dummy(const struct device *dev, uint8_t cfg_mode,
					       uint8_t cfg_rate)
{
	int ret = 0;
	struct mspi_stm32_data *dev_data = dev->data;
	uint8_t transmit_data = MSPI_NOR_CR2_DUMMY_CYCLES_66MHZ;
	XSPI_RegularCmdTypeDef s_command = mspi_stm32_xspi_prepare_cmd(cfg_mode, cfg_rate);

	/* Initializes the writing of configuration register 2 */
	s_command.Instruction = (cfg_mode == MSPI_IO_MODE_SINGLE) ? MSPI_NOR_CMD_WR_CFGREG2
								  : MSPI_NOR_OCMD_WR_CFGREG2;
	s_command.Address = MSPI_NOR_REG2_ADDR3;
	s_command.DummyCycles = 0U;

	if (cfg_mode == MSPI_IO_MODE_SINGLE) {
		s_command.DataLength = 1U;
	} else if (cfg_rate == MSPI_DATA_RATE_DUAL) {
		s_command.DataLength = 2U;
	} else {
		s_command.DataLength = 1U;
	}

	ret = mspi_stm32_xspi_abort_memmap_if_enabled(dev);
	if (ret != 0) {
		return ret;
	}

	if (HAL_XSPI_Command(&dev_data->hmspi.xspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("MSPI transmit cmd");
		return -EIO;
	}

	if (HAL_XSPI_Transmit(&dev_data->hmspi.xspi, &transmit_data,
			      HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI transmit");
		return -EIO;
	}

	return 0;
}

/* Write Flash configuration register 2 with new single or octal SPI protocol */
static int mspi_stm32_xspi_write_cfg2reg_io(const struct device *dev, uint8_t cfg_mode,
					    uint8_t cfg_rate, uint8_t op_enable)
{
	int ret = 0;
	struct mspi_stm32_data *dev_data = dev->data;
	XSPI_RegularCmdTypeDef s_command = mspi_stm32_xspi_prepare_cmd(cfg_mode, cfg_rate);

	/* Initializes the writing of configuration register 2 */
	s_command.Instruction = (cfg_mode == MSPI_IO_MODE_SINGLE) ? MSPI_NOR_CMD_WR_CFGREG2
								  : MSPI_NOR_OCMD_WR_CFGREG2;
	s_command.Address = MSPI_NOR_REG2_ADDR1;
	s_command.DummyCycles = 0U;

	if (cfg_mode == MSPI_IO_MODE_SINGLE) {
		s_command.DataLength = 1U;
	} else if (cfg_rate == MSPI_DATA_RATE_DUAL) {
		s_command.DataLength = 2U;
	} else {
		s_command.DataLength = 1U;
	}

	ret = mspi_stm32_xspi_abort_memmap_if_enabled(dev);
	if (ret != 0) {
		return ret;
	}

	if (HAL_XSPI_Command(&dev_data->hmspi.xspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	if (HAL_XSPI_Transmit(&dev_data->hmspi.xspi, &op_enable, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	return ret;
}

/* Reads Flash configuration register 2 with new single or octal SPI protocol */
static int mspi_stm32_xspi_read_cfg2reg(const struct device *dev, uint8_t cfg_mode,
					uint8_t cfg_rate, uint8_t *value)
{
	int ret = 0;
	struct mspi_stm32_data *dev_data = dev->data;
	XSPI_RegularCmdTypeDef s_command = mspi_stm32_xspi_prepare_cmd(cfg_mode, cfg_rate);

	/* Initializes the writing of configuration register 2 */
	s_command.Instruction = (cfg_mode == MSPI_IO_MODE_SINGLE) ? MSPI_NOR_CMD_RD_CFGREG2
								  : MSPI_NOR_OCMD_RD_CFGREG2;
	s_command.Address = MSPI_NOR_REG2_ADDR1;

	if (cfg_mode == MSPI_IO_MODE_SINGLE) {
		s_command.DummyCycles = 0U;
	} else if (cfg_rate == MSPI_DATA_RATE_DUAL) {
		s_command.DummyCycles = MSPI_NOR_DUMMY_REG_OCTAL_DTR;
	} else {
		s_command.DummyCycles = MSPI_NOR_DUMMY_REG_OCTAL;
	}

	s_command.DataLength = (cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U;

	ret = mspi_stm32_xspi_abort_memmap_if_enabled(dev);
	if (ret != 0) {
		return ret;
	}

	if (HAL_XSPI_Command(&dev_data->hmspi.xspi, &s_command, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Read Flash configuration reg2 failed");
		return -EIO;
	}

	if (HAL_XSPI_Receive(&dev_data->hmspi.xspi, value, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Read Flash configuration reg2 failed");
		return -EIO;
	}

	return ret;
}

/* Sends the command to configure the device according to the DTS */
static int mspi_stm32_xspi_config_mem(const struct device *dev, uint8_t cfg_mode, uint8_t cfg_rate)
{
	struct mspi_stm32_data *dev_data = dev->data;
	uint8_t reg[2];

	/* MSPI_IO_MODE_SINGLE/MSPI_DATA_RATE_SINGLE is already done */
	if ((cfg_mode == MSPI_IO_MODE_SINGLE) && (cfg_rate == MSPI_DATA_RATE_SINGLE)) {
		return 0;
	}

	/* Write Configuration register 2 (with new dummy cycles) */
	if (mspi_stm32_xspi_write_cfg2reg_dummy(dev, MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE) !=
	    0) {
		LOG_ERR("XSPI write CFGR2 failed");
		return -EIO;
	}
	if (mspi_stm32_xspi_mem_ready(dev, MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE) != 0) {
		LOG_ERR("XSPI autopolling failed");
		return -EIO;
	}
	if (mspi_stm32_xspi_write_enable(dev, MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE) != 0) {
		LOG_ERR("XSPI write Enable 2 failed");
		return -EIO;
	}

	/* Write Configuration register 2 (with Octal I/O SPI protocol : choose STR or DTR) */
	uint8_t mode_enable =
		(cfg_rate == MSPI_DATA_RATE_DUAL) ? MSPI_NOR_CR2_DTR_OPI_EN :
						    MSPI_NOR_CR2_STR_OPI_EN;

	if (mspi_stm32_xspi_write_cfg2reg_io(dev, MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE,
					     mode_enable) != 0) {
		LOG_ERR("XSPI write CFGR2 failed");
		return -EIO;
	}

	/* Wait till the configuration is effective and check that memory is ready */
	k_busy_wait(MSPI_STM32_WRITE_REG_MAX_TIME * USEC_PER_MSEC);

	/* Reconfigure the memory type of the peripheral */
	dev_data->hmspi.xspi.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
	dev_data->hmspi.xspi.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
	if (HAL_XSPI_Init(&dev_data->hmspi.xspi) != HAL_OK) {
		LOG_ERR("XSPI mem type MACRONIX failed");
		return -EIO;
	}

	if (mspi_stm32_xspi_mem_ready(dev, MSPI_IO_MODE_OCTAL, cfg_rate) != 0) {
		LOG_ERR("XSPI flash busy failed");
		return -EIO;
	}

	if (mspi_stm32_xspi_read_cfg2reg(dev, MSPI_IO_MODE_OCTAL, cfg_rate, reg) != 0) {
		LOG_ERR("MSPI flash config read failed");
		return -EIO;
	}

	LOG_DBG("XSPI flash config is OCTO / %s",
		cfg_rate == MSPI_DATA_RATE_SINGLE ? "STR" : "DTR");

	return 0;
}

static void mspi_stm32_xspi_isr(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	HAL_XSPI_IRQHandler(&dev_data->hmspi.xspi);

	k_sem_give(&dev_data->sync);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);
}

#if !defined(CONFIG_SOC_SERIES_STM32H7X)
/* weak function required for HAL compilation */
__weak HAL_StatusTypeDef HAL_DMA_Abort_IT(DMA_HandleTypeDef *hdma)
{
	ARG_UNUSED(hdma);
	return HAL_OK;
}

/* weak function required for HAL compilation */
__weak HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef *hdma)
{
	ARG_UNUSED(hdma);
	return HAL_OK;
}
#endif /* !CONFIG_SOC_SERIES_STM32H7X */

#ifdef CONFIG_MSPI_DMA
static __maybe_unused void mspi_stm32_xspi_dma_callback(const struct device *dev, void *arg,
							uint32_t channel, int status)
{
	DMA_HandleTypeDef *hdma = arg;

	ARG_UNUSED(dev);

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d", channel);
	}

	HAL_DMA_IRQHandler(hdma);
}
#endif

static int mspi_stm32_xspi_validate_freq(uint32_t freq, uint32_t max_freq)
{
	if (freq > max_freq) {
		LOG_ERR("%u, freq is too large", __LINE__);
		return -ENOTSUP;
	}
	return 0;
}

static int mspi_stm32_xspi_validate_io_mode(uint32_t io_mode)
{
	if (io_mode >= MSPI_IO_MODE_MAX) {
		LOG_ERR("%u, Invalid io_mode", __LINE__);
		return -EINVAL;
	}
	return 0;
}

static int mspi_stm32_xspi_validate_data_rate(uint32_t data_rate)
{
	if (data_rate >= MSPI_DATA_RATE_MAX) {
		LOG_ERR("%u, Invalid data_rate", __LINE__);
		return -EINVAL;
	}
	return 0;
}

static int mspi_stm32_xspi_validate_cpp(uint32_t cpp)
{
	if (cpp > MSPI_CPP_MODE_3) {
		LOG_ERR("%u, Invalid cpp", __LINE__);
		return -EINVAL;
	}
	return 0;
}

static int mspi_stm32_xspi_validate_endian(uint32_t endian)
{
	if (endian > MSPI_XFER_BIG_ENDIAN) {
		LOG_ERR("%u, Invalid endian", __LINE__);
		return -EINVAL;
	}
	return 0;
}

static int mspi_stm32_xspi_validate_ce_polarity(uint32_t ce_polarity)
{
	if (ce_polarity > MSPI_CE_ACTIVE_HIGH) {
		LOG_ERR("%u, Invalid ce_polarity", __LINE__);
		return -EINVAL;
	}
	return 0;
}

static int mspi_stm32_xspi_validate_dqs(bool dqs_enable, bool dqs_support)
{
	if (dqs_enable && !dqs_support) {
		LOG_ERR("%u, DQS mode not supported", __LINE__);
		return -ENOTSUP;
	}
	return 0;
}

static int mspi_stm32_assign_cfg(struct mspi_stm32_data *data,
				 const enum mspi_dev_cfg_mask param_mask,
				 const struct mspi_dev_cfg *dev_cfg,
				 const struct mspi_stm32_conf *cfg)
{
	int ret = 0;

	if ((param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) != 0) {
		data->dev_cfg.rx_dummy = dev_cfg->rx_dummy;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) != 0) {
		data->dev_cfg.tx_dummy = dev_cfg->tx_dummy;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_READ_CMD) != 0) {
		data->dev_cfg.read_cmd = dev_cfg->read_cmd;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_WRITE_CMD) != 0) {
		data->dev_cfg.write_cmd = dev_cfg->write_cmd;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) != 0) {
		data->dev_cfg.cmd_length = dev_cfg->cmd_length;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) != 0) {
		data->dev_cfg.addr_length = dev_cfg->addr_length;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) != 0) {
		data->dev_cfg.mem_boundary = dev_cfg->mem_boundary;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) != 0) {
		data->dev_cfg.time_to_break = dev_cfg->time_to_break;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_CE_POL) != 0) {
		ret = mspi_stm32_xspi_validate_ce_polarity(dev_cfg->ce_polarity);
		if (ret == 0) {
			goto end;
		}
		data->dev_cfg.ce_polarity = dev_cfg->ce_polarity;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_DQS) != 0) {
		ret = mspi_stm32_xspi_validate_dqs(dev_cfg->dqs_enable, cfg->mspicfg.dqs_support);
		if (ret != 0) {
			goto end;
		}
		data->dev_cfg.dqs_enable = dev_cfg->dqs_enable;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_ENDIAN) != 0) {
		ret = mspi_stm32_xspi_validate_endian(dev_cfg->endian);
		if (ret != 0) {
			goto end;
		}
		data->dev_cfg.endian = dev_cfg->endian;
	}
end:
	return ret;
}

/**
 * Check and save dev_cfg to controller data->dev_cfg.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param param_mask Macro definition of what to be configured in cfg.
 * @param dev_cfg The device runtime configuration for the MSPI controller.
 * @return 0 MSPI device configuration successful.
 * @return A negative errno value upon MSPI device configuration failure.
 */
static int mspi_stm32_xspi_dev_cfg_save(const struct device *controller,
					const enum mspi_dev_cfg_mask param_mask,
					const struct mspi_dev_cfg *dev_cfg)
{
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;
	int ret = 0;

	if ((param_mask & MSPI_DEVICE_CONFIG_CE_NUM) != 0) {
		data->dev_cfg.ce_num = dev_cfg->ce_num;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) != 0) {
		ret = mspi_stm32_xspi_validate_freq(dev_cfg->freq, cfg->mspicfg.max_freq);
		if (ret != 0) {
			goto end;
		}
		data->dev_cfg.freq = dev_cfg->freq;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0) {
		ret = mspi_stm32_xspi_validate_io_mode(dev_cfg->io_mode);
		if (ret != 0) {
			goto end;
		}
		data->dev_cfg.io_mode = dev_cfg->io_mode;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) != 0) {
		ret = mspi_stm32_xspi_validate_data_rate(dev_cfg->data_rate);
		if (ret != 0) {
			goto end;
		}
		data->dev_cfg.data_rate = dev_cfg->data_rate;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CPP) != 0) {
		ret = mspi_stm32_xspi_validate_cpp(dev_cfg->cpp);
		if (ret != 0) {
			goto end;
		}
		data->dev_cfg.cpp = dev_cfg->cpp;
	}

	mspi_stm32_assign_cfg(data, param_mask, dev_cfg, cfg);
end:
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
 * @retval A negative errno value upon failure.
 */
static int mspi_stm32_xspi_dev_config(const struct device *controller,
				      const struct mspi_dev_id *dev_id,
				      const enum mspi_dev_cfg_mask param_mask,
				      const struct mspi_dev_cfg *dev_cfg)
{
	int ret = 0;
	bool locked = false;
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;

	if (data->dev_id != dev_id) {
		if (k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE))) {
			LOG_ERR("MSPI config failed to access controller");
			return -EBUSY;
		}

		locked = true;
	}

	if (mspi_stm32_xspi_is_inp(controller)) {
		if (locked) {
			k_mutex_unlock(&data->lock);
		}
		return -EBUSY;
	}

	if (param_mask == MSPI_DEVICE_CONFIG_NONE && !cfg->mspicfg.sw_multi_periph) {
		data->dev_id = dev_id;
		if (locked) {
			k_mutex_unlock(&data->lock);
		}
		return 0;
	}

	(void)pm_device_runtime_get(controller);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	if (param_mask & (MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_DATA_RATE)) {
		/* Going to set the XSPI mode and transfer rate */
		ret = mspi_stm32_xspi_config_mem(controller, dev_cfg->io_mode, dev_cfg->data_rate);
		if (ret != 0) {
			goto e_return;
		}
		LOG_DBG("MSPI confg'd in %d / %d", dev_cfg->io_mode, dev_cfg->data_rate);
	}

	data->dev_id = dev_id;
	/* Go on with other parameters if supported */
	if (mspi_stm32_xspi_dev_cfg_save(controller, param_mask, dev_cfg) != 0) {
		LOG_ERR("failed to change device cfg");
		ret = -EIO;
	}

e_return:
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(controller);
	if (locked) {
		k_mutex_unlock(&data->lock);
	}
	return ret;
}

/**
 * API implementation of mspi_xip_config : XIP configuration
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param xip_cfg The controller XIP configuration for MSPI.
 *
 * @retval 0 if successful.
 * @retval A negative errno value upon failure.
 */
static int mspi_stm32_xspi_xip_config(const struct device *controller,
				      const struct mspi_dev_id *dev_id,
				      const struct mspi_xip_cfg *xip_cfg)
{
	int ret = 0;
	struct mspi_stm32_data *dev_data = controller->data;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("dev_id don't match");
		return -ESTALE;
	}
	(void)pm_device_runtime_get(controller);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	if (!xip_cfg->enable) {
		/* This is for aborting */
		ret = mspi_stm32_xspi_memmap_off(controller);
	} else {
		ret = mspi_stm32_xspi_memmap_on(controller);
	}

	if (ret == 0) {
		dev_data->xip_cfg = *xip_cfg;
		LOG_INF("XIP configured %d", xip_cfg->enable);
	}

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(controller);
	return ret;
}

/**
 * API implementation of mspi_get_channel_status.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param ch Not used.
 *
 * @retval 0 if successful.
 * @retval A negative errno value upon failure.
 */
static int mspi_stm32_xspi_get_channel_status(const struct device *controller, uint8_t ch)
{
	struct mspi_stm32_data *dev_data = controller->data;
	int ret = 0;

	ARG_UNUSED(ch);

	if (mspi_stm32_xspi_is_inp(controller) ||
	    (HAL_XSPI_GET_FLAG(&dev_data->hmspi.xspi, HAL_XSPI_FLAG_BUSY) == SET)) {
		ret = -EBUSY;
	}

	return ret;
}

static int mspi_stm32_xspi_pio_dma_transceive(const struct device *controller,
					      const struct mspi_xfer *xfer)
{
	int ret = 0;
	uint32_t packet_idx;
	struct mspi_stm32_data *dev_data = controller->data;
	struct mspi_stm32_context *ctx = &dev_data->ctx;
	struct mspi_xfer_packet *packet;

	if (xfer->num_packet == 0 || xfer->packets == NULL ||
	    xfer->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		LOG_ERR("Transfer: wrong parameters");
		return -EFAULT;
	}

	ret = mspi_stm32_xspi_context_lock(ctx, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to lock MSPI context");
		return ret;
	}

	while (ctx->packets_left > 0) {
		packet_idx = ctx->xfer.num_packet - ctx->packets_left;
		packet = (const struct mspi_xfer_packet *)&ctx->xfer.packets[packet_idx];
#ifdef CONFIG_MSPI_DMA
		const struct mspi_stm32_conf *dev_cfg = controller->config;

		if (dev_cfg->dma_specified) {
			ret = mspi_stm32_xspi_access(controller, packet, MSPI_ACCESS_DMA);
		} else {
			LOG_ERR("DMA configuration is missing from the device tree");
			ret = -EIO;
			goto end;
		}
#else
		ret = mspi_stm32_xspi_access(controller, packet,
					     ctx->xfer.async ? MSPI_ACCESS_ASYNC :
							       MSPI_ACCESS_SYNC);
#endif

		ctx->packets_left--;
		if (ret != 0) {
			ret = -EIO;
			goto end;
		}
	}

end:
	mspi_stm32_xspi_context_unlock(ctx);
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
 * @retval A negative errno value upon failure.
 */
static int mspi_stm32_xspi_transceive(const struct device *controller,
				      const struct mspi_dev_id *dev_id,
				      const struct mspi_xfer *xfer)
{
	int ret = 0;
	struct mspi_stm32_data *dev_data = controller->data;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("transceive : dev_id don't match");
		return -ESTALE;
	}

	/* Need to map the xfer to the data context */
	dev_data->ctx.xfer = *xfer;

	if ((xfer->packets->cmd == MSPI_NOR_OCMD_RDSR) ||
	    (xfer->packets->cmd == MSPI_NOR_CMD_RDSR)) {
		ret = mspi_stm32_xspi_status_reg(controller, xfer);
	} else {
		ret = mspi_stm32_xspi_pio_dma_transceive(controller, xfer);
	}

	return ret;
}

static int mspi_stm32_xspi_dma_init(DMA_HandleTypeDef *hdma, struct stm32_stream *dma_stream)
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

	dma_stream->cfg.user_data = hdma;
	/* This field is used to inform driver that it is overridden */
	dma_stream->cfg.linked_channel = STM32_DMA_HAL_OVERRIDE;
	ret = dma_config(dma_stream->dev, dma_stream->channel, &dma_stream->cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d", dma_stream->channel);
		return ret;
	}

	/* Proceed to the HAL DMA driver init */
	if (dma_stream->cfg.source_data_size != dma_stream->cfg.dest_data_size) {
		LOG_ERR("DMA Source and destination data sizes not aligned");
		return -EINVAL;
	}

	hdma->Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
	hdma->Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
	hdma->Init.SrcInc =
		(dma_stream->src_addr_increment) ? DMA_SINC_INCREMENTED : DMA_SINC_FIXED;
	hdma->Init.DestInc =
		(dma_stream->dst_addr_increment) ? DMA_DINC_INCREMENTED : DMA_DINC_FIXED;
	hdma->Init.SrcBurstLength = 4;
	hdma->Init.DestBurstLength = 4;
	hdma->Init.Priority = mspi_stm32_table_priority[dma_stream->cfg.channel_priority];
	hdma->Init.Direction = mspi_stm32_table_direction[dma_stream->cfg.channel_direction];
	hdma->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_SRC_ALLOCATED_PORT1;
	hdma->Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
	hdma->Init.Mode = DMA_NORMAL;
	hdma->Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
	hdma->Init.Request = dma_stream->cfg.dma_slot;

	/*
	 * HAL expects a valid DMA channel.
	 * The channel is from 0 to 7 because of the STM32_DMA_STREAM_OFFSET
	 * in the dma_stm32 driver
	 */
	hdma->Instance = LL_DMA_GET_CHANNEL_INSTANCE(dma_stream->reg, dma_stream->channel);

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

static int mspi_validate_config(const struct mspi_cfg *config, uint32_t max_frequency)
{
	if (config->op_mode != MSPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only support MSPI controller mode");
		return -ENOTSUP;
	}

	if (config->max_freq > max_frequency) {
		LOG_ERR("Max_freq %d too large", config->max_freq);
		return -ENOTSUP;
	}

	if (config->duplex != MSPI_HALF_DUPLEX) {
		LOG_ERR("Only support half duplex mode");
		return -ENOTSUP;
	}

	if (config->num_periph > MSPI_MAX_DEVICE) {
		LOG_ERR("Invalid MSPI peripheral number");
		return -ENOTSUP;
	}

	return 0;
}

static int mspi_stm32_xspi_activate(const struct device *dev)
{
	int ret;
	const struct mspi_stm32_conf *config = (struct mspi_stm32_conf *)dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (clock_control_on(clk, (clock_control_subsys_t)(uintptr_t)&config->pclken[0]) != 0) {
		return -EIO;
	}

	return 0;
}

static int mspi_hal_init(const struct mspi_stm32_conf *dev_cfg, struct mspi_stm32_data *dev_data,
			 uint32_t ahb_clock_freq)
{
	uint32_t prescaler = MSPI_STM32_CLOCK_PRESCALER_MIN;

	for (; prescaler <= MSPI_STM32_CLOCK_PRESCALER_MAX; prescaler++) {
		dev_data->dev_cfg.freq = MSPI_STM32_CLOCK_COMPUTE(ahb_clock_freq, prescaler);
		if (dev_data->dev_cfg.freq <= dev_cfg->mspicfg.max_freq) {
			break;
		}
	}
	__ASSERT_NO_MSG(prescaler <= MSPI_STM32_CLOCK_PRESCALER_MAX);

	dev_data->hmspi.xspi.Init.ClockPrescaler = prescaler;

#if defined(XSPI_DCR2_WRAPSIZE)
	dev_data->hmspi.xspi.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
#endif

	if (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_DUAL) {
		dev_data->hmspi.xspi.Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
		dev_data->hmspi.xspi.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE;
	} else {
		dev_data->hmspi.xspi.Init.MemoryType = HAL_XSPI_MEMTYPE_MICRON;
		dev_data->hmspi.xspi.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_DISABLE;
	}

#if defined(XSPI_DCR1_DLYBYP)
	dev_data->hmspi.xspi.Init.DelayBlockBypass = HAL_XSPI_DELAY_BLOCK_ON;
#endif /* XSPI_DCR1_DLYBYP */

	if (HAL_XSPI_Init(&dev_data->hmspi.xspi) != HAL_OK) {
		LOG_ERR("MSPI Init failed");
		return -EIO;
	}

	LOG_DBG("MSPI Init'd");
	return 0;
}

static __maybe_unused int mspi_dma_setup(const struct mspi_stm32_conf *dev_cfg,
					 struct mspi_stm32_data *dev_data)
{
	if (!dev_cfg->dma_specified) {
		LOG_ERR("DMA configuration is missing from the device tree");
		return -EIO;
	}

	if (mspi_stm32_xspi_dma_init(&dev_data->hdma_tx, &dev_data->dma_tx) != 0) {
		LOG_ERR("XSPI DMA Tx init failed");
		return -EIO;
	}
	__HAL_LINKDMA(&dev_data->hmspi.xspi, hdmatx, dev_data->hdma_tx);

	if (mspi_stm32_xspi_dma_init(&dev_data->hdma_rx, &dev_data->dma_rx) != 0) {
		LOG_ERR("XSPI DMA Rx init failed");
		return -EIO;
	}
	__HAL_LINKDMA(&dev_data->hmspi.xspi, hdmarx, dev_data->hdma_rx);

	return 0;
}

/**
 * API implementation of mspi_config : controller configuration.
 *
 * @param spec Pointer to MSPI device tree spec.
 * @return 0 if successful.
 * @return A negative errno value upon failure.
 */
static int mspi_stm32_xspi_config(const struct mspi_dt_spec *spec)
{
	int ret;
	const struct mspi_cfg *config = &spec->config;
	const struct mspi_stm32_conf *dev_cfg = spec->bus->config;
	struct mspi_stm32_data *dev_data = spec->bus->data;
	uint32_t ahb_clock_freq;

	ret = mspi_validate_config(config, dev_cfg->mspicfg.max_freq);
	if (ret != 0) {
		return ret;
	}

	(void)pm_device_runtime_get(spec->bus);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	/* pinctrl */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("MSPI pinctrl setup failed");
		goto end;
	}

	if (dev_data->dev_cfg.dqs_enable && !dev_cfg->mspicfg.dqs_support) {
		LOG_ERR("MSPI dqs mismatch (not supported but enabled)");
		ret = -ENOTSUP;
		goto end;
	}

	dev_cfg->irq_config();

	ret = mspi_stm32_xspi_activate(spec->bus);
	if (ret != 0) {
		goto end;
	}

	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				   (clock_control_subsys_t)&dev_cfg->pclken[0],
				   &ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken)");
		ret = -EIO;
		goto end;
	}

	ret = mspi_hal_init(dev_cfg, dev_data, ahb_clock_freq);
	if (ret != 0) {
		goto end;
	}

#if defined(HAL_XSPIM_IOPORT_1) || defined(HAL_XSPIM_IOPORT_2)
	/* XSPI I/O manager config */
	XSPIM_CfgTypeDef mspi_mgr_cfg;

	if (dev_data->hmspi.xspi.Instance == XSPI1) {
		mspi_mgr_cfg.IOPort = HAL_XSPIM_IOPORT_1;
	}
	if (dev_data->hmspi.xspi.Instance == XSPI2) {
		mspi_mgr_cfg.IOPort = HAL_XSPIM_IOPORT_2;
	}
	mspi_mgr_cfg.nCSOverride = HAL_XSPI_CSSEL_OVR_DISABLED;
	mspi_mgr_cfg.Req2AckTime = 1;
	if (HAL_XSPIM_Config(&dev_data->hmspi.xspi, &mspi_mgr_cfg,
			     HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI M config failed");
		ret = -EIO;
		goto end;
	}
#endif

#if defined(DLYB_XSPI1) || defined(DLYB_XSPI2) || defined(DLYB_OCTOSPI1) || defined(DLYB_OCTOSPI2)
	HAL_XSPI_DLYB_CfgTypeDef mspi_delay_block_cfg = {0};

	(void)HAL_XSPI_DLYB_GetClockPeriod(&dev_data->hmspi.xspi, &mspi_delay_block_cfg);
	mspi_delay_block_cfg.PhaseSel /= 4;
	if (HAL_XSPI_DLYB_SetConfig(&dev_data->hmspi.xspi, &mspi_delay_block_cfg) != HAL_OK) {
		LOG_ERR("XSPI DelayBlock failed");
		ret = -EIO;
		goto end;
	}
	LOG_DBG("Delay Block Init");
#endif

#ifdef CONFIG_MSPI_DMA
	ret = mspi_dma_setup(dev_cfg, dev_data);
	if (ret != 0) {
		goto end;
	}
#endif

	if (k_sem_count_get(&dev_data->ctx.lock) == 0) {
		k_sem_give(&dev_data->ctx.lock);
	}
	if (config->re_init) {
		k_mutex_unlock(&dev_data->lock);
	}

end:
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(spec->bus);

	if (ret == 0) {
		LOG_INF("MSPI configured");
	}

	return ret;
}

/**
 * Set up and config a new controller.
 *
 * @param dev MSPI controller.
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

	return mspi_stm32_xspi_config(&spec);
}

static DEVICE_API(mspi, mspi_stm32_driver_api) = {
	.config = mspi_stm32_xspi_config,
	.dev_config = mspi_stm32_xspi_dev_config,
	.xip_config = mspi_stm32_xspi_xip_config,
	.get_channel_status = mspi_stm32_xspi_get_channel_status,
	.transceive = mspi_stm32_xspi_transceive,
};

#ifdef CONFIG_PM_DEVICE
static int mspi_stm32_xspi_suspend(const struct device *dev)
{
	int ret;
	const struct mspi_stm32_conf *cfg = (struct mspi_stm32_conf *)dev->config;
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	/* Disable device clock. */
	ret = clock_control_off(clk, (clock_control_subsys_t)(uintptr_t)&cfg->pclken[0]);
	if (ret < 0) {
		LOG_ERR("Failed to disable MSPI clock during PM suspend process");
		return ret;
	}

	/* Move pins to sleep state */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
	if (ret == -ENOENT) {
		/* Warn but don't block suspend */
		LOG_WRN_ONCE("MSPI pinctrl sleep state not available");
		return 0;
	}

	return ret;
}

static int mspi_stm32_xspi_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return mspi_stm32_xspi_activate(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return mspi_stm32_xspi_suspend(dev);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

#define DMA_CHANNEL_CONFIG(node, dir) DT_DMAS_CELL_BY_NAME(node, dir, channel_config)

#define XSPI_DMA_CHANNEL_INIT(node, dir, dir_cap, src_dev, dest_dev)                              \
	.dev = DEVICE_DT_GET(DT_DMAS_CTLR(node)),                                                 \
	.channel = DT_DMAS_CELL_BY_NAME(node, dir, channel),                                      \
	.reg = (DMA_TypeDef *)DT_REG_ADDR(DT_PHANDLE_BY_NAME(node, dmas, dir)),                   \
	.cfg = {                                                                                  \
		.dma_slot = DT_DMAS_CELL_BY_NAME(node, dir, slot),                                \
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(DMA_CHANNEL_CONFIG(node, dir)),   \
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(DMA_CHANNEL_CONFIG(node, dir)),     \
		.dma_callback = mspi_stm32_xspi_dma_callback,                                     \
	},                                                                                        \
	.src_addr_increment =                                                                     \
		STM32_DMA_CONFIG_##src_dev##_ADDR_INC(DMA_CHANNEL_CONFIG(node, dir)),             \
	.dst_addr_increment =                                                                     \
		STM32_DMA_CONFIG_##dest_dev##_ADDR_INC(DMA_CHANNEL_CONFIG(node, dir)),

#define XSPI_DMA_CHANNEL(node, dir, DIR, src, dest)                                               \
	.dma_##dir = {                                                                            \
		COND_CODE_1(DT_DMAS_HAS_NAME(node, dir),                                          \
			(XSPI_DMA_CHANNEL_INIT(node, dir, DIR, src, dest)),                       \
			(NULL))                                                                   \
	},

/* MSPI control config */
#define MSPI_CONFIG(index)                                                                        \
	{                                                                                         \
		.channel_num = 0,                                                                 \
		.op_mode = DT_INST_ENUM_IDX_OR(index, op_mode, MSPI_OP_MODE_CONTROLLER),          \
		.duplex = DT_INST_ENUM_IDX_OR(index, duplex, MSPI_HALF_DUPLEX),                   \
		.max_freq = DT_INST_PROP(index, clock_frequency),                                 \
		.dqs_support = DT_INST_PROP(index, dqs_support),                                  \
		.num_periph = DT_INST_CHILD_NUM(index),                                           \
		.sw_multi_periph = DT_INST_PROP(index, software_multiperipheral),                 \
	}

#define STM32_SMPI_IRQ_HANDLER(index)                                                             \
	static void mspi_stm32_irq_config_func_##index(void)                                      \
	{                                                                                         \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),                    \
			    mspi_stm32_xspi_isr, DEVICE_DT_INST_GET(index), 0);                   \
		irq_enable(DT_INST_IRQN(index));                                                  \
	}

#define MSPI_STM32_INIT(index)                                                                    \
	static const struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);          \
	                                                                                          \
	PINCTRL_DT_INST_DEFINE(index);                                                            \
	                                                                                          \
	static struct gpio_dt_spec ce_gpios##index[] = MSPI_CE_GPIOS_DT_SPEC_INST_GET(index);     \
	                                                                                          \
	STM32_SMPI_IRQ_HANDLER(index)                                                             \
	                                                                                          \
	static const struct mspi_stm32_conf mspi_stm32_dev_conf_##index = {                       \
		.pclken = pclken_##index,                                                         \
		.pclk_len = DT_INST_NUM_CLOCKS(index),                                            \
		.irq_config = mspi_stm32_irq_config_func_##index,                                 \
		.mspicfg = MSPI_CONFIG(index),                                                    \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                    \
		.mspicfg.num_ce_gpios = ARRAY_SIZE(ce_gpios##index),                              \
		.dma_specified = DT_INST_NODE_HAS_PROP(index, dmas),                              \
	};                                                                                        \
	static struct mspi_stm32_data mspi_stm32_dev_data_##index = {                             \
		.hmspi.xspi = {                                                                   \
			.Instance = (XSPI_TypeDef *)DT_INST_REG_ADDR(index),                      \
			.Init = {                                                                 \
				.FifoThresholdByte = MSPI_STM32_FIFO_THRESHOLD,                   \
				.SampleShifting =                                                 \
						(DT_INST_PROP(index, st_ssht_enable)              \
						? HAL_XSPI_SAMPLE_SHIFT_HALFCYCLE                 \
						: HAL_XSPI_SAMPLE_SHIFT_NONE),                    \
				.ChipSelectHighTimeCycle = 1,                                     \
				.ClockMode = HAL_XSPI_CLOCK_MODE_0,                               \
				.ChipSelectBoundary = 0,                                          \
				.MemoryMode = HAL_XSPI_SINGLE_MEM,                                \
				.MemorySize = 0x19,                                               \
				.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE,                  \
			},                                                                        \
		},                                                                                \
		.memmap_base_addr = DT_INST_REG_ADDR_BY_IDX(index, 1),                            \
		.dev_id = index,                                                                  \
		.lock = Z_MUTEX_INITIALIZER(mspi_stm32_dev_data_##index.lock),                    \
		.sync = Z_SEM_INITIALIZER(mspi_stm32_dev_data_##index.sync, 0, 1),                \
		.dev_cfg = {0},                                                                   \
		.xip_cfg = {0},                                                                   \
		.ctx.lock = Z_SEM_INITIALIZER(mspi_stm32_dev_data_##index.ctx.lock, 0, 1),        \
		XSPI_DMA_CHANNEL(DT_DRV_INST(index), tx, TX, MEMORY, PERIPHERAL)                  \
		XSPI_DMA_CHANNEL(DT_DRV_INST(index), rx, RX, PERIPHERAL, MEMORY)                  \
	};                                                                                        \
                                                                                                  \
	PM_DEVICE_DT_INST_DEFINE(index, mspi_stm32_xspi_pm_action);                               \
	DEVICE_DT_INST_DEFINE(index, &mspi_stm32_init, PM_DEVICE_DT_INST_GET(index),              \
			      &mspi_stm32_dev_data_##index, &mspi_stm32_dev_conf_##index,         \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                    \
			      &mspi_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_STM32_INIT)
