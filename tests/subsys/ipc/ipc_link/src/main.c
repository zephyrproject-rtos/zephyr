/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Two IPC link instances in one image, connected by a test transport that hands the frames of
 * one to the other from a work item, with frames each way on their own. A side can be taken
 * down, which cuts it off the wire both ways.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/ipc/ipc_service.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

/* The transport defines the state of its instances */
#include "ipc_link_priv.h"

#define MTU          256
/* Room for a 4 byte message: a bind message with a name of 2 bytes */
#define TINY_MTU     24
#define RETX_MS      100
#define BIND_TIMEOUT 1000
#define STREAM_COUNT 500
#define RX_BUFFERS   CONFIG_IPC_SERVICE_BACKEND_LINK_RX_BUFFERS
#define MAX_RETX     CONFIG_IPC_SERVICE_BACKEND_LINK_MAX_RETRANSMITS
/* Long enough for the retransmissions to end a session */
#define GONE_MS      (2 * (MAX_RETX + 1) * RETX_MS)

struct pipe_config {
	struct ipc_link_config link;
	const struct device *peer;
};

struct pipe_data {
	const struct device *dev;
	struct k_work work;
	/* On the wire: sends, and takes what the peer sends */
	bool up;
	/* Drop one frame in drop_rate, 0 for none */
	uint32_t drop_rate;
	/* Drop the next frame */
	bool drop_next;
	uint32_t frames;
	uint32_t dropped;
};

static void pipe_work_handler(struct k_work *work)
{
	struct pipe_data *data = CONTAINER_OF(work, struct pipe_data, work);
	const struct pipe_config *cfg = data->dev->config;
	struct pipe_data *peer = cfg->peer->data;

	while (data->up && ipc_link_pending(data->dev)) {
		const uint8_t *frame;
		size_t len;

		frame = ipc_link_tx_prepare(data->dev, &len);
		data->frames++;

		if (!peer->up) {
			continue;
		}

		if ((data->drop_rate > 0U && data->frames % data->drop_rate == 0U) ||
		    data->drop_next) {
			data->drop_next = false;
			data->dropped++;
			continue;
		}

		memcpy(ipc_link_rx_buf(cfg->peer), frame, len);
		(void)ipc_link_rx(cfg->peer, len, true);
	}
}

static int pipe_open(const struct device *dev)
{
	struct pipe_data *data = dev->data;

	data->up = true;

	return 0;
}

static void pipe_kick(const struct device *dev)
{
	struct pipe_data *data = dev->data;

	k_work_submit(&data->work);
}

static const struct ipc_link_transport_api pipe_api = {
	.open = pipe_open,
	.kick = pipe_kick,
};

static int pipe_init(const struct device *dev)
{
	struct pipe_data *data = dev->data;

	data->dev = dev;
	k_work_init(&data->work, pipe_work_handler);

	return ipc_link_init(dev);
}

DEVICE_DECLARE(ipc_b);
DEVICE_DECLARE(ipc_tiny);

#define PIPE_DEFINE(name, peer_name, _mtu)                                                         \
	IPC_LINK_DEFINE(name##_link, _mtu);                                                        \
	static struct pipe_data name##_data;                                                       \
	static const struct pipe_config name##_config = {                                          \
		.link =                                                                            \
			{                                                                          \
				.api = &pipe_api,                                                  \
				IPC_LINK_CONFIG(name##_link),                                      \
				.mtu = _mtu,                                                       \
				.retransmit_timeout_ms = RETX_MS,                                  \
			},                                                                         \
		.peer = DEVICE_GET(peer_name),                                                     \
	};                                                                                         \
	DEVICE_DEFINE(name, #name, pipe_init, NULL, &name##_data, &name##_config, POST_KERNEL,     \
		      CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY, &ipc_link_backend_ops)

PIPE_DEFINE(ipc_a, ipc_b, MTU);
PIPE_DEFINE(ipc_b, ipc_a, MTU);
/* Never opened, for the checks before that */
PIPE_DEFINE(ipc_tiny, ipc_tiny, TINY_MTU);

static const struct device *const dev_a = DEVICE_GET(ipc_a);
static const struct device *const dev_b = DEVICE_GET(ipc_b);

static uint32_t stat_get(const struct device *dev, size_t offset)
{
	struct ipc_link_stats stats;

	zassert_ok(ipc_link_stats_get(dev, &stats));

	return *(uint32_t *)((uint8_t *)&stats + offset);
}

#define STAT(dev, name) stat_get(dev, offsetof(struct ipc_link_stats, name))

/* Waits until a side has nothing left to send: off the wire, what waits would time out */
static void wait_drained(struct ipc_link *link)
{
	for (int i = 0; i < 1000; i++) {
		if (link->core.head == link->core.base && link->core.fill_len == 0U) {
			return;
		}
		k_sleep(K_MSEC(1));
	}

	zassert_unreachable("the link never drained");
}

/* Waits for a counter to move on, as a delivery barrier */
#define WAIT_STAT_CHANGE(dev, name, from, timeout_ms)                                              \
	do {                                                                                       \
		int64_t _end = k_uptime_get() + (timeout_ms);                                      \
                                                                                                   \
		while (STAT(dev, name) == (from) && k_uptime_get() < _end) {                       \
			k_sleep(K_MSEC(1));                                                        \
		}                                                                                  \
		zassert_not_equal(STAT(dev, name), (from), #name " never changed");                \
	} while (0)

/* An endpoint under test: counts its callbacks and keeps the last message */
struct test_ept {
	struct ipc_ept ept;
	struct ipc_ept_cfg cfg;
	struct k_sem bound;
	struct k_sem unbound;
	struct k_sem received;
	uint8_t last[MTU];
	size_t last_len;
	uint32_t count;
	/* Echo what is received */
	bool echo;
	/* Keep the received buffers */
	bool hold;
	const void *held[RX_BUFFERS];
	size_t held_count;
	/* Expect numbered messages in order */
	bool stream;
	uint32_t expected;
	int error;
	/* Block in the next received callback until released */
	bool block;
	struct k_sem blocked;
	struct k_sem release;
	/* Register this endpoint from the next received callback */
	struct test_ept *register_from_cb;
	int register_ret;
	/* Release the buffer of the next message without having held it */
	bool release_unheld;
	int release_ret;
};

static void test_bound(void *priv)
{
	struct test_ept *t = priv;

	k_sem_give(&t->bound);
}

static void test_unbound(void *priv)
{
	struct test_ept *t = priv;

	k_sem_give(&t->unbound);
}

static void test_received(const void *data, size_t len, void *priv)
{
	struct test_ept *t = priv;

	t->count++;

	if (t->stream) {
		if (len != sizeof(uint32_t) || sys_get_le32(data) != t->expected) {
			t->error = -EBADMSG;
		}
		t->expected++;
	}

	if (len <= sizeof(t->last)) {
		memcpy(t->last, data, len);
		t->last_len = len;
	}

	if (t->echo) {
		int ret = ipc_service_send(&t->ept, data, len);

		if (ret < 0) {
			t->error = ret;
		}
	}

	if (t->hold && t->held_count < ARRAY_SIZE(t->held)) {
		if (ipc_service_hold_rx_buffer(&t->ept, (void *)data) == 0) {
			t->held[t->held_count++] = data;
		}
	}

	if (t->release_unheld) {
		t->release_unheld = false;
		t->release_ret = ipc_service_release_rx_buffer(&t->ept, (void *)data);
	}

	if (t->register_from_cb != NULL) {
		struct test_ept *other = t->register_from_cb;

		t->register_from_cb = NULL;
		t->register_ret =
			ipc_service_register_endpoint(t->ept.instance, &other->ept, &other->cfg);
	}

	if (t->block) {
		t->block = false;
		k_sem_give(&t->blocked);
		(void)k_sem_take(&t->release, K_FOREVER);
	}

	k_sem_give(&t->received);
}

static void test_ept_init(struct test_ept *t, const char *name)
{
	memset(t, 0, sizeof(*t));
	t->cfg.name = name;
	t->cfg.priv = t;
	t->cfg.cb.bound = test_bound;
	t->cfg.cb.unbound = test_unbound;
	t->cfg.cb.received = test_received;
	k_sem_init(&t->bound, 0, 1);
	k_sem_init(&t->unbound, 0, 1);
	k_sem_init(&t->received, 0, K_SEM_MAX_LIMIT);
	k_sem_init(&t->blocked, 0, 1);
	k_sem_init(&t->release, 0, 1);
}

static struct test_ept ept_a;
static struct test_ept ept_b;
static struct test_ept ept_a2;
static struct test_ept ept_b2;

/* Sends, waiting for room up to a timeout: the backend itself never waits */
static int send_wait(struct ipc_ept *ept, const void *data, size_t len, uint32_t timeout_ms)
{
	int64_t end = k_uptime_get() + timeout_ms;
	int ret;

	while (true) {
		ret = ipc_service_send(ept, data, len);
		if (ret != -ENOMEM || k_uptime_get() >= end) {
			return ret;
		}

		k_sleep(K_MSEC(1));
	}
}

static void register_pair(void)
{
	zassert_ok(ipc_service_register_endpoint(dev_b, &ept_b.ept, &ept_b.cfg));
	zassert_ok(ipc_service_register_endpoint(dev_a, &ept_a.ept, &ept_a.cfg));
	zassert_ok(k_sem_take(&ept_a.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b.bound, K_MSEC(BIND_TIMEOUT)));
}

/* A callback blocking the system work queue blocks the test transport with it */
static void needs_own_queue(void)
{
	if (!IS_ENABLED(CONFIG_IPC_SERVICE_BACKEND_LINK_WQ_ENABLE)) {
		ztest_test_skip();
	}
}

static void *suite_setup(void)
{
	zassert_ok(ipc_service_open_instance(dev_a));
	zassert_ok(ipc_service_open_instance(dev_b));

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_ept_init(&ept_a, "echo");
	test_ept_init(&ept_b, "echo");
	test_ept_init(&ept_a2, "other");
	test_ept_init(&ept_b2, "other");
	ept_b.echo = true;

	ipc_a_data.drop_rate = 0U;
	ipc_b_data.drop_rate = 0U;
	ipc_a_data.drop_next = false;
	ipc_b_data.drop_next = false;
	ipc_a_data.dropped = 0U;
	ipc_b_data.dropped = 0U;
}

static void after(void *fixture)
{
	struct test_ept *epts[] = {&ept_a, &ept_b, &ept_a2, &ept_b2};

	ARG_UNUSED(fixture);

	for (size_t i = 0; i < ARRAY_SIZE(epts); i++) {
		/* A callback left blocked by a failed test */
		k_sem_give(&epts[i]->release);

		while (epts[i]->held_count > 0) {
			(void)ipc_service_release_rx_buffer(
				&epts[i]->ept, (void *)epts[i]->held[--epts[i]->held_count]);
		}

		if (epts[i]->ept.instance != NULL) {
			(void)ipc_service_deregister_endpoint(&epts[i]->ept);
		}
	}

	/* A side taken down by a failed test */
	ipc_a_data.up = true;
	ipc_b_data.up = true;
}

ZTEST_SUITE(ipc_link, NULL, suite_setup, before, after, NULL);

ZTEST(ipc_link, test_open)
{
	zassert_equal(ipc_service_open_instance(dev_a), -EALREADY);
	zassert_equal(ipc_service_close_instance(dev_a), -ENOTSUP);
}

ZTEST(ipc_link, test_bind)
{
	/* A registers first and waits for B, without blocking */
	zassert_ok(ipc_service_register_endpoint(dev_a, &ept_a.ept, &ept_a.cfg));
	zassert_equal(k_sem_take(&ept_a.bound, K_MSEC(2 * RETX_MS)), -EAGAIN);
	zassert_equal(ipc_service_send(&ept_a.ept, "x", 1), -EBUSY);

	zassert_ok(ipc_service_register_endpoint(dev_b, &ept_b.ept, &ept_b.cfg));
	zassert_ok(k_sem_take(&ept_a.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b.bound, K_MSEC(BIND_TIMEOUT)));

	zassert_equal(ipc_service_register_endpoint(dev_a, &ept_a2.ept, &ept_a.cfg), -EEXIST);
}

ZTEST(ipc_link, test_bind_message_must_fit)
{
	const struct device *dev_tiny = DEVICE_GET(ipc_tiny);

	/* A name whose bind message does not fit a frame is refused, before any other check */
	test_ept_init(&ept_a2, "abc");
	zassert_equal(ipc_service_register_endpoint(dev_tiny, &ept_a2.ept, &ept_a2.cfg), -EMSGSIZE);
	test_ept_init(&ept_a2, "ab");
	zassert_equal(ipc_service_register_endpoint(dev_tiny, &ept_a2.ept, &ept_a2.cfg), -ENETDOWN);
}

ZTEST(ipc_link, test_register_from_callback)
{
	register_pair();

	/* Registered from a callback, the endpoint binds once B has it */
	ept_a.register_from_cb = &ept_a2;
	zassert_equal(ipc_service_send(&ept_a.ept, "hello", 5), 5);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zassert_equal(ept_a.register_ret, 0);

	zassert_ok(ipc_service_register_endpoint(dev_b, &ept_b2.ept, &ept_b2.cfg));
	zassert_ok(k_sem_take(&ept_a2.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b2.bound, K_MSEC(BIND_TIMEOUT)));
}

ZTEST(ipc_link, test_echo)
{
	register_pair();

	zassert_equal(ipc_service_send(&ept_a.ept, "hello", 5), 5);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zexpect_equal(ept_a.last_len, 5);
	zexpect_mem_equal(ept_a.last, "hello", 5);
	zexpect_equal(ept_b.count, 1);
	zexpect_equal(ept_b.error, 0);
}

ZTEST(ipc_link, test_stats)
{
	struct ipc_link_stats stats;

	register_pair();

	zassert_equal(ipc_service_send(&ept_a.ept, "count", 5), 5);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));

	zassert_ok(ipc_link_stats_get(dev_a, &stats));
	zexpect_true(stats.tx_payloads > 0);
	zexpect_true(stats.rx_payloads > 0);
	zexpect_true(stats.rx_frames > 0);

	zassert_ok(ipc_link_stats_reset(dev_a));
	zassert_ok(ipc_link_stats_get(dev_a, &stats));
	zexpect_equal(stats.tx_payloads, 0);
	zexpect_equal(stats.rx_payloads, 0);
	zexpect_equal(stats.rx_frames, 0);

	/* Counting goes on */
	zassert_equal(ipc_service_send(&ept_a.ept, "again", 5), 5);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zassert_ok(ipc_link_stats_get(dev_a, &stats));
	zexpect_equal(stats.tx_payloads, 1);
}

ZTEST(ipc_link, test_message_sizes)
{
	uint8_t big[MTU];
	int max;

	register_pair();

	max = ipc_service_get_tx_buffer_size(&ept_a.ept);
	zassert_equal(max, MTU - 20);
	for (int i = 0; i < max; i++) {
		big[i] = (uint8_t)i;
	}

	zassert_equal(ipc_service_send(&ept_a.ept, big, max + 1), -EMSGSIZE);

	zassert_equal(ipc_service_send(&ept_a.ept, big, max), max);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zexpect_equal(ept_a.last_len, max);
	zexpect_mem_equal(ept_a.last, big, max);

	zassert_equal(ipc_service_send(&ept_a.ept, big, 0), 0);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zexpect_equal(ept_a.last_len, 0);
}

static void stream(uint32_t drop_rate)
{
	ept_b.echo = false;
	ept_b.stream = true;
	ipc_a_data.drop_rate = drop_rate;
	ipc_b_data.drop_rate = drop_rate;

	register_pair();

	for (uint32_t i = 0; i < STREAM_COUNT; i++) {
		uint8_t data[sizeof(uint32_t)];
		int ret;

		sys_put_le32(i, data);

		ret = send_wait(&ept_a.ept, data, sizeof(data), 1000);
		zassert_equal(ret, sizeof(data), "message %u: %d", i, ret);
	}

	for (uint32_t i = 0; i < STREAM_COUNT; i++) {
		zassert_ok(k_sem_take(&ept_b.received, K_MSEC(1000)), "message %u", i);
	}

	zexpect_equal(ept_b.count, STREAM_COUNT);
	zexpect_equal(ept_b.error, 0);
}

ZTEST(ipc_link, test_stream)
{
	stream(0U);
	zexpect_equal(ipc_a_data.dropped, 0);
}

ZTEST(ipc_link, test_stream_lossy)
{
	stream(7U);
	zexpect_true(ipc_a_data.dropped > 0);
	zexpect_true(ipc_b_data.dropped > 0);
	zexpect_true(STAT(dev_a, retransmits) > 0);
}

ZTEST(ipc_link, test_hold)
{
	size_t count = 0;
	uint8_t big[MTU];
	int max;

	ept_b.echo = false;
	ept_b.hold = true;

	register_pair();

	/* B keeps every buffer: A runs out of credits and transmit room, with one message per
	 * frame, and sending no longer takes the message
	 */
	max = ipc_service_get_tx_buffer_size(&ept_a.ept);
	memcpy(big, "held", 4);

	while (true) {
		int ret = send_wait(&ept_a.ept, big, max, 3 * RETX_MS);

		if (ret == -ENOMEM) {
			break;
		}

		zassert_equal(ret, max);
		count++;
		zassert_true(count < 100, "A never ran out of room");
	}

	zexpect_equal(ept_b.held_count, RX_BUFFERS - 1);
	zassert_equal(count, RX_BUFFERS - 1 + IPC_LINK_WINDOW + 1);
	zassert_mem_equal(ept_b.held[0], "held", 4);

	zassert_equal(ipc_service_hold_rx_buffer(&ept_b.ept, (void *)"nope"), -EINVAL);

	/* Released, the rest comes through */
	while (ept_b.held_count > 0) {
		zassert_ok(ipc_service_release_rx_buffer(&ept_b.ept,
							 (void *)ept_b.held[--ept_b.held_count]));
	}
	ept_b.hold = false;

	for (size_t i = 0; i < count; i++) {
		zassert_ok(k_sem_take(&ept_b.received, K_MSEC(1000)), "message %zu", i);
	}
	zexpect_equal(ept_b.count, count);
}

ZTEST(ipc_link, test_release_unheld)
{
	ept_b.echo = false;
	register_pair();

	/* A buffer not held is the delivery's, not the application's to release */
	ept_b.release_unheld = true;
	zassert_equal(ipc_service_send(&ept_a.ept, "mine", 4), 4);
	zassert_ok(k_sem_take(&ept_b.received, K_MSEC(1000)));
	zassert_equal(ept_b.release_ret, -EALREADY);

	/* Held and released from the callback at once: the delivery keeps the buffer until it
	 * returns, and a second release has nothing to release
	 */
	ept_b.hold = true;
	ept_b.release_unheld = true;
	zassert_equal(ipc_service_send(&ept_a.ept, "held", 4), 4);
	zassert_ok(k_sem_take(&ept_b.received, K_MSEC(1000)));
	zassert_equal(ept_b.release_ret, 0);
	zassert_equal(ept_b.held_count, 1);
	ept_b.hold = false;
	zassert_equal(
		ipc_service_release_rx_buffer(&ept_b.ept, (void *)ept_b.held[--ept_b.held_count]),
		-EALREADY);

	zassert_equal(ipc_service_send(&ept_a.ept, "again", 5), 5);
	zassert_ok(k_sem_take(&ept_b.received, K_MSEC(1000)));
	zexpect_mem_equal(ept_b.last, "again", 5);
}

ZTEST(ipc_link, test_queued_at_zero_credits)
{
	uint8_t big[MTU];
	int max;

	ept_b.echo = false;
	ept_b.hold = true;
	register_pair();
	max = ipc_service_get_tx_buffer_size(&ept_a.ept);

	/* B holds every buffer: A is out of credits, with nothing left to send, and idle */
	for (int i = 0; i < RX_BUFFERS - 1; i++) {
		zassert_equal(send_wait(&ept_a.ept, big, max, 1000), max);
	}
	for (int i = 0; i < RX_BUFFERS - 1; i++) {
		zassert_ok(k_sem_take(&ept_b.received, K_MSEC(1000)));
	}
	k_sleep(K_MSEC(3 * RETX_MS));

	/* A queues a message, then B gives its buffers back but the header carrying the credits
	 * is lost: A must ask for them, from the timer started when the message was queued
	 */
	zassert_equal(ipc_service_send(&ept_a.ept, "more", 4), 4);
	ipc_b_data.drop_next = true;
	while (ept_b.held_count > 0) {
		zassert_ok(ipc_service_release_rx_buffer(&ept_b.ept,
							 (void *)ept_b.held[--ept_b.held_count]));
	}
	ept_b.hold = false;

	zassert_ok(k_sem_take(&ept_b.received, K_MSEC(5 * RETX_MS)));
	zexpect_mem_equal(ept_b.last, "more", 4);
	zexpect_equal(ipc_b_data.dropped, 1);
}

ZTEST(ipc_link, test_unbind)
{
	register_pair();

	zassert_ok(ipc_service_deregister_endpoint(&ept_b.ept));
	zassert_ok(k_sem_take(&ept_a.unbound, K_MSEC(1000)));
	zassert_equal(ipc_service_send(&ept_a.ept, "x", 1), -EBUSY);
	zassert_equal(ipc_service_send(&ept_b.ept, "x", 1), -ENOENT);
	zassert_equal(ipc_service_deregister_endpoint(&ept_b.ept), -ENOENT);

	/* Registered again, it binds again */
	zassert_ok(ipc_service_register_endpoint(dev_b, &ept_b.ept, &ept_b.cfg));
	zassert_ok(k_sem_take(&ept_a.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b.bound, K_MSEC(BIND_TIMEOUT)));

	zassert_equal(ipc_service_send(&ept_a.ept, "again", 5), 5);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zexpect_mem_equal(ept_a.last, "again", 5);
}

ZTEST(ipc_link, test_slot_reuse)
{
	register_pair();

	/* A's slot goes to another endpoint: what B sent to the old one must not reach it */
	zassert_ok(ipc_service_deregister_endpoint(&ept_a.ept));
	zassert_ok(ipc_service_register_endpoint(dev_a, &ept_a2.ept, &ept_a2.cfg));
	zassert_ok(ipc_service_register_endpoint(dev_b, &ept_b2.ept, &ept_b2.cfg));
	zassert_ok(k_sem_take(&ept_a2.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b2.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b.unbound, K_MSEC(1000)));

	zassert_equal(ipc_service_send(&ept_b2.ept, "other", 5), 5);
	zassert_ok(k_sem_take(&ept_a2.received, K_MSEC(1000)));
	zexpect_equal(ept_a2.count, 1);
	zexpect_equal(ept_a.count, 0);
}

ZTEST(ipc_link, test_address_reuse)
{
	uint32_t payloads;

	needs_own_queue();
	ept_b.echo = false;
	register_pair();

	/* B is stuck in a callback, so it does not see A deregister "echo" and still sends to
	 * its address. A registers "other" meanwhile, which B binds: it must not get what B
	 * sends to "echo", so A's address stays retired until B acknowledges the unbind
	 */
	ept_b.block = true;
	zassert_equal(ipc_service_send(&ept_a.ept, "hello", 5), 5);
	zassert_ok(k_sem_take(&ept_b.blocked, K_MSEC(1000)));

	payloads = STAT(dev_b, rx_payloads);
	zassert_ok(ipc_service_deregister_endpoint(&ept_a.ept));
	/* The unbind reaches B, and waits in its queue */
	WAIT_STAT_CHANGE(dev_b, rx_payloads, payloads, 1000);
	zassert_ok(ipc_service_register_endpoint(dev_a, &ept_a2.ept, &ept_a2.cfg));
	zassert_ok(ipc_service_register_endpoint(dev_b, &ept_b2.ept, &ept_b2.cfg));

	payloads = STAT(dev_a, rx_payloads);
	zassert_equal(ipc_service_send(&ept_b.ept, "stale", 5), 5);
	WAIT_STAT_CHANGE(dev_a, rx_payloads, payloads, 1000);

	k_sem_give(&ept_b.release);
	zassert_ok(k_sem_take(&ept_b.unbound, K_MSEC(1000)));
	zassert_ok(k_sem_take(&ept_a2.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b2.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_equal(ipc_service_send(&ept_b.ept, "x", 1), -EBUSY);
	zexpect_equal(ept_a2.count, 0);
	zexpect_equal(ept_a.count, 0);

	zassert_equal(ipc_service_send(&ept_b2.ept, "other", 5), 5);
	zassert_ok(k_sem_take(&ept_a2.received, K_MSEC(1000)));
	zexpect_mem_equal(ept_a2.last, "other", 5);
	zexpect_equal(ept_a2.count, 1);
}

ZTEST(ipc_link, test_peer_gone)
{
	uint32_t disconnects;
	uint32_t resets;

	register_pair();
	disconnects = STAT(dev_a, disconnects);
	resets = STAT(dev_b, peer_resets);

	/* B goes off the wire: A ends the session after the retransmissions */
	wait_drained(&ipc_b_link);
	ipc_b_data.up = false;
	zassert_equal(ipc_service_send(&ept_a.ept, "anyone", 6), 6);
	zassert_ok(k_sem_take(&ept_a.unbound, K_MSEC(GONE_MS)));
	zexpect_equal(STAT(dev_a, disconnects), disconnects + 1);

	/* Back, A's boot announcement resets B's session: everything binds again */
	ipc_b_data.up = true;
	zassert_ok(k_sem_take(&ept_b.unbound, K_MSEC(1000)));
	zassert_ok(k_sem_take(&ept_a.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b.bound, K_MSEC(BIND_TIMEOUT)));
	zexpect_equal(STAT(dev_b, peer_resets), resets + 1);

	zassert_equal(ipc_service_send(&ept_a.ept, "back", 4), 4);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zexpect_mem_equal(ept_a.last, "back", 4);
}

ZTEST(ipc_link, test_peer_gone_queued)
{
	uint32_t disconnects;
	uint32_t payloads;

	needs_own_queue();
	ept_b.echo = false;
	register_pair();
	/* Announced to B, which has no such endpoint yet */
	zassert_ok(ipc_service_register_endpoint(dev_a, &ept_a2.ept, &ept_a2.cfg));

	/* A is stuck in a callback while B's bind of that endpoint waits in A's queue; then B
	 * falls silent and A ends the session: the bind belongs to it, and must not bind the
	 * endpoint on a link that is down
	 */
	ept_a.block = true;
	zassert_equal(ipc_service_send(&ept_b.ept, "first", 5), 5);
	zassert_ok(k_sem_take(&ept_a.blocked, K_MSEC(1000)));
	payloads = STAT(dev_a, rx_payloads);
	zassert_ok(ipc_service_register_endpoint(dev_b, &ept_b2.ept, &ept_b2.cfg));
	WAIT_STAT_CHANGE(dev_a, rx_payloads, payloads, 1000);

	disconnects = STAT(dev_a, disconnects);
	wait_drained(&ipc_b_link);
	ipc_b_data.up = false;
	zassert_equal(ipc_service_send(&ept_a.ept, "anyone", 6), 6);
	WAIT_STAT_CHANGE(dev_a, disconnects, disconnects, GONE_MS);

	k_sem_give(&ept_a.release);
	zassert_ok(k_sem_take(&ept_a.unbound, K_MSEC(1000)));
	zassert_equal(k_sem_take(&ept_a2.bound, K_MSEC(100)), -EAGAIN);

	/* Back, everything binds */
	ipc_b_data.up = true;
	zassert_ok(k_sem_take(&ept_a.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_a2.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b2.bound, K_MSEC(BIND_TIMEOUT)));
}

ZTEST(ipc_link, test_session_change_queued)
{
	uint32_t disconnects;
	uint32_t payloads;
	uint32_t resets;
	uint8_t big[MTU];
	int max;

	needs_own_queue();
	ept_b.echo = false;
	register_pair();
	max = ipc_service_get_tx_buffer_size(&ept_a.ept);

	/* B is stuck in a callback while a second frame arrives and waits in its queue, holding
	 * a receive buffer. Then A goes off the wire and B's session ends, and A comes back and
	 * resets it again: the frame belongs to the old session, its buffer must be given back
	 * all the same, and B's endpoint is unbound once, not twice
	 */
	ept_b.block = true;
	zassert_equal(ipc_service_send(&ept_a.ept, big, max), max);
	zassert_ok(k_sem_take(&ept_b.blocked, K_MSEC(1000)));
	payloads = STAT(dev_b, rx_payloads);
	zassert_equal(ipc_service_send(&ept_a.ept, big, max), max);
	WAIT_STAT_CHANGE(dev_b, rx_payloads, payloads, 1000);

	disconnects = STAT(dev_b, disconnects);
	resets = STAT(dev_a, peer_resets);
	wait_drained(&ipc_a_link);
	ipc_a_data.up = false;
	zassert_equal(ipc_service_send(&ept_b.ept, "anyone", 6), 6);
	WAIT_STAT_CHANGE(dev_b, disconnects, disconnects, GONE_MS);
	/* A's session is reset by B's announcement before B's callback returns */
	ipc_a_data.up = true;
	WAIT_STAT_CHANGE(dev_a, peer_resets, resets, 1000);

	k_sem_give(&ept_b.release);
	zassert_ok(k_sem_take(&ept_b.unbound, K_MSEC(1000)));
	zassert_ok(k_sem_take(&ept_a.unbound, K_MSEC(1000)));
	zassert_ok(k_sem_take(&ept_a.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_equal(k_sem_take(&ept_b.unbound, K_MSEC(100)), -EAGAIN);
	zexpect_equal(ept_b.count, 1);
	zexpect_equal(POPCOUNT(ipc_b_link.core.rx_free), RX_BUFFERS - 1);

	/* B has all its credits: A's messages come through */
	k_sem_reset(&ept_b.received);
	for (int i = 0; i < RX_BUFFERS; i++) {
		zassert_equal(ipc_service_send(&ept_a.ept, big, max), max);
		zassert_ok(k_sem_take(&ept_b.received, K_MSEC(1000)), "message %d", i);
	}
}

static void send_from_isr(const void *param)
{
	int *ret = (int *)param;

	*ret = ipc_service_send(&ept_a.ept, "isr", 3);
}

ZTEST(ipc_link, test_send_from_isr)
{
	int ret = 0;

	register_pair();

	irq_offload(send_from_isr, &ret);
	zassert_equal(ret, 3);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zexpect_mem_equal(ept_a.last, "isr", 3);
}
