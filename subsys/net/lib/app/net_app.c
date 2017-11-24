/* net_app.c */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_APP)
#define SYS_LOG_DOMAIN "net/app"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define  MBEDTLS_PRINT printf
#else
#include <misc/printk.h>
#define  MBEDTLS_PRINT printk
#endif /* CONFIG_STDOUT_CONSOLE */
#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/dhcpv4.h>
#include <net/net_mgmt.h>
#include <net/udp.h>

#include <net/net_app.h>

#include "../../ip/udp_internal.h"

#include "net_app_private.h"

#if defined(CONFIG_NET_APP_DTLS_TIMEOUT)
#define DTLS_TIMEOUT K_SECONDS(CONFIG_NET_APP_DTLS_TIMEOUT)
#else
#define DTLS_TIMEOUT K_SECONDS(15)
#endif

#if defined(CONFIG_NET_DEBUG_APP)
static sys_slist_t _net_app_instances;

void _net_app_register(struct net_app_ctx *ctx)
{
	sys_slist_prepend(&_net_app_instances, &ctx->node);
}

void _net_app_unregister(struct net_app_ctx *ctx)
{
	sys_slist_find_and_remove(&_net_app_instances, &ctx->node);
}

static void net_app_foreach(net_app_ctx_cb_t cb, enum net_app_type type,
			    void *user_data)
{
	struct net_app_ctx *ctx;

	SYS_SLIST_FOR_EACH_CONTAINER(&_net_app_instances, ctx, node) {
		if (ctx->is_init && ctx->app_type == type) {
			if (ctx->app_type == NET_APP_CLIENT &&
			    !ctx->is_enabled) {
				continue;
			}

			cb(ctx, user_data);
		}
	}
}

void net_app_server_foreach(net_app_ctx_cb_t cb, void *user_data)
{
	net_app_foreach(cb, NET_APP_SERVER, user_data);
}

void net_app_client_foreach(net_app_ctx_cb_t cb, void *user_data)
{
	net_app_foreach(cb, NET_APP_CLIENT, user_data);
}
#endif /* CONFIG_NET_DEBUG_APP */

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
int net_app_set_net_pkt_pool(struct net_app_ctx *ctx,
			     net_pkt_get_slab_func_t tx_slab,
			     net_pkt_get_pool_func_t data_pool)
{
	ctx->tx_slab = tx_slab;
	ctx->data_pool = data_pool;

	return 0;
}
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#if defined(CONFIG_NET_DEBUG_APP)
char *_net_app_sprint_ipaddr(char *buf, int buflen,
			     const struct sockaddr *addr)
{
	if (addr->sa_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		char ipaddr[NET_IPV6_ADDR_LEN];

		net_addr_ntop(addr->sa_family,
			      &net_sin6(addr)->sin6_addr,
			      ipaddr, sizeof(ipaddr));
		snprintk(buf, buflen, "[%s]:%u", ipaddr,
			 ntohs(net_sin6(addr)->sin6_port));
#endif
	} else if (addr->sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		char ipaddr[NET_IPV4_ADDR_LEN];

		net_addr_ntop(addr->sa_family,
			      &net_sin(addr)->sin_addr,
			      ipaddr, sizeof(ipaddr));
		snprintk(buf, buflen, "%s:%u", ipaddr,
			 ntohs(net_sin(addr)->sin_port));
#endif
	} else {
		snprintk(buf, buflen, "<AF_UNSPEC %d>",
			 addr->sa_family);
	}

	return buf;
}

void _net_app_print_info(struct net_app_ctx *ctx)
{
#define PORT_STR_LEN sizeof("[]:xxxxx")
	char local[NET_IPV6_ADDR_LEN + PORT_STR_LEN];
	char remote[NET_IPV6_ADDR_LEN + PORT_STR_LEN];

	_net_app_sprint_ipaddr(local, sizeof(local), &ctx->default_ctx->local);
	_net_app_sprint_ipaddr(remote, sizeof(remote),
			       &ctx->default_ctx->remote);

	NET_DBG("net app connect %s %s %s",
		local,
		ctx->app_type == NET_APP_CLIENT ? "->" : "<-",
		remote);
}
#endif /* CONFIG_NET_DEBUG_APP */

#if defined(CONFIG_NET_APP_SERVER) || defined(CONFIG_NET_APP_CLIENT)
void _net_app_received(struct net_context *net_ctx,
		       struct net_pkt *pkt,
		       int status,
		       void *user_data)
{
	struct net_app_ctx *ctx = user_data;

#if defined(CONFIG_NET_APP_CLIENT)
	if (ctx->app_type == NET_APP_CLIENT) {
		if (!pkt) {
			if (ctx->cb.close) {
				ctx->cb.close(ctx, status, ctx->user_data);
			}

			return;
		}

		if (ctx->cb.recv) {
			ctx->cb.recv(ctx, pkt, status, ctx->user_data);
		}
	}
#endif

#if defined(CONFIG_NET_APP_SERVER)
	if (ctx->app_type == NET_APP_SERVER) {
		if (!pkt) {
#if defined(CONFIG_NET_TCP)
			int i;
#endif

			if (ctx->cb.close) {
				ctx->cb.close(ctx, status, ctx->user_data);
			}

#if defined(CONFIG_NET_TCP)
			for (i = 0;
			     ctx->proto == IPPROTO_TCP &&
				     i < CONFIG_NET_APP_SERVER_NUM_CONN;
			     i++) {
				if (ctx->server.net_ctxs[i] == net_ctx &&
				    ctx == net_ctx->net_app) {
					net_context_put(net_ctx);
					ctx->server.net_ctxs[i] = NULL;
					net_ctx->net_app = NULL;
					break;
				}
			}
#endif

			return;
		}

		if (ctx->cb.recv) {
			ctx->cb.recv(ctx, pkt, status, ctx->user_data);
		}
	}
#endif
}
#endif /* CONFIG_NET_APP_SERVER || CONFIG_NET_APP_CLIENT */

#if defined(CONFIG_NET_APP_SERVER) || defined(CONFIG_NET_APP_CLIENT)
int _net_app_set_net_ctx(struct net_app_ctx *ctx,
			 struct net_context *net_ctx,
			 struct sockaddr *addr,
			 socklen_t socklen,
			 enum net_ip_protocol proto)
{
	int ret;

	if (!net_ctx || !net_context_is_used(net_ctx)) {
		return -ENOENT;
	}

	ret = net_context_bind(net_ctx, addr, socklen);
	if (ret < 0) {
		NET_ERR("Cannot bind context (%d)", ret);
		goto out;
	}

#if defined(CONFIG_NET_APP_SERVER) && defined(CONFIG_NET_TCP)
	if (ctx->app_type == NET_APP_SERVER && proto == IPPROTO_TCP) {
		ret = net_context_listen(net_ctx, 0);
		if (ret < 0) {
			NET_ERR("Cannot listen context (%d)", ret);
			goto out;
		}

		ret = net_context_accept(net_ctx, _net_app_accept_cb,
					 K_NO_WAIT, ctx);
		if (ret < 0) {
			NET_ERR("Cannot accept context (%d)", ret);
			goto out;
		}

		/* TCP recv callback is set after we have accepted the
		 * connection.
		 */
	}
#endif /* CONFIG_NET_APP_SERVER && CONFIG_NET_TCP */

#if defined(CONFIG_NET_APP_SERVER) && defined(CONFIG_NET_UDP)
	if (ctx->app_type == NET_APP_SERVER && proto == IPPROTO_UDP) {
		net_context_recv(net_ctx, _net_app_received, K_NO_WAIT, ctx);
	}
#endif /* CONFIG_NET_APP_SERVER && CONFIG_NET_UDP */

out:
	return ret;
}

int _net_app_set_local_addr(struct sockaddr *addr, const char *myaddr,
			    u16_t port)
{
	if (myaddr) {
		void *inaddr;

		if (addr->sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
			inaddr = &net_sin(addr)->sin_addr;
			net_sin(addr)->sin_port = htons(port);
#else
			return -EPFNOSUPPORT;
#endif
		} else if (addr->sa_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
			inaddr = &net_sin6(addr)->sin6_addr;
			net_sin6(addr)->sin6_port = htons(port);
#else
			return -EPFNOSUPPORT;
#endif
		} else {
			return -EAFNOSUPPORT;
		}

		return net_addr_pton(addr->sa_family, myaddr, inaddr);
	}

	/* If the caller did not supply the address where to bind, then
	 * try to figure it out ourselves.
	 */
	if (addr->sa_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		net_ipaddr_copy(&net_sin6(addr)->sin6_addr,
				net_if_ipv6_select_src_addr(NULL,
					(struct in6_addr *)
					net_ipv6_unspecified_address()));
#else
		return -EPFNOSUPPORT;
#endif
	} else if (addr->sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		struct net_if *iface = net_if_get_default();

		/* For IPv4 we take the first address in the interface */
		net_ipaddr_copy(&net_sin(addr)->sin_addr,
				&iface->ipv4.unicast[0].address.in_addr);
#else
		return -EPFNOSUPPORT;
#endif
	}

	return 0;
}
#endif /* CONFIG_NET_APP_SERVER || CONFIG_NET_APP_CLIENT */

#if defined(CONFIG_NET_APP_SERVER)
#endif /* CONFIG_NET_APP_SERVER */

#if defined(CONFIG_NET_IPV4) && (defined(CONFIG_NET_APP_SERVER) || \
				 defined(CONFIG_NET_APP_CLIENT))
static int setup_ipv4_ctx(struct net_app_ctx *ctx,
			  enum net_sock_type sock_type,
			  enum net_ip_protocol proto)
{
	int ret;

	ret = net_context_get(AF_INET, sock_type, proto, &ctx->ipv4.ctx);
	if (ret < 0) {
		NET_ERR("Cannot get network context (%d)", ret);
		ctx->ipv4.ctx = NULL;
		return ret;
	}

	net_context_setup_pools(ctx->ipv4.ctx, ctx->tx_slab,
				ctx->data_pool);

	return ret;
}
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV6) && (defined(CONFIG_NET_APP_SERVER) || \
				 defined(CONFIG_NET_APP_CLIENT))
static int setup_ipv6_ctx(struct net_app_ctx *ctx,
			  enum net_sock_type sock_type,
			  enum net_ip_protocol proto)
{
	int ret;

	ret = net_context_get(AF_INET6, sock_type, proto, &ctx->ipv6.ctx);
	if (ret < 0) {
		NET_ERR("Cannot get network context (%d)", ret);
		ctx->ipv6.ctx = NULL;
		return ret;
	}

	net_context_setup_pools(ctx->ipv6.ctx, ctx->tx_slab,
				ctx->data_pool);

	return ret;
}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_APP_SERVER) || defined(CONFIG_NET_APP_CLIENT)
static void select_default_ctx(struct net_app_ctx *ctx)
{
#if defined(CONFIG_NET_IPV6)
	ctx->default_ctx = &ctx->ipv6;
#elif defined(CONFIG_NET_IPV4)
	ctx->default_ctx = &ctx->ipv4;
#endif
}

int _net_app_config_local_ctx(struct net_app_ctx *ctx,
			      enum net_sock_type sock_type,
			      enum net_ip_protocol proto,
			      struct sockaddr *addr)
{
	int ret;

	if (!addr) {
#if defined(CONFIG_NET_IPV6)
		if (ctx->ipv6.local.sa_family == AF_INET6 ||
		    ctx->ipv6.local.sa_family == AF_UNSPEC) {
			ret = setup_ipv6_ctx(ctx, sock_type, proto);
		} else {
			ret = -EPFNOSUPPORT;
			goto fail;
		}

		if (!ret) {
			select_default_ctx(ctx);
		}
#endif

#if defined(CONFIG_NET_IPV4)
		if (ctx->ipv4.local.sa_family == AF_INET ||
		    ctx->ipv4.local.sa_family == AF_UNSPEC) {
			ret = setup_ipv4_ctx(ctx, sock_type, proto);
		} else {
			ret = -EPFNOSUPPORT;
			goto fail;
		}

		if (!ret) {
			select_default_ctx(ctx);
		}
#endif
		return ret;
	} else {
		if (addr->sa_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
			ret = setup_ipv6_ctx(ctx, sock_type, proto);
			ctx->default_ctx = &ctx->ipv6;
#else
			ret = -EPFNOSUPPORT;
			goto fail;
#endif
		} else if (addr->sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
			ret = setup_ipv4_ctx(ctx, sock_type, proto);
			ctx->default_ctx = &ctx->ipv4;
#else
			ret = -EPFNOSUPPORT;
			goto fail;
#endif
		} else if (addr->sa_family == AF_UNSPEC) {
#if defined(CONFIG_NET_IPV4)
			ret = setup_ipv4_ctx(ctx, sock_type, proto);
			ctx->default_ctx = &ctx->ipv4;
#endif
			/* We ignore the IPv4 error if IPv6 is enabled */
#if defined(CONFIG_NET_IPV6)
			ret = setup_ipv6_ctx(ctx, sock_type, proto);
			ctx->default_ctx = &ctx->ipv6;
#endif
		} else {
			ret = -EINVAL;
			goto fail;
		}
	}

fail:
	return ret;
}
#endif /* CONFIG_NET_APP_SERVER || CONFIG_NET_APP_CLIENT */

int net_app_release(struct net_app_ctx *ctx)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

#if defined(CONFIG_NET_IPV6)
	if (ctx->ipv6.ctx) {
		net_context_put(ctx->ipv6.ctx);
		ctx->ipv6.ctx = NULL;
	}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (ctx->ipv4.ctx) {
		net_context_put(ctx->ipv4.ctx);
		ctx->ipv4.ctx = NULL;
	}
#endif /* CONFIG_NET_IPV4 */

	ctx->is_init = false;

	_net_app_unregister(ctx);

	return 0;
}

#if defined(CONFIG_NET_APP_CLIENT)
static inline
struct net_context *select_client_ctx(struct net_app_ctx *ctx,
				      const struct sockaddr *dst)
{
	if (ctx->proto == IPPROTO_UDP) {
		if (!dst) {
			if (ctx->is_tls) {
#if defined(CONFIG_NET_APP_DTLS)
				if (ctx->dtls.ctx) {
					return ctx->dtls.ctx;
				} else {
					return ctx->default_ctx->ctx;
				}
#else
				return NULL;
#endif
			} else {
				return ctx->default_ctx->ctx;
			}
		} else {
			if (ctx->is_tls) {
#if defined(CONFIG_NET_APP_DTLS)
				if (ctx->dtls.ctx) {
					return ctx->dtls.ctx;
				}
#else
				return NULL;
#endif
			}

			goto common_checks;
		}
	} else {
		if (!dst) {
			if (ctx->default_ctx->ctx &&
			    atomic_get(&ctx->default_ctx->ctx->refcount) <= 0) {
				ctx->default_ctx->ctx = NULL;
			}

			return ctx->default_ctx->ctx;
		} else {
		common_checks:
			if (dst->sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
				if (ctx->ipv4.ctx &&
				    atomic_get(&ctx->ipv4.ctx->refcount) <= 0) {
					ctx->ipv4.ctx = NULL;
				}

				return ctx->ipv4.ctx;
#else
				return NULL;
#endif
			}

			if (dst->sa_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
				if (ctx->ipv6.ctx &&
				    atomic_get(&ctx->ipv6.ctx->refcount) <= 0) {
					ctx->ipv6.ctx = NULL;
				}

				return ctx->ipv6.ctx;
#else
				return NULL;
#endif
			}

			if (dst->sa_family == AF_UNSPEC) {
				if (ctx->default_ctx->ctx &&
				    atomic_get(&ctx->default_ctx->ctx->refcount)
								       <= 0) {
					ctx->default_ctx->ctx = NULL;
				}

				return ctx->default_ctx->ctx;
			}
		}
	}

	return NULL;
}
#else
#define select_client_ctx(...) NULL
#endif /* CONFIG_NET_APP_CLIENT */

#if defined(CONFIG_NET_APP_SERVER)
#if defined(CONFIG_NET_TCP)
static struct net_context *get_server_ctx(struct net_app_ctx *ctx,
					  const struct sockaddr *dst)
{
	int i;

	for (i = 0; i < CONFIG_NET_APP_SERVER_NUM_CONN; i++) {
		struct net_context *tmp = ctx->server.net_ctxs[i];
		u16_t port, rport;

		if (!tmp || !net_context_is_used(tmp)) {
			continue;
		}

		if (!dst) {
			if (tmp->net_app == ctx) {
				NET_DBG("Selecting net_ctx %p iface %p for "
					"NULL dst",
					tmp, net_context_get_iface(tmp));
				return tmp;
			}

			continue;
		}

		/* Serve IPv6 first if the user does not care */
		if (IS_ENABLED(CONFIG_NET_IPV6) &&
		    (dst->sa_family == AF_UNSPEC ||
		     (tmp->remote.sa_family == AF_INET6 &&
		      dst->sa_family == AF_INET6))) {
			struct in6_addr *addr6 = &net_sin6(dst)->sin6_addr;
			struct in6_addr *remote6;

			remote6 = &net_sin6(&tmp->remote)->sin6_addr;
			rport = net_sin6(&tmp->remote)->sin6_port;
			port = net_sin6(dst)->sin6_port;

			if (net_ipv6_addr_cmp(addr6, remote6) &&
			    port == rport) {
				NET_DBG("Selecting net_ctx %p iface %p for "
					"AF_INET6 port %d", tmp,
					net_context_get_iface(tmp),
					ntohs(rport));
				return tmp;
			}

			if (tmp->net_app == ctx) {
				NET_DBG("Selecting net_ctx %p iface %p"
					" for %s port %d", tmp,
					net_context_get_iface(tmp),
					dst->sa_family == AF_UNSPEC ?
					"AF_UNSPEC" : "AF_INET6",
					ntohs(rport));
				return tmp;
			}
		}

		if (IS_ENABLED(CONFIG_NET_IPV4) &&
		    (dst->sa_family == AF_UNSPEC ||
		     (tmp->remote.sa_family == AF_INET &&
		      dst->sa_family == AF_INET))) {
			struct in_addr *addr4 = &net_sin(dst)->sin_addr;
			struct in_addr *remote4;

			remote4 = &net_sin(&tmp->remote)->sin_addr;
			rport = net_sin(&tmp->remote)->sin_port;
			port = net_sin(dst)->sin_port;

			if (net_ipv4_addr_cmp(addr4, remote4) &&
			    port == rport) {
				NET_DBG("Selecting net_ctx %p iface %p for "
					"AF_INET port %d", tmp,
					net_context_get_iface(tmp),
					ntohs(port));
				return tmp;
			}

			if (tmp->net_app == ctx) {
				NET_DBG("Selecting net_ctx %p iface %p"
					" for %s port %d", tmp,
					net_context_get_iface(tmp),
					dst->sa_family == AF_UNSPEC ?
					"AF_UNSPEC" : "AF_INET",
					ntohs(port));
				return tmp;
			}
		}
	}

	return NULL;
}
#endif /* CONFIG_NET_TCP */

static inline
struct net_context *select_server_ctx(struct net_app_ctx *ctx,
				      const struct sockaddr *dst)
{
	if (ctx->proto == IPPROTO_TCP) {
#if defined(CONFIG_NET_TCP)
		return get_server_ctx(ctx, dst);
#else
		return NULL;
#endif
	} else if (ctx->proto == IPPROTO_UDP) {
		if (!dst) {
			if (ctx->is_tls) {
#if defined(CONFIG_NET_APP_DTLS)
				return ctx->dtls.ctx;
#else
				return NULL;
#endif
			} else {
				return ctx->default_ctx->ctx;
			}
		} else {
			if (ctx->is_tls) {
#if defined(CONFIG_NET_APP_DTLS)
				return ctx->dtls.ctx;
#else
				return NULL;
#endif
			}

			if (dst->sa_family == AF_INET) {
#if defined(CONFIG_NET_IPV4)
				return ctx->ipv4.ctx;
#else
				return NULL;
#endif
			}

			if (dst->sa_family == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
				return ctx->ipv6.ctx;
#else
				return NULL;
#endif
			}

			if (dst->sa_family == AF_UNSPEC) {
				return ctx->default_ctx->ctx;
			}
		}
	}

	return NULL;
}
#else
#define select_server_ctx(...) NULL
#endif /* CONFIG_NET_APP_SERVER */

#if NET_LOG_ENABLED > 0
struct net_context *_net_app_select_net_ctx_debug(struct net_app_ctx *ctx,
						  const struct sockaddr *dst,
						  const char *caller,
						  int line)
#else
struct net_context *_net_app_select_net_ctx(struct net_app_ctx *ctx,
					    const struct sockaddr *dst)
#endif
{
	struct net_context *net_ctx = NULL;

	if (ctx->app_type == NET_APP_CLIENT) {
		net_ctx = select_client_ctx(ctx, dst);
	} else if (ctx->app_type == NET_APP_SERVER) {
		net_ctx = select_server_ctx(ctx, dst);
	}

	NET_DBG("Selecting %p net_ctx (%s():%d)", net_ctx, caller, line);

	return net_ctx;
}

int net_app_set_cb(struct net_app_ctx *ctx,
		   net_app_connect_cb_t connect_cb,
		   net_app_recv_cb_t recv_cb,
		   net_app_send_cb_t send_cb,
		   net_app_close_cb_t close_cb)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	ctx->cb.connect = connect_cb;
	ctx->cb.recv = recv_cb;
	ctx->cb.send = send_cb;
	ctx->cb.close = close_cb;

	return 0;
}

static void _app_send(struct net_context *net_ctx,
		      int status,
		      void *token,
		      void *user_data)
{
	struct net_app_ctx *ctx = user_data;

	ARG_UNUSED(ctx);

#if defined(CONFIG_NET_APP_CLIENT)
	if (ctx->app_type == NET_APP_CLIENT && ctx->cb.send) {
		ctx->cb.send(ctx, status, token, ctx->user_data);
	}
#endif

#if defined(CONFIG_NET_APP_SERVER)
	if (ctx->app_type == NET_APP_SERVER && ctx->cb.send) {
		ctx->cb.send(ctx, status, token, ctx->user_data);
	}
#endif
}

int net_app_send_pkt(struct net_app_ctx *ctx,
		     struct net_pkt *pkt,
		     const struct sockaddr *dst,
		     socklen_t dst_len,
		     s32_t timeout,
		     void *user_data_send)
{
	int ret;

	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	/* Get rid of IP + UDP/TCP header if it is there. The IP header
	 * will be put back just before sending the packet. Normally the
	 * data that is sent does not contain IP header, but if the caller
	 * replies the packet directly back, the IP header could be there
	 * at this point.
	 */
	if (net_pkt_appdatalen(pkt) > 0) {
		int header_len;

		header_len = net_buf_frags_len(pkt->frags) -
			net_pkt_appdatalen(pkt);
		if (header_len > 0) {
			net_buf_pull(pkt->frags, header_len);
		}
	} else {
		net_pkt_set_appdatalen(pkt, net_buf_frags_len(pkt->frags));
	}

	if (ctx->proto == IPPROTO_UDP) {
		if (!dst) {
			if (net_pkt_family(pkt) == AF_INET) {
#if defined(CONFIG_NET_IPV4)
				dst = &ctx->ipv4.remote;
				dst_len = sizeof(struct sockaddr_in);
#else
				return -EPFNOSUPPORT;
#endif
			} else {
				if (net_pkt_family(pkt) == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
					dst = &ctx->ipv6.remote;
					dst_len = sizeof(struct sockaddr_in6);
#else
					return -EPFNOSUPPORT;
#endif
				} else {
					return -EPFNOSUPPORT;
				}
			}
		} else {
			if (net_pkt_family(pkt) == AF_INET) {
#if defined(CONFIG_NET_IPV4)
				net_ipaddr_copy(net_sin(&ctx->ipv4.remote),
						net_sin(dst));
				dst_len = sizeof(struct sockaddr_in);
#else
				return -EPFNOSUPPORT;
#endif
			} else {
				if (net_pkt_family(pkt) == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
					net_ipaddr_copy(
						net_sin6(&ctx->ipv6.remote),
						net_sin6(dst));
					dst_len = sizeof(struct sockaddr_in6);
#else
					return -EPFNOSUPPORT;
#endif
				} else {
					return -EPFNOSUPPORT;
				}
			}
		}
	}

	ret = ctx->send_data(pkt, dst, dst_len, _app_send, timeout,
			     user_data_send, ctx);
	if (ret < 0) {
		NET_DBG("Cannot send to peer (%d)", ret);
	}

	return ret;
}

int net_app_send_buf(struct net_app_ctx *ctx,
		     u8_t *buf,
		     size_t buf_len,
		     const struct sockaddr *dst,
		     socklen_t dst_len,
		     s32_t timeout,
		     void *user_data_send)
{
	struct net_context *net_ctx;
	struct net_pkt *pkt;
	struct net_buf *frag;
	size_t len, pos = 0;
	int ret;

	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	if (!buf_len) {
		return -EMSGSIZE;
	}

	net_ctx = _net_app_select_net_ctx(ctx, dst);
	if (!net_ctx) {
		return -ENOENT;
	}

	pkt = net_pkt_get_tx(net_ctx, timeout);
	if (!pkt) {
		return -ENOMEM;
	}

	net_pkt_set_appdatalen(pkt, buf_len);

	while (buf_len) {
		frag = net_pkt_get_data(net_ctx, timeout);
		if (!frag) {
			net_pkt_unref(pkt);
			return -ENOMEM;
		}

		len = net_buf_tailroom(frag);
		if (len >= buf_len) {
			net_buf_add_mem(frag, buf + pos, buf_len);
			net_pkt_frag_add(pkt, frag);
			goto send;
		}

		net_buf_add_mem(frag, buf + pos, len);
		net_pkt_frag_add(pkt, frag);

		pos += len;
		buf_len -= len;
	}

send:
	ret = ctx->send_data(pkt, dst, dst_len, _app_send, timeout,
			     user_data_send, ctx);
	if (ret < 0) {
		NET_DBG("Cannot send to peer (%d)", ret);
		net_pkt_unref(pkt);
	}

	return ret;
}

struct net_pkt *net_app_get_net_pkt(struct net_app_ctx *ctx,
				    sa_family_t family,
				    s32_t timeout)
{
	struct net_context *net_ctx;
	struct sockaddr dst = { 0 };

	if (!ctx) {
		return NULL;
	}

	if (!ctx->is_init) {
		return NULL;
	}

	dst.sa_family = family;

	net_ctx = _net_app_select_net_ctx(ctx, &dst);
	if (!net_ctx) {
		return NULL;
	}

	return net_pkt_get_tx(net_ctx, timeout);
}

struct net_buf *net_app_get_net_buf(struct net_app_ctx *ctx,
				    struct net_pkt *pkt,
				    s32_t timeout)
{
	struct net_buf *frag;

	if (!ctx || !pkt) {
		return NULL;
	}

	if (!ctx->is_init) {
		return NULL;
	}

	frag = net_pkt_get_frag(pkt, timeout);
	if (!frag) {
		return NULL;
	}

	net_pkt_frag_add(pkt, frag);

	return frag;
}

int net_app_close(struct net_app_ctx *ctx)
{
	struct net_context *net_ctx;

	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
	if (ctx->tls.tx_pending) {
		ctx->tls.close_requested = true;
		return -EINPROGRESS;
	}
#endif

	net_ctx = _net_app_select_net_ctx(ctx, NULL);

	if (ctx->cb.close) {
		ctx->cb.close(ctx, 0, ctx->user_data);
	}

#if defined(CONFIG_NET_APP_SERVER) && defined(CONFIG_NET_TCP)
	if (net_ctx && ctx->app_type == NET_APP_SERVER) {
		int i;

		for (i = 0; i < CONFIG_NET_APP_SERVER_NUM_CONN; i++) {
			if (ctx->server.net_ctxs[i] == net_ctx) {
				NET_DBG("Releasing slot %d net_ctx %p",
					i, net_ctx);
				ctx->server.net_ctxs[i] = NULL;
				break;
			}
		}
	}
#endif

	if (net_ctx) {
		net_ctx->net_app = NULL;
		net_context_put(net_ctx);

		NET_DBG("Closing net_ctx %p", net_ctx);
	}

#if defined(CONFIG_NET_APP_CLIENT)
	if (ctx->app_type == NET_APP_CLIENT) {
		ctx->is_enabled = false;

		/* Make sure we do not re-use the same port if we
		 * re-connect after close.
		 */
#if defined(CONFIG_NET_IPV4)
		net_sin(&ctx->ipv4.local)->sin_port = 0;

		if (ctx->ipv4.ctx) {
			net_sin_ptr(&ctx->ipv4.ctx->local)->sin_port = 0;
		}
#endif
#if defined(CONFIG_NET_IPV6)
		net_sin6(&ctx->ipv6.local)->sin6_port = 0;

		if (ctx->ipv6.ctx) {
			net_sin6_ptr(&ctx->ipv6.ctx->local)->sin6_port = 0;
		}
#endif
	}
#endif

	return 0;
}

int net_app_close2(struct net_app_ctx *ctx, struct net_context *net_ctx)
{
	if (!ctx || !net_ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
	if (ctx->tls.tx_pending) {
		ctx->tls.close_requested = true;
		return -EINPROGRESS;
	}
#endif

	if (ctx->cb.close) {
		ctx->cb.close(ctx, 0, ctx->user_data);
	}

#if defined(CONFIG_NET_APP_SERVER) && defined(CONFIG_NET_TCP)
	if (ctx->app_type == NET_APP_SERVER) {
		int i;

		for (i = 0; i < CONFIG_NET_APP_SERVER_NUM_CONN; i++) {
			if (ctx->server.net_ctxs[i] == net_ctx) {
				ctx->server.net_ctxs[i] = NULL;
				break;
			}
		}
	}
#endif

#if defined(CONFIG_NET_APP_CLIENT)
	if (ctx->app_type == NET_APP_CLIENT) {
		if (net_ctx != _net_app_select_net_ctx(ctx, NULL)) {
			return -ENOENT;
		}

		ctx->is_enabled = false;

		/* Make sure we do not re-use the same port if we
		 * re-connect after close.
		 */
#if defined(CONFIG_NET_IPV4)
		net_sin(&ctx->ipv4.local)->sin_port = 0;

		if (net_ctx == ctx->ipv4.ctx) {
			net_sin_ptr(&ctx->ipv4.ctx->local)->sin_port = 0;
		}
#endif
#if defined(CONFIG_NET_IPV6)
		net_sin6(&ctx->ipv6.local)->sin6_port = 0;

		if (net_ctx == ctx->ipv6.ctx) {
			net_sin6_ptr(&ctx->ipv6.ctx->local)->sin6_port = 0;
		}
#endif
	}
#endif

	net_ctx->net_app = NULL;

	net_context_put(net_ctx);

	return 0;
}

#if defined(CONFIG_NET_APP_TLS) || defined(CONFIG_NET_APP_DTLS)
#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_APP)
static void my_debug(void *ctx, int level,
		     const char *file, int line, const char *str)
{
	const char *p, *basename;
	int len;

	ARG_UNUSED(ctx);

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}

	}

	/* Avoid printing double newlines */
	len = strlen(str);
	if (str[len - 1] == '\n') {
		((char *)str)[len - 1] = '\0';
	}

	NET_DBG("%s:%04d: |%d| %s", basename, line, level, str);
}
#endif /* MBEDTLS_DEBUG_C && CONFIG_NET_DEBUG_APP */

static void ssl_sent(struct net_context *context,
		     int status, void *token, void *user_data)
{
	struct net_app_ctx *ctx = user_data;

	k_sem_give(&ctx->tls.mbedtls.ssl_ctx.tx_sem);
}

int _net_app_tls_trigger_close(struct net_app_ctx *ctx)
{
	struct net_app_fifo_block *rx_data = NULL;
	struct k_mem_block block;
	int ret;

	ret = k_mem_pool_alloc(ctx->tls.pool, &block,
			       sizeof(struct net_app_fifo_block),
			       BUF_ALLOC_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	rx_data = block.data;
	rx_data->pkt = NULL;
	rx_data->dir = NET_APP_PKT_TX;

	memcpy(&rx_data->block, &block, sizeof(struct k_mem_block));

	NET_DBG("Triggering connection close");

	k_fifo_put(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo, (void *)rx_data);

	return 0;
}

/* Send encrypted data */
int _net_app_ssl_tx(void *context, const unsigned char *buf, size_t size)
{
	struct net_app_ctx *ctx = context;
	struct net_pkt *send_buf;
	size_t sent = 0;
	int ret, len = 0;

	while (size) {
		send_buf = net_app_get_net_pkt(ctx, AF_UNSPEC,
					       BUF_ALLOC_TIMEOUT);
		if (!send_buf) {
			return MBEDTLS_ERR_SSL_ALLOC_FAILED;
		}

		sent = net_pkt_append(send_buf, size, (u8_t *)buf + len,
				      BUF_ALLOC_TIMEOUT);
		size -= sent;
		len += sent;

		if (ctx->proto == IPPROTO_UDP) {
#if defined(CONFIG_NET_APP_DTLS)
			if (!ctx->dtls.ctx) {
				net_pkt_unref(send_buf);
				return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
			}

			ret = net_context_sendto(send_buf,
						 &ctx->dtls.ctx->remote,
						 sizeof(ctx->dtls.ctx->remote),
						 ssl_sent, K_NO_WAIT, NULL,
						 ctx);
#else
			ret = -EPROTONOSUPPORT;
#endif
		} else {
			ret = net_context_send(send_buf, ssl_sent, K_NO_WAIT,
					       NULL, ctx);
		}

		if (ret < 0) {
			net_pkt_unref(send_buf);
			return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
		}

		k_sem_take(&ctx->tls.mbedtls.ssl_ctx.tx_sem, K_FOREVER);

		if (ctx->tls.close_requested) {
			_net_app_tls_trigger_close(ctx);
		}
	}

	return len;
}

/* This gets plain data and then it passes it to TLS handler thread to be
 * encrypted and transmitted to peer. Note that we do not send the data
 * directly here because of the mbedtls stack requirements which are quite
 * high. So no calls to mbedtls from this processing context.
 */
int _net_app_tls_sendto(struct net_pkt *pkt,
			const struct sockaddr *dst_addr,
			socklen_t addrlen,
			net_context_send_cb_t cb,
			s32_t timeout,
			void *token,
			void *user_data)
{
	struct net_app_ctx *ctx = user_data;
	struct net_app_fifo_block *tx_data;
	struct k_mem_block block;
	int ret;

	if (!ctx->tls.handshake_done) {
		/* This means that the initial TLS handshake is not yet
		 * finished so our packet cannot be sent yet. Try sleeping
		 * a bit and hope things are ok after that. If not, then
		 * return error.
		 */
		k_sleep(MSEC(50));

		if (!ctx->tls.handshake_done) {
			NET_DBG("TLS handshake not yet done, pkt %p not sent",
				pkt);
			return -EBUSY;
		}
	}

	ARG_UNUSED(dst_addr);
	ARG_UNUSED(addrlen);

	if (pkt && !net_pkt_appdatalen(pkt)) {
		return -EINVAL;
	}

	ret = k_mem_pool_alloc(ctx->tls.pool, &block,
			       sizeof(struct net_app_fifo_block),
			       BUF_ALLOC_TIMEOUT);
	if (ret < 0) {
		return -ENOMEM;
	}

	tx_data = block.data;
	tx_data->pkt = pkt;
	tx_data->dir = NET_APP_PKT_TX;
	tx_data->token = token;
	tx_data->cb = cb;

	ctx->tls.tx_pending = true;

	/* For freeing memory later */
	memcpy(&tx_data->block, &block, sizeof(struct k_mem_block));

	k_fifo_put(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo, (void *)tx_data);

	return 0;
}

#if defined(CONFIG_NET_APP_DTLS)
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
static inline void copy_pool_vars(struct net_context *new_context,
				  struct net_context *listen_context)
{
	new_context->tx_slab = listen_context->tx_slab;
	new_context->data_pool = listen_context->data_pool;
}
#else
#define copy_pool_vars(...)
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

static void dtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
{
	struct dtls_timing_context *ctx = data;

#if DTLS_EXTRA_DEBUG == 1
	NET_DBG("Setting DTLS delays for %p, int %ums fin %ums",
		ctx, int_ms, fin_ms);
#endif

	ctx->int_ms = int_ms;
	ctx->fin_ms = fin_ms;

	if (fin_ms != 0) {
		ctx->snapshot = k_uptime_get_32();
	}
}

static int dtls_timing_get_delay(void *data)
{
	struct dtls_timing_context *timing = data;
	unsigned long elapsed_ms;

	NET_ASSERT(timing);

#if DTLS_EXTRA_DEBUG == 1
	NET_DBG("Get DTLS delays for %p, int %ums fin %ums snapshot %d",
		timing, timing->int_ms, timing->fin_ms, timing->snapshot);
#endif

	if (timing->fin_ms == 0) {
		return -1;
	}

	elapsed_ms = k_uptime_get_32() - timing->snapshot;

	if (elapsed_ms >= timing->fin_ms) {
		return 2;
	}

	if (elapsed_ms >= timing->int_ms) {
		return 1;
	}

	return 0;
}

static void dtls_cleanup(struct net_app_ctx *ctx, bool cancel_timer)
{
	if (cancel_timer) {
		k_delayed_work_cancel(&ctx->dtls.fin_timer);
	}

	/* It might be that ctx is already cleared so check it here */
	if (ctx->dtls.ctx) {
		net_udp_unregister(ctx->dtls.ctx->conn_handler);
		net_context_put(ctx->dtls.ctx);
		ctx->dtls.ctx = NULL;
	}
}

static void dtls_timeout(struct k_work *work)
{
	struct net_app_ctx *ctx =
		CONTAINER_OF(work, struct net_app_ctx, dtls.fin_timer);

	NET_DBG("Did not receive DTLS traffic in %dms", DTLS_TIMEOUT);

	dtls_cleanup(ctx, false);
}

enum net_verdict _net_app_dtls_established(struct net_conn *conn,
					   struct net_pkt *pkt,
					   void *user_data)
{
	struct net_app_ctx *ctx = user_data;
	struct net_app_fifo_block *rx_data = NULL;
	struct k_mem_block block;
	struct net_buf *frag;
	u16_t offset;
	int ret, len;

	if (!pkt) {
		return NET_DROP;
	}

	len = net_pkt_get_len(pkt) - net_pkt_ip_hdr_len(pkt) -
		net_pkt_ipv6_ext_len(pkt) - sizeof(struct net_udp_hdr);
	if (len <= 0) {
		return NET_DROP;
	}

	net_pkt_set_appdatalen(pkt, len);

	frag = net_frag_get_pos(pkt, net_pkt_ip_hdr_len(pkt) +
				net_pkt_ipv6_ext_len(pkt) +
				sizeof(struct net_udp_hdr),
				&offset);
	if (frag) {
		net_pkt_set_appdata(pkt, frag->data + offset);
	}

	ret = k_mem_pool_alloc(ctx->tls.pool, &block,
			       sizeof(struct net_app_fifo_block),
			       BUF_ALLOC_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Not enough space in DTLS mem pool");
		return NET_DROP;
	}

	rx_data = block.data;
	rx_data->pkt = pkt;
	rx_data->dir = NET_APP_PKT_RX;

	/* For freeing memory later */
	memcpy(&rx_data->block, &block, sizeof(struct k_mem_block));

	NET_DBG("Encrypted DTLS data received in pkt %p", pkt);

	k_fifo_put(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo, (void *)rx_data);

	k_delayed_work_cancel(&ctx->dtls.fin_timer);

	k_yield();

	k_delayed_work_submit(&ctx->dtls.fin_timer, DTLS_TIMEOUT);

	return NET_OK;
}

static int accept_dtls(struct net_app_ctx *ctx,
		       struct net_context *context,
		       struct net_pkt *pkt)
{
	struct net_context *dtls_context;
	struct net_udp_hdr hdr, *udp_hdr;
	struct sockaddr remote_addr;
	struct sockaddr local_addr;
	socklen_t addrlen;
	int ret;

	udp_hdr = net_udp_get_hdr(pkt, &hdr);
	if (!udp_hdr) {
		NET_DBG("Dropping invalid pkt %p", pkt);
		goto pkt_drop;
	}

	/* We create a new context that starts to wait data. */
	ret = net_context_get(net_pkt_family(pkt), SOCK_DGRAM, IPPROTO_UDP,
			      &dtls_context);
	if (ret < 0) {
		NET_DBG("Cannot get accepted context, pkt %p dropped", pkt);
		goto pkt_drop;
	}

#if defined(CONFIG_NET_IPV6)
	if (net_context_get_family(context) == AF_INET6) {
		struct sockaddr_in6 *local_addr6 = net_sin6(&local_addr);
		struct sockaddr_in6 *remote_addr6 = net_sin6(&remote_addr);

		remote_addr6->sin6_family = AF_INET6;
		local_addr6->sin6_family = AF_INET6;

		local_addr6->sin6_port = udp_hdr->dst_port;
		remote_addr6->sin6_port = udp_hdr->src_port;

		net_ipaddr_copy(&local_addr6->sin6_addr,
				&NET_IPV6_HDR(pkt)->dst);
		net_ipaddr_copy(&remote_addr6->sin6_addr,
				&NET_IPV6_HDR(pkt)->src);
		addrlen = sizeof(struct sockaddr_in6);
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_context_get_family(context) == AF_INET) {
		struct sockaddr_in *local_addr4 = net_sin(&local_addr);
		struct sockaddr_in *remote_addr4 = net_sin(&remote_addr);

		remote_addr4->sin_family = AF_INET;
		local_addr4->sin_family = AF_INET;

		local_addr4->sin_port = udp_hdr->dst_port;
		remote_addr4->sin_port = udp_hdr->src_port;

		net_ipaddr_copy(&local_addr4->sin_addr,
				&NET_IPV4_HDR(pkt)->dst);
		net_ipaddr_copy(&remote_addr4->sin_addr,
				&NET_IPV4_HDR(pkt)->src);
		addrlen = sizeof(struct sockaddr_in);
	} else
#endif /* CONFIG_NET_IPV4 */
	{
		NET_ASSERT_INFO(false, "Invalid protocol family %d",
				net_context_get_family(context));
		goto ctx_drop;
	}

	ret = net_context_bind(dtls_context, &local_addr, sizeof(local_addr));
	if (ret < 0) {
		NET_DBG("Cannot bind accepted DTLS context");
		goto ctx_drop;
	}

	dtls_context->flags |= NET_CONTEXT_REMOTE_ADDR_SET;

	memcpy(&dtls_context->remote, &remote_addr, sizeof(remote_addr));

	ret = net_udp_register(&dtls_context->remote,
			       &local_addr,
			       ntohs(net_sin(&dtls_context->remote)->sin_port),
			       ntohs(net_sin(&local_addr)->sin_port),
			       _net_app_dtls_established,
			       ctx,
			       &dtls_context->conn_handler);
	if (ret < 0) {
		NET_DBG("Cannot register accepted DTLS handler (%d)", ret);
		goto ctx_drop;
	}

	copy_pool_vars(dtls_context, context);

	net_context_set_state(dtls_context, NET_CONTEXT_CONNECTED);

	NET_DBG("New DTLS connection %p accepted", dtls_context);

	ctx->dtls.ctx = dtls_context;

	k_delayed_work_submit(&ctx->dtls.fin_timer, DTLS_TIMEOUT);

	return 0;

ctx_drop:
	net_context_unref(dtls_context);

pkt_drop:
	net_pkt_unref(pkt);

	return -ECONNABORTED;
}
#else /* CONFIG_NET_APP_DTLS */
#define dtls_cleanup(...)
#endif /* CONFIG_NET_APP_DTLS */

/* Receive encrypted data from network. Put that data into fifo
 * that will be read by tls thread.
 */
void _net_app_tls_received(struct net_context *context,
			   struct net_pkt *pkt,
			   int status,
			   void *user_data)
{
	struct net_app_ctx *ctx = user_data;
	struct net_app_fifo_block *rx_data = NULL;
	struct k_mem_block block;
	int ret;

	ARG_UNUSED(context);
	ARG_UNUSED(status);

	if (pkt && !net_pkt_appdatalen(pkt)) {
		net_pkt_unref(pkt);
		return;
	}

#if defined(CONFIG_NET_APP_DTLS)
	/* Client connections that are initiated by us, are passed through
	 * as is.
	 */
	if (ctx->proto == IPPROTO_UDP && ctx->app_type == NET_APP_SERVER) {
		if (ctx->dtls.ctx) {
			/* There will be a separate handler for these DTLS
			 * packets so if they are arriving here, then that is
			 * an error.
			 */
			NET_DBG("DTLS context already created, pkt %p dropped",
				pkt);
			net_pkt_unref(pkt);
			return;
		} else {
			ret = accept_dtls(ctx, context, pkt);
			if (ret < 0) {
				NET_DBG("Cannot accept new DTLS "
					"connection (%d)", ret);
				net_pkt_unref(pkt);
				return;
			}

			/* The first packet is passed as is in below code,
			 * subsequent packets are handled by dtls_established()
			 */
		}
	}
#endif /* CONFIG_NET_APP_DTLS */

	ret = k_mem_pool_alloc(ctx->tls.pool, &block,
			       sizeof(struct net_app_fifo_block),
			       BUF_ALLOC_TIMEOUT);
	if (ret < 0) {
		if (pkt) {
			net_pkt_unref(pkt);
		}

		NET_DBG("Not enough space in TLS mem pool");
		return;
	}

	rx_data = block.data;
	rx_data->pkt = pkt;
	rx_data->dir = NET_APP_PKT_RX;

	/* For freeing memory later */
	memcpy(&rx_data->block, &block, sizeof(struct k_mem_block));

	NET_DBG("Encrypted data received in pkt %p", pkt);

	k_fifo_put(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo, (void *)rx_data);

	/* Make sure that the tls handler thread runs now, even if we receive
	 * new packets.
	 */
	k_yield();
}

static int tls_sendto(struct net_app_ctx *ctx,
		      struct net_app_fifo_block *tx_data)
{
	u16_t len;
	int ret;

	len = net_pkt_appdatalen(tx_data->pkt);
	if (len == 0) {
		ret = -EINVAL;
		goto out;
	}

	ret = net_frag_linearize(ctx->tls.request_buf,
				 ctx->tls.request_buf_len,
				 tx_data->pkt,
				 net_pkt_ip_hdr_len(tx_data->pkt),
				 len);
	if (ret < 0) {
		NET_DBG("Cannot linearize send data (%d)", ret);
		goto out;
	}

	if (ret != len) {
		NET_DBG("Linear copy error (%u vs %d)", len, ret);
		ret = -EINVAL;
		goto out;
	}

	do {
		ret = mbedtls_ssl_write(&ctx->tls.mbedtls.ssl,
					ctx->tls.request_buf, len);
		if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
			_net_app_print_error(
				"peer closed the connection -0x%x", ret);
			goto out;
		}

		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (ret < 0) {
				_net_app_print_error(
					"mbedtls_ssl_write returned -0x%x",
					ret);
				goto out;
			}
		}
	} while (ret <= 0);

out:
	if (tx_data->cb) {
		tx_data->cb(net_pkt_context(tx_data->pkt), ret,
			    tx_data->token, ctx);
	}

	net_pkt_unref(tx_data->pkt);

	ctx->tls.tx_pending = false;

	return ret;
}

#if defined(CONFIG_NET_APP_DTLS)
static inline void set_remote_endpoint(struct net_app_ctx *ctx,
				       struct net_pkt *pkt)
{
	struct net_udp_hdr hdr, *udp_hdr;

	udp_hdr = net_udp_get_hdr(pkt, &hdr);
	if (!udp_hdr) {
		return;
	}

	if (net_pkt_family(pkt) == AF_INET) {
#if defined(CONFIG_NET_IPV4)
		net_sin(&ctx->ipv4.remote)->sin_port = udp_hdr->src_port;

		net_ipaddr_copy(&net_sin(&ctx->ipv4.remote)->sin_addr,
				&NET_IPV4_HDR(pkt)->src);
#endif
		return;
	}

	if (net_pkt_family(pkt) == AF_INET6) {
#if defined(CONFIG_NET_IPV6)
		net_sin6(&ctx->ipv6.remote)->sin6_port = udp_hdr->src_port;

		net_ipaddr_copy(&net_sin6(&ctx->ipv6.remote)->sin6_addr,
				&NET_IPV6_HDR(pkt)->src);
#endif
		return;
	}
}
#endif /* CONFIG_NET_APP_DTLS */

/* This will copy data from received net_pkt buf into mbedtls internal buffers.
 */
int _net_app_ssl_mux(void *context, unsigned char *buf, size_t size)
{
	struct net_app_ctx *ctx = context;
	struct net_app_fifo_block *rx_data;
	u16_t read_bytes;
	u8_t *ptr;
	int pos;
	int len;
	int ret = 0;

	if (!ctx->tls.mbedtls.ssl_ctx.frag) {
	again:
		rx_data = k_fifo_get(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo,
				     K_FOREVER);
		if (!rx_data->pkt) {
			k_mem_pool_free(&rx_data->block);
			ctx->tls.connection_closing = true;
			return -EIO;
		}

		NET_DBG("%s data in pkt %p (len %zd)",
			rx_data->dir == NET_APP_PKT_TX ? "Sending plain" :
							"Receiving encrypted",
			rx_data->pkt, net_pkt_get_len(rx_data->pkt));

		/* If the fifo contains something we need to send, then try
		 * to send it here and then go back waiting more data.
		 */
		if (rx_data->dir == NET_APP_PKT_TX) {
			tls_sendto(ctx, rx_data);
			k_mem_pool_free(&rx_data->block);
			goto again;
		}

		ctx->tls.mbedtls.ssl_ctx.rx_pkt = rx_data->pkt;

		k_mem_pool_free(&rx_data->block);

		read_bytes = net_pkt_appdatalen(
					ctx->tls.mbedtls.ssl_ctx.rx_pkt);

		ctx->tls.mbedtls.ssl_ctx.remaining = read_bytes;
		ctx->tls.mbedtls.ssl_ctx.frag =
			ctx->tls.mbedtls.ssl_ctx.rx_pkt->frags;

		ptr = net_pkt_appdata(ctx->tls.mbedtls.ssl_ctx.rx_pkt);
		len = ptr - ctx->tls.mbedtls.ssl_ctx.frag->data;

		if (len > ctx->tls.mbedtls.ssl_ctx.frag->size) {
			NET_ERR("Buf overflow (%d > %u)", len,
				ctx->tls.mbedtls.ssl_ctx.frag->size);
			return -EINVAL;
		}

		/* Save the IP header so that we can pass it to application. */
		if (!ctx->tls.mbedtls.ssl_ctx.hdr) {
			/* Only allocate a IP fragment header once. The header
			 * is same for every packet so we can ignore the
			 * duplicated one.
			 */
			ctx->tls.mbedtls.ssl_ctx.hdr =
				net_pkt_get_frag(
					ctx->tls.mbedtls.ssl_ctx.rx_pkt,
					BUF_ALLOC_TIMEOUT);

			if (ctx->tls.mbedtls.ssl_ctx.hdr) {
				net_buf_add_mem(
					ctx->tls.mbedtls.ssl_ctx.hdr,
					ctx->tls.mbedtls.ssl_ctx.frag->data,
					len);
			}
		}

		/* This will get rid of IP header */
		net_buf_pull(ctx->tls.mbedtls.ssl_ctx.frag, len);
	} else {
		read_bytes = ctx->tls.mbedtls.ssl_ctx.remaining;
		ptr = ctx->tls.mbedtls.ssl_ctx.frag->data;
	}

	len = ctx->tls.mbedtls.ssl_ctx.frag->len;
	pos = 0;
	if (read_bytes > size) {
		while (ctx->tls.mbedtls.ssl_ctx.frag) {
			read_bytes = len < (size - pos) ? len : (size - pos);

#if RX_EXTRA_DEBUG == 1
			NET_DBG("Copying %d bytes", read_bytes);
#endif

			memcpy(buf + pos, ptr, read_bytes);

			pos += read_bytes;
			if (pos < size) {
				ctx->tls.mbedtls.ssl_ctx.frag =
					ctx->tls.mbedtls.ssl_ctx.frag->frags;
				ptr = ctx->tls.mbedtls.ssl_ctx.frag->data;
				len = ctx->tls.mbedtls.ssl_ctx.frag->len;
			} else {
				if (read_bytes == len) {
					ctx->tls.mbedtls.ssl_ctx.frag =
					ctx->tls.mbedtls.ssl_ctx.frag->frags;
				} else {
					net_buf_pull(
					       ctx->tls.mbedtls.ssl_ctx.frag,
					       read_bytes);
				}

				ctx->tls.mbedtls.ssl_ctx.remaining -= size;
				return size;
			}
		}
	} else {
		while (ctx->tls.mbedtls.ssl_ctx.frag) {
#if RX_EXTRA_DEBUG == 1
			NET_DBG("Copying all %d bytes", len);
#endif

			memcpy(buf + pos, ptr, len);

			pos += len;
			ctx->tls.mbedtls.ssl_ctx.frag =
				ctx->tls.mbedtls.ssl_ctx.frag->frags;
			if (!ctx->tls.mbedtls.ssl_ctx.frag) {
				break;
			}

			ptr = ctx->tls.mbedtls.ssl_ctx.frag->data;
			len = ctx->tls.mbedtls.ssl_ctx.frag->len;
		}

#if defined(CONFIG_NET_APP_DTLS)
		if (ctx->proto == IPPROTO_UDP) {
			set_remote_endpoint(ctx,
					    ctx->tls.mbedtls.ssl_ctx.rx_pkt);
		}
#endif /* CONFIG_NET_APP_DTLS */

		net_pkt_unref(ctx->tls.mbedtls.ssl_ctx.rx_pkt);
		ctx->tls.mbedtls.ssl_ctx.rx_pkt = NULL;
		ctx->tls.mbedtls.ssl_ctx.frag = NULL;
		ctx->tls.mbedtls.ssl_ctx.remaining = 0;

		if (read_bytes != pos) {
			return -EIO;
		}

		ret = read_bytes;
	}

	return ret;
}

int _net_app_entropy_source(void *data, unsigned char *output, size_t len,
			    size_t *olen)
{
	u32_t seed;

	ARG_UNUSED(data);

	seed = sys_rand32_get();

	if (len > sizeof(seed)) {
		len = sizeof(seed);
	}

	memcpy(output, &seed, len);

	*olen = len;
	return 0;
}

int _net_app_ssl_mainloop(struct net_app_ctx *ctx)
{
	size_t len;
	int ret;

	ctx->tls.connect_cb_called = false;

reset:
	mbedtls_ssl_session_reset(&ctx->tls.mbedtls.ssl);

#if defined(CONFIG_NET_APP_DTLS)
	mbedtls_ssl_set_timer_cb(&ctx->tls.mbedtls.ssl,
				 &ctx->tls.mbedtls.timing_ctx,
				 dtls_timing_set_delay,
				 dtls_timing_get_delay);

#if defined(CONFIG_NET_APP_SERVER)
	if (ctx->app_type == NET_APP_SERVER) {
		ctx->tls.mbedtls.ssl_ctx.client_id =
			ctx->tls.mbedtls.ssl_ctx.remaining;

		ret = mbedtls_ssl_set_client_transport_id(
			&ctx->tls.mbedtls.ssl,
			&ctx->tls.mbedtls.ssl_ctx.client_id,
			sizeof(char));
		if (ret != 0) {
			_net_app_print_error(
				"mbedtls_ssl_set_client_transport_id "
				" returned -0x%x\n\n", ret);
			goto close;
		}
	}
#endif /* CONFIG_NET_APP_SERVER */
#endif /* CONFIG_NET_APP_DTLS */

	mbedtls_ssl_set_bio(&ctx->tls.mbedtls.ssl, ctx,
			    _net_app_ssl_tx, _net_app_ssl_mux, NULL);

	/* SSL handshake. The ssl_rx() function will be called next by
	 * mbedtls library. The ssl_rx() will block and wait that data is
	 * received by ssl_received() and passed to it via fifo. After
	 * receiving the data, this function will then proceed with secure
	 * connection establishment.
	 */
	/* Waiting SSL handshake */
	ctx->tls.handshake_done = false;

	NET_DBG("Starting TLS handshake");

	do {
		ret = mbedtls_ssl_handshake(&ctx->tls.mbedtls.ssl);
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			/* If we get MAC verification failure, then it usually
			 * means that we ran out of heap. As that Invalid MAC
			 * error is really confusing, give hint about possible
			 * out of memory issue.
			 */
			if (ret == MBEDTLS_ERR_SSL_INVALID_MAC) {
				NET_DBG("Check CONFIG_MBEDTLS_HEAP_SIZE as "
					"you could be out of mem in mbedtls");
			}

			if (ret < 0) {
				goto close;
			}
		}
	} while (ret != 0);

	ctx->tls.handshake_done = true;

	NET_DBG("TLS handshake done");

	/* We call the connect cb only once for each connection. The TLS
	 * might require new handshakes etc, but application does not need
	 * to care about that.
	 */
	if (!ctx->tls.connect_cb_called && ctx->cb.connect) {
		NET_DBG("Calling connect cb for ctx %p", ctx);
		ctx->cb.connect(ctx, 0, ctx->user_data);
		ctx->tls.connect_cb_called = true;
	}

	do {
	again:
		len = ctx->tls.request_buf_len - 1;
		memset(ctx->tls.request_buf, 0, ctx->tls.request_buf_len);

		ret = mbedtls_ssl_read(&ctx->tls.mbedtls.ssl,
				       ctx->tls.request_buf, len);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		    ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		}

		if (ret <= 0) {
			switch (ret) {
			case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
				NET_DBG("Connection was closed gracefully");
				goto close;

			case MBEDTLS_ERR_NET_CONN_RESET:
				NET_DBG("Connection was reset by peer");
				break;

			case -EIO:
				ctx->tls.connection_closing = true;
				break;

			default:
				_net_app_print_error(
					"mbedtls_ssl_read returned -0x%x",
					ret);
				break;
			}

			goto close;
		}

		if (ctx->cb.recv) {
			struct sockaddr dst = { 0 };
			struct net_context *net_ctx;
			struct net_pkt *pkt;
			int len = ret;
			int hdr_len = 0;

			dst.sa_family = AF_UNSPEC;

			/* If we cannot select any net_ctx, then the connection
			 * is closed already.
			 */
			net_ctx = _net_app_select_net_ctx(ctx, &dst);
			if (!net_ctx) {
				ctx->tls.connection_closing = true;
				ret = -EIO;
				goto close;
			}

			pkt = net_pkt_get_rx(net_ctx, BUF_ALLOC_TIMEOUT);
			if (!pkt) {
				ret = -ENOMEM;
				goto close;
			}

			/* Add the IP + UDP/TCP headers if found. This is done
			 * just in case the application needs to get some info
			 * from the IP header.
			 */
			if (ctx->tls.mbedtls.ssl_ctx.hdr) {
				/* Needed to skip the protocol header */
				hdr_len = ctx->tls.mbedtls.ssl_ctx.hdr->len;

				net_pkt_frag_add(pkt,
						 ctx->tls.mbedtls.ssl_ctx.hdr);
#if defined(CONFIG_NET_IPV6)
				if (net_pkt_family(pkt) == AF_INET6) {
					net_pkt_set_ip_hdr_len(pkt,
						sizeof(struct net_ipv6_hdr));
				}
#endif
#if defined(CONFIG_NET_IPV4)
				if (net_pkt_family(pkt) == AF_INET) {
					net_pkt_set_ip_hdr_len(pkt,
						sizeof(struct net_ipv4_hdr));
				}
#endif
				ctx->tls.mbedtls.ssl_ctx.hdr = NULL;
			}

			ret = net_pkt_append_all(pkt, len,
						 ctx->tls.request_buf,
						 BUF_ALLOC_TIMEOUT);
			if (!ret) {
				/* Not all data was appended */
				net_pkt_unref(pkt);
				ret = -ENOMEM;
				goto close;
			}

			net_pkt_set_appdatalen(pkt, len);

			if (hdr_len) {
				struct net_buf *frag;
				u16_t pos;

				frag = net_frag_get_pos(pkt, hdr_len, &pos);
				NET_ASSERT(frag);

				net_pkt_set_appdata(pkt, frag->data + pos);
			} else {
				net_pkt_set_appdata(pkt, pkt->frags->data);
			}

			ctx->cb.recv(ctx, pkt, 0, ctx->user_data);

			goto again;
		}

	} while (ret < 0);

	/* Read another message */
	goto reset;

close:
	/* The -EIO code means that the connection was closed. The error
	 * value is not known by mbedtls so do not print info about it.
	 */
	if (ret != -EIO) {
		_net_app_print_error("Closing connection -0x%x", ret);

		if (ctx->tls.mbedtls.ssl_ctx.hdr) {
			net_pkt_frag_unref(ctx->tls.mbedtls.ssl_ctx.hdr);
			ctx->tls.mbedtls.ssl_ctx.hdr = NULL;
		}
	}

#if defined(CONFIG_NET_APP_DTLS)
	if (ctx->proto == IPPROTO_UDP && ctx->dtls.ctx) {
		NET_DBG("Releasing DTLS context %p", ctx->dtls.ctx);

		dtls_cleanup(ctx, true);
	}
#endif /* CONFIG_NET_APP_DTLS */

	return ret;
}

int _net_app_tls_init(struct net_app_ctx *ctx, int client_or_server)
{
	enum net_sock_type sock_type;
	int ret;

	k_fifo_init(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo);
	k_sem_init(&ctx->tls.mbedtls.ssl_ctx.tx_sem, 0, UINT_MAX);

	mbedtls_platform_set_printf(MBEDTLS_PRINT);

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	if (client_or_server == MBEDTLS_SSL_IS_SERVER) {
#if defined(CONFIG_NET_APP_SERVER)
		mbedtls_x509_crt_init(&ctx->tls.mbedtls.srvcert);
#endif
	} else {
#if defined(CONFIG_NET_APP_CLIENT)
		mbedtls_x509_crt_init(&ctx->tls.mbedtls.ca_cert);
#endif
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

#if defined(CONFIG_NET_APP_SERVER)
	if (client_or_server == MBEDTLS_SSL_IS_SERVER) {
		mbedtls_pk_init(&ctx->tls.mbedtls.pkey);
	}
#endif

	mbedtls_ssl_init(&ctx->tls.mbedtls.ssl);
	mbedtls_ssl_config_init(&ctx->tls.mbedtls.conf);
	mbedtls_entropy_init(&ctx->tls.mbedtls.entropy);
	mbedtls_ctr_drbg_init(&ctx->tls.mbedtls.ctr_drbg);

#if defined(MBEDTLS_DEBUG_C) && defined(CONFIG_NET_DEBUG_APP)
	mbedtls_debug_set_threshold(DEBUG_THRESHOLD);
	mbedtls_ssl_conf_dbg(&ctx->tls.mbedtls.conf, my_debug, NULL);
#endif

	/* Seed the RNG */
	mbedtls_entropy_add_source(&ctx->tls.mbedtls.entropy,
				   ctx->tls.mbedtls.entropy_src_cb,
				   NULL,
				   MBEDTLS_ENTROPY_MAX_GATHER,
				   MBEDTLS_ENTROPY_SOURCE_STRONG);

	ret = mbedtls_ctr_drbg_seed(
		&ctx->tls.mbedtls.ctr_drbg,
		mbedtls_entropy_func,
		&ctx->tls.mbedtls.entropy,
		(const unsigned char *)ctx->tls.mbedtls.personalization_data,
		ctx->tls.mbedtls.personalization_data_len);
	if (ret != 0) {
		_net_app_print_error("mbedtls_ctr_drbg_seed returned -0x%x",
				     ret);
		goto exit;
	}

	if (ctx->sock_type == SOCK_DGRAM) {
		sock_type = MBEDTLS_SSL_TRANSPORT_DATAGRAM;
	} else {
		sock_type = MBEDTLS_SSL_TRANSPORT_STREAM;
	}

	/* Setup SSL defaults etc. */
	ret = mbedtls_ssl_config_defaults(&ctx->tls.mbedtls.conf,
					  client_or_server,
					  sock_type,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		_net_app_print_error("mbedtls_ssl_config_defaults "
				     "returned -0x%x", ret);
		goto exit;
	}

	mbedtls_ssl_conf_rng(&ctx->tls.mbedtls.conf,
			     mbedtls_ctr_drbg_random,
			     &ctx->tls.mbedtls.ctr_drbg);

#if defined(CONFIG_NET_APP_DTLS)
	if (sock_type == MBEDTLS_SSL_TRANSPORT_DATAGRAM) {
		ret = mbedtls_ssl_cookie_setup(&ctx->tls.mbedtls.cookie_ctx,
					       mbedtls_ctr_drbg_random,
					       &ctx->tls.mbedtls.ctr_drbg);
		if (ret != 0) {
			_net_app_print_error("mbedtls_ssl_cookie_setup "
					     "returned -0x%x", ret);
			goto exit;
		}

		mbedtls_ssl_conf_dtls_cookies(&ctx->tls.mbedtls.conf,
					      mbedtls_ssl_cookie_write,
					      mbedtls_ssl_cookie_check,
					      &ctx->tls.mbedtls.cookie_ctx);

		k_delayed_work_init(&ctx->dtls.fin_timer, dtls_timeout);
	}
#endif /* CONFIG_NET_APP_DTLS */

	if (client_or_server == MBEDTLS_SSL_IS_SERVER) {
		/* Load the certificates and private RSA key. This needs to be
		 * done by the user so we call a callback that user must have
		 * provided.
		 */
#if defined(CONFIG_NET_APP_SERVER)
		ret = ctx->tls.mbedtls.cert_cb(ctx, &ctx->tls.mbedtls.srvcert,
					       &ctx->tls.mbedtls.pkey);
		if (ret != 0) {
			goto exit;
		}
#endif
	} else {
#if defined(CONFIG_NET_APP_CLIENT)
		ret = ctx->tls.mbedtls.ca_cert_cb(ctx,
						  &ctx->tls.mbedtls.ca_cert);
		if (ret != 0) {
			goto exit;
		}
#endif
	}

#if defined(MBEDTLS_X509_CRT_PARSE_C) && defined(CONFIG_NET_APP_SERVER)
	if (client_or_server == MBEDTLS_SSL_IS_SERVER) {
		mbedtls_ssl_conf_ca_chain(&ctx->tls.mbedtls.conf,
					  ctx->tls.mbedtls.srvcert.next,
					  NULL);

		ret = mbedtls_ssl_conf_own_cert(&ctx->tls.mbedtls.conf,
						&ctx->tls.mbedtls.srvcert,
						&ctx->tls.mbedtls.pkey);
		if (ret != 0) {
			_net_app_print_error("mbedtls_ssl_conf_own_cert "
					     "returned -0x%x", ret);
			goto exit;
		}
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	ret = mbedtls_ssl_setup(&ctx->tls.mbedtls.ssl,
				&ctx->tls.mbedtls.conf);
	if (ret != 0) {
		_net_app_print_error("mbedtls_ssl_setup returned -0x%x", ret);
		goto exit;
	}

#if defined(MBEDTLS_X509_CRT_PARSE_C) && defined(CONFIG_NET_APP_CLIENT)
	if (client_or_server == MBEDTLS_SSL_IS_CLIENT &&
	    ctx->tls.cert_host) {
		ret = mbedtls_ssl_set_hostname(&ctx->tls.mbedtls.ssl,
					       ctx->tls.cert_host);
		if (ret != 0) {
			_net_app_print_error(
				"mbedtls_ssl_set_hostname returned -0x%x",
				ret);
			goto exit;
		}
	}
#endif

	NET_DBG("SSL %s setup done",
		client_or_server == MBEDTLS_SSL_IS_CLIENT ? "client" :
							    "server");

exit:
	/* The mbedtls resources are freed by _net_app_tls_handler_stop()
	 * which is called if this routine returns < 0
	 */
	return ret;
}

void _net_app_tls_handler_stop(struct net_app_ctx *ctx)
{
	mbedtls_ssl_free(&ctx->tls.mbedtls.ssl);
	mbedtls_ssl_config_free(&ctx->tls.mbedtls.conf);
	mbedtls_ctr_drbg_free(&ctx->tls.mbedtls.ctr_drbg);
	mbedtls_entropy_free(&ctx->tls.mbedtls.entropy);

	/* Empty the fifo just in case there is any received packets
	 * still there.
	 */
	while (1) {
		struct net_app_fifo_block *tx_rx_data;

		tx_rx_data = k_fifo_get(&ctx->tls.mbedtls.ssl_ctx.tx_rx_fifo,
					K_NO_WAIT);
		if (!tx_rx_data) {
			break;
		}

		net_pkt_unref(tx_rx_data->pkt);

		k_mem_pool_free(&tx_rx_data->block);
	}

	dtls_cleanup(ctx, true);

	NET_DBG("TLS thread %p stopped", ctx->tls.tid);

	k_thread_abort(ctx->tls.tid);
	ctx->tls.tid = 0;
}
#endif /* CONFIG_NET_APP_TLS || CONFIG_NET_APP_DTLS */

