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

#include <errno.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stm32_ll_system.h>
#include <stm32h7xx_hal_qspi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi/devicetree.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mspi_stm32_qspi, CONFIG_MSPI_LOG_LEVEL);

#define MSPI_NOR_STM32_NODE DT_CHILD(0)

/* Get the base address of the flash from the DTS node */
#define MSPI_STM32_BASE_ADDRESS DT_INST_REG_ADDR(0)

#define MSPI_STM32_RESET_GPIO DT_INST_NODE_HAS_PROP(0, reset_gpios)


#include "mspi_stm32.h"

static inline int mspi_context_lock(struct mspi_context *ctx, const struct mspi_dev_id *req,
				    const struct mspi_xfer *xfer, bool lockon)
{
	int ret = 0;
	if (k_sem_take(&ctx->lock, K_MSEC(xfer->timeout))) {
		return -EBUSY;
	}

	ctx->xfer = *xfer;
	ctx->packets_done = 0;
	ctx->packets_left = ctx->xfer.num_packet;
	return ret;
}

/**
 * @brief Gives a QSPI_CommandTypeDef with all parameters set except Instruction, Address, NbData
 */
static QSPI_CommandTypeDef mspi_stm32_prepare_cmd(uint8_t cfg_mode, uint8_t cfg_rate)
{
	/* Command empty structure */
	QSPI_CommandTypeDef cmd_tmp = {0};

	/* QSPI specific configuration */
	cmd_tmp.AddressSize = QSPI_ADDRESS_24_BITS;
	cmd_tmp.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	cmd_tmp.DdrMode = ((cfg_rate == MSPI_DATA_RATE_DUAL) ? QSPI_DDR_MODE_ENABLE 
							: QSPI_DDR_MODE_DISABLE);	
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

static uint32_t mspi_stm32_hal_address_size(uint8_t address_length)
{
	if (address_length == 4U) {
		return QSPI_ADDRESS_32_BITS;
	}

	return QSPI_ADDRESS_24_BITS;
}

/**
 * @brief Send a Command to the NOR and Receive/Transceive data if relevant in IT or DMA mode.
 *
 */
static int mspi_stm32_access(const struct device *dev, 
				   const struct mspi_xfer_packet *packet,
				   uint8_t access_mode)
{
	struct mspi_stm32_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;
	QSPI_CommandTypeDef cmd = 
		mspi_stm32_prepare_cmd(dev_data->dev_cfg.io_mode, dev_data->dev_cfg.data_rate);
	
	cmd.NbData = packet->num_bytes;
	cmd.Instruction = packet->cmd;
	if (packet->dir == MSPI_TX) {
		cmd.DummyCycles = dev_data->ctx.xfer.tx_dummy;
	} else {
		cmd.DummyCycles = dev_data->ctx.xfer.rx_dummy;
	}
	cmd.Address = packet->address;
	cmd.AddressSize = mspi_stm32_hal_address_size(dev_data->ctx.xfer.addr_length);
	if (cmd.NbData == 0) {
		cmd.DataMode = QSPI_DATA_NONE;
	}
	
	if (cmd.Instruction == MSPI_STM32_CMD_WREN) {
		cmd.AddressMode = QSPI_ADDRESS_NONE;
	}

	hal_ret = HAL_QSPI_Command(&dev_data->hmspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_QSPI_Command failed: %d", hal_ret);
		return -EIO;
	}

	if (packet->num_bytes == 0) {
		return 0;
	}

	if (packet->dir == MSPI_RX) {
		/* Receive the data */
		switch (access_mode) {
		case MSPI_ACCESS_SYNC:
			hal_ret = HAL_QSPI_Receive(&dev_data->hmspi, packet->data_buf,
						   HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
			goto e_access;
		case MSPI_ACCESS_ASYNC:
			hal_ret = HAL_QSPI_Receive_IT(&dev_data->hmspi, packet->data_buf);
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
			hal_ret = HAL_QSPI_Transmit(&dev_data->hmspi, packet->data_buf,
						    HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
			goto e_access;
		case MSPI_ACCESS_ASYNC:
			hal_ret = HAL_QSPI_Transmit_IT(&dev_data->hmspi, packet->data_buf);
			break;
		default:
			/* Not correct */
			hal_ret = HAL_BUSY;
			break;
		}
	}

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to access data", hal_ret);
		return -EIO;
	}

	/* Lock again expecting the IRQ for end of Tx or Rx */
	if (k_sem_take(&dev_data->sync, K_FOREVER)) {
		LOG_ERR("%d: Failed to access data", hal_ret);
		return -EIO;
	}

e_access:
	LOG_DBG("Access %zu data at 0x%lx", packet->num_bytes, (long)(packet->address));

	return 0;
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
	const struct device *controller = spec->bus;
	const struct mspi_cfg *config = &spec->config;
	const struct mspi_stm32_conf *cfg = controller->config;
	struct mspi_stm32_data *data = controller->data;
	QSPI_HandleTypeDef *hmspi = &data->hmspi;
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
				   (clock_control_subsys_t)&cfg->pclken[0],
				   &ahb_clock_freq) < 0) {
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
 * Check and save dev_cfg to controller data->dev_cfg.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param param_mask Macro definition of what to be configured in cfg.
 * @param dev_cfg The device runtime configuration for the MSPI controller.
 * @return 0 MSPI device configuration successful.
 * @return -Error MSPI device configuration fail.
 */
static int mspi_dev_cfg_save(const struct device *controller,
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

	/* Validate QSPI capabilities */
	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		if (dev_cfg->io_mode == MSPI_IO_MODE_OCTAL) {
			LOG_ERR("QSPI doesn't support octal mode");
			ret = -ENOTSUP;
			goto e_return;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		if (dev_cfg->data_rate != MSPI_DATA_RATE_SINGLE) {
			LOG_ERR("QSPI only supports single data rate");
			ret = -ENOTSUP;
			goto e_return;
		}
	}

	/*
	 * The SFDP is able to change the addr_length 4bytes or 3bytes
	 * this is reflected by the serial_cfg
	 */
	data->dev_id = (struct mspi_dev_id *)dev_id;
	/* Go on with other parameters if supported */
	if(mspi_dev_cfg_save(controller, param_mask, dev_cfg)){
		LOG_ERR("failed to change device cfg");
		return -1;
	}

e_return:
	k_mutex_unlock(&data->lock);

	return ret;
}

/* Set the device back in command mode */
static int mspi_stm32_memmap_off(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;

	if (HAL_QSPI_Abort(&dev_data->hmspi) != HAL_OK) {
		LOG_ERR("QSPI MemMapped abort failed");
		return -EIO;
	}
	return 0;
}

/* Set the device in MemMapped mode */
static int mspi_stm32_memmap_on(const struct device *controller)
{
	struct mspi_stm32_data *dev_data = controller->data;
	QSPI_CommandTypeDef s_command;
	QSPI_MemoryMappedTypeDef s_MemMappedCfg;
	HAL_StatusTypeDef hal_ret;

	LOG_DBG("Enabling QSPI memory mapped mode");

	/* Configure the read command for memory mapped mode */
	s_command = mspi_stm32_prepare_cmd(dev_data->dev_cfg.io_mode, dev_data->dev_cfg.data_rate);
	
	/* Set read command based on addressing mode */
	if (dev_data->dev_cfg.addr_length == 4) {
		s_command.Instruction = MSPI_STM32_CMD_READ_FAST_4B;  /* 4-byte address read */
	} else {
		s_command.Instruction = MSPI_STM32_CMD_READ_FAST;     /* 3-byte address read */
	}
	
	s_command.AddressSize = mspi_stm32_hal_address_size(dev_data->ctx.xfer.addr_length);
	s_command.DummyCycles = dev_data->dev_cfg.rx_dummy;  /* Use configured dummy cycles */


	LOG_DBG("QSPI memmap read cmd: 0x%02x, addr_size: %d, dummy: %d", 
		s_command.Instruction, dev_data->dev_cfg.addr_length, s_command.DummyCycles);

	/* Send read command configuration */
	hal_ret = HAL_QSPI_Command(&dev_data->hmspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to configure QSPI read command for memory mapping: %d", hal_ret);
		return -EIO;
	}

	/* Configure the write command for memory mapped mode */
	if (dev_data->dev_cfg.addr_length == 4) {
		s_command.Instruction = MSPI_STM32_CMD_PP_4B;  /* 4-byte address page program */
	} else {
		s_command.Instruction = MSPI_STM32_CMD_PP;     /* 3-byte address page program */
	}
	
	s_command.DummyCycles = 0;  /* No dummy cycles for write */
	
	/* For page program, typically use single line for instruction and address */
	s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	s_command.AddressMode = QSPI_ADDRESS_1_LINE;
	
	/* Data mode can be quad for faster writes */
	if (dev_data->dev_cfg.io_mode == MSPI_IO_MODE_QUAD) {
		s_command.DataMode = QSPI_DATA_4_LINES;
	} else if (dev_data->dev_cfg.io_mode == MSPI_IO_MODE_DUAL) {
		s_command.DataMode = QSPI_DATA_2_LINES;
	} else {
		s_command.DataMode = QSPI_DATA_1_LINE;
	}

	LOG_DBG("QSPI memmap write cmd: 0x%02x", s_command.Instruction);

	/* Send write command configuration */
	hal_ret = HAL_QSPI_Command(&dev_data->hmspi, &s_command, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to configure QSPI write command for memory mapping: %d", hal_ret);
		return -EIO;
	}

	/* Enable the memory-mapping */
	s_MemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
	s_MemMappedCfg.TimeOutPeriod = 0;

	hal_ret = HAL_QSPI_MemoryMapped(&dev_data->hmspi, &s_command, &s_MemMappedCfg);
	if (hal_ret != HAL_OK) {
		LOG_ERR("Failed to enable QSPI memory mapped mode: %d", hal_ret);
		return -EIO;
	}

	LOG_INF("QSPI memory mapped mode enabled");
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
	struct mspi_stm32_data *dev_data = controller->data;
	int ret = 0;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("xip_config: dev_id don't match");
		return -ESTALE;
	}

	if (!xip_cfg->enable) {
		/* This is for aborting memory mapped mode */
		ret = mspi_stm32_memmap_off(controller);
	} else {
		ret = mspi_stm32_memmap_on(controller);
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
static int mspi_stm32_get_channel_status(const struct device *controller, uint8_t ch)
{
	struct mspi_stm32_data *data = controller->data;
	QSPI_HandleTypeDef *hmspi = &data->hmspi;
	int ret = 0;

	ARG_UNUSED(ch);

	if (mspi_is_inp(controller) || (hmspi->Instance->SR & QUADSPI_SR_BUSY) != 0) {
		ret = -EBUSY;
	}

	data->dev_id = NULL;

	k_mutex_unlock(&data->lock);

	return ret;
}

/*
 * Transfer Error callback.
 */
void HAL_QSPI_ErrorCallback(QSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data = CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Error cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Command completed callback.
 */
void HAL_QSPI_CmdCpltCallback(QSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data = CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Cmd Cplt cb");

	k_sem_give(&dev_data->sync);
}


/*
 * Tx Transfer completed callback.
 */
void HAL_QSPI_TxCpltCallback(QSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data = CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Tx Cplt cb");

	dev_data->ctx.packets_done++;

	k_sem_give(&dev_data->sync);
}

/*
 * Rx Transfer completed callback.
 */
void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data = CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Rx Cplt cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Status Match callback.
 */
void HAL_QSPI_StatusMatchCallback(QSPI_HandleTypeDef *hmspi)
{
	struct mspi_stm32_data *dev_data = CONTAINER_OF(hmspi, struct mspi_stm32_data, hmspi);

	LOG_DBG("Status Match cb");

	k_sem_give(&dev_data->sync);
}

static int mspi_stm32_pio_transceive(const struct device *controller, const struct mspi_xfer *xfer)
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
		ret = mspi_stm32_access(controller, packet, (ctx->xfer.async == true) ? MSPI_ACCESS_ASYNC : MSPI_ACCESS_SYNC);

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
static int mspi_stm32_transceive(const struct device *controller, 
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


	/*
	 * async + MSPI_PIO : Use callback on Irq if PIO
	 * sync + MSPI_PIO use timeout (mainly for NOR command and param
	 * MSPI_DMA : async/sync is meaningless with DMA (no DMA IT function)
	 */

	if (xfer->xfer_mode == MSPI_PIO) {
		return mspi_stm32_pio_transceive(controller, xfer);
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
static void mspi_stm32_isr(const struct device *dev)
{
	struct mspi_stm32_data *dev_data = dev->data;

	HAL_QSPI_IRQHandler(&dev_data->hmspi);
}

/*
 * Driver Initialization
 */
static int mspi_stm32_init(const struct device *controller)
{
	const struct mspi_stm32_conf *cfg = controller->config;

	LOG_DBG("Initializing QSPI driver");

	/* Initialize QSPI with basic configuration */
	const struct mspi_dt_spec spec = {
		.bus = controller,
		.config = cfg->mspicfg,
	};

	return mspi_stm32_config(&spec);
}

static struct mspi_driver_api mspi_stm32_driver_api = {
	.config = mspi_stm32_config,
	.dev_config = mspi_stm32_dev_config,
	.xip_config = mspi_stm32_xip_config,
	.get_channel_status = mspi_stm32_get_channel_status,
	.transceive = mspi_stm32_transceive,
};


/* MSPI QSPI control config */
#define MSPI_QSPI_CONFIG(n)                                                                        \
	{                                                                                          \
		.channel_num = 0, .op_mode = DT_ENUM_IDX_OR(n, op_mode, MSPI_OP_MODE_CONTROLLER), \
		.duplex = DT_ENUM_IDX_OR(n, duplex, MSPI_HALF_DUPLEX),                             \
		.max_freq = DT_INST_PROP_OR(n, mspi_max_frequency, MSPI_MAX_FREQ),            \
		.dqs_support = false, /* QSPI typically doesn't support DQS */                      \
		.num_periph = DT_INST_CHILD_NUM(n),                                                \
		.sw_multi_periph = DT_INST_PROP_OR(n, software_multiperipheral, false),            \
		.num_ce_gpios = ARRAY_SIZE(ce_gpios##n),                                           \
		.ce_group = ce_gpios##n,                                                           \
	}

#define STM32_MSPI_QSPI_IRQ_HANDLER(index)                                                       \
	static void mspi_stm32_irq_config_func_##index(void)                                     \
	{                                                                                         \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), mspi_stm32_isr,   \
			    DEVICE_DT_INST_GET(index), 0);                                       \
		irq_enable(DT_INST_IRQN(index));                                             \
	}

#define MSPI_STM32_QSPI_INIT(index)                                                              \
	static const struct stm32_pclken pclken_##index[] = STM32_DT_INST_CLOCKS(index);         \
	static const struct gpio_dt_spec ce_gpios##index[] = MSPI_CE_GPIOS_DT_SPEC_INST_GET(index); \
	PINCTRL_DT_INST_DEFINE(index);                                                           \
	STM32_MSPI_QSPI_IRQ_HANDLER(index)                                                       \
	static const struct mspi_stm32_conf mspi_stm32_qspi_dev_conf_##index = {                \
		.pclken = pclken_##index,                                                     \
		.pclk_len = DT_INST_NUM_CLOCKS(index),                                       \
		.irq_config = mspi_stm32_irq_config_func_##index,                            \
		.mspicfg = MSPI_QSPI_CONFIG(index),                                          \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(index)),                      \
	};                                                                                    \
	static struct mspi_stm32_data mspi_stm32_qspi_dev_data_##index = {                      \
		.hmspi = {                                                                    \
			.Instance = (QUADSPI_TypeDef *)DT_INST_REG_ADDR(index),              \
			.Init = {                                                             \
				.ClockPrescaler = 0,                                          \
				.FifoThreshold = MSPI_STM32_FIFO_THRESHOLD,                   \
				.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE,                  \
				.FlashSize = 0x19,											\
				.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE,              \
				.ClockMode = QSPI_CLOCK_MODE_0,                               \
				.FlashID = QSPI_FLASH_ID_1,                                   \
				.DualFlash = QSPI_DUALFLASH_DISABLE,                          \
			},                                                                    \
		},                                                                            \
		.dev_id = index,                                                              \
		.lock = Z_MUTEX_INITIALIZER(mspi_stm32_qspi_dev_data_##index.lock),          \
		.sync = Z_SEM_INITIALIZER(mspi_stm32_qspi_dev_data_##index.sync, 0, 1),      \
		.dev_cfg = {0},                                                               \
		.xip_cfg = {0},                                                               \
		.ctx.lock = Z_SEM_INITIALIZER(mspi_stm32_qspi_dev_data_##index.ctx.lock, 0, 1), \
	};                                                                                    \
	DEVICE_DT_INST_DEFINE(index, &mspi_stm32_init, NULL,                                \
			       &mspi_stm32_qspi_dev_data_##index,                           \
			       &mspi_stm32_qspi_dev_conf_##index, POST_KERNEL,              \
			       CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mspi_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_STM32_QSPI_INIT)

