/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ice_lite_sample, LOG_LEVEL_INF);

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/ice_lite.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>

/* RFC 8445 section 11: a keepalive after 15 seconds without data. */
#define KEEPALIVE_MS 15000

static void log_nomination(const struct ice_lite_agent *agent)
{
	const struct net_sockaddr_in *remote = net_sin(net_sad(&agent->remote));
	char text[NET_IPV4_ADDR_LEN];

	LOG_INF("Peer nominated %s port %u",
		net_addr_ntop(NET_AF_INET, &remote->sin_addr, text, sizeof(text)),
		net_ntohs(remote->sin_port));
}

/* Answer the checks of the peer and echo its data once it has nominated. */
static void serve(int sock, struct ice_lite_agent *agent)
{
	struct zsock_pollfd pfd = {.fd = sock, .events = ZSOCK_POLLIN};
	uint8_t resp[ICE_LITE_RESPONSE_SIZE];
	struct net_sockaddr_storage nominated = {0};
	struct net_sockaddr_storage src;
	int64_t last_tx = k_uptime_get();
	net_socklen_t src_len;
	uint8_t buf[1280];
	ssize_t len;
	int ret;

	while (true) {
		/* A keepalive is due when nothing has been sent to the peer for
		 * KEEPALIVE_MS; what the peer sends does not count.
		 */
		ret = zsock_poll(&pfd, 1, (int)MAX(KEEPALIVE_MS - (k_uptime_get() - last_tx), 0));
		if (ret < 0) {
			LOG_ERR("poll failed (%d)", -errno);
			return;
		}

		if (ret == 0) {
			ret = ice_lite_keepalive(resp, sizeof(resp));
			if (agent->nominated && (ret > 0)) {
				(void)zsock_sendto(sock, resp, (size_t)ret, 0,
						   net_sad(&agent->remote),
						   sizeof(struct net_sockaddr_in));
			}
			last_tx = k_uptime_get();
			continue;
		}

		src_len = sizeof(src);
		len = zsock_recvfrom(sock, buf, sizeof(buf), 0, net_sad(&src), &src_len);
		if (len < 0) {
			LOG_ERR("recvfrom failed (%d)", -errno);
			return;
		}

		ret = ice_lite_input(agent, buf, (size_t)len, net_sad(&src), resp, sizeof(resp));
		if (ret > 0) {
			(void)zsock_sendto(sock, resp, (size_t)ret, 0, net_sad(&src), src_len);
		}

		if (agent->nominated &&
		    (memcmp(&nominated, &agent->remote, sizeof(struct net_sockaddr_in)) != 0)) {
			memcpy(&nominated, &agent->remote, sizeof(struct net_sockaddr_in));
			log_nomination(agent);
		}

		/* Data is taken from the nominated address only. */
		if ((ret == -ENOMSG) && agent->nominated &&
		    (memcmp(&src, &agent->remote, sizeof(struct net_sockaddr_in)) == 0)) {
			LOG_INF("Echoing %d bytes", (int)len);
			(void)zsock_sendto(sock, buf, (size_t)len, 0, net_sad(&src), src_len);
			last_tx = k_uptime_get();
		}
	}
}

int main(void)
{
	struct net_sockaddr_in local = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(CONFIG_NET_SAMPLE_ICE_LITE_PORT),
	};
	struct ice_lite_agent agent;
	int sock;
	int ret;

	LOG_INF("Waiting for network...");
	(void)conn_mgr_wait_ready(CONN_MGR_READY_IPV4, NULL, K_FOREVER);

	ret = ice_lite_init(&agent);
	if (ret != 0) {
		LOG_ERR("Cannot set up the agent (%d)", ret);
		return 0;
	}

	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Cannot open a socket (%d)", -errno);
		return 0;
	}

	if (zsock_bind(sock, (const struct net_sockaddr *)&local, sizeof(local)) < 0) {
		LOG_ERR("Cannot bind the socket (%d)", -errno);
		(void)zsock_close(sock);
		return 0;
	}

	/* What the signaling channel of an application carries to the peer. */
	LOG_INF("ice-lite, port %d, priority %lu", CONFIG_NET_SAMPLE_ICE_LITE_PORT,
		ICE_LITE_HOST_PRIORITY(65535U, 1U));
	LOG_INF("ice-ufrag: %s", agent.ufrag);
	LOG_INF("ice-pwd: %s", agent.pwd);

	serve(sock, &agent);

	(void)zsock_close(sock);

	return 0;
}
