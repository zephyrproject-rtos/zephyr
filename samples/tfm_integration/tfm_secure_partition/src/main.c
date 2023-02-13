/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <tfm_ns_interface.h>

#include "dummy_partition.h"

void main(void)
{
	uint8_t digest[32];

	for (int key = 0; key < 6; key++) {
		psa_status_t status = dp_secret_digest(key, digest, sizeof(digest));

		if (status == PSA_ERROR_INVALID_ARGUMENT && key == 5) {
			printk("No valid secret for key, received expected error code\n");
		} else if (status != PSA_SUCCESS) {
			printk("Status: %d\n", status);
		} else {
			printk("Digest: ");
			for (int i = 0; i < 32; i++) {
				printk("%02x", digest[i]);
			}
			printk("\n");
		}
	}
}
