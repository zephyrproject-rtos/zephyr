/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DOUBLE_BUFFER_SIZE
#define DOUBLE_BUFFER_SIZE 2
#endif

#ifndef TRIPLE_BUFFER_SIZE
#define TRIPLE_BUFFER_SIZE 3
#endif

#include <stddef.h>

u8_t util_ones_count_get(u8_t *octets, u8_t octets_len);

int util_rand(void *buf, size_t len);
