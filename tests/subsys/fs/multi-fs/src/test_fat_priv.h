/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define FATFS_MNTP		"/RAM:"
#define TEST_FILE		"testfile.txt"
#define TEST_FILE_PATH		FATFS_MNTP"/"TEST_FILE
#define TEST_DIR_PATH		FATFS_MNTP"/testdir"
#define TEST_DIR_FILE_PATH	TEST_DIR"/testfile.txt"
