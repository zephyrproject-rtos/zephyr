/*
 * RFC 7519 Json Web Tokens
 *
 * Copyright (C) 2018, Linaro, Ltd
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
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <ztest.h>
#include <zephyr/data/json.h>
#include <zephyr/data/jwt.h>

#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/sha256.h>

extern unsigned char jwt_test_private_der[];
extern unsigned int jwt_test_private_der_len;

ZTEST(jwt_tests, test_jwt)
{
	/*
	 * TODO: This length should be computable, based on the length
	 * of the audience string.
	 */
	char buf[460];
	struct jwt_builder build;
	int res;

	res = jwt_init_builder(&build, buf, sizeof(buf));

	zassert_equal(res, 0, "Setting up jwt");

	res = jwt_add_payload(&build, 1530312026, 1530308426,
			      "iot-work-199419");
	zassert_equal(res, 0, "Adding payload");

	res = jwt_sign(&build, jwt_test_private_der, jwt_test_private_der_len);
	zassert_equal(res, 0, "Signing payload");

	zassert_equal(build.overflowed, false, "Not overflow");

	printk("JWT:\n%s\n", buf);
	printk("len: %zd\n", jwt_payload_len(&build));
}

ZTEST_SUITE(jwt_tests, NULL, NULL, NULL, NULL, NULL);
