/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
#define LOG_MODULE_NAME wifi_uwp
#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL
LOG_MODULE_DECLARE(LOG_MODULE_NAME);

#include <zephyr.h>
#include <string.h>
#include <sipc.h>
#include <sblock.h>

#include "cmdevt.h"
#include "txrx.h"

#define RECV_BUF_SIZE (128)
#define GET_STA_BUF_SIZE (12)
#define ALL_2_4_GHZ_CHANNELS (0x3FFF)

extern struct wifi_priv uwp_wifi_ap_priv;

static unsigned char recv_buf[RECV_BUF_SIZE];
static unsigned int recv_len;
static unsigned int seq = 1;
static struct k_sem cmd_sem;

static const u16_t CRC_table[] = {
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00,
	0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401,
	0x5000, 0x9C01, 0x8801, 0x4400,
};

/* 5G channel list */
static const u16_t channels_5g_scan_table[] = {
	36, 40, 44, 48, 52,
	56, 60, 64, 100, 104,
	108, 112, 116, 120, 124,
	128, 132, 136, 140, 144,
	149, 153, 157, 161, 165
};

static u16_t CRC16(const u8_t *buf, u16_t len)
{
	u16_t CRC = 0xFFFF;
	u16_t i;
	u8_t ch_char;

	for (i = 0; i < len; i++) {
		ch_char = *buf++;
		CRC = CRC_table[(ch_char ^ CRC) & 15] ^ (CRC >> 4);
		CRC = CRC_table[((ch_char >> 4) ^ CRC) & 15] ^ (CRC >> 4);
	}
	return CRC;
}

static inline int check_cmdevt_len(int input_len, int expected_len)
{
	if (input_len != expected_len) {
		if (input_len > expected_len) {
			LOG_DBG("Invalid len %d, expected len %d",
					input_len, expected_len);
		} else {
			LOG_ERR("Invalid len %d, expected len %d",
					input_len, expected_len);
			return -EINVAL;
		}
	}

	return 0;
}

int wifi_cmd_load_ini(const u8_t *data, u32_t len, u8_t sec_num)
{
	int ret;
	u16_t crc = 0;
	int cmd_len;
	struct cmd_download_ini *cmd;

	/* Calculate total command length. */
	cmd_len = sizeof(*cmd) + len + sizeof(crc);

	cmd = k_malloc(cmd_len);
	if (!cmd) {
		LOG_ERR("cmd is null");
		return -ENOMEM;
	}

	/* Calc CRC value. */
	crc = CRC16(data, len);
	LOG_DBG("Sec: %d, len: %d, CRC value: 0x%x",
		    sec_num, len, crc);

	memset(cmd, 0, cmd_len);
	cmd->sec_num = sec_num;

	/* Copy data after section num. */
	memcpy(cmd->data, data, len);
	/* Put CRC value at the tail of INI data. */
	memcpy(cmd->data + len, &crc, sizeof(crc));

	ret = wifi_cmd_send(WIFI_CMD_DOWNLOAD_INI, (char *)cmd,
			    cmd_len, NULL, NULL);
	if (ret) {
		LOG_ERR("Load ini send cmd fail");
		k_free(cmd);
		return ret;
	}

	k_free(cmd);

	return 0;
}

int wifi_cmd_scan(struct wifi_device *wifi_dev,
		struct wifi_drv_scan_params *params)
{
	ARG_UNUSED(wifi_dev);

	int ret;
	int ssid_len = 0;
	int cmd_len;
	struct cmd_scan *cmd;
	u16_t channels_5g_cnt = 0;
	u8_t band = params->band;
	u8_t channel = params->channel;

	cmd_len = sizeof(*cmd) + ssid_len;

	switch (band) {
	case WIFI_BAND_2_4G:
		cmd = k_malloc(cmd_len);
		if (!cmd) {
			LOG_ERR("cmd is null");
			return -ENOMEM;
		}

		if (channel == 0) { /* All 2.4GHz channel */
			cmd->channels_2g = ALL_2_4_GHZ_CHANNELS;
		} else { /* One channel */
			cmd->channels_2g = BIT(channel - 1);
		}
		break;
	case WIFI_BAND_5G:
		if (channel == 0) { /* All 5GHz channel */
			cmd_len +=  sizeof(channels_5g_scan_table);

			cmd = k_malloc(cmd_len);
			if (!cmd) {
				LOG_ERR("cmd is null");
				return -ENOMEM;
			}
			memset(cmd, 0, cmd_len);

			channels_5g_cnt = ARRAY_SIZE(channels_5g_scan_table);
			memcpy(cmd->channels_5g, channels_5g_scan_table,
					sizeof(channels_5g_scan_table));
		} else { /* One channel */
			cmd_len += 1;

			cmd = k_malloc(cmd_len);
			if (!cmd) {
				LOG_ERR("cmd is null");
				return -ENOMEM;
			}
			memset(cmd, 0, cmd_len);

			channels_5g_cnt = 1;
			*(cmd->channels_5g) = channel;
		}

		cmd->channels_5g_cnt = channels_5g_cnt;
		break;
	default:
		if (channel == 0) { /* All channels */
			cmd_len += sizeof(channels_5g_scan_table);

			cmd = k_malloc(cmd_len);
			if (!cmd) {
				LOG_ERR("cmd is null");
				return -ENOMEM;
			}
			memset(cmd, 0, cmd_len);

			channels_5g_cnt = ARRAY_SIZE(channels_5g_scan_table);
			cmd->channels_5g_cnt = channels_5g_cnt;
			memcpy(cmd->channels_5g, channels_5g_scan_table,
					sizeof(channels_5g_scan_table));

			cmd->channels_2g = ALL_2_4_GHZ_CHANNELS;
		} else {
			LOG_ERR("Invalid band %d, channel %d.",
					band, channel);
			return -EINVAL;
		}
		break;
	}

	ret = wifi_cmd_send(WIFI_CMD_SCAN, (char *)cmd,
			    cmd_len, NULL, NULL);
	if (ret) {
		LOG_ERR("Scan send cmd fail");
		k_free(cmd);
		return ret;
	}

	k_free(cmd);

	return 0;
}

int wifi_cmd_connect(struct wifi_device *wifi_dev,
		     struct wifi_drv_connect_params *params)
{
	ARG_UNUSED(wifi_dev);

	int ret;
	struct cmd_connect cmd;

	memset(&cmd, 0, sizeof(cmd));

	if (!params->ssid) {
		LOG_ERR("Invalid SSID.");
		return -EINVAL;
	}

	if (params->bssid) {
		memcpy(cmd.bssid, params->bssid, ETH_ALEN);
	}

	cmd.channel = params->channel;
	cmd.ssid_len = params->ssid_len;
	cmd.psk_len = params->psk_len;

	memcpy(cmd.ssid, params->ssid, cmd.ssid_len);
	memcpy(cmd.psk, params->psk, cmd.psk_len);

	ret = wifi_cmd_send(WIFI_CMD_CONNECT, (char *)&cmd,
			    sizeof(cmd), NULL, NULL);
	if (ret) {
		LOG_ERR("Connect send cmd fail");
		return ret;
	}

	return 0;
}

int wifi_cmd_disconnect(struct wifi_device *wifi_dev)
{
	ARG_UNUSED(wifi_dev);

	int ret;
	struct cmd_disconnect cmd;

	cmd.reason_code = 0;

	ret = wifi_cmd_send(WIFI_CMD_DISCONNECT, (char *)&cmd,
			    sizeof(cmd), NULL, NULL);
	if (ret) {
		LOG_ERR("Disconnect send cmd fail");
		return ret;
	}

	return 0;
}

int wifi_cmd_get_cp_info(struct wifi_priv *priv)
{
	struct cmd_get_cp_info cmd;
	int ret;
	int rlen;

	memset(&cmd, 0, sizeof(cmd));

	ret = wifi_cmd_send(WIFI_CMD_GET_CP_INFO, (char *)&cmd,
			sizeof(struct trans_hdr), (char *)&cmd, &rlen);
	if (ret) {
		LOG_ERR("Get cp info send cmd fail");
		return ret;
	}

	if (check_cmdevt_len(rlen, sizeof(cmd))) {
		return -EINVAL;
	}

	priv->cp_version = cmd.version;

	/* Store sta mac. */
	memcpy(priv->wifi_dev[WIFI_DEV_STA].mac, cmd.mac, ETH_ALEN);
	/* Store softap mac. */
	cmd.mac[4] ^= 0x80;
	memcpy(priv->wifi_dev[WIFI_DEV_AP].mac, cmd.mac, ETH_ALEN);

	/* Store maximum station on softap. */
	priv->wifi_dev[WIFI_DEV_AP].max_sta_num = cmd.max_ap_assoc_sta_num;

	/* Store maximum stations in blacklist on softap. */
	priv->wifi_dev[WIFI_DEV_AP].max_blacklist_num =
		cmd.max_ap_blacklist_sta_num;

	priv->wifi_dev[WIFI_DEV_STA].max_rtt_num = cmd.max_rtt_num;

	LOG_INF("CP version: 0x%x\n", priv->cp_version);

	LOG_DBG("Max sta num: %d\n", cmd.max_ap_assoc_sta_num);
	LOG_DBG("Max blacklist num: %d\n", cmd.max_ap_blacklist_sta_num);
	LOG_DBG("Max rtt num: %d\n", cmd.max_rtt_num);

	return 0;
}

int wifi_cmd_open(struct wifi_device *wifi_dev)
{
	struct cmd_open cmd;
	int ret;

	LOG_DBG("Open mode %d", wifi_dev->mode);

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = wifi_dev->mode;
	memcpy(cmd.mac, wifi_dev->mac, ETH_ALEN);

	ret = wifi_cmd_send(WIFI_CMD_OPEN, (char *)&cmd, sizeof(cmd),
			    NULL, NULL);
	if (ret) {
		LOG_ERR("Open mode %d send cmd fail", wifi_dev->mode);
		return ret;
	}
	LOG_DBG("Open mode success.");

	return 0;
}

int wifi_cmd_close(struct wifi_device *wifi_dev)
{
	struct cmd_stop cmd;
	int ret;

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = wifi_dev->mode;
	memcpy(cmd.mac, wifi_dev->mac, 6);

	ret = wifi_cmd_send(WIFI_CMD_CLOSE, (char *)&cmd, sizeof(cmd),
			    NULL, NULL);
	if (ret) {
		LOG_ERR("Stop mode:%d send cmd fail", wifi_dev->mode);
		return ret;
	}

	return 0;
}

int wifi_cmd_get_sta(struct wifi_device *wifi_dev,
		s8_t *signal)
{
	ARG_UNUSED(wifi_dev);

	struct cmd_get_sta cmd;
	int ret;
	int rlen;

	memset(&cmd, 0, sizeof(cmd));

	ret = wifi_cmd_send(WIFI_CMD_GET_STATION, (char *)&cmd,
			sizeof(struct trans_hdr), (char *)&cmd, &rlen);
	if (ret) {
		LOG_ERR("Get sta send cmd fail %d", ret);
		return ret;
	}

	if (check_cmdevt_len(rlen, sizeof(cmd))) {
		return -EINVAL;
	}

	*signal = cmd.signal;

	LOG_DBG("signal %d", *signal);

	return 0;
}

static inline u8_t wifi_chan_bw_map(u8_t chan_bw)
{
	u8_t vht_chwidth;

	switch (chan_bw) {
	case 0:
		vht_chwidth = VHT_CHAN_BW_DEF;
		break;
	case 20:
		vht_chwidth = VHT_CHAN_BW_20M;
		break;
	case 40:
		vht_chwidth = VHT_CHAN_BW_40M;
		break;
	case 80:
		vht_chwidth = VHT_CHAN_BW_80M;
		break;
	case 160:
		vht_chwidth = VHT_CHAN_BW_160M;
		break;
	default:
		vht_chwidth = VHT_CHAN_BW_DEF;
		break;
	}

	return vht_chwidth;
}

int wifi_cmd_start_ap(struct wifi_device *wifi_dev,
		struct wifi_drv_start_ap_params *params)
{
	ARG_UNUSED(wifi_dev);

	struct cmd_start_ap cmd;
	int ret;

	LOG_DBG("Start ap at channel: %d.", params->channel);
	memset(&cmd, 0, sizeof(cmd));

	if (params->ssid_len > 0) {
		memcpy(cmd.ssid, params->ssid, params->ssid_len);
		cmd.ssid_len = params->ssid_len;
		LOG_DBG("SSID: %s(%d).", cmd.ssid, cmd.ssid_len);
	}
	if (params->psk_len > 0) {
		memcpy(cmd.password, params->psk, params->psk_len);
		cmd.password_len = params->psk_len;
		LOG_DBG("PSK: %s(%d).", cmd.password, cmd.password_len);
	}

	cmd.channel = params->channel;
	cmd.vht_chwidth = wifi_chan_bw_map(params->ch_width);

	ret = wifi_cmd_send(WIFI_CMD_START_AP, (char *)&cmd,
			    sizeof(cmd), NULL, NULL);
	if (ret) {
		LOG_ERR("AP start send cmd fail %d", ret);
		return ret;
	}

	LOG_DBG("Start ap ok.");

	return 0;
}

int wifi_cmd_del_sta(struct wifi_device *wifi_dev,
		u8_t *mac, u16_t reason_code)
{
	ARG_UNUSED(wifi_dev);

	struct cmd_del_sta cmd;
	int ret;

	memset(&cmd, 0, sizeof(cmd));
	memcpy(cmd.mac, mac, ETH_ALEN);
	cmd.reason_code = reason_code;

	ret = wifi_cmd_send(WIFI_CMD_DEL_STATION, (char *)&cmd,
			    sizeof(cmd), NULL, NULL);
	if (ret) {
		LOG_ERR("Delete station send cmd fail %d", ret);
		return ret;
	}

	LOG_DBG("Delete station done.");

	return 0;
}

int wifi_cmd_set_blacklist(struct wifi_device *wifi_dev,
		u8_t sub_type, u8_t mac_num, u8_t **mac_addr)
{
	struct cmd_set_blacklist *cmd;
	int ret;
	int cmd_len;
	int max_blacklist_num = wifi_dev->max_blacklist_num;

	if (mac_num > max_blacklist_num) {
		LOG_WRN("Over maximum %d", max_blacklist_num);
		return -EINVAL;
	}

	cmd_len = sizeof(*cmd) + ETH_ALEN * mac_num;

	cmd = k_malloc(cmd_len);
	if (!cmd) {
		LOG_ERR("cmd is null");
		return -ENOMEM;
	}

	memset(cmd, 0, cmd_len);
	cmd->sub_type = sub_type;
	cmd->mac_num = mac_num;
	memcpy(cmd->mac_addr, mac_addr,
			ETH_ALEN * mac_num);

	ret = wifi_cmd_send(WIFI_CMD_SET_BLACKLIST, (char *)cmd,
			    cmd_len, NULL, NULL);
	if (ret) {
		LOG_ERR("Set blacklist send cmd fail %d", ret);
		k_free(cmd);
		return ret;
	}

	k_free(cmd);

	return 0;
}

int wifi_cmd_stop_ap(struct wifi_device *wifi_dev)
{
	ARG_UNUSED(wifi_dev);

	/* Not supported by CP. */
	return 0;
}


/**
 * @param r_buf: address of return value
 * @param r_len: length of return value
 */
int wifi_cmd_hw_test(struct wifi_device *wifi_dev,
		char *t_buf, u32_t t_len,
		char *r_buf, u32_t *r_len)
{
	ARG_UNUSED(wifi_dev);

	struct cmd_npi *cmd;
	int ret;
	int cmd_len;

	/* Calculate total command length. */
	cmd_len = sizeof(*cmd) + t_len;

	cmd = k_malloc(cmd_len);
	if (!cmd) {
		LOG_ERR("cmd is null");
		return -ENOMEM;
	}

	memset(cmd, 0, cmd_len);

	memcpy(cmd->data, t_buf, t_len);

	ret = wifi_cmd_send(WIFI_CMD_NPI_MSG, (char *)cmd,
			cmd_len, r_buf, r_len);
	if (ret) {
		LOG_ERR("Npi send cmd fail %d", ret);
		k_free(cmd);
		return ret;
	}

	if (r_buf && r_len) {
		*r_len = *r_len - sizeof(*cmd);
		/* Drop trans_hdr. */
		memcpy(r_buf, r_buf + sizeof(*cmd), *r_len);
	}

	k_free(cmd);

	return 0;
}

int wifi_cmd_notify_ip_acquired(struct wifi_device *wifi_dev,
		u8_t *ip_addr, u8_t len)
{
	int ret;
	struct cmd_set_ip cmd;

	memset(&cmd, 0, sizeof(cmd));
	if (len == IPV4_LEN) {
		/* IPv4 only */
		memcpy(cmd.ip, ip_addr, len);
		/* Store ipv4 address in wifi device. */
		memcpy(wifi_dev->ipv4_addr, ip_addr, len);
	} else {
		LOG_WRN("Currently ipv4 only, 4 bytes.");
		return -EINVAL;
	}

	ret = wifi_cmd_send(WIFI_CMD_NOTIFY_IP_ACQUIRED, (char *)&cmd,
			    sizeof(cmd), NULL, NULL);
	if (ret) {
		LOG_ERR("Set ip send cmd fail");
		return ret;
	}

	LOG_DBG("Set ip ok.");

	return 0;
}

int wifi_cmd_session_request(struct wifi_device *wifi_dev,
		struct wifi_drv_rtt_request *params)
{
	ARG_UNUSED(wifi_dev);

	int ret;
	int cmd_len;
	struct cmd_session_request *cmd;
	u8_t n_peers;

	n_peers = params->nr_peers;
	cmd_len = sizeof(*cmd) + n_peers * sizeof(struct rtt_peer_info);

	cmd = k_malloc(cmd_len);
	if (!cmd) {
		LOG_ERR("cmd is null");
		return -ENOMEM;
	}

	memset(cmd, 0, cmd_len);
	cmd->sub_type = RTT_RANGE_REQUEST;
	cmd->len = sizeof(cmd->n_peers)+
		n_peers * sizeof(struct rtt_peer_info);
	cmd->n_peers = n_peers;

	for (int i = 0; i < n_peers; i++) {
		memcpy(cmd->peers[i].mac, params->peers[i].bssid, ETH_ALEN);
		cmd->peers[i].chan_info.chan_num = params->peers[i].channel;
	}

	ret = wifi_cmd_send(WIFI_CMD_RTT, (char *)cmd,
			    cmd_len, NULL, NULL);
	if (ret) {
		LOG_ERR("Session request send cmd fail\n");
		k_free(cmd);
		return ret;
	}

	k_free(cmd);

	return 0;
}

static int wifi_evt_scan_result(struct wifi_device *wifi_dev,
		char *data, int len)
{
	struct event_scan_result *event =
		(struct event_scan_result *)data;
	struct wifi_drv_scan_result_evt scan_result;

	if (check_cmdevt_len(len, sizeof(struct event_scan_result))) {
		return -EINVAL;
	}

	memset(&scan_result, 0, sizeof(scan_result));

	memcpy(scan_result.ssid, event->ssid, MAX_SSID_LEN);
	scan_result.ssid_len = strlen(scan_result.ssid);
	memcpy(scan_result.bssid, event->bssid, ETH_ALEN);

	scan_result.channel = event->channel;
	scan_result.rssi = event->rssi;
	scan_result.security = event->encrypt_mode;
	scan_result.rtt_supported = event->extra & BIT(0);

	LOG_DBG("ssid: %s", event->ssid);

	if (wifi_dev->scan_result_cb) {
		wifi_dev->scan_result_cb(wifi_dev->iface, 0, &scan_result);
	} else {
		LOG_DBG("No scan_result callback.");
	}

	k_yield();

	return 0;
}

static int wifi_evt_scan_done(struct wifi_device *wifi_dev, char *data, int len)
{
	struct event_scan_done *event =
		(struct event_scan_done *)data;

	if (check_cmdevt_len(len, sizeof(struct event_scan_done))) {
		return -EINVAL;
	}

	if (wifi_dev->scan_result_cb) {
		wifi_dev->scan_result_cb(wifi_dev->iface, event->status, NULL);
		wifi_dev->scan_result_cb = NULL;
	} else {
		LOG_WRN("No scan_result callback.");
	}

	return 0;
}

static int wifi_evt_connect(struct wifi_device *wifi_dev, char *data, int len)
{
	struct event_connect *event =
		(struct event_connect *)data;

	if (check_cmdevt_len(len, sizeof(struct event_connect))) {
		return -EINVAL;
	}

	if (wifi_dev->connect_cb) {
		wifi_dev->connect_cb(wifi_dev->iface, event->status,
				event->bssid, event->primary_chan_num);
	} else {
		LOG_WRN("No connect callback.");
	}

	if (!event->status) {
		wifi_dev->connected = true;
	}

	return 0;
}

static int wifi_evt_disconnect(struct wifi_device *wifi_dev,
		char *data, int len)
{
	struct event_disconnect *event =
		(struct event_disconnect *)data;

	if (check_cmdevt_len(len, sizeof(struct event_disconnect))) {
		return -EINVAL;
	}

	if (wifi_dev->disconnect_cb) {
		wifi_dev->disconnect_cb(wifi_dev->iface, event->reason_code);
	} else {
		LOG_WRN("No disconnect callback.");
	}

	wifi_dev->connected = false;

	return 0;
}

static int wifi_evt_new_sta(struct wifi_device *wifi_dev, char *data, int len)
{
	struct event_new_station *event =
		(struct event_new_station *)data;

	if (check_cmdevt_len(len, sizeof(struct event_new_station))) {
		return -EINVAL;
	}

	if (wifi_dev->new_station_cb) {
		wifi_dev->new_station_cb(wifi_dev->iface,
				event->is_connect,
				event->mac);
	} else {
		LOG_WRN("No new_station callback.");
	}

	return 0;
}

static int wifi_evt_rtt(struct wifi_device *wifi_dev, char *data, int len)
{
	struct event_rtt_per_dest_meter *sub_evt = NULL;
	struct event_rtt *event =
		(struct event_rtt *)data;
	struct wifi_drv_rtt_response_evt rtt_res;

	switch (event->sub_type) {
	case RTT_SESSION_END:
		if (check_cmdevt_len(len, sizeof(struct event_rtt))) {
			return -EINVAL;
		}
		if (wifi_dev->rtt_result_cb) {
			wifi_dev->rtt_result_cb(wifi_dev->iface, 0, NULL);
		}
		break;
	case RTT_PER_DEST_METER:
		if (check_cmdevt_len(len,
				sizeof(struct event_rtt_per_dest_meter))) {
			return -EINVAL;
		}
		sub_evt = (struct event_rtt_per_dest_meter *)data;

		LOG_DBG("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
				sub_evt->mac[0], sub_evt->mac[1],
				sub_evt->mac[2], sub_evt->mac[3],
				sub_evt->mac[4], sub_evt->mac[5]);
		LOG_DBG("meter: %d\n", sub_evt->meter);

		memcpy(rtt_res.bssid, sub_evt->mac, ETH_ALEN);
		rtt_res.range = sub_evt->meter;
		if (wifi_dev->rtt_result_cb) {
			wifi_dev->rtt_result_cb(wifi_dev->iface, 0, &rtt_res);
		}
		break;
	default:
		LOG_WRN("Not supported sub_evt: 0x%02x", event->sub_type);
		break;
	}

	return 0;
}

int wifi_cmdevt_process(struct wifi_priv *priv, char *data, int len)
{
	struct trans_hdr *hdr = (struct trans_hdr *)data;

	if (len > RECV_BUF_SIZE) {
		LOG_ERR("Invalid data len %d.", len);
		return -EINVAL;
	}

	/* Receive command response. */
	if (hdr->response == 1) {
		if (len > 0) {
			memcpy(recv_buf, data, len);
			recv_len = len;
		}

		k_sem_give(&cmd_sem);

		/**
		 * Release command wait semaphore, and switch current thread to
		 * command process thread. This routine could prevent the send
		 * command timeout if there are many data recived from CP.
		 */
		k_yield();

		return 0;
	}

	LOG_DBG("Receive event type 0x%x.", hdr->type);

	len -= sizeof(*hdr);

	/* Receive Events */
	switch (hdr->type) {
	case WIFI_EVENT_SCAN_RESULT:
		wifi_evt_scan_result(&priv->wifi_dev[WIFI_DEV_STA],
				hdr->data, len);
		break;
	case WIFI_EVENT_SCAN_DONE:
		wifi_evt_scan_done(&priv->wifi_dev[WIFI_DEV_STA],
				hdr->data, len);
		break;
	case WIFI_EVENT_DISCONNECT:
		wifi_evt_disconnect(&priv->wifi_dev[WIFI_DEV_STA],
				hdr->data, len);
		break;
	case WIFI_EVENT_CONNECT:
		wifi_evt_connect(&priv->wifi_dev[WIFI_DEV_STA],
				hdr->data, len);
		break;
	case WIFI_EVENT_RTT:
		wifi_evt_rtt(&priv->wifi_dev[WIFI_DEV_STA],
				hdr->data, len);
		break;
	case WIFI_EVENT_NEW_STATION:
		wifi_evt_new_sta(&priv->wifi_dev[WIFI_DEV_AP],
				hdr->data, len);
		break;
	default:
		break;
	}
	return 0;
}

int wifi_cmd_send(u8_t cmd, char *data, int len, char *rbuf, int *rlen)
{
	int ret = 0;

	struct trans_hdr *hdr = (struct trans_hdr *)data;

	if (cmd > WIFI_CMD_MAX || cmd < WIFI_CMD_BEGIN) {
		LOG_ERR("Invalid command %d ", cmd);
		return -EINVAL;
	}

	if (!data || len == 0) {
		LOG_ERR("Invalid data or len.");
		return -EINVAL;
	}

	hdr->len = len;
	hdr->type = cmd;
	hdr->seq = seq++;

	ret = wifi_tx_cmd(data, len);
	if (ret < 0) {
		LOG_ERR("tx cmd fail %d\n", ret);
		return ret;
	}

	ret = k_sem_take(&cmd_sem, 3000);
	if (ret) {
		LOG_ERR("Wait cmd(%d) timeout.\n", cmd);
		return ret;
	}

	hdr = (struct trans_hdr *)recv_buf;
	if (hdr->status != 0) {
		LOG_ERR("Invalid cmd status: %i\n", hdr->status);
		return hdr->status;
	}

	if (rbuf) {
		memcpy(rbuf, recv_buf, recv_len);
	}
	if (rlen) {
		*rlen = recv_len;
	}

	LOG_DBG("Get command response success");
	return 0;
}


int wifi_cmdevt_init(void)
{
	k_sem_init(&cmd_sem, 0, 1);
	return 0;
}
