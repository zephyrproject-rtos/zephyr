/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief File containing FMAC interface specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#ifdef CONFIG_NET_L2_ETHERNET
#include <zephyr/net/ethernet.h>
#endif
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>


#include <util.h>
#include "common/fmac_util.h"
#include <fmac_main.h>

#ifndef CONFIG_NRF70_RADIO_TEST
#ifdef CONFIG_NRF70_STA_MODE
#include <zephyr/net/wifi_nm.h>
#include <wifi_mgmt_scan.h>
#include <wifi_mgmt.h>
#include <wpa_supp_if.h>
#else
#include <wifi_mgmt.h>
#include <wifi_mgmt_scan.h>
#endif /* CONFIG_NRF70_STA_MODE */

#include <system/fmac_api.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#else
#include <radio_test/fmac_api.h>
#endif /* !CONFIG_NRF70_RADIO_TEST */

#define DT_DRV_COMPAT nordic_wlan
LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF70_LOG_LEVEL);

struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;
extern const struct nrf_wifi_osal_ops nrf_wifi_os_zep_ops;

/* 3 bytes for addreess, 3 bytes for length */
#define MAX_PKT_RAM_TX_ALIGN_OVERHEAD 6
#ifndef CONFIG_NRF70_RADIO_TEST
#ifdef CONFIG_NRF70_DATA_TX

#define MAX_RX_QUEUES 3

#define MAX_TX_FRAME_SIZE \
	(CONFIG_NRF_WIFI_IFACE_MTU + NRF_WIFI_FMAC_ETH_HDR_LEN + TX_BUF_HEADROOM)
#define TOTAL_TX_SIZE \
	(CONFIG_NRF70_MAX_TX_TOKENS * CONFIG_NRF70_TX_MAX_DATA_SIZE)
#define TOTAL_RX_SIZE \
	(CONFIG_NRF70_RX_NUM_BUFS * CONFIG_NRF70_RX_MAX_DATA_SIZE)

BUILD_ASSERT(CONFIG_NRF70_MAX_TX_TOKENS >= 1,
	"At least one TX token is required");
BUILD_ASSERT(CONFIG_NRF70_MAX_TX_AGGREGATION <= 15,
	"Max TX aggregation is 15");
BUILD_ASSERT(CONFIG_NRF70_RX_NUM_BUFS >= 1,
	"At least one RX buffer is required");
BUILD_ASSERT(RPU_PKTRAM_SIZE - TOTAL_RX_SIZE >= TOTAL_TX_SIZE,
	"Packet RAM overflow: not enough memory for TX");

BUILD_ASSERT(CONFIG_NRF70_TX_MAX_DATA_SIZE >= MAX_TX_FRAME_SIZE,
	"TX buffer size must be at least as big as the MTU and headroom");

BUILD_ASSERT(CONFIG_NRF70_TX_MAX_DATA_SIZE % 4 == 0,
	"TX buffer size must be a multiple of 4");
BUILD_ASSERT(CONFIG_NRF70_RX_MAX_DATA_SIZE % 4 == 0,
	"RX buffer size must be a multiple of 4");
BUILD_ASSERT(CONFIG_NRF70_RX_MAX_DATA_SIZE >= 400,
	"RX buffer size must be at least 400 bytes");

static const unsigned char aggregation = 1;
static const unsigned char max_num_tx_agg_sessions = 4;
static const unsigned char max_num_rx_agg_sessions = 8;
static const unsigned char reorder_buf_size = CONFIG_NRF70_RX_NUM_BUFS / 2;
static const unsigned char max_rxampdu_size = MAX_RX_AMPDU_SIZE_64KB;

static const unsigned char max_tx_aggregation = CONFIG_NRF70_MAX_TX_AGGREGATION;

static const unsigned int rx1_num_bufs = CONFIG_NRF70_RX_NUM_BUFS / MAX_RX_QUEUES;
static const unsigned int rx2_num_bufs = CONFIG_NRF70_RX_NUM_BUFS / MAX_RX_QUEUES;
static const unsigned int rx3_num_bufs = CONFIG_NRF70_RX_NUM_BUFS / MAX_RX_QUEUES;

static const unsigned int rx1_buf_sz = CONFIG_NRF70_RX_MAX_DATA_SIZE;
static const unsigned int rx2_buf_sz = CONFIG_NRF70_RX_MAX_DATA_SIZE;
static const unsigned int rx3_buf_sz = CONFIG_NRF70_RX_MAX_DATA_SIZE;

static const unsigned char rate_protection_type;
#else
/* Reduce buffers to Scan only operation */
static const unsigned int rx1_num_bufs = 2;
static const unsigned int rx2_num_bufs = 2;
static const unsigned int rx3_num_bufs = 2;

static const unsigned int rx1_buf_sz = 1000;
static const unsigned int rx2_buf_sz = 1000;
static const unsigned int rx3_buf_sz = 1000;
#endif

struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;
static K_MUTEX_DEFINE(reg_lock);

const char *nrf_wifi_get_drv_version(void)
{
	return NRF70_DRIVER_VERSION;
}

/* If the interface is not Wi-Fi then errors are expected, so, fail silently */
struct nrf_wifi_vif_ctx_zep *nrf_wifi_get_vif_ctx(struct net_if *iface)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx = &rpu_drv_priv_zep.rpu_ctx_zep;

	if (!iface || !rpu_ctx || !rpu_ctx->rpu_ctx) {
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(rpu_ctx->vif_ctx_zep); i++) {
		if (rpu_ctx->vif_ctx_zep[i].zep_net_if_ctx == iface) {
			vif_ctx_zep = &rpu_ctx->vif_ctx_zep[i];
			break;
		}
	}

	return vif_ctx_zep;
}

void nrf_wifi_event_proc_scan_start_zep(void *if_priv,
					struct nrf_wifi_umac_event_trigger_scan *scan_start_event,
					unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;

	vif_ctx_zep = if_priv;

	if (vif_ctx_zep->scan_type == SCAN_DISPLAY) {
		return;
	}

#ifdef CONFIG_NRF70_STA_MODE
	nrf_wifi_wpa_supp_event_proc_scan_start(if_priv);
#endif /* CONFIG_NRF70_STA_MODE */
}


void nrf_wifi_event_proc_scan_done_zep(void *vif_ctx,
				       struct nrf_wifi_umac_event_trigger_scan *scan_done_event,
				       unsigned int event_len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;

	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return;
	}

	switch (vif_ctx_zep->scan_type) {
#ifdef CONFIG_NET_L2_WIFI_MGMT
	case SCAN_DISPLAY:
		/* Schedule scan result processing in system workqueue to avoid deadlock */
		k_work_submit(&vif_ctx_zep->disp_scan_res_work);
		break;
#endif /* CONFIG_NET_L2_WIFI_MGMT */
#ifdef CONFIG_NRF70_STA_MODE
	case SCAN_CONNECT:
		nrf_wifi_wpa_supp_event_proc_scan_done(vif_ctx_zep,
						       scan_done_event,
						       event_len,
						       0);
		break;
#endif /* CONFIG_NRF70_STA_MODE */
	default:
		LOG_ERR("%s: Scan type = %d not supported yet", __func__, vif_ctx_zep->scan_type);
		return;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
}

void nrf_wifi_scan_timeout_work(struct k_work *work)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	vif_ctx_zep = CONTAINER_OF(work, struct nrf_wifi_vif_ctx_zep, scan_timeout_work.work);

	if (!vif_ctx_zep->scan_in_progress) {
		LOG_INF("%s: Scan not in progress", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

#ifdef CONFIG_NET_L2_WIFI_MGMT
	if (vif_ctx_zep->disp_scan_cb) {
		vif_ctx_zep->disp_scan_cb(vif_ctx_zep->zep_net_if_ctx, -ETIMEDOUT, NULL);
		vif_ctx_zep->disp_scan_cb = NULL;
	} else
#endif /* CONFIG_NET_L2_WIFI_MGMT */
	{
#ifdef CONFIG_NRF70_STA_MODE
		/* WPA supplicant scan */
		union wpa_event_data event;
		struct scan_info *info = NULL;

		memset(&event, 0, sizeof(event));

		info = &event.scan_info;

		info->aborted = 0;
		info->external_scan = 0;
		info->nl_scan_event = 1;

		if (vif_ctx_zep->supp_drv_if_ctx &&
			vif_ctx_zep->supp_callbk_fns.scan_done) {
			vif_ctx_zep->supp_callbk_fns.scan_done(vif_ctx_zep->supp_drv_if_ctx,
				&event);
		}
#endif /* CONFIG_NRF70_STA_MODE */
	}

	vif_ctx_zep->scan_in_progress = false;
}

void nrf_wifi_disp_scan_res_work_handler(struct k_work *work)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	vif_ctx_zep = CONTAINER_OF(work, struct nrf_wifi_vif_ctx_zep, disp_scan_res_work);

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return;
	}

	status = nrf_wifi_disp_scan_res_get_zep(vif_ctx_zep);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_disp_scan_res_get_zep failed", __func__);
		return;
	}
	vif_ctx_zep->scan_in_progress = false;
}

#ifdef CONFIG_NRF70_STA_MODE
static void nrf_wifi_process_rssi_from_rx(void *vif_ctx,
				   signed short signal)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = vif_ctx;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!(rpu_ctx_zep && rpu_ctx_zep->rpu_ctx)) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		return;
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	vif_ctx_zep->rssi = MBM_TO_DBM(signal);
	vif_ctx_zep->rssi_record_timestamp_us =
		nrf_wifi_osal_time_get_curr_us();
}
#endif /* CONFIG_NRF70_STA_MODE */


void nrf_wifi_event_get_reg_zep(void *vif_ctx,
				struct nrf_wifi_reg *get_reg_event,
				unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	LOG_DBG("%s: alpha2 = %c%c", __func__,
		   get_reg_event->nrf_wifi_alpha2[0],
		   get_reg_event->nrf_wifi_alpha2[1]);
	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	if (fmac_dev_ctx->alpha2_valid) {
		LOG_ERR("%s: Unsolicited regulatory get!", __func__);
		return;
	}

	memcpy(&fmac_dev_ctx->alpha2,
		   &get_reg_event->nrf_wifi_alpha2,
		   sizeof(get_reg_event->nrf_wifi_alpha2));

	fmac_dev_ctx->reg_chan_count = get_reg_event->num_channels;
	memcpy(fmac_dev_ctx->reg_chan_info,
	       &get_reg_event->chn_info,
	       fmac_dev_ctx->reg_chan_count *
		   sizeof(struct nrf_wifi_get_reg_chn_info));

	fmac_dev_ctx->alpha2_valid = true;
}

int nrf_wifi_reg_domain(const struct device *dev, struct wifi_reg_domain *reg_domain)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_fmac_reg_info reg_domain_info = {0};
	struct wifi_reg_chan_info *chan_info = NULL;
	struct nrf_wifi_get_reg_chn_info *reg_domain_chan_info = NULL;
	int ret = -1;
	int chan_idx = 0;

	k_mutex_lock(&reg_lock, K_FOREVER);

	if (!dev || !reg_domain) {
		goto out;
	}

	vif_ctx_zep = dev->data;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		goto out;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;

	if (!rpu_ctx_zep) {
		LOG_ERR("%s: rpu_ctx_zep is NULL", __func__);
		goto out;
	}

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;
	if (!fmac_dev_ctx) {
		LOG_ERR("%s: fmac_dev_ctx is NULL", __func__);
		goto out;
	}

#if defined(CONFIG_NRF70_SCAN_ONLY) || defined(CONFIG_NRF70_RAW_DATA_RX)
	if (reg_domain->oper == WIFI_MGMT_SET) {
		memcpy(reg_domain_info.alpha2, reg_domain->country_code, WIFI_COUNTRY_CODE_LEN);

		reg_domain_info.force = reg_domain->force;

		status = nrf_wifi_fmac_set_reg(fmac_dev_ctx, &reg_domain_info);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			LOG_ERR("%s: Failed to set regulatory domain", __func__);
			goto out;
		}

		ret = 0;
		goto out;
	}
#endif
	if (reg_domain->oper != WIFI_MGMT_GET) {
		LOG_ERR("%s: Invalid operation: %d", __func__, reg_domain->oper);
		goto out;
	}

	if (!reg_domain->chan_info)	{
		LOG_ERR("%s: Invalid regulatory info (NULL)\n", __func__);
		goto out;
	}

	status = nrf_wifi_fmac_get_reg(fmac_dev_ctx, &reg_domain_info);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed to get regulatory domain", __func__);
		goto out;
	}

	memcpy(reg_domain->country_code, reg_domain_info.alpha2, WIFI_COUNTRY_CODE_LEN);
	reg_domain->num_channels = reg_domain_info.reg_chan_count;

	for (chan_idx = 0; chan_idx < reg_domain_info.reg_chan_count; chan_idx++) {
		chan_info = &(reg_domain->chan_info[chan_idx]);
		reg_domain_chan_info = &(reg_domain_info.reg_chan_info[chan_idx]);
		chan_info->center_frequency = reg_domain_chan_info->center_frequency;
		chan_info->dfs = !!reg_domain_chan_info->dfs;
		chan_info->max_power = reg_domain_chan_info->max_power;
		chan_info->passive_only = !!reg_domain_chan_info->passive_channel;
		chan_info->supported = !!reg_domain_chan_info->supported;
	}

	ret = 0;
out:
	k_mutex_unlock(&reg_lock);
	return ret;
}
#ifdef CONFIG_NRF70_STA_MODE
void nrf_wifi_event_proc_cookie_rsp(void *vif_ctx,
				    struct nrf_wifi_umac_event_cookie_rsp *cookie_rsp_event,
				    unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	vif_ctx_zep = vif_ctx;

	if (!vif_ctx_zep) {
		LOG_ERR("%s: vif_ctx_zep is NULL", __func__);
		return;
	}

	rpu_ctx_zep = vif_ctx_zep->rpu_ctx_zep;
	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;

	LOG_DBG("%s: cookie_rsp_event->cookie = %llx", __func__, cookie_rsp_event->cookie);
	LOG_DBG("%s: host_cookie = %llx", __func__, cookie_rsp_event->host_cookie);
	LOG_DBG("%s: mac_addr = %x:%x:%x:%x:%x:%x", __func__,
		cookie_rsp_event->mac_addr[0],
		cookie_rsp_event->mac_addr[1],
		cookie_rsp_event->mac_addr[2],
		cookie_rsp_event->mac_addr[3],
		cookie_rsp_event->mac_addr[4],
		cookie_rsp_event->mac_addr[5]);

	vif_ctx_zep->cookie_resp_received = true;
	/* TODO: When supp_callbk_fns.mgmt_tx_status is implemented, add logic
	 * here to use the cookie and host_cookie to map requests to responses.
	 */
}
#endif /* CONFIG_NRF70_STA_MODE */

void reg_change_callbk_fn(void *vif_ctx,
			  struct nrf_wifi_event_regulatory_change *reg_change_event,
			  unsigned int event_len)
{
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = NULL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = NULL;

	LOG_DBG("%s: Regulatory change event received", __func__);

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

	fmac_dev_ctx = rpu_ctx_zep->rpu_ctx;
	if (!fmac_dev_ctx) {
		LOG_ERR("%s: fmac_dev_ctx is NULL", __func__);
		return;
	}

	if (!fmac_dev_ctx->waiting_for_reg_event) {
		LOG_DBG("%s: Unsolicited regulatory change event", __func__);
		/* TODO: Handle unsolicited regulatory change event */
		return;
	}

	fmac_dev_ctx->reg_change = nrf_wifi_osal_mem_alloc(sizeof(struct
							   nrf_wifi_event_regulatory_change));
	if (!fmac_dev_ctx->reg_change) {
		LOG_ERR("%s: Failed to allocate memory for reg_change", __func__);
		return;
	}

	memcpy(fmac_dev_ctx->reg_change,
		   reg_change_event,
		   sizeof(struct nrf_wifi_event_regulatory_change));
	fmac_dev_ctx->reg_set_status = true;
}
#endif /* !CONFIG_NRF70_RADIO_TEST */

#ifdef CONFIG_NRF71_ON_IPC
#define MAX_TX_PWR(label) DT_PROP(DT_NODELABEL(wifi), label) * 4
#else
/* DTS uses 1dBm as the unit for TX power, while the RPU uses 0.25dBm */
#define MAX_TX_PWR(label) DT_PROP(DT_NODELABEL(nrf70), label) * 4
#endif /* CONFIG_NRF71_ON_IPC */

void configure_tx_pwr_settings(struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
				struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params)
{
	tx_pwr_ctrl_params->ant_gain_2g = CONFIG_NRF70_ANT_GAIN_2G;
	tx_pwr_ctrl_params->ant_gain_5g_band1 = CONFIG_NRF70_ANT_GAIN_5G_BAND1;
	tx_pwr_ctrl_params->ant_gain_5g_band2 = CONFIG_NRF70_ANT_GAIN_5G_BAND2;
	tx_pwr_ctrl_params->ant_gain_5g_band3 = CONFIG_NRF70_ANT_GAIN_5G_BAND3;
	tx_pwr_ctrl_params->band_edge_2g_lo_dss = CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_DSSS;
	tx_pwr_ctrl_params->band_edge_2g_lo_ht = CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_2g_lo_he = CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_2g_hi_dsss = CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_DSSS;
	tx_pwr_ctrl_params->band_edge_2g_hi_ht = CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_2g_hi_he = CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_5g_unii_1_lo_ht =
		CONFIG_NRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_5g_unii_1_lo_he =
		CONFIG_NRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_5g_unii_1_hi_ht =
		CONFIG_NRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_5g_unii_1_hi_he =
		CONFIG_NRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_5g_unii_2a_lo_ht =
		CONFIG_NRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_5g_unii_2a_lo_he =
		CONFIG_NRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_5g_unii_2a_hi_ht =
		CONFIG_NRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_5g_unii_2a_hi_he =
		CONFIG_NRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_5g_unii_2c_lo_ht =
		CONFIG_NRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_5g_unii_2c_lo_he =
		CONFIG_NRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_5g_unii_2c_hi_ht =
		CONFIG_NRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_5g_unii_2c_hi_he =
		CONFIG_NRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_5g_unii_3_lo_ht =
		CONFIG_NRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_5g_unii_3_lo_he =
		CONFIG_NRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_5g_unii_3_hi_ht =
		CONFIG_NRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_5g_unii_3_hi_he =
		CONFIG_NRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_5g_unii_4_lo_ht =
		CONFIG_NRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_5g_unii_4_lo_he =
		CONFIG_NRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HE;
	tx_pwr_ctrl_params->band_edge_5g_unii_4_hi_ht =
		CONFIG_NRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HT;
	tx_pwr_ctrl_params->band_edge_5g_unii_4_hi_he =
		CONFIG_NRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HE;


	tx_pwr_ceil_params->max_pwr_2g_dsss = MAX_TX_PWR(wifi_max_tx_pwr_2g_dsss);
	tx_pwr_ceil_params->max_pwr_2g_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_2g_mcs7);
	tx_pwr_ceil_params->max_pwr_2g_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_2g_mcs0);
#ifndef CONFIG_NRF70_2_4G_ONLY
	tx_pwr_ceil_params->max_pwr_5g_low_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_5g_low_mcs7);
	tx_pwr_ceil_params->max_pwr_5g_mid_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_5g_mid_mcs7);
	tx_pwr_ceil_params->max_pwr_5g_high_mcs7 = MAX_TX_PWR(wifi_max_tx_pwr_5g_high_mcs7);
	tx_pwr_ceil_params->max_pwr_5g_low_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_5g_low_mcs0);
	tx_pwr_ceil_params->max_pwr_5g_mid_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_5g_mid_mcs0);
	tx_pwr_ceil_params->max_pwr_5g_high_mcs0 = MAX_TX_PWR(wifi_max_tx_pwr_5g_high_mcs0);
#endif /* CONFIG_NRF70_2_4G_ONLY */
}

void configure_board_dep_params(struct nrf_wifi_board_params *board_params)
{
	board_params->pcb_loss_2g = CONFIG_NRF70_PCB_LOSS_2G;
#ifndef CONFIG_NRF70_2_4G_ONLY
	board_params->pcb_loss_5g_band1 = CONFIG_NRF70_PCB_LOSS_5G_BAND1;
	board_params->pcb_loss_5g_band2 = CONFIG_NRF70_PCB_LOSS_5G_BAND2;
	board_params->pcb_loss_5g_band3 = CONFIG_NRF70_PCB_LOSS_5G_BAND3;
#endif /* CONFIG_NRF70_2_4G_ONLY */
}

enum nrf_wifi_status nrf_wifi_fmac_dev_add_zep(struct nrf_wifi_drv_priv_zep *drv_priv_zep)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;
	void *rpu_ctx = NULL;
	enum op_band op_band = CONFIG_NRF_WIFI_OP_BAND;
#ifdef CONFIG_NRF_WIFI_LOW_POWER
	int sleep_type = -1;

#ifndef CONFIG_NRF70_RADIO_TEST
	sleep_type = HW_SLEEP_ENABLE;
#else
	sleep_type = SLEEP_DISABLE;
#endif /* CONFIG_NRF70_RADIO_TEST */
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
	struct nrf_wifi_tx_pwr_ctrl_params tx_pwr_ctrl_params;
	struct nrf_wifi_tx_pwr_ceil_params tx_pwr_ceil_params;
	struct nrf_wifi_board_params board_params;

	unsigned int fw_ver = 0;

#if defined(CONFIG_NRF70_SR_COEX_SLEEP_CTRL_GPIO_CTRL) && \
	defined(CONFIG_NRF70_SYSTEM_MODE)
	unsigned int alt_swctrl1_function_bt_coex_status1 =
			(~CONFIG_NRF70_SR_COEX_SWCTRL1_OUTPUT) & 0x1;
	unsigned int invert_bt_coex_grant_output = CONFIG_NRF70_SR_COEX_BT_GRANT_ACTIVE_LOW;
#endif /* CONFIG_NRF70_SR_COEX_SLEEP_CTRL_GPIO_CTRL && CONFIG_NRF70_SYSTEM_MODE */

	rpu_ctx_zep = &drv_priv_zep->rpu_ctx_zep;

	rpu_ctx_zep->drv_priv_zep = drv_priv_zep;

#ifdef CONFIG_NRF70_RADIO_TEST
	rpu_ctx = nrf_wifi_rt_fmac_dev_add(drv_priv_zep->fmac_priv, rpu_ctx_zep);
#else
	rpu_ctx = nrf_wifi_sys_fmac_dev_add(drv_priv_zep->fmac_priv, rpu_ctx_zep);
#endif /* CONFIG_NRF70_RADIO_TEST */

	if (!rpu_ctx) {
		LOG_ERR("%s: nrf_wifi_fmac_dev_add failed", __func__);
		rpu_ctx_zep = NULL;
		goto err;
	}

	rpu_ctx_zep->rpu_ctx = rpu_ctx;

	status = nrf_wifi_fw_load(rpu_ctx);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fw_load failed", __func__);
		goto err;
	}

	status = nrf_wifi_fmac_ver_get(rpu_ctx,
				       &fw_ver);

	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: FW version read failed", __func__);
		goto err;
	}

	LOG_DBG("Firmware (v%d.%d.%d.%d) booted successfully",
		NRF_WIFI_UMAC_VER(fw_ver),
		NRF_WIFI_UMAC_VER_MAJ(fw_ver),
		NRF_WIFI_UMAC_VER_MIN(fw_ver),
		NRF_WIFI_UMAC_VER_EXTRA(fw_ver));

	configure_tx_pwr_settings(&tx_pwr_ctrl_params,
				  &tx_pwr_ceil_params);

	configure_board_dep_params(&board_params);

#if defined(CONFIG_NRF70_SR_COEX_SLEEP_CTRL_GPIO_CTRL) && \
	defined(CONFIG_NRF70_SYSTEM_MODE)
	LOG_DBG("Configuring SLEEP CTRL GPIO control register\n");
	status = nrf_wifi_coex_config_sleep_ctrl_gpio_ctrl(rpu_ctx_zep->rpu_ctx,
			alt_swctrl1_function_bt_coex_status1,
			invert_bt_coex_grant_output);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: Failed to configure GPIO control register", __func__);
		goto err;
	}
#endif /* CONFIG_NRF70_SR_COEX_SLEEP_CTRL_GPIO_CTRL  && CONFIG_NRF70_SYSTEM_MODE */

#ifdef CONFIG_NRF70_RADIO_TEST
	status = nrf_wifi_rt_fmac_dev_init(rpu_ctx_zep->rpu_ctx,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
					sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
					NRF_WIFI_DEF_PHY_CALIB,
					op_band,
					IS_ENABLED(CONFIG_NRF_WIFI_BEAMFORMING),
					&tx_pwr_ctrl_params,
					&tx_pwr_ceil_params,
					&board_params,
					STRINGIFY(CONFIG_NRF70_REG_DOMAIN));
#else
	status = nrf_wifi_sys_fmac_dev_init(rpu_ctx_zep->rpu_ctx,
#ifdef CONFIG_NRF_WIFI_LOW_POWER
					sleep_type,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
					NRF_WIFI_DEF_PHY_CALIB,
					op_band,
					IS_ENABLED(CONFIG_NRF_WIFI_BEAMFORMING),
					&tx_pwr_ctrl_params,
					&tx_pwr_ceil_params,
					&board_params,
					STRINGIFY(CONFIG_NRF70_REG_DOMAIN));
#endif /* CONFIG_NRF70_RADIO_TEST */


	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_sys_fmac_dev_init failed", __func__);
		goto err;
	}

	return status;
err:
	if (rpu_ctx) {
		nrf_wifi_fmac_dev_rem(rpu_ctx);
		rpu_ctx_zep->rpu_ctx = NULL;
	}
	return status;
}

enum nrf_wifi_status nrf_wifi_fmac_dev_rem_zep(struct nrf_wifi_drv_priv_zep *drv_priv_zep)
{
	struct nrf_wifi_ctx_zep *rpu_ctx_zep = NULL;

	rpu_ctx_zep = &drv_priv_zep->rpu_ctx_zep;
#ifdef CONFIG_NRF70_RADIO_TEST
	nrf_wifi_rt_fmac_dev_deinit(rpu_ctx_zep->rpu_ctx);
#else
	nrf_wifi_sys_fmac_dev_deinit(rpu_ctx_zep->rpu_ctx);
#endif /* CONFIG_NRF70_RADIO_TEST */

	nrf_wifi_fmac_dev_rem(rpu_ctx_zep->rpu_ctx);

	nrf_wifi_osal_mem_free(rpu_ctx_zep->extended_capa);
	rpu_ctx_zep->extended_capa = NULL;
	nrf_wifi_osal_mem_free(rpu_ctx_zep->extended_capa_mask);
	rpu_ctx_zep->extended_capa_mask = NULL;

	rpu_ctx_zep->rpu_ctx = NULL;
	LOG_DBG("%s: FMAC device removed", __func__);

	return NRF_WIFI_STATUS_SUCCESS;
}

static int nrf_wifi_drv_main_zep(const struct device *dev)
{
#ifndef CONFIG_NRF70_RADIO_TEST
	struct nrf_wifi_fmac_callbk_fns callbk_fns = { 0 };
	struct nrf_wifi_data_config_params data_config = { 0 };
	struct rx_buf_pool_params rx_buf_pools[MAX_NUM_OF_RX_QUEUES];
	struct nrf_wifi_vif_ctx_zep *vif_ctx_zep = dev->data;

	vif_ctx_zep->rpu_ctx_zep = &rpu_drv_priv_zep.rpu_ctx_zep;

#ifdef CONFIG_NRF70_DATA_TX
	data_config.aggregation = aggregation;
	data_config.wmm = IS_ENABLED(CONFIG_NRF_WIFI_FEAT_WMM);
	data_config.max_num_tx_agg_sessions = max_num_tx_agg_sessions;
	data_config.max_num_rx_agg_sessions = max_num_rx_agg_sessions;
	data_config.max_tx_aggregation = max_tx_aggregation;
	data_config.reorder_buf_size = reorder_buf_size;
	data_config.max_rxampdu_size = max_rxampdu_size;
	data_config.rate_protection_type = rate_protection_type;
	callbk_fns.if_carr_state_chg_callbk_fn = nrf_wifi_if_carr_state_chg;
	callbk_fns.rx_frm_callbk_fn = nrf_wifi_if_rx_frm;
#endif
#if defined(CONFIG_NRF70_RAW_DATA_RX) || defined(CONFIG_NRF70_PROMISC_DATA_RX)
	callbk_fns.sniffer_callbk_fn = nrf_wifi_if_sniffer_rx_frm;
#endif /* CONFIG_NRF70_RAW_DATA_RX || CONFIG_NRF70_PROMISC_DATA_RX */
	rx_buf_pools[0].num_bufs = rx1_num_bufs;
	rx_buf_pools[1].num_bufs = rx2_num_bufs;
	rx_buf_pools[2].num_bufs = rx3_num_bufs;
	rx_buf_pools[0].buf_sz = rx1_buf_sz;
	rx_buf_pools[1].buf_sz = rx2_buf_sz;
	rx_buf_pools[2].buf_sz = rx3_buf_sz;

#ifdef CONFIG_NRF_WIFI_RPU_RECOVERY
	callbk_fns.rpu_recovery_callbk_fn = nrf_wifi_rpu_recovery_cb;
#endif /* CONFIG_NRF_WIFI_RPU_RECOVERY */
	callbk_fns.scan_start_callbk_fn = nrf_wifi_event_proc_scan_start_zep;
	callbk_fns.scan_done_callbk_fn = nrf_wifi_event_proc_scan_done_zep;
	callbk_fns.reg_change_callbk_fn = reg_change_callbk_fn;
	callbk_fns.event_get_reg = nrf_wifi_event_get_reg_zep;
#ifdef CONFIG_NET_L2_WIFI_MGMT
	callbk_fns.disp_scan_res_callbk_fn = nrf_wifi_event_proc_disp_scan_res_zep;
#endif /* CONFIG_NET_L2_WIFI_MGMT */
#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
	callbk_fns.rx_bcn_prb_resp_callbk_fn = nrf_wifi_rx_bcn_prb_resp_frm;
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
#ifdef CONFIG_NRF70_SYSTEM_MODE
	callbk_fns.set_if_callbk_fn = nrf_wifi_set_iface_event_handler;
#endif /* CONFIG_NRF70_SYSTEM_MODE */
#ifdef CONFIG_NRF70_STA_MODE
	callbk_fns.twt_config_callbk_fn = nrf_wifi_event_proc_twt_setup_zep;
	callbk_fns.twt_teardown_callbk_fn = nrf_wifi_event_proc_twt_teardown_zep;
	callbk_fns.twt_sleep_callbk_fn = nrf_wifi_event_proc_twt_sleep_zep;
	callbk_fns.event_get_ps_info = nrf_wifi_event_proc_get_power_save_info;
	callbk_fns.cookie_rsp_callbk_fn = nrf_wifi_event_proc_cookie_rsp;
	callbk_fns.process_rssi_from_rx = nrf_wifi_process_rssi_from_rx;
	callbk_fns.scan_res_callbk_fn = nrf_wifi_wpa_supp_event_proc_scan_res;
	callbk_fns.auth_resp_callbk_fn = nrf_wifi_wpa_supp_event_proc_auth_resp;
	callbk_fns.assoc_resp_callbk_fn = nrf_wifi_wpa_supp_event_proc_assoc_resp;
	callbk_fns.deauth_callbk_fn = nrf_wifi_wpa_supp_event_proc_deauth;
	callbk_fns.disassoc_callbk_fn = nrf_wifi_wpa_supp_event_proc_disassoc;
	callbk_fns.get_station_callbk_fn = nrf_wifi_wpa_supp_event_proc_get_sta;
	callbk_fns.get_interface_callbk_fn = nrf_wifi_wpa_supp_event_proc_get_if;
	callbk_fns.mgmt_tx_status = nrf_wifi_wpa_supp_event_mgmt_tx_status;
	callbk_fns.unprot_mlme_mgmt_rx_callbk_fn = nrf_wifi_wpa_supp_event_proc_unprot_mgmt;
	callbk_fns.event_get_wiphy = nrf_wifi_wpa_supp_event_get_wiphy;
	callbk_fns.mgmt_rx_callbk_fn = nrf_wifi_wpa_supp_event_mgmt_rx_callbk_fn;
	callbk_fns.get_conn_info_callbk_fn = nrf_wifi_supp_event_proc_get_conn_info;
#endif /* CONFIG_NRF70_STA_MODE */

	/* The OSAL layer needs to be initialized before any other initialization
	 * so that other layers (like FW IF,HW IF etc) have access to OS ops
	 */
	nrf_wifi_osal_init(&nrf_wifi_os_zep_ops);

	rpu_drv_priv_zep.fmac_priv = nrf_wifi_sys_fmac_init(&data_config,
							    rx_buf_pools,
							    &callbk_fns);
#else /* !CONFIG_NRF70_RADIO_TEST */
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;

	/* The OSAL layer needs to be initialized before any other initialization
	 * so that other layers (like FW IF,HW IF etc) have access to OS ops
	 */
	nrf_wifi_osal_init(&nrf_wifi_os_zep_ops);

	rpu_drv_priv_zep.fmac_priv = nrf_wifi_rt_fmac_init();
#endif /* CONFIG_NRF70_RADIO_TEST */

	if (rpu_drv_priv_zep.fmac_priv == NULL) {
		LOG_ERR("%s: nrf_wifi_fmac_init failed",
			__func__);
		goto err;
	}

#ifdef CONFIG_NRF70_DATA_TX
	struct nrf_wifi_sys_fmac_priv *sys_fpriv = NULL;

	sys_fpriv = wifi_fmac_priv(rpu_drv_priv_zep.fmac_priv);
	sys_fpriv->max_ampdu_len_per_token =
		(RPU_PKTRAM_SIZE - (CONFIG_NRF70_RX_NUM_BUFS * CONFIG_NRF70_RX_MAX_DATA_SIZE)) /
		CONFIG_NRF70_MAX_TX_TOKENS;
	/* Align to 4-byte */
	sys_fpriv->max_ampdu_len_per_token &= ~0x3;

	/* Alignment overhead for size based coalesce */
	sys_fpriv->avail_ampdu_len_per_token =
	sys_fpriv->max_ampdu_len_per_token -
		(MAX_PKT_RAM_TX_ALIGN_OVERHEAD * max_tx_aggregation);
#endif /* CONFIG_NRF70_DATA_TX */

#ifdef CONFIG_NRF70_RADIO_TEST
	status = nrf_wifi_fmac_dev_add_zep(&rpu_drv_priv_zep);
	if (status != NRF_WIFI_STATUS_SUCCESS) {
		LOG_ERR("%s: nrf_wifi_fmac_dev_add_zep failed", __func__);
		goto fmac_deinit;
	}
#else
	k_work_init_delayable(&vif_ctx_zep->scan_timeout_work,
			      nrf_wifi_scan_timeout_work);
	k_work_init(&vif_ctx_zep->disp_scan_res_work,
		    nrf_wifi_disp_scan_res_work_handler);
#endif /* CONFIG_NRF70_RADIO_TEST */

	k_mutex_init(&rpu_drv_priv_zep.rpu_ctx_zep.rpu_lock);
#ifndef CONFIG_NRF70_RADIO_TEST
	vif_ctx_zep->bss_max_idle_period = USHRT_MAX;
#endif /* !CONFIG_NRF70_RADIO_TEST */
	return 0;
#ifdef CONFIG_NRF70_RADIO_TEST
fmac_deinit:
	nrf_wifi_fmac_deinit(rpu_drv_priv_zep.fmac_priv);
	nrf_wifi_osal_deinit();
#endif /* CONFIG_NRF70_RADIO_TEST */
err:
	return -1;
}

#ifndef CONFIG_NRF70_RADIO_TEST
#ifdef CONFIG_NET_L2_WIFI_MGMT
static const struct wifi_mgmt_ops nrf_wifi_mgmt_ops = {
	.scan = nrf_wifi_disp_scan_zep,
#ifdef CONFIG_NET_STATISTICS_WIFI
	.get_stats = nrf_wifi_stats_get,
	.reset_stats = nrf_wifi_stats_reset,
#endif /* CONFIG_NET_STATISTICS_WIFI */
#if defined(CONFIG_NRF70_STA_MODE) || defined(CONFIG_NRF70_RAW_DATA_RX)
	.reg_domain = nrf_wifi_reg_domain,
#endif
#ifdef CONFIG_NRF70_STA_MODE
	.set_power_save = nrf_wifi_set_power_save,
	.set_twt = nrf_wifi_set_twt,
	.get_power_save_config = nrf_wifi_get_power_save_config,
	.set_rts_threshold = nrf_wifi_set_rts_threshold,
	.get_rts_threshold = nrf_wifi_get_rts_threshold,
	.set_bss_max_idle_period = nrf_wifi_set_bss_max_idle_period,
#endif
#ifdef CONFIG_NRF70_SYSTEM_WITH_RAW_MODES
	.mode = nrf_wifi_mode,
#endif
#if defined(CONFIG_NRF70_RAW_DATA_TX) || defined(CONFIG_NRF70_RAW_DATA_RX)
	.channel = nrf_wifi_channel,
#endif /* CONFIG_NRF70_RAW_DATA_TX || CONFIG_NRF70_RAW_DATA_RX */
#if defined(CONFIG_NRF70_RAW_DATA_RX) || defined(CONFIG_NRF70_PROMISC_DATA_RX)
	.filter = nrf_wifi_filter,
#endif /* CONFIG_NRF70_RAW_DATA_RX || CONFIG_NRF70_PROMISC_DATA_RX */
};
#endif /* CONFIG_NET_L2_WIFI_MGMT */



#ifdef CONFIG_NRF70_STA_MODE
static const struct zep_wpa_supp_dev_ops wpa_supp_ops = {
	.init = nrf_wifi_wpa_supp_dev_init,
	.deinit = nrf_wifi_wpa_supp_dev_deinit,
	.scan2 = nrf_wifi_wpa_supp_scan2,
	.scan_abort = nrf_wifi_wpa_supp_scan_abort,
	.get_scan_results2 = nrf_wifi_wpa_supp_scan_results_get,
	.deauthenticate = nrf_wifi_wpa_supp_deauthenticate,
	.authenticate = nrf_wifi_wpa_supp_authenticate,
	.associate = nrf_wifi_wpa_supp_associate,
	.set_supp_port = nrf_wifi_wpa_set_supp_port,
	.set_key = nrf_wifi_wpa_supp_set_key,
	.signal_poll = nrf_wifi_wpa_supp_signal_poll,
	.send_mlme = nrf_wifi_nl80211_send_mlme,
	.get_wiphy = nrf_wifi_supp_get_wiphy,
	.register_frame = nrf_wifi_supp_register_frame,
	.get_capa = nrf_wifi_supp_get_capa,
	.get_conn_info = nrf_wifi_supp_get_conn_info,
	.set_country = nrf_wifi_supp_set_country,
	.get_country = nrf_wifi_supp_get_country,
#ifdef CONFIG_NRF70_AP_MODE
	.init_ap = nrf_wifi_wpa_supp_init_ap,
	.start_ap = nrf_wifi_wpa_supp_start_ap,
	.change_beacon = nrf_wifi_wpa_supp_change_beacon,
	.stop_ap = nrf_wifi_wpa_supp_stop_ap,
	.deinit_ap = nrf_wifi_wpa_supp_deinit_ap,
	.sta_add = nrf_wifi_wpa_supp_sta_add,
	.sta_remove = nrf_wifi_wpa_supp_sta_remove,
	.register_mgmt_frame = nrf_wifi_supp_register_mgmt_frame,
	.sta_set_flags = nrf_wifi_wpa_supp_sta_set_flags,
	.get_inact_sec = nrf_wifi_wpa_supp_sta_get_inact_sec,
#endif /* CONFIG_NRF70_AP_MODE */
};
#endif /* CONFIG_NRF70_STA_MODE */
#endif /* !CONFIG_NRF70_RADIO_TEST */


#ifdef CONFIG_NET_L2_ETHERNET
static const struct net_wifi_mgmt_offload wifi_offload_ops = {
	.wifi_iface.iface_api.init = nrf_wifi_if_init_zep,
	.wifi_iface.start = nrf_wifi_if_start_zep,
	.wifi_iface.stop = nrf_wifi_if_stop_zep,
	.wifi_iface.set_config = nrf_wifi_if_set_config_zep,
	.wifi_iface.get_config = nrf_wifi_if_get_config_zep,
	.wifi_iface.get_capabilities = nrf_wifi_if_caps_get,
	.wifi_iface.send = nrf_wifi_if_send,
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	.wifi_iface.get_stats = nrf_wifi_eth_stats_get,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
#ifdef CONFIG_NET_L2_WIFI_MGMT
	.wifi_mgmt_api = &nrf_wifi_mgmt_ops,
#endif /* CONFIG_NET_L2_WIFI_MGMT */
#ifdef CONFIG_NRF70_STA_MODE
	.wifi_drv_ops = &wpa_supp_ops,
#endif /* CONFIG_NRF70_STA_MODE */
};
#endif /* CONFIG_NET_L2_ETHERNET */



#ifdef CONFIG_NET_L2_ETHERNET
ETH_NET_DEVICE_DT_INST_DEFINE(0,
		    nrf_wifi_drv_main_zep, /* init_fn */
		    NULL, /* pm_action_cb */
		    &rpu_drv_priv_zep.rpu_ctx_zep.vif_ctx_zep[0], /* data */
#ifdef CONFIG_NRF70_STA_MODE
		    &wpa_supp_ops, /* cfg */
#else /* CONFIG_NRF70_STA_MODE */
		    NULL, /* cfg */
#endif /* !CONFIG_NRF70_STA_MODE */
		    CONFIG_WIFI_INIT_PRIORITY, /* prio */
		    &wifi_offload_ops, /* api */
		    CONFIG_NRF_WIFI_IFACE_MTU); /*mtu */
#else
DEVICE_DT_INST_DEFINE(0,
	      nrf_wifi_drv_main_zep, /* init_fn */
	      NULL, /* pm_action_cb */
#ifndef CONFIG_NRF70_RADIO_TEST
	      &rpu_drv_priv_zep, /* data */
#else /* !CONFIG_NRF70_RADIO_TEST */
	      NULL,
#endif /* CONFIG_NRF70_RADIO_TEST */
	      NULL, /* cfg */
	      POST_KERNEL,
	      CONFIG_WIFI_INIT_PRIORITY, /* prio */
	      NULL); /* api */
#endif /* CONFIG_NET_L2_ETHERNET */

#ifdef CONFIG_NET_CONNECTION_MANAGER_CONNECTIVITY_WIFI_MGMT
CONNECTIVITY_WIFI_MGMT_BIND(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)));
#endif /* CONFIG_NET_CONNECTION_MANAGER_CONNECTIVITY_WIFI_MGMT */
