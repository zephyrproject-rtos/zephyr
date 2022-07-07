/*
 * Copyright (c) 2022 Macronix.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_ospi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ospi_ll_stm32);

#include <sys/util.h>
#include <kernel.h>
#include <soc.h>
#include <stm32_ll_spi.h>
#include <errno.h>
#include <drivers/spi.h>
#include <toolchain.h>

#include <zephyr/drivers/pinctrl.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/clock_control.h>

#include "ospi_ll_stm32.h"

#define DEV_CFG(dev)						\
(const struct ospi_stm32_config * const)(dev->config)

#define DEV_DATA(dev)					\
(struct ospi_stm32_data * const)(dev->data)

#define STM32_OSPI_FIFO_THRESHOLD         8
#define STM32_OSPI_CLOCK_PRESCALER_MAX  255


/*
 * Prepare a command over OSPI bus.
 */

static int ospi_prepare_command(const struct device *dev, 
				const struct spi_buf_set *bufs, 
				OSPI_RegularCmdTypeDef *st_command)
{
	uint8_t *buffer =  bufs->buffers[0].buf;
	uint8_t data_mode = *(buffer + 5) & 0x0F;
	uint8_t data_rate = *(buffer + 5) >> 4;
	uint32_t operation;

	if (data_mode == OSPI_OPI_MODE) {
		if (data_rate == OSPI_DTR_TRANSFER) {
			operation = SPI_LINES_OCTAL | SPI_DTR_ENABLE;
		} else {
			operation = SPI_LINES_OCTAL;
		}
	} else {
		operation = SPI_LINES_SINGLE;
	}

	st_command->FlashId = HAL_OSPI_FLASH_ID_1;

	st_command->Instruction = ((operation & SPI_LINES_MASK) == SPI_LINES_OCTAL)
                                  ? *buffer << 8 | (0xFF - *buffer) : *buffer;
	switch (operation & SPI_LINES_MASK) {
            case SPI_LINES_SINGLE:
                st_command->InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
                break;
            case SPI_LINES_DUAL:
                st_command->InstructionMode = HAL_OSPI_INSTRUCTION_2_LINES;
                break;
            case SPI_LINES_QUAD:
                st_command->InstructionMode = HAL_OSPI_INSTRUCTION_4_LINES;
                break;
            case SPI_LINES_OCTAL:
                st_command->InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
                break;
            default:
                LOG_ERR("Command param error: wrong instruction format\n");
                return -EIO;
        }

	st_command->InstructionSize    = (st_command->InstructionMode == HAL_OSPI_INSTRUCTION_8_LINES) ? HAL_OSPI_INSTRUCTION_16_BITS : HAL_OSPI_INSTRUCTION_8_BITS;
	st_command->InstructionDtrMode = (operation & SPI_DTR_ENABLE) ? HAL_OSPI_INSTRUCTION_DTR_ENABLE : HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	st_command->DummyCycles = *(buffer + 6) ;
	// these are target specific settings, use default values
	st_command->SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;
	st_command->DataDtrMode = (operation & SPI_DTR_ENABLE) ? HAL_OSPI_DATA_DTR_ENABLE : HAL_OSPI_DATA_DTR_DISABLE;
	st_command->AddressDtrMode = (operation & SPI_DTR_ENABLE) ? HAL_OSPI_ADDRESS_DTR_ENABLE : HAL_OSPI_ADDRESS_DTR_DISABLE;
	st_command->AlternateBytesDtrMode = (operation & SPI_DTR_ENABLE) ? HAL_OSPI_ALTERNATE_BYTES_DTR_ENABLE : HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;
	st_command->DQSMode = (operation & SPI_DTR_ENABLE) ? HAL_OSPI_DQS_ENABLE : HAL_OSPI_DQS_DISABLE;

	st_command->OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
	if (bufs->buffers[0].len == 1) {
		st_command->AddressMode = HAL_OSPI_ADDRESS_NONE;
		st_command->AddressSize = 0;
		st_command->Address = 0;
	} else {
		if (bufs->buffers[0].len == 5) {
			/* 4 byte address */
			st_command->Address = (*(buffer + 1) << 24) | (*(buffer + 2) << 16) | (*(buffer + 3) << 8) | (*(buffer + 4));
		} else {
			/* 3 byte address */
			st_command->Address = (*(buffer + 1) << 16) | (*(buffer + 2) << 8) | (*(buffer + 3));
		}
		switch (operation & SPI_LINES_MASK) {
			case SPI_LINES_SINGLE:
				st_command->AddressMode = HAL_OSPI_ADDRESS_1_LINE;
				break;
			case SPI_LINES_DUAL:
				st_command->AddressMode = HAL_OSPI_ADDRESS_2_LINES;
				break;
			case SPI_LINES_QUAD:
				st_command->AddressMode = HAL_OSPI_ADDRESS_4_LINES;
				break;
			case SPI_LINES_OCTAL:
				st_command->AddressMode = HAL_OSPI_ADDRESS_8_LINES;
				break;
			default:
				LOG_ERR("Command param error: wrong address size\n");
				return -EIO;
		}
		switch (bufs->buffers[0].len - 1) {
			case 1:
				st_command->AddressSize = HAL_OSPI_ADDRESS_8_BITS;
				break;
			case 2:
				st_command->AddressSize = HAL_OSPI_ADDRESS_16_BITS;
				break;
			case 3:
				st_command->AddressSize = HAL_OSPI_ADDRESS_24_BITS;
				break;
			case 4:
				st_command->AddressSize = HAL_OSPI_ADDRESS_32_BITS;
				break;
			default:
				LOG_ERR("Command param error: wrong address size\n");
				return -EIO;
		}
	}   

	switch (operation & SPI_LINES_MASK) {
		case SPI_LINES_SINGLE:
			st_command->DataMode = HAL_OSPI_DATA_1_LINE;
			break;
		case SPI_LINES_DUAL:
			st_command->DataMode = HAL_OSPI_DATA_2_LINES;
			break;
		case SPI_LINES_QUAD:
			st_command->DataMode = HAL_OSPI_DATA_4_LINES;
			break;
		case SPI_LINES_OCTAL:
			st_command->DataMode = HAL_OSPI_DATA_8_LINES;
			break;
		default:
			st_command->DataMode = HAL_OSPI_DATA_NONE;
			return -EIO;
	}
 
	//st_command->AlternateBytesSize = 0;

	return 0;
}

/*
 * Send a command over OSPI bus.
 */
static int ospi_send_cmd(const struct device *dev, OSPI_RegularCmdTypeDef *cmd)
{
	const struct ospi_stm32_config *dev_cfg = DEV_CFG(dev);
	struct ospi_stm32_data *dev_data = DEV_DATA(dev);
	HAL_StatusTypeDef hal_ret;

	ARG_UNUSED(dev_cfg);

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	cmd->DataMode = HAL_OSPI_DATA_NONE; /* Instruction only */

	hal_ret = HAL_OSPI_Command(&dev_data->hospi, cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send OSPI instruction", hal_ret);
		return -EIO;
	}
	LOG_DBG("CCR 0x%x", dev_cfg->regs->CCR);

	return hal_ret;
}

/*
 * Perform a read access over OSPI bus.
 */
static int ospi_read_access(const struct device *dev, OSPI_RegularCmdTypeDef *cmd,
			    uint8_t *data, size_t size)
{
	const struct ospi_stm32_config *dev_cfg = DEV_CFG(dev);
	struct ospi_stm32_data *dev_data = DEV_DATA(dev);
	HAL_StatusTypeDef hal_ret;

	ARG_UNUSED(dev_cfg);

	cmd->NbData = size;

	hal_ret = HAL_OSPI_Command(&dev_data->hospi, cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send OSPI instruction", hal_ret);
		return -EIO;
	}

	hal_ret = HAL_OSPI_Receive(&dev_data->hospi, data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}

	return hal_ret;
}

/*
 * Perform a write access over OSPI bus.
 */
static int ospi_write_access(const struct device *dev, OSPI_RegularCmdTypeDef *cmd,
			     const uint8_t *data, size_t size)
{
	const struct ospi_stm32_config *dev_cfg = DEV_CFG(dev);
	struct ospi_stm32_data *dev_data = DEV_DATA(dev);
	HAL_StatusTypeDef hal_ret;

	ARG_UNUSED(dev_cfg);

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	cmd->NbData = size;

	hal_ret = HAL_OSPI_Command(&dev_data->hospi, cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send OSPI instruction", hal_ret);
		return -EIO;
	}

	hal_ret = HAL_OSPI_Transmit(&dev_data->hospi, (uint8_t *)data, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}
	LOG_DBG("CCR 0x%x", dev_cfg->regs->CCR);

	return hal_ret;
}


static int transceive(const struct device *dev,
		      const struct spi_config *config,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs,
		      bool asynchronous, struct k_poll_signal *signal)
{
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

	OSPI_RegularCmdTypeDef cmd;
	memset(&cmd, 0 ,sizeof(OSPI_RegularCmdTypeDef));

	/* read command */
	if (!tx_bufs) {

		ret = ospi_prepare_command(dev, rx_bufs, &cmd);

		ret = ospi_read_access(dev, &cmd, rx_bufs->buffers[1].buf, rx_bufs->buffers[1].len);

		if (ret != 0) {
			return -EIO;
		} else {
			return ret;
		}
	} else {
		ret = ospi_prepare_command(dev, tx_bufs, &cmd);

		if (tx_bufs->buffers[1].len == 0) { /* no address and no send data */
			ret = ospi_send_cmd(dev, &cmd);
		} else {
			ret = ospi_write_access(dev, &cmd, tx_bufs->buffers[1].buf, tx_bufs->buffers[1].len);
		}
	}

	return ret;
}

static int ospi_stm32_transceive(const struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, config, tx_bufs, rx_bufs, false, NULL);
}




static int ospi_stm32_release(const struct device *dev,
			     const struct spi_config *config)
{
	struct ospi_stm32_data *data = DEV_DATA(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api ospi_stm32_driver_api = {
	.transceive = ospi_stm32_transceive,
#ifdef CONFIG_SPI_ASYNC
	//.transceive_async = ospi_stm32_transceive_async,
#endif
	.release = ospi_stm32_release,
};

static int ospi_stm32_init(const struct device *dev)
{
	struct ospi_stm32_data *data __attribute__((unused)) = dev->data;
	const struct ospi_stm32_config *dev_cfg = dev->config;
	uint32_t ahb_clock_freq;
	int err;

	err = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("SPI pinctrl setup failed (%d)", err);
		return err;
	}

	/* Clock configuration */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			       (clock_control_subsys_t) &dev_cfg->pclken) != 0) {
		LOG_ERR("Could not enable OSPI clock");
		return -EIO;
	}

	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			(clock_control_subsys_t) &dev_cfg->pclken,
			&ahb_clock_freq) < 0) {
		LOG_DBG("Failed to get AHB clock frequency");
		return -EIO;
	}

	__ASSERT_NO_MSG(prescaler <= STM32_OSPI_CLOCK_PRESCALER_MAX);

	/* Initialize OSPI HAL */
	data->hospi.State = HAL_OSPI_STATE_RESET;

	data->hospi.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
//#if defined(TARGET_MX25LM512451G)
//	obj->handle.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX; // Read sequence in DTR mode: D1-D0-D3-D2
//#else
	data->hospi.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;   // Read sequence in DTR mode: D0-D1-D2-D3
	data->hospi.Init.ClockPrescaler = 4; // default value, will be overwritten in ospi_frequency
	data->hospi.Init.FifoThreshold = 4;
	data->hospi.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
	data->hospi.Init.DeviceSize = 32;
	data->hospi.Init.ChipSelectHighTime = 3;
	data->hospi.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
	data->hospi.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
	data->hospi.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
	data->hospi.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;
	data->hospi.Init.ChipSelectBoundary = 0;
	data->hospi.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_USED;

	data->hospi.Init.Refresh = 0;

	HAL_OSPI_Init(&data->hospi);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

PINCTRL_DT_INST_DEFINE(0);


static const struct ospi_stm32_config ospi_stm32_cfg = {
	.regs = (OCTOSPI_TypeDef *)DT_INST_REG_ADDR(0),
	.pclken = {
		.enr = DT_INST_CLOCKS_CELL(0, bits),
		.bus = DT_INST_CLOCKS_CELL(0, bus)
	},
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct ospi_stm32_data ospi_stm32_dev_data = {
	.hospi = {
		.Instance = (OCTOSPI_TypeDef *)DT_INST_REG_ADDR(0),
		.Init = {
			.FifoThreshold = STM32_OSPI_FIFO_THRESHOLD,
			.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE,
			.ChipSelectHighTime = 0,
			.ClockMode = HAL_OSPI_CLOCK_MODE_0,
			},
	},
	SPI_CONTEXT_INIT_LOCK(ospi_stm32_dev_data, ctx),
	SPI_CONTEXT_INIT_SYNC(ospi_stm32_dev_data, ctx)
};

DEVICE_DT_INST_DEFINE(0, &ospi_stm32_init, NULL,
		      &ospi_stm32_dev_data, &ospi_stm32_cfg,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &ospi_stm32_driver_api);
