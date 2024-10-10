/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_dns_dispatcher, CONFIG_DNS_SOCKET_DISPATCHER_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/slist.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/socket_service.h>

#include "../../ip/net_stats.h"
#include "dns_pack.h"

static K_MUTEX_DEFINE(lock);

static sys_slist_t sockets;

#define DNS_RESOLVER_MIN_BUF	1
#define DNS_RESOLVER_BUF_CTR	(DNS_RESOLVER_MIN_BUF + \
				 CONFIG_DNS_RESOLVER_ADDITIONAL_BUF_CTR)

NET_BUF_POOL_DEFINE(dns_msg_pool, DNS_RESOLVER_BUF_CTR,
		    DNS_RESOLVER_MAX_BUF_SIZE, 0, NULL);

static struct socket_dispatch_table {
	struct dns_socket_dispatcher *ctx;
} dispatch_table[CONFIG_ZVFS_OPEN_MAX];

static int dns_dispatch(struct dns_socket_dispatcher *dispatcher,
			int sock, struct sockaddr *addr, size_t addrlen,
			struct net_buf *dns_data, size_t buf_len)
{
	/* Helper struct to track the dns msg received from the server */
	struct dns_msg_t dns_msg;
	bool is_query;
	int data_len;
	int ret;

	data_len = MIN(buf_len, DNS_RESOLVER_MAX_BUF_SIZE);

	dns_msg.msg = dns_data->data;
	dns_msg.msg_size = data_len;

	/* Make sure that we can read DNS id, flags and rcode */
	if (dns_msg.msg_size < (sizeof(uint16_t) + sizeof(uint16_t))) {
		ret = -EINVAL;
		goto done;
	}

	if (dns_header_rcode(dns_msg.msg) == DNS_HEADER_REFUSED) {
		ret = -EINVAL;
		goto done;
	}

	is_query = (dns_header_qr(dns_msg.msg) == DNS_QUERY);
	if (is_query) {
		if (dispatcher->type == DNS_SOCKET_RESPONDER) {
			/* Call the responder callback */
			ret = dispatcher->cb(dispatcher->ctx, sock,
					     addr, addrlen,
					     dns_data, data_len);
		} else if (dispatcher->pair) {
			ret = dispatcher->pair->cb(dispatcher->pair->ctx, sock,
						   addr, addrlen,
						   dns_data, data_len);
		} else {
			/* Discard the message as it was a query and there are none
			 * expecting a query.
			 */
			ret = -ENOENT;
		}
	} else {
		/* So this was an answer to a query that was made by resolver.
		 */
		if (dispatcher->type == DNS_SOCKET_RESOLVER) {
			/* Call the resolver callback */
			ret = dispatcher->cb(dispatcher->ctx, sock,
					     addr, addrlen,
					     dns_data, data_len);
		} else if (dispatcher->pair) {
			ret = dispatcher->pair->cb(dispatcher->pair->ctx, sock,
						   addr, addrlen,
						   dns_data, data_len);
		} else {
			/* Discard the message as it was not a query reply and
			 * we were a reply.
			 */
			ret = -ENOENT;
		}
	}

done:
	if (IS_ENABLED(CONFIG_NET_STATISTICS_DNS)) {
		struct net_if *iface = NULL;

		if (IS_ENABLED(CONFIG_NET_IPV6) && addr->sa_family == AF_INET6) {
			iface = net_if_ipv6_select_src_iface(&net_sin6(addr)->sin6_addr);
		} else if (IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == AF_INET) {
			iface = net_if_ipv4_select_src_iface(&net_sin(addr)->sin_addr);
		}

		if (iface != NULL) {
			if (ret < 0) {
				net_stats_update_dns_drop(iface);
			} else {
				net_stats_update_dns_recv(iface);
			}
		}
	}

	return ret;
}

static int recv_data(struct net_socket_service_event *pev)
{
	struct socket_dispatch_table *table = pev->user_data;
	struct dns_socket_dispatcher *dispatcher;
	socklen_t optlen = sizeof(int);
	struct net_buf *dns_data = NULL;
	struct sockaddr addr;
	size_t addrlen;
	int family, sock_error;
	int ret = 0, len;

	dispatcher = table[pev->event.fd].ctx;

	k_mutex_lock(&dispatcher->lock, K_FOREVER);

	(void)zsock_getsockopt(pev->event.fd, SOL_SOCKET,
			       SO_DOMAIN, &family, &optlen);

	if ((pev->event.revents & ZSOCK_POLLERR) ||
	    (pev->event.revents & ZSOCK_POLLNVAL)) {
		(void)zsock_getsockopt(pev->event.fd, SOL_SOCKET,
				       SO_ERROR, &sock_error, &optlen);
		NET_ERR("Receiver IPv%d socket error (%d)",
			family == AF_INET ? 4 : 6, sock_error);
		ret = DNS_EAI_SYSTEM;
		goto unlock;
	}

	addrlen = (family == AF_INET) ? sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6);

	dns_data = net_buf_alloc(&dns_msg_pool, dispatcher->buf_timeout);
	if (!dns_data) {
		ret = DNS_EAI_MEMORY;
		goto unlock;
	}

	ret = zsock_recvfrom(pev->event.fd, dns_data->data,
			     net_buf_max_len(dns_data), 0,
			     (struct sockaddr *)&addr, &addrlen);
	if (ret < 0) {
		ret = -errno;
		NET_ERR("recv failed on IPv%d socket (%d)",
			family == AF_INET ? 4 : 6, -ret);
		goto free_buf;
	}

	len = ret;

	ret = dns_dispatch(dispatcher, pev->event.fd,
			   (struct sockaddr *)&addr, addrlen,
			   dns_data, len);
free_buf:
	if (dns_data) {
		net_buf_unref(dns_data);
	}

unlock:
	k_mutex_unlock(&dispatcher->lock);

	return ret;
}

void dns_dispatcher_svc_handler(struct k_work *work)
{
	struct net_socket_service_event *pev =
		CONTAINER_OF(work, struct net_socket_service_event, work);
	int ret;

	ret = recv_data(pev);
	if (ret < 0 && ret != DNS_EAI_ALLDONE && ret != -ENOENT) {
		NET_ERR("DNS recv error (%d)", ret);
	}
}

int dns_dispatcher_register(struct dns_socket_dispatcher *ctx)
{
	struct dns_socket_dispatcher *entry, *next, *found = NULL;
	sys_snode_t *prev_node = NULL;
	bool dup = false;
	size_t addrlen;
	int ret = 0;

	k_mutex_lock(&lock, K_FOREVER);

	if (sys_slist_find(&sockets, &ctx->node, &prev_node)) {
		ret = -EALREADY;
		goto out;
	}

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&sockets, entry, next, node) {
		/* Refuse to register context if we have identical context
		 * already registered.
		 */
		if (ctx->type == entry->type &&
		    ctx->local_addr.sa_family == entry->local_addr.sa_family) {
			if (net_sin(&entry->local_addr)->sin_port ==
			    net_sin(&ctx->local_addr)->sin_port) {
				dup = true;
				continue;
			}
		}

		/* Then check if there is an entry with same family and
		 * port already in the list. If there is then we can act
		 * as a dispatcher for the given socket. Do not break
		 * from the loop even if we found an entry so that we
		 * can catch possible duplicates.
		 */
		if (found == NULL && ctx->type != entry->type &&
		    ctx->local_addr.sa_family == entry->local_addr.sa_family) {
			if (net_sin(&entry->local_addr)->sin_port ==
			    net_sin(&ctx->local_addr)->sin_port) {
				found = entry;
				continue;
			}
		}
	}

	if (dup) {
		/* Found a duplicate */
		ret = -EALREADY;
		goto out;
	}

	if (found != NULL) {
		entry = found;

		if (entry->pair != NULL) {
			NET_DBG("Already paired connection found.");
			ret = -EALREADY;
			goto out;
		}

		entry->pair = ctx;

		for (int i = 0; i < ctx->fds_len; i++) {
			CHECKIF((int)ctx->fds[i].fd >= (int)ARRAY_SIZE(dispatch_table)) {
				ret = -ERANGE;
				goto out;
			}

			if (ctx->fds[i].fd < 0) {
				continue;
			}

			if (dispatch_table[ctx->fds[i].fd].ctx == NULL) {
				dispatch_table[ctx->fds[i].fd].ctx = ctx;
			}
		}

		/* Basically we are now done. If there is incoming data to
		 * the socket, the dispatcher will then pass it to the correct
		 * recipient.
		 */
		ret = 0;
		goto out;
	}

	ctx->buf_timeout = DNS_BUF_TIMEOUT;

	if (ctx->local_addr.sa_family == AF_INET) {
		addrlen = sizeof(struct sockaddr_in);
	} else {
		addrlen = sizeof(struct sockaddr_in6);
	}

	/* Bind and then register a socket service with this combo */
	ret = zsock_bind(ctx->sock, &ctx->local_addr, addrlen);
	if (ret < 0) {
		ret = -errno;
		NET_DBG("Cannot bind DNS socket %d (%d)", ctx->sock, ret);
		goto out;
	}

	ctx->pair = NULL;

	for (int i = 0; i < ctx->fds_len; i++) {
		if ((int)ctx->fds[i].fd >= (int)ARRAY_SIZE(dispatch_table)) {
			ret = -ERANGE;
			goto out;
		}

		if (ctx->fds[i].fd < 0) {
			continue;
		}

		if (dispatch_table[ctx->fds[i].fd].ctx == NULL) {
			dispatch_table[ctx->fds[i].fd].ctx = ctx;
		}
	}

	ret = net_socket_service_register(ctx->svc, ctx->fds, ctx->fds_len, &dispatch_table);
	if (ret < 0) {
		NET_DBG("Cannot register socket service (%d)", ret);
		goto out;
	}

	sys_slist_prepend(&sockets, &ctx->node);

out:
	k_mutex_unlock(&lock);

	return ret;
}

int dns_dispatcher_unregister(struct dns_socket_dispatcher *ctx)
{
	int ret = 0;

	k_mutex_lock(&lock, K_FOREVER);

	(void)sys_slist_find_and_remove(&sockets, &ctx->node);

	for (int i = 0; i < ctx->fds_len; i++) {
		CHECKIF((int)ctx->fds[i].fd >= (int)ARRAY_SIZE(dispatch_table)) {
			ret = -ERANGE;
			goto out;
		}

		dispatch_table[ctx->fds[i].fd].ctx = NULL;
	}

out:
	k_mutex_unlock(&lock);

	return ret;
}

void dns_dispatcher_init(void)
{
	sys_slist_init(&sockets);
}
