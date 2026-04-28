/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/cache.h>
#include "spi_nor.h"
#include "r_spi_flash_api.h"

#if defined(CONFIG_FLASH_RENESAS_RZ_QSPI_SPIBSC)
#include "r_spibsc.h"
#else
#include "r_xspi_qspi.h"
#endif

LOG_MODULE_REGISTER(renesas_rz_qspi, CONFIG_FLASH_LOG_LEVEL);

#if defined(CONFIG_FLASH_RENESAS_RZ_QSPI_XSPI)
#define QSPI_DEFAULT_SR     (0x40)
#define QSPI_UPDATE_CR      (0xC0) /* Configuration register (DC0=1, DC1=1 (Dummy cycle = 10)) */
#define QSPI_DATA_CR_UPDATE (QSPI_UPDATE_CR << 8 | QSPI_DEFAULT_SR)
#endif /* CONFIG_FLASH_RENESAS_RZ_QSPI_XSPI */

#define FLASH_RZ_BASE_ADDRESS (CONFIG_FLASH_BASE_ADDRESS - CONFIG_FLASH_RENESAS_RZ_MIRROR_OFFSET)

/* QSPI COMMANDS */
#define QSPI_CMD_RDSFDP (0x5A) /* Read SFDP */

/* XIP (Execute In Place) mode */
#define QSPI_CMD_XIP_ENTER (0xA5) /* XIP Enter command */
#define QSPI_CMD_XIP_EXIT  (0xFF) /* XIP Exit command */

#define QSPI_CMD_QUAD_PAGE_PROGRAM (0x33)

/* One byte data transfer */
#define DATA_LENGTH_DEFAULT_BYTE (0U)
#define ONE_BYTE                 (1U)
#define TWO_BYTE                 (2U)
#define THREE_BYTE               (3U)
#define FOUR_BYTE                (4U)

/* Default erase value */
#define QSPI_ERASE_VALUE (0xFF)

/* Maximum memory buffer size of write operation in memory-map mode */
#if defined(CONFIG_FLASH_RENESAS_RZ_QSPI_SPIBSC)
#define QSPI_MAX_BUFFER_SIZE 256U
#else
#define QSPI_MAX_BUFFER_SIZE 64U
#endif

struct flash_renesas_rz_data {
	spi_flash_ctrl_t *fsp_ctrl;
	spi_flash_cfg_t *fsp_cfg;

	struct k_sem sem;
};

struct flash_renesas_rz_config {
	const struct pinctrl_dev_config *pin_cfg;
	const spi_flash_api_t *fsp_api;

	uint32_t erase_block_size;
	uint32_t flash_size;
	struct flash_parameters flash_param;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

static const spi_flash_erase_command_t g_erase_command_list[4] = {
	{.command = SPI_NOR_CMD_SE, .size = SPI_NOR_SECTOR_SIZE},
	{.command = SPI_NOR_CMD_BE_32K, .size = SPI_NOR_BLOCK_32K_SIZE},
	{.command = SPI_NOR_CMD_BE, .size = SPI_NOR_BLOCK_SIZE},
	{.command = SPI_NOR_CMD_CE, .size = SPI_FLASH_ERASE_SIZE_CHIP_ERASE},
};

static void acquire_device(const struct device *dev)
{
	struct flash_renesas_rz_data *dev_data = dev->data;

	k_sem_take(&dev_data->sem, K_FOREVER);
}

static void release_device(const struct device *dev)
{
	struct flash_renesas_rz_data *dev_data = dev->data;

	k_sem_give(&dev_data->sem);
}

static int qspi_wait_until_ready(const struct device *dev)
{
	const struct flash_renesas_rz_config *config = dev->config;
	struct flash_renesas_rz_data *data = dev->data;
	spi_flash_status_t status = {.write_in_progress = true};
	uint32_t timeout = 0xFFFFFF;
	fsp_err_t err;

	while ((status.write_in_progress) && (timeout > 0)) {
		err = config->fsp_api->statusGet(data->fsp_ctrl, &status);
		if (err != FSP_SUCCESS) {
			LOG_ERR("Status get failed");
			return -EIO;
		}
		timeout--;
	}

	return 0;
}

#if CONFIG_FLASH_PAGE_LAYOUT
void flash_renesas_rz_page_layout(const struct device *dev,
				  const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct flash_renesas_rz_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if defined(CONFIG_FLASH_JESD216_API)
static int qspi_flash_rz_read_jedec_id(const struct device *dev, uint8_t *id)
{
	const struct flash_renesas_rz_config *config = dev->config;
	struct flash_renesas_rz_data *data = dev->data;
	int ret = 0;

	if (id == NULL) {
		return -EINVAL;
	}

	spi_flash_direct_transfer_t trans = {
		.command = SPI_NOR_CMD_RDID,
		.address = 0,
		.data = 0,
		.command_length = 1U,
		.address_length = 0U,
		.data_length = THREE_BYTE,
		.dummy_cycles = 0U,
	};

	acquire_device(dev);
	ret = config->fsp_api->directTransfer(data->fsp_ctrl, &trans,
					      SPI_FLASH_DIRECT_TRANSFER_DIR_READ);
	if (FSP_SUCCESS != (fsp_err_t)ret) {
		LOG_ERR("Failed to read device id");
		release_device(dev);
		return -EIO;
	}

	/* Get flash device ID */
	memcpy(id, &trans.data, sizeof(trans.data));
	release_device(dev);

	return ret;
}

static int qspi_flash_renesas_rz_sfdp_read(const struct device *dev, off_t addr, void *data,
					   size_t len)
{
	const struct flash_renesas_rz_config *config = dev->config;
	struct flash_renesas_rz_data *dev_data = dev->data;
	int ret = 0;
	size_t size;

	spi_flash_direct_transfer_t trans = {
		.command = QSPI_CMD_RDSFDP,
		.address = addr,
		.data = 0,
		.command_length = 1U,
		.address_length = 3U,
		.data_length = FOUR_BYTE,
		.dummy_cycles = SPI_NOR_DUMMY_RD,
	};

	acquire_device(dev);
	while (len > 0) {
		size = MIN(len, trans.data_length);
		trans.address = addr;
		trans.data_length = size;

		ret = config->fsp_api->directTransfer(dev_data->fsp_ctrl, &trans,
						      SPI_FLASH_DIRECT_TRANSFER_DIR_READ);

		if (FSP_SUCCESS != (fsp_err_t)ret) {
			LOG_ERR("Failed to read SFDP id");
			release_device(dev);
			return -EIO;
		}

		memcpy(data, &trans.data, size);

		len -= size;
		addr += size;
		data = (uint8_t *)data + size;
	}

	release_device(dev);
	return ret;
}
#endif

static bool qspi_flash_rz_valid(uint32_t area_size, off_t offset, size_t len)
{
	if ((offset < 0) || (offset >= area_size) || ((area_size - offset) < len)) {
		return false;
	}

	return true;
}

static int qspi_flash_rz_erase(const struct device *dev, off_t offset, size_t len)
{
	const struct flash_renesas_rz_config *config = dev->config;
	struct flash_renesas_rz_data *data = dev->data;
	int err = 0;
	struct flash_pages_info page_info_start, page_info_end;
	uint32_t erase_size;
	int rc;

	if (!len) {
		return 0;
	}

	if (!qspi_flash_rz_valid(config->flash_size, offset, len)) {
		LOG_ERR("The offset 0x%lx is invalid", (long)offset);
		return -EINVAL;
	}

	if (0 != (len % config->erase_block_size)) {
		LOG_ERR("The size %zu is not align with block size (%u)", len,
			config->erase_block_size);
		return -EINVAL;
	}

	rc = flash_get_page_info_by_offs(dev, offset, &page_info_start);
	if ((rc != 0) || (offset != page_info_start.start_offset)) {
		LOG_ERR("The offset 0x%lx is not aligned with the starting sector", (long)offset);
		return -EINVAL;
	}

	rc = flash_get_page_info_by_offs(dev, (offset + len), &page_info_end);
	if ((rc != 0) || ((offset + len) != page_info_end.start_offset)) {
		LOG_ERR("The size %zu is not aligned with the ending sector", len);
		return -EINVAL;
	}

	acquire_device(dev);
	while (len > 0) {
		if (len < SPI_NOR_BLOCK_32K_SIZE) {
			erase_size = SPI_NOR_SECTOR_SIZE;
		} else if (len < SPI_NOR_BLOCK_SIZE) {
			erase_size = SPI_NOR_BLOCK_32K_SIZE;
		} else {
			erase_size = SPI_NOR_BLOCK_SIZE;
		}

		uint8_t *dest = (uint8_t *)FLASH_RZ_BASE_ADDRESS;

		dest += (size_t)offset;
		err = config->fsp_api->erase(data->fsp_ctrl, dest, erase_size);
		if (FSP_SUCCESS != (fsp_err_t)err) {
			LOG_ERR("Erase failed");
			err = -EIO;
			break;
		}

		err = qspi_wait_until_ready(dev);
		if (err) {
			LOG_ERR("Failed to get status for QSPI operation");
			err = -EIO;
			break;
		}

		offset += erase_size;
		len -= erase_size;

#if defined(CONFIG_FLASH_RENESAS_RZ_QSPI_SPIBSC)
		spibsc_instance_ctrl_t *p_ctrl = (spibsc_instance_ctrl_t *)data->fsp_ctrl;

		/* Invalidating SPIBSC cache */
		p_ctrl->p_reg->DRCR_b.RCF = 1;
		sys_cache_data_invd_range((void *)dest, erase_size);
#endif
	}
	release_device(dev);

	return err;
}

static int qspi_flash_rz_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct flash_renesas_rz_config *config = dev->config;

	if (!len) {
		return 0;
	}

	if (!data) {
		return -EINVAL;
	}

	if (!qspi_flash_rz_valid(config->flash_size, offset, len)) {
		return -EINVAL;
	}

	acquire_device(dev);

	uint8_t *dest = (uint8_t *)FLASH_RZ_BASE_ADDRESS;

	dest += (size_t)offset;
	memcpy(data, dest, len);
	release_device(dev);

	return 0;
}

static int qspi_flash_rz_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	const struct flash_renesas_rz_config *config = dev->config;
	struct flash_renesas_rz_data *dev_data = dev->data;
	int err = 0;
	uint32_t remaining_bytes = len;
	uint32_t size = (uint32_t)len;

	if (!len) {
		return 0;
	}

	if (!data) {
		return -EINVAL;
	}

	if (!qspi_flash_rz_valid(config->flash_size, offset, len)) {
		return -EINVAL;
	}

	acquire_device(dev);
	while (remaining_bytes > 0) {
		size = MIN(remaining_bytes, QSPI_MAX_BUFFER_SIZE);
		uint8_t *dest = (uint8_t *)FLASH_RZ_BASE_ADDRESS;

		dest += (size_t)offset;
		err = config->fsp_api->write(dev_data->fsp_ctrl, (const uint8_t *)data, dest, size);
		if (FSP_SUCCESS != (fsp_err_t)err) {
			LOG_ERR("Flash write failed");
			err = -EIO;
			break;
		}

		err = qspi_wait_until_ready(dev);
		if (err) {
			LOG_ERR("Failed to get status for QSPI operation");
			err = -EIO;
			break;
		}

		remaining_bytes -= size;
		offset += size;
		data = (const uint8_t *)data + size;

#if defined(CONFIG_FLASH_RENESAS_RZ_QSPI_SPIBSC)
		spibsc_instance_ctrl_t *p_ctrl = (spibsc_instance_ctrl_t *)dev_data->fsp_ctrl;

		/* Invalidating SPIBSC cache */
		p_ctrl->p_reg->DRCR_b.RCF = 1;
		sys_cache_data_invd_range((void *)dest, size);
#endif
	}
	release_device(dev);
	return err;
}

static int qspi_flash_rz_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_renesas_rz_config *config = dev->config;

	*size = (uint64_t)config->flash_size;

	return 0;
}

static const struct flash_parameters *qspi_flash_rz_get_parameters(const struct device *dev)
{
	const struct flash_renesas_rz_config *config = dev->config;

	return &config->flash_param;
}

#if CONFIG_FLASH_RENESAS_RZ_QSPI_XSPI
static int spi_flash_direct_write(const struct device *dev, uint8_t command, uint32_t tx_data,
				  uint8_t data_length)
{
	const struct flash_renesas_rz_config *config = dev->config;
	struct flash_renesas_rz_data *data = dev->data;
	int ret;

	spi_flash_direct_transfer_t trans = {
		.command = command,
		.address = 0U,
		.data = tx_data,
		.command_length = 1U,
		.address_length = 0U,
		.data_length = data_length,
		.dummy_cycles = 0U,
	};

	ret = config->fsp_api->directTransfer(data->fsp_ctrl, &trans,
					      SPI_FLASH_DIRECT_TRANSFER_DIR_WRITE);
	if (FSP_SUCCESS != (fsp_err_t)ret) {
		LOG_ERR("Failed to write command");
		return -EIO;
	};

	return ret;
}
#endif

static int flash_renesas_rz_init(const struct device *dev)
{
	const struct flash_renesas_rz_config *config = dev->config;
	struct flash_renesas_rz_data *data = dev->data;
	int ret = 0;

#if CONFIG_FLASH_RENESAS_RZ_QSPI_XSPI
	ret = pinctrl_apply_state(config->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to configure pins for QSPI with code: %d", ret);
		return -EIO;
	}
#endif
	k_sem_init(&data->sem, 1, 1);

	ret = config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (FSP_SUCCESS != (fsp_err_t)ret) {
		LOG_ERR("Open failed");
		return -EIO;
	}

#if CONFIG_FLASH_RENESAS_RZ_QSPI_XSPI
	/* Write Enable Command */
	ret = spi_flash_direct_write(dev, data->fsp_cfg->write_enable_command, 0U,
				     DATA_LENGTH_DEFAULT_BYTE);
	if (ret) {
		return ret;
	}

	/* Write Status Command */
	ret = spi_flash_direct_write(dev, SPI_NOR_CMD_WRSR, QSPI_DATA_CR_UPDATE, TWO_BYTE);
	if (ret) {
		return ret;
	}
#endif
	return ret;
}

static DEVICE_API(flash, flash_renesas_rz_qspi_driver_api) = {
	.erase = qspi_flash_rz_erase,
	.write = qspi_flash_rz_write,
	.read = qspi_flash_rz_read,
	.get_parameters = qspi_flash_rz_get_parameters,
	.get_size = qspi_flash_rz_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_renesas_rz_page_layout,
#endif
#if defined(CONFIG_FLASH_JESD216_API)
	.sfdp_read = qspi_flash_renesas_rz_sfdp_read,
	.read_jedec_id = qspi_flash_rz_read_jedec_id,
#endif
};

#define DT_DRV_COMPAT renesas_rz_qspi_xspi

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define FLASH_RENESAS_RZ_QSPI_XSPI_DEFINE(n)                                                       \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(n));                                                      \
	static xspi_qspi_timing_setting_t g_qspi##n##_timing_settings = {                          \
		.command_to_command_interval = XSPI_QSPI_COMMAND_INTERVAL_CLOCKS_2,                \
		.cs_pullup_lag = XSPI_QSPI_CS_PULLUP_CLOCKS_1,                                     \
		.cs_pulldown_lead = XSPI_QSPI_CS_PULLDOWN_CLOCKS_1};                               \
	static xspi_qspi_address_space_t g_qspi##n##_address_space_settings = {                    \
		.unit0_cs0_end_address = XSPI_QSPI_CFG_UNIT_0_CS_0_END_ADDRESS,                    \
		.unit0_cs1_start_address = XSPI_QSPI_CFG_UNIT_0_CS_1_START_ADDRESS,                \
		.unit0_cs1_end_address = XSPI_QSPI_CFG_UNIT_0_CS_1_END_ADDRESS,                    \
		.unit1_cs0_end_address = XSPI_QSPI_CFG_UNIT_1_CS_0_END_ADDRESS,                    \
		.unit1_cs1_start_address = XSPI_QSPI_CFG_UNIT_1_CS_1_START_ADDRESS,                \
		.unit1_cs1_end_address = XSPI_QSPI_CFG_UNIT_1_CS_1_END_ADDRESS,                    \
	};                                                                                         \
	static const xspi_qspi_extended_cfg_t g_qspi##n##_extended_cfg = {                         \
		.unit = n,                                                                         \
		.chip_select = XSPI_QSPI_CHIP_SELECT_##n,                                          \
		.memory_size = XSPI_QSPI_MEMORY_SIZE_64MB,                                         \
		.p_timing_settings = &g_qspi##n##_timing_settings,                                 \
		.prefetch_en =                                                                     \
			(xspi_qspi_prefetch_function_t)XSPI_QSPI_CFG_UNIT_##n##_PREFETCH_FUNCTION, \
		.p_address_space = &g_qspi##n##_address_space_settings,                            \
	};                                                                                         \
	static spi_flash_cfg_t g_qspi##n##_cfg = {                                                 \
		.spi_protocol = SPI_FLASH_PROTOCOL_1S_1S_1S,                                       \
		.read_mode = SPI_FLASH_READ_MODE_FAST_READ,                                        \
		.address_bytes = SPI_FLASH_ADDRESS_BYTES_3,                                        \
		.dummy_clocks = SPI_FLASH_DUMMY_CLOCKS_10,                                         \
		.read_command = SPI_NOR_CMD_READ_FAST,                                             \
		.page_program_command = SPI_NOR_CMD_PP,                                            \
		.page_program_address_lines = SPI_FLASH_DATA_LINES_4,                              \
		.page_size_bytes = SPI_NOR_PAGE_SIZE,                                              \
		.write_enable_command = SPI_NOR_CMD_WREN,                                          \
		.status_command = SPI_NOR_CMD_RDSR,                                                \
		.write_status_bit = 0,                                                             \
		.xip_enter_command = QSPI_CMD_XIP_ENTER,                                           \
		.xip_exit_command = QSPI_CMD_XIP_EXIT,                                             \
		.p_erase_command_list = &g_erase_command_list[0],                                  \
		.erase_command_list_length = ARRAY_SIZE(g_erase_command_list),                     \
		.p_extend = &g_qspi##n##_extended_cfg,                                             \
	};                                                                                         \
	static xspi_qspi_instance_ctrl_t g_qspi##n##_ctrl;                                         \
	static struct flash_renesas_rz_data flash_renesas_rz_data_##n = {                          \
		.fsp_ctrl = &g_qspi##n##_ctrl,                                                     \
		.fsp_cfg = &g_qspi##n##_cfg,                                                       \
	};                                                                                         \
	static const struct flash_renesas_rz_config flash_renesas_rz_config_##n = {                \
		.pin_cfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(n)),                           \
		.fsp_api = &g_spi_flash_on_xspi_qspi,                                              \
		.flash_size = DT_INST_REG_SIZE(n),                                                 \
		.erase_block_size = DT_INST_PROP_OR(n, erase_block_size, 4096),                    \
		.flash_param =                                                                     \
			{                                                                          \
				.write_block_size = DT_INST_PROP(n, write_block_size),             \
				.erase_value = QSPI_ERASE_VALUE,                                   \
			},                                                                         \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,	\
		(.layout = {                                                                       \
			.pages_count =                                                             \
				DT_INST_REG_SIZE(n) / DT_INST_PROP_OR(n, erase_block_size, 4096),  \
			.pages_size = DT_INST_PROP_OR(n, erase_block_size, 4096),                  \
		},))};               \
	DEVICE_DT_INST_DEFINE(n, flash_renesas_rz_init, NULL, &flash_renesas_rz_data_##n,          \
			      &flash_renesas_rz_config_##n, POST_KERNEL,                           \
			      CONFIG_FLASH_INIT_PRIORITY, &flash_renesas_rz_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_RENESAS_RZ_QSPI_XSPI_DEFINE)
#endif

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_rz_qspi_spibsc

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define FLASH_RENESAS_RZ_QSPI_SPIBSC_DEFINE(n)                                                     \
	static const spibsc_extended_cfg_t g_qspi##n##_extended_cfg = {                            \
		.delay =                                                                           \
			{                                                                          \
				.slch = 0,                                                         \
				.clsh = 0,                                                         \
				.shsl = 6,                                                         \
			},                                                                         \
		.io_fix_mask = (0u << 2) | (1u << 3),                                              \
		.io_fix_value = (1u << 2) | (1u << 3),                                             \
	};                                                                                         \
	static spi_flash_cfg_t g_qspi##n##_cfg = {                                                 \
		.spi_protocol = SPI_FLASH_PROTOCOL_EXTENDED_SPI,                                   \
		.read_mode = SPI_FLASH_READ_MODE_FAST_READ_QUAD_IO,                                \
		.address_bytes = SPI_FLASH_ADDRESS_BYTES_3,                                        \
		.dummy_clocks = SPI_FLASH_DUMMY_CLOCKS_DEFAULT,                                    \
		.read_command = SPI_NOR_CMD_4READ,                                                 \
		.page_program_command = QSPI_CMD_QUAD_PAGE_PROGRAM,                                \
		.page_program_address_lines = SPI_FLASH_DATA_LINES_4,                              \
		.page_size_bytes = SPI_NOR_PAGE_SIZE,                                              \
		.write_enable_command = SPI_NOR_CMD_WREN,                                          \
		.status_command = SPI_NOR_CMD_RDSR,                                                \
		.write_status_bit = 0,                                                             \
		.xip_enter_command = QSPI_CMD_XIP_ENTER,                                           \
		.xip_exit_command = QSPI_CMD_XIP_EXIT,                                             \
		.p_erase_command_list = &g_erase_command_list[0],                                  \
		.erase_command_list_length = ARRAY_SIZE(g_erase_command_list),                     \
		.p_extend = &g_qspi##n##_extended_cfg,                                             \
	};                                                                                         \
	static spibsc_instance_ctrl_t g_qspi##n##_ctrl;                                            \
	static struct flash_renesas_rz_data flash_renesas_rz_data_##n = {                          \
		.fsp_ctrl = &g_qspi##n##_ctrl,                                                     \
		.fsp_cfg = &g_qspi##n##_cfg,                                                       \
	};                                                                                         \
	static const struct flash_renesas_rz_config flash_renesas_rz_config_##n = {                \
		.pin_cfg = NULL,                                                                   \
		.fsp_api = &g_spi_flash_on_spibsc,                                                 \
		.flash_size = DT_INST_REG_SIZE(n),                                                 \
		.erase_block_size = DT_INST_PROP_OR(n, erase_block_size, 4096),                    \
		.flash_param =                                                                     \
			{                                                                          \
				.write_block_size = DT_INST_PROP(n, write_block_size),             \
				.erase_value = QSPI_ERASE_VALUE,                                   \
			},                                                                         \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,	\
		(.layout = {                                                                       \
			.pages_count =                                                             \
				DT_INST_REG_SIZE(n) / DT_INST_PROP_OR(n, erase_block_size, 4096),  \
			.pages_size = DT_INST_PROP_OR(n, erase_block_size, 4096),                  \
		},))};               \
	DEVICE_DT_INST_DEFINE(n, flash_renesas_rz_init, NULL, &flash_renesas_rz_data_##n,          \
			      &flash_renesas_rz_config_##n, POST_KERNEL,                           \
			      CONFIG_FLASH_INIT_PRIORITY, &flash_renesas_rz_qspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_RENESAS_RZ_QSPI_SPIBSC_DEFINE)
#endif
