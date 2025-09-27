#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_qos);

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dummy.h>
#include <zephyr/random/random.h>

#include <posix_board_if.h>
#include "arp.h"

#include "net_private.h"

#define MTU 1500

#define MY_MAC_0 0x00
#define MY_MAC_1 0x00
#define MY_MAC_2 0x5E
#define MY_MAC_3 0x00
#define MY_MAC_4 0x53
#define MY_MAC_5 0x3B //sys_rand8_get() ?

static K_SEM_DEFINE(service_123_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_124_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_125_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_126_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_127_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_128_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_129_received, 0, UINT_MAX);
static K_SEM_DEFINE(service_130_received, 0, UINT_MAX);

static const char service_123_data[MTU] = {
	MY_MAC_0,
	MY_MAC_1,
	MY_MAC_2,
	MY_MAC_3,
	MY_MAC_4,
	MY_MAC_5,
	6,
	7,
	8,
	9,
	10,
	11,
	0,
	NET_ETH_PTYPE_123,
};

static const char service_124_data[MTU] = {
	MY_MAC_0,
	MY_MAC_1,
	MY_MAC_2,
	MY_MAC_3,
	MY_MAC_4,
	MY_MAC_5,
	6,
	7,
	8,
	9,
	10,
	11,
	0,
	NET_ETH_PTYPE_124,
};

static const char service_125_data[MTU] = {
	MY_MAC_0,
	MY_MAC_1,
	MY_MAC_2,
	MY_MAC_3,
	MY_MAC_4,
	MY_MAC_5,
	6,
	7,
	8,
	9,
	10,
	11,
	0,
	NET_ETH_PTYPE_125,
};

static const char service_126_data[MTU] = {
	MY_MAC_0,
	MY_MAC_1,
	MY_MAC_2,
	MY_MAC_3,
	MY_MAC_4,
	MY_MAC_5,
	6,
	7,
	8,
	9,
	10,
	11,
	0,
	NET_ETH_PTYPE_126,
};

static const char service_127_data[MTU] = {
	MY_MAC_0,
	MY_MAC_1,
	MY_MAC_2,
	MY_MAC_3,
	MY_MAC_4,
	MY_MAC_5,
	6,
	7,
	8,
	9,
	10,
	11,
	0,
	NET_ETH_PTYPE_127,
};

static const char service_128_data[MTU] = {
	MY_MAC_0,
	MY_MAC_1,
	MY_MAC_2,
	MY_MAC_3,
	MY_MAC_4,
	MY_MAC_5,
	6,
	7,
	8,
	9,
	10,
	11,
	0,
	NET_ETH_PTYPE_128,
};

static const char service_129_data[MTU] = {
	MY_MAC_0,
	MY_MAC_1,
	MY_MAC_2,
	MY_MAC_3,
	MY_MAC_4,
	MY_MAC_5,
	6,
	7,
	8,
	9,
	10,
	11,
	0,
	NET_ETH_PTYPE_129,
};

static const char service_130_data[MTU] = {
	MY_MAC_0,
	MY_MAC_1,
	MY_MAC_2,
	MY_MAC_3,
	MY_MAC_4,
	MY_MAC_5,
	6,
	7,
	8,
	9,
	10,
	11,
	0,
	NET_ETH_PTYPE_130,
};

uint32_t simulated_work_time; /* data race ! */

struct net_if_fake_data {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_fake_dev_init(const struct device *dev)
{
	return 0;
}

static void copy_mac_to(char destination[static 6])
{
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	destination[0] = MY_MAC_0;
	destination[1] = MY_MAC_1;
	destination[2] = MY_MAC_2;
	destination[3] = MY_MAC_3;
	destination[4] = MY_MAC_4;
	destination[5] = MY_MAC_5;//sys_rand8_get();
}

static uint8_t *net_get_mac(const struct device *dev)
{
	struct net_if_fake_data *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		copy_mac_to(context->mac_addr);
	}

	return context->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	return 0;
}


struct net_if_fake_data net_if_fake_data;

static const struct ethernet_api net_if_api = {
	.iface_api.init = net_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER ETHERNET_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)

NET_DEVICE_INIT(net_if_fake, "net_if_fake",
		net_fake_dev_init, NULL,
		&net_if_fake_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, MTU);

static void try_recv_data(struct net_if *iface, size_t size, const char data[static size])
{
	int res;
	struct net_pkt *pkt = NULL;

	pkt = net_pkt_rx_alloc_with_buffer(iface, size, AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt) {
		if (net_pkt_write(pkt, data, size)) {
			LOG_ERR("Failed to append RX buffer to context buffer");
			net_pkt_unref(pkt);
		}

		res = net_recv_data(net_pkt_iface(pkt), pkt);
		if (res < 0) {
			LOG_ERR("Failed to enqueue frame into RX queue: %d", res);
			net_pkt_unref(pkt);
		}
	}

}

struct statistics {
	unsigned int service_123_count;
	unsigned int service_124_count;
	unsigned int service_125_count;
	unsigned int service_126_count;
	unsigned int service_127_count;
	unsigned int service_128_count;
	unsigned int service_129_count;
	unsigned int service_130_count;
};

struct statistics single_run_with_simulated_work(struct net_if *iface, uint32_t work)
{

	simulated_work_time = work;

	k_timepoint_t deadline = sys_timepoint_calc(K_MSEC(1000));

	k_sem_reset(&service_123_received);
	k_sem_reset(&service_124_received);
	k_sem_reset(&service_125_received);
	k_sem_reset(&service_126_received);
	k_sem_reset(&service_127_received);
	k_sem_reset(&service_128_received);
	k_sem_reset(&service_129_received);
	k_sem_reset(&service_130_received);

	while(!sys_timepoint_expired(deadline))
	{
		k_sleep(K_TICKS(1)); /*every tick try receive a packet of each type*/
		try_recv_data(iface, ARRAY_SIZE(service_123_data), service_123_data);
		try_recv_data(iface, ARRAY_SIZE(service_124_data), service_124_data);
		try_recv_data(iface, ARRAY_SIZE(service_125_data), service_125_data);
		try_recv_data(iface, ARRAY_SIZE(service_126_data), service_126_data);
		try_recv_data(iface, ARRAY_SIZE(service_127_data), service_127_data);
		try_recv_data(iface, ARRAY_SIZE(service_128_data), service_128_data);
		try_recv_data(iface, ARRAY_SIZE(service_129_data), service_129_data);
		try_recv_data(iface, ARRAY_SIZE(service_130_data), service_130_data);
	}

	/* await simulation to settle down */
	k_msleep(3000);

	return (struct statistics) {
		.service_123_count = k_sem_count_get(&service_123_received),
		.service_124_count = k_sem_count_get(&service_124_received),
		.service_125_count = k_sem_count_get(&service_125_received),
		.service_126_count = k_sem_count_get(&service_126_received),
		.service_127_count = k_sem_count_get(&service_127_received),
		.service_128_count = k_sem_count_get(&service_128_received),
		.service_129_count = k_sem_count_get(&service_129_received),
		.service_130_count = k_sem_count_get(&service_130_received),
	};
}

struct sim {
	struct statistics result;
	uint32_t simulated_work_time;
};

void print_result(size_t cnt, struct sim sims[static cnt])
{
	LOG_INF("--- Statistics ---");
	LOG_INF("work us | service_123 | service_124 | service_125 | service_126 | service_127 | service_128 | service_129 | service_130");
	for (size_t i = 0; i < cnt; ++i) {
	LOG_INF("%7u | %11d | %11d | %11d | %11d | %11d | %11d | %11d | %11d ",
			sims[i].simulated_work_time,
			sims[i].result.service_123_count,
			sims[i].result.service_124_count,
			sims[i].result.service_125_count,
			sims[i].result.service_126_count,
			sims[i].result.service_127_count,
			sims[i].result.service_128_count,
			sims[i].result.service_129_count,
			sims[i].result.service_130_count
		);
	}
}

int main(int argc, char ** argv)
{
	struct net_if *iface = NULL;
	struct sim sims[] = {
		{ .simulated_work_time =  2 * 1000},
		{ .simulated_work_time =  3 * 1000},
		{ .simulated_work_time =  4 * 1000},
		{ .simulated_work_time =  6 * 1000},
		{ .simulated_work_time = 10 * 1000},
		{ .simulated_work_time = 25 * 1000},
	};

	iface = net_if_lookup_by_dev(DEVICE_GET(net_if_fake));
	if (iface == NULL) {
		LOG_ERR("No device");
		return 1;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sims); ++i) {
		sims[i].result = single_run_with_simulated_work(iface, sims[i].simulated_work_time);
		print_result(i+1, sims);
	}

	return 0;
}

static enum net_verdict l2_service(struct net_if *iface, uint16_t ptype,
				      struct net_pkt *pkt)
{
	LOG_INF("handler for service %d, iface %p, ptype %u, pkt %p",
			net_pkt_ll_proto_type(pkt), iface, ptype, pkt);
	posix_cpu_hold(simulated_work_time);
	switch(net_pkt_ll_proto_type(pkt)) {
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
