/*
 * Copyright (c) 2020 Piotr Mienkowski
 * Copyright (c) 2020 Linaro Limited
 * Copyright (c) 2022 Georgij Cernysiov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_qspi_nor

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <string.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/gpio.h>

#if DT_INST_NODE_HAS_PROP(0, spi_bus_width) && \
	DT_INST_PROP(0, spi_bus_width) == 4
#define STM32_QSPI_USE_QUAD_IO 1
#else
#define STM32_QSPI_USE_QUAD_IO 0
#endif

/* Get the base address of the flash from the DTS node */
#define STM32_QSPI_BASE_ADDRESS DT_INST_REG_ADDR(0)

#define STM32_QSPI_RESET_GPIO DT_INST_NODE_HAS_PROP(0, reset_gpios)
#define STM32_QSPI_RESET_CMD  DT_INST_PROP(0, reset_cmd)

#include <stm32_ll_dma.h>

#include "spi_nor.h"
#include "jesd216.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(flash_stm32_qspi, CONFIG_FLASH_LOG_LEVEL);

#define STM32_QSPI_FIFO_THRESHOLD         8
#define STM32_QSPI_CLOCK_PRESCALER_MAX  255

#define STM32_QSPI_UNKNOWN_MODE (0xFF)

#define STM32_QSPI_USE_DMA DT_NODE_HAS_PROP(DT_INST_PARENT(0), dmas)

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_qspi_nor)

/* In dual-flash mode, total size is twice the size of one flash component */
#define STM32_QSPI_DOUBLE_FLASH	DT_PROP(DT_NODELABEL(quadspi), dual_flash)

#if STM32_QSPI_USE_DMA
static const uint32_t table_m_size[] = {
	LL_DMA_MDATAALIGN_BYTE,
	LL_DMA_MDATAALIGN_HALFWORD,
	LL_DMA_MDATAALIGN_WORD,
};

static const uint32_t table_p_size[] = {
	LL_DMA_PDATAALIGN_BYTE,
	LL_DMA_PDATAALIGN_HALFWORD,
	LL_DMA_PDATAALIGN_WORD,
};

/* Lookup table to set dma priority from the DTS */
static const uint32_t table_priority[] = {
	DMA_PRIORITY_LOW,
	DMA_PRIORITY_MEDIUM,
	DMA_PRIORITY_HIGH,
	DMA_PRIORITY_VERY_HIGH,
};
#endif /* STM32_QSPI_USE_DMA */

typedef void (*irq_config_func_t)(const struct device *dev);

struct stream {
	DMA_TypeDef *reg;
	const struct device *dev;
	uint32_t channel;
	struct dma_config cfg;
};

struct flash_stm32_qspi_config {
	QUADSPI_TypeDef *regs;
	struct stm32_pclken pclken;
	irq_config_func_t irq_config;
	size_t flash_size;
	uint32_t max_frequency;
	const struct pinctrl_dev_config *pcfg;
#if STM32_QSPI_RESET_GPIO
	const struct gpio_dt_spec reset;
#endif
#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_qspi_nor), jedec_id)
	uint8_t jedec_id[DT_INST_PROP_LEN(0, jedec_id)];
#endif /* jedec_id */
};

struct flash_stm32_qspi_data {
	QSPI_HandleTypeDef hqspi;
	struct k_sem sem;
	struct k_sem sync;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];
	/* Number of bytes per page */
	uint16_t page_size;
	enum jesd216_dw15_qer_type qer_type;
	enum jesd216_mode_type mode;
	int cmd_status;
	struct stream dma;
	uint8_t qspi_write_cmd;
	uint8_t qspi_read_cmd;
	uint8_t qspi_read_cmd_latency;
	/*
	 * If set addressed operations should use 32-bit rather than
	 * 24-bit addresses.
	 */
	bool flag_access_32bit: 1;
};

static const QSPI_CommandTypeDef cmd_write_en = {
	.Instruction = SPI_NOR_CMD_WREN,
	.InstructionMode = QSPI_INSTRUCTION_1_LINE
};

static inline void qspi_lock_thread(const struct device *dev)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);
}

static inline void qspi_unlock_thread(const struct device *dev)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
}

static inline void qspi_set_address_size(const struct device *dev,
					 QSPI_CommandTypeDef *cmd)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;

	if (dev_data->flag_access_32bit) {
		cmd->AddressSize = QSPI_ADDRESS_32_BITS;
		return;
	}

	cmd->AddressSize = QSPI_ADDRESS_24_BITS;
}

static inline int qspi_prepare_quad_read(const struct device *dev,
					  QSPI_CommandTypeDef *cmd)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;

	__ASSERT_NO_MSG(dev_data->mode == JESD216_MODE_114 ||
			dev_data->mode == JESD216_MODE_144);

	cmd->Instruction = dev_data->qspi_read_cmd;
	cmd->AddressMode = ((dev_data->mode == JESD216_MODE_114)
				? QSPI_ADDRESS_1_LINE
				: QSPI_ADDRESS_4_LINES);
	cmd->DataMode = QSPI_DATA_4_LINES;
	cmd->DummyCycles = dev_data->qspi_read_cmd_latency;

	return 0;
}

static inline int qspi_prepare_quad_program(const struct device *dev,
					     QSPI_CommandTypeDef *cmd)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;

	__ASSERT_NO_MSG(dev_data->qspi_write_cmd == SPI_NOR_CMD_PP_1_1_4 ||
			dev_data->qspi_write_cmd == SPI_NOR_CMD_PP_1_4_4);

	cmd->Instruction = dev_data->qspi_write_cmd;
#if defined(CONFIG_USE_MICROCHIP_QSPI_FLASH_WITH_STM32)
	/* Microchip qspi-NOR flash, does not follow the standard rules */
	if (cmd->Instruction == SPI_NOR_CMD_PP_1_1_4) {
		cmd->AddressMode = QSPI_ADDRESS_4_LINES;
	}
#else
	cmd->AddressMode = ((cmd->Instruction == SPI_NOR_CMD_PP_1_1_4)
				? QSPI_ADDRESS_1_LINE
				: QSPI_ADDRESS_4_LINES);
#endif /* CONFIG_USE_MICROCHIP_QSPI_FLASH_WITH_STM32 */
	cmd->DataMode = QSPI_DATA_4_LINES;
	cmd->DummyCycles = 0;

	return 0;
}

/*
 * Send a command over QSPI bus.
 */
static int qspi_send_cmd(const struct device *dev, const QSPI_CommandTypeDef *cmd)
{
	const struct flash_stm32_qspi_config *dev_cfg = dev->config;
	struct flash_stm32_qspi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	ARG_UNUSED(dev_cfg);

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	dev_data->cmd_status = 0;

	hal_ret = HAL_QSPI_Command_IT(&dev_data->hqspi, (QSPI_CommandTypeDef *)cmd);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send QSPI instruction", hal_ret);
		return -EIO;
	}
	LOG_DBG("CCR 0x%x", dev_cfg->regs->CCR);

	k_sem_take(&dev_data->sync, K_FOREVER);

	return dev_data->cmd_status;
}

/*
 * Perform a read access over QSPI bus.
 */
static int qspi_read_access(const struct device *dev, QSPI_CommandTypeDef *cmd,
			    uint8_t *data, size_t size)
{
	const struct flash_stm32_qspi_config *dev_cfg = dev->config;
	struct flash_stm32_qspi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	ARG_UNUSED(dev_cfg);

	cmd->NbData = size;

	dev_data->cmd_status = 0;

	hal_ret = HAL_QSPI_Command_IT(&dev_data->hqspi, cmd);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send QSPI instruction", hal_ret);
		return -EIO;
	}

#if STM32_QSPI_USE_DMA
	hal_ret = HAL_QSPI_Receive_DMA(&dev_data->hqspi, data);
#else
	hal_ret = HAL_QSPI_Receive_IT(&dev_data->hqspi, data);
#endif
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}

	k_sem_take(&dev_data->sync, K_FOREVER);

	return dev_data->cmd_status;
}

/*
 * Perform a write access over QSPI bus.
 */
static int qspi_write_access(const struct device *dev, QSPI_CommandTypeDef *cmd,
			     const uint8_t *data, size_t size)
{
	const struct flash_stm32_qspi_config *dev_cfg = dev->config;
	struct flash_stm32_qspi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	ARG_UNUSED(dev_cfg);

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	cmd->NbData = size;

	dev_data->cmd_status = 0;

	hal_ret = HAL_QSPI_Command_IT(&dev_data->hqspi, cmd);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send QSPI instruction", hal_ret);
		return -EIO;
	}

#if STM32_QSPI_USE_DMA
	hal_ret = HAL_QSPI_Transmit_DMA(&dev_data->hqspi, (uint8_t *)data);
#else
	hal_ret = HAL_QSPI_Transmit_IT(&dev_data->hqspi, (uint8_t *)data);
#endif
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}
	LOG_DBG("CCR 0x%x", dev_cfg->regs->CCR);

	k_sem_take(&dev_data->sync, K_FOREVER);

	return dev_data->cmd_status;
}

#if defined(CONFIG_FLASH_JESD216_API)
/*
 * Read Serial Flash ID :
 * perform a read access over SPI bus for read Identification (DataMode is already set)
 * and compare to the jedec-id from the DTYS table exists
 */
static int qspi_read_jedec_id(const struct device *dev, uint8_t *id)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;
	uint8_t data[JESD216_READ_ID_LEN];

	QSPI_CommandTypeDef cmd = {
		.Instruction = JESD216_CMD_READ_ID,
		.AddressSize = QSPI_ADDRESS_NONE,
		.DummyCycles = 8,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
		.NbData = JESD216_READ_ID_LEN,
	};

	HAL_StatusTypeDef hal_ret;

	hal_ret = HAL_QSPI_Command_IT(&dev_data->hqspi, &cmd);

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send OSPI instruction", hal_ret);
		return -EIO;
	}

	hal_ret = HAL_QSPI_Receive(&dev_data->hqspi, data, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}

	LOG_DBG("Read JESD216-ID");

	dev_data->cmd_status = 0;
	memcpy(id, data, JESD216_READ_ID_LEN);

	return 0;
}
#endif /* CONFIG_FLASH_JESD216_API */

static int qspi_write_unprotect(const struct device *dev)
{
	int ret = 0;
	QSPI_CommandTypeDef cmd_unprotect = {
			.Instruction = SPI_NOR_CMD_ULBPR,
			.InstructionMode = QSPI_INSTRUCTION_1_LINE,
	};

	if (IS_ENABLED(DT_INST_PROP(0, requires_ulbpr))) {
		ret = qspi_send_cmd(dev, &cmd_write_en);

		if (ret != 0) {
			return ret;
		}

		ret = qspi_send_cmd(dev, &cmd_unprotect);
	}

	return ret;
}

/*
 * Read Serial Flash Discovery Parameter
 */
static int qspi_read_sfdp(const struct device *dev, off_t addr, void *data,
			  size_t size)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	__ASSERT(data != NULL, "null destination");

	LOG_INF("Reading SFDP");

	QSPI_CommandTypeDef cmd = {
		.Instruction = JESD216_CMD_READ_SFDP,
		.Address = addr,
		.AddressSize = QSPI_ADDRESS_24_BITS,
		.DummyCycles = 8,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
		.NbData = size,
	};

	hal_ret = HAL_QSPI_Command(&dev_data->hqspi, &cmd,
				   HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send SFDP instruction", hal_ret);
		return -EIO;
	}

	hal_ret = HAL_QSPI_Receive(&dev_data->hqspi, (uint8_t *)data,
				   HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read SFDP", hal_ret);
		return -EIO;
	}

	dev_data->cmd_status = 0;

	return 0;
}

static bool qspi_address_is_valid(const struct device *dev, off_t addr,
				  size_t size)
{
	const struct flash_stm32_qspi_config *dev_cfg = dev->config;
	size_t flash_size = dev_cfg->flash_size;

	return (addr >= 0) && ((uint64_t)addr + (uint64_t)size <= flash_size);
}

#ifdef CONFIG_STM32_MEMMAP
/* Must be called inside qspi_lock_thread(). */
static int stm32_qspi_set_memory_mapped(const struct device *dev)
{
	int ret;
	HAL_StatusTypeDef hal_ret;
	struct flash_stm32_qspi_data *dev_data = dev->data;

	QSPI_CommandTypeDef cmd = {
		.Instruction = SPI_NOR_CMD_READ,
		.Address = 0,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	qspi_set_address_size(dev, &cmd);
	if (IS_ENABLED(STM32_QSPI_USE_QUAD_IO)) {
		ret = qspi_prepare_quad_read(dev, &cmd);
		if (ret < 0) {
			return ret;
		}
	}

	QSPI_MemoryMappedTypeDef mem_mapped = {
		.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE,
	};

	hal_ret = HAL_QSPI_MemoryMapped(&dev_data->hqspi, &cmd, &mem_mapped);
	if (hal_ret != 0) {
		LOG_ERR("%d: Failed to enable memory mapped", hal_ret);
		return -EIO;
	}

	LOG_DBG("MemoryMap mode enabled");
	return 0;
}

static bool stm32_qspi_is_memory_mapped(const struct device *dev)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;

	return READ_BIT(dev_data->hqspi.Instance->CCR, QUADSPI_CCR_FMODE) == QUADSPI_CCR_FMODE;
}

static int stm32_qspi_abort(const struct device *dev)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	hal_ret = HAL_QSPI_Abort(&dev_data->hqspi);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: QSPI abort failed", hal_ret);
		return -EIO;
	}

	return 0;
}
#endif

static int flash_stm32_qspi_read(const struct device *dev, off_t addr,
				 void *data, size_t size)
{
	int ret;

	if (!qspi_address_is_valid(dev, addr, size)) {
		LOG_DBG("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	/* read non-zero size */
	if (size == 0) {
		return 0;
	}

#ifdef CONFIG_STM32_MEMMAP
	qspi_lock_thread(dev);

	/* Do reads through memory-mapping instead of indirect */
	if (!stm32_qspi_is_memory_mapped(dev)) {
		ret = stm32_qspi_set_memory_mapped(dev);
		if (ret != 0) {
			LOG_ERR("READ: failed to set memory mapped");
			goto end;
		}
	}

	__ASSERT_NO_MSG(stm32_qspi_is_memory_mapped(dev));

	uintptr_t mmap_addr = STM32_QSPI_BASE_ADDRESS + addr;

	LOG_DBG("Memory-mapped read from 0x%08lx, len %zu", mmap_addr, size);
	memcpy(data, (void *)mmap_addr, size);
	ret = 0;
	goto end;
#else
	QSPI_CommandTypeDef cmd = {
		.Instruction = SPI_NOR_CMD_READ,
		.Address = addr,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	qspi_set_address_size(dev, &cmd);
	if (IS_ENABLED(STM32_QSPI_USE_QUAD_IO)) {
		ret = qspi_prepare_quad_read(dev, &cmd);
		if (ret < 0) {
			return ret;
		}
	}

	qspi_lock_thread(dev);

	ret = qspi_read_access(dev, &cmd, data, size);
	goto end;
#endif

end:
	qspi_unlock_thread(dev);

	return ret;
}

static int qspi_wait_until_ready(const struct device *dev)
{
	uint8_t reg;
	int ret;

	QSPI_CommandTypeDef cmd = {
		.Instruction = SPI_NOR_CMD_RDSR,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	do {
		ret = qspi_read_access(dev, &cmd, &reg, sizeof(reg));
	} while (!ret && (reg & SPI_NOR_WIP_BIT));

	return ret;
}

static int flash_stm32_qspi_write(const struct device *dev, off_t addr,
				  const void *data, size_t size)
{
	int ret = 0;

	if (!qspi_address_is_valid(dev, addr, size)) {
		LOG_DBG("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	/* write non-zero size */
	if (size == 0) {
		return 0;
	}

	QSPI_CommandTypeDef cmd_pp = {
		.Instruction = SPI_NOR_CMD_PP,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	qspi_set_address_size(dev, &cmd_pp);
	if (IS_ENABLED(STM32_QSPI_USE_QUAD_IO)) {
		ret = qspi_prepare_quad_program(dev, &cmd_pp);
		if (ret < 0) {
			return ret;
		}
	}

	qspi_lock_thread(dev);

#ifdef CONFIG_STM32_MEMMAP
	if (stm32_qspi_is_memory_mapped(dev)) {
		/* Abort ongoing transfer to force CS high/BUSY deasserted */
		ret = stm32_qspi_abort(dev);
		if (ret != 0) {
			LOG_ERR("Failed to abort memory-mapped access before write");
			goto end;
		}
	}
#endif

	while (size > 0) {
		size_t to_write = size;

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

		ret = qspi_send_cmd(dev, &cmd_write_en);
		if (ret != 0) {
			break;
		}

		cmd_pp.Address = addr;
		ret = qspi_write_access(dev, &cmd_pp, data, to_write);
		if (ret != 0) {
			break;
		}

		size -= to_write;
		data = (const uint8_t *)data + to_write;
		addr += to_write;

		ret = qspi_wait_until_ready(dev);
		if (ret != 0) {
			break;
		}
	}
	goto end;

end:
	qspi_unlock_thread(dev);

	return ret;
}

static int flash_stm32_qspi_erase(const struct device *dev, off_t addr,
				  size_t size)
{
	const struct flash_stm32_qspi_config *dev_cfg = dev->config;
	struct flash_stm32_qspi_data *dev_data = dev->data;
	int ret = 0;

	if (!qspi_address_is_valid(dev, addr, size)) {
		LOG_DBG("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	/* erase non-zero size */
	if (size == 0) {
		return 0;
	}

	QSPI_CommandTypeDef cmd_erase = {
		.Instruction = 0,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
	};

	qspi_set_address_size(dev, &cmd_erase);
	qspi_lock_thread(dev);

#ifdef CONFIG_STM32_MEMMAP
	if (stm32_qspi_is_memory_mapped(dev)) {
		/* Abort ongoing transfer to force CS high/BUSY deasserted */
		ret = stm32_qspi_abort(dev);
		if (ret != 0) {
			LOG_ERR("Failed to abort memory-mapped access before erase");
			goto end;
		}
	}
#endif

	while ((size > 0) && (ret == 0)) {
		cmd_erase.Address = addr;
		qspi_send_cmd(dev, &cmd_write_en);

		if (size == dev_cfg->flash_size) {
			/* chip erase */
			cmd_erase.Instruction = SPI_NOR_CMD_CE;
			cmd_erase.AddressMode = QSPI_ADDRESS_NONE;
			qspi_send_cmd(dev, &cmd_erase);
			size -= dev_cfg->flash_size;
		} else {
			const struct jesd216_erase_type *erase_types =
							dev_data->erase_types;
			const struct jesd216_erase_type *bet = NULL;

			for (uint8_t ei = 0;
				ei < JESD216_NUM_ERASE_TYPES; ++ei) {
				const struct jesd216_erase_type *etp =
							&erase_types[ei];

				if ((etp->exp != 0)
				    && SPI_NOR_IS_ALIGNED(addr, etp->exp)
				    && SPI_NOR_IS_ALIGNED(size, etp->exp)
				    && ((bet == NULL)
					|| (etp->exp > bet->exp))) {
					bet = etp;
					cmd_erase.Instruction = bet->cmd;
				}
			}
			if (bet != NULL) {
				qspi_send_cmd(dev, &cmd_erase);
				addr += BIT(bet->exp);
				size -= BIT(bet->exp);
			} else {
				LOG_ERR("Can't erase %zu at 0x%lx",
					size, (long)addr);
				ret = -EINVAL;
			}
		}
		qspi_wait_until_ready(dev);
	}
	goto end;

end:
	qspi_unlock_thread(dev);

	return ret;
}

static const struct flash_parameters flash_stm32_qspi_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff
};

static const struct flash_parameters *
flash_stm32_qspi_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_stm32_qspi_parameters;
}

static int flash_stm32_qspi_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_stm32_qspi_config *dev_cfg = dev->config;

	*size = (uint64_t)dev_cfg->flash_size;

	return 0;
}

static void flash_stm32_qspi_isr(const struct device *dev)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;

	HAL_QSPI_IRQHandler(&dev_data->hqspi);
}

/* This function is executed in the interrupt context */
#if STM32_QSPI_USE_DMA
static void qspi_dma_callback(const struct device *dev, void *arg,
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

__weak HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef *hdma)
{
	return HAL_OK;
}

__weak HAL_StatusTypeDef HAL_DMA_Abort_IT(DMA_HandleTypeDef *hdma)
{
	return HAL_OK;
}

/*
 * Transfer Error callback.
 */
void HAL_QSPI_ErrorCallback(QSPI_HandleTypeDef *hqspi)
{
	struct flash_stm32_qspi_data *dev_data =
		CONTAINER_OF(hqspi, struct flash_stm32_qspi_data, hqspi);

	LOG_DBG("Enter");

	dev_data->cmd_status = -EIO;

	k_sem_give(&dev_data->sync);
}

/*
 * Command completed callback.
 */
void HAL_QSPI_CmdCpltCallback(QSPI_HandleTypeDef *hqspi)
{
	struct flash_stm32_qspi_data *dev_data =
		CONTAINER_OF(hqspi, struct flash_stm32_qspi_data, hqspi);

	k_sem_give(&dev_data->sync);
}

/*
 * Rx Transfer completed callback.
 */
void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
	struct flash_stm32_qspi_data *dev_data =
		CONTAINER_OF(hqspi, struct flash_stm32_qspi_data, hqspi);

	k_sem_give(&dev_data->sync);
}

/*
 * Tx Transfer completed callback.
 */
void HAL_QSPI_TxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
	struct flash_stm32_qspi_data *dev_data =
		CONTAINER_OF(hqspi, struct flash_stm32_qspi_data, hqspi);

	k_sem_give(&dev_data->sync);
}

/*
 * Status Match callback.
 */
void HAL_QSPI_StatusMatchCallback(QSPI_HandleTypeDef *hqspi)
{
	struct flash_stm32_qspi_data *dev_data =
		CONTAINER_OF(hqspi, struct flash_stm32_qspi_data, hqspi);

	k_sem_give(&dev_data->sync);
}

/*
 * Timeout callback.
 */
void HAL_QSPI_TimeOutCallback(QSPI_HandleTypeDef *hqspi)
{
	struct flash_stm32_qspi_data *dev_data =
		CONTAINER_OF(hqspi, struct flash_stm32_qspi_data, hqspi);

	LOG_DBG("Enter");

	dev_data->cmd_status = -EIO;

	k_sem_give(&dev_data->sync);
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_stm32_qspi_pages_layout(const struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;

	*layout = &dev_data->layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, flash_stm32_qspi_driver_api) = {
	.read = flash_stm32_qspi_read,
	.write = flash_stm32_qspi_write,
	.erase = flash_stm32_qspi_erase,
	.get_parameters = flash_stm32_qspi_get_parameters,
	.get_size = flash_stm32_qspi_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_stm32_qspi_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = qspi_read_sfdp,
	.read_jedec_id = qspi_read_jedec_id,
#endif /* CONFIG_FLASH_JESD216_API */
};

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static int setup_pages_layout(const struct device *dev)
{
	const struct flash_stm32_qspi_config *dev_cfg = dev->config;
	struct flash_stm32_qspi_data *data = dev->data;
	const size_t flash_size = dev_cfg->flash_size;
	uint32_t layout_page_size = data->page_size;
	uint8_t exp = 0;
	int rv = 0;

	/* Find the smallest erase size. */
	for (size_t i = 0; i < ARRAY_SIZE(data->erase_types); ++i) {
		const struct jesd216_erase_type *etp = &data->erase_types[i];

		if ((etp->cmd != 0)
		    && ((exp == 0) || (etp->exp < exp))) {
			exp = etp->exp;
		}
	}

	if (exp == 0) {
		return -ENOTSUP;
	}

	uint32_t erase_size = BIT(exp) << STM32_QSPI_DOUBLE_FLASH;

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
		LOG_INF("layout page %u wastes space with device size %zu",
			layout_page_size, flash_size);
	}

	data->layout.pages_size = layout_page_size;
	data->layout.pages_count = flash_size / layout_page_size;
	LOG_DBG("layout %u x %u By pages", data->layout.pages_count,
					   data->layout.pages_size);

	return rv;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int qspi_program_addr_4b(const struct device *dev, bool write_enable)
{
	int ret;

	/* Send write enable command, if required */
	if (write_enable) {
		ret = qspi_send_cmd(dev, &cmd_write_en);
		if (ret != 0) {
			return ret;
		}
	}

	/* Program the flash memory to use 4 bytes addressing */
	QSPI_CommandTypeDef cmd = {
		.Instruction = SPI_NOR_CMD_4BA,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
	};

	/*
	 * No need to Read control register afterwards to verify if 4byte addressing mode
	 * is enabled as the effect of the command is immediate
	 * and the SPI_NOR_CMD_RDCR is vendor-specific :
	 * SPI_NOR_4BYTE_BIT is BIT 5 for Macronix and 0 for Micron or Windbond
	 * Moreover bit value meaning is also vendor-specific
	 */

	return qspi_send_cmd(dev, &cmd);
}

static int qspi_read_status_register(const struct device *dev, uint8_t reg_num, uint8_t *reg)
{
	QSPI_CommandTypeDef cmd = {
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	switch (reg_num) {
	case 1U:
		cmd.Instruction = SPI_NOR_CMD_RDSR;
		break;
	case 2U:
		cmd.Instruction = SPI_NOR_CMD_RDSR2;
		break;
	case 3U:
		cmd.Instruction = SPI_NOR_CMD_RDSR3;
		break;
	default:
		return -EINVAL;
	}

	return qspi_read_access(dev, &cmd, reg, sizeof(*reg));
}

static int qspi_write_status_register(const struct device *dev, uint8_t reg_num, uint8_t reg)
{
	struct flash_stm32_qspi_data *dev_data = dev->data;
	size_t size;
	uint8_t regs[4] = { 0 };
	uint8_t *regs_p;
	int ret;

	QSPI_CommandTypeDef cmd = {
		.Instruction = SPI_NOR_CMD_WRSR,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	if (reg_num == 1) {
		size = 1U;
		regs[0] = reg;
		regs_p = &regs[0];
		/* 1 byte write clears SR2, write SR2 as well */
		if (dev_data->qer_type == JESD216_DW15_QER_S2B1v1) {
			ret = qspi_read_status_register(dev, 2, &regs[1]);
			if (ret < 0) {
				return ret;
			}
			size = 2U;
		}
	} else if (reg_num == 2) {
		cmd.Instruction = SPI_NOR_CMD_WRSR2;
		size = 1U;
		regs[1] = reg;
		regs_p = &regs[1];
		/* if SR2 write needs SR1 */
		if ((dev_data->qer_type == JESD216_DW15_QER_VAL_S2B1v1) ||
		    (dev_data->qer_type == JESD216_DW15_QER_VAL_S2B1v4) ||
		    (dev_data->qer_type == JESD216_DW15_QER_VAL_S2B1v5)) {
			ret = qspi_read_status_register(dev, 1, &regs[0]);
			if (ret < 0) {
				return ret;
			}
			cmd.Instruction = SPI_NOR_CMD_WRSR;
			size = 2U;
			regs_p = &regs[0];
		}
	} else if (reg_num == 3) {
		cmd.Instruction = SPI_NOR_CMD_WRSR3;
		size = 1U;
		regs[2] = reg;
		regs_p = &regs[2];
	} else {
		return -EINVAL;
	}

	return qspi_write_access(dev, &cmd, regs_p, size);
}

static int qspi_write_enable(const struct device *dev)
{
	uint8_t reg;
	int ret;

	ret = qspi_send_cmd(dev, &cmd_write_en);
	if (ret) {
		return ret;
	}

	do {
		ret = qspi_read_status_register(dev, 1U, &reg);
	} while (!ret && !(reg & SPI_NOR_WEL_BIT));

	return ret;
}

static int qspi_program_quad_io(const struct device *dev)
{
	struct flash_stm32_qspi_data *data = dev->data;
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

	ret = qspi_read_status_register(dev, qe_reg_num, &reg);
	if (ret < 0) {
		return ret;
	}

	/* exit early if QE bit is already set */
	if ((reg & qe_bit) != 0U) {
		return 0;
	}

	reg |= qe_bit;

	ret = qspi_write_enable(dev);
	if (ret < 0) {
		return ret;
	}

	ret = qspi_write_status_register(dev, qe_reg_num, reg);
	if (ret < 0) {
		return ret;
	}

	ret = qspi_wait_until_ready(dev);
	if (ret < 0) {
		return ret;
	}

	/* validate that QE bit is set */
	ret = qspi_read_status_register(dev, qe_reg_num, &reg);
	if (ret < 0) {
		return ret;
	}

	if ((reg & qe_bit) == 0U) {
		LOG_ERR("Status Register %u [0x%02x] not set", qe_reg_num, reg);
		return -EIO;
	}

	return ret;
}

static int spi_nor_process_bfp(const struct device *dev,
			       const struct jesd216_param_header *php,
			       const struct jesd216_bfp *bfp)
{
	const struct flash_stm32_qspi_config *dev_cfg = dev->config;
	struct flash_stm32_qspi_data *data = dev->data;
	struct jesd216_erase_type *etp = data->erase_types;
	uint8_t addr_mode;
	const size_t flash_size = (jesd216_bfp_density(bfp) / 8U) << STM32_QSPI_DOUBLE_FLASH;
	int rc;

	if (flash_size != dev_cfg->flash_size) {
		LOG_ERR("Unexpected flash size: %u", flash_size);
	}

	LOG_INF("%s: %u MiBy flash", dev->name, (uint32_t)(flash_size >> 20));

	/* Copy over the erase types, preserving their order.  (The
	 * Sector Map Parameter table references them by index.)
	 */
	memset(data->erase_types, 0, sizeof(data->erase_types));
	for (uint8_t ti = 1; ti <= ARRAY_SIZE(data->erase_types); ++ti) {
		if (jesd216_bfp_erase(bfp, ti, etp) == 0) {
			LOG_DBG("Erase %u with %02x",
					(uint32_t)BIT(etp->exp), etp->cmd);
		}
		++etp;
	}

	data->page_size = jesd216_bfp_page_size(php, bfp);

	LOG_DBG("Page size %u bytes", data->page_size);
	LOG_DBG("Flash size %u bytes", flash_size);

	addr_mode = jesd216_bfp_addrbytes(bfp);
	if (addr_mode == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B4B) {
		struct jesd216_bfp_dw16 dw16;

		if (jesd216_bfp_decode_dw16(php, bfp, &dw16) == 0) {
			/*
			 * According to JESD216, the bit0 of dw16.enter_4ba
			 * portion of flash description register 16 indicates
			 * if it is enough to use 0xB7 instruction without
			 * write enable to switch to 4 bytes addressing mode.
			 * If bit 1 is set, a write enable is needed.
			 */
			if (dw16.enter_4ba & 0x3) {
				rc = qspi_program_addr_4b(dev, dw16.enter_4ba & 2);
				if (rc == 0) {
					data->flag_access_32bit = true;
					LOG_INF("Flash - address mode: 4B");
				} else {
					LOG_ERR("Unable to enter 4B mode: %d\n", rc);
					return rc;
				}
			}
		}
	}
	if (addr_mode == JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B) {
		data->flag_access_32bit = true;
		LOG_INF("Flash - address mode: 4B");
	}

	/*
	 * Only check if the 1-4-4 (i.e. 4READ) or 1-1-4 (QREAD)
	 * is supported - other modes are not.
	 */
	if (IS_ENABLED(STM32_QSPI_USE_QUAD_IO)) {
		const enum jesd216_mode_type supported_modes[] = { JESD216_MODE_114,
								   JESD216_MODE_144 };
		struct jesd216_bfp_dw15 dw15;
		struct jesd216_instr res;

		/* reset active mode */
		data->mode = STM32_QSPI_UNKNOWN_MODE;

		/* query supported read modes, begin from the slowest */
		for (size_t i = 0; i < ARRAY_SIZE(supported_modes); ++i) {
			rc = jesd216_bfp_read_support(php, bfp, supported_modes[i], &res);
			if (rc >= 0) {
				LOG_INF("Quad read mode %d instr [0x%x] supported",
					supported_modes[i], res.instr);

				data->mode = supported_modes[i];
				data->qspi_read_cmd = res.instr;
				data->qspi_read_cmd_latency = res.wait_states;

				if (res.mode_clocks) {
					data->qspi_read_cmd_latency += res.mode_clocks;
				}
			}
		}

		/* don't continue when there is no supported mode */
		if (data->mode == STM32_QSPI_UNKNOWN_MODE) {
			LOG_ERR("No supported flash read mode found");
			return -ENOTSUP;
		}

		LOG_INF("Quad read mode %d instr [0x%x] will be used", data->mode, res.instr);

		/* try to decode QE requirement type */
		rc = jesd216_bfp_decode_dw15(php, bfp, &dw15);
		if (rc < 0) {
			/* will use QER from DTS or default (refer to device data) */
			LOG_WRN("Unable to decode QE requirement [DW15]: %d", rc);
		} else {
			/* bypass DTS QER value */
			data->qer_type = dw15.qer;
		}

		LOG_INF("QE requirement mode: %x", data->qer_type);

		/* enable QE */
		rc = qspi_program_quad_io(dev);
		if (rc < 0) {
			LOG_ERR("Failed to enable Quad mode: %d", rc);
			return rc;
		}

		LOG_INF("Quad mode enabled");
	}

	return 0;
}

#if STM32_QSPI_RESET_GPIO
static void flash_stm32_qspi_gpio_reset(const struct device *dev)
{
	const struct flash_stm32_qspi_config *dev_cfg = dev->config;

	/* Generate RESETn pulse for the flash memory */
	gpio_pin_configure_dt(&dev_cfg->reset, GPIO_OUTPUT_ACTIVE);
	k_msleep(DT_INST_PROP(0, reset_gpios_duration));
	gpio_pin_set_dt(&dev_cfg->reset, 0);
}
#endif

#if STM32_QSPI_RESET_CMD
static int flash_stm32_qspi_send_reset(const struct device *dev)
{
	QSPI_CommandTypeDef cmd = {
		.Instruction = SPI_NOR_CMD_RESET_EN,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
	};
	int ret;

	ret = qspi_send_cmd(dev, &cmd);
	if (ret != 0) {
		LOG_ERR("%d: Failed to send RESET_EN", ret);
		return ret;
	}

	cmd.Instruction = SPI_NOR_CMD_RESET_MEM;
	ret = qspi_send_cmd(dev, &cmd);
	if (ret != 0) {
		LOG_ERR("%d: Failed to send RESET_MEM", ret);
		return ret;
	}

	LOG_DBG("Send Reset command");

	return 0;
}
#endif

static int flash_stm32_qspi_init(const struct device *dev)
{
	const struct flash_stm32_qspi_config *dev_cfg = dev->config;
	struct flash_stm32_qspi_data *dev_data = dev->data;
	uint32_t ahb_clock_freq;
	uint32_t prescaler = 0;
	int ret;

	/* Signals configuration */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("QSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

#if STM32_QSPI_RESET_GPIO
	flash_stm32_qspi_gpio_reset(dev);
#endif
#if STM32_QSPI_USE_DMA
	/*
	 * DMA configuration
	 * Due to use of QSPI HAL API in current driver,
	 * both HAL and Zephyr DMA drivers should be configured.
	 * The required configuration for Zephyr DMA driver should only provide
	 * the minimum information to inform the DMA slot will be in used and
	 * how to route callbacks.
	 */
	struct dma_config dma_cfg = dev_data->dma.cfg;
	static DMA_HandleTypeDef hdma;

	if (!device_is_ready(dev_data->dma.dev)) {
		LOG_ERR("%s device not ready", dev_data->dma.dev->name);
		return -ENODEV;
	}

	/* Proceed to the minimum Zephyr DMA driver init */
	dma_cfg.user_data = &hdma;
	/* HACK: This field is used to inform driver that it is overridden */
	dma_cfg.linked_channel = STM32_DMA_HAL_OVERRIDE;
	ret = dma_config(dev_data->dma.dev, dev_data->dma.channel, &dma_cfg);
	if (ret != 0) {
		return ret;
	}

	/* Proceed to the HAL DMA driver init */
	if (dma_cfg.source_data_size != dma_cfg.dest_data_size) {
		LOG_ERR("Source and destination data sizes not aligned");
		return -EINVAL;
	}

	int index = find_lsb_set(dma_cfg.source_data_size) - 1;

	hdma.Init.PeriphDataAlignment = table_p_size[index];
	hdma.Init.MemDataAlignment = table_m_size[index];
	hdma.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma.Init.MemInc = DMA_MINC_ENABLE;
	hdma.Init.Mode = DMA_NORMAL;
	hdma.Init.Priority = table_priority[dma_cfg.channel_priority];
#ifdef CONFIG_DMA_STM32_V1
	/* TODO: Not tested in this configuration */
	hdma.Init.Channel = dma_cfg.dma_slot;
	hdma.Instance = __LL_DMA_GET_STREAM_INSTANCE(dev_data->dma.reg,
						     dev_data->dma.channel);
#else
	hdma.Init.Request = dma_cfg.dma_slot;
#ifdef CONFIG_DMAMUX_STM32
	/* HAL expects a valid DMA channel (not a DMAMUX channel) */
	hdma.Instance = __LL_DMA_GET_CHANNEL_INSTANCE(dev_data->dma.reg,
						      dev_data->dma.channel);
#else
	hdma.Instance = __LL_DMA_GET_CHANNEL_INSTANCE(dev_data->dma.reg,
						      dev_data->dma.channel-1);
#endif
#endif /* CONFIG_DMA_STM32_V1 */

	/* Initialize DMA HAL */
	__HAL_LINKDMA(&dev_data->hqspi, hdma, hdma);
	HAL_DMA_Init(&hdma);

#endif /* STM32_QSPI_USE_DMA */

	/* Clock configuration */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t) &dev_cfg->pclken) != 0) {
		LOG_DBG("Could not enable QSPI clock");
		return -EIO;
	}

	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			(clock_control_subsys_t) &dev_cfg->pclken,
			&ahb_clock_freq) < 0) {
		LOG_DBG("Failed to get AHB clock frequency");
		return -EIO;
	}

	for (; prescaler <= STM32_QSPI_CLOCK_PRESCALER_MAX; prescaler++) {
		uint32_t clk = ahb_clock_freq / (prescaler + 1);

		if (clk <= dev_cfg->max_frequency) {
			break;
		}
	}
	__ASSERT_NO_MSG(prescaler <= STM32_QSPI_CLOCK_PRESCALER_MAX);
	/* Initialize QSPI HAL */
	dev_data->hqspi.Init.ClockPrescaler = prescaler;
	/* Give a bit position from 0 to 31 to the HAL init minus 1 for the DCR1 reg */
	dev_data->hqspi.Init.FlashSize = find_lsb_set(dev_cfg->flash_size) - 2;
#if DT_PROP(DT_NODELABEL(quadspi), dual_flash) && defined(QUADSPI_CR_DFM)
	/*
	 * When the DTS has <dual-flash>, it means Dual Flash Mode
	 * Even in DUAL flash config, the SDFP is read from one single quad-NOR
	 * else the magic nb is wrong (0x46465353)
	 * That means that the Dual Flash config is set after the SFDP sequence
	 */
	dev_data->hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
	dev_data->hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_3_CYCLE;
	dev_data->hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
	/* Set Dual Flash Mode only on MemoryMapped */
	dev_data->hqspi.Init.FlashID = QSPI_FLASH_ID_1;
#endif /* dual_flash */

	HAL_QSPI_Init(&dev_data->hqspi);

#if DT_NODE_HAS_PROP(DT_NODELABEL(quadspi), flash_id) && \
	defined(QUADSPI_CR_FSEL)
	/*
	 * Some stm32 mcu with quadspi (like stm32l47x or stm32l48x)
	 * does not support Dual-Flash Mode
	 */
	uint8_t qspi_flash_id = DT_PROP(DT_NODELABEL(quadspi), flash_id);

	HAL_QSPI_SetFlashID(&dev_data->hqspi,
			    (qspi_flash_id - 1) << QUADSPI_CR_FSEL_Pos);
#endif
	/* Initialize semaphores */
	k_sem_init(&dev_data->sem, 1, 1);
	k_sem_init(&dev_data->sync, 0, 1);

	/* Run IRQ init */
	dev_cfg->irq_config(dev);

#if STM32_QSPI_RESET_CMD
	flash_stm32_qspi_send_reset(dev);
	k_busy_wait(DT_INST_PROP(0, reset_cmd_wait));
#endif

	/* Run NOR init */
	const uint8_t decl_nph = 2;
	union {
		/* We only process BFP so use one parameter block */
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u;
	const struct jesd216_sfdp_header *hp = &u.sfdp;

	ret = qspi_read_sfdp(dev, 0, u.raw, sizeof(u.raw));
	if (ret != 0) {
		LOG_ERR("SFDP read failed: %d", ret);
		return ret;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	if (magic != JESD216_SFDP_MAGIC) {
		LOG_ERR("SFDP magic %08x invalid", magic);
		return -EINVAL;
	}

	LOG_INF("%s: SFDP v %u.%u AP %x with %u PH", dev->name,
		hp->rev_major, hp->rev_minor, hp->access, 1 + hp->nph);

	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_param_header *phpe = php +
						     MIN(decl_nph, 1 + hp->nph);

	while (php != phpe) {
		uint16_t id = jesd216_param_id(php);

		LOG_INF("PH%u: %04x rev %u.%u: %u DW @ %x",
			(php - hp->phdr), id, php->rev_major, php->rev_minor,
			php->len_dw, jesd216_param_addr(php));

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			union {
				uint32_t dw[20];
				struct jesd216_bfp bfp;
			} u2;
			const struct jesd216_bfp *bfp = &u2.bfp;

			ret = qspi_read_sfdp(dev, jesd216_param_addr(php),
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
		++php;
	}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	ret = setup_pages_layout(dev);
	if (ret != 0) {
		LOG_ERR("layout setup failed: %d", ret);
		return -ENODEV;
	}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

	ret = qspi_write_unprotect(dev);
	if (ret != 0) {
		LOG_ERR("write unprotect failed: %d", ret);
		return -ENODEV;
	}
	LOG_DBG("Write Un-protected");

#ifdef CONFIG_STM32_MEMMAP
#if DT_PROP(DT_NODELABEL(quadspi), dual_flash) && defined(QUADSPI_CR_DFM)
	/*
	 * When the DTS has dual_flash, it means Dual Flash Mode for Memory MAPPED
	 * Force Dual Flash mode now, after the SFDP sequence which is reading
	 * one quad-NOR only
	 */
	MODIFY_REG(dev_data->hqspi.Instance->CR, (QUADSPI_CR_DFM), QSPI_DUALFLASH_ENABLE);
	LOG_DBG("Dual Flash Mode");
#endif /* dual_flash */

	ret = stm32_qspi_set_memory_mapped(dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable memory-mapped mode: %d", ret);
		return ret;
	}
	LOG_INF("Memory-mapped NOR quad-flash at 0x%lx (0x%x bytes)",
		(long)(STM32_QSPI_BASE_ADDRESS),
		dev_cfg->flash_size);
#else
	LOG_INF("NOR quad-flash at 0x%lx (0x%x bytes)",
		(long)(STM32_QSPI_BASE_ADDRESS),
		dev_cfg->flash_size);
#endif

	return 0;
}

#define DMA_CHANNEL_CONFIG(node, dir)					\
		DT_DMAS_CELL_BY_NAME(node, dir, channel_config)

#define QSPI_DMA_CHANNEL_INIT(node, dir)				\
	.dev = DEVICE_DT_GET(DT_DMAS_CTLR(node)),			\
	.channel = DT_DMAS_CELL_BY_NAME(node, dir, channel),		\
	.reg = (DMA_TypeDef *)DT_REG_ADDR(				\
				   DT_PHANDLE_BY_NAME(node, dmas, dir)),\
	.cfg = {							\
		.dma_slot = DT_DMAS_CELL_BY_NAME(node, dir, slot),	\
		.source_data_size = STM32_DMA_CONFIG_PERIPHERAL_DATA_SIZE( \
					DMA_CHANNEL_CONFIG(node, dir)), \
		.dest_data_size = STM32_DMA_CONFIG_MEMORY_DATA_SIZE(    \
					DMA_CHANNEL_CONFIG(node, dir)), \
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(		\
					DMA_CHANNEL_CONFIG(node, dir)), \
		.dma_callback = qspi_dma_callback,			\
	},								\

#define QSPI_DMA_CHANNEL(node, dir)					\
	.dma = {							\
		COND_CODE_1(DT_DMAS_HAS_NAME(node, dir),		\
			(QSPI_DMA_CHANNEL_INIT(node, dir)),		\
			(NULL))						\
		},

#define QSPI_FLASH_MODULE(drv_id, flash_id) 				\
		(DT_DRV_INST(drv_id), qspi_nor_flash_##flash_id)

static void flash_stm32_qspi_irq_config_func(const struct device *dev);

#define DT_WRITEOC_PROP_OR(inst, default_value)                                              \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, writeoc),                                    \
		    (_CONCAT(SPI_NOR_CMD_, DT_STRING_TOKEN(DT_DRV_INST(inst), writeoc))),    \
		    ((default_value)))

#define DT_QER_PROP_OR(inst, default_value)                                                  \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, quad_enable_requirements),                   \
		    (_CONCAT(JESD216_DW15_QER_VAL_,                                          \
			     DT_STRING_TOKEN(DT_DRV_INST(inst), quad_enable_requirements))), \
		    ((default_value)))

#define STM32_QSPI_NODE DT_INST_PARENT(0)

PINCTRL_DT_DEFINE(STM32_QSPI_NODE);

static const struct flash_stm32_qspi_config flash_stm32_qspi_cfg = {
	.regs = (QUADSPI_TypeDef *)DT_REG_ADDR(STM32_QSPI_NODE),
	.pclken = {
		.enr = DT_CLOCKS_CELL(STM32_QSPI_NODE, bits),
		.bus = DT_CLOCKS_CELL(STM32_QSPI_NODE, bus)
	},
	.irq_config = flash_stm32_qspi_irq_config_func,
	.flash_size = DT_INST_REG_SIZE(0) << STM32_QSPI_DOUBLE_FLASH,
	.max_frequency = DT_INST_PROP(0, qspi_max_frequency),
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(STM32_QSPI_NODE),
#if STM32_QSPI_RESET_GPIO
	.reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
#endif
#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_qspi_nor), jedec_id)
	.jedec_id = DT_INST_PROP(0, jedec_id),
#endif /* jedec_id */
};

static struct flash_stm32_qspi_data flash_stm32_qspi_dev_data = {
	.hqspi = {
		.Instance = (QUADSPI_TypeDef *)DT_REG_ADDR(STM32_QSPI_NODE),
		.Init = {
			.FifoThreshold = STM32_QSPI_FIFO_THRESHOLD,
			.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE,
			.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE,
			.ClockMode = QSPI_CLOCK_MODE_0,
			},
	},
	.qer_type = DT_QER_PROP_OR(0, JESD216_DW15_QER_VAL_S1B6),
	.qspi_write_cmd = DT_WRITEOC_PROP_OR(0, SPI_NOR_CMD_PP_1_4_4),
	QSPI_DMA_CHANNEL(STM32_QSPI_NODE, tx_rx)
};

DEVICE_DT_INST_DEFINE(0, &flash_stm32_qspi_init, NULL,
		      &flash_stm32_qspi_dev_data, &flash_stm32_qspi_cfg,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		      &flash_stm32_qspi_driver_api);

static void flash_stm32_qspi_irq_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(STM32_QSPI_NODE), DT_IRQ(STM32_QSPI_NODE, priority),
		    flash_stm32_qspi_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_IRQN(STM32_QSPI_NODE));
}

#endif
