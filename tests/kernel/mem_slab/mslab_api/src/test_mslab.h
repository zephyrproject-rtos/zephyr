/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TEST_MSLAB_H__
#define __TEST_MSLAB_H__

#define TIMEOUT 2000
#define BLK_NUM 3
#define BLK_ALIGN 8
#define BLK_SIZE 16
#define STACKSIZE (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

extern void tmslab_alloc_free(void *data);

#endif /*__TEST_MSLAB_H__*/
