/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sim_flash

#include <device.h>
#include <drivers/flash.h>
#include <init.h>
#include <kernel.h>
#include <sys/util.h>
#include <random/rand32.h>
#include <stats/stats.h>
#include <string.h>

#define SOC_NV_FLASH_NODE DT_CHILD(DT_DRV_INST(0), flash_sim_0)

#define FLASH_SIMULATOR_BASE_OFFSET DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define FLASH_SIMULATOR_ERASE_UNIT DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define FLASH_SIMULATOR_PROG_UNIT DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_SIMULATOR_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)

#define FLASH_SIMULATOR_ERASE_VALUE \
		DT_PROP(DT_PARENT(SOC_NV_FLASH_NODE), erase_value)

#define FLASH_SIMULATOR_PAGE_COUNT (FLASH_SIMULATOR_FLASH_SIZE / \
				    FLASH_SIMULATOR_ERASE_UNIT)

#if (FLASH_SIMULATOR_ERASE_UNIT % FLASH_SIMULATOR_PROG_UNIT)
#error "Erase unit must be a multiple of program unit"
#endif

static const struct flash_driver_api flash_sim_api;

static const struct flash_parameters flash_sim_parameters = {
	.write_block_size = FLASH_SIMULATOR_PROG_UNIT,
	.erase_value = FLASH_SIMULATOR_ERASE_VALUE
};

static int flash_range_is_valid(const struct device *dev, off_t offset,
				size_t len)
{
	ARG_UNUSED(dev);
	if ((offset + len > FLASH_SIMULATOR_FLASH_SIZE +
			    FLASH_SIMULATOR_BASE_OFFSET) ||
	    (offset < FLASH_SIMULATOR_BASE_OFFSET)) {
		return 0;
	}

	return 1;
}

static int flash_sim_read(const struct device *dev, const off_t offset,
			  void *data,
			  const size_t len)
{
	ARG_UNUSED(dev);

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_FLASH_SIMULATOR_UNALIGNED_READ)) {
		if ((offset % FLASH_SIMULATOR_PROG_UNIT) ||
		    (len % FLASH_SIMULATOR_PROG_UNIT)) {
			return -EINVAL;
		}
	}

	memset(data, flash_sim_parameters.erase_value, len);

	return 0;
}

static int flash_sim_write(const struct device *dev, const off_t offset,
			   const void *data, const size_t len)
{
	ARG_UNUSED(dev);

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	if ((offset % FLASH_SIMULATOR_PROG_UNIT) ||
	    (len % FLASH_SIMULATOR_PROG_UNIT)) {
		return -EINVAL;
	}

	return 0;
}

static int flash_sim_erase(const struct device *dev, const off_t offset,
			   const size_t len)
{
	ARG_UNUSED(dev);

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	/* erase operation must be aligned to the erase unit boundary */
	if ((offset % FLASH_SIMULATOR_ERASE_UNIT) ||
	    (len % FLASH_SIMULATOR_ERASE_UNIT)) {
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_sim_pages_layout = {
	.pages_count = FLASH_SIMULATOR_PAGE_COUNT,
	.pages_size = FLASH_SIMULATOR_ERASE_UNIT,
};

static void flash_sim_page_layout(const struct device *dev,
				  const struct flash_pages_layout **layout,
				  size_t *layout_size)
{
	*layout = &flash_sim_pages_layout;
	*layout_size = 1;
}
#endif

static const struct flash_parameters *
flash_sim_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_sim_parameters;
}

static const struct flash_driver_api flash_sim_api = {
	.read = flash_sim_read,
	.write = flash_sim_write,
	.erase = flash_sim_erase,
	.get_parameters = flash_sim_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_sim_page_layout,
#endif
};


static int flash_init(const struct device *dev)
{
	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_init, NULL,
		    NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &flash_sim_api);
