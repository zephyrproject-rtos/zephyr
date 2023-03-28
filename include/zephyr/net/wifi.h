/*
 * Copyright (c) 2018 Texas Instruments, Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief General WiFi Definitions
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_H_
#define ZEPHYR_INCLUDE_NET_WIFI_H_

#define WIFI_COUNTRY_CODE_LEN 2

/* Not having support for legacy types is deliberate to enforce
 * higher security.
 */
enum wifi_security_type {
	WIFI_SECURITY_TYPE_NONE = 0,
	WIFI_SECURITY_TYPE_WEP,
	WIFI_SECURITY_TYPE_WPA_PSK,
	WIFI_SECURITY_TYPE_PSK,
	WIFI_SECURITY_TYPE_PSK_SHA256,
	WIFI_SECURITY_TYPE_SAE,
	WIFI_SECURITY_TYPE_WAPI,
	WIFI_SECURITY_TYPE_EAP,

	__WIFI_SECURITY_TYPE_AFTER_LAST,
	WIFI_SECURITY_TYPE_MAX = __WIFI_SECURITY_TYPE_AFTER_LAST - 1,
	WIFI_SECURITY_TYPE_UNKNOWN
};

/**
 * wifi_security_txt - Get the security type as a text string
 */
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

/* Management frame protection (IEEE 802.11w) options */
enum wifi_mfp_options {
	WIFI_MFP_DISABLE = 0,
	WIFI_MFP_OPTIONAL,
	WIFI_MFP_REQUIRED,

	__WIFI_MFP_AFTER_LAST,
	WIFI_MFP_MAX = __WIFI_MFP_AFTER_LAST - 1,
	WIFI_MFP_UNKNOWN
};

/**
 * wifi_mfp_txt - Get the MFP as a text string
 */
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

enum wifi_frequency_bands {
	WIFI_FREQ_BAND_2_4_GHZ = 0,
	WIFI_FREQ_BAND_5_GHZ,
	WIFI_FREQ_BAND_6_GHZ,

	__WIFI_FREQ_BAND_AFTER_LAST,
	WIFI_FREQ_BAND_MAX = __WIFI_FREQ_BAND_AFTER_LAST - 1,
	WIFI_FREQ_BAND_UNKNOWN
};

/**
 * wifi_mode_txt - Get the interface mode type as a text string
 */
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
#define WIFI_PSK_MAX_LEN 64
#define WIFI_MAC_ADDR_LEN 6

#define WIFI_CHANNEL_MAX 233
#define WIFI_CHANNEL_ANY 255

/* Based on https://w1.fi/wpa_supplicant/devel/defs_8h.html#a4aeb27c1e4abd046df3064ea9756f0bc */
enum wifi_iface_state {
	WIFI_STATE_DISCONNECTED = 0,
	WIFI_STATE_INTERFACE_DISABLED,
	WIFI_STATE_INACTIVE,
	WIFI_STATE_SCANNING,
	WIFI_STATE_AUTHENTICATING,
	WIFI_STATE_ASSOCIATING,
	WIFI_STATE_ASSOCIATED,
	WIFI_STATE_4WAY_HANDSHAKE,
	WIFI_STATE_GROUP_HANDSHAKE,
	WIFI_STATE_COMPLETED,

	__WIFI_STATE_AFTER_LAST,
	WIFI_STATE_MAX = __WIFI_STATE_AFTER_LAST - 1,
	WIFI_STATE_UNKNOWN
};

/**
 * wifi_state_txt - Get the connection state name as a text string
 */
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

/* Based on https://w1.fi/wpa_supplicant/devel/structwpa__ssid.html#a625821e2acfc9014f3b3de6e6593ffb7 */
enum wifi_iface_mode {
	WIFI_MODE_INFRA = 0,
	WIFI_MODE_IBSS = 1,
	WIFI_MODE_AP = 2,
	WIFI_MODE_P2P_GO = 3,
	WIFI_MODE_P2P_GROUP_FORMATION = 4,
	WIFI_MODE_MESH = 5,

	__WIFI_MODE_AFTER_LAST,
	WIFI_MODE_MAX = __WIFI_MODE_AFTER_LAST - 1,
	WIFI_MODE_UNKNOWN
};

/**
 * wifi_mode_txt - Get the interface mode type as a text string
 */
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

/* As per https://en.wikipedia.org/wiki/Wi-Fi#Versions_and_generations */
enum wifi_link_mode {
	WIFI_0 = 0,
	WIFI_1,
	WIFI_2,
	WIFI_3,
	WIFI_4,
	WIFI_5,
	WIFI_6,
	WIFI_6E,
	WIFI_7,

	__WIFI_LINK_MODE_AFTER_LAST,
	WIFI_LINK_MODE_MAX = __WIFI_LINK_MODE_AFTER_LAST - 1,
	WIFI_LINK_MODE_UNKNOWN
};

/**
 * wifi_link_mode_txt - Get the link mode type as a text string
 */
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

enum wifi_ps {
	WIFI_PS_DISABLED = 0,
	WIFI_PS_ENABLED,
};

static const char * const wifi_ps2str[] = {
	[WIFI_PS_DISABLED] = "Power save disabled",
	[WIFI_PS_ENABLED] = "Power save enabled",
};

enum wifi_ps_mode {
	WIFI_PS_MODE_LEGACY = 0,
	/* This has to be configured before connecting to the AP,
	 * as support for ADDTS action frames is not available.
	 */
	WIFI_PS_MODE_WMM,
};

static const char * const wifi_ps_mode2str[] = {
	[WIFI_PS_MODE_LEGACY] = "Legacy power save",
	[WIFI_PS_MODE_WMM] = "WMM power save",
};

enum wifi_twt_operation {
	WIFI_TWT_SETUP = 0,
	WIFI_TWT_TEARDOWN,
};

static const char * const wifi_twt_operation2str[] = {
	[WIFI_TWT_SETUP] = "TWT setup",
	[WIFI_TWT_TEARDOWN] = "TWT teardown",
};

enum wifi_twt_negotiation_type {
	WIFI_TWT_INDIVIDUAL = 0,
	WIFI_TWT_BROADCAST,
	WIFI_TWT_WAKE_TBTT
};

static const char * const wifi_twt_negotiation_type2str[] = {
	[WIFI_TWT_INDIVIDUAL] = "TWT individual negotiation",
	[WIFI_TWT_BROADCAST] = "TWT broadcast negotiation",
	[WIFI_TWT_WAKE_TBTT] = "TWT wake TBTT negotiation",
};

enum wifi_twt_setup_cmd {
	/* TWT Requests */
	WIFI_TWT_SETUP_CMD_REQUEST = 0,
	WIFI_TWT_SETUP_CMD_SUGGEST,
	WIFI_TWT_SETUP_CMD_DEMAND,
	/* TWT Responses */
	WIFI_TWT_SETUP_CMD_GROUPING,
	WIFI_TWT_SETUP_CMD_ACCEPT,
	WIFI_TWT_SETUP_CMD_ALTERNATE,
	WIFI_TWT_SETUP_CMD_DICTATE,
	WIFI_TWT_SETUP_CMD_REJECT,
};

static const char * const wifi_twt_setup_cmd2str[] = {
	/* TWT Requests */
	[WIFI_TWT_SETUP_CMD_REQUEST] = "TWT request",
	[WIFI_TWT_SETUP_CMD_SUGGEST] = "TWT suggest",
	[WIFI_TWT_SETUP_CMD_DEMAND] = "TWT demand",
	/* TWT Responses */
	[WIFI_TWT_SETUP_CMD_GROUPING] = "TWT grouping",
	[WIFI_TWT_SETUP_CMD_ACCEPT] = "TWT accept",
	[WIFI_TWT_SETUP_CMD_ALTERNATE] = "TWT alternate",
	[WIFI_TWT_SETUP_CMD_DICTATE] = "TWT dictate",
	[WIFI_TWT_SETUP_CMD_REJECT] = "TWT reject",
};

enum wifi_twt_setup_resp_status {
	/* TWT Setup response status */
	WIFI_TWT_RESP_RECEIVED = 0,
	WIFI_TWT_RESP_NOT_RECEIVED,
};

enum wifi_extended_ps {
	WIFI_EXTENDED_PS_DISABLED = 0,
	WIFI_EXTENDED_PS_ENABLED,
};

static const char * const wifi_extended_ps_2str[] = {
	[WIFI_EXTENDED_PS_DISABLED] = "Extended ps disabled",
	[WIFI_EXTENDED_PS_ENABLED] = "Extended ps enabled",
};
#endif /* ZEPHYR_INCLUDE_NET_WIFI_H_ */
