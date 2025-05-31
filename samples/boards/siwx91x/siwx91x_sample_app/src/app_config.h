#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Wi-Fi Network Name */
#define SSID "SiWx91x_AP"
/* Wi-Fi Password */
#define PSK  "12345678"
/* Wi-Fi Security Type:
 * WIFI_SECURITY_TYPE_NONE/WIFI_SECURITY_TYPE_WPA_PSK/WIFI_SECURITY_TYPE_PSK
 */
#define SECURITY_TYPE WIFI_SECURITY_TYPE_PSK
/* Wi-Fi channel */
#define CHANNEL_NO    WIFI_CHANNEL_ANY
#define STACK_SIZE    4096
#define STATUS_OK     0
#define STATUS_FAIL   -1
#define CMD_WAIT_TIME 180000

/* Hostname to ping */
#define HOSTNAME         "www.zephyrproject.org"
#define DNS_TIMEOUT      (20 * MSEC_PER_SEC)
#define PING_PACKET_SIZE 64
#define PING_PACKETS     30

typedef enum siwx91x_app_state_e {
    SIWX91X_WIFI_CONNECT_STATE = 0,
    SIWX91X_IP_CONFIG_STATE,
    SIWX91X_PING_HOSTNAME_STATE
} siwx91x_app_state_t;

static struct {
    const struct shell *sh;
    uint32_t scan_result;

    union {
        struct {
            uint8_t connecting: 1;
            uint8_t disconnecting: 1;
            uint8_t _unused: 6;
        };
        uint8_t all;
    };
} context;

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT)

void application_start();
static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
    struct net_if *iface);
static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb);
static sl_status_t network_event_handler(sl_net_event_t event, sl_status_t status, void *data,
    uint32_t data_length);

#endif /* _CONFIG_H_ */
