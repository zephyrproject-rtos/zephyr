/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_semihost_flash

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/common/semihost.h>
#include <string.h>

#define SOC_NV_FLASH_COMPAT(node_id)                                                               \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, soc_nv_flash), (node_id), ())
#define SOC_NV_FLASH_NODE(inst) DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, SOC_NV_FLASH_COMPAT)

#define ERASE_FILL_BUFFER_SIZE 256

struct flash_semihost_config {
	const char *filename;
	const struct flash_parameters *flash_params;
	const struct flash_pages_layout *pages_layout;
	size_t flash_size;
	size_t erase_block_size;
};

struct flash_semihost_data {
	long fd;
	struct k_mutex lock;
};

static int flash_semihost_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	const struct flash_semihost_config *config = dev->config;
	struct flash_semihost_data *dev_data = dev->data;
	int ret;

	if (offset + len > config->flash_size) {
		return -EINVAL;
	}

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	ret = semihost_seek(dev_data->fd, (long)offset);
	if (ret != 0) {
		k_mutex_unlock(&dev_data->lock);
		return -EIO;
	}

	if (len == 0) {
		/* Semihost read will fail, but this is a valid operation */
		k_mutex_unlock(&dev_data->lock);
		return 0;
	}

	long bytes_read = semihost_read(dev_data->fd, data, (long)len);

	k_mutex_unlock(&dev_data->lock);

	if (bytes_read < 0) {
		return -EIO;
	}

	if (bytes_read < (long)len) {
		return -EIO;
	}

	return 0;
}

static int flash_semihost_write(const struct device *dev, off_t offset, const void *data,
				size_t len)
{
	const struct flash_semihost_config *config = dev->config;
	struct flash_semihost_data *dev_data = dev->data;
	int ret;

	if (offset + len > config->flash_size) {
		return -EINVAL;
	}

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	ret = semihost_seek(dev_data->fd, (long)offset);
	if (ret != 0) {
		k_mutex_unlock(&dev_data->lock);
		return -EIO;
	}

	long bytes_not_written = semihost_write(dev_data->fd, data, (long)len);

	k_mutex_unlock(&dev_data->lock);

	if (bytes_not_written != 0) {
		return -EIO;
	}

	return 0;
}

/*
 * Flash could be enabled with CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE, but
 * We emulate erase behavior for the best compatibility with zephyr's API, since
 * the primary purpose of this driver is for simulation
 */
static int flash_semihost_erase(const struct device *dev, off_t offset, size_t size)
{
	const struct flash_semihost_config *config = dev->config;
	struct flash_semihost_data *dev_data = dev->data;
	uint8_t erase_buffer[ERASE_FILL_BUFFER_SIZE];
	size_t remaining;
	int ret;

	if (offset + size > config->flash_size) {
		return -EINVAL;
	}

	if (size % config->erase_block_size != 0 || offset % config->erase_block_size != 0) {
		return -EINVAL;
	}

	memset(erase_buffer, 0xff, sizeof(erase_buffer));

	k_mutex_lock(&dev_data->lock, K_FOREVER);

	for (remaining = size; remaining > 0;) {
		size_t chunk =
			(remaining > sizeof(erase_buffer)) ? sizeof(erase_buffer) : remaining;

		ret = semihost_seek(dev_data->fd, (long)(offset + size - remaining));
		if (ret != 0) {
			k_mutex_unlock(&dev_data->lock);
			return -EIO;
		}

		long bytes_not_written = semihost_write(dev_data->fd, erase_buffer, (long)chunk);

		if (bytes_not_written != 0) {
			k_mutex_unlock(&dev_data->lock);
			return -EIO;
		}

		remaining -= chunk;
	}

	k_mutex_unlock(&dev_data->lock);

	return 0;
}

static const struct flash_parameters *flash_semihost_get_parameters(const struct device *dev)
{
	const struct flash_semihost_config *config = dev->config;

	return config->flash_params;
}

static int flash_semihost_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_semihost_config *config = dev->config;

	*size = config->flash_size;
	return 0;
}

static void flash_semihost_page_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	const struct flash_semihost_config *config = dev->config;

	*layout = config->pages_layout;
	*layout_size = 1;
}

static int flash_semihost_init(const struct device *dev)
{
	struct flash_semihost_data *data = dev->data;
	const struct flash_semihost_config *config = dev->config;
	long fd;
	int ret;
	uint8_t zero = 0;

	k_mutex_init(&data->lock);

	fd = semihost_open(config->filename, SEMIHOST_OPEN_RB_PLUS);
	if (fd < 0) {
		/* File does not exist, create it */
		fd = semihost_open(config->filename, SEMIHOST_OPEN_WB_PLUS);
		if (fd < 0) {
			return -ENODEV;
		}
	}

	ret = semihost_seek(fd, (long)(config->flash_size - 1));
	if (ret != 0) {
		semihost_close(fd);
		return -EIO;
	}

	/*
	 * Attempt a read from the end of flash. If this fails, we will write
	 * to the offset so that the flash binary file gets extended to
	 * the correct size
	 */
	long bytes_read = semihost_read(fd, &zero, 1);

	if (bytes_read < 0) {
		/* Write to the offset to extend flash */
		long bytes_not_written = semihost_write(fd, &zero, 1);

		if (bytes_not_written != 0) {
			semihost_close(fd);
			return -EIO;
		}
	}

	data->fd = fd;

	return 0;
}

static DEVICE_API(flash, flash_semihost_api) = {
	.read = flash_semihost_read,
	.write = flash_semihost_write,
	.erase = flash_semihost_erase,
	.get_parameters = flash_semihost_get_parameters,
	.get_size = flash_semihost_get_size,
	.page_layout = flash_semihost_page_layout,
};

#define FLASH_SEMIHOST_PAGES_LAYOUT_INIT(n)                                                        \
	static const struct flash_pages_layout flash_semihost_layout_##n[] = {                     \
		{                                                                                  \
			.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE(n)) /                         \
				       DT_PROP(SOC_NV_FLASH_NODE(n), erase_block_size),            \
			.pages_size = DT_PROP(SOC_NV_FLASH_NODE(n), erase_block_size),             \
		},                                                                                 \
	};

#define FLASH_SEMIHOST_PARAMS_INIT(n)                                                              \
	static const struct flash_parameters flash_semihost_params_##n = {                         \
		.write_block_size = 1,                                                             \
		.caps = {.no_explicit_erase = false},                                              \
		.erase_value = 0xff,                                                               \
	};

#define FLASH_SEMIHOST_DEVICE_INIT(n)                                                              \
	FLASH_SEMIHOST_PAGES_LAYOUT_INIT(n)                                                        \
	FLASH_SEMIHOST_PARAMS_INIT(n)                                                              \
	static struct flash_semihost_data flash_semihost_data_##n;                                 \
	static const struct flash_semihost_config flash_semihost_cfg_##n = {                       \
		.filename = DT_INST_PROP(n, filename),                                             \
		.flash_params = &flash_semihost_params_##n,                                        \
		.pages_layout = flash_semihost_layout_##n,                                         \
		.flash_size = DT_REG_SIZE(SOC_NV_FLASH_NODE(n)),                                   \
		.erase_block_size = DT_PROP(SOC_NV_FLASH_NODE(n), erase_block_size),               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, flash_semihost_init, NULL, &flash_semihost_data_##n,              \
			      &flash_semihost_cfg_##n, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,    \
			      &flash_semihost_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_SEMIHOST_DEVICE_INIT)
