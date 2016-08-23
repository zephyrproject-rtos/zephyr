/* dtls-server.c - dtls server */

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

#include <zephyr.h>

#include <errno.h>

#include <net/ip_buf.h>
#include <net/net_core.h>
#include <net/net_socket.h>

#include <net/tinydtls.h>

#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>

#if defined(CONFIG_NET_TESTING)
#include <net_testing.h>
#endif

static const unsigned char ecdsa_priv_key[] = {
			0xD9, 0xE2, 0x70, 0x7A, 0x72, 0xDA, 0x6A, 0x05,
			0x04, 0x99, 0x5C, 0x86, 0xED, 0xDB, 0xE3, 0xEF,
			0xC7, 0xF1, 0xCD, 0x74, 0x83, 0x8F, 0x75, 0x70,
			0xC8, 0x07, 0x2D, 0x0A, 0x76, 0x26, 0x1B, 0xD4};

static const unsigned char ecdsa_pub_key_x[] = {
			0xD0, 0x55, 0xEE, 0x14, 0x08, 0x4D, 0x6E, 0x06,
			0x15, 0x59, 0x9D, 0xB5, 0x83, 0x91, 0x3E, 0x4A,
			0x3E, 0x45, 0x26, 0xA2, 0x70, 0x4D, 0x61, 0xF2,
			0x7A, 0x4C, 0xCF, 0xBA, 0x97, 0x58, 0xEF, 0x9A};

static const unsigned char ecdsa_pub_key_y[] = {
			0xB4, 0x18, 0xB6, 0x4A, 0xFE, 0x80, 0x30, 0xDA,
			0x1D, 0xDC, 0xF4, 0xF4, 0x2E, 0x2F, 0x26, 0x31,
			0xD0, 0x43, 0xB1, 0xFB, 0x03, 0xE2, 0x2F, 0x4D,
			0x17, 0xDE, 0x43, 0xF9, 0xF9, 0xAD, 0xEE, 0x70};

#define MY_PORT 4242

static inline void init_app(void)
{
	PRINT("%s: run dtls server\n", __func__);

#if defined(CONFIG_NET_TESTING)
	net_testing_setup();
#endif
}

static inline void reverse(unsigned char *buf, int len)
{
	int i, last = len - 1;

	for (i = 0; i < len / 2; i++) {
		unsigned char tmp = buf[i];
		buf[i] = buf[last - i];
		buf[last - i] = tmp;
	}
}

/* How many tics to wait for a network packet */
#if 0
#define WAIT_TIME 1
#define WAIT_TICKS (WAIT_TIME * sys_clock_ticks_per_sec)
#else
#define WAIT_TICKS TICKS_UNLIMITED
#endif

static inline void receive_message(const char *name,
				   struct net_context *recv,
				   dtls_context_t *dtls)
{
	struct net_buf *buf;

	buf = net_receive(recv, WAIT_TICKS);
	if (buf) {
		session_t session;

		dtls_session_init(&session);

		uip_ipaddr_copy(&session.addr.ipaddr,
				&NET_BUF_IP(buf)->srcipaddr);
		session.addr.port = NET_BUF_UDP(buf)->srcport;

		PRINT("Received data %p datalen %d\n",
		      ip_buf_appdata(buf), ip_buf_appdatalen(buf));

		dtls_handle_message(dtls, &session, ip_buf_appdata(buf),
				    ip_buf_appdatalen(buf));

		/* We never send the buffer by this function. A network buffer
		 * to be sent is allocated in send_to_peer() that is
		 * responsible for sending the data.
		 */
		ip_buf_unref(buf);
	}
}

#if defined(CONFIG_NETWORKING_WITH_IPV6)
static struct in6_addr in6addr_my = IN6ADDR_ANY_INIT;
#else
static struct in_addr in4addr_my = { { { 0 } } };
#endif

static inline struct net_context *get_context(void)
{
	static struct net_addr any_addr;
	static struct net_addr my_addr;
	struct net_context *ctx;

#if defined(CONFIG_NETWORKING_WITH_IPV6)
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	any_addr.in6_addr = in6addr_any;
	any_addr.family = AF_INET6;

	my_addr.in6_addr = in6addr_my;
	my_addr.family = AF_INET6;
#else
	static const struct in_addr in4addr_any = { { { 0 } } };

	any_addr.in_addr = in4addr_any;
	any_addr.family = AF_INET;

	my_addr.in_addr = in4addr_my;
	my_addr.family = AF_INET;
#endif

	ctx = net_context_get(IPPROTO_UDP,
			      &any_addr, 0,
			      &my_addr, MY_PORT);
	if (!ctx) {
		PRINT("%s: Cannot get network context\n", __func__);
		return NULL;
	}

	return ctx;
}

static int read_from_peer(struct dtls_context_t *ctx,
			  session_t *session,
			  uint8 *data, size_t len)
{
	PRINT("%s: read from peer %p len %d\n", __func__, data, len);

	/* In this test we reverse the received bytes.
	 * We could also just pass the data back as is.
	 */
	reverse(data, len);

	/* echo incoming application data */
	return dtls_write(ctx, session, data, len);
}

static int send_to_peer(struct dtls_context_t *ctx,
			session_t *session,
			uint8 *data, size_t len)
{
	struct net_context *recv =
			(struct net_context *)dtls_get_app_data(ctx);
	struct net_buf *buf;
	int max_data_len;
	uint8_t *ptr;

	buf = ip_buf_get_tx(recv);
	if (!buf) {
		len = -ENOBUFS;
		goto out;
	}

	max_data_len = IP_BUF_MAX_DATA - sizeof(struct uip_udp_hdr) -
					sizeof(struct uip_ip_hdr);

	PRINT("%s: reply to peer data %p len %d\n", __func__, data, len);

	if (len > max_data_len) {
		PRINT("%s: too much (%d bytes) data to send (max %d bytes)\n",
		      __func__, len, max_data_len);
		ip_buf_unref(buf);
		len = -EINVAL;
		goto out;
	}

	/* Note that we have reversed the addresses here
	 * because net_reply() will reverse them again.
	 */
#if defined(CONFIG_NETWORKING_WITH_IPV6)
	uip_ip6addr_copy(&NET_BUF_IP(buf)->destipaddr,
			 (uip_ip6addr_t *)&in6addr_my);
	uip_ip6addr_copy(&NET_BUF_IP(buf)->srcipaddr,
			 (uip_ip6addr_t *)&session->addr.ipaddr);
#else
	uip_ip4addr_copy(&NET_BUF_IP(buf)->destipaddr,
			 (uip_ip4addr_t *)&in4addr_my);
	uip_ip4addr_copy(&NET_BUF_IP(buf)->srcipaddr,
			 (uip_ip4addr_t *)&session->addr.ipaddr);
#endif
	NET_BUF_UDP(buf)->destport = uip_ntohs(MY_PORT);
	NET_BUF_UDP(buf)->srcport = session->addr.port;

	uip_set_udp_conn(buf) = net_context_get_udp_connection(recv);

	ptr = net_buf_add(buf, len);
	memcpy(ptr, data, len);
	ip_buf_appdata(buf) = ptr;
	ip_buf_appdatalen(buf) = len;

	if (net_reply(recv, buf)) {
		ip_buf_unref(buf);
	}

out:
	return len;
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
					dtls_warn("buffer too small for PSK");
					return dtls_alert_fatal_create(
						DTLS_ALERT_INTERNAL_ERROR);
				}

				memcpy(result, psk[i].key, psk[i].key_length);
				return psk[i].key_length;
			}
		}
	}

	return dtls_alert_fatal_create(DTLS_ALERT_DECRYPT_ERROR);
}
#endif /* DTLS_PSK */

#if defined(DTLS_ECC)
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
#endif /* DTLS_ECC */

static int handle_event(struct dtls_context_t *ctx, session_t *session,
			dtls_alert_level_t level, unsigned short code)
{
	dtls_debug("event: level %d code %d\n", level, code);

	if (level > 0) {
		/* alert code, quit */
	} else if (level == 0) {
		/* internal event */
		if (code == DTLS_EVENT_CONNECTED) {
			PRINT("*** Connected ***\n");
		}
	}

	return 0;
}

static void init_dtls(struct net_context *recv, dtls_context_t **dtls)
{
	static dtls_handler_t cb = {
		.write = send_to_peer,
		.read  = read_from_peer,
		.event = handle_event,
#if defined(DTLS_PSK)
		.get_psk_info = get_psk_info,
#endif /* DTLS_PSK */
#if defined(DTLS_ECC)
		.get_ecdsa_key = get_ecdsa_key,
		.verify_ecdsa_key = verify_ecdsa_key
#endif /* DTLS_ECC */
	};

	PRINT("DTLS server started\n");

#if defined(CONFIG_TINYDTLS_DEBUG)
	dtls_set_log_level(DTLS_LOG_DEBUG);
#endif

	*dtls = dtls_new_context(recv);
	if (*dtls) {
		dtls_set_handler(*dtls, &cb);
	}
}

void startup(void)
{
	static dtls_context_t *dtls;
	static struct net_context *recv;

	net_init();

	dtls_init();

	init_app();

#if defined(CONFIG_NETWORKING_WITH_BT)
	if (bt_enable(NULL)) {
		PRINT("Bluetooth init failed\n");
		return;
	}
	ipss_init();
	ipss_advertise();
#endif

	recv = get_context();
	if (!recv) {
		PRINT("%s: Cannot get network context\n", __func__);
		return;
	}

	init_dtls(recv, &dtls);
	if (!dtls) {
		PRINT("%s: Cannot get DTLS context\n", __func__);
		return;
	}

	while (1) {
		receive_message(__func__, recv, dtls);
	}
}

#if defined(CONFIG_NANOKERNEL)

#define STACKSIZE 3000
char fiberStack[STACKSIZE];

void main(void)
{
	fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)startup, 0, 0, 7, 0);
}

#endif /* CONFIG_MICROKERNEL ||  CONFIG_NANOKERNEL */
