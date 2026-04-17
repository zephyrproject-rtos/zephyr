/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * suite_setup opens ipc_echo and pre-registers one echo endpoint for every
 * endpoint name used by the tests, so that each test has a ready remote peer
 * on ipc_backend without further echo-side setup.
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>

#define IPC_NODE      DT_NODELABEL(ipc_backend)
#define IPC_ECHO_NODE DT_NODELABEL(ipc_echo)

static const struct device *ipc_dev = DEVICE_DT_GET(IPC_NODE);
static const struct device *ipc_echo_dev = DEVICE_DT_GET(IPC_ECHO_NODE);

/*
 * Echo endpoint — registered on ipc_echo.
 * Reflects any received data back to the sender.
 */
struct echo_ept {
	struct ipc_ept ept;
	struct ipc_ept_cfg cfg;

	struct k_sem bound_sem;
};

struct test_ept {
	struct ipc_ept ept;
	struct ipc_ept_cfg cfg;

	struct k_sem bound_sem;
	struct k_sem unbound_sem;
	struct k_sem received_sem;

	uint8_t rx_buf[256];
	size_t rx_len;
};

static void echo_bound_cb(void *priv)
{
	struct echo_ept *echo = priv;

	k_sem_give(&echo->bound_sem);
}

static void echo_received_cb(const void *data, size_t len, void *priv)
{
	struct echo_ept *echo = priv;
	int ret;

	ret = ipc_service_send(&echo->ept, data, len);
	zassert_equal(ret, len, "ipc_service_send() failed in echo callback: %d", ret);
}

static int setup_echo_ept(struct echo_ept *echo, const char *name)
{
	echo->cfg = (struct ipc_ept_cfg){
		.name = name,
		.cb = {
			.bound = echo_bound_cb,
			.received = echo_received_cb,
		},
		.priv = echo,
	};

	k_sem_init(&echo->bound_sem, 0, 1);
	return ipc_service_register_endpoint(ipc_echo_dev, &echo->ept, &echo->cfg);
}

/* One echo endpoint per endpoint name used across all tests */
static struct echo_ept echo_ep_a;
static struct echo_ept echo_ep_b;

static void bound_cb(void *priv)
{
	struct test_ept *ept = priv;

	k_sem_give(&ept->bound_sem);
}

static void unbound_cb(void *priv)
{
	struct test_ept *ept = priv;

	k_sem_give(&ept->unbound_sem);
}

static void received_cb(const void *data, size_t len, void *priv)
{
	struct test_ept *ept = priv;

	if (len <= sizeof(ept->rx_buf)) {
		memcpy(ept->rx_buf, data, len);
		ept->rx_len = len;
	}
	k_sem_give(&ept->received_sem);
}

static void setup_test_ept(struct test_ept *ept, const char *name)
{
	memset(ept, 0, sizeof(*ept));
	ept->cfg = (struct ipc_ept_cfg){
		.name = name,
		.cb = {
			.bound = bound_cb,
			.unbound = unbound_cb,
			.received = received_cb,
		},
		.priv = ept,
	};

	k_sem_init(&ept->bound_sem, 0, 1);
	k_sem_init(&ept->unbound_sem, 0, 1);
	k_sem_init(&ept->received_sem, 0, 1);
}

static void *suite_setup(void)
{
	int ret;

	zassert_true(device_is_ready(ipc_echo_dev), "IPC echo device not ready");

	ret = ipc_service_open_instance(ipc_echo_dev);
	zassert_ok(ret, "Failed to open IPC echo instance: %d", ret);

	ret = setup_echo_ept(&echo_ep_a, "ep_a");
	zassert_ok(ret, "Failed to register echo endpoint a: %d", ret);

	ret = setup_echo_ept(&echo_ep_b, "ep_b");
	zassert_ok(ret, "Failed to register echo endpoint b: %d", ret);

	zassert_true(device_is_ready(ipc_dev), "IPC device not ready");

	ret = ipc_service_open_instance(ipc_dev);
	zassert_ok(ret, "Failed to open IPC instance: %d", ret);

	return NULL;
}

ZTEST(ipc_service_backend, test_register_endpoint_and_bind)
{
	struct test_ept test;
	int ret;

	setup_test_ept(&test, "ep_a");

	ret = ipc_service_register_endpoint(ipc_dev, &test.ept, &test.cfg);
	zassert_ok(ret, "Register endpoint failed: %d", ret);

	ret = k_sem_take(&test.bound_sem, K_MSEC(2000));
	zassert_ok(ret, "Endpoint did not bind within timeout");

	ret = ipc_service_deregister_endpoint(&test.ept);
	zassert_ok(ret, "De-register ep failed: %d", ret);

	ret = k_sem_take(&test.unbound_sem, K_MSEC(2000));
	zassert_ok(ret, "Endpoint did not unbind within timeout");
}

ZTEST(ipc_service_backend, test_send_receive)
{
	struct test_ept test;
	int ret;

	setup_test_ept(&test, "ep_a");

	ret = ipc_service_register_endpoint(ipc_dev, &test.ept, &test.cfg);
	zassert_ok(ret, "Register endpoint failed: %d", ret);

	ret = k_sem_take(&test.bound_sem, K_MSEC(2000));
	zassert_ok(ret, "Endpoint did not bind");

	/* Send test data */
	uint8_t tx_data[] = {0xAA, 0xBB, 0xCC};

	ret = ipc_service_send(&test.ept, tx_data, sizeof(tx_data));
	zassert_true(ret >= 0, "Send failed: %d", ret);

	/* Wait for echo receive */
	ret = k_sem_take(&test.received_sem, K_MSEC(2000));
	zassert_ok(ret, "Did not receive echoed data");

	zassert_equal(test.rx_len, sizeof(tx_data), "Received length mismatch: %zu != %zu",
		      test.rx_len, sizeof(tx_data));
	zassert_mem_equal(test.rx_buf, tx_data, sizeof(tx_data), "Received data mismatch");

	ret = ipc_service_deregister_endpoint(&test.ept);
	zassert_ok(ret, "De-register ep failed: %d", ret);
}

ZTEST(ipc_service_backend, test_multi_endpoint)
{
	struct test_ept test_a;
	struct test_ept test_b;
	int ret;

	setup_test_ept(&test_a, "ep_a");
	setup_test_ept(&test_b, "ep_b");

	ret = ipc_service_register_endpoint(ipc_dev, &test_a.ept, &test_a.cfg);
	zassert_ok(ret, "Register ep_a failed: %d", ret);

	ret = ipc_service_register_endpoint(ipc_dev, &test_b.ept, &test_b.cfg);
	zassert_ok(ret, "Register ep_b failed: %d", ret);

	ret = k_sem_take(&test_a.bound_sem, K_MSEC(2000));
	zassert_ok(ret, "ep_a did not bind");

	ret = k_sem_take(&test_b.bound_sem, K_MSEC(2000));
	zassert_ok(ret, "ep_b did not bind");

	/* Send on ep_a */
	uint8_t data_a[] = {0x11, 0x22};

	ret = ipc_service_send(&test_a.ept, data_a, sizeof(data_a));
	zassert_true(ret >= 0, "Send on ep_a failed: %d", ret);

	ret = k_sem_take(&test_a.received_sem, K_MSEC(2000));
	zassert_ok(ret, "ep_a did not receive");

	zassert_equal(test_a.rx_len, sizeof(data_a), "ep_a length mismatch");
	zassert_mem_equal(test_a.rx_buf, data_a, sizeof(data_a), "ep_a data mismatch");

	/* Send on ep_b */
	uint8_t data_b[] = {0x33, 0x44, 0x55};

	ret = ipc_service_send(&test_b.ept, data_b, sizeof(data_b));
	zassert_true(ret >= 0, "Send on ep_b failed: %d", ret);

	ret = k_sem_take(&test_b.received_sem, K_MSEC(2000));
	zassert_ok(ret, "ep_b did not receive");

	zassert_equal(test_b.rx_len, sizeof(data_b), "ep_b length mismatch");
	zassert_mem_equal(test_b.rx_buf, data_b, sizeof(data_b), "ep_b data mismatch");

	ret = ipc_service_deregister_endpoint(&test_a.ept);
	zassert_ok(ret, "De-register ep_a failed: %d", ret);

	ret = ipc_service_deregister_endpoint(&test_b.ept);
	zassert_ok(ret, "De-register ep_b failed: %d", ret);
}

/*
 * CONFIG_IPC_SERVICE_BACKEND_SERIAL_FRAME_BUF_SIZE is the same on both
 * sides in this test setup.  A payload of that size needs frame_buf_size
 * + 2 bytes in the remote's decode buffer (cmd_id + ept_id overhead),
 * which exceeds the remote's capacity by 2 bytes.
 */
static uint8_t oversized[CONFIG_IPC_SERVICE_BACKEND_SERIAL_FRAME_BUF_SIZE];

ZTEST(ipc_service_backend, test_send_oversized)
{
	struct test_ept test;
	int ret;

	setup_test_ept(&test, "ep_a");

	ret = ipc_service_register_endpoint(ipc_dev, &test.ept, &test.cfg);
	zassert_ok(ret, "Register endpoint failed: %d", ret);

	ret = k_sem_take(&test.bound_sem, K_MSEC(2000));
	zassert_ok(ret, "Endpoint did not bind");

	memset(oversized, 0xAB, sizeof(oversized));

	ret = ipc_service_send(&test.ept, oversized, sizeof(oversized));
	zassert_equal(ret, -EMSGSIZE, "Expected -EMSGSIZE for oversized payload, got %d", ret);

	ret = ipc_service_deregister_endpoint(&test.ept);
	zassert_ok(ret, "De-register ep failed: %d", ret);
}

ZTEST_SUITE(ipc_service_backend, NULL, suite_setup, NULL, NULL, NULL);
