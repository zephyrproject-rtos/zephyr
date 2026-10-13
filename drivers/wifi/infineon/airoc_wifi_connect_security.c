/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "airoc_wifi_connect_security.h"

whd_security_t convert_zephyr_security_to_whd(int security)
{
	whd_security_t whd_security = WHD_SECURITY_UNKNOWN;

	switch (security) {
	case WIFI_SECURITY_TYPE_NONE:
		whd_security = WHD_SECURITY_OPEN;
		break;

	case WIFI_SECURITY_TYPE_WEP:
		whd_security = WHD_SECURITY_WEP_PSK;
		break;

	case WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL:
		whd_security = WHD_SECURITY_WPA3_WPA2_PSK;
		break;

	case WIFI_SECURITY_TYPE_PSK:
		whd_security = WHD_SECURITY_WPA2_AES_PSK;
		break;

	case WIFI_SECURITY_TYPE_PSK_SHA256:
		whd_security = WHD_SECURITY_WPA2_AES_PSK_SHA256;
		break;

	case WIFI_SECURITY_TYPE_SAE:
		whd_security = WHD_SECURITY_WPA3_SAE;
		break;

	case WIFI_SECURITY_TYPE_WPA_PSK:
		whd_security = WHD_SECURITY_WPA_AES_PSK;
		break;

	default:
		break;
	}
	return whd_security;
}
