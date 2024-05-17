/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

int main(void)
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

		for (bool error = false; !error; cnt++) {

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

			for (bool rx_done = false; !rx_done;) {
				socklen_t server_addr_len = sizeof(server_addr);
				static uint8_t receive_buf[sizeof(transmit_buf)];
				ssize_t receive_buf_len =
					recvfrom(sock, receive_buf, sizeof(receive_buf), 0,
						(struct sockaddr *)&server_addr, &server_addr_len);
				if (receive_buf_len == -1) {
					LOG_ERR("recvfrom failed: (%d) %s", errno, strerror(errno));
					rx_done = true;
					break;
				} else if (receive_buf_len == transmit_buf_len) {
					if (!memcmp(receive_buf, transmit_buf, transmit_buf_len)) {
						LOG_INF("all OK");
						rx_done = true;
					} else {
						LOG_ERR("transmit and receive mismatch");
					}
				} else {
					LOG_ERR("transmit and receive lengths mismatch (%u - %u)",
						(unsigned int)transmit_buf_len,
						(unsigned int)receive_buf_len);
				}
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

	return 0;
}
