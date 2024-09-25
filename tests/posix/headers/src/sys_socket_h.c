/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <sys/socket.h>
#else
#include <zephyr/posix/sys/socket.h>
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
/**
 * @brief existence test for `<sys/socket.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_socket.h.html">sys/socket.h</a>
 */
ZTEST(posix_headers, test_sys_socket_h)
{
	struct cmsghdr cmsg = {0};
	struct msghdr mhdr = {0};

	zassert_true(sizeof(socklen_t) >= sizeof(uint32_t));
	zassert_true((sa_family_t)-1 >= 0);

	zassert_not_equal(-1, offsetof(struct sockaddr, sa_family));
	/*
	 * FIXME: in zephyr, we define struct sockaddr in <zephyr/net/net_ip.h>
	 * The sa_data field is defined (incorrectly) as data.
	 * Fixing that is a (possibly breaking) tree-wide change.
	 */
	/* zassert_not_equal(-1, offsetof(struct sockaddr, sa_data)); */ /* not implemented */

	zassert_not_equal(-1, offsetof(struct sockaddr_storage, ss_family));
	zassert_equal(offsetof(struct sockaddr, sa_family),
		      offsetof(struct sockaddr_storage, ss_family));

	zassert_not_equal(-1, offsetof(struct msghdr, msg_name));
	zassert_not_equal(-1, offsetof(struct msghdr, msg_namelen));
	zassert_not_equal(-1, offsetof(struct msghdr, msg_iov));
	zassert_not_equal(-1, offsetof(struct msghdr, msg_iovlen));
	zassert_not_equal(-1, offsetof(struct msghdr, msg_control));
	zassert_not_equal(-1, offsetof(struct msghdr, msg_controllen));
	zassert_not_equal(-1, offsetof(struct msghdr, msg_flags));

	zassert_not_equal(-1, offsetof(struct cmsghdr, cmsg_len));
	zassert_not_equal(-1, offsetof(struct cmsghdr, cmsg_level));
	zassert_not_equal(-1, offsetof(struct cmsghdr, cmsg_type));

	CMSG_DATA(&cmsg);
	CMSG_NXTHDR(&mhdr, &cmsg);
	CMSG_FIRSTHDR(&mhdr);

	zassert_not_equal(-1, offsetof(struct linger, l_onoff));
	zassert_not_equal(-1, offsetof(struct linger, l_linger));

	zassert_not_equal(-1, SOCK_DGRAM);
	zassert_not_equal(-1, SOCK_RAW);
	/* zassert_not_equal(-1, SOCK_SEQPACKET); */ /* not implemented */
	zassert_not_equal(-1, SOCK_STREAM);

	zassert_not_equal(-1, SO_ACCEPTCONN);
	zassert_not_equal(-1, SO_BROADCAST);
	zassert_not_equal(-1, SO_DEBUG);
	zassert_not_equal(-1, SO_DONTROUTE);
	zassert_not_equal(-1, SO_ERROR);
	zassert_not_equal(-1, SO_KEEPALIVE);
	zassert_not_equal(-1, SO_LINGER);
	zassert_not_equal(-1, SO_OOBINLINE);
	zassert_not_equal(-1, SO_RCVBUF);
	zassert_not_equal(-1, SO_RCVLOWAT);
	zassert_not_equal(-1, SO_RCVTIMEO);
	zassert_not_equal(-1, SO_REUSEADDR);
	zassert_not_equal(-1, SO_SNDBUF);
	zassert_not_equal(-1, SO_SNDLOWAT);
	zassert_not_equal(-1, SO_SNDTIMEO);
	zassert_not_equal(-1, SO_TYPE);

	zassert_not_equal(-1, SOMAXCONN);

	/* zassert_not_equal(-1, MSG_CTRUNC); */ /* not implemented */
	/* zassert_not_equal(-1, MSG_DONTROUTE); */ /* not implemented */
	/* zassert_not_equal(-1, MSG_EOR); */ /* not implemented */
	/* zassert_not_equal(-1, MSG_OOB); */ /* not implemented */
	/* zassert_not_equal(-1, MSG_NOSIGNAL); */ /* not implemented */
	zassert_not_equal(-1, MSG_PEEK);
	zassert_not_equal(-1, MSG_TRUNC);
	zassert_not_equal(-1, MSG_WAITALL);

	zassert_not_equal(-1, AF_INET);
	zassert_not_equal(-1, AF_INET6);
	zassert_not_equal(-1, AF_UNIX);
	zassert_not_equal(-1, AF_UNSPEC);

	zassert_not_equal(-1, SHUT_RD);
	zassert_not_equal(-1, SHUT_RDWR);
	zassert_not_equal(-1, SHUT_WR);

	if (IS_ENABLED(CONFIG_POSIX_NETWORKING)) {
		zassert_not_null(accept);
		zassert_not_null(bind);
		zassert_not_null(connect);
		zassert_not_null(getpeername);
		zassert_not_null(getsockname);
		zassert_not_null(listen);
		zassert_not_null(recv);
		zassert_not_null(recvfrom);
		/* zassert_not_null(recvmsg); */ /* not implemented */
		zassert_not_null(send);
		zassert_not_null(sendmsg);
		zassert_not_null(sendto);
		zassert_not_null(setsockopt);
		zassert_not_null(shutdown);
		zassert_not_null(sockatmark);
		zassert_not_null(socket);
		zassert_not_null(socketpair);
	}
}
#pragma GCC diagnostic pop
