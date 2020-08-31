/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/ec_host_cmd_periph/ec_host_cmd_simulator.h>
#include <ec_host_cmd.h>
#include <ztest.h>

/* Variables used to record what is "sent" to host for verification. */
K_SEM_DEFINE(send_called, 0, 1);
struct ec_host_cmd_periph_tx_buf sent;

static int host_send(const struct device *const dev,
		     const struct ec_host_cmd_periph_tx_buf *const buf)
{
	sent.len = buf->len;
	sent.buf = buf->buf;
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
		uint8_t raw[0];
	};
} __packed * const host_to_dut = (void *)&host_to_dut_buffer;

/* Buffer used to verify expected outgoing data from EC to host. */
static uint8_t expected_dut_to_host_buffer[256];
struct tx_structure {
	struct ec_host_cmd_response_header header;
	union {
		struct ec_response_add add;
		struct ec_response_too_big too_big;
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
	rv = ec_host_cmd_periph_sim_data_received(host_to_dut_buffer,
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

	zassert_equal(sent.len, expected_tx_size(), "Sent bytes did not match");
	zassert_mem_equal(sent.buf, expected_dut_to_host, expected_tx_size(),
			  "Sent buffer did not match");
}

static void verify_tx_error(enum ec_host_cmd_status error)
{
	expected_dut_to_host->header.prtcl_ver = 3;
	expected_dut_to_host->header.result = error;
	expected_dut_to_host->header.data_len = 0;
	expected_dut_to_host->header.reserved = 0;
	update_dut_to_host_checksum();

	zassert_equal(sent.len, expected_tx_size(), "Sent bytes did not match");
	zassert_mem_equal(sent.buf, expected_dut_to_host, expected_tx_size(),
			  "Sent buffer did not match");
}

#define EC_CMD_HELLO 0x0001
static enum ec_host_cmd_status
ec_host_cmd_add(struct ec_host_cmd_handler_args *args)
{
	const struct ec_params_add *const request = args->input_buf;
	struct ec_response_add *const response = args->output_buf;

	if (args->version == 0) {
		response->out_data = request->in_data + 0x01020304;
	} else if (args->version == 1) {
		response->out_data = request->in_data + 0x02040608;
	} else if (args->version == 2) {
		return EC_HOST_CMD_OVERFLOW;
	} else {
		zassert_unreachable("Should not get version %d", args->version);
	}

	args->output_buf_size = sizeof(*response);
	return EC_HOST_CMD_SUCCESS;
}
EC_HOST_CMD_HANDLER(ec_host_cmd_add, EC_CMD_HELLO, BIT(0) | BIT(1) | BIT(2),
		    struct ec_params_add, struct ec_response_add);

static void test_add(void)
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
	expected_dut_to_host->add.out_data = 0x11223344;

	verify_tx_data();
}

static void test_add_version_2(void)
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
	expected_dut_to_host->add.out_data = 0x12243648;

	verify_tx_data();
}

static void test_add_invalid_version(void)
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

static void test_add_invalid_version_big(void)
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

static void test_add_invalid_prtcl_ver_2(void)
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

static void test_add_invalid_prtcl_ver_4(void)
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

static void test_add_invalid_rx_checksum(void)
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
	rv = ec_host_cmd_periph_sim_data_received(host_to_dut_buffer,
						  sizeof(host_to_dut_buffer));
	zassert_equal(rv, 0, "Could not send data %d", rv);

	/* Ensure Send was called so we can verify outputs */
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");

	verify_tx_error(EC_HOST_CMD_INVALID_CHECKSUM);
}

static void test_add_rx_size_too_small_for_header(void)
{
	int rv;

	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 2;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	rv = ec_host_cmd_periph_sim_data_received(host_to_dut_buffer, 4);
	zassert_equal(rv, 0, "Could not send data %d", rv);

	/* Ensure Send was called so we can verify outputs */
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");

	verify_tx_error(EC_HOST_CMD_REQUEST_TRUNCATED);
}

static void test_add_rx_size_too_small(void)
{
	int rv;

	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_HELLO;
	host_to_dut->header.cmd_ver = 2;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->add);
	host_to_dut->add.in_data = 0x10203040;

	rv = ec_host_cmd_periph_sim_data_received(
		host_to_dut_buffer,
		sizeof(host_to_dut->header) + host_to_dut->header.data_len - 1);
	zassert_equal(rv, 0, "Could not send data %d", rv);

	/* Ensure Send was called so we can verify outputs */
	rv = k_sem_take(&send_called, K_SECONDS(1));
	zassert_equal(rv, 0, "Send was not called");

	verify_tx_error(EC_HOST_CMD_REQUEST_TRUNCATED);
}

static void test_unknown_command(void)
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
	if (request->bytes_to_write > args->output_buf_size) {
		return EC_HOST_CMD_OVERFLOW;
	}

	for (int i = 0; i < request->bytes_to_write; ++i) {
		zassert_equal(out_buffer[i], 0, "Ensure every TX byte is 0");
		out_buffer[i] = i;
	}

	args->output_buf_size = request->bytes_to_write;
	return EC_HOST_CMD_SUCCESS;
}
EC_HOST_CMD_HANDLER_UNBOUND(ec_host_cmd_unbounded, EC_CMD_UNBOUNDED,
			    BIT(0) | BIT(1) | BIT(2));

static void test_unbounded_handler_error_return(void)
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

static void test_unbounded_handler_response_too_big(void)
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

static void test_rx_buffer_cleared_foreach_hostcommand(void)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_UNBOUNDED;
	host_to_dut->header.cmd_ver = 2;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->unbounded);
	host_to_dut->unbounded.bytes_to_write = 5;

	/* Write data after the entire request message. The host command handler
	 * always assert that this data is cleared upon receipt.
	 */
	host_to_dut->raw[4] = 42;

	simulate_rx_data();

	expected_dut_to_host->header.prtcl_ver = 3;
	expected_dut_to_host->header.result = 0;
	expected_dut_to_host->header.reserved = 0;
	expected_dut_to_host->header.data_len = 5;
	expected_dut_to_host->raw[0] = 0;
	expected_dut_to_host->raw[1] = 1;
	expected_dut_to_host->raw[2] = 2;
	expected_dut_to_host->raw[3] = 3;
	expected_dut_to_host->raw[4] = 4;

	verify_tx_data();
}

static void test_tx_buffer_cleared_foreach_hostcommand(void)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_UNBOUNDED;
	host_to_dut->header.cmd_ver = 2;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(host_to_dut->unbounded);
	host_to_dut->unbounded.bytes_to_write = 5;

	simulate_rx_data();

	expected_dut_to_host->header.prtcl_ver = 3;
	expected_dut_to_host->header.result = 0;
	expected_dut_to_host->header.reserved = 0;
	expected_dut_to_host->header.data_len = 5;
	expected_dut_to_host->raw[0] = 0;
	expected_dut_to_host->raw[1] = 1;
	expected_dut_to_host->raw[2] = 2;
	expected_dut_to_host->raw[3] = 3;
	expected_dut_to_host->raw[4] = 4;

	verify_tx_data();

	/* Send second command with less bytes to write. Host command handler
	 * asserts that the previous output data is zero.
	 */

	host_to_dut->unbounded.bytes_to_write = 2;
	simulate_rx_data();

	expected_dut_to_host->header.data_len = 2;
	verify_tx_data();
}

#define EC_CMD_TOO_BIG 0x0003
static enum ec_host_cmd_status
ec_host_cmd_too_big(struct ec_host_cmd_handler_args *args)
{
	return EC_HOST_CMD_SUCCESS;
}
EC_HOST_CMD_HANDLER(ec_host_cmd_too_big, EC_CMD_TOO_BIG, BIT(0), uint32_t,
		    struct ec_response_too_big);

static void test_response_always_too_big(void)
{
	host_to_dut->header.prtcl_ver = 3;
	host_to_dut->header.cmd_id = EC_CMD_TOO_BIG;
	host_to_dut->header.cmd_ver = 0;
	host_to_dut->header.reserved = 0;
	host_to_dut->header.data_len = sizeof(uint32_t);

	simulate_rx_data();

	verify_tx_error(EC_HOST_CMD_INVALID_RESPONSE);
}

void test_main(void)
{
	ec_host_cmd_periph_sim_install_send_cb(host_send);

	ztest_test_suite(
		ec_host_cmd_tests, ztest_unit_test(test_add),
		ztest_unit_test(test_add_version_2),
		ztest_unit_test(test_add_invalid_prtcl_ver_2),
		ztest_unit_test(test_add_invalid_prtcl_ver_4),
		ztest_unit_test(test_add_invalid_version),
		ztest_unit_test(test_add_invalid_version_big),
		ztest_unit_test(test_add_invalid_rx_checksum),
		ztest_unit_test(test_add_rx_size_too_small_for_header),
		ztest_unit_test(test_add_rx_size_too_small),
		ztest_unit_test(test_unknown_command),
		ztest_unit_test(test_unbounded_handler_error_return),
		ztest_unit_test(test_unbounded_handler_response_too_big),
		ztest_unit_test(test_rx_buffer_cleared_foreach_hostcommand),
		ztest_unit_test(test_tx_buffer_cleared_foreach_hostcommand),
		ztest_unit_test(test_response_always_too_big));

	ztest_run_test_suite(ec_host_cmd_tests);
}
