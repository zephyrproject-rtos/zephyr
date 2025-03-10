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

#include "reg/reg_spic.h"

#define SPIC_REG_BASE ((SPIC_Type *)DT_INST_REG_ADDR(0))

#define MODE(x)    ((uint32_t)((x) & 0x00000003ul) << SPIC_CTRL0_SCPH_Pos)
#define TMOD(x)    ((uint32_t)(((x) & 0x00000003ul) << SPIC_CTRL0_TMOD_Pos))
#define CMD_CH(x)  ((uint32_t)(((x) & 0x00000003ul) << SPIC_CTRL0_CMDCH_Pos))
#define ADDR_CH(x) ((uint32_t)(((x) & 0x00000003ul) << SPIC_CTRL0_ADDRCH_Pos))
#define DATA_CH(x) ((uint32_t)(((x) & 0x00000003ul) << SPIC_CTRL0_DATACH_Pos))

#define USER_CMD_LENGTH(x)      ((uint32_t)(((x) & 0x00000003ul) << SPIC_USERLENGTH_CMDLEN_Pos))
#define USER_ADDR_LENGTH(x)     ((uint32_t)(((x) & 0x0000000ful) << SPIC_USERLENGTH_ADDRLEN_Pos))
#define USER_RD_DUMMY_LENGTH(x) ((uint32_t)(((x) & 0x00000ffful) << SPIC_USERLENGTH_RDDUMMYLEN_Pos))

#define TX_NDF(x) ((uint32_t)(((x) & 0x00fffffful) << SPIC_TXNDF_NUM_Pos))
#define RX_NDF(x) ((uint32_t)(((x) & 0x00fffffful) << SPIC_RXNDF_NUM_Pos))

#define CK_MTIMES(x)     ((uint32_t)(((x) & 0x0000001ful) << 23))
#define GET_CK_MTIMES(x) ((uint32_t)(((x >> 23) & 0x0000001ful)))

#define SIPOL(x)     ((uint32_t)(((x) & 0x0000001ful) << 0))
#define GET_SIPOL(x) ((uint32_t)(((x >> 0) & 0x0000001ful)))

#define SR_WIP (0x01) /* Write in progress */
#define SR_WEL (0x02) /* Write enable latch */

#define FLASH_CMD_WREN           0x06 /* write enable */
#define FLASH_CMD_WRDI           0x04 /* write disable */
#define FLASH_CMD_WRSR           0x01 /* write status register */
#define FLASH_CMD_WRSR2          0x31 /* write status register */
#define FLASH_CMD_RDID           0x9F /* read idenfication */
#define FLASH_CMD_RDSR           0x05 /* read status register */
#define FLASH_CMD_RDSR2          0x35 /* read status register-2 */
#define FLASH_CMD_RDSR3          0x15 /* read status register-3 */
#define FLASH_CMD_READ           0x03 /* read data */
#define FLASH_CMD_FREAD          0x0B /* fast read data */
#define FLASH_CMD_RDSFDP         0x5A /* Read SFDP */
#define FLASH_CMD_RES            0xAB /* Read Electronic ID */
#define FLASH_CMD_REMS           0x90 /* Read Electronic Manufacturer & Device ID */
#define FLASH_CMD_DREAD          0x3B /* Double Output Mode command */
#define FLASH_CMD_SE             0x20 /* Sector Erase for 3-byte addressing */
#define FLASH_CMD_SE_4B          0x21 /* Sector Erase for 4-byte addressing */
#define FLASH_CMD_BE             0xD8 /* 0x52 //64K Block Erase */
#define FLASH_CMD_CE             0xC7 /* Chip Erase(or 0x60) */
#define FLASH_CMD_PP             0x02 /* Page Program for 3-byte addressing */
#define FLASH_CMD_PP_4B          0x12 /* Page Program for 4-byte addressing */
#define FLASH_CMD_DP             0xB9 /* Deep Power Down */
#define FLASH_CMD_RDP            0xAB /* Release from Deep Power-Down */
#define FLASH_CMD_2READ          0xBB /* 2 x I/O read  command */
#define FLASH_CMD_4READ          0xEB /* 4 x I/O read  command */
#define FLASH_CMD_QREAD          0x6B /* 1I / 4O read command */
#define FLASH_CMD_4PP            0x38 /* quad page program */
#define FLASH_CMD_FF             0xFF /* Release Read Enhanced */
#define FLASH_CMD_REMS2          0x92 /* read ID for 2x I/O mode, diff with MXIC */
#define FLASH_CMD_REMS4          0x94 /* read ID for 4x I/O mode, diff with MXIC */
#define FLASH_CMD_RDSCUR         0x48 /* read security register,  diff with MXIC */
#define FLASH_CMD_WRSCUR         0x42 /* write security register, diff with MXIC */
#define FLASH_CMD_EN_RST         0x66 /* reset enable */
#define FLASH_CMD_RST_DEV        0x99 /* reset device */
#define FLASH_CMD_EN4B           0xB7 /* Enter 4-byte mode */
#define FLASH_CMD_EX4B           0xE9 /* Exit 4-byte mode */
#define FLASH_CMD_EXTNADDR_WREAR 0xC5 /* Write extended address register */
#define FLASH_CMD_EXTNADDR_RDEAR 0xC8 /* Read extended address register */

enum {
	COMMAND_READ = 0,
	COMMAND_WRITE = 1,
};

typedef enum spic_status {
	SPIC_STATUS_ERROR = -1,
	SPIC_STATUS_INVALID_PARAMETER = -2,
	SPIC_STATUS_OK = 0,
} SPIC_STATUS_t;

typedef enum {
	SPIC_FREQ_SYS_CLK_DIV2 = 1,
	SPIC_FREQ_SYS_CLK_DIV4,
	SPIC_FREQ_SYS_CLK_DIV8,
	SPIC_FREQ_SYS_CLK_DIV16,
} SPIC_FREQ_t;

typedef enum spic_bus_width {
	SPIC_CFG_BUS_SINGLE,
	SPIC_CFG_BUS_DUAL,
	SPIC_CFG_BUS_QUAD,
} SPIC_BUS_WIDTH_t;

typedef enum spic_address_size {
	SPIC_CFG_ADDR_SIZE_8,
	SPIC_CFG_ADDR_SIZE_16,
	SPIC_CFG_ADDR_SIZE_24,
	SPIC_CFG_ADDR_SIZE_32,
} SPIC_ADDRESS_SIZE_t;

typedef uint8_t SPIC_ALT_SIZE_t;

typedef struct qspi_command {
	struct {
		SPIC_BUS_WIDTH_t bus_width; /**< Bus width for the instruction >*/
		uint8_t value;              /**< Instruction value >*/
		uint8_t disabled; /**< Instruction phase skipped if disabled is set to true >*/
	} instruction;
	struct {
		SPIC_BUS_WIDTH_t bus_width; /**< Bus width for the address >*/
		SPIC_ADDRESS_SIZE_t size;   /**< Address size >*/
		uint32_t value;             /**< Address value >*/
		uint8_t disabled; /**< Address phase skipped if disabled is set to true >*/
	} address;
	struct {
		SPIC_BUS_WIDTH_t bus_width; /**< Bus width for alternative  >*/
		SPIC_ALT_SIZE_t size;       /**< Alternative size >*/
		uint32_t value;             /**< Alternative value >*/
		uint8_t disabled; /**< Alternative phase skipped if disabled is set to true >*/
	} alt;
	uint8_t dummy_count; /**< Dummy cycles count >*/
	struct {
		SPIC_BUS_WIDTH_t bus_width; /**< Bus width for data >*/
	} data;
} SPIC_COMMAND_t;

struct flash_rts5912_dev_data {
	struct k_sem sem;
	SPIC_COMMAND_t command_default;
};

static const uint8_t user_addr_len[] = {
	[SPIC_CFG_ADDR_SIZE_8] = 1,
	[SPIC_CFG_ADDR_SIZE_16] = 2,
	[SPIC_CFG_ADDR_SIZE_24] = 3,
	[SPIC_CFG_ADDR_SIZE_32] = 4,
};

static const struct flash_parameters flash_rts5912_parameters = {
	.write_block_size = FLASH_WRITE_BLK_SZ,
	.erase_value = 0xff,
};

static int config_command(SPIC_COMMAND_t *command, uint8_t cmd, uint32_t addr,
			  SPIC_ADDRESS_SIZE_t addr_size, uint8_t dummy_count)
{
	int8_t rc = 0;

	switch (cmd) {
	case FLASH_CMD_WREN:
	case FLASH_CMD_WRDI:
	case FLASH_CMD_WRSR:
	case FLASH_CMD_RDID:
	case FLASH_CMD_RDSR:
	case FLASH_CMD_RDSR2:
	case FLASH_CMD_CE:
	case FLASH_CMD_EN4B:
	case FLASH_CMD_EX4B:
	case FLASH_CMD_EXTNADDR_WREAR:
	case FLASH_CMD_EXTNADDR_RDEAR:
	case FLASH_CMD_EN_RST:
	case FLASH_CMD_RST_DEV:
		command->address.disabled = 1;
		command->data.bus_width = SPIC_CFG_BUS_SINGLE;
		break;
	case FLASH_CMD_READ:
	case FLASH_CMD_FREAD:
	case FLASH_CMD_SE:
	case FLASH_CMD_BE:
	case FLASH_CMD_RDSFDP:
	case FLASH_CMD_PP:
		command->address.disabled = 0;
		command->address.bus_width = SPIC_CFG_BUS_SINGLE;
		command->data.bus_width = SPIC_CFG_BUS_SINGLE;
		break;
	case FLASH_CMD_DREAD:
		command->address.disabled = 0;
		command->address.bus_width = SPIC_CFG_BUS_SINGLE;
		command->data.bus_width = SPIC_CFG_BUS_DUAL;
		break;
	case FLASH_CMD_QREAD:
		command->address.disabled = 0;
		command->address.bus_width = SPIC_CFG_BUS_SINGLE;
		command->data.bus_width = SPIC_CFG_BUS_QUAD;
		break;
	case FLASH_CMD_2READ:
		command->address.disabled = 0;
		command->address.bus_width = SPIC_CFG_BUS_DUAL;
		command->data.bus_width = SPIC_CFG_BUS_DUAL;
		break;
	case FLASH_CMD_4READ:
	case FLASH_CMD_4PP:
		command->address.disabled = 0;
		command->address.bus_width = SPIC_CFG_BUS_QUAD;
		command->data.bus_width = SPIC_CFG_BUS_QUAD;
		break;
	default:
		rc = -1;
		break;
	}

	command->instruction.value = cmd;
	command->address.size = addr_size;
	command->address.value = addr;
	command->dummy_count = dummy_count;

	return rc;
}

static void spic_wait_finish(void)
{
	SPIC_Type *spic_reg = SPIC_REG_BASE;

	while (spic_reg->SSIENR & SPIC_SSIENR_SPICEN_Msk) {
	}
}

static void spic_flush_fifo(void)
{
	SPIC_Type *spic_reg = SPIC_REG_BASE;

	spic_reg->FLUSH = SPIC_FLUSH_ALL_Msk;
}

static void spic_cs_active(void)
{
	SPIC_Type *spic_reg = SPIC_REG_BASE;

	spic_reg->SER = 1;
}

static void spic_cs_deactivate(void)
{
	SPIC_Type *spic_reg = SPIC_REG_BASE;

	spic_reg->SER = 0ul;
}

static void spic_usermode(void)
{
	SPIC_Type *spic_reg = SPIC_REG_BASE;

	spic_reg->CTRL0 |= SPIC_CTRL0_USERMD_Msk;
}

static void spic_automode(void)
{
	SPIC_Type *spic_reg = SPIC_REG_BASE;

	spic_reg->CTRL0 &= ~SPIC_CTRL0_USERMD_Msk;
}

static SPIC_STATUS_t spic_prepare_command(const SPIC_COMMAND_t *command, uint32_t tx_size,
					  uint32_t rx_size, uint8_t write)
{
	SPIC_Type *spic_reg = SPIC_REG_BASE;

	uint32_t i;
	uint8_t addr_len = user_addr_len[command->address.size];

	spic_flush_fifo();

	/* set SSIENR: deactivate to program this transfer */
	spic_reg->SSIENR = 0ul;

	/* set CTRLR0: TX mode and channel */
	spic_reg->CTRL0 &= ~(TMOD(3) | CMD_CH(3) | ADDR_CH(3) | DATA_CH(3));
	spic_reg->CTRL0 |= TMOD(write == 0x01 ? 0x00ul : 0x03ul) |
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
		for (i = 0; i < addr_len; i++) {
			spic_reg->DR.BYTE =
				(uint8_t)(command->address.value >> (8 * (addr_len - i - 1)));
		}
	}

	/* Set TX_NDF: frame number of Tx data */
	spic_reg->TXNDF = TX_NDF(tx_size);

	/* Set RX_NDF: frame number of receiving data. */
	spic_reg->RXNDF = RX_NDF(rx_size);

	return SPIC_STATUS_OK;
}

static SPIC_STATUS_t spic_transmit_data(const void *data, uint32_t *length)
{
	SPIC_Type *spic_reg = SPIC_REG_BASE;

	uint32_t i, len = *length;

	/* set SSIENR to start the transfer */
	spic_reg->SSIENR = SPIC_SSIENR_SPICEN_Msk;

	/* write the remaining data into fifo */
	for (i = 0; i < len;) {
		if (spic_reg->SR & SPIC_SR_TFNF_Msk) {
			spic_reg->DR.BYTE = ((uint8_t *)data)[i];
			i++;
		}
	}
	return SPIC_STATUS_OK;
}

static SPIC_STATUS_t spic_receive_data(void *data, uint32_t *length)
{
	SPIC_Type *spic_reg = SPIC_REG_BASE;

	uint32_t i, cnt, rx_num, fifo, len;
	uint8_t *rx_data = data;

	len = *length;
	rx_data = data;

	/* set SSIENR to start the transfer */
	spic_reg->SSIENR = SPIC_SSIENR_SPICEN_Msk;

	rx_num = 0;
	while (rx_num < len) {
		cnt = spic_reg->RXFLR;

		for (i = 0; i < (cnt / 4); i++) {
			fifo = spic_reg->DR.WORD;
			memcpy((void *)(rx_data + rx_num), (void *)&fifo, 4);
			rx_num += 4;
		}

		if (((rx_num / 4) == (len / 4)) && (rx_num < len)) {
			for (i = 0; i < (cnt % 4); i++) {
				*(uint8_t *)(rx_data + rx_num) = spic_reg->DR.BYTE;
				rx_num += 1;
			}
		}
	}

	return SPIC_STATUS_OK;
}

SPIC_STATUS_t spic_write(const SPIC_COMMAND_t *command, const void *data, uint32_t *length)
{
	spic_usermode();
	spic_prepare_command(command, *length, 0, COMMAND_WRITE);
	spic_cs_active();

	spic_transmit_data(data, length);
	spic_wait_finish();

	spic_cs_deactivate();
	spic_automode();

	return SPIC_STATUS_OK;
}

SPIC_STATUS_t spic_read(const SPIC_COMMAND_t *command, void *data, size_t *length)
{
	spic_usermode();
	spic_prepare_command(command, 0, *length, COMMAND_READ);
	spic_cs_active();

	spic_receive_data(data, length);
	spic_wait_finish();

	spic_cs_deactivate();
	spic_automode();

	return SPIC_STATUS_OK;
}

static int flash_write_enable(const struct device *dev)
{
	struct flash_rts5912_dev_data *data = dev->data;
	SPIC_COMMAND_t *command = &data->command_default;
	SPIC_STATUS_t status;

	uint32_t len = 0;

	config_command(command, FLASH_CMD_WREN, 0, 0, 0);
	status = spic_write(command, NULL, &len);
	if (status != SPIC_STATUS_OK) {
		return (int)status;
	}

	return 0;
}

static int flash_write_disable(const struct device *dev)
{
	struct flash_rts5912_dev_data *data = dev->data;
	SPIC_COMMAND_t *command = &data->command_default;
	SPIC_STATUS_t status;

	uint32_t len = 0;

	config_command(command, FLASH_CMD_WRDI, 0, 0, 0);
	status = spic_write(command, NULL, &len);
	if (status != SPIC_STATUS_OK) {
		return (int)status;
	}

	return 0;
}

static int flash_read_sr(const struct device *dev, uint8_t *val)
{
	struct flash_rts5912_dev_data *data = dev->data;
	SPIC_COMMAND_t *command = &data->command_default;
	SPIC_STATUS_t status;

	uint32_t len = 1;
	uint8_t sr;

	config_command(command, FLASH_CMD_RDSR, 0, 0, 0);
	status = spic_read(command, &sr, &len);
	LOG_DBG("flash: read_sr1 = 0x%02x", sr);
	if (status != SPIC_STATUS_OK) {
		return (int)status;
	}

	*val = sr;

	return 0;
}

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int flash_read_sr2(const struct device *dev, uint8_t *val)
{
	struct flash_rts5912_dev_data *data = dev->data;
	SPIC_COMMAND_t *command = &data->command_default;
	SPIC_STATUS_t status;

	uint32_t len = 1;
	uint8_t sr;

	config_command(command, FLASH_CMD_RDSR2, 0, 0, 0);
	status = spic_read(command, &sr, &len);
	LOG_DBG("flash: read_sr2 = 0x%02x", sr);
	if (status != SPIC_STATUS_OK) {
		return (int)status;
	}

	*val = sr;

	return 0;
}
#endif

static int32_t flash_wait_till_ready(const struct device *dev)
{
	int ret;
	int8_t sr = 0;

	do {
		ret = (int8_t)flash_read_sr(dev, &sr);
		if (ret < 0) {
			return ret;
		}
	} while (sr & SR_WIP);

	return 0;
}

#ifdef CONFIG_FLASH_EX_OP_ENABLED
static int flash_write_status_reg(const struct device *dev, uint8_t *val, uint8_t cnt)
{
	struct flash_rts5912_dev_data *data = dev->data;
	SPIC_COMMAND_t *command = &data->command_default;
	SPIC_STATUS_t status;

	uint32_t len = cnt;

	flash_write_enable(dev);

	LOG_DBG("WSR = 0x%02x", *(uint8_t *)val);

	config_command(command, FLASH_CMD_WRSR, 0, 0, 0);
	status = spic_write(command, val, &len);
	if (status != SPIC_STATUS_OK) {
		return (int)status;
	}

	flash_wait_till_ready(dev);
	flash_write_disable(dev);

	return 0;
}

static int flash_write_status_reg2(const struct device *dev, uint8_t *val, uint8_t cnt)
{
	struct flash_rts5912_dev_data *data = dev->data;
	SPIC_COMMAND_t *command = &data->command_default;
	SPIC_STATUS_t status;

	uint32_t len = cnt;

	flash_write_enable(dev);

	LOG_DBG("WSR2 = 0x%02x", *(uint8_t *)val);

	config_command(command, FLASH_CMD_WRSR2, 0, 0, 0);
	status = spic_write(command, val, &len);
	if (status != SPIC_STATUS_OK) {
		return (int)status;
	}

	flash_wait_till_ready(dev);
	flash_write_disable(dev);

	return 0;
}
#endif

static int flash_erase_sector(const struct device *dev, uint32_t address)
{
	struct flash_rts5912_dev_data *data = dev->data;
	SPIC_COMMAND_t *command = &data->command_default;
	SPIC_ADDRESS_SIZE_t addr_size = SPIC_CFG_ADDR_SIZE_24;
	SPIC_STATUS_t status;

	uint8_t erase_cmd = FLASH_CMD_SE;

	int32_t rc = 0;
	uint32_t len = 0;

	flash_write_enable(dev);

	config_command(command, erase_cmd, address, addr_size, 0);
	status = spic_write(command, NULL, &len);
	if (status != SPIC_STATUS_OK) {
		rc = -1;
		goto err_exit;
	}

	flash_wait_till_ready(dev);

err_exit:
	flash_write_disable(dev);
	return rc;
}

int32_t flash_program_page(const struct device *dev, uint32_t address, const uint8_t *data,
			   uint32_t size)
{
	struct flash_rts5912_dev_data *dev_data = dev->data;
	SPIC_COMMAND_t *command = &dev_data->command_default;
	SPIC_ADDRESS_SIZE_t addr_size = SPIC_CFG_ADDR_SIZE_24;
	SPIC_STATUS_t status;

	uint8_t wr_cmd = FLASH_CMD_PP;

	uint32_t offset = 0, chunk = 0, page_size = FLASH_PAGE_SZ;
	int32_t rc = 0;

	while (size > 0) {
		offset = address % page_size;
		chunk = (offset + size < page_size) ? size : (page_size - offset);

		flash_write_enable(dev);

		config_command(command, wr_cmd, address, addr_size, 0);
		status = spic_write(command, data, (size_t *)&chunk);
		if (status != SPIC_STATUS_OK) {
			rc = -1;
			goto err_exit;
		}

		data += chunk;
		address += chunk;
		size -= chunk;

		flash_wait_till_ready(dev);
	}

err_exit:
	flash_write_disable(dev);
	return rc;
}

static int flash_normal_read(const struct device *dev, uint8_t rdcmd, uint32_t address,
			     uint8_t *data, uint32_t size)
{
	struct flash_rts5912_dev_data *dev_data = dev->data;
	SPIC_COMMAND_t *command = &dev_data->command_default;
	SPIC_ADDRESS_SIZE_t addr_size = SPIC_CFG_ADDR_SIZE_24;
	SPIC_STATUS_t status;

	uint32_t src_addr = address;
	uint8_t *dst_idx = data;

	uint32_t remind_size = size;
	uint32_t block_size = 0x8000ul;

	while (remind_size > 0) {
		switch (rdcmd) {
		default:
		case 0x03:
			config_command(command, FLASH_CMD_READ, src_addr, addr_size, 0);
			break;
		case 0x0B:
			config_command(command, FLASH_CMD_FREAD, src_addr, addr_size, 8);
			break;
		case 0x3B:
			config_command(command, FLASH_CMD_DREAD, src_addr, addr_size, 8);
			break;
		case 0x6B:
			config_command(command, FLASH_CMD_QREAD, src_addr, addr_size, 8);
			break;
		}

		if (remind_size >= block_size) {
			status = spic_read(command, dst_idx, (size_t *)&block_size);
			src_addr += block_size;
			remind_size -= block_size;
		} else {
			status = spic_read(command, dst_idx, (size_t *)&remind_size);
			remind_size = 0;
		}

		if (status != SPIC_STATUS_OK) {
			return -1;
		}

		dst_idx += block_size;
	}

	return 0;
}

static int flash_rts5912_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_rts5912_dev_data *data = dev->data;
	int rc = -EINVAL;

	if ((offset % FLASH_ERASE_BLK_SZ) != 0) {
		return -EINVAL;
	}

	if ((len % FLASH_ERASE_BLK_SZ) != 0) {
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	for (; len > 0; len -= FLASH_ERASE_BLK_SZ) {
		LOG_DBG("@0x%08lx", offset);
		rc = flash_erase_sector(dev, offset);
		if (rc < 0) {
			LOG_ERR("erase @0x%08lx fail", offset);
		}
		offset += FLASH_ERASE_BLK_SZ;
	}

	k_sem_give(&data->sem);

	return rc;
}

static int flash_rts5912_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct flash_rts5912_dev_data *dev_data = dev->data;
	int rc = -EINVAL;

	LOG_DBG("@0x%08lx, len: %d", offset, len);

	k_sem_take(&dev_data->sem, K_FOREVER);
	__disable_irq();
	rc = flash_program_page(dev, offset, data, len);
	__enable_irq();
	k_sem_give(&dev_data->sem);

	return rc;
}

static int flash_rts5912_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct flash_rts5912_dev_data *dev_data = dev->data;
	int rc = -EINVAL;

	LOG_DBG("@0x%08lx, len: %d", offset, len);

	k_sem_take(&dev_data->sem, K_FOREVER);

	rc = flash_normal_read(dev, FLASH_CMD_READ, offset, data, len);

	k_sem_give(&dev_data->sem);

	return rc;
}

static const struct flash_parameters *flash_rts5912_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_rts5912_parameters;
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
	SPIC_Type *spic_reg = SPIC_REG_BASE;
	struct flash_rts5912_dev_data *data = dev->data;

	spic_reg->SSIENR = 0ul;
	spic_reg->IMR = 0ul;

	spic_reg->CTRL0 = CK_MTIMES(GET_CK_MTIMES(spic_reg->CTRL0)) | CMD_CH(0) | DATA_CH(0) |
			  ADDR_CH(0) | MODE(0) | SIPOL(GET_SIPOL(spic_reg->CTRL0));

	spic_reg->BAUDR = 1ul;
	spic_reg->FBAUD = 1ul;

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

DEVICE_DT_INST_DEFINE(0, flash_rts5912_init, NULL, &flash_rts5912_data, NULL, PRE_KERNEL_1,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_rts5912_api);
