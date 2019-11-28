/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Heavily based on flash_native_posix.c, which is:
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/eeprom.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "cmdline.h"
#include "soc.h"

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(eeprom_native_posix);

static const char default_eeprom_path[] = "eeprom.bin";

struct eeprom_native_posix_data {
	const char *path;
	int fd;
	u8_t *eeprom;
	bool init_called;
};

struct eeprom_native_posix_config {
	size_t size;
	bool readonly;
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CONFIG(dev) ((dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct eeprom_native_posix_data *const)(dev)->driver_data)

static int eeprom_native_posix_read(struct device *dev, off_t offset,
				    void *buf, size_t len)
{
	struct eeprom_native_posix_data *const data = DEV_DATA(dev);
	const struct eeprom_native_posix_config *config = DEV_CONFIG(dev);

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	if (data->eeprom == MAP_FAILED) {
		LOG_ERR("no EEPROM device mapped");
		return -EIO;
	}

	memcpy(buf, data->eeprom + offset, len);

	return 0;
}

static int eeprom_native_posix_write(struct device *dev, off_t offset,
				     const void *buf, size_t len)
{
	struct eeprom_native_posix_data *const data = DEV_DATA(dev);
	const struct eeprom_native_posix_config *config = DEV_CONFIG(dev);

	if (config->readonly) {
		LOG_WRN("attempt to write to read-only device");
		return -EACCES;
	}

	if (!len) {
		return 0;
	}

	if ((offset + len) > config->size) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	if (data->eeprom == MAP_FAILED) {
		LOG_ERR("no EEPROM device mapped");
		return -EIO;
	}

	memcpy(data->eeprom + offset, buf, len);

	return 0;
}


static size_t eeprom_native_posix_size(struct device *dev)
{
	const struct eeprom_native_posix_config *config = DEV_CONFIG(dev);

	return config->size;
}

static int eeprom_native_posix_init(struct device *dev)
{
	struct eeprom_native_posix_data *const data = DEV_DATA(dev);
	const struct eeprom_native_posix_config *config = DEV_CONFIG(dev);

	data->init_called = true;

	if (data->path == NULL) {
		data->path = default_eeprom_path;
	}

	data->fd = open(data->path, O_RDWR | O_CREAT, (mode_t)0600);
	if (data->fd == -1) {
		posix_print_warning("failed to open EEPROM device file "
				    "%s: %s\n",
				    data->path, strerror(errno));
		return -EIO;
	}

	if (ftruncate(data->fd, config->size) == -1) {
		posix_print_warning("failed to resize EEPROM device file "
				    "%s: %s\n",
				    data->path, strerror(errno));
		return -EIO;
	}

	data->eeprom = mmap(NULL, config->size, PROT_WRITE | PROT_READ,
			    MAP_SHARED, data->fd, 0);
	if (data->eeprom == MAP_FAILED) {
		posix_print_warning("failed to mmap EEPROM device file "
				    "%s: %s\n",
				    data->path, strerror(errno));
		return -EIO;
	}

	return 0;
}

static const struct eeprom_driver_api eeprom_native_posix_driver_api = {
	.read = eeprom_native_posix_read,
	.write = eeprom_native_posix_write,
	.size = eeprom_native_posix_size,
};

static const struct eeprom_native_posix_config eeprom_native_posix_config_0 = {
	.size = DT_INST_0_ZEPHYR_NATIVE_POSIX_EEPROM_SIZE,
	.readonly = DT_INST_0_ZEPHYR_NATIVE_POSIX_EEPROM_READ_ONLY,
};

static struct eeprom_native_posix_data eeprom_native_posix_data_0;

DEVICE_AND_API_INIT(eeprom_native_posix_0,
		DT_INST_0_ZEPHYR_NATIVE_POSIX_EEPROM_LABEL,
		&eeprom_native_posix_init, &eeprom_native_posix_data_0,
		&eeprom_native_posix_config_0, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&eeprom_native_posix_driver_api);

static void eeprom_native_posix_cleanup_0(void)
{
	struct eeprom_native_posix_data *const data =
		&eeprom_native_posix_data_0;
	const struct eeprom_native_posix_config *config =
		&eeprom_native_posix_config_0;

	if (!data->init_called) {
		return;
	}

	if (data->eeprom != MAP_FAILED) {
		munmap(data->eeprom, config->size);
	}

	if (data->fd != -1) {
		close(data->fd);
	}
}

void eeprom_native_posix_options_0(void)
{
	static struct args_struct_t eeprom_options[] = {
		{
			.manual = false,
			.is_mandatory = false,
			.is_switch = false,
			.option = "eeprom",
			.name = "path",
			.type = 's',
			.dest = (void *)&eeprom_native_posix_data_0.path,
			.call_when_found = NULL,
			.descript = "Path to binary file to be used as EEPROM",
		},
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(eeprom_options);
}

NATIVE_TASK(eeprom_native_posix_options_0, PRE_BOOT_1, 1);
NATIVE_TASK(eeprom_native_posix_cleanup_0, ON_EXIT, 1);
