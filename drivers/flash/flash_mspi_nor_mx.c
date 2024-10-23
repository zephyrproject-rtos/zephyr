/*
 * Copyright (c) 2024, STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * **************************************************************************
 * MSPI flash device driver for Macronix mx25lm51245 (or compatible)
 * This driver is working with the drivers/mspi/mspi_stm32.c driver
 * Flash device with compatible = "mspi-nor-mx25" in the DTS NODE
 * **************************************************************************
 */
#define DT_DRV_COMPAT mspi_nor_mx25

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>

#include <zephyr/drivers/flash.h>
#include "spi_nor.h"
#include "jesd216.h"

LOG_MODULE_REGISTER(flash_mspi_nor_mx, CONFIG_FLASH_LOG_LEVEL);

typedef struct mspi_timing_cfg mspi_timing_cfg;
typedef enum mspi_timing_param mspi_timing_param;

#define NOR_MX_WRITE_SIZE  1
#define NOR_MX_ERASE_VALUE 0xff

#define NOR_MX_STATUS_MEM_RDY    1
#define NOR_MX_STATUS_MEM_WREN   2
#define NOR_MX_STATUS_MEM_ERASED 3

#define NOR_MX_RESET_MAX_TIME              100U
#define NOR_MX_BULK_ERASE_MAX_TIME         460000U
#define NOR_MX_SECTOR_ERASE_MAX_TIME       1000U
#define NOR_MX_SUBSECTOR_4K_ERASE_MAX_TIME 400U
#define NOR_MX_WRITE_REG_MAX_TIME          40U

enum nor_mx_dummy_clock {
	NOR_MX_DC_8,
	NOR_MX_DC_10,
	NOR_MX_DC_12,
	NOR_MX_DC_14,
	NOR_MX_DC_16,
	NOR_MX_DC_18,
	NOR_MX_DC_20,
	NOR_MX_DC_22,
};

struct flash_mspi_nor_mx_config {
	uint32_t port;
	uint32_t mem_size; /* in Bytes */
	struct flash_parameters flash_param;
	struct flash_pages_layout page_layout;

	const struct device *bus;
	struct mspi_dev_id dev_id;
	struct mspi_dev_cfg serial_cfg;
	struct mspi_dev_cfg tar_dev_cfg;
	struct mspi_xip_cfg tar_xip_cfg;
	struct mspi_scramble_cfg tar_scramble_cfg;

	mspi_timing_cfg tar_timing_cfg;
	mspi_timing_param timing_cfg_mask;

	bool sw_multi_periph;
};

struct flash_mspi_nor_mx_data {
	struct mspi_dev_cfg dev_cfg;
	struct mspi_xip_cfg xip_cfg;
	struct mspi_scramble_cfg scramble_cfg;
	mspi_timing_cfg timing_cfg;
	struct mspi_xfer trans;
	struct mspi_xfer_packet packet;

	struct k_sem lock;
	uint8_t jedec_id[JESD216_READ_ID_LEN];
};

static bool flash_mspi_nor_mx_address_is_valid(const struct device *dev, off_t addr, size_t size)
{
	const struct flash_mspi_nor_mx_config *dev_cfg = dev->config;
	uint32_t flash_size = dev_cfg->mem_size;

	return (addr >= 0) && ((uint64_t)addr + (uint64_t)size <= flash_size);
}

/* Command to the flash for writing parameters */
static int flash_mspi_nor_mx_command_write(const struct device *flash, uint8_t cmd, uint32_t addr,
					   uint16_t addr_len, uint32_t tx_dummy, uint8_t *wdata,
					   uint32_t length)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;
	int ret;

	data->packet.dir = MSPI_TX;
	data->packet.cmd = cmd;
	data->packet.address = addr;
	data->packet.data_buf = wdata;
	data->packet.num_bytes = length;

	data->trans.async = false;        /* meaning : timeout mode */
	data->trans.xfer_mode = MSPI_PIO; /* command_write is always in PIO mode */
	data->trans.tx_dummy = tx_dummy;
	data->trans.cmd_length = 1;
	data->trans.addr_length = addr_len;
	data->trans.hold_ce = false;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = 10;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI Tx transaction failed with code: %d", ret);
		return -EIO;
	}

	LOG_DBG("MSPI Tx transaction (cmd = 0x%x)", data->packet.cmd);

	return ret;
}

/* Command to the flash for reading parameters */
static int flash_mspi_nor_mx_command_read(const struct device *flash, uint8_t cmd, uint32_t addr,
					  uint16_t addr_len, uint32_t rx_dummy, uint8_t *rdata,
					  uint32_t length)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;
	int ret;

	data->packet.dir = MSPI_RX;
	data->packet.cmd = cmd;
	data->packet.address = addr;
	data->packet.data_buf = rdata;
	data->packet.num_bytes = length;

	data->trans.async = false;        /* meaning : timeout mode */
	data->trans.xfer_mode = MSPI_PIO; /* command_read is always in PIO mode */
	data->trans.rx_dummy = rx_dummy;
	data->trans.cmd_length = 1;
	data->trans.addr_length = addr_len;
	data->trans.hold_ce = false;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = 10;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI Rx transaction failed with code: %d", ret);
		return -EIO;
	}

	LOG_DBG("MSPI Rx transaction (cmd = 0x%x)", data->packet.cmd);

	return ret;
}

/* Command to the flash for reading status register  */
static int flash_mspi_nor_mx_status_read(const struct device *flash, uint8_t status)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;
	int ret;
	uint8_t status_config[2]; /* index 0 for Match, index 1 for MASK */

	data->packet.dir = MSPI_TX;          /* a command to be sent */
	data->packet.cmd = SPI_NOR_CMD_RDSR; /* SPI/STR */
	data->packet.address = 0;

	data->trans.num_packet = 1;       /* 1 in STR; 2 in DTR */
	data->trans.async = true;         /* IT mode */
	data->trans.xfer_mode = MSPI_PIO; /* command is always in PIO mode */
	data->trans.tx_dummy = 0;
	data->trans.cmd_length = 1;
	data->trans.addr_length = 0;
	data->trans.hold_ce = false;

	/* Send the Read status Reg command and the autopolling with matching bit */
	switch (status) {
	case NOR_MX_STATUS_MEM_RDY:
		status_config[0] = SPI_NOR_MEM_RDY_MATCH;
		status_config[1] = SPI_NOR_MEM_RDY_MASK;
		data->trans.timeout = HAL_XSPI_TIMEOUT_DEFAULT_VALUE;
		break;
	case NOR_MX_STATUS_MEM_ERASED:
		status_config[0] = SPI_NOR_WEL_MATCH;
		status_config[1] = SPI_NOR_WEL_MASK;
		data->trans.timeout = NOR_MX_BULK_ERASE_MAX_TIME;
		break;
	case NOR_MX_STATUS_MEM_WREN:
		status_config[0] = SPI_NOR_WREN_MATCH;
		status_config[1] = SPI_NOR_WREN_MASK;
		data->trans.timeout = HAL_XSPI_TIMEOUT_DEFAULT_VALUE;
		break;
	default:
		LOG_ERR("Flash MSPI read status %d not supported", status);
		return -EIO;
	}
	data->packet.data_buf = status_config;
	data->packet.num_bytes = sizeof(status_config);

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("Flash MSPI read transaction failed with code: %d", ret);
		return -EIO;
	}

	LOG_DBG("Flash MSPI status transaction (mode = %d)", data->trans.xfer_mode);

	return ret;
}

static void acquire(const struct device *flash)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;

	k_sem_take(&data->lock, K_FOREVER);

	if (cfg->sw_multi_periph) {
		while (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL,
				       &data->dev_cfg))
			;
	} else {
		while (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_NONE, NULL))
			;
	}
}

static void release(const struct device *flash)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;

	while (mspi_get_channel_status(cfg->bus, cfg->port))
		;

	k_sem_give(&data->lock);
}

/* Enables writing to the memory sending a Write Enable and wait it is effective */
static int flash_mspi_nor_mx_write_enable(const struct device *flash)
{
	int ret;

	LOG_DBG("Enabling write");

	/* Initialize the write enable command */
	ret = flash_mspi_nor_mx_command_write(flash, SPI_NOR_CMD_WREN, 0, 4, 0, NULL, 0);
	if (ret) {
		return ret;
	}
	/* Followed by the polling on bit WREN */
	ret = flash_mspi_nor_mx_status_read(flash, NOR_MX_STATUS_MEM_WREN);

	return ret;
}

static int flash_mspi_nor_mx_write_disable(const struct device *flash)
{
	int ret;

	LOG_DBG("Disabling write");
	ret = flash_mspi_nor_mx_command_write(flash, SPI_NOR_CMD_WRDI, 0, 4, 0, NULL, 0);

	return ret;
}

static int flash_mspi_nor_mx_reset(const struct device *flash)
{
	int ret;

	/* TODO : reset by gpio pin :set pi, wait duration, release pin */
	LOG_DBG("Resetting ");
	/* Send Reset enable then reset Mem  use the SPI/STR command */
	ret = flash_mspi_nor_mx_command_write(flash, SPI_NOR_CMD_RESET_EN, 0, 0, 0, NULL, 0);
	if (ret) {
		return ret;
	}
	ret = flash_mspi_nor_mx_command_write(flash, SPI_NOR_CMD_RESET_MEM, 0, 0, 0, NULL, 0);
	if (ret) {
		return ret;
	}

	return ret;
}

static int flash_mspi_nor_mx_get_vendor_id(const struct device *flash, uint8_t *vendor_id)
{
	struct flash_mspi_nor_mx_data *data = flash->data;
	uint8_t buffer[JESD216_READ_ID_LEN];
	int ret;

	if (vendor_id == NULL) {
		return -EINVAL;
	}

	LOG_DBG("Reading id");

	/* Read bytes from flash : use the SPI/STR command */
	ret = flash_mspi_nor_mx_command_read(flash, SPI_NOR_CMD_RDID, 0, 4, data->dev_cfg.rx_dummy,
					     buffer, JESD216_READ_ID_LEN);
	if (ret) {
		return ret;
	}

	memcpy(data->jedec_id, buffer, JESD216_READ_ID_LEN);
	*vendor_id = data->jedec_id[0];

	LOG_DBG("Jedec ID = [%02x %02x %02x]", data->jedec_id[0], data->jedec_id[1],
		data->jedec_id[2]);

	return ret;
}

static int flash_mspi_nor_mx_unprotect_sector(const struct device *flash, off_t addr)
{
	int ret;

	LOG_DBG("unprotect sector at 0x%08zx", (ssize_t)addr);

	ret = flash_mspi_nor_mx_command_write(flash, 0x39, addr, 4, 0, NULL, 0);

	return ret;
}

static int flash_mspi_nor_mx_erase_sector(const struct device *flash, off_t addr)
{
	int ret;

	LOG_DBG("Erasing sector at 0x%08zx", (ssize_t)addr);
	/* Instruction SPI_NOR_CMD_SE is also accepted */
	ret = flash_mspi_nor_mx_command_write(flash, SPI_NOR_CMD_SE_4B, addr, 4, 0, NULL, 0);

	return ret;
}

static int flash_mspi_nor_mx_erased(const struct device *flash)
{
	int ret;

	LOG_DBG("Wait for mem erased");

	/* Wait polling the WEL (write enable latch) bit to become to 0
	 * When the Chip Erase Cycle is completed, the Write Enable Latch (WEL) bit is cleared.
	 * in cfg_mode SPI/OPI and cfg_rate transfer STR/DTR
	 */
	ret = flash_mspi_nor_mx_status_read(flash, NOR_MX_STATUS_MEM_ERASED);

	return ret;
}

static int flash_mspi_nor_mx_erase_block(const struct device *flash, off_t addr)
{
	int ret;

	LOG_DBG("Erasing block at 0x%08zx", (ssize_t)addr);

	ret = flash_mspi_nor_mx_command_write(flash, SPI_NOR_CMD_BE, addr, 4, 0, NULL, 0);

	return ret;
}

static int flash_mspi_nor_mx_erase_chip(const struct device *flash)
{
	int ret;

	LOG_DBG("Erasing chip");

	ret = flash_mspi_nor_mx_command_write(flash, SPI_NOR_CMD_CE, 0, 4, 0, NULL, 0);

	return ret;
}

static int flash_mspi_nor_mx_page_program(const struct device *flash, off_t offset, void *wdata,
					  size_t len)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;
	int ret;

	data->packet.dir = MSPI_TX;
	data->packet.cmd = data->dev_cfg.write_cmd;
	data->packet.address = offset;
	data->packet.data_buf = wdata;
	data->packet.num_bytes = len;

	data->trans.async = true;         /* use callback on Irq if PIO, meaningless with DMA */
	data->trans.xfer_mode = MSPI_PIO; /* TODO : transfer with DMA */
	data->trans.tx_dummy = data->dev_cfg.tx_dummy;
	data->trans.cmd_length = data->dev_cfg.cmd_length;
	data->trans.addr_length = data->dev_cfg.addr_length;
	data->trans.hold_ce = false;
	data->trans.priority = 1;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	LOG_DBG("Page programming %d bytes to 0x%08zx", len, (ssize_t)offset);

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI write transaction failed with code: %d", ret);
		return -EIO;
	}

	return ret;
}

/*
 * This function Polls the WIP(Write In Progress) bit to become to 0
 * in nor_mode SPI/OPI XSPI_SPI_MODE or XSPI_OCTO_MODE
 * and nor_rate transfer STR/DTR XSPI_STR_TRANSFER or XSPI_DTR_TRANSFER
 */
static int flash_mspi_nor_mx_mem_ready(const struct device *flash)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;
	mspi_timing_cfg bkp = data->timing_cfg;
	int ret;

	LOG_DBG("Reading status register");
	ret = flash_mspi_nor_mx_status_read(flash, NOR_MX_STATUS_MEM_RDY);
	if (ret) {
		LOG_ERR("Could not read status");
		return ret;
	}

	if (data->dev_cfg.io_mode != MSPI_IO_MODE_SINGLE) {
		data->timing_cfg = bkp;
		if (mspi_timing_config(cfg->bus, &cfg->dev_id, cfg->timing_cfg_mask,
				       (void *)&data->timing_cfg)) {
			LOG_ERR("Failed to config mspi controller");
			return -EIO;
		}
	}

	return ret;
}

/* Function to read the flash with possible PIO IT or DMA */
static int flash_mspi_nor_mx_read(const struct device *flash, off_t offset, void *rdata,
				  size_t size)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;
	int ret = 0;

	/* Ignore zero size write */
	if (size == 0) {
		return 0;
	}

	if (!flash_mspi_nor_mx_address_is_valid(flash, offset, size)) {
		LOG_ERR("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu",
			(long)offset, size);
		return -EINVAL;
	}

	LOG_DBG("Flash : read %d Bytes at 0x%lx", size, (long)offset);
	acquire(flash);

	/* During the MemoryMapped, read with a memcopy */
	if (cfg->tar_xip_cfg.enable) {

		/* REG_MSPI_BASEADDR is given by the cfg->tar_xip_cfg.address_offset */
		uintptr_t mmap_addr = cfg->tar_xip_cfg.address_offset + offset;

		memcpy(rdata, (void *)mmap_addr, size);

		LOG_DBG("Read %d bytes from 0x%lx", size, (long)offset);
		goto read_end;
	}

	data->packet.dir = MSPI_RX;
	data->packet.cmd = data->dev_cfg.read_cmd;
	data->packet.address = offset;
	data->packet.data_buf = rdata;
	data->packet.num_bytes = size;
	/* ASYNC transfer : use callback on Irq if PIO, meaningless with DMA */
	data->trans.async = true;
	data->trans.xfer_mode = MSPI_PIO; /* TODO : transfer with DMA */
	/* TODO set data->dev_cfg.rx_dummy; depending on the command */
	data->trans.rx_dummy = 0;
	data->trans.cmd_length = data->dev_cfg.cmd_length;
	data->trans.addr_length = data->dev_cfg.addr_length;
	data->trans.hold_ce = false;
	data->trans.priority = 1;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	LOG_DBG("Read %d bytes from 0x%08zx", size, (ssize_t)offset);

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI read transaction failed with code: %d", ret);
		return -EIO;
	}

read_end:
	release(flash);

	return ret;
}

/* Function to write the flash (page program) : with possible PIO IT (ASYNC) or DMA */
static int flash_mspi_nor_mx_write(const struct device *flash, off_t offset, const void *wdata,
				   size_t size)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;
	int ret = 0;
	uint8_t *src = (uint8_t *)wdata;
	int i;

	/* Ignore zero size write */
	if (size == 0) {
		return 0;
	}

	if (!flash_mspi_nor_mx_address_is_valid(flash, offset, size)) {
		LOG_ERR("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu",
			(long)offset, size);
		return -EINVAL;
	}

	LOG_DBG("Flash : write %d Bytes at 0x%lx", size, (long)offset);

	acquire(flash);

	/* Cannot write during MemoryMapped, first disable */
	if (cfg->tar_xip_cfg.enable) {

		if (mspi_xip_config(cfg->bus, &cfg->dev_id, false)) {
			LOG_ERR("Failed to disable XIP");
			goto write_end;
		}
		data->xip_cfg = cfg->tar_xip_cfg;
		/* And continue */
	}

	/* First check that flash is ready for erasing */
	ret = flash_mspi_nor_mx_mem_ready(flash);
	if (ret) {
		goto write_end;
	}

	while (size) {
		/* If the offset isn't a multiple of the NOR page size, we first need
		 * to write the remaining part that fits, otherwise the write could
		 * be wrapped around within the same page
		 */
		i = MIN(SPI_NOR_PAGE_SIZE - (offset % SPI_NOR_PAGE_SIZE), size);

		ret = flash_mspi_nor_mx_write_enable(flash);
		if (ret) {
			goto write_end;
		}

		ret = flash_mspi_nor_mx_page_program(flash, offset, src, i);
		if (ret) {
			goto write_end;
		}

		ret = flash_mspi_nor_mx_status_read(flash, NOR_MX_STATUS_MEM_RDY);
		if (ret) {
			goto write_end;
		}

		src += i;
		offset += i;
		size -= i;
	}

	ret = flash_mspi_nor_mx_write_disable(flash);
	if (ret) {
		goto write_end;
	}

write_end:
	release(flash);

	return ret;
}

/*
 * Function to erase the flash : chip or sector with possible OCTO/SPI and STR/DTR
 * to erase the complete chip (using dedicated command) :
 *   set size >= flash size
 *   set addr = 0
 */
static int flash_mspi_nor_mx_erase(const struct device *flash, off_t offset, size_t size)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;
	int ret = 0;
	const size_t num_sectors = size / SPI_NOR_SECTOR_SIZE;
	const size_t num_blocks = size / SPI_NOR_BLOCK_SIZE;

	int i;

	/* Ignore zero size erase */
	if (size == 0) {
		return 0;
	}

	if (!flash_mspi_nor_mx_address_is_valid(flash, offset, size)) {
		LOG_ERR("Error: address or size exceeds expected values: "
			"addr 0x%lx, size %zu",
			(long)offset, size);
		return -EINVAL;
	}

	/* Maximise erase size : means the complete chip */
	if (size > cfg->mem_size) {
		size = cfg->mem_size;
	}

	if (offset % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid offset");
		return -EINVAL;
	}

	if (size % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}

	acquire(flash);

	/* Cannot erase during MemoryMapped, first disable */
	if (cfg->tar_xip_cfg.enable) {
		if (mspi_xip_config(cfg->bus, &cfg->dev_id, false)) {
			LOG_ERR("Failed to disable XIP");
			goto erase_end;
		}
		data->xip_cfg = cfg->tar_xip_cfg;
		/* And continue */
	}

	/* First check that flash is ready for erasing */
	ret = flash_mspi_nor_mx_mem_ready(flash);
	if (ret) {
		goto erase_end;
	}

	if ((offset == 0) && (size == cfg->mem_size)) {
		ret = flash_mspi_nor_mx_write_enable(flash);
		if (ret) {
			goto erase_end;
		}

		ret = flash_mspi_nor_mx_erase_chip(flash);
		if (ret) {
			goto erase_end;
		}
		/* Chip (Bulk) erase started, wait until WEL becomes 0 */
		ret = flash_mspi_nor_mx_erased(flash);
		if (ret) {
			goto erase_end;
		}

	} else if ((0 == (offset % SPI_NOR_BLOCK_SIZE)) && (0 == (size % SPI_NOR_BLOCK_SIZE))) {
		for (i = 0; i < num_blocks; i++) {
			/* Check if write enable on each sector or for the whole device */
			ret = flash_mspi_nor_mx_write_enable(flash);
			if (ret) {
				goto erase_end;
			}
			ret = flash_mspi_nor_mx_unprotect_sector(flash, offset);
			if (ret) {
				goto erase_end;
			}
			ret = flash_mspi_nor_mx_write_enable(flash);
			if (ret) {
				goto erase_end;
			}
			ret = flash_mspi_nor_mx_erase_block(flash, offset);
			if (ret) {
				goto erase_end;
			}

			ret = flash_mspi_nor_mx_mem_ready(flash);
			if (ret) {
				goto erase_end;
			}

			offset += SPI_NOR_BLOCK_SIZE;
		}
	} else {
		for (i = 0; i < num_sectors; i++) {
			/* Check if write enable on each sector or for the whole device */
			ret = flash_mspi_nor_mx_write_enable(flash);
			if (ret) {
				goto erase_end;
			}
			ret = flash_mspi_nor_mx_unprotect_sector(flash, offset);
			if (ret) {
				goto erase_end;
			}
			/* Check if write enable on each sector or for the whole device */
			ret = flash_mspi_nor_mx_write_enable(flash);
			if (ret) {
				goto erase_end;
			}

			ret = flash_mspi_nor_mx_erase_sector(flash, offset);
			if (ret) {
				goto erase_end;
			}

			ret = flash_mspi_nor_mx_mem_ready(flash);
			if (ret) {
				goto erase_end;
			}

			offset += SPI_NOR_SECTOR_SIZE;
		}
	}

erase_end:
	release(flash);

	return ret;
}

static const struct flash_parameters *flash_mspi_nor_mx_get_parameters(const struct device *flash)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;

	return &cfg->flash_param;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_mspi_nor_mx_pages_layout(const struct device *flash,
					   const struct flash_pages_layout **layout,
					   size_t *layout_size)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;

	*layout = &cfg->page_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int flash_mspi_nor_mx_init(const struct device *flash)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;
	uint8_t vendor_id;

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("Controller device is not ready");
		return -ENODEV;
	}

	switch (cfg->tar_dev_cfg.io_mode) {
	case MSPI_IO_MODE_SINGLE:
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_OCTAL:
		break;
	default:
		LOG_ERR("MSPI bus mode %d not supported", cfg->tar_dev_cfg.io_mode);
		return -EIO;
	}

	switch (cfg->tar_dev_cfg.data_rate) {
	case MSPI_DATA_RATE_SINGLE:
	case MSPI_DATA_RATE_DUAL:
		break;
	default:
		LOG_ERR("MSPI bus data rate %d not supported", cfg->tar_dev_cfg.data_rate);
		return -EIO;
	}

	/* The SPI/DTR is not a valid config of data_mode/data_rate according to the DTS */
	if ((cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_SINGLE) &&
	    (cfg->tar_dev_cfg.data_rate == MSPI_DATA_RATE_DUAL)) {
		LOG_ERR("MSPI data rate SPI/DTR is not valid");
		return -EIO;
	}

	/* At this time only set the io_mode and data rate */
	if (mspi_dev_config(cfg->bus, &cfg->dev_id,
			    (MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_DATA_RATE),
			    &cfg->serial_cfg)) {
		LOG_ERR("Failed to config mspi controller");
		return -EIO;
	}
	data->dev_cfg = cfg->serial_cfg;

	if (flash_mspi_nor_mx_reset(flash)) {
		LOG_ERR("Could not reset Flash");
		return -EIO;
	}

	LOG_DBG("Flash reset");

	if (flash_mspi_nor_mx_get_vendor_id(flash, &vendor_id)) {
		LOG_ERR("Could not read vendor id");
		return -EIO;
	}
	LOG_DBG("Vendor id: 0x%0x", vendor_id);

	/*
	 * We should check here that the mem is ready,
	 * configure it according to the io-mode/data-rate
	 * MSPI_DEVICE_CONFIG_ALL will overwrite the previous config of the MSPI
	 * SKIP it for NOW
	 * TODO: MSPI_DEVICE_CONFIG_ALL
	 */
	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_NONE, &cfg->tar_dev_cfg)) {
		LOG_ERR("Failed to config mspi controller");
		return -EIO;
	}

	LOG_DBG("Flash config'd");

	/* XIP will need the base address and size for MemoryMapped mode */
	if (cfg->tar_xip_cfg.enable) {
		if (mspi_xip_config(cfg->bus, &cfg->dev_id, &cfg->tar_xip_cfg)) {
			LOG_ERR("Failed to enable XIP");
			return -EIO;
		}
		data->xip_cfg = cfg->tar_xip_cfg;
	}

	if (cfg->tar_scramble_cfg.enable) {
		if (mspi_scramble_config(cfg->bus, &cfg->dev_id, &cfg->tar_scramble_cfg)) {
			LOG_ERR("Failed to enable scrambling");
			return -EIO;
		}
		data->scramble_cfg = cfg->tar_scramble_cfg;
	}

	release(flash);

	return 0;
}

#if defined(CONFIG_FLASH_JESD216_API)
static int flash_mspi_nor_mx_read_sfdp(const struct device *flash, off_t addr, void *rdata,
				       size_t size)
{
	const struct flash_mspi_nor_mx_config *cfg = flash->config;
	struct flash_mspi_nor_mx_data *data = flash->data;
	int ret = 0;

	acquire(flash);

	data->packet.dir = MSPI_RX;
	data->packet.cmd = JESD216_CMD_READ_SFDP;
	data->packet.address = addr;
	data->packet.data_buf = rdata;
	data->packet.num_bytes = size;

	data->trans.async = false;
	data->trans.xfer_mode = MSPI_PIO;
	data->trans.rx_dummy = 8;
	data->trans.cmd_length = 1;
	data->trans.addr_length = 3; /* 24 bits */
	data->trans.hold_ce = false;
	data->trans.priority = 1;
	data->trans.packets = &data->packet;
	data->trans.num_packet = 1;
	data->trans.timeout = CONFIR_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	LOG_DBG("Read %d bytes from 0x%08zx", size, (ssize_t)addr);

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);

	if (ret) {
		LOG_ERR("MSPI read transaction failed with code: %d", ret);
		goto end_sfdp;
	}

end_sfdp:
	release(flash);
	return ret;
}

static int flash_mspi_nor_mx_read_jedec_id(const struct device *flash, uint8_t *id)
{
	struct flash_mspi_nor_mx_data *data = flash->data;

	id = &data->jedec_id;
	return 0;
}
#endif /* CONFIG_FLASH_JESD216_API */

#if defined(CONFIG_PM_DEVICE)
static int flash_mspi_nor_mx_pm_action(const struct device *flash, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		acquire(flash);

		release(flash);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		acquire(flash);

		release(flash);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /** IS_ENABLED(CONFIG_PM_DEVICE) */

static const struct flash_driver_api flash_mspi_nor_mx_api = {
	.erase = flash_mspi_nor_mx_erase,
	.write = flash_mspi_nor_mx_write,
	.read = flash_mspi_nor_mx_read,
	.get_parameters = flash_mspi_nor_mx_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mspi_nor_mx_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = flash_mspi_nor_mx_read_sfdp,
	.read_jedec_id = flash_mspi_nor_mx_read_jedec_id,
#endif
};

/* TODO: we use the serial_cfg.mem_boundary to give the NOR flash size to the MSPI controller
 * Other possible is to pass size through the XIP mspi_xip_cfg structure
 */
#define MSPI_DEVICE_CONFIG_SERIAL(n)                                                               \
	{                                                                                          \
		.ce_num = DT_INST_PROP(n, mspi_hardware_ce_num),                                   \
		.freq = DT_INST_PROP(n, mspi_max_frequency),                                       \
		.io_mode = DT_INST_STRING_UNQUOTED_OR(n, mspi_io_mode, 0),                         \
		.data_rate = DT_INST_STRING_UNQUOTED_OR(n, mspi_data_rate, 0),                     \
		.cpp = MSPI_CPP_MODE_0,                                                            \
		.endian = MSPI_XFER_LITTLE_ENDIAN,                                                 \
		.ce_polarity = MSPI_CE_ACTIVE_LOW,                                                 \
		.dqs_enable = false,                                                               \
		.rx_dummy = DT_INST_PROP_OR(n, rx_dummy, 0),                                       \
		.tx_dummy = DT_INST_PROP_OR(n, tx_dummy, 0),                                       \
		.read_cmd = DT_INST_PROP_OR(n, read_command, SPI_NOR_CMD_READ_FAST),               \
		.write_cmd = DT_INST_PROP_OR(n, write_command, SPI_NOR_CMD_PP),                    \
		.cmd_length = 1,                                                                   \
		.addr_length = 4,                                                                  \
		.mem_boundary = 0,                                                                 \
		.time_to_break = 0,                                                                \
	}

static const struct flash_mspi_nor_mx_config flash_mspi_nor_mx_cfg = {
	.port = DT_INST_REG_ADDR(0),       /* Not used */
	.mem_size = DT_INST_PROP(0, size), /* in Bytes */
	.flash_param = {
			.write_block_size = NOR_MX_WRITE_SIZE,
			.erase_value = NOR_MX_ERASE_VALUE,
		},
	.page_layout = {
			.pages_count = DT_INST_PROP(0, size) / SPI_NOR_PAGE_SIZE,
			.pages_size = SPI_NOR_PAGE_SIZE,
		},
	.bus = DEVICE_DT_GET(DT_INST_BUS(0)),
	.dev_id = MSPI_DEVICE_ID_DT_INST(0),
	.serial_cfg = MSPI_DEVICE_CONFIG_SERIAL(0),
	.tar_dev_cfg = MSPI_DEVICE_CONFIG_DT_INST(0),
	.tar_xip_cfg = MSPI_XIP_CONFIG_DT_INST(0),
	.tar_scramble_cfg = MSPI_SCRAMBLE_CONFIG_DT_INST(0),
	.sw_multi_periph = DT_PROP(DT_INST_BUS(0), software_multiperipheral)};

static struct flash_mspi_nor_mx_data flash_mspi_nor_mx_dev_data = {
	.lock = Z_SEM_INITIALIZER(flash_mspi_nor_mx_dev_data.lock, 0, 1),
};

DEVICE_DT_INST_DEFINE(0, flash_mspi_nor_mx_init, PM_DEVICE_DT_INST_GET(0),
		      &flash_mspi_nor_mx_dev_data, &flash_mspi_nor_mx_cfg, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_mspi_nor_mx_api);
