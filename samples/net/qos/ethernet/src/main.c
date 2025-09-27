/*
 * Copyright (c) 2025 The Zephyr Contributors.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt_filter.h>

#include <posix_board_if.h>

LOG_MODULE_REGISTER(qos_ethernet);

#define NET_ETH_PTYPE_123 123
#define NET_ETH_PTYPE_124 124
#define NET_ETH_PTYPE_125 125
#define NET_ETH_PTYPE_126 126
#define NET_ETH_PTYPE_127 127
#define NET_ETH_PTYPE_128 128
#define NET_ETH_PTYPE_129 129
#define NET_ETH_PTYPE_130 130

#define SINGLE_RUN_DEADLINE K_MSEC(2000)

#define MTU 1500

enum service_type {
	SERVICE_TYPE_COMMAND,
	SERVICE_TYPE_ECHO,
};

struct net_if_fake_data {
	uint8_t mac[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

struct statistics {
	unsigned int service_123_count;
	unsigned int service_124_count;
	unsigned int service_125_count;
	unsigned int service_126_count;
	unsigned int service_127_count;
	unsigned int service_128_count;
	unsigned int service_129_count;
	unsigned int service_130_count;
	unsigned int echo_123_count;
	unsigned int echo_124_count;
	unsigned int echo_125_count;
	unsigned int echo_126_count;
	unsigned int echo_127_count;
	unsigned int echo_128_count;
	unsigned int echo_129_count;
	unsigned int echo_130_count;
};

static K_SEM_DEFINE(service_123_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_124_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_125_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_126_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_127_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_128_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_129_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_130_received, 0, UINT_MAX);

static K_SEM_DEFINE(echo_123_send, 0, UINT_MAX);
static K_SEM_DEFINE(echo_124_send, 0, UINT_MAX);
static K_SEM_DEFINE(echo_125_send, 0, UINT_MAX);
static K_SEM_DEFINE(echo_126_send, 0, UINT_MAX);
static K_SEM_DEFINE(echo_127_send, 0, UINT_MAX);
static K_SEM_DEFINE(echo_128_send, 0, UINT_MAX);
static K_SEM_DEFINE(echo_129_send, 0, UINT_MAX);
static K_SEM_DEFINE(echo_130_send, 0, UINT_MAX);

static NPF_ETH_TYPE_MATCH(npf_123, NET_ETH_PTYPE_123);
static NPF_ETH_TYPE_MATCH(npf_124, NET_ETH_PTYPE_124);
static NPF_ETH_TYPE_MATCH(npf_125, NET_ETH_PTYPE_125);
static NPF_ETH_TYPE_MATCH(npf_126, NET_ETH_PTYPE_126);
static NPF_ETH_TYPE_MATCH(npf_127, NET_ETH_PTYPE_127);
static NPF_ETH_TYPE_MATCH(npf_128, NET_ETH_PTYPE_128);
static NPF_ETH_TYPE_MATCH(npf_129, NET_ETH_PTYPE_129);
static NPF_ETH_TYPE_MATCH(npf_130, NET_ETH_PTYPE_130);

static NPF_PRIORITY(priority_rule_123, NET_PRIORITY_BK, npf_123);
static NPF_PRIORITY(priority_rule_124, NET_PRIORITY_BE, npf_124);
static NPF_PRIORITY(priority_rule_125, NET_PRIORITY_EE, npf_125);
static NPF_PRIORITY(priority_rule_126, NET_PRIORITY_CA, npf_126);
static NPF_PRIORITY(priority_rule_127, NET_PRIORITY_VI, npf_127);
static NPF_PRIORITY(priority_rule_128, NET_PRIORITY_VO, npf_128);
static NPF_PRIORITY(priority_rule_129, NET_PRIORITY_IC, npf_129);
static NPF_PRIORITY(priority_rule_130, NET_PRIORITY_NC, npf_130);

static uint32_t simulated_work_time; /* data race ! */

int net_fake_dev_init(const struct device *dev)
{
	return 0;
}

static void copy_mac_to(char destination[static 6])
{
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	/* taken from arp test */
	destination[0] = 0x00;
	destination[1] = 0x00;
	destination[2] = 0x5E;
	destination[3] = 0x00;
	destination[4] = 0x53;
	destination[5] = 0x3B;
}

static void net_iface_init(struct net_if *iface)
{
	const struct device *device = net_if_get_device(iface);
	struct net_if_fake_data *context = device->data;

	if (context->mac[2] == 0x00) {
		copy_mac_to(context->mac);
	}

	net_if_set_link_addr(iface, context->mac, sizeof(context->mac), NET_LINK_ETHERNET);
}

static int net_if_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	LOG_INF("sending service %u, pkt %p", net_pkt_ll_proto_type(pkt), pkt);

	posix_cpu_hold(simulated_work_time);

	switch (net_pkt_ll_proto_type(pkt)) {
	case NET_ETH_PTYPE_123:
		k_sem_give(&echo_123_send);
		break;
	case NET_ETH_PTYPE_124:
		k_sem_give(&echo_124_send);
		break;
	case NET_ETH_PTYPE_125:
		k_sem_give(&echo_125_send);
		break;
	case NET_ETH_PTYPE_126:
		k_sem_give(&echo_126_send);
		break;
	case NET_ETH_PTYPE_127:
		k_sem_give(&echo_127_send);
		break;
	case NET_ETH_PTYPE_128:
		k_sem_give(&echo_128_send);
		break;
	case NET_ETH_PTYPE_129:
		k_sem_give(&echo_129_send);
		break;
	case NET_ETH_PTYPE_130:
		k_sem_give(&echo_130_send);
		break;
	default: /* nothing to do */
		break;
	}
	net_pkt_unref(pkt);
	return 0;
}

static const struct ethernet_api net_if_api = {
	.iface_api.init = net_iface_init,
	.send = net_if_fake_send,
};

static struct net_if_fake_data context;

#define _ETH_L2_LAYER    ETHERNET_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)

NET_DEVICE_INIT(net_if_fake, "net_if_fake", net_fake_dev_init, NULL, &context, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &net_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		MTU);

static void try_recv_data(struct net_if *iface, uint16_t ptype, enum service_type service_type)
{
	int res;
	struct net_pkt *pkt = NULL;

	char src[6] = {6, 7, 8, 9, 10, 11};
	char dest[6];

	copy_mac_to(dest);

	pkt = net_pkt_rx_alloc_with_buffer(iface, MTU, AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		return;
	}

	if (net_pkt_write(pkt, dest, ARRAY_SIZE(dest))) {
		goto error;
	}
	if (net_pkt_write(pkt, src, ARRAY_SIZE(src))) {
		goto error;
	}
	if (net_pkt_write_be16(pkt, ptype)) {
		goto error;
	}
	if (net_pkt_write_u8(pkt, service_type)) {
		goto error;
	}

	res = net_recv_data(net_pkt_iface(pkt), pkt);
	if (res < 0) {
		LOG_ERR("Failed to enqueue frame into RX queue: %d", res);
		goto error;
	}

	return;

error:
	net_pkt_unref(pkt);
}

struct statistics single_run_with_simulated_work(struct net_if *iface, uint32_t work)
{
	k_timepoint_t deadline = sys_timepoint_calc(SINGLE_RUN_DEADLINE);

	simulated_work_time = work;

	k_sem_reset(&service_123_received);
	k_sem_reset(&service_124_received);
	k_sem_reset(&service_125_received);
	k_sem_reset(&service_126_received);
	k_sem_reset(&service_127_received);
	k_sem_reset(&service_128_received);
	k_sem_reset(&service_129_received);
	k_sem_reset(&service_130_received);
	k_sem_reset(&echo_123_send);
	k_sem_reset(&echo_124_send);
	k_sem_reset(&echo_125_send);
	k_sem_reset(&echo_126_send);
	k_sem_reset(&echo_127_send);
	k_sem_reset(&echo_128_send);
	k_sem_reset(&echo_129_send);
	k_sem_reset(&echo_130_send);

	while (!sys_timepoint_expired(deadline)) {
		/*every tick try receive a packet of each type*/
		try_recv_data(iface, NET_ETH_PTYPE_123, SERVICE_TYPE_ECHO);
		try_recv_data(iface, NET_ETH_PTYPE_123, SERVICE_TYPE_COMMAND);
		try_recv_data(iface, NET_ETH_PTYPE_124, SERVICE_TYPE_ECHO);
		try_recv_data(iface, NET_ETH_PTYPE_124, SERVICE_TYPE_COMMAND);
		try_recv_data(iface, NET_ETH_PTYPE_125, SERVICE_TYPE_ECHO);
		try_recv_data(iface, NET_ETH_PTYPE_125, SERVICE_TYPE_COMMAND);
		try_recv_data(iface, NET_ETH_PTYPE_126, SERVICE_TYPE_ECHO);
		try_recv_data(iface, NET_ETH_PTYPE_126, SERVICE_TYPE_COMMAND);
		try_recv_data(iface, NET_ETH_PTYPE_127, SERVICE_TYPE_ECHO);
		try_recv_data(iface, NET_ETH_PTYPE_127, SERVICE_TYPE_COMMAND);
		try_recv_data(iface, NET_ETH_PTYPE_128, SERVICE_TYPE_ECHO);
		try_recv_data(iface, NET_ETH_PTYPE_128, SERVICE_TYPE_COMMAND);
		try_recv_data(iface, NET_ETH_PTYPE_129, SERVICE_TYPE_ECHO);
		try_recv_data(iface, NET_ETH_PTYPE_129, SERVICE_TYPE_COMMAND);
		try_recv_data(iface, NET_ETH_PTYPE_130, SERVICE_TYPE_ECHO);
		try_recv_data(iface, NET_ETH_PTYPE_130, SERVICE_TYPE_COMMAND);
		k_sleep(K_TICKS(1));
	}

	return (struct statistics){
		.service_123_count = k_sem_count_get(&service_123_received),
		.service_124_count = k_sem_count_get(&service_124_received),
		.service_125_count = k_sem_count_get(&service_125_received),
		.service_126_count = k_sem_count_get(&service_126_received),
		.service_127_count = k_sem_count_get(&service_127_received),
		.service_128_count = k_sem_count_get(&service_128_received),
		.service_129_count = k_sem_count_get(&service_129_received),
		.service_130_count = k_sem_count_get(&service_130_received),
		.echo_123_count = k_sem_count_get(&echo_123_send),
		.echo_124_count = k_sem_count_get(&echo_124_send),
		.echo_125_count = k_sem_count_get(&echo_125_send),
		.echo_126_count = k_sem_count_get(&echo_126_send),
		.echo_127_count = k_sem_count_get(&echo_127_send),
		.echo_128_count = k_sem_count_get(&echo_128_send),
		.echo_129_count = k_sem_count_get(&echo_129_send),
		.echo_130_count = k_sem_count_get(&echo_130_send),
	};
}

void print_result(const char *msg, size_t cnt, uint32_t simulated_work_times[static cnt],
		  struct statistics stats[static cnt])
{
	LOG_INF("--- Statistics (%s) ---", msg);
	LOG_INF("c (x) := command service for priority x (high means higher priority)");
	LOG_INF("e (x) := echo service for priority x (high means higher priority)");
	LOG_INF("+---------+------+------+------+------+------+------+------+------+------+------+-"
		"-----+------+------+------+------+------+");
	LOG_INF("| work us | c(7) | e(7) | c(6) | e(6) | c(5) | e(5) | c(4) | e(4) | c(3) | e(3) | "
		"c(2) | e(2) | c(1) | e(1) | c(0) | e(0) |");
	LOG_INF("+=========+======+======+======+======+======+======+======+======+======+======+="
		"=====+======+======+======+======+======+");
	for (size_t i = 0; i < cnt; ++i) {
		LOG_INF("| %7u | %4d | %4d | %4d | %4d | %4d | %4d | %4d | %4d | %4d | %4d | %4d | "
			"%4d | %4d | %4d | %4d | %4d |",
			simulated_work_times[i], stats[i].service_130_count,
			stats[i].echo_130_count, stats[i].service_129_count,
			stats[i].echo_129_count, stats[i].service_128_count,
			stats[i].echo_128_count, stats[i].service_127_count,
			stats[i].echo_127_count, stats[i].service_126_count,
			stats[i].echo_126_count, stats[i].service_125_count,
			stats[i].echo_125_count, stats[i].service_124_count,
			stats[i].echo_124_count, stats[i].service_123_count,
			stats[i].echo_123_count);
		LOG_INF("+---------+------+------+------+------+------+------+------+------+------+"
			"------+------+------+------+------+------+------+");
	}
}

int main(int argc, char **argv)
{
	struct net_if *iface = NULL;
	uint32_t simulated_work_times[] = {
		800,  850,  900,  950,  1000, 1100, 1200, 1300, 1400,  1600,  1800,
		2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 15000,
	};
	struct statistics stats_no_filter[ARRAY_SIZE(simulated_work_times)];
	struct statistics stats_with_filter[ARRAY_SIZE(simulated_work_times)];

	iface = net_if_lookup_by_dev(DEVICE_GET(net_if_fake));
	if (iface == NULL) {
		LOG_ERR("No device");
		return 1;
	}

	for (size_t i = 0; i < ARRAY_SIZE(simulated_work_times); ++i) {
		stats_no_filter[i] = single_run_with_simulated_work(iface, simulated_work_times[i]);
		k_msleep(200);
		print_result("In Progress", i + 1, simulated_work_times, stats_no_filter);
		/* let simulation settle down */
		k_msleep(800);
	}

	npf_append_recv_rule(&priority_rule_123);
	npf_append_recv_rule(&priority_rule_124);
	npf_append_recv_rule(&priority_rule_125);
	npf_append_recv_rule(&priority_rule_126);
	npf_append_recv_rule(&priority_rule_127);
	npf_append_recv_rule(&priority_rule_128);
	npf_append_recv_rule(&priority_rule_129);
	npf_append_recv_rule(&priority_rule_130);
	npf_append_recv_rule(&npf_default_ok);

	for (size_t i = 0; i < ARRAY_SIZE(simulated_work_times); ++i) {
		stats_with_filter[i] =
			single_run_with_simulated_work(iface, simulated_work_times[i]);
		k_msleep(1000);
		print_result("In Progress", i + 1, simulated_work_times, stats_with_filter);
		/* let simulation settle down */
		k_msleep(2000);
	}

	k_msleep(4000);
	print_result("No Quality of Service Filtering", ARRAY_SIZE(simulated_work_times),
		     simulated_work_times, stats_no_filter);
	print_result("With Quality of Service Filtering", ARRAY_SIZE(simulated_work_times),
		     simulated_work_times, stats_with_filter);

	return 0;
}

static enum net_verdict l2_service(struct net_if *iface, uint16_t ptype, struct net_pkt *pkt)
{
	struct net_pkt *echo_reply;
	uint8_t type;
	const char *service_type;

	net_pkt_cursor_init(pkt);
	net_pkt_read_u8(pkt, &type);
	service_type = (type == SERVICE_TYPE_ECHO) ? "echo" : "command";

	LOG_INF("handler for %s-service %d, iface %p, ptype %u, pkt %p", service_type,
		net_pkt_ll_proto_type(pkt), iface, ptype, pkt);

	posix_cpu_hold(simulated_work_time);

	if (type == SERVICE_TYPE_ECHO) {
		echo_reply = net_pkt_alloc_with_buffer(iface, 12, AF_UNSPEC, 0, K_NO_WAIT);
		if (echo_reply) {
			net_pkt_set_ll_proto_type(echo_reply, net_pkt_ll_proto_type(pkt));
			net_pkt_set_priority(echo_reply, net_pkt_priority(pkt));
			if (net_if_try_send_data(iface, echo_reply, K_NO_WAIT) != NET_OK) {
				net_pkt_unref(echo_reply);
			}
		}
		net_pkt_unref(pkt);
		return NET_OK;
	}

	switch (net_pkt_ll_proto_type(pkt)) {
	case NET_ETH_PTYPE_123:
		k_sem_give(&service_123_received);
		break;
	case NET_ETH_PTYPE_124:
		k_sem_give(&service_124_received);
		break;
	case NET_ETH_PTYPE_125:
		k_sem_give(&service_125_received);
		break;
	case NET_ETH_PTYPE_126:
		k_sem_give(&service_126_received);
		break;
	case NET_ETH_PTYPE_127:
		k_sem_give(&service_127_received);
		break;
	case NET_ETH_PTYPE_128:
		k_sem_give(&service_128_received);
		break;
	case NET_ETH_PTYPE_129:
		k_sem_give(&service_129_received);
		break;
	case NET_ETH_PTYPE_130:
		k_sem_give(&service_130_received);
		break;
	default: /* nothing to do */
		break;
	}
	net_pkt_unref(pkt);
	return NET_OK;
}

ETH_NET_L3_REGISTER(service_123, NET_ETH_PTYPE_123, l2_service);
ETH_NET_L3_REGISTER(service_124, NET_ETH_PTYPE_124, l2_service);
ETH_NET_L3_REGISTER(service_125, NET_ETH_PTYPE_125, l2_service);
ETH_NET_L3_REGISTER(service_126, NET_ETH_PTYPE_126, l2_service);
ETH_NET_L3_REGISTER(service_127, NET_ETH_PTYPE_127, l2_service);
ETH_NET_L3_REGISTER(service_128, NET_ETH_PTYPE_128, l2_service);
ETH_NET_L3_REGISTER(service_129, NET_ETH_PTYPE_129, l2_service);
ETH_NET_L3_REGISTER(service_130, NET_ETH_PTYPE_130, l2_service);
