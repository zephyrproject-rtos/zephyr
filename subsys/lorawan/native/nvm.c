/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/byteorder.h>

#include "nvm.h"
#include "lorawan_nvm.h"

#define DEV_NONCE_KEY       "lorawan/native/dev_nonce"
#define DEV_NONCE_EXHAUSTED (UINT16_MAX + 1U)

/* One device-wide counter, including 65536 as a persistent exhaustion marker. */
static uint32_t next_nonce;
static bool ready;

int lorawan_nvm_restore(void)
{
	uint8_t value[sizeof(uint32_t)];
	int ret;

	ready = false;
	ret = lorawan_nvm_read(DEV_NONCE_KEY, value, sizeof(value));
	if (ret != 0 && ret != -ENOENT) {
		return ret;
	}
	next_nonce = ret == 0 ? sys_get_le32(value) : 0;
	if (next_nonce > DEV_NONCE_EXHAUSTED) {
		return -EINVAL;
	}
	ready = true;
	return 0;
}

int lwan_nvm_dev_nonce_reserve(uint16_t *nonce)
{
	uint8_t value[sizeof(uint32_t)];
	int ret;

	if (nonce == NULL) {
		return -EINVAL;
	}
	if (!ready) {
		return -EACCES;
	}
	if (next_nonce == DEV_NONCE_EXHAUSTED) {
		return -EOVERFLOW;
	}
	sys_put_le32(next_nonce + 1U, value);
	ret = lorawan_nvm_write(DEV_NONCE_KEY, value, sizeof(value));
	if (ret != 0) {
		/* The write outcome may be uncertain. Require a reload before any retry. */
		ready = false;
		return ret;
	}
	*nonce = (uint16_t)next_nonce++;
	return 0;
}
