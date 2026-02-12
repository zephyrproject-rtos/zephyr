/*
 * Copyright (c) 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/rtp.h>
#include <zephyr/ztest.h>

#include "sine.h"

#define LOCALHOST_ADDR "127.0.0.1"
#define RTP_PORT       50200

#define PAYLOAD_TYPE 97
#define MARKER       1

#define N_PACKETS 20
#define RX_TMEOUT K_MSEC(5)

#define RTP_HDR_X_DATA_LEN 8
#define N_PADDING          4
#if CONFIG_RTP_MAX_CSRC_COUNT > 0
#define CSRC 0x12345678
#endif

static RTP_SESSION_DEFINE(test_tx_rtp_session, 0);
static RTP_SESSION_DEFINE(test_rx_rtp_session, 0);

K_SEM_DEFINE(rtp_rx_sem, 0, 1);

static int n_received;
static int8_t *payload = sine_100_48000_8_mono;
static size_t payload_len = sizeof(sine_100_48000_8_mono);

static uint8_t x_data[RTP_HDR_X_DATA_LEN];
static struct rtp_header_extension x_hdr;

static bool padding;
static bool header_extension;

static void rtp_cb(struct rtp_session *session, struct rtp_packet *packet, void *user_data)
{
	zassert_equal(packet->header.version, RTP_VERSION);
	zassert_equal(packet->header.m, MARKER);
	if (padding > 0) {
		zassert_equal(packet->header.p, 1);
	} else {
		zassert_equal(packet->header.p, 0);
	}

	if (header_extension) {
		zassert_equal(packet->header.x, 1);
		zassert_equal(x_hdr.definition, packet->header.header_extension.definition);
		zassert_equal(x_hdr.length, packet->header.header_extension.length);
		zassert_mem_equal(x_hdr.data, packet->header.header_extension.data,
				  packet->header.header_extension.length * sizeof(uint32_t));
	} else {
		zassert_equal(packet->header.x, 0);
	}

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	zassert_equal(packet->header.cc, 1);
	zassert_equal(packet->header.csrc[0], CSRC);
#endif
	zassert_equal(packet->header.pt, PAYLOAD_TYPE);

	zassert_equal(packet->payload_len, payload_len);
	zassert_mem_equal(packet->payload, payload, payload_len);

	n_received++;
	k_sem_give(&rtp_rx_sem);
}

ZTEST(rtp_tests, test_api)
{
	struct net_if *iface;
	struct net_in_addr test_in_addr;

	iface = net_if_lookup_by_dev(device_get_binding("lo"));
	zassert_not_null(iface);
	zassert_ok(net_addr_pton(NET_AF_INET, LOCALHOST_ADDR, &test_in_addr));

	/* Initialising NULL */
	zassert_not_ok(rtp_session_init(NULL, NULL, NULL, NULL, 0, 0, NULL, NULL));
	zassert_not_ok(rtp_session_start(NULL));
	zassert_not_ok(rtp_session_stop(NULL));
	zassert_not_ok(rtp_session_send(NULL, NULL, 0, 0, 0, NULL));
	zassert_not_ok(rtp_session_send_simple(NULL, NULL, 0, 0));

	/* Initialise header extension with data not alligend to uint32_t */
	zassert_not_ok(rtp_init_header_extension(&x_hdr, 0xBEEF, x_data, 1));

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	for (size_t i = 0; i < CONFIG_RTP_MAX_CSRC_COUNT; i++) {
		zassert_ok(rtp_session_add_csrc(&test_tx_rtp_session, CSRC + i));
	}

	/* Can't add more than CONFIG_RTP_MAX_CSRC_COUNT csrcs */
	zassert_not_ok(rtp_session_add_csrc(&test_tx_rtp_session, CSRC - 1));

	zassert_ok(rtp_session_remove_csrc(&test_tx_rtp_session, CSRC));
	zassert_not_ok(rtp_session_remove_csrc(&test_tx_rtp_session, CSRC - 1));
#endif

	/* Starting rx session twice */
	zassert_ok(rtp_session_start(&test_rx_rtp_session));
	zassert_not_ok(rtp_session_start(&test_rx_rtp_session));

	/* Stopping session and restarting it */
	zassert_ok(rtp_session_stop(&test_rx_rtp_session));
	zassert_ok(rtp_session_start(&test_rx_rtp_session));
}

ZTEST(rtp_tests, test_loopback)
{
	zassert_ok(rtp_session_start(&test_tx_rtp_session));
	zassert_ok(rtp_session_start(&test_rx_rtp_session));

#if CONFIG_RTP_MAX_CSRC_COUNT > 0
	zassert_ok(rtp_session_add_csrc(&test_tx_rtp_session, CSRC));
#endif

	/* Send simple RTP packets */

	k_sem_reset(&rtp_rx_sem);
	for (size_t i = 0; i < N_PACKETS; i++) {
		zassert_ok(rtp_session_send_simple(&test_tx_rtp_session, (void *)payload,
						   payload_len, 3));

		zassert_ok(k_sem_take(&rtp_rx_sem, RX_TMEOUT));
	}

	zassert_equal(n_received, N_PACKETS);

	/* Send RTP packets with a header extension and padding */

	zassert_ok(rtp_init_header_extension(&x_hdr, 0xBEEF, x_data, sizeof(x_data)));

	k_sem_reset(&rtp_rx_sem);
	n_received = 0;
	padding = true;
	header_extension = true;
	for (size_t i = 0; i < N_PACKETS; i++) {
		zassert_ok(rtp_session_send(&test_tx_rtp_session, (void *)payload, payload_len, 3,
					    N_PADDING, &x_hdr));

		zassert_ok(k_sem_take(&rtp_rx_sem, RX_TMEOUT));
	}

	zassert_equal(n_received, N_PACKETS);
}

static void *rtp_tests_setup(void)
{
	struct net_if *iface;
	struct net_in_addr test_in_addr;

	iface = net_if_lookup_by_dev(device_get_binding("lo"));
	zassert_not_null(iface);

	zassert_ok(net_addr_pton(NET_AF_INET, LOCALHOST_ADDR, &test_in_addr));

	struct net_sockaddr_in test_sockaddr_in = {
		.sin_family = NET_AF_INET,
		.sin_addr = test_in_addr,
		.sin_port = net_htons(RTP_PORT),
	};

	rtp_session_init_tx(&test_tx_rtp_session, iface, (struct net_sockaddr *)&test_sockaddr_in,
			    PAYLOAD_TYPE, MARKER);
	rtp_session_init_rx(&test_rx_rtp_session, iface, (struct net_sockaddr *)&test_sockaddr_in,
			    rtp_cb, NULL);

	return NULL;
}

static void rtp_tests_after(void *f)
{
	ARG_UNUSED(f);

	zassert_ok(rtp_session_stop(&test_tx_rtp_session));
	zassert_ok(rtp_session_stop(&test_rx_rtp_session));
}

ZTEST_SUITE(rtp_tests, NULL, rtp_tests_setup, NULL, rtp_tests_after, NULL);
