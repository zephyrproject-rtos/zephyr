/** @file
 * @brief Modem context helper driver
 *
 * A modem context driver allowing application to handle all
 * aspects of received protocol data.
 */

/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_context, CONFIG_MODEM_LOG_LEVEL);

#include <zephyr/kernel.h>

#include "modem_context.h"

static struct modem_context *contexts[CONFIG_MODEM_CONTEXT_MAX_NUM];

int modem_context_sprint_ip_addr(const struct sockaddr *addr, char *buf, size_t buf_size)
{
	static const char unknown_str[] = "unk";

	if (addr->sa_family == AF_INET6) {
		if (buf_size < NET_IPV6_ADDR_LEN) {
			return -ENOMEM;
		}

		if (net_addr_ntop(AF_INET6, &net_sin6(addr)->sin6_addr,
					buf, buf_size) == NULL) {
			return -ENOMEM;
		}
		return 0;
	}

	if (addr->sa_family == AF_INET) {
		if (buf_size < NET_IPV4_ADDR_LEN) {
			return -ENOMEM;
		}
		if (net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr,
					buf, buf_size) == NULL) {
			return -ENOMEM;
		}
		return 0;
	}

	LOG_ERR("Unknown IP address family:%d", addr->sa_family);

	if (buf_size < sizeof(unknown_str)) {
		return -ENOMEM;
	}
	strcpy(buf, unknown_str);
	return 0;
}

int modem_context_get_addr_port(const struct sockaddr *addr, uint16_t *port)
{
	if (!addr || !port) {
		return -EINVAL;
	}

	if (addr->sa_family == AF_INET6) {
		*port = ntohs(net_sin6(addr)->sin6_port);
		return 0;
	} else if (addr->sa_family == AF_INET) {
		*port = ntohs(net_sin(addr)->sin_port);
		return 0;
	}

	return -EPROTONOSUPPORT;
}

/**
 * @brief  Finds modem context which owns the iface device.
 *
 * @param  *dev: device used by the modem iface.
 *
 * @retval Modem context or NULL.
 */
struct modem_context *modem_context_from_iface_dev(const struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		if (contexts[i] && contexts[i]->iface.dev == dev) {
			return contexts[i];
		}
	}

	return NULL;
}

/**
 * @brief  Assign a modem context if there is free space.
 *
 * @note   Amount of stored modem contexts is determined by
 *         CONFIG_MODEM_CONTEXT_MAX_NUM.
 *
 * @param  *ctx: modem context to persist.
 *
 * @retval 0 if ok, < 0 if error.
 */
static int modem_context_get(struct modem_context *ctx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(contexts); i++) {
		if (!contexts[i]) {
			contexts[i] = ctx;
			return 0;
		}
	}

	return -ENOMEM;
}

struct modem_context *modem_context_from_id(int id)
{
	if (id >= 0 && id < ARRAY_SIZE(contexts)) {
		return contexts[id];
	} else {
		return NULL;
	}
}

int modem_context_register(struct modem_context *ctx)
{
	if (!ctx) {
		return -EINVAL;
	}

	return modem_context_get(ctx);
}
