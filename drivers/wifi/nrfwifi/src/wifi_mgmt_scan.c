/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing display scan specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "util.h"
#include "fmac_api.h"
#include "fmac_tx.h"
#include "fmac_main.h"
#include "wifi_mgmt_scan.h"

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF70_LOG_LEVEL);

extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;

static enum nrf_wifi_band nrf_wifi_map_zep_band_to_rpu(enum wifi_frequency_bands zep_band)
{
	switch (zep_band) {
	case WIFI_FREQ_BAND_2_4_GHZ:
		return NRF_WIFI_BAND_2GHZ;
	case WIFI_FREQ_BAND_5_GHZ:
		return NRF_WIFI_BAND_5GHZ;
	default:
		return NRF_WIFI_BAND_INVALID;
	}
}

int nrf_wifi_disp_scan_zep(const struct device *dev, struct wifi_scan_params *params,
			   scan_result_cb_t cb)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_umac_scan_info *scan_info = NULL;
	enum nrf_wifi_band band = NRF_WIFI_BAND_INVALID;
	uint8_t band_flags = 0xFF;
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t k = 0;
	uint16_t num_scan_channels = 0;
	int ret = -1;

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return ret;
	}

	if (vif_ctx_zep->if_op_state != NRF_WIFI_FMAC_IF_OP_STATE_UP) {
		LOG_ERR("%s: Interface not UP", __func__);
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


	if (vif_ctx_zep->scan_in_progress) {
		LOG_INF("%s: Scan already in progress", __func__);
		ret = -EBUSY;
		goto out;
	}

	if (params) {
		band_flags &= (~(1 << WIFI_FREQ_BAND_2_4_GHZ));

#ifndef CONFIG_NRF70_2_4G_ONLY
		band_flags &= (~(1 << WIFI_FREQ_BAND_5_GHZ));
#endif /* CONFIG_NRF70_2_4G_ONLY */

		if (params->bands & band_flags) {
			LOG_ERR("%s: Unsupported band(s) (0x%X)", __func__, params->bands);
			ret = -EBUSY;
			goto out;
		}

		for (j = 0; j < CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL; j++) {
			if (!params->band_chan[j].channel) {
				break;
			}

			num_scan_channels++;
		}
	}

	vif_ctx_zep->disp_scan_cb = cb;

	scan_info = k_calloc(sizeof(*scan_info) +
			     (num_scan_channels *
			      sizeof(scan_info->scan_params.center_frequency[0])),
			     sizeof(char));

	if (!scan_info) {
		LOG_ERR("%s: Unable to allocate memory for scan_info (size: %d bytes)",
			__func__,
		       sizeof(*scan_info) + (num_scan_channels *
					     sizeof(scan_info->scan_params.center_frequency[0])));
		goto out;
	}

	memset(scan_info, 0, sizeof(*scan_info) + (num_scan_channels *
		sizeof(scan_info->scan_params.center_frequency[0])));

	static uint8_t skip_local_admin_mac = IS_ENABLED(CONFIG_WIFI_NRF70_SKIP_LOCAL_ADMIN_MAC);

	scan_info->scan_params.skip_local_admin_macs = skip_local_admin_mac;

	scan_info->scan_reason = SCAN_DISPLAY;

	if (params) {
		if (params->scan_type == WIFI_SCAN_TYPE_PASSIVE) {
			scan_info->scan_params.passive_scan = 1;
		}

		scan_info->scan_params.bands = params->bands;

		if (params->dwell_time_active < 0) {
			LOG_ERR("%s: Invalid dwell_time_active %d", __func__,
				params->dwell_time_active);
			goto out;
		} else {
			scan_info->scan_params.dwell_time_active = params->dwell_time_active;
		}

		if (params->dwell_time_passive < 0) {
			LOG_ERR("%s: Invalid dwell_time_passive %d", __func__,
				params->dwell_time_passive);
			goto out;
		} else {
			scan_info->scan_params.dwell_time_passive = params->dwell_time_passive;
		}

		if ((params->max_bss_cnt < 0) ||
		    (params->max_bss_cnt > WIFI_MGMT_SCAN_MAX_BSS_CNT)) {
			LOG_ERR("%s: Invalid max_bss_cnt %d", __func__,
				params->max_bss_cnt);
			goto out;
		} else {
			vif_ctx_zep->max_bss_cnt = params->max_bss_cnt;
		}

		for (i = 0; i < NRF_WIFI_SCAN_MAX_NUM_SSIDS; i++) {
			if (!(params->ssids[i]) || !strlen(params->ssids[i])) {
				break;
			}

			memcpy(scan_info->scan_params.scan_ssids[i].nrf_wifi_ssid,
			       params->ssids[i],
			       sizeof(scan_info->scan_params.scan_ssids[i].nrf_wifi_ssid));

			scan_info->scan_params.scan_ssids[i].nrf_wifi_ssid_len =
				strlen(scan_info->scan_params.scan_ssids[i].nrf_wifi_ssid);

			scan_info->scan_params.num_scan_ssids++;
		}

		for (i = 0; i < CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL; i++) {
			if (!params->band_chan[i].channel) {
				break;
			}

			band = nrf_wifi_map_zep_band_to_rpu(params->band_chan[i].band);

			if (band == NRF_WIFI_BAND_INVALID) {
				LOG_ERR("%s: Unsupported band %d", __func__,
					params->band_chan[i].band);
				goto out;
			}

			scan_info->scan_params.center_frequency[k++] = nrf_wifi_utils_chan_to_freq(
				fmac_dev_ctx->fpriv->opriv, band, params->band_chan[i].channel);

			if (scan_info->scan_params.center_frequency[k - 1] == -1) {
				LOG_ERR("%s: Invalid channel %d", __func__,
					 params->band_chan[i].channel);
				goto out;
			}
		}

		scan_info->scan_params.num_scan_channels = k;
	}

	vif_ctx_zep->scan_res_cnt = 0;

	status = nrf_wifi_fmac_scan(rpu_ctx_zep->rpu_ctx, vif_ctx_zep->vif_idx, scan_info);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_scan failed", __func__);
		goto out;
	}

	vif_ctx_zep->scan_type = SCAN_DISPLAY;
	vif_ctx_zep->scan_in_progress = true;

	k_work_schedule(&vif_ctx_zep->scan_timeout_work,
		K_SECONDS(CONFIG_WIFI_NRF70_SCAN_TIMEOUT_S));

	ret = 0;
out:
	if (scan_info) {
		k_free(scan_info);
	}
	k_mutex_unlock(&vif_ctx_zep->vif_lock);
	return ret;
}

enum nrf_wifi_status nrf_wifi_disp_scan_res_get_zep(struct nrf_wifi_vif_ctx_zep *vif_ctx_zep)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	k_mutex_lock(&vif_ctx_zep->vif_lock, K_FOREVER);
	if (!rpu_ctx_zep->rpu_ctx) {
		LOG_DBG("%s: RPU context not initialized", __func__);
		goto out;
	}

	status = nrf_wifi_fmac_scan_res_get(rpu_ctx_zep->rpu_ctx,
					    vif_ctx_zep->vif_idx,
					    SCAN_DISPLAY);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_scan failed", __func__);
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	k_mutex_unlock(&vif_ctx_zep->vif_lock);
	return status;
}

static inline enum wifi_mfp_options drv_to_wifi_mgmt_mfp(unsigned char mfp_flag)
{
	if (!mfp_flag)
		return WIFI_MFP_DISABLE;
	if (mfp_flag & NRF_WIFI_MFP_REQUIRED)
		return WIFI_MFP_REQUIRED;
	if (mfp_flag & NRF_WIFI_MFP_CAPABLE)
		return WIFI_MFP_OPTIONAL;

	return WIFI_MFP_UNKNOWN;
}
static inline enum wifi_security_type drv_to_wifi_mgmt(int drv_security_type)
{
	switch (drv_security_type) {
	case NRF_WIFI_OPEN:
		return WIFI_SECURITY_TYPE_NONE;
	case NRF_WIFI_WEP:
		return WIFI_SECURITY_TYPE_WEP;
	case NRF_WIFI_WPA:
		return WIFI_SECURITY_TYPE_WPA_PSK;
	case NRF_WIFI_WPA2:
		return WIFI_SECURITY_TYPE_PSK;
	case NRF_WIFI_WPA2_256:
		return WIFI_SECURITY_TYPE_PSK_SHA256;
	case NRF_WIFI_WPA3:
		return WIFI_SECURITY_TYPE_SAE;
	case NRF_WIFI_WAPI:
		return WIFI_SECURITY_TYPE_WAPI;
	case NRF_WIFI_EAP:
		return WIFI_SECURITY_TYPE_EAP;
	default:
		return WIFI_SECURITY_TYPE_UNKNOWN;
	}
}

void nrf_wifi_event_proc_disp_scan_res_zep(void *vif_ctx,
				struct nrf_wifi_umac_event_new_scan_display_results *scan_res,
				unsigned int event_len,
				bool more_res)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct umac_display_results *r = NULL;
	struct wifi_scan_result res;
	uint16_t max_bss_cnt = 0;
	unsigned int i = 0;
	scan_result_cb_t cb = NULL;

	vif_ctx_zep = vif_ctx;

	cb = (scan_result_cb_t)vif_ctx_zep->disp_scan_cb;

	/* Delayed event (after scan timeout) or rogue event after scan done */
	if (!cb) {
		return;
	}

	max_bss_cnt = vif_ctx_zep->max_bss_cnt ?
		vif_ctx_zep->max_bss_cnt : CONFIG_NRF_WIFI_SCAN_MAX_BSS_CNT;

	for (i = 0; i < scan_res->event_bss_count; i++) {
		/* Limit the scan results to the configured maximum */
		if ((max_bss_cnt > 0) &&
		    (vif_ctx_zep->scan_res_cnt >= max_bss_cnt)) {
			break;
		}

		memset(&res, 0x0, sizeof(res));

		r = &scan_res->display_results[i];

		res.ssid_length = MIN(sizeof(res.ssid), r->ssid.nrf_wifi_ssid_len);

		res.band = r->nwk_band;

		res.channel = r->nwk_channel;

		res.security = drv_to_wifi_mgmt(r->security_type);

		res.mfp = drv_to_wifi_mgmt_mfp(r->mfp_flag);

		memcpy(res.ssid,
		       r->ssid.nrf_wifi_ssid,
		       res.ssid_length);

		memcpy(res.mac,	r->mac_addr, NRF_WIFI_ETH_ADDR_LEN);
		res.mac_length = NRF_WIFI_ETH_ADDR_LEN;

		if (r->signal.signal_type == NRF_WIFI_SIGNAL_TYPE_MBM) {
			int val = (r->signal.signal.mbm_signal);

			res.rssi = (val / 100);
		} else if (r->signal.signal_type == NRF_WIFI_SIGNAL_TYPE_UNSPEC) {
			res.rssi = (r->signal.signal.unspec_signal);
		}

		vif_ctx_zep->disp_scan_cb(vif_ctx_zep->zep_net_if_ctx,
					  0,
					  &res);

		vif_ctx_zep->scan_res_cnt++;

		/* NET_MGMT dropping events if too many are queued */
		k_yield();
	}

	if (more_res == false) {
		vif_ctx_zep->disp_scan_cb(vif_ctx_zep->zep_net_if_ctx, 0, NULL);
		vif_ctx_zep->scan_in_progress = false;
		vif_ctx_zep->disp_scan_cb = NULL;
		k_work_cancel_delayable(&vif_ctx_zep->scan_timeout_work);
	}
}


#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
void nrf_wifi_rx_bcn_prb_resp_frm(void *vif_ctx,
				  void *nwb,
				  unsigned short frequency,
				  signed short signal)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = vif_ctx;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct wifi_raw_scan_result bcn_prb_resp_info;
	int frame_length = 0;
	int val = signal;

	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return;
	}

	if (!vif_ctx_zep->scan_in_progress) {
		/*LOG_INF("%s: Scan not in progress : raw scan data not available", __func__);*/
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

	frame_length = nrf_wifi_osal_nbuf_data_size(fmac_dev_ctx->fpriv->opriv,
						    nwb);

	if (frame_length > CONFIG_WIFI_MGMT_RAW_SCAN_RESULT_LENGTH) {
		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &bcn_prb_resp_info.data,
				      nrf_wifi_osal_nbuf_data_get(
						fmac_dev_ctx->fpriv->opriv,
						nwb),
				      CONFIG_WIFI_MGMT_RAW_SCAN_RESULT_LENGTH);

	} else {
		nrf_wifi_osal_mem_cpy(fmac_dev_ctx->fpriv->opriv,
				      &bcn_prb_resp_info.data,
				      nrf_wifi_osal_nbuf_data_get(
					      fmac_dev_ctx->fpriv->opriv,
					      nwb),
				      frame_length);
	}

	bcn_prb_resp_info.rssi = MBM_TO_DBM(val);
	bcn_prb_resp_info.frequency = frequency;
	bcn_prb_resp_info.frame_length = frame_length;

	wifi_mgmt_raise_raw_scan_result_event(vif_ctx_zep->zep_net_if_ctx,
					      &bcn_prb_resp_info);

out:
	k_mutex_unlock(&vif_ctx_zep->vif_lock);
}
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
