/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

extern void test_mqtt_init(void);
extern void test_mqtt_connect(void);
extern void test_mqtt_subscribe(void);
extern void test_mqtt_unsubscribe(void);
extern void test_mqtt_disconnect(void);

ZTEST(net_mqtt_subscriber, test_mqtt_subscriber)
{
	test_mqtt_connect();
	test_mqtt_subscribe();
	test_mqtt_unsubscribe();
	test_mqtt_disconnect();
}

ZTEST_SUITE(net_mqtt_subscriber, NULL, NULL, NULL, NULL, NULL);
