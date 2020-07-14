/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SETTINGS_TEST_FS_H
#define _SETTINGS_TEST_FS_H

#include <stdio.h>
#include <string.h>
#include <ztest.h>
#include <fs/fs.h>

#include "settings/settings.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t val8;
extern uint16_t val16;
extern uint32_t val32;
extern uint64_t val64;

extern int test_get_called;
extern int test_set_called;
extern int test_commit_called;
extern int test_export_block;

extern int c2_var_count;

extern struct settings_handler c_test_handlers[];

void ctest_clear_call_state(void);
int ctest_get_call_state(void);

void config_wipe_srcs(void);

int fsutil_read_file(const char *path, off_t offset, size_t len, void *dst,
		     size_t *out_len);
int fsutil_write_file(const char *path, const void *data, size_t len);
int settings_test_file_strstr(const char *fname, char const *string,
			      size_t str_len);


void config_empty_lookups(void);
void test_config_insert(void);
void test_config_getset_unknown(void);
void test_config_getset_int(void);
void test_config_getset_int64(void);
void test_config_commit(void);

void test_config_empty_file(void);
void test_config_small_file(void);
void test_config_multiple_in_file(void);
void test_config_save_in_file(void);
void test_config_save_one_file(void);
void test_config_compress_file(void);

#ifdef __cplusplus
}
#endif

#endif /* _SETTINGS_TEST_FS_H */
