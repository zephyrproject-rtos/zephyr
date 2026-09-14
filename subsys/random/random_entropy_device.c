/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/entropy.h>
#include <string.h>

static int rand_get(uint8_t *dst, size_t outlen, bool csrand)
{
	uint32_t random_num;
	int ret = -ENODEV;
	const struct device *const entropy_dev = entropy_get_default_device();

	if (device_is_ready(entropy_dev)) {
		size_t len = 0;

		/* entropy_get_entropy() takes a uint16_t length, so split
		 * larger requests instead of silently truncating them.
		 */
		ret = 0;
		while ((len < outlen) && (ret == 0)) {
			uint16_t chunk = (uint16_t)MIN(outlen - len, UINT16_MAX);

			ret = entropy_get_entropy(entropy_dev, &dst[len], chunk);
			len += chunk;
		}
	}

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

		size_t len = 0;

		while (len < outlen) {
			size_t copylen = MIN(outlen - len, sizeof(random_num));

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
