/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

void z_impl_sys_rand_get(void *dst, size_t outlen)
{
	uint8_t *_dst = (uint8_t *)dst;
	int failure_counter = 0;

	while (outlen > 0) {
		unsigned long value;

		if (__builtin_aarch64_rndr(&value) != 0) {
			if (++failure_counter > CONFIG_ARM64_RANDOM_GENERATOR_MAX_RETRIES) {
				__ASSERT_PRINT("ARM64 RNDR keeps failing\n");
				k_panic();
			}
			k_msleep(CONFIG_ARM64_RANDOM_GENERATOR_RETRY_WAIT_MSEC);
		} else {
			size_t to_copy = MIN(outlen, sizeof(value));

			memcpy(_dst, &value, to_copy);
			_dst += to_copy;
			outlen -= to_copy;
			failure_counter = 0;
		}
	}
}
