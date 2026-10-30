/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A controller and a peripheral IPC link instance in one image, over a test SPI bus pair and a
 * test GPIO port, so both transfer threads, RDY and IRQ run as on two MCUs.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ipc/ipc_link.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define BIND_TIMEOUT 1000
#define STREAM_COUNT 300
#define RETX_MS      DT_PROP(DT_NODELABEL(ipc_controller), retransmit_timeout_ms)
#define MAX_RETX     CONFIG_IPC_SERVICE_BACKEND_LINK_MAX_RETRANSMITS
/* Long enough for the retransmissions to end a session */
#define GONE_MS      (2 * (MAX_RETX + 1) * RETX_MS)

#define RDY_PIN DT_GPIO_PIN(DT_NODELABEL(ipc_controller), rdy_gpios)

static const struct device *const dev_c = DEVICE_DT_GET(DT_NODELABEL(ipc_controller));
static const struct device *const dev_p = DEVICE_DT_GET(DT_NODELABEL(ipc_peripheral));

extern atomic_t spi_link_transfers;

struct test_ept {
	struct ipc_ept ept;
	struct ipc_ept_cfg cfg;
	struct k_sem bound;
	struct k_sem unbound;
	struct k_sem received;
	uint8_t last[256];
	size_t last_len;
	uint32_t count;
	bool echo;
	bool stream;
	uint32_t expected;
	int error;
	/* Keep the received buffers */
	bool hold;
	const void *held[CONFIG_IPC_SERVICE_BACKEND_LINK_RX_BUFFERS];
	size_t held_count;
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
}

static struct test_ept ept_c;

static uint32_t stat_get(const struct device *dev, size_t offset)
{
	struct ipc_link_stats stats;

	zassert_ok(ipc_link_stats_get(dev, &stats));

	return *(uint32_t *)((uint8_t *)&stats + offset);
}

#define STAT(dev, name) stat_get(dev, offsetof(struct ipc_link_stats, name))

static void *suite_setup(void)
{
	zassert_ok(ipc_service_open_instance(dev_p));
	zassert_ok(ipc_service_open_instance(dev_c));

	return NULL;
}

#ifdef CONFIG_TEST_IPC_LINK_SPI_MISMATCH

ZTEST_SUITE(ipc_link_spi_mismatch, NULL, suite_setup, NULL, NULL, NULL);

/* The peripheral runs another frame size: each side probes it once per timeout, no more */
ZTEST(ipc_link_spi_mismatch, test_probed_slowly)
{
	atomic_val_t transfers;

	k_sleep(K_MSEC(3 * RETX_MS));

	transfers = atomic_get(&spi_link_transfers);
	zassert_true(transfers >= 2 && transfers <= 8, "%ld transactions in %d ms", (long)transfers,
		     3 * RETX_MS);
	zexpect_true(STAT(dev_c, peer_mismatches) > 0);

	test_ept_init(&ept_c, "echo");
	zassert_ok(ipc_service_register_endpoint(dev_c, &ept_c.ept, &ept_c.cfg));
	zassert_equal(k_sem_take(&ept_c.bound, K_MSEC(RETX_MS)), -EAGAIN);
}

#else

static const struct device *const gpio = DEVICE_DT_GET(DT_NODELABEL(gpio_link));
static struct test_ept ept_p;
static struct test_ept ept_c2;

void spi_link_pulse(const struct device *dev);
void spi_link_unplug(bool unplugged);

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
	zassert_ok(ipc_service_register_endpoint(dev_p, &ept_p.ept, &ept_p.cfg));
	zassert_ok(ipc_service_register_endpoint(dev_c, &ept_c.ept, &ept_c.cfg));
	zassert_ok(k_sem_take(&ept_c.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_p.bound, K_MSEC(BIND_TIMEOUT)));
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_ept_init(&ept_c, "echo");
	test_ept_init(&ept_p, "echo");
	test_ept_init(&ept_c2, "other");
}

static void after(void *fixture)
{
	struct test_ept *epts[] = {&ept_c, &ept_p, &ept_c2};

	ARG_UNUSED(fixture);

	spi_link_unplug(false);

	for (size_t i = 0; i < ARRAY_SIZE(epts); i++) {
		while (epts[i]->held_count > 0) {
			(void)ipc_service_release_rx_buffer(
				&epts[i]->ept, (void *)epts[i]->held[--epts[i]->held_count]);
		}

		(void)ipc_service_deregister_endpoint(&epts[i]->ept);
	}
}

ZTEST_SUITE(ipc_link_spi, NULL, suite_setup, before, after, NULL);

static void echo(struct test_ept *from, struct test_ept *to)
{
	to->echo = true;

	register_pair();

	zassert_equal(ipc_service_send(&from->ept, "hello", 5), 5);
	zassert_ok(k_sem_take(&from->received, K_MSEC(1000)));
	zassert_equal(from->last_len, 5);
	zassert_mem_equal(from->last, "hello", 5);
	zassert_equal(to->error, 0);
}

ZTEST(ipc_link_spi, test_echo_from_controller)
{
	echo(&ept_c, &ept_p);
}

/* The peripheral starts: its IRQ line makes the controller clock */
ZTEST(ipc_link_spi, test_echo_from_peripheral)
{
	echo(&ept_p, &ept_c);
}

ZTEST(ipc_link_spi, test_bind_later)
{
	/* The controller registers first, and binds once the peripheral registers */
	zassert_ok(ipc_service_register_endpoint(dev_c, &ept_c.ept, &ept_c.cfg));
	zassert_equal(k_sem_take(&ept_c.bound, K_MSEC(2 * RETX_MS)), -EAGAIN);

	zassert_ok(ipc_service_register_endpoint(dev_p, &ept_p.ept, &ept_p.cfg));
	zassert_ok(k_sem_take(&ept_c.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_p.bound, K_MSEC(BIND_TIMEOUT)));
}

ZTEST(ipc_link_spi, test_largest_message)
{
	uint8_t big[256];
	int max;

	ept_p.echo = true;
	register_pair();

	max = ipc_service_get_tx_buffer_size(&ept_c.ept);
	zassert_true(max > 0 && max < sizeof(big));
	for (int i = 0; i < max; i++) {
		big[i] = (uint8_t)i;
	}

	zassert_equal(ipc_service_send(&ept_c.ept, big, max + 1), -EMSGSIZE);
	zassert_equal(ipc_service_send(&ept_c.ept, big, max), max);
	zassert_ok(k_sem_take(&ept_c.received, K_MSEC(1000)));
	zexpect_equal(ept_c.last_len, max);
	zexpect_mem_equal(ept_c.last, big, max);
}

ZTEST(ipc_link_spi, test_stream_both_ways)
{
	ept_c.stream = true;
	ept_p.stream = true;

	register_pair();

	for (uint32_t i = 0; i < STREAM_COUNT; i++) {
		uint8_t data[sizeof(uint32_t)];
		int ret;

		sys_put_le32(i, data);

		ret = send_wait(&ept_c.ept, data, sizeof(data), 1000);
		zassert_equal(ret, sizeof(data), "message %u to the peripheral: %d", i, ret);

		ret = send_wait(&ept_p.ept, data, sizeof(data), 1000);
		zassert_equal(ret, sizeof(data), "message %u to the controller: %d", i, ret);
	}

	for (uint32_t i = 0; i < STREAM_COUNT; i++) {
		zassert_ok(k_sem_take(&ept_c.received, K_MSEC(1000)), "message %u", i);
		zassert_ok(k_sem_take(&ept_p.received, K_MSEC(1000)), "message %u", i);
	}

	zexpect_equal(ept_c.count, STREAM_COUNT);
	zexpect_equal(ept_p.count, STREAM_COUNT);
	zexpect_equal(ept_c.error, 0);
	zexpect_equal(ept_p.error, 0);
}

ZTEST(ipc_link_spi, test_hold)
{
	size_t count = 0;
	uint8_t big[256];
	int max;

	ept_p.hold = true;
	register_pair();

	/* The peripheral keeps every buffer: the controller runs out of credits, then out of
	 * room. Once they are released, the peripheral has credits to give and nothing else: it
	 * must ask for the transaction that carries them with IRQ
	 */
	max = ipc_service_get_tx_buffer_size(&ept_c.ept);

	while (true) {
		int ret = send_wait(&ept_c.ept, big, max, 3 * RETX_MS);

		if (ret == -ENOMEM) {
			break;
		}

		zassert_equal(ret, max);
		count++;
		zassert_true(count < 100, "the controller never ran out of room");
	}

	zexpect_equal(ept_p.held_count, CONFIG_IPC_SERVICE_BACKEND_LINK_RX_BUFFERS - 1);

	while (ept_p.held_count > 0) {
		zassert_ok(ipc_service_release_rx_buffer(&ept_p.ept,
							 (void *)ept_p.held[--ept_p.held_count]));
	}
	ept_p.hold = false;

	for (size_t i = 0; i < count; i++) {
		zassert_ok(k_sem_take(&ept_p.received, K_MSEC(1000)), "message %zu", i);
	}
	zexpect_equal(ept_p.count, count);
}

ZTEST(ipc_link_spi, test_unbind)
{
	ept_p.echo = true;
	register_pair();

	zassert_ok(ipc_service_deregister_endpoint(&ept_p.ept));
	zassert_ok(k_sem_take(&ept_c.unbound, K_MSEC(1000)));
	zassert_equal(ipc_service_send(&ept_c.ept, "x", 1), -EBUSY);

	zassert_ok(ipc_service_register_endpoint(dev_p, &ept_p.ept, &ept_p.cfg));
	zassert_ok(k_sem_take(&ept_c.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_p.bound, K_MSEC(BIND_TIMEOUT)));

	zassert_equal(ipc_service_send(&ept_c.ept, "again", 5), 5);
	zassert_ok(k_sem_take(&ept_c.received, K_MSEC(1000)));
	zexpect_mem_equal(ept_c.last, "again", 5);
}

/* Waits until the controller clocks nothing for a while and the peripheral is armed */
static void wait_idle(void)
{
	atomic_val_t transfers;

	for (int i = 0; i < 100; i++) {
		transfers = atomic_get(&spi_link_transfers);
		k_sleep(K_MSEC(20));
		if (atomic_get(&spi_link_transfers) == transfers &&
		    gpio_pin_get(gpio, RDY_PIN) == 1) {
			return;
		}
	}

	zassert_unreachable("the link never went idle");
}

/* A chip select pulse without clocks delivers nothing: the stale receive buffer isn't a frame */
ZTEST(ipc_link_spi, test_peripheral_pulse)
{
	struct ipc_link_stats before;
	struct ipc_link_stats after;

	echo(&ept_c, &ept_p);
	wait_idle();

	zassert_ok(ipc_link_stats_get(dev_p, &before));
	spi_link_pulse(DEVICE_DT_GET(DT_NODELABEL(spi_peripheral)));

	for (int i = 0; i < 100; i++) {
		zassert_ok(ipc_link_stats_get(dev_p, &after));
		if (after.header_errors != before.header_errors) {
			break;
		}
		k_sleep(K_MSEC(1));
	}

	zassert_equal(after.header_errors, before.header_errors + 1);
	zassert_equal(after.rx_payloads, before.rx_payloads);
	zassert_equal(after.rx_dropped, before.rx_dropped);
	zassert_equal(ept_p.count, 1);

	zassert_equal(ipc_service_send(&ept_c.ept, "again", 5), 5);
	zassert_ok(k_sem_take(&ept_c.received, K_MSEC(1000)));
	zexpect_mem_equal(ept_c.last, "again", 5);
}

/*
 * The peripheral stops answering: no RDY, no transaction. The controller's timeouts end the
 * session all the same, and the link comes back once the peripheral does
 */
ZTEST(ipc_link_spi, test_peripheral_gone)
{
	uint32_t disconnects;
	int64_t start;

	ept_p.echo = true;
	register_pair();
	wait_idle();
	disconnects = STAT(dev_c, disconnects);

	/* Unplugged, with its RDY line dead */
	spi_link_unplug(true);
	zassert_ok(gpio_pin_set(gpio, RDY_PIN, 0));

	start = k_uptime_get();
	zassert_equal(ipc_service_send(&ept_c.ept, "anyone", 6), 6);
	zassert_ok(k_sem_take(&ept_c.unbound, K_MSEC(GONE_MS)));
	zassert_true(k_uptime_get() - start >= MAX_RETX * RETX_MS, "gone after %lld ms",
		     k_uptime_get() - start);
	zexpect_equal(STAT(dev_c, disconnects), disconnects + 1);

	/* Plugged back in, its armed transfer and RDY are as they were */
	spi_link_unplug(false);
	zassert_ok(gpio_pin_set(gpio, RDY_PIN, 1));

	zassert_ok(k_sem_take(&ept_p.unbound, K_MSEC(1000)));
	zassert_ok(k_sem_take(&ept_c.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_p.bound, K_MSEC(BIND_TIMEOUT)));

	zassert_equal(ipc_service_send(&ept_c.ept, "back", 4), 4);
	zassert_ok(k_sem_take(&ept_c.received, K_MSEC(1000)));
	zexpect_mem_equal(ept_c.last, "back", 4);
}

/*
 * The peripheral stops answering while the controller only has a control message to send: the
 * bind of a new endpoint. It is timed like a payload, and ends the session
 */
ZTEST(ipc_link_spi, test_peripheral_gone_control)
{
	uint32_t disconnects;

	ept_p.echo = true;
	register_pair();
	wait_idle();
	disconnects = STAT(dev_c, disconnects);

	spi_link_unplug(true);
	zassert_ok(gpio_pin_set(gpio, RDY_PIN, 0));

	zassert_ok(ipc_service_register_endpoint(dev_c, &ept_c2.ept, &ept_c2.cfg));
	zassert_ok(k_sem_take(&ept_c.unbound, K_MSEC(GONE_MS)));
	zexpect_equal(STAT(dev_c, disconnects), disconnects + 1);

	spi_link_unplug(false);
	zassert_ok(gpio_pin_set(gpio, RDY_PIN, 1));

	zassert_ok(k_sem_take(&ept_p.unbound, K_MSEC(1000)));
	zassert_ok(k_sem_take(&ept_c.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_p.bound, K_MSEC(BIND_TIMEOUT)));
}

#endif /* CONFIG_TEST_IPC_LINK_SPI_MISMATCH */
