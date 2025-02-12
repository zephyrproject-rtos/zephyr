/*
 * RFC 1521 base64 encoding/decoding
 *
 * Copyright (C) 2018, Nordic Semiconductor ASA
 * Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Adapted for Zephyr by Carles Cufi (carles.cufi@nordicsemi.no)
 *  - Removed mbedtls_ prefixes
 *  - Reworked coding style
 */

#include <string.h>
#include <stdbool.h>
#include <zephyr/sys/base64.h>
#include <zephyr/ztest.h>

#include "../../../lib/utils/base64.c"

static const unsigned char base64_test_dec[64] = {
	0x24, 0x48, 0x6E, 0x56, 0x87, 0x62, 0x5A, 0xBD,
	0xBF, 0x17, 0xD9, 0xA2, 0xC4, 0x17, 0x1A, 0x01,
	0x94, 0xED, 0x8F, 0x1E, 0x11, 0xB3, 0xD7, 0x09,
	0x0C, 0xB6, 0xE9, 0x10, 0x6F, 0x22, 0xEE, 0x13,
	0xCA, 0xB3, 0x07, 0x05, 0x76, 0xC9, 0xFA, 0x31,
	0x6C, 0x08, 0x34, 0xFF, 0x8D, 0xC2, 0x6C, 0x38,
	0x00, 0x43, 0xE9, 0x54, 0x97, 0xAF, 0x50, 0x4B,
	0xD1, 0x41, 0xBA, 0x95, 0x31, 0x5A, 0x0B, 0x97
};

static const unsigned char base64_test_enc[] =
	"JEhuVodiWr2/F9mixBcaAZTtjx4Rs9cJDLbpEG8i7hPK"
	"swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw==";

static const unsigned char base64_test_enc2[] =
	"Jkwo048//dw 0sf356efdaKFLowLKAfJdw410Lw3PdKl"
	"swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw==";

static const unsigned char base64_test_enc3[] =
	"PsdA1lf04JJ3nc00A30F8ker09i0ldkw36bv=SDW2\r\nv"
	"swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw==";

static const unsigned char base64_test_enc4[] =
	"===uVodiWr2/F9mixBcaAZTtjx4Rs9cJDLbpEG8i7hPK"
	"swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw==";

static const unsigned char base64_test_enc5[] =
	"JEhuVodiWr2/F9mixBcaAZTtjx4Rs9cJDLbpEG8i\n\r\ni"
	"swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw= ";

ZTEST(lib_base64, test_base64_codec)
{
	size_t len;
	int rc;
	const unsigned char *src;
	unsigned char buffer[128];
	int slen;
	int n;

	src = base64_test_dec;

	/* test base64_encode */
	rc = base64_encode(buffer, sizeof(buffer), &len, src, 64);
	zassert_equal(rc, 0, "Encode test return value");
	rc = memcmp(base64_test_enc, buffer, 88);
	zassert_equal(rc, 0, "Encode test comparison");

	src = base64_test_enc;

	/* test base64_decode */
	rc = base64_decode(buffer, sizeof(buffer), &len, src, 88);
	zassert_equal(rc, 0, "Decode test return value");
	rc = memcmp(base64_test_dec, buffer, 64);
	zassert_equal(rc, 0, "Decode test comparison");

	/* test error paths - encode */
	rc = base64_encode(buffer, sizeof(buffer), &len, src, 0);
	zassert_equal(rc, 0, "Error: slen: encode test return value");
	zassert_equal(len, 0, "Error: slen: length value");

	slen = ((-1 - 1) / 4) * 3 - 1;
	rc = base64_encode(buffer, sizeof(buffer), &len, src, slen);
	zassert_equal(rc, -ENOMEM, "Error: n: encode test return value");
	zassert_equal(len, -1, "Error: n: length value");

	slen = 100;
	n = slen / 3 + (slen % 3 != 0);
	n *= 4;
	rc = base64_encode(buffer, sizeof(buffer), &len, src, slen);
	zassert_equal(rc, -ENOMEM, "Error: dlen: encode test return value");
	zassert_equal(len, n + 1, "Error: dlen: length value");

	/* test error paths - decode */
	src = base64_test_enc2;
	rc = base64_decode(buffer, sizeof(buffer), &len, src, 88);
	zassert_equal(rc, -EINVAL, "Error: space: decode test return value");

	src = base64_test_enc3;
	rc = base64_decode(buffer, sizeof(buffer), &len, src, 88);
	zassert_equal(rc, -EINVAL, "Error: dec_map: decode test return value");

	src = base64_test_enc4;
	rc = base64_decode(buffer, sizeof(buffer), &len, src, 88);
	zassert_equal(rc, -EINVAL, "Error: equal: decode test return value");

	src = base64_test_enc5;
	rc = base64_decode(buffer, sizeof(buffer), &len, src, 88);
	zassert_equal(rc, 0, "return, newline: decode test return value");

	src = base64_test_enc;
	rc = base64_decode(buffer, sizeof(buffer), &len, src, 0);
	zassert_equal(rc, 0, "Error: n: decode test return value");
	zassert_equal(len, 0, "Error: n: length value");

	src = base64_test_enc;
	rc = base64_decode(NULL, -1, &len, src, 88);
	zassert_equal(rc, -ENOMEM, "Error: dst NULL: decode test return value");
}

ZTEST_SUITE(lib_base64, NULL, NULL, NULL, NULL, NULL);
