/*
 * Copyright (c) 2024, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mspi_atxp032

#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#if CONFIG_SOC_FAMILY_AMBIQ
#include "mspi_ambiq.h"
typedef struct mspi_ambiq_timing_cfg mspi_timing_cfg;
typedef enum mspi_ambiq_timing_param mspi_timing_param;

#else
typedef struct mspi_timing_cfg mspi_timing_cfg;
typedef enum mspi_timing_param mspi_timing_param;
#define TIMING_CFG_GET_RX_DUMMY(cfg)
#define TIMING_CFG_SET_RX_DUMMY(cfg, num)
#endif

#include <zephyr/drivers/flash.h>
#include "spi_nor.h"
LOG_MODULE_REGISTER(flash_mspi_atxp032, CONFIG_FLASH_LOG_LEVEL);

#define NOR_WRITE_SIZE                      1
#define NOR_ERASE_VALUE                     0xff

#define ATXP032_VENDOR_ID                   0x43

enum atxp032_dummy_clock {
	ATXP032_DC_8,
	ATXP032_DC_10,
	ATXP032_DC_12,
	ATXP032_DC_14,
	ATXP032_DC_16,
	ATXP032_DC_18,
	ATXP032_DC_20,
	ATXP032_DC_22,
};
struct flash_mspi_atxp032_config {
	uint32_t                            port;
	uint32_t                            mem_size;
	struct flash_parameters             flash_param;
	struct flash_pages_layout           page_layout;

	const struct device                 *bus;
	struct mspi_dev_id                  dev_id;
	struct mspi_dev_cfg                 serial_cfg;
	struct mspi_dev_cfg                 tar_dev_cfg;
	struct mspi_xip_cfg                 tar_xip_cfg;
	struct mspi_scramble_cfg            tar_scramble_cfg;

	mspi_timing_cfg                     tar_timing_cfg;
	mspi_timing_param                   timing_cfg_mask;

	bool                                sw_multi_periph;
};

struct flash_mspi_atxp032_data {
	struct mspi_dev_cfg                 dev_cfg;
	struct mspi_xip_cfg                 xip_cfg;
	struct mspi_scramble_cfg            scramble_cfg;
	mspi_timing_cfg                     timing_cfg;
	struct mspi_xfer                    trans;
	struct mspi_xfer_packet             packet;

	struct k_sem                        lock;
	uint32_t                            jedec_id;
};

static int atxp032_get_dummy_clk(uint8_t rxdummy, uint32_t *dummy_clk)
{
	switch (rxdummy) {
	case 8:
		*dummy_clk = ATXP032_DC_8;
		break;
	case 10:
		*dummy_clk = ATXP032_DC_10;
		break;
	case 12:
		*dummy_clk = ATXP032_DC_12;
		break;
	case 14:
		*dummy_clk = ATXP032_DC_14;
		break;
	case 16:
		*dummy_clk = ATXP032_DC_16;
		break;
	case 18:
		*dummy_clk = ATXP032_DC_18;
		break;
	case 20:
		*dummy_clk = ATXP032_DC_20;
		break;
	case 22:
		*dummy_clk = ATXP032_DC_22;
		break;
	default:
		return 1;
	}
	return 0;
}

static int flash_mspi_atxp032_command_write(const struct device *flash, uint8_t cmd, uint32_t addr,
					    uint16_t addr_len, uint32_t tx_dummy, uint8_t *wdata,
					    uint32_t length)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;
	struct flash_mspi_atxp032_data *data = flash->data;
	int ret;

	data->packet.dir              = MSPI_TX;
	data->packet.cmd              = cmd;
	data->packet.address          = addr;
	data->packet.data_buf         = wdata;
	data->packet.num_bytes        = length;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_PIO;
	data->trans.tx_dummy          = tx_dummy;
	data->trans.cmd_length        = 1;
	data->trans.addr_length       = addr_len;
	data->trans.hold_ce           = false;
	data->trans.packets           = &data->packet;
	data->trans.num_packet        = 1;
	data->trans.timeout           = 10;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI write transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}
	return ret;
}

static int flash_mspi_atxp032_command_read(const struct device *flash, uint8_t cmd, uint32_t addr,
					   uint16_t addr_len, uint32_t rx_dummy, uint8_t *rdata,
					   uint32_t length)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;
	struct flash_mspi_atxp032_data *data = flash->data;
	int ret;

	data->packet.dir              = MSPI_RX;
	data->packet.cmd              = cmd;
	data->packet.address          = addr;
	data->packet.data_buf         = rdata;
	data->packet.num_bytes        = length;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_PIO;
	data->trans.rx_dummy          = rx_dummy;
	data->trans.cmd_length        = 1;
	data->trans.addr_length       = addr_len;
	data->trans.hold_ce           = false;
	data->trans.packets           = &data->packet;
	data->trans.num_packet        = 1;
	data->trans.timeout           = 10;

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI read transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}
	return ret;
}

static void acquire(const struct device *flash)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;
	struct flash_mspi_atxp032_data *data = flash->data;

	k_sem_take(&data->lock, K_FOREVER);

	if (cfg->sw_multi_periph) {
		while (mspi_dev_config(cfg->bus, &cfg->dev_id,
				       MSPI_DEVICE_CONFIG_ALL, &data->dev_cfg)) {
			;
		}
	} else {
		while (mspi_dev_config(cfg->bus, &cfg->dev_id,
				       MSPI_DEVICE_CONFIG_NONE, NULL)) {
			;
		}
	}
}

static void release(const struct device *flash)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;
	struct flash_mspi_atxp032_data *data = flash->data;

	while (mspi_get_channel_status(cfg->bus, cfg->port)) {
		;
	}

	k_sem_give(&data->lock);
}

static int flash_mspi_atxp032_write_enable(const struct device *flash)
{
	int ret;

	LOG_DBG("Enabling write");
	ret = flash_mspi_atxp032_command_write(flash, SPI_NOR_CMD_WREN, 0, 0, 0, NULL, 0);

	return ret;
}

static int flash_mspi_atxp032_write_disable(const struct device *flash)
{
	int ret;

	LOG_DBG("Disabling write");
	ret = flash_mspi_atxp032_command_write(flash, SPI_NOR_CMD_WRDI, 0, 0, 0, NULL, 0);

	return ret;
}

static int flash_mspi_atxp032_reset(const struct device *flash)
{
	int ret;

	ret = flash_mspi_atxp032_write_enable(flash);
	if (ret) {
		return ret;
	}

	LOG_DBG("Return to SPI mode");
	ret = flash_mspi_atxp032_command_write(flash, 0xFF, 0, 0, 0, NULL, 0);
	if (ret) {
		return ret;
	}

	ret = flash_mspi_atxp032_write_disable(flash);
	if (ret) {
		return ret;
	}

	return ret;
}

static int flash_mspi_atxp032_get_vendor_id(const struct device *flash, uint8_t *vendor_id)
{
	struct flash_mspi_atxp032_data *data = flash->data;
	uint8_t buffer[11];
	int ret;

	if (vendor_id == NULL) {
		return -EINVAL;
	}

	LOG_DBG("Reading id");
	/* serial mode */
	ret = flash_mspi_atxp032_command_read(flash, SPI_NOR_CMD_RDID, 0, 0, 0, buffer, 11);
	*vendor_id = buffer[7];

	data->jedec_id = (buffer[7] << 16) | (buffer[8] << 8) | buffer[9];

	return ret;
}

static int flash_mspi_atxp032_unprotect_sector(const struct device *flash, off_t addr)
{
	int ret;

	LOG_DBG("unprotect sector at 0x%08zx", (ssize_t)addr);

	ret = flash_mspi_atxp032_command_write(flash, 0x39, addr, 4, 0, NULL, 0);

	return ret;
}

static int flash_mspi_atxp032_erase_sector(const struct device *flash, off_t addr)
{
	int ret;

	LOG_DBG("Erasing sector at 0x%08zx", (ssize_t)addr);

	ret = flash_mspi_atxp032_command_write(flash, SPI_NOR_CMD_SE, addr, 4, 0, NULL, 0);

	return ret;
}

static int flash_mspi_atxp032_erase_block(const struct device *flash, off_t addr)
{
	int ret;

	LOG_DBG("Erasing block at 0x%08zx", (ssize_t)addr);

	ret = flash_mspi_atxp032_command_write(flash, SPI_NOR_CMD_BE, addr, 4, 0, NULL, 0);

	return ret;
}

static int flash_mspi_atxp032_erase_chip(const struct device *flash)
{
	int ret;

	LOG_DBG("Erasing chip");

	ret = flash_mspi_atxp032_command_write(flash, SPI_NOR_CMD_CE, 0, 0, 0, NULL, 0);

	return ret;
}

static int flash_mspi_atxp032_page_program(const struct device *flash, off_t offset, void *wdata,
					   size_t len)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;
	struct flash_mspi_atxp032_data *data = flash->data;
	int ret;

	data->packet.dir              = MSPI_TX;
	data->packet.cmd              = data->dev_cfg.write_cmd;
	data->packet.address          = offset;
	data->packet.data_buf         = wdata;
	data->packet.num_bytes        = len;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_DMA;
	data->trans.tx_dummy          = data->dev_cfg.tx_dummy;
	data->trans.cmd_length        = data->dev_cfg.cmd_length;
	data->trans.addr_length       = data->dev_cfg.addr_length;
	data->trans.hold_ce           = false;
	data->trans.priority          = 1;
	data->trans.packets           = &data->packet;
	data->trans.num_packet        = 1;
	data->trans.timeout           = CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	LOG_DBG("Page programming %d bytes to 0x%08zx", len, (ssize_t)offset);

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI write transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}
	return ret;
}

static int flash_mspi_atxp032_busy_wait(const struct device *flash)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;
	struct flash_mspi_atxp032_data *data = flash->data;
	mspi_timing_cfg bkp = data->timing_cfg;

	uint32_t status = 0;
	uint32_t rx_dummy;
	int ret;

	if (data->dev_cfg.io_mode == MSPI_IO_MODE_SINGLE) {
		rx_dummy = 0;
	} else {
		rx_dummy = 4;
		TIMING_CFG_SET_RX_DUMMY(&data->timing_cfg, 4);
		if (mspi_timing_config(cfg->bus, &cfg->dev_id, cfg->timing_cfg_mask,
				       (void *)&data->timing_cfg)) {
			LOG_ERR("Failed to config mspi controller/%u", __LINE__);
			return -EIO;
		}
	}

	do {
		LOG_DBG("Reading status register");
		ret = flash_mspi_atxp032_command_read(flash, SPI_NOR_CMD_RDSR, 0, 0, rx_dummy,
						      (uint8_t *)&status, 1);
		if (ret) {
			LOG_ERR("Could not read status");
			return ret;
		}
		LOG_DBG("status: 0x%x", status);
	} while (status & SPI_NOR_WIP_BIT);

	if (data->dev_cfg.io_mode != MSPI_IO_MODE_SINGLE) {
		data->timing_cfg = bkp;
		if (mspi_timing_config(cfg->bus, &cfg->dev_id, cfg->timing_cfg_mask,
				       (void *)&data->timing_cfg)) {
			LOG_ERR("Failed to config mspi controller/%u", __LINE__);
			return -EIO;
		}
	}

	return ret;
}

static int flash_mspi_atxp032_read(const struct device *flash, off_t offset, void *rdata,
				   size_t len)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;
	struct flash_mspi_atxp032_data *data = flash->data;

	int ret;

	acquire(flash);

	data->packet.dir              = MSPI_RX;
	data->packet.cmd              = data->dev_cfg.read_cmd;
	data->packet.address          = offset;
	data->packet.data_buf         = rdata;
	data->packet.num_bytes        = len;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_DMA;
	data->trans.rx_dummy          = data->dev_cfg.rx_dummy;
	data->trans.cmd_length        = data->dev_cfg.cmd_length;
	data->trans.addr_length       = data->dev_cfg.addr_length;
	data->trans.hold_ce           = false;
	data->trans.priority          = 1;
	data->trans.packets           = &data->packet;
	data->trans.num_packet        = 1;
	data->trans.timeout           = CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	LOG_DBG("Read %d bytes from 0x%08zx", len, (ssize_t)offset);

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);
	if (ret) {
		LOG_ERR("MSPI read transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}

	release(flash);

	return ret;
}

static int flash_mspi_atxp032_write(const struct device *flash, off_t offset, const void *wdata,
				    size_t len)
{
	int ret;
	uint8_t *src = (uint8_t *)wdata;
	int i;

	acquire(flash);

	while (len) {
		/* If the offset isn't a multiple of the NOR page size, we first need
		 * to write the remaining part that fits, otherwise the write could
		 * be wrapped around within the same page
		 */
		i = MIN(SPI_NOR_PAGE_SIZE - (offset % SPI_NOR_PAGE_SIZE), len);

		ret = flash_mspi_atxp032_write_enable(flash);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_atxp032_page_program(flash, offset, src, i);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_atxp032_busy_wait(flash);
		if (ret) {
			return ret;
		}

		src += i;
		offset += i;
		len -= i;
	}

	ret = flash_mspi_atxp032_write_disable(flash);
	if (ret) {
		return ret;
	}

	release(flash);

	return ret;
}

static int flash_mspi_atxp032_erase(const struct device *flash, off_t offset, size_t size)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;
	int ret = 0;
	const size_t num_sectors = size / SPI_NOR_SECTOR_SIZE;
	const size_t num_blocks = size / SPI_NOR_BLOCK_SIZE;

	int i;

	acquire(flash);

	if (offset % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid offset");
		return -EINVAL;
	}

	if (size % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}

	if ((offset == 0) && (size == cfg->mem_size)) {
		ret = flash_mspi_atxp032_write_enable(flash);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_atxp032_erase_chip(flash);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_atxp032_busy_wait(flash);
		if (ret) {
			return ret;
		}

	} else if ((0 == (offset % SPI_NOR_BLOCK_SIZE)) && (0 == (size % SPI_NOR_BLOCK_SIZE))) {
		for (i = 0; i < num_blocks; i++) {
			ret = flash_mspi_atxp032_write_enable(flash);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_atxp032_unprotect_sector(flash, offset);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_atxp032_write_enable(flash);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_atxp032_erase_block(flash, offset);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_atxp032_busy_wait(flash);
			if (ret) {
				return ret;
			}

			offset += SPI_NOR_BLOCK_SIZE;
		}
	} else {
		for (i = 0; i < num_sectors; i++) {
			ret = flash_mspi_atxp032_write_enable(flash);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_atxp032_unprotect_sector(flash, offset);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_atxp032_write_enable(flash);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_atxp032_erase_sector(flash, offset);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_atxp032_busy_wait(flash);
			if (ret) {
				return ret;
			}

			offset += SPI_NOR_SECTOR_SIZE;
		}
	}

	release(flash);

	return ret;
}

static const struct flash_parameters *flash_mspi_atxp032_get_parameters(const struct device *flash)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;

	return &cfg->flash_param;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_mspi_atxp032_pages_layout(const struct device *flash,
					    const struct flash_pages_layout **layout,
					    size_t *layout_size)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;

	*layout = &cfg->page_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int flash_mspi_atxp032_init(const struct device *flash)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;
	struct flash_mspi_atxp032_data *data = flash->data;
	uint8_t vendor_id;
	uint32_t CRB3;

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
		LOG_ERR("bus mode %d not supported/%u", cfg->tar_dev_cfg.io_mode, __LINE__);
		return -EIO;
	}

	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL, &cfg->serial_cfg)) {
		LOG_ERR("Failed to config mspi controller/%u", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->serial_cfg;

	if (flash_mspi_atxp032_reset(flash)) {
		LOG_ERR("Could not reset Flash/%u", __LINE__);
		return -EIO;
	}

	if (flash_mspi_atxp032_get_vendor_id(flash, &vendor_id)) {
		LOG_ERR("Could not read vendor id/%u", __LINE__);
		return -EIO;
	}
	LOG_DBG("Vendor id: 0x%0x", vendor_id);
	if (vendor_id != ATXP032_VENDOR_ID) {
		LOG_WRN("Vendor ID does not match expected value of 0x%0x/%u", ATXP032_VENDOR_ID,
			__LINE__);
	}

	if (atxp032_get_dummy_clk((TIMING_CFG_GET_RX_DUMMY(&cfg->tar_timing_cfg)), &CRB3)) {
		return -ENOTSUP;
	}

	if (flash_mspi_atxp032_write_enable(flash)) {
		return -EIO;
	}

	if (flash_mspi_atxp032_command_write(flash, 0x71, 0x3, 1, 0, (uint8_t *)&CRB3, 1)) {
		return -EIO;
	}

	uint8_t cmd;

	if (cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_QUAD) {
		cmd = 0x38;
	} else if (cfg->tar_dev_cfg.io_mode == MSPI_IO_MODE_OCTAL) {
		cmd = 0xe8;
	} else {
		cmd = 0xff;
	}

	if (flash_mspi_atxp032_write_enable(flash)) {
		return -EIO;
	}
	if (flash_mspi_atxp032_command_write(flash, cmd, 0, 0, 0, NULL, 0)) {
		return -EIO;
	}

	if (mspi_dev_config(cfg->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL, &cfg->tar_dev_cfg)) {
		LOG_ERR("Failed to config mspi controller/%u", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->tar_dev_cfg;

	if (mspi_timing_config(cfg->bus, &cfg->dev_id, cfg->timing_cfg_mask,
			       (void *)&cfg->tar_timing_cfg)) {
		LOG_ERR("Failed to config mspi timing/%u", __LINE__);
		return -EIO;
	}
	data->timing_cfg = cfg->tar_timing_cfg;

	if (cfg->tar_xip_cfg.enable) {
		if (mspi_xip_config(cfg->bus, &cfg->dev_id, &cfg->tar_xip_cfg)) {
			LOG_ERR("Failed to enable XIP/%u", __LINE__);
			return -EIO;
		}
		data->xip_cfg = cfg->tar_xip_cfg;
	}

	if (cfg->tar_scramble_cfg.enable) {
		if (mspi_scramble_config(cfg->bus, &cfg->dev_id, &cfg->tar_scramble_cfg)) {
			LOG_ERR("Failed to enable scrambling/%u", __LINE__);
			return -EIO;
		}
		data->scramble_cfg = cfg->tar_scramble_cfg;
	}

	release(flash);

	return 0;
}

#if defined(CONFIG_FLASH_JESD216_API)
static int flash_mspi_atxp032_read_sfdp(const struct device *flash, off_t addr, void *rdata,
					size_t size)
{
	const struct flash_mspi_atxp032_config *cfg = flash->config;
	struct flash_mspi_atxp032_data *data = flash->data;
	int ret;

	acquire(flash);

	data->packet.dir              = MSPI_RX;
	data->packet.cmd              = 0x5A;
	data->packet.address          = addr;
	data->packet.data_buf         = rdata;
	data->packet.num_bytes        = size;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_DMA;
	data->trans.rx_dummy          = 8;
	data->trans.cmd_length        = 1;
	data->trans.addr_length       = 3;
	data->trans.hold_ce           = false;
	data->trans.priority          = 1;
	data->trans.packets           = &data->packet;
	data->trans.num_packet        = 1;
	data->trans.timeout           = CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	LOG_DBG("Read %d bytes from 0x%08zx", size, (ssize_t)addr);

	ret = mspi_transceive(cfg->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->trans);

	if (ret) {
		LOG_ERR("MSPI read transaction failed with code: %d/%u", ret, __LINE__);
		return -EIO;
	}

	release(flash);
	return 0;
}
static int flash_mspi_atxp032_read_jedec_id(const struct device *flash, uint8_t *id)
{
	struct flash_mspi_atxp032_data *data = flash->data;

	id = &data->jedec_id;
	return 0;
}
#endif /* CONFIG_FLASH_JESD216_API */

#if defined(CONFIG_PM_DEVICE)
static int flash_mspi_atxp032_pm_action(const struct device *flash, enum pm_device_action action)
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

static const struct flash_driver_api flash_mspi_atxp032_api = {
	.erase = flash_mspi_atxp032_erase,
	.write = flash_mspi_atxp032_write,
	.read = flash_mspi_atxp032_read,
	.get_parameters = flash_mspi_atxp032_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mspi_atxp032_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = flash_mspi_atxp032_read_sfdp,
	.read_jedec_id = flash_mspi_atxp032_read_jedec_id,
#endif
};

#define MSPI_DEVICE_CONFIG_SERIAL(n)                                                              \
	{                                                                                         \
		.ce_num             = DT_INST_PROP(n, mspi_hardware_ce_num),                      \
		.freq               = 12000000,                                                   \
		.io_mode            = MSPI_IO_MODE_SINGLE,                                        \
		.data_rate          = MSPI_DATA_RATE_SINGLE,                                      \
		.cpp                = MSPI_CPP_MODE_0,                                            \
		.endian             = MSPI_XFER_LITTLE_ENDIAN,                                    \
		.ce_polarity        = MSPI_CE_ACTIVE_LOW,                                         \
		.dqs_enable         = false,                                                      \
		.rx_dummy           = 8,                                                          \
		.tx_dummy           = 0,                                                          \
		.read_cmd           = SPI_NOR_CMD_READ_FAST,                                      \
		.write_cmd          = SPI_NOR_CMD_PP,                                             \
		.cmd_length         = 1,                                                          \
		.addr_length        = 4,                                                          \
		.mem_boundary       = 0,                                                          \
		.time_to_break      = 0,                                                          \
	}

#if CONFIG_SOC_FAMILY_AMBIQ
#define MSPI_TIMING_CONFIG(n)                                                                     \
	{                                                                                         \
		.ui8WriteLatency    = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 0),             \
		.ui8TurnAround      = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 1),             \
		.bTxNeg             = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 2),             \
		.bRxNeg             = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 3),             \
		.bRxCap             = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 4),             \
		.ui32TxDQSDelay     = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 5),             \
		.ui32RxDQSDelay     = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 6),             \
		.ui32RXDQSDelayEXT  = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 7),             \
	}
#define MSPI_TIMING_CONFIG_MASK(n) DT_INST_PROP(n, ambiq_timing_config_mask)
#else
#define MSPI_TIMING_CONFIG(n)
#define MSPI_TIMING_CONFIG_MASK(n)
#endif

#define FLASH_MSPI_ATXP032(n)                                                                     \
	static const struct flash_mspi_atxp032_config flash_mspi_atxp032_config_##n = {           \
		.mem_size = DT_INST_PROP(n, size) / 8,                                            \
		.port        = MSPI_PORT(n),                                                      \
		.flash_param =                                                                    \
			{                                                                         \
				.write_block_size = NOR_WRITE_SIZE,                               \
				.erase_value = NOR_ERASE_VALUE,                                   \
			},                                                                        \
		.page_layout =                                                                    \
			{                                                                         \
				.pages_count = DT_INST_PROP(n, size) / 8 / SPI_NOR_PAGE_SIZE,     \
				.pages_size = SPI_NOR_PAGE_SIZE,                                  \
			},                                                                        \
		.bus                = DEVICE_DT_GET(DT_INST_BUS(n)),                              \
		.dev_id             = MSPI_DEVICE_ID_DT_INST(n),                                  \
		.serial_cfg         = MSPI_DEVICE_CONFIG_SERIAL(n),                               \
		.tar_dev_cfg        = MSPI_DEVICE_CONFIG_DT_INST(n),                              \
		.tar_xip_cfg        = MSPI_XIP_CONFIG_DT_INST(n),                                 \
		.tar_scramble_cfg   = MSPI_SCRAMBLE_CONFIG_DT_INST(n),                            \
		.tar_timing_cfg     = MSPI_TIMING_CONFIG(n),                                      \
		.timing_cfg_mask    = MSPI_TIMING_CONFIG_MASK(n),                                 \
		.sw_multi_periph    = DT_PROP(DT_INST_BUS(n), software_multiperipheral)           \
	};                                                                                        \
	static struct flash_mspi_atxp032_data flash_mspi_atxp032_data_##n = {                     \
		.lock = Z_SEM_INITIALIZER(flash_mspi_atxp032_data_##n.lock, 0, 1),                \
	};                                                                                        \
	PM_DEVICE_DT_INST_DEFINE(n, flash_mspi_atxp032_pm_action);                                \
	DEVICE_DT_INST_DEFINE(n,                                                                  \
			      flash_mspi_atxp032_init,                                            \
			      PM_DEVICE_DT_INST_GET(n),                                           \
			      &flash_mspi_atxp032_data_##n,                                       \
			      &flash_mspi_atxp032_config_##n,                                     \
			      POST_KERNEL,                                                        \
			      CONFIG_FLASH_INIT_PRIORITY,                                         \
			      &flash_mspi_atxp032_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_MSPI_ATXP032)
