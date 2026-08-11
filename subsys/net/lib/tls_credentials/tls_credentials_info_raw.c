/*
 * Copyright (c) 2026 Joakim Andersson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/sys/timeutil.h>

#include "tls_internal.h"
#include "tls_credentials_info_raw.h"

#if defined(CONFIG_MBEDTLS_X509_CRT_PARSE_C)
#include <mbedtls/x509_crt.h>
#endif

int credential_info_expiry_get(struct tls_credential *credential, time_t *expiry_out)
{
#if defined(CONFIG_MBEDTLS_X509_CRT_PARSE_C)
	mbedtls_x509_crt crt;
	int ret = 0, err;
	time_t expiry;
	struct tm tm_expiry;

	mbedtls_x509_crt_init(&crt);

	err = mbedtls_x509_crt_parse(&crt, credential->buf, credential->len);
	if (err != 0) {
		ret = -EINVAL;
		goto exit;
	}

	/* Convert mbedTLS time to time_t. */
	tm_expiry.tm_year = crt.valid_to.year - 1900;
	tm_expiry.tm_mon = crt.valid_to.mon - 1;
	tm_expiry.tm_mday = crt.valid_to.day;
	tm_expiry.tm_hour = crt.valid_to.hour;
	tm_expiry.tm_min = crt.valid_to.min;
	tm_expiry.tm_sec = crt.valid_to.sec;

	errno = 0;
	expiry = timeutil_timegm(&tm_expiry);
	/* Only if errno is set to non-zero is the conversion considered failed */
	if (expiry == -1 && errno != 0) {
		ret = -errno;
		goto exit;
	}

	*expiry_out = expiry;
exit:
	mbedtls_x509_crt_free(&crt);

	return ret;
#else
	return -ENOTSUP;
#endif /* defined(CONFIG_MBEDTLS_X509_CRT_PARSE_C) */
}
