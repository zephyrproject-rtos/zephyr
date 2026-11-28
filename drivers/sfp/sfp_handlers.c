/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sfp.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_sfp_read_id(const struct device *dev, struct sff8472_id *id)
{
	K_OOPS(K_SYSCALL_DRIVER_SFP(dev, read_id));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(id, sizeof(*id)));

	return z_impl_sfp_read_id(dev, id);
}
#include <zephyr/syscalls/sfp_read_id_mrsh.c>

static inline int z_vrfy_sfp_read_diagnostics(const struct device *dev,
					      struct sff8472_diagnostics *diag)
{
	K_OOPS(K_SYSCALL_DRIVER_SFP(dev, read_diagnostics));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(diag, sizeof(*diag)));

	return z_impl_sfp_read_diagnostics(dev, diag);
}
#include <zephyr/syscalls/sfp_read_diagnostics_mrsh.c>
