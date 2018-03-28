/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "net-stats"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_stats.h>

static struct k_delayed_work stats_timer;

#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
#define GET_STAT(iface, s) (iface ? iface->stats.s : data->s)
#else
#define GET_STAT(iface, s) data->s
#endif

static void print_stats(struct net_if *iface, struct net_stats *data)
{
	if (iface) {
		printk("Statistics for interface %p [%d]\n", iface,
		       net_if_get_by_iface(iface));
	} else {
		printk("Global network statistics\n");
	}

#if defined(CONFIG_NET_IPV6)
	printk("IPv6 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(iface, ipv6.recv),
	       GET_STAT(iface, ipv6.sent),
	       GET_STAT(iface, ipv6.drop),
	       GET_STAT(iface, ipv6.forwarded));
#if defined(CONFIG_NET_IPV6_ND)
	printk("IPv6 ND recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, ipv6_nd.recv),
	       GET_STAT(iface, ipv6_nd.sent),
	       GET_STAT(iface, ipv6_nd.drop));
#endif /* CONFIG_NET_IPV6_ND */
#if defined(CONFIG_NET_STATISTICS_MLD)
	printk("IPv6 MLD recv  %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, ipv6_mld.recv),
	       GET_STAT(iface, ipv6_mld.sent),
	       GET_STAT(iface, ipv6_mld.drop));
#endif /* CONFIG_NET_STATISTICS_MLD */
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	printk("IPv4 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(iface, ipv4.recv),
	       GET_STAT(iface, ipv4.sent),
	       GET_STAT(iface, ipv4.drop),
	       GET_STAT(iface, ipv4.forwarded));
#endif /* CONFIG_NET_IPV4 */

	printk("IP vhlerr      %d\thblener\t%d\tlblener\t%d\n",
	       GET_STAT(iface, ip_errors.vhlerr),
	       GET_STAT(iface, ip_errors.hblenerr),
	       GET_STAT(iface, ip_errors.lblenerr));
	printk("IP fragerr     %d\tchkerr\t%d\tprotoer\t%d\n",
	       GET_STAT(iface, ip_errors.fragerr),
	       GET_STAT(iface, ip_errors.chkerr),
	       GET_STAT(iface, ip_errors.protoerr));

	printk("ICMP recv      %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, icmp.recv),
	       GET_STAT(iface, icmp.sent),
	       GET_STAT(iface, icmp.drop));
	printk("ICMP typeer    %d\tchkerr\t%d\n",
	       GET_STAT(iface, icmp.typeerr),
	       GET_STAT(iface, icmp.chkerr));

#if defined(CONFIG_NET_UDP)
	printk("UDP recv       %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, udp.recv),
	       GET_STAT(iface, udp.sent),
	       GET_STAT(iface, udp.drop));
	printk("UDP chkerr     %d\n",
	       GET_STAT(iface, udp.chkerr));
#endif

#if defined(CONFIG_NET_STATISTICS_TCP)
	printk("TCP bytes recv %u\tsent\t%d\n",
	       GET_STAT(iface, tcp.bytes.received),
	       GET_STAT(iface, tcp.bytes.sent));
	printk("TCP seg recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, tcp.recv),
	       GET_STAT(iface, tcp.sent),
	       GET_STAT(iface, tcp.drop));
	printk("TCP seg resent %d\tchkerr\t%d\tackerr\t%d\n",
	       GET_STAT(iface, tcp.resent),
	       GET_STAT(iface, tcp.chkerr),
	       GET_STAT(iface, tcp.ackerr));
	printk("TCP seg rsterr %d\trst\t%d\tre-xmit\t%d\n",
	       GET_STAT(iface, tcp.rsterr),
	       GET_STAT(iface, tcp.rst),
	       GET_STAT(iface, tcp.rexmit));
	printk("TCP conn drop  %d\tconnrst\t%d\n",
	       GET_STAT(iface, tcp.conndrop),
	       GET_STAT(iface, tcp.connrst));
#endif

#if defined(CONFIG_NET_STATISTICS_RPL)
	printk("RPL DIS recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, rpl.dis.recv),
	       GET_STAT(iface, rpl.dis.sent),
	       GET_STAT(iface, rpl.dis.drop));
	printk("RPL DIO recv   %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, rpl.dio.recv),
	       GET_STAT(iface, rpl.dio.sent),
	       GET_STAT(iface, rpl.dio.drop));
	printk("RPL DAO recv   %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
	       GET_STAT(iface, rpl.dao.recv),
	       GET_STAT(iface, rpl.dao.sent),
	       GET_STAT(iface, rpl.dao.drop),
	      GET_STAT(iface, rpl.dao.forwarded));
	printk("RPL DAOACK rcv %d\tsent\t%d\tdrop\t%d\n",
	       GET_STAT(iface, rpl.dao_ack.recv),
	       GET_STAT(iface, rpl.dao_ack.sent),
	       GET_STAT(iface, rpl.dao_ack.drop));
	printk("RPL overflows  %d\tl-repairs\t%d\tg-repairs\t%d\n",
	       GET_STAT(iface, rpl.mem_overflows),
	       GET_STAT(iface, rpl.local_repairs),
	       GET_STAT(iface, rpl.global_repairs));
	printk("RPL malformed  %d\tresets   \t%d\tp-switch\t%d\n",
	       GET_STAT(iface, rpl.malformed_msgs),
	       GET_STAT(iface, rpl.resets),
	       GET_STAT(iface, rpl.parent_switch));
	printk("RPL f-errors   %d\tl-errors\t%d\tl-warnings\t%d\n",
	       GET_STAT(iface, rpl.forward_errors),
	       GET_STAT(iface, rpl.loop_errors),
	       GET_STAT(iface, rpl.loop_warnings));
	printk("RPL r-repairs  %d\n",
	       GET_STAT(iface, rpl.root_repairs));
#endif

	printk("Bytes received %u\n", GET_STAT(iface, bytes.received));
	printk("Bytes sent     %u\n", GET_STAT(iface, bytes.sent));
	printk("Processing err %d\n", GET_STAT(iface, processing_error));
}

#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
static void iface_cb(struct net_if *iface, void *user_data)
{
	struct net_stats *data = user_data;

	net_mgmt(NET_REQUEST_STATS_GET_ALL, iface, data, sizeof(*data));

	print_stats(iface, data);
}
#endif

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static void print_eth_stats(struct net_if *iface, struct net_stats_eth *data)
{
	printk("Statistics for Ethernet interface %p [%d]\n", iface,
	       net_if_get_by_iface(iface));

	printk("Bytes received   : %u\n", data->bytes.received);
	printk("Bytes sent       : %u\n", data->bytes.sent);
	printk("Packets received : %u\n", data->pkts.rx);
	printk("Packets sent     : %u\n", data->pkts.tx);
	printk("Bcast received   : %u\n", data->broadcast.rx);
	printk("Bcast sent       : %u\n", data->broadcast.tx);
	printk("Mcast received   : %u\n", data->multicast.rx);
	printk("Mcast sent       : %u\n", data->multicast.tx);
}

static void eth_iface_cb(struct net_if *iface, void *user_data)
{
	struct net_stats_eth eth_data;
	int ret;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	ret = net_mgmt(NET_REQUEST_STATS_GET_ETHERNET, iface, &eth_data,
		       sizeof(eth_data));
	if (ret < 0) {
		return;
	}

	print_eth_stats(iface, &eth_data);
}
#endif

static void stats(struct k_work *work)
{
	struct net_stats data;

	/* It is also possible to query some specific statistics by setting
	 * the first request parameter properly. See include/net/net_stats.h
	 * what requests are available.
	 */
	net_mgmt(NET_REQUEST_STATS_GET_ALL, NULL, &data, sizeof(data));

	print_stats(NULL, &data);

#if defined(CONFIG_NET_STATISTICS_PER_INTERFACE)
	net_if_foreach(iface_cb, &data);
#endif

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	net_if_foreach(eth_iface_cb, &data);
#endif

	k_delayed_work_submit(&stats_timer, K_SECONDS(CONFIG_SAMPLE_PERIOD));
}

static void init_app(void)
{
	k_delayed_work_init(&stats_timer, stats);
	k_delayed_work_submit(&stats_timer, K_SECONDS(CONFIG_SAMPLE_PERIOD));
}

void main(void)
{
	/* Register a timer that will collect statistics after every n seconds.
	 */
	init_app();
}
