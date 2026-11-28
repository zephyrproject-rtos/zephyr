/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell_mqtt.h>
#include <zephyr/ztest.h>

#include <string.h>

struct shell_mqtt_backend_fixture {
	const struct shell *sh;
	struct shell_mqtt *ctx;
	struct k_timer watchdog;
	struct k_timer drain;
	struct k_timer disconnect;
	const uint8_t *payload;
	size_t payload_left;
	atomic_t abort_count;
	atomic_t timed_out;
	atomic_t drained;
};

static struct shell_mqtt_backend_fixture backend;

bool shell_mqtt_get_devid(char *id, int id_max_len)
{
	if (id_max_len < sizeof("test")) {
		return false;
	}
	memcpy(id, "test", sizeof("test"));
	return true;
}

int __wrap_mqtt_connect(struct mqtt_client *client)
{
	ARG_UNUSED(client);
	return -ECONNREFUSED;
}

int __wrap_mqtt_abort(struct mqtt_client *client)
{
	const struct mqtt_evt evt = {.type = MQTT_EVT_DISCONNECT};

	atomic_inc(&backend.abort_count);
	client->evt_cb(client, &evt);
	return 0;
}

int __wrap_mqtt_read_publish_payload_blocking(struct mqtt_client *client, void *buffer,
					      size_t length)
{
	size_t size = MIN(length, backend.payload_left);

	if (size == 0U) {
		const struct mqtt_evt evt = {
			.type = MQTT_EVT_DISCONNECT,
			.result = -ENOTCONN,
		};

		client->evt_cb(client, &evt);
		return -ENOTCONN;
	}
	memcpy(buffer, backend.payload, size);
	backend.payload += size;
	backend.payload_left -= size;
	return (int)size;
}

static void release_stalled_handler(struct k_timer *timer)
{
	struct shell_mqtt_backend_fixture *f =
		CONTAINER_OF(timer, struct shell_mqtt_backend_fixture, watchdog);
	uint8_t data[CONFIG_SHELL_MQTT_RX_BUF_SIZE];
	size_t count;
	int ret;

	/* Release the old, unbounded loop so a regression fails without hanging the suite. */
	atomic_set(&f->timed_out, 1);
	ret = f->sh->iface->api->read(f->sh->iface, data, sizeof(data), &count);
	if (ret != 0) {
		atomic_set(&f->timed_out, ret);
	}
}

static void drain_rx(struct k_timer *timer)
{
	struct shell_mqtt_backend_fixture *f =
		CONTAINER_OF(timer, struct shell_mqtt_backend_fixture, drain);
	uint8_t data[4];
	size_t count;
	int ret;

	ret = f->sh->iface->api->read(f->sh->iface, data, sizeof(data), &count);
	atomic_set(&f->drained, ret == 0 ? (atomic_val_t)count : ret);
}

static void disconnect_network(struct k_timer *timer)
{
	struct shell_mqtt_backend_fixture *f =
		CONTAINER_OF(timer, struct shell_mqtt_backend_fixture, disconnect);

	f->ctx->mgmt_cb.handler(&f->ctx->mgmt_cb, NET_EVENT_L4_DISCONNECTED, NULL);
}

static void cancel_backend_work(struct shell_mqtt *ctx)
{
	struct k_work_sync sync;

	(void)k_work_cancel_delayable_sync(&ctx->connect_dwork, &sync);
	(void)k_work_cancel_delayable_sync(&ctx->subscribe_dwork, &sync);
	(void)k_work_cancel_delayable_sync(&ctx->process_dwork, &sync);
	(void)k_work_cancel_delayable_sync(&ctx->publish_dwork, &sync);
	(void)k_work_flush(&ctx->net_disconnected_work, &sync);
}

static void *mqtt_setup(void)
{
	struct shell_mqtt_backend_fixture *f = &backend;

	f->sh = shell_backend_mqtt_get_ptr();
	zassert_true(WAIT_FOR(shell_ready(f->sh), 1000000, k_msleep(1)), "Shell not ready");
	f->ctx = f->sh->iface->ctx;
	k_thread_suspend(f->sh->ctx->tid);
	net_mgmt_del_event_callback(&f->ctx->mgmt_cb);
	f->ctx->network_state = SHELL_MQTT_NETWORK_DISCONNECTED;
	cancel_backend_work(f->ctx);

	/* The failed, wrapped connection attempt installs the real MQTT event callback. */
	f->ctx->network_state = SHELL_MQTT_NETWORK_CONNECTED;
	f->ctx->connect_dwork.work.handler(&f->ctx->connect_dwork.work);
	f->ctx->network_state = SHELL_MQTT_NETWORK_DISCONNECTED;
	cancel_backend_work(f->ctx);
	zassert_not_null(f->ctx->mqtt_cli.evt_cb, "MQTT callback was not installed");

	k_timer_init(&f->watchdog, release_stalled_handler, NULL);
	k_timer_init(&f->drain, drain_rx, NULL);
	k_timer_init(&f->disconnect, disconnect_network, NULL);
	return f;
}

static void mqtt_before(void *data)
{
	struct shell_mqtt_backend_fixture *f = data;

	f->ctx->network_state = SHELL_MQTT_NETWORK_CONNECTED;
	f->ctx->transport_state = SHELL_MQTT_TRANSPORT_CONNECTED;
	f->ctx->subscribe_state = SHELL_MQTT_SUBSCRIBED;
	atomic_set(&f->abort_count, 0);
	atomic_set(&f->timed_out, 0);
	atomic_set(&f->drained, 0);
	ring_buf_reset(&f->ctx->rx_rb);
}

static void mqtt_after(void *data)
{
	struct shell_mqtt_backend_fixture *f = data;
	struct k_work_sync sync;

	k_timer_stop(&f->watchdog);
	k_timer_stop(&f->drain);
	k_timer_stop(&f->disconnect);
	(void)k_work_flush(&f->ctx->net_disconnected_work, &sync);
}

static void mqtt_teardown(void *data)
{
	struct shell_mqtt_backend_fixture *f = data;

	f->ctx->network_state = SHELL_MQTT_NETWORK_DISCONNECTED;
	ring_buf_reset(&f->ctx->rx_rb);
	k_thread_resume(f->sh->ctx->tid);
}

static void receive_publish(struct shell_mqtt_backend_fixture *f, const uint8_t *payload,
			    size_t length)
{
	struct mqtt_evt evt = {
		.type = MQTT_EVT_PUBLISH,
		.param.publish.message.payload.len = length,
	};

	f->payload = payload;
	f->payload_left = length;
	zassert_equal(k_mutex_lock(&f->ctx->lock, K_FOREVER), 0);
	k_timer_start(&f->watchdog, K_SECONDS(2), K_NO_WAIT);
	f->ctx->mqtt_cli.evt_cb(&f->ctx->mqtt_cli, &evt);
	k_timer_stop(&f->watchdog);
	zassert_equal(k_mutex_unlock(&f->ctx->lock), 0);
	zassert_false(atomic_get(&f->timed_out), "Publish handler waited for the watchdog");
	zassert_equal(f->payload_left, 0U, "Payload was not consumed");
}

static void check_received(struct shell_mqtt_backend_fixture *f, const uint8_t *payload,
			   size_t length)
{
	uint8_t rx[CONFIG_SHELL_MQTT_RX_BUF_SIZE];
	const uint8_t crlf[] = {'\r', '\n'};
	size_t count;

	zassert_equal(f->sh->iface->api->read(f->sh->iface, rx, sizeof(rx), &count), 0);
	zassert_equal(count, length + sizeof(crlf), "Unexpected input length (%zu)", count);
	zassert_mem_equal(rx, payload, length);
	zassert_mem_equal(rx + length, crlf, sizeof(crlf));
	zassert_equal(atomic_get(&f->abort_count), 0);
}

ZTEST_F(shell_mqtt_backend, test_publish_terminator)
{
	static const uint8_t payload[] = "help";

	receive_publish(fixture, payload, sizeof(payload) - 1U);
	check_received(fixture, payload, sizeof(payload) - 1U);
}

ZTEST_F(shell_mqtt_backend, test_exact_terminator_space)
{
	uint8_t payload[CONFIG_SHELL_MQTT_RX_BUF_SIZE - 2U];

	memset(payload, 'a', sizeof(payload));
	receive_publish(fixture, payload, sizeof(payload));
	check_received(fixture, payload, sizeof(payload));
}

ZTEST_F(shell_mqtt_backend, test_full_rx_without_consumer)
{
	uint8_t payload[CONFIG_SHELL_MQTT_RX_BUF_SIZE];

	memset(payload, 'a', sizeof(payload));
	receive_publish(fixture, payload, sizeof(payload));
	zassert_true(atomic_get(&fixture->abort_count) > 0);
	zassert_equal(fixture->ctx->transport_state, SHELL_MQTT_TRANSPORT_DISCONNECTED);
	zassert_equal(ring_buf_size_get(&fixture->ctx->rx_rb), 0U);
}

ZTEST_F(shell_mqtt_backend, test_one_byte_free)
{
	uint8_t payload[CONFIG_SHELL_MQTT_RX_BUF_SIZE - 1U];

	memset(payload, 'a', sizeof(payload));
	receive_publish(fixture, payload, sizeof(payload));
	zassert_true(atomic_get(&fixture->abort_count) > 0);
	zassert_equal(ring_buf_size_get(&fixture->ctx->rx_rb), 0U);
}

ZTEST_F(shell_mqtt_backend, test_consumer_frees_space)
{
	uint8_t payload[CONFIG_SHELL_MQTT_RX_BUF_SIZE];

	memset(payload, 'a', sizeof(payload));
	k_timer_start(&fixture->drain, K_MSEC(200), K_NO_WAIT);
	receive_publish(fixture, payload, sizeof(payload));
	zassert_equal(atomic_get(&fixture->drained), 4);
	check_received(fixture, payload + 4, sizeof(payload) - 4U);
}

ZTEST_F(shell_mqtt_backend, test_network_disconnect_while_full)
{
	uint8_t payload[CONFIG_SHELL_MQTT_RX_BUF_SIZE];
	int64_t start = k_uptime_get();

	memset(payload, 'a', sizeof(payload));
	k_timer_start(&fixture->disconnect, K_MSEC(200), K_NO_WAIT);
	receive_publish(fixture, payload, sizeof(payload));
	zassert_true(k_uptime_get() - start < 500, "Disconnected handler did not return promptly");
	zassert_true(atomic_get(&fixture->abort_count) > 0);
	zassert_equal(ring_buf_size_get(&fixture->ctx->rx_rb), 0U);
}

ZTEST_SUITE(shell_mqtt_backend, NULL, mqtt_setup, mqtt_before, mqtt_after, mqtt_teardown);
