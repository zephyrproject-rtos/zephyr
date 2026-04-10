/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/sys/util.h>
#include "siwx91x_nwp_api.h"
#include "siwx91x_wifi.h"
#include "siwx91x_wifi_ps.h"

LOG_MODULE_DECLARE(siwx91x_wifi, CONFIG_WIFI_LOG_LEVEL);

static int siwx91x_wifi_get_beacon_interval(const struct device *dev)
{
	const struct siwx91x_wifi_config *config = dev->config;
	sl_wifi_operational_statistics_t sl_stat;

	siwx91x_nwp_get_bss_info(config->nwp_dev, &sl_stat);
	return sys_get_le16(sl_stat.beacon_interval) * 1024 / 1000;
}

/* This function impact Wifi and Bluetooth. We can enable PS only if bluetooth and Wifi are
 * configure to go to PS . However, all the parameters are set by the Wifi.
 * FIXME: Relocate the PS params in nwpdriver. Implement the PM_ops in the nwp and call this API
 * only from the nwp when suspend is requested.
 */
#define SIWX91X_WIFI_MAX_PSP   0
#define SIWX91X_WIFI_FAST_PSP  1
static int siwx91x_wifi_apply_power_save(const struct device *dev)
{
	const struct siwx91x_wifi_config *config = dev->config;
	struct siwx91x_wifi_data *data = dev->data;
	sli_wifi_power_save_request_t params = {
		.power_mode = 1,
		.ulp_mode_enable = 1,
		.beacon_miss_ignore_limit = 1,
		.monitor_interval = data->ps_timeout_ms,
	};
	int beacon_interval = siwx91x_wifi_get_beacon_interval(dev);

	 /* FIXME: In fact, we should enable PS only if WiFi AND Bluetooth request PS */
	if (!data->ps_enabled) {
		siwx91x_nwp_ps_disable(config->nwp_dev);
		return 0;
	}

	if (data->ps_exit_strategy == WIFI_PS_EXIT_EVERY_TIM) {
		params.psp_type = SIWX91X_WIFI_FAST_PSP;
	} else {
		params.psp_type = SIWX91X_WIFI_MAX_PSP;
	}

	if (data->ps_wakeup_mode == WIFI_PS_WAKEUP_MODE_DTIM || !data->ps_listen_interval) {
		params.dtim_aligned_type = 1;
	} else if (data->ps_wakeup_mode == WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL) {
		params.dtim_aligned_type = 0;
	} else {
		/* Already sanitized by siwx91x_wifi_set_power_save() */
		return -EINVAL;
	}

	/* 1000ms is arbitrary sane value */
	params.listen_interval = MIN(beacon_interval * data->ps_listen_interval, 1000);

	siwx91x_nwp_ps_enable(config->nwp_dev, &params);
	return 0;
}

int siwx91x_wifi_set_power_save(const struct device *dev, struct wifi_ps_params *params)
{
	struct siwx91x_wifi_data *data = dev->data;
	int ret;

	__ASSERT(params, "params cannot be NULL");

	switch (params->type) {
	case WIFI_PS_PARAM_STATE:
		data->ps_enabled = params->enabled;
		break;
	case WIFI_PS_PARAM_MODE:
		if (params->mode != WIFI_PS_MODE_LEGACY) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
			return -ENOTSUP;
		}
		break;
	case WIFI_PS_PARAM_LISTEN_INTERVAL:
		data->ps_listen_interval = params->listen_interval;
		break;
	case WIFI_PS_PARAM_WAKEUP_MODE:
		if (params->wakeup_mode != WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL &&
		    params->wakeup_mode != WIFI_PS_WAKEUP_MODE_DTIM) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
			return -ENOTSUP;
		}
		data->ps_wakeup_mode = params->wakeup_mode;
		break;
	case WIFI_PS_PARAM_TIMEOUT:
		/* 50ms and 1000ms is arbitrary sane values */
		if (params->timeout_ms < 50 ||
		    params->timeout_ms > 1000) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
			return -EINVAL;
		}
		data->ps_timeout_ms = params->timeout_ms;
		break;
	case WIFI_PS_PARAM_EXIT_STRATEGY:
		if (params->exit_strategy != WIFI_PS_EXIT_EVERY_TIM &&
		    params->exit_strategy != WIFI_PS_EXIT_CUSTOM_ALGO) {
			params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
			return -ENOTSUP;
		}
		data->ps_exit_strategy = params->exit_strategy;
		break;
	default:
		params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return -EINVAL;
	}
	ret = siwx91x_wifi_apply_power_save(dev);
	if (ret) {
		params->fail_reason = WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return ret;
	}
	return 0;
}

int siwx91x_wifi_get_power_save_config(const struct device *dev, struct wifi_ps_config *config)
{
	struct siwx91x_wifi_data *data = dev->data;

	config->ps_params.enabled = data->ps_enabled;
	config->ps_params.exit_strategy = data->ps_exit_strategy;
	config->ps_params.wakeup_mode = data->ps_wakeup_mode;
	config->ps_params.listen_interval = data->ps_listen_interval;
	config->ps_params.mode = WIFI_PS_MODE_LEGACY;
	config->ps_params.timeout_ms = data->ps_timeout_ms;
	return 0;
}

static int siwx91x_wifi_set_twt_setup(const struct device *dev, struct wifi_twt_params *params)
{
	const struct siwx91x_wifi_config *config = dev->config;
	sl_wifi_twt_request_t req = {
		.twt_enable = 1,
		.twt_retry_interval = 5,
		.wake_int_mantissa = params->setup.twt_mantissa,
		.un_announced_twt = !params->setup.announce,
		.triggered_twt = params->setup.trigger,
		.wake_int_exp = params->setup.twt_exponent,
		.implicit_twt = 1,
		.twt_flow_id = params->flow_id,
	};

	if (!params->setup.twt_info_disable) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (params->setup.responder) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	if (!params->setup.implicit) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	switch (params->setup_cmd) {
	case WIFI_TWT_SETUP_CMD_REQUEST:
		req.req_type = 0;
	case WIFI_TWT_SETUP_CMD_SUGGEST:
		req.req_type = 1;
	case WIFI_TWT_SETUP_CMD_DEMAND:
		req.req_type = 2;
	default:
		params->fail_reason = WIFI_TWT_FAIL_CMD_EXEC_FAIL;
		return -EINVAL;
	}

	if (params->setup.twt_wake_interval / 256 <= UINT8_MAX) {
		req.wake_duration_unit = 0;
		req.wake_duration = params->setup.twt_wake_interval / 256;
	} else {
		req.wake_duration_unit = 1;
		req.wake_duration = params->setup.twt_wake_interval / 1024;
	}

	siwx91x_nwp_twt_params(config->nwp_dev, &req);
	params->resp_status = WIFI_TWT_RESP_NOT_RECEIVED;
	return 0;
}

static int siwx91x_wifi_set_twt_teardown(const struct device *dev, struct wifi_twt_params *params)
{
	const struct siwx91x_wifi_config *config = dev->config;
	sl_wifi_twt_request_t req = {
		.twt_enable = 0,
		.twt_flow_id = params->teardown.teardown_all ? 0xFF : params->flow_id,
	};

	/* FIXME: Probably we need to check the status of the command */
	siwx91x_nwp_twt_params(config->nwp_dev, &req);
	params->resp_status = WIFI_TWT_RESP_RECEIVED;
	return 0;
}

int siwx91x_wifi_set_twt(const struct device *dev, struct wifi_twt_params *params)
{
	__ASSERT(params, "params cannot be a NULL");

	if (params->negotiation_type != WIFI_TWT_INDIVIDUAL) {
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	switch (params->operation) {
	case WIFI_TWT_SETUP:
		return siwx91x_wifi_set_twt_setup(dev, params);
	case WIFI_TWT_TEARDOWN:
		return siwx91x_wifi_set_twt_teardown(dev, params);
	default:
		params->fail_reason = WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}
}
