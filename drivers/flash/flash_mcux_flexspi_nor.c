/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_imx_flexspi_nor

#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include "spi_nor.h"
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
	/* Instructions matching with XIP layout */
	READ_FAST_QUAD_OUTPUT,
	READ_FAST_OUTPUT,
	READ_NORMAL_OUTPUT,
	READ_STATUS,
	WRITE_ENABLE,
	ERASE_SECTOR,
	PAGE_PROGRAM_INPUT,
	PAGE_PROGRAM_QUAD_INPUT,
	READ_ID,
	WRITE_STATUS_REG,
	ENTER_QPI,
	EXIT_QPI,
	READ_STATUS_REG,
	ERASE_CHIP,
};

/* Device variables used in critical sections should be in this structure */
struct flash_flexspi_nor_data {
	const struct device *controller;
	flexspi_device_config_t config;
	flexspi_port_t port;
	struct flash_pages_layout layout;
	struct flash_parameters flash_parameters;
};

static const uint32_t flash_flexspi_nor_lut[][4] = {
	[READ_ID] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, SPI_NOR_CMD_RDID,
				kFLEXSPI_Command_READ_SDR,  kFLEXSPI_1PAD, 0x04),
	},

	[READ_STATUS_REG] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, SPI_NOR_CMD_RDSR,
				kFLEXSPI_Command_READ_SDR,  kFLEXSPI_1PAD, 0x04),
	},

	[WRITE_STATUS_REG] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, SPI_NOR_CMD_WRSR,
				kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x04),
	},

	[WRITE_ENABLE] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, SPI_NOR_CMD_WREN,
				kFLEXSPI_Command_STOP,      kFLEXSPI_1PAD, 0),
	},

	[ERASE_SECTOR] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, SPI_NOR_CMD_SE,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
	},

	[ERASE_CHIP] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, SPI_NOR_CMD_CE,
				kFLEXSPI_Command_STOP,      kFLEXSPI_1PAD, 0),
	},

	[READ_FAST_QUAD_OUTPUT] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, 0x6B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_4PAD, 0x08,
				kFLEXSPI_Command_READ_SDR,  kFLEXSPI_4PAD, 0x04),
	},

	[READ_FAST_OUTPUT] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, 0x0B,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_DUMMY_SDR, kFLEXSPI_1PAD, 0x08,
				kFLEXSPI_Command_READ_SDR,  kFLEXSPI_1PAD, 0x04),
	},

	[READ_NORMAL_OUTPUT] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, SPI_NOR_CMD_READ,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_READ_SDR,  kFLEXSPI_1PAD, 0x04,
				kFLEXSPI_Command_STOP,      kFLEXSPI_1PAD, 0),
	},

	[READ_STATUS] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, 0x81,
				kFLEXSPI_Command_READ_SDR,  kFLEXSPI_1PAD, 0x04),
	},

	[PAGE_PROGRAM_INPUT] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, SPI_NOR_CMD_PP,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_1PAD, 0x04,
				kFLEXSPI_Command_STOP,      kFLEXSPI_1PAD, 0),
	},

	[PAGE_PROGRAM_QUAD_INPUT] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, 0x32,
				kFLEXSPI_Command_RADDR_SDR, kFLEXSPI_1PAD, 0x18),
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_WRITE_SDR, kFLEXSPI_4PAD, 0x04,
				kFLEXSPI_Command_STOP,      kFLEXSPI_1PAD, 0),
	},

	[ENTER_QPI] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_1PAD, 0x35,
				kFLEXSPI_Command_STOP,      kFLEXSPI_1PAD, 0),
	},

	[EXIT_QPI] = {
		FLEXSPI_LUT_SEQ(kFLEXSPI_Command_SDR,       kFLEXSPI_4PAD, 0xF5,
				kFLEXSPI_Command_STOP,      kFLEXSPI_1PAD, 0),
	},
};

static int flash_flexspi_nor_get_vendor_id(const struct device *dev,
		uint8_t *vendor_id)
{
	struct flash_flexspi_nor_data *data = dev->data;
	uint32_t buffer = 0;
	int ret;

	flexspi_transfer_t transfer = {
		.deviceAddress = 0,
		.port = data->port,
		.cmdType = kFLEXSPI_Read,
		.SeqNumber = 1,
		.seqIndex = READ_ID,
		.data = &buffer,
		.dataSize = 1,
	};

	LOG_DBG("Reading id");

	ret = memc_flexspi_transfer(data->controller, &transfer);
	*vendor_id = buffer;

	return ret;
}

static int flash_flexspi_nor_read_status(const struct device *dev,
		uint32_t *status)
{
	struct flash_flexspi_nor_data *data = dev->data;

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

	return memc_flexspi_transfer(data->controller, &transfer);
}

static int flash_flexspi_nor_write_status(const struct device *dev,
		uint32_t *status)
{
	struct flash_flexspi_nor_data *data = dev->data;

	flexspi_transfer_t transfer = {
		.deviceAddress = 0,
		.port = data->port,
		.cmdType = kFLEXSPI_Write,
		.SeqNumber = 1,
		.seqIndex = WRITE_STATUS_REG,
		.data = status,
		.dataSize = 1,
	};

	LOG_DBG("Writing status register");

	return memc_flexspi_transfer(data->controller, &transfer);
}

static int flash_flexspi_nor_write_enable(const struct device *dev)
{
	struct flash_flexspi_nor_data *data = dev->data;

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

	return memc_flexspi_transfer(data->controller, &transfer);
}

static int flash_flexspi_nor_erase_sector(const struct device *dev,
	off_t offset)
{
	struct flash_flexspi_nor_data *data = dev->data;

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

	return memc_flexspi_transfer(data->controller, &transfer);
}

static int flash_flexspi_nor_erase_chip(const struct device *dev)
{
	struct flash_flexspi_nor_data *data = dev->data;

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

	return memc_flexspi_transfer(data->controller, &transfer);
}

static int flash_flexspi_nor_page_program(const struct device *dev,
		off_t offset, const void *buffer, size_t len)
{
	struct flash_flexspi_nor_data *data = dev->data;

	flexspi_transfer_t transfer = {
		.deviceAddress = offset,
		.port = data->port,
		.cmdType = kFLEXSPI_Write,
		.SeqNumber = 1,
		.seqIndex = PAGE_PROGRAM_QUAD_INPUT,
		.data = (uint32_t *) buffer,
		.dataSize = len,
	};

	LOG_DBG("Page programming %d bytes to 0x%08zx", len, (ssize_t) offset);

	return memc_flexspi_transfer(data->controller, &transfer);
}

static int flash_flexspi_nor_wait_bus_busy(const struct device *dev)
{
	uint32_t status = 0;
	int ret;

	do {
		ret = flash_flexspi_nor_read_status(dev, &status);
		LOG_DBG("status: 0x%x", status);
		if (ret) {
			LOG_ERR("Could not read status");
			return ret;
		}
	} while (status & BIT(0));

	return 0;
}

static int flash_flexspi_nor_enable_quad_mode(const struct device *dev)
{
	struct flash_flexspi_nor_data *data = dev->data;
	uint32_t status = 0x40;

	flash_flexspi_nor_write_status(dev, &status);
	flash_flexspi_nor_wait_bus_busy(dev);
	memc_flexspi_reset(data->controller);

	return 0;
}

static int flash_flexspi_nor_read(const struct device *dev, off_t offset,
		void *buffer, size_t len)
{
	struct flash_flexspi_nor_data *data = dev->data;
	uint8_t *src = memc_flexspi_get_ahb_address(data->controller,
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

	uint8_t *dst = memc_flexspi_get_ahb_address(data->controller,
						    data->port,
						    offset);

	if (memc_flexspi_is_running_xip(data->controller)) {
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
		flash_flexspi_nor_write_enable(dev);
#ifdef CONFIG_FLASH_MCUX_FLEXSPI_NOR_WRITE_BUFFER
		flash_flexspi_nor_page_program(dev, offset, nor_write_buf, i);
#else
		flash_flexspi_nor_page_program(dev, offset, src, i);
#endif
		flash_flexspi_nor_wait_bus_busy(dev);
		memc_flexspi_reset(data->controller);
		src += i;
		offset += i;
		len -= i;
	}

	if (memc_flexspi_is_running_xip(data->controller)) {
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
	int num_sectors = size / SPI_NOR_SECTOR_SIZE;
	int i;
	unsigned int key = 0;

	uint8_t *dst = memc_flexspi_get_ahb_address(data->controller,
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

	if (memc_flexspi_is_running_xip(data->controller)) {
		/*
		 * ==== ENTER CRITICAL SECTION ====
		 * No flash access should be performed in critical section. All
		 * code and data accessed must reside in ram.
		 */
		key = irq_lock();
	}

	if ((offset == 0) && (size == data->config.flashSize * KB(1))) {
		flash_flexspi_nor_write_enable(dev);
		flash_flexspi_nor_erase_chip(dev);
		flash_flexspi_nor_wait_bus_busy(dev);
		memc_flexspi_reset(data->controller);
	} else {
		for (i = 0; i < num_sectors; i++) {
			flash_flexspi_nor_write_enable(dev);
			flash_flexspi_nor_erase_sector(dev, offset);
			flash_flexspi_nor_wait_bus_busy(dev);
			memc_flexspi_reset(data->controller);
			offset += SPI_NOR_SECTOR_SIZE;
		}
	}

	if (memc_flexspi_is_running_xip(data->controller)) {
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

static int flash_flexspi_nor_init(const struct device *dev)
{
	struct flash_flexspi_nor_data *data = dev->data;
	uint8_t vendor_id;

	if (!device_is_ready(data->controller)) {
		LOG_ERR("Controller device is not ready");
		return -ENODEV;
	}

	if (!memc_flexspi_is_running_xip(data->controller) &&
	    memc_flexspi_set_device_config(data->controller, &data->config,
					   data->port)) {
		LOG_ERR("Could not set device configuration");
		return -EINVAL;
	}

	if (memc_flexspi_update_lut(data->controller, 0,
				   (const uint32_t *) flash_flexspi_nor_lut,
				   sizeof(flash_flexspi_nor_lut) / 4)) {
		LOG_ERR("Could not update lut");
		return -EINVAL;
	}

	memc_flexspi_reset(data->controller);

	if (flash_flexspi_nor_get_vendor_id(dev, &vendor_id)) {
		LOG_ERR("Could not read vendor id");
		return -EIO;
	}
	LOG_DBG("Vendor id: 0x%0x", vendor_id);

	if (flash_flexspi_nor_enable_quad_mode(dev)) {
		LOG_ERR("Could not enable quad mode");
		return -EIO;
	}

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
};

#define CONCAT3(x, y, z) x ## y ## z

#define CS_INTERVAL_UNIT(unit)						\
	CONCAT3(kFLEXSPI_CsIntervalUnit, unit, SckCycle)

#define AHB_WRITE_WAIT_UNIT(unit)					\
	CONCAT3(kFLEXSPI_AhbWriteWaitUnit, unit, AhbCycle)

#define FLASH_FLEXSPI_DEVICE_CONFIG(n)					\
	{								\
		.flexspiRootClk = MHZ(120),				\
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
		.ARDSeqIndex = READ_FAST_QUAD_OUTPUT,			\
		.ARDSeqNumber = 1,					\
		.AHBWriteWaitUnit =					\
			AHB_WRITE_WAIT_UNIT(				\
				DT_INST_PROP(n, ahb_write_wait_unit)),	\
		.AHBWriteWaitInterval =					\
			DT_INST_PROP(n, ahb_write_wait_interval),	\
	}								\

#define FLASH_FLEXSPI_NOR(n)						\
	static struct flash_flexspi_nor_data				\
		flash_flexspi_nor_data_##n = {				\
		.controller = DEVICE_DT_GET(DT_INST_BUS(n)),		\
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
			      NULL,					\
			      POST_KERNEL,				\
			      CONFIG_FLASH_INIT_PRIORITY,		\
			      &flash_flexspi_nor_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_FLEXSPI_NOR)
