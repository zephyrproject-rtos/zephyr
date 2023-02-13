/*  Bluetooth Audio Content Control */

/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ccid_internal.h"

uint8_t bt_ccid_get_value(void)
{
	static uint8_t ccid_value;

	/* By spec, the CCID can take all values up to and including 0xFF.
	 * But since this is a value we provide, we do not have to use all of
	 * them.  254 CCID values on a device should be plenty, the last one
	 * can be used to prevent wraparound.
	 */
	__ASSERT(ccid_value != UINT8_MAX,
		 "Cannot allocate any more control control IDs");

	return ccid_value++;
}
