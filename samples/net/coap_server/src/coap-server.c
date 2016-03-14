/* coap-server.c - Erbium REST engine example */

/*
 * Copyright (c) 2015 Intel Corporation.
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

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#if defined(CONFIG_TINYDTLS_DEBUG)
#define DEBUG DEBUG_FULL
#else
#define DEBUG DEBUG_PRINT
#endif
#include "contiki/ip/uip-debug.h"

#include <zephyr.h>

#if defined(CONFIG_NANOKERNEL)
#if defined(CONFIG_TINYDTLS)
/* DTLS needs bigger stack */
#define STACKSIZE 2500
#else
#define STACKSIZE 1700
#endif
char fiberStack[STACKSIZE];
#endif

#include <drivers/rand32.h>

#include <errno.h>

#include <net/net_core.h>
#include <net/net_socket.h>

#include "contiki/ipv6/uip-ds6.h"
#include "rest-engine.h"
#include "er-coap.h"
#include "er-coap-engine.h"

#if defined(CONFIG_TINYDTLS_DEBUG)
#include <net/tinydtls.h>
#endif

#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>

#if defined(CONFIG_NET_TESTING)
#include <net_testing.h>
#endif

#if defined(CONFIG_ER_COAP_WITH_DTLS)
#define MY_PORT COAP_DEFAULT_SECURE_PORT
#else
#define MY_PORT COAP_DEFAULT_PORT
#endif

#define PEER_PORT 0

static inline void init_app(void)
{
	PRINT("%s: run coap server\n", __func__);

#if defined(CONFIG_NET_TESTING)
	net_testing_setup();
#endif
}

#if defined(DTLS_PSK)
/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
static int get_psk_info(struct dtls_context_t *ctx,
			const session_t *session,
			dtls_credentials_type_t type,
			const unsigned char *id, size_t id_len,
			unsigned char *result, size_t result_length)
{
	struct keymap_t {
		unsigned char *id;
		size_t id_length;
		unsigned char *key;
		size_t key_length;
	} psk[3] = {
		{ (unsigned char *)"Client_identity", 15,
		  (unsigned char *)"secretPSK", 9 },
		{ (unsigned char *)"default identity", 16,
		  (unsigned char *)"\x11\x22\x33", 3 },
		{ (unsigned char *)"\0", 2,
		  (unsigned char *)"", 1 }
	};

	if (type != DTLS_PSK_KEY) {
		return 0;
	}

	if (id) {
		int i;
		for (i = 0; i < sizeof(psk)/sizeof(struct keymap_t); i++) {
			if (id_len == psk[i].id_length &&
			    memcmp(id, psk[i].id, id_len) == 0) {
				if (result_length < psk[i].key_length) {
					PRINTF("buffer too small for PSK");
					return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
				}

				memcpy(result, psk[i].key, psk[i].key_length);
				return psk[i].key_length;
			}
		}
	}

	return dtls_alert_fatal_create(DTLS_ALERT_DECRYPT_ERROR);
}
#else
#define get_psk_info NULL
#endif /* DTLS_PSK */

#if defined(DTLS_ECC)
const unsigned char ecdsa_priv_key[] = {
			0xD9, 0xE2, 0x70, 0x7A, 0x72, 0xDA, 0x6A, 0x05,
			0x04, 0x99, 0x5C, 0x86, 0xED, 0xDB, 0xE3, 0xEF,
			0xC7, 0xF1, 0xCD, 0x74, 0x83, 0x8F, 0x75, 0x70,
			0xC8, 0x07, 0x2D, 0x0A, 0x76, 0x26, 0x1B, 0xD4};

const unsigned char ecdsa_pub_key_x[] = {
			0xD0, 0x55, 0xEE, 0x14, 0x08, 0x4D, 0x6E, 0x06,
			0x15, 0x59, 0x9D, 0xB5, 0x83, 0x91, 0x3E, 0x4A,
			0x3E, 0x45, 0x26, 0xA2, 0x70, 0x4D, 0x61, 0xF2,
			0x7A, 0x4C, 0xCF, 0xBA, 0x97, 0x58, 0xEF, 0x9A};

const unsigned char ecdsa_pub_key_y[] = {
			0xB4, 0x18, 0xB6, 0x4A, 0xFE, 0x80, 0x30, 0xDA,
			0x1D, 0xDC, 0xF4, 0xF4, 0x2E, 0x2F, 0x26, 0x31,
			0xD0, 0x43, 0xB1, 0xFB, 0x03, 0xE2, 0x2F, 0x4D,
			0x17, 0xDE, 0x43, 0xF9, 0xF9, 0xAD, 0xEE, 0x70};

static int get_ecdsa_key(struct dtls_context_t *ctx,
			 const session_t *session,
			 const dtls_ecdsa_key_t **result)
{
	static const dtls_ecdsa_key_t ecdsa_key = {
		.curve = DTLS_ECDH_CURVE_SECP256R1,
		.priv_key = ecdsa_priv_key,
		.pub_key_x = ecdsa_pub_key_x,
		.pub_key_y = ecdsa_pub_key_y
	};

	*result = &ecdsa_key;
	return 0;
}

static int verify_ecdsa_key(struct dtls_context_t *ctx,
			    const session_t *session,
			    const unsigned char *other_pub_x,
			    const unsigned char *other_pub_y,
			    size_t key_size)
{
	return 0;
}
#else
#define get_ecdsa_key    NULL
#define verify_ecdsa_key NULL
#endif /* DTLS_ECC */

#if 0
#define WAIT_TIME 1
#define WAIT_TICKS (WAIT_TIME * sys_clock_ticks_per_sec)
#else
#define WAIT_TICKS TICKS_UNLIMITED
#endif

extern resource_t
	res_plugtest_test,
	res_plugtest_validate,
	res_plugtest_create1,
	res_plugtest_create2,
	res_plugtest_create3,
	res_plugtest_longpath,
	res_plugtest_query,
	res_plugtest_locquery,
	res_plugtest_multi,
	res_plugtest_link1,
	res_plugtest_link2,
	res_plugtest_link3,
	res_plugtest_path,
	res_plugtest_separate,
	res_plugtest_large,
	res_plugtest_large_update,
	res_plugtest_large_create,
	res_plugtest_obs;

void startup(void)
{
	static coap_context_t *coap_ctx;
	static struct net_addr any_addr;
	static struct net_addr my_addr;

#if defined(CONFIG_NETWORKING_WITH_IPV6)
	static const struct in6_addr in6addr_my = IN6ADDR_ANY_INIT;
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	any_addr.in6_addr = in6addr_any;
	any_addr.family = AF_INET6;

	my_addr.in6_addr = in6addr_my;
	my_addr.family = AF_INET6;
#else
	any_addr.in_addr = in4addr_any;
	any_addr.family = AF_INET;

	my_addr.in_addr = in4addr_my;
	my_addr.family = AF_INET;
#endif

	PRINT("Starting ETSI IoT Plugtests Server\n");

	PRINT("uIP buffer: %u\n", UIP_BUFSIZE);
	PRINT("LL header: %u\n", UIP_LLH_LEN);
	PRINT("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
	PRINT("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

	net_init();

	rest_init_engine();

#if defined(CONFIG_TINYDTLS_DEBUG)
	dtls_set_log_level(DTLS_LOG_DEBUG);
#endif

	init_app();

#if defined(CONFIG_NETWORKING_WITH_BT)
	if (bt_enable(NULL)) {
		PRINT("Bluetooth init failed\n");
		return;
	}
	ipss_init();
	ipss_advertise();
#endif

	/* Activate the application-specific resources. */
	rest_activate_resource(&res_plugtest_test, "test");
	rest_activate_resource(&res_plugtest_longpath, "seg1/seg2/seg3");
	rest_activate_resource(&res_plugtest_query, "query");

#if NOT_SUPPORTED
	/* These are not supported atm. */
	rest_activate_resource(&res_plugtest_separate, "separate");
#endif

#if 0
	/* Currently these are not activated. */
	rest_activate_resource(&res_plugtest_validate, "validate");
	rest_activate_resource(&res_plugtest_create1, "create1");
	rest_activate_resource(&res_plugtest_create2, "create2");
	rest_activate_resource(&res_plugtest_create3, "create3");
	rest_activate_resource(&res_plugtest_locquery, "location-query");
	rest_activate_resource(&res_plugtest_multi, "multi-format");
	rest_activate_resource(&res_plugtest_link1, "link1");
	rest_activate_resource(&res_plugtest_link2, "link2");
	rest_activate_resource(&res_plugtest_link3, "link3");
	rest_activate_resource(&res_plugtest_path, "path");
	rest_activate_resource(&res_plugtest_large, "large");
	rest_activate_resource(&res_plugtest_large_update, "large-update");
	rest_activate_resource(&res_plugtest_large_create, "large-create");
	rest_activate_resource(&res_plugtest_obs, "obs");
#endif

#if defined(CONFIG_NETWORKING_WITH_IPV6)
	coap_ctx = coap_init_server((uip_ipaddr_t *)&my_addr.in6_addr,
				    MY_PORT,
				    (uip_ipaddr_t *)&any_addr.in6_addr,
				    PEER_PORT);
#else
	coap_ctx = coap_init_server((uip_ipaddr_t *)&my_addr.in4_addr,
				    MY_PORT,
				    (uip_ipaddr_t *)&any_addr.in4_addr,
				    PEER_PORT);
#endif

	coap_context_set_key_handlers(coap_ctx,
				      get_psk_info,
				      get_ecdsa_key,
				      verify_ecdsa_key);

	/* Read requests and pass them to rest engine */
	while (1) {
		if (coap_context_wait_data(coap_ctx, WAIT_TICKS)) {
#if defined(CONFIG_NANOKERNEL)
			/* Print the stack usage only if we did something */
			net_analyze_stack("CoAP server", fiberStack, STACKSIZE);
#endif
		}
		coap_check_transactions();
	}
}

#if defined(CONFIG_NANOKERNEL)
void main(void)
{
	fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)startup, 0, 0, 7, 0);
}
#endif
