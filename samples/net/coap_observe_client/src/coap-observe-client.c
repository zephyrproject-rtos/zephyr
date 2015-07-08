/* coap-observe-client.c - Observe a remote resource */

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

#include <drivers/rand32.h>

#include <errno.h>

#include <net/net_core.h>
#include <net/net_socket.h>

#include "er-coap-engine.h"
#include "er-coap-observe-client.h"

#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>

#if defined(CONFIG_NET_TESTING)
#include <net_testing.h>
#endif

#if defined(CONFIG_NANOKERNEL)
#define STACKSIZE 2000
char fiberStack[STACKSIZE];
#endif

static coap_observee_t *obs;

#if defined(CONFIG_NETWORKING_WITH_IPV6)
/* admin-local, dynamically allocated multicast address */
#define MCAST_IPADDR { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2 } } }

/* Define the peer IP address where to send messages */
#if !defined(CONFIG_NET_TESTING)
#define PEER_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#define MY_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2 } } }

#endif

#else /* CONFIG_NETWORKING_WITH_IPV6 */

#error "IPv4 not supported at the moment, fix me!"

/* Organization-local 239.192.0.0/14 */
#define MCAST_IPADDR { { { 239, 192, 0, 2 } } }

#if !defined(CONFIG_NET_TESTING)
#define PEER_IPADDR { { { 192, 0, 2, 1 } } }
#endif

#endif /* CONFIG_NETWORKING_WITH_IPV6 */

#define MY_PORT 8484
#define PEER_PORT COAP_DEFAULT_PORT

/* The path of the resource to observe */
//#define OBS_RESOURCE_URI "test/push"
#define OBS_RESOURCE_URI "time"

/* Toggle interval in seconds */
#define TOGGLE_INTERVAL 5

#if defined(CONFIG_NETWORKING_WITH_IPV6)
static const struct in6_addr in6addr_peer = PEER_IPADDR;
static struct in6_addr in6addr_my = MY_IPADDR;
#else
static struct in_addr in4addr_peer = PEER_IPADDR;
static struct in_addr in4addr_my = MY_IPADDR;
#endif

static inline void init_app(void)
{
	PRINT("%s: run coap observe client\n", __func__);

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

/*
 * Handle the response to the observe request and the following notifications
 */
static void notification_callback(coap_observee_t *obs, void *notification,
				  coap_notification_flag_t flag)
{
	int len = 0;
	const uint8_t *payload = NULL;

	PRINT("Notification handler\n");
	PRINT("Observee URI: %s\n", obs->url);
	if(notification) {
		len = coap_get_payload(notification, &payload);
	}
	switch(flag) {
	case NOTIFICATION_OK:
		PRINT("NOTIFICATION OK: %*s\n", len, (char *)payload);
		break;
	case OBSERVE_OK: /* server accepeted observation request */
		PRINT("OBSERVE_OK: %*s\n", len, (char *)payload);
		break;
	case OBSERVE_NOT_SUPPORTED:
		PRINT("OBSERVE_NOT_SUPPORTED: %*s\n", len, (char *)payload);
		obs = NULL;
		break;
	case ERROR_RESPONSE_CODE:
		PRINT("ERROR_RESPONSE_CODE: %*s\n", len, (char *)payload);
		obs = NULL;
		break;
	case NO_REPLY_FROM_SERVER:
		PRINT("NO_REPLY_FROM_SERVER: "
		      "removing observe registration with token %x%x\n",
		      obs->token[0], obs->token[1]);
		obs = NULL;
		break;
	}
}

/*
 * Toggle the observation of the remote resource
 */
void toggle_observation(coap_context_t *coap_ctx)
{
	if(obs) {
		PRINT("Stopping observation\n");
		coap_obs_remove_observee(obs);
		obs = NULL;
	} else {
		PRINT("Starting observation\n");
		obs = coap_obs_request_registration(coap_ctx,
					    (uip_ipaddr_t *)&in6addr_peer,
						    PEER_PORT,
						    OBS_RESOURCE_URI,
						    notification_callback,
						    NULL);
	}
}

#define WAIT_TIME 1
#define WAIT_TICKS (WAIT_TIME * sys_clock_ticks_per_sec)

#if 0
#define WAIT_PEER (5 * sys_clock_ticks_per_sec)
#else
#define WAIT_PEER TICKS_UNLIMITED
#endif

PROCESS(coap_observe_client_process, "CoAP observe client process");
PROCESS_THREAD(coap_observe_client_process, ev, data, buf, user_data)
{
  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_TIMER);
    /* Currently the timeout is handled in while loop in startup() */
  }
  PROCESS_END();
}

void startup(void)
{
	static struct etimer et;
	static coap_context_t *coap_ctx;
	bool first_round = true;

	net_init();

	coap_init_engine();
	coap_init_mid();

	init_app();

#if defined(CONFIG_NETWORKING_WITH_BT)
	if (bt_enable(NULL)) {
		PRINT("Bluetooth init failed\n");
		return;
	}
	ipss_init();
	ipss_advertise();
#endif

	coap_ctx = coap_context_new((uip_ipaddr_t *)&in6addr_my, MY_PORT);
	if (!coap_ctx) {
		PRINT("Cannot get CoAP context.\n");
		return;
	}

	coap_context_set_key_handlers(coap_ctx,
				      get_psk_info,
				      get_ecdsa_key,
				      verify_ecdsa_key);

	if (!coap_context_connect(coap_ctx,
				  (uip_ipaddr_t *)&in6addr_peer,
				  PEER_PORT)) {
		PRINT("Cannot connect to peer.\n");
		return;
	}

	etimer_set(&et, TOGGLE_INTERVAL * sys_clock_ticks_per_sec,
		   &coap_observe_client_process);

	while (1) {
		while (!coap_context_is_connected(coap_ctx)) {
			PRINT("Trying to connect to peer...\n");
			if (!coap_context_wait_data(coap_ctx, WAIT_TICKS)) {
				continue;
			}
		}

		if(first_round || etimer_expired(&et)) {
			PRINT("--Toggle timer--\n");
			toggle_observation(coap_ctx);
			PRINT("--Done--\n");
			etimer_restart(&et);
			first_round = false;
		}

		if (coap_context_wait_data(coap_ctx, WAIT_TICKS)) {
#if defined(CONFIG_NANOKERNEL)
			/* Print the stack usage only if we did something */
			net_analyze_stack("CoAP observe client",
					  fiberStack, STACKSIZE);
#endif
		}

		coap_check_transactions();
	}
	coap_context_close(coap_ctx);
}

#if defined(CONFIG_NANOKERNEL)

void main(void)
{
	fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)startup, 0, 0, 7, 0);
}

#endif /* CONFIG_NANOKERNEL */
