/*
 * Copyright (c) 2018 Texas Instruments, Incorporated
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.11 protocol and general Wi-Fi definitions.
 */

/**
 * @defgroup wifi_mgmt Wi-Fi Management
 * @brief Wi-Fi Management API.
 * @ingroup networking
 * @{
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_H_
#define ZEPHYR_INCLUDE_NET_WIFI_H_

#include <zephyr/sys/util.h>  /* for ARRAY_SIZE */

/** Length of the country code string */
#define WIFI_COUNTRY_CODE_LEN 2

/** @cond INTERNAL_HIDDEN */

#define WIFI_LISTEN_INTERVAL_MIN 0
#define WIFI_LISTEN_INTERVAL_MAX 65535

/** @endcond */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief IEEE 802.11 security types. */
enum wifi_security_type {
	/** No security. */
	WIFI_SECURITY_TYPE_NONE = 0,
	/** WPA2-PSK security. */
	WIFI_SECURITY_TYPE_PSK,
	/** WPA2-PSK-SHA256 security. */
	WIFI_SECURITY_TYPE_PSK_SHA256,
	/** WPA3-SAE security. */
	WIFI_SECURITY_TYPE_SAE,
	/** GB 15629.11-2003 WAPI security. */
	WIFI_SECURITY_TYPE_WAPI,
	/** WEP security. */
	WIFI_SECURITY_TYPE_WEP,
	/** WPA-PSK security. */
	WIFI_SECURITY_TYPE_WPA_PSK,
	/** WPA/WPA2/WPA3 PSK security. */
	WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL,
	/** DPP security */
	WIFI_SECURITY_TYPE_DPP,
	/** EAP TLS security - Enterprise. */
	WIFI_SECURITY_TYPE_EAP_TLS,
	/** EAP PEAP MSCHAPV2 security - Enterprise. */
	WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2,
	/** EAP PEAP GTC security - Enterprise. */
	WIFI_SECURITY_TYPE_EAP_PEAP_GTC,
	/** EAP TTLS MSCHAPV2 security - Enterprise. */
	WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2,
	/** EAP PEAP TLS security - Enterprise. */
	WIFI_SECURITY_TYPE_EAP_PEAP_TLS,
	/** FT-PSK security */
	WIFI_SECURITY_TYPE_FT_PSK,
	/** FT-SAE security */
	WIFI_SECURITY_TYPE_FT_SAE,
	/** FT-EAP security */
	WIFI_SECURITY_TYPE_FT_EAP,
	/** FT-EAP-SHA384 security */
	WIFI_SECURITY_TYPE_FT_EAP_SHA384,

/** @cond INTERNAL_HIDDEN */
	__WIFI_SECURITY_TYPE_AFTER_LAST,
	WIFI_SECURITY_TYPE_MAX = __WIFI_SECURITY_TYPE_AFTER_LAST - 1,
	WIFI_SECURITY_TYPE_UNKNOWN
/** @endcond */
};

enum wifi_eap_type {
	WIFI_EAP_TYPE_NONE = 0,
	WIFI_EAP_TYPE_IDENTITY = 1,
	WIFI_EAP_TYPE_NOTIFICATION = 2,
	WIFI_EAP_TYPE_NAK = 3,
	WIFI_EAP_TYPE_MD5 = 4,
	WIFI_EAP_TYPE_OTP = 5,
	WIFI_EAP_TYPE_GTC = 6,
	WIFI_EAP_TYPE_TLS = 13,
	WIFI_EAP_TYPE_LEAP = 17,
	WIFI_EAP_TYPE_SIM = 18,
	WIFI_EAP_TYPE_TTLS = 21,
	WIFI_EAP_TYPE_AKA = 23,
	WIFI_EAP_TYPE_PEAP = 25,
	WIFI_EAP_TYPE_MSCHAPV2 = 26,
	WIFI_EAP_TYPE_TLV = 33,
	WIFI_EAP_TYPE_TNC = 38,
	WIFI_EAP_TYPE_FAST = 43,
	WIFI_EAP_TYPE_PAX = 46,
	WIFI_EAP_TYPE_PSK = 47,
	WIFI_EAP_TYPE_SAKE = 48,
	WIFI_EAP_TYPE_IKEV2 = 49,
	WIFI_EAP_TYPE_AKA_PRIME = 50,
	WIFI_EAP_TYPE_GPSK = 51,
	WIFI_EAP_TYPE_PWD = 52,
	WIFI_EAP_TYPE_EKE = 53,
	WIFI_EAP_TYPE_TEAP = 55,
	WIFI_EAP_TYPE_EXPANDED = 254,
};

/** suiteb types. */
enum wifi_suiteb_type {
	/** suiteb. */
	WIFI_SUITEB = 1,
	/** suiteb-192. */
	WIFI_SUITEB_192,
};

enum wifi_eap_tls_cipher_type {
	/** EAP TLS with NONE */
	WIFI_EAP_TLS_NONE,
	/** EAP TLS with ECDH & ECDSA with p384 */
	WIFI_EAP_TLS_ECC_P384,
	/** EAP TLS with ECDH & RSA with > 3K */
	WIFI_EAP_TLS_RSA_3K,
};

/** gropu cipher and pairwise cipher types. */
enum wifi_cipher_type {
    WPA_CAPA_ENC_WEP40,
    WPA_CAPA_ENC_WEP104,
    WPA_CAPA_ENC_TKIP,
    WPA_CAPA_ENC_CCMP,
    WPA_CAPA_ENC_WEP128,
    WPA_CAPA_ENC_GCMP,
    WPA_CAPA_ENC_GCMP_256,
    WPA_CAPA_ENC_CCMP_256,
};

/** gropu mgmt cipher types. */
enum wifi_group_mgmt_cipher_type {
    WPA_CAPA_ENC_BIP,
    WPA_CAPA_ENC_BIP_GMAC_128,
    WPA_CAPA_ENC_BIP_GMAC_256,
    WPA_CAPA_ENC_BIP_CMAC_256,
};

struct wifi_cipher_desc {
	unsigned int capa;
	char *name;
};

struct wifi_eap_config {
	unsigned int type;
	enum wifi_eap_type eap_type_phase1;
	enum wifi_eap_type eap_type_phase2;
	char *method;
	char *phase2;
};

/** Helper function to get user-friendly security type name. */
const char *wifi_security_txt(enum wifi_security_type security);

/** @brief IEEE 802.11w - Management frame protection. */
enum wifi_mfp_options {
	/** MFP disabled. */
	WIFI_MFP_DISABLE = 0,
	/** MFP optional. */
	WIFI_MFP_OPTIONAL,
	/** MFP required. */
	WIFI_MFP_REQUIRED,

/** @cond INTERNAL_HIDDEN */
	__WIFI_MFP_AFTER_LAST,
	WIFI_MFP_MAX = __WIFI_MFP_AFTER_LAST - 1,
	WIFI_MFP_UNKNOWN
/** @endcond */
};

/** Helper function to get user-friendly MFP name.*/
const char *wifi_mfp_txt(enum wifi_mfp_options mfp);

/**
 * @brief IEEE 802.11 operational frequency bands (not exhaustive).
 */
enum wifi_frequency_bands {
	/** 2.4 GHz band. */
	WIFI_FREQ_BAND_2_4_GHZ = 0,
	/** 5 GHz band. */
	WIFI_FREQ_BAND_5_GHZ,
	/** 6 GHz band (Wi-Fi 6E, also extends to 7GHz). */
	WIFI_FREQ_BAND_6_GHZ,

	/** Number of frequency bands available. */
	__WIFI_FREQ_BAND_AFTER_LAST,
	/** Highest frequency band available. */
	WIFI_FREQ_BAND_MAX = __WIFI_FREQ_BAND_AFTER_LAST - 1,
	/** Invalid frequency band */
	WIFI_FREQ_BAND_UNKNOWN
};

/** Helper function to get user-friendly frequency band name. */
const char *wifi_band_txt(enum wifi_frequency_bands band);

/** Max SSID length */
#define WIFI_SSID_MAX_LEN 32
/** Minimum PSK length */
#define WIFI_PSK_MIN_LEN 8
/** Maximum PSK length */
#define WIFI_PSK_MAX_LEN 64
/** Max SAW password length */
#define WIFI_SAE_PSWD_MAX_LEN 128
/** MAC address length */
#define WIFI_MAC_ADDR_LEN 6
#define WIFI_IDENTITY_MAX_LEN 64
#define WIFI_PSWD_MAX_LEN 128
#define WIFI_IDENTITY_MAX_USERS 8

/** Minimum channel number */
#define WIFI_CHANNEL_MIN 1
/** Maximum channel number */
#define WIFI_CHANNEL_MAX 233
/** Any channel number */
#define WIFI_CHANNEL_ANY 255

/** @brief Wi-Fi interface states.
 *
 * Based on https://w1.fi/wpa_supplicant/devel/defs_8h.html#a4aeb27c1e4abd046df3064ea9756f0bc
 */
enum wifi_iface_state {
	/** Interface is disconnected. */
	WIFI_STATE_DISCONNECTED = 0,
	/** Interface is disabled (administratively). */
	WIFI_STATE_INTERFACE_DISABLED,
	/** No enabled networks in the configuration. */
	WIFI_STATE_INACTIVE,
	/** Interface is scanning for networks. */
	WIFI_STATE_SCANNING,
	/** Authentication with a network is in progress. */
	WIFI_STATE_AUTHENTICATING,
	/** Association with a network is in progress. */
	WIFI_STATE_ASSOCIATING,
	/** Association with a network completed. */
	WIFI_STATE_ASSOCIATED,
	/** 4-way handshake with a network is in progress. */
	WIFI_STATE_4WAY_HANDSHAKE,
	/** Group Key exchange with a network is in progress. */
	WIFI_STATE_GROUP_HANDSHAKE,
	/** All authentication completed, ready to pass data. */
	WIFI_STATE_COMPLETED,

/** @cond INTERNAL_HIDDEN */
	__WIFI_STATE_AFTER_LAST,
	WIFI_STATE_MAX = __WIFI_STATE_AFTER_LAST - 1,
	WIFI_STATE_UNKNOWN
/** @endcond */
};

/* We rely on the strict order of the enum values, so, let's check it */
BUILD_ASSERT(WIFI_STATE_DISCONNECTED < WIFI_STATE_INTERFACE_DISABLED &&
	     WIFI_STATE_INTERFACE_DISABLED < WIFI_STATE_INACTIVE &&
	     WIFI_STATE_INACTIVE < WIFI_STATE_SCANNING &&
	     WIFI_STATE_SCANNING < WIFI_STATE_AUTHENTICATING &&
	     WIFI_STATE_AUTHENTICATING < WIFI_STATE_ASSOCIATING &&
	     WIFI_STATE_ASSOCIATING < WIFI_STATE_ASSOCIATED &&
	     WIFI_STATE_ASSOCIATED < WIFI_STATE_4WAY_HANDSHAKE &&
	     WIFI_STATE_4WAY_HANDSHAKE < WIFI_STATE_GROUP_HANDSHAKE &&
	     WIFI_STATE_GROUP_HANDSHAKE < WIFI_STATE_COMPLETED);


/** Helper function to get user-friendly interface state name. */
const char *wifi_state_txt(enum wifi_iface_state state);

/** @brief Wi-Fi interface modes.
 *
 * Based on https://w1.fi/wpa_supplicant/devel/defs_8h.html#a4aeb27c1e4abd046df3064ea9756f0bc
 */
enum wifi_iface_mode {
	/** Infrastructure station mode. */
	WIFI_MODE_INFRA = 0,
	/** IBSS (ad-hoc) station mode. */
	WIFI_MODE_IBSS = 1,
	/** AP mode. */
	WIFI_MODE_AP = 2,
	/** P2P group owner mode. */
	WIFI_MODE_P2P_GO = 3,
	/** P2P group formation mode. */
	WIFI_MODE_P2P_GROUP_FORMATION = 4,
	/** 802.11s Mesh mode. */
	WIFI_MODE_MESH = 5,

/** @cond INTERNAL_HIDDEN */
	__WIFI_MODE_AFTER_LAST,
	WIFI_MODE_MAX = __WIFI_MODE_AFTER_LAST - 1,
	WIFI_MODE_UNKNOWN
/** @endcond */
};

/** Helper function to get user-friendly interface mode name. */
const char *wifi_mode_txt(enum wifi_iface_mode mode);

/** @brief Wi-Fi link operating modes
 *
 * As per https://en.wikipedia.org/wiki/Wi-Fi#Versions_and_generations.
 */
enum wifi_link_mode {
	/** 802.11 (legacy). */
	WIFI_0 = 0,
	/** 802.11b. */
	WIFI_1,
	/** 802.11a. */
	WIFI_2,
	/** 802.11g. */
	WIFI_3,
	/** 802.11n. */
	WIFI_4,
	/** 802.11ac. */
	WIFI_5,
	/** 802.11ax. */
	WIFI_6,
	/** 802.11ax 6GHz. */
	WIFI_6E,
	/** 802.11be. */
	WIFI_7,

/** @cond INTERNAL_HIDDEN */
	__WIFI_LINK_MODE_AFTER_LAST,
	WIFI_LINK_MODE_MAX = __WIFI_LINK_MODE_AFTER_LAST - 1,
	WIFI_LINK_MODE_UNKNOWN
/** @endcond */
};

/** Helper function to get user-friendly link mode name. */
const char *wifi_link_mode_txt(enum wifi_link_mode link_mode);

/** @brief Wi-Fi scanning types. */
enum wifi_scan_type {
	/** Active scanning (default). */
	WIFI_SCAN_TYPE_ACTIVE = 0,
	/** Passive scanning. */
	WIFI_SCAN_TYPE_PASSIVE,
};

/** @brief Wi-Fi power save states. */
enum wifi_ps {
	/** Power save disabled. */
	WIFI_PS_DISABLED = 0,
	/** Power save enabled. */
	WIFI_PS_ENABLED,
};

/** Helper function to get user-friendly ps name. */
const char *wifi_ps_txt(enum wifi_ps ps_name);

/** @brief Wi-Fi power save modes. */
enum wifi_ps_mode {
	/** Legacy power save mode. */
	WIFI_PS_MODE_LEGACY = 0,
	/* This has to be configured before connecting to the AP,
	 * as support for ADDTS action frames is not available.
	 */
	/** WMM power save mode. */
	WIFI_PS_MODE_WMM,
};

/** Helper function to get user-friendly ps mode name. */
const char *wifi_ps_mode_txt(enum wifi_ps_mode ps_mode);

/** Network interface index min value */
#define WIFI_INTERFACE_INDEX_MIN 1
/** Network interface index max value */
#define WIFI_INTERFACE_INDEX_MAX 255

/** @brief Wifi operational mode */
enum wifi_operational_modes {
	/** STA mode setting enable */
	WIFI_STA_MODE = BIT(0),
	/** Monitor mode setting enable */
	WIFI_MONITOR_MODE = BIT(1),
	/** TX injection mode setting enable */
	WIFI_TX_INJECTION_MODE = BIT(2),
	/** Promiscuous mode setting enable */
	WIFI_PROMISCUOUS_MODE = BIT(3),
	/** AP mode setting enable */
	WIFI_AP_MODE = BIT(4),
	/** Softap mode setting enable */
	WIFI_SOFTAP_MODE = BIT(5),
};

/** @brief Mode filter settings */
enum wifi_filter {
	/** Support management, data and control packet sniffing */
	WIFI_PACKET_FILTER_ALL = BIT(0),
	/** Support only sniffing of management packets */
	WIFI_PACKET_FILTER_MGMT = BIT(1),
	/** Support only sniffing of data packets */
	WIFI_PACKET_FILTER_DATA = BIT(2),
	/** Support only sniffing of control packets */
	WIFI_PACKET_FILTER_CTRL = BIT(3),
};

/** @brief Wi-Fi Target Wake Time (TWT) operations. */
enum wifi_twt_operation {
	/** TWT setup operation */
	WIFI_TWT_SETUP = 0,
	/** TWT teardown operation */
	WIFI_TWT_TEARDOWN,
};

/** Helper function to get user-friendly twt operation name. */
const char *wifi_twt_operation_txt(enum wifi_twt_operation twt_operation);

/** @brief Wi-Fi Target Wake Time (TWT) negotiation types. */
enum wifi_twt_negotiation_type {
	/** TWT individual negotiation */
	WIFI_TWT_INDIVIDUAL = 0,
	/** TWT broadcast negotiation */
	WIFI_TWT_BROADCAST,
	/** TWT wake TBTT negotiation */
	WIFI_TWT_WAKE_TBTT
};

/** Helper function to get user-friendly twt negotiation type name. */
const char *wifi_twt_negotiation_type_txt(enum wifi_twt_negotiation_type twt_negotiation);

/** @brief Wi-Fi Target Wake Time (TWT) setup commands. */
enum wifi_twt_setup_cmd {
	/** TWT setup request */
	WIFI_TWT_SETUP_CMD_REQUEST = 0,
	/** TWT setup suggest (parameters can be changed by AP) */
	WIFI_TWT_SETUP_CMD_SUGGEST,
	/** TWT setup demand (parameters can not be changed by AP) */
	WIFI_TWT_SETUP_CMD_DEMAND,
	/** TWT setup grouping (grouping of TWT flows) */
	WIFI_TWT_SETUP_CMD_GROUPING,
	/** TWT setup accept (parameters accepted by AP) */
	WIFI_TWT_SETUP_CMD_ACCEPT,
	/** TWT setup alternate (alternate parameters suggested by AP) */
	WIFI_TWT_SETUP_CMD_ALTERNATE,
	/** TWT setup dictate (parameters dictated by AP) */
	WIFI_TWT_SETUP_CMD_DICTATE,
	/** TWT setup reject (parameters rejected by AP) */
	WIFI_TWT_SETUP_CMD_REJECT,
};

/** Helper function to get user-friendly twt setup cmd name. */
const char *wifi_twt_setup_cmd_txt(enum wifi_twt_setup_cmd twt_setup);

/** @brief Wi-Fi Target Wake Time (TWT) negotiation status. */
enum wifi_twt_setup_resp_status {
	/** TWT response received for TWT request */
	WIFI_TWT_RESP_RECEIVED = 0,
	/** TWT response not received for TWT request */
	WIFI_TWT_RESP_NOT_RECEIVED,
};

/** @brief Target Wake Time (TWT) error codes. */
enum wifi_twt_fail_reason {
	/** Unspecified error */
	WIFI_TWT_FAIL_UNSPECIFIED,
	/** Command execution failed */
	WIFI_TWT_FAIL_CMD_EXEC_FAIL,
	/** Operation not supported */
	WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED,
	/** Unable to get interface status */
	WIFI_TWT_FAIL_UNABLE_TO_GET_IFACE_STATUS,
	/** Device not connected to AP */
	WIFI_TWT_FAIL_DEVICE_NOT_CONNECTED,
	/** Peer not HE (802.11ax/Wi-Fi 6) capable */
	WIFI_TWT_FAIL_PEER_NOT_HE_CAPAB,
	/** Peer not TWT capable */
	WIFI_TWT_FAIL_PEER_NOT_TWT_CAPAB,
	/** A TWT flow is already in progress */
	WIFI_TWT_FAIL_OPERATION_IN_PROGRESS,
	/** Invalid negotiated flow id */
	WIFI_TWT_FAIL_INVALID_FLOW_ID,
	/** IP address not assigned or configured */
	WIFI_TWT_FAIL_IP_NOT_ASSIGNED,
	/** Flow already exists */
	WIFI_TWT_FAIL_FLOW_ALREADY_EXISTS,
};

/** @brief Wi-Fi Target Wake Time (TWT) teradown status. */
enum wifi_twt_teardown_status {
	/** TWT teardown success */
	WIFI_TWT_TEARDOWN_SUCCESS = 0,
	/** TWT teardown failure */
	WIFI_TWT_TEARDOWN_FAILED,
};

/** @cond INTERNAL_HIDDEN */
static const char * const wifi_twt_err_code_tbl[] = {
	[WIFI_TWT_FAIL_UNSPECIFIED] = "Unspecified",
	[WIFI_TWT_FAIL_CMD_EXEC_FAIL] = "Command Execution failed",
	[WIFI_TWT_FAIL_OPERATION_NOT_SUPPORTED] =
		"Operation not supported",
	[WIFI_TWT_FAIL_UNABLE_TO_GET_IFACE_STATUS] =
		"Unable to get iface status",
	[WIFI_TWT_FAIL_DEVICE_NOT_CONNECTED] =
		"Device not connected",
	[WIFI_TWT_FAIL_PEER_NOT_HE_CAPAB] = "Peer not HE capable",
	[WIFI_TWT_FAIL_PEER_NOT_TWT_CAPAB] = "Peer not TWT capable",
	[WIFI_TWT_FAIL_OPERATION_IN_PROGRESS] =
		"Operation already in progress",
	[WIFI_TWT_FAIL_INVALID_FLOW_ID] =
		"Invalid negotiated flow id",
	[WIFI_TWT_FAIL_IP_NOT_ASSIGNED] =
		"IP address not assigned",
	[WIFI_TWT_FAIL_FLOW_ALREADY_EXISTS] =
		"Flow already exists",
};
/** @endcond */

/** Helper function to get user-friendly TWT error code name. */
static inline const char *wifi_twt_get_err_code_str(int16_t err_no)
{
	if ((err_no) < (int16_t)ARRAY_SIZE(wifi_twt_err_code_tbl)) {
		return wifi_twt_err_code_tbl[err_no];
	}

	return "<unknown>";
}

/** @brief Wi-Fi power save parameters. */
enum wifi_ps_param_type {
	/** Power save state. */
	WIFI_PS_PARAM_STATE,
	/** Power save listen interval. */
	WIFI_PS_PARAM_LISTEN_INTERVAL,
	/** Power save wakeup mode. */
	WIFI_PS_PARAM_WAKEUP_MODE,
	/** Power save mode. */
	WIFI_PS_PARAM_MODE,
	/** Power save timeout. */
	WIFI_PS_PARAM_TIMEOUT,
};

/** @brief Wi-Fi power save modes. */
enum wifi_ps_wakeup_mode {
	/** DTIM based wakeup. */
	WIFI_PS_WAKEUP_MODE_DTIM = 0,
	/** Listen interval based wakeup. */
	WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL,
};

/** Helper function to get user-friendly ps wakeup mode name. */
const char *wifi_ps_wakeup_mode_txt(enum wifi_ps_wakeup_mode ps_wakeup_mode);

/** @brief Wi-Fi power save error codes. */
enum wifi_config_ps_param_fail_reason {
	/** Unspecified error */
	WIFI_PS_PARAM_FAIL_UNSPECIFIED,
	/** Command execution failed */
	WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL,
	/** Parameter not supported */
	WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED,
	/** Unable to get interface status */
	WIFI_PS_PARAM_FAIL_UNABLE_TO_GET_IFACE_STATUS,
	/** Device not connected to AP */
	WIFI_PS_PARAM_FAIL_DEVICE_NOT_CONNECTED,
	/** Device already connected to AP */
	WIFI_PS_PARAM_FAIL_DEVICE_CONNECTED,
	/** Listen interval out of range */
	WIFI_PS_PARAM_LISTEN_INTERVAL_RANGE_INVALID,
};

/** @cond INTERNAL_HIDDEN */
static const char * const wifi_ps_param_config_err_code_tbl[] = {
	[WIFI_PS_PARAM_FAIL_UNSPECIFIED] = "Unspecified",
	[WIFI_PS_PARAM_FAIL_CMD_EXEC_FAIL] = "Command Execution failed",
	[WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED] =
		"Operation not supported",
	[WIFI_PS_PARAM_FAIL_UNABLE_TO_GET_IFACE_STATUS] =
		"Unable to get iface status",
	[WIFI_PS_PARAM_FAIL_DEVICE_NOT_CONNECTED] =
		"Cannot set parameters while device not connected",
	[WIFI_PS_PARAM_FAIL_DEVICE_CONNECTED] =
		"Cannot set parameters while device connected",
	[WIFI_PS_PARAM_LISTEN_INTERVAL_RANGE_INVALID] =
		"Parameter out of range",
};
/** @endcond */

/** 11v BSS transition management Query reasons. */
enum wifi_btm_query_reason {
	/** Unspecified. */
	WIFI_BTM_QUERY_REASON_UNDPECIFIED = 0,
	/** Low RSSI. */
	WIFI_BTM_QUERY_REASON_LOW_RSSI = 16,	
	/** Leaving ESS. */
	WIFI_BTM_QUERY_REASON_LEAVING_ESS=20,
};

/** Helper function to get user-friendly power save error code name. */
static inline const char *wifi_ps_get_config_err_code_str(int16_t err_no)
{
	if ((err_no) < (int16_t)ARRAY_SIZE(wifi_ps_param_config_err_code_tbl)) {
		return wifi_ps_param_config_err_code_tbl[err_no];
	}

	return "<unknown>";
}

/** @brief Wi-Fi AP mode configuration parameter */
enum wifi_ap_config_param {
	/** Used for AP mode configuration parameter ap_max_inactivity */
	WIFI_AP_CONFIG_PARAM_MAX_INACTIVITY = BIT(0),
	/** Used for AP mode configuration parameter max_num_sta */
	WIFI_AP_CONFIG_PARAM_MAX_NUM_STA = BIT(1),
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_NET_WIFI_H_ */
