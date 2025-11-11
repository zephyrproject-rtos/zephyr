/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SETTINGS_TEST_ZMS_H
#define _SETTINGS_TEST_ZMS_H

#include <stdio.h>
#include <string.h>
#include <zephyr/ztest.h>

#include <zephyr/settings/settings.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t val8;
extern uint8_t val8_un;
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
void *settings_config_setup(void);
void settings_config_teardown(void *fixture);

#ifdef __cplusplus
}
#endif
#endif /* _SETTINGS_TEST_ZMS_H */
