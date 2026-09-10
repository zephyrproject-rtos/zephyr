/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_mock.h>

#include "uart_mock.h"

#define CMD_HEADER_SIZE		   (sizeof(struct ec_host_cmd_request_header))
#define RSP_HEADER_SIZE		   (sizeof(struct ec_host_cmd_response_header))
/* Recovery time for the backend from invalid command. It has to be bigger than the RX timeout. */
#define UART_BACKEND_RECOVERY_TIME K_MSEC(160)
#define MAX_RESP_WAIT_TIME	   K_MSEC(1)

static uint8_t cal_checksum(const uint8_t *const buffer, const uint16_t size)
{
	uint8_t checksum = 0;

	for (size_t i = 0; i < size; ++i) {
		checksum += buffer[i];
	}
	return (uint8_t)(-checksum);
}

static void tx_done(void)
{
	struct uart_event evt;
	struct uart_mock_data *data = uart_mock.data;

	/* Prepare UART event passed to the UART callback */
	evt.type = UART_TX_DONE;
	evt.data.tx.buf = data->tx_buf;
	evt.data.tx.len = data->tx_len;

	data->cb(&uart_mock, &evt, data->user_data);
}

#define EC_CMD_HELLO 0x0001
#define EC_HELLO_STR "hello_ec"

const static uint8_t hello_magic[4] = {0xAB, 0xBC, 0xDE, 0xF1};
struct hello_cmd_data {
	uint8_t magic[sizeof(hello_magic)];
} __packed;

static enum ec_host_cmd_status ec_host_cmd_hello(struct ec_host_cmd_handler_args *args)
{
	const struct hello_cmd_data *cmd_data = args->input_buf;

	args->output_buf_size = 0;

	if (args->version != 0) {
		zassert_unreachable("Should not get version %d", args->version);
		return EC_HOST_CMD_INVALID_VERSION;
	}

	if (args->input_buf_size != sizeof(struct hello_cmd_data)) {
		return EC_HOST_CMD_INVALID_PARAM;
	}

	if (memcmp(hello_magic, cmd_data->magic, sizeof(hello_magic))) {
		return EC_HOST_CMD_INVALID_PARAM;
	}

	memcpy(args->output_buf, EC_HELLO_STR, sizeof(EC_HELLO_STR));
	args->output_buf_size = sizeof(EC_HELLO_STR);

	return EC_HOST_CMD_SUCCESS;
}
EC_HOST_CMD_HANDLER_UNBOUND(EC_CMD_HELLO, ec_host_cmd_hello, BIT(0));

static void prepare_hello_cmd(uint8_t *buf)
{
	struct ec_host_cmd_request_header *cmd = (struct ec_host_cmd_request_header *)buf;
	struct hello_cmd_data *cmd_data = (struct hello_cmd_data *)(buf + CMD_HEADER_SIZE);

	memset(cmd, 0, CMD_HEADER_SIZE);
	cmd->cmd_id = EC_CMD_HELLO;
	cmd->cmd_ver = 0;
	cmd->prtcl_ver = 3;
	cmd->data_len = sizeof(*cmd_data);
	memcpy(cmd_data->magic, hello_magic, sizeof(hello_magic));
	cmd->checksum = cal_checksum((uint8_t *)cmd, CMD_HEADER_SIZE + sizeof(*cmd_data));
}

static void test_hello(void)
{
	struct uart_event evt;
	struct uart_mock_data *data = uart_mock.data;
	struct ec_host_cmd_request_header *cmd = (struct ec_host_cmd_request_header *)data->rx_buf;
	uint8_t tx_buf[RSP_HEADER_SIZE + sizeof(EC_HELLO_STR)];
	struct ec_host_cmd_response_header *rsp = (struct ec_host_cmd_response_header *)tx_buf;
	int ret;

	/* Prepare command request */
	prepare_hello_cmd((uint8_t *)cmd);

	/* Prepare UART event passed to the UART callback */
	evt.type = UART_RX_RDY;
	evt.data.rx.len = CMD_HEADER_SIZE + sizeof(struct hello_cmd_data);
	evt.data.rx.offset = 0;
	evt.data.rx.buf = data->rx_buf;

	/* Prepare expected response to the Hello command */
	memset(rsp, 0, RSP_HEADER_SIZE);
	rsp->data_len = sizeof(EC_HELLO_STR);
	rsp->prtcl_ver = 3;
	rsp->result = 0;
	memcpy(&tx_buf[RSP_HEADER_SIZE], EC_HELLO_STR, sizeof(EC_HELLO_STR));
	rsp->checksum = cal_checksum(tx_buf, sizeof(tx_buf));

	/* Set expected data set from the EC */
	ztest_expect_value(uart_mock_tx, len, RSP_HEADER_SIZE + sizeof(EC_HELLO_STR));
	ztest_expect_data(uart_mock_tx, buf, tx_buf);

	/* Call the UART callback to inform about a new data */
	data->cb(&uart_mock, &evt, data->user_data);

	/* Let the handler handle command */
	ret = k_sem_take(&data->resp_sent, MAX_RESP_WAIT_TIME);

	zassert_equal(ret, 0, "Response not sent");

	tx_done();
}

/* Test recovering from overrun(receiving more data than the header indicates)*/
ZTEST(ec_host_cmd, test_recovery_from_overrun)
{
	struct uart_mock_data *data = uart_mock.data;
	struct ec_host_cmd_request_header *cmd = (struct ec_host_cmd_request_header *)data->rx_buf;
	struct uart_event evt;
	int ret;

	/* Header that indicates 0 data bytes */
	memset(cmd, 0, CMD_HEADER_SIZE);
	cmd->prtcl_ver = 3;
	cmd->data_len = 0;

	evt.type = UART_RX_RDY;
	evt.data.rx.len = CMD_HEADER_SIZE + 1;
	evt.data.rx.offset = 1;
	evt.data.rx.buf = data->rx_buf;

	/* Call the UART callback to inform about a new data */
	data->cb(&uart_mock, &evt, data->user_data);

	/* Make sure we don't get response */
	ret = k_sem_take(&data->resp_sent, UART_BACKEND_RECOVERY_TIME);
	zassert_equal(ret, -EAGAIN, "Got unexpected response");

	/* Make sure the backend is ready to receive a new command again */
	test_hello();
}

/* Test recovering from receiving invalid header*/
ZTEST(ec_host_cmd, test_recovery_from_invalid_header)
{
	int ret;
	struct uart_event evt;
	struct uart_mock_data *data = uart_mock.data;
	struct ec_host_cmd_request_header *cmd = (struct ec_host_cmd_request_header *)data->rx_buf;
	/* Different types of invalid header */
	struct ec_host_cmd_request_header cmds[] = {
		{
			.prtcl_ver = 3,
			.data_len = data->rx_buf_size + 1 - CMD_HEADER_SIZE,
		},
		{
			.prtcl_ver = 2,
			.data_len = 0,
		}};

	for (int i = 0; i < ARRAY_SIZE(cmds); i++) {
		memset(cmd, 0, CMD_HEADER_SIZE);
		cmd->prtcl_ver = cmds[i].prtcl_ver;
		cmd->data_len = cmds[i].data_len;

		evt.type = UART_RX_RDY;
		evt.data.rx.len = CMD_HEADER_SIZE;
		evt.data.rx.offset = 0;
		evt.data.rx.buf = data->rx_buf;

		/* Call the UART callback to inform about a new data */
		data->cb(&uart_mock, &evt, data->user_data);

		/* Make sure we don't get response */
		ret = k_sem_take(&data->resp_sent, UART_BACKEND_RECOVERY_TIME);
		zassert_equal(ret, -EAGAIN, "Got unexpected response");

		/* Make sure the backend is ready to receive a new command again */
		test_hello();
	}
}

/* Test recovering from receiving data that exceed buf size*/
ZTEST(ec_host_cmd, test_recovery_from_too_much_data)
{
	struct uart_event evt;
	int ret;
	struct uart_mock_data *data = uart_mock.data;

	/* One big chunk larger that the buff size */
	evt.type = UART_RX_RDY;
	evt.data.rx.len = data->rx_buf_size + 1;
	evt.data.rx.offset = 0;
	evt.data.rx.buf = data->rx_buf;

	/* Call the UART callback to inform about a new data */
	data->cb(&uart_mock, &evt, data->user_data);

	/* Make sure we don't get response */
	ret = k_sem_take(&data->resp_sent, UART_BACKEND_RECOVERY_TIME);
	zassert_equal(ret, -EAGAIN, "Got unexpected response");

	/* Make sure the backend is ready to receive a new command again */
	test_hello();

	/* Two chunks larger than the buf size */
	evt.type = UART_RX_RDY;
	evt.data.rx.len = CMD_HEADER_SIZE - 1;
	evt.data.rx.offset = 0;
	evt.data.rx.buf = data->rx_buf;

	/* Call the UART callback to inform about a new data */
	data->cb(&uart_mock, &evt, data->user_data);

	evt.type = UART_RX_RDY;
	evt.data.rx.len = data->rx_buf_size;
	evt.data.rx.offset = CMD_HEADER_SIZE - 1;
	evt.data.rx.buf = data->rx_buf;

	/* Call the UART callback to inform about a new data */
	data->cb(&uart_mock, &evt, data->user_data);

	/* Make sure we don't get response */
	ret = k_sem_take(&data->resp_sent, UART_BACKEND_RECOVERY_TIME);
	zassert_equal(ret, -EAGAIN, "Got response to incomplete command");

	/* Make sure the backend is ready to receive a new command again */
	test_hello();
}

/* Test recovering from incomplete command */
ZTEST(ec_host_cmd, test_recovery_from_underrun)
{
	struct uart_event evt;
	struct uart_mock_data *data = uart_mock.data;
	uint8_t *cmd = data->rx_buf;
	const size_t cmd_size = CMD_HEADER_SIZE + sizeof(struct hello_cmd_data);
	/* Test different types of underrun */
	size_t size_to_send[] = {CMD_HEADER_SIZE - 1, CMD_HEADER_SIZE, cmd_size - 1};
	int ret;

	for (int i = 0; i < ARRAY_SIZE(size_to_send); i++) {
		/* Prepare command request */
		prepare_hello_cmd((uint8_t *)cmd);
		memset(cmd + size_to_send[i], 0, cmd_size - size_to_send[i]);

		/* Prepare UART event passed to the UART callback */
		evt.type = UART_RX_RDY;
		evt.data.rx.len = size_to_send[i];
		evt.data.rx.offset = 0;
		evt.data.rx.buf = cmd;

		/* Call the UART callback to inform about a new data */
		data->cb(&uart_mock, &evt, data->user_data);

		/* Make sure we don't get response */
		ret = k_sem_take(&data->resp_sent, UART_BACKEND_RECOVERY_TIME);
		zassert_equal(ret, -EAGAIN, "Got unexpected response");

		/* Make sure the backend is ready to receive a new command again */
		test_hello();
	}
}

/* Test basic hello command */
ZTEST(ec_host_cmd, test_hello)
{
	test_hello();
}

static void *ec_host_cmd_tests_setup(void)
{
	struct uart_mock_data *data = uart_mock.data;

	k_sem_init(&data->resp_sent, 0, 1);
	ec_host_cmd_init(ec_host_cmd_backend_get_uart(&uart_mock));
	return NULL;
}

ZTEST_SUITE(ec_host_cmd, NULL, ec_host_cmd_tests_setup, NULL, NULL, NULL);
