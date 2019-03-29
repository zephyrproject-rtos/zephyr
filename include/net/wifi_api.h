/*
 * @file
 * @brief WiFi manager APIs for the external caller
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_API_H_
#define ZEPHYR_INCLUDE_NET_WIFI_API_H_

#include <net/wifi_drv.h>

enum wifi_security {
	WIFI_SECURITY_UNKNOWN,
	WIFI_SECURITY_OPEN,
	WIFI_SECURITY_PSK,
	WIFI_SECURITY_OTHERS,
};

/**
 * @struct wifi_config
 * @brief Structure holding WiFi configuration.
 *
 * @var char wifi_config::ssid
 * SSID
 *
 * @var char wifi_config::bssid
 * BSSID
 *
 * @var char wifi_config::security
 * Security type
 *
 * @var char wifi_config::passphrase
 * Passphrase for WPA/WPA2-PSK
 *
 * @var unsigned char wifi_config::band
 * Band
 *
 * @var unsigned char wifi_config::channel
 * Channel number
 *
 * @var unsigned char wifi_config::ch_width
 * Channel width
 *
 * @var int wifi_config::autorun
 * Autorun switch & interval (in milliseconds)
 */
struct wifi_config {
	char ssid[WIFI_MAX_SSID_LEN + 1];
	char bssid[WIFI_MAC_ADDR_LEN];
	char security;
	char passphrase[WIFI_MAX_PSPHR_LEN + 1];
	unsigned char band;
	unsigned char channel;
	unsigned char ch_width;
	unsigned int autorun;
};

enum wifi_sta_state {
	WIFI_STATE_STA_UNAVAIL,
	WIFI_STATE_STA_READY,
	WIFI_STATE_STA_SCANNING,
	WIFI_STATE_STA_RTTING,
	WIFI_STATE_STA_CONNECTING,
	WIFI_STATE_STA_CONNECTED,
	WIFI_STATE_STA_DISCONNECTING,
};

enum wifi_ap_state {
	WIFI_STATE_AP_UNAVAIL,
	WIFI_STATE_AP_READY,
	WIFI_STATE_AP_STARTED,
};

/**
 * @struct wifi_status
 * @brief Structure holding WiFi status.
 *
 * @param char state
 * WiFi state
 *
 * @param char own_mac
 * Own MAC address
 *
 * @param char host_found
 * Indicate whether the specified AP is found
 *
 * @param char host_bssid
 * BSSID of the specified AP
 *
 * @param signed char host_rssi
 * Signal strength of the connected AP
 *
 * @param unsigned char nr_sta
 * Number of connected STAs
 *
 * @param char sta_mac_addrs
 * Array of connected MAC addresses
 *
 * @param unsigned char nr_acl
 * Number of ACL MAC addresses
 *
 * @param char acl_mac_addrs
 * Array of ACL MAC addresses
 */
struct wifi_status {
	char state;
	char own_mac[WIFI_MAC_ADDR_LEN];
	union {
		struct {
			char host_found;
			char host_bssid[WIFI_MAC_ADDR_LEN];
			signed char host_rssi;
		} sta;
		struct {
			unsigned char nr_sta;
			char (*sta_mac_addrs)[WIFI_MAC_ADDR_LEN];
			unsigned char nr_acl;
			char (*acl_mac_addrs)[WIFI_MAC_ADDR_LEN];
		} ap;
	} u;
};

/**
 * @struct wifi_scan_params
 * @brief Structure holding scan parameters.
 *
 * @var unsigned char wifi_scan_params::band
 * Band
 *
 * @var unsigned char wifi_scan_params::channel
 * Channel number
 *
 */
struct wifi_scan_params {
	unsigned char band;
	unsigned char channel;
};

/**
 * @struct wifi_scan_result
 * @brief Structure holding WiFi scan result.
 *
 * @var char wifi_scan_result::ssid
 * SSID
 *
 * @var char wifi_scan_result::bssid
 * BSSID
 *
 * @var unsigned char wifi_scan_result::band
 * Band
 *
 * @var unsigned char wifi_scan_result::channel
 * Channel number
 *
 * @var signed char wifi_scan_result::rssi
 * Signal strength
 *
 * @var enum wifi_security wifi_scan_result::security
 * Security type
 *
 * @var char wifi_scan_result::rtt_supported
 * Indicate whether RTT is supported
 */
struct wifi_scan_result {
	char ssid[WIFI_MAX_SSID_LEN];
	char bssid[WIFI_MAC_ADDR_LEN];
	unsigned char band;
	unsigned char channel;
	signed char rssi;
	enum wifi_security security;
	char rtt_supported;
};

/**
 * @struct wifi_rtt_request
 * @brief Structure holding RTT session.
 *
 * @var unsigned char wifi_rtt_request::nr_peers
 * Number of RTT peers
 *
 * @var struct wifi_rtt_request::peers
 * RTT range request parameters (@ref wifi_rtt_peers)
 */
struct wifi_rtt_request {
	unsigned char nr_peers;
	struct wifi_rtt_peers *peers;
};

/**
 * @struct wifi_rtt_response
 * @brief Structure holding RTT range response.
 *
 * @var char wifi_rtt_response::bssid
 * BSSID of RTT peer
 *
 * @var int wifi_rtt_response::range
 * RTT range
 */
struct wifi_rtt_response {
	char bssid[WIFI_MAC_ADDR_LEN];
	int range;
};

enum wifi_mac_acl_subcmd {
	WIFI_MAC_ACL_BLOCK = 1,
	WIFI_MAC_ACL_UNBLOCK,
	WIFI_MAC_ACL_BLOCK_ALL,
	WIFI_MAC_ACL_UNBLOCK_ALL,
};

/**
 * @struct wifi_notifier_val
 * @brief Union holding notifier value.
 *
 * @var char wifi_notifier_val::val_char
 * Notifier value
 *
 * @var unsigned char wifi_notifier_val::val_ptr
 * Pointer to notifier value buffer
 */
union wifi_notifier_val {
	char val_char;
	void *val_ptr;
};

/**
 * @typedef wifi_notifier_fn_t
 * @brief Callback type of notifier chain.
 *
 * A function of this type is given to the wifi_register_*_notifier() function
 * and will be called for any notify event.
 *
 * @param val Notifier value (@ref wifi_notifier_val).
 */
typedef void (*wifi_notifier_fn_t)(union wifi_notifier_val val);

/**
 * @brief Register a callback to the connection notifier chain.
 *
 * @param notifier_call Callback to notify any connect event.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_register_connection_notifier(wifi_notifier_fn_t notifier_call);

/**
 * @brief Unregister a callback to the connection notifier chain.
 *
 * @param notifier_call Callback to notify any connect event.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_unregister_connection_notifier(wifi_notifier_fn_t notifier_call);

/**
 * @brief Register a callback to the disconnection notifier chain.
 *
 * @param notifier_call Callback to notify any disconnect event.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_register_disconnection_notifier(wifi_notifier_fn_t notifier_call);

/**
 * @brief Unregister a callback to the disconnection notifier chain.
 *
 * @param notifier_call Callback to notify any disconnect event.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_unregister_disconnection_notifier(wifi_notifier_fn_t notifier_call);

/**
 * @brief Register a callback to the new station notifier chain.
 *
 * @param notifier_call Callback to notify any new station event.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_register_new_station_notifier(wifi_notifier_fn_t notifier_call);

/**
 * @brief Unregister a callback to the new station notifier chain.
 *
 * @param notifier_call Callback to notify any new station event.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_unregister_new_station_notifier(wifi_notifier_fn_t notifier_call);

/**
 * @brief Register a callback to the station leave notifier chain.
 *
 * @param notifier_call Callback to notify any station leave event.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_register_station_leave_notifier(wifi_notifier_fn_t notifier_call);

/**
 * @brief Unregister a callback to the station leave notifier chain.
 *
 * @param notifier_call Callback to notify any station leave event.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_unregister_station_leave_notifier(wifi_notifier_fn_t notifier_call);

/**
 * @typedef scan_res_cb_t
 * @brief Callback type for reporting scan results.
 *
 * A function of this type is given to the wifi_sta_scan() function
 * and will be called for any discovered Access Point.
 *
 * @param res A scan result.
 */
typedef void (*scan_res_cb_t)(struct wifi_scan_result *res);

/**
 * @typedef rtt_resp_cb_t
 * @brief Callback type for reporting RTT range response.
 *
 * A function of this type is given to the wifi_sta_rtt_request() function
 * and will be called for any received RTT range response.
 *
 * @param resp A RTT range response.
 */
typedef void (*rtt_resp_cb_t)(struct wifi_rtt_response *resp);


/**
 * @brief Set STA configuration.
 *
 * This function set the STA configuration,
 * and store it to non-volatile memory using setting subsystem.
 *
 * @param conf STA configuration (@ref wifi_config).
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_set_conf(struct wifi_config *conf);

/**
 * @brief Clear STA configuration.
 *
 * This function clear the STA configuration,
 * and wipe it from non-volatile memory using setting subsystem.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_clear_conf(void);

/**
 * @brief Get STA configuration.
 *
 * This function get the STA configuration
 * from non-volatile memory using setting subsystem.
 *
 * @param conf STA configuration (@ref wifi_config).
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_get_conf(struct wifi_config *conf);

/**
 * @brief Get STA capability.
 *
 * This function get the STA capability.
 *
 * @param capa STA capability (@ref wifi_drv_capa).
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_get_capa(union wifi_drv_capa *capa);

/**
 * @brief Get STA status.
 *
 * This function get the STA status.
 *
 * @param sts STA status (@ref wifi_status).
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_get_status(struct wifi_status *sts);

/**
 * @brief Open STA.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_open(void);

/**
 * @brief Close STA.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_close(void);

/**
 * @brief STA scan.
 *
 * @param params Scan parameters, NULL for all (@ref wifi_scan_params).
 * @param cb Callback to invoke for each scan result (@ref scan_res_cb_t).
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_scan(struct wifi_scan_params *params, scan_res_cb_t cb);

/**
 * @brief Request RTT range.
 *
 * @param req RTT range request parameters (@ref wifi_rtt_request).
 * @param cb Callback to invoke for each RTT range response
 * (@ref rtt_resp_cb_t).
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_rtt_request(struct wifi_rtt_request *req, rtt_resp_cb_t cb);

/**
 * @brief Connect to the specified BSS/ESS
 *
 * This function connect STA to the BSS/ESS
 * with the parameter specified by wifi_sta_set_conf().
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_connect(void);

/**
 * @brief Disconnect from the BSS/ESS..
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_sta_disconnect(void);

/**
 * @brief Set AP configuration.
 *
 * This function set the STA configuration,
 * and store it to non-volatile memory using setting subsystem.
 *
 * @param conf AP configuration (@ref wifi_config).
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_set_conf(struct wifi_config *conf);

/**
 * @brief Clear AP configuration.
 *
 * This function clear the STA configuration,
 * and wipe it from non-volatile memory using setting subsystem.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_clear_conf(void);

/**
 * @brief Get AP configuration.
 *
 * This function get the STA configuration
 * from non-volatile memory using setting subsystem.
 *
 * @param conf AP configuration (@ref wifi_config).
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_get_conf(struct wifi_config *conf);

/**
 * @brief Get AP capability.
 *
 * This function get the STA capability.
 *
 * @param capa AP capability (@ref wifi_drv_capa).
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_get_capa(union wifi_drv_capa *capa);

/**
 * @brief Get AP status.
 *
 * This function get the STA status.
 *
 * @param sts AP status (@ref wifi_status).
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_get_status(struct wifi_status *sts);

/**
 * @brief Open AP.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_open(void);

/**
 * @brief Close AP.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_close(void);

/**
 * @brief Start acting in AP mode defined by the parameters.
 *
 * This function start AP mode
 * with the parameter specified by wifi_ap_set_conf().
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_start_ap(void);

/**
 * @brief Stop being an AP, including stopping beaconing.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_stop_ap(void);

/**
 * @brief Remove a STA.
 *
 * @param mac MAC address of the STA to be removed.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_del_station(char *mac);

/**
 * @brief Sets MAC address control list in AP mode.
 *
 * @param subcmd Subcommand (@ref wifi_mac_acl_subcmd).
 * @param mac ACL MAC address, NULL for all.
 *
 * @retval 0 on success, non-zero on failure.
 */
int wifi_ap_set_mac_acl(char subcmd, char *mac);

#ifndef MAC2STR
#define MAC2STR(m)	(m)[0], (m)[1], (m)[2], (m)[3], (m)[4], (m)[5]
#define MACSTR		"%02x:%02x:%02x:%02x:%02x:%02x"
#endif

static inline const char *security2str(int security)
{
	char *str = NULL;

	switch (security) {
	case WIFI_SECURITY_UNKNOWN:
		str = "UNKNOWN\t";
		break;
	case WIFI_SECURITY_OPEN:
		str = "OPEN\t";
		break;
	case WIFI_SECURITY_PSK:
		str = "WPA/WPA2";
		break;
	case WIFI_SECURITY_OTHERS:
		str = "OTHERS\t";
		break;
	default:
		break;
	}
	return str;
}

static inline const char *ap_sts2str(int state)
{
	char *str = NULL;

	switch (state) {
	case WIFI_STATE_AP_UNAVAIL:
		str = "AP <UNAVAILABLE>";
		break;
	case WIFI_STATE_AP_READY:
		str = "AP <READY>";
		break;
	case WIFI_STATE_AP_STARTED:
		str = "AP <STARTED>";
		break;
	default:
		str = "AP <UNKNOWN>";
		break;
	}
	return str;
}

static inline const char *sta_sts2str(int state)
{
	char *str = NULL;

	switch (state) {
	case WIFI_STATE_STA_UNAVAIL:
		str = "STA <UNAVAIL>";
		break;
	case WIFI_STATE_STA_READY:
		str = "STA <READY>";
		break;
	case WIFI_STATE_STA_SCANNING:
		str = "STA <SCANNING>";
		break;
	case WIFI_STATE_STA_RTTING:
		str = "STA <RTTING>";
		break;
	case WIFI_STATE_STA_CONNECTING:
		str = "STA <CONNECTING>";
		break;
	case WIFI_STATE_STA_CONNECTED:
		str = "STA <CONNECTED>";
		break;
	case WIFI_STATE_STA_DISCONNECTING:
		str = "STA <DISCONNECTING>";
		break;
	default:
		str = "STA <UNKNOWN>";
		break;
	}
	return str;
}

#endif
