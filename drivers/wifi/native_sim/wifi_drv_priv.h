/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Private functions for native_sim wifi driver.
 */

#ifndef ZEPHYR_DRIVERS_WIFI_NATIVE_SIM_PRIV_H_
#define ZEPHYR_DRIVERS_WIFI_NATIVE_SIM_PRIV_H_

int wifi_iface_create(const char *if_name);
int wifi_iface_get_mac(const char *if_name, unsigned char *mac);
int wifi_wait_data(int fd);
ssize_t wifi_read_data(int fd, void *buf, size_t buf_len);
ssize_t wifi_write_data(int fd, void *buf, size_t buf_len);
int wifi_promisc_mode(const char *if_name, bool enable);

/* Interface to the host-compiled nl80211 driver adapter. */
void *host_wifi_drv_init(void *ctx, const char *iface_name,
			 const char *debug_file, int wpa_debug_level);
void host_wifi_drv_deinit(void *priv);
int host_wifi_drv_get_event_fd(void *priv);
int host_wifi_drv_get_wiphy(void *priv, void **bands, int *count);
void host_wifi_drv_free_bands(void *bands);
int host_wifi_drv_scan2(void *priv, void *params);
int host_wifi_drv_register_frame(void *priv, uint16_t type, const uint8_t *match,
				 size_t match_len, bool multicast);
int host_wifi_drv_get_capa(void *priv, void *capa);
void *host_wifi_drv_get_scan_results(void *priv);
void host_wifi_drv_free_scan_results(void *res);
int host_wifi_drv_scan_abort(void *priv);
int host_wifi_drv_associate(void *priv, void *param);

/* The following helpers forward the corresponding supplicant driver ops to
 * the host nl80211 driver. As this header is shared by Zephyr and host
 * compiled translation units, hostap specific types are passed as void * and
 * enums as unsigned int so that no hostap headers leak to the Zephyr side.
 */
int host_wifi_drv_deauthenticate(void *priv, const char *addr, unsigned short reason_code);
int host_wifi_drv_authenticate(void *priv, void *params, void *curr_bss);
int host_wifi_drv_set_key(void *priv, const unsigned char *ifname, unsigned int alg,
			  const unsigned char *addr, int key_idx, int set_tx,
			  const unsigned char *seq, size_t seq_len,
			  const unsigned char *key, size_t key_len, unsigned int key_flag);
int host_wifi_drv_set_supp_port(void *priv, int authorized, char *bssid);
int host_wifi_drv_signal_poll(void *priv, void *signal_info, unsigned char *bssid);
int host_wifi_drv_send_mlme(void *priv, const unsigned char *data, size_t data_len, int noack,
			    unsigned int freq, int no_cck, int offchanok,
			    unsigned int wait_time, int cookie);
int host_wifi_drv_get_conn_info(void *priv, void *conn_info);
int host_wifi_drv_disconnect(void *priv);

/* Plain C view of the host side connection status. The wpa_state value uses
 * the same numbering as enum wifi_iface_state so the Zephyr side can assign it
 * directly.
 */
struct host_iface_status {
	int wpa_state;
	char ssid[33];
	unsigned int ssid_len;
	unsigned char bssid[6];
	unsigned int freq;
	int signal;
	unsigned short beacon_interval;
	unsigned char dtim_period;
};

int host_wifi_drv_iface_status(void *priv, struct host_iface_status *out);

/* Event datagram sent from the host eloop thread over the event pipe to the
 * Zephyr reader (the RX thread). The event field holds an enum wpa_event_type
 * value, immediately followed by an event-specific payload struct, then any
 * variable-length IE/frame blobs. Both ends are the same arch/toolchain, so
 * plain structs have a matching layout; the (de)serialization uses memcpy.
 */
struct host_wifi_event {
	uint32_t event;
};

/* EVENT_AUTH payload: followed by ies_len bytes of IEs. */
struct host_event_auth {
	unsigned char peer[6];
	unsigned short auth_type;
	unsigned short auth_transaction;
	unsigned short status_code;
	unsigned short ies_len;
};

/* EVENT_ASSOC payload: followed by req_ies_len then resp_ies_len IE bytes. */
struct host_event_assoc {
	unsigned char addr[6];
	unsigned short freq;
	unsigned short req_ies_len;
	unsigned short resp_ies_len;
};

/* EVENT_ASSOC_REJECT payload: followed by resp_ies_len IE bytes. */
struct host_event_assoc_reject {
	unsigned char bssid[6];
	unsigned short status_code;
	unsigned short resp_ies_len;
};

/* EVENT_DEAUTH / EVENT_DISASSOC payload: followed by ie_len IE bytes. */
struct host_event_deauth {
	unsigned char addr[6];
	unsigned short reason_code;
	unsigned char locally_generated;
	unsigned short ie_len;
};

/* EVENT_RX_MGMT payload: followed by frame_len bytes of the management frame. */
struct host_event_rx_mgmt {
	int freq;
	int signal;
	unsigned short frame_len;
};

/* Cap for serialized IE/frame blobs in a single event datagram. */
#define HOST_WIFI_EVENT_MAX_IES 1024

#endif /* ZEPHYR_DRIVERS_WIFI_NATIVE_SIM_PRIV_H_ */
