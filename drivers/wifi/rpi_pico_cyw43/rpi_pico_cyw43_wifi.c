/*
 * Copyright (c) 2023 Beechwoods Software, Inc.
 * Copyright (c) 2026 Igalia S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>

#include "rpi_pico_cyw43_wifi.h"
#include "cyw43.h"
#include "cyw43_country.h"
#include "rpi_pico_cyw43_spi.h"

#if defined(CONFIG_NET_DHCPV4_SERVER)
BUILD_ASSERT(CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE >= 2048,
	"cyw43_wifi driver requires CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE >= 2048 "
	"when NET_DHCPV4_SERVER is enabled");
#include <zephyr/net/dhcpv4_server.h>
#endif

#define DT_DRV_COMPAT raspberrypi_cyw43_wifi
/*
 * Maximum number of queued CYW43439 IRQ events allowed.
 * NOTE: We'll handle the isr in a bottom half (thread) but interrupts
 * will be disabled until the bottom half is done, so we won't be
 * processing more than one interrupt at the same time.
 */
#define MAX_PENDING_EVENTS 1

LOG_MODULE_REGISTER(rpi_pico_cyw43_drv, CONFIG_WIFI_LOG_LEVEL);

enum cyw43_request {
	CYW43_REQ_ACTIVE_SCAN,
	CYW43_REQ_PASSIVE_SCAN,
	CYW43_REQ_CONNECT,
	CYW43_REQ_DISCONNECT,
	CYW43_REQ_ENABLE_AP,
	CYW43_REQ_DISABLE_AP,
	CYW43_REQ_IFACE_STATUS,
	CYW43_REQ_SET_PM,
};

struct cyw43_apsta_params {
	char ssid[WIFI_SSID_MAX_LEN + 1];
	char psk[WIFI_SAE_PSWD_MAX_LEN + 1];
	uint32_t security;
	uint32_t cyw43_security;
	uint32_t channel;
	uint8_t bssid[WIFI_MAC_ADDR_LEN];
};

struct cyw43_wifi_dev_priv {
	struct net_if *iface;
	scan_result_cb_t scan_cb;
	struct k_work_q wq;
	struct k_work req_work;
	struct cyw43_apsta_params apsta_params;
	bool sta_joined;
	uint32_t cyw43_pm;
	enum cyw43_request req;
	struct net_stats_wifi stats;
	uint8_t frame_buf[NET_ETH_MAX_FRAME_SIZE];
	struct async_context *ctx;
};

#define MAX_SCAN_RESULTS 100
struct wifi_scan_result scan_results[MAX_SCAN_RESULTS];

struct async_context async_ctx;
struct cyw43_wifi_dev_priv cyw43_wifi_dev_data;
struct gpio_callback cyw43_wifi_gpio_cb;


#define CYW43_WORKQUEUE_STACK_SIZE 2048
K_KERNEL_STACK_DEFINE(cyw43_wq_stack, CYW43_WORKQUEUE_STACK_SIZE);


/*
 * cyw43_ll_send_ethernet() in the cyw43 pico sdk core driver calls this
 * lwip function inconditionally, even if lwip is disabled, so we need
 * to add a stub here.
 */
struct pbuf {};
uint16_t pbuf_copy_partial(const struct pbuf *p, void *dataptr,
				uint16_t len, uint16_t offset)
{
	return 0;
}

K_EVENT_DEFINE(cyw43_event);
enum events {
	CYW43_EVT_LINK_UP = BIT(0),
};

/***********************
 * cyw43 core driver API
 ***********************/

void cyw43_cb_process_ethernet(void *cb_data, int itf, size_t len,
				const uint8_t *buf)
{
	struct net_pkt *pkt;
	int ret;

	LOG_DBG("itf: %d, len: %d)", itf, len);
	if (cyw43_wifi_dev_data.iface == NULL) {
		LOG_ERR("network interface unavailable");
		return;
	}
	pkt = net_pkt_rx_alloc_with_buffer(cyw43_wifi_dev_data.iface, len,
						AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("Failed to get net buffer");
		return;
	}
	ret = net_pkt_write(pkt, buf, len);
	if (ret) {
		LOG_ERR("net_pkt_write(): %d", ret);
		goto error;
	}
	ret = net_recv_data(cyw43_wifi_dev_data.iface, pkt);
	if (ret) {
		LOG_ERR("net_recv_data(): %d", ret);
		goto error;
	}
#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

	cyw43_wifi_dev_data.stats.bytes.received += len;
	cyw43_wifi_dev_data.stats.pkts.rx++;
	if (net_eth_is_addr_multicast(&hdr->dst)) {
		cyw43_wifi_dev_data.stats.multicast.rx++;
	} else if (net_eth_is_addr_broadcast(&hdr->dst)) {
		cyw43_wifi_dev_data.stats.broadcast.rx++;
	} else {
		cyw43_wifi_dev_data.stats.unicast.rx++;
	}
#endif
	return;

error:
#if defined(CONFIG_NET_STATISTICS_WIFI)
	cyw43_wifi_dev_data.stats.errors.rx++;
#endif
	net_pkt_unref(pkt);
}

void cyw43_cb_tcpip_init(cyw43_t *self, int itf)
{
	LOG_DBG("itf: %d", itf);
}

void cyw43_cb_tcpip_deinit(cyw43_t *self, int itf)
{
	LOG_DBG("itf: %d", itf);
}

void cyw43_cb_tcpip_set_link_up(cyw43_t *self, int itf)
{
	LOG_DBG("itf: %d", itf);
	k_event_post(&cyw43_event, CYW43_EVT_LINK_UP);
}

/*
 * Triggered by the cyw43 core driver on a network disconnect event
 * (asynchronous). Go through the usual disconnect path here to do the
 * appropriate cleanup and reset the cyw43 core driver state.
 */
void cyw43_cb_tcpip_set_link_down(cyw43_t *self, int itf)
{
	LOG_DBG("itf: %d", itf);
	k_mutex_lock(&cyw43_wifi_dev_data.ctx->mutex, K_FOREVER);
	self->wifi_join_state = 0;
	cyw43_wifi_dev_data.req = CYW43_REQ_DISCONNECT;
	k_work_submit_to_queue(&cyw43_wifi_dev_data.wq,
			&cyw43_wifi_dev_data.req_work);
	k_mutex_unlock(&cyw43_wifi_dev_data.ctx->mutex);
}

/*
 * Generate a random unicast and locally-administered mac address if one
 * is not set in otp.
 */
void cyw43_hal_generate_laa_mac(int interface, uint8_t mac[WIFI_MAC_ADDR_LEN])
{
	sys_rand_get(mac, WIFI_MAC_ADDR_LEN);
	mac[0] = (mac[0] & (uint8_t)~1) | (1 << 1);
}

void cyw43_hal_get_mac(int interface, uint8_t mac[WIFI_MAC_ADDR_LEN])
{
#ifdef CONFIG_WIFI_RPI_PICO_CYW43_USE_FIXED_MAC_ADDRESS
#ifdef CONFIG_WIFI_RPI_PICO_CYW43_FIXED_MAC_ADDRESS
	int ret = sscanf(CONFIG_WIFI_RPI_PICO_CYW43_FIXED_MAC_ADDRESS,
		"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	if (ret != WIFI_MAC_ADDR_LEN) {
		LOG_ERR("Error parsing CONFIG_WIFI_FIXED_MAC_ADDRESS");
	}
#else
	LOG_WRN("No wifi MAC address configured "
		"(CONFIG_WIFI_RPI_PICO_CYW43_FIXED_MAC_ADDRESS)"
		", generating a random one");
	cyw43_hal_generate_laa_mac(interface, mac);
#endif
#endif
}

void cyw43_post_poll_hook(void)
{
	cyw43_irq_enable(&cyw43_wifi_dev_cfg, true);
}

/************************
 * Wifi driver definition
 ************************/

static inline bool mac_addr_specified(uint8_t *addr)
{
	int i;

	for (i = 0; i < WIFI_MAC_ADDR_LEN; i++) {
		if (addr[i] != 0) {
			return true;
		}
	}

	return false;
}

static inline uint32_t cyw43_security_to_zephyr(uint32_t cyw43_sec)
{
	uint32_t sec = (cyw43_sec | 0x00400000) & 0xFFFFFFFE;
	uint32_t ret;

	switch (sec) {
	case CYW43_AUTH_OPEN:
		ret = WIFI_SECURITY_TYPE_NONE;
		break;
	case CYW43_AUTH_WPA_TKIP_PSK:
		ret = WIFI_SECURITY_TYPE_WPA_PSK;
		break;
	case CYW43_AUTH_WPA2_AES_PSK:
		ret = WIFI_SECURITY_TYPE_PSK;
		break;
	case CYW43_AUTH_WPA2_MIXED_PSK:
		ret = WIFI_SECURITY_TYPE_PSK;
		break;
	default:
		ret = WIFI_SECURITY_TYPE_UNKNOWN;
	}

	return ret;
}

/*
 * Returns true if scan results a and b are the same, false
 * otherwise. For the comparison, only the ssid, channel, mac and
 * security fields are checked.
 */
bool scan_result_eq(struct wifi_scan_result *a, struct wifi_scan_result *b)
{
	if ((a->ssid_length == b->ssid_length)                   &&
		(strncmp(a->ssid, b->ssid, a->ssid_length) == 0) &&
		(a->channel == b->channel)                       &&
		(memcmp(a->mac, b->mac, WIFI_MAC_ADDR_LEN) == 0) &&
		(a->security == b->security)) {
		return true;
	}

	return false;
}

/*
 * Checks if the scan result is already in the scan_results
 * array and adds it to the array if it isn't.
 *
 * Returns:
 *  1 if the result is already in scan_results.
 *  0 if it's not and it was added to scan_results.
 *  -1 if it's not but scan_results is already full.
 */
int filter_scan_result(struct wifi_scan_result *result)
{
	for (int i = 0; i < MAX_SCAN_RESULTS; i++) {
		if (scan_results[i].ssid_length == 0) {
			/* Match not found: add to list */
			scan_results[i] = *result;
			return 0;
		}
		if (scan_result_eq(&scan_results[i], result)) {
			return 1;
		}
	}

	return -1;
}

static int parse_cyw43_scan_result(void *env,
				const cyw43_ev_scan_result_t *result)
{
	struct wifi_scan_result scan_result;
	struct cyw43_wifi_dev_priv *dev_data = env;

	if (!result) {
		return -1;
	}

	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	if (dev_data->scan_cb != NULL) {
		memset(&scan_result, 0, sizeof(struct wifi_scan_result));
		scan_result.mfp = WIFI_MFP_UNKNOWN;
		scan_result.band = WIFI_FREQ_BAND_2_4_GHZ;
		scan_result.ssid_length = strnlen(result->ssid,
						WIFI_SSID_MAX_LEN);
		memcpy(scan_result.ssid, result->ssid,
						sizeof(scan_result.ssid));
		scan_result.ssid[sizeof(scan_result.ssid) - 1] = '\0';
		scan_result.channel = result->channel;
		scan_result.rssi = result->rssi;
		scan_result.mac_length = WIFI_MAC_ADDR_LEN;
		memcpy(scan_result.mac, result->bssid, WIFI_MAC_ADDR_LEN);
		scan_result.security = cyw43_security_to_zephyr(result->auth_mode);
		if (!filter_scan_result(&scan_result)) {
			dev_data->scan_cb(cyw43_wifi_dev_data.iface, 0,
							&scan_result);
		}
	}
	k_mutex_unlock(&dev_data->ctx->mutex);

	return 0;
}

static int cyw43_wifi_do_scan(struct cyw43_wifi_dev_priv *dev_data, bool active)
{
	static cyw43_wifi_scan_options_t scan_options;
	int ret = 0;
	int i;

	LOG_DBG("%s", active ? "active scan" : "passive scan");
	if (!cyw43_wifi_scan_active(&cyw43_state)) {
		memset(&scan_options, 0, sizeof(cyw43_wifi_scan_options_t));
		scan_options.scan_type = (active ? 0 : 1);
		memset(scan_results, 0, sizeof(scan_results));
		ret = cyw43_wifi_scan(&cyw43_state, &scan_options, dev_data,
					parse_cyw43_scan_result);
		if (ret) {
			LOG_ERR("Failed to start scan: %d", ret);
			return ret;
		}
	} else {
		LOG_DBG("Scan already active");
		return ret;
	}
	/* Poll until the scan process is done. Max wait: 2.5 secs. */
	for (i = 0; i < 50; i++) {
		if (!cyw43_wifi_scan_active(&cyw43_state)) {
			break;
		}
		k_sleep(K_MSEC(50));
	}
	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	dev_data->scan_cb(dev_data->iface, 0, NULL);
	dev_data->scan_cb = NULL;
	k_mutex_unlock(&dev_data->ctx->mutex);

	return 0;
}

static int cyw43_wifi_connect(struct cyw43_wifi_dev_priv *dev_data)
{
	char ssid[WIFI_SSID_MAX_LEN + 1];
	char key[WIFI_SAE_PSWD_MAX_LEN + 1];
	int ssid_len;
	int key_len;
	int security;
	int channel;
	const uint8_t *bssid = NULL;
	int ret = 0;

	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	ssid_len = strlen(dev_data->apsta_params.ssid);
	memcpy(ssid, dev_data->apsta_params.ssid, ssid_len + 1);
	key_len = strlen(dev_data->apsta_params.psk);
	memcpy(key, dev_data->apsta_params.psk, key_len + 1);
	channel = dev_data->apsta_params.channel ?
		dev_data->apsta_params.channel : CYW43_CHANNEL_NONE;
	security = dev_data->apsta_params.cyw43_security;
	if (mac_addr_specified(dev_data->apsta_params.bssid)) {
		bssid = dev_data->apsta_params.bssid;
	}
	k_mutex_unlock(&dev_data->ctx->mutex);

	for (int i = 0; i < CONFIG_WIFI_RPI_PICO_CYW43_JOIN_ATTEMPTS; i++) {
		uint32_t event;

		ret = cyw43_wifi_join(&cyw43_state, ssid_len, ssid, key_len,
					key, security, bssid, channel);
		if (ret) {
			LOG_ERR("cyw43_wifi_join(): %d", ret);
			continue;
		}

		/*
		 * cyw43_wifi_link_status() doesn't provide a reliable
		 * status in all cases when the join operation fails:
		 *
		 * - cyw43_cb_process_async_event() will set
		 *   WIFI_JOIN_STATE_ACTIVE (and even
		 *   WIFI_JOIN_STATE_LINK and WIFI_JOIN_STATE_AUTH) in
		 *   wifi_join_state even when the authentication fails.
		 * - When wifi_join_state is set to
		 *   WIFI_JOIN_STATE_BADAUTH, it gets clobbered soon
		 *   afterwards in the next
		 *   cyw43_cb_process_async_event() call.
		 *
		 * Instead, rely on the cyw43_cb_tcpip_set_link_up()
		 * callback to detect a successful network join.
		 */
		event = k_event_wait_safe(&cyw43_event, CYW43_EVT_LINK_UP,
			false, K_SECONDS(CONFIG_WIFI_RPI_PICO_CYW43_LINK_UP_WAIT_SECS));
		if (event == 0) {
			int status = cyw43_wifi_link_status(&cyw43_state,
							CYW43_ITF_STA);
			/*
			 * The cyw43 driver may keep retrying forever in
			 * case of an authentication failure. Explicitly
			 * stop the retrying here.
			 */
			ret = cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
			if (ret) {
				LOG_ERR("cyw43_wifi_leave(): %d", ret);
			}
			if (status == CYW43_LINK_NONET) {
				/* AP not found: don't retry */
				ret = WIFI_STATUS_CONN_AP_NOT_FOUND;
				break;
			}
			ret = WIFI_STATUS_CONN_FAIL;
		} else {
			ret = WIFI_STATUS_CONN_SUCCESS;
			break;
		}
	}
	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	if (ret) {
		net_if_dormant_on(dev_data->iface);
		dev_data->sta_joined = false;
	} else {
		dev_data->sta_joined = true;
		net_if_dormant_off(dev_data->iface);
#if defined(CONFIG_WIFI_STA_AUTO_DHCPV4)
		net_dhcpv4_restart(dev_data->iface);
#endif
	}
	k_mutex_unlock(&dev_data->ctx->mutex);

	return ret;
}

static int cyw43_wifi_enable_ap(struct cyw43_wifi_dev_priv *dev_data)
{
	int auth_type;

	switch (dev_data->apsta_params.security) {
	case WIFI_SECURITY_TYPE_NONE:
		auth_type = CYW43_AUTH_OPEN;
		break;
	case WIFI_SECURITY_TYPE_WPA_PSK:
		auth_type = CYW43_AUTH_WPA_TKIP_PSK;
		break;
	case WIFI_SECURITY_TYPE_PSK:
		auth_type = CYW43_AUTH_WPA2_AES_PSK;
		break;
	default:
		LOG_ERR("Security type not supported: %d",
			dev_data->apsta_params.security);
		return -EINVAL;
	}
	cyw43_wifi_ap_set_ssid(&cyw43_state,
			strlen(dev_data->apsta_params.ssid),
			dev_data->apsta_params.ssid);
	if (auth_type != CYW43_AUTH_OPEN) {
		cyw43_wifi_ap_set_password(&cyw43_state,
			strlen(dev_data->apsta_params.psk),
			dev_data->apsta_params.psk);
	}
	cyw43_wifi_ap_set_channel(&cyw43_state,
				(dev_data->apsta_params.channel == 0) ?
				CYW43_CHANNEL_NONE :
				dev_data->apsta_params.channel);
	cyw43_wifi_ap_set_auth(&cyw43_state, auth_type);
	cyw43_wifi_set_up(&cyw43_state, CYW43_ITF_AP, true,
			CYW43_COUNTRY_WORLDWIDE);
	WRITE_BIT(cyw43_state.itf_state, CYW43_ITF_STA, 0);

#if defined(CONFIG_NET_DHCPV4_SERVER)
	static struct in_addr netmask;
	struct in_addr base_addr;
	struct in_addr addr;

	if (net_addr_pton(AF_INET,
			CONFIG_WIFI_RPI_PICO_CYW43_AP_DHCPV4_ADDRESS,
			&addr)) {
		LOG_ERR("Invalid address: %s",
			CONFIG_WIFI_RPI_PICO_CYW43_AP_DHCPV4_ADDRESS);
		return -EINVAL;
	}
	if (net_addr_pton(AF_INET,
			CONFIG_WIFI_RPI_PICO_CYW43_AP_DHCPV4_NETMASK,
			&netmask)) {
		LOG_ERR("Invalid netmask: %s",
			CONFIG_WIFI_RPI_PICO_CYW43_AP_DHCPV4_NETMASK);
		return -EINVAL;
	}
	if (net_addr_pton(AF_INET,
			CONFIG_WIFI_RPI_PICO_CYW43_AP_DHCPV4_BASE_ADDRESS,
			&base_addr)) {
		LOG_ERR("Invalid base address: %s",
			CONFIG_WIFI_RPI_PICO_CYW43_AP_DHCPV4_BASE_ADDRESS);
		return -EINVAL;
	}
	LOG_INF("Server IP address: %s, netmask: %s, base address: %s",
		CONFIG_WIFI_RPI_PICO_CYW43_AP_DHCPV4_ADDRESS,
		CONFIG_WIFI_RPI_PICO_CYW43_AP_DHCPV4_NETMASK,
		CONFIG_WIFI_RPI_PICO_CYW43_AP_DHCPV4_BASE_ADDRESS);

	if (!net_if_ipv4_addr_add(dev_data->iface, &addr, NET_ADDR_MANUAL, 0)) {
		LOG_ERR("net_if_ipv4_addr_add()");
	}
	net_if_ipv4_set_netmask_by_addr(dev_data->iface, &addr, &netmask);
#endif

	net_if_dormant_off(dev_data->iface);

#if defined(CONFIG_NET_DHCPV4_SERVER)
	int ret;

	LOG_INF("Starting dhcpv4 server");
	ret = net_dhcpv4_server_start(dev_data->iface, &base_addr);
	if (ret) {
		LOG_ERR("net_dhcpv4_server_start(): %d", ret);
		return ret;
	}
#endif

	return 0;
}

static int cyw43_wifi_disable_ap(struct cyw43_wifi_dev_priv *dev_data)
{
#if defined(CONFIG_NET_DHCPV4_SERVER)
	struct in_addr addr;
	int ret = net_dhcpv4_server_stop(dev_data->iface);

	if (ret) {
		LOG_ERR("net_dhcpv4_server_stop(): %d", ret);
		return ret;
	}
	net_addr_pton(AF_INET, CONFIG_WIFI_RPI_PICO_CYW43_AP_DHCPV4_ADDRESS,
			&addr);
	if (!net_if_ipv4_addr_rm(dev_data->iface, &addr)) {
		LOG_ERR("net_if_ipv4_addr_rm()");
	}
#endif
	net_if_dormant_on(dev_data->iface);
	cyw43_wifi_set_up(&cyw43_state, CYW43_ITF_AP, false,
			CYW43_COUNTRY_WORLDWIDE);
	WRITE_BIT(cyw43_state.itf_state, CYW43_ITF_STA, 1);

	return 0;
}

static int cyw43_wifi_disconnect(struct cyw43_wifi_dev_priv *dev_data)
{
	int ret = cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);

	if (ret) {
		LOG_ERR("cyw43_wifi_leave(): %d", ret);
		return -EIO;
	}
	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	dev_data->sta_joined = false;
#if defined(CONFIG_WIFI_STA_AUTO_DHCPV4)
	net_dhcpv4_stop(dev_data->iface);
#endif
	net_if_dormant_on(dev_data->iface);
	k_mutex_unlock(&dev_data->ctx->mutex);

	return 0;
}

static int cyw43_wifi_mgmt_scan(const struct device *dev,
				struct net_if *iface,
				struct wifi_scan_params *params,
				scan_result_cb_t cb)
{
	struct cyw43_wifi_dev_priv *dev_data = dev->data;

	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	dev_data->scan_cb = cb;
	dev_data->req = (params->scan_type == WIFI_SCAN_TYPE_ACTIVE) ?
			CYW43_REQ_ACTIVE_SCAN :
			CYW43_REQ_PASSIVE_SCAN;
	k_work_submit_to_queue(&dev_data->wq, &dev_data->req_work);
	k_mutex_unlock(&dev_data->ctx->mutex);

	return 0;
}

static int cyw43_wifi_mgmt_connect(const struct device *dev,
				struct net_if *iface,
				struct wifi_connect_req_params *params)
{
	struct cyw43_wifi_dev_priv *dev_data = dev->data;
	struct cyw43_apsta_params *apsta_params = &dev_data->apsta_params;
	int ret;

	if (cyw43_state.itf_state & BIT(CYW43_ITF_AP)) {
		LOG_ERR("Disable access point mode before initiating "
			"a client connection");
		ret = -EBUSY;
		goto error;
	}
	if (cyw43_state.itf_state & BIT(CYW43_ITF_STA)) {
		if (dev_data->sta_joined) {
			LOG_ERR("Already connected");
			ret = -EALREADY;
			goto error;
		}
	} else {
		LOG_ERR("Unsupported itf_state: %#x", cyw43_state.itf_state);
		ret = -EIO;
		goto error;
	}

	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	switch (params->security) {
	case WIFI_SECURITY_TYPE_NONE:
		apsta_params->cyw43_security = CYW43_AUTH_OPEN;
		break;
	case WIFI_SECURITY_TYPE_SAE:
		/* WPA3-SAE */
		apsta_params->cyw43_security = CYW43_AUTH_WPA3_SAE_AES_PSK;
		memcpy(apsta_params->psk, params->sae_password,
			sizeof(apsta_params->psk));
		apsta_params->psk[sizeof(apsta_params->psk) - 1] = '\0';
		break;
	case WIFI_SECURITY_TYPE_PSK:
		/* WPA2-PSK */
		apsta_params->cyw43_security = CYW43_AUTH_WPA2_AES_PSK;
		memcpy(apsta_params->psk, params->psk,
			sizeof(apsta_params->psk));
		apsta_params->psk[sizeof(apsta_params->psk) - 1] = '\0';
		break;
	case WIFI_SECURITY_TYPE_WPA_PSK:
		/* WPA-PSK */
		apsta_params->cyw43_security = CYW43_AUTH_WPA_TKIP_PSK;
		memcpy(apsta_params->psk, params->psk,
			sizeof(apsta_params->psk));
		apsta_params->psk[sizeof(apsta_params->psk) - 1] = '\0';
		break;
	default:
		LOG_ERR("Security type not supported: %d", params->security);
		apsta_params->psk[0] = '\0';
		k_mutex_unlock(&dev_data->ctx->mutex);
		ret = -EINVAL;
		goto error;
	}
	apsta_params->security = params->security;
	memcpy(apsta_params->ssid, params->ssid, sizeof(apsta_params->ssid));
	apsta_params->ssid[sizeof(apsta_params->ssid) - 1] = '\0';
	apsta_params->channel = params->channel;
	memcpy(apsta_params->bssid, params->bssid, sizeof(apsta_params->bssid));
	dev_data->req = CYW43_REQ_CONNECT;
	k_work_submit_to_queue(&dev_data->wq, &dev_data->req_work);
	k_mutex_unlock(&dev_data->ctx->mutex);

	return 0;

error:
	wifi_mgmt_raise_connect_result_event(dev_data->iface,
					WIFI_STATUS_CONN_FAIL);
	return ret;

}

static int cyw43_wifi_mgmt_disconnect(const struct device *dev,
					struct net_if *iface)
{
	struct cyw43_wifi_dev_priv *dev_data = dev->data;
	int link_status;

	if ((cyw43_state.itf_state & BIT(CYW43_ITF_STA)) == 0) {
		LOG_ERR("Not in STA mode");
		wifi_mgmt_raise_disconnect_result_event(dev_data->iface,
							-EBUSY);
		return -EBUSY;
	}
	link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
	if (link_status <= 0) {
		LOG_ERR("Not connected");
		wifi_mgmt_raise_disconnect_result_event(dev_data->iface,
							-EBUSY);
		return -EBUSY;
	}

	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	dev_data->req = CYW43_REQ_DISCONNECT;
	k_work_submit_to_queue(&dev_data->wq, &dev_data->req_work);
	k_mutex_unlock(&dev_data->ctx->mutex);

	return 0;
}

static int cyw43_wifi_mgmt_ap_enable(const struct device *dev,
				struct net_if *iface,
				struct wifi_connect_req_params *params)
{
	struct cyw43_wifi_dev_priv *dev_data = dev->data;
	struct cyw43_apsta_params *apsta_params = &dev_data->apsta_params;

	if (cyw43_state.itf_state & BIT(CYW43_ITF_AP)) {
		LOG_ERR("Already in AP mode");
		wifi_mgmt_raise_ap_enable_result_event(dev_data->iface,
							-EALREADY);
		return -EALREADY;
	}
	if (cyw43_state.itf_state & BIT(CYW43_ITF_STA)) {
		int link_status = cyw43_wifi_link_status(&cyw43_state,
							CYW43_ITF_STA);
		if (link_status == CYW43_LINK_JOIN ||
			link_status == CYW43_LINK_NONET) {
			LOG_ERR("Currently connected as a client. Please "
				"disconnect before enabling AP");
			return -EBUSY;
		}
	}
	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	memcpy(apsta_params->ssid, params->ssid, sizeof(apsta_params->ssid));
	apsta_params->ssid[sizeof(apsta_params->ssid) - 1] = '\0';
	memcpy(apsta_params->psk, params->psk, sizeof(apsta_params->psk));
	apsta_params->psk[sizeof(apsta_params->psk) - 1] = '\0';
	apsta_params->security = params->security;
	apsta_params->channel = params->channel;
	dev_data->req = CYW43_REQ_ENABLE_AP;
	k_work_submit_to_queue(&dev_data->wq, &dev_data->req_work);
	k_mutex_unlock(&dev_data->ctx->mutex);

	return 0;
}

static int cyw43_wifi_mgmt_ap_disable(const struct device *dev,
					struct net_if *iface)
{
	struct cyw43_wifi_dev_priv *dev_data = dev->data;

	if ((cyw43_state.itf_state & BIT(CYW43_ITF_AP)) == 0) {
		LOG_ERR("Not in AP mode");
		wifi_mgmt_raise_ap_disable_result_event(dev_data->iface,
							-EALREADY);
		return -EALREADY;
	}
	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	dev_data->req = CYW43_REQ_DISABLE_AP;
	k_work_submit_to_queue(&dev_data->wq, &dev_data->req_work);
	k_mutex_unlock(&dev_data->ctx->mutex);

	return 0;
}

static int cyw43_wifi_mgmt_set_pm(const struct device *dev,
					struct net_if *iface,
					struct wifi_ps_params *params)
{
	struct cyw43_wifi_dev_priv *dev_data = dev->data;
	uint8_t pm_mode;
	uint16_t pm2_sleep_ret_ms = 0;
	uint8_t li_beacon_period = 0;
	uint8_t li_dtim_period = 0;
	uint8_t li_assoc = 0;

	if (params->type == WIFI_PS_PARAM_STATE) {
		if (params->enabled == WIFI_PS_DISABLED) {
			pm_mode = CYW43_NO_POWERSAVE_MODE;
		} else {
			/*
			 * Set PM2 by default if no powersave mode is
			 * configured.
			 */
			if (CONFIG_WIFI_RPI_PICO_CYW43_PM_MODE ==
				CYW43_NO_POWERSAVE_MODE) {
				pm_mode = CYW43_PM2_POWERSAVE_MODE;
			} else {
				pm_mode = CONFIG_WIFI_RPI_PICO_CYW43_PM_MODE;
			}
		}
		pm2_sleep_ret_ms = CONFIG_WIFI_RPI_PICO_CYW43_PM2_SLEEP_RET_MS;
		li_beacon_period = CONFIG_WIFI_RPI_PICO_CYW43_PM_LI_BEACON_PERIOD;
		li_dtim_period = CONFIG_WIFI_RPI_PICO_CYW43_PM_LI_DTIM_PERIOD;
		li_assoc = CONFIG_WIFI_RPI_PICO_CYW43_PM_LI_ASSOC;
	} else {
		params->fail_reason = WIFI_PS_PARAM_FAIL_OPERATION_NOT_SUPPORTED;
		return -ENOTSUP;
	}

	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
	dev_data->cyw43_pm = cyw43_pm_value(pm_mode, pm2_sleep_ret_ms,
				li_beacon_period, li_dtim_period, li_assoc);
	dev_data->req = CYW43_REQ_SET_PM;
	k_work_submit_to_queue(&dev_data->wq, &dev_data->req_work);
	k_mutex_unlock(&dev_data->ctx->mutex);

	return 0;
}

#define PM_MASK_MODE             0x0000000f
#define PM_MASK_PM2_SLEEP_RET_MS 0x00000ff0
#define PM_MASK_LI_BEACON_PERIOD 0x0000f000
#define PM_MASK_LI_DTIM_PERIOD   0x000f0000
#define PM_MASK_LI_WAKE_INTERVAL 0x00f00000
static int cyw43_wifi_mgmt_get_pm(const struct device *dev,
				struct net_if *iface,
				struct wifi_ps_config *config)
{
	struct wifi_ps_params *ps_params = &config->ps_params;
	uint32_t pm;
	int ret;

	ret = cyw43_wifi_get_pm(&cyw43_state, &pm);
	if (ret) {
		LOG_ERR("cyw43_wifi_get_pm(): %d", ret);
		return -EIO;
	}
	if (FIELD_GET(PM_MASK_MODE, pm) == CYW43_NO_POWERSAVE_MODE) {
		ps_params->enabled = false;
	} else {
		ps_params->enabled = true;
		ps_params->timeout_ms =
			FIELD_GET(PM_MASK_PM2_SLEEP_RET_MS, pm) * 10;
		ps_params->listen_interval =
			FIELD_GET(PM_MASK_LI_BEACON_PERIOD, pm);
		if (FIELD_GET(PM_MASK_LI_DTIM_PERIOD, pm)) {
			ps_params->wakeup_mode = WIFI_PS_WAKEUP_MODE_DTIM;
		} else {
			ps_params->wakeup_mode = WIFI_PS_WAKEUP_MODE_LISTEN_INTERVAL;
		}
	}

	return 0;
}

/* Get phy rate (in 500 kbps units): ioctl 12 (<< 1). Bit 0 = 0 (get) */
#define CYW43_IOCTL_GET_RATE 0x18
static int cyw43_wifi_mgmt_iface_status(const struct device *dev,
					struct net_if *iface,
					struct wifi_iface_status *status)
{
	struct cyw43_wifi_dev_priv *dev_data = dev->data;
	uint32_t phy_rate_kbps = 0;
	uint32_t pm;
	int ret;

	if (cyw43_state.itf_state & BIT(CYW43_ITF_AP)) {
		status->iface_mode = WIFI_MODE_AP;
	} else if (cyw43_state.itf_state & BIT(CYW43_ITF_STA)) {
		status->iface_mode = WIFI_MODE_INFRA;
	} else {
		status->iface_mode = WIFI_MODE_UNKNOWN;
	}

	cyw43_ioctl(&cyw43_state, CYW43_IOCTL_GET_RATE,
		sizeof(status->current_phy_tx_rate),
		(uint8_t *)&phy_rate_kbps,
		(status->iface_mode == WIFI_MODE_INFRA) ?
					CYW43_ITF_STA :
					CYW43_ITF_AP);
	status->current_phy_tx_rate = phy_rate_kbps * 500 / 1000.0f;
	/* Resolve link mode from the current phy rate */
	if (status->current_phy_tx_rate < 6) {
		status->link_mode = WIFI_1;
	} else if (status->current_phy_tx_rate < 54) {
		status->link_mode = WIFI_3;
	} else {
		status->link_mode = WIFI_4;
	}
	status->band = WIFI_FREQ_BAND_2_4_GHZ;

	if (status->iface_mode == WIFI_MODE_INFRA) {
		switch (cyw43_wifi_link_status(&cyw43_state,
						CYW43_ITF_STA)) {
		case CYW43_LINK_JOIN:
			status->state = WIFI_STATE_COMPLETED;
			break;
		case CYW43_LINK_NONET:
			status->state = WIFI_STATE_INACTIVE;
			break;
		default:
			status->state = WIFI_STATE_DISCONNECTED;
		}
	} else if (status->iface_mode == WIFI_MODE_AP) {
		status->state = WIFI_STATE_COMPLETED;
	} else {
		status->state = WIFI_STATE_DISCONNECTED;
	}
	cyw43_wifi_get_bssid(&cyw43_state, status->bssid);
	cyw43_ioctl(&cyw43_state, CYW43_IOCTL_GET_CHANNEL,
		sizeof(status->channel),
		(uint8_t *)&status->channel,
		(status->iface_mode == WIFI_MODE_INFRA) ?
					CYW43_ITF_STA :
					CYW43_ITF_AP);

	if (status->state != WIFI_STATE_COMPLETED) {
		status->ssid[0] = '\0';
		status->ssid_len = 0;
	} else {
		if (status->iface_mode == WIFI_MODE_INFRA) {
			cyw43_wifi_get_rssi(&cyw43_state,
				(int32_t *)&(status->rssi));
			if (status->state == WIFI_STATE_COMPLETED) {
				memcpy(status->ssid, dev_data->apsta_params.ssid,
					sizeof(status->ssid));
				status->ssid[sizeof(status->ssid) - 1] = '\0';
			} else {
				status->ssid[0] = '\0';
			}
			status->ssid_len = strlen(status->ssid);
			status->security = dev_data->apsta_params.security;
		} else if (status->iface_mode == WIFI_MODE_AP) {
			memcpy(status->ssid, dev_data->apsta_params.ssid,
				sizeof(status->ssid));
			status->ssid[sizeof(status->ssid) - 1] = '\0';
			status->ssid_len = strlen(dev_data->apsta_params.ssid);
			status->security = dev_data->apsta_params.security;
		}
	}

	ret = cyw43_wifi_get_pm(&cyw43_state, &pm);
	if (ret) {
		LOG_ERR("cyw43_wifi_get_pm(): %d", ret);
	} else {
		LOG_DBG("pm_params.pm_in: %#x", pm);
		if ((pm & 0x0000000f) != CYW43_NO_POWERSAVE_MODE) {
			status->beacon_interval = FIELD_GET(0x0000f000, pm);
			status->dtim_period = FIELD_GET(0x000f0000, pm);
		}
	}
	status->twt_capable = false;

	return 0;
}

static void cyw43_wifi_request_work(struct k_work *item)
{
	struct cyw43_wifi_dev_priv *dev_data;
	int ret;

	dev_data = CONTAINER_OF(item, struct cyw43_wifi_dev_priv, req_work);
	switch (dev_data->req) {
	case CYW43_REQ_PASSIVE_SCAN:
		ret = cyw43_wifi_do_scan(dev_data, false);
		if (ret) {
			LOG_ERR("wifi scan error: %d", ret);
		}
		break;
	case CYW43_REQ_ACTIVE_SCAN:
		ret = cyw43_wifi_do_scan(dev_data, true);
		if (ret) {
			LOG_ERR("wifi scan error: %d", ret);
		}
		break;
	case CYW43_REQ_CONNECT:
		ret = cyw43_wifi_connect(dev_data);
		if (ret) {
			LOG_ERR("wifi connect error: %d", ret);
			if (ret < 0) {
				ret = WIFI_STATUS_CONN_FAIL;
			}
		}
		wifi_mgmt_raise_connect_result_event(dev_data->iface, ret);
		break;
	case CYW43_REQ_DISCONNECT:
		ret = cyw43_wifi_disconnect(dev_data);
		if (ret) {
			LOG_ERR("wifi disconnect error: %d", ret);
		}
		wifi_mgmt_raise_disconnect_result_event(dev_data->iface, ret);
		break;
	case CYW43_REQ_ENABLE_AP:
		ret = cyw43_wifi_enable_ap(dev_data);
		if (ret) {
			LOG_ERR("error enabling AP: %d", ret);
		}
		wifi_mgmt_raise_ap_enable_result_event(dev_data->iface, ret);
		break;
	case CYW43_REQ_DISABLE_AP:
		ret = cyw43_wifi_disable_ap(dev_data);
		if (ret) {
			LOG_ERR("error disabling AP: %d", ret);
		}
		wifi_mgmt_raise_ap_disable_result_event(dev_data->iface, ret);
		break;
	case CYW43_REQ_SET_PM:
		ret = cyw43_wifi_pm(&cyw43_state, dev_data->cyw43_pm);
		if (ret) {
			LOG_ERR("wifi set PM error: %d", ret);
		}
		break;
	default:
		LOG_DBG("Unsupported request: %d", dev_data->req);
		break;
	}
}

static void cyw43_wifi_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct cyw43_wifi_dev_priv *dev_data = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;
	dev_data->iface = iface;
	net_if_set_link_addr(iface, cyw43_state.mac, WIFI_MAC_ADDR_LEN,
				NET_LINK_ETHERNET);
	ethernet_init(iface);
	net_eth_carrier_on(iface);
	net_if_dormant_on(iface);

	k_mutex_unlock(&dev_data->ctx->mutex);
}

static int cyw43_wifi_send(const struct device *dev, struct net_pkt *pkt)
{
	struct cyw43_wifi_dev_priv *dev_data = dev->data;
	int pkt_len = net_pkt_get_len(pkt);
	int iface_mode;
	int ret;

	ret = net_pkt_read(pkt, dev_data->frame_buf, pkt_len);
	if (ret) {
		LOG_ERR("net_pkt_read(): %d", ret);
		ret = -EIO;
		goto error;
	}

	iface_mode = (cyw43_state.itf_state & BIT(CYW43_ITF_AP)) ?
		CYW43_ITF_AP : CYW43_ITF_STA;
	ret = cyw43_send_ethernet(&cyw43_state, iface_mode, pkt_len,
				(void *)(dev_data->frame_buf), false);
	if (ret) {
		LOG_ERR("cyw43_send_ethernet(): %d", ret);
		goto error;
	}

#if defined(CONFIG_NET_STATISTICS_WIFI)
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);

	dev_data->stats.bytes.sent += pkt_len;
	dev_data->stats.pkts.tx++;
	if (net_eth_is_addr_multicast(&hdr->dst)) {
		dev_data->stats.multicast.tx++;
	} else if (net_eth_is_addr_broadcast(&hdr->dst)) {
		dev_data->stats.broadcast.tx++;
	} else {
		dev_data->stats.unicast.tx++;
	}
#endif
	return 0;

error:
#if defined(CONFIG_NET_STATISTICS_WIFI)
	dev_data->stats.errors.tx++;
#endif

	return ret;
}

#if defined(CONFIG_NET_STATISTICS_WIFI)
static int cyw43_wifi_mgmt_stats(const struct device *dev,
				struct net_if *iface,
				struct net_stats_wifi *stats)
{
	struct cyw43_wifi_dev_priv *dev_data = dev->data;

	stats->bytes.received = dev_data->stats.bytes.received;
	stats->bytes.sent = dev_data->stats.bytes.sent;
	stats->pkts.rx = dev_data->stats.pkts.rx;
	stats->pkts.tx = dev_data->stats.pkts.tx;
	stats->errors.rx = dev_data->stats.errors.rx;
	stats->errors.tx = dev_data->stats.errors.tx;
	stats->broadcast.rx = dev_data->stats.broadcast.rx;
	stats->broadcast.tx = dev_data->stats.broadcast.tx;
	stats->multicast.rx = dev_data->stats.multicast.rx;
	stats->multicast.tx = dev_data->stats.multicast.tx;
	stats->sta_mgmt.beacons_rx = dev_data->stats.sta_mgmt.beacons_rx;
	stats->sta_mgmt.beacons_miss = dev_data->stats.sta_mgmt.beacons_miss;

	return 0;
}
#endif

#define CYW43_WIFI_THREAD_STACK_SIZE 2048
#define CYW43_WIFI_THREAD_PRIO 7
K_THREAD_STACK_DEFINE(cyw43_wifi_thread_stack, CYW43_WIFI_THREAD_STACK_SIZE);
struct k_thread cyw43_wifi_thread;

/*
 * Bottom-half handler for CYW43439 GPIO-triggered interrupt.
 */
static void cyw43_wifi_thread_func(const struct device *dev)
{
	struct cyw43_wifi_dev_priv *dev_data = dev->data;

	while (true) {
		k_sem_take(&dev_data->ctx->bh_sem, K_FOREVER);
		if (!cyw43_poll) {
			LOG_DBG("cyw43_poll not initialized: skip IRQ");
			continue;
		}
		k_mutex_lock(&dev_data->ctx->mutex, K_FOREVER);
		cyw43_poll();
		k_mutex_unlock(&dev_data->ctx->mutex);
	}
}

static void cyw43_wifi_evt_handler(const struct device *port,
				struct gpio_callback *cb,
				gpio_port_pins_t pins)
{
	cyw43_irq_enable(&cyw43_wifi_dev_cfg, false);
	k_sem_give(&cyw43_wifi_dev_data.ctx->bh_sem);
}

static int cyw43_wifi_init(const struct device *dev)
{
	struct gpio_dt_spec *host_wake_gpio = &cyw43_wifi_dev_cfg.host_wake_gpio;
	struct cyw43_wifi_dev_priv *dev_data = dev->data;
	const struct cyw43_wifi_dev_config *dev_config = dev->config;
	const struct k_work_queue_config wq_cfg = {.name = "cyw43_wq"};
	int ret = 0;

	dev_data->ctx = &async_ctx;
	k_mutex_init(&dev_data->ctx->mutex);
	k_sem_init(&dev_data->ctx->bh_sem, 0, MAX_PENDING_EVENTS);

	k_work_queue_init(&dev_data->wq);
	k_work_queue_start(&dev_data->wq, cyw43_wq_stack,
			K_KERNEL_STACK_SIZEOF(cyw43_wq_stack),
			CONFIG_SYSTEM_WORKQUEUE_PRIORITY - 1, &wq_cfg);
	k_work_init(&dev_data->req_work, cyw43_wifi_request_work);

	cyw43_init(&cyw43_state);

	gpio_init_callback(&cyw43_wifi_gpio_cb, cyw43_wifi_evt_handler,
		BIT(host_wake_gpio->pin));
	ret = gpio_add_callback_dt(host_wake_gpio, &cyw43_wifi_gpio_cb);
	if (ret) {
		LOG_ERR("Error in gpio_add_callback(): %d", ret);
		return ret;
	}
	ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);

	k_tid_t thread_id = k_thread_create(&cyw43_wifi_thread,
			cyw43_wifi_thread_stack,
			CYW43_WIFI_THREAD_STACK_SIZE,
			(k_thread_entry_t)cyw43_wifi_thread_func,
			(void *)dev, NULL, NULL,
			K_PRIO_COOP(CYW43_WIFI_THREAD_PRIO), 0, K_NO_WAIT);
	k_thread_name_set(thread_id, "cyw43_wifi_thread");
	cyw43_irq_enable(dev_config, true);

	/*
	 * Set up CYW43439 wifi in STA mode.
	 * This will also set the power mode to the "performance" setting.
	 */
	cyw43_wifi_set_up(&cyw43_state, CYW43_ITF_STA, true,
			CYW43_COUNTRY_WORLDWIDE);

	ret = cyw43_wifi_pm(&cyw43_state,
		cyw43_pm_value(CONFIG_WIFI_RPI_PICO_CYW43_PM_MODE,
			CONFIG_WIFI_RPI_PICO_CYW43_PM2_SLEEP_RET_MS,
			CONFIG_WIFI_RPI_PICO_CYW43_PM_LI_BEACON_PERIOD,
			CONFIG_WIFI_RPI_PICO_CYW43_PM_LI_DTIM_PERIOD,
			CONFIG_WIFI_RPI_PICO_CYW43_PM_LI_ASSOC));
	if (ret) {
		LOG_ERR("Failed to start CYW43");
		return -EIO;
	}

	return 0;
}

static const struct wifi_mgmt_ops cyw43_wifi_mgmt_api = {
	.scan = cyw43_wifi_mgmt_scan,
	.connect = cyw43_wifi_mgmt_connect,
	.disconnect = cyw43_wifi_mgmt_disconnect,
	.ap_enable = cyw43_wifi_mgmt_ap_enable,
	.ap_disable = cyw43_wifi_mgmt_ap_disable,
	.iface_status = cyw43_wifi_mgmt_iface_status,
	.set_power_save = cyw43_wifi_mgmt_set_pm,
	.get_power_save_config = cyw43_wifi_mgmt_get_pm,
#if defined(CONFIG_NET_STATISTICS_WIFI)
	.get_stats = cyw43_wifi_mgmt_stats,
#endif
};

static const struct net_wifi_mgmt_offload cyw43_wifi_api = {
	.wifi_iface.iface_api.init = cyw43_wifi_iface_init,
	.wifi_iface.send = cyw43_wifi_send,
	.wifi_mgmt_api = &cyw43_wifi_mgmt_api,
};

NET_DEVICE_DT_INST_DEFINE(0,
	cyw43_wifi_init, NULL,
	&cyw43_wifi_dev_data, &cyw43_wifi_dev_cfg, CONFIG_WIFI_INIT_PRIORITY,
	&cyw43_wifi_api, ETHERNET_L2,
	NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);
