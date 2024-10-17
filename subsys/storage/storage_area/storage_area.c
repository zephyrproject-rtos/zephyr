/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/storage/storage_area/storage_area.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage_area, CONFIG_STORAGE_AREA_LOG_LEVEL);

static bool sa_range_valid(const struct storage_area *area, size_t start,
			   size_t len)
{
	const size_t asize = area->erase_size * area->erase_blocks;

	if ((asize < len) || ((asize - len) < start)) {
		LOG_DBG("Invalid range");
		return false;
	}

	return true;
}

static size_t sa_iovec_size(const struct storage_area_iovec *iovec,
			    size_t iovcnt)
{
	size_t rv = 0U;

	for (size_t i = 0U; i < iovcnt; i++) {
		rv += iovec[i].len;
	}

	return rv;
}

int storage_area_readv(const struct storage_area *area, size_t start,
		       const struct storage_area_iovec *iovec, size_t iovcnt)
{
	if ((area == NULL) || (area->api == NULL) || (area->api->readv == NULL)) {
		return -ENOTSUP;
	}

	const size_t len = sa_iovec_size(iovec, iovcnt);

	if (!sa_range_valid(area, start, len)) {
		return -EINVAL;
	}

	return area->api->readv(area, start, iovec, iovcnt);
}

int storage_area_read(const struct storage_area *area, size_t start, void *data,
		      size_t len)
{
	struct storage_area_iovec rd = {
		.data = data,
		.len = len,
	};

	return storage_area_readv(area, start, &rd, 1U);
}

int storage_area_writev(const struct storage_area *area, size_t start,
			const struct storage_area_iovec *iovec, size_t iovcnt)
{
	if ((area == NULL) || (area->api == NULL) || (area->api->writev == NULL)) {
		return -ENOTSUP;
	}

	const size_t len = sa_iovec_size(iovec, iovcnt);

	if ((!sa_range_valid(area, start, len)) ||
	    (((len & (area->write_size - 1)) != 0U))) {
		return -EINVAL;
	}

	if (STORAGE_AREA_HAS_PROPERTY(area, SA_PROP_READONLY)) {
		LOG_DBG("prog not supported (read-only)");
		return -EROFS;
	}

	return area->api->writev(area, start, iovec, iovcnt);
}

int storage_area_write(const struct storage_area *area, size_t start,
		       const void *data, size_t len)
{
	struct storage_area_iovec wr = {
		.data = (void *)data,
		.len = len,
	};

	return storage_area_writev(area, start, &wr, 1U);
}

int storage_area_erase(const struct storage_area *area, size_t start,
		       size_t bcnt)
{
	if ((area == NULL) || (area->api == NULL) ||
	    (area->api->erase == NULL)) {
		return -ENOTSUP;
	}

	const size_t ablocks = area->erase_blocks;

	if ((ablocks < bcnt) || ((ablocks - bcnt) < start)) {
		LOG_DBG("invalid range");
		return -EINVAL;
	}

	if (STORAGE_AREA_HAS_PROPERTY(area, SA_PROP_READONLY)) {
		LOG_DBG("erase not supported (read-only)");
		return -EROFS;
	}

	return area->api->erase(area, start, bcnt);
}

int storage_area_ioctl(const struct storage_area *area,
		       enum storage_area_ioctl_cmd cmd, void *data)
{
	if ((area == NULL) || (area->api == NULL) ||
	    (area->api->ioctl == NULL)) {
		return -ENOTSUP;
	}

	return area->api->ioctl(area, cmd, data);
}
