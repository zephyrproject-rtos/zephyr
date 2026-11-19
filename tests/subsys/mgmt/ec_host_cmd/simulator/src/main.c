/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/mgmt/ec_host_cmd/simulator.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/cbprintf.h>
#include <zephyr/ztest.h>

static size_t hexdump_log_count;
static size_t first_hexdump_len;
static size_t last_hexdump_len;
static char last_log_str[128];
static size_t last_log_str_pos;
static uint8_t last_log_level;

static int test_log_str_out(int c, void *ctx)
{
	ARG_UNUSED(ctx);
	if (last_log_str_pos < sizeof(last_log_str) - 1) {
		last_log_str[last_log_str_pos++] = (char)c;
		last_log_str[last_log_str_pos] = '\0';
	}
	return c;
}

static void test_log_backend_process(const struct log_backend *const backend,
				     union log_msg_generic *msg)
{
	size_t data_len = 0;
	size_t pkg_len = 0;
	uint8_t *pkg;

	ARG_UNUSED(backend);
	log_msg_get_data(&msg->log, &data_len);
	if (data_len > 0) {
		if (hexdump_log_count == 0) {
			first_hexdump_len = data_len;
		}
		hexdump_log_count++;
		last_hexdump_len = data_len;
	}

	if (log_msg_get_level(&msg->log) == LOG_LEVEL_INTERNAL_RAW_STRING) {
		return;
	}

	pkg = log_msg_get_package(&msg->log, &pkg_len);
	if (pkg != NULL && pkg_len > 0) {
		last_log_level = log_msg_get_level(&msg->log);
		last_log_str_pos = 0;
		last_log_str[0] = '\0';
		cbpprintf(test_log_str_out, NULL, pkg);
	}
}

static void test_log_backend_panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static const struct log_backend_api test_log_backend_api = {
	.process = test_log_backend_process,
	.panic = test_log_backend_panic,
};

LOG_BACKEND_DEFINE(test_log_backend, test_log_backend_api, true);

/* Variables used to record what is "sent" to host for verification. */
K_SEM_DEFINE(send_called, 0, 1);
struct ec_host_cmd_tx_buf *sent;

static int host_send(const struct ec_host_cmd_backend *backend)
{
	k_sem_give(&send_called);
	return 0;
}

/**
 * struct ec_params_add - Parameters to the add command.
 * @in_data: Pass anything here.
 */
struct ec_params_add {
	uint32_t in_data;
} __packed;

struct ec_params_unbounded {
	uint32_t bytes_to_write;
} __packed;

struct ec_params_busy_wait {
	uint32_t usec;
} __packed;

/**
 * struct ec_response_add - Response to the add command.
 * @out_data: Output will be in_data + 0x01020304.
 */
struct ec_response_add {
	uint32_t out_data;
} __packed;

struct ec_response_too_big {
	uint8_t out_data[512];
} __packed;

/* Buffer used to simulate incoming data from host to EC. */
static uint8_t host_to_dut_buffer[256];
struct rx_structure {
	struct ec_host_cmd_request_header header;
	union {
		struct ec_params_add add;
		struct ec_params_unbounded unbounded;
		struct ec_params_busy_wait busy_wait;
		uint8_t raw[0];
	};
} __packed * const host_to_dut = (void *)&host_to_dut_buffer;

/* Buffer used to verify expected outgoing data from EC to host. */
static uint8_t expected_dut_to_host_buffer[256];
struct tx_structure {
	struct ec_host_cmd_response_header header;
	union {
		struct ec_response_add add;
		uint8_t raw[0];
	};
} __packed * const expected_dut_to_host = (void *)&expected_dut_to_host_buffer;

static void update_host_to_dut_checksum(void)
{
	host_to_dut->header.checksum = 0;

	uint8_t checksum = 0;

	for (size_t i = 0;
	     i < sizeof(host_to_dut->header) + host_to_dut->header.data_len;
	     ++i) {
		checksum += host_to_dut_buffer[i];
	}
	host_to_dut->header.checksum = (uint8_t)(-checksum);
}

static void update_dut_to_host_checksum(void)
{
	const uint16_t buf_size = sizeof(expected_dut_to_host->header) +
				  expected_dut_to_host->header.data_len;

	expected_dut_to_host->header.checksum = 0;

	uint8_t checksum = 0;

	for (size_t i = 0; i < buf_size; ++i) {
		checksum += expected_dut_to_host_buffer[i];
	}

	expected_dut_to_host->header.checksum = (uint8_t)(-checksum);
}

static void simulate_rx_data(void)
{
	int rv;

	update_host_to_dut_checksum();
	/*
	 * Always send entire buffer and let host command framework read what it
	 * needs.
	 */
	rv = ec_host_cmd_backend_sim_data_received(host_to_dut_buffer,
						  sizeof(host_to_dut_buffer));
	zassert_equal(rv, 0, "Could not send data %d", rv);

	/* Ensure send was called so we can verify outputs */
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");
}

static uint16_t expected_tx_size(void)
{
	return sizeof(expected_dut_to_host->header) +
	       expected_dut_to_host->header.data_len;
}

static void verify_tx_data(void)
{
	update_dut_to_host_checksum();

	zassert_equal(sent->len, expected_tx_size(), "Sent bytes did not match");
	zassert_mem_equal(sent->buf, expected_dut_to_host, expected_tx_size(),
			  "Sent buffer did not match");
}

static void verify_tx_error(enum ec_host_cmd_status error)
{
	expected_dut_to_host->header.prtcl_ver = 3;
	expected_dut_to_host->header.result = error;
	expected_dut_to_host->header.data_len = 0;
	expected_dut_to_host->header.reserved = 0;
	update_dut_to_host_checksum();

	zassert_equal(sent->len, expected_tx_size(), "Sent bytes did not match");
	zassert_mem_equal(sent->buf, expected_dut_to_host, expected_tx_size(),
			  "Sent buffer did not match");
}

#define EC_CMD_HELLO 0x0001
static uint32_t hello_v0_resp(uint32_t in_data)
{
	return in_data + 0x01020304;
}

static uint32_t hello_v1_resp(uint32_t in_data)
{
	return in_data + 0x02040608;
}

static enum ec_host_cmd_status
ec_host_cmd_add(struct ec_host_cmd_handler_args *args)
{
	const struct ec_params_add *const request = args->input_buf;
	struct ec_response_add *const response = args->output_buf;

	if (args->version == 0) {
		response->out_data = hello_v0_resp(request->in_data);
	} else if (args->version == 1) {
		response->out_data = hello_v1_resp(request->in_data);
	} else if (args->version == 2) {
		return EC_HOST_CMD_OVERFLOW;
	} else {
		zassert_unreachable("Should not get version %d", args->version);
	}

	args->output_buf_size = sizeof(*response);
	return EC_HOST_CMD_SUCCESS;
}
EC_HOST_CMD_HANDLER(EC_CMD_HELLO, ec_host_cmd_add, BIT(0) | BIT(1) | BIT(2),
		    struct ec_params_add, struct ec_response_add);

ZTEST(ec_host_cmd, test_add)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 0;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	simulate_rx_data();

	expected_dut_to_host->header.prtcl_ver = 3;
	expected_dut_to_host->header.result = 0;
	expected_dut_to_host->header.reserved = 0;
	expected_dut_to_host->header.data_len =
		sizeof(expected_dut_to_host->add);
	expected_dut_to_host->add.out_data =
		hello_v0_resp(host_to_dut->add.in_data);

	verify_tx_data();
}

ZTEST(ec_host_cmd, test_add_version_2)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 1;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	simulate_rx_data();

	expected_dut_to_host->header.prtcl_ver = 3;
	expected_dut_to_host->header.result = 0;
	expected_dut_to_host->header.reserved = 0;
	expected_dut_to_host->header.data_len =
		sizeof(expected_dut_to_host->add);
	expected_dut_to_host->add.out_data =
		hello_v1_resp(host_to_dut->add.in_data);

	verify_tx_data();
}

ZTEST(ec_host_cmd, test_add_invalid_version)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 3;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	simulate_rx_data();

	verify_tx_error(EC_HOST_CMD_INVALID_VERSION);
}

ZTEST(ec_host_cmd, test_add_invalid_version_big)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 128;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	simulate_rx_data();

	verify_tx_error(EC_HOST_CMD_INVALID_VERSION);
}

ZTEST(ec_host_cmd, test_add_invalid_prtcl_ver_2)
{
	host_to_dut->header.prtcl_ver = 2;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 2;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	simulate_rx_data();

	verify_tx_error(EC_HOST_CMD_INVALID_HEADER);
}

ZTEST(ec_host_cmd, test_add_invalid_prtcl_ver_4)
{
	host_to_dut->header.prtcl_ver = 4;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 2;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	simulate_rx_data();

	verify_tx_error(EC_HOST_CMD_INVALID_HEADER);
}

ZTEST(ec_host_cmd, test_add_invalid_rx_checksum)
{
	int rv;

	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 2;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	/* Set an invalid checksum */
	host_to_dut->header.checksum = 42;

	/* Always send entire buffer and let host command framework read what it
	 * needs.
	 */
	rv = ec_host_cmd_backend_sim_data_received(host_to_dut_buffer,
						  sizeof(host_to_dut_buffer));
	zassert_equal(rv, 0, "Could not send data %d", rv);

	/* Ensure Send was called so we can verify outputs */
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");

	verify_tx_error(EC_HOST_CMD_INVALID_CHECKSUM);
}

ZTEST(ec_host_cmd, test_add_rx_size_too_small_for_header)
{
	int rv;

	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 2;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	rv = ec_host_cmd_backend_sim_data_received(host_to_dut_buffer, 4);
	zassert_equal(rv, 0, "Could not send data %d", rv);

	/* Ensure Send was called so we can verify outputs */
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");

	verify_tx_error(EC_HOST_CMD_REQUEST_TRUNCATED);
}

ZTEST(ec_host_cmd, test_add_rx_size_too_small)
{
	int rv;

	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 2;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	rv = ec_host_cmd_backend_sim_data_received(
		host_to_dut_buffer,
		sizeof(host_to_dut->header) + host_to_dut->header.data_len - 1);
	zassert_equal(rv, 0, "Could not send data %d", rv);

	/* Ensure Send was called so we can verify outputs */
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");

	verify_tx_error(EC_HOST_CMD_REQUEST_TRUNCATED);
}

ZTEST(ec_host_cmd, test_add_rqt_size_too_small)
{
	*host_to_dut = (struct rx_structure){
		.header = {
				.prtcl_ver = 3,
				.cmd_id = EC_CMD_HELLO,
				.data_len = sizeof(host_to_dut->add) - 1,
			},
		.add.in_data = 0x10203040,
	};

	simulate_rx_data();

	verify_tx_error(EC_HOST_CMD_REQUEST_TRUNCATED);
}

ZTEST(ec_host_cmd, test_no_log_on_header_truncated)
{
	int rv;

	hexdump_log_count = 0;
	last_hexdump_len = 0;

	host_to_dut->header = (struct ec_host_cmd_request_header){
		.prtcl_ver = 3,
		.cmd_id = EC_CMD_HELLO,
		.data_len = 64,
	};

	rv = ec_host_cmd_backend_sim_data_received(host_to_dut_buffer,
						   sizeof(host_to_dut->header) - 1);
	zassert_equal(rv, 0, "Could not send data %d", rv);
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");
	verify_tx_error(EC_HOST_CMD_REQUEST_TRUNCATED);
	zassert_equal(hexdump_log_count, 0,
		      "Header-truncated request must not trigger hexdump log");
}

ZTEST(ec_host_cmd, test_no_log_on_invalid_prtcl_ver)
{
	hexdump_log_count = 0;
	last_hexdump_len = 0;

	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 2,
			.cmd_id = EC_CMD_HELLO,
			.data_len = sizeof(host_to_dut->add),
		},
		.add.in_data = 0x10203040,
	};

	simulate_rx_data();
	verify_tx_error(EC_HOST_CMD_INVALID_HEADER);
	zassert_equal(hexdump_log_count, 0,
		      "Invalid header version request must not trigger hexdump log");
}

ZTEST(ec_host_cmd, test_no_log_on_payload_truncated)
{
	int rv;

	hexdump_log_count = 0;
	last_hexdump_len = 0;

	host_to_dut->header = (struct ec_host_cmd_request_header){
		.prtcl_ver = 3,
		.cmd_id = EC_CMD_HELLO,
		.data_len = 128,
	};

	rv = ec_host_cmd_backend_sim_data_received(host_to_dut_buffer,
						   sizeof(host_to_dut->header));
	zassert_equal(rv, 0, "Could not send data %d", rv);
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");
	verify_tx_error(EC_HOST_CMD_REQUEST_TRUNCATED);
	zassert_equal(hexdump_log_count, 0,
		      "Oversized/truncated request must not trigger hexdump log");
}

ZTEST(ec_host_cmd, test_no_log_on_invalid_checksum)
{
	int rv;

	hexdump_log_count = 0;
	last_hexdump_len = 0;

	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_HELLO,
			.checksum = 42,
			.data_len = sizeof(host_to_dut->add),
		},
		.add.in_data = 0x10203040,
	};

	rv = ec_host_cmd_backend_sim_data_received(host_to_dut_buffer,
						   sizeof(host_to_dut_buffer));
	zassert_equal(rv, 0, "Could not send data %d", rv);
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");
	verify_tx_error(EC_HOST_CMD_INVALID_CHECKSUM);
	zassert_equal(hexdump_log_count, 0,
		      "Invalid checksum request must not trigger hexdump log");
}

ZTEST(ec_host_cmd, test_log_on_valid_request)
{
	hexdump_log_count = 0;
	first_hexdump_len = 0;
	last_hexdump_len = 0;

	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_HELLO,
			.data_len = sizeof(host_to_dut->add),
		},
		.add.in_data = 0x10203040,
	};

	simulate_rx_data();
	zassert_equal(hexdump_log_count, 2,
		      "Valid request should log request and response hexdumps");
	zassert_equal(first_hexdump_len, sizeof(host_to_dut->add),
		      "Unexpected request hexdump length %zu", first_hexdump_len);
	zassert_equal(last_hexdump_len, sizeof(expected_dut_to_host->add),
		      "Unexpected response hexdump length %zu", last_hexdump_len);
}

ZTEST(ec_host_cmd, test_unknown_command)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = 1234;
	host_to_dut->header.cmd_ver = 2;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = 0;

	simulate_rx_data();

	verify_tx_error(EC_HOST_CMD_INVALID_COMMAND);
}

#define EC_CMD_UNBOUNDED 0x0002
static enum ec_host_cmd_status
ec_host_cmd_unbounded(struct ec_host_cmd_handler_args *args)
{
	const struct ec_params_unbounded *const request = args->input_buf;
	const uint8_t *in_buffer = args->input_buf;
	uint8_t *out_buffer = args->output_buf;

	/* Version 1 just says we used the space without writing */
	if (args->version == 1) {
		args->output_buf_size = request->bytes_to_write;
		return EC_HOST_CMD_SUCCESS;
	}

	/* Version 2 adds extra asserts */
	if (args->version == 2) {
		zassert_equal(in_buffer[4], 0, "Ensure input data is clear");
		zassert_equal(out_buffer[0], 0, "Ensure output is clear");
		zassert_equal(out_buffer[1], 0, "Ensure output is clear");
		zassert_equal(out_buffer[2], 0, "Ensure output is clear");
		zassert_equal(out_buffer[3], 0, "Ensure output is clear");
	}

	/* Version 0 (and 2) write request bytes if it can fit */
	if (request->bytes_to_write == 0) {
		return EC_HOST_CMD_SUCCESS;
	}

	if (request->bytes_to_write > args->output_buf_max) {
		return EC_HOST_CMD_OVERFLOW;
	}

	for (int i = 0; i < request->bytes_to_write; ++i) {
		zassert_equal(out_buffer[i], 0, "Ensure every TX byte is 0");
		out_buffer[i] = i;
	}

	args->output_buf_size = request->bytes_to_write;
	return EC_HOST_CMD_SUCCESS;
}
EC_HOST_CMD_HANDLER_UNBOUND(EC_CMD_UNBOUNDED, ec_host_cmd_unbounded,
			    BIT(0) | BIT(1) | BIT(2));

ZTEST(ec_host_cmd, test_unbounded_output_cleared)
{
	const struct tx_structure *const tx = (void *)sent->buf;

	/*
	 * First call writes hello_v0_resp(in_data) to tx->add.out_data and sets
	 * output_buf_size = 4.
	 */
	*host_to_dut = (struct rx_structure){
		.header = {
				.prtcl_ver = 3,
				.cmd_id = EC_CMD_HELLO,
				.data_len = sizeof(host_to_dut->add),
			},
		.add.in_data = 0x10203040,
	};
	*expected_dut_to_host = (struct tx_structure){
		.header = {
				.prtcl_ver = 3,
				.data_len = sizeof(expected_dut_to_host->add),
			},
		.add.out_data = hello_v0_resp(host_to_dut->add.in_data),
	};
	simulate_rx_data();
	verify_tx_data();
	zassert_equal(tx->add.out_data,
		      hello_v0_resp(host_to_dut->add.in_data));

	/*
	 * With bytes_to_write == 0, the handler returns without modifying
	 * output_buf or output_buf_size, verifying that both were reset to 0
	 * before the handler was called.
	 */
	*host_to_dut = (struct rx_structure){
		.header = {
				.prtcl_ver = 3,
				.cmd_id = EC_CMD_UNBOUNDED,
				.data_len = sizeof(host_to_dut->unbounded),
			},
		.unbounded.bytes_to_write = 0,
	};
	simulate_rx_data();

	expected_dut_to_host->header.data_len = 0;
	verify_tx_data();
	zassert_equal(tx->add.out_data, 0);
}

ZTEST(ec_host_cmd, test_unbounded_handler_error_return)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_UNBOUNDED;
	host_to_dut->header.cmd_ver = 0;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->unbounded);
	host_to_dut->unbounded.bytes_to_write = INT16_MAX;

	simulate_rx_data();

	verify_tx_error(EC_HOST_CMD_OVERFLOW);
}

ZTEST(ec_host_cmd, test_unbounded_handler_response_too_big)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_UNBOUNDED;
	host_to_dut->header.cmd_ver = 1;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->unbounded);
	host_to_dut->unbounded.bytes_to_write = INT16_MAX;

	simulate_rx_data();

	verify_tx_error(EC_HOST_CMD_INVALID_RESPONSE);
}


ZTEST(ec_host_cmd, test_unbounded_handler_response_wrap)
{
	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_UNBOUNDED,
			.cmd_ver = 1,
			.data_len = sizeof(host_to_dut->unbounded),
		},
		/* 0xFFF8 + 8 wraps to 0 in uint16_t */
		.unbounded.bytes_to_write =
			UINT16_MAX - sizeof(struct ec_host_cmd_response_header) + 1,
	};
	simulate_rx_data();
	verify_tx_error(EC_HOST_CMD_INVALID_RESPONSE);
}

#define EC_CMD_TOO_BIG 0x0003
static enum ec_host_cmd_status
ec_host_cmd_too_big(struct ec_host_cmd_handler_args *args)
{
	return EC_HOST_CMD_SUCCESS;
}
EC_HOST_CMD_HANDLER(EC_CMD_TOO_BIG, ec_host_cmd_too_big, BIT(0), uint32_t,
		    struct ec_response_too_big);

ZTEST(ec_host_cmd, test_response_always_too_big)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_TOO_BIG;
	host_to_dut->header.cmd_ver = 0;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(uint32_t);

	simulate_rx_data();

	verify_tx_error(EC_HOST_CMD_INVALID_RESPONSE);
}

ZTEST(ec_host_cmd, test_rx_data_len_bounds)
{
	const uint16_t invalid_data_lens[] = {
		CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE -
			sizeof(struct ec_host_cmd_request_header) + 1,
		UINT16_MAX - sizeof(struct ec_host_cmd_request_header) + 1,
		UINT16_MAX,
	};

	ARRAY_FOR_EACH(invalid_data_lens, i) {
		int rv;

		hexdump_log_count = 0;
		host_to_dut->header = (struct ec_host_cmd_request_header){
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_HELLO,
			.data_len = invalid_data_lens[i],
		};

		rv = ec_host_cmd_backend_sim_data_received(
			host_to_dut_buffer, sizeof(host_to_dut_buffer));
		zassert_equal(rv, 0, "Could not send data %d", rv);
		rv = k_sem_take(&send_called, K_SECONDS(1));
		zassert_equal(rv, 0, "Send was not called");
		verify_tx_error(EC_HOST_CMD_REQUEST_TRUNCATED);
		zassert_equal(hexdump_log_count, 0,
			      "Rejected request must not trigger hexdump log");
	}
}

ZTEST(ec_host_cmd, test_err_log_on_unverified_rx)
{
	int rv;

	/* Prime rx_ctx.buf with a valid command (cmd_id = EC_CMD_HELLO = 0x0001) */
	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_HELLO,
			.data_len = sizeof(host_to_dut->add),
		},
		.add = {
			.in_data = 0x10203040,
		},
	};
	simulate_rx_data();

	/*
	 * Send a 2-byte truncated packet so bytes 2..3 (cmd_id) in rx_ctx.buf
	 * retain the stale 0x0001 value from the previous command.
	 */
	last_log_str[0] = '\0';
	rv = ec_host_cmd_backend_sim_data_received(host_to_dut_buffer, 2);
	zassert_equal(rv, 0, "Could not send data %d", rv);
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");
	verify_tx_error(EC_HOST_CMD_REQUEST_TRUNCATED);
	zassert_equal(strcmp(last_log_str, "HC rx err 13"), 0,
		      "Expected 'HC rx err 13' without stale cmd_id, got '%s'", last_log_str);

	/*
	 * Verify that when verify_rx() succeeds and handler validation fails,
	 * cmd_id is logged as expected.
	 */
	last_log_str[0] = '\0';
	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_HELLO,
			.cmd_ver = 3,
			.data_len = sizeof(host_to_dut->add),
		},
		.add = {
			.in_data = 0x10203040,
		},
	};
	simulate_rx_data();
	verify_tx_error(EC_HOST_CMD_INVALID_VERSION);
	zassert_equal(strcmp(last_log_str, "HC 0x0001 err 6"), 0,
		      "Expected 'HC 0x0001 err 6', got '%s'", last_log_str);
}

ZTEST(ec_host_cmd, test_no_log_on_invalid_response)
{
	const uint16_t oversized_len = UINT16_MAX - sizeof(struct ec_host_cmd_response_header) + 1;

	hexdump_log_count = 0;
	first_hexdump_len = 0;
	last_hexdump_len = 0;

	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_UNBOUNDED,
			.cmd_ver = 1,
			.data_len = sizeof(host_to_dut->unbounded),
		},
		.unbounded.bytes_to_write = oversized_len,
	};

	simulate_rx_data();
	verify_tx_error(EC_HOST_CMD_INVALID_RESPONSE);
	zassert_equal(hexdump_log_count, 1,
		      "Rejected response must not trigger response hexdump (got %zu hexdumps)",
		      hexdump_log_count);
	zassert_equal(last_hexdump_len, sizeof(host_to_dut->unbounded),
		      "Only the request payload should be hexdumped");
}

ZTEST(ec_host_cmd, test_log_on_request_max_version)
{
	hexdump_log_count = 0;
	first_hexdump_len = 0;

	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_HELLO,
			.cmd_ver = UINT8_MAX,
			.data_len = sizeof(host_to_dut->add),
		},
		.add.in_data = 0x10203040,
	};

	simulate_rx_data();

	/* It should log the request hexdump, then fail validation. */
	zassert_equal(hexdump_log_count, 1,
		      "Request with max version should log the request hexdump");
	zassert_equal(first_hexdump_len, sizeof(host_to_dut->add),
		      "Unexpected request hexdump length");

	verify_tx_error(EC_HOST_CMD_INVALID_VERSION);
}

#define EC_CMD_SUPPRESSED_ERR   0x0004
#define EC_CMD_UNSUPPRESSED_ERR 0x0005

static enum ec_host_cmd_status ec_host_cmd_return_err(struct ec_host_cmd_handler_args *args)
{
	if (args->version == 0) {
		return EC_HOST_CMD_UNAVAILABLE;
	}

	return EC_HOST_CMD_OVERFLOW;
}
EC_HOST_CMD_HANDLER_UNBOUND(EC_CMD_SUPPRESSED_ERR, ec_host_cmd_return_err, BIT(0) | BIT(1));
EC_HOST_CMD_HANDLER_UNBOUND(EC_CMD_UNSUPPRESSED_ERR, ec_host_cmd_return_err, BIT(0) | BIT(1));

ZTEST(ec_host_cmd, test_suppressed_unavailable_log_level)
{
#ifdef CONFIG_EC_HOST_CMD_LOG_SUPPRESSED
	static bool suppressed_registered;

	if (!suppressed_registered) {
		zassert_equal(ec_host_cmd_add_suppressed(EC_CMD_SUPPRESSED_ERR), 0,
			      "Failed to register suppressed command");
		suppressed_registered = true;
	}
#endif

	/* Suppressed command returning UNAVAILABLE logs at DBG (INF if suppression disabled). */
	last_log_str[0] = '\0';
	last_log_level = LOG_LEVEL_NONE;
	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_SUPPRESSED_ERR,
			.cmd_ver = 0,
			.data_len = 0,
		},
	};
	simulate_rx_data();
	verify_tx_error(EC_HOST_CMD_UNAVAILABLE);
	zassert_not_null(strstr(last_log_str, "HC 0x0004 err 9"),
			 "Unexpected log message '%s'", last_log_str);
	zassert_equal(last_log_level,
		      IS_ENABLED(CONFIG_EC_HOST_CMD_LOG_SUPPRESSED) ? LOG_LEVEL_DBG : LOG_LEVEL_INF,
		      "Unexpected log level %u", last_log_level);

	/* Suppressed command returning a different error still logs at INF. */
	last_log_str[0] = '\0';
	last_log_level = LOG_LEVEL_NONE;
	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_SUPPRESSED_ERR,
			.cmd_ver = 1,
			.data_len = 0,
		},
	};
	simulate_rx_data();
	verify_tx_error(EC_HOST_CMD_OVERFLOW);
	zassert_equal(strcmp(last_log_str, "HC 0x0004 err 11"), 0,
		      "Unexpected log message '%s'", last_log_str);
	zassert_equal(last_log_level, LOG_LEVEL_INF,
		      "Expected LOG_LEVEL_INF for non-UNAVAILABLE error, got %u", last_log_level);

	/* Unsuppressed command returning UNAVAILABLE still logs at INF. */
	last_log_str[0] = '\0';
	last_log_level = LOG_LEVEL_NONE;
	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_UNSUPPRESSED_ERR,
			.cmd_ver = 0,
			.data_len = 0,
		},
	};
	simulate_rx_data();
	verify_tx_error(EC_HOST_CMD_UNAVAILABLE);
	zassert_equal(strcmp(last_log_str, "HC 0x0005 err 9"), 0,
		      "Unexpected log message '%s'", last_log_str);
	zassert_equal(last_log_level, LOG_LEVEL_INF,
		      "Expected LOG_LEVEL_INF for unsuppressed command, got %u", last_log_level);
}

#define EC_CMD_BUSY_WAIT 0x0006

#ifdef CONFIG_EC_HOST_CMD_RATE_LIMITING
#define RATE_LIMIT_PERIOD_MS   CONFIG_EC_HOST_CMD_RATE_LIMITING_PERIOD_MS
#define RATE_LIMIT_MIN_REST_MS CONFIG_EC_HOST_CMD_RATE_LIMITING_MIN_REST_MS
#define RATE_LIMIT_RECESS_MS   CONFIG_EC_HOST_CMD_RATE_LIMITING_RECESS_MS
#else
#define RATE_LIMIT_PERIOD_MS   50
#define RATE_LIMIT_MIN_REST_MS 5
#define RATE_LIMIT_RECESS_MS   20
#endif

static enum ec_host_cmd_status ec_host_cmd_busy_wait(struct ec_host_cmd_handler_args *args)
{
	const struct ec_params_busy_wait *p = args->input_buf;

	k_busy_wait(p->usec);
	return EC_HOST_CMD_SUCCESS;
}
EC_HOST_CMD_HANDLER_REQ_ONLY(EC_CMD_BUSY_WAIT, ec_host_cmd_busy_wait, BIT(0),
			     struct ec_params_busy_wait);

static K_THREAD_STACK_DEFINE(low_prio_stack, 1024);
static struct k_thread low_prio_thread;
static atomic_t low_prio_runs;

static void low_prio_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	atomic_inc(&low_prio_runs);
}

static int32_t rate_limit_rest_ms(void)
{
	return (int32_t)k_ticks_to_ms_ceil32(k_ms_to_ticks_ceil64(RATE_LIMIT_MIN_REST_MS) + 2U);
}

static int64_t send_busy_wait_cmd(uint32_t usec)
{
	int64_t start_ms;
	int64_t elapsed_ms;

	*host_to_dut = (struct rx_structure){
		.header = {
			.prtcl_ver = 3,
			.cmd_id = EC_CMD_BUSY_WAIT,
			.cmd_ver = 0,
			.data_len = sizeof(host_to_dut->busy_wait),
		},
		.busy_wait = {
			.usec = usec,
		},
	};

	start_ms = k_uptime_get();
	simulate_rx_data();
	elapsed_ms = k_uptime_get() - start_ms;
	verify_tx_error(EC_HOST_CMD_SUCCESS);

	return elapsed_ms;
}

ZTEST(ec_host_cmd, test_rate_limiting_burst_allows_low_prio_thread)
{
	const uint32_t step_ms = 2U;
	const uint32_t count = (RATE_LIMIT_PERIOD_MS / step_ms) + 20U;
	int64_t max_cmd_ms = 0;

	/* Ensure a fresh rate-limiting burst window. */
	k_msleep(rate_limit_rest_ms());

	atomic_set(&low_prio_runs, 0);
	k_thread_create(&low_prio_thread, low_prio_stack, K_THREAD_STACK_SIZEOF(low_prio_stack),
			low_prio_entry, NULL, NULL, NULL, CONFIG_EC_HOST_CMD_HANDLER_PRIO + 1, 0,
			K_NO_WAIT);

	for (uint32_t i = 0; i < count; i++) {
		int64_t elapsed_ms = send_busy_wait_cmd(step_ms * USEC_PER_MSEC);

		if (elapsed_ms > max_cmd_ms) {
			max_cmd_ms = elapsed_ms;
		}
	}

	if (IS_ENABLED(CONFIG_EC_HOST_CMD_RATE_LIMITING)) {
		zassert_true(max_cmd_ms >= RATE_LIMIT_RECESS_MS,
			     "Expected recess >= %d ms during burst, max was %lld ms",
			     RATE_LIMIT_RECESS_MS, max_cmd_ms);
		zassert_equal(atomic_get(&low_prio_runs), 1,
			      "Lower-priority thread did not run during recess");
	} else {
		zassert_true(max_cmd_ms < RATE_LIMIT_RECESS_MS,
			     "Unexpected recess (%lld ms) with rate limiting disabled",
			     max_cmd_ms);
		zassert_equal(atomic_get(&low_prio_runs), 0,
			      "Lower-priority thread unexpectedly ran during burst");
	}

	zassert_equal(k_thread_join(&low_prio_thread, K_SECONDS(1)), 0,
		      "Failed to join lower-priority thread");
}

ZTEST(ec_host_cmd, test_rate_limiting_slow_handler_burst)
{
	const uint32_t slow_ms = (uint32_t)rate_limit_rest_ms() + 5U;
	const uint32_t count = (RATE_LIMIT_PERIOD_MS / slow_ms) + 3U;
	int64_t max_cmd_ms = 0;

	k_msleep(rate_limit_rest_ms());

	/*
	 * Each handler runs longer than MIN_REST_MS while the host sends
	 * commands back-to-back with no idle gap. Rate limiting must still
	 * trigger because there is no idle rest between response and next RX.
	 */
	for (uint32_t i = 0; i < count; i++) {
		int64_t elapsed_ms = send_busy_wait_cmd(slow_ms * USEC_PER_MSEC);

		if (elapsed_ms > max_cmd_ms) {
			max_cmd_ms = elapsed_ms;
		}
	}

	if (IS_ENABLED(CONFIG_EC_HOST_CMD_RATE_LIMITING)) {
		zassert_true(max_cmd_ms >= (int64_t)(slow_ms + RATE_LIMIT_RECESS_MS),
			     "Expected recess >= %u ms during slow-handler burst, max was %lld ms",
			     slow_ms + RATE_LIMIT_RECESS_MS, max_cmd_ms);
	} else {
		zassert_true(max_cmd_ms < (int64_t)(slow_ms + RATE_LIMIT_RECESS_MS),
			     "Unexpected recess (%lld ms) with rate limiting disabled",
			     max_cmd_ms);
	}
}

ZTEST(ec_host_cmd, test_rate_limiting_malformed_rx_burst)
{
	const uint32_t step_ms = 2U;
	const uint32_t count = (RATE_LIMIT_PERIOD_MS / step_ms) + 20U;
	int64_t max_cmd_ms = 0;

	k_msleep(rate_limit_rest_ms());

	for (uint32_t i = 0; i < count; i++) {
		int64_t start_ms = k_uptime_get();
		int64_t elapsed_ms;
		int rv;

		rv = ec_host_cmd_backend_sim_data_received(host_to_dut_buffer, 2);
		zassert_equal(rv, 0, "Could not send data %d", rv);
		rv = k_sem_take(&send_called, K_SECONDS(1));
		zassert_equal(rv, 0, "Send was not called");
		elapsed_ms = k_uptime_get() - start_ms;
		verify_tx_error(EC_HOST_CMD_REQUEST_TRUNCATED);

		if (elapsed_ms > max_cmd_ms) {
			max_cmd_ms = elapsed_ms;
		}

		k_busy_wait(step_ms * USEC_PER_MSEC);
	}

	if (IS_ENABLED(CONFIG_EC_HOST_CMD_RATE_LIMITING)) {
		zassert_true(max_cmd_ms >= RATE_LIMIT_RECESS_MS,
			     "Expected recess >= %d ms during malformed RX burst, max was %lld ms",
			     RATE_LIMIT_RECESS_MS, max_cmd_ms);
	} else {
		zassert_true(max_cmd_ms < RATE_LIMIT_RECESS_MS,
			     "Unexpected recess (%lld ms) with rate limiting disabled",
			     max_cmd_ms);
	}
}

ZTEST(ec_host_cmd, test_rate_limiting_idle_rest_resets_window)
{
	const uint32_t step_ms = RATE_LIMIT_PERIOD_MS / 4U;
	const int32_t rest_ms = rate_limit_rest_ms();
	int64_t max_cmd_ms = 0;

	k_msleep(rest_ms);

	/*
	 * Cumulative active processing exceeds 2 * PERIOD_MS, but the host
	 * pauses longer than MIN_REST_MS between commands, so no recess should
	 * be inserted.
	 */
	for (uint32_t i = 0; i < 8U; i++) {
		int64_t elapsed_ms = send_busy_wait_cmd(step_ms * USEC_PER_MSEC);

		if (elapsed_ms > max_cmd_ms) {
			max_cmd_ms = elapsed_ms;
		}

		k_msleep(rest_ms);
	}

	zassert_true(max_cmd_ms < (int64_t)(step_ms + RATE_LIMIT_RECESS_MS),
		     "Unexpected recess (%lld ms) when host rested between commands", max_cmd_ms);
}

static void *ec_host_cmd_tests_setup(void)
{
	ec_host_cmd_backend_sim_install_send_cb(host_send, &sent);
	return NULL;
}

ZTEST_SUITE(ec_host_cmd, NULL, ec_host_cmd_tests_setup, NULL, NULL, NULL);
