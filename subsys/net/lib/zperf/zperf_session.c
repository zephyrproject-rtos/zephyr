/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_zperf, CONFIG_NET_ZPERF_LOG_LEVEL);

#include <zephyr/kernel.h>

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/udp.h>

#include "zperf_session.h"

#define SESSION_MAX CONFIG_NET_ZPERF_MAX_SESSIONS

static struct session sessions[SESSION_PROTO_END][SESSION_MAX];

/* Get session from a given packet */
struct session *get_session(const struct sockaddr *addr,
			    enum session_proto proto)
{
	struct session *active = NULL;
	struct session *free = NULL;
	int i = 0;
	const struct sockaddr_in *addr4 = (const struct sockaddr_in *)addr;
	const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6 *)addr;

	if (proto != SESSION_TCP && proto != SESSION_UDP) {
		NET_ERR("Error! unsupported proto.\n");
		return NULL;
	}

	/* Check whether we already have an active session */
	while (!active && i < SESSION_MAX) {
		struct session *ptr = &sessions[proto][i];

		if (IS_ENABLED(CONFIG_NET_IPV4) &&
		    addr->sa_family == AF_INET &&
		    ptr->ip.family == AF_INET &&
		    ptr->port == addr4->sin_port &&
		    net_ipv4_addr_cmp(&ptr->ip.in_addr, &addr4->sin_addr)) {
			/* We found an active session */
			active = ptr;
			break;
		}

		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    addr->sa_family == AF_INET6 &&
		    ptr->ip.family == AF_INET6 &&
		    ptr->port == addr6->sin6_port &&
		    net_ipv6_addr_cmp(&ptr->ip.in6_addr, &addr6->sin6_addr)) {
			/* We found an active session */
			active = ptr;
			break;
		}

		if (!free && (ptr->state == STATE_NULL ||
			      ptr->state == STATE_COMPLETED)) {
			/* We found a free slot - just in case */
			free = ptr;
		}

		i++;
	}

	/* If no active session then create a new one */
	if (!active && free) {
		active = free;

		if (IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == AF_INET) {
			active->port = addr4->sin_port;
			active->ip.family = AF_INET;
			net_ipaddr_copy(&active->ip.in_addr, &addr4->sin_addr);
		} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
			   addr->sa_family == AF_INET6) {
			active->port = addr6->sin6_port;
			active->ip.family = AF_INET6;
			net_ipaddr_copy(&active->ip.in6_addr, &addr6->sin6_addr);
		}
	}

	return active;
}

void zperf_reset_session_stats(struct session *session)
{
	if (!session) {
		return;
	}

	session->counter = 0U;
	session->start_time = 0U;
	session->next_id = 1U;
	session->length = 0U;
	session->outorder = 0U;
	session->error = 0U;
	session->jitter = 0;
	session->last_transit_time = 0;
}

void zperf_session_reset(enum session_proto proto)
{
	int i, j;

	if (proto >= SESSION_PROTO_END) {
		return;
	}

	i = (int)proto;

	for (j = 0; j < SESSION_MAX; j++) {
		sessions[i][j].state = STATE_NULL;
		zperf_reset_session_stats(&(sessions[i][j]));
	}
}

void zperf_session_init(void)
{
	int i;

	for (i = 0; i < SESSION_PROTO_END; i++) {
		zperf_session_reset(i);
	}
}
