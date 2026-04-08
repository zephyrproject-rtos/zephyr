/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/util.h>
#include <zephyr/net/wifi_mgmt.h>
#include <siwx91x_nwp.h>
#include "siwx91x_wifi.h"
#include "siwx91x_wifi_ps.h"

#include "sl_wifi_constants.h"

LOG_MODULE_DECLARE(siwx91x_wifi);

enum {
	REQUEST_TWT = 0,
	SUGGEST_TWT = 1,
	DEMAND_TWT = 2,
};


static int siwx91x_get_connected_ap_beacon_interval_ms(void)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_operational_statistics_t sl_stat;
	int ret;

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		return 0;
	}

	ret = sl_wifi_get_operational_statistics(SL_WIFI_CLIENT_INTERFACE, &sl_stat);
	if (ret) {
		return 0;
	}

	return sys_get_le16(sl_stat.beacon_interval) * 1024 / 1000;
}

int siwx91x_apply_power_save(struct siwx91x_dev *sidev)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_performance_profile_v2_t sl_ps_profile;
	int beacon_interval;
	int ret;

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		LOG_ERR("Wi-Fi not in station mode");
		return -EINVAL;
	}

	if (sidev->state == WIFI_STATE_INTERFACE_DISABLED) {
		LOG_ERR("Command given in invalid state");
		return -EINVAL;
	}

	sl_wifi_get_performance_profile_v2(&sl_ps_profile);

	if (sidev->ps_params.enabled == WIFI_PS_DISABLED) {
		sl_ps_profile.profile = HIGH_PERFORMANCE;
		goto out;
	}
	if (sidev->ps_params.exit_strategy == WIFI_PS_EXIT_EVERY_TIM) {
		sl_ps_profile.profile = ASSOCIATED_POWER_SAVE_LOW_LATENCY;
	} else if (sidev->ps_params.exit_strategy == WIFI_PS_EXIT_CUSTOM_ALGO) {
		sl_ps_profile.profile = ASSOCIATED_POWER_SAVE;
	} else {
		/* Already sanitized by siwx91x_set_power_save() */
		return -EINVAL;
	}

	sl_ps_profile.monitor_interval = sidev->ps_params.timeout_ms;

	beacon_interval = siwx91x_get_connected_ap_beacon_interval_ms();
	/* 1000ms is arbitrary sane value */
	sl_ps_profile.listen_interval = MIN(beacon_interval * sidev->ps_params.listen_interval,
					    1000);

	if (sidev->ps_params.wakeup_mode == WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL &&
	    !sidev->ps_params.listen_interval) {
		LOG_INF("Disabling listen interval based wakeup until connection establishes");
	}
	if (sidev->ps_params.wakeup_mode == WIFI_PS_WAKEUP_MODE_DTIM ||
	    !sidev->ps_params.listen_interval) {
		sl_ps_profile.dtim_aligned_type = 1;
	} else if (sidev->ps_params.wakeup_mode == WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL) {
		sl_ps_profile.dtim_aligned_type = 0;
	} else {
		/* Already sanitized by siwx91x_set_power_save() */
		return -EINVAL;
	}

out:
	ret = sl_wifi_set_performance_profile_v2(&sl_ps_profile);
	return ret ? -EIO : 0;
}

int siwx91x_set_power_save(const struct device *dev, struct wifi_ps_params *params)
{
	struct siwx91x_dev *sidev = dev->data;
	int ret;

	__ASSERT(params, "params cannot be NULL");

	switch (params->type) {
	case WIFI_PS_PARAM_STATE:
		sidev->ps_params.enabled = params->enabled;
		break;
	case WIFI_PS_PARAM_MODE:
		if (params->mode != WIFI_PS_MODE_LEGACY) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
			return -ENOTSUP;
		}
		break;
	case WIFI_PS_PARAM_LISTEN_INTERVAL:
		sidev->ps_params.listen_interval = params->listen_interval;
		break;
	case WIFI_PS_PARAM_WAKEUP_MODE:
		if (params->wakeup_mode != WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL &&
		    params->wakeup_mode != WIFI_PS_WAKEUP_MODE_DTIM) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
			return -ENOTSUP;
		}
		sidev->ps_params.wakeup_mode = params->wakeup_mode;
		break;
	case WIFI_PS_PARAM_TIMEOUT:
		/* 50ms and 1000ms is arbitrary sane values */
		if (params->timeout_ms < 50 ||
		    params->timeout_ms > 1000) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
			return -EINVAL;
		}
		sidev->ps_params.timeout_ms = params->timeout_ms;
		break;
	case WIFI_PS_PARAM_EXIT_STRATEGY:
		if (params->exit_strategy != WIFI_PS_EXIT_EVERY_TIM &&
		    params->exit_strategy != WIFI_PS_EXIT_CUSTOM_ALGO) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
			return -ENOTSUP;
		}
		sidev->ps_params.exit_strategy = params->exit_strategy;
		break;
	default:
		params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return -EINVAL;
	}
	ret = siwx91x_apply_power_save(sidev);
	if (ret) {
		params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return ret;
	}
	return 0;
}

int siwx91x_get_power_save_config(const struct device *dev, struct wifi_ps_config *config)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_performance_profile_v2_t sl_ps_profile;
	struct siwx91x_dev *sidev = dev->data;
	uint16_t beacon_interval;
	int ret;

	__ASSERT(config, "config cannot be NULL");

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		LOG_ERR("Wi-Fi not in station mode");
		return -EINVAL;
	}

	if (sidev->state == WIFI_STATE_INTERFACE_DISABLED) {
		LOG_ERR("Command given in invalid state");
		return -EINVAL;
	}

	ret = sl_wifi_get_performance_profile_v2(&sl_ps_profile);
	if (ret) {
		LOG_ERR("Failed to get power save profile: 0x%x", ret);
		return -EIO;
	}

	switch (sl_ps_profile.profile) {
	case HIGH_PERFORMANCE:
		config->ps_params.enabled = WIFI_PS_DISABLED;
		break;
	case ASSOCIATED_POWER_SAVE_LOW_LATENCY:
		config->ps_params.enabled = WIFI_PS_ENABLED;
		config->ps_params.exit_strategy = WIFI_PS_EXIT_EVERY_TIM;
		break;
	case ASSOCIATED_POWER_SAVE:
		config->ps_params.enabled = WIFI_PS_ENABLED;
		config->ps_params.exit_strategy = WIFI_PS_EXIT_CUSTOM_ALGO;
		break;
	default:
		break;
	}

	if (sl_ps_profile.dtim_aligned_type) {
		config->ps_params.wakeup_mode = WIFI_PS_WAKEUP_MODE_DTIM;
	} else {
		config->ps_params.wakeup_mode = WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL;

		beacon_interval = siwx91x_get_connected_ap_beacon_interval_ms();
		if (beacon_interval > 0) {
			config->ps_params.listen_interval =
				DIV_ROUND_UP(sl_ps_profile.listen_interval, beacon_interval);
		}
	}

	/* Device supports only legacy power-save mode */
	config->ps_params.mode = WIFI_PS_MODE_LEGACY;
	config->ps_params.timeout_ms = sl_ps_profile.monitor_interval;

	return 0;
}

static int siwx91x_convert_z_sl_twt_req_type(enum wifi_twt_setup_cmd z_req_cmd)
{
	switch (z_req_cmd) {
	case WIFI_TWT_SETUP_CMD_REQUEST:
		return REQUEST_TWT;
	case WIFI_TWT_SETUP_CMD_SUGGEST:
		return SUGGEST_TWT;
	case WIFI_TWT_SETUP_CMD_DEMAND:
		return DEMAND_TWT;
	default:
		return -EINVAL;
	}
}

static int siwx91x_set_twt_setup(struct wifi_twt_params *params)
{
	int twt_req_type = siwx91x_convert_z_sl_twt_req_type(params->setup_cmd);
	sl_wifi_twt_request_t twt_req = {
		.twt_retry_interval = 5,
		.wake_duration_unit = 0,
		.wake_int_mantissa = params->setup.twt_mantissa,
		.un_announced_twt = !params->setup.announce,
		.wake_duration = params->setup.twt_wake_interval,
		.triggered_twt = params->setup.trigger,
		.wake_int_exp = params->setup.twt_exponent,
		.implicit_twt = 1,
		.twt_flow_id = params->flow_id,
		.twt_enable = 1,
		.req_type = twt_req_type,
	};
	int ret;

	if (twt_req_type < 0) {
		params->fail_reason = WIFI_TWT_FAIL_CMD_EXEC_FAIL;
		return -EINVAL;
	}

	if (!params->setup.twt_info_disable) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (params->setup.responder) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	/* implicit -> won't do renegotiation
	 * explicit -> must do renegotiation for each session
	 */
	if (!params->setup.implicit) {
		/* explicit twt is not supported */
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (params->setup.twt_wake_interval > 255 * 256) {
		twt_req.wake_duration_unit = 1;
		twt_req.wake_duration = params->setup.twt_wake_interval / 1024;
	} else {
		twt_req.wake_duration_unit = 0;
		twt_req.wake_duration = params->setup.twt_wake_interval / 256;
	}

	ret = sl_wifi_enable_target_wake_time(&twt_req);
	if (ret) {
		params->fail_reason = WIFI_TWT_FAIL_CMD_EXEC_FAIL;
		params->resp_status = WIFI_TWT_RESP_NOT_RECEIVED;
		return -EINVAL;
	}

	return 0;
}

static int siwx91x_set_twt_teardown(struct wifi_twt_params *params)
{
	sl_wifi_twt_request_t twt_req = { };
	int ret;

	twt_req.twt_enable = 0;

	if (params->teardown.teardown_all) {
		twt_req.twt_flow_id = 0xFF;
	} else {
		twt_req.twt_flow_id = params->flow_id;
	}

	ret = sl_wifi_disable_target_wake_time(&twt_req);
	if (ret) {
		params->fail_reason = WIFI_TWT_FAIL_CMD_EXEC_FAIL;
		params->teardown_status = WIFI_TWT_TEARDOWN_FAILED;
		return -EINVAL;
	}

	params->teardown_status = WIFI_TWT_TEARDOWN_SUCCESS;

	return 0;
}

int siwx91x_set_twt(const struct device *dev, struct wifi_twt_params *params)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	struct siwx91x_dev *sidev = dev->data;

	__ASSERT(params, "params cannot be a NULL");

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (sidev->state != WIFI_STATE_DISCONNECTED && sidev->state != WIFI_STATE_INACTIVE &&
	    sidev->state != WIFI_STATE_COMPLETED) {
		LOG_ERR("Command given in invalid state");
		return -EBUSY;
	}

	if (params->negotiation_type != WIFI_TWT_INDIVIDUAL) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (params->operation == WIFI_TWT_SETUP) {
		return siwx91x_set_twt_setup(params);
	} else if (params->operation == WIFI_TWT_TEARDOWN) {
		return siwx91x_set_twt_teardown(params);
	}
	params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
	return -ENOTSUP;
}

sl_status_t siwx91x_on_twt(sl_wifi_event_t twt_event, sl_status_t status_code, void *data,
			   uint32_t data_length, void *arg)
{
	uint32_t ev = (uint32_t)twt_event & ~(uint32_t)SL_WIFI_EVENT_FAIL_INDICATION;
	bool ev_failed =
		(uint32_t)twt_event & (uint32_t)SL_WIFI_EVENT_FAIL_INDICATION;

	struct siwx91x_dev *sidev = arg;
	struct wifi_twt_params params = { };
	sl_wifi_twt_response_t *resp = NULL;

	/*
	 * Many TWT events pass sl_wifi_twt_response_t; some use NULL data.
	 * Non-NULL with a short buffer must not be cast: log and exit instead of silent ignore.
	 */
	if (data != NULL) {
		if (data_length < sizeof(sl_wifi_twt_response_t)) {
			LOG_WRN("TWT event payload length %u < %zu (sl_wifi_twt_response_t)",
				(unsigned int)data_length, sizeof(sl_wifi_twt_response_t));
			return SL_STATUS_OK;
		}
		resp = (sl_wifi_twt_response_t *)data;
	}

	if (ev == SL_WIFI_TWT_EVENTS_END) {
		LOG_DBG("TWT events end marker from NWP");
		return SL_STATUS_OK;
	}

	if (ev == SL_WIFI_TWT_RESPONSE_EVENT && resp == NULL) {
		LOG_DBG("Generic TWT response event without payload, ignored");
		return SL_STATUS_OK;
	}

	LOG_DBG("TWT twt_event=0x%08x ev=0x%08x status=0x%x", (unsigned int)twt_event, ev,
		(unsigned int)status_code);

	/*
	 * WiseConnect may set SL_WIFI_EVENT_FAIL_INDICATION or a non-OK status_code on
	 * some callbacks; for the events below we still follow the success path in the
	 * switch. Other events with indicated failure map to NOT_RECEIVED here.
	 */
	bool indicated_fail = ev_failed || (status_code != SL_STATUS_OK);
	bool success_class =
		(ev == SL_WIFI_TWT_RESPONSE_EVENT) ||
		(ev == SL_WIFI_TWT_UNSOLICITED_SESSION_SUCCESS_EVENT) ||
		(ev == SL_WIFI_RESCHEDULE_TWT_SUCCESS_EVENT) ||
		(ev == SL_WIFI_TWT_TEARDOWN_SUCCESS_EVENT) ||
		(ev == SL_WIFI_TWT_AP_TEARDOWN_SUCCESS_EVENT);

	if (indicated_fail && !success_class) {
		params.negotiation_type = WIFI_TWT_INDIVIDUAL;
		params.operation = WIFI_TWT_SETUP;
		params.resp_status = WIFI_TWT_RESP_NOT_RECEIVED;
		params.fail_reason = WIFI_TWT_FAIL_CMD_EXEC_FAIL;
		if (resp != NULL) {
			params.flow_id = resp->twt_flow_id;
		}
		wifi_mgmt_raise_twt_event(sidev->iface, &params);
		return SL_STATUS_OK;
	}

	params.negotiation_type = WIFI_TWT_INDIVIDUAL;

	switch (ev) {
	case SL_WIFI_TWT_UNSOLICITED_SESSION_SUCCESS_EVENT:
	case SL_WIFI_TWT_RESPONSE_EVENT:
	case SL_WIFI_RESCHEDULE_TWT_SUCCESS_EVENT:
		params.operation = WIFI_TWT_SETUP;
		params.resp_status = WIFI_TWT_RESP_RECEIVED;
		params.setup_cmd = WIFI_TWT_SETUP_CMD_ACCEPT;
		break;
	case SL_WIFI_TWT_AP_REJECTED_EVENT:
	case SL_WIFI_TWT_UNSUPPORTED_RESPONSE_EVENT:
	case SL_WIFI_TWT_FAIL_MAX_RETRIES_REACHED_EVENT:
		params.operation = WIFI_TWT_SETUP;
		params.resp_status = WIFI_TWT_RESP_RECEIVED;
		params.setup_cmd = WIFI_TWT_SETUP_CMD_REJECT;
		params.fail_reason = WIFI_TWT_FAIL_CMD_EXEC_FAIL;
		break;
	case SL_WIFI_TWT_OUT_OF_TOLERANCE_EVENT:
	case SL_WIFI_TWT_RESPONSE_NOT_MATCHED_EVENT:
	case SL_WIFI_TWT_INFO_FRAME_EXCHANGE_FAILED_EVENT:
		params.operation = WIFI_TWT_SETUP;
		params.resp_status = WIFI_TWT_RESP_RECEIVED;
		params.setup_cmd = WIFI_TWT_SETUP_CMD_ALTERNATE;
		params.fail_reason = WIFI_TWT_FAIL_CMD_EXEC_FAIL;
		break;
	case SL_WIFI_TWT_TEARDOWN_SUCCESS_EVENT:
	case SL_WIFI_TWT_AP_TEARDOWN_SUCCESS_EVENT:
	case SL_WIFI_TWT_INACTIVE_DUE_TO_ROAMING_EVENT:
	case SL_WIFI_TWT_INACTIVE_DUE_TO_DISCONNECT_EVENT:
	case SL_WIFI_TWT_INACTIVE_NO_AP_SUPPORT_EVENT:
		params.operation = WIFI_TWT_TEARDOWN;
		params.teardown_status = WIFI_TWT_TEARDOWN_SUCCESS;
		break;
	default:
		LOG_WRN("Unknown Wi-Fi TWT event: 0x%08x", ev);
		return SL_STATUS_OK;
	}

	if (params.operation == WIFI_TWT_SETUP && resp != NULL) {
		params.flow_id = resp->twt_flow_id;
		params.setup.twt_exponent = resp->wake_int_exp;
		params.setup.twt_mantissa = resp->wake_int_mantissa;
		params.setup.twt_interval = (uint64_t)resp->wake_int_mantissa << resp->wake_int_exp;
		params.setup.implicit = resp->implicit_twt;
		params.setup.trigger = resp->triggered_twt;
		params.setup.announce = !resp->un_announced_twt;
		params.setup.twt_wake_interval = resp->wake_duration_unit != 0
			? (uint32_t)resp->wake_duration * 1024
			: (uint32_t)resp->wake_duration * 256;
		params.setup.twt_info_disable = true;
	}

	if (params.operation == WIFI_TWT_TEARDOWN && resp != NULL) {
		params.flow_id = resp->twt_flow_id;
	}

	wifi_mgmt_raise_twt_event(sidev->iface, &params);
	return SL_STATUS_OK;
}
