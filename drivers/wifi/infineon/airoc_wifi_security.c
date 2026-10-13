/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "airoc_wifi_security.h"

int airoc_wifi_security_to_whd(enum wifi_security_type security, whd_security_t *out_security)
{
	switch (security) {
	case WIFI_SECURITY_TYPE_NONE:
		*out_security = WHD_SECURITY_OPEN;
		return 0;
	case WIFI_SECURITY_TYPE_PSK:
		*out_security = WHD_SECURITY_WPA2_AES_PSK;
		return 0;
	case WIFI_SECURITY_TYPE_SAE:
		*out_security = WHD_SECURITY_WPA3_SAE;
		return 0;
	default:
		return -ENOTSUP;
	}
}
