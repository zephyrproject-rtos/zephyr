/*
 * Copyright (c) 2019 Tobias Svehagen
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp_at

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_esp_at, CONFIG_WIFI_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/conn_mgr/connectivity_wifi_mgmt.h>

#include "esp.h"

struct esp_config {
#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	const struct gpio_dt_spec power;
#endif
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	const struct gpio_dt_spec reset;
#endif
};

NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE,
		    0, NULL);

/* RX thread structures */
K_KERNEL_STACK_DEFINE(esp_rx_stack,
		      CONFIG_WIFI_ESP_AT_RX_STACK_SIZE);
struct k_thread esp_rx_thread;

/* RX thread work queue */
K_KERNEL_STACK_DEFINE(esp_workq_stack,
		      CONFIG_WIFI_ESP_AT_WORKQ_STACK_SIZE);

static const struct esp_config esp_driver_config = {
#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	.power = GPIO_DT_SPEC_INST_GET(0, power_gpios),
#endif
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	.reset = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
#endif
};
struct esp_data esp_driver_data;

static void esp_configure_hostname(struct esp_data *data)
{
#if defined(CONFIG_NET_HOSTNAME_ENABLE)
	char cmd[sizeof("AT+CWHOSTNAME=\"\"") + NET_HOSTNAME_MAX_LEN];

	snprintk(cmd, sizeof(cmd), "AT+CWHOSTNAME=\"%s\"", net_hostname_get());
	cmd[sizeof(cmd) - 1] = '\0';

	esp_cmd_send(data, NULL, 0, cmd, ESP_CMD_TIMEOUT);
#else
	ARG_UNUSED(data);
#endif
}

static inline uint8_t esp_mode_from_flags(struct esp_data *data)
{
	uint8_t flags = data->flags;
	uint8_t mode = 0;

	if (flags & (EDF_STA_CONNECTED | EDF_STA_LOCK)) {
		mode |= ESP_MODE_STA;
	}

	if (flags & EDF_AP_ENABLED) {
		mode |= ESP_MODE_AP;
	}

	/*
	 * ESP AT 1.7 does not allow to disable radio, so enter STA mode
	 * instead.
	 */
	if (IS_ENABLED(CONFIG_WIFI_ESP_AT_VERSION_1_7) &&
	    mode == ESP_MODE_NONE) {
		mode = ESP_MODE_STA;
	}

	return mode;
}

static int esp_mode_switch(struct esp_data *data, uint8_t mode)
{
	char cmd[] = "AT+"_CWMODE"=X";
	int err;

	cmd[sizeof(cmd) - 2] = ('0' + mode);
	LOG_DBG("Switch to mode %hhu", mode);

	err = esp_cmd_send(data, NULL, 0, cmd, ESP_CMD_TIMEOUT);
	if (err) {
		LOG_WRN("Failed to switch to mode %d: %d", (int) mode, err);
	}

	return err;
}

static int esp_mode_switch_if_needed(struct esp_data *data)
{
	uint8_t new_mode = esp_mode_from_flags(data);
	uint8_t old_mode = data->mode;
	int err;

	if (old_mode == new_mode) {
		return 0;
	}

	data->mode = new_mode;

	err = esp_mode_switch(data, new_mode);
	if (err) {
		return err;
	}

	if (!(old_mode & ESP_MODE_STA) && (new_mode & ESP_MODE_STA)) {
		/*
		 * Hostname change is applied only when STA is enabled.
		 */
		esp_configure_hostname(data);
	}

	return 0;
}

static void esp_mode_switch_submit_if_needed(struct esp_data *data)
{
	if (data->mode != esp_mode_from_flags(data)) {
		k_work_submit_to_queue(&data->workq, &data->mode_switch_work);
	}
}

static void esp_mode_switch_work(struct k_work *work)
{
	struct esp_data *data =
		CONTAINER_OF(work, struct esp_data, mode_switch_work);

	(void)esp_mode_switch_if_needed(data);
}

static inline int esp_mode_flags_set(struct esp_data *data, uint8_t flags)
{
	esp_flags_set(data, flags);

	return esp_mode_switch_if_needed(data);
}

static inline int esp_mode_flags_clear(struct esp_data *data, uint8_t flags)
{
	esp_flags_clear(data, flags);

	return esp_mode_switch_if_needed(data);
}

/*
 * Modem Response Command Handlers
 */

/* Handler: OK */
MODEM_CMD_DEFINE(on_cmd_ok)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	modem_cmd_handler_set_error(data, 0);
	k_sem_give(&dev->sem_response);

	return 0;
}

/* Handler: ERROR */
MODEM_CMD_DEFINE(on_cmd_error)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&dev->sem_response);

	return 0;
}

/* RX thread */
static void esp_rx(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct esp_data *data = p1;

	while (true) {
		/* wait for incoming data */
		modem_iface_uart_rx_wait(&data->mctx.iface, K_FOREVER);

		modem_cmd_handler_process(&data->mctx.cmd_handler, &data->mctx.iface);

		/* give up time if we have a solid stream of data */
		k_yield();
	}
}

static char *str_unquote(char *str)
{
	char *end;

	if (str[0] != '"') {
		return str;
	}

	str++;

	end = strrchr(str, '"');
	if (end != NULL) {
		*end = 0;
	}

	return str;
}

/* +CIPSTAMAC:"xx:xx:xx:xx:xx:xx" */
MODEM_CMD_DEFINE(on_cmd_cipstamac)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);
	char *mac;

	mac = str_unquote(argv[0]);
	net_bytes_from_str(dev->mac_addr, sizeof(dev->mac_addr), mac);

	return 0;
}

static int esp_pull_quoted(char **str, char *str_end, char **unquoted)
{
	if (**str != '"') {
		return -EBADMSG;
	}

	(*str)++;

	*unquoted = *str;

	while (*str < str_end) {
		if (**str == '"') {
			**str = '\0';
			(*str)++;

			if (**str == ',') {
				(*str)++;
			}

			return 0;
		}

		(*str)++;
	}

	return -EAGAIN;
}

static int esp_pull(char **str, char *str_end)
{
	while (*str < str_end) {
		if (**str == ',' || **str == '\r' || **str == '\n') {
			char last_c = **str;

			**str = '\0';

			if (last_c == ',') {
				(*str)++;
			}

			return 0;
		}

		(*str)++;
	}

	return -EAGAIN;
}

static int esp_pull_raw(char **str, char *str_end, char **raw)
{
	*raw = *str;

	return esp_pull(str, str_end);
}

/* +CWLAP:(sec,ssid,rssi,channel) */
/* with: CONFIG_WIFI_ESP_AT_SCAN_MAC_ADDRESS: +CWLAP:<ecn>,<ssid>,<rssi>,<mac>,<ch>*/
MODEM_CMD_DIRECT_DEFINE(on_cmd_cwlap)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);
	struct wifi_scan_result res = { 0 };
	char cwlap_buf[sizeof("\"0\",\"\",-100,\"xx:xx:xx:xx:xx:xx\",12") +
		       WIFI_SSID_MAX_LEN * 2 + 1];
	char *ecn;
	char *ssid;
	char *mac;
	char *channel;
	char *rssi;
	long ecn_id;
	int err;

	len = net_buf_linearize(cwlap_buf, sizeof(cwlap_buf) - 1,
				data->rx_buf, 0, sizeof(cwlap_buf) - 1);
	cwlap_buf[len] = '\0';

	char *str = &cwlap_buf[sizeof("+CWJAP:(") - 1];
	char *str_end = cwlap_buf + len;

	err = esp_pull_raw(&str, str_end, &ecn);
	if (err) {
		return err;
	}

	ecn_id = strtol(ecn, NULL, 10);
	if (ecn_id == 0) {
		res.security = WIFI_SECURITY_TYPE_NONE;
	} else {
		res.security = WIFI_SECURITY_TYPE_PSK;
	}

	err = esp_pull_quoted(&str, str_end, &ssid);
	if (err) {
		return err;
	}

	err = esp_pull_raw(&str, str_end, &rssi);
	if (err) {
		return err;
	}

	if (strlen(ssid) > WIFI_SSID_MAX_LEN) {
		return -EBADMSG;
	}

	strncpy(res.ssid, ssid, sizeof(res.ssid));
	res.ssid_length = MIN(sizeof(res.ssid), strlen(ssid));

	res.rssi = strtol(rssi, NULL, 10);

	if (IS_ENABLED(CONFIG_WIFI_ESP_AT_SCAN_MAC_ADDRESS)) {
		err = esp_pull_quoted(&str, str_end, &mac);
		if (err) {
			return err;
		}

		res.mac_length = WIFI_MAC_ADDR_LEN;
		if (net_bytes_from_str(res.mac, sizeof(res.mac), mac) < 0) {
			LOG_ERR("Invalid MAC address");
			res.mac_length = 0;
		}
	}

	err = esp_pull_raw(&str, str_end, &channel);
	if (err) {
		return err;
	}

	if (dev->scan_cb) {
		dev->scan_cb(dev->net_iface, 0, &res);
	}

	return str - cwlap_buf;
}

/* +CWJAP:(ssid,bssid,channel,rssi) */
MODEM_CMD_DIRECT_DEFINE(on_cmd_cwjap)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);
	struct wifi_iface_status *status = dev->wifi_status;
	char cwjap_buf[sizeof("\"\",\"xx:xx:xx:xx:xx:xx\",12,-100") +
		       WIFI_SSID_MAX_LEN * 2 + 1];
	uint8_t flags = dev->flags;
	char *ssid;
	char *bssid;
	char *channel;
	char *rssi;
	int err;

	len = net_buf_linearize(cwjap_buf, sizeof(cwjap_buf) - 1,
				data->rx_buf, 0, sizeof(cwjap_buf) - 1);
	cwjap_buf[len] = '\0';

	char *str = &cwjap_buf[sizeof("+CWJAP:") - 1];
	char *str_end = cwjap_buf + len;

	status->band = WIFI_FREQ_BAND_2_4_GHZ;
	status->iface_mode = WIFI_MODE_INFRA;

	if (flags & EDF_STA_CONNECTED) {
		status->state = WIFI_STATE_COMPLETED;
	} else if (flags & EDF_STA_CONNECTING) {
		status->state = WIFI_STATE_SCANNING;
	} else {
		status->state = WIFI_STATE_DISCONNECTED;
	}

	err = esp_pull_quoted(&str, str_end, &ssid);
	if (err) {
		return err;
	}

	err = esp_pull_quoted(&str, str_end, &bssid);
	if (err) {
		return err;
	}

	err = esp_pull_raw(&str, str_end, &channel);
	if (err) {
		return err;
	}

	err = esp_pull_raw(&str, str_end, &rssi);
	if (err) {
		return err;
	}

	strncpy(status->ssid, ssid, sizeof(status->ssid));
	status->ssid_len = strnlen(status->ssid, sizeof(status->ssid));

	err = net_bytes_from_str(status->bssid, sizeof(status->bssid), bssid);
	if (err) {
		LOG_WRN("Invalid MAC address");
		memset(status->bssid, 0x0, sizeof(status->bssid));
	}

	status->channel = strtol(channel, NULL, 10);
	status->rssi = strtol(rssi, NULL, 10);

	return str - cwjap_buf;
}

static void esp_dns_work(struct k_work *work)
{
#if defined(ESP_MAX_DNS)
	struct esp_data *data = CONTAINER_OF(work, struct esp_data, dns_work);
	struct dns_resolve_context *dnsctx;
	struct sockaddr_in *addrs = data->dns_addresses;
	const struct sockaddr *dns_servers[ESP_MAX_DNS + 1] = {};
	size_t i;
	int err;

	for (i = 0; i < ESP_MAX_DNS; i++) {
		if (!addrs[i].sin_addr.s_addr) {
			break;
		}
		dns_servers[i] = (struct sockaddr *) &addrs[i];
	}

	dnsctx = dns_resolve_get_default();
	err = dns_resolve_reconfigure(dnsctx, NULL, dns_servers);
	if (err) {
		LOG_ERR("Could not set DNS servers: %d", err);
	}

	LOG_DBG("DNS resolver reconfigured");
#endif
}

/* +CIPDNS:enable[,"DNS IP1"[,"DNS IP2"[,"DNS IP3"]]] */
MODEM_CMD_DEFINE(on_cmd_cipdns)
{
#if defined(ESP_MAX_DNS)
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);
	struct sockaddr_in *addrs = dev->dns_addresses;
	char **servers = (char **)argv + 1;
	size_t num_servers = argc - 1;
	size_t valid_servers = 0;
	size_t i;
	int err;

	for (i = 0; i < ESP_MAX_DNS; i++) {
		if (i >= num_servers) {
			addrs[i].sin_addr.s_addr = 0;
			break;
		}

		servers[i] = str_unquote(servers[i]);
		LOG_DBG("DNS[%zu]: %s", i, servers[i]);

		err = net_addr_pton(AF_INET, servers[i], &addrs[i].sin_addr);
		if (err) {
			LOG_ERR("Invalid DNS address: %s",
				servers[i]);
			addrs[i].sin_addr.s_addr = 0;
			break;
		}

		addrs[i].sin_family = AF_INET;
		addrs[i].sin_port = htons(53);

		valid_servers++;
	}

	if (valid_servers) {
		k_work_submit(&dev->dns_work);
	}
#endif

	return 0;
}

static const struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", on_cmd_ok, 0U, ""), /* 3GPP */
	MODEM_CMD("ERROR", on_cmd_error, 0U, ""), /* 3GPP */
};

MODEM_CMD_DEFINE(on_cmd_wifi_connected)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	if (esp_flags_are_set(dev, EDF_STA_CONNECTED)) {
		return 0;
	}

	esp_flags_set(dev, EDF_STA_CONNECTED);
	wifi_mgmt_raise_connect_result_event(dev->net_iface, 0);
	net_if_dormant_off(dev->net_iface);

	return 0;
}

static void esp_mgmt_disconnect_work(struct k_work *work)
{
	struct esp_socket *sock;
	struct esp_data *dev;

	dev = CONTAINER_OF(work, struct esp_data, disconnect_work);

	/* Cleanup any sockets that weren't closed */
	for (int i = 0; i < ARRAY_SIZE(dev->sockets); i++) {
		sock = &dev->sockets[i];
		if (esp_socket_connected(sock)) {
			LOG_WRN("Socket %d left open, manually closing", i);
			esp_socket_close(sock);
		}
	}

	esp_flags_clear(dev, EDF_STA_CONNECTED);
	esp_mode_switch_submit_if_needed(dev);

#if defined(CONFIG_NET_NATIVE_IPV4)
	net_if_ipv4_addr_rm(dev->net_iface, &dev->ip);
#endif
	if (!esp_flags_are_set(dev, EDF_AP_ENABLED)) {
		net_if_dormant_on(dev->net_iface);
	}
	wifi_mgmt_raise_disconnect_result_event(dev->net_iface, 0);
}

MODEM_CMD_DEFINE(on_cmd_wifi_disconnected)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	if (esp_flags_are_set(dev, EDF_STA_CONNECTED)) {
		k_work_submit_to_queue(&dev->workq, &dev->disconnect_work);
	}

	return 0;
}

/*
 * +CIPSTA:ip:"<ip>"
 * +CIPSTA:gateway:"<ip>"
 * +CIPSTA:netmask:"<ip>"
 */
MODEM_CMD_DEFINE(on_cmd_cipsta)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);
	char *ip;

	ip = str_unquote(argv[1]);

	if (!strcmp(argv[0], "ip")) {
		net_addr_pton(AF_INET, ip, &dev->ip);
	} else if (!strcmp(argv[0], "gateway")) {
		net_addr_pton(AF_INET, ip, &dev->gw);
	} else if (!strcmp(argv[0], "netmask")) {
		net_addr_pton(AF_INET, ip, &dev->nm);
	} else {
		LOG_WRN("Unknown IP type %s", argv[0]);
	}

	return 0;
}

static void esp_ip_addr_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct esp_data *dev = CONTAINER_OF(dwork, struct esp_data,
					    ip_addr_work);
	int ret;

	static const struct modem_cmd cmds[] = {
		MODEM_CMD("+"_CIPSTA":", on_cmd_cipsta, 2U, ":"),
	};
	static const struct modem_cmd dns_cmds[] = {
		MODEM_CMD_ARGS_MAX("+CIPDNS:", on_cmd_cipdns, 1U, 3U, ","),
	};

	ret = esp_cmd_send(dev, cmds, ARRAY_SIZE(cmds), "AT+"_CIPSTA"?",
			   ESP_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to query IP settings: ret %d", ret);
		k_work_reschedule_for_queue(&dev->workq, &dev->ip_addr_work,
					    K_SECONDS(5));
		return;
	}

#if defined(CONFIG_NET_NATIVE_IPV4)
	/* update interface addresses */
#if defined(CONFIG_WIFI_ESP_AT_IP_STATIC)
	net_if_ipv4_addr_add(dev->net_iface, &dev->ip, NET_ADDR_MANUAL, 0);
#else
	net_if_ipv4_addr_add(dev->net_iface, &dev->ip, NET_ADDR_DHCP, 0);
#endif
	net_if_ipv4_set_gw(dev->net_iface, &dev->gw);
	net_if_ipv4_set_netmask_by_addr(dev->net_iface, &dev->ip, &dev->nm);
#endif

	if (IS_ENABLED(CONFIG_WIFI_ESP_AT_DNS_USE)) {
		ret = esp_cmd_send(dev, dns_cmds, ARRAY_SIZE(dns_cmds),
				   "AT+CIPDNS?", ESP_CMD_TIMEOUT);
		if (ret) {
			LOG_WRN("DNS fetch failed: %d", ret);
		}
	}
}

MODEM_CMD_DEFINE(on_cmd_got_ip)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	k_work_reschedule_for_queue(&dev->workq, &dev->ip_addr_work,
				    K_SECONDS(1));

	return 0;
}

MODEM_CMD_DEFINE(on_cmd_connect)
{
	struct esp_socket *sock;
	struct esp_data *dev;
	uint8_t link_id;

	link_id = data->match_buf[0] - '0';

	dev = CONTAINER_OF(data, struct esp_data, cmd_handler_data);
	sock = esp_socket_ref_from_link_id(dev, link_id);
	if (!sock) {
		LOG_ERR("No socket for link %d", link_id);
		return 0;
	}

	esp_socket_unref(sock);

	return 0;
}

MODEM_CMD_DEFINE(on_cmd_closed)
{
	struct esp_socket *sock;
	struct esp_data *dev;
	uint8_t link_id;
	atomic_val_t old_flags;

	link_id = data->match_buf[0] - '0';

	LOG_DBG("Link %d closed", link_id);

	dev = CONTAINER_OF(data, struct esp_data, cmd_handler_data);
	sock = esp_socket_ref_from_link_id(dev, link_id);
	if (!sock) {
		LOG_ERR("No socket for link %d", link_id);
		return 0;
	}

	old_flags = esp_socket_flags_clear_and_set(sock,
				ESP_SOCK_CONNECTED, ESP_SOCK_CLOSE_PENDING);

	if (!(old_flags & ESP_SOCK_CONNECTED)) {
		LOG_DBG("Link %d already closed", link_id);
		goto socket_unref;
	}

	if (!(old_flags & ESP_SOCK_CLOSE_PENDING)) {
		esp_socket_work_submit(sock, &sock->close_work);
	}

socket_unref:
	esp_socket_unref(sock);

	return 0;
}

/*
 * Passive mode: "+IPD,<id>,<len>\r\n"
 * Other:        "+IPD,<id>,<len>:<data>"
 */
#define MIN_IPD_LEN (sizeof("+IPD,I,0E") - 1)
#define MAX_IPD_LEN (sizeof("+IPD,I,4294967295E") - 1)

static int cmd_ipd_parse_hdr(struct net_buf *buf, uint16_t len,
			     uint8_t *link_id,
			     int *data_offset, int *data_len,
			     char *end)
{
	char *endptr, ipd_buf[MAX_IPD_LEN + 1];
	size_t frags_len;
	size_t match_len;

	frags_len = net_buf_frags_len(buf);

	/* Wait until minimum cmd length is available */
	if (frags_len < MIN_IPD_LEN) {
		return -EAGAIN;
	}

	match_len = net_buf_linearize(ipd_buf, MAX_IPD_LEN,
				      buf, 0, MAX_IPD_LEN);

	ipd_buf[match_len] = 0;
	if (ipd_buf[len] != ',' || ipd_buf[len + 2] != ',') {
		LOG_ERR("Invalid IPD: %s", ipd_buf);
		return -EBADMSG;
	}

	*link_id = ipd_buf[len + 1] - '0';

	*data_len = strtol(&ipd_buf[len + 3], &endptr, 10);

	if (endptr == &ipd_buf[len + 3] ||
	    (*endptr == 0 && match_len >= MAX_IPD_LEN)) {
		LOG_ERR("Invalid IPD len: %s", ipd_buf);
		return -EBADMSG;
	} else if (*endptr == 0) {
		return -EAGAIN;
	}

	*end = *endptr;
	*data_offset = (endptr - ipd_buf) + 1;

	return 0;
}

static int cmd_ipd_check_hdr_end(struct esp_socket *sock, char actual)
{
	char expected;

	/* When using passive mode, the +IPD command ends with \r\n */
	if (ESP_PROTO_PASSIVE(esp_socket_ip_proto(sock))) {
		expected = '\r';
	} else {
		expected = ':';
	}

	if (expected != actual) {
		LOG_ERR("Invalid cmd end 0x%02x, expected 0x%02x", actual,
			expected);
		return -EBADMSG;
	}

	return 0;
}

MODEM_CMD_DIRECT_DEFINE(on_cmd_ipd)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);
	struct esp_socket *sock;
	int data_offset, data_len;
	uint8_t link_id;
	char cmd_end;
	int err;
	int ret;

	err = cmd_ipd_parse_hdr(data->rx_buf, len, &link_id, &data_offset,
				&data_len, &cmd_end);
	if (err) {
		if (err == -EAGAIN) {
			return -EAGAIN;
		}

		return len;
	}

	sock = esp_socket_ref_from_link_id(dev, link_id);
	if (!sock) {
		LOG_ERR("No socket for link %d", link_id);
		return len;
	}

	err = cmd_ipd_check_hdr_end(sock, cmd_end);
	if (err) {
		ret = len;
		goto socket_unref;
	}

	/*
	 * When using passive TCP, the data itself is not included in the +IPD
	 * command but must be polled with AT+CIPRECVDATA.
	 */
	if (ESP_PROTO_PASSIVE(esp_socket_ip_proto(sock))) {
		esp_socket_work_submit(sock, &sock->recvdata_work);
		ret = data_offset;
		goto socket_unref;
	}

	/* Do we have the whole message? */
	if (data_offset + data_len > net_buf_frags_len(data->rx_buf)) {
		ret = -EAGAIN;
		goto socket_unref;
	}

	esp_socket_rx(sock, data->rx_buf, data_offset, data_len);

	ret = data_offset + data_len;

socket_unref:
	esp_socket_unref(sock);

	return ret;
}

MODEM_CMD_DEFINE(on_cmd_busy_sending)
{
	LOG_WRN("Busy sending");
	return 0;
}

MODEM_CMD_DEFINE(on_cmd_busy_processing)
{
	LOG_WRN("Busy processing");
	return 0;
}

/*
 * The 'ready' command is sent when device has booted and is ready to receive
 * commands. It is only expected after a reset of the device.
 */
MODEM_CMD_DEFINE(on_cmd_ready)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);
	k_sem_give(&dev->sem_if_ready);


	if (net_if_is_carrier_ok(dev->net_iface)) {
		net_if_dormant_on(dev->net_iface);
		net_if_carrier_off(dev->net_iface);
		LOG_ERR("Unexpected reset");
	}

	if (esp_flags_are_set(dev, EDF_STA_CONNECTING)) {
		wifi_mgmt_raise_connect_result_event(dev->net_iface, -1);
	} else if (esp_flags_are_set(dev, EDF_STA_CONNECTED)) {
		wifi_mgmt_raise_disconnect_result_event(dev->net_iface, 0);
	}

	dev->flags = 0;
	dev->mode = 0;

#if defined(CONFIG_NET_NATIVE_IPV4)
	net_if_ipv4_addr_rm(dev->net_iface, &dev->ip);
#endif
	k_work_submit_to_queue(&dev->workq, &dev->init_work);

	return 0;
}

#if defined(CONFIG_WIFI_ESP_AT_FETCH_VERSION)

static int cmd_version_log(struct modem_cmd_handler_data *data,
			   const char *type, const char *version)
{
	LOG_INF("%s: %s", type, version);

	return 0;
}

MODEM_CMD_DEFINE(on_cmd_at_version)
{
	return cmd_version_log(data, "AT version", argv[0]);
}

MODEM_CMD_DEFINE(on_cmd_sdk_version)
{
	return cmd_version_log(data, "SDK version", argv[0]);
}

MODEM_CMD_DEFINE(on_cmd_compile_time)
{
	return cmd_version_log(data, "compile time", argv[0]);
}

MODEM_CMD_DEFINE(on_cmd_bin_version)
{
	return cmd_version_log(data, "Bin version", argv[0]);
}

#endif /* CONFIG_WIFI_ESP_AT_FETCH_VERSION */

static const struct modem_cmd unsol_cmds[] = {
	MODEM_CMD("WIFI CONNECTED", on_cmd_wifi_connected, 0U, ""),
	MODEM_CMD("WIFI DISCONNECT", on_cmd_wifi_disconnected, 0U, ""),
	MODEM_CMD("WIFI GOT IP", on_cmd_got_ip, 0U, ""),
	MODEM_CMD("0,CONNECT", on_cmd_connect, 0U, ""),
	MODEM_CMD("1,CONNECT", on_cmd_connect, 0U, ""),
	MODEM_CMD("2,CONNECT", on_cmd_connect, 0U, ""),
	MODEM_CMD("3,CONNECT", on_cmd_connect, 0U, ""),
	MODEM_CMD("4,CONNECT", on_cmd_connect, 0U, ""),
	MODEM_CMD("0,CLOSED", on_cmd_closed, 0U, ""),
	MODEM_CMD("1,CLOSED", on_cmd_closed, 0U, ""),
	MODEM_CMD("2,CLOSED", on_cmd_closed, 0U, ""),
	MODEM_CMD("3,CLOSED", on_cmd_closed, 0U, ""),
	MODEM_CMD("4,CLOSED", on_cmd_closed, 0U, ""),
	MODEM_CMD("busy s...", on_cmd_busy_sending, 0U, ""),
	MODEM_CMD("busy p...", on_cmd_busy_processing, 0U, ""),
	MODEM_CMD("ready", on_cmd_ready, 0U, ""),
#if defined(CONFIG_WIFI_ESP_AT_FETCH_VERSION)
	MODEM_CMD("AT version:", on_cmd_at_version, 1U, ""),
	MODEM_CMD("SDK version:", on_cmd_sdk_version, 1U, ""),
	MODEM_CMD("Compile time", on_cmd_compile_time, 1U, ""),
	MODEM_CMD("Bin version:", on_cmd_bin_version, 1U, ""),
#endif
	MODEM_CMD_DIRECT("+IPD", on_cmd_ipd),
};

static void esp_mgmt_iface_status_work(struct k_work *work)
{
	struct esp_data *data = CONTAINER_OF(work, struct esp_data, iface_status_work);
	struct wifi_iface_status *status = data->wifi_status;
	int ret;
	static const struct modem_cmd cmds[] = {
		MODEM_CMD_DIRECT("+CWJAP:", on_cmd_cwjap),
	};

	ret = esp_cmd_send(data, cmds, ARRAY_SIZE(cmds), "AT+CWJAP?",
			   ESP_IFACE_STATUS_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to request STA status: ret %d", ret);
		status->state = WIFI_STATE_UNKNOWN;
	}

	k_sem_give(&data->wifi_status_sem);
}

static int esp_mgmt_iface_status(const struct device *dev,
				 struct wifi_iface_status *status)
{
	struct esp_data *data = dev->data;

	memset(status, 0x0, sizeof(*status));

	status->state = WIFI_STATE_UNKNOWN;
	status->band = WIFI_FREQ_BAND_UNKNOWN;
	status->iface_mode = WIFI_MODE_UNKNOWN;
	status->link_mode = WIFI_LINK_MODE_UNKNOWN;
	status->security = WIFI_SECURITY_TYPE_UNKNOWN;
	status->mfp = WIFI_MFP_UNKNOWN;

	if (!net_if_is_carrier_ok(data->net_iface)) {
		status->state = WIFI_STATE_INTERFACE_DISABLED;
		return 0;
	}

	data->wifi_status = status;
	k_sem_init(&data->wifi_status_sem, 0, 1);

	k_work_submit_to_queue(&data->workq, &data->iface_status_work);

	k_sem_take(&data->wifi_status_sem, K_FOREVER);

	return 0;
}

static void esp_mgmt_scan_work(struct k_work *work)
{
	struct esp_data *dev;
	int ret;
	static const struct modem_cmd cmds[] = {
		MODEM_CMD_DIRECT("+CWLAP:", on_cmd_cwlap),
	};

	dev = CONTAINER_OF(work, struct esp_data, scan_work);

	ret = esp_mode_flags_set(dev, EDF_STA_LOCK);
	if (ret < 0) {
		goto out;
	}
	ret = esp_cmd_send(dev,
			   cmds, ARRAY_SIZE(cmds),
			   ESP_CMD_CWLAP,
			   ESP_SCAN_TIMEOUT);
	esp_mode_flags_clear(dev, EDF_STA_LOCK);
	LOG_DBG("ESP Wi-Fi scan: cmd = %s", ESP_CMD_CWLAP);

	if (ret < 0) {
		LOG_ERR("Failed to scan: ret %d", ret);
	}

out:
	dev->scan_cb(dev->net_iface, 0, NULL);
	dev->scan_cb = NULL;
}

static int esp_mgmt_scan(const struct device *dev,
			 struct wifi_scan_params *params,
			 scan_result_cb_t cb)
{
	struct esp_data *data = dev->data;

	ARG_UNUSED(params);

	if (data->scan_cb != NULL) {
		return -EINPROGRESS;
	}

	if (!net_if_is_carrier_ok(data->net_iface)) {
		return -EIO;
	}

	data->scan_cb = cb;

	k_work_submit_to_queue(&data->workq, &data->scan_work);

	return 0;
};

MODEM_CMD_DEFINE(on_cmd_fail)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	modem_cmd_handler_set_error(data, -EIO);
	k_sem_give(&dev->sem_response);

	return 0;
}

static void esp_mgmt_connect_work(struct k_work *work)
{
	struct esp_data *dev;
	int ret;
	static const struct modem_cmd cmds[] = {
		MODEM_CMD("FAIL", on_cmd_fail, 0U, ""),
	};

	dev = CONTAINER_OF(work, struct esp_data, connect_work);

	ret = esp_mode_flags_set(dev, EDF_STA_LOCK);
	if (ret < 0) {
		goto out;
	}

	ret = esp_cmd_send(dev, cmds, ARRAY_SIZE(cmds), dev->conn_cmd,
			   ESP_CONNECT_TIMEOUT);

	memset(dev->conn_cmd, 0, sizeof(dev->conn_cmd));

	if (ret < 0) {
		net_if_dormant_on(dev->net_iface);
		if (esp_flags_are_set(dev, EDF_STA_CONNECTED)) {
			esp_flags_clear(dev, EDF_STA_CONNECTED);
			wifi_mgmt_raise_disconnect_result_event(dev->net_iface,
								0);
		} else {
			wifi_mgmt_raise_connect_result_event(dev->net_iface,
							     ret);
		}
	} else if (!esp_flags_are_set(dev, EDF_STA_CONNECTED)) {
		esp_flags_set(dev, EDF_STA_CONNECTED);
		wifi_mgmt_raise_connect_result_event(dev->net_iface, 0);
		net_if_dormant_off(dev->net_iface);
	}

	esp_mode_flags_clear(dev, EDF_STA_LOCK);

out:
	esp_flags_clear(dev, EDF_STA_CONNECTING);
}

static int esp_conn_cmd_append(struct esp_data *data, size_t *off,
			       const char *chunk, size_t chunk_len)
{
	char *str_end = &data->conn_cmd[sizeof(data->conn_cmd)];
	char *str = &data->conn_cmd[*off];
	const char *chunk_end = chunk + chunk_len;

	for (; chunk < chunk_end; chunk++) {
		if (str_end - str < 1) {
			return -ENOSPC;
		}

		*str = *chunk;
		str++;
	}

	*off = str - data->conn_cmd;

	return 0;
}

#define esp_conn_cmd_append_literal(data, off, chunk)			\
	esp_conn_cmd_append(data, off, chunk, sizeof(chunk) - 1)

static int esp_conn_cmd_escape_and_append(struct esp_data *data, size_t *off,
					  const char *chunk, size_t chunk_len)
{
	char *str_end = &data->conn_cmd[sizeof(data->conn_cmd)];
	char *str = &data->conn_cmd[*off];
	const char *chunk_end = chunk + chunk_len;

	for (; chunk < chunk_end; chunk++) {
		switch (*chunk) {
		case ',':
		case '\\':
		case '"':
			if (str_end - str < 2) {
				return -ENOSPC;
			}

			*str = '\\';
			str++;

			break;
		}

		if (str_end - str < 1) {
			return -ENOSPC;
		}

		*str = *chunk;
		str++;
	}

	*off = str - data->conn_cmd;

	return 0;
}

static int esp_mgmt_connect(const struct device *dev,
			    struct wifi_connect_req_params *params)
{
	struct esp_data *data = dev->data;
	size_t off = 0;
	int err;

	if (!net_if_is_carrier_ok(data->net_iface) ||
	    !net_if_is_admin_up(data->net_iface)) {
		return -EIO;
	}

	if (esp_flags_are_set(data, EDF_STA_CONNECTED | EDF_STA_CONNECTING)) {
		return -EALREADY;
	}

	esp_flags_set(data, EDF_STA_CONNECTING);

	err = esp_conn_cmd_append_literal(data, &off, "AT+"_CWJAP"=\"");
	if (err) {
		return err;
	}

	err = esp_conn_cmd_escape_and_append(data, &off,
					     params->ssid, params->ssid_length);
	if (err) {
		return err;
	}

	err = esp_conn_cmd_append_literal(data, &off, "\",\"");
	if (err) {
		return err;
	}

	if (params->security == WIFI_SECURITY_TYPE_PSK) {
		err = esp_conn_cmd_escape_and_append(data, &off,
						     params->psk, params->psk_length);
		if (err) {
			return err;
		}
	}

	err = esp_conn_cmd_append_literal(data, &off, "\"");
	if (err) {
		return err;
	}

	k_work_submit_to_queue(&data->workq, &data->connect_work);

	return 0;
}

static int esp_mgmt_disconnect(const struct device *dev)
{
	struct esp_data *data = dev->data;
	int ret;

	ret = esp_cmd_send(data, NULL, 0, "AT+CWQAP", ESP_CMD_TIMEOUT);

	return ret;
}

static int esp_mgmt_ap_enable(const struct device *dev,
			      struct wifi_connect_req_params *params)
{
	char cmd[sizeof("AT+"_CWSAP"=\"\",\"\",xx,x") + WIFI_SSID_MAX_LEN +
		 WIFI_PSK_MAX_LEN];
	struct esp_data *data = dev->data;
	int ecn = 0, len, ret;

	ret = esp_mode_flags_set(data, EDF_AP_ENABLED);
	if (ret < 0) {
		LOG_ERR("Failed to enable AP mode, ret %d", ret);
		return ret;
	}

	len = snprintk(cmd, sizeof(cmd), "AT+"_CWSAP"=\"");
	memcpy(&cmd[len], params->ssid, params->ssid_length);
	len += params->ssid_length;

	if (params->security == WIFI_SECURITY_TYPE_PSK) {
		len += snprintk(&cmd[len], sizeof(cmd) - len, "\",\"");
		memcpy(&cmd[len], params->psk, params->psk_length);
		len += params->psk_length;
		ecn = 3;
	} else {
		len += snprintk(&cmd[len], sizeof(cmd) - len, "\",\"");
	}

	snprintk(&cmd[len], sizeof(cmd) - len, "\",%d,%d", params->channel,
		 ecn);

	ret = esp_cmd_send(data, NULL, 0, cmd, ESP_CMD_TIMEOUT);

	net_if_dormant_off(data->net_iface);

	return ret;
}

static int esp_mgmt_ap_disable(const struct device *dev)
{
	struct esp_data *data = dev->data;

	if (!esp_flags_are_set(data, EDF_STA_CONNECTED)) {
		net_if_dormant_on(data->net_iface);
	}

	return esp_mode_flags_clear(data, EDF_AP_ENABLED);
}

static void esp_init_work(struct k_work *work)
{
	struct esp_data *dev;
	int ret;
	static const struct setup_cmd setup_cmds[] = {
		SETUP_CMD_NOHANDLE("AT"),
		/* turn off echo */
		SETUP_CMD_NOHANDLE("ATE0"),
		SETUP_CMD_NOHANDLE("AT+UART_CUR="_UART_CUR),
#if DT_INST_NODE_HAS_PROP(0, target_speed)
	};
	static const struct setup_cmd setup_cmds_target_baudrate[] = {
		SETUP_CMD_NOHANDLE("AT"),
#endif
#if defined(CONFIG_WIFI_ESP_AT_FETCH_VERSION)
		SETUP_CMD_NOHANDLE("AT+GMR"),
#endif
#if defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
		SETUP_CMD_NOHANDLE(ESP_CMD_CWMODE(STA)),
#endif
#if defined(CONFIG_WIFI_ESP_AT_IP_STATIC)
		/* enable Static IP Config */
		SETUP_CMD_NOHANDLE(ESP_CMD_DHCP_ENABLE(STATION, 0)),
		SETUP_CMD_NOHANDLE(ESP_CMD_SET_IP(CONFIG_WIFI_ESP_AT_IP_ADDRESS,
						  CONFIG_WIFI_ESP_AT_IP_GATEWAY,
						  CONFIG_WIFI_ESP_AT_IP_MASK)),
#else
		/* enable DHCP */
		SETUP_CMD_NOHANDLE(ESP_CMD_DHCP_ENABLE(STATION, 1)),
#endif
		/* enable multiple socket support */
		SETUP_CMD_NOHANDLE("AT+CIPMUX=1"),

		SETUP_CMD_NOHANDLE(
			ESP_CMD_CWLAPOPT(ESP_CMD_CWLAPOPT_ORDERED, ESP_CMD_CWLAPOPT_MASK)),

#if !defined(CONFIG_WIFI_ESP_AT_VERSION_1_7)
		SETUP_CMD_NOHANDLE(ESP_CMD_CWMODE(STA)),
		SETUP_CMD_NOHANDLE("AT+CWAUTOCONN=0"),
		SETUP_CMD_NOHANDLE(ESP_CMD_CWMODE(NONE)),
#endif
#if defined(CONFIG_WIFI_ESP_AT_PASSIVE_MODE)
		SETUP_CMD_NOHANDLE("AT+CIPRECVMODE=1"),
#endif
#if defined(CONFIG_WIFI_ESP_AT_CIPDINFO_USE)
		SETUP_CMD_NOHANDLE("AT+CIPDINFO=1"),
#endif
		SETUP_CMD("AT+"_CIPSTAMAC"?", "+"_CIPSTAMAC":",
			  on_cmd_cipstamac, 1U, ""),
	};

	dev = CONTAINER_OF(work, struct esp_data, init_work);

	ret = modem_cmd_handler_setup_cmds(&dev->mctx.iface,
					   &dev->mctx.cmd_handler, setup_cmds,
					   ARRAY_SIZE(setup_cmds),
					   &dev->sem_response,
					   ESP_INIT_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Init failed %d", ret);
		return;
	}

#if DT_INST_NODE_HAS_PROP(0, target_speed)
	static const struct uart_config uart_config = {
		.baudrate = DT_INST_PROP(0, target_speed),
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = DT_PROP(ESP_BUS, hw_flow_control) ?
			UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE,
	};

	ret = uart_configure(DEVICE_DT_GET(DT_INST_BUS(0)), &uart_config);
	if (ret < 0) {
		LOG_ERR("Baudrate change failed %d", ret);
		return;
	}

	/* arbitrary sleep period to give ESP enough time to reconfigure */
	k_sleep(K_MSEC(100));

	ret = modem_cmd_handler_setup_cmds(&dev->mctx.iface,
				&dev->mctx.cmd_handler,
				setup_cmds_target_baudrate,
				ARRAY_SIZE(setup_cmds_target_baudrate),
				&dev->sem_response,
				ESP_INIT_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Init failed %d", ret);
		return;
	}
#endif

	net_if_set_link_addr(dev->net_iface, dev->mac_addr,
			     sizeof(dev->mac_addr), NET_LINK_ETHERNET);

	if (IS_ENABLED(CONFIG_WIFI_ESP_AT_VERSION_1_7)) {
		/* This is the mode entered in above setup commands */
		dev->mode = ESP_MODE_STA;

		/*
		 * In case of ESP 1.7 this is the first time CWMODE is entered
		 * STA mode, so request hostname change now.
		 */
		esp_configure_hostname(dev);
	}

	LOG_INF("ESP Wi-Fi ready");

	/* L1 network layer (physical layer) is up */
	net_if_carrier_on(dev->net_iface);

	k_sem_give(&dev->sem_if_up);
}

static int esp_reset(const struct device *dev)
{
	struct esp_data *data = dev->data;
	int ret = -EAGAIN;

	if (net_if_is_carrier_ok(data->net_iface)) {
		net_if_carrier_off(data->net_iface);
	}

#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	const struct esp_config *config = dev->config;

	gpio_pin_set_dt(&config->power, 0);
	k_sleep(K_MSEC(100));
	gpio_pin_set_dt(&config->power, 1);
#elif DT_INST_NODE_HAS_PROP(0, reset_gpios)
	const struct esp_config *config = dev->config;

	gpio_pin_set_dt(&config->reset, 1);
	k_sleep(K_MSEC(100));
	gpio_pin_set_dt(&config->reset, 0);
#else
#if DT_INST_NODE_HAS_PROP(0, external_reset)
	/* Wait to see if the interface comes up by itself */
	ret = k_sem_take(&data->sem_if_ready, K_MSEC(CONFIG_WIFI_ESP_AT_RESET_TIMEOUT));
#endif
	int retries = 3;

	/* Don't need to run this if the interface came up by itself */
	while ((ret != 0) && retries--) {
		ret = modem_cmd_send(&data->mctx.iface, &data->mctx.cmd_handler,
				     NULL, 0, "AT+RST", &data->sem_if_ready,
				     K_MSEC(CONFIG_WIFI_ESP_AT_RESET_TIMEOUT));
		if (ret == 0 || ret != -ETIMEDOUT) {
			break;
		}
	}

	if (ret < 0) {
		LOG_ERR("Failed to reset device: %d", ret);
		return -EAGAIN;
	}
#endif
	LOG_INF("Waiting for interface to come up");

	ret = k_sem_take(&data->sem_if_up, ESP_INIT_TIMEOUT);
	if (ret == -EAGAIN) {
		LOG_ERR("Timeout waiting for interface");
	}

	return ret;
}

static void esp_iface_init(struct net_if *iface)
{
	esp_offload_init(iface);

	/* Not currently connected to a network */
	net_if_dormant_on(iface);
}

static enum offloaded_net_if_types esp_offload_get_type(void)
{
	return L2_OFFLOADED_NET_IF_TYPE_WIFI;
}

static const struct wifi_mgmt_ops esp_mgmt_ops = {
	.scan		   = esp_mgmt_scan,
	.connect	   = esp_mgmt_connect,
	.disconnect	   = esp_mgmt_disconnect,
	.ap_enable	   = esp_mgmt_ap_enable,
	.ap_disable	   = esp_mgmt_ap_disable,
	.iface_status	   = esp_mgmt_iface_status,
};

static const struct net_wifi_mgmt_offload esp_api = {
	.wifi_iface.iface_api.init = esp_iface_init,
	.wifi_iface.get_type = esp_offload_get_type,
	.wifi_mgmt_api = &esp_mgmt_ops,
};

static int esp_init(const struct device *dev);

/* The network device must be instantiated above the init function in order
 * for the struct net_if that the macro declares to be visible inside the
 * function. An `extern` declaration does not work as the struct is static.
 */
NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, esp_init, NULL,
				  &esp_driver_data, &esp_driver_config,
				  CONFIG_WIFI_INIT_PRIORITY, &esp_api,
				  ESP_MTU);

CONNECTIVITY_WIFI_MGMT_BIND(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)));

static int esp_init(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, power_gpios) || DT_INST_NODE_HAS_PROP(0, reset_gpios)
	const struct esp_config *config = dev->config;
#endif
	struct esp_data *data = dev->data;
	int ret = 0;

	k_sem_init(&data->sem_tx_ready, 0, 1);
	k_sem_init(&data->sem_response, 0, 1);
	k_sem_init(&data->sem_if_ready, 0, 1);
	k_sem_init(&data->sem_if_up, 0, 1);

	k_work_init(&data->init_work, esp_init_work);
	k_work_init_delayable(&data->ip_addr_work, esp_ip_addr_work);
	k_work_init(&data->scan_work, esp_mgmt_scan_work);
	k_work_init(&data->connect_work, esp_mgmt_connect_work);
	k_work_init(&data->disconnect_work, esp_mgmt_disconnect_work);
	k_work_init(&data->iface_status_work, esp_mgmt_iface_status_work);
	k_work_init(&data->mode_switch_work, esp_mode_switch_work);
	if (IS_ENABLED(CONFIG_WIFI_ESP_AT_DNS_USE)) {
		k_work_init(&data->dns_work, esp_dns_work);
	}

	esp_socket_init(data);

	/* initialize the work queue */
	k_work_queue_start(&data->workq, esp_workq_stack,
			   K_KERNEL_STACK_SIZEOF(esp_workq_stack),
			   K_PRIO_COOP(CONFIG_WIFI_ESP_AT_WORKQ_THREAD_PRIORITY),
			   NULL);
	k_thread_name_set(&data->workq.thread, "esp_workq");

	/* cmd handler */
	const struct modem_cmd_handler_config cmd_handler_config = {
		.match_buf = &data->cmd_match_buf[0],
		.match_buf_len = sizeof(data->cmd_match_buf),
		.buf_pool = &mdm_recv_pool,
		.alloc_timeout = K_NO_WAIT,
		.eol = "\r\n",
		.user_data = NULL,
		.response_cmds = response_cmds,
		.response_cmds_len = ARRAY_SIZE(response_cmds),
		.unsol_cmds = unsol_cmds,
		.unsol_cmds_len = ARRAY_SIZE(unsol_cmds),
	};

	ret = modem_cmd_handler_init(&data->mctx.cmd_handler, &data->cmd_handler_data,
				     &cmd_handler_config);
	if (ret < 0) {
		goto error;
	}

	/* modem interface */
	const struct modem_iface_uart_config uart_config = {
		.rx_rb_buf = &data->iface_rb_buf[0],
		.rx_rb_buf_len = sizeof(data->iface_rb_buf),
		.dev = DEVICE_DT_GET(DT_INST_BUS(0)),
		.hw_flow_control = DT_PROP(ESP_BUS, hw_flow_control),
	};

	ret = modem_iface_uart_init(&data->mctx.iface, &data->iface_data, &uart_config);
	if (ret < 0) {
		goto error;
	}

	/* pin setup */
#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	ret = gpio_pin_configure_dt(&config->power, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure %s pin", "power");
		goto error;
	}
#endif
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure %s pin", "reset");
		goto error;
	}
#endif

	data->mctx.driver_data = data;

	ret = modem_context_register(&data->mctx);
	if (ret < 0) {
		LOG_ERR("Error registering modem context: %d", ret);
		goto error;
	}

	/* start RX thread */
	k_thread_create(&esp_rx_thread, esp_rx_stack,
			K_KERNEL_STACK_SIZEOF(esp_rx_stack),
			esp_rx,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_WIFI_ESP_AT_RX_THREAD_PRIORITY), 0,
			K_NO_WAIT);
	k_thread_name_set(&esp_rx_thread, "esp_rx");

	/* Retrieve associated network interface so asynchronous messages can be processed early */
	data->net_iface = NET_IF_GET(Z_DEVICE_DT_DEV_ID(DT_DRV_INST(0)), 0);

	/* Reset the modem */
	ret = esp_reset(dev);

error:
	return ret;
}
