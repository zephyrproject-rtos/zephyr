/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <tc_util.h>
#include <errno.h>
#include <toolchain.h>
#include <sections.h>

#include <net/net_mgmt.h>

#define TEST_MGMT_REQUEST		0x0ABC1234
#define TEST_MGMT_EVENT			0x8ABC1234
#define TEST_MGMT_EVENT_UNHANDLED	0x8ABC4321

/* Notifier infra */
static uint32_t event2throw;
static uint32_t throw_times;
static char __noinit __stack thrower_stack[512];
static struct nano_sem thrower_lock;

/* Receiver infra */
static uint32_t rx_event;
static uint32_t rx_calls;
static struct net_mgmt_event_callback rx_cb;

int test_mgmt_request(uint32_t mgmt_request,
		      struct net_if *iface, void *data, uint32_t len)
{
	uint32_t *test_data = data;

	ARG_UNUSED(iface);

	if (len == sizeof(uint32_t)) {
		*test_data = 1;

		return 0;
	}

	return -EIO;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(TEST_MGMT_REQUEST, test_mgmt_request);

static inline int test_requesting_nm(void)
{
	uint32_t data = 0;

	TC_PRINT("- Request Net MGMT\n");

	if (net_mgmt(TEST_MGMT_REQUEST, NULL, &data, sizeof(data))) {
		return TC_FAIL;
	}

	return TC_PASS;
}

static void thrower_fiber(void)
{
	while (1) {
		nano_fiber_sem_take(&thrower_lock, TICKS_UNLIMITED);

		TC_PRINT("\tThrowing event 0x%08X %u times\n",
			 event2throw, throw_times);

		for (; throw_times; throw_times--) {
			net_mgmt_event_notify(event2throw, NULL);
		}
	}
}

static void receiver_cb(struct net_mgmt_event_callback *cb,
			uint32_t nm_event, struct net_if *iface)
{
	TC_PRINT("\t\tReceived event 0x%08X\n", nm_event);

	rx_event = nm_event;
	rx_calls++;
}

static inline int test_sending_event(uint32_t times, bool receiver)
{
	int ret = TC_PASS;

	TC_PRINT("- Sending event %u times, %s a receiver\n",
		 times, receiver ? "with" : "without");

	event2throw = TEST_MGMT_EVENT;
	throw_times = times;

	if (receiver) {
		net_mgmt_add_event_callback(&rx_cb);
	}

	nano_sem_give(&thrower_lock);

	fiber_yield();

	if (receiver) {
		TC_PRINT("\tReceived 0x%08X %u times\n",
			 rx_event, rx_calls);

		if (rx_event != event2throw) {
			ret = TC_FAIL;
		}

		if (rx_calls != times) {
			ret = TC_FAIL;
		}

		net_mgmt_del_event_callback(&rx_cb);
		rx_event = rx_calls = 0;
	}

	return ret;
}

static void initialize_event_tests(void)
{
	event2throw = 0;
	throw_times = 0;

	rx_event = 0;
	rx_calls = 0;

	nano_sem_init(&thrower_lock);

	net_mgmt_init_event_callback(&rx_cb, receiver_cb, TEST_MGMT_EVENT);

	fiber_start(thrower_stack, sizeof(thrower_stack),
		    (nano_fiber_entry_t)thrower_fiber, 0, 0, 7, 0);
}

void main(void)
{
	int status = TC_FAIL;

	TC_PRINT("Starting Network Management API test\n");

	if (test_requesting_nm() != TC_PASS) {
		goto end;
	}

	initialize_event_tests();

	if (test_sending_event(1, false) != TC_PASS) {
		goto end;
	}

	if (test_sending_event(2, false) != TC_PASS) {
		goto end;
	}

	if (test_sending_event(1, true) != TC_PASS) {
		goto end;
	}

	if (test_sending_event(2, true) != TC_PASS) {
		goto end;
	}

	status = TC_PASS;

end:
	TC_END_RESULT(status);
	TC_END_REPORT(status);
}
