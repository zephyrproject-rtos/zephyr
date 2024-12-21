/*
 * Copyright (c) 2018 Aurelien Jarno
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This driver defines a page as the erase_block_size.
 * This driver defines a write page as defined by the flash controller
 * This driver defines a section as a contiguous array of bytes
 * This driver defines an area as the entire flash area
 * This driver defines the write block size as the minimum write block size
 */

#define DT_DRV_COMPAT atmel_sam_flash_controller

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/barrier.h>
#include <string.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_sam, CONFIG_FLASH_LOG_LEVEL);

#define SAM_FLASH_WRITE_PAGE_SIZE (512)

typedef void (*sam_flash_irq_init_fn_ptr)(void);

struct sam_flash_config {
	Efc *regs;
	sam_flash_irq_init_fn_ptr irq_init;
	off_t area_address;
	off_t area_size;
	struct flash_parameters parameters;
	struct flash_pages_layout *pages_layouts;
	size_t pages_layouts_size;
};

struct sam_flash_erase_data {
	off_t section_start;
	size_t section_end;
	bool succeeded;
};

struct sam_flash_data {
	const struct device *dev;
	struct k_spinlock lock;
	struct sam_flash_erase_data erase_data;
	struct k_sem ready_sem;
};

static bool sam_flash_validate_offset_len(off_t offset, size_t len)
{
	if (offset < 0) {
		return false;
	}

	if ((offset + len) < len) {
		return false;
	}

	return true;
}

static bool sam_flash_aligned(size_t value, size_t alignment)
{
	return (value & (alignment - 1)) == 0;
}

static bool sam_flash_offset_is_on_write_page_boundary(off_t offset)
{
	return sam_flash_aligned(offset, SAM_FLASH_WRITE_PAGE_SIZE);
}

static inline void sam_flash_mask_ready_interrupt(const struct sam_flash_config *config)
{
	Efc *regs = config->regs;

	regs->EEFC_FMR &= ~EEFC_FMR_FRDY;
}

static inline void sam_flash_unmask_ready_interrupt(const struct sam_flash_config *config)
{
	Efc *regs = config->regs;

	regs->EEFC_FMR |= EEFC_FMR_FRDY;
}

static void sam_flash_isr(const struct device *dev)
{
	struct sam_flash_data *data = dev->data;
	const struct sam_flash_config *config = dev->config;

	sam_flash_mask_ready_interrupt(config);
	k_sem_give(&data->ready_sem);
}

static int sam_flash_section_wait_until_ready(const struct device *dev)
{
	struct sam_flash_data *data = dev->data;
	const struct sam_flash_config *config = dev->config;
	Efc *regs = config->regs;
	uint32_t eefc_fsr;

	k_sem_reset(&data->ready_sem);
	sam_flash_unmask_ready_interrupt(config);

	if (k_sem_take(&data->ready_sem, K_MSEC(500)) < 0) {
		LOG_ERR("Command did not execute in time");
		return -EFAULT;
	}

	/* FSR register is cleared on read */
	eefc_fsr = regs->EEFC_FSR;

	if (eefc_fsr & EEFC_FSR_FCMDE) {
		LOG_ERR("Invalid command requested");
		return -EPERM;
	}

	if (eefc_fsr & EEFC_FSR_FLOCKE) {
		LOG_ERR("Tried to modify locked region");
		return -EPERM;
	}

	if (eefc_fsr & EEFC_FSR_FLERR) {
		LOG_ERR("Programming failed");
		return -EPERM;
	}

	return 0;
}

static bool sam_flash_section_is_within_area(const struct device *dev, off_t offset, size_t len)
{
	const struct sam_flash_config *config = dev->config;

	if ((offset + ((off_t)len)) < offset) {
		return false;
	}

	if ((offset >= 0) && ((offset + len) <= config->area_size)) {
		return true;
	}

	LOG_WRN("Section from 0x%x to 0x%x is not within flash area (0x0 to %x)",
		(size_t)offset, (size_t)(offset + len), (size_t)config->area_size);

	return false;
}

static bool sam_flash_section_is_aligned_with_write_block_size(const struct device *dev,
							       off_t offset, size_t len)
{
	const struct sam_flash_config *config = dev->config;

	if (sam_flash_aligned(offset, config->parameters.write_block_size) &&
	    sam_flash_aligned(len, config->parameters.write_block_size)) {
		return true;
	}

	LOG_WRN("Section from 0x%x to 0x%x is not aligned with write block size (%u)",
		(size_t)offset, (size_t)(offset + len), config->parameters.write_block_size);

	return false;
}

static bool sam_flash_section_is_aligned_with_pages(const struct device *dev, off_t offset,
						    size_t len)
{
	const struct sam_flash_config *config = dev->config;
	struct flash_pages_info pages_info;

	/* Get the page offset points to */
	if (flash_get_page_info_by_offs(dev, offset, &pages_info) < 0) {
		return false;
	}

	/* Validate offset points to start of page */
	if (offset != pages_info.start_offset) {
		return false;
	}

	/* Check if end of section is aligned with end of area */
	if ((offset + len) == (config->area_size)) {
		return true;
	}

	/* Get the page pointed to by end of section */
	if (flash_get_page_info_by_offs(dev, offset + len, &pages_info) < 0) {
		return false;
	}

	/* Validate offset points to start of page */
	if ((offset + len) != pages_info.start_offset) {
		return false;
	}

	return true;
}

static int sam_flash_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct sam_flash_data *sam_data = dev->data;
	const struct sam_flash_config *sam_config = dev->config;
	k_spinlock_key_t key;

	if (len == 0) {
		return 0;
	}

	if (!sam_flash_validate_offset_len(offset, len)) {
		return -EINVAL;
	}

	if (!sam_flash_section_is_within_area(dev, offset, len)) {
		return -EINVAL;
	}

	key = k_spin_lock(&sam_data->lock);
	memcpy(data, (uint8_t *)(sam_config->area_address + offset), len);
	k_spin_unlock(&sam_data->lock, key);
	return 0;
}

static int sam_flash_write_latch_buffer_to_page(const struct device *dev, off_t offset)
{
	const struct sam_flash_config *sam_config = dev->config;
	Efc *regs = sam_config->regs;
	uint32_t page = offset / SAM_FLASH_WRITE_PAGE_SIZE;

	regs->EEFC_FCR = EEFC_FCR_FCMD_WP | EEFC_FCR_FARG(page) | EEFC_FCR_FKEY_PASSWD;
	sam_flash_section_wait_until_ready(dev);
	return 0;
}

static int sam_flash_write_latch_buffer_to_previous_page(const struct device *dev, off_t offset)
{
	return sam_flash_write_latch_buffer_to_page(dev, offset - SAM_FLASH_WRITE_PAGE_SIZE);
}

static void sam_flash_write_dword_to_latch_buffer(off_t offset, uint32_t dword)
{
	*((uint32_t *)offset) = dword;
	barrier_dsync_fence_full();
}

static int sam_flash_write_dwords_to_flash(const struct device *dev, off_t offset,
					   const uint32_t *dwords, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		sam_flash_write_dword_to_latch_buffer(offset, dwords[i]);
		offset += sizeof(uint32_t);
		if (sam_flash_offset_is_on_write_page_boundary(offset)) {
			sam_flash_write_latch_buffer_to_previous_page(dev, offset);
		}
	}

	if (!sam_flash_offset_is_on_write_page_boundary(offset)) {
		sam_flash_write_latch_buffer_to_page(dev, offset);
	}

	return 0;
}

static int sam_flash_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct sam_flash_data *sam_data = dev->data;
	k_spinlock_key_t key;

	if (len == 0) {
		return 0;
	}

	if (!sam_flash_validate_offset_len(offset, len)) {
		return -EINVAL;
	}

	if (!sam_flash_section_is_aligned_with_write_block_size(dev, offset, len)) {
		return -EINVAL;
	}

	LOG_DBG("Writing sector from 0x%x to 0x%x", (size_t)offset, (size_t)(offset + len));

	key = k_spin_lock(&sam_data->lock);
	if (sam_flash_write_dwords_to_flash(dev, offset, data, len / sizeof(uint32_t)) < 0) {
		k_spin_unlock(&sam_data->lock, key);
		return -EAGAIN;
	}

	k_spin_unlock(&sam_data->lock, key);
	return 0;
}

static int sam_flash_unlock_write_page(const struct device *dev, uint16_t page_index)
{
	const struct sam_flash_config *sam_config = dev->config;
	Efc *regs = sam_config->regs;

	/* Perform unlock command of write page */
	regs->EEFC_FCR = EEFC_FCR_FCMD_CLB
		       | EEFC_FCR_FARG(page_index)
		       | EEFC_FCR_FKEY_PASSWD;

	return sam_flash_section_wait_until_ready(dev);
}

static int sam_flash_unlock_page(const struct device *dev, const struct flash_pages_info *info)
{
	uint16_t page_index_start;
	uint16_t page_index_end;
	int ret;

	/* Convert from page offset and size to write page index and count */
	page_index_start = info->start_offset / SAM_FLASH_WRITE_PAGE_SIZE;
	page_index_end = page_index_start + (info->size / SAM_FLASH_WRITE_PAGE_SIZE);

	for (uint16_t i = page_index_start; i < page_index_end; i++) {
		ret = sam_flash_unlock_write_page(dev, i);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int sam_flash_erase_page(const struct device *dev, const struct flash_pages_info *info)
{
	const struct sam_flash_config *sam_config = dev->config;
	Efc *regs = sam_config->regs;
	uint32_t page_index;
	int ret;

	/* Convert from page offset to write page index */
	page_index = info->start_offset / SAM_FLASH_WRITE_PAGE_SIZE;

	LOG_DBG("Erasing page at 0x%x of size 0x%x", (size_t)info->start_offset, info->size);

	/* Perform erase command of page */
	switch (info->size) {
	case 0x800:
		regs->EEFC_FCR = EEFC_FCR_FCMD_EPA
			       | EEFC_FCR_FARG(page_index)
			       | EEFC_FCR_FKEY_PASSWD;
		break;

	case 0x1000:
		regs->EEFC_FCR = EEFC_FCR_FCMD_EPA
			       | EEFC_FCR_FARG(page_index | 1)
			       | EEFC_FCR_FKEY_PASSWD;
		break;

	case 0x2000:
		regs->EEFC_FCR = EEFC_FCR_FCMD_EPA
			       | EEFC_FCR_FARG(page_index | 2)
			       | EEFC_FCR_FKEY_PASSWD;
		break;

	case 0x4000:
		regs->EEFC_FCR = EEFC_FCR_FCMD_EPA
			       | EEFC_FCR_FARG(page_index | 3)
			       | EEFC_FCR_FKEY_PASSWD;
		break;

	default:
		return -EINVAL;
	}

	ret = sam_flash_section_wait_until_ready(dev);
	if (ret == 0) {
		return ret;
	}

	LOG_ERR("Failed to erase page at 0x%x of size 0x%x", (size_t)info->start_offset,
		info->size);

	return ret;
}

static bool sam_flash_erase_foreach_page(const struct flash_pages_info *info, void *data)
{
	struct sam_flash_data *sam_data = data;
	const struct device *dev = sam_data->dev;
	struct sam_flash_erase_data *erase_data = &sam_data->erase_data;

	/* Validate we reached first page to erase */
	if (info->start_offset < erase_data->section_start) {
		/* Next page */
		return true;
	}

	/* Check if we've reached the end of pages to erase */
	if (info->start_offset >= erase_data->section_end) {
		/* Succeeded, stop iterating */
		erase_data->succeeded = true;
		return false;
	}

	if (sam_flash_unlock_page(dev, info) < 0) {
		/* Failed to unlock page, stop iterating */
		return false;
	}

	if (sam_flash_erase_page(dev, info) < 0) {
		/* Failed to erase page, stop iterating */
		return false;
	}

	/* Next page */
	return true;
}

static int sam_flash_erase(const struct device *dev, off_t offset, size_t size)
{
	struct sam_flash_data *sam_data = dev->data;
	k_spinlock_key_t key;

	if (size == 0) {
		return 0;
	}

	if (!sam_flash_validate_offset_len(offset, size)) {
		return -EINVAL;
	}

	if (!sam_flash_section_is_aligned_with_pages(dev, offset, size)) {
		return -EINVAL;
	}

	LOG_DBG("Erasing sector from 0x%x to 0x%x", (size_t)offset, (size_t)(offset + size));

	key = k_spin_lock(&sam_data->lock);
	sam_data->erase_data.section_start = offset;
	sam_data->erase_data.section_end = offset + size;
	sam_data->erase_data.succeeded = false;
	flash_page_foreach(dev, sam_flash_erase_foreach_page, sam_data);
	if (!sam_data->erase_data.succeeded) {
		k_spin_unlock(&sam_data->lock, key);
		return -EFAULT;
	}

	k_spin_unlock(&sam_data->lock, key);
	return 0;
}

static const struct flash_parameters *sam_flash_get_parameters(const struct device *dev)
{
	const struct sam_flash_config *config = dev->config;

	return &config->parameters;
}

static void sam_flash_api_pages_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	const struct sam_flash_config *config = dev->config;

	*layout = config->pages_layouts;
	*layout_size = config->pages_layouts_size;
}

static DEVICE_API(flash, sam_flash_api) = {
	.read = sam_flash_read,
	.write = sam_flash_write,
	.erase = sam_flash_erase,
	.get_parameters = sam_flash_get_parameters,
	.page_layout = sam_flash_api_pages_layout,
};

static int sam_flash_init(const struct device *dev)
{
	struct sam_flash_data *sam_data = dev->data;
	const struct sam_flash_config *sam_config = dev->config;

	sam_data->dev = dev;
	k_sem_init(&sam_data->ready_sem, 0, 1);
	sam_flash_mask_ready_interrupt(sam_config);
	sam_config->irq_init();
	return 0;
}

#define SAM_FLASH_DEVICE DT_INST(0, atmel_sam_flash)

#define SAM_FLASH_PAGES_LAYOUT(node_id, prop, idx)						\
	{											\
		.pages_count = DT_PHA_BY_IDX(node_id, prop, idx, pages_count),			\
		.pages_size = DT_PHA_BY_IDX(node_id, prop, idx, pages_size),			\
	}

#define SAM_FLASH_PAGES_LAYOUTS									\
	DT_FOREACH_PROP_ELEM_SEP(SAM_FLASH_DEVICE, erase_blocks, SAM_FLASH_PAGES_LAYOUT, (,))

#define SAM_FLASH_CONTROLLER(inst)								\
	struct flash_pages_layout sam_flash_pages_layouts##inst[] = {				\
		SAM_FLASH_PAGES_LAYOUTS								\
	};											\
												\
	static void sam_flash_irq_init_##inst(void)						\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),			\
			    sam_flash_isr, DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));							\
												\
	}											\
												\
	static const struct sam_flash_config sam_flash_config##inst = {				\
		.regs = (Efc *)DT_INST_REG_ADDR(inst),						\
		.irq_init = sam_flash_irq_init_##inst,						\
		.area_address = DT_REG_ADDR(SAM_FLASH_DEVICE),					\
		.area_size = DT_REG_SIZE(SAM_FLASH_DEVICE),					\
		.parameters = {									\
			.write_block_size = DT_PROP(SAM_FLASH_DEVICE, write_block_size),	\
			.erase_value = 0xFF,							\
		},										\
		.pages_layouts = sam_flash_pages_layouts##inst,					\
		.pages_layouts_size = ARRAY_SIZE(sam_flash_pages_layouts##inst),		\
	};											\
												\
	static struct sam_flash_data sam_flash_data##inst;					\
												\
	DEVICE_DT_INST_DEFINE(inst, sam_flash_init, NULL, &sam_flash_data##inst,		\
			      &sam_flash_config##inst, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,	\
			      &sam_flash_api);

SAM_FLASH_CONTROLLER(0)
