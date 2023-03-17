/**
 * Copyright (c) 2018 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_WIFI_ESWIFI_BUS_UART)
#define DT_DRV_COMPAT inventek_eswifi_uart
#else
#define DT_DRV_COMPAT inventek_eswifi
#endif

#include "eswifi_log.h"
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/wifi_mgmt.h>

#include <zephyr/net/ethernet.h>
#include <net_private.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/sys/printk.h>

#include "eswifi.h"

#define ESWIFI_WORKQUEUE_STACK_SIZE 1024
K_KERNEL_STACK_DEFINE(eswifi_work_q_stack, ESWIFI_WORKQUEUE_STACK_SIZE);

static const struct eswifi_cfg eswifi0_cfg = {
	.resetn = GPIO_DT_SPEC_INST_GET(0, resetn_gpios),
	.wakeup = GPIO_DT_SPEC_INST_GET(0, wakeup_gpios),
};

static struct eswifi_dev eswifi0; /* static instance */

static int eswifi_reset(struct eswifi_dev *eswifi, const struct eswifi_cfg *cfg)
{
	gpio_pin_set_dt(&cfg->resetn, 0);
	k_sleep(K_MSEC(10));
	gpio_pin_set_dt(&cfg->resetn, 1);
	gpio_pin_set_dt(&cfg->wakeup, 1);
	k_sleep(K_MSEC(500));

	/* fetch the cursor */
	return eswifi_request(eswifi, NULL, 0, eswifi->buf,
			      sizeof(eswifi->buf));
}

static inline int __parse_ssid(char *str, char *ssid)
{
	int i = 0;

	/* fmt => "SSID" */

	if (*str != '"') {
		return 0;
	}
	str++;

	while (*str && (*str != '"') && i < WIFI_SSID_MAX_LEN) {
		ssid[i++] = *str++;
	}

	if (*str != '"') {
		return 0;
	}

	return i;
}

static void __parse_scan_res(char *str, struct wifi_scan_result *res)
{
	int field = 0;

	/* fmt => #001,"SSID",MACADDR,RSSI,BITRATE,MODE,SECURITY,BAND,CHANNEL */

	while (*str) {
		if (*str != ',') {
			str++;
			continue;
		}

		if (!*++str) {
			break;
		}

		switch (++field) {
		case 1: /* SSID */
			res->ssid_length = __parse_ssid(str, res->ssid);
			str += res->ssid_length;
			break;
		case 2: /* mac addr */
			break;
		case 3: /* RSSI */
			res->rssi = atoi(str);
			break;
		case 4: /* bitrate */
			break;
		case 5: /* mode */
			break;
		case 6: /* security */
			if (!strncmp(str, "Open", 4)) {
				res->security = WIFI_SECURITY_TYPE_NONE;
			} else {
				res->security = WIFI_SECURITY_TYPE_PSK;
			}
			break;
		case 7: /* band */
			break;
		case 8: /* channel */
			res->channel = atoi(str);
			break;
		}

	}
}

int eswifi_at_cmd_rsp(struct eswifi_dev *eswifi, char *cmd, char **rsp)
{
	const char startstr[] = "\r\n";
	const char endstr[] = "\r\nOK\r\n>";
	int i, len, rsplen = -EINVAL;

	len = eswifi_request(eswifi, cmd, strlen(cmd), eswifi->buf,
			     sizeof(eswifi->buf));
	if (len < 0) {
		return -EIO;
	}

	/*
	 * Check response, format should be "\r\n[DATA]\r\nOK\r\n>"
	 * Data is in arbitrary format (not only ASCII)
	 */

	/* Check start characters */
	if (strncmp(eswifi->buf, startstr, strlen(startstr))) {
		return -EINVAL;
	}

	if (len < sizeof(endstr) - 1 + sizeof(startstr) - 1) {
		return -EINVAL;
	}

	/* Check end characters */
	for (i = len - sizeof(endstr); i > 0; i--) {
		if (!strncmp(&eswifi->buf[i], endstr, 7)) {
			if (rsp) {
				eswifi->buf[i] = '\0';
				*rsp = &eswifi->buf[2];
				rsplen = &eswifi->buf[i] - *rsp;
			} else {
				rsplen = 0;
			}
			break;
		}
	}

	return rsplen;
}

int eswifi_at_cmd(struct eswifi_dev *eswifi, char *cmd)
{
	return eswifi_at_cmd_rsp(eswifi, cmd, NULL);
}

struct eswifi_dev *eswifi_by_iface_idx(uint8_t iface)
{
	/* only one instance */
	LOG_DBG("%d", iface);
	return &eswifi0;
}

static int __parse_ipv4_address(char *str, char *ssid, uint8_t ip[4])
{
	int byte = -1;

	/* fmt => [JOIN   ] SSID,192.168.2.18,0,0 */
	while (*str && byte < 4) {
		if (byte == -1) {
			if (!strncmp(str, ssid, strlen(ssid))) {
				byte = 0;
				str += strlen(ssid);
			}
			str++;
			continue;
		}

		ip[byte++] = atoi(str);
		while (*str && (*str++ != '.')) {
		}
	}

	return 0;
}

static void eswifi_scan(struct eswifi_dev *eswifi)
{
	char cmd[] = "F0\r";
	char *data;
	int i, ret;

	LOG_DBG("");

	eswifi_lock(eswifi);

	ret = eswifi_at_cmd_rsp(eswifi, cmd, &data);
	if (ret < 0) {
		eswifi->scan_cb(eswifi->iface, -EIO, NULL);
		eswifi_unlock(eswifi);
		return;
	}

	for (i = 0; i < ret; i++) {
		if (data[i] == '#') {
			struct wifi_scan_result res = {0};

			__parse_scan_res(&data[i], &res);

			eswifi->scan_cb(eswifi->iface, 0, &res);
			k_yield();

			while (data[i] && data[i] != '\n') {
				i++;
			}
		}
	}

	/* WiFi scan is done. */
	eswifi->scan_cb(eswifi->iface, 0, NULL);

	eswifi_unlock(eswifi);
}

static int eswifi_connect(struct eswifi_dev *eswifi)
{
	char connect[] = "C0\r";
	struct in_addr addr;
	char *rsp;
	int err;

	LOG_DBG("Connecting to %s (pass=%s)", eswifi->sta.ssid,
		eswifi->sta.pass);

	eswifi_lock(eswifi);

	/* Set SSID */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "C1=%s\r", eswifi->sta.ssid);
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set SSID");
		goto error;
	}

	/* Set passphrase */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "C2=%s\r", eswifi->sta.pass);
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set passphrase");
		goto error;
	}

	/* Set Security type */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "C3=%u\r",
		 eswifi->sta.security);
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to configure security");
		goto error;
	}

	/* Join Network */
	err = eswifi_at_cmd_rsp(eswifi, connect, &rsp);
	if (err < 0) {
		LOG_ERR("Unable to join network");
		goto error;
	}

	/* Any IP assigned ? (dhcp offload or manually) */
	err = __parse_ipv4_address(rsp, eswifi->sta.ssid,
				   (uint8_t *)&addr.s4_addr);
	if (err < 0) {
		LOG_ERR("Unable to retrieve IP address");
		goto error;
	}

	LOG_DBG("ip = %d.%d.%d.%d", addr.s4_addr[0], addr.s4_addr[1],
		   addr.s4_addr[2], addr.s4_addr[3]);

	net_if_ipv4_addr_add(eswifi->iface, &addr, NET_ADDR_DHCP, 0);

	eswifi->sta.connected = true;

	LOG_DBG("Connected!");

	eswifi_unlock(eswifi);
	return 0;

error:
	eswifi_unlock(eswifi);
	return -EIO;
}

static int eswifi_disconnect(struct eswifi_dev *eswifi)
{
	char disconnect[] = "CD\r";
	int err;

	LOG_DBG("");

	eswifi_lock(eswifi);

	err = eswifi_at_cmd(eswifi, disconnect);
	if (err < 0) {
		LOG_ERR("Unable to disconnect network");
		err = -EIO;
	}

	eswifi->sta.connected = false;

	eswifi_unlock(eswifi);

	return err;
}

static void eswifi_status_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct eswifi_dev *eswifi;
	char status[] = "CS\r";
	char rssi[] = "CR\r";
	char *rsp;
	int ret;

	eswifi = CONTAINER_OF(dwork, struct eswifi_dev, status_work);

	eswifi_lock(eswifi);

	if (eswifi->role == ESWIFI_ROLE_AP) {
		goto done;
	}

	ret = eswifi_at_cmd_rsp(eswifi, status, &rsp);
	if (ret < 1) {
		LOG_ERR("Unable to retrieve status");
		goto done;
	}

	if (rsp[0] == '0' && eswifi->sta.connected) {
		eswifi->sta.connected = false;
		wifi_mgmt_raise_disconnect_result_event(eswifi->iface, 0);
		goto done;
	} else if (rsp[0] == '1' && !eswifi->sta.connected) {
		eswifi->sta.connected = true;
		wifi_mgmt_raise_connect_result_event(eswifi->iface, 0);
	}

	ret = eswifi_at_cmd_rsp(eswifi, rssi, &rsp);
	if (ret < 1) {
		LOG_ERR("Unable to retrieve rssi");
		/* continue */
	} else {
		eswifi->sta.rssi = atoi(rsp);
	}

	k_work_reschedule_for_queue(&eswifi->work_q, &eswifi->status_work,
				    K_MSEC(1000 * 30));

done:
	eswifi_unlock(eswifi);
}

static void eswifi_request_work(struct k_work *item)
{
	struct eswifi_dev *eswifi;
	int err;

	LOG_DBG("");

	eswifi = CONTAINER_OF(item, struct eswifi_dev, request_work);

	switch (eswifi->req) {
	case ESWIFI_REQ_CONNECT:
		err = eswifi_connect(eswifi);
		wifi_mgmt_raise_connect_result_event(eswifi->iface, err);
		k_work_reschedule_for_queue(&eswifi->work_q, &eswifi->status_work,
					    K_MSEC(1000));
		break;
	case ESWIFI_REQ_DISCONNECT:
		err = eswifi_disconnect(eswifi);
		wifi_mgmt_raise_disconnect_result_event(eswifi->iface, err);
		break;
	case ESWIFI_REQ_SCAN:
		eswifi_scan(eswifi);
		break;
	case ESWIFI_REQ_NONE:
	default:
		break;
	}
}

static int eswifi_get_mac_addr(struct eswifi_dev *eswifi, uint8_t addr[6])
{
	char cmd[] = "Z5\r";
	int ret, i, byte = 0;
	char *rsp;

	ret = eswifi_at_cmd_rsp(eswifi, cmd, &rsp);
	if (ret < 0) {
		return ret;
	}

	/* format is "ff:ff:ff:ff:ff:ff" */
	for (i = 0; i < ret && byte < 6; i++) {
		addr[byte++] = strtol(&rsp[i], NULL, 16);
		i += 2;
	}

	if (byte != 6) {
		return -EIO;
	}

	return 0;
}

static void eswifi_iface_init(struct net_if *iface)
{
	struct eswifi_dev *eswifi = &eswifi0;
	const struct eswifi_cfg *cfg = &eswifi0_cfg;
	uint8_t mac[6];

	LOG_DBG("");

	eswifi_lock(eswifi);

	if (eswifi_reset(eswifi, cfg) < 0) {
		LOG_ERR("Unable to reset device");
		return;
	}

	if (eswifi_get_mac_addr(eswifi, mac) < 0) {
		LOG_ERR("Unable to read MAC address");
		return;
	}

	LOG_DBG("MAC Address %02X:%02X:%02X:%02X:%02X:%02X",
		   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	memcpy(eswifi->mac, mac, sizeof(eswifi->mac));
	net_if_set_link_addr(iface, eswifi->mac, sizeof(eswifi->mac),
			     NET_LINK_ETHERNET);

	eswifi->iface = iface;

	eswifi_unlock(eswifi);

	eswifi_offload_init(eswifi);
#if defined(CONFIG_NET_SOCKETS_OFFLOAD)
	eswifi_socket_offload_init(eswifi);

	net_if_socket_offload_set(iface, eswifi_socket_create);
#endif

}

int eswifi_mgmt_iface_status(const struct device *dev,
			     struct wifi_iface_status *status)
{
	struct eswifi_dev *eswifi = dev->data;
	struct eswifi_sta *sta = &eswifi->sta;

	/* Update status */
	eswifi_status_work(&eswifi->status_work.work);

	if (!sta->connected) {
		status->state = WIFI_STATE_DISCONNECTED;
		return 0;
	}

	status->state = WIFI_STATE_COMPLETED;
	strcpy(status->ssid, sta->ssid);
	status->ssid_len = strlen(sta->ssid);
	status->band = WIFI_FREQ_BAND_2_4_GHZ;
	status->channel = 0;

	if (eswifi->role == ESWIFI_ROLE_CLIENT) {
		status->iface_mode = WIFI_MODE_INFRA;
	} else {
		status->iface_mode = WIFI_MODE_AP;
	}

	status->link_mode = WIFI_LINK_MODE_UNKNOWN;

	switch (sta->security) {
	case ESWIFI_SEC_OPEN:
		status->security = WIFI_SECURITY_TYPE_NONE;
		break;
	case ESWIFI_SEC_WPA2_MIXED:
		status->security = WIFI_SECURITY_TYPE_PSK;
		break;
	default:
		status->security = WIFI_SECURITY_TYPE_UNKNOWN;
	}

	status->mfp = WIFI_MFP_DISABLE;
	status->rssi = sta->rssi;

	return 0;
}

static int eswifi_mgmt_scan(const struct device *dev, scan_result_cb_t cb)
{
	struct eswifi_dev *eswifi = dev->data;

	LOG_DBG("");

	eswifi_lock(eswifi);

	eswifi->scan_cb = cb;
	eswifi->req = ESWIFI_REQ_SCAN;
	k_work_submit_to_queue(&eswifi->work_q, &eswifi->request_work);

	eswifi_unlock(eswifi);

	return 0;
}

static int eswifi_mgmt_disconnect(const struct device *dev)
{
	struct eswifi_dev *eswifi = dev->data;

	LOG_DBG("");

	eswifi_lock(eswifi);

	eswifi->req = ESWIFI_REQ_DISCONNECT;
	k_work_submit_to_queue(&eswifi->work_q, &eswifi->request_work);

	eswifi_unlock(eswifi);

	return 0;
}

static int __eswifi_sta_config(struct eswifi_dev *eswifi,
			       struct wifi_connect_req_params *params)
{
	memcpy(eswifi->sta.ssid, params->ssid, params->ssid_length);
	eswifi->sta.ssid[params->ssid_length] = '\0';

	switch (params->security) {
	case WIFI_SECURITY_TYPE_NONE:
		eswifi->sta.pass[0] = '\0';
		eswifi->sta.security = ESWIFI_SEC_OPEN;
		break;
	case WIFI_SECURITY_TYPE_PSK:
		memcpy(eswifi->sta.pass, params->psk, params->psk_length);
		eswifi->sta.pass[params->psk_length] = '\0';
		eswifi->sta.security = ESWIFI_SEC_WPA2_MIXED;
		break;
	default:
		return -EINVAL;
	}

	if (params->channel == WIFI_CHANNEL_ANY) {
		eswifi->sta.channel = 0U;
	} else {
		eswifi->sta.channel = params->channel;
	}

	return 0;
}

static int eswifi_mgmt_connect(const struct device *dev,
			       struct wifi_connect_req_params *params)
{
	struct eswifi_dev *eswifi = dev->data;
	int err;

	LOG_DBG("");

	eswifi_lock(eswifi);

	err = __eswifi_sta_config(eswifi, params);
	if (!err) {
		eswifi->req = ESWIFI_REQ_CONNECT;
		k_work_submit_to_queue(&eswifi->work_q,
				       &eswifi->request_work);
	}

	eswifi_unlock(eswifi);

	return err;
}

void eswifi_async_msg(struct eswifi_dev *eswifi, char *msg, size_t len)
{
	eswifi_offload_async_msg(eswifi, msg, len);
}

#if defined(CONFIG_NET_IPV4)
static int eswifi_mgmt_ap_enable(const struct device *dev,
				 struct wifi_connect_req_params *params)
{
	struct eswifi_dev *eswifi = dev->data;
	struct net_if_ipv4 *ipv4 = eswifi->iface->config.ip.ipv4;
	struct net_if_addr *unicast = NULL;
	int err = -EIO, i;

	LOG_DBG("");

	eswifi_lock(eswifi);

	if (eswifi->role == ESWIFI_ROLE_AP) {
		err = -EALREADY;
		goto error;
	}

	err = __eswifi_sta_config(eswifi, params);
	if (err) {
		goto error;
	}

	/* security */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "A1=%u\r",
		 eswifi->sta.security);
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set Security");
		goto error;
	}

	/* Passkey */
	if (eswifi->sta.security != ESWIFI_SEC_OPEN) {
		snprintk(eswifi->buf, sizeof(eswifi->buf), "A2=%s\r",
			 eswifi->sta.pass);
		err = eswifi_at_cmd(eswifi, eswifi->buf);
		if (err < 0) {
			LOG_ERR("Unable to set passkey");
			goto error;
		}
	}

	/* Set SSID (0=no MAC, 1=append MAC) */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "AS=0,%s\r",
		 eswifi->sta.ssid);
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set SSID");
		goto error;
	}

	/* Set Channel */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "AC=%u\r",
		 eswifi->sta.channel);
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to set Channel");
		goto error;
	}

	/* Set IP Address */
	for (i = 0; ipv4 && i < NET_IF_MAX_IPV4_ADDR; i++) {
		if (ipv4->unicast[i].is_used) {
			unicast = &ipv4->unicast[i];
			break;
		}
	}

	if (!unicast) {
		LOG_ERR("No IPv4 assigned for AP mode");
		err = -EADDRNOTAVAIL;
		goto error;
	}

	snprintk(eswifi->buf, sizeof(eswifi->buf), "Z6=%s\r",
		 net_sprint_ipv4_addr(&unicast->address.in_addr));
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to active access point");
		goto error;
	}

	/* Enable AP */
	snprintk(eswifi->buf, sizeof(eswifi->buf), "AD\r");
	err = eswifi_at_cmd(eswifi, eswifi->buf);
	if (err < 0) {
		LOG_ERR("Unable to active access point");
		goto error;
	}

	eswifi->role = ESWIFI_ROLE_AP;

	eswifi_unlock(eswifi);
	return 0;
error:
	eswifi_unlock(eswifi);
	return err;
}
#else
static int eswifi_mgmt_ap_enable(const struct device *dev,
				 struct wifi_connect_req_params *params)
{
	LOG_ERR("IPv4 requested for AP mode");
	return -ENOTSUP;
}
#endif /* CONFIG_NET_IPV4 */

static int eswifi_mgmt_ap_disable(const struct device *dev)
{
	struct eswifi_dev *eswifi = dev->data;
	char cmd[] = "AE\r";
	int err;

	eswifi_lock(eswifi);

	err = eswifi_at_cmd(eswifi, cmd);
	if (err < 0) {
		eswifi_unlock(eswifi);
		return -EIO;
	}

	eswifi->role = ESWIFI_ROLE_CLIENT;

	eswifi_unlock(eswifi);

	return 0;
}

static int eswifi_init(const struct device *dev)
{
	struct eswifi_dev *eswifi = dev->data;
	const struct eswifi_cfg *cfg = dev->config;

	LOG_DBG("");

	eswifi->role = ESWIFI_ROLE_CLIENT;
	k_mutex_init(&eswifi->mutex);

	eswifi->bus = eswifi_get_bus();
	eswifi->bus->init(eswifi);

	if (!device_is_ready(cfg->resetn.port)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
				cfg->resetn.port->name);
		return -ENODEV;
	}
	gpio_pin_configure_dt(&cfg->resetn, GPIO_OUTPUT_INACTIVE);

	if (!device_is_ready(cfg->wakeup.port)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
				cfg->wakeup.port->name);
		return -ENODEV;
	}
	gpio_pin_configure_dt(&cfg->wakeup, GPIO_OUTPUT_ACTIVE);

	k_work_queue_start(&eswifi->work_q, eswifi_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(eswifi_work_q_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY - 1, NULL);

	k_work_init(&eswifi->request_work, eswifi_request_work);
	k_work_init_delayable(&eswifi->status_work, eswifi_status_work);

	eswifi_shell_register(eswifi);

	return 0;
}

static const struct net_wifi_mgmt_offload eswifi_offload_api = {
	.wifi_iface.iface_api.init = eswifi_iface_init,
	.scan			   = eswifi_mgmt_scan,
	.connect		   = eswifi_mgmt_connect,
	.disconnect		   = eswifi_mgmt_disconnect,
	.ap_enable		   = eswifi_mgmt_ap_enable,
	.ap_disable		   = eswifi_mgmt_ap_disable,
	.iface_status		   = eswifi_mgmt_iface_status,
};

NET_DEVICE_DT_INST_OFFLOAD_DEFINE(0, eswifi_init, NULL,
				  &eswifi0, &eswifi0_cfg,
				  CONFIG_WIFI_INIT_PRIORITY,
				  &eswifi_offload_api,
				  1500);
