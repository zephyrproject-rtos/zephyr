/* dtls-server.c - dtls server */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#ifdef CONFIG_MICROKERNEL
#include <zephyr.h>
#else
#include <nanokernel.h>
#endif

#include <errno.h>

#include <net/net_core.h>
#include <net/net_socket.h>

#include <net/tinydtls.h>

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

/* The peer is the client in our case. Just invent a mac
 * address for it because lower parts of the stack cannot set it
 * in this test as we do not have any radios.
 */
//static uint8_t peer_mac[] = { 0x15, 0x0a, 0xbe, 0xef, 0xf0, 0x0d };

/* This is my mac address
 */
static uint8_t my_mac[] = { 0x0a, 0xbe, 0xef, 0x15, 0xf0, 0x0d };

#ifdef CONFIG_NETWORKING_WITH_IPV6
/* The 2001:db8::/32 is the private address space for documentation RFC 3849 */
#define MY_IPADDR { { { 0x20,0x01,0x0d,0xb8,0,0,0,0,0,0,0,0,0,0,0,0x2 } } }

#else
/* The 192.0.2.0/24 is the private address space for documentation RFC 5737 */
#define MY_IPADDR { { { 192,0,2,2 } } }

#endif
#define MY_PORT 4242

#ifdef CONFIG_NETWORKING_WITH_IPV6
static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
static struct in6_addr in6addr_my = MY_IPADDR;
#else
static const struct in_addr in4addr_any = { { { 0 } } };
static struct in_addr in4addr_my = MY_IPADDR;
#endif

static inline void init_server()
{
	PRINT("%s: run echo server\n", __FUNCTION__);

	net_set_mac(my_mac, sizeof(my_mac));

#ifdef CONFIG_NETWORKING_WITH_IPV4
	{
		uip_ipaddr_t addr;
		uip_ipaddr(&addr, 192,0,2,2);
		uip_sethostaddr(&addr);
	}
#else
	{
		uip_ipaddr_t *addr;

		addr = (uip_ipaddr_t *)&in6addr_my;
		uip_ds6_addr_add(addr, 0, ADDR_MANUAL);
	}
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
#define WAIT_TIME 1
#define WAIT_TICKS (WAIT_TIME * sys_clock_ticks_per_sec)

static inline void receive_message(const char *name,
				   struct net_context *recv,
				   dtls_context_t *dtls)
{
	struct net_buf *buf;

	buf = net_receive(recv, WAIT_TICKS);
	if (buf) {
		session_t session;

		dtls_session_init(&session);

		uip_ipaddr_copy(&session.addr.ipaddr, &NET_BUF_IP(buf)->srcipaddr);
		session.addr.port = NET_BUF_UDP(buf)->srcport;

		PRINT("Received data buf %p buflen %d data %p datalen %d\n",
		      buf, uip_len(buf),
		      net_buf_data(buf), net_buf_datalen(buf));

		dtls_handle_message(dtls, &session, net_buf_data(buf),
				    net_buf_datalen(buf));

		/* We never send the buffer by this function. A network buffer
		 * to be sent is allocated in send_to_peer() that is
		 * responsible for sending the data.
		 */
		net_buf_put(buf);
	}
}

static inline struct net_context *get_context(void)
{
	static struct net_addr any_addr;
	static struct net_addr my_addr;
	struct net_context *ctx;

#ifdef CONFIG_NETWORKING_WITH_IPV6
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

	ctx = net_context_get(IPPROTO_UDP,
			      &any_addr, 0,
			      &my_addr, MY_PORT);
	if (!ctx) {
		PRINT("%s: Cannot get network context\n", __FUNCTION__);
		return NULL;
	}

	return ctx;
}

static int read_from_peer(struct dtls_context_t *ctx,
			  session_t *session,
			  uint8 *data, size_t len)
{
	PRINT("%s: read from peer %p len %d\n", __FUNCTION__, data, len);

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

	buf = net_buf_get(recv);
	if (!buf) {
		len = -ENOBUFS;
		goto out;
	}

	max_data_len = sizeof(buf->buf) - sizeof(struct uip_udp_hdr) -
					sizeof(struct uip_ip_hdr);

	PRINT("%s: reply to peer data %p len %d\n", __FUNCTION__, data, len);

	if (len > max_data_len) {
		PRINT("%s: too much (%d bytes) data to send (max %d bytes)\n",
		      __FUNCTION__, len, max_data_len);
		net_buf_put(buf);
		len = -EINVAL;
		goto out;
	}

	/* Note that we have reversed the addresses here
	 * because net_reply() will reverse them again.
	 */
#ifdef CONFIG_NETWORKING_WITH_IPV6
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

	memcpy(net_buf_add(buf, 0), data, len);
	net_buf_add(buf, len);
	net_buf_datalen(buf) = len;

	if (net_reply(recv, buf)) {
		net_buf_put(buf);
	}

out:
	return len;
}

#ifdef DTLS_PSK
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

#ifdef DTLS_ECC
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
#ifdef DTLS_PSK
		.get_psk_info = get_psk_info,
#endif /* DTLS_PSK */
#ifdef DTLS_ECC
		.get_ecdsa_key = get_ecdsa_key,
		.verify_ecdsa_key = verify_ecdsa_key
#endif /* DTLS_ECC */
	};

	PRINT("DTLS server started\n");

#ifdef CONFIG_TINYDTLS_DEBUG
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

	init_server();

	recv = get_context();
	if (!recv) {
		PRINT("%s: Cannot get network context\n", __FUNCTION__);
		return;
	}

	init_dtls(recv, &dtls);
	if (!dtls) {
		PRINT("%s: Cannot get DTLS context\n", __FUNCTION__);
		return;
	}

	while (1) {
		receive_message(__FUNCTION__, recv, dtls);
	}
}

#ifdef CONFIG_NANOKERNEL

#define STACKSIZE 3000
char fiberStack[STACKSIZE];

void main(void)
{
	fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)startup, 0, 0, 7, 0);
}

#endif /* CONFIG_MICROKERNEL ||  CONFIG_NANOKERNEL */
