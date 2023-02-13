/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/ipc/ipc_service.h>

#include "common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(remote, LOG_LEVEL_INF);


K_SEM_DEFINE(bound_sem, 0, 1);

static void ep_bound(void *priv)
{
	k_sem_give(&bound_sem);
	LOG_INF("Ep bounded");
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	struct data_packet *packet = (struct data_packet *)data;
	static unsigned char expected_message = 'a';
	static size_t expected_len = PACKET_SIZE_START;

	__ASSERT(packet->data[0] == expected_message, "Unexpected message. Expected %c, got %c",
		expected_message, packet->data[0]);
	__ASSERT(len == expected_len, "Unexpected length. Expected %zu, got %zu",
		expected_len, len);

	expected_message++;
	expected_len++;

	if (expected_message > 'z') {
		expected_message = 'a';
	}

	if (expected_len > sizeof(struct data_packet)) {
		expected_len = PACKET_SIZE_START;
	}
}

static int send_for_time(struct ipc_ept *ep, const int64_t sending_time_ms)
{
	struct data_packet msg = {.data[0] = 'A'};
	size_t mlen = PACKET_SIZE_START;
	size_t bytes_sent = 0;
	int ret = 0;

	LOG_INF("Perform sends for %lld [ms]", sending_time_ms);

	int64_t start = k_uptime_get();

	while ((k_uptime_get() - start) < sending_time_ms) {
		ret = ipc_service_send(ep, &msg, mlen);
		if (ret == -ENOMEM) {
			/* No space in the buffer. Retry. */
			continue;
		} else if (ret < 0) {
			LOG_ERR("Failed to send (%c) failed with ret %d", msg.data[0], ret);
			break;
		}

		msg.data[0]++;
		if (msg.data[0] > 'Z') {
			msg.data[0] = 'A';
		}

		bytes_sent += mlen;
		mlen++;

		if (mlen > sizeof(struct data_packet)) {
			mlen = PACKET_SIZE_START;
		}

		k_usleep(1);
	}

	LOG_INF("Sent %zu [Bytes] over %lld [ms]", bytes_sent, sending_time_ms);

	return ret;
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {
		.bound    = ep_bound,
		.received = ep_recv,
	},
};

int main(void)
{
	const struct device *ipc0_instance;
	struct ipc_ept ep;
	int ret;

	LOG_INF("IPC-service REMOTE demo started");

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("ipc_service_open_instance() failure");
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret != 0) {
		LOG_ERR("ipc_service_register_endpoint() failure");
		return ret;
	}

	k_sem_take(&bound_sem, K_FOREVER);

	ret = send_for_time(&ep, SENDING_TIME_MS);
	if (ret < 0) {
		LOG_ERR("send_for_time() failure");
		return ret;
	}

	LOG_INF("IPC-service REMOTE demo ended");

	return 0;
}
