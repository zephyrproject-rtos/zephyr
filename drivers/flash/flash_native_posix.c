/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/flash.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "cmdline.h"
#include "soc.h"

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(flash_native_posix);

static const char default_flash_path[] = "flash.bin";

struct flash_native_posix_data {
	struct k_sem mutex;
	const char *flash_path;
	int fd;
	u8_t *flash;
	bool init_called;
};

struct flash_native_posix_config {
	size_t flash_size;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CONFIG(dev) ((dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct flash_native_posix_data *const)(dev)->driver_data)

static int flash_native_posix_read(struct device *dev, off_t offset, void *data,
				   size_t size)
{
	struct flash_native_posix_data *const dev_data = DEV_DATA(dev);
	const struct flash_native_posix_config *config = DEV_CONFIG(dev);

	if (dev_data->flash == MAP_FAILED) {
		LOG_ERR("No flash device mapped");
		return -EIO;
	}

	if ((offset + size) > config->flash_size) {
		LOG_WRN("Reading outside of flash boundaries");
		return -EINVAL;
	}

	memcpy(data, dev_data->flash + offset, size);

	return 0;
}

static int flash_native_posix_write(struct device *dev, off_t offset,
				    const void *data, size_t size)
{
	struct flash_native_posix_data *const dev_data = DEV_DATA(dev);
	const struct flash_native_posix_config *config = DEV_CONFIG(dev);

	if (dev_data->flash == MAP_FAILED) {
		LOG_ERR("No flash device mapped");
		return -EIO;
	}

	if ((offset + size) > config->flash_size) {
		LOG_WRN("Writing outside of flash boundaries");
		return -EINVAL;
	}

	memcpy(dev_data->flash + offset, data, size);

	return 0;
}

static int flash_native_posix_erase(struct device *dev, off_t offset,
				    size_t size)
{
	struct flash_native_posix_data *const dev_data = DEV_DATA(dev);
	const struct flash_native_posix_config *config = DEV_CONFIG(dev);

	if (dev_data->flash == MAP_FAILED) {
		LOG_ERR("No flash device mapped");
		return -EIO;
	}

	if ((offset + size) > config->flash_size) {
		LOG_WRN("Erasing outside of flash boundaries");
		return -EINVAL;
	}

	memset(dev_data->flash + offset, 0xff, size);

	return 0;
}

static int flash_native_posix_write_protection(struct device *dev, bool enable)
{
	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void
flash_native_posix_pages_layout(struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	const struct flash_native_posix_config *config = DEV_CONFIG(dev);
	*layout = &config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static int flash_native_posix_init(struct device *dev)
{
	struct flash_native_posix_data *const data = DEV_DATA(dev);
	const struct flash_native_posix_config *config = DEV_CONFIG(dev);

	data->init_called = true;

	k_sem_init(&data->mutex, 1, 1);

	if (data->flash_path == NULL) {
		data->flash_path = default_flash_path;
	}

	data->fd = open(data->flash_path, O_RDWR | O_CREAT, (mode_t)0600);
	if (data->fd == -1) {
		posix_print_warning("Failed to open flash device file "
				    "%s: %s\n",
				    data->flash_path, strerror(errno));
		return -EIO;
	}

	if (ftruncate(data->fd, config->flash_size) == -1) {
		posix_print_warning("Failed to resize flash device file "
				    "%s: %s\n",
				    data->flash_path, strerror(errno));
		return -EIO;
	}

	data->flash = mmap(NULL, config->flash_size,
			   PROT_WRITE | PROT_READ, MAP_SHARED, data->fd, 0);
	if (data->flash == MAP_FAILED) {
		posix_print_warning("Failed to mmap flash device file "
				    "%s: %s\n",
				    data->flash_path, strerror(errno));
		return -EIO;
	}

	LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct flash_driver_api flash_native_posix_driver_api = {
	.read = flash_native_posix_read,
	.write = flash_native_posix_write,
	.erase = flash_native_posix_erase,
	.write_protection = flash_native_posix_write_protection,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_native_posix_pages_layout,
#endif
	.write_block_size = 1,
};

static const struct flash_native_posix_config flash_native_posix_config = {
	.flash_size = DT_FLASH_SIZE * 1024,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.layout = { .pages_count = (DT_FLASH_SIZE * 1024) /
				(CONFIG_FLASH_NATIVE_POSIX_SECTOR_SIZE * 1024),
		    .pages_size = CONFIG_FLASH_NATIVE_POSIX_SECTOR_SIZE * 1024
	},
#endif
};

static struct flash_native_posix_data flash_native_posix_data;

DEVICE_AND_API_INIT(flash_native_posix_0, DT_FLASH_DEV_NAME,
		    &flash_native_posix_init, &flash_native_posix_data,
		    &flash_native_posix_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &flash_native_posix_driver_api);

static void flash_native_posix_cleanup(void)
{
	struct flash_native_posix_data *const data = &flash_native_posix_data;
	const struct flash_native_posix_config *config =
		&flash_native_posix_config;

	if (!data->init_called) {
		return;
	}

	if (data->flash != MAP_FAILED) {
		munmap(data->flash, config->flash_size);
	}

	if (data->fd != -1) {
		close(data->fd);
	}
}

void flash_native_posix_options(void)
{
	static struct args_struct_t flash_options[] = {
		{ .manual = false,
		  .is_mandatory = false,
		  .is_switch = false,
		  .option = "flash",
		  .name = "path",
		  .type = 's',
		  .dest = (void *)&flash_native_posix_data.flash_path,
		  .call_when_found = NULL,
		  .descript = "Path to binary file to be used as flash" },
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(flash_options);
}

NATIVE_TASK(flash_native_posix_options, PRE_BOOT_1, 1);
NATIVE_TASK(flash_native_posix_cleanup, ON_EXIT, 1);
