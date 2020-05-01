/*
 * Copyright (c) 2020 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_quadspi

#include <errno.h>
#include <kernel.h>
#include <toolchain.h>
#include <arch/common/ffs.h>
#include <sys/util.h>
#include <soc.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/clock_control.h>
#include <drivers/flash.h>
#include "spi_nor.h"
#include "sfdp.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(flash_stm32_qspi, CONFIG_FLASH_LOG_LEVEL);

#define STM32_QSPI_FIFO_THRESHOLD         8
#define STM32_QSPI_FLASH_SIZE_MIN         2
#define STM32_QSPI_CLOCK_PRESCALER_MAX  255

typedef void (*irq_config_func_t)(struct device *dev);

/* Data from DT child node containing flash parameters */
struct spi_nor_flash_config {
	/* JEDEC Id */
	u8_t jedec_id[SPI_NOR_MAX_ID_LEN];

	/* Max SPI frequency supported by the flash module */
	u32_t spi_max_frequency;
};

struct flash_stm32_qspi_config {
	QUADSPI_TypeDef *regs;
	struct stm32_pclken pclken;
	irq_config_func_t irq_config;
	struct spi_nor_flash_config flash_config;
};

struct flash_params {
	u32_t size;                     /* Flash size in bytes */
	struct sector_layout {
		u8_t size_n;
		u8_t erase_cmd;
	} sector_layout[4];
};

struct flash_stm32_qspi_data {
	QSPI_HandleTypeDef hqspi;
	struct k_sem sem;
	struct k_sem sync;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
	struct flash_params flash_params;
	bool write_protection;
	int cmd_status;
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_CFG(dev) \
	(const struct flash_stm32_qspi_config * const)(dev->config_info)
#define DEV_DATA(dev) \
	(struct flash_stm32_qspi_data * const)(dev->driver_data)

static inline void qspi_lock_thread(struct device *dev)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);

	k_sem_take(&dev_data->sem, K_FOREVER);
}

static inline void qspi_unlock_thread(struct device *dev)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);

	k_sem_give(&dev_data->sem);
}

/*
 * Send a command over QSPI bus.
 */
static int qspi_send_cmd(struct device *dev, QSPI_CommandTypeDef *cmd)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);
	HAL_StatusTypeDef hal_ret;

	ARG_UNUSED(dev_cfg);

	LOG_DBG("Instruction 0x%x", cmd->Instruction);

	dev_data->cmd_status = 0;

	hal_ret = HAL_QSPI_Command_IT(&dev_data->hqspi, cmd);
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
static int qspi_read_access(struct device *dev, QSPI_CommandTypeDef *cmd,
			    u8_t *data, size_t size)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);
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

	hal_ret = HAL_QSPI_Receive_IT(&dev_data->hqspi, data);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}
	LOG_DBG("CCR 0x%x", dev_cfg->regs->CCR);

	k_sem_take(&dev_data->sync, K_FOREVER);

	return dev_data->cmd_status;
}

/*
 * Perform a write access over QSPI bus.
 */
static int qspi_write_access(struct device *dev, QSPI_CommandTypeDef *cmd,
			     const u8_t *data, size_t size)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);
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

	hal_ret = HAL_QSPI_Transmit_IT(&dev_data->hqspi, (uint8_t *)data);
	if (hal_ret != HAL_OK) {
		LOG_ERR("%d: Failed to read data", hal_ret);
		return -EIO;
	}
	LOG_DBG("CCR 0x%x", dev_cfg->regs->CCR);

	k_sem_take(&dev_data->sync, K_FOREVER);

	return dev_data->cmd_status;
}

/*
 * Retrieve Flash JEDEC ID and compare it with the one expected
 */
static int qspi_read_id(struct device *dev)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	const struct spi_nor_flash_config *flash_cfg = &dev_cfg->flash_config;
	u8_t rx_buf[SPI_NOR_MAX_ID_LEN];

	QSPI_CommandTypeDef cmd = {
		.Instruction = SPI_NOR_CMD_RDID,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	qspi_read_access(dev, &cmd, rx_buf, sizeof(rx_buf));

	if (memcmp(flash_cfg->jedec_id, rx_buf, sizeof(rx_buf)) != 0) {
		LOG_ERR("Invalid JEDEC ID: expected [%x %x %x], got [%x %x %x]",
			flash_cfg->jedec_id[0], flash_cfg->jedec_id[1],
			flash_cfg->jedec_id[2], rx_buf[0], rx_buf[1],
			rx_buf[2]);
		return -ENODEV;
	}

	return 0;
}

/*
 * Read Serial Flash Discovery Parameter
 */
static int qspi_read_sfdp(struct device *dev, off_t addr, u8_t *data,
			  size_t size)
{
	QSPI_CommandTypeDef cmd = {
		.Instruction = SPI_NOR_CMD_RSFDP,
		.Address = addr,
		.AddressSize = QSPI_ADDRESS_24_BITS,
		.DummyCycles = 8,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	return qspi_read_access(dev, &cmd, data, size);
}

static int qspi_process_jedec_flash_parameter_table(struct device *dev,
		off_t addr, unsigned int word_len)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);
	union sfdp_dword data[word_len];
	int ret;

	ret = qspi_read_sfdp(dev, addr, (u8_t *)data, sizeof(data));
	if (ret != 0) {
		return ret;
	}

	/*
	 * Process 2nd DWORD
	 */
	bool is_gt_2_gbits = sfdp_pt_1v0_dw2_is_gt_2_gigabits(data[1]);
	u32_t bit_size_n = sfdp_pt_1v0_dw2_get_density_n(data[1]);

	if (is_gt_2_gbits) {
		/* Convert bit size_n to byte size_n */
		u32_t size_n = bit_size_n - 3;
		/* Only 31 bit addressing is supported by the current flash API.
		 * Maximum supported flash size is 2^31 bytes.
		 */
		dev_data->flash_params.size = (size_n < 31) ? (1 << size_n) : (1 << 31);
	} else {
		dev_data->flash_params.size = (bit_size_n >> 3) + 1;
	}

	/*
	 * Process 8th DWORD
	 */
	dev_data->flash_params.sector_layout[0].size_n =
		sfdp_pt_1v0_dw8_get_sector_type_1_size_n(data[7]);
	dev_data->flash_params.sector_layout[0].erase_cmd =
		sfdp_pt_1v0_dw8_get_sector_type_1_erase_opcode(data[7]);
	dev_data->flash_params.sector_layout[1].size_n =
		sfdp_pt_1v0_dw8_get_sector_type_2_size_n(data[7]);
	dev_data->flash_params.sector_layout[1].erase_cmd =
		sfdp_pt_1v0_dw8_get_sector_type_2_erase_opcode(data[7]);

	/*
	 * Process 9th DWORD
	 */
	dev_data->flash_params.sector_layout[2].size_n =
		sfdp_pt_1v0_dw9_get_sector_type_3_size_n(data[8]);
	dev_data->flash_params.sector_layout[2].erase_cmd =
		sfdp_pt_1v0_dw9_get_sector_type_3_erase_opcode(data[8]);
	dev_data->flash_params.sector_layout[3].size_n =
		sfdp_pt_1v0_dw9_get_sector_type_4_size_n(data[8]);
	dev_data->flash_params.sector_layout[3].erase_cmd =
		sfdp_pt_1v0_dw9_get_sector_type_4_erase_opcode(data[8]);

	return ret;
}

static bool qspi_address_is_valid(struct device *dev, off_t addr, size_t size)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);
	struct flash_params *flash_params = &dev_data->flash_params;

	return (addr >= 0) && ((u64_t)addr + (u64_t)size <= flash_params->size);
}

static int flash_stm32_qspi_read(struct device *dev, off_t addr, void *data,
				 size_t size)
{
	int ret;

	if (!qspi_address_is_valid(dev, addr, size)) {
		LOG_DBG("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	QSPI_CommandTypeDef cmd = {
		.Instruction = SPI_NOR_CMD_READ,
		.Address = addr,
		.AddressSize = QSPI_ADDRESS_24_BITS,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	qspi_lock_thread(dev);

	ret = qspi_read_access(dev, &cmd, data, size);

	qspi_unlock_thread(dev);

	return ret;
}

static int qspi_wait_until_ready(struct device *dev)
{
	u8_t reg;
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

static int flash_stm32_qspi_write(struct device *dev, off_t addr,
				  const void *data, size_t size)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);
	int ret = 0;

	if (dev_data->write_protection) {
		return -EACCES;
	}

	if (!qspi_address_is_valid(dev, addr, size)) {
		LOG_DBG("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	QSPI_CommandTypeDef cmd_write_en = {
		.Instruction = SPI_NOR_CMD_WREN,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
	};

	QSPI_CommandTypeDef cmd_pp = {
		.Instruction = SPI_NOR_CMD_PP,
		.AddressSize = QSPI_ADDRESS_24_BITS,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	qspi_lock_thread(dev);

	while (size > 0) {
		size_t to_write = size;

		/* Don't write more than a page. */
		if (to_write >= SPI_NOR_PAGE_SIZE) {
			to_write = SPI_NOR_PAGE_SIZE;
		}

		/* Don't write across a page boundary */
		if (((addr + to_write - 1U) / SPI_NOR_PAGE_SIZE)
		    != (addr / SPI_NOR_PAGE_SIZE)) {
			to_write = SPI_NOR_PAGE_SIZE - (addr % SPI_NOR_PAGE_SIZE);
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
		data = (const u8_t *)data + to_write;
		addr += to_write;

		ret = qspi_wait_until_ready(dev);
		if (ret != 0) {
			break;
		}
	}

	qspi_unlock_thread(dev);

	return ret;
}

static int flash_stm32_qspi_erase(struct device *dev, off_t addr, size_t size)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);
	struct flash_params *flash_params = &dev_data->flash_params;
	int ret = 0;

	if (dev_data->write_protection) {
		return -EACCES;
	}

	if (!qspi_address_is_valid(dev, addr, size)) {
		LOG_DBG("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu", (long)addr, size);
		return -EINVAL;
	}

	QSPI_CommandTypeDef cmd_write_en = {
		.Instruction = SPI_NOR_CMD_WREN,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
	};

	QSPI_CommandTypeDef cmd_erase = {
		.Instruction = 0,
		.AddressSize = QSPI_ADDRESS_24_BITS,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
	};

	qspi_lock_thread(dev);

	while (size) {
		cmd_erase.Address = addr;

		if (size == flash_params->size) {
			/* Chip erase */
			cmd_erase.Instruction = SPI_NOR_CMD_CE;
			cmd_erase.AddressMode = QSPI_ADDRESS_NONE;
			size -= flash_params->size;
		} else {
			for (int i = ARRAY_SIZE(flash_params->sector_layout) - 1; i >= 0; i--) {
				u32_t block_size = 1 << flash_params->sector_layout[i].size_n;

				if ((block_size > 1) && (size >= block_size)
				    && SPI_NOR_IS_ADDR_ALIGNED(addr, block_size)) {
					addr += block_size;
					size -= block_size;
					cmd_erase.Instruction = flash_params->sector_layout[i].erase_cmd;
					break;
				}
			}
		}

		if (cmd_erase.Instruction == 0) {
			LOG_DBG("unsupported at 0x%lx size %zu", (long)addr,
				size);
			ret = -EINVAL;
			break;
		}

		ret = qspi_send_cmd(dev, &cmd_write_en);
		if (ret != 0) {
			break;
		}

		ret = qspi_send_cmd(dev, &cmd_erase);
		if (ret != 0) {
			break;
		}

		ret = qspi_wait_until_ready(dev);
		if (ret != 0) {
			break;
		}
	}

	qspi_unlock_thread(dev);

	return ret;
}

static int flash_stm32_qspi_write_protection_set(struct device *dev,
						 bool write_protect)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);

	dev_data->write_protection = write_protect;

	return 0;
}

static void flash_stm32_qspi_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);

	LOG_DBG("SR 0x%x", dev_cfg->regs->SR);

	HAL_QSPI_IRQHandler(&dev_data->hqspi);
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

	LOG_DBG("Enter");

	k_sem_give(&dev_data->sync);
}

/*
 * Rx Transfer completed callback.
 */
void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
	struct flash_stm32_qspi_data *dev_data =
		CONTAINER_OF(hqspi, struct flash_stm32_qspi_data, hqspi);

	LOG_DBG("Enter");

	k_sem_give(&dev_data->sync);
}

/*
 * Tx Transfer completed callback.
 */
void HAL_QSPI_TxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
	struct flash_stm32_qspi_data *dev_data =
		CONTAINER_OF(hqspi, struct flash_stm32_qspi_data, hqspi);

	LOG_DBG("Enter");

	k_sem_give(&dev_data->sync);
}

/*
 * Status Match callback.
 */
void HAL_QSPI_StatusMatchCallback(QSPI_HandleTypeDef *hqspi)
{
	struct flash_stm32_qspi_data *dev_data =
		CONTAINER_OF(hqspi, struct flash_stm32_qspi_data, hqspi);

	LOG_DBG("Enter");

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
static void flash_stm32_qspi_pages_layout(struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);

	*layout = &dev_data->layout;
	*layout_size = 1;
}
#endif

static const struct flash_driver_api flash_stm32_qspi_driver_api = {
	.read = flash_stm32_qspi_read,
	.write = flash_stm32_qspi_write,
	.erase = flash_stm32_qspi_erase,
	.write_protection = flash_stm32_qspi_write_protection_set,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_stm32_qspi_pages_layout,
#endif
	.write_block_size = 1,
};

static int flash_stm32_qspi_init(struct device *dev)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);
	u32_t ahb_clock_freq;
	u32_t prescaler = 0;
	int ret;

	__ASSERT_NO_MSG(device_get_binding(STM32_CLOCK_CONTROL_NAME));

	if (clock_control_on(device_get_binding(STM32_CLOCK_CONTROL_NAME),
			     (clock_control_subsys_t) &dev_cfg->pclken) != 0) {
		LOG_DBG("Could not enable QSPI clock");
		return -EIO;
	}

	if (clock_control_get_rate(device_get_binding(STM32_CLOCK_CONTROL_NAME),
			(clock_control_subsys_t) &dev_cfg->pclken,
			&ahb_clock_freq) < 0) {
		LOG_DBG("Failed to get_AHB clock frequency");
		return -EIO;
	}

	LOG_DBG("AHB clock running at %u Hz", ahb_clock_freq / (prescaler + 1));

	for (; prescaler <= STM32_QSPI_CLOCK_PRESCALER_MAX; prescaler++) {
		u32_t clk = ahb_clock_freq / (prescaler + 1);

		if (clk <= SPI_NOR_SFDP_READ_CLOCK_FREQUENCY) {
			break;
		}
	}
	__ASSERT_NO_MSG(prescaler <= STM32_QSPI_CLOCK_PRESCALER_MAX);

	dev_data->hqspi.Init.ClockPrescaler = prescaler;

	HAL_QSPI_Init(&dev_data->hqspi);

	LOG_DBG("QSPI clock set to %u Hz", ahb_clock_freq / (prescaler + 1));
	LOG_DBG("CR 0x%x", dev_cfg->regs->CR);
	LOG_DBG("DCR 0x%x", dev_cfg->regs->DCR);

	k_sem_init(&dev_data->sem, 1, 1);
	k_sem_init(&dev_data->sync, 0, 1);

	dev_cfg->irq_config(dev);

	ret = qspi_read_id(dev);
	if (ret != 0) {
		return ret;
	}

	union sfdp_header sfdp_header[2];
	u32_t sfdp_signature;
	off_t jedec_pt_addr;
	u8_t jedec_pt_len;
	u8_t header_id;

	qspi_read_sfdp(dev, SFDP_HEADER_ADDRESS, (u8_t *)sfdp_header,
			sizeof(sfdp_header));

	LOG_HEXDUMP_DBG(sfdp_header, sizeof(sfdp_header), "SFDP");

	sfdp_signature = sfdp_get_header_signature(&sfdp_header[0]);
	if (sfdp_signature != SFDP_SIGNATURE) {
		LOG_ERR("Invalid SFDP signature: expected 0x%x, received 0x%x",
			SFDP_SIGNATURE, sfdp_signature);
		return -EIO;
	}

	header_id = sfdp_get_param_header_id(&sfdp_header[1]);
	if (header_id != SFDP_HEADER_JEDEC_ID) {
		LOG_ERR("Invalid JEDEC header id: expected 0x%x, received 0x%x",
			SFDP_HEADER_JEDEC_ID, header_id);
		return -EIO;
	}

	jedec_pt_addr = sfdp_get_param_header_pt_pointer(&sfdp_header[1]);
	jedec_pt_len = sfdp_get_param_header_pt_length(&sfdp_header[1]);

	ret = qspi_process_jedec_flash_parameter_table(dev, jedec_pt_addr, jedec_pt_len);
	if (ret != 0) {
		LOG_ERR("Failed to read JEDEC flash parameter table");
		return ret;
	}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	dev_data->layout.pages_size = 1 << dev_data->flash_params.sector_layout[0].size_n;
	dev_data->layout.pages_count = dev_data->flash_params.size / dev_data->layout.pages_size;
#endif

	LOG_INF("Detected flash size %u bytes", dev_data->flash_params.size);
	LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

#define QSPI_FLASH_MODULE(drv_id, flash_id) \
	DT_CHILD(DT_DRV_INST(drv_id), qspi_nor_flash_##flash_id)

#define STM32_QSPI_INIT(id)						\
static void flash_stm32_qspi_irq_config_func_##id(struct device *dev);	\
									\
static const struct flash_stm32_qspi_config flash_stm32_qspi_cfg_##id = { \
	.regs = (QUADSPI_TypeDef *)DT_INST_REG_ADDR(id),		\
	.pclken = {							\
		.enr = DT_INST_CLOCKS_CELL(id, bits),			\
		.bus = DT_INST_CLOCKS_CELL(id, bus)			\
	},								\
	.irq_config = flash_stm32_qspi_irq_config_func_##id,		\
	.flash_config = {						\
		.jedec_id = DT_PROP(QSPI_FLASH_MODULE(id, 0), jedec_id), \
		.spi_max_frequency = DT_PROP(QSPI_FLASH_MODULE(id, 0), spi_max_frequency), \
	},								\
};									\
									\
static struct flash_stm32_qspi_data flash_stm32_qspi_dev_data_##id = {	\
	.hqspi = {							\
		.Instance = (QUADSPI_TypeDef *)DT_INST_REG_ADDR(id),	\
		.Init = {						\
			.FifoThreshold = STM32_QSPI_FIFO_THRESHOLD,	\
			.FlashSize = 31,				\
			.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE,	\
			.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE,\
			.ClockMode = QSPI_CLOCK_MODE_0,			\
		},							\
	},								\
};									\
									\
DEVICE_AND_API_INIT(flash_stm32_qspi_##id, DT_INST_LABEL(id),		\
		    &flash_stm32_qspi_init,				\
		    &flash_stm32_qspi_dev_data_##id,			\
		    &flash_stm32_qspi_cfg_##id,				\
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &flash_stm32_qspi_driver_api);			\
									\
static void flash_stm32_qspi_irq_config_func_##id(struct device *dev)	\
{									\
	IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority),	\
		    flash_stm32_qspi_isr,				\
		    DEVICE_GET(flash_stm32_qspi_##id), 0);		\
	irq_enable(DT_INST_IRQN(id));					\
}

DT_INST_FOREACH_STATUS_OKAY(STM32_QSPI_INIT)
