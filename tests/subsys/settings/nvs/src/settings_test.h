/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SETTINGS_TEST_NVS_H
#define _SETTINGS_TEST_NVS_H

#include <stdio.h>
#include <string.h>
#include <ztest.h>

#include "settings/settings.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_TEST_NVS_VAL_STR_CNT   64
#define SETTINGS_TEST_NVS_FLASH_CNT   4


extern uint8_t val8;
extern uint8_t val8_un;
extern uint32_t val32;
extern uint64_t val64;

extern int test_get_called;
extern int test_set_called;
extern int test_commit_called;
extern int test_export_block;

extern char val_string[SETTINGS_TEST_NVS_VAL_STR_CNT][SETTINGS_MAX_VAL_LEN];
extern char test_ref_value[SETTINGS_TEST_NVS_VAL_STR_CNT][SETTINGS_MAX_VAL_LEN];

extern struct settings_handler c_test_handlers[];

void ctest_clear_call_state(void);
int ctest_get_call_state(void);

void config_wipe_srcs(void);

#ifdef __cplusplus
}
#endif
#endif /* _SETTINGS_TEST_NVS_H */
