/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_sntp_client_sample, LOG_LEVEL_DBG);

#include <net/sntp.h>

#define SNTP_PORT 123

void main(void)
{
	struct sntp_ctx ctx;
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;
	u64_t epoch_time;
	int rv;

	/* ipv4 */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SNTP_PORT);
	inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
		  &addr.sin_addr);

	rv = sntp_init(&ctx, (struct sockaddr *) &addr,
		       sizeof(struct sockaddr_in));
	if (rv < 0) {
		LOG_ERR("Failed to init sntp ctx: %d", rv);
		goto end;
	}

	rv = sntp_request(&ctx, K_FOREVER, &epoch_time);
	if (rv < 0) {
		LOG_ERR("Failed to send sntp request: %d", rv);
		goto end;
	}
	LOG_DBG("time: %lld", epoch_time);
	LOG_DBG("status: %d", rv);

	sntp_close(&ctx);

	/* ipv6 */
	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(SNTP_PORT);
	inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
		  &addr6.sin6_addr);

	rv = sntp_init(&ctx, (struct sockaddr *) &addr6,
		       sizeof(struct sockaddr_in6));
	if (rv < 0) {
		LOG_ERR("Failed to init sntp ctx: %d", rv);
		goto end;
	}

	rv = sntp_request(&ctx, K_NO_WAIT, &epoch_time);
	if (rv < 0) {
		LOG_ERR("Failed to send sntp request: %d", rv);
		goto end;
	}

	LOG_DBG("time: %lld", epoch_time);
	LOG_DBG("status: %d", rv);

end:
	sntp_close(&ctx);
}
