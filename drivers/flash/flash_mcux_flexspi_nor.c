/*
 * Copyright 2020,2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_imx_flexspi_nor

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include "spi_nor.h"
#include "jesd216.h"
#include "memc_mcux_flexspi.h"

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#define NOR_WRITE_SIZE	1
#define NOR_ERASE_VALUE	0xff

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_NOR_WRITE_BUFFER
static uint8_t nor_write_buf[SPI_NOR_PAGE_SIZE];
#endif

/*
 * NOTE: If CONFIG_FLASH_MCUX_FLEXSPI_XIP is selected, Any external functions
 * called while interacting with the flexspi MUST be relocated to SRAM or ITCM
 * at runtime, so that the chip does not access the flexspi to read program
 * instructions while it is being written to
 *
 * Additionally, no data used by this driver should be stored in flash.
 */
#if defined(CONFIG_FLASH_MCUX_FLEXSPI_XIP) && (CONFIG_FLASH_LOG_LEVEL > 0)
#warning "Enabling flash driver logging and XIP mode simultaneously can cause \
	read-while-write hazards. This configuration is not recommended."
#endif

LOG_MODULE_REGISTER(flash_flexspi_nor, CONFIG_FLASH_LOG_LEVEL);

enum {
	READ,
	PAGE_PROGRAM,
	READ_STATUS,
	WRITE_ENABLE,
	ERASE_SECTOR,
	ERASE_BLOCK,
	READ_ID,
	READ_STATUS_REG,
	ERASE_CHIP,
	READ_JESD216,
	/* Entries after this should be for scratch commands */
	FLEXSPI_INSTR_PROG_END,
	/* Used for temporary commands during initialization */
	SCRATCH_CMD = FLEXSPI_INSTR_PROG_END,
	SCRATCH_CMD2,
	/* Must be last entry */
	FLEXSPI_INSTR_END,
};

struct flash_flexspi_nor_config {
	/* Note: don't use this controller reference in code. It is
	 * only used during init to copy the device structure from ROM
	 * into a RAM structure
	 */
	const struct device *controller;
};

/* Device variables used in critical sections should be in this structure */
struct flash_flexspi_nor_data {
	struct device controller;
	flexspi_device_config_t config;
	flexspi_port_t port;
	bool legacy_poll;
	struct flash_pages_layout layout;
	struct flash_parameters flash_parameters;
};

/*
 * FLEXSPI LUT buffer used during configuration. Stored in .data to avoid
 * using too much stack
 */
static uint32_t flexspi_probe_lut[FLEXSPI_INSTR_END][MEMC_FLEXSPI_CMD_PER_SEQ] = {0};

/* Initial LUT table */
static const uint32_t flash_flexspi_nor_base_lut[][MEMC_FLEXSPI_CMD_PER_SEQ] = {
	/* 1S-1S-1S flash read command, should be compatible with all SPI nor flashes */
	[READ] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_READ,
			kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 24),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x1,
			kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
	},
	[READ_JESD216] = {
		/* Install read SFDP command */
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, JESD216_CMD_READ_SFDP,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 24),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 8,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x4),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0),
	},
	/* Standard 1S-1S-1S flash write command, can be switched to 1S-1S-4S when QE is set */
	[PAGE_PROGRAM] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_PP,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x04,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
	},

	[WRITE_ENABLE] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_WREN,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
	},

	[ERASE_SECTOR] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_SE,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
	},

	[ERASE_BLOCK] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_BE,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
	},

	[ERASE_CHIP] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_CE,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0),
	},

	[READ_ID] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_RDID,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x01),
	},

	[READ_STATUS_REG] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_RDSR,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x01),
	},
};

/* Helper so we can read flash ID without flash access for XIP */
static int flash_flexspi_nor_read_id_helper(struct flash_flexspi_nor_data *data,
		uint8_t *vendor_id)
{
	uint32_t buffer = 0;
	int ret;

	flexspi_transfer_t transfer = {
		.deviceAddress = 0,
		.port = data->port,
		.cmdType = kFLEXSPI_Read,
		.SeqNumber = 1,
		.seqIndex = READ_ID,
		.data = &buffer,
		.dataSize = 3,
	};

	LOG_DBG("Reading id");

	ret = memc_flexspi_transfer(&data->controller, &transfer);
	if (ret < 0) {
		return ret;
	}

	memcpy(vendor_id, &buffer, 3);

	return ret;
}

static int flash_flexspi_nor_read_id(const struct device *dev, uint8_t *vendor_id)
{
	struct flash_flexspi_nor_data *data = dev->data;

	return flash_flexspi_nor_read_id_helper(data, vendor_id);
}

static int flash_flexspi_nor_read_status(struct flash_flexspi_nor_data *data,
		uint32_t *status)
{
	flexspi_transfer_t transfer = {
		.deviceAddress = 0,
		.port = data->port,
		.cmdType = kFLEXSPI_Read,
		.SeqNumber = 1,
		.seqIndex = READ_STATUS_REG,
		.data = status,
		.dataSize = 1,
	};

	LOG_DBG("Reading status register");

	return memc_flexspi_transfer(&data->controller, &transfer);
}

static int flash_flexspi_nor_write_enable(struct flash_flexspi_nor_data *data)
{
	flexspi_transfer_t transfer = {
		.deviceAddress = 0,
		.port = data->port,
		.cmdType = kFLEXSPI_Command,
		.SeqNumber = 1,
		.seqIndex = WRITE_ENABLE,
		.data = NULL,
		.dataSize = 0,
	};

	LOG_DBG("Enabling write");

	return memc_flexspi_transfer(&data->controller, &transfer);
}

static int flash_flexspi_nor_erase_sector(struct flash_flexspi_nor_data *data,
	off_t offset)
{
	flexspi_transfer_t transfer = {
		.deviceAddress = offset,
		.port = data->port,
		.cmdType = kFLEXSPI_Command,
		.SeqNumber = 1,
		.seqIndex = ERASE_SECTOR,
		.data = NULL,
		.dataSize = 0,
	};

	LOG_DBG("Erasing sector at 0x%08zx", (ssize_t) offset);

	return memc_flexspi_transfer(&data->controller, &transfer);
}

static int flash_flexspi_nor_erase_block(struct flash_flexspi_nor_data *data,
					  off_t offset)
{
	flexspi_transfer_t transfer = {
		.deviceAddress = offset,
		.port = data->port,
		.cmdType = kFLEXSPI_Command,
		.SeqNumber = 1,
		.seqIndex = ERASE_BLOCK,
		.data = NULL,
		.dataSize = 0,
	};

	LOG_DBG("Erasing block at 0x%08zx", (ssize_t) offset);

	return memc_flexspi_transfer(&data->controller, &transfer);
}

static int flash_flexspi_nor_erase_chip(struct flash_flexspi_nor_data *data)
{
	flexspi_transfer_t transfer = {
		.deviceAddress = 0,
		.port = data->port,
		.cmdType = kFLEXSPI_Command,
		.SeqNumber = 1,
		.seqIndex = ERASE_CHIP,
		.data = NULL,
		.dataSize = 0,
	};

	LOG_DBG("Erasing chip");

	return memc_flexspi_transfer(&data->controller, &transfer);
}

static int flash_flexspi_nor_page_program(struct flash_flexspi_nor_data *data,
		off_t offset, const void *buffer, size_t len)
{
	flexspi_transfer_t transfer = {
		.deviceAddress = offset,
		.port = data->port,
		.cmdType = kFLEXSPI_Write,
		.SeqNumber = 1,
		.seqIndex = PAGE_PROGRAM,
		.data = (uint32_t *) buffer,
		.dataSize = len,
	};

	LOG_DBG("Page programming %d bytes to 0x%08zx", len, (ssize_t) offset);

	return memc_flexspi_transfer(&data->controller, &transfer);
}

static int flash_flexspi_nor_wait_bus_busy(struct flash_flexspi_nor_data *data)
{
	uint32_t status = 0;
	int ret;

	while (1) {
		ret = flash_flexspi_nor_read_status(data, &status);
		LOG_DBG("status: 0x%x", status);
		if (ret) {
			LOG_ERR("Could not read status");
			return ret;
		}

		if (data->legacy_poll) {
			if ((status & BIT(0)) == 0) {
				break;
			}
		} else {
			if (status & BIT(7)) {
				break;
			}
		}
	}

	return 0;
}

static int flash_flexspi_nor_read(const struct device *dev, off_t offset,
		void *buffer, size_t len)
{
	struct flash_flexspi_nor_data *data = dev->data;
	uint8_t *src = memc_flexspi_get_ahb_address(&data->controller,
						    data->port,
						    offset);

	memcpy(buffer, src, len);

	return 0;
}

static int flash_flexspi_nor_write(const struct device *dev, off_t offset,
		const void *buffer, size_t len)
{
	struct flash_flexspi_nor_data *data = dev->data;
	size_t size = len;
	uint8_t *src = (uint8_t *) buffer;
	int i;
	unsigned int key = 0;

	uint8_t *dst = memc_flexspi_get_ahb_address(&data->controller,
						    data->port,
						    offset);

	if (memc_flexspi_is_running_xip(&data->controller)) {
		/*
		 * ==== ENTER CRITICAL SECTION ====
		 * No flash access should be performed in critical section. All
		 * code and data accessed must reside in ram.
		 */
		key = irq_lock();
	}

	while (len) {
		/* If the offset isn't a multiple of the NOR page size, we first need
		 * to write the remaining part that fits, otherwise the write could
		 * be wrapped around within the same page
		 */
		i = MIN(SPI_NOR_PAGE_SIZE - (offset % SPI_NOR_PAGE_SIZE), len);
#ifdef CONFIG_FLASH_MCUX_FLEXSPI_NOR_WRITE_BUFFER
		memcpy(nor_write_buf, src, i);
#endif
		flash_flexspi_nor_write_enable(data);
#ifdef CONFIG_FLASH_MCUX_FLEXSPI_NOR_WRITE_BUFFER
		flash_flexspi_nor_page_program(data, offset, nor_write_buf, i);
#else
		flash_flexspi_nor_page_program(data, offset, src, i);
#endif
		flash_flexspi_nor_wait_bus_busy(data);
		memc_flexspi_reset(&data->controller);
		src += i;
		offset += i;
		len -= i;
	}

	if (memc_flexspi_is_running_xip(&data->controller)) {
		/* ==== EXIT CRITICAL SECTION ==== */
		irq_unlock(key);
	}

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_InvalidateByRange((uint32_t) dst, size);
#endif

	return 0;
}

static int flash_flexspi_nor_erase(const struct device *dev, off_t offset,
		size_t size)
{
	struct flash_flexspi_nor_data *data = dev->data;
	const size_t num_sectors = size / SPI_NOR_SECTOR_SIZE;
	const size_t num_blocks = size / SPI_NOR_BLOCK_SIZE;

	int i;
	unsigned int key = 0;

	uint8_t *dst = memc_flexspi_get_ahb_address(&data->controller,
						    data->port,
						    offset);

	if (offset % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid offset");
		return -EINVAL;
	}

	if (size % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}

	if (memc_flexspi_is_running_xip(&data->controller)) {
		/*
		 * ==== ENTER CRITICAL SECTION ====
		 * No flash access should be performed in critical section. All
		 * code and data accessed must reside in ram.
		 */
		key = irq_lock();
	}

	if ((offset == 0) && (size == data->config.flashSize * KB(1))) {
		flash_flexspi_nor_write_enable(data);
		flash_flexspi_nor_erase_chip(data);
		flash_flexspi_nor_wait_bus_busy(data);
		memc_flexspi_reset(&data->controller);
	} else if ((0 == (offset % SPI_NOR_BLOCK_SIZE)) && (0 == (size % SPI_NOR_BLOCK_SIZE))) {
		for (i = 0; i < num_blocks; i++) {
			flash_flexspi_nor_write_enable(data);
			flash_flexspi_nor_erase_block(data, offset);
			flash_flexspi_nor_wait_bus_busy(data);
			memc_flexspi_reset(&data->controller);
			offset += SPI_NOR_BLOCK_SIZE;
		}
	} else {
		for (i = 0; i < num_sectors; i++) {
			flash_flexspi_nor_write_enable(data);
			flash_flexspi_nor_erase_sector(data, offset);
			flash_flexspi_nor_wait_bus_busy(data);
			memc_flexspi_reset(&data->controller);
			offset += SPI_NOR_SECTOR_SIZE;
		}
	}

	if (memc_flexspi_is_running_xip(&data->controller)) {
		/* ==== EXIT CRITICAL SECTION ==== */
		irq_unlock(key);
	}

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_InvalidateByRange((uint32_t) dst, size);
#endif

	return 0;
}

static const struct flash_parameters *flash_flexspi_nor_get_parameters(
		const struct device *dev)
{
	struct flash_flexspi_nor_data *data = dev->data;

	return &data->flash_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_flexspi_nor_pages_layout(const struct device *dev,
		const struct flash_pages_layout **layout, size_t *layout_size)
{
	struct flash_flexspi_nor_data *data = dev->data;

	*layout = &data->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */


/*
 * This function enables quad mode, when supported. Otherwise it
 * returns an error.
 * @param dev: Flexspi device
 * @param flexspi_lut: flexspi lut table, useful if instruction writes are needed
 * @param qer: DW15 quad enable parameter
 * @return 0 if quad mode was entered, or -ENOTSUP if quad mode is not supported
 */
static int flash_flexspi_nor_quad_enable(struct flash_flexspi_nor_data *data,
					uint32_t (*flexspi_lut)[MEMC_FLEXSPI_CMD_PER_SEQ],
					uint8_t qer)
{
	int ret;
	uint32_t buffer = 0;
	uint16_t bit = 0;
	uint8_t rd_size, wr_size;
	flexspi_transfer_t transfer = {
		.deviceAddress = 0,
		.port = data->port,
		.SeqNumber = 1,
		.data = &buffer,
	};
	flexspi_device_config_t config = {
		.flexspiRootClk = MHZ(50),
		.flashSize = FLEXSPI_FLSHCR0_FLSHSZ_MASK, /* Max flash size */
		.ARDSeqNumber = 1,
		.ARDSeqIndex = READ,
	};

	switch (qer) {
	case JESD216_DW15_QER_VAL_NONE:
		/* No init needed */
		return 0;
	case JESD216_DW15_QER_VAL_S2B1v1:
	case JESD216_DW15_QER_VAL_S2B1v4:
		/* Install read and write status command */
		flexspi_lut[SCRATCH_CMD][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_RDSR,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x1);
		flexspi_lut[SCRATCH_CMD2][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_WRSR,
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x1);

		/* Set bit 1 of status register 2 */
		bit = BIT(9);
		rd_size = 2;
		wr_size = 2;
		break;
	case JESD216_DW15_QER_VAL_S1B6:
		/* Install read and write status command */
		flexspi_lut[SCRATCH_CMD][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_RDSR,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x1);
		flexspi_lut[SCRATCH_CMD2][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_WRSR,
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x1);

		/* Set bit 6 of status register 1 */
		bit = BIT(6);
		rd_size = 1;
		wr_size = 1;
		break;
	case JESD216_DW15_QER_VAL_S2B7:
		/* Install read and write status command */
		flexspi_lut[SCRATCH_CMD][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x3F,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x1);
		flexspi_lut[SCRATCH_CMD2][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x3E,
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x1);

		/* Set bit 7 of status register 2 */
		bit = BIT(7);
		rd_size = 1;
		wr_size = 1;
		break;
	case JESD216_DW15_QER_VAL_S2B1v5:
		/* Install read and write status command */
		flexspi_lut[SCRATCH_CMD][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_RDSR2,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x1);
		flexspi_lut[SCRATCH_CMD2][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_WRSR,
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x1);

		/* Set bit 1 of status register 2 */
		bit = BIT(9);
		rd_size = 1;
		wr_size = 2;
		break;
	case JESD216_DW15_QER_VAL_S2B1v6:
		/* Install read and write status command */
		flexspi_lut[SCRATCH_CMD][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_RDSR2,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x1);
		flexspi_lut[SCRATCH_CMD2][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_WRSR2,
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x1);

		/* Set bit 7 of status register 2 */
		bit = BIT(7);
		rd_size = 1;
		wr_size = 1;
		break;
	default:
		return -ENOTSUP;
	}
	ret = memc_flexspi_set_device_config(&data->controller,
				&config,
				(uint32_t *)flexspi_lut,
				FLEXSPI_INSTR_END * MEMC_FLEXSPI_CMD_PER_SEQ,
				data->port);
	if (ret < 0) {
		return ret;
	}
	transfer.dataSize = rd_size;
	transfer.seqIndex = SCRATCH_CMD;
	transfer.cmdType = kFLEXSPI_Read;
	/* Read status register */
	ret = memc_flexspi_transfer(&data->controller, &transfer);
	if (ret < 0) {
		return ret;
	}
	/* Enable write */
	ret = flash_flexspi_nor_write_enable(data);
	if (ret < 0) {
		return ret;
	}
	if (qer == JESD216_DW15_QER_VAL_S2B1v5) {
		/* Left shift buffer by a byte */
		buffer = buffer << 8;
	}
	buffer |= bit;
	transfer.dataSize = wr_size;
	transfer.seqIndex = SCRATCH_CMD2;
	transfer.cmdType = kFLEXSPI_Write;
	ret = memc_flexspi_transfer(&data->controller, &transfer);
	if (ret < 0) {
		return ret;
	}

	/* Wait for QE bit to complete programming */
	return flash_flexspi_nor_wait_bus_busy(data);
}

/*
 * This function enables 4 byte addressing, when supported. Otherwise it
 * returns an error.
 * @param dev: Flexspi device
 * @param flexspi_lut: flexspi lut table, useful if instruction writes are needed
 * @param en4b: DW16 enable 4 byte mode parameter
 * @return 0 if 4 byte mode was entered, or -ENOTSUP if 4 byte mode was not supported
 */
static int flash_flexspi_nor_4byte_enable(struct flash_flexspi_nor_data *data,
					uint32_t (*flexspi_lut)[MEMC_FLEXSPI_CMD_PER_SEQ],
					uint32_t en4b)
{
	int ret;
	uint32_t buffer = 0;
	flexspi_transfer_t transfer = {
		.deviceAddress = 0,
		.port = data->port,
		.SeqNumber = 1,
		.data = &buffer,
	};
	flexspi_device_config_t config = {
		.flexspiRootClk = MHZ(50),
		.flashSize = FLEXSPI_FLSHCR0_FLSHSZ_MASK, /* Max flash size */
		.ARDSeqNumber = 1,
		.ARDSeqIndex = READ,
	};
	if (en4b & BIT(6)) {
		/* Flash is always in 4 byte mode. We just need to configure LUT */
		return 0;
	} else if (en4b & BIT(5)) {
		/* Dedicated vendor instruction set, which we don't support. Exit here */
		return -ENOTSUP;
	} else if (en4b & BIT(4)) {
		/* Set bit 0 of 16 bit configuration register */
		flexspi_lut[SCRATCH_CMD][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xB5,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x1);
		flexspi_lut[SCRATCH_CMD2][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xB1,
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x1);
		ret = memc_flexspi_set_device_config(&data->controller,
					&config,
					(uint32_t *)flexspi_lut,
					FLEXSPI_INSTR_END * MEMC_FLEXSPI_CMD_PER_SEQ,
					data->port);
		if (ret < 0) {
			return ret;
		}
		transfer.dataSize = 2;
		transfer.seqIndex = SCRATCH_CMD;
		transfer.cmdType = kFLEXSPI_Read;
		/* Read config register */
		ret = memc_flexspi_transfer(&data->controller, &transfer);
		if (ret < 0) {
			return ret;
		}
		buffer |= BIT(0);
		/* Set config register */
		transfer.seqIndex = SCRATCH_CMD2;
		transfer.cmdType = kFLEXSPI_Read;
		return memc_flexspi_transfer(&data->controller, &transfer);
	} else if (en4b & BIT(1)) {
		/* Issue write enable, then instruction 0xB7 */
		flash_flexspi_nor_write_enable(data);
		flexspi_lut[SCRATCH_CMD][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xB7,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0);
		ret = memc_flexspi_set_device_config(&data->controller,
					&config,
					(uint32_t *)flexspi_lut,
					FLEXSPI_INSTR_END * MEMC_FLEXSPI_CMD_PER_SEQ,
					data->port);
		if (ret < 0) {
			return ret;
		}
		transfer.dataSize = 0;
		transfer.seqIndex = SCRATCH_CMD;
		transfer.cmdType = kFLEXSPI_Command;
		return memc_flexspi_transfer(&data->controller, &transfer);
	} else if (en4b & BIT(0)) {
		/* Issue instruction 0xB7 */
		flexspi_lut[SCRATCH_CMD][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xB7,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0);
		ret = memc_flexspi_set_device_config(&data->controller,
					&config,
					(uint32_t *)flexspi_lut,
					FLEXSPI_INSTR_END * MEMC_FLEXSPI_CMD_PER_SEQ,
					data->port);
		if (ret < 0) {
			return ret;
		}
		transfer.dataSize = 0;
		transfer.seqIndex = SCRATCH_CMD;
		transfer.cmdType = kFLEXSPI_Command;
		return memc_flexspi_transfer(&data->controller, &transfer);
	}
	/* Other methods not supported */
	return -ENOTSUP;
}

/*
 * This function configures the FlexSPI to manage the flash device
 * based on values in SFDP header
 * @param data: Flexspi device data
 * @param header: SFDP header for flash
 * @param bfp: basic flash parameters for flash
 * @param flexspi_lut: LUT table, filled with READ LUT command
 * @return 0 on success, or negative value on error
 */
static int flash_flexspi_nor_config_flash(struct flash_flexspi_nor_data *data,
			struct jesd216_sfdp_header *header,
			struct jesd216_bfp *bfp,
			uint32_t (*flexspi_lut)[MEMC_FLEXSPI_CMD_PER_SEQ])
{
	struct jesd216_instr instr;
	struct jesd216_bfp_dw16 dw16;
	struct jesd216_bfp_dw15 dw15;
	struct jesd216_bfp_dw14 dw14;
	uint8_t addr_width;
	uint8_t mode_cmd;
	int ret;

	/* Read DW14 to determine the polling method we should use while programming */
	ret = jesd216_bfp_decode_dw14(&header->phdr[0], bfp, &dw14);
	if (ret < 0) {
		/* Default to legacy polling mode */
		dw14.poll_options = 0x0;
	}
	if (dw14.poll_options & BIT(1)) {
		/* Read instruction used for polling is 0x70 */
		data->legacy_poll = false;
		flexspi_lut[READ_STATUS_REG][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0x70,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x01);
	} else {
		/* Read instruction used for polling is 0x05 */
		data->legacy_poll = true;
		flexspi_lut[READ_STATUS_REG][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_RDSR,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x01);
	}

	addr_width = jesd216_bfp_addrbytes(bfp) ==
		JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_4B ? 32 : 24;

	/* Check to see if we can enable 4 byte addressing */
	ret = jesd216_bfp_decode_dw16(&header->phdr[0], bfp, &dw16);
	if (ret == 0) {
		/* Attempt to enable 4 byte addressing */
		ret = flash_flexspi_nor_4byte_enable(data, flexspi_lut,
						     dw16.enter_4ba);
		if (ret == 0) {
			/* Use 4 byte address width */
			addr_width = 32;
			/* Update LUT for ERASE_SECTOR and ERASE_BLOCK to use 32 bit addr */
			flexspi_lut[ERASE_SECTOR][0] = FLEXSPI_LUT_SEQ(
					kFLEXSPI_Command_SDR, kFLEXSPI_1PAD,
					SPI_NOR_CMD_SE, kFLEXSPI_Command_RADDR_SDR,
					kFLEXSPI_1PAD, addr_width);
			flexspi_lut[ERASE_BLOCK][0] = FLEXSPI_LUT_SEQ(
					kFLEXSPI_Command_SDR, kFLEXSPI_1PAD,
					SPI_NOR_CMD_BE, kFLEXSPI_Command_RADDR_SDR,
					kFLEXSPI_1PAD, addr_width);
		}
	}
	/* Extract the read command.
	 * Note- enhanced XIP not currently supported, nor is 4-4-4 mode.
	 */
	if (jesd216_bfp_read_support(&header->phdr[0], bfp,
	    JESD216_MODE_144, &instr) > 0) {
		LOG_DBG("Enable 144 mode");
		/* Configure for 144 QUAD read mode */
		if (instr.mode_clocks == 2) {
			mode_cmd = kFLEXSPI_Command_MODE8_SDR;
		} else if (instr.mode_clocks == 1) {
			mode_cmd = kFLEXSPI_Command_MODE4_SDR;
		} else if (instr.mode_clocks == 0) {
			/* Just send dummy cycles during mode clock period */
			mode_cmd = kFLEXSPI_Command_DUMMY_SDR;
		} else {
			return -ENOTSUP;
		}
		flexspi_lut[READ][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, instr.instr,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, addr_width);
		/* Note- we always set mode bits to 0x0 */
		flexspi_lut[READ][1] = FLEXSPI_LUT_SEQ(
				mode_cmd, kFLEXSPI_4PAD, 0x00,
				kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_4PAD, instr.wait_states);
		flexspi_lut[READ][2] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x04,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0);
		/* Read 1S-4S-4S enable method */
		ret = jesd216_bfp_decode_dw15(&header->phdr[0], bfp, &dw15);
		if (ret == 0) {
			ret = flash_flexspi_nor_quad_enable(data, flexspi_lut,
							    dw15.qer);
			if (ret == 0) {
				/* Now, install 1S-1S-4S page program command */
				flexspi_lut[PAGE_PROGRAM][0] = FLEXSPI_LUT_SEQ(
						kFLEXSPI_Command_SDR, kFLEXSPI_1PAD,
						SPI_NOR_CMD_PP_1_1_4, kFLEXSPI_Command_RADDR_SDR,
						kFLEXSPI_1PAD, addr_width);
				flexspi_lut[PAGE_PROGRAM][1] = FLEXSPI_LUT_SEQ(
						kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD,
						0x4, kFLEXSPI_Command_STOP,
						kFLEXSPI_1PAD, 0x0);
			}
		}

	} else if (jesd216_bfp_read_support(&header->phdr[0], bfp,
	    JESD216_MODE_122, &instr) > 0) {
		LOG_DBG("Enable 122 mode");
		if (instr.mode_clocks == 4) {
			mode_cmd = kFLEXSPI_Command_MODE8_SDR;
		} else if (instr.mode_clocks == 2) {
			mode_cmd = kFLEXSPI_Command_MODE4_SDR;
		} else if (instr.mode_clocks == 1) {
			mode_cmd = kFLEXSPI_Command_MODE2_SDR;
		} else if (instr.mode_clocks == 0) {
			/* Just send dummy cycles during mode clock period */
			mode_cmd = kFLEXSPI_Command_DUMMY_SDR;
		} else {
			return -ENOTSUP;
		}
		flexspi_lut[READ][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, instr.instr,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_2PAD, addr_width);
		/* Note- we always set mode bits to 0x0 */
		flexspi_lut[READ][1] = FLEXSPI_LUT_SEQ(
				mode_cmd, kFLEXSPI_2PAD, 0x0,
				kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_2PAD, instr.wait_states);
		flexspi_lut[READ][2] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_2PAD, 0x02,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0);
		/* Now, install 1S-1S-2S page program command */
		flexspi_lut[PAGE_PROGRAM][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_PP_1_1_2,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, addr_width);
		flexspi_lut[PAGE_PROGRAM][1] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_2PAD, 0x4,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0);
	}
	/* Default to 111 mode if no support exists, leave READ/WRITE untouched */

	return 0;
}

/* Helper so we can avoid flash access while performing SFDP probe */
static int flash_flexspi_nor_sfdp_read_helper(struct flash_flexspi_nor_data *dev_data,
		off_t offset, void *data, size_t len)
{
	flexspi_transfer_t transfer = {
		.deviceAddress = offset,
		.port = dev_data->port,
		.cmdType = kFLEXSPI_Read,
		.seqIndex = READ_JESD216,
		.SeqNumber = 1,
		.data = (uint32_t *)data,
		.dataSize = len,
	};

	/* Get SFDP data */
	return memc_flexspi_transfer(&dev_data->controller, &transfer);
}


#if defined(CONFIG_FLASH_JESD216_API)

static int flash_flexspi_nor_sfdp_read(const struct device *dev,
		off_t offset, void *data, size_t len)
{
	struct flash_flexspi_nor_data *dev_data = dev->data;

	return flash_flexspi_nor_sfdp_read_helper(dev_data, offset, data, len);
}

#endif

/* Helper to configure IS25 flash, by clearing read param bits */
static int flash_flexspi_nor_is25_clear_read_param(struct flash_flexspi_nor_data *data,
			uint32_t (*flexspi_lut)[MEMC_FLEXSPI_CMD_PER_SEQ],
			uint32_t *read_params)
{
	int ret;
	/* Install Set Read Parameters (Volatile) command */
	flexspi_transfer_t transfer = {
		.deviceAddress = 0,
		.port = data->port,
		.seqIndex = SCRATCH_CMD,
		.SeqNumber = 1,
		.data = read_params,
		.dataSize = 1,
		.cmdType = kFLEXSPI_Write,
	};
	flexspi_device_config_t config = {
		.flexspiRootClk = MHZ(50),
		.flashSize = FLEXSPI_FLSHCR0_FLSHSZ_MASK, /* Max flash size */
		.ARDSeqNumber = 1,
		.ARDSeqIndex = READ,
	};

	flexspi_lut[SCRATCH_CMD][0] = FLEXSPI_LUT_SEQ(
			kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xC0,
			kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x1);
	ret = memc_flexspi_set_device_config(&data->controller,
				&config,
				(uint32_t *)flexspi_lut,
				FLEXSPI_INSTR_END * MEMC_FLEXSPI_CMD_PER_SEQ,
				data->port);
	if (ret < 0) {
		return ret;
	}
	return memc_flexspi_transfer(&data->controller, &transfer);
}

/* Checks JEDEC ID of flash. If supported, installs custom LUT table */
static int flash_flexspi_nor_check_jedec(struct flash_flexspi_nor_data *data,
			uint32_t (*flexspi_lut)[MEMC_FLEXSPI_CMD_PER_SEQ])
{
	int ret;
	uint32_t vendor_id;
	uint32_t read_params;

	ret = flash_flexspi_nor_read_id_helper(data, (uint8_t *)&vendor_id);
	if (ret < 0) {
		return ret;
	}

	/* Switch on manufacturer and vendor ID */
	switch (vendor_id & 0xFFFF) {
	case 0x609d: /* IS25LP flash, needs P[4:3] cleared with same method as IS25WP */
		read_params = 0xE0U;
		ret = flash_flexspi_nor_is25_clear_read_param(data, flexspi_lut, &read_params);
		if (ret < 0) {
			while (1) {
				/*
				 * Spin here, this flash won't configure correctly.
				 * We can't print a warning, as we are unlikely to
				 * be able to XIP at this point.
				 */
			}
		}
		/* Still return an error- we want the JEDEC configuration to run */
		return -ENOTSUP;
	case 0x709d:
		/*
		 * IS25WP flash. We can support this flash with the JEDEC probe,
		 * but we need to insure P[6:3] are at the default value
		 */
		read_params = 0;
		ret = flash_flexspi_nor_is25_clear_read_param(data, flexspi_lut, &read_params);
		if (ret < 0) {
			while (1) {
				/*
				 * Spin here, this flash won't configure correctly.
				 * We can't print a warning, as we are unlikely to
				 * be able to XIP at this point.
				 */
			}
		}
		/* Still return an error- we want the JEDEC configuration to run */
		return -ENOTSUP;
	case 0x40ef:
		if ((vendor_id & 0xFFFFFF) != 0x2040ef) {
			/*
			 * This is not the correct flash chip, and will not
			 * support the LUT table. Return here
			 */
			return -ENOTSUP;
		}
		/* W25Q512JV flash, use 4 byte read/write */
		flexspi_lut[READ][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_4READ_4B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 32);
		/* Flash needs 6 dummy cycles (at 104MHz) */
		flexspi_lut[READ][1] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_4PAD, 6,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x04);
		/* Only 1S-1S-4S page program supported */
		flexspi_lut[PAGE_PROGRAM][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_PP_1_1_4_4B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 32);
		flexspi_lut[PAGE_PROGRAM][1] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0x4,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0);
		/* Update ERASE commands for 4 byte mode */
		flexspi_lut[ERASE_SECTOR][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_SE_4B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 32);
		flexspi_lut[ERASE_BLOCK][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xDC,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 32),
		/* Read instruction used for polling is 0x05 */
		data->legacy_poll = true;
		flexspi_lut[READ_STATUS_REG][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_RDSR,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x01);
		/* Device uses bit 1 of status reg 2 for QE */
		return flash_flexspi_nor_quad_enable(data, flexspi_lut,
						     JESD216_DW15_QER_VAL_S2B1v5);
	case 0x60ef:
		if ((vendor_id & 0xFFFFFF) != 0x2060ef) {
			/*
			 * This is not the correct flash chip, and will not
			 * support the LUT table. Return here
			 */
			return -ENOTSUP;
		}
		/* W25Q512NW-IQ/IN flash, use 4 byte read/write */
		flexspi_lut[READ][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_4READ_4B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 32);
		/* Flash needs 8 dummy cycles (at 133MHz) */
		flexspi_lut[READ][1] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_4PAD, 8,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x04);
		/* Only 1S-1S-4S page program supported */
		flexspi_lut[PAGE_PROGRAM][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_PP_1_1_4_4B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 32);
		flexspi_lut[PAGE_PROGRAM][1] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0x4,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0);
		/* Update ERASE commands for 4 byte mode */
		flexspi_lut[ERASE_SECTOR][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_SE_4B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 32);
		flexspi_lut[ERASE_BLOCK][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xDC,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 32),
		/* Read instruction used for polling is 0x05 */
		data->legacy_poll = true;
		flexspi_lut[READ_STATUS_REG][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_RDSR,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x01);
		/* Device uses bit 1 of status reg 2 for QE */
		return flash_flexspi_nor_quad_enable(data, flexspi_lut,
						     JESD216_DW15_QER_VAL_S2B1v5);
	case 0x25C2:
		/* MX25 flash, use 4 byte read/write */
		flexspi_lut[READ][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_4READ_4B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 32);
		/* Flash needs 10 dummy cycles */
		flexspi_lut[READ][1] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_4PAD, 10,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_4PAD, 0x04);
		/* Only 1S-4S-4S page program supported */
		flexspi_lut[PAGE_PROGRAM][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_PP_1_4_4_4B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_4PAD, 32);
		flexspi_lut[PAGE_PROGRAM][1] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0x4,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x0);
		/* Update ERASE commands for 4 byte mode */
		flexspi_lut[ERASE_SECTOR][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_SE_4B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 32);
		flexspi_lut[ERASE_BLOCK][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, 0xDC,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 32),
		/* Read instruction used for polling is 0x05 */
		data->legacy_poll = true;
		flexspi_lut[READ_STATUS_REG][0] = FLEXSPI_LUT_SEQ(
				kFLEXSPI_Command_SDR, kFLEXSPI_1PAD, SPI_NOR_CMD_RDSR,
				kFLEXSPI_Command_READ_SDR, kFLEXSPI_1PAD, 0x01);
		/* Device uses bit 6 of status reg 1 for QE */
		return flash_flexspi_nor_quad_enable(data, flexspi_lut, JESD216_DW15_QER_VAL_S1B6);
	default:
		return -ENOTSUP;
	}
}

/* Probe parameters from flash SFDP header, and use them to configure the FlexSPI */
static int flash_flexspi_nor_probe(struct flash_flexspi_nor_data *data)
{
	/* JESD216B defines up to 23 basic flash parameters */
	uint32_t param_buf[23];
	/* Space to store SFDP header and first parameter header */
	uint8_t sfdp_buf[JESD216_SFDP_SIZE(1)] __aligned(4);
	struct jesd216_bfp *bfp = (struct jesd216_bfp *)param_buf;
	struct jesd216_sfdp_header *header = (struct jesd216_sfdp_header *)sfdp_buf;
	int ret;
	unsigned int key = 0U;

	flexspi_device_config_t config = {
		.flexspiRootClk = MHZ(50),
		.flashSize = FLEXSPI_FLSHCR0_FLSHSZ_MASK, /* Max flash size */
		.ARDSeqNumber = 1,
		.ARDSeqIndex = READ,
	};

	if (memc_flexspi_is_running_xip(&data->controller)) {
		/*
		 * ==== ENTER CRITICAL SECTION ====
		 * No flash access should be performed in critical section. All
		 * code and data accessed must reside in ram.
		 */
		key = irq_lock();
		memc_flexspi_wait_bus_idle(&data->controller);
	}

	/* SFDP spec requires that we downclock the FlexSPI to 50MHz or less */
	ret = memc_flexspi_update_clock(&data->controller, &config,
					data->port, MHZ(50));
	if (ret < 0) {
		goto _exit;
	}

	/* Setup initial LUT table and FlexSPI configuration */
	memcpy(flexspi_probe_lut, flash_flexspi_nor_base_lut,
	       sizeof(flash_flexspi_nor_base_lut));

	ret = memc_flexspi_set_device_config(&data->controller, &config,
					(uint32_t *)flexspi_probe_lut,
					FLEXSPI_INSTR_END * MEMC_FLEXSPI_CMD_PER_SEQ,
					data->port);
	if (ret < 0) {
		goto _exit;
	}

	/* First, check if the JEDEC ID of this flash has explicit support
	 * in this driver
	 */
	ret = flash_flexspi_nor_check_jedec(data, flexspi_probe_lut);
	if (ret == 0) {
		/* Flash was supported, SFDP probe not needed */
		goto _program_lut;
	}

	ret = flash_flexspi_nor_sfdp_read_helper(data, 0, sfdp_buf, sizeof(sfdp_buf));
	if (ret < 0) {
		goto _exit;
	}

	LOG_DBG("SFDP header magic: 0x%x", header->magic);
	if (jesd216_sfdp_magic(header) != JESD216_SFDP_MAGIC) {
		/* Header was read incorrectly */
		LOG_WRN("Invalid header, using legacy SPI mode");
		data->legacy_poll = true;
		goto _program_lut;
	}

	if (header->phdr[0].len_dw > ARRAY_SIZE(param_buf)) {
		/* Not enough space to read parameter table */
		ret = -ENOBUFS;
		goto _exit;
	}

	/* Read basic flash parameter table */
	ret = flash_flexspi_nor_sfdp_read_helper(data,
			jesd216_param_addr(&header->phdr[0]),
			param_buf,
			sizeof(uint32_t) * header->phdr[0].len_dw);
	if (ret < 0) {
		goto _exit;
	}

	/* Configure flash */
	ret = flash_flexspi_nor_config_flash(data, header, bfp,
					     flexspi_probe_lut);
	if (ret < 0) {
		goto _exit;
	}

_program_lut:
	/*
	 * Update the FlexSPI with the config structure provided
	 * from devicetree and the configured LUT
	 */
	ret = memc_flexspi_set_device_config(&data->controller, &data->config,
					(uint32_t *)flexspi_probe_lut,
					FLEXSPI_INSTR_PROG_END * MEMC_FLEXSPI_CMD_PER_SEQ,
					data->port);
	if (ret < 0) {
		return ret;
	}

_exit:
	memc_flexspi_reset(&data->controller);

	if (memc_flexspi_is_running_xip(&data->controller)) {
		/* ==== EXIT CRITICAL SECTION ==== */
		irq_unlock(key);
	}

	return ret;
}

static int flash_flexspi_nor_init(const struct device *dev)
{
	const struct flash_flexspi_nor_config *config = dev->config;
	struct flash_flexspi_nor_data *data = dev->data;
	uint32_t vendor_id;

	/* First step- use ROM pointer to controller device to create
	 * a copy of the device structure in RAM we can use while in
	 * critical sections of code.
	 */
	memcpy(&data->controller, config->controller, sizeof(struct device));

	if (!device_is_ready(&data->controller)) {
		LOG_ERR("Controller device is not ready");
		return -ENODEV;
	}

	if (flash_flexspi_nor_probe(data)) {
		if (memc_flexspi_is_running_xip(&data->controller)) {
			/* We can't continue from here- the LUT stored in
			 * the FlexSPI will be invalid so we cannot XIP.
			 * Instead, spin here
			 */
			while (1) {
				/* Spin */
			}
		}
		LOG_ERR("SFDP probe failed");
		return -EIO;
	}

	/* Set the FlexSPI to full clock speed */
	if (memc_flexspi_update_clock(&data->controller, &data->config,
					data->port, data->config.flexspiRootClk)) {
		LOG_ERR("Could not set flexspi clock speed");
		return -ENOTSUP;
	}


	memc_flexspi_reset(&data->controller);

	if (flash_flexspi_nor_read_id(dev, (uint8_t *)&vendor_id)) {
		LOG_ERR("Could not read vendor id");
		return -EIO;
	}
	LOG_DBG("Vendor id: 0x%0x", vendor_id);

	return 0;
}

static const struct flash_driver_api flash_flexspi_nor_api = {
	.erase = flash_flexspi_nor_erase,
	.write = flash_flexspi_nor_write,
	.read = flash_flexspi_nor_read,
	.get_parameters = flash_flexspi_nor_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_flexspi_nor_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = flash_flexspi_nor_sfdp_read,
	.read_jedec_id = flash_flexspi_nor_read_id,
#endif
};

#define CONCAT3(x, y, z) x ## y ## z

#define CS_INTERVAL_UNIT(unit)						\
	CONCAT3(kFLEXSPI_CsIntervalUnit, unit, SckCycle)

#define AHB_WRITE_WAIT_UNIT(unit)					\
	CONCAT3(kFLEXSPI_AhbWriteWaitUnit, unit, AhbCycle)

#define FLASH_FLEXSPI_DEVICE_CONFIG(n)					\
	{								\
		.flexspiRootClk = DT_INST_PROP(n, spi_max_frequency),	\
		.flashSize = DT_INST_PROP(n, size) / 8 / KB(1),		\
		.CSIntervalUnit =					\
			CS_INTERVAL_UNIT(				\
				DT_INST_PROP(n, cs_interval_unit)),	\
		.CSInterval = DT_INST_PROP(n, cs_interval),		\
		.CSHoldTime = DT_INST_PROP(n, cs_hold_time),		\
		.CSSetupTime = DT_INST_PROP(n, cs_setup_time),		\
		.dataValidTime = DT_INST_PROP(n, data_valid_time),	\
		.columnspace = DT_INST_PROP(n, column_space),		\
		.enableWordAddress = DT_INST_PROP(n, word_addressable),	\
		.AWRSeqIndex = 0,					\
		.AWRSeqNumber = 0,					\
		.ARDSeqIndex = READ,					\
		.ARDSeqNumber = 1,					\
		.AHBWriteWaitUnit =					\
			AHB_WRITE_WAIT_UNIT(				\
				DT_INST_PROP(n, ahb_write_wait_unit)),	\
		.AHBWriteWaitInterval =					\
			DT_INST_PROP(n, ahb_write_wait_interval),	\
	}								\

#define FLASH_FLEXSPI_NOR(n)						\
	static const struct flash_flexspi_nor_config			\
		flash_flexspi_nor_config_##n = {			\
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),		\
	};								\
	static struct flash_flexspi_nor_data				\
		flash_flexspi_nor_data_##n = {				\
		.config = FLASH_FLEXSPI_DEVICE_CONFIG(n),		\
		.port = DT_INST_REG_ADDR(n),				\
		.layout = {						\
			.pages_count = DT_INST_PROP(n, size) / 8	\
				/ SPI_NOR_SECTOR_SIZE,			\
			.pages_size = SPI_NOR_SECTOR_SIZE,		\
		},							\
		.flash_parameters = {					\
			.write_block_size = NOR_WRITE_SIZE,		\
			.erase_value = NOR_ERASE_VALUE,			\
		},							\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      flash_flexspi_nor_init,			\
			      NULL,					\
			      &flash_flexspi_nor_data_##n,		\
			      &flash_flexspi_nor_config_##n,		\
			      POST_KERNEL,				\
			      CONFIG_FLASH_INIT_PRIORITY,		\
			      &flash_flexspi_nor_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_FLEXSPI_NOR)
