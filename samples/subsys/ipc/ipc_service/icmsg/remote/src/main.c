/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>

#include <zephyr/ipc/ipc_service.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(remote, LOG_LEVEL_INF);

#define MLEN_0 40

struct data_packet {
	unsigned char message;
	unsigned char data[100];
};


K_SEM_DEFINE(bound_sem, 0, 1);

static void ep_bound(void *priv)
{
	k_sem_give(&bound_sem);
	LOG_INF("Ep bounded");
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	char received_data = *((char *)data);
	static char expected_message = 'a';
	static uint16_t expected_len = MLEN_0;

	static unsigned long long cnt;
	static unsigned int stats_every;
	static uint32_t start;
	static unsigned int err;

	if (start == 0) {
		start = k_uptime_get_32();
	}

	if (received_data != expected_message) {
		err++;
	}

	if (len != expected_len) {
		err++;
	}

	expected_message++;
	expected_len++;

	cnt += len;

	if (expected_message > 'z') {
		if (stats_every++ > 50) {
			/* Print throuhput [Bytes/s]. Use printk not to overload CPU with logger.
			 * Sample never reaches lower priority thread because of high throughput
			 * (100% cpu load) so logging would not be able to handle messages in
			 * deferred mode (immediate mode would be heavier than printk).
			 */
			printk("%llu\n", (1000*cnt)/(k_uptime_get_32() - start));
			stats_every = 0;
		}
		if (err) {
			printk("Unexpected message\n");
		}
		expected_message = 'a';
	}

	if (expected_len > sizeof(struct data_packet)) {
		expected_len = MLEN_0;
	}
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
	struct data_packet msg = {.message = 'A'};
	struct ipc_ept ep;
	int ret;

	LOG_INF("IPC-service REMOTE demo started");

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_INF("ipc_service_open_instance() failure");
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret < 0) {
		printf("ipc_service_register_endpoint() failure");
		return ret;
	}

	k_sem_take(&bound_sem, K_FOREVER);

	uint16_t mlen = MLEN_0;

	while (true) {
		ret = ipc_service_send(&ep, &msg, mlen);
		if (ret == -ENOMEM) {
			/* No space in the buffer. Retry. */
			continue;
		} else if (ret < 0) {
			LOG_ERR("send_message(%d) failed with ret %d", msg.message, ret);
			break;
		}

		msg.message++;
		if (msg.message > 'Z') {
			msg.message = 'A';
		}

		mlen++;
		if (mlen > sizeof(struct data_packet)) {
			mlen = MLEN_0;
		}

		/* Quasi minimal busy wait time which allows to continuosly send
		 * data without -ENOMEM error code. The purpose is to test max
		 * throughput. Determined experimentally.
		 */
		k_busy_wait(50);
	}

	LOG_INF("IPC-service REMOTE demo ended.");

	return 0;
}
