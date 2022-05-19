/*
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "factory_data_common.h"

#include <stdint.h>
#include <string.h>

/** Len of record without write-block-size alignment. Same logic as in settings_line_len_calc(). */
int factory_data_line_len_calc(const char *const name, const size_t val_len)
{
	/* <name>\0<value> */
	return strlen(name) + 1 + val_len;
}
