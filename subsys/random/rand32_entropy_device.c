/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <atomic.h>
#include <kernel.h>
#include <entropy.h>

static atomic_t entropy_driver;

u32_t sys_rand32_get(void)
{
	struct device *dev = (struct device *)atomic_get(&entropy_driver);
	u32_t random_num;
	int ret;

	if (unlikely(!dev)) {
		/* Only one entropy device exists, so this is safe even
		 * if the whole operation isn't atomic.
		 */
		dev = device_get_binding(CONFIG_ENTROPY_NAME);
		__ASSERT((dev != NULL),
			"Device driver for %s (CONFIG_ENTROPY_NAME) not found. "
			"Check your build configuration!",
			CONFIG_ENTROPY_NAME);
		atomic_set(&entropy_driver, (atomic_t)(uintptr_t)dev);
	}

	ret = entropy_get_entropy(dev, (u8_t *)&random_num,
				  sizeof(random_num));
	if (unlikely(ret < 0)) {
		/* Use system timer in case the entropy device couldn't deliver
		 * 32-bit of data.  There's not much that can be done in this
		 * situation.  An __ASSERT() isn't used here as the HWRNG might
		 * still be gathering entropy during early boot situations.
		 */

		random_num = k_cycle_get_32();
	}

	return random_num;
}
