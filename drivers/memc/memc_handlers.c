/*
 * Copyright (c) 2026 YunHung Hua
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/memc.h>
#include <zephyr/internal/syscall_handler.h>

/*
 * memc_read() and memc_write() copy straight to or from the mapped base when
 * the device has one, so the range has to be checked against the device size
 * before a user thread gets there.
 */
static int memc_verify_range(const struct device *dev, uint32_t addr, size_t len)
{
	uint64_t size;
	int ret;

	ret = z_impl_memc_get_size(dev, &size);
	if (ret != 0) {
		/* Without a size the range cannot be validated */
		return ret;
	}

	if (len > size || addr > size - len) {
		return -EINVAL;
	}

	return 0;
}

static inline int z_vrfy_memc_read(const struct device *dev, uint32_t addr, uint8_t *data,
				   size_t len)
{
	int ret;

	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_MEMC));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(data, len));

	ret = memc_verify_range(dev, addr, len);
	if (ret != 0) {
		return ret;
	}

	return z_impl_memc_read(dev, addr, data, len);
}
#include <zephyr/syscalls/memc_read_mrsh.c>

static inline int z_vrfy_memc_write(const struct device *dev, uint32_t addr,
				    const uint8_t *data, size_t len)
{
	int ret;

	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_MEMC));
	K_OOPS(K_SYSCALL_MEMORY_READ(data, len));

	ret = memc_verify_range(dev, addr, len);
	if (ret != 0) {
		return ret;
	}

	return z_impl_memc_write(dev, addr, data, len);
}
#include <zephyr/syscalls/memc_write_mrsh.c>

static inline int z_vrfy_memc_get_size(const struct device *dev, uint64_t *size)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_MEMC));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(size, sizeof(*size)));

	return z_impl_memc_get_size(dev, size);
}
#include <zephyr/syscalls/memc_get_size_mrsh.c>

static inline int z_vrfy_memc_read_id(const struct device *dev, uint8_t *id, size_t len)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_MEMC));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(id, len));

	return z_impl_memc_read_id(dev, id, len);
}
#include <zephyr/syscalls/memc_read_id_mrsh.c>
