/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2017 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <storage/flash_map.h>
#include <drivers/flash.h>
#include <soc.h>
#include <init.h>

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
struct layout_data {
	u32_t area_idx;
	u32_t area_off;
	u32_t area_len;
	void *ret;        /* struct flash_area* or struct flash_sector* */
	u32_t ret_idx;
	u32_t ret_len;
	int status;
};
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

extern const struct flash_area *flash_map;
extern const int flash_map_entries;

static struct flash_area const *get_flash_area_from_id(int idx)
{
	for (int i = 0; i < flash_map_entries; i++) {
		if (flash_map[i].fa_id == idx) {
			return &flash_map[i];
		}
	}

	return NULL;
}

void flash_area_foreach(flash_area_cb_t user_cb, void *user_data)
{
	for (int i = 0; i < flash_map_entries; i++) {
		user_cb(&flash_map[i], user_data);
	}
}

int flash_area_open(u8_t id, const struct flash_area **fap)
{
	const struct flash_area *area;

	if (flash_map == NULL) {
		return -EACCES;
	}

	area = get_flash_area_from_id(id);
	if (area == NULL) {
		return -ENOENT;
	}

	*fap = area;
	return 0;
}

void flash_area_close(const struct flash_area *fa)
{
	/* nothing to do for now */
}

static inline bool is_in_flash_area_bounds(const struct flash_area *fa,
					   off_t off, size_t len)
{
	return (off <= fa->fa_size && off + len <= fa->fa_size);
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
/*
 * Check if a flash_page_foreach() callback should exit early, due to
 * one of the following conditions:
 *
 * - The flash page described by "info" is before the area of interest
 *   described in "data"
 * - The flash page is after the end of the area
 * - There are too many flash pages on the device to fit in the array
 *   held in data->ret. In this case, data->status is set to -ENOMEM.
 *
 * The value to return to flash_page_foreach() is stored in
 * "bail_value" if the callback should exit early.
 */
static bool should_bail(const struct flash_pages_info *info,
						struct layout_data *data,
						bool *bail_value)
{
	if (info->start_offset < data->area_off) {
		*bail_value = true;
		return true;
	} else if (info->start_offset >= data->area_off + data->area_len) {
		*bail_value = false;
		return true;
	} else if (data->ret_idx >= data->ret_len) {
		data->status = -ENOMEM;
		*bail_value = false;
		return true;
	}

	return false;
}

/*
 * Generic page layout discovery routine. This is kept separate to
 * support both the deprecated flash_area_to_sectors() and the current
 * flash_area_get_sectors(). A lot of this can be inlined once
 * flash_area_to_sectors() is removed.
 */
static int flash_area_layout(int idx, u32_t *cnt, void *ret,
flash_page_cb cb, struct layout_data *cb_data)
{
	struct device *flash_dev;

	cb_data->area_idx = idx;

	const struct flash_area *fa;

	fa = get_flash_area_from_id(idx);
	if (fa == NULL) {
		return -EINVAL;
	}

	cb_data->area_idx = idx;
	cb_data->area_off = fa->fa_off;
	cb_data->area_len = fa->fa_size;

	cb_data->ret = ret;
	cb_data->ret_idx = 0U;
	cb_data->ret_len = *cnt;
	cb_data->status = 0;

	flash_dev = device_get_binding(fa->fa_dev_name);
	if (flash_dev == NULL) {
		return -ENODEV;
	}

	flash_page_foreach(flash_dev, cb, cb_data);

	if (cb_data->status == 0) {
		*cnt = cb_data->ret_idx;
	}

	return cb_data->status;
}

static bool get_sectors_cb(const struct flash_pages_info *info, void *datav)
{
	struct layout_data *data = datav;
	struct flash_sector *ret = data->ret;
	bool bail;

	if (should_bail(info, data, &bail)) {
		return bail;
	}

	ret[data->ret_idx].fs_off = info->start_offset - data->area_off;
	ret[data->ret_idx].fs_size = info->size;
	data->ret_idx++;

	return true;
}

int flash_area_get_sectors(int idx, u32_t *cnt, struct flash_sector *ret)
{
	struct layout_data data;

	return flash_area_layout(idx, cnt, ret, get_sectors_cb, &data);
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

int flash_area_read(const struct flash_area *fa, off_t off, void *dst,
		    size_t len)
{
	struct device *dev;

	if (!is_in_flash_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	dev = device_get_binding(fa->fa_dev_name);

	return flash_read(dev, fa->fa_off + off, dst, len);
}

int flash_area_write(const struct flash_area *fa, off_t off, const void *src,
		     size_t len)
{
	struct device *flash_dev;
	int rc;

	if (!is_in_flash_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	flash_dev = device_get_binding(fa->fa_dev_name);

	rc = flash_write_protection_set(flash_dev, false);
	if (rc) {
		return rc;
	}

	rc = flash_write(flash_dev, fa->fa_off + off, (void *)src, len);

	/* Ignore errors here - this does not affect write operation */
	(void) flash_write_protection_set(flash_dev, true);

	return rc;
}

int flash_area_erase(const struct flash_area *fa, off_t off, size_t len)
{
	struct device *flash_dev;
	int rc;

	if (!is_in_flash_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	flash_dev = device_get_binding(fa->fa_dev_name);

	rc = flash_write_protection_set(flash_dev, false);
	if (rc) {
		return rc;
	}

	rc = flash_erase(flash_dev, fa->fa_off + off, len);

	/* Ignore errors here - this does not affect write operation */
	(void) flash_write_protection_set(flash_dev, true);

	return rc;
}

u8_t flash_area_align(const struct flash_area *fa)
{
	struct device *dev;

	dev = device_get_binding(fa->fa_dev_name);

	return flash_get_write_block_size(dev);
}

int flash_area_has_driver(const struct flash_area *fa)
{
	if (device_get_binding(fa->fa_dev_name) == NULL) {
		return -ENODEV;
	}

	return 1;
}

struct device *flash_area_get_device(const struct flash_area *fa)
{
	return device_get_binding(fa->fa_dev_name);
}
