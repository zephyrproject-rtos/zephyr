/*
 * Copyright (c) 2020 Piotr Mienkowski
 * Copyright (c) 2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_qspi_nor

#include <errno.h>
#include <kernel.h>
#include <toolchain.h>
#include <arch/common/ffs.h>
#include <sys/util.h>
#include <soc.h>
#include <pinmux/stm32/pinmux_stm32.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/clock_control.h>
#include <drivers/flash.h>
#include <dt-bindings/dma/stm32_dma.h>
#include <drivers/dma.h>
#include <drivers/dma/dma_stm32.h>

#include <stm32_ll_dma.h>

#include "spi_nor.h"
#include "jesd216.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(flash_stm32_qspi, CONFIG_FLASH_LOG_LEVEL);

#define STM32_QSPI_FIFO_THRESHOLD         8
#define STM32_QSPI_CLOCK_PRESCALER_MAX  255

#define STM32_QSPI_USE_DMA DT_NODE_HAS_PROP(DT_PARENT(DT_DRV_INST(0)), dmas)

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_qspi_nor)

uint32_t table_m_size[] = {
	LL_DMA_MDATAALIGN_BYTE,
	LL_DMA_MDATAALIGN_HALFWORD,
	LL_DMA_MDATAALIGN_WORD,
};

uint32_t table_p_size[] = {
	LL_DMA_PDATAALIGN_BYTE,
	LL_DMA_PDATAALIGN_HALFWORD,
	LL_DMA_PDATAALIGN_WORD,
};

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
	const struct soc_gpio_pinctrl *pinctrl_list;
	size_t pinctrl_list_size;
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
	bool write_protection;
	int cmd_status;
	struct stream dma;
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_CFG(dev) \
	(const struct flash_stm32_qspi_config * const)(dev->config)
#define DEV_DATA(dev) \
	(struct flash_stm32_qspi_data * const)(dev->data)

static inline void qspi_lock_thread(const struct device *dev)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);

	k_sem_take(&dev_data->sem, K_FOREVER);
}

static inline void qspi_unlock_thread(const struct device *dev)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);

	k_sem_give(&dev_data->sem);
}

/*
 * Send a command over QSPI bus.
 */
static int qspi_send_cmd(const struct device *dev, QSPI_CommandTypeDef *cmd)
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
static int qspi_read_access(const struct device *dev, QSPI_CommandTypeDef *cmd,
			    uint8_t *data, size_t size)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);
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

/*
 * Read Serial Flash Discovery Parameter
 */
static int qspi_read_sfdp(const struct device *dev, off_t addr, uint8_t *data,
			  size_t size)
{
	QSPI_CommandTypeDef cmd = {
		.Instruction = JESD216_CMD_READ_SFDP,
		.Address = addr,
		.AddressSize = QSPI_ADDRESS_24_BITS,
		.DummyCycles = 8,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
		.DataMode = QSPI_DATA_1_LINE,
	};

	return qspi_read_access(dev, &cmd, data, size);
}

static bool qspi_address_is_valid(const struct device *dev, off_t addr,
				  size_t size)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	size_t flash_size = dev_cfg->flash_size;

	return (addr >= 0) && ((uint64_t)addr + (uint64_t)size <= flash_size);
}

static int flash_stm32_qspi_read(const struct device *dev, off_t addr,
				 void *data, size_t size)
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

	qspi_unlock_thread(dev);

	return ret;
}

static int flash_stm32_qspi_erase(const struct device *dev, off_t addr,
				  size_t size)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
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

	QSPI_CommandTypeDef cmd_erase = {
		.Instruction = 0,
		.AddressSize = QSPI_ADDRESS_24_BITS,
		.InstructionMode = QSPI_INSTRUCTION_1_LINE,
		.AddressMode = QSPI_ADDRESS_1_LINE,
	};

	qspi_lock_thread(dev);

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

	qspi_unlock_thread(dev);

	return ret;
}

static int flash_stm32_qspi_write_protection_set(const struct device *dev,
						 bool write_protect)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);

	dev_data->write_protection = write_protect;

	return 0;
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

static void flash_stm32_qspi_isr(const struct device *dev)
{
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);

	HAL_QSPI_IRQHandler(&dev_data->hqspi);
}

/* This function is executed in the interrupt context */
#if STM32_QSPI_USE_DMA
static void qspi_dma_callback(const struct device *dev, void *arg,
			 uint32_t channel, int status)
{
	DMA_HandleTypeDef *hdma = arg;

	if (status != 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);

	}

	HAL_DMA_IRQHandler(hdma);
}
#endif

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
	.get_parameters = flash_stm32_qspi_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_stm32_qspi_pages_layout,
#endif
};

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static int setup_pages_layout(const struct device *dev)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	struct flash_stm32_qspi_data *data = DEV_DATA(dev);
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

	uint32_t erase_size = BIT(exp);

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

static int spi_nor_process_bfp(const struct device *dev,
			       const struct jesd216_param_header *php,
			       const struct jesd216_bfp *bfp)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	struct flash_stm32_qspi_data *data = DEV_DATA(dev);
	struct jesd216_erase_type *etp = data->erase_types;
	const size_t flash_size = jesd216_bfp_density(bfp) / 8U;

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
	return 0;
}

static int flash_stm32_qspi_init(const struct device *dev)
{
	const struct flash_stm32_qspi_config *dev_cfg = DEV_CFG(dev);
	struct flash_stm32_qspi_data *dev_data = DEV_DATA(dev);
	uint32_t ahb_clock_freq;
	uint32_t prescaler = 0;
	int ret;

	/* Signals configuration */
	ret = stm32_dt_pinctrl_configure(dev_cfg->pinctrl_list,
					 dev_cfg->pinctrl_list_size,
					 (uint32_t)dev_cfg->regs);
	if (ret < 0) {
		LOG_ERR("QSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

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
	hdma.Init.Priority = dma_cfg.channel_priority;
#ifdef CONFIG_DMA_STM32_V1
	/* TODO: Not tested in this configuration */
	hdma.Init.Channel = dma_cfg.dma_slot;
	hdma.Instance = __LL_DMA_GET_STREAM_INSTANCE(dev_data->dma.reg,
						     dev_data->dma.channel);
#else
	hdma.Init.Request = dma_cfg.dma_slot;
#ifdef CONFIG_DMAMUX_STM32
	/* HAL expects a valid DMA channel (not DAMMUX) */
	/* TODO: Get DMA instance from DT */
	hdma.Instance = __LL_DMA_GET_CHANNEL_INSTANCE(DMA1,
						      dev_data->dma.channel+1);
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
	dev_data->hqspi.Init.FlashSize = find_lsb_set(dev_cfg->flash_size);

	HAL_QSPI_Init(&dev_data->hqspi);

	/* Initialize semaphores */
	k_sem_init(&dev_data->sem, 1, 1);
	k_sem_init(&dev_data->sync, 0, 1);

	/* Run IRQ init */
	dev_cfg->irq_config(dev);

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
				uint32_t dw[MIN(php->len_dw, 20)];
				struct jesd216_bfp bfp;
			} u;
			const struct jesd216_bfp *bfp = &u.bfp;

			ret = qspi_read_sfdp(dev, jesd216_param_addr(php),
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

	LOG_INF("Device %s initialized", DEV_NAME(dev));

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

static const struct soc_gpio_pinctrl qspi_pins[] =
					ST_STM32_DT_PINCTRL(quadspi, 0);

#define STM32_QSPI_NODE DT_PARENT(DT_DRV_INST(0))

static const struct flash_stm32_qspi_config flash_stm32_qspi_cfg = {
	.regs = (QUADSPI_TypeDef *)DT_REG_ADDR(STM32_QSPI_NODE),
	.pclken = {
		.enr = DT_CLOCKS_CELL(STM32_QSPI_NODE, bits),
		.bus = DT_CLOCKS_CELL(STM32_QSPI_NODE, bus)
	},
	.irq_config = flash_stm32_qspi_irq_config_func,
	.flash_size = DT_INST_PROP(0, size) / 8U,
	.max_frequency = DT_INST_PROP(0, qspi_max_frequency),
	.pinctrl_list = qspi_pins,
	.pinctrl_list_size = ARRAY_SIZE(qspi_pins),
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
	QSPI_DMA_CHANNEL(STM32_QSPI_NODE, tx_rx)
};

DEVICE_DT_INST_DEFINE(0, &flash_stm32_qspi_init, device_pm_control_nop,
		      &flash_stm32_qspi_dev_data, &flash_stm32_qspi_cfg,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &flash_stm32_qspi_driver_api);

static void flash_stm32_qspi_irq_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(STM32_QSPI_NODE), DT_IRQ(STM32_QSPI_NODE, priority),
		    flash_stm32_qspi_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_IRQN(STM32_QSPI_NODE));
}

#endif
