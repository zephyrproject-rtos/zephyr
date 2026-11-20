/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/energy_meter.h>
#include <zephyr/internal/syscall_handler.h>

/* Channels copied from user mode per driver call; covers one line of energy_meter_get_snapshot() */
#define CHANNEL_CHUNK ENERGY_METER_QUANTITY_COMMON_COUNT

static inline int z_vrfy_energy_meter_get_info(const struct device *dev,
					       struct energy_meter_info *info)
{
	struct energy_meter_info k_info;
	int ret;

	K_OOPS(K_SYSCALL_DRIVER_ENERGY_METER(dev, get_info));

	/* Clears the padding too, so no kernel stack data reaches user mode */
	(void)memset(&k_info, 0, sizeof(k_info));

	ret = z_impl_energy_meter_get_info(dev, &k_info);
	if (ret == 0) {
		K_OOPS(k_usermode_to_copy(info, &k_info, sizeof(k_info)));
	}

	return ret;
}
#include <zephyr/syscalls/energy_meter_get_info_mrsh.c>

static inline int z_vrfy_energy_meter_get(const struct device *dev,
					  const struct energy_meter_channel *channels,
					  int64_t *values, size_t count)
{
	struct energy_meter_channel k_channels[CHANNEL_CHUNK];
	int ret = 0;

	K_OOPS(K_SYSCALL_DRIVER_ENERGY_METER(dev, get));
	K_OOPS(K_SYSCALL_MEMORY_ARRAY_READ(channels, count, sizeof(*channels)));
	K_OOPS(K_SYSCALL_MEMORY_ARRAY_WRITE(values, count, sizeof(*values)));

	/*
	 * Drivers read each channel more than once (to validate it, then to read it), so they get
	 * a kernel copy that another user thread cannot change in between.
	 */
	for (size_t done = 0; (ret == 0) && (done < count); done += CHANNEL_CHUNK) {
		size_t n = MIN(count - done, CHANNEL_CHUNK);

		K_OOPS(k_usermode_from_copy(k_channels, &channels[done], n * sizeof(*channels)));
		ret = z_impl_energy_meter_get(dev, k_channels, &values[done], n);
	}

	return ret;
}
#include <zephyr/syscalls/energy_meter_get_mrsh.c>

static inline int z_vrfy_energy_meter_reset_energy(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_ENERGY_METER));

	return z_impl_energy_meter_reset_energy(dev);
}
#include <zephyr/syscalls/energy_meter_reset_energy_mrsh.c>
