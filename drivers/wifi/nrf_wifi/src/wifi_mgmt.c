/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing WiFi management operation implementations
 * for the Zephyr OS.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "util.h"
#include "fmac_api.h"
#include "fmac_tx.h"
#include "fmac_util.h"
#include "fmac_main.h"
#include "wifi_mgmt.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF70_LOG_LEVEL);

extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;

int nrf_wifi_set_power_save(const struct device *dev,
			    struct wifi_ps_params *params)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	int ret = -1;
	unsigned int uapsd_queue = UAPSD_Q_MIN; /* Legacy mode */

	if (!dev || !params) {
		LOG_ERR("%s: dev or params is NULL", __func__);
		return ret;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return ret;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		return ret;
	}

	k_mutex_lock(&vif_ctx_zep->vif_lock, K_FOREVER);
	if (!rpu_ctx_zep->rpu_ctx) {
		LOG_DBG("%s: RPU context not initialized", __func__);
		goto out;
	}

	switch (params->type) {
	case WIFI_PS_PARAM_LISTEN_INTERVAL:
		if ((params->listen_interval <
		     NRF_WIFI_LISTEN_INTERVAL_MIN) ||
		    (params->listen_interval >
		     WIFI_LISTEN_INTERVAL_MAX)) {
			params->fail_reason =
				WIFI_PS_PARAM_LISTEN_INTERVAL_RANGE_INVALID;
			return -EINVAL;
		}
		status = nrf_wifi_fmac_set_listen_interval(
						rpu_ctx_zep->rpu_ctx,
						vif_ctx_zep->vif_idx,
						params->listen_interval);
	break;
	case  WIFI_PS_PARAM_TIMEOUT:
		if ((vif_ctx_zep->if_type != NRF_WIFI_IFTYPE_STATION)
#ifdef CONFIG_NRF70_RAW_DATA_TX
		    && (vif_ctx_zep->if_type != NRF_WIFI_STA_TX_INJECTOR)
#endif /* CONFIG_NRF70_RAW_DATA_TX */
#ifdef CONFIG_NRF70_PROMISC_DATA_RX
		    && (vif_ctx_zep->if_type != NRF_WIFI_STA_PROMISC_TX_INJECTOR)
#endif /* CONFIG_NRF70_PROMISC_DATA_RX */
		   ) {
			LOG_ERR("%s: Operation supported only in STA enabled mode",
				__func__);
			params->fail_reason =
				WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
			goto out;
		}

		status = nrf_wifi_fmac_set_power_save_timeout(
							rpu_ctx_zep->rpu_ctx,
							vif_ctx_zep->vif_idx,
							params->timeout_ms);
	break;
	case WIFI_PS_PARAM_MODE:
		if (params->mode == WIFI_PS_MODE_WMM) {
			uapsd_queue = UAPSD_Q_MAX; /* WMM mode */
		}

		status = nrf_wifi_fmac_set_uapsd_queue(rpu_ctx_zep->rpu_ctx,
						       vif_ctx_zep->vif_idx,
						       uapsd_queue);
	break;
	case  WIFI_PS_PARAM_STATE:
		status = nrf_wifi_fmac_set_power_save(rpu_ctx_zep->rpu_ctx,
						      vif_ctx_zep->vif_idx,
						      params->enabled);
	break;
	case WIFI_PS_PARAM_WAKEUP_MODE:
		status = nrf_wifi_fmac_set_ps_wakeup_mode(
							rpu_ctx_zep->rpu_ctx,
							vif_ctx_zep->vif_idx,
							params->wakeup_mode);
	break;
	case WIFI_PS_PARAM_EXIT_STRATEGY:
		unsigned int exit_strategy;

		if (params->exit_strategy == WIFI_PS_EXIT_EVERY_TIM) {
			exit_strategy = EVERY_TIM;
		} else if (params->exit_strategy == WIFI_PS_EXIT_CUSTOM_ALGO) {
			exit_strategy = INT_PS;
		} else {
			params->fail_reason =
				WIFI_PS_PARAM_FAIL_INVALID_EXIT_STRATEGY;
			return -EINVAL;
		}

		status = nrf_wifi_fmac_set_ps_exit_strategy(
							rpu_ctx_zep->rpu_ctx,
							vif_ctx_zep->vif_idx,
							exit_strategy);
	break;
	default:
		params->fail_reason =
			WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		return -ENOTSUP;
	}

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Confiuring PS param %d failed",
			__func__, params->type);
		params->fail_reason =
			WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL;
		goto out;
	}


	ret = 0;
out:
	k_mutex_unlock(&vif_ctx_zep->vif_lock);
	return ret;
}

int nrf_wifi_get_power_save_config(const struct device *dev,
				   struct wifi_ps_config *ps_config)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	int ret = -1;
	int count = 0;

	if (!dev || !ps_config) {
		return ret;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return ret;
	}

	if ((vif_ctx_zep->if_type != NRF_WIFI_IFTYPE_STATION)
#ifdef CONFIG_NRF70_RAW_DATA_TX
	    && (vif_ctx_zep->if_type != NRF_WIFI_STA_TX_INJECTOR)
#endif /* CONFIG_NRF70_RAW_DATA_TX */
#ifdef CONFIG_NRF70_PROMISC_DATA_RX
	    && (vif_ctx_zep->if_type != NRF_WIFI_STA_PROMISC_TX_INJECTOR)
#endif /* CONFIG_NRF70_PROMISC_DATA_RX */
	    ) {
		LOG_ERR("%s: Operation supported only in STA enabled mode",
			__func__);
		return ret;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		return ret;
	}

	k_mutex_lock(&vif_ctx_zep->vif_lock, K_FOREVER);
	if (!rpu_ctx_zep->rpu_ctx) {
		LOG_DBG("%s: RPU context not initialized", __func__);
		goto out;
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		goto out;
	}

	vif_ctx_zep->ps_info = ps_config;

	vif_ctx_zep->ps_config_info_evnt = false;

	status = nrf_wifi_fmac_get_power_save_info(rpu_ctx_zep->rpu_ctx,
						   vif_ctx_zep->vif_idx);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_get_power_save_info failed",
			__func__);
		goto out;
	}

	do {
		nrf_wifi_osal_sleep_ms(1);
		 count++;
	} while ((vif_ctx_zep->ps_config_info_evnt == false) &&
		 (count < NRF_WIFI_FMAC_PS_CONF_EVNT_RECV_TIMEOUT));

	if (count == NRF_WIFI_FMAC_PS_CONF_EVNT_RECV_TIMEOUT) {
		nrf_wifi_osal_log_err("%s: Timed out",
				      __func__);
		goto out;
	}

	ret = 0;
out:
	k_mutex_unlock(&vif_ctx_zep->vif_lock);
	return ret;
}

/* TWT interval conversion helpers: User <-> Protocol */
static struct twt_interval_float nrf_wifi_twt_us_to_float(uint32_t twt_interval)
{
	double mantissa = 0.0;
	int exponent = 0;
	struct twt_interval_float twt_interval_fp;

	double twt_interval_ms = twt_interval / 1000.0;

	mantissa = frexp(twt_interval_ms, &exponent);
	/* Ceiling and conversion to milli seconds */
	twt_interval_fp.mantissa = ceil(mantissa * 1000);
	twt_interval_fp.exponent = exponent;

	return twt_interval_fp;
}

static uint64_t nrf_wifi_twt_float_to_us(struct twt_interval_float twt_interval_fp)
{
	/* Conversion to micro-seconds */
	return floor(ldexp(twt_interval_fp.mantissa, twt_interval_fp.exponent) / (1000)) *
			     1000;
}

static unsigned char twt_wifi_mgmt_to_rpu_neg_type(enum wifi_twt_negotiation_type neg_type)
{
	unsigned char rpu_neg_type = 0;

	switch (neg_type) {
	case WIFI_TWT_INDIVIDUAL:
		rpu_neg_type = NRF_WIFI_TWT_NEGOTIATION_TYPE_INDIVIDUAL;
		break;
	case WIFI_TWT_BROADCAST:
		rpu_neg_type = NRF_WIFI_TWT_NEGOTIATION_TYPE_BROADCAST;
		break;
	default:
		LOG_ERR("%s: Invalid negotiation type: %d",
			__func__, neg_type);
		break;
	}

	return rpu_neg_type;
}

static enum wifi_twt_negotiation_type twt_rpu_to_wifi_mgmt_neg_type(unsigned char neg_type)
{
	enum wifi_twt_negotiation_type wifi_neg_type = WIFI_TWT_INDIVIDUAL;

	switch (neg_type) {
	case NRF_WIFI_TWT_NEGOTIATION_TYPE_INDIVIDUAL:
		wifi_neg_type = WIFI_TWT_INDIVIDUAL;
		break;
	case NRF_WIFI_TWT_NEGOTIATION_TYPE_BROADCAST:
		wifi_neg_type = WIFI_TWT_BROADCAST;
		break;
	default:
		LOG_ERR("%s: Invalid negotiation type: %d",
			__func__, neg_type);
		break;
	}

	return wifi_neg_type;
}

/* Though setup_cmd enums have 1-1 mapping but due to data type different need these */
static enum wifi_twt_setup_cmd twt_rpu_to_wifi_mgmt_setup_cmd(signed int setup_cmd)
{
	enum wifi_twt_setup_cmd wifi_setup_cmd = WIFI_TWT_SETUP_CMD_REQUEST;

	switch (setup_cmd) {
	case NRF_WIFI_REQUEST_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_REQUEST;
		break;
	case NRF_WIFI_SUGGEST_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_SUGGEST;
		break;
	case NRF_WIFI_DEMAND_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_DEMAND;
		break;
	case NRF_WIFI_GROUPING_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_GROUPING;
		break;
	case NRF_WIFI_ACCEPT_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_ACCEPT;
		break;
	case NRF_WIFI_ALTERNATE_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_ALTERNATE;
		break;
	case NRF_WIFI_DICTATE_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_DICTATE;
		break;
	case NRF_WIFI_REJECT_TWT:
		wifi_setup_cmd = WIFI_TWT_SETUP_CMD_REJECT;
		break;
	default:
		LOG_ERR("%s: Invalid setup command: %d",
			__func__, setup_cmd);
		break;
	}

	return wifi_setup_cmd;
}

static signed int twt_wifi_mgmt_to_rpu_setup_cmd(enum wifi_twt_setup_cmd setup_cmd)
{
	signed int rpu_setup_cmd = NRF_WIFI_REQUEST_TWT;

	switch (setup_cmd) {
	case WIFI_TWT_SETUP_CMD_REQUEST:
		rpu_setup_cmd = NRF_WIFI_REQUEST_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_SUGGEST:
		rpu_setup_cmd = NRF_WIFI_SUGGEST_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_DEMAND:
		rpu_setup_cmd = NRF_WIFI_DEMAND_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_GROUPING:
		rpu_setup_cmd = NRF_WIFI_GROUPING_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_ACCEPT:
		rpu_setup_cmd = NRF_WIFI_ACCEPT_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_ALTERNATE:
		rpu_setup_cmd = NRF_WIFI_ALTERNATE_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_DICTATE:
		rpu_setup_cmd = NRF_WIFI_DICTATE_TWT;
		break;
	case WIFI_TWT_SETUP_CMD_REJECT:
		rpu_setup_cmd = NRF_WIFI_REJECT_TWT;
		break;
	default:
		LOG_ERR("%s: Invalid setup command: %d",
			__func__, setup_cmd);
		break;
	}

	return rpu_setup_cmd;
}

void nrf_wifi_event_proc_get_power_save_info(void *vif_ctx,
					     struct nrf_wifi_umac_event_power_save_info *ps_info,
					     unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;

	if (!vif_ctx || !ps_info) {
		return;
	}

	vif_ctx_zep = vif_ctx;

	vif_ctx_zep->ps_info->ps_params.mode = ps_info->ps_mode;
	vif_ctx_zep->ps_info->ps_params.enabled = ps_info->enabled;
	vif_ctx_zep->ps_info->num_twt_flows = ps_info->num_twt_flows;
	vif_ctx_zep->ps_info->ps_params.timeout_ms = ps_info->ps_timeout;
	vif_ctx_zep->ps_info->ps_params.listen_interval = ps_info->listen_interval;
	vif_ctx_zep->ps_info->ps_params.wakeup_mode = ps_info->extended_ps;
	if (ps_info->ps_exit_strategy == EVERY_TIM) {
		vif_ctx_zep->ps_info->ps_params.exit_strategy = WIFI_PS_EXIT_EVERY_TIM;
	} else if (ps_info->ps_exit_strategy == INT_PS) {
		vif_ctx_zep->ps_info->ps_params.exit_strategy = WIFI_PS_EXIT_CUSTOM_ALGO;
	}

	for (int i = 0; i < ps_info->num_twt_flows; i++) {
		struct twt_interval_float twt_interval_fp;
		struct wifi_twt_flow_info *twt_zep = &vif_ctx_zep->ps_info->twt_flows[i];
		struct nrf_wifi_umac_config_twt_info *twt_rpu = &ps_info->twt_flow_info[i];

		memset(twt_zep, 0, sizeof(struct wifi_twt_flow_info));

		twt_zep->flow_id = twt_rpu->twt_flow_id;
		twt_zep->implicit = twt_rpu->is_implicit ? 1 : 0;
		twt_zep->trigger = twt_rpu->ap_trigger_frame ? 1 : 0;
		twt_zep->announce = twt_rpu->twt_flow_type == NRF_WIFI_TWT_FLOW_TYPE_ANNOUNCED;
		twt_zep->negotiation_type = twt_rpu_to_wifi_mgmt_neg_type(twt_rpu->neg_type);
		twt_zep->dialog_token = twt_rpu->dialog_token;
		twt_interval_fp.mantissa = twt_rpu->twt_target_wake_interval_mantissa;
		twt_interval_fp.exponent = twt_rpu->twt_target_wake_interval_exponent;
		twt_zep->twt_interval = nrf_wifi_twt_float_to_us(twt_interval_fp);
		twt_zep->twt_wake_interval = twt_rpu->nominal_min_twt_wake_duration;
	}

	vif_ctx_zep->ps_config_info_evnt = true;
}

static void nrf_wifi_twt_update_internal_state(struct nrf_wifi_vif_ctx_zep *vif_ctx_zep,
	bool setup, unsigned char flow_id)
{
	if (setup) {
		vif_ctx_zep->twt_flows_map |= BIT(flow_id);
		vif_ctx_zep->twt_flow_in_progress_map &= ~BIT(flow_id);
	} else {
		vif_ctx_zep->twt_flows_map &= ~BIT(flow_id);
	}
}

int nrf_wifi_twt_teardown_flows(struct nrf_wifi_vif_ctx_zep *vif_ctx_zep,
		unsigned char start_flow_id, unsigned char end_flow_id)
{
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_umac_config_twt_info twt_info = {0};
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int ret = 0;
	struct wifi_twt_params twt_params = {0};

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return ret;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		return ret;
	}

	k_mutex_lock(&vif_ctx_zep->vif_lock, K_FOREVER);
	if (!rpu_ctx_zep->rpu_ctx) {
		LOG_DBG("%s: RPU context not initialized", __func__);
		goto out;
	}

	for (int flow_id = start_flow_id; flow_id < end_flow_id; flow_id++) {
		if (!(vif_ctx_zep->twt_flows_map & BIT(flow_id))) {
			continue;
		}
		twt_info.twt_flow_id = flow_id;
		status = nrf_wifi_fmac_twt_teardown(rpu_ctx_zep->rpu_ctx,
						vif_ctx_zep->vif_idx,
						&twt_info);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: TWT teardown for flow id %d failed",
				__func__, flow_id);
			ret = -1;
			continue;
		}
		/* UMAC doesn't send TWT teardown event for host initiated teardown */
		nrf_wifi_twt_update_internal_state(vif_ctx_zep, false, flow_id);
		/* TODO: Remove this once UMAC sends the status */
		twt_params.operation = WIFI_TWT_TEARDOWN;
		twt_params.flow_id = flow_id;
		twt_params.teardown_status = WIFI_TWT_TEARDOWN_SUCCESS;
		wifi_mgmt_raise_twt_event(vif_ctx_zep->zep_net_if_ctx, &twt_params);
	}

out:
	k_mutex_unlock(&vif_ctx_zep->vif_lock);
	return ret;
}

int nrf_wifi_set_twt(const struct device *dev,
		     struct wifi_twt_params *twt_params)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_umac_config_twt_info twt_info = {0};
	int ret = -1;

	if (!dev || !twt_params) {
		LOG_ERR("%s: dev or twt_params is NULL", __func__);
		return ret;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return ret;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		return ret;
	}

	k_mutex_lock(&vif_ctx_zep->vif_lock, K_FOREVER);
	if (!rpu_ctx_zep->rpu_ctx) {
		LOG_DBG("%s: RPU context not initialized", __func__);
		goto out;
	}

	if (!(twt_params->operation == WIFI_TWT_TEARDOWN && twt_params->teardown.teardown_all) &&
		twt_params->flow_id >= WIFI_MAX_TWT_FLOWS) {
		LOG_ERR("%s: Invalid flow id: %d",
			__func__, twt_params->flow_id);
		twt_params->fail_reason = WIFI_TWT_FAIL_INVALID_FLOW_ID;
		goto out;
	}

	switch (twt_params->operation) {
	case WIFI_TWT_SETUP:
		if (vif_ctx_zep->twt_flow_in_progress_map & BIT(twt_params->flow_id)) {
			twt_params->fail_reason = WIFI_TWT_FAIL_OPERATION_IN_PROGRESS;
			goto out;
		}

		if (twt_params->setup_cmd == WIFI_TWT_SETUP_CMD_REQUEST) {
			if (vif_ctx_zep->twt_flows_map & BIT(twt_params->flow_id)) {
				twt_params->fail_reason = WIFI_TWT_FAIL_FLOW_ALREADY_EXISTS;
				goto out;
			}
		}

		struct twt_interval_float twt_interval_fp =
			nrf_wifi_twt_us_to_float(twt_params->setup.twt_interval);

		twt_info.twt_flow_id = twt_params->flow_id;
		twt_info.neg_type = twt_wifi_mgmt_to_rpu_neg_type(twt_params->negotiation_type);
		twt_info.setup_cmd = twt_wifi_mgmt_to_rpu_setup_cmd(twt_params->setup_cmd);
		twt_info.ap_trigger_frame = twt_params->setup.trigger;
		twt_info.is_implicit = twt_params->setup.implicit;
		if (twt_params->setup.announce) {
			twt_info.twt_flow_type = NRF_WIFI_TWT_FLOW_TYPE_ANNOUNCED;
		} else {
			twt_info.twt_flow_type = NRF_WIFI_TWT_FLOW_TYPE_UNANNOUNCED;
		}

		twt_info.nominal_min_twt_wake_duration =
				twt_params->setup.twt_wake_interval;
		twt_info.twt_target_wake_interval_mantissa = twt_interval_fp.mantissa;
		twt_info.twt_target_wake_interval_exponent = twt_interval_fp.exponent;

		twt_info.dialog_token = twt_params->dialog_token;
		twt_info.twt_wake_ahead_duration = twt_params->setup.twt_wake_ahead_duration;

		status = nrf_wifi_fmac_twt_setup(rpu_ctx_zep->rpu_ctx,
					   vif_ctx_zep->vif_idx,
					   &twt_info);

		break;
	case WIFI_TWT_TEARDOWN:
		unsigned char start_flow_id = 0;
		unsigned char end_flow_id = WIFI_MAX_TWT_FLOWS;

		if (!twt_params->teardown.teardown_all) {
			if (!(vif_ctx_zep->twt_flows_map & BIT(twt_params->flow_id))) {
				twt_params->fail_reason = WIFI_TWT_FAIL_INVALID_FLOW_ID;
				goto out;
			}
			start_flow_id = twt_params->flow_id;
			end_flow_id = twt_params->flow_id + 1;
			twt_info.twt_flow_id = twt_params->flow_id;
		}

		status = nrf_wifi_twt_teardown_flows(vif_ctx_zep,
						     start_flow_id,
						     end_flow_id);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: TWT teardown failed: start_flow_id: %d, end_flow_id: %d",
				__func__, start_flow_id, end_flow_id);
			goto out;
		}
		break;

	default:
		LOG_ERR("Unknown TWT operation");
		status = NRF_WIFI_STATUS_FAIL;
		break;
	}

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed", __func__);
		goto out;
	}

	ret = 0;
out:
	k_mutex_unlock(&vif_ctx_zep->vif_lock);
	return ret;
}

void nrf_wifi_event_proc_twt_setup_zep(void *vif_ctx,
				       struct nrf_wifi_umac_cmd_config_twt *twt_setup_info,
				       unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_twt_params twt_params = { 0 };
	struct twt_interval_float twt_interval_fp = { 0 };

	if (!vif_ctx || !twt_setup_info) {
		return;
	}

	vif_ctx_zep = vif_ctx;

	twt_params.operation = WIFI_TWT_SETUP;
	twt_params.flow_id = twt_setup_info->info.twt_flow_id;
	twt_params.negotiation_type = twt_rpu_to_wifi_mgmt_neg_type(twt_setup_info->info.neg_type);
	twt_params.setup_cmd = twt_rpu_to_wifi_mgmt_setup_cmd(twt_setup_info->info.setup_cmd);
	twt_params.setup.trigger = twt_setup_info->info.ap_trigger_frame ? 1 : 0;
	twt_params.setup.implicit = twt_setup_info->info.is_implicit ? 1 : 0;
	twt_params.setup.announce =
		twt_setup_info->info.twt_flow_type == NRF_WIFI_TWT_FLOW_TYPE_ANNOUNCED;
	twt_params.setup.twt_wake_interval =
			twt_setup_info->info.nominal_min_twt_wake_duration;
	twt_interval_fp.mantissa = twt_setup_info->info.twt_target_wake_interval_mantissa;
	twt_interval_fp.exponent = twt_setup_info->info.twt_target_wake_interval_exponent;
	twt_params.setup.twt_interval = nrf_wifi_twt_float_to_us(twt_interval_fp);
	twt_params.dialog_token = twt_setup_info->info.dialog_token;
	twt_params.resp_status = twt_setup_info->info.twt_resp_status;

	if ((twt_setup_info->info.twt_resp_status == 0) ||
	    (twt_setup_info->info.neg_type == NRF_WIFI_ACCEPT_TWT)) {
		nrf_wifi_twt_update_internal_state(vif_ctx_zep, true, twt_params.flow_id);
	}

	wifi_mgmt_raise_twt_event(vif_ctx_zep->zep_net_if_ctx, &twt_params);
}


void nrf_wifi_event_proc_twt_teardown_zep(void *vif_ctx,
					  struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_info,
					  unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct wifi_twt_params twt_params = {0};

	if (!vif_ctx || !twt_teardown_info) {
		return;
	}

	vif_ctx_zep = vif_ctx;

	twt_params.operation = WIFI_TWT_TEARDOWN;
	twt_params.flow_id = twt_teardown_info->info.twt_flow_id;
	/* TODO: ADD reason code in the twt_params structure */
	nrf_wifi_twt_update_internal_state(vif_ctx_zep, false, twt_params.flow_id);

	wifi_mgmt_raise_twt_event(vif_ctx_zep->zep_net_if_ctx, &twt_params);
}

void nrf_wifi_event_proc_twt_sleep_zep(void *vif_ctx,
					struct nrf_wifi_umac_event_twt_sleep *sleep_evnt,
					unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_priv_def *def_priv = NULL;
#ifdef CONFIG_NRF70_DATA_TX
	int desc = 0;
	int ac = 0;
#endif
	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		return;
	}

	k_mutex_lock(&vif_ctx_zep->vif_lock, K_FOREVER);
	if (!rpu_ctx_zep->rpu_ctx) {
		LOG_DBG("%s: RPU context not initialized", __func__);
		goto out;
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);
	def_priv = wifi_fmac_priv(fmac_dev_ctx->fpriv);

	if (!sleep_evnt) {
		LOG_ERR("%s: sleep_evnt is NULL", __func__);
		return;
	}

	switch (sleep_evnt->info.type) {
	case TWT_BLOCK_TX:
		nrf_wifi_osal_spinlock_take(def_dev_ctx->tx_config.tx_lock);

		def_dev_ctx->twt_sleep_status = NRF_WIFI_FMAC_TWT_STATE_SLEEP;

		wifi_mgmt_raise_twt_sleep_state(vif_ctx_zep->zep_net_if_ctx,
						WIFI_TWT_STATE_SLEEP);
		nrf_wifi_osal_spinlock_rel(def_dev_ctx->tx_config.tx_lock);
	break;
	case TWT_UNBLOCK_TX:
		nrf_wifi_osal_spinlock_take(def_dev_ctx->tx_config.tx_lock);
		def_dev_ctx->twt_sleep_status = NRF_WIFI_FMAC_TWT_STATE_AWAKE;
		wifi_mgmt_raise_twt_sleep_state(vif_ctx_zep->zep_net_if_ctx,
						WIFI_TWT_STATE_AWAKE);
#ifdef CONFIG_NRF70_DATA_TX
		for (ac = NRF_WIFI_FMAC_AC_BE;
		     ac <= NRF_WIFI_FMAC_AC_MAX; ++ac) {
			desc = tx_desc_get(fmac_dev_ctx, ac);
			if (desc < def_priv->num_tx_tokens) {
				tx_pending_process(fmac_dev_ctx, desc, ac);
			}
		}
#endif
		nrf_wifi_osal_spinlock_rel(def_dev_ctx->tx_config.tx_lock);
	break;
	default:
	break;
	}
out:
	k_mutex_unlock(&vif_ctx_zep->vif_lock);
}

#ifdef CONFIG_NRF70_SYSTEM_WITH_RAW_MODES
int nrf_wifi_mode(const struct device *dev,
		  struct wifi_mode_info *mode)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	int ret = -1;

	if (!dev || !mode) {
		LOG_ERR("%s: illegal input parameters", __func__);
		return ret;
	}

	vif_ctx_zep = dev->data;
	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return ret;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		return ret;
	}

	k_mutex_lock(&vif_ctx_zep->vif_lock, K_FOREVER);
	if (!rpu_ctx_zep->rpu_ctx) {
		LOG_DBG("%s: RPU context not initialized", __func__);
		goto out;
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (!device_is_ready(dev)) {
		LOG_ERR("%s: Device %s is not ready",
			__func__, dev->name);
		goto out;
	}

	if (mode->oper == WIFI_MGMT_SET) {
		status = nrf_wifi_check_mode_validity(mode->mode);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: mode setting is not valid", __func__);
			goto out;
		}

		if (vif_ctx_zep->authorized && (mode->mode == NRF_WIFI_MONITOR_MODE)) {
			LOG_ERR("%s: Cannot set monitor mode when station is connected",
				__func__);
			goto out;
		}

		/**
		 * Send the driver vif_idx instead of upper layer sent if_index.
		 * we map network if_index 1 to vif_idx of 0 and so on. The vif_ctx_zep
		 * context maps the correct network interface index to current driver
		 * interface index.
		 */
		status = nrf_wifi_fmac_set_mode(rpu_ctx_zep->rpu_ctx,
						vif_ctx_zep->vif_idx, mode->mode);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: mode set operation failed", __func__);
			goto out;
		}

	} else {
		mode->mode = def_dev_ctx->vif_ctx[vif_ctx_zep->vif_idx]->mode;
		/**
		 * This is a work-around to handle current UMAC mode handling.
		 * This might be removed in future versions when UMAC has more space.
		 */
#ifdef CONFIG_NRF70_RAW_DATA_TX
		if (def_dev_ctx->vif_ctx[vif_ctx_zep->vif_idx]->txinjection_mode == true) {
			mode->mode ^= NRF_WIFI_TX_INJECTION_MODE;
		}
#endif /* CONFIG_NRF70_RAW_DATA_TX */
#ifdef CONFIG_NRF70_PROMISC_DATA_RX
		if (def_dev_ctx->vif_ctx[vif_ctx_zep->vif_idx]->promisc_mode == true) {
			mode->mode ^= NRF_WIFI_PROMISCUOUS_MODE;
		}
#endif /* CONFIG_NRF70_PROMISC_DATA_RX */
	}
	ret = 0;
out:
	k_mutex_unlock(&vif_ctx_zep->vif_lock);
	return ret;
}
#endif /* CONFIG_NRF70_SYSTEM_WITH_RAW_MODES */

#if defined(CONFIG_NRF70_RAW_DATA_TX) || defined(CONFIG_NRF70_RAW_DATA_RX)
int nrf_wifi_channel(const struct device *dev,
		     struct wifi_channel_info *channel)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	int ret = -1;

	if (!dev || !channel) {
		LOG_ERR("%s: illegal input parameters", __func__);
		return ret;
	}

	vif_ctx_zep = dev->data;
	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return ret;
	}

	if (vif_ctx_zep->authorized) {
		LOG_ERR("%s: Cannot change channel when in station connected mode", __func__);
		return ret;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		return ret;
	}

	k_mutex_lock(&vif_ctx_zep->vif_lock, K_FOREVER);
	if (!rpu_ctx_zep->rpu_ctx) {
		LOG_DBG("%s: RPU context not initialized", __func__);
		goto out;
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (channel->oper == WIFI_MGMT_SET) {
		/**
		 * Send the driver vif_idx instead of upper layer sent if_index.
		 * we map network if_index 1 to vif_idx of 0 and so on. The vif_ctx_zep
		 * context maps the correct network interface index to current driver
		 * interface index.
		 */
		status = nrf_wifi_fmac_set_channel(rpu_ctx_zep->rpu_ctx, vif_ctx_zep->vif_idx,
						   channel->channel);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: set channel failed", __func__);
			goto out;
		}
	} else {
		channel->channel = def_dev_ctx->vif_ctx[vif_ctx_zep->vif_idx]->channel;
	}
	ret = 0;
out:
	k_mutex_unlock(&vif_ctx_zep->vif_lock);
	return ret;
}
#endif /* CONFIG_NRF70_RAW_DATA_TX || CONFIG_NRF70_RAW_DATA_RX */

#if defined(CONFIG_NRF70_RAW_DATA_RX) || defined(CONFIG_NRF70_PROMISC_DATA_RX)
int nrf_wifi_filter(const struct device *dev,
		    struct wifi_filter_info *filter)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_fmac_dev_ctx_def *def_dev_ctx = NULL;
	int ret = -1;

	if (!dev || !filter) {
		LOG_ERR("%s: Illegal input parameters", __func__);
		goto out;
	}

	vif_ctx_zep = dev->data;
	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL\n", __func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;
	def_dev_ctx = wifi_dev_priv(fmac_dev_ctx);

	if (filter->oper == WIFI_MGMT_SET) {
		/**
		 * If promiscuous mode is enabled, filter settings
		 * cannot be plumbed to the lower layers as that might
		 * affect connectivity. Save the filter settings in the
		 * driver and filter packet type on packet receive by
		 * checking the 802.11 header in the packet
		 */
		if (((def_dev_ctx->vif_ctx[vif_ctx_zep->vif_idx]->mode) &
		    (NRF_WIFI_PROMISCUOUS_MODE)) == NRF_WIFI_PROMISCUOUS_MODE) {
			def_dev_ctx->vif_ctx[vif_ctx_zep->vif_idx]->packet_filter =
				filter->filter;
			ret = 0;
			goto out;
		}

		/**
		 * In case a user sets data + management + ctrl bits
		 * or all the filter bits. Map it to bit 0 set to
		 * enable "all" packet filter bit setting.
		 * In case only filter packet size is configured and filter
		 * setting is sent as zero, set the filter value to
		 * previously configured value.
		 */
		if (filter->filter == WIFI_MGMT_DATA_CTRL_FILTER_SETTING
		    || filter->filter == WIFI_ALL_FILTER_SETTING) {
			filter->filter = 1;
		} else if (filter->filter == 0) {
			filter->filter =
				def_dev_ctx->vif_ctx[vif_ctx_zep->vif_idx]->packet_filter;
		}

		/**
		 * Send the driver vif_idx instead of upper layer sent if_index.
		 * we map network if_index 1 to vif_idx of 0 and so on. The vif_ctx_zep
		 * context maps the correct network interface index to current driver
		 * interface index
		 */
		status = nrf_wifi_fmac_set_packet_filter(rpu_ctx_zep->rpu_ctx, filter->filter,
							 vif_ctx_zep->vif_idx, filter->buffer_size);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: Set filter operation failed\n", __func__);
			goto out;
		}
	} else {
		filter->filter = def_dev_ctx->vif_ctx[vif_ctx_zep->vif_idx]->packet_filter;
	}
	ret = 0;
out:
	return ret;
}
#endif /* CONFIG_NRF70_RAW_DATA_RX || CONFIG_NRF70_PROMISC_DATA_RX */

int nrf_wifi_set_rts_threshold(const struct device *dev,
			       unsigned int rts_threshold)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_umac_set_wiphy_info wiphy_info;
	int ret = -1;

	if (!dev) {
		LOG_ERR("%s: dev is NULL", __func__);
		return ret;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return ret;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		return ret;
	}


	if (!rpu_ctx_zep->rpu_ctx) {
		LOG_ERR("%s: RPU context not initialized", __func__);
		return ret;
	}

	if ((int)rts_threshold < -1) {
		/* 0 or any positive value is passed to f/w.
		 * For RTS off, -1 is passed to f/w.
		 * All other negative values considered as invalid.
		 */
		LOG_ERR("%s: Invalid threshold value : %d", __func__, (int)rts_threshold);
		return ret;
	}

	memset(&wiphy_info, 0, sizeof(struct nrf_wifi_umac_set_wiphy_info));

	wiphy_info.rts_threshold = (int)rts_threshold;

	k_mutex_lock(&vif_ctx_zep->vif_lock, K_FOREVER);

	status = nrf_wifi_fmac_set_wiphy_params(rpu_ctx_zep->rpu_ctx,
						vif_ctx_zep->vif_idx,
						&wiphy_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Configuring rts threshold failed\n", __func__);
		goto out;
	}

	vif_ctx_zep->rts_threshold_value = (int)rts_threshold;

	ret = 0;
out:
	k_mutex_unlock(&vif_ctx_zep->vif_lock);

	return ret;
}

int nrf_wifi_get_rts_threshold(const struct device *dev,
			       unsigned int *rts_threshold)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	int ret = -1;

	if (!dev) {
		LOG_ERR("%s: dev is NULL", __func__);
		return ret;
	}

	vif_ctx_zep = dev->data;
	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return ret;
	}

	*rts_threshold = vif_ctx_zep->rts_threshold_value;
	ret = 0;

	return ret;
}
