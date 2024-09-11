/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing WiFi management operation implementations
 * for the Zephyr OS.
 */

#ifndef __ZEPHYR_WIFI_MGMT_H__
#define __ZEPHYR_WIFI_MGMT_H__
#include <math.h>

#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>

#include "osal_api.h"

/** Filter setting defines for sniffer mode. */
#define WIFI_MGMT_DATA_CTRL_FILTER_SETTING 0xE
#define WIFI_ALL_FILTER_SETTING 0xF

struct twt_interval_float {
	unsigned short mantissa;
	unsigned char exponent;
};

int nrf_wifi_set_power_save(const struct device *dev,
			    struct wifi_ps_params *params);

int nrf_wifi_set_twt(const struct device *dev,
	struct wifi_twt_params *twt_params);

void nrf_wifi_event_proc_twt_setup_zep(void *vif_ctx,
		struct nrf_wifi_umac_cmd_config_twt *twt_setup_info,
		unsigned int event_len);

void nrf_wifi_event_proc_twt_teardown_zep(void *vif_ctx,
		struct nrf_wifi_umac_cmd_teardown_twt *twt_teardown_info,
		unsigned int event_len);

void nrf_wifi_event_proc_twt_sleep_zep(void *vif_ctx,
		struct nrf_wifi_umac_event_twt_sleep *twt_sleep_info,
		unsigned int event_len);

int nrf_wifi_twt_teardown_flows(struct nrf_wifi_vif_ctx_zep *vif_ctx_zep,
		unsigned char start_flow_id, unsigned char end_flow_id);

int nrf_wifi_get_power_save_config(const struct device *dev,
		struct wifi_ps_config *ps_config);

void nrf_wifi_event_proc_get_power_save_info(void *vif_ctx,
		struct nrf_wifi_umac_event_power_save_info *ps_info,
		unsigned int event_len);

#ifdef CONFIG_NRF70_SYSTEM_WITH_RAW_MODES
int nrf_wifi_mode(const struct device *dev,
		  struct wifi_mode_info *mode);
#endif

#if defined(CONFIG_NRF70_RAW_DATA_TX) || defined(CONFIG_NRF70_RAW_DATA_RX)
int nrf_wifi_channel(const struct device *dev,
		     struct wifi_channel_info *channel);
#endif /* CONFIG_NRF70_RAW_DATA_TX || CONFIG_NRF70_RAW_DATA_RX */

#if defined(CONFIG_NRF70_RAW_DATA_RX) || defined(CONFIG_NRF70_PROMISC_DATA_RX)
int nrf_wifi_filter(const struct device *dev,
		    struct wifi_filter_info *filter);
#endif /* CONFIG_NRF70_RAW_DATA_RX || CONFIG_NRF70_PROMISC_DATA_RX */

int nrf_wifi_set_rts_threshold(const struct device *dev,
			       unsigned int rts_threshold);

int nrf_wifi_get_rts_threshold(const struct device *dev,
			       unsigned int *rts_threshold);
#endif /*  __ZEPHYR_WIFI_MGMT_H__ */
