/*
 * Copyright (c) 2025 Netfeasa
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

LOG_MODULE_REGISTER(hl78xx_apis, CONFIG_MODEM_LOG_LEVEL);

static void hl78xx_chat_callback_handler(struct modem_chat *chat,
					 enum modem_chat_script_result result, void *user_data)
{
	if (result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS) {
		#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
		LOG_DBG("%d RUN MODEM_CHAT_SCRIPT_RESULT_SUCCESS", __LINE__);
		#endif
	} else {
		#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
		LOG_DBG("%d RUN MODEM_CHAT_SCRIPT_RESULT_FAIL", __LINE__);
		#endif
	}
}

static void hl78xx_on_cmerror(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s %s", __LINE__, __func__, argv[0]);
#endif
}

static void hl78xx_on_ok(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_DBG("%d %s %s", __LINE__, __func__, argv[0]);
#endif
}

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", hl78xx_on_ok);
MODEM_CHAT_MATCHES_DEFINE(allow_match, MODEM_CHAT_MATCH("OK", "", hl78xx_on_ok),
			  MODEM_CHAT_MATCH("+CME ERROR: ", "", hl78xx_on_cmerror));

int hl78xx_api_func_get_signal(const struct device *dev, const enum hl78xx_signal_type type,
			       int16_t *value)
{
	int ret = -ENOTSUP;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	const char *signal_cmd_csq = "AT+CSQ";
	const char *signal_cmd_cesq = "AT+CESQ";

	if (data->status.state != MODEM_HL78XX_STATE_CARRIER_ON) {
		return -ENODATA;
	}

	/* Run chat script */
	switch (type) {
	case HL78XX_SIGNAL_RSSI:
		ret = modem_cmd_send_int(data, hl78xx_chat_callback_handler, signal_cmd_csq,
					 strlen(signal_cmd_csq), allow_match, 2, true);
		break;

	case HL78XX_SIGNAL_RSRP:
	case HL78XX_SIGNAL_RSRQ:
		ret = modem_cmd_send_int(data, hl78xx_chat_callback_handler, signal_cmd_cesq,
					 strlen(signal_cmd_cesq), allow_match, 2, true);
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
	case HL78XX_SIGNAL_RSSI:
		ret = hl78xx_parse_rssi(data->status.rssi, value);
		break;

	case HL78XX_SIGNAL_RSRP:
		ret = hl78xx_parse_rsrp(data->status.rsrp, value);
		break;

	case HL78XX_SIGNAL_RSRQ:
		ret = hl78xx_parse_rsrq(data->status.rsrq, value);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

int hl78xx_api_func_get_registration_status(const struct device *dev,
					    enum hl78xx_cell_rat_mode *tech,
					    enum hl78xx_registration_status *status)
{
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	if (tech == NULL || status == NULL) {
		return -EINVAL;
	}

	*tech = data->status.registration.rat_mode;
	*status = data->status.registration.network_state;

	return 0;
}

int hl78xx_api_func_get_modem_info(const struct device *dev, enum hl78xx_modem_info_type type,
				   char *info, size_t size)
{
	int ret = 0;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	if (info == NULL || size == 0) {
		return -EINVAL;
	}

	switch (type) {
	case HL78XX_MODEM_INFO_IMEI:
		strncpy(info, data->identity.imei, MIN(size, sizeof(data->identity.imei)));
		break;
	case HL78XX_MODEM_INFO_SIM_IMSI:
		strncpy(info, data->identity.imsi, MIN(size, sizeof(data->identity.imsi)));
		break;
	case HL78XX_MODEM_INFO_MANUFACTURER:
		strncpy(info, data->identity.manufacturer,
			MIN(size, sizeof(data->identity.manufacturer)));
		break;
	case HL78XX_MODEM_INFO_FW_VERSION:
		strncpy(info, data->identity.fw_version,
			MIN(size, sizeof(data->identity.fw_version)));
		break;
	case HL78XX_MODEM_INFO_MODEL_ID:
		strncpy(info, data->identity.model_id, MIN(size, sizeof(data->identity.model_id)));
		break;
	case HL78XX_MODEM_INFO_SIM_ICCID:
		strncpy(info, data->identity.iccid, MIN(size, sizeof(data->identity.iccid)));
		break;
	case HL78XX_MODEM_INFO_APN:
		strncpy(info, data->identity.apn, MIN(size, sizeof(data->identity.apn)));
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

int hl78xx_set_apn_internal(struct hl78xx_data *data, const char *apn, uint16_t size)
{
	int ret = 0;
	char cmd_string[sizeof("AT+KCNXCFG=,\"\",\"\"") + sizeof(uint8_t) +
			sizeof(MODEM_HL78XX_ADDRESS_FAMILY) + MDM_APN_MAX_LENGTH] = {0};
	int cmd_max_len = sizeof(cmd_string) - 1;
	int apn_size = strlen(apn);

	if (apn == NULL || size >= MDM_APN_MAX_LENGTH) {
		return -EINVAL;
	}

	if (strncmp(data->identity.apn, apn, apn_size) != 0) {
		strncpy(data->identity.apn, apn, apn_size);
	}
	/* check if the pdp is active, if yes, disable it first.*/
	/* Important: Deactivating all PDP contexts (e.g. by using AT+CGACT=0 with no <cid>
	 *	parameters) also causes the device to detach from the network (equivalent to
	 *	AT+CGATT=0)
	 * Theorically it is also equivalent to at+cfun=4
	 * to keep sync use SET_AIRPLANE_MODE_CMD_LEGACY to deactivate pdp context if you have
	 * only one pdp context
	 */
	snprintk(cmd_string, cmd_max_len, "AT+CGDCONT=1,\"%s\",\"%s\"", MODEM_HL78XX_ADDRESS_FAMILY,
		 apn);

	ret = modem_cmd_send_int(data, NULL, cmd_string, strlen(cmd_string), &ok_match, 1, true);
	if (ret < 0) {
		goto error;
	}

	snprintk(cmd_string, cmd_max_len,
		 "AT+KCNXCFG=1,\"GPRS\",\"%s\",,,\"" MODEM_HL78XX_ADDRESS_FAMILY "\"", apn);

	ret = modem_cmd_send_int(data, NULL, cmd_string, strlen(cmd_string), &ok_match, 1, true);
	if (ret < 0) {
		goto error;
	}

error:
	return ret;
}

int hl78xx_api_func_set_apn(const struct device *dev, const char *apn, uint16_t size)
{
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	if (apn == NULL || size > MDM_APN_MAX_LENGTH) {
		return -EINVAL;
	}

	strncpy(data->identity.apn, apn, sizeof(data->identity.apn));
	hl78xx_enter_state(data, MODEM_HL78XX_STATE_CARRIER_OFF);
	return 0;
}

int hl78xx_api_func_set_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality functionality,
					    bool reset)
{
	char cmd_string[sizeof(SET_FULLFUNCTIONAL_MODE_CMD) + sizeof(int)] = {0};
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	/* configure modem fully fuctinal without restart  */

	snprintf(cmd_string, sizeof(cmd_string), "AT+CFUN=%d,%d", functionality, reset);

	return modem_cmd_send_int(data, NULL, cmd_string, strlen(cmd_string), &ok_match, 1, true);
}

int hl78xx_api_func_get_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality *functionality)
{
	const char *cmd_string = GET_FULLFUNCTIONAL_MODE_CMD;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	/* configure modem fully fuctinal without restart  */
	return modem_cmd_send_int(data, NULL, cmd_string, strlen(cmd_string), &ok_match, 1, true);
}

int hl78xx_api_func_modem_cmd_send_int(const struct device *dev, const char *cmd, uint16_t cmd_size,
				       const struct modem_chat_match *response_matches,
				       uint16_t matches_size)
{

	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	return modem_cmd_send_int(data, NULL, cmd, cmd_size, response_matches, 1, true);
}
