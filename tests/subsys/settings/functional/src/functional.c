/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Settings functional testing:
 *
 * This file was defines as the original testing is highly dependent on test
 * execution order.
 * Every test here should create separate part and the settings module should
 * be set into known state before every test.
 */
extern void settings_load_pattern(void);



void test_main(void)
{
	settings_load_pattern();

}
