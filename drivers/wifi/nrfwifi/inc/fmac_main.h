/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing FMAC interface specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#ifndef __ZEPHYR_FMAC_MAIN_H__
#define __ZEPHYR_FMAC_MAIN_H__

#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/net/net_if.h>
#ifndef CONFIG_NRF70_RADIO_TEST
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/ethernet.h>
#ifdef CONFIG_NETWORKING
#include <net_if.h>
#endif /* CONFIG_NETWORKING */
#ifdef CONFIG_NRF70_STA_MODE
#include <drivers/driver_zephyr.h>
#endif /* CONFIG_NRF70_STA_MODE */
#endif /* !CONFIG_NRF70_RADIO_TEST */

#include <fmac_api.h>
#include <host_rpu_umac_if.h>

#define NRF70_DRIVER_VERSION "1."KERNEL_VERSION_STRING

#ifndef CONFIG_NRF70_RADIO_TEST
struct nrf_wifi_vif_ctx_zep {
	const struct device *zep_dev_ctx;
	struct net_if *zep_net_if_ctx;
	void *supp_drv_if_ctx;
	void *rpu_ctx_zep;
	unsigned char vif_idx;
	struct k_mutex vif_lock;

	scan_result_cb_t disp_scan_cb;
	bool scan_in_progress;
	int scan_type;
	uint16_t max_bss_cnt;
	unsigned int scan_res_cnt;
	struct k_work_delayable scan_timeout_work;

	struct net_eth_addr mac_addr;
	int if_type;
	char ifname[16];
	enum nrf_wifi_fmac_if_op_state if_op_state;
	bool set_if_event_received;
	int set_if_status;
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	struct net_stats_eth eth_stats;
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
#ifdef CONFIG_NRF70_STA_MODE
	unsigned int assoc_freq;
	enum nrf_wifi_fmac_if_carr_state if_carr_state;
	struct wpa_signal_info *signal_info;
	struct wpa_conn_info *conn_info;
	struct zep_wpa_supp_dev_callbk_fns supp_callbk_fns;
	unsigned char twt_flows_map;
	unsigned char twt_flow_in_progress_map;
	struct wifi_ps_config *ps_info;
	bool ps_config_info_evnt;
	bool authorized;
	bool cookie_resp_received;
#ifdef CONFIG_NRF70_DATA_TX
	struct k_work nrf_wifi_net_iface_work;
#endif /* CONFIG_NRF70_DATA_TX */
	unsigned long rssi_record_timestamp_us;
	signed short rssi;
#endif /* CONFIG_NRF70_STA_MODE */
#ifdef CONFIG_NRF70_AP_MODE
	int inactive_time_sec;
#endif /* CONFIG_NRF70_AP_MODE */
#ifdef CONFIG_NRF_WIFI_RPU_RECOVERY
	struct k_work nrf_wifi_rpu_recovery_work;
#endif /* CONFIG_NRF_WIFI_RPU_RECOVERY */
};

struct nrf_wifi_vif_ctx_map {
	const char *ifname;
	struct nrf_wifi_vif_ctx_zep *vif_ctx;
};
#endif /* !CONFIG_NRF70_RADIO_TEST */

struct nrf_wifi_ctx_zep {
	void *drv_priv_zep;
	void *rpu_ctx;
#ifdef CONFIG_NRF70_RADIO_TEST
	struct rpu_conf_params conf_params;
	bool rf_test_run;
	unsigned char rf_test;
#else /* CONFIG_NRF70_RADIO_TEST */
	struct nrf_wifi_vif_ctx_zep vif_ctx_zep[MAX_NUM_VIFS];
#ifdef CONFIG_NRF70_UTIL
	struct rpu_conf_params conf_params;
#endif /* CONFIG_NRF70_UTIL */
#endif /* CONFIG_NRF70_RADIO_TEST */
	unsigned char *extended_capa, *extended_capa_mask;
	unsigned int extended_capa_len;
	struct k_mutex rpu_lock;
};

struct nrf_wifi_drv_priv_zep {
	struct nrf_wifi_fmac_priv *fmac_priv;
	/* TODO: Replace with a linked list to handle unlimited RPUs */
	struct nrf_wifi_ctx_zep rpu_ctx_zep;
};

extern struct nrf_wifi_drv_priv_zep rpu_drv_priv_zep;

void nrf_wifi_scan_timeout_work(struct k_work *work);
void configure_tx_pwr_settings(struct nrf_wifi_tx_pwr_ctrl_params *tx_pwr_ctrl_params,
				struct nrf_wifi_tx_pwr_ceil_params *tx_pwr_ceil_params);
void configure_board_dep_params(struct nrf_wifi_board_params *board_params);
void set_tx_pwr_ceil_default(struct nrf_wifi_tx_pwr_ceil_params *pwr_ceil_params);
const char *nrf_wifi_get_drv_version(void);
enum nrf_wifi_status nrf_wifi_fmac_dev_add_zep(struct nrf_wifi_drv_priv_zep *drv_priv_zep);
enum nrf_wifi_status nrf_wifi_fmac_dev_rem_zep(struct nrf_wifi_drv_priv_zep *drv_priv_zep);
enum nrf_wifi_status nrf_wifi_fw_load(void *rpu_ctx);
struct nrf_wifi_vif_ctx_zep *nrf_wifi_get_vif_ctx(struct net_if *iface);
void nrf_wifi_rpu_recovery_cb(void *vif_ctx,
		void *event_data,
		unsigned int event_len);

#endif /* __ZEPHYR_FMAC_MAIN_H__ */
