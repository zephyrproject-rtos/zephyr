/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/rtp.h>
#ifdef CONFIG_SRTP
#include <zephyr/net/srtp.h>
#endif
#include <zephyr/ztest.h>

#include "sine.h"

#define LOCALHOST_ADDR  "127.0.0.1"
#define LOCALHOST_ADDR6 "::1"
#define RTP_PORT        5004

#define PAYLOAD_TYPE 97

#define N_PACKETS  20
#define RX_TIMEOUT K_MSEC(5)

#define RTP_HDR_X_DATA_LEN 8
#define N_PADDING          4
#if CONFIG_RTP_MAX_CSRC_COUNT > 0
#define CSRC 0x12345678
#endif

static RTP_SESSION_DEFINE(test_rtp_session, 0);

#ifdef CONFIG_SRTP
/* clang-format off */
static const uint8_t srtp_master_key[SRTP_AES_256_KEY_LEN] = {
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
};
static const uint8_t srtp_master_salt[SRTP_MASTER_SALT_LEN] = {
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d,
};
/* clang-format on */
static struct srtp_session_ctx srtp_ctx;
#endif /* CONFIG_SRTP */

K_SEM_DEFINE(rtp_rx_sem, 0, 1);

#ifdef CONFIG_RTP_TRANSPORT_NET_PKT
#define TRANSPORT_TYPE RTP_TRANSPORT_NET_PKT
#else
#define TRANSPORT_TYPE RTP_TRANSPORT_SOCKET
#endif

static int n_received;
static void *payload = (void *)sine_100_48000_8_mono;
#ifdef CONFIG_SRTP
/* Keep room for the SRTP trailer within the loopback MTU */
static size_t payload_len = sizeof(sine_100_48000_8_mono) - SRTP_MAX_TRAILER_LEN;
#else
static size_t payload_len = sizeof(sine_100_48000_8_mono);
#endif

/* Data is mono and 8bit, so payload_len equals number of samples */
#define DELTA_TS payload_len

static uint8_t x_data[RTP_HDR_X_DATA_LEN];
static struct rtp_header_extension x_hdr;

static uint16_t prev_seq_num;
static uint32_t prev_ts;
static bool first_packet = true;

static bool padding;
static bool header_extension;
static bool marker;
static bool zero_payload;

static void rtp_cb(struct rtp_session *session, struct rtp_packet *packet, void *user_data)
{
	ARG_UNUSED(session);
	ARG_UNUSED(user_data);

	if (!first_packet) {
		zassert_equal(packet->header.seq - 1, prev_seq_num);
		zassert_equal(packet->header.ts - DELTA_TS, prev_ts);
	}
	first_packet = false;
	prev_seq_num = packet->header.seq;
	prev_ts = packet->header.ts;

	zassert_equal(rtp_header_get_v(&packet->header), RTP_VERSION);
	zassert_equal(rtp_header_get_m(&packet->header), marker ? RTP_MARKER : 0);
	if (padding > 0) {
		zassert_equal(rtp_header_get_p(&packet->header), 1);
	} else {
		zassert_equal(rtp_header_get_p(&packet->header), 0);
	}

	if (header_extension) {
		zassert_equal(rtp_header_get_x(&packet->header), 1);
		zassert_equal(x_hdr.definition, packet->header.header_extension.definition);
		zassert_equal(x_hdr.length, packet->header.header_extension.length);
		zassert_mem_equal(x_hdr.data, packet->header.header_extension.data,
				  packet->header.header_extension.length * sizeof(uint32_t));
	} else {
		zassert_equal(rtp_header_get_x(&packet->header), 0);
	}

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	zassert_equal(rtp_header_get_cc(&packet->header), 1);
	zassert_equal(packet->header.csrc[0], CSRC);
#endif
	zassert_equal(rtp_header_get_pt(&packet->header), PAYLOAD_TYPE);

	if (zero_payload) {
		zassert_equal(packet->payload_len, 0);
	} else {
		zassert_equal(packet->payload_len, payload_len);
		zassert_mem_equal(packet->payload, payload, payload_len);
	}

	n_received++;
	k_sem_give(&rtp_rx_sem);
}

ZTEST(rtp_tests, test_api)
{
	/* Initialising NULL */
	zassert_not_ok(
		rtp_session_init(NULL, NULL, NULL, RTP_ROLE_BOTH, 0, NULL, NULL, TRANSPORT_TYPE));
	zassert_not_ok(rtp_session_start(NULL));
	zassert_not_ok(rtp_session_stop(NULL));
	zassert_not_ok(rtp_session_send(NULL, NULL, 0, 0, 0, 0, NULL, NULL));
	zassert_not_ok(rtp_session_send_simple(NULL, NULL, 0, 0));

	/* Initialise header extension with data not aligned to uint32_t */
	zassert_not_ok(rtp_init_header_extension(&x_hdr, 0xBEEF, x_data, 1));

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	for (size_t i = 0; i < CONFIG_RTP_MAX_CSRC_COUNT; i++) {
		zassert_ok(rtp_session_add_csrc(&test_rtp_session, CSRC + i));
	}

	/* Can't add more than CONFIG_RTP_MAX_CSRC_COUNT csrcs */
	zassert_not_ok(rtp_session_add_csrc(&test_rtp_session, CSRC - 1));

	zassert_ok(rtp_session_remove_csrc(&test_rtp_session, CSRC));
	zassert_not_ok(rtp_session_remove_csrc(&test_rtp_session, CSRC - 1));
#endif

	/* Starting rx session twice */
	zassert_ok(rtp_session_start(&test_rtp_session));
	zassert_not_ok(rtp_session_start(&test_rtp_session));

	/* Stopping session and restarting it */
	zassert_ok(rtp_session_stop(&test_rtp_session));
	zassert_ok(rtp_session_start(&test_rtp_session));
}

ZTEST(rtp_tests, test_loopback)
{
	uint32_t timestamp;

	zassert_ok(rtp_session_start(&test_rtp_session));

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	zassert_ok(rtp_session_add_csrc(&test_rtp_session, CSRC));
#endif

	/* Send simple RTP packets */

	k_sem_reset(&rtp_rx_sem);
	for (size_t i = 0; i < N_PACKETS; i++) {
		zassert_ok(rtp_session_send_simple(&test_rtp_session, (void *)payload, payload_len,
						   DELTA_TS));

		zassert_ok(k_sem_take(&rtp_rx_sem, RX_TIMEOUT));
	}

	zassert_equal(n_received, N_PACKETS);

	/* Send RTP packets with a header extension and padding */

	zassert_ok(rtp_init_header_extension(&x_hdr, 0xBEEF, x_data, sizeof(x_data)));

	k_sem_reset(&rtp_rx_sem);
	n_received = 0;
	padding = true;
	header_extension = true;
	marker = true;
	for (size_t i = 0; i < N_PACKETS; i++) {
		zassert_ok(rtp_session_send(&test_rtp_session, (void *)payload, payload_len,
					    DELTA_TS, N_PADDING, RTP_MARKER, &x_hdr, &timestamp));
		zassert_equal(timestamp, test_rtp_session.timestamp - DELTA_TS);
		zassert_ok(k_sem_take(&rtp_rx_sem, RX_TIMEOUT));
	}

	zassert_equal(n_received, N_PACKETS);

	/* Send zero-payload RTP packets */
	zero_payload = true;
	zassert_ok(rtp_session_send(&test_rtp_session, NULL, 0, 0, N_PADDING, RTP_MARKER, &x_hdr,
				    NULL));
	zassert_ok(k_sem_take(&rtp_rx_sem, RX_TIMEOUT));
}

static struct net_sockaddr_storage test_sockaddr;

/* Initialize the session (and SRTP when enabled); also used to verify
 * re-initialization of an SRTP protected session.
 */
static void test_session_init(void)
{
	struct net_if *iface;

	iface = net_if_lookup_by_dev(device_get_binding("lo"));
	zassert_not_null(iface);

	zassert_ok(rtp_session_init(&test_rtp_session, iface, (struct net_sockaddr *)&test_sockaddr,
				    RTP_ROLE_BOTH, PAYLOAD_TYPE, rtp_cb, NULL, TRANSPORT_TYPE));

#ifdef CONFIG_SRTP
	struct srtp_policy policy = {
#if defined(CONFIG_RTP_LOOPBACK_TEST_SRTP_GCM_256)
		.cipher = SRTP_CIPHER_AES_256_GCM,
		.auth = SRTP_AUTH_NULL,
		.master_key_len = SRTP_AES_256_KEY_LEN,
		.master_salt_len = SRTP_AES_GCM_SALT_LEN,
#elif defined(CONFIG_RTP_LOOPBACK_TEST_SRTP_GCM)
		.cipher = SRTP_CIPHER_AES_128_GCM,
		.auth = SRTP_AUTH_NULL,
		.master_key_len = SRTP_AES_128_KEY_LEN,
		.master_salt_len = SRTP_AES_GCM_SALT_LEN,
#else
		.cipher = SRTP_CIPHER_AES_128_CM,
		.auth = SRTP_AUTH_HMAC_SHA1_80,
		.master_key_len = SRTP_AES_128_KEY_LEN,
		.master_salt_len = SRTP_MASTER_SALT_LEN,
#endif
		.master_key = srtp_master_key,
		.master_salt = srtp_master_salt,
	};

	zassert_ok(rtp_session_set_srtp(&test_rtp_session, &policy, &policy, &srtp_ctx));
#endif /* CONFIG_SRTP */
}

#ifdef CONFIG_SRTP
ZTEST(rtp_tests, test_srtp_lifecycle)
{
	uint16_t seq_before;

	zassert_ok(rtp_session_start(&test_rtp_session));

	k_sem_reset(&rtp_rx_sem);
	n_received = 0;
	first_packet = true;
	padding = false;
	header_extension = false;
	marker = false;
	zero_payload = false;

	zassert_ok(rtp_session_send_simple(&test_rtp_session, payload, payload_len, DELTA_TS));
	zassert_ok(k_sem_take(&rtp_rx_sem, RX_TIMEOUT));

	/* Restarting without re-initialization preserves the sequence number,
	 * keeping the SRTP packet index monotonic under the same master key.
	 */
	seq_before = test_rtp_session.sequence_number;
	zassert_ok(rtp_session_stop(&test_rtp_session));
	zassert_ok(rtp_session_start(&test_rtp_session));
	zassert_equal(test_rtp_session.sequence_number, seq_before);

	first_packet = true;
	zassert_ok(rtp_session_send_simple(&test_rtp_session, payload, payload_len, DELTA_TS));
	zassert_ok(k_sem_take(&rtp_rx_sem, RX_TIMEOUT));

	/* Re-initialization clears the installed SRTP context, so it can be
	 * installed again for the new SSRC.
	 */
	zassert_ok(rtp_session_stop(&test_rtp_session));
	test_session_init();
	zassert_ok(rtp_session_start(&test_rtp_session));

	first_packet = true;
	zassert_ok(rtp_session_send_simple(&test_rtp_session, payload, payload_len, DELTA_TS));
	zassert_ok(k_sem_take(&rtp_rx_sem, RX_TIMEOUT));
}
#endif /* CONFIG_SRTP */

static void *rtp_tests_setup(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		struct net_sockaddr_in *sockaddr_in = (struct net_sockaddr_in *)&test_sockaddr;
		struct net_in_addr test_in_addr;

		zassert_ok(net_addr_pton(NET_AF_INET, LOCALHOST_ADDR, &test_in_addr));

		sockaddr_in->sin_family = NET_AF_INET;
		sockaddr_in->sin_addr = test_in_addr;
		sockaddr_in->sin_port = net_htons(RTP_PORT);
	} else {
		struct net_sockaddr_in6 *sockaddr_in6 = (struct net_sockaddr_in6 *)&test_sockaddr;
		struct net_in6_addr test_in6_addr;

		zassert_ok(net_addr_pton(NET_AF_INET6, LOCALHOST_ADDR6, &test_in6_addr));

		sockaddr_in6->sin6_family = NET_AF_INET6;
		sockaddr_in6->sin6_addr = test_in6_addr;
		sockaddr_in6->sin6_port = net_htons(RTP_PORT);
	}

	test_session_init();

	return NULL;
}

static void rtp_tests_after(void *f)
{
	ARG_UNUSED(f);

	zassert_ok(rtp_session_stop(&test_rtp_session));
}

ZTEST_SUITE(rtp_tests, NULL, rtp_tests_setup, NULL, rtp_tests_after, NULL);
