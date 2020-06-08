/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/atomic.h>
#include <kernel.h>
#include <drivers/entropy.h>
#include <string.h>

static struct device *entropy_driver;

#if defined(CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR)
u32_t sys_rand32_get(void)
{
	struct device *dev = entropy_driver;
	u32_t random_num;
	int ret;

	if (unlikely(!dev)) {
		/* Only one entropy device exists, so this is safe even
		 * if the whole operation isn't atomic.
		 */
		dev = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
		__ASSERT((dev != NULL),
			"Device driver for %s (DT_CHOSEN_ZEPHYR_ENTROPY_LABEL) not found. "
			"Check your build configuration!",
			DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
		entropy_driver = dev;
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
#endif /* CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR */

static int rand_get(u8_t *dst, size_t outlen, bool csrand)
{
	struct device *dev = entropy_driver;
	u32_t random_num;
	int ret;

	if (unlikely(!dev)) {
		/* Only one entropy device exists, so this is safe even
		 * if the whole operation isn't atomic.
		 */
		dev = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
		__ASSERT((dev != NULL),
			"Device driver for %s (DT_CHOSEN_ZEPHYR_ENTROPY_LABEL) not found. "
			"Check your build configuration!",
			DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
		entropy_driver = dev;
	}

	ret = entropy_get_entropy(dev, dst, outlen);

	if (unlikely(ret < 0)) {
		/* Don't try to fill the buffer in case of
		 * cryptographically secure random numbers, just
		 * propagate the driver error.
		 */
		if (csrand) {
			return ret;
		}

		/* Use system timer in case the entropy device couldn't deliver
		 * 32-bit of data.  There's not much that can be done in this
		 * situation.  An __ASSERT() isn't used here as the HWRNG might
		 * still be gathering entropy during early boot situations.
		 */

		u32_t len = 0;
		u32_t blocksize = 4;

		while (len < outlen) {
			random_num = k_cycle_get_32();
			if ((outlen-len) < sizeof(random_num)) {
				blocksize = len;
				(void)memcpy(&(dst[random_num]),
						&random_num, blocksize);
			} else {
				*((u32_t *)&dst[len]) = random_num;
			}

			len += blocksize;
		}
	}

	return 0;
}

#if defined(CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR)
void sys_rand_get(void *dst, size_t outlen)
{
	rand_get(dst, outlen, false);
}
#endif /* CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR */

#if defined(CONFIG_HARDWARE_DEVICE_CS_GENERATOR)

int sys_csrand_get(void *dst, size_t outlen)
{
	if (rand_get(dst, outlen, true) != 0) {
		/* Is it the only error it should return ? entropy_sam
		 * can return -ETIMEDOUT for example
		 */
		return -EIO;
	}

	return 0;
}

#endif /* CONFIG_HARDWARE_DEVICE_CS_GENERATOR */
