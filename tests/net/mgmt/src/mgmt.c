/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <errno.h>
#include <toolchain.h>
#include <linker/sections.h>

#include <net/net_mgmt.h>
#include <net/net_pkt.h>
#include <ztest.h>

#define TEST_MGMT_REQUEST		0x17AB1234
#define TEST_MGMT_EVENT			0x97AB1234
#define TEST_MGMT_EVENT_UNHANDLED	0x97AB4321
#define TEST_MGMT_EVENT_INFO_SIZE	sizeof("mgmt event info")

/* Notifier infra */
static u32_t event2throw;
static u32_t throw_times;
static int throw_sleep;
static bool with_info;
static K_THREAD_STACK_DEFINE(thrower_stack, 512);
static struct k_thread thrower_thread_data;
static struct k_sem thrower_lock;

/* Receiver infra */
static u32_t rx_event;
static u32_t rx_calls;
static struct net_mgmt_event_callback rx_cb;

static struct in6_addr addr6 = { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static char info_data[TEST_MGMT_EVENT_INFO_SIZE] = "mgmt event info";

static int test_mgmt_request(u32_t mgmt_request,
			     struct net_if *iface, void *data, u32_t len)
{
	u32_t *test_data = data;

	ARG_UNUSED(iface);

	if (len == sizeof(u32_t)) {
		*test_data = 1;

		return 0;
	}

	return -EIO;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(TEST_MGMT_REQUEST, test_mgmt_request);

int fake_dev_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static void fake_iface_init(struct net_if *iface)
{
	static u8_t mac[8] = { 0x00, 0x00, 0x00, 0x00, 0x0a, 0x0b, 0x0c, 0x0d};

	net_if_set_link_addr(iface, mac, 8, NET_LINK_DUMMY);
}

static int fake_iface_send(struct net_if *iface, struct net_pkt *pkt)
{
	net_pkt_unref(pkt);

	return NET_OK;
}

static struct net_if_api fake_iface_api = {
	.init = fake_iface_init,
	.send = fake_iface_send,
};

NET_DEVICE_INIT(net_event_test, "net_event_test",
		fake_dev_init, NULL, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&fake_iface_api, DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

void test_requesting_nm(void)
{
	u32_t data = 0;

	TC_PRINT("- Request Net MGMT\n");

	zassert_false(net_mgmt(TEST_MGMT_REQUEST, NULL, &data, sizeof(data)),
		      "Requesting Net MGMT failed");
}

static void thrower_thread(void)
{
	while (1) {
		k_sem_take(&thrower_lock, K_FOREVER);

		TC_PRINT("\tThrowing event 0x%08X %u times\n",
			 event2throw, throw_times);

		for (; throw_times; throw_times--) {
			k_sleep(throw_sleep);

			if (with_info) {
				net_mgmt_event_notify_with_info(
					event2throw, net_if_get_default(),
					info_data,
					TEST_MGMT_EVENT_INFO_SIZE);
			} else {
				net_mgmt_event_notify(event2throw,
						      net_if_get_default());
			}

		}
	}
}

static void receiver_cb(struct net_mgmt_event_callback *cb,
			u32_t nm_event, struct net_if *iface)
{
	TC_PRINT("\t\tReceived event 0x%08X\n", nm_event);

	if (with_info && cb->info) {
		if (memcmp(info_data, cb->info,
			   TEST_MGMT_EVENT_INFO_SIZE)) {
			rx_calls = (u32_t) -1;
			return;
		}
	}

	rx_event = nm_event;
	rx_calls++;
}

static int sending_event(u32_t times, bool receiver, bool info)
{
	int ret = TC_PASS;

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

	k_yield();

	if (receiver) {
		TC_PRINT("\tReceived 0x%08X %u times\n",
			 rx_event, rx_calls);

		zassert_equal(rx_event, event2throw, "rx_event check failed");
		zassert_equal(rx_calls, times, "rx_calls check failed");

		net_mgmt_del_event_callback(&rx_cb);
		rx_event = rx_calls = 0;
	}

	return ret;
}

static int test_sending_event(u32_t times, bool receiver)
{
	return sending_event(times, receiver, false);
}

static int test_sending_event_info(u32_t times, bool receiver)
{
	return sending_event(times, receiver, true);
}

static int test_synchronous_event_listener(u32_t times, bool on_iface)
{
	u32_t event_mask;
	int ret;

	TC_PRINT("- Synchronous event listener %s\n",
		 on_iface ? "on interface" : "");

	event2throw = TEST_MGMT_EVENT | (on_iface ? NET_MGMT_IFACE_BIT : 0);
	throw_times = times;
	throw_sleep = K_MSEC(200);

	event_mask = event2throw;

	k_sem_give(&thrower_lock);

	if (on_iface) {
		ret = net_mgmt_event_wait_on_iface(net_if_get_default(),
						   event_mask, NULL, NULL,
						   K_SECONDS(1));
	} else {
		ret = net_mgmt_event_wait(event_mask, NULL, NULL, NULL,
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

static void initialize_event_tests(void)
{
	event2throw = 0;
	throw_times = 0;
	throw_sleep = K_NO_WAIT;
	with_info = false;

	rx_event = 0;
	rx_calls = 0;

	k_sem_init(&thrower_lock, 0, UINT_MAX);

	net_mgmt_init_event_callback(&rx_cb, receiver_cb, TEST_MGMT_EVENT);

	k_thread_create(&thrower_thread_data, thrower_stack,
			K_THREAD_STACK_SIZEOF(thrower_stack),
			(k_thread_entry_t)thrower_thread,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, 0);
}

static int test_core_event(u32_t event, bool (*func)(void))
{
	int ret = TC_PASS;

	TC_PRINT("- Triggering core event: 0x%08X\n", event);

	net_mgmt_init_event_callback(&rx_cb, receiver_cb, event);

	net_mgmt_add_event_callback(&rx_cb);

	zassert_true(func(), "func() check failed");

	k_yield();

	zassert_true(rx_calls, "rx_calls empty");
	zassert_equal(rx_event, event, "rx_event check failed");

	net_mgmt_del_event_callback(&rx_cb);
	rx_event = rx_calls = 0;

	return ret;
}

static bool _iface_ip6_add(void)
{
	if (net_if_ipv6_addr_add(net_if_get_default(),
				 &addr6, NET_ADDR_MANUAL, 0)) {
		return true;
	}

	return false;
}

static bool _iface_ip6_del(void)
{
	if (net_if_ipv6_addr_rm(net_if_get_default(), &addr6)) {
		return true;
	}

	return false;
}

void test_mgmt(void)
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

	zassert_false(test_core_event(NET_EVENT_IPV6_ADDR_ADD, _iface_ip6_add),
		      "test_core_event failed");

	zassert_false(test_core_event(NET_EVENT_IPV6_ADDR_DEL, _iface_ip6_del),
		      "test_core_event failed");

	zassert_false(test_synchronous_event_listener(2, false),
		      "test_synchronous_event_listener failed");

	zassert_false(test_synchronous_event_listener(2, true),
		      "test_synchronous_event_listener failed");
}

void test_main(void)
{
	ztest_test_suite(test_mgmt_fn, ztest_unit_test(test_mgmt));
	ztest_run_test_suite(test_mgmt_fn);
}
