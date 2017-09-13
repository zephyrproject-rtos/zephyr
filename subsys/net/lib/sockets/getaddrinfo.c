/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_SOCKETS)
#define SYS_LOG_DOMAIN "net/sock"
#define NET_LOG_ENABLED 1
#endif

/* libc headers */
#include <stdlib.h>

/* Zephyr headers */
#include <kernel.h>
#include <net/socket.h>

struct getaddrinfo_state {
	const struct zsock_addrinfo *hints;
	struct k_sem sem;
	int status;
	int idx;
};

static struct zsock_addrinfo ai_arr[2];
static struct getaddrinfo_state ai_state;

static void dns_resolve_cb(enum dns_resolve_status status,
			   struct dns_addrinfo *info, void *user_data)
{
	struct getaddrinfo_state *state = user_data;
	struct zsock_addrinfo *ai = ai_arr + state->idx;
	int socktype = SOCK_STREAM;
	int proto;

	NET_DBG("dns status: %d", status);

	if (info == NULL) {
		if (status == DNS_EAI_ALLDONE) {
			status = 0;
		}
		state->status = status;
		k_sem_give(&state->sem);
		return;
	}

	memcpy(&ai->_ai_addr, &info->ai_addr, info->ai_addrlen);
	ai->ai_addr = &ai->_ai_addr;
	ai->ai_addrlen = info->ai_addrlen;
	memcpy(&ai->_ai_canonname, &info->ai_canonname,
	       sizeof(ai->_ai_canonname));
	ai->ai_canonname = ai->_ai_canonname;
	ai->ai_family = info->ai_family;

	if (state->hints) {
		if (state->hints->ai_socktype) {
			socktype = state->hints->ai_socktype;
		}
	}

	proto = IPPROTO_TCP;
	if (socktype == SOCK_DGRAM) {
		proto = IPPROTO_UDP;
	}

	ai->ai_socktype = socktype;
	ai->ai_protocol = proto;

	state->idx++;
}


int zsock_getaddrinfo(const char *host, const char *service,
		      const struct zsock_addrinfo *hints,
		      struct zsock_addrinfo **res)
{
	int family = AF_UNSPEC;
	long int port;
	int st1 = DNS_EAI_ADDRFAMILY, st2 = DNS_EAI_ADDRFAMILY;

	if (hints) {
		family = hints->ai_family;
	}

	port = strtol(service, NULL, 10);
	if (port < 1 || port > 65535) {
		return DNS_EAI_NONAME;
	}

	ai_state.hints = hints;
	ai_state.idx = 0;
	k_sem_init(&ai_state.sem, 0, UINT_MAX);

	/* Link entries in advance */
	ai_arr[0].ai_next = &ai_arr[1];

	/* Execute if AF_UNSPEC or AF_INET4 */
	if (family != AF_INET6) {
		dns_get_addr_info(host, DNS_QUERY_TYPE_A, NULL,
				  dns_resolve_cb, &ai_state, 1000);
		k_sem_take(&ai_state.sem, K_FOREVER);
		net_sin(&ai_arr[ai_state.idx - 1]._ai_addr)->sin_port =
								htons(port);
		st1 = ai_state.status;
	}

	/* Execute if AF_UNSPEC or AF_INET6 */
	if (family != AF_INET) {
		dns_get_addr_info(host, DNS_QUERY_TYPE_AAAA, NULL,
				  dns_resolve_cb, &ai_state, 1000);
		k_sem_take(&ai_state.sem, K_FOREVER);
		net_sin6(&ai_arr[ai_state.idx - 1]._ai_addr)->sin6_port =
								htons(port);
		st2 = ai_state.status;
	}

	/* If both attempts failed, it's error */
	if (st1 && st2) {
		if (st1 != DNS_EAI_ADDRFAMILY) {
			return st1;
		}
		return st2;
	}

	/* Mark entry as last */
	ai_arr[ai_state.idx - 1].ai_next = NULL;

	*res = ai_arr;

	return 0;
}
