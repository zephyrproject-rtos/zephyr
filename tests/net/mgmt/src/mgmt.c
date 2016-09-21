/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <tc_util.h>
#include <errno.h>

#include <net/net_mgmt.h>

#define TEST_MGMT_REQUEST	0x0ABC1234

int test_mgmt_request(uint32_t mgmt_request,
		    struct net_if *iface, void *data, uint32_t len)
{
	uint32_t *test_data = data;

	ARG_UNUSED(iface);

	if (len == sizeof(uint32_t)) {
		*test_data = 1;

		return 0;
	}

	return -EIO;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(TEST_MGMT_REQUEST, test_mgmt_request);

static inline int test_requesting_nm(void)
{
	uint32_t data = 0;

	TC_PRINT("- Request Net MGMT\n");

	if (net_mgmt(TEST_MGMT_REQUEST, NULL, &data, sizeof(data))) {
		return TC_FAIL;
	}

	return TC_PASS;
}

void main(void)
{
	int status = TC_FAIL;

	TC_PRINT("Starting Network Management API test\n");

	if (test_requesting_nm() != TC_PASS) {
		goto end;
	}

	status = TC_PASS;

end:
	TC_END_RESULT(status);
	TC_END_REPORT(status);
}
