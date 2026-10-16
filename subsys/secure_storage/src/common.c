/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/secure_storage/common.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(secure_storage, CONFIG_SECURE_STORAGE_LOG_LEVEL);

#if !defined(secure_storage_zeroize)

void secure_storage_zeroize(void *buf, size_t len)
{
	volatile uint8_t *p = buf;

	while (len-- > 0) {
		*p++ = 0;
	}
}
#endif
