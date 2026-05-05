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

	zassert_false(modem_chat_is_running(&cmd));
	zassert_ok(modem_chat_run_script_async(&cmd, &script_timeout_cmd),
		   "Failed to start script");
	zassert_true(modem_chat_is_running(&cmd));
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
	zassert_false(modem_chat_is_running(&cmd));

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
/*                                  Line tap (raw line forwarding)                               */
/*************************************************************************************************/

/* Two extra chat instances dedicated to the line_tap tests. Both share the
 * same delimiter / argv buffers as the main `cmd`, but are wired with
 * independent receive buffers, mock pipes, and user_data so they can run
 * without disturbing the script-driven tests above.
 */

#define TAP_BUF_SIZE       64
#define TAP_CAPTURE_SLOTS  4
#define TAP_CAPTURE_MAX    128

struct tap_capture {
	size_t len;
	uint8_t bytes[TAP_CAPTURE_MAX];
};

static struct tap_state {
	uint32_t invoke_count;
	void *last_user_data;
	struct tap_capture slots[TAP_CAPTURE_SLOTS];
} tap_state;

static uint32_t tap_user_data = 0xCAFEBABE;

static void on_tap(struct modem_chat *chat, const char *line, size_t line_len, void *user_data)
{
	uint32_t slot;

	ARG_UNUSED(chat);

	tap_state.last_user_data = user_data;
	slot = tap_state.invoke_count % TAP_CAPTURE_SLOTS;
	tap_state.invoke_count++;
	tap_state.slots[slot].len = MIN(line_len, sizeof(tap_state.slots[slot].bytes));
	memcpy(tap_state.slots[slot].bytes, line, tap_state.slots[slot].len);
}

static void tap_state_reset(void)
{
	memset(&tap_state, 0, sizeof(tap_state));
}

/* Plain tap chat — no typed matchers, full tap_buf for byte-faithful capture. */
static struct modem_chat tap_cmd;
static uint8_t tap_cmd_receive_buf[128];
static uint8_t *tap_cmd_argv[8];
static uint8_t tap_cmd_buf[TAP_BUF_SIZE];

static struct modem_backend_mock tap_mock;
static uint8_t tap_mock_rx_buf[128];
static uint8_t tap_mock_tx_buf[128];
static struct modem_pipe *tap_mock_pipe;

/* Typed-matcher tap chat — has an unsol matcher for "MARK," so we can verify
 * tap_buf preserves separators while receive_buf gets mutated by argv parsing.
 */
static atomic_t tap_typed_match_called;

#define TAP_TYPED_MATCH_BIT 0

static void on_tap_typed_match(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	ARG_UNUSED(chat);
	ARG_UNUSED(argv);
	ARG_UNUSED(argc);
	ARG_UNUSED(user_data);
	atomic_set_bit(&tap_typed_match_called, TAP_TYPED_MATCH_BIT);
}

MODEM_CHAT_MATCH_DEFINE(tap_typed_match, "MARK,", ",*", on_tap_typed_match);
MODEM_CHAT_MATCHES_DEFINE(tap_typed_unsol, tap_typed_match);

static struct modem_chat tap_typed_cmd;
static uint8_t tap_typed_receive_buf[128];
static uint8_t *tap_typed_argv[8];
static uint8_t tap_typed_buf[TAP_BUF_SIZE];

static struct modem_backend_mock tap_typed_mock;
static uint8_t tap_typed_mock_rx_buf[128];
static uint8_t tap_typed_mock_tx_buf[128];
static struct modem_pipe *tap_typed_mock_pipe;

static void test_modem_chat_tap_setup(void)
{
	const struct modem_chat_config tap_config = {
		.user_data = &tap_user_data,
		.receive_buf = tap_cmd_receive_buf,
		.receive_buf_size = ARRAY_SIZE(tap_cmd_receive_buf),
		.delimiter = cmd_delimiter,
		.delimiter_size = ARRAY_SIZE(cmd_delimiter),
		.argv = tap_cmd_argv,
		.argv_size = ARRAY_SIZE(tap_cmd_argv),
		.unsol_matches = NULL,
		.unsol_matches_size = 0,
		.line_tap = on_tap,
		.tap_buf = tap_cmd_buf,
		.tap_buf_size = ARRAY_SIZE(tap_cmd_buf),
	};

	const struct modem_backend_mock_config tap_mock_cfg = {
		.rx_buf = tap_mock_rx_buf,
		.rx_buf_size = ARRAY_SIZE(tap_mock_rx_buf),
		.tx_buf = tap_mock_tx_buf,
		.tx_buf_size = ARRAY_SIZE(tap_mock_tx_buf),
		.limit = 8,
	};

	zassert_ok(modem_chat_init(&tap_cmd, &tap_config), "tap_cmd init");
	tap_mock_pipe = modem_backend_mock_init(&tap_mock, &tap_mock_cfg);
	zassert_ok(modem_pipe_open(tap_mock_pipe, K_SECONDS(10)), "tap mock pipe open");
	zassert_ok(modem_chat_attach(&tap_cmd, tap_mock_pipe), "tap_cmd attach");

	const struct modem_chat_config tap_typed_config = {
		.user_data = &tap_user_data,
		.receive_buf = tap_typed_receive_buf,
		.receive_buf_size = ARRAY_SIZE(tap_typed_receive_buf),
		.delimiter = cmd_delimiter,
		.delimiter_size = ARRAY_SIZE(cmd_delimiter),
		.argv = tap_typed_argv,
		.argv_size = ARRAY_SIZE(tap_typed_argv),
		.unsol_matches = tap_typed_unsol,
		.unsol_matches_size = ARRAY_SIZE(tap_typed_unsol),
		.line_tap = on_tap,
		.tap_buf = tap_typed_buf,
		.tap_buf_size = ARRAY_SIZE(tap_typed_buf),
	};

	const struct modem_backend_mock_config tap_typed_mock_cfg = {
		.rx_buf = tap_typed_mock_rx_buf,
		.rx_buf_size = ARRAY_SIZE(tap_typed_mock_rx_buf),
		.tx_buf = tap_typed_mock_tx_buf,
		.tx_buf_size = ARRAY_SIZE(tap_typed_mock_tx_buf),
		.limit = 8,
	};

	zassert_ok(modem_chat_init(&tap_typed_cmd, &tap_typed_config), "tap_typed_cmd init");
	tap_typed_mock_pipe = modem_backend_mock_init(&tap_typed_mock, &tap_typed_mock_cfg);
	zassert_ok(modem_pipe_open(tap_typed_mock_pipe, K_SECONDS(10)),
		   "tap_typed mock pipe open");
	zassert_ok(modem_chat_attach(&tap_typed_cmd, tap_typed_mock_pipe), "tap_typed_cmd attach");
}

ZTEST(modem_chat, test_line_tap_fires_per_line)
{
	const char line1[] = "HELLO\r\n";
	const char line2[] = "WORLD\r\n";

	tap_state_reset();

	modem_backend_mock_put(&tap_mock, line1, sizeof(line1) - 1);
	modem_backend_mock_put(&tap_mock, line2, sizeof(line2) - 1);
	k_msleep(100);

	zassert_equal(tap_state.invoke_count, 2U, "tap should fire once per line");
	zassert_equal(tap_state.last_user_data, &tap_user_data,
		      "tap should receive chat user_data");

	zassert_equal(tap_state.slots[0].len, 5U, "first line length excludes delimiter");
	zassert_mem_equal(tap_state.slots[0].bytes, "HELLO", 5, "first line content");
	zassert_equal(tap_state.slots[1].len, 5U, "second line length excludes delimiter");
	zassert_mem_equal(tap_state.slots[1].bytes, "WORLD", 5, "second line content");
}

ZTEST(modem_chat, test_line_tap_skips_empty_lines)
{
	const char only_delim[] = "\r\n";
	const char real_line[] = "HI\r\n";

	tap_state_reset();

	modem_backend_mock_put(&tap_mock, only_delim, sizeof(only_delim) - 1);
	modem_backend_mock_put(&tap_mock, real_line, sizeof(real_line) - 1);
	k_msleep(100);

	zassert_equal(tap_state.invoke_count, 1U,
		      "tap must NOT fire for delimiter-only lines");
	zassert_equal(tap_state.slots[0].len, 2U, "real line length");
	zassert_mem_equal(tap_state.slots[0].bytes, "HI", 2, "real line content");
}

ZTEST(modem_chat, test_line_tap_truncates_oversize_line)
{
	/* tap_buf is TAP_BUF_SIZE (64). Send a line longer than that and
	 * verify the tap reports len == tap_buf_size and the captured bytes
	 * are exactly the leading TAP_BUF_SIZE bytes of the input. */
	uint8_t line[TAP_BUF_SIZE + 32 + 2];

	for (size_t i = 0; i < ARRAY_SIZE(line) - 2; i++) {
		line[i] = (uint8_t)('A' + (i % 26));
	}
	line[ARRAY_SIZE(line) - 2] = '\r';
	line[ARRAY_SIZE(line) - 1] = '\n';

	tap_state_reset();

	modem_backend_mock_put(&tap_mock, line, ARRAY_SIZE(line));
	k_msleep(100);

	zassert_equal(tap_state.invoke_count, 1U, "tap fires once");
	zassert_equal(tap_state.slots[0].len, (size_t)TAP_BUF_SIZE,
		      "truncated len equals tap_buf_size");
	zassert_mem_equal(tap_state.slots[0].bytes, line, TAP_BUF_SIZE,
			  "truncated content matches the leading bytes");
}

ZTEST(modem_chat, test_line_tap_resets_between_lines)
{
	/* A line that fills tap_buf (forces truncation) followed by a short
	 * line. The short line must not bleed bytes from the previous line
	 * — verifies parse_reset zeroes tap_buf_len.
	 */
	uint8_t big[TAP_BUF_SIZE + 32 + 2];
	const char small[] = "OK\r\n";

	for (size_t i = 0; i < ARRAY_SIZE(big) - 2; i++) {
		big[i] = (uint8_t)('A' + (i % 26));
	}
	big[ARRAY_SIZE(big) - 2] = '\r';
	big[ARRAY_SIZE(big) - 1] = '\n';

	tap_state_reset();

	modem_backend_mock_put(&tap_mock, big, ARRAY_SIZE(big));
	modem_backend_mock_put(&tap_mock, small, sizeof(small) - 1);
	k_msleep(100);

	zassert_equal(tap_state.invoke_count, 2U, "two taps fire");
	zassert_equal(tap_state.slots[1].len, 2U,
		      "second tap reports a clean 2-byte length");
	zassert_mem_equal(tap_state.slots[1].bytes, "OK", 2,
			  "second tap content does not bleed from the first");
}

ZTEST(modem_chat, test_line_tap_byte_faithful_when_matcher_engages)
{
	/* A typed matcher engages on "MARK," and modem_chat replaces commas
	 * in receive_buf with '\0' for argv parsing. The tap, sourced from
	 * tap_buf, must still see the original commas.
	 */
	const char line[] = "MARK,1,2,3\r\n";

	tap_state_reset();
	atomic_set(&tap_typed_match_called, 0);

	modem_backend_mock_put(&tap_typed_mock, line, sizeof(line) - 1);
	k_msleep(100);

	zassert_true(atomic_test_bit(&tap_typed_match_called, TAP_TYPED_MATCH_BIT),
		     "typed matcher must dispatch alongside the tap");
	zassert_equal(tap_state.invoke_count, 1U, "tap fires once");
	zassert_equal(tap_state.slots[0].len, (sizeof(line) - 1) - 2,
		      "len excludes the trailing CRLF");
	/* If tap_buf were not in use, this assertion would catch the byte
	 * mutation: receive_buf would have '\0' in place of the commas. */
	zassert_mem_equal(tap_state.slots[0].bytes, "MARK,1,2,3",
			  (sizeof(line) - 1) - 2,
			  "tap_buf preserves commas the matcher path replaced");
}

/*************************************************************************************************/
/*                                         Test suite                                            */
/*************************************************************************************************/

static void *test_modem_chat_full_setup(void)
{
	void *ret = test_modem_chat_setup();

	test_modem_chat_tap_setup();
	return ret;
}

ZTEST_SUITE(modem_chat, NULL, test_modem_chat_full_setup, test_modem_chat_before,
	    test_modem_chat_after, NULL);
