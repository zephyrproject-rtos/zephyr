/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT     realtek_rts5912_flash_controller
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#define FLASH_PAGE_SZ      256
#define FLASH_WRITE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_ERASE_BLK_SZ DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_rts5912);

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#ifdef CONFIG_FLASH_EX_OP_ENABLED
#include <zephyr/drivers/flash/rts5912_flash_api_ex.h>
#endif
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <string.h>

#include "spi_nor.h"
#include "reg/reg_spic.h"

#define FLASH_CMD_RDSFDP         0x5A /* Read SFDP */
#define FLASH_CMD_EX4B           0xE9 /* Exit 4-byte mode */
#define FLASH_CMD_EXTNADDR_WREAR 0xC5 /* Write extended address register */
#define FLASH_CMD_EXTNADDR_RDEAR 0xC8 /* Read extended address register */

#define MODE(x)    (((x) << 6) & SPIC_CTRL0_SCPH)
#define TMOD(x)    (((x) << SPIC_CTRL0_TMOD_Pos) & SPIC_CTRL0_TMOD_Msk)
#define CMD_CH(x)  (((x) << SPIC_CTRL0_CMDCH_Pos) & SPIC_CTRL0_CMDCH_Msk)
#define ADDR_CH(x) (((x) << SPIC_CTRL0_ADDRCH_Pos) & SPIC_CTRL0_ADDRCH_Msk)
#define DATA_CH(x) (((x) << SPIC_CTRL0_DATACH_Pos) & SPIC_CTRL0_DATACH_Msk)

#define USER_CMD_LENGTH(x)  (((x) << SPIC_USERLENGTH_CMDLEN_Pos) & SPIC_USERLENGTH_CMDLEN_Msk)
#define USER_ADDR_LENGTH(x) (((x) << SPIC_USERLENGTH_ADDRLEN_Pos) & SPIC_USERLENGTH_ADDRLEN_Msk)
#define USER_RD_DUMMY_LENGTH(x)                                                                    \
	(((x) << SPIC_USERLENGTH_RDDUMMYLEN_Pos) & SPIC_USERLENGTH_RDDUMMYLEN_Msk)

#define TX_NDF(x) (((x) << SPIC_TXNDF_NUM_Pos) & SPIC_TXNDF_NUM_Msk)
#define RX_NDF(x) (((x) << SPIC_RXNDF_NUM_Pos) & SPIC_RXNDF_NUM_Msk)

#define TIMEOUT_SPICEN  10UL
#define TIMEOUT_SPIBUSY 10000UL

enum {
	COMMAND_READ = 0,
	COMMAND_WRITE = 1,
};

enum spic_freq {
	SPIC_FREQ_SYS_CLK_DIV2 = 1,
	SPIC_FREQ_SYS_CLK_DIV4,
	SPIC_FREQ_SYS_CLK_DIV8,
	SPIC_FREQ_SYS_CLK_DIV16,
};

enum spic_bus_width {
	SPIC_CFG_BUS_SINGLE,
	SPIC_CFG_BUS_DUAL,
	SPIC_CFG_BUS_QUAD,
};

enum spic_address_size {
	SPIC_CFG_ADDR_SIZE_8,
	SPIC_CFG_ADDR_SIZE_16,
	SPIC_CFG_ADDR_SIZE_24,
	SPIC_CFG_ADDR_SIZE_32,
};

struct qspi_cmd {
	struct {
		enum spic_bus_width bus_width; /* Bus width for the instruction */
		uint8_t value;                 /* Instruction value */
		uint8_t disabled; /* Instruction phase skipped if disabled is set to true */
	} instruction;
	struct {
		enum spic_bus_width bus_width; /* Bus width for the address */
		enum spic_address_size size;   /* Address size */
		uint32_t value;                /* Address value */
		uint8_t disabled; /* Address phase skipped if disabled is set to true */
	} address;
	struct {
		enum spic_bus_width bus_width; /* Bus width for alternative */
		uint8_t size;                  /* Alternative size */
		uint32_t value;                /* Alternative value */
		uint8_t disabled; /* Alternative phase skipped if disabled is set to true */
	} alt;
	uint8_t dummy_count; /* Dummy cycles count */
	struct {
		enum spic_bus_width bus_width; /* Bus width for data */
	} data;
};

struct flash_rts5912_dev_config {
	volatile struct reg_spic_reg *regs;
	struct flash_parameters flash_rts5912_parameters;
};

struct flash_rts5912_dev_data {
	struct k_sem sem;
	struct qspi_cmd command_default;
};

static const uint8_t user_addr_len[] = {
	[SPIC_CFG_ADDR_SIZE_8] = 1,
	[SPIC_CFG_ADDR_SIZE_16] = 2,
	[SPIC_CFG_ADDR_SIZE_24] = 3,
	[SPIC_CFG_ADDR_SIZE_32] = 4,
};

static int config_command(struct qspi_cmd *command, uint8_t cmd, uint32_t addr,
			  enum spic_address_size addr_size, uint8_t dummy_count)
{
	int ret = 0;

	switch (cmd) {
	case SPI_NOR_CMD_WREN:
	case SPI_NOR_CMD_WRDI:
	case SPI_NOR_CMD_WRSR:
	case SPI_NOR_CMD_RDID:
	case SPI_NOR_CMD_RDSR:
	case SPI_NOR_CMD_RDSR2:
	case SPI_NOR_CMD_CE:
	case SPI_NOR_CMD_4BA:
	case FLASH_CMD_EX4B:
	case FLASH_CMD_EXTNADDR_WREAR:
	case FLASH_CMD_EXTNADDR_RDEAR:
	case SPI_NOR_CMD_RESET_EN:
	case SPI_NOR_CMD_RESET_MEM:
		command->address.disabled = 1;
		command->data.bus_width = SPIC_CFG_BUS_SINGLE;
		break;
	case SPI_NOR_CMD_READ:
	case SPI_NOR_CMD_READ_FAST:
	case SPI_NOR_CMD_SE:
	case SPI_NOR_CMD_BE:
	case FLASH_CMD_RDSFDP:
	case SPI_NOR_CMD_PP:
		command->address.disabled = 0;
		command->address.bus_width = SPIC_CFG_BUS_SINGLE;
		command->data.bus_width = SPIC_CFG_BUS_SINGLE;
		break;
	case SPI_NOR_CMD_DREAD:
		command->address.disabled = 0;
		command->address.bus_width = SPIC_CFG_BUS_SINGLE;
		command->data.bus_width = SPIC_CFG_BUS_DUAL;
		break;
	case SPI_NOR_CMD_QREAD:
		command->address.disabled = 0;
		command->address.bus_width = SPIC_CFG_BUS_SINGLE;
		command->data.bus_width = SPIC_CFG_BUS_QUAD;
		break;
	case SPI_NOR_CMD_2READ:
		command->address.disabled = 0;
		command->address.bus_width = SPIC_CFG_BUS_DUAL;
		command->data.bus_width = SPIC_CFG_BUS_DUAL;
		break;
	case SPI_NOR_CMD_4READ:
	case SPI_NOR_CMD_PP_1_4_4:
		command->address.disabled = 0;
		command->address.bus_width = SPIC_CFG_BUS_QUAD;
		command->data.bus_width = SPIC_CFG_BUS_QUAD;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	command->instruction.value = cmd;
	command->address.size = addr_size;
	command->address.value = addr;
	command->dummy_count = dummy_count;

	return ret;
}

static int spic_wait_finish(const struct device *dev)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;
	int count = TIMEOUT_SPICEN;

	while (spic_reg->SSIENR & SPIC_SSIENR_SPICEN && count) {
		--count;
	}
	if (!count) {
		return -ETIMEDOUT;
	}
	return 0;
}

static inline void spic_flush_fifo(const struct device *dev)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;

	spic_reg->FLUSH = SPIC_FLUSH_ALL;
}

static inline void spic_cs_active(const struct device *dev)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;

	spic_reg->SER = 1UL;
}

static inline void spic_cs_deactivate(const struct device *dev)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;

	spic_reg->SER = 0UL;
}

static inline void spic_usermode(const struct device *dev)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;

	spic_reg->CTRL0 |= SPIC_CTRL0_USERMD;
}

static inline void spic_automode(const struct device *dev)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;

	spic_reg->CTRL0 &= ~SPIC_CTRL0_USERMD;
}

static void spic_prepare_command(const struct device *dev, const struct qspi_cmd *command,
				 uint32_t tx_size, uint32_t rx_size, uint8_t write)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;
	uint8_t addr_len = user_addr_len[command->address.size];

	spic_flush_fifo(dev);

	/* set SSIENR: deactivate to program this transfer */
	spic_reg->SSIENR = 0UL;

	/* set CTRLR0: TX mode and channel */
	spic_reg->CTRL0 &= ~(TMOD(3) | CMD_CH(3) | ADDR_CH(3) | DATA_CH(3));
	spic_reg->CTRL0 |= TMOD(write == 0x01 ? 0x00UL : 0x03UL) |
			   ADDR_CH(command->address.bus_width) | DATA_CH(command->data.bus_width);

	/* set USER_LENGTH */
	spic_reg->USERLENGTH = USER_CMD_LENGTH(1) |
			       USER_ADDR_LENGTH(command->address.disabled ? 0 : addr_len) |
			       USER_RD_DUMMY_LENGTH(command->dummy_count * spic_reg->BAUDR * 2);

	/* Write command */
	if (!command->instruction.disabled) {
		spic_reg->DR.BYTE = command->instruction.value;
	}

	/* Write address */
	if (!command->address.disabled) {
		for (int i = 0; i < addr_len; i++) {
			spic_reg->DR.BYTE =
				(uint8_t)(command->address.value >> (8 * (addr_len - i - 1)));
		}
	}

	/* Set TX_NDF: frame number of Tx data */
	spic_reg->TXNDF = TX_NDF(tx_size);

	/* Set RX_NDF: frame number of receiving data. */
	spic_reg->RXNDF = RX_NDF(rx_size);
}

static void spic_transmit_data(const struct device *dev, const void *data, uint32_t *length)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;

	uint32_t len = *length;

	/* set SSIENR to start the transfer */
	spic_reg->SSIENR = SPIC_SSIENR_SPICEN;

	/* write the remaining data into fifo */
	for (int i = 0; i < len;) {
		if (spic_reg->SR & SPIC_SR_TFNF) {
			spic_reg->DR.BYTE = ((const uint8_t *)data)[i];
			i++;
		}
	}
}

static void spic_receive_data(const struct device *dev, void *data, uint32_t *length)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;

	uint32_t i, cnt, rx_num, fifo, len;
	uint8_t *rx_data = data;

	len = *length;
	rx_data = data;

	/* set SSIENR to start the transfer */
	spic_reg->SSIENR = SPIC_SSIENR_SPICEN;

	rx_num = 0;
	while (rx_num < len) {
		cnt = spic_reg->RXFLR;

		for (i = 0; i < (cnt / 4); i++) {
			fifo = spic_reg->DR.WORD;
			memcpy((void *)(rx_data + rx_num), (void *)&fifo, 4);
			rx_num += 4;
		}

		if (rx_num < len) {
			uint32_t remaining = (len - rx_num < cnt % 4) ? len - rx_num : cnt % 4;

			for (i = 0; i < remaining; i++) {
				*(uint8_t *)(rx_data + rx_num) = spic_reg->DR.BYTE;
				rx_num += 1;
			}
		}
	}
}

static int spic_write(const struct device *dev, const struct qspi_cmd *command, const void *data,
		      uint32_t *length)
{
	int ret;

	spic_usermode(dev);
	spic_prepare_command(dev, command, *length, 0, COMMAND_WRITE);
	spic_cs_active(dev);

	spic_transmit_data(dev, data, length);
	ret = spic_wait_finish(dev);

	spic_cs_deactivate(dev);
	spic_automode(dev);

	return ret;
}

static int spic_read(const struct device *dev, const struct qspi_cmd *command, void *data,
		     size_t *length)
{
	int ret;

	spic_usermode(dev);
	spic_prepare_command(dev, command, 0, *length, COMMAND_READ);
	spic_cs_active(dev);

	spic_receive_data(dev, data, length);
	ret = spic_wait_finish(dev);

	spic_cs_deactivate(dev);
	spic_automode(dev);

	return ret;
}

static int flash_write_enable(const struct device *dev)
{
	struct flash_rts5912_dev_data *data = dev->data;
	struct qspi_cmd *command = &data->command_default;
	uint32_t len = 0;

	config_command(command, SPI_NOR_CMD_WREN, 0, 0, 0);
	return spic_write(dev, command, NULL, &len);
}

static int flash_write_disable(const struct device *dev)
{
	struct flash_rts5912_dev_data *data = dev->data;
	struct qspi_cmd *command = &data->command_default;
	uint32_t len = 0;

	config_command(command, SPI_NOR_CMD_WRDI, 0, 0, 0);
	return spic_write(dev, command, NULL, &len);
}

static int flash_read_sr(const struct device *dev, uint8_t *val)
{
	struct flash_rts5912_dev_data *data = dev->data;
	struct qspi_cmd *command = &data->command_default;
	int status;
	uint32_t len = 1;
	uint8_t sr;

	config_command(command, SPI_NOR_CMD_RDSR, 0, 0, 0);
	status = spic_read(dev, command, &sr, &len);
	if (status) {
		return status;
	}
	*val = sr;

	return 0;
}

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int flash_read_sr2(const struct device *dev, uint8_t *val)
{
	struct flash_rts5912_dev_data *data = dev->data;
	struct qspi_cmd *command = &data->command_default;
	int status;
	uint32_t len = 1;
	uint8_t sr;

	config_command(command, SPI_NOR_CMD_RDSR2, 0, 0, 0);
	status = spic_read(dev, command, &sr, &len);
	if (status) {
		return status;
	}
	*val = sr;

	return 0;
}
#endif

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int flash_set_wp(const struct device *dev, uint8_t *val)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;

	if (!val) {
		return -EINVAL;
	}

	if (*val) {
		spic_reg->CTRLR2 |= SPIC_CTRLR2_WPN_SET;
	}

	return 0;
}

static int flash_get_wp(const struct device *dev, uint8_t *val)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;

	*val = (uint8_t)(spic_reg->CTRLR2 & SPIC_CTRLR2_WPN_SET);

	return 0;
}
#endif

static int flash_wait_till_ready(const struct device *dev)
{
	int timeout = TIMEOUT_SPIBUSY;
	uint8_t sr = 0;

	/* If it's a sector erase loop, it requires approximately 3000 cycles,
	 * while a program page requires about 40 cycles.
	 */
	do {
		flash_read_sr(dev, &sr);
		if (!(sr & SPI_NOR_WIP_BIT)) {
			return 0;
		}
		timeout--;
	} while (timeout > 0);

	LOG_ERR("Flash wait timed out");
	return -ETIMEDOUT;
}

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int flash_write_status_reg(const struct device *dev, uint8_t *val, uint8_t cnt)
{
	struct flash_rts5912_dev_data *data = dev->data;
	struct qspi_cmd *command = &data->command_default;
	int ret;
	uint32_t len = cnt;

	ret = flash_write_enable(dev);
	if (ret < 0) {
		return ret;
	}

	config_command(command, SPI_NOR_CMD_WRSR, 0, 0, 0);
	ret = spic_write(dev, command, val, &len);
	if (ret < 0) {
		goto exit;
	}

	ret = flash_wait_till_ready(dev);
exit:
	flash_write_disable(dev);
	return ret;
}

static int flash_write_status_reg2(const struct device *dev, uint8_t *val, uint8_t cnt)
{
	struct flash_rts5912_dev_data *data = dev->data;
	struct qspi_cmd *command = &data->command_default;
	int ret;
	uint32_t len = cnt;

	ret = flash_write_enable(dev);
	if (ret < 0) {
		return ret;
	}

	config_command(command, SPI_NOR_CMD_WRSR2, 0, 0, 0);
	ret = spic_write(dev, command, val, &len);
	if (ret < 0) {
		goto exit;
	}

	ret = flash_wait_till_ready(dev);
exit:
	flash_write_disable(dev);
	return ret;
}
#endif

static int flash_erase_sector(const struct device *dev, uint32_t address)
{
	struct flash_rts5912_dev_data *data = dev->data;
	struct qspi_cmd *command = &data->command_default;
	enum spic_address_size addr_size = SPIC_CFG_ADDR_SIZE_24;
	int ret;
	uint32_t len = 0;

	ret = flash_write_enable(dev);
	if (ret < 0) {
		return ret;
	}

	config_command(command, SPI_NOR_CMD_SE, address, addr_size, 0);
	ret = spic_write(dev, command, NULL, &len);
	if (ret < 0) {
		goto err_exit;
	}
	ret = flash_wait_till_ready(dev);

err_exit:
	flash_write_disable(dev);
	return ret;
}

static int flash_program_page(const struct device *dev, uint32_t address, const uint8_t *data,
			      uint32_t size)
{
	struct flash_rts5912_dev_data *dev_data = dev->data;
	struct qspi_cmd *command = &dev_data->command_default;
	enum spic_address_size addr_size = SPIC_CFG_ADDR_SIZE_24;
	int ret = 0;
	uint32_t offset = 0, chunk = 0, page_size = FLASH_PAGE_SZ;

	while (size > 0) {
		ret = flash_write_enable(dev);
		if (ret < 0) {
			return ret;
		}

		offset = address % page_size;
		chunk = (offset + size < page_size) ? size : (page_size - offset);

		config_command(command, SPI_NOR_CMD_PP, address, addr_size, 0);
		ret = spic_write(dev, command, data, (size_t *)&chunk);
		if (ret < 0) {
			goto err_exit;
		}

		data += chunk;
		address += chunk;
		size -= chunk;

		flash_wait_till_ready(dev);
	}

err_exit:
	flash_write_disable(dev);
	return ret;
}

static int flash_normal_read(const struct device *dev, uint8_t rdcmd, uint32_t address,
			     uint8_t *data, uint32_t size)
{
	struct flash_rts5912_dev_data *dev_data = dev->data;
	struct qspi_cmd *command = &dev_data->command_default;
	enum spic_address_size addr_size = SPIC_CFG_ADDR_SIZE_24;
	int ret;

	uint32_t src_addr = address;
	uint8_t *dst_idx = data;

	uint32_t remind_size = size;
	uint32_t block_size = 0x8000UL;
	uint8_t dummy_count = (rdcmd == SPI_NOR_CMD_READ) ? 0 : 8;

	config_command(command, rdcmd, src_addr, addr_size, dummy_count);

	while (remind_size > 0) {
		command->address.value = src_addr;

		if (remind_size >= block_size) {
			ret = spic_read(dev, command, dst_idx, (size_t *)&block_size);
			src_addr += block_size;
			remind_size -= block_size;
			dst_idx += block_size;
		} else {
			ret = spic_read(dev, command, dst_idx, (size_t *)&remind_size);
			dst_idx += remind_size;
			remind_size = 0;
		}

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int check_boundary(off_t offset, size_t len)
{
	if (offset < 0) {
		return -EINVAL;
	}

	if (offset >= DT_REG_SIZE(SOC_NV_FLASH_NODE)) {
		return -EINVAL;
	}

	if (len > DT_REG_SIZE(SOC_NV_FLASH_NODE) - offset) {
		return -EINVAL;
	}

	return 0;
}

static int flash_rts5912_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_rts5912_dev_data *data = dev->data;
	int ret = -EINVAL;

	if (len == 0) {
		return 0;
	}

	if ((offset % FLASH_ERASE_BLK_SZ) != 0) {
		return -EINVAL;
	}

	if ((len % FLASH_ERASE_BLK_SZ) != 0) {
		return -EINVAL;
	}

	ret = check_boundary(offset, len);
	if (ret < 0) {
		return ret;
	}

	k_sem_take(&data->sem, K_FOREVER);

	for (; len > 0; len -= FLASH_ERASE_BLK_SZ) {
		ret = flash_erase_sector(dev, offset);
		if (ret < 0) {
			LOG_ERR("erase @0x%08lx fail", offset);
		}
		offset += FLASH_ERASE_BLK_SZ;
	}

	k_sem_give(&data->sem);

	return ret;
}

static int flash_rts5912_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct flash_rts5912_dev_data *dev_data = dev->data;
	int ret;
	unsigned int key;

	if (len == 0) {
		return 0;
	}

	ret = check_boundary(offset, len);
	if (ret < 0) {
		return ret;
	}

	k_sem_take(&dev_data->sem, K_FOREVER);
	key = irq_lock();
	ret = flash_program_page(dev, offset, data, len);
	irq_unlock(key);
	k_sem_give(&dev_data->sem);

	return ret;
}

static int flash_rts5912_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct flash_rts5912_dev_data *dev_data = dev->data;
	int ret;

	if (len == 0) {
		return 0;
	}

	ret = check_boundary(offset, len);
	if (ret < 0) {
		return ret;
	}

	k_sem_take(&dev_data->sem, K_FOREVER);
	ret = flash_normal_read(dev, SPI_NOR_CMD_READ, offset, data, len);
	k_sem_give(&dev_data->sem);

	return ret;
}

static const struct flash_parameters *flash_rts5912_get_parameters(const struct device *dev)
{
	const struct flash_rts5912_dev_config *config = dev->config;

	return &config->flash_rts5912_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout dev_layout = {
	.pages_count =
		DT_REG_SIZE(SOC_NV_FLASH_NODE) / DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
	.pages_size = DT_PROP(SOC_NV_FLASH_NODE, erase_block_size),
};

static void flash_rts5912_pages_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int flash_rts5912_ex_op(const struct device *dev, uint16_t opcode, const uintptr_t in,
			       void *out)
{
	struct flash_rts5912_dev_data *dev_data = dev->data;
	int ret = -EINVAL;

	k_sem_take(&dev_data->sem, K_FOREVER);

	switch (opcode) {
	case FLASH_RTS5912_EX_OP_WR_ENABLE:
		ret = flash_write_enable(dev);
		break;
	case FLASH_RTS5912_EX_OP_WR_DISABLE:
		ret = flash_write_disable(dev);
		break;
	case FLASH_RTS5912_EX_OP_WR_SR:
		ret = flash_write_status_reg(dev, (uint8_t *)out, 1);
		break;
	case FLASH_RTS5912_EX_OP_WR_SR2:
		ret = flash_write_status_reg2(dev, (uint8_t *)out, 1);
		break;
	case FLASH_RTS5912_EX_OP_RD_SR:
		ret = flash_read_sr(dev, (uint8_t *)in);
		break;
	case FLASH_RTS5912_EX_OP_RD_SR2:
		ret = flash_read_sr2(dev, (uint8_t *)in);
		break;
	case FLASH_RTS5912_EX_OP_SET_WP:
		ret = flash_set_wp(dev, (uint8_t *)out);
		break;
	case FLASH_RTS5912_EX_OP_GET_WP:
		ret = flash_get_wp(dev, (uint8_t *)in);
		break;
	}

	k_sem_give(&dev_data->sem);

	return ret;
}
#endif

static DEVICE_API(flash, flash_rts5912_api) = {
	.erase = flash_rts5912_erase,
	.write = flash_rts5912_write,
	.read = flash_rts5912_read,
	.get_parameters = flash_rts5912_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_rts5912_pages_layout,
#endif
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	.ex_op = flash_rts5912_ex_op,
#endif
};

static int flash_rts5912_init(const struct device *dev)
{
	const struct flash_rts5912_dev_config *config = dev->config;
	volatile struct reg_spic_reg *spic_reg = config->regs;
	struct flash_rts5912_dev_data *data = dev->data;

	spic_reg->SSIENR = 0UL;
	spic_reg->IMR = 0UL;

	spic_reg->CTRL0 = ((spic_reg->CTRL0 & SPIC_CTRL0_CK_MTIMES_Msk) | CMD_CH(0) | DATA_CH(0) |
			   ADDR_CH(0) | MODE(0) | ((spic_reg->CTRL0 & SPIC_CTRL0_SIPOL_Msk)));

	spic_reg->BAUDR = 1UL;
	spic_reg->FBAUD = 1UL;

	k_sem_init(&data->sem, 1, 1);

	return 0;
}

static struct flash_rts5912_dev_data flash_rts5912_data = {
	.command_default = {
		.instruction = {
				.bus_width = SPIC_CFG_BUS_SINGLE,
				.disabled = 0,
			},
		.address = {
				.bus_width = SPIC_CFG_BUS_SINGLE,
				.size = SPIC_CFG_ADDR_SIZE_24,
				.disabled = 0,
			},
		.alt = {
				.size = 0,
				.disabled = 1,
			},
		.dummy_count = 0,
		.data = {
				.bus_width = SPIC_CFG_BUS_SINGLE,
			},
	},
};

static const struct flash_rts5912_dev_config flash_rts5912_config = {
	.regs = (volatile struct reg_spic_reg *)DT_INST_REG_ADDR(0),
	.flash_rts5912_parameters = {
			.write_block_size = FLASH_WRITE_BLK_SZ,
			.erase_value = 0xff,
		},
};

DEVICE_DT_INST_DEFINE(0, flash_rts5912_init, NULL, &flash_rts5912_data, &flash_rts5912_config,
		      PRE_KERNEL_1, CONFIG_FLASH_INIT_PRIORITY, &flash_rts5912_api);
