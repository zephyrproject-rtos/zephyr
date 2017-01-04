/** @file at.h
 *  @brief Internal APIs for AT command handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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
	AT_CMD_START,
	AT_CMD_GET_VALUE,
	AT_CMD_PROCESS_VALUE,
	AT_CMD_STATE_END_LF,
	AT_CMD_STATE_END
};

enum at_cmd_type {
	AT_CMD_TYPE_NORMAL,
	AT_CMD_TYPE_UNSOLICITED
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
				  const char *prefix, parse_val_t func,
				  enum at_cmd_type type);

struct at_client {
	char *buf;
	uint8_t pos;
	uint8_t buf_max_len;
	uint8_t state;
	uint8_t cmd_state;
	at_resp_cb_t resp;
	at_resp_cb_t unsolicited;
	at_finish_cb_t finish;
};

/* Register the callback functions */
void at_register(struct at_client *at, at_resp_cb_t resp,
		 at_finish_cb_t finish);
void at_register_unsolicited(struct at_client *at, at_resp_cb_t unsolicited);
int at_get_number(struct at_client *at, uint32_t *val);
/* This parsing will only works for non-fragmented net_buf */
int at_parse_input(struct at_client *at, struct net_buf *buf);
/* This command parsing will only works for non-fragmented net_buf */
int at_parse_cmd_input(struct at_client *at, struct net_buf *buf,
		       const char *prefix, parse_val_t func,
		       enum at_cmd_type type);
int at_check_byte(struct net_buf *buf, char check_byte);
int at_list_get_range(struct at_client *at, uint32_t *min, uint32_t *max);
int at_list_get_string(struct at_client *at, char *name, uint8_t len);
int at_close_list(struct at_client *at);
int at_open_list(struct at_client *at);
int at_has_next_list(struct at_client *at);
