/*
 * Copyright (c) 2018 Texas Instruments, Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.11 protocol and general Wi-Fi definitions.
 */

/**
 * @addtogroup wifi_mgmt
 * @{
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_H_
#define ZEPHYR_INCLUDE_NET_WIFI_H_

#include <zephyr/sys/util.h>  /* for ARRAY_SIZE */

#define WIFI_COUNTRY_CODE_LEN 2

#define WIFI_LISTEN_INTERVAL_MIN 0
#define WIFI_LISTEN_INTERVAL_MAX 65535

/** IEEE 802.11 security types. */
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
	/** EAP security - Enterprise. */
	WIFI_SECURITY_TYPE_EAP,
	/** WEP security. */
	WIFI_SECURITY_TYPE_WEP,
	/** WPA-PSK security. */
	WIFI_SECURITY_TYPE_WPA_PSK,

	__WIFI_SECURITY_TYPE_AFTER_LAST,
	WIFI_SECURITY_TYPE_MAX = __WIFI_SECURITY_TYPE_AFTER_LAST - 1,
	WIFI_SECURITY_TYPE_UNKNOWN
};

/** Helper function to get user-friendly security type name. */
static inline const char *wifi_security_txt(enum wifi_security_type security)
{
	switch (security) {
	case WIFI_SECURITY_TYPE_NONE:
		return "OPEN";
	case WIFI_SECURITY_TYPE_WEP:
		return "WEP";
	case WIFI_SECURITY_TYPE_WPA_PSK:
		return "WPA-PSK";
	case WIFI_SECURITY_TYPE_PSK:
		return "WPA2-PSK";
	case WIFI_SECURITY_TYPE_PSK_SHA256:
		return "WPA2-PSK-SHA256";
	case WIFI_SECURITY_TYPE_SAE:
		return "WPA3-SAE";
	case WIFI_SECURITY_TYPE_WAPI:
		return "WAPI";
	case WIFI_SECURITY_TYPE_EAP:
		return "EAP";
	case WIFI_SECURITY_TYPE_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

/** IEEE 802.11w - Management frame protection. */
enum wifi_mfp_options {
	/** MFP disabled. */
	WIFI_MFP_DISABLE = 0,
	/** MFP optional. */
	WIFI_MFP_OPTIONAL,
	/** MFP required. */
	WIFI_MFP_REQUIRED,

	__WIFI_MFP_AFTER_LAST,
	WIFI_MFP_MAX = __WIFI_MFP_AFTER_LAST - 1,
	WIFI_MFP_UNKNOWN
};

/** Helper function to get user-friendly MFP name.*/
static inline const char *wifi_mfp_txt(enum wifi_mfp_options mfp)
{
	switch (mfp) {
	case WIFI_MFP_DISABLE:
		return "Disable";
	case WIFI_MFP_OPTIONAL:
		return "Optional";
	case WIFI_MFP_REQUIRED:
		return "Required";
	case WIFI_MFP_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

/** IEEE 802.11 operational frequency bands (not exhaustive). */
enum wifi_frequency_bands {
	/** 2.4GHz band. */
	WIFI_FREQ_BAND_2_4_GHZ = 0,
	/** 5GHz band. */
	WIFI_FREQ_BAND_5_GHZ,
	/** 6GHz band (Wi-Fi 6E, also extends to 7GHz). */
	WIFI_FREQ_BAND_6_GHZ,

	__WIFI_FREQ_BAND_AFTER_LAST,
	WIFI_FREQ_BAND_MAX = __WIFI_FREQ_BAND_AFTER_LAST - 1,
	WIFI_FREQ_BAND_UNKNOWN
};

/** Helper function to get user-friendly frequency band name. */
static inline const char *wifi_band_txt(enum wifi_frequency_bands band)
{
	switch (band) {
	case WIFI_FREQ_BAND_2_4_GHZ:
		return "2.4GHz";
	case WIFI_FREQ_BAND_5_GHZ:
		return "5GHz";
	case WIFI_FREQ_BAND_6_GHZ:
		return "6GHz";
	case WIFI_FREQ_BAND_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PSK_MIN_LEN 8
#define WIFI_PSK_MAX_LEN 64
#define WIFI_SAE_PSWD_MAX_LEN 128
#define WIFI_MAC_ADDR_LEN 6

#define WIFI_CHANNEL_MAX 233
#define WIFI_CHANNEL_ANY 255

/** Wi-Fi interface states.
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

	__WIFI_STATE_AFTER_LAST,
	WIFI_STATE_MAX = __WIFI_STATE_AFTER_LAST - 1,
	WIFI_STATE_UNKNOWN
};

/** Helper function to get user-friendly interface state name. */
static inline const char *wifi_state_txt(enum wifi_iface_state state)
{
	switch (state) {
	case WIFI_STATE_DISCONNECTED:
		return "DISCONNECTED";
	case WIFI_STATE_INACTIVE:
		return "INACTIVE";
	case WIFI_STATE_INTERFACE_DISABLED:
		return "INTERFACE_DISABLED";
	case WIFI_STATE_SCANNING:
		return "SCANNING";
	case WIFI_STATE_AUTHENTICATING:
		return "AUTHENTICATING";
	case WIFI_STATE_ASSOCIATING:
		return "ASSOCIATING";
	case WIFI_STATE_ASSOCIATED:
		return "ASSOCIATED";
	case WIFI_STATE_4WAY_HANDSHAKE:
		return "4WAY_HANDSHAKE";
	case WIFI_STATE_GROUP_HANDSHAKE:
		return "GROUP_HANDSHAKE";
	case WIFI_STATE_COMPLETED:
		return "COMPLETED";
	case WIFI_STATE_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

/** Wi-Fi interface modes.
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

	__WIFI_MODE_AFTER_LAST,
	WIFI_MODE_MAX = __WIFI_MODE_AFTER_LAST - 1,
	WIFI_MODE_UNKNOWN
};

/** Helper function to get user-friendly interface mode name. */
static inline const char *wifi_mode_txt(enum wifi_iface_mode mode)
{
	switch (mode) {
	case WIFI_MODE_INFRA:
		return "STATION";
	case WIFI_MODE_IBSS:
		return "ADHOC";
	case WIFI_MODE_AP:
		return "ACCESS POINT";
	case WIFI_MODE_P2P_GO:
		return "P2P GROUP OWNER";
	case WIFI_MODE_P2P_GROUP_FORMATION:
		return "P2P GROUP FORMATION";
	case WIFI_MODE_MESH:
		return "MESH";
	case WIFI_MODE_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

/** Wi-Fi link operating modes
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

	__WIFI_LINK_MODE_AFTER_LAST,
	WIFI_LINK_MODE_MAX = __WIFI_LINK_MODE_AFTER_LAST - 1,
	WIFI_LINK_MODE_UNKNOWN
};

/** Helper function to get user-friendly link mode name. */
static inline const char *wifi_link_mode_txt(enum wifi_link_mode link_mode)
{
	switch (link_mode) {
	case WIFI_0:
		return "WIFI 0 (802.11)";
	case WIFI_1:
		return "WIFI 1 (802.11b)";
	case WIFI_2:
		return "WIFI 2 (802.11a)";
	case WIFI_3:
		return "WIFI 3 (802.11g)";
	case WIFI_4:
		return "WIFI 4 (802.11n/HT)";
	case WIFI_5:
		return "WIFI 5 (802.11ac/VHT)";
	case WIFI_6:
		return "WIFI 6 (802.11ax/HE)";
	case WIFI_6E:
		return "WIFI 6E (802.11ax 6GHz/HE)";
	case WIFI_7:
		return "WIFI 7 (802.11be/EHT)";
	case WIFI_LINK_MODE_UNKNOWN:
	default:
		return "UNKNOWN";
	}
}

/** Wi-Fi scanning types. */
enum wifi_scan_type {
	/** Active scanning (default). */
	WIFI_SCAN_TYPE_ACTIVE = 0,
	/** Passive scanning. */
	WIFI_SCAN_TYPE_PASSIVE,
};

/** Wi-Fi power save states. */
enum wifi_ps {
	/** Power save disabled. */
	WIFI_PS_DISABLED = 0,
	/** Power save enabled. */
	WIFI_PS_ENABLED,
};

static const char * const wifi_ps2str[] = {
	[WIFI_PS_DISABLED] = "Power save disabled",
	[WIFI_PS_ENABLED] = "Power save enabled",
};

/** Wi-Fi power save modes. */
enum wifi_ps_mode {
	/** Legacy power save mode. */
	WIFI_PS_MODE_LEGACY = 0,
	/* This has to be configured before connecting to the AP,
	 * as support for ADDTS action frames is not available.
	 */
	/** WMM power save mode. */
	WIFI_PS_MODE_WMM,
};

static const char * const wifi_ps_mode2str[] = {
	[WIFI_PS_MODE_LEGACY] = "Legacy power save",
	[WIFI_PS_MODE_WMM] = "WMM power save",
};

/** Wi-Fi Target Wake Time (TWT) operations. */
enum wifi_twt_operation {
	/** TWT setup operation */
	WIFI_TWT_SETUP = 0,
	/** TWT teardown operation */
	WIFI_TWT_TEARDOWN,
};

static const char * const wifi_twt_operation2str[] = {
	[WIFI_TWT_SETUP] = "TWT setup",
	[WIFI_TWT_TEARDOWN] = "TWT teardown",
};

/** Wi-Fi Target Wake Time (TWT) negotiation types. */
enum wifi_twt_negotiation_type {
	/** TWT individual negotiation */
	WIFI_TWT_INDIVIDUAL = 0,
	/** TWT broadcast negotiation */
	WIFI_TWT_BROADCAST,
	/** TWT wake TBTT negotiation */
	WIFI_TWT_WAKE_TBTT
};

static const char * const wifi_twt_negotiation_type2str[] = {
	[WIFI_TWT_INDIVIDUAL] = "TWT individual negotiation",
	[WIFI_TWT_BROADCAST] = "TWT broadcast negotiation",
	[WIFI_TWT_WAKE_TBTT] = "TWT wake TBTT negotiation",
};

/** Wi-Fi Target Wake Time (TWT) setup commands. */
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

static const char * const wifi_twt_setup_cmd2str[] = {
	[WIFI_TWT_SETUP_CMD_REQUEST] = "TWT request",
	[WIFI_TWT_SETUP_CMD_SUGGEST] = "TWT suggest",
	[WIFI_TWT_SETUP_CMD_DEMAND] = "TWT demand",
	[WIFI_TWT_SETUP_CMD_GROUPING] = "TWT grouping",
	[WIFI_TWT_SETUP_CMD_ACCEPT] = "TWT accept",
	[WIFI_TWT_SETUP_CMD_ALTERNATE] = "TWT alternate",
	[WIFI_TWT_SETUP_CMD_DICTATE] = "TWT dictate",
	[WIFI_TWT_SETUP_CMD_REJECT] = "TWT reject",
};

/** Wi-Fi Target Wake Time (TWT) negotiation status. */
enum wifi_twt_setup_resp_status {
	/** TWT response received for TWT request */
	WIFI_TWT_RESP_RECEIVED = 0,
	/** TWT response not received for TWT request */
	WIFI_TWT_RESP_NOT_RECEIVED,
};

/** Target Wake Time (TWT) error codes. */
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

static const char * const twt_err_code_tbl[] = {
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

/** Helper function to get user-friendly TWT error code name. */
static inline const char *get_twt_err_code_str(int16_t err_no)
{
	if ((err_no) < (int16_t)ARRAY_SIZE(twt_err_code_tbl)) {
		return twt_err_code_tbl[err_no];
	}

	return "<unknown>";
}

/** Wi-Fi power save parameters. */
enum ps_param_type {
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

/** Wi-Fi power save modes. */
enum wifi_ps_wakeup_mode {
	/** DTIM based wakeup. */
	WIFI_PS_WAKEUP_MODE_DTIM = 0,
	/** Listen interval based wakeup. */
	WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL,
};

static const char * const wifi_ps_wakeup_mode2str[] = {
	[WIFI_PS_WAKEUP_MODE_DTIM] = "PS wakeup mode DTIM",
	[WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL] = "PS wakeup mode listen interval",
};

/** Wi-Fi power save error codes. */
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

static const char * const ps_param_config_err_code_tbl[] = {
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

/** Helper function to get user-friendly power save error code name. */
static inline const char *get_ps_config_err_code_str(int16_t err_no)
{
	if ((err_no) < (int16_t)ARRAY_SIZE(ps_param_config_err_code_tbl)) {
		return ps_param_config_err_code_tbl[err_no];
	}

	return "<unknown>";
}

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_NET_WIFI_H_ */
