/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"

void test_config_insert_x(int idx)
{
	int rc;

	rc = settings_register(&c_test_handlers[idx]);
	zassert_true(rc == 0, "settings_register fail");
}

int settings_unregister(struct settings_handler *handler)
{
	extern sys_slist_t settings_handlers;

	return sys_slist_find_and_remove(&settings_handlers, &handler->node);
}

void test_config_insert2(void)
{
	test_config_insert_x(1);
}

void test_config_insert3(void)
{
	test_config_insert_x(2);
}

void *settings_config_setup(void)
{
	int rc;

	rc = settings_register(&c_test_handlers[0]);
	zassume_true(rc == 0, "settings_register fail");
	return NULL;
}

void settings_config_teardown(void *fixture)
{

	settings_unregister(&c_test_handlers[0]);
}
