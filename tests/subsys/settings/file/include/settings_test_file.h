/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SETTINGS_TEST_FILE_H
#define _SETTINGS_TEST_FILE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>
#include <zephyr/types.h>

#include <zephyr/settings/settings.h>

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

extern struct settings_handler c_test_handlers[];

void ctest_clear_call_state(void);
int ctest_get_call_state(void);

void config_wipe_srcs(void);

int fsutil_read_file(const char *path, k_off_t offset, size_t len, void *dst, size_t *out_len);
int fsutil_write_file(const char *path, const void *data, size_t len);
int settings_test_file_strstr(const char *fname, char const *string,
			      size_t str_len);


#ifdef __cplusplus
}
#endif

#endif /* _SETTINGS_TEST_FILE_H */
