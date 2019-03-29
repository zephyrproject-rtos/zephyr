/*
 * @file
 * @brief Driver interfaces and callbacks for WiFi manager
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_DRV_H_
#define ZEPHYR_INCLUDE_NET_WIFI_DRV_H_

#include <net/ethernet.h>

#define WIFI_MAC_ADDR_LEN	NET_LINK_ADDR_MAX_LENGTH
#define WIFI_MAX_SSID_LEN	32
#define WIFI_MAX_PSPHR_LEN	63

#define WIFI_DRV_BLACKLIST_ADD	(1)
#define WIFI_DRV_BLACKLIST_DEL	(2)

/**
 * @struct wifi_drv_capa
 * @brief Union holding driver capability info.
 *
 * @param unsigned char max_rtt_peers
 * Maximum RTT peers
 *
 * @param unsigned char max_ap_assoc_sta
 * Maximum number of associated STAs supported in AP mode
 *
 * @param unsigned char max_acl_mac_addrs
 * Maximum number of ACL MAC addresses supported in AP mode
 */
union wifi_drv_capa {
	struct {
		unsigned char max_rtt_peers;
	} sta;
	struct {
		unsigned char max_ap_assoc_sta;
		unsigned char max_acl_mac_addrs;
	} ap;
};

#define WIFI_BAND_2G		(2)
#define WIFI_BAND_5G		(5)

#define WIFI_CHANNEL_2G_MIN	(1)
#define WIFI_CHANNEL_2G_MAX	(14)
#define WIFI_CHANNEL_5G_MIN	(7)
#define WIFI_CHANNEL_5G_MAX	(196)

#define WIFI_CHANNEL_WIDTH_20	(20)
#define WIFI_CHANNEL_WIDTH_40	(40)
#define WIFI_CHANNEL_WIDTH_80	(80)
#define WIFI_CHANNEL_WIDTH_160	(160)

/**
 * @struct wifi_drv_scan_params
 * @brief Structure holding scan parameters.
 *
 * @var unsigned char wifi_drv_scan_params::band
 * Band
 *
 * @var unsigned char wifi_drv_scan_params::channel
 * Channel number
 *
 */
struct wifi_drv_scan_params {
	unsigned char band;
	unsigned char channel;
};

/**
 * @struct wifi_drv_connect_params
 * @brief Structure holding connect parameters.
 *
 * @var char wifi_drv_connect_params::ssid
 * SSID
 *
 * @var char wifi_drv_connect_params::ssid_len
 * SSID length (maximum is 32 bytes)
 *
 * @var char wifi_drv_connect_params::bssid
 * BSSID
 *
 * @var char wifi_drv_connect_params::psk
 * PSK for WPA/WPA2-PSK
 *
 * @var char wifi_drv_connect_params::psk_len
 * PSK length (maximum is 32 bytes)
 *
 * @var unsigned char wifi_drv_connect_params::channel
 * Channel number
 *
 * @var char wifi_drv_connect_params::security
 * Security type
 */
struct wifi_drv_connect_params {
	char *ssid;
	char ssid_len;
	char *bssid;
	char *psk;
	char psk_len;
	unsigned char channel;
	char security;
};

/**
 * @struct wifi_drv_start_ap_params
 * @brief Structure holding connect parameters.
 *
 * @var char wifi_drv_start_ap_params::ssid
 * SSID
 *
 * @var char wifi_drv_start_ap_params::ssid_len
 * SSID length (maximum is 32 bytes)
 *
 * @var char wifi_drv_start_ap_params::psk
 * PSK for WPA/WPA2-PSK
 *
 * @var char wifi_drv_start_ap_params::psk_len
 * PSK length (maximum is 32 bytes)
 *
 * @var unsigned char wifi_drv_start_ap_params::channel
 * Channel number
 *
 * @var unsigned char wifi_config::ch_width
 * Channel width
 *
 * @var char wifi_drv_start_ap_params::security
 * Security type
 */
struct wifi_drv_start_ap_params {
	char *ssid;
	char ssid_len;
	char *psk;
	char psk_len;
	unsigned char channel;
	unsigned char ch_width;
	char security;
};

/**
 * @struct wifi_rtt_peers
 * @brief Structure holding RTT peer.
 *
 * @var char wifi_rtt_peers::bssid
 * BSSID of RTT peer
 *
 * @var unsigned char wifi_rtt_peers::band
 * Band of RTT peer
 *
 * @var unsigned char wifi_rtt_peers::channel
 * Channel number of RTT peer
 *
 */
struct wifi_rtt_peers {
	char bssid[NET_LINK_ADDR_MAX_LENGTH];
	unsigned char band;
	unsigned char channel;
};

/**
 * @struct wifi_drv_rtt_request
 * @brief Structure holding RTT request parameters.
 *
 * @var unsigned char wifi_drv_rtt_request::nr_peers
 * Number of RTT peers
 *
 * @var struct wifi_drv_rtt_request::peers
 * RTT peer (@ref wifi_rtt_peers)
 *
 */
struct wifi_drv_rtt_request {
	unsigned char nr_peers;
	struct wifi_rtt_peers *peers;
};

/**
 * @struct wifi_drv_connect_evt
 * @brief Structure holding RTT peer.
 *
 * @var char wifi_drv_connect_evt::status
 * Connect result
 *
 * @var char wifi_drv_connect_evt::bssid
 * BSSID of the connected AP
 *
 * @var unsigned char wifi_drv_connect_evt::channel
 * Channel number of the connected AP
 *
 */
struct wifi_drv_connect_evt {
	char status;
	char bssid[NET_LINK_ADDR_MAX_LENGTH];
	unsigned char channel;
};

/**
 * @struct wifi_drv_scan_result_evt
 * @brief Structure holding WiFi scan result.
 *
 * @var char wifi_drv_scan_result_evt::bssid
 * BSSID
 *
 * @var char wifi_drv_scan_result_evt::ssid
 * SSID
 *
 * @var char wifi_drv_scan_result_evt::ssid_len
 * SSID length (maximum is 32 bytes)
 *
 * @var unsigned char wifi_drv_scan_result_evt::band
 * Band
 *
 * @var unsigned char wifi_drv_scan_result_evt::channel
 * Channel number
 *
 * @var signed char wifi_drv_scan_result_evt::rssi
 * Signal strength
 *
 * @var char wifi_drv_scan_result_evt::security
 * Security type
 *
 * @var char wifi_drv_scan_result_evt::rtt_supported
 * Indicate whether RTT is supported
 */
struct wifi_drv_scan_result_evt {
	char bssid[NET_LINK_ADDR_MAX_LENGTH];
	char ssid[WIFI_MAX_SSID_LEN];
	char ssid_len;
	unsigned char band;
	unsigned char channel;
	signed char rssi;
	char security;
	char rtt_supported;
};

/**
 * @struct wifi_drv_rtt_response_evt
 * @brief Structure holding RTT range response.
 *
 * @var char wifi_drv_rtt_response_evt::bssid
 * BSSID of RTT peer
 *
 * @var int wifi_drv_rtt_response_evt::range
 * RTT range
 */
struct wifi_drv_rtt_response_evt {
	char bssid[NET_LINK_ADDR_MAX_LENGTH];
	int range;
};

/**
 * @struct wifi_drv_new_station_evt
 * @brief Structure holding RTT peer.
 *
 * @var char wifi_drv_new_station_evt::is_connect
 * Indicate whether the station is connected or disconnected.
 *
 * @var char wifi_drv_new_station_evt::mac
 * BSSID of the STA
 */
struct wifi_drv_new_station_evt {
	char is_connect;
	char mac[NET_LINK_ADDR_MAX_LENGTH];
};

/**
 * @typedef scan_result_evt_t
 * @brief Callback type for reporting scan results.
 *
 * A function of this type is given to the .scan() function
 * and will be called for any discovered Access Point.
 *
 * @param iface Wi-Fi device interface.
 * @param status 0 on a scan result or scan done, non-zero on scan abort.
 * @param entry NULL on scan done/abort, non-NULL on a scan result.
 */
typedef void (*scan_result_evt_t)(void *iface, int status,
				  struct wifi_drv_scan_result_evt *entry);

/**
 * @typedef rtt_result_evt_t
 * @brief Callback type for reporting RTT range response.
 *
 * A function of this type is given to the .rtt_req() function
 * and will be called for any received RTT range response.
 *
 * @param iface Wi-Fi device interface.
 * @param status 0 on a RTT range response or RTT done, non-zero on RTT abort.
 * @param entry NULL on RTT done/abort, non-NULL on a RTT range response.
 */
typedef void (*rtt_result_evt_t)(void *iface, int status,
				 struct wifi_drv_rtt_response_evt *entry);

/**
 * @typedef connect_evt_t
 * @brief Callback type for reporting connect result of STA.
 *
 * A function of this type is given to the .connect() function
 * and will be called for received connect result.
 *
 * @param iface Wi-Fi device interface.
 * @param status 0 on success, non-zero on failure.
 * @param bssid BSSID of the Access Point connected.
 * @param channel Channel of the Access Point connected.
 */
typedef void (*connect_evt_t)(void *iface, int status, char *bssid,
			      unsigned char channel);

/**
 * @typedef disconnect_evt_t
 * @brief Callback type for reporting disconnect result of STA.
 *
 * A function of this type is given to the .connect() or .disconnect function
 * and will be called for received disconnect result.
 *
 * @param iface Wi-Fi device interface.
 * @param status Reason code from the AP.
 */
typedef void (*disconnect_evt_t)(void *iface, int status);

/**
 * @typedef new_station_evt_t
 * @brief Callback type for reporting connected/disconnected STA to AP.
 *
 * A function of this type is given to the .connect() or .disconnect function
 * and will be called for received disconnect result.
 *
 * @param iface Wi-Fi device interface.
 * @param status 0 on disconnection, non-zero on connection.
 */
typedef void (*new_station_evt_t)(void *iface, int status, char *mac);

/**
 * @struct wifi_drv_api
 * Driver interfaces for WiFi Manager.
 */
struct wifi_drv_api {
	/**
	 * Mandatory to get in first position.
	 * A network device should indeed provide a pointer on such
	 * ethernet_api structure. So we make current structure pointer
	 * that can be casted to a eth_api structure pointer.
	 */
	struct ethernet_api eth_api;

	/**
	 * @brief Get driver capability.
	 *
	 * @param dev Wi-Fi device.
	 * @param capa Driver capability info (@ref wifi_drv_capa).
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*get_capa)(struct device *dev, union wifi_drv_capa *capa);

	/**
	 * @brief Open WiFi device.
	 *
	 * @param dev Wi-Fi device.
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*open)(struct device *dev);

	/**
	 * @brief Close WiFi device.
	 *
	 * @param dev Wi-Fi device.
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*close)(struct device *dev);

	/**
	 * @brief Trigger scan.
	 *
	 * @param dev Wi-Fi device.
	 * @param params Scan parameters (@ref wifi_drv_scan_params).
	 * @param cb Callback to invoke for each scan result.
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*scan)(struct device *dev, struct wifi_drv_scan_params *params,
		    scan_result_evt_t cb);

	/**
	 * @brief Request RTT range.
	 *
	 * @param dev Wi-Fi device.
	 * @param req RTT range request parameters
	 * (@ref wifi_drv_rtt_request).
	 * @param cb Callback to invoke for each RTT range response
	 * (@ref rtt_result_evt_t).
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*rtt_req)(struct device *dev, struct wifi_drv_rtt_request *req,
		    rtt_result_evt_t cb);

	/**
	 * @brief Connect to the BSS/ESS with the specified parameters.
	 *
	 * When connected, call conn_cb with status code 0.
	 * If the connection fails, call conn_cb with status code non-zero.
	 * When deauthenticated by AP, call disc_cb with the reason code
	 * from the AP.
	 *
	 * @param dev Wi-Fi device.
	 * @param params Connect parameters (@ref wifi_drv_connect_params).
	 * @param conn_cb Callback to notify connect result
	 * (@ref connect_evt_t).
	 * @param disc_cb Callback to notify disconnect result due to AP deauth
	 * (@ref disconnect_evt_t).
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*connect)(struct device *dev,
		       struct wifi_drv_connect_params *params,
		       connect_evt_t conn_cb, disconnect_evt_t disc_cb);

	/**
	 * @brief Disconnect from the BSS/ESS..
	 *
	 * When deauthenticated locally, call cb with the reason code 0.
	 * When deauthenticated by AP, call cb with the reason code
	 * from the AP.
	 *
	 * @param dev Wi-Fi device.
	 * @param cb Callback to notify disconnect result
	 * (@ref disconnect_evt_t).
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*disconnect)(struct device *dev, disconnect_evt_t cb);

	/**
	 * @brief Get STA information.
	 *
	 * @param dev Wi-Fi device.
	 * @param rssi Signal strength of the connected AP.
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*get_station)(struct device *dev, signed char *rssi);

	/**
	 * @brief tell the driver IP address.
	 *
	 * @param dev Wi-Fi device.
	 * @param ipaddr IP address.
	 * @param len Length of IP address.
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*notify_ip)(struct device *dev, char *ipaddr, unsigned char len);

	/**
	 * @brief Start acting in AP mode defined by the parameters.
	 *
	 * @param dev Wi-Fi device.
	 * @param params AP parameters (@ref wifi_drv_start_ap_params).
	 * @param cb Callback to notify connected/disconnected STA
	 * (@ref new_station_evt_t).
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*start_ap)(struct device *dev,
			struct wifi_drv_start_ap_params *params,
			new_station_evt_t cb);

	/**
	 * @brief Stop being an AP, including stopping beaconing.
	 *
	 * @param dev Wi-Fi device.
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*stop_ap)(struct device *dev);

	/**
	 * @brief Remove a STA.
	 *
	 * @param dev Wi-Fi device.
	 * @param mac MAC address of the STA to be removed.
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*del_station)(struct device *dev, char *mac);

	/**
	 * @brief Sets MAC address control list in AP mode.
	 *
	 * @param dev Wi-Fi device.
	 * @param subcmd Subcommand (@ref wifi_mac_acl_subcmd).
	 * @param acl_nr Number of ACL MAC addresses.
	 * @param acl_mac_addrs Array of ACL MAC addresses.
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*set_mac_acl)(struct device *dev, char subcmd,
			   unsigned char acl_nr,
			   char acl_mac_addrs[][NET_LINK_ADDR_MAX_LENGTH]);

	/**
	 * @brief Hardware handler for production test.
	 *
	 * @param dev Wi-Fi device.
	 * @param tx_buf Buffer containing the testing command and parameters.
	 * @param tx_len Length of tx_buf.
	 * @param rx_buf Buffer containing the test result.
	 * @param rx_len Length of rx_buf.
	 *
	 * @retval 0 on success, non-zero on failure.
	 */
	int (*hw_test)(struct device *dev, char *tx_buf, unsigned int tx_len,
		       char *rx_buf, unsigned int *rx_len);
};

/**
 * @brief Determine if give Ethernet address is all zeros.
 *
 * @param addr: Pointer to a six-byte array containing the Ethernet address
 *
 * @retval true if the address is all zeroes.
 */
static inline bool is_zero_ether_addr(const char *addr)
{
	return (addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]) == 0;
}

/**
 * @brief Determine if the Ethernet address is broadcast
 *
 * @param addr: Pointer to a six-byte array containing the Ethernet address
 *
 * @retval true if the address is the broadcast address.
 */
static inline bool is_broadcast_ether_addr(const char *addr)
{
	return (addr[0] & addr[1] & addr[2] & addr[3] & addr[4] & addr[5]) ==
	    0xff;
}

#endif
