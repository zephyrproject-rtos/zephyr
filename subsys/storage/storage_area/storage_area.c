/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/storage/storage_area/storage_area.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(storage_area, CONFIG_STORAGE_AREA_LOG_LEVEL);

static bool sa_range_valid(const struct storage_area *area, sa_off_t offset,
			   size_t len)
{
	const size_t asize = area->erase_size * area->erase_blocks;

	if ((offset < 0) || (asize < len) || ((asize - len) < offset)) {
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

int storage_area_readv(const struct storage_area *area, sa_off_t offset,
		       const struct storage_area_iovec *iovec, size_t iovcnt)
{
	if ((area == NULL) || (area->api == NULL) ||
	    (area->api->readv == NULL)) {
		return -ENOTSUP;
	}

	const size_t len = sa_iovec_size(iovec, iovcnt);

	if (!sa_range_valid(area, offset, len)) {
		return -EINVAL;
	}

	return area->api->readv(area, offset, iovec, iovcnt);
}

int storage_area_read(const struct storage_area *area, sa_off_t offset,
		      void *data, size_t len)
{
	struct storage_area_iovec rd = {
		.data = data,
		.len = len,
	};

	return storage_area_readv(area, offset, &rd, 1U);
}

int storage_area_writev(const struct storage_area *area, sa_off_t offset,
			const struct storage_area_iovec *iovec, size_t iovcnt)
{
	if ((area == NULL) || (area->api == NULL) ||
	    (area->api->writev == NULL)) {
		return -ENOTSUP;
	}

	const size_t len = sa_iovec_size(iovec, iovcnt);

	if ((!sa_range_valid(area, offset, len)) ||
	    (((len & (area->write_size - 1)) != 0U))) {
		return -EINVAL;
	}

	return area->api->writev(area, offset, iovec, iovcnt);
}

int storage_area_write(const struct storage_area *area, sa_off_t offset,
		       const void *data, size_t len)
{
	struct storage_area_iovec wr = {
		.data = (void *)data,
		.len = len,
	};

	return storage_area_writev(area, offset, &wr, 1U);
}

int storage_area_erase(const struct storage_area *area, size_t sblk, size_t bcnt)
{
	if ((area == NULL) || (area->api == NULL) ||
	    (area->api->erase == NULL)) {
		return -ENOTSUP;
	}

	const size_t ablocks = area->erase_blocks;

	if ((ablocks < bcnt) || ((ablocks - bcnt) < sblk)) {
		LOG_DBG("invalid range");
		return -EINVAL;
	}

	return area->api->erase(area, sblk, bcnt);
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
