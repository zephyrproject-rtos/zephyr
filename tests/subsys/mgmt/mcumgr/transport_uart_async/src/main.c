/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/net_buf.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
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

static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_uart_mcumgr));
static struct mcumgr_serial_rx_ctxt rx_ctxt;
static struct net_buf *nb;

/* Writes a framed request to the receive side of the emulated UART */
static int uart_emul_rx_cb(const void *data, int len)
{
	return uart_emul_put_rx_data(uart_dev, data, len) == len ? 0 : -ENOSPC;
}

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
}

ZTEST(transport_uart_async, test_rx_restart)
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

	zassert_true(device_is_ready(uart_dev), "UART device not ready");

	zcbor_new_encode_state(zse, 2, buffer, ARRAY_SIZE(buffer), 0);
	ok = create_os_mgmt_echo_packet(zse, buffer, buffer_out, &buffer_size, TEST_STRING);
	zassert_true(ok, "Expected packet creation to be successful");

	/* Test 1: Send an echo request and check the response */
	rc = mcumgr_serial_tx_pkt(buffer_out, buffer_size, uart_emul_rx_cb);
	zassert_equal(rc, 0, "Failed to send request: %d", rc);

	k_sleep(SMP_RESPONSE_WAIT_TIME);

	tx_len = uart_emul_get_tx_data(uart_dev, tx_data, sizeof(tx_data));
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

	/* Test 2: Stop reception, then send the echo request again and check the response. An RX
	 * error stops reception with the same UART_RX_DISABLED event as uart_rx_disable().
	 */
	rc = uart_rx_disable(uart_dev);
	zassert_equal(rc, 0, "Failed to disable RX: %d", rc);

	k_sleep(SMP_RESPONSE_WAIT_TIME);

	rc = mcumgr_serial_tx_pkt(buffer_out, buffer_size, uart_emul_rx_cb);
	zassert_equal(rc, 0, "Failed to send request: %d", rc);

	k_sleep(SMP_RESPONSE_WAIT_TIME);

	tx_len = uart_emul_get_tx_data(uart_dev, tx_data, sizeof(tx_data));
	zassert_true(tx_len > 0, "Expected to receive a response after RX stopped");

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
}

ZTEST_SUITE(transport_uart_async, NULL, NULL, NULL, cleanup_test, NULL);
