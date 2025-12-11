/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/uuid.h>

/* Ad-hoc randomized UUID. The value is not significant. */
#define MTU_VALIDATION_SVC                                                                         \
	BT_UUID_DECLARE_128(0xdb, 0x1f, 0xe2, 0x52, 0xf3, 0xc6, 0x43, 0x66, 0xb3, 0x92, 0x5d,      \
			    0xc6, 0xe7, 0xc9, 0x59, 0x9d)

/* Ad-hoc randomized UUID. The value is not significant. */
#define MTU_VALIDATION_CHRC                                                                        \
	BT_UUID_DECLARE_128(0x3f, 0xa4, 0x7f, 0x44, 0x2e, 0x2a, 0x43, 0x05, 0xab, 0x38, 0x07,      \
			    0x8d, 0x16, 0xbf, 0x99, 0xf1)
