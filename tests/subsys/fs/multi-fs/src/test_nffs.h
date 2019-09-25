/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <fs/fs.h>
#include <nffs/nffs.h>

void test_nffs_mount(void);
void test_nffs_mkdir(void);
void test_nffs_readdir(void);
void test_nffs_unlink(void);
void test_nffs_open(void);
void test_nffs_write(void);
void test_nffs_read(void);
