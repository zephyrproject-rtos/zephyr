/*
 * @file
 * @brief PSK calculation for IEEE 802.11i
 */

/*
 * Copyright (c) 2019, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#if defined(CONFIG_MBEDTLS)
#include "mbedtls/pkcs5.h"
#endif /* CONFIG_MBEDTLS */

#include "psk.h"

/**
 * pbkdf2_sha1 - SHA1-based key derivation function (PBKDF2) for IEEE 802.11i
 * @passphrase: ASCII passphrase
 * @ssid: SSID
 * @iterations: Number of iterations to run
 * @buf: Buffer for the generated key
 * @buflen: Length of the buffer in bytes
 * Returns: 0 on success, -1 of failure
 *
 * This function is used to derive PSK for WPA-PSK. For this protocol,
 * iterations is set to 4096 and buflen to 32. This function is described in
 * IEEE Std 802.11-2004, Clause H.4. The main construction is from PKCS#5 v2.0.
 */
int pbkdf2_sha1(const char *passphrase, const char *ssid, int iterations,
		char *buf, size_t buflen)
{
	mbedtls_md_context_t sha1_ctx;
	const mbedtls_md_info_t *info_sha1;
	int ret;

	mbedtls_md_init(&sha1_ctx);

	info_sha1 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
	if (!info_sha1) {
		ret = -1;
		goto exit;
	}

	ret = mbedtls_md_setup(&sha1_ctx, info_sha1, 1);
	if (ret) {
		ret = -1;
		goto exit;
	}

	ret = mbedtls_pkcs5_pbkdf2_hmac(&sha1_ctx, passphrase,
					strlen(passphrase), ssid, strlen(ssid),
					iterations, buflen, buf);
	if (ret) {
		ret = -1;
		goto exit;
	}

exit:
	mbedtls_md_free(&sha1_ctx);
	return ret;
}
