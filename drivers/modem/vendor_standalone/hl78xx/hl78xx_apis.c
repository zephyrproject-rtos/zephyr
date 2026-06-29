/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hl78xx.h"
#include "hl78xx_chat.h"
#ifdef CONFIG_HL78XX_GNSS
#include "hl78xx_gnss.h"
#endif /* CONFIG_HL78XX_GNSS */
#include "hl78xx_cfg.h"
#include <zephyr/drivers/modem/hl78xx_apis.h>

LOG_MODULE_REGISTER(hl78xx_apis, CONFIG_MODEM_LOG_LEVEL);

static void hl78xx_at_cmd_capture_reset(struct hl78xx_at_cmd_capture_ctx *capture)
{
	if (capture == NULL) {
		return;
	}

	capture->buf = NULL;
	capture->len = 0U;
	capture->used = 0U;
	capture->captured = false;
	capture->truncated = false;
	capture->terminal_result = HL78XX_AT_CMD_TERMINAL_RESULT_NONE;
	capture->error_type = HL78XX_AT_CMD_ERROR_TYPE_NONE;
	capture->error_code = 0;
}

static void hl78xx_at_cmd_capture_prepare(struct hl78xx_at_cmd_capture_ctx *capture, char *response,
					  size_t response_size)
{
	hl78xx_at_cmd_capture_reset(capture);

	if ((capture == NULL) || (response == NULL) || (response_size == 0U)) {
		return;
	}

	capture->buf = response;
	capture->len = response_size;
	capture->used = 0U;
	response[0] = '\0';
}

static void hl78xx_at_cmd_capture_release(struct hl78xx_at_cmd_capture_ctx *capture)
{
	if (capture == NULL) {
		return;
	}

	capture->buf = NULL;
	capture->len = 0U;
	capture->used = 0U;
}
struct at_cmd_ctx {
	const char *cmd;
	uint16_t cmd_len;
	const struct modem_chat_match *matches;
	uint16_t match_count;
	char *response;
	size_t response_size;
};
/* Shared executor for internal helpers and public AT helpers.
 * Terminal handling semantics depend on the response matches supplied by the caller.
 */
static int hl78xx_at_cmd_execute(struct hl78xx_data *data, modem_chat_script_callback chat_cb,
				 struct at_cmd_ctx *ctx)
{
	if ((data == NULL) || (ctx == NULL) || (ctx->cmd == NULL)) {
		return -EINVAL;
	}

	if ((ctx->response == NULL) != (ctx->response_size == 0U)) {
		return -EINVAL;
	}

	hl78xx_at_cmd_capture_prepare(&data->at_cmd_capture, ctx->response, ctx->response_size);

	int ret = modem_dynamic_cmd_send_req(data, &(const struct hl78xx_dynamic_cmd_request){
							   .script_user_callback = chat_cb,
							   .cmd = (const uint8_t *)ctx->cmd,
							   .cmd_len = ctx->cmd_len,
							   .response_matches = ctx->matches,
							   .matches_size = ctx->match_count,
							   .response_timeout = MDM_CMD_TIMEOUT,
							   .user_cmd = true,
						   });

	hl78xx_at_cmd_capture_release(&data->at_cmd_capture);

	return ret;
}

/* Wrapper to centralize modem_dynamic_cmd_send calls and reduce repetition.
 * returns negative errno on failure or the value returned by modem_dynamic_cmd_send.
 */
static int hl78xx_send_cmd(struct hl78xx_data *data, const char *cmd,
			   modem_chat_script_callback chat_cb,
			   const struct modem_chat_match *matches, uint16_t match_count)
{
	if (data == NULL || cmd == NULL) {
		return -EINVAL;
	}

	struct at_cmd_ctx ctx = {
		.cmd = cmd,
		.cmd_len = (uint16_t)strlen(cmd),
		.matches = matches,
		.match_count = match_count,
		.response = NULL,
		.response_size = 0U,
	};

	return hl78xx_at_cmd_execute(data, chat_cb, &ctx);
}

static bool hl78xx_soft_restart_state_is_ready(enum hl78xx_state state)
{
	switch (state) {
	case MODEM_HL78XX_STATE_IDLE:
	case MODEM_HL78XX_STATE_RESET_PULSE:
	case MODEM_HL78XX_STATE_SOFT_RESET:
	case MODEM_HL78XX_STATE_POWER_ON_PULSE:
	case MODEM_HL78XX_STATE_AWAIT_POWER_ON:
	case MODEM_HL78XX_STATE_SLEEP:
	case MODEM_HL78XX_STATE_INIT_POWER_OFF:
	case MODEM_HL78XX_STATE_POWER_OFF_PULSE:
	case MODEM_HL78XX_STATE_AWAIT_POWER_OFF:
		return false;
	default:
		return true;
	}
}

static void hl78xx_at_cmd_set_terminal_result(struct hl78xx_at_cmd_capture_ctx *capture,
					      enum hl78xx_at_cmd_terminal_result terminal_result,
					      enum hl78xx_at_cmd_error_type error_type,
					      int error_code)
{
	if (capture == NULL) {
		return;
	}

	capture->terminal_result = terminal_result;
	capture->error_type = error_type;
	capture->error_code = error_code;
}

static int hl78xx_at_cmd_parse_error_code(char **argv, uint16_t argc)
{
	if ((argc > 1U) && (argv[1] != NULL) && (argv[1][0] != '\0')) {
		return ATOI(argv[1], 0, "at_error");
	}

	return 0;
}

static void hl78xx_at_cmd_capture_append(struct hl78xx_at_cmd_capture_ctx *capture, char **argv,
					 uint16_t argc)
{
	int ret;
	size_t remaining;

	if ((capture == NULL) || (capture->buf == NULL) || (capture->len == 0U) ||
	    capture->truncated || (argc == 0U)) {
		return;
	}

	if (capture->used >= capture->len) {
		capture->truncated = true;
		return;
	}

	remaining = capture->len - capture->used;

	if ((argc > 1U) && (argv[0] != NULL) && (argv[1] != NULL)) {
		ret = snprintf(capture->buf + capture->used, remaining, "%s%s\r\n", argv[0],
			       argv[1]);
	} else if ((argv[0] != NULL) && (argv[0][0] != '\0')) {
		ret = snprintf(capture->buf + capture->used, remaining, "%s\r\n", argv[0]);
	} else if ((argc > 1U) && (argv[1] != NULL)) {
		ret = snprintf(capture->buf + capture->used, remaining, "%s\r\n", argv[1]);
	} else {
		return;
	}

	if (ret < 0) {
		return;
	}

	capture->captured = true;

	if ((size_t)ret >= remaining) {
		capture->truncated = true;
		capture->used = capture->len - 1U;
		capture->buf[capture->used] = '\0';
		return;
	}

	capture->used += (size_t)ret;
}

static void hl78xx_at_cmd_capture_cb(struct modem_chat *chat, char **argv, uint16_t argc,
				     void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = user_data;

	if (data == NULL) {
		return;
	}

	hl78xx_at_cmd_capture_append(&data->at_cmd_capture, argv, argc);
}

static void hl78xx_at_cmd_on_ok(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = user_data;

	if (data == NULL) {
		return;
	}

	hl78xx_at_cmd_capture_append(&data->at_cmd_capture, argv, argc);
	hl78xx_at_cmd_set_terminal_result(&data->at_cmd_capture, HL78XX_AT_CMD_TERMINAL_RESULT_OK,
					  HL78XX_AT_CMD_ERROR_TYPE_NONE, 0);
}

static void hl78xx_at_cmd_on_error(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = user_data;

	if (data == NULL) {
		return;
	}

	hl78xx_at_cmd_capture_append(&data->at_cmd_capture, argv, argc);
	hl78xx_at_cmd_set_terminal_result(&data->at_cmd_capture,
					  HL78XX_AT_CMD_TERMINAL_RESULT_ERROR,
					  HL78XX_AT_CMD_ERROR_TYPE_GENERIC, 0);
}

static void hl78xx_at_cmd_on_cme_error(struct modem_chat *chat, char **argv, uint16_t argc,
				       void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = user_data;

	if (data == NULL) {
		return;
	}

	hl78xx_at_cmd_capture_append(&data->at_cmd_capture, argv, argc);
	hl78xx_at_cmd_set_terminal_result(
		&data->at_cmd_capture, HL78XX_AT_CMD_TERMINAL_RESULT_ERROR,
		HL78XX_AT_CMD_ERROR_TYPE_CME, hl78xx_at_cmd_parse_error_code(argv, argc));
}

static void hl78xx_at_cmd_on_cms_error(struct modem_chat *chat, char **argv, uint16_t argc,
				       void *user_data)
{
	ARG_UNUSED(chat);
	struct hl78xx_data *data = user_data;

	if (data == NULL) {
		return;
	}

	hl78xx_at_cmd_capture_append(&data->at_cmd_capture, argv, argc);
	hl78xx_at_cmd_set_terminal_result(
		&data->at_cmd_capture, HL78XX_AT_CMD_TERMINAL_RESULT_ERROR,
		HL78XX_AT_CMD_ERROR_TYPE_CMS, hl78xx_at_cmd_parse_error_code(argv, argc));
}

static int hl78xx_modem_at_encode_error(const struct hl78xx_at_cmd_capture_ctx *capture)
{
	int error_type;

	if (capture == NULL) {
		return -EINVAL;
	}

	switch (capture->error_type) {
	case HL78XX_AT_CMD_ERROR_TYPE_GENERIC:
		error_type = HL78XX_MODEM_AT_ERROR;
		break;
	case HL78XX_AT_CMD_ERROR_TYPE_CME:
		error_type = HL78XX_MODEM_AT_CME_ERROR;
		break;
	case HL78XX_AT_CMD_ERROR_TYPE_CMS:
		error_type = HL78XX_MODEM_AT_CMS_ERROR;
		break;
	default:
		return -EIO;
	}

	return (error_type << 16) | (capture->error_code & 0x0000ffff);
}

static int hl78xx_modem_at_exec(struct hl78xx_data *data, char *response, size_t response_size,
				const char *cmd)
{
	struct modem_chat_match matches[5];
	uint16_t match_count = 0U;
	int ret;

	if ((data == NULL) || (cmd == NULL)) {
		return -EINVAL;
	}

	matches[match_count++] = (struct modem_chat_match){
		.match = (const uint8_t *)"",
		.match_size = 0,
		.separators = (const uint8_t *)"",
		.separators_size = 0,
		.wildcards = false,
		.partial = true,
		.callback = hl78xx_at_cmd_capture_cb,
	};
	matches[match_count++] = (struct modem_chat_match){
		.match = (const uint8_t *)MDM_HL78XX_OK_STRING,
		.match_size = (uint8_t)(sizeof(MDM_HL78XX_OK_STRING) - 1U),
		.separators = (const uint8_t *)"",
		.separators_size = 0,
		.wildcards = false,
		.partial = false,
		.callback = hl78xx_at_cmd_on_ok,
	};
	matches[match_count++] = (struct modem_chat_match){
		.match = (const uint8_t *)MDM_HL78XX_CME_ERROR_STRING,
		.match_size = (uint8_t)(sizeof(MDM_HL78XX_CME_ERROR_STRING) - 1U),
		.separators = (const uint8_t *)"",
		.separators_size = 0,
		.wildcards = false,
		.partial = false,
		.callback = hl78xx_at_cmd_on_cme_error,
	};
	matches[match_count++] = (struct modem_chat_match){
		.match = (const uint8_t *)MDM_HL78XX_ERROR_STRING,
		.match_size = (uint8_t)(sizeof(MDM_HL78XX_ERROR_STRING) - 1U),
		.separators = (const uint8_t *)"",
		.separators_size = 0,
		.wildcards = false,
		.partial = false,
		.callback = hl78xx_at_cmd_on_error,
	};
	matches[match_count++] = (struct modem_chat_match){
		.match = (const uint8_t *)MDM_HL78XX_CMS_ERROR_STRING,
		.match_size = (uint8_t)(sizeof(MDM_HL78XX_CMS_ERROR_STRING) - 1U),
		.separators = (const uint8_t *)"",
		.separators_size = 0,
		.wildcards = false,
		.partial = false,
		.callback = hl78xx_at_cmd_on_cms_error,
	};

	struct at_cmd_ctx ctx = {
		.cmd = cmd,
		.cmd_len = (uint16_t)strlen(cmd),
		.matches = matches,
		.match_count = match_count,
		.response = response,
		.response_size = response_size,
	};

	ret = hl78xx_at_cmd_execute(data, NULL, &ctx);

	if (data->at_cmd_capture.truncated) {
		return -E2BIG;
	}

	if ((ret == -EIO) &&
	    (data->at_cmd_capture.terminal_result == HL78XX_AT_CMD_TERMINAL_RESULT_ERROR)) {
		return hl78xx_modem_at_encode_error(&data->at_cmd_capture);
	}

	return ret;
}

int hl78xx_modem_at_send(const struct device *dev, const char *cmd)
{
	struct hl78xx_data *data;

	if ((dev == NULL) || (dev->data == NULL) || (cmd == NULL)) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;

	return hl78xx_modem_at_exec(data, NULL, 0U, cmd);
}

int hl78xx_modem_at_cmd(const struct device *dev, char *response, size_t response_size,
			const char *cmd)
{
	struct hl78xx_data *data;

	if ((dev == NULL) || (dev->data == NULL) || (response == NULL) || (response_size == 0U) ||
	    (cmd == NULL)) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;

	return hl78xx_modem_at_exec(data, response, response_size, cmd);
}

int hl78xx_api_func_get_signal(const struct device *dev, const enum cellular_signal_type type,
			       int16_t *value)
{
	int ret = -ENOTSUP;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	const char *signal_cmd_csq = "AT+CSQ";
	const char *signal_cmd_cesq = "AT+CESQ";

	/* quick check of state under api_lock */
	k_mutex_lock(&data->api_lock, K_FOREVER);
	if (data->status.state != MODEM_HL78XX_STATE_CARRIER_ON) {
		k_mutex_unlock(&data->api_lock);
		return -ENODATA;
	}
	k_mutex_unlock(&data->api_lock);

	/* Run chat script */
	switch (type) {
	case CELLULAR_SIGNAL_RSSI:
		ret = hl78xx_send_cmd(data, signal_cmd_csq, NULL, hl78xx_get_allow_match(),
				      hl78xx_get_allow_match_size());
		break;

	case CELLULAR_SIGNAL_RSRP:
	case CELLULAR_SIGNAL_RSRQ:
		ret = hl78xx_send_cmd(data, signal_cmd_cesq, NULL, hl78xx_get_allow_match(),
				      hl78xx_get_allow_match_size());
		break;

	default:
		ret = -ENOTSUP;
		break;
	}
	/* Verify chat script ran successfully */
	if (ret < 0) {
		return ret;
	}
	/* Parse received value */
	switch (type) {
	case CELLULAR_SIGNAL_RSSI:
		ret = hl78xx_parse_rssi(data->status.signal.rssi, value);
		break;

	case CELLULAR_SIGNAL_RSRP:
		ret = hl78xx_parse_rsrp(data->status.signal.rsrp, value);
		break;

	case CELLULAR_SIGNAL_RSRQ:
		ret = hl78xx_parse_rsrq(data->status.signal.rsrq, value);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
}

int hl78xx_api_func_get_rsrp_validity(const struct device *dev, bool *is_valid)
{
	int ret;
	int16_t rsrp;

	if ((dev == NULL) || (is_valid == NULL)) {
		return -EINVAL;
	}

	ret = hl78xx_api_func_get_signal(dev, CELLULAR_SIGNAL_RSRP, &rsrp);
	if (ret < 0) {
		return ret;
	}

	*is_valid = hl78xx_is_rsrp_value_valid(rsrp);

	return 0;
}

int hl78xx_api_func_get_rsrq_validity(const struct device *dev, bool *is_valid)
{
	int ret;
	int16_t rsrq;

	if ((dev == NULL) || (is_valid == NULL)) {
		return -EINVAL;
	}

	ret = hl78xx_api_func_get_signal(dev, CELLULAR_SIGNAL_RSRQ, &rsrq);
	if (ret < 0) {
		return ret;
	}

	*is_valid = hl78xx_is_rsrq_value_valid(rsrq);

	return 0;
}

int hl78xx_api_func_get_sinr_validity(const struct device *dev, bool *is_valid)
{
	int ret;
	int16_t sinr;

	if ((dev == NULL) || (is_valid == NULL)) {
		return -EINVAL;
	}

	ret = hl78xx_api_func_get_network_info(dev, HL78XX_NETWORK_INFO_SINR, &sinr, sizeof(sinr));
	if (ret < 0) {
		return ret;
	}

	*is_valid = hl78xx_is_sinr_value_valid(sinr);

	return 0;
}

/** Convert hl78xx RAT mode to cellular access technology */
enum cellular_access_technology hl78xx_rat_to_access_tech(enum hl78xx_cell_rat_mode rat_mode)
{
	switch (rat_mode) {
	case HL78XX_RAT_CAT_M1:
		return CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN;
	case HL78XX_RAT_NB1:
		return CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1;
#ifdef CONFIG_MODEM_HL78XX_12
	case HL78XX_RAT_GSM:
		return CELLULAR_ACCESS_TECHNOLOGY_GSM;
#ifdef CONFIG_MODEM_HL78XX_12_FW_R6
	case HL78XX_RAT_NBNTN:
		/** NBNTN might not have a direct mapping; choose closest or define new */
		return CELLULAR_ACCESS_TECHNOLOGY_NG_RAN_SAT;
#endif
#endif
#ifdef CONFIG_MODEM_HL78XX_AUTORAT
	case HL78XX_RAT_MODE_AUTO:
		/** AUTO mode doesn't map directly; return LTE as default or NONE */
		return CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN;
#endif
	case HL78XX_RAT_MODE_NONE:
	default:
		return CELLULAR_ACCESS_TECHNOLOGY_UNKNOWN;
	}
}

enum hl78xx_cell_rat_mode hl78xx_access_tech_to_rat(enum cellular_access_technology access_tech)
{
	switch (access_tech) {
	case CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN:
		return HL78XX_RAT_CAT_M1;
	case CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN_NB_S1:
		return HL78XX_RAT_NB1;
#ifdef CONFIG_MODEM_HL78XX_12
	case CELLULAR_ACCESS_TECHNOLOGY_GSM:
		return HL78XX_RAT_GSM;
#ifdef CONFIG_MODEM_HL78XX_12_FW_R6
	case CELLULAR_ACCESS_TECHNOLOGY_NG_RAN_SAT:
		return HL78XX_RAT_NBNTN;
#endif
#endif
	default:
		return HL78XX_RAT_MODE_NONE;
	}
}

int hl78xx_api_func_get_registration_status(const struct device *dev,
					    enum cellular_access_technology tech,
					    enum cellular_registration_status *status)
{
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	if (status == NULL) {
		return -EINVAL;
	}
	LOG_DBG("Requested tech: %d, current rat mode: %d REG: %d %d", tech,
		data->status.registration.rat_mode, data->status.registration.network_state_current,
		hl78xx_rat_to_access_tech(data->status.registration.rat_mode));
	if (tech != hl78xx_rat_to_access_tech(data->status.registration.rat_mode)) {
		return -ENODATA;
	}
	k_mutex_lock(&data->api_lock, K_FOREVER);
	*status = data->status.registration.network_state_current;
	k_mutex_unlock(&data->api_lock);
	return 0;
}

#define HL78XX_NETWORK_OPERATOR_CMD "AT+COPS?"

static int hl78xx_refresh_network_operator(struct hl78xx_data *data)
{
	int ret;

	ret = hl78xx_send_cmd(data, HL78XX_NETWORK_OPERATOR_CMD, NULL, hl78xx_get_allow_match(),
			      hl78xx_get_allow_match_size());
	if (ret < 0) {
		LOG_ERR("Failed to refresh network operator: %d", ret);
		return ret;
	}

	return 0;
}

static int hl78xx_copy_string_if_present(char *dst, size_t dst_size, const char *src)
{
	if (src[0] == '\0') {
		return -ENODATA;
	}

	safe_strncpy(dst, src, dst_size);

	return 0;
}

static int hl78xx_get_apn(struct hl78xx_data *data, void *info, size_t size)
{
	int ret = 0;

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (data->status.apn.state != APN_STATE_CONFIGURED) {
		ret = -ENODATA;
	} else {
		safe_strncpy((char *)info, (const char *)data->identity.apn, size);
	}

	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int hl78xx_get_current_rat(struct hl78xx_data *data, void *info, size_t size)
{
	if (size < sizeof(enum hl78xx_cell_rat_mode)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);
	*(enum hl78xx_cell_rat_mode *)info = data->status.registration.rat_mode;
	k_mutex_unlock(&data->api_lock);

	return 0;
}

static int hl78xx_get_operator_format(struct hl78xx_data *data, void *info, size_t size)
{
	int ret = 0;

	if (size < sizeof(enum hl78xx_operator_format)) {
		return -EINVAL;
	}

	if (!data->status.network_info.operator_info.has_operator) {
		ret = hl78xx_refresh_network_operator(data);
		if (ret < 0) {
			return ret;
		}
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (!data->status.network_info.operator_info.has_operator) {
		ret = -ENODATA;
	} else {
		*(enum hl78xx_operator_format *)info =
			data->status.network_info.operator_info.format;
	}

	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int hl78xx_prepare_operator_format(const struct device *dev, struct hl78xx_data *data,
					  enum hl78xx_operator_format format)
{
	int ret;
	bool already_set;

	k_mutex_lock(&data->api_lock, K_FOREVER);
	already_set = data->status.network_info.operator_info.has_operator &&
		      (data->status.network_info.operator_info.format == format);
	k_mutex_unlock(&data->api_lock);

	if (already_set) {
		return 0;
	}

	ret = hl78xx_api_func_set_network_operator_format(dev, format);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int hl78xx_get_operator_string(const struct device *dev, struct hl78xx_data *data,
				      enum hl78xx_operator_format format, void *info, size_t size)
{
	int ret = 0;

	ret = hl78xx_prepare_operator_format(dev, data, format);
	if (ret < 0) {
		return ret;
	}

	ret = hl78xx_refresh_network_operator(data);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (!data->status.network_info.operator_info.has_operator) {
		ret = -ENODATA;
	} else {
		safe_strncpy((char *)info,
			     (const char *)data->status.network_info.operator_info.operator_name,
			     size);
	}

	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int hl78xx_get_tac(struct hl78xx_data *data, void *info, size_t size)
{
	int ret = 0;

	if (size < sizeof(uint32_t)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (!data->status.cxreg.has_tac) {
		ret = -ENODATA;
	} else {
		*(uint32_t *)info = data->status.cxreg.tac;
	}

	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int hl78xx_get_mcc(struct hl78xx_data *data, void *info, size_t size)
{
	int ret = 0;

	if (size < sizeof(uint16_t)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (!data->status.network_info.operator_info.has_mcc) {
		ret = -ENODATA;
	} else {
		*(uint16_t *)info = data->status.network_info.operator_info.mcc;
	}

	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int hl78xx_get_mnc(struct hl78xx_data *data, void *info, size_t size)
{
	int ret = 0;

	if (size < sizeof(uint16_t)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (!data->status.network_info.operator_info.has_mnc) {
		ret = -ENODATA;
	} else {
		*(uint16_t *)info = data->status.network_info.operator_info.mnc;
	}

	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int hl78xx_get_cell_id(struct hl78xx_data *data, void *info, size_t size)
{
	int ret = 0;

	if (size < sizeof(uint32_t)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (!data->status.cxreg.has_cell_id) {
		ret = -ENODATA;
	} else {
		*(uint32_t *)info = data->status.cxreg.cell_id;
	}

	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int hl78xx_get_ip_address(struct hl78xx_data *data, void *info, size_t size)
{
	int ret;

	k_mutex_lock(&data->api_lock, K_FOREVER);
	ret = hl78xx_copy_string_if_present((char *)info, size,
					    (const char *)data->status.network_info.ip_address);
	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int hl78xx_get_dns_primary(struct hl78xx_data *data, void *info, size_t size)
{
	int ret;

	k_mutex_lock(&data->api_lock, K_FOREVER);
	ret = hl78xx_copy_string_if_present((char *)info, size,
					    (const char *)data->status.network_info.dns_primary);
	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int hl78xx_get_active_band(struct hl78xx_data *data, void *info, size_t size)
{
	int ret;

	if (size < sizeof(uint16_t)) {
		return -EINVAL;
	}

#ifdef CONFIG_MODEM_HL78XX_12
	if (data->status.registration.rat_mode == HL78XX_RAT_GSM) {
		return -ENOTSUP;
	}
#endif /* CONFIG_MODEM_HL78XX_12 */

	ret = hl78xx_send_cmd(data, "AT+KBND?", NULL, hl78xx_get_allow_match(),
			      hl78xx_get_allow_match_size());
	if (ret < 0) {
		LOG_ERR("Failed to query active band: %d", ret);
		return ret;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);

	ret = hl78xx_band_bitmap_to_number((const char *)data->status.band.active_band.bnd_bitmap);
	if (ret > 0) {
		*(uint16_t *)info = (uint16_t)ret;
		ret = 0;
	}

	k_mutex_unlock(&data->api_lock);

	return ret;
}

static int hl78xx_get_sinr(struct hl78xx_data *data, void *info, size_t size)
{
	int ret = 0;

	if (size < sizeof(int16_t)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->api_lock, K_FOREVER);

	if (!data->status.kcellmeas.bootstrap_done) {
		ret = -ENODATA;
	} else {
		*(int16_t *)info = data->status.signal.sinr;
	}

	k_mutex_unlock(&data->api_lock);

	return ret;
}

int hl78xx_api_func_get_network_info(const struct device *dev, enum hl78xx_network_info_type type,
				     void *info, size_t size)
{
	struct hl78xx_data *data;

	if ((dev == NULL) || (dev->data == NULL) || (info == NULL) || (size == 0U)) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;

	switch (type) {
	case HL78XX_NETWORK_INFO_APN:
		return hl78xx_get_apn(data, info, size);

	case HL78XX_NETWORK_INFO_CURRENT_RAT:
		return hl78xx_get_current_rat(data, info, size);

	case HL78XX_NETWORK_INFO_OPERATOR_FORMAT:
		return hl78xx_get_operator_format(data, info, size);

	case HL78XX_NETWORK_INFO_NETWORK_OPERATOR_LONG_ALPHA:
		return hl78xx_get_operator_string(
			dev, data, HL78XX_OPERATOR_FORMAT_LONG_ALPHANUMERIC, info, size);

	case HL78XX_NETWORK_INFO_NETWORK_OPERATOR_SHORT_ALPHA:
		return hl78xx_get_operator_string(
			dev, data, HL78XX_OPERATOR_FORMAT_SHORT_ALPHANUMERIC, info, size);

	case HL78XX_NETWORK_INFO_NETWORK_OPERATOR_NUMERIC:
		return hl78xx_get_operator_string(dev, data, HL78XX_OPERATOR_FORMAT_NUMERIC, info,
						  size);

	case HL78XX_NETWORK_INFO_TAC:
		return hl78xx_get_tac(data, info, size);

	case HL78XX_NETWORK_INFO_MCC:
		return hl78xx_get_mcc(data, info, size);

	case HL78XX_NETWORK_INFO_MNC:
		return hl78xx_get_mnc(data, info, size);

	case HL78XX_NETWORK_INFO_CELL_ID:
		return hl78xx_get_cell_id(data, info, size);

	case HL78XX_NETWORK_INFO_IP_ADDRESS:
		return hl78xx_get_ip_address(data, info, size);

	case HL78XX_NETWORK_INFO_DNS_PRIMARY:
		return hl78xx_get_dns_primary(data, info, size);

	case HL78XX_NETWORK_INFO_ACTIVE_BAND:
		return hl78xx_get_active_band(data, info, size);

	case HL78XX_NETWORK_INFO_SINR:
		return hl78xx_get_sinr(data, info, size);

	default:
		return -ENOTSUP;
	}
}

int hl78xx_api_func_get_modem_info(const struct device *dev, enum hl78xx_modem_info_type type,
				   void *info, size_t size)
{
	struct hl78xx_data *data;

	if (dev == NULL || dev->data == NULL || info == NULL || size == 0U) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;

	switch (type) {
	case HL78XX_MODEM_INFO_SERIAL_NUMBER:
		k_mutex_lock(&data->api_lock, K_FOREVER);
		safe_strncpy((char *)info, (const char *)data->identity.serial_number, size);
		k_mutex_unlock(&data->api_lock);
		return 0;

	case HL78XX_MODEM_INFO_CURRENT_BAUD_RATE:
		if (size < sizeof(uint32_t)) {
			return -EINVAL;
		}

		k_mutex_lock(&data->api_lock, K_FOREVER);
		*(uint32_t *)info = data->status.uart.current_baudrate;
		k_mutex_unlock(&data->api_lock);
		return 0;
	default:
		return -ENOTSUP;
	}
}

int hl78xx_api_func_get_modem_info_standard(const struct device *dev,
					    enum cellular_modem_info_type type, char *info,
					    size_t size)
{
	int ret = 0;
	struct hl78xx_data *data;

	if ((dev == NULL) || (dev->data == NULL) || (info == NULL) || (size == 0U)) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;
	/* copy identity under api lock to a local buffer then write to caller
	 * prevents holding lock during the return/caller access
	 */
	k_mutex_lock(&data->api_lock, K_FOREVER);
	switch (type) {
	case CELLULAR_MODEM_INFO_IMEI:
		safe_strncpy(info, (const char *)data->identity.imei,
			     MIN(size, sizeof(data->identity.imei)));
		break;
	case CELLULAR_MODEM_INFO_SIM_IMSI:
		safe_strncpy(info, (const char *)data->identity.imsi,
			     MIN(size, sizeof(data->identity.imsi)));
		break;
	case CELLULAR_MODEM_INFO_MANUFACTURER:
		safe_strncpy(info, (const char *)data->identity.manufacturer,
			     MIN(size, sizeof(data->identity.manufacturer)));
		break;
	case CELLULAR_MODEM_INFO_FW_VERSION:
		safe_strncpy(info, (const char *)data->identity.fw_version,
			     MIN(size, sizeof(data->identity.fw_version)));
		break;
	case CELLULAR_MODEM_INFO_MODEL_ID:
		safe_strncpy(info, (const char *)data->identity.model_id,
			     MIN(size, sizeof(data->identity.model_id)));
		break;
	case CELLULAR_MODEM_INFO_SIM_ICCID:
		safe_strncpy(info, (const char *)data->identity.iccid,
			     MIN(size, sizeof(data->identity.iccid)));
		break;
	default:
		ret = -ENOTSUP;
		break;
	}
	k_mutex_unlock(&data->api_lock);
	return ret;
}

int hl78xx_api_func_set_apn(const struct device *dev, const char *apn)
{
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	/**
	 * Validate APN
	 * APN can be empty string to clear it
	 * to request it from network
	 * If the value is null or omitted, then the subscription
	 * value will be requested
	 */
	if (apn == NULL) {
		return -EINVAL;
	}
	if (strlen(apn) >= MDM_APN_MAX_LENGTH) {
		return -EINVAL;
	}
	/* Update in-memory APN under api lock */
	k_mutex_lock(&data->api_lock, K_FOREVER);
	safe_strncpy(data->identity.apn, apn, sizeof(data->identity.apn));
	data->status.apn.state = APN_STATE_REFRESH_REQUESTED;
	k_mutex_unlock(&data->api_lock);
	hl78xx_enter_state(data, MODEM_HL78XX_STATE_CARRIER_OFF);
	return 0;
}

int hl78xx_api_func_set_network_operator_format(const struct device *dev,
						enum hl78xx_operator_format format)
{
	struct hl78xx_data *data;

	if ((dev == NULL) || (dev->data == NULL)) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;

	if (format > HL78XX_OPERATOR_FORMAT_NUMERIC) {
		return -EINVAL;
	}

	return hl78xx_set_network_operator_format(data, format);
}

#ifdef CONFIG_MODEM_HL78XX_AUTORAT
int hl78xx_api_func_set_prl(const struct device *dev, const struct kselacq_syntax kselacq_rats)
{
	struct hl78xx_data *data;

	if (dev == NULL || dev->data == NULL) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;

	return hl78xx_set_prl_internal(data, kselacq_rats);
}

int hl78xx_api_func_get_prl(const struct device *dev, struct kselacq_syntax *kselacq_rats)
{
	struct hl78xx_data *data;

	if (dev == NULL || dev->data == NULL || kselacq_rats == NULL) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;

	k_mutex_lock(&data->api_lock, K_FOREVER);
	*kselacq_rats = data->kselacq_data;
	k_mutex_unlock(&data->api_lock);

	return 0;
}
#endif /* CONFIG_MODEM_HL78XX_AUTORAT */

int hl78xx_set_runtime_band_provider(const struct device *dev,
				     hl78xx_runtime_band_provider_t provider, void *user_data)
{
	struct hl78xx_data *data;

	if ((dev == NULL) || (dev->data == NULL)) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;

	k_mutex_lock(&data->api_lock, K_FOREVER);
	data->runtime_band.provider = provider;
	data->runtime_band.provider_user_data = user_data;
	k_mutex_unlock(&data->api_lock);

	LOG_DBG("Runtime band provider %s", (provider != NULL) ? "registered" : "cleared");

	return 0;
}

int hl78xx_api_func_set_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality functionality,
					    bool reset)
{
	char cmd_string[sizeof(SET_FULLFUNCTIONAL_MODE_CMD) + sizeof(int)] = {0};
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	struct hl78xx_evt event = {.type = HL78XX_LTE_PHONE_FUNCTIONALITY_UPDATE};
	int ret = 0;

#ifdef CONFIG_HL78XX_GNSS
	if (!reset && functionality == HL78XX_FULLY_FUNCTIONAL &&
	    data->status.phone_functionality.functionality == HL78XX_AIRPLANE &&
	    data->status.state != MODEM_HL78XX_STATE_RUN_ENABLE_GPRS_SCRIPT &&
	    data->status.boot.init_sequence_completed == false) {
		/* RUN_ENABLE_GPRS_SCRIPT is already the deferred LTE restore path. */
		LOG_INF("Restoring LTE after GNSS: delegating to modem_workq");
		hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_LTE_RESTORE_REQUESTED);
		return 0;
	}
#endif /* CONFIG_HL78XX_GNSS */
	LOG_DBG("Setting phone functionality to %d with reset %d", functionality, reset);

	/* configure modem functionality with/without restart  */
	snprintf(cmd_string, sizeof(cmd_string), "AT+CFUN=%d,%d", functionality, reset);
	ret = hl78xx_send_cmd(data, cmd_string, NULL, hl78xx_get_ok_match(),
			      hl78xx_get_ok_match_size());
	if (ret == 0) {
		data->status.phone_functionality.in_progress = true;
		data->status.phone_functionality.functionality = functionality;
		event.content.value = functionality;
		event_dispatcher_dispatch(&event);
	}

	return ret;
}

int hl78xx_api_func_restart(const struct device *dev, enum hl78xx_modem_restart_mode mode)
{
	struct hl78xx_data *data;
	const struct hl78xx_config *config;
	enum hl78xx_state state;

	if ((dev == NULL) || (dev->data == NULL)) {
		return -EINVAL;
	}

	if ((mode != HL78XX_MODEM_RESTART_HARD) && (mode != HL78XX_MODEM_RESTART_SOFT)) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;
	config = dev->config;

	k_mutex_lock(&data->api_lock, K_FOREVER);
	if (data->status.restart.requested) {
		k_mutex_unlock(&data->api_lock);
		return -EBUSY;
	}

	state = data->status.state;
	if ((mode == HL78XX_MODEM_RESTART_HARD) &&
	    !hl78xx_gpio_is_enabled(&config->mdm_gpio_reset)) {
		k_mutex_unlock(&data->api_lock);
		return -ENOTSUP;
	}

	if ((mode == HL78XX_MODEM_RESTART_SOFT) && !hl78xx_soft_restart_state_is_ready(state)) {
		k_mutex_unlock(&data->api_lock);
		return -EAGAIN;
	}

	data->status.restart.requested = true;
	data->status.restart.mode = mode;
	k_mutex_unlock(&data->api_lock);

	LOG_INF("Restart requested via %s path",
		mode == HL78XX_MODEM_RESTART_HARD ? "hard" : "soft");
	hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_RESTART_REQUESTED);

	return 0;
}

int hl78xx_api_func_get_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality *functionality)
{
	const char *cmd_string = GET_FULLFUNCTIONAL_MODE_CMD;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	/* get modem phone functionality */
	int ret = hl78xx_send_cmd(data, cmd_string, NULL, hl78xx_get_ok_match(),
				  hl78xx_get_ok_match_size());

	/* hl78xx_on_cfun() updates data->status.phone_functionality.functionality
	 * synchronously before the OK is matched. Copy it back to the caller.
	 */
	if (ret == 0 && functionality != NULL) {
		*functionality = data->status.phone_functionality.functionality;
	}

	return ret;
}

int hl78xx_api_func_modem_dynamic_cmd_send(const struct device *dev, const char *cmd,
					   uint16_t cmd_size,
					   const struct modem_chat_match *response_matches,
					   uint16_t matches_size)
{
	struct hl78xx_data *data;

	if (dev == NULL || dev->data == NULL || cmd == NULL) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;

	/* respect provided matches_size and serialize modem access */
	struct at_cmd_ctx ctx = {
		.cmd = cmd,
		.cmd_len = cmd_size,
		.matches = response_matches,
		.match_count = matches_size,
		.response = NULL,
		.response_size = 0U,
	};
	return hl78xx_at_cmd_execute(data, NULL, &ctx);
}
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
int hl78xx_start_airvantage_dm_session(const struct device *dev)
{
	int ret = 0;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	ret = modem_dynamic_cmd_send_req(
		data, &(const struct hl78xx_dynamic_cmd_request){
			      .script_user_callback = NULL,
			      .cmd = (const uint8_t *)WDSI_USER_INITIATED_CONNECTION_START_CMD,
			      .cmd_len = strlen(WDSI_USER_INITIATED_CONNECTION_START_CMD),
			      .response_matches = hl78xx_get_ok_match(),
			      .matches_size = hl78xx_get_ok_match_size(),
			      .response_timeout = MDM_CMD_TIMEOUT,
			      .user_cmd = false,
		      });
	if (ret < 0) {
		LOG_ERR("Start DM session error %d", ret);
		return ret;
	}
	return 0;
}

int hl78xx_stop_airvantage_dm_session(const struct device *dev)
{
	int ret = 0;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	ret = modem_dynamic_cmd_send_req(
		data, &(const struct hl78xx_dynamic_cmd_request){
			      .script_user_callback = NULL,
			      .cmd = (const uint8_t *)WDSI_USER_INITIATED_CONNECTION_STOP_CMD,
			      .cmd_len = strlen(WDSI_USER_INITIATED_CONNECTION_STOP_CMD),
			      .response_matches = hl78xx_get_ok_match(),
			      .matches_size = hl78xx_get_ok_match_size(),
			      .response_timeout = MDM_CMD_TIMEOUT,
			      .user_cmd = false,
		      });
	if (ret < 0) {
		LOG_ERR("Stop DM session error %d", ret);
		return ret;
	}
	return 0;
}
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */

static int hl78xx_set_wake_pin_level(const struct device *dev, int value)
{
	struct hl78xx_data *data;
	const struct hl78xx_config *config;
	int ret;

	if (dev == NULL) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;
	config = dev->config;
	if (data == NULL || config == NULL) {
		return -EINVAL;
	}

	if (!hl78xx_gpio_is_enabled(&config->mdm_gpio_wake)) {
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&config->mdm_gpio_wake, value);
	if (ret < 0) {
		LOG_ERR("Failed to set WAKE pin to %d: %d", value, ret);
		return ret;
	}

	LOG_DBG("Set WAKE pin to %d via public API", value);
	return 0;
}

int hl78xx_set_wake_pin_low(const struct device *dev)
{
	return hl78xx_set_wake_pin_level(dev, 0);
}

int hl78xx_set_wake_pin_high(const struct device *dev)
{
	return hl78xx_set_wake_pin_level(dev, 1);
}

int hl78xx_set_active_sim(const struct device *dev, enum hl78xx_sim_slot sim_slot)
{
	struct hl78xx_data *data;
	const struct hl78xx_config *config;
	int ret;
	int value;

	if (dev == NULL) {
		return -EINVAL;
	}

	data = (struct hl78xx_data *)dev->data;
	config = dev->config;
	if ((data == NULL) || (config == NULL)) {
		return -EINVAL;
	}

	if (!hl78xx_gpio_is_enabled(&config->mdm_gpio_sim_switch)) {
		return -ENOTSUP;
	}

	switch (sim_slot) {
	case HL78XX_SIM_SLOT_1:
		value = 0;
		break;
	case HL78XX_SIM_SLOT_2:
		value = 1;
		break;
	default:
		return -EINVAL;
	}

	ret = gpio_pin_set_dt(&config->mdm_gpio_sim_switch, value);
	if (ret < 0) {
		LOG_ERR("Failed to select SIM slot %d: %d", sim_slot + 1, ret);
		return ret;
	}

	LOG_DBG("Selected SIM slot %d via public API", sim_slot + 1);
	return 0;
}

#ifdef CONFIG_MODEM_HL78XX_LOW_POWER_MODE
int hl78xx_wakeup_modem(const struct device *dev)
{
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	if (data == NULL) {
		return -EINVAL;
	}

	if (data->status.state != MODEM_HL78XX_STATE_SLEEP &&
	    data->status.state != MODEM_HL78XX_STATE_IDLE) {
		LOG_DBG("Modem not in sleep state (state=%d), wakeup not needed",
			data->status.state);
		return -EALREADY;
	}

	LOG_INF("User-initiated modem wakeup from low power mode");
	hl78xx_delegate_event(data, MODEM_HL78XX_EVENT_RESUME);
	return 0;
}

#ifdef CONFIG_MODEM_HL78XX_EDRX
int hl78xx_edrx_get_time_to_sleep(const struct device *dev)
{
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	if (data == NULL) {
		return -EINVAL;
	}

	if (!hl78xx_is_edrx_idle_scheduled(data)) {
		LOG_DBG("Not scheduled eDRX idle state, time to sleep not available");
		return -ENODATA;
	}

	return hl78xx_edrx_idle_get_remaining_timetosleep(data);
}
#endif /* CONFIG_MODEM_HL78XX_EDRX */
#endif /* CONFIG_MODEM_HL78XX_LOW_POWER_MODE */

#ifdef CONFIG_HL78XX_GNSS

/**
 * @brief Helper function to extract GNSS and modem data from either device type
 *
 * This function handles both device types:
 * - GNSS device (gnss_dev): dev->data is hl78xx_gnss_data directly
 * - Modem device: dev->data is hl78xx_data, navigate via gnss_dev
 *
 * @param dev The device (either GNSS or modem device)
 * @param gnss_data_out Output pointer for GNSS data structure
 * @param modem_data_out Output pointer for modem data structure (optional, can be NULL)
 * @return 0 on success, negative errno on failure
 */
static int hl78xx_get_gnss_context(const struct device *dev,
				   struct hl78xx_gnss_data **gnss_data_out,
				   struct hl78xx_data **modem_data_out)
{
	if (dev == NULL || dev->data == NULL) {
		return -EINVAL;
	}

	/* Check if this is a GNSS device by checking for GNSS API */
	if (DEVICE_API_IS(gnss, dev)) {
		/* GNSS device path: dev->data is hl78xx_gnss_data directly */
		struct hl78xx_gnss_data *gnss_data = (struct hl78xx_gnss_data *)dev->data;

		*gnss_data_out = gnss_data;

		if (modem_data_out != NULL) {
			*modem_data_out = gnss_data->parent_data;
		}
	} else {
		/* Modem device path: dev->data is hl78xx_data */
		struct hl78xx_data *modem_data = (struct hl78xx_data *)dev->data;

		if (modem_data->devices.gnss == NULL || modem_data->devices.gnss->data == NULL) {
			return -EINVAL;
		}

		/* gnss_dev->data is hl78xx_gnss_data */
		*gnss_data_out = (struct hl78xx_gnss_data *)modem_data->devices.gnss->data;

		if (modem_data_out != NULL) {
			*modem_data_out = modem_data;
		}
	}

	if (*gnss_data_out == NULL) {
		return -EINVAL;
	}

	return 0;
}

int hl78xx_enter_gnss_mode(const struct device *dev)
{
	struct hl78xx_gnss_data *data_gnss = NULL;
	struct hl78xx_data *data_modem = NULL;

	if (hl78xx_get_gnss_context(dev, &data_gnss, &data_modem) < 0) {
		return -EINVAL;
	}

	if (data_modem == NULL) {
		return -EINVAL;
	}

	/* Check if already in GNSS mode */
	if (hl78xx_is_in_gnss_mode(data_modem)) {
		LOG_DBG("Already in GNSS mode");
		return -EALREADY;
	}

	/* Clear exit pending flag */
	data_gnss->exit_to_lte_pending = false;

	data_gnss->gnss_mode_enter_pending = true;

	LOG_DBG("Requesting GNSS mode entry %d", data_modem->status.state);

	/* Request GNSS mode entry via state machine event */
	hl78xx_delegate_event(data_modem, MODEM_HL78XX_EVENT_GNSS_MODE_ENTER_REQUESTED);

	return 0;
}

int hl78xx_exit_gnss_mode(const struct device *dev)
{
	struct hl78xx_gnss_data *data_gnss = NULL;
	struct hl78xx_data *data_modem = NULL;

	if (hl78xx_get_gnss_context(dev, &data_gnss, &data_modem) < 0) {
		return -EINVAL;
	}

	if (data_modem == NULL) {
		return -EINVAL;
	}

	/* Check if not in GNSS mode */
	if (!hl78xx_is_in_gnss_mode(data_modem)) {
		LOG_DBG("Not in GNSS mode, nothing to exit");
		return -EALREADY;
	}

	/* Request GNSS mode exit via state machine event */
	hl78xx_delegate_event(data_modem, MODEM_HL78XX_EVENT_GNSS_MODE_EXIT_REQUESTED);

	return 0;
}

int hl78xx_queue_gnss_search(const struct device *dev)
{
	struct hl78xx_gnss_data *data_gnss = NULL;
	struct hl78xx_data *data_modem = NULL;

	if (hl78xx_get_gnss_context(dev, &data_gnss, &data_modem) < 0) {
		return -EINVAL;
	}
	ARG_UNUSED(data_modem);

	return hl78xx_gnss_queue_search(data_gnss);
}

int hl78xx_gnss_set_nmea_output(const struct device *dev, enum nmea_output_port port)
{
	struct hl78xx_gnss_data *data_gnss = NULL;
	struct hl78xx_data *data_modem = NULL;

	if (hl78xx_get_gnss_context(dev, &data_gnss, &data_modem) < 0) {
		return -EINVAL;
	}
	ARG_UNUSED(data_modem);

	/* Don't allow changes while GNSS is active */
	if (hl78xx_gnss_is_active(data_gnss)) {
		LOG_WRN("Cannot change NMEA output while GNSS is active");
		return -EBUSY;
	}

	data_gnss->output_port = port;
	return 0;
}

int hl78xx_gnss_set_search_timeout(const struct device *dev, uint32_t timeout_ms)
{
	struct hl78xx_gnss_data *data_gnss = NULL;
	struct hl78xx_data *data_modem = NULL;

	if (hl78xx_get_gnss_context(dev, &data_gnss, &data_modem) < 0) {
		return -EINVAL;
	}
	ARG_UNUSED(data_modem);

	/* Don't allow changes while GNSS is active */
	if (hl78xx_gnss_is_active(data_gnss)) {
		LOG_WRN("Cannot change search timeout while GNSS is active");
		return -EBUSY;
	}

	data_gnss->search_timeout_ms = timeout_ms;
	return 0;
}

int hl78xx_gnss_get_latest_known_fix(const struct device *dev)
{
	struct hl78xx_gnss_data *data_gnss = NULL;
	struct hl78xx_data *data_modem = NULL;

	if (hl78xx_get_gnss_context(dev, &data_gnss, &data_modem) < 0) {
		return -EINVAL;
	}
	ARG_UNUSED(data_gnss);

	return hl78xx_run_gnss_gnssloc_script(data_modem);
}

#ifdef CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE
/**
 * @brief Validate that days value is one of the allowed values
 */
static bool hl78xx_agnss_validate_days(enum hl78xx_agnss_days days)
{
	switch (days) {
	case HL78XX_AGNSS_DAYS_1:
	case HL78XX_AGNSS_DAYS_2:
	case HL78XX_AGNSS_DAYS_3:
	case HL78XX_AGNSS_DAYS_7:
	case HL78XX_AGNSS_DAYS_14:
	case HL78XX_AGNSS_DAYS_28:
		return true;
	default:
		return false;
	}
}

int hl78xx_gnss_assist_data_get_status(const struct device *dev, struct hl78xx_agnss_status *status)
{
	int ret;
	const char *cmd = "AT+GNSSAD?";
	struct hl78xx_gnss_data *data_gnss = NULL;
	struct hl78xx_data *data_modem = NULL;

	if (hl78xx_get_gnss_context(dev, &data_gnss, &data_modem) < 0) {
		return -EINVAL;
	}

	if (status == NULL) {
		return -EINVAL;
	}

	if (data_modem == NULL) {
		return -EINVAL;
	}

	/* Send the query command - response is parsed by hl78xx_on_gnssad handler */
	ret = hl78xx_send_cmd(data_modem, cmd, NULL, hl78xx_get_gnssad_match(), 1);

	if (ret < 0) {
		LOG_ERR("Failed to query A-GNSS status: %d", ret);
		return ret;
	}

	/* Copy the parsed status to caller */
	status->mode = data_gnss->agnss_status.mode;
	status->days = data_gnss->agnss_status.days;
	status->hours = data_gnss->agnss_status.hours;
	status->minutes = data_gnss->agnss_status.minutes;

	return 0;
}

int hl78xx_gnss_assist_data_download(const struct device *dev, enum hl78xx_agnss_days days)
{
	int ret;
	char cmd[32];
	struct hl78xx_gnss_data *data_gnss = NULL;
	struct hl78xx_data *data_modem = NULL;

	if (hl78xx_get_gnss_context(dev, &data_gnss, &data_modem) < 0) {
		return -EINVAL;
	}
	ARG_UNUSED(data_gnss);

	/* Validate days parameter */
	if (!hl78xx_agnss_validate_days(days)) {
		LOG_ERR("Invalid days value for A-GNSS download: %d", days);
		return -EINVAL;
	}

	if (data_modem == NULL) {
		return -EINVAL;
	}

	/* Build the download command: AT+GNSSAD=1,<days> */
	ret = snprintf(cmd, sizeof(cmd), "AT+GNSSAD=1,%d", (int)days);
	if (ret < 0 || ret >= (int)sizeof(cmd)) {
		return -ENOMEM;
	}

	/* Send the download command */
	ret = hl78xx_send_cmd(data_modem, cmd, NULL, hl78xx_get_ok_match(),
			      hl78xx_get_ok_match_size());

	if (ret < 0) {
		LOG_ERR("Failed to download A-GNSS data: %d", ret);
		return ret;
	}

	LOG_INF("A-GNSS data download initiated for %d days", (int)days);
	return 0;
}

int hl78xx_gnss_assist_data_delete(const struct device *dev)
{
	int ret;
	const char *cmd = "AT+GNSSAD=0";
	struct hl78xx_gnss_data *data_gnss = NULL;
	struct hl78xx_data *data_modem = NULL;

	if (hl78xx_get_gnss_context(dev, &data_gnss, &data_modem) < 0) {
		return -EINVAL;
	}

	if (data_modem == NULL) {
		return -EINVAL;
	}

	/* Send the delete command */
	ret = hl78xx_send_cmd(data_modem, cmd, NULL, hl78xx_get_ok_match(),
			      hl78xx_get_ok_match_size());

	if (ret < 0) {
		LOG_ERR("Failed to delete A-GNSS data: %d", ret);
		return ret;
	}

	/* Clear local status */
	data_gnss->agnss_status.mode = HL78XX_AGNSS_MODE_INVALID;
	data_gnss->agnss_status.days = 0;
	data_gnss->agnss_status.hours = 0;
	data_gnss->agnss_status.minutes = 0;

	LOG_INF("A-GNSS data deleted");
	return 0;
}
#endif /* CONFIG_HL78XX_GNSS_SUPPORT_ASSISTED_MODE */
#endif /* CONFIG_HL78XX_GNSS */
