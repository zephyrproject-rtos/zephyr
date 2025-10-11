/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * **************************************************************************
 * MSPI flash controller driver for stm32 series with QSPI peripheral
 * This driver is based on the stm32Cube HAL QSPI driver
 * **************************************************************************
 */
#define DT_DRV_COMPAT st_stm32_qspi_controller

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <stm32_ll_dma.h>
#include "mspi_stm32.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mspi_stm32_qspi, CONFIG_MSPI_LOG_LEVEL);

static inline int mspi_context_lock(struct mspi_context *ctx, const struct mspi_dev_id *req,
				    const struct mspi_xfer *xfer, bool lockon)
{
	int ret = 0;

	if (k_sem_take(&ctx->lock, K_MSEC(xfer->timeout))) {
		return -EBUSY;
	}

	ctx->xfer = *xfer;

	ctx->packets_left = ctx->xfer.num_packet;
	return ret;
}

/**
 * @brief Gives a QSPI_CommandTypeDef with all parameters set except Instruction, Address, NbData
 */
static QSPI_CommandTypeDef mspi_stm32_qspi_prepare_cmd(uint8_t cfg_mode, uint8_t cfg_rate)
{
	/* Command empty structure */
	QSPI_CommandTypeDef cmd_tmp = {0};

	/* QSPI specific configuration */
	cmd_tmp.AddressSize = QSPI_ADDRESS_24_BITS;
	cmd_tmp.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	cmd_tmp.DdrMode =
		((cfg_rate == MSPI_DATA_RATE_DUAL) ? QSPI_DDR_MODE_ENABLE : QSPI_DDR_MODE_DISABLE);
	cmd_tmp.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	cmd_tmp.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure based on IO mode */
	switch (cfg_mode) {
	case MSPI_IO_MODE_QUAD: {
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_4_LINES;
		cmd_tmp.AddressMode = QSPI_ADDRESS_4_LINES;
		cmd_tmp.DataMode = QSPI_DATA_4_LINES;
		break;
	}
	case MSPI_IO_MODE_QUAD_1_4_4: {
		/* Command uses 1 line, address and data use 4 lines */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = QSPI_ADDRESS_4_LINES;
		cmd_tmp.DataMode = QSPI_DATA_4_LINES;
		break;
	}
	case MSPI_IO_MODE_QUAD_1_1_4: {
		/* Command and address use 1 line, data uses 4 lines */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = QSPI_ADDRESS_1_LINE;
		cmd_tmp.DataMode = QSPI_DATA_4_LINES;
		break;
	}
	case MSPI_IO_MODE_DUAL: {
		/* All phases use 2 lines */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_2_LINES;
		cmd_tmp.AddressMode = QSPI_ADDRESS_2_LINES;
		cmd_tmp.DataMode = QSPI_DATA_2_LINES;
		break;
	}
	case MSPI_IO_MODE_DUAL_1_2_2: {
		/* Command uses 1 line, address and data use 2 lines */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = QSPI_ADDRESS_2_LINES;
		cmd_tmp.DataMode = QSPI_DATA_2_LINES;
		break;
	}
	case MSPI_IO_MODE_DUAL_1_1_2: {
		/* Command and address use 1 line, data uses 2 lines */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = QSPI_ADDRESS_1_LINE;
		cmd_tmp.DataMode = QSPI_DATA_2_LINES;
		break;
	}
	case MSPI_IO_MODE_OCTAL:
		/* QSPI doesn't support octal mode, fall through to single */
		LOG_WRN("QSPI doesn't support octal mode, using single line");
		__fallthrough;
	case MSPI_IO_MODE_SINGLE:
	default: {
		/* All phases use 1 line */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = QSPI_ADDRESS_1_LINE;
		cmd_tmp.DataMode = QSPI_DATA_1_LINE;
		break;
	}
	}

	return cmd_tmp;
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

static uint32_t mspi_stm32_qspi_hal_address_size(uint8_t address_length)
{
	if (address_length == 4U) {
		return QSPI_ADDRESS_32_BITS;
	}

	return QSPI_ADDRESS_24_BITS;
}

/* Check if device is in memory-mapped mode */
static bool mspi_stm32_qspi_is_memmap(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	/* Check the FMODE bits in CCR register to see if in memory-mapped mode */
	return (READ_BIT(dev_data->hmspi.qspi.Instance->CCR, QUADSPI_CCR_FMODE) ==
		QUADSPI_CCR_FMODE);
}

/* Set the device back in command mode */
static int mspi_stm32_qspi_memmap_off(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	if (!mspi_stm32_qspi_is_memmap(controller)) {
		/* Already in command mode */
		return 0;
	}

	if (HAL_QSPI_Abort(&dev_data->hmspi.qspi) != HAL_OK) {
		LOG_ERR("QSPI MemMapped abort failed");
		return -EIO;
	}

	LOG_DBG("QSPI memory mapped mode disabled");
	return 0;
}

/* Set the device in MemMapped mode */
static int mspi_stm32_qspi_memmap_on(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;
	QSPI_CommandTypeDef s_command;
	QSPI_MemoryMappedTypeDef s_MemMappedCfg;
	HAL_StatusTypeDef hal_ret;

	if (mspi_stm32_qspi_is_memmap(controller)) {
		/* Already in memory-mapped mode */
		return 0;
	}

	s_command =
		mspi_stm32_qspi_prepare_cmd(dev_data->dev_cfg.io_mode, dev_data->dev_cfg.data_rate);

	/* Set read command - use the configured read command if available */
	if (dev_data->dev_cfg.read_cmd != 0) {
		s_command.Instruction = dev_data->dev_cfg.read_cmd;
	} else {
		/* Fallback to standard fast read commands */
		if (dev_data->dev_cfg.addr_length == 4) {
			s_command.Instruction = MSPI_STM32_CMD_READ_FAST_4B;
		} else {
			s_command.Instruction = MSPI_STM32_CMD_READ_FAST;
		}
	}

	s_command.AddressSize = mspi_stm32_qspi_hal_address_size(dev_data->dev_cfg.addr_length);
	s_command.DummyCycles = dev_data->dev_cfg.rx_dummy;
	s_command.Address = 0;

	/* Enable the memory-mapping */
	s_MemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
	s_MemMappedCfg.TimeOutPeriod = 0;

	hal_ret = HAL_QSPI_MemoryMapped(&dev_data->hmspi.qspi, &s_command, &s_MemMappedCfg);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to enable QSPI memory mapped mode: %d", hal_ret);
		return -EIO;
	}

	LOG_DBG("QSPI memory mapped mode enabled");
	return 0;
}

/**
 * @brief Check if command requires indirect mode in XIP configuration
 *
 * @param packet Pointer to transfer packet
 * @return true if command needs indirect mode.
 * @return -Error if fail.
 */
static bool mspi_stm32_qspi_needs_indirect_mode(const struct mspi_xfer_packet *packet)
{
	return ((packet->cmd == MSPI_STM32_CMD_WREN) || (packet->cmd == MSPI_STM32_CMD_SE) ||
		(packet->cmd == MSPI_STM32_CMD_SE_4B) || (packet->cmd == MSPI_STM32_CMD_RDSR) ||
		(packet->dir == MSPI_TX));
}

/**
 * @brief Execute data transfer (TX or RX) in indirect mode
 *
 * @param dev_data Pointer to device data structure
 * @param packet Pointer to transfer packet
 * @param access_mode Access mode (SYNC, ASYNC, or DMA)
 * @return 0 on success
 * @return -Error if fail.
 */
static int mspi_stm32_qspi_execute_transfer(struct mspi_stm32_data *dev_data,
					    const struct mspi_xfer_packet *packet,
					    uint8_t access_mode)
{
	HAL_StatusTypeDef hal_ret;

	if (packet->dir == MSPI_RX) {
		/* Receive the data */
		switch (access_mode) {
		case MSPI_ACCESS_SYNC:
			hal_ret = HAL_QSPI_Receive(&dev_data->hmspi.qspi, packet->data_buf,
						   HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
			goto e_transfer;
		case MSPI_ACCESS_ASYNC:
			hal_ret = HAL_QSPI_Receive_IT(&dev_data->hmspi.qspi, packet->data_buf);
			break;
		case MSPI_ACCESS_DMA:
			/* DMA mode not yet implemented for QSPI */
			LOG_ERR("DMA mode not supported yet for QSPI");
			return -ENOTSUP;
		default:
			LOG_ERR("Invalid access mode: %d", access_mode);
			return -EINVAL;
		}
	} else {
		/* Transmit the data */
		switch (access_mode) {
		case MSPI_ACCESS_SYNC:
			hal_ret = HAL_QSPI_Transmit(&dev_data->hmspi.qspi, packet->data_buf,
						    HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
			goto e_transfer;
		case MSPI_ACCESS_ASYNC:
			hal_ret = HAL_QSPI_Transmit_IT(&dev_data->hmspi.qspi, packet->data_buf);
			break;
		case MSPI_ACCESS_DMA:
			/* DMA mode not yet implemented for QSPI */
			LOG_ERR("DMA mode not supported yet for QSPI");
			return -ENOTSUP;
		default:
			LOG_ERR("Invalid access mode: %d", access_mode);
			return -EINVAL;
		}
	}

	/* For ASYNC mode, wait for operation to complete */
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to start %s transfer: %d",
			(packet->dir == MSPI_RX) ? "receive" : "transmit", hal_ret);
		return -EIO;
	}

	if (k_sem_take(&dev_data->sync, K_FOREVER)) {
		LOG_ERR("Failed to complete async transfer");
		return -EIO;
	}

e_transfer:
	return (hal_ret == HAL_OK) ? 0 : -EIO;
}

/**
 * @brief Read data in memory-mapped mode (XIP)
 *
 * Note: Write operations are NOT supported in memory-mapped mode for QSPI.
 * Writes must use indirect mode .
 *
 * @param dev Pointer to the device structure
 * @param packet Pointer to transfer packet (must be RX direction)
 * @return 0 on success.
 * @return -Error if fail.
 */
static int mspi_stm32_qspi_memory_mapped_read(const struct device *dev,
					      const struct mspi_xfer_packet *packet)
{
	struct mspi_stm32_data *dev_data = dev->data;
	int ret;

	if (!mspi_stm32_qspi_is_memmap(dev)) {
		ret = mspi_stm32_qspi_memmap_on(dev);
		if (ret != 0) {
			LOG_ERR("Failed to enable memory mapped mode");
			return ret;
		}
	}

	uintptr_t mmap_addr = dev_data->memmap_base_addr + packet->address;

	/* Memory-mapped mode is READ-ONLY for QSPI */
	LOG_DBG("Memory-mapped read from 0x%08lx, len %zu", mmap_addr, packet->num_bytes);
	memcpy(packet->data_buf, (void *)mmap_addr, packet->num_bytes);

	return 0;
}

/**
 * @brief Send a Command to the NOR and Receive/Transceive data if relevant in IT or DMA mode.
 *
 * @param dev Pointer to device structure
 * @param packet Pointer to transfer packet
 * @param access_mode Access mode (SYNC or ASYNC)
 * @return 0 on success.
 * @return -Error if fail.
 */
static int mspi_stm32_qspi_access(const struct device *dev, const struct mspi_xfer_packet *packet,
				  uint8_t access_mode)
{
	struct mspi_stm32_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;
	int ret;

	/* === XIP Mode: Handle memory-mapped or indirect mode switching === */
	if (dev_data->xip_cfg.enable) {
		ARG_UNUSED(access_mode);

		/* Read operations can use memory-mapped mode */
		if (!mspi_stm32_qspi_needs_indirect_mode(packet)) {
			return mspi_stm32_qspi_memory_mapped_read(dev, packet);
		}

		/* Commands that need indirect mode*/
		ret = mspi_stm32_qspi_memmap_off(dev);
		if (ret != 0) {
			LOG_ERR("Failed to abort memory-mapped mode");
			return ret;
		}
	}

	/* === Indirect Mode: Standard command + data transfer === */

	/* Prepare QSPI command structure */
	QSPI_CommandTypeDef cmd =
		mspi_stm32_qspi_prepare_cmd(dev_data->dev_cfg.io_mode, dev_data->dev_cfg.data_rate);

	cmd.NbData = packet->num_bytes;
	cmd.Instruction = packet->cmd;
	cmd.DummyCycles = (packet->dir == MSPI_TX) ? dev_data->ctx.xfer.tx_dummy
						   : dev_data->ctx.xfer.rx_dummy;
	cmd.Address = packet->address;
	cmd.AddressSize = mspi_stm32_qspi_hal_address_size(dev_data->ctx.xfer.addr_length);

	if (cmd.NbData == 0) {
		cmd.DataMode = QSPI_DATA_NONE;
	}

	if (cmd.Instruction == MSPI_STM32_CMD_WREN) {
		cmd.AddressMode = QSPI_ADDRESS_NONE;
	}

	hal_ret = HAL_QSPI_Command(&dev_data->hmspi.qspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_QSPI_Command failed: %d", hal_ret);
		return -EIO;
	}

	/* If no data phase, we're done */
	if (packet->num_bytes == 0) {
		return 0;
	}

	/* Execute the data transfer (TX or RX) */
	ret = mspi_stm32_qspi_execute_transfer(dev_data, packet, access_mode);
	if (ret == 0) {
		LOG_DBG("Access %zu data at 0x%lx", packet->num_bytes, (long)(packet->address));
	}

	return ret;
}

/**
 * API implementation of mspi_config : controller configuration.
 *
 * @param spec Pointer to MSPI device tree spec.
 * @return 0 if successful.
 * @return -Error if fail.
 */
static int mspi_stm32_qspi_config(const struct mspi_dt_spec *spec)
{
	const struct device *controller = spec->bus;
	const struct mspi_cfg *config = &spec->config;
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;
	QSPI_HandleTypeDef *hmspi = &data->hmspi.qspi;
	HAL_StatusTypeDef hal_ret;
	uint32_t ahb_clock_freq;
	uint32_t prescaler = MSPI_STM32_CLOCK_PRESCALER_MIN;
	int ret;

	LOG_DBG("Configuring QSPI controller");

	/* Only Controller mode is supported */
	if (config->op_mode != MSPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only support MSPI controller mode.");
		return -ENOTSUP;
	}

	/* Check the max possible freq. */
	if (config->max_freq > MSPI_MAX_FREQ) {
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

	/* Configure pins */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("MSPI pinctrl setup failed");
		return ret;
	}

	if (data->dev_cfg.dqs_enable && !cfg->mspicfg.dqs_support) {
		LOG_ERR("MSPI dqs mismatch (not supported but enabled)");
		return -ENOTSUP;
	}

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Configure IRQ */
	cfg->irq_config();

	/* Clock configuration */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t)&cfg->pclken[0]) != 0) {
		LOG_ERR("Could not enable MSPI clock");
		return -EIO;
	}
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				   (clock_control_subsys_t)&cfg->pclken[0], &ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken)");
		return -EIO;
	}

	/* Calculate prescaler based on desired frequency */
	LOG_DBG("AHB clock: %u Hz, Max freq: %u Hz", ahb_clock_freq, cfg->mspicfg.max_freq);

	for (prescaler = 2; prescaler <= MSPI_STM32_CLOCK_PRESCALER_MAX; prescaler++) {
		data->dev_cfg.freq = MSPI_STM32_CLOCK_COMPUTE(ahb_clock_freq, prescaler);

		if (data->dev_cfg.freq <= cfg->mspicfg.max_freq) {
			break;
		}
	}

	/* Initialize QSPI handle with calculated prescaler */
	hmspi->Init.ClockPrescaler = prescaler;
	hmspi->Init.FifoThreshold = MSPI_STM32_FIFO_THRESHOLD;
	hmspi->Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
	hmspi->Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
	hmspi->Init.ClockMode = QSPI_CLOCK_MODE_0;
	hmspi->Init.FlashID = QSPI_FLASH_ID_1;
	hmspi->Init.DualFlash = QSPI_DUALFLASH_DISABLE;

	/* Initialize HAL QSPI */
	hal_ret = HAL_QSPI_Init(hmspi);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_QSPI_Init failed: %d", hal_ret);
		return -EIO;
	}

	/* Initialize semaphores */
	if (!k_sem_count_get(&data->ctx.lock)) {
		k_sem_give(&data->ctx.lock);
	}

	LOG_INF("QSPI controller configured successfully");

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

		if (device_index >= cfg->mspicfg.num_periph || device_index != dev_id->dev_idx) {
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
 * Validate and set frequency configuration.
 */
static int mspi_stm32_qspi_validate_and_set_freq(struct mspi_stm32_data *data, uint32_t freq)
{
	if (freq > MSPI_MAX_FREQ) {
		LOG_ERR("%u, freq is too large", __LINE__);
		return -ENOTSUP;
	}
	data->dev_cfg.freq = freq;
	return 0;
}

/**
 * Validate and set QSPI IO mode.
 * QSPI hardware doesn't support octal mode.
 */
static int mspi_stm32_qspi_validate_and_set_io_mode(struct mspi_stm32_data *data, uint32_t io_mode)
{
	if (io_mode == MSPI_IO_MODE_OCTAL) {
		LOG_ERR("%u, QSPI doesn't support octal mode", __LINE__);
		return -ENOTSUP;
	}

	if (io_mode >= MSPI_IO_MODE_MAX) {
		LOG_ERR("%u, Invalid io_mode", __LINE__);
		return -EINVAL;
	}

	data->dev_cfg.io_mode = io_mode;
	return 0;
}

/**
 * Validate and set QSPI data rate.
 * Only single data rate (SDR) is currently supported.
 */
static int mspi_stm32_qspi_validate_and_set_data_rate(struct mspi_stm32_data *data,
						      uint32_t data_rate)
{
	if (data_rate != MSPI_DATA_RATE_SINGLE) {
		LOG_ERR("%u, only single data rate supported", __LINE__);
		return -ENOTSUP;
	}

	if (data_rate >= MSPI_DATA_RATE_MAX) {
		LOG_ERR("%u, Invalid data_rate", __LINE__);
		return -EINVAL;
	}

	data->dev_cfg.data_rate = data_rate;
	return 0;
}

/**
 * Validate and set CPP (Clock Polarity/Phase).
 */
static int mspi_stm32_qspi_validate_and_set_cpp(struct mspi_stm32_data *data, uint32_t cpp)
{
	if (cpp > MSPI_CPP_MODE_3) {
		LOG_ERR("%u, Invalid cpp", __LINE__);
		return -EINVAL;
	}
	data->dev_cfg.cpp = cpp;
	return 0;
}

/**
 * Validate and set endianness.
 */
static int mspi_stm32_qspi_validate_and_set_endian(struct mspi_stm32_data *data, uint32_t endian)
{
	if (endian > MSPI_XFER_BIG_ENDIAN) {
		LOG_ERR("%u, Invalid endian", __LINE__);
		return -EINVAL;
	}
	data->dev_cfg.endian = endian;
	return 0;
}

/**
 * Validate and set chip select polarity.
 */
static int mspi_stm32_qspi_validate_and_set_ce_polarity(struct mspi_stm32_data *data,
							uint32_t ce_polarity)
{
	if (ce_polarity > MSPI_CE_ACTIVE_HIGH) {
		LOG_ERR("%u, Invalid ce_polarity", __LINE__);
		return -EINVAL;
	}
	data->dev_cfg.ce_polarity = ce_polarity;
	return 0;
}

/**
 * Validate and set DQS (Data Strobe) configuration.
 */
static int mspi_stm32_qspi_validate_and_set_dqs(struct mspi_stm32_data *data, bool dqs_enable,
						bool dqs_support)
{
	if (dqs_enable && !dqs_support) {
		LOG_ERR("%u, DQS mode not supported", __LINE__);
		return -ENOTSUP;
	}
	data->dev_cfg.dqs_enable = dqs_enable;
	return 0;
}

/**
 * @brief Set transfer-related configuration parameters
 *
 * @param data Pointer to device data
 * @param param_mask Configuration mask
 * @param dev_cfg Device configuration to apply
 */
static void mspi_stm32_qspi_set_transfer_params(struct mspi_stm32_data *data,
						const enum mspi_dev_cfg_mask param_mask,
						const struct mspi_dev_cfg *dev_cfg)
{
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
}

/**
 * Check and save dev_cfg to controller data->dev_cfg.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param param_mask Macro definition of what to be configured in cfg.
 * @param dev_cfg The device runtime configuration for the MSPI controller.
 * @return 0 MSPI device configuration successful.
 * @return -Error MSPI device configuration fail.
 */
static int mspi_stm32_qspi_dev_cfg_save(const struct device *controller,
					const enum mspi_dev_cfg_mask param_mask,
					const struct mspi_dev_cfg *dev_cfg)
{
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;
	int ret = 0;

	if (param_mask & MSPI_DEVICE_CONFIG_CE_NUM) {
		data->dev_cfg.ce_num = dev_cfg->ce_num;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		ret = mspi_stm32_qspi_validate_and_set_freq(data, dev_cfg->freq);
		if (ret != 0) {
			return ret;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		ret = mspi_stm32_qspi_validate_and_set_io_mode(data, dev_cfg->io_mode);
		if (ret != 0) {
			return ret;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		ret = mspi_stm32_qspi_validate_and_set_data_rate(data, dev_cfg->data_rate);
		if (ret != 0) {
			return ret;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		ret = mspi_stm32_qspi_validate_and_set_cpp(data, dev_cfg->cpp);
		if (ret != 0) {
			return ret;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_ENDIAN) {
		ret = mspi_stm32_qspi_validate_and_set_endian(data, dev_cfg->endian);
		if (ret != 0) {
			return ret;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CE_POL) {
		ret = mspi_stm32_qspi_validate_and_set_ce_polarity(data, dev_cfg->ce_polarity);
		if (ret != 0) {
			return ret;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		ret = mspi_stm32_qspi_validate_and_set_dqs(data, dev_cfg->dqs_enable,
							   cfg->mspicfg.dqs_support);
		if (ret != 0) {
			return ret;
		}
	}

	/* Set transfer-related configuration parameters */
	mspi_stm32_qspi_set_transfer_params(data, param_mask, dev_cfg);

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
static int mspi_stm32_qspi_dev_config(const struct device *controller,
				      const struct mspi_dev_id *dev_id,
				      const enum mspi_dev_cfg_mask param_mask,
				      const struct mspi_dev_cfg *dev_cfg)
{
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;
	int ret = 0;

	/* Check if device ID has changed and lock accordingly */
	if (data->dev_id != dev_id) {
		if (k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE))) {
			LOG_ERR("Failed to acquire lock for device config");
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

	if (param_mask == MSPI_DEVICE_CONFIG_NONE && !cfg->mspicfg.sw_multi_periph) {
		/* Just obtaining the controller lock */
		data->dev_id = (struct mspi_dev_id *)dev_id;
		return ret;
	}

	data->dev_id = (struct mspi_dev_id *)dev_id;
	/* Validate and save device configuration */
	ret = mspi_stm32_qspi_dev_cfg_save(controller, param_mask, dev_cfg);
	if (ret) {
		LOG_ERR("failed to change device cfg");
		goto e_return;
	}

e_return:
	k_mutex_unlock(&data->lock);

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
static int mspi_stm32_qspi_xip_config(const struct device *controller,
				      const struct mspi_dev_id *dev_id,
				      const struct mspi_xip_cfg *xip_cfg)
{
	struct mspi_stm32_data *dev_data = controller->data;
	int ret = 0;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("xip_config: dev_id don't match");
		return -ESTALE;
	}

	if (!xip_cfg->enable) {
		/* This is for aborting memory mapped mode */
		ret = mspi_stm32_qspi_memmap_off(controller);
	} else {
		ret = mspi_stm32_qspi_memmap_on(controller);
	}

	if (ret == 0) {
		dev_data->xip_cfg = *xip_cfg;
		LOG_INF("QSPI XIP configured %d", xip_cfg->enable);
	}
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
static int mspi_stm32_qspi_get_channel_status(const struct device *controller, uint8_t ch)
{
	struct mspi_stm32_data *data = controller->data;
	QSPI_HandleTypeDef *hmspi = &data->hmspi.qspi;
	int ret = 0;

	ARG_UNUSED(ch);

	if (mspi_is_inp(controller) || (hmspi->Instance->SR & QUADSPI_SR_BUSY) != 0) {
		ret = -EBUSY;
	}

	data->dev_id = NULL;

	k_mutex_unlock(&data->lock);

	return ret;
}

static int mspi_stm32_qspi_pio_transceive(const struct device *controller,
					  const struct mspi_xfer *xfer)
{
	struct mspi_stm32_data *dev_data = controller->data;
	struct mspi_context *ctx = &dev_data->ctx;
	const struct mspi_xfer_packet *packet;
	uint32_t packet_idx;
	int ret = 0;

	if (xfer->num_packet == 0 || !xfer->packets ||
	    xfer->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		LOG_ERR("Transfer: wrong parameters");
		return -EFAULT;
	}

	/* DummyCycle to give to the mspi_stm32_read_access/mspi_stm32_write_access */
	ret = mspi_context_lock(ctx, dev_data->dev_id, xfer, true);
	if (ret) {
		goto pio_err;
	}

	while (ctx->packets_left > 0) {
		packet_idx = ctx->xfer.num_packet - ctx->packets_left;
		packet = &ctx->xfer.packets[packet_idx];

		/*
		 * Always starts with a command,
		 * then payload is given by the xfer->num_packet
		 */
		ret = mspi_stm32_qspi_access(controller, packet,
					     (ctx->xfer.async == true) ? MSPI_ACCESS_ASYNC
								       : MSPI_ACCESS_SYNC);

		ctx->packets_left--;

		if (ret) {
			LOG_ERR("QSPI access failed for packet %d: %d", packet_idx, ret);
			ret = -EIO;
			goto pio_err;
		}
	}

pio_err:
	k_sem_give(&ctx->lock);
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
static int mspi_stm32_qspi_transceive(const struct device *controller,
				      const struct mspi_dev_id *dev_id,
				      const struct mspi_xfer *xfer)
{
	struct mspi_stm32_data *data = controller->data;

	/* Verify device ID matches */
	if (dev_id != data->dev_id) {
		LOG_ERR("transceive : dev_id don't match");
		return -ESTALE;
	}

	/* Need to map the xfer to the data context */
	data->ctx.xfer = *xfer;

	if (xfer->xfer_mode == MSPI_PIO) {
		return mspi_stm32_qspi_pio_transceive(controller, xfer);
	} else {
		return -EIO;
	}
}

/*
 * HAL MSP Init - Required by STM32 HAL
 */
void HAL_QSPI_MspInit(QSPI_HandleTypeDef *hmspi)
{
	/* The clocks and pins are already configured in mspi_stm32_init */
}

void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef *hmspi)
{
	/* No-op for Zephyr implementation */
}

/**
 * @brief QSPI ISR function
 */
static void mspi_stm32_qspi_isr(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	HAL_QSPI_IRQHandler(&dev_data->hmspi.qspi);
}

/*
 * Driver Initialization
 */
static int mspi_stm32_qspi_init(const struct device *controller)
{
	const struct mspi_stm32_conf *cfg = controller->config;

	LOG_DBG("Initializing QSPI driver");

	/* Initialize QSPI with basic configuration */
	const struct mspi_dt_spec spec = {
		.bus = controller,
		.config = cfg->mspicfg,
	};

	return mspi_stm32_qspi_config(&spec);
}

static struct mspi_driver_api mspi_stm32_qspi_driver_api = {
	.config = mspi_stm32_qspi_config,
	.dev_config = mspi_stm32_qspi_dev_config,
	.xip_config = mspi_stm32_qspi_xip_config,
	.get_channel_status = mspi_stm32_qspi_get_channel_status,
	.transceive = mspi_stm32_qspi_transceive,
};

/* MSPI QSPI control config */
#define MSPI_QSPI_CONFIG(n)                                                                        \
	{                                                                                          \
		.channel_num = 0,                                                                  \
		.op_mode = DT_ENUM_IDX_OR(n, op_mode, MSPI_OP_MODE_CONTROLLER),                    \
		.duplex = DT_ENUM_IDX_OR(n, duplex, MSPI_HALF_DUPLEX),                             \
		.max_freq = DT_INST_PROP_OR(n, mspi_max_frequency, MSPI_MAX_FREQ),                 \
		.dqs_support = false, /* QSPI typically doesn't support DQS */                     \
		.num_periph = DT_INST_CHILD_NUM(n),                                                \
		.sw_multi_periph = DT_INST_PROP_OR(n, software_multiperipheral, false),            \
		.num_ce_gpios = ARRAY_SIZE(ce_gpios##n),                                           \
		.ce_group = ce_gpios##n,                                                           \
	}

#define STM32_MSPI_QSPI_IRQ_HANDLER(index)                                                         \
	static void mspi_stm32_irq_config_func_##index(void)                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),                     \
			    mspi_stm32_qspi_isr, DEVICE_DT_INST_GET(index), 0);                    \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#define MSPI_STM32_QSPI_INIT(index)                                                                \
	static const struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);           \
	static const struct gpio_dt_spec ce_gpios##index[] =                                       \
		MSPI_CE_GPIOS_DT_SPEC_INST_GET(index);                                             \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	STM32_MSPI_QSPI_IRQ_HANDLER(index)                                                         \
	static const struct mspi_stm32_conf mspi_stm32_qspi_dev_conf_##index = {                   \
		.pclken = pclken_##index,                                                          \
		.pclk_len = DT_INST_NUM_CLOCKS(index),                                             \
		.irq_config = mspi_stm32_irq_config_func_##index,                                  \
		.mspicfg = MSPI_QSPI_CONFIG(index),                                                \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(index)),                             \
	};                                                                                         \
	static struct mspi_stm32_data mspi_stm32_qspi_dev_data_##index = {                         \
		.hmspi.qspi =                                                                      \
			{                                                                          \
				.Instance = (QUADSPI_TypeDef *)DT_INST_REG_ADDR(index),            \
				.Init =                                                            \
					{                                                          \
						.ClockPrescaler = 0,                               \
						.FifoThreshold = MSPI_STM32_FIFO_THRESHOLD,        \
						.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE,       \
						.FlashSize = 0x19,                                 \
						.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE,   \
						.ClockMode = QSPI_CLOCK_MODE_0,                    \
						.FlashID = QSPI_FLASH_ID_1,                        \
						.DualFlash = QSPI_DUALFLASH_DISABLE,               \
					},                                                         \
			},                                                                         \
		.memmap_base_addr = DT_REG_ADDR_BY_IDX(DT_DRV_INST(index), 1),                     \
		.dev_id = index,                                                                   \
		.lock = Z_MUTEX_INITIALIZER(mspi_stm32_qspi_dev_data_##index.lock),                \
		.sync = Z_SEM_INITIALIZER(mspi_stm32_qspi_dev_data_##index.sync, 0, 1),            \
		.dev_cfg = {0},                                                                    \
		.xip_cfg = {0},                                                                    \
		.ctx.lock = Z_SEM_INITIALIZER(mspi_stm32_qspi_dev_data_##index.ctx.lock, 0, 1),    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, &mspi_stm32_qspi_init, NULL,                                  \
			      &mspi_stm32_qspi_dev_data_##index,                                   \
			      &mspi_stm32_qspi_dev_conf_##index, POST_KERNEL,                      \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mspi_stm32_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_STM32_QSPI_INIT)
