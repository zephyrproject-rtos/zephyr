/*
 * Copyright (c) 2022 STMicroelectronics
 * Copyright (c) 2022 Georgij Cernysiov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_ospi_nor

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/dt-bindings/flash_controller/ospi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>

#include "spi_nor.h"
#include "jesd216.h"

#include "flash_stm32_ospi.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_stm32_ospi, CONFIG_FLASH_LOG_LEVEL);

#define STM32_OSPI_RESET_GPIO DT_INST_NODE_HAS_PROP(0, reset_gpios)

#define STM32_OSPI_DLYB_BYPASSED DT_NODE_HAS_PROP(DT_PARENT(DT_DRV_INST(0)), dlyb_bypass)

#define STM32_OSPI_USE_DMA DT_NODE_HAS_PROP(DT_PARENT(DT_DRV_INST(0)), dmas)

#if STM32_OSPI_USE_DMA
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>
#include <stm32_ll_dma.h>
#endif /* STM32_OSPI_USE_DMA */

#define STM32_OSPI_FIFO_THRESHOLD         4
#define STM32_OSPI_CLOCK_PRESCALER_MAX  255

/* Max Time value during reset or erase operation */
#define STM32_OSPI_RESET_MAX_TIME               100U
#define STM32_OSPI_BULK_ERASE_MAX_TIME          460000U
#define STM32_OSPI_SECTOR_ERASE_MAX_TIME        1000U
#define STM32_OSPI_SUBSECTOR_4K_ERASE_MAX_TIME  400U
#define STM32_OSPI_WRITE_REG_MAX_TIME           40U

/* used as default value for DTS writeoc */
#define SPI_NOR_WRITEOC_NONE 0xFF

#if STM32_OSPI_USE_DMA
#if CONFIG_DMA_STM32U5
static const uint32_t table_src_size[] = {
	LL_DMA_SRC_DATAWIDTH_BYTE,
	LL_DMA_SRC_DATAWIDTH_HALFWORD,
	LL_DMA_SRC_DATAWIDTH_WORD,
};

static const uint32_t table_dest_size[] = {
	LL_DMA_DEST_DATAWIDTH_BYTE,
	LL_DMA_DEST_DATAWIDTH_HALFWORD,
	LL_DMA_DEST_DATAWIDTH_WORD,
};

/* Lookup table to set dma priority from the DTS */
static const uint32_t table_priority[] = {
	LL_DMA_LOW_PRIORITY_LOW_WEIGHT,
	LL_DMA_LOW_PRIORITY_MID_WEIGHT,
	LL_DMA_LOW_PRIORITY_HIGH_WEIGHT,
	LL_DMA_HIGH_PRIORITY,
};
#else
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
#endif /* CONFIG_DMA_STM32U5 */

struct stream {
	DMA_TypeDef *reg;
	const struct device *dev;
	uint32_t channel;
	struct dma_config cfg;
};
#endif /* STM32_OSPI_USE_DMA */

typedef void (*irq_config_func_t)(const struct device *dev);

#define STM32_OSPI_NODE DT_INST_PARENT(0)

struct flash_stm32_ospi_config {
	OCTOSPI_TypeDef *regs;
	const struct stm32_pclken pclken; /* clock subsystem */
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_ker)
	const struct stm32_pclken pclken_ker; /* clock subsystem */
#endif
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_mgr)
	const struct stm32_pclken pclken_mgr; /* clock subsystem */
#endif
	irq_config_func_t irq_config;
	size_t flash_size;
	uint32_t max_frequency;
	int data_mode; /* SPI or QSPI or OSPI */
	int data_rate; /* DTR or STR */
	const struct pinctrl_dev_config *pcfg;
#if STM32_OSPI_RESET_GPIO
	const struct gpio_dt_spec reset;
#endif /* STM32_OSPI_RESET_GPIO */
#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_ospi_nor), sfdp_bfp)
	uint8_t sfdp_bfp[DT_INST_PROP_LEN(0, sfdp_bfp)];
#endif /* sfdp_bfp */
};

struct flash_stm32_ospi_data {
	OSPI_HandleTypeDef hospi;
	struct k_sem sem;
	struct k_sem sync;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];
	/* Number of bytes per page */
	uint16_t page_size;
	/* Address width in bytes */
	uint8_t address_width;
	/* Read operation dummy cycles (wait states) */
	uint8_t read_dummy_cycles;
	uint32_t read_opcode;
	uint32_t write_opcode;
	enum jesd216_mode_type read_mode;
	enum jesd216_dw15_qer_type qer_type;
#if defined(CONFIG_FLASH_JESD216_API)
	/* Table to hold the jedec Read ID given by the octoFlash or the DTS */
	uint8_t jedec_id[JESD216_READ_ID_LEN];
#endif /* CONFIG_FLASH_JESD216_API */
	int cmd_status;
#if STM32_OSPI_USE_DMA
	struct stream dma;
#endif /* STM32_OSPI_USE_DMA */
};

static inline void ospi_lock_thread(const struct device *dev)
{
	struct flash_stm32_ospi_data *dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);
}

static inline void ospi_unlock_thread(const struct device *dev)
{
	struct flash_stm32_ospi_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
}

static int ospi_send_cmd(const struct device *dev, OSPI_RegularCmdTypeDef *cmd)
{
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	struct flash_stm32_ospi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	dev_data->cmd_status = 0;

	hal_ret = HAL_OSPI_Command(&dev_data->hospi, cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send OSPI instruction", hal_ret);
		return -EIO;
	}
	LOG_DBG("CCR 0x%x", dev_cfg->regs->CCR);

	return dev_data->cmd_status;
}

static int ospi_read_access(const struct device *dev, OSPI_RegularCmdTypeDef *cmd,
			    uint8_t *data, size_t size)
{
	struct flash_stm32_ospi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	cmd->NbData = size;

	dev_data->cmd_status = 0;

	hal_ret = HAL_OSPI_Command(&dev_data->hospi, cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send OSPI instruction", hal_ret);
		return -EIO;
	}

#if STM32_OSPI_USE_DMA
	hal_ret = HAL_OSPI_Receive_DMA(&dev_data->hospi, data);
#else
	hal_ret = HAL_OSPI_Receive_IT(&dev_data->hospi, data);
#endif
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}

	k_sem_take(&dev_data->sync, K_FOREVER);

	return dev_data->cmd_status;
}

static int ospi_write_access(const struct device *dev, OSPI_RegularCmdTypeDef *cmd,
			     const uint8_t *data, size_t size)
{
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	struct flash_stm32_ospi_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	cmd->NbData = size;

	dev_data->cmd_status = 0;

	/* in OPI/STR the 3-byte AddressSize is not supported by the NOR flash */
	if ((dev_cfg->data_mode == OSPI_OPI_MODE) &&
		(cmd->AddressSize != HAL_OSPI_ADDRESS_32_BITS)) {
		LOG_ERR("OSPI wr in OPI/STR mode is for 32bit address only");
		return -EIO;
	}

	hal_ret = HAL_OSPI_Command(&dev_data->hospi, cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send OSPI instruction", hal_ret);
		return -EIO;
	}

#if STM32_OSPI_USE_DMA
	hal_ret = HAL_OSPI_Transmit_DMA(&dev_data->hospi, (uint8_t *)data);
#else
	hal_ret = HAL_OSPI_Transmit_IT(&dev_data->hospi, (uint8_t *)data);
#endif

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to write data", hal_ret);
		return -EIO;
	}

	k_sem_take(&dev_data->sync, K_FOREVER);

	return dev_data->cmd_status;
}

/*
 * Gives a OSPI_RegularCmdTypeDef with all parameters set
 * except Instruction, Address, DummyCycles, NbData
 */
static OSPI_RegularCmdTypeDef ospi_prepare_cmd(uint8_t transfer_mode, uint8_t transfer_rate)
{
	OSPI_RegularCmdTypeDef cmd_tmp = {
		.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId = HAL_OSPI_FLASH_ID_1,
		.InstructionSize = ((transfer_mode == OSPI_OPI_MODE)
				? HAL_OSPI_INSTRUCTION_16_BITS
				: HAL_OSPI_INSTRUCTION_8_BITS),
		.InstructionDtrMode = ((transfer_rate == OSPI_DTR_TRANSFER)
				? HAL_OSPI_INSTRUCTION_DTR_ENABLE
				: HAL_OSPI_INSTRUCTION_DTR_DISABLE),
		.AddressDtrMode = ((transfer_rate == OSPI_DTR_TRANSFER)
				? HAL_OSPI_ADDRESS_DTR_ENABLE
				: HAL_OSPI_ADDRESS_DTR_DISABLE),
		/* AddressSize must be set to 32bits for init and mem config phase */
		.AddressSize = HAL_OSPI_ADDRESS_32_BITS,
		.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.DataDtrMode = ((transfer_rate == OSPI_DTR_TRANSFER)
				? HAL_OSPI_DATA_DTR_ENABLE
				: HAL_OSPI_DATA_DTR_DISABLE),
		.DQSMode = (transfer_rate == OSPI_DTR_TRANSFER)
				? HAL_OSPI_DQS_ENABLE
				: HAL_OSPI_DQS_DISABLE,
		.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD,
	};

	switch (transfer_mode) {
	case OSPI_OPI_MODE: {
		cmd_tmp.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
		cmd_tmp.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
		cmd_tmp.DataMode = HAL_OSPI_DATA_8_LINES;
		break;
	}
	case OSPI_QUAD_MODE: {
		cmd_tmp.InstructionMode = HAL_OSPI_INSTRUCTION_4_LINES;
		cmd_tmp.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
		cmd_tmp.DataMode = HAL_OSPI_DATA_4_LINES;
		break;
	}
	case OSPI_DUAL_MODE: {
		cmd_tmp.InstructionMode = HAL_OSPI_INSTRUCTION_2_LINES;
		cmd_tmp.AddressMode = HAL_OSPI_ADDRESS_2_LINES;
		cmd_tmp.DataMode = HAL_OSPI_DATA_2_LINES;
		break;
	}
	default: {
		cmd_tmp.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
		cmd_tmp.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
		cmd_tmp.DataMode = HAL_OSPI_DATA_1_LINE;
		break;
	}
	}

	return cmd_tmp;
}

#if defined(CONFIG_FLASH_JESD216_API)
/*
 * Read the JEDEC ID data from the octoFlash at init or DTS
 * and store in the jedec_id Table of the flash_stm32_ospi_data
 */
static int stm32_ospi_read_jedec_id(const struct device *dev)
{
	struct flash_stm32_ospi_data *dev_data = dev->data;

#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_ospi_nor), jedec_id)
	/* If DTS has the jedec_id property, check its length */
	if (DT_INST_PROP_LEN(0, jedec_id) != JESD216_READ_ID_LEN) {
		LOG_ERR("Read ID length is wrong (%d)", DT_INST_PROP_LEN(0, jedec_id));
		return -EIO;
	}

	/* The dev_data->jedec_id if filled from the DTS property */
#else
	/* This is a SPI/STR command to issue to the octoFlash device */
	OSPI_RegularCmdTypeDef cmd = ospi_prepare_cmd(OSPI_SPI_MODE, OSPI_STR_TRANSFER);

	cmd.Instruction = JESD216_CMD_READ_ID;
	cmd.DummyCycles = 8U;
	cmd.AddressSize = HAL_OSPI_ADDRESS_NONE;
	cmd.NbData = JESD216_READ_ID_LEN; /* 3 bytes in the READ ID */

	HAL_StatusTypeDef hal_ret;

	hal_ret = HAL_OSPI_Command(&dev_data->hospi, &cmd,
				   HAL_OSPI_TIMEOUT_DEFAULT_VALUE);

	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send OSPI instruction", hal_ret);
		return -EIO;
	}

	/* Place the received data directly into the jedec Table */
	hal_ret = HAL_OSPI_Receive(&dev_data->hospi, dev_data->jedec_id,
				   HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}
#endif /* jedec_id */
	LOG_DBG("Jedec ID = [%02x %02x %02x]",
		dev_data->jedec_id[0], dev_data->jedec_id[1], dev_data->jedec_id[2]);

	dev_data->cmd_status = 0;

	return 0;
}

/*
 * Read Serial Flash ID :
 * just gives the values received by the octoFlash or from the DTS
 */
static int ospi_read_jedec_id(const struct device *dev,  uint8_t *id)
{
	struct flash_stm32_ospi_data *dev_data = dev->data;

	/* Take jedec Id values from the table (issued from the octoFlash) */
	memcpy(id, dev_data->jedec_id, JESD216_READ_ID_LEN);

	LOG_INF("Manuf ID = %02x   Memory Type = %02x   Memory Density = %02x",
		id[0], id[1], id[2]);

	return 0;
}
#endif /* CONFIG_FLASH_JESD216_API */

#if !DT_NODE_HAS_PROP(DT_INST(0, st_stm32_ospi_nor), sfdp_bfp)
/*
 * Read Serial Flash Discovery Parameter from the octoFlash at init :
 * perform a read access over SPI bus for SDFP (DataMode is already set)
 */
static int stm32_ospi_read_sfdp(const struct device *dev, off_t addr,
				void *data,
				size_t size)
{
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	struct flash_stm32_ospi_data *dev_data = dev->data;

	OSPI_RegularCmdTypeDef cmd = ospi_prepare_cmd(dev_cfg->data_mode,
						      dev_cfg->data_rate);
	if (dev_cfg->data_mode == OSPI_OPI_MODE) {
		cmd.Instruction = JESD216_OCMD_READ_SFDP;
		cmd.DummyCycles = 20U;
		cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
	} else {
		cmd.Instruction = JESD216_CMD_READ_SFDP;
		cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
		cmd.DataMode = HAL_OSPI_DATA_1_LINE;
		cmd.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
		cmd.DummyCycles = 8U;
		cmd.AddressSize = HAL_OSPI_ADDRESS_24_BITS;
	}
	cmd.Address = addr;
	cmd.NbData = size;

	HAL_StatusTypeDef hal_ret;

	hal_ret = HAL_OSPI_Command(&dev_data->hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to send OSPI instruction", hal_ret);
		return -EIO;
	}

	hal_ret = HAL_OSPI_Receive(&dev_data->hospi, (uint8_t *)data,
				   HAL_OSPI_TIMEOUT_DEFAULT_VALUE);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}

	dev_data->cmd_status = 0;

	return 0;
}
#endif /* ! sfdp_bfp */

/*
 * Read Serial Flash Discovery Parameter :
 * perform a read access over SPI bus for SDFP (DataMode is already set)
 * or get it from the sdfp table (in the DTS)
 */
static int ospi_read_sfdp(const struct device *dev, off_t addr, void *data,
			  size_t size)
{
#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_ospi_nor), sfdp_bfp)
	/* There is a sfdp-bfp property in the deviceTree : do not read the flash */
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;

	LOG_INF("Read SFDP from DTS property");
	/* If DTS has the sdfp table property, check its length */
	if (size > DT_INST_PROP_LEN(0, sfdp_bfp)) {
		LOG_ERR("SDFP bdfp length is wrong (%d)", DT_INST_PROP_LEN(0, sfdp_bfp));
		return -EIO;
	}
	/* The dev_cfg->sfdp_bfp if filled from the DTS property */
	memcpy(data, dev_cfg->sfdp_bfp + addr, size);

	return 0;
#else
	LOG_INF("Read SFDP from octoFlash");
	/* Get the SFDP from the octoFlash (no sfdp-bfp table in the DeviceTree) */
	if (stm32_ospi_read_sfdp(dev, addr, data, size) == 0) {
		/* If valid, then ignore any table from the DTS */
		return 0;
	}
	LOG_INF("Error reading SFDP from octoFlash and none in the DTS");
	return -EINVAL;
#endif /* sfdp_bfp */
}

static bool ospi_address_is_valid(const struct device *dev, off_t addr,
				  size_t size)
{
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	size_t flash_size = dev_cfg->flash_size;

	return (addr >= 0) && ((uint64_t)addr + (uint64_t)size <= flash_size);
}

/*
 * This function Polls the WIP(Write In Progress) bit to become to 0
 * in nor_mode SPI/OPI OSPI_SPI_MODE or OSPI_OPI_MODE
 * and nor_rate transfer STR/DTR OSPI_STR_TRANSFER or OSPI_DTR_TRANSFER
 */
static int stm32_ospi_mem_ready(OSPI_HandleTypeDef *hospi, uint8_t nor_mode, uint8_t nor_rate)
{
	OSPI_AutoPollingTypeDef s_config = {0};
	OSPI_RegularCmdTypeDef s_command = ospi_prepare_cmd(nor_mode, nor_rate);

	/* Configure automatic polling mode command to wait for memory ready */
	if (nor_mode == OSPI_OPI_MODE) {
		s_command.Instruction = SPI_NOR_OCMD_RDSR;
		s_command.DummyCycles = (nor_rate == OSPI_DTR_TRANSFER)
					? SPI_NOR_DUMMY_REG_OCTAL_DTR
					: SPI_NOR_DUMMY_REG_OCTAL;
	} else {
		s_command.Instruction = SPI_NOR_CMD_RDSR;
		/* force 1-line InstructionMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
		s_command.AddressMode = HAL_OSPI_ADDRESS_NONE;
		/* force 1-line DataMode for any non-OSPI transfer */
		s_command.DataMode = HAL_OSPI_DATA_1_LINE;
		s_command.DummyCycles = 0;
	}
	s_command.NbData = ((nor_rate == OSPI_DTR_TRANSFER) ? 2U : 1U);
	s_command.Address = 0U;

	/* Set the mask to  0x01 to mask all Status REG bits except WIP */
	/* Set the match to 0x00 to check if the WIP bit is Reset */
	s_config.Match              = SPI_NOR_MEM_RDY_MATCH;
	s_config.Mask               = SPI_NOR_MEM_RDY_MASK; /* Write in progress */
	s_config.MatchMode          = HAL_OSPI_MATCH_MODE_AND;
	s_config.Interval           = SPI_NOR_AUTO_POLLING_INTERVAL;
	s_config.AutomaticStop      = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

	if (HAL_OSPI_Command(hospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI AutoPoll command failed");
		return -EIO;
	}

	/* Start Automatic-Polling mode to wait until the memory is ready WIP=0 */
	if (HAL_OSPI_AutoPolling(hospi, &s_config, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI AutoPoll failed");
		return -EIO;
	}

	return 0;
}

/* Enables writing to the memory sending a Write Enable and wait it is effective */
static int stm32_ospi_write_enable(OSPI_HandleTypeDef *hospi, uint8_t nor_mode, uint8_t nor_rate)
{
	OSPI_AutoPollingTypeDef s_config = {0};
	OSPI_RegularCmdTypeDef s_command = ospi_prepare_cmd(nor_mode, nor_rate);

	/* Initialize the write enable command */
	if (nor_mode == OSPI_OPI_MODE) {
		s_command.Instruction = SPI_NOR_OCMD_WREN;
	} else {
		s_command.Instruction = SPI_NOR_CMD_WREN;
		/* force 1-line InstructionMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
	}
	s_command.AddressMode = HAL_OSPI_ADDRESS_NONE;
	s_command.DataMode    = HAL_OSPI_DATA_NONE;
	s_command.DummyCycles = 0U;

	if (HAL_OSPI_Command(hospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI flash write enable cmd failed");
		return -EIO;
	}

	/* New command to Configure automatic polling mode to wait for write enabling */
	if (nor_mode == OSPI_OPI_MODE) {
		s_command.Instruction = SPI_NOR_OCMD_RDSR;
		s_command.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
		s_command.DataMode = HAL_OSPI_DATA_8_LINES;
		s_command.DummyCycles = (nor_rate == OSPI_DTR_TRANSFER)
				? SPI_NOR_DUMMY_REG_OCTAL_DTR
						: SPI_NOR_DUMMY_REG_OCTAL;
	} else {
		s_command.Instruction = SPI_NOR_CMD_RDSR;
		/* force 1-line DataMode for any non-OSPI transfer */
		s_command.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
		s_command.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
		s_command.DataMode = HAL_OSPI_DATA_1_LINE;
		s_command.DummyCycles = 0;

		/* DummyCycles remains 0 */
	}
	s_command.NbData = (nor_rate == OSPI_DTR_TRANSFER) ? 2U : 1U;
	s_command.Address = 0U;

	if (HAL_OSPI_Command(hospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI config auto polling cmd failed");
		return -EIO;
	}

	s_config.Match           = SPI_NOR_WREN_MATCH;
	s_config.Mask            = SPI_NOR_WREN_MASK;
	s_config.MatchMode       = HAL_OSPI_MATCH_MODE_AND;
	s_config.Interval        = SPI_NOR_AUTO_POLLING_INTERVAL;
	s_config.AutomaticStop   = HAL_OSPI_AUTOMATIC_STOP_ENABLE;

	if (HAL_OSPI_AutoPolling(hospi, &s_config, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI config auto polling failed");
		return -EIO;
	}

	return 0;
}

/* Write Flash configuration register 2 with new dummy cycles */
static int stm32_ospi_write_cfg2reg_dummy(OSPI_HandleTypeDef *hospi,
					uint8_t nor_mode, uint8_t nor_rate)
{
	uint8_t transmit_data = SPI_NOR_CR2_DUMMY_CYCLES_66MHZ;
	OSPI_RegularCmdTypeDef s_command = ospi_prepare_cmd(nor_mode, nor_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (nor_mode == OSPI_SPI_MODE)
				? SPI_NOR_CMD_WR_CFGREG2
				: SPI_NOR_OCMD_WR_CFGREG2;
	s_command.Address = SPI_NOR_REG2_ADDR3;
	s_command.DummyCycles = 0U;
	s_command.NbData = (nor_mode == OSPI_SPI_MODE) ? 1U
			: ((nor_rate == OSPI_DTR_TRANSFER) ? 2U : 1U);

	if (HAL_OSPI_Command(hospi, &s_command,
		HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI transmit ");
		return -EIO;
	}

	if (HAL_OSPI_Transmit(hospi, &transmit_data,
		HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI transmit ");
		return -EIO;
	}

	return 0;
}

/* Write Flash configuration register 2 with new single or octal SPI protocol */
static int stm32_ospi_write_cfg2reg_io(OSPI_HandleTypeDef *hospi,
				       uint8_t nor_mode, uint8_t nor_rate, uint8_t op_enable)
{
	OSPI_RegularCmdTypeDef s_command = ospi_prepare_cmd(nor_mode, nor_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (nor_mode == OSPI_SPI_MODE)
				? SPI_NOR_CMD_WR_CFGREG2
				: SPI_NOR_OCMD_WR_CFGREG2;
	s_command.Address = SPI_NOR_REG2_ADDR1;
	s_command.DummyCycles = 0U;
	s_command.NbData = (nor_mode == OSPI_SPI_MODE) ? 1U
			: ((nor_rate == OSPI_DTR_TRANSFER) ? 2U : 1U);

	if (HAL_OSPI_Command(hospi, &s_command,
		HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	if (HAL_OSPI_Transmit(hospi, &op_enable,
		HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	return 0;
}

/* Read Flash configuration register 2 with new single or octal SPI protocol */
static int stm32_ospi_read_cfg2reg(OSPI_HandleTypeDef *hospi,
				   uint8_t nor_mode, uint8_t nor_rate, uint8_t *value)
{
	OSPI_RegularCmdTypeDef s_command = ospi_prepare_cmd(nor_mode, nor_rate);

	/* Initialize the writing of configuration register 2 */
	s_command.Instruction = (nor_mode == OSPI_SPI_MODE)
				? SPI_NOR_CMD_RD_CFGREG2
				: SPI_NOR_OCMD_RD_CFGREG2;
	s_command.Address = SPI_NOR_REG2_ADDR1;
	s_command.DummyCycles = (nor_mode == OSPI_SPI_MODE)
				? 0U
				: ((nor_rate == OSPI_DTR_TRANSFER)
					? SPI_NOR_DUMMY_REG_OCTAL_DTR
					: SPI_NOR_DUMMY_REG_OCTAL);
	s_command.NbData = (nor_rate == OSPI_DTR_TRANSFER) ? 2U : 1U;

	if (HAL_OSPI_Command(hospi, &s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	if (HAL_OSPI_Receive(hospi, value, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("Write Flash configuration reg2 failed");
		return -EIO;
	}

	return 0;
}

/* Set the NOR Flash to desired Interface mode : SPI/OSPI and STR/DTR according to the DTS */
static int stm32_ospi_config_mem(const struct device *dev)
{
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	struct flash_stm32_ospi_data *dev_data = dev->data;
	uint8_t reg[2];

	/* Going to set the SPI mode and STR transfer rate : done */
	if ((dev_cfg->data_mode != OSPI_OPI_MODE)
		&& (dev_cfg->data_rate == OSPI_STR_TRANSFER)) {
		LOG_INF("OSPI flash config is SPI|DUAL|QUAD / STR");
		return 0;
	}

	/* Going to set the OPI mode (STR or DTR transfer rate) */
	LOG_DBG("OSPI configuring OctoSPI mode");

	if (stm32_ospi_write_enable(&dev_data->hospi,
		OSPI_SPI_MODE, OSPI_STR_TRANSFER) != 0) {
		LOG_ERR("OSPI write Enable failed");
		return -EIO;
	}

	/* Write Configuration register 2 (with new dummy cycles) */
	if (stm32_ospi_write_cfg2reg_dummy(&dev_data->hospi,
		OSPI_SPI_MODE, OSPI_STR_TRANSFER) != 0) {
		LOG_ERR("OSPI write CFGR2 failed");
		return -EIO;
	}
	if (stm32_ospi_mem_ready(&dev_data->hospi,
		OSPI_SPI_MODE, OSPI_STR_TRANSFER) != 0) {
		LOG_ERR("OSPI autopolling failed");
		return -EIO;
	}
	if (stm32_ospi_write_enable(&dev_data->hospi,
		OSPI_SPI_MODE, OSPI_STR_TRANSFER) != 0) {
		LOG_ERR("OSPI write Enable 2 failed");
		return -EIO;
	}

	/* Write Configuration register 2 (with Octal I/O SPI protocol : choose STR or DTR) */
	uint8_t mode_enable = ((dev_cfg->data_rate == OSPI_DTR_TRANSFER)
				? SPI_NOR_CR2_DTR_OPI_EN
				: SPI_NOR_CR2_STR_OPI_EN);
	if (stm32_ospi_write_cfg2reg_io(&dev_data->hospi,
		OSPI_SPI_MODE, OSPI_STR_TRANSFER, mode_enable) != 0) {
		LOG_ERR("OSPI write CFGR2 failed");
		return -EIO;
	}

	/* Wait that the configuration is effective and check that memory is ready */
	k_msleep(STM32_OSPI_WRITE_REG_MAX_TIME);

	/* Reconfigure the memory type of the peripheral */
	dev_data->hospi.Init.MemoryType            = HAL_OSPI_MEMTYPE_MACRONIX;
	dev_data->hospi.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;
	if (HAL_OSPI_Init(&dev_data->hospi) != HAL_OK) {
		LOG_ERR("OSPI mem type MACRONIX failed");
		return -EIO;
	}

	if (dev_cfg->data_rate == OSPI_STR_TRANSFER) {
		if (stm32_ospi_mem_ready(&dev_data->hospi,
			OSPI_OPI_MODE, OSPI_STR_TRANSFER) != 0) {
			/* Check Flash busy ? */
			LOG_ERR("OSPI flash busy failed");
			return -EIO;
		}

		if (stm32_ospi_read_cfg2reg(&dev_data->hospi,
			OSPI_OPI_MODE, OSPI_STR_TRANSFER, reg) != 0) {
			/* Check the configuration has been correctly done on SPI_NOR_REG2_ADDR1 */
			LOG_ERR("OSPI flash config read failed");
			return -EIO;
		}

		LOG_INF("OSPI flash config is OPI / STR");
	}

	if (dev_cfg->data_rate == OSPI_DTR_TRANSFER) {
		if (stm32_ospi_mem_ready(&dev_data->hospi,
			OSPI_OPI_MODE, OSPI_DTR_TRANSFER) != 0) {
			/* Check Flash busy ? */
			LOG_ERR("OSPI flash busy failed");
			return -EIO;
		}

		LOG_INF("OSPI flash config is OPI / DTR");
	}

	return 0;
}

/* gpio or send the different reset command to the NOR flash in SPI/OSPI and STR/DTR */
static int stm32_ospi_mem_reset(const struct device *dev)
{
	struct flash_stm32_ospi_data *dev_data = dev->data;

#if STM32_OSPI_RESET_GPIO
	/* Generate RESETn pulse for the flash memory */
	gpio_pin_configure_dt(&dev_cfg->reset, GPIO_OUTPUT_ACTIVE);
	k_msleep(DT_INST_PROP(0, reset_gpios_duration));
	gpio_pin_set_dt(&dev_cfg->reset, 0);
#else

	/* Reset command sent sucessively for each mode SPI/OPS & STR/DTR */
	OSPI_RegularCmdTypeDef s_command = {
		.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId = HAL_OSPI_FLASH_ID_1,
		.AddressMode = HAL_OSPI_ADDRESS_NONE,
		.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Instruction = SPI_NOR_CMD_RESET_EN,
		.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS,
		.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.DataMode = HAL_OSPI_DATA_NONE,
		.DummyCycles = 0U,
		.DQSMode = HAL_OSPI_DQS_DISABLE,
		.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD,
	};

	/* Reset enable in SPI mode and STR transfer mode */
	if (HAL_OSPI_Command(&dev_data->hospi,
		&s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI reset enable (SPI/STR) failed");
		return -EIO;
	}

	/* Reset memory in SPI mode and STR transfer mode */
	s_command.Instruction = SPI_NOR_CMD_RESET_MEM;
	if (HAL_OSPI_Command(&dev_data->hospi,
		&s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI reset memory (SPI/STR) failed");
		return -EIO;
	}

	/* Reset enable in OPI mode and STR transfer mode */
	s_command.InstructionMode    = HAL_OSPI_INSTRUCTION_8_LINES;
	s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	s_command.Instruction = SPI_NOR_OCMD_RESET_EN;
	s_command.InstructionSize = HAL_OSPI_INSTRUCTION_16_BITS;
	if (HAL_OSPI_Command(&dev_data->hospi,
		&s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI reset enable (OPI/STR) failed");
		return -EIO;
	}

	/* Reset memory in OPI mode and STR transfer mode */
	s_command.Instruction = SPI_NOR_OCMD_RESET_MEM;
	if (HAL_OSPI_Command(&dev_data->hospi,
		&s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI reset memory (OPI/STR) failed");
		return -EIO;
	}

	/* Reset enable in OPI mode and DTR transfer mode */
	s_command.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_ENABLE;
	s_command.Instruction = SPI_NOR_OCMD_RESET_EN;
	if (HAL_OSPI_Command(&dev_data->hospi,
		&s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI reset enable (OPI/DTR) failed");
		return -EIO;
	}

	/* Reset memory in OPI mode and DTR transfer mode */
	s_command.Instruction = SPI_NOR_OCMD_RESET_MEM;
	if (HAL_OSPI_Command(&dev_data->hospi,
		&s_command, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI reset memory (OPI/DTR) failed");
		return -EIO;
	}

#endif
	/* After SWreset CMD, wait in case SWReset occurred during erase operation */
	k_msleep(STM32_OSPI_RESET_MAX_TIME);

	return 0;
}

static uint32_t stm32_ospi_hal_address_size(const struct device *dev)
{
	struct flash_stm32_ospi_data *dev_data = dev->data;

	if (dev_data->address_width == 4U) {
		return HAL_OSPI_ADDRESS_32_BITS;
	}

	return HAL_OSPI_ADDRESS_24_BITS;
}

/*
 * Function to erase the flash : chip or sector with possible OSPI/SPI and STR/DTR
 * to erase the complete chip (using dedicated command) :
 *   set size >= flash size
 *   set addr = 0
 */
static int flash_stm32_ospi_erase(const struct device *dev, off_t addr,
				  size_t size)
{
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	struct flash_stm32_ospi_data *dev_data = dev->data;
	int ret = 0;

	/* Ignore zero size erase */
	if (size == 0) {
		return 0;
	}

	/* Maximise erase size : means the complete chip */
	if (size > dev_cfg->flash_size) {
		/* Reset addr in that case */
		addr = 0;
		size = dev_cfg->flash_size;
	}

	if (!ospi_address_is_valid(dev, addr, size)) {
		LOG_ERR("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	if (((size % SPI_NOR_SECTOR_SIZE) != 0) && (size < dev_cfg->flash_size)) {
		LOG_ERR("Error: wrong sector size 0x%x", size);
		return -ENOTSUP;
	}

	OSPI_RegularCmdTypeDef cmd_erase = {
		.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId = HAL_OSPI_FLASH_ID_1,
		.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.DataMode = HAL_OSPI_DATA_NONE,
		.DummyCycles = 0U,
		.DQSMode = HAL_OSPI_DQS_DISABLE,
		.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD,
	};

	ospi_lock_thread(dev);

	if (stm32_ospi_mem_ready(&dev_data->hospi,
		dev_cfg->data_mode, dev_cfg->data_rate) != 0) {
		ospi_unlock_thread(dev);
		LOG_ERR("Erase failed : flash busy");
		return -EBUSY;
	}

	cmd_erase.InstructionMode    = (dev_cfg->data_mode == OSPI_OPI_MODE)
					? HAL_OSPI_INSTRUCTION_8_LINES
					: HAL_OSPI_INSTRUCTION_1_LINE;
	cmd_erase.InstructionDtrMode = (dev_cfg->data_rate == OSPI_DTR_TRANSFER)
					? HAL_OSPI_INSTRUCTION_DTR_ENABLE
					: HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	cmd_erase.InstructionSize    = (dev_cfg->data_mode == OSPI_OPI_MODE)
					? HAL_OSPI_INSTRUCTION_16_BITS
					: HAL_OSPI_INSTRUCTION_8_BITS;

	while ((size > 0) && (ret == 0)) {

		ret = stm32_ospi_write_enable(&dev_data->hospi,
			dev_cfg->data_mode, dev_cfg->data_rate);
		if (ret != 0) {
			LOG_ERR("Erase failed : write enable");
			break;
		}

		if (size == dev_cfg->flash_size) {
			/* Chip erase */
			LOG_DBG("Chip Erase");
			cmd_erase.Instruction = (dev_cfg->data_mode == OSPI_OPI_MODE)
					? SPI_NOR_OCMD_BULKE
					: SPI_NOR_CMD_BULKE;
			cmd_erase.AddressMode = HAL_OSPI_ADDRESS_NONE;
			/* Full chip erase command */
			ospi_send_cmd(dev, &cmd_erase);

			size -= dev_cfg->flash_size;
		} else {
			/* Sector erase */
			LOG_DBG("Sector Erase");

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
				    && SPI_NOR_IS_ALIGNED(size, etp->exp)
				    && ((bet == NULL)
					|| (etp->exp > bet->exp))) {
					bet = etp;
					cmd_erase.Instruction = bet->cmd;
				} else {
					/* Use the default sector erase cmd */
					cmd_erase.Instruction =
						(dev_cfg->data_mode == OSPI_OPI_MODE)
						? SPI_NOR_OCMD_SE
						: SPI_NOR_CMD_SE;  /* Erase sector size 4K-Bytes */
					cmd_erase.AddressMode =
						(dev_cfg->data_mode == OSPI_OPI_MODE)
						? HAL_OSPI_ADDRESS_8_LINES
						: HAL_OSPI_ADDRESS_1_LINE;
					cmd_erase.AddressDtrMode =
						(dev_cfg->data_rate == OSPI_DTR_TRANSFER)
						? HAL_OSPI_ADDRESS_DTR_ENABLE
						: HAL_OSPI_ADDRESS_DTR_DISABLE;
					cmd_erase.AddressSize =
						(dev_cfg->data_mode == OSPI_OPI_MODE)
						? stm32_ospi_hal_address_size(dev)
						: HAL_OSPI_ADDRESS_24_BITS;
					cmd_erase.Address = addr;
					/* Avoid using wrong erase type,
					 * if zero entries are found in erase_types
					 */
					bet = NULL;
				}
			}

			ospi_send_cmd(dev, &cmd_erase);

			if (bet != NULL) {
				addr += BIT(bet->exp);
				size -= BIT(bet->exp);
			} else {
				addr += SPI_NOR_SECTOR_SIZE;
				size -= SPI_NOR_SECTOR_SIZE;
			}

			ret = stm32_ospi_mem_ready(&dev_data->hospi,
						   dev_cfg->data_mode, dev_cfg->data_rate);
		}
	}

	ospi_unlock_thread(dev);

	return ret;
}

/* Function to read the flash with possible OSPI/SPI and STR/DTR */
static int flash_stm32_ospi_read(const struct device *dev, off_t addr,
				 void *data, size_t size)
{
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	struct flash_stm32_ospi_data *dev_data = dev->data;
	int ret;

	if (!ospi_address_is_valid(dev, addr, size)) {
		LOG_ERR("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	/* Ignore zero size read */
	if (size == 0) {
		return 0;
	}

	OSPI_RegularCmdTypeDef cmd = ospi_prepare_cmd(dev_cfg->data_mode, dev_cfg->data_rate);

	if (dev_cfg->data_mode != OSPI_OPI_MODE) {
		switch (dev_data->read_mode) {
		case JESD216_MODE_112: {
			cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
			cmd.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
			cmd.DataMode = HAL_OSPI_DATA_2_LINES;
			break;
		}
		case JESD216_MODE_122: {
			cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
			cmd.AddressMode = HAL_OSPI_ADDRESS_2_LINES;
			cmd.DataMode = HAL_OSPI_DATA_2_LINES;
			break;
		}
		case JESD216_MODE_114: {
			cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
			cmd.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
			cmd.DataMode = HAL_OSPI_DATA_4_LINES;
			break;
		}
		case JESD216_MODE_144: {
			cmd.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
			cmd.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
			cmd.DataMode = HAL_OSPI_DATA_4_LINES;
			break;
		}
		default:
			/* use the mode from ospi_prepare_cmd */
			break;
		}
	}

	/* Instruction and DummyCycles are set below */
	cmd.Address = addr; /* AddressSize is 32bits in OPSI mode */
	cmd.AddressSize = stm32_ospi_hal_address_size(dev);
	/* DataSize is set by the read cmd */

	/* Configure other parameters */
	if (dev_cfg->data_rate == OSPI_DTR_TRANSFER) {
		/* DTR transfer rate (==> Octal mode) */
		cmd.Instruction = SPI_NOR_OCMD_DTR_RD;
		cmd.DummyCycles = SPI_NOR_DUMMY_RD_OCTAL_DTR;
	} else {
		/* STR transfer rate */
		if (dev_cfg->data_mode == OSPI_OPI_MODE) {
			/* OPI and STR */
			cmd.Instruction = SPI_NOR_OCMD_RD;
			cmd.DummyCycles = SPI_NOR_DUMMY_RD_OCTAL;
		} else {
			/* use SFDP:BFP read instruction */
			cmd.Instruction = dev_data->read_opcode;
			cmd.DummyCycles = dev_data->read_dummy_cycles;
			/* in SPI and STR : expecting SPI_NOR_CMD_READ_FAST_4B */
		}
	}

	LOG_DBG("OSPI: read %zu data", size);
	ospi_lock_thread(dev);

	ret = ospi_read_access(dev, &cmd, data, size);

	ospi_unlock_thread(dev);

	return ret;
}

/* Function to write the flash (page program) : with possible OSPI/SPI and STR/DTR */
static int flash_stm32_ospi_write(const struct device *dev, off_t addr,
				  const void *data, size_t size)
{
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	struct flash_stm32_ospi_data *dev_data = dev->data;
	size_t to_write;
	int ret = 0;

	if (!ospi_address_is_valid(dev, addr, size)) {
		LOG_ERR("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	/* Ignore zero size write */
	if (size == 0) {
		return 0;
	}

	/* page program for STR or DTR mode */
	OSPI_RegularCmdTypeDef cmd_pp = ospi_prepare_cmd(dev_cfg->data_mode, dev_cfg->data_rate);

	/* using 32bits address also in SPI/STR mode */
	cmd_pp.Instruction = dev_data->write_opcode;

	if (dev_cfg->data_mode != OSPI_OPI_MODE) {
		switch (cmd_pp.Instruction) {
		case SPI_NOR_CMD_PP_4B:
			__fallthrough;
		case SPI_NOR_CMD_PP: {
			cmd_pp.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
			cmd_pp.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
			cmd_pp.DataMode = HAL_OSPI_DATA_1_LINE;
			break;
		}
		case SPI_NOR_CMD_PP_1_1_4_4B:
			__fallthrough;
		case SPI_NOR_CMD_PP_1_1_4: {
			cmd_pp.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
			cmd_pp.AddressMode = HAL_OSPI_ADDRESS_1_LINE;
			cmd_pp.DataMode = HAL_OSPI_DATA_4_LINES;
			break;
		}
		case SPI_NOR_CMD_PP_1_4_4_4B:
			__fallthrough;
		case SPI_NOR_CMD_PP_1_4_4: {
			cmd_pp.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE;
			cmd_pp.AddressMode = HAL_OSPI_ADDRESS_4_LINES;
			cmd_pp.DataMode = HAL_OSPI_DATA_4_LINES;
			break;
		}
		default:
			/* use the mode from ospi_prepare_cmd */
			break;
		}
	}

	cmd_pp.Address = addr;
	cmd_pp.AddressSize = stm32_ospi_hal_address_size(dev);
	cmd_pp.DummyCycles = 0U;

	LOG_DBG("OSPI: write %zu data", size);
	ospi_lock_thread(dev);

	ret = stm32_ospi_mem_ready(&dev_data->hospi,
				   dev_cfg->data_mode, dev_cfg->data_rate);
	if (ret != 0) {
		ospi_unlock_thread(dev);
		LOG_ERR("OSPI: write not ready");
		return -EIO;
	}

	while ((size > 0) && (ret == 0)) {
		to_write = size;
		ret = stm32_ospi_write_enable(&dev_data->hospi,
						    dev_cfg->data_mode, dev_cfg->data_rate);
		if (ret != 0) {
			LOG_ERR("OSPI: write not enabled");
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

		ret = ospi_write_access(dev, &cmd_pp, data, to_write);
		if (ret != 0) {
			LOG_ERR("OSPI: write not access");
			break;
		}

		size -= to_write;
		data = (const uint8_t *)data + to_write;
		addr += to_write;

		/* Configure automatic polling mode to wait for end of program */
		ret = stm32_ospi_mem_ready(&dev_data->hospi,
						 dev_cfg->data_mode, dev_cfg->data_rate);
		if (ret != 0) {
			LOG_ERR("OSPI: write PP not ready");
			break;
		}
	}

	ospi_unlock_thread(dev);

	return ret;
}

static const struct flash_parameters flash_stm32_ospi_parameters = {
	.write_block_size = 1,
	.erase_value = 0xff
};

static const struct flash_parameters *
flash_stm32_ospi_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_stm32_ospi_parameters;
}

static void flash_stm32_ospi_isr(const struct device *dev)
{
	struct flash_stm32_ospi_data *dev_data = dev->data;

	HAL_OSPI_IRQHandler(&dev_data->hospi);
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
#if STM32_OSPI_USE_DMA
static void ospi_dma_callback(const struct device *dev, void *arg,
			 uint32_t channel, int status)
{
	DMA_HandleTypeDef *hdma = arg;

	ARG_UNUSED(dev);

	if (status != 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);
	}

	HAL_DMA_IRQHandler(hdma);
}
#endif

/*
 * Transfer Error callback.
 */
void HAL_OSPI_ErrorCallback(OSPI_HandleTypeDef *hospi)
{
	struct flash_stm32_ospi_data *dev_data =
		CONTAINER_OF(hospi, struct flash_stm32_ospi_data, hospi);

	LOG_DBG("Error cb");

	dev_data->cmd_status = -EIO;

	k_sem_give(&dev_data->sync);
}

/*
 * Command completed callback.
 */
void HAL_OSPI_CmdCpltCallback(OSPI_HandleTypeDef *hospi)
{
	struct flash_stm32_ospi_data *dev_data =
		CONTAINER_OF(hospi, struct flash_stm32_ospi_data, hospi);

	LOG_DBG("Cmd Cplt cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Rx Transfer completed callback.
 */
void HAL_OSPI_RxCpltCallback(OSPI_HandleTypeDef *hospi)
{
	struct flash_stm32_ospi_data *dev_data =
		CONTAINER_OF(hospi, struct flash_stm32_ospi_data, hospi);

	LOG_DBG("Rx Cplt cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Tx Transfer completed callback.
 */
void HAL_OSPI_TxCpltCallback(OSPI_HandleTypeDef *hospi)
{
	struct flash_stm32_ospi_data *dev_data =
		CONTAINER_OF(hospi, struct flash_stm32_ospi_data, hospi);

	LOG_DBG("Tx Cplt cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Status Match callback.
 */
void HAL_OSPI_StatusMatchCallback(OSPI_HandleTypeDef *hospi)
{
	struct flash_stm32_ospi_data *dev_data =
		CONTAINER_OF(hospi, struct flash_stm32_ospi_data, hospi);

	LOG_DBG("Status Match cb");

	k_sem_give(&dev_data->sync);
}

/*
 * Timeout callback.
 */
void HAL_OSPI_TimeOutCallback(OSPI_HandleTypeDef *hospi)
{
	struct flash_stm32_ospi_data *dev_data =
		CONTAINER_OF(hospi, struct flash_stm32_ospi_data, hospi);

	LOG_DBG("Timeout cb");

	dev_data->cmd_status = -EIO;

	k_sem_give(&dev_data->sync);
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_stm32_ospi_pages_layout(const struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	struct flash_stm32_ospi_data *dev_data = dev->data;

	*layout = &dev_data->layout;
	*layout_size = 1;
}
#endif

static const struct flash_driver_api flash_stm32_ospi_driver_api = {
	.read = flash_stm32_ospi_read,
	.write = flash_stm32_ospi_write,
	.erase = flash_stm32_ospi_erase,
	.get_parameters = flash_stm32_ospi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_stm32_ospi_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = ospi_read_sfdp,
	.read_jedec_id = ospi_read_jedec_id,
#endif /* CONFIG_FLASH_JESD216_API */
};

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static int setup_pages_layout(const struct device *dev)
{
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	struct flash_stm32_ospi_data *data = dev->data;
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

static int stm32_ospi_read_status_register(const struct device *dev, uint8_t reg_num, uint8_t *reg)
{
	OSPI_RegularCmdTypeDef s_command = {
		.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE,
		.DataMode = HAL_OSPI_DATA_1_LINE,
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

	return ospi_read_access(dev, &s_command, reg, sizeof(*reg));
}

static int stm32_ospi_write_status_register(const struct device *dev, uint8_t reg_num, uint8_t reg)
{
	struct flash_stm32_ospi_data *data = dev->data;
	OSPI_RegularCmdTypeDef s_command = {
		.Instruction = SPI_NOR_CMD_WRSR,
		.InstructionMode = HAL_OSPI_INSTRUCTION_1_LINE,
		.DataMode = HAL_OSPI_DATA_1_LINE
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
			ret = stm32_ospi_read_status_register(dev, 2, &regs[1]);
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
			ret = stm32_ospi_read_status_register(dev, 1, &regs[0]);
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

	return ospi_write_access(dev, &s_command, regs_p, size);
}

static int stm32_ospi_enable_qe(const struct device *dev)
{
	struct flash_stm32_ospi_data *data = dev->data;
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

	ret = stm32_ospi_read_status_register(dev, qe_reg_num, &reg);
	if (ret < 0) {
		return ret;
	}

	/* exit early if QE bit is already set */
	if ((reg & qe_bit) != 0U) {
		return 0;
	}

	ret = stm32_ospi_write_enable(&data->hospi, OSPI_SPI_MODE, OSPI_STR_TRANSFER);
	if (ret < 0) {
		return ret;
	}

	reg |= qe_bit;

	ret = stm32_ospi_write_status_register(dev, qe_reg_num, reg);
	if (ret < 0) {
		return ret;
	}

	ret = stm32_ospi_mem_ready(&data->hospi, OSPI_SPI_MODE, OSPI_STR_TRANSFER);
	if (ret < 0) {
		return ret;
	}

	/* validate that QE bit is set */
	ret = stm32_ospi_read_status_register(dev, qe_reg_num, &reg);
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
	struct flash_stm32_ospi_data *data = dev->data;

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
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	struct flash_stm32_ospi_data *data = dev->data;
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
		case OSPI_OPI_MODE:
			data->write_opcode = SPI_NOR_OCMD_PAGE_PRG;
			break;
		case OSPI_QUAD_MODE:
			data->write_opcode = SPI_NOR_CMD_PP_1_4_4;
			break;
		case OSPI_DUAL_MODE:
			data->write_opcode = SPI_NOR_CMD_PP_1_1_2;
			break;
		default:
			data->write_opcode = SPI_NOR_CMD_PP;
			break;
		}
	}

	if (dev_cfg->data_mode != OSPI_OPI_MODE) {
		/* determine supported read modes, begin from the slowest */
		data->read_mode = JESD216_MODE_111;
		data->read_opcode = SPI_NOR_CMD_READ;

		if (dev_cfg->data_mode != OSPI_SPI_MODE) {
			if (dev_cfg->data_mode == OSPI_DUAL_MODE) {
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
				data->read_dummy_cycles =
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
		if (dev_cfg->data_mode == OSPI_QUAD_MODE) {
			if (jesd216_bfp_decode_dw15(php, bfp, &dw15) < 0) {
				/* will use QER from DTS or default (refer to device data) */
				LOG_WRN("Unable to decode QE requirement [DW15]");
			} else {
				/* bypass DTS QER value */
				data->qer_type = dw15.qer;
			}

			LOG_DBG("QE requirement mode: %x", data->qer_type);

			if (stm32_ospi_enable_qe(dev) < 0) {
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
		data->read_mode, data->read_opcode, data->read_dummy_cycles);
	LOG_DBG("Using write instr: 0x%X", data->write_opcode);

	return 0;
}

static int flash_stm32_ospi_init(const struct device *dev)
{
	const struct flash_stm32_ospi_config *dev_cfg = dev->config;
	struct flash_stm32_ospi_data *dev_data = dev->data;
	uint32_t ahb_clock_freq;
	uint32_t prescaler = 0;
	int ret;

	/* The SPI/DTR is not a valid config of data_mode/data_rate according to the DTS */
	if ((dev_cfg->data_mode != OSPI_OPI_MODE)
		&& (dev_cfg->data_rate == OSPI_DTR_TRANSFER)) {
		/* already the right config, continue */
		LOG_ERR("OSPI mode SPI|DUAL|QUAD/DTR is not valid");
		return -ENOTSUP;
	}

	/* Signals configuration */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("OSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

#if STM32_OSPI_USE_DMA
	/*
	 * DMA configuration
	 * Due to use of OSPI HAL API in current driver,
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
	/* Because of the STREAM OFFSET, the DMA channel given here is from 1 - 8 */
	ret = dma_config(dev_data->dma.dev,
			 (dev_data->dma.channel + STM32_DMA_STREAM_OFFSET), &dma_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure DMA channel %d",
			dev_data->dma.channel + STM32_DMA_STREAM_OFFSET);
		return ret;
	}

	/* Proceed to the HAL DMA driver init */
	if (dma_cfg.source_data_size != dma_cfg.dest_data_size) {
		LOG_ERR("Source and destination data sizes not aligned");
		return -EINVAL;
	}

	int index = find_lsb_set(dma_cfg.source_data_size) - 1;

#if CONFIG_DMA_STM32U5
	/* Fill the structure for dma init */
	hdma.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
	hdma.Init.SrcInc = DMA_SINC_FIXED;
	hdma.Init.DestInc = DMA_DINC_INCREMENTED;
	hdma.Init.SrcDataWidth = table_src_size[index];
	hdma.Init.DestDataWidth = table_dest_size[index];
	hdma.Init.SrcBurstLength = 4;
	hdma.Init.DestBurstLength = 4;
	hdma.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
	hdma.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
#else
	hdma.Init.PeriphDataAlignment = table_p_size[index];
	hdma.Init.MemDataAlignment = table_m_size[index];
	hdma.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma.Init.MemInc = DMA_MINC_ENABLE;
#endif /* CONFIG_DMA_STM32U5 */
	hdma.Init.Mode = DMA_NORMAL;
	hdma.Init.Priority = table_priority[dma_cfg.channel_priority];
	hdma.Init.Direction = DMA_PERIPH_TO_MEMORY;
#ifdef CONFIG_DMA_STM32_V1
	/* TODO: Not tested in this configuration */
	hdma.Init.Channel = dma_cfg.dma_slot;
	hdma.Instance = __LL_DMA_GET_STREAM_INSTANCE(dev_data->dma.reg,
						     dev_data->dma.channel);
#else
	hdma.Init.Request = dma_cfg.dma_slot;
#if CONFIG_DMA_STM32U5
	hdma.Instance = LL_DMA_GET_CHANNEL_INSTANCE(dev_data->dma.reg,
						      dev_data->dma.channel);
#elif defined(CONFIG_DMAMUX_STM32)
	/*
	 * HAL expects a valid DMA channel (not DMAMUX).
	 * The channel is from 0 to 7 because of the STM32_DMA_STREAM_OFFSET in the dma_stm32 driver
	 */
	hdma.Instance = __LL_DMA_GET_CHANNEL_INSTANCE(dev_data->dma.reg,
						      dev_data->dma.channel);
#else
	hdma.Instance = __LL_DMA_GET_CHANNEL_INSTANCE(dev_data->dma.reg,
						      dev_data->dma.channel-1);
#endif /* CONFIG_DMA_STM32U5 */
#endif /* CONFIG_DMA_STM32_V1 */

	/* Initialize DMA HAL */
	__HAL_LINKDMA(&dev_data->hospi, hdma, hdma);
	if (HAL_DMA_Init(&hdma) != HAL_OK) {
		LOG_ERR("OSPI DMA Init failed");
		return -EIO;
	}
	LOG_INF("OSPI with DMA transfer");

#endif /* STM32_OSPI_USE_DMA */

	/* Clock configuration */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t) &dev_cfg->pclken) != 0) {
		LOG_ERR("Could not enable OSPI clock");
		return -EIO;
	}
	/* Alternate clock config for peripheral if any */
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_ker)
	if (clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &dev_cfg->pclken_ker,
				NULL) != 0) {
		LOG_ERR("Could not select OSPI domain clock");
		return -EIO;
	}
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &dev_cfg->pclken_ker,
					&ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken_ker)");
		return -EIO;
	}
#else
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &dev_cfg->pclken,
					&ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken)");
		return -EIO;
	}
#endif
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_mgr)
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t) &dev_cfg->pclken_mgr) != 0) {
		LOG_ERR("Could not enable OSPI Manager clock");
		return -EIO;
	}
#endif

	for (; prescaler <= STM32_OSPI_CLOCK_PRESCALER_MAX; prescaler++) {
		uint32_t clk = ahb_clock_freq / (prescaler + 1);

		if (clk <= dev_cfg->max_frequency) {
			break;
		}
	}
	__ASSERT_NO_MSG(prescaler <= STM32_OSPI_CLOCK_PRESCALER_MAX);

	/* Initialize OSPI HAL structure completely */
	dev_data->hospi.Init.FifoThreshold = 4;
	dev_data->hospi.Init.ClockPrescaler = prescaler;
#if defined(CONFIG_SOC_SERIES_STM32H5X)
	/* The stm32h5xx_hal_xspi does not reduce DEVSIZE before writing the DCR1 */
	dev_data->hospi.Init.DeviceSize = find_lsb_set(dev_cfg->flash_size) - 2;
#else
	/* Give a bit position from 0 to 31 to the HAL init for the DCR1 reg */
	dev_data->hospi.Init.DeviceSize = find_lsb_set(dev_cfg->flash_size) - 1;
#endif /* CONFIG_SOC_SERIES_STM32U5X */
	dev_data->hospi.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
	dev_data->hospi.Init.ChipSelectHighTime = 2;
	dev_data->hospi.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
	dev_data->hospi.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
#if defined(OCTOSPI_DCR2_WRAPSIZE)
	dev_data->hospi.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
#endif /* OCTOSPI_DCR2_WRAPSIZE */
	dev_data->hospi.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
	/* STR mode else Macronix for DTR mode */
	if (dev_cfg->data_rate == OSPI_DTR_TRANSFER) {
		dev_data->hospi.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
		dev_data->hospi.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;
	} else {
		dev_data->hospi.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
		dev_data->hospi.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
	}
	dev_data->hospi.Init.ChipSelectBoundary = 0;
#if STM32_OSPI_DLYB_BYPASSED
	dev_data->hospi.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
#else
	dev_data->hospi.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_USED;
#endif /* STM32_OSPI_DLYB_BYPASSED */
#if defined(OCTOSPI_DCR4_REFRESH)
	dev_data->hospi.Init.Refresh = 0;
#endif /* OCTOSPI_DCR4_REFRESH */

	if (HAL_OSPI_Init(&dev_data->hospi) != HAL_OK) {
		LOG_ERR("OSPI Init failed");
		return -EIO;
	}

	LOG_DBG("OSPI Init'd");

#if defined(OCTOSPIM)
	/* OCTOSPI I/O manager init Function */
	OSPIM_CfgTypeDef ospi_mgr_cfg = {0};

	if (dev_data->hospi.Instance == OCTOSPI1) {
		ospi_mgr_cfg.ClkPort = 1;
		ospi_mgr_cfg.DQSPort = 1;
		ospi_mgr_cfg.NCSPort = 1;
		ospi_mgr_cfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
		ospi_mgr_cfg.IOHighPort = HAL_OSPIM_IOPORT_1_HIGH;
	} else if (dev_data->hospi.Instance == OCTOSPI2) {
		ospi_mgr_cfg.ClkPort = 2;
		ospi_mgr_cfg.DQSPort = 2;
		ospi_mgr_cfg.NCSPort = 2;
		ospi_mgr_cfg.IOLowPort = HAL_OSPIM_IOPORT_2_LOW;
		ospi_mgr_cfg.IOHighPort = HAL_OSPIM_IOPORT_2_HIGH;
	}
#if defined(OCTOSPIM_CR_MUXEN)
	ospi_mgr_cfg.Req2AckTime = 1;
#endif /* OCTOSPIM_CR_MUXEN */
	if (HAL_OSPIM_Config(&dev_data->hospi, &ospi_mgr_cfg,
		HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI M config failed");
		return -EIO;
	}
#if defined(CONFIG_SOC_SERIES_STM32U5X)
	/* OCTOSPI2 delay block init Function */
	HAL_OSPI_DLYB_CfgTypeDef ospi_delay_block_cfg = {0};

	ospi_delay_block_cfg.Units = 56;
	ospi_delay_block_cfg.PhaseSel = 2;
	if (HAL_OSPI_DLYB_SetConfig(&dev_data->hospi, &ospi_delay_block_cfg) != HAL_OK) {
		LOG_ERR("OSPI DelayBlock failed");
		return -EIO;
	}
#endif /* CONFIG_SOC_SERIES_STM32U5X */

#endif /* OCTOSPIM */

#if defined(CONFIG_SOC_SERIES_STM32H5X)
	/* OCTOSPI1 delay block init Function */
	HAL_XSPI_DLYB_CfgTypeDef xspi_delay_block_cfg = {0};

	(void)HAL_XSPI_DLYB_GetClockPeriod(&dev_data->hospi, &xspi_delay_block_cfg);
	/*  with DTR, set the PhaseSel/4 (empiric value from stm32Cube) */
	xspi_delay_block_cfg.PhaseSel /= 4;

	if (HAL_XSPI_DLYB_SetConfig(&dev_data->hospi, &xspi_delay_block_cfg) != HAL_OK) {
		LOG_ERR("XSPI DelayBlock failed");
		return -EIO;
	}

	LOG_DBG("Delay Block Init");

#endif /* CONFIG_SOC_SERIES_STM32H5X */

	/* Reset NOR flash memory : still with the SPI/STR config for the NOR */
	if (stm32_ospi_mem_reset(dev) != 0) {
		LOG_ERR("OSPI reset failed");
		return -EIO;
	}

	LOG_DBG("Reset Mem (SPI/STR)");

	/* Check if memory is ready in the SPI/STR mode */
	if (stm32_ospi_mem_ready(&dev_data->hospi,
		OSPI_SPI_MODE, OSPI_STR_TRANSFER) != 0) {
		LOG_ERR("OSPI memory not ready");
		return -EIO;
	}

	LOG_DBG("Mem Ready (SPI/STR)");

#if defined(CONFIG_FLASH_JESD216_API)
	/* Process with the RDID (jedec read ID) instruction at init and fill jedec_id Table */
	ret = stm32_ospi_read_jedec_id(dev);
	if (ret != 0) {
		LOG_ERR("Read ID failed: %d", ret);
		return ret;
	}
#endif /* CONFIG_FLASH_JESD216_API */

	if (stm32_ospi_config_mem(dev) != 0) {
		LOG_ERR("OSPI mode not config'd (%u rate %u)",
			dev_cfg->data_mode, dev_cfg->data_rate);
		return -EIO;
	}

	/* Initialize semaphores */
	k_sem_init(&dev_data->sem, 1, 1);
	k_sem_init(&dev_data->sync, 0, 1);

	/* Run IRQ init */
	dev_cfg->irq_config(dev);

	/* Send the instruction to read the SFDP  */
	const uint8_t decl_nph = 2;
	union {
		/* We only process BFP so use one parameter block */
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u;
	const struct jesd216_sfdp_header *hp = &u.sfdp;

	ret = ospi_read_sfdp(dev, 0, u.raw, sizeof(u.raw));
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
				uint32_t dw[MIN(php->len_dw, 20)];
				struct jesd216_bfp bfp;
			} u;
			const struct jesd216_bfp *bfp = &u.bfp;

			ret = ospi_read_sfdp(dev, jesd216_param_addr(php),
					     (uint8_t *)u.dw, sizeof(u.dw));
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

	return 0;
}

#if STM32_OSPI_USE_DMA
#define DMA_CHANNEL_CONFIG(node, dir)					\
		DT_DMAS_CELL_BY_NAME(node, dir, channel_config)

#define OSPI_DMA_CHANNEL_INIT(node, dir)				\
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
		.dma_callback = ospi_dma_callback,			\
	},								\

#define OSPI_DMA_CHANNEL(node, dir)					\
	.dma = {							\
		COND_CODE_1(DT_DMAS_HAS_NAME(node, dir),		\
			(OSPI_DMA_CHANNEL_INIT(node, dir)),		\
			(NULL))						\
		},

#else
#define OSPI_DMA_CHANNEL(node, dir)
#endif /* CONFIG_USE_STM32_HAL_DMA */

#define OSPI_FLASH_MODULE(drv_id, flash_id)				\
		(DT_DRV_INST(drv_id), ospi_nor_flash_##flash_id)

#define DT_WRITEOC_PROP_OR(inst, default_value)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, writeoc),					\
		    (_CONCAT(SPI_NOR_CMD_, DT_STRING_TOKEN(DT_DRV_INST(inst), writeoc))),	\
		    ((default_value)))

#define DT_QER_PROP_OR(inst, default_value)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, quad_enable_requirements),			\
		    (_CONCAT(JESD216_DW15_QER_VAL_,						\
			     DT_STRING_TOKEN(DT_DRV_INST(inst), quad_enable_requirements))),	\
		    ((default_value)))

static void flash_stm32_ospi_irq_config_func(const struct device *dev);

PINCTRL_DT_DEFINE(STM32_OSPI_NODE);

static const struct flash_stm32_ospi_config flash_stm32_ospi_cfg = {
	.regs = (OCTOSPI_TypeDef *)DT_REG_ADDR(STM32_OSPI_NODE),
	.pclken = {.bus = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospix, bus),
		   .enr = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospix, bits)},
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_ker)
	.pclken_ker = {.bus = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_ker, bus),
		       .enr = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_ker, bits)},
#endif
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_mgr)
	.pclken_mgr = {.bus = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_mgr, bus),
		       .enr = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_mgr, bits)},
#endif
	.irq_config = flash_stm32_ospi_irq_config_func,
	.flash_size = DT_INST_PROP(0, size) / 8U,
	.max_frequency = DT_INST_PROP(0, ospi_max_frequency),
	.data_mode = DT_INST_PROP(0, spi_bus_width), /* SPI or OPI */
	.data_rate = DT_INST_PROP(0, data_rate), /* DTR or STR */
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(STM32_OSPI_NODE),
#if STM32_OSPI_RESET_GPIO
	.reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
#endif /* STM32_OSPI_RESET_GPIO */
#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_ospi_nor), sfdp_bfp)
	.sfdp_bfp = DT_INST_PROP(0, sfdp_bfp),
#endif /* sfdp_bfp */
};

static struct flash_stm32_ospi_data flash_stm32_ospi_dev_data = {
	.hospi = {
		.Instance = (OCTOSPI_TypeDef *)DT_REG_ADDR(STM32_OSPI_NODE),
		.Init = {
			.FifoThreshold = STM32_OSPI_FIFO_THRESHOLD,
			.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE,
			.ChipSelectHighTime = 1,
			.ClockMode = HAL_OSPI_CLOCK_MODE_0,
			},
	},
	.read_dummy_cycles = 0U,
	.qer_type = DT_QER_PROP_OR(0, JESD216_DW15_QER_VAL_S1B6),
	.write_opcode = DT_WRITEOC_PROP_OR(0, SPI_NOR_WRITEOC_NONE),
	.page_size = SPI_NOR_PAGE_SIZE, /* by default, to be updated by sfdp */
#if DT_NODE_HAS_PROP(DT_INST(0, st_stm32_ospi_nor), jedec_id)
	.jedec_id = DT_INST_PROP(0, jedec_id),
#endif /* jedec_id */
	OSPI_DMA_CHANNEL(STM32_OSPI_NODE, tx_rx)
};

DEVICE_DT_INST_DEFINE(0, &flash_stm32_ospi_init, NULL,
		      &flash_stm32_ospi_dev_data, &flash_stm32_ospi_cfg,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &flash_stm32_ospi_driver_api);

static void flash_stm32_ospi_irq_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(STM32_OSPI_NODE), DT_IRQ(STM32_OSPI_NODE, priority),
		    flash_stm32_ospi_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_IRQN(STM32_OSPI_NODE));
}
