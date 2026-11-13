/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define TEST_PROTO     0x1f0
#define TEST_PROTO_BAD 0x1ee

#define BASE_US  1000U
#define SCANCODE 0x42U

#define N_READS 3
#define FRAMES  4

static const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(rc_loopback0));
static const struct device *const dev_ungranted = DEVICE_DT_GET(DT_NODELABEL(rc_loopback1));

RC_DT_EVENT_IODEV(evt_iodev, DT_NODELABEL(rc_loopback0));
RC_DT_TX_IODEV(tx_iodev, DT_NODELABEL(rc_loopback0));
RTIO_DEFINE(ctx, 16, 16);

/* Written by the rc receive work queue: not on the user thread's stack */
static ZTEST_BMEM struct rc_event rx[N_READS];

static K_SEM_DEFINE(inject_go, 0, 1);
static K_THREAD_STACK_DEFINE(sender_stack, 1024);
static struct k_thread sender_thread;

static int test_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	uint32_t d = rc_raw_duration_us(word);

	ARG_UNUSED(state);

	if (!rc_raw_is_pulse(word) || (d < BASE_US)) {
		return 0;
	}

	res->proto = TEST_PROTO;
	res->flags = 0U;
	res->scancode = (d - BASE_US) & 0xfffU;

	return 1;
}

RC_PROTOCOL_DEFINE(test_proto) = {
	.name = "test-us",
	.proto_first = TEST_PROTO,
	.proto_last = TEST_PROTO,
	.repeat_period_ms = 1000,
	.decode = test_decode,
};

static void sender(void *a, void *b, void *c)
{
	const uint32_t word = RC_RAW_PULSE_US(BASE_US + SCANCODE);

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (true) {
		k_sem_take(&inject_go, K_FOREVER);
		(void)rc_loopback_inject(dev, &word, 1);
	}
}

static void queue_read(struct rc_event *slot)
{
	struct rtio_sqe sqe;

	rtio_sqe_prep_read(&sqe, &evt_iodev, RTIO_PRIO_NORM, (uint8_t *)slot, sizeof(*slot), slot);
	zassert_ok(rtio_sqe_copy_in_get_handles(&ctx, &sqe, NULL, 1));
	zassert_ok(rtio_submit(&ctx, 0));
}

ZTEST_USER(rc_userspace, test_queued_reads_no_loss)
{
	int events = 0;

	zassert_ok(rc_protocol_enable(dev, TEST_PROTO));

	for (int i = 0; i < N_READS; i++) {
		queue_read(&rx[i]);
	}

	for (int k = 0; k < FRAMES; k++) {
		k_sem_give(&inject_go);

		for (int j = 0; j < N_READS; j++) {
			struct rtio_cqe cqe;
			struct rc_event *evt;

			/* A finite timeout busy-waits and starves the rc receive work queue */
			zassert_equal(rtio_cqe_copy_out(&ctx, &cqe, 1, K_FOREVER), 1,
				      "frame %d read %d", k, j);
			zassert_ok(cqe.result, "frame %d read %d result %d", k, j, cqe.result);

			evt = cqe.userdata;
			zassert_equal(evt->proto, TEST_PROTO);
			zassert_equal(evt->scancode, SCANCODE, "scancode 0x%x", evt->scancode);
			events++;
		}

		if (k < FRAMES - 1) {
			for (int i = 0; i < N_READS; i++) {
				queue_read(&rx[i]);
			}
		}
	}

	zassert_equal(events, N_READS * FRAMES, "events %d", events);
}

ZTEST_USER(rc_userspace, test_protocol_syscalls)
{
	zassert_equal(rc_protocol_is_enabled(dev, TEST_PROTO), 1);
	zassert_ok(rc_protocol_disable(dev, TEST_PROTO));
	zassert_equal(rc_protocol_is_enabled(dev, TEST_PROTO), 0);
	zassert_ok(rc_protocol_enable(dev, TEST_PROTO));
	zassert_equal(rc_protocol_is_enabled(dev, TEST_PROTO), 1);
	zassert_equal(rc_protocol_enable(dev, TEST_PROTO_BAD), -ENOTSUP);
}

ZTEST_USER(rc_userspace, test_caps_timeout_syscalls)
{
	struct rc_caps caps;

	zassert_ok(rc_get_caps(dev, &caps));
	zassert_true((caps.roles & RC_ROLE_RAW_TX) != 0U);
	zassert_ok(rc_set_rx_timeout(dev, 5000U));
}

ZTEST_USER(rc_userspace, test_bad_device_fault)
{
	ztest_set_fault_valid(true);
	(void)rc_protocol_is_enabled(dev_ungranted, TEST_PROTO);
	ztest_test_fail();
}

static ZTEST_DMEM uint32_t tx_words[] = {RC_RAW_PULSE_US(BASE_US)};
static ZTEST_BMEM bool tx_copied;

static void tx_user(void *p1, void *p2, void *p3)
{
	bool chained = (bool)(uintptr_t)p1;
	struct rtio_sqe sqes[2];
	size_t n = 0;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (chained) {
		rtio_sqe_prep_read(&sqes[n], &evt_iodev, RTIO_PRIO_NORM, (uint8_t *)&rx[0],
				   sizeof(rx[0]), NULL);
		sqes[n].flags |= RTIO_SQE_CHAINED;
		n++;
	}
	rtio_sqe_prep_write(&sqes[n], &tx_iodev, RTIO_PRIO_NORM, (const uint8_t *)tx_words,
			    sizeof(tx_words), NULL);
	n++;

	ztest_set_fault_valid(true);
	(void)rtio_sqe_copy_in(&ctx, sqes, n);
	ztest_set_fault_valid(false);
	tx_copied = true;
}

static void tx_denied(bool chained)
{
	static K_THREAD_STACK_DEFINE(tx_user_stack, 1024);
	static struct k_thread tx_user_thread;
	uint32_t acquirable = rtio_sqe_acquirable(&ctx);
	uint32_t total = rc_loopback_tx_total(dev);

	tx_copied = false;
	k_thread_create(&tx_user_thread, tx_user_stack, K_THREAD_STACK_SIZEOF(tx_user_stack),
			tx_user, (void *)(uintptr_t)chained, NULL, NULL, K_PRIO_PREEMPT(5), K_USER,
			K_FOREVER);
	rtio_access_grant(&ctx, &tx_user_thread);
	k_object_access_grant(&evt_iodev, &tx_user_thread);
	k_thread_start(&tx_user_thread);
	zassert_ok(k_thread_join(&tx_user_thread, K_FOREVER));

	zassert_false(tx_copied, "copy-in of a denied write returned");
	zassert_equal(rtio_sqe_acquirable(&ctx), acquirable, "submissions left queued");
	zassert_is_null(rtio_cqe_consume(&ctx));

	k_msleep(10);
	zassert_equal(rc_loopback_tx_total(dev), total, "transmitted");
}

ZTEST(rc_userspace, test_tx_denied)
{
	tx_denied(false);
}

ZTEST(rc_userspace, test_tx_denied_chained)
{
	tx_denied(true);
}

static void *setup(void)
{
	k_object_access_grant(dev, k_current_get());
	k_object_access_grant(&evt_iodev, k_current_get());
	k_object_access_grant(&inject_go, k_current_get());
	rtio_access_grant(&ctx, k_current_get());

	zassert_ok(rc_rx_enable(dev, true));

	k_thread_create(&sender_thread, sender_stack, K_THREAD_STACK_SIZEOF(sender_stack), sender,
			NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	return NULL;
}

ZTEST_SUITE(rc_userspace, NULL, setup, NULL, NULL, NULL);
