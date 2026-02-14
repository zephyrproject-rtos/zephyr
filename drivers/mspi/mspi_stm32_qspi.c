/*
 * Copyright (c) 2025 EXALT Technologies.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 *
 * MSPI flash controller driver for stm32 series with QSPI peripheral
 * This driver is based on the stm32Cube HAL QSPI driver
 *
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
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/logging/log.h>
#include <stm32_ll_dma.h>
#include <stm32_bitops.h>
#include "mspi_stm32.h"

LOG_MODULE_REGISTER(mspi_stm32_qspi, CONFIG_MSPI_LOG_LEVEL);

/**
 * @brief Gives a QSPI_CommandTypeDef with all parameters set except Instruction, Address, NbData
 */
static QSPI_CommandTypeDef mspi_stm32_qspi_prepare_cmd(enum mspi_io_mode cfg_mode,
						       enum mspi_data_rate cfg_rate)
{
	/* Command empty structure */
	QSPI_CommandTypeDef cmd_tmp = {0};

	/* QSPI specific configuration */
	cmd_tmp.AddressSize = QSPI_ADDRESS_24_BITS;
	cmd_tmp.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	cmd_tmp.DdrMode =
		cfg_rate == MSPI_DATA_RATE_DUAL ? QSPI_DDR_MODE_ENABLE : QSPI_DDR_MODE_DISABLE;
	cmd_tmp.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	cmd_tmp.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	/* Configure based on IO mode */
	switch (cfg_mode) {
	case MSPI_IO_MODE_QUAD:
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_4_LINES;
		cmd_tmp.AddressMode = QSPI_ADDRESS_4_LINES;
		cmd_tmp.DataMode = QSPI_DATA_4_LINES;
		break;
	case MSPI_IO_MODE_QUAD_1_4_4:
		/* Command uses 1 line, address and data use 4 lines */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = QSPI_ADDRESS_4_LINES;
		cmd_tmp.DataMode = QSPI_DATA_4_LINES;
		break;
	case MSPI_IO_MODE_QUAD_1_1_4:
		/* Command and address use 1 line, data uses 4 lines */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = QSPI_ADDRESS_1_LINE;
		cmd_tmp.DataMode = QSPI_DATA_4_LINES;
		break;
	case MSPI_IO_MODE_DUAL:
		/* All phases use 2 lines */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_2_LINES;
		cmd_tmp.AddressMode = QSPI_ADDRESS_2_LINES;
		cmd_tmp.DataMode = QSPI_DATA_2_LINES;
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
		/* Command uses 1 line, address and data use 2 lines */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = QSPI_ADDRESS_2_LINES;
		cmd_tmp.DataMode = QSPI_DATA_2_LINES;
		break;
	case MSPI_IO_MODE_DUAL_1_1_2:
		/* Command and address use 1 line, data uses 2 lines */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = QSPI_ADDRESS_1_LINE;
		cmd_tmp.DataMode = QSPI_DATA_2_LINES;
		break;
	case MSPI_IO_MODE_OCTAL:
		/* QSPI doesn't support octal mode, fall through to single */
		LOG_WRN("QSPI doesn't support octal mode, using single line");
		__fallthrough;
	case MSPI_IO_MODE_SINGLE:
	default:
		/* All phases use 1 line */
		cmd_tmp.InstructionMode = QSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = QSPI_ADDRESS_1_LINE;
		cmd_tmp.DataMode = QSPI_DATA_1_LINE;
		break;
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
static bool mspi_is_inp(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	return k_sem_count_get(&dev_data->ctx.lock) == 0;
}

static uint32_t mspi_stm32_qspi_hal_address_size(uint8_t address_length)
{
	if (address_length == 4U) {
		return QSPI_ADDRESS_32_BITS;
	}

	return QSPI_ADDRESS_24_BITS;
}

/*
 * DMA Support
 */
#if defined(CONFIG_MSPI_DMA)
/**
 * @brief Initialize DMA for QSPI
 *
 * Configures both Zephyr DMA driver and HAL DMA driver for QSPI transfers.
 * Due to use of QSPI HAL API, both drivers need to be configured.
 *
 * @param hdma Pointer to HAL DMA handle
 * @param dma_stream Pointer to Zephyr DMA stream structure
 * @return 0 on success, negative errno on failure
 */
static int mspi_stm32_qspi_dma_init(DMA_HandleTypeDef *hdma, struct stm32_stream *dma_stream)
{
	int ret;

	/* Check if DMA device is ready */
	if (!device_is_ready(dma_stream->dev)) {
		LOG_ERR("DMA %s device not ready", dma_stream->dev->name);
		return -ENODEV;
	}

	/* Configure Zephyr DMA driver */
	dma_stream->cfg.user_data = hdma;
	/* This field is used to inform driver that it is overridden */
	dma_stream->cfg.linked_channel = STM32_DMA_HAL_OVERRIDE;

	ret = dma_config(dma_stream->dev, dma_stream->channel, &dma_stream->cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d", dma_stream->channel);
		return ret;
	}

	/* Validate data size alignment */
	if (dma_stream->cfg.source_data_size != dma_stream->cfg.dest_data_size) {
		LOG_ERR("DMA Source and destination data sizes not aligned");
		return -EINVAL;
	}

	/* Configure HAL DMA driver for QSPI */
	int index = find_lsb_set(dma_stream->cfg.source_data_size) - 1;

	hdma->Init.PeriphDataAlignment = mspi_stm32_table_dest_size[index];
	hdma->Init.MemDataAlignment = mspi_stm32_table_src_size[index];
	hdma->Init.PeriphInc = DMA_PINC_DISABLE;
	hdma->Init.MemInc = DMA_MINC_ENABLE;
	hdma->Init.Mode = DMA_NORMAL;
	hdma->Init.Priority = mspi_stm32_table_priority[dma_stream->cfg.channel_priority];
	hdma->Init.Direction = mspi_stm32_table_direction[dma_stream->cfg.channel_direction];
#ifdef CONFIG_DMA_STM32_V1
	/* TODO: Not tested in this configuration */
	hdma->Init.Channel = dma_stream->cfg.dma_slot;
#else
	hdma->Init.Request = dma_stream->cfg.dma_slot;
#endif /* CONFIG_DMA_STM32_V1 */

	/* Get DMA channel instance */
	hdma->Instance = STM32_DMA_GET_CHANNEL_INSTANCE(dma_stream->reg, dma_stream->channel);

	/* Initialize HAL DMA */
	if (HAL_DMA_Init(hdma) != HAL_OK) {
		LOG_ERR("QSPI DMA Init failed");
		return -EIO;
	}

	LOG_DBG("QSPI DMA initialized");
	return 0;
}

/**
 * @brief Setup DMA for QSPI controller
 *
 * @param dev_cfg Device configuration
 * @param dev_data Device data
 * @return 0 on success, negative errno on failure
 */
static int mspi_stm32_qspi_dma_setup(const struct mspi_stm32_conf *dev_cfg,
				     struct mspi_stm32_data *dev_data)
{
	int ret;

	if (!dev_cfg->dma_specified) {
		LOG_ERR("DMA configuration is missing from the device tree");
		return -EIO;
	}

	/* Initialize DMA */
	ret = mspi_stm32_qspi_dma_init(&dev_data->hdma, &dev_data->dma);
	if (ret != 0) {
		LOG_ERR("QSPI DMA init failed");
		return ret;
	}

	/* Link DMA to QSPI HAL handle */
	__HAL_LINKDMA(&dev_data->hmspi.qspi, hdma, dev_data->hdma);

	LOG_DBG("QSPI with DMA Transfer configured");
	return 0;
}

/**
 * @brief DMA callback for QSPI transfers
 *
 * Routes DMA interrupts to HAL DMA IRQ handler
 */
static void mspi_stm32_qspi_dma_callback(const struct device *dev, void *arg,
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
#endif /* CONFIG_MSPI_DMA */

/* Check if device is in memory-mapped mode */
static bool mspi_stm32_qspi_is_memmap(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	/* Check the FMODE bits in CCR register to see if in memory-mapped mode */
	return stm32_reg_read_bits(&dev_data->hmspi.qspi.Instance->CCR, QUADSPI_CCR_FMODE) ==
	       QUADSPI_CCR_FMODE;
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

	s_command = mspi_stm32_qspi_prepare_cmd(dev_data->dev_cfg.io_mode,
						dev_data->dev_cfg.data_rate);

	/* Set read command - use the configured read command if available */
	if (dev_data->dev_cfg.read_cmd != 0) {
		s_command.Instruction = dev_data->dev_cfg.read_cmd;
	} else {
		/* Fallback to standard fast read commands */
		if (dev_data->dev_cfg.addr_length == 4) {
			s_command.Instruction = MSPI_NOR_CMD_READ_FAST_4B;
		} else {
			s_command.Instruction = MSPI_NOR_CMD_READ_FAST;
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
 * @return false if command can use memory-mapped mode.
 */
static bool mspi_stm32_qspi_needs_indirect_mode(const struct mspi_xfer_packet *packet)
{
	return (packet->cmd == MSPI_NOR_CMD_WREN) || (packet->cmd == MSPI_NOR_CMD_SE) ||
	       (packet->cmd == MSPI_NOR_CMD_SE_4B) || (packet->cmd == MSPI_NOR_CMD_RDSR) ||
	       (packet->dir == MSPI_TX);
}

/**
 * @brief Execute data transfer (TX or RX) in indirect mode
 *
 * @param dev Pointer to device structure
 * @param packet Pointer to transfer packet
 * @param access_mode Access mode (SYNC, ASYNC, or DMA)
 * @return 0 on success
 * @return A negative errno value upon failure.
 */
static int mspi_stm32_qspi_execute_transfer(const struct device *dev,
					    const struct mspi_xfer_packet *packet,
					    uint8_t access_mode)
{
	HAL_StatusTypeDef hal_ret;
	struct mspi_stm32_data *dev_data = dev->data;

	if (packet->dir == MSPI_RX) {
		/* Receive the data */
		switch (access_mode) {
		case MSPI_ACCESS_SYNC:
			hal_ret = HAL_QSPI_Receive(&dev_data->hmspi.qspi, packet->data_buf,
						   HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
			break;
		case MSPI_ACCESS_ASYNC:
			hal_ret = HAL_QSPI_Receive_IT(&dev_data->hmspi.qspi, packet->data_buf);
			break;
		case MSPI_ACCESS_DMA:
#if defined(CONFIG_MSPI_DMA)
			hal_ret = HAL_QSPI_Receive_DMA(&dev_data->hmspi.qspi, packet->data_buf);
#else
			LOG_ERR("DMA mode not enabled (CONFIG_MSPI_DMA not set)");
			return -ENOTSUP;
#endif
			break;
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
			break;
		case MSPI_ACCESS_ASYNC:
			hal_ret = HAL_QSPI_Transmit_IT(&dev_data->hmspi.qspi, packet->data_buf);
			break;
		case MSPI_ACCESS_DMA:
#if defined(CONFIG_MSPI_DMA)
			hal_ret = HAL_QSPI_Transmit_DMA(&dev_data->hmspi.qspi, packet->data_buf);
#else
			LOG_ERR("DMA mode not enabled (CONFIG_MSPI_DMA not set)");
			return -ENOTSUP;
#endif
			break;
		default:
			LOG_ERR("Invalid access mode: %d", access_mode);
			return -EINVAL;
		}
	}

	if ((hal_ret != HAL_OK) || (access_mode == MSPI_ACCESS_SYNC)) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);

		if (hal_ret != HAL_OK) {
			LOG_ERR("Failed to start %s transfer: %d",
				packet->dir == MSPI_RX ? "receive" : "transmit", hal_ret);
			return -EIO;
		}

		return 0;
	}

	/* For ASYNC mode, wait for IRQ completion (PM locks released in ISR) */
	if (k_sem_take(&dev_data->sync, K_FOREVER) < 0) {
		LOG_ERR("Failed to complete async transfer");
		/* If semaphore wait fails, ISR never completed, so release PM locks */
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		return -EIO;
	}

	return 0;
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
 * @return A negative errno value upon failure.
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
 * @return A negative errno value upon failure.
 */
static int mspi_stm32_qspi_access(const struct device *dev, const struct mspi_xfer_packet *packet,
				  uint8_t access_mode)
{
	struct mspi_stm32_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;
	int ret;

	/* === XIP Mode: Handle memory-mapped or indirect mode switching === */
	if (dev_data->xip_cfg.enable) {
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

	/* Acquire PM locks for indirect mode operations */
	(void)pm_device_runtime_get(dev);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	/* Prepare QSPI command structure */
	QSPI_CommandTypeDef cmd = mspi_stm32_qspi_prepare_cmd(dev_data->dev_cfg.io_mode,
							      dev_data->dev_cfg.data_rate);

	cmd.NbData = packet->num_bytes;
	cmd.Instruction = packet->cmd;
	cmd.DummyCycles = packet->dir == MSPI_TX ? dev_data->ctx.xfer.tx_dummy
					  : dev_data->ctx.xfer.rx_dummy;
	cmd.Address = packet->address;
	cmd.AddressSize = mspi_stm32_qspi_hal_address_size(dev_data->ctx.xfer.addr_length);

	if (cmd.NbData == 0) {
		cmd.DataMode = QSPI_DATA_NONE;
	}

	if (cmd.Instruction == MSPI_NOR_CMD_WREN) {
		cmd.AddressMode = QSPI_ADDRESS_NONE;
	}

	hal_ret = HAL_QSPI_Command(&dev_data->hmspi.qspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_QSPI_Command failed: %d", hal_ret);
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		return -EIO;
	}

	/* If no data phase, we're done */
	if (packet->num_bytes == 0) {
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		(void)pm_device_runtime_put(dev);
		return 0;
	}

	/* Execute the data transfer (TX or RX) */
	return mspi_stm32_qspi_execute_transfer(dev, packet, access_mode);
}

/**
 * @brief Validate MSPI configuration parameters
 *
 * @param config Pointer to MSPI configuration
 * @return 0 on success, negative errno on failure
 */
static int mspi_stm32_qspi_conf_validate(const struct mspi_cfg *config, uint32_t max_frequency)
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

/**
 * @brief Configure QSPI clocks and calculate prescaler
 *
 * @param cfg Device configuration
 * @param data Device data
 * @return 0 on success, negative errno on failure
 */
static int mspi_stm32_qspi_clock_config(const struct mspi_stm32_conf *cfg,
					struct mspi_stm32_data *data)
{
	uint32_t ahb_clock_freq;
	uint32_t prescaler;

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
	for (prescaler = MSPI_STM32_CLOCK_PRESCALER_MIN;
	     prescaler <= MSPI_STM32_CLOCK_PRESCALER_MAX; prescaler++) {
		data->dev_cfg.freq = MSPI_STM32_CLOCK_COMPUTE(ahb_clock_freq, prescaler);

		if (data->dev_cfg.freq <= cfg->mspicfg.max_freq) {
			break;
		}
	}

	__ASSERT_NO_MSG(prescaler <= MSPI_STM32_CLOCK_PRESCALER_MAX);

	/* Set prescaler in QSPI HAL handle */
	data->hmspi.qspi.Init.ClockPrescaler = prescaler;

	return 0;
}

/**
 * @brief Initialize QSPI HAL
 *
 * @param hmspi QSPI HAL handle
 * @return 0 on success, negative errno on failure
 */
static int mspi_stm32_qspi_hal_init(QSPI_HandleTypeDef *hmspi)
{
	HAL_StatusTypeDef hal_ret;

	/* Initialize remaining QSPI handle parameters */
	hmspi->Init.FifoThreshold = MSPI_STM32_FIFO_THRESHOLD;
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

	return 0;
}

/**
 * API implementation of mspi_config : controller configuration.
 *
 * @param spec Pointer to MSPI device tree spec.
 * @return 0 if successful.
 * @return A negative errno value upon failure.
 */
static int mspi_stm32_qspi_config(const struct mspi_dt_spec *spec)
{
	const struct device *controller = spec->bus;
	const struct mspi_cfg *config = &spec->config;
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;
	int ret;

	LOG_DBG("Configuring QSPI controller");

	ret = mspi_stm32_qspi_conf_validate(config, cfg->mspicfg.max_freq);
	if (ret != 0) {
		return ret;
	}

	(void)pm_device_runtime_get(controller);
	/* Prevent the clocks to be stopped during the request */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	/* Configure pins */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("MSPI pinctrl setup failed");
		goto end;
	}

	if (data->dev_cfg.dqs_enable && !cfg->mspicfg.dqs_support) {
		LOG_ERR("MSPI dqs mismatch (not supported but enabled)");
		ret = -ENOTSUP;
		goto end;
	}

	/* Configure IRQ */
	cfg->irq_config();

	/* Configure clocks and calculate prescaler */
	ret = mspi_stm32_qspi_clock_config(cfg, data);
	if (ret != 0) {
		goto end;
	}

	/* Initialize HAL */
	ret = mspi_stm32_qspi_hal_init(&data->hmspi.qspi);
	if (ret != 0) {
		goto end;
	}

#if defined(CONFIG_MSPI_DMA)
	/* Setup DMA if specified in device tree */
	if (cfg->dma_specified) {
		ret = mspi_stm32_qspi_dma_setup(cfg, data);
		if (ret != 0) {
			LOG_ERR("QSPI DMA setup failed: %d", ret);
			goto end;
		}
	}
#endif

	/* Initialize semaphores */
	if (k_sem_count_get(&data->ctx.lock) == 0) {
		k_sem_give(&data->ctx.lock);
	}

	LOG_INF("QSPI controller configured successfully");

end:
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(controller);

	return ret;
}

/**
 * Validate and set frequency configuration.
 */
static int mspi_stm32_qspi_validate_and_set_freq(struct mspi_stm32_data *data, uint32_t freq,
						 uint32_t max_frequency)
{
	if (freq > max_frequency) {
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
 * @return A negative errno value upon failure.
 */
static int mspi_stm32_qspi_dev_cfg_save(const struct device *controller,
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
		ret = mspi_stm32_qspi_validate_and_set_freq(data, dev_cfg->freq,
							    cfg->mspicfg.max_freq);
		if (ret != 0) {
			return ret;
		}
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0) {
		ret = mspi_stm32_qspi_validate_and_set_io_mode(data, dev_cfg->io_mode);
		if (ret != 0) {
			return ret;
		}
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) != 0) {
		ret = mspi_stm32_qspi_validate_and_set_data_rate(data, dev_cfg->data_rate);
		if (ret != 0) {
			return ret;
		}
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CPP) != 0) {
		ret = mspi_stm32_qspi_validate_and_set_cpp(data, dev_cfg->cpp);
		if (ret != 0) {
			return ret;
		}
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_ENDIAN) != 0) {
		ret = mspi_stm32_qspi_validate_and_set_endian(data, dev_cfg->endian);
		if (ret != 0) {
			return ret;
		}
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CE_POL) != 0) {
		ret = mspi_stm32_qspi_validate_and_set_ce_polarity(data, dev_cfg->ce_polarity);
		if (ret != 0) {
			return ret;
		}
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_DQS) != 0) {
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
	bool locked = false;
	int ret = 0;

	/* Check if device ID has changed and lock accordingly */
	if (data->dev_id != dev_id) {
		if (k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE))) {
			LOG_ERR("Failed to acquire lock for device config");
			return -EBUSY;
		}

		locked = true;
	}

	if (mspi_is_inp(controller)) {
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

	data->dev_id = dev_id;
	/* Validate and save device configuration */
	ret = mspi_stm32_qspi_dev_cfg_save(controller, param_mask, dev_cfg);
	if (ret != 0) {
		LOG_ERR("failed to change device cfg");
	}

	/* Release PM resources */
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

	ret = pm_device_runtime_get(controller);
	if (ret != 0) {
		LOG_ERR("%u, pm_device_runtime_get() failed: %d", __LINE__, ret);
		return ret;
	}

	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

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

	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	if (pm_device_runtime_put(controller)) {
		LOG_ERR("%u, pm_device_runtime_put() failed", __LINE__);
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

	return ret;
}

static int mspi_stm32_qspi_pio_transceive(const struct device *controller,
					  const struct mspi_xfer *xfer)
{
	struct mspi_stm32_data *dev_data = controller->data;
	struct mspi_stm32_context *ctx = &dev_data->ctx;
	const struct mspi_xfer_packet *packet;
	uint32_t packet_idx;
	int ret = 0;

	if (xfer->num_packet == 0 || xfer->packets == NULL ||
	    xfer->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		LOG_ERR("Transfer: wrong parameters");
		return -EFAULT;
	}

	/* Acquire the context lock (semaphore) */
	if (k_sem_take(&ctx->lock, K_MSEC(xfer->timeout)) < 0) {
		return -EBUSY;
	}

	ctx->xfer = *xfer;
	ctx->packets_left = ctx->xfer.num_packet;

	while (ctx->packets_left > 0) {
		packet_idx = ctx->xfer.num_packet - ctx->packets_left;
		packet = &ctx->xfer.packets[packet_idx];

		/*
		 * Always starts with a command,
		 * then payload is given by the xfer->num_packet
		 */
		ret = mspi_stm32_qspi_access(controller, packet, ctx->xfer.async ?
					     MSPI_ACCESS_ASYNC : MSPI_ACCESS_SYNC);

		if (ret != 0) {
			LOG_ERR("QSPI access failed for packet %d: %d", packet_idx, ret);
			ret = -EIO;
			goto out;
		}

		ctx->packets_left--;
	}

out:
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
 * @retval A negative errno value upon failure.
 */
static int mspi_stm32_qspi_transceive(const struct device *controller,
				      const struct mspi_dev_id *dev_id,
				      const struct mspi_xfer *xfer)
{
	struct mspi_stm32_data *data = controller->data;
	int ret = 0;

	/* Verify device ID matches */
	if (dev_id != data->dev_id) {
		LOG_ERR("transceive : dev_id don't match");
		return -ESTALE;
	}

	/* Need to map the xfer to the data context */
	data->ctx.xfer = *xfer;

	if (xfer->xfer_mode == MSPI_PIO) {
		ret = mspi_stm32_qspi_pio_transceive(controller, xfer);
	} else {
		ret = -EIO;
	}

	return ret;
}

/**
 * @brief QSPI ISR function
 */
static void mspi_stm32_qspi_isr(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	HAL_QSPI_IRQHandler(&dev_data->hmspi.qspi);

	k_sem_give(&dev_data->sync);
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	(void)pm_device_runtime_put(dev);
}

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Power management action callback
 */
static int mspi_stm32_qspi_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct mspi_stm32_conf *cfg = dev->config;
	struct mspi_stm32_data *dev_data = dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Cannot apply default pins state (%d)", ret);
			return ret;
		}

		/* Re-enable clock */
		if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				     (clock_control_subsys_t)&cfg->pclken[0]) != 0) {
			LOG_ERR("Could not enable MSPI clock on resume");
			return -EIO;
		}

		LOG_DBG("QSPI resumed");
		return 0;

	case PM_DEVICE_ACTION_SUSPEND:
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
		if (ret < 0 && ret != -ENOENT) {
			LOG_ERR("Cannot apply sleep pins state (%d)", ret);
			return ret;
		}

		/* Check if XIP is enabled or if controller is in use */
		if (dev_data->xip_cfg.enable || k_mutex_lock(&dev_data->lock, K_NO_WAIT) != 0) {
			LOG_ERR("Controller in use, cannot be suspended");
			return -EBUSY;
		}

		/* Disable QSPI peripheral */
		if (HAL_QSPI_DeInit(&dev_data->hmspi.qspi) != HAL_OK) {
			LOG_WRN("HAL_QSPI_DeInit failed during suspend");
		}

		/* Disable clock */
		if (clock_control_off(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				      (clock_control_subsys_t)&cfg->pclken[0]) != 0) {
			LOG_WRN("Could not disable MSPI clock on suspend");
		}

		k_mutex_unlock(&dev_data->lock);

		LOG_DBG("QSPI suspended");
		return 0;

	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

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

static DEVICE_API(mspi, mspi_stm32_qspi_driver_api) = {
	.config = mspi_stm32_qspi_config,
	.dev_config = mspi_stm32_qspi_dev_config,
	.xip_config = mspi_stm32_qspi_xip_config,
	.get_channel_status = mspi_stm32_qspi_get_channel_status,
	.transceive = mspi_stm32_qspi_transceive,
};

/*
 * DMA Configuration Macros
 */
#if defined(CONFIG_MSPI_DMA)
#define DMA_CHANNEL_CONFIG(node, dir) DT_DMAS_CELL_BY_NAME(node, dir, channel_config)

#define QSPI_DMA_CHANNEL_INIT(node, dir)                                                      \
	.dev = DEVICE_DT_GET(DT_DMAS_CTLR(node)),                                             \
	.channel = DT_DMAS_CELL_BY_NAME(node, dir, channel),                                  \
	.reg = (DMA_TypeDef *)DT_REG_ADDR(DT_PHANDLE_BY_NAME(node, dmas, dir)),               \
	.cfg = {                                                                              \
		.dma_slot = DT_DMAS_CELL_BY_NAME(node, dir, slot),                            \
		.source_data_size =                                                           \
			STM32_DMA_CONFIG_PERIPHERAL_DATA_SIZE(DMA_CHANNEL_CONFIG(node, dir)), \
		.dest_data_size =                                                             \
			STM32_DMA_CONFIG_MEMORY_DATA_SIZE(DMA_CHANNEL_CONFIG(node, dir)),     \
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(DMA_CHANNEL_CONFIG(node, dir)), \
		.dma_callback = mspi_stm32_qspi_dma_callback,                                 \
	},

#define QSPI_DMA_CHANNEL(node, dir)                                                           \
	.dma = {                                                                              \
		COND_CODE_1(DT_DMAS_HAS_NAME(node, dir),                                      \
			    (QSPI_DMA_CHANNEL_INIT(node, dir)),                               \
			    (NULL))                                                           \
	},
#else
#define QSPI_DMA_CHANNEL(node, dir)
#endif /* CONFIG_MSPI_DMA */

/* MSPI QSPI control config */
#define MSPI_QSPI_CONFIG(index)                                                                \
	{                                                                                      \
		.channel_num = 0,                                                              \
		.op_mode = DT_INST_ENUM_IDX_OR(index, op_mode, MSPI_OP_MODE_CONTROLLER),       \
		.duplex = DT_INST_ENUM_IDX_OR(index, duplex, MSPI_HALF_DUPLEX),                \
		.max_freq = DT_INST_PROP(index, clock_frequency),                              \
		.dqs_support = false, /* QSPI typically doesn't support DQS */                 \
		.num_periph = DT_INST_CHILD_NUM(index),                                        \
		.sw_multi_periph = DT_INST_PROP(index, software_multiperipheral),              \
		.num_ce_gpios = ARRAY_SIZE(ce_gpios##index),                                   \
		.ce_group = ce_gpios##index,                                                   \
	}

#define STM32_MSPI_QSPI_IRQ_HANDLER(index)                                                     \
	static void mspi_stm32_irq_config_func_##index(void)                                   \
	{                                                                                      \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),                 \
			    mspi_stm32_qspi_isr, DEVICE_DT_INST_GET(index), 0);                \
		irq_enable(DT_INST_IRQN(index));                                               \
	}

#define MSPI_STM32_QSPI_INIT(index)                                                            \
	static const struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);       \
                                                                                               \
	static struct gpio_dt_spec ce_gpios##index[] = MSPI_CE_GPIOS_DT_SPEC_INST_GET(index);  \
                                                                                               \
	PINCTRL_DT_INST_DEFINE(index);                                                         \
                                                                                               \
	STM32_MSPI_QSPI_IRQ_HANDLER(index)                                                     \
                                                                                               \
	PM_DEVICE_DT_INST_DEFINE(index, mspi_stm32_qspi_pm_action);                            \
                                                                                               \
	static const struct mspi_stm32_conf mspi_stm32_qspi_dev_conf_##index = {               \
		.pclken = pclken_##index,                                                      \
		.pclk_len = DT_INST_NUM_CLOCKS(index),                                         \
		.irq_config = mspi_stm32_irq_config_func_##index,                              \
		.mspicfg = MSPI_QSPI_CONFIG(index),                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                 \
		.dma_specified = DT_INST_NODE_HAS_PROP(index, dmas),                           \
	};                                                                                     \
                                                                                               \
	static struct mspi_stm32_data mspi_stm32_qspi_dev_data_##index = {                     \
		.hmspi.qspi = {                                                                \
			.Instance = (QUADSPI_TypeDef *)DT_INST_REG_ADDR(index),                \
			.Init = {                                                              \
				.ClockPrescaler = 0,                                           \
				.FifoThreshold = MSPI_STM32_FIFO_THRESHOLD,                    \
				.SampleShifting = (DT_INST_PROP(index, st_ssht_enable)         \
						  ? QSPI_SAMPLE_SHIFTING_HALFCYCLE             \
						  : QSPI_SAMPLE_SHIFTING_NONE),                \
				.FlashSize = 0x19,                                             \
				.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE,               \
				.ClockMode = QSPI_CLOCK_MODE_0,                                \
				.FlashID = QSPI_FLASH_ID_1,                                    \
				.DualFlash = QSPI_DUALFLASH_DISABLE,                           \
			},                                                                     \
		},                                                                             \
		.memmap_base_addr = DT_INST_REG_ADDR_BY_IDX(index, 1),                         \
		.dev_id = index,                                                               \
		.lock = Z_MUTEX_INITIALIZER(mspi_stm32_qspi_dev_data_##index.lock),            \
		.sync = Z_SEM_INITIALIZER(mspi_stm32_qspi_dev_data_##index.sync, 0, 1),        \
		.dev_cfg = {0},                                                                \
		.xip_cfg = {0},                                                                \
		.ctx.lock = Z_SEM_INITIALIZER(mspi_stm32_qspi_dev_data_##index.ctx.lock, 0, 1),\
		QSPI_DMA_CHANNEL(DT_DRV_INST(index), tx_rx)                                    \
	};                                                                                     \
                                                                                               \
	DEVICE_DT_INST_DEFINE(index, &mspi_stm32_qspi_init,                                    \
			      PM_DEVICE_DT_INST_GET(index),                                    \
			      &mspi_stm32_qspi_dev_data_##index,                               \
			      &mspi_stm32_qspi_dev_conf_##index, POST_KERNEL,                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mspi_stm32_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_STM32_QSPI_INIT)
