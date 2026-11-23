/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2022 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <hardware/flash.h>

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK
#include <rp2350_smp.h>
#endif

#include "flash_priv.h"

LOG_MODULE_REGISTER(flash_rpi_pico, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT raspberrypi_pico_flash_controller

/*
 * Writing or erasing flash takes the XIP interface away from every core. The
 * non-SMP code below keeps interrupts locked on the calling core, which is
 * the only one, and is also used with SMP on one CPU. With SMP on more than
 * one CPU the other core would keep executing from flash while the flash is
 * being reprogrammed, so it is parked in RAM for each page written and each
 * erase unit (see rp2350_smp_park_other()).
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP) || (CONFIG_MP_MAX_NUM_CPUS == 1) ||
	     IS_ENABLED(CONFIG_RPI_PICO_SMP_FLASH_PARK),
	     "flash_rpi_pico with CONFIG_SMP on more than one CPU needs "
	     "CONFIG_RPI_PICO_SMP_FLASH_PARK to stop the other core during write and erase");

#define SOC_NV_FLASH_NODE SOC_NV_FLASH_CHILD_NODE(0)

#define PAGE_SIZE   256
#define SECTOR_SIZE DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define ERASE_VALUE 0xff
#define FLASH_SIZE  DT_REG_SIZE(SOC_NV_FLASH_NODE)

#define FLASH_SEM_TIMEOUT (k_is_in_isr() ? K_NO_WAIT : K_FOREVER)

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK
#define PARK_TIMEOUT K_MSEC(CONFIG_RPI_PICO_SMP_FLASH_PARK_TIMEOUT_MS)

/*
 * Between two units (page or erase unit) of one call the other core runs
 * again and this core has its interrupts enabled again. That alone is not
 * enough for a thread there that sleeps and polls: its timer interrupt is
 * taken by core 0, which then has to send the other core a scheduler IPI,
 * and the other core has to run the thread before it is parked again. So
 * sleep for a moment in between. A sleep, not k_yield(): a yield on this core
 * does not run a thread of the other core. Only called from a thread.
 */
static void flash_rpi_park_gap(void)
{
	if (CONFIG_RPI_PICO_SMP_FLASH_PARK_GAP_US > 0) {
		(void)k_usleep(CONFIG_RPI_PICO_SMP_FLASH_PARK_GAP_US);
	}
}
#endif

#ifdef CONFIG_FLASH_RPI_PICO_READ_UNTRANSLATED
#define PICO_FLASH_READ_BASE XIP_NOCACHE_NOALLOC_NOTRANSLATE_BASE
#else
#define PICO_FLASH_READ_BASE DT_REG_ADDR(SOC_NV_FLASH_NODE)
#endif

static const struct flash_parameters flash_rpi_parameters = {
	.write_block_size = 1,
	.erase_value = ERASE_VALUE,
};

static uint8_t flash_ram_buffer[PAGE_SIZE];

#ifdef CONFIG_MULTITHREADING
static K_SEM_DEFINE(flash_rpi_sem, 1, 1);
#endif

static inline int flash_rpi_sem_take(void)
{
#ifdef CONFIG_MULTITHREADING
	return k_sem_take(&flash_rpi_sem, FLASH_SEM_TIMEOUT);
#else
	return 0;
#endif
}

static inline void flash_rpi_sem_give(void)
{
#ifdef CONFIG_MULTITHREADING
	k_sem_give(&flash_rpi_sem);
#endif
}

static bool is_valid_range(off_t offset, uint32_t size)
{
	return (offset >= 0) && ((offset + size) <= FLASH_SIZE);
}

static int flash_rpi_read(const struct device *dev, off_t offset, void *data, size_t size)
{
	int ret;

	ARG_UNUSED(dev);

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Read range exceeds the flash boundaries");
		return -EINVAL;
	}

	ret = flash_rpi_sem_take();
	if (ret != 0) {
		return ret;
	}

	memcpy(data, (uint8_t *)(PICO_FLASH_READ_BASE + (uintptr_t)offset), size);

	flash_rpi_sem_give();

	return 0;
}

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK

/*
 * Program at most one page. The data is copied to RAM first, while the flash
 * can still be read: the source may be in flash, and so is the memcpy()
 * itself. Nothing but the pico-sdk routines, which run from RAM, may be
 * called between parking the other core and unparking it.
 */
static int flash_rpi_program_chunk(off_t offset, const uint8_t *data, size_t size)
{
	unsigned int key;
	int ret;

	memcpy(flash_ram_buffer, data, size);

	ret = rp2350_smp_park_other(&key, PARK_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	if (size == PAGE_SIZE) {
		flash_range_program(offset, flash_ram_buffer, size);
	} else {
		flash_write_partial(offset, flash_ram_buffer, size);
	}

	rp2350_smp_unpark_other(key);

	return 0;
}

static int flash_rpi_write(const struct device *dev, off_t offset, const void *data, size_t size)
{
	const uint8_t *data_pointer = data;
	size_t bytes_to_write;
	int ret;

	ARG_UNUSED(dev);

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Write range exceeds the flash boundaries. Offset=%#lx, Size=%u",
			(unsigned long)offset, size);
		return -EINVAL;
	}

	/* The other core can only be parked from a thread */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = flash_rpi_sem_take();
	if (ret != 0) {
		return ret;
	}

	while (size > 0) {
		/* Up to the end of the page: a write never crosses a page boundary */
		bytes_to_write = MIN(PAGE_SIZE - (offset & (PAGE_SIZE - 1)), size);

		ret = flash_rpi_program_chunk(offset, data_pointer, bytes_to_write);
		if (ret != 0) {
			/* What was written before stays written */
			break;
		}

		data_pointer += bytes_to_write;
		size -= bytes_to_write;
		offset += bytes_to_write;

		if (size > 0) {
			flash_rpi_park_gap();
		}
	}

	flash_rpi_sem_give();

	return ret;
}

#else /* !CONFIG_RPI_PICO_SMP_FLASH_PARK */

static int flash_rpi_write(const struct device *dev, off_t offset, const void *data, size_t size)
{
	uint32_t key;
	size_t bytes_to_write;
	uint8_t *data_pointer = (uint8_t *)data;
	int ret;

	ARG_UNUSED(dev);

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Write range exceeds the flash boundaries. Offset=%#lx, Size=%u",
			(unsigned long)offset, size);
		return -EINVAL;
	}

	ret = flash_rpi_sem_take();
	if (ret != 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_FLASH_RPI_PICO_LOCK_IRQS)) {
		key = irq_lock();
	}

	if ((offset & (PAGE_SIZE - 1)) > 0) {
		bytes_to_write = MIN(PAGE_SIZE - (offset & (PAGE_SIZE - 1)), size);
		memcpy(flash_ram_buffer, data_pointer, bytes_to_write);
		flash_write_partial(offset, flash_ram_buffer, bytes_to_write);
		data_pointer += bytes_to_write;
		size -= bytes_to_write;
		offset += bytes_to_write;
	}

	while (size >= PAGE_SIZE) {
		bytes_to_write = PAGE_SIZE;
		memcpy(flash_ram_buffer, data_pointer, bytes_to_write);
		flash_range_program(offset, flash_ram_buffer, bytes_to_write);
		data_pointer += bytes_to_write;
		size -= bytes_to_write;
		offset += bytes_to_write;
	}

	if (size > 0) {
		memcpy(flash_ram_buffer, data_pointer, size);
		flash_write_partial(offset, flash_ram_buffer, size);
	}

	if (IS_ENABLED(CONFIG_FLASH_RPI_PICO_LOCK_IRQS)) {
		irq_unlock(key);
	}

	flash_rpi_sem_give();

	return 0;
}

#endif /* !CONFIG_RPI_PICO_SMP_FLASH_PARK */

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK

static int flash_rpi_erase(const struct device *dev, off_t offset, size_t size)
{
	unsigned int key;
	size_t unit;
	int ret = 0;

	ARG_UNUSED(dev);

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Erase range exceeds the flash boundaries. Offset=%#lx, Size=%u",
			(unsigned long)offset, size);
		return -EINVAL;
	}

	if ((offset % SECTOR_SIZE) || (size % SECTOR_SIZE)) {
		LOG_ERR("Erase range is not a multiple of the sector size. Offset=%#lx, Size=%u",
			(unsigned long)offset, size);
		return -EINVAL;
	}

	/* The other core can only be parked from a thread */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	ret = flash_rpi_sem_take();
	if (ret != 0) {
		return ret;
	}

	while (size > 0) {
		/*
		 * The other core is parked for one unit at a time. A whole block is
		 * erased by one block erase command only if it is aligned, so the
		 * ends of a range are erased sector by sector.
		 */
		if (IS_ENABLED(CONFIG_RPI_PICO_SMP_FLASH_PARK_ERASE_BLOCK) &&
		    (offset % FLASH_BLOCK_SIZE) == 0 && size >= FLASH_BLOCK_SIZE) {
			unit = FLASH_BLOCK_SIZE;
		} else {
			unit = SECTOR_SIZE;
		}

		ret = rp2350_smp_park_other(&key, PARK_TIMEOUT);
		if (ret != 0) {
			/* What was erased before stays erased */
			break;
		}

		flash_range_erase(offset, unit);

		rp2350_smp_unpark_other(key);

		offset += unit;
		size -= unit;

		if (size > 0) {
			flash_rpi_park_gap();
		}
	}

	flash_rpi_sem_give();

	return ret;
}

#else /* !CONFIG_RPI_PICO_SMP_FLASH_PARK */

static int flash_rpi_erase(const struct device *dev, off_t offset, size_t size)
{
	uint32_t key;
	int ret;

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Erase range exceeds the flash boundaries. Offset=%#lx, Size=%u",
			(unsigned long)offset, size);
		return -EINVAL;
	}

	if ((offset % SECTOR_SIZE) || (size % SECTOR_SIZE)) {
		LOG_ERR("Erase range is not a multiple of the sector size. Offset=%#lx, Size=%u",
			(unsigned long)offset, size);
		return -EINVAL;
	}

	ret = flash_rpi_sem_take();
	if (ret != 0) {
		return ret;
	}

	if (IS_ENABLED(CONFIG_FLASH_RPI_PICO_LOCK_IRQS)) {
		key = irq_lock();
	}

	flash_range_erase(offset, size);

	if (IS_ENABLED(CONFIG_FLASH_RPI_PICO_LOCK_IRQS)) {
		irq_unlock(key);
	}

	flash_rpi_sem_give();

	return 0;
}

#endif /* !CONFIG_RPI_PICO_SMP_FLASH_PARK */

static const struct flash_parameters *flash_rpi_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_rpi_parameters;
}

#if CONFIG_FLASH_PAGE_LAYOUT

static const struct flash_pages_layout flash_rpi_pages_layout = {
	.pages_count = FLASH_SIZE / SECTOR_SIZE,
	.pages_size = SECTOR_SIZE,
};

void flash_rpi_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
			   size_t *layout_size)
{
	*layout = &flash_rpi_pages_layout;
	*layout_size = 1;
}

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_rpi_driver_api) = {
	.read = flash_rpi_read,
	.write = flash_rpi_write,
	.erase = flash_rpi_erase,
	.get_parameters = flash_rpi_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_rpi_page_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		      &flash_rpi_driver_api);
