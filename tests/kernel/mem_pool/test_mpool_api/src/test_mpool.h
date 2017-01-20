/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_MPOOL_H__
#define __TEST_MPOOL_H__

#define TIMEOUT 100
#define BLK_SIZE_MIN 4
#define BLK_SIZE_MAX 64
#define BLK_NUM_MIN 32
#define BLK_NUM_MAX 2
#define BLK_ALIGN BLK_SIZE_MIN

extern void tmpool_alloc_free(void *data);

#endif /*__TEST_MPOOL_H__*/
