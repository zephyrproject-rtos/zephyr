/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

extern void test_mqtt_init(void);
extern void test_mqtt_connect(void);
extern void test_mqtt_pingreq(void);
extern void test_mqtt_publish(void);
extern void test_mqtt_disconnect(void);

ZTEST(net_mqtt_publisher, test_mqtt_publisher)
{
	test_mqtt_connect();
	test_mqtt_pingreq();
	test_mqtt_publish();
	test_mqtt_disconnect();
}

ZTEST_SUITE(net_mqtt_publisher, NULL, NULL, NULL, NULL, NULL);
