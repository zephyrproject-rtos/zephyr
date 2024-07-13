/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* FIXME: use k_off_t instead of off_t */
#include <sys/types.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>

LOG_MODULE_REGISTER(flash, CONFIG_FLASH_LOG_LEVEL);

int z_impl_flash_fill(const struct device *dev, uint8_t val, off_t offset,
		      size_t size)
{
	uint8_t filler[CONFIG_FLASH_FILL_BUFFER_SIZE];
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;
	const struct flash_parameters *fparams = api->get_parameters(dev);
	int rc = 0;
	size_t stored = 0;

	if (sizeof(filler) < fparams->write_block_size) {
		LOG_ERR("Size of CONFIG_FLASH_FILL_BUFFER_SIZE");
		return -EINVAL;
	}
	/* The flash_write will, probably, check write alignment but this
	 * is too late, as we write datain chunks; data alignment may be
	 * broken by the size of the last chunk, that is why the check
	 * happens here too.
	 * Note that we have no way to check whether offset and size are
	 * are correct, as such info is only available at the level of
	 * a driver, so only basic check on offset.
	 */
	if (offset < 0) {
		LOG_ERR("Negative offset not allowed\n");
		return -EINVAL;
	}
	if ((size | (size_t)offset) & (fparams->write_block_size - 1)) {
		LOG_ERR("Incorrect size or offset alignment, expected %zx\n",
			fparams->write_block_size);
		return -EINVAL;
	}

	memset(filler, val, sizeof(filler));

	while (stored < size) {
		size_t chunk = MIN(sizeof(filler), size - stored);

		rc = api->write(dev, offset + stored, filler, chunk);
		if (rc < 0) {
			LOG_DBG("Fill to dev %p failed at offset 0x%zx\n",
				dev, (size_t)offset + stored);
			break;
		}
		stored += chunk;
	}
	return rc;
}

int z_impl_flash_flatten(const struct device *dev, off_t offset, size_t size)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;
	__maybe_unused const struct flash_parameters *params = api->get_parameters(dev);

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
	if ((flash_params_get_erase_cap(params) & FLASH_ERASE_C_EXPLICIT) &&
		api->erase != NULL) {
		return api->erase(dev, offset, size);
	}
#endif

#if defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
	return flash_fill(dev, params->erase_value, offset, size);
#else
	return -ENOSYS;
#endif
}

/* note: caller must first check for positivity (>=0) */
static inline bool off_add_overflow(off_t offset, off_t size, off_t *result)
{
	BUILD_ASSERT((sizeof(off_t) == sizeof(uint32_t)) || (sizeof(off_t) == sizeof(uint64_t)));

	if (sizeof(off_t) == sizeof(uint32_t)) {
		uint32_t end;

		/* account for signedness of off_t due to lack of s32_add_overflow() */
		if (u32_add_overflow((uint32_t)offset, (uint32_t)size, &end) || (end > INT32_MAX)) {
			return true;
		}
	} else if (sizeof(off_t) == sizeof(uint64_t)) {
		uint64_t end;

		/* account for signedness of off_t due to lack of s64_add_overflow() */
		if (u64_add_overflow((uint64_t)offset, (uint64_t)size, &end) || (end > INT64_MAX)) {
			return true;
		}
	}

	return false;
}

/* note: caller must first check for overflow */
static inline bool flash_ranges_overlap(off_t a_start, off_t a_size, off_t b_start, off_t b_size)
{
	off_t a_end = a_start + a_size;
	off_t b_end = b_start + b_size;

	return (a_start < b_end) && (a_end > b_start);
}

int z_impl_flash_copy(const struct device *src_dev, off_t src_offset, const struct device *dst_dev,
		      off_t dst_offset, off_t size, uint8_t *buf, size_t buf_size)
{
	int ret;
	off_t end;
	size_t write_size;

	if ((src_offset < 0) || (dst_offset < 0) || (size < 0) || (buf == NULL) ||
	    (buf_size == 0) || off_add_overflow(src_offset, size, &end) ||
	    off_add_overflow(dst_offset, size, &end)) {
		LOG_DBG("invalid argument");
		return -EINVAL;
	}

	if (src_dev == dst_dev) {
		if (src_offset == dst_offset) {
			return 0;
		}

		if (flash_ranges_overlap(src_offset, size, dst_offset, size) != 0) {
			return -EINVAL;
		}
	}

	if (!device_is_ready(src_dev)) {
		LOG_DBG("%s device not ready", "src");
		return -ENODEV;
	}

	if (!device_is_ready(dst_dev)) {
		LOG_DBG("%s device not ready", "dst");
		return -ENODEV;
	}

	write_size = flash_get_write_block_size(dst_dev);
	if ((buf_size < write_size) || ((buf_size % write_size) != 0)) {
		LOG_DBG("buf size %zu is incompatible with write_size of %zu", buf_size,
			write_size);
		return -EINVAL;
	}

	for (uint32_t offs = 0, N = size, bytes_read = 0, bytes_left = N; offs < N;
	     offs += bytes_read, bytes_left -= bytes_read) {

		if (bytes_left < write_size) {
			const struct flash_driver_api *api =
				(const struct flash_driver_api *)dst_dev->api;
			const struct flash_parameters *params = api->get_parameters(dst_dev);

			memset(buf, params->erase_value, write_size);
		}
		bytes_read = MIN(MAX(bytes_left, write_size), buf_size);
		ret = flash_read(src_dev, src_offset + offs, buf, bytes_read);
		if (ret < 0) {
			LOG_DBG("%s() failed at offset %lx: %d", "flash_read",
				(long)(src_offset + offs), ret);
			return ret;
		}

		ret = flash_write(dst_dev, dst_offset + offs, buf, bytes_read);
		if (ret < 0) {
			LOG_DBG("%s() failed at offset %lx: %d", "flash_write",
				(long)(src_offset + offs), ret);
			return ret;
		}
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_flash_copy(const struct device *src_dev, off_t src_offset, const struct device *dst_dev,
		      off_t dst_offset, off_t size, uint8_t *buf, size_t buf_size)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(buf, buf_size));
	return z_impl_flash_copy(src_dev, src_offset, dst_dev, dst_offset, size, buf, buf_size);
}
#include <zephyr/syscalls/flash_copy_mrsh.c>
#endif
