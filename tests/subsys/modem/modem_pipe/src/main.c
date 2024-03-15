/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*************************************************************************************************/
/*                                        Dependencies                                           */
/*************************************************************************************************/
#include <zephyr/kernel.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/atomic.h>
#include <string.h>

#define TEST_MODEM_PIPE_EVENT_OPENED_BIT        0
#define TEST_MODEM_PIPE_EVENT_TRANSMIT_IDLE_BIT 1
#define TEST_MODEM_PIPE_EVENT_RECEIVE_READY_BIT 2
#define TEST_MODEM_PIPE_EVENT_CLOSED_BIT        3
#define TEST_MODEM_PIPE_NOTIFY_TIMEOUT          K_MSEC(10)
#define TEST_MODEM_PIPE_WAIT_TIMEOUT            K_MSEC(20)

/*************************************************************************************************/
/*                                   Fake modem_pipe backend                                     */
/*************************************************************************************************/
struct modem_backend_fake {
	struct modem_pipe pipe;

	struct k_work_delayable opened_dwork;
	struct k_work_delayable transmit_idle_dwork;
	struct k_work_delayable closed_dwork;

	const uint8_t *transmit_buffer;
	size_t transmit_buffer_size;

	uint8_t *receive_buffer;
	size_t receive_buffer_size;

	uint8_t synchronous : 1;
	uint8_t open_called : 1;
	uint8_t transmit_called : 1;
	uint8_t receive_called : 1;
	uint8_t close_called : 1;
};

static void modem_backend_fake_opened_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_backend_fake *backend =
		CONTAINER_OF(dwork, struct modem_backend_fake, opened_dwork);

	modem_pipe_notify_opened(&backend->pipe);
}

static int modem_backend_fake_open(void *data)
{
	struct modem_backend_fake *backend = data;

	backend->open_called = true;

	if (backend->synchronous) {
		modem_pipe_notify_opened(&backend->pipe);
	} else {
		k_work_schedule(&backend->opened_dwork, TEST_MODEM_PIPE_NOTIFY_TIMEOUT);
	}

	return 0;
}

static void modem_backend_fake_transmit_idle_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_backend_fake *backend =
		CONTAINER_OF(dwork, struct modem_backend_fake, transmit_idle_dwork);

	modem_pipe_notify_transmit_idle(&backend->pipe);
}

static int modem_backend_fake_transmit(void *data, const uint8_t *buf, size_t size)
{
	struct modem_backend_fake *backend = data;

	backend->transmit_called = true;
	backend->transmit_buffer = buf;
	backend->transmit_buffer_size = size;

	if (backend->synchronous) {
		modem_pipe_notify_transmit_idle(&backend->pipe);
	} else {
		k_work_schedule(&backend->transmit_idle_dwork, TEST_MODEM_PIPE_NOTIFY_TIMEOUT);
	}

	return size;
}

static int modem_backend_fake_receive(void *data, uint8_t *buf, size_t size)
{
	struct modem_backend_fake *backend = data;

	backend->receive_called = true;
	backend->receive_buffer = buf;
	backend->receive_buffer_size = size;
	return size;
}

static void modem_backend_fake_closed_handler(struct k_work *item)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(item);
	struct modem_backend_fake *backend =
		CONTAINER_OF(dwork, struct modem_backend_fake, closed_dwork);

	modem_pipe_notify_closed(&backend->pipe);
}

static int modem_backend_fake_close(void *data)
{
	struct modem_backend_fake *backend = data;

	backend->close_called = true;

	if (backend->synchronous) {
		modem_pipe_notify_closed(&backend->pipe);
	} else {
		k_work_schedule(&backend->closed_dwork, TEST_MODEM_PIPE_NOTIFY_TIMEOUT);
	}

	return 0;
}

static struct modem_pipe_api modem_backend_fake_api = {
	.open = modem_backend_fake_open,
	.transmit = modem_backend_fake_transmit,
	.receive = modem_backend_fake_receive,
	.close = modem_backend_fake_close,
};

static struct modem_pipe *modem_backend_fake_init(struct modem_backend_fake *backend)
{
	k_work_init_delayable(&backend->opened_dwork,
			      modem_backend_fake_opened_handler);
	k_work_init_delayable(&backend->transmit_idle_dwork,
			      modem_backend_fake_transmit_idle_handler);
	k_work_init_delayable(&backend->closed_dwork,
			      modem_backend_fake_closed_handler);

	modem_pipe_init(&backend->pipe, backend, &modem_backend_fake_api);
	return &backend->pipe;
}

static void modem_backend_fake_reset(struct modem_backend_fake *backend)
{
	backend->transmit_buffer = NULL;
	backend->transmit_buffer_size = 0;
	backend->receive_buffer = NULL;
	backend->transmit_buffer_size = 0;
	backend->open_called = false;
	backend->transmit_called = false;
	backend->receive_called = false;
	backend->close_called = false;
}

static void modem_backend_fake_set_sync(struct modem_backend_fake *backend, bool sync)
{
	backend->synchronous = sync;
}

/*************************************************************************************************/
/*                                          Instances                                            */
/*************************************************************************************************/
static struct modem_backend_fake test_backend;
static struct modem_pipe *test_pipe;
static uint32_t test_user_data;
static atomic_t test_state;
static uint8_t test_buffer[4];
static size_t test_buffer_size = sizeof(test_buffer);

/*************************************************************************************************/
/*                                          Callbacks                                            */
/*************************************************************************************************/
static void modem_pipe_fake_handler(struct modem_pipe *pipe, enum modem_pipe_event event,
				    void *user_data)
{
	__ASSERT(pipe == test_pipe, "Incorrect pipe provided with callback");
	__ASSERT(user_data == (void *)&test_user_data, "Incorrect user data ptr");

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		atomic_set_bit(&test_state, TEST_MODEM_PIPE_EVENT_OPENED_BIT);
		break;

	case MODEM_PIPE_EVENT_RECEIVE_READY:
		atomic_set_bit(&test_state, TEST_MODEM_PIPE_EVENT_RECEIVE_READY_BIT);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		atomic_set_bit(&test_state, TEST_MODEM_PIPE_EVENT_TRANSMIT_IDLE_BIT);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		atomic_set_bit(&test_state, TEST_MODEM_PIPE_EVENT_CLOSED_BIT);
		break;
	}
}

static void test_reset(void)
{
	modem_backend_fake_reset(&test_backend);
	atomic_set(&test_state, 0);
}

static void *modem_backend_fake_setup(void)
{
	test_pipe = modem_backend_fake_init(&test_backend);
	return NULL;
}

static void modem_backend_fake_before(void *f)
{
	modem_backend_fake_set_sync(&test_backend, false);
	modem_pipe_attach(test_pipe, modem_pipe_fake_handler, &test_user_data);
	test_reset();
}

static void modem_backend_fake_after(void *f)
{
	__ASSERT(modem_pipe_close(test_pipe) == 0, "Failed to close pipe");
	modem_pipe_release(test_pipe);
}

/* Opening pipe shall raise events OPENED and TRANSMIT_IDLE */
static void test_pipe_open(void)
{
	zassert_ok(modem_pipe_open(test_pipe), "Failed to open pipe");
	zassert_true(test_backend.open_called, "open was not called");
	zassert_equal(atomic_get(&test_state),
		      BIT(TEST_MODEM_PIPE_EVENT_OPENED_BIT) |
		      BIT(TEST_MODEM_PIPE_EVENT_TRANSMIT_IDLE_BIT),
		      "Unexpected state %u", atomic_get(&test_state));
}

/* Re-opening pipe shall have no effect */
static void test_pipe_reopen(void)
{
	zassert_ok(modem_pipe_open(test_pipe), "Failed to re-open pipe");
	zassert_false(test_backend.open_called, "open was called");
	zassert_equal(atomic_get(&test_state), 0,
		      "Unexpected state %u", atomic_get(&test_state));
}

/* Closing pipe shall raise event CLOSED */
static void test_pipe_close(void)
{
	zassert_ok(modem_pipe_close(test_pipe), "Failed to close pipe");
	zassert_true(test_backend.close_called, "close was not called");
	zassert_equal(atomic_get(&test_state), BIT(TEST_MODEM_PIPE_EVENT_CLOSED_BIT),
		      "Unexpected state %u", atomic_get(&test_state));
}

/* Re-closing pipe shall have no effect */
static void test_pipe_reclose(void)
{
	zassert_ok(modem_pipe_close(test_pipe), "Failed to re-close pipe");
	zassert_false(test_backend.close_called, "close was called");
	zassert_equal(atomic_get(&test_state), 0,
		      "Unexpected state %u", atomic_get(&test_state));
}

static void test_pipe_async_transmit(void)
{
	zassert_equal(modem_pipe_transmit(test_pipe, test_buffer, test_buffer_size),
		      test_buffer_size, "Failed to transmit");
	zassert_true(test_backend.transmit_called, "transmit was not called");
	zassert_equal(test_backend.transmit_buffer, test_buffer, "Incorrect buffer");
	zassert_equal(test_backend.transmit_buffer_size, test_buffer_size,
		      "Incorrect buffer size");
	zassert_equal(atomic_get(&test_state), 0, "Unexpected state %u",
		      atomic_get(&test_state));
	k_sleep(TEST_MODEM_PIPE_WAIT_TIMEOUT);
	zassert_equal(atomic_get(&test_state), BIT(TEST_MODEM_PIPE_EVENT_TRANSMIT_IDLE_BIT),
		      "Unexpected state %u", atomic_get(&test_state));
}

static void test_pipe_sync_transmit(void)
{
	zassert_equal(modem_pipe_transmit(test_pipe, test_buffer, test_buffer_size),
		      test_buffer_size, "Failed to transmit");
	zassert_true(test_backend.transmit_called, "transmit was not called");
	zassert_equal(test_backend.transmit_buffer, test_buffer, "Incorrect buffer");
	zassert_equal(test_backend.transmit_buffer_size, test_buffer_size,
		      "Incorrect buffer size");
	zassert_equal(atomic_get(&test_state), BIT(TEST_MODEM_PIPE_EVENT_TRANSMIT_IDLE_BIT),
		      "Unexpected state %u", atomic_get(&test_state));
}

static void test_pipe_attach_receive_not_ready_transmit_idle(void)
{
	modem_pipe_attach(test_pipe, modem_pipe_fake_handler, &test_user_data);
	zassert_equal(atomic_get(&test_state), BIT(TEST_MODEM_PIPE_EVENT_TRANSMIT_IDLE_BIT),
		      "Unexpected state %u", atomic_get(&test_state));
}

static void test_pipe_attach_receive_ready_transmit_idle(void)
{
	modem_pipe_attach(test_pipe, modem_pipe_fake_handler, &test_user_data);
	zassert_equal(atomic_get(&test_state),
		      BIT(TEST_MODEM_PIPE_EVENT_TRANSMIT_IDLE_BIT) |
		      BIT(TEST_MODEM_PIPE_EVENT_RECEIVE_READY_BIT),
		      "Unexpected state %u", atomic_get(&test_state));
}

static void test_pipe_receive(void)
{
	zassert_equal(modem_pipe_receive(test_pipe, test_buffer, test_buffer_size),
		      test_buffer_size, "Failed to receive");
	zassert_true(test_backend.receive_called, "receive was not called");
	zassert_equal(test_backend.receive_buffer, test_buffer, "Incorrect buffer");
	zassert_equal(test_backend.receive_buffer_size, test_buffer_size,
		      "Incorrect buffer size");
	zassert_equal(atomic_get(&test_state), 0, "Unexpected state %u",
		      atomic_get(&test_state));
}

static void test_pipe_notify_receive_ready(void)
{
	modem_pipe_notify_receive_ready(test_pipe);
	zassert_equal(atomic_get(&test_state), BIT(TEST_MODEM_PIPE_EVENT_RECEIVE_READY_BIT),
		      "Unexpected state %u", atomic_get(&test_state));
}

ZTEST(modem_pipe, test_async_open_close)
{
	test_pipe_open();
	test_reset();
	test_pipe_reopen();
	test_reset();
	test_pipe_close();
	test_reset();
	test_pipe_reclose();
}

ZTEST(modem_pipe, test_sync_open_close)
{
	modem_backend_fake_set_sync(&test_backend, true);
	test_pipe_open();
	test_reset();
	test_pipe_reopen();
	test_reset();
	test_pipe_close();
	test_reset();
	test_pipe_reclose();
}

ZTEST(modem_pipe, test_async_transmit)
{
	test_pipe_open();
	test_reset();
	test_pipe_async_transmit();
}

ZTEST(modem_pipe, test_sync_transmit)
{
	modem_backend_fake_set_sync(&test_backend, true);
	test_pipe_open();
	test_reset();
	test_pipe_sync_transmit();
}

ZTEST(modem_pipe, test_attach)
{
	test_pipe_open();

	/*
	 * Attaching pipe shall reinvoke TRANSMIT IDLE, but not RECEIVE READY as
	 * receive is not ready.
	 */
	test_reset();
	test_pipe_attach_receive_not_ready_transmit_idle();

	/*
	 * Notify receive ready and expect receive ready to be re-invoked every
	 * time the pipe is attached to.
	 */
	test_reset();
	test_pipe_notify_receive_ready();
	test_reset();
	test_pipe_attach_receive_ready_transmit_idle();
	test_reset();
	test_pipe_attach_receive_ready_transmit_idle();

	/*
	 * Receiving data from the pipe shall clear the receive ready state, stopping
	 * the invocation of receive ready on attach.
	 */
	test_reset();
	test_pipe_receive();
	test_reset();
	test_pipe_attach_receive_not_ready_transmit_idle();
}

ZTEST_SUITE(modem_pipe, NULL, modem_backend_fake_setup, modem_backend_fake_before,
	    modem_backend_fake_after, NULL);
