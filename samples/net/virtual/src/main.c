/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_virtual_interface_sample, LOG_LEVEL_DBG);

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>

/* User data for the interface callback */
struct ud {
	struct net_if *my_iface;
	struct net_if *ethernet;
	struct net_if *ip_tunnel_1;
	struct net_if *ip_tunnel_2;
	struct net_if *ipip;
};

/* The MTU value here is just an arbitrary number for testing purposes */
#define VIRTUAL_TEST_MTU 1024

#define VIRTUAL_TEST "VIRTUAL_TEST"
#define VIRTUAL_TEST2 "VIRTUAL_TEST2"
#define VIRTUAL_TEST3 "VIRTUAL_TEST3"
static const char *dev_name = VIRTUAL_TEST;
static const char *ipip_dev_name = "IP_tunnel";

struct virtual_test_context {
	struct net_if *iface;
	struct net_if *attached_to;
	bool status;
	bool init_done;
};

static void virtual_test_iface_init(struct net_if *iface)
{
	struct virtual_test_context *ctx = net_if_get_device(iface)->data;
	char name[sizeof("VirtualTest-+##########")];
	static int count;

	if (ctx->init_done) {
		return;
	}

	ctx->iface = iface;
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	snprintk(name, sizeof(name), "VirtualTest-%d", count + 1);
	count++;
	net_virtual_set_name(iface, name);
	(void)net_virtual_set_flags(iface, NET_L2_POINT_TO_POINT);

	ctx->init_done = true;
}

static struct virtual_test_context virtual_test_context_data1 = {
};

static struct virtual_test_context virtual_test_context_data2 = {
};

static struct virtual_test_context virtual_test_context_data3 = {
};

static int virtual_test_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static enum virtual_interface_caps
virtual_test_get_capabilities(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return (enum virtual_interface_caps)0;
}

static int virtual_test_interface_start(const struct device *dev)
{
	struct virtual_test_context *ctx = dev->data;

	if (ctx->status) {
		return -EALREADY;
	}

	ctx->status = true;

	LOG_DBG("Starting iface %d", net_if_get_by_iface(ctx->iface));

	/* You can implement here any special action that is needed
	 * when the network interface is coming up.
	 */

	return 0;
}

static int virtual_test_interface_stop(const struct device *dev)
{
	struct virtual_test_context *ctx = dev->data;

	if (!ctx->status) {
		return -EALREADY;
	}

	ctx->status = false;

	LOG_DBG("Stopping iface %d", net_if_get_by_iface(ctx->iface));

	/* You can implement here any special action that is needed
	 * when the network interface is going down.
	 */

	return 0;
}

static int virtual_test_interface_send(struct net_if *iface,
				       struct net_pkt *pkt)
{
	struct virtual_test_context *ctx = net_if_get_device(iface)->data;

	if (ctx->attached_to == NULL) {
		return -ENOENT;
	}

	return net_send_data(pkt);
}

static enum net_verdict virtual_test_interface_recv(struct net_if *iface,
						    struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(pkt);

	return NET_CONTINUE;
}

static int virtual_test_interface_attach(struct net_if *virtual_iface,
					 struct net_if *iface)
{
	struct virtual_test_context *ctx = net_if_get_device(virtual_iface)->data;

	LOG_INF("This interface %d/%p attached to %d/%p",
		net_if_get_by_iface(virtual_iface), virtual_iface,
		net_if_get_by_iface(iface), iface);

	ctx->attached_to = iface;

	return 0;
}

static const struct virtual_interface_api virtual_test_iface_api = {
	.iface_api.init = virtual_test_iface_init,

	.get_capabilities = virtual_test_get_capabilities,
	.start = virtual_test_interface_start,
	.stop = virtual_test_interface_stop,
	.send = virtual_test_interface_send,
	.recv = virtual_test_interface_recv,
	.attach = virtual_test_interface_attach,
};

NET_VIRTUAL_INTERFACE_INIT(virtual_test1, VIRTUAL_TEST,
			   virtual_test_init, NULL,
			   &virtual_test_context_data1,
			   NULL,
			   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			   &virtual_test_iface_api,
			   VIRTUAL_TEST_MTU);

NET_VIRTUAL_INTERFACE_INIT(virtual_test2, VIRTUAL_TEST2,
			   virtual_test_init, NULL,
			   &virtual_test_context_data2,
			   NULL,
			   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			   &virtual_test_iface_api,
			   VIRTUAL_TEST_MTU);

NET_VIRTUAL_INTERFACE_INIT(virtual_test3, VIRTUAL_TEST3,
			   virtual_test_init, NULL,
			   &virtual_test_context_data3,
			   NULL,
			   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			   &virtual_test_iface_api,
			   VIRTUAL_TEST_MTU);

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *ud = user_data;

	if ((net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) &&
	    (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL))) {
		return;
	}

	if (!ud->ethernet && net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		ud->ethernet = iface;
		return;
	}

	if (!ud->ipip && net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL) &&
	    net_if_get_device(iface) == device_get_binding(ipip_dev_name)) {
		ud->ipip = iface;
		return;
	}

	if (!ud->my_iface && net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL) &&
	    net_if_get_device(iface)->data == &virtual_test_context_data1) {
		ud->my_iface = iface;
		return;
	}

	if (!ud->ip_tunnel_1 && net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		ud->ip_tunnel_1 = iface;
		return;
	}

	if (!ud->ip_tunnel_2 && net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		ud->ip_tunnel_2 = iface;
		return;
	}
}

static int setup_iface(struct net_if *iface,
		       const char *ipv6_addr,
		       const char *ipv4_addr,
		       const char *peer6addr,
		       const char *peer4addr,
		       const char *netmask)
{
	struct virtual_interface_req_params params = { 0 };
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	struct in6_addr addr6;
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    net_if_flag_is_set(iface, NET_IF_IPV6)) {

		if (net_addr_pton(AF_INET6, ipv6_addr, &addr6)) {
			LOG_ERR("Invalid address: %s", ipv6_addr);
			return -EINVAL;
		}

		ifaddr = net_if_ipv6_addr_add(iface, &addr6,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			LOG_ERR("Cannot add %s to interface %p",
				ipv6_addr, iface);
			return -EINVAL;
		}

		if (!peer6addr || *peer6addr == '\0') {
			goto try_ipv4;
		}

		params.family = AF_INET6;

		if (net_addr_pton(AF_INET6, peer6addr, &addr6)) {
			LOG_ERR("Cannot parse peer %s address %s to tunnel",
				"IPv6", peer6addr);
		} else {
			net_ipaddr_copy(&params.peer6addr, &addr6);

			ret = net_mgmt(
				NET_REQUEST_VIRTUAL_INTERFACE_SET_PEER_ADDRESS,
				iface, &params, sizeof(params));
			if (ret < 0 && ret != -ENOTSUP) {
				LOG_ERR("Cannot set peer %s address %s to "
					"interface %d (%d)",
					"IPv6", peer6addr,
					net_if_get_by_iface(iface),
					ret);
			}
		}

		params.mtu = NET_ETH_MTU - sizeof(struct net_ipv6_hdr);

		ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_MTU,
			       iface, &params, sizeof(params));
		if (ret < 0 && ret != -ENOTSUP) {
			LOG_ERR("Cannot set interface %d MTU to %d (%d)",
				net_if_get_by_iface(iface), params.mtu, ret);
		}
	}

try_ipv4:
	if (IS_ENABLED(CONFIG_NET_IPV4) &&
	    net_if_flag_is_set(iface, NET_IF_IPV4)) {

		if (net_addr_pton(AF_INET, ipv4_addr, &addr4)) {
			LOG_ERR("Invalid address: %s", ipv4_addr);
			return -EINVAL;
		}

		ifaddr = net_if_ipv4_addr_add(iface, &addr4,
					      NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			LOG_ERR("Cannot add %s to interface %p",
				ipv4_addr, iface);
			return -EINVAL;
		}

		if (netmask) {
			if (net_addr_pton(AF_INET, netmask, &addr4)) {
				LOG_ERR("Invalid netmask: %s", netmask);
				return -EINVAL;
			}

			net_if_ipv4_set_netmask(iface, &addr4);
		}

		if (!peer4addr || *peer4addr == '\0') {
			goto done;
		}

		params.family = AF_INET;

		if (net_addr_pton(AF_INET, peer4addr, &addr4)) {
			LOG_ERR("Cannot parse peer %s address %s to tunnel",
				"IPv4", peer4addr);
		} else {
			net_ipaddr_copy(&params.peer4addr, &addr4);

			ret = net_mgmt(
				NET_REQUEST_VIRTUAL_INTERFACE_SET_PEER_ADDRESS,
				iface, &params, sizeof(params));
			if (ret < 0 && ret != -ENOTSUP) {
				LOG_ERR("Cannot set peer %s address %s to "
					"interface %d (%d)",
					"IPv4", peer4addr,
					net_if_get_by_iface(iface),
					ret);
			}
		}

		params.mtu = NET_ETH_MTU - sizeof(struct net_ipv4_hdr);

		ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_MTU,
			       iface, &params,
			       sizeof(struct virtual_interface_req_params));
		if (ret < 0 && ret != -ENOTSUP) {
			LOG_ERR("Cannot set interface %d MTU to %d (%d)",
				net_if_get_by_iface(iface), params.mtu, ret);
		}
	}

done:
	return 0;
}

void main(void)
{
#define MAX_NAME_LEN 32
	char buf[MAX_NAME_LEN];
	struct ud ud;
	int ret;

	LOG_INF("Start application (dev %s/%p)", dev_name,
		device_get_binding(dev_name));

	memset(&ud, 0, sizeof(ud));
	net_if_foreach(iface_cb, &ud);

	LOG_INF("My example tunnel interface %d (%s / %p)",
		net_if_get_by_iface(ud.my_iface),
		net_virtual_get_name(ud.my_iface, buf, sizeof(buf)),
		ud.my_iface);
	LOG_INF("Tunnel interface %d (%s / %p)",
		net_if_get_by_iface(ud.ip_tunnel_1),
		net_virtual_get_name(ud.ip_tunnel_1, buf, sizeof(buf)),
		ud.ip_tunnel_1);
	LOG_INF("Tunnel interface %d (%s / %p)",
		net_if_get_by_iface(ud.ip_tunnel_2),
		net_virtual_get_name(ud.ip_tunnel_2, buf, sizeof(buf)),
		ud.ip_tunnel_2);
	LOG_INF("IPIP interface %d (%p)",
		net_if_get_by_iface(ud.ipip), ud.ipip);
	LOG_INF("Ethernet interface %d (%p)",
		net_if_get_by_iface(ud.ethernet), ud.ethernet);

	/* Attach the network interfaces on top of the Ethernet interface */
	net_virtual_interface_attach(ud.ip_tunnel_1, ud.ethernet);

	/* Attach our example virtual interface on top of the IPv4 one.
	 * This is just an example how to stack the interface on top of
	 * each other.
	 */
	net_virtual_interface_attach(ud.my_iface, ud.ip_tunnel_1);
	net_virtual_interface_attach(ud.my_iface, ud.ip_tunnel_2);

	ret = setup_iface(ud.my_iface,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV6_ADDR,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV4_ADDR,
			  NULL, NULL,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV4_NETMASK);
	if (ret < 0) {
		LOG_ERR("Cannot set IP address to test interface");
	}

	if (ud.ethernet) {
		ret = setup_iface(ud.ethernet,
				  CONFIG_NET_CONFIG_MY_IPV6_ADDR,
				  CONFIG_NET_CONFIG_MY_IPV4_ADDR,
				  NULL, NULL,
				  CONFIG_NET_CONFIG_MY_IPV4_NETMASK);
		if (ret < 0) {
			LOG_ERR("Cannot set IP address to Ethernet interface");
		}
	}

	ret = setup_iface(ud.ipip,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV6_ADDR,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV4_ADDR,
			  CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
			  CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV4_NETMASK);
	if (ret < 0) {
		LOG_ERR("Cannot set IP address to IPIP tunnel");
	}

	/* This sample application does nothing itself. You can use
	 * net-shell to send ping or UDP/TCP packets for testing
	 * purposes.
	 */
}
