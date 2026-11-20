/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "airoc_wifi_connect_security.h"

ZTEST_SUITE(airoc_wifi_connect_security, NULL, NULL, NULL, NULL, NULL);

ZTEST(airoc_wifi_connect_security, test_none_maps_to_whd_open)
{
	zassert_equal(WHD_SECURITY_OPEN,
		      airoc_convert_zephyr_security_to_whd(WIFI_SECURITY_TYPE_NONE));
}

ZTEST(airoc_wifi_connect_security, test_wep_maps_to_whd_wep_psk)
{
	zassert_equal(WHD_SECURITY_WEP_PSK,
		      airoc_convert_zephyr_security_to_whd(WIFI_SECURITY_TYPE_WEP));
}

ZTEST(airoc_wifi_connect_security, test_wpa_auto_personal_maps_to_whd_mixed)
{
	zassert_equal(WHD_SECURITY_WPA3_WPA2_PSK,
		      airoc_convert_zephyr_security_to_whd(WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL));
}

ZTEST(airoc_wifi_connect_security, test_psk_maps_to_whd_wpa2_aes_psk)
{
	zassert_equal(WHD_SECURITY_WPA2_AES_PSK,
		      airoc_convert_zephyr_security_to_whd(WIFI_SECURITY_TYPE_PSK));
}

ZTEST(airoc_wifi_connect_security, test_psk_sha256_maps_to_whd_wpa2_aes_psk_sha256)
{
	zassert_equal(WHD_SECURITY_WPA2_AES_PSK_SHA256,
		      airoc_convert_zephyr_security_to_whd(WIFI_SECURITY_TYPE_PSK_SHA256));
}

ZTEST(airoc_wifi_connect_security, test_sae_maps_to_whd_wpa3_sae)
{
	zassert_equal(WHD_SECURITY_WPA3_SAE,
		      airoc_convert_zephyr_security_to_whd(WIFI_SECURITY_TYPE_SAE));
}

ZTEST(airoc_wifi_connect_security, test_wpa_psk_maps_to_whd_wpa_aes_psk)
{
	zassert_equal(WHD_SECURITY_WPA_AES_PSK,
		      airoc_convert_zephyr_security_to_whd(WIFI_SECURITY_TYPE_WPA_PSK));
}

ZTEST(airoc_wifi_connect_security, test_unsupported_security_returns_whd_unknown)
{
	zassert_equal(WHD_SECURITY_UNKNOWN,
		      airoc_convert_zephyr_security_to_whd(WIFI_SECURITY_TYPE_EAP_TLS));
}

ZTEST(airoc_wifi_connect_security, test_ap_supports_open_psk_sae)
{
	zassert_true(airoc_wifi_security_supported_for_ap(WHD_SECURITY_OPEN));
	zassert_true(airoc_wifi_security_supported_for_ap(WHD_SECURITY_WPA2_AES_PSK));
	zassert_true(airoc_wifi_security_supported_for_ap(WHD_SECURITY_WPA3_SAE));
}

ZTEST(airoc_wifi_connect_security, test_ap_rejects_sta_only_security_types)
{
	zassert_false(airoc_wifi_security_supported_for_ap(WHD_SECURITY_WEP_PSK));
	zassert_false(airoc_wifi_security_supported_for_ap(WHD_SECURITY_WPA3_WPA2_PSK));
	zassert_false(airoc_wifi_security_supported_for_ap(WHD_SECURITY_WPA2_AES_PSK_SHA256));
	zassert_false(airoc_wifi_security_supported_for_ap(WHD_SECURITY_WPA_AES_PSK));
}

ZTEST(airoc_wifi_connect_security, test_ap_rejects_unknown_security)
{
	zassert_false(airoc_wifi_security_supported_for_ap(WHD_SECURITY_UNKNOWN));
}
