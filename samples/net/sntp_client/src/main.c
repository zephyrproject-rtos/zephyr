/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <net/sntp.h>

#define SNTP_PORT 123

struct k_sem sem;

void resp_callback(struct sntp_ctx *ctx,
		   int status,
		   u64_t epoch_time,
		   void *user_data)
{
	printk("time: %lld\n", epoch_time);
	printk("status: %d\n", status);
	printk("user_data: %p\n", user_data);

	k_sem_give(&sem);
}

void main(void)
{
	struct sntp_ctx ctx;
	int rv;

	k_sem_init(&sem, 0, 1);

	/* ipv4 */
	rv = sntp_init(&ctx,
		       CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
		       SNTP_PORT,
		       K_FOREVER);
	if (rv < 0) {
		printk("Failed to init sntp ctx: %d\n", rv);
		return;
	}

	rv = sntp_request(&ctx, K_FOREVER, resp_callback, NULL);
	if (rv < 0) {
		printk("Failed to send sntp request: %d\n", rv);
		return;
	}

	k_sem_take(&sem, K_FOREVER);
	sntp_close(&ctx);

	/* ipv6 */
	rv = sntp_init(&ctx,
		       CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
		       SNTP_PORT,
		       K_NO_WAIT);
	if (rv < 0) {
		printk("Failed to initi sntp ctx: %d\n", rv);
		return;
	}

	rv = sntp_request(&ctx, K_NO_WAIT, resp_callback, NULL);
	if (rv < 0) {
		printk("Failed to send sntp request: %d\n", rv);
		return;
	}

	k_sem_take(&sem, K_FOREVER);
	sntp_close(&ctx);
}
