/*
 * Copyright (c) 2021 Volvo Construction Equipment
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_flexspi_hyperflash

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/drivers/flash.h>

#include <zephyr/logging/log.h>

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

LOG_MODULE_REGISTER(flexspi_hyperflash, CONFIG_FLASH_LOG_LEVEL);

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif

#include <zephyr/sys/util.h>

#include "memc_mcux_flexspi.h"

#define SPI_HYPERFLASH_SECTOR_SIZE              (0x40000U)
#define SPI_HYPERFLASH_PAGE_SIZE                (512U)

#define HYPERFLASH_ERASE_VALUE                  (0xFF)

#ifdef CONFIG_FLASH_MCUX_FLEXSPI_HYPERFLASH_WRITE_BUFFER
static uint8_t hyperflash_write_buf[SPI_HYPERFLASH_PAGE_SIZE];
#endif

enum {
	/* Instructions matching with XIP layout */
	READ_DATA = 0,
	WRITE_DATA = 1,
	READ_STATUS = 2,
	WRITE_ENABLE = 4,
	ERASE_SECTOR = 6,
	PAGE_PROGRAM = 10,
	ERASE_CHIP = 12,
};

#define CUSTOM_LUT_LENGTH                      64

static const uint32_t flash_flexspi_hyperflash_lut[CUSTOM_LUT_LENGTH] = {
	/* Read Data */
	[4 * READ_DATA] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR,       kFLEXSPI_8PAD, 0xA0,
				kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18),
	[4 * READ_DATA + 1] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10,
				kFLEXSPI_Command_READ_DDR,  kFLEXSPI_8PAD, 0x04),
	/* Write Data */
	[4 * WRITE_DATA] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR,       kFLEXSPI_8PAD, 0x20,
				kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18),
	[4 * WRITE_DATA + 1] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10,
				kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_8PAD, 0x02),
	/* Read Status */
	[4 * READ_STATUS] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * READ_STATUS + 1] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	[4 * READ_STATUS + 2] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
	[4 * READ_STATUS + 3] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x70),
	[4 * READ_STATUS + 4] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR,       kFLEXSPI_8PAD, 0xA0,
				kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18),
	[4 * READ_STATUS + 5] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_CADDR_DDR,      kFLEXSPI_8PAD, 0x10,
				kFLEXSPI_Command_DUMMY_RWDS_DDR, kFLEXSPI_8PAD, 0x0B),
	[4 * READ_STATUS + 6] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_DDR, kFLEXSPI_8PAD, 0x04,
				kFLEXSPI_Command_STOP,     kFLEXSPI_1PAD, 0x0),
	/* Write Enable */
	[4 * WRITE_ENABLE] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x20,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * WRITE_ENABLE + 1] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	[4 * WRITE_ENABLE + 2] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
	[4 * WRITE_ENABLE + 3] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	[4 * WRITE_ENABLE + 4] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x20,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * WRITE_ENABLE + 5] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55),
	[4 * WRITE_ENABLE + 6] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x02),
	[4 * WRITE_ENABLE + 7] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55),

	/* Erase Sector  */
	[4 * ERASE_SECTOR] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * ERASE_SECTOR + 1] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	[4 * ERASE_SECTOR + 2] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
	[4 * ERASE_SECTOR + 3] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x80),
	[4 * ERASE_SECTOR + 4] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * ERASE_SECTOR + 5] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	[4 * ERASE_SECTOR + 6] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
	[4 * ERASE_SECTOR + 7] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	[4 * ERASE_SECTOR + 8] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * ERASE_SECTOR + 9] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55),
	[4 * ERASE_SECTOR + 10] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x02),
	[4 * ERASE_SECTOR + 11] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55),
	[4 * ERASE_SECTOR + 12] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR,       kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18),
	[4 * ERASE_SECTOR + 13] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10,
				kFLEXSPI_Command_DDR,       kFLEXSPI_8PAD, 0x00),
	[4 * ERASE_SECTOR + 14] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR,  kFLEXSPI_8PAD, 0x30,
				kFLEXSPI_Command_STOP, kFLEXSPI_1PAD, 0x00),

	/* program page with word program command sequence */
	[4 * PAGE_PROGRAM] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x20,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * PAGE_PROGRAM + 1] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	[4 * PAGE_PROGRAM + 2] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
	[4 * PAGE_PROGRAM + 3] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xA0),
	[4 * PAGE_PROGRAM + 4] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR,       kFLEXSPI_8PAD, 0x20,
				kFLEXSPI_Command_RADDR_DDR, kFLEXSPI_8PAD, 0x18),
	[4 * PAGE_PROGRAM + 5] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_CADDR_DDR, kFLEXSPI_8PAD, 0x10,
				kFLEXSPI_Command_WRITE_DDR, kFLEXSPI_8PAD, 0x80),

	/* Erase chip */
	[4 * ERASE_CHIP] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * ERASE_CHIP + 1] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	[4 * ERASE_CHIP + 2] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
	[4 * ERASE_CHIP + 3] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x80),
	/* 1 */
	[4 * ERASE_CHIP + 4] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * ERASE_CHIP + 5] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	[4 * ERASE_CHIP + 6] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
	[4 * ERASE_CHIP + 7] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	/* 2 */
	[4 * ERASE_CHIP + 8] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * ERASE_CHIP + 9] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55),
	[4 * ERASE_CHIP + 10] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x02),
	[4 * ERASE_CHIP + 11] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x55),
	/* 3 */
	[4 * ERASE_CHIP + 12] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00),
	[4 * ERASE_CHIP + 13] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0xAA),
	[4 * ERASE_CHIP + 14] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x05),
	[4 * ERASE_CHIP + 15] =
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x00,
				kFLEXSPI_Command_DDR, kFLEXSPI_8PAD, 0x10),
};


struct flash_flexspi_hyperflash_config {
	const struct device *controller;
};

/* Device variables used in critical sections should be in this structure */
struct flash_flexspi_hyperflash_data {
	struct device controller;
	flexspi_device_config_t config;
	flexspi_port_t port;
	struct flash_pages_layout layout;
	struct flash_parameters flash_parameters;
};

static int flash_flexspi_hyperflash_wait_bus_busy(const struct device *dev)
{
	struct flash_flexspi_hyperflash_data *data = dev->data;
	flexspi_transfer_t transfer;
	int ret;
	bool is_busy;
	uint32_t read_value;

	transfer.deviceAddress = 0;
	transfer.port = data->port;
	transfer.cmdType = kFLEXSPI_Read;
	transfer.SeqNumber = 2;
	transfer.seqIndex = READ_STATUS;
	transfer.data = &read_value;
	transfer.dataSize = 2;

	do {
		ret = memc_flexspi_transfer(&data->controller, &transfer);
		if (ret != 0) {
			return ret;
		}

		is_busy = !(read_value & 0x8000);

		if (read_value & 0x3200) {
			ret = -EINVAL;
			break;
		}
	} while (is_busy);

	return ret;
}

static int flash_flexspi_hyperflash_write_enable(const struct device *dev, uint32_t address)
{
	struct flash_flexspi_hyperflash_data *data = dev->data;
	flexspi_transfer_t transfer;
	int ret;

	transfer.deviceAddress = address;
	transfer.port = data->port;
	transfer.cmdType = kFLEXSPI_Command;
	transfer.SeqNumber = 2;
	transfer.seqIndex = WRITE_ENABLE;

	ret = memc_flexspi_transfer(&data->controller, &transfer);

	return ret;
}

static int flash_flexspi_hyperflash_check_vendor_id(const struct device *dev)
{
	struct flash_flexspi_hyperflash_data *data = dev->data;
	uint8_t writebuf[4] = {0x00, 0x98};
	uint32_t buffer[2];
	int ret;
	flexspi_transfer_t transfer;

	transfer.deviceAddress = (0x555 * 2);
	transfer.port = data->port;
	transfer.cmdType = kFLEXSPI_Write;
	transfer.SeqNumber = 1;
	transfer.seqIndex = WRITE_DATA;
	transfer.data = (uint32_t *)writebuf;
	transfer.dataSize = 2;

	LOG_DBG("Reading id");

	ret = memc_flexspi_transfer(&data->controller, &transfer);
	if (ret != 0) {
		LOG_ERR("failed to CFI");
		return ret;
	}

	transfer.deviceAddress = (0x10 * 2);
	transfer.port = data->port;
	transfer.cmdType = kFLEXSPI_Read;
	transfer.SeqNumber = 1;
	transfer.seqIndex = READ_DATA;
	transfer.data = buffer;
	transfer.dataSize = 8;

	ret = memc_flexspi_transfer(&data->controller, &transfer);
	if (ret != 0) {
		LOG_ERR("failed to read id");
		return ret;
	}
	buffer[1] &= 0xFFFF;
	/* Check that the data read out is  unicode "QRY" in big-endian order */
	if ((buffer[0] != 0x52005100) || (buffer[1] != 0x5900)) {
		LOG_ERR("data read out is wrong!");
		return -EINVAL;
	}

	writebuf[1] = 0xF0;
	transfer.deviceAddress = 0;
	transfer.port = data->port;
	transfer.cmdType = kFLEXSPI_Write;
	transfer.SeqNumber = 1;
	transfer.seqIndex = WRITE_DATA;
	transfer.data = (uint32_t *)writebuf;
	transfer.dataSize = 2;

	ret = memc_flexspi_transfer(&data->controller, &transfer);
	if (ret != 0) {
		LOG_ERR("failed to exit");
		return ret;
	}

	memc_flexspi_reset(&data->controller);

	return ret;
}

static int flash_flexspi_hyperflash_page_program(const struct device *dev, off_t
		offset, const void *buffer, size_t len)
{
	struct flash_flexspi_hyperflash_data *data = dev->data;

	flexspi_transfer_t transfer = {
		.deviceAddress = offset,
		.port = data->port,
		.cmdType = kFLEXSPI_Write,
		.SeqNumber = 2,
		.seqIndex = PAGE_PROGRAM,
		.data = (uint32_t *)buffer,
		.dataSize = len,
	};

	LOG_DBG("Page programming %d bytes to 0x%08lx", len, offset);

	return memc_flexspi_transfer(&data->controller, &transfer);
}

static int flash_flexspi_hyperflash_read(const struct device *dev, off_t offset,
		void *buffer, size_t len)
{
	struct flash_flexspi_hyperflash_data *data = dev->data;

	uint8_t *src = memc_flexspi_get_ahb_address(&data->controller,
			data->port,
			offset);
	if (!src) {
		return -EINVAL;
	}

	memcpy(buffer, src, len);

	return 0;
}

static int flash_flexspi_hyperflash_write(const struct device *dev, off_t offset,
		const void *buffer, size_t len)
{
	struct flash_flexspi_hyperflash_data *data = dev->data;
	size_t size = len;
	uint8_t *src = (uint8_t *)buffer;
	unsigned int key = 0;
	int i, j;
	int ret = -1;

	uint8_t *dst = memc_flexspi_get_ahb_address(&data->controller,
			data->port,
			offset);
	if (!dst) {
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

	/* Clock FlexSPI at 84 MHZ (42MHz SCLK in DDR mode) */
	(void)memc_flexspi_update_clock(&data->controller, &data->config,
					data->port, MHZ(84));

	while (len) {
		/* Writing between two page sizes crashes the platform so we
		 * have to write the part that fits in the first page and then
		 * update the offset.
		 */
		i = MIN(SPI_HYPERFLASH_PAGE_SIZE - (offset %
					SPI_HYPERFLASH_PAGE_SIZE), len);
#ifdef CONFIG_FLASH_MCUX_FLEXSPI_HYPERFLASH_WRITE_BUFFER
		for (j = 0; j < i; j++) {
			hyperflash_write_buf[j] = src[j];
		}
#endif
		ret = flash_flexspi_hyperflash_write_enable(dev, offset);
		if (ret != 0) {
			LOG_ERR("failed to enable write");
			break;
		}
#ifdef CONFIG_FLASH_MCUX_FLEXSPI_HYPERFLASH_WRITE_BUFFER
		ret = flash_flexspi_hyperflash_page_program(dev, offset,
				hyperflash_write_buf, i);
#else
		ret = flash_flexspi_hyperflash_page_program(dev, offset, src, i);
#endif
		if (ret != 0) {
			LOG_ERR("failed to write");
			break;
		}

		ret = flash_flexspi_hyperflash_wait_bus_busy(dev);
		if (ret != 0) {
			LOG_ERR("failed to wait bus busy");
			break;
		}

		/* Do software reset. */
		memc_flexspi_reset(&data->controller);
		src += i;
		offset += i;
		len -= i;
	}

	/* Clock FlexSPI at 332 MHZ (166 MHz SCLK in DDR mode) */
	(void)memc_flexspi_update_clock(&data->controller, &data->config,
					data->port, MHZ(332));

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_InvalidateByRange((uint32_t) dst, size);
#endif

	if (memc_flexspi_is_running_xip(&data->controller)) {
		/* ==== EXIT CRITICAL SECTION ==== */
		irq_unlock(key);
	}

	return ret;
}

static int flash_flexspi_hyperflash_erase(const struct device *dev, off_t offset, size_t size)
{
	struct flash_flexspi_hyperflash_data *data = dev->data;
	flexspi_transfer_t transfer;
	int ret = -1;
	int i;
	unsigned int key = 0;
	int num_sectors = size / SPI_HYPERFLASH_SECTOR_SIZE;
	uint8_t *dst = memc_flexspi_get_ahb_address(&data->controller,
			data->port,
			offset);

	if (!dst) {
		return -EINVAL;
	}

	if (offset % SPI_HYPERFLASH_SECTOR_SIZE) {
		LOG_ERR("Invalid offset");
		return -EINVAL;
	}

	if (size % SPI_HYPERFLASH_SECTOR_SIZE) {
		LOG_ERR("Invalid offset");
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

	for (i = 0; i < num_sectors; i++) {
		ret = flash_flexspi_hyperflash_write_enable(dev, offset);
		if (ret != 0) {
			LOG_ERR("failed to write_enable");
			break;
		}

		LOG_DBG("Erasing sector at 0x%08lx", offset);

		transfer.deviceAddress = offset;
		transfer.port = data->port;
		transfer.cmdType = kFLEXSPI_Command;
		transfer.SeqNumber = 4;
		transfer.seqIndex = ERASE_SECTOR;

		ret = memc_flexspi_transfer(&data->controller, &transfer);
		if (ret != 0) {
			LOG_ERR("failed to erase");
			break;
		}

		/* wait bus busy */
		ret = flash_flexspi_hyperflash_wait_bus_busy(dev);
		if (ret != 0) {
			LOG_ERR("failed to wait bus busy");
			break;
		}

		/* Do software reset. */
		memc_flexspi_reset(&data->controller);

		offset += SPI_HYPERFLASH_SECTOR_SIZE;
	}

#ifdef CONFIG_HAS_MCUX_CACHE
	DCACHE_InvalidateByRange((uint32_t) dst, size);
#endif

	if (memc_flexspi_is_running_xip(&data->controller)) {
		/* ==== EXIT CRITICAL SECTION ==== */
		irq_unlock(key);
	}

	return ret;
}

static const struct flash_parameters *flash_flexspi_hyperflash_get_parameters(
		const struct device *dev)
{
	struct flash_flexspi_hyperflash_data *data = dev->data;

	return &data->flash_parameters;
}


static void flash_flexspi_hyperflash_pages_layout(const struct device *dev,
		const struct flash_pages_layout **layout,
		size_t *layout_size)
{
	struct flash_flexspi_hyperflash_data *data = dev->data;

	*layout = &data->layout;
	*layout_size = 1;
}

static int flash_flexspi_hyperflash_init(const struct device *dev)
{
	const struct flash_flexspi_hyperflash_config *config = dev->config;
	struct flash_flexspi_hyperflash_data *data = dev->data;

	/* Since the controller variable may be used in critical sections,
	 * copy the device pointer into a variable stored in RAM
	 */
	memcpy(&data->controller, config->controller, sizeof(struct device));

	if (!device_is_ready(&data->controller)) {
		LOG_ERR("Controller device not ready");
		return -ENODEV;
	}

	memc_flexspi_wait_bus_idle(&data->controller);

	if (memc_flexspi_is_running_xip(&data->controller)) {
		/* Wait for bus idle before configuring */
		memc_flexspi_wait_bus_idle(&data->controller);
	}
	if (memc_flexspi_set_device_config(&data->controller, &data->config,
	    (const uint32_t *)flash_flexspi_hyperflash_lut,
	    sizeof(flash_flexspi_hyperflash_lut) / MEMC_FLEXSPI_CMD_SIZE,
	    data->port)) {
		LOG_ERR("Could not set device configuration");
		return -EINVAL;
	}

	memc_flexspi_reset(&data->controller);

	if (flash_flexspi_hyperflash_check_vendor_id(dev)) {
		LOG_ERR("Could not read vendor id");
		return -EIO;
	}

	return 0;
}

static const struct flash_driver_api flash_flexspi_hyperflash_api = {
	.read = flash_flexspi_hyperflash_read,
	.write = flash_flexspi_hyperflash_write,
	.erase = flash_flexspi_hyperflash_erase,
	.get_parameters = flash_flexspi_hyperflash_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_flexspi_hyperflash_pages_layout,
#endif
};

#define CONCAT3(x, y, z) x ## y ## z

#define CS_INTERVAL_UNIT(unit)						\
	CONCAT3(kFLEXSPI_CsIntervalUnit, unit, SckCycle)

#define AHB_WRITE_WAIT_UNIT(unit)					\
	CONCAT3(kFLEXSPI_AhbWriteWaitUnit, unit, AhbCycle)

#define FLASH_FLEXSPI_DEVICE_CONFIG(n)					\
	{								\
		.flexspiRootClk = MHZ(42),				\
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
		.AWRSeqIndex = WRITE_DATA,					\
		.AWRSeqNumber = 1,					\
		.ARDSeqIndex = READ_DATA,				\
		.ARDSeqNumber = 1,					\
		.AHBWriteWaitUnit =					\
			AHB_WRITE_WAIT_UNIT(				\
				DT_INST_PROP(n, ahb_write_wait_unit)),	\
		.AHBWriteWaitInterval =					\
			DT_INST_PROP(n, ahb_write_wait_interval),	\
	}								\

#define FLASH_FLEXSPI_HYPERFLASH(n)					\
	static struct flash_flexspi_hyperflash_config			\
		flash_flexspi_hyperflash_config_##n = {			\
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),		\
	};								\
	static struct flash_flexspi_hyperflash_data			\
		flash_flexspi_hyperflash_data_##n = {			\
		.config = FLASH_FLEXSPI_DEVICE_CONFIG(n),		\
		.port = DT_INST_REG_ADDR(n),				\
		.layout = {						\
			.pages_count = DT_INST_PROP(n, size) / 8	\
				/ SPI_HYPERFLASH_SECTOR_SIZE,		\
			.pages_size = SPI_HYPERFLASH_SECTOR_SIZE,	\
		},							\
		.flash_parameters = {					\
			.write_block_size = DT_INST_PROP(n, write_block_size), \
			.erase_value = HYPERFLASH_ERASE_VALUE,		\
		},							\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      flash_flexspi_hyperflash_init,		\
			      NULL,					\
			      &flash_flexspi_hyperflash_data_##n,	\
			      &flash_flexspi_hyperflash_config_##n,	\
			      POST_KERNEL,				\
			      CONFIG_FLASH_INIT_PRIORITY,		\
			      &flash_flexspi_hyperflash_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_FLEXSPI_HYPERFLASH)
