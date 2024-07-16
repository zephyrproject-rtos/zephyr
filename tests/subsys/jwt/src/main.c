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
#include <zephyr/ztest.h>
#include <zephyr/data/json.h>
#include <zephyr/data/jwt.h>

extern unsigned char jwt_test_private_der[];
extern unsigned int jwt_test_private_der_len;

ZTEST(jwt_tests, test_jwt)
{
	const int32_t expt = 1530312026;
	const int32_t iat = 1530308426;
	static const char aud[] = "iot-work-199419";
	/*
	 * TODO: This length should be computable, based on the length
	 * of the audience string.
	 */
	char buf[460];
	struct jwt_builder build;
	char jwt[460];
	struct jwt_parser parse;
	int32_t parsed_expt = 0;
	int32_t parsed_iat = 0;
	char parsed_aud[32] = {0};
	int res;

	res = jwt_init_builder(&build, buf, sizeof(buf));
	zassert_equal(res, 0, "Setting up jwt");

	res = jwt_add_payload(&build, expt, iat, aud);
	zassert_equal(res, 0, "Adding payload");

	res = jwt_sign(&build, jwt_test_private_der, jwt_test_private_der_len);
	zassert_equal(res, 0, "Signing payload");
	zassert_equal(build.overflowed, false, "Not overflow");

	printk("JWT:\n%s\n", buf);
	printk("JWT length: %zd\n", strlen(buf));

	/* Save the token to be able to use buf again */
	strncpy(jwt, buf, sizeof(jwt));

	res = jwt_init_parser(&parse, jwt, buf, sizeof(buf));
	zassert_equal(res, 0, "Setting up jwt parsing");

	res = jwt_parse_payload(&parse, &parsed_expt, &parsed_iat, parsed_aud);
	zassert_equal(res, 0, "Parsing payload");
	zassert_equal(parsed_expt, expt, "Comparing expiration time");
	zassert_equal(parsed_iat, iat, "Comparing issued at");
	zassert_mem_equal(parsed_aud, aud, sizeof(aud), "Comparing audience");

	res = jwt_verify(&parse, jwt_test_private_der, jwt_test_private_der_len);
	zassert_equal(res, 0, "Verifying signature");
}

ZTEST_SUITE(jwt_tests, NULL, NULL, NULL, NULL, NULL);
