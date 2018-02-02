/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SETTINGS_TEST_FCB_H
#define _SETTINGS_TEST_FCB_H

#include <stdio.h>
#include <string.h>
#include <ztest.h>
#include <fs.h>

#include "settings/settings.h"

#ifdef __cplusplus
#extern "C" {
#endif

#define TEST_FS_MPTR "/nffs"
#define TEST_CONFIG_DIR TEST_FS_MPTR"/config"

extern u8_t val8;
extern u32_t val32;
extern u64_t val64;

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
int settings_test_file_strstr(const char *fname, char *string);

#ifdef __cplusplus
}
#endif
#endif /* _SETTINGS_TEST_FCB_H */
