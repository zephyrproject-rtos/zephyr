/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(CONFIG_BUILD_DATE_TIME_TEST) && !defined(CONFIG_LIMITED_TEST)

#include <zephyr/ztest.h>
#include <zephyr/net/buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_dummy.h>
#include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <os_mgmt_processor.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <version.h>
#include <string.h>
#include <smp_internal.h>
#include "smp_test_util.h"

#define SMP_RESPONSE_WAIT_TIME 3
#define QUERY_BUFFER_SIZE 16
#define ZCBOR_BUFFER_SIZE 256
#define OUTPUT_BUFFER_SIZE 256
#define ZCBOR_HISTORY_ARRAY_SIZE 4
#define QUERY_TEST_CMD_BITMASK OS_MGMT_INFO_FORMAT_USER_CUSTOM_START

/* Test sets */
enum {
	OS_MGMT_TEST_SET_MAIN = 0,
#ifdef CONFIG_MCUMGR_GRP_OS_INFO_CUSTOM_HOOKS
	OS_MGMT_TEST_SET_CUSTOM_OS,
	OS_MGMT_TEST_SET_CUSTOM_OS_DISABLED,
	OS_MGMT_TEST_SET_CUSTOM_CMD_DISABLED,
	OS_MGMT_TEST_SET_CUSTOM_CMD,
	OS_MGMT_TEST_SET_CUSTOM_CMD_DISABLED_VERIFY,
#endif

	OS_MGMT_TEST_SET_COUNT
};

/* Test os_mgmt info command requesting 's' (kernel name) */
static const uint8_t command[] = {
	0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x01, 0x07,
	0xbf, 0x66, 0x66, 0x6f, 0x72, 0x6d, 0x61, 0x74,
	0x61, 0x73, 0xff
};

/* Expected response from mcumgr */
static const uint8_t expected_response[] = {
	0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x01, 0x07,
	0xbf, 0x66, 0x6f, 0x75, 0x74, 0x70, 0x75, 0x74,
	0x66, 0x5a, 0x65, 0x70, 0x68, 0x79, 0x72, 0xff
};

static struct net_buf *nb;

struct state {
	uint8_t test_set;
};

static struct state test_state = {
	.test_set = 0,
};

/* Responses to commands */
const uint8_t response_kernel_name[] = "Zephyr";

#if defined(CONFIG_BT)
const uint8_t response_node_name[] = CONFIG_BT_DEVICE_NAME;
#elif defined(CONFIG_NET_HOSTNAME_ENABLE)
const uint8_t response_node_name[] = CONFIG_NET_HOSTNAME;
#else
const uint8_t response_node_name[] = "unknown";
#endif

const uint8_t response_kernel_release[] = STRINGIFY(BUILD_VERSION);
const uint8_t response_kernel_version[] = KERNEL_VERSION_STRING;
const uint8_t response_machine[] = CONFIG_ARCH;
const uint8_t response_processor[] = PROCESSOR_NAME;
const uint8_t response_board_revision[] = CONFIG_BOARD "@" CONFIG_BOARD_REVISION;
const uint8_t response_board[] = CONFIG_BOARD;
const uint8_t response_os[] = "Zephyr";
const uint8_t response_custom_cmd[] = "Magic Output for Test";
const uint8_t response_os_custom[] = CONFIG_CUSTOM_OS_NAME_VALUE;

const uint8_t response_all_board_revision[] = "Zephyr "
#if defined(CONFIG_BT)
					      CONFIG_BT_DEVICE_NAME
#elif defined(CONFIG_NET_HOSTNAME_ENABLE)
					      CONFIG_NET_HOSTNAME
#else
					      "unknown"
#endif
					      " " STRINGIFY(BUILD_VERSION) " "
					      KERNEL_VERSION_STRING " " CONFIG_ARCH " "
					      PROCESSOR_NAME " " CONFIG_BOARD "@"
					      CONFIG_BOARD_REVISION " Zephyr";

const uint8_t response_all[] = "Zephyr "
#if defined(CONFIG_BT)
			       CONFIG_BT_DEVICE_NAME
#elif defined(CONFIG_NET_HOSTNAME_ENABLE)
			       CONFIG_NET_HOSTNAME
#else
			       "unknown"
#endif
			       " " STRINGIFY(BUILD_VERSION) " " KERNEL_VERSION_STRING " "
			       CONFIG_ARCH " " PROCESSOR_NAME " " CONFIG_BOARD " Zephyr";

const uint8_t query_kernel_name[] = "s";
const uint8_t query_node_name[] = "n";
const uint8_t query_kernel_release[] = "r";
const uint8_t query_kernel_version[] = "v";
const uint8_t query_machine[] = "m";
const uint8_t query_processor[] = "p";
const uint8_t query_platform[] = "i";
const uint8_t query_os[] = "o";
const uint8_t query_all[] = "a";
const uint8_t query_test_cmd[] = "k";

#ifdef CONFIG_MCUMGR_GRP_OS_INFO_CUSTOM_HOOKS
static int32_t os_mgmt_info_custom_os_callback(uint32_t event, int32_t rc, bool *abort_more,
					       void *data, size_t data_size)
{
	if (event == MGMT_EVT_OP_OS_MGMT_INFO_CHECK) {
		struct os_mgmt_info_check *check_data = (struct os_mgmt_info_check *)data;

		*check_data->custom_os_name = true;
	} else if (event == MGMT_EVT_OP_OS_MGMT_INFO_APPEND) {
		int rc;
		struct os_mgmt_info_append *append_data = (struct os_mgmt_info_append *)data;

		if (*append_data->format_bitmask & OS_MGMT_INFO_FORMAT_OPERATING_SYSTEM) {
			rc = snprintf(&append_data->output[*append_data->output_length],
				      (append_data->buffer_size - *append_data->output_length),
				      "%s%s", (*append_data->prior_output == true ? " " : ""),
				      CONFIG_CUSTOM_OS_NAME_VALUE);

			if (rc < 0 ||
			    rc >= (append_data->buffer_size - *append_data->output_length)) {
				*abort_more = true;
				return -1;
			}

			*append_data->output_length += (uint16_t)rc;
			*append_data->prior_output = true;
			*append_data->format_bitmask &= ~OS_MGMT_INFO_FORMAT_OPERATING_SYSTEM;
		}
	}

	return MGMT_ERR_EOK;
}

static struct mgmt_callback custom_os_check_callback = {
	.callback = os_mgmt_info_custom_os_callback,
	.event_id = MGMT_EVT_OP_OS_MGMT_INFO_CHECK,
};

static struct mgmt_callback custom_os_append_callback = {
	.callback = os_mgmt_info_custom_os_callback,
	.event_id = MGMT_EVT_OP_OS_MGMT_INFO_APPEND,
};

static int32_t os_mgmt_info_custom_cmd_callback(uint32_t event, int32_t rc, bool *abort_more,
						void *data, size_t data_size)
{
	if (event == MGMT_EVT_OP_OS_MGMT_INFO_CHECK) {
		struct os_mgmt_info_check *check_data = (struct os_mgmt_info_check *)data;
		size_t i = 0;

		while (i < check_data->format->len) {
			if (check_data->format->value[i] == query_test_cmd[0]) {
				*check_data->format_bitmask |= QUERY_TEST_CMD_BITMASK;
				++(*check_data->valid_formats);
			}

			++i;
		}
	} else if (event == MGMT_EVT_OP_OS_MGMT_INFO_APPEND) {
		int rc;
		struct os_mgmt_info_append *append_data = (struct os_mgmt_info_append *)data;

		if (append_data->all_format_specified ||
		    (*append_data->format_bitmask & QUERY_TEST_CMD_BITMASK)) {
			rc = snprintf(&append_data->output[*append_data->output_length],
				      (append_data->buffer_size - *append_data->output_length),
				      "%sMagic Output for Test",
				      (*append_data->prior_output == true ? " " : ""));

			if (rc < 0 ||
			    rc >= (append_data->buffer_size - *append_data->output_length)) {
				*abort_more = true;
				return -1;
			}

			*append_data->output_length += (uint16_t)rc;
			*append_data->prior_output = true;
			*append_data->format_bitmask &= ~QUERY_TEST_CMD_BITMASK;
		}
	}

	return MGMT_ERR_EOK;
}

static struct mgmt_callback custom_cmd_check_callback = {
	.callback = os_mgmt_info_custom_cmd_callback,
	.event_id = (MGMT_EVT_OP_OS_MGMT_INFO_CHECK | MGMT_EVT_OP_OS_MGMT_INFO_APPEND),
};
#endif

ZTEST(os_mgmt_info, test_info_1)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(command, sizeof(command));
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	bool received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	zassert_equal(sizeof(expected_response), nb->len,
		      "Expected to receive %d bytes but got %d\n", sizeof(expected_response),
		      nb->len);

	zassert_mem_equal(expected_response, nb->data, nb->len,
			  "Expected received data mismatch");

	net_buf_unref(nb);

	/* Generate the same command dynamically */
	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, "s", buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Ensure the dynamically-generated size and payload matches the expected payload */
	zassert_equal(sizeof(command), buffer_size,
			  "Expected received data mismatch");
	zassert_mem_equal(command, buffer_out, sizeof(command),
			  "Expected received data mismatch");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	zassert_equal(sizeof(expected_response), nb->len,
		      "Expected to receive %d bytes but got %d\n", sizeof(expected_response),
		      nb->len);

	zassert_mem_equal(expected_response, nb->data, nb->len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info, test_info_2_kernel_name)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_kernel_name, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal((sizeof(response_kernel_name) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n",
		      (sizeof(response_kernel_name) - 1), output.len);

	zassert_mem_equal(response_kernel_name, output.value, output.len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info, test_info_3_node_name)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_node_name, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal((sizeof(response_node_name) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n",
		      (sizeof(response_node_name) - 1), output.len);

	zassert_mem_equal(response_node_name, output.value, output.len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info, test_info_4_kernel_release)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_kernel_release, buffer, buffer_out,
					 &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal((sizeof(response_kernel_release) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n",
		      (sizeof(response_kernel_release) - 1), output.len);

	zassert_mem_equal(response_kernel_release, output.value, output.len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info, test_info_5_kernel_version)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_kernel_version, buffer, buffer_out,
					 &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal((sizeof(response_kernel_version) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n",
		      (sizeof(response_kernel_version) - 1), output.len);

	zassert_mem_equal(response_kernel_version, output.value, output.len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info, test_info_6_machine)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_machine, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal((sizeof(response_machine) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n", (sizeof(response_machine) - 1),
		      output.len);

	zassert_mem_equal(response_machine, output.value, output.len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info, test_info_7_processor)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_processor, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal((sizeof(response_processor) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n", (sizeof(response_processor) - 1),
		      output.len);

	zassert_mem_equal(response_processor, output.value, output.len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info, test_info_8_platform)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_platform, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	if (sizeof(CONFIG_BOARD_REVISION) > 1) {
		/* Check with board revision */
		zassert_equal((sizeof(response_board_revision) - 1), output.len,
			      "Expected to receive %d bytes but got %d\n",
			      (sizeof(response_board_revision) - 1), output.len);

		zassert_mem_equal(response_board_revision, output.value, output.len,
				  "Expected received data mismatch");
	} else {
		/* Check without board revision */
		zassert_equal((sizeof(response_board) - 1), output.len,
			      "Expected to receive %d bytes but got %d\n",
			      (sizeof(response_board) - 1), output.len);

		zassert_mem_equal(response_board, output.value, output.len,
				  "Expected received data mismatch");
	}
}

ZTEST(os_mgmt_info, test_info_9_os)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_os, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal((sizeof(response_os) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n", (sizeof(response_os) - 1),
		      output.len);

	zassert_mem_equal(response_os, output.value, output.len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info, test_info_10_all)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_all, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	if (sizeof(CONFIG_BOARD_REVISION) > 1) {
		/* Check with board revision */
		zassert_equal((sizeof(response_all_board_revision) - 1), output.len,
			      "Expected to receive %d bytes but got %d\n",
			      (sizeof(response_all_board_revision) - 1), output.len);

		zassert_mem_equal(response_all_board_revision, output.value, output.len,
				  "Expected received data mismatch");
	} else {
		/* Check without board revision */
		zassert_equal((sizeof(response_all) - 1), output.len,
			      "Expected to receive %d bytes but got %d\n",
			      (sizeof(response_all) - 1), output.len);

		zassert_mem_equal(response_all, output.value, output.len,
				  "Expected received data mismatch");
	}
}

ZTEST(os_mgmt_info, test_info_11_multi_1)
{
	uint8_t query[QUERY_BUFFER_SIZE];
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Construct query for processor, kernel release and OS name */
	sprintf(query, "%s%s%s", query_processor, query_kernel_release, query_os);
	ok = create_mcumgr_format_packet(zse, query, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	/* Construct expected response to be compared against */
	sprintf(buffer, "%s %s %s", response_kernel_release, response_processor, response_os);

	zassert_equal(strlen(buffer), output.len, "Expected to receive %d bytes but got %d\n",
		      strlen(buffer), output.len);

	zassert_mem_equal(buffer, output.value, output.len, "Expected received data mismatch");
}

ZTEST(os_mgmt_info, test_info_12_multi_2)
{
	uint8_t query[QUERY_BUFFER_SIZE];
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Construct query for node name and kernel version (twice) */
	sprintf(query, "%s%s%s", query_kernel_version, query_node_name, query_kernel_version);
	ok = create_mcumgr_format_packet(zse, query, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	/* Construct expected response to be compared against, only 2 responses will be returned
	 * despite 3 being sent, because 2 were duplicates
	 */
	sprintf(buffer, "%s %s", response_node_name, response_kernel_version);

	zassert_equal(strlen(buffer), output.len, "Expected to receive %d bytes but got %d\n",
		      strlen(buffer), output.len);

	zassert_mem_equal(buffer, output.value, output.len, "Expected received data mismatch");
}

ZTEST(os_mgmt_info, test_info_13_invalid_1)
{
	uint8_t query[QUERY_BUFFER_SIZE];
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;
	int32_t rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	struct zcbor_map_decode_key_val error_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Construct query for node name with invalid specifier */
	sprintf(query, "%sM", query_kernel_version);
	ok = create_mcumgr_format_packet(zse, query, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	/* Ensure only an error is received */
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor element\n");

	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);
	ok = zcbor_map_decode_bulk(zsd, error_decode, ARRAY_SIZE(error_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");
	zassert_equal(output.len, 0, "Expected to receive 0 bytes but got %d\n", output.len);
	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected to receive EINVAL error but got %d\n", rc);
}

ZTEST(os_mgmt_info, test_info_14_invalid_2)
{
	uint8_t query[QUERY_BUFFER_SIZE];
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;
	int32_t rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	struct zcbor_map_decode_key_val error_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	/* Construct query for processor with invalid specifier */
	sprintf(query, "2%s", query_processor);
	ok = create_mcumgr_format_packet(zse, query, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	/* Ensure only an error is received */
	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor element\n");

	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);
	ok = zcbor_map_decode_bulk(zsd, error_decode, ARRAY_SIZE(error_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");
	zassert_equal(output.len, 0, "Expected to receive 0 bytes but got %d\n", output.len);
	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected to receive EINVAL error but got %d\n", rc);
}

#ifdef CONFIG_MCUMGR_GRP_OS_INFO_CUSTOM_HOOKS
static void *setup_custom_os(void)
{
	mgmt_callback_register(&custom_os_check_callback);
	mgmt_callback_register(&custom_os_append_callback);
	return NULL;
}

static void destroy_custom_os(void *p)
{
	mgmt_callback_unregister(&custom_os_check_callback);
	mgmt_callback_unregister(&custom_os_append_callback);
}

ZTEST(os_mgmt_info_custom_os, test_info_os_custom)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_os, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal((sizeof(response_os_custom) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n",
		      (sizeof(response_os_custom) - 1), output.len);

	zassert_mem_equal(response_os_custom, output.value, output.len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info_custom_os_disabled, test_info_os_custom_disabled)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_os, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal((sizeof(response_os) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n",
		      (sizeof(response_os) - 1), output.len);

	zassert_mem_equal(response_os, output.value, output.len,
			  "Expected received data mismatch");
}

static void *setup_custom_cmd(void)
{
	mgmt_callback_register(&custom_cmd_check_callback);

	return NULL;
}

static void destroy_custom_cmd(void *p)
{
	mgmt_callback_unregister(&custom_cmd_check_callback);
}

ZTEST(os_mgmt_info_custom_cmd, test_info_cmd_custom)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_test_cmd, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal((sizeof(response_custom_cmd) - 1), output.len,
		      "Expected to receive %d bytes but got %d\n",
		      (sizeof(response_custom_cmd) - 1), output.len);

	zassert_mem_equal(response_custom_cmd, output.value, output.len,
			  "Expected received data mismatch");
}

ZTEST(os_mgmt_info_custom_cmd_disabled, test_info_cmd_custom_disabled)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;
	int32_t rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	struct zcbor_map_decode_key_val error_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_test_cmd, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor element\n");

	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);
	ok = zcbor_map_decode_bulk(zsd, error_decode, ARRAY_SIZE(error_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal(output.len, 0, "Expected to receive 0 bytes but got %d\n", output.len);

	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected to receive EINVAL error but got %d\n", rc);
}

ZTEST(os_mgmt_info_custom_cmd_disabled_verify, test_info_cmd_custom_disabled)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	bool ok;
	uint16_t buffer_size;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = { 0 };
	bool received;
	struct zcbor_string output = { 0 };
	size_t decoded = 0;
	int32_t rc;

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("output", zcbor_tstr_decode, &output),
	};

	struct zcbor_map_decode_key_val error_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_int32_decode, &rc),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);

	ok = create_mcumgr_format_packet(zse, query_test_cmd, buffer, buffer_out, &buffer_size);
	zassert_true(ok, "Expected packet creation to be successful\n");

	/* Enable dummy SMP backend and ready for usage */
	smp_dummy_enable();
	smp_dummy_clear_state();

	/* Send query command to dummy SMP backend */
	(void)smp_dummy_tx_pkt(buffer_out, buffer_size);
	smp_dummy_add_data();

	/* For a short duration to see if response has been received */
	received = smp_dummy_wait_for_data(SMP_RESPONSE_WAIT_TIME);

	zassert_true(received, "Expected to receive data but timed out\n");

	/* Retrieve response buffer and ensure validity */
	nb = smp_dummy_get_outgoing();
	smp_dummy_disable();

	/* Process received data by removing header */
	(void)net_buf_pull(nb, sizeof(struct smp_hdr));
	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);

	ok = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 0, "Expected to receive 0 decoded zcbor element\n");

	zcbor_new_decode_state(zsd, 3, nb->data, nb->len, 1);
	ok = zcbor_map_decode_bulk(zsd, error_decode, ARRAY_SIZE(error_decode), &decoded) == 0;

	zassert_true(ok, "Expected decode to be successful\n");
	zassert_equal(decoded, 1, "Expected to receive 1 decoded zcbor element\n");

	zassert_equal(output.len, 0, "Expected to receive 0 bytes but got %d\n", output.len);

	zassert_equal(rc, MGMT_ERR_EINVAL, "Expected to receive EINVAL error but got %d\n", rc);

}
#endif

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		net_buf_unref(nb);
		nb = NULL;
	}
}

void test_main(void)
{
	while (test_state.test_set < OS_MGMT_TEST_SET_COUNT) {
		ztest_run_all(&test_state);
		++test_state.test_set;
	}

	ztest_verify_all_test_suites_ran();
}

static bool main_predicate(const void *state)
{
	return ((struct state *)state)->test_set == OS_MGMT_TEST_SET_MAIN;
}

#ifdef CONFIG_MCUMGR_GRP_OS_INFO_CUSTOM_HOOKS
static bool custom_os_predicate(const void *state)
{
	return ((struct state *)state)->test_set == OS_MGMT_TEST_SET_CUSTOM_OS;
}

static bool custom_os_disabled_predicate(const void *state)
{
	return ((struct state *)state)->test_set == OS_MGMT_TEST_SET_CUSTOM_OS_DISABLED;
}

static bool custom_cmd_disabled_predicate(const void *state)
{
	return ((struct state *)state)->test_set == OS_MGMT_TEST_SET_CUSTOM_CMD_DISABLED;
}

static bool custom_cmd_predicate(const void *state)
{
	return ((struct state *)state)->test_set == OS_MGMT_TEST_SET_CUSTOM_CMD;
}

static bool custom_cmd_disabled_verify_predicate(const void *state)
{
	return ((struct state *)state)->test_set == OS_MGMT_TEST_SET_CUSTOM_CMD_DISABLED_VERIFY;
}
#endif

/* Main test set */
ZTEST_SUITE(os_mgmt_info, main_predicate, NULL, NULL, cleanup_test, NULL);

#ifdef CONFIG_MCUMGR_GRP_OS_INFO_CUSTOM_HOOKS
/* Custom OS test set */
ZTEST_SUITE(os_mgmt_info_custom_os, custom_os_predicate, setup_custom_os, NULL, cleanup_test,
	    destroy_custom_os);
ZTEST_SUITE(os_mgmt_info_custom_os_disabled, custom_os_disabled_predicate, NULL, NULL,
	    cleanup_test, NULL);

/* Custom cmd test set */
ZTEST_SUITE(os_mgmt_info_custom_cmd_disabled, custom_cmd_disabled_predicate, NULL, NULL,
	    cleanup_test, NULL);
ZTEST_SUITE(os_mgmt_info_custom_cmd, custom_cmd_predicate, setup_custom_cmd, NULL, cleanup_test,
	    destroy_custom_cmd);
ZTEST_SUITE(os_mgmt_info_custom_cmd_disabled_verify, custom_cmd_disabled_verify_predicate, NULL,
	    NULL, cleanup_test, NULL);
#endif

#endif
