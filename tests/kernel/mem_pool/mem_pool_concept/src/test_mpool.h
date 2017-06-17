/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define TIMEOUT 2000
#ifdef CONFIG_RISCV32
#define STACK_SIZE 1024
#else
#define STACK_SIZE 512
#endif
#define BLK_SIZE_MIN 8
#define BLK_SIZE_MAX 32
#define BLK_NUM_MIN 8
#define BLK_NUM_MAX 2
#define BLK_ALIGN BLK_SIZE_MIN
