/** @file at.h
 *  @brief Internal APIs for AT command handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

enum at_result {
	AT_RESULT_OK,
	AT_RESULT_ERROR
};

enum at_state {
	AT_STATE_START,
	AT_STATE_START_CR,
	AT_STATE_START_LF,
	AT_STATE_GET_CMD_STRING,
	AT_STATE_PROCESS_CMD,
	AT_STATE_GET_RESULT_STRING,
	AT_STATE_PROCESS_RESULT,
	AT_STATE_UNSOLICITED_CMD,
	AT_STATE_END
};

enum at_cmd_state {
	CMD_START,
	CMD_GET_VALUE,
	CMD_PROCESS_VALUE,
	CMD_STATE_END_LF,
	CMD_STATE_END
};

struct at_client;

/* Callback at_resp_cb_t used to parse response value received for the
 * particular AT command. Eg: +CIND=<value>
 */
typedef int (*at_resp_cb_t)(struct at_client *at, struct net_buf *buf);

/* Callback at_finish_cb used to monitor the success or failure of the AT
 * command received from server
 */
typedef int (*at_finish_cb_t)(struct at_client *at, struct net_buf *buf,
			       enum at_result result);
typedef int (*parse_val_t)(struct at_client *at);
typedef int (*handle_parse_input_t)(struct at_client *at, struct net_buf *buf);
typedef int (*handle_cmd_input_t)(struct at_client *at, struct net_buf *buf,
				  const char *prefix, parse_val_t func);

struct at_client {
	char *buf;
	uint8_t buf_pos;
	uint8_t buf_max_len;
	uint8_t state;
	uint8_t cmd_state;
	at_resp_cb_t resp;
	at_finish_cb_t finish;
};

/* Register the callback functions */
void at_register(struct at_client *at, at_resp_cb_t resp,
		 at_finish_cb_t finish);
int at_get_number(const char *buf, uint32_t *val);
/* This parsing will only works for non-fragmented net_buf */
int at_parse_input(struct at_client *at, struct net_buf *buf);
/* This command parsing will only works for non-fragmented net_buf */
int at_parse_cmd_input(struct at_client *at, struct net_buf *buf,
		       const char *prefix, parse_val_t func);
int at_check_byte(struct net_buf *buf, char check_byte);
