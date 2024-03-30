/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/sys_clock.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/ethernet_mgmt.h>

#define MAX_BUF_LEN 64
#define STACK_SIZE 1024
#define THREAD_PRIORITY K_PRIO_COOP(8)

static struct net_if *default_iface;

static ZTEST_BMEM int fd;
static ZTEST_BMEM struct in6_addr addr_v6;
static ZTEST_DMEM struct in_addr addr_v4 = { { { 192, 0, 2, 3 } } };

#if defined(CONFIG_NET_SOCKETS_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static const uint8_t mac_addr_init[6] = { 0x01, 0x02, 0x03,
				       0x04,  0x05,  0x06 };

struct eth_fake_context {
	struct net_if *iface;
	uint8_t mac_address[6];

	bool auto_negotiation;
	bool full_duplex;
	bool link_10bt;
	bool link_100bt;
	bool promisc_mode;
	struct {
		bool qav_enabled;
		int idle_slope;
		int delta_bandwidth;
	} priority_queues[2];
};

static struct eth_fake_context eth_fake_data;

static void eth_fake_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_address,
			     sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev,
			 struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static int eth_fake_get_total_bandwidth(struct eth_fake_context *ctx)
{
	if (ctx->link_100bt) {
		return 100 * 1000 * 1000 / 8;
	}

	if (ctx->link_10bt) {
		return 10 * 1000 * 1000 / 8;
	}

	/* No link */
	return 0;
}

static void eth_fake_recalc_qav_delta_bandwidth(struct eth_fake_context *ctx)
{
	int bw;
	int i;

	bw = eth_fake_get_total_bandwidth(ctx);

	for (i = 0; i < ARRAY_SIZE(ctx->priority_queues); ++i) {
		if (bw == 0) {
			ctx->priority_queues[i].delta_bandwidth = 0;
		} else {
			ctx->priority_queues[i].delta_bandwidth =
				(ctx->priority_queues[i].idle_slope * 100);

			ctx->priority_queues[i].delta_bandwidth /= bw;
		}
	}
}

static void eth_fake_recalc_qav_idle_slopes(struct eth_fake_context *ctx)
{
	int bw;
	int i;

	bw = eth_fake_get_total_bandwidth(ctx);

	for (i = 0; i < ARRAY_SIZE(ctx->priority_queues); ++i) {
		ctx->priority_queues[i].idle_slope =
			(ctx->priority_queues[i].delta_bandwidth * bw) / 100;
	}
}

static int eth_fake_set_config(const struct device *dev,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	struct eth_fake_context *ctx = dev->data;
	int priority_queues_num = ARRAY_SIZE(ctx->priority_queues);
	enum ethernet_qav_param_type qav_param_type;
	int queue_id;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_QAV_PARAM:
		queue_id = config->qav_param.queue_id;
		qav_param_type = config->qav_param.type;

		if (queue_id < 0 || queue_id >= priority_queues_num) {
			return -EINVAL;
		}

		switch (qav_param_type) {
		case ETHERNET_QAV_PARAM_TYPE_STATUS:
			ctx->priority_queues[queue_id].qav_enabled =
				config->qav_param.enabled;
			break;
		case ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE:
			ctx->priority_queues[queue_id].idle_slope =
				config->qav_param.idle_slope;

			eth_fake_recalc_qav_delta_bandwidth(ctx);
			break;
		case ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH:
			ctx->priority_queues[queue_id].delta_bandwidth =
				config->qav_param.delta_bandwidth;

			eth_fake_recalc_qav_idle_slopes(ctx);
			break;
		default:
			return -ENOTSUP;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int eth_fake_get_config(const struct device *dev,
			       enum ethernet_config_type type,
			       struct ethernet_config *config)
{
	struct eth_fake_context *ctx = dev->data;
	int priority_queues_num = ARRAY_SIZE(ctx->priority_queues);
	enum ethernet_qav_param_type qav_param_type;
	int queue_id;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_QAV_PARAM:
		queue_id = config->qav_param.queue_id;
		qav_param_type = config->qav_param.type;

		if (queue_id < 0 || queue_id >= priority_queues_num) {
			return -EINVAL;
		}

		switch (qav_param_type) {
		case ETHERNET_QAV_PARAM_TYPE_STATUS:
			config->qav_param.enabled =
				ctx->priority_queues[queue_id].qav_enabled;
			break;
		case ETHERNET_QAV_PARAM_TYPE_IDLE_SLOPE:
		case ETHERNET_QAV_PARAM_TYPE_OPER_IDLE_SLOPE:
			/* No distinction between idle slopes for fake eth */
			config->qav_param.idle_slope =
				ctx->priority_queues[queue_id].idle_slope;
			break;
		case ETHERNET_QAV_PARAM_TYPE_DELTA_BANDWIDTH:
			config->qav_param.delta_bandwidth =
				ctx->priority_queues[queue_id].delta_bandwidth;
			break;
		case ETHERNET_QAV_PARAM_TYPE_TRAFFIC_CLASS:
			/* Default TC for BE - it doesn't really matter here */
			config->qav_param.traffic_class =
				net_tx_priority2tc(NET_PRIORITY_BE);
			break;
		default:
			return -ENOTSUP;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static enum ethernet_hw_caps eth_fake_get_capabilities(const struct device *dev)
{
	return ETHERNET_AUTO_NEGOTIATION_SET | ETHERNET_LINK_10BASE_T |
		ETHERNET_LINK_100BASE_T | ETHERNET_DUPLEX_SET | ETHERNET_QAV |
		ETHERNET_PROMISC_MODE | ETHERNET_PRIORITY_QUEUES;
}

static struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,

	.get_capabilities = eth_fake_get_capabilities,
	.set_config = eth_fake_set_config,
	.get_config = eth_fake_get_config,
	.send = eth_fake_send,
};

static int eth_fake_init(const struct device *dev)
{
	struct eth_fake_context *ctx = dev->data;
	int i;

	ctx->auto_negotiation = true;
	ctx->full_duplex = true;
	ctx->link_10bt = true;
	ctx->link_100bt = false;

	memcpy(ctx->mac_address, mac_addr_init, 6);

	/* Initialize priority queues */
	for (i = 0; i < ARRAY_SIZE(ctx->priority_queues); ++i) {
		ctx->priority_queues[i].qav_enabled = true;
		if (i + 1 == ARRAY_SIZE(ctx->priority_queues)) {
			/* 75% for the last priority queue */
			ctx->priority_queues[i].delta_bandwidth = 75;
		} else {
			/* 0% for the rest */
			ctx->priority_queues[i].delta_bandwidth = 0;
		}
	}

	eth_fake_recalc_qav_idle_slopes(ctx);

	return 0;
}

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", eth_fake_init, NULL,
		    &eth_fake_data, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

/* A test thread that spits out events that we can catch and show to user */
static void trigger_events(void)
{
	int operation = 0;
	struct net_if_addr *ifaddr_v6, *ifaddr_v4;
	struct net_if *iface;
	int ret;

	iface = default_iface;

	net_ipv6_addr_create(&addr_v6, 0x2001, 0x0db8, 0, 0, 0, 0, 0, 0x0003);

	while (1) {
		switch (operation) {
		case 0:
			ifaddr_v6 = net_if_ipv6_addr_add(iface, &addr_v6,
							 NET_ADDR_MANUAL, 0);
			if (!ifaddr_v6) {
				LOG_ERR("Cannot add IPv%c address", '6');
				break;
			}

			break;
		case 1:
			ifaddr_v4 = net_if_ipv4_addr_add(iface, &addr_v4,
							 NET_ADDR_MANUAL, 0);
			if (!ifaddr_v4) {
				LOG_ERR("Cannot add IPv%c address", '4');
				break;
			}

			break;
		case 2:
			ret = net_if_ipv6_addr_rm(iface, &addr_v6);
			if (!ret) {
				LOG_ERR("Cannot del IPv%c address", '6');
				break;
			}

			break;
		case 3:
			ret = net_if_ipv4_addr_rm(iface, &addr_v4);
			if (!ret) {
				LOG_ERR("Cannot del IPv%c address", '4');
				break;
			}

			break;
		default:
			operation = -1;
			break;
		}

		operation++;

		k_sleep(K_MSEC(100));
	}
}

K_THREAD_DEFINE(trigger_events_thread_id, STACK_SIZE,
		trigger_events, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

static char *get_ip_addr(char *ipaddr, size_t len, sa_family_t family,
			 struct net_mgmt_msghdr *hdr)
{
	char *buf;

	buf = net_addr_ntop(family, hdr->nm_msg, ipaddr, len);
	if (!buf) {
		return "?";
	}

	return buf;
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct net_if **my_iface = user_data;

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		if (PART_OF_ARRAY(NET_IF_GET_NAME(eth_fake, 0), iface)) {
			*my_iface = iface;
		}
	}
}

static void test_net_mgmt_setup(void)
{
	struct sockaddr_nm sockaddr;
	int ret;

	net_if_foreach(iface_cb, &default_iface);
	zassert_not_null(default_iface, "Cannot find test interface");

	fd = socket(AF_NET_MGMT, SOCK_DGRAM, NET_MGMT_EVENT_PROTO);
	zassert_false(fd < 0, "Cannot create net_mgmt socket (%d)", errno);

#ifdef CONFIG_USERSPACE
	/* Set the underlying net_context to global access scope so that
	 * other scenario threads may use it
	 */
	void *ctx = zsock_get_context_object(fd);

	zassert_not_null(ctx, "null net_context");
	k_object_access_all_grant(ctx);
#endif /* CONFIG_USERSPACE */

	memset(&sockaddr, 0, sizeof(sockaddr));

	sockaddr.nm_family = AF_NET_MGMT;
	sockaddr.nm_ifindex = net_if_get_by_iface(default_iface);
	sockaddr.nm_pid = (uintptr_t)k_current_get();
	sockaddr.nm_mask = NET_EVENT_IPV6_DAD_SUCCEED |
			   NET_EVENT_IPV6_ADDR_ADD |
			   NET_EVENT_IPV6_ADDR_DEL;

	ret = bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	zassert_false(ret < 0, "Cannot bind net_mgmt socket (%d)", errno);

	k_thread_start(trigger_events_thread_id);
}

static void test_net_mgmt_catch_events(void)
{
	struct sockaddr_nm event_addr;
	socklen_t event_addr_len;
	char ipaddr[INET6_ADDRSTRLEN];
	uint8_t buf[MAX_BUF_LEN];
	int event_count = 2;
	int ret;

	while (event_count > 0) {
		struct net_mgmt_msghdr *hdr;

		memset(buf, 0, sizeof(buf));
		event_addr_len = sizeof(event_addr);

		ret = recvfrom(fd, buf, sizeof(buf), 0,
			       (struct sockaddr *)&event_addr,
			       &event_addr_len);
		if (ret < 0) {
			continue;
		}

		hdr = (struct net_mgmt_msghdr *)buf;

		if (hdr->nm_msg_version != NET_MGMT_SOCKET_VERSION_1) {
			/* Do not know how to parse the message */
			continue;
		}

		switch (event_addr.nm_mask) {
		case NET_EVENT_IPV6_ADDR_ADD:
			DBG("IPv6 address added to interface %d (%s)\n",
			    event_addr.nm_ifindex,
			    get_ip_addr(ipaddr, sizeof(ipaddr),
					AF_INET6, hdr));
			zassert_equal(strncmp(ipaddr, "2001:db8::3",
					      sizeof(ipaddr) - 1), 0,
				      "Invalid IPv6 address %s added",
				      ipaddr);
			event_count--;
			break;
		case NET_EVENT_IPV6_ADDR_DEL:
			DBG("IPv6 address removed from interface %d (%s)\n",
			    event_addr.nm_ifindex,
			    get_ip_addr(ipaddr, sizeof(ipaddr),
					AF_INET6, hdr));
			zassert_equal(strncmp(ipaddr, "2001:db8::3",
					      sizeof(ipaddr) - 1), 0,
				      "Invalid IPv6 address %s removed",
				      ipaddr);
			event_count--;
			break;
		}
	}
}

ZTEST(net_socket_net_mgmt, test_net_mgmt_catch_kernel)
{
	test_net_mgmt_catch_events();
}

ZTEST_USER(net_socket_net_mgmt, test_net_mgmt_catch_user)
{
	test_net_mgmt_catch_events();
}

static void test_net_mgmt_catch_events_failure(void)
{
#define SMALL_BUF_LEN 16
	struct sockaddr_nm event_addr;
	socklen_t event_addr_len;
	uint8_t buf[SMALL_BUF_LEN];
	int ret;

	memset(buf, 0, sizeof(buf));
	event_addr_len = sizeof(event_addr);

	ret = recvfrom(fd, buf, sizeof(buf), 0,
		       (struct sockaddr *)&event_addr,
		       &event_addr_len);
	zassert_equal(ret, -1, "Msg check failed, %d", errno);
	zassert_equal(errno, EMSGSIZE, "Msg check failed, errno %d", errno);
}

ZTEST(net_socket_net_mgmt, test_net_mgmt_catch_failure_kernel)
{
	test_net_mgmt_catch_events_failure();
}

ZTEST_USER(net_socket_net_mgmt, test_net_mgmt_catch_failure_user)
{
	test_net_mgmt_catch_events_failure();
}

ZTEST(net_socket_net_mgmt, test_net_mgmt_cleanup)
{
	k_thread_abort(trigger_events_thread_id);
}

static void test_ethernet_set_qav(void)
{
	struct ethernet_req_params params;
	int ret;

	memset(&params, 0, sizeof(params));

	params.qav_param.queue_id = 1;
	params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_STATUS;
	params.qav_param.enabled = true;

	ret = setsockopt(fd, SOL_NET_MGMT_RAW,
			 NET_REQUEST_ETHERNET_SET_QAV_PARAM,
			 &params, sizeof(params));
	zassert_equal(ret, 0, "Cannot set Qav parameters");
}

ZTEST(net_socket_net_mgmt, test_ethernet_set_qav_kernel)
{
	test_ethernet_set_qav();
}

ZTEST_USER(net_socket_net_mgmt, test_ethernet_set_qav_user)
{
	test_ethernet_set_qav();
}

static void test_ethernet_get_qav(void)
{
	struct ethernet_req_params params;
	socklen_t optlen = sizeof(params);
	int ret;

	memset(&params, 0, sizeof(params));

	params.qav_param.queue_id = 1;
	params.qav_param.type = ETHERNET_QAV_PARAM_TYPE_STATUS;

	ret = getsockopt(fd, SOL_NET_MGMT_RAW,
			 NET_REQUEST_ETHERNET_GET_QAV_PARAM,
			 &params, &optlen);
	zassert_equal(ret, 0, "Cannot get Qav parameters (%d)", ret);
	zassert_equal(optlen, sizeof(params), "Invalid optlen (%d)", optlen);

	zassert_true(params.qav_param.enabled, "Qav not enabled");
}

ZTEST(net_socket_net_mgmt, test_ethernet_get_qav_kernel)
{
	test_ethernet_get_qav();
}

ZTEST_USER(net_socket_net_mgmt, test_ethernet_get_qav_user)
{
	test_ethernet_get_qav();
}

static void test_ethernet_get_unknown_option(void)
{
	struct ethernet_req_params params;
	socklen_t optlen = sizeof(params);
	int ret;

	memset(&params, 0, sizeof(params));

	ret = getsockopt(fd, SOL_NET_MGMT_RAW,
			 NET_REQUEST_ETHERNET_GET_PRIORITY_QUEUES_NUM,
			 &params, &optlen);
	zassert_equal(ret, -1, "Could get prio queue parameters (%d)", errno);
	zassert_equal(errno, EINVAL, "prio queue get parameters");
}

ZTEST(net_socket_net_mgmt, test_ethernet_get_unknown_opt_kernel)
{
	test_ethernet_get_unknown_option();
}

ZTEST_USER(net_socket_net_mgmt, test_ethernet_get_unknown_opt_user)
{
	test_ethernet_get_unknown_option();
}

static void test_ethernet_set_unknown_option(void)
{
	struct ethernet_req_params params;
	socklen_t optlen = sizeof(params);
	int ret;

	memset(&params, 0, sizeof(params));

	ret = setsockopt(fd, SOL_NET_MGMT_RAW,
			 NET_REQUEST_ETHERNET_SET_MAC_ADDRESS,
			 &params, optlen);
	zassert_equal(ret, -1, "Could set promisc_mode parameters (%d)", errno);
	zassert_equal(errno, EINVAL, "promisc_mode set parameters");
}

ZTEST(net_socket_net_mgmt, test_ethernet_set_unknown_opt_kernel)
{
	test_ethernet_set_unknown_option();
}

ZTEST_USER(net_socket_net_mgmt, test_ethernet_set_unknown_opt_user)
{
	test_ethernet_set_unknown_option();
}

static void *setup(void)
{
	k_thread_system_pool_assign(k_current_get());
	test_net_mgmt_setup();
	return NULL;
}

ZTEST_SUITE(net_socket_net_mgmt, NULL, setup, NULL, NULL, NULL);
