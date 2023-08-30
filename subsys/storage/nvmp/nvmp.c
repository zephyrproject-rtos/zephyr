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

enum nvmp_type nvmp_get_type(const struct nvmp_info *info)
{
	if (info == NULL) {
		return UNKNOWN;
	}

	return info->type;
}

ssize_t nvmp_get_store_start(const struct nvmp_info *info)
{
	if (info == NULL) {
		return -EINVAL;
	}

	return info->store_start;
}

const void *nvmp_get_store(const struct nvmp_info *info)
{
	if (info == NULL) {
		return NULL;
	}

	return info->store;
}

int nvmp_read(const struct nvmp_info *info, size_t start, void *data,
	      size_t len)
{
	if (info == NULL) {
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

	return info->write(info, start, data, len);
}

int nvmp_erase(const struct nvmp_info *info, size_t start, size_t len)
{
	if (info == NULL) {
		return -EINVAL;
	}

	return info->erase(info, start, len);
}
