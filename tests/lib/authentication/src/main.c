/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <auth/auth_lib.h>

/**
 * @brief Test Asserts
 *
 * This test verifies various assert macros provided by ztest.
 *
 */

#define SHARED_KEY_LEN          (32u)

/* Use a different key than default */
static uint8_t chal_resp_sharedkey[SHARED_KEY_LEN] = {
	0x21, 0x8e, 0x37, 0x42, 0x1e, 0xe1, 0x2a, 0x22, 0x7c, 0x4b, 0x3f, 0x3f, 0x07, 0x5e, 0x8a, 0xd8,
	0x24, 0xdf, 0xca, 0xf4, 0x04, 0xd0, 0x3e, 0x22, 0x61, 0x9f, 0x24, 0xa3, 0xc7, 0xf6, 0x5d, 0x66
};

static struct auth_optional_param chal_resp_param = {
	.param_id = AUTH_CHALRESP_PARAM,
	.param_body = {
		.chal_resp = {
			.shared_key = chal_resp_sharedkey,
		},
	}
};


static void auth_status_callback(struct authenticate_conn *auth_conn, enum auth_instance_id instance,
				 enum auth_status status, void *context)
{
	/* dummy function */
}

static void test_auth_api(void)
{
	int ret_val = 0;
	struct authenticate_conn auth_conn;

	/* init library with NULL status function callback */
	ret_val = auth_lib_init(&auth_conn, AUTH_INST_1_ID,  NULL, NULL,
				NULL, AUTH_CONN_SERVER | AUTH_CONN_CHALLENGE_AUTH_METHOD);

	zassert_equal(ret_val, AUTH_ERROR_INVALID_PARAM, "NULL status function param test failed.");

	/* Verify server and client roles flags fail */
	ret_val = auth_lib_init(&auth_conn, AUTH_INST_1_ID,  auth_status_callback, NULL,
				NULL, AUTH_CONN_SERVER | AUTH_CONN_CLIENT | AUTH_CONN_CHALLENGE_AUTH_METHOD);

	zassert_equal(ret_val, AUTH_ERROR_INVALID_PARAM, "Invalid flags test failed.");

	/* Verify DLTS and Challenge-Response flags fail */
	ret_val = auth_lib_init(&auth_conn, AUTH_INST_1_ID,  auth_status_callback, NULL,
				NULL, AUTH_CONN_SERVER | AUTH_CONN_DTLS_AUTH_METHOD |
					      AUTH_CONN_CHALLENGE_AUTH_METHOD);

	zassert_equal(ret_val, AUTH_ERROR_INVALID_PARAM, "Invalid flags test failed.");

	/* init lib with valid params */
	ret_val = auth_lib_init(&auth_conn, AUTH_INST_1_ID,  auth_status_callback, NULL,
				&chal_resp_param, AUTH_CONN_SERVER | AUTH_CONN_CHALLENGE_AUTH_METHOD);

	zassert_equal(ret_val, AUTH_SUCCESS, "Failed to initialize Authentication library.");

	/* de-init */
	ret_val = auth_lib_deinit(&auth_conn);

	zassert_equal(ret_val, AUTH_SUCCESS, "Failed to start Challenge-Response authentication.");

}

void test_main(void)
{
	ztest_test_suite(authentication_tests,
			 ztest_unit_test(test_auth_api)
			 );

	ztest_run_test_suite(authentication_tests);
}
