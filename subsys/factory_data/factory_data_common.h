/*
 * Copyright (c) 2022 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FACTORY_DATA_COMMON_H_
#define FACTORY_DATA_COMMON_H_

#include <stdint.h>
#include <string.h>

/** Len of record without write-block-size alignment. Same logic as in settings_line_len_calc(). */
int factory_data_line_len_calc(const char *const name, const size_t val_len);

#endif /* FACTORY_DATA_COMMON_H_ */
