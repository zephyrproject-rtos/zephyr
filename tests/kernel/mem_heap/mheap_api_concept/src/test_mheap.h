/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TIMEOUT 2000
#define BLK_SIZE_MIN 64
#define BLK_NUM_MAX (CONFIG_HEAP_MEM_POOL_SIZE / BLK_SIZE_MIN)
#define BLK_NUM_MIN (CONFIG_HEAP_MEM_POOL_SIZE / (BLK_SIZE_MIN << 2))
#define BLK_SIZE_EXCLUDE_DESC (BLK_SIZE_MIN - sizeof(void *))
#define BLK_SIZE_QUARTER (CONFIG_HEAP_MEM_POOL_SIZE >> 2)
