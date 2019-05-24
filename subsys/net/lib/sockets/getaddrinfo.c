/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* libc headers */
#include <stdlib.h>

/* Zephyr headers */
#include <logging/log.h>
LOG_MODULE_REGISTER(net_sock_addr, CONFIG_NET_SOCKETS_LOG_LEVEL);

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

/* Initialize static fields of addrinfo structure. A macro to let it work
 * with any sockaddr_* type.
 */
#define INIT_ADDRINFO(addrinfo, sockaddr) { \
		(addrinfo)->ai_addr = &(addrinfo)->_ai_addr; \
		(addrinfo)->ai_addrlen = sizeof(*sockaddr); \
		(addrinfo)->ai_canonname = (addrinfo)->_ai_canonname; \
		(addrinfo)->_ai_canonname[0] = '\0'; \
		(addrinfo)->ai_next = NULL; \
	}

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

static int exec_query(const char *host, int family,
		      struct getaddrinfo_state *ai_state)
{
	enum dns_query_type qtype = DNS_QUERY_TYPE_A;

	if (IS_ENABLED(CONFIG_NET_IPV6) && family != AF_INET) {
		qtype = DNS_QUERY_TYPE_AAAA;
	}

	return dns_get_addr_info(host, qtype, NULL,
				 dns_resolve_cb, ai_state,
				 CONFIG_NET_SOCKETS_DNS_TIMEOUT);
}

static int getaddrinfo_null_host(int port, const struct zsock_addrinfo *hints,
				struct zsock_addrinfo *res)
{
	if (hints && (hints->ai_flags & AI_PASSIVE)) {
		struct sockaddr_in *addr =
		    (struct sockaddr_in *)&res->_ai_addr;
		addr->sin_addr.s_addr = INADDR_ANY;
		addr->sin_port = htons(port);
		addr->sin_family = AF_INET;
		INIT_ADDRINFO(res, addr);
		res->ai_family = AF_INET;
		res->ai_socktype = SOCK_STREAM;
		res->ai_protocol = IPPROTO_TCP;
		return 0;
	}

	return DNS_EAI_FAIL;
}

int z_impl_z_zsock_getaddrinfo_internal(const char *host, const char *service,
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

	if (host == NULL) {
		/* Per POSIX, both can't be NULL. */
		if (service == NULL) {
			errno = EINVAL;
			return DNS_EAI_SYSTEM;
		}

		return getaddrinfo_null_host(port, hints, res);
	}

	ai_state.hints = hints;
	ai_state.idx = 0U;
	ai_state.port = htons(port);
	ai_state.ai_arr = res;
	k_sem_init(&ai_state.sem, 0, UINT_MAX);

	/* Link entries in advance */
	ai_state.ai_arr[0].ai_next = &ai_state.ai_arr[1];

	ret = exec_query(host, family, &ai_state);
	if (ret == 0) {
		/* If the DNS query for reason fails so that the
		 * dns_resolve_cb() would not be called, then we want the
		 * semaphore to timeout so that we will not hang forever.
		 * So make the sem timeout longer than the DNS timeout so that
		 * we do not need to start to cancel any pending DNS queries.
		 */
		int ret = k_sem_take(&ai_state.sem,
				     CONFIG_NET_SOCKETS_DNS_TIMEOUT +
				     K_MSEC(100));
		if (ret == -EAGAIN) {
			return DNS_EAI_AGAIN;
		}

		st1 = ai_state.status;
	} else {
		errno = -ret;
		st1 = DNS_EAI_SYSTEM;
	}

	if (ai_state.idx > 0) {
		ai_addr = &ai_state.ai_arr[ai_state.idx - 1]._ai_addr;
		net_sin(ai_addr)->sin_port = htons(port);
	}

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

	ret = z_impl_z_zsock_getaddrinfo_internal(host_copy, service_copy,
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

#define ERR(e) case DNS_ ## e: return #e
const char *zsock_gai_strerror(int errcode)
{
	switch (errcode) {
	ERR(EAI_BADFLAGS);
	ERR(EAI_NONAME);
	ERR(EAI_AGAIN);
	ERR(EAI_FAIL);
	ERR(EAI_NODATA);
	ERR(EAI_MEMORY);
	ERR(EAI_SYSTEM);
	ERR(EAI_SERVICE);

	default:
		return "EAI_UNKNOWN";
	}
}
#undef ERR

#endif
