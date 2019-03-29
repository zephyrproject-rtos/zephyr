/*
 * @file
 * @brief Driver interface implementation for Zephyr
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_WIFIMGR_STA) || defined(CONFIG_WIFIMGR_AP)

#include <net/wifi_drv.h>

#include "os_adapter.h"
#include "drv_iface.h"

void *wifi_drv_init(char *devname)
{
	struct device *dev;
	struct net_if *iface;

	if (!devname)
		return NULL;

	dev = device_get_binding(devname);
	if (!dev)
		return NULL;

	iface = net_if_lookup_by_dev(dev);

	return (void *)iface;
}

int wifi_drv_get_mac(void *iface, char *mac)
{
	if (!mac)
		return -EINVAL;

	memcpy(mac, net_if_get_link_addr(iface)->addr,
	       NET_LINK_ADDR_MAX_LENGTH);

	return 0;
}

int wifi_drv_get_capa(void *iface, union wifi_drv_capa *capa)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->get_capa)
		return -EIO;

	return drv_api->get_capa(dev, capa);
}

int wifi_drv_open(void *iface)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->open)
		return -EIO;

	return drv_api->open(dev);
}

int wifi_drv_close(void *iface)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->close)
		return -EIO;

	return drv_api->close(dev);
}

static
void wifi_drv_event_iface_scan_result(void *iface, int status,
				      struct wifi_drv_scan_result_evt *entry)
{
	struct wifi_drv_scan_result_evt *scan_res = entry;
	char evt_status = status;

	if (!entry)
		wifimgr_notify_event(WIFIMGR_EVT_SCAN_DONE, &evt_status,
				     sizeof(evt_status));
	else
		wifimgr_notify_event(WIFIMGR_EVT_SCAN_RESULT, scan_res,
				     sizeof(struct wifi_drv_scan_result_evt));
}

int wifi_drv_scan(void *iface, unsigned char band, unsigned char channel)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;
	struct wifi_drv_scan_params params;

	if (!drv_api->scan)
		return -EIO;

	params.band = band;
	params.channel = channel;

	return drv_api->scan(dev, &params, wifi_drv_event_iface_scan_result);
}

#ifdef CONFIG_WIFIMGR_STA
static
void wifi_drv_event_iface_rtt_response(void *iface, int status,
				       struct wifi_drv_rtt_response_evt *entry)
{
	struct wifi_drv_rtt_response_evt *rtt_resp = entry;
	char evt_status = status;

	if (!entry)
		wifimgr_notify_event(WIFIMGR_EVT_RTT_DONE, &evt_status,
				     sizeof(evt_status));
	else
		wifimgr_notify_event(WIFIMGR_EVT_RTT_RESPONSE, rtt_resp,
				     sizeof(struct wifi_drv_rtt_response_evt));
}

int wifi_drv_rtt(void *iface, struct wifi_rtt_peers *peers,
		 unsigned char nr_peers)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;
	struct wifi_drv_rtt_request params;

	if (!drv_api->rtt_req)
		return -EIO;

	params.nr_peers = nr_peers;
	params.peers = peers;

	return drv_api->rtt_req(dev, &params,
				wifi_drv_event_iface_rtt_response);
}

static void wifi_drv_event_disconnect(void *iface, int status)
{
	char reason_code = status;

	wifimgr_notify_event(WIFIMGR_EVT_DISCONNECT, &reason_code,
			     sizeof(reason_code));
}

int wifi_drv_disconnect(void *iface)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->disconnect)
		return -EIO;

	return drv_api->disconnect(dev, wifi_drv_event_disconnect);
}

static void wifi_drv_event_connect(void *iface, int status, char *bssid,
				   unsigned char channel)
{
	struct wifi_drv_connect_evt conn;

	conn.status = status;
	if (bssid && !is_zero_ether_addr(bssid))
		memcpy(conn.bssid, bssid, NET_LINK_ADDR_MAX_LENGTH);
	conn.channel = channel;

	wifimgr_notify_event(WIFIMGR_EVT_CONNECT, &conn, sizeof(conn));
}

int wifi_drv_connect(void *iface, char *ssid, char *bssid, char *psk,
		     unsigned char psk_len, unsigned char channel)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;
	struct wifi_drv_connect_params params;

	if (!drv_api->connect)
		return -EIO;

	/* SSID is mandatory */
	params.ssid_len = strlen(ssid);
	if (!ssid || !params.ssid_len)
		return -EINVAL;
	params.ssid = ssid;

	/* BSSID is optional */
	if (bssid && is_zero_ether_addr(bssid))
		return -EINVAL;
	params.bssid = bssid;

	/* Passphrase is only valid for WPA/WPA2-PSK */
	params.psk_len = psk_len;
	if (psk && !params.psk_len)
		return -EINVAL;
	params.psk = psk;

	return drv_api->connect(dev, &params, wifi_drv_event_connect,
				wifi_drv_event_disconnect);
}

int wifi_drv_get_station(void *iface, char *rssi)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->get_station)
		return -EIO;

	if (!rssi)
		return -EINVAL;

	return drv_api->get_station(dev, rssi);
}

int wifi_drv_notify_ip(void *iface, char *ipaddr, char len)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->notify_ip)
		return -EIO;

	if (!ipaddr)
		return -EINVAL;

	return drv_api->notify_ip(dev, ipaddr, len);
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static void wifi_drv_event_new_station(void *iface, int status, char *mac)
{
	struct wifi_drv_new_station_evt new_sta;

	new_sta.is_connect = status;
	if (mac && !is_zero_ether_addr(mac))
		memcpy(new_sta.mac, mac, NET_LINK_ADDR_MAX_LENGTH);

	wifimgr_notify_event(WIFIMGR_EVT_NEW_STATION, &new_sta,
			     sizeof(new_sta));
}

int wifi_drv_start_ap(void *iface, char *ssid, char *psk, unsigned char psk_len,
		      unsigned char channel, unsigned char ch_width)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;
	struct wifi_drv_start_ap_params params;

	if (!drv_api->start_ap)
		return -EIO;

	/* SSID is mandatory */
	params.ssid_len = strlen(ssid);
	if (!ssid || !params.ssid_len)
		return -EINVAL;
	params.ssid = ssid;

	/* Passphrase is only valid for WPA/WPA2-PSK */
	params.psk_len = psk_len;
	if (psk && !params.psk_len)
		return -EINVAL;
	params.psk = psk;

	/* Channel & channel width are optional */
	params.channel = channel;
	params.ch_width = ch_width;

	return drv_api->start_ap(dev, &params, wifi_drv_event_new_station);
}

int wifi_drv_stop_ap(void *iface)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->stop_ap)
		return -EIO;

	return drv_api->stop_ap(dev);
}

int wifi_drv_del_station(void *iface, char *mac)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->del_station)
		return -EIO;

	if (!mac)
		return -EINVAL;

	return drv_api->del_station(dev, mac);
}

int wifi_drv_set_mac_acl(void *iface, char subcmd, unsigned char acl_nr,
			 char acl_mac_addrs[][NET_LINK_ADDR_MAX_LENGTH])
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->set_mac_acl)
		return -EIO;

	if (!acl_mac_addrs)
		return -EINVAL;

	if (!acl_nr)
		return 0;

	return drv_api->set_mac_acl(dev, subcmd, acl_nr, acl_mac_addrs);
}
#endif

#endif
