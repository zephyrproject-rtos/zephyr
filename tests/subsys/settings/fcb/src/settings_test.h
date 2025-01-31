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
#include <zephyr/ztest.h>

#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_TEST_FCB_VAL_STR_CNT   64
#define SETTINGS_TEST_FCB_FLASH_CNT   4


extern uint8_t val8;
extern uint8_t val8_un;
extern uint32_t val32;
extern uint64_t val64;

extern int test_get_called;
extern int test_set_called;
extern int test_commit_called;
extern int test_export_block;

extern int c2_var_count;

extern const struct flash_sector fcb_sectors[SETTINGS_TEST_FCB_FLASH_CNT];

extern char val_string[SETTINGS_TEST_FCB_VAL_STR_CNT][SETTINGS_MAX_VAL_LEN];
extern char test_ref_value[SETTINGS_TEST_FCB_VAL_STR_CNT][SETTINGS_MAX_VAL_LEN];

extern struct settings_handler c_test_handlers[];

void ctest_clear_call_state(void);
int ctest_get_call_state(void);

void config_wipe_srcs(void);
void config_wipe_fcb(const struct flash_sector *fs, int cnt);

void test_config_fill_area(
	char test_value[SETTINGS_TEST_FCB_VAL_STR_CNT][SETTINGS_MAX_VAL_LEN],
		int iteration);

void *settings_config_setup(void);
void settings_config_teardown(void *fixture);
void test_config_insert2(void);
void test_config_insert3(void);
int settings_unregister(struct settings_handler *handler);
#ifdef __cplusplus
}
#endif
#endif /* _SETTINGS_TEST_FCB_H */
