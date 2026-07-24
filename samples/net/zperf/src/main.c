/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zperf sample.
 */
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/net/net_config.h>

LOG_MODULE_REGISTER(zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#ifdef CONFIG_NET_LOOPBACK_SIMULATE_PACKET_DROP
#include <zephyr/net/loopback.h>
#endif

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
#include <sample_usbd.h>
#endif

#if defined(CONFIG_ZPERF_LOOPBACK_SELFTEST)
#include <zephyr/net/net_ip.h>
#include <zephyr/net/zperf.h>

#define SELFTEST_PORT        CONFIG_ZPERF_LOOPBACK_SELFTEST_PORT
#define SELFTEST_DURATION_MS CONFIG_ZPERF_LOOPBACK_SELFTEST_DURATION_MS
#define SELFTEST_PACKET_SIZE CONFIG_ZPERF_LOOPBACK_SELFTEST_PACKET_SIZE
#define SELFTEST_RATE_KBPS   CONFIG_ZPERF_LOOPBACK_SELFTEST_RATE_KBPS
#define SELFTEST_FRAG_PACKET_SIZE CONFIG_ZPERF_LOOPBACK_SELFTEST_FRAG_PACKET_SIZE

/* Non-default socket priority applied to every upload so that, when more than
 * one traffic class is configured, traffic is routed through the per-traffic-
 * class threads (net_tc.c) instead of the caller's context. Set to -1 (i.e. do
 * not set SO_PRIORITY) when priority support is not compiled in.
 */
#if defined(CONFIG_NET_CONTEXT_PRIORITY)
#define SELFTEST_PRIORITY NET_PRIORITY_VI
#else
#define SELFTEST_PRIORITY (-1)
#endif

/* Loopback addresses used for both the server bind and the client connect.
 * The loopback driver assigns 127.0.0.1 and ::1 to the loopback interface, so
 * both are valid local endpoints.
 */
#define SELFTEST_IPV4_ADDR   "127.0.0.1"
#define SELFTEST_IPV6_ADDR   "::1"

/* When the configured rate is 0 (unlimited), the runner picks a rate high
 * enough that the per-packet pacing delay in the UDP uploader rounds down to
 * zero, so the client sends as fast as the (virtual) CPU allows. The computed
 * rate (in kbps) is stored in a uint32_t field, so it must fit; the math below
 * is done in 64-bit to avoid an intermediate overflow. This bounds the usable
 * packet size at roughly 500 kB, far above any realistic MTU.
 */
BUILD_ASSERT(((uint64_t)SELFTEST_PACKET_SIZE * 8U * 1000000U) / 1024U + 1U <=
		     UINT32_MAX,
	     "Unlimited-rate loopback selftest packet size overflows rate_kbps");
BUILD_ASSERT(((uint64_t)SELFTEST_FRAG_PACKET_SIZE * 8U * 1000000U) / 1024U + 1U <=
		     UINT32_MAX,
	     "Unlimited-rate loopback selftest frag packet size overflows rate_kbps");

static void selftest_session_cb(enum zperf_status status,
				struct zperf_results *result,
				void *user_data)
{
	ARG_UNUSED(status);
	ARG_UNUSED(result);
	ARG_UNUSED(user_data);
}

/* Fill a socket address with the loopback endpoint for the given family. */
static void selftest_fill_addr(struct net_sockaddr *addr, int family)
{
	if (family == NET_AF_INET6) {
		struct net_sockaddr_in6 *a6 = net_sin6(addr);

		memset(a6, 0, sizeof(*a6));
		a6->sin6_family = NET_AF_INET6;
		a6->sin6_port = net_htons(SELFTEST_PORT);
		(void)net_addr_pton(NET_AF_INET6, SELFTEST_IPV6_ADDR,
				    &a6->sin6_addr);
	} else {
		struct net_sockaddr_in *a4 = net_sin(addr);

		memset(a4, 0, sizeof(*a4));
		a4->sin_family = NET_AF_INET;
		a4->sin_port = net_htons(SELFTEST_PORT);
		(void)net_addr_pton(NET_AF_INET, SELFTEST_IPV4_ADDR,
				    &a4->sin_addr);
	}
}

static void selftest_fill_upload(struct zperf_upload_params *param, int family,
				 uint32_t packet_size, bool tcp_nodelay)
{
	uint32_t rate_kbps = SELFTEST_RATE_KBPS;

	if (rate_kbps == 0U) {
		/* Force packet_duration_us == 0, i.e. no pacing. Compute in
		 * 64-bit to avoid overflowing the intermediate product for
		 * near-MTU packet sizes, then store the (small enough) result.
		 */
		rate_kbps = (uint32_t)(((uint64_t)packet_size * 8U *
					1000000U) / 1024U + 1U);
	}

	memset(param, 0, sizeof(*param));
	selftest_fill_addr(&param->peer_addr, family);
	param->duration_ms = SELFTEST_DURATION_MS;
	param->packet_size = packet_size;
	param->rate_kbps = rate_kbps;
	param->unix_offset_us = (uint64_t)k_uptime_get() * USEC_PER_MSEC;
	param->options.priority = SELFTEST_PRIORITY;
	param->options.tcp_nodelay = tcp_nodelay ? 1 : 0;
}

/* Machine-parseable throughput line for the twister console harness. */
static void selftest_report(const char *proto, struct zperf_results *res)
{
	uint64_t kbps = 0U;

	if (res->client_time_in_us != 0U) {
		kbps = ((uint64_t)res->nb_packets_sent *
			(uint64_t)res->packet_size * 8U * USEC_PER_SEC) /
		       ((uint64_t)res->client_time_in_us * 1000U);
	}

	printk("ZPERF-RESULT %s_mbps=%u.%03u\n", proto,
	       (uint32_t)(kbps / 1000U), (uint32_t)(kbps % 1000U));
}

static int selftest_run_udp(int family, const char *label, uint32_t packet_size)
{
	struct zperf_download_params dl = { .port = SELFTEST_PORT };
	struct zperf_upload_params ul;
	struct zperf_results res = { 0 };
	int ret;

	/* Bind the receiver to the loopback address so the client can reach it
	 * (in particular ::1, which is not the interface's default address).
	 */
	selftest_fill_addr(&dl.addr, family);

	ret = zperf_udp_download(&dl, selftest_session_cb, NULL);
	if (ret < 0) {
		return ret;
	}

	selftest_fill_upload(&ul, family, packet_size, false);
	ret = zperf_udp_upload(&ul, &res);
	(void)zperf_udp_download_stop();
	if (ret < 0) {
		return ret;
	}

	selftest_report(label, &res);

	return 0;
}

static int selftest_run_tcp(int family, const char *label, bool tcp_nodelay)
{
	struct zperf_download_params dl = { .port = SELFTEST_PORT };
	struct zperf_upload_params ul;
	struct zperf_results res = { 0 };
	int ret;

	selftest_fill_addr(&dl.addr, family);

	ret = zperf_tcp_download(&dl, selftest_session_cb, NULL);
	if (ret < 0) {
		return ret;
	}

	selftest_fill_upload(&ul, family, SELFTEST_PACKET_SIZE, tcp_nodelay);
	ret = zperf_tcp_upload(&ul, &res);
	(void)zperf_tcp_download_stop();
	if (ret < 0) {
		return ret;
	}

	selftest_report(label, &res);

	return 0;
}

/* Whether the extra, intentionally-fragmenting UDP run should be performed for
 * the given family (needs the matching IP fragmentation support compiled in).
 */
static bool selftest_frag_enabled(int family)
{
	if (family == NET_AF_INET6) {
		return IS_ENABLED(CONFIG_NET_IPV6_FRAGMENT);
	}

	return IS_ENABLED(CONFIG_NET_IPV4_FRAGMENT);
}

static void selftest_run_family(int family, const char *udp_label,
				const char *udp_frag_label,
				const char *tcp_label,
				const char *tcp_nodelay_label)
{
	int ret;

	ret = selftest_run_udp(family, udp_label, SELFTEST_PACKET_SIZE);
	if (ret < 0) {
		printk("ZPERF-RESULT %s_mbps=error(%d)\n", udp_label, ret);
	}

	/* Oversized datagram: forces IP fragmentation on TX and reassembly on
	 * RX. Only meaningful when fragmentation is compiled in.
	 */
	if (selftest_frag_enabled(family)) {
		ret = selftest_run_udp(family, udp_frag_label,
				       SELFTEST_FRAG_PACKET_SIZE);
		if (ret < 0) {
			printk("ZPERF-RESULT %s_mbps=error(%d)\n",
			       udp_frag_label, ret);
		}
	}

	ret = selftest_run_tcp(family, tcp_label, false);
	if (ret < 0) {
		printk("ZPERF-RESULT %s_mbps=error(%d)\n", tcp_label, ret);
	}

	/* Same transfer with Nagle disabled to cover the TCP_NODELAY send path. */
	ret = selftest_run_tcp(family, tcp_nodelay_label, true);
	if (ret < 0) {
		printk("ZPERF-RESULT %s_mbps=error(%d)\n", tcp_nodelay_label,
		       ret);
	}
}

static void run_loopback_selftest(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		selftest_run_family(NET_AF_INET, "udp4", "udp4_frag", "tcp4",
				    "tcp4_nodelay");
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		selftest_run_family(NET_AF_INET6, "udp6", "udp6_frag", "tcp6",
				    "tcp6_nodelay");
	}

	printk("ZPERF-DONE\n");
}
#endif /* CONFIG_ZPERF_LOOPBACK_SELFTEST */

int main(void)
{
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
	struct usbd_context *sample_usbd;
	int err;

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	err = usbd_enable(sample_usbd);
	if (err) {
		return err;
	}

	(void)net_config_init_app(NULL, "Initializing network");
#endif /* CONFIG_USB_DEVICE_STACK_NEXT */

#ifdef CONFIG_NET_LOOPBACK_SIMULATE_PACKET_DROP
	loopback_set_packet_drop_ratio(1);
#endif

#if defined(CONFIG_NET_DHCPV4) && !defined(CONFIG_NET_CONFIG_SETTINGS)
	net_dhcpv4_start(net_if_get_default());
#endif

#if defined(CONFIG_ZPERF_LOOPBACK_SELFTEST)
	/* Bring the network up ourselves (auto-init is disabled in the
	 * loopback overlay) and then run the self-contained throughput test.
	 */
	(void)net_config_init_app(NULL, "Initializing network");

	run_loopback_selftest();
#endif /* CONFIG_ZPERF_LOOPBACK_SELFTEST */

	return 0;
}
