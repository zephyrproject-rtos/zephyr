/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_transport, CONFIG_PTP_LOG_LEVEL);

#include <zephyr/kernel.h>

#include <zephyr/net/socket.h>

#include "transport.h"

#define INTERFACE_NAME_LEN (32)

#if CONFIG_PTP_UDP_IPv4_PROTOCOL
static struct in_addr mcast_addr = {{{224, 0, 1, 129}}};
#elif CONFIG_PTP_UDP_IPv6_PROTOCOL
static struct in6_addr mcast_addr = {{{0xff, 0xe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
				       0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x81}}};
#else
#error "Chosen PTP transport protocol not implemented"
#endif

static int transport_socket_open(struct net_if *iface, struct sockaddr *addr)
{
	static const int feature_on = 1;
	static const uint8_t priority = NET_PRIORITY_CA;
	static const uint8_t ts_mask = SOF_TIMESTAMPING_TX_HARDWARE | SOF_TIMESTAMPING_RX_HARDWARE;
	struct ifreq ifreq = { 0 };
	int cnt;
	int socket = zsock_socket(addr->sa_family, SOCK_DGRAM, IPPROTO_UDP);

	if (net_if_get_by_iface(iface) < 0) {
		LOG_ERR("Failed to obtain interface index");
		return -1;
	}

	if (socket < 0) {
		return -1;
	}

	if (zsock_setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &feature_on, sizeof(feature_on))) {
		LOG_ERR("Failed to set SO_REUSEADDR");
		goto error;
	}

	if (zsock_bind(socket, addr, sizeof(*addr))) {
		LOG_ERR("Failed to bind socket");
		goto error;
	}

	cnt = net_if_get_name(iface, ifreq.ifr_name, INTERFACE_NAME_LEN);
	if (cnt > 0 && zsock_setsockopt(socket,
					SOL_SOCKET,
					SO_BINDTODEVICE,
					ifreq.ifr_name,
					sizeof(ifreq.ifr_name))) {
		LOG_ERR("Failed to set socket binding to an interface");
		goto error;
	}

	if (zsock_setsockopt(socket, SOL_SOCKET, SO_TIMESTAMPING, &ts_mask, sizeof(ts_mask))) {
		LOG_ERR("Failed to set SO_TIMESTAMPING");
		goto error;
	}

	if (zsock_setsockopt(socket, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority))) {
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
		struct ip_mreqn mreqn = {0};

		memcpy(&mreqn.imr_multiaddr, &mcast_addr, sizeof(struct in_addr));
		mreqn.imr_ifindex = net_if_get_by_iface(port->iface);

		zsock_setsockopt(port->socket[1], IPPROTO_IP,
				 IP_ADD_MEMBERSHIP, &mreqn, sizeof(mreqn));
	} else {
		struct ipv6_mreq mreqn = {0};

		memcpy(&mreqn.ipv6mr_multiaddr, &mcast_addr, sizeof(struct in6_addr));
		mreqn.ipv6mr_ifindex = net_if_get_by_iface(port->iface);

		zsock_setsockopt(port->socket[0], IPPROTO_IPV6,
				 IPV6_ADD_MEMBERSHIP, &mreqn, sizeof(mreqn));
	}

	return 0;
}

static int transport_udp_ipv4_open(struct net_if *iface, uint16_t port)
{
	uint8_t tos;
	socklen_t length;
	int socket, ttl = 1;
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr = INADDR_ANY_INIT,
		.sin_port = htons(port),
	};

	socket = transport_socket_open(iface, (struct sockaddr *)&addr);
	if (socket < 0) {
		return -1;
	}

	if (zsock_setsockopt(socket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl))) {
		LOG_ERR("Failed to set ip multicast ttl socket option");
		goto error;
	}

	if (zsock_getsockopt(socket, IPPROTO_IP, IP_TOS, &tos, &length)) {
		tos = 0;
	}

	tos &= ~0xFC;
	tos |= CONFIG_PTP_DSCP_VALUE << 2;
	length = sizeof(tos);

	if (zsock_setsockopt(socket, IPPROTO_IP, IP_TOS, &tos, length)) {
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
	socklen_t length;
	int socket, hops = 1, feature_on = 1;
	struct sockaddr_in6 addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_port = htons(port)
	};

	socket = transport_socket_open(iface, (struct sockaddr *)&addr);
	if (socket < 0) {
		return -1;
	}

	if (zsock_setsockopt(socket,
			     IPPROTO_IPV6,
			     IPV6_RECVPKTINFO,
			     &feature_on,
			     sizeof(feature_on))) {
		LOG_ERR("Failed to set IPV6_RECVPKTINFO");
		goto error;
	}

	if (zsock_setsockopt(socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops))) {
		LOG_ERR("Failed to set ip multicast hops socket option");
		goto error;
	}

	if (zsock_getsockopt(socket, IPPROTO_IPV6, IPV6_TCLASS, &tclass, &length)) {
		tclass = 0;
	}

	tclass &= ~0xFC;
	tclass |= CONFIG_PTP_DSCP_VALUE << 2;
	length = sizeof(tclass);

	if (zsock_setsockopt(socket, IPPROTO_IPV6, IPV6_TCLASS, &tclass, length)) {
		LOG_WRN("Failed to set priority");
	}

	return socket;
error:
	zsock_close(socket);
	return -1;
}

static int transport_send(int socket, int port, void *buf, int length, struct sockaddr *addr)
{
	struct sockaddr m_addr;
	socklen_t addrlen;
	int cnt;

	if (!addr) {
		if (IS_ENABLED(CONFIG_PTP_UDP_IPv4_PROTOCOL)) {
			m_addr.sa_family = AF_INET;
			net_sin(&m_addr)->sin_port = htons(port);
			net_sin(&m_addr)->sin_addr.s_addr = mcast_addr.s_addr;

		} else if (IS_ENABLED(CONFIG_PTP_UDP_IPv6_PROTOCOL)) {
			m_addr.sa_family = AF_INET6;
			net_sin6(&m_addr)->sin6_port = htons(port);
			memcpy(&net_sin6(&m_addr)->sin6_addr,
			       &mcast_addr,
			       sizeof(struct in6_addr));
		}
		addr = &m_addr;
	}

	addrlen = IS_ENABLED(CONFIG_PTP_UDP_IPv4_PROTOCOL) ? sizeof(struct sockaddr_in) :
							     sizeof(struct sockaddr_in6);
	cnt = zsock_sendto(socket, buf, length, 0, addr, addrlen);
	if (cnt < 1) {
		LOG_ERR("Failed to send message");
		return -EFAULT;
	}

	return cnt;
}

int ptp_transport_open(struct ptp_port *port)
{
	static const int socket_ports[] = {PTP_SOCKET_PORT_EVENT, PTP_SOCKET_PORT_GENERAL};
	int socket;

	for (int i = 0; i < PTP_SOCKET_CNT; i++) {
		socket = IS_ENABLED(CONFIG_PTP_UDP_IPv4_PROTOCOL) ?
			transport_udp_ipv4_open(port->iface, socket_ports[i]) :
			transport_udp_ipv6_open(port->iface, socket_ports[i]);

		if (socket == -1) {
			if (i == PTP_SOCKET_GENERAL) {
				zsock_close(port->socket[PTP_SOCKET_EVENT]);
				port->socket[PTP_SOCKET_EVENT] = -1;
			}

			return -1;
		}

		port->socket[i] = socket;
	}

	return transport_join_multicast(port);
}

int ptp_transport_close(struct ptp_port *port)
{
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
	int length = ntohs(msg->header.msg_length);

	return transport_send(port->socket[idx], socket_port[idx], msg, length, NULL);
}

int ptp_transport_sendto(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx)
{
	__ASSERT(PTP_SOCKET_CNT > idx, "Invalid socket index");

	static const int socket_port[] = {PTP_SOCKET_PORT_EVENT, PTP_SOCKET_PORT_GENERAL};
	int length = ntohs(msg->header.msg_length);

	return transport_send(port->socket[idx], socket_port[idx], msg, length, &msg->addr);
}

int ptp_transport_recv(struct ptp_port *port, struct ptp_msg *msg, enum ptp_socket idx)
{
	__ASSERT(PTP_SOCKET_CNT > idx, "Invalid socket index");

	int cnt = 0;
	uint8_t ctrl[CMSG_SPACE(sizeof(struct net_ptp_time))] = {0};
	struct cmsghdr *cmsg;
	struct msghdr msghdr = {0};
	struct iovec iov = {
		.iov_base = msg,
		.iov_len = sizeof(msg->mtu),
	};

	msghdr.msg_iov = &iov;
	msghdr.msg_iovlen = 1;
	msghdr.msg_control = ctrl;
	msghdr.msg_controllen = sizeof(ctrl);

	cnt = zsock_recvmsg(port->socket[idx], &msghdr, ZSOCK_MSG_DONTWAIT);
	if (cnt < 0) {
		LOG_ERR("Failed receive PTP message");
	}

	for (cmsg = CMSG_FIRSTHDR(&msghdr); cmsg != NULL; cmsg = CMSG_NXTHDR(&msghdr, cmsg)) {
		if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING) {
			memcpy(&msg->timestamp.host, CMSG_DATA(cmsg), sizeof(struct net_ptp_time));
		}
	}

	return cnt;
}

int ptp_transport_protocol_addr(struct ptp_port *port, uint8_t *addr)
{
	__ASSERT_NO_MSG(addr);

	int length = 0;

	if (IS_ENABLED(CONFIG_PTP_UDP_IPv4_PROTOCOL)) {
		struct in_addr *ip = net_if_ipv4_get_global_addr(port->iface, NET_ADDR_PREFERRED);

		if (ip) {
			length = NET_IPV4_ADDR_SIZE;
			*addr = ip->s_addr;
		}
	} else if (IS_ENABLED(CONFIG_PTP_UDP_IPv6_PROTOCOL)) {
		struct in6_addr *ip = net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &port->iface);

		if (ip) {
			length = NET_IPV6_ADDR_SIZE;
			memcpy(addr, ip, length);
		}
	}

	return length;
}

struct net_linkaddr *ptp_transport_physical_addr(struct ptp_port *port)
{
	return net_if_get_link_addr(port->iface);
}
