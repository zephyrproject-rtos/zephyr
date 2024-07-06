/*
 * Copyright (c) 2024 Ambiq Micro Inc. <www.ambiq.com>
 * SPDX-License-Identifier: Apache-2.0
 * Emulate a memory device on MSPI emulator bus
 */
#define DT_DRV_COMPAT zephyr_mspi_emul_flash

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/mspi_emul.h>
#include <zephyr/drivers/flash.h>
#include "spi_nor.h"

LOG_MODULE_REGISTER(zephyr_mspi_emul_flash, CONFIG_FLASH_LOG_LEVEL);

/* add else if for other SoC platforms */
#if defined(CONFIG_SOC_POSIX)
typedef struct mspi_timing_cfg mspi_timing_cfg;
typedef enum mspi_timing_param mspi_timing_param;
#endif

struct flash_mspi_emul_device_config {
	uint32_t                            size;
	struct flash_parameters             flash_param;
	struct flash_pages_layout           page_layout;

	struct mspi_dev_id                  dev_id;
	struct mspi_dev_cfg                 tar_dev_cfg;
	struct mspi_xip_cfg                 tar_xip_cfg;
	struct mspi_scramble_cfg            tar_scramble_cfg;

	bool                                sw_multi_periph;
};

struct flash_mspi_emul_device_data {
	const struct device                 *bus;
	struct mspi_dev_cfg                 dev_cfg;
	struct mspi_xip_cfg                 xip_cfg;
	struct mspi_scramble_cfg            scramble_cfg;
	mspi_timing_cfg                     timing_cfg;

	struct mspi_xfer                    xfer;
	struct mspi_xfer_packet             packet;

	struct k_sem                        lock;
	uint8_t                             *mem;
};

/**
 * Acquire the device lock.
 *
 * @param flash MSPI emulation flash device.
 */
static void acquire(const struct device *flash)
{
	const struct flash_mspi_emul_device_config *cfg = flash->config;
	struct flash_mspi_emul_device_data *data = flash->data;

	k_sem_take(&data->lock, K_FOREVER);
	if (cfg->sw_multi_periph) {
		while (mspi_dev_config(data->bus, &cfg->dev_id,
				       MSPI_DEVICE_CONFIG_ALL, &data->dev_cfg))
			;
	} else {
		while (mspi_dev_config(data->bus, &cfg->dev_id,
				       MSPI_DEVICE_CONFIG_NONE, NULL))
			;

	}
}

/**
 * Release the device lock.
 *
 * @param flash MSPI emulation flash device.
 */
static void release(const struct device *flash)
{
	struct flash_mspi_emul_device_data *data = flash->data;

	while (mspi_get_channel_status(data->bus, 0)) {

	}

	k_sem_give(&data->lock);
}

/**
 * API implementation of emul_mspi_dev_api_transceive transceive.
 *
 * @param target Pointer to MSPI device emulator.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param xfer Pointer to the MSPI transfer started by dev_id.
 *
 * @retval 0 if successful.
 * @retval -ESTALE device ID don't match, need to call mspi_dev_config first.
 * @retval -Error transfer failed.
 */
static int emul_mspi_device_transceive(const struct emul *target,
				       const struct mspi_xfer_packet *packets,
				       uint32_t num_packet,
				       bool async,
				       uint32_t timeout)
{
	ARG_UNUSED(timeout);
	const struct flash_mspi_emul_device_config *cfg = target->dev->config;
	struct flash_mspi_emul_device_data *data = target->dev->data;
	struct emul_mspi_driver_api *api = (struct emul_mspi_driver_api *)data->bus->api;

	__ASSERT_NO_MSG(api);
	__ASSERT_NO_MSG(api->trigger_event);

	for (uint32_t count = 0; count < num_packet; ++count) {
		const struct mspi_xfer_packet *packet = &packets[count];

		if (packet->address > cfg->size ||
		    packet->address + packet->num_bytes > cfg->size) {
			return -ENOMEM;
		}

		if (packet->dir == MSPI_RX) {
			memcpy(packet->data_buf, data->mem + packet->address,
			       packet->num_bytes);
		} else if (packet->dir == MSPI_TX) {
			memcpy(data->mem + packet->address, packet->data_buf,
			       packet->num_bytes);
		}

		if (async) {
			if (packet->cb_mask == MSPI_BUS_XFER_COMPLETE_CB) {
				api->trigger_event(data->bus, MSPI_BUS_XFER_COMPLETE);
			}
		}
	}

	return 0;
}

static int flash_mspi_emul_erase(const struct device *flash, k_off_t offset, size_t size)
{
	const struct flash_mspi_emul_device_config *cfg = flash->config;
	struct flash_mspi_emul_device_data *data = flash->data;

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

	if ((offset == 0) && (size == cfg->size)) {

		memset(data->mem, cfg->flash_param.erase_value, size);

	} else if ((0 == (offset % SPI_NOR_BLOCK_SIZE)) && (0 == (size % SPI_NOR_BLOCK_SIZE))) {
		for (i = 0; i < num_blocks; i++) {
			memset(data->mem + offset, cfg->flash_param.erase_value,
			       SPI_NOR_BLOCK_SIZE);
			offset += SPI_NOR_BLOCK_SIZE;
		}
	} else {
		for (i = 0; i < num_sectors; i++) {
			memset(data->mem + offset, cfg->flash_param.erase_value,
			       SPI_NOR_SECTOR_SIZE);
			offset += SPI_NOR_SECTOR_SIZE;
		}
	}

	release(flash);

	return 0;
}

/**
 * API implementation of flash write.
 *
 * @param flash Pointer to MSPI flash device.
 * @param offset Flash device address.
 * @param wdata Pointer to the write data buffer.
 * @param len Number of bytes to write.
 *
 * @retval 0 if successful.
 * @retval -Error flash read fail.
 */
static int flash_mspi_emul_write(const struct device *flash, k_off_t offset,
				 const void *wdata, size_t len)
{
	const struct flash_mspi_emul_device_config *cfg = flash->config;
	struct flash_mspi_emul_device_data *data = flash->data;

	int ret;
	uint8_t *src = (uint8_t *)wdata;
	int i;

	acquire(flash);

	data->xfer.async               = false;
	data->xfer.xfer_mode           = MSPI_DMA;
	data->xfer.tx_dummy            = data->dev_cfg.tx_dummy;
	data->xfer.cmd_length          = data->dev_cfg.cmd_length;
	data->xfer.addr_length         = data->dev_cfg.addr_length;
	data->xfer.hold_ce             = false;
	data->xfer.priority            = 1;
	data->xfer.packets             = &data->packet;
	data->xfer.num_packet          = 1;
	data->xfer.timeout             = CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	while (len) {
		/* If the offset isn't a multiple of the NOR page size, we first need
		 * to write the remaining part that fits, otherwise the write could
		 * be wrapped around within the same page
		 */
		i = MIN(SPI_NOR_PAGE_SIZE - (offset % SPI_NOR_PAGE_SIZE), len);

		data->packet.dir               = MSPI_TX;
		data->packet.cmd               = data->dev_cfg.write_cmd;
		data->packet.address           = offset;
		data->packet.data_buf          = src;
		data->packet.num_bytes         = i;

		LOG_DBG("Write %d bytes to 0x%08zx", i, (k_ssize_t)offset);

		ret = mspi_transceive(data->bus, &cfg->dev_id,
				      (const struct mspi_xfer *)&data->xfer);
		if (ret) {
			LOG_ERR("%u, MSPI write transaction failed with code: %d", __LINE__, ret);
			return -EIO;
		}

		/* emulate flash write busy wait */
		k_busy_wait(100);

		src += i;
		offset += i;
		len -= i;
	}

	release(flash);

	return ret;
}

/**
 * API implementation of flash read.
 *
 * @param flash Pointer to MSPI flash device.
 * @param offset Flash device address.
 * @param rdata Pointer to the read data buffer.
 * @param len Number of bytes to read.
 *
 * @retval 0 if successful.
 * @retval -Error flash read fail.
 */
static int flash_mspi_emul_read(const struct device *flash, k_off_t offset,
				void *rdata, size_t len)
{
	const struct flash_mspi_emul_device_config *cfg = flash->config;
	struct flash_mspi_emul_device_data *data = flash->data;

	int ret;

	acquire(flash);

	data->packet.dir               = MSPI_RX;
	data->packet.cmd               = data->dev_cfg.read_cmd;
	data->packet.address           = offset;
	data->packet.data_buf          = rdata;
	data->packet.num_bytes         = len;

	data->xfer.async               = false;
	data->xfer.xfer_mode           = MSPI_DMA;
	data->xfer.rx_dummy            = data->dev_cfg.rx_dummy;
	data->xfer.cmd_length          = data->dev_cfg.cmd_length;
	data->xfer.addr_length         = data->dev_cfg.addr_length;
	data->xfer.hold_ce             = false;
	data->xfer.priority            = 1;
	data->xfer.packets             = &data->packet;
	data->xfer.num_packet          = 1;
	data->xfer.timeout             = CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	LOG_DBG("Read %d bytes from 0x%08zx", len, (k_ssize_t)offset);

	ret = mspi_transceive(data->bus, &cfg->dev_id, (const struct mspi_xfer *)&data->xfer);
	if (ret) {
		LOG_ERR("%u, MSPI read transaction failed with code: %d", __LINE__, ret);
		return -EIO;
	}

	release(flash);

	return ret;
}

/**
 * API implementation of flash get_parameters.
 *
 * @param flash Pointer to MSPI flash device.
 *
 * @retval @ref flash_parameters.
 */
static const struct flash_parameters *flash_mspi_emul_get_parameters(const struct device *flash)
{
	const struct flash_mspi_emul_device_config *cfg = flash->config;

	return &cfg->flash_param;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
/**
 * API implementation of flash pages_layout.
 *
 * @param flash Pointer to MSPI flash device.
 * @param layout @ref flash_pages_layout.
 * @param layout_size
 */
static void flash_mspi_emul_pages_layout(const struct device *flash,
					 const struct flash_pages_layout **layout,
					 size_t *layout_size)
{
	const struct flash_mspi_emul_device_config *cfg = flash->config;

	*layout = &cfg->page_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_mspi_emul_device_api = {
	.erase = flash_mspi_emul_erase,
	.write = flash_mspi_emul_write,
	.read = flash_mspi_emul_read,
	.get_parameters = flash_mspi_emul_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_mspi_emul_pages_layout,
#endif
};

static const struct emul_mspi_device_api emul_mspi_dev_api = {
	.transceive = emul_mspi_device_transceive,
};

/**
 * Set up a new MSPI device emulator
 *
 * @param emul The MSPI device emulator instance itself
 * @param bus The MSPI bus emulator instance
 * @return 0 If successful
 */
static int emul_mspi_device_init(const struct emul *emul_flash, const struct device *bus)
{
	const struct flash_mspi_emul_device_config *cfg = emul_flash->dev->config;
	struct flash_mspi_emul_device_data *data = emul_flash->dev->data;

	data->bus = bus;

	if (mspi_dev_config(data->bus, &cfg->dev_id, MSPI_DEVICE_CONFIG_ALL,
			    &cfg->tar_dev_cfg)) {
		LOG_ERR("%u, Failed to config mspi controller", __LINE__);
		return -EIO;
	}
	data->dev_cfg = cfg->tar_dev_cfg;

#if CONFIG_MSPI_XIP
	if (cfg->tar_xip_cfg.enable) {
		if (mspi_xip_config(data->bus, &cfg->dev_id, &cfg->tar_xip_cfg)) {
			LOG_ERR("%u, Failed to enable XIP.", __LINE__);
			return -EIO;
		}
		data->xip_cfg = cfg->tar_xip_cfg;
	}
#endif

#if CONFIG_MSPI_SCRAMBLE
	if (cfg->tar_scramble_cfg.enable) {
		if (mspi_scramble_config(data->bus, &cfg->dev_id, &cfg->tar_scramble_cfg)) {
			LOG_ERR("%u, Failed to enable scrambling.", __LINE__);
			return -EIO;
		}
		data->scramble_cfg = cfg->tar_scramble_cfg;
	}
#endif

#if CONFIG_MSPI_TIMING
	if (mspi_timing_config(data->bus, &cfg->dev_id,
			       MSPI_TIMING_PARAM_DUMMY, &data->timing_cfg)) {
		LOG_ERR("%u, Failed to configure timing.", __LINE__);
		return -EIO;
	}
#endif

	release(emul_flash->dev);

	return 0;
}

static int flash_mspi_emul_device_init_stub(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

#define FLASH_MSPI_EMUL_DEVICE(n)                                                                 \
	static uint8_t flash_mspi_emul_device_mem##n[DT_INST_PROP(n, size) / 8];                  \
	static const struct flash_mspi_emul_device_config flash_mspi_emul_device_config_##n = {   \
		.size                     = DT_INST_PROP(n, size) / 8,                            \
		.flash_param =                                                                    \
			{                                                                         \
				.write_block_size = 1,                                            \
				.erase_value      = 0xff,                                         \
			},                                                                        \
		.page_layout =                                                                    \
			{                                                                         \
				.pages_count      = DT_INST_PROP(n, size) / 8 / SPI_NOR_PAGE_SIZE,\
				.pages_size       = SPI_NOR_PAGE_SIZE,                            \
			},                                                                        \
		.dev_id                   = MSPI_DEVICE_ID_DT_INST(n),                            \
		.tar_dev_cfg              = MSPI_DEVICE_CONFIG_DT_INST(n),                        \
		.tar_xip_cfg              = MSPI_XIP_CONFIG_DT_INST(n),                           \
		.tar_scramble_cfg         = MSPI_SCRAMBLE_CONFIG_DT_INST(n),                      \
		.sw_multi_periph          = DT_PROP(DT_INST_BUS(n), software_multiperipheral)     \
	};                                                                                        \
	static struct flash_mspi_emul_device_data flash_mspi_emul_device_data_##n = {             \
		.lock = Z_SEM_INITIALIZER(flash_mspi_emul_device_data_##n.lock, 0, 1),            \
		.mem  = (uint8_t *)flash_mspi_emul_device_mem##n,                                 \
	};                                                                                        \
	DEVICE_DT_INST_DEFINE(n,                                                                  \
			      flash_mspi_emul_device_init_stub,                                   \
			      NULL,                                                               \
			      &flash_mspi_emul_device_data_##n,                                   \
			      &flash_mspi_emul_device_config_##n,                                 \
			      POST_KERNEL,                                                        \
			      CONFIG_FLASH_INIT_PRIORITY,                                         \
			      &flash_mspi_emul_device_api);

#define EMUL_TEST(n)                                                                              \
	EMUL_DT_INST_DEFINE(n,                                                                    \
			    emul_mspi_device_init,                                                \
			    NULL,                                                                 \
			    NULL,                                                                 \
			    &emul_mspi_dev_api,                                                   \
			    NULL);

DT_INST_FOREACH_STATUS_OKAY(EMUL_TEST);

DT_INST_FOREACH_STATUS_OKAY(FLASH_MSPI_EMUL_DEVICE);
