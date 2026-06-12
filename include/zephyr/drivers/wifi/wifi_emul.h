/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Backend API for the emulated Wi-Fi driver
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WIFI_WIFI_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_WIFI_WIFI_EMUL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi.h>

#ifdef __cplusplus
extern "C" {
#endif

struct net_if;
struct net_pkt;

/**
 * @brief Emulated Wi-Fi driver backend API
 * @defgroup wifi_emul Emulated Wi-Fi driver backend API
 * @ingroup io_interfaces
 * @{
 */

/** @brief Virtual access point description */
struct wifi_emul_ap {
	/** SSID */
	uint8_t ssid[WIFI_SSID_MAX_LEN + 1];
	/** SSID length */
	uint8_t ssid_length;
	/** BSSID, also used as the key when removing or updating an AP */
	uint8_t bssid[WIFI_MAC_ADDR_LEN];
	/** Channel */
	uint8_t channel;
	/** Frequency band */
	enum wifi_frequency_bands band;
	/** Security type */
	enum wifi_security_type security;
	/** Pre-shared key, expected from a connection request when the
	 * security type is not WIFI_SECURITY_TYPE_NONE. A connection
	 * request with a different PSK fails with
	 * WIFI_STATUS_CONN_WRONG_PASSWORD.
	 */
	char psk[WIFI_PSK_MAX_LEN + 1];
	/** RSSI in dBm reported in scan results and interface status */
	int8_t rssi;
	/** Hidden APs are excluded from scan results but can be
	 * connected to.
	 */
	bool hidden;
};

/**
 * @brief Register a virtual access point.
 *
 * The AP shows up in subsequent scan results (unless hidden) and can be
 * connected to.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param ap Virtual access point description, copied by the driver
 *
 * @retval 0 on success
 * @retval -EINVAL if the description is invalid
 * @retval -EEXIST if an AP with the same BSSID is already registered
 * @retval -ENOMEM if CONFIG_WIFI_EMUL_AP_COUNT_MAX APs are already registered
 */
int wifi_emul_ap_add(const struct device *dev, const struct wifi_emul_ap *ap);

/**
 * @brief Remove a virtual access point.
 *
 * Removing the currently connected AP does not disconnect the station,
 * use wifi_emul_trigger_disconnect() for that.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param bssid BSSID of the virtual access point to remove
 *
 * @retval 0 on success
 * @retval -ENOENT if no AP with the given BSSID is registered
 */
int wifi_emul_ap_remove(const struct device *dev, const uint8_t bssid[WIFI_MAC_ADDR_LEN]);

/**
 * @brief Remove all virtual access points.
 *
 * @param dev The emulated Wi-Fi device instance
 */
void wifi_emul_ap_clear(const struct device *dev);

/**
 * @brief Update the RSSI of a virtual access point.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param bssid BSSID of the virtual access point to update
 * @param rssi New RSSI in dBm
 *
 * @retval 0 on success
 * @retval -ENOENT if no AP with the given BSSID is registered
 */
int wifi_emul_ap_update_rssi(const struct device *dev, const uint8_t bssid[WIFI_MAC_ADDR_LEN],
			     int8_t rssi);

/**
 * @brief Callback invoked for each registered virtual access point.
 *
 * @param ap Virtual access point description
 * @param user_data Opaque pointer passed to wifi_emul_ap_foreach()
 */
typedef void (*wifi_emul_ap_cb_t)(const struct wifi_emul_ap *ap, void *user_data);

/**
 * @brief Iterate over all registered virtual access points.
 *
 * The callback is invoked for each registered AP while the driver lock is
 * held, so it must not call back into the wifi_emul API.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param cb Callback invoked for each registered AP
 * @param user_data Opaque pointer passed to the callback
 */
void wifi_emul_ap_foreach(const struct device *dev, wifi_emul_ap_cb_t cb, void *user_data);

/**
 * @brief Set the time between a scan request and the scan results.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param delay Scan duration
 */
void wifi_emul_set_scan_delay(const struct device *dev, k_timeout_t delay);

/**
 * @brief Set the time between a connect request and the connect result.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param delay Connect duration
 */
void wifi_emul_set_connect_delay(const struct device *dev, k_timeout_t delay);

/**
 * @brief Force the failure of subsequent connection requests.
 *
 * When set to a failure status, subsequent connection requests fail with
 * the given status instead of being evaluated against the registered
 * virtual access points, e.g. WIFI_STATUS_CONN_TIMEOUT. A success status
 * (WIFI_STATUS_CONN_SUCCESS) is ignored as it has no matching access
 * point; use a registered AP for successful connections.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param conn_status A failure status from enum wifi_conn_status, or a
 *		      negative value to resume normal evaluation
 */
void wifi_emul_force_connect_status(const struct device *dev, int conn_status);

/**
 * @brief Emulate an AP-initiated disconnect.
 *
 * Disconnects the station and raises a disconnect event with the given
 * reason, as if the access point dropped the connection.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param reason A disconnect reason from enum wifi_disconn_reason
 *
 * @retval 0 on success
 * @retval -ENOTCONN if the station is not connected
 */
int wifi_emul_trigger_disconnect(const struct device *dev, int reason);

/**
 * @brief Check whether the station is connected.
 *
 * @param dev The emulated Wi-Fi device instance
 *
 * @return true if connected, false otherwise
 */
bool wifi_emul_is_connected(const struct device *dev);

/**
 * @brief Get the virtual access point the station is connected to.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param ap Output location for the virtual access point description
 *
 * @retval 0 on success
 * @retval -ENOTCONN if the station is not connected
 */
int wifi_emul_get_current_ap(const struct device *dev, struct wifi_emul_ap *ap);

/**
 * @brief Callback invoked for each frame transmitted in self-contained mode.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param pkt Transmitted network packet. It is borrowed and freed by the
 *	      network stack after the callback returns; clone it to retain.
 * @param user_data Opaque pointer passed to wifi_emul_set_tx_cb()
 */
typedef void (*wifi_emul_tx_cb_t)(const struct device *dev, struct net_pkt *pkt, void *user_data);

/**
 * @brief Register a transmit callback for the self-contained data plane.
 *
 * When no underlying Ethernet data plane is attached, frames sent on the
 * Wi-Fi interface while connected are handed to this callback instead of
 * being forwarded to an Ethernet device. Pass @c NULL to drop transmitted
 * frames silently.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param cb Callback invoked for each transmitted frame, or NULL
 * @param user_data Opaque pointer passed to the callback
 */
void wifi_emul_set_tx_cb(const struct device *dev, wifi_emul_tx_cb_t cb, void *user_data);

/**
 * @brief Inject a received frame on the Wi-Fi interface.
 *
 * Delivers a raw Ethernet frame to the network stack as if it had been
 * received over the air. Requires the station to be connected.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param frame Raw Ethernet frame, starting with the Ethernet header
 * @param len Length of the frame in bytes
 *
 * @retval 0 on success
 * @retval -ENETDOWN if the station is not connected
 * @retval -ENOMEM if a packet could not be allocated
 * @retval -errno on other delivery errors
 */
int wifi_emul_rx_inject(const struct device *dev, const uint8_t *frame, size_t len);

/**
 * @brief Attach an Ethernet interface as the data plane at runtime.
 *
 * Bridges the Wi-Fi interface to an existing Ethernet interface: transmitted
 * frames are forwarded to it and frames it receives are redirected to the
 * Wi-Fi interface. Use this for Ethernet interfaces without a devicetree node
 * (e.g. the native_sim TAP interface); devicetree-instantiated devices can be
 * wired through the @c ethernet phandle instead.
 *
 * Pass @c NULL to detach the current data plane and revert to self-contained
 * mode.
 *
 * @param dev The emulated Wi-Fi device instance
 * @param eth_iface Ethernet interface to use as the data plane, or NULL to
 *		    detach
 *
 * @retval 0 on success
 * @retval -EINVAL if @p eth_iface is not a non-Wi-Fi Ethernet interface
 * @retval -errno if the receive redirect could not be installed
 */
int wifi_emul_set_eth_iface(const struct device *dev, struct net_if *eth_iface);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_WIFI_WIFI_EMUL_H_ */
