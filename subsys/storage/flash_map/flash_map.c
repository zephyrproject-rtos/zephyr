/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 * Copyright (c) 2017 Linaro Ltd
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/check.h>
#include <device.h>
#include <storage/flash_map.h>
#include <drivers/flash.h>
#include <soc.h>
#include <init.h>

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY)
#include <tinycrypt/constants.h>
#include <tinycrypt/sha256.h>
#include <string.h>
#endif

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
struct layout_data {
	const struct flash_area *fa;
	void *ret;        /* struct flash_area* or struct flash_sector* */
	uint32_t ret_idx;
	uint32_t ret_len;
	int status;
};
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

void flash_area_foreach(flash_area_cb_t user_cb, void *user_data)
{
	extern const struct flash_area __flash_map_list_start[];
	extern const struct flash_area __flash_map_list_end[];
	const struct flash_area *iter = __flash_map_list_start;

	while (iter < __flash_map_list_end) {
		user_cb(iter, user_data);
		++iter;
	}
}

int flash_area_open(uint8_t id, const struct flash_area **fap)
{
	extern const struct flash_area __flash_map_list_start[];
	extern const struct flash_area __flash_map_list_end[];

	if ((uint8_t)(__flash_map_list_end - __flash_map_list_start) == 0) {
		return -EACCES;
	} else if (id >= (uint8_t)(__flash_map_list_end - __flash_map_list_start)) {
		fap = NULL;
		return -ENOENT;
	} else {
		*fap = &__flash_map_list_start[id];
	}

	return 0;
}

static inline bool is_in_flash_area_bounds(const struct flash_area *fa,
					   off_t off, size_t len)
{
	return (off >= 0) && ((off + len) <= fa->fa_size);
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
	if (info->start_offset < data->fa->fa_off) {
		*bail_value = true;
		return true;
	} else if (info->start_offset >= data->fa->fa_off + data->fa->fa_size) {
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
static int flash_area_layout(const struct flash_area *fa, uint32_t *cnt, void *ret,
flash_page_cb cb, struct layout_data *cb_data)
{
	if (fa == NULL) {
		return -EINVAL;
	}

	cb_data->fa = fa;
	cb_data->ret = ret;
	cb_data->ret_idx = 0U;
	cb_data->ret_len = *cnt;
	cb_data->status = 0;

	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

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

	ret[data->ret_idx].fs_off = info->start_offset - data->fa->fa_off;
	ret[data->ret_idx].fs_size = info->size;
	data->ret_idx++;

	return true;
}

int flash_area_get_sectors(int id, uint32_t *cnt, struct flash_sector *ret)
{
	struct layout_data data;
	const struct flash_area *fa;

	int rc = flash_area_open(id, &fa);

	if (rc != 0) {
		return rc;
	}

	return flash_area_layout(fa, cnt, ret, get_sectors_cb, &data);
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

int flash_area_read(const struct flash_area *fa, off_t off, void *dst,
		    size_t len)
{
	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	if (!is_in_flash_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	if (!device_is_ready(fa->fa_dev)) {
		return -EIO;
	}

	return flash_read(fa->fa_dev, fa->fa_off + off, dst, len);
}

int flash_area_write(const struct flash_area *fa, off_t off, const void *src,
		     size_t len)
{
	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	if (!is_in_flash_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	if (!device_is_ready(fa->fa_dev)) {
		return -EIO;
	}

	return flash_write(fa->fa_dev, fa->fa_off + off, (void *)src, len);

}

int flash_area_erase(const struct flash_area *fa, off_t off, size_t len)
{
	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	if (!is_in_flash_area_bounds(fa, off, len)) {
		return -EINVAL;
	}

	if (!device_is_ready(fa->fa_dev)) {
		return -EIO;
	}

	return flash_erase(fa->fa_dev, fa->fa_off + off, len);
}

uint8_t flash_area_align(const struct flash_area *fa)
{
	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	return flash_get_write_block_size(fa->fa_dev);
}

int flash_area_has_driver(const struct flash_area *fa)
{
	return fa->fa_dev == NULL ? -ENODEV : 0;
}

const struct device *flash_area_get_device(const struct flash_area *fa)
{
	return fa->fa_dev;
}

uint8_t flash_area_erased_val(const struct flash_area *fa)
{
	const struct flash_parameters *param;

	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	param = flash_get_parameters(fa->fa_dev);

	return param->erase_value;
}

#if defined(CONFIG_FLASH_AREA_CHECK_INTEGRITY)
int flash_area_check_int_sha256(const struct flash_area *fa,
				const struct flash_area_check *fac)
{
	unsigned char hash[TC_SHA256_DIGEST_SIZE];
	struct tc_sha256_state_struct sha;
	int to_read;
	int pos;
	int rc;

	if (fa == NULL || fac == NULL || fac->match == NULL ||
	    fac->rbuf == NULL || fac->clen == 0 || fac->rblen == 0) {
		return -EINVAL;
	}

	if (!is_in_flash_area_bounds(fa, fac->off, fac->clen)) {
		return -EINVAL;
	}

	if (tc_sha256_init(&sha) != TC_CRYPTO_SUCCESS) {
		return -ESRCH;
	}

	CHECKIF(fa->fa_dev == NULL) {
		return -ENODEV;
	}

	to_read = fac->rblen;

	for (pos = 0; pos < fac->clen; pos += to_read) {
		if (pos + to_read > fac->clen) {
			to_read = fac->clen - pos;
		}

		rc = flash_read(fa->fa_dev, (fa->fa_off + fac->off + pos),
				fac->rbuf, to_read);
		if (rc != 0) {
			return rc;
		}

		if (tc_sha256_update(&sha,
				     fac->rbuf,
				     to_read) != TC_CRYPTO_SUCCESS) {
			return -ESRCH;
		}
	}

	if (tc_sha256_final(hash, &sha) != TC_CRYPTO_SUCCESS) {
		return -ESRCH;
	}

	if (memcmp(hash, fac->match, TC_SHA256_DIGEST_SIZE)) {
		return -EILSEQ;
	}

	return 0;
}
#endif /* CONFIG_FLASH_AREA_CHECK_INTEGRITY */
