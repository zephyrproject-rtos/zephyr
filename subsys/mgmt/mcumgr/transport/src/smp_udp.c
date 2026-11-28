/*
 * Copyright (c) 2019-2020, Prevas A/S
 * Copyright (c) 2022-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief UDP transport for the mcumgr SMP protocol.
 */

#include <assert.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_udp.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <errno.h>

#if defined(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS)
#include <zephyr/net/tls_credentials.h>
#endif

#include <mgmt/mcumgr/transport/smp_internal.h>

#define LOG_LEVEL CONFIG_MCUMGR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smp_udp);

BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_UDP_MTU != 0, "CONFIG_MCUMGR_TRANSPORT_UDP_MTU must be > 0");

#if !defined(CONFIG_MCUMGR_TRANSPORT_UDP_IPV4) && !defined(CONFIG_MCUMGR_TRANSPORT_UDP_IPV6)
BUILD_ASSERT(0, "Either IPv4 or IPv6 SMP must be enabled for the MCUmgr UDP SMP transport using "
		"CONFIG_MCUMGR_TRANSPORT_UDP_IPV4 or CONFIG_MCUMGR_TRANSPORT_UDP_IPV6");
#endif

/* What a reply needs, kept in the user data of the request's buffer: the
 * sender, and the interface and local address the request came in on. With two
 * interfaces on one subnet, replying by destination alone could leave through
 * the other interface, from its address, and the client would drop the reply.
 * ifindex is 0 when unknown: an SMP client request, or a socket without
 * PKTINFO. Without CONFIG_MCUMGR_TRANSPORT_UDP_PKTINFO only the sender is kept:
 * with DTLS, the TLS layer replies from the local address of each session
 * itself, and native simulator offloaded sockets leave the choice to the host
 * (their addresses are also too large to fit more).
 */
struct smp_udp_ud {
	struct net_sockaddr_storage remote;
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_PKTINFO
	union {
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
		struct net_in_addr in;
#endif
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
		struct net_in6_addr in6;
#endif
	} local;
	int ifindex;
#endif
};

BUILD_ASSERT(sizeof(struct smp_udp_ud) <= CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE,
	     "CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE must be >= sizeof(struct smp_udp_ud)");

enum proto_type {
	PROTOCOL_IPV4 = 0,
	PROTOCOL_IPV6,
};

struct config {
	int sock;
	enum proto_type proto;
	struct k_sem network_ready_sem;
	struct smp_transport smp_transport;
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_PKTINFO
	bool pktinfo;
#endif
	char recv_buffer[CONFIG_MCUMGR_TRANSPORT_UDP_MTU];
	struct k_thread thread;
	K_KERNEL_STACK_MEMBER(stack, CONFIG_MCUMGR_TRANSPORT_UDP_STACK_SIZE);
};

struct configs {
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	struct config ipv4;
#ifdef CONFIG_SMP_CLIENT
	struct smp_client_transport_entry ipv4_transport;
#endif
#endif
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	struct config ipv6;
#ifdef CONFIG_SMP_CLIENT
	struct smp_client_transport_entry ipv6_transport;
#endif
#endif
};

static bool threads_created;

static struct configs smp_udp_configs;

static struct net_mgmt_event_callback smp_udp_mgmt_cb;

static const char *smp_udp_proto_to_name(enum proto_type proto)
{
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	if (proto == PROTOCOL_IPV4) {
		return "IPv4";
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	if (proto == PROTOCOL_IPV6) {
		return "IPv6";
	}
#endif

	return "??";
}

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_PKTINFO
/* Send through the interface and from the local address of the request, with
 * IP_PKTINFO or IPV6_PKTINFO. An unspecified local address (a request to a
 * multicast group) lets the interface pick one of its own.
 */
static int smp_udp_sendmsg(int sock, struct net_buf *nb, const struct smp_udp_ud *ud,
			   net_socklen_t addr_len)
{
	union {
		struct net_cmsghdr hdr;
		uint8_t buf[NET_CMSG_SPACE(sizeof(struct net_in6_pktinfo))];
	} cmsg_storage = {0};
	struct net_cmsghdr *cmsg = &cmsg_storage.hdr;
	struct net_iovec io_vec = {.iov_base = nb->data, .iov_len = nb->len};
	struct net_msghdr msg = {
		.msg_name = (void *)&ud->remote,
		.msg_namelen = addr_len,
		.msg_iov = &io_vec,
		.msg_iovlen = 1,
		.msg_control = cmsg_storage.buf,
	};

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	if (ud->remote.ss_family == NET_AF_INET6) {
		struct net_in6_pktinfo info = {
			.ipi6_addr = ud->local.in6,
			.ipi6_ifindex = (unsigned int)ud->ifindex,
		};

		cmsg->cmsg_level = NET_IPPROTO_IPV6;
		cmsg->cmsg_type = ZSOCK_IPV6_PKTINFO;
		cmsg->cmsg_len = NET_CMSG_LEN(sizeof(info));
		memcpy(NET_CMSG_DATA(cmsg), &info, sizeof(info));
		msg.msg_controllen = NET_CMSG_SPACE(sizeof(info));
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	if (ud->remote.ss_family == NET_AF_INET) {
		struct net_in_pktinfo info = {
			.ipi_ifindex = (unsigned int)ud->ifindex,
			.ipi_spec_dst = ud->local.in,
		};

		cmsg->cmsg_level = NET_IPPROTO_IP;
		cmsg->cmsg_type = ZSOCK_IP_PKTINFO;
		cmsg->cmsg_len = NET_CMSG_LEN(sizeof(info));
		memcpy(NET_CMSG_DATA(cmsg), &info, sizeof(info));
		msg.msg_controllen = NET_CMSG_SPACE(sizeof(info));
	}
#endif

	return zsock_sendmsg(sock, &msg, 0);
}
#endif

/* The reply goes to the peer with the length of its address: a DTLS server
 * socket matches the session by the exact address length, and
 * sizeof(struct net_sockaddr) is larger than an IPv4 address.
 */
static int smp_udp_tx(int sock, struct net_buf *nb, net_socklen_t addr_len)
{
	const struct smp_udp_ud *ud = net_buf_user_data(nb);
	int ret;

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_PKTINFO
	if (ud->ifindex != 0) {
		ret = smp_udp_sendmsg(sock, nb, ud, addr_len);
	} else
#endif
	{
		ret = zsock_sendto(sock, nb->data, nb->len, 0, net_sad(&ud->remote), addr_len);
	}

	if (ret < 0) {
		if (errno == ENOMEM) {
			ret = MGMT_ERR_EMSGSIZE;
		} else {
			ret = MGMT_ERR_EINVAL;
		}
	} else {
		ret = MGMT_ERR_EOK;
	}

	smp_packet_free(nb);

	return ret;
}

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
static int smp_udp4_tx(struct net_buf *nb)
{
	return smp_udp_tx(smp_udp_configs.ipv4.sock, nb, sizeof(struct net_sockaddr_in));
}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
static int smp_udp6_tx(struct net_buf *nb)
{
	return smp_udp_tx(smp_udp_configs.ipv6.sock, nb, sizeof(struct net_sockaddr_in6));
}
#endif

static uint16_t smp_udp_get_mtu(const struct net_buf *nb)
{
	ARG_UNUSED(nb);

	return CONFIG_MCUMGR_TRANSPORT_UDP_MTU;
}

static int smp_udp_ud_copy(struct net_buf *dst, const struct net_buf *src)
{
	const struct smp_udp_ud *src_ud = net_buf_user_data(src);
	struct smp_udp_ud *dst_ud = net_buf_user_data(dst);

	memcpy(dst_ud, src_ud, sizeof(*dst_ud));

	return MGMT_ERR_EOK;
}

/* An SMP client request: sent by destination, with no interface of its own. */
static void smp_udp_ud_init(struct net_buf *nb, void *priv)
{
	struct smp_udp_ud *ud = net_buf_user_data(nb);
	const struct net_sockaddr *addr = priv;

	if (addr != NULL) {
		/* The address may be smaller than the storage, copy its family's size */
		memcpy(&ud->remote, addr,
		       MIN(net_family2size(addr->sa_family), sizeof(ud->remote)));
	}

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_PKTINFO
	memset(&ud->local, 0, sizeof(ud->local));
	ud->ifindex = 0;
#endif
}

static int create_socket(enum proto_type proto, int *sock)
{
	int tmp_sock;
	int err;
	struct net_sockaddr_storage addr_storage;
	struct net_sockaddr *addr = (struct net_sockaddr *)&addr_storage;
	net_socklen_t addr_len = 0;

#if defined(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS)
	int socket_role = ZSOCK_TLS_DTLS_ROLE_SERVER;
#endif

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_UDP_IPV4) &&
	    proto == PROTOCOL_IPV4) {
		struct net_sockaddr_in *addr4 = (struct net_sockaddr_in *)addr;

		addr_len = sizeof(*addr4);
		memset(addr4, 0, sizeof(*addr4));
		addr4->sin_family = NET_AF_INET;
		addr4->sin_port = net_htons(CONFIG_MCUMGR_TRANSPORT_UDP_PORT);
		addr4->sin_addr.s_addr = net_htonl(NET_INADDR_ANY);
	} else if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_UDP_IPV6) &&
		   proto == PROTOCOL_IPV6) {
		struct net_sockaddr_in6 *addr6 = (struct net_sockaddr_in6 *)addr;

		addr_len = sizeof(*addr6);
		memset(addr6, 0, sizeof(*addr6));
		addr6->sin6_family = NET_AF_INET6;
		addr6->sin6_port = net_htons(CONFIG_MCUMGR_TRANSPORT_UDP_PORT);
		addr6->sin6_addr = net_in6addr_any;
	}

#if defined(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS)
	tmp_sock = zsock_socket(addr->sa_family, NET_SOCK_DGRAM, NET_IPPROTO_DTLS_1_2);
#else
	tmp_sock = zsock_socket(addr->sa_family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
#endif
	err = errno;

	if (tmp_sock < 0) {
		LOG_ERR("Could not open receive socket (%s), err: %i",
			smp_udp_proto_to_name(proto), err);

		return -err;
	}

#if defined(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS)
	sec_tag_t sec_tag_list[] = {
		CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_TLS_TAG,
	};

	err = zsock_setsockopt(tmp_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, sec_tag_list,
			       sizeof(sec_tag_list));

	if (err < 0) {
		LOG_ERR("Failed to set UDP secure option: %d", errno);
		return err;
	}

	/* Set role to DTLS server */
	err = zsock_setsockopt(tmp_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_DTLS_ROLE, &socket_role,
			       sizeof(socket_role));

	if (err < 0) {
		LOG_ERR("Failed to set DTLS role secure option: %d", errno);
		return err;
	}

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_CLIENT_AUTH)) {
		int peer_verify = ZSOCK_TLS_PEER_VERIFY_REQUIRED;

		err = zsock_setsockopt(tmp_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY, &peer_verify,
				       sizeof(peer_verify));

		if (err < 0) {
			LOG_ERR("Failed to set DTLS peer verify secure option: %d", errno);
			return err;
		}
	}
#endif

	if (zsock_bind(tmp_sock, addr, addr_len) < 0) {
		err = errno;
		LOG_ERR("Could not bind to receive socket (%s), err: %i",
			smp_udp_proto_to_name(proto), err);

		zsock_close(tmp_sock);

		return -err;
	}

	*sock = tmp_sock;
	return 0;
}

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_PKTINFO
/* Learn the interface and local address of each request, to reply through them.
 * Not every offloaded socket supports it; without it, replies are sent by
 * destination, as before.
 */
static bool smp_udp_enable_pktinfo(const struct config *conf)
{
	int on = 1;
	int rc;

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_UDP_IPV6) && conf->proto == PROTOCOL_IPV6) {
		rc = zsock_setsockopt(conf->sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_RECVPKTINFO, &on,
				      sizeof(on));
	} else {
		rc = zsock_setsockopt(conf->sock, NET_IPPROTO_IP, ZSOCK_IP_PKTINFO, &on,
				      sizeof(on));
	}

	if (rc < 0) {
		LOG_WRN("No PKTINFO (%s), err: %i: replies are sent by destination only",
			smp_udp_proto_to_name(conf->proto), errno);
		return false;
	}

	return true;
}

/* Take the interface and local address of a request from its PKTINFO. For IPv4,
 * ipi_spec_dst is the address to reply from (one of the interface's own for a
 * multicast or broadcast request). For IPv6, a multicast destination leaves the
 * local address unspecified, so that the interface picks one of its own.
 */
static void smp_udp_ud_set_local(struct smp_udp_ud *ud, struct net_msghdr *msg)
{
	struct net_cmsghdr *cmsg;

	for (cmsg = NET_CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = NET_CMSG_NXTHDR(msg, cmsg)) {
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
		if (cmsg->cmsg_level == NET_IPPROTO_IPV6 && cmsg->cmsg_type == ZSOCK_IPV6_PKTINFO &&
		    cmsg->cmsg_len == NET_CMSG_LEN(sizeof(struct net_in6_pktinfo))) {
			struct net_in6_pktinfo info;

			memcpy(&info, NET_CMSG_DATA(cmsg), sizeof(info));
			if (!net_ipv6_is_addr_mcast(&info.ipi6_addr)) {
				ud->local.in6 = info.ipi6_addr;
			}

			ud->ifindex = (int)info.ipi6_ifindex;
			return;
		}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
		if (cmsg->cmsg_level == NET_IPPROTO_IP && cmsg->cmsg_type == ZSOCK_IP_PKTINFO &&
		    cmsg->cmsg_len == NET_CMSG_LEN(sizeof(struct net_in_pktinfo))) {
			struct net_in_pktinfo info;

			memcpy(&info, NET_CMSG_DATA(cmsg), sizeof(info));
			ud->local.in = info.ipi_spec_dst;
			ud->ifindex = (int)info.ipi_ifindex;
			return;
		}
#endif
	}
}
#endif

/* Receive one request into conf->recv_buffer, and what its reply needs into ud. */
static int smp_udp_recv(struct config *conf, struct smp_udp_ud *ud)
{
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_PKTINFO
	if (conf->pktinfo) {
		union {
			struct net_cmsghdr hdr;
			uint8_t buf[NET_CMSG_SPACE(sizeof(struct net_in6_pktinfo))];
		} cmsg_storage = {0};
		struct net_iovec io_vec = {
			.iov_base = conf->recv_buffer,
			.iov_len = CONFIG_MCUMGR_TRANSPORT_UDP_MTU,
		};
		struct net_msghdr msg = {
			.msg_name = &ud->remote,
			.msg_namelen = sizeof(ud->remote),
			.msg_iov = &io_vec,
			.msg_iovlen = 1,
			.msg_control = cmsg_storage.buf,
			.msg_controllen = sizeof(cmsg_storage.buf),
		};
		int len = zsock_recvmsg(conf->sock, &msg, 0);

		if (len > 0) {
			smp_udp_ud_set_local(ud, &msg);
		}

		return len;
	}
#endif

	net_socklen_t addr_len = sizeof(ud->remote);

	return zsock_recvfrom(conf->sock, conf->recv_buffer, CONFIG_MCUMGR_TRANSPORT_UDP_MTU, 0,
			      net_sad(&ud->remote), &addr_len);
}

static void smp_udp_receive_thread(void *p1, void *p2, void *p3)
{
	struct config *conf = (struct config *)p1;
	int rc;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)k_sem_take(&conf->network_ready_sem, K_FOREVER);
	rc = create_socket(conf->proto, &conf->sock);

	if (rc < 0) {
		return;
	}

	__ASSERT(rc >= 0, "Socket is invalid");
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_PKTINFO
	conf->pktinfo = smp_udp_enable_pktinfo(conf);
#endif
	LOG_INF("Started (%s)", smp_udp_proto_to_name(conf->proto));

	while (1) {
		struct smp_udp_ud ud = {0};
		int len = smp_udp_recv(conf, &ud);

		if (len > 0) {
			struct net_buf *nb;

			/* Store what the reply needs in user data */
			nb = smp_packet_alloc();
			if (!nb) {
				LOG_ERR("Failed to allocate mcumgr buffer");
				/* No free space, drop SMP frame */
				continue;
			}

			if (net_buf_tailroom(nb) < (size_t)len) {
				LOG_ERR("SMP frame (%d) exceeds mcumgr buffer (%zu)", len,
					net_buf_tailroom(nb));
				smp_packet_free(nb);
				continue;
			}

			net_buf_add_mem(nb, conf->recv_buffer, len);
			memcpy(net_buf_user_data(nb), &ud, sizeof(ud));

			smp_rx_req(&conf->smp_transport, nb);
		} else if (len < 0) {
			LOG_ERR("Receive error (%s): %i, %d", smp_udp_proto_to_name(conf->proto),
				errno, len);
		}
	}
}

static void smp_udp_open_iface(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	if (net_if_is_up(iface)) {
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
		if (net_if_flag_is_set(iface, NET_IF_IPV4) &&
		    k_thread_join(&smp_udp_configs.ipv4.thread, K_NO_WAIT) == -EBUSY) {
			k_sem_give(&smp_udp_configs.ipv4.network_ready_sem);
		}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
		if (net_if_flag_is_set(iface, NET_IF_IPV6) &&
		    k_thread_join(&smp_udp_configs.ipv6.thread, K_NO_WAIT) == -EBUSY) {
			k_sem_give(&smp_udp_configs.ipv6.network_ready_sem);
		}
#endif
	}
}

static void smp_udp_net_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				      struct net_if *iface)
{
	ARG_UNUSED(cb);

	if (mgmt_event == NET_EVENT_IF_UP) {
		smp_udp_open_iface(iface, NULL);
	}
}

static void create_thread(struct config *conf, const char *name)
{
	k_thread_create(&(conf->thread), conf->stack,
			K_KERNEL_STACK_SIZEOF(conf->stack),
			smp_udp_receive_thread, conf, NULL, NULL,
			CONFIG_MCUMGR_TRANSPORT_UDP_THREAD_PRIO, 0, K_FOREVER);

	k_thread_name_set(&(conf->thread), name);
	k_thread_start(&(conf->thread));
}

int smp_udp_open(void)
{
	bool started = false;

#if defined(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS)
	int rc;
	size_t len = 0;

	rc = tls_credential_get(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_TLS_TAG,
				TLS_CREDENTIAL_PUBLIC_CERTIFICATE, NULL, &len);

	if (rc == -ENOENT) {
		LOG_ERR("Missing DTLS public certificate credential");
		return rc;
	}

	len = 0;
	rc = tls_credential_get(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_TLS_TAG,
				TLS_CREDENTIAL_PRIVATE_KEY, NULL, &len);

	if (rc == -ENOENT) {
		LOG_ERR("Missing DTLS private key credential");
		return rc;
	}

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_CLIENT_AUTH)) {
		len = 0;
		rc = tls_credential_get(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_TLS_TAG,
					TLS_CREDENTIAL_CA_CERTIFICATE, NULL, &len);

		if (rc == -ENOENT) {
			LOG_ERR("Missing DTLS CA certificate credential");
			return rc;
		}
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	if (k_thread_join(&smp_udp_configs.ipv4.thread, K_NO_WAIT) == 0 ||
	    threads_created == false) {
		(void)k_sem_reset(&smp_udp_configs.ipv4.network_ready_sem);
		create_thread(&smp_udp_configs.ipv4, "smp_udp4");
		started = true;
	} else {
		LOG_ERR("IPv4 UDP MCUmgr thread is already running");
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	if (k_thread_join(&smp_udp_configs.ipv6.thread, K_NO_WAIT) == 0 ||
	    threads_created == false) {
		(void)k_sem_reset(&smp_udp_configs.ipv6.network_ready_sem);
		create_thread(&smp_udp_configs.ipv6, "smp_udp6");
		started = true;
	} else {
		LOG_ERR("IPv6 UDP MCUmgr thread is already running");
	}
#endif

	if (started) {
		/* One or more threads were started, check existing interfaces */
		threads_created = true;
		net_if_foreach(smp_udp_open_iface, NULL);
	}

	return 0;
}

int smp_udp_close(void)
{
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	if (k_thread_join(&smp_udp_configs.ipv4.thread, K_NO_WAIT) == -EBUSY) {
		k_thread_abort(&(smp_udp_configs.ipv4.thread));

		if (smp_udp_configs.ipv4.sock >= 0) {
			zsock_close(smp_udp_configs.ipv4.sock);
			smp_udp_configs.ipv4.sock = -1;
		}
	} else {
		LOG_ERR("IPv4 UDP MCUmgr thread is not running");
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	if (k_thread_join(&smp_udp_configs.ipv6.thread, K_NO_WAIT) == -EBUSY) {
		k_thread_abort(&(smp_udp_configs.ipv6.thread));

		if (smp_udp_configs.ipv6.sock >= 0) {
			zsock_close(smp_udp_configs.ipv6.sock);
			smp_udp_configs.ipv6.sock = -1;
		}
	} else {
		LOG_ERR("IPv6 UDP MCUmgr thread is not running");
	}
#endif

	return 0;
}

static void smp_udp_start(void)
{
	int rc;

	threads_created = false;

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	smp_udp_configs.ipv4.proto = PROTOCOL_IPV4;
	smp_udp_configs.ipv4.sock = -1;

	k_sem_init(&smp_udp_configs.ipv4.network_ready_sem, 0, 1);
	smp_udp_configs.ipv4.smp_transport.functions.output = smp_udp4_tx;
	TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	smp_udp_configs.ipv4.smp_transport.functions.get_mtu = smp_udp_get_mtu;
	TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	smp_udp_configs.ipv4.smp_transport.functions.ud_copy = smp_udp_ud_copy;
	smp_udp_configs.ipv4.smp_transport.functions.ud_init = smp_udp_ud_init;

	rc = smp_transport_init(&smp_udp_configs.ipv4.smp_transport);
#ifdef CONFIG_SMP_CLIENT
	if (rc == 0) {
		smp_udp_configs.ipv4_transport.smpt = &smp_udp_configs.ipv4.smp_transport;
		smp_udp_configs.ipv4_transport.smpt_type = SMP_UDP_IPV4_TRANSPORT;
		smp_client_transport_register(&smp_udp_configs.ipv4_transport);
	}
#endif
	if (rc) {
		LOG_ERR("Failed to register IPv4 UDP MCUmgr SMP transport: %d", rc);
	}
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	smp_udp_configs.ipv6.proto = PROTOCOL_IPV6;
	smp_udp_configs.ipv6.sock = -1;

	k_sem_init(&smp_udp_configs.ipv6.network_ready_sem, 0, 1);
	smp_udp_configs.ipv6.smp_transport.functions.output = smp_udp6_tx;
	TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	smp_udp_configs.ipv6.smp_transport.functions.get_mtu = smp_udp_get_mtu;
	TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	smp_udp_configs.ipv6.smp_transport.functions.ud_copy = smp_udp_ud_copy;
	smp_udp_configs.ipv6.smp_transport.functions.ud_init = smp_udp_ud_init;

	rc = smp_transport_init(&smp_udp_configs.ipv6.smp_transport);
#ifdef CONFIG_SMP_CLIENT
	if (rc == 0) {
		smp_udp_configs.ipv6_transport.smpt = &smp_udp_configs.ipv6.smp_transport;
		smp_udp_configs.ipv6_transport.smpt_type = SMP_UDP_IPV6_TRANSPORT;
		smp_client_transport_register(&smp_udp_configs.ipv6_transport);
	}
#endif

	if (rc) {
		LOG_ERR("Failed to register IPv6 UDP MCUmgr SMP transport: %d", rc);
	}
#endif

	net_mgmt_init_event_callback(&smp_udp_mgmt_cb, smp_udp_net_event_handler, NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&smp_udp_mgmt_cb);

#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_AUTOMATIC_INIT
	smp_udp_open();
#endif
}

#ifdef CONFIG_SMP_CLIENT
int smp_client_udp_set_host_addr(struct smp_client_object *obj, struct net_sockaddr *addr)
{
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV4
	if (obj->smpt == smp_udp_configs.ipv4_transport.smpt && addr->sa_family == NET_AF_INET) {
		smp_client_object_set_data(obj, addr);
		return 0;
	}
#endif
#ifdef CONFIG_MCUMGR_TRANSPORT_UDP_IPV6
	if (obj->smpt == smp_udp_configs.ipv6_transport.smpt && addr->sa_family == NET_AF_INET6) {
		smp_client_object_set_data(obj, addr);
		return 0;
	}
#endif

	return -EINVAL;
}
#endif

MCUMGR_HANDLER_DEFINE(smp_udp, smp_udp_start);
