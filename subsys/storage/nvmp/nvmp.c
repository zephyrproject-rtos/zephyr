/*
 * Copyright (c) 2023, Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/storage/nvmp.h>

size_t nvmp_get_size(const struct nvmp_info *info)
{
	if (info == NULL) {
		return 0U;
	}

	return info->size;
}

size_t nvmp_get_block_size(const struct nvmp_info *info)
{
	if (info == NULL) {
		return 0U;
	}

	return info->block_size;
}

size_t nvmp_get_write_block_size(const struct nvmp_info *info)
{
	if (info == NULL) {
		return 0U;
	}

	return info->write_block_size;
}

int nvmp_open(const struct nvmp_info *info)
{
	if (info == NULL) {
		return -EINVAL;
	}

	if (info->open == NULL) {
		return -ENOTSUP;
	}

	return info->open(info);
}

int nvmp_read(const struct nvmp_info *info, size_t start, void *data, size_t len)
{
	if (info == NULL) {
		return -EINVAL;
	}

	if (info->read == NULL) {
		return -ENOTSUP;
	}

	if ((start + len) > info->size) {
		return -EINVAL;
	}

	return info->read(info, start, data, len);
}

int nvmp_write(const struct nvmp_info *info, size_t start, const void *data,
	       size_t len)
{
	if (info == NULL) {
		return -EINVAL;
	}

	if (info->write == NULL) {
		return -ENOTSUP;
	}

	if ((start + len) > info->size) {
		return -EINVAL;
	}

	return info->write(info, start, data, len);
}

int nvmp_erase(const struct nvmp_info *info, size_t start, size_t len)
{
	if (info == NULL) {
		return -EINVAL;
	}

	if (info->erase == NULL) {
		return -ENOTSUP;
	}

	if ((start + len) > info->size) {
		return -EINVAL;
	}

	return info->erase(info, start, len);
}

int nvmp_clear(const struct nvmp_info *info, void *data, size_t len)
{
	if (info == NULL) {
		return -EINVAL;
	}

	if (info->clear == NULL) {
		return -ENOTSUP;
	}

	return info->clear(info, data, len);
}

int nvmp_close(const struct nvmp_info *info)
{
	if (info == NULL) {
		return -EINVAL;
	}

	if (info->close == NULL) {
		return -ENOTSUP;
	}

	return info->close(info);
}
