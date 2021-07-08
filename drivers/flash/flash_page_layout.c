/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/flash.h>

#if !defined(CONFIG_FLASH_PAGE_LAYOUT_WITHOUT_API_PAGE_LAYOUT)

static int flash_get_page_info_non_syscall(const struct device *dev, off_t offs,
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
	return flash_get_page_info_non_syscall(dev, offs, 0U, info);
}

int z_impl_flash_get_page_info_by_idx(const struct device *dev,
				      uint32_t page_index,
				      struct flash_pages_info *info)
{
	return flash_get_page_info_non_syscall(dev, 0, page_index, info);
}

#else

int z_impl_flash_get_page_info_by_offs(const struct device *dev, off_t offs,
				       struct flash_pages_info *info)
{
	const struct flash_driver_api *api = dev->api;
	const struct flash_parameters *fparam = api->get_parameters(dev);
	struct flash_page_info fpi;
	int rc = -EINVAL;

	rc = api->get_page_info(dev, offs, &fpi);

	if (rc == 0) {
		info->start_offset = fpi.offset;
		info->size = fpi.size;

		if (!(fparam->flags & FPF_NON_UNIFORM_LAYOUT)) {
			/* Uniform layout */
			info->index = offs / fpi.size;
		} else {
			/* Non-uniform layout requires to iterate through the pages */
			info->index = 0;
			while (fpi.offset) {
				++info->index;
				api->get_page_info(dev, fpi.offset - fpi.size, &fpi);
				if (rc < 0) {
					break;
				}
			}
		}
	}

	return rc;
}

int z_impl_flash_get_page_info_by_idx(const struct device *dev,
				      uint32_t page_index,
				      struct flash_pages_info *info)
{
	const struct flash_driver_api *api = dev->api;
	const struct flash_parameters *fparam = api->get_parameters(dev);
	struct flash_page_info fpi;
	int rc = -EINVAL;
	ssize_t page_count = 0;


	BUILD_ASSERT(sizeof(ssize_t) >= sizeof(uint32_t),
		 "The function will not work properly when uin32_t > ssize_t");
	/*
	 * Get total page count to check if request does not fall out of the flash range;
	 * in case of the non-uniform layout, the value will be also used to evaluate
	 * whether it is better to start index calculation from the beginning or the end
	 * of the flash.
	 */
	if ((ssize_t)page_index < 0) {
		return -EINVAL;
	}

	page_count = api->get_page_count(dev);
	if (page_count < 0) {
		return (int)page_count;
	}

	rc = api->get_page_info(dev, 0, &fpi);

	if (rc >= 0 && page_count < (ssize_t)page_index) {
		info->index = page_index;

		if (!(fparam->flags & FPF_NON_UNIFORM_LAYOUT)) {
			/* Uniform layout */
			info->start_offset = (ssize_t)page_index * fpi.size;
			info->size = fpi.size;
		} else {
			/* Non-uniform layout */
			ssize_t size = api->get_size(dev);

			if (size < 0) {
				return size;
			}

			if ((ssize_t)page_index > (page_count >> 1)) {
				/*
				 * For an index that is above the half of the page count,
				 * go from the end of the flash.
				 */
				fpi.offset = size;
				fpi.size = 1;

				while (rc >= 0 && page_count >= (ssize_t)page_index) {
					--page_count;
					rc = api->get_page_info(dev, fpi.offset - fpi.size, &fpi);
				}
			} else {
				/*
				 * For an index that is below or equal to the half of
				 * the page count go from beginning of the flash.
				 */
				page_count = 0;
				fpi.offset = 0;
				fpi.size = 0;

				while (rc >= 0 && page_count != (ssize_t)page_index) {
					++page_count;
					rc = api->get_page_info(dev, fpi.offset + fpi.size, &fpi);
					if (rc < 0) {
						break;
					}
				}
				if (rc > 0) {
					info->start_offset = fpi.offset;
					info->size = fpi.size;
				}
			}
		}
	}

	return rc;
}
#endif /* !defined(CONFIG_FLASH_PAGE_LAYOUT_WITHOUT_API_PAGE_LAYOUT) */

#if !defined(CONFIG_FLASH_PAGE_LAYOUT_WITHOUT_API_PAGE_LAYOUT)

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

#else

void flash_page_foreach(const struct device *dev, flash_page_cb cb,
			void *data)
{
	const struct flash_parameters *fparam = flash_get_parameters(dev);
	struct flash_pages_info page_info;
	size_t num_blocks;
	struct flash_page_info fpi;

	num_blocks = flash_get_page_count(dev);
	page_info.start_offset = 0;
	page_info.index = 0;


	if (!(fparam->flags & FPF_NON_UNIFORM_LAYOUT)) {
		flash_get_page_info(dev, 0, &fpi);
		page_info.size = fpi.size;

		while (page_info.index < num_blocks) {
			if (!cb(&page_info, data)) {
				return;
			}
			page_info.index++;
			page_info.start_offset += fpi.size;
		}
	} else {
		while (page_info.index < num_blocks) {
			flash_get_page_info(dev, page_info.start_offset, &fpi);

			page_info.size = fpi.size;

			if (!cb(&page_info, data)) {
				return;
			}

			page_info.index++;
			page_info.start_offset += fpi.size;
		}
	}
}
#endif /* !defined(CONFIG_FLASH_PAGE_LAYOUT_WITHOUT_API_PAGE_LAYOUT) */
