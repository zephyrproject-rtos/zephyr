/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/lwm2m.h>

#include "lwm2m_engine.h"
#include "lwm2m_registry.h"

#define PUSH_TIMEOUT_MS (CONFIG_LWM2M_FIRMWARE_UPDATE_PUSH_TIMEOUT * MSEC_PER_SEC)

static uint8_t block[16];
static int write_calls;
static int cancel_calls;

static int write_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
		    uint16_t data_len, bool last_block, size_t total_size, size_t offset)
{
	write_calls++;
	return 0;
}

static int cancel_cb(uint16_t obj_inst_id)
{
	cancel_calls++;
	return 0;
}

static uint8_t get_state(void)
{
	uint8_t value;

	zassert_ok(lwm2m_get_u8(&LWM2M_OBJ(5, 0, 3), &value));
	return value;
}

static uint8_t get_result(void)
{
	uint8_t value;

	zassert_ok(lwm2m_get_u8(&LWM2M_OBJ(5, 0, 5), &value));
	return value;
}

/* Drive the resource callbacks the way the engine does for a write request */
static int package_write(uint8_t *data, uint16_t len, bool last_block, size_t offset)
{
	struct lwm2m_engine_res *res = lwm2m_engine_get_res(&LWM2M_OBJ(5, 0, 0));

	zassert_not_null(res);
	zassert_not_null(res->post_write_cb);

	return res->post_write_cb(0, 0, 0, data, len, last_block, 0, offset);
}

/* The engine lets the pre-write callback pick the buffer, then stores the
 * string with its terminator and calls the post-write callback with that length.
 */
static int package_uri_write(const char *uri)
{
	struct lwm2m_engine_res *res = lwm2m_engine_get_res(&LWM2M_OBJ(5, 0, 1));
	size_t buf_len;
	char *buf;

	zassert_not_null(res);
	zassert_not_null(res->pre_write_cb);
	zassert_not_null(res->post_write_cb);

	buf = res->pre_write_cb(0, 1, 0, &buf_len);
	zassert_not_null(buf);
	zassert_true(strlen(uri) < buf_len);
	strcpy(buf, uri);

	return res->post_write_cb(0, 1, 0, (uint8_t *)buf, strlen(uri) + 1, true, 0, 0);
}

/* What a reader of the resource gets, through the object's read callback */
static const char *stored_uri(void)
{
	static char uri[CONFIG_LWM2M_SWMGMT_PACKAGE_URI_LEN];

	zassert_ok(lwm2m_get_string(&LWM2M_OBJ(5, 0, 1), uri, sizeof(uri)));
	return uri;
}

static void push_first_block(void)
{
	int writes = write_calls;

	zassert_ok(package_write(block, sizeof(block), false, 0));
	zassert_equal(get_state(), STATE_DOWNLOADING);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(write_calls, writes + 1);
}

static void *suite_setup(void)
{
	lwm2m_firmware_set_write_cb(write_cb);
	lwm2m_firmware_set_cancel_cb(cancel_cb);

	return NULL;
}

static void test_before(void *fixture)
{
	/* Back to Idle whatever the previous case left behind, without callbacks */
	lwm2m_firmware_set_update_result(RESULT_DEFAULT);

	zassert_equal(get_state(), STATE_IDLE);
	write_calls = 0;
	cancel_calls = 0;
}

ZTEST_SUITE(lwm2m_firmware_update, NULL, suite_setup, test_before, NULL, NULL);

ZTEST(lwm2m_firmware_update, test_push_timeout)
{
	push_first_block();

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS / 2));
	zassert_ok(package_write(block, sizeof(block), false, sizeof(block)));
	zassert_equal(write_calls, 2);

	/* The timeout counts from the last block */
	k_sleep(K_MSEC(PUSH_TIMEOUT_MS * 3 / 4));
	zassert_equal(get_state(), STATE_DOWNLOADING);
	zassert_equal(cancel_calls, 0);

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS / 2));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_CONNECTION_LOST);
	zassert_equal(cancel_calls, 1);

	/* A new push is accepted after the failure */
	push_first_block();
}

ZTEST(lwm2m_firmware_update, test_push_late_block_after_timeout)
{
	push_first_block();

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS * 3 / 2));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_CONNECTION_LOST);

	/* The server was only slow: its next block must not start a download mid-image */
	zassert_equal(package_write(block, sizeof(block), false, sizeof(block)), -EFAULT);
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(write_calls, 1);
}

ZTEST(lwm2m_firmware_update, test_push_completes)
{
	push_first_block();

	zassert_ok(package_write(block, sizeof(block), true, sizeof(block)));
	zassert_equal(get_state(), STATE_DOWNLOADED);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(write_calls, 2);

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS * 2));
	zassert_equal(get_state(), STATE_DOWNLOADED);
	zassert_equal(cancel_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_push_reset_by_null_package)
{
	uint8_t nul = '\0';

	push_first_block();

	zassert_ok(package_write(&nul, 1, true, 0));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 1);
	zassert_equal(write_calls, 1);

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS * 2));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(cancel_calls, 1);
}

ZTEST(lwm2m_firmware_update, test_push_reset_by_empty_package)
{
	push_first_block();

	zassert_ok(package_write(block, 0, true, 0));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 1);
	zassert_equal(write_calls, 1);
}

ZTEST(lwm2m_firmware_update, test_push_last_block_null_byte_is_data)
{
	uint8_t nul = '\0';

	push_first_block();

	zassert_ok(package_write(&nul, 1, true, sizeof(block)));
	zassert_equal(get_state(), STATE_DOWNLOADED);
	zassert_equal(write_calls, 2);
	zassert_equal(cancel_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_push_reset_by_empty_uri)
{
	push_first_block();

	zassert_ok(package_uri_write(""));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 1);
	zassert_equal(strlen(stored_uri()), 0);

	k_sleep(K_MSEC(PUSH_TIMEOUT_MS * 2));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(cancel_calls, 1);
}

ZTEST(lwm2m_firmware_update, test_uri_rejected_while_downloading)
{
	push_first_block();

	zassert_equal(package_uri_write("coap://[::1]/fw.bin"), -EPERM);
	zassert_equal(get_state(), STATE_DOWNLOADING);
	zassert_equal(cancel_calls, 0);
	/* The rejected URI never reached the resource */
	zassert_equal(strlen(stored_uri()), 0);
}

ZTEST(lwm2m_firmware_update, test_reset_rejected_while_updating)
{
	uint8_t nul = '\0';

	push_first_block();
	zassert_ok(package_write(block, sizeof(block), true, sizeof(block)));
	zassert_equal(get_state(), STATE_DOWNLOADED);
	lwm2m_firmware_set_update_state(STATE_UPDATING);

	zassert_equal(package_write(&nul, 1, true, 0), -EPERM);
	zassert_equal(package_uri_write(""), -EPERM);
	zassert_equal(package_uri_write("coap://[::1]/fw.bin"), -EPERM);

	zassert_equal(get_state(), STATE_UPDATING);
	zassert_equal(write_calls, 2);
	zassert_equal(cancel_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_writes_rejected_during_pull)
{
	uint8_t nul = '\0';

	/* A pull is Downloading without a push in progress */
	lwm2m_firmware_set_update_state(STATE_DOWNLOADING);

	zassert_equal(package_write(block, sizeof(block), false, 0), -EPERM);
	zassert_equal(package_write(&nul, 1, true, 0), -EPERM);
	zassert_equal(package_write(block, 0, true, 0), -EPERM);
	zassert_equal(package_uri_write(""), -EPERM);
	zassert_equal(package_uri_write("coap://[::1]/fw.bin"), -EPERM);

	zassert_equal(get_state(), STATE_DOWNLOADING);
	zassert_equal(write_calls, 0);
	zassert_equal(cancel_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_pull_refused_without_write_cb)
{
	/* The pull cannot start without a write callback; the object reports it and stays usable */
	lwm2m_firmware_set_write_cb(NULL);

	zassert_ok(package_uri_write("coap://[::1]/fw.bin"));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_UPDATE_FAILED);

	zassert_ok(package_uri_write("coap://[::1]/fw.bin"));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_UPDATE_FAILED);

	lwm2m_firmware_set_write_cb(write_cb);
	push_first_block();
}

ZTEST(lwm2m_firmware_update, test_uri_rejected_when_downloaded)
{
	push_first_block();
	zassert_ok(package_write(block, sizeof(block), true, sizeof(block)));
	zassert_equal(get_state(), STATE_DOWNLOADED);

	zassert_equal(package_uri_write("coap://[::1]/fw.bin"), -EPERM);
	zassert_equal(get_state(), STATE_DOWNLOADED);
}

ZTEST(lwm2m_firmware_update, test_reset_when_idle)
{
	uint8_t nul = '\0';

	lwm2m_set_u8(&LWM2M_OBJ(5, 0, 5), RESULT_CONNECTION_LOST);

	zassert_ok(package_write(&nul, 1, true, 0));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(write_calls, 0);
	zassert_equal(cancel_calls, 0);

	lwm2m_set_u8(&LWM2M_OBJ(5, 0, 5), RESULT_CONNECTION_LOST);

	zassert_ok(package_uri_write(""));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 0);
}

ZTEST(lwm2m_firmware_update, test_downloaded_reset)
{
	uint8_t nul = '\0';

	push_first_block();
	zassert_ok(package_write(block, sizeof(block), true, sizeof(block)));
	zassert_equal(get_state(), STATE_DOWNLOADED);

	zassert_ok(package_write(&nul, 1, true, 0));
	zassert_equal(get_state(), STATE_IDLE);
	zassert_equal(get_result(), RESULT_DEFAULT);
	zassert_equal(cancel_calls, 1);
	zassert_equal(write_calls, 2);
}
