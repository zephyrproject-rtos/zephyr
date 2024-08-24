/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*************************************************************************************************/
/*                                        Dependencies                                           */
/*************************************************************************************************/
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <string.h>

#include <zephyr/modem/chat.h>
#include <modem_backend_mock.h>

/*************************************************************************************************/
/*                                         Instances                                             */
/*************************************************************************************************/
static struct modem_chat cmd;
static uint8_t cmd_delimiter[] = {'\r', '\n'};
static uint8_t cmd_receive_buf[128];
static uint8_t *cmd_argv[32];
static uint32_t cmd_user_data = 0x145212;

static struct modem_backend_mock mock;
static uint8_t mock_rx_buf[128];
static uint8_t mock_tx_buf[128];
static struct modem_pipe *mock_pipe;

/*************************************************************************************************/
/*                                        Track callbacks                                        */
/*************************************************************************************************/
#define MODEM_CHAT_UTEST_ON_IMEI_CALLED_BIT		 (0)
#define MODEM_CHAT_UTEST_ON_CREG_CALLED_BIT		 (1)
#define MODEM_CHAT_UTEST_ON_CGREG_CALLED_BIT		 (2)
#define MODEM_CHAT_UTEST_ON_QENG_SERVINGCELL_CALLED_BIT	 (3)
#define MODEM_CHAT_UTEST_ON_NO_CARRIER_CALLED_BIT	 (4)
#define MODEM_CHAT_UTEST_ON_ERROR_CALLED_BIT		 (5)
#define MODEM_CHAT_UTEST_ON_RDY_CALLED_BIT		 (6)
#define MODEM_CHAT_UTEST_ON_APP_RDY_CALLED_BIT		 (7)
#define MODEM_CHAT_UTEST_ON_NORMAL_POWER_DOWN_CALLED_BIT (8)
#define MODEM_CHAT_UTEST_ON_SCRIPT_CALLBACK_BIT		 (9)
#define MODEM_CHAT_UTEST_ON_CMGL_PARTIAL_CALLED_BIT	 (10)
#define MODEM_CHAT_UTEST_ON_CMGL_PARTIAL_ANY_CALLED_BIT	 (11)

static atomic_t callback_called;

/*************************************************************************************************/
/*                                  Script callbacks args copy                                   */
/*************************************************************************************************/
static uint8_t argv_buffers[32][128];
static uint16_t argc_buffers;

static void clone_args(char **argv, uint16_t argc)
{
	argc_buffers = argc;

	for (uint16_t i = 0; i < argc; i++) {
		memcpy(argv_buffers[i], argv[i], strlen(argv[i]) + 1);
	}
}

/*************************************************************************************************/
/*                                   Script match callbacks                                      */
/*************************************************************************************************/
static void on_imei(struct modem_chat *cmd, char **argv, uint16_t argc, void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_IMEI_CALLED_BIT);
	clone_args(argv, argc);
}

static void on_creg(struct modem_chat *cmd, char **argv, uint16_t argc, void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_CREG_CALLED_BIT);
	clone_args(argv, argc);
}

static void on_cgreg(struct modem_chat *cmd, char **argv, uint16_t argc, void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_CGREG_CALLED_BIT);
	clone_args(argv, argc);
}

static void on_qeng_serving_cell(struct modem_chat *cmd, char **argv, uint16_t argc,
				 void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_QENG_SERVINGCELL_CALLED_BIT);
	clone_args(argv, argc);
}

static void on_no_carrier(struct modem_chat *cmd, char **argv, uint16_t argc, void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_NO_CARRIER_CALLED_BIT);
	clone_args(argv, argc);
}

static void on_error(struct modem_chat *cmd, char **argv, uint16_t argc, void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_ERROR_CALLED_BIT);
	clone_args(argv, argc);
}

static void on_rdy(struct modem_chat *cmd, char **argv, uint16_t argc, void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_RDY_CALLED_BIT);
	clone_args(argv, argc);
}

static void on_app_rdy(struct modem_chat *cmd, char **argv, uint16_t argc, void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_APP_RDY_CALLED_BIT);
	clone_args(argv, argc);
}

static void on_normal_power_down(struct modem_chat *cmd, char **argv, uint16_t argc,
				 void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_NORMAL_POWER_DOWN_CALLED_BIT);
	clone_args(argv, argc);
}

static void on_cmgl_partial(struct modem_chat *cmd, char **argv, uint16_t argc, void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_CMGL_PARTIAL_CALLED_BIT);
	clone_args(argv, argc);
}

static void on_cmgl_any_partial(struct modem_chat *cmd, char **argv, uint16_t argc,
				void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_CMGL_PARTIAL_ANY_CALLED_BIT);
	clone_args(argv, argc);
}

/*************************************************************************************************/
/*                                       Script callback                                         */
/*************************************************************************************************/
static enum modem_chat_script_result script_result;
static void *script_result_user_data;

static void on_script_result(struct modem_chat *cmd, enum modem_chat_script_result result,
			     void *user_data)
{
	atomic_set_bit(&callback_called, MODEM_CHAT_UTEST_ON_SCRIPT_CALLBACK_BIT);
	script_result = result;
	script_result_user_data = user_data;
}

/*************************************************************************************************/
/*                                            Script                                             */
/*************************************************************************************************/
MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCH_DEFINE(imei_match, "", "", on_imei);
MODEM_CHAT_MATCH_DEFINE(creg_match, "CREG: ", ",", on_creg);
MODEM_CHAT_MATCH_DEFINE(cgreg_match, "CGREG: ", ",", on_cgreg);
MODEM_CHAT_MATCH_DEFINE(qeng_servinc_cell_match, "+QENG: \"servingcell\",", ",",
			on_qeng_serving_cell);

MODEM_CHAT_MATCHES_DEFINE(unsol_matches, MODEM_CHAT_MATCH("RDY", "", on_rdy),
			  MODEM_CHAT_MATCH("APP RDY", "", on_app_rdy),
			  MODEM_CHAT_MATCH("NORMAL POWER DOWN", "", on_normal_power_down));

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("AT", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("ATE0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("IMEI?", imei_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CREG?;+CGREG?", creg_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", cgreg_match), MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+QENG=\"servingcell\"", qeng_servinc_cell_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("", ok_match));

MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("NO CARRIER", "", on_no_carrier),
			  MODEM_CHAT_MATCH("ERROR ", ",:", on_error));

MODEM_CHAT_SCRIPT_DEFINE(script, script_cmds, abort_matches, on_script_result, 4);

/*************************************************************************************************/
/*                             Script implementing partial matches                               */
/*************************************************************************************************/
MODEM_CHAT_MATCHES_DEFINE(
	cmgl_matches,
	MODEM_CHAT_MATCH_INITIALIZER("+CMGL: ", ",", on_cmgl_partial, false, true),
	MODEM_CHAT_MATCH_INITIALIZER("", "", on_cmgl_any_partial, false, true),
	MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, false)
);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	script_partial_cmds,
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CMGL=4", cmgl_matches),
);

MODEM_CHAT_SCRIPT_DEFINE(script_partial, script_partial_cmds, abort_matches, on_script_result, 4);

/*************************************************************************************************/
/*                         Script containing timeout script chat command                         */
/*************************************************************************************************/
MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	script_timeout_cmd_cmds,
	MODEM_CHAT_SCRIPT_CMD_RESP("AT", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("", 4000),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT", ok_match),
);

MODEM_CHAT_SCRIPT_DEFINE(script_timeout_cmd, script_timeout_cmd_cmds, abort_matches,
			 on_script_result, 10);

/*************************************************************************************************/
/*                           Small echo script and mock transactions                             */
/*************************************************************************************************/
static const uint8_t at_echo_data[] = {'A', 'T', '\r', '\n'};
static const struct modem_backend_mock_transaction at_echo_transaction = {
	.get = at_echo_data,
	.get_size = sizeof(at_echo_data),
	.put = at_echo_data,
	.put_size = sizeof(at_echo_data),
};

static const uint8_t at_echo_error_data[] = {'E', 'R', 'R', 'O', 'R', ' ', '1', '\r', '\n'};
static const struct modem_backend_mock_transaction at_echo_error_transaction = {
	.get = at_echo_data,
	.get_size = sizeof(at_echo_data),
	.put = at_echo_error_data,
	.put_size = sizeof(at_echo_error_data),
};

MODEM_CHAT_MATCH_DEFINE(at_match, "AT", "", NULL);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	script_echo_cmds,
	MODEM_CHAT_SCRIPT_CMD_RESP("AT", at_match),
);

MODEM_CHAT_SCRIPT_DEFINE(script_echo, script_echo_cmds, abort_matches, on_script_result, 4);

/*************************************************************************************************/
/*                                      Script responses                                         */
/*************************************************************************************************/
static const char at_response[] = "AT\r\n";
static const char ok_response[] = "OK\r\n";
static const char imei_response[] = "23412354123123\r\n";
static const char creg_response[] = "CREG: 1,2\r\n";
static const char cgreg_response[] = "CGREG: 10,43\r\n";

static const char qeng_servinc_cell_response[] = "+QENG: \"servingcell\",\"NOCONN\",\"GSM\",260"
						 ",03,E182,AEAD,52,32,2,-68,255,255,0,38,38,1,,"
						 ",,,,,,,,\r\n";

static const char cmgl_response_0[] = "+CMGL: 1,1,,50\r\n";
static const char cmgl_response_1[] = "07911326060032F064A9542954\r\n";

/*************************************************************************************************/
/*                                         Test setup                                            */
/*************************************************************************************************/
static void *test_modem_chat_setup(void)
{
	const struct modem_chat_config cmd_config = {
		.user_data = &cmd_user_data,
		.receive_buf = cmd_receive_buf,
		.receive_buf_size = ARRAY_SIZE(cmd_receive_buf),
		.delimiter = cmd_delimiter,
		.delimiter_size = ARRAY_SIZE(cmd_delimiter),
		.filter = NULL,
		.filter_size = 0,
		.argv = cmd_argv,
		.argv_size = ARRAY_SIZE(cmd_argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
	};

	zassert(modem_chat_init(&cmd, &cmd_config) == 0, "Failed to init modem CMD");

	const struct modem_backend_mock_config mock_config = {
		.rx_buf = mock_rx_buf,
		.rx_buf_size = ARRAY_SIZE(mock_rx_buf),
		.tx_buf = mock_tx_buf,
		.tx_buf_size = ARRAY_SIZE(mock_tx_buf),
		.limit = 8,
	};

	mock_pipe = modem_backend_mock_init(&mock, &mock_config);
	zassert(modem_pipe_open(mock_pipe, K_SECONDS(10)) == 0, "Failed to open mock pipe");
	zassert(modem_chat_attach(&cmd, mock_pipe) == 0, "Failed to attach pipe mock to modem CMD");
	return NULL;
}

static void test_modem_chat_before(void *f)
{
	/* Reset callback called */
	atomic_set(&callback_called, 0);

	/* Reset mock pipe */
	modem_backend_mock_reset(&mock);
}

static void test_modem_chat_after(void *f)
{
	/* Abort script */
	modem_chat_script_abort(&cmd);

	k_msleep(100);
}

/*************************************************************************************************/
/*                                          Buffers                                              */
/*************************************************************************************************/
static uint8_t buffer[4096];

/*************************************************************************************************/
/*                                           Tests                                               */
/*************************************************************************************************/
ZTEST(modem_chat, test_script_no_error)
{
	bool called;

	zassert_true(modem_chat_script_run(&cmd, &script) == 0, "Failed to start script");
	k_msleep(100);

	/*
	 * Script sends "AT\r\n"
	 * Modem responds "AT\r\n"
	 * Modem responds "OK\r\n"
	 */

	modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer));
	zassert_true(memcmp(buffer, "AT\r", sizeof("AT\r") - 1) == 0,
		     "Request not sent as expected");

	modem_backend_mock_put(&mock, at_response, sizeof(at_response) - 1);
	modem_backend_mock_put(&mock, ok_response, sizeof(ok_response) - 1);

	k_msleep(100);

	/*
	 * Script sends "ATE0\r\n"
	 * Modem responds "OK\r\n"
	 */

	modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer));
	zassert_true(memcmp(buffer, "ATE0\r\n", sizeof("ATE0\r\n") - 1) == 0,
		     "Request not sent as expected");

	modem_backend_mock_put(&mock, ok_response, sizeof(ok_response) - 1);

	k_msleep(100);

	/*
	 * Script sends "IMEI?\r\n"
	 * Modem responds "23412354123123\r\n"
	 * Modem responds "OK\r\n"
	 */

	modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer));
	zassert_true(memcmp(buffer, "IMEI?\r\n", sizeof("IMEI?\r\n") - 1) == 0,
		     "Request not sent as expected");

	modem_backend_mock_put(&mock, imei_response, sizeof(imei_response) - 1);
	modem_backend_mock_put(&mock, ok_response, sizeof(ok_response) - 1);

	k_msleep(100);

	zassert_true(atomic_test_bit(&callback_called, MODEM_CHAT_UTEST_ON_IMEI_CALLED_BIT) == true,
		     "Expected IMEI callback called");

	zassert_true(argv_buffers[0][0] == '\0', "Unexpected argv");
	zassert_true(memcmp(argv_buffers[1], "23412354123123", sizeof("23412354123123")) == 0,
		     "Unexpected argv");

	zassert_true(argc_buffers == 2, "Unexpected argc");

	/*
	 * Script sends "AT+CREG?;+CGREG?\r\n"
	 * Modem responds "CREG: 1,2\r\n"
	 * Modem responds "CGREG: 1,2\r\n"
	 * Modem responds "OK\r\n"
	 */

	modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer));
	zassert_true(memcmp(buffer, "AT+CREG?;+CGREG?\r\n", sizeof("AT+CREG?;+CGREG?\r\n") - 1) ==
			     0,
		     "Request not sent as expected");

	modem_backend_mock_put(&mock, creg_response, sizeof(creg_response) - 1);

	k_msleep(100);

	zassert_true(memcmp(argv_buffers[0], "CREG: ", sizeof("CREG: ")) == 0, "Unexpected argv");
	zassert_true(memcmp(argv_buffers[1], "1", sizeof("1")) == 0, "Unexpected argv");
	zassert_true(memcmp(argv_buffers[2], "2", sizeof("2")) == 0, "Unexpected argv");
	zassert_true(argc_buffers == 3, "Unexpected argc");
	modem_backend_mock_put(&mock, cgreg_response, sizeof(cgreg_response) - 1);

	k_msleep(100);

	zassert_true(memcmp(argv_buffers[0], "CGREG: ", sizeof("CGREG: ")) == 0, "Unexpected argv");
	zassert_true(memcmp(argv_buffers[1], "10", sizeof("10")) == 0, "Unexpected argv");
	zassert_true(memcmp(argv_buffers[2], "43", sizeof("43")) == 0, "Unexpected argv");
	zassert_true(argc_buffers == 3, "Unexpected argc");
	modem_backend_mock_put(&mock, ok_response, sizeof(ok_response) - 1);

	k_msleep(100);

	/*
	 * Script sends "AT+QENG=\"servingcell\"\r\n"
	 * Modem responds qeng_servinc_cell_response (long string)
	 * Modem responds "OK\r\n"
	 */

	modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer));
	zassert_true(memcmp(buffer, "AT+QENG=\"servingcell\"\r\n",
			    sizeof("AT+QENG=\"servingcell\"\r\n") - 1) == 0,
		     "Request not sent as expected");

	modem_backend_mock_put(&mock, qeng_servinc_cell_response,
			       sizeof(qeng_servinc_cell_response) - 1);

	k_msleep(100);

	zassert_true(memcmp(argv_buffers[0], "+QENG: \"servingcell\",",
			    sizeof("+QENG: \"servingcell\",")) == 0,
		     "Unexpected argv");

	zassert_true(memcmp(argv_buffers[1], "\"NOCONN\"", sizeof("\"NOCONN\"")) == 0,
		     "Unexpected argv");

	zassert_true(memcmp(argv_buffers[10], "-68", sizeof("-68")) == 0, "Unexpected argv");
	zassert_true(argv_buffers[25][0] == '\0', "Unexpected argv");
	zassert_true(argc_buffers == 26, "Unexpected argc");

	/*
	 * Script ends after modem responds OK
	 */

	called = atomic_test_bit(&callback_called, MODEM_CHAT_UTEST_ON_SCRIPT_CALLBACK_BIT);
	zassert_true(called == false, "Script callback should not have been called yet");
	modem_backend_mock_put(&mock, ok_response, sizeof(ok_response) - 1);

	k_msleep(100);

	called = atomic_test_bit(&callback_called, MODEM_CHAT_UTEST_ON_SCRIPT_CALLBACK_BIT);
	zassert_true(called == true, "Script callback should have been called");
	zassert_true(script_result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS,
		     "Script result should be SUCCESS");
	zassert_true(script_result_user_data == &cmd_user_data,
		     "Script result callback user data is incorrect");
}

ZTEST(modem_chat, test_start_script_twice_then_abort)
{
	bool called;

	zassert_true(modem_chat_script_run(&cmd, &script) == 0, "Failed to start script");

	k_msleep(100);

	zassert_true(modem_chat_script_run(&cmd, &script) == -EBUSY,
		     "Started new script while script is running");

	called = atomic_test_bit(&callback_called, MODEM_CHAT_UTEST_ON_SCRIPT_CALLBACK_BIT);
	zassert_true(called == false, "Script callback should not have been called yet");
	modem_chat_script_abort(&cmd);

	k_msleep(100);

	called = atomic_test_bit(&callback_called, MODEM_CHAT_UTEST_ON_SCRIPT_CALLBACK_BIT);
	zassert_true(called == true, "Script callback should have been called");
	zassert_true(script_result == MODEM_CHAT_SCRIPT_RESULT_ABORT,
		     "Script result should be ABORT");
	zassert_true(script_result_user_data == &cmd_user_data,
		     "Script result callback user data is incorrect");
}

ZTEST(modem_chat, test_start_script_then_time_out)
{
	bool called;

	zassert_true(modem_chat_script_run(&cmd, &script) == 0, "Failed to start script");
	k_msleep(100);

	called = atomic_test_bit(&callback_called, MODEM_CHAT_UTEST_ON_SCRIPT_CALLBACK_BIT);
	zassert_true(called == false, "Script callback should not have been called yet");

	k_msleep(5900);

	called = atomic_test_bit(&callback_called, MODEM_CHAT_UTEST_ON_SCRIPT_CALLBACK_BIT);
	zassert_true(called == true, "Script callback should have been called");
	zassert_true(script_result == MODEM_CHAT_SCRIPT_RESULT_TIMEOUT,
		     "Script result should be TIMEOUT");
	zassert_true(script_result_user_data == &cmd_user_data,
		     "Script result callback user data is incorrect");
}

ZTEST(modem_chat, test_script_with_partial_matches)
{
	bool called;

	zassert_true(modem_chat_script_run(&cmd, &script_partial) == 0, "Failed to start script");
	k_msleep(100);

	/*
	 * Script sends "AT+CMGL=4\r";
	 */

	modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer));
	zassert_true(memcmp(buffer, "AT+CMGL=4\r", sizeof("AT+CMGL=4\r") - 1) == 0,
		     "Request not sent as expected");

	/*
	 * Modem will return the following sequence 3 times
	 * "+CMGL: 1,1,,50\r";
	 * "07911326060032F064A9542954\r"
	 */

	for (uint8_t i = 0; i < 3; i++) {
		atomic_set(&callback_called, 0);
		modem_backend_mock_put(&mock, cmgl_response_0, sizeof(cmgl_response_0) - 1);
		k_msleep(100);

		called = atomic_test_bit(&callback_called,
					 MODEM_CHAT_UTEST_ON_CMGL_PARTIAL_CALLED_BIT);
		zassert_equal(called, true, "Match callback not called");
		zassert_equal(argc_buffers, 5, "Incorrect number of args");
		zassert_str_equal(argv_buffers[0], "+CMGL: ",
				  "Incorrect argv received");
		zassert_str_equal(argv_buffers[1], "1",
				  "Incorrect argv received");
		zassert_str_equal(argv_buffers[2], "1",
				  "Incorrect argv received");
		zassert_str_equal(argv_buffers[3], "",
				  "Incorrect argv received");
		zassert_str_equal(argv_buffers[4], "50",
				  "Incorrect argv received");

		atomic_set(&callback_called, 0);
		modem_backend_mock_put(&mock, cmgl_response_1, sizeof(cmgl_response_1) - 1);
		k_msleep(100);

		called = atomic_test_bit(&callback_called,
					 MODEM_CHAT_UTEST_ON_CMGL_PARTIAL_ANY_CALLED_BIT);
		zassert_equal(called, true, "Match callback not called");
		zassert_equal(argc_buffers, 2, "Incorrect number of args");
		zassert_str_equal(argv_buffers[0], "",
				  "Incorrect argv received");
		zassert_str_equal(argv_buffers[1],
				  "07911326060032F064A9542954",
				  "Incorrect argv received");
	}

	atomic_set(&callback_called, 0);
	modem_backend_mock_put(&mock, ok_response, sizeof(ok_response) - 1);
	k_msleep(100);

	/*
	 * Modem returns "OK\r"
	 * Script terminates
	 */

	called = atomic_test_bit(&callback_called, MODEM_CHAT_UTEST_ON_SCRIPT_CALLBACK_BIT);
	zassert_true(called == true, "Script callback should have been called");
	zassert_equal(script_result, MODEM_CHAT_SCRIPT_RESULT_SUCCESS,
		      "Script should have stopped with success");

	/* Assert no data was sent except the request */
	zassert_equal(modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer)), 0,
		      "Script sent too many requests");
}

ZTEST(modem_chat, test_script_run_sync_complete)
{
	modem_backend_mock_prime(&mock, &at_echo_transaction);
	zassert_ok(modem_chat_run_script(&cmd, &script_echo), "Failed to run echo script");
}

ZTEST(modem_chat, test_script_run_sync_timeout)
{
	zassert_equal(modem_chat_run_script(&cmd, &script_echo), -EAGAIN,
		      "Failed to run echo script");
}

ZTEST(modem_chat, test_script_run_sync_abort)
{
	modem_backend_mock_prime(&mock, &at_echo_error_transaction);
	zassert_equal(modem_chat_run_script(&cmd, &script_echo), -EAGAIN,
		      "Echo script should time out and return -EAGAIN");
}

ZTEST(modem_chat, test_script_run_dynamic_script_sync)
{
	char match[] = "AT";
	char separators[] = ",";
	char request[] = "AT";
	char name[] = "Dynamic";

	struct modem_chat_match stack_response_match = {
		.match = NULL,
		.match_size = 0,
		.separators = NULL,
		.separators_size = 0,
		.wildcards = false,
		.partial = false,
		.callback = NULL,
	};

	struct modem_chat_script_chat stack_script_chat = {
		.request = NULL,
		.response_matches = &stack_response_match,
		.response_matches_size = 1,
		.timeout = 0,
	};

	struct modem_chat_script stack_script = {
		.name = name,
		.script_chats = &stack_script_chat,
		.script_chats_size = 1,
		.abort_matches = NULL,
		.abort_matches_size = 0,
		.callback = NULL,
		.timeout = 1,
	};

	stack_response_match.match = match;
	stack_response_match.match_size = strlen(match);
	stack_response_match.separators = separators;
	stack_response_match.separators_size = strlen(match);
	stack_script_chat.request = request;
	stack_script_chat.request_size = strlen(request);

	modem_backend_mock_prime(&mock, &at_echo_transaction);
	zassert_ok(modem_chat_run_script(&cmd, &stack_script), "Failed to run script");
}

ZTEST(modem_chat, test_script_chat_timeout_cmd)
{
	int ret;
	bool called;

	zassert_ok(modem_chat_run_script_async(&cmd, &script_timeout_cmd),
		   "Failed to start script");
	k_msleep(100);

	/*
	 * Script sends "AT\r\n";
	 */
	ret = modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer));
	zassert_equal(ret, sizeof("AT\r\n") - 1);
	zassert_true(memcmp(buffer, "AT\r\n", sizeof("AT\r\n") - 1) == 0,
		     "Request not sent as expected");

	/*
	 * Modem responds OK
	 */
	modem_backend_mock_put(&mock, ok_response, sizeof(ok_response) - 1);

	/*
	 * Script waits 4 seconds
	 */
	k_msleep(3000);
	zassert_equal(modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer)), 0);
	k_msleep(2000);

	/*
	 * Script sends "AT\r\n";
	 */
	ret = modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer));
	zassert_equal(ret, sizeof("AT\r\n") - 1);
	zassert_true(memcmp(buffer, "AT\r\n", sizeof("AT\r\n") - 1) == 0,
		     "Request not sent as expected");

	/*
	 * Modem responds OK
	 */
	modem_backend_mock_put(&mock, ok_response, sizeof(ok_response) - 1);
	k_msleep(100);

	called = atomic_test_bit(&callback_called, MODEM_CHAT_UTEST_ON_SCRIPT_CALLBACK_BIT);
	zassert_true(called == true, "Script callback should have been called");
	zassert_equal(script_result, MODEM_CHAT_SCRIPT_RESULT_SUCCESS,
		      "Script should have stopped with success");

	/* Assert no data was sent except the request */
	zassert_equal(modem_backend_mock_get(&mock, buffer, ARRAY_SIZE(buffer)), 0,
		      "Script sent too many requests");
}

ZTEST(modem_chat, test_runtime_match)
{
	int ret;
	struct modem_chat_match test_match;

	modem_chat_match_init(&test_match);

	ret = modem_chat_match_set_match(&test_match, "AT345");
	zassert_ok(ret, "Failed to set match");
	zassert_ok(strcmp(test_match.match, "AT345"), "Failed to set match");
	zassert_equal(test_match.match_size, 5, "Failed to set size of match");

	ret = modem_chat_match_set_separators(&test_match, ",*");
	zassert_ok(ret, "Failed to set match");
	zassert_ok(strcmp(test_match.separators, ",*"), "Failed to set separators");
	zassert_equal(test_match.separators_size, 2, "Failed to set size of separators");

	modem_chat_match_set_partial(&test_match, true);
	zassert_equal(test_match.partial, true);
	modem_chat_match_set_partial(&test_match, false);
	zassert_equal(test_match.partial, false);

	modem_chat_match_enable_wildcards(&test_match, true);
	zassert_equal(test_match.wildcards, true);
	modem_chat_match_enable_wildcards(&test_match, false);
	zassert_equal(test_match.wildcards, false);
}

ZTEST(modem_chat, test_runtime_script_chat)
{
	int ret;
	struct modem_chat_script_chat test_script_chat;
	struct modem_chat_match test_response_matches[2];

	modem_chat_script_chat_init(&test_script_chat);

	ret = modem_chat_script_chat_set_request(&test_script_chat, "AT345");
	zassert_ok(ret, "Failed to set request");
	zassert_ok(strcmp(test_script_chat.request, "AT345"), "Failed to set script_chat");
	zassert_equal(test_script_chat.request_size, 5, "Failed to set size of script_chat");

	ret = modem_chat_script_chat_set_response_matches(&test_script_chat,
							  test_response_matches,
							  ARRAY_SIZE(test_response_matches));
	zassert_ok(ret, "Failed to set response matches");
	zassert_equal(test_script_chat.response_matches, test_response_matches,
		      "Failed to set response_matches");
	zassert_equal(test_script_chat.response_matches_size, ARRAY_SIZE(test_response_matches),
		      "Failed to set response_matches");

	ret = modem_chat_script_chat_set_response_matches(&test_script_chat,
							  test_response_matches, 0);
	zassert_equal(ret, -EINVAL, "Should have failed to set response matches");

	ret = modem_chat_script_chat_set_response_matches(&test_script_chat, NULL, 1);
	zassert_equal(ret, -EINVAL, "Should have failed to set response matches");
}

ZTEST(modem_chat, test_runtime_script)
{
	int ret;
	struct modem_chat_script test_script;
	struct modem_chat_script_chat test_script_chats[2];
	struct modem_chat_match test_abort_matches[2];

	modem_chat_script_init(&test_script);
	zassert_equal(strlen(test_script.name), 0, "Failed to set default name");

	ret = modem_chat_script_set_script_chats(&test_script, test_script_chats,
						 ARRAY_SIZE(test_script_chats));
	zassert_ok(ret, "Failed to set script chats");
	zassert_equal(test_script.script_chats, test_script_chats,
		      "Failed to set script_chats");
	zassert_equal(test_script.script_chats_size, ARRAY_SIZE(test_script_chats),
		      "Failed to set script_chats_size");

	ret = modem_chat_script_set_script_chats(&test_script, test_script_chats, 0);
	zassert_equal(ret, -EINVAL, "Should have failed to set script chats");

	ret = modem_chat_script_set_script_chats(&test_script, NULL, 1);
	zassert_equal(ret, -EINVAL, "Should have failed to set script chats");

	ret = modem_chat_script_set_abort_matches(&test_script, test_abort_matches,
						  ARRAY_SIZE(test_abort_matches));
	zassert_ok(ret, "Failed to set abort matches");
	zassert_equal(test_script.abort_matches, test_abort_matches,
		      "Failed to set script_chats");
	zassert_equal(test_script.abort_matches_size, ARRAY_SIZE(test_abort_matches),
		      "Failed to set script_chats_size");

	ret = modem_chat_script_set_abort_matches(&test_script, test_abort_matches, 0);
	zassert_equal(ret, -EINVAL, "Should have failed to set abort matches");

	ret = modem_chat_script_set_abort_matches(&test_script, NULL, 1);
	zassert_equal(ret, -EINVAL, "Should have failed to set abort matches");
}

/*************************************************************************************************/
/*                                         Test suite                                            */
/*************************************************************************************************/
ZTEST_SUITE(modem_chat, NULL, test_modem_chat_setup, test_modem_chat_before, test_modem_chat_after,
	    NULL);
