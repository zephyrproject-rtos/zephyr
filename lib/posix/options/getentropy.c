/*
 * Copyright (c) 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/entropy.h>
#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>

#define ENTROPY_NODE DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_entropy))

int getentropy(void *buffer, size_t length)
{
	const struct device *const entropy = ENTROPY_NODE;

	if (!buffer) {
		errno = EFAULT;
		return -1;
	}

	if (length > 256) {
		errno = EIO;
		return -1;
	}

	if (entropy == NULL || !device_is_ready(entropy)) {
		errno = EIO;
		return -1;
	}

	/*
	 * getentropy() uses size_t to represent buffer size, but Zephyr uses
	 * uint16_t. The length check above allows us to safely cast without
	 * overflow.
	 */
	if (entropy_get_entropy(entropy, buffer, (uint16_t)length)) {
		errno = EIO;
		return -1;
	}

	return 0;
}
