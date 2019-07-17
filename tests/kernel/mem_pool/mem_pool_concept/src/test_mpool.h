/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TIMEOUT 2000
#ifdef CONFIG_RISCV
#define STACK_SIZE (1024 + CONFIG_TEST_EXTRA_STACKSIZE)
#else
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#endif
#define BLK_SIZE_MIN 16
#define BLK_SIZE_MAX 64
#define BLK_NUM_MIN 8
#define BLK_NUM_MAX 2
#define BLK_ALIGN BLK_SIZE_MIN
