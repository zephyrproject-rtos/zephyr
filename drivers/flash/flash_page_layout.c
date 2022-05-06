/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/flash.h>

static int flash_get_page_info(const struct device *dev, off_t offs,
			       uint32_t index, struct flash_pages_info *info)
{
	const struct flash_driver_api *api = dev->api;
	const struct flash_pages_layout *layout;
	size_t layout_size;
	uint32_t index_jmp;

	info->start_offset = 0;
	info->index = 0U;

	api->page_layout(dev, &layout, &layout_size);

	while (layout_size--) {
		info->size = layout->pages_size;
		if (offs == 0) {
			index_jmp = index - info->index;
		} else {
			index_jmp = (offs - info->start_offset) / info->size;
		}

		index_jmp = MIN(index_jmp, layout->pages_count);
		info->start_offset += (index_jmp * info->size);
		info->index += index_jmp;
		if (index_jmp < layout->pages_count) {
			return 0;
		}

		layout++;
	}

	return -EINVAL; /* page at offs or idx doesn't exist */
}

int z_impl_flash_get_page_info_by_offs(const struct device *dev, off_t offs,
				       struct flash_pages_info *info)
{
	return flash_get_page_info(dev, offs, 0U, info);
}

int z_impl_flash_get_page_info_by_idx(const struct device *dev,
				      uint32_t page_index,
				      struct flash_pages_info *info)
{
	return flash_get_page_info(dev, 0, page_index, info);
}

size_t z_impl_flash_get_page_count(const struct device *dev)
{
	const struct flash_driver_api *api = dev->api;
	const struct flash_pages_layout *layout;
	size_t layout_size;
	size_t count = 0;

	api->page_layout(dev, &layout, &layout_size);

	while (layout_size--) {
		count += layout->pages_count;
		layout++;
	}

	return count;
}

void flash_page_foreach(const struct device *dev, flash_page_cb cb,
			void *data)
{
	const struct flash_driver_api *api = dev->api;
	const struct flash_pages_layout *layout;
	struct flash_pages_info page_info;
	size_t block, num_blocks, page = 0, i;
	off_t off = 0;

	api->page_layout(dev, &layout, &num_blocks);

	for (block = 0; block < num_blocks; block++) {
		const struct flash_pages_layout *l = &layout[block];
		page_info.size = l->pages_size;

		for (i = 0; i < l->pages_count; i++) {
			page_info.start_offset = off;
			page_info.index = page;

			if (!cb(&page_info, data)) {
				return;
			}

			off += page_info.size;
			page++;
		}
	}
}
