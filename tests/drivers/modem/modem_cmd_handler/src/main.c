/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2025, Joakim Andersson
 */

/**
 * @addtogroup t_modem_driver
 * @{
 * @defgroup t_modem_cmd_handler test_modem_cmd_handler
 * @}
 */


#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <modem_cmd_handler.h>

DEFINE_FFF_GLOBALS;

static struct modem_cmd_handler_data cmd_handler_data;
static struct modem_cmd_handler cmd_handler;
static struct modem_iface mock_modem_iface;
static struct k_sem sem_response;
#define MDM_RECV_BUF_SIZE 512
NET_BUF_POOL_DEFINE(_mdm_recv_pool, 10, MDM_RECV_BUF_SIZE, 0, NULL);

uint8_t cmd_match_buf[MDM_RECV_BUF_SIZE + 1];

MODEM_CMD_DEFINE(mock_on_cmd_ok)
{
	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&sem_response);
	return 0;
}

MODEM_CMD_DEFINE(mock_on_cmd_error)
{
	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&sem_response);
	return 0;
}

static const struct modem_cmd mock_response_cmds[] = {
	MODEM_CMD("OK", mock_on_cmd_ok, 0U, ""), /* 3GPP */
	MODEM_CMD("ERROR", mock_on_cmd_error, 0U, ""), /* 3GPP */
};

static const struct modem_cmd_handler_config cmd_handler_config = {
	.match_buf = &cmd_match_buf[0],
	.match_buf_len = sizeof(cmd_match_buf),
	.buf_pool = &_mdm_recv_pool,
	.alloc_timeout = K_NO_WAIT,
	.eol = "\r",
	.user_data = NULL,
	.response_cmds = mock_response_cmds,
	.response_cmds_len = ARRAY_SIZE(mock_response_cmds),
	.unsol_cmds = NULL,
	.unsol_cmds_len = 0,
};

static const char *response_delayed;
size_t mock_write_expected_data_count;
static const char *mock_write_expected_data[10];

FAKE_VALUE_FUNC(int, mock_read, struct modem_iface *, uint8_t *, size_t, size_t *);
FAKE_VALUE_FUNC(int, mock_write, struct modem_iface *, const uint8_t *, size_t);
FAKE_VALUE_FUNC(int, mock_on_response, struct modem_cmd_handler_data *, uint16_t,
		uint8_t **, uint16_t);

static void mock_modem_iface_init(struct modem_iface *iface)
{
	iface->read = mock_read;
	iface->write = mock_write;
}

static int mock_modem_iface_receive_data(struct modem_iface *iface,
					 uint8_t *data,
					 size_t len, size_t *bytes_read)
{
	if (!response_delayed) {
		*bytes_read = 0;
		return mock_read_fake.return_val;
	}

	size_t response_delayed_len = strlen(response_delayed);

	zassert_true(len > response_delayed_len, "Insufficient data length for response");
	memcpy(data, response_delayed, response_delayed_len);
	*bytes_read = response_delayed_len;
	response_delayed = NULL;

	return mock_read_fake.return_val;
}

static void _send_response_delayed_work(struct k_work *work)
{
	mock_read_fake.custom_fake = mock_modem_iface_receive_data;
	modem_cmd_handler_process(&cmd_handler, &mock_modem_iface);
}

K_WORK_DELAYABLE_DEFINE(response_delayable, _send_response_delayed_work);

void recv_data_delayed(const char *str, k_timeout_t delay)
{
	response_delayed = str;
	k_work_schedule(&response_delayable, delay);
}

static int modem_modem_iface_send_data(struct modem_iface *iface,
				       const uint8_t *data,
				       size_t len)
{
	zassert_true(strncmp((const char *)data,
			     mock_write_expected_data[mock_write_fake.call_count - 1], len) == 0,
				 "Sent command does not match expected");
	return mock_write_fake.return_val;
}

void send_data_verify(const char *expected_cmd)
{
	mock_write_expected_data[mock_write_expected_data_count++] = expected_cmd;
	mock_write_fake.custom_fake = modem_modem_iface_send_data;
}

ZTEST(suite_modem_cmd_send, test_recv_ok)
{
	int ret = modem_cmd_handler_init(&cmd_handler,
					 &cmd_handler_data,
					 &cmd_handler_config);
	zassert_equal(ret, 0, "modem_cmd_handler_init should return 0 on success");

	send_data_verify("AT+INIT");
	send_data_verify("\r");

	recv_data_delayed("OK\r", K_MSEC(100));

	ret = modem_cmd_send(&mock_modem_iface, &cmd_handler,
			     NULL, 0U, "AT+INIT", &sem_response,
			     K_SECONDS(1));
	zassert_equal(ret, 0, "modem_cmd_send should return 0 on success");
}

ZTEST(suite_modem_cmd_send, test_recv_error)
{
	int ret = modem_cmd_handler_init(&cmd_handler,
					 &cmd_handler_data,
					 &cmd_handler_config);
	zassert_equal(ret, 0, "modem_cmd_handler_init should return 0 on success");

	send_data_verify("AT+INIT");
	send_data_verify("\r");

	recv_data_delayed("ERROR\r", K_MSEC(100));

	ret = modem_cmd_send(&mock_modem_iface, &cmd_handler,
			     NULL, 0U, "AT+INIT", &sem_response,
			     K_SECONDS(1));
	zassert_equal(ret, -EIO, "modem_cmd_send should return -EIO on error");
}

ZTEST(suite_modem_cmd_send, test_recv_timeout)
{
	int ret = modem_cmd_handler_init(&cmd_handler,
					 &cmd_handler_data,
					 &cmd_handler_config);
	zassert_equal(ret, 0, "modem_cmd_handler_init should return 0 on success");

	send_data_verify("AT+INIT");
	send_data_verify("\r");

	/* No response is sent to trigger timeout */

	ret = modem_cmd_send(&mock_modem_iface, &cmd_handler,
			     NULL, 0U, "AT+INIT", &sem_response,
			     K_SECONDS(1));
	zassert_equal(ret, -ETIMEDOUT, "modem_cmd_send should return -ETIMEDOUT on timeout");
}

MODEM_CMD_DEFINE(on_cmd_response)
{
	zassert_equal(argc, 1);
	zassert_equal(strcmp(argv[0], "123"), 0);
	return 0;
}

ZTEST(suite_modem_cmd_send, test_recv_response)
{
	int ret = modem_cmd_handler_init(&cmd_handler,
					 &cmd_handler_data,
					 &cmd_handler_config);
	zassert_equal(ret, 0, "modem_cmd_handler_init should return 0 on success");

	send_data_verify("AT+CMD");
	send_data_verify("\r");

	recv_data_delayed("+CMD: 123\r"
			  "OK\r",
			  K_MSEC(100));

	mock_on_response_fake.custom_fake = on_cmd_response;
	struct modem_cmd cmd[] = {
		MODEM_CMD("+CMD: ", mock_on_response, 1, ""),
	};

	ret = modem_cmd_send(&mock_modem_iface, &cmd_handler,
			     cmd, ARRAY_SIZE(cmd), "AT+CMD", &sem_response,
			     K_SECONDS(1));
	zassert_equal(ret, 0, "modem_cmd_send should return 0 on success");
	zassert_equal(mock_on_response_fake.call_count, 1);
}

MODEM_CMD_DEFINE(on_cmd_response_parse_args)
{
	zassert_equal(argc, 4);
	zassert_equal(strcmp(argv[0], "1"), 0);
	zassert_equal(strcmp(argv[1], "\"two\""), 0);
	zassert_equal(strcmp(argv[2], "\"three\""), 0);
	zassert_equal(strcmp(argv[3], "4"), 0);
	return 0;
}

ZTEST(suite_modem_cmd_send, test_recv_response_parse_args)
{
	int ret = modem_cmd_handler_init(&cmd_handler,
					 &cmd_handler_data,
					 &cmd_handler_config);
	zassert_equal(ret, 0, "modem_cmd_handler_init should return 0 on success");

	send_data_verify("AT+CMD");
	send_data_verify("\r");

	recv_data_delayed("+CMD: 1,\"two\",\"three\",4\r"
			  "OK\r",
			  K_MSEC(100));

	mock_on_response_fake.custom_fake = on_cmd_response_parse_args;
	struct modem_cmd cmd[] = {
		MODEM_CMD("+CMD: ", mock_on_response, 4, ","),
	};

	ret = modem_cmd_send(&mock_modem_iface, &cmd_handler,
			     cmd, ARRAY_SIZE(cmd), "AT+CMD", &sem_response,
			     K_SECONDS(1));
	zassert_equal(ret, 0, "modem_cmd_send should return 0 on success");
	zassert_equal(mock_on_response_fake.call_count, 1);
}

MODEM_CMD_DEFINE(on_cmd_response_parse_args_quoted_delim)
{
	zassert_equal(argc, 4);
	zassert_equal(strcmp(argv[0], "1"), 0);
	zassert_equal(strcmp(argv[1], "\"two\""), 0);
	zassert_equal(strcmp(argv[2], "\"thr,ee\""), 0);
	zassert_equal(strcmp(argv[3], "4"), 0);
	return 0;
}

ZTEST(suite_modem_cmd_send, test_recv_response_parse_args_quoted_delim)
{
	int ret = modem_cmd_handler_init(&cmd_handler,
					 &cmd_handler_data,
					 &cmd_handler_config);
	zassert_equal(ret, 0, "modem_cmd_handler_init should return 0 on success");

	send_data_verify("AT+CMD");
	send_data_verify("\r");

	recv_data_delayed("+CMD: 1,\"two\",\"thr,ee\",4\r"
			  "OK\r",
			  K_MSEC(100));

	mock_on_response_fake.custom_fake = on_cmd_response_parse_args_quoted_delim;
	struct modem_cmd cmd[] = {
		MODEM_CMD("+CMD: ", mock_on_response, 4, ","),
	};

	ret = modem_cmd_send(&mock_modem_iface, &cmd_handler,
			     cmd, ARRAY_SIZE(cmd), "AT+CMD", &sem_response,
			     K_SECONDS(1));
	zassert_equal(ret, 0, "modem_cmd_send should return 0 on success");
	zassert_equal(mock_on_response_fake.call_count, 1);
}

MODEM_CMD_DEFINE(on_cmd_response_parse_args_empty_arg)
{
	zassert_equal(argc, 4);
	zassert_equal(strcmp(argv[0], "1"), 0);
	zassert_equal(strcmp(argv[1], "\"two\""), 0);
	zassert_equal(strcmp(argv[2], ""), 0);
	zassert_equal(strcmp(argv[3], "4"), 0);
	return 0;
}

ZTEST(suite_modem_cmd_send, test_recv_response_parse_args_empty_arg)
{
	int ret = modem_cmd_handler_init(&cmd_handler,
					 &cmd_handler_data,
					 &cmd_handler_config);
	zassert_equal(ret, 0, "modem_cmd_handler_init should return 0 on success");

	send_data_verify("AT+CMD");
	send_data_verify("\r");

	recv_data_delayed("+CMD: 1,\"two\",,4\r"
			  "OK\r",
			  K_MSEC(100));

	mock_on_response_fake.custom_fake = on_cmd_response_parse_args_empty_arg;
	struct modem_cmd cmd[] = {
		MODEM_CMD("+CMD: ", mock_on_response, 4, ","),
	};

	ret = modem_cmd_send(&mock_modem_iface, &cmd_handler,
			     cmd, ARRAY_SIZE(cmd), "AT+CMD", &sem_response,
			     K_SECONDS(1));
	zassert_equal(ret, 0, "modem_cmd_send should return 0 on success");
	zassert_equal(mock_on_response_fake.call_count, 1);
}

MODEM_CMD_DEFINE(on_cmd_response_parse_args_empty_arg_end)
{
	zassert_equal(argc, 4);
	zassert_equal(strcmp(argv[0], "1"), 0);
	zassert_equal(strcmp(argv[1], "\"two\""), 0);
	zassert_equal(strcmp(argv[2], "\"three\""), 0);
	zassert_equal(strcmp(argv[3], ""), 0);
	return 0;
}

ZTEST(suite_modem_cmd_send, test_recv_response_parse_args_empty_arg_end)
{
	int ret = modem_cmd_handler_init(&cmd_handler,
					 &cmd_handler_data,
					 &cmd_handler_config);
	zassert_equal(ret, 0, "modem_cmd_handler_init should return 0 on success");

	send_data_verify("AT+CMD");
	send_data_verify("\r");

	recv_data_delayed("+CMD: 1,\"two\",\"three\",\r"
			  "OK\r",
			  K_MSEC(100));

	mock_on_response_fake.custom_fake = on_cmd_response_parse_args_empty_arg_end;
	struct modem_cmd cmd[] = {
		MODEM_CMD("+CMD: ", mock_on_response, 4, ","),
	};

	ret = modem_cmd_send(&mock_modem_iface, &cmd_handler,
			     cmd, ARRAY_SIZE(cmd), "AT+CMD", &sem_response,
			     K_SECONDS(1));
	zassert_equal(ret, 0, "modem_cmd_send should return 0 on success");
	zassert_equal(mock_on_response_fake.call_count, 1);
}

MODEM_CMD_DEFINE(on_cmd_response_parse_args_empty_arg_begin)
{
	zassert_equal(argc, 4);
	zassert_equal(strcmp(argv[0], ""), 0);
	zassert_equal(strcmp(argv[1], "\"two\""), 0);
	zassert_equal(strcmp(argv[2], "\"three\""), 0);
	zassert_equal(strcmp(argv[3], "4"), 0);
	return 0;
}

ZTEST(suite_modem_cmd_send, test_recv_response_parse_args_empty_arg_begin)
{
	int ret = modem_cmd_handler_init(&cmd_handler,
					 &cmd_handler_data,
					 &cmd_handler_config);
	zassert_equal(ret, 0, "modem_cmd_handler_init should return 0 on success");

	send_data_verify("AT+CMD");
	send_data_verify("\r");

	recv_data_delayed("+CMD: ,\"two\",\"three\",4\r"
			  "OK\r",
			  K_MSEC(100));

	mock_on_response_fake.custom_fake = on_cmd_response_parse_args_empty_arg_begin;
	struct modem_cmd cmd[] = {
		MODEM_CMD("+CMD: ", mock_on_response, 4, ","),
	};

	ret = modem_cmd_send(&mock_modem_iface, &cmd_handler,
			     cmd, ARRAY_SIZE(cmd), "AT+CMD", &sem_response,
			     K_SECONDS(1));
	zassert_equal(ret, 0, "modem_cmd_send should return 0 on success");
	zassert_equal(mock_on_response_fake.call_count, 1);
}

MODEM_CMD_DEFINE(on_cmd_response_parse_args_multi_delim)
{
	zassert_equal(argc, 4);
	zassert_equal(strcmp(argv[0], "1"), 0);
	zassert_equal(strcmp(argv[1], "\"two\""), 0);
	zassert_equal(strcmp(argv[2], "\"three\""), 0);
	zassert_equal(strcmp(argv[3], "4"), 0);
	return 0;
}

ZTEST(suite_modem_cmd_send, test_recv_response_parse_args_multi_delim)
{
	int ret = modem_cmd_handler_init(&cmd_handler,
					 &cmd_handler_data,
					 &cmd_handler_config);
	zassert_equal(ret, 0, "modem_cmd_handler_init should return 0 on success");

	send_data_verify("AT+CMD");
	send_data_verify("\r");

	recv_data_delayed("+CMD: 1:\"two\";\"three\",4\r"
			  "OK\r",
			  K_MSEC(100));

	mock_on_response_fake.custom_fake = on_cmd_response_parse_args_multi_delim;
	struct modem_cmd cmd[] = {
		MODEM_CMD("+CMD: ", mock_on_response, 4, ",;:"),
	};

	ret = modem_cmd_send(&mock_modem_iface, &cmd_handler,
			     cmd, ARRAY_SIZE(cmd), "AT+CMD", &sem_response,
			     K_SECONDS(1));
	zassert_equal(ret, 0, "modem_cmd_send should return 0 on success");
	zassert_equal(mock_on_response_fake.call_count, 1);
}

static void test_setup(void *fixture)
{
	mock_write_expected_data_count = 0;

	RESET_FAKE(mock_read);
	RESET_FAKE(mock_write);
	RESET_FAKE(mock_on_response);

	k_sem_init(&sem_response, 0, 1);

	mock_modem_iface_init(&mock_modem_iface);
}

static void test_teardown(void *fixture)
{
	zassert_equal(mock_write_fake.call_count, mock_write_expected_data_count,
		     "Not all expected commands were sent");
}

ZTEST_SUITE(suite_modem_cmd_send, NULL, NULL, test_setup, test_teardown, NULL);
