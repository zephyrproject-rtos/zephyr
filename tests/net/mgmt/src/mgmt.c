/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_MGMT_EVENT_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/tc_util.h>
#include <errno.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

#include <zephyr/net/dummy.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/ztest.h>

#define THREAD_SLEEP 50 /* ms */
#define TEST_INFO_STRING "mgmt event info"

#define TEST_MGMT_REQUEST		0x17AB1234
#define TEST_MGMT_EVENT			0x97AB1234
#define TEST_MGMT_EVENT_UNHANDLED	0x97AB4321
#define TEST_MGMT_EVENT_INFO_SIZE	\
	MAX(sizeof(TEST_INFO_STRING), sizeof(struct in6_addr))

/* Notifier infra */
static uint32_t event2throw;
static uint32_t throw_times;
static uint32_t throw_sleep;
static bool with_info;
static bool with_static;
static K_THREAD_STACK_DEFINE(thrower_stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread thrower_thread_data;
static struct k_sem thrower_lock;

/* Receiver infra */
static uint32_t rx_event;
static uint32_t rx_calls;
static size_t info_length_in_test;
static struct net_mgmt_event_callback rx_cb;
static char *info_string = TEST_INFO_STRING;

static struct in6_addr addr6 = { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static char info_data[TEST_MGMT_EVENT_INFO_SIZE];

static int test_mgmt_request(uint32_t mgmt_request,
			     struct net_if *iface, void *data, uint32_t len)
{
	uint32_t *test_data = data;

	ARG_UNUSED(iface);

	if (len == sizeof(uint32_t)) {
		*test_data = 1U;

		return 0;
	}

	return -EIO;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(TEST_MGMT_REQUEST, test_mgmt_request);

static void test_mgmt_event_handler(uint32_t mgmt_event, struct net_if *iface, void *info,
				    size_t info_length, void *user_data)
{
	if (!with_static) {
		return;
	}

	TC_PRINT("\t\tReceived static event 0x%08X\n", mgmt_event);

	ARG_UNUSED(user_data);

	if (with_info && info) {
		if (info_length != info_length_in_test) {
			rx_calls = (uint32_t) -1;
			return;
		}

		if (memcmp(info_data, info, info_length_in_test)) {
			rx_calls = (uint32_t) -1;
			return;
		}
	}

	rx_event = mgmt_event;
	rx_calls++;
}

NET_MGMT_REGISTER_EVENT_HANDLER(my_test_handler, TEST_MGMT_EVENT, test_mgmt_event_handler, NULL);

int fake_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static void fake_iface_init(struct net_if *iface)
{
	static uint8_t mac[8] = { 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0b, 0x0c, 0x0d};

	net_if_set_link_addr(iface, mac, 8, NET_LINK_DUMMY);
}

static int fake_iface_send(const struct device *dev, struct net_pkt *pkt)
{
	return 0;
}

static struct dummy_api fake_iface_api = {
	.iface_api.init = fake_iface_init,
	.send = fake_iface_send,
};

NET_DEVICE_INIT(net_event_test, "net_event_test",
		fake_dev_init, NULL,
		NULL, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&fake_iface_api, DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

void test_requesting_nm(void)
{
	uint32_t data = 0U;

	TC_PRINT("- Request Net MGMT\n");

	zassert_false(net_mgmt(TEST_MGMT_REQUEST, NULL, &data, sizeof(data)),
		      "Requesting Net MGMT failed");
}

static void thrower_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		k_sem_take(&thrower_lock, K_FOREVER);

		TC_PRINT("\tThrowing event 0x%08X %u times\n",
			 event2throw, throw_times);

		for (; throw_times; throw_times--) {
			k_msleep(throw_sleep);

			if (with_info) {
				net_mgmt_event_notify_with_info(
					event2throw,
					net_if_get_first_by_type(
						      &NET_L2_GET_NAME(DUMMY)),
					info_data,
					TEST_MGMT_EVENT_INFO_SIZE);
			} else {
				net_mgmt_event_notify(event2throw,
					net_if_get_first_by_type(
						&NET_L2_GET_NAME(DUMMY)));
			}

		}
	}
}

static void receiver_cb(struct net_mgmt_event_callback *cb,
			uint32_t nm_event, struct net_if *iface)
{
	TC_PRINT("\t\tReceived event 0x%08X\n", nm_event);

	if (with_info && cb->info) {
		if (cb->info_length != info_length_in_test) {
			rx_calls = (uint32_t) -1;
			return;
		}

		if (memcmp(info_data, cb->info, info_length_in_test)) {
			rx_calls = (uint32_t) -1;
			return;
		}
	}

	rx_event = nm_event;
	rx_calls++;
}

static int sending_event(uint32_t times, bool receiver, bool info)
{
	TC_PRINT("- Sending event %u times, %s a receiver, %s info\n",
		 times, receiver ? "with" : "without",
		 info ? "with" : "without");

	event2throw = TEST_MGMT_EVENT;
	throw_times = times;
	with_info = info;

	if (receiver) {
		net_mgmt_add_event_callback(&rx_cb);
	}

	k_sem_give(&thrower_lock);

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);

	if (receiver) {
		TC_PRINT("\tReceived 0x%08X %u times\n",
			 rx_event, rx_calls);

		zassert_equal(rx_event, event2throw, "rx_event check failed");
		zassert_equal(rx_calls, times, "rx_calls check failed");

		net_mgmt_del_event_callback(&rx_cb);
		rx_event = rx_calls = 0U;
	}

	return TC_PASS;
}

static int test_sending_event(uint32_t times, bool receiver)
{
	return sending_event(times, receiver, false);
}

static int test_sending_event_info(uint32_t times, bool receiver)
{
	return sending_event(times, receiver, true);
}

static int test_synchronous_event_listener(uint32_t times, bool on_iface)
{
	uint32_t event_mask;
	int ret;

	TC_PRINT("- Synchronous event listener %s\n",
		 on_iface ? "on interface" : "");

	event2throw = TEST_MGMT_EVENT | (on_iface ? NET_MGMT_IFACE_BIT : 0);
	throw_times = times;
	throw_sleep = 200;

	event_mask = event2throw;

	k_sem_give(&thrower_lock);

	if (on_iface) {
		ret = net_mgmt_event_wait_on_iface(
			net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)),
			event_mask, NULL, NULL,
			NULL, K_SECONDS(1));
	} else {
		ret = net_mgmt_event_wait(event_mask, NULL, NULL, NULL, NULL,
					  K_SECONDS(1));
	}

	if (ret < 0) {
		if (ret == -ETIMEDOUT) {
			TC_ERROR("Call timed out\n");
		}

		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_static_event_listener(uint32_t times, bool info)
{
	TC_PRINT("- Static event listener %s\n", info ? "with info" : "");

	event2throw = TEST_MGMT_EVENT;
	throw_times = times;
	throw_sleep = 0;
	with_info = info;
	with_static = true;

	k_sem_give(&thrower_lock);

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);

	TC_PRINT("\tReceived 0x%08X %u times\n",
			rx_event, rx_calls);

	zassert_equal(rx_event, event2throw, "rx_event check failed");
	zassert_equal(rx_calls, times, "rx_calls check failed");

	rx_event = rx_calls = 0U;
	with_static = false;

	return TC_PASS;
}

static void initialize_event_tests(void)
{
	event2throw = 0U;
	throw_times = 0U;
	throw_sleep = 0;
	with_info = false;

	rx_event = 0U;
	rx_calls = 0U;

	k_sem_init(&thrower_lock, 0, UINT_MAX);

	info_length_in_test = TEST_MGMT_EVENT_INFO_SIZE;
	memcpy(info_data, info_string, strlen(info_string) + 1);

	net_mgmt_init_event_callback(&rx_cb, receiver_cb, TEST_MGMT_EVENT);

	k_thread_create(&thrower_thread_data, thrower_stack,
			K_THREAD_STACK_SIZEOF(thrower_stack),
			thrower_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
}

static int test_core_event(uint32_t event, bool (*func)(void))
{
	TC_PRINT("- Triggering core event: 0x%08X\n", event);

	info_length_in_test = sizeof(struct in6_addr);
	memcpy(info_data, &addr6, sizeof(addr6));

	net_mgmt_init_event_callback(&rx_cb, receiver_cb, event);

	net_mgmt_add_event_callback(&rx_cb);

	zassert_true(func(), "func() check failed");

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);

	zassert_true(rx_calls > 0 && rx_calls != -1, "rx_calls empty");
	zassert_equal(rx_event, event, "rx_event check failed, "
		      "0x%08x vs 0x%08x", rx_event, event);

	net_mgmt_del_event_callback(&rx_cb);
	rx_event = rx_calls = 0U;

	return TC_PASS;
}

static bool _iface_ip6_add(void)
{
	if (net_if_ipv6_addr_add(
		    net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)),
		    &addr6, NET_ADDR_MANUAL, 0)) {
		return true;
	}

	return false;
}

static bool _iface_ip6_del(void)
{
	if (net_if_ipv6_addr_rm(
		    net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY)),
		    &addr6)) {
		return true;
	}

	return false;
}

ZTEST(mgmt_fn_test_suite, test_mgmt)
{
	TC_PRINT("Starting Network Management API test\n");

	test_requesting_nm();

	initialize_event_tests();

	zassert_false(test_sending_event(1, false),
		      "test_sending_event failed");

	zassert_false(test_sending_event(2, false),
		      "test_sending_event failed");

	zassert_false(test_sending_event(1, true),
		      "test_sending_event failed");

	zassert_false(test_sending_event(2, true),
		      "test_sending_event failed");

	zassert_false(test_sending_event_info(1, false),
		      "test_sending_event failed");

	zassert_false(test_sending_event_info(2, false),
		      "test_sending_event failed");

	zassert_false(test_sending_event_info(1, true),
		      "test_sending_event failed");

	zassert_false(test_sending_event_info(2, true),
		      "test_sending_event failed");

	zassert_false(test_static_event_listener(1, false),
		      "test_static_event_listener failed");

	zassert_false(test_static_event_listener(2, false),
		      "test_static_event_listener failed");

	zassert_false(test_static_event_listener(1, true),
		      "test_static_event_listener failed");

	zassert_false(test_static_event_listener(2, true),
		      "test_static_event_listener failed");

	zassert_false(test_core_event(NET_EVENT_IPV6_ADDR_ADD, _iface_ip6_add),
		      "test_core_event failed");

	zassert_false(test_core_event(NET_EVENT_IPV6_ADDR_DEL, _iface_ip6_del),
		      "test_core_event failed");

	zassert_false(test_synchronous_event_listener(2, false),
		      "test_synchronous_event_listener failed");

	zassert_false(test_synchronous_event_listener(2, true),
		      "test_synchronous_event_listener failed");
}

static K_SEM_DEFINE(wait_for_event_processing, 0, 1);

static void net_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, struct net_if *iface)
{
	static int cb_call_count;

	ARG_UNUSED(cb);
	ARG_UNUSED(iface);
	ARG_UNUSED(mgmt_event);

	k_sem_give(&wait_for_event_processing);
	cb_call_count++;
	zassert_equal(cb_call_count, 1, "Too many calls to event callback");
}

ZTEST(mgmt_fn_test_suite, test_mgmt_duplicate_handler)
{
	struct net_mgmt_event_callback cb;
	int ret;

	net_mgmt_init_event_callback(&cb, net_mgmt_event_handler, NET_EVENT_IPV6_ADDR_ADD);
	net_mgmt_add_event_callback(&cb);
	net_mgmt_add_event_callback(&cb);

	net_mgmt_event_notify(NET_EVENT_IPV6_ADDR_ADD, NULL);

	ret = k_sem_take(&wait_for_event_processing, K_MSEC(50));
	zassert_equal(ret, 0, "Event is not processed");

	net_mgmt_del_event_callback(&cb);
}

ZTEST_SUITE(mgmt_fn_test_suite, NULL, NULL, NULL, NULL, NULL);
