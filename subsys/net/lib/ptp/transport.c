/*
 * Copyright (c) 2024 BayLibre SAS
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_transport, CONFIG_PTP_LOG_LEVEL);

#include <inttypes.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/ptp_clock.h>

#include <errno.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/socket.h>

#include "transport.h"

#define INTERFACE_NAME_LEN (32)
#define PTP_L2_ADDR_LEN    (NET_ETH_ADDR_LEN)
#define PTP_L2_RECVMSG_RETRY_MS MSEC_PER_SEC

static const struct net_in_addr mcast_addr_ipv4 = {{{224, 0, 1, 129}}};
static const struct net_in6_addr mcast_addr_ipv6 = {
	{{0xff, 0x0e, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x81}}};
static const uint8_t mcast_addr_l2[PTP_L2_ADDR_LEN] = {0x01, 0x1B, 0x19, 0x00, 0x00, 0x00};

static int transport_socket_open(struct net_if *iface, struct net_sockaddr *addr)
{
	static const int feature_on = 1;
	static const uint8_t priority = NET_PRIORITY_CA;
	static const uint8_t ts_mask =
		ZSOCK_SOF_TIMESTAMPING_TX_HARDWARE | ZSOCK_SOF_TIMESTAMPING_RX_HARDWARE;
	struct net_ifreq ifreq = {0};
	int cnt;
	int socket = zsock_socket(addr->sa_family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	if (net_if_get_by_iface(iface) < 0) {
		LOG_ERR("Failed to obtain interface index");
		return -1;
	}

	if (socket < 0) {
		return -1;
	}

	if (zsock_setsockopt(socket, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &feature_on,
			     sizeof(feature_on))) {
		LOG_ERR("Failed to set SO_REUSEADDR");
		goto error;
	}

	if (zsock_bind(socket, addr, sizeof(*addr))) {
		LOG_ERR("Failed to bind socket");
		goto error;
	}

	cnt = net_if_get_name(iface, ifreq.ifr_name, INTERFACE_NAME_LEN);
	if (cnt > 0 && zsock_setsockopt(socket, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE,
					ifreq.ifr_name, sizeof(ifreq.ifr_name))) {
		LOG_ERR("Failed to set socket binding to an interface");
		goto error;
	}

	if (zsock_setsockopt(socket, ZSOCK_SOL_SOCKET, ZSOCK_SO_TIMESTAMPING, &ts_mask,
			     sizeof(ts_mask))) {
		LOG_ERR("Failed to set SO_TIMESTAMPING");
		goto error;
	}

	if (zsock_setsockopt(socket, ZSOCK_SOL_SOCKET, ZSOCK_SO_PRIORITY, &priority,
			     sizeof(priority))) {
		LOG_ERR("Failed to set SO_PRIORITY");
		goto error;
	}

	return socket;
error:
	zsock_close(socket);
	return -1;
}

static int transport_join_multicast(struct ptp_port *port)
{
	if (IS_ENABLED(CONFIG_PTP_UDP_IPv4_PROTOCOL)) {
		struct net_ip_mreqn mreqn = {0};

		memcpy(&mreqn.imr_multiaddr, &mcast_addr_ipv4, sizeof(struct net_in_addr));
		mreqn.imr_ifindex = net_if_get_by_iface(port->iface);

		if (zsock_setsockopt(port->socket[1], NET_IPPROTO_IP, ZSOCK_IP_ADD_MEMBERSHIP,
				     &mreqn, sizeof(mreqn))) {
			LOG_ERR("Failed to join IPv4 multicast group");
			return -1;
		}
	} else {
		struct net_ipv6_mreq mreqn = {0};

		memcpy(&mreqn.ipv6mr_multiaddr, &mcast_addr_ipv6, sizeof(struct net_in6_addr));
		mreqn.ipv6mr_ifindex = net_if_get_by_iface(port->iface);

		if (zsock_setsockopt(port->socket[0], NET_IPPROTO_IPV6, ZSOCK_IPV6_ADD_MEMBERSHIP,
				     &mreqn, sizeof(mreqn))) {
			LOG_ERR("Failed to join IPv6 multicast group");
			return -1;
		}
	}

	return 0;
}

static int transport_udp_ipv4_open(struct net_if *iface, uint16_t port)
{
	uint8_t tos;
	net_socklen_t length = sizeof(tos);
	int socket, ttl = 1;
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_addr = NET_INADDR_ANY_INIT,
		.sin_port = net_htons(port),
	};

	socket = transport_socket_open(iface, (struct net_sockaddr *)&addr);
	if (socket < 0) {
		return -1;
	}

	if (zsock_setsockopt(socket, NET_IPPROTO_IP, ZSOCK_IP_MULTICAST_TTL, &ttl, sizeof(ttl))) {
		LOG_ERR("Failed to set ip multicast ttl socket option");
		goto error;
	}

	if (zsock_getsockopt(socket, NET_IPPROTO_IP, ZSOCK_IP_TOS, &tos, &length)) {
		tos = 0;
	}

	tos &= ~0xFC;
	tos |= CONFIG_PTP_DSCP_VALUE << 2;
	length = sizeof(tos);

	if (zsock_setsockopt(socket, NET_IPPROTO_IP, ZSOCK_IP_TOS, &tos, length)) {
		LOG_WRN("Failed to set DSCP priority");
	}

	return socket;
error:
	zsock_close(socket);
	return -1;
}

static int transport_udp_ipv6_open(struct net_if *iface, uint16_t port)
{
	uint8_t tclass;
	net_socklen_t length;
	int socket, hops = 1, feature_on = 1;
	struct net_sockaddr_in6 addr = {.sin6_family = NET_AF_INET6,
					.sin6_addr = NET_IN6ADDR_ANY_INIT,
					.sin6_port = net_htons(port)};

	socket = transport_socket_open(iface, (struct net_sockaddr *)&addr);
	if (socket < 0) {
		return -1;
	}

	if (zsock_setsockopt(socket, NET_IPPROTO_IPV6, ZSOCK_IPV6_RECVPKTINFO, &feature_on,
			     sizeof(feature_on))) {
		LOG_ERR("Failed to set IPV6_RECVPKTINFO");
		goto error;
	}

	if (zsock_setsockopt(socket, NET_IPPROTO_IPV6, ZSOCK_IPV6_MULTICAST_HOPS, &hops,
			     sizeof(hops))) {
		LOG_ERR("Failed to set ip multicast hops socket option");
		goto error;
	}

	if (zsock_getsockopt(socket, NET_IPPROTO_IPV6, ZSOCK_IPV6_TCLASS, &tclass, &length)) {
		tclass = 0;
	}

	tclass &= ~0xFC;
	tclass |= CONFIG_PTP_DSCP_VALUE << 2;
	length = sizeof(tclass);

	if (zsock_setsockopt(socket, NET_IPPROTO_IPV6, ZSOCK_IPV6_TCLASS, &tclass, length)) {
		LOG_WRN("Failed to set priority");
	}

	return socket;
error:
	zsock_close(socket);
	return -1;
}

static int transport_l2_open(struct net_if *iface)
{
	struct net_sockaddr_ll addr = {0};
	static const uint8_t ts_mask =
		ZSOCK_SOF_TIMESTAMPING_TX_HARDWARE | ZSOCK_SOF_TIMESTAMPING_RX_HARDWARE;
	int ifindex = net_if_get_by_iface(iface);
	int socket;

	if (ifindex < 0) {
		LOG_ERR("Failed to obtain interface index");
		return -1;
	}

	socket = zsock_socket(NET_AF_PACKET, NET_SOCK_DGRAM, net_htons(NET_ETH_PTYPE_PTP));
	if (socket < 0) {
		return -1;
	}

	addr.sll_family = NET_AF_PACKET;
	addr.sll_protocol = net_htons(NET_ETH_PTYPE_PTP);
	addr.sll_ifindex = ifindex;

	if (zsock_bind(socket, (struct net_sockaddr *)&addr, sizeof(addr))) {
		LOG_ERR("Failed to bind L2 PTP socket");
		zsock_close(socket);
		return -1;
	}

	if (zsock_setsockopt(socket, ZSOCK_SOL_SOCKET, ZSOCK_SO_TIMESTAMPING, &ts_mask,
			     sizeof(ts_mask))) {
		LOG_ERR("Failed to set SO_TIMESTAMPING on L2 socket");
		zsock_close(socket);
		return -1;
	}

	return socket;
}

static int transport_send_udp(int socket, int port, void *buf, int length,
			      struct net_sockaddr *addr)
{
	struct net_sockaddr m_addr;
	net_socklen_t addrlen;
	int cnt;

	if (!addr) {
		if (IS_ENABLED(CONFIG_PTP_UDP_IPv4_PROTOCOL)) {
			m_addr.sa_family = NET_AF_INET;
			net_sin(&m_addr)->sin_port = net_htons(port);
			net_sin(&m_addr)->sin_addr.s_addr = mcast_addr_ipv4.s_addr;

		} else if (IS_ENABLED(CONFIG_PTP_UDP_IPv6_PROTOCOL)) {
			m_addr.sa_family = NET_AF_INET6;
			net_sin6(&m_addr)->sin6_port = net_htons(port);
			memcpy(&net_sin6(&m_addr)->sin6_addr, &mcast_addr_ipv6,
			       sizeof(struct net_in6_addr));
		}
		addr = &m_addr;
	}

	addrlen = IS_ENABLED(CONFIG_PTP_UDP_IPv4_PROTOCOL) ? sizeof(struct net_sockaddr_in)
							   : sizeof(struct net_sockaddr_in6);
	cnt = zsock_sendto(socket, buf, length, 0, addr, addrlen);
	if (cnt < 1) {
		LOG_ERR("Failed to send message");
		return -EFAULT;
	}

	return cnt;
}

static int transport_send_l2(struct ptp_port *port, int socket, void *buf, int length)
{
	struct net_sockaddr_ll addr = {0};
	int ifindex = net_if_get_by_iface(port->iface);
	int cnt;

	if (ifindex < 0) {
		LOG_ERR("Failed to obtain interface index");
		return -EFAULT;
	}

	addr.sll_family = NET_AF_PACKET;
	addr.sll_protocol = net_htons(NET_ETH_PTYPE_PTP);
	addr.sll_ifindex = ifindex;
	addr.sll_halen = PTP_L2_ADDR_LEN;
	memcpy(addr.sll_addr, mcast_addr_l2, sizeof(mcast_addr_l2));

	cnt = zsock_sendto(socket, buf, length, 0, (struct net_sockaddr *)&addr, sizeof(addr));
	if (cnt < 1) {
		LOG_ERR("Failed to send L2 message");
		return -EFAULT;
	}

	return cnt;
}

int ptp_transport_open(struct ptp_port *port)
{
	static const int socket_ports[] = {PTP_SOCKET_PORT_EVENT, PTP_SOCKET_PORT_GENERAL};
	int socket;

	if (IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		socket = transport_l2_open(port->iface);
		if (socket < 0) {
			return -1;
		}

		port->socket[PTP_SOCKET_EVENT] = socket;
		port->socket[PTP_SOCKET_GENERAL] = -1;

		return 0;
	}

	for (int i = 0; i < PTP_SOCKET_CNT; i++) {
		socket = IS_ENABLED(CONFIG_PTP_UDP_IPv4_PROTOCOL)
				 ? transport_udp_ipv4_open(port->iface, socket_ports[i])
				 : transport_udp_ipv6_open(port->iface, socket_ports[i]);

		if (socket == -1) {
			if (i == PTP_SOCKET_GENERAL) {
				zsock_close(port->socket[PTP_SOCKET_EVENT]);
				port->socket[PTP_SOCKET_EVENT] = -1;
			}

			return -1;
		}

		port->socket[i] = socket;
	}

	if (transport_join_multicast(port)) {
		ptp_transport_close(port);
		return -1;
	}

	return 0;
}

int ptp_transport_close(struct ptp_port *port)
{
	if (IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		if (port->socket[PTP_SOCKET_EVENT] >= 0 &&
		    zsock_close(port->socket[PTP_SOCKET_EVENT])) {
			LOG_ERR("Failed to close socket on PTP Port %d",
				port->port_ds.id.port_number);
			return -1;
		}

		port->socket[PTP_SOCKET_EVENT] = -1;
		port->socket[PTP_SOCKET_GENERAL] = -1;

		return 0;
	}

	for (int i = 0; i < PTP_SOCKET_CNT; i++) {

		if (port->socket[i] >= 0) {
			if (zsock_close(port->socket[i])) {
				LOG_ERR("Failed to close socket on PTP Port %d",
					port->port_ds.id.port_number);
				return -1;
			}
		}

		port->socket[i] = -1;
	}

	return 0;
}

int ptp_transport_send(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx)
{
	__ASSERT(PTP_SOCKET_CNT > idx, "Invalid socket index");

	static const int socket_port[] = {PTP_SOCKET_PORT_EVENT, PTP_SOCKET_PORT_GENERAL};
	int length = net_ntohs(msg->header.msg_length);

	if (IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		return transport_send_l2(port, port->socket[PTP_SOCKET_EVENT], msg, length);
	}

	return transport_send_udp(port->socket[idx], socket_port[idx], msg, length, NULL);
}

int ptp_transport_sendto(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx)
{
	__ASSERT(PTP_SOCKET_CNT > idx, "Invalid socket index");

	static const int socket_port[] = {PTP_SOCKET_PORT_EVENT, PTP_SOCKET_PORT_GENERAL};
	int length = net_ntohs(msg->header.msg_length);

	if (IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		return transport_send_l2(port, port->socket[PTP_SOCKET_EVENT], msg, length);
	}

	return transport_send_udp(port->socket[idx], socket_port[idx], msg, length, &msg->addr);
}

static void transport_init_recv_msghdr(struct ptp_msg *msg, struct net_msghdr *msghdr,
				       struct net_iovec *iov, uint8_t *ctrl, size_t ctrl_len)
{
	*iov = (struct net_iovec){
		.iov_base = msg,
		.iov_len = sizeof(msg->mtu),
	};

	*msghdr = (struct net_msghdr){
		.msg_iov = iov,
		.msg_iovlen = 1,
		.msg_control = ctrl,
		.msg_controllen = ctrl_len,
	};
}

static bool transport_extract_rx_timestamp(struct ptp_msg *msg, struct net_msghdr *msghdr)
{
	for (struct net_cmsghdr *cmsg = NET_CMSG_FIRSTHDR(msghdr); cmsg != NULL;
	     cmsg = NET_CMSG_NXTHDR(msghdr, cmsg)) {
		if (cmsg->cmsg_level == ZSOCK_SOL_SOCKET &&
		    cmsg->cmsg_type == ZSOCK_SO_TIMESTAMPING) {
			memcpy(&msg->timestamp.host, NET_CMSG_DATA(cmsg),
			       sizeof(struct net_ptp_time));
			return true;
		}
	}

	return false;
}

static void transport_set_host_timestamp_now(struct ptp_msg *msg)
{
	int64_t current = k_uptime_get();

	msg->timestamp.host.second = (uint64_t)(current / MSEC_PER_SEC);
	msg->timestamp.host.nanosecond = (current % MSEC_PER_SEC) * NSEC_PER_MSEC;
}

static void transport_finalize_l2_rx_timestamp(struct ptp_port *port, struct ptp_msg *msg,
					       bool rx_ts_found)
{
	const struct device *phc = net_eth_get_ptp_clock(port->iface);

	msg->rx_timestamp_valid = rx_ts_found;

	if (!msg->rx_timestamp_valid && phc && ptp_clock_get(phc, &msg->timestamp.host) == 0) {
		msg->rx_timestamp_valid = true;
	}

	if (!msg->rx_timestamp_valid) {
		transport_set_host_timestamp_now(msg);
	}
}

static int transport_recv_l2_msg(struct ptp_port *port, struct ptp_msg *msg)
{
	bool recvmsg_ok = false;
	uint8_t ctrl[NET_CMSG_SPACE(sizeof(struct net_ptp_time))] = {0};
	struct net_msghdr msghdr;
	struct net_iovec iov;
	int64_t now;
	int cnt;

	transport_init_recv_msghdr(msg, &msghdr, &iov, ctrl, sizeof(ctrl));

	now = k_uptime_get();

	if (!port->l2_try_recvmsg && now >= port->l2_recvmsg_retry_at) {
		port->l2_try_recvmsg = true;
	}

	if (port->l2_try_recvmsg) {
		cnt = zsock_recvmsg(port->socket[PTP_SOCKET_EVENT], &msghdr, ZSOCK_MSG_DONTWAIT);
		if (cnt < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return 0;
			}

			if (!port->l2_recvmsg_fallback_warned) {
				LOG_WRN("L2 recvmsg timestamping failed (errno %d), fallback to "
					"recvfrom() until recvmsg retry",
					errno);
				port->l2_recvmsg_fallback_warned = true;
			}

			port->l2_try_recvmsg = false;
			port->l2_recvmsg_retry_at = now + PTP_L2_RECVMSG_RETRY_MS;
		} else {
			recvmsg_ok = true;
			port->l2_recvmsg_retry_at = 0;
		}
	}

	if (!recvmsg_ok) {
		cnt = zsock_recvfrom(port->socket[PTP_SOCKET_EVENT], msg, sizeof(msg->mtu),
				     ZSOCK_MSG_DONTWAIT, NULL, NULL);
		if (cnt < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return 0;
			}

			LOG_ERR("Failed receive L2 PTP message (errno %d)", errno);
			return cnt;
		}
	}

	transport_finalize_l2_rx_timestamp(
		port, msg, recvmsg_ok && transport_extract_rx_timestamp(msg, &msghdr));

	return cnt;
}

static int transport_recv_udp_msg(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx)
{
	uint8_t ctrl[NET_CMSG_SPACE(sizeof(struct net_ptp_time))] = {0};
	struct net_msghdr msghdr;
	struct net_iovec iov;
	int cnt;

	transport_init_recv_msghdr(msg, &msghdr, &iov, ctrl, sizeof(ctrl));

	cnt = zsock_recvmsg(port->socket[idx], &msghdr, ZSOCK_MSG_DONTWAIT);
	if (cnt < 0) {
		LOG_ERR("Failed receive PTP message");
		return cnt;
	}

	msg->rx_timestamp_valid = transport_extract_rx_timestamp(msg, &msghdr);

	return cnt;
}

int ptp_transport_recv(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx)
{
	__ASSERT(PTP_SOCKET_CNT > idx, "Invalid socket index");

	if (IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		return transport_recv_l2_msg(port, msg);
	}

	return transport_recv_udp_msg(port, msg, idx);
}

int ptp_transport_protocol_addr(struct ptp_port *port, uint8_t *addr)
{
	__ASSERT_NO_MSG(addr);

	int length = 0;

	if (IS_ENABLED(CONFIG_PTP_UDP_IPv4_PROTOCOL)) {
		struct net_in_addr *ip =
			net_if_ipv4_get_global_addr(port->iface, NET_ADDR_PREFERRED);

		if (ip) {
			length = NET_IPV4_ADDR_SIZE;
			memcpy(addr, &ip->s_addr, length);
		}
	} else if (IS_ENABLED(CONFIG_PTP_UDP_IPv6_PROTOCOL)) {
		struct net_in6_addr *ip =
			net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &port->iface);

		if (ip) {
			length = NET_IPV6_ADDR_SIZE;
			memcpy(addr, ip, length);
		}
	} else if (IS_ENABLED(CONFIG_PTP_IEEE_802_3_PROTOCOL)) {
		struct net_linkaddr *lladdr = net_if_get_link_addr(port->iface);

		if (lladdr && lladdr->len == NET_ETH_ADDR_LEN) {
			length = NET_ETH_ADDR_LEN;
			memcpy(addr, lladdr->addr, length);
		}
	} else {
		length = 0;
	}

	return length;
}

struct net_linkaddr *ptp_transport_physical_addr(struct ptp_port *port)
{
	return net_if_get_link_addr(port->iface);
}
