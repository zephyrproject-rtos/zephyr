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
#include <syscall_handler.h>

#define AI_ARR_MAX	2

#if defined(CONFIG_DNS_RESOLVER)

struct getaddrinfo_state {
	const struct zsock_addrinfo *hints;
	struct k_sem sem;
	int status;
	u16_t idx;
	u16_t port;
	struct zsock_addrinfo *ai_arr;
};

static void dns_resolve_cb(enum dns_resolve_status status,
			   struct dns_addrinfo *info, void *user_data)
{
	struct getaddrinfo_state *state = user_data;
	struct zsock_addrinfo *ai;
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

	if (state->idx >= AI_ARR_MAX) {
		NET_DBG("getaddrinfo entries overflow");
		return;
	}

	ai = &state->ai_arr[state->idx];
	memcpy(&ai->_ai_addr, &info->ai_addr, info->ai_addrlen);
	net_sin(&ai->_ai_addr)->sin_port = state->port;
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


int _impl_z_zsock_getaddrinfo_internal(const char *host, const char *service,
				       const struct zsock_addrinfo *hints,
				       struct zsock_addrinfo *res)
{
	int family = AF_UNSPEC;
	long int port = 0;
	int st1 = DNS_EAI_ADDRFAMILY, st2 = DNS_EAI_ADDRFAMILY;
	struct sockaddr *ai_addr;
	int ret;
	struct getaddrinfo_state ai_state;

	if (hints) {
		family = hints->ai_family;
	}

	if (service) {
		port = strtol(service, NULL, 10);
		if (port < 1 || port > 65535) {
			return DNS_EAI_NONAME;
		}
	}

	ai_state.hints = hints;
	ai_state.idx = 0;
	ai_state.port = htons(port);
	ai_state.ai_arr = res;
	k_sem_init(&ai_state.sem, 0, UINT_MAX);

	/* Link entries in advance */
	ai_state.ai_arr[0].ai_next = &ai_state.ai_arr[1];

	/* Execute if AF_UNSPEC or AF_INET4 */
	if (family != AF_INET6) {
		ret = dns_get_addr_info(host, DNS_QUERY_TYPE_A, NULL,
					dns_resolve_cb, &ai_state, 1000);
		if (ret == 0) {
			k_sem_take(&ai_state.sem, K_FOREVER);
			st1 = ai_state.status;
		} else {
			errno = -ret;
			st1 = DNS_EAI_SYSTEM;
		}

		if (ai_state.idx > 0) {
			ai_addr = &ai_state.ai_arr[ai_state.idx - 1]._ai_addr;
			net_sin(ai_addr)->sin_port = htons(port);
		}
	}

#if defined(CONFIG_NET_IPV6)
	/* Execute if AF_UNSPEC or AF_INET6 */
	if (family != AF_INET) {
		ret = dns_get_addr_info(host, DNS_QUERY_TYPE_AAAA, NULL,
					dns_resolve_cb, &ai_state, 1000);
		if (ret == 0) {
			k_sem_take(&ai_state.sem, K_FOREVER);
			st2 = ai_state.status;
		} else {
			errno = -ret;
			st2 = DNS_EAI_SYSTEM;
		}

		if (ai_state.idx > 0) {
			ai_addr = &ai_state.ai_arr[ai_state.idx - 1]._ai_addr;
			net_sin6(ai_addr)->sin6_port = htons(port);
		}
	}
#endif

	/* If both attempts failed, it's error */
	if (st1 && st2) {
		if (st1 != DNS_EAI_ADDRFAMILY) {
			return st1;
		}
		return st2;
	}

	/* Mark entry as last */
	ai_state.ai_arr[ai_state.idx - 1].ai_next = NULL;

	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(z_zsock_getaddrinfo_internal, host, service, hints, res)
{
	struct zsock_addrinfo hints_copy;
	char *host_copy = NULL, *service_copy = NULL;
	u32_t ret;

	if (hints) {
		Z_OOPS(z_user_from_copy(&hints_copy, (void *)hints,
					sizeof(hints_copy)));
	}
	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_WRITE(res, AI_ARR_MAX,
					    sizeof(struct zsock_addrinfo)));

	if (service) {
		service_copy = z_user_string_alloc_copy((char *)service, 64);
		if (!service_copy) {
			ret = DNS_EAI_MEMORY;
			goto out;
		}
	}

	if (host) {
		host_copy = z_user_string_alloc_copy((char *)host, 64);
		if (!host_copy) {
			ret = DNS_EAI_MEMORY;
			goto out;
		}
	}

	ret = _impl_z_zsock_getaddrinfo_internal(host_copy, service_copy,
						 hints ? &hints_copy : NULL,
						 (struct zsock_addrinfo *)res);
out:
	k_free(service_copy);
	k_free(host_copy);

	return ret;
}
#endif /* CONFIG_USERSPACE */

int zsock_getaddrinfo(const char *host, const char *service,
		      const struct zsock_addrinfo *hints,
		      struct zsock_addrinfo **res)
{
	int ret;

	*res = calloc(AI_ARR_MAX, sizeof(struct zsock_addrinfo));
	if (!(*res)) {
		return DNS_EAI_MEMORY;
	}
	ret = z_zsock_getaddrinfo_internal(host, service, hints, *res);
	if (ret) {
		free(*res);
		*res = NULL;
	}
	return ret;
}

#endif
