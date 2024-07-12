/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/wifi_mgmt.h>

#include <zephyr/posix/unistd.h>
#include <zephyr/posix/netdb.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/arpa/inet.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sock_app, LOG_LEVEL_INF);

#define ECHO_SERVER_IP                      CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#define ECHO_SERVER_IPV6                    CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#define ECHO_SERVER_PORT                    2024
#define ECHO_SERVER_TIMEOUT_MS              1000
#define ECHO_SERVER_UDP_BUF_SIZE            1472
#define ECHO_SERVER_UDP_IPV6_BUF_SIZE       1452
#define ECHO_SERVER_TCP_BUF_SIZE            2048
#define ECHO_SERVER_TCP_IPV6_BUF_SIZE       2048
#define ECHO_SERVER_DEADTIME_MS             1000

#if CONFIG_APP_SOCKET_UDP
static void udp_data_exchange(volatile bool *connected)
{
	LOG_INF("app started");

	int serv_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	do {
		if (serv_sock < 0) {
			LOG_ERR("socket failed: (%d) %s", errno, strerror(errno));
			break;
		}

		struct sockaddr_in me_addr = {
			.sin_family = AF_INET,
			.sin_port = htons(ECHO_SERVER_PORT),
			.sin_addr.s_addr = htonl(INADDR_ANY)
		};

		if (bind(serv_sock, (const struct sockaddr *)&me_addr,
			sizeof(me_addr)) < 0) {
			LOG_ERR("bind: (%d) %s", errno, strerror(errno));
		}

		struct timeval tv = {
			.tv_sec = ECHO_SERVER_TIMEOUT_MS / 1000,
			.tv_usec = (ECHO_SERVER_TIMEOUT_MS % 1000) * 1000
		};

		if (setsockopt(serv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
			LOG_ERR("setsockopt failed: (%d) %s", errno, strerror(errno));
			break;
		}

		uint8_t cnt = 0;

		for (bool error = false; !error && *connected; cnt++) {

			struct sockaddr_in server_addr = {
				.sin_family = AF_INET,
				.sin_port = htons(ECHO_SERVER_PORT),
			};

			if (inet_pton(AF_INET, ECHO_SERVER_IP, &server_addr.sin_addr) < 0) {
				LOG_ERR("inet_pton failed");
				error = true;
				continue;
			}

			/* flush socket */
			uint8_t trash;

			while (recv(serv_sock, &trash, sizeof(trash), MSG_DONTWAIT) > 0) {
			}

			static uint8_t tx_buf[ECHO_SERVER_UDP_BUF_SIZE];
			size_t tx_len = 0;

			for (size_t i = 0; i < sizeof(tx_buf); i++) {
				tx_buf[i] = cnt + i;
			}

			while (tx_len < sizeof(tx_buf)) {
				ssize_t l = sendto(serv_sock,
					&tx_buf[tx_len], sizeof(tx_buf) - tx_len, 0,
					(struct sockaddr *)&server_addr, sizeof(server_addr));
				if (l < 0) {
					LOG_ERR("sendto failed: (%d) %s", errno, strerror(errno));
					error = true;
					break;
				}
				tx_len += l;
			}

			if (error) {
				continue;
			}

			LOG_INF("sent message to: %s:%u",
				ECHO_SERVER_IP, ntohs(server_addr.sin_port));

			static uint8_t rx_buf[sizeof(tx_buf)];
			ssize_t rx_len = 0;

			while (rx_len < sizeof(rx_buf)) {
				struct sockaddr_in client_addr;
				socklen_t client_addr_len = sizeof(client_addr);

				ssize_t l = recvfrom(serv_sock,
					&rx_buf[rx_len], sizeof(rx_buf) - rx_len, 0,
					(struct sockaddr *)&client_addr, &client_addr_len);
				if (l < 0) {
					LOG_ERR("recvfrom failed: (%d) %s", errno, strerror(errno));
					break;
				}

				if (server_addr.sin_addr.s_addr == client_addr.sin_addr.s_addr) {
					rx_len += l;
				}
			}

			if (error) {
				continue;
			}

			if (rx_len == tx_len) {
				if (!memcmp(rx_buf, tx_buf, tx_len)) {
					LOG_INF("all OK");
				} else {
					LOG_ERR("transmit and receive mismatch");
				}
			} else {
				LOG_ERR("transmit and receive lengths mismatch (%u - %u)",
					(unsigned int)tx_len,
					(unsigned int)rx_len);
			}
			k_msleep(ECHO_SERVER_DEADTIME_MS);
		}

	} while (0);

	if (serv_sock >= -0) {
		if (close(serv_sock) < 0) {
			LOG_ERR("close failed: (%d) %s", errno, strerror(errno));
		}
	}

	LOG_INF("app finished");
}

static void (*data_exchange)(volatile bool *connected) = udp_data_exchange;

#endif /* CONFIG_APP_SOCKET_UDP */

#if CONFIG_APP_SOCKET_UDP_IPV6
static void udp_ipv6_data_exchange(volatile bool *connected)
{
	LOG_INF("app started");

	int serv_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

	do {
		if (serv_sock < 0) {
			LOG_ERR("socket failed: (%d) %s", errno, strerror(errno));
			break;
		}

		struct sockaddr_in6 me_addr = {
			.sin6_family = AF_INET6,
			.sin6_port = htons(ECHO_SERVER_PORT),
			.sin6_addr = in6addr_any
		};

		if (bind(serv_sock, (const struct sockaddr *)&me_addr,
			sizeof(me_addr)) < 0) {
			LOG_ERR("bind: (%d) %s", errno, strerror(errno));
		}

		struct timeval tv = {
			.tv_sec = ECHO_SERVER_TIMEOUT_MS / 1000,
			.tv_usec = (ECHO_SERVER_TIMEOUT_MS % 1000) * 1000
		};

		if (setsockopt(serv_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
			LOG_ERR("setsockopt failed: (%d) %s", errno, strerror(errno));
			break;
		}

		uint8_t cnt = 0;

		for (bool error = false; !error && *connected; cnt++) {

			struct sockaddr_in6 server_addr = {
				.sin6_family = AF_INET6,
				.sin6_port = htons(ECHO_SERVER_PORT),
			};

			if (inet_pton(AF_INET6, ECHO_SERVER_IPV6, &server_addr.sin6_addr) < 0) {
				LOG_ERR("inet_pton failed");
				error = true;
				continue;
			}

			/* flush socket */
			uint8_t trash;

			while (recv(serv_sock, &trash, sizeof(trash), MSG_DONTWAIT) > 0) {
			}

			static uint8_t tx_buf[ECHO_SERVER_UDP_IPV6_BUF_SIZE];
			size_t tx_len = 0;

			for (size_t i = 0; i < sizeof(tx_buf); i++) {
				tx_buf[i] = cnt + i;
			}

			while (tx_len < sizeof(tx_buf)) {
				ssize_t l = sendto(serv_sock,
					&tx_buf[tx_len], sizeof(tx_buf) - tx_len, 0,
					(struct sockaddr *)&server_addr, sizeof(server_addr));
				if (l < 0) {
					LOG_ERR("sendto failed: (%d) %s", errno, strerror(errno));
					error = true;
					break;
				}
				tx_len += l;
			}

			if (error) {
				continue;
			}

			LOG_INF("sent message to: %s:%u",
				ECHO_SERVER_IPV6, ntohs(server_addr.sin6_port));

			static uint8_t rx_buf[sizeof(tx_buf)];
			ssize_t rx_len = 0;

			while (rx_len < sizeof(rx_buf)) {
				struct sockaddr_in6 client_addr;
				socklen_t client_addr_len = sizeof(client_addr);

				ssize_t l = recvfrom(serv_sock,
					&rx_buf[rx_len], sizeof(rx_buf) - rx_len, 0,
					(struct sockaddr *)&client_addr, &client_addr_len);
				if (l < 0) {
					LOG_ERR("recvfrom failed: (%d) %s", errno, strerror(errno));
					break;
				}

				if (!memcmp(&server_addr.sin6_addr, &client_addr.sin6_addr,
					sizeof(struct in6_addr))) {
					rx_len += l;
				}
			}

			if (error) {
				continue;
			}

			if (rx_len == tx_len) {
				if (!memcmp(rx_buf, tx_buf, tx_len)) {
					LOG_INF("all OK");
				} else {
					LOG_ERR("transmit and receive mismatch");
				}
			} else {
				LOG_ERR("transmit and receive lengths mismatch (%u - %u)",
					(unsigned int)tx_len,
					(unsigned int)rx_len);
			}
			k_msleep(ECHO_SERVER_DEADTIME_MS);
		}

	} while (0);

	if (serv_sock >= -0) {
		if (close(serv_sock) < 0) {
			LOG_ERR("close failed: (%d) %s", errno, strerror(errno));
		}
	}

	LOG_INF("app finished");
}

static void (*data_exchange)(volatile bool *connected) = udp_ipv6_data_exchange;

#endif /* CONFIG_APP_SOCKET_UDP_IPV6 */

#if CONFIG_APP_SOCKET_TCP
static void tcp_data_exchange(volatile bool *connected)
{
	LOG_INF("app started");

	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(ECHO_SERVER_PORT)
	};

	if (inet_pton(AF_INET, ECHO_SERVER_IP, &server_addr.sin_addr) < 0) {
		LOG_ERR("inet_pton failed");
		return;
	}

	uint8_t cnt = 0;

	for (bool error = false; !error && *connected; cnt++) {
		int serv_sock = -1;

		do {
			serv_sock = socket(AF_INET, SOCK_STREAM, 0);
			if (serv_sock < 0) {
				LOG_ERR("socket failed: (%d) %s", errno, strerror(errno));
				error = true;
				break;
			}

			if (connect(serv_sock,
				(struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
				LOG_ERR("connect failed: (%d) %s", errno, strerror(errno));
				error = true;
				break;
			}

			static uint8_t tx_buf[ECHO_SERVER_TCP_BUF_SIZE];
			size_t tx_len = 0;

			for (size_t i = 0; i < sizeof(tx_buf); i++) {
				tx_buf[i] = cnt + i;
			}

			while (tx_len < sizeof(tx_buf)) {
				ssize_t l = write(serv_sock,
					&tx_buf[tx_len], sizeof(tx_buf) - tx_len);

				if (l < 0) {
					LOG_ERR("write: (%d) %s", errno, strerror(errno));
					break;
				}
				tx_len += l;
			}

			if (tx_len != sizeof(tx_buf)) {
				error = true;
				break;
			}

			static uint8_t rx_buf[sizeof(tx_buf)];
			size_t rx_len = 0;

			while (rx_len < sizeof(rx_buf)) {
				ssize_t l = read(serv_sock,
					&rx_buf[rx_len], sizeof(rx_buf) - rx_len);

				if (l < 0) {
					LOG_ERR("read: (%d) %s", errno, strerror(errno));
					break;
				}
				rx_len += l;
			}

			if (rx_len == tx_len) {
				if (!memcmp(rx_buf, tx_buf, tx_len)) {
					LOG_INF("all OK");
				} else {
					LOG_ERR("transmit and receive mismatch");
				}
			} else {
				LOG_ERR("transmit and receive lengths mismatch (%u - %u)",
					(unsigned int)tx_len,
					(unsigned int)rx_len);
			}

			k_msleep(ECHO_SERVER_DEADTIME_MS);
		} while (0);

		if (serv_sock >= -0) {
			if (close(serv_sock) < 0) {
				error = true;
				LOG_ERR("close failed: (%d) %s", errno, strerror(errno));
			}
		}
	}

	LOG_INF("app finished");
}

static void (*data_exchange)(volatile bool *connected) = tcp_data_exchange;

#endif /* CONFIG_APP_SOCKET_TCP */

#if CONFIG_APP_SOCKET_TCP_IPV6
static void tcp_ipv6_data_exchange(volatile bool *connected)
{
	LOG_INF("app started");

	struct sockaddr_in6 server_addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(ECHO_SERVER_PORT)
	};

	if (inet_pton(AF_INET6, ECHO_SERVER_IPV6, &server_addr.sin6_addr) < 0) {
		LOG_ERR("inet_pton failed");
		return;
	}

	uint8_t cnt = 0;

	for (bool error = false; !error && *connected; cnt++) {
		int serv_sock = -1;

		do {
			serv_sock = socket(AF_INET6, SOCK_STREAM, 0);
			if (serv_sock < 0) {
				LOG_ERR("socket failed: (%d) %s", errno, strerror(errno));
				error = true;
				break;
			}

			if (connect(serv_sock,
				(struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
				LOG_ERR("connect failed: (%d) %s", errno, strerror(errno));
				error = true;
				break;
			}

			static uint8_t tx_buf[ECHO_SERVER_TCP_IPV6_BUF_SIZE];
			size_t tx_len = 0;

			for (size_t i = 0; i < sizeof(tx_buf); i++) {
				tx_buf[i] = cnt + i;
			}

			while (tx_len < sizeof(tx_buf)) {
				ssize_t l = write(serv_sock,
					&tx_buf[tx_len], sizeof(tx_buf) - tx_len);

				if (l < 0) {
					LOG_ERR("write: (%d) %s", errno, strerror(errno));
					break;
				}
				tx_len += l;
			}

			if (tx_len != sizeof(tx_buf)) {
				error = true;
				break;
			}

			static uint8_t rx_buf[sizeof(tx_buf)];
			size_t rx_len = 0;

			while (rx_len < sizeof(rx_buf)) {
				ssize_t l = read(serv_sock,
					&rx_buf[rx_len], sizeof(rx_buf) - rx_len);

				if (l < 0) {
					LOG_ERR("read: (%d) %s", errno, strerror(errno));
					break;
				}
				rx_len += l;
			}

			if (rx_len == tx_len) {
				if (!memcmp(rx_buf, tx_buf, tx_len)) {
					LOG_INF("all OK");
				} else {
					LOG_ERR("transmit and receive mismatch");
				}
			} else {
				LOG_ERR("transmit and receive lengths mismatch (%u - %u)",
					(unsigned int)tx_len,
					(unsigned int)rx_len);
			}

			k_msleep(ECHO_SERVER_DEADTIME_MS);
		} while (0);

		if (serv_sock >= -0) {
			if (close(serv_sock) < 0) {
				error = true;
				LOG_ERR("close failed: (%d) %s", errno, strerror(errno));
			}
		}
	}

	LOG_INF("app finished");
}

static void (*data_exchange)(volatile bool *connected) = tcp_ipv6_data_exchange;

#endif /* CONFIG_APP_SOCKET_TCP_IPV6 */

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
