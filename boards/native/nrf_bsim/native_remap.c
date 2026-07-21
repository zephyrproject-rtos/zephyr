/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include "NHW_misc.h"

bool native_emb_addr_remap(void **addr)
{
	return nhw_convert_RAM_addr(addr);
}
