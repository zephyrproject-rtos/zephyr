/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
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
#include "flash_map_priv.h"

struct layout_data {
	uint32_t area_idx;
	uint32_t area_off;
	uint32_t area_len;
	void *ret;        /* struct flash_area* or struct flash_sector* */
	uint32_t ret_idx;
	uint32_t ret_len;
	int status;
};

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
static int flash_area_layout(int idx, uint32_t *cnt, void *ret, flash_page_cb cb,
	struct layout_data *cb_data)
{
	int rc;
	const struct flash_area *fa;

	cb_data->area_idx = idx;

	rc = flash_area_open(idx, &fa);
	if (rc != 0) {
		return rc;
	}

	cb_data->area_off = fa->fa_off;
	cb_data->area_len = fa->fa_size;

	cb_data->ret = ret;
	cb_data->ret_idx = 0U;
	cb_data->ret_len = *cnt;
	cb_data->status = 0;

	flash_page_foreach(fa->fa_dev, cb, cb_data);

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

int flash_area_get_sectors(int idx, uint32_t *cnt, struct flash_sector *ret)
{
	struct layout_data data;

	return flash_area_layout(idx, cnt, ret, get_sectors_cb, &data);
}

int flash_area_get_page_info_by_idx(const struct flash_area *fa, off_t idx,
	struct flash_pages_info *pi)
{
	struct flash_pages_info lpi;
	int ret;

	if (idx < 0) {
		return -EINVAL;
	}

	/* Get information for the base, which is the first page of flash area */
	ret = flash_get_page_info_by_offs(fa->fa_dev, fa->fa_off, &lpi);
	if (ret < 0) {
		return ret;
	}

	ret = flash_get_page_info_by_idx(fa->fa_dev, lpi.index + idx, &lpi);

	if (ret < 0) {
		return ret;
	} else if (lpi.start_offset >= (fa->fa_off + fa->fa_size)) {
		return -EINVAL;
	}

	pi->index = idx;
	pi->start_offset = lpi.start_offset - fa->fa_off;
	pi->size = lpi.size;

	return 0;
}

int flash_area_get_page_info_by_offs(const struct flash_area *fa, off_t offset,
	struct flash_pages_info *pi)
{
	struct flash_pages_info lpi;
	int ret;
	uint32_t base_index;

	if (offset < 0 || offset >= fa->fa_size) {
		return -EINVAL;
	}

	/* Get information for the base, which is the first page of flash area */
	ret = flash_get_page_info_by_offs(fa->fa_dev, fa->fa_off, &lpi);
	if (ret < 0) {
		return ret;
	}

	base_index = lpi.index;

	if ((lpi.start_offset - fa->fa_off) >= fa->fa_size) {
		return -EINVAL;
	}

	ret = flash_get_page_info_by_offs(fa->fa_dev, fa->fa_off + offset, &lpi);

	if (ret < 0) {
		return ret;
	}

	pi->index = lpi.index - base_index;
	pi->start_offset = lpi.start_offset - fa->fa_off;
	pi->size = lpi.size;

	return 0;
}

ssize_t flash_area_get_page_count(const struct flash_area *fa)
{
	struct flash_pages_info lpi;
	off_t start;
	int ret;

	/* Get information for the base, which is the first page of flash area */
	ret = flash_get_page_info_by_offs(fa->fa_dev, fa->fa_off, &lpi);
	if (ret != 0) {
		return ret;
	}

	start = lpi.index;
	ret = flash_get_page_info_by_offs(fa->fa_dev, fa->fa_off + fa->fa_size - 1, &lpi);

	return (ret < 0) ? ret : (lpi.index - start + 1);
}

int flash_area_get_next_page_info(const struct flash_area *fa, struct flash_pages_info *pi,
	bool wrap)
{
	struct flash_pages_info lpi;
	int ret;

	ret = flash_area_get_page_info_by_idx(fa, pi->index + 1, &lpi);
	if ((ret == -EINVAL) && (wrap == true)) {
		ret = flash_area_get_page_info_by_idx(fa, 0, &lpi);
	}

	if (ret >= 0) {
		*pi = lpi;
	}
	return ret;
}
