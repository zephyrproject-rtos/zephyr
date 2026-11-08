/*
 * Copyright (c) 2026 FIRST RF
 * SPDX-License-Identifier: Apache-2.0
 *
 * Lattice MachXO2 user flash memory (UFM) driver.
 *
 * The UFM is accessed through the sysCONFIG transparent mode transport owned by
 * the parent Lattice MachXO2 FPGA driver. The whole UFM is a single erase
 * sector and programming happens in 16-byte pages. The hardware does not
 * support selective erase; erasing any part of the UFM erases all of it.
 */

#define DT_DRV_COMPAT lattice_machxo2_ufm

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "fpga_machxo2.h"

LOG_MODULE_REGISTER(flash_machxo2_ufm, CONFIG_FLASH_LOG_LEVEL);

#define MACHXO2_UFM_BUSY_TIMEOUT_MS  1000U
#define MACHXO2_UFM_ERASE_TIMEOUT_MS 2000U
#define MACHXO2_UFM_PROG_DELAY_US    200U

struct machxo2_ufm_config {
	const struct device *parent;
	uint32_t size;
	struct flash_parameters parameters;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	struct flash_pages_layout layout;
#endif
};

static int machxo2_ufm_enter(const struct device *parent, bool *entered)
{
	int ret;

	ret = fpga_machxo2_transceive(parent, FPGA_MACHXO2_CMD_ISC_ENABLE_X,
				      FPGA_MACHXO2_ENABLE_FLASH, NULL, 0U, NULL, 0U);
	if (ret != 0) {
		return ret;
	}

	*entered = true;

	return fpga_machxo2_wait_not_busy(parent, MACHXO2_UFM_BUSY_TIMEOUT_MS);
}

static int machxo2_ufm_exit(const struct device *parent)
{
	int ret;

	ret = fpga_machxo2_transceive(parent, FPGA_MACHXO2_CMD_ISC_DISABLE, 0U, NULL, 0U, NULL, 0U);
	if (ret != 0) {
		return ret;
	}

	ret = fpga_machxo2_transceive(parent, FPGA_MACHXO2_CMD_ISC_NOOP, 0U, NULL, 0U, NULL, 0U);
	if (ret != 0) {
		return ret;
	}

	return fpga_machxo2_wait_not_busy(parent, MACHXO2_UFM_BUSY_TIMEOUT_MS);
}

static int machxo2_ufm_check_status(const struct device *parent)
{
	uint8_t buf[4];
	uint32_t status;
	int ret;

	ret = fpga_machxo2_transceive(parent, FPGA_MACHXO2_CMD_LSC_READ_STATUS, 0U, NULL, 0U, buf,
				      sizeof(buf));
	if (ret != 0) {
		return ret;
	}

	status = sys_get_be32(buf);
	if (((status & FPGA_MACHXO2_STATUS_FAIL) != 0U) ||
	    (((status >> FPGA_MACHXO2_STATUS_ERR_POS) & FPGA_MACHXO2_STATUS_ERR_MASK) != 0U)) {
		LOG_ERR("UFM operation failed, status 0x%08x", status);
		return -EIO;
	}

	return 0;
}

static int machxo2_ufm_set_page(const struct device *parent, size_t page)
{
	uint8_t address[4];
	uint32_t value = FPGA_MACHXO2_UFM_ADDRESS_MARKER | (page & 0x3FFFU);

	if (page == 0U) {
		return fpga_machxo2_transceive(parent, FPGA_MACHXO2_CMD_LSC_INIT_ADDR_UFM, 0U, NULL, 0U, NULL, 0U);
	}

	sys_put_be32(value, address);

	return fpga_machxo2_transceive(parent, FPGA_MACHXO2_CMD_LSC_WRITE_ADDRESS, 0U, address,
				       sizeof(address), NULL, 0U);
}

static int machxo2_ufm_read_page(const struct device *parent, uint8_t *page)
{
	return fpga_machxo2_transceive(parent, FPGA_MACHXO2_CMD_LSC_READ_UFM,
				       FPGA_MACHXO2_UFM_READ_OPERAND, NULL, 0U, page,
				       FPGA_MACHXO2_PAGE_SIZE);
}

static int machxo2_ufm_write_page(const struct device *parent, const uint8_t *page)
{
	int ret;

	ret = fpga_machxo2_transceive(parent, FPGA_MACHXO2_CMD_LSC_PROG_TAG,
				      FPGA_MACHXO2_PROG_INCR, page, FPGA_MACHXO2_PAGE_SIZE,
				      NULL, 0U);
	if (ret != 0) {
		return ret;
	}

	k_busy_wait(MACHXO2_UFM_PROG_DELAY_US);

	ret = fpga_machxo2_wait_not_busy(parent, MACHXO2_UFM_BUSY_TIMEOUT_MS);
	if (ret != 0) {
		return ret;
	}

	return machxo2_ufm_check_status(parent);
}

static int machxo2_ufm_erase_sector(const struct device *parent)
{
	int ret;

	ret = fpga_machxo2_transceive(parent, FPGA_MACHXO2_CMD_LSC_ERASE_TAG, 0U, NULL, 0U, NULL, 0U);
	if (ret != 0) {
		return ret;
	}

	ret = fpga_machxo2_wait_not_busy(parent, MACHXO2_UFM_ERASE_TIMEOUT_MS);
	if (ret != 0) {
		return ret;
	}

	return machxo2_ufm_check_status(parent);
}

static bool machxo2_ufm_range_valid(const struct machxo2_ufm_config *config, off_t offset,
				    size_t len)
{
	if (offset < 0) {
		return false;
	}

	return ((uint64_t)offset + len) <= config->size;
}

static int machxo2_ufm_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct machxo2_ufm_config *config = dev->config;
	uint8_t page[FPGA_MACHXO2_PAGE_SIZE];
	uint8_t *buffer = data;
	bool entered = false;
	bool first = true;
	int ret;

	if (len == 0U) {
		return 0;
	}

	if (data == NULL) {
		return -EINVAL;
	}

	if (!machxo2_ufm_range_valid(config, offset, len)) {
		return -EINVAL;
	}

	ret = fpga_machxo2_lock(config->parent, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	ret = machxo2_ufm_enter(config->parent, &entered);

	while (ret == 0 && len > 0U) {
		size_t page_offset = (size_t)offset % FPGA_MACHXO2_PAGE_SIZE;
		size_t chunk = MIN(len, FPGA_MACHXO2_PAGE_SIZE - page_offset);

		if (first) {
			size_t page_index = (size_t)offset / FPGA_MACHXO2_PAGE_SIZE;

			ret = machxo2_ufm_set_page(config->parent, page_index);
			if (ret != 0) {
				break;
			}

			first = false;
		}

		ret = machxo2_ufm_read_page(config->parent, page);
		if (ret != 0) {
			break;
		}

		memcpy(buffer, &page[page_offset], chunk);
		buffer += chunk;
		offset += chunk;
		len -= chunk;
	}

	if (entered) {
		int exit_ret = machxo2_ufm_exit(config->parent);

		if (ret == 0) {
			ret = exit_ret;
		} else if (exit_ret != 0) {
			LOG_ERR("Failed to exit UFM mode: %d", exit_ret);
		}
	}

	fpga_machxo2_unlock(config->parent);

	return ret;
}

static int machxo2_ufm_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	const struct machxo2_ufm_config *config = dev->config;
	const uint8_t *buffer = data;
	bool entered = false;
	bool first = true;
	int ret;

	if (len == 0U) {
		return 0;
	}

	if (data == NULL) {
		return -EINVAL;
	}

	if (!machxo2_ufm_range_valid(config, offset, len)) {
		return -EINVAL;
	}

	if ((((size_t)offset % config->parameters.write_block_size) != 0) ||
	    ((len % config->parameters.write_block_size) != 0)) {
		return -EINVAL;
	}

	ret = fpga_machxo2_lock(config->parent, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	ret = machxo2_ufm_enter(config->parent, &entered);

	while (ret == 0 && len > 0U) {
		if (first) {
			size_t page_index = (size_t)offset / FPGA_MACHXO2_PAGE_SIZE;

			ret = machxo2_ufm_set_page(config->parent, page_index);
			if (ret != 0) {
				break;
			}

			first = false;
		}

		ret = machxo2_ufm_write_page(config->parent, buffer);
		if (ret != 0) {
			break;
		}

		buffer += FPGA_MACHXO2_PAGE_SIZE;
		offset += FPGA_MACHXO2_PAGE_SIZE;
		len -= FPGA_MACHXO2_PAGE_SIZE;
	}

	if (entered) {
		int exit_ret = machxo2_ufm_exit(config->parent);

		if (ret == 0) {
			ret = exit_ret;
		} else if (exit_ret != 0) {
			LOG_ERR("Failed to exit UFM mode: %d", exit_ret);
		}
	}

	fpga_machxo2_unlock(config->parent);

	return ret;
}

static int machxo2_ufm_erase(const struct device *dev, off_t offset, size_t len)
{
	const struct machxo2_ufm_config *config = dev->config;
	bool entered = false;
	int ret;

	if (len == 0U) {
		return 0;
	}

	/* The whole UFM is one erase sector, so only a full erase is supported. */
	if ((offset != 0) || (len != config->size)) {
		return -EINVAL;
	}

	ret = fpga_machxo2_lock(config->parent, K_FOREVER);
	if (ret != 0) {
		return ret;
	}

	ret = machxo2_ufm_enter(config->parent, &entered);
	if (ret == 0) {
		ret = machxo2_ufm_erase_sector(config->parent);
	}

	if (entered) {
		int exit_ret = machxo2_ufm_exit(config->parent);

		if (ret == 0) {
			ret = exit_ret;
		} else if (exit_ret != 0) {
			LOG_ERR("Failed to exit UFM mode: %d", exit_ret);
		}
	}

	fpga_machxo2_unlock(config->parent);

	return ret;
}

static const struct flash_parameters *machxo2_ufm_get_parameters(const struct device *dev)
{
	const struct machxo2_ufm_config *config = dev->config;

	return &config->parameters;
}

static int machxo2_ufm_get_size(const struct device *dev, uint64_t *size)
{
	const struct machxo2_ufm_config *config = dev->config;

	*size = config->size;

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static void machxo2_ufm_page_layout(const struct device *dev,
				    const struct flash_pages_layout **layout, size_t *layout_size)
{
	const struct machxo2_ufm_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif

static int machxo2_ufm_init(const struct device *dev)
{
	const struct machxo2_ufm_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("MachXO2 FPGA parent is not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(flash, machxo2_ufm_api) = {
	.read = machxo2_ufm_read,
	.write = machxo2_ufm_write,
	.erase = machxo2_ufm_erase,
	.get_parameters = machxo2_ufm_get_parameters,
	.get_size = machxo2_ufm_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = machxo2_ufm_page_layout,
#endif
};

BUILD_ASSERT(CONFIG_MACHXO2_UFM_INIT_PRIORITY > CONFIG_FPGA_INIT_PRIORITY,
	     "MachXO2 UFM init priority must be greater than the FPGA init priority");

#define MACHXO2_UFM_INIT(inst)                                                                     \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_PARENT(DT_DRV_INST(inst)), lattice_machxo2_fpga),       \
		     "MachXO2 UFM must be a child of a lattice,machxo2-fpga node");                \
	BUILD_ASSERT(DT_INST_PROP(inst, size) >= FPGA_MACHXO2_PAGE_SIZE,                           \
		     "MachXO2 UFM size must be at least one page (16 bytes)");                     \
	BUILD_ASSERT((DT_INST_PROP(inst, size) % FPGA_MACHXO2_PAGE_SIZE) == 0,                     \
		     "MachXO2 UFM size must be a multiple of the page size");                      \
	BUILD_ASSERT(DT_INST_PROP(inst, write_block_size) == FPGA_MACHXO2_PAGE_SIZE,               \
		     "MachXO2 UFM write block size must be the page size");                        \
                                                                                                   \
	static const struct machxo2_ufm_config machxo2_ufm_config_##inst = {                       \
		.parent = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                             \
		.size = DT_INST_PROP(inst, size),                                                  \
		.parameters =                                                                      \
			{                                                                          \
				.write_block_size = DT_INST_PROP(inst, write_block_size),          \
				.erase_value = 0xff,                                               \
			},                                                                         \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,                                               \
			(.layout = {                                                               \
				.pages_count = 1,                                                  \
				.pages_size = DT_INST_PROP(inst, size),                            \
			},))                                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, machxo2_ufm_init, NULL, NULL, &machxo2_ufm_config_##inst,      \
			      POST_KERNEL, CONFIG_MACHXO2_UFM_INIT_PRIORITY, &machxo2_ufm_api)

DT_INST_FOREACH_STATUS_OKAY(MACHXO2_UFM_INIT)
