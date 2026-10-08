/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/uart/serial_test.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include "smp_test_util.h"

#define SMP_RESPONSE_WAIT_TIME   K_MSEC(100)
#define ZCBOR_BUFFER_SIZE        128
#define OUTPUT_BUFFER_SIZE       256
#define ZCBOR_HISTORY_ARRAY_SIZE 8
#define TEST_STRING              "tEsTiNg eChO dAtA"

/* With nodes of enabled transports, the transports must ignore the chosen UART */
#define HAS_NODES DT_NODE_EXISTS(DT_NODELABEL(uart_console0))

static const struct device *const uart_chosen = DEVICE_DT_GET(DT_NODELABEL(uart_chosen));
#if DT_NODE_EXISTS(DT_NODELABEL(uart_console0))
static const struct device *const uart_console0 = DEVICE_DT_GET(DT_NODELABEL(uart_console0));
static const struct device *const uart_raw0 = DEVICE_DT_GET(DT_NODELABEL(uart_raw0));
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(uart_console1))
static const struct device *const uart_console1 = DEVICE_DT_GET(DT_NODELABEL(uart_console1));
static const struct device *const uart_raw1 = DEVICE_DT_GET(DT_NODELABEL(uart_raw1));
#endif
static struct mcumgr_serial_rx_ctxt rx_ctxt;
static struct net_buf *nb;

#if HAS_NODES
static K_SEM_DEFINE(sysworkq_blocked, 0, 1);
static K_SEM_DEFINE(sysworkq_release, 0, 1);
static bool sysworkq_held;

/* Holds the system workqueue, which processes received SMP over console lines */
static void block_sysworkq(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&sysworkq_blocked);
	k_sem_take(&sysworkq_release, K_FOREVER);
}

static K_WORK_DEFINE(sysworkq_blocker, block_sysworkq);
#endif

#if defined(CONFIG_MCUMGR_TRANSPORT_UART)
/* Writes a framed request to the receive side of the test UART passed as context */
static int serial_vnd_rx_cb(const void *data, int len, void *ctx)
{
	return serial_vnd_queue_in_data(ctx, data, len) == len ? 0 : -ENOSPC;
}
#endif

static void cleanup_test(void *p)
{
	if (nb != NULL) {
		smp_packet_free(nb);
		nb = NULL;
	}

	if (rx_ctxt.nb != NULL) {
		smp_packet_free(rx_ctxt.nb);
		rx_ctxt.nb = NULL;
	}

#if HAS_NODES
	if (sysworkq_held) {
		k_sem_give(&sysworkq_release);
		sysworkq_held = false;
	}
#endif
}

ZTEST(transport_uart, test_echo)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	uint8_t tx_data[OUTPUT_BUFFER_SIZE];
	bool ok;
	int rc;
	uint16_t buffer_size;
	uint32_t tx_len;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = {0};
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = {0};
	struct smp_hdr *header;
	size_t decoded = 0;
	struct zcbor_string data = {0};

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("r", zcbor_tstr_decode, &data),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	memset(tx_data, 0, sizeof(tx_data));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Test 1: Send an echo request to every UART before reading any */
#if defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
	rc = serial_vnd_queue_in_data(uart_chosen, buffer_out, buffer_size);
	zassert_equal(rc, buffer_size, "Failed to queue request on the chosen UART");
#else
	rc = mcumgr_serial_tx_pkt(buffer_out, buffer_size, serial_vnd_rx_cb, (void *)uart_chosen);
	zassert_equal(rc, 0, "Failed to send request on the chosen UART: %d", rc);
#endif

#if HAS_NODES
	rc = mcumgr_serial_tx_pkt(buffer_out, buffer_size, serial_vnd_rx_cb, (void *)uart_console0);
	zassert_equal(rc, 0, "Failed to send request on uart_console0: %d", rc);
	rc = serial_vnd_queue_in_data(uart_raw0, buffer_out, buffer_size);
	zassert_equal(rc, buffer_size, "Failed to queue request on uart_raw0");
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(uart_console1))
	rc = mcumgr_serial_tx_pkt(buffer_out, buffer_size, serial_vnd_rx_cb, (void *)uart_console1);
	zassert_equal(rc, 0, "Failed to send request on uart_console1: %d", rc);
	rc = serial_vnd_queue_in_data(uart_raw1, buffer_out, buffer_size);
	zassert_equal(rc, buffer_size, "Failed to queue request on uart_raw1");
#endif

	k_sleep(SMP_RESPONSE_WAIT_TIME);

#if HAS_NODES
	/* Test 2: Check the chosen UART gets no response */
	tx_len = serial_vnd_read_out_data(uart_chosen, tx_data, sizeof(tx_data));
	zassert_equal(tx_len, 0, "Expected no response on the chosen UART");

	/* Test 3: Check the response on uart_console0 */
	tx_len = serial_vnd_read_out_data(uart_console0, tx_data, sizeof(tx_data));
	zassert_true(tx_len > 0, "Expected to receive a response");

	nb = mcumgr_serial_process_frag(&rx_ctxt, tx_data, tx_len);
	zassert_not_null(nb, "Expected a valid response");

	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP,
		      "Expected SMP opcode MGMT_OP_READ_RSP, got %d", header->nh_op);
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "Expected group MGMT_GROUP_ID_OS, got %d", sys_be16_to_cpu(header->nh_group));
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "Expected command ID OS_MGMT_ID_ECHO, got %d",
		      header->nh_id);

	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode response: %d", rc);
	zassert_equal(decoded, 1, "Expected to decode 1 zcbor element, got %zu", decoded);
	zassert_equal(data.len, strlen(TEST_STRING), "Unexpected value of data.len %zu", data.len);
	zassert_mem_equal(data.value, TEST_STRING, data.len, "os mgmt echo response mismatch");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	memset(tx_data, 0, sizeof(tx_data));
	memset(zsd, 0, sizeof(zsd));
	decoded = 0;

	/* Test 4: Check the response on uart_raw0 */
	tx_len = serial_vnd_read_out_data(uart_raw0, tx_data, sizeof(tx_data));
	zassert_true(tx_len > sizeof(struct smp_hdr), "Expected to receive a response");

	header = (struct smp_hdr *)tx_data;
	zassert_equal(sys_be16_to_cpu(header->nh_len), tx_len - sizeof(struct smp_hdr),
		      "Expected exactly one response");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP,
		      "Expected SMP opcode MGMT_OP_READ_RSP, got %d", header->nh_op);
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "Expected group MGMT_GROUP_ID_OS, got %d", sys_be16_to_cpu(header->nh_group));
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "Expected command ID OS_MGMT_ID_ECHO, got %d",
		      header->nh_id);

	zcbor_new_decode_state(zsd, 4, &tx_data[sizeof(struct smp_hdr)],
			       tx_len - sizeof(struct smp_hdr), 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode response: %d", rc);
	zassert_equal(decoded, 1, "Expected to decode 1 zcbor element, got %zu", decoded);
	zassert_equal(data.len, strlen(TEST_STRING), "Unexpected value of data.len %zu", data.len);
	zassert_mem_equal(data.value, TEST_STRING, data.len, "os mgmt echo response mismatch");

#if DT_NODE_EXISTS(DT_NODELABEL(uart_console1))
	/* Clean up test */
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	memset(tx_data, 0, sizeof(tx_data));
	memset(zsd, 0, sizeof(zsd));
	decoded = 0;

	/* Test 5: Check the response on uart_console1 */
	tx_len = serial_vnd_read_out_data(uart_console1, tx_data, sizeof(tx_data));
	zassert_true(tx_len > 0, "Expected to receive a response");

	nb = mcumgr_serial_process_frag(&rx_ctxt, tx_data, tx_len);
	zassert_not_null(nb, "Expected a valid response");

	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP,
		      "Expected SMP opcode MGMT_OP_READ_RSP, got %d", header->nh_op);
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "Expected group MGMT_GROUP_ID_OS, got %d", sys_be16_to_cpu(header->nh_group));
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "Expected command ID OS_MGMT_ID_ECHO, got %d",
		      header->nh_id);

	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode response: %d", rc);
	zassert_equal(decoded, 1, "Expected to decode 1 zcbor element, got %zu", decoded);
	zassert_equal(data.len, strlen(TEST_STRING), "Unexpected value of data.len %zu", data.len);
	zassert_mem_equal(data.value, TEST_STRING, data.len, "os mgmt echo response mismatch");

	/* Clean up test */
	cleanup_test(NULL);
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	memset(tx_data, 0, sizeof(tx_data));
	memset(zsd, 0, sizeof(zsd));
	decoded = 0;

	/* Test 6: Check the response on uart_raw1 */
	tx_len = serial_vnd_read_out_data(uart_raw1, tx_data, sizeof(tx_data));
	zassert_true(tx_len > sizeof(struct smp_hdr), "Expected to receive a response");

	header = (struct smp_hdr *)tx_data;
	zassert_equal(sys_be16_to_cpu(header->nh_len), tx_len - sizeof(struct smp_hdr),
		      "Expected exactly one response");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP,
		      "Expected SMP opcode MGMT_OP_READ_RSP, got %d", header->nh_op);
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "Expected group MGMT_GROUP_ID_OS, got %d", sys_be16_to_cpu(header->nh_group));
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "Expected command ID OS_MGMT_ID_ECHO, got %d",
		      header->nh_id);

	zcbor_new_decode_state(zsd, 4, &tx_data[sizeof(struct smp_hdr)],
			       tx_len - sizeof(struct smp_hdr), 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode response: %d", rc);
	zassert_equal(decoded, 1, "Expected to decode 1 zcbor element, got %zu", decoded);
	zassert_equal(data.len, strlen(TEST_STRING), "Unexpected value of data.len %zu", data.len);
	zassert_mem_equal(data.value, TEST_STRING, data.len, "os mgmt echo response mismatch");
#endif
#elif defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
	/* Test 2: Check the response on the chosen UART */
	tx_len = serial_vnd_read_out_data(uart_chosen, tx_data, sizeof(tx_data));
	zassert_true(tx_len > sizeof(struct smp_hdr), "Expected to receive a response");

	header = (struct smp_hdr *)tx_data;
	zassert_equal(sys_be16_to_cpu(header->nh_len), tx_len - sizeof(struct smp_hdr),
		      "Expected exactly one response");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP,
		      "Expected SMP opcode MGMT_OP_READ_RSP, got %d", header->nh_op);
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "Expected group MGMT_GROUP_ID_OS, got %d", sys_be16_to_cpu(header->nh_group));
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "Expected command ID OS_MGMT_ID_ECHO, got %d",
		      header->nh_id);

	zcbor_new_decode_state(zsd, 4, &tx_data[sizeof(struct smp_hdr)],
			       tx_len - sizeof(struct smp_hdr), 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode response: %d", rc);
	zassert_equal(decoded, 1, "Expected to decode 1 zcbor element, got %zu", decoded);
	zassert_equal(data.len, strlen(TEST_STRING), "Unexpected value of data.len %zu", data.len);
	zassert_mem_equal(data.value, TEST_STRING, data.len, "os mgmt echo response mismatch");
#else
	/* Test 2: Check the response on the chosen UART */
	tx_len = serial_vnd_read_out_data(uart_chosen, tx_data, sizeof(tx_data));
	zassert_true(tx_len > 0, "Expected to receive a response");

	nb = mcumgr_serial_process_frag(&rx_ctxt, tx_data, tx_len);
	zassert_not_null(nb, "Expected a valid response");

	header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP,
		      "Expected SMP opcode MGMT_OP_READ_RSP, got %d", header->nh_op);
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "Expected group MGMT_GROUP_ID_OS, got %d", sys_be16_to_cpu(header->nh_group));
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "Expected command ID OS_MGMT_ID_ECHO, got %d",
		      header->nh_id);

	zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode response: %d", rc);
	zassert_equal(decoded, 1, "Expected to decode 1 zcbor element, got %zu", decoded);
	zassert_equal(data.len, strlen(TEST_STRING), "Unexpected value of data.len %zu", data.len);
	zassert_mem_equal(data.value, TEST_STRING, data.len, "os mgmt echo response mismatch");
#endif
}

#if defined(CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT)
ZTEST(transport_uart, test_raw_input_timeout)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	uint8_t tx_data[OUTPUT_BUFFER_SIZE];
	bool ok;
	int rc;
	uint16_t buffer_size;
	uint32_t tx_len;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = {0};
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = {0};
	struct smp_hdr *header;
	size_t decoded = 0;
	struct zcbor_string data = {0};
#if HAS_NODES
	const struct device *raw_uart = uart_raw0;
#else
	const struct device *raw_uart = uart_chosen;
#endif

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("r", zcbor_tstr_decode, &data),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	memset(tx_data, 0, sizeof(tx_data));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Test 1: Send a partial request, which is discarded once the input timeout passes */
	rc = serial_vnd_queue_in_data(raw_uart, buffer_out, buffer_size - 4);
	zassert_equal(rc, buffer_size - 4, "Failed to queue partial request");

	k_sleep(K_MSEC(CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS * 3));

	tx_len = serial_vnd_read_out_data(raw_uart, tx_data, sizeof(tx_data));
	zassert_equal(tx_len, 0, "Expected no response to a partial request");

	/* Test 2: Send the full request and check the response */
	rc = serial_vnd_queue_in_data(raw_uart, buffer_out, buffer_size);
	zassert_equal(rc, buffer_size, "Failed to queue request");

	k_sleep(SMP_RESPONSE_WAIT_TIME);

	tx_len = serial_vnd_read_out_data(raw_uart, tx_data, sizeof(tx_data));
	zassert_true(tx_len > sizeof(struct smp_hdr), "Expected to receive a response");

	header = (struct smp_hdr *)tx_data;
	zassert_equal(sys_be16_to_cpu(header->nh_len), tx_len - sizeof(struct smp_hdr),
		      "Expected exactly one response");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP,
		      "Expected SMP opcode MGMT_OP_READ_RSP, got %d", header->nh_op);
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "Expected group MGMT_GROUP_ID_OS, got %d", sys_be16_to_cpu(header->nh_group));
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "Expected command ID OS_MGMT_ID_ECHO, got %d",
		      header->nh_id);

	zcbor_new_decode_state(zsd, 4, &tx_data[sizeof(struct smp_hdr)],
			       tx_len - sizeof(struct smp_hdr), 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode response: %d", rc);
	zassert_equal(decoded, 1, "Expected to decode 1 zcbor element, got %zu", decoded);
	zassert_equal(data.len, strlen(TEST_STRING), "Unexpected value of data.len %zu", data.len);
	zassert_mem_equal(data.value, TEST_STRING, data.len, "os mgmt echo response mismatch");

#if DT_NODE_EXISTS(DT_NODELABEL(uart_raw1))
	/* Clean up test */
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	memset(tx_data, 0, sizeof(tx_data));
	memset(zsd, 0, sizeof(zsd));
	decoded = 0;

	/* Test 3: Send a partial request to uart_raw1, which has its own input timeout */
	rc = serial_vnd_queue_in_data(uart_raw1, buffer_out, buffer_size - 4);
	zassert_equal(rc, buffer_size - 4, "Failed to queue partial request");

	k_sleep(K_MSEC(CONFIG_MCUMGR_TRANSPORT_RAW_UART_INPUT_TIMEOUT_TIME_MS * 3));

	tx_len = serial_vnd_read_out_data(uart_raw1, tx_data, sizeof(tx_data));
	zassert_equal(tx_len, 0, "Expected no response to a partial request");

	/* Test 4: Send the full request to uart_raw1 and check the response */
	rc = serial_vnd_queue_in_data(uart_raw1, buffer_out, buffer_size);
	zassert_equal(rc, buffer_size, "Failed to queue request");

	k_sleep(SMP_RESPONSE_WAIT_TIME);

	tx_len = serial_vnd_read_out_data(uart_raw1, tx_data, sizeof(tx_data));
	zassert_true(tx_len > sizeof(struct smp_hdr), "Expected to receive a response");

	header = (struct smp_hdr *)tx_data;
	zassert_equal(sys_be16_to_cpu(header->nh_len), tx_len - sizeof(struct smp_hdr),
		      "Expected exactly one response");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP,
		      "Expected SMP opcode MGMT_OP_READ_RSP, got %d", header->nh_op);
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "Expected group MGMT_GROUP_ID_OS, got %d", sys_be16_to_cpu(header->nh_group));
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "Expected command ID OS_MGMT_ID_ECHO, got %d",
		      header->nh_id);

	zcbor_new_decode_state(zsd, 4, &tx_data[sizeof(struct smp_hdr)],
			       tx_len - sizeof(struct smp_hdr), 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode response: %d", rc);
	zassert_equal(decoded, 1, "Expected to decode 1 zcbor element, got %zu", decoded);
	zassert_equal(data.len, strlen(TEST_STRING), "Unexpected value of data.len %zu", data.len);
	zassert_mem_equal(data.value, TEST_STRING, data.len, "os mgmt echo response mismatch");
#endif
}
#endif

#if HAS_NODES
ZTEST(transport_uart, test_rx_pool_isolation)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	uint8_t tx_data[OUTPUT_BUFFER_SIZE];
	bool ok;
	int rc;
	uint16_t buffer_size;
	uint32_t tx_len;
	uint8_t *frame;
	uint8_t *frame_end;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = {0};
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = {0};
	struct smp_hdr *header;
	size_t decoded = 0;
	struct zcbor_string data = {0};

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("r", zcbor_tstr_decode, &data),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	memset(tx_data, 0, sizeof(tx_data));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Test 1: Hold the system workqueue and send one line more than uart_console0 has receive
	 * buffers for, so its received lines wait in its receive buffers
	 */
	k_work_submit(&sysworkq_blocker);
	rc = k_sem_take(&sysworkq_blocked, K_SECONDS(1));
	zassert_equal(rc, 0, "Expected the system workqueue to be held");
	sysworkq_held = true;

	for (int i = 0; i <= CONFIG_UART_MCUMGR_RX_BUF_COUNT; i++) {
		rc = mcumgr_serial_tx_pkt(buffer_out, buffer_size, serial_vnd_rx_cb,
					  (void *)uart_console0);
		zassert_equal(rc, 0, "Failed to send request: %d", rc);
	}

	/* Test 2: Check uart_raw0, which has receive buffers of its own, still gets a response */
	rc = serial_vnd_queue_in_data(uart_raw0, buffer_out, buffer_size);
	zassert_equal(rc, buffer_size, "Failed to queue request");

	k_sleep(SMP_RESPONSE_WAIT_TIME);

	tx_len = serial_vnd_read_out_data(uart_raw0, tx_data, sizeof(tx_data));
	zassert_true(tx_len > sizeof(struct smp_hdr), "Expected to receive a response");

	header = (struct smp_hdr *)tx_data;
	zassert_equal(sys_be16_to_cpu(header->nh_len), tx_len - sizeof(struct smp_hdr),
		      "Expected exactly one response");
	zassert_equal(header->nh_op, MGMT_OP_READ_RSP,
		      "Expected SMP opcode MGMT_OP_READ_RSP, got %d", header->nh_op);
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
		      "Expected group MGMT_GROUP_ID_OS, got %d", sys_be16_to_cpu(header->nh_group));
	zassert_equal(header->nh_id, OS_MGMT_ID_ECHO, "Expected command ID OS_MGMT_ID_ECHO, got %d",
		      header->nh_id);

	zcbor_new_decode_state(zsd, 4, &tx_data[sizeof(struct smp_hdr)],
			       tx_len - sizeof(struct smp_hdr), 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode response: %d", rc);
	zassert_equal(decoded, 1, "Expected to decode 1 zcbor element, got %zu", decoded);
	zassert_equal(data.len, strlen(TEST_STRING), "Unexpected value of data.len %zu", data.len);
	zassert_mem_equal(data.value, TEST_STRING, data.len, "os mgmt echo response mismatch");

	/* Clean up test */
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	memset(tx_data, 0, sizeof(tx_data));
	memset(zsd, 0, sizeof(zsd));
	decoded = 0;

	/* Test 3: Release the system workqueue and check uart_console0 gets one response per
	 * receive buffer
	 */
	k_sem_give(&sysworkq_release);
	sysworkq_held = false;

	k_sleep(SMP_RESPONSE_WAIT_TIME);

	tx_len = serial_vnd_read_out_data(uart_console0, tx_data, sizeof(tx_data));
	zassert_true(tx_len > 0, "Expected to receive responses");

	frame = tx_data;

	for (int i = 0; i < CONFIG_UART_MCUMGR_RX_BUF_COUNT; i++) {
		frame_end = memchr(frame, '\n', &tx_data[tx_len] - frame);
		zassert_not_null(frame_end, "Expected a response per receive buffer");

		nb = mcumgr_serial_process_frag(&rx_ctxt, frame, frame_end - frame + 1);
		zassert_not_null(nb, "Expected a valid response");

		header = net_buf_pull_mem(nb, sizeof(struct smp_hdr));
		zassert_equal(header->nh_op, MGMT_OP_READ_RSP,
			      "Expected SMP opcode MGMT_OP_READ_RSP, got %d", header->nh_op);
		zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_OS),
			      "Expected group MGMT_GROUP_ID_OS, got %d",
			      sys_be16_to_cpu(header->nh_group));
		zassert_equal(header->nh_id, OS_MGMT_ID_ECHO,
			      "Expected command ID OS_MGMT_ID_ECHO, got %d", header->nh_id);

		zcbor_new_decode_state(zsd, 4, nb->data, nb->len, 1, NULL, 0);
		rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode),
					   &decoded);
		zassert_equal(rc, 0, "Failed to decode response: %d", rc);
		zassert_equal(decoded, 1, "Expected to decode 1 zcbor element, got %zu", decoded);
		zassert_equal(data.len, strlen(TEST_STRING), "Unexpected value of data.len %zu",
			      data.len);
		zassert_mem_equal(data.value, TEST_STRING, data.len,
				  "os mgmt echo response mismatch");

		/* Clean up test */
		cleanup_test(NULL);
		zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
		memset(zsd, 0, sizeof(zsd));
		decoded = 0;

		frame = frame_end + 1;
	}

	zassert_equal(frame, &tx_data[tx_len], "Expected exactly one response per receive buffer");
}
#endif

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT)
ZTEST(transport_uart, test_bridge)
{
	uint8_t buffer[ZCBOR_BUFFER_SIZE];
	uint8_t buffer_out[OUTPUT_BUFFER_SIZE];
	uint8_t tx_data[OUTPUT_BUFFER_SIZE];
	bool ok;
	int rc;
	uint16_t buffer_size;
	uint32_t tx_len;
	zcbor_state_t zse[ZCBOR_HISTORY_ARRAY_SIZE] = {0};
	zcbor_state_t zsd[ZCBOR_HISTORY_ARRAY_SIZE] = {0};
	struct smp_hdr *header;
	size_t decoded = 0;
	struct zcbor_string data = {0};

	struct zcbor_map_decode_key_val output_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("r", zcbor_tstr_decode, &data),
	};

	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	memset(tx_data, 0, sizeof(tx_data));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));

	/* Test 1: Bridge requests received on uart_raw0 to the SMP over console transport */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_transport_mgmt_connect_packet(zse, buffer, buffer_out, &buffer_size,
						  SMP_SERIAL_TRANSPORT);
	zassert_true(ok, "Expected packet creation to be successful");

	rc = serial_vnd_queue_in_data(uart_raw0, buffer_out, buffer_size);
	zassert_equal(rc, buffer_size, "Failed to queue request");

	k_sleep(SMP_RESPONSE_WAIT_TIME);

	tx_len = serial_vnd_read_out_data(uart_raw0, tx_data, sizeof(tx_data));
	zassert_true(tx_len > sizeof(struct smp_hdr), "Expected to receive a response");

	header = (struct smp_hdr *)tx_data;
	zassert_equal(sys_be16_to_cpu(header->nh_len), tx_len - sizeof(struct smp_hdr),
		      "Expected exactly one response");
	zassert_equal(header->nh_op, MGMT_OP_WRITE_RSP,
		      "Expected SMP opcode MGMT_OP_WRITE_RSP, got %d", header->nh_op);
	zassert_equal(header->nh_group, sys_cpu_to_be16(MGMT_GROUP_ID_TRANSPORT),
		      "Expected group MGMT_GROUP_ID_TRANSPORT, got %d",
		      sys_be16_to_cpu(header->nh_group));
	zassert_equal(header->nh_id, TRANSPORT_MGMT_ID_CONNECT,
		      "Expected command ID TRANSPORT_MGMT_ID_CONNECT, got %d", header->nh_id);

	zcbor_new_decode_state(zsd, 4, &tx_data[sizeof(struct smp_hdr)],
			       tx_len - sizeof(struct smp_hdr), 1, NULL, 0);
	rc = zcbor_map_decode_bulk(zsd, output_decode, ARRAY_SIZE(output_decode), &decoded);
	zassert_equal(rc, 0, "Failed to decode response: %d", rc);
	zassert_equal(decoded, 0, "Expected to decode 0 zcbor elements, got %zu", decoded);

	/* Clean up test */
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	memset(tx_data, 0, sizeof(tx_data));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));
	memset(zsd, 0, sizeof(zsd));
	zcbor_map_decode_bulk_reset(output_decode, ARRAY_SIZE(output_decode));
	decoded = 0;

	/* Test 2: Check an echo request on uart_raw0 goes out of uart_console0 unchanged */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING);
	zassert_true(ok, "Expected packet creation to be successful");

	rc = serial_vnd_queue_in_data(uart_raw0, buffer_out, buffer_size);
	zassert_equal(rc, buffer_size, "Failed to queue request");

	k_sleep(SMP_RESPONSE_WAIT_TIME);

	tx_len = serial_vnd_read_out_data(uart_console0, tx_data, sizeof(tx_data));
	zassert_true(tx_len > 0, "Expected to receive the request");

	nb = mcumgr_serial_process_frag(&rx_ctxt, tx_data, tx_len);
	zassert_not_null(nb, "Expected a valid request");
	zassert_equal(nb->len, buffer_size, "Expected the request unchanged");
	zassert_mem_equal(nb->data, buffer_out, buffer_size, "Expected the request unchanged");

	/* Clean up test */
	cleanup_test(NULL);
	memset(buffer, 0, sizeof(buffer));
	memset(buffer_out, 0, sizeof(buffer_out));
	memset(tx_data, 0, sizeof(tx_data));
	buffer_size = 0;
	memset(zse, 0, sizeof(zse));

	/* Test 3: Check its response received on uart_console0 goes out of uart_raw0 unchanged */
	zcbor_new_encode_state(zse, CBOR_MAP_STATES, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_response_packet(zse, buffer, buffer_out, &buffer_size,
						 TEST_STRING);
	zassert_true(ok, "Expected packet creation to be successful");

	rc = mcumgr_serial_tx_pkt(buffer_out, buffer_size, serial_vnd_rx_cb, (void *)uart_console0);
	zassert_equal(rc, 0, "Failed to send response: %d", rc);

	k_sleep(SMP_RESPONSE_WAIT_TIME);

	tx_len = serial_vnd_read_out_data(uart_raw0, tx_data, sizeof(tx_data));
	zassert_equal(tx_len, buffer_size, "Expected the response unchanged");
	zassert_mem_equal(tx_data, buffer_out, buffer_size, "Expected the response unchanged");

	/* Test 4: Disconnect the bridge */
	rc = transport_mgmt_disconnect_all();
	zassert_equal(rc, 0, "Expected bridge to be disconnected");
}
#endif

ZTEST_SUITE(transport_uart, NULL, NULL, NULL, cleanup_test, NULL);
