/*
 * Copyright (c) 2017 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PROT_TEST_TARGETS_H_
#define _PROT_TEST_TARGETS_H_

#define RODATA_VALUE  0xF00FF00F
extern const u32_t rodata_var;

#define BUF_SIZE 16
extern u8_t data_buf[BUF_SIZE];

extern int overwrite_target(int i);

#endif
