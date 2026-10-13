/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "airoc_wifi_security.h"

ZTEST_SUITE(airoc_wifi_security, NULL, NULL, NULL, NULL, NULL);

ZTEST(airoc_wifi_security, test_open_maps_to_whd_open)
{
	whd_security_t out = 0xFFFF;

	zassert_equal(0, airoc_wifi_security_to_whd(WIFI_SECURITY_TYPE_NONE, &out));
	zassert_equal(WHD_SECURITY_OPEN, out);
}

ZTEST(airoc_wifi_security, test_psk_maps_to_whd_wpa2_aes_psk)
{
	whd_security_t out = 0xFFFF;

	zassert_equal(0, airoc_wifi_security_to_whd(WIFI_SECURITY_TYPE_PSK, &out));
	zassert_equal(WHD_SECURITY_WPA2_AES_PSK, out);
}

ZTEST(airoc_wifi_security, test_sae_maps_to_whd_wpa3_sae)
{
	whd_security_t out = 0xFFFF;

	zassert_equal(0, airoc_wifi_security_to_whd(WIFI_SECURITY_TYPE_SAE, &out));
	zassert_equal(WHD_SECURITY_WPA3_SAE, out);
}

ZTEST(airoc_wifi_security, test_unsupported_security_returns_enotsup_not_success)
{
	whd_security_t out = 0xFFFF;

	zassert_equal(-ENOTSUP, airoc_wifi_security_to_whd(WIFI_SECURITY_TYPE_WEP, &out));
	zassert_equal(0xFFFF, out, "out_security must not be written on failure");
}

ZTEST(airoc_wifi_security, test_eap_security_returns_enotsup)
{
	whd_security_t out = 0xFFFF;

	zassert_equal(-ENOTSUP, airoc_wifi_security_to_whd(WIFI_SECURITY_TYPE_EAP_TLS, &out));
	zassert_equal(0xFFFF, out);
}
