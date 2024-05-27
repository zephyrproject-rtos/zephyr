/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/wifi_mgmt.h>
#ifdef CONFIG_NET_SOCKETS_POSIX_NAMES
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/arpa/inet.h>
#else
#include <zephyr/net/socket.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sock_app, LOG_LEVEL_INF);

#define ECHO_SERVER_IP            CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#define ECHO_SERVER_PORT          2024
#define ECHO_SERVER_TIMEOUT_MS    1000
#define ECHO_SERVER_BUF_SIZE      300
#define ECHO_SERVER_DEADTIME_MS   1000

static void data_exchange(volatile bool *connected)
{
	LOG_INF("app started");

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	do {
		if (sock == -1) {
			LOG_ERR("socket failed: (%d) %s", errno, strerror(errno));
			break;
		}

		struct sockaddr_in me_addr = {
			.sin_family = AF_INET,
			.sin_port = htons(ECHO_SERVER_PORT),
			.sin_addr.s_addr = htonl(INADDR_ANY)
		};

		if (bind(sock, (const struct sockaddr *)&me_addr,
			sizeof(me_addr)) == -1) {
			LOG_ERR("bind: (%d) %s", errno, strerror(errno));
		}

		struct timeval tv = {
			.tv_sec = ECHO_SERVER_TIMEOUT_MS / 1000,
			.tv_usec = (ECHO_SERVER_TIMEOUT_MS % 1000) * 1000
		};

		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
			LOG_ERR("setsockopt failed: (%d) %s", errno, strerror(errno));
			break;
		}

		uint8_t cnt = 0;

		for (bool error = false; !error && *connected; cnt++) {

			struct sockaddr_in server_addr = {
				.sin_family = AF_INET,
				.sin_port = htons(ECHO_SERVER_PORT),
			};

			if (inet_pton(AF_INET, ECHO_SERVER_IP, &server_addr.sin_addr) != 1) {
				LOG_ERR("inet_pton failed");
				error = true;
				continue;
			}

			static uint8_t transmit_buf[ECHO_SERVER_BUF_SIZE];
			size_t transmit_buf_len = sizeof(transmit_buf);

			for (size_t i = 0; i < transmit_buf_len; i++) {
				transmit_buf[i] = cnt + i;
			}

			for (size_t i = 0; i < transmit_buf_len;) {
				ssize_t l = sendto(sock, &transmit_buf[i], transmit_buf_len - i, 0,
					(struct sockaddr *)&server_addr, sizeof(server_addr));
				if (l == -1) {
					LOG_ERR("sendto failed: (%d) %s", errno, strerror(errno));
					error = true;
					break;
				}
				i += l;
			}

			if (error) {
				continue;
			}

			LOG_INF("sent message to: %s:%u",
				ECHO_SERVER_IP, ntohs(server_addr.sin_port));

			socklen_t server_addr_len = sizeof(server_addr);
			static uint8_t receive_buf[sizeof(transmit_buf)];
			ssize_t receive_buf_len =
				recvfrom(sock, receive_buf, sizeof(receive_buf), 0,
					(struct sockaddr *)&server_addr, &server_addr_len);
			if (receive_buf_len == -1) {
				LOG_ERR("recvfrom failed: (%d) %s", errno, strerror(errno));
				continue;
			} else if (receive_buf_len == transmit_buf_len) {
				if (!memcmp(receive_buf, transmit_buf, transmit_buf_len)) {
					LOG_INF("all OK");
				} else {
					LOG_ERR("transmit and receive mismatch");
				}
			} else {
				LOG_ERR("transmit and receive lengths mismatch (%u - %u)",
					(unsigned int)transmit_buf_len,
					(unsigned int)receive_buf_len);
			}
			k_msleep(ECHO_SERVER_DEADTIME_MS);
		}

	} while (0);

	if (sock != -1) {
		if (close(sock) == -1) {
			LOG_ERR("close failed: (%d) %s", errno, strerror(errno));
		}
	}

	LOG_INF("app finished");
}

static struct {
	volatile bool connected;
	struct k_sem sem;
} app_data;

static void on_wifi_event(struct net_mgmt_event_callback *cb,
	uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		app_data.connected = true;
		k_sem_give(&app_data.sem);
		LOG_INF("connected");
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		app_data.connected = false;
		LOG_INF("disconnected");
		break;
	default:
		LOG_INF("unhandled network event %u", mgmt_event);
		break;
	}
}

int main(void)
{
	app_data.connected = false;
	(void) k_sem_init(&app_data.sem, 0, 1);

	static struct net_mgmt_event_callback wifi_cb;

	net_mgmt_init_event_callback(&wifi_cb, on_wifi_event,
		NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_cb);

	for (;;) {
		if (!app_data.connected) {
			struct wifi_connect_req_params connect_req_params = {
				.ssid = CONFIG_SAMPLE_WIFI_SSID,
				.ssid_length = strlen(CONFIG_SAMPLE_WIFI_SSID),
				.psk = CONFIG_SAMPLE_WIFI_PASSWORD,
				.psk_length = strlen(CONFIG_SAMPLE_WIFI_PASSWORD),
				.security = WIFI_SECURITY_TYPE_PSK
			};

			if (net_mgmt(NET_REQUEST_WIFI_CONNECT, net_if_get_default(),
				&connect_req_params, sizeof(connect_req_params))) {
				LOG_ERR("connection request failed\n");
			} else {
				LOG_INF("connecting...");
			}

			(void) k_sem_take(&app_data.sem, K_FOREVER);
		}
		if (app_data.connected) {
			data_exchange(&app_data.connected);
		}
	}

	return 0;
}
