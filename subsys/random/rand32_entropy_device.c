/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/atomic.h>
#include <kernel.h>
#include <drivers/entropy.h>
#include <string.h>

static const struct device *entropy_driver;

#if defined(CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR)
uint32_t z_impl_sys_rand32_get(void)
{
	const struct device *dev = entropy_driver;
	uint32_t random_num;
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

	ret = entropy_get_entropy(dev, (uint8_t *)&random_num,
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

static int rand_get(uint8_t *dst, size_t outlen, bool csrand)
{
	const struct device *dev = entropy_driver;
	uint32_t random_num;
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

		uint32_t len = 0;
		uint32_t blocksize = 4;

		while (len < outlen) {
			size_t copylen = outlen - len;

			if (copylen > blocksize) {
				copylen = blocksize;
			}

			random_num = k_cycle_get_32();
			(void)memcpy(&(dst[len]), &random_num, copylen);
			len += copylen;
		}
	}

	return 0;
}

#if defined(CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR)
void z_impl_sys_rand_get(void *dst, size_t outlen)
{
	rand_get(dst, outlen, false);
}
#endif /* CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR */

#if defined(CONFIG_HARDWARE_DEVICE_CS_GENERATOR)

int z_impl_sys_csrand_get(void *dst, size_t outlen)
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
