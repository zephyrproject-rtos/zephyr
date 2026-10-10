/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *  SPDX-FileCopyrightText: 2026 Sayed Naser Moravej
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/srtp.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

#include "net_sample_common.h"
#ifdef CONFIG_SRTP_SAMPLE_ROLE_SOURCE
#include "sine.h"
#endif

LOG_MODULE_REGISTER(main);

#define TRANSPORT_TYPE RTP_TRANSPORT_SOCKET

#define MCAST_ADDR "239.0.1.1"
#define MCAST_ADDR6 "ff02::5004"
#define MCAST_PORT 5004

static RTP_SESSION_DEFINE(my_rtp_session, 0);

struct srtp_session my_srtp_session =
{
	.rtp_se = &my_rtp_session
};


#ifdef CONFIG_SRTP_SAMPLE_ROLE_SINK
static uint32_t n_received;

static void timeout_worker(struct k_work *work)
{
	ARG_UNUSED(work);

	n_received = 0;
	LOG_WRN("Timeout occurred");
}

K_WORK_DELAYABLE_DEFINE(timeout_work, timeout_worker);

static void my_srtp_session_recv_cb(struct rtp_session *session, struct rtp_packet *packet,
				   void *user_data)
{
	ARG_UNUSED(session);
	ARG_UNUSED(packet);
	ARG_UNUSED(user_data);

	if (n_received++ == 0) {
		LOG_INF("Start receiving");
	} else if (n_received % 100 == 0) {
		LOG_INF("Received %u packets", n_received);
	}

	k_work_reschedule(&timeout_work, K_MSEC(500));
}

static void srtp_sink(struct srtp_session *session, struct net_sockaddr *sockaddress)
{
	int ret;

	ret = srtp_session_init_rx(session, net_if_get_default(), sockaddress,
				  my_srtp_session_recv_cb, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to init srtp session (%d)", ret);
		return;
	}

	n_received = 0;
	ret = srtp_session_start(session);
	if (ret < 0) {
		LOG_ERR("Failed to start srtp session (%d)", ret);
		return;
	}
}
#endif /* CONFIG_SRTP_SAMPLE_ROLE_SINK */

#ifdef CONFIG_SRTP_SAMPLE_ROLE_SOURCE
#define PAYLOAD_TYPE 1
// #define PAYLOAD_TYPE 97

static void srtp_source(struct srtp_session *session, struct net_sockaddr *sockaddress)
{
	void *payload;
	int period_ms;
	int delta_ts;
	size_t size;
	int ret;

	ret = srtp_session_init_tx(session, net_if_get_default(), sockaddress, PAYLOAD_TYPE);
	if (ret < 0) {
		LOG_ERR("Failed to init srtp session (%d)", ret);
		return;
	}

	ret = srtp_session_start(session);
	if (ret < 0) {
		LOG_ERR("Failed to start srtp session (%d)", ret);
	}

	ARRAY_FOR_EACH(sine_100_8000_16_mono, i) {
		sine_100_8000_16_mono[i] = sys_cpu_to_be16(sine_100_8000_16_mono[i]);
	}
	payload = (void *)sine_100_8000_16_mono;
	size = sizeof(sine_100_8000_16_mono);
	period_ms = 50;
	delta_ts = size / sizeof(uint16_t);
	char word[] = "Text to send over SRTP";
	LOG_INF("Start sending SRTP packets");
	while (true) {
		ret = srtp_session_send_simple(session, (void*)word, strlen(word) + 1, delta_ts);
		// ret = srtp_session_send_simple(session, payload, size, delta_ts);
		LOG_INF("%s", word);
		if (ret < 0) {
			LOG_ERR("Failed to send srtp (%d)", ret);
			return;
		}

		k_msleep(period_ms);
	}
}
#endif /* CONFIG_SRTP_SAMPLE_ROLE_SOURCE */

int main(void)
{
	struct net_sockaddr_storage sockaddress = {};

#if defined(CONFIG_SRTP_SAMPLE_ROLE_SINK)
	LOG_INF("RTP sink on %s", CONFIG_BOARD);
#elif defined(CONFIG_SRTP_SAMPLE_ROLE_SOURCE)
	LOG_INF("RTP source on %s", CONFIG_BOARD);
#endif

	wait_for_network();

	if (IS_ENABLED(CONFIG_RTP_SAMPLE_IPV6)) {
		struct net_sockaddr_in6 *sockaddress_in6 = net_sin6(net_sad(&sockaddress));
		struct net_in6_addr in6_address;

		if (net_addr_pton(NET_AF_INET6, MCAST_ADDR6, &in6_address) != 0) {
			LOG_ERR("Invalid address: %s", MCAST_ADDR6);
			return -EINVAL;
		}

		sockaddress_in6->sin6_family = NET_AF_INET6;
		sockaddress_in6->sin6_addr = in6_address;
		sockaddress_in6->sin6_port = net_htons(MCAST_PORT);
	} else {
		struct net_sockaddr_in *sockaddress_in = net_sin(net_sad(&sockaddress));
		struct net_in_addr in_address;

		if (net_addr_pton(NET_AF_INET, MCAST_ADDR, &in_address) != 0) {
			LOG_ERR("Invalid address: %s", MCAST_ADDR);
			return -EINVAL;
		}

		sockaddress_in->sin_family = NET_AF_INET;
		sockaddress_in->sin_addr = in_address;
		sockaddress_in->sin_port = net_htons(MCAST_PORT);
	}

#if defined(CONFIG_SRTP_SAMPLE_ROLE_SINK)
	srtp_sink(&my_srtp_session, net_sad(&sockaddress));
#elif defined(CONFIG_SRTP_SAMPLE_ROLE_SOURCE)
	srtp_source(&my_srtp_session, net_sad(&sockaddress));
#endif

	return 0;
}
