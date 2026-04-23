/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_flexspi_nand

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "memc_mcux_flexspi.h"

LOG_MODULE_REGISTER(flash_flexspi_nand, CONFIG_FLASH_LOG_LEVEL);

/* SPI-NAND common commands. */
#define SPI_NAND_CMD_READ_REG      0x0FU
#define SPI_NAND_CMD_WRITE_REG     0x1FU
#define SPI_NAND_CMD_READ_ID       0x9FU
#define SPI_NAND_CMD_RESET         0xFFU
#define SPI_NAND_CMD_PAGE_READ     0x13U
#define SPI_NAND_CMD_READ_CACHE_X4 0x6BU
#define SPI_NAND_CMD_WRITE_ENABLE  0x06U
#define SPI_NAND_CMD_PROG_LOAD     0x32U
#define SPI_NAND_CMD_PROG_LOAD_X1  0x02U
#define SPI_NAND_CMD_PROG_EXEC     0x10U
#define SPI_NAND_CMD_BLOCK_ERASE   0xD8U

/*
 * W25N01GV OOB (spare) area starts at column address 2048.
 * The overlay uses column-space = 11, so CADDR can only address columns 0–2047.
 * For OOB access we build a custom LUT using raw SDR bytes to encode CA[15:8] = 0x08
 * (= 2048 >> 8) and CA[7:0] = 0x00 directly, bypassing the CADDR mechanism.
 */
#define SPI_NAND_OOB_COLUMN_HI 0x08U /* CA[15:8] for column 2048 (0x0800) */
#define SPI_NAND_OOB_COLUMN_LO 0x00U /* CA[7:0]  for column 2048 */

/* Register addresses. */
#define SPI_NAND_REG_BLOCK_PROT 0xA0U
#define SPI_NAND_REG_CONFIG     0xB0U
#define SPI_NAND_REG_STATUS     0xC0U

/* STATUS register bits. */
#define SPI_NAND_STATUS_BUSY      BIT(0)
#define SPI_NAND_STATUS_WEL       BIT(1)
#define SPI_NAND_STATUS_E_FAIL    BIT(2)
#define SPI_NAND_STATUS_P_FAIL    BIT(3)
#define SPI_NAND_STATUS_ECC_MASK  (BIT(4) | BIT(5))
#define SPI_NAND_STATUS_ECC_SHIFT 4

/* CONFIG register bits. */
#define SPI_NAND_CFG_ECC_EN BIT(4)
#define SPI_NAND_CFG_OTP_EN BIT(6)

#define SPI_NAND_RESET_TIME_US 5


/* ONFI parameter page, treat as a raw 256-byte block.
 * CRC is calculated over the first 254 bytes, with seed 0x4F4E.
 */
struct nand_onfi_parameter_page_raw {
	uint8_t data[256];
};

enum {
	DYNAMIC_SEQ = 0,
	PAGE_READ,
	READ_CACHE,
	PROG_LOAD,
	PROG_EXEC,
	LUT_END,
};

struct flash_flexspi_nand_data {
	const struct device *controller;
	flexspi_device_config_t config;
	flexspi_port_t port;
	uint64_t size;
	uint32_t page_size;
	uint16_t oob_size;
	uint32_t pages_per_block;
	uint32_t blocks_per_unit;
	uint8_t units;
	uint32_t block_size;
	struct k_mutex lock;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
	struct flash_parameters flash_parameters;
};

static const uint32_t flexspi_lut[LUT_END][MEMC_FLEXSPI_CMD_PER_SEQ] = {
	[DYNAMIC_SEQ] = {0},

	[PAGE_READ] = {
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_PAGE_READ,
					kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
		},

	[READ_CACHE] = {
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD,
					SPI_NAND_CMD_READ_CACHE_X4, kFLEXSPI_Command_CADDR_SDR,
					kFLEXSPI_1PAD, 0x10),
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 0x08,
					kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x80),
		},

	[PROG_LOAD] = {
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_PROG_LOAD,
					kFLEXSPI_Command_CADDR_SDR, kFLEXSPI_1PAD, 0x10),
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0x40,
					kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
		},

	[PROG_EXEC] = {
			FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_PROG_EXEC,
					kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
		},
};

static ALWAYS_INLINE bool area_is_subregion(const struct device *dev, off_t offset, size_t size)
{
	struct flash_flexspi_nand_data *nand = dev->data;

	return ((offset >= 0) && (offset < nand->size) && ((nand->size - offset) >= size));
}

static int flash_flexspi_nand_xfer(const struct device *dev, uint8_t seq_index,
				   flexspi_command_type_t type, uint32_t addr, void *data,
				   size_t data_len)
{
	struct flash_flexspi_nand_data *nand = dev->data;
	flexspi_transfer_t transfer = {
		.deviceAddress = addr,
		.port = nand->port,
		.cmdType = type,
		.SeqNumber = 1,
		.seqIndex = seq_index,
		.data = data,
		.dataSize = data_len,
	};

	return memc_flexspi_transfer(nand->controller, &transfer);
}

static int flash_flexspi_update_lut(const struct device *dev, const uint32_t *lut_ptr)
{
	struct flash_flexspi_nand_data *nand = dev->data;

	return memc_flexspi_update_lut(nand->controller, nand->port, DYNAMIC_SEQ, lut_ptr,
				       MEMC_FLEXSPI_CMD_PER_SEQ);
}

static int flash_flexspi_nand_read_reg(const struct device *dev, uint8_t reg, uint8_t *value)
{
	const uint32_t lut_read_reg[MEMC_FLEXSPI_CMD_PER_SEQ] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_READ_REG,
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, reg),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x01,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
	};
	uint32_t data = 0;
	int ret;

	ret = flash_flexspi_update_lut(dev, lut_read_reg);
	if (ret != 0) {
		return ret;
	}

	ret = flash_flexspi_nand_xfer(dev, DYNAMIC_SEQ, kFLEXSPI_Read, 0, &data, 1);
	if (ret == 0) {
		*value = (uint8_t)data;
	}

	return ret;
}

static int flash_flexspi_nand_write_reg(const struct device *dev, uint8_t reg, uint8_t value)
{
	const uint32_t lut_write_reg[MEMC_FLEXSPI_CMD_PER_SEQ] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_WRITE_REG,
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, reg),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x01,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
	};
	int ret;

	ret = flash_flexspi_update_lut(dev, lut_write_reg);
	if (ret != 0) {
		return ret;
	}

	return flash_flexspi_nand_xfer(dev, DYNAMIC_SEQ, kFLEXSPI_Write, 0, &value, 1);
}

static int flash_flexspi_nand_set_block_protect(const struct device *dev, uint8_t value)
{
	return flash_flexspi_nand_write_reg(dev, SPI_NAND_REG_BLOCK_PROT, value);
}

static int flash_flexspi_nand_write_enable(const struct device *dev)
{
	const uint32_t lut_we[MEMC_FLEXSPI_CMD_PER_SEQ] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_WRITE_ENABLE,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
	};
	int ret;

	ret = flash_flexspi_update_lut(dev, lut_we);
	if (ret != 0) {
		return ret;
	}

	return flash_flexspi_nand_xfer(dev, DYNAMIC_SEQ, kFLEXSPI_Command, 0, NULL, 0);
}

static int flash_flexspi_nand_wait_busy(const struct device *dev, uint32_t timeout_us)
{
	uint32_t cycles = k_us_to_cyc_ceil32(timeout_us);
	uint32_t start = k_cycle_get_32();
	uint8_t status;
	int ret;

	do {
		ret = flash_flexspi_nand_read_reg(dev, SPI_NAND_REG_STATUS, &status);
		if (ret != 0) {
			return ret;
		}
		if ((status & SPI_NAND_STATUS_BUSY) == 0U) {
			return 0;
		}
		k_busy_wait(5);
	} while ((timeout_us == 0U) || ((k_cycle_get_32() - start) < cycles));

	return -ETIMEDOUT;
}

static int flash_flexspi_nand_reset(const struct device *dev)
{
	const uint32_t lut_reset[MEMC_FLEXSPI_CMD_PER_SEQ] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_RESET,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
	};
	int ret;

	ret = flash_flexspi_update_lut(dev, lut_reset);
	if (ret != 0) {
		return ret;
	}

	ret = flash_flexspi_nand_xfer(dev, DYNAMIC_SEQ, kFLEXSPI_Command, 0, NULL, 0);
	if (ret != 0) {
		return ret;
	}

	/* W25N01GV tRST in IDLE. */
	k_usleep(SPI_NAND_RESET_TIME_US);

	return ret;
}

static int flash_flexspi_nand_read_id(const struct device *dev, uint8_t id[3])
{
	const uint32_t lut_read_id[MEMC_FLEXSPI_CMD_PER_SEQ] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_READ_ID,
				kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 0x08),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x03,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
	};
	uint32_t value = 0;
	int ret;

	ret = flash_flexspi_update_lut(dev, lut_read_id);
	if (ret != 0) {
		return ret;
	}

	ret = flash_flexspi_nand_xfer(dev, DYNAMIC_SEQ, kFLEXSPI_Read, 0, &value, 3);
	if (ret != 0) {
		return ret;
	}

	id[0] = (uint8_t)(value & 0xFF);
	id[1] = (uint8_t)((value >> 8) & 0xFF);
	id[2] = (uint8_t)((value >> 16) & 0xFF);

	return 0;
}

static void flash_flexspi_nand_parse_onfi_fields(struct flash_flexspi_nand_data *nand,
						 const struct nand_onfi_parameter_page_raw *onfi,
						 uint16_t crc)
{
	char manufacturer[13];
	char model[21];
	uint64_t onfi_size;

	memcpy(manufacturer, &onfi->data[32], 12);
	manufacturer[12] = '\0';
	memcpy(model, &onfi->data[44], 20);
	model[20] = '\0';

	LOG_DBG("NAND probe OK (ONFI CRC %04X)", crc);
	LOG_DBG("Manufacturer: %s", manufacturer);
	LOG_DBG("Model: %s", model);

	nand->page_size = sys_get_le32(&onfi->data[80]);
	nand->oob_size = sys_get_le16(&onfi->data[84]);
	nand->pages_per_block = sys_get_le32(&onfi->data[92]);
	nand->blocks_per_unit = sys_get_le32(&onfi->data[96]);
	nand->units = onfi->data[100];
	nand->block_size = nand->page_size * nand->pages_per_block;

	LOG_DBG("Page size (data): %u", nand->page_size);
	LOG_DBG("Page size (spare/OOB): %u", nand->oob_size);
	LOG_DBG("Pages per block: %u", nand->pages_per_block);
	LOG_DBG("Blocks per unit: %u", nand->blocks_per_unit);
	LOG_DBG("Units: %u", nand->units);

	/* Keep DT size as the authoritative limit, but log if it disagrees with ONFI. */
	if ((nand->page_size != 0U) && (nand->pages_per_block != 0U) &&
	    (nand->blocks_per_unit != 0U) && (nand->units != 0U)) {
		onfi_size = (uint64_t)nand->page_size * (uint64_t)nand->pages_per_block *
			    (uint64_t)nand->blocks_per_unit * (uint64_t)nand->units;
		if ((onfi_size != 0U) && (nand->size != onfi_size)) {
			LOG_WRN("DT size (%llu) != ONFI size (%llu)", nand->size, onfi_size);
		}
	}
}

static int flash_flexspi_nand_onfi_read(const struct device *dev)
{
	struct nand_onfi_parameter_page_raw onfi;
	uint16_t computed_crc;
	uint8_t cfg;
	uint8_t cfg_restore;
	struct flash_flexspi_nand_data *nand = dev->data;
	int ret, ret_temp;

	ret = flash_flexspi_nand_read_reg(dev, SPI_NAND_REG_CONFIG, &cfg);
	if (ret != 0) {
		return ret;
	}

	cfg_restore = cfg;

	/* Enable OTP/parameter page access. */
	cfg |= SPI_NAND_CFG_OTP_EN;
	ret = flash_flexspi_nand_write_reg(dev, SPI_NAND_REG_CONFIG, cfg);
	if (ret != 0) {
		return ret;
	}

	ret = flash_flexspi_nand_wait_busy(dev, CONFIG_FLASH_MCUX_FLEXSPI_NAND_TIMEOUT_US);
	if (ret != 0) {
		return ret;
	}

	/* Load ONFI parameter page. */
	ret = flash_flexspi_nand_xfer(dev, PAGE_READ, kFLEXSPI_Command, 2048, NULL, 0);
	if (ret != 0) {
		goto restore;
	}

	ret = flash_flexspi_nand_wait_busy(dev, CONFIG_FLASH_MCUX_FLEXSPI_NAND_TIMEOUT_US);
	if (ret != 0) {
		goto restore;
	}

	memset(&onfi, 0, sizeof(onfi));
	ret = flash_flexspi_nand_xfer(dev, READ_CACHE, kFLEXSPI_Read, 2048, &onfi, sizeof(onfi));
	if (ret != 0) {
		goto restore;
	}

	if (memcmp(&onfi.data[0], "ONFI", 4) != 0) {
		ret = -EINVAL;
		goto restore;
	}

	computed_crc = crc16(CRC16_POLY, 0x4F4E, (void *)&onfi, sizeof(onfi) - sizeof(uint16_t));
	if (computed_crc != sys_get_le16(&onfi.data[254])) {
		ret = -EIO;
		goto restore;
	}

	flash_flexspi_nand_parse_onfi_fields(nand, &onfi, computed_crc);

restore:
	/* Restore config register, clear OTP_EN. */
	ret_temp = flash_flexspi_nand_write_reg(dev, SPI_NAND_REG_CONFIG,
						cfg_restore & ~SPI_NAND_CFG_OTP_EN);
	if (ret == 0) {
		ret = ret_temp;
	}
	ret_temp = flash_flexspi_nand_wait_busy(dev, CONFIG_FLASH_MCUX_FLEXSPI_NAND_TIMEOUT_US);
	if (ret == 0) {
		ret = ret_temp;
	}
	return ret;
}

static int flash_flexspi_nand_enable_ecc(const struct device *dev)
{
	struct flash_flexspi_nand_data *nand = dev->data;
	uint8_t cfg;
	int ret;

	ret = flash_flexspi_nand_read_reg(dev, SPI_NAND_REG_CONFIG, &cfg);
	if (ret != 0) {
		return ret;
	}

	if ((cfg & SPI_NAND_CFG_ECC_EN) != 0U) {
		return 0;
	}

	cfg |= SPI_NAND_CFG_ECC_EN;

	ret = flash_flexspi_nand_write_reg(dev, SPI_NAND_REG_CONFIG, cfg);
	if (ret != 0) {
		return ret;
	}

	ret = flash_flexspi_nand_wait_busy(dev, CONFIG_FLASH_MCUX_FLEXSPI_NAND_TIMEOUT_US);
	if (ret != 0) {
		return ret;
	}

	memc_flexspi_reset(nand->controller);

	return 0;
}

static int flash_flexspi_nand_check_ecc(const struct device *dev)
{
	uint8_t status;
	uint8_t ecc;
	int ret;

	ret = flash_flexspi_nand_read_reg(dev, SPI_NAND_REG_STATUS, &status);
	if (ret != 0) {
		return ret;
	}

	ecc = (status & SPI_NAND_STATUS_ECC_MASK) >> SPI_NAND_STATUS_ECC_SHIFT;

	if (ecc == 0x3U) {
		LOG_ERR("Uncorrectable ECC error (STATUS=0x%02x)", status);
		return -EIO;
	}
	if (ecc != 0U) {
		LOG_WRN("ECC corrected bitflips (STATUS=0x%02x)", status);
	}

	return 0;
}

static int flash_flexspi_nand_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct flash_flexspi_nand_data *nand = dev->data;
	uint32_t off = (uint32_t)offset;
	uint8_t *buffer = data;
	size_t chunk;
	int ret = 0;

	if (len == 0U) {
		return 0;
	}

	if ((data == NULL) || (!area_is_subregion(dev, offset, len))) {
		return -EINVAL;
	}

	k_mutex_lock(&nand->lock, K_FOREVER);

	while (len > 0U) {
		chunk = MIN(len, (size_t)(nand->page_size - (off % nand->page_size)));

		ret = flash_flexspi_nand_xfer(dev, PAGE_READ, kFLEXSPI_Command, off, NULL, 0);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_nand_wait_busy(dev, CONFIG_FLASH_MCUX_FLEXSPI_NAND_TIMEOUT_US);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_nand_check_ecc(dev);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_nand_xfer(dev, READ_CACHE, kFLEXSPI_Read, off, buffer, chunk);
		if (ret != 0) {
			goto out;
		}

		buffer += chunk;
		off += chunk;
		len -= chunk;
	}

out:
	k_mutex_unlock(&nand->lock);
	return ret;
}

static int flash_flexspi_nand_write(const struct device *dev, off_t offset, const void *data,
				    size_t len)
{
	struct flash_flexspi_nand_data *nand = dev->data;
	uint32_t off = (uint32_t)offset;
	const uint8_t *buffer = data;
	uint8_t status;
	size_t chunk;
	int ret = 0;

	if (len == 0U) {
		return 0;
	}

	if ((data == NULL) || (!area_is_subregion(dev, offset, len))) {
		return -EINVAL;
	}

	if (((off % nand->page_size) != 0U) || ((len % nand->page_size) != 0U)) {
		return -EINVAL;
	}

	k_mutex_lock(&nand->lock, K_FOREVER);

	while (len > 0U) {
		chunk = nand->page_size;

		ret = flash_flexspi_nand_write_enable(dev);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_nand_xfer(dev, PROG_LOAD, kFLEXSPI_Write, off, (void *)buffer,
					      chunk);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_nand_xfer(dev, PROG_EXEC, kFLEXSPI_Command, off, NULL, 0);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_nand_wait_busy(dev, CONFIG_FLASH_MCUX_FLEXSPI_NAND_TIMEOUT_US);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_nand_read_reg(dev, SPI_NAND_REG_STATUS, &status);
		if (ret != 0) {
			goto out;
		}
		if ((status & SPI_NAND_STATUS_P_FAIL) != 0U) {
			ret = -EIO;
			goto out;
		}

		off += chunk;
		buffer += chunk;
		len -= chunk;
	}

out:
	k_mutex_unlock(&nand->lock);
	return ret;
}

static int flash_flexspi_nand_erase(const struct device *dev, off_t offset, size_t size)
{
	const uint32_t lut_block_erase[MEMC_FLEXSPI_CMD_PER_SEQ] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_BLOCK_ERASE,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
	};
	struct flash_flexspi_nand_data *nand = dev->data;
	uint32_t block_size = nand->block_size;
	uint32_t off = (uint32_t)offset;
	uint8_t status;
	int ret = 0;

	if (size == 0U) {
		return 0;
	}

	if (!area_is_subregion(dev, offset, size)) {
		return -EINVAL;
	}

	if (((off % block_size) != 0U) || ((size % block_size) != 0U)) {
		return -EINVAL;
	}

	k_mutex_lock(&nand->lock, K_FOREVER);

	while (size > 0U) {
		ret = flash_flexspi_nand_write_enable(dev);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_update_lut(dev, lut_block_erase);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_nand_xfer(dev, DYNAMIC_SEQ, kFLEXSPI_Command, off, NULL, 0);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_nand_wait_busy(dev, CONFIG_FLASH_MCUX_FLEXSPI_NAND_TIMEOUT_US);
		if (ret != 0) {
			goto out;
		}

		ret = flash_flexspi_nand_read_reg(dev, SPI_NAND_REG_STATUS, &status);
		if (ret != 0) {
			goto out;
		}
		if ((status & SPI_NAND_STATUS_E_FAIL) != 0U) {
			ret = -EIO;
			goto out;
		}

		off += block_size;
		size -= block_size;
	}

out:
	k_mutex_unlock(&nand->lock);
	return ret;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_flexspi_nand_page_layout(const struct device *dev,
					   const struct flash_pages_layout **layout,
					   size_t *layout_size)
{
	struct flash_flexspi_nand_data *nand = dev->data;

	*layout = &nand->layout;
	*layout_size = 1;
}
#endif

#if defined(CONFIG_FLASH_EX_OP_ENABLED)
/*
 * Read OOB byte 0 of the page currently in cache.
 *
 * column-space = 11 → CADDR can only address columns 0–2047.
 * OOB starts at column 2048 (0x0800), so we embed CA[15:8] and CA[7:0] as
 * literal SDR bytes in a dynamic LUT instead of using CADDR.
 *
 * Sequence: READ_CACHE_X4 (0x6B) | CA[15:8]=0x08 | CA[7:0]=0x00 | Dummy(8) | Data
 */
static int flash_flexspi_nand_read_oob0(const struct device *dev, uint8_t *byte)
{
	const uint32_t lut_read_oob[MEMC_FLEXSPI_CMD_PER_SEQ] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_READ_CACHE_X4,
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_OOB_COLUMN_HI),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_OOB_COLUMN_LO,
				kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 8),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x04U,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
		0,
	};
	uint32_t buf = 0;
	int ret;

	ret = flash_flexspi_update_lut(dev, lut_read_oob);
	if (ret != 0) {
		return ret;
	}

	/* deviceAddress is unused (column is hardcoded in LUT via SDR bytes) */
	ret = flash_flexspi_nand_xfer(dev, DYNAMIC_SEQ, kFLEXSPI_Read, 0, &buf, 1);
	if (ret != 0) {
		return ret;
	}

	*byte = (uint8_t)buf;
	return 0;
}

/*
 * Write 0x00 to OOB byte 0 of the first page of a block (bad-block marker).
 *
 * Uses PROGRAM LOAD x1 (0x02) at column 2048 with a single marker byte,
 * then PROGRAM EXECUTE.  All bytes not loaded default to 0xFF in the internal
 * page buffer, so the data area is written as all-0xFF (erased state).
 *
 * Sequence: PROG_LOAD_X1 (0x02) | CA[15:8]=0x08 | CA[7:0]=0x00 | Marker(1B)
 *           PROG_EXEC | row_addr
 */
static int flash_flexspi_nand_mark_oob0(const struct device *dev, off_t block_addr)
{
	const uint32_t lut_prog_oob[MEMC_FLEXSPI_CMD_PER_SEQ] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_CMD_PROG_LOAD_X1,
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_OOB_COLUMN_HI),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NAND_OOB_COLUMN_LO,
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x04U),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0, kFLEXSPI_Command_STOP,
				kFLEXSPI_1PAD, 0),
		0,
	};
	uint8_t marker = 0x00U;
	uint8_t status;
	int ret;

	ret = flash_flexspi_nand_write_enable(dev);
	if (ret != 0) {
		return ret;
	}

	ret = flash_flexspi_update_lut(dev, lut_prog_oob);
	if (ret != 0) {
		return ret;
	}

	ret = flash_flexspi_nand_xfer(dev, DYNAMIC_SEQ, kFLEXSPI_Write, 0, &marker, 1);
	if (ret != 0) {
		return ret;
	}

	ret = flash_flexspi_nand_xfer(dev, PROG_EXEC, kFLEXSPI_Command, (uint32_t)block_addr, NULL,
				      0);
	if (ret != 0) {
		return ret;
	}

	ret = flash_flexspi_nand_wait_busy(dev, CONFIG_FLASH_MCUX_FLEXSPI_NAND_TIMEOUT_US);
	if (ret != 0) {
		return ret;
	}

	ret = flash_flexspi_nand_read_reg(dev, SPI_NAND_REG_STATUS, &status);
	if (ret != 0) {
		return ret;
	}

	/* P_FAIL is best-effort: log but don't fail — block is unusable anyway */
	if ((status & SPI_NAND_STATUS_P_FAIL) != 0U) {
		LOG_WRN("bad-block mark at 0x%08lx: P_FAIL set", (long)block_addr);
	}

	return 0;
}

static int flash_flexspi_nand_ex_op(const struct device *dev, uint16_t code, const uintptr_t in,
				    void *out)
{
	struct flash_flexspi_nand_data *nand = dev->data;
	int ret;

	k_mutex_lock(&nand->lock, K_FOREVER);

	switch (code) {
	case FLASH_EX_OP_RESET:
		ret = flash_flexspi_nand_reset(dev);
		break;

	case FLASH_EX_OP_IS_BAD_BLOCK: {
		/*
		 * W25N01GV bad-block convention: factory-bad blocks have
		 * OOB byte 0 of the first page set to a non-0xFF value.
		 * Read OOB[0] by doing PAGE_READ (load page into cache) then
		 * READ_CACHE at column 2048.
		 */
		const off_t block_addr = *(const off_t *)in;
		uint8_t oob0;

		ret = flash_flexspi_nand_xfer(dev, PAGE_READ, kFLEXSPI_Command,
					      (uint32_t)block_addr, NULL, 0);
		if (ret != 0) {
			break;
		}

		ret = flash_flexspi_nand_wait_busy(dev, CONFIG_FLASH_MCUX_FLEXSPI_NAND_TIMEOUT_US);
		if (ret != 0) {
			break;
		}

		ret = flash_flexspi_nand_read_oob0(dev, &oob0);
		if (ret != 0) {
			break;
		}

		*(int *)out = (oob0 != 0xFFU) ? FLASH_BLOCK_BAD : FLASH_BLOCK_GOOD;
		break;
	}

	case FLASH_EX_OP_MARK_BAD_BLOCK: {
		/*
		 * Program 0x00 to OOB byte 0 of the first page of the block.
		 * This is a best-effort operation: if the block failed to erase,
		 * the program may also fail, but we log and continue so dhara
		 * can work around it through its journal mechanism.
		 */
		const off_t block_addr = *(const off_t *)in;

		ret = flash_flexspi_nand_mark_oob0(dev, block_addr);
		break;
	}

	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&nand->lock);
	return ret;
}
#endif /* CONFIG_FLASH_EX_OP_ENABLED */

static const struct flash_parameters *flash_flexspi_nand_get_parameters(const struct device *dev)
{
	struct flash_flexspi_nand_data *nand = dev->data;

	return &nand->flash_parameters;
}

static int flash_flexspi_nand_get_size(const struct device *dev, uint64_t *size)
{
	struct flash_flexspi_nand_data *nand = dev->data;

	*size = nand->size;
	return 0;
}

static int flash_flexspi_nand_init(const struct device *dev)
{
	struct flash_flexspi_nand_data *nand = dev->data;
	uint8_t id[3];
	int ret;

	k_mutex_init(&nand->lock);

	if (!device_is_ready(nand->controller)) {
		LOG_ERR("FlexSPI controller not ready");
		return -ENODEV;
	}

	ret = memc_flexspi_set_device_config(
		nand->controller, &nand->config, (const uint32_t *)flexspi_lut,
		sizeof(flexspi_lut) / MEMC_FLEXSPI_CMD_SIZE, nand->port);
	if (ret != 0) {
		LOG_ERR("Failed to configure FlexSPI NAND (%d)", ret);
		return ret;
	}

	/* Apply FlexSPI pinmux for the NAND port. */
	ret = memc_flexspi_apply_pinctrl(nand->controller, PINCTRL_STATE_DEFAULT);
	if (ret != 0 && ret != -ENOENT) {
		LOG_ERR("Failed to apply FlexSPI pinctrl (%d)", ret);
		return ret;
	}

	memc_flexspi_reset(nand->controller);

	ret = flash_flexspi_nand_reset(dev);
	if (ret != 0) {
		LOG_ERR("NAND reset failed (%d)", ret);
		return ret;
	}

	/* Clear all block protection bits for flash_shell convenience. */
	ret = flash_flexspi_nand_set_block_protect(dev, 0x00);
	if (ret != 0) {
		LOG_WRN("Failed to clear block protect (%d)", ret);
	}

	ret = flash_flexspi_nand_read_id(dev, id);
	if (ret != 0) {
		LOG_ERR("NAND read-id failed (%d)", ret);
		return ret;
	}
	LOG_DBG("JEDEC ID (raw): %02X %02X %02X", id[0], id[1], id[2]);

	ret = flash_flexspi_nand_onfi_read(dev);
	if (ret != 0) {
		LOG_ERR("ONFI parameter read/CRC failed (%d)", ret);
		return ret;
	}

	ret = flash_flexspi_nand_enable_ecc(dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable on-die ECC (%d)", ret);
		return ret;
	}

	if (nand->page_size == 0U) {
		LOG_ERR("ONFI did not provide a valid page size");
		return -EIO;
	}
	if ((nand->pages_per_block == 0U) || (nand->block_size == 0U)) {
		LOG_ERR("ONFI did not provide a valid block geometry");
		return -EIO;
	}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	nand->layout.pages_size = nand->block_size;
	nand->layout.pages_count = (size_t)(nand->size / nand->block_size);
#endif

	return 0;
}

static DEVICE_API(flash, flash_flexspi_nand_api) = {
	.erase = flash_flexspi_nand_erase,
	.write = flash_flexspi_nand_write,
	.read = flash_flexspi_nand_read,
	.get_parameters = flash_flexspi_nand_get_parameters,
	.get_size = flash_flexspi_nand_get_size,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_flexspi_nand_page_layout,
#endif
#if defined(CONFIG_FLASH_EX_OP_ENABLED)
	.ex_op = flash_flexspi_nand_ex_op,
#endif
};

#define CS_INTERVAL_UNIT(unit)    UTIL_CAT(UTIL_CAT(kFLEXSPI_CsIntervalUnit, unit), SckCycle)
#define AHB_WRITE_WAIT_UNIT(unit) UTIL_CAT(UTIL_CAT(kFLEXSPI_AhbWriteWaitUnit, unit), AhbCycle)

#define FLASH_FLEXSPI_DEVICE_CONFIG(n)                                                             \
	{                                                                                          \
		.flexspiRootClk = DT_INST_PROP(n, spi_max_frequency),                              \
		.flashSize = DT_INST_PROP(n, size) / 8 / KB(1),                                    \
		.CSIntervalUnit = CS_INTERVAL_UNIT(DT_INST_PROP(n, cs_interval_unit)),             \
		.CSInterval = DT_INST_PROP(n, cs_interval),                                        \
		.CSHoldTime = DT_INST_PROP(n, cs_hold_time),                                       \
		.CSSetupTime = DT_INST_PROP(n, cs_setup_time),                                     \
		.dataValidTime = DT_INST_PROP(n, data_valid_time),                                 \
		.columnspace = DT_INST_PROP(n, column_space),                                      \
		.enableWordAddress = DT_INST_PROP(n, word_addressable),                            \
		.AHBWriteWaitUnit = AHB_WRITE_WAIT_UNIT(DT_INST_PROP(n, ahb_write_wait_unit)),     \
		.AHBWriteWaitInterval = DT_INST_PROP(n, ahb_write_wait_interval),                  \
	}

#define FLASH_FLEXSPI_NAND(n)                                                                      \
	static struct flash_flexspi_nand_data flash_flexspi_nand_data_##n = {                      \
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),                                       \
		.config = FLASH_FLEXSPI_DEVICE_CONFIG(n),                                          \
		.port = DT_INST_REG_ADDR(n),                                                       \
		.size = DT_INST_PROP(n, size) / 8,                                                 \
		.page_size = 0,                                                                    \
		.oob_size = 0,                                                                     \
		.pages_per_block = 0,                                                              \
		.blocks_per_unit = 0,                                                              \
		.units = 0,                                                                        \
		.flash_parameters =                                                                \
			{                                                                          \
				.write_block_size = DT_INST_PROP(n, write_block_size),            \
				.erase_value = 0xff,                                               \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, flash_flexspi_nand_init, NULL, &flash_flexspi_nand_data_##n,      \
			      NULL, POST_KERNEL,                                                   \
			      CONFIG_FLASH_MCUX_FLEXSPI_NAND_INIT_PRIORITY,                        \
			      &flash_flexspi_nand_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_FLEXSPI_NAND)
