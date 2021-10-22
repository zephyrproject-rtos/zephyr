/*  Bluetooth Audio Content Control */

/*
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ccid_internal.h"


uint8_t ccid_get_value(void)
{
	static uint8_t ccid_value;

	__ASSERT(ccid_value != 0xFF,
		 "Cannot allocate any more control control IDs");
	return ccid_value++;
}
