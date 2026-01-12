/*
 * Copyright (c) 2025 EXALT Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * **************************************************************************
 * MSPI flash controller driver for stm32 series with multi-SPI peripherals
 * This driver is based on the stm32Cube HAL OSPI driver
 * with one mspi DTS node.
 * **************************************************************************
 */
#define DT_DRV_COMPAT st_stm32_ospi_controller

#include <zephyr/cache.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <stm32_bitops.h>
#include <stm32_ll_dma.h>
#include "mspi_stm32.h"

#define DT_OSPI_IO_PORT_PROP_OR(prop, default_value, index)                             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, prop),				\
		    (_CONCAT(HAL_OSPIM_, DT_INST_STRING_TOKEN(index, prop))),	\
		    (default_value))

#define DT_OSPI_PROP_OR(prop, default_value, index)                                     \
	DT_INST_PROP_OR(index, prop, default_value)

LOG_MODULE_REGISTER(ospi_stm32, CONFIG_MSPI_LOG_LEVEL);

static int mspi_stm32_ospi_context_lock(struct mspi_stm32_context *ctx,
					const struct mspi_dev_id *req, const struct mspi_xfer *xfer,
					bool lockon)
{
	ARG_UNUSED(lockon);
	if (k_sem_take(&ctx->lock, K_MSEC(xfer->timeout)) < 0) {
		return -EBUSY;
	}
	ctx->xfer = *xfer;
	ctx->packets_left = ctx->xfer.num_packet;

	return 0;
}

static void mspi_stm32_ospi_context_unlock(struct mspi_stm32_context *ctx)
{
	k_sem_give(&ctx->lock);
}

/**
 * Check if the MSPI bus is busy.
 *
 * @param controller MSPI emulation controller device.
 * @return true The MSPI bus is busy.
 * @return false The MSPI bus is idle.
 */
static bool mspi_stm32_ospi_is_inp(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	return k_sem_count_get(&dev_data->ctx.lock) == 0;
}

static uint32_t mspi_stm32_ospi_hal_address_size(uint8_t address_length)
{
	if (address_length == 4U) {
		return HAL_OSPI_ADDRESS_32_BITS;
	}

	return HAL_OSPI_ADDRESS_24_BITS;
}

/*
 * Gives a OSPI_RegularCmdTypeDef with all parameters set
 * except Instruction, Address, NbData
 */
static OSPI_RegularCmdTypeDef mspi_stm32_ospi_prepare_cmd(uint8_t cfg_mode, uint8_t cfg_rate)
{
	/* Command empty structure */
	OSPI_RegularCmdTypeDef cmd_tmp = {0};

	cmd_tmp.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
	cmd_tmp.InstructionSize = (cfg_mode == MSPI_IO_MODE_OCTAL) ? HAL_OSPI_INSTRUCTION_16_BITS
								   : HAL_OSPI_INSTRUCTION_8_BITS;
	cmd_tmp.InstructionDtrMode = (cfg_rate == MSPI_DATA_RATE_DUAL)
					     ? HAL_OSPI_INSTRUCTION_DTR_ENABLE
					     : HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	cmd_tmp.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	cmd_tmp.AddressDtrMode = (cfg_rate == MSPI_DATA_RATE_DUAL) ? HAL_OSPI_ADDRESS_DTR_ENABLE
								   : HAL_OSPI_ADDRESS_DTR_DISABLE;
	cmd_tmp.DataDtrMode = (cfg_rate == MSPI_DATA_RATE_DUAL) ? HAL_OSPI_DATA_DTR_ENABLE
								: HAL_OSPI_DATA_DTR_DISABLE;
	/* AddressWidth must be set to 32bits for init and mem config phase */
	cmd_tmp.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
	cmd_tmp.DataDtrMode = (cfg_rate == MSPI_DATA_RATE_DUAL) ? HAL_OSPI_DATA_DTR_ENABLE
								: HAL_OSPI_DATA_DTR_DISABLE;
	cmd_tmp.DQSMode =
		(cfg_rate == MSPI_DATA_RATE_DUAL) ? HAL_OSPI_DQS_ENABLE : HAL_OSPI_DQS_DISABLE;
	cmd_tmp.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

	switch (cfg_mode) {
	case MSPI_IO_MODE_OCTAL:
		cmd_tmp.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
		cmd_tmp.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
		cmd_tmp.DataMode = HAL_OSPI_DATA_8_LINES;
		break;
	case MSPI_IO_MODE_QUAD:
		cmd_tmp.InstructionMode = HAL_OSPI_INSTRUCTION_4_LINES;
		cmd_tmp.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
		cmd_tmp.DataMode = HAL_OSPI_DATA_4_LINES;
		break;
	case MSPI_IO_MODE_DUAL:
		cmd_tmp.InstructionMode = HAL_OSPI_INSTRUCTION_2_LINES;
		cmd_tmp.AddressMode = HAL_OSPI_ADDRESS_2_LINES;
		cmd_tmp.DataMode = HAL_OSPI_DATA_2_LINES;
		break;
	default:
		cmd_tmp.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
		cmd_tmp.DataMode = HAL_OSPI_DATA_1_LINE;
		break;
	}

	return cmd_tmp;
}

static bool mspi_stm32_ospi_is_memorymap(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	return stm32_reg_read_bits(&dev_data->hmspi.ospi.Instance->CR, OCTOSPI_CR_FMODE) ==
	       OCTOSPI_CR_FMODE;
}

static int mspi_stm32_ospi_memmap_off(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	if (HAL_OSPI_Abort(&dev_data->hmspi.ospi) != HAL_OK) {
		LOG_ERR("MemMapped abort failed");
		return -EIO;
	}
	return 0;
}

/* Set the device in MemMapped mode */
static int mspi_stm32_ospi_memmap_on(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;
	OSPI_RegularCmdTypeDef s_command =
		mspi_stm32_ospi_prepare_cmd(dev_data->dev_cfg.io_mode, dev_data->dev_cfg.data_rate);
	OSPI_MemoryMappedTypeDef s_MemMappedCfg;
	HAL_StatusTypeDef hal_ret;

	if (mspi_stm32_ospi_is_memorymap(controller)) {
		return 0;
	}
	/* Configure in MemoryMapped mode */
	if ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE) &&
	    (mspi_stm32_ospi_hal_address_size(dev_data->dev_cfg.addr_length) ==
	     HAL_OSPI_ADDRESS_24_BITS)) {
		/* OPI mode and 3-bytes address size not supported by memory */
		LOG_ERR("MSPI_IO_MODE_SINGLE in 3Bytes addressing is not supported");
		return -EIO;
	}

	/* Initialize the read command */
	s_command.OperationType = HAL_OSPI_OPTYPE_READ_CFG;
	s_command.InstructionMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					    ? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
						       ? HAL_OSPI_INSTRUCTION_1_LINE
						       : HAL_OSPI_INSTRUCTION_8_LINES)
					    : HAL_OSPI_INSTRUCTION_8_LINES;
	s_command.InstructionDtrMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					       ? HAL_OSPI_INSTRUCTION_DTR_DISABLE
					       : HAL_OSPI_INSTRUCTION_DTR_ENABLE;
	s_command.InstructionSize = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					     ? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
							   ? HAL_OSPI_INSTRUCTION_8_BITS
							   : HAL_OSPI_INSTRUCTION_16_BITS)
						: HAL_OSPI_INSTRUCTION_16_BITS;
	s_command.Instruction = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
						   ? ((mspi_stm32_ospi_hal_address_size(
								   dev_data->dev_cfg.addr_length) ==
							   HAL_OSPI_ADDRESS_24_BITS)
								  ? MSPI_NOR_CMD_READ_FAST
								  : MSPI_NOR_CMD_READ_FAST_4B)
						   : dev_data->dev_cfg.read_cmd)
					: MSPI_NOR_OCMD_DTR_RD;
	s_command.AddressMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
						   ? HAL_OSPI_ADDRESS_1_LINE
						   : HAL_OSPI_ADDRESS_8_LINES)
					: HAL_OSPI_ADDRESS_8_LINES;
	s_command.AddressDtrMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					   ? HAL_OSPI_ADDRESS_DTR_DISABLE
					   : HAL_OSPI_ADDRESS_DTR_ENABLE;
	s_command.AddressSize =
		(dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
			? mspi_stm32_ospi_hal_address_size(dev_data->dev_cfg.addr_length)
			: HAL_OSPI_ADDRESS_32_BITS;
	s_command.DataMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					 ? ((dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
						? HAL_OSPI_DATA_1_LINE
						: HAL_OSPI_DATA_8_LINES)
					 : HAL_OSPI_DATA_8_LINES;
	s_command.DataDtrMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					? HAL_OSPI_DATA_DTR_DISABLE
					: HAL_OSPI_DATA_DTR_ENABLE;
	s_command.DummyCycles = dev_data->ctx.xfer.rx_dummy;
	s_command.DQSMode = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE)
					? HAL_OSPI_DQS_DISABLE
					: HAL_OSPI_DQS_ENABLE;

	if (HAL_OSPI_Command(&dev_data->hmspi.ospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Failed to set memory map %d", dev_data->hmspi.ospi.ErrorCode);
		return -EIO;
	}

	/* Initialize the program command */
	s_command.OperationType = HAL_OSPI_OPTYPE_WRITE_CFG;
	if (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_SINGLE) {
		s_command.Instruction = (dev_data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE)
						? ((mspi_stm32_ospi_hal_address_size(
								dev_data->ctx.xfer.addr_length) ==
							HAL_OSPI_ADDRESS_24_BITS)
							   ? MSPI_NOR_CMD_PP
							   : MSPI_NOR_CMD_PP_4B)
						: MSPI_NOR_OCMD_PAGE_PRG;
	} else {
		s_command.Instruction = MSPI_NOR_OCMD_PAGE_PRG;
	}
	s_command.DQSMode = HAL_OSPI_DQS_DISABLE;
	hal_ret =
		HAL_OSPI_Command(&dev_data->hmspi.ospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to set memory mapped");
		return -EIO;
	}

	/* Enable the memory-mapping */
	s_MemMappedCfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;
	hal_ret = HAL_OSPI_MemoryMapped(&dev_data->hmspi.ospi, &s_MemMappedCfg);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to enable memory mapped");
		return -EIO;
	}

	LOG_INF("Memory mapped mode enabled");

	return 0;
}

static int mspi_stm32_ospi_memmap_read(const struct device *dev,
				       const struct mspi_xfer_packet *packet)
{
	struct mspi_stm32_data *dev_data = dev->data;
	int ret = 0;

	if (!mspi_stm32_ospi_is_memorymap(dev)) {
		ret = mspi_stm32_ospi_memmap_on(dev);
		if (ret != 0) {
			LOG_ERR("Failed to set memory-mapped before read");
			return ret;
		}
		k_usleep(50);
	}
#ifdef CONFIG_DCACHE
	uint32_t addr = dev_data->memmap_base_addr + packet->address;
	uint32_t size = packet->num_bytes;

	__ASSERT_NO_MSG(IS_ALIGNED(addr, CONFIG_DCACHE_LINE_SIZE) &&
			IS_ALIGNED(size, CONFIG_DCACHE_LINE_SIZE));

	sys_cache_data_invd_range((void *)addr, size);
#endif
	memcpy(packet->data_buf, (uint8_t *)dev_data->memmap_base_addr + packet->address,
	       packet->num_bytes);

	return ret;
}

static int mspi_stm32_ospi_abort_memmap(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;
	int ret = 0;

	if (dev_data->xip_cfg.enable && mspi_stm32_ospi_is_memorymap(dev)) {
		ret = mspi_stm32_ospi_memmap_off(dev);
		if (ret != 0) {
			LOG_ERR("%s: Failed to abort memory-mapped", dev->name);
		}
	}

	return ret;
}

/* Send a Command to the NOR and Receive/Transceive data if relevant in IT or DMA mode */
static int mspi_stm32_ospi_access(const struct device *dev, const struct mspi_xfer_packet *packet,
				  uint8_t access_mode)
{

	struct mspi_stm32_data *dev_data = dev->data;

	HAL_StatusTypeDef hal_ret;
	int ret;

	if (dev_data->xip_cfg.enable && packet->dir == MSPI_RX) {
		return mspi_stm32_ospi_memmap_read(dev, packet);
	}

	ret = mspi_stm32_ospi_abort_memmap(dev);
	if (ret != 0) {
		return ret;
	}

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	OSPI_RegularCmdTypeDef cmd =
		mspi_stm32_ospi_prepare_cmd(dev_data->dev_cfg.io_mode, dev_data->dev_cfg.data_rate);

	cmd.NbData = packet->num_bytes;
	cmd.Instruction = packet->cmd;
	if (packet->dir == MSPI_TX) {
		cmd.DummyCycles = dev_data->ctx.xfer.tx_dummy;
	} else {
		cmd.DummyCycles = dev_data->ctx.xfer.rx_dummy;
	}
	cmd.Address = packet->address;
	cmd.AddressSize = mspi_stm32_ospi_hal_address_size(dev_data->ctx.xfer.addr_length);
	if (cmd.NbData == 0) {
		cmd.DataMode = HAL_OSPI_DATA_NONE;
	}

	if ((cmd.Instruction == MSPI_NOR_CMD_WREN) || (cmd.Instruction == MSPI_NOR_OCMD_WREN)) {
		cmd.AddressMode = HAL_OSPI_ADDRESS_NONE;
	}

	LOG_DBG("MSPI access Instruction 0x%x", cmd.Instruction);

	hal_ret = HAL_OSPI_Command(&dev_data->hmspi.ospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		LOG_ERR("Failed to send OSPI instruction");
		return -EIO;
	}

	if (packet->num_bytes == 0) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		return 0;
	}

	if (packet->dir == MSPI_RX) {
		/* Receive the data */
		switch (access_mode) {
		case MSPI_ACCESS_SYNC:
			hal_ret = HAL_OSPI_Receive(&dev_data->hmspi.ospi, packet->data_buf,
					       HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
			break;
		case MSPI_ACCESS_ASYNC:
			hal_ret = HAL_OSPI_Receive_IT(&dev_data->hmspi.ospi, packet->data_buf);
			break;
		case MSPI_ACCESS_DMA:
			hal_ret = HAL_OSPI_Receive_DMA(&dev_data->hmspi.ospi, packet->data_buf);
			break;
		default:
			/* Not correct */
			hal_ret = HAL_BUSY;
			break;
		}
	} else {
		/* Transmit the data */
		switch (access_mode) {
		case MSPI_ACCESS_SYNC:
			hal_ret = HAL_OSPI_Transmit(&dev_data->hmspi.ospi, packet->data_buf,
						HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
			break;
		case MSPI_ACCESS_ASYNC:
			hal_ret = HAL_OSPI_Transmit_IT(&dev_data->hmspi.ospi, packet->data_buf);
			break;
		case MSPI_ACCESS_DMA:
			hal_ret = HAL_OSPI_Transmit_DMA(&dev_data->hmspi.ospi, packet->data_buf);
			break;
		default:
			/* Not correct */
			LOG_INF("default");
			hal_ret = HAL_BUSY;
			break;
		}
	}

	if (hal_ret != HAL_OK || access_mode == MSPI_ACCESS_SYNC) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);

		if (hal_ret != HAL_OK) {
			LOG_ERR("Failed to access data");
			return -EIO;
		}

		goto e_access;
	}

	/* Lock again expecting the IRQ for end of Tx or Rx */
	if (k_sem_take(&dev_data->sync, K_FOREVER) < 0) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		LOG_ERR("Failed to access data");
		return -EIO;
	}

e_access:

	LOG_DBG("Access %zu data at 0x%x", packet->num_bytes, packet->address);

	return 0;
}

/* Start Automatic-Polling mode to wait until the memory is setting mask/value bit */
static int mspi_stm32_ospi_wait_auto_polling(const struct device *dev, uint8_t match_value,
					     uint8_t match_mask, uint32_t timeout_ms)
{
	struct mspi_stm32_data *dev_data = dev->data;
	OSPI_AutoPollingTypeDef s_config = {0};

	/* Set the match to check if the bit is Reset */
	s_config.Match = match_value;
	s_config.Mask = match_mask;

	s_config.MatchMode = HAL_OSPI_MATCH_MODE_AND;
	s_config.Interval = MSPI_NOR_AUTO_POLLING_INTERVAL;
	s_config.AutomaticStop = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

	(void)pm_device_runtime_get(dev);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	if (HAL_OSPI_AutoPolling_IT(&dev_data->hmspi.ospi, &s_config) != HAL_OK) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		LOG_ERR("OSPI AutoPoll failed");
		return -EIO;
	}

	if (k_sem_take(&dev_data->sync, K_MSEC(timeout_ms)) != 0) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		LOG_ERR("OSPI AutoPoll wait failed");
		HAL_OSPI_Abort(&dev_data->hmspi.ospi);
		k_sem_reset(&dev_data->sync);
		return -EIO;
	}

	/* HAL_OSPI_AutoPolling_IT enables transfer error interrupt which sets
	 * cmd_status.
	 */
	return 0;
}

/*
 * Function to Read the status reg of the device
 * Send the RDSR command (according to io_mode/data_rate
 * Then set the Autopolling mode with match mask/value bit
 * --> Blocking
 */
static int mspi_stm32_ospi_status_reg(const struct device *controller, const struct mspi_xfer *xfer)
{
	int ret = 0;
	struct mspi_stm32_data *dev_data = controller->data;

	ret = mspi_stm32_ospi_abort_memmap(controller);
	if (ret != 0) {
		return ret;
	}

	struct mspi_stm32_context *ctx = &dev_data->ctx;

	if (xfer->num_packet == 0 || xfer->packets == NULL) {
		LOG_ERR("Status Reg.: wrong parameters");
		return -EFAULT;
	}

	(void)pm_device_runtime_get(controller);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	/* Lock with the expected timeout value = ctx->xfer.timeout */
	ret = mspi_stm32_ospi_context_lock(ctx, dev_data->dev_id, xfer, true);
	if (ret != 0) {
		goto pm_put;
	}

	OSPI_RegularCmdTypeDef cmd =
		mspi_stm32_ospi_prepare_cmd(dev_data->dev_cfg.io_mode, dev_data->dev_cfg.data_rate);

	/* with this command for tstaus Reg, only one packet containing 2 bytes match/mask */
	if (dev_data->dev_cfg.io_mode == MSPI_IO_MODE_OCTAL) {
		cmd.Instruction = MSPI_NOR_OCMD_RDSR;
		cmd.DummyCycles = dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_DUAL
					  ? MSPI_NOR_DUMMY_REG_OCTAL_DTR
					  : MSPI_NOR_DUMMY_REG_OCTAL;
	} else {
		cmd.Instruction = MSPI_NOR_CMD_RDSR;
		cmd.AddressMode = HAL_OSPI_ADDRESS_NONE;
		cmd.DataMode = HAL_OSPI_DATA_1_LINE;
		cmd.DummyCycles = 0;
		cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
	}
	cmd.NbData = (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U;
	cmd.Address = 0U;

	LOG_DBG("MSPI poll status reg.");

	if (HAL_OSPI_Command(&dev_data->hmspi.ospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Failed to send OSPI instruction");
		ret = -EIO;
		goto ctx_unlock;
	}

	ret = mspi_stm32_ospi_wait_auto_polling(controller, MSPI_NOR_MEM_RDY_MATCH,
						MSPI_NOR_MEM_RDY_MASK,
						HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
ctx_unlock:
	mspi_stm32_ospi_context_unlock(ctx);
pm_put:
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(controller);

	return ret;
}

/*
 * This function Polls the WIP(Write In Progress) bit to become to 0
 * in cfg_mode SPI/OPI MSPI_IO_MODE_SINGLE or MSPI_IO_MODE_OCTAL
 * and cfg_rate transfer STR/DTR MSPI_DATA_RATE_SINGLE or MSPI_DATA_RATE_DUAL
 */
static int mspi_stm32_ospi_mem_ready(const struct device *dev, uint8_t cfg_mode, uint8_t cfg_rate)
{
	struct mspi_stm32_data *dev_data = dev->data;
	int ret = 0;

	ret = mspi_stm32_ospi_abort_memmap(dev);
	if (ret != 0) {
		return ret;
	}

	OSPI_RegularCmdTypeDef s_command = mspi_stm32_ospi_prepare_cmd(cfg_mode, cfg_rate);

	/* Configure automatic polling mode command to wait for memory ready */
	if (cfg_mode == MSPI_IO_MODE_OCTAL) {
		s_command.Instruction = MSPI_NOR_OCMD_RDSR;
		s_command.DummyCycles = (cfg_rate == MSPI_DATA_RATE_DUAL)
						? MSPI_NOR_DUMMY_REG_OCTAL_DTR
						: MSPI_NOR_DUMMY_REG_OCTAL;
	} else {
		s_command.Instruction = MSPI_NOR_CMD_RDSR;
		/* force 1-line InstructionMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
		s_command.AddressMode = HAL_OSPI_ADDRESS_NONE;
		/* force 1-line DataMode for any non-OSPI transfer */
		s_command.DataMode = HAL_OSPI_DATA_1_LINE;
		s_command.DummyCycles = 0;
	}
	s_command.NbData = ((cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U);
	s_command.Address = 0U;

	if (HAL_OSPI_Command(&dev_data->hmspi.ospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("MSPI AutoPoll command failed");
		return -EIO;
	}
	/* Set the match to 0x00 to check if the WIP bit is Reset */
	LOG_DBG("MSPI read status reg MemRdy");
	return mspi_stm32_ospi_wait_auto_polling(dev, MSPI_NOR_MEM_RDY_MATCH, MSPI_NOR_MEM_RDY_MASK,
						 HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
}

/* Enables writing to the memory sending a Write Enable and wait it is effective */
static int mspi_stm32_ospi_write_enable(const struct device *dev, uint8_t cfg_mode,
					uint8_t cfg_rate)
{
	struct mspi_stm32_data *dev_data = dev->data;
	int ret = 0;

	ret = mspi_stm32_ospi_abort_memmap(dev);
	if (ret != 0) {
		return ret;
	}

	OSPI_RegularCmdTypeDef s_command = mspi_stm32_ospi_prepare_cmd(cfg_mode, cfg_rate);

	/* Initialize the write enable command */
	if (cfg_mode == MSPI_IO_MODE_OCTAL) {
		s_command.Instruction = MSPI_NOR_OCMD_WREN;
	} else {
		s_command.Instruction = MSPI_NOR_CMD_WREN;
		/* force 1-line InstructionMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
	}
	s_command.AddressMode = HAL_OSPI_ADDRESS_NONE;
	s_command.DataMode = HAL_OSPI_DATA_NONE;
	s_command.DummyCycles = 0U;

	if (HAL_OSPI_Command(&dev_data->hmspi.ospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("MSPI flash write enable cmd failed");
		return -EIO;
	}
	LOG_DBG("MSPI write enable");

	/* New command to Configure automatic polling mode to wait for write enabling */
	if (cfg_mode == MSPI_IO_MODE_OCTAL) {
		s_command.Instruction = MSPI_NOR_OCMD_RDSR;
		s_command.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
		s_command.DataMode = HAL_OSPI_DATA_8_LINES;
		s_command.DummyCycles = (cfg_rate == MSPI_DATA_RATE_DUAL)
						? MSPI_NOR_DUMMY_REG_OCTAL_DTR
						: MSPI_NOR_DUMMY_REG_OCTAL;
	} else {
		s_command.Instruction = MSPI_NOR_CMD_RDSR;
		/* force 1-line DataMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
		s_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
		s_command.DataMode = HAL_OSPI_DATA_1_LINE;
		s_command.DummyCycles = 0;

		/* DummyCycles remains 0 */
	}
	s_command.NbData = (cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U;
	s_command.Address = 0U;

	if (HAL_OSPI_Command(&dev_data->hmspi.ospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("MSPI config auto polling cmd failed");
		return -EIO;
	}
	LOG_DBG("MSPI read status reg");

	return mspi_stm32_ospi_wait_auto_polling(dev, MSPI_NOR_WREN_MATCH, MSPI_NOR_WREN_MASK,
						 HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
}

/* Write Flash configuration register 2 with new dummy cycles */
static int mspi_stm32_opsi_write_cfg2reg_dummy(const struct device *dev, uint8_t cfg_mode,
					       uint8_t cfg_rate)
{

	int ret = 0;
	struct mspi_stm32_data *dev_data = dev->data;

	ret = mspi_stm32_ospi_abort_memmap(dev);
	if (ret != 0) {
		return ret;
	}

	uint8_t transmit_data = MSPI_NOR_CR2_DUMMY_CYCLES_66MHZ;
	OSPI_RegularCmdTypeDef s_command = mspi_stm32_ospi_prepare_cmd(cfg_mode, cfg_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (cfg_mode == MSPI_IO_MODE_SINGLE) ? MSPI_NOR_CMD_WR_CFGREG2
								  : MSPI_NOR_OCMD_WR_CFGREG2;
	s_command.Address = MSPI_NOR_REG2_ADDR3;
	s_command.DummyCycles = 0U;

	if (cfg_mode == MSPI_IO_MODE_SINGLE) {
		s_command.NbData = 1U;
	} else if (cfg_rate == MSPI_DATA_RATE_DUAL) {
		s_command.NbData = 2U;
	} else {
		s_command.NbData = 1U;
	}

	if (HAL_OSPI_Command(&dev_data->hmspi.ospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("MSPI transmit cmd");
		return -EIO;
	}

	if (HAL_OSPI_Transmit(&dev_data->hmspi.ospi, &transmit_data,
			      HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("MSPI transmit ");
		return -EIO;
	}

	return ret;
}

/* Write Flash configuration register 2 with new single or octal SPI protocol */
static int mspi_stm32_ospi_write_cfg2reg_io(const struct device *dev, uint8_t cfg_mode,
					    uint8_t cfg_rate, uint8_t op_enable)
{
	int ret = 0;
	struct mspi_stm32_data *dev_data = dev->data;

	ret = mspi_stm32_ospi_abort_memmap(dev);
	if (ret != 0) {
		return ret;
	}

	OSPI_RegularCmdTypeDef s_command = mspi_stm32_ospi_prepare_cmd(cfg_mode, cfg_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (cfg_mode == MSPI_IO_MODE_SINGLE) ? MSPI_NOR_CMD_WR_CFGREG2
								  : MSPI_NOR_OCMD_WR_CFGREG2;
	s_command.Address = MSPI_NOR_REG2_ADDR1;
	s_command.DummyCycles = 0U;

	if (cfg_mode == MSPI_IO_MODE_SINGLE) {
		s_command.NbData = 1U;
	} else if (cfg_rate == MSPI_DATA_RATE_DUAL) {
		s_command.NbData = 2U;
	} else {
		s_command.NbData = 1U;
	}

	if (HAL_OSPI_Command(&dev_data->hmspi.ospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	if (HAL_OSPI_Transmit(&dev_data->hmspi.ospi, &op_enable, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	return 0;
}

/* Read Flash configuration register 2 with new single or octal SPI protocol */
static int mspi_stm32_ospi_read_cfg2reg(const struct device *dev, uint8_t cfg_mode,
					uint8_t cfg_rate, uint8_t *value)
{
	int ret = 0;
	struct mspi_stm32_data *dev_data = dev->data;

	ret = mspi_stm32_ospi_abort_memmap(dev);
	if (ret != 0) {
		return ret;
	}

	OSPI_RegularCmdTypeDef s_command = mspi_stm32_ospi_prepare_cmd(cfg_mode, cfg_rate);

	/* Initialize the writing of configuration register 2 */
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
	s_command.NbData = (cfg_rate == MSPI_DATA_RATE_DUAL) ? 2U : 1U;

	if (HAL_OSPI_Command(&dev_data->hmspi.ospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	if (HAL_OSPI_Receive(&dev_data->hmspi.ospi, value, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
	    HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	return 0;
}

static int mspi_stm32_ospi_config_mem(const struct device *dev, uint8_t cfg_mode, uint8_t cfg_rate)
{
	struct mspi_stm32_data *dev_data = dev->data;
	uint8_t reg[2];

	if ((cfg_mode == MSPI_IO_MODE_SINGLE) && (cfg_rate == MSPI_DATA_RATE_SINGLE)) {
		return 0;
	}

	if (mspi_stm32_opsi_write_cfg2reg_dummy(dev, MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE) !=
	    0) {
		LOG_ERR("OSPI write CFGR2 failed");
		return -EIO;
	}
	if (mspi_stm32_ospi_mem_ready(dev, MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE) != 0) {
		LOG_ERR("OSPI autopolling failed");
		return -EIO;
	}
	if (mspi_stm32_ospi_write_enable(dev, MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE) != 0) {
		LOG_ERR("OSPI write Enable 2 failed");
		return -EIO;
	}

	uint8_t mode_enable = ((cfg_rate == MSPI_DATA_RATE_DUAL) ? MSPI_NOR_CR2_DTR_OPI_EN
								 : MSPI_NOR_CR2_STR_OPI_EN);

	if (mspi_stm32_ospi_write_cfg2reg_io(dev, MSPI_IO_MODE_SINGLE, MSPI_DATA_RATE_SINGLE,
					     mode_enable) != 0) {
		LOG_ERR("OSPI write CFGR2 failed");
		return -EIO;
	}

	/* Wait that the configuration is effective and check that memory is ready */
	k_busy_wait(MSPI_STM32_WRITE_REG_MAX_TIME * USEC_PER_MSEC);

	/* Reconfigure the memory type of the peripheral */
	dev_data->hmspi.ospi.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
	dev_data->hmspi.ospi.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;

	if (HAL_OSPI_Init(&dev_data->hmspi.ospi) != HAL_OK) {
		LOG_ERR("OSPI mem type MACRONIX failed");
		return -EIO;
	}

	if (mspi_stm32_ospi_mem_ready(dev, MSPI_IO_MODE_OCTAL, cfg_rate) != 0) {
		/* Check Flash busy ? */
		LOG_ERR("OSPI flash busy failed");
		return -EIO;
	}
	if (mspi_stm32_ospi_read_cfg2reg(dev, MSPI_IO_MODE_OCTAL, cfg_rate, reg) != 0) {
		LOG_ERR("MSPI flash config read failed");
		return -EIO;
	}

	LOG_INF("OSPI flash config is OCTO / %s",
		(cfg_rate == MSPI_DATA_RATE_SINGLE) ? "STR" : "DTR");

	return 0;
}

static void mspi_stm32_ospi_isr(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	HAL_OSPI_IRQHandler(&dev_data->hmspi.ospi);

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


void mspi_stm32_ospi_set_cfg(struct mspi_stm32_data *data, const enum mspi_dev_cfg_mask param_mask,
			     const struct mspi_dev_cfg *dev_cfg)
{
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
static int mspi_stm32_ospi_dev_cfg_save(const struct device *controller,
					const enum mspi_dev_cfg_mask param_mask,
					const struct mspi_dev_cfg *dev_cfg)
{
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;

	if ((param_mask & MSPI_DEVICE_CONFIG_CE_NUM) != 0) {
		data->dev_cfg.ce_num = dev_cfg->ce_num;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) != 0) {
		if (dev_cfg->freq > cfg->mspicfg.max_freq) {
			LOG_ERR("%u, freq is too large.", __LINE__);
			return -ENOTSUP;
		}
		data->dev_cfg.freq = dev_cfg->freq;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0) {
		if (dev_cfg->io_mode >= MSPI_IO_MODE_MAX) {
			LOG_ERR("%u, Invalid io_mode.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.io_mode = dev_cfg->io_mode;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) != 0) {
		if (dev_cfg->data_rate >= MSPI_DATA_RATE_MAX) {
			LOG_ERR("%u, Invalid data_rate.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.data_rate = dev_cfg->data_rate;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CPP) != 0) {
		if (dev_cfg->cpp > MSPI_CPP_MODE_3) {
			LOG_ERR("%u, Invalid cpp.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.cpp = dev_cfg->cpp;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_ENDIAN) != 0) {
		if (dev_cfg->endian > MSPI_XFER_BIG_ENDIAN) {
			LOG_ERR("%u, Invalid endian.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.endian = dev_cfg->endian;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CE_POL) != 0) {
		if (dev_cfg->ce_polarity > MSPI_CE_ACTIVE_HIGH) {
			LOG_ERR("%u, Invalid ce_polarity.", __LINE__);
			return -EINVAL;
		}
		data->dev_cfg.ce_polarity = dev_cfg->ce_polarity;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_DQS) != 0) {
		if (dev_cfg->dqs_enable && !cfg->mspicfg.dqs_support) {
			LOG_ERR("%u, DQS mode not supported.", __LINE__);
			return -ENOTSUP;
		}
		data->dev_cfg.dqs_enable = dev_cfg->dqs_enable;
	}

	mspi_stm32_ospi_set_cfg(data, param_mask, dev_cfg);

	return 0;
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
static int mspi_stm32_ospi_dev_config(const struct device *controller,
				      const struct mspi_dev_id *dev_id,
				      const enum mspi_dev_cfg_mask param_mask,
				      const struct mspi_dev_cfg *dev_cfg)
{
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;
	int ret = 0;
	bool locked = false;

	if (data->dev_id != dev_id) {
		if (k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE))) {
			LOG_ERR("MSPI config failed to access controller.");
			return -EBUSY;
		}

		locked = true;
	}

	if (mspi_stm32_ospi_is_inp(controller)) {
		ret = -EBUSY;
		goto e_return;
	}

	if (param_mask == MSPI_DEVICE_CONFIG_NONE && !cfg->mspicfg.sw_multi_periph) {
		/* Nothing to do but saving the device ID */
		data->dev_id = dev_id;
		goto e_return;
	}

	(void)pm_device_runtime_get(controller);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	/* Proceed step by step in configuration */
	if (param_mask & (MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_DATA_RATE)) {
		/* Going to set the OSPI mode and transfer rate */
		ret = mspi_stm32_ospi_config_mem(controller, dev_cfg->io_mode, dev_cfg->data_rate);
		if (ret != 0) {
			goto e_pm_put;
		}
		LOG_DBG("MSPI confg'd in %d / %d", dev_cfg->io_mode, dev_cfg->data_rate);
	}

	/*
	 * The SFDP is able to change the addr_length 4bytes or 3bytes
	 * this is reflected by the serial_cfg
	 */
	data->dev_id = dev_id;
	/* Go on with other parameters if supported */
	if (mspi_stm32_ospi_dev_cfg_save(controller, param_mask, dev_cfg) != 0) {
		LOG_ERR("failed to set device config");
		ret = -EIO;
	}

e_pm_put:
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(controller);

e_return:

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
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 */
static int mspi_stm32_ospi_xip_config(const struct device *controller,
				      const struct mspi_dev_id *dev_id,
				      const struct mspi_xip_cfg *xip_cfg)
{
	struct mspi_stm32_data *dev_data = controller->data;
	int ret = 0;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("dev_id don't match");
		return -ESTALE;
	}

	(void)pm_device_runtime_get(controller);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	if (!xip_cfg->enable) {
		/* This is for aborting */
		ret = mspi_stm32_ospi_memmap_off(controller);
	} else {
		ret = mspi_stm32_ospi_memmap_on(controller);
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
 * @retval -EBUSY MSPI bus is busy
 */
static int mspi_stm32_ospi_get_channel_status(const struct device *controller, uint8_t ch)
{
	struct mspi_stm32_data *dev_data = controller->data;
	int ret = 0;

	ARG_UNUSED(ch);

	if (mspi_stm32_ospi_is_inp(controller) ||
	    __HAL_OSPI_GET_FLAG(&dev_data->hmspi.ospi, HAL_OSPI_FLAG_BUSY) == SET) {
		ret = -EBUSY;
	}

	dev_data->dev_id = NULL;

	return ret;
}

static int mspi_stm32_ospi_pio_transceive(const struct device *controller,
					  const struct mspi_xfer *xfer)
{
	int ret = 0;
	uint32_t packet_idx;
	struct mspi_stm32_data *dev_data = controller->data;
	struct mspi_stm32_context *ctx = &dev_data->ctx;
	const struct mspi_xfer_packet *packet;

	if (xfer->num_packet == 0 || xfer->packets == NULL ||
	    xfer->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		LOG_ERR("Transfer: wrong parameters");
		return -EFAULT;
	}

	/* DummyCycle to give to the mspi_stm32_read_access/mspi_stm32_write_access */
	ret = mspi_stm32_ospi_context_lock(ctx, dev_data->dev_id, xfer, true);
	if (ret != 0) {
		return ret;
	}

	/* Asynchronous transfer call read/write with IT and callback function */
	while (ctx->packets_left > 0) {
		packet_idx = ctx->xfer.num_packet - ctx->packets_left;
		packet = &ctx->xfer.packets[packet_idx];

		ret = mspi_stm32_ospi_access(
			controller, packet, ctx->xfer.async ? MSPI_ACCESS_ASYNC : MSPI_ACCESS_SYNC);

		if (ret != 0) {
			ret = -EIO;
			goto pio_end;
		}
		ctx->packets_left--;
	}

pio_end:
	mspi_stm32_ospi_context_unlock(ctx);
	return ret;
}

#ifdef CONFIG_MSPI_DMA
static int mspi_stm32_ospi_dma_transceive(const struct device *controller,
					  const struct mspi_xfer *xfer)
{
	int ret = 0;
	struct mspi_stm32_data *dev_data = controller->data;
	struct mspi_stm32_context *ctx = &dev_data->ctx;
	const struct mspi_stm32_conf *dev_conf = controller->config;
	const struct mspi_xfer_packet *packet;

	if (!dev_conf->dma_specified) {
		LOG_ERR("DMA configuration is missing from the device tree");
		return -EIO;
	}

	if (xfer->num_packet == 0 || xfer->packets == NULL ||
	    xfer->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		return -EFAULT;
	}

	ret = mspi_stm32_ospi_context_lock(ctx, dev_data->dev_id, xfer, true);

	if (ret != 0) {
		return ret;
	}

	while (ctx->packets_left > 0) {
		uint32_t packet_idx = ctx->xfer.num_packet - ctx->packets_left;

		packet = &ctx->xfer.packets[packet_idx];

		ret = mspi_stm32_ospi_access(controller, packet, MSPI_ACCESS_DMA);

		ctx->packets_left--;
		if (ret != 0) {
			ret = -EIO;
			goto dma_end;
		}
	}

dma_end:
	mspi_stm32_ospi_context_unlock(ctx);
	return ret;
}
#endif

/**
 * API implementation of mspi_transceive.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param xfer Pointer to the MSPI transfer started by dev_id.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 * @retval A negative errno value upon failure.
 */
static int mspi_stm32_ospi_transceive(const struct device *controller,
				      const struct mspi_dev_id *dev_id,
				      const struct mspi_xfer *xfer)
{
	struct mspi_stm32_data *dev_data = controller->data;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("transceive : dev_id don't match");
		return -ESTALE;
	}

	/* Need to map the xfer to the data context */
	dev_data->ctx.xfer = *xfer;

	/*
	 * async + MSPI_PIO : Use callback on Irq if PIO
	 * sync + MSPI_PIO use timeout (mainly for NOR command and param
	 * MSPI_DMA : async/sync is meaningless with DMA (no DMA IT function)t
	 */
	if ((xfer->packets->cmd == MSPI_NOR_OCMD_RDSR) ||
	    (xfer->packets->cmd == MSPI_NOR_CMD_RDSR)) {
		/* This is a command and an autopolling on the status register */
		return mspi_stm32_ospi_status_reg(controller, xfer);
	}
#ifdef CONFIG_MSPI_DMA
	return mspi_stm32_ospi_dma_transceive(controller, xfer);
#endif
	if (xfer->xfer_mode == MSPI_PIO) {
		return mspi_stm32_ospi_pio_transceive(controller, xfer);
	} else {
		return -EIO;
	}
}

#if defined(CONFIG_MSPI_DMA) && !defined(HAL_MDMA_MODULE_ENABLED)
static int mspi_stm32_ospi_dma_setup(const struct mspi_stm32_conf *dev_cfg,
				     struct mspi_stm32_data *dev_data)
{
	int ret = 0;

	struct dma_config dma_cfg = dev_data->dma.cfg;
	DMA_HandleTypeDef *hdma = &dev_data->hdma;

	if (!device_is_ready(dev_data->dma.dev)) {
		LOG_ERR("%s device not ready", dev_data->dma.dev->name);
		return -ENODEV;
	}

	dma_cfg.user_data = hdma;
	dma_cfg.linked_channel = STM32_DMA_HAL_OVERRIDE;

	ret = dma_config(dev_data->dma.dev, dev_data->dma.channel, &dma_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d", dev_data->dma.channel);
		return ret;
	}

	if (dma_cfg.source_data_size != dma_cfg.dest_data_size) {
		LOG_ERR("Source and Destination data sizes are not aligned");
		return -EINVAL;
	}

	int index = find_lsb_set(dma_cfg.source_data_size) - 1;

#ifdef CONFIG_DMA_STM32U5
	/* Fill the structure for dma init */
	hdma->Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
	hdma->Init.SrcInc = DMA_SINC_FIXED;
	hdma->Init.DestInc = DMA_DINC_INCREMENTED;
	hdma->Init.SrcDataWidth = mspi_stm32_table_src_size[index];
	hdma->Init.DestDataWidth = mspi_stm32_table_dest_size[index];
	hdma->Init.SrcBurstLength = 4;
	hdma->Init.DestBurstLength = 4;
	hdma->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
	hdma->Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
#else
	hdma->Init.PeriphDataAlignment = mspi_stm32_table_dest_size[index];
	hdma->Init.MemDataAlignment = mspi_stm32_table_src_size[index];
	hdma->Init.PeriphInc = DMA_PINC_DISABLE;
	hdma->Init.MemInc = DMA_MINC_ENABLE;
#endif /* CONFIG_DMA_STM32U5 */

	hdma->Init.Mode = DMA_NORMAL;
	hdma->Init.Priority = mspi_stm32_table_priority[dma_cfg.channel_priority];
	hdma->Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma->Instance = STM32_DMA_GET_INSTANCE(dev_data->dma.reg, dev_data->dma.channel);
	hdma->Init.Request = dma_cfg.dma_slot;
	__HAL_LINKDMA(&dev_data->hmspi.ospi, hdma, *hdma);
	if (HAL_DMA_Init(hdma) != HAL_OK) {
		LOG_ERR("OSPI DMA Init failed");
		return -EIO;
	}
	LOG_INF("OSPI with DMA Transfer");

	return ret;
}

static __maybe_unused void mspi_stm32_ospi_dma_callback(const struct device *dev, void *arg,
							uint32_t channel, int status)
{
	DMA_HandleTypeDef *hdma = arg;

	ARG_UNUSED(dev);
	ARG_UNUSED(channel);

	if (status < 0) {
		LOG_ERR("DMA callback error with channel %d", channel);
	}

	HAL_DMA_IRQHandler(hdma);
}
#endif /* CONFIG_MSPI_DMA && !HAL_MDMA_MODULE_ENABLED */

static int mspi_stm32_ospi_conf_validate(const struct mspi_cfg *config, uint32_t max_frequency)
{
	/* Only Controller mode is supported */
	if (config->op_mode != MSPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only support MSPI controller mode.");
		return -ENOTSUP;
	}

	/* Check the max possible freq. */
	if (config->max_freq > max_frequency) {
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

	return 0;
}

static int mspi_stm32_ospi_activate(const struct device *dev)
{
	int ret;
	const struct mspi_stm32_conf *config = (const struct mspi_stm32_conf *)dev->config;

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (config->pclk_len > 1) {
		if (clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					    (clock_control_subsys_t)&config->pclken[1],
					    NULL) != 0) {
			LOG_ERR("Could not select OSPI domain clock");
			return -EIO;
		}
	}

	/* Clock configuration */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t)(uintptr_t)&config->pclken[0]) != 0) {
		LOG_ERR("Could not enable OSPI clock");
		return -EIO;
	}

	return 0;
}

static int mspi_stm32_ospi_clock_config(struct mspi_stm32_data *dev_data,
					const struct mspi_stm32_conf *dev_cfg)
{
	uint32_t ahb_clock_freq;
	uint32_t prescaler = MSPI_STM32_CLOCK_PRESCALER_MIN;

	/* Max 3 domain clock are expected */
	if (dev_cfg->pclk_len > 3) {
		LOG_ERR("Could not select %d OSPI domain clock", dev_cfg->pclk_len);
		return -EIO;
	}

	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				   (clock_control_subsys_t)&dev_cfg->pclken[0],
				   &ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken)");
		return -EIO;
	}

	/* Alternate clock config for peripheral if any */
	if (dev_cfg->pclk_len > 1) {
		if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					   (clock_control_subsys_t)&dev_cfg->pclken[1],
					   &ahb_clock_freq) < 0) {
			LOG_ERR("Failed call clock_control_get_rate(pclken)");
			return -EIO;
		}
	}

	if (dev_cfg->pclk_len > 2) {
		if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				     (clock_control_subsys_t)&dev_cfg->pclken[2]) != 0) {
			LOG_ERR("Could not enable OSPI Manager clock");
			return -EIO;
		}
	}

	for (prescaler = MSPI_STM32_CLOCK_PRESCALER_MIN;
	     prescaler <= MSPI_STM32_CLOCK_PRESCALER_MAX; prescaler++) {
		dev_data->dev_cfg.freq = MSPI_STM32_CLOCK_COMPUTE(ahb_clock_freq, prescaler);

		if (dev_data->dev_cfg.freq <= dev_cfg->mspicfg.max_freq) {
			break;
		}
	}
	__ASSERT_NO_MSG(prescaler <= MSPI_STM32_CLOCK_PRESCALER_MAX);

	/* Initialize XSPI HAL structure completely */
	dev_data->hmspi.ospi.Init.ClockPrescaler = prescaler;

	return 0;
}

/**
 * API implementation of mspi_config : controller configuration.
 *
 * @param spec Pointer to MSPI device tree spec.
 * @return 0 if successful.
 * @return A negative errno value upon failure.
 */
static int mspi_stm32_ospi_config(const struct mspi_dt_spec *spec)
{
	const struct mspi_cfg *config = &spec->config;
	const struct mspi_stm32_conf *dev_cfg = spec->bus->config;
	struct mspi_stm32_data *dev_data = spec->bus->data;
	int ret = 0;

	ret = mspi_stm32_ospi_conf_validate(config, dev_cfg->mspicfg.max_freq);
	if (ret != 0) {
		return ret;
	}

	(void)pm_device_runtime_get(spec->bus);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

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
	ret = mspi_stm32_ospi_activate(spec->bus);
	if (ret != 0) {
		goto end;
	}

	ret = mspi_stm32_ospi_clock_config(dev_data, dev_cfg);
	if (ret != 0) {
		goto end;
	}
	/** The stm32 hal_mspi driver does not reduce DEVSIZE before writing the DCR1
	 * dev_data->hmspi.ospi.Init.MemorySize = find_lsb_set(dev_cfg->reg_size) - 2;
	 * dev_data->hmspi.ospi.Init.MemorySize is mandatory now (BUSY = 0) for HAL_XSPI Init
	 * give the value from the child node
	 */
#if defined(XSPI_DCR2_WRAPSIZE)
	dev_data->hmspi.ospi.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
#endif /* XSPI_DCR2_WRAPSIZE */
	/* STR mode else Macronix for DTR mode */
	if (dev_data->dev_cfg.data_rate == MSPI_DATA_RATE_DUAL) {
		dev_data->hmspi.ospi.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
		dev_data->hmspi.ospi.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;
	} else {
		dev_data->hmspi.ospi.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
		dev_data->hmspi.ospi.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
	}
#if MSPI_STM32_DLYB_BYPASSED
	dev_data->hmspi.ospi.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
#else
	dev_data->hmspi.ospi.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_USED;
#endif /* MSPI_STM32_DLYB_BYPASSED */

	if (HAL_OSPI_Init(&dev_data->hmspi.ospi) != HAL_OK) {
		LOG_ERR("MSPI Init failed");
		ret = -EIO;
		goto end;
	}

	LOG_DBG("MSPI Init'd");
#if defined(OCTOSPIM)
	/* OCTOSPI I/O manager init Function */
	OSPIM_CfgTypeDef ospi_mgr_cfg = {0};

	if (dev_data->hmspi.ospi.Instance == OCTOSPI1) {
		ospi_mgr_cfg.ClkPort = DT_OSPI_PROP_OR(clk_port, 1, dev_data->dev_id);
		ospi_mgr_cfg.DQSPort = DT_OSPI_PROP_OR(dqs_port, 1, dev_data->dev_id);
		ospi_mgr_cfg.NCSPort = DT_OSPI_PROP_OR(ncs_port, 1, dev_data->dev_id);
		ospi_mgr_cfg.IOLowPort = DT_OSPI_IO_PORT_PROP_OR(
			io_low_port, HAL_OSPIM_IOPORT_1_LOW, dev_data->dev_id);
		ospi_mgr_cfg.IOHighPort = DT_OSPI_IO_PORT_PROP_OR(
			io_high_port, HAL_OSPIM_IOPORT_1_HIGH, dev_data->dev_id);
	} else if (dev_data->hmspi.ospi.Instance == OCTOSPI2) {
		ospi_mgr_cfg.ClkPort = DT_OSPI_PROP_OR(clk_port, 2, dev_data->dev_id);
		ospi_mgr_cfg.DQSPort = DT_OSPI_PROP_OR(dqs_port, 2, dev_data->dev_id);
		ospi_mgr_cfg.NCSPort = DT_OSPI_PROP_OR(ncs_port, 2, dev_data->dev_id);
		ospi_mgr_cfg.IOLowPort = DT_OSPI_IO_PORT_PROP_OR(
			io_low_port, HAL_OSPIM_IOPORT_2_LOW, dev_data->dev_id);
		ospi_mgr_cfg.IOHighPort = DT_OSPI_IO_PORT_PROP_OR(
			io_high_port, HAL_OSPIM_IOPORT_2_HIGH, dev_data->dev_id);
	} else {
		LOG_ERR("Unknown OSPI Instance");
		ret = -EINVAL;
		goto end;
	}
#if defined(OCTOSPIM_CR_MUXEN)
	ospi_mgr_cfg.Req2AckTime = 1;
#endif /* OCTOSPIM_CR_MUXEN */
	if (HAL_OSPIM_Config(&dev_data->hmspi.ospi, &ospi_mgr_cfg,
			     HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI M config failed");
		ret = -EIO;
		goto end;
	}
#if defined(CONFIG_SOC_SERIES_STM32U5X)
	/* OCTOSPI2 delay block init Function */
	HAL_OSPI_DLYB_CfgTypeDef ospi_delay_block_cfg = {0};

	ospi_delay_block_cfg.Units = 56;
	ospi_delay_block_cfg.PhaseSel = 2;
	if (HAL_OSPI_DLYB_SetConfig(&dev_data->hmspi.ospi, &ospi_delay_block_cfg) != HAL_OK) {
		LOG_ERR("OSPI DelayBlock failed");
		ret = -EIO;
		goto end;
	}
#endif /* CONFIG_SOC_SERIES_STM32U5X */

#endif /* OCTOSPIM */
#if defined(CONFIG_MSPI_DMA) && !defined(HAL_MDMA_MODULE_ENABLED)
	if (dev_cfg->dma_specified) {
		mspi_stm32_ospi_dma_setup(dev_cfg, dev_data);
	}
#endif

	if (k_sem_count_get(&dev_data->ctx.lock) == 0) {
		mspi_stm32_ospi_context_unlock(&dev_data->ctx);
	}

	if (config->re_init) {
		k_mutex_unlock(&dev_data->lock);
	}
end:
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(spec->bus);

	LOG_INF("MSPI config result: %s", (ret == 0) ? "success" : "failed");

	return ret;
}

/**
 * Set up a new controller and add its child to the list.
 *
 * @param dev MSPI emulation controller.
 *
 * @retval 0 if successful.
 */
static int mspi_stm32_ospi_init(const struct device *controller)
{
	const struct mspi_stm32_conf *cfg = controller->config;
	const struct mspi_dt_spec spec = {
		.bus = controller,
		.config = cfg->mspicfg,
	};

	return mspi_stm32_ospi_config(&spec);
}

static DEVICE_API(mspi, mspi_stm32_driver_api) = {
	.config = mspi_stm32_ospi_config,
	.dev_config = mspi_stm32_ospi_dev_config,
	.xip_config = mspi_stm32_ospi_xip_config,
	.get_channel_status = mspi_stm32_ospi_get_channel_status,
	.transceive = mspi_stm32_ospi_transceive,
};

#ifdef CONFIG_PM_DEVICE
static int mspi_stm32_ospi_suspend(const struct device *dev)
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

	/* Optional alternate clocks */
	if (cfg->pclk_len > 1) {
		if (clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				      (clock_control_subsys_t)&cfg->pclken[1]) != 0) {
			LOG_ERR("Could not enable XSPI Manager clock");
			return -EIO;
		}
	}

	if (cfg->pclk_len > 2) {
		if (clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				      (clock_control_subsys_t)&cfg->pclken[2]) != 0) {
			LOG_ERR("Could not enable XSPI Manager clock");
			return -EIO;
		}
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

static int mspi_stm32_ospi_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return mspi_stm32_ospi_activate(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		return mspi_stm32_ospi_suspend(dev);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

#if CONFIG_MSPI_DMA
#define DMA_CHANNEL_CONFIG(node, dir) DT_DMAS_CELL_BY_NAME(node, dir, channel_config)

#define OSPI_DMA_CHANNEL_INIT(node, dir)                                                           \
	.dev = DEVICE_DT_GET(DT_DMAS_CTLR(node)),                                                  \
	.channel = DT_DMAS_CELL_BY_NAME(node, dir, channel),                                       \
	.reg = (DMA_TypeDef *)DT_REG_ADDR(DT_PHANDLE_BY_NAME(node, dmas, dir)),                    \
	.cfg = {                                                                                   \
		.dma_slot = DT_DMAS_CELL_BY_NAME(node, dir, slot),                                 \
		.source_data_size =                                                                \
			STM32_DMA_CONFIG_PERIPHERAL_DATA_SIZE(DMA_CHANNEL_CONFIG(node, dir)),      \
		.dest_data_size =                                                                  \
			STM32_DMA_CONFIG_MEMORY_DATA_SIZE(DMA_CHANNEL_CONFIG(node, dir)),          \
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(DMA_CHANNEL_CONFIG(node, dir)),      \
		.dma_callback = mspi_stm32_ospi_dma_callback,                                      \
	},

#define OSPI_DMA_CHANNEL(node, dir)                                                                \
	.dma = {                                                                                   \
		COND_CODE_1(DT_DMAS_HAS_NAME(node, dir),                                           \
			    (OSPI_DMA_CHANNEL_INIT(node, dir)),                                    \
			    (NULL))                                                                \
	},
#else
#define OSPI_DMA_CHANNEL(node, dir)
#endif
/* MSPI control config */
#define MSPI_CONFIG(index)                                                                         \
	{                                                                                          \
		.channel_num = 0,                                                                  \
		.op_mode = DT_INST_ENUM_IDX_OR(index, op_mode, MSPI_OP_MODE_CONTROLLER),           \
		.duplex = DT_INST_ENUM_IDX_OR(index, duplex, MSPI_HALF_DUPLEX),                    \
		.max_freq = DT_INST_PROP(index, clock_frequency),                                  \
		.dqs_support = DT_INST_PROP(index, dqs_support),                                   \
		.num_periph = DT_INST_CHILD_NUM(index),                                            \
		.sw_multi_periph = DT_INST_PROP(index, software_multiperipheral),                  \
	}

#define STM32_SMPI_IRQ_HANDLER(index)                                                              \
	static void mspi_stm32_irq_config_func_##index(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),                     \
			    mspi_stm32_ospi_isr, DEVICE_DT_INST_GET(index), 0);                    \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#define MSPI_STM32_INIT(index)                                                                     \
	static const struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);           \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static struct gpio_dt_spec ce_gpios##index[] = MSPI_CE_GPIOS_DT_SPEC_INST_GET(index);      \
                                                                                                   \
	STM32_SMPI_IRQ_HANDLER(index)                                                              \
                                                                                                   \
	static const struct mspi_stm32_conf mspi_stm32_dev_conf_##index = {                        \
		.pclken = pclken_##index,                                                          \
		.pclk_len = DT_INST_NUM_CLOCKS(index),                                             \
		.irq_config = mspi_stm32_irq_config_func_##index,                                  \
		.mspicfg = MSPI_CONFIG(index),                                                     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                     \
		.mspicfg.num_ce_gpios = ARRAY_SIZE(ce_gpios##index),                               \
		.dma_specified = DT_INST_NODE_HAS_PROP(index, dmas),                               \
	};                                                                                         \
	static struct mspi_stm32_data mspi_stm32_dev_data_##index = {                              \
		.hmspi.ospi = {                                                                    \
			.Instance = (OCTOSPI_TypeDef *)DT_INST_REG_ADDR(index),                    \
			.Init = {                                                                  \
				.FifoThreshold = MSPI_STM32_FIFO_THRESHOLD,                        \
				.SampleShifting = (DT_INST_PROP(index, st_ssht_enable) ?           \
						  HAL_OSPI_SAMPLE_SHIFTING_HALFCYCLE :             \
						  HAL_OSPI_SAMPLE_SHIFTING_NONE),                  \
				.ChipSelectHighTime = 1,                                           \
				.ClockMode = HAL_OSPI_CLOCK_MODE_0,                                \
				.ChipSelectBoundary = 0,                                           \
				.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE,                   \
			},                                                                         \
		},                                                                                 \
		.memmap_base_addr = DT_INST_REG_ADDR_BY_IDX(index, 1),                             \
		.dev_id = index,                                                                   \
		.lock = Z_MUTEX_INITIALIZER(mspi_stm32_dev_data_##index.lock),                     \
		.sync = Z_SEM_INITIALIZER(mspi_stm32_dev_data_##index.sync, 0, 1),                 \
		.dev_cfg = {0},                                                                    \
		.xip_cfg = {0},                                                                    \
		.ctx.lock = Z_SEM_INITIALIZER(mspi_stm32_dev_data_##index.ctx.lock, 0, 1),         \
		OSPI_DMA_CHANNEL(DT_DRV_INST(index), tx_rx)                                        \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(index, mspi_stm32_ospi_pm_action);                                \
	DEVICE_DT_INST_DEFINE(index, &mspi_stm32_ospi_init, PM_DEVICE_DT_INST_GET(index),          \
			      &mspi_stm32_dev_data_##index, &mspi_stm32_dev_conf_##index,          \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                     \
			      &mspi_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_STM32_INIT)
