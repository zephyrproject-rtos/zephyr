/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mspi_is25xx0xx
/*
 * This driver supports the non-standard 1s-1/8s-8s operation
 * as well as basic 1s-1s-1s operation.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/cache.h>
#include <zephyr/drivers/flash.h>
#include "spi_nor.h"

#if CONFIG_SOC_FAMILY_AMBIQ

#include "mspi_ambiq.h"
typedef struct mspi_ambiq_timing_cfg mspi_timing_cfg;
typedef enum mspi_ambiq_timing_param mspi_timing_param;

#else

typedef struct mspi_timing_cfg mspi_timing_cfg;
typedef enum mspi_timing_param mspi_timing_param;

#endif

LOG_MODULE_REGISTER(flash_mspi_is25xX0xx, CONFIG_FLASH_LOG_LEVEL);

#define NOR_WRITE_SIZE                      1
#define NOR_ERASE_VALUE                     0xff

#define IS25XX0XX_VENDOR_ID                 0x9D

#define IS25XX0XX_DC_DEFAULT                0x1F

#define IS25XX0XX_32KSECTOR_SIZE            0x8000
#define IS25XX0XX_BLOCK_SIZE                0x20000

#define IS25XX0XX_WRITE_VOL_REG_CMD         0x81

enum is25xX0xx_io_mode {
	IS25XX0XX_IO_MODE_EXTENDED_SPI        = 0xFF,
	IS25XX0XX_IO_MODE_EXTENDED_SPI_NONDQS = 0xDF,
};

struct flash_mspi_is25xX0xx_config {
	uint8_t                             port;
	uint32_t                            mem_size;
	struct flash_parameters             flash_param;
	struct flash_pages_layout           page_layout;

	const struct device                 *bus;
	struct mspi_dev_id                  dev_id;
	struct mspi_dev_cfg                 serial_cfg;
	struct mspi_dev_cfg                 tar_dev_cfg;

	MSPI_XIP_CFG_STRUCT_DECLARE(tar_xip_cfg)
	MSPI_XIP_BASE_ADDR_DECLARE(xip_base_addr)
	MSPI_SCRAMBLE_CFG_STRUCT_DECLARE(tar_scramble_cfg)
	MSPI_TIMING_CFG_STRUCT_DECLARE(tar_timing_cfg)
	MSPI_TIMING_PARAM_DECLARE(timing_cfg_mask)

	bool                                sw_multi_periph;

	struct gpio_dt_spec                 reset_gpio;
	uint32_t                            reset_pulse_us;
	uint32_t                            reset_recovery_us;
};

struct flash_mspi_is25xX0xx_data {
	struct mspi_dev_cfg                 dev_cfg;
	struct mspi_xip_cfg                 xip_cfg;
	struct mspi_scramble_cfg            scramble_cfg;
	mspi_timing_cfg                     timing_cfg;
	struct mspi_xfer                    trans;
	struct mspi_xfer_packet             packet;

	struct k_sem                        lock;
	uint8_t                             id[20];
};

static int is25xX0xx_get_dummy_clk(uint8_t rxdummy, uint8_t *dummy_clk)
{
	if (rxdummy == 0 || rxdummy > 30) {
		return 1;
	}
	*dummy_clk = rxdummy;
	return 0;
}

static int flash_mspi_is25xX0xx_enter_command_mode(const struct device *flash)
{
	const struct flash_mspi_is25xX0xx_config *cfg  = flash->config;
	struct flash_mspi_is25xX0xx_data         *data = flash->data;
	int                                       ret;

	if (cfg->serial_cfg.io_mode == data->dev_cfg.io_mode) {
		return 0;
	}

	ret = mspi_dev_config(cfg->bus, &cfg->dev_id,
			      MSPI_DEVICE_CONFIG_ALL, &cfg->serial_cfg);
	if (ret) {
		LOG_ERR("Failed to enter command mode/%u", __LINE__);
		return -EIO;
	}
	return 0;
}

static int flash_mspi_is25xX0xx_exit_command_mode(const struct device *flash)
{
	const struct flash_mspi_is25xX0xx_config *cfg  = flash->config;
	struct flash_mspi_is25xX0xx_data         *data = flash->data;
	int                                       ret;

	if (cfg->serial_cfg.io_mode == data->dev_cfg.io_mode) {
		return 0;
	}

	ret = mspi_dev_config(cfg->bus, &cfg->dev_id,
			      MSPI_DEVICE_CONFIG_ALL, &data->dev_cfg);
	if (ret) {
		LOG_ERR("Failed to exit command mode/%u", __LINE__);
		return -EIO;
	}
	return 0;
}

static int flash_mspi_is25xX0xx_command_write(const struct device *flash, uint8_t cmd,
					      uint32_t addr, uint16_t addr_len, uint32_t tx_dummy,
					      uint8_t *wdata, uint32_t length)
{
	const struct flash_mspi_is25xX0xx_config *cfg  = flash->config;
	struct flash_mspi_is25xX0xx_data         *data = flash->data;
	int                                       ret;

	data->packet.dir              = MSPI_TX;
	data->packet.cmd              = cmd;
	data->packet.address          = addr;
	data->packet.data_buf         = wdata;
	data->packet.num_bytes        = length;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_PIO;
	data->trans.tx_dummy          = tx_dummy;
	data->trans.rx_dummy          = data->dev_cfg.rx_dummy;
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

static int flash_mspi_is25xX0xx_command_read(const struct device *flash, uint8_t cmd,
					     uint32_t addr, uint16_t addr_len, uint32_t rx_dummy,
					     uint8_t *rdata, uint32_t length)
{
	const struct flash_mspi_is25xX0xx_config *cfg  = flash->config;
	struct flash_mspi_is25xX0xx_data         *data = flash->data;
	int                                       ret;

	data->packet.dir              = MSPI_RX;
	data->packet.cmd              = cmd;
	data->packet.address          = addr;
	data->packet.data_buf         = rdata;
	data->packet.num_bytes        = length;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_PIO;
	data->trans.rx_dummy          = rx_dummy;
	data->trans.tx_dummy          = data->dev_cfg.tx_dummy;
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
	const struct flash_mspi_is25xX0xx_config *cfg  = flash->config;
	struct flash_mspi_is25xX0xx_data         *data = flash->data;

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
	const struct flash_mspi_is25xX0xx_config *cfg  = flash->config;
	struct flash_mspi_is25xX0xx_data         *data = flash->data;

	while (mspi_get_channel_status(cfg->bus, cfg->port)) {
		;
	}

	k_sem_give(&data->lock);
}

static int flash_mspi_is25xX0xx_write_enable(const struct device *flash)
{
	int ret;

	LOG_DBG("Enabling write");
	ret = flash_mspi_is25xX0xx_command_write(flash, SPI_NOR_CMD_WREN, 0, 0, 0, NULL, 0);

	return ret;
}

static int flash_mspi_is25xX0xx_write_disable(const struct device *flash)
{
	int ret;

	LOG_DBG("Disabling write");
	ret = flash_mspi_is25xX0xx_command_write(flash, SPI_NOR_CMD_WRDI, 0, 0, 0, NULL, 0);

	return ret;
}

static int flash_mspi_is25xX0xx_is_ready(const struct device *flash)
{
	uint32_t flag_stat = 0;
	uint32_t rx_dummy  = 0;
	uint32_t timeout   = 400; /* max tSSE */
	int      ret;

	do {
		LOG_DBG("Reading flag status register");
		ret = flash_mspi_is25xX0xx_command_read(flash, 0x70, 0, 0, rx_dummy,
							(uint8_t *)&flag_stat, 1);
		if (ret) {
			LOG_ERR("Could not read flag status");
			return ret;
		}

		LOG_DBG("flag status: 0x%x", flag_stat);
		if (flag_stat & BIT(7)) {
			LOG_DBG("Device is ready");
			break;
		}

		k_sleep(K_MSEC(1));
		timeout--;
	} while (timeout);

	if (timeout == 0) {
		LOG_ERR("Operation timed out");
		return -ETIMEDOUT;
	}

	return ret;
}

static int flash_mspi_is25xX0xx_reset(const struct device *flash)
{
	const struct flash_mspi_is25xX0xx_config *cfg = flash->config;
	int                                       ret;

	LOG_DBG("RESETTING");

	if (cfg->reset_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("Device %s is not ready",
				cfg->reset_gpio.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio,
					   GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to activate RESET: %d", ret);
			return -EIO;
		}

		if (cfg->reset_pulse_us != 0) {
			k_busy_wait(cfg->reset_pulse_us);
		}

		ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Failed to deactivate RESET: %d", ret);
			return -EIO;
		}

		if (cfg->reset_recovery_us != 0) {
			k_busy_wait(cfg->reset_recovery_us);
		}
	} else {
		ret = flash_mspi_is25xX0xx_command_write(flash, SPI_NOR_CMD_RESET_EN,
							 0, 0, 0, NULL, 0);
		if (ret) {
			return ret;
		}
		ret = flash_mspi_is25xX0xx_command_write(flash, SPI_NOR_CMD_RESET_MEM,
							 0, 0, 0, NULL, 0);
		if (ret) {
			return ret;
		}
	}

	ret = flash_mspi_is25xX0xx_is_ready(flash);

	return ret;
}

static int flash_mspi_is25xX0xx_get_vendor_id(const struct device *flash, uint8_t *vendor_id)
{
	struct flash_mspi_is25xX0xx_data *data = flash->data;
	int                               ret;

	if (vendor_id == NULL) {
		return -EINVAL;
	}

	LOG_DBG("Reading id");
	/* serial mode */
	ret = flash_mspi_is25xX0xx_command_read(flash, SPI_NOR_CMD_RDID, 0, 0, 0,
						(uint8_t *)data->id, 20);
	*vendor_id = data->id[0];

	return ret;
}

static int flash_mspi_is25xX0xx_erase_sector(const struct device *flash, off_t addr)
{
	struct flash_mspi_is25xX0xx_data *data = flash->data;
	int                               ret;

	LOG_DBG("Erasing sector at 0x%08zx", (ssize_t)addr);

	ret = flash_mspi_is25xX0xx_command_write(flash, SPI_NOR_CMD_SE, addr,
						 data->dev_cfg.addr_length, 0, NULL, 0);

	return ret;
}

static int flash_mspi_is25xX0xx_erase_32k_sector(const struct device *flash, off_t addr)
{
	struct flash_mspi_is25xX0xx_data *data = flash->data;
	int                               ret;

	LOG_DBG("Erasing sector at 0x%08zx", (ssize_t)addr);

	ret = flash_mspi_is25xX0xx_command_write(flash, SPI_NOR_CMD_BE_32K, addr,
						 data->dev_cfg.addr_length, 0, NULL, 0);

	return ret;
}

static int flash_mspi_is25xX0xx_erase_block(const struct device *flash, off_t addr)
{
	struct flash_mspi_is25xX0xx_data *data = flash->data;
	int                               ret;

	LOG_DBG("Erasing block at 0x%08zx", (ssize_t)addr);

	ret = flash_mspi_is25xX0xx_command_write(flash, SPI_NOR_CMD_BE, addr,
						 data->dev_cfg.addr_length, 0, NULL, 0);

	return ret;
}

static int flash_mspi_is25xX0xx_erase_chip(const struct device *flash)
{
	int ret;

	LOG_DBG("Erasing chip");

	ret = flash_mspi_is25xX0xx_command_write(flash, SPI_NOR_CMD_CE, 0, 0, 0, NULL, 0);

	return ret;
}

static int flash_mspi_is25xX0xx_page_program(const struct device *flash, off_t offset,
					     void *wdata, size_t len)
{
	const struct flash_mspi_is25xX0xx_config *cfg  = flash->config;
	struct flash_mspi_is25xX0xx_data         *data = flash->data;
	int                                       ret;

	data->packet.dir              = MSPI_TX;
	data->packet.cmd              = data->dev_cfg.write_cmd;
	data->packet.address          = offset;
	data->packet.data_buf         = wdata;
	data->packet.num_bytes        = len;

	data->trans.async             = false;
	data->trans.xfer_mode         = MSPI_DMA;
	data->trans.tx_dummy          = data->dev_cfg.tx_dummy;
	data->trans.rx_dummy          = data->dev_cfg.rx_dummy;
	data->trans.cmd_length        = data->dev_cfg.cmd_length;
	data->trans.addr_length       = data->dev_cfg.addr_length;
	data->trans.hold_ce           = false;
	data->trans.priority          = MSPI_XFER_PRIORITY_MEDIUM;
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

static int flash_mspi_is25xX0xx_busy_wait(const struct device *flash, unsigned int timeout)
{
	uint32_t status    = 0;
	uint32_t flag_stat = 0;
	uint32_t rx_dummy  = 0;
	int      ret;

	do {
		LOG_DBG("Reading status register");
		ret = flash_mspi_is25xX0xx_command_read(flash, SPI_NOR_CMD_RDSR, 0, 0, rx_dummy,
							(uint8_t *)&status, 1);
		if (ret) {
			LOG_ERR("Could not read status");
			return ret;
		}
		ret = flash_mspi_is25xX0xx_command_read(flash, 0x70, 0, 0, rx_dummy,
							(uint8_t *)&flag_stat, 1);
		if (ret) {
			LOG_ERR("Could not read flag status");
			return ret;
		}
		LOG_DBG("status: 0x%x, flag status: 0x%x", status, flag_stat);

		if (flag_stat & BIT(1)) {
			LOG_ERR("Access denied");
			return -EACCES;
		} else if (flag_stat & BIT(4)) {
			LOG_ERR("Program operation failed");
			return -EIO;
		} else if (flag_stat & BIT(5)) {
			LOG_ERR("Erase operation failed");
			return -EIO;
		}

		k_sleep(K_MSEC(1));
		timeout--;
	} while ((status & SPI_NOR_WIP_BIT) && timeout);

	if (timeout == 0) {
		LOG_ERR("Operation timed out");
		return -ETIMEDOUT;
	}

	return ret;
}

static int flash_mspi_is25xX0xx_read(const struct device *flash, off_t offset, void *rdata,
				     size_t len)
{
	const struct flash_mspi_is25xX0xx_config *cfg  = flash->config;
	struct flash_mspi_is25xX0xx_data         *data = flash->data;
	int                                       ret;

	acquire(flash);

#if CONFIG_FLASH_MSPI_XIP_READ
	if (cfg->tar_xip_cfg.enable) {
		uint32_t xip_addr = cfg->xip_base_addr + cfg->tar_xip_cfg.address_offset + offset;

		memcpy(rdata, (void *)xip_addr, len);
	} else {
#endif /* CONFIG_FLASH_MSPI_XIP_READ */

#if CONFIG_FLASH_MSPI_HANDLE_CACHE
		if (!buf_in_nocache((uintptr_t)rdata, len)) {
			if (len > CONFIG_FLASH_MSPI_RANGE_HANDLE_CACHE_SIZE) {
				sys_cache_data_flush_all();
			} else {
				sys_cache_data_flush_range(rdata, len);
			}
		}
#endif /* CONFIG_FLASH_MSPI_HANDLE_CACHE */

		data->packet.dir              = MSPI_RX;
		data->packet.cmd              = data->dev_cfg.read_cmd;
		data->packet.address          = offset;
		data->packet.data_buf         = rdata;
		data->packet.num_bytes        = len;

		data->trans.async             = false;
		data->trans.xfer_mode         = MSPI_DMA;
		data->trans.tx_dummy          = data->dev_cfg.tx_dummy;
		data->trans.rx_dummy          = data->dev_cfg.rx_dummy;
		data->trans.cmd_length        = data->dev_cfg.cmd_length;
		data->trans.addr_length       = data->dev_cfg.addr_length;
		data->trans.hold_ce           = false;
		data->trans.priority          = MSPI_XFER_PRIORITY_MEDIUM;
		data->trans.packets           = &data->packet;
		data->trans.num_packet        = 1;
		data->trans.timeout           = CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

		LOG_DBG("Read %d bytes from 0x%08zx", len, (ssize_t)offset);

		ret = mspi_transceive(cfg->bus, &cfg->dev_id,
				      (const struct mspi_xfer *)&data->trans);
		if (ret) {
			LOG_ERR("MSPI read transaction failed with code: %d/%u", ret, __LINE__);
			return -EIO;
		}

#if CONFIG_FLASH_MSPI_HANDLE_CACHE
		if (!buf_in_nocache((uintptr_t)rdata, len)) {
			if (len > CONFIG_FLASH_MSPI_RANGE_HANDLE_CACHE_SIZE) {
				sys_cache_data_flush_and_invd_all();
			} else {
				sys_cache_data_invd_range(rdata, len);
			}
		}
#endif /* CONFIG_FLASH_MSPI_HANDLE_CACHE */
#if CONFIG_FLASH_MSPI_XIP_READ
	}
#endif /* CONFIG_FLASH_MSPI_XIP_READ */

	release(flash);

	return ret;
}

static int flash_mspi_is25xX0xx_write(const struct device *flash, off_t offset, const void *wdata,
				      size_t len)
{
	int      ret;
	uint8_t *src = (uint8_t *)wdata;
	int      i;
#if CONFIG_FLASH_MSPI_HANDLE_CACHE && CONFIG_FLASH_MSPI_XIP_READ
	off_t  addr = offset;
	size_t size = len;
#endif

	acquire(flash);

#if CONFIG_FLASH_MSPI_HANDLE_CACHE
	if (!buf_in_nocache((uintptr_t)src, len)) {
		if (len > CONFIG_FLASH_MSPI_RANGE_HANDLE_CACHE_SIZE) {
			sys_cache_data_flush_all();
		} else {
			sys_cache_data_flush_range(src, len);
		}
	}
#endif

	while (len) {
		/* If the offset isn't a multiple of the NOR page size, we first need
		 * to write the remaining part that fits, otherwise the write could
		 * be wrapped around within the same page
		 */
		i = MIN(SPI_NOR_PAGE_SIZE - (offset % SPI_NOR_PAGE_SIZE), len);

		ret = flash_mspi_is25xX0xx_enter_command_mode(flash);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_is25xX0xx_write_enable(flash);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_is25xX0xx_exit_command_mode(flash);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_is25xX0xx_page_program(flash, offset, src, i);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_is25xX0xx_enter_command_mode(flash);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_is25xX0xx_busy_wait(flash, 3);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_is25xX0xx_exit_command_mode(flash);
		if (ret) {
			return ret;
		}

		src    += i;
		offset += i;
		len    -= i;
	}

	ret = flash_mspi_is25xX0xx_write_disable(flash);
	if (ret) {
		return ret;
	}

#if CONFIG_FLASH_MSPI_HANDLE_CACHE && CONFIG_FLASH_MSPI_XIP_READ
	const struct flash_mspi_is25xX0xx_config *cfg = flash->config;

	if (cfg->tar_xip_cfg.enable) {
		uint32_t xip_addr = cfg->xip_base_addr + cfg->tar_xip_cfg.address_offset + addr;

		if (!buf_in_nocache((uintptr_t)xip_addr, size)) {
			if (size > CONFIG_FLASH_MSPI_RANGE_HANDLE_CACHE_SIZE) {
				sys_cache_data_flush_and_invd_all();
			} else {
				sys_cache_data_invd_range((void *)xip_addr, size);
			}
		}
	}
#endif

	release(flash);

	return ret;
}

static int flash_mspi_is25xX0xx_erase(const struct device *flash, off_t offset, size_t size)
{
	const struct flash_mspi_is25xX0xx_config *cfg = flash->config;
	int          ret             = 0;
	const size_t num_sectors     = size / SPI_NOR_SECTOR_SIZE;
	const size_t num_32k_sectors = size / IS25XX0XX_32KSECTOR_SIZE;
	const size_t num_blocks      = size / IS25XX0XX_BLOCK_SIZE;
	int          i;

	acquire(flash);

	if (offset % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid offset");
		return -EINVAL;
	}

	if (size % SPI_NOR_SECTOR_SIZE) {
		LOG_ERR("Invalid size");
		return -EINVAL;
	}

	ret = flash_mspi_is25xX0xx_enter_command_mode(flash);
	if (ret) {
		return ret;
	}

	if ((offset == 0) && (size == cfg->mem_size)) {
		ret = flash_mspi_is25xX0xx_write_enable(flash);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_is25xX0xx_erase_chip(flash);
		if (ret) {
			return ret;
		}

		ret = flash_mspi_is25xX0xx_busy_wait(flash, 45000);
		if (ret) {
			return ret;
		}
	} else if ((0 == (offset % IS25XX0XX_BLOCK_SIZE)) &&
		   (0 == (size % IS25XX0XX_BLOCK_SIZE))) {
		for (i = 0; i < num_blocks; i++) {
			ret = flash_mspi_is25xX0xx_write_enable(flash);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_is25xX0xx_erase_block(flash, offset);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_is25xX0xx_busy_wait(flash, 1000);
			if (ret) {
				return ret;
			}

			offset += IS25XX0XX_BLOCK_SIZE;
		}
	} else if ((0 == (offset % IS25XX0XX_32KSECTOR_SIZE)) &&
		   (0 == (size % IS25XX0XX_32KSECTOR_SIZE))) {
		for (i = 0; i < num_32k_sectors; i++) {
			ret = flash_mspi_is25xX0xx_write_enable(flash);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_is25xX0xx_erase_32k_sector(flash, offset);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_is25xX0xx_busy_wait(flash, 1000);
			if (ret) {
				return ret;
			}

			offset += IS25XX0XX_32KSECTOR_SIZE;
		}
	} else {
		for (i = 0; i < num_sectors; i++) {
			ret = flash_mspi_is25xX0xx_write_enable(flash);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_is25xX0xx_erase_sector(flash, offset);
			if (ret) {
				return ret;
			}

			ret = flash_mspi_is25xX0xx_busy_wait(flash, 400);
			if (ret) {
				return ret;
			}

			offset += SPI_NOR_SECTOR_SIZE;
		}
	}

	ret = flash_mspi_is25xX0xx_exit_command_mode(flash);
	if (ret) {
		return ret;
	}

	release(flash);

	return ret;
}

static const struct flash_parameters *
flash_mspi_is25xX0xx_get_parameters(const struct device *flash)
{
	const struct flash_mspi_is25xX0xx_config *cfg = flash->config;

	return &cfg->flash_param;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_mspi_is25xX0xx_pages_layout(const struct device              *flash,
					      const struct flash_pages_layout **layout,
					      size_t                           *layout_size)
{
	const struct flash_mspi_is25xX0xx_config *cfg = flash->config;

	*layout      = &cfg->page_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int flash_mspi_is25xX0xx_init(const struct device *flash)
{
	const struct flash_mspi_is25xX0xx_config *cfg  = flash->config;
	struct flash_mspi_is25xX0xx_data         *data = flash->data;
	uint8_t                                   vendor_id;
	uint8_t                                   reg_dummy;
	uint8_t                                   reg_io_mode;

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("Controller device is not ready.");
		return -ENODEV;
	}

	switch (cfg->tar_dev_cfg.io_mode) {
	case MSPI_IO_MODE_SINGLE:
	case MSPI_IO_MODE_OCTAL_1_1_8:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		reg_io_mode = IS25XX0XX_IO_MODE_EXTENDED_SPI;
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

	if (flash_mspi_is25xX0xx_reset(flash)) {
		LOG_ERR("Could not reset Flash/%u", __LINE__);
		return -EIO;
	}

	if (flash_mspi_is25xX0xx_get_vendor_id(flash, &vendor_id)) {
		LOG_ERR("Could not read vendor id/%u", __LINE__);
		return -EIO;
	}
	LOG_DBG("Vendor id: 0x%0x", vendor_id);
	if (vendor_id != IS25XX0XX_VENDOR_ID) {
		LOG_WRN("Vendor ID does not match expected value of 0x%0x/%u",
			IS25XX0XX_VENDOR_ID, __LINE__);
	}

	if (is25xX0xx_get_dummy_clk(cfg->tar_dev_cfg.rx_dummy, &reg_dummy)) {
		return -ENOTSUP;
	}

	if (flash_mspi_is25xX0xx_write_enable(flash)) {
		return -EIO;
	}

	if (flash_mspi_is25xX0xx_command_write(flash, IS25XX0XX_WRITE_VOL_REG_CMD,
						0x1, 1, 0, &reg_dummy, 1)) {
		return -EIO;
	}

	if (!cfg->tar_dev_cfg.dqs_enable) {
		reg_io_mode = IS25XX0XX_IO_MODE_EXTENDED_SPI_NONDQS;
		if (flash_mspi_is25xX0xx_write_enable(flash)) {
			return -EIO;
		}
		if (flash_mspi_is25xX0xx_command_write(flash, IS25XX0XX_WRITE_VOL_REG_CMD,
							0x0, 1, 0, &reg_io_mode, 1)) {
			return -EIO;
		}
	}

	if (cfg->tar_dev_cfg.addr_length == 4) {
		LOG_DBG("Enter 4 byte address mode");
		if (flash_mspi_is25xX0xx_write_enable(flash)) {
			return -EIO;
		}
		if (flash_mspi_is25xX0xx_command_write(flash, SPI_NOR_CMD_4BA,
							0, 0, 0, NULL, 0)) {
			return -EIO;
		}
	}

	if (mspi_dev_config(cfg->bus, &cfg->dev_id,
			    MSPI_DEVICE_CONFIG_ALL, &cfg->tar_dev_cfg)) {
		LOG_ERR("Failed to config mspi controller/%u", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->tar_dev_cfg;

#if CONFIG_MSPI_TIMING
	if (mspi_timing_config(cfg->bus, &cfg->dev_id, cfg->timing_cfg_mask,
				(void *)&cfg->tar_timing_cfg)) {
		LOG_ERR("Failed to config mspi timing/%u", __LINE__);
		return -EIO;
	}
	data->timing_cfg = cfg->tar_timing_cfg;
#endif

#if CONFIG_MSPI_XIP
	if (cfg->tar_xip_cfg.enable) {
		if (mspi_xip_config(cfg->bus, &cfg->dev_id, &cfg->tar_xip_cfg)) {
			LOG_ERR("Failed to enable XIP/%u", __LINE__);
			return -EIO;
		}
		data->xip_cfg = cfg->tar_xip_cfg;
	}
#endif

#if CONFIG_MSPI_SCRAMBLE
	if (cfg->tar_scramble_cfg.enable) {
		if (mspi_scramble_config(cfg->bus, &cfg->dev_id, &cfg->tar_scramble_cfg)) {
			LOG_ERR("Failed to enable scrambling/%u", __LINE__);
			return -EIO;
		}
		data->scramble_cfg = cfg->tar_scramble_cfg;
	}
#endif

	release(flash);

	return 0;
}

#if defined(CONFIG_FLASH_JESD216_API)
static int flash_mspi_is25xX0xx_read_sfdp(const struct device *flash, off_t addr, void *rdata,
					  size_t size)
{
	const struct flash_mspi_is25xX0xx_config *cfg  = flash->config;
	struct flash_mspi_is25xX0xx_data         *data = flash->data;
	int                                       ret;

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
	data->trans.priority          = MSPI_XFER_PRIORITY_MEDIUM;
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
static int flash_mspi_is25xX0xx_read_jedec_id(const struct device *flash, uint8_t *id)
{
	struct flash_mspi_is25xX0xx_data *data = flash->data;

	id = &data->id;
	return 0;
}
#endif /* CONFIG_FLASH_JESD216_API */

static DEVICE_API(flash, flash_mspi_is25xX0xx_api) = {
	.erase          = flash_mspi_is25xX0xx_erase,
	.write          = flash_mspi_is25xX0xx_write,
	.read           = flash_mspi_is25xX0xx_read,
	.get_parameters = flash_mspi_is25xX0xx_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout    = flash_mspi_is25xX0xx_pages_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read      = flash_mspi_is25xX0xx_read_sfdp,
	.read_jedec_id  = flash_mspi_is25xX0xx_read_jedec_id,
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
		.addr_length        = 3,                                                          \
		.mem_boundary       = 0,                                                          \
		.time_to_break      = 0,                                                          \
	}

#define MSPI_TIMING_CONFIG(n)                                                                     \
	COND_CODE_1(CONFIG_SOC_FAMILY_AMBIQ,                                                      \
		(MSPI_AMBIQ_TIMING_CONFIG(n)), ({}))

#define MSPI_TIMING_CONFIG_MASK(n)                                                                \
	COND_CODE_1(CONFIG_SOC_FAMILY_AMBIQ,                                                      \
		(MSPI_AMBIQ_TIMING_CONFIG_MASK(n)), (MSPI_TIMING_PARAM_DUMMY))

#define MSPI_PORT(n)                                                                              \
	COND_CODE_1(CONFIG_SOC_FAMILY_AMBIQ,                                                      \
		(MSPI_AMBIQ_PORT(n)), (0))

#define FLASH_MSPI_IS25XX0XX(n)                                                                   \
	static const struct flash_mspi_is25xX0xx_config flash_mspi_is25xX0xx_config_##n = {       \
		.mem_size    = DT_INST_PROP(n, size) / 8,                                         \
		.port        = MSPI_PORT(n),                                                      \
		.flash_param =                                                                    \
			{                                                                         \
				.write_block_size = NOR_WRITE_SIZE,                               \
				.erase_value      = NOR_ERASE_VALUE,                              \
			},                                                                        \
		.page_layout =                                                                    \
			{                                                                         \
				.pages_count = DT_INST_PROP(n, size) / 8 / SPI_NOR_PAGE_SIZE,     \
				.pages_size  = SPI_NOR_PAGE_SIZE,                                 \
			},                                                                        \
		.bus                = DEVICE_DT_GET(DT_INST_BUS(n)),                              \
		.dev_id             = MSPI_DEVICE_ID_DT_INST(n),                                  \
		.serial_cfg         = MSPI_DEVICE_CONFIG_SERIAL(n),                               \
		.tar_dev_cfg        = MSPI_DEVICE_CONFIG_DT_INST(n),                              \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_XIP,                                    \
					      tar_xip_cfg, MSPI_XIP_CONFIG_DT_INST(n))            \
		MSPI_XIP_BASE_ADDR_INIT(xip_base_addr, DT_INST_BUS(n))                            \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_SCRAMBLE,                               \
					      tar_scramble_cfg, MSPI_SCRAMBLE_CONFIG_DT_INST(n))  \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_TIMING,                                 \
					      tar_timing_cfg, MSPI_TIMING_CONFIG(n))              \
		MSPI_OPTIONAL_CFG_STRUCT_INIT(CONFIG_MSPI_TIMING,                                 \
					      timing_cfg_mask, MSPI_TIMING_CONFIG_MASK(n))        \
		.sw_multi_periph    = DT_PROP(DT_INST_BUS(n), software_multiperipheral),          \
		.reset_gpio         = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),              \
		.reset_pulse_us     = DT_INST_PROP_OR(n, t_reset_pulse, 0),                       \
		.reset_recovery_us  = DT_INST_PROP_OR(n, t_reset_recovery, 0),                    \
	};                                                                                        \
	static struct flash_mspi_is25xX0xx_data flash_mspi_is25xX0xx_data_##n = {                 \
		.lock = Z_SEM_INITIALIZER(flash_mspi_is25xX0xx_data_##n.lock, 0, 1),              \
	};                                                                                        \
	DEVICE_DT_INST_DEFINE(n,                                                                  \
			      flash_mspi_is25xX0xx_init,                                          \
			      NULL,                                                               \
			      &flash_mspi_is25xX0xx_data_##n,                                     \
			      &flash_mspi_is25xX0xx_config_##n,                                   \
			      POST_KERNEL,                                                        \
			      CONFIG_FLASH_INIT_PRIORITY,                                         \
			      &flash_mspi_is25xX0xx_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_MSPI_IS25XX0XX)
