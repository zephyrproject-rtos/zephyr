/*
 * Copyright (c) 2021 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <net/socket.h>
#include <drivers/modem/ublox-sara-n310.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#define N310_DEVICE_LABEL DT_LABEL(DT_INST(0, ublox_sara_n310))
#define MAX_BUF 512

void main(void)
{
	static const struct device *dev_ubloxn3;
	struct sockaddr_in addr;

	/* check if device exists in device tree */
	dev_ubloxn3 = device_get_binding(N310_DEVICE_LABEL);

	if (dev_ubloxn3 == NULL) {
		LOG_ERR("Failed to get device binding.");
		return;
	}

	LOG_INF("Manufacturer: %s", log_strdup(n310_get_manufacturer()));
	LOG_INF("Model: %s", log_strdup(n310_get_model()));
	LOG_INF("Revision: %s", log_strdup(n310_get_revision()));
	LOG_INF("ICCID: %s", log_strdup(n310_get_iccid()));
	LOG_INF("IMEI: %s", log_strdup(n310_get_imei()));
	LOG_INF("IP: %s", log_strdup(n310_get_ip()));
	LOG_INF("Network state: %d", n310_get_state());

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(4242);
	inet_pton(AF_INET, "192.168.0.20", &addr.sin_addr);

	while (1) {
		char *buffer = "ABDD";
		char recvBuffer[MAX_BUF] = "\0";
		int sockfd = 0, n = 0;

		sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (sockfd < 0) {
			LOG_ERR("Could not create socket. Return value: %d", sockfd);
		}

		n = connect(sockfd, (const struct sockaddr *)&addr, sizeof(addr));
		if (n < 0) {
			LOG_ERR("Bind failed with return value: %d", n);
		}

		n = sendto(sockfd, buffer, strlen(buffer) / 2,
			   RELEASE_AFTER_FIRST_DOWNLINK,
			   (const struct sockaddr *)&addr, sizeof(addr));
		if (n < 0) {
			LOG_ERR("Send failed with return value: %d", n);
		}

		k_msleep(2000);

		n = recvfrom(sockfd, recvBuffer, MAX_BUF, ZSOCK_MSG_DONTWAIT, NULL, NULL);
		if (n <= 0) {
			strcpy(recvBuffer, "No data in buffer.");
		}

		LOG_INF("Received: %s", log_strdup(recvBuffer));

		k_msleep(4000);
		close(sockfd);
		k_msleep(4000);

		n310_modem_reset();
	}
}
