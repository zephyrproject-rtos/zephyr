/*
 * Copyright (c) 2022 Trackunit Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

#include <zephyr/modem/ubx.h>
#include <zephyr/modem/ubx/protocol.h>
#include <modem_backend_mock.h>

static struct modem_ubx cmd;

static uint32_t cmd_user_data = 0x145212;
static uint8_t cmd_receive_buf[128];

static uint8_t cmd_response[128];

static struct modem_backend_mock mock;
static uint8_t mock_rx_buf[128];
static uint8_t mock_tx_buf[128];
static struct modem_pipe *mock_pipe;

#define MODEM_UBX_UTEST_ON_NAK_RECEIVED_BIT	(0)
#define MODEM_UBX_UTEST_ON_ACK_RECEIVED_BIT	(1)

static atomic_t callback_called;

static void on_nak_received(struct modem_ubx *ubx, const struct ubx_frame *frame, size_t len,
			    void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_UBX_UTEST_ON_NAK_RECEIVED_BIT);
}

static void on_ack_received(struct modem_ubx *ubx, const struct ubx_frame *frame, size_t len,
			    void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_UBX_UTEST_ON_ACK_RECEIVED_BIT);
}

MODEM_UBX_MATCH_ARRAY_DEFINE(unsol_matches,
	MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_ACK, UBX_MSG_ID_ACK, on_ack_received),
	MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_ACK, UBX_MSG_ID_NAK, on_nak_received)
);

static struct ubx_frame test_req = UBX_FRAME_ACK_INITIALIZER(0x01, 0x02);

struct script_runner {
	struct modem_ubx_script script;
	struct {
		bool done;
		int ret;
	} result;
};

static struct script_runner test_script_runner;

static void *test_setup(void)
{
	const struct modem_ubx_config cmd_config = {
		.user_data = &cmd_user_data,
		.receive_buf = cmd_receive_buf,
		.receive_buf_size = ARRAY_SIZE(cmd_receive_buf),
		.unsol_matches = {
			.array = unsol_matches,
			.size = ARRAY_SIZE(unsol_matches),
		},
	};

	zassert(modem_ubx_init(&cmd, &cmd_config) == 0, "Failed to init modem CMD");

	const struct modem_backend_mock_config mock_config = {
		.rx_buf = mock_rx_buf,
		.rx_buf_size = ARRAY_SIZE(mock_rx_buf),
		.tx_buf = mock_tx_buf,
		.tx_buf_size = ARRAY_SIZE(mock_tx_buf),
		.limit = 128,
	};

	mock_pipe = modem_backend_mock_init(&mock, &mock_config);
	zassert(modem_pipe_open(mock_pipe, K_SECONDS(10)) == 0, "Failed to open mock pipe");
	zassert(modem_ubx_attach(&cmd, mock_pipe) == 0, "Failed to attach pipe mock to modem CMD");

	return NULL;
}

static inline void restore_ubx_script(void)
{
	static const struct ubx_frame frame_restored = UBX_FRAME_ACK_INITIALIZER(0x01, 0x02);
	const struct script_runner script_runner_restored = {
		.script = {
			.request = {
				.buf = &test_req,
				.len = UBX_FRAME_SZ(frame_restored.payload_size),
			},
			.match = MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_ACK, UBX_MSG_ID_ACK, NULL),
			.response = {
				.buf = cmd_response,
				.buf_len = sizeof(cmd_response),
			},
			.timeout = K_SECONDS(1),
		},
	};

	test_script_runner = script_runner_restored;
	test_req = frame_restored;
}

static void test_before(void *f)
{
	atomic_set(&callback_called, 0);
	modem_backend_mock_reset(&mock);
	restore_ubx_script();
}

ZTEST_SUITE(modem_ubx, NULL, test_setup, test_before, NULL, NULL);

static K_THREAD_STACK_ARRAY_DEFINE(stacks, 3, 2048);
static struct k_thread threads[3];

static void script_runner_handler(void *val, void *unused1, void *unused2)
{
	struct script_runner *runner = (struct script_runner *)val;

	int ret = modem_ubx_run_script(&cmd, &runner->script);

	runner->result.done = true;
	runner->result.ret = ret;
}

static inline void script_runner_start(struct script_runner *runner, uint8_t idx)
{
	k_thread_create(&threads[idx],
			stacks[idx],
			K_THREAD_STACK_SIZEOF(stacks[idx]),
			script_runner_handler,
			runner, NULL, NULL,
			K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1),
			0,
			K_NO_WAIT);

	k_thread_start(&threads[idx]);
}

static inline void test_thread_yield(void)
{
	/** Used instead of k_yield() since internals of modem pipe may rely on
	 * multiple thread interactions which may not be served by simply
	 * yielding.
	 */
	k_sleep(K_MSEC(1));
}

ZTEST(modem_ubx, test_cmd_no_rsp_is_non_blocking)
{
	/** Keep in mind this only happens if there isn't an on-going transfer
	 * already. If that happens, it will wait until the other script
	 * finishes or this request times out. Check test-case:
	 * test_script_is_thread_safe for details.
	 */
	uint8_t buf[256];
	int len;

	/* Setting filter class to 0 means no response is to be awaited */
	test_script_runner.script.match.filter.class = 0;

	script_runner_start(&test_script_runner, 0);
	test_thread_yield();

	len = modem_backend_mock_get(&mock, buf, sizeof(buf));

	zassert_true(test_script_runner.result.done, "Script should be done");
	zassert_ok(test_script_runner.result.ret, "%d", test_script_runner.result.ret);
	zassert_equal(UBX_FRAME_SZ(test_req.payload_size), len, "expected: %d, got: %d",
		      UBX_FRAME_SZ(test_req.payload_size), len);
}

ZTEST(modem_ubx, test_cmd_rsp_retries_and_times_out)
{
	uint8_t buf[512];

	test_script_runner.script.timeout = K_SECONDS(3);
	test_script_runner.script.retry_count = 2; /* 2 Retries -> 3 Tries */

	script_runner_start(&test_script_runner, 0);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	for (size_t i = 0 ; i < (test_script_runner.script.retry_count + 1) ; i++) {

		int len = modem_backend_mock_get(&mock, buf, sizeof(buf));

		zassert_false(test_script_runner.result.done, "Script should not be done. "
							 "Iteration: %d", i);
		zassert_equal(UBX_FRAME_SZ(test_req.payload_size), len,
			      "Payload Sent does not match. "
			      "Expected: %d, Received: %d, Iteration: %d",
			      UBX_FRAME_SZ(test_req.payload_size), len, i);

		k_sleep(K_SECONDS(1));
	}

	zassert_true(test_script_runner.result.done, "Script should be done");
	zassert_equal(test_script_runner.result.ret, -EAGAIN, "Script should time out");
}

ZTEST(modem_ubx, test_cmd_rsp_blocks_and_receives_rsp)
{
	static struct ubx_frame test_rsp = UBX_FRAME_ACK_INITIALIZER(0x02, 0x03);

	script_runner_start(&test_script_runner, 0);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	modem_backend_mock_put(&mock, (uint8_t *)&test_rsp, UBX_FRAME_SZ(test_rsp.payload_size));
	test_thread_yield();

	zassert_true(test_script_runner.result.done, "Script should be done");
	zassert_ok(test_script_runner.result.ret);
	zassert_equal(UBX_FRAME_SZ(test_rsp.payload_size),
		      test_script_runner.script.response.received_len,
		      "expected: %d, got: %d",
		      UBX_FRAME_SZ(test_rsp.payload_size),
		      test_script_runner.script.response.received_len);
}

ZTEST(modem_ubx, test_script_is_thread_safe)
{
	static struct ubx_frame test_rsp = UBX_FRAME_ACK_INITIALIZER(0x02, 0x03);
	struct script_runner script_runner_1 = {
		.script = {
			.request = {
				.buf = &test_req,
				.len = UBX_FRAME_SZ(test_req.payload_size),
			},
			.match = MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_ACK, UBX_MSG_ID_ACK, NULL),
			.response = {
				.buf = cmd_response,
				.buf_len = sizeof(cmd_response),
			},
			.timeout = K_SECONDS(1),
		},
	};
	struct script_runner script_runner_2 = {
		.script = {
			.request = {
				.buf = &test_req,
				.len = UBX_FRAME_SZ(test_req.payload_size),
			},
			.match = MODEM_UBX_MATCH_DEFINE(UBX_CLASS_ID_ACK, UBX_MSG_ID_ACK, NULL),
			.response = {
				.buf = cmd_response,
				.buf_len = sizeof(cmd_response),
			},
			.timeout = K_SECONDS(1),
		},
	};

	script_runner_start(&script_runner_1, 0);
	script_runner_start(&script_runner_2, 1);
	test_thread_yield();

	zassert_false(script_runner_1.result.done);
	zassert_false(script_runner_2.result.done);

	modem_backend_mock_put(&mock, (uint8_t *)&test_rsp, UBX_FRAME_SZ(test_rsp.payload_size));
	test_thread_yield();

	zassert_true(script_runner_1.result.done);
	zassert_ok(script_runner_1.result.ret);
	zassert_false(script_runner_2.result.done);

	modem_backend_mock_put(&mock, (uint8_t *)&test_rsp, UBX_FRAME_SZ(test_rsp.payload_size));
	test_thread_yield();

	zassert_true(script_runner_2.result.done);
	zassert_ok(script_runner_2.result.ret);
}

ZTEST(modem_ubx, test_rsp_filters_out_bytes_before_payload)
{
	static struct ubx_frame test_rsp = UBX_FRAME_ACK_INITIALIZER(0x02, 0x03);

	/** Create a buf that contains an "AT command" followed by the UBX frame
	 * we're expecting. This should be handled by the modem_ubx.
	 */
	char atcmd[] = "Here's an AT command: AT\r\nOK.";
	uint8_t buf[256];
	size_t buf_len = 0;

	memcpy(buf, atcmd, sizeof(atcmd));
	buf_len += sizeof(atcmd);
	memcpy(buf + buf_len, &test_rsp, UBX_FRAME_SZ(test_rsp.payload_size));
	buf_len += UBX_FRAME_SZ(test_rsp.payload_size);

	script_runner_start(&test_script_runner, 0);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	modem_backend_mock_put(&mock, (uint8_t *)buf, buf_len);
	test_thread_yield();

	zassert_true(test_script_runner.result.done, "Script should be done");
	zassert_ok(test_script_runner.result.ret);
	zassert_equal(UBX_FRAME_SZ(test_rsp.payload_size),
		      test_script_runner.script.response.received_len,
		      "expected: %d, got: %d",
		      UBX_FRAME_SZ(test_rsp.payload_size),
		      test_script_runner.script.response.received_len);
	zassert_mem_equal(&test_rsp,
			  test_script_runner.script.response.buf,
			  UBX_FRAME_SZ(test_rsp.payload_size));
}

ZTEST(modem_ubx, test_rsp_incomplete_packet_discarded)
{
	static struct ubx_frame test_rsp = UBX_FRAME_ACK_INITIALIZER(0x02, 0x03);

	uint8_t buf[256];
	size_t buf_len = 0;

	memcpy(buf, &test_rsp, UBX_FRAME_SZ(test_rsp.payload_size) - 5);
	buf_len += UBX_FRAME_SZ(test_rsp.payload_size) - 5;

	script_runner_start(&test_script_runner, 0);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	modem_backend_mock_put(&mock, (uint8_t *)buf, buf_len);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	k_sleep(K_SECONDS(1));

	zassert_true(test_script_runner.result.done, "Script should be done");
	zassert_equal(-EAGAIN, test_script_runner.result.ret);
}

ZTEST(modem_ubx, test_rsp_discards_invalid_len)
{
	static struct ubx_frame test_rsp = UBX_FRAME_ACK_INITIALIZER(0x02, 0x03);

	/** Invalidate checksum */
	size_t frame_size = UBX_FRAME_SZ(test_rsp.payload_size);

	test_rsp.payload_size = 0xFFFF;

	script_runner_start(&test_script_runner, 0);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	modem_backend_mock_put(&mock, (uint8_t *)&test_rsp, frame_size);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	k_sleep(K_SECONDS(1));

	zassert_true(test_script_runner.result.done, "Script should be done");
	zassert_equal(-EAGAIN, test_script_runner.result.ret);
}

ZTEST(modem_ubx, test_rsp_discards_invalid_checksum)
{
	static struct ubx_frame test_rsp = UBX_FRAME_ACK_INITIALIZER(0x02, 0x03);

	/** Invalidate checksum */
	test_rsp.payload_and_checksum[test_rsp.payload_size] = 0xDE;
	test_rsp.payload_and_checksum[test_rsp.payload_size + 1] = 0xAD;

	script_runner_start(&test_script_runner, 0);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	modem_backend_mock_put(&mock, (uint8_t *)&test_rsp, UBX_FRAME_SZ(test_rsp.payload_size));
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	k_sleep(K_SECONDS(1));

	zassert_true(test_script_runner.result.done, "Script should be done");
	zassert_equal(-EAGAIN, test_script_runner.result.ret);
}

ZTEST(modem_ubx, test_rsp_split_in_two_events)
{
	static struct ubx_frame test_rsp = UBX_FRAME_ACK_INITIALIZER(0x02, 0x03);
	uint8_t *data_ptr = (uint8_t *)&test_rsp;

	script_runner_start(&test_script_runner, 0);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	/** The first portion of the packet. At this point the data should not be discarded,
	 * understanding there's more data to come.
	 */
	modem_backend_mock_put(&mock, data_ptr, UBX_FRAME_SZ(test_rsp.payload_size) - 5);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	/** The other portion of the packet. This should complete the packet reception */
	modem_backend_mock_put(&mock, &data_ptr[UBX_FRAME_SZ(test_rsp.payload_size) - 5], 5);
	test_thread_yield();

	zassert_true(test_script_runner.result.done, "Script should be done");
	zassert_ok(test_script_runner.result.ret);
	zassert_equal(UBX_FRAME_SZ(test_rsp.payload_size),
		      test_script_runner.script.response.received_len,
		      "expected: %d, got: %d",
		      UBX_FRAME_SZ(test_rsp.payload_size),
		      test_script_runner.script.response.received_len);
}

ZTEST(modem_ubx, test_rsp_filters_out_non_matches)
{
	static struct ubx_frame test_rsp_non_match = UBX_FRAME_NAK_INITIALIZER(0x02, 0x03);
	static struct ubx_frame test_rsp_match = UBX_FRAME_ACK_INITIALIZER(0x02, 0x03);

	uint8_t buf[256];
	size_t buf_len = 0;

	/** We're passing a valid packet, but not what we're expecing. We
	 * should not get an event out of this one.
	 */
	memcpy(buf, &test_rsp_non_match, UBX_FRAME_SZ(test_rsp_non_match.payload_size));
	buf_len += UBX_FRAME_SZ(test_rsp_non_match.payload_size);

	script_runner_start(&test_script_runner, 0);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	modem_backend_mock_put(&mock, (uint8_t *)buf, buf_len);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	/** Now we're passing two valid packets, on the same event: one which
	 * does not match, one which matches. We should get the latter.
	 */
	memcpy(buf, &test_rsp_match, UBX_FRAME_SZ(test_rsp_match.payload_size));
	buf_len += UBX_FRAME_SZ(test_rsp_match.payload_size);

	modem_backend_mock_put(&mock, (uint8_t *)buf, buf_len);
	test_thread_yield();

	zassert_true(test_script_runner.result.done, "Script should be done");
	zassert_ok(test_script_runner.result.ret);
	zassert_equal(UBX_FRAME_SZ(test_rsp_match.payload_size),
		      test_script_runner.script.response.received_len,
		      "expected: %d, got: %d",
		      UBX_FRAME_SZ(test_rsp_match.payload_size),
		      test_script_runner.script.response.received_len);
	zassert_mem_equal(&test_rsp_match,
			  test_script_runner.script.response.buf,
			  UBX_FRAME_SZ(test_rsp_match.payload_size));
}

ZTEST(modem_ubx, test_rsp_match_with_payload)
{
	static struct ubx_frame test_rsp_non_match = UBX_FRAME_ACK_INITIALIZER(0x02, 0x03);
	static struct ubx_frame test_rsp_match = UBX_FRAME_ACK_INITIALIZER(0x03, 0x04);

	test_script_runner.script.match.filter.payload.buf = test_rsp_match.payload_and_checksum;
	test_script_runner.script.match.filter.payload.len = test_rsp_match.payload_size;

	uint8_t buf[256];
	size_t buf_len = 0;

	/** We're passing a valid packet, but not what we're expecing. We
	 * should not get an event out of this one.
	 */
	memcpy(buf, &test_rsp_non_match, UBX_FRAME_SZ(test_rsp_non_match.payload_size));
	buf_len += UBX_FRAME_SZ(test_rsp_non_match.payload_size);

	script_runner_start(&test_script_runner, 0);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	modem_backend_mock_put(&mock, (uint8_t *)buf, buf_len);
	test_thread_yield();

	zassert_false(test_script_runner.result.done, "Script should not be done");

	/** Now we're passing two valid packets, on the same event: one which
	 * does not match, one which matches. We should get the latter.
	 */
	memcpy(buf, &test_rsp_match, UBX_FRAME_SZ(test_rsp_match.payload_size));
	buf_len += UBX_FRAME_SZ(test_rsp_match.payload_size);

	modem_backend_mock_put(&mock, (uint8_t *)buf, buf_len);
	test_thread_yield();

	zassert_true(test_script_runner.result.done, "Script should be done");
	zassert_ok(test_script_runner.result.ret);
	zassert_equal(UBX_FRAME_SZ(test_rsp_match.payload_size),
		      test_script_runner.script.response.received_len,
		      "expected: %d, got: %d",
		      UBX_FRAME_SZ(test_rsp_match.payload_size),
		      test_script_runner.script.response.received_len);
	zassert_mem_equal(&test_rsp_match,
			  test_script_runner.script.response.buf,
			  UBX_FRAME_SZ(test_rsp_match.payload_size));
}

ZTEST(modem_ubx, test_unsol_matches_trigger_cb)
{
	static struct ubx_frame ack_frame = UBX_FRAME_ACK_INITIALIZER(0x01, 0x02);
	static struct ubx_frame nak_frame = UBX_FRAME_NAK_INITIALIZER(0x01, 0x02);

	zassert_false(atomic_test_bit(&callback_called, MODEM_UBX_UTEST_ON_ACK_RECEIVED_BIT));
	zassert_false(atomic_test_bit(&callback_called, MODEM_UBX_UTEST_ON_NAK_RECEIVED_BIT));

	modem_backend_mock_put(&mock,
			       (const uint8_t *)&ack_frame,
			       UBX_FRAME_SZ(ack_frame.payload_size));
	test_thread_yield();

	zassert_true(atomic_test_bit(&callback_called, MODEM_UBX_UTEST_ON_ACK_RECEIVED_BIT));
	zassert_false(atomic_test_bit(&callback_called, MODEM_UBX_UTEST_ON_NAK_RECEIVED_BIT));

	modem_backend_mock_put(&mock,
			       (const uint8_t *)&nak_frame,
			       UBX_FRAME_SZ(nak_frame.payload_size));
	test_thread_yield();

	zassert_true(atomic_test_bit(&callback_called, MODEM_UBX_UTEST_ON_NAK_RECEIVED_BIT));
}

ZTEST(modem_ubx, test_ubx_frame_encode_matches_compile_time_macro)
{
	static struct ubx_frame ack_frame = UBX_FRAME_ACK_INITIALIZER(0x01, 0x02);

	uint8_t buf[256];
	struct ubx_ack ack = {
		.class = 0x01,
		.id = 0x02,
	};

	int len = ubx_frame_encode(UBX_CLASS_ID_ACK, UBX_MSG_ID_ACK,
				   (const uint8_t *)&ack, sizeof(ack),
				   buf, sizeof(buf));
	zassert_equal(len, UBX_FRAME_SZ(sizeof(ack)), "Expected: %d, got: %d",
		      UBX_FRAME_SZ(sizeof(ack)), len);
	zassert_mem_equal(buf, &ack_frame, UBX_FRAME_SZ(sizeof(ack)));
}
