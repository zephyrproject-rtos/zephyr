/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_ESP_HOSTED_UTIL_H_
#define ZEPHYR_DRIVERS_WIFI_ESP_HOSTED_UTIL_H_

static inline void esp_hosted_mac_to_str(const uint8_t *mac_bytes, uint8_t *mac_str)
{
	static const char *hex = "0123456789ABCDEF";

	for (int i = 0; i < 6; i++) {
		*mac_str++ = hex[(mac_bytes[i] >> 4) & 0x0F];
		*mac_str++ = hex[mac_bytes[i] & 0x0F];
		if (i < 5) {
			*mac_str++ = ':';
		}
	}
}

static inline void esp_hosted_str_to_mac(const uint8_t *mac_str, uint8_t *mac_bytes)
{
	uint8_t byte = 0;

	for (int i = 0; i < ESP_HOSTED_MAC_STR_LEN; i++) {
		char c = mac_str[i];

		if (c >= '0' && c <= '9') {
			byte = (byte << 4) | (c - '0');
		} else if (c >= 'a' && c <= 'f') {
			byte = (byte << 4) | (c - 'a' + 10);
		} else if (c >= 'A' && c <= 'F') {
			byte = (byte << 4) | (c - 'A' + 10);
		}
		if (c == ':' || (i + 1) == ESP_HOSTED_MAC_STR_LEN) {
			*mac_bytes++ = byte;
			byte = 0;
		}
	}
}

static inline enum wifi_security_type esp_hosted_prot_to_sec(Ctrl_WifiSecProt sec_prot)
{
	switch (sec_prot) {
	case Ctrl_WifiSecProt_Open:
		return WIFI_SECURITY_TYPE_NONE;
	case Ctrl_WifiSecProt_WEP:
		return WIFI_SECURITY_TYPE_WEP;
	case Ctrl_WifiSecProt_WPA_PSK:
		return WIFI_SECURITY_TYPE_WPA_PSK;
	case Ctrl_WifiSecProt_WPA2_PSK:
		return WIFI_SECURITY_TYPE_PSK;
	case Ctrl_WifiSecProt_WPA2_ENTERPRISE:
		return WIFI_SECURITY_TYPE_EAP;
	case Ctrl_WifiSecProt_WPA3_PSK:
		return WIFI_SECURITY_TYPE_SAE;
	case Ctrl_WifiSecProt_WPA2_WPA3_PSK:
	case Ctrl_WifiSecProt_WPA_WPA2_PSK:
		return WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL;
	default:
		return WIFI_SECURITY_TYPE_UNKNOWN;
	}
}

static inline Ctrl_WifiSecProt esp_hosted_sec_to_prot(enum wifi_security_type sec_type)
{
	switch (sec_type) {
	case WIFI_SECURITY_TYPE_NONE:
		return Ctrl_WifiSecProt_Open;
	case WIFI_SECURITY_TYPE_WEP:
		return Ctrl_WifiSecProt_WEP;
	case WIFI_SECURITY_TYPE_WPA_PSK:
		return Ctrl_WifiSecProt_WPA_PSK;
	case WIFI_SECURITY_TYPE_PSK:
		return Ctrl_WifiSecProt_WPA2_PSK;
	case WIFI_SECURITY_TYPE_EAP:
		return Ctrl_WifiSecProt_WPA2_ENTERPRISE;
	case WIFI_SECURITY_TYPE_SAE:
		return Ctrl_WifiSecProt_WPA3_PSK;
	case WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL:
		return Ctrl_WifiSecProt_WPA2_WPA3_PSK;
	default:
		return -1;
	}
}

static inline int esp_hosted_ctrl_response(CtrlMsg *ctrl_msg)
{
	/* Response values are located at a different offsets. */
	switch (ctrl_msg->msg_id) {
	case CtrlMsgId_Resp_GetMacAddress:
		return ctrl_msg->resp_get_mac_address.resp;
	case CtrlMsgId_Resp_SetMacAddress:
		return ctrl_msg->resp_set_mac_address.resp;
	case CtrlMsgId_Resp_GetWifiMode:
		return ctrl_msg->resp_get_wifi_mode.resp;
	case CtrlMsgId_Resp_SetWifiMode:
		return ctrl_msg->resp_set_wifi_mode.resp;
	case CtrlMsgId_Resp_GetAPScanList:
		return ctrl_msg->resp_scan_ap_list.resp;
	case CtrlMsgId_Resp_GetAPConfig:
		return ctrl_msg->resp_get_ap_config.resp;
	case CtrlMsgId_Resp_ConnectAP:
		return ctrl_msg->resp_connect_ap.resp;
	case CtrlMsgId_Resp_DisconnectAP:
		return ctrl_msg->resp_disconnect_ap.resp;
	case CtrlMsgId_Resp_GetSoftAPConfig:
		return ctrl_msg->resp_get_softap_config.resp;
	case CtrlMsgId_Resp_SetSoftAPVendorSpecificIE:
		return ctrl_msg->resp_set_softap_vendor_specific_ie.resp;
	case CtrlMsgId_Resp_StartSoftAP:
		return ctrl_msg->resp_start_softap.resp;
	case CtrlMsgId_Resp_GetSoftAPConnectedSTAList:
		return ctrl_msg->resp_softap_connected_stas_list.resp;
	case CtrlMsgId_Resp_StopSoftAP:
		return ctrl_msg->resp_stop_softap.resp;
	case CtrlMsgId_Resp_SetPowerSaveMode:
		return ctrl_msg->resp_set_power_save_mode.resp;
	case CtrlMsgId_Resp_GetPowerSaveMode:
		return ctrl_msg->resp_get_power_save_mode.resp;
	case CtrlMsgId_Resp_OTABegin:
		return ctrl_msg->resp_ota_begin.resp;
	case CtrlMsgId_Resp_OTAWrite:
		return ctrl_msg->resp_ota_write.resp;
	case CtrlMsgId_Resp_OTAEnd:
		return ctrl_msg->resp_ota_end.resp;
	case CtrlMsgId_Resp_SetWifiMaxTxPower:
		return ctrl_msg->resp_set_wifi_max_tx_power.resp;
	case CtrlMsgId_Resp_GetWifiCurrTxPower:
		return ctrl_msg->resp_get_wifi_curr_tx_power.resp;
	case CtrlMsgId_Resp_ConfigHeartbeat:
		return ctrl_msg->resp_config_heartbeat.resp;
	default:
		return -1;
	}
}

#if CONFIG_WIFI_ESP_HOSTED_DEBUG
static void esp_hosted_frame_dump(esp_frame_t *frame)
{
	static const char *const if_strs[] = {"STA", "AP", "SERIAL", "HCI", "PRIV", "TEST"};

	if (frame->if_type > ESP_HOSTED_MAX_IF) {
		return;
	}
	LOG_DBG("esp header: if %s_IF length %d offset %d checksum %d seq %d flags %x",
		if_strs[frame->if_type], frame->len, frame->offset, frame->checksum, frame->seq_num,
		frame->flags);

	if (frame->if_type == ESP_HOSTED_SERIAL_IF) {
		LOG_DBG("tlv header: ep_type %d ep_length %d ep_value %.8s data_type %d "
			"data_length %d",
			frame->ep_type, frame->ep_length, frame->ep_value, frame->data_type,
			frame->data_length);
	}
}
#endif

static inline uint16_t esp_hosted_frame_checksum(esp_frame_t *frame)
{
	uint16_t checksum = 0;
	uint8_t *buf = (uint8_t *)frame;

	frame->checksum = 0;
	for (size_t i = 0; i < (frame->len + ESP_FRAME_HEADER_SIZE); i++) {
		checksum += buf[i];
	}

	return checksum;
}

#endif /* ZEPHYR_DRIVERS_WIFI_ESP_HOSTED_UTIL_H_ */
