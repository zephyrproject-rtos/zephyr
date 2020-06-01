/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/entropy.h>

void sys_rand_access_grant(struct k_thread *thread)
{
	struct device *dev = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);

	__ASSERT((dev != NULL),
			"Device driver for %s (DT_CHOSEN_ZEPHYR_ENTROPY_LABEL) not found. "
			"Check your build configuration!",
			DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);

	k_object_access_grant(dev, thread);
}
