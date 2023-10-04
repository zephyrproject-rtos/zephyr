/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/usdai.h>
#include <zephyr/drivers/usdai_ioctl.h>

static inline int z_vrfy_usdai_read(const struct device *dev, off_t offset,
				    void *data, size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_USDAI(dev, read));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(data, len));
	return z_impl_usdai_read((const struct device *)dev, offset, (void *)data, len);
}
#include <syscalls/usdai_read_mrsh.c>

static inline int z_vrfy_usdai_write(const struct device *dev, off_t offset,
				     const void *data, size_t len)
{
	Z_OOPS(Z_SYSCALL_DRIVER_USDAI(dev, write));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(data, len));
	return z_impl_usdai_write((const struct device *)dev, offset,
				  (const void *)data, len);
}
#include <syscalls/usdai_write_mrsh.c>

static inline int z_vrfy_usdai_sys_ioctl(const struct device *dev, uint32_t id, uintptr_t in,
					 uintptr_t in_out)
{
	Z_OOPS(Z_SYSCALL_DRIVER_USDAI(dev, sys_ioctl);

	/* There is no verification of other parameters here: it should happen
	 * inside driver.
	 * Reson for that is that inlining all param tests, for each call, that
	 * would cover any IOCTL ID would take a lot of code size.
	 */
	return z_impl_usdai_sys_ioctl(dev, id, in, in_out);
}

#include <syscalls/usdai_sys_ioctl_mrsh.c>
