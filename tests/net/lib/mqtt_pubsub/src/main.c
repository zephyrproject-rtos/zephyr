/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

extern void test_mqtt_connect(void);
extern void test_mqtt_subscribe(void);
extern void test_mqtt_publish_short(void);
extern void test_mqtt_publish_long(void);
extern void test_mqtt_unsubscribe(void);
extern void test_mqtt_disconnect(void);

void test_main(void)
{
	ztest_test_suite(mqtt_test,
			ztest_unit_test(test_mqtt_connect),
			ztest_unit_test(test_mqtt_subscribe),
			ztest_unit_test(test_mqtt_publish_short),
			ztest_unit_test(test_mqtt_publish_long),
			ztest_unit_test(test_mqtt_unsubscribe),
			ztest_unit_test(test_mqtt_disconnect));
	ztest_run_test_suite(mqtt_test);
}
