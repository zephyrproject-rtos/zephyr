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

LOG_MODULE_REGISTER(hl78xx_apis, CONFIG_MODEM_LOG_LEVEL);

/* Wrapper to centralize modem_dynamic_cmd_send calls and reduce repetition.
 * returns negative errno on failure or the value returned by modem_dynamic_cmd_send.
 */
static int hl78xx_send_cmd(struct hl78xx_data *data, const char *cmd,
			   void (*chat_cb)(struct modem_chat *, enum modem_chat_script_result,
					   void *),
			   const struct modem_chat_match *matches, uint16_t match_count)
{
	if (data == NULL || cmd == NULL) {
		return -EINVAL;
	}
	return modem_dynamic_cmd_send(data, chat_cb, cmd, (uint16_t)strlen(cmd), matches,
				      match_count, true);
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
		ret = hl78xx_parse_rssi(data->status.rssi, value);
		break;

	case CELLULAR_SIGNAL_RSRP:
		ret = hl78xx_parse_rsrp(data->status.rsrp, value);
		break;

	case CELLULAR_SIGNAL_RSRQ:
		ret = hl78xx_parse_rsrq(data->status.rsrq, value);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
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
		return -ENODATA;
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

int hl78xx_api_func_get_modem_info_vendor(const struct device *dev,
					  enum hl78xx_modem_info_type type, void *info, size_t size)
{
	int ret = 0;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	const char *network_operator = "AT+COPS?";

	if (info == NULL || size == 0) {
		return -EINVAL;
	}
	/* copy identity under api lock to a local buffer then write to caller
	 * prevents holding lock during the return/caller access
	 */
	k_mutex_lock(&data->api_lock, K_FOREVER);
	switch (type) {
	case HL78XX_MODEM_INFO_APN:
		if (data->status.apn.state != APN_STATE_CONFIGURED) {
			ret = -ENODATA;
			break;
		}
		safe_strncpy(info, (const char *)data->identity.apn, size);
		break;

	case HL78XX_MODEM_INFO_CURRENT_RAT:
		*(enum hl78xx_cell_rat_mode *)info = data->status.registration.rat_mode;
		break;

	case HL78XX_MODEM_INFO_NETWORK_OPERATOR:
		/* Network operator not currently tracked; return empty or implement tracking */
		ret = hl78xx_send_cmd(data, network_operator, NULL, hl78xx_get_allow_match(),
				      hl78xx_get_allow_match_size());
		if (ret < 0) {
			LOG_ERR("Failed to get network operator");
		}
		safe_strncpy(info, (const char *)data->status.network_operator.operator,
			     MIN(size, sizeof(data->status.network_operator.operator)));
		break;

	default:
		break;
	}
	k_mutex_unlock(&data->api_lock);
	return ret;
}

int hl78xx_api_func_get_modem_info_standard(const struct device *dev,
					    enum cellular_modem_info_type type, char *info,
					    size_t size)
{
	int ret = 0;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	if (info == NULL || size == 0) {
		return -EINVAL;
	}
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

int hl78xx_api_func_set_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality functionality,
					    bool reset)
{
	char cmd_string[sizeof(SET_FULLFUNCTIONAL_MODE_CMD) + sizeof(int)] = {0};
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	/* configure modem functionality with/without restart  */
	snprintf(cmd_string, sizeof(cmd_string), "AT+CFUN=%d,%d", functionality, reset);
	return hl78xx_send_cmd(data, cmd_string, NULL, hl78xx_get_ok_match(), 1);
}

int hl78xx_api_func_get_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality *functionality)
{
	const char *cmd_string = GET_FULLFUNCTIONAL_MODE_CMD;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	/* get modem phone functionality */
	return hl78xx_send_cmd(data, cmd_string, NULL, hl78xx_get_ok_match(), 1);
}

int hl78xx_api_func_modem_dynamic_cmd_send(const struct device *dev, const char *cmd,
					   uint16_t cmd_size,
					   const struct modem_chat_match *response_matches,
					   uint16_t matches_size)
{
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	if (cmd == NULL) {
		return -EINVAL;
	}
	/* respect provided matches_size and serialize modem access */
	return modem_dynamic_cmd_send(data, NULL, cmd, cmd_size, response_matches, matches_size,
				      true);
}
