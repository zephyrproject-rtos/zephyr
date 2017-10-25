/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <flash.h>

static int _flash_get_page_info(struct device *dev, off_t offs,
				   bool use_addr, struct flash_pages_info *info)
{
	const struct flash_driver_api *api = dev->driver_api;
	const struct flash_pages_layout *layout;
	size_t page_count = 0;
	off_t group_offs = 0;
	u32_t num_in_group;
	off_t end = 0;
	size_t layout_size;

	api->page_layout(dev, &layout, &layout_size);

	while (layout_size--) {
		if (use_addr) {
			end += layout->pages_count * layout->pages_size;
		} else {
			end += layout->pages_count;
		}

		if (offs < end) {
			info->size = layout->pages_size;

			if (use_addr) {
				num_in_group = (offs - group_offs) /
					       layout->pages_size;
			} else {
				num_in_group = offs - page_count;
			}

			info->start_offset = group_offs +
					     num_in_group * layout->pages_size;
			info->index = page_count + num_in_group;

			return 0;
		}

		group_offs += layout->pages_count * layout->pages_size;
		page_count += layout->pages_count;

		layout++;
	}

	return -EINVAL; /* page of the index doesn't exist */
}

int _impl_flash_get_page_info_by_offs(struct device *dev, off_t offs,
				      struct flash_pages_info *info)
{
	return _flash_get_page_info(dev, offs, true, info);
}

int _impl_flash_get_page_info_by_idx(struct device *dev, u32_t page_index,
				     struct flash_pages_info *info)
{
	return _flash_get_page_info(dev, page_index, false, info);
}

size_t _impl_flash_get_page_count(struct device *dev)
{
	const struct flash_driver_api *api = dev->driver_api;
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

void flash_page_foreach(struct device *dev, flash_page_cb cb, void *data)
{
	const struct flash_driver_api *api = dev->driver_api;
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
