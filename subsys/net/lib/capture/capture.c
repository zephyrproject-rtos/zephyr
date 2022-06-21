/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_capture, CONFIG_NET_CAPTURE_LOG_LEVEL);

#include <zephyr/zephyr.h>
#include <stdlib.h>
#include <zephyr/sys/slist.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/net/capture.h>
#include <zephyr/net/ethernet.h>

#include "net_private.h"
#include "ipv4.h"
#include "ipv6.h"
#include "udp_internal.h"

#define PKT_ALLOC_TIME K_MSEC(50)
#define DEFAULT_PORT 4242

#if defined(CONFIG_NET_CAPTURE_TX_DEBUG)
#define DEBUG_TX 1
#else
#define DEBUG_TX 0
#endif

static K_MUTEX_DEFINE(lock);

NET_PKT_SLAB_DEFINE(capture_pkts, CONFIG_NET_CAPTURE_PKT_COUNT);

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
NET_BUF_POOL_FIXED_DEFINE(capture_bufs, CONFIG_NET_CAPTURE_BUF_COUNT,
			  CONFIG_NET_BUF_DATA_SIZE, 4, NULL);
#else
NET_BUF_POOL_VAR_DEFINE(capture_bufs, CONFIG_NET_CAPTURE_BUF_COUNT,
			CONFIG_NET_BUF_DATA_POOL_SIZE, 4, NULL);
#endif

static sys_slist_t net_capture_devlist;

struct net_capture {
	sys_snode_t node;

	/** The capture device */
	const struct device *dev;

	/**
	 * Network interface where we are capturing network packets.
	 */
	struct net_if *capture_iface;

	/**
	 * IPIP tunnel network interface where the capture API sends the
	 * captured network packets.
	 */
	struct net_if *tunnel_iface;

	/**
	 * Network context that is used to store net_buf pool information.
	 */
	struct net_context *context;

	/**
	 * Peer (inner) tunnel IP address.
	 */
	struct sockaddr peer;

	/**
	 * Local (inner) tunnel IP address. This will be set
	 * as a local address to tunnel network interface.
	 */
	struct sockaddr local;

	/**
	 * Is this context setup already
	 */
	bool in_use : 1;

	/**
	 * Is this active or not?
	 */
	bool is_enabled : 1;

	/**
	 * Is this context initialized yet
	 */
	bool init_done : 1;
};

static struct k_mem_slab *get_net_pkt(void)
{
	return &capture_pkts;
}

static struct net_buf_pool *get_net_buf(void)
{
	return &capture_bufs;
}

void net_capture_foreach(net_capture_cb_t cb, void *user_data)
{
	struct net_capture *ctx = NULL;
	sys_snode_t *sn, *sns;

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE_SAFE(&net_capture_devlist, sn, sns) {
		struct net_capture_info info;

		ctx = CONTAINER_OF(sn, struct net_capture, node);
		if (!ctx->in_use) {
			continue;
		}

		info.capture_dev = ctx->dev;
		info.capture_iface = ctx->capture_iface;
		info.tunnel_iface = ctx->tunnel_iface;
		info.peer = &ctx->peer;
		info.local = &ctx->local;
		info.is_enabled = ctx->is_enabled;

		k_mutex_unlock(&lock);
		cb(&info, user_data);
		k_mutex_lock(&lock, K_FOREVER);
	}

	k_mutex_unlock(&lock);
}

static struct net_capture *alloc_capture_dev(void)
{
	struct net_capture *ctx = NULL;
	sys_snode_t *sn, *sns;

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE_SAFE(&net_capture_devlist, sn, sns) {
		ctx = CONTAINER_OF(sn, struct net_capture, node);
		if (ctx->in_use) {
			ctx = NULL;
			continue;
		}

		ctx->in_use = true;
		goto out;
	}

out:
	k_mutex_unlock(&lock);

	return ctx;
}

static bool is_ipip_interface(struct net_if *iface)
{
	return net_virtual_get_iface_capabilities(iface) &
					VIRTUAL_INTERFACE_IPIP;
}

static bool is_ipip_tunnel(struct net_if *iface)
{
	if ((net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) &&
	    is_ipip_interface(iface)) {
		return true;
	}

	return false;
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct net_if **ret_iface = user_data;

	if (!is_ipip_tunnel(iface)) {
		return;
	}

	*ret_iface = iface;
}

static int setup_iface(struct net_if *iface, const char *ipaddr,
		       struct sockaddr *addr, int *addr_len)
{
	struct net_if_addr *ifaddr;

	if (!net_ipaddr_parse(ipaddr, strlen(ipaddr), addr)) {
		NET_ERR("Tunnel local address \"%s\" invalid.",
			log_strdup(ipaddr));
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && addr->sa_family == AF_INET6) {
		/* No need to have dual address for IPIP tunnel interface */
		net_if_flag_clear(iface, NET_IF_IPV4);
		net_if_flag_set(iface, NET_IF_IPV6);

		ifaddr = net_if_ipv6_addr_add(iface, &net_sin6(addr)->sin6_addr,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			NET_ERR("Cannot add %s to interface %d",
				log_strdup(ipaddr), net_if_get_by_iface(iface));
			return -EINVAL;
		}

		*addr_len = sizeof(struct sockaddr_in6);

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == AF_INET) {
		struct in_addr netmask = { { { 255, 255, 255, 255 } } };

		net_if_flag_clear(iface, NET_IF_IPV6);
		net_if_flag_set(iface, NET_IF_IPV4);

		ifaddr = net_if_ipv4_addr_add(iface, &net_sin(addr)->sin_addr,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			NET_ERR("Cannot add %s to interface %d",
				log_strdup(ipaddr), net_if_get_by_iface(iface));
			return -EINVAL;
		}

		/* Set the netmask so that we do not get IPv4 traffic routed
		 * into this interface.
		 */
		net_if_ipv4_set_netmask(iface, &netmask);

		*addr_len = sizeof(struct sockaddr_in);
	} else {
		return -EINVAL;
	}

	return 0;
}

static int cleanup_iface(struct net_if *iface, struct sockaddr *addr)
{
	int ret = -EINVAL;

	if (IS_ENABLED(CONFIG_NET_IPV6) && addr->sa_family == AF_INET6) {
		ret = net_if_ipv6_addr_rm(iface, &net_sin6(addr)->sin6_addr);
		if (!ret) {
			NET_ERR("Cannot remove %s from interface %d",
				log_strdup(net_sprint_ipv6_addr(
						  &net_sin6(addr)->sin6_addr)),
				net_if_get_by_iface(iface));
			ret = -EINVAL;
		}

		net_if_flag_clear(iface, NET_IF_IPV6);

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && addr->sa_family == AF_INET) {
		ret = net_if_ipv4_addr_rm(iface, &net_sin(addr)->sin_addr);
		if (!ret) {
			NET_ERR("Cannot remove %s from interface %d",
				log_strdup(net_sprint_ipv4_addr(
						  &net_sin(addr)->sin_addr)),
				net_if_get_by_iface(iface));
		}

		net_if_flag_clear(iface, NET_IF_IPV4);
	}

	return ret;
}

int net_capture_setup(const char *remote_addr, const char *my_local_addr,
		      const char *peer_addr, const struct device **dev)
{
	struct virtual_interface_req_params params = { 0 };
	struct net_context *context = NULL;
	struct net_if *ipip_iface = NULL;
	struct sockaddr remote = { 0 };
	struct sockaddr local = { 0 };
	struct sockaddr peer = { 0 };
	struct net_if *remote_iface;
	struct net_capture *ctx;
	int local_addr_len;
	int orig_mtu;
	int ret;
	int mtu;

	if (dev == NULL || remote_addr == NULL || my_local_addr == NULL ||
	    peer_addr == NULL) {
		ret = -EINVAL;
		goto fail;
	}

	if (!net_ipaddr_parse(remote_addr, strlen(remote_addr), &remote)) {
		NET_ERR("IPIP tunnel %s address \"%s\" invalid.",
			"remote", remote_addr);
		ret = -EINVAL;
		goto fail;
	}

	if (!net_ipaddr_parse(peer_addr, strlen(peer_addr), &peer)) {
		NET_ERR("IPIP tunnel %s address \"%s\" invalid.",
			"peer", peer_addr);
		ret = -EINVAL;
		goto fail;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && remote.sa_family == AF_INET6) {
		remote_iface = net_if_ipv6_select_src_iface(
						&net_sin6(&remote)->sin6_addr);
		params.family = AF_INET6;
		net_ipaddr_copy(&params.peer6addr,
				&net_sin6(&remote)->sin6_addr);
		orig_mtu = net_if_get_mtu(remote_iface);
		mtu = orig_mtu - sizeof(struct net_ipv6_hdr) -
			sizeof(struct net_udp_hdr);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && remote.sa_family == AF_INET) {
		remote_iface = net_if_ipv4_select_src_iface(
						&net_sin(&remote)->sin_addr);
		params.family = AF_INET;
		net_ipaddr_copy(&params.peer4addr,
				&net_sin(&remote)->sin_addr);
		orig_mtu = net_if_get_mtu(remote_iface);
		mtu = orig_mtu - sizeof(struct net_ipv4_hdr) -
			sizeof(struct net_udp_hdr);
	} else {
		NET_ERR("Invalid address family %d", remote.sa_family);
		ret = -EINVAL;
		goto fail;
	}

	if (remote_iface == NULL) {
		NET_ERR("Remote address %s unreachable", remote_addr);
		ret = -ENETUNREACH;
		goto fail;
	}

	/* We only get net_context so that net_pkt allocation routines
	 * can allocate net_buf's from our net_buf pool.
	 */
	ret = net_context_get(params.family, SOCK_DGRAM, IPPROTO_UDP,
			      &context);
	if (ret < 0) {
		NET_ERR("Cannot allocate net_context (%d)", ret);
		return ret;
	}

	/* Then select the IPIP tunnel. The capture device is hooked to it.
	 */
	net_if_foreach(iface_cb, &ipip_iface);

	if (ipip_iface == NULL) {
		NET_ERR("Cannot find available %s interface", "ipip");
		ret = -ENOENT;
		goto fail;
	}

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PEER_ADDRESS,
		       ipip_iface, &params, sizeof(params));
	if (ret < 0) {
		NET_ERR("Cannot set remote address %s to interface %d (%d)",
			remote_addr, net_if_get_by_iface(ipip_iface), ret);
		goto fail;
	}

	params.mtu = orig_mtu;

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_MTU,
		       ipip_iface, &params, sizeof(params));
	if (ret < 0) {
		NET_ERR("Cannot set interface %d MTU to %d (%d)",
			net_if_get_by_iface(ipip_iface), params.mtu, ret);
		goto fail;
	}

	ret = setup_iface(ipip_iface, my_local_addr, &local, &local_addr_len);
	if (ret < 0) {
		NET_ERR("Cannot set IP address %s to tunnel interface",
			my_local_addr);
		goto fail;
	}

	if (peer.sa_family != local.sa_family) {
		NET_ERR("Peer and local address are not the same family "
			"(%d vs %d)", peer.sa_family, local.sa_family);
		ret = -EINVAL;
		goto fail;
	}

	ctx = alloc_capture_dev();
	if (ctx == NULL) {
		ret = -ENOMEM;
		goto fail;
	}

	/* Lower the remote interface MTU so that our packets can fit to it */
	net_if_set_mtu(remote_iface, mtu);

	ctx->context = context;
	net_context_setup_pools(ctx->context, get_net_pkt, get_net_buf);

	ctx->tunnel_iface = ipip_iface;
	*dev = ctx->dev;

	memcpy(&ctx->peer, &peer, local_addr_len);
	memcpy(&ctx->local, &local, local_addr_len);

	if (net_sin(&ctx->peer)->sin_port == 0) {
		net_sin(&ctx->peer)->sin_port = htons(DEFAULT_PORT);
	}

	if (net_sin(&ctx->local)->sin_port == 0) {
		net_sin(&ctx->local)->sin_port = htons(DEFAULT_PORT);
	}

	ret = net_virtual_interface_attach(ctx->tunnel_iface, remote_iface);
	if (ret < 0 && ret != -EALREADY) {
		NET_ERR("Cannot attach IPIP interface %d to interface %d",
			net_if_get_by_iface(ipip_iface),
			net_if_get_by_iface(remote_iface));
		(void)net_capture_cleanup(ctx->dev);

		/* net_context is cleared by the cleanup so no need to goto
		 * to fail label.
		 */
		return ret;
	}

	net_virtual_set_name(ipip_iface, "Capture tunnel");

	return 0;

fail:
	if (context) {
		net_context_unref(context);
	}

	return ret;
}

static int capture_cleanup(const struct device *dev)
{
	struct net_capture *ctx = dev->data;

	(void)net_capture_disable(dev);
	(void)net_virtual_interface_attach(ctx->tunnel_iface, NULL);

	if (ctx->context) {
		net_context_put(ctx->context);
	}

	(void)cleanup_iface(ctx->tunnel_iface, &ctx->local);

	ctx->tunnel_iface = NULL;
	ctx->in_use = false;

	return 0;
}

static bool capture_is_enabled(const struct device *dev)
{
	struct net_capture *ctx = dev->data;

	return ctx->is_enabled ? true : false;
}

static int capture_enable(const struct device *dev, struct net_if *iface)
{
	struct net_capture *ctx = dev->data;

	if (ctx->is_enabled) {
		return -EALREADY;
	}

	/* We cannot capture the tunnel interface as that would cause
	 * recursion.
	 */
	if (ctx->tunnel_iface == iface) {
		return -EINVAL;
	}

	ctx->capture_iface = iface;
	ctx->is_enabled = true;

	net_if_up(ctx->tunnel_iface);

	return 0;
}

static int capture_disable(const struct device *dev)
{
	struct net_capture *ctx = dev->data;

	ctx->capture_iface = NULL;
	ctx->is_enabled = false;

	net_if_down(ctx->tunnel_iface);

	return 0;
}

void net_capture_pkt(struct net_if *iface, struct net_pkt *pkt)
{
	struct k_mem_slab *orig_slab;
	struct net_pkt *captured;
	sys_snode_t *sn, *sns;

	/* We must prevent to capture network packet that is already captured
	 * in order to avoid recursion.
	 */
	if (net_pkt_is_captured(pkt)) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE_SAFE(&net_capture_devlist, sn, sns) {
		struct net_capture *ctx = CONTAINER_OF(sn, struct net_capture,
						       node);
		int ret;

		if (!ctx->in_use || !ctx->is_enabled ||
		    ctx->capture_iface != iface) {
			continue;
		}

		orig_slab = pkt->slab;
		pkt->slab = get_net_pkt();

		captured = net_pkt_clone(pkt, K_NO_WAIT);

		pkt->slab = orig_slab;

		if (captured == NULL) {
			NET_DBG("Captured pkt %s", "dropped");
			/* TODO: update capture data statistics */
			goto out;
		}

		net_pkt_set_orig_iface(captured, iface);
		net_pkt_set_iface(captured, ctx->tunnel_iface);
		net_pkt_set_captured(pkt, true);

		ret = net_capture_send(ctx->dev, ctx->tunnel_iface, captured);
		if (ret < 0) {
			net_pkt_unref(captured);
		}

		goto out;
	}

out:
	k_mutex_unlock(&lock);
}

static int capture_dev_init(const struct device *dev)
{
	struct net_capture *ctx = dev->data;

	k_mutex_lock(&lock, K_FOREVER);

	sys_slist_find_and_remove(&net_capture_devlist, &ctx->node);
	sys_slist_prepend(&net_capture_devlist, &ctx->node);

	ctx->dev = dev;
	ctx->init_done = true;

	k_mutex_unlock(&lock);

	return 0;
}

static int capture_send(const struct device *dev, struct net_if *iface,
			struct net_pkt *pkt)
{
	struct net_capture *ctx = dev->data;
	enum net_verdict verdict;
	struct net_pkt *ip;
	int ret;
	int len;

	if (!ctx->in_use) {
		return -ENOENT;
	}

	if (ctx->local.sa_family == AF_INET) {
		len = sizeof(struct net_ipv4_hdr);
	} else if (ctx->local.sa_family == AF_INET6) {
		len = sizeof(struct net_ipv6_hdr);
	} else {
		return -EINVAL;
	}

	len += sizeof(struct net_udp_hdr);

	/* Add IP and UDP header */
	ip = net_pkt_alloc_from_slab(ctx->context->tx_slab(), PKT_ALLOC_TIME);
	if (!ip) {
		return -ENOMEM;
	}

	net_pkt_set_context(ip, ctx->context);
	net_pkt_set_family(ip, ctx->local.sa_family);
	net_pkt_set_iface(ip, ctx->tunnel_iface);

	ret = net_pkt_alloc_buffer(ip, len, IPPROTO_UDP, PKT_ALLOC_TIME);
	if (ret < 0) {
		net_pkt_unref(ip);
		return ret;
	}

	if (ctx->local.sa_family == AF_INET) {
		net_pkt_set_ipv4_ttl(ip,
				     net_if_ipv4_get_ttl(ctx->tunnel_iface));

		ret = net_ipv4_create(ip, &net_sin(&ctx->local)->sin_addr,
				      &net_sin(&ctx->peer)->sin_addr);
	} else {
		ret = net_ipv6_create(ip, &net_sin6(&ctx->local)->sin6_addr,
				      &net_sin6(&ctx->peer)->sin6_addr);
	}

	if (ret < 0) {
		net_pkt_unref(ip);
		return ret;
	}

	(void)net_udp_create(ip, net_sin(&ctx->local)->sin_port,
			     net_sin(&ctx->peer)->sin_port);

	net_buf_frag_add(ip->buffer, pkt->buffer);
	pkt->buffer = ip->buffer;
	ip->buffer = NULL;
	net_pkt_unref(ip);

	/* Clear the context if it was set as the pkt was cloned and we
	 * do not want to affect the original pkt.
	 */
	net_pkt_set_context(pkt, NULL);
	net_pkt_set_captured(pkt, true);
	net_pkt_set_iface(pkt, ctx->tunnel_iface);
	net_pkt_set_family(pkt, ctx->local.sa_family);
	net_pkt_set_ipv6_ext_len(pkt, 0);

	net_pkt_cursor_init(pkt);

	if (ctx->local.sa_family == AF_INET) {
		net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
		net_pkt_set_ipv4_opts_len(pkt, 0);

		net_ipv4_finalize(pkt, IPPROTO_UDP);
	} else {
		net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
		net_pkt_set_ipv6_ext_opt_len(pkt, 0);

		net_ipv6_finalize(pkt, IPPROTO_UDP);
	}

	if (DEBUG_TX) {
		char str[sizeof("TX iface xx")];

		snprintk(str, sizeof(str), "TX iface %d",
			 net_if_get_by_iface(net_pkt_iface(pkt)));

		net_pkt_hexdump(pkt, str);
	}

	net_pkt_cursor_init(pkt);

	verdict = net_if_send_data(ctx->tunnel_iface, pkt);
	if (verdict == NET_DROP) {
		ret = -EIO;
	}

	return ret;
}

static const struct net_capture_interface_api capture_interface_api = {
	.cleanup = capture_cleanup,
	.enable = capture_enable,
	.disable = capture_disable,
	.is_enabled = capture_is_enabled,
	.send = capture_send,
};

#define DEFINE_NET_CAPTURE_DEV_DATA(x, _)				\
	static struct net_capture capture_dev_data_##x

#define DEFINE_NET_CAPTURE_DEVICE(x, _)					\
	DEVICE_DEFINE(net_capture_##x,					\
		      "NET_CAPTURE" #x,					\
		      &capture_dev_init,				\
		      NULL,						\
		      &capture_dev_data_##x,				\
		      NULL,						\
		      POST_KERNEL,					\
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
		      &capture_interface_api)

LISTIFY(CONFIG_NET_CAPTURE_DEVICE_COUNT, DEFINE_NET_CAPTURE_DEV_DATA, (;), _);
LISTIFY(CONFIG_NET_CAPTURE_DEVICE_COUNT, DEFINE_NET_CAPTURE_DEVICE, (;), _);
