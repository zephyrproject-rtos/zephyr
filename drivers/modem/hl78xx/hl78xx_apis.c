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
				      match_count, MDM_CMD_TIMEOUT, true);
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
	case HL78XX_MODEM_INFO_SERIAL_NUMBER:
		safe_strncpy(info, (const char *)data->identity.serial_number,
			     MIN(size, sizeof(data->identity.serial_number)));
		break;
	case HL78XX_MODEM_INFO_CURRENT_BAUD_RATE:
		*(uint32_t *)info = data->status.uart.current_baudrate;
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
	int ret = 0;

	/* configure modem functionality with/without restart  */
	snprintf(cmd_string, sizeof(cmd_string), "AT+CFUN=%d,%d", functionality, reset);
	ret = hl78xx_send_cmd(data, cmd_string, NULL, hl78xx_get_ok_match(),
			      hl78xx_get_ok_match_size());
	if (ret == 0) {
		data->status.phone_functionality.in_progress = true;
	}

	return ret;
}

int hl78xx_api_func_get_phone_functionality(const struct device *dev,
					    enum hl78xx_phone_functionality *functionality)
{
	const char *cmd_string = GET_FULLFUNCTIONAL_MODE_CMD;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;
	/* get modem phone functionality */
	return hl78xx_send_cmd(data, cmd_string, NULL, hl78xx_get_ok_match(),
			       hl78xx_get_ok_match_size());
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
				      MDM_CMD_TIMEOUT, true);
}
#ifdef CONFIG_MODEM_HL78XX_AIRVANTAGE
int hl78xx_start_airvantage_dm_session(const struct device *dev)
{
	int ret = 0;
	struct hl78xx_data *data = (struct hl78xx_data *)dev->data;

	ret = modem_dynamic_cmd_send(data, NULL, WDSI_USER_INITIATED_CONNECTION_START_CMD,
				     strlen(WDSI_USER_INITIATED_CONNECTION_START_CMD),
				     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				     MDM_CMD_TIMEOUT, false);
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

	ret = modem_dynamic_cmd_send(data, NULL, WDSI_USER_INITIATED_CONNECTION_STOP_CMD,
				     strlen(WDSI_USER_INITIATED_CONNECTION_STOP_CMD),
				     hl78xx_get_ok_match(), hl78xx_get_ok_match_size(),
				     MDM_CMD_TIMEOUT, false);
	if (ret < 0) {
		LOG_ERR("Stop DM session error %d", ret);
		return ret;
	}
	return 0;
}
#endif /* CONFIG_MODEM_HL78XX_AIRVANTAGE */

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

		if (modem_data->gnss_dev == NULL || modem_data->gnss_dev->data == NULL) {
			return -EINVAL;
		}

		/* gnss_dev->data is hl78xx_gnss_data */
		*gnss_data_out = (struct hl78xx_gnss_data *)modem_data->gnss_dev->data;

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
