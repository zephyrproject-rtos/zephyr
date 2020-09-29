/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_MPOOL_H__
#define __TEST_MPOOL_H__

#define TIMEOUT_MS 100
#define TIMEOUT K_MSEC(TIMEOUT_MS)
#define BLK_SIZE_MIN 16
#define BLK_SIZE_MAX 256
#define BLK_NUM_MIN 32
#define BLK_NUM_MAX 2
#define BLK_ALIGN BLK_SIZE_MIN

extern void tmpool_alloc_free(const void *data);

#endif /*__TEST_MPOOL_H__*/
