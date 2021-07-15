/*
 * Copyright (c) 2021 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/fixed_partition.h>

int fxp_read(const struct fxp_info *fxp, off_t off, void *dst, size_t len)
{
	return fxp->read(fxp, off, dst, len);
}


int fxp_write(const struct fxp_info *fxp, off_t off, const void *src,
	      size_t len)
{
	return fxp->write(fxp, off, src, len);
}

int fxp_erase(const struct fxp_info *fxp, off_t off, size_t len)
{
	return fxp->erase(fxp, off, len);
}

const struct flash_parameters *fxp_get_parameters(const struct fxp_info *fxp)
{
	if (!fxp) {
		return NULL;
	}

	return flash_get_parameters(fxp->device);
}

#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
struct fxp_block_info {
	size_t p_size;
	size_t p_left;
};

static int fxp_get_block_info(const struct fxp_info *fxp, off_t offset,
			      struct fxp_block_info *info)
{
	if (!fxp) {
		return -EINVAL;
	}

	const struct device *dev = fxp->device;
	const struct flash_driver_api *api = dev->api;
	const struct flash_pages_layout *layout;
	size_t layout_size, idx_jmp;

	offset = fxp->off;
	api->page_layout(dev, &layout, &layout_size);

	while (layout_size--) {
		idx_jmp = offset / layout->pages_size;
		if (idx_jmp < layout->pages_count) {
			info->p_size = layout->pages_size;
			info->p_left = layout->pages_count - idx_jmp;
			return 0;
		}

		offset -= (layout->pages_size * layout->pages_count);
		layout++;
	}

	return -EINVAL;
}

void fxp_page_foreach(const struct fxp_info *fxp, flash_page_cb cb, void *data)
{
	struct fxp_block_info b_info;
	struct flash_pages_info info;

	info.start_offset = 0;
	info.index = 0U;

	while (fxp_get_block_info(fxp, info.start_offset, &b_info) == 0) {
		while (b_info.p_left) {
			if ((info.start_offset >= fxp->size) ||
			    (!cb(&info, data))) {
				return;
			}
			b_info.p_left--;
			info.size = b_info.p_size;
			info.start_offset += b_info.p_size;
			info.index++;
		}
	}
}

static int fxp_get_page_info(const struct fxp_info *fxp, off_t offset,
			     uint32_t idx, struct flash_pages_info *info)
{
	struct fxp_block_info b_info;
	off_t start_offset;
	uint32_t index;

	start_offset = 0;
	index = 0U;

	while (fxp_get_block_info(fxp, info->start_offset, &b_info) == 0) {
		uint32_t idx_jmp;

		if (offset) {
			idx_jmp = (offset - start_offset) / b_info.p_size;
		} else {
			idx_jmp = idx - index;
		}

		idx_jmp = MIN(idx_jmp, b_info.p_left);

		start_offset += idx_jmp * b_info.p_size;
		index += idx_jmp;

		if (start_offset >= fxp->size) {
			break;
		}

		if (idx_jmp < b_info.p_left) {
			info->start_offset = start_offset;
			info->index = index;
			info->size = b_info.p_size;
			return 0;
		}

	}

	return -EINVAL;
}

int fxp_get_page_info_by_offs(const struct fxp_info *fxp, off_t offset,
				  struct flash_pages_info *info)
{
	return fxp_get_page_info(fxp, offset, 0U, info);
}

int fxp_get_page_info_by_idx(const struct fxp_info *fxp, uint32_t idx,
			     struct flash_pages_info *info)
{
	return fxp_get_page_info(fxp, 0, idx, info);
}

static bool page_count_cb(const struct flash_pages_info *info, void *data)
{
	size_t *pages = (size_t *)data;

	(*pages) += 1;
	return true;
}

size_t fxp_get_page_count(const struct fxp_info *fxp)
{
	size_t pages = 0;

	fxp_page_foreach(fxp, page_count_cb, &pages);
	return pages;
}
#endif

/* End public functions */

/* Internal functions */
static bool outside_range(off_t off, size_t len, size_t size)
{
	return ((off < 0) || ((off + len) > size));
}

static int _read(const struct fxp_info *fxp, off_t off, void *dst, size_t len)
{
	if ((!fxp) || (outside_range(off, len, fxp->size))) {
		return -EINVAL;
	}

	if (!device_is_ready(fxp->device)) {
		return -EIO;
	}

	return flash_read(fxp->device, off + fxp->off, dst, len);
}

static int _write(const struct fxp_info *fxp, off_t off, const void *src,
		  size_t len)
{
	if ((!fxp) || (outside_range(off, len, fxp->size))) {
		return -EINVAL;
	}

	if (!device_is_ready(fxp->device)) {
		return -EIO;
	}

	return flash_write(fxp->device, off + fxp->off, (void *)src, len);
}

static int _erase(const struct fxp_info *fxp, off_t off, size_t len)
{
	if ((!fxp) || (outside_range(off, len, fxp->size))) {
		return -EINVAL;
	}

	if (!device_is_ready(fxp->device)) {
		return -EIO;
	}

	return flash_erase(fxp->device, off + fxp->off, len);
}

/* End Internal functions */

#define DT_DRV_COMPAT fixed_partitions

#define GEN_FXP_INFO(_name, _dev, _off, _size) \
	const struct fxp_info _name = { \
		.device = _dev, \
		.off = _off, \
		.size = _size, \
		.read = _read, \
		.write = _write, \
		.erase = _erase, \
	};

#define GET_PART_SIZE(n) DT_PROP_OR(n, size, DT_REG_SIZE(n))

#define GEN_FXP_STRUCT(n) \
	GEN_FXP_INFO(UTIL_CAT(fxp_, DT_NODELABEL(n)), \
		     DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(n)), \
		     DT_REG_ADDR(n), GET_PART_SIZE(n))

#define FOREACH_PARTITION(n) \
	DT_FOREACH_CHILD(DT_DRV_INST(n), GEN_FXP_STRUCT)

DT_INST_FOREACH_STATUS_OKAY(FOREACH_PARTITION)
