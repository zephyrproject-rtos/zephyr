/*
 * Copyright (c) 2019 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp

#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wifi_esp);

#include <kernel.h>
#include <ctype.h>
#include <errno.h>
#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <stdlib.h>

#include <drivers/gpio.h>

#include <net/net_if.h>
#include <net/net_offload.h>
#include <net/wifi_mgmt.h>

#include "esp.h"

/* pin settings */
enum modem_control_pins {
#if DT_INST_NODE_HAS_PROP(0, wifi_reset_gpios)
	WIFI_RESET,
#endif
	NUM_PINS,
};

static struct modem_pin modem_pins[] = {
#if DT_INST_NODE_HAS_PROP(0, wifi_reset_gpios)
	MODEM_PIN(DT_INST_GPIO_LABEL(0, wifi_reset_gpios),
		  DT_INST_GPIO_PIN(0, wifi_reset_gpios),
		  DT_INST_GPIO_FLAGS(0, wifi_reset_gpios) | GPIO_OUTPUT),
#endif
};

NET_BUF_POOL_DEFINE(mdm_recv_pool, MDM_RECV_MAX_BUF, MDM_RECV_BUF_SIZE,
		    0, NULL);

/* RX thread structures */
K_KERNEL_STACK_DEFINE(esp_rx_stack,
		      CONFIG_WIFI_ESP_RX_STACK_SIZE);
struct k_thread esp_rx_thread;

/* RX thread work queue */
K_KERNEL_STACK_DEFINE(esp_workq_stack,
		      CONFIG_WIFI_ESP_WORKQ_STACK_SIZE);

struct esp_data esp_driver_data;

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
static void esp_rx(struct device *dev)
{
	struct esp_data *data = dev->data;

	while (true) {
		/* wait for incoming data */
		k_sem_take(&data->iface_data.rx_sem, K_FOREVER);

		data->mctx.cmd_handler.process(&data->mctx.cmd_handler,
					       &data->mctx.iface);

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

/* +CWLAP:(sec,ssid,rssi,channel) */
MODEM_CMD_DEFINE(on_cmd_cwlap)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);
	struct wifi_scan_result res = { 0 };
	int i;

	i = strtol(&argv[0][1], NULL, 10);
	if (i == 0) {
		res.security = WIFI_SECURITY_TYPE_NONE;
	} else {
		res.security = WIFI_SECURITY_TYPE_PSK;
	}

	argv[1] = str_unquote(argv[1]);
	i = strlen(argv[1]);
	if (i > sizeof(res.ssid)) {
		i = sizeof(res.ssid);
	}

	memcpy(res.ssid, argv[1], i);
	res.ssid_length = i;
	res.rssi = strtol(argv[2], NULL, 10);
	res.channel = strtol(argv[3], NULL, 10);

	if (dev->scan_cb) {
		dev->scan_cb(dev->net_iface, 0, &res);
	}

	return 0;
}

static struct modem_cmd response_cmds[] = {
	MODEM_CMD("OK", on_cmd_ok, 0U, ""), /* 3GPP */
	MODEM_CMD("ERROR", on_cmd_error, 0U, ""), /* 3GPP */
};

MODEM_CMD_DEFINE(on_cmd_wifi_connected)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	if (esp_flag_is_set(dev, EDF_STA_CONNECTED)) {
		return 0;
	}

	esp_flag_set(dev, EDF_STA_CONNECTED);
	wifi_mgmt_raise_connect_result_event(dev->net_iface, 0);

	return 0;
}

MODEM_CMD_DEFINE(on_cmd_wifi_disconnected)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	if (!esp_flag_is_set(dev, EDF_STA_CONNECTED)) {
		return 0;
	}

	esp_flag_clear(dev, EDF_STA_CONNECTED);
	net_if_ipv4_addr_rm(dev->net_iface, &dev->ip);
	wifi_mgmt_raise_disconnect_result_event(dev->net_iface, 0);

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
		LOG_WRN("Unknown IP type %s", log_strdup(argv[0]));
	}

	return 0;
}

static void esp_ip_addr_work(struct k_work *work)
{
	struct esp_data *dev = CONTAINER_OF(work, struct esp_data,
					    ip_addr_work);
	int ret;

	struct modem_cmd cmds[] = {
		MODEM_CMD("+"_CIPSTA":", on_cmd_cipsta, 2U, ":"),
	};

	ret = modem_cmd_send(&dev->mctx.iface, &dev->mctx.cmd_handler,
			     cmds, ARRAY_SIZE(cmds), "AT+"_CIPSTA"?",
			     &dev->sem_response, ESP_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_WRN("Failed to query IP settings: ret %d", ret);
		k_delayed_work_submit_to_queue(&dev->workq, &dev->ip_addr_work,
					       K_SECONDS(5));
		return;
	}

	/* update interface addresses */
	net_if_ipv4_set_gw(dev->net_iface, &dev->gw);
	net_if_ipv4_set_netmask(dev->net_iface, &dev->nm);
	net_if_ipv4_addr_add(dev->net_iface, &dev->ip, NET_ADDR_DHCP, 0);
}

MODEM_CMD_DEFINE(on_cmd_got_ip)
{
	struct esp_data *dev = CONTAINER_OF(data, struct esp_data,
					    cmd_handler_data);

	k_delayed_work_submit_to_queue(&dev->workq, &dev->ip_addr_work,
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
	sock = esp_socket_from_link_id(dev, link_id);
	if (sock == NULL) {
		LOG_ERR("No socket for link %d", link_id);
	}

	return 0;
}

MODEM_CMD_DEFINE(on_cmd_closed)
{
	struct esp_socket *sock;
	struct esp_data *dev;
	uint8_t link_id;

	link_id = data->match_buf[0] - '0';

	dev = CONTAINER_OF(data, struct esp_data, cmd_handler_data);
	sock = esp_socket_from_link_id(dev, link_id);
	if (sock == NULL) {
		LOG_ERR("No socket for link %d", link_id);
		return 0;
	}

	if (!esp_socket_connected(sock)) {
		LOG_WRN("Link %d already closed", link_id);
		return 0;
	}

	sock->flags &= ~(ESP_SOCK_CONNECTED);
	k_work_submit_to_queue(&dev->workq, &sock->recv_work);

	return 0;
}

struct net_pkt *esp_prepare_pkt(struct esp_data *dev, struct net_buf *src,
				size_t offset, size_t len)
{
	struct net_buf *frag;
	struct net_pkt *pkt;
	size_t to_copy;

	pkt = net_pkt_rx_alloc_with_buffer(dev->net_iface, len, AF_UNSPEC,
					   0, K_MSEC(100));
	if (!pkt) {
		return NULL;
	}

	frag = src;

	/* find the right fragment to start copying from */
	while (frag && offset >= frag->len) {
		offset -= frag->len;
		frag = frag->frags;
	}

	/* traverse the fragment chain until len bytes are copied */
	while (frag && len > 0) {
		to_copy = MIN(len, frag->len - offset);
		if (net_pkt_write(pkt, frag->data + offset, to_copy) != 0) {
			net_pkt_unref(pkt);
			return NULL;
		}

		/* to_copy is always <= len */
		len -= to_copy;
		frag = frag->frags;

		/* after the first iteration, this value will be 0 */
		offset = 0;
	}

	net_pkt_cursor_init(pkt);

	return pkt;
}

/*
 * Passive mode: "+IPD,<id>,<len>\r\n"
 * Other:        "+IPD,<id>,<len>:<data>"
 */
#define MIN_IPD_LEN (sizeof("+IPD,I,LE") - 1)
#define MAX_IPD_LEN (sizeof("+IPD,I,LLLLE") - 1)
MODEM_CMD_DIRECT_DEFINE(on_cmd_ipd)
{
	char *endptr, end, ipd_buf[MAX_IPD_LEN + 1];
	int data_offset, data_len, ret;
	size_t match_len, frags_len;
	struct esp_socket *sock;
	struct esp_data *dev;
	struct net_pkt *pkt;
	uint8_t link_id;

	dev = CONTAINER_OF(data, struct esp_data, cmd_handler_data);

	frags_len = net_buf_frags_len(data->rx_buf);

	/* Wait until minimum cmd length is available */
	if (frags_len < MIN_IPD_LEN) {
		ret = -EAGAIN;
		goto out;
	}

	match_len = net_buf_linearize(ipd_buf, MAX_IPD_LEN,
				      data->rx_buf, 0, MAX_IPD_LEN);

	ipd_buf[match_len] = 0;
	if (ipd_buf[len] != ',' || ipd_buf[len + 2] != ',') {
		LOG_ERR("Invalid IPD: %s", log_strdup(ipd_buf));
		ret = len;
		goto out;
	}

	link_id = ipd_buf[len + 1] - '0';
	sock = esp_socket_from_link_id(dev, link_id);
	if (sock == NULL) {
		LOG_ERR("No socket for link %d", link_id);
		ret = len;
		goto out;
	}

	/* When using passive mode, the +IPD command ends with \r\n */
	if (ESP_PROTO_PASSIVE(sock->ip_proto)) {
		end = '\r';
	} else {
		end = ':';
	}

	data_len = strtol(&ipd_buf[len + 3], &endptr, 10);
	if (endptr == &ipd_buf[len + 3] ||
	    (*endptr == 0 && match_len >= MAX_IPD_LEN)) {
		/* Invalid */
		LOG_ERR("Invalid IPD len: %s", log_strdup(ipd_buf));
		ret = len;
		goto out;
	} else if (*endptr == 0) {
		ret = -EAGAIN;
		goto out;
	} else if (*endptr != end) {
		LOG_ERR("Invalid cmd end 0x%02x, expected 0x%02x", *endptr,
			end);
		ret = len;
		goto out;
	}

	*endptr = 0;
	data_offset = strlen(ipd_buf) + 1;

	/*
	 * When using passive TCP, the data itself is not included in the +IPD
	 * command but must be polled with AT+CIPRECVDATA.
	 */
	if (ESP_PROTO_PASSIVE(sock->ip_proto)) {
		sock->bytes_avail = data_len;
		k_work_submit_to_queue(&dev->workq, &sock->recvdata_work);
		ret = data_offset;
		goto out;
	}

	/* Do we have the whole message? */
	if (data_offset + data_len > frags_len) {
		ret = -EAGAIN;
		goto out;
	}

	ret = data_offset + data_len; /* Skip */

	pkt = esp_prepare_pkt(dev, data->rx_buf, data_offset, data_len);
	if (!pkt) {
		/* FIXME: Should probably terminate connection */
		LOG_ERR("Failed to get net_pkt: len %d", data_len);
		goto out;
	}

	k_fifo_put(&sock->fifo_rx_pkt, pkt);
	k_work_submit_to_queue(&dev->workq, &sock->recv_work);

out:
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

	if (net_if_is_up(dev->net_iface)) {
		net_if_down(dev->net_iface);
		LOG_ERR("Unexpected reset");
	}

	if (esp_flag_is_set(dev, EDF_STA_CONNECTING)) {
		esp_flag_clear(dev, EDF_STA_CONNECTING);
		wifi_mgmt_raise_connect_result_event(dev->net_iface, -1);
	} else if (esp_flag_is_set(dev, EDF_STA_CONNECTED)) {
		esp_flag_clear(dev, EDF_STA_CONNECTED);
		wifi_mgmt_raise_disconnect_result_event(dev->net_iface, 0);
	}

	net_if_ipv4_addr_rm(dev->net_iface, &dev->ip);
	k_work_submit_to_queue(&dev->workq, &dev->init_work);

	return 0;
}

static struct modem_cmd unsol_cmds[] = {
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
	MODEM_CMD_DIRECT("+IPD", on_cmd_ipd),
};

static void esp_mgmt_scan_work(struct k_work *work)
{
	struct esp_data *dev;
	int ret;
	struct modem_cmd cmds[] = {
		MODEM_CMD("+CWLAP:", on_cmd_cwlap, 4U, ","),
	};

	dev = CONTAINER_OF(work, struct esp_data, scan_work);

	ret = modem_cmd_send(&dev->mctx.iface, &dev->mctx.cmd_handler,
			     cmds, ARRAY_SIZE(cmds), "AT+CWLAP",
			     &dev->sem_response, ESP_SCAN_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Failed to scan: ret %d", ret);
	}

	dev->scan_cb(dev->net_iface, 0, NULL);
	dev->scan_cb = NULL;
}

static int esp_mgmt_scan(struct device *dev, scan_result_cb_t cb)
{
	struct esp_data *data = dev->data;

	if (data->scan_cb != NULL) {
		return -EINPROGRESS;
	}

	if (!net_if_is_up(data->net_iface)) {
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
	struct modem_cmd cmds[] = {
		MODEM_CMD("FAIL", on_cmd_fail, 0U, ""),
	};

	dev = CONTAINER_OF(work, struct esp_data, connect_work);

	ret = modem_cmd_send(&dev->mctx.iface, &dev->mctx.cmd_handler,
			     cmds, ARRAY_SIZE(cmds), dev->conn_cmd,
			     &dev->sem_response, ESP_CONNECT_TIMEOUT);

	memset(dev->conn_cmd, 0, sizeof(dev->conn_cmd));

	if (ret < 0) {
		if (esp_flag_is_set(dev, EDF_STA_CONNECTED)) {
			esp_flag_clear(dev, EDF_STA_CONNECTED);
			wifi_mgmt_raise_disconnect_result_event(dev->net_iface,
								0);
		} else {
			wifi_mgmt_raise_connect_result_event(dev->net_iface,
							     ret);
		}
	} else if (!esp_flag_is_set(dev, EDF_STA_CONNECTED)) {
		esp_flag_set(dev, EDF_STA_CONNECTED);
		wifi_mgmt_raise_connect_result_event(dev->net_iface, 0);
	}

	esp_flag_clear(dev, EDF_STA_CONNECTING);
}

static int esp_mgmt_connect(struct device *dev,
			    struct wifi_connect_req_params *params)
{
	struct esp_data *data = dev->data;
	int len;

	if (!net_if_is_up(data->net_iface)) {
		return -EIO;
	}

	if (esp_flag_is_set(data, EDF_STA_CONNECTED) ||
	    esp_flag_is_set(data, EDF_STA_CONNECTING)) {
		return -EALREADY;
	}

	esp_flag_set(data, EDF_STA_CONNECTING);

	len = snprintk(data->conn_cmd, sizeof(data->conn_cmd),
		       "AT+"_CWJAP"=\"");
	memcpy(&data->conn_cmd[len], params->ssid, params->ssid_length);
	len += params->ssid_length;

	if (params->security == WIFI_SECURITY_TYPE_PSK) {
		len += snprintk(&data->conn_cmd[len],
				sizeof(data->conn_cmd) - len, "\",\"");
		memcpy(&data->conn_cmd[len], params->psk, params->psk_length);
		len += params->psk_length;
	}

	len += snprintk(&data->conn_cmd[len], sizeof(data->conn_cmd) - len,
			"\"");

	k_work_submit_to_queue(&data->workq, &data->connect_work);

	return 0;
}

static int esp_mgmt_disconnect(struct device *dev)
{
	struct esp_data *data = dev->data;
	int ret;

	ret = modem_cmd_send(&data->mctx.iface, &data->mctx.cmd_handler,
			     NULL, 0, "AT+CWQAP", &data->sem_response,
			     ESP_CMD_TIMEOUT);

	return ret;
}

static int esp_mgmt_ap_enable(struct device *dev,
			      struct wifi_connect_req_params *params)
{
	char cmd[sizeof("AT+"_CWSAP"=\"\",\"\",xx,x") + WIFI_SSID_MAX_LEN +
		 WIFI_PSK_MAX_LEN];
	struct esp_data *data = dev->data;
	int ecn = 0, len, ret;

	ret = modem_cmd_send(&data->mctx.iface, &data->mctx.cmd_handler,
			     NULL, 0, "AT+"_CWMODE"=3", &data->sem_response,
			     ESP_CMD_TIMEOUT);
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

	ret = modem_cmd_send(&data->mctx.iface, &data->mctx.cmd_handler,
			     NULL, 0, cmd, &data->sem_response,
			     ESP_CMD_TIMEOUT);

	return ret;
}

static int esp_mgmt_ap_disable(struct device *dev)
{
	struct esp_data *data = dev->data;
	int ret;

	ret = modem_cmd_send(&data->mctx.iface, &data->mctx.cmd_handler,
			     NULL, 0, "AT+"_CWMODE"=1", &data->sem_response,
			     ESP_CMD_TIMEOUT);

	return ret;
}

static void esp_init_work(struct k_work *work)
{
	struct esp_data *dev;
	int ret;
	static struct setup_cmd setup_cmds[] = {
		SETUP_CMD_NOHANDLE("AT"),
		/* turn off echo */
		SETUP_CMD_NOHANDLE("ATE0"),
		SETUP_CMD_NOHANDLE("AT+UART_CUR="_UART_CUR),
		SETUP_CMD_NOHANDLE("AT+"_CWMODE"=1"),
		/* enable multiple socket support */
		SETUP_CMD_NOHANDLE("AT+CIPMUX=1"),
		/* only need ecn,ssid,rssi,channel */
		SETUP_CMD_NOHANDLE("AT+CWLAPOPT=0,23"),
#if defined(CONFIG_WIFI_ESP_AT_VERSION_2_0)
		SETUP_CMD_NOHANDLE("AT+CWAUTOCONN=0"),
#endif
#if defined(CONFIG_WIFI_ESP_PASSIVE_MODE)
		SETUP_CMD_NOHANDLE("AT+CIPRECVMODE=1"),
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

	net_if_set_link_addr(dev->net_iface, dev->mac_addr,
			     sizeof(dev->mac_addr), NET_LINK_ETHERNET);

	LOG_INF("ESP Wi-Fi ready");

	net_if_up(dev->net_iface);

	k_sem_give(&dev->sem_if_up);
}

static void esp_reset(struct esp_data *dev)
{
	int ret;

	if (net_if_is_up(dev->net_iface)) {
		net_if_down(dev->net_iface);
	}

#if DT_INST_NODE_HAS_PROP(0, wifi_reset_gpios)
	modem_pin_write(&dev->mctx, WIFI_RESET, 1);
	k_sleep(K_MSEC(100));
	modem_pin_write(&dev->mctx, WIFI_RESET, 0);
#else
	int retries = 3;

	while (retries--) {
		ret = modem_cmd_send(&dev->mctx.iface, &dev->mctx.cmd_handler,
				     NULL, 0, "AT+RST", &dev->sem_if_ready,
				     K_MSEC(CONFIG_WIFI_ESP_RESET_TIMEOUT));
		if (ret == 0 || ret != -ETIMEDOUT) {
			break;
		}
	}

	if (ret < 0) {
		LOG_ERR("Failed to reset device: %d", ret);
		return;
	}
#endif

	LOG_INF("Waiting for interface to come up");

	ret = k_sem_take(&dev->sem_if_up, ESP_INIT_TIMEOUT);
	if (ret == -EAGAIN) {
		LOG_ERR("Timeout waiting for interface");
	}
}

static void esp_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct esp_data *data = dev->data;

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	data->net_iface = iface;
	esp_offload_init(iface);
	esp_reset(data);
}

static const struct net_wifi_mgmt_offload esp_api = {
	.iface_api.init = esp_iface_init,
	.scan		= esp_mgmt_scan,
	.connect	= esp_mgmt_connect,
	.disconnect	= esp_mgmt_disconnect,
	.ap_enable	= esp_mgmt_ap_enable,
	.ap_disable	= esp_mgmt_ap_disable,
};

static int esp_init(struct device *dev)
{
	struct esp_data *data = dev->data;
	int ret = 0;

	k_sem_init(&data->sem_tx_ready, 0, 1);
	k_sem_init(&data->sem_response, 0, 1);
	k_sem_init(&data->sem_if_ready, 0, 1);
	k_sem_init(&data->sem_if_up, 0, 1);

	k_work_init(&data->init_work, esp_init_work);
	k_delayed_work_init(&data->ip_addr_work, esp_ip_addr_work);
	k_work_init(&data->scan_work, esp_mgmt_scan_work);
	k_work_init(&data->connect_work, esp_mgmt_connect_work);

	esp_socket_init(data);

	/* initialize the work queue */
	k_work_q_start(&data->workq, esp_workq_stack,
		       K_KERNEL_STACK_SIZEOF(esp_workq_stack),
		       K_PRIO_COOP(CONFIG_WIFI_ESP_WORKQ_THREAD_PRIORITY));
	k_thread_name_set(&data->workq.thread, "esp_workq");

	/* cmd handler */
	data->cmd_handler_data.cmds[CMD_RESP] = response_cmds;
	data->cmd_handler_data.cmds_len[CMD_RESP] = ARRAY_SIZE(response_cmds);
	data->cmd_handler_data.cmds[CMD_UNSOL] = unsol_cmds;
	data->cmd_handler_data.cmds_len[CMD_UNSOL] = ARRAY_SIZE(unsol_cmds);
	data->cmd_handler_data.read_buf = &data->cmd_read_buf[0];
	data->cmd_handler_data.read_buf_len = sizeof(data->cmd_read_buf);
	data->cmd_handler_data.match_buf = &data->cmd_match_buf[0];
	data->cmd_handler_data.match_buf_len = sizeof(data->cmd_match_buf);
	data->cmd_handler_data.buf_pool = &mdm_recv_pool;
	data->cmd_handler_data.alloc_timeout = CMD_BUF_ALLOC_TIMEOUT;
	data->cmd_handler_data.eol = "\r\n";
	ret = modem_cmd_handler_init(&data->mctx.cmd_handler,
				       &data->cmd_handler_data);
	if (ret < 0) {
		goto error;
	}

	/* modem interface */
	data->iface_data.isr_buf = &data->iface_isr_buf[0];
	data->iface_data.isr_buf_len = sizeof(data->iface_isr_buf);
	data->iface_data.rx_rb_buf = &data->iface_rb_buf[0];
	data->iface_data.rx_rb_buf_len = sizeof(data->iface_rb_buf);
	ret = modem_iface_uart_init(&data->mctx.iface, &data->iface_data,
				    DT_INST_BUS_LABEL(0));
	if (ret < 0) {
		goto error;
	}

	/* pin setup */
	data->mctx.pins = modem_pins;
	data->mctx.pins_len = ARRAY_SIZE(modem_pins);

	data->mctx.driver_data = data;

	ret = modem_context_register(&data->mctx);
	if (ret < 0) {
		LOG_ERR("Error registering modem context: %d", ret);
		goto error;
	}

	/* start RX thread */
	k_thread_create(&esp_rx_thread, esp_rx_stack,
			K_KERNEL_STACK_SIZEOF(esp_rx_stack),
			(k_thread_entry_t)esp_rx,
			dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_WIFI_ESP_RX_THREAD_PRIORITY), 0,
			K_NO_WAIT);
	k_thread_name_set(&esp_rx_thread, "esp_rx");

error:
	return ret;
}

NET_DEVICE_OFFLOAD_INIT(wifi_esp, CONFIG_WIFI_ESP_NAME,
			esp_init, device_pm_control_nop, &esp_driver_data, NULL,
			CONFIG_WIFI_INIT_PRIORITY, &esp_api,
			ESP_MTU);
