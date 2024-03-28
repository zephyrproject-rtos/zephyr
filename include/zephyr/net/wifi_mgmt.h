/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WiFi L2 stack public header
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_MGMT_H_
#define ZEPHYR_INCLUDE_NET_WIFI_MGMT_H_

#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/offloaded_netdev.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup wifi_mgmt
 * @{
 */

/* Management part definitions */

#define _NET_WIFI_LAYER	NET_MGMT_LAYER_L2
#define _NET_WIFI_CODE	0x156
#define _NET_WIFI_BASE	(NET_MGMT_IFACE_BIT |			\
			 NET_MGMT_LAYER(_NET_WIFI_LAYER) |	\
			 NET_MGMT_LAYER_CODE(_NET_WIFI_CODE))
#define _NET_WIFI_EVENT	(_NET_WIFI_BASE | NET_MGMT_EVENT_BIT)

#ifdef CONFIG_WIFI_MGMT_SCAN_SSID_FILT_MAX
#define WIFI_MGMT_SCAN_SSID_FILT_MAX CONFIG_WIFI_MGMT_SCAN_SSID_FILT_MAX
#else
#define WIFI_MGMT_SCAN_SSID_FILT_MAX 1
#endif /* CONFIG_WIFI_MGMT_SCAN_SSID_FILT_MAX */

#ifdef CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL
#define WIFI_MGMT_SCAN_CHAN_MAX_MANUAL CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL
#else
#define WIFI_MGMT_SCAN_CHAN_MAX_MANUAL 1
#endif /* CONFIG_WIFI_MGMT_SCAN_CHAN_MAX_MANUAL */

#define WIFI_MGMT_BAND_STR_SIZE_MAX 8
#define WIFI_MGMT_SCAN_MIN_DWELL_TIME_ACTIVE 5
#define WIFI_MGMT_SCAN_MAX_DWELL_TIME_ACTIVE 1000
#define WIFI_MGMT_SCAN_MIN_DWELL_TIME_PASSIVE 10
#define WIFI_MGMT_SCAN_MAX_DWELL_TIME_PASSIVE 1000
#define WIFI_MGMT_SCAN_DEFAULT_DWELL_TIME_ACTIVE 50
#define WIFI_MGMT_SCAN_DEFAULT_DWELL_TIME_PASSIVE 130
#define WIFI_MGMT_SCAN_MAX_BSS_CNT 65535

/** Wi-Fi management commands */
enum net_request_wifi_cmd {
	/** Scan for Wi-Fi networks */
	NET_REQUEST_WIFI_CMD_SCAN = 1,
	/** Connect to a Wi-Fi network */
	NET_REQUEST_WIFI_CMD_CONNECT,
	/** Disconnect from a Wi-Fi network */
	NET_REQUEST_WIFI_CMD_DISCONNECT,
	/** Enable AP mode */
	NET_REQUEST_WIFI_CMD_AP_ENABLE,
	/** Disable AP mode */
	NET_REQUEST_WIFI_CMD_AP_DISABLE,
	/** Get interface status */
	NET_REQUEST_WIFI_CMD_IFACE_STATUS,
	/** Set power save status */
	NET_REQUEST_WIFI_CMD_PS,
	/** Set power save mode */
	NET_REQUEST_WIFI_CMD_PS_MODE,
	/** Setup or teardown TWT flow */
	NET_REQUEST_WIFI_CMD_TWT,
	/** Get power save config */
	NET_REQUEST_WIFI_CMD_PS_CONFIG,
	/** Set or get regulatory domain */
	NET_REQUEST_WIFI_CMD_REG_DOMAIN,
	/** Set power save timeout */
	NET_REQUEST_WIFI_CMD_PS_TIMEOUT,
	/** Set or get Mode of operation */
	NET_REQUEST_WIFI_CMD_MODE,
	/** Set or get packet filter setting for current mode */
	NET_REQUEST_WIFI_CMD_PACKET_FILTER,
	/** Set or get Wi-Fi channel for Monitor or TX-Injection mode */
	NET_REQUEST_WIFI_CMD_CHANNEL,
	/** Disconnect a STA from AP */
	NET_REQUEST_WIFI_CMD_AP_STA_DISCONNECT,
	/** Get Wi-Fi driver and Firmware versions */
	NET_REQUEST_WIFI_CMD_VERSION,
	NET_REQUEST_WIFI_CMD_MAX
};

#define NET_REQUEST_WIFI_SCAN					\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_SCAN)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_SCAN);

#define NET_REQUEST_WIFI_CONNECT				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_CONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT);

#define NET_REQUEST_WIFI_DISCONNECT				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_DISCONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_DISCONNECT);

#define NET_REQUEST_WIFI_AP_ENABLE				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_AP_ENABLE)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_ENABLE);

#define NET_REQUEST_WIFI_AP_DISABLE				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_AP_DISABLE)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_DISABLE);

#define NET_REQUEST_WIFI_IFACE_STATUS				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_IFACE_STATUS)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_IFACE_STATUS);

#define NET_REQUEST_WIFI_PS				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_PS)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_PS);

#define NET_REQUEST_WIFI_PS_MODE			\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_PS_MODE)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_PS_MODE);

#define NET_REQUEST_WIFI_TWT			\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_TWT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_TWT);

#define NET_REQUEST_WIFI_PS_CONFIG				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_PS_CONFIG)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_PS_CONFIG);
#define NET_REQUEST_WIFI_REG_DOMAIN				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_REG_DOMAIN)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_REG_DOMAIN);

#define NET_REQUEST_WIFI_PS_TIMEOUT			\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_PS_TIMEOUT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_PS_TIMEOUT);

#define NET_REQUEST_WIFI_MODE				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_MODE)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_MODE);

#define NET_REQUEST_WIFI_PACKET_FILTER			\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_PACKET_FILTER)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_PACKET_FILTER);

#define NET_REQUEST_WIFI_CHANNEL			\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_CHANNEL)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CHANNEL);

#define NET_REQUEST_WIFI_AP_STA_DISCONNECT			\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_AP_STA_DISCONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_AP_STA_DISCONNECT);

#define NET_REQUEST_WIFI_VERSION			\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_VERSION)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_VERSION);

/** Wi-Fi management events */
enum net_event_wifi_cmd {
	/** Scan results available */
	NET_EVENT_WIFI_CMD_SCAN_RESULT = 1,
	/** Scan done */
	NET_EVENT_WIFI_CMD_SCAN_DONE,
	/** Connect result */
	NET_EVENT_WIFI_CMD_CONNECT_RESULT,
	/** Disconnect result */
	NET_EVENT_WIFI_CMD_DISCONNECT_RESULT,
	/** Interface status */
	NET_EVENT_WIFI_CMD_IFACE_STATUS,
	/** TWT events */
	NET_EVENT_WIFI_CMD_TWT,
	/** TWT sleep status: awake or sleeping, can be used by application
	 * to determine if it can send data or not.
	 */
	NET_EVENT_WIFI_CMD_TWT_SLEEP_STATE,
	/** Raw scan results available */
	NET_EVENT_WIFI_CMD_RAW_SCAN_RESULT,
	/** Disconnect complete */
	NET_EVENT_WIFI_CMD_DISCONNECT_COMPLETE,
	/** AP mode enable result */
	NET_EVENT_WIFI_CMD_AP_ENABLE_RESULT,
	/** AP mode disable result */
	NET_EVENT_WIFI_CMD_AP_DISABLE_RESULT,
	/** STA connected to AP */
	NET_EVENT_WIFI_CMD_AP_STA_CONNECTED,
	/** STA disconnected from AP */
	NET_EVENT_WIFI_CMD_AP_STA_DISCONNECTED,
};

#define NET_EVENT_WIFI_SCAN_RESULT				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_SCAN_RESULT)

#define NET_EVENT_WIFI_SCAN_DONE				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_SCAN_DONE)

#define NET_EVENT_WIFI_CONNECT_RESULT				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_CONNECT_RESULT)

#define NET_EVENT_WIFI_DISCONNECT_RESULT			\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_DISCONNECT_RESULT)

#define NET_EVENT_WIFI_IFACE_STATUS						\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_IFACE_STATUS)

#define NET_EVENT_WIFI_TWT					\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_TWT)

#define NET_EVENT_WIFI_TWT_SLEEP_STATE				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_TWT_SLEEP_STATE)

#define NET_EVENT_WIFI_RAW_SCAN_RESULT                          \
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_RAW_SCAN_RESULT)

#define NET_EVENT_WIFI_DISCONNECT_COMPLETE			\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_DISCONNECT_COMPLETE)

#define NET_EVENT_WIFI_AP_ENABLE_RESULT				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_AP_ENABLE_RESULT)

#define NET_EVENT_WIFI_AP_DISABLE_RESULT			\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_AP_DISABLE_RESULT)

#define NET_EVENT_WIFI_AP_STA_CONNECTED				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_AP_STA_CONNECTED)

#define NET_EVENT_WIFI_AP_STA_DISCONNECTED			\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_AP_STA_DISCONNECTED)

/** Wi-Fi version */
struct wifi_version {
	/** Driver version */
	const char *drv_version;
	/** Firmware version */
	const char *fw_version;
};

/**
 * @brief Wi-Fi structure to uniquely identify a band-channel pair
 */
struct wifi_band_channel {
	/** Frequency band */
	uint8_t band;
	/** Channel */
	uint8_t channel;
};

/**
 * @brief Wi-Fi scan parameters structure.
 * Used to specify parameters which can control how the Wi-Fi scan
 * is performed.
 */
struct wifi_scan_params {
	/** Scan type, see enum wifi_scan_type.
	 *
	 * The scan_type is only a hint to the underlying Wi-Fi chip for the
	 * preferred mode of scan. The actual mode of scan can depend on factors
	 * such as the Wi-Fi chip implementation support, regulatory domain
	 * restrictions etc.
	 */
	enum wifi_scan_type scan_type;
	/** Bitmap of bands to be scanned.
	 *  Refer to ::wifi_frequency_bands for bit position of each band.
	 */
	uint8_t bands;
	/** Active scan dwell time (in ms) on a channel.
	 */
	uint16_t dwell_time_active;
	/** Passive scan dwell time (in ms) on a channel.
	 */
	uint16_t dwell_time_passive;
	/** Array of SSID strings to scan.
	 */
	const char *ssids[WIFI_MGMT_SCAN_SSID_FILT_MAX];
	/** Specifies the maximum number of scan results to return. These results would be the
	 * BSSIDS with the best RSSI values, in all the scanned channels. This should only be
	 * used to limit the number of returned scan results, and cannot be counted upon to limit
	 * the scan time, since the underlying Wi-Fi chip might have to scan all the channels to
	 * find the max_bss_cnt number of APs with the best signal strengths. A value of 0
	 * signifies that there is no restriction on the number of scan results to be returned.
	 */
	uint16_t max_bss_cnt;
	/** Channel information array indexed on Wi-Fi frequency bands and channels within that
	 * band.
	 * E.g. to scan channel 6 and 11 on the 2.4 GHz band, channel 36 on the 5 GHz band:
	 * @code{.c}
	 *     chan[0] = {WIFI_FREQ_BAND_2_4_GHZ, 6};
	 *     chan[1] = {WIFI_FREQ_BAND_2_4_GHZ, 11};
	 *     chan[2] = {WIFI_FREQ_BAND_5_GHZ, 36};
	 * @endcode
	 *
	 *  This list specifies the channels to be __considered for scan__. The underlying
	 *  Wi-Fi chip can silently omit some channels due to various reasons such as channels
	 *  not conforming to regulatory restrictions etc. The invoker of the API should
	 *  ensure that the channels specified follow regulatory rules.
	 */
	struct wifi_band_channel band_chan[WIFI_MGMT_SCAN_CHAN_MAX_MANUAL];
};

/** Wi-Fi scan result, each result is provided to the net_mgmt_event_callback
 * via its info attribute (see net_mgmt.h)
 */
struct wifi_scan_result {
	/** SSID */
	uint8_t ssid[WIFI_SSID_MAX_LEN];
	/** SSID length */
	uint8_t ssid_length;
	/** Frequency band */
	uint8_t band;
	/** Channel */
	uint8_t channel;
	/** Security type */
	enum wifi_security_type security;
	/** MFP options */
	enum wifi_mfp_options mfp;
	/** RSSI */
	int8_t rssi;
	/** BSSID */
	uint8_t mac[WIFI_MAC_ADDR_LEN];
	/** BSSID length */
	uint8_t mac_length;
};

/** Wi-Fi connect request parameters */
struct wifi_connect_req_params {
	/** SSID */
	const uint8_t *ssid;
	/** SSID length */
	uint8_t ssid_length; /* Max 32 */
	/** Pre-shared key */
	const uint8_t *psk;
	/** Pre-shared key length */
	uint8_t psk_length; /* Min 8 - Max 64 */
	/** SAE password (same as PSK but with no length restrictions), optional */
	const uint8_t *sae_password;
	/** SAE password length */
	uint8_t sae_password_length; /* No length restrictions */
	/** Frequency band */
	uint8_t band;
	/** Channel */
	uint8_t channel;
	/** Security type */
	enum wifi_security_type security;
	/** MFP options */
	enum wifi_mfp_options mfp;
	/** BSSID */
	uint8_t bssid[WIFI_MAC_ADDR_LEN];
	/** Connect timeout in seconds, SYS_FOREVER_MS for no timeout */
	int timeout;
};

/** Wi-Fi connect result codes. To be overlaid on top of \ref wifi_status
 * in the connect result event for detailed status.
 */
enum wifi_conn_status {
	/** Connection successful */
	WIFI_STATUS_CONN_SUCCESS = 0,
	/** Connection failed - generic failure */
	WIFI_STATUS_CONN_FAIL,
	/** Connection failed - wrong password */
	WIFI_STATUS_CONN_WRONG_PASSWORD,
	/** Connection timed out */
	WIFI_STATUS_CONN_TIMEOUT,
	/** Connection failed - AP not found */
	WIFI_STATUS_CONN_AP_NOT_FOUND,
	/** Last connection status */
	WIFI_STATUS_CONN_LAST_STATUS,
	/** Connection disconnected status */
	WIFI_STATUS_DISCONN_FIRST_STATUS = WIFI_STATUS_CONN_LAST_STATUS,
};

/** Wi-Fi disconnect reason codes. To be overlaid on top of \ref wifi_status
 * in the disconnect result event for detailed reason.
 */
enum wifi_disconn_reason {
	/** Unspecified reason */
	WIFI_REASON_DISCONN_UNSPECIFIED = WIFI_STATUS_DISCONN_FIRST_STATUS,
	/** Disconnected due to user request */
	WIFI_REASON_DISCONN_USER_REQUEST,
	/** Disconnected due to AP leaving */
	WIFI_REASON_DISCONN_AP_LEAVING,
	/** Disconnected due to inactivity */
	WIFI_REASON_DISCONN_INACTIVITY,
};

/** Wi-Fi AP mode result codes. To be overlaid on top of \ref wifi_status
 * in the AP mode enable or disable result event for detailed status.
 */
enum wifi_ap_status {
	/** AP mode enable or disable successful */
	WIFI_STATUS_AP_SUCCESS = 0,
	/** AP mode enable or disable failed - generic failure */
	WIFI_STATUS_AP_FAIL,
	/** AP mode enable failed - channel not supported */
	WIFI_STATUS_AP_CHANNEL_NOT_SUPPORTED,
	/** AP mode enable failed - channel not allowed */
	WIFI_STATUS_AP_CHANNEL_NOT_ALLOWED,
	/** AP mode enable failed - SSID not allowed */
	WIFI_STATUS_AP_SSID_NOT_ALLOWED,
	/** AP mode enable failed - authentication type not supported */
	WIFI_STATUS_AP_AUTH_TYPE_NOT_SUPPORTED,
	/** AP mode enable failed - operation not supported */
	WIFI_STATUS_AP_OP_NOT_SUPPORTED,
	/** AP mode enable failed - operation not permitted */
	WIFI_STATUS_AP_OP_NOT_PERMITTED,
};

/** Generic Wi-Fi status for commands and events */
struct wifi_status {
	union {
		int status;
		enum wifi_conn_status conn_status;
		enum wifi_disconn_reason disconn_reason;
		enum wifi_ap_status ap_status;
	};
};

/** Wi-Fi interface status */
struct wifi_iface_status {
	/** Interface state, see enum wifi_iface_state */
	int state;
	/** SSID length */
	unsigned int ssid_len;
	/** SSID */
	char ssid[WIFI_SSID_MAX_LEN];
	/** BSSID */
	char bssid[WIFI_MAC_ADDR_LEN];
	/** Frequency band */
	enum wifi_frequency_bands band;
	/** Channel */
	unsigned int channel;
	/** Interface mode, see enum wifi_iface_mode */
	enum wifi_iface_mode iface_mode;
	/** Link mode, see enum wifi_link_mode */
	enum wifi_link_mode link_mode;
	/** Security type, see enum wifi_security_type */
	enum wifi_security_type security;
	/** MFP options, see enum wifi_mfp_options */
	enum wifi_mfp_options mfp;
	/** RSSI */
	int rssi;
	/** DTIM period */
	unsigned char dtim_period;
	/** Beacon interval */
	unsigned short beacon_interval;
	/** is TWT capable? */
	bool twt_capable;
};

/** Wi-Fi power save parameters */
struct wifi_ps_params {
	/* Power save state */
	enum wifi_ps enabled;
	/* Listen interval */
	unsigned short listen_interval;
	/** Wi-Fi power save wakeup mode */
	enum wifi_ps_wakeup_mode wakeup_mode;
	/** Wi-Fi power save mode */
	enum wifi_ps_mode mode;
	/** Wi-Fi power save timeout
	 *
	 * This is the time out to wait after sending a TX packet
	 * before going back to power save (in ms) to receive any replies
	 * from the AP. Zero means this feature is disabled.
	 *
	 * It's a tradeoff between power consumption and latency.
	 */
	unsigned int timeout_ms;
	/** Wi-Fi power save type */
	enum wifi_ps_param_type type;
	/** Wi-Fi power save fail reason */
	enum wifi_config_ps_param_fail_reason fail_reason;
};

/** Wi-Fi TWT parameters */
struct wifi_twt_params {
	/** TWT operation, see enum wifi_twt_operation */
	enum wifi_twt_operation operation;
	/** TWT negotiation type, see enum wifi_twt_negotiation_type */
	enum wifi_twt_negotiation_type negotiation_type;
	/** TWT setup command, see enum wifi_twt_setup_cmd */
	enum wifi_twt_setup_cmd setup_cmd;
	/** TWT setup response status, see enum wifi_twt_setup_resp_status */
	enum wifi_twt_setup_resp_status resp_status;
	/** TWT teardown cmd status, see enum wifi_twt_teardown_status */
	enum wifi_twt_teardown_status teardown_status;
	/** Dialog token, used to map requests to responses */
	uint8_t dialog_token;
	/** Flow ID, used to map setup with teardown */
	uint8_t flow_id;
	union {
		/** Setup specific parameters */
		struct {
			/**Interval = Wake up time + Sleeping time */
			uint64_t twt_interval;
			/** Requestor or responder */
			bool responder;
			/** Trigger enabled or disabled */
			bool trigger;
			/** Implicit or explicit */
			bool implicit;
			/** Announced or unannounced */
			bool announce;
			/** Wake up time */
			uint32_t twt_wake_interval;
			/* Wake ahead notification is sent earlier than
			 * TWT Service period (SP) start based on this duration.
			 * This should give applications ample time to
			 * prepare the data before TWT SP starts.
			 */
			uint32_t twt_wake_ahead_duration;
		} setup;
		/** Teardown specific parameters */
		struct {
			/** Teardown all flows */
			bool teardown_all;
		} teardown;
	};
	/** TWT fail reason, see enum wifi_twt_fail_reason */
	enum wifi_twt_fail_reason fail_reason;
};

/* Flow ID is only 3 bits */
#define WIFI_MAX_TWT_FLOWS 8
#define WIFI_MAX_TWT_INTERVAL_US (LONG_MAX - 1)
/* 256 (u8) * 1TU */
#define WIFI_MAX_TWT_WAKE_INTERVAL_US 262144
#define WIFI_MAX_TWT_WAKE_AHEAD_DURATION_US (LONG_MAX - 1)

/** Wi-Fi TWT flow information */
struct wifi_twt_flow_info {
	/** Interval = Wake up time + Sleeping time */
	uint64_t  twt_interval;
	/** Dialog token, used to map requests to responses */
	uint8_t dialog_token;
	/** Flow ID, used to map setup with teardown */
	uint8_t flow_id;
	/** TWT negotiation type, see enum wifi_twt_negotiation_type */
	enum wifi_twt_negotiation_type negotiation_type;
	/** Requestor or responder */
	bool responder;
	/** Trigger enabled or disabled */
	bool trigger;
	/** Implicit or explicit */
	bool implicit;
	/** Announced or unannounced */
	bool announce;
	/** Wake up time */
	uint32_t twt_wake_interval;
	/* wake ahead duration */
	uint32_t twt_wake_ahead_duration;
};

/** Wi-Fi power save configuration */
struct wifi_ps_config {
	/** Number of TWT flows */
	char num_twt_flows;
	/** TWT flow details */
	struct wifi_twt_flow_info twt_flows[WIFI_MAX_TWT_FLOWS];
	/** Power save configuration */
	struct wifi_ps_params ps_params;
};

/** Generic get/set operation for any command*/
enum wifi_mgmt_op {
	/** Get operation */
	WIFI_MGMT_GET = 0,
	/** Set operation */
	WIFI_MGMT_SET = 1,
};

#define MAX_REG_CHAN_NUM  42

/** Per-channel regulatory attributes */
struct wifi_reg_chan_info {
	/** Center frequency in MHz */
	unsigned short center_frequency;
	/** Maximum transmission power (in dBm) */
	unsigned short max_power:8;
	/** Is channel supported or not */
	unsigned short supported:1;
	/** Passive transmissions only */
	unsigned short passive_only:1;
	/** Is a DFS channel */
	unsigned short dfs:1;
} __packed;

/** Regulatory domain information or configuration */
struct wifi_reg_domain {
	/* Regulatory domain operation */
	enum wifi_mgmt_op oper;
	/** Ignore all other regulatory hints over this one */
	bool force;
	/** Country code: ISO/IEC 3166-1 alpha-2 */
	uint8_t country_code[WIFI_COUNTRY_CODE_LEN];
	/** Number of channels supported */
	unsigned int num_channels;
	/** Channels information */
	struct wifi_reg_chan_info *chan_info;
};

/** Wi-Fi TWT sleep states */
enum wifi_twt_sleep_state {
	/** TWT sleep state: sleeping */
	WIFI_TWT_STATE_SLEEP = 0,
	/** TWT sleep state: awake */
	WIFI_TWT_STATE_AWAKE = 1,
};

#if defined(CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS) || defined(__DOXYGEN__)
/** Wi-Fi raw scan result */
struct wifi_raw_scan_result {
	/** RSSI */
	int8_t rssi;
	/** Frame length */
	int frame_length;
	/** Frequency */
	unsigned short frequency;
	/** Raw scan data */
	uint8_t data[CONFIG_WIFI_MGMT_RAW_SCAN_RESULT_LENGTH];
};
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */

/** AP mode - connected STA details */
struct wifi_ap_sta_info {
	/** Link mode, see enum wifi_link_mode */
	enum wifi_link_mode link_mode;
	/** MAC address */
	uint8_t mac[WIFI_MAC_ADDR_LEN];
	/** MAC address length */
	uint8_t mac_length;
	/** is TWT capable ? */
	bool twt_capable;
};

/* for use in max info size calculations */
union wifi_mgmt_events {
	struct wifi_scan_result scan_result;
	struct wifi_status connect_status;
	struct wifi_iface_status iface_status;
#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
	struct wifi_raw_scan_result raw_scan_result;
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */
	struct wifi_twt_params twt_params;
	struct wifi_ap_sta_info ap_sta_info;
};

/** Wi-Fi mode setup */
struct wifi_mode_info {
	/** Mode setting for a specific mode of operation */
	uint8_t mode;
	/** Interface index */
	uint8_t if_index;
	/** Get or set operation */
	enum wifi_mgmt_op oper;
};

/** Wi-Fi filter setting for monitor, prmoiscuous, TX-injection modes */
struct wifi_filter_info {
	/** Filter setting */
	uint8_t filter;
	/** Interface index */
	uint8_t if_index;
	/** Filter buffer size */
	uint16_t buffer_size;
	/** Get or set operation */
	enum wifi_mgmt_op oper;
};

/** Wi-Fi channel setting for monitor and TX-injection modes */
struct wifi_channel_info {
	/** Channel value to set */
	uint16_t channel;
	/** Interface index */
	uint8_t if_index;
	/** Get or set operation */
	enum wifi_mgmt_op oper;
};

#include <zephyr/net/net_if.h>

/** Scan result callback
 *
 * @param iface Network interface
 * @param status Scan result status
 * @param entry Scan result entry
 */
typedef void (*scan_result_cb_t)(struct net_if *iface, int status,
				 struct wifi_scan_result *entry);

#ifdef CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS
/** Raw scan result callback
 *
 * @param iface Network interface
 * @param status Raw scan result status
 * @param entry Raw scan result entry
 */
typedef void (*raw_scan_result_cb_t)(struct net_if *iface, int status,
				     struct wifi_raw_scan_result *entry);
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */

/** Wi-Fi management API */
struct wifi_mgmt_ops {
	/** Scan for Wi-Fi networks
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Scan parameters
	 * @param cb Callback to be called for each result
	 *           cb parameter is the cb that should be called for each
	 *           result by the driver. The wifi mgmt part will take care of
	 *           raising the necessary event etc.
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*scan)(const struct device *dev,
		    struct wifi_scan_params *params,
		    scan_result_cb_t cb);
	/** Connect to a Wi-Fi network
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Connect parameters
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*connect)(const struct device *dev,
		       struct wifi_connect_req_params *params);
	/** Disconnect from a Wi-Fi network
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*disconnect)(const struct device *dev);
	/** Enable AP mode
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params AP mode parameters
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*ap_enable)(const struct device *dev,
			 struct wifi_connect_req_params *params);
	/** Disable AP mode
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*ap_disable)(const struct device *dev);
	/** Disconnect a STA from AP
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param mac MAC address of the STA to disconnect
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*ap_sta_disconnect)(const struct device *dev, const uint8_t *mac);
	/** Get interface status
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param status Interface status
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*iface_status)(const struct device *dev, struct wifi_iface_status *status);
#if defined(CONFIG_NET_STATISTICS_WIFI) || defined(__DOXYGEN__)
	/** Get Wi-Fi statistics
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param stats Wi-Fi statistics
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*get_stats)(const struct device *dev, struct net_stats_wifi *stats);
#endif /* CONFIG_NET_STATISTICS_WIFI */
	/** Set power save status
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params Power save parameters
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*set_power_save)(const struct device *dev, struct wifi_ps_params *params);
	/** Setup or teardown TWT flow
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param params TWT parameters
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*set_twt)(const struct device *dev, struct wifi_twt_params *params);
	/** Get power save config
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param config Power save config
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*get_power_save_config)(const struct device *dev, struct wifi_ps_config *config);
	/** Set or get regulatory domain
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param reg_domain Regulatory domain
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*reg_domain)(const struct device *dev, struct wifi_reg_domain *reg_domain);
	/** Set or get packet filter settings for monitor and promiscuous modes
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param packet filter settings
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*filter)(const struct device *dev, struct wifi_filter_info *filter);
	/** Set or get mode of operation
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param mode settings
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*mode)(const struct device *dev, struct wifi_mode_info *mode);
	/** Set or get current channel of operation
	 *
	 * @param dev Pointer to the device structure for the driver instance.
	 * @param channel settings
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*channel)(const struct device *dev, struct wifi_channel_info *channel);
	/** Get Version of WiFi driver and Firmware
	 *
	 * The driver that implements the get_version function must not use stack to allocate the
	 * version information pointers that are returned as params struct members.
	 * The version pointer parameters should point to a static memory either in ROM (preferred)
	 * or in RAM.
	 *
	 * @param dev Pointer to the device structure for the driver instance
	 * @param params Version parameters
	 *
	 * @return 0 if ok, < 0 if error
	 */
	int (*get_version)(const struct device *dev, struct wifi_version *params);
};

/** Wi-Fi management offload API */
struct net_wifi_mgmt_offload {
	/**
	 * Mandatory to get in first position.
	 * A network device should indeed provide a pointer on such
	 * net_if_api structure. So we make current structure pointer
	 * that can be casted to a net_if_api structure pointer.
	 */
#if defined(CONFIG_WIFI_USE_NATIVE_NETWORKING) || defined(__DOXYGEN__)
	/** Ethernet API */
	struct ethernet_api wifi_iface;
#else
	/** Offloaded network device API */
	struct offloaded_if_api wifi_iface;
#endif
	/** Wi-Fi management API */
	const struct wifi_mgmt_ops *const wifi_mgmt_api;

#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT) || defined(__DOXYGEN__)
	/** Wi-Fi supplicant driver API */
	void *wifi_drv_ops;
#endif
};

#if defined(CONFIG_WIFI_NM_WPA_SUPPLICANT)
/* Make sure wifi_drv_ops is after wifi_mgmt_api */
BUILD_ASSERT(offsetof(struct net_wifi_mgmt_offload, wifi_mgmt_api) <
	     offsetof(struct net_wifi_mgmt_offload, wifi_drv_ops));
#endif

/* Make sure that the network interface API is properly setup inside
 * Wifi mgmt offload API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct net_wifi_mgmt_offload, wifi_iface) == 0);

/** Wi-Fi management connect result event
 *
 * @param iface Network interface
 * @param status Connect result status
 */
void wifi_mgmt_raise_connect_result_event(struct net_if *iface, int status);

/** Wi-Fi management disconnect result event
 *
 * @param iface Network interface
 * @param status Disconnect result status
 */
void wifi_mgmt_raise_disconnect_result_event(struct net_if *iface, int status);

/** Wi-Fi management interface status event
 *
 * @param iface Network interface
 * @param iface_status Interface status
 */
void wifi_mgmt_raise_iface_status_event(struct net_if *iface,
		struct wifi_iface_status *iface_status);

/** Wi-Fi management TWT event
 *
 * @param iface Network interface
 * @param twt_params TWT parameters
 */
void wifi_mgmt_raise_twt_event(struct net_if *iface,
		struct wifi_twt_params *twt_params);

/** Wi-Fi management TWT sleep state event
 *
 * @param iface Network interface
 * @param twt_sleep_state TWT sleep state
 */
void wifi_mgmt_raise_twt_sleep_state(struct net_if *iface, int twt_sleep_state);

#if defined(CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS) || defined(__DOXYGEN__)
/** Wi-Fi management raw scan result event
 *
 * @param iface Network interface
 * @param raw_scan_info Raw scan result
 */
void wifi_mgmt_raise_raw_scan_result_event(struct net_if *iface,
		struct wifi_raw_scan_result *raw_scan_info);
#endif /* CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS */

/** Wi-Fi management disconnect complete event
 *
 * @param iface Network interface
 * @param status Disconnect complete status
 */
void wifi_mgmt_raise_disconnect_complete_event(struct net_if *iface, int status);

/** Wi-Fi management AP mode enable result event
 *
 * @param iface Network interface
 * @param status AP mode enable result status
 */
void wifi_mgmt_raise_ap_enable_result_event(struct net_if *iface, enum wifi_ap_status status);

/** Wi-Fi management AP mode disable result event
 *
 * @param iface Network interface
 * @param status AP mode disable result status
 */
void wifi_mgmt_raise_ap_disable_result_event(struct net_if *iface, enum wifi_ap_status status);

/** Wi-Fi management AP mode STA connected event
 *
 * @param iface Network interface
 * @param sta_info STA information
 */
void wifi_mgmt_raise_ap_sta_connected_event(struct net_if *iface,
		struct wifi_ap_sta_info *sta_info);

/** Wi-Fi management AP mode STA disconnected event
 * @param iface Network interface
 * @param sta_info STA information
 */
void wifi_mgmt_raise_ap_sta_disconnected_event(struct net_if *iface,
		struct wifi_ap_sta_info *sta_info);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_WIFI_MGMT_H_ */
