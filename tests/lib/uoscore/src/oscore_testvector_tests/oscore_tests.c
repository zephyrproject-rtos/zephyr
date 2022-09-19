/*
 * Copyright (c) 2021 Fraunhofer AISEC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <oscore.h>

#include "oscore_test_vectors.h"

/**
 * Test 1:
 * - Client Key derivation with master salt see RFC8613 Appendix C.1.1
 * - Generating OSCORE request with key form C.1.1 see RFC8613 Appendix C.4
 */
ZTEST(oscore_tests, oscore_client_test1)
{
	enum err r;
	struct context c_client;
	struct oscore_init_params params = {
		.dev_type = CLIENT,
		.master_secret.ptr = (uint8_t *)T1__MASTER_SECRET,
		.master_secret.len = T1__MASTER_SECRET_LEN,
		.sender_id.ptr = (uint8_t *)T1__SENDER_ID,
		.sender_id.len = T1__SENDER_ID_LEN,
		.recipient_id.ptr = (uint8_t *)T1__RECIPIENT_ID,
		.recipient_id.len = T1__RECIPIENT_ID_LEN,
		.master_salt.ptr = (uint8_t *)T1__MASTER_SALT,
		.master_salt.len = T1__MASTER_SALT_LEN,
		.id_context.ptr = (uint8_t *)T1__ID_CONTEXT,
		.id_context.len = T1__ID_CONTEXT_LEN,
		.aead_alg = OSCORE_AES_CCM_16_64_128,
		.hkdf = OSCORE_SHA_256,
	};

	r = oscore_context_init(&params, &c_client);

	zassert_equal(r, ok, "Error in oscore_context_init");

	/*
	 * required only for the test vector.
	 * during normal operation the sender sequence number is
	 * increased automatically after every sending
	 */
	c_client.sc.sender_seq_num = 20;

	uint8_t buf_oscore[256];
	uint32_t buf_oscore_len = sizeof(buf_oscore);

	r = coap2oscore((uint8_t *)T1__COAP_REQ, T1__COAP_REQ_LEN,
			(uint8_t *)&buf_oscore, &buf_oscore_len, &c_client);
	zassert_equal(r, ok, "Error in coap2oscore!");

	zassert_mem_equal__(c_client.sc.sender_key.ptr, T1__SENDER_KEY,
			    c_client.sc.sender_key.len,
			    "T1 sender key derivation failed");

	zassert_mem_equal__(c_client.rc.recipient_key.ptr, T1__RECIPIENT_KEY,
			    c_client.rc.recipient_key.len,
			    "T1 recipient key derivation failed");

	zassert_mem_equal__(c_client.cc.common_iv.ptr, T1__COMMON_IV,
			    c_client.cc.common_iv.len,
			    "T1 common IV derivation failed");

	zassert_mem_equal__(&buf_oscore, T1__OSCORE_REQ, T1__OSCORE_REQ_LEN,
			    "coap2oscore failed");
}

/**
 * Test 3:
 * - Client Key derivation without master salt see RFC8613 Appendix C.2.1
 * - Generating OSCORE request with key form C.2.1 see RFC8613 Appendix C.5
 */
ZTEST(oscore_tests, oscore_client_test3)
{
	enum err r;
	struct context c_client;
	struct oscore_init_params params = {
		.dev_type = CLIENT,
		.master_secret.ptr = (uint8_t *)T3__MASTER_SECRET,
		.master_secret.len = T3__MASTER_SECRET_LEN,
		.sender_id.ptr = (uint8_t *)T3__SENDER_ID,
		.sender_id.len = T3__SENDER_ID_LEN,
		.recipient_id.ptr = (uint8_t *)T3__RECIPIENT_ID,
		.recipient_id.len = T3__RECIPIENT_ID_LEN,
		.master_salt.ptr = (uint8_t *)T3__MASTER_SALT,
		.master_salt.len = T3__MASTER_SALT_LEN,
		.id_context.ptr = (uint8_t *)T3__ID_CONTEXT,
		.id_context.len = T3__ID_CONTEXT_LEN,
		.aead_alg = OSCORE_AES_CCM_16_64_128,
		.hkdf = OSCORE_SHA_256,
	};

	r = oscore_context_init(&params, &c_client);

	zassert_equal(r, ok, "Error in oscore_context_init");

	/*
	 * required only for the test vector.
	 * during normal operation the sender sequence number is
	 * increased automatically after every sending
	 */
	c_client.sc.sender_seq_num = 20;

	uint8_t buf_oscore[256];
	uint32_t buf_oscore_len = sizeof(buf_oscore);

	r = coap2oscore((uint8_t *)T3__COAP_REQ, T3__COAP_REQ_LEN,
			(uint8_t *)&buf_oscore, &buf_oscore_len, &c_client);

	zassert_equal(r, ok, "Error in coap2oscore!");

	zassert_mem_equal__(&buf_oscore, T3__OSCORE_REQ, T3__OSCORE_REQ_LEN,
			    "coap2oscore failed");
}

/**
 * Test 5 :
 * - Client Key derivation with ID Context see Appendix 3.1
 * - OSCORE request generation see Appendix C6
 */
ZTEST(oscore_tests, oscore_client_test5)
{
	enum err r;
	struct context c_client;
	struct oscore_init_params params = {
		.dev_type = CLIENT,
		.master_secret.ptr = (uint8_t *)T5__MASTER_SECRET,
		.master_secret.len = T5__MASTER_SECRET_LEN,
		.sender_id.ptr = (uint8_t *)T5__SENDER_ID,
		.sender_id.len = T5__SENDER_ID_LEN,
		.recipient_id.ptr = (uint8_t *)T5__RECIPIENT_ID,
		.recipient_id.len = T5__RECIPIENT_ID_LEN,
		.master_salt.ptr = (uint8_t *)T5__MASTER_SALT,
		.master_salt.len = T5__MASTER_SALT_LEN,
		.id_context.ptr = (uint8_t *)T5__ID_CONTEXT,
		.id_context.len = T5__ID_CONTEXT_LEN,
		.aead_alg = OSCORE_AES_CCM_16_64_128,
		.hkdf = OSCORE_SHA_256,
	};

	r = oscore_context_init(&params, &c_client);

	zassert_equal(r, ok, "Error in oscore_context_init");

	/*
	 * required only for the test vector.
	 * during normal operation the sender sequence number is
	 * increased automatically after every sending
	 */
	c_client.sc.sender_seq_num = 20;

	uint8_t buf_oscore[256];
	uint32_t buf_oscore_len = sizeof(buf_oscore);

	r = coap2oscore((uint8_t *)T5__COAP_REQ, T5__COAP_REQ_LEN,
			(uint8_t *)&buf_oscore, &buf_oscore_len, &c_client);

	zassert_equal(r, ok, "Error in coap2oscore!");

	zassert_mem_equal__(&buf_oscore, T5__OSCORE_REQ, buf_oscore_len,
			    "coap2oscore failed");
}

/**
 * Test 2:
 * - Server Key derivation with master salt see RFC8613 Appendix C.1.2
 * - Generating OSCORE response with key form C.1.2 see RFC8613 Appendix C.7
 */
ZTEST(oscore_tests, oscore_server_test2)
{
	enum err r;
	struct context c_server;
	struct oscore_init_params params_server = {
		.dev_type = SERVER,
		.master_secret.ptr = (uint8_t *)T2__MASTER_SECRET,
		.master_secret.len = T2__MASTER_SECRET_LEN,
		.sender_id.ptr = (uint8_t *)T2__SENDER_ID,
		.sender_id.len = T2__SENDER_ID_LEN,
		.recipient_id.ptr = (uint8_t *)T2__RECIPIENT_ID,
		.recipient_id.len = T2__RECIPIENT_ID_LEN,
		.master_salt.ptr = (uint8_t *)T2__MASTER_SALT,
		.master_salt.len = T2__MASTER_SALT_LEN,
		.id_context.ptr = (uint8_t *)T2__ID_CONTEXT,
		.id_context.len = T2__ID_CONTEXT_LEN,
		.aead_alg = OSCORE_AES_CCM_16_64_128,
		.hkdf = OSCORE_SHA_256,
	};

	r = oscore_context_init(&params_server, &c_server);

	zassert_equal(r, ok, "Error in oscore_context_init");

	/* Test decrypting of an incoming request */
	uint8_t buf_coap[256];
	uint32_t buf_coap_len = sizeof(buf_coap);
	bool oscore_present_flag = false;

	r = oscore2coap((uint8_t *)T2__OSCORE_REQ, T2__OSCORE_REQ_LEN, buf_coap,
			&buf_coap_len, &oscore_present_flag, &c_server);

	zassert_equal(r, ok, "Error in oscore2coap!");
	zassert_true(oscore_present_flag, "The packet is not OSCORE packet");
	zassert_mem_equal__(&buf_coap, T2__COAP_REQ, buf_coap_len,
			    "oscore2coap failed");

	/* Test generating an encrypted response, see Appendix C7 */
	uint8_t buf_oscore[256];
	uint32_t buf_oscore_len = sizeof(buf_oscore);

	r = coap2oscore((uint8_t *)T2__COAP_RESPONSE, T2__COAP_RESPONSE_LEN,
			(uint8_t *)&buf_oscore, &buf_oscore_len, &c_server);

	zassert_equal(r, ok, "Error in coap2oscore");

	zassert_mem_equal__(&buf_oscore, T2__OSCORE_RESP, buf_oscore_len,
			    "coap2oscore failed");
}

ZTEST(oscore_tests, oscore_server_test4)
{
	enum err r;
	struct context c_server;
	struct oscore_init_params params_server = {
		.dev_type = SERVER,
		.master_secret.ptr = (uint8_t *)T4__MASTER_SECRET,
		.master_secret.len = T4__MASTER_SECRET_LEN,
		.sender_id.ptr = (uint8_t *)T4__SENDER_ID,
		.sender_id.len = T4__SENDER_ID_LEN,
		.recipient_id.ptr = (uint8_t *)T4__RECIPIENT_ID,
		.recipient_id.len = T4__RECIPIENT_ID_LEN,
		.master_salt.ptr = (uint8_t *)T4__MASTER_SALT,
		.master_salt.len = T4__MASTER_SALT_LEN,
		.id_context.ptr = (uint8_t *)T4__ID_CONTEXT,
		.id_context.len = T4__ID_CONTEXT_LEN,
		.aead_alg = OSCORE_AES_CCM_16_64_128,
		.hkdf = OSCORE_SHA_256,
	};

	r = oscore_context_init(&params_server, &c_server);

	zassert_equal(r, ok, "Error in oscore_context_init");

	zassert_mem_equal__(c_server.sc.sender_key.ptr, T4__SENDER_KEY,
			    c_server.sc.sender_key.len,
			    "T4 sender key derivation failed");

	zassert_mem_equal__(c_server.rc.recipient_key.ptr, T4__RECIPIENT_KEY,
			    c_server.rc.recipient_key.len,
			    "T4 recipient key derivation failed");

	zassert_mem_equal__(c_server.cc.common_iv.ptr, T4__COMMON_IV,
			    c_server.cc.common_iv.len,
			    "T4 common IV derivation failed");
}

/**
 * Test 6:
 * - Server Key derivation with ID context see RFC8613 Appendix C.3.2
 */
ZTEST(oscore_tests, oscore_server_test6)
{
	enum err r;
	struct context c_server;
	struct oscore_init_params params_server = {
		.dev_type = SERVER,
		.master_secret.ptr = (uint8_t *)T6__MASTER_SECRET,
		.master_secret.len = T6__MASTER_SECRET_LEN,
		.sender_id.ptr = (uint8_t *)T6__SENDER_ID,
		.sender_id.len = T6__SENDER_ID_LEN,
		.recipient_id.ptr = (uint8_t *)T6__RECIPIENT_ID,
		.recipient_id.len = T6__RECIPIENT_ID_LEN,
		.master_salt.ptr = (uint8_t *)T6__MASTER_SALT,
		.master_salt.len = T6__MASTER_SALT_LEN,
		.id_context.ptr = (uint8_t *)T6__ID_CONTEXT,
		.id_context.len = T6__ID_CONTEXT_LEN,
		.aead_alg = OSCORE_AES_CCM_16_64_128,
		.hkdf = OSCORE_SHA_256,
	};

	r = oscore_context_init(&params_server, &c_server);

	zassert_equal(r, ok, "Error in oscore_context_init");

	zassert_mem_equal__(c_server.sc.sender_key.ptr, T6__SENDER_KEY,
			    c_server.sc.sender_key.len,
			    "T6 sender key derivation failed");

	zassert_mem_equal__(c_server.rc.recipient_key.ptr, T6__RECIPIENT_KEY,
			    c_server.rc.recipient_key.len,
			    "T6 recipient key derivation failed");

	zassert_mem_equal__(c_server.cc.common_iv.ptr, T6__COMMON_IV,
			    c_server.cc.common_iv.len,
			    "T6 common IV derivation failed");
}

/**
 * Test 8:
 * - Simple ACK packet should not be encrypted and result should be the same as
 *   input buffer (see RFC8613 Section 4.2)
 */
ZTEST(oscore_tests, oscore_misc_test8)
{
	enum err r;
	struct context c;
	struct oscore_init_params params = {
		.dev_type = SERVER,
		.master_secret.ptr = (uint8_t *)T7__MASTER_SECRET,
		.master_secret.len = T7__MASTER_SECRET_LEN,
		.sender_id.ptr = (uint8_t *)T7__SENDER_ID,
		.sender_id.len = T7__SENDER_ID_LEN,
		.recipient_id.ptr = (uint8_t *)T7__RECIPIENT_ID,
		.recipient_id.len = T7__RECIPIENT_ID_LEN,
		.master_salt.ptr = (uint8_t *)T7__MASTER_SALT,
		.master_salt.len = T7__MASTER_SALT_LEN,
		.id_context.ptr = (uint8_t *)T7__ID_CONTEXT,
		.id_context.len = T7__ID_CONTEXT_LEN,
		.aead_alg = OSCORE_AES_CCM_16_64_128,
		.hkdf = OSCORE_SHA_256,
	};

	r = oscore_context_init(&params, &c);

	zassert_equal(r, ok, "Error in oscore_context_init");

	/* Test if encrypting simple ACK message results in valid unencrypted
	 * message, see Section 4.2
	 */
	uint8_t buf_oscore[256];
	uint32_t buf_oscore_len = sizeof(buf_oscore);

	r = coap2oscore((uint8_t *)T8__COAP_ACK, T8__COAP_ACK_LEN,
			(uint8_t *)&buf_oscore, &buf_oscore_len, &c);

	zassert_equal(r, ok, "Error in coap2oscore");

	zassert_mem_equal__(&buf_oscore, T8__COAP_ACK, T8__COAP_ACK_LEN,
			    "coap2oscore failed");

	zassert_equal(buf_oscore_len, T8__COAP_ACK_LEN, "coap2oscore failed");
}
